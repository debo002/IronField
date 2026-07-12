#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IFGameMode.generated.h"

class AIFStronghold;

UCLASS()
class IRONFIELD_API AIFGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode|GameFlow")
	void OnGameWon();

	void OnGameLost();

private:
	// Level actors register with subsystems during their own BeginPlay; bind on the next tick so those registrations exist.
	void BindGameFlowDelegates();
	void UnbindGameFlowDelegates();

	UFUNCTION()
	void HandleAllWavesCompleted();

	UFUNCTION()
	void HandleStrongholdDestroyed(AIFStronghold* Stronghold);
};
