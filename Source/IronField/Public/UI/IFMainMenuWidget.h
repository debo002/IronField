#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/IFGameInstance.h"
#include "IFMainMenuWidget.generated.h"

class UButton;

UCLASS()
class IRONFIELD_API UIFMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NormalModeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UnlimitedModeButton;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleNormalModeClicked();

	UFUNCTION()
	void HandleUnlimitedModeClicked();

private:
	void StartRunAndOpenGameplayLevel(EIFRunMode RunMode);
};
