#include "AI/BTTask_IFMoveToTarget.h"

#include "AIController.h"
#include "AI/IFBTUtils.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Character/IFEnemyCharacter.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_IFMoveToTarget::UBTTask_IFMoveToTarget()
{
	NodeName = TEXT("Move To Target");
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFMoveToTarget, TargetActorKey), AActor::StaticClass());
}

uint16 UBTTask_IFMoveToTarget::GetInstanceMemorySize() const
{
	return sizeof(float);
}

EBTNodeResult::Type UBTTask_IFMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	new (NodeMemory) float(0.f);

	AAIController* const AIController = OwnerComp.GetAIOwner();
	AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!AIController || !Enemy || !Target)
	{
		return EBTNodeResult::Failed;
	}

	const float Range = Enemy->GetCombatRange();
	if (IsWithinRange(Enemy, Target, Range))
	{
		AIController->StopMovement();
		return EBTNodeResult::Succeeded;
	}

	const EPathFollowingRequestResult::Type Result = AIController->MoveToActor(Target, Range, false);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}
	if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_IFMoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	float* const TimeSinceRepath = reinterpret_cast<float*>(NodeMemory);
	AAIController* const AIController = OwnerComp.GetAIOwner();
	AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!TimeSinceRepath || !AIController || !Enemy || !Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float Range = Enemy->GetCombatRange();
	if (IsWithinRange(Enemy, Target, Range))
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	*TimeSinceRepath += DeltaSeconds;
	if (*TimeSinceRepath >= RepathInterval)
	{
		*TimeSinceRepath = 0.f;
		AIController->MoveToActor(Target, Range, false);
	}
}
