#include "Core/IFMainMenuGameMode.h"

#include "Core/IFMenuPlayerController.h"

AIFMainMenuGameMode::AIFMainMenuGameMode()
{
	PlayerControllerClass = AIFMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
