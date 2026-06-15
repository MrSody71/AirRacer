"""Flight and race constants — extracted from UE5 C++ source (AirplanePawn.h/cpp)."""

import numpy as np

# Flight physics (from AirplanePawn.h defaults)
MAX_SPEED = 3000.0
MIN_SPEED = 500.0
ACCELERATION = 1500.0
PITCH_SPEED = 90.0   # degrees/sec
YAW_SPEED = 60.0     # degrees/sec
ROLL_SPEED = 120.0   # degrees/sec

# Simulation
DT = 1.0 / 30.0  # 30 FPS equivalent
MAX_STEPS = 6000

# Race
NUM_CHECKPOINTS = 5
NUM_LAPS = 3
CHECKPOINT_RADIUS = 800.0  # trigger box extent from CheckpointActor

# Track layout — checkpoints in a ring
TRACK_RADIUS = 20000.0
TRACK_ALTITUDE = 1000.0
OUT_OF_BOUNDS_RADIUS = 50000.0
GROUND_LEVEL = 0.0

# Generate checkpoint positions (evenly spaced circle)
CHECKPOINT_POSITIONS = np.array([
    [
        np.cos(2.0 * np.pi * i / NUM_CHECKPOINTS) * TRACK_RADIUS,
        np.sin(2.0 * np.pi * i / NUM_CHECKPOINTS) * TRACK_RADIUS,
        TRACK_ALTITUDE,
    ]
    for i in range(NUM_CHECKPOINTS)
], dtype=np.float32)

# Reward weights
REWARD_CHECKPOINT = 200.0            # big payoff for actually hitting the ring
REWARD_LAP = 500.0
REWARD_FINISH = 2000.0
REWARD_TIME_PENALTY = -0.01
REWARD_PROGRESS_SCALE = 50.0
REWARD_HEADING_SCALE = 0.05          # bonus for pointing toward checkpoint
REWARD_SPEED_SCALE = 0.01            # reduced — was encouraging too much speed
REWARD_PROXIMITY_SCALE = 0.3         # exponential bonus near checkpoint
REWARD_OVERSPEED_PENALTY = 0.03      # penalty for high speed when turning
REWARD_CRASH = -200.0

# Observation size (rel_local:3 + up_local:3 + speed/dist/progress:3)
OBS_SIZE = 9
# Action size
ACTION_SIZE = 4
