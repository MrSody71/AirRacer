# AirRacer

Гоночная игра на самолётах с интеллектуальными агентами (ИИ-ботами), разработанная на Unreal Engine 5.6 (C++) с Python-модулем обучения с подкреплением.

**Курсовой проект** — Вариант 48: Интеллектуальные агенты для игры в жанре «гонки на самолётах».
**Дисциплина** — Методы и технологии программирования.
**Выполнил** — Артюх Виталий, группа 221131.

## Возможности

- **Гоночный геймплей** — полёт на самолёте через 5 чекпоинтов, 3 круга, таймер, лучший круг
- **Комбинированное управление** — клавиатура (WASD/стрелки/Q/E) + мышь (тангаж и рыскание)
- **3 ИИ-бота** — автономные противники с rule-based навигацией (dot-product steering к чекпоинтам), разные скорости (Alpha 2400, Bravo 2100, Charlie 1800)
- **Python RL-пайплайн** — Gymnasium-среда, PPO-агент (stable-baselines3, 500K шагов), экспорт в ONNX
- **Процедурный мир** — зелёный остров (куб 600x600), синий океан, 60 деревьев из примитивов (цилиндр + конус)
- **Canvas HUD** — скорость, круг, таймер, лучший круг, позиция (1st–4th), таблица участников
- **Система меню** — главное меню (PLAY/QUIT), пауза (RESUME/QUIT), экран финиша (время, лучший круг, RESTART/QUIT)
- **Пакетирование** — standalone .exe (Shipping), без редактора

## Архитектура

### Компонентная схема (UE5 C++)

```
AirRacer (UE 5.6, C++20)
│
├── AAirRaceGameMode (GameModeBase)
│   ├── Управляет состоянием гонки (ERaceState: MainMenu → Racing → Paused → Finished)
│   ├── Спавнит: чекпоинты (5 шт.), ИИ-ботов (3 шт.), землю, океан, деревья (60 шт.)
│   ├── Считает круги, время, лучший круг
│   ├── Рассчитывает позиции (1st–4th) по кругу/чекпоинту/дистанции
│   └── Скрывает сломанный ландшафт Open World шаблона
│
├── AAirplanePawn (Pawn) — Игрок
│   ├── Полёт: скорость 500–3000, ускорение 1500, тангаж/рыскание/крен
│   ├── Enhanced Input: IMC_Flight + 6 действий (газ, тангаж, рыскание, крен, пауза)
│   ├── Мышь: GetInputMouseDelta(), чувствительность 10.0
│   ├── Камера: SpringArm (600 ед., лаг 5.0) + CameraComponent
│   ├── Визуал: цилиндр-фюзеляж + конус-нос + кубы-крылья/хвост (синий/серый/красный/жёлтый)
│   └── Коллизия: невидимый куб для overlap-детекции с чекпоинтами
│
├── AAiBotPawn (Pawn) — ИИ-бот
│   ├── Та же физика полёта, что и у игрока
│   ├── Визуал: красно-оранжевая цветовая схема
│   ├── Настраиваемые лимиты скорости для каждого бота
│   └── Трекинг: NextCheckpointIndex, CurrentLap, bFinished
│
├── AAiBotController (AIController) — Навигация ботов
│   ├── Каждый тик: вычисляет направление к следующему чекпоинту
│   ├── Dot-product проекция на локальные оси (right → yaw, up → pitch)
│   ├── Агрессивный руль: gain 5x, всегда полный газ
│   └── Авто-выравнивание крена при малом отклонении
│
├── ACheckpointActor (Actor) — Чекпоинт
│   ├── UBoxComponent триггер (200×800×800)
│   ├── Кольцо из 12 сфер (радиус 600)
│   ├── Подсветка: зелёный = следующий, серый = пройден
│   ├── UPointLightComponent (500K intensity, 5000 radius)
│   └── Overlap → GameMode::OnCheckpointReached / OnBotCheckpointReached
│
├── ARaceHUD (HUD) — Интерфейс
│   ├── Canvas-отрисовка (DrawHUD override)
│   ├── Ввод: FViewport::KeyState() для мыши (обход ограничений packaged builds)
│   ├── Кнопки: DrawButton() с hit-test и edge detection (bWasMouseDown)
│   └── 4 экрана: DrawRaceHUD, DrawMainMenu, DrawPauseMenu, DrawFinishScreen
│
└── AGroundPlane (Actor) — Земля
    ├── Куб с масштабом 600×600×0.01
    └── Динамический зелёный материал (BasicShapeMaterial, параметр "Color")
```

### Компонентная схема (Python RL)

```
Python (RL Training Pipeline)
│
├── air_racer/env.py — Gymnasium-среда (AirRacerEnv)
│   ├── Observation (12 float): позиция, скорость, направление, до чекпоинта, крен, круг
│   ├── Action (4 float): throttle, pitch, yaw, roll ([-1, 1])
│   ├── 3D физика полёта, идентичная UE5 (scipy.spatial.transform.Rotation)
│   └── Награды: +100 чекпоинт, +500 круг, +2000 финиш, -0.1/шаг, -200 крэш
│
├── air_racer/config.py — Константы (из AirplanePawn.h)
│   ├── Физика: MAX_SPEED=3000, MIN_SPEED=500, ACCELERATION=1500
│   ├── Трек: 5 чекпоинтов по кругу (R=20000), 3 круга, 4000 макс. шагов
│   └── Веса наград
│
├── train.py — Обучение PPO
│   ├── stable-baselines3, 500K timesteps
│   ├── 4 параллельных среды (SubprocVecEnv)
│   ├── MLP policy [64, 64]
│   └── TensorBoard логирование
│
├── export_onnx.py — Экспорт в ONNX
│   ├── Загрузка модели → ONNX (opset 11)
│   ├── Размер: ~2 КБ
│   └── Вход: 12 float → Выход: 4 float
│
├── tests/test_env.py — 11 pytest тестов
│   ├── Создание среды, reset, step
│   ├── Прохождение чекпоинтов и полная гонка
│   ├── Out-of-bounds и below-ground терминация
│   ├── Truncation по MAX_STEPS
│   └── gymnasium.utils.env_checker.check_env()
│
├── Dockerfile — Контейнер (python:3.11-slim)
└── docker-compose.yml — 3 сервиса: train, export, test
```

## Стек технологий

| Компонент | Технологии |
|-----------|-----------|
| Игровой движок | Unreal Engine 5.6 |
| Язык (игра) | C++20, MSVC v14.38 |
| Язык (RL) | Python 3.12 |
| Сборка UE | UnrealBuildTool, RunUAT (BuildCookRun) |
| RL-фреймворк | Gymnasium 1.0, stable-baselines3, PyTorch |
| Экспорт модели | ONNX Runtime (opset 11) |
| Тестирование | pytest (11 тестов), gymnasium env_checker |
| SAST | bandit (Python) |
| Контейнеризация | Docker, Docker Compose (3 сервиса) |
| Контроль версий | Git (семантические коммиты), GitHub |
| AI-ассистент | Claude Code (Anthropic) |
| Диаграммы | PlantUML (class, sequence, C4) |

## Быстрый старт

### Вариант А — Запуск готового .exe (без Unreal Engine)

```bash
# 1. Скачать билд из релизов GitHub
# 2. Запустить
AirRacer.exe
```

### Вариант Б — Сборка из исходников

**Требования:**
- Unreal Engine 5.6 (установлен через Epic Games Launcher)
- MSVC v14.38 (Visual Studio 2022, компонент "C++ Desktop Development")
- .NET Framework 4.6.2 Targeting Pack

```bash
# 1. Клонировать репозиторий
git clone https://github.com/MrSody71/AirRacer.git
cd AirRacer

# 2. Открыть в редакторе
#    Двойной клик AirRacer.uproject → Unreal Engine 5.6 откроет проект
#    Или: File → Open Project в уже запущенном редакторе

# 3. Запуск в редакторе
#    Нажать Play в Unreal Editor (Alt+P)

# 4. Пакетирование в standalone .exe (Shipping)
"C:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/RunUAT.bat" ^
  BuildCookRun ^
  -project="<ПУТЬ>/AirRacer.uproject" ^
  -noP4 -platform=Win64 -clientconfig=Shipping ^
  -cook -build -stage -pak -archive ^
  -archivedirectory="<ПУТЬ_ВЫХОДНОЙ>" ^
  -ddc=InstalledNoZenLocalFallback

# 5. Результат: <ПУТЬ_ВЫХОДНОЙ>/Windows/AirRacer.exe
```

### Вариант В — Обучение RL-агента

**Требования:**
- Python 3.11+ с pip
- Docker и Docker Compose (для контейнерного запуска)

```bash
cd Python

# --- Через Docker Compose (рекомендуется) ---
docker compose up --build
# Запускает 3 сервиса последовательно:
#   1. train   — обучение PPO (500K шагов, ~10 мин)
#   2. export  — экспорт ONNX модели в output/
#   3. test    — запуск 11 pytest тестов

# --- Или локально ---
pip install -r requirements.txt

# Обучение
python train.py --timesteps 500000 --n-envs 4 --output ppo_airracer

# Экспорт обученной модели в ONNX
python export_onnx.py --model ppo_airracer --output airracer_bot.onnx

# Тесты
pytest tests/ -v

# SAST-анализ
pip install bandit
bandit -r air_racer/ -f json -o bandit_report.json
```

## Управление в игре

| Клавиша | Действие |
|---------|---------|
| **W** / **S** | Газ (ускорение) / Тормоз (замедление) |
| **Стрелки вверх/вниз** | Тангаж — нос вверх/вниз |
| **A** / **D** | Рыскание — поворот влево/вправо |
| **Q** / **E** | Крен — наклон влево/вправо |
| **Мышь** (движение) | Тангаж + рыскание (чувствительность 10.0) |
| **ESC** | Пауза (открывает меню паузы) |
| **PLAY** (в меню) | Начать гонку |
| **QUIT** (в меню) | Выход из игры |

### Правила гонки

1. После нажатия **PLAY** спавнятся 3 ИИ-бота рядом с игроком
2. Необходимо пролететь через 5 чекпоинтов (кольцевых ворот) **по порядку**
3. Следующий чекпоинт подсвечен **зелёным**, остальные серые
4. После всех 5 чекпоинтов засчитывается круг, всего **3 круга**
5. Позиция (1st–4th) обновляется в реальном времени
6. По завершении гонки показывается итоговое время и лучший круг

## Структура проекта

```
AirRacer/
├── AirRacer.uproject                  # Файл проекта Unreal Engine
├── README.md                          # Этот файл
│
├── Config/                            # Конфигурация UE5
│   ├── DefaultEngine.ini              # Настройки движка, GameMode
│   ├── DefaultGame.ini                # Название проекта
│   └── DefaultInput.ini               # Настройки ввода
│
├── Content/                           # Ассеты UE5 (бинарные .uasset)
│   ├── BP_Airplane.uasset             # Blueprint пешки игрока
│   ├── BP_Checkpoint.uasset           # Blueprint чекпоинта
│   └── Input/                         # Enhanced Input ассеты
│       ├── IMC_Flight.uasset          # Input Mapping Context
│       ├── IA_Throttle.uasset         # Действие: газ (W/S → float)
│       ├── IA_Pitch.uasset            # Действие: тангаж (стрелки → float)
│       ├── IA_Yaw.uasset              # Действие: рыскание (A/D → float)
│       ├── IA_Roll.uasset             # Действие: крен (Q/E → float)
│       └── IA_Pause.uasset            # Действие: пауза (ESC → bool)
│
├── Source/
│   ├── AirRacer.Target.cs             # Build target
│   ├── AirRacerEditor.Target.cs       # Editor build target
│   └── AirRacer/
│       ├── AirRacer.Build.cs          # Модули: Landscape, AIModule, EnhancedInput
│       ├── AirRacer.h / .cpp          # Модуль верхнего уровня
│       ├── AirplanePawn.h / .cpp      # Пешка игрока (274 строки)
│       ├── AiBotPawn.h / .cpp         # Пешка ИИ-бота (125 строк)
│       ├── AiBotController.h / .cpp   # Контроллер ИИ (76 строк)
│       ├── AirRaceGameMode.h / .cpp   # Логика гонки (497 строк)
│       ├── CheckpointActor.h / .cpp   # Чекпоинты (136 строк)
│       ├── RaceHUD.h / .cpp           # Canvas HUD + меню
│       └── GroundPlane.h / .cpp       # Зелёный остров
│
├── Python/                            # RL Training Pipeline
│   ├── air_racer/
│   │   ├── __init__.py
│   │   ├── config.py                  # Константы (физика, трек, награды)
│   │   └── env.py                     # Gymnasium-среда AirRacerEnv
│   ├── tests/
│   │   ├── __init__.py
│   │   └── test_env.py                # 11 pytest тестов
│   ├── train.py                       # Обучение PPO (SB3)
│   ├── export_onnx.py                 # Экспорт ONNX
│   ├── airracer_bot.onnx              # Обученная модель (2 КБ)
│   ├── requirements.txt               # Python-зависимости
│   ├── pyproject.toml                 # Конфигурация проекта
│   ├── Dockerfile                     # Контейнер (python:3.11-slim)
│   ├── docker-compose.yml             # 3 сервиса: train, export, test
│   ├── bandit_report.json             # Результат SAST-анализа
│   └── pytest_report.xml              # Результат тестов (JUnit XML)
│
└── Docs/                              # Документация и диаграммы
    ├── class_diagram.puml             # UML-диаграмма классов (PlantUML)
    ├── sequence_race_flow.puml        # UML-диаграмма последовательности
    ├── c4_context.puml                # C4 Context диаграмма
    └── c4_container.puml              # C4 Container диаграмма
```

## Диаграммы

В каталоге `Docs/` содержатся PlantUML-диаграммы:

- **class_diagram.puml** — диаграмма классов со всеми 7 игровыми классами, их атрибутами, методами и связями
- **sequence_race_flow.puml** — диаграмма последовательности: инициализация, игровой цикл, прохождение чекпоинтов, поведение ботов, пауза
- **c4_context.puml** — C4 Level 1: контекстная диаграмма (Игрок ↔ AirRacer ↔ RL Pipeline ↔ Docker)
- **c4_container.puml** — C4 Level 2: контейнерная диаграмма (все компоненты и их взаимодействие)

Для рендеринга: [PlantUML Online](https://www.plantuml.com/plantuml/uml/) или расширение PlantUML для VS Code.

## Тестирование

### Python-тесты (pytest)

```bash
cd Python && pytest tests/ -v
```

11 тестов покрывают:
| Тест | Описание |
|------|----------|
| `test_env_creation` | Создание среды, проверка размерностей |
| `test_reset_returns_valid_obs` | Reset возвращает корректное наблюдение |
| `test_step_with_zeros` | Step с нулевым действием |
| `test_step_with_full_throttle` | Полный газ увеличивает скорость |
| `test_action_clipping` | Действия за пределами [-1,1] обрезаются |
| `test_checkpoint_reached` | Телепорт к чекпоинту → награда |
| `test_out_of_bounds` | Вылет за границы → терминация |
| `test_below_ground` | Ниже земли → терминация |
| `test_max_steps_truncation` | Превышение MAX_STEPS → truncation |
| `test_full_race_possible` | Полная гонка (3 круга × 5 чекпоинтов) |
| `test_gymnasium_check_env` | Стандартная валидация Gymnasium |

### SAST-анализ (bandit)

```bash
cd Python && bandit -r air_racer/ -f json -o bandit_report.json
```

Результат: 0 уязвимостей (Severity: 0 High, 0 Medium, 0 Low).

## Технические решения

### Почему Canvas HUD вместо UMG Widgets?
Canvas-отрисовка (`AHUD::DrawHUD`) не требует Blueprint-настройки виджетов в редакторе. Все экраны (меню, пауза, HUD, финиш) создаются полностью из C++, что упрощает пакетирование и исключает зависимость от .uasset файлов виджетов.

### Почему процедурный мир из примитивов?
Все визуальные элементы (самолёты, деревья, чекпоинты, земля, океан) строятся в runtime из базовых форм движка (`/Engine/BasicShapes/Cube`, `Cylinder`, `Cone`, `Sphere`). Это гарантирует корректную работу в packaged builds без дополнительных ассетов.

### Проблема World Partition в packaged builds
При `DefaultPawnClass = nullptr` движок не создаёт пешку, нет позиции игрока → World Partition не стримит ячейки уровня → размещённые в редакторе акторы не загружаются. Решение: `DefaultPawnClass = AAirplanePawn::StaticClass()` + все ассеты загружаются через `ConstructorHelpers::FObjectFinder` в конструкторах.

### RL-среда повторяет физику UE5
Константы в `config.py` (скорости, ускорение, углы поворота) скопированы из `AirplanePawn.h`. Среда использует `scipy.spatial.transform.Rotation` для 3D-вращений, что обеспечивает совместимость обученной модели с игрой.

## Требования

### Для запуска .exe
- **Windows 10/11** (64-bit)

### Для сборки из исходников
- **Unreal Engine 5.6** (Epic Games Launcher)
- **Visual Studio 2022** с MSVC v14.38
- **.NET Framework 4.6.2 Targeting Pack**

### Для RL-обучения
- **Python 3.11+**
- **Docker** + **Docker Compose** (опционально)
- Зависимости: `gymnasium`, `stable-baselines3`, `torch`, `onnx`, `scipy`
