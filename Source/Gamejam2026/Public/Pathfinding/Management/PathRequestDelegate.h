// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
	 
#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h" // the macro delegates rely on this for full functionality.

/// TODO: Investigate delegates for running functions only in the Editor for future projects.

DECLARE_DELEGATE_TwoParams(FPathRequestCallback, const TArray<FVector>& /*Path*/, bool /*bSuccess*/);

/// TODO? The below delegate needs rework of the entire codebase if needed for BPs with runners.
/// Create a dynamic delegate (serializable and usable for BPs)
/// DECLARE_DYNAMIC_DELEGATE_TwoParams(FPathRequestCallback, const TArray<FVector>&, Path, bool, bSuccess);
