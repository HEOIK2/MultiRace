#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;
class UParticleSystem;
class USoundBase;
class ACXPawn;

UCLASS(abstract)
class CX_API ACXItemBase : public AActor
{
	GENERATED_BODY()

public:
	ACXItemBase();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	virtual void ApplyItemEffect(ACXPawn* Vehicle);

	UFUNCTION()
	void OnItemBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void PlayDestroyEffects() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<URotatingMovementComponent> RotatingMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<UParticleSystem> DestroyEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Item")
	FVector DestroyEffectScale = FVector(0.3);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Item")
	TObjectPtr<USoundBase> DestroySound;

	bool bCollected = false;
};
