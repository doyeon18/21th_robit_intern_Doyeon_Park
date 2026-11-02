# Humanoid-v5 PPO 학습률 비교 실험

## 폴더 구성

- `train.py`: PPO 학습, 모델 저장·재로딩 확인, 별도 환경에서 20회 평가.
- `check_env.py`: 환경 생성 후 무작위 행동으로 관측·행동·보상·종료 신호 확인.
- `make_results.py`: 저장된 모델과 결과로 비교 그래프·MP4·미리보기 이미지 생성.
- `configs/baseline.json`: 기본 A, 학습률 0.0003.
- `configs/changed.json`: 변경 B, 학습률 0.0001.
- `requirements.txt`: 실행에 사용한 라이브러리 버전.
- `results/baseline/`, `results/changed/`: 최종 모델, 실행 설정, 학습 CSV, 20회 평가 CSV, 요약 결과.
- `results/baseline_console.log`, `results/changed_console.log`: 본 학습의 손실·보상·속도 등 출력 기록.
- `results/env_check.json`: 환경 확인 코드의 실행 결과.
- `results/comparison/`: 비교 그래프 2장, 대표·최저 보상 영상 4개, 미리보기 이미지 4장, 영상별 정보.


## 설치

Python 3.12를 기준으로 프로젝트 폴더에서 실행한다. 

```bash
python3 -m venv .venv
.venv/bin/python -m pip install torch==2.15.0+cpu --index-url https://download.pytorch.org/whl/cpu
.venv/bin/python -m pip install -r requirements.txt
```

실제 실험은 CPU 1스레드로 실행했다. 영상 생성에는 ffmpeg가 설치되어 PATH에 등록되어 있어야 하며, MuJoCo가 사용할 OpenGL 렌더러가 필요합니다. Linux에 OSMesa가 설치되어 있으면 자동으로 사용하고, Windows/macOS 등에서는 MuJoCo 기본 렌더러를 사용한다.

## 환경 확인

```bash
.venv/bin/python check_env.py --steps 1000 --seed 0
```

`results/env_check.json`에 확인 결과를 저장한다.

## 학습 및 평가

기존 실험 결과를 보존하도록 재실행 시 새 출력 폴더를 사용한다. 아래 명령은 순서대로 실행한다.

```bash
.venv/bin/python train.py --config configs/baseline.json --output results/rerun_baseline
.venv/bin/python train.py --config configs/changed.json --output results/rerun_changed
```

짧은 실행 확인에는 `--smoke-steps 4096`을 추가한다.
학습 완료 후 모델 저장과 20회 평가가 자동으로 실행된다.
`model.zip`은 최종 모델, `train.monitor.csv`는 학습 기록, `evaluation.csv`는 개별 평가,
`summary.json`은 평균·표준편차·시간·버전 등의 요약이다.
학습 중에는 진행 상황과 중간 모델도 자동 저장된다.

## 그래프 및 영상 생성

```bash
.venv/bin/python make_results.py
```

이 코드는 기존 `results/baseline/`와 `results/changed/`를 읽고 `results/comparison/`의 그래프와 영상을 다시 생성한다.
새 학습 결과를 시각화하려면 코드에서 읽는 결과 폴더도 맞춰야 한다.
영상은 각 조건의 평가 보상 중앙값에 가까운 사례와 최저 보상 사례이며, 성공 사례를 의미하지 않는다.

## 실험 조건 및 결과

- 조건당 1,001,472 환경 step, 학습 seed 0.
- 동일한 평가 seed 2000~2019, deterministic=True, 모델별 20회 평가.
- 보상과 환경 등은 같고 학습률만 변경했다.
- 평균 보상: A 496.01, B 479.96.
- 평균 유지 시간: A 1.463초, B 1.410초.

