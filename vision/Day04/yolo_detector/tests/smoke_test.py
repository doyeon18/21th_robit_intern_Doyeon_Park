"""카메라 없이 샘플 영상으로 실제 Launch/추론/토픽을 검증합니다."""
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

import cv2
import rclpy
import yaml
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import Float32, String
from ultralytics.utils import ASSETS
from yolo_detector.common import image_qos, validate_classes


ROOT = Path(__file__).resolve().parents[1]


def check_case(directory, video, name, all_classes, confidence, expect_objects):
    config = yaml.safe_load((ROOT / 'config/settings.yaml').read_text())
    camera = config['camera_node']['ros__parameters']
    camera.update(camera_source=str(video), publish_hz=10.0, image_topic=f'/{name}/input')
    detector = config['detector_node']['ros__parameters']
    detector.update(image_topic=f'/{name}/input', result_image_topic=f'/{name}/image',
                    detections_topic=f'/{name}/detections', latency_topic=f'/{name}/latency',
                    detect_all_classes=all_classes, class_ids=[0], class_names=['person'],
                    confidence_threshold=confidence, show_window=False)
    config_path = directory / f'{name}.yaml'
    config_path.write_text(yaml.safe_dump(config))
    received, images, latency = [], [], []
    node = Node(f'{name}_tester')
    node.create_subscription(String, f'/{name}/detections',
                             lambda m: received.append(json.loads(m.data)), 10)
    node.create_subscription(Image, f'/{name}/image', lambda m: images.append(m), image_qos())
    node.create_subscription(Float32, f'/{name}/latency', lambda m: latency.append(m.data), 10)
    with (directory / f'{name}.log').open('w') as log:
        process = subprocess.Popen(['ros2', 'launch', 'yolo_detector', 'detection.launch.py',
                                    f'config:={config_path}'], stdout=log, stderr=log,
                                   start_new_session=True)
        try:
            deadline = time.monotonic() + 45
            while time.monotonic() < deadline and (len(received) < 4 or not images or not latency):
                rclpy.spin_once(node, timeout_sec=0.1)
                if process.poll() is not None:
                    raise AssertionError(f'Launch exited unexpectedly: {process.returncode}')
            assert len(received) >= 4 and images and latency, 'Missing output topics'
            timestamps = [(r['header']['stamp']['sec'], r['header']['stamp']['nanosec']) for r in received]
            assert len(set(timestamps)) == len(timestamps), 'Same source frame processed twice'
            assert all(r['header']['frame_id'] == 'camera' for r in received)
            assert images[-1].encoding == 'bgr8' and images[-1].width == 640 and images[-1].height == 480
            for result in received:
                assert result['inference_ms'] > 0 and result['frame_age_ms'] >= 0
                detections = result['detections']
                assert bool(detections) == expect_objects
                ids = {d['class_id'] for d in detections}
                if expect_objects:
                    assert 0 in ids
                    assert (5 in ids) if all_classes else ids == {0}
                for detection in detections:
                    assert detection['confidence'] >= confidence
                    x1, y1, x2, y2 = detection['xyxy']
                    assert 0 <= x1 <= x2 <= 640 and 0 <= y1 <= y2 <= 480
            print(f'PASS {name}: {len(received)} frames; last classes={ids}; '
                  f'prediction={received[-1]["inference_ms"]:.1f} ms')
        except Exception:
            log.flush()
            print((directory / f'{name}.log').read_text())
            raise
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGINT)
                try:
                    process.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
            node.destroy_node()
        log.flush()
        shutdown_log = (directory / f'{name}.log').read_text()
        assert 'Traceback' not in shutdown_log and 'context is invalid' not in shutdown_log, shutdown_log
        assert 'process has finished cleanly' in shutdown_log, shutdown_log


def main():
    # 임의 이미지가 아닌 라이브러리의 실제 객체 사진을 사용합니다.
    image = cv2.imread(str(ASSETS / 'bus.jpg'))
    assert image is not None, 'Ultralytics sample bus.jpg is missing'
    image = cv2.resize(image, (640, 480))
    with tempfile.TemporaryDirectory(prefix='yolo_smoke_') as temp:
        directory = Path(temp)
        video = directory / 'sample.avi'
        writer = cv2.VideoWriter(str(video), cv2.VideoWriter_fourcc(*'MJPG'), 10, (640, 480))
        assert writer.isOpened()
        for _ in range(600):
            writer.write(image)
        writer.release()
        rclpy.init()
        try:
            check_case(directory, video, 'all_classes', True, 0.5, True)
            check_case(directory, video, 'person_only', False, 0.5, True)
            check_case(directory, video, 'high_threshold', True, 0.99999, False)
        finally:
            rclpy.shutdown()
    try:
        validate_classes([0], ['car'], {0: 'person'})
    except ValueError:
        print('PASS mismatched class ID/name rejected')
    else:
        raise AssertionError('Invalid class name was accepted')


if __name__ == '__main__':
    main()
