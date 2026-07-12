#include "AI/BTTask_IFMoveToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/IFBTAttackAreaUtils.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_IFMoveToTarget::UBTTask_IFMoveToTarget()
{
	NodeName = TEXT("Move To Target");
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFMoveToTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_IFMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* const AIController = OwnerComp.GetAIOwner();
	const UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	if (TargetActorKey.SelectedKeyName.IsNone())
	{
		return EBTNodeResult::Failed;
	}

	AActor* const TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	const float Radius = ResolveAcceptanceRadius(*TargetActor, AcceptanceRadius);

	// Radius already includes target geometry; bStopOnOverlap would expand it again and stop too early.
	const EPathFollowingRequestResult::Type RequestResult = AIController->MoveToActor(TargetActor, Radius, false);

	switch (RequestResult)
	{
	case EPathFollowingRequestResult::Failed:
		return EBTNodeResult::Failed;

	case EPathFollowingRequestResult::AlreadyAtGoal:
		return EBTNodeResult::Succeeded;

	case EPathFollowingRequestResult::RequestSuccessful:
	default:
		return EBTNodeResult::InProgress;
	}
}

void UBTTask_IFMoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* const AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
