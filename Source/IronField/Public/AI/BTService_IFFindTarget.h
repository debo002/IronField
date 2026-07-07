#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_IFFindTarget.generated.h"

class AIFWaveManager;

UCLASS()
class IRONFIELD_API UBTService_IFFindTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_IFFindTarget();

protected:
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayerTargetSpawnChance = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SwitchToPlayerAfterAttackChance = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SwitchToPlayerAfterReviveChance = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "0.0"))
	float PlayerAttackRecencyWindowSeconds = 3.f;

private:
	void PickInitialTarget(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, AIFWaveManager& WaveManager) const;
	void ReEvaluateStrongholdTarget(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, AIFWaveManager& WaveManager) const;
	void SetBlackboardTarget(UBehaviorTreeComponent& OwnerComp, AActor* NewTarget) const;
};
