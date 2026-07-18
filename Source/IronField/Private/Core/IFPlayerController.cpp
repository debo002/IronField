#include "Core/IFPlayerController.h"

#include "Core/IFPlayerControllerUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UI/IFHUD.h"
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

	HUDWidget = CreateWidget<UIFHUD>(this, HUDWidgetClass);
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

	IFPlayerControllerUtils::FocusWidgetWithUIOnlyInput(this, LoseScreenWidget);

	if (LoseStinger)
	{
		UGameplayStatics::PlaySound2D(this, LoseStinger);
	}

	UGameplayStatics::SetGamePaused(this, true);
}
