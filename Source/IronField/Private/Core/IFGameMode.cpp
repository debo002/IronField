#include "Core/IFGameMode.h"

#include "Building/IFStronghold.h"
#include "Character/IFPlayerCharacter.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerController.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Wave/IFWaveManager.h"

AIFGameMode::AIFGameMode()
{
	static ConstructorHelpers::FClassFinder<APlayerController> ControllerFinder(TEXT("/Game/IronField/Core/Gameplay/BP_PlayerController"));
	if (ControllerFinder.Succeeded())
	{
		PlayerControllerClass = ControllerFinder.Class;
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-GameMode] BP_PlayerController not found; falling back to AIFPlayerController."));
		PlayerControllerClass = AIFPlayerController::StaticClass();
	}

	static ConstructorHelpers::FClassFinder<APawn> PawnFinder(TEXT("/Game/IronField/Characters/Player/BP_Player"));
	if (PawnFinder.Succeeded())
	{
		DefaultPawnClass = PawnFinder.Class;
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-GameMode] BP_Player not found; falling back to AIFPlayerCharacter."));
		DefaultPawnClass = AIFPlayerCharacter::StaticClass();
	}
}

void AIFGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UIFWaveManagerSubsystem* const WaveSubsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
	{
		if (AIFWaveManager* const WaveManager = WaveSubsystem->GetWaveManager())
		{
			HandleWaveManagerRegistered(WaveManager);
		}
		else
		{
			WaveSubsystem->OnWaveManagerRegistered.AddDynamic(this, &AIFGameMode::HandleWaveManagerRegistered);
		}
	}

	if (UIFStrongholdSubsystem* const StrongholdSubsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
	{
		if (AIFStronghold* const Stronghold = StrongholdSubsystem->GetStronghold())
		{
			HandleStrongholdRegistered(Stronghold);
		}
		else
		{
			StrongholdSubsystem->OnStrongholdRegistered.AddDynamic(this, &AIFGameMode::HandleStrongholdRegistered);
		}
	}
}

void AIFGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* const World = GetWorld();
	if (World)
	{
		if (UIFWaveManagerSubsystem* const WaveSubsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
		{
			WaveSubsystem->OnWaveManagerRegistered.RemoveDynamic(this, &AIFGameMode::HandleWaveManagerRegistered);
		}

		if (UIFStrongholdSubsystem* const StrongholdSubsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			StrongholdSubsystem->OnStrongholdRegistered.RemoveDynamic(this, &AIFGameMode::HandleStrongholdRegistered);
		}
	}

	if (BoundWaveManager)
	{
		BoundWaveManager->OnAllWavesCompleted.RemoveDynamic(this, &AIFGameMode::HandleAllWavesCompleted);
		BoundWaveManager = nullptr;
	}

	if (BoundStronghold)
	{
		BoundStronghold->OnStrongholdDestroyed.RemoveDynamic(this, &AIFGameMode::HandleStrongholdDestroyed);
		BoundStronghold = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AIFGameMode::HandleWaveManagerRegistered(AIFWaveManager* WaveManager)
{
	if (!WaveManager || BoundWaveManager)
	{
		return;
	}

	BoundWaveManager = WaveManager;
	WaveManager->OnAllWavesCompleted.AddDynamic(this, &AIFGameMode::HandleAllWavesCompleted);
}

void AIFGameMode::HandleStrongholdRegistered(AIFStronghold* Stronghold)
{
	if (!Stronghold || BoundStronghold)
	{
		return;
	}

	BoundStronghold = Stronghold;
	Stronghold->OnStrongholdDestroyed.AddDynamic(this, &AIFGameMode::HandleStrongholdDestroyed);
}

void AIFGameMode::HandleAllWavesCompleted()
{
	UE_LOG(LogIronField, Log, TEXT("[IF-GameMode] All waves completed."));
	ShowGameOver(EIFGameResult::Victory);
}

void AIFGameMode::HandleStrongholdDestroyed(AIFStronghold*)
{
	UE_LOG(LogIronField, Log, TEXT("[IF-GameMode] Stronghold destroyed."));
	ShowGameOver(EIFGameResult::Defeat);
}

void AIFGameMode::ShowGameOver(EIFGameResult Result)
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

	PlayerController->ShowGameOverScreen(Result);
}
