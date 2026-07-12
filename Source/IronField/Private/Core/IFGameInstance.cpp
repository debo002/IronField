#include "Core/IFGameInstance.h"

const FName UIFGameInstance::MainMenuLevelName(TEXT("MainMenu"));
const FName UIFGameInstance::GameplayLevelName(TEXT("MainLevel"));

void UIFGameInstance::SetRunMode(EIFRunMode NewRunMode)
{
	RunMode = NewRunMode;
}
