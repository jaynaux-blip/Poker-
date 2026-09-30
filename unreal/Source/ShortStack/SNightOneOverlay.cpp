#include "SNightOneOverlay.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace NightOneOverlayDetail
{
FSlateFontInfo OverlayFont(const TCHAR* Face, int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(FName(Face), Size);
}

FSlateColor OverlayColor(float R, float G, float B, float A = 1.0f)
{
	return FSlateColor(FLinearColor(R, G, B, A));
}
} // namespace NightOneOverlayDetail

using namespace NightOneOverlayDetail;

void SNightOneOverlay::Construct(const FArguments& InArgs)
{
	const FSlateColor Ink = OverlayColor(0.8f, 0.84f, 0.9f);
	const FSlateColor Muted = OverlayColor(0.35f, 0.4f, 0.5f);

	SAssignNew(ToastPanel, SBox)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 24.0f, 24.0f, 0.0f))
		.Visibility(this, &SNightOneOverlay::ToastVisibility)
		[
			SNew(SBox)
			.WidthOverride(360.0f)
			[
				SNew(SBorder)
				.BorderImage(&ToastBrush)
				.Padding(FMargin(16.0f, 12.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("MESSAGES · NOW"))).Font(OverlayFont(TEXT("Bold"), 9)).ColorAndOpacity(Muted)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 2.0f)
					[
						SNew(STextBlock).Text(this, &SNightOneOverlay::ToastFrom).Font(OverlayFont(TEXT("Bold"), 13)).ColorAndOpacity(Ink)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(this, &SNightOneOverlay::ToastBody).Font(OverlayFont(TEXT("Regular"), 12)).ColorAndOpacity(Ink).AutoWrapText(true)
					]
				]
			]
		];

	SAssignNew(HintPanel, SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 28.0f))
		.Visibility(EVisibility::HitTestInvisible)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Space: lean back    F C R A: fold / call / raise / all-in    Up/Down: bet size    P / Esc: pause")))
			.Font(OverlayFont(TEXT("Regular"), 11))
			.ColorAndOpacity(OverlayColor(0.7f, 0.75f, 0.82f))
		];
	HintPanel->SetRenderOpacity(0.0f);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[ToastPanel.ToSharedRef()]
		+ SOverlay::Slot()[HintPanel.ToSharedRef()]
	];
	SetVisibility(EVisibility::SelfHitTestInvisible);
}

void SNightOneOverlay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Clock += InDeltaTime;
	if (ToastPanel)
	{
		const double Age = Clock - ToastAt;
		ToastPanel->SetRenderOpacity(static_cast<float>(FMath::Clamp(FMath::Min(Age * 4.0, (8.0 - Age) * 2.0), 0.0, 1.0)));
	}
	if (HintPanel)
	{
		const double Age = Clock - HintAt;
		HintPanel->SetRenderOpacity(static_cast<float>(FMath::Clamp(FMath::Min(Age, (14.0 - Age)), 0.0, 1.0)));
	}
}

void SNightOneOverlay::ShowToast(const FString& From, const FString& Body)
{
	ToastFromText = From;
	ToastBodyText = Body;
	ToastAt = Clock;
}

void SNightOneOverlay::ShowHint()
{
	HintAt = Clock;
}

EVisibility SNightOneOverlay::ToastVisibility() const
{
	return Clock - ToastAt < 8.0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}
