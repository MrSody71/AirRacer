"""Tests for AirRacer gymnasium environment."""

import numpy as np
import pytest
from gymnasium.utils.env_checker import check_env

from air_racer.env import AirRacerEnv
from air_racer import config as C


@pytest.fixture
def env():
    e = AirRacerEnv()
    yield e
    e.close()


def test_env_creation(env):
    assert env.observation_space.shape == (C.OBS_SIZE,)
    assert env.action_space.shape == (C.ACTION_SIZE,)


def test_reset_returns_valid_obs(env):
    obs, info = env.reset()
    assert obs.shape == (C.OBS_SIZE,)
    assert env.observation_space.contains(obs)
    assert isinstance(info, dict)


def test_step_with_zeros(env):
    env.reset()
    obs, reward, terminated, truncated, info = env.step(np.zeros(4, dtype=np.float32))
    assert obs.shape == (C.OBS_SIZE,)
    assert isinstance(reward, float)
    assert not terminated
    assert not truncated


def test_step_with_full_throttle(env):
    env.reset()
    action = np.array([1.0, 0.0, 0.0, 0.0], dtype=np.float32)
    obs, reward, terminated, truncated, info = env.step(action)
    assert info["speed"] > C.MIN_SPEED


def test_action_clipping(env):
    env.reset()
    action = np.array([5.0, -5.0, 5.0, -5.0], dtype=np.float32)
    obs, reward, terminated, truncated, info = env.step(action)
    assert env.observation_space.contains(obs)


def test_checkpoint_reached(env):
    env.reset()
    # Teleport airplane to near checkpoint 0
    env.position = C.CHECKPOINT_POSITIONS[0].astype(np.float64).copy()
    env.prev_distance = 100.0
    obs, reward, terminated, truncated, info = env.step(np.zeros(4, dtype=np.float32))
    assert env.next_cp == 1
    assert reward > C.REWARD_CHECKPOINT * 0.5  # Should include checkpoint bonus


def test_out_of_bounds(env):
    env.reset()
    env.position = np.array([C.OUT_OF_BOUNDS_RADIUS + 1000, 0, C.TRACK_ALTITUDE])
    obs, reward, terminated, truncated, info = env.step(np.zeros(4, dtype=np.float32))
    assert terminated


def test_below_ground(env):
    env.reset()
    env.position = np.array([0, 0, -100.0])
    obs, reward, terminated, truncated, info = env.step(np.zeros(4, dtype=np.float32))
    assert terminated


def test_max_steps_truncation(env):
    env.reset()
    env.steps = C.MAX_STEPS - 1
    obs, reward, terminated, truncated, info = env.step(np.zeros(4, dtype=np.float32))
    assert truncated


def test_full_race_possible(env):
    """Verify that manually hitting all checkpoints completes the race."""
    env.reset()
    for lap in range(C.NUM_LAPS):
        for cp_idx in range(C.NUM_CHECKPOINTS):
            env.position = C.CHECKPOINT_POSITIONS[cp_idx].astype(np.float64).copy()
            env.prev_distance = 100.0
            obs, reward, terminated, truncated, info = env.step(
                np.zeros(4, dtype=np.float32)
            )
            if terminated:
                break
        if terminated:
            break
    assert terminated  # Race should be finished


def test_gymnasium_check_env(env):
    """Standard gymnasium environment validation."""
    check_env(env, skip_render_check=True)
