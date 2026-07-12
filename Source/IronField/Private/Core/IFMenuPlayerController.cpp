#include "Core/IFMenuPlayerController.h"

#include "UI/IFMainMenuWidget.h"

void AIFMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyMenuInputMode();
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

	MainMenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AIFMenuPlayerController::ApplyMenuInputMode()
{
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
