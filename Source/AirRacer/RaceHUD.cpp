#include "RaceHUD.h"
#include "Engine/Canvas.h"
#include "AirRaceGameMode.h"
#include "AirplanePawn.h"

void ARaceHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
		return;

	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	AAirplanePawn* Plane = Cast<AAirplanePawn>(GetOwningPawn());
	if (!GM || !Plane)
		return;

	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;

	// Use default engine font (robust, always available)
	UFont* Font = GEngine->GetLargeFont();
	UFont* SmallFont = GEngine->GetSmallFont();

	// === Top-left panel: Lap & Checkpoint ===
	{
		const float PanelX = 20.0f;
		const float PanelY = 20.0f;

		DrawBackground(PanelX, PanelY, 260.0f, 70.0f);

		FString LapText = FString::Printf(TEXT("LAP  %d / %d"), GM->CurrentLap, GM->TotalLaps);
		Canvas->DrawColor = FColor::White;
		Canvas->DrawText(Font, LapText, PanelX + 15.0f, PanelY + 10.0f);

		FString CheckpointText = FString::Printf(TEXT("CHECKPOINT  %d / %d"), GM->NextCheckpointIndex, GM->TotalCheckpoints);
		Canvas->DrawColor = FColor(200, 200, 200);
		Canvas->DrawText(SmallFont, CheckpointText, PanelX + 15.0f, PanelY + 42.0f);
	}

	// === Top-center: Race Timer ===
	{
		int32 Minutes = FMath::FloorToInt(GM->RaceTime / 60.0f);
		int32 Seconds = FMath::FloorToInt(FMath::Fmod(GM->RaceTime, 60.0f));
		int32 Millis = FMath::FloorToInt(FMath::Fmod(GM->RaceTime * 100.0f, 100.0f));
		FString TimeText = FString::Printf(TEXT("%02d:%02d.%02d"), Minutes, Seconds, Millis);

		float TextW = 0.0f, TextH = 0.0f;
		Canvas->StrLen(Font, TimeText, TextW, TextH);

		float PanelW = TextW + 40.0f;
		float PanelX = (ScreenW - PanelW) * 0.5f;
		float PanelY = 20.0f;

		DrawBackground(PanelX, PanelY, PanelW, TextH + 20.0f);

		Canvas->DrawColor = FColor::Yellow;
		Canvas->DrawText(Font, TimeText, PanelX + 20.0f, PanelY + 10.0f);
	}

	// === Top-right: Best Lap ===
	if (GM->BestLapTime > 0.0f)
	{
		int32 BMinutes = FMath::FloorToInt(GM->BestLapTime / 60.0f);
		int32 BSeconds = FMath::FloorToInt(FMath::Fmod(GM->BestLapTime, 60.0f));
		int32 BMillis = FMath::FloorToInt(FMath::Fmod(GM->BestLapTime * 100.0f, 100.0f));
		FString BestText = FString::Printf(TEXT("BEST  %02d:%02d.%02d"), BMinutes, BSeconds, BMillis);

		float TextW = 0.0f, TextH = 0.0f;
		Canvas->StrLen(Font, BestText, TextW, TextH);

		float PanelW = TextW + 40.0f;
		float PanelX = ScreenW - PanelW - 20.0f;
		float PanelY = 20.0f;

		DrawBackground(PanelX, PanelY, PanelW, TextH + 20.0f);

		Canvas->DrawColor = FColor::Green;
		Canvas->DrawText(Font, BestText, PanelX + 20.0f, PanelY + 10.0f);
	}

	// === Bottom-center: Speed ===
	{
		float Speed = Plane->GetCurrentSpeed();
		// Convert UU/s to a display value (divide by 100 for ~m/s feel)
		int32 DisplaySpeed = FMath::RoundToInt(Speed / 100.0f);
		FString SpeedText = FString::Printf(TEXT("%d"), DisplaySpeed);
		FString UnitText = TEXT("km/h");

		float SpeedW = 0.0f, SpeedH = 0.0f;
		Canvas->StrLen(Font, SpeedText, SpeedW, SpeedH);
		float UnitW = 0.0f, UnitH = 0.0f;
		Canvas->StrLen(SmallFont, UnitText, UnitW, UnitH);

		float PanelW = FMath::Max(SpeedW, UnitW) + 50.0f;
		float PanelH = SpeedH + UnitH + 20.0f;
		float PanelX = (ScreenW - PanelW) * 0.5f;
		float PanelY = ScreenH - PanelH - 40.0f;

		DrawBackground(PanelX, PanelY, PanelW, PanelH);

		Canvas->DrawColor = FColor::White;
		Canvas->DrawText(Font, SpeedText, PanelX + (PanelW - SpeedW) * 0.5f, PanelY + 8.0f);

		Canvas->DrawColor = FColor(180, 180, 180);
		Canvas->DrawText(SmallFont, UnitText, PanelX + (PanelW - UnitW) * 0.5f, PanelY + SpeedH + 8.0f);
	}

	// === Race Finished overlay ===
	if (GM->bRaceFinished)
	{
		FString FinishText = TEXT("RACE FINISHED!");
		float TextW = 0.0f, TextH = 0.0f;
		Canvas->StrLen(Font, FinishText, TextW, TextH);

		float Scale = 2.0f;
		float ScaledW = TextW * Scale;
		float ScaledH = TextH * Scale;

		DrawBackground((ScreenW - ScaledW - 40.0f) * 0.5f, (ScreenH - ScaledH - 20.0f) * 0.5f, ScaledW + 40.0f, ScaledH + 20.0f, 0.7f);

		Canvas->DrawColor = FColor::Yellow;
		FCanvasTextItem TextItem(
			FVector2D((ScreenW - ScaledW) * 0.5f, (ScreenH - ScaledH) * 0.5f),
			FText::FromString(FinishText),
			Font,
			FLinearColor::Yellow
		);
		TextItem.Scale = FVector2D(Scale, Scale);
		Canvas->DrawItem(TextItem);
	}
}

void ARaceHUD::DrawBackground(float X, float Y, float Width, float Height, float Opacity)
{
	FLinearColor BgColor(0.0f, 0.0f, 0.0f, Opacity);
	Canvas->DrawColor = BgColor.ToFColor(true);

	FCanvasTileItem TileItem(
		FVector2D(X, Y),
		FVector2D(Width, Height),
		FLinearColor(0.0f, 0.0f, 0.0f, Opacity)
	);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}
