#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IFBaseCharacter.generated.h"

class UAnimMontage;
class UBoxComponent;
class UPrimitiveComponent;
class UIFCombatComponent;
class UIFHealthComponent;
class UIFStaminaComponent;

/**
 * Base class for characters in the game.
 * Integrates health, stamina, and combat components, and manages death/revive lifecycles.
 */
UCLASS()
class IRONFIELD_API AIFBaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AIFBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "Characters|Components")
    UIFHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category = "Characters|Components")
    UIFStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

    UFUNCTION(BlueprintPure, Category = "Characters|Components")
    UIFCombatComponent* GetCombatComponent() const { return CombatComponent; }

    /** Enables or disables the weapon's overlap collision detection. */
    void SetWeaponCollisionEnabled(bool bEnabled);

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void Landed(const FHitResult& Hit) override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Characters|Animation")
    TObjectPtr<UAnimMontage> DeathMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Characters|Animation")
    TObjectPtr<UAnimMontage> GetUpMontage;

    /** Time in seconds to wait after death before initiating an automatic revive. */
    UPROPERTY(EditDefaultsOnly, Category = "Characters|Gameplay")
    float ReviveDelaySeconds = 10.f;

    virtual void OnDeathStarted() {}

    virtual void OnStaminaDepleted() {}

    virtual void OnReviveFinished() {}

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UIFHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UIFStaminaComponent> StaminaComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UIFCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> WeaponCollisionBox;

    UFUNCTION()
    void HandleStaminaDepleted();

    UFUNCTION()
    void HandleDeath();

    UFUNCTION()
    void HandleDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void HandleGetUpMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void HandleWeaponCollisionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void BindGameplayDelegates();
    void UnbindGameplayDelegates();
    void ClearLifecycleMontageDelegates() const;
    void ClearReviveTimer();
    bool PlayMontageAndBindEnd(UAnimMontage* Montage, void (AIFBaseCharacter::*EndHandler)(UAnimMontage*, bool));
    void StartReviveTimer();
    void AttemptRevive();
    void CompleteRevive();
    UAnimInstance* GetMeshAnimInstance() const;

    FTimerHandle ReviveTimerHandle;
};

