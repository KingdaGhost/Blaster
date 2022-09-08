// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
}

void ABlasterPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());
	if(CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime - GetServerTime());
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

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest; // The current client's time minus the time when the client request from the server
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime); //time of the server + half of the roundtriptime
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds(); // CurrentServerTime - CurrentClientTime
	
}

float ABlasterPlayerController::GetServerTime()
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
	}
	HideElimmedText();
	SetHUDWeaponType(EWeaponType::EWT_NoWeapon);
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
		UEnum::GetDisplayValueAsText<EWeaponType>(WeaponType, WeaponTypeText);
		if(WeaponType == EWeaponType::EWT_NoWeapon)
		{
			WeaponTypeText = FText::FromString("");
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
		int32 Minutes = CountdownTime / 60; // Since CountdownTime is in seconds
		int32 Seconds = CountdownTime - Minutes * 60; //if CountdownTime is 61 seconds we want to get 1 second. Eg, 61 - 1 * 60 = 1
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds); //%02d means only two characters and in case only one is available then pad with 0 
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
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


