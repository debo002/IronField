#include "Combat/IFProjectile.h"

#include "Combat/IFCombatTargetingUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"

AIFProjectile::AIFProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(8.f);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(CollisionSphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->ProjectileGravityScale = ProjectileGravityScale;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AIFProjectile::HandleSphereBeginOverlap);
}

void AIFProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpanSeconds);

	if (ProjectileVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			ProjectileVFX,
			GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);
	}
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
	if (bHasHit || !bInitialized || !OtherActor || OtherActor == this || OtherActor == ProjectileInstigator)
	{
		return;
	}

	if (!IFCombatTargetingUtils::GetValidAttackTargetHealth(ProjectileInstigator, OtherActor))
	{
		return;
	}

	bHasHit = true;
	IFCombatTargetingUtils::DeliverDamage(OtherActor, ProjectileInstigator, Damage, DamageTypeClass);
	Destroy();
}
