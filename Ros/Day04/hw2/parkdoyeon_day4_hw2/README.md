# ROS 2 Day 4: Lifecycle + QoS 카운터

이 패키지는 ROS 2 Jazzy와 C++로 작성한 Lifecycle 노드 두 개를 포함합니다.
`counter_publisher`는 `/count` 토픽에 1초마다 1씩 증가하는
`std_msgs/msg/Int32` 값을 발행합니다. `counter_subscriber`는 Active 상태일 때만
받은 숫자를 터미널에 출력합니다.

두 노드의 QoS는 모두 Keep Last, Depth 10, Reliable, Volatile로 설정했습니다.

## 빌드

작업 공간에서 다음 명령을 실행합니다.

```bash
source /opt/ros/jazzy/setup.bash
colcon build --packages-select parkdoyeon_day4_hw2
source install/setup.bash
```

## 자동 상태 전환

```bash
ros2 launch parkdoyeon_day4_hw2 auto.launch.py
```

이 Launch 파일은 두 노드를 자동으로 configure한 뒤 activate합니다.

## 수동 상태 전환

첫 번째 터미널에서 다음 명령을 실행합니다.

```bash
ros2 launch parkdoyeon_day4_hw2 manual.launch.py
```

두 번째 터미널에서도 ROS 2 환경을 불러온 다음, 아래 순서대로 상태를 전환합니다.
먼저 Subscriber를 활성화하면 첫 메시지부터 확인하기 쉽습니다.

```bash
ros2 lifecycle get /counter_publisher
ros2 lifecycle get /counter_subscriber
ros2 lifecycle set /counter_subscriber configure
ros2 lifecycle set /counter_subscriber activate
ros2 lifecycle set /counter_publisher configure
ros2 lifecycle set /counter_publisher activate
```

상태에 따라 통신이 달라지는지도 확인할 수 있습니다.

```bash
ros2 lifecycle set /counter_publisher deactivate
ros2 lifecycle set /counter_publisher activate
ros2 lifecycle set /counter_subscriber deactivate
```

Publisher를 deactivate하면 새 숫자를 발행하지 않습니다. Subscriber만
deactivate하면 숫자는 계속 발행되지만, Subscriber는 받은 숫자를 출력하지 않습니다.
일반 Subscription은 LifecycleNode 상태에 따라 자동으로 멈추지 않으므로,
Subscriber 코드에서 Active 여부를 확인해 처리하도록 구현했습니다.
