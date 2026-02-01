/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pathfinding/Core/NavNode.h"
#include "TerrainType.h"
#include "NavGrid.generated.h"

/**
 * @brief Grid Resolution Mode - a helper enum class to help change modes when tweaking and visualizing the grid in the Editor.
 * @details ByWorldSize -> Adjusts GridSizeX and GridSizeY based on GridWorldSize & NodeDiameter.
 * @details ByNodeCount -> Adjusts GridWorldSize based on GridSizeX & GridSizeY and NodeDiameter.
 * @details Manual -> Adjusts NodeDiameter based on GridSizeX, GridSizeY and GridWorldSize.
 * @note Used inside ValidateGridParameters().
 */
UENUM()
enum class EGridResolutionMode : uint8
{
	ByWorldSize,
	ByNodeCount,
	Manual
};

struct FTerrainType;
class UInstancedStaticMeshComponent;
class UNavigationSubsystem;

/// @brief InverseLerp returns a fraction between 2 points, based on a given value - returning where a point is between A and B.
/// @details InverseLamp also clamps the float value it returns between 0.0f and 1.0f.
static float InverseLerp(const float A, const float B, const float Value);

/**
 * @brief A navigational grid for all AI runners and pathfinding.
 * @details Uses ECollisionChannels and Penalties via FTerrainType, and stores Walkable terrain in a TMap.
 * @note 75% Implemented with only ECollisionChannels (LayerMask) and Physics (Trace Channels) remaining, and testing.
 */
UCLASS()
class GAMEJAM2026_API ANavGrid : public AActor
{
	GENERATED_BODY()
	
	/// ------ Structs and Components ------
	
	UPROPERTY(EditDefaultsOnly, Category="NavGrid")
	EGridResolutionMode ResolutionMode = EGridResolutionMode::ByNodeCount;
	EGridResolutionMode PrevResolutionMode = EGridResolutionMode::ByNodeCount;

	UPROPERTY(VisibleDefaultsOnly, Category="NavGrid")
	TObjectPtr<USceneComponent> GridRoot;
	
	UPROPERTY(VisibleDefaultsOnly, Category="NavGrid")
	TObjectPtr<UInstancedStaticMeshComponent> NodeVisuals;
	
	UPROPERTY(VisibleDefaultsOnly, Category="NavGrid")
	TObjectPtr<UInstancedStaticMeshComponent> EdgeVisuals;
	
	/// @brief The EPhysicalSurface number type (SurfaceType1). Stored to help with readability and adjustability. 
	/// @param 'SurfaceType1' 'Unwalkable'.
	EPhysicalSurface SurfaceTypeUnwalkable = SurfaceType1;

	/// Storing the nodeDiameter based on NodeRadius.
	float NodeDiameter;
	
	/// Used internally to check if transforms have changed in OnConstruction().
	float PrevNodeRadius = 100.0f;
	
	/// How far does the grid stretch across the x and y axes?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NavGrid", meta= (AllowPrivateAccess, ClampMin="1", ClampMax="100", UIMin="1", UIMax="100"))
	int32 GridSizeX = 10;
	int32 PrevGridSizeX = 10;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NavGrid", meta= (AllowPrivateAccess, ClampMin="1", ClampMax="100", UIMin="1", UIMax="100"))
	int32 GridSizeY = 10;
	int32 PrevGridSizeY = 10;
	
	/// Only used for clamping values, and should match the above UPROPERTY ClampMin and ClampMax values for GridSizeX and GridSizeY.
	int32 GridSizeXMax = 100;
	int32 GridSizeYMax = 100;
	
	/// PenaltyMin is used internally for a check and is set to the max int value for comparisons as a safeguard.
	int32 PenaltyMin = INT_MAX;
	/// PenaltyMin is used internally for a check and is set to the min int value for comparisons as a safeguard.
    int32 PenaltyMax = INT_MIN;
	
	/// Used internally to register when this grid is registered with the NavigationSubsystem.
	bool bRegisteredWithNavSubsystem = false;
	
	/// Used internally to track when the grid has been created.
	UPROPERTY(DuplicateTransient, BlueprintReadOnly, Category="NavGrid", meta=(AllowPrivateAccess, ExposeOnSpawn="true"))
	bool bGridCreated = false;

	/// TODO: Translate LayerMask stuff from Unity to UE - Implement ECollisionChannels and test them working
	/// private LayerMask _walkableMask;
	FTerrainType WalkableTerrainLayer;
	
	/// WalkableRegionsMap deals with 1. FTerrainLayers and 2. their TerrainPenalties for all WalkableRegions.
    TMap<int32, int32> WalkableRegionsMap;
	
	/// @brief The 2D Array (grid) of nodes. Contains all nodes, source of anything grid-referenced.
	/// @note If the TArray<TArray<FNavNodeInternal>> is a bit confusing, then think of it as 1 array storing a row of nodes,
	/// and the final array storing the arrays / rows with nodes.
    TArray<TArray<FNavNodeInternal>> Grid;
	

public:

	/// ------ Nodes ------
	
	/// Display InstancedStaticMeshes for the nodes of the graph?
	UPROPERTY(EditAnywhere, Category="NavGrid | Debug")
	bool bShowNodeVisuals = true;
	
	/// Display nodes when floored or without flooring towards obstacles?
	UPROPERTY(EditAnywhere, Category="NavGrid | Debug")
	bool bShowNodesFloored = true;
	
	/// Value is clamped between 10.f and 1000.f to avoid overflowing / impracticalities, floats round to int during run-time.
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | NavNode", meta=(ClampMin="10", ClampMax="1000.0", UIMin="10.0", UIMax="1000.0"))
	float NodeRadius = 100.0f;
	
	/// @brief Used only for clamping the NodeRadius minimum
	/// @note Remember to adjust this value to be the same inside the UPROPERTY of NodeRadius in NavGrid.h
	float NodeRadiusMin = 10.0f;
	
	/// @brief Used only for clamping the NodeRadius maximum
	/// @note Remember to adjust this value to be the same inside the UPROPERTY of NodeRadius in NavGrid.h
	float NodeRadiusMax = 1000.0f;
	
	/// Should be more pastel-like or less bright than the node colors
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | Debug")
	FColor WalkableNodeColor = FColor(0, 255, 0, 255);

	/// Should be more pastel-like or less bright than the node colors
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | Debug")
	FColor BlockedNodeColor = FColor(255, 00, 0, 255);
	
	/// ------ Edges ------
	
	/// Display InstancedStaticMeshes for the edges of the graph?
	UPROPERTY(EditAnywhere, Category="NavGrid | Debug")
	bool bShowEdgeVisuals = true;
	
	UPROPERTY(EditAnywhere, Category="NavGrid | Debug")
	float EdgeThickness = 5.0f;
	
	/// @brief Used to determine whether an edge is too long to be climbed / traversed. 
	/// @details If a node compared to its neighborNode has a really low z-value, the edge length becomes rather high, 
	/// indicating future troubles with ascending from node to neighbor.
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | NavEdge", meta=(ClampMin="10.0", ClampMax="10000.0", UIMin="10.0", UIMax="10000.0"))
	float MaxEdgeClimbLength = 50.f;
	
	/// @brief Used to determine whether an edge is too long to be traversed when dropping. 
	/// @details If a node compared to its neighborNode has a really high z-value, the edge length becomes rather high, 
	/// indicating future troubles with descending from node to neighbor.
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | NavEdge", meta=(ClampMin="10.0", ClampMax="10000.0", UIMin="10.0", UIMax="10000.0"))
	float MaxEdgeDropLength = 500.f;
	
	/// Should be more pastel-like or less bright than the node colors
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | Debug")
	FLinearColor WalkableEdgeColor = FLinearColor(0, 255, 40, 255);

	/// Should be more pastel-like or less bright than the node colors
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | Debug")
	FLinearColor OneWayEdgeColor = FLinearColor(200, 200, 0, 255);

	/// Should be more pastel-like or less bright than the node colors
	UPROPERTY(EditDefaultsOnly, Category="NavGrid | Debug")
	FLinearColor BlockedEdgeColor = FLinearColor(255, 20, 0, 255);
	
	/// ------ CollisionChannels ------
	
    /// TODO: Translate LayerMask stuff from Unity to UE - Implement ECollisionChannels / PhysicsMaterials
    /// LayerMask UnwalkableMask;
	ECollisionChannel UnWalkableTerrainLayer;
	TArray<FTerrainType> WalkableRegions;
	
	/// ------ NavGrid ------
	
	/// The Grid size in world units
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NavGrid")
    FVector2D GridWorldSize;
	
	/// Assign penalties around obstacles in the grid.
    int32 ObstacleProximityPenalty = 10;
	
	/// How many nodes out from the source node do you blur? 1, 3x3, 9x9, etc.
	int32 BlurPenaltySize = 2;
	
	/// ------ NavGrid Agents ------
	
	/// How many agents (runners) does this grid initialize with?
	int32 AgentCountOnGrid = 15;
	
	/// @brief Sets the number of agents running on this grid and removes agents currently active.
	/// @note Is run from ADebugManager whenever needed (when adjusting the total number of agents or local amount)
	void SetAgentCount(const int NewAgentCount);
	 
private:
	/// @brief Creates the Grid based on default values. Runs the following functions in order and turns bGridCreated = true:
	/// @note ResetGrid(), GenerateNavGridNodes(), GenerateNavGridEdges(), GenerateNodeVisuals(), GenerateEdgeVisuals(),
	/// BlurPenaltyMap(const int BlurSize).
    void CreateGrid();
	
	/// @brief Clears arrays, allocates new space, clears visual instances of GridVisuals and EdgeVisuals.
	/// @details Handles components and variables AllNodes, AllEdges, NodeVisuals, and EdgeVisuals.
	/// @note Is run as part of CreateGrid().
	void ResetGrid();
	
	/// @brief Generates the internal nodes within the NavGrid.
	/// @note Is run as part of CreateGrid().
	void GenerateNavGridNodes();
	
	/// @brief Generates the internal edges between grid nodes.
	/// @note Is run as part of CreateGrid().
	void GenerateNavGridEdges();
	
	/// @brief Generates static instanced meshes of the nodes.
	/// @note Is run as part of CreateGrid().
	void GenerateNodeVisuals();
	
	/// @brief Generates static instanced meshes of the edges between nodes, assuming instances have been cleared.
	/// @note Is run as part of CreateGrid().
	void GenerateEdgeVisuals();
	
	/// @brief Performs a Z-trace from high above the grid in a straight line on the z-axis, and floors node if enabled.
	void PerformZTraceAndUpdateNode(FNavNodeInternal* Node) const;

	/// @brief Runs and checks through each node and updates its data accordingly.
	/// @note Is run inside OnConstruction() and receives FTransform parameter there.
	void UpdateNodeTransforms();
	
	/// @brief Runs and checks through each edge and updates its data accordingly.
	/// @note Is run inside OnConstruction() and receives FTransform parameter there.
	void UpdateEdges();
	
	/// @brief Blurs the penalty map to help with path smoothing. Uses a box blur approach.
	/// @note Is run as part of CreateGrid().
	void BlurPenaltyMap(const int BlurSize);
	
	/// All nodes stored as pointer references here - retrieved from Grid, the single source of truth.
	TArray<FNavNodeInternal*> AllNodes;
	
	/// All walkable nodes stored as pointer references here, retrieved from Grid during BeginPlay and updated.
	TArray<FNavNodeInternal*> WalkableNodes;
	
	/// All edges stored as pointer references here - filled during CreateGrid() under GenerateNavGridEdges().
	TArray<FNavEdge> AllEdges;
	
public:	
	/// Sets default values for this actor's properties
	ANavGrid();
	
	/// Runs and checks current NodeRadius, NodeDiameter, etc. 
	/// Updates recent variables in order according to the current EGridResolutionMode.
	void ValidateGridParameters();
	
	/// Changes and run whenever the object is adjusted in the Editor
	virtual void OnConstruction(const FTransform& Transform) override;
	
	/// Register the NavGrid with the NavSubsystem.
	void RegisterGridWithNavSubsystem();

	/// Runs when play mode begins.
	virtual void BeginPlay() override;

	/// -------------------------------
	/// ----------- Getters: -----------
	/// -------------------------------
	
	/// Returns the most appropriate node based on WorldPosition
	FNavNodeInternal* GetNodeFromWorldPoint(const FVector& WorldPosition);

	/// @brief Returns an FVector to a random grid position. 
	/// @details Based on a random node and its location (has to be walkable too), or a random location based on somewhere inside the grid.x and .y. 
	/// @note Use the bool to return your desired outcome.
	FVector GetRandomGridLocation(const bool bNodeBased = true) const;
	
	/// Returns up to 8 possible neighbors from the NavGrid, in a 3x3 grid around the node passed to the method.
	TArray<FNavNodeInternal*> GetNeighbors(const FNavNodeInternal* Node);
	
	/// Returns all current walkable nodes, but can be empty if no walkable nodes are found.
	TArray<FNavNodeInternal*>& GetWalkableNodes() { return WalkableNodes; };
	
	/// Returns all walkable edges from a node, both 2-way or 1-way traversable edges
	TArray<FNavEdge> GetWalkableEdges(FNavNodeInternal* Node);
	
	/// Returns the total amount of nodes in the grid (GridSizeX * GridSizeY).
	int32 GetMaxSize() const {return GridSizeX * GridSizeY; }
	
	/// TODO: Adjust for 3D space with nodes having different z-heights.
	/// Returns a diagonal vector equal to the width and the length of the grid
	FVector2D GetGridWorldSize() const { return GridWorldSize; }
	
	/// Get all nodes (for systems that need to iterate)
	TArray<FNavNodeInternal*>& GetAllNodes() { return AllNodes; }
	const TArray<FNavNodeInternal*>& GetAllNodes() const { return AllNodes; }
};
