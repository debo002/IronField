#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "Combat/IFWeaponBoxOwner.h"
#include "InputActionValue.h"
#include "IFPlayerCharacter.generated.h"

class UCameraComponent;
class UBoxComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
class UIFPlayerCombatComponent;

UCLASS()
class IRONFIELD_API AIFPlayerCharacter : public AIFBaseCharacter, public IIFWeaponBoxOwner
{
	GENERATED_BODY()

public:
	AIFPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Player|Components")
	virtual UBoxComponent* GetWeaponCollisionBox() const override { return WeaponCollisionBox; }

	UFUNCTION(BlueprintPure, Category = "Player|Movement")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Player|Stamina")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "Player|State")
	bool IsGettingUp() const { return bIsGettingUp; }

	UFUNCTION(BlueprintCallable, Category = "Player|State")
	void NotifyGetUpFinished();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> MoveInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> LookInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> JumpInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> SprintInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> BlockInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> AttackInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> SpinAttackInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float WalkSpeed = 375.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float BackpedalSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float BlockingSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float SprintSpeed = 620.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float AttackMoveSpeed = 260.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float SprintInputThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float BackpedalInputThreshold = -0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
	float SprintExitSpeedSquared = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
	float SprintStaminaDrainRate = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
	float MinimumStaminaToStartSprint = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	float NormalCameraArmLength = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	FVector NormalCameraSocketOffset = FVector(0.f, 0.f, 80.f);

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	float DeathCameraArmLength = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	FVector DeathCameraSocketOffset = FVector(0.f, 0.f, 120.f);

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	float CameraTransitionInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	float CameraBoomPitch = -52.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	float CameraLagSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
	float CameraRotationLagSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Gameplay")
	float ReviveDelaySeconds = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Gameplay")
	float GetUpDuration = 2.5f;

	virtual void OnDeathStarted() override;
	virtual void OnDeathSequenceStarted() override;
	virtual void OnStaminaDepleted() override;

	void OnReviveFinished();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> WeaponCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SwordMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	UPROPERTY(VisibleInstanceOnly, Category = "Player|Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Player|Camera", meta = (AllowPrivateAccess = "true"))
	bool bIsCameraTransitioning = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Player|Movement", meta = (AllowPrivateAccess = "true"))
	FVector2D CachedMovementInput = FVector2D::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|State", meta = (AllowPrivateAccess = "true"))
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

	// Shared gate for attack/block/spin: rejects dead/get-up and cancels sprint first.
	bool TryPrepareCombatAction();

	UFUNCTION()
	void HandleCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

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
