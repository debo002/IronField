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

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnComboStepStarted OnComboStepStarted;

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StopBlock();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	virtual void ResetCombatState();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void HandleOwnerDeath();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void HandleOwnerRevived();

	void BeginAttackCollision();

	void EndAttackCollision();

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

	UFUNCTION(BlueprintPure, Category = "Combat|AI")
	float GetComboContinueChance(int32 ComboIndex) const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UIFStaminaComponent> StaminaComponent;

	void SetCombatState(ECombatState NewState);

	UAnimInstance* GetAnimInstance() const;

	bool HasUsableStamina(float Amount) const;

	void ResetRegisteredAttackHits();

	void RestoreIdleStateUnlessDead();

	virtual float GetCurrentAttackDamage() const;

	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const;

	virtual bool CanQueueComboAttack() const { return ActiveAttackMontage != nullptr; }

	virtual bool ShouldReactivelyBlock(bool bFacingAttacker) const { return false; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> HitReactionMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockReactionMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TArray<FIFComboStep> ComboSteps;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float BlockFacingDotThreshold = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float BlockStaminaDrainRate = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float MinimumStaminaToStartBlock = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	float BlockBlendOutTime = 0.15f;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState = ECombatState::Idle;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	bool bComboQueued = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	int32 CurrentComboIndex = 0;

	bool bAttackCollisionActive = false;

	float ActiveAttackDamage = 0.f;
	TSubclassOf<UDamageType> ActiveDamageTypeClass;

	TSet<TWeakObjectPtr<AActor>> RegisteredAttackHits;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh;

	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> WeaponCollisionBox;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBlockMontage;

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

	void ResolveAttackHit(AActor* TargetActor);

	bool TryRegisterAttackHit(AActor* TargetActor);

	UIFHealthComponent* GetValidAttackTargetHealth(AActor* TargetActor) const;

	bool IsOwnerFacingTarget(AActor* TargetActor) const;
	void ApplyDamageTo(AActor* TargetActor, AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass) const;
	void PlayHitReactionMontage();
	void PlayBlockReactionMontage();
	void SetWeaponCollisionEnabled(bool bEnabled) const;
};
