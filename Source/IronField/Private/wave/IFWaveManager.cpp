#include "Wave/IFWaveManager.h"

#include "Building/IFStronghold.h"
#include "Character/IFPlayerCharacter.h"
#include "Character/IFBaseCharacter.h"
#include "Wave/IFEnemySpawnPoint.h"
#include "Combat/IFCombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Stats/IFHealthComponent.h"
#include "Core/IFStrongholdSubsystem.h"

AIFWaveManager::AIFWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentWaveIndex = -1;
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
		UE_LOG(LogTemp, Warning, TEXT("[IF-Wave] Cannot start next wave while wave is already active."));
		return;
	}

	CurrentWaveIndex++;
	CurrentWave = CurrentWaveIndex + 1;

	if (CurrentWaveIndex >= Waves.Num())
	{
		bIsWaveActive = false;
		UE_LOG(LogTemp, Log, TEXT("[IF-Wave] All waves completed."));
		OnAllWavesCompleted.Broadcast();
		return;
	}

	EnemiesSpawnedSoFar = 0;
	EnemiesAlive = 0;
	TotalEnemiesInWave = 0;
	SpawnedEnemies.Empty();
	bIsWaveActive = true;

	UE_LOG(LogTemp, Log, TEXT("[IF-Wave] Starting wave %d."), CurrentWave);
	OnWaveStarted.Broadcast(CurrentWave);

	const FWaveDefinition& Wave = Waves[CurrentWaveIndex];
	SpawnEnemyFromWave(Wave);
}

void AIFWaveManager::SpawnEnemyFromWave(const FWaveDefinition& Wave)
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
		UE_LOG(LogTemp, Warning, TEXT("[IF-Wave] No AIFEnemySpawnPoint actors found in the level; spawning at manager location."));
	}

	TotalEnemiesInWave = 0;
	for (const FEnemyGroupDefinition& Group : Wave.EnemyGroups)
	{
		TotalEnemiesInWave += Group.EnemyCount;
	}

	EnemiesSpawnedSoFar = 0;
	EnemiesAlive = 0;

	for (const FEnemyGroupDefinition& Group : Wave.EnemyGroups)
	{
		if (!Group.EnemyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[IF-Wave] Wave %d has an enemy group with no Enemy Class assigned - skipping."), CurrentWave);
			// These enemies will never exist, so they must not count toward the wave total,
			// or completion could never trigger.
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

			// Slight random offset so enemies don't spawn perfectly overlapping.
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
				// Failed spawns must not count toward the wave total either, for the same reason.
				TotalEnemiesInWave = FMath::Max(0, TotalEnemiesInWave - 1);
				UE_LOG(LogTemp, Warning, TEXT("[IF-Wave] Failed to spawn %s at %s"),
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

	// Wave completes once every enemy configured in the wave has been spawned and killed.
	if (EnemiesAlive <= 0 && EnemiesSpawnedSoFar >= TotalEnemiesInWave)
	{
		bIsWaveActive = false;
		UE_LOG(LogTemp, Log, TEXT("[IF-Wave] Wave %d complete."), CurrentWave);
		OnWaveCompleted.Broadcast(CurrentWave);
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

	CachePlayerAndStronghold();
	BindPlayerDelegates();

	// Bound in C++ so wave progression requires no Blueprint wiring: OnWaveCompleted fires
	// when the last enemy in a wave dies, and this immediately starts the next one.
	OnWaveCompleted.AddDynamic(this, &AIFWaveManager::HandleWaveCompleted);

	if (bAutoStartOnBeginPlay)
	{
		StartNextWave();
	}
}

void AIFWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

void AIFWaveManager::HandleWaveCompleted(int32 CompletedWaveNumber)
{
	StartNextWave();
}

void AIFWaveManager::CachePlayerAndStronghold()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		CachedPlayer = nullptr;
		CachedStronghold = nullptr;
		return;
	}

	CachedPlayer = Cast<AIFPlayerCharacter>(UGameplayStatics::GetActorOfClass(World, AIFPlayerCharacter::StaticClass()));

	if (const UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
	{
		CachedStronghold = Subsystem->GetStronghold();
	}
	else
	{
		CachedStronghold = nullptr;
	}

	if (!CachedPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IF-Wave] No AIFPlayerCharacter found in the level."));
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
		Combat->OnCombatStateChanged.AddDynamic(this, &AIFWaveManager::HandlePlayerCombatStateChanged);
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

void AIFWaveManager::HandlePlayerCombatStateChanged(ECombatState PreviousState, ECombatState NewState)
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