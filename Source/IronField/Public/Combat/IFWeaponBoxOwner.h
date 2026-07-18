#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IFWeaponBoxOwner.generated.h"

class UBoxComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UIFWeaponBoxOwner : public UInterface
{
	GENERATED_BODY()
};

class IRONFIELD_API IIFWeaponBoxOwner
{
	GENERATED_BODY()

public:
	virtual UBoxComponent* GetWeaponCollisionBox() const = 0;
};
