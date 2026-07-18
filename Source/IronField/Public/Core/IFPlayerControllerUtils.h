#pragma once

#include "CoreMinimal.h"

class APlayerController;
class UUserWidget;

namespace IFPlayerControllerUtils
{
	/** Show a full-screen UI widget and switch the controller to UI-only input focused on it. */
	void FocusWidgetWithUIOnlyInput(APlayerController* Controller, UUserWidget* Widget);
}
