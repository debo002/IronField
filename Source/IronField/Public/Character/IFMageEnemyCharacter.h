#pragma once

#include "CoreMinimal.h"
#include "Character/IFEnemyCharacter.h"
#include "IFMageEnemyCharacter.generated.h"

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

	void UpdateFocusOnTarget();
};
