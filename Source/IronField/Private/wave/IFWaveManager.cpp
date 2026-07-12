#include "Wave/IFWaveManager.h"

#include "Building/IFStronghold.h"
#include "Character/IFPlayerCharacter.h"
#include "Character/IFBaseCharacter.h"
#include "Wave/IFEnemySpawnPoint.h"
#include "Combat/IFCombatComponent.h"
#include "Core/IFGameInstance.h"
#include "Core/IFLog.h"
#include "Kismet/GameplayStatics.h"
#include "Stats/IFHealthComponent.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Core/IFWaveManagerSubsystem.h"

AIFWaveManager::AIFWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AIFWaveManager::TryReserveEngagementSlot()
{
	if (CurrentPlayerEngagementCount >= MaxPlayerEngagementSlots)
	{
		return false;
	}

	++CurrentPlayerEngagementCount;
	return true;
}

void AIFWaveManager::ReleaseEngagementSlot()
{
	CurrentPlayerEngagementCount = FMath::Max(0, CurrentPlayerEngagementCount - 1);
}

AActor* AIFWaveManager::GetPlayerActor() const
{
	return CachedPlayer;
}

AActor* AIFWaveManager::GetStrongholdActor() const
{
	if (const UWorld* const World = GetWorld())
	{
		if (const UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			return Subsystem->GetStronghold();
		}
	}
	return nullptr;
}

void AIFWaveManager::StartNextWave()
{
	if (bIsWaveActive)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Cannot start next wave while a wave is already active."));
		return;
	}

	CurrentWaveIndex++;

	// Unlimited never touches Waves — every wave is UnlimitedBaseWave scaled by index.
	if (IsUnlimitedRunMode())
	{
		const FWaveDefinition UnlimitedWave = BuildUnlimitedWave(CurrentWaveIndex);
		BeginWaveFromDefinition(UnlimitedWave);
		return;
	}

	if (CurrentWaveIndex >= Waves.Num())
	{
		bIsWaveActive = false;
		bWaitingForNextWave = false;
		UE_LOG(LogIronField, Log, TEXT("[IF-Wave] All waves completed."));
		OnAllWavesCompleted.Broadcast();
		return;
	}

	BeginWaveFromDefinition(Waves[CurrentWaveIndex]);
}

void AIFWaveManager::BeginWaveFromDefinition(const FWaveDefinition& Wave)
{
	for (AIFBaseCharacter* Enemy : SpawnedEnemies)
	{
		if (Enemy)
		{
			Enemy->OnCharacterDied.RemoveAll(this);
		}
	}

	EnemiesSpawnedSoFar = 0;
	EnemiesAlive = 0;
	TotalEnemiesInWave = 0;
	SpawnedEnemies.Empty();
	bIsWaveActive = true;
	bWaitingForNextWave = false;

	const int32 WaveNumber = GetCurrentWave();
	UE_LOG(LogIronField, Log, TEXT("[IF-Wave] Starting wave %d."), WaveNumber);
	OnWaveStarted.Broadcast(WaveNumber);

	SpawnAllEnemiesInWave(Wave);
}

bool AIFWaveManager::IsUnlimitedRunMode() const
{
	const UIFGameInstance* const GameInstance = Cast<UIFGameInstance>(GetGameInstance());
	return GameInstance && GameInstance->GetRunMode() == EIFRunMode::Unlimited;
}

FWaveDefinition AIFWaveManager::BuildUnlimitedWave(int32 WaveIndex) const
{
	// WaveIndex is 0-based: wave 1 keeps authored counts, each later wave multiplies again.
	FWaveDefinition Result = UnlimitedBaseWave;
	const float Scale = FMath::Pow(UnlimitedEnemyCountScaleFactor, static_cast<float>(WaveIndex));

	for (FEnemyGroupDefinition& Group : Result.EnemyGroups)
	{
		Group.EnemyCount = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Group.EnemyCount) * Scale));
	}

	return Result;
}

void AIFWaveManager::SpawnAllEnemiesInWave(const FWaveDefinition& Wave)
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> SpawnPointActors;
	UGameplayStatics::GetAllActorsOfClass(World, AIFEnemySpawnPoint::StaticClass(), SpawnPointActors);

	if (SpawnPointActors.Num() <= 0)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] No AIFEnemySpawnPoint actors found; spawning at manager location."));
	}

	TotalEnemiesInWave = 0;
	for (const FEnemyGroupDefinition& Group : Wave.EnemyGroups)
	{
		TotalEnemiesInWave += Group.EnemyCount;
	}

	for (const FEnemyGroupDefinition& Group : Wave.EnemyGroups)
	{
		if (!Group.EnemyClass)
		{
			UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Wave %d has an enemy group with no Enemy Class — skipping."), GetCurrentWave());
			// Keep the completion target honest when a group cannot spawn.
			TotalEnemiesInWave = FMath::Max(0, TotalEnemiesInWave - Group.EnemyCount);
			continue;
		}

		for (int32 i = 0; i < Group.EnemyCount; ++i)
		{
			FVector SpawnLocation = GetActorLocation();
			FRotator SpawnRotation = GetActorRotation();

			if (SpawnPointActors.Num() > 0)
			{
				const int32 RandomIndex = FMath::RandRange(0, SpawnPointActors.Num() - 1);
				if (AActor* const SpawnPoint = SpawnPointActors[RandomIndex])
				{
					SpawnLocation = SpawnPoint->GetActorLocation();
					SpawnRotation = SpawnPoint->GetActorRotation();
				}
			}

			SpawnLocation.X += FMath::RandRange(-40.f, 40.f);
			SpawnLocation.Y += FMath::RandRange(-40.f, 40.f);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AIFBaseCharacter* const SpawnedEnemy = World->SpawnActor<AIFBaseCharacter>(Group.EnemyClass, SpawnLocation, SpawnRotation, SpawnParams);
			if (SpawnedEnemy)
			{
				EnemiesSpawnedSoFar++;
				NotifyEnemySpawned(SpawnedEnemy);
			}
			else
			{
				TotalEnemiesInWave = FMath::Max(0, TotalEnemiesInWave - 1);
				UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Failed to spawn %s at %s"),
					*Group.EnemyClass->GetName(), *SpawnLocation.ToString());
			}
		}
	}
}

void AIFWaveManager::NotifyEnemySpawned(AIFBaseCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	SpawnedEnemies.AddUnique(Enemy);
	EnemiesAlive++;
	OnEnemiesAliveCountChanged.Broadcast(EnemiesAlive);

	Enemy->OnCharacterDied.AddDynamic(this, &AIFWaveManager::HandleEnemyDied);
}

void AIFWaveManager::HandleEnemyDied(AIFBaseCharacter* DeadEnemy)
{
	if (!DeadEnemy)
	{
		return;
	}

	DeadEnemy->OnCharacterDied.RemoveDynamic(this, &AIFWaveManager::HandleEnemyDied);

	EnemiesAlive = FMath::Max(0, EnemiesAlive - 1);
	OnEnemiesAliveCountChanged.Broadcast(EnemiesAlive);

	if (EnemiesAlive <= 0 && EnemiesSpawnedSoFar >= TotalEnemiesInWave)
	{
		bIsWaveActive = false;
		const int32 WaveNumber = GetCurrentWave();
		UE_LOG(LogIronField, Log, TEXT("[IF-Wave] Wave %d complete."), WaveNumber);
		OnWaveCompleted.Broadcast(WaveNumber);
	}
}

void AIFWaveManager::CleanupWaveCorpses()
{
	TArray<TObjectPtr<AIFBaseCharacter>> RemainingEnemies;

	for (AIFBaseCharacter* Enemy : SpawnedEnemies)
	{
		if (!Enemy)
		{
			continue;
		}

		if (Enemy->IsDead())
		{
			Enemy->Destroy();
		}
		else
		{
			RemainingEnemies.Add(Enemy);
		}
	}

	SpawnedEnemies = RemainingEnemies;
}

void AIFWaveManager::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* const World = GetWorld())
	{
		if (UIFWaveManagerSubsystem* const Subsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
		{
			Subsystem->RegisterWaveManager(this);
		}
	}

	CachePlayer();
	BindPlayerDelegates();

	OnWaveCompleted.AddDynamic(this, &AIFWaveManager::HandleWaveCompleted);

	if (bAutoStartOnBeginPlay)
	{
		StartNextWave();
	}
}

void AIFWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const World = GetWorld())
	{
		if (UIFWaveManagerSubsystem* const Subsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
		{
			Subsystem->UnregisterWaveManager(this);
		}
	}

	OnWaveCompleted.RemoveDynamic(this, &AIFWaveManager::HandleWaveCompleted);

	UnbindPlayerDelegates();

	for (AIFBaseCharacter* Enemy : SpawnedEnemies)
	{
		if (Enemy)
		{
			Enemy->OnCharacterDied.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AIFWaveManager::HandleWaveCompleted(int32)
{
	CleanupWaveCorpses();

	// Shop/rest UI will own this later; for now proceed immediately.
	bWaitingForNextWave = true;
	BeginNextWave();
}

void AIFWaveManager::BeginNextWave()
{
	if (!bWaitingForNextWave)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] BeginNextWave called while not waiting for the next wave."));
		return;
	}

	StartNextWave();
}

void AIFWaveManager::CachePlayer()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		CachedPlayer = nullptr;
		return;
	}

	CachedPlayer = Cast<AIFPlayerCharacter>(UGameplayStatics::GetActorOfClass(World, AIFPlayerCharacter::StaticClass()));

	if (!CachedPlayer)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] No AIFPlayerCharacter found in the level."));
	}
}

void AIFWaveManager::BindPlayerDelegates()
{
	if (!CachedPlayer)
	{
		return;
	}

	if (UIFCombatComponent* const Combat = CachedPlayer->GetCombatComponent())
	{
		Combat->OnCombatStateChanged.AddDynamic(this, &AIFWaveManager::UpdatePlayerActivityTimestamps);
	}

	if (UIFHealthComponent* const Health = CachedPlayer->GetHealthComponent())
	{
		Health->OnHealthDepleted.AddDynamic(this, &AIFWaveManager::HandlePlayerHealthDepleted);
	}
}

void AIFWaveManager::UnbindPlayerDelegates()
{
	if (!CachedPlayer)
	{
		return;
	}

	if (UIFCombatComponent* const Combat = CachedPlayer->GetCombatComponent())
	{
		Combat->OnCombatStateChanged.RemoveAll(this);
	}

	if (UIFHealthComponent* const Health = CachedPlayer->GetHealthComponent())
	{
		Health->OnHealthDepleted.RemoveAll(this);
	}
}

void AIFWaveManager::UpdatePlayerActivityTimestamps(ECombatState PreviousState, ECombatState NewState)
{
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();

	if (NewState == ECombatState::Attacking)
	{
		LastPlayerAttackTime = Now;
	}
	else if (PreviousState == ECombatState::Dead && NewState == ECombatState::Idle)
	{
		LastPlayerReviveTime = Now;
	}
}

void AIFWaveManager::HandlePlayerHealthDepleted()
{
	OnPlayerDied.Broadcast();
}
