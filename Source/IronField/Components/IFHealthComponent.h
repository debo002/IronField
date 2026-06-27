#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatTypes.h"
#include "Components/ActorComponent.h"
#include "IFHealthComponent.generated.h"

/**
 * Actor component managing character damage, healing, and survival state.
 * Dispatches events upon health changes, death, and invincibility triggers.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UIFHealthComponent();

    UPROPERTY(BlueprintAssignable, Category = "Health|Events")
    FOnHealthDepleted OnHealthDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Health|Events")
    FOnHealthChanged OnHealthChanged;

    UFUNCTION(BlueprintCallable, Category = "Health|Actions")
    void ApplyDamage(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Health|Actions")
    void ApplyHealing(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Health|Actions")
    void Revive();

    /** Enables or disables damage invincibility, used during spawning and revival. */
    UFUNCTION(BlueprintCallable, Category = "Health|Actions")
    void SetInvincible(bool bNewInvincible) { bIsInvincible = bNewInvincible; }

    UFUNCTION(BlueprintPure, Category = "Health|State")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, Category = "Health|State")
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintPure, Category = "Health|State")
    bool IsInvincible() const { return bIsInvincible; }

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Health|Attributes", meta = (AllowPrivateAccess = "true"))
    float MaxHealth = 100.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Health|Attributes", meta = (AllowPrivateAccess = "true"))
    float CurrentHealth = 0.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Health|Attributes", meta = (AllowPrivateAccess = "true"))
    bool bIsInvincible = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health|State", meta = (AllowPrivateAccess = "true"))
    bool bIsDead = false;

    UFUNCTION()
    void HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

    void BroadcastHealthChangedIfNeeded(float PreviousHealth);
};

