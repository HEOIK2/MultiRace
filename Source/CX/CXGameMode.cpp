#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
	GameStateClass = ACXGameState::StaticClass();
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	checkf(AvailablePlayerStarts.IsEmpty() == false, TEXT("There is no available Vehicle PlayerStart."));

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}

void ACXGameMode::InitGameState()
{
	Super::InitGameState();

	if (ACXGameState* CXGameState = GetCXGameState())
	{
		CXGameState->SetMatchState(ECXMatchState::Waiting);
		CXGameState->SetRemainingTime(WaitingDuration);
	}
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (IsValid(NewPlayer) && IsValid(NewPlayer->PlayerState))
	{
		BroadcastChatMessage(FString::Printf(TEXT("[System] %s joined."), *NewPlayer->PlayerState->GetPlayerName()));
	}

	if (bWaitingCountdownStarted == false)
	{
		StartWaitingCountdown();
	}
}

void ACXGameMode::BroadcastChatMessage(const FString& Message)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(It->Get()))
		{
			CXPlayerController->Client_ReceiveChatMessage(Message);
		}
	}
}

void ACXGameMode::StartWaitingCountdown()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	bWaitingCountdownStarted = true;

	CXGameState->SetMatchState(ECXMatchState::Waiting);
	CXGameState->SetRemainingTime(WaitingDuration);

	GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &ThisClass::OnMatchTimerTick, 1.0f, true);
}

void ACXGameMode::StartPlaying()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	CXGameState->SetMatchState(ECXMatchState::Playing);
	CXGameState->SetRemainingTime(PlayingDuration);

	BroadcastChatMessage(TEXT("[System] Game start! Collect as many coins as you can."));
}

void ACXGameMode::FinishMatch()
{
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);

	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	TArray<APlayerState*> Winners;
	int32 WinningCoins = -1;

	for (APlayerState* PlayerState : CXGameState->PlayerArray)
	{
		const ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState);
		if (IsValid(CXPlayerState) == false)
		{
			continue;
		}

		const int32 Coins = CXPlayerState->GetCoins();
		if (Coins > WinningCoins)
		{
			WinningCoins = Coins;
			Winners.Reset();
			Winners.Add(PlayerState);
		}
		else if (Coins == WinningCoins)
		{
			Winners.Add(PlayerState);
		}
	}

	WinningCoins = FMath::Max(WinningCoins, 0);

	CXGameState->SetWinners(Winners, WinningCoins);
	CXGameState->SetMatchState(ECXMatchState::Finished);

	TArray<FString> WinnerNames;
	for (const APlayerState* Winner : Winners)
	{
		WinnerNames.Add(Winner->GetPlayerName());
	}
	BroadcastChatMessage(FString::Printf(TEXT("[System] Game over! Winner : %s (%d coins)"), *FString::Join(WinnerNames, TEXT(", ")), WinningCoins));
}

void ACXGameMode::OnMatchTimerTick()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	CXGameState->SetRemainingTime(CXGameState->GetRemainingTime() - 1);

	if (CXGameState->GetRemainingTime() > 0)
	{
		return;
	}

	switch (CXGameState->GetMatchState())
	{
	case ECXMatchState::Waiting:
		StartPlaying();
		break;

	case ECXMatchState::Playing:
		FinishMatch();
		break;

	default:
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		break;
	}
}

ACXGameState* ACXGameMode::GetCXGameState() const
{
	return GetGameState<ACXGameState>();
}
