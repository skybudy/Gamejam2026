// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pathfinding/Management/NavigationSubsystem.h" // leave here for later connection with the UNavigationSubsystem
#include "DijkstraGlobalMap.generated.h"

/**
 * @brief Updated and retrieved by the UNavigationSubsystem every few frames / seconds, to be shared with runners,
 * storing safety / danger values relative to player location.
 *
 * @note 10% implemented - nothing really implemented yet besides class hierarchy and pseudocode intent above.
 */
UCLASS()
class GAMEJAM2026_API UDijkstraGlobalMap : public UObject
{
	GENERATED_BODY()
	
};
