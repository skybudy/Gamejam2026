/// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Actors/Runner.h"
#include "GameJam2026Character.h"
#include "Pathfinding/Management/NavigationSubsystem.h"
#include "Pathfinding/Management/PathRequestManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/UObjectGlobals.h"

/// Sets default values
ARunner::ARunner()
{
 	/// Set this pawn to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	RootComponent = Capsule;
	Capsule->InitCapsuleSize(34.f, 88.f);
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Capsule->SetCollisionObjectType(ECC_Pawn);
	
	MoveDirection = FVector2D::ZeroVector;
}

/// Called when the game starts or when spawned
void ARunner::BeginPlay()
{
	Super::BeginPlay();
	
	InitialSpawnPosition = GetActorLocation();
	
	UNavigationSubsystem* NavSubsystem = GetWorld()->GetSubsystem<UNavigationSubsystem>();
	if (!NavSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Runner %s: NavigationSubsystem not found!"), *GetName());
		return;
	}

	/// Calls the BeginRepeatedPathUpdates with a little delay to account for level loading and stuff.
	/// GetWorldTimerManager().SetTimer(PathUpdateTimerHandle, this, &ARunner::UpdatePath, MinPathUpdateTime, true, MinPathUpdateTime);
	
	NavSubsystem->RegisterRunner(this);
	
	if (!PathRequestManager)
	{
		NavSubsystem->RegisterRunner(this);

		if (!PathRequestManager)
		{
			UE_LOG(LogTemp, Warning, TEXT("PathRequestManager is still null in ARunner::BeginPlay()!"));
			return;
		}
	}
	
	/// Binding a delegate to OnPathFound(), which is called when a path is found for this runner
	Callback.BindUObject(this, &ARunner::OnPathFound);
	
	// Choose first random target at start
	ChooseNewRandomTarget();
	RunnerTargetOld = RunnerTarget + FVector(99999,99999,99999);
	
	RequestNewPath();
	
	RunnerTargetOld = RunnerTarget;
	
	// update path every 0.2s like Unity
	GetWorldTimerManager().SetTimer(PathRequestTimerHandle, this,
		&ARunner::RequestNewPath, PathRequestCooldown, true);
}

void ARunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	FollowPath(DeltaTime);
}

void ARunner::OnRunnerCaught()
{
	UE_LOG(LogTemp, Warning, TEXT("Runner %s caught by player."), *GetName());
	
	Path.Reset();
	MoveDirection = FVector2D::ZeroVector;
	bIsFollowingPath = false;
	
	if (PathRequestManager)
	{
		if (const UNavigationSubsystem* NavSys = GetWorld()->GetSubsystem<UNavigationSubsystem>())
		{
			if (PathRequestManager->OwningAgent)
			{
				NavSys->ReleaseRunnerAgent(PathRequestManager->OwningAgent);
			}
		}
	}
	
	GetWorldTimerManager().ClearTimer(PathRequestTimerHandle);
	
	Destroy();
}

void ARunner::FellOutOfWorld(const UDamageType& DmgType)
{
	Super::FellOutOfWorld(DmgType);
	SetActorLocation(InitialSpawnPosition);
}

void ARunner::ChooseNewRandomTarget()
{
	UNavigationSubsystem* NavSys = GetWorld()->GetSubsystem<UNavigationSubsystem>();
	if (!NavSys) return;
	
	RunnerTarget = NavSys->GetRandomGridPosAwayFromPlayer();
	UE_LOG(LogTemp, Warning, TEXT("Runner %s picked new target: %s"), *GetName(), *RunnerTarget.ToString());
}

void ARunner::RequestNewPath()
{
	if (!PathRequestManager) return;

	// Detect target change
	const float DistSq = FVector::DistSquared(RunnerTarget, RunnerTargetOld);

	if (DistSq < FMath::Square(PathUpdateMoveThreshold))
		return;

	// REQUEST A STABLE PATH
	UE_LOG(LogTemp, Warning, TEXT("%s REQUESTING PATH to %s"), *GetName(), *RunnerTarget.ToString());

	PathRequestManager->RequestPath(
		this,
		GetActorLocation(),
		RunnerTarget,
		Callback
	);

	RunnerTargetOld = RunnerTarget;
}

void ARunner::StartFollowingPath()
{
	if (!Path.IsSet()) return;

	UE_LOG(LogTemp, Warning, TEXT("Runner %s: Starting to follow path."),
		*GetName());
}

/*void ARunner::UpdatePath()
{
	if (!PathRequestManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Runner %s: UpdatePath called but PathRequestManager invalid."), *GetName());
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastPathRequestTime < PathRequestCooldown)
	{
		return; // throttle requests
	}
	LastPathRequestTime = Now;
	
	// Only request a new path if RunnerTarget has moved significantly
	const float DistSq = FVector::DistSquared(RunnerTarget.GetLocation(), TargetPositionOld);
	if (DistSq < FMath::Square(PathUpdateMoveThreshold))
	{
		// Target hasn't changed much; no need to request new path
		return;
	}
	
	TargetPositionOld = RunnerTarget.GetLocation();

	PathRequestManager->RequestPath(
		this,
		GetActorLocation(),
		RunnerTarget.GetLocation(),
		Callback
	);
}*/

void ARunner::OnPathFound(const TArray<FVector>& Waypoints, const bool bSuccess)
{
	UE_LOG(LogTemp, Error, TEXT("Runner %s got %d waypoints."), *GetName(), Waypoints.Num());
	
	if (!bSuccess || Waypoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Path failed"), *GetName());
		return;
	}

	// --- CLEAN WAYPOINT LIST (remove starting duplicates) ---
	TArray<FVector> CleanedWaypoints = Waypoints;

	// Remove any initial points that equal the runner's current position (Unity analogue)
	while (CleanedWaypoints.Num() > 0 &&
		   FVector::Dist2D(CleanedWaypoints[0], GetActorLocation()) < 1.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Removing duplicate starting waypoint %s"),
			*GetName(), *CleanedWaypoints[0].ToString());

		CleanedWaypoints.RemoveAt(0);
	}

	// If we removed too much or if the path is invalid, just bail
	if (CleanedWaypoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: No usable waypoints after cleanup!"), *GetName());
		return;
	}

	// --- BUILD the PATHSCRIPT ---
	Path = FPathScript(
		CleanedWaypoints,
		GetActorLocation(),
		RunnerTurnDistance,
		RunnerStoppingDistance
	);

	PathIndex = 0;
	bIsFollowingPath = true;

	// --- INITIAL MOVEMENT DIRECTION ---
	FVector First = CleanedWaypoints[0];
	FVector2D Start2D(GetActorLocation().X, GetActorLocation().Y);
	FVector2D First2D(First.X, First.Y);

	MoveDirection = (First2D - Start2D).GetSafeNormal();

	// Safety: if somehow zero, pick ANY direction toward the next waypoint
	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = FVector2D(1, 0);
		UE_LOG(LogTemp, Warning, TEXT("%s: MoveDirection was zero, applying fallback vector."), *GetName());
	}

	// --- INITIAL ORIENTATION ---
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), First));

	// --- DEBUG ---
	UE_LOG(LogTemp, Warning, TEXT("%s READY | First usable waypoint = %s | MoveDir = %s"),
		*GetName(), *First.ToString(), *MoveDirection.ToString());

	UE_LOG(LogTemp, Warning, TEXT("Runner %s: Starting to follow path."), *GetName());
}

void ARunner::FollowPath(float DeltaTime)
{
	if (!Path.IsSet() || !bIsFollowingPath) return;

	FPathScript& P = Path.GetValue();

	if (PathIndex >= P.LookPoints.Num())
	{
		// We reached the target → pick a new one
		ChooseNewRandomTarget();
		RequestNewPath();
		Path.Reset();
		bIsFollowingPath = false;
		return;
	}

	// Position
	const FVector Pos3D = GetActorLocation();
	const FVector2D Pos2D(Pos3D.X, Pos3D.Y);

	// if close to the waypoint → go to the next waypoint
	if (FVector::Dist2D(Pos3D, P.LookPoints[PathIndex]) < 40.f)
	{
		PathIndex++;
		if (PathIndex >= P.LookPoints.Num())
			return;
	}

	const FVector2D Target2D(P.LookPoints[PathIndex].X, P.LookPoints[PathIndex].Y);
	const FVector2D Desired = (Target2D - Pos2D).GetSafeNormal();

	MoveDirection = FMath::Lerp(MoveDirection,Desired,RunnerTurnSpeed * DeltaTime).GetSafeNormal();

	const FVector Move3D(MoveDirection.X, MoveDirection.Y, 0.f);
	FVector NewLocation = Pos3D + Move3D * (RunnerSpeed * DeltaTime);
	NewLocation.Z = P.LookPoints[PathIndex].Z;

	UE_LOG(LogTemp, Warning, TEXT("%s MoveDir: %s"), *GetName(), *MoveDirection.ToString());
	bool bMoved = SetActorLocation(NewLocation, false, nullptr, ETeleportType::ResetPhysics);
	UE_LOG(LogTemp, Error, TEXT("TRY MOVE: %d -> %s"), bMoved, *NewLocation.ToString());

	if (!Move3D.IsNearlyZero())
	{
		const FVector LookAhead = NewLocation + Move3D * 50.f;
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(NewLocation, LookAhead));
	}
}

/*void ARunner::DrawDebugGizmos() const
{
	if (Path.IsSet() && GetWorld())
	{
		Path->DrawWithDebugGizmos(GetWorld());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Path or GetWorld is null in Runner.h."));
	}
}*/

void ARunner::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	// Prevent immediate death on spawn
	if (GetWorld()->GetTimeSeconds() < 1.f)
		return;
}