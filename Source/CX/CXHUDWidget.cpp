#include "CXHUDWidget.h"
#include "Components/TextBlock.h"

void UCXHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateCoins(0);
	UpdateBonusMaxSpeed(0.0f);
	UpdateMatchState(ECXMatchState::Waiting, 0);
	HideResult();
}

void UCXHUDWidget::UpdateCoins(int32 NewCoins)
{
	if (IsValid(CoinsText))
	{
		CoinsText->SetText(FText::FromString(FString::Printf(TEXT("Coins : %d"), NewCoins)));
	}
}

void UCXHUDWidget::UpdateBonusMaxSpeed(float NewBonusMaxSpeed)
{
	if (IsValid(BonusMaxSpeedText))
	{
		BonusMaxSpeedText->SetText(FText::FromString(FString::Printf(TEXT("Bonus Max Speed : %+.1f km/h"), NewBonusMaxSpeed)));
	}
}

void UCXHUDWidget::UpdateMatchState(ECXMatchState NewMatchState, int32 RemainingTime)
{
	if (IsValid(MatchStateText))
	{
		FString StateString;
		switch (NewMatchState)
		{
		case ECXMatchState::Waiting:	StateString = TEXT("Waiting for players...");	break;
		case ECXMatchState::Playing:	StateString = TEXT("Playing");					break;
		case ECXMatchState::Finished:	StateString = TEXT("Finished");					break;
		}

		MatchStateText->SetText(FText::FromString(StateString));
	}

	if (IsValid(RemainingTimeText))
	{
		const FString TimeString = (NewMatchState == ECXMatchState::Finished)
			? FString()
			: FString::Printf(TEXT("%02d:%02d"), RemainingTime / 60, RemainingTime % 60);

		RemainingTimeText->SetText(FText::FromString(TimeString));
	}
}

void UCXHUDWidget::ShowResult(bool bIsWinner, int32 WinnerCount, int32 WinningCoins)
{
	if (IsValid(ResultText) == false)
	{
		return;
	}

	FString ResultString;
	if (bIsWinner)
	{
		ResultString = (WinnerCount > 1) ? TEXT("DRAW - Co-Winner!") : TEXT("YOU WIN!");
	}
	else
	{
		ResultString = TEXT("YOU LOSE...");
	}
	ResultString += FString::Printf(TEXT("\nTop Coins : %d"), WinningCoins);

	ResultText->SetText(FText::FromString(ResultString));
	ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCXHUDWidget::HideResult()
{
	if (IsValid(ResultText))
	{
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
