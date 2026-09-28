import json
import math
import os
from pathlib import Path
import sys
import signal
import threading
import time
from collections import deque

import cv2
import numpy as np
import rclpy
from ament_index_python.packages import get_package_share_directory
from cv_bridge import CvBridge
from rclpy.executors import ExternalShutdownException, SingleThreadedExecutor
from rclpy.node import Node
from rclpy.signals import SignalHandlerOptions
from sensor_msgs.msg import Image
from std_msgs.msg import Float32, String

from .common import image_qos, positive, validate_classes


def measured_fps(timestamps):
    if len(timestamps) < 2:
        return 0.0
    return (len(timestamps) - 1) / max(timestamps[-1] - timestamps[0], 1e-9)


def draw_result(frame, detections, line1, line2):
    annotated = frame.copy()
    for detection in detections:
        x1, y1, x2, y2 = [int(round(v)) for v in detection['xyxy']]
        cv2.rectangle(annotated, (x1, y1), (x2, y2), (0, 220, 0), 2)
        label = f'{detection["class_id"]}: {detection["class_name"]} {detection["confidence"]:.2f}'
        cv2.putText(annotated, label, (max(0, x1), max(18, y1 - 7)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 220, 0), 2)
    cv2.rectangle(annotated, (0, 0), (annotated.shape[1], 52), (25, 25, 25), -1)
    for y, text in [(20, line1), (42, line2)]:
        cv2.putText(annotated, text, (8, y), cv2.FONT_HERSHEY_SIMPLEX,
                    0.5, (255, 255, 255), 1)
    return annotated


class DetectorNode(Node):
    def __init__(self):
        super().__init__('detector_node')
        self.declare_parameters('', [
            ('model_path', ''), ('device', 'cpu'), ('cpu_threads', 4),
            ('image_size', 640), ('inference_hz', 30.0),
            ('preview_hz', 30.0), ('max_box_age_ms', 250.0),
            ('image_topic', '/camera/image_raw'), ('result_image_topic', '/yolo/image'),
            ('detections_topic', '/yolo/detections'), ('latency_topic', '/yolo/latency_ms'),
            ('confidence_threshold', 0.5), ('detect_all_classes', True),
            ('class_ids', [0, 32]), ('class_names', ['person', 'sports ball']),
            ('show_window', True),
        ])
        p = lambda name: self.get_parameter(name).value
        hz = positive(p('inference_hz'), 'inference_hz')
        self.inference_period = 1.0 / hz
        self.preview_period = 1.0 / positive(p('preview_hz'), 'preview_hz')
        self.max_box_age_ms = positive(p('max_box_age_ms'), 'max_box_age_ms')
        self.confidence = p('confidence_threshold')
        if not math.isfinite(self.confidence) or not 0 <= self.confidence <= 1:
            raise ValueError('confidence_threshold must be between 0 and 1')
        self.image_size = positive(p('image_size'), 'image_size')
        self.device = p('device')
        model_path = Path(p('model_path')).expanduser() if p('model_path') else (
            Path(get_package_share_directory('yolo_detector')) / 'models/yolo26n.pt')
        if not model_path.is_file():
            raise FileNotFoundError(f'Model not found: {model_path}')
        import torch
        from ultralytics import YOLO
        torch.set_num_threads(positive(p('cpu_threads'), 'cpu_threads'))
        self.model = YOLO(str(model_path), task='detect')
        self.names = dict(self.model.names)
        selected = validate_classes(p('class_ids'), p('class_names'), self.names)
        self.classes = None if p('detect_all_classes') else selected
        self.predict_options = dict(device=self.device, imgsz=self.image_size,
                                    conf=self.confidence, classes=self.classes, verbose=False)
        # 모델 준비 비용은 실제 프레임의 지연시간 측정에서 분리합니다.
        self.model.predict(np.zeros((self.image_size, self.image_size, 3), dtype=np.uint8),
                           **self.predict_options)
        self.bridge = CvBridge()
        self.lock = threading.Lock()
        self.pending = None
        self.preview_message = None
        self.last_result = None
        self.receive_times = deque(maxlen=30)
        self.inference_times = deque(maxlen=30)
        self.preview_times = deque(maxlen=30)
        self.last_preview = 0.0
        self.preview_fps = 0.0
        self.show_window = p('show_window')
        if self.show_window and not (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')):
            self.get_logger().warning('No desktop display found; publishing result images without a window.')
            self.show_window = False
        self.stop_requested = False
        self.window_created = False
        self.image_pub = self.create_publisher(Image, p('result_image_topic'), image_qos())
        self.detections_pub = self.create_publisher(String, p('detections_topic'), 10)
        self.latency_pub = self.create_publisher(Float32, p('latency_topic'), 10)
        self.subscription = self.create_subscription(
            Image, p('image_topic'), self.receive, image_qos())
        # 추론은 전용 스레드에서 직렬 실행합니다. ROS 타이머의 밀린 작업을 만들지 않습니다.
        self.inference_stop = threading.Event()
        self.worker_error = None
        self.worker = threading.Thread(target=self.inference_loop, daemon=True)
        self.worker.start()
        self.get_logger().info(f'Model ready: {model_path.name}; classes={self.classes or "all"}; target={hz:g} Hz')

    def inference_loop(self):
        try:
            while not self.inference_stop.is_set():
                start = time.perf_counter()
                self.process()
                self.inference_stop.wait(max(0, self.inference_period - (time.perf_counter() - start)))
        except Exception as exc:
            self.worker_error = exc

    def stop_inference(self):
        self.inference_stop.set()
        self.worker.join()

    def receive(self, message):
        with self.lock:
            self.pending = message
            self.preview_message = message
            self.receive_times.append(time.perf_counter())

    def process(self):
        with self.lock:
            message, self.pending = self.pending, None
        if message is None:
            return
        frame = self.bridge.imgmsg_to_cv2(message, desired_encoding='bgr8')
        start = time.perf_counter()
        result = self.model.predict(frame, **self.predict_options)[0]
        inference_ms = (time.perf_counter() - start) * 1000
        # Ultralytics가 전처리/출력 해석/원본 좌표 복원을 담당합니다.
        boxes = result.boxes
        detections = []
        for xyxy, class_id, score in zip(boxes.xyxy.cpu().tolist(),
                                         boxes.cls.int().cpu().tolist(), boxes.conf.cpu().tolist()):
            name = self.names[class_id]
            detections.append({'class_id': class_id, 'class_name': name,
                               'confidence': score, 'xyxy': xyxy})
        now = time.perf_counter()
        self.inference_times.append(now)
        fps = measured_fps(self.inference_times)
        stamp_ns = message.header.stamp.sec * 1_000_000_000 + message.header.stamp.nanosec
        age_ms = (self.get_clock().now().nanoseconds - stamp_ns) / 1e6
        annotated = draw_result(frame, detections,
                                f'Predict: {inference_ms:.1f} ms | Infer: {fps:.1f} FPS',
                                f'Frame age: {age_ms:.1f} ms | Objects: {len(detections)}')
        output = self.bridge.cv2_to_imgmsg(annotated, encoding='bgr8')
        output.header = message.header
        self.image_pub.publish(output)
        self.latency_pub.publish(Float32(data=inference_ms))
        with self.lock:
            camera_fps = measured_fps(self.receive_times)
        payload = {'header': {'stamp': {'sec': message.header.stamp.sec,
                                        'nanosec': message.header.stamp.nanosec},
                              'frame_id': message.header.frame_id},
                   'inference_ms': inference_ms, 'frame_age_ms': age_ms,
                   'camera_fps': camera_fps, 'inference_fps': fps,
                   'preview_fps': self.preview_fps, 'detections': detections}
        self.detections_pub.publish(String(data=json.dumps(payload, allow_nan=False)))
        with self.lock:
            self.last_result = (detections, stamp_ns, frame.shape[:2], inference_ms, fps)

    def update_window(self):
        # OpenCV의 GUI 작업은 메인 스레드에서만 실행합니다.
        if not self.show_window:
            return
        now = time.perf_counter()
        with self.lock:
            message = None
            if now - self.last_preview >= self.preview_period:
                message, self.preview_message = self.preview_message, None
            snapshot = self.last_result
            camera_fps = measured_fps(self.receive_times)
        if message is not None:
            frame = self.bridge.imgmsg_to_cv2(message, desired_encoding='bgr8')
            self.last_preview = now
            self.preview_times.append(now)
            self.preview_fps = measured_fps(self.preview_times)
            detections, inference_ms, inference_fps, age = [], 0.0, 0.0, None
            if snapshot is not None:
                candidates, stamp_ns, shape, inference_ms, inference_fps = snapshot
                age = (self.get_clock().now().nanoseconds - stamp_ns) / 1e6
                if shape == frame.shape[:2] and 0 <= age <= self.max_box_age_ms:
                    detections = candidates
            # 영상은 최신 프레임, 박스는 최근 추론 결과입니다. 추적 결과가 아닙니다.
            box_age = 'waiting' if age is None else f'{age:.0f} ms'
            preview = draw_result(frame, detections,
                f'Camera: {camera_fps:.1f} | Infer: {inference_fps:.1f} | View: {self.preview_fps:.1f} FPS',
                f'Predict: {inference_ms:.1f} ms | Box age: {box_age} | Q: quit')
            cv2.imshow('YOLO26 ROS2', preview)
            if not self.window_created:
                self.get_logger().info('Result window opened. Press Q or Esc to quit.')
            self.window_created = True
        if self.window_created:
            if cv2.waitKey(1) & 0xFF in (ord('q'), 27):
                self.stop_requested = True
            elif cv2.getWindowProperty('YOLO26 ROS2', cv2.WND_PROP_VISIBLE) < 1:
                self.stop_requested = True


def main(args=None):
    rclpy.init(args=args, signal_handler_options=SignalHandlerOptions.NO)
    node = None
    executor = None
    code = 0
    try:
        node = DetectorNode()
        executor = SingleThreadedExecutor()
        executor.add_node(node)
        while rclpy.ok() and not node.stop_requested:
            executor.spin_once(timeout_sec=0.005)
            node.update_window()
            if node.worker_error is not None:
                raise RuntimeError(f'Inference failed: {node.worker_error}') from node.worker_error
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    except Exception as exc:
        print(f'Detector error: {exc}', file=sys.stderr)
        code = 1
    finally:
        # Launch와 터미널에서 SIGINT가 중복되어도 자원 정리를 끝냅니다.
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        if node is not None:
            node.stop_inference()
        if executor is not None:
            executor.remove_node(node)
            executor.shutdown()
        if node is not None:
            if node.window_created:
                cv2.destroyAllWindows()
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
    if code:
        raise SystemExit(code)
