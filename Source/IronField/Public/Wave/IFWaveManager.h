#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatTypes.h"
#include "GameFramework/Actor.h"
#include "IFWaveManager.generated.h"

class AIFPlayerCharacter;
class AIFStronghold;
class AIFBaseCharacter;

// Fired once when the Player's health hits 0. Lets enemies release the Player and retarget the
// Stronghold without each of them binding directly to the Player's health component.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

// Wave Manager events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCompleted, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemiesAliveCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesCompleted);

/**
 * Structured group definition of a specific enemy class and count to spawn.
 */
USTRUCT(BlueprintType)
struct FEnemyGroupDefinition
{
	GENERATED_BODY()

	// The C++ enemy class (inheriting from IFBaseCharacter) to spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TSubclassOf<AIFBaseCharacter> EnemyClass;

	// Total count of this enemy class to spawn in this wave.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1"))
	int32 EnemyCount = 5;
};

/**
 * Structured definition of a Wave. Supports spawning multiple enemy groups/types in the same wave.
 */
USTRUCT(BlueprintType)
struct FWaveDefinition
{
	GENERATED_BODY()

	// Array of enemy groups that compose this wave.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FEnemyGroupDefinition> EnemyGroups;
};

/**
 * Central authority for the enemy encounter. Holds the Player-engagement slot cap and the two
 * player-state timestamps enemy AI needs for targeting decisions. Now fully controls wave progression,
 * C++ internal spawning from placed spawn points, and lifecycle events.
 */
UCLASS()
class IRONFIELD_API AIFWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AIFWaveManager();

	UPROPERTY(BlueprintAssignable, Category = "WaveManager|Events")
	FOnPlayerDied OnPlayerDied;

	UPROPERTY(BlueprintAssignable, Category = "WaveManager|Events")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "WaveManager|Events")
	FOnWaveCompleted OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "WaveManager|Events")
	FOnEnemiesAliveCountChanged OnEnemiesAliveCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "WaveManager|Events")
	FOnAllWavesCompleted OnAllWavesCompleted;

	// Configurable array of wave definitions. Each wave composition is explicitly defined here.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaveManager|Config")
	TArray<FWaveDefinition> Waves;

	// If true, automatically starts Wave 1 when BeginPlay is executed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaveManager|Config")
	bool bAutoStartOnBeginPlay = true;

	// Total number of enemies that must be spawned in the current wave. Calculated automatically in C++.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 TotalEnemiesInWave = 0;

	// Total number of enemies spawned so far in the current wave. Tracked automatically in C++.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 EnemiesSpawnedSoFar = 0;

	// Current active wave number (1-based for display).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 CurrentWave = 0;

	// Number of enemies currently alive in the active wave.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 EnemiesAlive = 0;

	// Whether a wave is currently running.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	bool bIsWaveActive = false;

	// Returns true and reserves a slot if under the cap. The Stronghold has no equivalent - it's
	// uncapped, so there's nothing to reserve for it.
	UFUNCTION(BlueprintCallable, Category = "WaveManager|Engagement")
	bool TryReserveEngagementSlot();

	UFUNCTION(BlueprintCallable, Category = "WaveManager|Engagement")
	void ReleaseEngagementSlot();

	UFUNCTION(BlueprintPure, Category = "WaveManager|Targeting")
	AActor* GetPlayerActor() const;

	UFUNCTION(BlueprintPure, Category = "WaveManager|Targeting")
	AActor* GetStrongholdActor() const;

	UFUNCTION(BlueprintPure, Category = "WaveManager|Targeting")
	float GetLastPlayerAttackTime() const { return LastPlayerAttackTime; }

	UFUNCTION(BlueprintPure, Category = "WaveManager|Targeting")
	float GetLastPlayerReviveTime() const { return LastPlayerReviveTime; }

	// Advances wave index, loads wave definitions, and triggers internal spawner logic.
	UFUNCTION(BlueprintCallable, Category = "WaveManager|Actions")
	void StartNextWave();

	// Cleans up (destroys) all dead enemy characters currently stored in the wave's tracking list.
	UFUNCTION(BlueprintCallable, Category = "WaveManager|Actions")
	void CleanupWaveCorpses();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "WaveManager|Engagement")
	int32 MaxPlayerEngagementSlots = 3;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "WaveManager|Engagement", meta = (AllowPrivateAccess = "true"))
	int32 CurrentPlayerEngagementCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<AIFPlayerCharacter> CachedPlayer;

	UPROPERTY(Transient)
	TObjectPtr<AIFStronghold> CachedStronghold;

	// List of all enemies spawned in the current wave. Used to track dead ones for cleanup.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AIFBaseCharacter>> SpawnedEnemies;

	// 0-based index tracking the current wave state in Waves array
	int32 CurrentWaveIndex = -1;

	// -1 means "hasn't happened yet" - never within any recency window.
	float LastPlayerAttackTime = -1.f;
	float LastPlayerReviveTime = -1.f;

	// Internal function to register newly spawned enemies and set up death hooks.
	void NotifyEnemySpawned(AIFBaseCharacter* Enemy);

	// Spawns enemies defined in the wave definition from random spawn points.
	void SpawnEnemyFromWave(const FWaveDefinition& Wave);

	UFUNCTION()
	void HandlePlayerCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

	UFUNCTION()
	void HandlePlayerHealthDepleted();

	UFUNCTION()
	void HandleEnemyDied(AIFBaseCharacter* DeadEnemy);

	// Bound to OnWaveCompleted in BeginPlay so waves chain automatically in C++ without
	// requiring any Blueprint event binding.
	UFUNCTION()
	void HandleWaveCompleted(int32 CompletedWaveNumber);

	void CachePlayerAndStronghold();
	void BindPlayerDelegates();
	void UnbindPlayerDelegates();
};