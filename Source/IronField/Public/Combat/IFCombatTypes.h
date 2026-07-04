#pragma once

#include "CoreMinimal.h"
#include "IFCombatTypes.generated.h"

class UAnimMontage;
class UDamageType;

/* Current combat state. Used to block actions that don't make sense together
 (e.g. can't attack while dead, can't block while attacking).
*/
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle,
	Attacking,
	Blocking,
	Dead
};

/** One step in an attack combo: its animation, stamina cost, and damage. */
USTRUCT(BlueprintType)
struct FIFComboStep
{
	GENERATED_BODY()

	// Animation played for this step of the combo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Stamina used when this step is played.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep", meta = (ClampMin = "0.0"))
	float StaminaCost = 10.f;

	// Damage dealt if this step's swing connects with a target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep", meta = (ClampMin = "0.0"))
	float Damage = 20.f;

	// Type of damage dealt (for future resistance/armor systems).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep")
	TSubclassOf<UDamageType> DamageTypeClass;
};

// Fired when health reaches 0.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);

// Fired whenever health changes. Percent is 0-1.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, Percent);

// Fired when stamina reaches 0.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);

// Fired whenever stamina changes. Percent is 0-1.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, Percent);

// Fired when combat state changes (e.g. Idle -> Attacking).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChanged, ECombatState, PreviousState, ECombatState, NewState);