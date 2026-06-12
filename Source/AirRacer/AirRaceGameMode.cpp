#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "RaceHUD.h"
#include "CheckpointActor.h"
#include "GroundPlane.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AAirRaceGameMode::AAirRaceGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
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
	RaceState = ERaceState::MainMenu;

	CollectCheckpoints();
	SpawnGroundPlane();

	GetWorldTimerManager().SetTimerForNextTick(this, &AAirRaceGameMode::UpdateCheckpointHighlights);

	// Show main menu: pause game, show cursor
	UGameplayStatics::SetGamePaused(GetWorld(), true);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

void AAirRaceGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (RaceState == ERaceState::Racing)
	{
		RaceTime += DeltaTime;
	}
}

void AAirRaceGameMode::StartRace()
{
	RaceState = ERaceState::Racing;
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AAirRaceGameMode::TogglePause()
{
	if (RaceState == ERaceState::Racing)
	{
		RaceState = ERaceState::Paused;
		UGameplayStatics::SetGamePaused(GetWorld(), true);
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeGameAndUI());
		}
	}
	else if (RaceState == ERaceState::Paused)
	{
		RaceState = ERaceState::Racing;
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

void AAirRaceGameMode::RestartRace()
{
	UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
}

void AAirRaceGameMode::QuitGame()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
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
		float LapTime = RaceTime - LapStartTime;
		if (BestLapTime <= 0.0f || LapTime < BestLapTime)
		{
			BestLapTime = LapTime;
		}

		CurrentLap++;
		NextCheckpointIndex = 0;
		LapStartTime = RaceTime;

		if (CurrentLap > TotalLaps)
		{
			bRaceFinished = true;
			RaceState = ERaceState::Finished;

			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC)
			{
				PC->bShowMouseCursor = true;
				PC->SetInputMode(FInputModeGameAndUI());
			}
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

	Checkpoints.Sort([](const ACheckpointActor& A, const ACheckpointActor& B)
	{
		return A.CheckpointIndex < B.CheckpointIndex;
	});
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
	World->SpawnActor<AGroundPlane>(Location, Rotation);
}
