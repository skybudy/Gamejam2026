// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/Management/DebugManager.h"
#include "EngineUtils.h"
#include "Public/Pathfinding/Grid/NavGrid.h"
#include "Public/Pathfinding/Management/NavigationSubsystem.h"

// Sets default values
ADebugManager::ADebugManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ADebugManager::BeginPlay()
{
	Super::BeginPlay();
	
}

void ADebugManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.GetPropertyName() == TEXT("GlobalAgentCount"))
	{
		/// If our array of NavGrids is empty, search for them and add them into the array.
		if (AllNavGridActors.Num() == 0)
		{
			if (GetWorld())
			{
				for (TActorIterator<ANavGrid> It(GetWorld()); It; ++It)
				{
					AllNavGridActors.Add(*It);
				}
			}
		}

		if (AllNavGridActors.Num() != 0)
		{
			const int32 Quotient = GlobalAgentCount / AllNavGridActors.Num();
			int32 PossibleRemainder = GlobalAgentCount % AllNavGridActors.Num();
			
			for (const TWeakObjectPtr<ANavGrid> Grid : AllNavGridActors)
			{
				if (PossibleRemainder == 0)
				{
					Grid->SetAgentCount(Quotient);
				}
				else
				{
					if (PossibleRemainder > 0)
					{
						Grid->SetAgentCount(Quotient + 1);
						PossibleRemainder--;
					}
					else
					{
						Grid->SetAgentCount(Quotient);
					}
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No NavGrids found in the scene!"));
		}
	}
	
	if (PropertyChangedEvent.GetPropertyName() == TEXT("bShowAnyDebug"))
	{
		FlushPersistentDebugLines(GetWorld());
		/// TODO: Run appriopriate debug code from NavGrid and Agents (DrawDebug() something).
	}
}

