#include "CXSetupCommandlet.h"

UCXSetupCommandlet::UCXSetupCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

#if WITH_EDITOR
#include "CXChatWidget.h"
#include "CXHUDWidget.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "CXCoinItem.h"
#include "CXSpeedBuffItem.h"
#include "CXItemSpawner.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/GameModeBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace CXSetup
{
bool Save(UObject* Object)
{
	UPackage* Package = Object->GetOutermost();
	Package->MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	return UPackage::SavePackage(Package, Object, *Filename, Args);
}

bool SetObject(UObject* Target, const FName Name, UObject* Value)
{
	FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Target->GetClass(), Name);
	if (!Property || !Value)
	{
		UE_LOG(LogTemp, Error, TEXT("CXSetup: missing property/value %s"), *Name.ToString());
		return false;
	}
	Target->Modify();
	Property->SetObjectPropertyValue_InContainer(Target, Value);
	Target->PostEditChange();
	return true;
}

UInputAction* Action(const FString& Path)
{
	if (UInputAction* Existing = LoadObject<UInputAction>(nullptr, *Path)) return Existing;
	UPackage* Package = CreatePackage(*Path);
	UInputAction* Result = NewObject<UInputAction>(Package, *FPackageName::GetLongPackageAssetName(Path), RF_Public | RF_Standalone);
	Result->ValueType = EInputActionValueType::Boolean;
	FAssetRegistryModule::AssetCreated(Result);
	return Save(Result) ? Result : nullptr;
}

UWidgetBlueprint* Widget(const FString& Path, UClass* Parent)
{
	if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, *Path)) return Existing;
	UPackage* Package = CreatePackage(*Path);
	UWidgetBlueprint* BP = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		Parent, Package, *FPackageName::GetLongPackageAssetName(Path), BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	FAssetRegistryModule::AssetCreated(BP);
	return BP;
}

UTextBlock* Text(UWidgetTree* Tree, UPanelWidget* Parent, const TCHAR* Name, const TCHAR* Label, int32 Size, FLinearColor Color = FLinearColor::White)
{
	UTextBlock* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Result->SetText(FText::FromString(Label));
	FSlateFontInfo Font = Result->GetFont();
	Font.Size = Size;
	Result->SetFont(Font);
	Result->SetColorAndOpacity(FSlateColor(Color));
	Result->SetShadowOffset(FVector2D(1.0, 2.0));
	Result->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f));
	Parent->AddChild(Result);
	return Result;
}

UVerticalBox* Panel(UWidgetTree* Tree, UCanvasPanel* Root, const TCHAR* Name, FAnchors Anchors, FVector2D Alignment, FMargin Offsets)
{
	UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Border->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.88f));
	Border->SetPadding(FMargin(18.0f, 12.0f));
	UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Border);
	Slot->SetAnchors(Anchors);
	Slot->SetAlignment(Alignment);
	Slot->SetOffsets(Offsets);
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>();
	Border->SetContent(Box);
	return Box;
}

bool HUD()
{
	UWidgetBlueprint* BP = Widget(TEXT("/Game/MultiRace/UI/WBP_HUD"), UCXHUDWidget::StaticClass());
	if (!BP->WidgetTree->RootWidget)
	{
		UWidgetTree* Tree = BP->WidgetTree;
		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;
		UVerticalBox* Stats = Panel(Tree, Root, TEXT("StatsPanel"), FAnchors(0, 0), FVector2D::ZeroVector, FMargin(24, 24, 360, 138));
		Text(Tree, Stats, TEXT("GameTitle"), TEXT("MULTIRACE / COIN RUSH"), 16, FLinearColor(1, 0.7f, 0.12f));
		Text(Tree, Stats, TEXT("CoinsText"), TEXT("Coins : 0"), 30);
		Text(Tree, Stats, TEXT("BonusMaxSpeedText"), TEXT("Bonus Max Speed : +0.0 km/h"), 18);
		UVerticalBox* Match = Panel(Tree, Root, TEXT("MatchPanel"), FAnchors(0.5f, 0), FVector2D(0.5f, 0), FMargin(0, 24, 310, 110));
		Text(Tree, Match, TEXT("MatchStateText"), TEXT("Waiting for players..."), 20)->SetJustification(ETextJustify::Center);
		Text(Tree, Match, TEXT("RemainingTimeText"), TEXT("00:10"), 34, FLinearColor(1, 0.7f, 0.12f))->SetJustification(ETextJustify::Center);
		UTextBlock* Result = Text(Tree, Root, TEXT("ResultText"), TEXT("YOU WIN!"), 46, FLinearColor(1, 0.8f, 0.2f));
		Result->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot* ResultSlot = CastChecked<UCanvasPanelSlot>(Result->Slot);
		ResultSlot->SetAnchors(FAnchors(0.5f, 0.4f));
		ResultSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ResultSlot->SetAutoSize(true);
		Result->SetVisibility(ESlateVisibility::Collapsed);
		UTextBlock* Help = Text(Tree, Root, TEXT("ControlsHint"), TEXT("WASD Drive  |  Space Handbrake  |  LMB Steal 5 coins  |  Enter Chat"), 16);
		UCanvasPanelSlot* HelpSlot = CastChecked<UCanvasPanelSlot>(Help->Slot);
		HelpSlot->SetAnchors(FAnchors(0.5f, 1));
		HelpSlot->SetAlignment(FVector2D(0.5f, 1));
		HelpSlot->SetPosition(FVector2D(0, -20));
		HelpSlot->SetAutoSize(true);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	return BP->Status != BS_Error && Save(BP);
}

bool Chat()
{
	UWidgetBlueprint* BP = Widget(TEXT("/Game/MultiRace/UI/WBP_Chat"), UCXChatWidget::StaticClass());
	if (!BP->WidgetTree->RootWidget)
	{
		UWidgetTree* Tree = BP->WidgetTree;
		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;
		UVerticalBox* Box = Panel(Tree, Root, TEXT("ChatPanel"), FAnchors(1, 1), FVector2D(1, 1), FMargin(-24, -64, 480, 230));
		Text(Tree, Box, TEXT("ChatTitle"), TEXT("CHAT / ENTER TO TYPE"), 14, FLinearColor(1, 0.7f, 0.12f));
		UScrollBox* Log = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChatLogScrollBox"));
		UVerticalBoxSlot* LogSlot = Box->AddChildToVerticalBox(Log);
		LogSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LogSlot->SetPadding(FMargin(0, 8, 0, 8));
		UEditableTextBox* Input = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ChatInputTextBox"));
		Input->SetHintText(FText::FromString(TEXT("Enter to chat...")));
		Input->SetClearKeyboardFocusOnCommit(true);
		Box->AddChildToVerticalBox(Input);
	}
	// Keep the template speedometer visible at the bottom left.
	if (UWidget* ChatPanel = BP->WidgetTree->FindWidget(TEXT("ChatPanel")))
	{
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(ChatPanel->Slot))
		{
			Slot->SetAnchors(FAnchors(1, 1));
			Slot->SetAlignment(FVector2D(1, 1));
			Slot->SetOffsets(FMargin(-24, -64, 480, 230));
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	return BP->Status != BS_Error && Save(BP);
}

bool ConfigureBP(const TCHAR* Path, const TMap<FName, UObject*>& Properties)
{
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, Path);
	if (!BP || !BP->GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("CXSetup: blueprint missing: %s"), Path);
		return false;
	}
	for (const TPair<FName, UObject*>& Pair : Properties)
	{
		if (!SetObject(BP->GeneratedClass->GetDefaultObject(), Pair.Key, Pair.Value)) return false;
	}
	return Save(BP);
}
}
#endif

int32 UCXSetupCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	using namespace CXSetup;
	UInputAction* Fire = Action(TEXT("/Game/VehicleTemplate/Input/Actions/IA_Fire"));
	UInputAction* ChatAction = Action(TEXT("/Game/VehicleTemplate/Input/Actions/IA_Chat"));
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/VehicleTemplate/Input/IMC_Default"));
	if (!Fire || !ChatAction || !Context || !HUD() || !Chat()) return 1;
	Context->UnmapAllKeysFromAction(Fire);
	Context->UnmapAllKeysFromAction(ChatAction);
	Context->MapKey(Fire, EKeys::LeftMouseButton);
	Context->MapKey(ChatAction, EKeys::Enter);
	if (!Save(Context)) return 1;
	UClass* HUDClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/MultiRace/UI/WBP_HUD.WBP_HUD_C"));
	UClass* ChatClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/MultiRace/UI/WBP_Chat.WBP_Chat_C"));
	if (!ConfigureBP(TEXT("/Game/VehicleTemplate/Blueprints/BP_VehicleAdvPlayerController"), {
		{TEXT("HUDWidgetClass"), HUDClass}, {TEXT("ChatWidgetClass"), ChatClass}, {TEXT("ChatAction"), ChatAction}})) return 1;
	for (const TCHAR* Path : { TEXT("/Game/VehicleTemplate/Blueprints/SportsCar/BP_SportsCar_Pawn"), TEXT("/Game/VehicleTemplate/Blueprints/OffroadCar/BP_OffroadCar_Pawn") })
	{
		if (!ConfigureBP(Path, {{TEXT("FireAction"), Fire}})) return 1;
	}
	for (const TCHAR* Path : { TEXT("/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode"), TEXT("/Game/VehicleTemplate/Blueprints/BP_VehicleAdvOffroadGameMode") })
	{
		if (!ConfigureBP(Path, {{TEXT("GameStateClass"), ACXGameState::StaticClass()}, {TEXT("PlayerStateClass"), ACXPlayerState::StaticClass()}})) return 1;
	}

	const FString MapPath = TEXT("/Game/VehicleTemplate/Maps/VehicleAdvExampleMap");
	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapPath);
	if (!World) return 1;
	FVector StartLocation = FVector::ZeroVector;
	int32 Starts = 0;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		StartLocation = It->GetActorLocation();
		UE_LOG(LogTemp, Display, TEXT("CXSetup PlayerStart: %s"), *StartLocation.ToString());
		++Starts;
	}
	for (int32 Index = Starts; Index < 3; ++Index)
	{
		APlayerStart* Start = World->SpawnActor<APlayerStart>(StartLocation + FVector(0, (Index + 1) * 650.0f, 0), FRotator::ZeroRotator);
		if (!Start) return 1;
		Start->SetActorLabel(FString::Printf(TEXT("MultiRace_PlayerStart_%d"), Index));
	}
	int32 ExistingSpawners = 0;
	for (TActorIterator<ACXItemSpawner> It(World); It; ++It) ++ExistingSpawners;
	if (ExistingSpawners == 0)
	{
		for (int32 Row = 0; Row < 6; ++Row)
		{
			for (int32 Lane = 0; Lane < 3; ++Lane)
			{
				const FVector Location = StartLocation + FVector(1100.0f + Row * 650.0f, (Lane - 1) * 500.0f, 10.0f);
				ACXItemSpawner* Spawner = World->SpawnActor<ACXItemSpawner>(Location, FRotator::ZeroRotator);
				if (!Spawner) return 1;
				const bool bBuff = Lane == 1 && Row % 2 == 1;
				if (!SetObject(Spawner, TEXT("ItemClass"), bBuff ? ACXSpeedBuffItem::StaticClass() : ACXCoinItem::StaticClass())) return 1;
				Spawner->SetActorLabel(FString::Printf(TEXT("MultiRace_%s_%d_%d"), bBuff ? TEXT("SpeedBuff") : TEXT("Coin"), Row, Lane));
			}
		}
	}
	World->GetWorldSettings()->DefaultGameMode = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode.BP_VehicleAdvGameMode_C"));
	if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPath)) return 1;
	if (!UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true)) return 1;
	UE_LOG(LogTemp, Display, TEXT("CXSetup SUCCESS: input, widgets, blueprint defaults, starts and pickup spawners saved."));
	return 0;
#else
	return 1;
#endif
}


#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
static FAutoConsoleCommand CXSetupRequiredAssetsCommand(
	TEXT("CX.SetupRequiredAssets"),
	TEXT("Create and connect MultiRace required input/UI assets and map pickups."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UCXSetupCommandlet* Setup = NewObject<UCXSetupCommandlet>();
		Setup->AddToRoot();
		const int32 Result = Setup->Main(FString());
		UE_LOG(LogTemp, Display, TEXT("CXSetup result: %d"), Result);
		Setup->RemoveFromRoot();
	}));
#endif
