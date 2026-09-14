#include "Core/IFMenuPlayerController.h"

#include "Core/IFPlayerControllerUtils.h"
#include "UI/IFMainMenuWidget.h"

void AIFMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	CreateAndShowMainMenu();
}

void AIFMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	// Reset to game input so the LocalPlayer does not carry UI-only state into the gameplay level.
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	Super::EndPlay(EndPlayReason);
}

void AIFMenuPlayerController::CreateAndShowMainMenu()
{
	if (!MainMenuWidgetClass)
	{
		return;
	}

	MainMenuWidget = CreateWidget<UIFMainMenuWidget>(this, MainMenuWidgetClass);
	if (!MainMenuWidget)
	{
		return;
	}

	IFPlayerControllerUtils::FocusWidgetWithUIOnlyInput(this, MainMenuWidget);
}
