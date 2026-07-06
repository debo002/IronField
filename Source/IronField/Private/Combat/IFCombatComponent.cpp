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
#include "AIController.h"

UIFCombatComponent::UIFCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ============================================================================
// Actions
// ============================================================================

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

	// Set state to Idle first so any montage end delegate triggered by Montage_Stop
	// does not see IsBlocking()==true and accidentally restart the block loop.
	SetCombatState(ECombatState::Idle);

	UAnimInstance* const AnimInstance = GetAnimInstance();

	// Stop whichever block-family montage is actually playing.
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

	// Active block: character is explicitly in the Blocking state and facing the attacker.
	if (IsBlocking() && bFacing)
	{
		PlayBlockReactionMontage();
		return;
	}

	// Reactive block chance: AI characters can roll to block even when not in the Blocking state.
	// Set EnemyBlockChance > 0 on the enemy's combat component; leave at 0 for the player.
	if (EnemyBlockChance > 0.f && bFacing && FMath::FRand() < EnemyBlockChance)
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

	// Damage may have killed this character — don't play a hit reaction over the death montage.
	if (IsDead())
	{
		return;
	}

	PlayHitReactionMontage();
}

// ============================================================================
// Lifecycle
// ============================================================================

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
		// Without this, attacks will play animations but never register a hit on anyone.
		UE_LOG(LogTemp, Warning, TEXT("[IF-Combat] %s has no WeaponCollisionBox — attacks will never register hits."),
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

// ============================================================================
// Queries
// ============================================================================

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

// ============================================================================
// Internal helpers - state
// ============================================================================

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

// ============================================================================
// Internal helpers - attack montages
// ============================================================================

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
	if (!CanPlayAttackMontage(ComboIndex))
	{
		return false;
	}

	// CanPlayAttackMontage already confirmed StaminaComponent is valid via HasUsableStamina,
	// but that dependency isn't obvious from this function alone — guard explicitly.
	if (!StaminaComponent)
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

// ============================================================================
// Internal helpers - block
// ============================================================================

bool UIFCombatComponent::TryPlayBlockMontage()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	if (!AnimInstance || !BlockMontage)
	{
		// No block pose configured — treat as trivially successful so the state machine still
		// enters Blocking (useful while blocking anims aren't authored yet).
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

// ============================================================================
// Internal helpers - reaction montages
// ============================================================================

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

	// Stop the idle block loop first so the reaction owns the slot cleanly.
	if (BlockMontage && AnimInstance->Montage_IsPlaying(BlockMontage))
	{
		AnimInstance->Montage_Stop(FMath::Max(0.01f, BlockBlendOutTime), BlockMontage);
	}

	ClearMontageEndDelegate(AnimInstance, BlockReactionMontage);

	const float PlayLength = AnimInstance->Montage_Play(BlockReactionMontage);
	if (PlayLength > 0.f)
	{
		// Reaction now owns the block slot until it ends or StopBlock interrupts it.
		ActiveBlockMontage = BlockReactionMontage;

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UIFCombatComponent::HandleBlockReactionMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, BlockReactionMontage);
	}
}

void UIFCombatComponent::HandleHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != HitReactionMontage)
	{
		return;
	}

	ClearMontageEndDelegate(GetAnimInstance(), HitReactionMontage);

	// If block was pressed while the hit reaction was playing, restore the block pose now.
	if (IsBlocking())
	{
		TryPlayBlockMontage();
	}
}

void UIFCombatComponent::HandleBlockReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != BlockReactionMontage)
	{
		return;
	}

	ClearMontageEndDelegate(GetAnimInstance(), BlockReactionMontage);
	ActiveBlockMontage = nullptr;

	// After a block reaction the guard is always broken — return to Idle.
	// The player must release and re-press Block to block again.
	// (If StopBlock was already called while the reaction played, this is a safe no-op.)
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

// ============================================================================
// Internal helpers - hit resolution
// ============================================================================

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

	// No friendly fire: skip the hit if both sides are AI-controlled.
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
	if (!CachedMesh)
	{
		return nullptr;
	}

	return CachedMesh->GetAnimInstance();
}