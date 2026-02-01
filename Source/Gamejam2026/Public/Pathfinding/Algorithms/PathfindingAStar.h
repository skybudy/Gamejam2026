/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pathfinding/Core/NavNode.h"
#include "Pathfinding/Management/PathRequestDelegate.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "PathfindingAStar.generated.h"

class FNavNodeInternal;
class ANavGrid;

/**
 * @brief The main algorithmic class for doing PathfindingAStar, the 1 instanced solver of paths.
 * Kept as a UObject for the time being, to be able to run UI & Gizmos for debugging (maybe via a helper class).
 * @note 75% implemented, a coroutine yet to be translated, and then testing.
 */
UCLASS(NotBlueprintable, NotPlaceable)
class GAMEJAM2026_API UPathfindingAStar : public UObject
{
	GENERATED_BODY()

	/// A weak pointer to the shared global instance of ANavGrid. Avoid UPROPERTY with TWeakObjectPtr (error-prone).
	TWeakObjectPtr<ANavGrid> Grid;
	
	/// Set to 10 000 as default in the constructor.
	UPROPERTY(EditDefaultsOnly, Category="Pathfinding")
	int32 MaxHeapSize;

	/// A synchronous, blocking function that computes a path and returns results. 
	TArray<FVector> FindPath_Internal(const FVector& StartPos, const FVector& TargetPos, bool& bPathSuccess);
	
	TArray<FNavNodeInternal*> RetraceNodes(FNavNodeInternal* StartNode, FNavNodeInternal* EndNode);

	/// Atomic flag to safely request mid-run cancellation of a path. Using std::atomic since TAtomic<T> is deprecated.
	std::atomic<bool> bShouldCancel { false };

public:

	/// Default constructor, initializing MaxHeapSize and OpenSet. 
	UPathfindingAStar();

	/// Starts finding a path between the StartPos and TargetPos. 
    void StartFindPath(const FVector& StartPos, const FVector& TargetPos, FPathRequestCallback Callback);

	/// Flags the atomic bool BShouldCancel to true, which helps stops any current run of FindPath_Internal. 
	void CancelPathfinding();

	/// Simplifies and optimizes the path according to the NavGrid. 
	TArray<FVector> SimplifyPath(const TArray<FVector>& Raw);
	
	/// --- Setters and Getters ---
	
	/// To be used by the NavigationSubsystem to assign the shared NavGrid to the variable WeakObjectPtr Grid.
	void SetNavGrid(ANavGrid* NavGrid) { Grid = NavGrid; };

	/**
	* @brief Calculate the distance between 2 nodes on the grid.
	* @details Uses integer approximation of diagonal movement cost for A* pathfinding (common scaling convention with A*).
	* Specifics about the internal math can be found inside the function.
	*/
    static int32 GetDistance(const FNavNodeInternal* NodeA, const FNavNodeInternal* NodeB);
};
