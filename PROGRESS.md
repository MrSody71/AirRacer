# AirRacer - Progress Snapshot

## Variant 48: Intelligent Agents for Airplane Racing Game
- **Tech stack:** C++ + Unreal Engine 5.6 + Python (RL training)
- **Deadline:** 6 days from 2026-06-12 (до 2026-06-18)
- **Repo:** https://github.com/MrSody71/AirRacer
- **Project path:** C:\AirRacer\AirRacer\
- **UE Editor language:** Russian

## Current Status: Day 6 IN PROGRESS (2026-06-13)

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
- [x] Race state machine: ERaceState (MainMenu -> Racing -> Paused -> Finished)
- [x] Colored airplane model from primitives (fuselage, nose, wings, tail)
- [x] Colored checkpoint rings (12 spheres, green=active, gray=inactive)
- [x] Environment: green island ground plane + blue ocean + 60 procedural trees
- [x] Hidden broken Open World landscape/HLOD/cloud artifacts
- [x] Dynamic materials via BasicShapeMaterial + "Color" parameter

### DONE (Day 3):
- [x] Python RL environment (gymnasium) — 3D airplane flight simulation
- [x] Train PPO agent (stable-baselines3, 500K timesteps)
- [x] Export to ONNX (airracer_bot.onnx, 2KB)
- [x] 11 pytest tests (all passing)
- [x] Dockerfile for training

### DONE (Day 4):
- [x] AiBotPawn (C++) — AI airplane with flight physics, red color scheme
- [x] AiBotController (C++) — rule-based steering via dot product targeting
- [x] 3 AI bots with different max speeds (Alpha=2400, Bravo=2100, Charlie=1800)
- [x] Bots spawn near player at race start, fly checkpoints autonomously
- [x] Per-bot checkpoint/lap tracking with overlap detection
- [x] Race position system (1st/2nd/3rd/4th) in HUD
- [x] Live standings list showing all racers
- [x] AIModule added to Build.cs

### DONE (Day 5):
- [x] Unit tests (pytest, 11 tests passing)
- [x] SAST (bandit)
- [x] docker-compose.yml
- [x] Packaged build fixes: C++ pawn spawn, input assets via ConstructorHelpers
- [x] Checkpoints spawned from C++ (World Partition fix)
- [x] HUD click detection via FViewport::KeyState
- [x] Collision mesh for overlap detection
- [x] Development + Shipping .exe builds working

### DONE (Day 6):
- [x] README.md
- [x] PlantUML diagrams (class, sequence, C4 context, C4 container)
- [ ] Пояснительная записка (по структуре из методички)
- [ ] Presentation (10 slides)
- [ ] Final Shipping build

## Key Files:
- `Source/AirRacer/AirplanePawn.h/cpp` — Player airplane pawn (flight, mouse controls, visual model)
- `Source/AirRacer/AiBotPawn.h/cpp` — AI airplane pawn (same physics, red scheme, configurable speed)
- `Source/AirRacer/AiBotController.h/cpp` — AI steering logic (dot product toward checkpoints)
- `Source/AirRacer/CheckpointActor.h/cpp` — Race checkpoint (sphere ring, highlight, overlap for player+bots)
- `Source/AirRacer/AirRaceGameMode.h/cpp` — Race logic, state machine, bot spawning, positions, environment
- `Source/AirRacer/RaceHUD.h/cpp` — Canvas HUD + menus + race positions
- `Source/AirRacer/GroundPlane.h/cpp` — Green island ground plane
- `Python/air_racer/env.py` — Gymnasium RL environment (3D flight sim)
- `Python/air_racer/config.py` — Flight constants matching UE5
- `Python/train.py` — PPO training script
- `Python/export_onnx.py` — ONNX export for UE5 integration
- `Python/airracer_bot.onnx` — Trained neural network (12 float in, 4 float out)
- `Python/Dockerfile` — Training container
- `Content/Input/` — IA_Throttle, IA_Pitch, IA_Yaw, IA_Roll, IA_Pause, IMC_Flight
- `Docs/` — UML diagrams (class_diagram.puml, sequence_race_flow.puml)

## Technical Notes:
- Dynamic materials: use `/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial` with "Color" param
- Landscape hiding: Open World template landscape is broken (checkerboard), hidden at runtime via Tick
- Canvas HUD chosen over UMG widgets (more reliable without Blueprint setup)
- Mouse input: APlayerController::GetInputMouseDelta() in Tick, sensitivity = 10.0
- Bot overlap: BotMesh needs a StaticMesh (invisible cube) for overlap detection to work
- Bot steering: dot product of desired direction with local axes, multiplied by 5x gain
- Editor requires `-DDC-ForceMemoryCache` flag to launch
- Live Coding blocks external builds — editor must be closed for builds
- Modules: Landscape, AIModule added to Build.cs
