#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Stronghold.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UIFHealthComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStrongholdDestroyed, AIFStronghold*, Stronghold);

UCLASS()
class IRONFIELD_API AIFStronghold : public AActor
{
	GENERATED_BODY()

public:
	AIFStronghold();

	/** Returns the health component for external queries (e.g. WaveManager, UI). */
	UFUNCTION(BlueprintPure, Category = "Stronghold|Components")
	UIFHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UPROPERTY(BlueprintAssignable, Category = "Stronghold")
	FOnStrongholdDestroyed OnStrongholdDestroyed;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold|Components")
	TObjectPtr<UIFHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Stronghold|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "Stronghold|Feedback")
	TObjectPtr<UNiagaraSystem> HitVFX;

private:
	UFUNCTION()
	void HandleHealthChanged(float Percent);

	UFUNCTION()
	void HandleDeath();

	void PlayHitFeedback(const FVector& Location);
	void HandleDestruction();
};