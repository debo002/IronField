#include "IFPlayerController.h"
#include "Blueprint/UserWidget.h"

void AIFPlayerController::BeginPlay()
{
    Super::BeginPlay();

    CreateAndShowHUD();
}

void AIFPlayerController::CreateAndShowHUD()
{
    if (!HUDWidgetClass)
    {
        return;
    }

    HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
    if (HUDWidget)
    {
        HUDWidget->AddToViewport();
    }
}
