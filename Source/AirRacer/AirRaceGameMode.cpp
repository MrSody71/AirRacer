#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "RaceHUD.h"
#include "CheckpointActor.h"
#include "GroundPlane.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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
	HideBrokenLandscape();
	SpawnGroundPlane();
	SpawnWaterPlane();
	SpawnTrees();

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

	// Keep hiding landscape chunks that World Partition streams in
	if (bHidingLandscapes)
	{
		HideLandscapesNow();
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

void AAirRaceGameMode::HideBrokenLandscape()
{
	// Hide landscapes every tick for a few seconds — World Partition streams them in after BeginPlay
	bHidingLandscapes = true;
	HideLandscapeTimer = 0.0f;
	HideLandscapesNow();
}

void AAirRaceGameMode::HideLandscapesNow()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	// Hide landscape proxies
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		ALandscapeProxy* Landscape = *It;
		if (Landscape && !Landscape->IsHidden())
		{
			Landscape->SetActorHiddenInGame(true);
			Landscape->SetActorEnableCollision(false);
		}
	}

	// Hide any other Open World template actors (HLOD, clouds, etc.)
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor->IsHidden())
			continue;

		FString Name = Actor->GetName();
		FString ClassName = Actor->GetClass()->GetName();

		// Hide HLOD actors, volumetric clouds, sky atmosphere leftovers
		if (ClassName.Contains(TEXT("HLOD")) ||
			ClassName.Contains(TEXT("VolumetricCloud")) ||
			Name.Contains(TEXT("HLOD")) ||
			Name.Contains(TEXT("Volumetric")))
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
		}
	}
}

void AAirRaceGameMode::SpawnWaterPlane()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
		return;

	// Water slightly below ground level — visible beyond green ground edges
	FVector WaterLocation(0.0f, 0.0f, -5.0f);
	FActorSpawnParameters SpawnParams;
	FRotator WaterRotation = FRotator::ZeroRotator;
	AActor* WaterActor = World->SpawnActor<AActor>(WaterLocation, WaterRotation);
	if (!WaterActor)
		return;

	UStaticMeshComponent* WaterMesh = NewObject<UStaticMeshComponent>(WaterActor, TEXT("WaterMesh"));
	WaterMesh->SetStaticMesh(CubeMesh);
	WaterMesh->SetWorldLocation(WaterLocation);
	WaterMesh->SetWorldScale3D(FVector(3000.0f, 3000.0f, 0.01f)); // Huge flat plane
	WaterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterMesh->SetCastShadow(false);

	// Blue translucent material
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat)
	{
		UMaterialInstanceDynamic* WaterMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		WaterMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.3f, 0.7f));
		WaterMesh->SetMaterial(0, WaterMat);
	}

	WaterMesh->RegisterComponent();
	WaterActor->SetRootComponent(WaterMesh);

	UE_LOG(LogTemp, Warning, TEXT("SpawnWaterPlane: Water spawned at Z=-5"));
}

void AAirRaceGameMode::SpawnTrees()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (!Cylinder || !Cone || !BaseMat)
		return;

	FLinearColor TrunkColor(0.4f, 0.25f, 0.1f);   // brown
	FLinearColor LeafColors[] = {
		FLinearColor(0.1f, 0.55f, 0.1f),   // dark green
		FLinearColor(0.15f, 0.65f, 0.15f),  // medium green
		FLinearColor(0.2f, 0.5f, 0.05f),    // olive green
	};

	// Island radius ~600 scale * 50 (half cube size) = 30000 units. Stay within ~25000.
	const float IslandRadius = 25000.0f;
	const int32 NumTrees = 60;
	FRandomStream Rand(42); // Fixed seed for consistent placement

	for (int32 i = 0; i < NumTrees; i++)
	{
		// Random position on island
		float Angle = Rand.FRandRange(0.0f, 2.0f * PI);
		float Dist = Rand.FRandRange(2000.0f, IslandRadius);
		float X = FMath::Cos(Angle) * Dist;
		float Y = FMath::Sin(Angle) * Dist;

		// Random tree size
		float TreeScale = Rand.FRandRange(0.7f, 1.5f);
		float TrunkHeight = 150.0f * TreeScale;
		float CrownHeight = 250.0f * TreeScale;

		FVector TreeBase(X, Y, 0.0f);

		// Spawn tree actor
		AActor* TreeActor = World->SpawnActor<AActor>(TreeBase, FRotator::ZeroRotator);
		if (!TreeActor)
			continue;

		// Trunk — brown cylinder
		UStaticMeshComponent* Trunk = NewObject<UStaticMeshComponent>(TreeActor, *FString::Printf(TEXT("Trunk_%d"), i));
		Trunk->SetStaticMesh(Cylinder);
		Trunk->SetWorldLocation(FVector(X, Y, TrunkHeight * 0.5f));
		Trunk->SetWorldScale3D(FVector(0.4f * TreeScale, 0.4f * TreeScale, TrunkHeight / 100.0f));
		Trunk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Trunk->SetCastShadow(true);

		UMaterialInstanceDynamic* TrunkMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		TrunkMat->SetVectorParameterValue(TEXT("Color"), TrunkColor);
		Trunk->SetMaterial(0, TrunkMat);
		Trunk->RegisterComponent();
		TreeActor->SetRootComponent(Trunk);

		// Crown — green cone on top of trunk
		UStaticMeshComponent* Crown = NewObject<UStaticMeshComponent>(TreeActor, *FString::Printf(TEXT("Crown_%d"), i));
		Crown->SetStaticMesh(Cone);
		Crown->SetWorldLocation(FVector(X, Y, TrunkHeight + CrownHeight * 0.5f));
		Crown->SetWorldScale3D(FVector(1.5f * TreeScale, 1.5f * TreeScale, CrownHeight / 100.0f));
		Crown->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Crown->SetCastShadow(true);

		FLinearColor LeafColor = LeafColors[i % 3];
		UMaterialInstanceDynamic* LeafMat = UMaterialInstanceDynamic::Create(BaseMat, this);
		LeafMat->SetVectorParameterValue(TEXT("Color"), LeafColor);
		Crown->SetMaterial(0, LeafMat);
		Crown->RegisterComponent();
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnTrees: Spawned %d trees"), NumTrees);
}
