''' 환경과 Q-table 준비 '''

import json
from pathlib import Path

import gymnasium as gym
import matplotlib
import numpy as np

matplotlib.use("Agg")  # Save graphs without opening a separate window.
import matplotlib.pyplot as plt

env = gym.make("FrozenLake-v1", map_name="4x4", is_slippery=False) 
# FrozenLake환경 선택, 4 X 4 크기의 맵, 미끄러짐 = 끔 -> env변수에 저장 
map_desc = env.unwrapped.desc.copy() # 시각화에 사용할 맵 정보 보관
rng = np.random.default_rng(0) # 난수 seed 설정
Q = np.zeros((env.observation_space.n, env.action_space.n)) 
# 값 : 16, 4 / 의미 : 상태 개수, 행동 개수 -> Q = np.zeros((16, 4))
''' 
    Q = [[0, 0, 0, 0]  # 상태 0
         [0, 0, 0, 0]  # 상태 1
         ...
         [0, 0, 0, 0]] # 상태 15
'''
alpha, gamma = 0.1, 0.95 # 학습 설정 : 목표값과 현재 Q값 차이의 10% 반영 / 다음 상태의 가치를 목표값에 반영할때 0.95만큼 반영
returns = [] # 에피소드별 결과를 저장할 빈 리스트

''' 행동 선택 함수 '''

def choose_action(state, epsilon, rng=rng): # 탐험 상태, 탐험 확률 받음
    if rng.random() < epsilon: # 0 < rng.random() < 1 인 무작위 숫자. 
        return int(rng.integers(4)) # 0, 1, 2, 3중 무작위 하나
    best = np.flatnonzero(Q[state] == Q[state].max()) 
    # best = np.flatnonzero([False, True, True, False])
    # best: [1, 2] 
    return int(rng.choice(best)) # best 값 중 무작위로 반환

''' 경험 수집과 Q 갱신 '''

for episode in range(5000): # 5000판 진행
    state, _ = env.reset(seed=0 if episode == 0 else None) # 환경 초기화
    epsilon = max(0.05, 0.999 ** episode) # 탐험 확률 설정, 현재 설정값 0:1, 1000:0.368, 3000:0.05 단 최소 5프로 유지
    total = 0 # total 값 초기화
    while True: # 한 판에 몇 번 이동할지 미리 정하지 않고, 종료 조건을 만나 break가 실행될 때까지 반복
        action = choose_action(state, epsilon) # 현재 상태에서 행동 번호를 고르기
        ns, r, terminated, truncated, _ = env.step(action) # 환경에서 실제로 행동을 실행
        # 이동 후 상태 번호. next state의 줄임말, 이번 행동으로 받은 보상, 목표 도착·구멍 진입으로 종료됐는지, 시간 제한 등으로 중단됐는지, _
        if terminated:
            target = r
        else:
            target = r + gamma * Q[ns].max()
        Q[state, action] += alpha * (target - Q[state, action]) # 방금 실행한 행동의 Q값을 수정
        total += r 
        state = ns # state 수정 
        if terminated or truncated:
            break # 목표 도착, 구멍 진입, 시간 제한 중 하나가 발생하면 break가 실행
    returns.append(total) # 끝난 판에 보상합계 저장

q_path = Path(__file__).resolve().parent / "q_table.npy"
np.save(q_path, Q) # 파일로 저장 

''' 학습과 평가의 분리 '''

Q = np.load(q_path) # 불러오기 
eval_env = gym.make("FrozenLake-v1", is_slippery=False) # 평가용 환경 만들기 
eval_rng = np.random.default_rng(1000) # 평가용 난수생성기 만들기 
wins = 0 # 성공 횟수 초기화
success_lengths = [] # 성공한 판의 행동 횟수만 저장
for ep in range(200): # 200판 평가 시작
    state, _ = eval_env.reset(seed=1000 if ep == 0 else None)
    steps = 0 # 새 판이 시작될 때 행동 횟수 초기화
    while True:
        action = choose_action(state, 0.0, eval_rng) # 탐험 없이 행동 선택
        state, r, term, trunc, _ = eval_env.step(action) # 선택한 행동 실행
        steps += 1 # 실제 행동을 한 번 실행할 때마다 증가
        if term or trunc:
            if r == 1: # 목표에 도착한 판만 성공 경로 길이로 기록
                wins += 1
                success_lengths.append(steps)
            break

''''''

# 평가 결과(Metrics)를 저장할 딕셔너리 생성
evaluation_results = {
    "evaluation_episodes": 200,  # 총 평가 진행 횟수 (테스트 에피소드 수)
    "wins": wins,                # 성공(목표 지점 도달)한 총 횟수
    "success_rate": wins / 200,  # 최종 성공률 (0.0 ~ 1.0)
    "success_path_lengths": success_lengths,  # 성공한 에피소드들의 실제 스텝(액션) 수 리스트
    
    # 삼항 연산자를 사용해 성공한 판이 있을 때만 통계치 계산 (없으면 None 처리하여 에러 방지)
    # JSON 직렬화를 위해 numpy float 데이터를 파이썬 내장 float형으로 변환
    "mean_success_path_length": float(np.mean(success_lengths)) if success_lengths else None,  # 평균 성공 경로 길이
    "min_success_path_length": min(success_lengths) if success_lengths else None,              # 최단 성공 경로 길이
    "max_success_path_length": max(success_lengths) if success_lengths else None,              # 최장 성공 경로 길이
}

# 평가 결과 콘솔(Console) 출력
print(f"평가 성공: {wins}/200 ({wins / 200:.1%})") # 소수점 첫째자리까지 출력 

# 성공한 데이터가 최소 1개 이상 존재하는 경우에만 상세 통계 출력
if success_lengths:
    print(
        f"성공 경로 길이: 평균 {np.mean(success_lengths):.2f} actions, "
        f"최소 {min(success_lengths)}, 최대 {max(success_lengths)}"
    )
else:
    # 200판 중 단 한 판도 성공하지 못했을 경우 예외 안내
    print("성공한 판이 없어 성공 경로 길이를 계산할 수 없습니다.")


# JSON 파일 저장 및 마무리 환경 정리
# 현재 스크립트 파일과 동일한 디렉토리에 'evaluation_results.json' 파일 경로 설정
result_path = Path(__file__).resolve().parent / "evaluation_results.json"

# json.dumps를 통해 딕셔너리를 문자열로 변환 후 파일에 기록
# ensure_ascii=False: 한글이나 특수문자가 \uXXXX 형태로 깨지지 않고 그대로 저장되도록 설정
# indent=2: 가독성을 위해 2칸 들여쓰기로 보기 좋게 포맷팅
result_path.write_text(
    json.dumps(evaluation_results, ensure_ascii=False, indent=2) + "\n",
    encoding="utf-8",
)

# 콘솔창에 JSON 저장 완료 메시지 출력
print(f"평가 결과 저장: {result_path}")

# 메모리 해제 및 프로세스 종료를 위해 사용했던 환경(Gym Environments)들을 안전하게 닫음
eval_env.close()  # 평가용 환경 종료
env.close()       # 학습용 환경 종료

# 1. 데이터 가공 및 이동 평균(Moving Average) 계산을 위한 준비
# 에피소드 번호 생성 (1부터 시작하여 총 에피소드 수만큼 1씩 증가하는 배열)
episode_numbers = np.arange(1, len(returns) + 1) 

# 최근 성공률을 계산할 이동 평균 윈도우 크기 설정 (최근 100개 에피소드 기준)
window = 100 

# FrozenLake는 성공 시 1, 실패 시 0의 보상을 가짐 -> 보상이 1인 경우를 성공(1.0)으로 변환
successes = (np.asarray(returns) == 1).astype(float) 

# np.convolve(합성곱)를 활용하여 최근 100개 에피소드의 '이동 평균 성공률' 계산
# mode="valid"는 윈도우(100개)가 데이터와 완전히 겹치는 구간만 계산하므로, 결과 배열의 길이는 [전체 길이 - window + 1]이 됨
success_rate = np.convolve(successes, np.ones(window) / window, mode="valid") 


# Matplotlib을 이용한 2단 서브플롯(Subplots) 시각화 설정
# 세로로 2개의 그래프를 배치 (figsize: 가로 10, 세로 7 인치)
# sharex=True를 통해 상단과 하단 그래프의 X축(에피소드) 범위를 동기화
fig, axes = matplotlib.pyplot.subplots(2, 1, figsize=(10, 7), sharex=True) 

# --- 상단 그래프 (axes[0]): 에피소드별 가공되지 않은 실제 보상 데이터 ---
# s=3: 점의 크기를 작게 설정, alpha=0.25: 투명도를 주어 점들이 밀집된 구간(데이터 밀도)을 시각적으로 파악
axes[0].scatter(episode_numbers, returns, s=3, alpha=0.25) 
axes[0].set_title("Training reward per episode")           # 그래프 제목
axes[0].set_ylabel("Total reward")                        # Y축 라벨
axes[0].set_yticks([0, 1])                                 # Y축 눈금을 0과 1로 고정 (FrozenLake 보상 특성)
axes[0].set_ylim(-0.05, 1.05)                              # 점이 경계선에 잘리지 않도록 Y축 여백 설정

# --- 하단 그래프 (axes[1]): 최근 100개 에피소드 기준 성공률 추이 ---
# 이동 평균 특성상 앞선 (window - 1)개의 데이터는 계산할 수 없으므로, X축도 그만큼 건너뛰고 매칭
axes[1].plot(episode_numbers[window - 1:], success_rate * 100) 
axes[1].set_title(f"Training success rate (latest {window} episodes)") # 그래프 제목
axes[1].set_xlabel("Episode")                                          # X축 라벨 (공통 사용)
axes[1].set_ylabel("Success rate (%)")                                 # Y축 라벨
axes[1].set_ylim(0, 100)                                               # 백분율이므로 Y축 범위를 0~100으로 제한


# 그래프 스타일 세부 조정 및 전체 타이틀 설정
# 모든 서브플롯에 격자(Grid)를 추가하고 투명도(alpha)를 조절하여 가독성 향상
for ax in axes:
    ax.grid(alpha=0.3) 

# 그래프 전체의 메인 타이틀 설정 (사용한 환경 정보 및 하이퍼파라미터 기록)
fig.suptitle(f"FrozenLake | slippery=False | alpha={alpha} | gamma={gamma} | seed=0") 

# 각 서브플롯과 타이틀 간의 간격을 자동으로 최적화하여 글자가 겹치지 않게 함
fig.tight_layout() 


# 이미지 파일 저장 및 마무리
# 현재 실행 중인 스크립트 파일과 동일한 디렉토리에 'training_results.png' 저장 경로 설정
graph_path = Path(__file__).resolve().parent / "training_results.png" 

# 설정한 경로에 고해상도(dpi=180) 이미지 파일로 그래프 저장
fig.savefig(graph_path, dpi=180) 

# 파일 저장 후, 메모리 누수를 방지하기 위해 생성했던 pyplot figure 객체를 닫아줌
matplotlib.pyplot.close(fig) 

# 콘솔창에 저장 완료 메시지 출력
print(f"Training graph saved: {graph_path}")


# Q-table heatmap: 행은 상태, 열은 행동, 숫자와 색은 학습된 Q값.
output_dir = Path(__file__).resolve().parent
map_cells = map_desc.astype("U1").ravel()
fig, ax = plt.subplots(figsize=(7, 9))
image = ax.imshow(Q, cmap="viridis", vmin=0, vmax=max(1.0, float(Q.max())), aspect="auto")
ax.set_xticks(range(4), ["0: Left", "1: Down", "2: Right", "3: Up"])
ax.set_yticks(range(len(Q)), [f"{s}: {cell}" for s, cell in enumerate(map_cells)])
ax.set_xlabel("Action")
ax.set_ylabel("State: map cell")
ax.set_title("Learned Q-table | FrozenLake 4x4")
for state_index in range(Q.shape[0]):
    for action_index in range(Q.shape[1]):
        value = Q[state_index, action_index]
        ax.text(action_index, state_index, f"{value:.3f}", ha="center", va="center",
                color="black" if value > 0.55 else "white", fontsize=9)
fig.colorbar(image, ax=ax, label="Estimated discounted return")
fig.text(0.5, 0.015, "H / G: terminal states; outgoing action values remain zero.",
         ha="center", fontsize=9)
fig.tight_layout(rect=(0, 0.035, 1, 1))
heatmap_path = output_dir / "q_table_heatmap.png"
fig.savefig(heatmap_path, dpi=180)
plt.close(fig)
print(f"Q-table heatmap saved: {heatmap_path}")

# 정책 격자: choose_action과 동일하게 정확히 최대인 행동을 모두 표시.
# 일반 칸의 Q값이 모두 0이면 방향을 임의로 그리지 않고 별도로 표시.
from matplotlib.patches import Rectangle

fig, ax = plt.subplots(figsize=(7, 7))
rows, cols = map_desc.shape
colors = {"S": "#c7e8ff", "F": "#edf5fc", "H": "#344054", "G": "#b7e6c4"}
arrows = np.array(["←", "↓", "→", "↑"])
for state_index, cell in enumerate(map_cells):
    row, col = divmod(state_index, cols)
    zero_values = cell not in ("H", "G") and np.all(Q[state_index] == 0)
    facecolor = "#ffe2a8" if zero_values else colors[cell]
    ax.add_patch(Rectangle((col, row), 1, 1, facecolor=facecolor, edgecolor="white", linewidth=2))
    text_color = "white" if cell == "H" else "#172b4d"
    ax.text(col + 0.08, row + 0.17, f"{state_index}: {cell}",
            ha="left", va="center", color=text_color, fontsize=11)
    if cell in ("H", "G"):
        label = "HOLE" if cell == "H" else "GOAL"
        ax.text(col + 0.5, row + 0.6, label, ha="center", va="center",
                color=text_color, fontsize=17, weight="bold")
    elif zero_values:
        ax.text(col + 0.5, row + 0.6, "Q = 0\ncheck learning", ha="center", va="center",
                color=text_color, fontsize=12)
    else:
        best_actions = np.flatnonzero(Q[state_index] == Q[state_index].max())
        ax.text(col + 0.5, row + 0.6, " ".join(arrows[best_actions]),
                ha="center", va="center", color=text_color, fontsize=26)
ax.set_xlim(0, cols)
ax.set_ylim(rows, 0)
ax.set_aspect("equal")
ax.axis("off")
ax.set_title("Greedy policy | FrozenLake 4x4", pad=16)
fig.text(0.5, 0.07, "S: start   H: hole   G: goal\nAll tied best actions are shown; evaluation chooses among them randomly.\nOrange: all Q-values are zero; inspect learning coverage.",
         ha="center", va="center", fontsize=9)
fig.tight_layout(rect=(0, 0.12, 1, 1))
policy_path = output_dir / "policy_grid.png"
fig.savefig(policy_path, dpi=180)
plt.close(fig)
print(f"Policy grid saved: {policy_path}")
