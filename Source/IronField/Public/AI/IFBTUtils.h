#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTDecorator.h"
#include "Character/IFEnemyCharacter.h"
#include "GameFramework/Actor.h"

/** Default blackboard key name used by enemy BTs and AIFEnemyController. Keep in sync with BB_Enemy. */
inline FName GetDefaultTargetActorKeyName()
{
	return FName(TEXT("TargetActor"));
}

inline AActor* GetBlackboardTargetActor(const UBehaviorTreeComponent& OwnerComp, const FBlackboardKeySelector& TargetKey)
{
	const UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	return Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr;
}

inline AIFEnemyCharacter* GetControlledEnemy(const UBehaviorTreeComponent& OwnerComp)
{
	const AAIController* const AIController = OwnerComp.GetAIOwner();
	return AIController ? Cast<AIFEnemyCharacter>(AIController->GetPawn()) : nullptr;
}

inline bool IsWithinRange(const AActor* A, const AActor* B, float Range)
{
	if (!A || !B)
	{
		return false;
	}
	const float CombinedRadius = A->GetSimpleCollisionRadius() + B->GetSimpleCollisionRadius();
	return FVector::DistSquared(A->GetActorLocation(), B->GetActorLocation()) <= FMath::Square(Range + CombinedRadius);
}

/** Instance memory for decorators that re-evaluate continuous world state and abort on change. */
struct FIFBTConditionMemory
{
	uint8 bLastResult : 1;
	uint8 bInitialized : 1;

	FIFBTConditionMemory()
		: bLastResult(false)
		, bInitialized(false)
	{
	}
};

inline void UpdateConditionDecoratorAbort(UBehaviorTreeComponent& OwnerComp, UBTDecorator* Decorator, uint8* NodeMemory, bool bCurrentResult)
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
