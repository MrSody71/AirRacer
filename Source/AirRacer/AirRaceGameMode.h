#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AirRaceGameMode.generated.h"

class AAirplanePawn;
class ACheckpointActor;
class AAiBotPawn;

UENUM()
enum class ERaceState : uint8
{
	MainMenu,
	Racing,
	Paused,
	Finished
};

UCLASS()
class AIRRACER_API AAirRaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAirRaceGameMode();

	void OnCheckpointReached(AAirplanePawn* Plane, int32 CheckpointIndex);
	void OnBotCheckpointReached(AAiBotPawn* Bot, int32 CheckpointIndex);

	const TArray<ACheckpointActor*>& GetCheckpoints() const { return Checkpoints; }

	// Race position tracking
	int32 GetPlayerPosition() const;

	struct FRacerInfo
	{
		FString Name;
		int32 Lap;
		int32 Checkpoint;
		float DistToNext;
	};
	TArray<FRacerInfo> GetRaceStandings() const;

	UPROPERTY()
	TArray<AAiBotPawn*> AIBots;

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

	ERaceState RaceState = ERaceState::MainMenu;

	void StartRace();
	void TogglePause();
	void RestartRace();
	void QuitGame();

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
	void HideBrokenLandscape();
	void HideLandscapesNow();
	void SpawnWaterPlane();
	void SpawnTrees();
	void SpawnAIBots();
	void SpawnCheckpoints();

	bool bHidingLandscapes = false;
	float HideLandscapeTimer = 0.0f;
};
