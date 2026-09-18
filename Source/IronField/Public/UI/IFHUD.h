#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "IFHUD.generated.h"

class AIFStronghold;
class AIFPlayerCharacter;
class AIFWaveManager;
class UIFStatBarWidget;
class UIFHealthComponent;
class UIFStaminaComponent;
class UTextBlock;
class UBorder;

/**
 * Gameplay HUD root. Binds player and stronghold stat delegates and forwards
 * percentages to child UIFStatBarWidget instances. Shows one info line
 * (wave | left | kills), a wave-start banner, and a damage flash.
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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BannerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> DamageFlash;

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

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber);

	UFUNCTION()
	void HandleEnemiesAliveChanged(int32 NewCount);

	UFUNCTION()
	void HandleKillCountChanged(int32 NewCount);

	UFUNCTION()
	void HandleWaveManagerRegistered(AIFWaveManager* WaveManager);

	UFUNCTION()
	void HideBanner();

	UFUNCTION()
	void HideDamageFlash();

	void BindStrongholdStatBar(AIFStronghold* Stronghold);
	void BindWaveManager(AIFWaveManager* WaveManager);
	void UnbindWaveManager();
	void UnbindAllSources();
	void UnbindPlayerStatBars();
	void ShowBanner(const FString& BannerString);
	void FlashDamage();
	void UpdateInfoLine();
	void ApplyBarColors();

	TWeakObjectPtr<UIFHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UIFStaminaComponent> BoundPlayerStamina;
	TWeakObjectPtr<UIFHealthComponent> BoundStrongholdHealth;
	TWeakObjectPtr<AIFWaveManager> BoundWaveManager;

	FTimerHandle BannerTimerHandle;
	FTimerHandle DamageFlashTimerHandle;

	float LastPlayerHealthPercent = 1.f;

	int32 CurrentWaveNumber = 1;
	int32 CurrentEnemiesAlive = 0;
	int32 CurrentKillCount = 0;

	bool bPlayerBound = false;
	bool bStrongholdBound = false;
	bool bWaveBound = false;
};
