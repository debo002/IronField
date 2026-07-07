#include "AI/BTDecorator_IFInAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/IFAttackAreaProvider.h"

namespace
{
	float ResolveAttackRange(const AActor& TargetActor, float DefaultRange)
	{
		if (const IIFAttackAreaProvider* const Provider = Cast<IIFAttackAreaProvider>(&TargetActor))
		{
			return Provider->GetAttackAreaRange();
		}

		return DefaultRange;
	}
}

UBTDecorator_IFInAttackRange::UBTDecorator_IFInAttackRange()
{
	NodeName = TEXT("In Attack Range");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IFInAttackRange, TargetActorKey), AActor::StaticClass());
}

bool UBTDecorator_IFInAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* const AIController = OwnerComp.GetAIOwner();
	if (!Blackboard || !AIController)
	{
		return false;
	}

	const APawn* const ControlledPawn = AIController->GetPawn();
	const AActor* const TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!ControlledPawn || !TargetActor)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const float Range = ResolveAttackRange(*TargetActor, PlayerAttackRange);

	return DistSq <= FMath::Square(Range);
}
