#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFPlayerCombatComponent.generated.h"

class UDamageType;

UCLASS()
class IRONFIELD_API UIFPlayerCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "IronField|Combat|Actions")
	void StartSpinAttack();

	UFUNCTION(BlueprintCallable, Category = "IronField|Combat|Actions")
	void StopSpinAttack();

	virtual void StartAttack() override;
	virtual void StartBlock() override;
	virtual void StopBlock() override;
	virtual void ResetCombatState() override;
	virtual void ReceiveAttack(AActor* Instigator, float Damage, TSubclassOf<UDamageType> DamageTypeClass) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ClearReactionMontageDelegates() override;
	virtual void HandleHitReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;
	virtual bool CanQueueComboAttack() const override;
	virtual float GetCurrentAttackDamage() const override;
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const override;
	virtual bool RequiresWeaponCollisionBox() const override { return true; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Blocking", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Blocking", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockReactionMontage;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Blocking", meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float BlockFacingDotThreshold = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float BlockStaminaDrainRate = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float MinimumStaminaToStartBlock = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Blocking", meta = (AllowPrivateAccess = "true"))
	float BlockBlendOutTime = 0.15f;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBlockMontage;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> SpinAttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	FName SpinIntroSectionName = TEXT("Intro");

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	FName SpinLoopSectionName = TEXT("Loop");

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	FName SpinEndSectionName = TEXT("End");

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float SpinStaminaDrainRate = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float MinimumStaminaToStartSpin = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Animation", meta = (AllowPrivateAccess = "true"))
	float SpinBlendOutTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SpinDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Combat|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> SpinDamageTypeClass;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Combat|State", meta = (AllowPrivateAccess = "true"))
	bool bIsSpinning = false;

	UFUNCTION()
	void HandleSpinMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleBlockReactionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleStaminaDepleted();

	bool TryPlayBlockMontage();
	void PlayBlockReactionMontage();
	bool IsOwnerFacingTarget(AActor* TargetActor) const;
	void ClearSpinState();
	void StopSpinGracefully();
	void StopSpinImmediately();
};
