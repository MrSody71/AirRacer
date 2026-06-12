#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckpointActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class AIRRACER_API ACheckpointActor : public AActor
{
	GENERATED_BODY()

public:
	ACheckpointActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	int32 CheckpointIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* RingMesh;

	/** Highlight this checkpoint as the next target */
	void SetHighlight(bool bActive);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY()
	UPointLightComponent* HighlightLight = nullptr;
};
