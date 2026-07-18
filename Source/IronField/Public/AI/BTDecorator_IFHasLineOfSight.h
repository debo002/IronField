#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_IFHasLineOfSight.generated.h"

UCLASS()
class IRONFIELD_API UBTDecorator_IFHasLineOfSight : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IFHasLineOfSight();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;

	/** Vertical offset applied to both ends of the visibility trace (approximate eye height). */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0"))
	float TraceHeightOffset = 50.f;
};
