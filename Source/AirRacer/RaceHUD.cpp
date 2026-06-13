#include "RaceHUD.h"
#include "Engine/Canvas.h"
#include "AirRaceGameMode.h"
#include "AirplanePawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

void ARaceHUD::BeginPlay()
{
	Super::BeginPlay();
	bShowHUD = true;
}

void ARaceHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
		return;

	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (!GM)
		return;

	// Get mouse state directly from FViewport (bypasses PlayerInput, works in all build configs)
	APlayerController* PC = GetOwningPlayerController();
	bMousePressed = false;
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		FViewport* Viewport = GEngine->GameViewport->Viewport;
		MouseX = Viewport->GetMouseX();
		MouseY = Viewport->GetMouseY();
		bool bMouseDown = Viewport->KeyState(EKeys::LeftMouseButton);
		bMousePressed = bMouseDown && !bWasMouseDown;
		bWasMouseDown = bMouseDown;
	}

	switch (GM->RaceState)
	{
	case ERaceState::MainMenu:
		DrawMainMenu();
		break;
	case ERaceState::Racing:
		DrawRaceHUD();
		break;
	case ERaceState::Paused:
		DrawRaceHUD();
		DrawPauseMenu();
		break;
	case ERaceState::Finished:
		DrawRaceHUD();
		DrawFinishScreen();
		break;
	}
}

void ARaceHUD::DrawMainMenu()
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;
	UFont* Font = GEngine->GetLargeFont();

	// Dark background
	DrawBackground(0, 0, ScreenW, ScreenH, 0.7f);

	// Title
	FString Title = TEXT("AIR RACER");
	float TW, TH;
	Canvas->StrLen(Font, Title, TW, TH);
	float Scale = 3.0f;

	FCanvasTextItem TitleItem(
		FVector2D((ScreenW - TW * Scale) * 0.5f, ScreenH * 0.2f),
		FText::FromString(Title), Font, FLinearColor(1.0f, 0.9f, 0.1f));
	TitleItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(TitleItem);

	// Buttons
	float BtnW = 250.0f, BtnH = 55.0f;
	float BtnX = (ScreenW - BtnW) * 0.5f;

	bool bStartClicked = DrawButton(TEXT("PLAY"), BtnX, ScreenH * 0.5f, BtnW, BtnH, Font);

	// Also allow Enter/Space to start
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
	{
		FViewport* VP = GEngine->GameViewport->Viewport;
		if (VP->KeyState(EKeys::Enter) || VP->KeyState(EKeys::SpaceBar))
			bStartClicked = true;
	}

	if (bStartClicked)
	{
		if (GM) GM->StartRace();
	}

	if (DrawButton(TEXT("QUIT"), BtnX, ScreenH * 0.5f + 80.0f, BtnW, BtnH, Font))
	{
		if (GM) GM->QuitGame();
	}
}

void ARaceHUD::DrawPauseMenu()
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;
	UFont* Font = GEngine->GetLargeFont();

	DrawBackground(0, 0, ScreenW, ScreenH, 0.5f);

	FString Title = TEXT("PAUSED");
	float TW, TH;
	Canvas->StrLen(Font, Title, TW, TH);
	float Scale = 2.0f;

	FCanvasTextItem TitleItem(
		FVector2D((ScreenW - TW * Scale) * 0.5f, ScreenH * 0.3f),
		FText::FromString(Title), Font, FLinearColor::White);
	TitleItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(TitleItem);

	float BtnW = 250.0f, BtnH = 55.0f;
	float BtnX = (ScreenW - BtnW) * 0.5f;

	if (DrawButton(TEXT("RESUME"), BtnX, ScreenH * 0.5f, BtnW, BtnH, Font))
	{
		if (GM) GM->TogglePause();
	}

	if (DrawButton(TEXT("QUIT"), BtnX, ScreenH * 0.5f + 80.0f, BtnW, BtnH, Font))
	{
		if (GM) GM->QuitGame();
	}
}

void ARaceHUD::DrawFinishScreen()
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;
	UFont* Font = GEngine->GetLargeFont();
	UFont* SmallFont = GEngine->GetSmallFont();

	DrawBackground(0, 0, ScreenW, ScreenH, 0.6f);

	// Title
	FString Title = TEXT("RACE FINISHED!");
	float TW, TH;
	Canvas->StrLen(Font, Title, TW, TH);
	float Scale = 2.5f;

	FCanvasTextItem TitleItem(
		FVector2D((ScreenW - TW * Scale) * 0.5f, ScreenH * 0.15f),
		FText::FromString(Title), Font, FLinearColor(1.0f, 0.9f, 0.1f));
	TitleItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(TitleItem);

	// Results
	auto FormatTime = [](float Time) -> FString
	{
		int32 Min = FMath::FloorToInt(Time / 60.0f);
		int32 Sec = FMath::FloorToInt(FMath::Fmod(Time, 60.0f));
		int32 Ms = FMath::FloorToInt(FMath::Fmod(Time * 100.0f, 100.0f));
		return FString::Printf(TEXT("%02d:%02d.%02d"), Min, Sec, Ms);
	};

	float InfoScale = 1.5f;

	FString TotalText = FString::Printf(TEXT("Total Time:  %s"), *FormatTime(GM->RaceTime));
	float TTW, TTH;
	Canvas->StrLen(Font, TotalText, TTW, TTH);
	FCanvasTextItem TotalItem(
		FVector2D((ScreenW - TTW * InfoScale) * 0.5f, ScreenH * 0.4f),
		FText::FromString(TotalText), Font, FLinearColor::White);
	TotalItem.Scale = FVector2D(InfoScale, InfoScale);
	Canvas->DrawItem(TotalItem);

	FString BestText = FString::Printf(TEXT("Best Lap:  %s"), *FormatTime(GM->BestLapTime));
	float BTW, BTH;
	Canvas->StrLen(Font, BestText, BTW, BTH);
	FCanvasTextItem BestItem(
		FVector2D((ScreenW - BTW * InfoScale) * 0.5f, ScreenH * 0.4f + TTH * InfoScale + 15.0f),
		FText::FromString(BestText), Font, FLinearColor(0.2f, 1.0f, 0.3f));
	BestItem.Scale = FVector2D(InfoScale, InfoScale);
	Canvas->DrawItem(BestItem);

	// Buttons
	float BtnW = 250.0f, BtnH = 55.0f;
	float BtnX = (ScreenW - BtnW) * 0.5f;

	if (DrawButton(TEXT("RESTART"), BtnX, ScreenH * 0.7f, BtnW, BtnH, Font))
	{
		if (GM) GM->RestartRace();
	}

	if (DrawButton(TEXT("QUIT"), BtnX, ScreenH * 0.7f + 80.0f, BtnW, BtnH, Font))
	{
		if (GM) GM->QuitGame();
	}
}

bool ARaceHUD::DrawButton(const FString& Text, float X, float Y, float W, float H, UFont* Font)
{
	bool bHovered = IsMouseInRect(X, Y, W, H);
	bool bClicked = bHovered && bMousePressed;

	FLinearColor BgColor = bHovered
		? FLinearColor(0.3f, 0.3f, 0.4f, 0.9f)
		: FLinearColor(0.15f, 0.15f, 0.2f, 0.8f);

	FCanvasTileItem Bg(FVector2D(X, Y), FVector2D(W, H), BgColor);
	Bg.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Bg);

	// Border
	float Border = 2.0f;
	FLinearColor BorderColor = bHovered
		? FLinearColor(1.0f, 0.9f, 0.1f, 1.0f)
		: FLinearColor(0.5f, 0.5f, 0.5f, 0.8f);

	FCanvasTileItem Top(FVector2D(X, Y), FVector2D(W, Border), BorderColor);
	Top.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Top);
	FCanvasTileItem Bottom(FVector2D(X, Y + H - Border), FVector2D(W, Border), BorderColor);
	Bottom.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Bottom);
	FCanvasTileItem Left(FVector2D(X, Y), FVector2D(Border, H), BorderColor);
	Left.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Left);
	FCanvasTileItem Right(FVector2D(X + W - Border, Y), FVector2D(Border, H), BorderColor);
	Right.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Right);

	// Text centered
	float TextW, TextH;
	Canvas->StrLen(Font, Text, TextW, TextH);
	FCanvasTextItem TextItem(
		FVector2D(X + (W - TextW) * 0.5f, Y + (H - TextH) * 0.5f),
		FText::FromString(Text), Font, FLinearColor::White);
	Canvas->DrawItem(TextItem);

	return bClicked;
}

bool ARaceHUD::IsMouseInRect(float X, float Y, float W, float H) const
{
	return MouseX >= X && MouseX <= X + W && MouseY >= Y && MouseY <= Y + H;
}

void ARaceHUD::DrawRaceHUD()
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	AAirplanePawn* Plane = Cast<AAirplanePawn>(GetOwningPawn());
	if (!GM || !Plane)
		return;

	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;
	UFont* Font = GEngine->GetLargeFont();
	UFont* SmallFont = GEngine->GetSmallFont();

	// Top-left: Lap & Checkpoint
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

	// Top-center: Race Timer
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

	// Top-right: Best Lap
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

	// Left side: Race Position + Standings
	{
		int32 Position = GM->GetPlayerPosition();
		FString PosText = FString::Printf(TEXT("%d"), Position);
		FString PosLabel;
		switch (Position)
		{
		case 1: PosLabel = TEXT("st"); break;
		case 2: PosLabel = TEXT("nd"); break;
		case 3: PosLabel = TEXT("rd"); break;
		default: PosLabel = TEXT("th"); break;
		}

		float PanelX = 20.0f;
		float PanelY = 110.0f;

		DrawBackground(PanelX, PanelY, 200.0f, 140.0f);

		// Big position number
		float PosW, PosH;
		Canvas->StrLen(Font, PosText, PosW, PosH);
		float PosScale = 2.0f;
		FCanvasTextItem PosItem(
			FVector2D(PanelX + 15.0f, PanelY + 5.0f),
			FText::FromString(PosText + PosLabel), Font,
			Position == 1 ? FLinearColor(1.0f, 0.9f, 0.1f) : FLinearColor::White);
		PosItem.Scale = FVector2D(PosScale, PosScale);
		Canvas->DrawItem(PosItem);

		// Standings list
		TArray<AAirRaceGameMode::FRacerInfo> Standings = GM->GetRaceStandings();
		float ListY = PanelY + 55.0f;
		for (int32 i = 0; i < Standings.Num() && i < 4; i++)
		{
			FString EntryText = FString::Printf(TEXT("%d. %s"), i + 1, *Standings[i].Name);
			FColor EntryColor = Standings[i].Name == TEXT("Player") ? FColor::Yellow : FColor(180, 180, 180);
			Canvas->DrawColor = EntryColor;
			Canvas->DrawText(SmallFont, EntryText, PanelX + 15.0f, ListY);
			ListY += 20.0f;
		}
	}

	// Bottom-center: Speed
	{
		float Speed = Plane->GetCurrentSpeed();
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
}

void ARaceHUD::DrawBackground(float X, float Y, float Width, float Height, float Opacity)
{
	FCanvasTileItem TileItem(
		FVector2D(X, Y),
		FVector2D(Width, Height),
		FLinearColor(0.0f, 0.0f, 0.0f, Opacity)
	);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}
