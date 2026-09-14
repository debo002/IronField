#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "InputActionValue.h"
#include "IFPlayerCharacter.generated.h"

class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UIFPlayerCombatComponent;
class UIFStaminaComponent;

UCLASS()
class IRONFIELD_API AIFPlayerCharacter : public AIFBaseCharacter
{
	GENERATED_BODY()

public:
	AIFPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "IronField|Player|Movement")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "IronField|Player|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "IronField|Player|Stamina")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "IronField|Player|Components")
	UIFStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

	UFUNCTION(BlueprintPure, Category = "IronField|Player|State")
	bool IsGettingUp() const { return bIsGettingUp; }

	UFUNCTION(BlueprintCallable, Category = "IronField|Player|State")
	void NotifyGetUpFinished();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	/** Priority for DefaultInputMappingContext; higher wins when several contexts are active. */
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input", meta = (ClampMin = "0"))
	int32 DefaultInputMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> MoveInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> LookInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> JumpInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> SprintInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> BlockInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> AttackInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Input")
	TObjectPtr<UInputAction> SpinAttackInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float WalkSpeed = 375.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float BackpedalSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float BlockingSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float SprintSpeed = 620.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float AttackMoveSpeed = 260.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float SprintInputThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float BackpedalInputThreshold = -0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Movement")
	float SprintExitSpeedSquared = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Stamina")
	float SprintStaminaDrainRate = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Stamina")
	float MinimumStaminaToStartSprint = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	float NormalCameraArmLength = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	FVector NormalCameraSocketOffset = FVector(0.f, 0.f, 80.f);

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	float DeathCameraArmLength = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	FVector DeathCameraSocketOffset = FVector(0.f, 0.f, 120.f);

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	float CameraTransitionInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	float CameraBoomPitch = -52.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	float CameraLagSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Camera")
	float CameraRotationLagSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Gameplay")
	float ReviveDelaySeconds = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Player|Gameplay")
	float GetUpDuration = 2.5f;

	virtual void OnDeathStarted() override;
	void OnReviveFinished();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Player|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Player|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Player|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIFStaminaComponent> StaminaComponent;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Player|Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Player|Camera", meta = (AllowPrivateAccess = "true"))
	bool bIsCameraTransitioning = false;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Player|Movement", meta = (AllowPrivateAccess = "true"))
	FVector2D CachedMovementInput = FVector2D::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "IronField|Player|State", meta = (AllowPrivateAccess = "true"))
	bool bIsGettingUp = false;

	void Move(const FInputActionValue& Value);
	void StopMoving();
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void Attack();
	void StartBlock();
	void StopBlock();
	void StartSpinAttack();
	void StopSpinAttack();

	UFUNCTION()
	void HandleStaminaDepleted();

	// Shared gate for attack/block/spin: rejects dead/get-up and cancels sprint first.
	bool TryPrepareCombatAction();

	UFUNCTION()
	void HandleCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

	void ApplyCameraDefaults();
	void UpdateMovementSpeed();
	bool HasSprintInput() const;
	float CalculateDesiredMovementSpeed() const;
	UIFPlayerCombatComponent* GetPlayerCombatComponent() const;

	void TickCameraTransition(float DeltaTime);
	void UpdateTickEnabled();

	void ClearReviveTimers();
	void StartReviveTimer();
	void AttemptRevive();
	void CompleteRevive();

	FTimerHandle ReviveTimerHandle;
	FTimerHandle GetUpFallbackTimerHandle;
};
