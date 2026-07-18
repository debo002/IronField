#include "Combat/IFPlayerCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Core/IFAnimMontageUtils.h"
#include "Core/IFLog.h"
#include "Stats/IFStaminaComponent.h"

void UIFPlayerCombatComponent::StartAttack()
{
	if (IsDead() || IsBlocking() || bIsSpinning)
	{
		return;
	}

	if (!IsAttacking())
	{
		TryPlayAttackMontage(0);
		return;
	}

	// One-slot input buffer: press during any active step (except the last) to queue the next.
	if (CanQueueComboAttack())
	{
		bComboQueued = true;
	}
}

bool UIFPlayerCombatComponent::CanQueueComboAttack() const
{
	return !bIsSpinning && Super::CanQueueComboAttack();
}

void UIFPlayerCombatComponent::StartSpinAttack()
{
	if (bIsSpinning || !IsIdle() || !HasUsableStamina(MinimumStaminaToStartSpin))
	{
		return;
	}

	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !SpinAttackMontage)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Combat] %s StartSpinAttack with no SpinAttackMontage assigned."),
			*GetNameSafe(GetOwner()));
		return;
	}

	const float PlayLength = AnimInstance->Montage_Play(SpinAttackMontage);
	if (PlayLength <= 0.f)
	{
		return;
	}

	bIsSpinning = true;
	ResetRegisteredAttackHits();
	SetCombatState(ECombatState::Attacking);

	if (StaminaComponent)
	{
		StaminaComponent->StartContinuousDrain(SpinStaminaDrainRate);
	}

	AnimInstance->Montage_SetNextSection(SpinIntroSectionName, SpinLoopSectionName, SpinAttackMontage);
	AnimInstance->Montage_SetNextSection(SpinLoopSectionName, SpinLoopSectionName, SpinAttackMontage);
	AnimInstance->Montage_JumpToSection(SpinIntroSectionName, SpinAttackMontage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UIFPlayerCombatComponent::HandleSpinMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, SpinAttackMontage);
}

void UIFPlayerCombatComponent::StopSpinAttack()
{
	if (!bIsSpinning)
	{
		return;
	}

	StopSpinGracefully();
}

void UIFPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaDepleted.AddDynamic(this, &UIFPlayerCombatComponent::HandleStaminaDepleted);
	}
}

void UIFPlayerCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaDepleted.RemoveDynamic(this, &UIFPlayerCombatComponent::HandleStaminaDepleted);
	}

	StopSpinImmediately();
	Super::EndPlay(EndPlayReason);
}

void UIFPlayerCombatComponent::ResetCombatState()
{
	StopSpinImmediately();
	Super::ResetCombatState();
}

float UIFPlayerCombatComponent::GetCurrentAttackDamage() const
{
	return bIsSpinning ? SpinDamage : Super::GetCurrentAttackDamage();
}

TSubclassOf<UDamageType> UIFPlayerCombatComponent::GetCurrentDamageTypeClass() const
{
	return bIsSpinning ? SpinDamageTypeClass : Super::GetCurrentDamageTypeClass();
}

void UIFPlayerCombatComponent::ClearSpinState()
{
	bIsSpinning = false;

	if (StaminaComponent)
	{
		StaminaComponent->StopContinuousDrain();
	}
}

void UIFPlayerCombatComponent::StopSpinGracefully()
{
	ClearSpinState();

	if (SpinLoopSectionName.IsNone() || SpinEndSectionName.IsNone())
	{
		StopSpinImmediately();
		RestoreIdleStateUnlessDead();
		return;
	}

	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (AnimInstance && SpinAttackMontage)
	{
		AnimInstance->Montage_SetNextSection(SpinLoopSectionName, SpinEndSectionName, SpinAttackMontage);
	}
}

void UIFPlayerCombatComponent::StopSpinImmediately()
{
	ClearSpinState();

	UAnimInstance* const AnimInstance = GetAnimInstance();
	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, SpinAttackMontage);

	if (AnimInstance && SpinAttackMontage && AnimInstance->Montage_IsPlaying(SpinAttackMontage))
	{
		AnimInstance->Montage_Stop(SpinBlendOutTime, SpinAttackMontage);
	}
}

void UIFPlayerCombatComponent::HandleSpinMontageEnded(UAnimMontage* Montage, bool)
{
	if (Montage != SpinAttackMontage)
	{
		return;
	}

	StopSpinImmediately();
	RestoreIdleStateUnlessDead();
}

void UIFPlayerCombatComponent::HandleStaminaDepleted()
{
	if (bIsSpinning)
	{
		StopSpinGracefully();
	}
}
