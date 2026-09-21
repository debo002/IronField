#pragma once

#include "CoreMinimal.h"
#include "Core/IFGameTypes.h"
#include "GameFramework/PlayerController.h"
#include "IFPlayerController.generated.h"

class UIFGameOverScreenWidget;
class UIFHUD;
class UIFPauseMenuWidget;

UCLASS()
class IRONFIELD_API AIFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AIFPlayerController();

	void ShowGameOverScreen(EIFGameResult Result);
	void TogglePauseGame();
	void ResumeGame();
	void RestartLevel();
	void ReturnToMainMenu();
	virtual void OnPossess(APawn* InPawn) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFHUD> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFGameOverScreenWidget> GameOverScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFPauseMenuWidget> PauseMenuWidgetClass;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFHUD> HUDWidget;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFGameOverScreenWidget> GameOverScreenWidget;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFPauseMenuWidget> PauseMenuWidget;

	void CreateAndShowHUD();

	// Game input + hidden cursor so level entry never inherits UI-only state from the menu.
	void ApplyGameplayInputMode();
	bool IsGameOverActive() const { return GameOverScreenWidget != nullptr; }
	bool IsPauseMenuOpen() const { return PauseMenuWidget != nullptr; }
	void ShowPauseMenu();
	void ClosePauseMenu();
	void OpenLevelUnpaused(FName LevelName);
};
