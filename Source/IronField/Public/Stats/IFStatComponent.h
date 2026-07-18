#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IFStatComponent.generated.h"

/**
 * Shared clamp / percent helpers for health, stamina, and future resource components.
 * Domain types keep their own MaxX/CurrentX UPROPERTY names for Blueprint stability.
 */
UCLASS(Abstract, ClassGroup = (Custom))
class IRONFIELD_API UIFStatComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	static float ComputePercent(float Current, float Max)
	{
		return Max > 0.f ? Current / Max : 0.f;
	}

	/** Clamps Current into [0, Max]. Returns true if the value changed. */
	static bool ApplyClampedValue(float& Current, float Max, float NewValue)
	{
		const float Previous = Current;
		Current = FMath::Clamp(NewValue, 0.f, Max);
		return !FMath::IsNearlyEqual(Current, Previous);
	}
};
