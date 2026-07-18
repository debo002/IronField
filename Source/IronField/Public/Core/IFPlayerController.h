#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IFPlayerController.generated.h"

class UIFHUD;
class UIFLoseScreenWidget;
class USoundBase;

UCLASS()
class IRONFIELD_API AIFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void ShowLoseScreen();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFHUD> HUDWidgetClass;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFHUD> HUDWidget;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TSubclassOf<UIFLoseScreenWidget> LoseScreenWidgetClass;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "IronField|PlayerController|UI")
	TObjectPtr<UIFLoseScreenWidget> LoseScreenWidget;

	UPROPERTY(EditDefaultsOnly, Category = "IronField|PlayerController|UI")
	TObjectPtr<USoundBase> LoseStinger;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateAndShowHUD();

	// Game input + hidden cursor so level entry never inherits UI-only state from the menu.
	void ApplyGameplayInputMode();
};
