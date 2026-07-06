#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFStrongholdSubsystem.generated.h"

class AIFStronghold;

/**
 * World subsystem to provide fast, direct access to the level's Stronghold actor,
 * avoiding costly GetAllActorsOfClass iterations in C++ and Blueprints.
 */
UCLASS()
class IRONFIELD_API UIFStrongholdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Set the current active stronghold actor.
	void RegisterStronghold(AIFStronghold* InStronghold);

	// Clear the stronghold reference if it matches the current active stronghold.
	void UnregisterStronghold(AIFStronghold* InStronghold);

	// Retrieve the active stronghold actor.
	UFUNCTION(BlueprintPure, Category = "Stronghold")
	AIFStronghold* GetStronghold() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFStronghold> ActiveStronghold = nullptr;
};
