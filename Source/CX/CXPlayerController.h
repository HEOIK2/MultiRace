#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ACXPawn;
class UCXUI;
class UCXHUDWidget;
class UCXChatWidget;
struct FInputActionValue;

UCLASS(abstract)
class CX_API ACXPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	bool bUseSteeringWheelControls = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(EditCondition="bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ChatAction;

	UPROPERTY(Replicated)
	TObjectPtr<ACXPawn> VehiclePawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXUI> VehicleUIClass;

	UPROPERTY()
	TObjectPtr<UCXUI> VehicleUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXHUDWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UCXHUDWidget> HUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXChatWidget> ChatWidgetClass;

	UPROPERTY()
	TObjectPtr<UCXChatWidget> ChatWidget;

	UPROPERTY(EditDefaultsOnly, Category = Chat)
	int32 MaxChatMessageLength = 100;

	bool bIsChatting = false;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float Delta) override;

	virtual bool InputKey(const FInputKeyParams& Params) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

public:
	virtual void OnRep_PlayerState() override;

public:
	void RefreshHUD();

	void SendChatMessage(const FString& Message);

	void EndChatInput();

	UFUNCTION(Client, Reliable)
	void Client_ReceiveChatMessage(const FString& Message);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SendChatMessage(const FString& Message);

	void BeginChatInput(const FInputActionValue& Value);

	bool SanitizeChatMessage(const FString& InMessage, FString& OutMessage) const;
};
