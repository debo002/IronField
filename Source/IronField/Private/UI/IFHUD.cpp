#include "UI/IFHUD.h"

#include "Building/IFStronghold.h"
#include "Character/IFPlayerCharacter.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerSubsystem.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Core/IFWaveManagerSubsystem.h"
#include "Engine/World.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"
#include "TimerManager.h"
#include "UI/IFStatBarWidget.h"
#include "Wave/IFWaveManager.h"

void UIFHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// Designer defaults can leave these visible; code owns their runtime state.
	if (BannerText)
	{
		BannerText->SetVisibility(ESlateVisibility::Hidden);
	}
	if (DamageFlash)
	{
		DamageFlash->SetRenderOpacity(0.f);
	}

	BindPlayerStatBars();

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!bPlayerBound)
	{
		if (UIFPlayerSubsystem* const PlayerSubsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			PlayerSubsystem->OnPlayerRegistered.AddDynamic(this, &UIFHUD::HandlePlayerRegistered);
		}
	}

	if (UIFStrongholdSubsystem* const StrongholdSubsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
	{
		// The HUD normally constructs before level actors begin play, so the stronghold
		// usually arrives through the registration delegate rather than the direct path.
		if (AIFStronghold* const Stronghold = StrongholdSubsystem->GetStronghold())
		{
			BindStrongholdStatBar(Stronghold);
		}
		else
		{
			StrongholdSubsystem->OnStrongholdRegistered.AddDynamic(this, &UIFHUD::HandleStrongholdRegistered);
		}
	}

	// Same dual-path pattern for waves: direct when the manager already registered,
	// delegate otherwise (covers HUD-constructed-first ordering).
	if (UIFWaveManagerSubsystem* const WaveSubsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
	{
		if (AIFWaveManager* const WaveManager = WaveSubsystem->GetWaveManager())
		{
			BindWaveManager(WaveManager);
		}
		else
		{
			WaveSubsystem->OnWaveManagerRegistered.AddDynamic(this, &UIFHUD::HandleWaveManagerRegistered);
		}
	}
}

void UIFHUD::NativeDestruct()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BannerTimerHandle);
		World->GetTimerManager().ClearTimer(DamageFlashTimerHandle);

		if (UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			Subsystem->OnStrongholdRegistered.RemoveDynamic(this, &UIFHUD::HandleStrongholdRegistered);
		}

		if (UIFPlayerSubsystem* const PlayerSubsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			PlayerSubsystem->OnPlayerRegistered.RemoveDynamic(this, &UIFHUD::HandlePlayerRegistered);
		}

		if (UIFWaveManagerSubsystem* const WaveSubsystem = World->GetSubsystem<UIFWaveManagerSubsystem>())
		{
			WaveSubsystem->OnWaveManagerRegistered.RemoveDynamic(this, &UIFHUD::HandleWaveManagerRegistered);
		}
	}

	UnbindWaveManager();
	UnbindAllSources();
	Super::NativeDestruct();
}

void UIFHUD::HandlePlayerRegistered(AIFPlayerCharacter* Player)
{
	BindPlayerStatBars(Player);
}

void UIFHUD::BindPlayerStatBars(AIFPlayerCharacter* InPlayer)
{
	if (!PlayerHealthBar || !PlayerStaminaBar)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] PlayerHealthBar or PlayerStaminaBar BindWidget is missing."));
		return;
	}

	AIFPlayerCharacter* Player = InPlayer;
	if (!Player)
	{
		Player = Cast<AIFPlayerCharacter>(GetOwningPlayerPawn());
	}
	if (!Player)
	{
		if (const UWorld* const World = GetWorld())
		{
			if (const UIFPlayerSubsystem* const Subsystem = World->GetSubsystem<UIFPlayerSubsystem>())
			{
				Player = Subsystem->GetPlayer();
			}
		}
	}

	if (!Player)
	{
		UE_LOG(LogIronField, Log, TEXT("[IF-HUD] Player pawn not yet available; awaiting registration/possession."));
		return;
	}

	UIFHealthComponent* const Health = Player->GetHealthComponent();
	UIFStaminaComponent* const Stamina = Player->GetStaminaComponent();
	if (!Health || !Stamina)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] Player %s is missing a health or stamina component."), *GetNameSafe(Player));
		return;
	}

	if (bPlayerBound && BoundPlayerHealth.Get() == Health && BoundPlayerStamina.Get() == Stamina)
	{
		return;
	}

	UnbindPlayerStatBars();

	Health->OnHealthChanged.AddDynamic(this, &UIFHUD::HandlePlayerHealthChanged);
	Stamina->OnStaminaChanged.AddDynamic(this, &UIFHUD::HandlePlayerStaminaChanged);

	BoundPlayerHealth = Health;
	BoundPlayerStamina = Stamina;
	bPlayerBound = true;
	LastPlayerHealthPercent = Health->GetHealthPercent();

	ApplyBarColors();
	PlayerHealthBar->SetTargetPercent(Health->GetHealthPercent());
	PlayerStaminaBar->SetTargetPercent(Stamina->GetStaminaPercent());
}

void UIFHUD::HandleStrongholdRegistered(AIFStronghold* Stronghold)
{
	BindStrongholdStatBar(Stronghold);
}

void UIFHUD::BindStrongholdStatBar(AIFStronghold* Stronghold)
{
	if (bStrongholdBound)
	{
		return;
	}

	if (!StrongholdHealthBar)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] StrongholdHealthBar BindWidget is missing."));
		return;
	}

	UIFHealthComponent* const Health = Stronghold ? Stronghold->GetHealthComponent() : nullptr;
	if (!Health)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] Stronghold %s has no health component."), *GetNameSafe(Stronghold));
		return;
	}

	bStrongholdBound = true;
	Health->OnHealthChanged.AddDynamic(this, &UIFHUD::HandleStrongholdHealthChanged);
	BoundStrongholdHealth = Health;
	ApplyBarColors();
	StrongholdHealthBar->SetTargetPercent(Health->GetHealthPercent());
}

void UIFHUD::HandleWaveManagerRegistered(AIFWaveManager* WaveManager)
{
	BindWaveManager(WaveManager);
}

void UIFHUD::BindWaveManager(AIFWaveManager* WaveManager)
{
	if (bWaveBound || !WaveManager)
	{
		return;
	}

	if (!InfoText)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] InfoText BindWidget is missing; wave counter will not update."));
	}
	if (!BannerText)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] BannerText BindWidget is missing; wave banners will not show."));
	}
	if (!DamageFlash)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] DamageFlash BindWidget is missing; damage flash will not show."));
	}

	WaveManager->OnWaveStarted.AddDynamic(this, &UIFHUD::HandleWaveStarted);
	WaveManager->OnEnemiesAliveCountChanged.AddDynamic(this, &UIFHUD::HandleEnemiesAliveChanged);
	WaveManager->OnKillCountChanged.AddDynamic(this, &UIFHUD::HandleKillCountChanged);
	WaveManager->OnWaveCompleted.AddDynamic(this, &UIFHUD::HandleWaveCompleted);
	WaveManager->OnWaveClearHeal.AddDynamic(this, &UIFHUD::HandleWaveClearHeal);
	BoundWaveManager = WaveManager;
	bWaveBound = true;

	// Push initial values so a late-binding HUD never shows stale designer text.
	// CurrentWaveIndex starts at -1 before the first wave, so clamp to 1 for display.
	const int32 InitialWave = FMath::Max(1, WaveManager->GetCurrentWave());
	HandleWaveStarted(InitialWave);
	HandleEnemiesAliveChanged(WaveManager->EnemiesAlive);
	HandleKillCountChanged(WaveManager->GetKillCount());

	// The initial push calls ShowBanner via HandleWaveStarted; hide it again when the
	// manager has not actually started a wave yet (HUD constructed first ordering).
	if (!WaveManager->bIsWaveActive && BannerText)
	{
		if (UWorld* const World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BannerTimerHandle);
		}
		HideBanner();
	}
}

void UIFHUD::UnbindWaveManager()
{
	if (AIFWaveManager* const WaveManager = BoundWaveManager.Get())
	{
		WaveManager->OnWaveStarted.RemoveDynamic(this, &UIFHUD::HandleWaveStarted);
		WaveManager->OnEnemiesAliveCountChanged.RemoveDynamic(this, &UIFHUD::HandleEnemiesAliveChanged);
		WaveManager->OnKillCountChanged.RemoveDynamic(this, &UIFHUD::HandleKillCountChanged);
		WaveManager->OnWaveCompleted.RemoveDynamic(this, &UIFHUD::HandleWaveCompleted);
		WaveManager->OnWaveClearHeal.RemoveDynamic(this, &UIFHUD::HandleWaveClearHeal);
	}
	BoundWaveManager = nullptr;
	bWaveBound = false;
}

void UIFHUD::HandleWaveStarted(int32 WaveNumber)
{
	CurrentWaveNumber = WaveNumber;
	UpdateInfoLine();
	ShowBanner(FString::Printf(TEXT("WAVE %d"), WaveNumber));
}

void UIFHUD::HandleEnemiesAliveChanged(int32 NewCount)
{
	CurrentEnemiesAlive = NewCount;
	UpdateInfoLine();
}

void UIFHUD::HandleKillCountChanged(int32 NewCount)
{
	CurrentKillCount = NewCount;
	UpdateInfoLine();
}

void UIFHUD::UpdateInfoLine()
{
	if (!InfoText)
	{
		return;
	}

	InfoText->SetText(FText::FromString(
		FString::Printf(TEXT("WAVE %d  |  LEFT %d  |  KILLS %d"), CurrentWaveNumber, CurrentEnemiesAlive, CurrentKillCount)));
}

void UIFHUD::ApplyBarColors()
{
	// Theme colors live here so the designer never picks bar colors.
	// Hex: HP #B8332A, Stamina #4FA3E3, Stronghold #C9A227.
	if (PlayerHealthBar)
	{
		PlayerHealthBar->SetFillColor(FLinearColor(FColor(0xB8, 0x33, 0x2A)));
	}
	if (PlayerStaminaBar)
	{
		PlayerStaminaBar->SetFillColor(FLinearColor(FColor(0x4F, 0xA3, 0xE3)));
	}
	if (StrongholdHealthBar)
	{
		StrongholdHealthBar->SetFillColor(FLinearColor(FColor(0xC9, 0xA2, 0x27)));
	}
}

void UIFHUD::ShowBanner(const FString& BannerString)
{
	if (!BannerText)
	{
		return;
	}

	BannerText->SetText(FText::FromString(BannerString));
	BannerText->SetVisibility(ESlateVisibility::Visible);

	// Timer hide owns the banner lifecycle with or without a designer fade.
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BannerTimerHandle);
		World->GetTimerManager().SetTimer(BannerTimerHandle, this, &UIFHUD::HideBanner, 3.f, false);
	}
}

void UIFHUD::HideBanner()
{
	if (BannerText)
	{
		BannerText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UIFHUD::FlashDamage()
{
	if (!DamageFlash)
	{
		return;
	}

	DamageFlash->SetRenderOpacity(0.45f);

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageFlashTimerHandle);
		World->GetTimerManager().SetTimer(DamageFlashTimerHandle, this, &UIFHUD::HideDamageFlash, 0.3f, false);
	}
}

void UIFHUD::HideDamageFlash()
{
	if (DamageFlash)
	{
		DamageFlash->SetRenderOpacity(0.f);
	}
}

void UIFHUD::UnbindPlayerStatBars()
{
	if (UIFHealthComponent* const Health = BoundPlayerHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UIFHUD::HandlePlayerHealthChanged);
	}
	BoundPlayerHealth = nullptr;

	if (UIFStaminaComponent* const Stamina = BoundPlayerStamina.Get())
	{
		Stamina->OnStaminaChanged.RemoveDynamic(this, &UIFHUD::HandlePlayerStaminaChanged);
	}
	BoundPlayerStamina = nullptr;
	bPlayerBound = false;
}

void UIFHUD::UnbindAllSources()
{
	UnbindPlayerStatBars();

	if (UIFHealthComponent* const Health = BoundStrongholdHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UIFHUD::HandleStrongholdHealthChanged);
	}
	BoundStrongholdHealth = nullptr;
	bStrongholdBound = false;
}

void UIFHUD::HandlePlayerHealthChanged(float Percent)
{
	if (PlayerHealthBar)
	{
		PlayerHealthBar->SetTargetPercent(Percent);
	}
	if (Percent < LastPlayerHealthPercent)
	{
		FlashDamage();
	}
	LastPlayerHealthPercent = Percent;
}

void UIFHUD::HandlePlayerStaminaChanged(float Percent)
{
	if (PlayerStaminaBar)
	{
		PlayerStaminaBar->SetTargetPercent(Percent);
	}
}

void UIFHUD::HandleStrongholdHealthChanged(float Percent)
{
	if (StrongholdHealthBar)
	{
		StrongholdHealthBar->SetTargetPercent(Percent);
	}
}

void UIFHUD::HandleWaveCompleted(int32 WaveNumber)
{
	// Banner with exact amounts arrives via OnWaveClearHeal below (broadcast by the
	// manager in the same frame). This handler only keeps the info line in sync.
	CurrentWaveNumber = WaveNumber;
	UpdateInfoLine();
}

void UIFHUD::HandleWaveClearHeal(int32 WaveNumber, float PlayerHealed, float GateRepaired)
{
	CurrentWaveNumber = WaveNumber;
	UpdateInfoLine();

	if (PlayerHealed > 0.f || GateRepaired > 0.f)
	{
		ShowBanner(FString::Printf(TEXT("WAVE %d CLEARED  +%.0f HP  /  GATE +%.0f"), WaveNumber, PlayerHealed, GateRepaired));
	}
	else
	{
		ShowBanner(FString::Printf(TEXT("WAVE %d CLEARED"), WaveNumber));
	}
}
