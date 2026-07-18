#pragma once

#include "CoreMinimal.h"

class AActor;
class UDamageType;
class UIFCombatComponent;
class UIFHealthComponent;

namespace IFCombatTargetingUtils
{
	/** Returns the target's health component if InstigatorActor is legally allowed to damage TargetActor right now, otherwise nullptr. */
	UIFHealthComponent* GetValidAttackTargetHealth(const AActor* InstigatorActor, AActor* TargetActor);

	/** Applies damage through the engine damage pipeline so health components and TakeAnyDamage listeners both see it. */
	void ApplyDamageTo(AActor* TargetActor, AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass);

	/**
	 * Routes a hit: combat-capable targets go through ReceiveAttack (block/hit-reaction),
	 * everything else takes raw damage (e.g. stronghold).
	 */
	void DeliverDamage(AActor* TargetActor, AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass);

	/** Null-safe FindComponentByClass for combat; used by anim notifies and hit resolution. */
	UIFCombatComponent* FindCombatComponent(const AActor* Actor);
}
