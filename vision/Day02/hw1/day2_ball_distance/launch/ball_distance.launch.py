from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    share_dir = Path(get_package_share_directory("day2_ball_distance"))
    config = share_dir / "config" / "ball_distance.yaml"

    return LaunchDescription(
        [
            Node(
                package="day2_ball_distance",
                executable="ball_distance_node",
                name="ball_distance_node",
                output="screen",
                parameters=[str(config)],
            )
        ]
    )
