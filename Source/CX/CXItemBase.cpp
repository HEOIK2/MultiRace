#include "CXItemBase.h"
#include "CXPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACXItemBase::ACXItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(1.0f);

	bAlwaysRelevant = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(100.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);

	RotatingMovementComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovementComponent->RotationRate = FRotator(0.0f, 180.0f, 0.0f);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (DefaultEffect.Succeeded())
	{
		DestroyEffect = DefaultEffect.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultSound(TEXT("/Game/StarterContent/Audio/Collapse_Cue.Collapse_Cue"));
	if (DefaultSound.Succeeded())
	{
		DestroySound = DefaultSound.Object;
	}
}

void ACXItemBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnItemBeginOverlap);
	}
}

void ACXItemBase::Destroyed()
{
	PlayDestroyEffects();

	Super::Destroyed();
}

void ACXItemBase::ApplyItemEffect(ACXPawn* Vehicle)
{
}

void ACXItemBase::OnItemBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority() == false || bCollected)
	{
		return;
	}

	ACXPawn* Vehicle = Cast<ACXPawn>(OtherActor);
	if (IsValid(Vehicle) == false)
	{
		return;
	}

	bCollected = true;

	ApplyItemEffect(Vehicle);
	Destroy();
}

void ACXItemBase::PlayDestroyEffects() const
{
	const UWorld* World = GetWorld();

	if (IsValid(World) == false || World->IsGameWorld() == false || World->bIsTearingDown || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (DestroyEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, DestroyEffect, GetActorLocation(), GetActorRotation(), DestroyEffectScale);
	}

	if (DestroySound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DestroySound, GetActorLocation());
	}
}
