#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GroundPlane.generated.h"

UCLASS()
class AIRRACER_API AGroundPlane : public AActor
{
	GENERATED_BODY()

public:
	AGroundPlane();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

protected:
	virtual void BeginPlay() override;
};
