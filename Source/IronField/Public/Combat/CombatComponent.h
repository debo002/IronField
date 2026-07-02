#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimInstance;
class UBoxComponent;
class UDamageType;
class UIFHealthComponent;
class UIFStaminaComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;

/**
 * Actor component managing standard combat logic.
 * Coordinates melee attacks, combo chains, blocking, and hit/block reactions.
 *
 * Owns the weapon-hit-detection lifecycle end to end: it enables the owner's weapon
 * collision volume during the animation's active hit window, listens for its overlap
 * events directly, and resolves each overlap into damage or a block reaction.
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

    /** Transitions combat to the Dead state and clears any in-progress action. Safe to call once per death. */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void HandleOwnerDeath();

    /** Transitions combat back to Idle after the owner has revived. */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void HandleOwnerRevived();

    /** Begins the damage-dealing overlap window for the current weapon attack. Called by the attack's AnimNotifyState. */
    void BeginAttackCollision(float Damage, TSubclassOf<UDamageType> DamageTypeClass);

    /** Ends the damage-dealing overlap window. Called by the attack's AnimNotifyState. */
    void EndAttackCollision();

    /**
     * Receives an incoming melee attack from Instigator and resolves it into a block reaction
     * or damage plus a hit reaction. Called by the attacker's CombatComponent when one of its
     * swings lands on this component's owner — this component is solely responsible for how
     * its own owner reacts to being hit.
     */
    void ReceiveMeleeAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass);

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    ECombatState GetCombatState() const { return CombatState; }

    UFUNCTION(BlueprintPure, Category = "Combat|State")
    bool IsIdle() const { return CombatState == ECombatState::Idle; }

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

    /** Not public: state transitions are driven by the action methods above, or by HandleOwnerDeath/Revived for lifecycle events. */
    void SetCombatState(ECombatState NewState);

    UAnimInstance* GetAnimInstance() const;

    bool HasUsableStamina(float Amount) const;

    void ResetRegisteredAttackHits();

    void RestoreIdleStateUnlessDead();

    /**
     * Whether combo input can currently be queued into the next attack. Base behavior queues
     * whenever a combo-tracked attack montage is active. Overridden by subclasses with attack
     * types that bypass ActiveAttackMontage tracking (e.g. the player's spin attack), so the
     * base class never has to infer subclass state from an unrelated field.
     */
    virtual bool CanQueueComboAttack() const { return ActiveAttackMontage != nullptr; }

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
    float MinimumStaminaToStartBlock = 1.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    ECombatState CombatState = ECombatState::Idle;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    bool bComboQueued = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
    int32 CurrentComboIndex = 0;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> CachedMesh;

    /** The owner's weapon overlap volume. Cached from AIFBaseCharacter; this component drives when it's enabled and listens for its overlaps directly. */
    UPROPERTY(Transient)
    TObjectPtr<UBoxComponent> WeaponCollisionBox;

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

    UFUNCTION()
    void HandleWeaponBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** Resolves a single weapon-overlap against a target: validates it, prevents duplicate hits, and hands off to the target's own reaction handling (or applies damage directly if it has none). */
    void ResolveAttackHit(AActor* TargetActor);

    /** Registers an actor hit during the current attack. Returns false if already hit this swing. */
    bool TryRegisterAttackHit(AActor* TargetActor);

    /** Returns the target's health component if it is a legal, alive attack target this swing; nullptr otherwise. */
    UIFHealthComponent* GetValidAttackTargetHealth(AActor* TargetActor) const;

    bool IsOwnerFacingTarget(AActor* TargetActor) const;
    void ApplyDamageTo(AActor* TargetActor, AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass) const;
    void PlayHitReactionMontage();
    void PlayBlockReactionMontage();
    void SetWeaponCollisionEnabled(bool bEnabled) const;
};