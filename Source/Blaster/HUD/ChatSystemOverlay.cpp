// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatSystemOverlay.h"

#include "ChatBox.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"


void UChatSystemOverlay::SetChatText(const FString& Text, const FString& PlayerName)
{
	const FString Chat = PlayerName + " : " + Text;
	
	if (ChatBoxClass)
	{
		OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayer() : OwningPlayer;
		if (OwningPlayer)
		{
			UChatBox* ChatBoxWidget = CreateWidget<UChatBox>(OwningPlayer, ChatBoxClass); // Create a new UWidget of type UChatBox every time a text is being committed
			if (InputScrollBox && ChatBoxWidget && ChatBoxWidget->ChatTextBlock)
			{
				// UE_LOG(LogTemp, Warning, TEXT("Text: %s"), *Chat);
				ChatBoxWidget->ChatTextBlock->SetText(FText::FromString(Chat));
				ChatBoxWidget->ChatTextBlock->SetAutoWrapText(true);
				ChatMessages.Add(ChatBoxWidget);
				InputScrollBox->AddChild(ChatBoxWidget->ChatTextBlock); // This will add the TextBlock from WBP_ChatBox to the ScrollBox from top to bottom
				InputScrollBox->ScrollToEnd();
				InputScrollBox->bAnimateWheelScrolling = true;
			}
		}		
	}
}


