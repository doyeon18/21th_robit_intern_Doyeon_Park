"""Train one controlled PPO condition and evaluate the saved model."""
import argparse
import csv
import json
import platform
import time
from importlib.metadata import version
from pathlib import Path

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import BaseCallback, CheckpointCallback, CallbackList
from stable_baselines3.common.monitor import Monitor

# 전체 순서: 설정 읽기 → 환경/모델 생성 → 학습 → 저장 → 불러오기 → 20회 평가.
# 이 파일은 PPO를 직접 구현하지 않고 Stable-Baselines3의 구현을 사용한다.
# 관측(obs)은 로봇 상태 348개, 행동(action)은 연속 제어 입력 17개다.

BASE_DIR = Path(__file__).resolve().parent


def write_json(path, data):
    # 저장 도중 진행률 파일을 읽어도 불완전한 JSON이 보이지 않도록,
    # 임시 파일에 먼저 쓴 다음 완성된 파일로 교체한다.
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    temporary.replace(path)


class Progress(BaseCallback):
    # Callback은 학습 도중 SB3가 정해진 시점에 자동 호출하는 보조 기능이다.
    def __init__(self, output, budget):
        super().__init__()
        self.output, self.budget = output, budget

    def _on_training_start(self):
        # 실제 벽시계 시간 측정 시작. 시뮬레이션 속 시간과는 다르다.
        self.started = time.perf_counter()

    def _on_step(self):
        # 환경 step: 행동 한 번을 보내고 다음 관측/보상을 받는 단위.
        # 2,048 step마다 진행률을 기록한다. 이 시점에는 마지막 PPO 업데이트가
        # 아직 끝나지 않았을 수 있어 최종 속도는 summary.json으로 확인한다.
        if self.num_timesteps % 2048 == 0:
            elapsed = time.perf_counter() - self.started
            write_json(self.output / 'progress.json', {
                'status': 'training', 'steps': self.num_timesteps,
                'budget': self.budget, 'elapsed_seconds': elapsed,
                'steps_per_second': self.num_timesteps / elapsed,
            })
        return True  # False를 반환하면 SB3가 학습을 중단한다.


def evaluate(model, env_id, count, seed_start):
    # 학습 때 사용한 환경과 분리하여 평가한다. 이 함수에서는 학습하지 않는다.
    env = gym.make(env_id)
    rows = []
    try:
        for seed in range(seed_start, seed_start + count):
            # 두 모델에 같은 seed 목록을 사용해 같은 초기 조건으로 비교한다.
            # reset: 종료된 로봇을 새 에피소드의 초기 상태로 되돌린다.
            obs, _ = env.reset(seed=seed)
            total_reward, steps, distance, action_sum = 0.0, 0, 0.0, 0.0
            reward_terms = {}
            while True:
                # deterministic=True: 정책 분포에서 무작위로 뽑지 않고
                # 이 Gaussian 정책의 평균에 해당하는 행동을 사용한다.
                action, _ = model.predict(obs, deterministic=True)
                # 환경에 실제로 전달하는 입력 범위(-0.4~0.4)를 보장한다.
                action = np.clip(action, env.action_space.low, env.action_space.high)
                obs, reward, terminated, truncated, info = env.step(action)
                # obs: 다음 상태, reward: 이번 step 보상, info: 속도/보상 항목 등.
                # terminated: 환경 종료 조건 충족, truncated: 시간 제한 등으로 중단.
                total_reward += float(reward)
                steps += 1
                # dt는 환경 1 step에 흐르는 시뮬레이션 시간(여기서는 0.015초).
                distance += float(info['x_velocity']) * env.unwrapped.dt
                action_sum += float(np.square(action).sum())
                # 보상 총합뿐 아니라 자세 유지/전진/제어/접촉 항목도 따로 합산.
                for key, value in info.items():
                    if key.startswith('reward_'):
                        reward_terms[key] = reward_terms.get(key, 0.0) + float(value)
                if terminated or truncated:
                    break
            seconds = steps * env.unwrapped.dt
            # 평균 속도 = 전진 변위 / 시뮬레이션 시간.
            # 제어 입력 제곱 평균 = 전체 제곱합 / (step 수 × 행동 차원 수).
            # 제어 입력 지표는 실제 소비 에너지와 동일한 값이 아니다.
            rows.append({
                'evaluation_seed': seed, 'return': total_reward,
                'steps': steps, 'simulated_seconds': seconds,
                'mean_forward_velocity': distance / seconds,
                'mean_squared_action': action_sum / (steps * env.action_space.shape[0]),
                'terminated': bool(terminated), 'truncated': bool(truncated),
                **reward_terms,
            })
    finally:
        # 정상 종료든 오류든 환경 자원을 정리한다.
        env.close()
    return rows


def main():
    # 예: --config configs/baseline.json --output results/baseline
    # 설정 파일을 바꿔 같은 학습 코드를 A와 B에 공통으로 사용한다.
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--smoke-steps', type=int, help='Short validation only; not a final experiment')
    args = parser.parse_args()
    # 상대 경로는 터미널 위치가 아닌 이 코드가 있는 hw1 폴더 기준으로 해석한다.
    if not args.config.is_absolute():
        args.config = BASE_DIR / args.config
    if not args.output.is_absolute():
        args.output = BASE_DIR / args.output
    config = json.loads(args.config.read_text(encoding='utf-8'))
    if args.smoke_steps is not None:
        # 코드가 작동하는지만 빠르게 확인할 때만 학습량을 줄인다.
        config['total_timesteps'] = args.smoke_steps
    budget = config['total_timesteps']
    # PPO 기본값은 2,048 step 경험을 모은 뒤 그 데이터로 여러 번 학습한다.
    # 정수배를 사용해야 실제 수집량이 목표를 넘어가는 것을 피할 수 있다.
    if budget < 2048 or budget % 2048:
        parser.error('Step budget must be a positive multiple of PPO rollout size 2048')
    if args.output.exists():
        # 기존 모델과 실험 결과를 실수로 덮어쓰지 않는다.
        parser.error('Output directory already exists; choose a new directory')
    args.output.mkdir(parents=True)
    torch.set_num_threads(config['threads'])
    # 실행 당시 설정을 결과 폴더에도 복사해 나중에 같은 실험을 재현한다.
    write_json(args.output / 'config.json', config)
    # gym.make는 MuJoCo의 Humanoid 환경을 생성한다. 창 없이 물리 계산만 한다.
    # Monitor는 에피소드마다 총보상/길이를 train.monitor.csv에 기록한다.
    env = Monitor(gym.make(config['env_id']), str(args.output / 'train'))
    started = time.perf_counter()
    try:
        # MlpPolicy: 숫자 관측을 받는 신경망 정책. Actor와 Critic을 함께 사용한다.
        # learning_rate: 신경망 가중치를 수정하는 보폭. A/B에서 바꾸는 유일한 설정.
        # seed: 초기 가중치와 난수 설정, device='cpu': CPU에서 계산.
        # 다른 PPO 설정과 신경망 구조는 설치된 SB3 버전의 기본값을 사용한다.
        model = PPO('MlpPolicy', env, learning_rate=config['learning_rate'],
                    seed=config['seed'], device='cpu', verbose=1)
        callbacks = CallbackList([
            Progress(args.output, budget),
            # 약 10만 환경 step마다 중간 모델을 보관한다. 환경이 1개인 설정 기준.
            CheckpointCallback(save_freq=100352, save_path=str(args.output / 'checkpoints'), name_prefix='ppo'),
        ])
        # 실제 학습은 이 줄에서 수행된다.
        # SB3 내부: 경험 수집 → Return/Advantage 계산 → PPO 손실로 가중치 갱신 반복.
        # 기본적으로 2,048 step씩 수집하고, batch 64로 나눠 10 epoch 업데이트한다.
        model.learn(total_timesteps=budget, callback=callbacks)
        training_seconds = time.perf_counter() - started
        assert model.num_timesteps == budget
        # 최종 모델을 저장한 뒤 다시 불러와 저장 파일이 정상인지 확인한다.
        model.save(args.output / 'model')
        loaded = PPO.load(args.output / 'model', device='cpu')
        obs, _ = env.reset(seed=999)
        # 같은 관측을 넣었을 때 저장 전후의 결정적 행동이 완전히 같아야 한다.
        np.testing.assert_array_equal(model.predict(obs, deterministic=True)[0],
                                      loaded.predict(obs, deterministic=True)[0])
        write_json(args.output / 'progress.json', {'status': 'evaluating', 'steps': budget})
        rows = evaluate(loaded, config['env_id'], config['evaluation_episodes'], config['evaluation_seed_start'])
        # 평가 1회당 한 행: 평균만 남기지 않고 개별 결과도 CSV로 보관한다.
        with (args.output / 'evaluation.csv').open('w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
        metrics = {}
        # 표준편차는 이번 모델의 평가 에피소드 사이 편차다.
        # 학습 seed를 바꿔 재학습했을 때의 변동성을 뜻하지 않는다.
        for key in ['return', 'steps', 'simulated_seconds', 'mean_forward_velocity', 'mean_squared_action']:
            values = np.asarray([row[key] for row in rows])
            metrics[key] = {'mean': float(values.mean()), 'std': float(values.std(ddof=0))}
        report = {
            # 버전/설정/실제 step 수/평가 지표를 보고서 근거로 남긴다.
            'purpose': 'short validation' if args.smoke_steps else 'comparison experiment',
            'config': config, 'actual_steps': model.num_timesteps,
            'training_seconds': training_seconds,
            'steps_per_second': model.num_timesteps / training_seconds,
            'versions': {p: version(p) for p in ['gymnasium', 'mujoco', 'torch', 'stable-baselines3', 'numpy']},
            'os': platform.platform(), 'device': 'cpu', 'reload_actions_identical': True,
            'evaluation_count': len(rows), 'deterministic': True,
            'metrics': metrics,
            'terminated_count': sum(r['terminated'] for r in rows),
            'truncated_count': sum(r['truncated'] for r in rows),
            'limitations': 'One training seed. Evaluation std describes episodes, not variability across retraining.',
        }
        write_json(args.output / 'summary.json', report)
        write_json(args.output / 'progress.json', {'status': 'complete', 'steps': budget})
        print(json.dumps(report, indent=2))
    except BaseException as exc:
        # 오류나 중단 시 완료로 표시하지 않고 원인을 진행 파일에 남긴다.
        write_json(args.output / 'progress.json', {'status': 'failed_or_interrupted', 'error': str(exc)})
        raise
    finally:
        env.close()


if __name__ == '__main__':
    main()
