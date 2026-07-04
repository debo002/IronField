#include "Combat/IFCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Character/IFBaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Core/IFAnimMontageUtils.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"
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
		AnimInstance->Montage_Stop(BlockBlendOutTime, BlockMontage);
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

	if (RegisteredAttackHits.Contains(TargetActor))
	{
		return false;
	}

	RegisteredAttackHits.Add(TargetActor);
	return true;
}

float UIFCombatComponent::GetCurrentAttackDamage() const
{
	return ComboSteps.IsValidIndex(CurrentComboIndex) ? ComboSteps[CurrentComboIndex].Damage : 0.f;
}

TSubclassOf<UDamageType> UIFCombatComponent::GetCurrentDamageTypeClass() const
{
	return ComboSteps.IsValidIndex(CurrentComboIndex) ? ComboSteps[CurrentComboIndex].DamageTypeClass : nullptr;
}

void UIFCombatComponent::BeginAttackCollision()
{
	if (!IsAttacking())
	{
		return;
	}

	const float Damage = GetCurrentAttackDamage();
	if (Damage <= 0.f)
	{
		return;
	}

	ActiveAttackDamage = Damage;
	ActiveDamageTypeClass = GetCurrentDamageTypeClass();
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

void UIFCombatComponent::HandleWeaponBoxBeginOverlap(UPrimitiveComponent* , AActor* OtherActor, UPrimitiveComponent* , int32 , bool , const FHitResult& )
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
		TargetCombat->ReceiveMeleeAttack(GetOwner(), ActiveAttackDamage, ActiveDamageTypeClass);
		return;
	}

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

	if (!OwnHealthComponent || !OwnHealthComponent->IsDead())
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
	OwnHealthComponent = Owner ? Owner->FindComponentByClass<UIFHealthComponent>() : nullptr;

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
	EndDelegate.BindUObject(this, &UIFCombatComponent::HandleAttackMontageEnded);
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
	ClearMontageEndDelegate(GetAnimInstance(), ActiveAttackMontage);
}

void UIFCombatComponent::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
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