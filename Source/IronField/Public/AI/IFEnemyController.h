#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/IFCombatTypes.h"
#include "IFEnemyController.generated.h"

class UBehaviorTree;
class UIFCombatComponent;
class AIFWaveManager;

UCLASS()
class IRONFIELD_API AIFEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AIFEnemyController();

	bool IsReadyForNewAttack();

	UIFCombatComponent* GetControlledCombatComponent() const;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Combat")
	float MinReattackCooldownSeconds = 1.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Combat")
	float MaxReattackCooldownSeconds = 2.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Combat")
	float BlockReactionChance = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Combat")
	float MinBlockHoldSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Combat")
	float MaxBlockHoldSeconds = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement")
	float ChaseSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement")
	float AttackingSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement")
	float BlockingSpeed = 160.f;

private:
	float LastAttackEndedTime = -1.f;

	UPROPERTY(Transient)
	TObjectPtr<AIFWaveManager> CachedWaveManager;

	FTimerHandle BlockHoldTimerHandle;
	FTimerHandle PostPossessionInitTimerHandle;
	FDelegateHandle TargetObserverHandle;

	UFUNCTION()
	void HandleOwnCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

	UFUNCTION()
	void HandleComboStepStarted(int32 ComboIndex);

	UFUNCTION()
	void HandleOwnHealthDepleted();

	UFUNCTION()
	void HandlePlayerCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

	UFUNCTION()
	void HandlePlayerDied();

	EBlackboardNotificationResult HandleTargetBlackboardChanged(const UBlackboardComponent& TargetBlackboard, FBlackboard::FKey KeyID);

	// OnPossess can run before BeginPlay on world actors the behavior tree depends on.
	void InitializeAfterPossession();

	void BindOwnDelegates();
	void UnbindOwnDelegates();

	void BindPlayerBlockingDelegates();
	void UnbindPlayerBlockingDelegates();

	void StopBlockAfterHold();

	void ApplyMovementSpeedForState(ECombatState State);
};
