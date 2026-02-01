/// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Algorithms/PathfindingAStar.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Pathfinding/Grid/NavGrid.h"
#include "Tasks/Task.h"
#include "Async/Async.h"

UPathfindingAStar::UPathfindingAStar()
{
	bShouldCancel.store(false);
}

void UPathfindingAStar::StartFindPath(const FVector& StartPos, const FVector& TargetPos, FPathRequestCallback Callback)
{
	if (!Grid.Get())
	{
		UE_LOG(LogTemp, Error, TEXT("PathfindingAStar: No Grid assigned!"));
		Callback.ExecuteIfBound({}, false);
		return;
	}
	
	/// Toggling the bool to false
	bShouldCancel.store(false);
	/// Copying these instances by value (for thread safety)
	const FVector Start = StartPos;
	const FVector End = TargetPos;
	
	/**
	 * Using UE::Tasks::Launch() over Async(...) for more heavyweight loads; better integration and concurrency management overall.
	 * Runs the code on a background thread outside the GameThread.
	 * UE_SOURCE_LOCATION helps the debug source the location to here where the code is called.
	 * When we Launch() an FTask, we check if a request to stop pathfinding has come in via checking bShouldCancel.
	 * If we can proceed, set bPathFound to false.
	 * When the function FindPath_Internal is done running, it sets the flag to true.
	 */
	UE::Tasks::FTask PathTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Start, End, Callback]()
	{
		if (bShouldCancel.load())
		{
			Callback.ExecuteIfBound({}, false);
			return;
		}
		
		bool bSuccess = false;
		TArray<FVector> Waypoints = FindPath_Internal(Start, End, bSuccess);

		/**
		 * AsyncTask helps us bring the results from the FindPath_Internal above back over to the GameThread
		 * where the runners are, so we can call the function OnPathFound() in ARunner.
		 * 
		 * Do note that MoveTemp is used to pass over the Waypoints to the GameThread from the background thread
		 * without having to first copy and then move the array (which would be expensive), since we don't need
		 * the Waypoints anymore on the background thread.
		 * 
		 * ExecuteIfBound calls the function if it has been bound on a runner.
		 */
		AsyncTask(ENamedThreads::GameThread, [Callback, Waypoints = MoveTemp(Waypoints), bSuccess]()
		{
			Callback.ExecuteIfBound(Waypoints, bSuccess);
		});
	});
}

TArray<FVector> UPathfindingAStar::FindPath_Internal(const FVector& StartPos, const FVector& TargetPos, bool& bPathSuccess)
{
	/// UE_LOG(LogTemp, Warning, TEXT("Finding path -> StartPos: %s, TargetPos: %s"), *StartPos.ToString(), *TargetPos.ToString());
	TArray<FVector> FinalWaypoints;
	bPathSuccess = false;
	
	/// If we can't find the grid, we can't find waypoints, so might as well return.
	if (!Grid.Get()) return FinalWaypoints;
	
	const TArray<FNavNodeInternal*>& AllNodes = Grid->GetAllNodes();
	
	/// Ensuring nodes get their costs set to proper defaults every time we run pathfinding. We don't want to run old values.
	for (FNavNodeInternal* Node : AllNodes)
	{
		Node->SetGCost(INT_MAX);
		Node->SetHCost(0);
		Node->SetParent(nullptr);
	}

	FNavNodeInternal* StartNode = Grid->GetNodeFromWorldPoint(StartPos);
	FNavNodeInternal* TargetNode = Grid->GetNodeFromWorldPoint(TargetPos);
	
	if (!StartNode || !TargetNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPath_Internal: StartNode or TargetNode is null."));
		return FinalWaypoints;
	}
	
	if (!TargetNode->GetWalkable())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindPath_Internal: Target node not walkable."));
		return FinalWaypoints;
	}
	
	// --- 3. Classic A* with TArray open list (THeap scrapped for now, due to time constraints) ---

    TArray<FNavNodeInternal*> OpenList;
    OpenList.Reserve(AllNodes.Num());

    TSet<FNavNodeInternal*> ClosedSet;

    StartNode->SetGCost(0);
    StartNode->SetHCost(GetDistance(StartNode, TargetNode));

    OpenList.Add(StartNode);

    while (OpenList.Num() > 0 && !bShouldCancel.load())
    {
        // 3.1 Pick a node in OpenList with the lowest F cost (G + H)
        int32 BestIndex = 0;
        FNavNodeInternal* CurrentNode = OpenList[0];

        for (int32 i = 1; i < OpenList.Num(); i++)
        {
            FNavNodeInternal* Candidate = OpenList[i];

            const int32 CurrentF  = CurrentNode->GetGCost() + CurrentNode->GetHCost();
            const int32 CandidateF = Candidate->GetGCost() + Candidate->GetHCost();

            if (CandidateF < CurrentF || (CandidateF == CurrentF && Candidate->GetHCost() < CurrentNode->GetHCost()))
            {
                BestIndex = i;
                CurrentNode = Candidate;
            }
        }

        // Remove current from the open list, put into closed
        OpenList.RemoveAt(BestIndex);
        ClosedSet.Add(CurrentNode);

        // Reached goal?
        if (CurrentNode == TargetNode)
        {
            bPathSuccess = true;
            break;
        }

        // 3.2 Process neighbors
        for (FNavNodeInternal* Neighbor : Grid->GetNeighbors(CurrentNode))
        {
            if (!Neighbor->GetWalkable() || ClosedSet.Contains(Neighbor))
                continue;

            const int32 NewGCost =
                CurrentNode->GetGCost() + GetDistance(CurrentNode, Neighbor) + Neighbor->GetMovementPenalty();

            const bool bNotInOpen = !OpenList.Contains(Neighbor);

            if (NewGCost < Neighbor->GetGCost() || bNotInOpen)
            {
                Neighbor->SetGCost(NewGCost);
                Neighbor->SetHCost(GetDistance(Neighbor, TargetNode));
                Neighbor->SetParent(CurrentNode);

            	// --- Cycle prevention ---
            	if (CurrentNode->GetParent() == Neighbor)
            	{
            		Neighbor->SetParent(nullptr);
            	}
            	
                if (bNotInOpen)
                {
                    OpenList.Add(Neighbor);
                }
            }
        }
    }
	
	if (bPathSuccess)
	{
		TArray<FNavNodeInternal*> Nodes = RetraceNodes(StartNode, TargetNode);
		
		TArray<FVector> Raw;
		Raw.Reserve(Nodes.Num());
		for (const FNavNodeInternal* Node : Nodes)
		{
			Raw.Add(Node->WorldPosition);
		}

		// Unity-style smoothing
		FinalWaypoints = SimplifyPath(Raw);
		
		// Debug
		for (int32 i = 0; i < FinalWaypoints.Num(); ++i)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Waypoint %d / %d at %s"), i + 1, FinalWaypoints.Num(), *FinalWaypoints[i].ToString());
		}
	
		return FinalWaypoints;
	}
	
	if (FinalWaypoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No waypoints found in FindPath_Internal!"));
	}
	
	return FinalWaypoints;
}

TArray<FNavNodeInternal*> UPathfindingAStar::RetraceNodes(FNavNodeInternal* StartNode, FNavNodeInternal* EndNode)
{
	TArray<FNavNodeInternal*> Nodes;
	FNavNodeInternal* Current = EndNode;

	int32 Safety = 0;

	while (Current && Current != StartNode)
	{
		Nodes.Add(Current);
		Current = Current->GetParent();

		if (++Safety > 8000)
		{
			UE_LOG(LogTemp, Error, TEXT("RetraceNodes: infinite loop."));
			break;
		}
	}

	Nodes.Add(StartNode);
	Algo::Reverse(Nodes);
	return Nodes;
}

void UPathfindingAStar::CancelPathfinding()
{
	bShouldCancel.store(true);
}

TArray<FVector> UPathfindingAStar::SimplifyPath(const TArray<FVector>& Raw)
{
	TArray<FVector> Waypoints;

	if (Raw.Num() == 0) return Waypoints;

	FVector2D OldDirection(0,0);

	Waypoints.Add(Raw[0]); 

	for (int32 i = 1; i < Raw.Num(); i++)
	{
		FVector2D NewDir =
			FVector2D(Raw[i].X - Raw[i-1].X, Raw[i].Y - Raw[i-1].Y).GetSafeNormal();

		if (NewDir != OldDirection)
		{
			Waypoints.Add(Raw[i]);
		}

		OldDirection = NewDir;
	}

	return Waypoints;
}

int32 UPathfindingAStar::GetDistance(const FNavNodeInternal* NodeA, const FNavNodeInternal* NodeB)
{
	const int32 DistanceX = FMath::Abs(NodeA->GridX - NodeB->GridX);
	const int32 DistanceY = FMath::Abs(NodeA->GridY - NodeB->GridY);
	
	/// Straight (horizontal/vertical) movement = 10
	/// Diagonal movement = 14 (~√2 * 10)
	/// Formula = 14 * min(Δx, Δy) + 10 * abs(Δx - Δy)
	/// Fast integer version of Euclidean distance on a grid.
	if (DistanceX > DistanceY)
		return 14 * DistanceY + 10 * (DistanceX - DistanceY);
	return 14 * DistanceX + 10 * (DistanceY - DistanceX);
}
