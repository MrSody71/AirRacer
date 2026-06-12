#include "GroundPlane.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AGroundPlane::AGroundPlane()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	SetRootComponent(MeshComp);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCastShadow(false);

	UE_LOG(LogTemp, Warning, TEXT("GroundPlane: Constructor called"));
}

void AGroundPlane::BeginPlay()
{
	Super::BeginPlay();

	// Use Cube instead of Plane — Plane has backface culling and is invisible from many angles
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (CubeMesh)
	{
		MeshComp->SetStaticMesh(CubeMesh);
		// Flat cube: 500x500 wide, very thin (0.01 height)
		SetActorScale3D(FVector(500.0f, 500.0f, 0.01f));

		// Green ground material
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(
			MeshComp->GetMaterial(0), this);
		Mat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.15f, 0.45f, 0.1f));
		MeshComp->SetMaterial(0, Mat);

		UE_LOG(LogTemp, Warning, TEXT("GroundPlane: Mesh set OK, Location=%s Scale=%s"),
			*GetActorLocation().ToString(), *GetActorScale3D().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GroundPlane: FAILED to load Plane mesh!"));
	}
}
