#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "IFBaseCharacter.generated.h"

class UBoxComponent;
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

	UFUNCTION(BlueprintPure, Category = "Characters|Components")
	UBoxComponent* GetWeaponCollisionBox() const { return WeaponCollisionBox; }

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

	UPROPERTY(EditDefaultsOnly, Category = "Characters|Combat")
	FVector WeaponCollisionExtent = FVector(20.f, 60.f, 20.f);

	virtual void OnDeathStarted() {}

	virtual void OnDeathMontageFinished() {}

	virtual void OnStaminaDepleted() {}

	UAnimInstance* GetMeshAnimInstance() const;

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

	void BindGameplayDelegates();
	void UnbindGameplayDelegates();

	void StopMovementForDeath();
	void DisableCollisionForDeath();
};
