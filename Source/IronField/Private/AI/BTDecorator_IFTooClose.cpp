#include "AI/BTDecorator_IFTooClose.h"

#include "AI/IFBTUtils.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_IFTooClose::UBTDecorator_IFTooClose()
{
	NodeName = TEXT("Too Close");
	bNotifyTick = true;
	bAllowAbortLowerPri = true;
	bAllowAbortNone = true;
	FlowAbortMode = EBTFlowAbortMode::Both;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IFTooClose, TargetActorKey), AActor::StaticClass());
}

uint16 UBTDecorator_IFTooClose::GetInstanceMemorySize() const
{
	return sizeof(FIFBTConditionMemory);
}

void UBTDecorator_IFTooClose::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateConditionDecoratorAbort(OwnerComp, this, NodeMemory, CalculateRawConditionValue(OwnerComp, NodeMemory));
}

bool UBTDecorator_IFTooClose::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	const AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!Enemy || !Target)
	{
		return false;
	}

	const float Threshold = Enemy->GetCombatRange() * MinRangeFraction;
	const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), Target->GetActorLocation());
	return DistSq < FMath::Square(Threshold);
}
