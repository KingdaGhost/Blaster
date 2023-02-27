// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatSystemOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UChatSystemOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetChatText(const FString& Text, const FString& PlayerName);
	UPROPERTY(meta = (BindWidget))
	class UScrollBox* InputScrollBox;
	UPROPERTY(meta = (BindWidget))
	class UEditableText* InputTextBox;

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UChatBox> ChatBoxClass;
	UPROPERTY()
	TArray<UChatBox*> ChatMessages;

	UPROPERTY()
	class APlayerController* OwningPlayer;
};
