#include "AI/BTTask_IFAttack.h"

#include "AI/IFEnemyController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/IFCombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_IFAttack::UBTTask_IFAttack()
{
	NodeName = TEXT("Attack");
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFAttack, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_IFAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AIFEnemyController* const EnemyController = Cast<AIFEnemyController>(OwnerComp.GetAIOwner());
	const UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	if (!EnemyController || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AActor* const TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
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
	if (!Combat)
	{
		return EBTNodeResult::Failed;
	}

	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	ControlledPawn->SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));

	Combat->StartAttack();

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
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
