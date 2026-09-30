// Renders ShortStackCore draw lists (the RiverLine client, the phone, printed props) with Slate.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "ShortStack/UI/Canvas.h"
#include "Widgets/SLeafWidget.h"

/** Text measurement through Slate's font cache (the default Roboto font). */
class FSlateTextMeasurer : public ss::ui::TextMeasurer
{
public:
	virtual float Width(const std::string& Text, ss::ui::Font Face, float SizePx) const override;
	virtual float Ascent(ss::ui::Font Face, float SizePx) const override;

	/** Canvas sizes are pixels; Slate font sizes are points at 96 DPI. */
	static FSlateFontInfo FontFor(ss::ui::Font Face, float SizePx);
	static bool IsAvailable();
};

/** Paints the latest draw list, scaled to the widget's size. */
class SHORTSTACK_API SDrawListWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDrawListWidget)
		: _DesiredSize(FVector2D(1600.0, 1000.0))
	{
	}
	SLATE_ARGUMENT(FVector2D, DesiredSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetDrawList(const TSharedPtr<const ss::ui::DrawList>& InList) { List = InList; }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return DesiredSize; }

private:
	TSharedPtr<const ss::ui::DrawList> List;
	FVector2D DesiredSize = FVector2D(1600.0, 1000.0);
};
