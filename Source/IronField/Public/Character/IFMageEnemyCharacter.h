#pragma once

#include "CoreMinimal.h"
#include "Character/IFEnemyCharacter.h"
#include "IFMageEnemyCharacter.generated.h"

class AIFEnemyController;
class UBlackboardComponent;

UCLASS()
class IRONFIELD_API AIFMageEnemyCharacter : public AIFEnemyCharacter
{
	GENERATED_BODY()

public:
	AIFMageEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void OnDeathStarted() override;

private:
	// Avoids redundant SetFocus/ClearFocus calls every tick while the target is unchanged.
	TWeakObjectPtr<AActor> CurrentFocusTarget;

	// Cached to skip the per-tick controller cast and blackboard/key lookups. Resolved lazily so
	// late AI possession still works; the blackboard target value itself is read every tick.
	TWeakObjectPtr<AIFEnemyController> CachedEnemyController;
	TWeakObjectPtr<UBlackboardComponent> CachedBlackboard;
	FName CachedTargetKey;

	void UpdateFocusOnTarget();
};
