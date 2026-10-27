#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "CXChatWidget.generated.h"

class UEditableTextBox;
class UScrollBox;

UCLASS(abstract)
class CX_API UCXChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void AddChatMessage(const FString& Message);

	TSharedPtr<SWidget> GetChatInputSlateWidget() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnChatInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ChatLogScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInputTextBox;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Chat")
	int32 MaxChatLines = 50;

	UPROPERTY(EditDefaultsOnly, Category = "CX|Chat")
	int32 ChatFontSize = 14;
};
