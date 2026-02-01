// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavLine.generated.h"

// TODO: Comment and explain functionalities in NavLine.
/**
 * @brief Line struct to help draw / debug path smoothing, e.g. draw the path of the AI runners, running towards their goals.
 * @note 90% implemented - just needs comments and testing.
 */
USTRUCT(BlueprintType)
struct GAMEJAM2026_API FNavLine
{
	GENERATED_BODY()

    // Const variables usually have to be a part of the constructor's initializer list, UE is a bit fuzzy about const variables.
    // 'static constexpr' works fine, though.
    static constexpr float VerticalLineGradient = 1e5f;

    // We don't use these default values, but we initialize them for good measure.
    float Gradient = 0.f;
    float YIntercept = 0.f;
    float GradientPerpendicular = 0.f;
    bool bApproachSide = false;

    // These below FVector variables are initialized as ZeroVectors in the constructor.
    FVector2D PointOnLine_1;
    FVector2D PointOnLine_2;

    // Default Constructor with an order-of-declaration initializer list in .cpp.
    FNavLine();

    // Parameterized constructor, the one used by other classes:
    FNavLine(FVector2D PointOnLine, FVector2D PointPerpendicularToLine);

    bool GetSide(FVector2D Point) const;
    bool HasCrossedLine(const FVector2D Point) const;
    float DistanceFromPoint(const FVector2D Point) const;

    void DrawLineDebug(const UWorld* World, float Length, FColor Color, const float LifeTime) const;
};
