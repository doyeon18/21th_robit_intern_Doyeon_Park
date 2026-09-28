#!/usr/bin/env bash
set -e
package_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# src/패키지 배치이면 기존 작업공간, 독립 폴더이면 내부 임시 작업공간 사용.
if [[ "$(basename -- "$(dirname -- "$package_dir")")" == "src" ]]; then
  workspace_dir="$(dirname -- "$(dirname -- "$package_dir")")"
else
  workspace_dir="$package_dir/.workspace"
fi
if [[ ! -x "$workspace_dir/.venv_yolo/bin/python" ]]; then
  echo "먼저 $package_dir/setup_yolo_environment.sh 를 실행하세요." >&2
  exit 1
fi
cd -- "$workspace_dir"
source /opt/ros/jazzy/setup.bash
source "$workspace_dir/.venv_yolo/bin/activate"
export YOLO_CONFIG_DIR="$workspace_dir/.runtime/ultralytics"
export ROS_LOG_DIR="$workspace_dir/.runtime/ros_logs"
mkdir -p "$YOLO_CONFIG_DIR" "$ROS_LOG_DIR"
python -c 'import rclpy, cv_bridge, torch, ultralytics' || {
  echo "의존성 확인: $package_dir/setup_yolo_environment.sh" >&2
  exit 1
}
python -m colcon build --symlink-install --base-paths "$package_dir" --packages-select yolo_detector
source "$workspace_dir/install/setup.bash"
exec ros2 launch yolo_detector detection.launch.py \
  config:="$package_dir/config/settings.yaml" "$@"
