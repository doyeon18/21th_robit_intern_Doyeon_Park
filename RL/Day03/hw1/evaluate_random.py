import gymnasium as gym
import numpy as np

env = gym.make("CartPole-v1")
env.action_space.seed(0)
scores = []

for episode in range(50):
    # DQN 평가와 같은 초기 상태 seed
    state, _ = env.reset(seed=1000 + episode)
    score = 0

    while True:
        action = env.action_space.sample()
        state, reward, terminated, truncated, _ = env.step(action)
        score += reward

        if terminated or truncated:
            break

    scores.append(score)
    print(f"{episode + 1:2d}회: {score:.0f}점")

env.close()

print("\n무작위 정책 평가 결과")
print(f"평균: {np.mean(scores):.2f}")
print(f"표준편차: {np.std(scores):.2f}")
print(f"최소: {np.min(scores):.0f}")
print(f"최대: {np.max(scores):.0f}")