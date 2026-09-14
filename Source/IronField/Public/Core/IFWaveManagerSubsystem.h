#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFWaveManagerSubsystem.generated.h"

class AIFWaveManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveManagerRegistered, AIFWaveManager*, WaveManager);

UCLASS()
class IRONFIELD_API UIFWaveManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Broadcast when the wave manager registers. Consumers should also check GetWaveManager()
	// to cover registration that already happened before they subscribed.
	UPROPERTY(BlueprintAssignable, Category = "IronField|Wave|Events")
	FOnWaveManagerRegistered OnWaveManagerRegistered;

	void RegisterWaveManager(AIFWaveManager* InWaveManager);
	void UnregisterWaveManager(AIFWaveManager* InWaveManager);

	UFUNCTION(BlueprintPure, Category = "IronField|Wave")
	AIFWaveManager* GetWaveManager() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFWaveManager> ActiveWaveManager = nullptr;
};
