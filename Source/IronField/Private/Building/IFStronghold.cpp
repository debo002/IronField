#include "Building/IFStronghold.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IFFeedbackUtils.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Engine/World.h"
#include "Stats/IFHealthComponent.h"

AIFStronghold::AIFStronghold()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCanEverAffectNavigation(false);

	HealthComponent = CreateDefaultSubobject<UIFHealthComponent>(TEXT("HealthComponent"));

	// Sole lose condition: survives ~50 melee hits @16 / ~80 mage hits @10.
	// Pure HP pool with no regen. BP can still override.
	HealthComponent->SetMaxHealth(800.f);
}

void AIFStronghold::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnHealthDepleted.AddDynamic(this, &AIFStronghold::HandleDeath);
		HealthComponent->OnHealthChanged.AddDynamic(this, &AIFStronghold::HandleHealthChanged);
		LastHealthPercent = HealthComponent->GetHealthPercent();
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
		HealthComponent->OnHealthDepleted.RemoveAll(this);
		HealthComponent->OnHealthChanged.RemoveAll(this);
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

void AIFStronghold::HandleDeath()
{
	HandleDestruction();
}

void AIFStronghold::HandleHealthChanged(float Percent)
{
	if (Percent >= LastHealthPercent)
	{
		LastHealthPercent = Percent;
		return;
	}

	LastHealthPercent = Percent;
	PlayHitFeedback();
}

void AIFStronghold::PlayHitFeedback()
{
	IFFeedbackUtils::PlayAtLocation(GetWorld(), HitSound, HitVFX, GetActorLocation());
}

void AIFStronghold::HandleDestruction()
{
	OnStrongholdDestroyed.Broadcast(this);

	// Disable the visible mesh's collision when the stronghold is destroyed.
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorEnableCollision(false);
}
