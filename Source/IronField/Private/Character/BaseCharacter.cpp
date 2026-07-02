#include "Character/BaseCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Combat/CombatComponent.h"
#include "Core/AnimMontageUtils.h"
#include "Stats/HealthComponent.h"
#include "Stats/StaminaComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AIFBaseCharacter::AIFBaseCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;

    HealthComponent = CreateDefaultSubobject<UIFHealthComponent>(TEXT("Health"));
    StaminaComponent = CreateDefaultSubobject<UIFStaminaComponent>(TEXT("Stamina"));

    CombatComponent = CreateDefaultSubobject<UIFCombatComponent>(TEXT("Combat"));

    WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
    WeaponCollisionBox->SetupAttachment(GetMesh(), FName("weapon_r"));
    WeaponCollisionBox->SetBoxExtent(FVector(20.f, 60.f, 20.f));
    WeaponCollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    WeaponCollisionBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
    WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponCollisionBox->SetGenerateOverlapEvents(false);
}

void AIFBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    BindGameplayDelegates();
}

void AIFBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    ClearReviveTimer();
    ClearLifecycleMontageDelegates();
    UnbindGameplayDelegates();
}

void AIFBaseCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    if (HealthComponent && HealthComponent->IsDead())
    {
        if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
        {
            Movement->DisableMovement();
        }
    }
}

void AIFBaseCharacter::BindGameplayDelegates()
{
    if (StaminaComponent)
    {
        StaminaComponent->OnStaminaDepleted.AddDynamic(this, &AIFBaseCharacter::HandleStaminaDepleted);
    }
    
    if (HealthComponent)
    {
        HealthComponent->OnHealthDepleted.AddDynamic(this, &AIFBaseCharacter::HandleDeath);
    }
}

void AIFBaseCharacter::UnbindGameplayDelegates()
{
    if (StaminaComponent)
    {
        StaminaComponent->OnStaminaDepleted.RemoveAll(this);
    }

    if (HealthComponent)
    {
        HealthComponent->OnHealthDepleted.RemoveAll(this);
    }
}

void AIFBaseCharacter::ClearReviveTimer()
{
    if (UWorld* const World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReviveTimerHandle);
    }
}

void AIFBaseCharacter::HandleStaminaDepleted()
{
    if (!CombatComponent)
    {
        return;
    }

    CombatComponent->StopBlock();
    OnStaminaDepleted();
}

void AIFBaseCharacter::HandleDeath()
{
    if (!CombatComponent || CombatComponent->IsDead())
    {
        return;
    }

    ClearReviveTimer();
    ClearLifecycleMontageDelegates();

    CombatComponent->HandleOwnerDeath();

    if (UAnimInstance* const AnimInstance = GetMeshAnimInstance())
    {
        AnimInstance->Montage_Stop(0.15f);
    }

    if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
    {
        Movement->StopMovementImmediately();

        if (!Movement->IsFalling())
        {
            // Keep movement physics active during falls so the character lands before movement is stopped.
            Movement->DisableMovement();
        }
    }

    OnDeathStarted();

    if (!PlayMontageAndBindEnd(DeathMontage, &AIFBaseCharacter::HandleDeathMontageEnded))
    {
        StartReviveTimer();
    }
}

bool AIFBaseCharacter::PlayMontageAndBindEnd(UAnimMontage* Montage, void (AIFBaseCharacter::*EndHandler)(UAnimMontage*, bool))
{
    UAnimInstance* const AnimInstance = GetMeshAnimInstance();
    if (!Montage || !AnimInstance || PlayAnimMontage(Montage) <= 0.f)
    {
        return false;
    }

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, EndHandler);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

    return true;
}

void AIFBaseCharacter::HandleDeathMontageEnded(UAnimMontage* Montage, bool /*bInterrupted*/)
{
    if (Montage != DeathMontage)
    {
        return;
    }

    ClearLifecycleMontageDelegates();
    StartReviveTimer();
}

void AIFBaseCharacter::HandleGetUpMontageEnded(UAnimMontage* Montage, bool /*bInterrupted*/)
{
    if (Montage != GetUpMontage)
    {
        return;
    }

    ClearLifecycleMontageDelegates();
    CompleteRevive();
}

void AIFBaseCharacter::StartReviveTimer()
{
    UWorld* const World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        ReviveTimerHandle,
        this,
        &AIFBaseCharacter::AttemptRevive,
        ReviveDelaySeconds,
        FTimerManagerTimerParameters{ .bLoop = false }
    );
}

void AIFBaseCharacter::AttemptRevive()
{
    if (!HealthComponent || !CombatComponent)
    {
        return;
    }

    HealthComponent->Revive();
    // Grant temporary invincibility to prevent taking damage during the get-up animation.
    HealthComponent->SetInvincible(true);

    CombatComponent->HandleOwnerRevived();

    if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Walking);
    }

    if (!PlayMontageAndBindEnd(GetUpMontage, &AIFBaseCharacter::HandleGetUpMontageEnded))
    {
        CompleteRevive();
    }
}

void AIFBaseCharacter::CompleteRevive()
{
    if (HealthComponent)
    {
        HealthComponent->SetInvincible(false);
    }

    OnReviveFinished();
}

UAnimInstance* AIFBaseCharacter::GetMeshAnimInstance() const
{
    USkeletalMeshComponent* const LocalMesh = GetMesh();
    if (!LocalMesh)
    {
        return nullptr;
    }

    return LocalMesh->GetAnimInstance();
}

void AIFBaseCharacter::ClearLifecycleMontageDelegates() const
{
    UAnimInstance* const AnimInstance = GetMeshAnimInstance();
    IF::AnimMontageUtils::ClearMontageEndDelegate(AnimInstance, DeathMontage);
    IF::AnimMontageUtils::ClearMontageEndDelegate(AnimInstance, GetUpMontage);
}