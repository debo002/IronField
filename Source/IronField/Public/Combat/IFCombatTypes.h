#pragma once

#include "CoreMinimal.h"
#include "IFCombatTypes.generated.h"

class UAnimMontage;
class UDamageType;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle,
	Attacking,
	Blocking,
	Dead
};

/** One step in a melee combo. Shared by player and melee enemies — no AI fields. */
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
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, Percent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, Percent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChanged, ECombatState, PreviousState, ECombatState, NewState);
