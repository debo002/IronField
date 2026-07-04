#pragma once

#include "CoreMinimal.h"
#include "Character/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "InputActionValue.h"
#include "IFPlayerCharacter.generated.h"

class UAnimMontage;
class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UIFPlayerCombatComponent;

/**
 * Represents the playable character controlled by the user.
 */
UCLASS()
class IRONFIELD_API AIFPlayerCharacter : public AIFBaseCharacter
{
    GENERATED_BODY()

public:
    AIFPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "Player|Movement")
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category = "Player|Movement")
    bool IsBlocking() const;

    UFUNCTION(BlueprintPure, Category = "Player|Health")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, Category = "Player|Health")
    bool IsDead() const;

    UFUNCTION(BlueprintPure, Category = "Player|Stamina")
    float GetStaminaPercent() const;

    UFUNCTION(BlueprintPure, Category = "Player|Combat")
    bool IsAttacking() const;

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

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
    float BlockSpeed = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float SprintSpeed = 620.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float AttackMoveSpeed = 260.f;

    // Dot product threshold to detect if the movement input direction qualifies as "sprinting forward".
    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float SprintInputThreshold = 0.5f;

    // Input magnitude threshold to determine if the player is backpedaling (should be negative).
    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float BackpedalInputThreshold = -0.1f;

    // Squared speed threshold to determine if the player has decelerated enough to stop sprinting.
    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float SprintExitSpeedSquared = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float SprintStaminaDrainRate = 10.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float MinimumStaminaToStartSprint = 1.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
    float NormalCameraArmLength = 400.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
    FVector NormalCameraSocketOffset = FVector(0.f, 45.f, 70.f);

    UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
    float DeathCameraArmLength = 500.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
    FVector DeathCameraSocketOffset = FVector(0.f, 45.f, 110.f);

    // Interpolation speed for smooth camera transitions between states (e.g., alive to death).
    UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
    float CameraTransitionInterpSpeed = 3.f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Animation")
    TObjectPtr<UAnimMontage> GetUpMontage;

    // Time to wait on the ground, dead, before getting back up.
    UPROPERTY(EditDefaultsOnly, Category = "Player|Gameplay")
    float ReviveDelaySeconds = 10.f;

    virtual void OnDeathStarted() override;

    // Called once the death montage finishes - starts the revive countdown.
    virtual void OnDeathMontageFinished() override;

    virtual void OnStaminaDepleted() override;

    // Called once revive fully completes (after the get-up montage finishes).
    void OnReviveFinished();

private:
    UPROPERTY(VisibleAnywhere, Category = "Player|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = "Player|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleInstanceOnly, Category = "Player|Movement", meta = (AllowPrivateAccess = "true"))
    bool bIsSprinting = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Player|Camera", meta = (AllowPrivateAccess = "true"))
    bool bIsCameraTransitioning = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Player|Movement", meta = (AllowPrivateAccess = "true"))
    FVector2D CachedMovementInput = FVector2D::ZeroVector;

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
    void HandleCombatStateChanged(ECombatState PreviousState, ECombatState NewState);

    void UpdateMovementSpeed();
    bool HasSprintInput() const;
    float CalculateDesiredMovementSpeed() const;
    UIFPlayerCombatComponent* GetPlayerCombatComponent() const;

    void TickCameraTransition(float DeltaTime);
    void UpdateTickEnabled();

    UFUNCTION()
    void HandleGetUpMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void ClearReviveTimer();
    void StartReviveTimer();
    void AttemptRevive();
    void CompleteRevive();

    FTimerHandle ReviveTimerHandle;
};