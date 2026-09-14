#include "Combat/IFProjectile.h"

#include "Combat/IFCombatTargetingUtils.h"
#include "Core/IFLog.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

AIFProjectile::AIFProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->bRotationFollowsVelocity = true;

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AIFProjectile::HandleSphereBeginOverlap);
	CollisionSphere->OnComponentHit.AddDynamic(this, &AIFProjectile::HandleSphereHit);
}

void AIFProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpanSeconds);
}

void AIFProjectile::InitializeProjectile(AActor* InInstigator, float InDamage, TSubclassOf<UDamageType> InDamageTypeClass)
{
	ProjectileInstigator = InInstigator;
	Damage = InDamage;
	DamageTypeClass = InDamageTypeClass;
	bInitialized = true;

	if (InInstigator)
	{
		CollisionSphere->IgnoreActorWhenMoving(InInstigator, true);
	}

	// Movement values are applied here, not just in the constructor: Blueprint overrides
	// of the EditDefaultsOnly properties only exist after the constructor has run.
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = ProjectileSpeed;
		ProjectileMovement->MaxSpeed = ProjectileSpeed;
		ProjectileMovement->ProjectileGravityScale = ProjectileGravityScale;
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileSpeed;
	}
}

void AIFProjectile::HandleSphereBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	HandleImpact(OtherActor);
}

void AIFProjectile::HandleSphereHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult&)
{
	HandleImpact(OtherActor);
}

void AIFProjectile::HandleImpact(AActor* OtherActor)
{
	if (bHasHit || !OtherActor || OtherActor == this || OtherActor == ProjectileInstigator)
	{
		return;
	}

	if (!bInitialized)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Combat] %s impacted %s without InitializeProjectile; destroying."),
			*GetNameSafe(this), *GetNameSafe(OtherActor));
		bHasHit = true;
		Destroy();
		return;
	}

	if (Cast<AIFProjectile>(OtherActor))
	{
		return;
	}

	if (!IFCombatTargetingUtils::GetValidAttackTargetHealth(ProjectileInstigator, OtherActor))
	{
		// Pawns pass through (no friendly fire, corpses never eat shots);
		// anything else is geometry and must consume the projectile.
		if (!Cast<APawn>(OtherActor))
		{
			bHasHit = true;
			Destroy();
		}
		return;
	}

	bHasHit = true;
	IFCombatTargetingUtils::DeliverDamage(OtherActor, ProjectileInstigator, Damage, DamageTypeClass);
	Destroy();
}
