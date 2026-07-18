#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_IFTooClose.generated.h"

/** True when closer than CombatRange * MinRangeFraction (mage kite band). */
UCLASS()
class IRONFIELD_API UBTDecorator_IFTooClose : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IFTooClose();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;

	/** Fraction of CombatRange that counts as "too close". 0.5 = half combat range. */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinRangeFraction = 0.5f;
};
