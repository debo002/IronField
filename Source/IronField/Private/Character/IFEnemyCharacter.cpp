#include "Character/IFEnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AIFEnemyCharacter::AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UCharacterMovementComponent* const Movement = GetCharacterMovement();
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceWeight = 0.5f;
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.f, 480.f, 0.f);
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
	case ECombatState::Idle:
		Movement->MaxWalkSpeed = ChaseSpeed;
		break;
	case ECombatState::Dead:
		break;
	}
}
