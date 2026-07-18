#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Combat/IFCombatTypes.h"
#include "IFEnemyController.generated.h"

class UBehaviorTree;
class UIFCombatComponent;
class UIFEnemyAIData;
class AIFWaveManager;

UCLASS()
class IRONFIELD_API AIFEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	bool IsReadyForNewAttack() const;

	UIFCombatComponent* GetControlledCombatComponent() const { return CachedCombatComponent; }

	/** Shared AI tunables. Never null at runtime — falls back to CDO defaults if unset (with a one-time warning). */
	const UIFEnemyAIData* GetAIData() const;

	FName GetTargetActorKeyName() const { return TargetActorKeyName; }

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UIFEnemyAIData> AIData;

	/** Must match the TargetActor key on BB_Enemy and every BT node selector. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FName TargetActorKeyName = TEXT("TargetActor");

private:
	float LastAttackEndedTime = -1.f;
	float CurrentReattackCooldownSeconds = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UIFCombatComponent> CachedCombatComponent;

	UPROPERTY(Transient)
	TObjectPtr<AIFWaveManager> CachedWaveManager;

	UFUNCTION()
	void HandleOwnCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

	UFUNCTION()
	void HandleOwnHealthDepleted();

	UFUNCTION()
	void HandlePlayerDied();

	void InitializeAfterPossession();
	void BindOwnDelegates();
	void UnbindOwnDelegates();
	void ApplyMovementSpeedForState(ECombatState State);
};
