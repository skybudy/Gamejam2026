/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HeapInterface.h"
// TODO: Why is this commented out? Not needed, I think?
// #include "NavNode.generated.h"

class FNavNodeInternal;
/**
 * @brief Represents a directed edge between two navigation nodes - allows for better path handling.
 * @details This struct defines the connection between
 *          two nodes in the grid graph. Edges store directional traversal rules,
 *          traversal costs, height deltas, the OwnerNode it comes from, and the NeighborNode it connects to.
 * @note UNUSED for the exam implementation, currently not working as intended!
 */
struct FNavEdge
{
	/// TODO: Rework the neighbor node storage from pointers to int32 indices (big performance increase)
	
	/// What node created this edge
	FNavNodeInternal* OwnerNode = nullptr;
	
	/// What node this edge connects to from the OwnerNode.
	FNavNodeInternal* NeighborNode = nullptr;
	
	/// The edge position in the world - stored here for debugging and other purposes.
	FVector EdgeWorldPosition = FVector::ZeroVector;
	
	/// The edge direction - stored here for debugging and other purposes.
	FVector EdgeDirection = FVector::ZeroVector;
	
	/// Length of the edge
	float EdgeLength = 0.0f;
	
	/// Does this edge allow traversal from OwnerNode to NeighborNode?
	bool bAllowedForward = true;
	
	/// Does this edge allow traversal from NeighborNode to OwnerNode?
	bool bAllowedBackward = true;
	
};

/**
 * @brief NavNode class which NavGrid uses to create a grid composed of nodes, which functions as a graph for pathfinding.
 * @details This internal class is in pure C++ (with a raw pointer!), since USTRUCTS cannot directly have template interfaces.
 * USTRUCT FNavNode retrieves values from this internal class, and C++ classes should use this internal class.
 * @note 95% done with implementation? Only testing remains.
 */
class GAMEJAM2026_API FNavNodeInternal : public TIHeapItem<FNavNodeInternal>
{
public:
	
	/// If the node is walkable or not.
	bool bWalkable = false;
	
	/// @brief Node's current position in the world space. 
	/// @details WorldPosition.Z never touches obstacles directly, is always at least 1.f above floorZ.
	FVector WorldPosition = FVector::ZeroVector;
	
	/// @brief The exact contact-point location during trace down-or-up from the node within the NavGrid onto an obstacle.
	float FloorZ = 0.0f;
	
	/// Which X-value this node has in the grid TArray<TArray<int32>>.
	int32 GridX = 0;
	
	/// Which Y-value this node has in the grid TArray<TArray<int32>>.
	int32 GridY = 0;
	
	/// Which movement penalty value this node has during PathfindingAStar calculations.
	int32 MovementPenalty = 0;
	
	/// For use with NodeVisuals inside NavGrid - we need to track which node instance we are on
	uint32 NodeInstanceIndex = 0;

private:
	
	/// Start Cost - PathfindingAStar, the lower the cost, the closer you are to the starting node.
	int32 GCost = INT_MAX;
	/// Heuristic / Goal Cost - PathfindingAStar, the lower the cost, the closer you are to the end goal node.
	int32 HCost = 0;
	
	/// We can't use TObjectPtr<T> since the class is pure C++. Actually, a 'safe' pointer; do not use new or delete.
	FNavNodeInternal* ParentNode = nullptr; 
	
	/// Initialized with edges to neighboring nodes inside NavGrid during CreateGrid().
	TArray<FNavEdge> Edges;
	int32 HeapIndex = 0;

public:
	/// Constructor
	FNavNodeInternal() = default;

	FNavNodeInternal(const bool bIsWalkable, const FVector& WorldPos, const int GridPosX, const int GridPosY, const int Penalty)
	{
		bWalkable = bIsWalkable;
		WorldPosition = WorldPos;
		GridX = GridPosX;
		GridY = GridPosY;
		MovementPenalty = Penalty;
	}

	virtual int32 CompareWith(const T& OtherNode) const override
	{
		/// Compare by FCost, then HCost, returns -1 if this is less than nodeToCompare, 1 if it is greater, 0 if equal
		int32 Compare = GetFCost() - OtherNode.GetFCost();
		if (Compare == 0)
		{
			Compare = HCost - OtherNode.GetHCost();
		}
		/// Lower FCost = higher priority (min-heap)
		return -Compare;
	}

	/// ---------- Getters and Setters: ----------

	FNavNodeInternal* GetNode() { return this; }
	const FNavNodeInternal* GetNode() const { return this; }

	FNavNodeInternal* GetParent() const { return ParentNode; }
	void SetParent(FNavNodeInternal* Parent) { ParentNode = Parent; }
	
	TArray<FNavEdge>& GetEdges() { return Edges; }
	
	int32 GetGCost() const { return GCost; }
	int32 GetHCost() const { return HCost; }
	int32 GetFCost() const { return GCost + HCost; }

	void SetGCost(const int32 Value) { GCost = Value; }
	void SetHCost(const int32 Value) { HCost = Value; }
	
	virtual int32 GetHeapIndex() const override { return HeapIndex; }
	virtual void SetHeapIndex(const int32 Index) override { HeapIndex = Index; }
	
	bool GetWalkable() const { return GetNode()->bWalkable; }
	void SetWalkable(const bool bNewValue) { GetNode()->bWalkable = bNewValue; }
	
	int32 GetNodeInstanceIndex() { return GetNode()->NodeInstanceIndex; }
	void SetNodeInstanceIndex(const uint32 NewIndex) { GetNode()->NodeInstanceIndex = NewIndex; }

	FVector GetWorldPosition() const { return GetNode()->WorldPosition; }
	void SetWorldPosition(const FVector& NewPos) { GetNode()->WorldPosition = NewPos; }

	int32 GetMovementPenalty() const { return GetNode()->MovementPenalty; }
	void SetMovementPenalty(const int32 NewValue) { GetNode()->MovementPenalty = NewValue; }
};

/**
 * Blueprint-facing wrapper for possible usage with any NON-UObject class, for BPs to safely find the position
 * of a node and e.g. play VFX.
 * 95% implemented, just requires testing.
 * TODO: Test with the Pathfinding stuff if necessary, for BP exposure and stuff.
 */
/*USTRUCT(BlueprintType)
struct KOOLKIDSGAME_API FNavNode
{
	GENERATED_BODY()

private:
	TSharedPtr<FNavNodeInternal> NodeInternal; /// Smart pointer for safety

public:
	FNavNode()
	{
		NodeInternal = MakeShared<FNavNodeInternal>();
	}

	/// Accessor functions for Blueprint and C++
	bool GetWalkable() const { return NodeInternal->bWalkable; }
	void SetWalkable(bool bNewValue) const { NodeInternal->bWalkable = bNewValue; }

	FVector GetWorldPosition() const { return NodeInternal->WorldPosition; }
	void SetWorldPosition(const FVector& NewPos) const { NodeInternal->WorldPosition = NewPos; }

	int32 GetMovementPenalty() const { return NodeInternal->MovementPenalty; }
	void SetMovementPenalty(const int32 NewValue) const { NodeInternal->MovementPenalty = NewValue; }

	/// If a blueprint needs further access, just retrieve the internal class and work from there.
	FNavNodeInternal* GetInternal() { return NodeInternal.Get(); }
	
	/// If a blueprint needs further access, just retrieve the internal class and work from there.
	const FNavNodeInternal* GetInternal() const { return NodeInternal.Get(); }
	
};*/

