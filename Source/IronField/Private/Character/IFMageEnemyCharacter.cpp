#include "Character/IFMageEnemyCharacter.h"

#include "AI/IFEnemyController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Combat/IFMageCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AIFMageEnemyCharacter::AIFMageEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.SetDefaultSubobjectClass<UIFMageCombatComponent>(TEXT("Combat"))
		.DoNotCreateDefaultSubobject(TEXT("Stamina")))
{
	PrimaryActorTick.bCanEverTick = true;

	CombatRange = 900.f;
	ChaseSpeed = 320.f;
	AttackingSpeed = 200.f;

	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate = FRotator(0.f, 220.f, 0.f);
		Movement->MaxAcceleration = 600.f;
		Movement->BrakingDecelerationWalking = 600.f;
	}

	StaffMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Staff"));
	StaffMesh->SetupAttachment(GetMesh(), TEXT("weapon_r"));
	StaffMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaffMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AIFMageEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFocusOnTarget();
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
	const FName TargetKey = EnemyController ? EnemyController->GetTargetActorKeyName() : FName(TEXT("TargetActor"));
	AActor* const Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetKey));
	if (Target)
	{
		AIController->SetFocus(Target);
	}
	else
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
