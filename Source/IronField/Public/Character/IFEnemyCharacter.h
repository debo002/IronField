#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "IFEnemyCharacter.generated.h"

/**
 * Shared enemy base. CombatRange is the extra gap between collision surfaces
 * at which AI attacks; configure it per enemy Blueprint to match its weapon.
 */
UCLASS(Abstract)
class IRONFIELD_API AIFEnemyCharacter : public AIFBaseCharacter
{
	GENERATED_BODY()

public:
	AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "IronField|Enemy|Combat")
	float GetCombatRange() const { return CombatRange; }

	void ApplyMovementSpeedForState(ECombatState State);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Combat", meta = (ClampMin = "1.0", ToolTip = "Extra distance allowed between the enemy and target collision surfaces before attacking. Set this to the weapon's actual reach in the enemy Blueprint."))
	float CombatRange = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Movement", meta = (ClampMin = "0.0"))
	float ChaseSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Movement", meta = (ClampMin = "0.0"))
	float AttackingSpeed = 150.f;
};
