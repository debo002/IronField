#include "Stats/IFStaminaComponent.h"

UIFStaminaComponent::UIFStaminaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ============================================================================
// Actions
// ============================================================================

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

    SetStaminaClamped(CurrentStamina - Amount);
    TimeSinceLastStaminaDrain = 0.f;

    if (StaminaRegenRate > 0.f)
    {
        EnableStaminaTick();
    }

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

// ============================================================================
// Queries
// ============================================================================

float UIFStaminaComponent::GetStaminaPercent() const
{
    return CurrentStamina / MaxStamina;
}

// ============================================================================
// Lifecycle
// ============================================================================

void UIFStaminaComponent::BeginPlay()
{
    Super::BeginPlay();

    MaxStamina = FMath::Max(1.f, MaxStamina);
    StaminaRegenRate = FMath::Max(0.f, StaminaRegenRate);
    StaminaRegenDelay = FMath::Max(0.f, StaminaRegenDelay);

    CurrentStamina = MaxStamina;

    // Initialize to delay to prevent immediate regeneration on spawn.
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

// ============================================================================
// Internal helpers
// ============================================================================

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
    SetStaminaClamped(CurrentStamina - ContinuousDrainRate * DeltaTime);
    TimeSinceLastStaminaDrain = 0.f;

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

    SetStaminaClamped(CurrentStamina + StaminaRegenRate * DeltaTime);

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

void UIFStaminaComponent::SetStaminaClamped(float NewStamina)
{
    const float PreviousStamina = CurrentStamina;
    CurrentStamina = FMath::Clamp(NewStamina, 0.f, MaxStamina);
    BroadcastStaminaChangedIfNeeded(PreviousStamina);
    BroadcastStaminaDepletedIfNeeded(PreviousStamina);
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