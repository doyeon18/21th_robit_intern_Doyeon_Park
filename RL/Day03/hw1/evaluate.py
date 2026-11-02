from pathlib import Path
from config import MODEL_PATH, LOG_PATH, BASE_DIR, LEARNING_RATE

import gymnasium as gym
import numpy as np
import torch
import torch.nn as nn


def make_net():
    return nn.Sequential(
        nn.Linear(4, 64),
        nn.ReLU(),
        nn.Linear(64, 64),
        nn.ReLU(),
        nn.Linear(64, 2),
    )


model = make_net()
model_path = MODEL_PATH
model.load_state_dict(
    torch.load(model_path, map_location="cpu", weights_only=True)
)
model.eval()

env = gym.make("CartPole-v1") # render_mode = "human")
scores = []

with torch.no_grad():
    for episode in range(50):
        state, _ = env.reset(seed=1000 + episode)
        score = 0

        while True:
            x = torch.as_tensor(
                state, dtype=torch.float32
            ).unsqueeze(0)
            action = model(x).argmax(dim=1).item()

            state, reward, terminated, truncated, _ = env.step(action)
            score += reward

            if terminated or truncated:
                position = float(state[0])
                angle = float(state[2])
                angle_deg = np.degrees(angle)

                if terminated:
                    reasons = []

                    if abs(position) > env.unwrapped.x_threshold:
                        reasons.append("카트 위치 한계 초과")

                    if abs(angle) > env.unwrapped.theta_threshold_radians:
                        reasons.append("막대 각도 한계 초과")

                    print(f"\n종료 원인: {', '.join(reasons)}")
                    print(f"카트 위치: {position:.3f} m")
                    print(f"막대 각도: {angle_deg:.3f}°")

                elif truncated:
                    print("시간 제한 도달")

                break

        scores.append(score)
        print(f"{episode + 1:2d}회: {score:.0f}점")

env.close()

print("\n평가 결과")
print(f"평균: {np.mean(scores):.2f}")
print(f"표준편차: {np.std(scores):.2f}")
print(f"최소: {np.min(scores):.0f}")
print(f"최대: {np.max(scores):.0f}")

# 50회 평가가 끝난 뒤 학습 그래프 4종과 평가 기록 저장
from plot_training import plot_training

plot_training(LOG_PATH)

np.savez(
    BASE_DIR / f"evaluation_lr{LEARNING_RATE}.npz",
    scores=np.array(scores),
    seeds=np.arange(1000, 1050),
    learning_rate=LEARNING_RATE,
)
