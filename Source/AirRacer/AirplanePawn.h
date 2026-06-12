#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "AirplanePawn.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class AIRRACER_API AAirplanePawn : public APawn
{
	GENERATED_BODY()

public:
	AAirplanePawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PlaneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	// --- Input ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ThrottleAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PitchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* YawAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* RollAction;

	// --- Flight Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MaxSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float Acceleration = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float PitchSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float YawSpeed = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float RollSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MinSpeed = 500.0f;

private:
	// Input handlers
	void HandleThrottle(const FInputActionValue& Value);
	void HandlePitch(const FInputActionValue& Value);
	void HandleYaw(const FInputActionValue& Value);
	void HandleRoll(const FInputActionValue& Value);

	// Current state
	float CurrentSpeed = 0.0f;
	float ThrottleInput = 0.0f;
	float PitchInput = 0.0f;
	float YawInput = 0.0f;
	float RollInput = 0.0f;
};
