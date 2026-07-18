#include "Core/IFWaveManagerSubsystem.h"

#include "Core/IFSingletonActorRegistry.h"
#include "Wave/IFWaveManager.h"

void UIFWaveManagerSubsystem::RegisterWaveManager(AIFWaveManager* InWaveManager)
{
	IFSingletonActorRegistry::Register(ActiveWaveManager, InWaveManager, TEXT("WaveManager"));
}

void UIFWaveManagerSubsystem::UnregisterWaveManager(AIFWaveManager* InWaveManager)
{
	IFSingletonActorRegistry::Unregister(ActiveWaveManager, InWaveManager);
}

AIFWaveManager* UIFWaveManagerSubsystem::GetWaveManager() const
{
	return ActiveWaveManager;
}
