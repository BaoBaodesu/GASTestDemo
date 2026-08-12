#include "UI/T_AIAwarenessWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UT_AIAwarenessWidget::UT_AIAwarenessWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> SuspicionIconAsset(
		TEXT("/Game/GASTestDemo/UI/Alert/SuspicionIcon.SuspicionIcon"));
	SuspicionIconTexture = SuspicionIconAsset.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> AlertIconAsset(
		TEXT("/Game/GASTestDemo/UI/Alert/AlertIcon.AlertIcon"));
	AlertIconTexture = AlertIconAsset.Object;
}

void UT_AIAwarenessWidget::UpdateAwareness(float InAwareness, ETGuardAIState InAIState, bool bHasVisualContact)
{
	const float ClampedAwareness = FMath::Clamp(InAwareness, 0.f, 100.f);
	const bool bFullyDetected = InAIState == ETGuardAIState::Combat;
	const ESlateVisibility WidgetVisibility = ClampedAwareness > 0.f
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed;
	if (GetVisibility() != WidgetVisibility) SetVisibility(WidgetVisibility);

	if (IsValid(ProgressBar_Awareness))
	{
		const float Progress = ClampedAwareness / 100.f;
		if (!FMath::IsNearlyEqual(ProgressBar_Awareness->GetPercent(), Progress)) ProgressBar_Awareness->SetPercent(Progress);
		const ESlateVisibility ProgressVisibility = bHasVisualContact && !bFullyDetected
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed;
		if (ProgressBar_Awareness->GetVisibility() != ProgressVisibility)
		{
			ProgressBar_Awareness->SetVisibility(ProgressVisibility);
		}
	}
	if (IsValid(Image_Awareness))
	{
		UTexture2D* Icon = bFullyDetected ? AlertIconTexture.Get() : SuspicionIconTexture.Get();
		if (IsValid(Icon) && Image_Awareness->GetBrush().GetResourceObject() != Icon)
		{
			Image_Awareness->SetBrushFromTexture(Icon, true);
		}
	}
}
