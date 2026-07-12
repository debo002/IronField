#include "Core/IFWaveManagerSubsystem.h"
#include "Wave/IFWaveManager.h"
#include "Core/IFLog.h"

void UIFWaveManagerSubsystem::RegisterWaveManager(AIFWaveManager* InWaveManager)
{
	if (ActiveWaveManager && ActiveWaveManager != InWaveManager)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Subsystem] RegisterWaveManager called while a different wave manager (%s) is already registered. Overwriting."),
			*GetNameSafe(ActiveWaveManager));
	}
	ActiveWaveManager = InWaveManager;
}

void UIFWaveManagerSubsystem::UnregisterWaveManager(AIFWaveManager* InWaveManager)
{
	if (ActiveWaveManager == InWaveManager)
	{
		ActiveWaveManager = nullptr;
	}
}

AIFWaveManager* UIFWaveManagerSubsystem::GetWaveManager() const
{
	return ActiveWaveManager;
}
