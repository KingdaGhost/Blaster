// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"


void UOverheadWidget::SetDisplayText(FString TextToDisplay) 
{
    if(DisplayText)
    {
        DisplayText->SetText(FText::FromString(TextToDisplay));
    }
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
    ENetRole RemoteRole = InPawn->GetRemoteRole();
    FString Role;
    switch (RemoteRole)
    {
    case ENetRole::ROLE_Authority:
        Role = FString("Authority");        
        break;
    case ENetRole::ROLE_AutonomousProxy:
        Role = FString("Autonomous  Proxy");        
        break;
    case ENetRole::ROLE_SimulatedProxy:
        Role = FString("Simulated Proxy");        
        break;
    case ENetRole::ROLE_None:
        Role = FString("None");        
        break;    
<<<<<<< HEAD
    }    
    APlayerState* PlayerState = InPawn->GetPlayerState<APlayerState>();
    if(PlayerState)
    {
        FString NameOfPlayer = PlayerState->GetPlayerName();  
        FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s \n Name: %s"), *Role, *NameOfPlayer);
        SetDisplayText(RemoteRoleString);
    }
=======
    }
    FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
    SetDisplayText(RemoteRoleString);

}

void UOverheadWidget::ShowPlayerName()
{
    APlayerController* PlayerController = GetOwningPlayer();
    APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();
    if(PlayerState)
    {
        FString PlayerName = PlayerState->GetPlayerName();  
        SetDisplayText(PlayerName);
    }
    
>>>>>>> e737ba831575f11e1e8f7b09bfb05a21db45032f
}

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
    RemoveFromParent();
    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}
