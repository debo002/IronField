#include "AI/BTTask_IFMoveToTarget.h"

#include "AIController.h"
#include "Building/IFStronghold.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_IFMoveToTarget::UBTTask_IFMoveToTarget()
{
	NodeName = TEXT("Move To Target");
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFMoveToTarget, TargetActorKey), AActor::StaticClass());
	MoveDestinationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFMoveToTarget, MoveDestinationKey));
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
		UE_LOG(LogTemp, Warning, TEXT("[IF-AI] MoveToTarget: TargetActorKey is unset on this node."));
		return EBTNodeResult::Failed;
	}

	AActor* const TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	EPathFollowingRequestResult::Type RequestResult;

	if (TargetActor->IsA<AIFStronghold>())
	{
		FName KeyName = MoveDestinationKey.SelectedKeyName;
		if (KeyName.IsNone())
		{
			KeyName = TEXT("MoveDestination");
		}

		const FVector Destination = Blackboard->GetValueAsVector(KeyName);

		FAIMoveRequest MoveReq;
		MoveReq.SetGoalLocation(Destination);
		MoveReq.SetAcceptanceRadius(AcceptanceRadius);
		MoveReq.SetProjectGoalLocation(true);
		MoveReq.SetUsePathfinding(true);
		MoveReq.SetAllowPartialPath(true);
		RequestResult = AIController->MoveTo(MoveReq);
	}
	else
	{
		RequestResult = AIController->MoveToActor(TargetActor, AcceptanceRadius);
	}

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