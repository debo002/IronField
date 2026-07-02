#include "Combat/PlayerCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Core/AnimMontageUtils.h"
#include "Stats/StaminaComponent.h"

void UIFPlayerCombatComponent::StartSpinAttack()
{
    if (bIsSpinning || !IsIdle() || !HasUsableStamina(MinimumStaminaToStartSpin))
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

    // Loop the Loop section into itself; StopSpinAttack later redirects it into the End section.
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
    HaltSpinning();

    if (SpinLoopSectionName.IsNone() || SpinEndSectionName.IsNone())
    {
        AbortSpinAndRestoreIdle();
        return;
    }

    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (AnimInstance && SpinAttackMontage)
    {
        // Redirect out through the End section so a quick tap still completes at least one loop.
        AnimInstance->Montage_SetNextSection(SpinLoopSectionName, SpinEndSectionName, SpinAttackMontage);
    }
}

void UIFPlayerCombatComponent::HaltSpinning()
{
    bIsSpinning = false;

    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }
}

void UIFPlayerCombatComponent::AbortSpin()
{
    HaltSpinning();

    UAnimInstance* const AnimInstance = GetAnimInstance();
    IF::AnimMontageUtils::ClearMontageEndDelegate(AnimInstance, SpinAttackMontage);

    if (AnimInstance && SpinAttackMontage && AnimInstance->Montage_IsPlaying(SpinAttackMontage))
    {
        AnimInstance->Montage_Stop(SpinBlendOutTime, SpinAttackMontage);
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

    // Reaching here means the montage finished or was interrupted without StopSpinAttack being called first.
    HaltSpinning();
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