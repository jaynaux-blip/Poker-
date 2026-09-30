#pragma once

#include "Brushes/SlateColorBrush.h"
#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Viewport overlay during play: phone notifications and the controls hint.
 * The title screen and menus are drawn by SFrontEndWidget.
 */
class SHORTSTACK_API SNightOneOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNightOneOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void ShowToast(const FString& From, const FString& Body);
	void ShowHint();

private:
	EVisibility ToastVisibility() const;
	FText ToastFrom() const { return FText::FromString(ToastFromText); }
	FText ToastBody() const { return FText::FromString(ToastBodyText); }

	TSharedPtr<SWidget> ToastPanel;
	TSharedPtr<SWidget> HintPanel;
	FSlateColorBrush ToastBrush = FSlateColorBrush(FLinearColor(0.02f, 0.025f, 0.04f, 0.85f));
	FString ToastFromText;
	FString ToastBodyText;
	double Clock = 0.0;
	double ToastAt = -100.0;
	double HintAt = -100.0;
};
