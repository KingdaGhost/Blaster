// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"

#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"


void ATeamsGameMode::PostLogin(APlayerController* NewPlayer) // This is for the players joining mid game
{
	Super::PostLogin(NewPlayer);
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		ABlasterPlayerState* BPState = NewPlayer->GetPlayerState<ABlasterPlayerState>(); // Getting the PlayerState of the new player
		if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam) // Checking to see if the player has no team to assign them a team
		{
			if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
			{
				BGameState->RedTeam.AddUnique(BPState); // Adding the BlasterPlayerState to Red Team  Array if Blue Team is >= Red Team
				BPState->SetTeam(ETeam::ET_RedTeam); // Setting that BlasterPlayerState to the Red Team
			}
			else
			{
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_BlueTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BPState = Exiting->GetPlayerState<ABlasterPlayerState>(); // Getting the PlayerState of the new player
	if (BGameState && BPState)
	{
		if (BGameState->RedTeam.Contains(BPState))
		{
			BGameState->RedTeam.Remove(BPState);
		}
		if (BGameState->BlueTeam.Contains(BPState))
		{
			BGameState->BlueTeam.Remove(BPState);
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		for (auto PState : BGameState->PlayerArray) //we can get all PlayerStates fromm PlayerArray from the BlasterGameState
		{
			ABlasterPlayerState* BPState = Cast<ABlasterPlayerState>(PState.Get()); // Getting the PlayerState of each player
			if (BPState && BPState->GetTeam() == ETeam::ET_NoTeam) // Checking to see if the players have no team to assign them a team
			{
				if (BGameState->BlueTeam.Num() >= BGameState->RedTeam.Num())
				{
					BGameState->RedTeam.AddUnique(BPState); // Adding the BlasterPlayerState to Red Team  Array if Blue Team is >= Red Team
					BPState->SetTeam(ETeam::ET_RedTeam); // Setting that BlasterPlayerState to the Red Team
				}
				else
				{
					BGameState->BlueTeam.AddUnique(BPState);
					BPState->SetTeam(ETeam::ET_BlueTeam);
				}
			}
		}
	}
}
