#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFPlayerSubsystem.generated.h"

class AIFPlayerCharacter;

UCLASS()
class IRONFIELD_API UIFPlayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterPlayer(AIFPlayerCharacter* InPlayer);
	void UnregisterPlayer(AIFPlayerCharacter* InPlayer);

	UFUNCTION(BlueprintPure, Category = "IronField|Player")
	AIFPlayerCharacter* GetPlayer() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFPlayerCharacter> ActivePlayer = nullptr;
};
