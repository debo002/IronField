#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFMainMenuWidget.generated.h"

class UButton;
enum class EIFRunMode : uint8;

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
