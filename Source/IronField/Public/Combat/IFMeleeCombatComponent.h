#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFMeleeCombatComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFMeleeCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	virtual float GetComboContinueChance(int32 ComboIndex) const override;

protected:
	virtual bool ShouldReactivelyBlock(bool bFacingAttacker) const override;

private:
	/** Per-combo-step chance (0–1) to auto-continue after that step ends. Index matches ComboSteps. AI only. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|AI", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	TArray<float> ComboContinueChances;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float ReactiveBlockChance = 0.2f;
};
