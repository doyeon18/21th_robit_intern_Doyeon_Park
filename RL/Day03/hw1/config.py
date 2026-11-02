from pathlib import Path

# 학습률은 여기 한 곳에서만 변경
LEARNING_RATE = 0.001

# 코드가 있는 폴더 기준으로 저장·불러오기
BASE_DIR = Path(__file__).resolve().parent
MODEL_PATH = BASE_DIR / f"dqn_lr{LEARNING_RATE}.pt"
LOG_PATH = BASE_DIR / f"training_log_lr{LEARNING_RATE}.npz"
