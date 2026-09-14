#include "Combat/IFMageCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Combat/IFProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Core/IFAnimMontageUtils.h"

void UIFMageCombatComponent::PlayHitReactionMontage()
{
	if (IsAttacking())
	{
		return;
	}

	Super::PlayHitReactionMontage();
}

void UIFMageCombatComponent::StartAttack()
{
	if (IsDead() || IsAttacking() || !CastMontage)
	{
		return;
	}

	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	ClearAttackMontageDelegate();

	const float PlayLength = AnimInstance->Montage_Play(CastMontage);
	if (PlayLength <= 0.f)
	{
		return;
	}

	ActiveAttackMontage = CastMontage;
	bComboQueued = false;
	CurrentComboIndex = 0;
	SetCombatState(ECombatState::Attacking);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UIFMageCombatComponent::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);
}

void UIFMageCombatComponent::LaunchProjectileAttack()
{
	if (!ProjectileClass)
	{
		return;
	}

	AActor* const Target = GetAttackTarget();
	AActor* const Owner = GetOwner();
	UWorld* const World = GetWorld();
	if (!Target || !Owner || !World)
	{
		return;
	}

	// Spawn clear of the shooter's collision. A shot materialized inside the owner
	// reports its initial overlap during SpawnActor, before InitializeProjectile runs.
	float ClearanceRadius = ProjectileSpawnForwardOffset;
	if (const UCapsuleComponent* const OwnerCapsule = Owner->FindComponentByClass<UCapsuleComponent>())
	{
		const AIFProjectile* const ProjectileCDO = ProjectileClass->GetDefaultObject<AIFProjectile>();
		const float ProjectileRadius = ProjectileCDO ? ProjectileCDO->GetCollisionSphereRadius() : 0.f;
		ClearanceRadius = OwnerCapsule->GetScaledCapsuleRadius() + ProjectileRadius + 20.f;
	}

	const FVector Forward2D = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector SpawnLocation = Owner->GetActorLocation() + Forward2D * ClearanceRadius;
	const FRotator SpawnRotation = (Target->GetActorLocation() - SpawnLocation).Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AIFProjectile* const Projectile = World->SpawnActor<AIFProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile)
	{
		Projectile->InitializeProjectile(Owner, GetCurrentAttackDamage(), GetCurrentDamageTypeClass());
	}
}
