#include "AirplanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AirRaceGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "InputMappingContext.h"
#include "InputAction.h"

AAirplanePawn::AAirplanePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root mesh (collision + overlap, visual hidden at runtime)
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
	SetRootComponent(PlaneMesh);
	PlaneMesh->SetSimulatePhysics(false);
	PlaneMesh->SetCollisionProfileName(TEXT("Pawn"));
	PlaneMesh->SetGenerateOverlapEvents(true);

	// Assign a cube mesh so the collision component has geometry for overlaps
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		PlaneMesh->SetStaticMesh(CubeFinder.Object);
	}

	// Spring arm (camera boom)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(PlaneMesh);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;

	// Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Load input assets from C++ so they work in packaged builds (no BP dependency)
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCFinder(
		TEXT("/Game/Input/IMC_Flight.IMC_Flight"));
	static ConstructorHelpers::FObjectFinder<UInputAction> ThrottleFinder(
		TEXT("/Game/Input/IA_Throttle.IA_Throttle"));
	static ConstructorHelpers::FObjectFinder<UInputAction> PitchFinder(
		TEXT("/Game/Input/IA_Pitch.IA_Pitch"));
	static ConstructorHelpers::FObjectFinder<UInputAction> YawFinder(
		TEXT("/Game/Input/IA_Yaw.IA_Yaw"));
	static ConstructorHelpers::FObjectFinder<UInputAction> RollFinder(
		TEXT("/Game/Input/IA_Roll.IA_Roll"));
	static ConstructorHelpers::FObjectFinder<UInputAction> PauseFinder(
		TEXT("/Game/Input/IA_Pause.IA_Pause"));

	if (IMCFinder.Succeeded()) DefaultMappingContext = IMCFinder.Object;
	if (ThrottleFinder.Succeeded()) ThrottleAction = ThrottleFinder.Object;
	if (PitchFinder.Succeeded()) PitchAction = PitchFinder.Object;
	if (YawFinder.Succeeded()) YawAction = YawFinder.Object;
	if (RollFinder.Succeeded()) RollAction = RollFinder.Object;
	if (PauseFinder.Succeeded()) PauseAction = PauseFinder.Object;
}

void AAirplanePawn::BeginPlay()
{
	Super::BeginPlay();

	// Try to register input mapping context (may fail if not possessed yet)
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("AirplanePawn: MappingContext added in BeginPlay"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AirplanePawn: DefaultMappingContext is NULL!"));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AirplanePawn: No Controller in BeginPlay, will add MappingContext in PossessedBy"));
	}

	CurrentSpeed = MinSpeed;

	// Hide the Blueprint's cube mesh (it's still root for physics)
	PlaneMesh->SetVisibility(false);

	// Build airplane from primitives at runtime
	BuildAirplaneVisual();

	// Force camera setup (override Blueprint-saved values)
	if (SpringArm)
	{
		SpringArm->TargetArmLength = 600.0f;
		SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
		SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	}
}

void AAirplanePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only move when racing
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (GM && GM->RaceState != ERaceState::Racing)
		return;

	// Update speed
	CurrentSpeed += ThrottleInput * Acceleration * DeltaTime;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, MinSpeed, MaxSpeed);

	// Read mouse delta for pitch/yaw
	float MouseX = 0.0f, MouseY = 0.0f;
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->GetInputMouseDelta(MouseX, MouseY);
	}

	// Apply rotation (keyboard + mouse combined)
	FRotator DeltaRotation = FRotator::ZeroRotator;
	DeltaRotation.Pitch = (PitchInput * PitchSpeed + MouseY * MousePitchSensitivity) * DeltaTime;
	DeltaRotation.Yaw = (YawInput * YawSpeed + MouseX * MouseYawSensitivity) * DeltaTime;
	DeltaRotation.Roll = RollInput * RollSpeed * DeltaTime;
	AddActorLocalRotation(DeltaRotation);

	// Move forward
	FVector ForwardMovement = GetActorForwardVector() * CurrentSpeed * DeltaTime;
	SetActorLocation(GetActorLocation() + ForwardMovement, true);
}

void AAirplanePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Register input mapping context after possession (in packaged builds, BeginPlay may fire before possession)
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("AirplanePawn: MappingContext added in PossessedBy"));
			}
		}
	}
}

void AAirplanePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ThrottleAction)
		{
			EnhancedInput->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleThrottle);
			EnhancedInput->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AAirplanePawn::ResetThrottle);
		}
		if (PitchAction)
		{
			EnhancedInput->BindAction(PitchAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandlePitch);
			EnhancedInput->BindAction(PitchAction, ETriggerEvent::Completed, this, &AAirplanePawn::ResetPitch);
		}
		if (YawAction)
		{
			EnhancedInput->BindAction(YawAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleYaw);
			EnhancedInput->BindAction(YawAction, ETriggerEvent::Completed, this, &AAirplanePawn::ResetYaw);
		}
		if (RollAction)
		{
			EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleRoll);
			EnhancedInput->BindAction(RollAction, ETriggerEvent::Completed, this, &AAirplanePawn::ResetRoll);
		}
		if (PauseAction)
		{
			EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AAirplanePawn::HandlePause);
		}
	}
}

void AAirplanePawn::HandleThrottle(const FInputActionValue& Value)
{
	ThrottleInput = Value.Get<float>();
}

void AAirplanePawn::HandlePitch(const FInputActionValue& Value)
{
	PitchInput = Value.Get<float>();
}

void AAirplanePawn::HandleYaw(const FInputActionValue& Value)
{
	YawInput = Value.Get<float>();
}

void AAirplanePawn::HandleRoll(const FInputActionValue& Value)
{
	RollInput = Value.Get<float>();
}

void AAirplanePawn::ResetThrottle(const FInputActionValue& Value)
{
	ThrottleInput = 0.0f;
}

void AAirplanePawn::ResetPitch(const FInputActionValue& Value)
{
	PitchInput = 0.0f;
}

void AAirplanePawn::ResetYaw(const FInputActionValue& Value)
{
	YawInput = 0.0f;
}

void AAirplanePawn::ResetRoll(const FInputActionValue& Value)
{
	RollInput = 0.0f;
}

void AAirplanePawn::HandlePause(const FInputActionValue& Value)
{
	AAirRaceGameMode* GM = Cast<AAirRaceGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->TogglePause();
	}
}

UStaticMeshComponent* AAirplanePawn::CreatePart(const FName& Name, UStaticMesh* Mesh,
	const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color)
{
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(this, Name);
	Comp->SetupAttachment(PlaneMesh);
	Comp->SetStaticMesh(Mesh);
	Comp->SetRelativeLocation(Location);
	Comp->SetRelativeRotation(Rotation);
	Comp->SetRelativeScale3D(Scale);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMat)
	{
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(BaseMat, this);
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
		Comp->SetMaterial(0, Mat);
	}

	Comp->RegisterComponent();
	return Comp;
}

void AAirplanePawn::BuildAirplaneVisual()
{
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	if (!Cylinder || !Cube || !Cone)
		return;

	FLinearColor BodyColor(0.1f, 0.3f, 0.8f);    // синий
	FLinearColor WingColor(0.6f, 0.6f, 0.65f);    // серый
	FLinearColor TailColor(0.8f, 0.1f, 0.1f);     // красный
	FLinearColor NoseColor(0.9f, 0.9f, 0.2f);     // жёлтый

	// Фюзеляж — цилиндр, лежит вдоль X
	CreatePart(TEXT("Fuselage"), Cylinder,
		FVector(0, 0, 0), FRotator(0, 0, 90), FVector(0.5f, 0.5f, 1.5f), BodyColor);

	// Нос — конус, направлен вперёд (+X)
	CreatePart(TEXT("Nose"), Cone,
		FVector(170, 0, 0), FRotator(-90, 0, 0), FVector(0.5f, 0.5f, 0.7f), NoseColor);

	// Главные крылья — два плоских куба по бокам
	CreatePart(TEXT("WingLeft"), Cube,
		FVector(0, -120, 0), FRotator(0, 0, 0), FVector(0.6f, 2.0f, 0.04f), WingColor);
	CreatePart(TEXT("WingRight"), Cube,
		FVector(0, 120, 0), FRotator(0, 0, 0), FVector(0.6f, 2.0f, 0.04f), WingColor);

	// Хвостовое оперение — вертикальный стабилизатор
	CreatePart(TEXT("TailFin"), Cube,
		FVector(-140, 0, 40), FRotator(0, 0, 0), FVector(0.4f, 0.04f, 0.6f), TailColor);

	// Горизонтальные стабилизаторы хвоста
	CreatePart(TEXT("TailWingLeft"), Cube,
		FVector(-140, -40, 0), FRotator(0, 0, 0), FVector(0.3f, 0.7f, 0.04f), TailColor);
	CreatePart(TEXT("TailWingRight"), Cube,
		FVector(-140, 40, 0), FRotator(0, 0, 0), FVector(0.3f, 0.7f, 0.04f), TailColor);
}
