"""완료된 두 실험으로 비교 그래프와 재현 가능한 실행 영상을 만든다."""
import csv
import ctypes.util
import json
import os
import subprocess
import sys
import tempfile
import shutil
from pathlib import Path

# 별도 창이나 GPU 없이 MuJoCo 영상을 그리는 소프트웨어 렌더러.
# Linux에 OSMesa가 있으면 사용하고, 그 외에는 MuJoCo의 기본 렌더러를 사용한다.
# 사용자가 지정한 MUJOCO_GL 설정은 유지한다.
if sys.platform.startswith('linux') and ctypes.util.find_library('OSMesa'):
    os.environ.setdefault('MUJOCO_GL', 'osmesa')
os.environ.setdefault('MPLCONFIGDIR', str(Path(tempfile.gettempdir()) / 'humanoid_matplotlib'))
os.environ.setdefault('MESA_SHADER_CACHE_DISABLE', 'true')
import gymnasium as gym
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import torch
from PIL import Image, ImageDraw, ImageFont
from stable_baselines3 import PPO

BASE_DIR = Path(__file__).resolve().parent
ROOT = BASE_DIR / 'results'
OUTPUT = ROOT / 'comparison'
CONDITIONS = [('baseline', 'A: lr=0.0003', '#2764ae'), ('changed', 'B: lr=0.0001', '#d47525')]


def read_csv(path):
    with path.open() as f:
        return list(csv.DictReader(line for line in f if not line.startswith('#')))


def make_plots():
    plt.rcParams.update({'font.size': 11, 'axes.spines.top': False, 'axes.spines.right': False})
    fig, ax = plt.subplots(figsize=(10, 5), layout='constrained')
    for name, label, color in CONDITIONS:
        rows = read_csv(ROOT / name / 'train.monitor.csv')
        steps = np.cumsum([int(r['l']) for r in rows])
        rewards = np.array([float(r['r']) for r in rows])
        window = 100
        smooth = np.convolve(rewards, np.ones(window) / window, mode='valid')
        ax.plot(steps[window - 1:], smooth, label=label, color=color)
    ax.set(xlabel='Environment steps', ylabel='Training episode return',
           title='Training return | moving average: 100 episodes')
    ax.ticklabel_format(axis='x', style='sci', scilimits=(0, 0))
    ax.grid(alpha=.2)
    ax.legend()
    fig.savefig(OUTPUT / 'learning_curves.png', dpi=160)
    plt.close(fig)

    keys = [('return', 'Episode return'), ('simulated_seconds', 'Episode duration (s)'),
            ('mean_forward_velocity', 'Mean forward velocity (m/s)'),
            ('mean_squared_action', 'Mean squared control input')]
    fig, axes = plt.subplots(2, 2, figsize=(11, 8), layout='constrained')
    for ax, (key, title) in zip(axes.flat, keys):
        values = []
        for i, (name, label, color) in enumerate(CONDITIONS):
            rows = read_csv(ROOT / name / 'evaluation.csv')
            v = np.array([float(r[key]) for r in rows])
            values.append(v)
            ax.bar(i, v.mean(), yerr=v.std(ddof=0), width=.5, color=color, alpha=.65, capsize=6)
            ax.scatter(i + np.linspace(-.14, .14, len(v)), v, s=16, color=color, edgecolors='white', linewidth=.4)
        # 같은 평가 seed끼리 연결하여 어떤 초기 조건에서 달라졌는지 보여준다.
        for a, b in zip(*values):
            ax.plot([0, 1], [a, b], color='gray', alpha=.15, linewidth=.7)
        ax.set_xticks([0, 1], ['A: 0.0003', 'B: 0.0001'])
        ax.set_title(title)
        ax.set_ylim(bottom=0)
        ax.grid(axis='y', alpha=.2)
    fig.suptitle('Final evaluation | 20 matched seeds | mean +/- episode SD\n'
                 'One training seed (0); dots are individual evaluations', fontsize=13)
    fig.savefig(OUTPUT / 'evaluation_comparison.png', dpi=160)
    plt.close(fig)


def make_videos():
    torch.set_num_threads(1)
    metadata = []
    # 특정 컴퓨터의 글꼴 경로에 의존하지 않는다. 없으면 Pillow 기본 글꼴 사용.
    try:
        font = ImageFont.truetype('DejaVuSans.ttf', 18)
    except OSError:
        font = ImageFont.load_default(size=18)
    for name, label, _ in CONDITIONS:
        summary = json.loads((ROOT / name / 'summary.json').read_text(encoding='utf-8'))
        rows = read_csv(ROOT / name / 'evaluation.csv')
        median = float(np.median([float(r['return']) for r in rows]))
        selected = {
            'representative': min(rows, key=lambda r: (abs(float(r['return']) - median), int(r['evaluation_seed']))),
            'lowest_return': min(rows, key=lambda r: (float(r['return']), int(r['evaluation_seed']))),
        }
        model = PPO.load(ROOT / name / 'model', device='cpu')
        for kind, row in selected.items():
            seed = int(row['evaluation_seed'])
            path = OUTPUT / f'{name}_{kind}_seed{seed}.mp4'
            # 로봇의 발까지 보이도록 카메라 중심과 각도를 두 조건에 동일하게 적용.
            env = gym.make('Humanoid-v5', render_mode='rgb_array', width=960, height=540,
                           default_camera_config={'trackbodyid': 1, 'distance': 3.5,
                                                  'lookat': np.array([0., 0., 1.]),
                                                  'elevation': -20., 'azimuth': 135.})
            process = None
            total_reward, steps = 0.0, 0
            try:
                obs, _ = env.reset(seed=seed)
                dt = env.unwrapped.dt
                process = subprocess.Popen([
                    'ffmpeg', '-hide_banner', '-loglevel', 'error', '-y',
                    '-f', 'rawvideo', '-pixel_format', 'rgb24', '-video_size', '960x640',
                    '-framerate', str(1 / dt), '-i', 'pipe:0', '-an',
                    '-c:v', 'libx264', '-crf', '20', '-pix_fmt', 'yuv420p',
                    '-movflags', '+faststart', str(path),
                ], stdin=subprocess.PIPE)
                while True:
                    action, _ = model.predict(obs, deterministic=True)
                    obs, reward, terminated, truncated, _ = env.step(action)
                    total_reward += float(reward)
                    steps += 1
                    frame = Image.new('RGB', (960, 640), '#102030')
                    frame.paste(Image.fromarray(env.render()), (0, 100))
                    draw = ImageDraw.Draw(frame)
                    draw.text((18, 8), f'{label} | {kind} | evaluation seed={seed}', font=font, fill='white')
                    draw.text((18, 38), f'checkpoint={summary["actual_steps"]:,} steps | deterministic | playback=1x', font=font, fill='white')
                    draw.text((18, 68), f't={steps * dt:.3f}s | return={total_reward:.2f} | terminated={terminated}', font=font, fill='white')
                    process.stdin.write(frame.tobytes())
                    if steps == max(1, int(row['steps']) // 2):
                        frame.save(OUTPUT / f'{name}_{kind}_preview.png')
                    if terminated or truncated:
                        break
                process.stdin.close()
                if process.wait() != 0:
                    raise RuntimeError('Video encoding failed')
                # 영상으로 재실행한 결과가 기존 평가 기록과 일치하는지 확인한다.
                np.testing.assert_allclose(total_reward, float(row['return']), atol=1e-5, rtol=0)
                assert steps == int(row['steps'])
                metadata.append({
                    'condition': name, 'kind': kind, 'evaluation_seed': seed,
                    'checkpoint_steps': summary['actual_steps'], 'steps': steps,
                    'return': total_reward, 'simulated_seconds': steps * dt,
                    'playback_speed': 1, 'fps': 1 / dt,
                    'terminated': bool(terminated), 'truncated': bool(truncated),
                    'final_root_height': float(env.unwrapped.data.qpos[2]),
                    'file': path.relative_to(BASE_DIR).as_posix(),
                })
                print(f'Saved {path}', flush=True)
            finally:
                if process is not None and process.poll() is None:
                    process.kill()
                    process.wait()
                env.close()
    (OUTPUT / 'video_metadata.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')


def main():
    if shutil.which('ffmpeg') is None:
        raise RuntimeError('MP4 생성에는 ffmpeg가 필요합니다. 설치 후 PATH에 등록하세요.')
    OUTPUT.mkdir(parents=True, exist_ok=True)
    make_plots()
    make_videos()


if __name__ == '__main__':
    main()
