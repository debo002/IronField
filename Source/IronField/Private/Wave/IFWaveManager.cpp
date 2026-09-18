#include "Wave/IFWaveManager.h"

#include "Building/IFStronghold.h"
#include "Character/IFBaseCharacter.h"
#include "Character/IFEnemyCharacter.h"
#include "Character/IFPlayerCharacter.h"
#include "Combat/IFMageCombatComponent.h"
#include "Combat/IFMeleeCombatComponent.h"
#include "Core/IFGameInstance.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerSubsystem.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Stats/IFHealthComponent.h"
#include "TimerManager.h"
#include "Wave/IFEnemySpawnPoint.h"

AIFWaveManager::AIFWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
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

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InterWaveTimerHandle);
	}

	CurrentWaveIndex++;

	// Unlimited never touches Waves — every wave is UnlimitedBaseWave scaled by index.
	if (IsUnlimitedRunMode())
	{
		BeginWaveFromDefinition(BuildUnlimitedWave(CurrentWaveIndex));
		return;
	}

	if (CurrentWaveIndex >= Waves.Num())
	{
		UE_LOG(LogIronField, Log, TEXT("[IF-Wave] All waves completed."));
		OnAllWavesCompleted.Broadcast();
		return;
	}

	BeginWaveFromDefinition(Waves[CurrentWaveIndex]);
}

void AIFWaveManager::BeginWaveFromDefinition(const FWaveDefinition& Wave)
{
	UnbindAllSpawnedEnemyDelegates();

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InterWaveTimerHandle);
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	EnemiesSpawnedSoFar = 0;
	EnemiesAlive = 0;
	TotalEnemiesInWave = 0;
	SpawnedEnemies.Empty();
	PendingSpawns.Reset();
	bIsWaveActive = true;

	const int32 WaveNumber = GetCurrentWave();
	UE_LOG(LogIronField, Log, TEXT("[IF-Wave] Starting wave %d."), WaveNumber);
	OnWaveStarted.Broadcast(WaveNumber);

	SpawnAllEnemiesInWave(Wave);
	CompleteWaveIfFinished();
}

void AIFWaveManager::ScheduleNextWave()
{
	UWorld* const World = GetWorld();
	if (!World || InterWaveDelaySeconds <= 0.f)
	{
		StartNextWave();
		return;
	}

	World->GetTimerManager().SetTimer(InterWaveTimerHandle, this, &AIFWaveManager::StartNextWave, InterWaveDelaySeconds, false);
}

void AIFWaveManager::ApplyWaveScaling(AIFBaseCharacter* Enemy) const
{
	if (!Enemy || !IsUnlimitedRunMode() || CurrentWaveIndex <= 0)
	{
		return;
	}

	const float HPScale = 1.f + UnlimitedHPScalePerWave * static_cast<float>(CurrentWaveIndex);
	const float DamageScale = 1.f + UnlimitedDamageScalePerWave * static_cast<float>(CurrentWaveIndex);

	if (UIFHealthComponent* const Health = Enemy->GetHealthComponent())
	{
		Health->SetMaxHealth(Health->GetMaxHealth() * HPScale);
	}

	if (UIFMeleeCombatComponent* const Melee = Enemy->FindComponentByClass<UIFMeleeCombatComponent>())
	{
		Melee->ApplyDamageScale(DamageScale);
	}
	else if (UIFMageCombatComponent* const Mage = Enemy->FindComponentByClass<UIFMageCombatComponent>())
	{
		Mage->ApplyDamageScale(DamageScale);
	}
}

FVector AIFWaveManager::PickSpawnLocation(FRotator& OutRotation) const
{
	OutRotation = GetActorRotation();
	FVector SpawnLocation = GetActorLocation();

	if (SpawnPoints.Num() > 0)
	{
		if (AActor* const SpawnPoint = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)])
		{
			SpawnLocation = SpawnPoint->GetActorLocation();
			OutRotation = SpawnPoint->GetActorRotation();
		}
	}

	if (SpawnLocationJitterRadius > 0.f)
	{
		SpawnLocation.X += FMath::RandRange(-SpawnLocationJitterRadius, SpawnLocationJitterRadius);
		SpawnLocation.Y += FMath::RandRange(-SpawnLocationJitterRadius, SpawnLocationJitterRadius);
	}

	return SpawnLocation;
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

void AIFWaveManager::CacheSpawnPoints()
{
	SpawnPoints.Empty();

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundPoints;
	UGameplayStatics::GetAllActorsOfClass(World, AIFEnemySpawnPoint::StaticClass(), FoundPoints);
	for (AActor* Point : FoundPoints)
	{
		SpawnPoints.Add(Point);
	}

	if (SpawnPoints.Num() <= 0)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] No AIFEnemySpawnPoint actors found; spawning at manager location."));
	}
}

void AIFWaveManager::SpawnAllEnemiesInWave(const FWaveDefinition& Wave)
{
	BuildSpawnQueue(Wave);

	UWorld* const World = GetWorld();
	if (World && PendingSpawns.Num() > 0 && SpawnIntervalSeconds > 0.f)
	{
		// First spawn waits out the wave banner so a wave never instant-contacts.
		World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AIFWaveManager::TrySpawnPending, SpawnIntervalSeconds, true, SpawnStartDelaySeconds);
	}
	else
	{
		// No timer possible (or nothing queued): spawn synchronously so the wave cannot stall.
		TrySpawnPending();
	}
}

void AIFWaveManager::BuildSpawnQueue(const FWaveDefinition& Wave)
{
	PendingSpawns.Reset();
	TotalEnemiesInWave = 0;

	for (const FEnemyGroupDefinition& Group : Wave.EnemyGroups)
	{
		if (!Group.EnemyClass)
		{
			UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Wave %d has an enemy group with no Enemy Class — skipping."), GetCurrentWave());
			continue;
		}

		for (int32 i = 0; i < Group.EnemyCount; ++i)
		{
			PendingSpawns.Add(Group.EnemyClass);
		}
	}

	TotalEnemiesInWave = PendingSpawns.Num();
}

void AIFWaveManager::TrySpawnPending()
{
	UWorld* const World = GetWorld();
	if (!World || !bIsWaveActive)
	{
		return;
	}

	const int32 CappedMax = FMath::Max(1, MaxConcurrentAlive);
	while (PendingSpawns.Num() > 0 && EnemiesAlive < CappedMax)
	{
		const TSubclassOf<AIFBaseCharacter> EnemyClass = PendingSpawns[0];
		PendingSpawns.RemoveAt(0);

		if (!EnemyClass)
		{
			TotalEnemiesInWave = FMath::Max(0, TotalEnemiesInWave - 1);
			continue;
		}

		FRotator SpawnRotation;
		const FVector SpawnLocation = PickSpawnLocation(SpawnRotation);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AIFBaseCharacter* const SpawnedEnemy = World->SpawnActor<AIFBaseCharacter>(EnemyClass, SpawnLocation, SpawnRotation, SpawnParams);
		if (SpawnedEnemy)
		{
			EnemiesSpawnedSoFar++;
			NotifyEnemySpawned(SpawnedEnemy);
		}
		else
		{
			TotalEnemiesInWave = FMath::Max(0, TotalEnemiesInWave - 1);
			UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Failed to spawn %s at %s"),
				*EnemyClass->GetName(), *SpawnLocation.ToString());
		}
	}

	if (PendingSpawns.Num() <= 0)
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	CompleteWaveIfFinished();
}

void AIFWaveManager::NotifyEnemySpawned(AIFBaseCharacter* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	ApplyWaveScaling(Enemy);

	SpawnedEnemies.Add(Enemy);
	EnemiesAlive++;
	OnEnemiesAliveCountChanged.Broadcast(EnemiesAlive);

	Enemy->OnCharacterDied.AddDynamic(this, &AIFWaveManager::HandleEnemyDied);
}

void AIFWaveManager::CompleteWaveIfFinished()
{
	if (!bIsWaveActive || EnemiesAlive > 0 || PendingSpawns.Num() > 0 || EnemiesSpawnedSoFar < TotalEnemiesInWave)
	{
		return;
	}

	if (TotalEnemiesInWave <= 0)
	{
		// Empty or all-invalid waves never spawn, so HandleEnemyDied cannot complete them.
		// The breather timer below keeps this from recursing into the next wave same-frame.
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Wave %d has no spawnable enemies; completing immediately."), GetCurrentWave());
	}

	bIsWaveActive = false;
	const int32 WaveNumber = GetCurrentWave();
	UE_LOG(LogIronField, Log, TEXT("[IF-Wave] Wave %d complete."), WaveNumber);
	OnWaveCompleted.Broadcast(WaveNumber);

	CleanupWaveCorpses();
	ScheduleNextWave();
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

	KillCount++;
	OnKillCountChanged.Broadcast(KillCount);

	// Refill concurrency from the queue as the player scores kills.
	TrySpawnPending();
	CompleteWaveIfFinished();
}

void AIFWaveManager::CleanupWaveCorpses()
{
	for (int32 Index = SpawnedEnemies.Num() - 1; Index >= 0; --Index)
	{
		AIFBaseCharacter* const Enemy = SpawnedEnemies[Index];
		if (!Enemy)
		{
			SpawnedEnemies.RemoveAt(Index);
			continue;
		}

		if (Enemy->IsDead())
		{
			Enemy->Destroy();
			SpawnedEnemies.RemoveAt(Index);
		}
	}
}

void AIFWaveManager::UnbindAllSpawnedEnemyDelegates()
{
	for (AIFBaseCharacter* Enemy : SpawnedEnemies)
	{
		if (Enemy)
		{
			Enemy->OnCharacterDied.RemoveDynamic(this, &AIFWaveManager::HandleEnemyDied);
		}
	}
}

void AIFWaveManager::BeginPlay()
{
	Super::BeginPlay();

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UIFWaveManagerSubsystem* const Subsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
	{
		Subsystem->RegisterWaveManager(this);
	}

	if (UIFPlayerSubsystem* const PlayerSubsystem = World->GetSubsystem<UIFPlayerSubsystem>())
	{
		if (AIFPlayerCharacter* const Player = PlayerSubsystem->GetPlayer())
		{
			HandlePlayerRegistered(Player);
		}
		else
		{
			PlayerSubsystem->OnPlayerRegistered.AddDynamic(this, &AIFWaveManager::HandlePlayerRegistered);
		}
	}

	CacheSpawnPoints();

	if (bAutoStartOnBeginPlay)
	{
		// Opening grace: the run breathes before wave 1 instead of spawning on frame one.
		World->GetTimerManager().SetTimer(InterWaveTimerHandle, this, &AIFWaveManager::StartNextWave, InitialWaveStartDelaySeconds, false);
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

		if (UIFPlayerSubsystem* const PlayerSubsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			PlayerSubsystem->OnPlayerRegistered.RemoveDynamic(this, &AIFWaveManager::HandlePlayerRegistered);
		}

		World->GetTimerManager().ClearTimer(InterWaveTimerHandle);
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	UnbindPlayer();
	UnbindAllSpawnedEnemyDelegates();

	Super::EndPlay(EndPlayReason);
}

void AIFWaveManager::HandlePlayerRegistered(AIFPlayerCharacter* Player)
{
	if (CachedPlayer)
	{
		return;
	}

	if (!Player)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Player registration arrived with no player instance."));
		return;
	}

	BindPlayer(Player);
}

void AIFWaveManager::BindPlayer(AIFPlayerCharacter* Player)
{
	if (!Player)
	{
		return;
	}

	CachedPlayer = Player;

	if (UIFHealthComponent* const Health = Player->GetHealthComponent())
	{
		Health->OnHealthDepleted.AddDynamic(this, &AIFWaveManager::HandlePlayerHealthDepleted);
	}
}

void AIFWaveManager::UnbindPlayer()
{
	if (!CachedPlayer)
	{
		return;
	}

	if (UIFHealthComponent* const Health = CachedPlayer->GetHealthComponent())
	{
		Health->OnHealthDepleted.RemoveDynamic(this, &AIFWaveManager::HandlePlayerHealthDepleted);
	}

	CachedPlayer = nullptr;
}

void AIFWaveManager::HandlePlayerHealthDepleted()
{
	OnPlayerDowned.Broadcast();
}
