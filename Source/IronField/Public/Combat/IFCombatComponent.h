#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatTypes.h"
#include "Components/ActorComponent.h"
#include "IFCombatComponent.generated.h"

class UAnimInstance;
class UBoxComponent;
class UDamageType;
class UIFHealthComponent;
class UIFStaminaComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;

/**
 * Handles combat for a character: attacking, blocking, taking hits, and dying.
 *
 * Works together with:
 *  - The weapon collision box (owned by AIFBaseCharacter) for swing hit detection.
 *  - UIFAnimNotifyStateAttackCollision, which opens/closes the hit window during a swing.
 *  - UIFStaminaComponent, which gates attacks/blocks and drains while blocking.
 *  - UIFHealthComponent, which applies damage via TakeDamage.
 *
 * Shared by player and enemies. UIFPlayerCombatComponent extends this with the player-only
 * spin attack; enemies use this class directly.
 *
 * Hit reactions: every unblocked hit that doesn't kill the owner plays HitReactionMontage.
 * There is no stagger/poise threshold - if you want enemies to "tank" small hits without
 * reacting, do it by not giving them a HitReactionMontage, or by adding damage-type-based
 * filtering later. Keeping it binary avoids hidden threshold math nobody can read at a glance.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIFCombatComponent();

	//~ Begin Events
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatStateChanged OnCombatStateChanged;

	// Fired when a new combo step starts playing (used by enemy AI to roll whether to chain).
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnComboStepStarted OnComboStepStarted;
	//~ End Events

	//~ Begin Actions
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StopBlock();

	// Cancels whatever attack/combo is in progress and returns to Idle (unless dead).
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	virtual void ResetCombatState();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void HandleOwnerDeath();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void HandleOwnerRevived();

	// Called by the attack collision anim notify when the weapon becomes "live" for this swing.
	void BeginAttackCollision();

	// Called by the attack collision anim notify when the swing's hit window ends.
	void EndAttackCollision();

	// Called on the defending character when someone else's swing connects with them.
	void ReceiveMeleeAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass);
	//~ End Actions

	//~ Begin Queries
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

	// AI-facing: chance to chain from the given combo step into the next one. Returns 0 for an
	// out-of-range index, so the chain naturally stops at the last configured step.
	UFUNCTION(BlueprintPure, Category = "Combat|AI")
	float GetComboContinueChance(int32 ComboIndex) const;
	//~ End Queries

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Cached in BeginPlay to avoid repeated FindComponentByClass calls.
	UPROPERTY(Transient)
	TObjectPtr<UIFStaminaComponent> StaminaComponent;

	void SetCombatState(ECombatState NewState);

	UAnimInstance* GetAnimInstance() const;

	bool HasUsableStamina(float Amount) const;

	// Clears the "already hit this swing" list for a new swing.
	void ResetRegisteredAttackHits();

	void RestoreIdleStateUnlessDead();

	// Damage for the current swing. Reads from the combo step; overridden for the spin attack.
	virtual float GetCurrentAttackDamage() const;

	// Damage type for the current swing. See GetCurrentAttackDamage.
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const;

	// Whether the current attack can be extended into a combo (overridden to block this while spinning).
	virtual bool CanQueueComboAttack() const { return ActiveAttackMontage != nullptr; }

private:
	//~ Begin Configured Animation
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitReactionMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockReactionMontage;

	// Each entry is one hit in the combo: its animation, stamina cost, damage, and AI continue chance.
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TArray<FIFComboStep> ComboSteps;
	//~ End Configured Animation

	//~ Begin Configured Blocking/Stamina
	// How closely you must face the attacker to block their hit (1 = facing them directly).
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float BlockFacingDotThreshold = 0.6f;

	// Chance (0–1) to reactively block an incoming hit regardless of current CombatState.
	// Intended for AI-controlled characters — set this in the enemy's combat component Blueprint.
	// Leave at 0.0 for the player so this path is never taken.
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float EnemyBlockChance = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float BlockStaminaDrainRate = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float MinimumStaminaToStartBlock = 10.f;

	// Blend-out time when block is released.
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	float BlockBlendOutTime = 0.15f;
	//~ End Configured Blocking/Stamina

	//~ Begin Runtime State
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState = ECombatState::Idle;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	bool bComboQueued = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	int32 CurrentComboIndex = 0;

	bool bAttackCollisionActive = false;

	// Damage/type for the swing in progress, set in BeginAttackCollision and read when the
	// weapon box overlaps something.
	float ActiveAttackDamage = 0.f;
	TSubclassOf<UDamageType> ActiveDamageTypeClass;

	// Targets already hit this swing (prevents double damage). Weak pointers avoid a dangling
	// reference if a target is destroyed mid-swing.
	TSet<TWeakObjectPtr<AActor>> RegisteredAttackHits;
	//~ End Runtime State

	//~ Begin Cached References
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> WeaponCollisionBox;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;

	// Whichever block-family montage is currently playing - BlockMontage while idly blocking,
	// or BlockReactionMontage while a reaction is interrupting the block pose. Lets StopBlock
	// stop the montage that's actually active instead of assuming it's always BlockMontage.
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBlockMontage;
	//~ End Cached References

	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleBlockReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool CanPlayAttackMontage(int32 ComboIndex) const;
	bool TryPlayBlockMontage();
	bool TryPlayAttackMontage(int32 ComboIndex);
	float GetComboStaminaCost(int32 ComboIndex) const;
	void ClearAttackMontageDelegate();
	void ClearReactionMontageDelegates();

	UFUNCTION()
	void HandleWeaponBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Routes a weapon hit: targets with a combat component react properly (block/hit reaction);
	// others (like the Stronghold) just take damage directly.
	void ResolveAttackHit(AActor* TargetActor);

	// True and remembers the target the first time it's hit this swing; false otherwise.
	bool TryRegisterAttackHit(AActor* TargetActor);

	UIFHealthComponent* GetValidAttackTargetHealth(AActor* TargetActor) const;

	bool IsOwnerFacingTarget(AActor* TargetActor) const;
	void ApplyDamageTo(AActor* TargetActor, AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass) const;
	void PlayHitReactionMontage();
	void PlayBlockReactionMontage();
	void SetWeaponCollisionEnabled(bool bEnabled) const;
};