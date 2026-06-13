#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AiBotPawn.generated.h"

class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class AIRRACER_API AAiBotPawn : public APawn
{
	GENERATED_BODY()

public:
	AAiBotPawn();

	virtual void Tick(float DeltaTime) override;

	// AI control inputs (set by controller each tick)
	void SetThrottleInput(float Val) { ThrottleInput = Val; }
	void SetPitchInput(float Val) { PitchInput = Val; }
	void SetYawInput(float Val) { YawInput = Val; }
	void SetRollInput(float Val) { RollInput = Val; }

	float GetCurrentSpeed() const { return CurrentSpeed; }

	// Per-bot race state
	int32 NextCheckpointIndex = 0;
	int32 CurrentLap = 1;
	bool bFinished = false;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BotMesh;

	// Flight params (same as AirplanePawn)
	UPROPERTY()
	float MinSpeed = 500.0f;
	UPROPERTY()
	float MaxSpeed = 3000.0f;
	float Acceleration = 1500.0f;

public:
	void SetSpeedLimits(float InMin, float InMax) { MinSpeed = InMin; MaxSpeed = InMax; }
	FString BotName;
	float PitchSpeed = 90.0f;
	float YawSpeed = 60.0f;
	float RollSpeed = 120.0f;

	float CurrentSpeed = 500.0f;
	float ThrottleInput = 0.0f;
	float PitchInput = 0.0f;
	float YawInput = 0.0f;
	float RollInput = 0.0f;

	void BuildBotVisual();
	UStaticMeshComponent* CreatePart(const FName& Name, UStaticMesh* Mesh,
		const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color);
};
