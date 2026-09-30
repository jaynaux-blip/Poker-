#pragma once

#include "ShortStack/Common.h"

namespace ss
{
namespace ui
{
/** sRGB color with straight alpha, components 0..1. */
struct Color
{
	float R = 0.0f;
	float G = 0.0f;
	float B = 0.0f;
	float A = 1.0f;
};

inline Color Hex(uint32_t Rgb, float A = 1.0f)
{
	return {static_cast<float>((Rgb >> 16) & 0xff) / 255.0f, static_cast<float>((Rgb >> 8) & 0xff) / 255.0f, static_cast<float>(Rgb & 0xff) / 255.0f, A};
}
inline Color Rgba(int R, int G, int B, float A)
{
	return {static_cast<float>(R) / 255.0f, static_cast<float>(G) / 255.0f, static_cast<float>(B) / 255.0f, A};
}
Color Mix(const Color& A, const Color& B, float T);
/** CSS hsl(h, s%, l%). */
Color Hsl(float HueDeg, float Sat, float Light, float A = 1.0f);

struct Vec2
{
	float X = 0.0f;
	float Y = 0.0f;
};

struct Rect
{
	float X = 0.0f;
	float Y = 0.0f;
	float W = 0.0f;
	float H = 0.0f;
};

/** 2D affine transform: x' = A x + C y + Tx, y' = B x + D y + Ty (canvas setTransform order). */
struct Affine
{
	float A = 1.0f;
	float B = 0.0f;
	float C = 0.0f;
	float D = 1.0f;
	float Tx = 0.0f;
	float Ty = 0.0f;

	Vec2 Apply(Vec2 P) const { return {A * P.X + C * P.Y + Tx, B * P.X + D * P.Y + Ty}; }
	/** this * Other: apply Other first. */
	Affine Then(const Affine& Other) const;
	float UniformScale() const;
	bool IsUniform(float Tolerance = 0.12f) const;
};

/** Solid color or a linear/radial gradient with up to three stops, evaluated per vertex. */
struct Paint
{
	enum class Kind : int
	{
		Solid,
		Linear,
		Radial,
	};
	Kind Type = Kind::Solid;
	Color C0;
	Color C1;
	Color C2;
	float Mid = -1.0f; // position of the middle stop (C1) or -1 for two stops (C0 -> C1)
	Vec2 P0;           // linear: start; radial: inner circle center
	Vec2 P1;           // linear: end; radial: outer circle center
	float R0 = 0.0f;
	float R1 = 1.0f;

	Paint() = default;
	Paint(const Color& C) : C0(C), C1(C), C2(C) {}
	static Paint Solid(const Color& C) { return Paint(C); }
	static Paint Linear(Vec2 From, Vec2 To, const Color& A, const Color& B);
	static Paint Radial(Vec2 C0Center, float Radius0, Vec2 C1Center, float Radius1, const Color& A, float MidT, const Color& Mid, const Color& B);
	Color At(Vec2 P) const;
};

enum class Font : int
{
	Regular,
	Bold,
	Mono,
};

enum class Align : int
{
	Left,
	Center,
	Right,
};

enum class Baseline : int
{
	Alphabetic,
	Middle,
	Top,
	Bottom,
};

struct TextStyle
{
	float Size = 18.0f;
	int Weight = 500;
	Color Col = Hex(0xe6edf7);
	Align HAlign = Align::Left;
	Baseline VAlign = Baseline::Alphabetic;
	bool Mono = false;
	float MaxWidth = 0.0f; // truncate with an ellipsis beyond this width (0 = no limit)
};

/** Text measurement comes from the host (Slate's font cache in Unreal, a metrics table in tests). */
class TextMeasurer
{
public:
	virtual ~TextMeasurer() = default;
	/** Advance width of Text at SizePx pixels. */
	virtual float Width(const std::string& Text, Font Face, float SizePx) const = 0;
	/** Distance from the top of the drawn text box to the baseline. */
	virtual float Ascent(Font Face, float SizePx) const = 0;
};

struct Vertex
{
	float X = 0.0f;
	float Y = 0.0f;
	Color Col;
};

struct TextItem
{
	std::string Text;
	float X = 0.0f; // top-left of the text box
	float Y = 0.0f;
	float BaselineY = 0.0f;
	float SizePx = 18.0f;
	Font Face = Font::Regular;
	Color Col;
};

struct DrawCmd
{
	enum class Kind : int
	{
		Triangles,
		Text,
		PushClip,
		PopClip,
	};
	Kind Type = Kind::Triangles;
	uint32_t VertexStart = 0;
	uint32_t VertexCount = 0;
	uint32_t IndexStart = 0;
	uint32_t IndexCount = 0; // indices are relative to VertexStart
	TextItem Text;
	Rect Clip;
};

/**
 * Output of one UI frame: anti-aliased triangles with per-vertex colors,
 * text runs and clip rectangles, in draw order, in the canvas's logical
 * coordinate space. Hosts replay it (Unreal: Slate custom verts + text).
 */
struct DrawList
{
	float Width = 0.0f;
	float Height = 0.0f;
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;
	std::vector<DrawCmd> Cmds;

	void Clear();
	/** Opens (or continues) a triangle run; returns the base vertex index for relative indices. */
	uint32_t BeginTriangles(size_t ExtraVertices);
	void AddVertex(float X, float Y, const Color& C);
	void AddTriangle(uint32_t I0, uint32_t I1, uint32_t I2);
	void AddText(const TextItem& Item);
	void PushClip(const Rect& R);
	void PopClip();
	/** Compact JSON for the browser replay tool used by the Standalone UI tests. */
	std::string ToJson() const;
};

/**
 * A small canvas-style vector painter. Paths are transformed on the CPU and
 * tessellated with a one-pixel alpha fringe for anti-aliasing, so any host
 * that can draw colored triangles and text renders it faithfully.
 */
class Canvas
{
public:
	/** PixelScale: output pixels per logical unit (sets the anti-aliasing fringe width). */
	Canvas(DrawList& InOut, const TextMeasurer& InMeasurer, float Width, float Height, float PixelScale);

	void Save();
	void Restore();
	void Translate(float X, float Y);
	void Scale(float SX, float SY);
	void Rotate(float Radians);
	const Affine& Transform() const { return State.Xf; }
	/** Canvas globalAlpha (absolute, not multiplied). */
	void SetAlpha(float A) { State.Alpha = A < 0.0f ? 0.0f : A > 1.0f ? 1.0f : A; }
	float GetAlpha() const { return State.Alpha; }

	void PushClip(const Rect& R);
	void PopClip();

	// Shapes.
	void FillRect(const Rect& R, const Paint& P);
	void FillRoundRect(const Rect& R, float Radius, const Paint& P);
	void StrokeRoundRect(const Rect& R, float Radius, const Color& C, float LineWidth);
	void FillEllipse(float CX, float CY, float RX, float RY, const Paint& P);
	void StrokeEllipse(float CX, float CY, float RX, float RY, const Color& C, float LineWidth, float Dash = 0.0f);
	void FillCircle(float CX, float CY, float R, const Paint& P) { FillEllipse(CX, CY, R, R, P); }
	void StrokeArc(float CX, float CY, float R, float A0, float A1, const Color& C, float LineWidth, bool RoundCap = false);
	void FillPolygon(const std::vector<Vec2>& Points, const Paint& P);
	void StrokePolyline(const std::vector<Vec2>& Points, bool Closed, const Color& C, float LineWidth, bool RoundCap = false);
	/** Soft shadow or glow around a rounded rectangle (canvas shadowBlur). */
	void GlowRoundRect(const Rect& R, float Radius, const Color& C, float Blur);

	// Text.
	float Text(const std::string& S, float X, float Y, const TextStyle& Style);
	float Measure(const std::string& S, float Size, int Weight = 500, bool Mono = false) const;
	static Font FaceFor(int Weight, bool Mono) { return Mono ? Font::Mono : Weight >= 600 ? Font::Bold : Font::Regular; }

	float Width() const { return W; }
	float Height() const { return H; }
	float PixelScale() const { return PxScale; }
	DrawList& List() { return Out; }

	// Path helpers (logical coordinates).
	static std::vector<Vec2> RoundRectPath(const Rect& R, float Radius, int SegmentsPerCorner);
	static std::vector<Vec2> EllipsePath(float CX, float CY, float RX, float RY, float A0, float A1, int Segments, bool IncludeEnd);
	int SegmentsFor(float RadiusLogical, float ArcRadians) const;

private:
	struct StateData
	{
		Affine Xf;
		float Alpha = 1.0f;
	};

	void FillPath(const std::vector<Vec2>& Logical, const Paint& P, float FeatherUnits, bool Convex);
	void FillRings(float CX, float CY, float RX, float RY, const Paint& P);
	void StrokeDevice(const std::vector<Vec2>& Device, bool Closed, const Color& C, float Width, bool ExtendCaps);
	float Feather() const { return 1.0f / PxScale; }
	Color Faded(const Color& C) const { return {C.R, C.G, C.B, C.A * State.Alpha}; }

	DrawList& Out;
	const TextMeasurer& Measurer;
	float W = 0.0f;
	float H = 0.0f;
	float PxScale = 1.0f;
	StateData State;
	std::vector<StateData> Stack;
	std::vector<Rect> Clips;
};
} // namespace ui
} // namespace ss
