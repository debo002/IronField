#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFMageCombatComponent.generated.h"

class AIFProjectile;

/**
 * Ranged combat for the mage: cast montage + projectile fire.
 * The inherited melee categories (combo/weapon) are hidden because they do not apply.
 */
UCLASS(HideCategories = ("IronField|Combat|Stamina", "IronField|Combat|Combo", "IronField|Combat|State", "IronField|Combat|Weapon"))
class IRONFIELD_API UIFMageCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	virtual void StartAttack() override;
	virtual void LaunchProjectileAttack() override;
	virtual void BeginAttackCollision() override {}
	virtual void EndAttackCollision() override {}

protected:
	virtual float GetCurrentAttackDamage() const override { return AttackDamage; }
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const override { return DamageTypeClass; }
	virtual bool CanQueueComboAttack() const override { return false; }
	virtual void PlayHitReactionMontage() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Cast", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Projectile", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AIFProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Projectile", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ProjectileSpawnForwardOffset = 60.f;

	// Tuned down while the mage deals damage for the first time; re-tune after the re-test.
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackDamage = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Enemy|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> DamageTypeClass;
};
