#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class ACXGameState;
class APlayerStart;

UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	void BroadcastChatMessage(const FString& Message);

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

	void StartWaitingCountdown();
	void StartPlaying();
	void FinishMatch();

	void OnMatchTimerTick();

	ACXGameState* GetCXGameState() const;

private:
	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

	UPROPERTY(EditDefaultsOnly, Category = "CX|Match")
	int32 WaitingDuration = 10;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Match")
	int32 PlayingDuration = 90;

	FTimerHandle MatchTimerHandle;

	bool bWaitingCountdownStarted = false;
};
