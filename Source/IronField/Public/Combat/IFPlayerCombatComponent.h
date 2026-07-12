#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFPlayerCombatComponent.generated.h"

class UDamageType;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFPlayerCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartSpinAttack();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StopSpinAttack();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void ResetCombatState() override;

	virtual bool CanQueueComboAttack() const override { return !bIsSpinning && Super::CanQueueComboAttack(); }

	virtual float GetCurrentAttackDamage() const override;
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const override;

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

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SpinDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> SpinDamageTypeClass;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|State", meta = (AllowPrivateAccess = "true"))
	bool bIsSpinning = false;

	UFUNCTION()
	void HandleSpinMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void HandleStaminaDepleted();

	void StopSpinGracefully();

	void StopSpinImmediately();
};
