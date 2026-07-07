#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IFPlayerController.generated.h"

class UUserWidget;

UCLASS()
class IRONFIELD_API AIFPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    UPROPERTY(EditDefaultsOnly, Category = "PlayerController|UI")
    TSubclassOf<UUserWidget> HUDWidgetClass;

    UPROPERTY(VisibleInstanceOnly, Transient, Category = "PlayerController|UI")
    TObjectPtr<UUserWidget> HUDWidget;

    virtual void BeginPlay() override;

private:
    void CreateAndShowHUD();
};
