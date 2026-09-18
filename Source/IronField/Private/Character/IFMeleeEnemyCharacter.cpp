#include "Character/IFMeleeEnemyCharacter.h"

#include "Combat/IFMeleeCombatComponent.h"
#include "Stats/IFHealthComponent.h"

AIFMeleeEnemyCharacter::AIFMeleeEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIFMeleeCombatComponent>(TEXT("Combat")))
{
	// 3 player combo hits to kill (25+30=55, +40). BP can still override.
	if (UIFHealthComponent* const Health = GetHealthComponent())
	{
		Health->SetMaxHealth(70.f);
	}
}
