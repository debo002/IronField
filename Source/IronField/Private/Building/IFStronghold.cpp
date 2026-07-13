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
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(false);

	HitboxComponent = CreateDefaultSubobject<USphereComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(RootComponent);
	HitboxComponent->SetSphereRadius(200.f);
	// ECC_GameTraceChannel1 is the project "Hittable Objectives" channel (shared with weapon boxes).
	HitboxComponent->SetCollisionObjectType(ECC_GameTraceChannel1);
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	// Weapon collision boxes are WorldDynamic; they must explicitly overlap this channel.
	HitboxComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitboxComponent->SetGenerateOverlapEvents(true);

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
	if (UWorld* const World = GetWorld())
	{
		if (UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			Subsystem->UnregisterStronghold(this);
		}
	}

	Super::EndPlay(EndPlayReason);

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.RemoveAll(this);
		HealthComponent->OnHealthDepleted.RemoveAll(this);
	}
}

float AIFStronghold::GetAttackAreaRange() const
{
	return FMath::Max(0.f, AttackAreaRange);
}

float AIFStronghold::GetAttackAreaAcceptanceRadius() const
{
	return FMath::Clamp(MoveAcceptanceRadius, 0.f, GetAttackAreaRange());
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
