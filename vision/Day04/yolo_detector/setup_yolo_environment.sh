#!/usr/bin/env bash
set -e
package_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# src/패키지 배치이면 기존 작업공간, 독립 폴더이면 내부 임시 작업공간 사용.
if [[ "$(basename -- "$(dirname -- "$package_dir")")" == "src" ]]; then
  workspace_dir="$(dirname -- "$(dirname -- "$package_dir")")"
else
  workspace_dir="$package_dir/.workspace"
fi
if [[ ! -f /opt/ros/jazzy/setup.bash ]]; then
  echo 'ROS 2 Jazzy를 먼저 설치해야 합니다.' >&2
  exit 1
fi
mkdir -p "$workspace_dir"
if [[ ! -x "$workspace_dir/.venv_yolo/bin/python" ]]; then
  /usr/bin/python3 -m venv --system-site-packages "$workspace_dir/.venv_yolo" || {
    echo 'venv 생성 실패: python3-venv 패키지 설치 후 다시 실행하세요.' >&2
    exit 1
  }
fi
touch "$workspace_dir/.venv_yolo/COLCON_IGNORE"
if [[ "$workspace_dir" == "$package_dir/.workspace" ]]; then
  touch "$workspace_dir/COLCON_IGNORE"
fi
"$workspace_dir/.venv_yolo/bin/python" -m pip install torch==2.14.0 torchvision==0.29.0 --index-url https://download.pytorch.org/whl/cpu
"$workspace_dir/.venv_yolo/bin/python" -m pip install -r "$package_dir/yolo_requirements.txt"
echo "설치 완료. $package_dir/run_yolo.sh 로 실행하세요."
