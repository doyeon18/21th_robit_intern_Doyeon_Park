import os

from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    config_file = os.path.join(
        get_package_share_directory("parkdoyeon_day3_hw2"),
        "config",
        "traffic.yaml"
    )

    return LaunchDescription([
        Node(
            package="parkdoyeon_day3_hw2",
            executable="traffic_light_node",
            name="traffic_light_node",
            parameters=[config_file],
            output="screen"
        ),

        Node(
            package="parkdoyeon_day3_hw2",
            executable="vehicle_node",
            name="vehicle_node",
            parameters=[config_file],
            output="screen"
        )
    ])