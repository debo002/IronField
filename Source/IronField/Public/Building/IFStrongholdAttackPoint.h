#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IFStrongholdAttackPoint.generated.h"

/**
 * A placeable actor around the Stronghold marking a valid position for an enemy to stand
 * at and attack from. Contains no gameplay logic beyond occupancy tracking - reservation
 * is handled by AIFStronghold so only one enemy claims a given point at a time.
 */
UCLASS()
class IRONFIELD_API AIFStrongholdAttackPoint : public AActor
{
	GENERATED_BODY()

public:
	AIFStrongholdAttackPoint();

	bool IsOccupied() const { return bOccupied; }

	void SetOccupied(bool bNewOccupied) { bOccupied = bNewOccupied; }

private:
	bool bOccupied = false;
};
