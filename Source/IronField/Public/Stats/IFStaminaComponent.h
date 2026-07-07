#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatTypes.h"
#include "Components/ActorComponent.h"
#include "IFStaminaComponent.generated.h"

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

    UFUNCTION(BlueprintCallable, Category = "Stamina|Actions")
    bool TryConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Stamina|Actions")
    void StartContinuousDrain(float DrainRate);

    UFUNCTION(BlueprintCallable, Category = "Stamina|Actions")
    void StopContinuousDrain();

    UFUNCTION(BlueprintPure, Category = "Stamina|State")
    float GetStaminaPercent() const;

    UFUNCTION(BlueprintPure, Category = "Stamina|State")
    bool HasStamina(float MinimumAmount) const { return CurrentStamina >= MinimumAmount; }

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float MaxStamina = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Stamina|Attributes", meta = (AllowPrivateAccess = "true"))
    float StaminaRegenRate = 20.f;

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

    void SetStaminaClamped(float NewStamina);
    void BroadcastStaminaChangedIfNeeded(float PreviousStamina);
    void BroadcastStaminaDepletedIfNeeded(float PreviousStamina);
};
