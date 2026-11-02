## 환경 및 설치

Ubuntu, Python 3.12.3, CPU에서 실행했으며, requirements.txt에는 실습에 필요한 직접 의존성의 설치 버전을 기록했다.

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

## 파일 역할

| 파일 | 역할 |
|---|---|
| config.py | 학습률과 저장·불러오기 경로 설정 |
| Day03_code.py | DQN 학습, 모델과 학습 기록 저장 |
| evaluate.py | 모델 재로딩, 50회 평가, 학습 그래프와 평가 기록 저장 |
| evaluate_random.py | 무작위 정책 50회 평가 |
| plot_training.py | 학습 기록으로 그래프 4종 생성 |
| requirements.txt | 필요한 패키지와 버전 |
| report_day03.md | 평가 결과·실험 분석·개념 설명·AI 사용 내역 |

## 실행 순서

### 1. 학습률 설정

config.py에서 다음 값만 변경한다.

```python
LEARNING_RATE = 0.0005  # 기본 설정. 변경 실험은 0.001
```

학습과 평가는 같은 학습률 설정으로 실행합니다.

### 2. 학습

```bash
python Day03_code.py
```

환경 40,000스텝 동안 학습한다. 2,000스텝마다 별도 환경에서 10회 평가한 결과를 출력하며, 종료 후 모델과 학습 기록을 저장하며 사진은 생성하지 않는다.

### 3. 최종 평가와 그래프 저장

```bash
python evaluate.py
```

저장 모델을 불러와 환경 seed 1000~1049에서 50회 greedy 평가한다. 평균·표준편차·최소·최대와 종료 원인을 출력하고, 평가 종료 후 학습 그래프와 평가 기록을 저장합니다.

움직이는 화면을 보려면 evaluate.py의 환경 생성 코드를 다음처럼 설정한다.

```python
env = gym.make("CartPole-v1", render_mode="human")
```

### 4. 무작위 정책 비교

```bash
python evaluate_random.py
```

같은 초기 상태 seed 목록에서 무작위 행동을 선택하며, 50회 평가 결과를 터미널에 출력한다.

## 생성 파일과 경로

학습률이 0.0005일 때의 이름이다. 0.001로 변경하면 이름에도 자동 반영된다.

| 파일 | 내용 |
|---|---|
| dqn_lr0.0005.pt | 학습한 신경망 가중치 |
| training_log_lr0.0005.npz | 학습 보상·loss·탐험률·중간 평가 기록 |
| evaluation_lr0.0005.npz | 최종 50회 평가 점수·seed·학습률 |
| training_curves_lr0.0005.png | 학습 보상·평가 보상·TD loss·ε 그래프 한 장 |

config.py가 있는 프로젝트 폴더를 기준으로 저장하고 불러오며. 같은 학습률로 다시 실행하면 해당 파일은 덮어씌어진다.


```bash
python plot_training.py
```

