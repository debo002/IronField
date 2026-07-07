#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFEnemyCombatComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFEnemyCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

protected:
	virtual bool ShouldReactivelyBlock(bool bFacingAttacker) const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float ReactiveBlockChance = 0.2f;
};
