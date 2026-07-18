#include "Combat/IFMageCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Combat/IFProjectile.h"
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

	const FVector SpawnLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * ProjectileSpawnForwardOffset;
	const FRotator SpawnRotation = (Target->GetActorLocation() - SpawnLocation).Rotation();

	AIFProjectile* const Projectile = World->SpawnActor<AIFProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
	if (Projectile)
	{
		Projectile->InitializeProjectile(Owner, GetCurrentAttackDamage(), GetCurrentDamageTypeClass());
	}
}
