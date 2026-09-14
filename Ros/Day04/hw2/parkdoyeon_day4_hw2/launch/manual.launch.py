from launch import LaunchDescription
from launch_ros.actions import LifecycleNode


def generate_launch_description():
    return LaunchDescription([
        LifecycleNode(
            package='parkdoyeon_day4_hw2',
            executable='counter_publisher',
            name='counter_publisher',
            namespace='',
            output='screen',
            autostart=False,
        ),
        LifecycleNode(
            package='parkdoyeon_day4_hw2',
            executable='counter_subscriber',
            name='counter_subscriber',
            namespace='',
            output='screen',
            autostart=False,
        ),
    ])
