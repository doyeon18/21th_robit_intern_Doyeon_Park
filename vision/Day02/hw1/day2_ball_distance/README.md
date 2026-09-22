# Day 2: HSV 공 검출 및 단안 거리 추정

PPT의 `insta360_usb_cam` ROS 2 Jazzy 카메라 노드에 연결해 노란 공을
검출하고, 다음 두 방식의 거리를 동시에 계산하는 C++ 패키지다.

- 방법 A: 공의 실제 지름과 영상 속 픽셀 지름 이용
- 방법 B: 카메라 높이, 짐벌에서 읽은 실제 틸트 각, 공의 바닥 접촉 픽셀 이용

## 참고 카메라 노드와 토픽

참고 저장소: <https://github.com/sasdfdse/insta360_usb_cam>

저장소의 기본 설정에서 사용하는 토픽은 다음과 같다.

| 용도 | 토픽 | 실제 메시지 형식 |
|---|---|---|
| 영상 | `/camera1/camera/compressed_image` | `sensor_msgs/msg/Image` |
| 카메라 정보 | `/camera1/info` | `sensor_msgs/msg/CameraInfo` |
| 검출 결과 영상 | `/ball_distance/debug_image` | `sensor_msgs/msg/Image` |
| 거리/오차 배열 | `/ball_distance/results` | `std_msgs/msg/Float64MultiArray` |

`compressed_image`라는 이름과 달리 참고 노드는 `CompressedImage`가 아닌
일반 `Image`를 발행한다. 이 과제 노드도 실제 메시지 형식에 맞췄다.

결과 배열의 순서는 다음과 같다.

```text
[방법A 거리, 방법B 거리, 실제 거리, 방법A 절대오차, 방법B 절대오차]
```

사용할 수 없는 값은 `NaN`으로 발행한다.

## 구현 흐름

1. `CameraInfo`에서 내부 행렬과 왜곡계수를 읽는다.
2. 입력 영상에 렌즈 왜곡 보정을 적용한다.
3. BGR 영상을 HSV로 변환하고 노란색 마스크를 만든다.
4. Opening/Closing으로 작은 노이즈와 구멍을 정리한다.
5. 최소 면적과 원형도 조건을 만족하는 가장 큰 윤곽선을 공으로 선택한다.
6. 최소 외접원 지름으로 방법 A를 계산한다.
7. 윤곽선의 최하단 픽셀을 바닥 접촉점으로 사용해 방법 B를 계산한다.
8. 결과와 검출 영상을 ROS 2 토픽으로 발행한다.

## 먼저 측정할 값

자동 추적 모드에서는 공의 지름과 카메라 높이만 실제 장비에서 측정한다.
틸트 각도는 공을 추적한 뒤 카메라 짐벌에서 자동으로 읽는다.

```yaml
ball_diameter_m: 0.0
camera_height_m: 0.0
auto_tracking: true
```

- `ball_diameter_m`: 공의 실제 지름
- `camera_height_m`: 바닥에서 카메라 광학 중심까지의 수직 높이
- `auto_tracking`: 노란 공을 화면 중앙으로 자동 추적할지 여부

값이 0이면 해당 거리 계산은 비활성화되고 `NaN`이 나온다. 임의의 숫자로
실험 결과를 만들지 않기 위한 동작이다.

## 빌드와 실행

빌드가 끝난 현재 작업공간에서는 최상위 폴더에서 다음 한 줄로 실행한다.

```bash
./run_assignment.sh
```

직접 ROS 2 환경을 불러와 실행하려면 다음 순서를 사용한다.

```bash
source /opt/ros/jazzy/setup.bash
colcon build --packages-select day2_ball_distance
source install/setup.bash
ros2 launch day2_ball_distance full_assignment.launch.py
```

카메라, 거리 계산 노드, 결과 화면이 함께 실행된다. 결과 창에는 방법 A와
방법 B의 추정값이 실시간으로 표시된다. 카메라 광학 중심의 높이를 측정한
뒤에는 실행할 때 다음처럼 입력한다.

```bash
./run_assignment.sh camera_height_m:=0.23
```

위 숫자는 예시이며 실제 측정값을 사용한다. 높이 단위는 m다. 노란 공이
보이면 카메라가 공 중심을 따라 팬·틸트하고, 중앙에서 약 0.5초 안정되면
화면 상태가 `LOCKED`로 바뀐다. 이때 짐벌에서 읽은 실제 틸트 각과 공의
바닥 접촉 픽셀로 방법 B를 계산한다. 카메라 받침대는 수평으로 설치해야
한다. 높이를 입력하지 않으면 방법 A는 표시되고 방법 B는 `N/A`다.

화면의 추적 상태는 다음과 같다.

- `SEARCHING`: 노란 공을 찾는 중
- `TRACKING`: 공을 중앙으로 이동시키는 중
- `STABILIZING`: 중앙에서 안정 여부를 확인하는 중
- `LOCKED`: 추적이 안정되어 방법 B 거리도 표시되는 상태

기존처럼 사용자가 각도를 지정하고 자동 추적하지 않으려면 다음처럼
실행한다.

```bash
./run_assignment.sh camera_height_m:=0.23 camera_pitch_deg:=20.0 \
  auto_tracking:=false
```

거리값은 별도 터미널에서도 확인할 수 있다.

```bash
ros2 topic echo /ball_distance/results
```

## 반드시 확인할 카메라 보정 조건

참고 저장소는 1280x960으로 캡처한 영상을 내부에서 640x480으로 줄여
발행한다. 로컬 카메라 노드는 `CameraInfo.width/height`도 실제 발행 영상인
640x480으로 내보내도록 수정했다. 실행 후 다음 두 값이 일치하는지 확인한다.

```bash
ros2 topic echo /camera1/camera/compressed_image --once --field width --field height
ros2 topic echo /camera1/info --once
```

영상 크기와 `CameraInfo.width/height` 또는 내부행렬이 맞지 않으면 실제
발행 해상도로 다시 캘리브레이션한 뒤 사용한다. 과제 노드는 크기 불일치를
발견하면 내부행렬을 비례 조정하고 경고하지만, 잘못된 원본 캘리브레이션을
완전히 복구할 수는 없다.

## 실험 방법

공의 바닥 접촉점까지 실제 거리를 예를 들어 0.1 m 간격으로 측정한다.
각 거리에서 `ground_truth_m`를 바꾸고 두 추정값을 기록한다. 같은 거리에서
실제 측정한 각 거리 5회를 기록해 평균과 표준편차를 함께 계산하면 흔들림까지
비교할 수 있다.
