// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Core/NavLine.h"
#include "DrawDebugHelpers.h"

constexpr float FNavLine::VerticalLineGradient; // definition of the constexpr

FNavLine::FNavLine()
	: Gradient(0.f)
	, YIntercept(0.f)
	, GradientPerpendicular(0.f)
	, bApproachSide(false)
	, PointOnLine_1(FVector2D::ZeroVector)
	, PointOnLine_2(FVector2D::ZeroVector)
{}

FNavLine::FNavLine(const FVector2D PointOnLine, const FVector2D PointPerpendicularToLine) : FNavLine()
{
	const float DX = PointOnLine.X - PointPerpendicularToLine.X;
	const float Dy = PointOnLine.Y - PointPerpendicularToLine.Y;

	if (FMath::IsNearlyZero(DX))
	{
		GradientPerpendicular = VerticalLineGradient;
	}
	else
	{
		GradientPerpendicular = Dy / DX;
	}

	if (FMath::IsNearlyZero(GradientPerpendicular))
	{
		Gradient = VerticalLineGradient;
	}
	else
	{
		Gradient = -1 / GradientPerpendicular;
	}
        
	YIntercept = PointOnLine.Y - Gradient * PointOnLine.X;
	PointOnLine_1 = PointOnLine;
	PointOnLine_2 = PointOnLine + FVector2D(1, Gradient);
	
	bApproachSide = GetSide(PointPerpendicularToLine);
}

bool FNavLine::GetSide(const FVector2D Point) const
{
	return (Point.X-PointOnLine_1.X) * (PointOnLine_2.Y - PointOnLine_1.Y) > 
			(Point.Y - PointOnLine_1.Y) * (PointOnLine_2.X - PointOnLine_1.X);
}

bool FNavLine::HasCrossedLine(const FVector2D Point) const
{
	return GetSide(Point) != bApproachSide;
}

float FNavLine::DistanceFromPoint(const FVector2D Point) const
{
	const float YInterceptPerpendicular = Point.Y - GradientPerpendicular * Point.X;
	const float IntersectX = (YInterceptPerpendicular - YIntercept) / (Gradient - GradientPerpendicular);
	const float IntersectY = Gradient * IntersectX + YIntercept;
	
	return FVector2D::Distance(Point, FVector2D(IntersectX, IntersectY));
}

void FNavLine::DrawLineDebug(const UWorld* World, const float Length, const FColor Color = FColor::White, const float LifeTime = 2) const
{
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawLine() called with null World!"));
		return;
	}
	
	const FVector LineDirection = FVector(1, 0, Gradient).GetSafeNormal();
	const FVector LineCentre = FVector(PointOnLine_1.X, 0, PointOnLine_1.Y) + FVector::UpVector;

	const FVector Start = LineCentre - LineDirection * Length / 2.f;
	const FVector End = LineCentre + LineDirection * Length / 2.f;

	// UE-equivalent to gizmos
	DrawDebugLine(World, Start, End, Color, false, LifeTime, 0, 2.f);
}
