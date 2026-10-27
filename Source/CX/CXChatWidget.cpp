#include "CXChatWidget.h"
#include "CXPlayerController.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UCXChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(ChatInputTextBox))
	{
		ChatInputTextBox->OnTextCommitted.AddUniqueDynamic(this, &ThisClass::OnChatInputCommitted);
	}
}

void UCXChatWidget::NativeDestruct()
{
	if (IsValid(ChatInputTextBox))
	{
		ChatInputTextBox->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputCommitted);
	}

	Super::NativeDestruct();
}

void UCXChatWidget::AddChatMessage(const FString& Message)
{
	if (IsValid(ChatLogScrollBox) == false)
	{
		return;
	}

	UTextBlock* MessageText = NewObject<UTextBlock>(ChatLogScrollBox);
	MessageText->SetText(FText::FromString(Message));
	MessageText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", ChatFontSize));
	MessageText->SetAutoWrapText(true);

	ChatLogScrollBox->AddChild(MessageText);

	while (ChatLogScrollBox->GetChildrenCount() > MaxChatLines)
	{
		ChatLogScrollBox->RemoveChildAt(0);
	}

	ChatLogScrollBox->ScrollToEnd();
}

TSharedPtr<SWidget> UCXChatWidget::GetChatInputSlateWidget() const
{
	return IsValid(ChatInputTextBox) ? ChatInputTextBox->GetCachedWidget() : nullptr;
}

void UCXChatWidget::OnChatInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	ACXPlayerController* PC = GetOwningPlayer<ACXPlayerController>();
	if (IsValid(PC) == false)
	{
		return;
	}

	if (CommitMethod == ETextCommit::OnEnter)
	{
		PC->SendChatMessage(Text.ToString());
		ChatInputTextBox->SetText(FText::GetEmpty());
	}

	PC->EndChatInput();
}
