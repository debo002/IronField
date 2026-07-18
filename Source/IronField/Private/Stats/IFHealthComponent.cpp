#include "Stats/IFHealthComponent.h"

#include "GameFramework/Actor.h"

UIFHealthComponent::UIFHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentHealth = MaxHealth;
}

void UIFHealthComponent::ApplyDamage(float Amount)
{
	if (bIsDead || bIsInvincible || Amount <= 0.f)
	{
		return;
	}

	SetHealthClamped(CurrentHealth - Amount);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnHealthDepleted.Broadcast();
	}
}

void UIFHealthComponent::ApplyHealing(float Amount)
{
	if (bIsDead || Amount <= 0.f)
	{
		return;
	}

	SetHealthClamped(CurrentHealth + Amount);
}

void UIFHealthComponent::Revive()
{
	bIsDead = false;
	SetHealthClamped(FMath::Clamp(ReviveHealth, 1.f, MaxHealth));
}

void UIFHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.f, MaxHealth);
	CurrentHealth = MaxHealth;
	bIsDead = false;

	if (AActor* const Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UIFHealthComponent::HandleOwnerTakeAnyDamage);
	}

	OnHealthChanged.Broadcast(GetHealthPercent());
}

void UIFHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* const Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UIFHealthComponent::HandleOwnerTakeAnyDamage(AActor*, float Damage, const UDamageType*, AController*, AActor*)
{
	ApplyDamage(Damage);
}

void UIFHealthComponent::SetHealthClamped(float NewHealth)
{
	if (ApplyClampedValue(CurrentHealth, MaxHealth, NewHealth))
	{
		OnHealthChanged.Broadcast(GetHealthPercent());
	}
}
