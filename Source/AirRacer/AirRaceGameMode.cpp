#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "RaceHUD.h"
#include "CheckpointActor.h"
#include "GroundPlane.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

AAirRaceGameMode::AAirRaceGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	// Do NOT set DefaultPawnClass — BP_Airplane is placed in the level
	// with Auto Possess Player 0, setting this spawns a duplicate pawn
	DefaultPawnClass = nullptr;
	HUDClass = ARaceHUD::StaticClass();
}

void AAirRaceGameMode::BeginPlay()
{
	Super::BeginPlay();
	CurrentLap = 1;
	NextCheckpointIndex = 0;
	RaceTime = 0.0f;
	LapStartTime = 0.0f;
	bRaceFinished = false;
	BestLapTime = 0.0f;

	CollectCheckpoints();
	SpawnGroundPlane();

	// Delay highlight update by 1 frame so checkpoints finish their BeginPlay first
	GetWorldTimerManager().SetTimerForNextTick(this, &AAirRaceGameMode::UpdateCheckpointHighlights);
}

void AAirRaceGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bRaceFinished)
	{
		RaceTime += DeltaTime;
	}
}

void AAirRaceGameMode::OnCheckpointReached(AAirplanePawn* Plane, int32 CheckpointIndex)
{
	if (bRaceFinished)
		return;

	if (CheckpointIndex != NextCheckpointIndex)
		return;

	NextCheckpointIndex++;

	if (NextCheckpointIndex >= TotalCheckpoints)
	{
		// Lap completed
		float LapTime = RaceTime - LapStartTime;
		if (BestLapTime <= 0.0f || LapTime < BestLapTime)
		{
			BestLapTime = LapTime;
		}

		UE_LOG(LogTemp, Warning, TEXT("Lap %d complete! Time: %.2f s"), CurrentLap, LapTime);

		CurrentLap++;
		NextCheckpointIndex = 0;
		LapStartTime = RaceTime;

		if (CurrentLap > TotalLaps)
		{
			bRaceFinished = true;
			UE_LOG(LogTemp, Warning, TEXT("Race finished! Total time: %.2f s, Best lap: %.2f s"), RaceTime, BestLapTime);
		}
	}

	UpdateCheckpointHighlights();
}

void AAirRaceGameMode::CollectCheckpoints()
{
	Checkpoints.Empty();
	for (TActorIterator<ACheckpointActor> It(GetWorld()); It; ++It)
	{
		Checkpoints.Add(*It);
	}

	// Sort by CheckpointIndex
	Checkpoints.Sort([](const ACheckpointActor& A, const ACheckpointActor& B)
	{
		return A.CheckpointIndex < B.CheckpointIndex;
	});

	UE_LOG(LogTemp, Warning, TEXT("Collected %d checkpoints"), Checkpoints.Num());
	for (ACheckpointActor* CP : Checkpoints)
	{
		if (CP)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Checkpoint %d at %s"), CP->CheckpointIndex, *CP->GetActorLocation().ToString());
		}
	}
}

void AAirRaceGameMode::UpdateCheckpointHighlights()
{
	for (ACheckpointActor* CP : Checkpoints)
	{
		if (CP)
		{
			CP->SetHighlight(CP->CheckpointIndex == NextCheckpointIndex);
		}
	}
}

void AAirRaceGameMode::SpawnGroundPlane()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	FVector Location(0.0f, 0.0f, 0.0f);
	FRotator Rotation = FRotator::ZeroRotator;

	AGroundPlane* Ground = World->SpawnActor<AGroundPlane>(Location, Rotation);
	if (Ground)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ground plane spawned at %s, Scale=%s"),
			*Ground->GetActorLocation().ToString(), *Ground->GetActorScale3D().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn GroundPlane!"));
	}
}
