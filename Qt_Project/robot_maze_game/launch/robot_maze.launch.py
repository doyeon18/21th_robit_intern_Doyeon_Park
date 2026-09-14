from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_share = Path(get_package_share_directory("robot_maze_game"))
    parameters = str(package_share / "config" / "game.yaml")

    return LaunchDescription([
        Node(
            package="robot_maze_game",
            executable="game_engine_node",
            namespace="robot_maze",
            name="game_engine",
            parameters=[parameters],
            output="screen",
        ),
        Node(
            package="robot_maze_game",
            executable="game_gui_node",
            namespace="robot_maze",
            name="game_gui",
            output="screen",
        ),
    ])
