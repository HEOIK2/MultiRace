#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXGameState.h"
#include "CXHUDWidget.generated.h"

class UTextBlock;

UCLASS(abstract)
class CX_API UCXHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateCoins(int32 NewCoins);
	void UpdateBonusMaxSpeed(float NewBonusMaxSpeed);
	void UpdateMatchState(ECXMatchState NewMatchState, int32 RemainingTime);

	void ShowResult(bool bIsWinner, int32 WinnerCount, int32 WinningCoins);
	void HideResult();

protected:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CoinsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BonusMaxSpeedText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchStateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RemainingTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;
};
