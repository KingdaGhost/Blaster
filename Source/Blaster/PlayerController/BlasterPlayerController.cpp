// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"

#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterTypes/Announcement.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/GameModes/BlasterGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/ChatSystemOverlay.h"
#include "Blaster/HUD/ReturnToMainMenu.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


void ABlasterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim); // This is so that all the clients will display the ElimAnnouncement locally on to their own screen
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			if (Attacker == Self && Victim != Self) // we killed other player
			{
				BlasterHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Victim == Self && Attacker != Self) // someone killed us
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
				return;
			}
			if (Attacker == Victim && Attacker == Self) // we killed ourselves
			{
				BlasterHUD->AddElimAnnouncement("You", "Yourself");
				return;
			}
			if (Attacker == Victim && Attacker != Self) // someone else killed themselves
			{
				BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "Themselves");
				return;
			}
			BlasterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());
		}
	}
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	BlasterHUD = Cast<ABlasterHUD>(GetHUD());

	ServerCheckMatchState();
	AddChatBox();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamScores);
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->RedTeamScore &&
				BlasterHUD->CharacterOverlay->BlueTeamScore &&
					BlasterHUD->CharacterOverlay->ScoreSeparator;
	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText());
		BlasterHUD->CharacterOverlay->ScoreSeparator->SetText(FText());
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->RedTeamScore &&
				BlasterHUD->CharacterOverlay->BlueTeamScore &&
					BlasterHUD->CharacterOverlay->ScoreSeparator;
	if(bHudValid)
	{
		FString Zero("0");
		FString Separator("|");
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(Zero));
		BlasterHUD->CharacterOverlay->ScoreSeparator->SetText(FText::FromString(Separator));
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->RedTeamScore;
	if(bHudValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), RedScore);
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->BlueTeamScore;
	if(bHudValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), BlueScore);
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);	
	CheckTimeSync(DeltaSeconds);
	SetHUDTime();
	PollInit();
	CheckPing(DeltaSeconds);	
}

void ABlasterPlayerController::CheckPing(float DeltaSeconds)
{
	HighPingRunningTime += DeltaSeconds;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		// UE_LOG(LogTemp, Warning, TEXT("PlayerState->GetCompressedPing() * 4: %d"), PlayerState->GetCompressedPing() * 4);
		PlayerState = PlayerState == nullptr  ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			if(PlayerState->GetCompressedPing() * 4 > HighPingThreshold) //ping is compressed; it's actually ping/4
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f; //This is done once every 20 seconds
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying = BlasterHUD && BlasterHUD->CharacterOverlay
	&& BlasterHUD->CharacterOverlay->HighPingAnim
	&& BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnim);
	
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaSeconds;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuWidget == nullptr) return;;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen; // This is set to the opposite to itself because
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTeardown();
		}
	}	
}

void ABlasterPlayerController::AddChatBox()
{
	if (!IsLocalPlayerController()) return;
	if (ChatSystemOverlayClass)
	{
		ChatSystemWidget = ChatSystemWidget == nullptr ? CreateWidget<UChatSystemOverlay>(this, ChatSystemOverlayClass) : ChatSystemWidget;
		if (ChatSystemWidget)
		{
			ChatSystemWidget->AddToViewport();
			ChatSystemWidget->InputTextBox->SetVisibility(ESlateVisibility::Collapsed);
			ChatSystemWidget->InputTextBox->OnTextCommitted.AddDynamic(this, &ABlasterPlayerController::OnTextCommitted);
		}
	}
}

void ABlasterPlayerController::ToggleInputChatBox()
{
	if (ChatSystemWidget && ChatSystemWidget->InputTextBox)
	{
 		if (ChatSystemWidget->InputTextBox->GetVisibility() == ESlateVisibility::Collapsed)
 		{
 			ChatSystemWidget->InputTextBox->SetVisibility(ESlateVisibility::Visible);
 			FInputModeGameAndUI InputMode;
 			InputMode.SetWidgetToFocus(ChatSystemWidget->InputTextBox->TakeWidget());
 			SetInputMode(InputMode);
 			SetShowMouseCursor(true);
 		}
 		else
 		{
 			ChatSystemWidget->InputTextBox->SetVisibility(ESlateVisibility::Collapsed);
 			FInputModeGameOnly InputMode;
 			SetInputMode(InputMode);
 			SetShowMouseCursor(false);
 		}
	}
}

void ABlasterPlayerController::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter) return;
	
	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
	FString PlayerName("");
	if (PlayerState)
	{
		PlayerName = PlayerState->GetPlayerName();
	}
	if(ChatSystemWidget)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Here"));
		if (!Text.IsEmpty())
		{
			ServerSetText(Text.ToString(), PlayerName);
		}
		ChatSystemWidget->InputTextBox->SetText(FText());
		ChatSystemWidget->InputTextBox->SetVisibility(ESlateVisibility::Collapsed);
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
	}
}

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

void ABlasterPlayerController::ServerSetText_Implementation(const FString& Text, const FString& PlayerName)
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	if (BlasterGameMode)
	{
		BlasterGameMode->SendChat(Text, PlayerName);
	}
}

void ABlasterPlayerController::ClientSetText_Implementation(const FString& Text, const FString& PlayerName)
{
	PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
	if (ChatSystemWidget && PlayerState)
	{
		if (PlayerName == PlayerState->GetPlayerName())
		{
			ChatSystemWidget->SetChatText(Text, "You");
		}
		else
		{
			ChatSystemWidget->SetChatText(Text, PlayerName);
		}
	}
}


// Is the ping too high
void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	// UE_LOG(LogTemp, Warning, TEXT("bHighPing: %d"), bHighPing);
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::PollInit()
{
	if(CharacterOverlay == nullptr)
	{
		if(BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if(CharacterOverlay)
			{
				SetHUDWeaponType(EWeaponType::EWT_Pistol);
				if(bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if(bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if(bInitializeScore) SetHUDScore(HUDScore);
				if(bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if(bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if(bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);
				
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombat())
				{
					if(bInitializeGrenades) SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
				}				
			}
		}
	}
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ABlasterPlayerController::ShowReturnToMainMenu);
	InputComponent->BindAction("Chat", IE_Pressed, this, &ABlasterPlayerController::ToggleInputChatBox);
}

void ABlasterPlayerController::SetHUDTime()
{
	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
		}
	}
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime; //since GetServerTime() is very large we need to add the LevelStartingTime as well
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime; // InProgress warmup time, LevelStartingTime has already passed so we add to MatchTime
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority()) // This is done because the GetServerTime for the Server might be longer due to waiting in the lobby
	{ //Secondsleft is taken directly from the GameMode
		BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if(BlasterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}
	if(CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;	
}

void ABlasterPlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += DeltaSeconds;
	if(IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->HighPingImage &&
				BlasterHUD->CharacterOverlay->HighPingAnim;
	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.0f);
		BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingAnim, 0.f, 5);
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->HighPingImage &&
				BlasterHUD->CharacterOverlay->HighPingAnim;
	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if(BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingAnim))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingAnim);
		}
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode*  GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if(GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, LevelStartingTime, CooldownTime);
		// if(BlasterHUD && MatchState == MatchState::WaitingToStart)
		// {
		// 	BlasterHUD->AddAnouncement(); //Adding the announcement only when the player joins from the start
		// }
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float StartingTime, float Cooldown)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if(BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // The current client's time minus the time when the client request from the server
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime; //time of the server + half of the roundtriptime
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds(); // CurrentServerTime - CurrentClientTime
	
}

float ABlasterPlayerController::GetServerTime() // Time since start of the game
{
	if(HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer() // This will be called first by the system 
{
	Super::ReceivedPlayer();

	if(IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
	
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if(BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
		if (BlasterCharacter && BlasterCharacter->GetCombat())
		{
			SetHUDCarriedAmmo(BlasterCharacter->GetCombat()->GetCarriedAmmo());
			SetHUDWeaponAmmo(BlasterCharacter->GetCombat()->GetWeaponAmmo());
		}
	}
	HideElimmedText();
	SetHUDWeaponType(EWeaponType::EWT_Pistol);
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->HealthBar &&
				BlasterHUD->CharacterOverlay->HealthText;
	if(bHudValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->ShieldBar &&
				BlasterHUD->CharacterOverlay->ShieldText;
	if(bHudValid)
	{
		const float HealthPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(HealthPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->ScoreAmount;
	if(bHudValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->DefeatsAmount;
	if(bHudValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
				BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	if(bHudValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
				BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if(bHudValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDWeaponType(EWeaponType WeaponType)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
				BlasterHUD->CharacterOverlay->WeaponTypeText;
	if(bHudValid)
	{
		FText WeaponTypeText;
		switch (WeaponType)
		{
		case EWeaponType::EWT_Pistol:
			WeaponTypeText = FText::FromString("Pistol");
			break;
		case EWeaponType::EWT_AssaultRifle:
			WeaponTypeText = FText::FromString("Assault Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			WeaponTypeText = FText::FromString("Rocket Launcher");
			break;
		default:
			WeaponTypeText = FText::FromString("");
			break;
		}
		BlasterHUD->CharacterOverlay->WeaponTypeText->SetText(WeaponTypeText);
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
			BlasterHUD->CharacterOverlay->MatchCountdownText;
	if(bHudValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f); // Since CountdownTime is in seconds
		int32 Seconds = CountdownTime - Minutes * 60; //if CountdownTime is 61 seconds we want to get 1 second. Eg, 61 - 1 * 60 = 1
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); //%02d means only two characters and in case only one is available then pad with 0 
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->Announcement &&
			BlasterHUD->Announcement->WarmupTime;
	if(bHudValid)
	{
		if(CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f); // Since CountdownTime is in seconds
		int32 Seconds = CountdownTime - Minutes * 60; //if CountdownTime is 61 seconds we want to get 1 second. Eg, 61 - 1 * 60 = 1
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); //%02d means only two characters and in case only one is available then pad with 0 
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
				BlasterHUD->CharacterOverlay->GrenadesText;
	if(bHudValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::ShowElimmedText()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
				BlasterHUD->CharacterOverlay->ElimmedText;
	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->ElimmedText->SetVisibility(ESlateVisibility::Visible);
	}
}

void ABlasterPlayerController::HideElimmedText()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHudValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
				BlasterHUD->CharacterOverlay->ElimmedText;
	if(bHudValid)
	{
		BlasterHUD->CharacterOverlay->ElimmedText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State, bool bTeamsMatch)
{
	MatchState = State;
	if(MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted(bTeamsMatch);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}	
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if(MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted(bool bTeamsMatch)
{
	if (HasAuthority()) bShowTeamScores = bTeamsMatch; // this is for the clients to show or hide the team scores 
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if(BlasterHUD)
	{
		if(BlasterHUD->CharacterOverlay == nullptr)	BlasterHUD->AddCharacterOverlay(); // Add the character overlay only when it is null to avoid multiple addition of the HUD
		if(BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
		if (!HasAuthority()) return;
		
		if (bTeamsMatch)
		{
			InitTeamScores();
		}
		else
		{
			HideTeamScores();
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if(BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent(); //Remove the character overlay from the screen
		bool bHUDValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
				BlasterHUD->Announcement->InfoText;
		if(bHUDValid)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState  = GetPlayerState<ABlasterPlayerState>();
			if(BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText(BlasterGameState) : GetInfoText(TopPlayers);
				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if(BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGameplay = true; // This is to disable inputs
		BlasterCharacter->GetCombat()->FireButtonPressed(false); // This is to make sure that the weapon will not be fired upon death even when the fire button is already pressed
	}
}

FString ABlasterPlayerController::GetInfoText(const TArray<ABlasterPlayerState*>& Players)
{
	ABlasterPlayerState* BlasterPlayerState  = GetPlayerState<ABlasterPlayerState>();
	if (BlasterPlayerState == nullptr) return FString();
	
	FString InfoTextString;
	if(Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == BlasterPlayerState)
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1 && Players[0] != BlasterPlayerState)
	{
		InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *Players[0]->GetPlayerName());
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append(FString::Printf(TEXT("\n")));
		for (auto TiedPlayer : Players)
		{
			InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
		}
	}
	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText(ABlasterGameState* BlasterGameState)
{
	if (BlasterGameState == nullptr) return FString();

	FString InfoTextString;

	const int32 RedTeamScore = BlasterGameState->RedTeamScore;
	const int32 BlueTeamScore = BlasterGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf(TEXT("%s\n"), *Announcement::TeamsTiedForTheWin);
		InfoTextString.Append(Announcement::RedTeam);
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(Announcement::BlueTeam);
		InfoTextString.Append(TEXT("\n"));
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::RedTeam, RedTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::BlueTeam, BlueTeamScore));
	}
	else if (BlueTeamScore > RedTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append(TEXT("\n"));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::BlueTeam, BlueTeamScore));
		InfoTextString.Append(FString::Printf(TEXT("%s : %d\n"), *Announcement::RedTeam, RedTeamScore));
	}
	return  InfoTextString;	
}
