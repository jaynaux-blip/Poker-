#include "BadgeAtlas.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "SlateDrawList.h"
#include "ShortStack/UI/BadgeArt.h"
#include "ShortStack/UI/Canvas.h"

DEFINE_LOG_CATEGORY_STATIC(LogBadgeAtlas, Log, All);

bool FBadgeAtlas::Build(UObject* Outer)
{
	if (IsReady())
	{
		return true;
	}
	if (!Outer || !FSlateTextMeasurer::IsAvailable())
	{
		return false;
	}
	const double T0 = FPlatformTime::Seconds();
	UTextureRenderTarget2D* T = FCoreCanvasTarget::MakeTarget(Outer, TargetSize, TargetSize);
	if (!T)
	{
		return false;
	}
	// The art's own sizes: a cell is 128 units, drawn 1:1 into the target's pixels (the small sizes' detail drawn large,
	// as the plates scale it down).
	TSharedPtr<ss::ui::DrawList> List = MakeShared<ss::ui::DrawList>();
	{
		FSlateTextMeasurer Measurer;
		ss::ui::Canvas C(*List, Measurer, static_cast<float>(TargetSize), static_cast<float>(TargetSize), 1.0f);
		Cells = FMath::Clamp(ss::ui::badgeart::Atlas(C, static_cast<float>(CellPixels)), 0, PerRow * PerRow);
	}
	Painter.Render(T, List);
	// One render: the painter's widget renderer goes (deferred, as Slate's resources must) and the target keeps the glyphs.
	Painter.Release();
	Target = T;
	Brushes.Reset(Cells);
	for (int32 Cell = 0; Cell < Cells; ++Cell)
	{
		FSlateBrush& B = Brushes.AddDefaulted_GetRef();
		B.SetResourceObject(T);
		B.ImageSize = FVector2D(CellPixels, CellPixels);
		B.DrawAs = ESlateBrushDrawType::Image;
		B.Tiling = ESlateBrushTileType::NoTile;
		B.SetUVRegion(CellUV(Cell));
	}
	BuildMs = (FPlatformTime::Seconds() - T0) * 1000.0;
	UE_LOG(LogBadgeAtlas, Log, TEXT("Badge atlas: %d cells at %d px in %dx%d (%.2f ms)"), Cells, CellPixels, TargetSize, TargetSize, BuildMs);
	return true;
}

bool FBadgeAtlas::IsReady() const
{
	return Target.IsValid() && Cells > 0;
}

UTextureRenderTarget2D* FBadgeAtlas::GetTarget() const
{
	return Target.Get();
}

FBox2f FBadgeAtlas::CellUV(int32 Cell)
{
	if (Cell < 0 || Cell >= PerRow * PerRow)
	{
		return FBox2f(FVector2f::ZeroVector, FVector2f::ZeroVector);
	}
	const float Step = 1.0f / static_cast<float>(PerRow);
	const FVector2f Min(static_cast<float>(Cell % PerRow) * Step, static_cast<float>(Cell / PerRow) * Step);
	return FBox2f(Min, Min + FVector2f(Step, Step));
}

const FSlateBrush* FBadgeAtlas::CellBrush(int32 Cell) const
{
	return IsReady() && Brushes.IsValidIndex(Cell) ? &Brushes[Cell] : nullptr;
}

void FBadgeAtlas::Paint(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const std::vector<ss::ui::GlyphSpot>& Spots, float CanvasHeight,
	const FWidgetStyle& Style) const
{
	if (!IsReady() || Spots.empty())
	{
		return;
	}
	const FVector2f Local = Geometry.GetLocalSize();
	const float K = CanvasHeight > 1.0f && Local.Y > 1.0f ? Local.Y / CanvasHeight : 1.0f;
	const float Fade = Style.GetColorAndOpacityTint().A;
	for (const ss::ui::GlyphSpot& S : Spots)
	{
		const FSlateBrush* Brush = CellBrush(S.Cell);
		const float A = FMath::Clamp(S.Alpha, 0.0f, 1.0f) * Fade;
		if (!Brush || A <= 0.003f || S.Size <= 0.0f)
		{
			continue;
		}
		const float Px = S.Size * K;
		const FVector2f At((S.Cx - S.Size * 0.5f) * K, (S.Cy - S.Size * 0.5f) * K);
		// The target holds premultiplied colour (Slate drew the glyphs over a clear target): a fade scales all four.
		FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(FVector2f(Px, Px), FSlateLayoutTransform(At)), Brush, ESlateDrawEffect::PreMultipliedAlpha,
			FLinearColor(A, A, A, A));
	}
}

// ------------------------------------------------------------------ the HUD's layer

void SBadgeGlyphLayer::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::HitTestInvisible);
}

void SBadgeGlyphLayer::SetAtlas(const FBadgeAtlas* InAtlas)
{
	// (The game mode makes its atlas with MakeShared: the layer holds it weakly, never by the bare pointer.)
	Atlas = InAtlas ? TWeakPtr<const FBadgeAtlas>(InAtlas->AsShared()) : TWeakPtr<const FBadgeAtlas>();
	AtlasTarget = InAtlas ? InAtlas->GetTarget() : nullptr;
}

void SBadgeGlyphLayer::SetSpots(std::vector<ss::ui::GlyphSpot> InSpots, float InCanvasHeight)
{
	Spots = MoveTemp(InSpots);
	CanvasHeight = InCanvasHeight;
}

int32 SBadgeGlyphLayer::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// (The atlas goes with the game mode; its target can outlive it in Slate's texture cache: both holds are checked.)
	const TSharedPtr<const FBadgeAtlas> Held = Atlas.Pin();
	if (Held.IsValid() && AtlasTarget.IsValid())
	{
		Held->Paint(OutDrawElements, LayerId, AllottedGeometry, Spots, CanvasHeight, InWidgetStyle);
	}
	return LayerId;
}
