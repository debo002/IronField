#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "IFEnemyCharacter.generated.h"

UCLASS()
class IRONFIELD_API AIFEnemyCharacter : public AIFBaseCharacter
{
	GENERATED_BODY()

public:
	AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	float GetEngagementRadius() const { return EngagementRadius; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Movement")
	float EngagementRadius = 120.f;
};
