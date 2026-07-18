#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IFProjectile.generated.h"

class UDamageType;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class IRONFIELD_API AIFProjectile : public AActor
{
	GENERATED_BODY()

public:
	AIFProjectile();

	void InitializeProjectile(AActor* InInstigator, float InDamage, TSubclassOf<UDamageType> InDamageTypeClass);

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Arcade-slow default so projectiles are visibly dodgeable. */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float ProjectileGravityScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float LifeSpanSeconds = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ProjectileVFX;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> ProjectileInstigator;

	float Damage = 0.f;
	TSubclassOf<UDamageType> DamageTypeClass;
	bool bInitialized = false;
	bool bHasHit = false;

	UFUNCTION()
	void HandleSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
