// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
    PathToLobby = FString::Printf(TEXT("%s?listen"), *LobbyPath);
    NumPublicConnections = NumberOfPublicConnections;
    MatchType = TypeOfMatch;

    AddToViewport();
    SetVisibility(ESlateVisibility::Visible);
    bIsFocusable = true;

    UWorld* World = GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);
        }
    }
    // We need the GameInstance in order to call the Subsystem since The MultiplayerSessionsSubsystem is derived from UGameInstanceSubsystem
    UGameInstance* GameInstance = GetGameInstance();
    if(GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
    }
    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
        MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
        MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
        MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
        MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
    }
}

bool UMenu::Initialize()
{
    if(!Super::Initialize())
    {
        return false;
    }

    if(HostButton)
    {
        HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
    }
    if(JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
    }
    if(StartButton)
    {
        StartButton->OnClicked.AddDynamic(this, &ThisClass::StartButtonClicked);
    }
    return true;
}

void UMenu::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
    // Super::OnLevelRemovedFromWorld(InLevel, InWorld);
	MenuTeardown();
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
    if(bWasSuccessful)
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Yellow,
                FString(TEXT("Session Created Successfully!"))
            );
        }        
    }
    else
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Red,
                FString(TEXT("Failed to create session!"))
            );
        }
        HostButton->SetVisibility(ESlateVisibility::Visible);
        JoinButton->SetVisibility(ESlateVisibility::Visible);
        StartButton->SetVisibility(ESlateVisibility::Hidden);        
    }
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResult, bool bWasSuccessful)
{
    if(MultiplayerSessionsSubsystem == nullptr)
    {
        return;
    }

    for(auto Result : SessionResult)
    {
        FString SettingsValue;
        Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);

        if(SettingsValue == MatchType)
        {
            MultiplayerSessionsSubsystem->JoinSession(Result);
            return;
        }
    }
    if(!bWasSuccessful || SessionResult.Num() == 0)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem* SubSystem = IOnlineSubsystem::Get();
    if(SubSystem)
    {
        IOnlineSessionPtr SessionInterface = SubSystem->GetSessionInterface();
        if(SessionInterface.IsValid())
        {
            FString Address;
            SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);
            
            APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if(PlayerController)
            {
                PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
            }
        }
    }
    if(Result != EOnJoinSessionCompleteResult::Success)
    {
        JoinButton->SetIsEnabled(true);
    }
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
    if(bWasSuccessful)
    {
        if(GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Yellow,
                FString(TEXT("Session started Successfully!"))
            );
        }   
        // Go to Lobby Map
        UWorld* World = GetWorld();
        if(World)
        {  
            World->ServerTravel(PathToLobby);
        }
    }
    else
    {
        StartButton->SetIsEnabled(true);
    }
       
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{

}

void UMenu::HostButtonClicked()
{	
    HostButton->SetVisibility(ESlateVisibility::Hidden);
    JoinButton->SetVisibility(ESlateVisibility::Hidden);
    StartButton->SetVisibility(ESlateVisibility::Visible);
    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);        
    }    
}

void UMenu::JoinButtonClicked()
{
	JoinButton->SetIsEnabled(false);
    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->FindSessions(10000);
    }
}

void UMenu::StartButtonClicked()
{
    StartButton->SetIsEnabled(false);
    if(MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->StartSession();
    }
}

void UMenu::MenuTeardown()
{
	RemoveFromParent();
    UWorld* World = GetWorld();
    if(World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if(PlayerController)
        {
            FInputModeGameOnly InputModeData;
            PlayerController->SetInputMode(InputModeData); // Using Default of FInputModeGameOnly
            PlayerController->SetShowMouseCursor(false);
        }
        
    }
}