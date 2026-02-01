// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pathfinding/Management/PathRequestDelegate.h"
#include "PathRequest.generated.h"

class UPathRequestManager;

/**
 * @brief PathRequest Struct, which helps set the PathStart, PathEnd, a delegate FPathRequestCallback, and
 * a PathRequestManager to help call functions in ARunner from UNavigationSubsystem when a path has been found.
 * @note 100% implemented? Just remains to be tested.
 */
USTRUCT(BlueprintType)
struct GAMEJAM2026_API FPathRequest
{
	GENERATED_BODY()
	
	FVector PathStart;
	FVector PathEnd;
	// Non-dynamic delegate member (no UPROPERTY for this one)
	FPathRequestCallback Callback;
	TWeakObjectPtr<UPathRequestManager> Requester;

	// Constructor, with quick initialization.
	FPathRequest() = default;

	FPathRequest(const FVector& PathStart, const FVector& PathEnd, FPathRequestCallback InCallback, UPathRequestManager* InRequester);
};
