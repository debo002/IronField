#include "Stats/HealthComponent.h"

#include "GameFramework/Actor.h"

UIFHealthComponent::UIFHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UIFHealthComponent::ApplyDamage(float Amount)
{
    if (bIsDead || bIsInvincible || Amount <= 0.f)
    {
        return;
    }

    const float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Max(CurrentHealth - Amount, 0.f);

    BroadcastHealthChangedIfNeeded(PreviousHealth);

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

    const float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);

    BroadcastHealthChangedIfNeeded(PreviousHealth);
}

void UIFHealthComponent::Revive()
{
    CurrentHealth = FMath::Clamp(ReviveHealth, 1.f, MaxHealth);
    bIsDead = false;
    OnHealthChanged.Broadcast(GetHealthPercent());
}

float UIFHealthComponent::GetHealthPercent() const
{
    return CurrentHealth / MaxHealth;
}

void UIFHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // Floor of 1 prevents division by zero in GetHealthPercent.
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
    Super::EndPlay(EndPlayReason);

    if (AActor* const Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.RemoveAll(this);
    }
}

void UIFHealthComponent::HandleOwnerTakeAnyDamage(AActor* /*DamagedActor*/, float Damage, const UDamageType* /*DamageType*/, AController* /*InstigatedBy*/, AActor* /*DamageCauser*/)
{
    ApplyDamage(Damage);
}

void UIFHealthComponent::BroadcastHealthChangedIfNeeded(float PreviousHealth)
{
    if (!FMath::IsNearlyEqual(CurrentHealth, PreviousHealth))
    {
        OnHealthChanged.Broadcast(GetHealthPercent());
    }
}
