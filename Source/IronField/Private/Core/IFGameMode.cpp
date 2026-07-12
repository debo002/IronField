#include "Core/IFGameMode.h"

#include "Building/IFStronghold.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerController.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "TimerManager.h"
#include "Wave/IFWaveManager.h"

void AIFGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &AIFGameMode::BindGameFlowDelegates);
	}
}

void AIFGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	UnbindGameFlowDelegates();
	Super::EndPlay(EndPlayReason);
}

void AIFGameMode::BindGameFlowDelegates()
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	if (const UIFWaveManagerSubsystem* const WaveSubsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
	{
		if (AIFWaveManager* const WaveManager = WaveSubsystem->GetWaveManager())
		{
			WaveManager->OnAllWavesCompleted.AddDynamic(this, &AIFGameMode::HandleAllWavesCompleted);
		}
		else
		{
			UE_LOG(LogIronField, Warning, TEXT("[IF-GameMode] No wave manager registered; win condition will not fire."));
		}
	}

	if (const UIFStrongholdSubsystem* const StrongholdSubsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
	{
		if (AIFStronghold* const Stronghold = StrongholdSubsystem->GetStronghold())
		{
			Stronghold->OnStrongholdDestroyed.AddDynamic(this, &AIFGameMode::HandleStrongholdDestroyed);
		}
		else
		{
			UE_LOG(LogIronField, Warning, TEXT("[IF-GameMode] No stronghold registered; lose condition will not fire."));
		}
	}
}

void AIFGameMode::UnbindGameFlowDelegates()
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	if (const UIFWaveManagerSubsystem* const WaveSubsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
	{
		if (AIFWaveManager* const WaveManager = WaveSubsystem->GetWaveManager())
		{
			WaveManager->OnAllWavesCompleted.RemoveAll(this);
		}
	}

	if (const UIFStrongholdSubsystem* const StrongholdSubsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
	{
		if (AIFStronghold* const Stronghold = StrongholdSubsystem->GetStronghold())
		{
			Stronghold->OnStrongholdDestroyed.RemoveAll(this);
		}
	}
}

void AIFGameMode::HandleAllWavesCompleted()
{
	UE_LOG(LogIronField, Log, TEXT("[IF-GameMode] All waves completed."));
	OnGameWon();
}

void AIFGameMode::HandleStrongholdDestroyed(AIFStronghold* )
{
	UE_LOG(LogIronField, Log, TEXT("[IF-GameMode] Stronghold destroyed."));
	OnGameLost();
}

void AIFGameMode::OnGameLost()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	AIFPlayerController* const PlayerController = Cast<AIFPlayerController>(World->GetFirstPlayerController());
	if (!PlayerController)
	{
		return;
	}

	PlayerController->ShowLoseScreen();
}
