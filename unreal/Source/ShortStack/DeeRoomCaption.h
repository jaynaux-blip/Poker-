#pragma once

#include "Brushes/SlateColorBrush.h"
#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/SCompoundWidget.h"

/**
 * What's said in the hub's room, as a caption low on the screen: Dee in her armchair, or a line of the room's own
 * (her door shut, her coming up the stairs) with no one's name on it. Texts go to the phone and its toast; this is for
 * the people in the room. Each line stays long enough to read, then fades.
 */
class SHORTSTACK_API SDeeRoomCaption : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDeeRoomCaption) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** Shows a line: Speaker's (empty: the room's own, set in italics). */
	void Show(const FString& Speaker, const FString& Line);

private:
	EVisibility CaptionVisibility() const;
	EVisibility SpeakerVisibility() const { return SpeakerText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; }
	FText Speaker() const { return FText::FromString(SpeakerText.ToUpper()); }
	FText Line() const { return FText::FromString(LineText); }
	FSlateFontInfo LineFont() const;

	TSharedPtr<SWidget> Panel;
	FSlateColorBrush Brush = FSlateColorBrush(FLinearColor(0.015f, 0.018f, 0.028f, 0.72f));
	FString SpeakerText;
	FString LineText;
	double Clock = 0.0;
	double ShownAt = -100.0;
	double Duration = 0.0;
};
