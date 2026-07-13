#include "Combat/IFCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "AIController.h"
#include "Building/IFStronghold.h"
#include "Character/IFBaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/IFLog.h"
#include "Core/IFAnimMontageUtils.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
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

	// Montage_Stop can fire its end delegate immediately, so leave Blocking before stopping.
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

void UIFCombatComponent::ReceiveMeleeAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (IsDead())
	{
		return;
	}

	const bool bFacing = IsOwnerFacingTarget(Instigator);

	if (IsBlocking() && bFacing)
	{
		PlayBlockReactionMontage();
		return;
	}

	if (ShouldReactivelyBlock(bFacing))
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

	const AIFBaseCharacter* const BaseCharacterOwner = Cast<AIFBaseCharacter>(Owner);
	WeaponCollisionBox = BaseCharacterOwner ? BaseCharacterOwner->GetWeaponCollisionBox() : nullptr;

	if (WeaponCollisionBox)
	{
		WeaponCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &UIFCombatComponent::HandleWeaponBoxBeginOverlap);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Combat] %s has no WeaponCollisionBox - attacks will never register hits."),
			*GetNameSafe(Owner));
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
	ClearReactionMontageDelegates();
	ActiveBlockMontage = nullptr;

	if (StaminaComponent)
	{
		StaminaComponent->StopContinuousDrain();
	}
}


float UIFCombatComponent::GetComboContinueChance(int32 ComboIndex) const
{
	return ComboSteps.IsValidIndex(ComboIndex) ? ComboSteps[ComboIndex].AIContinueChance : 0.f;
}

float UIFCombatComponent::GetCurrentAttackDamage() const
{
	return ComboSteps.IsValidIndex(CurrentComboIndex) ? ComboSteps[CurrentComboIndex].Damage : 0.f;
}

TSubclassOf<UDamageType> UIFCombatComponent::GetCurrentDamageTypeClass() const
{
	return ComboSteps.IsValidIndex(CurrentComboIndex) ? ComboSteps[CurrentComboIndex].DamageTypeClass : nullptr;
}

bool UIFCombatComponent::HasUsableStamina(float Amount) const
{
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

bool UIFCombatComponent::TryPlayAttackMontage(int32 ComboIndex)
{
	if (!CanPlayAttackMontage(ComboIndex) || !StaminaComponent)
	{
		return false;
	}

	UAnimInstance* const AnimInstance = GetAnimInstance();
	UAnimMontage* const AttackMontage = ComboSteps[ComboIndex].AttackMontage;
	const float StaminaCost = GetComboStaminaCost(ComboIndex);

	// Play first so a bad montage never spends stamina. Cost was already gated by CanPlayAttackMontage.
	ClearAttackMontageDelegate();

	const float PlayLength = AnimInstance->Montage_Play(AttackMontage);
	if (PlayLength <= 0.f)
	{
		return false;
	}

	if (!StaminaComponent->TryConsumeStamina(StaminaCost))
	{
		// Race with continuous drain (sprint/block/spin) between the gate check and now.
		AnimInstance->Montage_Stop(0.f, AttackMontage);
		return false;
	}

	ActiveAttackMontage = AttackMontage;
	CurrentComboIndex = ComboIndex;
	bComboQueued = false;
	ResetRegisteredAttackHits();
	SetCombatState(ECombatState::Attacking);
	OnComboStepStarted.Broadcast(CurrentComboIndex);

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

void UIFCombatComponent::ClearAttackMontageDelegate()
{
	ClearMontageEndDelegate(GetAnimInstance(), ActiveAttackMontage);
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
		// Still allow blocking without a montage so stamina drain and damage rejection work.
		ActiveBlockMontage = nullptr;
		return true;
	}

	ClearMontageEndDelegate(AnimInstance, BlockMontage);

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

	const FVector OwnerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(OwnerForward, DirectionToTarget);
	return Dot >= BlockFacingDotThreshold;
}


void UIFCombatComponent::PlayHitReactionMontage()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !HitReactionMontage)
	{
		return;
	}

	ClearMontageEndDelegate(AnimInstance, HitReactionMontage);

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

	ClearMontageEndDelegate(AnimInstance, BlockReactionMontage);

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

	ClearMontageEndDelegate(GetAnimInstance(), HitReactionMontage);

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

	ClearMontageEndDelegate(GetAnimInstance(), BlockReactionMontage);
	ActiveBlockMontage = nullptr;

	if (IsBlocking())
	{
		StopBlock();
	}
}

void UIFCombatComponent::ClearReactionMontageDelegates()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	ClearMontageEndDelegate(AnimInstance, HitReactionMontage);
	ClearMontageEndDelegate(AnimInstance, BlockReactionMontage);
}


void UIFCombatComponent::HandleWeaponBoxBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
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

	const APawn* const OwnerPawn = Cast<APawn>(Owner);
	const APawn* const TargetPawn = Cast<APawn>(TargetActor);
	if (OwnerPawn && TargetPawn)
	{
		const bool bOwnerIsAI = OwnerPawn->GetController() && OwnerPawn->GetController()->IsA<AAIController>();
		const bool bTargetIsAI = TargetPawn->GetController() && TargetPawn->GetController()->IsA<AAIController>();
		if (bOwnerIsAI && bTargetIsAI)
		{
			return nullptr;
		}
	}

	// The stronghold is the player's own objective to defend - only enemy AI may damage it, never the player.
	if (Cast<AIFStronghold>(TargetActor))
	{
		const bool bOwnerIsAI = OwnerPawn && OwnerPawn->GetController() && OwnerPawn->GetController()->IsA<AAIController>();
		if (!bOwnerIsAI)
		{
			return nullptr;
		}
	}

	UIFHealthComponent* const TargetHealth = TargetActor->FindComponentByClass<UIFHealthComponent>();
	return (TargetHealth && !TargetHealth->IsDead()) ? TargetHealth : nullptr;
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
