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

		Movement->MaxWalkSpeed = 300.f;

		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
		Movement->RotationRate = FRotator(0.f, 320.f, 0.f);
	}
}
