#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CXSetupCommandlet.generated.h"

// Reproducible editor setup: UnrealEditor-Cmd CX.uproject -run=CXSetup
UCLASS()
class UCXSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UCXSetupCommandlet();
	virtual int32 Main(const FString& Params) override;
};
