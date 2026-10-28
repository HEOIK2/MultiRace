// Editor-only integration check. Run CX.VerifyRequired (or CX.VerifyRequired Tie)
// during a fresh 2-player listen-server PIE session. Changes affect PIE only.
#if WITH_EDITOR
#include "CoreMinimal.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "CXPawn.h"
#include "CXCoinItem.h"
#include "CXSpeedBuffItem.h"
#include "CXItemSpawner.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

namespace CXVerification
{
UObject* ObjectProperty(UObject* Object, const FName Name)
{
	const FObjectPropertyBase* Property = Object ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
	return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
}

struct FRun
{
	TWeakObjectPtr<UWorld> Server, Client;
	TWeakObjectPtr<ACXPlayerController> Host, Remote, Viewer;
	TWeakObjectPtr<ACXItemSpawner> Spawner;
	TWeakObjectPtr<AActor> OldItem;
	int32 Stage = 0, Failures = 0;
	double Next = 0;
	bool bTie = false;

	void Check(bool Condition, const TCHAR* Label)
	{
		if (Condition) { UE_LOG(LogTemp, Display, TEXT("CXVerify PASS: %s"), Label); }
		else { ++Failures; UE_LOG(LogTemp, Error, TEXT("CXVerify FAIL: %s"), Label); }
	}

	bool Tick(float)
	{
		if (!Server.IsValid() || !Client.IsValid() || !Host.IsValid() || !Remote.IsValid() || !Viewer.IsValid()) return false;
		if (FPlatformTime::Seconds() < Next) return true;
		Next = FPlatformTime::Seconds() + 1.5;
		ACXPlayerState* HostState = Host->GetPlayerState<ACXPlayerState>();
		ACXPlayerState* RemoteState = Remote->GetPlayerState<ACXPlayerState>();
		ACXPlayerState* ViewState = Viewer->GetPlayerState<ACXPlayerState>();
		ACXGameState* GameState = Server->GetGameState<ACXGameState>();
		ACXGameState* ClientState = Client->GetGameState<ACXGameState>();
		ACXPawn* HostPawn = Cast<ACXPawn>(Host->GetPawn());
		ACXPawn* RemotePawn = Cast<ACXPawn>(Remote->GetPawn());
		if (!HostState || !RemoteState || !ViewState || !GameState || !ClientState || !HostPawn || !RemotePawn) return false;
		switch (Stage++)
		{
		case 0:
		{
			Check(ObjectProperty(Viewer.Get(), TEXT("HUDWidget")) != nullptr, TEXT("client HUD created"));
			Check(ObjectProperty(Viewer.Get(), TEXT("ChatWidget")) != nullptr, TEXT("client chat created"));
			Check(ObjectProperty(Host.Get(), TEXT("HUDWidget")) != nullptr, TEXT("listen host HUD created"));
			Check(GameState->GetMatchState() != ECXMatchState::Finished, TEXT("fresh match available"));
			HostState->RemoveCoins(HostState->GetCoins());
			RemoteState->RemoveCoins(RemoteState->GetCoins());
			HostState->AddCoins(20);
			GameState->SetMatchState(ECXMatchState::Waiting);
			GameState->SetRemainingTime(1);
			ACXCoinItem* Coin = Server->SpawnActor<ACXCoinItem>(RemotePawn->GetActorLocation() + FVector(0, 0, 2000), FRotator::ZeroRotator);
			Coin->SetActorLocation(RemotePawn->GetActorLocation());
			ACXSpeedBuffItem* Buff = Server->SpawnActor<ACXSpeedBuffItem>(RemotePawn->GetActorLocation() + FVector(0, 0, 2000), FRotator::ZeroRotator);
			Buff->SetActorLocation(RemotePawn->GetActorLocation());
			for (TActorIterator<ACXItemSpawner> It(Server.Get()); It; ++It)
			{
				Spawner = *It;
				OldItem = Cast<AActor>(ObjectProperty(*It, TEXT("SpawnedItem")));
				if (OldItem.IsValid()) { OldItem->Destroy(); break; }
			}
			Viewer->SendChatMessage(TEXT("CX network verification"));
			Next = FPlatformTime::Seconds() + 3.5;
			break;
		}
		case 1:
		{
			Check(RemoteState->GetCoins() == 10, TEXT("server coin overlap awards 10"));
			Check(ViewState->GetCoins() == RemoteState->GetCoins(), TEXT("owner coin replication"));
			Check(FMath::IsNearlyEqual(ViewState->GetBonusMaxSpeed(), 10.0f), TEXT("speed pickup and owner replication"));
			Check(GameState->GetMatchState() == ECXMatchState::Playing && ClientState->GetMatchState() == ECXMatchState::Playing, TEXT("waiting countdown transitions to playing on both machines"));
			Check(Spawner.IsValid() && ObjectProperty(Spawner.Get(), TEXT("SpawnedItem")) && ObjectProperty(Spawner.Get(), TEXT("SpawnedItem")) != OldItem.Get(), TEXT("destroyed pickup respawns after 2 seconds"));
			APlayerState* Other = nullptr;
			for (APlayerState* State : ClientState->PlayerArray) if (State != ViewState) Other = State;
			const ACXPlayerState* OtherState = Cast<ACXPlayerState>(Other);
			Check(OtherState && OtherState->GetCoins() == 0, TEXT("non-owner does not receive other player's coins"));
			for (ACXPlayerController* PC : { Host.Get(), Viewer.Get() })
			{
				UObject* Chat = ObjectProperty(PC, TEXT("ChatWidget"));
				UScrollBox* Log = Cast<UScrollBox>(ObjectProperty(Chat, TEXT("ChatLogScrollBox")));
				bool bFound = false;
				if (Log) for (UWidget* Child : Log->GetAllChildren())
					if (const UTextBlock* Text = Cast<UTextBlock>(Child)) bFound |= Text->GetText().ToString().Contains(TEXT("CX network verification"));
				Check(bFound, PC == Host.Get() ? TEXT("chat RPC delivered to host") : TEXT("chat RPC delivered to client"));
			}
			const FVector Location(-3500, -5000, 120);
			RemotePawn->ClearDrivingInput();
			HostPawn->ClearDrivingInput();
			RemotePawn->SetActorLocationAndRotation(Location, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
			HostPawn->SetActorLocationAndRotation(Location + FVector(1200, 0, 0), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
			for (ACXPawn* Pawn : {RemotePawn, HostPawn})
			{
				Pawn->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Pawn->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
			break;
		}
		case 2:
			if (APawn* Pawn = Viewer->GetPawn()) Pawn->ProcessEvent(Pawn->FindFunctionChecked(TEXT("Server_Fire")), nullptr);
			break;
		case 3:
			Check(HostState->GetCoins() == 15 && RemoteState->GetCoins() == 15, TEXT("client attack RPC traces vehicle and steals exactly 5"));
			HostState->RemoveCoins(HostState->GetCoins());
			HostState->AddCoins(4);
			if (APawn* Pawn = Viewer->GetPawn()) Pawn->ProcessEvent(Pawn->FindFunctionChecked(TEXT("Server_Fire")), nullptr);
			break;
		case 4:
			Check(HostState->GetCoins() == 4 && RemoteState->GetCoins() == 15, TEXT("victim with fewer than 5 coins loses nothing"));
			if (bTie) HostState->AddCoins(11);
			GameState->SetRemainingTime(1);
			Next = FPlatformTime::Seconds() + 2.5;
			break;
		case 5:
		{
			Check(GameState->GetMatchState() == ECXMatchState::Finished && ClientState->GetMatchState() == ECXMatchState::Finished, TEXT("playing countdown finishes and replicates"));
			Check(ClientState->GetWinners().Num() == (bTie ? 2 : 1) && ClientState->IsWinner(ViewState), bTie ? TEXT("tied players are co-winners") : TEXT("highest coin player wins"));
			UObject* HUD = ObjectProperty(Viewer.Get(), TEXT("HUDWidget"));
			UTextBlock* Result = Cast<UTextBlock>(ObjectProperty(HUD, TEXT("ResultText")));
			Check(Result && Result->GetText().ToString().Contains(bTie ? TEXT("Co-Winner") : TEXT("YOU WIN")) && Result->GetText().ToString().Contains(TEXT("15")), TEXT("client result UI shows winner and correct top coins"));
			UTextBlock* HostResult = Cast<UTextBlock>(ObjectProperty(ObjectProperty(Host.Get(), TEXT("HUDWidget")), TEXT("ResultText")));
			Check(HostResult && HostResult->GetText().ToString().Contains(bTie ? TEXT("Co-Winner") : TEXT("YOU LOSE")), TEXT("host result UI shows appropriate outcome"));
			UE_LOG(LogTemp, Display, TEXT("CXVerify COMPLETE (%s): %d failures"), bTie ? TEXT("Tie") : TEXT("Winner"), Failures);
			return false;
		}
		}
		return true;
	}
};

static FAutoConsoleCommand VerifyCommand(TEXT("CX.VerifyRequired"),
	TEXT("Verify required features in a fresh 2-player listen-server PIE. Optional argument: Tie"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		TSharedRef<FRun> Run = MakeShared<FRun>();
		Run->bTie = Args.Contains(TEXT("Tie"));
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || World->WorldType != EWorldType::PIE) continue;
			if (World->GetNetMode() == NM_ListenServer) Run->Server = World;
			if (World->GetNetMode() == NM_Client) Run->Client = World;
		}
		if (!Run->Server.IsValid() || !Run->Client.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("CXVerify: start 2-player listen-server PIE first.")); return;
		}
		for (FConstPlayerControllerIterator It = Run->Server->GetPlayerControllerIterator(); It; ++It)
		{
			ACXPlayerController* PC = Cast<ACXPlayerController>(It->Get());
			if (!PC) continue;
			if (PC->IsLocalController()) Run->Host = PC; else Run->Remote = PC;
		}
		Run->Viewer = Cast<ACXPlayerController>(Run->Client->GetFirstPlayerController());
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Delta){ return Run->Tick(Delta); }), 0.1f);
	}));
}
#endif
