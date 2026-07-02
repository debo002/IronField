#include "Combat/CombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Character/BaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Core/AnimMontageUtils.h"
#include "Stats/HealthComponent.h"
#include "Stats/StaminaComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

UIFCombatComponent::UIFCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UIFCombatComponent::StartAttack()
{
    if (IsDead() || IsBlocking())
    {
        return;
    }

    if (!IsAttacking())
    {
        TryPlayAttackMontage(0);
        return;
    }

    if (CanQueueComboAttack())
    {
        bComboQueued = true;
    }
}

void UIFCombatComponent::StartBlock()
{
    if (!IsIdle() || !HasUsableStamina(MinimumStaminaToStartBlock))
    {
        return;
    }

    SetCombatState(ECombatState::Blocking);

    if (!TryPlayBlockMontage())
    {
        SetCombatState(ECombatState::Idle);
        return;
    }

    if (StaminaComponent)
    {
        StaminaComponent->StartContinuousDrain(BlockStaminaDrainRate);
    }
}

void UIFCombatComponent::StopBlock()
{
    if (!IsBlocking())
    {
        return;
    }

    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (AnimInstance && BlockMontage)
    {
        AnimInstance->Montage_Stop(0.15f, BlockMontage);
    }

    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }

    SetCombatState(ECombatState::Idle);
}

void UIFCombatComponent::ResetCombatState()
{
    bComboQueued = false;
    CurrentComboIndex = 0;
    ResetRegisteredAttackHits();
    ClearAttackMontageDelegate();
    ActiveAttackMontage = nullptr;
    EndAttackCollision();

    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }

    RestoreIdleStateUnlessDead();
}

void UIFCombatComponent::HandleOwnerDeath()
{
    if (IsDead())
    {
        return;
    }

    SetCombatState(ECombatState::Dead);
    ResetCombatState();
}

void UIFCombatComponent::HandleOwnerRevived()
{
    SetCombatState(ECombatState::Idle);
}

void UIFCombatComponent::SetCombatState(ECombatState NewState)
{
    if (CombatState == NewState)
    {
        return;
    }

    const ECombatState PreviousState = CombatState;
    CombatState = NewState;
    OnCombatStateChanged.Broadcast(PreviousState, CombatState);
}

bool UIFCombatComponent::TryRegisterAttackHit(AActor* TargetActor)
{
    if (!TargetActor || !IsAttacking())
    {
        return false;
    }

    // Prevent hitting the same target multiple times during a single attack swing.
    if (RegisteredAttackHits.Contains(TargetActor))
    {
        return false;
    }

    RegisteredAttackHits.Add(TargetActor);
    return true;
}

void UIFCombatComponent::BeginAttackCollision(float Damage, TSubclassOf<UDamageType> DamageTypeClass)
{
    if (!IsAttacking() || Damage <= 0.f)
    {
        return;
    }

    ActiveAttackDamage = Damage;
    ActiveDamageTypeClass = DamageTypeClass;
    bAttackCollisionActive = true;
    ResetRegisteredAttackHits();
    SetWeaponCollisionEnabled(true);
}

void UIFCombatComponent::EndAttackCollision()
{
    bAttackCollisionActive = false;
    ActiveAttackDamage = 0.f;
    ActiveDamageTypeClass = nullptr;
    SetWeaponCollisionEnabled(false);
}

void UIFCombatComponent::HandleWeaponBoxBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
    ResolveAttackHit(OtherActor);
}

void UIFCombatComponent::ResolveAttackHit(AActor* TargetActor)
{
    UIFHealthComponent* const TargetHealth = GetValidAttackTargetHealth(TargetActor);
    if (!TargetHealth || !TryRegisterAttackHit(TargetActor))
    {
        return;
    }

    if (UIFCombatComponent* const TargetCombat = TargetActor->FindComponentByClass<UIFCombatComponent>())
    {
        // The target owns how it reacts to being hit — block check, damage, and reaction
        // montage all happen inside its own component, not here.
        TargetCombat->ReceiveMeleeAttack(GetOwner(), ActiveAttackDamage, ActiveDamageTypeClass);
        return;
    }

    // Targets without a CombatComponent (e.g. Stronghold) can't block or play a reaction —
    // just apply damage directly.
    ApplyDamageTo(TargetActor, GetOwner(), ActiveAttackDamage, ActiveDamageTypeClass);
}

void UIFCombatComponent::ReceiveMeleeAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass)
{
    if (IsBlocking() && IsOwnerFacingTarget(Instigator))
    {
        PlayBlockReactionMontage();
        return;
    }

    AActor* const Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    ApplyDamageTo(Owner, Instigator, Damage, DamageTypeClass);

    // Re-check post-damage: a killing blow should not also trigger a hit-reaction montage.
    const UIFHealthComponent* const OwnHealth = Owner->FindComponentByClass<UIFHealthComponent>();
    if (!OwnHealth || !OwnHealth->IsDead())
    {
        PlayHitReactionMontage();
    }
}

void UIFCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* const Owner = GetOwner();

    const ACharacter* const OwnerCharacter = Cast<ACharacter>(Owner);
    CachedMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;

    StaminaComponent = Owner ? Owner->FindComponentByClass<UIFStaminaComponent>() : nullptr;

    // The weapon collision volume is a physical component owned by the character; cache it here
    // so this component can fully own its lifecycle (enable/disable, overlap handling).
    const AIFBaseCharacter* const BaseCharacterOwner = Cast<AIFBaseCharacter>(Owner);
    WeaponCollisionBox = BaseCharacterOwner ? BaseCharacterOwner->GetWeaponCollisionBox() : nullptr;

    if (WeaponCollisionBox)
    {
        WeaponCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &UIFCombatComponent::HandleWeaponBoxBeginOverlap);
    }
}

void UIFCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (WeaponCollisionBox)
    {
        WeaponCollisionBox->OnComponentBeginOverlap.RemoveAll(this);
    }

    EndAttackCollision();
    ClearAttackMontageDelegate();

    if (StaminaComponent)
    {
        StaminaComponent->StopContinuousDrain();
    }
}

bool UIFCombatComponent::CanPlayAttackMontage(int32 ComboIndex) const
{
    if (!ComboSteps.IsValidIndex(ComboIndex))
    {
        return false;
    }

    if (!ComboSteps[ComboIndex].AttackMontage)
    {
        return false;
    }

    return GetAnimInstance() && HasUsableStamina(GetComboStaminaCost(ComboIndex));
}

bool UIFCombatComponent::TryPlayBlockMontage()
{
    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (!AnimInstance || !BlockMontage)
    {
        return true;
    }

    return AnimInstance->Montage_Play(BlockMontage) > 0.f;
}

bool UIFCombatComponent::TryPlayAttackMontage(int32 ComboIndex)
{
    if (!CanPlayAttackMontage(ComboIndex))
    {
        return false;
    }

    UAnimInstance* const AnimInstance = GetAnimInstance();
    const float StaminaCost = GetComboStaminaCost(ComboIndex);
    ClearAttackMontageDelegate();

    UAnimMontage* const AttackMontage = ComboSteps[ComboIndex].AttackMontage;

    // StaminaComponent is guaranteed non-null here: CanPlayAttackMontage already verified it via HasUsableStamina.
    if (!StaminaComponent->TryConsumeStamina(StaminaCost))
    {
        return false;
    }

    ActiveAttackMontage = AttackMontage;

    const float PlayLength = AnimInstance->Montage_Play(AttackMontage);
    if (PlayLength <= 0.f)
    {
        ActiveAttackMontage = nullptr;
        return false;
    }

    CurrentComboIndex = ComboIndex;
    bComboQueued = false;
    ResetRegisteredAttackHits();
    SetCombatState(ECombatState::Attacking);

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UIFCombatComponent::OnAttackMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);

    return true;
}

bool UIFCombatComponent::HasUsableStamina(float Amount) const
{
    return StaminaComponent && StaminaComponent->HasStamina(Amount);
}

void UIFCombatComponent::ResetRegisteredAttackHits()
{
    RegisteredAttackHits.Reset();
}

UIFHealthComponent* UIFCombatComponent::GetValidAttackTargetHealth(AActor* TargetActor) const
{
    // bAttackCollisionActive narrows Attacking down to the notify's active hit window within the swing.
    if (!bAttackCollisionActive || !IsAttacking())
    {
        return nullptr;
    }

    const AActor* const Owner = GetOwner();
    if (!TargetActor || TargetActor == Owner)
    {
        return nullptr;
    }

    UIFHealthComponent* const TargetHealth = TargetActor->FindComponentByClass<UIFHealthComponent>();
    return (TargetHealth && !TargetHealth->IsDead()) ? TargetHealth : nullptr;
}

bool UIFCombatComponent::IsOwnerFacingTarget(AActor* TargetActor) const
{
    const AActor* const Owner = GetOwner();
    if (!Owner || !TargetActor)
    {
        return false;
    }

    const FVector DirectionToTarget = (TargetActor->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
    if (DirectionToTarget.IsNearlyZero())
    {
        return true;
    }

    const FVector OwnerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
    return FVector::DotProduct(OwnerForward, DirectionToTarget) >= BlockFacingDotThreshold;
}

void UIFCombatComponent::ApplyDamageTo(AActor* TargetActor, AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass) const
{
    if (!TargetActor || !Instigator)
    {
        return;
    }

    const APawn* const InstigatorPawn = Cast<APawn>(Instigator);
    AController* const InstigatorController = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;

    FDamageEvent DamageEvent(DamageTypeClass);
    TargetActor->TakeDamage(Damage, DamageEvent, InstigatorController, Instigator);
}

void UIFCombatComponent::PlayHitReactionMontage()
{
    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (AnimInstance && HitReactionMontage)
    {
        AnimInstance->Montage_Play(HitReactionMontage);
    }
}

void UIFCombatComponent::PlayBlockReactionMontage()
{
    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (AnimInstance && BlockReactionMontage)
    {
        AnimInstance->Montage_Play(BlockReactionMontage);
    }
}

void UIFCombatComponent::SetWeaponCollisionEnabled(bool bEnabled) const
{
    if (!WeaponCollisionBox)
    {
        return;
    }

    WeaponCollisionBox->SetGenerateOverlapEvents(bEnabled);
    WeaponCollisionBox->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

float UIFCombatComponent::GetComboStaminaCost(int32 ComboIndex) const
{
    return ComboSteps.IsValidIndex(ComboIndex) ? ComboSteps[ComboIndex].StaminaCost : 0.f;
}

UAnimInstance* UIFCombatComponent::GetAnimInstance() const
{
    if (!CachedMesh)
    {
        return nullptr;
    }

    return CachedMesh->GetAnimInstance();
}

void UIFCombatComponent::ClearAttackMontageDelegate()
{
    IF::AnimMontageUtils::ClearMontageEndDelegate(GetAnimInstance(), ActiveAttackMontage);
}

void UIFCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != ActiveAttackMontage)
    {
        return;
    }

    if (bInterrupted)
    {
        ResetCombatState();
        return;
    }

    if (bComboQueued)
    {
        const int32 NextComboIndex = CurrentComboIndex + 1;
        if (TryPlayAttackMontage(NextComboIndex))
        {
            return;
        }
    }

    ResetCombatState();
}

void UIFCombatComponent::RestoreIdleStateUnlessDead()
{
    if (!IsDead())
    {
        SetCombatState(ECombatState::Idle);
    }
}
