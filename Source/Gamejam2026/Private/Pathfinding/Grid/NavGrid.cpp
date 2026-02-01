/// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Grid/NavGrid.h"
#include "Pathfinding/Core/NavNode.h"
#include "Pathfinding/Management/NavigationSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

float InverseLerp(const float A, const float B, const float Value)
{
	/// Preventing possible division by 0 kind of situations.
	if (FMath::IsNearlyEqual(A, B))
	{
		return 0.0f;
	}
	/// Returning the inverse of the lerp, clamped to the range [0, 1]
	return FMath::Clamp((Value - A) / (B - A), 0.0f, 1.0f);
}

/// Sets default values
ANavGrid::ANavGrid()
{
 	/// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	GridSizeX = FMath::Clamp(GridSizeX, 1, GridSizeXMax);
	GridSizeY = FMath::Clamp(GridSizeY, 1, GridSizeYMax);

	Grid.SetNum(GridSizeX);
	for (int32 x = 0; x < GridSizeX; x++)
	{
		Grid[x].SetNum(GridSizeY);
	}
	
	NodeRadius = FMath::Clamp(NodeRadius, NodeRadiusMin, NodeRadiusMax);
	NodeDiameter = NodeRadius * 2;
	GridWorldSize = FVector2D(GridSizeX * NodeDiameter, GridSizeY * NodeDiameter);

	GridRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = GridRoot.Get();
	
	NodeVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridVisuals"));
	NodeVisuals->SetupAttachment(RootComponent);
	NodeVisuals->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	NodeVisuals->NumCustomDataFloats = 3;
	
	EdgeVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EdgeVisuals"));
	EdgeVisuals->SetupAttachment(RootComponent);
	EdgeVisuals->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	EdgeVisuals->NumCustomDataFloats = 3;
	
	PrevGridSizeX = GridSizeX;
	PrevGridSizeY = GridSizeY;
	PrevNodeRadius = NodeRadius;
	PrevResolutionMode = ResolutionMode;
	
	bGridCreated = false;
}

void ANavGrid::ValidateGridParameters()
{
	/// clamp node radius but do NOT rewrite GridWorldSize
	NodeRadius = FMath::Clamp(NodeRadius, NodeRadiusMin, NodeRadiusMax);
	NodeDiameter = NodeRadius * 2.f;

	switch (ResolutionMode)
	{
	case EGridResolutionMode::ByWorldSize:
		{
			/// GridWorldSize is fixed; node size is fixed → node count is derived
			GridSizeX = FMath::Clamp(FMath::FloorToInt(GridWorldSize.X / NodeDiameter), 1, GridSizeXMax);
			GridSizeY = FMath::Clamp(FMath::FloorToInt(GridWorldSize.Y / NodeDiameter), 1, GridSizeYMax);
			break;
		}

	case EGridResolutionMode::ByNodeCount:
		{
			/// Node count is fixed; node size is fixed → world size is derived
			GridSizeX = FMath::Clamp(GridSizeX, 1, GridSizeXMax);
			GridSizeY = FMath::Clamp(GridSizeY, 1, GridSizeYMax);
			
			GridWorldSize.X = GridSizeX * NodeDiameter;
			GridWorldSize.Y = GridSizeY * NodeDiameter;
			break;
		}

	case EGridResolutionMode::Manual:
		{
			/// User sets everything
			GridSizeX = FMath::Clamp(GridSizeX, 1, GridSizeXMax);
			GridSizeY = FMath::Clamp(GridSizeY, 1, GridSizeYMax);
			
			/// 2. Compute node diameter from world size + count
			const float IdealDiameterX = GridWorldSize.X / GridSizeX;
			const float IdealDiameterY = GridWorldSize.Y / GridSizeY;

			NodeDiameter = FMath::RoundToInt(FMath::Min(IdealDiameterX, IdealDiameterY));
			NodeRadius = NodeDiameter * 0.5f;
			
			break;
		}
	}

	WalkableRegionsMap.Empty();
	for (const FTerrainType& Region : WalkableRegions)
	{
		WalkableRegionsMap.Add(Region.CollisionChannel, Region.TerrainPenalty);
	}
}

void ANavGrid::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	RegisterGridWithNavSubsystem();
	ValidateGridParameters();
	
	/// Check if any of the below properties have changed compared to previous render, if different -> then compute new.
	const bool bShouldRecreate =
		!bGridCreated ||
		PrevGridSizeX != GridSizeX ||
		PrevGridSizeY != GridSizeY ||
		!FMath::IsNearlyEqual(PrevNodeRadius, NodeRadius) ||
		PrevResolutionMode != ResolutionMode;
	
	if (bShouldRecreate)
	{
		CreateGrid();
		PrevGridSizeX = GridSizeX;
		PrevGridSizeY = GridSizeY;
		PrevNodeRadius = NodeRadius;
		PrevResolutionMode = ResolutionMode;
	}
	else
	{
		/// FIXME: Is the why grids are locked into place in the Editor? Why they can't be moved with gizmos.
		GridRoot->SetWorldLocation(Transform.GetLocation());
		UpdateNodeTransforms();
		// UpdateEdges();
	}
}

void ANavGrid::RegisterGridWithNavSubsystem()
{
	if (bRegisteredWithNavSubsystem) return; 
	
	/// Every NavGrid lets the UWorldSubsystem know that they exist.
	if (const UWorld* World = GetWorld())
	{
		if (UNavigationSubsystem* NavSubsystem = World->GetSubsystem<UNavigationSubsystem>())
		{
			NavSubsystem->InitializeNavGrid();
			bRegisteredWithNavSubsystem = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BeginPlay: No UNavigationSubsystem found in World."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginPlay: No World found."));
	}
}

void ANavGrid::BeginPlay()
{
	Super::BeginPlay();
	
	RegisterGridWithNavSubsystem(); // Runs in both OnConstruction and BeginPlay(), just for good measure. Is cached.
	ValidateGridParameters();
	CreateGrid();
}

void ANavGrid::CreateGrid()
{
    UE_LOG(LogTemp, Log, TEXT("Creating Grid..."));
    UE_LOG(LogTemp, Log, TEXT("Grid Size: %d x %d"), GridSizeX, GridSizeY);
	
	/// STEP 1: Clear previous instances and reserve space inside arrays and GridVisualsComponents.
    ResetGrid();
	
    /// STEP 2: allocate inner arrays & create nodes (no neighbour reads)
    GenerateNavGridNodes();

    /// STEP 3: build edges (now every inner row is allocated so neighbor reads are safe)
    // GenerateNavGridEdges();
	
	/// STEP 4: Node Visuals - now use InverseTransformPosition(Node->WorldPosition) (WorldPos now is set)
    GenerateNodeVisuals();
	
    /// STEP 5: Edge Visuals - Much like Node Visuals
	// GenerateEdgeVisuals();
	
    /// STEP 6: Post-process blurring.
    BlurPenaltyMap(BlurPenaltySize);
	
	bGridCreated = true;
    UE_LOG(LogTemp, Log, TEXT("CreateGrid finished: %d Nodes, %f, %f GridWorldSize"), AllNodes.Num(), GetGridWorldSize().X, GetGridWorldSize().Y);
	UE_LOG(LogTemp, Log, TEXT("WalkableNodes %d / %d"), WalkableNodes.Num(), AllNodes.Num());
}

void ANavGrid::ResetGrid()
{
	/// Ensure outer array has GridSizeX rows
	Grid.SetNum(GridSizeX);
	
	AllNodes.Empty();
	AllNodes.Reserve(GridSizeX * GridSizeY);
	
	WalkableNodes.Empty();
	WalkableNodes.Reserve(GridSizeX * GridSizeY);
	
	AllEdges.Empty();
	AllEdges.Reserve(GridSizeX * GridSizeY * 8);
	
	/// Clear visuals early (safe to call even if nullptr)
	if (NodeVisuals)
	{
		NodeVisuals->ClearInstances();
	}
	if (EdgeVisuals)
	{
		EdgeVisuals->ClearInstances();
	}
}

void ANavGrid::GenerateNavGridNodes()
{
	for (int x = 0; x < GridSizeX; ++x)
	{
		/// make sure the rows are sized before any node lookup
		Grid[x].SetNum(GridSizeY); 

		for (int y = 0; y < GridSizeY; ++y)
		{
			/// Find the vectorPos of the current node we are checking.
			const FVector LocalCell =
				FVector(x * NodeDiameter + NodeRadius,
				        y * NodeDiameter + NodeRadius,
				        0.f);

			/// Converts local cell position to world position.
			const FVector WorldPoint = GetActorTransform().TransformPosition(LocalCell);
			
			/// Construct node in-place, with 0 values for now.
			Grid[x][y] = FNavNodeInternal(true, WorldPoint, x, y, 0);
			FNavNodeInternal* Node = &Grid[x][y];
			
			PerformZTraceAndUpdateNode(Node);
			if (Node->bWalkable)
			{
				WalkableNodes.Add(Node);
			}
			
			AllNodes.Add(Node);
		}
	}
}

void ANavGrid::GenerateNavGridEdges()
{
	for (int x = 0; x < GridSizeX; ++x)
	{
		for (int y = 0; y < GridSizeY; ++y)
		{
			FNavNodeInternal* Node = &Grid[x][y];
			Node->GetEdges().Empty();

			TArray<FNavNodeInternal*> Neighbors = GetNeighbors(Node);

			for (FNavNodeInternal* Neighbor : Neighbors)
			{
				/// Create the edge, and assign and adjust variables according to HeightDelta.
				FNavEdge Edge;
				Edge.OwnerNode = Node;
				Edge.NeighborNode = Neighbor;
					
				Edge.EdgeWorldPosition = Node->WorldPosition + (Neighbor->WorldPosition - Node->WorldPosition) * 0.5f;
				Edge.EdgeDirection = (Neighbor->WorldPosition - Node->WorldPosition).GetSafeNormal();
				Edge.EdgeLength = (Neighbor->WorldPosition - Node->WorldPosition).Size();
            	
				const float HeightDelta = Neighbor->WorldPosition.Z - Node->WorldPosition.Z;
				Edge.bAllowedForward  = HeightDelta <= MaxEdgeClimbLength;
				Edge.bAllowedBackward = -HeightDelta <= MaxEdgeDropLength;

				Node->GetEdges().Add(Edge);
            	
				AllEdges.Add(Edge);
			}
			
			/*
			/// Checking nearby neighbours in a 3x3 square around current node.
			for (int GridNeighborXOffset = -1; GridNeighborXOffset <= 1; ++GridNeighborXOffset)
			{
				for (int GridNeighborYOffset = -1; GridNeighborYOffset <= 1; ++GridNeighborYOffset)
				{
					/// If [0,0] then we are checking current node, which is not a neighbor.
					if (GridNeighborXOffset == 0 && GridNeighborYOffset == 0)
					{
						continue;
					}
					
					/// Find neighbors to current grid node
					const int GridNeighborXPos = x + GridNeighborXOffset;
					const int GridNeighborYPos = y + GridNeighborYOffset;

					/// Bounds guard to avoid crashing
					if (GridNeighborXPos < 0 || GridNeighborYPos < 0 || GridNeighborXPos >= GridSizeX || GridNeighborYPos >= GridSizeY)
					{
						continue;
					}
					
					FNavNodeInternal* Neighbor = &Grid[GridNeighborXPos][GridNeighborYPos];
            	
					/// If neighboring node is unwalkable, don't bother creating edges.
					if (!Neighbor->bWalkable)
					{
						continue;
					}

					/// TODO: Is this where we ensure edges have the right FVector position?
					/// Calculate distance from node world z-pos to neighbour world z-pos.
					const float HeightDelta = Neighbor->WorldPosition.Z - Node->WorldPosition.Z;

					/// Create the edge, and assign and adjust variables according to HeightDelta.
					FNavEdge Edge;
					Edge.OwnerNode = Node;
					Edge.NeighborNode = Neighbor;
					
					Edge.EdgeWorldPosition = Node->WorldPosition + (Neighbor->WorldPosition - Node->WorldPosition) * 0.5f;
					Edge.EdgeDirection = (Neighbor->WorldPosition - Node->WorldPosition).GetSafeNormal();
					Edge.EdgeLength = HeightDelta;
            	
					Edge.bAllowedForward = Edge.EdgeLength <= MaxEdgeClimbLength;
					Edge.bAllowedBackward = -Edge.EdgeLength <= MaxEdgeDropLength;

					Node->GetEdges().Add(Edge);
            	
					AllEdges.Add(Edge);
				}
			}*/
		}
	}
}

void ANavGrid::GenerateNodeVisuals()
{
	if (!NodeVisuals) { return; }
	
	NodeVisuals->ClearInstances();
	
	/// Protect against near-zero NodeDiameter (avoid tiny scales that risk crashing the ISMC (Instanced Static Mesh Component))
	const float SafeNodeDiameter = FMath::Max(NodeDiameter, 1.f); /// 1 unit minimum
	FBoxSphereBounds Bounds;
	if (NodeVisuals->GetStaticMesh() != nullptr)
	{
		Bounds = NodeVisuals->GetStaticMesh()->GetBounds();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NodeVisuals mesh is null, cannot compute bounds."));
		return;
	}

	for (int x = 0; x < GridSizeX; ++x)
	{
		for (int y = 0; y < GridSizeY; ++y)
		{
			constexpr float GapFactor = 0.9f;
			FNavNodeInternal* Node = &Grid[x][y];

			/// InverseTransformPosition(..) converts from world position to local position
			const FVector LocalInstancePos = NodeVisuals->GetComponentTransform().InverseTransformPosition(
				Node->WorldPosition);

			const float MeshSize = Bounds.BoxExtent.X * 2.f; // width of mesh in X
			const float TargetSize = SafeNodeDiameter * GapFactor;

			/// If we are displaying edge visuals, node visuals need to be halved in size.
			const float Scale = bShowEdgeVisuals ? (TargetSize / MeshSize) * 0.5f : TargetSize / MeshSize;
			
			FTransform InstTransform(FRotator::ZeroRotator, LocalInstancePos, FVector(Scale));
			
			/// Store the nodeInstanceIndex onto each node, created from AddInstance(FTransform).
			Node->SetNodeInstanceIndex(NodeVisuals->AddInstance(InstTransform));
			const int32 InstanceIndex = Node->NodeInstanceIndex;

			const FLinearColor NodeColor = Node->bWalkable ? WalkableNodeColor : BlockedNodeColor;

			/// call SetCustomDataValue for each float; the last call can be true for marking dirty
			NodeVisuals->SetCustomDataValue(InstanceIndex, 0, NodeColor.R, false);
			NodeVisuals->SetCustomDataValue(InstanceIndex, 1, NodeColor.G, false);
			NodeVisuals->SetCustomDataValue(InstanceIndex, 2, NodeColor.B, false);
		}
	}
	
	NodeVisuals->SetVisibility(bShowNodeVisuals);
	
	/// Mark render state dirty after adjusting the grid visuals
	NodeVisuals->MarkRenderStateDirty();
}

void ANavGrid::GenerateEdgeVisuals()
{
	if (!EdgeVisuals) return; 
	EdgeVisuals->ClearInstances();

	if (!bShowEdgeVisuals) return;
	
	const UStaticMesh* Mesh = EdgeVisuals->GetStaticMesh(); 
	
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerateEdgeVisuals: EdgeVisuals mesh is null."));
		return;
	}
	
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector Extents = Bounds.BoxExtent; // half-sizes

	for (FNavNodeInternal* Node : AllNodes)
	{
		for (int i = 0; i < Node->GetEdges().Num(); ++i)
		{
			FNavEdge Edge = Node->GetEdges()[i];
			UE_LOG(LogTemp, Warning, TEXT("Alo, I'm visualizing an edge 'ere."))
			UE_LOG(LogTemp, Warning, TEXT("GridX: %d, GridY: %d, Neighbors: %d, EdgeWorldPos: (%d, %d, %d), Edge: %d / %d"), 
				Node->GridX, Node->GridY, GetNeighbors(Node).Num(), (int)Edge.EdgeWorldPosition.X, (int)Edge.EdgeWorldPosition.Y, 
				(int)Edge.EdgeWorldPosition.Z, i+1, Node->GetEdges().Num());
	 
			FRotator EdgeRotation = UKismetMathLibrary::MakeRotFromX(Edge.EdgeDirection);
			FVector EdgeMeshScale(Edge.EdgeLength / (Extents.X*2), 1.f, 1.f);
	 
			// FTransform EdgeTransform(EdgeRotation, Edge.EdgeWorldPosition, EdgeMeshScale);
			
			const FTransform& CompTransform = EdgeVisuals->GetComponentTransform();

			const FVector LocalPos = CompTransform.InverseTransformPosition(Edge.EdgeWorldPosition);
			const FQuat LocalQuat = CompTransform.InverseTransformRotation(EdgeRotation.Quaternion());

			FTransform EdgeTransform(LocalQuat, LocalPos, EdgeMeshScale);

			const int32 InstanceIndex = EdgeVisuals->AddInstance(EdgeTransform);
			
			/// 3. Check walkability and adjust edge color.
			FLinearColor EdgeColor;
			if (!Edge.bAllowedForward || Node->bWalkable == false)
			{
				EdgeColor = BlockedEdgeColor;
			}
			else if (!Edge.bAllowedBackward)
			{
				EdgeColor = OneWayEdgeColor;
			}
			else
			{
				EdgeColor = WalkableEdgeColor;
			}
			
			/// call SetCustomDataValue for each float; the last call can be true for marking dirty
			EdgeVisuals->SetCustomDataValue(InstanceIndex, 0, EdgeColor.R, false);
			EdgeVisuals->SetCustomDataValue(InstanceIndex, 1, EdgeColor.G, false);
			EdgeVisuals->SetCustomDataValue(InstanceIndex, 2, EdgeColor.B, false);
		}
	}

	EdgeVisuals->SetVisibility(bShowEdgeVisuals);
	
	/// Mark render state dirty after adjusting the grid visuals
	EdgeVisuals->MarkRenderStateDirty();
}

void ANavGrid::UpdateNodeTransforms()
{
	if (!NodeVisuals) { return; }
	
	NodeVisuals->SetVisibility(bShowNodeVisuals);
	
	// Precompute some stable values used to compute instance scale (same logic as in GenerateNodeVisuals)
	const float SafeNodeDiameter = FMath::Max(NodeDiameter, 1.f);
	constexpr float GapFactor = 0.9f;
	const bool bHalfWhenEdgeVisuals = bShowEdgeVisuals;

	// Cache mesh bounds width (avoid calling GetBounds per node)
	const float MeshSize = NodeVisuals->GetStaticMesh() ? NodeVisuals->GetStaticMesh()->GetBounds().BoxExtent.X * 2.f : 100.f;
	const float TargetSize = SafeNodeDiameter * GapFactor;

	for (FNavNodeInternal* Node : AllNodes)
	{
		// Run new traces and update the node data.
		PerformZTraceAndUpdateNode(Node);
			
		/// If we are not displaying NodeVisuals, then we just continue.
		if (!bShowNodeVisuals)
		{
			continue;
		}
			
		// TODO: Do we need this check?
		// Validate instance index
		const int32 InstanceIndex = Node->GetNodeInstanceIndex(); // or Node->NodeInstanceIndex if public
		if (InstanceIndex < 0 || InstanceIndex >= NodeVisuals->GetInstanceCount())
		{
			continue;
		}
			
		/// Do we want to see nodes visually represented along the world, or as a flat floating grid?
		FVector LocalInstancePos = NodeVisuals->GetComponentTransform().InverseTransformPosition(Node->WorldPosition);
		if (bShowNodesFloored)
		{
			LocalInstancePos.Z = NodeVisuals->GetComponentTransform().InverseTransformPosition(Node->WorldPosition).Z;
		}
		else
		{
			LocalInstancePos.Z = GetActorLocation().Z;
		}
			
		// recompute per-instance scale to match GenerateNodeVisuals
		const float Scale = bHalfWhenEdgeVisuals ? (TargetSize / MeshSize) * 0.5f : (TargetSize / MeshSize);
		FTransform InstTransform(FRotator::ZeroRotator, LocalInstancePos, FVector(Scale));

		// Update transform in component-space (default UpdateInstanceTransform bWorldSpace = false)
		NodeVisuals->UpdateInstanceTransform(InstanceIndex, InstTransform, false, false);

		// Update color via custom data; mark dirty only after the last custom data value or after loop
		const FLinearColor NodeColor = Node->bWalkable ? WalkableNodeColor : BlockedNodeColor;
		NodeVisuals->SetCustomDataValue(InstanceIndex, 0, NodeColor.R, false);
		NodeVisuals->SetCustomDataValue(InstanceIndex, 1, NodeColor.G, false);
		NodeVisuals->SetCustomDataValue(InstanceIndex, 2, NodeColor.B, false);
	}
	
	/// Mark render state dirty after adjusting the grid visuals
	NodeVisuals->MarkRenderStateDirty();
}

void ANavGrid::UpdateEdges()
{
	GenerateEdgeVisuals();
}

void ANavGrid::BlurPenaltyMap(const int32 BlurSize)
{
	if (GridSizeX == 0 || GridSizeY == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridSizeX or GridSizeY is 0, cannot blur."));
		return;
	}
	const int32 KernelSize = BlurSize * 2 + 1;
	const int32 KernalExtents = (KernelSize - 1) / 2;

	/// Creating a 2D int grid for the PenaltiesHorizontalPass (...1 line in C# = 10 lines in C++)
	TArray<TArray<int32>> PenaltiesHorizontalPass;
	PenaltiesHorizontalPass.SetNum(GridSizeX); /// Set the number of rows
	for (int i = 0; i < GridSizeX; ++i)
	{
		PenaltiesHorizontalPass[i].SetNum(GridSizeY); /// Set the number of columns
		for (int j = 0; j < GridSizeY; ++j)
		{
			PenaltiesHorizontalPass[i][j] = 0; /// Initialize with a default value of 0 for each element
		}
	}

	/// Creating a new 2D int grid for the PenaltiesVerticalPass
	TArray<TArray<int32>> PenaltiesVerticalPass;
	PenaltiesVerticalPass.SetNum(GridSizeX); /// Set the number of rows
	for (int i = 0; i < GridSizeX; ++i)
	{
		PenaltiesVerticalPass[i].SetNum(GridSizeY); /// Set the number of columns
		for (int j = 0; j < GridSizeY; ++j)
		{
			PenaltiesVerticalPass[i][j] = 0; /// Initialize with a default value of 0 for each element
		}
	}

	for (int y = 0; y < GridSizeY; y++)
	{
		for (int x = -KernalExtents; x <= KernalExtents; x++)
		{
			const int32 SampleX = FMath::Clamp(x, 0, KernalExtents);
			PenaltiesHorizontalPass[0][y] += Grid[SampleX][y].GetMovementPenalty();
		}

		for (int x = 1; x < GridSizeX; x++)
		{
			const int32 RemoveIndex = FMath::Clamp(x - KernalExtents - 1, 0, GridSizeX);
			const int32 AddIndex = FMath::Clamp(x + KernalExtents, 0, GridSizeX - 1);

			PenaltiesHorizontalPass[x][y] = PenaltiesHorizontalPass[x - 1][y] - Grid[RemoveIndex][y].GetMovementPenalty() +
				Grid[AddIndex][y].GetMovementPenalty();
		}
	}
	for (int x = 0; x < GridSizeX; x++)
	{
		for (int y = -KernalExtents; y <= KernalExtents; y++)
		{
			const int32 SampleY = FMath::Clamp(y, 0, KernalExtents);
			PenaltiesVerticalPass[x][0] += PenaltiesHorizontalPass[x][SampleY];
		}

		int32 BlurredPenalty = FMath::RoundToInt(static_cast<float>(PenaltiesVerticalPass[x][0]) / (KernelSize * KernelSize));
		Grid[x][0].SetMovementPenalty(BlurredPenalty);

		for (int y = 1; y < GridSizeY; y++)
		{
			const int32 RemoveIndex = FMath::Clamp(y - KernalExtents - 1, 0, GridSizeY);
			const int32 AddIndex = FMath::Clamp(y + KernalExtents, 0, GridSizeY - 1);

			PenaltiesVerticalPass[x][y] = PenaltiesVerticalPass[x][y - 1]
			- PenaltiesHorizontalPass[x][RemoveIndex]
			+ PenaltiesHorizontalPass[x][AddIndex];
			
			BlurredPenalty = FMath::RoundToInt(static_cast<float>(PenaltiesVerticalPass[x][y]) / (KernelSize * KernelSize));
			Grid[x][y].SetMovementPenalty(BlurredPenalty);

			
			if (BlurredPenalty > PenaltyMax)
			{
				PenaltyMax = BlurredPenalty;
			}
			if (BlurredPenalty < PenaltyMin)
			{
				PenaltyMin = BlurredPenalty;
			}
		}
	}
}

void ANavGrid::SetAgentCount(const int NewAgentCount)
{
	AgentCountOnGrid = NewAgentCount;
	
	/// TODO: Run functionality for removing active agents on this grid, probs with UNavigationSubsystem.
}

void ANavGrid::PerformZTraceAndUpdateNode(FNavNodeInternal* Node) const
{
	/// Robust vertical trace (use world-space XY, but trace from very high to very low Z)
	constexpr float TraceZ = 5000.f;
	const FVector TraceStart = Node->WorldPosition + FVector(0, 0, TraceZ);
	const FVector TraceEnd   = Node->WorldPosition - FVector(0, 0, TraceZ);
			
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	/// We want to detect and see if the physical material is walkable or not.
	QueryParams.bReturnPhysicalMaterial = true;
	/// avoid hitting the NavGrid actor itself
	QueryParams.AddIgnoredActor(this); 
			
	/// TODO: Possibly adjust below logic to adapt to more ECollisionChannels than just WorldStatic.
	FHitResult Hit;
	
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		if (Hit.PhysMaterial.IsValid())
        {
            if (SurfaceTypeUnwalkable == Hit.PhysMaterial.Get()->SurfaceType)
            {
                Node->bWalkable = false;
            }
            else
            {
                Node->bWalkable = true;
            }
        }
        else
        {
            // No phys material - decide a reasonable default = unwalkable.
            Node->bWalkable = false;
        }

        Node->FloorZ = Hit.ImpactPoint.Z;
        Node->WorldPosition.Z = Node->FloorZ + 1.f;
	}
	else
	{
		Node->bWalkable = false;
	}
}

FNavNodeInternal* ANavGrid::GetNodeFromWorldPoint(const FVector& WorldPosition)
{
	/// InverseLerp works great when we know the worldPosition but want to know where it fits in the grid
	/// InverseLamp already clamps the values.
	const float PercentX = InverseLerp(-GridWorldSize.X * 0.5f, GridWorldSize.X * 0.5f, WorldPosition.X);
	const float PercentY = InverseLerp(-GridWorldSize.Y * 0.5f, GridWorldSize.Y * 0.5f, WorldPosition.Y);

	/// Then we use the percentages to get the x and y values of the node, clamping again to avoid edge cases
	const int32 x = FMath::Clamp(FMath::FloorToInt(PercentX * GridSizeX), 0, GridSizeX - 1);
	const int32 y = FMath::Clamp(FMath::FloorToInt(PercentY * GridSizeY), 0, GridSizeY - 1);

	return &Grid[x][y];
}

FVector ANavGrid::GetRandomGridLocation(const bool bNodeBased) const
{
	/// A: Either return a location on a grid based on a random node
	if (bNodeBased)
	{
		/// If walkable nodes are null, return just the origin as a fallback.
		if (WalkableNodes.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("No walkable nodes in the grid, returning the grid origin."));
			return GetActorLocation();
		}

		const int32 RandomIndex = FMath::RandRange(0, WalkableNodes.Num() - 1);
		return WalkableNodes[RandomIndex]->GetWorldPosition();
	}
	
	/// B: Or return a random position inside the grid based on gridExtents (will not be bound to node positions)
	const FVector GridOrigin = GetActorLocation();
	const FVector GridExtent(GridWorldSize.X * 0.5f, GridWorldSize.Y * 0.5f, 0.f);

	const FVector RandPoint(
		FMath::FRandRange(-GridExtent.X, GridExtent.X),
		FMath::FRandRange(-GridExtent.Y, GridExtent.Y),
		0.f
	);
	
	UE_LOG(LogTemp, Log, TEXT("Grid Random Location: %s"), *RandPoint.ToString());
	return GridOrigin + RandPoint;
}

TArray<FNavNodeInternal*> ANavGrid::GetNeighbors(const FNavNodeInternal* Node)
{
	TArray<FNavNodeInternal*> Neighbors;

	const int X = Node->GridX;
	const int Y = Node->GridY;

	for (int DX = -1; DX <= 1; DX++)
	{
		for (int Dy = -1; Dy <= 1; Dy++)
		{
			if (DX == 0 && Dy == 0)
				continue;

			const int Nx = X + DX;
			const int Ny = Y + Dy;

			// Bounds check first
			if (Nx < 0 || Nx >= GridSizeX || Ny < 0 || Ny >= GridSizeY)
				continue;

			// Block diagonal corner-cutting
			if (DX != 0 && Dy != 0)
			{
				const bool bSide1 = Grid[X + DX][Y].bWalkable;
				const bool bSide2 = Grid[X][Y + Dy].bWalkable;

				if (!bSide1 || !bSide2)
					continue;
			}

			Neighbors.Add(&Grid[Nx][Ny]);
		}
	}

	return Neighbors;
}

TArray<FNavEdge> ANavGrid::GetWalkableEdges(FNavNodeInternal* Node)
{
	TArray<FNavEdge> WalkableEdges;

	for (const FNavEdge &Edge : Node->GetEdges())
	{
		if (Edge.bAllowedForward || Edge.bAllowedBackward)
		{
			WalkableEdges.Add(Edge);
		}
	}
	return WalkableEdges;
}
