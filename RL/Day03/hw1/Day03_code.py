from config import LEARNING_RATE, MODEL_PATH, LOG_PATH
from pathlib import Path
from collections import deque
import random
import numpy as np
import gymnasium as gym
import torch
import torch.nn as nn

torch.manual_seed(0)  # 신경망 초기 가중치의 난수 기준 고정
random.seed(0)        # 탐험 여부와 경험 추출
np.random.seed(0)     # NumPy 난수 사용 시
torch.manual_seed(0) # 신경망 초기 가중치

# 코드 파일이 있는 폴더를 저장·불러오기 기준으로 사용
BASE_DIR = Path(__file__).resolve().parent

# 환경 만들기 
env = gym.make("CartPole-v1")
env.action_space.seed(0)  # 무작위 행동 선택
state, _ = env.reset(seed=0)

# 저장소 최대 20000개 보관 가능
memory = deque(maxlen=20000)

# batch를 뽑기 전에 경험 64개 수집
for _ in range(64):
    action = env.action_space.sample() # sample()로 왼쪽·오른쪽 중 무작위 행동 고르기 
    next_state, reward, terminated, truncated, _ = env.step(action) # step(action)으로 실제 환경에 그 행동을 적용

    memory.append(( 
        state.copy(), action, reward,
        next_state.copy(), terminated
    )) # 그 결과를 하나의 경험으로 묶어서 저장

    state = next_state
    if terminated or truncated:
        state, _ = env.reset()
batch = random.sample(memory, 64)
states, actions, rewards, next_states, terms = zip(*batch) # (현재 상태, 행동, 보상, 다음 상태, 실패 여부)


S = torch.tensor(np.array(states), dtype=torch.float32) # 상태, 64행 × 4열 텐서
A = torch.tensor(actions, dtype=torch.int64).unsqueeze(1) # 행동, 차원 하나 추가
R = torch.tensor(rewards, dtype=torch.float32) # 보상
NS = torch.tensor(np.array(next_states), dtype=torch.float32) # 다음상태
T = torch.tensor(terms, dtype=torch.float32) # 실패여부 

def make_net():
    return nn.Sequential(
        # 상태숫자 4개 
        nn.Linear(4, 64),
        # 중간값 64개 
        nn.ReLU(),
        nn.Linear(64, 64),
        # 중간값 64개 
        nn.ReLU(),
        nn.Linear(64, 2),
        # 행동 별 Q값2개 
    )

online = make_net() # 행동을 선택하고, 학습으로 가중치를 수정
target = make_net() # 학습할 목표 Q 값을 계산

target.load_state_dict(online.state_dict())
target.requires_grad_(False)

# before training (once)
optimizer = torch.optim.Adam(online.parameters(), lr=LEARNING_RATE) # Adam으로 online 가중치 수정, 학습률 0.0005
loss_fn = nn.SmoothL1Loss() # 예측 Q값과 목표 Q값의 오차를 계산하는 함수
prediction = online(S).gather(1, A).squeeze(1) # 행동별 Q값에서 실제 선택한 행동의 Q값만 추출, [64]
with torch.no_grad(): # 목표값 계산은 기울기 계산에서 제외
    next_q = target(NS).max(dim=1).values # 다음 상태의 행동별 Q값 중 최댓값, [64]
    y = R + 0.99 * (1 - T) * next_q # 목표값 = 보상 + 할인된 다음 가치, 실패하면 다음 가치는 제외

loss = loss_fn(prediction, y) # 현재 예측과 목표값의 손실 계산
optimizer.zero_grad() # 이전 기울기 초기화
loss.backward() # 손실로부터 online 가중치의 기울기 계산
nn.utils.clip_grad_norm_(online.parameters(), 10) # 전체 기울기 norm을 최대 10으로 제한
optimizer.step() # 계산한 기울기로 online 가중치 수정


state, _ = env.reset(seed=0) # 본 학습 시작 전 환경 초기화, 모델과 경험은 유지

def select_action(state, epsilon): # 탐험 또는 Q값에 따라 행동을 고르는 함수
    # epsilon 확률로 무작위 행동
    if random.random() < epsilon: # 0~1 난수가 epsilon보다 작으면 탐험
        return env.action_space.sample() # 왼쪽 또는 오른쪽 무작위 선택

    # 나머지는 Q 값이 가장 큰 행동
    with torch.no_grad(): # 행동 선택에서는 기울기를 계산하지 않음
        x = torch.as_tensor(
            state, dtype=torch.float32
        ).unsqueeze(0) # 상태 [4]를 신경망 입력 [1, 4]로 변환
        return online(x).argmax(dim=1).item() # Q값이 큰 행동의 번호를 Python 정수로 반환

def optimize_batch(): # 경험 64개로 online을 한 번 학습하는 함수
    batch = random.sample(memory, 64) # 저장된 경험 64개 무작위 추출
    states, actions, rewards, next_states, terms = zip(*batch) # 현재 상태, 행동, 보상, 다음 상태, 실패 여부로 분리

    S = torch.tensor(np.array(states), dtype=torch.float32) # 현재 상태 [64, 4]
    A = torch.tensor(actions, dtype=torch.int64).unsqueeze(1) # 선택한 행동 번호 [64, 1]
    R = torch.tensor(rewards, dtype=torch.float32) # 보상 [64]
    NS = torch.tensor(np.array(next_states), dtype=torch.float32) # 다음 상태 [64, 4]
    T = torch.tensor(terms, dtype=torch.float32) # 실패 True는 1, False는 0으로 변환, [64]

    prediction = online(S).gather(1, A).squeeze(1) # 실제 선택한 행동의 현재 Q값 추출, [64]

    with torch.no_grad(): # 목표값 계산 과정은 역전파에서 제외
        next_q = target(NS).max(dim=1).values # target으로 다음 상태의 최대 Q값 계산
        y = R + 0.99 * (1 - T) * next_q # 실패 종료만 다음 가치를 제외, 시간 제한은 포함

    loss = loss_fn(prediction, y) # 예측 Q값과 목표값의 손실 계산
    optimizer.zero_grad() # 이전 기울기 초기화
    loss.backward() # online 가중치의 기울기 계산
    nn.utils.clip_grad_norm_(online.parameters(), 10) # 기울기가 너무 커지지 않도록 제한
    optimizer.step()  # online 가중치 수정
    return loss.item()  # 기록할 손실 반환# online 가중치 수정
def evaluate_policy():
    # 학습 환경과 별도의 평가 환경
    eval_env = gym.make("CartPole-v1")
    scores = []

    with torch.no_grad():
        for episode in range(10):
            state, _ = eval_env.reset(seed=1000 + episode)
            score = 0.0

            while True:
                x = torch.as_tensor(
                    state, dtype=torch.float32
                ).unsqueeze(0)

                # 탐험 없이 Q값이 가장 큰 행동 선택
                action = online(x).argmax(dim=1).item()
                state, reward, term, trunc, _ = eval_env.step(action)
                score += reward

                if term or trunc:
                    break

            scores.append(score)

    eval_env.close()
    return float(np.mean(scores)), float(np.std(scores))
# 그래프를 만들기 위한 학습 기록
episode_returns = []  # 에피소드마다 받은 총보상
loss_history = []     # 신경망 업데이트마다 계산한 손실
epsilon_history = []  # 환경 스텝마다 탐험률
eval_steps = []       # 중간 평가를 실행한 환경 스텝
eval_means = []       # 중간 평가 평균 보상
eval_stds = []        # 중간 평가 보상 표준편차

episode_return = 0.0  # 현재 에피소드에서 누적한 보상


for step in range(1, 40001): # 환경에서 총 40000번 행동, 에피소드 수와는 다름
    epsilon = max(0.05, 1 - 0.95 * (step - 1) / 15000) # 탐험률을 1.0에서 0.05까지 15000스텝에 걸쳐 줄임
    epsilon_history.append(epsilon)
    action = select_action(state, epsilon) # epsilon-greedy 방식으로 행동 선택
    ns, r, term, trunc, _ = env.step(action) # 행동 실행, 다음 상태·보상·실패 종료·시간 제한 받기
    episode_return += r
    memory.append((state.copy(), action, r, ns.copy(), term)) # 새 경험 저장, 시간 제한 여부는 실패 여부와 구분

    if len(memory) >= 1000 and step % 4 == 0: # 경험 1000개 이상이면 환경 4스텝마다 학습
        loss_value = optimize_batch()
        loss_history.append(loss_value)
    if step % 500 == 0: # 환경 500스텝마다 목표 신경망 갱신
        target.load_state_dict(online.state_dict()) # online 가중치를 target에 복사

    state = ns # 다음 행동을 위해 현재 상태 갱신

    if term or trunc:
        episode_returns.append(episode_return)  # 끝난 에피소드 점수 저장
        episode_return = 0.0  # 다음 에피소드 보상 초기화
        state, _ = env.reset()  # 환경 다시 시작

    # 환경 2000스텝마다 별도 환경에서 10회 평가
    if step % 2000 == 0:
        mean_score, std_score = evaluate_policy()

        eval_steps.append(step)
        eval_means.append(mean_score)
        eval_stds.append(std_score)

        print(
            f"step {step:5d} | "
            f"평가 평균 {mean_score:.2f} | "
            f"표준편차 {std_score:.2f}"
        )

torch.save(online.state_dict(), MODEL_PATH) # 코드와 같은 폴더에 저장
# 그래프용 기록 저장
np.savez(
    LOG_PATH,
    episode_returns=np.array(episode_returns),
    loss_history=np.array(loss_history),
    epsilon_history=np.array(epsilon_history),
    eval_steps=np.array(eval_steps),
    eval_means=np.array(eval_means),
    eval_stds=np.array(eval_stds),
    learning_rate=optimizer.param_groups[0]["lr"],
    seed=0,
    eval_episodes=10,
)

print(f"모델 저장: {MODEL_PATH}")
print(f"학습 기록 저장: {LOG_PATH}")
model = make_net() # 저장 결과를 불러올 같은 구조의 신경망 생성
model.load_state_dict(torch.load(MODEL_PATH, weights_only=True)) # 저장된 가중치를 model에 적용
model.eval() # 평가 모드 전환, 기울기 차단은 no_grad가 담당
with torch.no_grad(): # 평가 행동 계산에서 기울기 계산 제외
    x = torch.as_tensor(state).unsqueeze(0) # 상태 하나를 입력 형태 [1, 4]로 변환
    action = model(x).argmax(dim=1).item() # Q값이 큰 행동 계산, 여기서는 환경에 실행하지 않음
