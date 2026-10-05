// Painting the Embercrest's screens and signs: straight into a draw list (ShortStackCore's Canvas draws through methods
// its module doesn't export to this one), replayed by SDrawListWidget like every other screen in the game.
#pragma once

#include "CoreMinimal.h"
#include "ShortStack/UI/Canvas.h"
#include "SlateDrawList.h"

#include <string>

class FEcxPaint
{
public:
	FEcxPaint(ss::ui::DrawList& InList, float InWidth, float InHeight) : L(InList)
	{
		L.Width = InWidth;
		L.Height = InHeight;
	}

	static ss::ui::Color Ink(uint32 Hex, float Alpha = 1.0f) { return ss::ui::Hex(Hex, Alpha); }

	/** A rectangle, its top one color and its bottom another. */
	void Quad(float X, float Y, float W, float H, const ss::ui::Color& Top, const ss::ui::Color& Bottom)
	{
		const uint32 B = Begin(4, 6);
		Vertex(X, Y, Top);
		Vertex(X + W, Y, Top);
		Vertex(X + W, Y + H, Bottom);
		Vertex(X, Y + H, Bottom);
		Index(B, 0, 1, 2);
		Index(B, 0, 2, 3);
	}

	void Rect(float X, float Y, float W, float H, const ss::ui::Color& C) { Quad(X, Y, W, H, C, C); }

	/** A convex polygon (a fan from its first point). */
	void Poly(std::initializer_list<FVector2f> Points, const ss::ui::Color& C)
	{
		const int32 N = static_cast<int32>(Points.size());
		if (N < 3)
		{
			return;
		}
		const uint32 B = Begin(N, (N - 2) * 3);
		for (const FVector2f& P : Points)
		{
			Vertex(P.X, P.Y, C);
		}
		for (int32 I = 1; I + 1 < N; ++I)
		{
			Index(B, 0, I, I + 1);
		}
	}

	/** A disc whose color runs from Inner at its middle to Outer at its rim (a glow, with Outer transparent). */
	void Disc(float Cx, float Cy, float R, const ss::ui::Color& Inner, const ss::ui::Color& Outer, int32 Segments = 64)
	{
		const uint32 B = Begin(Segments + 1, Segments * 3);
		Vertex(Cx, Cy, Inner);
		for (int32 K = 0; K < Segments; ++K)
		{
			const float A = 2.0f * UE_PI * K / Segments;
			Vertex(Cx + R * FMath::Cos(A), Cy + R * FMath::Sin(A), Outer);
		}
		for (int32 K = 0; K < Segments; ++K)
		{
			Index(B, 0, 1 + K, 1 + (K + 1) % Segments);
		}
	}

	/** A disc lit from Top down to Bottom (the ember sun's gradient). */
	void DiscV(float Cx, float Cy, float R, const ss::ui::Color& Top, const ss::ui::Color& Bottom, int32 Segments = 64)
	{
		const uint32 B = Begin(Segments + 1, Segments * 3);
		Vertex(Cx, Cy, Mix(Top, Bottom, 0.5f));
		for (int32 K = 0; K < Segments; ++K)
		{
			const float A = 2.0f * UE_PI * K / Segments;
			const float Y = Cy + R * FMath::Sin(A);
			Vertex(Cx + R * FMath::Cos(A), Y, Mix(Top, Bottom, FMath::Clamp((Y - (Cy - R)) / (2.0f * R), 0.0f, 1.0f)));
		}
		for (int32 K = 0; K < Segments; ++K)
		{
			Index(B, 0, 1 + K, 1 + (K + 1) % Segments);
		}
	}

	static ss::ui::Font FaceFor(int32 Weight) { return Weight >= 900 ? ss::ui::Font::Black : Weight >= 600 ? ss::ui::Font::Bold : ss::ui::Font::Regular; }

	float Measure(const FString& S, float Size, int32 Weight) const { return Measurer.Width(std::string(TCHAR_TO_UTF8(*S)), FaceFor(Weight), Size); }

	/** Text whose middle sits on Y, at X (Align 0 its left, 1 its middle, 2 its right), shrunk until it fits MaxW (0: any width). */
	void Text(const FString& S, float X, float Y, float Size, int32 Weight, const ss::ui::Color& C, int32 Align = 0, float MaxW = 0.0f)
	{
		if (S.IsEmpty() || !FSlateTextMeasurer::IsAvailable())
		{
			return;
		}
		float Px = Size;
		float W = Measure(S, Px, Weight);
		for (int32 Guard = 0; Guard < 40 && MaxW > 0.0f && W > MaxW && Px > 10.0f; ++Guard)
		{
			Px *= 0.94f;
			W = Measure(S, Px, Weight);
		}
		const ss::ui::Font Face = FaceFor(Weight);
		const float Ascent = Measurer.Ascent(Face, Px);
		ss::ui::DrawCmd Cmd;
		Cmd.Type = ss::ui::DrawCmd::Kind::Text;
		Cmd.Text.Text = std::string(TCHAR_TO_UTF8(*S));
		Cmd.Text.X = Align == 1 ? X - W * 0.5f : Align == 2 ? X - W : X;
		// The capitals' middle on Y: their top is about 0.7 of the ascent above the baseline.
		Cmd.Text.BaselineY = Y + Ascent * 0.36f;
		Cmd.Text.Y = Cmd.Text.BaselineY - Ascent;
		Cmd.Text.SizePx = Px;
		Cmd.Text.Face = Face;
		Cmd.Text.Col = C;
		L.Cmds.push_back(Cmd);
	}

	/**
	 * The brand's mark (art/blender/brand.py): the faceted three-peaked crest (the north shoulder 34/60, the summit 60/60,
	 * the south 40/60), each peak lit on its left face and in shade on its right, the summit's snow facet, the ember disc
	 * rising behind, the ground lines under it. Size across, centered at (Cx, Cy).
	 */
	void Mark(float Cx, float Cy, float Size)
	{
		const float W = Size;
		const float H = Size * 0.62f;
		const float X0 = Cx - W * 0.5f;
		const float Base = Cy + H * 0.5f;
		const float R = Size * 0.27f;
		const FVector2f Sun(Cx, Base - H * 0.86f);
		Disc(Sun.X, Sun.Y, R * 1.45f, Ink(0xff6a24, 0.35f), Ink(0xff6a24, 0.0f));
		DiscV(Sun.X, Sun.Y, R, Ink(0xffd78a), Ink(0x9e2f0c));
		const FVector2f P[7] = {FVector2f(X0, Base), FVector2f(X0 + W * 0.18f, Base - H * 0.567f), FVector2f(X0 + W * 0.33f, Base - H * 0.30f), FVector2f(Cx, Base - H),
			FVector2f(X0 + W * 0.67f, Base - H * 0.34f), FVector2f(X0 + W * 0.82f, Base - H * 0.667f), FVector2f(X0 + W, Base)};
		for (const int32 Peak : {1, 3, 5})
		{
			const FVector2f Foot(P[Peak].X, Base);
			Poly({P[Peak - 1], P[Peak], Foot}, Ink(0xebe7df));
			Poly({Foot, P[Peak], P[Peak + 1]}, Ink(0x8d9ab4));
		}
		Poly({P[3], FVector2f(Cx - W * 0.06f, Base - H * 0.78f), FVector2f(Cx + W * 0.02f, Base - H * 0.84f)}, Ink(0xffffff));
		Rect(X0, Base + H * 0.06f, W, H * 0.025f, Ink(0xebe7df, 0.9f));
		Rect(X0 + W * 0.15f, Base + H * 0.12f, W * 0.7f, H * 0.018f, Ink(0xff6a24, 0.9f));
	}

private:
	static ss::ui::Color Mix(const ss::ui::Color& A, const ss::ui::Color& B, float T)
	{
		return {FMath::Lerp(A.R, B.R, T), FMath::Lerp(A.G, B.G, T), FMath::Lerp(A.B, B.B, T), FMath::Lerp(A.A, B.A, T)};
	}

	/** Opens a run of triangles (continuing the last one when it's triangles too); returns the base for this shape's indices. */
	uint32 Begin(int32 Vertices, int32 Indices)
	{
		if (L.Cmds.empty() || L.Cmds.back().Type != ss::ui::DrawCmd::Kind::Triangles)
		{
			ss::ui::DrawCmd Cmd;
			Cmd.Type = ss::ui::DrawCmd::Kind::Triangles;
			Cmd.VertexStart = static_cast<uint32_t>(L.Vertices.size());
			Cmd.IndexStart = static_cast<uint32_t>(L.Indices.size());
			L.Cmds.push_back(Cmd);
		}
		ss::ui::DrawCmd& Run = L.Cmds.back();
		const uint32 Base = static_cast<uint32>(L.Vertices.size()) - Run.VertexStart;
		Run.VertexCount += static_cast<uint32_t>(Vertices);
		Run.IndexCount += static_cast<uint32_t>(Indices);
		return Base;
	}

	void Vertex(float X, float Y, const ss::ui::Color& C)
	{
		ss::ui::Vertex V;
		V.X = X;
		V.Y = Y;
		V.Col = C;
		L.Vertices.push_back(V);
	}

	void Index(uint32 Base, int32 A, int32 B, int32 C)
	{
		L.Indices.push_back(Base + static_cast<uint32>(A));
		L.Indices.push_back(Base + static_cast<uint32>(B));
		L.Indices.push_back(Base + static_cast<uint32>(C));
	}

	ss::ui::DrawList& L;
	FSlateTextMeasurer Measurer;
};
