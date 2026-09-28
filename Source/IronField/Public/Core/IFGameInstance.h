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
	static constexpr const TCHAR* DefaultMainMenuLevelName = TEXT("MainMenu");
	static constexpr const TCHAR* DefaultGameplayLevelName = TEXT("MainLevel");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronField|GameInstance|Levels")
	FName MainMenuLevelName = DefaultMainMenuLevelName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronField|GameInstance|Levels")
	FName GameplayLevelName = DefaultGameplayLevelName;

	// Survives level travel so the wave manager can read the mode chosen on the menu.
	UFUNCTION(BlueprintCallable, Category = "IronField|GameInstance|RunMode")
	void SetRunMode(EIFRunMode NewRunMode);

	UFUNCTION(BlueprintPure, Category = "IronField|GameInstance|RunMode")
	EIFRunMode GetRunMode() const { return RunMode; }

private:
	UPROPERTY(VisibleInstanceOnly, Category = "IronField|GameInstance|RunMode")
	EIFRunMode RunMode = EIFRunMode::Normal;
};
