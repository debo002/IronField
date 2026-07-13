#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IFInAttackRange.generated.h"

UCLASS()
class IRONFIELD_API UBTDecorator_IFInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IFInAttackRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;

	// Fallback when the target is not an IIFAttackAreaProvider (e.g. the player).
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0"))
	float PlayerAttackRange = 120.f;
};
