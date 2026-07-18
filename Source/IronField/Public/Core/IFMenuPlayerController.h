#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IFMenuPlayerController.generated.h"

class UIFMainMenuWidget;

UCLASS()
class IRONFIELD_API AIFMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFMainMenuWidget> MainMenuWidget;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateAndShowMainMenu();
};
