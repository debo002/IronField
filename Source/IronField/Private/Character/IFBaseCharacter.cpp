#include "Character/IFBaseCharacter.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Combat/IFCombatComponent.h"
#include "Core/IFLog.h"
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
	WeaponCollisionBox->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponCollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// ECC_GameTraceChannel1 is the project "Hittable Objectives" channel (e.g. stronghold hitbox).
	WeaponCollisionBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponCollisionBox->SetGenerateOverlapEvents(false);

	if (UCapsuleComponent* const CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	if (USkeletalMeshComponent* const MeshComp = GetMesh())
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}


bool AIFBaseCharacter::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

bool AIFBaseCharacter::IsBlocking() const
{
	return CombatComponent && CombatComponent->IsBlocking();
}

bool AIFBaseCharacter::IsAttacking() const
{
	return CombatComponent && CombatComponent->IsAttacking();
}


void AIFBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	AttachWeaponCollisionToSocket();
	BindGameplayDelegates();
}

void AIFBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnbindGameplayDelegates();
}

void AIFBaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Corpses keep falling physics until they land, then become fully inert.
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

	UE_LOG(LogIronField, Log, TEXT("[IF-Death] %s died."), *GetName());

	CombatComponent->HandleOwnerDeath();

	if (UAnimInstance* const AnimInstance = GetMeshAnimInstance())
	{
		AnimInstance->Montage_Stop(DeathMontageBlendOutTime);
	}

	StopMovementForDeath();
	DisableCollisionForDeath();

	OnDeathStarted();
	OnCharacterDied.Broadcast(this);

	// After this, death is visual only — AnimBP owns the pose.
	OnDeathSequenceStarted();
}

void AIFBaseCharacter::StopMovementForDeath()
{
	UCharacterMovementComponent* const Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	Movement->StopMovementImmediately();
	Movement->SetAvoidanceEnabled(false);

	if (!Movement->IsFalling())
	{
		Movement->DisableMovement();
	}
}

void AIFBaseCharacter::DisableCollisionForDeath()
{
	if (UCapsuleComponent* const Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		Capsule->SetCanEverAffectNavigation(false);
	}

	if (USkeletalMeshComponent* const LocalMesh = GetMesh())
	{
		LocalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LocalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		LocalMesh->SetCanEverAffectNavigation(false);
	}
}

void AIFBaseCharacter::RestoreCollisionAfterDeath()
{
	if (UCapsuleComponent* const Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionProfileName(FName("Pawn"));
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Capsule->SetCanEverAffectNavigation(true);
	}

	if (USkeletalMeshComponent* const LocalMesh = GetMesh())
	{
		LocalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		LocalMesh->SetCollisionProfileName(FName("CharacterMesh"));
		LocalMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		LocalMesh->SetCanEverAffectNavigation(true);
	}

	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void AIFBaseCharacter::AttachWeaponCollisionToSocket()
{
	if (!WeaponCollisionBox || !GetMesh() || WeaponSocketName.IsNone())
	{
		return;
	}

	// KeepRelative preserves box offsets tuned on the Blueprint; only the socket name is reapplied.
	WeaponCollisionBox->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::KeepRelativeTransform,
		WeaponSocketName);
}

UAnimInstance* AIFBaseCharacter::GetMeshAnimInstance() const
{
	USkeletalMeshComponent* const LocalMesh = GetMesh();
	return LocalMesh ? LocalMesh->GetAnimInstance() : nullptr;
}
