#include "Core/IFPlayerControllerUtils.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

namespace IFPlayerControllerUtils
{
	void FocusWidgetWithUIOnlyInput(APlayerController* Controller, UUserWidget* Widget)
	{
		if (!Controller || !Widget)
		{
			return;
		}

		Widget->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
		Controller->bShowMouseCursor = true;
	}
}
