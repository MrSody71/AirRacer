#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AiBotController.generated.h"

class AAiBotPawn;
class ACheckpointActor;

UCLASS()
class AIRRACER_API AAiBotController : public AAIController
{
	GENERATED_BODY()

public:
	AAiBotController();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	AAiBotPawn* BotPawn = nullptr;

	// Steering gains
	float PitchGain = 2.5f;
	float YawGain = 3.0f;
	float RollGain = 1.5f;
};
