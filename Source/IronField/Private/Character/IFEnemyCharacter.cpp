#include "Character/IFEnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AIFEnemyCharacter::AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->bUseRVOAvoidance = true;
		Movement->AvoidanceWeight = 0.5f;
		Movement->bOrientRotationToMovement = true;
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
	case ECombatState::Idle:
		Movement->MaxWalkSpeed = ChaseSpeed;
		break;
	default:
		break;
	}
}
