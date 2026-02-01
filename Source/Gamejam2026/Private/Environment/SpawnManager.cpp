

#include "Environment/SpawnManager.h"
#include "Pathfinding/Grid/NavGrid.h"
#include "Engine/World.h"
#include "EngineUtils.h"                  // TActorIterator
#include "Kismet/GameplayStatics.h"

USpawnManager::USpawnManager()
{
	
}
ANavGrid* USpawnManager::FindNavGrid(UWorld* World) const
{
	// Find the first ANavGrid in the world.
	for (TActorIterator<ANavGrid> It(World); It; ++It)
	{
		ANavGrid* Grid = *It;
		if (Grid)
		{
			return Grid;
		}
	}

	// No grid found
	return nullptr;
}

bool USpawnManager::ValidateNavGrid(UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnManager: Invalid World pointer."));
		return false;
	}

	ANavGrid* Grid = FindNavGrid(World);
	if (!Grid)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnManager: No ANavGrid found in world. Make sure a NavGrid actor exists in the level."));
		return false;
	}

	// Extra checks: ensure the grid has nodes
	if (Grid->GetAllNodes().Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnManager: ANavGrid found, but it contains 0 nodes (GetAllNodes().Num() == 0)."));
		return false;
	}

	return true;
}

void USpawnManager::StartSpawning(UWorld* World, float IntervalSeconds, int32 MaxConcurrent, int32 InSpawnPerTick)
{

	// Update config
	SpawnInterval = FMath::Max(0.01f, IntervalSeconds);
	// Ensure we don't set multiple timers (clear existing first)
	World->GetTimerManager().ClearTimer(SpawnTimerHandle);

	// Bind the timer to call SpawnTick(World) repeatedly
	FTimerDelegate Del = FTimerDelegate::CreateUObject(this, &USpawnManager::SpawnTick, World);
	World->GetTimerManager().SetTimer(SpawnTimerHandle, Del, SpawnInterval, true);
	
}
void USpawnManager::StopSpawning(UWorld* World)
{
	World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	UE_LOG(LogTemp, Log, TEXT("USpawnManager::StopSpawning"));
}

void USpawnManager::SpawnTick(UWorld* World)
{
	// Resolve NavGrid (we want to spawn on nav nodes)
	ANavGrid* NavGridPtr = FindNavGrid(World);
	if (!NavGridPtr || NavGridPtr->GetAllNodes().Num() == 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("USpawnManager::SpawnTick - no navgrid or navgrid empty"));
		return;
	}
}

