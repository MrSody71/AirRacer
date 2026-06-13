#include "AiBotController.h"
#include "AiBotPawn.h"
#include "AirRaceGameMode.h"
#include "CheckpointActor.h"

AAiBotController::AAiBotController()
{
	PrimaryActorTick.bCanEverTick = true;
}

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

	int32 CpIdx = BotPawn->NextCheckpointIndex;
	if (CpIdx >= Checkpoints.Num())
		CpIdx = 0;

	ACheckpointActor* TargetCP = Checkpoints[CpIdx];
	if (!TargetCP)
		return;

	// Direction to checkpoint in world space
	FVector ToCheckpoint = TargetCP->GetActorLocation() - BotPawn->GetActorLocation();
	float Distance = ToCheckpoint.Size();

	// Desired direction
	FVector DesiredDir = ToCheckpoint.GetSafeNormal();
	FVector CurrentFwd = BotPawn->GetActorForwardVector();
	FVector CurrentRight = BotPawn->GetActorRightVector();
	FVector CurrentUp = BotPawn->GetActorUpVector();

	// Project desired direction onto bot's local axes
	float DotRight = FVector::DotProduct(DesiredDir, CurrentRight);  // positive = turn right
	float DotUp = FVector::DotProduct(DesiredDir, CurrentUp);        // positive = pull up
	float DotFwd = FVector::DotProduct(DesiredDir, CurrentFwd);      // positive = on target

	// Aggressive steering — always full input toward checkpoint
	float YawInput = FMath::Clamp(DotRight * 5.0f, -1.0f, 1.0f);
	float PitchInput = FMath::Clamp(DotUp * 5.0f, -1.0f, 1.0f);
	float RollInput = FMath::Clamp(DotRight * 2.0f, -1.0f, 1.0f);

	// Full throttle always
	float ThrottleInput = 1.0f;

	// Auto-level roll when roughly on target
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
