#include "CheckpointActor.h"
#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "AiBotPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Trigger volume
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.0f, 800.0f, 800.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetHiddenInGame(true);

	// Blueprint visual mesh (will be hidden at runtime)
	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(TriggerBox);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACheckpointActor::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointActor::OnOverlapBegin);

	// Hide ALL existing mesh components (Blueprint may have saved extras)
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Comp : MeshComps)
	{
		Comp->SetVisibility(false);
	}

	// Create gate ring from spheres dynamically
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh)
	{
		const int32 NumSpheres = 12;
		const float RingRadius = 600.0f;
		FLinearColor InactiveColor(0.4f, 0.4f, 0.45f); // серый

		for (int32 i = 0; i < NumSpheres; i++)
		{
			float Angle = (2.0f * PI * i) / NumSpheres;
			FVector Offset(0.0f, FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius);

			FString Name = FString::Printf(TEXT("GateSphere_%d"), i);
			UStaticMeshComponent* SphereComp = NewObject<UStaticMeshComponent>(this, *Name);
			SphereComp->SetupAttachment(TriggerBox);
			SphereComp->SetStaticMesh(SphereMesh);
			SphereComp->SetRelativeLocation(Offset);
			SphereComp->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
			SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			if (BaseMat)
			{
				UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(BaseMat, this);
				Mat->SetVectorParameterValue(TEXT("Color"), InactiveColor);
				SphereComp->SetMaterial(0, Mat);
			}

			SphereComp->RegisterComponent();
			GateSpheres.Add(SphereComp);
		}
	}

	// Create highlight lights dynamically (bright, visible from far)
	HighlightLight = NewObject<UPointLightComponent>(this, TEXT("HighlightLight"));
	HighlightLight->SetupAttachment(TriggerBox);
	HighlightLight->SetRelativeLocation(FVector::ZeroVector);
	HighlightLight->SetIntensity(500000.0f);
	HighlightLight->SetAttenuationRadius(5000.0f);
	HighlightLight->SetLightColor(FLinearColor(0.0f, 1.0f, 0.2f));
	HighlightLight->SetVisibility(false);
	HighlightLight->RegisterComponent();
}

void ACheckpointActor::SetHighlight(bool bActive)
{
	if (HighlightLight)
	{
		HighlightLight->SetVisibility(bActive);
	}

	FLinearColor Color = bActive
		? FLinearColor(0.0f, 1.0f, 0.2f)   // зелёный
		: FLinearColor(0.4f, 0.4f, 0.45f);  // серый

	for (UStaticMeshComponent* Sphere : GateSpheres)
	{
		if (Sphere)
		{
			UMaterialInstanceDynamic* Mat = Cast<UMaterialInstanceDynamic>(Sphere->GetMaterial(0));
			if (Mat)
			{
				Mat->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	}
}

void ACheckpointActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM)
		return;

	// Player airplane
	AAirplanePawn* Plane = Cast<AAirplanePawn>(OtherActor);
	if (Plane)
	{
		UE_LOG(LogTemp, Warning, TEXT("Checkpoint %d reached by player!"), CheckpointIndex);
		GM->OnCheckpointReached(Plane, CheckpointIndex);
		return;
	}

	// AI bot
	AAiBotPawn* Bot = Cast<AAiBotPawn>(OtherActor);
	if (Bot)
	{
		GM->OnBotCheckpointReached(Bot, CheckpointIndex);
		return;
	}
}
