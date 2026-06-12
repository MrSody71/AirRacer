#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AirRaceGameMode.generated.h"

class AAirplanePawn;
class ACheckpointActor;

UCLASS()
class AIRRACER_API AAirRaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAirRaceGameMode();

	void OnCheckpointReached(AAirplanePawn* Plane, int32 CheckpointIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
	int32 TotalCheckpoints = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
	int32 TotalLaps = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	int32 CurrentLap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	int32 NextCheckpointIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	float RaceTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	bool bRaceFinished = false;

	UPROPERTY(BlueprintReadOnly, Category = "Race")
	float BestLapTime = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	float LapStartTime = 0.0f;

	UPROPERTY()
	TArray<ACheckpointActor*> Checkpoints;

	void CollectCheckpoints();
	void UpdateCheckpointHighlights();
	void SpawnGroundPlane();
};
