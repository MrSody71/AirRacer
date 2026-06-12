# AirRacer - Progress Snapshot

## Variant 48: Intelligent Agents for Airplane Racing Game
- **Tech stack:** C++ + Unreal Engine 5.6 + Python (RL training)
- **Deadline:** 6 days from 2026-06-12 (до 2026-06-18)
- **Repo:** https://github.com/MrSody71/AirRacer
- **Project path:** C:\AirRacer\AirRacer\
- **UE Editor language:** Russian

## Current Status: Day 1 COMPLETE (2026-06-12)

### DONE:
- [x] UE 5.6 C++ project created
- [x] Git repo + GitHub remote (MrSody71/AirRacer)
- [x] MSVC v14.38 + .NET Framework 4.6.2 DevPack configured
- [x] AirplanePawn (C++) — flight physics, Enhanced Input, camera (SpringArm)
- [x] Input Actions: IA_Throttle, IA_Pitch, IA_Yaw, IA_Roll (Axis1D float)
- [x] IMC_Flight mapping: W/S=газ, Arrows=тангаж, A/D=рыскание, Q/E=крен
- [x] BP_Airplane blueprint — inputs assigned, Cube mesh placeholder, Auto Possess Player 0
- [x] CheckpointActor (C++) — trigger box (800x800), overlap detection, debug logging
- [x] AirRaceGameMode (C++) — lap counter (3 laps), timer, checkpoint tracking (5 checkpoints)
- [x] BP_Checkpoint blueprint — 5 штук размещены на уровне (indices 0-4)
- [x] GameMode set via DefaultEngine.ini (GlobalDefaultGameMode)
- [x] RaceLevel saved in Content/
- [x] Player can fly and complete laps, logs confirm checkpoint detection

### TODO (Day 2): HUD + Track
- [ ] HUD widget (speed, lap, time, next checkpoint indicator)
- [ ] Improve race track (landscape or platforms, visual markers)
- [ ] Airplane mesh replacement (вместо куба)
- [ ] UML diagrams (PlantUML)

### TODO (Day 3): Python RL
- [ ] Python RL environment (gymnasium) — 2D race simulation
- [ ] Train PPO agent (stable-baselines3)
- [ ] Export to ONNX
- [ ] pytest tests for env
- [ ] Dockerfile for training

### TODO (Day 4): AI Bots in UE
- [ ] C++ ONNX Runtime integration in UE
- [ ] UAIBrainComponent — inference on tick
- [ ] AAIAirplaneController + 2-3 AI bots on track

### TODO (Day 5): Testing + Polish
- [ ] Unit tests (pytest for Python, UE automation tests)
- [ ] SAST (bandit, clang-tidy)
- [ ] docker-compose.yml
- [ ] Gameplay polish, balancing

### TODO (Day 6): Documentation
- [ ] README.md
- [ ] Пояснительная записка (по структуре из методички)
- [ ] PlantUML/Mermaid diagrams (class, sequence, C4)
- [ ] Presentation (10 slides)

## Key Files:
- `Source/AirRacer/AirplanePawn.h/cpp` — Player airplane pawn
- `Source/AirRacer/CheckpointActor.h/cpp` — Race checkpoint with trigger
- `Source/AirRacer/AirRaceGameMode.h/cpp` — Race logic (laps, timer)
- `Content/Input/` — IA_Throttle, IA_Pitch, IA_Yaw, IA_Roll, IMC_Flight
- `Content/BP_Airplane.uasset` — Player blueprint
- `Content/BP_Checkpoint.uasset` — Checkpoint blueprint
- `Content/RaceLevel.umap` — Main race level
- `Config/DefaultEngine.ini` — GameMode config

## UE Editor Notes:
- UE 5.6, Editor in Russian
- BP_Airplane: Auto Possess Player = Player 0
- Static Mesh: /Engine/BasicShapes/Cube (placeholder)
- Live Coding (Ctrl+Alt+F11) for code changes, but restart UE for constructor changes
- "Класс Blueprint" в русской локализации может называться "Схема Blueprint"
- "Настройки мира" не найдены в UI — используем DefaultEngine.ini напрямую

## Build Notes:
- MSVC v14.38 set in BuildConfiguration.xml
- .NET Framework 4.6.2 DevPack required for SwarmInterface
- Memory pressure during builds — 16GB RAM is tight, builds limited to 2 parallel
