import sys
import signal
import threading
import time

import cv2
import rclpy
from cv_bridge import CvBridge
from rclpy.node import Node
from rclpy.executors import ExternalShutdownException
from rclpy.signals import SignalHandlerOptions
from sensor_msgs.msg import Image

from .common import image_qos, positive


class CameraNode(Node):
    def __init__(self):
        super().__init__('camera_node')
        self.declare_parameters('', [
            ('camera_source', '0'), ('publish_hz', 30.0),
            ('width', 640), ('height', 480), ('frame_id', 'camera'),
            ('image_topic', '/camera/image_raw'),
        ])
        p = lambda name: self.get_parameter(name).value
        hz = positive(p('publish_hz'), 'publish_hz')
        source = p('camera_source')
        source = int(source) if source.isdigit() else source
        self.capture = cv2.VideoCapture(source)
        if not self.capture.isOpened():
            self.capture.release()
            raise RuntimeError(f'Cannot open camera {source!r}. Check camera_source and /dev/video* permissions.')
        self.capture.set(cv2.CAP_PROP_FRAME_WIDTH, positive(p('width'), 'width'))
        self.capture.set(cv2.CAP_PROP_FRAME_HEIGHT, positive(p('height'), 'height'))
        self.capture.set(cv2.CAP_PROP_FPS, hz)
        # V4L2는 촬영과 읽기에 최소 두 버퍼가 필요합니다.
        # 한 개로 제한하면 프레임을 읽는 동안 다음 촬영을 놓칠 수 있습니다.
        self.capture.set(cv2.CAP_PROP_BUFFERSIZE, 2)
        self.bridge = CvBridge()
        self.frame_id = p('frame_id')
        self.publisher = self.create_publisher(Image, p('image_topic'), image_qos())
        self.capture_lock = threading.Lock()
        self.capture_stop = threading.Event()
        self.latest_frame = None
        self.capture_error = None
        self.file_interval = 1.0 / hz if isinstance(source, str) else 0.0
        self.reader = threading.Thread(target=self.read_frames, daemon=True)
        self.reader.start()
        self.timer = self.create_timer(1.0 / hz, self.publish_frame)
        self.get_logger().info(f'Camera {source!r} -> {p("image_topic")} (target {hz:g} Hz)')

    def read_frames(self):
        # 촬영을 ROS 타이머와 분리하여 드라이버의 두 버퍼를 계속 순환시킵니다.
        failures = 0
        try:
            while not self.capture_stop.is_set():
                start = time.monotonic()
                ok, frame = self.capture.read()
                if not ok:
                    failures += 1
                    if failures >= 30:
                        raise RuntimeError('Camera stopped delivering frames (or test video ended).')
                    self.capture_stop.wait(0.1)
                    continue
                failures = 0
                stamp = self.get_clock().now().to_msg()
                with self.capture_lock:
                    self.latest_frame = (frame, stamp)
                if self.file_interval:
                    self.capture_stop.wait(max(0, self.file_interval - (time.monotonic() - start)))
        except Exception as exc:
            with self.capture_lock:
                self.capture_error = exc

    def publish_frame(self):
        with self.capture_lock:
            latest, self.latest_frame = self.latest_frame, None
            error = self.capture_error
        if error is not None:
            raise RuntimeError(f'Camera capture failed: {error}') from error
        if latest is None:
            return
        frame, stamp = latest
        message = self.bridge.cv2_to_imgmsg(frame, encoding='bgr8')
        message.header.stamp = stamp
        message.header.frame_id = self.frame_id
        self.publisher.publish(message)

    def destroy_node(self):
        self.capture_stop.set()
        self.reader.join(timeout=3.0)
        if not self.reader.is_alive():
            self.capture.release()
        else:
            self.get_logger().warning('Camera read did not return; device will close when process exits.')
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args, signal_handler_options=SignalHandlerOptions.NO)
    node = None
    code = 0
    try:
        node = CameraNode()
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    except Exception as exc:
        print(f'Camera error: {exc}', file=sys.stderr)
        code = 1
    finally:
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
    if code:
        raise SystemExit(code)
