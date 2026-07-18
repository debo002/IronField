#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_IFInAttackRange.generated.h"

UCLASS()
class IRONFIELD_API UBTDecorator_IFInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IFInAttackRange();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;
};
