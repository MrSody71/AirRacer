#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RaceHUD.generated.h"

UCLASS()
class AIRRACER_API ARaceHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBackground(float X, float Y, float Width, float Height, float Opacity = 0.4f);
};
