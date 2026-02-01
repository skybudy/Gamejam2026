// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavLine.h"
#include "PathScript.generated.h"

/**
 * @brief The FPathScript helps create a smoother path for a runner when a path has been received.
 * @note 95% implemented - just requires testing and tweaking.
 */
USTRUCT(BlueprintType)
struct GAMEJAM2026_API FPathScript
{
	GENERATED_BODY()

	TArray<FVector> LookPoints;
	TArray<FNavLine> TurnBoundaries;
	
	int32 FinishLineIndex = 0;
	int32 SlowDownIndex = 0;

	// Constructor, with quick initialization.
	FPathScript(){}

	// Constructor with definition kept in the header since it's a struct
	FPathScript(const TArray<FVector>& Waypoints, const FVector& StartPos, float TurnDistance, float StoppingDistance);

	// Needing to include UWorld, for safety purposes regarding UWorld*. Actor classes have safe access to GetWorld(),
	// and can pass it around via functions.
	void DrawWithDebugGizmos(const UWorld* World) const;
};
