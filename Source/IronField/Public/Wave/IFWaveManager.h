#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IFWaveManager.generated.h"

class AIFPlayerCharacter;
class AIFBaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveCompleted, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemiesAliveCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesCompleted);

USTRUCT(BlueprintType)
struct FEnemyGroupDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|Wave")
	TSubclassOf<AIFBaseCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|Wave", meta = (ClampMin = "1"))
	int32 EnemyCount = 5;
};

USTRUCT(BlueprintType)
struct FWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|Wave")
	TArray<FEnemyGroupDefinition> EnemyGroups;
};

UCLASS()
class IRONFIELD_API AIFWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AIFWaveManager();

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnPlayerDied OnPlayerDied;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnWaveCompleted OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnEnemiesAliveCountChanged OnEnemiesAliveCountChanged;

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
	float UnlimitedEnemyCountScaleFactor = 1.25f;

	/** Random XY offset applied around spawn points so enemies don't stack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IronField|Wave|Config", meta = (ClampMin = "0.0"))
	float SpawnLocationJitterRadius = 40.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 TotalEnemiesInWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 EnemiesSpawnedSoFar = 0;

	// 1-based wave number for UI. Derived from zero-based CurrentWaveIndex.
	UFUNCTION(BlueprintPure, Category = "IronField|Wave|State")
	int32 GetCurrentWave() const { return CurrentWaveIndex + 1; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	int32 EnemiesAlive = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	bool bIsWaveActive = false;

	// True between OnWaveCompleted and BeginNextWave — gates a future shop/rest phase.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Wave|State")
	bool bWaitingForNextWave = false;

	UFUNCTION(BlueprintPure, Category = "IronField|Wave|Targeting")
	AActor* GetPlayerActor() const;

	UFUNCTION(BlueprintPure, Category = "IronField|Wave|Targeting")
	AActor* GetStrongholdActor() const;

	UFUNCTION(BlueprintCallable, Category = "IronField|Wave|Actions")
	void StartNextWave();

	// Call after bWaitingForNextWave becomes true (e.g. from shop/rest UI) to start the next wave.
	UFUNCTION(BlueprintCallable, Category = "IronField|Wave|Actions")
	void BeginNextWave();

	UFUNCTION(BlueprintCallable, Category = "IronField|Wave|Actions")
	void CleanupWaveCorpses();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFPlayerCharacter> CachedPlayer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AIFBaseCharacter>> SpawnedEnemies;

	int32 CurrentWaveIndex = -1;

	void NotifyEnemySpawned(AIFBaseCharacter* Enemy);
	void SpawnAllEnemiesInWave(const FWaveDefinition& Wave);
	bool IsUnlimitedRunMode() const;
	FWaveDefinition BuildUnlimitedWave(int32 WaveIndex) const;
	void BeginWaveFromDefinition(const FWaveDefinition& Wave);
	void CompleteWaveIfFinished();
	void UnbindAllSpawnedEnemyDelegates();

	UFUNCTION()
	void HandlePlayerHealthDepleted();

	UFUNCTION()
	void HandleEnemyDied(AIFBaseCharacter* DeadEnemy);

	UFUNCTION()
	void HandleWaveCompleted(int32 CompletedWaveNumber);

	void InitializePlayerBindings();
	void CachePlayer();
	void BindPlayerDelegates();
	void UnbindPlayerDelegates();
};
