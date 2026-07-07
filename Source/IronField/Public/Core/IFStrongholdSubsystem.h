#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IFStrongholdSubsystem.generated.h"

class AIFStronghold;

UCLASS()
class IRONFIELD_API UIFStrongholdSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterStronghold(AIFStronghold* InStronghold);

	void UnregisterStronghold(AIFStronghold* InStronghold);

	UFUNCTION(BlueprintPure, Category = "Stronghold")
	AIFStronghold* GetStronghold() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AIFStronghold> ActiveStronghold = nullptr;
};
