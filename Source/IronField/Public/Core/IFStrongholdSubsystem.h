#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFStrongholdSubsystem.generated.h"

class AIFStronghold;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStrongholdRegistered, AIFStronghold*, Stronghold);

UCLASS()
class IRONFIELD_API UIFStrongholdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Broadcast when the stronghold registers. Consumers should also check GetStronghold()
	// to cover registration that already happened before they subscribed.
	UPROPERTY(BlueprintAssignable, Category = "IronField|Stronghold|Events")
	FOnStrongholdRegistered OnStrongholdRegistered;

	void RegisterStronghold(AIFStronghold* InStronghold);
	void UnregisterStronghold(AIFStronghold* InStronghold);

	UFUNCTION(BlueprintPure, Category = "IronField|Stronghold")
	AIFStronghold* GetStronghold() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFStronghold> ActiveStronghold = nullptr;
};
