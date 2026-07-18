#include "AI/BTTask_IFAttack.h"

#include "AI/IFBTUtils.h"
#include "AI/IFEnemyController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Combat/IFCombatComponent.h"

UBTTask_IFAttack::UBTTask_IFAttack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFAttack, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_IFAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AIFEnemyController* const EnemyController = Cast<AIFEnemyController>(OwnerComp.GetAIOwner());
	if (!EnemyController)
	{
		return EBTNodeResult::Failed;
	}

	AActor* const TargetActor = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	APawn* const ControlledPawn = EnemyController->GetPawn();
	if (!TargetActor || !ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	if (!EnemyController->IsReadyForNewAttack())
	{
		return EBTNodeResult::Failed;
	}

	UIFCombatComponent* const Combat = EnemyController->GetControlledCombatComponent();
	if (!Combat || !Combat->IsIdle())
	{
		return EBTNodeResult::Failed;
	}

	EnemyController->StopMovement();

	const FRotator LookAtRotation = (TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation()).Rotation();
	ControlledPawn->SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));

	Combat->SetAttackTarget(TargetActor);
	Combat->StartAttack();

	if (!Combat->IsAttacking())
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_IFAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AIFEnemyController* const EnemyController = Cast<AIFEnemyController>(OwnerComp.GetAIOwner());
	if (!EnemyController)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const UIFCombatComponent* const Combat = EnemyController->GetControlledCombatComponent();
	if (!Combat || !Combat->IsAttacking())
	{
		FinishLatentTask(OwnerComp, Combat ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
}
