#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IFAttackAreaProvider.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UIFAttackAreaProvider : public UInterface
{
	GENERATED_BODY()
};

class IRONFIELD_API IIFAttackAreaProvider
{
	GENERATED_BODY()

public:
	virtual float GetAttackAreaRange() const = 0;

	virtual float GetAttackAreaAcceptanceRadius() const = 0;
};
