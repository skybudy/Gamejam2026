// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TerrainType.generated.h"

/**
 * @brief Struct for managing collision and layers with TerrainPenalities for the runners
 * @note 50% implemented? requires testing and iterations.
 */
USTRUCT(BlueprintType)
struct GAMEJAM2026_API FTerrainType
{
	GENERATED_BODY()

	// TODO: Translate the LayerMask from Unity
	// public LayerMask terrainMask;
	
	// TODO: Consider swapping away from CollisionChannel to PhysicsMaterials, or do both.
	ECollisionChannel CollisionChannel;
	int32 TerrainPenalty = 0;
	
	FTerrainType() : CollisionChannel() {}
};
