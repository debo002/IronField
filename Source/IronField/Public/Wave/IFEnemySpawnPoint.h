#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IFEnemySpawnPoint.generated.h"

/**
 * A placeable actor in the level that defines a valid transform for enemy spawning.
 * Contains no gameplay logic, serving purely as a locator.
 */
UCLASS()
class IRONFIELD_API AIFEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AIFEnemySpawnPoint();
};
