/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DebugManager.generated.h"

class ANavGrid;
class UNavigationSubSystem;

/**
 * @brief A manager class to globally toggle and adjust all NavGrids in a level, debugging / Editor functionalities.
 * @details Tracks all NavGrids and agents and talks to the UNavigationSubsystem that runs the numbers and pathfinding.
 */
UCLASS()
class GAMEJAM2026_API ADebugManager : public AActor
{
	GENERATED_BODY()
	
	TWeakObjectPtr<UNavigationSubSystem> NavigationSubsystem;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent", meta = (AllowPrivateAccess = "true"))
	int32 GlobalAgentCount = 0;
	
public:
	/// Sets default values for this actor's properties
	ADebugManager();
	
	/// Stores all found NavGrid actors in a level.
	UPROPERTY(EditAnywhere, Category="Grid")
	TArray<TWeakObjectPtr<ANavGrid>> AllNavGridActors;
	
	/// Toggle to turn on or off debug visuals. If this is false, none of the NavGrids will display any debug visuals.
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bShowAnyDebug = false;
	
	/// Toggle to turn on debug node visuals on the grids.
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bShowDebugNodes = false;
	
	/// Toggle to turn on debug line visuals on the grids.
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bShowDebugLines = false;
	
	/// Toggle to turn on debug path visuals on the grids, specifically the path an agent takes currently.
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bShowDebugPath = false;
	
protected:
	/// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
#if WITH_EDITOR
public:
	/// This is the function that listens for when Editor checkboxes are clicked
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
};