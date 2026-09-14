#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_IFMoveAwayFromTarget.generated.h"

UCLASS()
class IRONFIELD_API UBTTask_IFMoveAwayFromTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IFMoveAwayFromTarget();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "IronField|AI|Targeting")
	FBlackboardKeySelector TargetActorKey;

	// Finish slightly inside the ideal range so we don't thrash at the boundary.
	UPROPERTY(EditAnywhere, Category = "IronField|AI|Movement", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SuccessRangeTolerance = 0.95f;

	/** Seconds between repath requests while retreating. */
	UPROPERTY(EditAnywhere, Category = "IronField|AI|Movement", meta = (ClampMin = "0.05"))
	float RepathInterval = 0.25f;

	/** How far the projected retreat destination must move before a new move request is issued. */
	UPROPERTY(EditAnywhere, Category = "IronField|AI|Movement", meta = (ClampMin = "0.0"))
	float DestinationDriftThreshold = 120.f;

	UPROPERTY(EditAnywhere, Category = "IronField|AI|Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 50.f;
};
