from pathlib import Path
import os
os.environ.setdefault('MPLCONFIGDIR', '/tmp/rl_day03_mpl')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

def plot_training(log_path):
    # 전달받은 기록의 실제 학습률로 제목과 저장 이름 결정
    log_path = Path(log_path)
    data = np.load(log_path)
    lr = float(data['learning_rate'])
    seed = int(data['seed'])
    output_path = log_path.parent / f'training_curves_lr{lr}.png'
    returns = data['episode_returns']
    losses = data['loss_history']
    steps = data['eval_steps']
    means = data['eval_means']
    stds = data['eval_stds']
    fig, axes = plt.subplots(2, 2, figsize=(12, 8), constrained_layout=True)
    fig.suptitle(f'CartPole-v1 DQN | CPU | seed={seed} | lr={lr} | gamma=0.99\n40,000 environment steps | batch=64 | replay=20,000 | target copy=500 steps', fontsize=13)

    ax = axes[0, 0]
    episodes = np.arange(1, len(returns) + 1)
    ax.plot(episodes, returns, alpha=0.35, color='#2878b5', label='Episode return')
    window = 20
    if len(returns) >= window:
        ax.plot(episodes[window-1:], np.convolve(returns, np.ones(window)/window, mode='valid'), color='#14466b', label='20-episode moving average')
    ax.set(title='Training return (exploration enabled)', xlabel='Completed episode', ylabel='Undiscounted return')
    ax.legend(fontsize=8)

    ax = axes[0, 1]
    ax.plot(steps, means, marker='o', markersize=3, color='#23956f', label='Mean return')
    ax.fill_between(steps, means-stds, means+stds, alpha=0.2, color='#23956f', label='Mean +/- 1 std')
    ax.set(title='Greedy evaluation | 10 episodes/check | seeds 1000-1009', xlabel='Environment step', ylabel='Undiscounted return')
    ax.legend(fontsize=8)

    ax = axes[1, 0]
    updates = np.arange(1, len(losses)+1)
    ax.plot(updates, losses, alpha=0.3, color='#da8740', linewidth=0.7, label='Batch loss')
    window = 100
    if len(losses) >= window:
        ax.plot(updates[window-1:], np.convolve(losses, np.ones(window)/window, mode='valid'), color='#974b13', label='100-update moving average')
    ax.set(title='TD loss | SmoothL1 | main-loop updates only', xlabel='Optimizer update', ylabel='Loss')
    ax.legend(fontsize=8)

    ax = axes[1, 1]
    epsilon = data['epsilon_history']
    ax.plot(np.arange(1, len(epsilon)+1), epsilon, color='#8757ab')
    ax.set(title='Exploration schedule', xlabel='Environment step', ylabel='Epsilon', ylim=(0, 1.05))
    for ax in axes.flat:
        ax.grid(alpha=0.2)
    fig.savefig(output_path, dpi=160)
    plt.close(fig)
    data.close()
    print(f"그래프 저장: {output_path}")
    return output_path


if __name__ == "__main__":
    from config import LOG_PATH
    plot_training(LOG_PATH)
