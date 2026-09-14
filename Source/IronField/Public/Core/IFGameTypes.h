#pragma once

#include "CoreMinimal.h"
#include "IFGameTypes.generated.h"

/** Outcome of a finished run, shown on the game over screen. */
UENUM(BlueprintType)
enum class EIFGameResult : uint8
{
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat")
};

/** Run mode chosen on the main menu. Stored on the game instance so it survives level travel. */
UENUM(BlueprintType)
enum class EIFRunMode : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Unlimited UMETA(DisplayName = "Unlimited")
};
