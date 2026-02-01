/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Environment/SpawnManager.h"
#include "TagGameMode.generated.h"

class ANavGrid;
class ARunner;
class UNavigationSubsystem;
class AKoolKidsGameCharacter;

/**
 * 
 */
UCLASS()
class GAMEJAM2026_API ATagGameMode : public AGameMode
{
	GENERATED_BODY()

	TWeakObjectPtr<UNavigationSubsystem> NavigationSubsystem;
	int32 RunnersCaught = 0;

public:

	/// @brief Class template we spawn NavGrids from, to be able to use BP_NavGrid instead of pure C++ NavGrid.
	/// @details Use this BP_Edition if I'm planning to spawn grids dynamically in-game, otherwise stick to the C++ NavGrid.
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<ANavGrid> BP_NavGrid;
	
	/// TODO: The grids shouldn't be stored here in the GameMode when we can have multiple grids. Talk with NavigationSubsystem.
	
	/// The class itself which we can use functions from.
	/// TWeakObjectPtr<ANavGrid> NavGrid;
	
	/// @brief An array of NavGrid classes containing every placed NavGrid in the world.
	/// @details This C++ reference is preferred as long as we do not dynamically spawn grids in runtime.
	TWeakObjectPtr<ANavGrid> NavGrid;
	
	/// Class template we spawn runners from, to be able to use BP_Runners instead of pure C++ Runners.
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSubclassOf<ARunner> RunnerClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float SpawnHeightAboveGround = 300.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float MinSpawnDistanceFromPlayer = 750.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	float MaxSpawnDistanceFromPlayer = 3000.f;

	/// Time to add on each successful tag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	float CatchIncreaseTime=10;

	/// Total time in seconds
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "Timer")
	float RunTimer;
	
	/// Remaining time
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "Timer")
	float CurrentTimeRemaining;

	/// --- Functions ---
	ATagGameMode();
	
	/// Need to override BeginPlay to start the timer
	virtual void BeginPlay() override;

	/// Spawns a new Runner, accounting for playerPosition.
	void SpawnNewRunner() const;

	UFUNCTION(BlueprintCallable)
	void AddExtraTime();

private:
	/// Game Over Widget class
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameOverWidgetClass;
	
	/// Game Over Widget instance
	UUserWidget* GameOverWidget;
	
    /// Timer handle
    FTimerHandle TimerHandle;

    /// Function called every second
    void UpdateTimer();

    /// Called when time runs out
    void EndGame();


	UPROPERTY(EditAnywhere)
	TSubclassOf<USpawnManager> SpawnManagerClass;
	//Get the SpawnManager instance
	UPROPERTY()
	USpawnManager* SpawnManagerInstance;

public:

	FORCEINLINE int32 GetRunnersCaught() const { return RunnersCaught; }
};
