// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Management/NavigationSubsystem.h"
#include "EngineUtils.h"
#include "Pathfinding/Grid/NavGrid.h"
#include "Pathfinding/Algorithms/PathfindingAStar.h"
#include "Pathfinding/Management/PathRequestManager.h" /// For the RequestPath method.
#include "TagGameMode.h"
#include "Kismet/GameplayStatics.h"

void UNavigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UNavigationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	/// Find and retrieve the NavGrid.
	/// TODO: Reverse this! Let the NavGrids event-based signal to the subsystem that they exist, by calling this function.
	InitializeNavGrid();

	if (!Grid)
	{
		UE_LOG(LogTemp, Error, TEXT("NavigationSubsystem: No NavGrid found in level!"));
		return;
	}
	
	/*if (!GlobalDijkstraMap.Get())
	{
		/// Add the GlobalDijkstraMap class - to be added into the... pathfinder?
		GlobalDijkstraMap = NewObject<UDijkstraGlobalMap>(this);
		UE_LOG(LogTemp, Log, TEXT("NavigationSubsystem: Initialized GlobalDijkstraMap"));
	}*/

	/// Creating a pool of RunnerAgents based on NumberOfRunnersAlwaysActive + 1 reserve
	const int32 PoolSize = NumberOfRunnersAlwaysActive + 1;
	RunnerAgents.Reserve(PoolSize);

	/// Create a FPathRunnerAgent for the amount of runners + 1, and store that in a pool
	for (int32 i = 0; i < PoolSize; ++i)
	{
		FPathRunnerAgent Agent;
		/// NewObject instead of SubObject (for actors/components) inside this Subsystem.
		const UPathRequestManager* PathRequestManager = Agent.RequestManager = NewObject<UPathRequestManager>(this);
		UE_LOG(LogTemp, Warning, TEXT("New Requester pointer: %p"), PathRequestManager);
		const UPathfindingAStar* Pathfinder = Agent.Pathfinder = NewObject<UPathfindingAStar>(this);
		UE_LOG(LogTemp, Warning, TEXT("New Pathfinder pointer: %p"), Pathfinder);

		/// Make sure each RequestManager have a reference to the pathfinder & grid
		Agent.RequestManager->SetPathfinder(Agent.Pathfinder);
		Agent.Pathfinder->SetNavGrid(GetNavGrid());
		
		Agent.bInUse = false;
		
		UE_LOG(LogTemp, Warning, TEXT("Created new runner agent %d with RequestManager: %s, and Pathfinder: %s"), 
			i, *PathRequestManager->GetName(), *Pathfinder->GetName());
		
		RunnerAgents.Add(MoveTemp(Agent));
	}
	
	UE_LOG(LogTemp, Warning, TEXT("NavigationSubsystem: Initialized with %d pooled runner agents. (%d + 1 reserve)"), PoolSize, NumberOfRunnersAlwaysActive);
}

void UNavigationSubsystem::InitializeNavGrid()
{
	// Get custom GameMode
	const ATagGameMode* GameMode = Cast<ATagGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode not found in UNavigationSubsystem::InitializeNavGrid."));
		return;
	}
	 
	// Get BP_NavGrid class reference from GameMode
	const TSubclassOf<ANavGrid> NavGridClass = GameMode->BP_NavGrid;
	if (!NavGridClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BP_NavGrid class not assigned in GameMode in UNavigationSubsystem::InitializeNavGrid."));
		return;
	}
	 
	// Search for existing NavGrid instance in the world
	/* ANavGrid* FoundNavGrid = nullptr;
	for (TActorIterator<ANavGrid> It(GetWorld()); It; ++It)
	{
		if (It->IsA(NavGridClass))
		{
			FoundNavGrid = *It;
			// UE_LOG(LogTemp, Log, TEXT("Found existing NavGrid instance in Navigation Subsystem: %s"), *FoundNavGrid->GetName());
			break;
		}
	}
	 
	if (FoundNavGrid)
	{
		Grid = FoundNavGrid;
		UE_LOG(LogTemp, Log, TEXT("Using existing NavGrid instance in Navigation Subsystem: %s"), *FoundNavGrid->GetName());
	}
	else
	{
		// Spawn new instance if none found
		const FTransform SpawnTransform = FTransform::Identity; // Or customize spawn location
		if (ANavGrid* SpawnedNavGrid = GetWorld()->SpawnActor<ANavGrid>(NavGridClass, SpawnTransform))
		{
			Grid = SpawnedNavGrid;
			UE_LOG(LogTemp, Log, TEXT("Spawned new NavGrid instance in NavigationSubsystem: %s"), *SpawnedNavGrid->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn NavGrid instance in UNavigationSubsystem::InitializeNavGrid."));
		}
	}*/
}

void UNavigationSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("NavigationSubsystem deinitializing"));
	
	for (const FPathRunnerAgent& Agent : RunnerAgents)
	{
		Agent.RequestManager->bIsRequestActive = false;
		Agent.Pathfinder->CancelPathfinding();
	}

	// Clear pooled runner agents
	RunnerAgents.Empty();
	
	if (Grid.Get() && Grid->IsValidLowLevel())
	{
		Grid->Destroy();
		Grid = nullptr;
	}

	GlobalDijkstraMap = nullptr;

	Super::Deinitialize();
}

void UNavigationSubsystem::RegisterRunner(ARunner* Runner)
{
	if (!Runner) return;

	// Find an unused agent OR create new
	FPathRunnerAgent* AssignedAgent = nullptr;

	for (FPathRunnerAgent& Agent : RunnerAgents)
	{
		if (!Agent.bInUse)
		{
			AssignedAgent = &Agent;
			break;
		}
	}

	if (!AssignedAgent)
	{
		FPathRunnerAgent NewAgent;
		RunnerAgents.Add(NewAgent);
		AssignedAgent = &RunnerAgents.Last();
	}

	AssignedAgent->bInUse = true;

	// Create manager + pathfinder
	AssignedAgent->RequestManager = NewObject<UPathRequestManager>(this);
	AssignedAgent->Pathfinder     = NewObject<UPathfindingAStar>(this);

	// Link them
	AssignedAgent->RequestManager->SetPathfinder(AssignedAgent->Pathfinder);
	AssignedAgent->RequestManager->SetOwnerRunner(Runner);

	// Inject into Runner
	Runner->PathRequestManager = AssignedAgent->RequestManager;
	Runner->PathRequestManager->SetPathfinder(AssignedAgent->Pathfinder);
	AssignedAgent->RequestManager->OwningAgent = AssignedAgent;

	// Give pathfinder the grid
	if (Grid)
	{
		AssignedAgent->Pathfinder->SetNavGrid(Grid);
	}

	UE_LOG(LogTemp, Warning, TEXT("NavigationSubsystem: Runner %s registered with PathRequestManager %s and Pathfinder %s"),
		   *Runner->GetName(),
		   *AssignedAgent->RequestManager->GetName(),
		   *AssignedAgent->Pathfinder->GetName());
}

FPathRunnerAgent* UNavigationSubsystem::AcquireRunnerAgent()
{
	for (FPathRunnerAgent& Agent : RunnerAgents)
	{
		if (!Agent.bInUse)
		{
			return &Agent;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("No available runner agents in NavigationSubsystem."));
	return nullptr;
}

void UNavigationSubsystem::ReleaseRunnerAgent(FPathRunnerAgent* Agent) const
{
	if (!Agent) return;
	
	Agent->bInUse = false;
	Agent->RequestManager->bIsRequestActive = false;
	Agent->RequestManager->OwningAgent = nullptr;
	Agent->RequestManager->OwnerRunner = nullptr;
	Agent->Pathfinder->CancelPathfinding();

	/// Spawn a replacement
	if (AGameModeBase* Gm = GetWorld()->GetAuthGameMode())
	{
		Cast<ATagGameMode>(Gm)->SpawnNewRunner();
	}
}

/*void UNavigationSubsystem::RequestPath(const UPathRequestManager* Requester, const FVector& Start, const FVector& Target, const FPathRequestCallback& Callback)
{
	/// Only proceed if the Requester can be sourced (in-case it has been deleted / destroyed, e.g. runner caught)
	if (!Requester) return;
	
	if (UPathfindingAStar* Pathfinder = Requester->GetPathFinder())
	{
		// UE_LOG(LogTemp, Warning, TEXT("Pathfinder found for Requester %s, now requesting path"), *Requester->GetAssignedRunner()->GetName());
		/// Clear any ongoing pathfinding
		Pathfinder->CancelPathfinding();
		Pathfinder->StartFindPath(Start, Target, Callback);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Pathfinder found for Requester %s"), *Requester->GetAssignedRunner()->GetName());
	}
}

void UNavigationSubsystem::CancelPathfinding(const UPathRequestManager* Requester)
{
	/// Cancel the current path only if currently processing a path and if it belongs to the current Requester 
	if (!Requester) return;

	if (UPathfindingAStar* Pathfinder = Requester->GetPathFinder())
	{
		Pathfinder->CancelPathfinding();
	}
}*/

FVector UNavigationSubsystem::GetRandomGridPosAwayFromPlayer(const float MinDistanceFromPlayer) const
{
	if (!Grid) return FVector::ZeroVector;
	
	const FVector PlayerPos = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation();

	/// 50 attempts
	for (int i = 0; i < 50; i++) 
	{
		FVector Candidate = Grid->GetRandomGridLocation(true);

		if (FVector::DistSquared(Candidate, PlayerPos) > MinDistanceFromPlayer * MinDistanceFromPlayer)
		{
			return Candidate;
		}
	}

	// fallback: just return any walkable node
	return Grid->GetRandomGridLocation(true);
}