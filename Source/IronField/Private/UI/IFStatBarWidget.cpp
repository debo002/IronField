#include "UI/IFStatBarWidget.h"

#include "Components/ProgressBar.h"

UIFStatBarWidget::UIFStatBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Pure C++ NativeTick is not enabled unless this flag is set (or BP implements Tick).
	bHasScriptImplementedTick = true;
}

void UIFStatBarWidget::SetTargetPercent(float NewTargetPercent)
{
	TargetPercent = FMath::Clamp(NewTargetPercent, 0.f, 1.f);

	if (!bHasReceivedTarget)
	{
		bHasReceivedTarget = true;
		CurrentPercent = TargetPercent;
		if (ProgressBar)
		{
			ProgressBar->SetPercent(CurrentPercent);
		}
	}
}

void UIFStatBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ProgressBar)
	{
		return;
	}

	ProgressBar->SetFillColorAndOpacity(FillColor);
	ProgressBar->SetPercent(CurrentPercent);
}

void UIFStatBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ProgressBar)
	{
		return;
	}

	// Same formula as WBP_HUD InterpAndSetBarPercent: FInterpTo(Current, Target, DeltaTime, InterpSpeed).
	CurrentPercent = FMath::FInterpTo(CurrentPercent, TargetPercent, InDeltaTime, InterpSpeed);
	ProgressBar->SetPercent(CurrentPercent);
}
