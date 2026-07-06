#include "Character/IFEnemyCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AIFEnemyCharacter::AIFEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		// RVO keeps enemies from stacking on top of each other in groups.
		Movement->bUseRVOAvoidance = true;
		Movement->AvoidanceWeight = 0.5f;

		// Souls-like baseline: enemies close distance at a purposeful but readable pace.
		// 300 is slow enough that a player who breaks contact can create breathing room;
		// fast enough that ignoring an enemy is dangerous.
		Movement->MaxWalkSpeed = 300.f;

		// Deliberate rotation: enemies track the player visibly but cannot instantly face-snap.
		// 320 degrees/sec means a 180-degree turn takes ~0.56 s — enough that strafing behind
		// an enemy pays off without making it trivially easy.
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
		Movement->RotationRate = FRotator(0.f, 320.f, 0.f);
	}
}

