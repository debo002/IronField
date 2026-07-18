#include "AI/BTTask_IFMoveAwayFromTarget.h"

#include "AIController.h"
#include "AI/IFBTUtils.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

namespace
{
	struct FIFMoveAwayMemory
	{
		FVector LastDestination = FVector::ZeroVector;
		float TimeSinceRepath = 0.f;
		bool bHasDestination = false;
	};

	// Finish slightly inside the ideal range so we don't thrash at the boundary.
	constexpr float SuccessRangeTolerance = 0.95f;
	constexpr float RepathInterval = 0.25f;
	constexpr float DestinationDriftThreshold = 120.f;
	constexpr float AcceptanceRadius = 50.f;

	FVector ComputeRetreatDirection(const AActor* Enemy, const AActor* Target)
	{
		FVector Away = (Enemy->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			// Exact overlap (e.g. spawn/teleport) — pick a stable horizontal fallback.
			Away = FMath::VRand().GetSafeNormal2D();
			if (Away.IsNearlyZero())
			{
				Away = FVector::ForwardVector;
			}
		}
		return Away;
	}

	FVector ProjectRetreatDestination(UWorld* World, const FVector& Desired)
	{
		if (!World)
		{
			return Desired;
		}

		UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSys)
		{
			return Desired;
		}

		FNavLocation Projected;
		if (NavSys->ProjectPointToNavigation(Desired, Projected))
		{
			return Projected.Location;
		}

		return Desired;
	}

	bool RequestRetreatMove(AAIController* AIController, AIFEnemyCharacter* Enemy, AActor* Target, FIFMoveAwayMemory* Memory)
	{
		const float Range = Enemy->GetCombatRange();
		const FVector Away = ComputeRetreatDirection(Enemy, Target);
		const FVector Desired = Target->GetActorLocation() + Away * Range;
		const FVector Destination = ProjectRetreatDestination(AIController->GetWorld(), Desired);

		const EPathFollowingRequestResult::Type Result = AIController->MoveToLocation(Destination, AcceptanceRadius, false);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			return false;
		}

		Memory->LastDestination = Destination;
		Memory->bHasDestination = true;
		Memory->TimeSinceRepath = 0.f;
		return true;
	}
}

UBTTask_IFMoveAwayFromTarget::UBTTask_IFMoveAwayFromTarget()
{
	NodeName = TEXT("Move Away From Target");
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_IFMoveAwayFromTarget, TargetActorKey), AActor::StaticClass());
}

uint16 UBTTask_IFMoveAwayFromTarget::GetInstanceMemorySize() const
{
	return sizeof(FIFMoveAwayMemory);
}

EBTNodeResult::Type UBTTask_IFMoveAwayFromTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	new (NodeMemory) FIFMoveAwayMemory();

	AAIController* const AIController = OwnerComp.GetAIOwner();
	AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!AIController || !Enemy || !Target)
	{
		return EBTNodeResult::Failed;
	}

	const float Range = Enemy->GetCombatRange();
	const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), Target->GetActorLocation());
	if (DistSq >= FMath::Square(Range))
	{
		return EBTNodeResult::Succeeded;
	}

	FIFMoveAwayMemory* const Memory = reinterpret_cast<FIFMoveAwayMemory*>(NodeMemory);
	if (!RequestRetreatMove(AIController, Enemy, Target, Memory))
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_IFMoveAwayFromTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	FIFMoveAwayMemory* const Memory = reinterpret_cast<FIFMoveAwayMemory*>(NodeMemory);
	AAIController* const AIController = OwnerComp.GetAIOwner();
	AIFEnemyCharacter* const Enemy = GetControlledEnemy(OwnerComp);
	AActor* const Target = GetBlackboardTargetActor(OwnerComp, TargetActorKey);
	if (!Memory || !AIController || !Enemy || !Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float Range = Enemy->GetCombatRange();
	const float DistSq = FVector::DistSquared(Enemy->GetActorLocation(), Target->GetActorLocation());
	if (DistSq >= FMath::Square(Range * SuccessRangeTolerance))
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	Memory->TimeSinceRepath += DeltaSeconds;

	const FVector Away = ComputeRetreatDirection(Enemy, Target);
	const FVector Desired = Target->GetActorLocation() + Away * Range;
	const FVector Destination = ProjectRetreatDestination(AIController->GetWorld(), Desired);

	const UPathFollowingComponent* const PathFollowing = AIController->GetPathFollowingComponent();
	const bool bIsMoving = PathFollowing && PathFollowing->GetStatus() == EPathFollowingStatus::Moving;
	const bool bDestinationDrifted = !Memory->bHasDestination
		|| FVector::DistSquared(Destination, Memory->LastDestination) > FMath::Square(DestinationDriftThreshold);

	if ((!bIsMoving || bDestinationDrifted) && Memory->TimeSinceRepath >= RepathInterval)
	{
		if (!RequestRetreatMove(AIController, Enemy, Target, Memory))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		}
	}
}
