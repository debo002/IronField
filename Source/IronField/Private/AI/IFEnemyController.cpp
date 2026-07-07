#include "AI/IFEnemyController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Combat/IFCombatComponent.h"
#include "Core/IFLog.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Wave/IFWaveManager.h"
#include "Kismet/GameplayStatics.h"
#include "Stats/IFHealthComponent.h"
#include "TimerManager.h"

namespace
{
	static const FName TargetActorKeyName(TEXT("TargetActor"));
}

AIFEnemyController::AIFEnemyController()
{
}

UIFCombatComponent* AIFEnemyController::GetControlledCombatComponent() const
{
	const APawn* const ControlledPawn = GetPawn();
	return ControlledPawn ? ControlledPawn->FindComponentByClass<UIFCombatComponent>() : nullptr;
}

bool AIFEnemyController::IsReadyForNewAttack()
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
		UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s: BehaviorTreeAsset is not set on this controller."), *GetName());
	}

	CachedWaveManager = Cast<AIFWaveManager>(UGameplayStatics::GetActorOfClass(this, AIFWaveManager::StaticClass()));
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
		TargetObserverHandle = BB->RegisterObserver(TargetKeyID, this,
			FOnBlackboardChangeNotification::CreateUObject(this, &AIFEnemyController::HandleTargetBlackboardChanged));

		HandleTargetBlackboardChanged(*BB, TargetKeyID);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s: no BlackboardComponent after RunBehaviorTree."), *GetName());
	}

	BindOwnDelegates();
}

void AIFEnemyController::OnUnPossess()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostPossessionInitTimerHandle);
	}

	if (UBlackboardComponent* const BB = GetBlackboardComponent())
	{
		BB->UnregisterObserver(BB->GetKeyID(TargetActorKeyName), TargetObserverHandle);
	}

	UnbindOwnDelegates();

	Super::OnUnPossess();
}

void AIFEnemyController::BindOwnDelegates()
{
	if (UIFCombatComponent* const Combat = GetControlledCombatComponent())
	{
		Combat->OnCombatStateChanged.AddDynamic(this, &AIFEnemyController::HandleOwnCombatStateChanged);
		Combat->OnComboStepStarted.AddDynamic(this, &AIFEnemyController::HandleComboStepStarted);
	}

	if (const APawn* const ControlledPawn = GetPawn())
	{
		if (UIFHealthComponent* const Health = ControlledPawn->FindComponentByClass<UIFHealthComponent>())
		{
			Health->OnHealthDepleted.AddDynamic(this, &AIFEnemyController::HandleOwnHealthDepleted);
		}
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

EBlackboardNotificationResult AIFEnemyController::HandleTargetBlackboardChanged(const UBlackboardComponent& TargetBlackboard, FBlackboard::FKey KeyID)
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
	if (UBrainComponent* const Brain = GetBrainComponent())
	{
		Brain->StopLogic(TEXT("Enemy died"));
	}

	if (CachedWaveManager)
	{
		if (const UBlackboardComponent* const BB = GetBlackboardComponent())
		{
			AActor* const CurrentTarget = Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName));
			if (CurrentTarget == CachedWaveManager->GetPlayerActor())
			{
				CachedWaveManager->ReleaseEngagementSlot();
			}
		}
	}

	UnbindPlayerBlockingDelegates();
}

void AIFEnemyController::HandlePlayerCombatStateChanged(ECombatState PreviousState, ECombatState NewState)
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
	if (!Combat || !World)
	{
		return;
	}

	Combat->StartBlock();

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

	CachedWaveManager->ReleaseEngagementSlot();

	// Clearing the key aborts active BT tasks before FindTarget re-evaluates on its next tick.
	BB->SetValueAsObject(TargetActorKeyName, nullptr);
}

void AIFEnemyController::StopBlockAfterHold()
{
	if (UIFCombatComponent* const Combat = GetControlledCombatComponent())
	{
		Combat->StopBlock();
	}
}

void AIFEnemyController::ApplyMovementSpeedForState(ECombatState State)
{
	APawn* const ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UCharacterMovementComponent* const Movement = ControlledPawn->FindComponentByClass<UCharacterMovementComponent>();
	if (!Movement)
	{
		return;
	}

	float TargetSpeed = ChaseSpeed;

	switch (State)
	{
	case ECombatState::Attacking:
		TargetSpeed = AttackingSpeed;
		break;

	case ECombatState::Blocking:
		TargetSpeed = BlockingSpeed;
		break;

	case ECombatState::Dead:
		TargetSpeed = 0.f;
		break;

	case ECombatState::Idle:
	default:
		TargetSpeed = ChaseSpeed;
		break;
	}

	Movement->MaxWalkSpeed = TargetSpeed;
}
