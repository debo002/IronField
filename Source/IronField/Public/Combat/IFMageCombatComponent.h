#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFMageCombatComponent.generated.h"

class AIFProjectile;

/**
 * Lean ranged combat. Hides inherited melee categories (combo/block/weapon) and only implements
 * cast montage + projectile fire. No combo, block, or weapon-box path.
 */
UCLASS(ClassGroup = (Custom), HideCategories = ("Combat|Blocking", "Combat|Stamina", "Combat|Combo", "Combat|State"), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFMageCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	virtual void StartAttack() override;
	virtual void StartBlock() override {}
	virtual void StopBlock() override {}
	virtual void LaunchProjectileAttack() override;
	virtual void BeginAttackCollision() override {}
	virtual void EndAttackCollision() override {}

protected:
	virtual float GetCurrentAttackDamage() const override { return AttackDamage; }
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const override { return DamageTypeClass; }
	virtual bool CanQueueComboAttack() const override { return false; }
	virtual bool ShouldReactivelyBlock(bool bFacingAttacker) const override { return false; }
	virtual void PlayHitReactionMontage() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Mage|Cast", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Mage|Projectile", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AIFProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Mage|Projectile", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ProjectileSpawnForwardOffset = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Mage|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Mage|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> DamageTypeClass;
};
