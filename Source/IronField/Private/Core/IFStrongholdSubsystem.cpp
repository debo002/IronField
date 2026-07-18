#include "Core/IFStrongholdSubsystem.h"

#include "Building/IFStronghold.h"
#include "Core/IFSingletonActorRegistry.h"

void UIFStrongholdSubsystem::RegisterStronghold(AIFStronghold* InStronghold)
{
	IFSingletonActorRegistry::Register(ActiveStronghold, InStronghold, TEXT("Stronghold"));
}

void UIFStrongholdSubsystem::UnregisterStronghold(AIFStronghold* InStronghold)
{
	IFSingletonActorRegistry::Unregister(ActiveStronghold, InStronghold);
}

AIFStronghold* UIFStrongholdSubsystem::GetStronghold() const
{
	return ActiveStronghold;
}
