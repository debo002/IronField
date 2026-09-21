#include "UI/IFPauseMenuWidget.h"

#include "Components/Button.h"
#include "Core/IFLog.h"
#include "Core/IFPlayerController.h"

void UIFPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UIFPauseMenuWidget::HandleResumeClicked);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] Pause ResumeButton BindWidget is missing."));
	}

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UIFPauseMenuWidget::HandleRestartClicked);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] Pause RestartButton BindWidget is missing."));
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UIFPauseMenuWidget::HandleMainMenuClicked);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] Pause MainMenuButton BindWidget is missing."));
	}
}

void UIFPauseMenuWidget::NativeDestruct()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveAll(this);
	}

	if (RestartButton)
	{
		RestartButton->OnClicked.RemoveAll(this);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UIFPauseMenuWidget::HandleResumeClicked()
{
	AIFPlayerController* const Controller = Cast<AIFPlayerController>(GetOwningPlayer());
	if (!Controller)
	{
		return;
	}

	Controller->ResumeGame();
}

void UIFPauseMenuWidget::HandleRestartClicked()
{
	AIFPlayerController* const Controller = Cast<AIFPlayerController>(GetOwningPlayer());
	if (!Controller)
	{
		return;
	}

	Controller->RestartLevel();
}

void UIFPauseMenuWidget::HandleMainMenuClicked()
{
	AIFPlayerController* const Controller = Cast<AIFPlayerController>(GetOwningPlayer());
	if (!Controller)
	{
		return;
	}

	Controller->ReturnToMainMenu();
}
