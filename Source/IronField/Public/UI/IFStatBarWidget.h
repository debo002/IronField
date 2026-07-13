#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFStatBarWidget.generated.h"

class UProgressBar;

/**
 * Single progress bar that smoothly interpolates toward a target percent.
 * Call SetTargetPercent from the owning HUD; do not drive this widget from Tick outside.
 */
UCLASS()
class IRONFIELD_API UIFStatBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UIFStatBarWidget(const FObjectInitializer& ObjectInitializer);

	/** Updates the interpolation target only. Displayed fill is advanced in NativeTick. */
	UFUNCTION(BlueprintCallable, Category = "IronField|StatBar")
	void SetTargetPercent(float NewTargetPercent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|StatBar")
	FLinearColor FillColor = FLinearColor::White;

	// Matches WBP_HUD's InterpAndSetBarPercent (FInterpTo) speed parameter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|StatBar", meta = (ClampMin = "0.0"))
	float InterpSpeed = 8.f;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	float TargetPercent = 0.f;
	float CurrentPercent = 0.f;
	bool bHasReceivedTarget = false;
};
