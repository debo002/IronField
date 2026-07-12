#include "AI/BTService_IFFindTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/IFBaseCharacter.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Wave/IFWaveManager.h"

namespace
{
	struct FIFFindTargetMemory
	{
		TWeakObjectPtr<AIFWaveManager> WaveManager;
		bool bHoldingPlayerSlot = false;
	};

	AIFWaveManager* ResolveWaveManager(UBehaviorTreeComponent& OwnerComp)
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

	Interval = 1.f;
	RandomDeviation = 0.1f;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_IFFindTarget, TargetActorKey), AActor::StaticClass());
}

uint16 UBTService_IFFindTarget::GetInstanceMemorySize() const
{
	return sizeof(FIFFindTargetMemory);
}

void UBTService_IFFindTarget::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	FIFFindTargetMemory* const Memory = new (NodeMemory) FIFFindTargetMemory();
	AIFWaveManager* const WaveManager = ResolveWaveManager(OwnerComp);
	Memory->WaveManager = WaveManager;

	if (!WaveManager)
	{
		return;
	}

	UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* const ExistingTarget = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	if (ExistingTarget && ExistingTarget == WaveManager->GetPlayerActor())
	{
		AIFBaseCharacter* const PlayerChar = Cast<AIFBaseCharacter>(ExistingTarget);
		if (PlayerChar && !PlayerChar->IsDead() && WaveManager->TryReserveEngagementSlot())
		{
			Memory->bHoldingPlayerSlot = true;
			return;
		}
	}

	if (ExistingTarget && ExistingTarget == WaveManager->GetStrongholdActor())
	{
		return;
	}

	PickInitialTarget(OwnerComp, NodeMemory, *WaveManager);
}

void UBTService_IFFindTarget::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FIFFindTargetMemory* const Memory = reinterpret_cast<FIFFindTargetMemory*>(NodeMemory);

	if (Memory && Memory->bHoldingPlayerSlot)
	{
		if (AIFWaveManager* const WaveManager = Memory->WaveManager.Get())
		{
			WaveManager->ReleaseEngagementSlot();
		}
	}

	if (Memory)
	{
		Memory->~FIFFindTargetMemory();
	}

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTService_IFFindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FIFFindTargetMemory* const Memory = reinterpret_cast<FIFFindTargetMemory*>(NodeMemory);
	if (!Memory)
	{
		return;
	}

	AIFWaveManager* WaveManager = Memory->WaveManager.Get();
	if (!WaveManager)
	{
		WaveManager = ResolveWaveManager(OwnerComp);
		Memory->WaveManager = WaveManager;
		if (!WaveManager)
		{
			return;
		}
	}

	UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	AActor* const CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));

	if (CurrentTarget && CurrentTarget == WaveManager->GetPlayerActor())
	{
		AIFBaseCharacter* const PlayerChar = Cast<AIFBaseCharacter>(CurrentTarget);
		if (PlayerChar && PlayerChar->IsDead())
		{
			if (Memory->bHoldingPlayerSlot)
			{
				WaveManager->ReleaseEngagementSlot();
				Memory->bHoldingPlayerSlot = false;
			}
			SetBlackboardTarget(OwnerComp, WaveManager->GetStrongholdActor());
			return;
		}
	}

	if (!CurrentTarget)
	{
		PickInitialTarget(OwnerComp, NodeMemory, *WaveManager);
		return;
	}

	if (CurrentTarget == WaveManager->GetStrongholdActor())
	{
		ReEvaluateStrongholdTarget(OwnerComp, NodeMemory, *WaveManager);
	}
}

void UBTService_IFFindTarget::PickInitialTarget(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, AIFWaveManager& WaveManager) const
{
	FIFFindTargetMemory* const Memory = reinterpret_cast<FIFFindTargetMemory*>(NodeMemory);
	if (!Memory)
	{
		return;
	}

	// Single handoff point for slot release so we never double-release.
	if (Memory->bHoldingPlayerSlot)
	{
		WaveManager.ReleaseEngagementSlot();
		Memory->bHoldingPlayerSlot = false;
	}

	const bool bRolledPlayer = (FMath::FRand() < PlayerTargetSpawnChance);
	AActor* NewTarget = nullptr;
	bool bReservedSlot = false;

	if (bRolledPlayer && WaveManager.TryReserveEngagementSlot())
	{
		AActor* const PlayerActor = WaveManager.GetPlayerActor();
		AIFBaseCharacter* const PlayerChar = Cast<AIFBaseCharacter>(PlayerActor);
		if (PlayerChar && !PlayerChar->IsDead())
		{
			NewTarget = PlayerActor;
			bReservedSlot = true;
		}
		else
		{
			WaveManager.ReleaseEngagementSlot();
		}
	}

	if (!NewTarget)
	{
		NewTarget = WaveManager.GetStrongholdActor();
	}

	Memory->bHoldingPlayerSlot = bReservedSlot;
	SetBlackboardTarget(OwnerComp, NewTarget);
}

void UBTService_IFFindTarget::ReEvaluateStrongholdTarget(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, AIFWaveManager& WaveManager) const
{
	const UWorld* const World = OwnerComp.GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float LastAttack = WaveManager.GetLastPlayerAttackTime();
	const float LastRevive = WaveManager.GetLastPlayerReviveTime();

	const bool bPlayerAttackedRecently = (LastAttack >= 0.f) && ((Now - LastAttack) <= PlayerAttackRecencyWindowSeconds);
	const bool bPlayerJustRevived = (LastRevive >= 0.f) && ((Now - LastRevive) <= ReviveCheckWindowSeconds);

	if (bPlayerAttackedRecently && TrySwitchToPlayer(OwnerComp, NodeMemory, WaveManager, SwitchToPlayerAfterAttackChance))
	{
		return;
	}

	if (bPlayerJustRevived)
	{
		TrySwitchToPlayer(OwnerComp, NodeMemory, WaveManager, SwitchToPlayerAfterReviveChance);
	}
}

bool UBTService_IFFindTarget::TrySwitchToPlayer(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, AIFWaveManager& WaveManager, float SwitchChance) const
{
	if (FMath::FRand() >= SwitchChance)
	{
		return false;
	}

	FIFFindTargetMemory* const Memory = reinterpret_cast<FIFFindTargetMemory*>(NodeMemory);
	const APawn* const ControlledPawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!Memory || !ControlledPawn)
	{
		return false;
	}

	AActor* const PlayerActor = WaveManager.GetPlayerActor();
	AIFBaseCharacter* const PlayerChar = Cast<AIFBaseCharacter>(PlayerActor);
	if (!PlayerChar || PlayerChar->IsDead())
	{
		return false;
	}

	const float DistToPlayer = FVector::Dist(ControlledPawn->GetActorLocation(), PlayerChar->GetActorLocation());
	if (DistToPlayer > MaxPlayerSwitchDistance || !WaveManager.TryReserveEngagementSlot())
	{
		return false;
	}

	Memory->bHoldingPlayerSlot = true;
	SetBlackboardTarget(OwnerComp, PlayerActor);
	return true;
}

void UBTService_IFFindTarget::SetBlackboardTarget(UBehaviorTreeComponent& OwnerComp, AActor* NewTarget) const
{
	if (UBlackboardComponent* const Blackboard = OwnerComp.GetBlackboardComponent())
	{
		Blackboard->SetValueAsObject(TargetActorKey.SelectedKeyName, NewTarget);
	}
}
