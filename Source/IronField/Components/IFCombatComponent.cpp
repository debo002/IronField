#include "Components/IFCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Characters/IFBaseCharacter.h"
#include "Components/IFHealthComponent.h"
#include "Components/IFStaminaComponent.h"
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

    if (CombatState != ECombatState::Attacking)
    {
        TryPlayAttackMontage(0);
        return;
    }

    if (ActiveAttackMontage != nullptr)
    {
        bComboQueued = true;
    }
}

void UIFCombatComponent::StartBlock()
{
    if (CombatState != ECombatState::Idle || !HasUsableStamina(MinimumStaminaToStartAction))
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
    if (CombatState != ECombatState::Blocking)
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
    if (!TargetActor || CombatState != ECombatState::Attacking)
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
    if (CombatState != ECombatState::Attacking || Damage <= 0.f)
    {
        return;
    }

    ActiveAttackDamage = Damage;
    ActiveDamageTypeClass = DamageTypeClass;
    bAttackCollisionActive = true;
    ResetRegisteredAttackHits();
    SetOwnerWeaponCollisionEnabled(true);
}

void UIFCombatComponent::EndAttackCollision()
{
    bAttackCollisionActive = false;
    ActiveAttackDamage = 0.f;
    ActiveDamageTypeClass = nullptr;
    SetOwnerWeaponCollisionEnabled(false);
}

void UIFCombatComponent::HandleWeaponCollisionOverlap(AActor* TargetActor)
{
    if (!IsValidAttackOverlap(TargetActor) || !TryRegisterAttackHit(TargetActor))
    {
        return;
    }

    UIFCombatComponent* const TargetCombat = TargetActor->FindComponentByClass<UIFCombatComponent>();
    if (TargetCombat && TargetCombat->IsBlocking() && TargetCombat->IsOwnerFacingTarget(GetOwner()))
    {
        TargetCombat->PlayBlockReactionMontage();
        return;
    }

    ApplyAttackDamage(TargetActor);

    const UIFHealthComponent* const TargetHealth = TargetActor->FindComponentByClass<UIFHealthComponent>();
    if (TargetCombat && !TargetHealth->IsDead())
    {
        TargetCombat->PlayHitReactionMontage();
    }
}

void UIFCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    ACharacter* const OwnerCharacter = Cast<ACharacter>(GetOwner());
    CachedMesh = nullptr;
    if (OwnerCharacter)
    {
        CachedMesh = OwnerCharacter->GetMesh();
    }

    AActor* const Owner = GetOwner();
    StaminaComponent = nullptr;
    if (Owner)
    {
        StaminaComponent = Owner->FindComponentByClass<UIFStaminaComponent>();
    }
}

void UIFCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

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

    // Consume stamina before playing the montage so the animation never starts if the cost can't be paid.
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

bool UIFCombatComponent::IsValidAttackOverlap(AActor* TargetActor) const
{
    if (!bAttackCollisionActive || ActiveAttackDamage <= 0.f || CombatState != ECombatState::Attacking)
    {
        return false;
    }

    AActor* const Owner = GetOwner();
    if (!TargetActor || TargetActor == Owner)
    {
        return false;
    }

    const UIFHealthComponent* const TargetHealth = TargetActor->FindComponentByClass<UIFHealthComponent>();
    return TargetHealth && !TargetHealth->IsDead();
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

void UIFCombatComponent::ApplyAttackDamage(AActor* TargetActor)
{
    AActor* const Owner = GetOwner();
    if (!Owner || !TargetActor)
    {
        return;
    }

    const APawn* const OwnerPawn = Cast<APawn>(Owner);
    AController* const InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

    FDamageEvent DamageEvent(ActiveDamageTypeClass);
    TargetActor->TakeDamage(ActiveAttackDamage, DamageEvent, InstigatorController, Owner);
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

void UIFCombatComponent::SetOwnerWeaponCollisionEnabled(bool bEnabled) const
{
    if (AIFBaseCharacter* const OwnerCharacter = Cast<AIFBaseCharacter>(GetOwner()))
    {
        OwnerCharacter->SetWeaponCollisionEnabled(bEnabled);
    }
}

float UIFCombatComponent::GetComboStaminaCost(int32 ComboIndex) const
{
    if (!ComboSteps.IsValidIndex(ComboIndex))
    {
        return 0.f;
    }

    return FMath::Max(0.f, ComboSteps[ComboIndex].StaminaCost);
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
    UAnimInstance* const AnimInstance = GetAnimInstance();
    if (!AnimInstance || !ActiveAttackMontage)
    {
        return;
    }

    FOnMontageEnded EmptyDelegate;
    AnimInstance->Montage_SetEndDelegate(EmptyDelegate, ActiveAttackMontage);
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
    if (CombatState != ECombatState::Dead)
    {
        SetCombatState(ECombatState::Idle);
    }
}

