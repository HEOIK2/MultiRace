#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

UENUM(BlueprintType)
enum class ECXMatchState : uint8
{
	Waiting,
	Playing,
	Finished
};

UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ECXMatchState GetMatchState() const { return MatchState; }
	int32 GetRemainingTime() const { return RemainingTime; }
	const TArray<TObjectPtr<APlayerState>>& GetWinners() const { return Winners; }
	int32 GetWinningCoins() const { return WinningCoins; }
	bool IsWinner(const APlayerState* PlayerState) const;

	void SetMatchState(ECXMatchState NewMatchState);

	void SetRemainingTime(int32 NewRemainingTime);

	void SetWinners(const TArray<APlayerState*>& NewWinners, int32 NewWinningCoins);

protected:
	UFUNCTION()
	void OnRep_MatchState();

	UFUNCTION()
	void OnRep_RemainingTime();

	UFUNCTION()
	void OnRep_Winners();

private:
	void NotifyLocalHUD() const;

private:
	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	ECXMatchState MatchState = ECXMatchState::Waiting;

	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime)
	int32 RemainingTime = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Winners)
	TArray<TObjectPtr<APlayerState>> Winners;

	UPROPERTY(ReplicatedUsing = OnRep_Winners)
	int32 WinningCoins = 0;
};
