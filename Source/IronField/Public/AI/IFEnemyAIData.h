#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IFEnemyAIData.generated.h"

UCLASS(Blueprintable)
class IRONFIELD_API UIFEnemyAIData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float MinReattackCooldownSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float MaxReattackCooldownSeconds = 1.2f;

	/** Player farther than this → target the stronghold instead. */
	UPROPERTY(EditDefaultsOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float PlayerDetectionRange = 2000.f;

	/** Other target must be this much closer before switching. */
	UPROPERTY(EditDefaultsOnly, Category = "Targeting", meta = (ClampMin = "0.0"))
	float TargetSwitchMargin = 250.f;
};
