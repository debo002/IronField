#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_IFMoveAwayFromTarget.generated.h"

UCLASS()
class IRONFIELD_API UBTTask_IFMoveAwayFromTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IFMoveAwayFromTarget();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;
};
