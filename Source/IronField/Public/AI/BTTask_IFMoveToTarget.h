#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_IFMoveToTarget.generated.h"

UCLASS()
class IRONFIELD_API UBTTask_IFMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IFMoveToTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Movement")
	FBlackboardKeySelector TargetActorKey;

	// Vector key holding the reserved attack point location, set by BTService_IFFindTarget
	// whenever the target resolves to the Stronghold. Ignored for the player target.
	UPROPERTY(EditAnywhere, Category = "Movement")
	FBlackboardKeySelector MoveDestinationKey;

	// Single flat acceptance radius used for both the player and Stronghold attack-point moves.
	// Attack points should be placed at a distance from the Stronghold that's within
	// UBTDecorator_IFInAttackRange's AttackRange, or the enemy will arrive but never be
	// considered "in range" to attack.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 130.f;
};
