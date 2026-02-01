/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pathfinding/Core/PathScript.h"
#include "GameFramework/Pawn.h"
#include "Pathfinding/Management/PathRequestDelegate.h"
#include "Runner.generated.h"

class UCapsuleComponent;
class UNavigationSubsystem;
class UPathRequestManager;

/// TODO: Implement running away from the player / Implement boid-like behaviour

/**
 * @brief The Runner class, based on APawn. Talks with a PathRequestManager whenever it wants to request a new path.
 * @details Uses PathScript to run a given path more smoothly towards a target.
 * @note 40% implemented towards the ideal runner, 75%-ish done with translating over from C# Unity to C++ Ùnreal Engine.
 */
UCLASS()
class GAMEJAM2026_API ARunner : public APawn
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;
	
	/// Callback delegate to help call OnPathFound() when the assigned pathfinder class has found a new path for this runner
	FPathRequestCallback Callback;
	
	/// Used for keeping track of runners in the game.
	int32 RunnerID;
	
	/// Used for a quick and inexpensive way to respawn runners.
	FVector InitialSpawnPosition;

	/// How often the runner updates following their path.
	float MinPathUpdateTime = 0.2f;
	
	float LastPathRequestTime = 0.f;
	float SquareMoveThreshold = 0.0f;

	/// Making it a TOptional (pointer), since the struct (a path) might not always be present. We can do null checks on it. :)
	TOptional<FPathScript> Path;
	int32 PathIndex = 0;
	bool bIsFollowingPath = false;
	FVector2D MoveDirection = FVector2D::ZeroVector;
    
public:

	/// The assigned PathRequestManager to this Runner, fetched from NavigationSubsystem
	UPROPERTY()
	TObjectPtr<UPathRequestManager>	PathRequestManager;
	
	/// Current goal / destination for running, ideally away from the player
	FVector RunnerTarget;
	FVector RunnerTargetOld;

	/// TimerHandles to help mimic coroutine behaviors from Unity and C#, is used to call functions again in timed intervals.
	FTimerHandle PathRequestTimerHandle;
	FTimerHandle FollowPathTimerHandle;

	/// Current running speed
	UPROPERTY(EditDefaultsOnly, Category="Runner")
	float RunnerSpeed = 300.f;

	/// Current runner turning speed to move around corners and stuff.
	UPROPERTY(EditDefaultsOnly, Category="Runner")
	float RunnerTurnSpeed = 8.f;

	/// Current limit of how much the player can turn (path smoothing)?
	UPROPERTY(EditDefaultsOnly, Category="Runner")
	float RunnerTurnDistance = 5.f;

	/// Current distance for when the runner starts slowing down when close to the RunnerTarget.
	UPROPERTY(EditDefaultsOnly, Category="Runner")
	float RunnerStoppingDistance = 50.f;
	
	float PathRequestCooldown = 0.5f;
	/// Used to calculate the square for runner position between PathUpdate checks, to avoid updating if the runner has not moved.
    float PathUpdateMoveThreshold = 0.5f;

	/// TODO: Test functionality above to runSpeed and walkSpeed maybe? Test with how the runners feel in-game.
	
private:
	
	void RequestNewPath();
	void StartFollowingPath();
	void FollowPath(float DeltaTime);

public:
	
	/// Default constructor, setting default values
	ARunner();
	
	/// Function called when the player catches a runner 
	UFUNCTION()
	void OnRunnerCaught();

	void ChooseNewRandomTarget();
	
	/// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaTime) override;
	
	/// Function called when runner falls out of the world for some reason.
	virtual void FellOutOfWorld(const class UDamageType& DmgType) override;

	/// Called when a new path is found, to stop following the current path and switch over to a new path.
	void OnPathFound(const TArray<FVector>& Waypoints, const bool bSuccess);
	
	UFUNCTION()
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	/// Helper for storing the initial spawn position of the runner into its own private variable
	void SetInitialSpawnPosition(const FVector& SpawnPosition) { InitialSpawnPosition = SpawnPosition; };
};
