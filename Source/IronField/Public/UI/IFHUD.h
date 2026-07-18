#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFHUD.generated.h"

class UIFStatBarWidget;
class UIFHealthComponent;
class UIFStaminaComponent;

/**
 * Gameplay HUD root. Binds player/stronghold health and stamina delegates and
 * forwards percentages to child UIFStatBarWidget instances. No tick, no interpolation.
 */
UCLASS()
class IRONFIELD_API UIFHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UIFStatBarWidget> PlayerHealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UIFStatBarWidget> PlayerStaminaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UIFStatBarWidget> StrongholdHealthBar;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePlayerHealthChanged(float Percent);

	UFUNCTION()
	void HandlePlayerStaminaChanged(float Percent);

	UFUNCTION()
	void HandleStrongholdHealthChanged(float Percent);

	void BindPlayerStatBars();
	void BindStrongholdStatBar();
	void UnbindAllSources();
	void SetBarPercent(UIFStatBarWidget* Bar, float Percent);

	TWeakObjectPtr<UIFHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UIFStaminaComponent> BoundPlayerStamina;
	TWeakObjectPtr<UIFHealthComponent> BoundStrongholdHealth;
};
