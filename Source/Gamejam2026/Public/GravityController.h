// This code was retrieved from the UE tutorial on custom gravity for UE 5.4

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
UCLASS()
class GAMEJAM2026_API AGravityController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	AGravityController();
	
	virtual void UpdateRotation(float DeltaTime) override;
	
	// Converts a rotation from world space to gravity relative space.
	UFUNCTION(BlueprintPure)
	static FRotator GetGravityRelativeRotation(FRotator Rotation, FVector GravityDirection);
	 
	// Converts a rotation from gravity relative space to world space.
	UFUNCTION(BlueprintPure)
	static FRotator GetGravityWorldRotation(FRotator Rotation, FVector GravityDirection);
	 
private:
	FVector LastFrameGravity = FVector::ZeroVector;
};
