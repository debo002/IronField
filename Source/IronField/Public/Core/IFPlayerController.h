#pragma once

#include "CoreMinimal.h"
#include "Core/IFGameTypes.h"
#include "GameFramework/PlayerController.h"
#include "IFPlayerController.generated.h"

class UIFGameOverScreenWidget;
class UIFHUD;

UCLASS()
class IRONFIELD_API AIFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AIFPlayerController();

	void ShowGameOverScreen(EIFGameResult Result);
	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFHUD> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFGameOverScreenWidget> GameOverScreenWidgetClass;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFHUD> HUDWidget;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFGameOverScreenWidget> GameOverScreenWidget;

	void CreateAndShowHUD();

	// Game input + hidden cursor so level entry never inherits UI-only state from the menu.
	void ApplyGameplayInputMode();
};
