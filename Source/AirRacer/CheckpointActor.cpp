#include "CheckpointActor.h"
#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Trigger volume
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.0f, 800.0f, 800.0f));
	TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetHiddenInGame(false);
	TriggerBox->ShapeColor = FColor::Green;
	TriggerBox->SetLineThickness(5.0f);

	// Visual ring mesh
	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(TriggerBox);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RingMesh->SetRelativeScale3D(FVector(8.0f, 8.0f, 8.0f));
}

void ACheckpointActor::BeginPlay()
{
	Super::BeginPlay();
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointActor::OnOverlapBegin);
}

void ACheckpointActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Overlap with: %s"), *OtherActor->GetName());

	AAirplanePawn* Plane = Cast<AAirplanePawn>(OtherActor);
	if (!Plane)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not an airplane, ignoring"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Checkpoint %d reached!"), CheckpointIndex);

	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->OnCheckpointReached(Plane, CheckpointIndex);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode is not AirRaceGameMode!"));
	}
}
