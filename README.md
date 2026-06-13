# AirRacer

Гоночная игра на самолётах с интеллектуальными агентами (ИИ-ботами).

**Вариант 48** — Интеллектуальные агенты для авиагонок.

## Возможности

- **Гоночный геймплей** — полёт на самолёте через чекпоинты, 3 круга, таймер и лучший круг
- **Управление мышью и клавиатурой** — W/S газ, стрелки тангаж, A/D рыскание, Q/E крен, мышь pitch+yaw
- **3 ИИ-бота** — автономные противники с rule-based навигацией к чекпоинтам
- **Python RL обучение** — Gymnasium-среда, PPO-агент (stable-baselines3), экспорт в ONNX
- **Процедурный мир** — зелёный остров, океан, 60 деревьев, кольцевые ворота из примитивов
- **HUD и меню** — Canvas-HUD с позицией в гонке, главное меню, пауза, экран финиша
- **Пакетирование** — standalone .exe без редактора

## Архитектура

```
AirRacer (UE 5.6 C++)
├── AirplanePawn          — Пешка игрока (полёт, ввод, камера)
├── AiBotPawn             — Пешка ИИ-бота (полёт, визуал)
├── AiBotController       — Контроллер ИИ (навигация dot-product)
├── AirRaceGameMode       — Логика гонки, спавн, состояния
├── CheckpointActor       — Чекпоинт (триггер, кольцо из сфер)
├── RaceHUD               — Canvas HUD + меню
└── GroundPlane           — Земля (зелёный остров)

Python (RL Training)
├── air_racer/env.py      — Gymnasium-среда (3D полёт)
├── train.py              — Обучение PPO (500K шагов)
└── export_onnx.py        — Экспорт в ONNX
```

## Стек технологий

| Компонент | Технологии |
|-----------|-----------|
| Игровой движок | Unreal Engine 5.6, C++20 |
| Сборка | MSVC v14.38, UnrealBuildTool |
| RL обучение | Python 3.12, Gymnasium, stable-baselines3, PyTorch |
| Экспорт модели | ONNX Runtime |
| Тестирование | pytest (11 тестов), bandit (SAST) |
| Контейнеризация | Docker, Docker Compose |
| Контроль версий | Git, GitHub |

## Быстрый старт

### Запуск из .exe (без редактора)

```bash
# Скачать билд из релизов или собрать самостоятельно (см. ниже)
AirRacer.exe
```

### Сборка из исходников

```bash
# 1. Клонировать репозиторий
git clone https://github.com/MrSody71/AirRacer.git
cd AirRacer

# 2. Открыть в Unreal Engine 5.6
#    Файл -> AirRacer.uproject

# 3. Пакетирование (Shipping)
"C:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/RunUAT.bat" ^
  BuildCookRun ^
  -project="C:/AirRacer/AirRacer/AirRacer.uproject" ^
  -noP4 -platform=Win64 -clientconfig=Shipping ^
  -cook -build -stage -pak -archive ^
  -archivedirectory="C:/AirRacer/Build" ^
  -ddc=InstalledNoZenLocalFallback
```

### Обучение RL-агента

```bash
cd Python

# Через Docker Compose
docker compose up --build

# Или локально
pip install -r requirements.txt
python train.py          # Обучение PPO (500K шагов)
python export_onnx.py    # Экспорт в ONNX
pytest tests/ -v         # Запуск тестов
```

## Управление

| Клавиша | Действие |
|---------|---------|
| W / S | Газ / Тормоз |
| Стрелки вверх/вниз | Тангаж (нос вверх/вниз) |
| A / D | Рыскание (поворот влево/вправо) |
| Q / E | Крен (наклон влево/вправо) |
| Мышь | Тангаж + Рыскание |
| ESC | Пауза |

## Структура проекта

```
AirRacer/
├── Source/AirRacer/        # C++ исходники (UE5)
│   ├── AirplanePawn.*      # Пешка игрока
│   ├── AiBotPawn.*         # Пешка ИИ-бота
│   ├── AiBotController.*   # Контроллер ИИ
│   ├── AirRaceGameMode.*   # Логика гонки
│   ├── CheckpointActor.*   # Чекпоинты
│   ├── RaceHUD.*           # HUD и меню
│   └── GroundPlane.*       # Земля
├── Content/                # Ассеты UE5
│   └── Input/              # Enhanced Input (IMC + IA)
├── Python/                 # RL обучение
│   ├── air_racer/          # Gymnasium-среда
│   ├── tests/              # pytest тесты
│   ├── train.py            # Обучение PPO
│   ├── export_onnx.py      # Экспорт ONNX
│   ├── Dockerfile          # Контейнер для обучения
│   └── docker-compose.yml  # Оркестрация
├── Docs/                   # UML-диаграммы
└── AirRacer.uproject       # Файл проекта UE5
```

## Требования

- **Unreal Engine 5.6** (для сборки из исходников)
- **MSVC v14.38** + .NET Framework 4.6.2 Targeting Pack
- **Python 3.12+** (для RL обучения)
- **Docker** (опционально, для контейнерного обучения)
- **Windows 10/11** (для запуска .exe)
