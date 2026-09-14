#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_IFMoveToTarget.generated.h"

UCLASS()
class IRONFIELD_API UBTTask_IFMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IFMoveToTarget();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "IronField|AI|Targeting")
	FBlackboardKeySelector TargetActorKey;

	/** Seconds between repath requests while the target keeps moving. */
	UPROPERTY(EditAnywhere, Category = "IronField|AI|Movement", meta = (ClampMin = "0.05"))
	float RepathInterval = 0.25f;
};
