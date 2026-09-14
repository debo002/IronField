#include "UI/IFHUD.h"

#include "Building/IFStronghold.h"
#include "Character/IFPlayerCharacter.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerSubsystem.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Engine/World.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"
#include "UI/IFStatBarWidget.h"

void UIFHUD::NativeConstruct()
{
	Super::NativeConstruct();

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

	UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	// The HUD normally constructs before level actors begin play, so the stronghold
	// usually arrives through the registration delegate rather than the direct path.
	if (AIFStronghold* const Stronghold = Subsystem->GetStronghold())
	{
		BindStrongholdStatBar(Stronghold);
	}
	else
	{
		Subsystem->OnStrongholdRegistered.AddDynamic(this, &UIFHUD::HandleStrongholdRegistered);
	}
}

void UIFHUD::NativeDestruct()
{
	if (UWorld* const World = GetWorld())
	{
		if (UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>())
		{
			Subsystem->OnStrongholdRegistered.RemoveDynamic(this, &UIFHUD::HandleStrongholdRegistered);
		}

		if (UIFPlayerSubsystem* const PlayerSubsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			PlayerSubsystem->OnPlayerRegistered.RemoveDynamic(this, &UIFHUD::HandlePlayerRegistered);
		}
	}

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
	StrongholdHealthBar->SetTargetPercent(Health->GetHealthPercent());
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
