#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeTypes.h"

class AActor;
class AIFEnemyCharacter;
class UBTDecorator;
class UBehaviorTreeComponent;

namespace IFAI
{
// Single source of truth for the enemy TargetActor blackboard key name.
// AIFEnemyController::TargetActorKeyName defaults to this; every BT node
// TargetActorKey selector must select the same key in the BT asset editor.
inline const FName TargetActorKey(TEXT("TargetActor"));
}

AActor* GetBlackboardTargetActor(const UBehaviorTreeComponent& OwnerComp, const FBlackboardKeySelector& TargetKey);

AIFEnemyCharacter* GetControlledEnemy(const UBehaviorTreeComponent& OwnerComp);

float GetActorCombatRadius(const AActor* Actor);

bool IsWithinRange(const AActor* A, const AActor* B, float Range);

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

void UpdateConditionDecoratorAbort(UBehaviorTreeComponent& OwnerComp, UBTDecorator* Decorator, uint8* NodeMemory, bool bCurrentResult);
