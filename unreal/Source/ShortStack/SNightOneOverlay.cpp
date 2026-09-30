#include "SNightOneOverlay.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
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
	const FSlateColor Accent = OverlayColor(0.02f, 0.65f, 0.55f);

	SAssignNew(IntroPanel, SBorder)
		.BorderImage(&Dim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(this, &SNightOneOverlay::IntroVisibility)
		[
			SNew(SBox)
			.WidthOverride(760.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("A FIRST-PERSON POKER RPG \u00B7 PROTOTYPE"))).Font(OverlayFont(TEXT("Bold"), 11)).ColorAndOpacity(Muted)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("SHORT STACK"))).Font(OverlayFont(TEXT("Bold"), 60)).ColorAndOpacity(Ink)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 24.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Night One"))).Font(OverlayFont(TEXT("Regular"), 22)).ColorAndOpacity(Accent)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 28.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("2:07 AM. Rain on the window. $2.37 in your RiverLine account and a final notice on the door.\nRent is due Friday.")))
					.Font(OverlayFont(TEXT("Regular"), 14))
					.ColorAndOpacity(Ink)
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 18.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Screen name"))).Font(OverlayFont(TEXT("Regular"), 12)).ColorAndOpacity(Muted)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(240.0f)
						[
							SAssignNew(NameBox, SEditableTextBox)
							.Font(OverlayFont(TEXT("Mono"), 14))
							.OnTextCommitted(this, &SNightOneOverlay::OnNameCommitted)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 28.0f)
				[
					SNew(SBox)
					.WidthOverride(220.0f)
					.HeightOverride(48.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ButtonColorAndOpacity(FLinearColor(0.02f, 0.55f, 0.48f, 1.0f))
						.OnClicked(this, &SNightOneOverlay::OnBeginClicked)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Begin"))).Font(OverlayFont(TEXT("Bold"), 16)).ColorAndOpacity(OverlayColor(0.0f, 0.02f, 0.02f))
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Mouse: play on the laptop    Space: lean back and look around    F C R A: fold, call, raise, all-in    Up/Down: bet size    M: mute")))
					.Font(OverlayFont(TEXT("Regular"), 10))
					.ColorAndOpacity(Muted)
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Headphones recommended. Everything you see and hear is generated in code: no photos, no samples.")))
					.Font(OverlayFont(TEXT("Regular"), 9))
					.ColorAndOpacity(OverlayColor(0.25f, 0.28f, 0.34f))
				]
			]
		];

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
						SNew(STextBlock).Text(FText::FromString(TEXT("MESSAGES \u00B7 NOW"))).Font(OverlayFont(TEXT("Bold"), 9)).ColorAndOpacity(Muted)
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
			.Text(FText::FromString(TEXT("Space: lean back    F C R A: fold / call / raise / all-in    Up/Down: bet size")))
			.Font(OverlayFont(TEXT("Regular"), 11))
			.ColorAndOpacity(OverlayColor(0.7f, 0.75f, 0.82f))
		];
	HintPanel->SetRenderOpacity(0.0f);

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[IntroPanel.ToSharedRef()]
		+ SOverlay::Slot()[ToastPanel.ToSharedRef()]
		+ SOverlay::Slot()[HintPanel.ToSharedRef()]
	];
	SetVisibility(EVisibility::SelfHitTestInvisible);
}

void SNightOneOverlay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Clock += InDeltaTime;
	if (bIntroHidden && IntroPanel)
	{
		IntroPanel->SetRenderOpacity(static_cast<float>(FMath::Clamp(1.0 - (Clock - IntroHiddenAt) / 1.4, 0.0, 1.0)));
	}
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

void SNightOneOverlay::SetName(const FString& Name)
{
	if (NameBox)
	{
		NameBox->SetText(FText::FromString(Name));
	}
}

void SNightOneOverlay::HideIntro()
{
	if (!bIntroHidden)
	{
		bIntroHidden = true;
		IntroHiddenAt = Clock;
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

FReply SNightOneOverlay::OnBeginClicked()
{
	if (!bIntroHidden && OnBegin)
	{
		OnBegin(NameBox ? NameBox->GetText().ToString() : FString());
	}
	return FReply::Handled();
}

void SNightOneOverlay::OnNameCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter && !bIntroHidden && OnBegin)
	{
		OnBegin(Text.ToString());
	}
}

EVisibility SNightOneOverlay::IntroVisibility() const
{
	if (!bIntroHidden)
	{
		return EVisibility::Visible;
	}
	return Clock - IntroHiddenAt < 1.4 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

EVisibility SNightOneOverlay::ToastVisibility() const
{
	return Clock - ToastAt < 8.0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}
