#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_IFFindTarget.generated.h"

UCLASS()
class IRONFIELD_API UBTService_IFFindTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_IFFindTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;

private:
	void ChooseTarget(UBehaviorTreeComponent& OwnerComp) const;
};
