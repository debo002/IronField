#include "Core/IFPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "UI/IFLoseScreenWidget.h"

void AIFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyGameplayInputMode();
	CreateAndShowHUD();
}

void AIFPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	if (LoseScreenWidget)
	{
		LoseScreenWidget->RemoveFromParent();
		LoseScreenWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AIFPlayerController::CreateAndShowHUD()
{
	if (!HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void AIFPlayerController::ApplyGameplayInputMode()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	FlushPressedKeys();
}

void AIFPlayerController::ShowLoseScreen()
{
	if (!LoseScreenWidgetClass || LoseScreenWidget)
	{
		return;
	}

	LoseScreenWidget = CreateWidget<UIFLoseScreenWidget>(this, LoseScreenWidgetClass);
	if (!LoseScreenWidget)
	{
		return;
	}

	LoseScreenWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(LoseScreenWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	if (LoseStinger)
	{
		UGameplayStatics::PlaySound2D(this, LoseStinger);
	}

	UGameplayStatics::SetGamePaused(this, true);
}
