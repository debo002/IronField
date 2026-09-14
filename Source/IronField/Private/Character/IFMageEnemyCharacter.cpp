#include "Character/IFMageEnemyCharacter.h"

#include "AI/IFEnemyController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/IFMageCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AIFMageEnemyCharacter::AIFMageEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.SetDefaultSubobjectClass<UIFMageCombatComponent>(TEXT("Combat"))
		.DoNotCreateDefaultSubobject(TEXT("Stamina")))
{
	PrimaryActorTick.bCanEverTick = true;

	CombatRange = 850.f;

	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
	}
}

void AIFMageEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFocusOnTarget();
}

void AIFMageEnemyCharacter::OnDeathStarted()
{
	SetActorTickEnabled(false);
}

void AIFMageEnemyCharacter::UpdateFocusOnTarget()
{
	if (IsDead())
	{
		return;
	}

	AAIController* const AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	const UBlackboardComponent* const Blackboard = AIController->GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	const AIFEnemyController* const EnemyController = Cast<AIFEnemyController>(AIController);
	if (!EnemyController)
	{
		return;
	}

	AActor* const DesiredFocus = Cast<AActor>(Blackboard->GetValueAsObject(EnemyController->GetTargetActorKeyName()));

	if (DesiredFocus == CurrentFocusTarget.Get())
	{
		return;
	}

	CurrentFocusTarget = DesiredFocus;
	if (DesiredFocus)
	{
		AIController->SetFocus(DesiredFocus);
	}
	else
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
