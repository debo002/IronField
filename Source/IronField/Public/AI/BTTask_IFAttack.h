#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_IFAttack.generated.h"

/**
 * Behavior Tree task for handling latent AI combat swings.
 */
UCLASS()
class IRONFIELD_API UBTTask_IFAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IFAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;
};