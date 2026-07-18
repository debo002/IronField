#include "AI/BTDecorator_IFInAttackRange.h"

#include "AI/IFBTUtils.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_IFInAttackRange::UBTDecorator_IFInAttackRange()
{
	NodeName = TEXT("In Attack Range");
	bNotifyTick = true;
	bAllowAbortLowerPri = true;
	bAllowAbortNone = true;
	FlowAbortMode = EBTFlowAbortMode::Both;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IFInAttackRange, TargetActorKey), AActor::StaticClass());
}

uint16 UBTDecorator_IFInAttackRange::GetInstanceMemorySize() const
{
	return sizeof(FIFBTConditionMemory);
}

void UBTDecorator_IFInAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateConditionDecoratorAbort(OwnerComp, this, NodeMemory, CalculateRawConditionValue(OwnerComp, NodeMemory));
}

bool UBTDecorator_IFInAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	const AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!Enemy || !Target)
	{
		return false;
	}

	return IsWithinRange(Enemy, Target, Enemy->GetCombatRange());
}
