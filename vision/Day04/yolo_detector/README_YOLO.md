# YOLO26n 카메라 객체 탐지 — ROS 2 Jazzy

카메라 노드가 영상을 발행하고 추론 노드가 PyTorch로 객체를 탐지한다. 결과 창에는 클래스 ID·이름·confidence, 탐지 박스와 처리 시간을 표시한다.

## 현재 컴퓨터에서 실행

```bash
cd '/home/doyeon/colcon_ws'
./run_yolo.sh
```

이 작업공간의 `.venv_yolo`에 실행 라이브러리가 들어 있다. 첫 실행 시 자동으로 패키지를 빌드한다. 기본 카메라는 `0`이다. `Q`, `Esc`, 창 닫기 또는 터미널의 `Ctrl+C`로 종료.

영상은 기본적으로 파일에 저장하지 않음. 다른 컴퓨터에서는 카메라 번호와 접근 권한을 다시 확인필요

## 폴더 구조

```text
yolo_detector/
│
├── README_YOLO.md                 # 프로젝트 설명
│
├── config/
│   └── settings.yaml             # Hz·토픽·클래스·confidence 설정
│
├── launch/
│   └── detection.launch.py       # 카메라·추론 노드를 함께 실행
│
├── models/
│   └── yolo26n.pt                # 학습된 YOLO 모델
│
├── src/
│   └── yolo_detector/            # 실제 코드가 들어가는 하위 폴더
│       ├── __init__.py           # Python 패키지 표시
│       ├── camera_node.py        # 카메라 촬영·이미지 발행
│       ├── detector_node.py      # 객체 탐지·결과 발행·화면 표시
│       └── common.py             # 공통 설정·검증 함수
│
├── tests/
│   └── smoke_test.py             # 테스트 영상으로 기능 검증
│
├── docs/
│   ├── YOLO_COCO_CLASSES.md      # 클래스 ID·이름 참고표
│   └── YOLO_VERIFICATION.md      # 검증 결과 기록
│
├── resource/
│   └── yolo_detector             # ROS 2 패키지 등록용 파일
│
├── package.xml                   # 패키지 정보·ROS 의존성
├── setup.py                      # Python 코드·설정 파일 설치 정의
├── setup.cfg                     # 실행 파일 설치 위치
├── yolo_requirements.txt         # Python 라이브러리·버전 목록
├── setup_yolo_environment.sh     # 실행 환경 설치
└── run_yolo.sh                   # 빌드 후 프로그램 실행
```


## YAML 설정

| 항목 | 기본값 | 의미 |
|---|---|---|
| `camera_source` | `"0"` | 카메라 번호. 테스트 동영상 경로도 가능 |
| `publish_hz` | `30.0` | 카메라 영상 발행 목표 Hz |
| `width`, `height` | `640`, `480` | 카메라에 요청할 크기. 장치가 다르게 제공할 수도 있음 |
| `inference_hz` | `30.0` | 추론 목표 Hz. 실제 성능보다 빠르게 실행할 수는 없음 |
| `preview_hz` | `30.0` | 카메라 미리보기 갱신 목표 Hz |
| `max_box_age_ms` | `250.0` | 미리보기에 표시할 최근 박스의 최대 나이 |
| `confidence_threshold` | `0.5` | 이 점수 미만의 탐지 제외 |
| `detect_all_classes` | `false` | 기본은 YAML에 지정한 일상 클래스 30개. true이면 80개 전체 |
| `class_ids` | 30개 ID | 일부 클래스 선택 시 사용할 모델 인덱스 |
| `class_names` | 일상 클래스 30개 | ID에 대응하는 모델의 정확한 클래스 이름. YAML에 한국어 주석 포함 |
| `device` | `"cpu"` | 실행 장치 |
| `cpu_threads` | `4` | PyTorch CPU 연산 스레드 수 |
| `image_size` | `640` | 모델 전처리 목표 크기 |
| `model_path` | `""` | 비우면 포함된 모델. 직접 지정할 때는 절대 경로 권장 |
| `show_window` | `true` | 결과 창 표시. 디스플레이가 없으면 창 없이 토픽만 발행 |

사람과 공만 선택하려면 다음과 같이 설정한다. 전체를 사용하는 동안에도 ID/이름 목록은 유효해야 한다.

```yaml
detect_all_classes: false
class_ids: [0, 32]
class_names: ["person", "sports ball"]
```

`detect_all_classes: true`로 바꾸면 모델의 80개 클래스를 모두 사용한다. 전체 모델 이름표는 `YOLO_COCO_CLASSES.md`에 있으며, 원본 COCO 주석의 `category_id`가 아닌 모델의 0~79 인덱스를 사용한다.

## 토픽과 처리 흐름

```text
camera_node → /camera/image_raw → detector_node → /yolo/image
                                               → /yolo/detections
                                               → /yolo/latency_ms
```

| 토픽 | 타입 | 내용 |
|---|---|---|
| `/camera/image_raw` | `sensor_msgs/msg/Image` | BGR8 카메라 이미지 |
| `/yolo/image` | `sensor_msgs/msg/Image` | 박스와 정보가 그려진 원본 크기 이미지 |
| `/yolo/detections` | `std_msgs/msg/String` | JSON: 원본 header, 시간, 단계별 FPS, 클래스 ID·이름·점수·xyxy 좌표 |
| `/yolo/latency_ms` | `std_msgs/msg/Float32` | predict 호출 전체 소요 시간(ms) |

토픽 이름은 YAML에서 변경가능하다. 영상 토픽은 Best Effort, depth=1이다.

`Predict` 시간은 전처리·추론·후처리를 포함한 `model.predict()` 호출 시간이다. 그림 그리기와 ROS 발행 시간은 포함하지 않으며, 모델 로딩과 초기 준비 비용도 제외한다. `Frame age`는 카메라 노드가 읽은 이미지에 붙인 시간부터 추론 결과를 해석한 시점까지이다. `Infer` FPS는 최근 최대 30개 추론 완료 시각으로 계산한다. 첫 프레임은 0으로 표시한다. 창의 `Camera`는 수신 FPS, `View`는 미리보기 갱신 FPS이다.

Ultralytics가 이미지 전처리와 모델 출력 해석, 원본 좌표 복원을 담당한다. 추론은 전용 스레드에서 실행하고 이미지 수신과 GUI 처리는 메인 스레드에서 진행하며, 처리 중 들어온 이미지들은 최신 한 장으로 교체한다. 같은 프레임을 반복 추론하지 않는다.

미리보기 창은 추론 완료를 기다리지 않고 최신 카메라 프레임을 표시한다. `Box age`에 원본 탐지 프레임으로부터의 경과시간을 표시하며, 250ms보다 오래된 박스는 숨긴다.

카메라 읽기도 별도 스레드에서 계속 수행하고 ROS 발행 타이머는 최신 프레임만 전달한다. 현재 카메라에서는 1개로 제한하면 촬영 프레임이 누락되어 입력 속도가 떨어지는 것을 확인하였고, 카메라 하드웨어 버퍼는 2개로 설정하여 문제를 해결하였다. ROS 이미지 큐 depth=1과 카메라 촬영 버퍼 수는 별개의 설정이다.
