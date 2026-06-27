#include "Components/IFPlayerCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/IFStaminaComponent.h"

void UIFPlayerCombatComponent::StartSpinAttack()
{
    if (bIsSpinning || GetCombatState() != ECombatState::Idle || !HasUsableStamina(MinimumStaminaToStartSpin))
    {
        return;
    }

    // Overlap-box collision is used here, same as combo attacks; tunneling on fast rotation is an accepted tradeoff.
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
        AbortSpinAndRestoreIdle();
        return;
    }

    // Configure the animation montage sections to loop the spin continuously.
    AnimInstance->Montage_SetNextSection(SpinIntroSectionName, SpinLoopSectionName, SpinAttackMontage);
    AnimInstance->Montage_SetNextSection(SpinLoopSectionName, SpinLoopSectionName, SpinAttackMontage);
    AnimInstance->Montage_JumpToSection(SpinIntroSectionName, SpinAttackMontage);

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UIFPlayerCombatComponent::OnSpinMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, SpinAttackMontage);
}

void UIFPlayerCombatComponent::StopSpinAttack()
{
    if (!bIsSpinning)
    {
        return;
    }

    RequestSpinEnd();
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
        StaminaComponent->OnStaminaDepleted.RemoveAll(this);
    }

    AbortSpin();
    Super::EndPlay(EndPlayReason);
}

void UIFPlayerCombatComponent::ResetCombatState()
{
    // Death and external resets own the final combat state, so spin must not force Idle.
    AbortSpin();
    Super::ResetCombatState();
}

void UIFPlayerCombatComponent::RequestSpinEnd()
{
    bIsSpinning = false;

    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }

    if (SpinLoopSectionName.IsNone() || SpinEndSectionName.IsNone())
    {
        AbortSpinAndRestoreIdle();
        return;
    }

    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (AnimInstance && SpinAttackMontage)
    {
        // Transition to the end section. This ensures the attack loops at least once even on a quick tap.
        AnimInstance->Montage_SetNextSection(SpinLoopSectionName, SpinEndSectionName, SpinAttackMontage);
    }
}

void UIFPlayerCombatComponent::AbortSpin()
{
    bIsSpinning = false;

    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }

    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (AnimInstance && SpinAttackMontage)
    {
        FOnMontageEnded EmptyDelegate;
        AnimInstance->Montage_SetEndDelegate(EmptyDelegate, SpinAttackMontage);

        if (AnimInstance->Montage_IsPlaying(SpinAttackMontage))
        {
            AnimInstance->Montage_Stop(SpinBlendOutTime, SpinAttackMontage);
        }
    }
}

void UIFPlayerCombatComponent::AbortSpinAndRestoreIdle()
{
    AbortSpin();
    RestoreIdleStateUnlessDead();
}

void UIFPlayerCombatComponent::OnSpinMontageEnded(UAnimMontage* Montage, bool /*bInterrupted*/)
{
    if (Montage != SpinAttackMontage)
    {
        return;
    }

    // Handle cases where the animation finishes or gets interrupted before StopSpinAttack is called.
    bIsSpinning = false;
    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }
    RestoreIdleStateUnlessDead();
}

void UIFPlayerCombatComponent::HandleStaminaDepleted()
{
    if (!bIsSpinning)
    {
        return;
    }

    RequestSpinEnd();
}

