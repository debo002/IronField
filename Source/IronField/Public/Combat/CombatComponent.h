#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimInstance;
class UDamageType;
class UIFStaminaComponent;
class USkeletalMeshComponent;

/**
 * Actor component managing standard combat logic.
 * Coordinates melee attacks, combo chains, blocking, and hit/block reactions.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UIFCombatComponent();

    /** Fires whenever the combat state transitions to a new value. */
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatStateChanged OnCombatStateChanged;

    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StartAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StartBlock();

    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StopBlock();

    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    virtual void ResetCombatState();

    void SetCombatState(ECombatState NewState);

    /** Registers an actor hit during the current attack. Returns false if already hit. */
    bool TryRegisterAttackHit(AActor* TargetActor);

    /** Begins the damage-dealing overlap window for the current weapon attack. */
    void BeginAttackCollision(float Damage, TSubclassOf<UDamageType> DamageTypeClass);

    void EndAttackCollision();

    void HandleWeaponCollisionOverlap(AActor* TargetActor);

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    ECombatState GetCombatState() const { return CombatState; }

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsAttacking() const { return CombatState == ECombatState::Attacking; }

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsBlocking() const { return CombatState == ECombatState::Blocking; }

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsDead() const { return CombatState == ECombatState::Dead; }

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UIFStaminaComponent> StaminaComponent;

    UAnimInstance* GetAnimInstance() const;

    bool HasUsableStamina(float Amount) const;

    void ResetRegisteredAttackHits();

    void RestoreIdleStateUnlessDead();

private:
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> BlockMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> HitReactionMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> BlockReactionMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
    TArray<FIFComboStep> ComboSteps;

    /** Minimum dot product between owner forward and target direction to successfully block. */
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
    float BlockFacingDotThreshold = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
    float BlockStaminaDrainRate = 5.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
    float MinimumStaminaToStartAction = 1.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    ECombatState CombatState = ECombatState::Idle;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    bool bComboQueued = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    int32 CurrentComboIndex = 0;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> CachedMesh;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveAttackMontage;

    UPROPERTY(Transient)
    TSubclassOf<UDamageType> ActiveDamageTypeClass;

    TSet<TWeakObjectPtr<AActor>> RegisteredAttackHits;

    float ActiveAttackDamage = 0.f;
    bool bAttackCollisionActive = false;

    void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    bool CanPlayAttackMontage(int32 ComboIndex) const;
    bool TryPlayBlockMontage();
    bool TryPlayAttackMontage(int32 ComboIndex);
    float GetComboStaminaCost(int32 ComboIndex) const;
    void ClearAttackMontageDelegate();
    bool IsValidAttackOverlap(AActor* TargetActor) const;
    bool IsOwnerFacingTarget(AActor* TargetActor) const;
    void ApplyAttackDamage(AActor* TargetActor);
    void PlayHitReactionMontage();
    void PlayBlockReactionMontage();
    void SetOwnerWeaponCollisionEnabled(bool bEnabled) const;
};

