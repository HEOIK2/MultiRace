#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CXPlayerState.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));
	GetMesh()->SetGenerateOverlapEvents(true);

	FireStartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("Fire Start Point"));
	FireStartPoint->SetupAttachment(GetMesh());
	FireStartPoint->SetRelativeLocation(FVector(250.0f, 0.0f, 60.0f));

	static ConstructorHelpers::FObjectFinder<UParticleSystem> HitEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (HitEffect.Succeeded())
	{
		FireHitEffect = HitEffect.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> HitSound(TEXT("/Game/StarterContent/Audio/Explosion_Cue.Explosion_Cue"));
	if (HitSound.Succeeded())
	{
		FireHitSound = HitSound.Object;
	}

	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());
}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);

		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ACXPawn::Fire);
		}
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));

	if (IsLocallyControlled())
	{
		ApplyThrottleInput();
	}
}

void ACXPawn::ClearDrivingInput()
{
	ThrottleInput = 0.0f;
	ChaosVehicleMovement->SetThrottleInput(0.0f);
	ChaosVehicleMovement->SetSteeringInput(0.0f);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
	ChaosVehicleMovement->SetHandbrakeInput(false);
	BrakeLights(false);
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
	float SteeringValue = Value.Get<float>();

	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
	ThrottleInput = Value.Get<float>();

	ApplyThrottleInput();
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
	float BreakValue = Value.Get<float>();

	ChaosVehicleMovement->SetBrakeInput(BreakValue);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	BrakeLights(false);

	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	ChaosVehicleMovement->SetHandbrakeInput(true);

	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
	ChaosVehicleMovement->SetHandbrakeInput(false);

	BrakeLights(false);
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	float LookValue = Value.Get<float>();

	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);

	UE_LOG(LogTemplateVehicle, Error, TEXT("Reset Vehicle"));
}

float ACXPawn::GetMaxSpeed() const
{
	const ACXPlayerState* CXPlayerState = GetPlayerState<ACXPlayerState>();
	const float BonusMaxSpeed = IsValid(CXPlayerState) ? CXPlayerState->GetBonusMaxSpeed() : 0.0f;

	return BaseMaxSpeed + BonusMaxSpeed;
}

void ACXPawn::ApplyThrottleInput()
{
	float FilteredThrottle = ThrottleInput;

	const float CurrentSpeed = ChaosVehicleMovement->GetForwardSpeed() * 0.036f;
	if (FilteredThrottle > 0.0f && CurrentSpeed >= GetMaxSpeed())
	{
		FilteredThrottle = 0.0f;
	}

	ChaosVehicleMovement->SetThrottleInput(FilteredThrottle);
}

bool ACXPawn::CanFire() const
{
	return GetWorld()->GetTimeSeconds() - LastFireTime >= FireCooldown;
}

void ACXPawn::Fire(const FInputActionValue& Value)
{
	if (CanFire() == false)
	{
		return;
	}

	if (HasAuthority() == false)
	{
		LastFireTime = GetWorld()->GetTimeSeconds();
	}

	Server_Fire();
}

void ACXPawn::Server_Fire_Implementation()
{
	if (CanFire() == false)
	{
		return;
	}

	LastFireTime = GetWorld()->GetTimeSeconds();

	const FVector Start = FireStartPoint->GetComponentLocation();
	const FVector End = Start + GetActorForwardVector() * FireRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CXFire), false, this);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	ACXPawn* HitVehicle = bHit ? Cast<ACXPawn>(Hit.GetActor()) : nullptr;

	Multicast_PlayFireEffect(Start, bHit ? FVector(Hit.ImpactPoint) : End, IsValid(HitVehicle));

	if (IsValid(HitVehicle))
	{
		UGameplayStatics::ApplyDamage(HitVehicle, FireDamage, GetController(), this, UDamageType::StaticClass());
	}
}

void ACXPawn::Multicast_PlayFireEffect_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End, bool bHitVehicle)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	DrawDebugLine(GetWorld(), Start, End, bHitVehicle ? FColor::Red : FColor::Green, false, 1.0f, 0, 5.0f);

	if (bHitVehicle == false)
	{
		return;
	}

	if (FireHitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireHitEffect, End, FRotator::ZeroRotator, FVector(0.3f));
	}

	if (FireHitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireHitSound, End);
	}
}

float ACXPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority() == false)
	{
		return ActualDamage;
	}

	const ACXPawn* Attacker = Cast<ACXPawn>(DamageCauser);
	if (IsValid(Attacker) == false || Attacker == this)
	{
		return ActualDamage;
	}

	ACXPlayerState* VictimPlayerState = GetPlayerState<ACXPlayerState>();
	ACXPlayerState* AttackerPlayerState = Attacker->GetPlayerState<ACXPlayerState>();
	if (IsValid(VictimPlayerState) == false || IsValid(AttackerPlayerState) == false)
	{
		return ActualDamage;
	}

	if (VictimPlayerState->RemoveCoins(StealCoinAmount))
	{
		AttackerPlayerState->AddCoins(StealCoinAmount);
	}

	return ActualDamage;
}

#undef LOCTEXT_NAMESPACE
