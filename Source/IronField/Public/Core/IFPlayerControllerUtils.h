#pragma once

#include "CoreMinimal.h"

class APlayerController;
class UUserWidget;

namespace IFPlayerControllerUtils
{
	/** Show a full-screen UI widget and switch the controller to UI-only input focused on it. */
	void FocusWidgetWithUIOnlyInput(APlayerController* Controller, UUserWidget* Widget);

	/** Shared unpause-then-travel used by pause and game-over screens. */
	void OpenLevelUnpaused(const UObject* WorldContext, FName LevelName);
}
