// Fill out your copyright notice in the Description page of Project Settings.

#include "TagGameState.h"
#include "Net/UnrealNetwork.h"

void ATagGameState::OnRep_CurrentTagger() const
{
	// Notify listeners (characters can bind to this)
	OnPlayerTaggerChange.Broadcast();
}

bool ATagGameState::IsTagger(APlayerState* Player) const
{
	if (!Player) return false;
	
	return CurrentTagger == Player;

	// --- OR ---
	// If we upgrade to multiple taggers later:
	// return CurrentTaggers.Contains(Player);
}

void ATagGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATagGameState, CurrentTagger);
}