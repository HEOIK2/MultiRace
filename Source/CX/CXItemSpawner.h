#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXItemSpawner.generated.h"

class ACXItemBase;

UCLASS()
class CX_API ACXItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACXItemSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SpawnItem();

	UFUNCTION()
	void OnSpawnedItemDestroyed(AActor* DestroyedActor);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Spawner")
	TSubclassOf<ACXItemBase> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Spawner")
	float RespawnDelay = 2.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Spawner")
	TObjectPtr<ACXItemBase> SpawnedItem;

	FTimerHandle RespawnTimerHandle;
};
