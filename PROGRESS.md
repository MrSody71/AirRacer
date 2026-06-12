# AirRacer - Progress Snapshot

## Variant 48: Intelligent Agents for Airplane Racing Game
- **Tech stack:** C++ + Unreal Engine 5.6 + Python (RL training)
- **Deadline:** 6 days from 2026-06-12 (до 2026-06-18)
- **Repo:** https://github.com/MrSody71/AirRacer
- **Project path:** C:\AirRacer\AirRacer\
- **UE Editor language:** Russian

## Current Status: Day 2 COMPLETE (2026-06-12)

### DONE (Day 1):
- [x] UE 5.6 C++ project created
- [x] Git repo + GitHub remote (MrSody71/AirRacer)
- [x] MSVC v14.38 + .NET Framework 4.6.2 DevPack configured
- [x] AirplanePawn (C++) — flight physics, Enhanced Input, camera (SpringArm)
- [x] Input Actions: IA_Throttle, IA_Pitch, IA_Yaw, IA_Roll (Axis1D float)
- [x] IMC_Flight mapping: W/S=газ, Arrows=тангаж, A/D=рыскание, Q/E=крен
- [x] BP_Airplane blueprint — inputs assigned, Auto Possess Player 0
- [x] CheckpointActor (C++) — trigger box (800x800), overlap detection
- [x] AirRaceGameMode (C++) — lap counter (3 laps), timer, checkpoint tracking (5 checkpoints)
- [x] BP_Checkpoint blueprint — 5 штук размещены на уровне (indices 0-4)
- [x] GameMode set via DefaultEngine.ini
- [x] UML diagrams — Docs/class_diagram.puml, Docs/sequence_race_flow.puml

### DONE (Day 2):
- [x] RaceHUD (Canvas-based) — speed, lap, timer, best lap, checkpoint counter
- [x] Mouse flight controls (pitch + yaw via GetInputMouseDelta)
- [x] Q/E roll fix (ETriggerEvent::Completed handlers for key release)
- [x] IA_Pause + ESC key for pause menu
- [x] Main Menu screen (PLAY / QUIT buttons)
- [x] Pause Menu screen (RESUME / QUIT buttons)
- [x] Finish Screen (total time, best lap, RESTART / QUIT)
- [x] Race state machine: ERaceState (MainMenu → Racing → Paused → Finished)
- [x] Colored airplane model from primitives (fuselage, nose, wings, tail)
- [x] Colored checkpoint rings (12 spheres, green=active, gray=inactive)
- [x] Environment: green island ground plane + blue ocean + 60 procedural trees
- [x] Hidden broken Open World landscape/HLOD/cloud artifacts
- [x] Dynamic materials via BasicShapeMaterial + "Color" parameter

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
- `Source/AirRacer/AirplanePawn.h/cpp` — Player airplane pawn (flight, mouse controls, visual model)
- `Source/AirRacer/CheckpointActor.h/cpp` — Race checkpoint (sphere ring, highlight, overlap)
- `Source/AirRacer/AirRaceGameMode.h/cpp` — Race logic, state machine, environment spawning
- `Source/AirRacer/RaceHUD.h/cpp` — Canvas HUD + menus (main, pause, finish)
- `Source/AirRacer/GroundPlane.h/cpp` — Green island ground plane
- `Content/Input/` — IA_Throttle, IA_Pitch, IA_Yaw, IA_Roll, IA_Pause, IMC_Flight
- `Content/BP_Airplane.uasset` — Player blueprint
- `Content/BP_Checkpoint.uasset` — Checkpoint blueprint
- `Content/RaceLevel.umap` — Main race level
- `Docs/` — UML diagrams (class_diagram.puml, sequence_race_flow.puml)

## Technical Notes:
- Dynamic materials: use `/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial` with "Color" param
- Landscape hiding: Open World template landscape is broken (checkerboard), hidden at runtime via Tick
- Canvas HUD chosen over UMG widgets (more reliable without Blueprint setup)
- Mouse input: APlayerController::GetInputMouseDelta() in Tick, sensitivity = 10.0
- Editor requires `-DDC-ForceMemoryCache` flag to launch
- Live Coding blocks external builds — editor must be closed for builds
- Landscape module added to Build.cs for ALandscapeProxy iteration
