#pragma once

#include "CoreMinimal.h"
#include "Characters/IFBaseCharacter.h"
#include "Combat/IFCombatTypes.h"
#include "InputActionValue.h"
#include "IFPlayerCharacter.generated.h"

class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UIFPlayerCombatComponent;

/**
 * Main character class controlled by the player.
 * Handles local input bindings, movement speed transitions, and stamina-draining actions.
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

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float SprintInputThreshold = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Player|Movement")
    float BackpedalInputThreshold = -0.1f;

    /** Stamina consumed per second while sprinting. */
    UPROPERTY(EditDefaultsOnly, Category = "Player|Stamina")
    float SprintStaminaDrainRate = 10.f;

    /** Minimum stamina required to initiate a sprint. */
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

    /** Speed at which the camera transitions to/from the death view. */
    UPROPERTY(EditDefaultsOnly, Category = "Player|Camera")
    float CameraTransitionInterpSpeed = 3.f;

    virtual void OnDeathStarted() override;

    virtual void OnStaminaDepleted() override;

    virtual void OnReviveFinished() override;

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
};

