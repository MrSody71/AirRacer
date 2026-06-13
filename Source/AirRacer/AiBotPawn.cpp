#include "AiBotPawn.h"
#include "AirRaceGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AAiBotPawn::AAiBotPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	BotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BotMesh"));
	SetRootComponent(BotMesh);
	BotMesh->SetSimulatePhysics(false);
	BotMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BotMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	BotMesh->SetGenerateOverlapEvents(true);
}

void AAiBotPawn::BeginPlay()
{
	Super::BeginPlay();
	CurrentSpeed = MinSpeed;

	// Set a cube mesh on root for overlap detection (invisible but has collision geometry)
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh)
	{
		BotMesh->SetStaticMesh(CubeMesh);
		BotMesh->SetWorldScale3D(FVector(1.5f, 1.5f, 1.5f));
	}
	BotMesh->SetVisibility(false);

	BuildBotVisual();
}

void AAiBotPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only move when racing
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (GM && GM->RaceState != ERaceState::Racing)
		return;

	if (bFinished)
		return;

	// Update speed (same as AirplanePawn)
	CurrentSpeed += ThrottleInput * Acceleration * DeltaTime;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, MinSpeed, MaxSpeed);

	// Apply rotation
	FRotator DeltaRotation = FRotator::ZeroRotator;
	DeltaRotation.Pitch = PitchInput * PitchSpeed * DeltaTime;
	DeltaRotation.Yaw = YawInput * YawSpeed * DeltaTime;
	DeltaRotation.Roll = RollInput * RollSpeed * DeltaTime;
	AddActorLocalRotation(DeltaRotation);

	// Move forward (no sweep — avoid collision with hidden landscape)
	FVector ForwardMovement = GetActorForwardVector() * CurrentSpeed * DeltaTime;
	SetActorLocation(GetActorLocation() + ForwardMovement, false);
}

UStaticMeshComponent* AAiBotPawn::CreatePart(const FName& Name, UStaticMesh* Mesh,
	const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color)
{
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, Name);
	Comp->SetupAttachment(BotMesh);
	Comp->SetStaticMesh(Mesh);
	Comp->SetRelativeLocation(Location);
	Comp->SetRelativeRotation(Rotation);
	Comp->SetRelativeScale3D(Scale);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat)
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(BaseMat, this);
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
		Comp->SetMaterial(0, Mat);
	}

	Comp->RegisterComponent();
	return Comp;
}

void AAiBotPawn::BuildBotVisual()
{
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	if (!Cylinder || !Cube || !Cone)
		return;

	// Red/orange color scheme to distinguish from player (blue)
	FLinearColor BodyColor(0.8f, 0.2f, 0.1f);     // red
	FLinearColor WingColor(0.7f, 0.7f, 0.3f);      // yellow-gray
	FLinearColor TailColor(0.9f, 0.5f, 0.1f);      // orange
	FLinearColor NoseColor(0.9f, 0.9f, 0.9f);      // white

	// Fuselage
	CreatePart(TEXT("Fuselage"), Cylinder,
		FVector(0, 0, 0), FRotator(0, 0, 90), FVector(0.5f, 0.5f, 1.5f), BodyColor);

	// Nose
	CreatePart(TEXT("Nose"), Cone,
		FVector(170, 0, 0), FRotator(-90, 0, 0), FVector(0.5f, 0.5f, 0.7f), NoseColor);

	// Wings
	CreatePart(TEXT("WingLeft"), Cube,
		FVector(0, -120, 0), FRotator(0, 0, 0), FVector(0.6f, 2.0f, 0.04f), WingColor);
	CreatePart(TEXT("WingRight"), Cube,
		FVector(0, 120, 0), FRotator(0, 0, 0), FVector(0.6f, 2.0f, 0.04f), WingColor);

	// Tail
	CreatePart(TEXT("TailFin"), Cube,
		FVector(-140, 0, 40), FRotator(0, 0, 0), FVector(0.4f, 0.04f, 0.6f), TailColor);
	CreatePart(TEXT("TailWingLeft"), Cube,
		FVector(-140, -40, 0), FRotator(0, 0, 0), FVector(0.3f, 0.7f, 0.04f), TailColor);
	CreatePart(TEXT("TailWingRight"), Cube,
		FVector(-140, 40, 0), FRotator(0, 0, 0), FVector(0.3f, 0.7f, 0.04f), TailColor);
}
