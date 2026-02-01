/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PathRequestDelegate.h"
#include "Pathfinding/Actors/Runner.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "PathRequestManager.generated.h"

struct FPathRunnerAgent;
class UPathfindingAStar;
class ARunner;
/**
 * @brief Originally set to be just 1 manager for all runners in C# prototype, the current intended functionality is to have
 * 1 PathRequestManager per runner, requesting NavigationSubsystem for paths, with 1 PathfindingAStar per runner.
 * Per-runner interface layer with pathfinding, an AI runner's pathfinding client kinda.
 *
 * @details Intended to handle simple logic such as...
 * - Should I recalculate my path?;
 * - Is the player too close -> new route?;
 * - Am I near my next waypoint?, etc.
 * 
 * @note About 60% implemented? Functional, only optional features are missing for the above-described logic + testing.
 */
UCLASS()
class GAMEJAM2026_API UPathRequestManager : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TObjectPtr<UPathfindingAStar> Pathfinder;
	
	/// Reference to the runner this PathRequestManager is assigned to
	UPROPERTY()
	TWeakObjectPtr<ARunner> OwnerRunner;
	
	FPathRunnerAgent* OwningAgent = nullptr;
	
	FPathRequestCallback CurrentCallback;
	
	/// True while a path is being calculated
	bool bIsRequestActive = false;
	
	/// default constructor, trivial
	UPathRequestManager();

	/// Requests a path and cancels any current ongoing pathfinding calculation
	void RequestPath(const ARunner* Requester, const FVector& PathStart, const FVector& PathEnd, const FPathRequestCallback& Callback);

	/// Called when a path has been found (meaning a PathRequest is fulfilled)
	void OnPathFound(const TArray<FVector>& Waypoints, bool bSuccess);

	/// --- Getters and Setters: --- 
	
	ARunner* GetAssignedRunner() const { return OwnerRunner.Get(); }
	
	FVector GetRandomPositionAwayFromPlayer() const;
	
	UPathfindingAStar* GetPathFinder() const { return Pathfinder.Get(); }
	
	void SetPathfinder(UPathfindingAStar* InPathfinder) { Pathfinder = InPathfinder; };
	void SetOwnerRunner(ARunner* Runner) { OwnerRunner = Runner; };

	/// TODO: Extend functionality / checks for when the runner should be requesting new paths.
};
