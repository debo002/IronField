#include "Character/IFBaseCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Combat/IFCombatComponent.h"
#include "Core/IFAnimMontageUtils.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AIFBaseCharacter::AIFBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	HealthComponent = CreateDefaultSubobject<UIFHealthComponent>(TEXT("Health"));
	StaminaComponent = CreateDefaultSubobject<UIFStaminaComponent>(TEXT("Stamina"));

	CombatComponent = CreateDefaultSubobject<UIFCombatComponent>(TEXT("Combat"));

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
	WeaponCollisionBox->SetupAttachment(GetMesh(), FName("weapon_r"));
	WeaponCollisionBox->SetBoxExtent(WeaponCollisionExtent);
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

	ClearLifecycleMontageDelegates();

	CombatComponent->HandleOwnerDeath();

	UAnimInstance* const AnimInstance = GetMeshAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(DeathMontageBlendOutTime);
	}

	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();

		if (!Movement->IsFalling())
		{
			Movement->DisableMovement();
		}
	}

	OnDeathStarted();

	// Wait for the death montage to finish before calling OnDeathMontageFinished
	// (or call it right away if there's no montage).
	if (DeathMontage && AnimInstance && PlayAnimMontage(DeathMontage) > 0.f)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AIFBaseCharacter::HandleDeathMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, DeathMontage);
	}
	else
	{
		OnDeathMontageFinished();
	}
}

void AIFBaseCharacter::HandleDeathMontageEnded(UAnimMontage* Montage, bool )
{
	if (Montage != DeathMontage)
	{
		return;
	}

	ClearLifecycleMontageDelegates();
	OnDeathMontageFinished();
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
	ClearMontageEndDelegate(GetMeshAnimInstance(), DeathMontage);
}