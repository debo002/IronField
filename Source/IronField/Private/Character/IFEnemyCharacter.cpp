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

void AIFEnemyCharacter::RollAggression(float Bias, float Spread)
{
	Aggression = FMath::Clamp(BaseAggression + FMath::FRandRange(-Spread, Spread) - Bias, 0.f, 1.f);
}

void AIFEnemyCharacter::NotifyHitByPlayer()
{
	if (const UWorld* const World = GetWorld())
	{
		LastPlayerHitTime = World->GetTimeSeconds();
	}
}

void AIFEnemyCharacter::NotifyTargetSwitched()
{
	if (const UWorld* const World = GetWorld())
	{
		LastTargetSwitchTime = World->GetTimeSeconds();
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
