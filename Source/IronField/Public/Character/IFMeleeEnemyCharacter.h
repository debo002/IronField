#pragma once

#include "CoreMinimal.h"
#include "Character/IFEnemyCharacter.h"
#include "IFMeleeEnemyCharacter.generated.h"

/** Close-range enemy. Installs UIFMeleeCombatComponent (weapon-box hits). */
UCLASS()
class IRONFIELD_API AIFMeleeEnemyCharacter : public AIFEnemyCharacter
{
	GENERATED_BODY()

public:
	AIFMeleeEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
