#pragma once

#include "CoreMinimal.h"
#include "Core/IFGameTypes.h"
#include "Engine/GameInstance.h"
#include "IFGameInstance.generated.h"

UCLASS()
class IRONFIELD_API UIFGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronField|GameInstance|Levels")
	FName MainMenuLevelName = TEXT("MainMenu");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronField|GameInstance|Levels")
	FName GameplayLevelName = TEXT("MainLevel");

	// Survives level travel so the wave manager can read the mode chosen on the menu.
	UFUNCTION(BlueprintCallable, Category = "IronField|GameInstance|RunMode")
	void SetRunMode(EIFRunMode NewRunMode);

	UFUNCTION(BlueprintPure, Category = "IronField|GameInstance|RunMode")
	EIFRunMode GetRunMode() const { return RunMode; }

private:
	UPROPERTY(VisibleInstanceOnly, Category = "IronField|GameInstance|RunMode")
	EIFRunMode RunMode = EIFRunMode::Normal;
};
