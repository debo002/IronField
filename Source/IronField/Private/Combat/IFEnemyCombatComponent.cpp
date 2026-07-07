#include "Combat/IFEnemyCombatComponent.h"

bool UIFEnemyCombatComponent::ShouldReactivelyBlock(bool bFacingAttacker) const
{
	return bFacingAttacker && ReactiveBlockChance > 0.f && FMath::FRand() < ReactiveBlockChance;
}
