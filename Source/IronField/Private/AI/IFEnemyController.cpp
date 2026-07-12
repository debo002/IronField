#include "AI/IFEnemyController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Character/IFEnemyCharacter.h"
#include "Combat/IFCombatComponent.h"
#include "Core/IFLog.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Stats/IFHealthComponent.h"
#include "TimerManager.h"
#include "Wave/IFWaveManager.h"

namespace
{
	static const FName TargetActorKeyName(TEXT("TargetActor"));
}

AIFEnemyController::AIFEnemyController()
{
}

UIFCombatComponent* AIFEnemyController::GetControlledCombatComponent() const
{
	return CachedCombatComponent;
}

bool AIFEnemyController::IsReadyForNewAttack() const
{
	if (LastAttackEndedTime < 0.f)
	{
		return true;
	}

	const UWorld* const World = GetWorld();
	if (!World)
	{
		return true;
	}

	return (World->GetTimeSeconds() - LastAttackEndedTime) >= CurrentReattackCooldownSeconds;
}

void AIFEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &AIFEnemyController::InitializeAfterPossession);
	}
}

void AIFEnemyController::InitializeAfterPossession()
{
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s: BehaviorTreeAsset is not set."), *GetName());
	}

	if (const UWorld* const World = GetWorld())
	{
		if (const UIFWaveManagerSubsystem* const Subsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
		{
			CachedWaveManager = Subsystem->GetWaveManager();
		}
	}

	if (CachedWaveManager)
	{
		CachedWaveManager->OnPlayerDied.AddDynamic(this, &AIFEnemyController::HandlePlayerDied);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s: no AIFWaveManager found in the level."), *GetName());
	}

	if (UBlackboardComponent* const BB = GetBlackboardComponent())
	{
		const FBlackboard::FKey TargetKeyID = BB->GetKeyID(TargetActorKeyName);
		if (TargetKeyID == FBlackboard::InvalidKey)
		{
			UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s: Blackboard has no key named 'TargetActor'."), *GetName());
		}
		else
		{
			TargetObserverHandle = BB->RegisterObserver(
				TargetKeyID,
				this,
				FOnBlackboardChangeNotification::CreateUObject(this, &AIFEnemyController::HandleTargetBlackboardChanged));

			HandleTargetBlackboardChanged(*BB, TargetKeyID);
		}
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s: no BlackboardComponent after RunBehaviorTree."), *GetName());
	}

	BindOwnDelegates();
}

void AIFEnemyController::OnUnPossess()
{
	ClearBlockHoldTimer();

	if (UBlackboardComponent* const BB = GetBlackboardComponent())
	{
		if (TargetObserverHandle.IsValid())
		{
			const FBlackboard::FKey TargetKeyID = BB->GetKeyID(TargetActorKeyName);
			if (TargetKeyID != FBlackboard::InvalidKey)
			{
				BB->UnregisterObserver(TargetKeyID, TargetObserverHandle);
			}
			TargetObserverHandle.Reset();
		}
	}

	UnbindOwnDelegates();
	CachedCombatComponent = nullptr;
	CachedWaveManager = nullptr;

	Super::OnUnPossess();
}

void AIFEnemyController::BindOwnDelegates()
{
	const APawn* const ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	CachedCombatComponent = ControlledPawn->FindComponentByClass<UIFCombatComponent>();
	if (UIFCombatComponent* const Combat = CachedCombatComponent)
	{
		Combat->OnCombatStateChanged.AddDynamic(this, &AIFEnemyController::HandleOwnCombatStateChanged);
		Combat->OnComboStepStarted.AddDynamic(this, &AIFEnemyController::HandleComboStepStarted);
	}

	if (UIFHealthComponent* const Health = ControlledPawn->FindComponentByClass<UIFHealthComponent>())
	{
		Health->OnHealthDepleted.AddDynamic(this, &AIFEnemyController::HandleOwnHealthDepleted);
	}
}

void AIFEnemyController::UnbindOwnDelegates()
{
	if (UIFCombatComponent* const Combat = GetControlledCombatComponent())
	{
		Combat->OnCombatStateChanged.RemoveAll(this);
		Combat->OnComboStepStarted.RemoveAll(this);
	}

	if (const APawn* const ControlledPawn = GetPawn())
	{
		if (UIFHealthComponent* const Health = ControlledPawn->FindComponentByClass<UIFHealthComponent>())
		{
			Health->OnHealthDepleted.RemoveAll(this);
		}
	}

	UnbindPlayerBlockingDelegates();

	if (CachedWaveManager)
	{
		CachedWaveManager->OnPlayerDied.RemoveAll(this);
	}
}

void AIFEnemyController::BindPlayerBlockingDelegates()
{
	AActor* const PlayerActor = CachedWaveManager ? CachedWaveManager->GetPlayerActor() : nullptr;
	if (UIFCombatComponent* const PlayerCombat = PlayerActor ? PlayerActor->FindComponentByClass<UIFCombatComponent>() : nullptr)
	{
		PlayerCombat->OnCombatStateChanged.AddUniqueDynamic(this, &AIFEnemyController::HandlePlayerCombatStateChanged);
	}
}

void AIFEnemyController::UnbindPlayerBlockingDelegates()
{
	AActor* const PlayerActor = CachedWaveManager ? CachedWaveManager->GetPlayerActor() : nullptr;
	if (UIFCombatComponent* const PlayerCombat = PlayerActor ? PlayerActor->FindComponentByClass<UIFCombatComponent>() : nullptr)
	{
		PlayerCombat->OnCombatStateChanged.RemoveDynamic(this, &AIFEnemyController::HandlePlayerCombatStateChanged);
	}
}

EBlackboardNotificationResult AIFEnemyController::HandleTargetBlackboardChanged(const UBlackboardComponent& TargetBlackboard, FBlackboard::FKey)
{
	AActor* const NewTarget = Cast<AActor>(TargetBlackboard.GetValueAsObject(TargetActorKeyName));
	const bool bTargetingPlayer = NewTarget && CachedWaveManager && NewTarget == CachedWaveManager->GetPlayerActor();

	if (bTargetingPlayer)
	{
		BindPlayerBlockingDelegates();
	}
	else
	{
		UnbindPlayerBlockingDelegates();
	}

	return EBlackboardNotificationResult::ContinueObserving;
}

void AIFEnemyController::HandleOwnCombatStateChanged(ECombatState PreviousState, ECombatState NewState)
{
	if (PreviousState == ECombatState::Attacking && NewState == ECombatState::Idle)
	{
		if (const UWorld* const World = GetWorld())
		{
			LastAttackEndedTime = World->GetTimeSeconds();
			CurrentReattackCooldownSeconds = FMath::RandRange(MinReattackCooldownSeconds, MaxReattackCooldownSeconds);
		}
	}

	ApplyMovementSpeedForState(NewState);
}

void AIFEnemyController::HandleComboStepStarted(int32 ComboIndex)
{
	UIFCombatComponent* const Combat = GetControlledCombatComponent();
	if (!Combat)
	{
		return;
	}

	if (FMath::FRand() >= Combat->GetComboContinueChance(ComboIndex))
	{
		return;
	}

	Combat->StartAttack();
}

void AIFEnemyController::HandleOwnHealthDepleted()
{
	ClearBlockHoldTimer();

	if (UBrainComponent* const Brain = GetBrainComponent())
	{
		// StopLogic fires OnCeaseRelevant on BT services, which release engagement slots.
		Brain->StopLogic(TEXT("Enemy died"));
	}

	UnbindPlayerBlockingDelegates();
}

void AIFEnemyController::HandlePlayerCombatStateChanged(ECombatState, ECombatState NewState)
{
	if (NewState != ECombatState::Attacking)
	{
		return;
	}

	if (FMath::FRand() >= BlockReactionChance)
	{
		return;
	}

	UIFCombatComponent* const Combat = GetControlledCombatComponent();
	UWorld* const World = GetWorld();
	if (!Combat || !World || !Combat->IsIdle())
	{
		return;
	}

	Combat->StartBlock();
	if (!Combat->IsBlocking())
	{
		return;
	}

	// Replace any in-flight hold so rapid player attacks cannot stack stop timers.
	ClearBlockHoldTimer();
	const float HoldSeconds = FMath::RandRange(MinBlockHoldSeconds, MaxBlockHoldSeconds);
	World->GetTimerManager().SetTimer(BlockHoldTimerHandle, this, &AIFEnemyController::StopBlockAfterHold, HoldSeconds, false);
}

void AIFEnemyController::HandlePlayerDied()
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!BB || !CachedWaveManager)
	{
		return;
	}

	AActor* const CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName));
	if (CurrentTarget != CachedWaveManager->GetPlayerActor())
	{
		return;
	}

	// Do not release the engagement slot here. PickInitialTarget owns that so we never double-release.
	// Clearing the key makes the service pick a new target on its next tick.
	BB->SetValueAsObject(TargetActorKeyName, nullptr);
}

void AIFEnemyController::ClearBlockHoldTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlockHoldTimerHandle);
	}
}

void AIFEnemyController::StopBlockAfterHold()
{
	UIFCombatComponent* const Combat = GetControlledCombatComponent();
	if (Combat && Combat->IsBlocking())
	{
		Combat->StopBlock();
	}
}

void AIFEnemyController::ApplyMovementSpeedForState(ECombatState State)
{
	if (AIFEnemyCharacter* const EnemyChar = Cast<AIFEnemyCharacter>(GetPawn()))
	{
		EnemyChar->ApplyMovementSpeedForState(State);
	}
}
