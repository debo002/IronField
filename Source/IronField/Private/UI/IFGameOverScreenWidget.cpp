#include "UI/IFGameOverScreenWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/IFGameInstance.h"
#include "Core/IFLog.h"
#include "Kismet/GameplayStatics.h"

void UIFGameOverScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UIFGameOverScreenWidget::HandleRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UIFGameOverScreenWidget::HandleMainMenuClicked);
	}

	ApplyResultTitle();
}

void UIFGameOverScreenWidget::ApplyResultTitle()
{
	if (!ResultTitleText)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] ResultTitleText BindWidget is missing; the result title will not update."));
		return;
	}

	const bool bVictory = Result == EIFGameResult::Victory;
	ResultTitleText->SetText(bVictory ? VictoryText : DefeatText);
	ResultTitleText->SetColorAndOpacity(bVictory ? VictoryColor : DefeatColor);
}

void UIFGameOverScreenWidget::UnpauseAndOpenLevel(FName LevelName)
{
	if (!GetWorld())
	{
		return;
	}

	UGameplayStatics::SetGamePaused(GetWorld(), false);
	UGameplayStatics::OpenLevel(this, LevelName);
}

void UIFGameOverScreenWidget::HandleRestartClicked()
{
	UnpauseAndOpenLevel(FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void UIFGameOverScreenWidget::HandleMainMenuClicked()
{
	const UIFGameInstance* const GameInstance = Cast<UIFGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] Game over screen has no IFGameInstance; cannot return to the main menu."));
		return;
	}

	UnpauseAndOpenLevel(GameInstance->MainMenuLevelName);
}
