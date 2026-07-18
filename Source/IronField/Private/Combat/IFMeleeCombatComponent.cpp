#include "Combat/IFMeleeCombatComponent.h"

float UIFMeleeCombatComponent::GetComboContinueChance(int32 ComboIndex) const
{
	return ComboContinueChances.IsValidIndex(ComboIndex) ? ComboContinueChances[ComboIndex] : 0.f;
}

bool UIFMeleeCombatComponent::ShouldReactivelyBlock(bool bFacingAttacker) const
{
	return bFacingAttacker && ReactiveBlockChance > 0.f && FMath::FRand() < ReactiveBlockChance;
}
