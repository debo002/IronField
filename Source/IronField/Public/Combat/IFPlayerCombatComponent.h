#pragma once

#include "CoreMinimal.h"
#include "Combat/IFCombatComponent.h"
#include "IFPlayerCombatComponent.generated.h"

class UDamageType;

/**
 * Adds the player-only spin attack on top of the base combat component. The spin attack is a
 * "hold to keep spinning" move, so it's handled outside the normal combo system.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class IRONFIELD_API UIFPlayerCombatComponent : public UIFCombatComponent
{
	GENERATED_BODY()

public:
	// Starts spinning if idle and stamina allows. Loops until stopped or stamina runs out.
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartSpinAttack();

	// Ends the spin - finishes the current loop and plays the exit animation rather than stopping instantly.
	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StopSpinAttack();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void ResetCombatState() override;

	// Can't chain a normal combo while spinning.
	virtual bool CanQueueComboAttack() const override { return !bIsSpinning; }

	// Spin isn't part of the combo array, so it needs its own damage value.
	virtual float GetCurrentAttackDamage() const override;
	virtual TSubclassOf<UDamageType> GetCurrentDamageTypeClass() const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> SpinAttackMontage;

	// Section for the spin's entry animation (plays once, at the start).
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	FName SpinIntroSectionName = TEXT("Intro");

	// Section for the spin's loop animation (repeats while spinning continues).
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	FName SpinLoopSectionName = TEXT("Loop");

	// Section for the spin's exit animation (plays once, when spinning stops).
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation", meta = (AllowPrivateAccess = "true"))
	FName SpinEndSectionName = TEXT("End");

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float SpinStaminaDrainRate = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Stamina", meta = (AllowPrivateAccess = "true"))
	float MinimumStaminaToStartSpin = 5.f;

	// Blend-out time when the spin is cut short (e.g. on death).
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

	// Ends the spin naturally via its exit animation (input released or stamina ran out).
	void StopSpinGracefully();

	// Hard-stops the spin with no exit animation (death, EndPlay, full reset).
	void StopSpinImmediately();
};