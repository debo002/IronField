#include "AI/BTTask_IFMoveToTarget.h"

#include "AIController.h"
#include "AI/IFBTUtils.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	struct FIFMoveToMemory
	{
		float TimeSinceRepath = 0.f;
	};

	constexpr float RepathInterval = 0.25f;
}

UBTTask_IFMoveToTarget::UBTTask_IFMoveToTarget()
{
	NodeName = TEXT("Move To Target");
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFMoveToTarget, TargetActorKey), AActor::StaticClass());
}

uint16 UBTTask_IFMoveToTarget::GetInstanceMemorySize() const
{
	return sizeof(FIFMoveToMemory);
}

EBTNodeResult::Type UBTTask_IFMoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	new (NodeMemory) FIFMoveToMemory();

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

	FIFMoveToMemory* const Memory = reinterpret_cast<FIFMoveToMemory*>(NodeMemory);
	AAIController* const AIController = OwnerComp.GetAIOwner();
	AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!Memory || !AIController || !Enemy || !Target)
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

	Memory->TimeSinceRepath += DeltaSeconds;
	if (Memory->TimeSinceRepath >= RepathInterval)
	{
		Memory->TimeSinceRepath = 0.f;
		AIController->MoveToActor(Target, Range, false);
	}
}
