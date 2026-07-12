#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "IFGameInstance.generated.h"

UENUM(BlueprintType)
enum class EIFRunMode : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Unlimited UMETA(DisplayName = "Unlimited")
};

UCLASS()
class IRONFIELD_API UIFGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	static const FName MainMenuLevelName;
	static const FName GameplayLevelName;

	// Survives level travel so the wave manager can read the mode chosen on the menu.
	UFUNCTION(BlueprintCallable, Category = "GameInstance|RunMode")
	void SetRunMode(EIFRunMode NewRunMode);

	UFUNCTION(BlueprintPure, Category = "GameInstance|RunMode")
	EIFRunMode GetRunMode() const { return RunMode; }

private:
	UPROPERTY(VisibleInstanceOnly, Category = "GameInstance|RunMode")
	EIFRunMode RunMode = EIFRunMode::Normal;
};
