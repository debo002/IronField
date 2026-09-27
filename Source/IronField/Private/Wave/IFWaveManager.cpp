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
#include "Core/IFFeedbackUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/IFHealPickup.h"
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
		if (UWorld* const World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(SpawnTimerHandle);
			World->GetTimerManager().ClearTimer(ValidateTimerHandle);
		}
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
		World->GetTimerManager().ClearTimer(ValidateTimerHandle);
	}

	// Invalidate any timer callbacks scheduled under the previous wave.
	WaveGeneration++;

	EnemiesSpawnedSoFar = 0;
	EnemiesAlive = 0;
	TotalEnemiesInWave = 0;
	SpawnedEnemies.Empty();
	PendingSpawns.Reset();
	bIsWaveActive = true;

	const int32 WaveNumber = GetCurrentWave();
	UE_LOG(LogIronField, Log, TEXT("[IF-Wave] Starting wave %d."), WaveNumber);
	OnWaveStarted.Broadcast(WaveNumber);
	PlayWaveStartFeedback();

	// Failsafe: reconcile the alive-count against live enemies while the wave runs,
	// so removals that bypass OnCharacterDied cannot stall the wave forever.
	if (UWorld* const World = GetWorld())
	{
		if (WaveValidationIntervalSeconds > 0.f)
		{
			FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AIFWaveManager::HandleValidateTimer, WaveGeneration);
			World->GetTimerManager().SetTimer(ValidateTimerHandle, Delegate, WaveValidationIntervalSeconds, true, WaveValidationIntervalSeconds);
		}
	}

	SpawnAllEnemiesInWave(Wave);
	CompleteWaveIfFinished();
}

void AIFWaveManager::PlayWaveStartFeedback() const
{
	IFFeedbackUtils::PlayAtLocation(GetWorld(), WaveStartSound, nullptr, GetActorLocation());
}

void AIFWaveManager::ScheduleNextWave()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		StartNextWave();
		return;
	}

	if (InterWaveDelaySeconds <= 0.f)
	{
		// Never recurse same-frame: a chain of empty waves would deepen the stack
		// once per wave. A single-tick defer keeps the rhythm tight without recursion.
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AIFWaveManager::HandleInterWaveTimer, WaveGeneration);
		World->GetTimerManager().SetTimer(InterWaveTimerHandle, Delegate, 0.01f, false);
		return;
	}

	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AIFWaveManager::HandleInterWaveTimer, WaveGeneration);
	World->GetTimerManager().SetTimer(InterWaveTimerHandle, Delegate, InterWaveDelaySeconds, false);
}

void AIFWaveManager::HandleInterWaveTimer(uint32 ScheduledGeneration)
{
	if (ScheduledGeneration != WaveGeneration)
	{
		return;
	}
	StartNextWave();
}

void AIFWaveManager::HandleSpawnTimerTick(uint32 ScheduledGeneration)
{
	if (ScheduledGeneration != WaveGeneration)
	{
		if (UWorld* const World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		}
		return;
	}
	TrySpawnPending();
}

void AIFWaveManager::HandleValidateTimer(uint32 ScheduledGeneration)
{
	if (ScheduledGeneration != WaveGeneration)
	{
		if (UWorld* const World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ValidateTimerHandle);
		}
		return;
	}

	if (!bIsWaveActive)
	{
		return;
	}

	ValidateWaveState();
}

void AIFWaveManager::ValidateWaveState()
{
	int32 ActualAlive = 0;
	for (int32 Index = SpawnedEnemies.Num() - 1; Index >= 0; --Index)
	{
		AIFBaseCharacter* const Enemy = SpawnedEnemies[Index];
		if (!IsValid(Enemy))
		{
			SpawnedEnemies.RemoveAt(Index);
			continue;
		}

		if (!Enemy->IsDead())
		{
			ActualAlive++;
		}
	}

	if (ActualAlive != EnemiesAlive)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Wave] Wave %d alive-count mismatch: tracked=%d actual=%d; reconciling."),
			GetCurrentWave(), EnemiesAlive, ActualAlive);
		EnemiesAlive = ActualAlive;
		OnEnemiesAliveCountChanged.Broadcast(EnemiesAlive);
	}

	// Refill the trickle queue and unstall completion if the mismatch held either back.
	TrySpawnPending();
	CompleteWaveIfFinished();
}

void AIFWaveManager::ApplyWaveScaling(AIFBaseCharacter* Enemy) const
{
	// No stat scaling in Unlimited mode — only enemy count increases via BuildUnlimitedWave
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
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AIFWaveManager::HandleSpawnTimerTick, WaveGeneration);
		World->GetTimerManager().SetTimer(SpawnTimerHandle, Delegate, SpawnIntervalSeconds, true, SpawnStartDelaySeconds);
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

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ValidateTimerHandle);
	}

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

	// Spawn heal pickup on death
	TrySpawnHealPickup(DeadEnemy);
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
	SpawnedEnemies.Empty();
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

	OnWaveCompleted.AddDynamic(this, &AIFWaveManager::HandleWaveClearHeal);
	bWaveCompletedBound = true;

	if (bAutoStartOnBeginPlay)
	{
		// Opening grace: the run breathes before wave 1 instead of spawning on frame one.
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AIFWaveManager::HandleInterWaveTimer, WaveGeneration);
		World->GetTimerManager().SetTimer(InterWaveTimerHandle, Delegate, InitialWaveStartDelaySeconds, false);
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
		World->GetTimerManager().ClearTimer(ValidateTimerHandle);
	}

	// Invalidate timer callbacks already in flight during teardown.
	WaveGeneration++;

	if (bWaveCompletedBound)
	{
		OnWaveCompleted.RemoveDynamic(this, &AIFWaveManager::HandleWaveClearHeal);
		bWaveCompletedBound = false;
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

void AIFWaveManager::HandleWaveClearHeal(int32 WaveNumber)
{
	UE_LOG(LogIronField, Log, TEXT("[IF-Heal] Wave %d clear - applying heal/repair."), WaveNumber);

	float PlayerHealed = 0.f;
	float GateRepaired = 0.f;

	if (UWorld* const World = GetWorld())
	{
		// Heal player
		if (UIFPlayerSubsystem* const PlayerSubsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			if (AIFPlayerCharacter* const Player = PlayerSubsystem->GetPlayer())
			{
				if (UIFHealthComponent* const Health = Player->GetHealthComponent())
				{
					if (!Health->IsDead())
					{
						const float HealAmount = Health->GetMaxHealth() * PlayerHealPercent;
						Health->ApplyHealing(HealAmount);
						PlayerHealed = HealAmount;
						UE_LOG(LogIronField, Log, TEXT("[IF-Heal] Player healed %.0f (%.0f%% of %.0f max)."), HealAmount, PlayerHealPercent * 100.f, Health->GetMaxHealth());

					if (HealSound)
					{
						IFFeedbackUtils::PlayAtLocation(World, HealSound, nullptr, Player->GetActorLocation());
					}
					}
					else
					{
						UE_LOG(LogIronField, Log, TEXT("[IF-Heal] Player dead - skipping heal."));
					}
				}
			}
		}

		// Repair stronghold
		if (AActor* const StrongholdActor = GetStrongholdActor())
		{
			if (UIFHealthComponent* const Health = StrongholdActor->FindComponentByClass<UIFHealthComponent>())
			{
				if (!Health->IsDead())
				{
					const float RepairAmount = Health->GetMaxHealth() * StrongholdRepairPercent;
					Health->ApplyHealing(RepairAmount);
					GateRepaired = RepairAmount;
					UE_LOG(LogIronField, Log, TEXT("[IF-Heal] Stronghold repaired %.0f (%.0f%% of %.0f max)."), RepairAmount, StrongholdRepairPercent * 100.f, Health->GetMaxHealth());

					if (HealSound)
					{
						IFFeedbackUtils::PlayAtLocation(World, HealSound, nullptr, StrongholdActor->GetActorLocation());
					}
				}
				else
				{
					UE_LOG(LogIronField, Log, TEXT("[IF-Heal] Stronghold destroyed - skipping repair."));
				}
			}
		}
	}

	OnWaveClearHeal.Broadcast(WaveNumber, PlayerHealed, GateRepaired);
}

void AIFWaveManager::TrySpawnHealPickup(AIFBaseCharacter* DeadEnemy)
{
	if (!DeadEnemy || !HealPickupClass || DropChance <= 0.f)
	{
		return;
	}

	if (FMath::FRand() > DropChance)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector SpawnLocation = DeadEnemy->GetActorLocation();
	// Trace down to find ground so pickup sits on floor, not floating at capsule base + 50
	FHitResult HitResult;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PickupSpawnTrace), false, DeadEnemy);
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.bTraceComplex = true;
	if (World->LineTraceSingleByChannel(HitResult, SpawnLocation, SpawnLocation - FVector(0.f, 0.f, 500.f), ECC_WorldStatic, TraceParams))
	{
		SpawnLocation = HitResult.Location;
	}
	else
	{
		SpawnLocation.Z += 10.f; // slight offset above ground if no trace hit
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AIFHealPickup* const Pickup = World->SpawnActor<AIFHealPickup>(HealPickupClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (Pickup)
	{
		Pickup->HealAmount = PickupHealAmount;
		UE_LOG(LogIronField, Log, TEXT("[IF-Heal] Spawned heal pickup at %s (amount=%.0f)."), *SpawnLocation.ToString(), PickupHealAmount);
	}
}
