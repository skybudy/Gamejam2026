// Fill out your copyright notice in the Description page of Project Settings.

#include "TagGameMode.h"

#include "EngineUtils.h"
#include "TagGameState.h"
#include "Pathfinding/Management/NavigationSubsystem.h"
#include "Pathfinding/Grid/NavGrid.h"
#include "Pathfinding/Actors/Runner.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ATagGameMode::ATagGameMode()
{
	GameStateClass = ATagGameState::StaticClass();
	// Set default values for the run timer
	RunTimer = 60.0f;
	CurrentTimeRemaining = RunTimer;
	
}

void ATagGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// Spawn power-ups at the start of the game
	const UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode: No world in BeginPlay"));
		return;
	}
	
	NavigationSubsystem = World->GetSubsystem<UNavigationSubsystem>();
	
	ANavGrid* FoundNavGrid = nullptr;
	
	/// TODO: Replace this with asking the NavigationSubsystem for grids instead, if GameMode needs it!
	/*for (TActorIterator<ANavGrid> It(World); It;)
	{
		FoundNavGrid = *It;
		break; // Use the first found instance
	}
	
	if (FoundNavGrid)
	{
		NavGrid = FoundNavGrid;
		UE_LOG(LogTemp, Log, TEXT("Using existing NavGrid: %s"), *FoundNavGrid->GetName());
	}
	else
	{
		NavGrid = NavigationSubsystem->GetNavGrid();
		if (!NavGrid.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("No NavGrid found in world. Make sure a NavGrid actor exists in the level."));
		}
	}*/

	// Use the member SpawnManager to spawn power-ups
	SpawnManagerInstance = NewObject<USpawnManager>(this, SpawnManagerClass);
	SpawnManagerInstance->StartSpawning(GetWorld());
	
	// Start the timer to call UpdateTimer every second
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ATagGameMode::UpdateTimer, 1.0f, true);

	// Power-up time increase binding
	// Bind to all pickups already in the level
	TArray<AActor*> Runners;
	UGameplayStatics::GetAllActorsOfClass(this, ARunner::StaticClass(), Runners);
	
	for (int i = 0; i < NavigationSubsystem->GetNumberOfRunnersAlwaysActive(); ++i)
	{
		SpawnNewRunner();
	}
}

void ATagGameMode::SpawnNewRunner() const
{
	if (!RunnerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RunnerClass null in SpawnNewRunner()."));
		return;
	}
	/// 
	if (!NavGrid.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("NavGrid null in SpawnNewRunner()."));
		return;
	}
	if (!GetWorld()) return;
	
	const FVector SpawnLocation = NavGrid->GetRandomGridLocation(true);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	/// If we succeed at spawning the new runner, set initial spawn position.
	if (ARunner* SpawnedRunner = GetWorld()->SpawnActor<ARunner>(RunnerClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
	{
		SpawnedRunner->SetInitialSpawnPosition(SpawnLocation);
	}
}


//Added it to UI in the GameMode blueprint
void ATagGameMode::UpdateTimer()
{
	// Decrease the remaining time and increase the total run timer
	CurrentTimeRemaining -= 1.0f;
	RunTimer += 1.0f;

	UE_LOG(LogTemp, Log, TEXT("Time left: %.0f seconds"), CurrentTimeRemaining);

	if (CurrentTimeRemaining <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		EndGame();
	}
}
void ATagGameMode::EndGame()
{
	// Stop spawning power-ups
	SpawnManagerInstance->StopSpawning(GetWorld());
	//Rest of logic to end the game
	UE_LOG(LogTemp, Warning, TEXT("Time's up! Ending the game."));
	if (!GameOverWidgetClass) return;
	GameOverWidget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
	if (!GameOverWidget) return;
		GameOverWidget->AddToViewport();
		//show mouse cursor
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				PC->bShowMouseCursor = true;
				PC->SetInputMode(FInputModeUIOnly());
			}
}
void ATagGameMode::AddExtraTime()
{
	// Add extra time to the timer, currently 10.
	CurrentTimeRemaining += CatchIncreaseTime;

	UE_LOG(LogTemp, Log, TEXT("Added %.0f seconds! New time: %.0f"), CatchIncreaseTime, CurrentTimeRemaining);

}



