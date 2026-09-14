#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "IFProjectile.generated.h"

class UDamageType;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class IRONFIELD_API AIFProjectile : public AActor
{
	GENERATED_BODY()

public:
	AIFProjectile();

	void InitializeProjectile(AActor* InInstigator, float InDamage, TSubclassOf<UDamageType> InDamageTypeClass);

	/** Unscaled collision radius, used to spawn clear of the shooter's collision. */
	float GetCollisionSphereRadius() const { return CollisionSphere ? CollisionSphere->GetUnscaledSphereRadius() : 0.f; }

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "IronField|Projectile|Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "IronField|Projectile|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Arcade-slow default so projectiles stay visibly dodgeable. */
	UPROPERTY(EditDefaultsOnly, Category = "IronField|Projectile|Movement", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Projectile|Movement", meta = (ClampMin = "0.0"))
	float ProjectileGravityScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|Projectile|Lifetime", meta = (ClampMin = "0.1"))
	float LifeSpanSeconds = 5.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> ProjectileInstigator;

	float Damage = 0.f;
	TSubclassOf<UDamageType> DamageTypeClass;
	bool bInitialized = false;
	bool bHasHit = false;

	UFUNCTION()
	void HandleSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleSphereHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void HandleImpact(AActor* OtherActor);
};
