#include "DeeRoomCaption.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace DeeRoomCaptionDetail
{
FSlateFontInfo CaptionFont(const TCHAR* Face, int32 Size)
{
	return FCoreStyle::GetDefaultFontStyle(FName(Face), Size);
}
} // namespace DeeRoomCaptionDetail

using namespace DeeRoomCaptionDetail;

void SDeeRoomCaption::Construct(const FArguments& InArgs)
{
	// Over the controls hint (SNightOneOverlay keeps the bottom 28 px), as wide as a line should read.
	SAssignNew(Panel, SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 92.0f))
		.Visibility(this, &SDeeRoomCaption::CaptionVisibility)
		[
			SNew(SBox)
			.MaxDesiredWidth(760.0f)
			[
				SNew(SBorder)
				.BorderImage(&Brush)
				.Padding(FMargin(18.0f, 10.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SDeeRoomCaption::Speaker)
						.Visibility(this, &SDeeRoomCaption::SpeakerVisibility)
						.Font(CaptionFont(TEXT("Bold"), 10))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.66f, 0.3f)))
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SDeeRoomCaption::Line)
						.Font(this, &SDeeRoomCaption::LineFont)
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.88f, 0.92f)))
					]
				]
			]
		];
	Panel->SetRenderOpacity(0.0f);
	ChildSlot
	[
		Panel.ToSharedRef()
	];
	SetVisibility(EVisibility::SelfHitTestInvisible);
}

void SDeeRoomCaption::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Clock += InDeltaTime;
	if (Panel)
	{
		const double Age = Clock - ShownAt;
		Panel->SetRenderOpacity(static_cast<float>(FMath::Clamp(FMath::Min(Age * 5.0, (Duration - Age) * 1.5), 0.0, 1.0)));
	}
}

void SDeeRoomCaption::Show(const FString& Speaker, const FString& Line)
{
	SpeakerText = Speaker;
	LineText = Line;
	ShownAt = Clock;
	// Long enough to read twice at an easy pace.
	Duration = FMath::Clamp(2.6 + 0.065 * Line.Len(), 3.5, 11.0);
}

EVisibility SDeeRoomCaption::CaptionVisibility() const
{
	return Clock - ShownAt < Duration ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

FSlateFontInfo SDeeRoomCaption::LineFont() const
{
	return CaptionFont(SpeakerText.IsEmpty() ? TEXT("Italic") : TEXT("Regular"), 14);
}
