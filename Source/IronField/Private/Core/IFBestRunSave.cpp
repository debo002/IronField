#include "Core/IFBestRunSave.h"

FText UIFBestRunSave::BuildBestText() const
{
	// Em dash keeps the empty-slot line visually quiet.
	if (BestNormalWave <= 0 && BestUnlimitedWave <= 0)
	{
		return FText::FromString(TEXT("NORMAL BEST: \u2014  \u2022  UNLIMITED BEST: \u2014"));
	}

	FString ClearedSuffix;
	if (bNormalCleared)
	{
		ClearedSuffix = TEXT("  \u2022  NORMAL CLEARED");
	}

	return FText::FromString(FString::Printf(TEXT("NORMAL BEST: W%d K%d  \u2022  UNLIMITED BEST: W%d K%d%s"),
		BestNormalWave, BestNormalKills, BestUnlimitedWave, BestUnlimitedKills, *ClearedSuffix));
}
