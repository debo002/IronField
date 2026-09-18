#include "Combat/IFMeleeCombatComponent.h"

float UIFMeleeCombatComponent::GetComboContinueChance(int32 ComboIndex) const
{
	return ComboContinueChances.IsValidIndex(ComboIndex) ? ComboContinueChances[ComboIndex] : 0.f;
}

float UIFMeleeCombatComponent::GetCurrentAttackDamage() const
{
	return Super::GetCurrentAttackDamage() * DamageMultiplier;
}
