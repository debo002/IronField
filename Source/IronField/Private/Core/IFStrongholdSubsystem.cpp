#include "Core/IFStrongholdSubsystem.h"
#include "Building/IFStronghold.h"
#include "Core/IFLog.h"

void UIFStrongholdSubsystem::RegisterStronghold(AIFStronghold* InStronghold)
{
	if (ActiveStronghold && ActiveStronghold != InStronghold)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-Subsystem] RegisterStronghold called while a different stronghold (%s) is already registered. Overwriting."),
			*GetNameSafe(ActiveStronghold));
	}
	ActiveStronghold = InStronghold;
}

void UIFStrongholdSubsystem::UnregisterStronghold(AIFStronghold* InStronghold)
{
	if (ActiveStronghold == InStronghold)
	{
		ActiveStronghold = nullptr;
	}
}

AIFStronghold* UIFStrongholdSubsystem::GetStronghold() const
{
	return ActiveStronghold;
}
