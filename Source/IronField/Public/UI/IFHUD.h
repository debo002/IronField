#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFHUD.generated.h"

class AIFStronghold;
class AIFPlayerCharacter;
class UIFStatBarWidget;
class UIFHealthComponent;
class UIFStaminaComponent;

/**
 * Gameplay HUD root. Binds player and stronghold stat delegates and forwards
 * percentages to child UIFStatBarWidget instances.
 */
UCLASS()
class IRONFIELD_API UIFHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindPlayerStatBars(AIFPlayerCharacter* InPlayer = nullptr);

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
	void HandlePlayerRegistered(AIFPlayerCharacter* Player);

	UFUNCTION()
	void HandleStrongholdRegistered(AIFStronghold* Stronghold);

	UFUNCTION()
	void HandleStrongholdHealthChanged(float Percent);

	void BindStrongholdStatBar(AIFStronghold* Stronghold);
	void UnbindAllSources();
	void UnbindPlayerStatBars();

	TWeakObjectPtr<UIFHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UIFStaminaComponent> BoundPlayerStamina;
	TWeakObjectPtr<UIFHealthComponent> BoundStrongholdHealth;

	bool bPlayerBound = false;
	bool bStrongholdBound = false;
};
