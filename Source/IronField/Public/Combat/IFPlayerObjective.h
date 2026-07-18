#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IFPlayerObjective.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UIFPlayerObjective : public UInterface
{
	GENERATED_BODY()
};

/** Player-owned objective (stronghold, towers, etc.). Combat targeting uses this instead of hardcoding classes. */
class IRONFIELD_API IIFPlayerObjective
{
	GENERATED_BODY()

public:
	/** When true, non-AI instigators (the player) cannot damage this actor. */
	virtual bool IsProtectedFromPlayerDamage() const = 0;
};
