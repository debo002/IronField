#include "UI/IFLoseScreenWidget.h"

#include "Components/Button.h"
#include "Core/IFGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UIFLoseScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UIFLoseScreenWidget::HandleRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UIFLoseScreenWidget::HandleMainMenuClicked);
	}
}

void UIFLoseScreenWidget::UnpauseAndOpenLevel(FName LevelName)
{
	if (!GetWorld())
	{
		return;
	}

	UGameplayStatics::SetGamePaused(GetWorld(), false);
	UGameplayStatics::OpenLevel(this, LevelName);
}

void UIFLoseScreenWidget::HandleRestartClicked()
{
	UnpauseAndOpenLevel(FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void UIFLoseScreenWidget::HandleMainMenuClicked()
{
	UnpauseAndOpenLevel(UIFGameInstance::MainMenuLevelName);
}
