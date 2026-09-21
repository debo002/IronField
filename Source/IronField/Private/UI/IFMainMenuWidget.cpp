#include "UI/IFMainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/IFBestRunSave.h"
#include "Core/IFGameInstance.h"
#include "Core/IFLog.h"
#include "Kismet/GameplayStatics.h"

void UIFMainMenuWidget::SetBestText(const FText& InBestText)
{
	if (!BestText)
	{
		UE_LOG(LogIronField, Warning, TEXT("[IF-UI] BestText BindWidget is missing; best-run line will not update."));
		return;
	}

	BestText->SetText(InBestText);
}

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

	// Single sync slot; missing slot shows the empty combined line.
	const FString SlotName = UIFBestRunSave::GetSlotName();
	const UIFBestRunSave* const Save = Cast<UIFBestRunSave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save)
	{
		const UIFBestRunSave* const DefaultSave = NewObject<UIFBestRunSave>(this);
		SetBestText(DefaultSave ? DefaultSave->BuildBestText() : FText::FromString(TEXT("BEST: \u2014")));
		return;
	}

	SetBestText(Save->BuildBestText());
}

void UIFMainMenuWidget::NativeDestruct()
{
	if (NormalModeButton)
	{
		NormalModeButton->OnClicked.RemoveAll(this);
	}

	if (UnlimitedModeButton)
	{
		UnlimitedModeButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
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
