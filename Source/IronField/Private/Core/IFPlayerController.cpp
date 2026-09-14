#include "Core/IFPlayerController.h"

#include "Character/IFPlayerCharacter.h"
#include "Core/IFPlayerControllerUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UI/IFGameOverScreenWidget.h"
#include "UI/IFHUD.h"
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

	GameOverScreenWidget = CreateWidget<UIFGameOverScreenWidget>(this, GameOverScreenWidgetClass);
	if (!GameOverScreenWidget)
	{
		return;
	}

	GameOverScreenWidget->SetResult(Result);
	IFPlayerControllerUtils::FocusWidgetWithUIOnlyInput(this, GameOverScreenWidget);

	UGameplayStatics::SetGamePaused(this, true);
}
