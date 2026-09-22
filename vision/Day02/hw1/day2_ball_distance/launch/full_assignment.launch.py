from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    camera_share = Path(get_package_share_directory("insta360_usb_cam"))
    detector_share = Path(get_package_share_directory("day2_ball_distance"))

    camera_config = camera_share / "config" / "camera_config.yaml"
    camera_info = camera_share / "config" / "camera_info_config.yaml"
    detector_config = detector_share / "config" / "ball_distance.yaml"

    camera_height = LaunchConfiguration("camera_height_m")
    camera_pitch = LaunchConfiguration("camera_pitch_deg")
    auto_tilt = LaunchConfiguration("auto_tilt_camera")
    auto_tracking = LaunchConfiguration("auto_tracking")
    ground_truth = LaunchConfiguration("ground_truth_m")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "camera_height_m",
                default_value="0.0",
                description="Floor to camera optical-center height in metres",
            ),
            DeclareLaunchArgument(
                "camera_pitch_deg",
                default_value="0.0",
                description="Downward camera pitch from horizontal in degrees",
            ),
            DeclareLaunchArgument(
                "auto_tilt_camera",
                default_value="true",
                description="Move the gimbal to camera_pitch_deg when tracking is off",
            ),
            DeclareLaunchArgument(
                "auto_tracking",
                default_value="true",
                description="Track the yellow ball and read pitch from the gimbal",
            ),
            DeclareLaunchArgument(
                "ground_truth_m",
                default_value="0.0",
                description="Optional measured distance in metres",
            ),
            Node(
                package="insta360_usb_cam",
                executable="pan_tilt_camera_node",
                name="pan_tilt_camera_node_camera1",
                output="screen",
                parameters=[str(camera_config), str(camera_info)],
            ),
            Node(
                package="day2_ball_distance",
                executable="ball_distance_node",
                name="ball_distance_node",
                output="screen",
                parameters=[
                    str(detector_config),
                    {
                        "camera_height_m": ParameterValue(
                            camera_height, value_type=float
                        ),
                        "camera_pitch_deg": ParameterValue(
                            camera_pitch, value_type=float
                        ),
                        "auto_tilt_camera": ParameterValue(
                            auto_tilt, value_type=bool
                        ),
                        "auto_tracking": ParameterValue(
                            auto_tracking, value_type=bool
                        ),
                        "ground_truth_m": ParameterValue(
                            ground_truth, value_type=float
                        ),
                    },
                ],
            ),
            Node(
                package="rqt_image_view",
                executable="rqt_image_view",
                name="ball_distance_viewer",
                output="screen",
                arguments=["/ball_distance/debug_image"],
            ),
        ]
    )
