#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "CXHUDWidget.h"
#include "CXChatWidget.h"
#include "CXGameMode.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Net/UnrealNetwork.h"

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	if (IsValid(VehicleUIClass))
	{
		VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);
		if (VehicleUI) VehicleUI->AddToViewport();
	}

	if (IsValid(HUDWidgetClass))
	{
		HUDWidget = CreateWidget<UCXHUDWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport();
	}

	if (IsValid(ChatWidgetClass))
	{
		ChatWidget = CreateWidget<UCXChatWidget>(this, ChatWidgetClass);
		ChatWidget->AddToViewport();
	}

	RefreshHUD();
}

bool ACXPlayerController::InputKey(const FInputKeyParams& Params)
{
	// Focus chat while the viewport handles Enter, before deferred action processing.
	// Returning here prevents the same press from also becoming a chat submission.
	if (IsLocalController() && Params.Key == EKeys::Enter && Params.Event == IE_Pressed)
	{
		BeginChatInput(FInputActionValue());
		return true;
	}
	return Super::InputKey(Params);
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);

		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ChatAction)
		{
			EnhancedInputComponent->BindAction(ChatAction, ETriggerEvent::Started, this, &ACXPlayerController::BeginChatInput);
		}
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	VehiclePawn = CastChecked<ACXPawn>(InPawn);
}

void ACXPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	RefreshHUD();
}

void ACXPlayerController::RefreshHUD()
{
	if (IsValid(HUDWidget) == false)
	{
		return;
	}

	if (const ACXPlayerState* CXPlayerState = GetPlayerState<ACXPlayerState>())
	{
		HUDWidget->UpdateCoins(CXPlayerState->GetCoins());
		HUDWidget->UpdateBonusMaxSpeed(CXPlayerState->GetBonusMaxSpeed());
	}

	const ACXGameState* CXGameState = GetWorld()->GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	HUDWidget->UpdateMatchState(CXGameState->GetMatchState(), CXGameState->GetRemainingTime());

	if (CXGameState->GetMatchState() == ECXMatchState::Finished)
	{
		HUDWidget->ShowResult(CXGameState->IsWinner(PlayerState), CXGameState->GetWinners().Num(), CXGameState->GetWinningCoins());
	}
	else
	{
		HUDWidget->HideResult();
	}
}

void ACXPlayerController::BeginChatInput(const FInputActionValue& Value)
{
	if (IsValid(ChatWidget) == false || bIsChatting)
	{
		return;
	}

	bIsChatting = true;
	// UI-only focus consumes key-release events; clear driving input first.
	if (IsValid(VehiclePawn)) VehiclePawn->ClearDrivingInput();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ChatWidget->GetChatInputSlateWidget());
	SetInputMode(InputMode);
	ChatWidget->FocusChatInput();
}

void ACXPlayerController::EndChatInput()
{
	if (bIsChatting == false)
	{
		return;
	}

	bIsChatting = false;
	SetInputMode(FInputModeGameOnly());
}

bool ACXPlayerController::SanitizeChatMessage(const FString& InMessage, FString& OutMessage) const
{
	OutMessage = InMessage.TrimStartAndEnd().Left(MaxChatMessageLength);
	return OutMessage.IsEmpty() == false;
}

void ACXPlayerController::SendChatMessage(const FString& Message)
{
	FString SafeMessage;
	if (SanitizeChatMessage(Message, SafeMessage))
	{
		Server_SendChatMessage(SafeMessage);
	}
}

void ACXPlayerController::Server_SendChatMessage_Implementation(const FString& Message)
{
	FString SafeMessage;
	if (SanitizeChatMessage(Message, SafeMessage) == false)
	{
		return;
	}

	ACXGameMode* CXGameMode = GetWorld()->GetAuthGameMode<ACXGameMode>();
	if (IsValid(CXGameMode) == false)
	{
		return;
	}

	const FString SenderName = IsValid(PlayerState) ? PlayerState->GetPlayerName() : TEXT("Unknown");
	CXGameMode->BroadcastChatMessage(FString::Printf(TEXT("%s : %s"), *SenderName, *SafeMessage));
}

void ACXPlayerController::Client_ReceiveChatMessage_Implementation(const FString& Message)
{
	if (IsValid(ChatWidget))
	{
		ChatWidget->AddChatMessage(Message);
	}
}
