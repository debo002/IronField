#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IFStatBarWidget.generated.h"

class UProgressBar;

/**
 * Single progress bar that smoothly interpolates toward a target percent.
 * Driven exclusively by the owning HUD through SetTargetPercent.
 */
UCLASS()
class IRONFIELD_API UIFStatBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UIFStatBarWidget(const FObjectInitializer& ObjectInitializer);

	/** Updates the interpolation target only; the displayed fill advances in NativeTick. */
	UFUNCTION(BlueprintCallable, Category = "IronField|UI|StatBar")
	void SetTargetPercent(float NewTargetPercent);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|UI|StatBar")
	FLinearColor FillColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IronField|UI|StatBar", meta = (ClampMin = "0.0"))
	float InterpSpeed = 8.f;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	float TargetPercent = 0.f;
	float CurrentPercent = 0.f;
	bool bHasReceivedTarget = false;
};
