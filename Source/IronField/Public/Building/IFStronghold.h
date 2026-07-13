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

	// AI engagement distances from origin (independent of hitbox radius).
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold|Targeting", meta = (ClampMin = "0.0"))
	float AttackAreaRange = 280.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Stronghold|Targeting", meta = (ClampMin = "0.0"))
	float MoveAcceptanceRadius = 200.f;

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
