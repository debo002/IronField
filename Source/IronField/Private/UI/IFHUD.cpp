#include "UI/IFHUD.h"

#include "Building/IFStronghold.h"
#include "Character/IFBaseCharacter.h"
#include "Core/IFLog.h"
#include "Core/IFStrongholdSubsystem.h"
#include "Engine/World.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"
#include "TimerManager.h"
#include "UI/IFStatBarWidget.h"

void UIFHUD::NativeConstruct()
{
	Super::NativeConstruct();

	BindPlayerStatBars();

	// Stronghold registers during its own BeginPlay; HUD may construct first from the player controller.
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UIFHUD::BindStrongholdStatBar));
	}
}

void UIFHUD::NativeDestruct()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	UnbindAllSources();
	Super::NativeDestruct();
}

void UIFHUD::BindPlayerStatBars()
{
	if (!PlayerHealthBar || !PlayerStaminaBar)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] PlayerHealthBar or PlayerStaminaBar BindWidget is missing."));
		return;
	}

	AIFBaseCharacter* const Player = Cast<AIFBaseCharacter>(GetOwningPlayerPawn());
	if (!Player)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] No owning player pawn (AIFBaseCharacter); player bars not bound."));
		return;
	}

	UIFHealthComponent* const Health = Player->GetHealthComponent();
	if (!Health)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] Player %s has no health component."), *GetNameSafe(Player));
		return;
	}

	UIFStaminaComponent* const Stamina = Player->GetStaminaComponent();
	if (!Stamina)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] Player %s has no stamina component."), *GetNameSafe(Player));
		return;
	}

	Health->OnHealthChanged.AddDynamic(this, &UIFHUD::HandlePlayerHealthChanged);
	Stamina->OnStaminaChanged.AddDynamic(this, &UIFHUD::HandlePlayerStaminaChanged);

	BoundPlayerHealth = Health;
	BoundPlayerStamina = Stamina;

	PlayerHealthBar->SetTargetPercent(Health->GetHealthPercent());
	PlayerStaminaBar->SetTargetPercent(Stamina->GetStaminaPercent());
}

void UIFHUD::BindStrongholdStatBar()
{
	if (!StrongholdHealthBar)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] StrongholdHealthBar BindWidget is missing."));
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] No world; stronghold bar not bound."));
		return;
	}

	const UIFStrongholdSubsystem* const Subsystem = World->GetSubsystem<UIFStrongholdSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] UIFStrongholdSubsystem unavailable; stronghold bar not bound."));
		return;
	}

	AIFStronghold* const Stronghold = Subsystem->GetStronghold();
	if (!Stronghold)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] No stronghold registered; stronghold bar not bound."));
		return;
	}

	UIFHealthComponent* const Health = Stronghold->GetHealthComponent();
	if (!Health)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-HUD] Stronghold %s has no health component."), *GetNameSafe(Stronghold));
		return;
	}

	Health->OnHealthChanged.AddDynamic(this, &UIFHUD::HandleStrongholdHealthChanged);
	BoundStrongholdHealth = Health;

	StrongholdHealthBar->SetTargetPercent(Health->GetHealthPercent());
}

void UIFHUD::UnbindAllSources()
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

	if (UIFHealthComponent* const Health = BoundStrongholdHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UIFHUD::HandleStrongholdHealthChanged);
	}
	BoundStrongholdHealth = nullptr;
}

void UIFHUD::SetBarPercent(UIFStatBarWidget* Bar, float Percent)
{
	if (Bar)
	{
		Bar->SetTargetPercent(Percent);
	}
}

void UIFHUD::HandlePlayerHealthChanged(float Percent)
{
	SetBarPercent(PlayerHealthBar, Percent);
}

void UIFHUD::HandlePlayerStaminaChanged(float Percent)
{
	SetBarPercent(PlayerStaminaBar, Percent);
}

void UIFHUD::HandleStrongholdHealthChanged(float Percent)
{
	SetBarPercent(StrongholdHealthBar, Percent);
}
