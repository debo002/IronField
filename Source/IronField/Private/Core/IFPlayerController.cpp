#include "Core/IFPlayerController.h"

#include "Character/IFPlayerCharacter.h"
#include "Core/IFGameInstance.h"
#include "Core/IFPlayerControllerUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UI/IFGameOverScreenWidget.h"
#include "UI/IFHUD.h"
#include "UI/IFPauseMenuWidget.h"
#include "UObject/ConstructorHelpers.h"

AIFPlayerController::AIFPlayerController()
{
	static ConstructorHelpers::FClassFinder<UIFHUD> HUDFinder(TEXT("/Game/IronField/UI/WBP_HUD"));
	if (HUDFinder.Succeeded())
	{
		HUDWidgetClass = HUDFinder.Class;
	}
}

void AIFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyGameplayInputMode();
	CreateAndShowHUD();
}

void AIFPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (HUDWidget)
	{
		HUDWidget->BindPlayerStatBars(Cast<AIFPlayerCharacter>(InPawn));
	}
}

void AIFPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	if (GameOverScreenWidget)
	{
		GameOverScreenWidget->RemoveFromParent();
		GameOverScreenWidget = nullptr;
	}

	ClosePauseMenu();

	Super::EndPlay(EndPlayReason);
}

void AIFPlayerController::CreateAndShowHUD()
{
	if (!HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UIFHUD>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void AIFPlayerController::ApplyGameplayInputMode()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	FlushPressedKeys();
}

void AIFPlayerController::ShowGameOverScreen(EIFGameResult Result)
{
	if (!GameOverScreenWidgetClass || GameOverScreenWidget)
	{
		return;
	}

	// Game over owns the pause; never stack it on the pause menu.
	ClosePauseMenu();

	GameOverScreenWidget = CreateWidget<UIFGameOverScreenWidget>(this, GameOverScreenWidgetClass);
	if (!GameOverScreenWidget)
	{
		return;
	}

	GameOverScreenWidget->SetResult(Result);
	IFPlayerControllerUtils::FocusWidgetWithUIOnlyInput(this, GameOverScreenWidget);

	UGameplayStatics::SetGamePaused(this, true);
}

void AIFPlayerController::TogglePauseGame()
{
	if (IsGameOverActive())
	{
		return;
	}

	if (IsPauseMenuOpen())
	{
		ResumeGame();
		return;
	}

	ShowPauseMenu();
}

void AIFPlayerController::ResumeGame()
{
	if (IsGameOverActive())
	{
		return;
	}

	ClosePauseMenu();
	UGameplayStatics::SetGamePaused(this, false);
	ApplyGameplayInputMode();
}

void AIFPlayerController::RestartLevel()
{
	const FName CurrentLevel = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	OpenLevelUnpaused(CurrentLevel);
}

void AIFPlayerController::ReturnToMainMenu()
{
	UIFGameInstance* const GameInstance = Cast<UIFGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		return;
	}

	OpenLevelUnpaused(GameInstance->MainMenuLevelName);
}

void AIFPlayerController::ShowPauseMenu()
{
	if (IsGameOverActive() || IsPauseMenuOpen())
	{
		return;
	}

	if (!PauseMenuWidgetClass)
	{
		return;
	}

	PauseMenuWidget = CreateWidget<UIFPauseMenuWidget>(this, PauseMenuWidgetClass);
	if (!PauseMenuWidget)
	{
		return;
	}

	IFPlayerControllerUtils::FocusWidgetWithUIOnlyInput(this, PauseMenuWidget);
	UGameplayStatics::SetGamePaused(this, true);
}

void AIFPlayerController::ClosePauseMenu()
{
	if (!PauseMenuWidget)
	{
		return;
	}

	PauseMenuWidget->RemoveFromParent();
	PauseMenuWidget = nullptr;
}

void AIFPlayerController::OpenLevelUnpaused(FName LevelName)
{
	ClosePauseMenu();
	IFPlayerControllerUtils::OpenLevelUnpaused(this, LevelName);
}
