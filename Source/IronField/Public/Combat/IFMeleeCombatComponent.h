#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFMeleeCombatComponent.generated.h"

UCLASS()
class IRONFIELD_API UIFMeleeCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	virtual float GetComboContinueChance(int32 ComboIndex) const override;

protected:
	virtual bool RequiresWeaponCollisionBox() const override { return true; }

private:
	/** Per-combo-step chance (0-1) to auto-continue after that step ends. Index matches ComboSteps. */
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|AI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	TArray<float> ComboContinueChances;
};
