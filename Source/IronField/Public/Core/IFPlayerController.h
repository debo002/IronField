#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IFPlayerController.generated.h"

class UIFLoseScreenWidget;
class USoundBase;
class UUserWidget;

UCLASS()
class IRONFIELD_API AIFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void ShowLoseScreen();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "PlayerController|UI")
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TSubclassOf<UIFLoseScreenWidget> LoseScreenWidgetClass;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "PlayerController|UI")
	TObjectPtr<UIFLoseScreenWidget> LoseScreenWidget;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
	TObjectPtr<USoundBase> LoseStinger;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateAndShowHUD();

	// Game input + hidden cursor so level entry never inherits UI-only state from the menu.
	void ApplyGameplayInputMode();
};
