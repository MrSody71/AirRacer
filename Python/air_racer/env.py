"""AirRacer Gymnasium environment — 3D airplane race simulation."""

import gymnasium as gym
import numpy as np
from gymnasium import spaces
from scipy.spatial.transform import Rotation

from . import config as C


class AirRacerEnv(gym.Env):
    """3D airplane racing environment matching UE5 flight physics."""

    metadata = {"render_modes": []}

    def __init__(self):
        super().__init__()

        # Action: [throttle, pitch, yaw, roll] in [-1, 1]
        self.action_space = spaces.Box(
            low=-1.0, high=1.0, shape=(C.ACTION_SIZE,), dtype=np.float32
        )

        # Observation: 12 floats
        self.observation_space = spaces.Box(
            low=-np.inf, high=np.inf, shape=(C.OBS_SIZE,), dtype=np.float32
        )

        self.position = np.zeros(3, dtype=np.float64)
        self.rotation = Rotation.identity()
        self.speed = C.MIN_SPEED
        self.next_cp = 0
        self.current_lap = 0
        self.steps = 0
        self.prev_distance = 0.0

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)

        # Start near checkpoint 0, facing toward checkpoint 1
        cp0 = C.CHECKPOINT_POSITIONS[0].astype(np.float64)
        cp1 = C.CHECKPOINT_POSITIONS[1].astype(np.float64)

        # Place airplane slightly behind checkpoint 0
        start_offset = cp0 - (cp1 - cp0)
        start_offset = cp0 + (cp0 - cp1) / np.linalg.norm(cp1 - cp0) * 2000.0
        self.position = start_offset.copy()
        self.position[2] = C.TRACK_ALTITUDE

        # Face toward checkpoint 0
        fwd = cp0 - self.position
        fwd = fwd / np.linalg.norm(fwd)
        yaw = np.degrees(np.arctan2(fwd[1], fwd[0]))
        pitch = np.degrees(np.arcsin(np.clip(fwd[2], -1, 1)))
        self.rotation = Rotation.from_euler("ZYX", [yaw, pitch, 0.0], degrees=True)

        self.speed = C.MIN_SPEED
        self.next_cp = 0
        self.current_lap = 0
        self.steps = 0
        self.prev_distance = float(np.linalg.norm(
            C.CHECKPOINT_POSITIONS[0] - self.position
        ))

        return self._get_obs(), {}

    def step(self, action):
        action = np.clip(action, -1.0, 1.0).astype(np.float64)
        throttle, pitch_in, yaw_in, roll_in = action

        # Update speed (matches AirplanePawn::Tick)
        self.speed += throttle * C.ACCELERATION * C.DT
        self.speed = np.clip(self.speed, C.MIN_SPEED, C.MAX_SPEED)

        # Compute local rotation deltas (degrees)
        d_pitch = pitch_in * C.PITCH_SPEED * C.DT
        d_yaw = yaw_in * C.YAW_SPEED * C.DT
        d_roll = roll_in * C.ROLL_SPEED * C.DT

        # Apply local rotation (matches AddActorLocalRotation)
        delta_rot = Rotation.from_euler("YXZ", [d_yaw, d_pitch, d_roll], degrees=True)
        self.rotation = self.rotation * delta_rot

        # Move forward
        forward = self.rotation.apply([1.0, 0.0, 0.0])
        self.position += forward * self.speed * C.DT

        self.steps += 1

        # Check checkpoint
        reward = C.REWARD_TIME_PENALTY
        terminated = False
        truncated = False

        cp_pos = C.CHECKPOINT_POSITIONS[self.next_cp].astype(np.float64)
        dist = float(np.linalg.norm(cp_pos - self.position))

        # Progress reward
        progress = self.prev_distance - dist
        reward += progress * C.REWARD_PROGRESS_SCALE / C.TRACK_RADIUS
        self.prev_distance = dist

        # Checkpoint reached?
        if dist < C.CHECKPOINT_RADIUS:
            reward += C.REWARD_CHECKPOINT
            self.next_cp += 1

            if self.next_cp >= C.NUM_CHECKPOINTS:
                self.next_cp = 0
                self.current_lap += 1
                reward += C.REWARD_LAP

                if self.current_lap >= C.NUM_LAPS:
                    reward += C.REWARD_FINISH
                    terminated = True

            # Update prev_distance for next checkpoint
            if not terminated:
                new_cp = C.CHECKPOINT_POSITIONS[self.next_cp].astype(np.float64)
                self.prev_distance = float(np.linalg.norm(new_cp - self.position))

        # Out of bounds?
        dist_from_origin = float(np.linalg.norm(self.position[:2]))
        if dist_from_origin > C.OUT_OF_BOUNDS_RADIUS:
            reward += C.REWARD_CRASH
            terminated = True

        # Below ground?
        if self.position[2] < C.GROUND_LEVEL:
            reward += C.REWARD_CRASH
            terminated = True

        # Max steps?
        if self.steps >= C.MAX_STEPS:
            truncated = True

        info = {
            "lap": self.current_lap,
            "checkpoint": self.next_cp,
            "speed": self.speed,
            "distance": dist,
        }

        return self._get_obs(), float(reward), terminated, truncated, info

    def _get_obs(self):
        cp_pos = C.CHECKPOINT_POSITIONS[self.next_cp].astype(np.float64)

        # Relative vector to checkpoint (normalized)
        rel = cp_pos - self.position
        rel_norm = rel / (C.TRACK_RADIUS * 2.0)

        # Forward and up directions
        forward = self.rotation.apply([1.0, 0.0, 0.0])
        up = self.rotation.apply([0.0, 0.0, 1.0])

        # Normalized speed
        speed_norm = self.speed / C.MAX_SPEED

        # Distance normalized
        dist_norm = float(np.linalg.norm(rel)) / (C.TRACK_RADIUS * 2.0)

        # Race progress fraction
        total = C.NUM_LAPS * C.NUM_CHECKPOINTS
        done = self.current_lap * C.NUM_CHECKPOINTS + self.next_cp
        progress = done / total

        obs = np.concatenate([
            rel_norm.astype(np.float32),       # 3
            forward.astype(np.float32),         # 3
            up.astype(np.float32),              # 3
            np.array([speed_norm, dist_norm, progress], dtype=np.float32),  # 3
        ])
        return obs
