#include "Building/Stronghold.h"
#include "Stats/HealthComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AIFStronghold::AIFStronghold()
{
	PrimaryActorTick.bCanEverTick = false;	

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

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
}

void AIFStronghold::HandleHealthChanged(float Percent)
{
	// The killing blow's feedback is played by HandleDeath instead, to avoid double-firing sound/VFX.
	if (Percent <= 0.0f)
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

	// Placeholder destruction visuals until Chaos Fracture (Geometry Collections) replaces this with a real break-apart effect.
	MeshComponent->SetVisibility(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);
}