#include "Combat/IFCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Combat/IFCombatTargetingUtils.h"
#include "Combat/IFWeaponBoxOwner.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/IFAnimMontageUtils.h"
#include "Core/IFLog.h"
#include "GameFramework/Character.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"

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

	if (!BlockMontage)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Combat] %s StartBlock with no BlockMontage assigned — logic-only block."),
			*GetNameSafe(GetOwner()));
	}

	if (!TryPlayBlockMontage())
	{
		return;
	}

	SetCombatState(ECombatState::Blocking);

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

	SetCombatState(ECombatState::Idle);

	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (AnimInstance && ActiveBlockMontage)
	{
		AnimInstance->Montage_Stop(BlockBlendOutTime, ActiveBlockMontage);
	}

	ActiveBlockMontage = nullptr;

	if (StaminaComponent)
	{
		StaminaComponent->StopContinuousDrain();
	}
}

void UIFCombatComponent::ResetCombatState()
{
	bComboQueued = false;
	CurrentComboIndex = 0;
	ResetRegisteredAttackHits();
	ClearAttackMontageDelegate();
	ClearReactionMontageDelegates();
	ActiveAttackMontage = nullptr;
	ActiveBlockMontage = nullptr;
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

void UIFCombatComponent::ReceiveAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (IsDead())
	{
		return;
	}

	const bool bFacing = IsOwnerFacingTarget(Instigator);
	if ((IsBlocking() && bFacing) || ShouldReactivelyBlock(bFacing))
	{
		PlayBlockReactionMontage();
		return;
	}

	AActor* const Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	IFCombatTargetingUtils::ApplyDamageTo(Owner, Instigator, Damage, DamageTypeClass);

	if (IsDead())
	{
		return;
	}

	PlayHitReactionMontage();
}

void UIFCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* const Owner = GetOwner();
	const ACharacter* const OwnerCharacter = Cast<ACharacter>(Owner);
	CachedMesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	StaminaComponent = Owner ? Owner->FindComponentByClass<UIFStaminaComponent>() : nullptr;

	if (const IIFWeaponBoxOwner* const WeaponOwner = Cast<IIFWeaponBoxOwner>(Owner))
	{
		WeaponCollisionBox = WeaponOwner->GetWeaponCollisionBox();
	}

	if (WeaponCollisionBox)
	{
		WeaponCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &UIFCombatComponent::HandleWeaponBoxBeginOverlap);
	}
}

void UIFCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WeaponCollisionBox)
	{
		WeaponCollisionBox->OnComponentBeginOverlap.RemoveAll(this);
	}

	EndAttackCollision();
	ClearAttackMontageDelegate();
	ClearReactionMontageDelegates();
	ActiveBlockMontage = nullptr;

	if (StaminaComponent)
	{
		StaminaComponent->StopContinuousDrain();
	}

	Super::EndPlay(EndPlayReason);
}

float UIFCombatComponent::GetCurrentAttackDamage() const
{
	return ComboSteps.IsValidIndex(CurrentComboIndex) ? ComboSteps[CurrentComboIndex].Damage : 0.f;
}

TSubclassOf<UDamageType> UIFCombatComponent::GetCurrentDamageTypeClass() const
{
	return ComboSteps.IsValidIndex(CurrentComboIndex) ? ComboSteps[CurrentComboIndex].DamageTypeClass : nullptr;
}

bool UIFCombatComponent::CanQueueComboAttack() const
{
	return ActiveAttackMontage != nullptr && HasNextComboStep();
}

bool UIFCombatComponent::HasUsableStamina(float Amount) const
{
	if (Amount <= 0.f)
	{
		return true;
	}
	return StaminaComponent && StaminaComponent->HasStamina(Amount);
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

void UIFCombatComponent::RestoreIdleStateUnlessDead()
{
	if (!IsDead())
	{
		SetCombatState(ECombatState::Idle);
	}
}

void UIFCombatComponent::ResetRegisteredAttackHits()
{
	RegisteredAttackHits.Reset();
}

bool UIFCombatComponent::CanPlayAttackMontage(int32 ComboIndex) const
{
	if (!ComboSteps.IsValidIndex(ComboIndex) || !ComboSteps[ComboIndex].AttackMontage)
	{
		return false;
	}

	return GetAnimInstance() && HasUsableStamina(GetComboStaminaCost(ComboIndex));
}

bool UIFCombatComponent::TryPlayAttackMontage(int32 ComboIndex)
{
	if (!CanPlayAttackMontage(ComboIndex))
	{
		return false;
	}

	UAnimInstance* const AnimInstance = GetAnimInstance();
	UAnimMontage* const AttackMontage = ComboSteps[ComboIndex].AttackMontage;
	const float StaminaCost = GetComboStaminaCost(ComboIndex);

	ClearAttackMontageDelegate();

	const float PlayLength = AnimInstance->Montage_Play(AttackMontage);
	if (PlayLength <= 0.f)
	{
		return false;
	}

	if (StaminaCost > 0.f && (!StaminaComponent || !StaminaComponent->TryConsumeStamina(StaminaCost)))
	{
		AnimInstance->Montage_Stop(0.f, AttackMontage);
		return false;
	}

	ActiveAttackMontage = AttackMontage;
	CurrentComboIndex = ComboIndex;
	bComboQueued = false;
	ResetRegisteredAttackHits();
	SetCombatState(ECombatState::Attacking);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UIFCombatComponent::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);

	return true;
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

	const int32 NextComboIndex = CurrentComboIndex + 1;
	// bComboQueued = player input buffer. GetComboContinueChance is 0 except on melee AI.
	const bool bWantsNext = bComboQueued || (FMath::FRand() < GetComboContinueChance(CurrentComboIndex));
	bComboQueued = false;

	if (bWantsNext && TryPlayAttackMontage(NextComboIndex))
	{
		return;
	}

	ResetCombatState();
}

void UIFCombatComponent::ClearAttackMontageDelegate()
{
	IFAnimMontageUtils::ClearMontageEndDelegate(GetAnimInstance(), ActiveAttackMontage);
}

float UIFCombatComponent::GetComboStaminaCost(int32 ComboIndex) const
{
	return ComboSteps.IsValidIndex(ComboIndex) ? ComboSteps[ComboIndex].StaminaCost : 0.f;
}

bool UIFCombatComponent::TryPlayBlockMontage()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !BlockMontage)
	{
		ActiveBlockMontage = nullptr;
		return true;
	}

	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, BlockMontage);

	const float PlayLength = AnimInstance->Montage_Play(BlockMontage);
	if (PlayLength <= 0.f)
	{
		ActiveBlockMontage = nullptr;
		return false;
	}

	ActiveBlockMontage = BlockMontage;
	return true;
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

	return FVector::DotProduct(Owner->GetActorForwardVector().GetSafeNormal2D(), DirectionToTarget) >= BlockFacingDotThreshold;
}

void UIFCombatComponent::PlayHitReactionMontage()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !HitReactionMontage)
	{
		return;
	}

	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, HitReactionMontage);

	if (AnimInstance->Montage_Play(HitReactionMontage) > 0.f)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UIFCombatComponent::HandleHitReactionMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, HitReactionMontage);
	}
}

void UIFCombatComponent::PlayBlockReactionMontage()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !BlockReactionMontage)
	{
		return;
	}

	if (BlockMontage && AnimInstance->Montage_IsPlaying(BlockMontage))
	{
		AnimInstance->Montage_Stop(FMath::Max(0.01f, BlockBlendOutTime), BlockMontage);
	}

	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, BlockReactionMontage);

	const float PlayLength = AnimInstance->Montage_Play(BlockReactionMontage);
	if (PlayLength > 0.f)
	{
		ActiveBlockMontage = BlockReactionMontage;

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UIFCombatComponent::HandleBlockReactionMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, BlockReactionMontage);
	}
}

void UIFCombatComponent::HandleHitReactionMontageEnded(UAnimMontage* Montage, bool)
{
	if (Montage != HitReactionMontage)
	{
		return;
	}

	IFAnimMontageUtils::ClearMontageEndDelegate(GetAnimInstance(), HitReactionMontage);

	if (IsBlocking())
	{
		TryPlayBlockMontage();
	}
}

void UIFCombatComponent::HandleBlockReactionMontageEnded(UAnimMontage* Montage, bool)
{
	if (Montage != BlockReactionMontage)
	{
		return;
	}

	IFAnimMontageUtils::ClearMontageEndDelegate(GetAnimInstance(), BlockReactionMontage);
	ActiveBlockMontage = nullptr;

	if (IsBlocking())
	{
		StopBlock();
	}
}

void UIFCombatComponent::ClearReactionMontageDelegates()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, HitReactionMontage);
	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, BlockReactionMontage);
}

void UIFCombatComponent::HandleWeaponBoxBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	ResolveAttackHit(OtherActor);
}

void UIFCombatComponent::ResolveAttackHit(AActor* TargetActor)
{
	if (!GetValidActiveAttackTargetHealth(TargetActor) || !TryRegisterAttackHit(TargetActor))
	{
		return;
	}

	IFCombatTargetingUtils::DeliverDamage(TargetActor, GetOwner(), ActiveAttackDamage, ActiveDamageTypeClass);
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

UIFHealthComponent* UIFCombatComponent::GetValidActiveAttackTargetHealth(AActor* TargetActor) const
{
	if (!bAttackCollisionActive || !IsAttacking())
	{
		return nullptr;
	}

	return IFCombatTargetingUtils::GetValidAttackTargetHealth(GetOwner(), TargetActor);
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

UAnimInstance* UIFCombatComponent::GetAnimInstance() const
{
	return CachedMesh ? CachedMesh->GetAnimInstance() : nullptr;
}
