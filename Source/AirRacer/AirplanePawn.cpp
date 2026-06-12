#include "AirplanePawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AAirplanePawn::AAirplanePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root mesh
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
	SetRootComponent(PlaneMesh);
	PlaneMesh->SetSimulatePhysics(false);
	PlaneMesh->SetCollisionProfileName(TEXT("Pawn"));

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
}

void AAirplanePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update speed
	CurrentSpeed += ThrottleInput * Acceleration * DeltaTime;
	CurrentSpeed = FMath::Clamp(CurrentSpeed, MinSpeed, MaxSpeed);

	// Apply rotation
	FRotator DeltaRotation = FRotator::ZeroRotator;
	DeltaRotation.Pitch = PitchInput * PitchSpeed * DeltaTime;
	DeltaRotation.Yaw = YawInput * YawSpeed * DeltaTime;
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
			EnhancedInput->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleThrottle);
		if (PitchAction)
			EnhancedInput->BindAction(PitchAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandlePitch);
		if (YawAction)
			EnhancedInput->BindAction(YawAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleYaw);
		if (RollAction)
			EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &AAirplanePawn::HandleRoll);
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
