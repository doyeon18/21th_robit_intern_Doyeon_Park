from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    parameters_file = PathJoinSubstitution(
        [FindPackageShare('camera_ui_pkg'), 'config', 'camera_ui.yaml']
    )

    return LaunchDescription(
        [
            Node(
                package='camera_ui_pkg',
                executable='camera_node',
                name='camera_node',
                parameters=[parameters_file],
                output='screen',
            ),
            Node(
                package='camera_ui_pkg',
                executable='ui_node',
                name='ui_node',
                parameters=[parameters_file],
                output='screen',
            ),
        ]
    )
