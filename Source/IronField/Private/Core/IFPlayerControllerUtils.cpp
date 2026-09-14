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

		TSharedPtr<SWidget> FocusWidget = Widget->TakeWidget();
		if (!FocusWidget.IsValid())
		{
			return;
		}

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FocusWidget.ToSharedRef());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
		Controller->bShowMouseCursor = true;
	}
}
