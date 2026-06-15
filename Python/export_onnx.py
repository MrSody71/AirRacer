"""Export trained PPO model to ONNX for UE5 integration.

No VecNormalize — observations are already well-scaled by design,
so the C++ side feeds raw observations directly.
"""

import argparse

import numpy as np
import onnx
import onnxruntime as ort
import torch
import torch.nn as nn
from stable_baselines3 import PPO


class PolicyWrapper(nn.Module):
    """Wrapper that extracts deterministic actions from SB3 PPO policy."""

    def __init__(self, policy):
        super().__init__()
        self.features_extractor = policy.features_extractor
        self.policy_net = policy.mlp_extractor.policy_net
        self.action_net = policy.action_net

    def forward(self, obs):
        features = self.features_extractor(obs)
        latent = self.policy_net(features)
        actions = self.action_net(latent)
        return torch.tanh(actions)  # Bounded [-1, 1]


def main():
    parser = argparse.ArgumentParser(description="Export PPO to ONNX")
    parser.add_argument("--model", type=str, default="ppo_airracer", help="SB3 model path")
    parser.add_argument("--output", type=str, default="airracer_bot.onnx", help="ONNX output path")
    args = parser.parse_args()

    model = PPO.load(args.model)
    policy = model.policy.to("cpu")
    policy.eval()

    wrapper = PolicyWrapper(policy)
    wrapper.eval()

    obs_size = model.observation_space.shape[0]
    dummy = torch.randn(1, obs_size)

    torch.onnx.export(
        wrapper,
        dummy,
        args.output,
        input_names=["obs"],
        output_names=["action"],
        opset_version=17,
        dynamic_axes={"obs": {0: "batch"}, "action": {0: "batch"}},
    )

    # Verify
    onnx_model = onnx.load(args.output)
    onnx.checker.check_model(onnx_model)

    session = ort.InferenceSession(args.output)
    test_obs = np.random.randn(1, obs_size).astype(np.float32)
    result = session.run(None, {"obs": test_obs})
    print(f"ONNX export OK: input shape {test_obs.shape} -> output shape {result[0].shape}")
    print(f"Sample output: {result[0]}")
    print(f"Saved to {args.output}")


if __name__ == "__main__":
    main()
