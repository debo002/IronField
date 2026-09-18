#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
enum class EIFRunMode : uint8;

UCLASS()
class IRONFIELD_API UIFMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetBestText(const FText& InBestText);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NormalModeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> UnlimitedModeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BestText;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleNormalModeClicked();

	UFUNCTION()
	void HandleUnlimitedModeClicked();

private:
	void StartRunAndOpenGameplayLevel(EIFRunMode RunMode);
};
