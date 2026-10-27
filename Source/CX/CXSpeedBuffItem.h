#pragma once

#include "CoreMinimal.h"
#include "CXItemBase.h"
#include "CXSpeedBuffItem.generated.h"

UCLASS()
class CX_API ACXSpeedBuffItem : public ACXItemBase
{
	GENERATED_BODY()

public:
	ACXSpeedBuffItem();

protected:
	virtual void ApplyItemEffect(ACXPawn* Vehicle) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CX|Item")
	float BonusMaxSpeedAmount = 10.0f;
};
