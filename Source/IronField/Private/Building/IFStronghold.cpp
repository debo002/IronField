#include "Building/IFStronghold.h"
#include "Building/IFStrongholdAttackPoint.h"
#include "Stats/IFHealthComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Core/IFStrongholdSubsystem.h"

AIFStronghold::AIFStronghold()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	// Visual mesh keeps blocking collision (so pawns can't walk through the building) but
	// does not generate overlaps - hit detection lives entirely on HitboxComponent instead.
	MeshComponent->SetGenerateOverlapEvents(false);

	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(RootComponent);
	HitboxComponent->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	HitboxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitboxComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
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

void AIFStronghold::HandleHealthChanged(float Percent)
{
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

	MeshComponent->SetVisibility(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);
}

AIFStrongholdAttackPoint* AIFStronghold::ReserveNearestFreeAttackPoint(const FVector& FromLocation)
{
	AIFStrongholdAttackPoint* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AIFStrongholdAttackPoint* const Point : AttackPoints)
	{
		if (!Point || Point->IsOccupied())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(FromLocation, Point->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Point;
		}
	}

	if (Best)
	{
		Best->SetOccupied(true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[IF-Stronghold] No free attack points available (configured: %d)"), AttackPoints.Num());
	}

	return Best;
}

void AIFStronghold::ReleaseAttackPoint(AIFStrongholdAttackPoint* Point)
{
	if (Point)
	{
		Point->SetOccupied(false);
	}
}