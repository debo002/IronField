#include "Character/IFMeleeEnemyCharacter.h"

#include "Combat/IFMeleeCombatComponent.h"

AIFMeleeEnemyCharacter::AIFMeleeEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIFMeleeCombatComponent>(TEXT("Combat")))
{
}
