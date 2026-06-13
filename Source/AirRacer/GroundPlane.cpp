#include "GroundPlane.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

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
		// Green island: 600x600, thin
		SetActorScale3D(FVector(600.0f, 600.0f, 0.01f));

		// Green ground material
		UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (BaseMat)
		{
			UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(BaseMat, this);

			// Log available vector parameters for debugging
			TArray<FMaterialParameterInfo> VecParams;
			TArray<FGuid> VecGuids;
			Mat->GetAllVectorParameterInfo(VecParams, VecGuids);
			for (const FMaterialParameterInfo& P : VecParams)
			{
				UE_LOG(LogTemp, Warning, TEXT("GroundPlane: Material has vector param: '%s'"), *P.Name.ToString());
			}

			// Try both common parameter names
			FLinearColor Green(0.15f, 0.45f, 0.1f);
			Mat->SetVectorParameterValue(TEXT("Color"), Green);
			Mat->SetVectorParameterValue(TEXT("BaseColor"), Green);
			Mat->SetVectorParameterValue(TEXT("Base Color"), Green);
			MeshComp->SetMaterial(0, Mat);
			UE_LOG(LogTemp, Warning, TEXT("GroundPlane: Material applied, BaseMat=%s"), *BaseMat->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("GroundPlane: Failed to load BasicShapeMaterial!"));
		}

		UE_LOG(LogTemp, Warning, TEXT("GroundPlane: Mesh set OK, Location=%s Scale=%s"),
			*GetActorLocation().ToString(), *GetActorScale3D().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GroundPlane: FAILED to load Plane mesh!"));
	}
}
