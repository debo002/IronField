#include "AI/BTDecorator_IFInAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Building/IFStronghold.h"

UBTDecorator_IFInAttackRange::UBTDecorator_IFInAttackRange()
{
	NodeName = TEXT("In Attack Range");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IFInAttackRange, TargetActorKey), AActor::StaticClass());
	MoveDestinationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IFInAttackRange, MoveDestinationKey));
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
	AActor* const TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!ControlledPawn || !TargetActor)
	{
		return false;
	}

	// Stronghold targets check distance to the reserved attack point (what the enemy walked
	// to), using the Stronghold-specific range. Other targets check distance directly.
	if (TargetActor->IsA<AIFStronghold>())
	{
		const FVector Destination = Blackboard->GetValueAsVector(MoveDestinationKey.SelectedKeyName);
		const float DistSq = FVector::DistSquared(ControlledPawn->GetActorLocation(), Destination);
		return DistSq <= FMath::Square(StrongholdAttackRange);
	}

	const float DistSq = FVector::DistSquared(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	return DistSq <= FMath::Square(PlayerAttackRange);
}