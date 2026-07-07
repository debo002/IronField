#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatTypes.h"
#include "GameFramework/Actor.h"
#include "IFWaveManager.generated.h"

class AIFPlayerCharacter;
class AIFStronghold;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TSubclassOf<AIFBaseCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1"))
	int32 EnemyCount = 5;
};

USTRUCT(BlueprintType)
struct FWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<FEnemyGroupDefinition> EnemyGroups;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaveManager|Config")
	TArray<FWaveDefinition> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaveManager|Config")
	bool bAutoStartOnBeginPlay = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 TotalEnemiesInWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 EnemiesSpawnedSoFar = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 CurrentWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	int32 EnemiesAlive = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WaveManager|State")
	bool bIsWaveActive = false;

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

	UFUNCTION(BlueprintCallable, Category = "WaveManager|Actions")
	void StartNextWave();

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

	UPROPERTY(Transient)
	TArray<TObjectPtr<AIFBaseCharacter>> SpawnedEnemies;

	int32 CurrentWaveIndex = -1;

	// Negative timestamps keep recency checks false until the player has actually acted.
	float LastPlayerAttackTime = -1.f;
	float LastPlayerReviveTime = -1.f;

	void NotifyEnemySpawned(AIFBaseCharacter* Enemy);

	void SpawnEnemyFromWave(const FWaveDefinition& Wave);

	UFUNCTION()
	void HandlePlayerCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

	UFUNCTION()
	void HandlePlayerHealthDepleted();

	UFUNCTION()
	void HandleEnemyDied(AIFBaseCharacter* DeadEnemy);

	UFUNCTION()
	void HandleWaveCompleted(int32 CompletedWaveNumber);

	void CachePlayerAndStronghold();
	void BindPlayerDelegates();
	void UnbindPlayerDelegates();
};
