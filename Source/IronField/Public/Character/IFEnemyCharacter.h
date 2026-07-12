#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "IFEnemyCharacter.generated.h"

UCLASS()
class IRONFIELD_API AIFEnemyCharacter : public AIFBaseCharacter
{
	GENERATED_BODY()

public:
	AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Exposed for Blueprint / behavior-tree tuning of approach distance.
	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	float GetEngagementRadius() const { return EngagementRadius; }

	void ApplyMovementSpeedForState(ECombatState State);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement")
	float EngagementRadius = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float ChaseSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float AttackingSpeed = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0"))
	float BlockingSpeed = 160.f;
};
