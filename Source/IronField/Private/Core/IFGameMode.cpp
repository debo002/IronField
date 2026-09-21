#include "Core/IFGameMode.h"

#include "Building/IFStronghold.h"
#include "Character/IFPlayerCharacter.h"
#include "Core/IFBestRunSave.h"
#include "Core/IFGameInstance.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerController.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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

	SaveBestRun(Result);

	AIFPlayerController* const PlayerController = Cast<AIFPlayerController>(World->GetFirstPlayerController());
	if (!PlayerController)
	{
		return;
	}

	PlayerController->ShowGameOverScreen(Result);
}

void AIFGameMode::SaveBestRun(EIFGameResult Result)
{
	if (!BoundWaveManager)
	{
		return;
	}

	UIFGameInstance* const GameInstance = Cast<UIFGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		return;
	}

	const int32 WaveNumber = FMath::Max(1, BoundWaveManager->GetCurrentWave());
	const int32 KillCount = BoundWaveManager->GetKillCount();
	const EIFRunMode RunMode = GameInstance->GetRunMode();
	const FString SlotName = UIFBestRunSave::GetSlotName();

	UIFBestRunSave* Save = Cast<UIFBestRunSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save)
	{
		Save = NewObject<UIFBestRunSave>(this);
	}
	if (!Save)
	{
		return;
	}

	// Higher wave wins; kills break ties so late pushes still record.
	if (RunMode == EIFRunMode::Normal)
	{
		if (WaveNumber > Save->BestNormalWave || (WaveNumber == Save->BestNormalWave && KillCount > Save->BestNormalKills))
		{
			Save->BestNormalWave = WaveNumber;
			Save->BestNormalKills = KillCount;
		}
		if (Result == EIFGameResult::Victory)
		{
			Save->bNormalCleared = true;
		}
	}
	else
	{
		if (WaveNumber > Save->BestUnlimitedWave || (WaveNumber == Save->BestUnlimitedWave && KillCount > Save->BestUnlimitedKills))
		{
			Save->BestUnlimitedWave = WaveNumber;
			Save->BestUnlimitedKills = KillCount;
		}
	}

	UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}
