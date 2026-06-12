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

protected:
	virtual void BeginPlay() override;

private:
	void DrawBackground(float X, float Y, float Width, float Height, float Opacity = 0.4f);
	void DrawRaceHUD();
	void DrawMainMenu();
	void DrawPauseMenu();
	void DrawFinishScreen();

	bool DrawButton(const FString& Text, float X, float Y, float W, float H, UFont* Font);
	bool IsMouseInRect(float X, float Y, float W, float H) const;
	bool bMousePressed = false;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
};
