#include "AirplanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AAirplanePawn::AAirplanePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root mesh (collision + overlap, visual hidden at runtime)
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
	SetRootComponent(PlaneMesh);
	PlaneMesh->SetSimulatePhysics(false);
	PlaneMesh->SetCollisionProfileName(TEXT("Pawn"));
	PlaneMesh->SetGenerateOverlapEvents(true);

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
}

void AAirplanePawn::BeginPlay()
{
	Super::BeginPlay();

	// Register input mapping context
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	CurrentSpeed = MinSpeed;

	// Capture mouse for flight control
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	UE_LOG(LogTemp, Warning, TEXT("AirplanePawn: Start Location=%s"), *GetActorLocation().ToString());

	// Hide the Blueprint's cube mesh (it's still root for physics)
	PlaneMesh->SetVisibility(false);

	// Create visual cone at runtime (Blueprint can't override this)
	UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh)
	{
		UStaticMeshComponent* VisualMesh = NewObject<UStaticMeshComponent>(this, TEXT("VisualMesh"));
		VisualMesh->SetupAttachment(PlaneMesh);
		VisualMesh->SetStaticMesh(ConeMesh);
		// Cone points +Z by default, rotate so tip points +X (forward)
		VisualMesh->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		VisualMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 2.0f));
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualMesh->RegisterComponent();
	}

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
