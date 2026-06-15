"""AirRacer Gymnasium environment — matches UE5 left-handed coordinate system exactly.

Coordinate system (identical to UE5):
  +X = forward, +Y = right, +Z = up  (left-handed)

Rotation conventions (identical to UE5 FRotator + AddActorLocalRotation):
  Pitch = rotation around -Y axis (positive = nose up)
  Yaw   = rotation around +Z axis (positive = turn right)
  Roll  = rotation around -X axis (positive = bank right / right wing down)

Combined delta rotation: R_delta = Rx_neg(roll) @ Ry_neg(pitch) @ Rz(yaw)
Local rotation update:   R_new = R_old @ R_delta
"""

import gymnasium as gym
import numpy as np
from gymnasium import spaces

from . import config as C


# ---------------------------------------------------------------------------
# UE5-matched rotation matrices
# ---------------------------------------------------------------------------

def _rx_neg(angle_deg: float) -> np.ndarray:
    """Rotation around -X axis by *angle_deg* (UE5 Roll)."""
    r = np.radians(angle_deg)
    c, s = np.cos(r), np.sin(r)
    return np.array([
        [1.0,  0.0,  0.0],
        [0.0,    c,    s],
        [0.0,   -s,    c],
    ])


def _ry_neg(angle_deg: float) -> np.ndarray:
    """Rotation around -Y axis by *angle_deg* (UE5 Pitch)."""
    r = np.radians(angle_deg)
    c, s = np.cos(r), np.sin(r)
    return np.array([
        [  c,  0.0,   -s],
        [0.0,  1.0,  0.0],
        [  s,  0.0,    c],
    ])


def _rz(angle_deg: float) -> np.ndarray:
    """Rotation around +Z axis by *angle_deg* (UE5 Yaw)."""
    r = np.radians(angle_deg)
    c, s = np.cos(r), np.sin(r)
    return np.array([
        [  c,   -s,  0.0],
        [  s,    c,  0.0],
        [0.0,  0.0,  1.0],
    ])


def _reorthogonalise(R: np.ndarray) -> np.ndarray:
    """Keep rotation matrix orthogonal (SVD snap)."""
    U, _, Vt = np.linalg.svd(R)
    return U @ Vt


class AirRacerEnv(gym.Env):
    """3D airplane racing environment with UE5-identical physics."""

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
        self.rotation = np.eye(3, dtype=np.float64)   # 3x3 rotation matrix
        self.speed = C.MIN_SPEED
        self.next_cp = 0
        self.current_lap = 0
        self.steps = 0
        self.prev_distance = 0.0

    # ------------------------------------------------------------------
    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)

        cp0 = C.CHECKPOINT_POSITIONS[0].astype(np.float64)
        cp1 = C.CHECKPOINT_POSITIONS[1].astype(np.float64)

        # Place airplane behind checkpoint 0, facing toward it
        self.position = cp0 + (cp0 - cp1) / np.linalg.norm(cp1 - cp0) * 2000.0
        self.position[2] = C.TRACK_ALTITUDE

        # Build rotation facing toward cp0
        fwd = cp0 - self.position
        fwd /= np.linalg.norm(fwd)
        yaw = np.degrees(np.arctan2(fwd[1], fwd[0]))
        pitch = np.degrees(np.arcsin(np.clip(fwd[2], -1.0, 1.0)))
        self.rotation = (_ry_neg(pitch) @ _rz(yaw)).astype(np.float64)

        self.speed = C.MIN_SPEED
        self.next_cp = 0
        self.current_lap = 0
        self.steps = 0
        self.prev_distance = float(np.linalg.norm(cp0 - self.position))

        return self._get_obs(), {}

    # ------------------------------------------------------------------
    def step(self, action):
        action = np.clip(action, -1.0, 1.0).astype(np.float64)
        throttle, pitch_in, yaw_in, roll_in = action

        # Update speed (matches AiBotPawn::Tick)
        self.speed += throttle * C.ACCELERATION * C.DT
        self.speed = np.clip(self.speed, C.MIN_SPEED, C.MAX_SPEED)

        # Compute rotation deltas in degrees
        d_pitch = pitch_in * C.PITCH_SPEED * C.DT
        d_yaw = yaw_in * C.YAW_SPEED * C.DT
        d_roll = roll_in * C.ROLL_SPEED * C.DT

        # Apply local rotation — identical to UE5 AddActorLocalRotation(FRotator)
        delta = _rx_neg(d_roll) @ _ry_neg(d_pitch) @ _rz(d_yaw)
        self.rotation = self.rotation @ delta

        # Re-orthogonalise every 100 steps to prevent drift
        if self.steps % 100 == 0:
            self.rotation = _reorthogonalise(self.rotation)

        # Move forward (first column of rotation = forward vector)
        forward = self.rotation[:, 0]
        self.position += forward * self.speed * C.DT

        self.steps += 1

        # --- Reward logic ---
        reward = C.REWARD_TIME_PENALTY
        terminated = False
        truncated = False

        cp_pos = C.CHECKPOINT_POSITIONS[self.next_cp].astype(np.float64)
        dist = float(np.linalg.norm(cp_pos - self.position))

        # Progress reward (approach checkpoint)
        progress = self.prev_distance - dist
        reward += progress * C.REWARD_PROGRESS_SCALE / C.TRACK_RADIUS
        self.prev_distance = dist

        # Heading reward: bonus for pointing toward the checkpoint
        to_cp = cp_pos - self.position
        to_cp_norm = to_cp / (np.linalg.norm(to_cp) + 1e-8)
        heading_dot = float(np.dot(forward, to_cp_norm))  # 1.0 = perfect aim
        reward += heading_dot * C.REWARD_HEADING_SCALE

        # Proximity bonus: exponential reward when close to checkpoint
        # Peaks at dist=0, falls off with distance. Teaches precision approach.
        proximity = np.exp(-dist / C.CHECKPOINT_RADIUS) * C.REWARD_PROXIMITY_SCALE
        reward += proximity

        # Speed reward: bonus for speed, but penalise high speed when
        # the checkpoint is NOT straight ahead (need to brake for turns)
        speed_ratio = self.speed / C.MAX_SPEED
        reward += speed_ratio * C.REWARD_SPEED_SCALE
        if heading_dot < 0.8:
            # Checkpoint is to the side — penalise excess speed
            reward -= speed_ratio * C.REWARD_OVERSPEED_PENALTY * (1.0 - heading_dot)

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

            if not terminated:
                new_cp = C.CHECKPOINT_POSITIONS[self.next_cp].astype(np.float64)
                self.prev_distance = float(np.linalg.norm(new_cp - self.position))

        # Out of bounds?
        if float(np.linalg.norm(self.position[:2])) > C.OUT_OF_BOUNDS_RADIUS:
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

    # ------------------------------------------------------------------
    def _get_obs(self):
        cp_pos = C.CHECKPOINT_POSITIONS[self.next_cp].astype(np.float64)

        # Relative vector to checkpoint in WORLD space
        rel = cp_pos - self.position
        dist = float(np.linalg.norm(rel))

        # Transform to LOCAL frame (R^T @ world_vec) — so the NN sees
        # "checkpoint is to my right/left/above/below" directly
        rel_local = self.rotation.T @ rel
        rel_local_norm = rel_local / (C.TRACK_RADIUS * 2.0)

        # World-up in local frame — helps the agent stabilise roll
        world_up = np.array([0.0, 0.0, 1.0])
        up_local = self.rotation.T @ world_up

        speed_norm = self.speed / C.MAX_SPEED
        dist_norm = dist / (C.TRACK_RADIUS * 2.0)

        total = C.NUM_LAPS * C.NUM_CHECKPOINTS
        done = self.current_lap * C.NUM_CHECKPOINTS + self.next_cp
        progress = done / total

        obs = np.concatenate([
            rel_local_norm.astype(np.float32),   # 3: target direction in local frame
            up_local.astype(np.float32),          # 3: world-up in local frame (roll ref)
            np.array([speed_norm, dist_norm, progress], dtype=np.float32),  # 3: scalars
        ])
        return obs
