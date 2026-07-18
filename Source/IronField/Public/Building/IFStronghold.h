#pragma once

#include "CoreMinimal.h"
#include "Combat/IFPlayerObjective.h"
#include "GameFramework/Actor.h"
#include "IFStronghold.generated.h"

class UIFHealthComponent;
class UStaticMeshComponent;
class USphereComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStrongholdDestroyed, AIFStronghold*, Stronghold);

UCLASS()
class IRONFIELD_API AIFStronghold : public AActor, public IIFPlayerObjective
{
	GENERATED_BODY()

public:
	AIFStronghold();

	virtual bool IsProtectedFromPlayerDamage() const override { return true; }

	UFUNCTION(BlueprintPure, Category = "IronField|Stronghold")
	UIFHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UPROPERTY(BlueprintAssignable, Category = "IronField|Stronghold")
	FOnStrongholdDestroyed OnStrongholdDestroyed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Stronghold")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronField|Stronghold")
	TObjectPtr<USphereComponent> HitboxComponent;

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
