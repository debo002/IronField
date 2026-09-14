#include "UI/IFMainMenuWidget.h"

#include "Components/Button.h"
#include "Core/IFGameInstance.h"
#include "Core/IFLog.h"
#include "Kismet/GameplayStatics.h"

void UIFMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NormalModeButton)
	{
		NormalModeButton->OnClicked.AddDynamic(this, &UIFMainMenuWidget::HandleNormalModeClicked);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] NormalModeButton BindWidget is missing; normal mode cannot be started from the menu."));
	}

	if (UnlimitedModeButton)
	{
		UnlimitedModeButton->OnClicked.AddDynamic(this, &UIFMainMenuWidget::HandleUnlimitedModeClicked);
	}
	else
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] UnlimitedModeButton BindWidget is missing; unlimited mode cannot be started from the menu."));
	}
}

void UIFMainMenuWidget::HandleNormalModeClicked()
{
	StartRunAndOpenGameplayLevel(EIFRunMode::Normal);
}

void UIFMainMenuWidget::HandleUnlimitedModeClicked()
{
	StartRunAndOpenGameplayLevel(EIFRunMode::Unlimited);
}

void UIFMainMenuWidget::StartRunAndOpenGameplayLevel(EIFRunMode RunMode)
{
	UIFGameInstance* const GameInstance = Cast<UIFGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] Main menu has no IFGameInstance; the run cannot start."));
		return;
	}

	GameInstance->SetRunMode(RunMode);
	UGameplayStatics::OpenLevel(this, GameInstance->GameplayLevelName);
}
