#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "IFBestRunSave.generated.h"

/** Single-slot best-run record, split per run mode. */
UCLASS()
class IRONFIELD_API UIFBestRunSave : public USaveGame
{
	GENERATED_BODY()

public:
	static FString GetSlotName() { return FString(TEXT("IronFieldBest")); }

	// Highest wave reached per mode, with kills from that run.
	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Save")
	int32 BestNormalWave = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Save")
	int32 BestNormalKills = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Save")
	int32 BestUnlimitedWave = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Save")
	int32 BestUnlimitedKills = 0;

	// Set once a Normal run ends in victory, never cleared.
	UPROPERTY(VisibleInstanceOnly, Category = "IronField|Save")
	bool bNormalCleared = false;

	/** Combined menu line covering both modes. */
	FText BuildBestText() const;
};
