// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pathfinding/Grid/NavGrid.h"
#include "PathRequest.h"
#include "Pathfinding/Management/PathRequestDelegate.h"
#include "Pathfinding/Actors/Runner.h"
#include "Subsystems/WorldSubsystem.h"
#include "NavigationSubsystem.generated.h"

class UPathRequestManager;
class UPathfindingAStar;
class UDijkstraGlobalMap;
class ANavGrid;

/**
 * @brief Collective struct for the runners in NavigationSubsystem, featuring the PathRequestManager and the PathfinderAStar.
 *
 * @details To be used in a pool for runners to have agents helping out with pathfinding, without having to delete and
 * re-allocate memory everytime a Runner is deleted / respawned.
 */
USTRUCT()
struct FPathRunnerAgent
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UPathRequestManager> RequestManager;
	
	UPROPERTY()
	TObjectPtr<UPathfindingAStar> Pathfinder;

	bool bInUse = false;
};

/**
 * @brief For handling several UObjects with Pathfinding, such as UPathRequestManager, UDijkstraGlobalMap and UPathfindingAStar.
 * @details Exists both client and server-side, and is created for each UWorld, and it is safe to call GetWorld() here.
 * More robust than AGameMode and acts as the global brain for pathfinding / navigation.
 * Using UWorldSubsystem as a base came from asking ChatGPT 5 if there are solutions in UE to global singletons in Unity.
 *
 * @note 65% Implemented - Functional? Only extended features are missing and testing is required.
 */
UCLASS()
class GAMEJAM2026_API UNavigationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/// RunnerAgents kept in a pool, to be assigned to each runner
	UPROPERTY()
	TArray<FPathRunnerAgent> RunnerAgents;

	/// CurrentRunners active in the scene
	UPROPERTY()
	TArray<ARunner*> CurrentRunners;

	/// TODO: Extend functionality with DijkstraGlobalMap and bias to the paths.
	/// 1 shared instance solving the DijkstraGlobalMap for runners based on PlayerLocation.
	UPROPERTY()
	TObjectPtr<UDijkstraGlobalMap> GlobalDijkstraMap;

	/// The C++ Version of the NavGrid
	UPROPERTY(EditAnywhere, Category="NavGrid")
	TObjectPtr<ANavGrid> Grid;
	
	/// The BP version of the NavGrid
	UPROPERTY(EditAnywhere, Category="Navigation")
	TSubclassOf<ANavGrid> BP_NavGrid;

	/// Determines how many runners are to be spawned and kept inside the level at all times.
	int32 NumberOfRunnersAlwaysActive = 100;
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	/// A function to ensure NavGrid has been found and initialized
	void InitializeNavGrid();
	
	/// For deinitializing components.
	virtual void Deinitialize() override;

	/// Assign and recycle agents for the runners
	void RegisterRunner(ARunner* Runner);
	FPathRunnerAgent* AcquireRunnerAgent();
	void ReleaseRunnerAgent(FPathRunnerAgent* Agent) const;

	/// Handles path requests from PathRequestManagers.
	// void RequestPath(const UPathRequestManager* Requester, const FVector& Start, const FVector& Target, const FPathRequestCallback& Callback);
	// void CancelPathfinding(const UPathRequestManager* Requester);

	/// Called to update the GlobalDijkstraMap every so often in accordance with the player position.
	void UpdateGlobalDijkstraMap(const FVector& PlayerPosition);

	UFUNCTION(BlueprintCallable, Category="Navigation")
	ANavGrid* GetNavGrid() const { return Grid; }
	
	FVector GetRandomGridPosAwayFromPlayer(float MinDistanceFromPlayer = 100.f) const;
	
	int32 GetNumberOfRunnersAlwaysActive() const { return NumberOfRunnersAlwaysActive; }
};