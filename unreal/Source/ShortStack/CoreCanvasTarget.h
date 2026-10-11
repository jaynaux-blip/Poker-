// Core draw lists (ss::ui::Canvas, ShortStackCore's UI) painted into render targets through Slate's widget renderer: the
// Embercrest's marquee, ticker and panels, and in the next segment the card room's clock screens, the badge atlas and the
// sponsors' patch sheets (next_segment_spec 3.10, 5.7, 6.5). Lifted from AEmbercrestStage (EmbercrestStageLook.cpp), which
// uses it as it always painted.
#pragma once

#include "CoreMinimal.h"
#include "ShortStack/UI/Canvas.h"

class FWidgetRenderer;
class UTextureRenderTarget2D;

/** One painter: a Slate widget renderer made on first use and released with Release (or the painter). */
class SHORTSTACK_API FCoreCanvasTarget
{
public:
	FCoreCanvasTarget() = default;
	FCoreCanvasTarget(const FCoreCanvasTarget&) = delete;
	FCoreCanvasTarget& operator=(const FCoreCanvasTarget&) = delete;
	~FCoreCanvasTarget();

	/**
	 * A transient B8G8R8A8 target Width x Height (at least 8 x 8) owned by Outer, cleared to transparent, as a widget
	 * component makes its own; null when nothing can render (a commandlet, a server). The caller keeps it alive (a
	 * UPROPERTY).
	 */
	static UTextureRenderTarget2D* MakeTarget(UObject* Outer, int32 Width, int32 Height);
	/** Draws List into Target at the target's size (nothing without Slate, a target or a list). Game thread. */
	void Render(UTextureRenderTarget2D* Target, const TSharedPtr<const ss::ui::DrawList>& List);
	/** The renderer goes (deferred, as Slate's resources must): EndPlay and BeginDestroy call this. */
	void Release();

private:
	FWidgetRenderer* Renderer = nullptr;
};
