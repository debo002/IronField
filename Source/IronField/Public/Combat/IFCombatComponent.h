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

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIFCombatComponent();

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatStateChanged OnCombatStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	virtual void StartAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	virtual void StartBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	virtual void StopBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	virtual void ResetCombatState();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void HandleOwnerDeath();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void HandleOwnerRevived();

	virtual void BeginAttackCollision();
	virtual void EndAttackCollision();

	/** Shared hit entry for melee weapon boxes and ranged projectiles (block / damage / hit-reaction). */
	void ReceiveAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass);

	void SetAttackTarget(AActor* InTarget) { CachedAttackTarget = InTarget; }
	AActor* GetAttackTarget() const { return CachedAttackTarget.Get(); }

	virtual void LaunchProjectileAttack() {}

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

	/** AI melee only; base always returns 0 so player combo never auto-continues. */
	virtual float GetComboContinueChance(int32 ComboIndex) const { return 0.f; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UIFStaminaComponent> StaminaComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	TWeakObjectPtr<AActor> CachedAttackTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Combo")
	TArray<FIFComboStep> ComboSteps;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State")
	bool bComboQueued = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State")
	int32 CurrentComboIndex = 0;

	void SetCombatState(ECombatState NewState);
	UAnimInstance* GetAnimInstance() const;
	bool HasUsableStamina(float Amount) const;
	void ResetRegisteredAttackHits();
	void RestoreIdleStateUnlessDead();
	void ClearAttackMontageDelegate();
	void ClearReactionMontageDelegates();

	virtual float GetCurrentAttackDamage() const;
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const;
	virtual bool CanQueueComboAttack() const;
	virtual bool ShouldReactivelyBlock(bool bFacingAttacker) const { return false; }
	virtual void PlayHitReactionMontage();

	bool CanPlayAttackMontage(int32 ComboIndex) const;
	bool TryPlayAttackMontage(int32 ComboIndex);
	float GetComboStaminaCost(int32 ComboIndex) const;
	bool HasNextComboStep() const { return ComboSteps.IsValidIndex(CurrentComboIndex + 1); }

	UFUNCTION()
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitReactionMontage;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockReactionMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float BlockFacingDotThreshold = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float BlockStaminaDrainRate = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float MinimumStaminaToStartBlock = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true"))
	float BlockBlendOutTime = 0.15f;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState = ECombatState::Idle;

	bool bAttackCollisionActive = false;
	float ActiveAttackDamage = 0.f;
	TSubclassOf<UDamageType> ActiveDamageTypeClass;
	TSet<TWeakObjectPtr<AActor>> RegisteredAttackHits;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> WeaponCollisionBox;

	UFUNCTION()
	void HandleHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleBlockReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool TryPlayBlockMontage();

	UFUNCTION()
	void HandleWeaponBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ResolveAttackHit(AActor* TargetActor);
	bool TryRegisterAttackHit(AActor* TargetActor);
	/** Health target only if weapon collision is active and target is legally hittable. */
	UIFHealthComponent* GetValidActiveAttackTargetHealth(AActor* TargetActor) const;
	bool IsOwnerFacingTarget(AActor* TargetActor) const;
	void PlayBlockReactionMontage();
	void SetWeaponCollisionEnabled(bool bEnabled) const;
};
