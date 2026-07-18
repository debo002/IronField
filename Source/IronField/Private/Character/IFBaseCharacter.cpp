#include "Character/IFBaseCharacter.h"

#include "Combat/IFCombatComponent.h"
#include "Components/CapsuleComponent.h"
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

	// Cache the live profile names once so RestoreCollisionAfterDeath() can restore them exactly,
	// without hardcoding strings that can silently go stale if profiles are renamed.
	if (UCapsuleComponent* const Capsule = GetCapsuleComponent())
	{
		CapsuleCollisionProfile = Capsule->GetCollisionProfileName();
	}
	if (USkeletalMeshComponent* const MeshComp = GetMesh())
	{
		MeshCollisionProfile = MeshComp->GetCollisionProfileName();
	}

	// Cache original movement rotation settings
	if (const UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
		bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
	}

	BindGameplayDelegates();
}

void AIFBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindGameplayDelegates();
	Super::EndPlay(EndPlayReason);
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
	// bHasDied is the single re-entrancy guard — owned here, not derived from CombatComponent state.
	if (bHasDied)
	{
		return;
	}
	bHasDied = true;

	UE_LOG(LogIronField, Log, TEXT("[IF-Death] %s died."), *GetName());

	if (CombatComponent)
	{
		CombatComponent->HandleOwnerDeath();
	}

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

	// Stop the controller rotation from continuing to turn the dead pawn.
	Movement->bUseControllerDesiredRotation = false;
	Movement->bOrientRotationToMovement = false;
}

void AIFBaseCharacter::DisableCollisionForDeath()
{
	if (UCapsuleComponent* const Capsule = GetCapsuleComponent())
	{
		// Keep capsule collision enabled against the environment so it doesn't fall through the ground,
		// but ignore Pawns so other characters can walk through the corpse.
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
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
		Capsule->SetCollisionProfileName(CapsuleCollisionProfile);
		Capsule->SetCanEverAffectNavigation(true);
	}

	if (USkeletalMeshComponent* const LocalMesh = GetMesh())
	{
		LocalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		LocalMesh->SetCollisionProfileName(MeshCollisionProfile);
		LocalMesh->SetCanEverAffectNavigation(true);
	}

	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
		Movement->FindFloor(GetActorLocation(), Movement->CurrentFloor, false);

		// Restore original movement rotation settings
		Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		Movement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
	}
}


UAnimInstance* AIFBaseCharacter::GetMeshAnimInstance() const
{
	USkeletalMeshComponent* const LocalMesh = GetMesh();
	return LocalMesh ? LocalMesh->GetAnimInstance() : nullptr;
}
