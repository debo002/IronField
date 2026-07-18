#include "AI/BTService_IFFindTarget.h"

#include "AI/IFBTUtils.h"
#include "AI/IFEnemyAIData.h"
#include "AI/IFEnemyController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Stats/IFHealthComponent.h"
#include "Wave/IFWaveManager.h"

namespace
{
	bool IsUsableTarget(AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return false;
		}

		if (const UIFHealthComponent* const Health = Actor->FindComponentByClass<UIFHealthComponent>())
		{
			return !Health->IsDead();
		}

		return true;
	}

	AIFWaveManager* ResolveWaveManager(const UBehaviorTreeComponent& OwnerComp)
	{
		if (const UWorld* const World = OwnerComp.GetWorld())
		{
			if (const UIFWaveManagerSubsystem* const Subsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
			{
				return Subsystem->GetWaveManager();
			}
		}
		return nullptr;
	}
}

UBTService_IFFindTarget::UBTService_IFFindTarget()
{
	NodeName = TEXT("Find Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;
	bCallTickOnSearchStart = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_IFFindTarget, TargetActorKey), AActor::StaticClass());
}

void UBTService_IFFindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	ChooseTarget(OwnerComp);
}

void UBTService_IFFindTarget::ChooseTarget(UBehaviorTreeComponent& OwnerComp) const
{
	const APawn* const Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	AIFWaveManager* const WaveManager = ResolveWaveManager(OwnerComp);
	UBlackboardComponent* const BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !WaveManager || !BB)
	{
		return;
	}

	const AIFEnemyController* const Controller = Cast<AIFEnemyController>(OwnerComp.GetAIOwner());
	const UIFEnemyAIData* const AIData = Controller ? Controller->GetAIData() : GetDefault<UIFEnemyAIData>();
	const float DetectionRange = AIData->PlayerDetectionRange;
	const float SwitchMargin = AIData->TargetSwitchMargin;

	AActor* const Player = WaveManager->GetPlayerActor();
	AActor* const Stronghold = WaveManager->GetStrongholdActor();
	AActor* const Current = GetBlackboardTargetActor(OwnerComp, TargetActorKey);

	const bool bPlayerOk = IsUsableTarget(Player)
		&& FVector::DistSquared(Pawn->GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(DetectionRange);
	const bool bStrongholdOk = IsUsableTarget(Stronghold);

	AActor* Desired = nullptr;

	if (bPlayerOk && bStrongholdOk)
	{
		const float DistPlayer = FVector::Dist(Pawn->GetActorLocation(), Player->GetActorLocation());
		const float DistStronghold = FVector::Dist(Pawn->GetActorLocation(), Stronghold->GetActorLocation());

		if (IsUsableTarget(Current) && (Current == Player || Current == Stronghold))
		{
			const float DistCurrent = (Current == Player) ? DistPlayer : DistStronghold;
			const float DistOther = (Current == Player) ? DistStronghold : DistPlayer;
			Desired = (DistOther + SwitchMargin < DistCurrent)
				? ((Current == Player) ? Stronghold : Player)
				: Current;
		}
		else
		{
			Desired = DistPlayer <= DistStronghold ? Player : Stronghold;
		}
	}
	else if (bPlayerOk)
	{
		Desired = Player;
	}
	else if (bStrongholdOk)
	{
		Desired = Stronghold;
	}

	if (Desired != Current)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, Desired);
	}
}
