#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/IFAttackAreaProvider.h"

inline float ResolveAttackRange(const AActor& TargetActor, float DefaultRange)
{
	if (const IIFAttackAreaProvider* const Provider = Cast<IIFAttackAreaProvider>(&TargetActor))
	{
		return Provider->GetAttackAreaRange();
	}

	return DefaultRange;
}

inline float ResolveAcceptanceRadius(const AActor& TargetActor, float DefaultRadius)
{
	if (const IIFAttackAreaProvider* const Provider = Cast<IIFAttackAreaProvider>(&TargetActor))
	{
		return Provider->GetAttackAreaAcceptanceRadius();
	}

	return DefaultRadius;
}
