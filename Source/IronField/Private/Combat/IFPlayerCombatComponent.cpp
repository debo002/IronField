#include "Combat/IFPlayerCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Core/IFAnimMontageUtils.h"
#include "Stats/IFStaminaComponent.h"

// ============================================================================
// Actions
// ============================================================================

void UIFPlayerCombatComponent::StartSpinAttack()
{
	if (bIsSpinning || !IsIdle() || !HasUsableStamina(MinimumStaminaToStartSpin))
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

	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !SpinAttackMontage || AnimInstance->Montage_Play(SpinAttackMontage) <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IF-Combat] %s could not start spin attack - missing AnimInstance or SpinAttackMontage."), *GetNameSafe(GetOwner()));
		StopSpinImmediately();
		RestoreIdleStateUnlessDead();
		return;
	}

	// Intro plays once into Loop; Loop repeats until StopSpinAttack redirects it to End.
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

// ============================================================================
// Lifecycle
// ============================================================================

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
	Super::EndPlay(EndPlayReason);

	if (StaminaComponent)
	{
		StaminaComponent->OnStaminaDepleted.RemoveAll(this);
	}

	StopSpinImmediately();
}

void UIFPlayerCombatComponent::ResetCombatState()
{
	StopSpinImmediately();
	Super::ResetCombatState();
}

// ============================================================================
// Queries (overrides)
// ============================================================================

float UIFPlayerCombatComponent::GetCurrentAttackDamage() const
{
	return bIsSpinning ? SpinDamage : Super::GetCurrentAttackDamage();
}

TSubclassOf<UDamageType> UIFPlayerCombatComponent::GetCurrentDamageTypeClass() const
{
	return bIsSpinning ? SpinDamageTypeClass : Super::GetCurrentDamageTypeClass();
}

// ============================================================================
// Internal helpers
// ============================================================================

void UIFPlayerCombatComponent::StopSpinGracefully()
{
	bIsSpinning = false;

	if (StaminaComponent)
	{
		StaminaComponent->StopContinuousDrain();
	}

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
	bIsSpinning = false;

	if (StaminaComponent)
	{
		StaminaComponent->StopContinuousDrain();
	}

	UAnimInstance* const AnimInstance = GetAnimInstance();
	ClearMontageEndDelegate(AnimInstance, SpinAttackMontage);

	if (AnimInstance && SpinAttackMontage && AnimInstance->Montage_IsPlaying(SpinAttackMontage))
	{
		AnimInstance->Montage_Stop(SpinBlendOutTime, SpinAttackMontage);
	}
}

void UIFPlayerCombatComponent::HandleSpinMontageEnded(UAnimMontage* Montage, bool )
{
	if (Montage != SpinAttackMontage)
	{
		return;
	}

	// Montage already finished naturally; this just clears remaining state.
	StopSpinImmediately();
	RestoreIdleStateUnlessDead();
}

void UIFPlayerCombatComponent::HandleStaminaDepleted()
{
	if (!bIsSpinning)
	{
		return;
	}

	StopSpinGracefully();
}