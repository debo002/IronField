#include "Core/IFPlayerSubsystem.h"

#include "Character/IFPlayerCharacter.h"
#include "Core/IFSingletonActorRegistry.h"

void UIFPlayerSubsystem::RegisterPlayer(AIFPlayerCharacter* InPlayer)
{
	IFSingletonActorRegistry::Register(ActivePlayer, InPlayer, TEXT("Player"));
}

void UIFPlayerSubsystem::UnregisterPlayer(AIFPlayerCharacter* InPlayer)
{
	IFSingletonActorRegistry::Unregister(ActivePlayer, InPlayer);
}

AIFPlayerCharacter* UIFPlayerSubsystem::GetPlayer() const
{
	return ActivePlayer;
}
