"""Train PPO agent for AirRacer."""

import argparse

from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import SubprocVecEnv

from air_racer.env import AirRacerEnv


def make_env():
    def _init():
        return AirRacerEnv()
    return _init


def main():
    parser = argparse.ArgumentParser(description="Train AirRacer PPO agent")
    parser.add_argument("--timesteps", type=int, default=500_000, help="Total training timesteps")
    parser.add_argument("--n-envs", type=int, default=4, help="Number of parallel environments")
    parser.add_argument("--output", type=str, default="ppo_airracer", help="Output model path")
    args = parser.parse_args()

    env = SubprocVecEnv([make_env() for _ in range(args.n_envs)])

    model = PPO(
        "MlpPolicy",
        env,
        verbose=1,
        learning_rate=3e-4,
        n_steps=2048,
        batch_size=64,
        n_epochs=10,
        gamma=0.99,
        gae_lambda=0.95,
        clip_range=0.2,
        ent_coef=0.01,
        tensorboard_log="./tb_logs/",
    )

    print(f"Training PPO for {args.timesteps} timesteps with {args.n_envs} envs...")
    model.learn(total_timesteps=args.timesteps)
    model.save(args.output)
    print(f"Model saved to {args.output}.zip")

    env.close()


if __name__ == "__main__":
    main()
