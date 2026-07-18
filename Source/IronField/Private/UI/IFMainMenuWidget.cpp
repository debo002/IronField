#include "UI/IFMainMenuWidget.h"

#include "Components/Button.h"
#include "Core/IFGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UIFMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NormalModeButton)
	{
		NormalModeButton->OnClicked.AddDynamic(this, &UIFMainMenuWidget::HandleNormalModeClicked);
	}

	if (UnlimitedModeButton)
	{
		UnlimitedModeButton->OnClicked.AddDynamic(this, &UIFMainMenuWidget::HandleUnlimitedModeClicked);
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
	if (!GetWorld())
	{
		return;
	}

	if (UIFGameInstance* const GameInstance = Cast<UIFGameInstance>(GetGameInstance()))
	{
		GameInstance->SetRunMode(RunMode);
	}

	UGameplayStatics::OpenLevel(this, UIFGameInstance::GameplayLevelName);
}
