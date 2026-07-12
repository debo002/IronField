#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFWaveManagerSubsystem.generated.h"

class AIFWaveManager;

UCLASS()
class IRONFIELD_API UIFWaveManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterWaveManager(AIFWaveManager* InWaveManager);

	void UnregisterWaveManager(AIFWaveManager* InWaveManager);

	UFUNCTION(BlueprintPure, Category = "WaveManager")
	AIFWaveManager* GetWaveManager() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFWaveManager> ActiveWaveManager = nullptr;
};
