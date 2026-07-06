#pragma once

#include "CoreMinimal.h"
#include "IFCombatTypes.generated.h"

class UAnimMontage;
class UDamageType;

// Current combat state. Blocks actions that don't make sense together (e.g. can't attack while dead).
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle,
	Attacking,
	Blocking,
	Dead
};

// One step in an attack combo: its animation, cost, damage, and chance to chain into the next step.
USTRUCT(BlueprintType)
struct FIFComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep", meta = (ClampMin = "0.0"))
	float StaminaCost = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep", meta = (ClampMin = "0.0"))
	float Damage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep")
	TSubclassOf<UDamageType> DamageTypeClass;

	// Chance the AI queues this step's follow-up when this step lands (or completes, for AI purposes).
	// Only used by AI-driven combat; ignored for player input, which chains on button presses instead.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AIContinueChance = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, Percent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, Percent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChanged, ECombatState, PreviousState, ECombatState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboStepStarted, int32, ComboIndex);