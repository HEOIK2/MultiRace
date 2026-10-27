#include "CXPlayerState.h"
#include "CXPlayerController.h"
#include "Net/UnrealNetwork.h"

ACXPlayerState::ACXPlayerState()
{
	bReplicates = true;
}

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, BonusMaxSpeed, COND_OwnerOnly);
}

void ACXPlayerState::AddCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0)
	{
		return;
	}

	Coins += Amount;

	OnRep_Coins();
}

bool ACXPlayerState::RemoveCoins(int32 Amount)
{
	if (HasAuthority() == false || Amount <= 0 || Coins < Amount)
	{
		return false;
	}

	Coins -= Amount;
	OnRep_Coins();

	return true;
}

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (HasAuthority() == false || Amount <= 0.0f)
	{
		return;
	}

	BonusMaxSpeed += Amount;
	OnRep_BonusMaxSpeed();
}

void ACXPlayerState::OnRep_Coins()
{
	NotifyOwnerHUD();
}

void ACXPlayerState::OnRep_BonusMaxSpeed()
{
	NotifyOwnerHUD();
}

void ACXPlayerState::NotifyOwnerHUD() const
{
	if (ACXPlayerController* PC = Cast<ACXPlayerController>(GetPlayerController()))
	{
		PC->RefreshHUD();
	}
}
