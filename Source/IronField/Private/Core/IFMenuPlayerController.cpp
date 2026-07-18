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
