#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "IFWaveManager.generated.h"

class AIFPlayerCharacter;
class AIFBaseCharacter;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDowned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCompleted, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemiesAliveCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesCompleted);

USTRUCT(BlueprintType)
struct FEnemyGroupDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|Wave|Config")
	TSubclassOf<AIFBaseCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|Wave|Config", meta = (ClampMin = "1"))
	int32 EnemyCount = 5;
};

USTRUCT(BlueprintType)
struct FWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|Wave|Config")
	TArray<FEnemyGroupDefinition> EnemyGroups;
};

UCLASS()
class IRONFIELD_API AIFWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AIFWaveManager();

	// Broadcast each time the player's health is depleted. The player revives afterwards,
	// so enemies use this to drop their current target, not to end the run.
	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnPlayerDowned OnPlayerDowned;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnWaveCompleted OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnEnemiesAliveCountChanged OnEnemiesAliveCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnKillCountChanged OnKillCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnAllWavesCompleted OnAllWavesCompleted;

	// Authored sequence for Normal mode only. Unused in Unlimited mode.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config")
	TArray<FWaveDefinition> Waves;

	// Single base definition for Unlimited mode. Every wave is this definition scaled by wave index.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config")
	FWaveDefinition UnlimitedBaseWave;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config")
	bool bAutoStartOnBeginPlay = true;

	// Multiplier applied to UnlimitedBaseWave enemy counts per wave (wave 1 = factor^0, wave 2 = factor^1, ...).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "1.0"))
	float UnlimitedEnemyCountScaleFactor = 1.15f;

	/** Random XY offset applied around spawn points so enemies do not stack on one spot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float SpawnLocationJitterRadius = 150.f;

	/** Breather between waves so the run has rhythm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float InterWaveDelaySeconds = 8.f;

	/** Stagger between spawns so the whole wave never pops in one frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.05"))
	float SpawnIntervalSeconds = 0.5f;

	/** Wait before the first spawn of each wave so the banner reads before contact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float SpawnStartDelaySeconds = 2.5f;

	/** Grace after level load before wave 1 starts (auto-start only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float InitialWaveStartDelaySeconds = 5.f;

	/** Cap on simultaneous alive enemies; the queue trickles in as they die. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "1"))
	int32 MaxConcurrentAlive = 5;

	/** How often the active wave reconciles its alive-count against live enemies. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float WaveValidationIntervalSeconds = 2.f;

	/** Unlimited stat pressure per wave index (wave 1 = unscaled). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float UnlimitedHPScalePerWave = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float UnlimitedDamageScalePerWave = 0.03f;

	// Empty until assigned in Blueprint; guarded at play time.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Feedback")
	TObjectPtr<USoundBase> WaveStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Feedback")
	TObjectPtr<UNiagaraSystem> WaveStartVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 TotalEnemiesInWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 EnemiesSpawnedSoFar = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 EnemiesAlive = 0;

	// Cumulative kills for this run. Never reset per wave (a new run is a new
	// manager actor, so the initializer is the reset). Drives HUD KILLS + stats.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 KillCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	bool bIsWaveActive = false;

	// 1-based wave number for UI. Derived from the zero-based CurrentWaveIndex.
	UFUNCTION(BlueprintPure, Category = "IronField|Wave|State")
	int32 GetCurrentWave() const { return CurrentWaveIndex + 1; }

	UFUNCTION(BlueprintPure, Category = "IronField|Wave|State")
	int32 GetKillCount() const { return KillCount; }

	UFUNCTION(BlueprintPure, Category = "IronField|Wave|Targeting")
	AActor* GetPlayerActor() const;

	UFUNCTION(BlueprintPure, Category = "IronField|Wave|Targeting")
	AActor* GetStrongholdActor() const;

	UFUNCTION(BlueprintCallable, Category = "IronField|Wave|Actions")
	void StartNextWave();

	/** Personality shift for the current wave: 1 = full siege (opening), 0 = even split. */
	UFUNCTION(BlueprintPure, Category = "IronField|Wave|Targeting")
	float GetSiegeBiasForWave() const { return CurrentWaveIndex <= 0 ? 1.f : 0.f; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFPlayerCharacter> CachedPlayer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AIFBaseCharacter>> SpawnedEnemies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnPoints;

	int32 CurrentWaveIndex = -1;

	/** Flattened spawn queue for the active wave; trickled out by the spawn timer. */
	UPROPERTY(Transient)
	TArray<TSubclassOf<AIFBaseCharacter>> PendingSpawns;

	FTimerHandle InterWaveTimerHandle;
	FTimerHandle SpawnTimerHandle;
	FTimerHandle ValidateTimerHandle;

	// Generation counter for async identity: bumped on every BeginWaveFromDefinition
	// and EndPlay; timer delegates capture the scheduling generation and drop stale firings.
	uint32 WaveGeneration = 0;

	void NotifyEnemySpawned(AIFBaseCharacter* Enemy);
	void SpawnAllEnemiesInWave(const FWaveDefinition& Wave);
	void BuildSpawnQueue(const FWaveDefinition& Wave);
	UFUNCTION()
	void TrySpawnPending();
	UFUNCTION()
	void HandleInterWaveTimer(uint32 ScheduledGeneration);
	UFUNCTION()
	void HandleSpawnTimerTick(uint32 ScheduledGeneration);
	UFUNCTION()
	void HandleValidateTimer(uint32 ScheduledGeneration);
	void ScheduleNextWave();
	void ApplyWaveScaling(AIFBaseCharacter* Enemy) const;
	FVector PickSpawnLocation(FRotator& OutRotation) const;
	bool IsUnlimitedRunMode() const;
	FWaveDefinition BuildUnlimitedWave(int32 WaveIndex) const;
	void BeginWaveFromDefinition(const FWaveDefinition& Wave);
	void PlayWaveStartFeedback() const;
	void CompleteWaveIfFinished();
	void ValidateWaveState();
	void CleanupWaveCorpses();
	void UnbindAllSpawnedEnemyDelegates();
	void CacheSpawnPoints();

	UFUNCTION()
	void HandlePlayerRegistered(AIFPlayerCharacter* Player);

	UFUNCTION()
	void HandlePlayerHealthDepleted();

	UFUNCTION()
	void HandleEnemyDied(AIFBaseCharacter* DeadEnemy);

	void BindPlayer(AIFPlayerCharacter* Player);
	void UnbindPlayer();
};
