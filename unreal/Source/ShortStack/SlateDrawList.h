// Renders ShortStackCore draw lists (the RiverLine client, the phone, printed props) with Slate.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "ShortStack/UI/Canvas.h"
#include "Widgets/SLeafWidget.h"

#include <string>
#include <unordered_map>

/** Text measurement through Slate's font cache (the default Roboto font). */
class FSlateTextMeasurer : public ss::ui::TextMeasurer
{
public:
	virtual float Width(const std::string& Text, ss::ui::Font Face, float SizePx) const override;
	virtual float Ascent(ss::ui::Font Face, float SizePx) const override;

	/** Canvas sizes are pixels; Slate font sizes are points at 96 DPI. */
	static FSlateFontInfo FontFor(ss::ui::Font Face, float SizePx);
	static bool IsAvailable();


private:
	/**
	 * The UI draws every page 30 times a second and measures the same strings each time: widths are remembered by
	 * (face, size, text), and the font infos by (face, size). Cleared when it grows large (a page's worth is a few
	 * thousand strings), so it never holds more than the screens in use.
	 */
	struct FKey
	{
		std::string Text;
		int32 Face = 0;
		int32 Size = 0; // SizePx * 64
		bool operator==(const FKey& O) const { return Face == O.Face && Size == O.Size && Text == O.Text; }
	};
	struct FKeyHash
	{
		size_t operator()(const FKey& K) const { return std::hash<std::string>()(K.Text) ^ (static_cast<size_t>(K.Face) * 0x9E3779B97F4A7C15ull) ^ (static_cast<size_t>(K.Size) << 20); }
	};
	mutable std::unordered_map<FKey, float, FKeyHash> Widths;
	mutable TMap<uint64, FSlateFontInfo> Fonts;
	mutable TMap<uint64, float> Ascents;
	const FSlateFontInfo& CachedFont(ss::ui::Font Face, float SizePx) const;
};

/**
 * A screen redrawn every frame comes out at much the same size each time: a fresh draw list is made with room for
 * what the last one held, so its buffers don't grow (and copy) a dozen times on the way to tens of thousands of
 * vertices.
 */
struct FDrawListHint
{
	size_t Vertices = 0;
	size_t Indices = 0;
	size_t Cmds = 0;

	TSharedPtr<ss::ui::DrawList> Make() const
	{
		TSharedPtr<ss::ui::DrawList> L = MakeShared<ss::ui::DrawList>();
		L->Vertices.reserve(Vertices + Vertices / 8);
		L->Indices.reserve(Indices + Indices / 8);
		L->Cmds.reserve(Cmds + Cmds / 8);
		return L;
	}
	void Note(const ss::ui::DrawList& L)
	{
		Vertices = L.Vertices.size();
		Indices = L.Indices.size();
		Cmds = L.Cmds.size();
	}
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
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return IdealSize; }

private:
	TSharedPtr<const ss::ui::DrawList> List;
	FVector2D IdealSize = FVector2D(1600.0, 1000.0);
};
