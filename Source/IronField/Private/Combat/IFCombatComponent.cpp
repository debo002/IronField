#include "Combat/IFCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Combat/IFCombatTargetingUtils.h"
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

void UIFCombatComponent::ResetCombatState()
{
	bComboQueued = false;
	CurrentComboIndex = 0;
	ResetRegisteredAttackHits();
	ClearAttackMontageDelegate();
	ClearReactionMontageDelegates();
	ActiveAttackMontage = nullptr;
	EndAttackCollision();

	RestoreIdleStateUnlessDead();
}

void UIFCombatComponent::CancelAttack()
{
	ClearAttackMontageDelegate();

	if (UAnimInstance* const AnimInstance = GetAnimInstance())
	{
		if (ActiveAttackMontage)
		{
			AnimInstance->Montage_Stop(0.15f, ActiveAttackMontage);
		}
	}

	ResetCombatState();
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

	// Stationary targets already inside the box do not reliably produce a new
	// BeginOverlap when the box toggles on, so sweep the current overlappers too.
	// TryRegisterAttackHit makes the event path + this query mutually exclusive.
	if (WeaponCollisionBox)
	{
		TArray<AActor*> CurrentlyOverlapping;
		WeaponCollisionBox->GetOverlappingActors(CurrentlyOverlapping);
		for (AActor* const Other : CurrentlyOverlapping)
		{
			ResolveAttackHit(Other);
		}
	}
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

	if (WeaponCollisionBox)
	{
		// A weapon box with no attach parent sits at the world origin and can never
		// touch a target. Anchor it to the owner so it at least follows the fighter.
		if (!WeaponCollisionBox->GetAttachParent())
		{
			UE_LOG(LogIronField, Warning, TEXT("[IF-Combat] %s WeaponCollisionBox has no attach parent; auto-attaching to the root. Set the intended parent/socket in Blueprint."),
				*GetNameSafe(Owner));

			if (USceneComponent* const Parent = Owner ? Owner->GetRootComponent() : nullptr)
			{
				WeaponCollisionBox->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
			}
		}

		// Untouched UE defaults (32cm cube at the origin) can never reach a target
		// the AI considers in range. Apply weapon defaults that cover CombatRange;
		// any Blueprint-configured transform is respected as-is.
		if (WeaponCollisionBox->GetRelativeLocation().IsNearlyZero()
			&& WeaponCollisionBox->GetScaledBoxExtent().Equals(FVector(32.f), 1.f))
		{
			WeaponCollisionBox->SetRelativeLocation(FVector(60.f, 0.f, 0.f));
			WeaponCollisionBox->SetBoxExtent(FVector(80.f, 60.f, 60.f));
		}

		// The box must start disabled; it is only live inside an attack window.
		// (BP defaults leave it enabled, which produced idle-contact events.)
		SetWeaponCollisionEnabled(false);
		WeaponCollisionBox->OnComponentBeginOverlap.AddDynamic(this, &UIFCombatComponent::HandleWeaponBoxBeginOverlap);
	}
	else if (RequiresWeaponCollisionBox())
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Combat] %s has no WeaponCollisionBox assigned; attacks will not register hits."), *GetNameSafe(Owner));
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

	// Stamina is a player-only component. Enemies do not need a stamina setup
	// to use an authored attack montage.
	return !StaminaComponent || StaminaComponent->HasStamina(Amount);
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

	if (StaminaCost > 0.f && StaminaComponent && !StaminaComponent->TryConsumeStamina(StaminaCost))
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

void UIFCombatComponent::HandleHitReactionMontageEnded(UAnimMontage* Montage, bool)
{
	if (Montage != HitReactionMontage)
	{
		return;
	}

	IFAnimMontageUtils::ClearMontageEndDelegate(GetAnimInstance(), HitReactionMontage);
}

void UIFCombatComponent::ClearReactionMontageDelegates()
{
	UAnimInstance* const AnimInstance = GetAnimInstance();
	IFAnimMontageUtils::ClearMontageEndDelegate(AnimInstance, HitReactionMontage);
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
