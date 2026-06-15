#include "AiBotController.h"
#include "AiBotPawn.h"
#include "AirRaceGameMode.h"
#include "CheckpointActor.h"

// Заголовки NNE (для инференса нейросети)
#include "NNE.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"

AAiBotController::AAiBotController()
{
	PrimaryActorTick.bCanEverTick = true;
}

AAiBotController::~AAiBotController() = default;

void AAiBotController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BotPawn = Cast<AAiBotPawn>(InPawn);
}

void AAiBotController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!BotPawn || BotPawn->bFinished)
		return;

	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM || GM->RaceState != ERaceState::Racing)
		return;

	const TArray<ACheckpointActor*>& Checkpoints = GM->GetCheckpoints();
	if (Checkpoints.Num() == 0)
		return;

	// 1) Пытаемся управлять обученной моделью
	bool bUsedNN = false;
	if (bUseNeuralNetwork)
	{
		if (!bTriedInit)
		{
			bModelReady = InitNeuralNetwork();
			bTriedInit = true;
		}
		if (bModelReady)
		{
			bUsedNN = RunNeuralControl(DeltaTime);
		}
	}

	// 2) Если модель недоступна/выключена — rule-based (запасной вариант + базовая линия)
	if (!bUsedNN)
	{
		RunRuleBasedControl(DeltaTime);
	}
}

bool AAiBotController::InitNeuralNetwork()
{
	// Загрузка импортированного ONNX-ассета (NNEModelData) по пути
	UNNEModelData* Model = LoadObject<UNNEModelData>(nullptr, *ModelAssetPath);
	if (!Model)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AiBot] NNE: модель не найдена по пути %s. Боты используют rule-based управление."),
			*ModelAssetPath);
		return false;
	}

	// Получаем CPU-рантайм ONNX Runtime (плагин NNERuntimeORT)
	TWeakInterfacePtr<INNERuntimeCPU> Runtime =
		UE::NNE::GetRuntime<INNERuntimeCPU>(FString(TEXT("NNERuntimeORTCpu")));
	if (!Runtime.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AiBot] NNE: рантайм NNERuntimeORTCpu недоступен. Включите плагин NNERuntimeORT. Fallback: rule-based."));
		return false;
	}

	TSharedPtr<UE::NNE::IModelCPU> CPUModel = Runtime->CreateModelCPU(Model);
	if (!CPUModel.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AiBot] NNE: не удалось создать модель из ассета. Fallback: rule-based."));
		return false;
	}

	ModelInstance = CPUModel->CreateModelInstanceCPU();
	if (!ModelInstance.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AiBot] NNE: не удалось создать экземпляр модели. Fallback: rule-based."));
		return false;
	}

	// Фиксированная форма входа: batch=1, наблюдение=12
	TArray<uint32> Shape = { 1u, (uint32)OBS_SIZE };
	TArray<UE::NNE::FTensorShape> InputShapes;
	InputShapes.Add(UE::NNE::FTensorShape::Make(Shape));
	ModelInstance->SetInputTensorShapes(InputShapes);

	InputData.SetNumZeroed(OBS_SIZE);
	OutputData.SetNumZeroed(ACT_SIZE);

	UE_LOG(LogTemp, Log,
		TEXT("[AiBot] NNE: модель загружена. Боты управляются обученной PPO-политикой (%s)."),
		*ModelAssetPath);
	return true;
}

bool AAiBotController::RunNeuralControl(float DeltaTime)
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM)
		return false;

	const TArray<ACheckpointActor*>& Checkpoints = GM->GetCheckpoints();
	int32 CpIdx = BotPawn->NextCheckpointIndex;
	if (CpIdx >= Checkpoints.Num())
		CpIdx = 0;

	ACheckpointActor* TargetCP = Checkpoints[CpIdx];
	if (!TargetCP)
		return false;

	// ---- Сбор наблюдения, ИДЕНТИЧНЫЙ Python/air_racer/env.py::_get_obs() ----
	const FVector Pos   = BotPawn->GetActorLocation();
	const FVector CpPos = TargetCP->GetActorLocation();

	const FVector Rel   = CpPos - Pos;               // вектор до чекпоинта (мир)
	const float   Div   = OBS_TRACK_RADIUS * 2.0f;   // та же нормализация, что при обучении (40000)

	// Вектор к чекпоинту в ЛОКАЛЬНЫХ координатах самолёта
	// InverseTransformDirection: world -> local (= R^T @ world_vec в Python)
	const FVector RelLocal  = BotPawn->GetActorTransform().InverseTransformVectorNoScale(Rel);
	const FVector RelLocalN = RelLocal / Div;

	// World-up в локальных координатах (для стабилизации крена)
	const FVector WorldUp   = FVector::UpVector;
	const FVector UpLocal   = BotPawn->GetActorTransform().InverseTransformVectorNoScale(WorldUp);

	const float SpeedN  = BotPawn->GetCurrentSpeed() / OBS_MAX_SPEED;
	const float DistN   = Rel.Size() / Div;

	// progress = (lap*NUM_CP + next_cp) / (NUM_LAPS*NUM_CP); в env круги 0-based,
	// у бота CurrentLap начинается с 1 -> приводим к 0-based.
	const int32 Lap0    = FMath::Max(0, BotPawn->CurrentLap - 1);
	const float Progress = float(Lap0 * OBS_NUM_CP + CpIdx) / float(OBS_NUM_LAPS * OBS_NUM_CP);

	// Порядок строго как в env.py: rel_local(3), up_local(3), [speed_norm, dist_norm, progress]
	InputData[0] = RelLocalN.X;  InputData[1] = RelLocalN.Y;  InputData[2] = RelLocalN.Z;
	InputData[3] = UpLocal.X;    InputData[4] = UpLocal.Y;    InputData[5] = UpLocal.Z;
	InputData[6] = SpeedN;       InputData[7] = DistN;        InputData[8] = Progress;

	// ---- Инференс ----
	TArray<UE::NNE::FTensorBindingCPU> Inputs;
	Inputs.SetNum(1);
	Inputs[0].Data = InputData.GetData();
	Inputs[0].SizeInBytes = (uint64)InputData.Num() * sizeof(float);

	TArray<UE::NNE::FTensorBindingCPU> Outputs;
	Outputs.SetNum(1);
	Outputs[0].Data = OutputData.GetData();
	Outputs[0].SizeInBytes = (uint64)OutputData.Num() * sizeof(float);

	ModelInstance->RunSync(Inputs, Outputs);

	// ---- Применение действий (прямой маппинг, Python env использует ту же СК, что UE5) ----
	const float Throttle = FMath::Clamp(OutputData[0], -1.0f, 1.0f);
	const float Pitch    = FMath::Clamp(OutputData[1], -1.0f, 1.0f);
	const float Yaw      = FMath::Clamp(OutputData[2], -1.0f, 1.0f);
	const float Roll     = FMath::Clamp(OutputData[3], -1.0f, 1.0f);

	BotPawn->SetThrottleInput(Throttle);
	BotPawn->SetPitchInput(Pitch);
	BotPawn->SetYawInput(Yaw);
	BotPawn->SetRollInput(Roll);

	return true;
}

void AAiBotController::RunRuleBasedControl(float DeltaTime)
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM)
		return;

	const TArray<ACheckpointActor*>& Checkpoints = GM->GetCheckpoints();
	int32 CpIdx = BotPawn->NextCheckpointIndex;
	if (CpIdx >= Checkpoints.Num())
		CpIdx = 0;

	ACheckpointActor* TargetCP = Checkpoints[CpIdx];
	if (!TargetCP)
		return;

	// Направление к чекпоинту в мировых координатах
	FVector ToCheckpoint = TargetCP->GetActorLocation() - BotPawn->GetActorLocation();
	float Distance = ToCheckpoint.Size();

	FVector DesiredDir = ToCheckpoint.GetSafeNormal();
	FVector CurrentFwd = BotPawn->GetActorForwardVector();
	FVector CurrentRight = BotPawn->GetActorRightVector();
	FVector CurrentUp = BotPawn->GetActorUpVector();

	float DotRight = FVector::DotProduct(DesiredDir, CurrentRight);  // >0 = поворот вправо
	float DotUp = FVector::DotProduct(DesiredDir, CurrentUp);        // >0 = задрать нос
	float DotFwd = FVector::DotProduct(DesiredDir, CurrentFwd);      // >0 = на цели

	float YawInput = FMath::Clamp(DotRight * 5.0f, -1.0f, 1.0f);
	float PitchInput = FMath::Clamp(DotUp * 5.0f, -1.0f, 1.0f);
	float RollInput = FMath::Clamp(DotRight * 2.0f, -1.0f, 1.0f);

	float ThrottleInput = 1.0f;

	// Авто-выравнивание крена при малом отклонении
	if (FMath::Abs(DotRight) < 0.2f)
	{
		FVector WorldUp = FVector::UpVector;
		float RollError = FVector::DotProduct(CurrentRight, WorldUp);
		RollInput = FMath::Clamp(-RollError * 3.0f, -1.0f, 1.0f);
	}

	BotPawn->SetThrottleInput(ThrottleInput);
	BotPawn->SetPitchInput(PitchInput);
	BotPawn->SetYawInput(YawInput);
	BotPawn->SetRollInput(RollInput);
}
