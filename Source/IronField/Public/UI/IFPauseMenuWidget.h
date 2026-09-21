#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFPauseMenuWidget.generated.h"

class UButton;

/** Pause root. Blueprint needs ResumeButton, RestartButton, MainMenuButton. */
UCLASS()
class IRONFIELD_API UIFPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleMainMenuClicked();
};
