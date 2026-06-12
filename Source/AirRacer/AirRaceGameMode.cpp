#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "Engine/World.h"

AAirRaceGameMode::AAirRaceGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AAirplanePawn::StaticClass();
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
}
