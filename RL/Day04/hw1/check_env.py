"""Step 1: inspect Humanoid-v5 without training or opening a window."""
import argparse
import json
import platform
import time
from importlib.metadata import version
from pathlib import Path

import gymnasium as gym
import numpy as np

# 학습 전 확인용: 무작위 행동으로 환경이 정상 작동하는지만 확인한다.

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--steps', type=int, default=1000)
    parser.add_argument('--seed', type=int, default=0)
    args = parser.parse_args()
    if args.steps < 1:
        parser.error('--steps must be positive')
    started = time.perf_counter()
    # 별도의 앱을 실행하지 않아도 여기서 MuJoCo 환경을 생성한다.
    # render_mode를 지정하지 않아 로봇 창은 뜨지 않는다.
    env = gym.make('Humanoid-v5')
    try:
        obs, info = env.reset(seed=args.seed)
        # 초기 상태의 난수와 무작위 행동의 난수는 따로 설정한다.
        env.action_space.seed(args.seed)
        # unwrapped로 시간 제한 등의 래퍼 안에 있는 실제 MuJoCo 환경에 접근.
        base = env.unwrapped
        # shape는 숫자 배열의 크기. 현재 기본 관측 348개, 행동 17개.
        report = {
            'environment': 'Humanoid-v5',
            'os': platform.platform(),
            'versions': {p: version(p) for p in ['gymnasium', 'mujoco', 'numpy']},
            'seed': args.seed,
            'policy': 'random actions (no training)',
            'observation_shape': list(obs.shape),
            'action_shape': list(env.action_space.shape),
            'action_low': env.action_space.low.tolist(),
            'action_high': env.action_space.high.tolist(),
            'simulation_seconds_per_step': float(base.dt),
            'max_episode_steps': env.spec.max_episode_steps,
            'actuator_names': [base.model.actuator(i).name for i in range(base.model.nu)],
            'first_step': None,
            'completed_episodes': [],
        }
        episode_return = 0.0
        episode_steps = 0
        for step in range(args.steps):
            # 학습한 정책 대신 허용 범위에서 임의의 행동을 뽑는다.
            action = env.action_space.sample()
            obs, reward, terminated, truncated, info = env.step(action)
            # terminated: 환경 종료 조건, truncated: 시간 제한 등에 의한 중단.
            # NaN/무한대가 나오면 정상적인 실험을 계속할 수 없으므로 중단한다.
            if not np.isfinite(obs).all() or not np.isfinite(reward):
                raise RuntimeError('Non-finite observation or reward')
            episode_return += float(reward)
            episode_steps += 1
            if step == 0:
                # 첫 행동의 보상과 상세 항목을 예시로 저장한다.
                report['first_step'] = {
                    'reward': float(reward), 'terminated': bool(terminated),
                    'truncated': bool(truncated),
                    'info': {k: np.asarray(v).tolist() for k, v in info.items()},
                }
            if terminated or truncated:
                # 한 에피소드가 끝났으므로 기록하고 초기화한다.
                report['completed_episodes'].append({
                    'steps': episode_steps, 'return': episode_return,
                    'terminated': bool(terminated), 'truncated': bool(truncated),
                })
                obs, info = env.reset()
                episode_return, episode_steps = 0.0, 0
        report['total_steps'] = args.steps
        report['unfinished_episode_steps'] = episode_steps
        # 이 속도는 물리 계산/무작위 행동 속도이며 PPO 학습 속도가 아니다.
        report['elapsed_seconds'] = time.perf_counter() - started
        path = Path(__file__).resolve().parent / 'results' / 'env_check.json'
        # 버전과 관측/행동 정보 등을 보고서에서 확인할 수 있도록 저장한다.
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
        print(json.dumps({k: v for k, v in report.items() if k != 'completed_episodes'}, indent=2))
        print(f'Completed episodes: {len(report["completed_episodes"])}')
        print(f'Full record: {path.resolve()}')
    finally:
        env.close()


if __name__ == '__main__':
    main()
