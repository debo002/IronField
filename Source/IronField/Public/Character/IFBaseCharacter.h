#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IFBaseCharacter.generated.h"

class UAnimInstance;
class UIFCombatComponent;
class UIFHealthComponent;
class UIFStaminaComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDied, AIFBaseCharacter*, Character);

UCLASS()
class IRONFIELD_API AIFBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Characters|Events")
	FOnCharacterDied OnCharacterDied;

	AIFBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Characters|Components")
	UIFHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Characters|Components")
	UIFStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

	UFUNCTION(BlueprintPure, Category = "Characters|Components")
	UIFCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Characters|State")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Characters|State")
	bool IsBlocking() const;

	UFUNCTION(BlueprintPure, Category = "Characters|State")
	bool IsAttacking() const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Characters|Animation")
	float DeathMontageBlendOutTime = 0.15f;

	virtual void OnDeathStarted() {}

	virtual void OnDeathSequenceStarted() {}

	virtual void OnStaminaDepleted() {}

	UAnimInstance* GetMeshAnimInstance() const;

	void StopMovementForDeath();
	void DisableCollisionForDeath();
	void RestoreCollisionAfterDeath();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIFHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIFStaminaComponent> StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Characters|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIFCombatComponent> CombatComponent;

	UFUNCTION()
	void HandleStaminaDepleted();

	UFUNCTION()
	void HandleDeath();

	void BindGameplayDelegates();
	void UnbindGameplayDelegates();

	// Set to true the moment HandleDeath() begins — single re-entrancy guard owned by the character.
	bool bHasDied = false;

	// Original collision profile names captured in BeginPlay; restored verbatim by RestoreCollisionAfterDeath().
	FName CapsuleCollisionProfile;
	FName MeshCollisionProfile;

	// Original rotation settings captured in BeginPlay; restored by RestoreCollisionAfterDeath().
	bool bSavedOrientRotationToMovement = false;
	bool bSavedUseControllerDesiredRotation = false;
};
