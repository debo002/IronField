#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFLoseScreenWidget.generated.h"

class UButton;

UCLASS()
class IRONFIELD_API UIFLoseScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	void UnpauseAndOpenLevel(FName LevelName);
};
