// The badges' glyphs in one texture (next_segment_spec 5.7, 10): ss::ui::badgeart::Atlas drawn once, at map start, at
// 128-pixel cells into a 1,024-square render target through FCoreCanvasTarget (eight cells a row, cell i at (i % 8,
// i / 8)). The table HUD's plates and its studying caption draw cells by UV (badgeart::AtlasCell, ss::ui::HudGlyphs), so
// no glyph is tessellated per frame: one render, then a textured quad per glyph. ue-venue.
#pragma once

#include "CoreMinimal.h"
#include "CoreCanvasTarget.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SLeafWidget.h"
#include "ShortStack/UI/TableHud.h"

#include <vector>

class UTextureRenderTarget2D;

class SHORTSTACK_API FBadgeAtlas : public TSharedFromThis<FBadgeAtlas>
{
public:
	static constexpr int32 TargetSize = 1024;
	static constexpr int32 CellPixels = 128;
	static constexpr int32 PerRow = TargetSize / CellPixels;

	FBadgeAtlas() = default;
	FBadgeAtlas(const FBadgeAtlas&) = delete;
	FBadgeAtlas& operator=(const FBadgeAtlas&) = delete;

	/**
	 * Draws every variant (badgeart::AtlasBadges) into a new target made with Outer. The caller keeps the target alive
	 * (GetTarget into a UPROPERTY: the game mode's VenueObjects). False when nothing can render (no Slate: a commandlet, a
	 * server), and then the HUD draws the badges as vector art instead.
	 */
	bool Build(UObject* Outer);
	bool IsReady() const;
	UTextureRenderTarget2D* GetTarget() const;
	/** How many cells hold a glyph (badgeart::Atlas's count; 54 today). */
	int32 NumCells() const { return Cells; }
	/** A cell's box in the target, 0..1 (an empty box for a cell outside the atlas). */
	static FBox2f CellUV(int32 Cell);
	/** The cell's brush (the target with the cell's UV region; drawn premultiplied, as Slate drew it), or null. */
	const FSlateBrush* CellBrush(int32 Cell) const;
	/**
	 * Paints Spots, given on a canvas CanvasHeight units tall (the HUD's 1,080), into Geometry's space (the HUD widget's
	 * geometry: its height is the canvas's). One quad per spot; spots without a cell are skipped.
	 */
	void Paint(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const std::vector<ss::ui::GlyphSpot>& Spots, float CanvasHeight,
		const FWidgetStyle& Style) const;
	/** The game thread's time for the one render (ms), for ss.Venue.Describe. */
	double BuildMs = 0.0;

private:
	TWeakObjectPtr<UTextureRenderTarget2D> Target;
	TArray<FSlateBrush> Brushes;
	int32 Cells = 0;
	FCoreCanvasTarget Painter;
};

/**
 * A layer over the table HUD that draws its atlas cells: give it the atlas (the game mode's GetBadgeAtlas, made with
 * MakeShared) and, each frame the HUD draws, the spots ss::ui::HudGlyphs left room for. Add it to the viewport just above
 * the HUD's own widget. It keeps weak holds on the atlas and its target, so a layer that outlives the night (the game
 * mode gone, its target still held by Slate's texture cache) draws nothing rather than touch a freed atlas.
 */
class SHORTSTACK_API SBadgeGlyphLayer : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SBadgeGlyphLayer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetAtlas(const FBadgeAtlas* InAtlas);
	void SetSpots(std::vector<ss::ui::GlyphSpot> InSpots, float InCanvasHeight = 1080.0f);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return FVector2D(1920.0, 1080.0); }

private:
	TWeakPtr<const FBadgeAtlas> Atlas;
	TWeakObjectPtr<UTextureRenderTarget2D> AtlasTarget;
	std::vector<ss::ui::GlyphSpot> Spots;
	float CanvasHeight = 1080.0f;
};
