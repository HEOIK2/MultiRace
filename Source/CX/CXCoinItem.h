#pragma once

#include "CoreMinimal.h"
#include "CXItemBase.h"
#include "CXCoinItem.generated.h"

UCLASS()
class CX_API ACXCoinItem : public ACXItemBase
{
	GENERATED_BODY()

public:
	ACXCoinItem();

protected:
	virtual void ApplyItemEffect(ACXPawn* Vehicle) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Item")
	int32 CoinAmount = 10;
};
