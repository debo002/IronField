#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatTypes.h"
#include "Components/ActorComponent.h"
#include "IFStaminaComponent.generated.h"

/**
 * Actor component managing character stamina mechanics.
 * Supports continuous stamina drain operations and time-delayed automatic regeneration.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UIFStaminaComponent();

    UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
    FOnStaminaDepleted OnStaminaDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
    FOnStaminaChanged OnStaminaChanged;

    /** Attempts to consume a flat amount of stamina. Returns false if insufficient. */
    bool TryConsumeStamina(float Amount);

    /** Initiates a continuous stamina drain at the specified rate per second. */
    void StartContinuousDrain(float DrainRate);

    void StopContinuousDrain();

    float GetStaminaPercent() const;

    bool HasStamina(float MinimumAmount) const { return CurrentStamina >= MinimumAmount; }

    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float MaxStamina = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float StaminaRegenRate = 20.f;

    /** Seconds after the last drain event before regeneration resumes. */
    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float StaminaRegenDelay = 3.5f;

    UPROPERTY(VisibleInstanceOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float CurrentStamina = 0.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float TimeSinceLastStaminaDrain = 0.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float ContinuousDrainRate = 0.f;

    bool IsDrainingStamina() const;
    bool CanRegenerateStamina() const;
    void DrainStamina(float DeltaTime);
    void RegenerateStamina(float DeltaTime);
    void UpdateTickForRegen();
    void EnableStaminaTick();
    void DisableStaminaTick();
    void BroadcastStaminaChangedIfNeeded(float PreviousStamina);
    void BroadcastStaminaDepletedIfNeeded(float PreviousStamina);
};

