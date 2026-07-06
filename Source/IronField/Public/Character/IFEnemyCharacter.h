#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "IFEnemyCharacter.generated.h"

/**
 * Enemy melee character. No player input, no camera - just combat and movement, driven entirely
 * by AIFEnemyController and its Behavior Tree. Never revives; inherits AIFBaseCharacter's
 * death behavior (die, stay dead) as-is.
 */
UCLASS()
class IRONFIELD_API AIFEnemyCharacter : public AIFBaseCharacter
{
	GENERATED_BODY()

public:
	AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Acceptance radius used by movement tasks when approaching a target.
	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	float GetEngagementRadius() const { return EngagementRadius; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement")
	float EngagementRadius = 120.f;
};
