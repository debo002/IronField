#pragma once

#include "CoreMinimal.h"
#include "Components/IFCombatComponent.h"
#include "IFPlayerCombatComponent.generated.h"

/**
 * Specialized combat component for player characters.
 * Integrates customized player-specific abilities like the continuous spin attack.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFPlayerCombatComponent : public UIFCombatComponent
{
    GENERATED_BODY()

public:
    /** Starts a continuous spin attack that drains stamina over time. */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StartSpinAttack();

    /** Requests the spin attack to stop and transition to its end section. */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StopSpinAttack();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void ResetCombatState() override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> SpinAttackMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    FName SpinIntroSectionName = TEXT("Intro");

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    FName SpinLoopSectionName = TEXT("Loop");

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    FName SpinEndSectionName = TEXT("End");

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
    float SpinStaminaDrainRate = 15.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
    float MinimumStaminaToStartSpin = 5.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    float SpinBlendOutTime = 0.1f;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    bool bIsSpinning = false;

    UFUNCTION()
    void OnSpinMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void HandleStaminaDepleted();

    void RequestSpinEnd();

    void AbortSpin();
    void AbortSpinAndRestoreIdle();
};

