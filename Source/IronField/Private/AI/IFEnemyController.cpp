#include "AI/IFEnemyController.h"

#include "AI/IFEnemyAIData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Character/IFEnemyCharacter.h"
#include "Combat/IFCombatComponent.h"
#include "Core/IFLog.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Stats/IFHealthComponent.h"
#include "Wave/IFWaveManager.h"

const UIFEnemyAIData* AIFEnemyController::GetAIData() const
{
	return AIData ? AIData.Get() : GetDefault<UIFEnemyAIData>();
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
	if (!AIData)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-AI] %s has no AIData assigned — using UIFEnemyAIData CDO defaults."), *GetName());
	}

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
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

	BindOwnDelegates();
}

void AIFEnemyController::OnUnPossess()
{
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
	}

	if (const APawn* const ControlledPawn = GetPawn())
	{
		if (UIFHealthComponent* const Health = ControlledPawn->FindComponentByClass<UIFHealthComponent>())
		{
			Health->OnHealthDepleted.RemoveAll(this);
		}
	}

	if (CachedWaveManager)
	{
		CachedWaveManager->OnPlayerDied.RemoveAll(this);
	}
}

void AIFEnemyController::HandleOwnCombatStateChanged(ECombatState PreviousState, ECombatState NewState)
{
	if (PreviousState == ECombatState::Attacking && NewState == ECombatState::Idle)
	{
		if (const UWorld* const World = GetWorld())
		{
			LastAttackEndedTime = World->GetTimeSeconds();
			const UIFEnemyAIData* const Data = GetAIData();
			CurrentReattackCooldownSeconds = FMath::RandRange(Data->MinReattackCooldownSeconds, Data->MaxReattackCooldownSeconds);
		}
	}

	ApplyMovementSpeedForState(NewState);
}

void AIFEnemyController::HandleOwnHealthDepleted()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	ClearFocus(EAIFocusPriority::Default);

	if (UBrainComponent* const Brain = GetBrainComponent())
	{
		Brain->StopLogic(TEXT("Enemy died"));
	}

	if (APawn* const ControlledPawn = GetPawn())
	{
		ControlledPawn->DetachFromControllerPendingDestroy();
	}
}

void AIFEnemyController::HandlePlayerDied()
{
	UBlackboardComponent* const BB = GetBlackboardComponent();
	if (!BB || !CachedWaveManager)
	{
		return;
	}

	if (Cast<AActor>(BB->GetValueAsObject(TargetActorKeyName)) == CachedWaveManager->GetPlayerActor())
	{
		BB->SetValueAsObject(TargetActorKeyName, nullptr);
	}
}

void AIFEnemyController::ApplyMovementSpeedForState(ECombatState State)
{
	if (AIFEnemyCharacter* const EnemyChar = Cast<AIFEnemyCharacter>(GetPawn()))
	{
		EnemyChar->ApplyMovementSpeedForState(State);
	}
}
