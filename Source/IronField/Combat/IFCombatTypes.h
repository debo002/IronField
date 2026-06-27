#pragma once

#include "CoreMinimal.h"
#include "IFCombatTypes.generated.h"

/**
 * Shared combat declarations, enumerations, and structures.
 * Defines combat states, combo step configurations, and event delegate signatures.
 */

class UAnimMontage;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
    Idle,
    Attacking,
    Blocking,
    Dead
};

/** Single step within a melee attack combo chain. */
USTRUCT(BlueprintType)
struct FIFComboStep
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep")
    TObjectPtr<UAnimMontage> AttackMontage;

    /** Stamina required to execute this combo step. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|ComboStep")
    float StaminaCost = 10.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, Percent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, Percent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChanged, ECombatState, PreviousState, ECombatState, NewState);
