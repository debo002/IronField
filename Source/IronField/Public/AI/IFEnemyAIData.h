#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IFEnemyAIData.generated.h"

/** Shared enemy AI tunables, referenced by AIFEnemyController. */
UCLASS(Blueprintable)
class IRONFIELD_API UIFEnemyAIData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|AI|Combat", meta = (ClampMin = "0.0"))
	float MinReattackCooldownSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|AI|Combat", meta = (ClampMin = "0.0"))
	float MaxReattackCooldownSeconds = 1.2f;

	/** Player farther than this → target the stronghold instead. */
	UPROPERTY(EditDefaultsOnly, Category = "IronField|AI|Targeting", meta = (ClampMin = "0.0"))
	float PlayerDetectionRange = 2000.f;

	/** Other target must be this much closer before switching. */
	UPROPERTY(EditDefaultsOnly, Category = "IronField|AI|Targeting", meta = (ClampMin = "0.0"))
	float TargetSwitchMargin = 250.f;
};
