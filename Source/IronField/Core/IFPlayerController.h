#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IFPlayerController.generated.h"

class UUserWidget;

/**
 * Player controller class that manages the local player UI.
 * Handles HUD widget creation and initialization.
 */
UCLASS()
class IRONFIELD_API AIFPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    /** Blueprint class to instantiate as the player HUD on BeginPlay. */
    UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
    TSubclassOf<UUserWidget> HUDWidgetClass;

    /** The live HUD widget instance. Created at runtime from HUDWidgetClass. */
    UPROPERTY(VisibleInstanceOnly, Transient, Category = "PlayerController|UI")
    TObjectPtr<UUserWidget> HUDWidget;

    virtual void BeginPlay() override;

private:
    void CreateAndShowHUD();
};
