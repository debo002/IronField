#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IFBaseCharacter.generated.h"

class UAnimMontage;
class UBoxComponent;
class UIFCombatComponent;
class UIFHealthComponent;
class UIFStaminaComponent;

/**
 * Base class for all characters (player and enemies). Handles the shared stuff: health, stamina,
 * combat, the weapon hitbox, and death (play death montage, stop, done). Does not know about
 * reviving - that's player-only behavior, added on top in AIFPlayerCharacter.
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

	UFUNCTION(BlueprintPure, Category = "Characters|Components")
	UBoxComponent* GetWeaponCollisionBox() const { return WeaponCollisionBox; }

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Landed(const FHitResult& Hit) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Characters|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// Blend-out time for whatever montage was playing when death happens.
	UPROPERTY(EditDefaultsOnly, Category = "Characters|Animation")
	float DeathMontageBlendOutTime = 0.15f;

	// Size of the weapon hitbox, attached to the mesh's weapon socket.
	UPROPERTY(EditDefaultsOnly, Category = "Characters|Combat")
	FVector WeaponCollisionExtent = FVector(20.f, 60.f, 20.f);

	// Called when death starts, before the death montage plays.
	virtual void OnDeathStarted() {}

	// Called when the death montage finishes. Base does nothing here - override this to react
	// to death being "complete" (for example the player starts its revive timer from here).
	virtual void OnDeathMontageFinished() {}

	// Called when stamina hits 0.
	virtual void OnStaminaDepleted() {}

	UAnimInstance* GetMeshAnimInstance() const;
	void ClearLifecycleMontageDelegates() const;

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

	void BindGameplayDelegates();
	void UnbindGameplayDelegates();
};