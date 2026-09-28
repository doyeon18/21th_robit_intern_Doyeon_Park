from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, EmitEvent, RegisterEventHandler, OpaqueFunction
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def launch_nodes(context):
    config = LaunchConfiguration('config').perform(context)
    display = LaunchConfiguration('show_window').perform(context)
    parameters = [config]
    if display:
        if display.lower() not in ('true', 'false'):
            raise ValueError('show_window must be true or false')
        parameters.append({'show_window': display.lower() == 'true'})
    camera = Node(package='yolo_detector', executable='camera_node',
                  name='camera_node', output='screen', parameters=[config])
    detector = Node(package='yolo_detector', executable='detector_node',
                    name='detector_node', output='screen',
                    parameters=parameters)
    return [
        RegisterEventHandler(OnProcessExit(target_action=camera, on_exit=[
            EmitEvent(event=Shutdown(reason='Camera node exited'))])),
        RegisterEventHandler(OnProcessExit(target_action=detector, on_exit=[
            EmitEvent(event=Shutdown(reason='Detector node exited'))])),
        camera, detector,
    ]


def generate_launch_description():
    default_config = str(Path(get_package_share_directory('yolo_detector')) / 'config/settings.yaml')
    return LaunchDescription([
        DeclareLaunchArgument('config', default_value=default_config),
        DeclareLaunchArgument('show_window', default_value='', description='Override YAML: true/false'),
        OpaqueFunction(function=launch_nodes),
    ])
