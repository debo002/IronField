#include "Core/IFStrongholdSubsystem.h"
#include "Building/IFStronghold.h"

void UIFStrongholdSubsystem::RegisterStronghold(AIFStronghold* InStronghold)
{
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
