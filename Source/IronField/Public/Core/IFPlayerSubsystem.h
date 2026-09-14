#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFPlayerSubsystem.generated.h"

class AIFPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRegistered, AIFPlayerCharacter*, Player);

UCLASS()
class IRONFIELD_API UIFPlayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Broadcast when the player registers. Consumers should also check GetPlayer()
	// to cover registration that already happened before they subscribed.
	UPROPERTY(BlueprintAssignable, Category = "IronField|Player|Events")
	FOnPlayerRegistered OnPlayerRegistered;

	void RegisterPlayer(AIFPlayerCharacter* InPlayer);
	void UnregisterPlayer(AIFPlayerCharacter* InPlayer);

	UFUNCTION(BlueprintPure, Category = "IronField|Player")
	AIFPlayerCharacter* GetPlayer() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFPlayerCharacter> ActivePlayer = nullptr;
};
