// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TagGameState.generated.h"

/**
 * 
 */
UCLASS()
class GAMEJAM2026_API ATagGameState : public AGameState
{
	GENERATED_BODY()

public:
	// Who is currently the tagger
	/**
	 * Represents the current tagger in the game.
	 *
	 * This variable holds a pointer to the player state of the player who is currently the tagger.
	 * It is replicated across the network to ensure all clients are aware of the current tagger.
	 * The tagger is updated through gameplay mechanics, such as when a player is tagged.
	 *
	 * Listeners can respond to changes in this variable by subscribing to the `OnPlayerTaggerChange` delegate.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentTagger)
	TObjectPtr<APlayerState> CurrentTagger;
	
	UFUNCTION()
	void OnRep_CurrentTagger() const;

	// Used for checking if a player is a tagger, but the logic is handled in this single, global gameState.
	UFUNCTION(BlueprintCallable)
	bool IsTagger(APlayerState* Player) const;

public:
	DECLARE_MULTICAST_DELEGATE(FOnPlayerTaggerChange)
	FOnPlayerTaggerChange OnPlayerTaggerChange;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
