#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IFStronghold.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UBoxComponent;
class UIFHealthComponent;
class USoundBase;
class UNiagaraSystem;
class AIFStrongholdAttackPoint;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStrongholdDestroyed, AIFStronghold*, Stronghold);

/**
 * Represents a stronghold building that can be captured or defended.
 */
UCLASS()
class IRONFIELD_API AIFStronghold : public AActor
{
	GENERATED_BODY()

public:
	AIFStronghold();

	UFUNCTION(BlueprintPure, Category = "Stronghold|Components")
	UIFHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UPROPERTY(BlueprintAssignable, Category = "Stronghold")
	FOnStrongholdDestroyed OnStrongholdDestroyed;

	// Called by enemy AI to claim the closest unoccupied attack point. Returns nullptr if
	// none are free or none are configured.
	UFUNCTION(BlueprintCallable, Category = "Stronghold|AttackPoints")
	AIFStrongholdAttackPoint* ReserveNearestFreeAttackPoint(const FVector& FromLocation);

	// Called when an enemy stops targeting the Stronghold (retargeted, died, etc).
	UFUNCTION(BlueprintCallable, Category = "Stronghold|AttackPoints")
	void ReleaseAttackPoint(AIFStrongholdAttackPoint* Point);

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// Dedicated hit volume weapon swings overlap against. The visual mesh may be large,
	// composite, or have collision that doesn't reach every attack point, so combat hit
	// detection is decoupled from it - resize/reposition this in the editor to cover
	// wherever enemies are actually meant to land hits from.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold|Components")
	TObjectPtr<UBoxComponent> HitboxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold|Components")
	TObjectPtr<UIFHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Stronghold|Feedback")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "Stronghold|Feedback")
	TObjectPtr<UNiagaraSystem> HitVFX;

	// Designer-placed points around the Stronghold enemies path to and attack from.
	UPROPERTY(EditInstanceOnly, Category = "Stronghold|AttackPoints")
	TArray<TObjectPtr<AIFStrongholdAttackPoint>> AttackPoints;

private:
	UFUNCTION()
	void HandleHealthChanged(float Percent);

	UFUNCTION()
	void HandleDeath();

	void PlayHitFeedback(const FVector& Location);
	void HandleDestruction();
};