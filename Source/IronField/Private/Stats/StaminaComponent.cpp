#include "Stats/StaminaComponent.h"

UIFStaminaComponent::UIFStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Component tick is enabled dynamically only during active drain or regeneration to save CPU time.
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UIFStaminaComponent::TryConsumeStamina(float Amount)
{
    if (Amount <= 0.f)
    {
        return true;
    }

    if (!HasStamina(Amount))
    {
        return false;
    }

    const float PreviousStamina = CurrentStamina;
    CurrentStamina = FMath::Max(CurrentStamina - Amount, 0.f);
    TimeSinceLastStaminaDrain = 0.f;

    BroadcastStaminaChangedIfNeeded(PreviousStamina);

    if (StaminaRegenRate > 0.f)
    {
        EnableStaminaTick();
    }

    BroadcastStaminaDepletedIfNeeded(PreviousStamina);

    return true;
}

void UIFStaminaComponent::StartContinuousDrain(float DrainRate)
{
    ContinuousDrainRate = FMath::Max(0.f, DrainRate);

    if (IsDrainingStamina() && CurrentStamina > 0.f)
    {
        EnableStaminaTick();
    }
}

void UIFStaminaComponent::StopContinuousDrain()
{
    ContinuousDrainRate = 0.f;
    UpdateTickForRegen();
}

float UIFStaminaComponent::GetStaminaPercent() const
{
    return CurrentStamina / MaxStamina;
}

void UIFStaminaComponent::BeginPlay()
{
    Super::BeginPlay();

    MaxStamina = FMath::Max(1.f, MaxStamina);
    StaminaRegenRate = FMath::Max(0.f, StaminaRegenRate);
    StaminaRegenDelay = FMath::Max(0.f, StaminaRegenDelay);

    CurrentStamina = MaxStamina;
    // Start the timer pre-expired so stamina can regenerate immediately if consumed right away.
    TimeSinceLastStaminaDrain = StaminaRegenDelay;
    ContinuousDrainRate = 0.f;

    OnStaminaChanged.Broadcast(GetStaminaPercent());
}

void UIFStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (IsDrainingStamina())
    {
        DrainStamina(DeltaTime);
        return;
    }

    RegenerateStamina(DeltaTime);
}

bool UIFStaminaComponent::IsDrainingStamina() const
{
    return ContinuousDrainRate > 0.f;
}

bool UIFStaminaComponent::CanRegenerateStamina() const
{
    return CurrentStamina < MaxStamina && StaminaRegenRate > 0.f;
}

void UIFStaminaComponent::DrainStamina(float DeltaTime)
{
    const float PreviousStamina = CurrentStamina;
    CurrentStamina = FMath::Max(CurrentStamina - ContinuousDrainRate * DeltaTime, 0.f);
    TimeSinceLastStaminaDrain = 0.f;

    BroadcastStaminaChangedIfNeeded(PreviousStamina);
    BroadcastStaminaDepletedIfNeeded(PreviousStamina);

    if (CurrentStamina <= 0.f)
    {
        StopContinuousDrain();
    }
}

void UIFStaminaComponent::RegenerateStamina(float DeltaTime)
{
    TimeSinceLastStaminaDrain += DeltaTime;
    if (TimeSinceLastStaminaDrain < StaminaRegenDelay)
    {
        return;
    }

    const float PreviousStamina = CurrentStamina;
    CurrentStamina = FMath::Min(CurrentStamina + StaminaRegenRate * DeltaTime, MaxStamina);

    BroadcastStaminaChangedIfNeeded(PreviousStamina);

    if (CurrentStamina >= MaxStamina)
    {
        DisableStaminaTick();
    }
}

void UIFStaminaComponent::UpdateTickForRegen()
{
    if (CanRegenerateStamina())
    {
        EnableStaminaTick();
        return;
    }

    DisableStaminaTick();
}

void UIFStaminaComponent::EnableStaminaTick()
{
    SetComponentTickEnabled(true);
}

void UIFStaminaComponent::DisableStaminaTick()
{
    SetComponentTickEnabled(false);
}

void UIFStaminaComponent::BroadcastStaminaChangedIfNeeded(float PreviousStamina)
{
    if (!FMath::IsNearlyEqual(CurrentStamina, PreviousStamina))
    {
        OnStaminaChanged.Broadcast(GetStaminaPercent());
    }
}

void UIFStaminaComponent::BroadcastStaminaDepletedIfNeeded(float PreviousStamina)
{
    if (PreviousStamina > 0.f && CurrentStamina <= 0.f)
    {
        OnStaminaDepleted.Broadcast();
    }
}
