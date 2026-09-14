#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IFBaseCharacter.generated.h"

class UAnimInstance;
class UIFCombatComponent;
class UIFHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDied, AIFBaseCharacter*, Character);

UCLASS()
class IRONFIELD_API AIFBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "IronField|Character|Events")
	FOnCharacterDied OnCharacterDied;

	AIFBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "IronField|Character|Components")
	UIFHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "IronField|Character|Components")
	UIFCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "IronField|Character|State")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "IronField|Character|State")
	bool IsAttacking() const;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Character|Animation")
	float DeathMontageBlendOutTime = 0.15f;

	virtual void OnDeathStarted() {}

	UAnimInstance* GetMeshAnimInstance() const;

	/** Restores collision, movement, and the death gate so the character can live (and die) again. */
	void RestoreAliveState();

	void StopMovementForDeath();
	void DisableCollisionForDeath();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Character|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIFHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Character|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIFCombatComponent> CombatComponent;

	UFUNCTION()
	void HandleDeath();

	void BindGameplayDelegates();
	void UnbindGameplayDelegates();

	// Death re-entrancy guard; cleared by RestoreAliveState so a revived character can die again.
	bool bHasDied = false;

	// Live settings captured in BeginPlay and restored verbatim on revive.
	FName CapsuleCollisionProfile;
	FName MeshCollisionProfile;
	bool bSavedOrientRotationToMovement = false;
	bool bSavedUseControllerDesiredRotation = false;
};
