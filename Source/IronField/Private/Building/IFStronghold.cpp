#include "Building/IFStronghold.h"

#include "Components/SphereComponent.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Stats/IFHealthComponent.h"

AIFStronghold::AIFStronghold()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCanEverAffectNavigation(false);

	HitboxComponent = CreateDefaultSubobject<USphereComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(RootComponent);
	HitboxComponent->SetSphereRadius(200.f);

	HealthComponent = CreateDefaultSubobject<UIFHealthComponent>(TEXT("HealthComponent"));
}

void AIFStronghold::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &AIFStronghold::HandleHealthChanged);
		HealthComponent->OnHealthDepleted.AddDynamic(this, &AIFStronghold::HandleDeath);
	}

	if (UWorld* const World = GetWorld())
	{
		if (UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			Subsystem->RegisterStronghold(this);
		}
	}
}

void AIFStronghold::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveAll(this);
		HealthComponent->OnHealthDepleted.RemoveAll(this);
	}

	if (UWorld* const World = GetWorld())
	{
		if (UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			Subsystem->UnregisterStronghold(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AIFStronghold::HandleHealthChanged(float Percent)
{
	// Skip BeginPlay full-health broadcast and death (0%).
	if (Percent <= 0.f || FMath::IsNearlyEqual(Percent, 1.f))
	{
		return;
	}

	PlayHitFeedback(GetActorLocation());
}

void AIFStronghold::HandleDeath()
{
	PlayHitFeedback(GetActorLocation());
	HandleDestruction();
}

void AIFStronghold::PlayHitFeedback(const FVector& Location)
{
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, Location);
	}

	if (HitVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitVFX, Location, FRotator::ZeroRotator);
	}
}

void AIFStronghold::HandleDestruction()
{
	OnStrongholdDestroyed.Broadcast(this);

	// Component-level off for mesh/hitbox queries, plus actor-level override so nothing re-enables mid-frame.
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HitboxComponent)
	{
		HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorEnableCollision(false);
}
