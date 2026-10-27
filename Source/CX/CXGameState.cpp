#include "CXGameState.h"
#include "CXPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
	DOREPLIFETIME(ThisClass, RemainingTime);
	DOREPLIFETIME(ThisClass, Winners);
	DOREPLIFETIME(ThisClass, WinningCoins);
}

bool ACXGameState::IsWinner(const APlayerState* PlayerState) const
{
	return IsValid(PlayerState) && Winners.Contains(PlayerState);
}

void ACXGameState::SetMatchState(ECXMatchState NewMatchState)
{
	if (HasAuthority() == false)
	{
		return;
	}

	MatchState = NewMatchState;
	OnRep_MatchState();
}

void ACXGameState::SetRemainingTime(int32 NewRemainingTime)
{
	if (HasAuthority() == false)
	{
		return;
	}

	RemainingTime = FMath::Max(NewRemainingTime, 0);
	OnRep_RemainingTime();
}

void ACXGameState::SetWinners(const TArray<APlayerState*>& NewWinners, int32 NewWinningCoins)
{
	if (HasAuthority() == false)
	{
		return;
	}

	Winners.Reset();
	for (APlayerState* Winner : NewWinners)
	{
		Winners.Add(Winner);
	}
	WinningCoins = NewWinningCoins;

	OnRep_Winners();
}

void ACXGameState::OnRep_MatchState()
{
	NotifyLocalHUD();
}

void ACXGameState::OnRep_RemainingTime()
{
	NotifyLocalHUD();
}

void ACXGameState::OnRep_Winners()
{
	NotifyLocalHUD();
}

void ACXGameState::NotifyLocalHUD() const
{
	if (ACXPlayerController* PC = Cast<ACXPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->RefreshHUD();
	}
}
