#include "AI/IFBTUtils.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTDecorator.h"
#include "Character/IFEnemyCharacter.h"
#include "GameFramework/Actor.h"

AActor* GetBlackboardTargetActor(const UBehaviorTreeComponent& OwnerComp, const FBlackboardKeySelector& TargetKey)
{
	const UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr;
}

AIFEnemyCharacter* GetControlledEnemy(const UBehaviorTreeComponent& OwnerComp)
{
	const AAIController* const AIController = OwnerComp.GetAIOwner();
	return AIController ? Cast<AIFEnemyCharacter>(AIController->GetPawn()) : nullptr;
}

float GetActorCombatRadius(const AActor* Actor)
{
	if (!Actor)
	{
		return 0.f;
	}

	const float SimpleRadius = Actor->GetSimpleCollisionRadius();
	if (SimpleRadius > KINDA_SMALL_NUMBER)
	{
		return SimpleRadius;
	}

	FVector Origin;
	FVector BoxExtent;
	Actor->GetActorBounds(false, Origin, BoxExtent);
	return BoxExtent.Size2D();
}

bool IsWithinRange(const AActor* A, const AActor* B, float Range)
{
	if (!A || !B)
	{
		return false;
	}
	const float CombinedRadius = GetActorCombatRadius(A) + GetActorCombatRadius(B);
	return FVector::DistSquared(A->GetActorLocation(), B->GetActorLocation()) <= FMath::Square(Range + CombinedRadius);
}

void UpdateConditionDecoratorAbort(UBehaviorTreeComponent& OwnerComp, UBTDecorator* Decorator, uint8* NodeMemory, bool bCurrentResult)
{
	FIFBTConditionMemory* const Memory = reinterpret_cast<FIFBTConditionMemory*>(NodeMemory);
	if (!Memory || !Decorator)
	{
		return;
	}

	const bool bChanged = !Memory->bInitialized || static_cast<bool>(Memory->bLastResult) != bCurrentResult;
	if (bChanged)
	{
		Memory->bInitialized = true;
		Memory->bLastResult = bCurrentResult;
		OwnerComp.RequestExecution(Decorator);
	}
}
