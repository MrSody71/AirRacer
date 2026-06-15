#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AiBotController.generated.h"

class AAiBotPawn;
class ACheckpointActor;

// Предварительное объявление типа NNE, чтобы не тянуть тяжёлые заголовки в .h
namespace UE::NNE { class IModelInstanceCPU; }

UCLASS()
class AIRRACER_API AAiBotController : public AAIController
{
	GENERATED_BODY()

public:
	AAiBotController();
	virtual ~AAiBotController();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	// ---- Управление нейросетью (RL) ----

	// Включить управление обученной моделью. Если выключить или модель не загрузится —
	// бот автоматически использует rule-based управление (см. ниже).
	UPROPERTY(EditAnywhere, Category = "AI|NeuralNetwork")
	bool bUseNeuralNetwork = true;

	// Путь к импортированному ONNX-ассету (NNEModelData). По умолчанию ожидается в Content/AI.
	UPROPERTY(EditAnywhere, Category = "AI|NeuralNetwork")
	FString ModelAssetPath = TEXT("/Game/AI/airracer_bot.airracer_bot");

	// Python env теперь использует ту же левостороннюю СК, что UE5 — коррекции не нужны.

private:
	AAiBotPawn* BotPawn = nullptr;

	// ---- Состояние NNE ----
	bool bTriedInit = false;   // попытка инициализации уже была (чтобы не пытаться каждый кадр)
	bool bModelReady = false;  // модель успешно загружена и готова к инференсу
	TSharedPtr<UE::NNE::IModelInstanceCPU> ModelInstance;
	TArray<float> InputData;   // 12 чисел наблюдения
	TArray<float> OutputData;  // 4 числа действия

	// Константы, идентичные Python/air_racer/config.py — нужны для совпадения наблюдений с обучением
	static constexpr float OBS_TRACK_RADIUS = 20000.0f; // C.TRACK_RADIUS
	static constexpr float OBS_MAX_SPEED    = 3000.0f;  // C.MAX_SPEED
	static constexpr int32 OBS_NUM_CP       = 5;        // C.NUM_CHECKPOINTS
	static constexpr int32 OBS_NUM_LAPS     = 3;        // C.NUM_LAPS
	static constexpr int32 OBS_SIZE         = 9;        // C.OBS_SIZE (rel_local:3 + up_local:3 + scalars:3)
	static constexpr int32 ACT_SIZE         = 4;        // C.ACTION_SIZE

	// Инициализация модели (один раз). Возвращает true, если инференс доступен.
	bool InitNeuralNetwork();

	// Управление обученной моделью на одном тике. Возвращает true, если применилось.
	bool RunNeuralControl(float DeltaTime);

	// Прежнее rule-based управление (dot-product наведение). Запасной вариант и базовая линия.
	void RunRuleBasedControl(float DeltaTime);

	// Steering gains (для rule-based)
	float PitchGain = 2.5f;
	float YawGain = 3.0f;
	float RollGain = 1.5f;
};
