// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/Core/PathScript.h"

FPathScript::FPathScript(const TArray<FVector>& Waypoints, const FVector& StartPos, const float TurnDistance, const float StoppingDistance)
{
	LookPoints = Waypoints;
	
	const int32 Count = LookPoints.Num();

	if (Count == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FPathScript: No waypoints given."));
		return;
	}
	
	TurnBoundaries.SetNum(Count);
	FinishLineIndex = Count - 1;
		
	FVector2D Prev2D = FVector2D(StartPos.X, StartPos.Y);
	
	for (int32 i = 0; i < Count; i++)
	{
		FVector2D Current2D = FVector2D(LookPoints[i].X, LookPoints[i].Y);
		
		FVector2D DirectionToCurrentPoint = (Current2D - Prev2D).GetSafeNormal();
		
		FVector2D TurnPoint;
		
		if (i == FinishLineIndex)
		{
			TurnPoint = Current2D;
		}
		else
		{
			TurnPoint = Current2D - DirectionToCurrentPoint * TurnDistance;
		}

		// Create a boundary line
		const FVector2D BoundaryStart = TurnPoint;
		const FVector2D BoundaryEnd = Prev2D - DirectionToCurrentPoint * TurnDistance;
		
		TurnBoundaries[i] = FNavLine(BoundaryStart, BoundaryEnd);
		Prev2D = TurnPoint;
	}
	
	float DistanceFromEndPoint = 0.f;
	SlowDownIndex = FinishLineIndex;
	
	for (int32 i = Count - 1; i > 0; i--)
	{
		DistanceFromEndPoint += FVector::Distance(LookPoints[i], LookPoints[i - 1]);
		if (DistanceFromEndPoint > StoppingDistance)
		{
			SlowDownIndex = i;
			break;
		}
	}
}

void FPathScript::DrawWithDebugGizmos(const UWorld* World) const
{
	for (FVector Point : LookPoints)
	{
		DrawDebugBox(World, Point + FVector::UpVector, FVector::OneVector, FColor::Black, false, 3.f, 0, 2.f);
	}
		
	for (const FNavLine Line : TurnBoundaries)
	{
		Line.DrawLineDebug(World, 10.f, FColor::White, 3.f);
	}
}
