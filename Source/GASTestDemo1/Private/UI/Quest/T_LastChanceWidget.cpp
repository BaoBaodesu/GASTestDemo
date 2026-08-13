#include "UI/Quest/T_LastChanceWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UT_LastChanceWidget::RebuildWidget()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("最后机会")))
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FLinearColor::White)
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
			.ShadowOffset(FVector2D(2.f, 2.f))
			.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 48))
		];
}
