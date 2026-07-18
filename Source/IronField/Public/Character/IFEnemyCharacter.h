#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "IFEnemyCharacter.generated.h"

/**
 * Shared enemy base. One CombatRange drives both "close enough to attack" and "stop moving".
 * Set low on melee (~120), high on mage (~900).
 */
UCLASS(Abstract)
class IRONFIELD_API AIFEnemyCharacter : public AIFBaseCharacter
{
	GENERATED_BODY()

public:
	AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	float GetCombatRange() const { return CombatRange; }

	void ApplyMovementSpeedForState(ECombatState State);

protected:
	/** Distance for both attacking and stopping MoveTo. Melee low, mage high. */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Combat", meta = (ClampMin = "1.0"))
	float CombatRange = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float ChaseSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float AttackingSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float BlockingSpeed = 160.f;
};
