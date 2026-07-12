#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IFAttackAreaProvider.h"
#include "IFStronghold.generated.h"

class UIFHealthComponent;
class UStaticMeshComponent;
class USphereComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStrongholdDestroyed, AIFStronghold*, Stronghold);

UCLASS()
class IRONFIELD_API AIFStronghold : public AActor, public IIFAttackAreaProvider
{
	GENERATED_BODY()

public:
	AIFStronghold();

	UPROPERTY(BlueprintAssignable, Category = "IronField|Stronghold")
	FOnStrongholdDestroyed OnStrongholdDestroyed;

	virtual float GetAttackAreaRange() const override;
	virtual float GetAttackAreaAcceptanceRadius() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Stronghold")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Stronghold")
	TObjectPtr<USphereComponent> HitboxComponent;

	// Hitbox is at least this many units larger than the mesh sphere bounds.
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold")
	float HitboxMeshPaddingRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold|Targeting")
	float AttackAreaRangeMargin = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold|Targeting")
	float MoveAcceptanceRadiusMargin = -70.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Stronghold")
	TObjectPtr<UIFHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold|Feedback")
	TObjectPtr<UNiagaraSystem> HitVFX;

private:
	UFUNCTION()
	void HandleHealthChanged(float Percent);

	UFUNCTION()
	void HandleDeath();

	void PlayHitFeedback(const FVector& Location);
	void HandleDestruction();
};
