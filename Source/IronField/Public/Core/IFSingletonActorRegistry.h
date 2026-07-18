#pragma once

#include "CoreMinimal.h"
#include "Core/IFLog.h"

/** Shared Register/Unregister for one-active-instance world subsystems (player, stronghold, wave manager). */
namespace IFSingletonActorRegistry
{
	template <typename TActor>
	void Register(TObjectPtr<TActor>& Slot, TActor* Instance, const TCHAR* Label)
	{
		if (Slot && Slot != Instance)
		{
			UE_LOG(LogIronField, Warning,
				TEXT("[IF-Subsystem] Register%s called while a different instance (%s) is already registered. Overwriting."),
				Label, *GetNameSafe(Slot.Get()));
		}
		Slot = Instance;
	}

	template <typename TActor>
	void Unregister(TObjectPtr<TActor>& Slot, TActor* Instance)
	{
		if (Slot == Instance)
		{
			Slot = nullptr;
		}
	}
}
