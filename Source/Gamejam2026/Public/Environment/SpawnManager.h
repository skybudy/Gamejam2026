// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/NoExportTypes.h"
#include "SpawnManager.generated.h"

class APickUpBase;
class ANavGrid;

UCLASS(Blueprintable, BlueprintType)
class GAMEJAM2026_API USpawnManager : public UObject
{
	GENERATED_BODY()
public:
	USpawnManager();

	// Starts spawning power-ups at regular intervals, instead of auto spawning all at once.
	UFUNCTION(BlueprintCallable, Category = "Spawning|Runtime")
	void StartSpawning(UWorld* World, float IntervalSeconds = 5.0f, int32 MaxConcurrent = 10, int32 SpawnPerTick = 1);
	// Stops any ongoing spawning process, might not be used.
	UFUNCTION(BlueprintCallable, Category = "Spawning|Runtime")
	void StopSpawning(UWorld* World);
	
	// Validates we can reach a nav grid in this world
	bool ValidateNavGrid(UWorld* World);

	// Finds the first NavGrid in the world (nullptr if not found)
	ANavGrid* FindNavGrid(UWorld* World) const;

protected:
	/*
	 *Setting up timed spawning
	 */
	//Logic for spawning
	void SpawnTick(UWorld* World);
	
	FTimerHandle SpawnTimerHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Runtime")
	float SpawnInterval = 10.0f;

	// Minimum allowed distance from an existing active pickup when placing a new one
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Runtime", meta = (ClampMin = "0.0"))
	float MinDistanceBetweenPickUps = 200.0f;
};
