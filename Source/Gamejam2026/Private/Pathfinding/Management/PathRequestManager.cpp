/// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Management/PathRequestManager.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Pathfinding/Management/NavigationSubsystem.h"
#include "Pathfinding/Algorithms/PathfindingAStar.h"

UPathRequestManager::UPathRequestManager()
{
}

void UPathRequestManager::RequestPath(const ARunner* Requester, const FVector& PathStart, const FVector& PathEnd, const FPathRequestCallback& Callback)
{
	if (!Pathfinder)
	{
		UE_LOG(LogTemp, Error, TEXT("PathRequestManager: Pathfinder is null!"));
		return;
	}
	
	if (!OwnerRunner.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("PathRequestManager: OwnerRunner is invalid!"));
		return;
	}
	
	/// Cancel any existing path first
	if (bIsRequestActive)
	{
		UE_LOG(LogTemp, Log, TEXT("Cancelling existing path request for %s"), *Requester->GetName());
		Pathfinder->CancelPathfinding();
		bIsRequestActive = false;
	}

	// Store callback safely
	CurrentCallback = Callback;
	bIsRequestActive = true;

	// Log
	UE_LOG(LogTemp, Verbose, TEXT("PathRequestManager: Starting path request for %s"), *OwnerRunner->GetName());

	// Begin the threaded pathfinding
	Pathfinder->StartFindPath(PathStart,PathEnd, FPathRequestCallback::CreateUObject(this, &UPathRequestManager::OnPathFound));
}

void UPathRequestManager::OnPathFound(const TArray<FVector>& Waypoints, const bool bSuccess)
{
	bIsRequestActive = false;
	
	if (!OwnerRunner.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("PathRequestManager: OwnerRunner destroyed before path returned!"));
		return;
	}
	
	// Deliver results to Runner via stored callback
	if (CurrentCallback.IsBound())
	{
		CurrentCallback.Execute(Waypoints, bSuccess);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PathRequestManager: StoredCallback was not bound!"));
	}
}

FVector UPathRequestManager::GetRandomPositionAwayFromPlayer() const
{
	UNavigationSubsystem* NavSubsystem = GetWorld()->GetSubsystem<UNavigationSubsystem>();
	if (!NavSubsystem) return FVector::ZeroVector;
	
	return NavSubsystem->GetRandomGridPosAwayFromPlayer();
}



