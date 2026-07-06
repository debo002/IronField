#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IFInAttackRange.generated.h"

UCLASS()
class IRONFIELD_API UBTDecorator_IFInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IFInAttackRange();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;

	// Vector key holding the reserved attack point location. Must match the key used by
	// BTTask_IFMoveToTarget's MoveDestinationKey, since that's the position the enemy
	// actually walks to when targeting the Stronghold.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector MoveDestinationKey;

	// Range used when the target is the Player.
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0"))
	float PlayerAttackRange = 160.f;

	// Range used when the target is the Stronghold. Must be >= BTTask_IFMoveToTarget's
	// AcceptanceRadius, or the enemy can "arrive" and still fail this check.
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0"))
	float StrongholdAttackRange = 160.f;
};