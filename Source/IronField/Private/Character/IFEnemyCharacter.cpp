#include "Character/IFEnemyCharacter.h"

#include "Combat/IFEnemyCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AIFEnemyCharacter::AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIFEnemyCombatComponent>(TEXT("Combat")))
{
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->bUseRVOAvoidance = true;
		Movement->AvoidanceWeight = 0.5f;

		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
		Movement->RotationRate = FRotator(0.f, 320.f, 0.f);
	}
}

void AIFEnemyCharacter::ApplyMovementSpeedForState(ECombatState State)
{
	UCharacterMovementComponent* const Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	switch (State)
	{
	case ECombatState::Attacking:
		Movement->MaxWalkSpeed = AttackingSpeed;
		break;

	case ECombatState::Blocking:
		Movement->MaxWalkSpeed = BlockingSpeed;
		break;

	case ECombatState::Dead:
		Movement->MaxWalkSpeed = 0.f;
		break;

	case ECombatState::Idle:
	default:
		Movement->MaxWalkSpeed = ChaseSpeed;
		break;
	}
}
