#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IFGameMode.generated.h"

/**
 * Root game mode for the project.
 * Extend this class to add match rules, default pawn assignments, and session logic.
 */
UCLASS()
class IRONFIELD_API AIFGameMode : public AGameModeBase
{
    GENERATED_BODY()
};
