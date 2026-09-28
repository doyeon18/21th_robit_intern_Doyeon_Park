from glob import glob
from setuptools import find_packages, setup

setup(
    name='yolo_detector', version='0.1.0',
    packages=find_packages(where='src'), package_dir={'': 'src'},
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/yolo_detector']),
        ('share/yolo_detector', ['package.xml']),
        ('share/yolo_detector/launch', glob('launch/*.launch.py')),
        ('share/yolo_detector/config', glob('config/*.yaml')),
        ('share/yolo_detector/models', glob('models/*.pt')),
    ],
    install_requires=['setuptools'], zip_safe=False,
    entry_points={'console_scripts': [
        'camera_node = yolo_detector.camera_node:main',
        'detector_node = yolo_detector.detector_node:main',
    ]},
)
