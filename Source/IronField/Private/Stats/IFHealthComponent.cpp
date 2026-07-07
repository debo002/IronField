#include "Stats/IFHealthComponent.h"

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


void UIFHealthComponent::HandleOwnerTakeAnyDamage(AActor* , float Damage, const UDamageType* , AController* , AActor* )
{
    ApplyDamage(Damage);
}

void UIFHealthComponent::SetHealthClamped(float NewHealth)
{
    const float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
    BroadcastHealthChangedIfNeeded(PreviousHealth);
}

void UIFHealthComponent::BroadcastHealthChangedIfNeeded(float PreviousHealth)
{
    if (!FMath::IsNearlyEqual(CurrentHealth, PreviousHealth))
    {
        OnHealthChanged.Broadcast(GetHealthPercent());
    }
}
