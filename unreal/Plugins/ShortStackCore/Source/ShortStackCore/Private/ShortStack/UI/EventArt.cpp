// Every tournament's emblem. Glyphs are drawn in unit space (about -1..1) through a Pen that scales and places them;
// tiles and crests are built from layers the way a badge is: a shadow, a bevelled rim of metal, a field in the
// series' colours, an engraved line, the motif with its own shadow, a ribbon, and the ornaments that say what an
// event means (rays, laurel and crown for a Main Event, a bracelet, a ring, a diamond, twinkling glints).
#include "ShortStack/UI/EventArt.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace eventart
{
namespace eventart_detail
{
const float TwoPiF = 6.28318530718f;
const float HalfPiF = 1.57079632679f;
const float PiF = 3.14159265359f;

template <typename T>
float Fl(T V)
{
	return static_cast<float>(V);
}

Color Fade(const Color& C, float A)
{
	return {C.R, C.G, C.B, C.A * A};
}

Color Lift(const Color& C, float T)
{
	return Mix(C, Hex(0xffffff), T);
}

Color Sink(const Color& C, float T)
{
	return Mix(C, Hex(0x000000), T);
}

/** Unit space: (U, V) -> X + U * S, Y + V * S. */
struct Pen
{
	Canvas* Cv = nullptr;
	float X = 0.0f;
	float Y = 0.0f;
	float S = 1.0f;
	Vec2 P(float U, float V) const { return {X + U * S, Y + V * S}; }
	Pen At(float U, float V, float Scale) const { return {Cv, X + U * S, Y + V * S, S * Scale}; }
};

/** A glyph's colours. */
struct Tone
{
	Color Body;  // the glyph
	Color Deep;  // its shading
	Color Core;  // the darkest parts (pupils, holes)
	Color Gem;   // accents (eyes, flames, gems)
	Color Glint; // highlights
};

Tone Shadowed(float A)
{
	const Color K = Rgba(0, 0, 0, A);
	return {K, K, K, K, K};
}

using UV = std::pair<float, float>;

Vec2 Rot(float U, float V, float A)
{
	const float Cs = std::cos(A);
	const float Sn = std::sin(A);
	return {U * Cs - V * Sn, U * Sn + V * Cs};
}

std::vector<Vec2> Unit(std::initializer_list<UV> L)
{
	std::vector<Vec2> Out;
	for (const UV& Q : L)
	{
		Out.push_back({Q.first, Q.second});
	}
	return Out;
}

std::vector<Vec2> Place(const Pen& P, const std::vector<Vec2>& U)
{
	std::vector<Vec2> Out;
	Out.reserve(U.size());
	for (const Vec2& Q : U)
	{
		Out.push_back(P.P(Q.X, Q.Y));
	}
	return Out;
}

/** Unit points turned by A about (Cu, Cv). */
std::vector<Vec2> Turned(const std::vector<Vec2>& U, float A, float Cu = 0.0f, float Cv = 0.0f)
{
	std::vector<Vec2> Out;
	Out.reserve(U.size());
	for (const Vec2& Q : U)
	{
		const Vec2 R = Rot(Q.X - Cu, Q.Y - Cv, A);
		Out.push_back({R.X + Cu, R.Y + Cv});
	}
	return Out;
}

std::vector<Vec2> Scaled(const std::vector<Vec2>& U, float K, float Du = 0.0f, float Dv = 0.0f)
{
	std::vector<Vec2> Out;
	Out.reserve(U.size());
	for (const Vec2& Q : U)
	{
		Out.push_back({Q.X * K + Du, Q.Y * K + Dv});
	}
	return Out;
}

void Poly(const Pen& P, std::initializer_list<UV> L, const Paint& Fill)
{
	P.Cv->FillPolygon(Place(P, Unit(L)), Fill);
}

void PolyV(const Pen& P, const std::vector<Vec2>& U, const Paint& Fill)
{
	P.Cv->FillPolygon(Place(P, U), Fill);
}

void Disc(const Pen& P, float U, float V, float R, const Paint& Fill)
{
	P.Cv->FillEllipse(P.X + U * P.S, P.Y + V * P.S, R * P.S, R * P.S, Fill);
}

void Oval(const Pen& P, float U, float V, float Rx, float Ry, const Paint& Fill)
{
	P.Cv->FillEllipse(P.X + U * P.S, P.Y + V * P.S, Rx * P.S, Ry * P.S, Fill);
}

void Hoop(const Pen& P, float U, float V, float Rx, float Ry, const Color& C, float W)
{
	P.Cv->StrokeEllipse(P.X + U * P.S, P.Y + V * P.S, Rx * P.S, Ry * P.S, C, W * P.S);
}

void Arc(const Pen& P, float U, float V, float R, float A0, float A1, const Color& C, float W, bool Cap = true)
{
	P.Cv->StrokeArc(P.X + U * P.S, P.Y + V * P.S, R * P.S, A0, A1, C, W * P.S, Cap);
}

void Stroke(const Pen& P, std::initializer_list<UV> L, const Color& C, float W, bool Closed = false)
{
	P.Cv->StrokePolyline(Place(P, Unit(L)), Closed, C, W * P.S, true);
}

void StrokeV(const Pen& P, const std::vector<Vec2>& U, const Color& C, float W, bool Closed = false)
{
	P.Cv->StrokePolyline(Place(P, U), Closed, C, W * P.S, true);
}

void Box(const Pen& P, float U, float V, float W, float H, float R, const Paint& Fill)
{
	P.Cv->FillRoundRect({P.X + U * P.S, P.Y + V * P.S, W * P.S, H * P.S}, R * P.S, Fill);
}

Paint Lin(const Pen& P, float U0, float V0, float U1, float V1, const Color& A, const Color& B)
{
	return Paint::Linear(P.P(U0, V0), P.P(U1, V1), A, B);
}

/** A soft round glow (radial: the canvas shades ellipses with rings). */
void Glow(const Pen& P, float U, float V, float R, const Color& C)
{
	const Vec2 Q = P.P(U, V);
	P.Cv->FillEllipse(Q.X, Q.Y, R * P.S, R * P.S, Paint::Radial(Q, 0.0f, Q, R * P.S, C, 0.45f, Fade(C, 0.45f), Fade(C, 0.0f)));
}

/** An ellipse turned by A (petals, leaves, wings), as unit points. */
std::vector<Vec2> Leaf(float U, float V, float Rx, float Ry, float A, int N = 20)
{
	std::vector<Vec2> Out;
	for (int I = 0; I < N; ++I)
	{
		const float T = TwoPiF * Fl(I) / Fl(N);
		const Vec2 R = Rot(Rx * std::cos(T), Ry * std::sin(T), A);
		Out.push_back({U + R.X, V + R.Y});
	}
	return Out;
}

/** A pointed leaf (two arcs meeting at the tips), along angle A. */
std::vector<Vec2> Blade(float U, float V, float Len, float Wid, float A)
{
	std::vector<Vec2> Out;
	const int N = 10;
	for (int I = 0; I <= N; ++I)
	{
		const float T = Fl(I) / Fl(N);
		Out.push_back({-Len + 2.0f * Len * T, -Wid * std::sin(T * PiF)});
	}
	for (int I = N - 1; I > 0; --I)
	{
		const float T = Fl(I) / Fl(N);
		Out.push_back({-Len + 2.0f * Len * T, Wid * std::sin(T * PiF)});
	}
	for (Vec2& Q : Out)
	{
		const Vec2 R = Rot(Q.X, Q.Y, A);
		Q = {U + R.X, V + R.Y};
	}
	return Out;
}

std::vector<Vec2> StarPts(float U, float V, float Outer, float Inner, int Points, float Turn)
{
	std::vector<Vec2> Out;
	for (int I = 0; I < Points * 2; ++I)
	{
		const float A = -HalfPiF + Turn + Fl(I) * PiF / Fl(Points);
		const float R = I % 2 == 0 ? Outer : Inner;
		Out.push_back({U + R * std::cos(A), V + R * std::sin(A)});
	}
	return Out;
}

std::vector<Vec2> ArcPts(float U, float V, float Rx, float Ry, float A0, float A1, int N)
{
	std::vector<Vec2> Out;
	for (int I = 0; I <= N; ++I)
	{
		const float A = A0 + (A1 - A0) * Fl(I) / Fl(N);
		Out.push_back({U + Rx * std::cos(A), V + Ry * std::sin(A)});
	}
	return Out;
}

/** A four-pointed glint. */
void Glint(const Pen& P, float U, float V, float R, const Color& C)
{
	PolyV(P, StarPts(U, V, R, R * 0.2f, 4, 0.0f), C);
}

/** A cut gem: table, crown and pavilion facets in shades of C. */
void Gem(const Pen& P, float U, float V, float K, const Color& C)
{
	auto Q = [&](float X, float Y) { return Vec2{U + X * K, V + Y * K}; };
	const Color Hi = Lift(C, 0.55f);
	const Color Mid = C;
	const Color Lo = Sink(C, 0.35f);
	P.Cv->FillPolygon(Place(P, {Q(-0.9f, -0.15f), Q(-0.42f, -0.55f), Q(-0.2f, -0.15f)}), Lift(C, 0.3f));
	P.Cv->FillPolygon(Place(P, {Q(-0.42f, -0.55f), Q(0.42f, -0.55f), Q(0.2f, -0.15f), Q(-0.2f, -0.15f)}), Hi);
	P.Cv->FillPolygon(Place(P, {Q(0.42f, -0.55f), Q(0.9f, -0.15f), Q(0.2f, -0.15f)}), Mid);
	P.Cv->FillPolygon(Place(P, {Q(-0.9f, -0.15f), Q(-0.2f, -0.15f), Q(0.0f, 0.9f)}), Mid);
	P.Cv->FillPolygon(Place(P, {Q(-0.2f, -0.15f), Q(0.2f, -0.15f), Q(0.0f, 0.9f)}), Lift(C, 0.15f));
	P.Cv->FillPolygon(Place(P, {Q(0.2f, -0.15f), Q(0.9f, -0.15f), Q(0.0f, 0.9f)}), Lo);
	P.Cv->StrokePolyline(Place(P, {Q(-0.9f, -0.15f), Q(-0.42f, -0.55f), Q(0.42f, -0.55f), Q(0.9f, -0.15f), Q(0.0f, 0.9f)}), true, Fade(Hex(0xffffff), 0.55f), 0.03f * K * P.S, true);
	Glint(P, U - 0.32f * K, V - 0.42f * K, 0.22f * K, Fade(Hex(0xffffff), 0.95f));
}

std::vector<Vec2> HeartPts(float U, float V, float K)
{
	std::vector<Vec2> Out;
	for (int I = 0; I < 32; ++I)
	{
		const float T = TwoPiF * Fl(I) / 32.0f;
		const float X = 16.0f * std::pow(std::sin(T), 3.0f);
		const float Y = 13.0f * std::cos(T) - 5.0f * std::cos(2.0f * T) - 2.0f * std::cos(3.0f * T) - std::cos(4.0f * T);
		Out.push_back({U + X / 17.0f * K, V - Y / 17.0f * K});
	}
	return Out;
}

void Spade(const Pen& P, float U, float V, float K, const Paint& Fill)
{
	std::vector<Vec2> H = HeartPts(0.0f, 0.0f, 1.0f);
	for (Vec2& Q : H)
	{
		Q = {U + Q.X * K, V - Q.Y * K - 0.1f * K};
	}
	PolyV(P, H, Fill);
	PolyV(P, {{U, V + 0.1f * K}, {U - 0.32f * K, V + 0.95f * K}, {U + 0.32f * K, V + 0.95f * K}}, Fill);
}

/** A playing card: a rounded rectangle centered at (U, V), turned by A. */
std::vector<Vec2> CardPts(float U, float V, float W, float H, float A)
{
	const std::vector<Vec2> R = Canvas::RoundRectPath({-W / 2.0f, -H / 2.0f, W, H}, std::min(W, H) * 0.12f, 3);
	std::vector<Vec2> Out;
	for (const Vec2& Q : R)
	{
		const Vec2 T = Rot(Q.X, Q.Y, A);
		Out.push_back({U + T.X, V + T.Y});
	}
	return Out;
}

/** A cloud (overlapping puffs on a flat base). */
void Cloud(const Pen& P, float U, float V, float K, const Color& Top, const Color& Under)
{
	Disc(P, U - 0.38f * K, V + 0.06f * K, 0.32f * K, Under);
	Disc(P, U + 0.02f * K, V - 0.12f * K, 0.42f * K, Under);
	Disc(P, U + 0.42f * K, V + 0.1f * K, 0.3f * K, Under);
	Box(P, U - 0.72f * K, V + 0.06f * K, 1.44f * K, 0.34f * K, 0.17f * K, Under);
	Disc(P, U - 0.38f * K, V, 0.32f * K, Top);
	Disc(P, U + 0.02f * K, V - 0.18f * K, 0.42f * K, Top);
	Disc(P, U + 0.42f * K, V + 0.04f * K, 0.3f * K, Top);
	Box(P, U - 0.72f * K, V, 1.44f * K, 0.32f * K, 0.16f * K, Top);
}

/** A laurel branch along a circle of radius R, from angle A0 to A1 (radians, y down). */
void Laurel(const Pen& P, float R, float A0, float A1, const Color& LeafHi, const Color& LeafLo, const Color& Stem, int Leaves)
{
	Arc(P, 0.0f, 0.0f, R, std::min(A0, A1), std::max(A0, A1), Stem, 0.05f);
	const float Dir = A1 > A0 ? 1.0f : -1.0f;
	for (int I = 0; I < Leaves; ++I)
	{
		const float T = (Fl(I) + 0.5f) / Fl(Leaves);
		const float A = A0 + (A1 - A0) * T;
		const float Size = 0.17f * (1.0f - 0.45f * T);
		const float Cx = R * std::cos(A);
		const float Cy = R * std::sin(A);
		// The tangent, pointing the way the branch grows; leaves lean out and in from it.
		const float Tangent = A + Dir * HalfPiF;
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float Lean = Tangent + Fl(Side) * 0.55f * Dir;
			const float Ox = Cx + std::cos(Lean) * Size * 0.95f;
			const float Oy = Cy + std::sin(Lean) * Size * 0.95f;
			PolyV(P, Blade(Ox, Oy, Size, Size * 0.42f, Lean), Lin(P, Ox - Size, Oy - Size, Ox + Size, Oy + Size, LeafHi, LeafLo));
		}
	}
	// The tip.
	const float At = A1 + Dir * 0.04f;
	PolyV(P, Blade(R * std::cos(At), R * std::sin(At), 0.1f, 0.045f, At + Dir * HalfPiF), LeafHi);
}

// ------------------------------------------------------------------ the glyphs

void GOwl(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.62f, -0.3f}, {-0.56f, -0.92f}, {-0.16f, -0.5f}}, T.Deep);
	Poly(P, {{0.62f, -0.3f}, {0.56f, -0.92f}, {0.16f, -0.5f}}, T.Deep);
	Oval(P, 0.0f, 0.14f, 0.66f, 0.76f, Lin(P, 0.0f, -0.6f, 0.0f, 0.9f, T.Body, T.Deep));
	PolyV(P, Leaf(-0.58f, 0.32f, 0.2f, 0.44f, 0.25f), T.Deep);
	PolyV(P, Leaf(0.58f, 0.32f, 0.2f, 0.44f, -0.25f), T.Deep);
	Disc(P, -0.27f, -0.13f, 0.3f, Lift(T.Body, 0.3f));
	Disc(P, 0.27f, -0.13f, 0.3f, Lift(T.Body, 0.3f));
	Disc(P, -0.27f, -0.13f, 0.2f, T.Gem);
	Disc(P, 0.27f, -0.13f, 0.2f, T.Gem);
	Disc(P, -0.27f, -0.13f, 0.1f, T.Core);
	Disc(P, 0.27f, -0.13f, 0.1f, T.Core);
	Disc(P, -0.22f, -0.18f, 0.045f, T.Glint);
	Disc(P, 0.32f, -0.18f, 0.045f, T.Glint);
	Poly(P, {{-0.09f, 0.06f}, {0.09f, 0.06f}, {0.0f, 0.25f}}, T.Gem);
	for (int K = 0; K < 3; ++K)
	{
		const float X = -0.22f + Fl(K) * 0.22f;
		Stroke(P, {{X - 0.08f, 0.44f}, {X, 0.5f}, {X + 0.08f, 0.44f}}, T.Deep, 0.05f);
	}
	Stroke(P, {{-0.19f, 0.64f}, {-0.11f, 0.7f}, {-0.03f, 0.64f}}, T.Deep, 0.05f);
	Stroke(P, {{0.03f, 0.64f}, {0.11f, 0.7f}, {0.19f, 0.64f}}, T.Deep, 0.05f);
}

void GBolt(const Pen& P, const Tone& T)
{
	Poly(P, {{0.2f, -0.95f}, {-0.52f, 0.1f}, {-0.04f, 0.1f}, {-0.22f, 0.95f}, {0.56f, -0.2f}, {0.07f, -0.2f}}, Lin(P, -0.5f, -0.9f, 0.5f, 0.9f, T.Body, T.Deep));
	Stroke(P, {{0.12f, -0.76f}, {-0.36f, -0.02f}}, Fade(T.Glint, 0.7f), 0.05f);
}

/** A crescent: the outer circle's arc outside the inner circle, then the inner circle's arc back. */
std::vector<Vec2> CrescentPts(float R, float Ix, float Iy, float Ir)
{
	auto Outside = [&](float X, float Y) { return (X - Ix) * (X - Ix) + (Y - Iy) * (Y - Iy) >= Ir * Ir; };
	const int N = 96;
	// Start the outer arc just after the inner circle lets go of it.
	int Start = 0;
	for (int I = 0; I < N; ++I)
	{
		const float A0 = TwoPiF * Fl(I) / Fl(N);
		const float A1 = TwoPiF * Fl(I + 1) / Fl(N);
		if (!Outside(R * std::cos(A0), R * std::sin(A0)) && Outside(R * std::cos(A1), R * std::sin(A1)))
		{
			Start = I + 1;
		}
	}
	std::vector<Vec2> Out;
	for (int K = 0; K < N; ++K)
	{
		const float A = TwoPiF * Fl(Start + K) / Fl(N);
		const float X = R * std::cos(A);
		const float Y = R * std::sin(A);
		if (!Outside(X, Y))
		{
			break;
		}
		Out.push_back({X, Y});
	}
	// Back along the inner circle, inside the outer one.
	const Vec2 End = Out.back();
	const float Ae = std::atan2(End.Y - Iy, End.X - Ix);
	const Vec2 Begin = Out.front();
	float Ab = std::atan2(Begin.Y - Iy, Begin.X - Ix);
	while (Ab > Ae)
	{
		Ab -= TwoPiF;
	}
	for (int K = 1; K < 40; ++K)
	{
		const float A = Ae + (Ab - Ae) * Fl(K) / 40.0f;
		Out.push_back({Ix + Ir * std::cos(A), Iy + Ir * std::sin(A)});
	}
	return Out;
}

void GMoon(const Pen& P, const Tone& T, bool Stars)
{
	const std::vector<Vec2> M = CrescentPts(0.8f, 0.36f, -0.2f, 0.66f);
	PolyV(P, M, Lin(P, -0.7f, -0.6f, 0.3f, 0.8f, T.Body, T.Deep));
	Disc(P, -0.5f, 0.14f, 0.08f, Fade(T.Deep, 0.8f));
	Disc(P, -0.42f, -0.3f, 0.05f, Fade(T.Deep, 0.8f));
	Disc(P, -0.3f, 0.48f, 0.06f, Fade(T.Deep, 0.8f));
	if (Stars)
	{
		PolyV(P, StarPts(0.48f, -0.42f, 0.18f, 0.08f, 5, 0.0f), T.Gem);
		PolyV(P, StarPts(0.62f, 0.26f, 0.11f, 0.05f, 5, 0.3f), T.Glint);
		Glint(P, 0.3f, 0.62f, 0.1f, T.Glint);
	}
}

void GEye(const Pen& P, const Tone& T)
{
	std::vector<Vec2> E;
	for (int I = 0; I <= 20; ++I)
	{
		const float S = Fl(I) / 20.0f;
		E.push_back({-0.92f + 1.84f * S, -0.55f * std::sin(S * PiF)});
	}
	for (int I = 19; I > 0; --I)
	{
		const float S = Fl(I) / 20.0f;
		E.push_back({-0.92f + 1.84f * S, 0.5f * std::sin(S * PiF)});
	}
	PolyV(P, E, T.Body);
	Disc(P, 0.0f, 0.0f, 0.38f, T.Gem);
	Hoop(P, 0.0f, 0.0f, 0.38f, 0.38f, Sink(T.Gem, 0.35f), 0.05f);
	Disc(P, 0.0f, 0.0f, 0.17f, T.Core);
	Disc(P, 0.11f, -0.12f, 0.07f, T.Glint);
	for (int K = 0; K < 5; ++K)
	{
		const float S = 0.2f + 0.15f * Fl(K);
		const float X = -0.92f + 1.84f * S;
		const float Y = -0.55f * std::sin(S * PiF);
		const float Out = (S - 0.5f) * 0.9f;
		Stroke(P, {{X, Y}, {X + Out * 0.25f, Y - 0.2f}}, T.Body, 0.06f);
	}
	Arc(P, 0.0f, -0.05f, 0.62f, 0.55f, PiF - 0.55f, Fade(T.Deep, 0.9f), 0.05f);
}

void GCrosshair(const Pen& P, const Tone& T)
{
	Hoop(P, 0.0f, 0.0f, 0.68f, 0.68f, T.Body, 0.13f);
	Hoop(P, 0.0f, 0.0f, 0.3f, 0.3f, T.Body, 0.08f);
	Stroke(P, {{0.0f, -0.98f}, {0.0f, -0.48f}}, T.Body, 0.12f);
	Stroke(P, {{0.0f, 0.48f}, {0.0f, 0.98f}}, T.Body, 0.12f);
	Stroke(P, {{-0.98f, 0.0f}, {-0.48f, 0.0f}}, T.Body, 0.12f);
	Stroke(P, {{0.48f, 0.0f}, {0.98f, 0.0f}}, T.Body, 0.12f);
	Disc(P, 0.0f, 0.0f, 0.1f, T.Gem);
}

void GGear(const Pen& P, const Tone& T)
{
	std::vector<Vec2> G;
	const int Teeth = 9;
	for (int K = 0; K < Teeth; ++K)
	{
		const float A = TwoPiF * Fl(K) / Fl(Teeth);
		const float Step = TwoPiF / Fl(Teeth);
		G.push_back({0.68f * std::cos(A - Step * 0.32f), 0.68f * std::sin(A - Step * 0.32f)});
		G.push_back({0.92f * std::cos(A - Step * 0.18f), 0.92f * std::sin(A - Step * 0.18f)});
		G.push_back({0.92f * std::cos(A + Step * 0.18f), 0.92f * std::sin(A + Step * 0.18f)});
		G.push_back({0.68f * std::cos(A + Step * 0.32f), 0.68f * std::sin(A + Step * 0.32f)});
	}
	PolyV(P, G, Lin(P, -0.8f, -0.8f, 0.8f, 0.8f, T.Body, T.Deep));
	Disc(P, 0.0f, 0.0f, 0.46f, T.Deep);
	Disc(P, 0.0f, 0.0f, 0.38f, T.Body);
	Disc(P, 0.0f, 0.0f, 0.18f, T.Core);
	Disc(P, 0.0f, 0.0f, 0.07f, T.Gem);
}

void GCrown(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.78f, 0.46f}, {-0.88f, -0.42f}, {-0.42f, -0.02f}, {0.0f, -0.64f}, {0.42f, -0.02f}, {0.88f, -0.42f}, {0.78f, 0.46f}}, Lin(P, 0.0f, -0.6f, 0.0f, 0.5f, T.Body, T.Deep));
	Box(P, -0.82f, 0.4f, 1.64f, 0.28f, 0.06f, Lin(P, 0.0f, 0.4f, 0.0f, 0.68f, T.Body, T.Deep));
	Disc(P, -0.88f, -0.46f, 0.12f, T.Gem);
	Disc(P, 0.0f, -0.7f, 0.14f, T.Gem);
	Disc(P, 0.88f, -0.46f, 0.12f, T.Gem);
	Disc(P, -0.45f, 0.54f, 0.07f, T.Gem);
	Disc(P, 0.0f, 0.54f, 0.09f, T.Glint);
	Disc(P, 0.45f, 0.54f, 0.07f, T.Gem);
	Stroke(P, {{-0.6f, 0.2f}, {-0.66f, -0.18f}}, Fade(T.Glint, 0.6f), 0.05f);
}

void GDiamond(const Pen& P, const Tone& T)
{
	Gem(P, 0.0f, -0.05f, 1.0f, T.Gem);
}

void GWave(const Pen& P, const Tone& T)
{
	for (int B = 0; B < 3; ++B)
	{
		std::vector<Vec2> W;
		const float Y0 = -0.46f + Fl(B) * 0.44f;
		for (int I = 0; I <= 24; ++I)
		{
			const float X = -0.88f + 1.76f * Fl(I) / 24.0f;
			W.push_back({X, Y0 + 0.15f * std::sin(X * 3.6f + Fl(B) * 0.9f)});
		}
		StrokeV(P, W, Mix(T.Body, T.Deep, Fl(B) * 0.35f), 0.17f);
	}
	Disc(P, 0.62f, -0.62f, 0.06f, T.Gem);
	Disc(P, 0.78f, -0.5f, 0.04f, T.Gem);
}

void GEnvelope(const Pen& P, const Tone& T)
{
	Box(P, -0.86f, -0.56f, 1.72f, 1.12f, 0.12f, Lin(P, 0.0f, -0.56f, 0.0f, 0.56f, T.Body, T.Deep));
	Stroke(P, {{-0.78f, 0.46f}, {-0.2f, 0.02f}}, Fade(T.Deep, 0.9f), 0.05f);
	Stroke(P, {{0.78f, 0.46f}, {0.2f, 0.02f}}, Fade(T.Deep, 0.9f), 0.05f);
	Poly(P, {{-0.84f, -0.5f}, {0.0f, 0.14f}, {0.84f, -0.5f}}, Mix(T.Body, T.Deep, 0.45f));
	Disc(P, 0.0f, 0.1f, 0.32f, T.Gem);
	Hoop(P, 0.0f, 0.1f, 0.32f, 0.32f, Sink(T.Gem, 0.3f), 0.04f);
	Arc(P, 0.0f, 0.02f, 0.11f, PiF * 1.05f, PiF * 2.35f, T.Glint, 0.06f);
	Stroke(P, {{0.05f, 0.11f}, {0.0f, 0.15f}, {0.0f, 0.2f}}, T.Glint, 0.06f);
	Disc(P, 0.0f, 0.3f, 0.04f, T.Glint);
}

void GCoin(const Pen& P, const Tone& T)
{
	Arc(P, 0.0f, 0.0f, 0.95f, -0.75f, 0.75f, Fade(T.Body, 0.55f), 0.06f);
	Arc(P, 0.0f, 0.0f, 0.95f, PiF - 0.75f, PiF + 0.75f, Fade(T.Body, 0.55f), 0.06f);
	Oval(P, 0.0f, 0.0f, 0.62f, 0.76f, Lin(P, -0.6f, -0.7f, 0.6f, 0.7f, Lift(T.Gem, 0.35f), Sink(T.Gem, 0.25f)));
	Hoop(P, 0.0f, 0.0f, 0.62f, 0.76f, Sink(T.Gem, 0.35f), 0.06f);
	Hoop(P, 0.0f, 0.0f, 0.48f, 0.6f, Fade(Lift(T.Gem, 0.6f), 0.8f), 0.04f);
	PolyV(P, StarPts(0.0f, 0.02f, 0.3f, 0.13f, 5, 0.0f), Lift(T.Gem, 0.7f));
	Oval(P, -0.28f, -0.32f, 0.08f, 0.16f, Fade(T.Glint, 0.6f));
}

void GStopwatch(const Pen& P, const Tone& T)
{
	Box(P, -0.1f, -0.86f, 0.2f, 0.2f, 0.04f, T.Body);
	Box(P, -0.2f, -0.96f, 0.4f, 0.12f, 0.05f, T.Body);
	PolyV(P, Turned(Unit({{-0.07f, -0.8f}, {0.07f, -0.8f}, {0.07f, -0.66f}, {-0.07f, -0.66f}}), 0.75f, 0.0f, 0.12f), T.Body);
	Disc(P, 0.0f, 0.12f, 0.78f, Lin(P, 0.0f, -0.6f, 0.0f, 0.9f, T.Body, T.Deep));
	Disc(P, 0.0f, 0.12f, 0.6f, T.Core);
	for (int K = 0; K < 12; ++K)
	{
		const float A = TwoPiF * Fl(K) / 12.0f;
		const float R0 = K % 3 == 0 ? 0.42f : 0.5f;
		Stroke(P, {{R0 * std::cos(A), 0.12f + R0 * std::sin(A)}, {0.56f * std::cos(A), 0.12f + 0.56f * std::sin(A)}}, Fade(T.Body, 0.85f), 0.04f);
	}
	Stroke(P, {{0.0f, 0.12f}, {0.3f, -0.2f}}, T.Gem, 0.08f);
	Disc(P, 0.0f, 0.12f, 0.07f, T.Gem);
}

void GFan(const Pen& P, const Tone& T)
{
	static const bool RedSuit[4] = {false, true, false, true};
	for (int K = 0; K < 4; ++K)
	{
		const float A = -0.54f + Fl(K) * 0.36f;
		const Vec2 C = Rot(0.0f, -0.62f, A);
		const float Cx = C.X;
		const float Cy = C.Y + 0.76f;
		PolyV(P, CardPts(Cx, Cy, 0.74f, 1.06f, A), Lin(P, 0.0f, -0.8f, 0.0f, 0.9f, T.Body, Mix(T.Body, T.Deep, 0.6f)));
		StrokeV(P, CardPts(Cx, Cy, 0.74f, 1.06f, A), Fade(T.Deep, 0.9f), 0.03f, true);
		const Vec2 Pip = Rot(-0.17f, -0.3f, A);
		const Pen Pp = P.At(Cx + Pip.X, Cy + Pip.Y, 0.15f);
		if (RedSuit[K])
		{
			PolyV(Pp, HeartPts(0.0f, 0.0f, 1.0f), T.Gem);
		}
		else
		{
			Spade(Pp, 0.0f, 0.0f, 1.0f, T.Core);
		}
	}
}

void GChips(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 4; ++K)
	{
		const float Y = 0.6f - Fl(K) * 0.27f;
		const Color Face = K % 2 == 0 ? T.Body : T.Gem;
		Box(P, -0.7f, Y - 0.04f, 1.4f, 0.2f, 0.0f, Lin(P, -0.7f, 0.0f, 0.7f, 0.0f, Sink(Face, 0.25f), Sink(Face, 0.45f)));
		Oval(P, 0.0f, Y + 0.16f, 0.7f, 0.2f, Sink(Face, 0.4f));
		for (int S = -1; S <= 1; ++S)
		{
			Box(P, Fl(S) * 0.42f - 0.06f, Y - 0.02f, 0.12f, 0.17f, 0.0f, Fade(T.Glint, 0.75f));
		}
		Oval(P, 0.0f, Y - 0.04f, 0.7f, 0.2f, Lin(P, 0.0f, Y - 0.24f, 0.0f, Y + 0.16f, Lift(Face, 0.2f), Face));
	}
	Hoop(P, 0.0f, 0.6f - 3.0f * 0.27f - 0.04f, 0.46f, 0.12f, Fade(T.Glint, 0.8f), 0.04f);
}

void GBricks(const Pen& P, const Tone& T)
{
	const float H = 0.38f;
	for (int Row = 0; Row < 3; ++Row)
	{
		const int Count = 3 - Row;
		const float Y = 0.5f - Fl(Row) * (H + 0.06f);
		const float W = 0.54f;
		const float X0 = -Fl(Count) * (W + 0.06f) / 2.0f + 0.03f;
		for (int K = 0; K < Count; ++K)
		{
			const float X = X0 + Fl(K) * (W + 0.06f);
			Box(P, X, Y, W, H, 0.06f, Lin(P, X, Y, X, Y + H, T.Body, T.Deep));
			Box(P, X + 0.04f, Y + 0.04f, W - 0.08f, 0.06f, 0.03f, Fade(T.Glint, 0.45f));
		}
	}
	PolyV(P, StarPts(0.0f, -0.82f, 0.16f, 0.07f, 5, 0.0f), T.Gem);
}

void GTombstone(const Pen& P, const Tone& T)
{
	std::vector<Vec2> S = ArcPts(0.0f, -0.25f, 0.55f, 0.55f, PiF, TwoPiF, 20);
	S.push_back({0.55f, 0.66f});
	S.push_back({-0.55f, 0.66f});
	PolyV(P, S, Lin(P, -0.5f, -0.8f, 0.5f, 0.7f, T.Body, T.Deep));
	Stroke(P, {{0.0f, -0.5f}, {0.0f, 0.1f}}, T.Deep, 0.08f);
	Stroke(P, {{-0.2f, -0.3f}, {0.2f, -0.3f}}, T.Deep, 0.08f);
	Box(P, -0.9f, 0.62f, 1.8f, 0.2f, 0.1f, T.Gem);
	for (int K = 0; K < 5; ++K)
	{
		const float X = -0.78f + Fl(K) * 0.38f;
		Poly(P, {{X - 0.06f, 0.64f}, {X, 0.46f}, {X + 0.06f, 0.64f}}, T.Gem);
	}
	PolyV(P, StarPts(0.72f, -0.72f, 0.12f, 0.05f, 5, 0.2f), T.Glint);
}

void GSunrise(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 9; ++K)
	{
		const float A = PiF + 0.17f + Fl(K) * (PiF - 0.34f) / 8.0f;
		Poly(P, {{0.66f * std::cos(A - 0.07f), 0.28f + 0.66f * std::sin(A - 0.07f)}, {0.98f * std::cos(A), 0.28f + 0.98f * std::sin(A)},
					{0.66f * std::cos(A + 0.07f), 0.28f + 0.66f * std::sin(A + 0.07f)}},
			T.Body);
	}
	std::vector<Vec2> S = ArcPts(0.0f, 0.28f, 0.52f, 0.52f, PiF, TwoPiF, 22);
	PolyV(P, S, Lin(P, 0.0f, -0.25f, 0.0f, 0.3f, Lift(T.Gem, 0.4f), T.Gem));
	Box(P, -0.92f, 0.28f, 1.84f, 0.09f, 0.045f, T.Body);
	Box(P, -0.6f, 0.47f, 1.2f, 0.07f, 0.035f, Fade(T.Body, 0.75f));
	Box(P, -0.34f, 0.63f, 0.68f, 0.07f, 0.035f, Fade(T.Body, 0.5f));
}

void GClaw(const Pen& P, const Tone& T)
{
	for (int K = -1; K <= 1; ++K)
	{
		const float D = Fl(K) * 0.38f;
		std::vector<Vec2> S;
		for (int I = 0; I <= 12; ++I)
		{
			const float U = Fl(I) / 12.0f;
			const float Bend = 0.18f * std::sin(U * PiF);
			S.push_back({0.42f + D - 0.84f * U - Bend, -0.9f + 1.8f * U - 0.0f});
		}
		std::vector<Vec2> Shape;
		for (int I = 0; I <= 12; ++I)
		{
			const float W = 0.11f * std::sin(Fl(I) / 12.0f * PiF);
			Shape.push_back({S[static_cast<size_t>(I)].X - W, S[static_cast<size_t>(I)].Y - W * 0.4f});
		}
		for (int I = 12; I >= 0; --I)
		{
			const float W = 0.11f * std::sin(Fl(I) / 12.0f * PiF);
			Shape.push_back({S[static_cast<size_t>(I)].X + W, S[static_cast<size_t>(I)].Y + W * 0.4f});
		}
		PolyV(P, Shape, Lin(P, 0.5f, -0.9f, -0.5f, 0.9f, T.Body, T.Gem));
	}
}

void GStairs(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.9f, 0.8f}, {-0.9f, 0.42f}, {-0.46f, 0.42f}, {-0.46f, 0.04f}, {-0.02f, 0.04f}, {-0.02f, -0.34f}, {0.42f, -0.34f}, {0.42f, -0.72f}, {0.9f, -0.72f}, {0.9f, 0.8f}},
		Lin(P, -0.9f, -0.7f, 0.9f, 0.8f, T.Body, T.Deep));
	Stroke(P, {{-0.9f, 0.42f}, {-0.46f, 0.42f}}, T.Glint, 0.04f);
	Stroke(P, {{-0.46f, 0.04f}, {-0.02f, 0.04f}}, T.Glint, 0.04f);
	Stroke(P, {{-0.02f, -0.34f}, {0.42f, -0.34f}}, T.Glint, 0.04f);
	Stroke(P, {{0.42f, -0.72f}, {0.9f, -0.72f}}, T.Glint, 0.04f);
	Stroke(P, {{0.66f, -0.72f}, {0.66f, -1.0f}}, T.Body, 0.04f);
	Poly(P, {{0.68f, -1.0f}, {0.94f, -0.92f}, {0.68f, -0.84f}}, T.Gem);
}

void GSpiral(const Pen& P, const Tone& T)
{
	std::vector<Vec2> S;
	for (int I = 0; I <= 120; ++I)
	{
		const float U = Fl(I) / 120.0f;
		const float A = U * 3.1f * TwoPiF;
		const float R = 0.06f + 0.82f * U;
		S.push_back({R * std::cos(A), R * std::sin(A)});
	}
	StrokeV(P, S, T.Body, 0.13f);
	Disc(P, 0.0f, 0.0f, 0.1f, T.Gem);
}

void GRocket(const Pen& P, const Tone& T)
{
	const float A = 0.62f;
	std::vector<Vec2> Body = ArcPts(0.0f, -0.36f, 0.3f, 0.6f, PiF, TwoPiF, 16);
	Body.push_back({0.3f, 0.36f});
	Body.push_back({-0.3f, 0.36f});
	const std::vector<Vec2> Flame = Unit({{-0.2f, 0.4f}, {0.2f, 0.4f}, {0.12f, 0.7f}, {0.0f, 0.98f}, {-0.12f, 0.7f}});
	const std::vector<Vec2> Inner = Unit({{-0.1f, 0.4f}, {0.1f, 0.4f}, {0.0f, 0.74f}});
	PolyV(P, Turned(Flame, A), Lin(P, 0.0f, 0.2f, -0.5f, 0.9f, Lift(T.Gem, 0.4f), T.Gem));
	PolyV(P, Turned(Inner, A), Lift(T.Gem, 0.75f));
	PolyV(P, Turned(Unit({{-0.3f, 0.0f}, {-0.6f, 0.46f}, {-0.3f, 0.36f}}), A), T.Deep);
	PolyV(P, Turned(Unit({{0.3f, 0.0f}, {0.6f, 0.46f}, {0.3f, 0.36f}}), A), T.Deep);
	PolyV(P, Turned(Body, A), Lin(P, -0.4f, 0.0f, 0.4f, 0.0f, T.Body, T.Deep));
	const Vec2 W = Rot(0.0f, -0.32f, A);
	Disc(P, W.X, W.Y, 0.15f, T.Deep);
	Disc(P, W.X, W.Y, 0.1f, T.Gem);
	Disc(P, W.X - 0.03f, W.Y - 0.03f, 0.035f, T.Glint);
}

void GFlame(const Pen& P, const Tone& T)
{
	Poly(P, {{0.05f, -0.98f}, {0.32f, -0.5f}, {0.56f, -0.12f}, {0.6f, 0.32f}, {0.36f, 0.74f}, {0.0f, 0.88f}, {-0.36f, 0.74f}, {-0.58f, 0.34f}, {-0.5f, -0.08f}, {-0.28f, -0.3f},
				{-0.3f, -0.64f}, {-0.1f, -0.4f}},
		Lin(P, 0.0f, -0.9f, 0.0f, 0.9f, Lift(T.Gem, 0.3f), Sink(T.Gem, 0.15f)));
	Poly(P, {{0.02f, -0.3f}, {0.26f, 0.08f}, {0.3f, 0.42f}, {0.0f, 0.7f}, {-0.3f, 0.42f}, {-0.22f, 0.08f}, {-0.08f, 0.0f}}, Lin(P, 0.0f, -0.3f, 0.0f, 0.7f, T.Glint, Lift(T.Gem, 0.55f)));
}

void GGlove(const Pen& P, const Tone& T)
{
	Box(P, -0.42f, 0.36f, 0.84f, 0.5f, 0.12f, Lin(P, 0.0f, 0.36f, 0.0f, 0.86f, T.Body, T.Deep));
	Box(P, -0.44f, 0.4f, 0.88f, 0.1f, 0.04f, T.Gem);
	Oval(P, 0.06f, -0.16f, 0.66f, 0.62f, Lin(P, -0.6f, -0.7f, 0.6f, 0.5f, Lift(T.Gem, 0.3f), Sink(T.Gem, 0.2f)));
	Oval(P, -0.5f, 0.06f, 0.22f, 0.32f, Sink(T.Gem, 0.12f));
	Arc(P, 0.06f, -0.12f, 0.42f, -2.7f, -0.55f, Fade(Sink(T.Gem, 0.45f), 0.8f), 0.05f);
	Oval(P, -0.12f, -0.42f, 0.18f, 0.1f, Fade(T.Glint, 0.55f));
}

void GSixSeats(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 6; ++K)
	{
		const float A = TwoPiF * Fl(K) / 6.0f + TwoPiF / 12.0f;
		Disc(P, 0.8f * std::cos(A), 0.62f * std::sin(A), 0.16f, Lin(P, 0.0f, -0.8f, 0.0f, 0.8f, T.Body, T.Deep));
	}
	Oval(P, 0.0f, 0.0f, 0.56f, 0.38f, T.Body);
	Oval(P, 0.0f, 0.0f, 0.46f, 0.29f, Lin(P, 0.0f, -0.3f, 0.0f, 0.3f, Lift(T.Gem, 0.2f), Sink(T.Gem, 0.2f)));
	Oval(P, 0.0f, -0.08f, 0.2f, 0.06f, Fade(T.Glint, 0.4f));
}

void GLaurelM(const Pen& P, const Tone& T)
{
	Laurel(P, 0.78f, HalfPiF + 0.35f, HalfPiF + 2.7f, T.Body, T.Deep, T.Deep, 7);
	Laurel(P, 0.78f, HalfPiF - 0.35f, HalfPiF - 2.7f, T.Body, T.Deep, T.Deep, 7);
	Disc(P, 0.0f, 0.0f, 0.46f, Lin(P, 0.0f, -0.46f, 0.0f, 0.46f, Lift(T.Gem, 0.3f), Sink(T.Gem, 0.2f)));
	Poly(P, {{-0.28f, 0.24f}, {-0.28f, -0.24f}, {-0.16f, -0.24f}, {0.0f, 0.0f}, {0.16f, -0.24f}, {0.28f, -0.24f}, {0.28f, 0.24f}, {0.17f, 0.24f}, {0.17f, -0.06f}, {0.0f, 0.18f},
				{-0.17f, -0.06f}, {-0.17f, 0.24f}},
		T.Core);
}

void GShowdown(const Pen& P, const Tone& T)
{
	PolyV(P, CardPts(-0.24f, 0.04f, 0.74f, 1.04f, -0.2f), T.Body);
	StrokeV(P, CardPts(-0.24f, 0.04f, 0.74f, 1.04f, -0.2f), T.Deep, 0.03f, true);
	PolyV(P, CardPts(0.26f, 0.0f, 0.74f, 1.04f, 0.18f), Lift(T.Body, 0.4f));
	StrokeV(P, CardPts(0.26f, 0.0f, 0.74f, 1.04f, 0.18f), T.Deep, 0.03f, true);
	Spade(P.At(-0.26f, 0.0f, 0.24f), 0.0f, 0.0f, 1.0f, T.Core);
	PolyV(P, HeartPts(0.28f, 0.0f, 0.26f), T.Gem);
}

void GStar(const Pen& P, const Tone& T)
{
	const std::vector<Vec2> S = StarPts(0.0f, 0.04f, 0.95f, 0.4f, 5, 0.0f);
	for (size_t I = 0; I < S.size(); ++I)
	{
		const Vec2& A = S[I];
		const Vec2& B = S[(I + 1) % S.size()];
		PolyV(P, {{0.0f, 0.04f}, A, B}, I % 2 == 0 ? T.Body : T.Deep);
	}
	Glint(P, 0.62f, -0.62f, 0.16f, T.Gem);
}

void GStorm(const Pen& P, const Tone& T)
{
	Cloud(P, 0.0f, -0.3f, 1.0f, T.Body, T.Deep);
	Poly(P, {{0.08f, 0.1f}, {-0.24f, 0.56f}, {-0.02f, 0.56f}, {-0.14f, 0.96f}, {0.28f, 0.42f}, {0.05f, 0.42f}, {0.2f, 0.1f}}, T.Gem);
	Stroke(P, {{-0.56f, 0.24f}, {-0.64f, 0.46f}}, Fade(T.Body, 0.7f), 0.06f);
	Stroke(P, {{0.52f, 0.24f}, {0.44f, 0.46f}}, Fade(T.Body, 0.7f), 0.06f);
}

void GSkull(const Pen& P, const Tone& T)
{
	Oval(P, 0.0f, -0.18f, 0.68f, 0.62f, Lin(P, 0.0f, -0.8f, 0.0f, 0.4f, T.Body, T.Deep));
	Box(P, -0.38f, 0.22f, 0.76f, 0.48f, 0.14f, Lin(P, 0.0f, 0.22f, 0.0f, 0.7f, T.Body, T.Deep));
	Oval(P, -0.26f, -0.1f, 0.18f, 0.21f, T.Core);
	Oval(P, 0.26f, -0.1f, 0.18f, 0.21f, T.Core);
	Disc(P, -0.26f, -0.08f, 0.06f, T.Gem);
	Disc(P, 0.26f, -0.08f, 0.06f, T.Gem);
	Poly(P, {{0.0f, 0.12f}, {-0.09f, 0.28f}, {0.09f, 0.28f}}, T.Core);
	for (int K = 0; K < 4; ++K)
	{
		const float X = -0.18f + Fl(K) * 0.12f;
		Stroke(P, {{X, 0.46f}, {X, 0.62f}}, T.Core, 0.035f);
	}
	Oval(P, -0.28f, -0.5f, 0.16f, 0.08f, Fade(T.Glint, 0.5f));
}

void GTicket(const Pen& P, const Tone& T)
{
	std::vector<Vec2> S = {{-0.88f, -0.5f}, {0.88f, -0.5f}};
	for (const Vec2& Q : ArcPts(0.88f, 0.0f, 0.17f, 0.17f, -HalfPiF, -HalfPiF - PiF, 10))
	{
		S.push_back(Q);
	}
	S.push_back({0.88f, 0.5f});
	S.push_back({-0.88f, 0.5f});
	for (const Vec2& Q : ArcPts(-0.88f, 0.0f, 0.17f, 0.17f, HalfPiF, HalfPiF - PiF, 10))
	{
		S.push_back(Q);
	}
	PolyV(P, S, Lin(P, 0.0f, -0.5f, 0.0f, 0.5f, T.Body, T.Deep));
	for (int K = 0; K < 6; ++K)
	{
		Disc(P, 0.34f, -0.4f + Fl(K) * 0.16f, 0.035f, T.Deep);
	}
	PolyV(P, StarPts(-0.26f, 0.02f, 0.3f, 0.13f, 5, 0.0f), T.Gem);
	Box(P, 0.48f, -0.22f, 0.26f, 0.08f, 0.04f, T.Deep);
	Box(P, 0.48f, -0.04f, 0.2f, 0.08f, 0.04f, T.Deep);
	Box(P, 0.48f, 0.14f, 0.24f, 0.08f, 0.04f, T.Deep);
}

void GSnowflake(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 6; ++K)
	{
		const float A = -HalfPiF + TwoPiF * Fl(K) / 6.0f;
		const Vec2 E = Rot(0.92f, 0.0f, A);
		Stroke(P, {{0.0f, 0.0f}, {E.X, E.Y}}, T.Body, 0.11f);
		for (int B = 0; B < 2; ++B)
		{
			const float R = B == 0 ? 0.48f : 0.72f;
			const float L = B == 0 ? 0.24f : 0.16f;
			const Vec2 At = Rot(R, 0.0f, A);
			const Vec2 L1 = Rot(R + L * 0.7f, L * 0.7f, A);
			const Vec2 L2 = Rot(R + L * 0.7f, -L * 0.7f, A);
			Stroke(P, {{At.X, At.Y}, {L1.X, L1.Y}}, T.Body, 0.08f);
			Stroke(P, {{At.X, At.Y}, {L2.X, L2.Y}}, T.Body, 0.08f);
		}
		Disc(P, E.X, E.Y, 0.07f, T.Gem);
	}
	std::vector<Vec2> H;
	for (int K = 0; K < 6; ++K)
	{
		const float A = TwoPiF * Fl(K) / 6.0f;
		H.push_back({0.2f * std::cos(A), 0.2f * std::sin(A)});
	}
	PolyV(P, H, T.Gem);
	Disc(P, 0.0f, 0.0f, 0.08f, T.Glint);
}

void GRing(const Pen& P, const Tone& T)
{
	Hoop(P, 0.0f, 0.24f, 0.58f, 0.58f, T.Deep, 0.2f);
	Hoop(P, 0.0f, 0.22f, 0.58f, 0.58f, T.Body, 0.14f);
	Arc(P, 0.0f, 0.22f, 0.6f, PiF + 0.4f, PiF + 1.3f, Fade(T.Glint, 0.8f), 0.04f);
	Poly(P, {{-0.2f, -0.34f}, {0.2f, -0.34f}, {0.12f, -0.46f}, {-0.12f, -0.46f}}, T.Body);
	Gem(P, 0.0f, -0.66f, 0.36f, T.Gem);
	Glint(P, 0.46f, -0.78f, 0.13f, T.Glint);
}

void GFlower(const Pen& P, const Tone& T, int Petals)
{
	for (int K = 0; K < Petals; ++K)
	{
		const float A = TwoPiF * Fl(K) / Fl(Petals) - HalfPiF;
		const Vec2 C = Rot(0.46f, 0.0f, A);
		PolyV(P, Leaf(C.X, C.Y, 0.44f, 0.25f, A), Lin(P, 0.0f, 0.0f, C.X * 2.0f, C.Y * 2.0f, Lift(T.Gem, 0.55f), T.Gem));
		const Vec2 Vein = Rot(0.7f, 0.0f, A);
		Stroke(P, {{C.X * 0.55f, C.Y * 0.55f}, {Vein.X, Vein.Y}}, Fade(T.Glint, 0.45f), 0.03f);
	}
	Disc(P, 0.0f, 0.0f, 0.26f, Lin(P, 0.0f, -0.26f, 0.0f, 0.26f, T.Body, T.Deep));
	for (int K = 0; K < 6; ++K)
	{
		const Vec2 D = Rot(0.13f, 0.0f, TwoPiF * Fl(K) / 6.0f);
		Disc(P, D.X, D.Y, 0.035f, T.Core);
	}
}

void GBracelet(const Pen& P, const Tone& T)
{
	// A bangle seen from above at an angle, links all the way round, a gem at the front.
	const std::vector<Vec2> Back = ArcPts(0.0f, 0.0f, 0.82f, 0.46f, PiF, TwoPiF, 30);
	StrokeV(P, Back, Sink(T.Body, 0.25f), 0.2f);
	const std::vector<Vec2> Front = ArcPts(0.0f, 0.0f, 0.82f, 0.46f, 0.0f, PiF, 30);
	StrokeV(P, Front, T.Deep, 0.24f);
	StrokeV(P, Front, T.Body, 0.17f);
	for (int K = 1; K < 10; ++K)
	{
		const float A = PiF * Fl(K) / 10.0f;
		const float X = 0.82f * std::cos(A);
		const float Y = 0.46f * std::sin(A);
		Stroke(P, {{X * 0.9f, Y * 0.82f}, {X * 1.1f, Y * 1.18f}}, Fade(T.Deep, 0.85f), 0.03f);
	}
	Arc(P, 0.0f, 0.0f, 0.86f, 0.35f, 0.85f, Fade(T.Glint, 0.7f), 0.035f);
	Gem(P, 0.0f, 0.42f, 0.34f, T.Gem);
}

void GSkyline(const Pen& P, const Tone& T)
{
	struct B
	{
		float X, Top, W;
	};
	const B Bs[5] = {{-0.88f, 0.0f, 0.32f}, {-0.52f, -0.5f, 0.3f}, {-0.16f, -0.82f, 0.32f}, {0.2f, -0.32f, 0.3f}, {0.54f, -0.06f, 0.32f}};
	for (int I = 0; I < 5; ++I)
	{
		const B& Bd = Bs[I];
		Box(P, Bd.X, Bd.Top, Bd.W, 0.88f - Bd.Top, 0.02f, Lin(P, Bd.X, Bd.Top, Bd.X + Bd.W, 0.88f, T.Body, T.Deep));
		for (float Y = Bd.Top + 0.1f; Y < 0.8f; Y += 0.16f)
		{
			for (int C = 0; C < 2; ++C)
			{
				const bool Lit = ((I * 7 + C * 3 + static_cast<int>(Y * 40.0f)) % 3) != 0;
				Box(P, Bd.X + 0.06f + Fl(C) * 0.12f, Y, 0.07f, 0.07f, 0.01f, Lit ? T.Gem : Fade(T.Core, 0.6f));
			}
		}
	}
	Poly(P, {{-0.06f, -0.82f}, {0.0f, -1.0f}, {0.06f, -0.82f}}, T.Body);
	Box(P, -0.95f, 0.86f, 1.9f, 0.06f, 0.03f, T.Body);
}

void GSun(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 12; ++K)
	{
		const float A = TwoPiF * Fl(K) / 12.0f;
		const float R1 = K % 2 == 0 ? 0.98f : 0.84f;
		Poly(P, {{0.58f * std::cos(A - 0.12f), 0.58f * std::sin(A - 0.12f)}, {R1 * std::cos(A), R1 * std::sin(A)}, {0.58f * std::cos(A + 0.12f), 0.58f * std::sin(A + 0.12f)}},
			T.Body);
	}
	Disc(P, 0.0f, 0.0f, 0.52f, Lin(P, -0.4f, -0.5f, 0.4f, 0.5f, Lift(T.Gem, 0.45f), T.Gem));
	Hoop(P, 0.0f, 0.0f, 0.52f, 0.52f, Lift(T.Gem, 0.6f), 0.04f);
	Oval(P, -0.18f, -0.2f, 0.16f, 0.1f, Fade(T.Glint, 0.6f));
}

void GGift(const Pen& P, const Tone& T)
{
	Box(P, -0.7f, -0.12f, 1.4f, 0.98f, 0.06f, Lin(P, 0.0f, -0.12f, 0.0f, 0.86f, T.Body, T.Deep));
	Box(P, -0.8f, -0.38f, 1.6f, 0.3f, 0.06f, Lin(P, 0.0f, -0.38f, 0.0f, -0.08f, Lift(T.Body, 0.2f), T.Body));
	Box(P, -0.12f, -0.38f, 0.24f, 1.24f, 0.0f, T.Gem);
	Box(P, -0.8f, -0.3f, 1.6f, 0.1f, 0.0f, Fade(Sink(T.Gem, 0.1f), 0.9f));
	PolyV(P, Leaf(-0.28f, -0.56f, 0.3f, 0.15f, 0.45f), T.Gem);
	PolyV(P, Leaf(0.28f, -0.56f, 0.3f, 0.15f, -0.45f), T.Gem);
	PolyV(P, Leaf(-0.18f, -0.5f, 0.12f, 0.06f, 0.45f), Sink(T.Gem, 0.3f));
	PolyV(P, Leaf(0.18f, -0.5f, 0.12f, 0.06f, -0.45f), Sink(T.Gem, 0.3f));
	Disc(P, 0.0f, -0.46f, 0.11f, Lift(T.Gem, 0.25f));
}

void GMountain(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.98f, 0.78f}, {-0.36f, -0.32f}, {0.22f, 0.78f}}, T.Deep);
	Poly(P, {{-0.36f, -0.32f}, {-0.5f, -0.06f}, {-0.4f, -0.12f}, {-0.32f, -0.02f}, {-0.24f, -0.12f}, {-0.2f, -0.04f}}, Lift(T.Body, 0.4f));
	Poly(P, {{-0.46f, 0.78f}, {0.26f, -0.82f}, {0.98f, 0.78f}}, Lin(P, -0.2f, -0.8f, 0.6f, 0.8f, T.Body, T.Deep));
	Poly(P, {{0.26f, -0.82f}, {0.04f, -0.36f}, {0.16f, -0.44f}, {0.26f, -0.32f}, {0.38f, -0.46f}, {0.48f, -0.34f}}, T.Glint);
	Glint(P, -0.66f, -0.62f, 0.14f, T.Gem);
}

void GAurora(const Pen& P, const Tone& T)
{
	for (int B = 0; B < 3; ++B)
	{
		std::vector<Vec2> W;
		for (int I = 0; I <= 24; ++I)
		{
			const float X = -0.9f + 1.8f * Fl(I) / 24.0f;
			W.push_back({X, -0.5f + Fl(B) * 0.2f + 0.2f * std::sin(X * 2.4f + Fl(B) * 1.2f)});
		}
		StrokeV(P, W, B == 0 ? T.Gem : B == 1 ? Mix(T.Gem, T.Body, 0.5f) : Fade(T.Body, 0.75f), 0.13f);
	}
	Poly(P, {{-0.95f, 0.88f}, {-0.95f, 0.52f}, {-0.52f, 0.2f}, {-0.2f, 0.5f}, {0.18f, 0.08f}, {0.56f, 0.46f}, {0.95f, 0.3f}, {0.95f, 0.88f}}, T.Deep);
	Disc(P, -0.6f, -0.8f, 0.04f, T.Glint);
	Disc(P, 0.4f, -0.86f, 0.05f, T.Glint);
	Disc(P, 0.8f, -0.62f, 0.03f, T.Glint);
}

void Shard(const Pen& P, float U, float V, float H, float W, float A, const Tone& T)
{
	const std::vector<Vec2> L = Unit({{0.0f, -H}, {0.0f, H * 0.82f}, {-W, H * 0.6f}, {-W, -H * 0.6f}});
	const std::vector<Vec2> R = Unit({{0.0f, -H}, {W, -H * 0.6f}, {W, H * 0.6f}, {0.0f, H * 0.82f}});
	std::vector<Vec2> Lt;
	std::vector<Vec2> Rt;
	for (const Vec2& Q : Turned(L, A))
	{
		Lt.push_back({Q.X + U, Q.Y + V});
	}
	for (const Vec2& Q : Turned(R, A))
	{
		Rt.push_back({Q.X + U, Q.Y + V});
	}
	PolyV(P, Lt, Lift(T.Body, 0.2f));
	PolyV(P, Rt, T.Deep);
}

void GCrystal(const Pen& P, const Tone& T)
{
	Shard(P, -0.44f, 0.24f, 0.5f, 0.17f, -0.42f, T);
	Shard(P, 0.46f, 0.28f, 0.44f, 0.16f, 0.45f, T);
	Shard(P, 0.0f, -0.04f, 0.88f, 0.26f, 0.0f, T);
	Stroke(P, {{-0.1f, -0.6f}, {-0.1f, 0.3f}}, Fade(T.Glint, 0.6f), 0.04f);
	Glint(P, 0.3f, -0.72f, 0.16f, T.Gem);
	Box(P, -0.9f, 0.72f, 1.8f, 0.1f, 0.05f, Fade(T.Body, 0.6f));
}

void GThermometer(const Pen& P, const Tone& T)
{
	Box(P, -0.17f, -0.92f, 0.34f, 1.36f, 0.17f, T.Body);
	Disc(P, 0.0f, 0.54f, 0.32f, T.Body);
	Box(P, -0.07f, -0.36f, 0.14f, 0.86f, 0.07f, T.Gem);
	Disc(P, 0.0f, 0.54f, 0.21f, Lin(P, 0.0f, 0.3f, 0.0f, 0.8f, Lift(T.Gem, 0.3f), T.Gem));
	for (int K = 0; K < 5; ++K)
	{
		const float Y = -0.74f + Fl(K) * 0.22f;
		Stroke(P, {{0.24f, Y}, {K % 2 == 0 ? 0.44f : 0.36f, Y}}, T.Body, 0.05f);
	}
	Oval(P, -0.08f, 0.46f, 0.05f, 0.08f, Fade(T.Glint, 0.7f));
}

void GTopHat(const Pen& P, const Tone& T)
{
	// The brim behind, the crown, the band, and the front of the brim curling up.
	Oval(P, 0.0f, 0.4f, 0.94f, 0.24f, T.Deep);
	Box(P, -0.5f, -0.8f, 1.0f, 1.2f, 0.08f, Lin(P, -0.5f, 0.0f, 0.5f, 0.0f, T.Body, T.Deep));
	Oval(P, 0.0f, -0.8f, 0.5f, 0.12f, Lift(T.Body, 0.25f));
	Box(P, -0.5f, 0.1f, 1.0f, 0.2f, 0.0f, T.Gem);
	std::vector<Vec2> Lip = ArcPts(0.0f, 0.4f, 0.94f, 0.24f, 0.0f, PiF, 24);
	for (const Vec2& Q : ArcPts(0.0f, 0.36f, 0.5f, 0.1f, PiF, 0.0f, 16))
	{
		Lip.push_back(Q);
	}
	PolyV(P, Lip, Lin(P, 0.0f, 0.3f, 0.0f, 0.64f, T.Body, T.Deep));
	Stroke(P, {{-0.34f, -0.62f}, {-0.34f, -0.06f}}, Fade(T.Glint, 0.5f), 0.06f);
}

void GSevens(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 2; ++K)
	{
		const float D = K == 0 ? -0.4f : 0.4f;
		const std::vector<Vec2> Seven = Unit({{-0.32f, -0.64f}, {0.32f, -0.64f}, {0.32f, -0.46f}, {-0.02f, 0.66f}, {-0.26f, 0.66f}, {0.08f, -0.44f}, {-0.32f, -0.44f}});
		PolyV(P, Scaled(Seven, 1.0f, D + 0.04f, 0.05f), Fade(T.Core, 0.6f));
		PolyV(P, Scaled(Seven, 1.0f, D, 0.0f), Lin(P, 0.0f, -0.64f, 0.0f, 0.66f, Lift(T.Gem, 0.35f), T.Gem));
	}
	Glint(P, 0.72f, -0.78f, 0.14f, T.Glint);
}

void GAnteUp(const Pen& P, const Tone& T)
{
	Oval(P, 0.0f, 0.42f, 0.66f, 0.3f, Sink(T.Gem, 0.35f));
	Oval(P, 0.0f, 0.34f, 0.66f, 0.3f, Lin(P, 0.0f, 0.0f, 0.0f, 0.6f, Lift(T.Gem, 0.3f), T.Gem));
	Hoop(P, 0.0f, 0.34f, 0.46f, 0.2f, Fade(T.Glint, 0.8f), 0.05f);
	Poly(P, {{0.0f, -0.98f}, {0.44f, -0.46f}, {0.17f, -0.46f}, {0.17f, 0.0f}, {-0.17f, 0.0f}, {-0.17f, -0.46f}, {-0.44f, -0.46f}}, Lin(P, 0.0f, -0.98f, 0.0f, 0.0f, T.Body, T.Deep));
}

void GFirework(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 14; ++K)
	{
		const float A = TwoPiF * Fl(K) / 14.0f;
		const Color C = K % 2 == 0 ? T.Body : T.Gem;
		Stroke(P, {{0.28f * std::cos(A), 0.28f * std::sin(A)}, {0.78f * std::cos(A), 0.78f * std::sin(A)}}, C, 0.07f);
		Disc(P, 0.9f * std::cos(A), 0.9f * std::sin(A), 0.06f, C);
	}
	Disc(P, 0.0f, 0.0f, 0.14f, T.Glint);
	for (int K = 0; K < 7; ++K)
	{
		const float A = TwoPiF * (Fl(K) + 0.5f) / 7.0f;
		Disc(P, 0.52f * std::cos(A), 0.52f * std::sin(A), 0.04f, T.Glint);
	}
}

void GHorseshoe(const Pen& P, const Tone& T)
{
	Arc(P, 0.0f, 0.02f, 0.6f, -PiF / 3.0f, PiF * 4.0f / 3.0f, T.Deep, 0.32f, false);
	Arc(P, 0.0f, 0.0f, 0.6f, -PiF / 3.0f, PiF * 4.0f / 3.0f, T.Body, 0.26f, false);
	for (int K = 0; K < 7; ++K)
	{
		const float A = -PiF / 3.0f + 0.2f + Fl(K) * (PiF * 5.0f / 3.0f - 0.4f) / 6.0f;
		Disc(P, 0.6f * std::cos(A), 0.6f * std::sin(A), 0.04f, T.Core);
	}
	Glint(P, 0.0f, -0.66f, 0.2f, T.Gem);
}

void GCup(const Pen& P, const Tone& T)
{
	Arc(P, 0.46f, 0.2f, 0.26f, -HalfPiF, HalfPiF, T.Body, 0.13f);
	Box(P, -0.6f, -0.26f, 0.96f, 1.02f, 0.14f, Lin(P, -0.6f, 0.0f, 0.36f, 0.0f, T.Body, T.Deep));
	Oval(P, -0.12f, -0.24f, 0.44f, 0.1f, T.Gem);
	Stroke(P, {{-0.4f, -0.1f}, {-0.4f, 0.5f}}, Fade(T.Glint, 0.5f), 0.06f);
	for (int K = 0; K < 3; ++K)
	{
		const float X = -0.36f + Fl(K) * 0.24f;
		std::vector<Vec2> S;
		for (int I = 0; I <= 10; ++I)
		{
			const float U = Fl(I) / 10.0f;
			S.push_back({X + 0.06f * std::sin(U * TwoPiF + Fl(K)), -0.42f - 0.5f * U});
		}
		StrokeV(P, S, Fade(T.Body, 0.75f), 0.06f);
	}
}

void GSlingshot(const Pen& P, const Tone& T)
{
	Stroke(P, {{0.0f, 0.96f}, {0.0f, 0.2f}}, T.Deep, 0.22f);
	Stroke(P, {{0.0f, 0.26f}, {-0.46f, -0.56f}}, T.Body, 0.18f);
	Stroke(P, {{0.0f, 0.26f}, {0.46f, -0.56f}}, T.Body, 0.18f);
	Stroke(P, {{-0.46f, -0.52f}, {-0.1f, 0.0f}}, T.Gem, 0.06f);
	Stroke(P, {{0.46f, -0.52f}, {0.1f, 0.0f}}, T.Gem, 0.06f);
	Oval(P, 0.0f, 0.02f, 0.16f, 0.1f, Sink(T.Gem, 0.3f));
	Disc(P, 0.0f, -0.06f, 0.11f, T.Glint);
	Stroke(P, {{0.0f, 0.5f}, {0.0f, 0.8f}}, Fade(T.Body, 0.6f), 0.06f);
}

void GVault(const Pen& P, const Tone& T)
{
	Box(P, -0.86f, -0.86f, 1.72f, 1.72f, 0.18f, Lin(P, -0.86f, -0.86f, 0.86f, 0.86f, T.Body, T.Deep));
	Box(P, -0.94f, -0.52f, 0.12f, 0.26f, 0.04f, T.Deep);
	Box(P, -0.94f, 0.26f, 0.12f, 0.26f, 0.04f, T.Deep);
	Disc(P, 0.0f, 0.0f, 0.56f, T.Core);
	Hoop(P, 0.0f, 0.0f, 0.52f, 0.52f, T.Body, 0.06f);
	for (int K = 0; K < 16; ++K)
	{
		const float A = TwoPiF * Fl(K) / 16.0f;
		Stroke(P, {{0.4f * std::cos(A), 0.4f * std::sin(A)}, {0.47f * std::cos(A), 0.47f * std::sin(A)}}, Fade(T.Body, 0.8f), 0.03f);
	}
	for (int K = 0; K < 3; ++K)
	{
		const float A = -HalfPiF + TwoPiF * Fl(K) / 3.0f;
		Stroke(P, {{0.0f, 0.0f}, {0.32f * std::cos(A), 0.32f * std::sin(A)}}, T.Gem, 0.08f);
		Disc(P, 0.32f * std::cos(A), 0.32f * std::sin(A), 0.06f, T.Gem);
	}
	Disc(P, 0.0f, 0.0f, 0.1f, T.Glint);
}

void GBell(const Pen& P, const Tone& T)
{
	std::vector<Vec2> B = ArcPts(0.0f, -0.28f, 0.42f, 0.42f, PiF, TwoPiF, 16);
	B.push_back({0.5f, 0.2f});
	B.push_back({0.74f, 0.56f});
	B.push_back({-0.74f, 0.56f});
	B.push_back({-0.5f, 0.2f});
	PolyV(P, B, Lin(P, -0.6f, 0.0f, 0.6f, 0.0f, T.Body, T.Deep));
	Box(P, -0.8f, 0.5f, 1.6f, 0.16f, 0.08f, Lift(T.Body, 0.2f));
	Disc(P, 0.0f, 0.76f, 0.13f, T.Gem);
	PolyV(P, Leaf(-0.18f, -0.78f, 0.18f, 0.09f, 0.4f), T.Gem);
	PolyV(P, Leaf(0.18f, -0.78f, 0.18f, 0.09f, -0.4f), T.Gem);
	Stroke(P, {{-0.24f, -0.3f}, {-0.36f, 0.3f}}, Fade(T.Glint, 0.55f), 0.06f);
}

void GCandle(const Pen& P, const Tone& T)
{
	Glow(P, 0.0f, -0.56f, 0.5f, Fade(T.Gem, 0.5f));
	Box(P, -0.26f, -0.18f, 0.52f, 1.06f, 0.06f, Lin(P, -0.26f, 0.0f, 0.26f, 0.0f, T.Body, T.Deep));
	Oval(P, -0.14f, -0.12f, 0.08f, 0.2f, Lift(T.Body, 0.2f));
	Stroke(P, {{0.0f, -0.18f}, {0.0f, -0.3f}}, T.Core, 0.04f);
	Poly(P, {{0.0f, -0.88f}, {0.14f, -0.56f}, {0.1f, -0.38f}, {0.0f, -0.32f}, {-0.1f, -0.38f}, {-0.14f, -0.56f}}, T.Gem);
	Poly(P, {{0.0f, -0.66f}, {0.06f, -0.48f}, {0.0f, -0.4f}, {-0.06f, -0.48f}}, T.Glint);
	Oval(P, 0.0f, 0.88f, 0.44f, 0.08f, T.Deep);
}

void GHourglass(const Pen& P, const Tone& T)
{
	Box(P, -0.62f, -0.94f, 1.24f, 0.14f, 0.05f, T.Deep);
	Box(P, -0.62f, 0.8f, 1.24f, 0.14f, 0.05f, T.Deep);
	Poly(P, {{-0.48f, -0.8f}, {0.48f, -0.8f}, {0.06f, 0.0f}, {0.48f, 0.8f}, {-0.48f, 0.8f}, {-0.06f, 0.0f}}, Fade(T.Body, 0.9f));
	Poly(P, {{-0.26f, -0.4f}, {0.26f, -0.4f}, {0.0f, -0.06f}}, T.Gem);
	Poly(P, {{-0.42f, 0.8f}, {0.42f, 0.8f}, {0.0f, 0.4f}}, T.Gem);
	Stroke(P, {{0.0f, -0.04f}, {0.0f, 0.6f}}, T.Gem, 0.03f);
	Stroke(P, {{-0.56f, -0.8f}, {-0.56f, 0.8f}}, T.Deep, 0.06f);
	Stroke(P, {{0.56f, -0.8f}, {0.56f, 0.8f}}, T.Deep, 0.06f);
}

void GRainCloud(const Pen& P, const Tone& T)
{
	Cloud(P, 0.0f, -0.32f, 1.0f, T.Body, T.Deep);
	for (int K = 0; K < 4; ++K)
	{
		const float X = -0.48f + Fl(K) * 0.32f;
		Stroke(P, {{X + 0.06f, 0.24f}, {X - 0.04f, 0.5f}}, T.Gem, 0.07f);
		Stroke(P, {{X + 0.18f, 0.58f}, {X + 0.1f, 0.8f}}, T.Gem, 0.07f);
	}
}

void GDroplet(const Pen& P, const Tone& T)
{
	std::vector<Vec2> D = {{0.0f, -0.95f}};
	for (const Vec2& Q : ArcPts(0.0f, 0.24f, 0.56f, 0.56f, -0.62f, PiF + 0.62f, 26))
	{
		D.push_back(Q);
	}
	PolyV(P, D, Lin(P, -0.5f, -0.6f, 0.5f, 0.8f, Lift(T.Gem, 0.45f), Sink(T.Gem, 0.1f)));
	PolyV(P, Leaf(-0.22f, 0.12f, 0.1f, 0.24f, 0.35f), Fade(T.Glint, 0.75f));
	Hoop(P, 0.0f, 0.86f, 0.6f, 0.08f, Fade(T.Body, 0.6f), 0.04f);
}

void GCherries(const Pen& P, const Tone& T)
{
	std::vector<Vec2> S1;
	std::vector<Vec2> S2;
	for (int I = 0; I <= 12; ++I)
	{
		const float U = Fl(I) / 12.0f;
		S1.push_back({-0.34f + 0.44f * U, 0.2f - 0.88f * U + 0.2f * std::sin(U * PiF)});
		S2.push_back({0.32f - 0.22f * U, 0.28f - 0.96f * U + 0.1f * std::sin(U * PiF)});
	}
	StrokeV(P, S1, T.Deep, 0.06f);
	StrokeV(P, S2, T.Deep, 0.06f);
	PolyV(P, Leaf(0.36f, -0.66f, 0.3f, 0.13f, -0.35f), Lin(P, 0.1f, -0.8f, 0.6f, -0.5f, T.Body, T.Deep));
	Disc(P, -0.34f, 0.42f, 0.34f, Lin(P, -0.6f, 0.1f, -0.1f, 0.7f, Lift(T.Gem, 0.3f), Sink(T.Gem, 0.2f)));
	Disc(P, 0.32f, 0.5f, 0.32f, Lin(P, 0.0f, 0.2f, 0.6f, 0.8f, Lift(T.Gem, 0.3f), Sink(T.Gem, 0.2f)));
	Oval(P, -0.44f, 0.3f, 0.07f, 0.1f, Fade(T.Glint, 0.8f));
	Oval(P, 0.22f, 0.38f, 0.07f, 0.1f, Fade(T.Glint, 0.8f));
}

void GUmbrella(const Pen& P, const Tone& T)
{
	std::vector<Vec2> C = ArcPts(0.0f, 0.0f, 0.9f, 0.82f, PiF, TwoPiF, 26);
	for (int K = 4; K > 0; --K)
	{
		const float X0 = -0.9f + 0.45f * Fl(K);
		for (const Vec2& Q : ArcPts(X0 - 0.225f, 0.0f, 0.225f, 0.12f, 0.0f, -PiF, 6))
		{
			C.push_back({Q.X, -Q.Y});
		}
	}
	PolyV(P, C, Lin(P, 0.0f, -0.82f, 0.0f, 0.1f, Lift(T.Gem, 0.3f), T.Gem));
	for (int K = -1; K <= 1; ++K)
	{
		Stroke(P, {{0.0f, -0.82f}, {Fl(K) * 0.45f, 0.06f}}, Fade(Sink(T.Gem, 0.4f), 0.8f), 0.03f);
	}
	Stroke(P, {{0.0f, -0.82f}, {0.0f, 0.7f}}, T.Body, 0.07f);
	Arc(P, 0.15f, 0.7f, 0.15f, 0.0f, PiF, T.Body, 0.07f);
	Disc(P, 0.0f, -0.88f, 0.06f, T.Body);
}

void GColumns(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.96f, -0.46f}, {0.0f, -0.96f}, {0.96f, -0.46f}}, Lin(P, 0.0f, -0.96f, 0.0f, -0.46f, T.Body, T.Deep));
	Box(P, -0.9f, -0.46f, 1.8f, 0.14f, 0.02f, T.Body);
	for (int K = 0; K < 4; ++K)
	{
		const float X = -0.74f + Fl(K) * 0.44f;
		Box(P, X, -0.28f, 0.2f, 1.0f, 0.02f, Lin(P, X, 0.0f, X + 0.2f, 0.0f, T.Body, T.Deep));
		Stroke(P, {{X + 0.07f, -0.22f}, {X + 0.07f, 0.66f}}, Fade(T.Deep, 0.7f), 0.02f);
		Stroke(P, {{X + 0.13f, -0.22f}, {X + 0.13f, 0.66f}}, Fade(T.Deep, 0.7f), 0.02f);
	}
	Box(P, -0.96f, 0.72f, 1.92f, 0.18f, 0.03f, T.Body);
	Disc(P, 0.0f, -0.64f, 0.08f, T.Gem);
}

void GTower(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.3f, 0.92f}, {-0.2f, -0.46f}, {0.2f, -0.46f}, {0.3f, 0.92f}}, Lin(P, -0.3f, 0.0f, 0.3f, 0.0f, T.Body, T.Deep));
	Box(P, -0.32f, -0.6f, 0.64f, 0.16f, 0.03f, T.Body);
	Poly(P, {{-0.2f, -0.6f}, {0.0f, -0.8f}, {0.2f, -0.6f}}, T.Deep);
	Stroke(P, {{0.0f, -0.8f}, {0.0f, -0.99f}}, T.Body, 0.04f);
	Disc(P, 0.0f, -0.99f, 0.05f, T.Gem);
	for (int K = 0; K < 6; ++K)
	{
		Box(P, -0.04f, -0.34f + Fl(K) * 0.2f, 0.08f, 0.1f, 0.02f, K % 2 == 0 ? T.Gem : Fade(T.Core, 0.5f));
	}
	Box(P, -0.9f, 0.86f, 1.8f, 0.06f, 0.03f, Fade(T.Body, 0.6f));
}

void GCard(const Pen& P, const Tone& T)
{
	Box(P, -0.88f, -0.56f, 1.76f, 1.12f, 0.12f, Lin(P, -0.88f, -0.56f, 0.88f, 0.56f, Hex(0x2a2f3a), Hex(0x05070b)));
	Box(P, -0.88f, -0.56f, 1.76f, 1.12f, 0.12f, Fade(T.Body, 0.0f));
	P.Cv->StrokeRoundRect({P.X - 0.88f * P.S, P.Y - 0.56f * P.S, 1.76f * P.S, 1.12f * P.S}, 0.12f * P.S, Fade(T.Body, 0.5f), 0.03f * P.S);
	Box(P, -0.66f, -0.24f, 0.34f, 0.26f, 0.05f, Lin(P, -0.66f, -0.24f, -0.32f, 0.02f, Lift(T.Gem, 0.4f), T.Gem));
	Stroke(P, {{-0.66f, -0.11f}, {-0.32f, -0.11f}}, Sink(T.Gem, 0.4f), 0.02f);
	Stroke(P, {{-0.49f, -0.24f}, {-0.49f, 0.02f}}, Sink(T.Gem, 0.4f), 0.02f);
	for (int K = 0; K < 4; ++K)
	{
		Box(P, -0.66f + Fl(K) * 0.3f, 0.2f, 0.22f, 0.07f, 0.03f, Fade(T.Body, 0.7f));
	}
	Disc(P, 0.46f, 0.3f, 0.14f, Fade(T.Gem, 0.95f));
	Disc(P, 0.64f, 0.3f, 0.14f, Fade(T.Body, 0.7f));
}

void GRope(const Pen& P, const Tone& T)
{
	for (int K = -1; K <= 1; K += 2)
	{
		const float X = Fl(K) * 0.7f;
		Box(P, X - 0.07f, -0.36f, 0.14f, 1.1f, 0.05f, Lin(P, X - 0.07f, 0.0f, X + 0.07f, 0.0f, T.Body, T.Deep));
		Disc(P, X, -0.44f, 0.13f, T.Body);
		Oval(P, X, 0.78f, 0.28f, 0.08f, T.Deep);
	}
	std::vector<Vec2> R;
	for (int I = 0; I <= 20; ++I)
	{
		const float U = Fl(I) / 20.0f;
		const float X = -0.7f + 1.4f * U;
		R.push_back({X, -0.28f + 0.5f * (1.0f - (2.0f * U - 1.0f) * (2.0f * U - 1.0f))});
	}
	StrokeV(P, R, Sink(T.Gem, 0.3f), 0.15f);
	StrokeV(P, R, T.Gem, 0.1f);
}

void GOrnament(const Pen& P, const Tone& T)
{
	Disc(P, 0.0f, 0.16f, 0.74f, Lin(P, -0.6f, -0.5f, 0.6f, 0.9f, Lift(T.Gem, 0.35f), Sink(T.Gem, 0.25f)));
	std::vector<Vec2> W;
	for (int I = 0; I <= 20; ++I)
	{
		const float X = -0.7f + 1.4f * Fl(I) / 20.0f;
		W.push_back({X, 0.16f + 0.08f * std::sin(X * 9.0f)});
	}
	StrokeV(P, W, Fade(T.Body, 0.85f), 0.07f);
	for (int K = -2; K <= 2; ++K)
	{
		Disc(P, Fl(K) * 0.22f, -0.12f, 0.035f, T.Body);
		Disc(P, Fl(K) * 0.22f + 0.11f, 0.44f, 0.035f, T.Body);
	}
	Box(P, -0.2f, -0.74f, 0.4f, 0.2f, 0.04f, Lin(P, -0.2f, 0.0f, 0.2f, 0.0f, T.Body, T.Deep));
	Arc(P, 0.0f, -0.84f, 0.11f, PiF, TwoPiF, T.Body, 0.05f);
	PolyV(P, Leaf(-0.32f, -0.18f, 0.12f, 0.22f, 0.6f), Fade(T.Glint, 0.6f));
}

void GStocking(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.42f, -0.68f}, {0.18f, -0.68f}, {0.18f, 0.22f}, {0.56f, 0.38f}, {0.66f, 0.62f}, {0.52f, 0.86f}, {0.12f, 0.9f}, {-0.3f, 0.7f}, {-0.44f, 0.38f}},
		Lin(P, 0.0f, -0.6f, 0.0f, 0.9f, Lift(T.Gem, 0.2f), Sink(T.Gem, 0.2f)));
	Oval(P, 0.46f, 0.68f, 0.18f, 0.16f, Fade(T.Body, 0.9f));
	Box(P, -0.54f, -0.94f, 0.84f, 0.32f, 0.1f, Lin(P, 0.0f, -0.94f, 0.0f, -0.62f, T.Body, T.Deep));
	PolyV(P, StarPts(-0.12f, 0.12f, 0.17f, 0.07f, 5, 0.0f), T.Body);
	Arc(P, 0.42f, -0.82f, 0.14f, -HalfPiF, HalfPiF, T.Body, 0.04f);
}

void GGlass(const Pen& P, const Tone& T)
{
	Poly(P, {{-0.78f, -0.74f}, {0.78f, -0.74f}, {0.0f, 0.08f}}, Fade(T.Body, 0.92f));
	Poly(P, {{-0.54f, -0.5f}, {0.54f, -0.5f}, {0.0f, 0.06f}}, Lin(P, 0.0f, -0.5f, 0.0f, 0.06f, Lift(T.Gem, 0.3f), T.Gem));
	Box(P, -0.045f, 0.06f, 0.09f, 0.62f, 0.0f, T.Body);
	Oval(P, 0.0f, 0.7f, 0.44f, 0.1f, T.Body);
	Stroke(P, {{0.1f, -0.3f}, {0.5f, -0.96f}}, T.Deep, 0.04f);
	Disc(P, 0.26f, -0.56f, 0.12f, Lin(P, 0.14f, -0.68f, 0.38f, -0.44f, Hex(0xa3e635), Hex(0x3f6212)));
	Stroke(P, {{-0.5f, -0.66f}, {-0.2f, -0.66f}}, Fade(T.Glint, 0.7f), 0.04f);
}

void GClock(const Pen& P, const Tone& T, float Minute)
{
	Disc(P, 0.0f, 0.0f, 0.86f, Lin(P, 0.0f, -0.86f, 0.0f, 0.86f, T.Body, T.Deep));
	Disc(P, 0.0f, 0.0f, 0.72f, Lift(T.Body, 0.35f));
	for (int K = 0; K < 12; ++K)
	{
		const float A = TwoPiF * Fl(K) / 12.0f;
		const float R0 = K % 3 == 0 ? 0.48f : 0.58f;
		Stroke(P, {{R0 * std::cos(A), R0 * std::sin(A)}, {0.66f * std::cos(A), 0.66f * std::sin(A)}}, T.Core, K % 3 == 0 ? 0.07f : 0.04f);
	}
	const float Am = -HalfPiF + Minute * TwoPiF;
	Stroke(P, {{0.0f, 0.0f}, {0.0f, -0.4f}}, T.Core, 0.09f);
	Stroke(P, {{0.0f, 0.0f}, {0.58f * std::cos(Am), 0.58f * std::sin(Am)}}, T.Gem, 0.05f);
	Disc(P, 0.0f, 0.0f, 0.08f, T.Gem);
}

void GLock(const Pen& P, const Tone& T)
{
	Arc(P, 0.0f, -0.24f, 0.36f, PiF, TwoPiF, T.Body, 0.15f, false);
	Stroke(P, {{-0.36f, -0.24f}, {-0.36f, 0.0f}}, T.Body, 0.15f);
	Stroke(P, {{0.36f, -0.24f}, {0.36f, 0.0f}}, T.Body, 0.15f);
	Box(P, -0.58f, -0.08f, 1.16f, 0.98f, 0.16f, Lin(P, 0.0f, -0.08f, 0.0f, 0.9f, T.Body, T.Deep));
	Disc(P, 0.0f, 0.3f, 0.12f, T.Core);
	Poly(P, {{-0.06f, 0.34f}, {0.06f, 0.34f}, {0.09f, 0.62f}, {-0.09f, 0.62f}}, T.Core);
	Box(P, -0.46f, 0.0f, 0.92f, 0.07f, 0.03f, Fade(T.Glint, 0.45f));
}

void GMistletoe(const Pen& P, const Tone& T)
{
	PolyV(P, Leaf(-0.36f, 0.12f, 0.46f, 0.2f, 0.55f), Lin(P, -0.8f, -0.2f, 0.0f, 0.4f, T.Body, T.Deep));
	PolyV(P, Leaf(0.36f, 0.12f, 0.46f, 0.2f, -0.55f), Lin(P, 0.8f, -0.2f, 0.0f, 0.4f, T.Body, T.Deep));
	Stroke(P, {{-0.62f, -0.06f}, {-0.08f, 0.32f}}, Fade(T.Deep, 0.7f), 0.03f);
	Stroke(P, {{0.62f, -0.06f}, {0.08f, 0.32f}}, Fade(T.Deep, 0.7f), 0.03f);
	Disc(P, -0.12f, 0.46f, 0.15f, T.Glint);
	Disc(P, 0.14f, 0.48f, 0.15f, T.Glint);
	Disc(P, 0.0f, 0.68f, 0.15f, T.Glint);
	Disc(P, -0.1f, 0.44f, 0.03f, T.Deep);
	PolyV(P, Leaf(-0.26f, -0.62f, 0.24f, 0.13f, 0.35f), T.Gem);
	PolyV(P, Leaf(0.26f, -0.62f, 0.24f, 0.13f, -0.35f), T.Gem);
	Disc(P, 0.0f, -0.56f, 0.09f, Sink(T.Gem, 0.2f));
	Stroke(P, {{0.0f, -0.5f}, {0.0f, 0.2f}}, T.Gem, 0.04f);
}

void GEquinox(const Pen& P, const Tone& T)
{
	for (int K = 0; K < 7; ++K)
	{
		const float A = HalfPiF + 0.25f + Fl(K) * (PiF - 0.5f) / 6.0f;
		Poly(P, {{0.56f * std::cos(A - 0.1f), 0.56f * std::sin(A - 0.1f)}, {0.9f * std::cos(A), 0.9f * std::sin(A)}, {0.56f * std::cos(A + 0.1f), 0.56f * std::sin(A + 0.1f)}}, T.Gem);
	}
	PolyV(P, ArcPts(0.0f, 0.0f, 0.5f, 0.5f, HalfPiF, HalfPiF + PiF, 22), Lin(P, -0.5f, 0.0f, 0.0f, 0.0f, Lift(T.Gem, 0.4f), T.Gem));
	PolyV(P, ArcPts(0.0f, 0.0f, 0.5f, 0.5f, -HalfPiF, HalfPiF, 22), Lin(P, 0.0f, 0.0f, 0.5f, 0.0f, T.Body, T.Deep));
	Disc(P, 0.24f, -0.12f, 0.06f, Fade(T.Deep, 0.8f));
	Disc(P, 0.3f, 0.2f, 0.08f, Fade(T.Deep, 0.8f));
	PolyV(P, StarPts(0.72f, -0.6f, 0.13f, 0.06f, 5, 0.0f), T.Body);
	PolyV(P, StarPts(0.78f, 0.44f, 0.09f, 0.04f, 5, 0.3f), T.Body);
	Stroke(P, {{0.0f, -0.96f}, {0.0f, 0.96f}}, Fade(T.Glint, 0.6f), 0.03f);
}

void GTrafficLight(const Pen& P, const Tone& T)
{
	Box(P, -0.36f, -0.92f, 0.72f, 1.84f, 0.2f, Lin(P, -0.36f, 0.0f, 0.36f, 0.0f, T.Deep, T.Core));
	const Color Lamps[3] = {Hex(0x7f1d1d), Hex(0x78350f), Hex(0x22c55e)};
	for (int K = 0; K < 3; ++K)
	{
		const float Y = -0.56f + Fl(K) * 0.56f;
		if (K == 2)
		{
			Glow(P, 0.0f, Y, 0.5f, Fade(Hex(0x4ade80), 0.55f));
		}
		Disc(P, 0.0f, Y, 0.21f, Lamps[K]);
		Oval(P, -0.06f, Y - 0.07f, 0.07f, 0.04f, Fade(T.Glint, K == 2 ? 0.8f : 0.25f));
	}
}

void GTrophy(const Pen& P, const Tone& T)
{
	Arc(P, -0.52f, -0.42f, 0.24f, HalfPiF, HalfPiF + PiF, T.Deep, 0.1f);
	Arc(P, 0.52f, -0.42f, 0.24f, -HalfPiF, HalfPiF, T.Deep, 0.1f);
	std::vector<Vec2> Cup = {{-0.6f, -0.78f}, {0.6f, -0.78f}};
	for (const Vec2& Q : ArcPts(0.0f, -0.6f, 0.6f, 0.62f, 0.0f, PiF, 16))
	{
		Cup.push_back(Q);
	}
	PolyV(P, Cup, Lin(P, -0.6f, -0.78f, 0.6f, 0.0f, Lift(T.Gem, 0.45f), Sink(T.Gem, 0.15f)));
	Box(P, -0.08f, 0.0f, 0.16f, 0.36f, 0.0f, T.Gem);
	Box(P, -0.34f, 0.32f, 0.68f, 0.12f, 0.03f, T.Gem);
	Box(P, -0.48f, 0.44f, 0.96f, 0.36f, 0.06f, Lin(P, 0.0f, 0.44f, 0.0f, 0.8f, T.Body, T.Deep));
	Box(P, -0.32f, 0.54f, 0.64f, 0.14f, 0.03f, Lift(T.Gem, 0.2f));
	PolyV(P, StarPts(0.0f, -0.42f, 0.22f, 0.1f, 5, 0.0f), Lift(T.Gem, 0.75f));
	Oval(P, -0.32f, -0.52f, 0.07f, 0.2f, Fade(T.Glint, 0.55f));
}

void GSpade(const Pen& P, const Tone& T)
{
	Spade(P, 0.0f, -0.02f, 0.92f, Lin(P, 0.0f, -0.9f, 0.0f, 0.9f, T.Body, T.Deep));
	Oval(P, -0.3f, -0.3f, 0.08f, 0.16f, Fade(T.Glint, 0.5f));
}

void GSprout(const Pen& P, const Tone& T)
{
	Oval(P, 0.0f, 0.66f, 0.62f, 0.2f, Sink(T.Gem, 0.35f));
	Oval(P, 0.0f, 0.58f, 0.62f, 0.2f, Lin(P, 0.0f, 0.4f, 0.0f, 0.8f, Lift(T.Gem, 0.35f), T.Gem));
	Hoop(P, 0.0f, 0.58f, 0.44f, 0.13f, Fade(T.Glint, 0.8f), 0.04f);
	Stroke(P, {{0.0f, 0.5f}, {0.0f, -0.3f}}, T.Body, 0.08f);
	PolyV(P, Blade(-0.34f, -0.36f, 0.36f, 0.17f, -0.5f), Lin(P, -0.7f, -0.6f, 0.0f, -0.2f, T.Body, T.Deep));
	PolyV(P, Blade(0.3f, -0.62f, 0.32f, 0.15f, 0.62f), Lin(P, 0.6f, -0.9f, 0.0f, -0.4f, T.Body, T.Deep));
}

void Draw(const Pen& P, Glyph G, const Tone& T)
{
	switch (G)
	{
	case Glyph::Owl: GOwl(P, T); break;
	case Glyph::Bolt: GBolt(P, T); break;
	case Glyph::Moon: GMoon(P, T, true); break;
	case Glyph::Eye: GEye(P, T); break;
	case Glyph::Crosshair: GCrosshair(P, T); break;
	case Glyph::Gear: GGear(P, T); break;
	case Glyph::Crown: GCrown(P, T); break;
	case Glyph::Diamond: GDiamond(P, T); break;
	case Glyph::Wave: GWave(P, T); break;
	case Glyph::Envelope: GEnvelope(P, T); break;
	case Glyph::Coin: GCoin(P, T); break;
	case Glyph::Stopwatch: GStopwatch(P, T); break;
	case Glyph::Fan: GFan(P, T); break;
	case Glyph::Chips: GChips(P, T); break;
	case Glyph::Bricks: GBricks(P, T); break;
	case Glyph::Tombstone: GTombstone(P, T); break;
	case Glyph::Sunrise: GSunrise(P, T); break;
	case Glyph::Claw: GClaw(P, T); break;
	case Glyph::Stairs: GStairs(P, T); break;
	case Glyph::Spiral: GSpiral(P, T); break;
	case Glyph::Rocket: GRocket(P, T); break;
	case Glyph::Flame: GFlame(P, T); break;
	case Glyph::Glove: GGlove(P, T); break;
	case Glyph::SixSeats: GSixSeats(P, T); break;
	case Glyph::Laurel: GLaurelM(P, T); break;
	case Glyph::Showdown: GShowdown(P, T); break;
	case Glyph::Star: GStar(P, T); break;
	case Glyph::Storm: GStorm(P, T); break;
	case Glyph::Skull: GSkull(P, T); break;
	case Glyph::Ticket: GTicket(P, T); break;
	case Glyph::Snowflake: GSnowflake(P, T); break;
	case Glyph::Ring: GRing(P, T); break;
	case Glyph::Flower: GFlower(P, T, 6); break;
	case Glyph::Bracelet: GBracelet(P, T); break;
	case Glyph::Skyline: GSkyline(P, T); break;
	case Glyph::Sun: GSun(P, T); break;
	case Glyph::Gift: GGift(P, T); break;
	case Glyph::Mountain: GMountain(P, T); break;
	case Glyph::Aurora: GAurora(P, T); break;
	case Glyph::Crystal: GCrystal(P, T); break;
	case Glyph::Thermometer: GThermometer(P, T); break;
	case Glyph::TopHat: GTopHat(P, T); break;
	case Glyph::Sevens: GSevens(P, T); break;
	case Glyph::AnteUp: GAnteUp(P, T); break;
	case Glyph::Firework: GFirework(P, T); break;
	case Glyph::Horseshoe: GHorseshoe(P, T); break;
	case Glyph::Cup: GCup(P, T); break;
	case Glyph::Slingshot: GSlingshot(P, T); break;
	case Glyph::Vault: GVault(P, T); break;
	case Glyph::Bell: GBell(P, T); break;
	case Glyph::Candle: GCandle(P, T); break;
	case Glyph::Hourglass: GHourglass(P, T); break;
	case Glyph::RainCloud: GRainCloud(P, T); break;
	case Glyph::Droplet: GDroplet(P, T); break;
	case Glyph::Cherries: GCherries(P, T); break;
	case Glyph::Umbrella: GUmbrella(P, T); break;
	case Glyph::Columns: GColumns(P, T); break;
	case Glyph::Tower: GTower(P, T); break;
	case Glyph::Card: GCard(P, T); break;
	case Glyph::Rope: GRope(P, T); break;
	case Glyph::Ornament: GOrnament(P, T); break;
	case Glyph::Stocking: GStocking(P, T); break;
	case Glyph::Glass: GGlass(P, T); break;
	case Glyph::Clock: GClock(P, T, 0.92f); break;
	case Glyph::Lock: GLock(P, T); break;
	case Glyph::Mistletoe: GMistletoe(P, T); break;
	case Glyph::Equinox: GEquinox(P, T); break;
	case Glyph::TrafficLight: GTrafficLight(P, T); break;
	case Glyph::Trophy: GTrophy(P, T); break;
	case Glyph::Spade: GSpade(P, T); break;
	case Glyph::Sprout: GSprout(P, T); break;
	case Glyph::Count: break;
	}
}

// ------------------------------------------------------------------ tiles and crests

/** Precious metals for rims: the stakes, at a glance. */
struct Metal
{
	Color Hi;
	Color Mid;
	Color Lo;
};

enum class Alloy : int
{
	Bronze,
	Silver,
	Gold,
	Platinum,
};

Metal MetalOf(Alloy A)
{
	switch (A)
	{
	case Alloy::Bronze: return {Hex(0xffd9b3), Hex(0xcd8a52), Hex(0x5e3216)};
	case Alloy::Silver: return {Hex(0xffffff), Hex(0xc3ccd8), Hex(0x515e70)};
	case Alloy::Gold: return {Hex(0xfff4c6), Hex(0xf2c14e), Hex(0x7a4e0e)};
	case Alloy::Platinum: return {Hex(0xffffff), Hex(0xd6defa), Hex(0x5a6396)};
	}
	return {Hex(0xffffff), Hex(0xc3ccd8), Hex(0x515e70)};
}

Alloy AlloyFor(Chips BuyInCents)
{
	switch (net::TierOf(BuyInCents))
	{
	case net::Tier::Freeroll:
	case net::Tier::Micro: return Alloy::Bronze;
	case net::Tier::Low: return Alloy::Silver;
	case net::Tier::Mid: return Alloy::Gold;
	case net::Tier::High: return Alloy::Platinum;
	}
	return Alloy::Silver;
}

enum class Frame : int
{
	Shield,   // RCOP
	Hexagon,  // Micro Madness
	Sunburst, // Summer Slam
	Medallion, // Ring Rush, The Championship Online
	Rhombus,  // the high rollers' July
	Snowcut,  // winter
	Scallop,  // spring
	Ornament, // the holidays
};

/** "win27" -> "win". */
std::string SlotOf(const std::string& SeriesId)
{
	size_t N = 0;
	while (N < SeriesId.size() && std::isalpha(static_cast<unsigned char>(SeriesId[N])))
	{
		++N;
	}
	return SeriesId.substr(0, N);
}

Frame FrameOf(const std::string& Slot)
{
	return Slot == "rcop" ? Frame::Shield
		: Slot == "mm"	  ? Frame::Hexagon
		: Slot == "slam"  ? Frame::Sunburst
		: Slot == "ring" || Slot == "tco" ? Frame::Medallion
		: Slot == "hrs"	  ? Frame::Rhombus
		: Slot == "win"	  ? Frame::Snowcut
		: Slot == "spr"	  ? Frame::Scallop
		: Slot == "hol"	  ? Frame::Ornament
						  : Frame::Medallion;
}

std::vector<Vec2> Bezier(Vec2 A, Vec2 C, Vec2 B, int N)
{
	std::vector<Vec2> Out;
	for (int I = 1; I <= N; ++I)
	{
		const float T = Fl(I) / Fl(N);
		const float U = 1.0f - T;
		Out.push_back({U * U * A.X + 2.0f * U * T * C.X + T * T * B.X, U * U * A.Y + 2.0f * U * T * C.Y + T * T * B.Y});
	}
	return Out;
}

/** The frame's outline, K times its full size. */
std::vector<Vec2> FramePts(Frame F, float K)
{
	std::vector<Vec2> P;
	switch (F)
	{
	case Frame::Shield:
	{
		P = {{-0.86f, -0.9f}, {-0.3f, -0.9f}, {0.0f, -1.0f}, {0.3f, -0.9f}, {0.86f, -0.9f}, {0.86f, -0.12f}};
		for (const Vec2& Q : Bezier({0.86f, -0.12f}, {0.86f, 0.62f}, {0.0f, 1.02f}, 14))
		{
			P.push_back(Q);
		}
		for (const Vec2& Q : Bezier({0.0f, 1.02f}, {-0.86f, 0.62f}, {-0.86f, -0.12f}, 14))
		{
			P.push_back(Q);
		}
		break;
	}
	case Frame::Hexagon:
		for (int I = 0; I < 6; ++I)
		{
			const float A = -HalfPiF + TwoPiF * Fl(I) / 6.0f;
			P.push_back({0.98f * std::cos(A), 0.98f * std::sin(A)});
		}
		break;
	case Frame::Rhombus:
		P = {{-0.07f, -0.95f}, {0.07f, -0.95f}, {0.9f, -0.07f}, {0.9f, 0.07f}, {0.07f, 0.95f}, {-0.07f, 0.95f}, {-0.9f, 0.07f}, {-0.9f, -0.07f}};
		break;
	case Frame::Snowcut:
		for (int I = 0; I < 8; ++I)
		{
			const float A = -HalfPiF + TwoPiF * Fl(I) / 8.0f;
			P.push_back({0.95f * std::cos(A - 0.09f), 0.95f * std::sin(A - 0.09f)});
			P.push_back({0.84f * std::cos(A), 0.84f * std::sin(A)});
			P.push_back({0.95f * std::cos(A + 0.09f), 0.95f * std::sin(A + 0.09f)});
		}
		break;
	case Frame::Scallop:
		for (int I = 0; I < 96; ++I)
		{
			const float A = TwoPiF * Fl(I) / 96.0f;
			const float R = 0.88f + 0.07f * std::cos(12.0f * A);
			P.push_back({R * std::cos(A), R * std::sin(A)});
		}
		break;
	case Frame::Ornament:
		P = ArcPts(0.0f, 0.06f, 0.9f, 0.9f, 0.0f, TwoPiF - TwoPiF / 72.0f, 71);
		break;
	case Frame::Sunburst:
	case Frame::Medallion:
		P = ArcPts(0.0f, 0.0f, 0.92f, 0.92f, 0.0f, TwoPiF - TwoPiF / 72.0f, 71);
		break;
	}
	return Scaled(P, K);
}

/** How far guilloche rings can reach inside the frame's field. */
float InnerReach(Frame F)
{
	return F == Frame::Rhombus ? 0.5f : F == Frame::Shield ? 0.66f : 0.7f;
}

/** A ribbon across the crest at height V with Text on it (the text is skipped when it would be too small to read). */
void Ribbon(const Pen& P, float V, float HalfW, const Color& Cloth, const Metal& M, const std::string& Text)
{
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float Sd = Fl(Side);
		Poly(P, {{Sd * (HalfW - 0.1f), V + 0.04f}, {Sd * (HalfW + 0.2f), V + 0.08f}, {Sd * (HalfW + 0.1f), V + 0.19f}, {Sd * (HalfW + 0.24f), V + 0.33f}, {Sd * (HalfW - 0.06f), V + 0.29f}},
			Lin(P, 0.0f, V, 0.0f, V + 0.33f, Sink(Cloth, 0.35f), Sink(Cloth, 0.55f)));
		Poly(P, {{Sd * (HalfW - 0.06f), V + 0.24f}, {Sd * (HalfW - 0.06f), V + 0.29f}, {Sd * (HalfW + 0.04f), V + 0.27f}}, Sink(Cloth, 0.7f));
	}
	std::vector<Vec2> Band;
	std::vector<Vec2> Top;
	std::vector<Vec2> Bottom;
	const int N = 16;
	for (int I = 0; I <= N; ++I)
	{
		const float X = -HalfW + 2.0f * HalfW * Fl(I) / Fl(N);
		const float Y = V - 0.06f * (1.0f - (X / HalfW) * (X / HalfW));
		Top.push_back({X, Y});
		Bottom.push_back({X, Y + 0.26f});
	}
	Band = Top;
	for (auto It = Bottom.rbegin(); It != Bottom.rend(); ++It)
	{
		Band.push_back(*It);
	}
	PolyV(P, Band, Lin(P, 0.0f, V - 0.06f, 0.0f, V + 0.26f, Lift(Cloth, 0.12f), Sink(Cloth, 0.25f)));
	StrokeV(P, Top, Fade(M.Hi, 0.9f), 0.025f);
	StrokeV(P, Bottom, Fade(M.Mid, 0.85f), 0.025f);
	// The lettering shrinks to fit the band (and is left off when it would be too small to read).
	const float Room = 2.0f * HalfW * P.S * 0.84f;
	float Px = 0.2f * P.S;
	const float Wide = P.Cv->Measure(Text, Px, 900);
	if (Wide > Room)
	{
		Px *= Room / Wide;
	}
	if (!Text.empty() && Px >= 7.0f)
	{
		TextStyle St;
		St.Size = Px;
		St.Weight = 900;
		St.Col = Sink(Cloth, 0.75f);
		St.HAlign = Align::Center;
		St.VAlign = Baseline::Middle;
		P.Cv->Text(Text, P.X + 0.012f * P.S, P.Y + (V + 0.11f) * P.S + 1.0f, St);
		St.Col = M.Hi;
		P.Cv->Text(Text, P.X, P.Y + (V + 0.1f) * P.S, St);
	}
}

/** Twinkling glints around a crest (each on its own beat). */
void Sparkles(const Pen& P, double Time, int Count, float Reach)
{
	static const float Spots[6][2] = {{-0.74f, -0.66f}, {0.8f, -0.38f}, {0.58f, 0.8f}, {-0.86f, 0.36f}, {0.2f, -1.02f}, {-0.3f, 0.98f}};
	for (int I = 0; I < Count && I < 6; ++I)
	{
		const float A = 0.5f + 0.5f * Fl(std::sin(Time * (1.7 + 0.37 * I) + 2.1 * I));
		if (A < 0.15f)
		{
			continue;
		}
		Glint(P, Spots[I][0] * Reach, Spots[I][1] * Reach, 0.06f + 0.1f * A, Fade(Hex(0xffffff), A));
		Disc(P, Spots[I][0] * Reach, Spots[I][1] * Reach, 0.03f, Fade(Hex(0xffffff), A));
	}
}

/** A bracelet wrapped around the crest: the back of the band (drawn before the crest) or the front (after). */
void BraceletBand(const Pen& P, const Metal& M, const Color& Stone, bool Front)
{
	const float Cy = 0.42f;
	const float Rx = 1.16f;
	const float Ry = 0.34f;
	if (!Front)
	{
		const std::vector<Vec2> Back = ArcPts(0.0f, Cy, Rx, Ry, PiF + 0.08f, TwoPiF - 0.08f, 36);
		StrokeV(P, Back, Sink(M.Lo, 0.2f), 0.2f);
		StrokeV(P, Back, Sink(M.Mid, 0.35f), 0.13f);
		return;
	}
	const std::vector<Vec2> Arc0 = ArcPts(0.0f, Cy, Rx, Ry, -0.05f, PiF + 0.05f, 40);
	StrokeV(P, Arc0, M.Lo, 0.22f);
	StrokeV(P, Arc0, M.Mid, 0.15f);
	StrokeV(P, ArcPts(0.0f, Cy - 0.03f, Rx, Ry, 0.3f, PiF - 0.3f, 30), Fade(M.Hi, 0.85f), 0.04f);
	for (int K = 1; K < 14; ++K)
	{
		const float A = PiF * Fl(K) / 14.0f;
		const float X = Rx * std::cos(A);
		const float Y = Cy + Ry * std::sin(A);
		Stroke(P, {{X * 0.95f, Y - 0.08f}, {X * 1.03f, Y + 0.08f}}, Fade(M.Lo, 0.75f), 0.025f);
	}
	Oval(P, 0.0f, Cy + Ry, 0.2f, 0.15f, Lin(P, -0.2f, 0.0f, 0.2f, 0.0f, M.Hi, M.Lo));
	Gem(P, 0.0f, Cy + Ry + 0.02f, 0.22f, Stone);
}

/** A ring hanging over the crest (a Grand Circuit ring event). */
void HangingRing(const Pen& P, const Metal& M, const Color& Stone)
{
	Hoop(P, 0.0f, -1.08f, 0.26f, 0.26f, M.Lo, 0.13f);
	Hoop(P, 0.0f, -1.09f, 0.26f, 0.26f, M.Mid, 0.085f);
	Arc(P, 0.0f, -1.09f, 0.27f, PiF + 0.3f, PiF + 1.3f, Fade(M.Hi, 0.9f), 0.03f);
	Poly(P, {{-0.12f, -1.3f}, {0.12f, -1.3f}, {0.07f, -1.38f}, {-0.07f, -1.38f}}, M.Mid);
	Gem(P, 0.0f, -1.5f, 0.24f, Stone);
}

/** A diamond set on top of the crest (a high roller). */
void DiamondMount(const Pen& P, const Metal& M)
{
	Poly(P, {{-0.22f, -0.86f}, {0.22f, -0.86f}, {0.14f, -1.0f}, {-0.14f, -1.0f}}, Lin(P, -0.22f, 0.0f, 0.22f, 0.0f, M.Hi, M.Lo));
	Stroke(P, {{-0.2f, -1.0f}, {-0.3f, -1.16f}}, M.Mid, 0.05f);
	Stroke(P, {{0.2f, -1.0f}, {0.3f, -1.16f}}, M.Mid, 0.05f);
	Gem(P, 0.0f, -1.2f, 0.44f, Hex(0x9ee7ff));
}

/** The rays behind a Main Event, turning slowly. */
void Rays(const Pen& P, const Metal& M, double Time, float Reach)
{
	const float Turn = Fl(std::fmod(Time * 0.12, static_cast<double>(TwoPiF)));
	for (int K = 0; K < 24; ++K)
	{
		const float A = Turn + TwoPiF * Fl(K) / 24.0f;
		const float R1 = (K % 2 == 0 ? 1.55f : 1.28f) * Reach;
		const float W = K % 2 == 0 ? 0.055f : 0.04f;
		Poly(P, {{0.5f * std::cos(A - W), 0.5f * std::sin(A - W)}, {R1 * std::cos(A), R1 * std::sin(A)}, {0.5f * std::cos(A + W), 0.5f * std::sin(A + W)}},
			Paint::Linear(P.P(0.5f * std::cos(A), 0.5f * std::sin(A)), P.P(R1 * std::cos(A), R1 * std::sin(A)), Fade(K % 2 == 0 ? M.Hi : M.Mid, 0.75f), Fade(M.Mid, 0.0f)));
	}
	Glow(P, 0.0f, 0.0f, 1.5f * Reach, Fade(M.Mid, 0.5f));
}

struct CrestLook
{
	Frame Shape = Frame::Medallion;
	Alloy Rim = Alloy::Silver;
	Color Field = Hex(0x27d3c3);
	Color Field2 = Hex(0x3b82f6);
	Glyph Motif = Glyph::Spade;
	Glyph Seal = Glyph::Count; // the event's format, in a small seal (Count: none)
	std::string Banner;
	bool Main = false;      // rays, laurel, crown
	bool Laurel = false;    // laurel without the rest (RCOP's own crest)
	bool Crown = false;
	bool Bracelet = false;
	bool Ring = false;
	bool HighRoller = false;
};

// The field's enamel. The gold flagships get their own (RCOP's royal purple, the Championship's royal blue, Ring
// Rush's garnet) so the gold of their motif stands out; the rest wear the series' colours.
void Enamel(CrestLook& L, const std::string& Slot, uint32_t A, uint32_t B)
{
	L.Field = Slot == "rcop" ? Hex(0x5b2bc4) : Slot == "tco" ? Hex(0x2147a8) : Slot == "ring" ? Hex(0xa3173e) : Hex(A);
	L.Field2 = Slot == "tco" ? Hex(0x0a1747) : Slot == "ring" ? Hex(0x3d0618) : Hex(B);
}

void Crest(Canvas& C, const CrestLook& L, float Cx, float Cy, float Size, double Time)
{
	// Small crests (a schedule row, a chip) drop the banner and the finer work, and fill more of their box.
	const bool Rich = Size >= 44.0f;
	const Pen P{&C, Cx, Cy + Size * (L.Ring ? 0.07f : L.Main || L.HighRoller || L.Crown ? 0.04f : 0.0f), Size * (Rich ? 0.3f : 0.36f)};
	const Metal M = MetalOf(L.Rim);
	// A light field (gold, cream) would swallow an embossed motif of the same metal: those sink to a deep enamel.
	const float Light = 0.3f * L.Field.R + 0.59f * L.Field.G + 0.11f * L.Field.B;
	const float Deep = Light > 0.55f ? 0.55f : 0.08f;
	const Color Field = Sink(L.Field, Deep);
	const Color Field2 = Sink(L.Field2, std::max(0.48f, Deep + 0.2f));
	const Color Stone = Lift(L.Field, 0.1f);

	// Behind: shadow, rays, laurel, the back of a bracelet, the sun's points.
	Oval(P, 0.0f, 0.12f, 1.05f, 1.0f, Paint::Radial(P.P(0.0f, 0.12f), 0.0f, P.P(0.0f, 0.12f), 1.05f * P.S, Rgba(0, 0, 0, 0.5f), 0.6f, Rgba(0, 0, 0, 0.25f), Rgba(0, 0, 0, 0.0f)));
	if (L.Main && Rich)
	{
		Rays(P, M, Time, 1.0f);
	}
	if (L.Main || L.Laurel)
	{
		const Color Leaf1 = L.Rim == Alloy::Platinum ? M.Hi : Lift(M.Mid, 0.12f);
		Laurel(P, 1.06f, HalfPiF + 0.3f, HalfPiF + 2.5f, Leaf1, M.Lo, M.Lo, Rich ? 8 : 5);
		Laurel(P, 1.06f, HalfPiF - 0.3f, HalfPiF - 2.5f, Leaf1, M.Lo, M.Lo, Rich ? 8 : 5);
	}
	if (L.Bracelet)
	{
		BraceletBand(P, M, Stone, false);
	}
	if (L.Shape == Frame::Sunburst)
	{
		for (int K = 0; K < 16; ++K)
		{
			const float A = TwoPiF * Fl(K) / 16.0f + Fl(Time * 0.05);
			const float R1 = K % 2 == 0 ? 1.24f : 1.1f;
			Poly(P, {{0.86f * std::cos(A - 0.13f), 0.86f * std::sin(A - 0.13f)}, {R1 * std::cos(A), R1 * std::sin(A)}, {0.86f * std::cos(A + 0.13f), 0.86f * std::sin(A + 0.13f)}},
				Lin(P, 0.0f, -1.2f, 0.0f, 1.2f, K % 2 == 0 ? M.Hi : M.Mid, M.Lo));
		}
	}
	if (L.Shape == Frame::Hexagon)
	{
		StrokeV(P, FramePts(Frame::Hexagon, 1.1f), Fade(Lift(L.Field, 0.2f), 0.35f), 0.08f, true);
		StrokeV(P, FramePts(Frame::Hexagon, 1.1f), Lift(L.Field, 0.35f), 0.025f, true);
	}
	if (L.Shape == Frame::Ornament)
	{
		Box(P, -0.24f, -1.02f, 0.48f, 0.22f, 0.05f, Lin(P, -0.24f, 0.0f, 0.24f, 0.0f, M.Hi, M.Lo));
		Arc(P, 0.0f, -1.12f, 0.12f, PiF, TwoPiF, M.Mid, 0.05f);
		PolyV(P, Leaf(-0.36f, -0.9f, 0.2f, 0.08f, 0.35f), Hex(0x16a34a));
		PolyV(P, Leaf(0.36f, -0.9f, 0.2f, 0.08f, -0.35f), Hex(0x16a34a));
		Disc(P, -0.18f, -0.84f, 0.06f, Hex(0xef4444));
		Disc(P, 0.18f, -0.84f, 0.06f, Hex(0xef4444));
	}

	// The badge: a bevelled rim of metal around a field in the series' colours.
	PolyV(P, FramePts(L.Shape, 1.0f), Lin(P, -1.0f, -1.0f, 1.0f, 1.0f, M.Mid, M.Lo));
	PolyV(P, Scaled(FramePts(L.Shape, 0.955f), 1.0f, -0.012f, -0.018f), Lin(P, -0.9f, -0.9f, 0.6f, 0.8f, M.Hi, M.Mid));
	PolyV(P, FramePts(L.Shape, 0.875f), Sink(M.Lo, 0.35f));
	PolyV(P, FramePts(L.Shape, 0.85f), Lin(P, -0.6f, -0.85f, 0.6f, 0.85f, Field, Field2));
	if (Rich)
	{
		// Engine-turned rings, a light from above, and an engraved line inside the rim.
		const float Reach = InnerReach(L.Shape);
		for (float R = 0.16f; R <= Reach; R += 0.09f)
		{
			Hoop(P, 0.0f, 0.0f, R, R, Fade(Lift(L.Field, 0.4f), 0.07f), 0.012f);
		}
		Glow(P, 0.0f, -0.18f, 0.72f, Fade(Lift(L.Field, 0.35f), 0.6f));
		StrokeV(P, FramePts(L.Shape, 0.79f), Fade(M.Hi, 0.5f), 0.018f, true);
		if (L.Shape == Frame::Medallion)
		{
			for (int K = 0; K < 36; ++K)
			{
				const float A = TwoPiF * Fl(K) / 36.0f;
				Disc(P, 0.92f * std::cos(A) * 0.985f, 0.92f * std::sin(A) * 0.985f, 0.022f, Fade(M.Hi, 0.85f));
			}
		}
		if (L.Shape == Frame::Rhombus)
		{
			Gem(P, -0.9f, 0.0f, 0.12f, Hex(0x9ee7ff));
			Gem(P, 0.9f, 0.0f, 0.12f, Hex(0x9ee7ff));
		}
	}
	StrokeV(P, FramePts(L.Shape, 1.0f), Fade(Sink(M.Lo, 0.5f), 0.8f), 0.02f, true);

	// The motif, embossed in the metal, with its own shadow.
	const bool Banner = Rich && !L.Banner.empty();
	const float Mv = Banner ? -0.1f : 0.0f;
	const float Mk = Banner ? 0.46f : 0.56f;
	const Tone Emboss{M.Hi, M.Mid, Sink(M.Lo, 0.55f), Stone, Hex(0xffffff)};
	Draw(P.At(0.02f, Mv + 0.05f, Mk), L.Motif, Shadowed(0.45f));
	Draw(P.At(0.0f, Mv, Mk), L.Motif, Emboss);

	// In front: the bracelet, the banner, the crown, the ring, the diamond, the format's seal, the glints.
	if (L.Bracelet)
	{
		BraceletBand(P, M, Stone, true);
	}
	if (Banner)
	{
		Ribbon(P, 0.52f, 0.92f, L.Field2, M, L.Banner);
	}
	if (L.Main || L.Crown)
	{
		const Tone Regal{M.Hi, M.Mid, Sink(M.Lo, 0.5f), Hex(0xef4466), Hex(0xffffff)};
		Draw(P.At(0.02f, -1.02f, 0.4f), Glyph::Crown, Shadowed(0.4f));
		Draw(P.At(0.0f, -1.06f, 0.4f), Glyph::Crown, Regal);
	}
	if (L.Ring)
	{
		HangingRing(P, M, Hex(0xff4d6d));
	}
	if (L.HighRoller)
	{
		DiamondMount(P, MetalOf(Alloy::Platinum));
	}
	if (L.Seal != Glyph::Count && Size >= 64.0f)
	{
		const float Su = L.Ring ? -0.78f : 0.74f;
		Disc(P, Su + 0.02f, -0.6f, 0.3f, Rgba(0, 0, 0, 0.35f));
		Disc(P, Su, -0.64f, 0.29f, Lin(P, Su - 0.3f, -0.9f, Su + 0.3f, -0.35f, M.Hi, M.Lo));
		Disc(P, Su, -0.64f, 0.23f, Sink(L.Field2, 0.5f));
		const Tone White{Hex(0xf8fafc), Hex(0xc7d2e4), Hex(0x0b1220), Stone, Hex(0xffffff)};
		Draw(P.At(Su, -0.64f, 0.14f), L.Seal, White);
	}
	if (Rich)
	{
		Sparkles(P, Time, L.Main ? 6 : 3, 1.0f);
	}
}

struct Family
{
	const char* Prefix;
	Glyph G;
	uint32_t A;
	uint32_t B;
	uint32_t Gem;
};

// Each brand on the schedule, by its template id.
const Family Families[] = {
	{"night-owl", Glyph::Owl, 0x4c3bb3, 0x160f3d, 0xf2c14e},
	{"hyper-sprint", Glyph::Bolt, 0xff8a1f, 0xb4320b, 0xfff1b8},
	{"freeroll", Glyph::Gift, 0x0f9488, 0x0b2f4a, 0xb8ff2e},
	{"insomniac", Glyph::Eye, 0x7c3aed, 0x240a4f, 0x67e8f9},
	{"hh-215", Glyph::Crosshair, 0x991b1b, 0x18120f, 0xf2c14e},
	{"hh-", Glyph::Crosshair, 0xe53935, 0x6b1414, 0xfde68a},
	{"grind-", Glyph::Gear, 0x56677d, 0x18222f, 0xf59e0b},
	{"daily-main", Glyph::Crown, 0x2357e8, 0x1a1452, 0xf2c14e},
	{"daily-hr", Glyph::Diamond, 0x1e293b, 0x475569, 0x7dd3fc},
	{"deep-", Glyph::Wave, 0x0284c7, 0x0b3954, 0x7dd3fc},
	{"mystery-108", Glyph::Envelope, 0x8b2fd4, 0x2c0750, 0xf2c14e},
	{"mystery-", Glyph::Envelope, 0xd03fe0, 0x55157f, 0xfde047},
	{"flip-", Glyph::Coin, 0xe11d48, 0x6d0f2c, 0xfacc15},
	{"shot-", Glyph::Stopwatch, 0x0aa5c5, 0x123f55, 0xf43f5e},
	{"plo-", Glyph::Fan, 0x16a34a, 0x0f3d24, 0xef4444},
	{"bigstack-", Glyph::Chips, 0x2f6fef, 0x152a6b, 0xef4444},
	{"bounty-builder", Glyph::Bricks, 0xf0661a, 0x6b240c, 0xfde68a},
	{"graveyard", Glyph::Tombstone, 0x3d4b60, 0x0b111b, 0x4ade80},
	{"sunrise", Glyph::Sunrise, 0xfb923c, 0xbe185d, 0xfde047},
	{"monster-", Glyph::Claw, 0x5d9e10, 0x15240a, 0xd9f99d},
	{"step", Glyph::Stairs, 0xc2700f, 0x3d1704, 0xf2c14e},
	{"monday-madness", Glyph::Spiral, 0xe0337f, 0x45061f, 0xfbcfe8},
	{"turbo-tuesday", Glyph::Rocket, 0x2b63f0, 0x13204d, 0xff8a1f},
	{"thursday-heater", Glyph::Flame, 0xe02424, 0x3d0909, 0xfb923c},
	{"friday-fight", Glyph::Glove, 0x2a3445, 0x0b0f17, 0xe53935},
	{"saturday-six", Glyph::SixSeats, 0x0d9488, 0x0f3b38, 0x22c55e},
	{"millions", Glyph::Laurel, 0xb7791f, 0x1c1205, 0xfde68a},
	{"sunday-showdown", Glyph::Showdown, 0x2349c2, 0x0b1029, 0xef4444},
	{"sunday-special", Glyph::Star, 0x7c3aed, 0x1b1340, 0xfde047},
	{"sunday-storm", Glyph::Storm, 0x3a4a63, 0x0a0f1c, 0xfacc15},
	{"sunday-shr", Glyph::Skull, 0x1f2937, 0x3d0b0b, 0xf2c14e},
	{"mega-rcop", Glyph::Ticket, 0x8b5cf6, 0x2e0b52, 0xf2c14e},
	{"moonlight-marathon", Glyph::Moon, 0x2346a8, 0x050a1f, 0xe0e7ff},
	{"bankroll-builder", Glyph::Sprout, 0x1e9b4f, 0x0d3b1f, 0xfacc15},
	{"tuesday-tycoon", Glyph::TopHat, 0x1f2a3d, 0x0a0f18, 0xf2c14e},
	{"high-noon", Glyph::Clock, 0xe08a0b, 0x6b2f06, 0xfef3c7},
	{"hump-day-heater", Glyph::Thermometer, 0xf0661a, 0x6b1d0c, 0xef4444},
	{"lucky-sevens", Glyph::Sevens, 0x16803d, 0x052e16, 0xfacc15},
	{"ante-up", Glyph::AnteUp, 0x0e8aa8, 0x07303d, 0xf2c14e},
	{"friday-fireworks", Glyph::Firework, 0x5b21b6, 0x05031a, 0xf472b6},
	{"twilight-omaha", Glyph::Fan, 0xa23be8, 0x1a1040, 0xfb923c},
	{"saturday-stampede", Glyph::Horseshoe, 0xa04a12, 0x1c1108, 0xfcd34d},
	{"brunch-stack", Glyph::Cup, 0xf5a20b, 0x8a3d07, 0x7c3a12},
	{"slingshot", Glyph::Slingshot, 0x0a8ad6, 0x0b3550, 0xf87171},
};

const Family* FamilyOf(const std::string& Id)
{
	for (const Family& F : Families)
	{
		if (Id.rfind(F.Prefix, 0) == 0)
		{
			return &F;
		}
	}
	return nullptr;
}

/** An icon tile for one of the schedule's tournaments. */
void Tile(Canvas& C, Glyph G, uint32_t A, uint32_t B, uint32_t GemColor, bool Featured, int Badge, float Cx, float Cy, float Size, double Time)
{
	const float H = Size * 0.5f;
	const Rect R{Cx - H, Cy - H, Size, Size};
	const float Rad = Size * 0.24f;
	const Color Ca = Hex(A);
	const Color Cb = Hex(B);
	C.GlowRoundRect({R.X, R.Y + Size * 0.05f, R.W, R.H}, Rad, Rgba(0, 0, 0, 0.45f), Size * 0.14f);
	C.FillRoundRect(R, Rad, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, Lift(Ca, 0.06f), Cb));
	const Pen P{&C, Cx, Cy + Size * 0.02f, Size * 0.29f};
	Glow(P, 0.0f, -0.1f, 1.25f, Fade(Lift(Ca, 0.35f), 0.55f));
	// A sheen over the top half, and the glass edge.
	C.FillRoundRect({R.X + Size * 0.03f, R.Y + Size * 0.03f, R.W - Size * 0.06f, R.H * 0.48f}, Rad * 0.85f,
		Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H * 0.5f}, Rgba(255, 255, 255, 0.2f), Rgba(255, 255, 255, 0.0f)));
	const Tone White{Hex(0xf8fafc), Mix(Hex(0xf8fafc), Cb, 0.3f), Sink(Cb, 0.55f), Hex(GemColor), Hex(0xffffff)};
	Draw(P.At(0.0f, 0.07f, 1.0f), G, Shadowed(0.4f));
	Draw(P, G, White);
	C.StrokeRoundRect(R, Rad, Featured ? Hex(0xf2c14e) : Rgba(255, 255, 255, 0.16f), Featured ? std::max(1.5f, Size * 0.035f) : 1.0f);
	if (Featured && Size >= 40.0f)
	{
		const float A0 = 0.6f + 0.4f * Fl(std::sin(Time * 2.2 + Fl(Cx) * 0.01f));
		Glint(Pen{&C, R.X + R.W - Size * 0.12f, R.Y + Size * 0.12f, Size * 0.3f}, 0.0f, 0.0f, 0.32f, Fade(Hex(0xfff4c6), A0));
	}
	if (Badge > 0)
	{
		const float Br = Size * 0.17f;
		const float Bx = R.X + R.W - Br * 0.7f;
		const float By = R.Y + Br * 0.7f;
		C.FillCircle(Bx, By, Br, Paint::Linear({Bx, By - Br}, {Bx, By + Br}, Hex(0xfff4c6), Hex(0xc9962b)));
		C.StrokeEllipse(Bx, By, Br, Br, Hex(0x3d2a07), 1.0f);
		if (Br >= 6.0f)
		{
			TextStyle St;
			St.Size = Br * 1.3f;
			St.Weight = 900;
			St.Col = Hex(0x2a1a04);
			St.HAlign = Align::Center;
			St.VAlign = Baseline::Middle;
			C.Text(std::to_string(Badge), Bx, By + 0.5f, St);
		}
	}
}

Glyph FormatGlyph(const net::EventTemplate& T)
{
	if (T.Omaha)
	{
		return Glyph::Fan;
	}
	switch (T.Fmt)
	{
	case net::Format::Bounty: return Glyph::Crosshair;
	case net::Format::Mystery: return Glyph::Envelope;
	case net::Format::Satellite: return Glyph::Ticket;
	case net::Format::Flip: return Glyph::Coin;
	case net::Format::Freezeout: return Glyph::Lock;
	case net::Format::ReEntry: break;
	}
	if (T.TableSize < 8)
	{
		return Glyph::SixSeats;
	}
	return T.Speed == "Hyper" ? Glyph::Rocket : T.Speed == "Turbo" ? Glyph::Bolt : T.Speed == "Deep" ? Glyph::Wave : Glyph::Spade;
}

bool Has(const std::string& S, const char* Word)
{
	return S.find(Word) != std::string::npos;
}

// ------------------------------------------------------------------ bracelets and rings

/** A bracelet's or a ring's look: where it was won decides the enamel and the stones. */
struct TrophyLook
{
	Metal Band;
	Color Plate;  // a bracelet's plaque (enamel)
	Color Plate2;
	Color Mark;   // the plaque's spade
	Color Stone;  // a ring's center stone; a bracelet's corner stones
	bool Main = false;
};

/** Each Grand Circuit stop has its own stone (the year doesn't change it). */
uint32_t CityStone(const std::string& Series)
{
	static const uint32_t Stones[] = {0x2f6fef, 0x10b981, 0xe11d48, 0x9b5de5, 0xf59e0b, 0x22d3ee, 0xec4899};
	std::string City = Series.rfind("Grand Circuit ", 0) == 0 ? Series.substr(14) : Series;
	const size_t Sp = City.rfind(' ');
	if (Sp != std::string::npos)
	{
		City = City.substr(0, Sp);
	}
	uint32_t H = 2166136261u;
	for (char Ch : City)
	{
		H ^= static_cast<unsigned char>(Ch);
		H *= 16777619u;
	}
	return Stones[H % (sizeof(Stones) / sizeof(Stones[0]))];
}

TrophyLook LookOf(const world::Award& A)
{
	TrophyLook L;
	L.Band = MetalOf(Alloy::Gold);
	L.Main = A.Main;
	const bool Tco = A.Online && SlotOf(A.Series) == "tco";
	if (!A.Ring && Tco)
	{
		// The Championship Online: royal-blue enamel, a white-gold spade, sapphires.
		L.Plate = Hex(0x2f5fd6);
		L.Plate2 = Hex(0x0a1747);
		L.Mark = Hex(0xf4f7ff);
		L.Stone = Hex(0x8fbaff);
	}
	else if (!A.Ring)
	{
		// Las Vegas: yellow gold on black onyx, a diamond spade.
		L.Plate = Hex(0x2a2a35);
		L.Plate2 = Hex(0x040406);
		L.Mark = Hex(0xeaf7ff);
		L.Stone = Hex(0xdff6ff);
	}
	else
	{
		// Ring Rush's garnet; each Grand Circuit stop's own stone.
		L.Plate = Hex(0x2a2a35);
		L.Plate2 = Hex(0x040406);
		L.Mark = Hex(0xeaf7ff);
		L.Stone = A.Online && SlotOf(A.Series) == "ring" ? Hex(0xd0123a) : A.Series.empty() ? Hex(0x2f6fef) : Hex(CityStone(A.Series));
	}
	return L;
}

/** A small diamond (pavé, halos): a white disc with a spark. */
void Pave(const Pen& P, float U, float V, float R, const Color& Tint)
{
	Disc(P, U, V, R * 1.25f, Fade(Hex(0x3a2a08), 0.8f));
	Disc(P, U, V, R, Lin(P, U - R, V - R, U + R, V + R, Hex(0xffffff), Tint));
	Disc(P, U - R * 0.3f, V - R * 0.3f, R * 0.35f, Fade(Hex(0xffffff), 0.95f));
}

/** A bracelet seen from above and in front: the band's far side, its links, the plaque with its spade. */
void BraceletArt(const Pen& P, const TrophyLook& L, double Time)
{
	const Metal& M = L.Band;
	const float Cy = -0.12f;
	const float Rx = 0.88f;
	const float Ry = 0.4f;
	const float Hb = 0.17f; // the band's height at the back
	const float Hf = 0.27f; // and at the front
	auto At = [&](float A) { return Vec2{Rx * std::cos(A), Cy + Ry * std::sin(A)}; };
	auto Height = [&](float A) { return Hb + (Hf - Hb) * (0.5f + 0.5f * std::sin(A)); };
	Oval(P, 0.0f, 0.72f, 0.92f, 0.16f, Paint::Radial(P.P(0.0f, 0.72f), 0.0f, P.P(0.0f, 0.72f), 0.92f * P.S, Rgba(0, 0, 0, 0.45f), 0.6f, Rgba(0, 0, 0, 0.2f), Rgba(0, 0, 0, 0.0f)));
	// The far side: the inside of the band, in shadow.
	{
		std::vector<Vec2> Back;
		const int N = 28;
		for (int I = 0; I <= N; ++I)
		{
			const float A = PiF + PiF * Fl(I) / Fl(N);
			const Vec2 Q = At(A);
			Back.push_back({Q.X, Q.Y - Height(A) * 0.5f});
		}
		for (int I = N; I >= 0; --I)
		{
			const float A = PiF + PiF * Fl(I) / Fl(N);
			const Vec2 Q = At(A);
			Back.push_back({Q.X, Q.Y + Height(A) * 0.5f});
		}
		PolyV(P, Back, Lin(P, 0.0f, Cy - Ry - Hb, 0.0f, Cy - Ry + Hb, Sink(M.Mid, 0.2f), Sink(M.Lo, 0.45f)));
		for (int K = 1; K < 12; ++K)
		{
			const float A = PiF + PiF * Fl(K) / 12.0f;
			const Vec2 Q = At(A);
			const float H = Height(A) * 0.5f;
			Stroke(P, {{Q.X, Q.Y - H}, {Q.X, Q.Y + H}}, Fade(Sink(M.Lo, 0.5f), 0.7f), 0.018f);
		}
		StrokeV(P, ArcPts(0.0f, Cy - Hb * 0.5f, Rx, Ry, PiF + 0.2f, TwoPiF - 0.2f, 24), Fade(M.Hi, 0.35f), 0.015f);
	}
	// The front: polished links, catching the light in turn.
	{
		const int N = 14;
		for (int K = 0; K < N; ++K)
		{
			const float Step = PiF / Fl(N);
			const float A0 = -0.06f + Step * Fl(K);
			const float A1 = A0 + Step * 0.92f;
			const Vec2 Q0 = At(A0);
			const Vec2 Q1 = At(A1);
			const float H0 = Height(A0) * 0.5f;
			const float H1 = Height(A1) * 0.5f;
			const Color Top = K % 2 == 0 ? M.Hi : Lift(M.Mid, 0.25f);
			const Color Bot = K % 2 == 0 ? M.Mid : Sink(M.Mid, 0.25f);
			Poly(P, {{Q0.X, Q0.Y - H0}, {Q1.X, Q1.Y - H1}, {Q1.X, Q1.Y + H1}, {Q0.X, Q0.Y + H0}}, Lin(P, 0.0f, Q0.Y - H0, 0.0f, Q0.Y + H0, Top, Bot));
			Stroke(P, {{Q1.X, Q1.Y - H1}, {Q1.X, Q1.Y + H1}}, Fade(M.Lo, 0.9f), 0.02f);
			// A bevel across each link.
			Stroke(P, {{Q0.X, Q0.Y - H0 * 0.35f}, {Q1.X, Q1.Y - H1 * 0.35f}}, Fade(Hex(0xffffff), K % 2 == 0 ? 0.55f : 0.25f), 0.014f);
		}
		StrokeV(P, ArcPts(0.0f, Cy + Hf * 0.5f, Rx, Ry, -0.04f, PiF + 0.04f, 30), Fade(M.Lo, 0.9f), 0.02f);
		StrokeV(P, ArcPts(0.0f, Cy - Hf * 0.5f, Rx, Ry, 0.05f, PiF - 0.05f, 30), Fade(M.Hi, 0.8f), 0.016f);
	}
	// The plaque at the front, facing you.
	const float Pw = L.Main ? 0.9f : 0.82f;
	const float Ph = L.Main ? 0.6f : 0.54f;
	const float Pv = Cy + Ry + 0.06f;
	if (L.Main)
	{
		// A Main Event's: a crown over the plaque, and light behind it.
		Glow(P, 0.0f, Pv, 0.9f, Fade(M.Mid, 0.35f));
	}
	Box(P, -Pw * 0.5f - 0.02f, Pv - Ph * 0.5f + 0.04f, Pw + 0.04f, Ph, 0.13f, Rgba(0, 0, 0, 0.4f));
	Box(P, -Pw * 0.5f, Pv - Ph * 0.5f, Pw, Ph, 0.12f, Lin(P, -Pw * 0.5f, Pv - Ph * 0.5f, Pw * 0.5f, Pv + Ph * 0.5f, M.Hi, M.Lo));
	Box(P, -Pw * 0.5f + 0.02f, Pv - Ph * 0.5f + 0.02f, Pw - 0.05f, Ph - 0.05f, 0.11f, Lin(P, -Pw * 0.5f, Pv - Ph * 0.5f, Pw * 0.4f, Pv + Ph * 0.4f, Lift(M.Hi, 0.3f), M.Mid));
	const float Iw = Pw - 0.14f;
	const float Ih = Ph - 0.14f;
	Box(P, -Iw * 0.5f, Pv - Ih * 0.5f, Iw, Ih, 0.07f, Lin(P, 0.0f, Pv - Ih * 0.5f, 0.0f, Pv + Ih * 0.5f, L.Plate, L.Plate2));
	Box(P, -Iw * 0.5f + 0.03f, Pv - Ih * 0.5f + 0.02f, Iw - 0.06f, Ih * 0.42f, 0.05f, Rgba(255, 255, 255, 0.07f));
	// The spade, in white gold or diamonds.
	Spade(P, 0.012f, Pv - 0.01f + 0.012f, Ih * 0.36f, Rgba(0, 0, 0, 0.45f));
	Spade(P, 0.0f, Pv - 0.01f, Ih * 0.36f, Lin(P, -0.1f, Pv - 0.15f, 0.1f, Pv + 0.15f, Hex(0xffffff), Sink(L.Mark, 0.25f)));
	Glint(P, -0.05f, Pv - 0.1f, 0.05f, Fade(Hex(0xffffff), 0.9f));
	if (L.Main)
	{
		// Pavé all the way round the enamel.
		const int N = 18;
		for (int K = 0; K < N; ++K)
		{
			const float T = TwoPiF * Fl(K) / Fl(N);
			const float U = std::cos(T) * (Iw * 0.5f + 0.025f);
			const float V = Pv + std::sin(T) * (Ih * 0.5f + 0.022f);
			Pave(P, std::max(-Iw * 0.5f, std::min(Iw * 0.5f, U * 1.12f)), std::max(Pv - Ih * 0.5f - 0.01f, std::min(Pv + Ih * 0.5f + 0.01f, V)), 0.026f, L.Stone);
		}
		const Tone Regal{M.Hi, M.Mid, Sink(M.Lo, 0.5f), Hex(0xef4466), Hex(0xffffff)};
		Draw(P.At(0.01f, Pv - Ph * 0.5f - 0.1f, 0.2f), Glyph::Crown, Shadowed(0.4f));
		Draw(P.At(0.0f, Pv - Ph * 0.5f - 0.12f, 0.2f), Glyph::Crown, Regal);
	}
	else
	{
		for (int K = 0; K < 4; ++K)
		{
			Pave(P, (K % 2 == 0 ? -1.0f : 1.0f) * (Iw * 0.5f - 0.06f), Pv + (K < 2 ? -1.0f : 1.0f) * (Ih * 0.5f - 0.06f), 0.03f, L.Stone);
		}
	}
	Sparkles(P, Time, L.Main ? 5 : 3, 0.92f);
}

/** A ring: the band, its shoulders, the head with a halo, and the stone. */
void RingArt(const Pen& P, const TrophyLook& L, double Time)
{
	const Metal& M = L.Band;
	const float Bv = 0.3f;  // the band's center
	const float Br = 0.52f; // its radius
	const float K = L.Main ? 0.5f : 0.42f;
	const float Sv = -0.5f; // the stone's girdle
	Oval(P, 0.0f, Bv + Br + 0.1f, 0.62f, 0.12f, Paint::Radial(P.P(0.0f, Bv + Br + 0.1f), 0.0f, P.P(0.0f, Bv + Br + 0.1f), 0.62f * P.S, Rgba(0, 0, 0, 0.45f), 0.6f,
		Rgba(0, 0, 0, 0.2f), Rgba(0, 0, 0, 0.0f)));
	if (L.Main)
	{
		Glow(P, 0.0f, Sv, 0.8f, Fade(L.Stone, 0.45f));
	}
	// The band: dark inside, polished outside.
	Hoop(P, 0.0f, Bv, Br, Br, M.Lo, 0.2f);
	Hoop(P, 0.0f, Bv, Br, Br, M.Mid, 0.14f);
	Arc(P, 0.0f, Bv, Br + 0.035f, PiF * 0.62f, PiF * 1.3f, Fade(M.Hi, 0.95f), 0.04f);
	Arc(P, 0.0f, Bv, Br + 0.035f, PiF * 0.05f, PiF * 0.4f, Fade(M.Hi, 0.55f), 0.03f);
	Arc(P, 0.0f, Bv, Br - 0.075f, 0.0f, TwoPiF, Fade(Sink(M.Lo, 0.4f), 0.85f), 0.025f, false);
	// The shoulders rise into the head.
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float Sd = Fl(Side);
		Poly(P, {{Sd * 0.36f, Bv - Br + 0.16f}, {Sd * 0.13f, Sv + 0.28f}, {Sd * 0.06f, Sv + 0.36f}, {Sd * 0.22f, Bv - Br + 0.22f}},
			Lin(P, Sd * 0.36f, Bv - Br, 0.0f, Sv + 0.3f, M.Mid, M.Hi));
		if (L.Main)
		{
			Pave(P, Sd * 0.26f, Bv - Br + 0.12f, 0.035f, Hex(0xdff6ff));
			Pave(P, Sd * 0.18f, Bv - Br + 0.02f, 0.03f, Hex(0xdff6ff));
		}
	}
	// The head: a basket under the stone.
	Poly(P, {{-0.15f, Sv + 0.36f}, {0.15f, Sv + 0.36f}, {0.27f, Sv + 0.06f}, {-0.27f, Sv + 0.06f}}, Lin(P, -0.27f, 0.0f, 0.27f, 0.0f, M.Hi, M.Lo));
	Stroke(P, {{-0.21f, Sv + 0.2f}, {0.21f, Sv + 0.2f}}, Fade(M.Lo, 0.8f), 0.02f);
	// The halo: small diamonds set round the stone's girdle, behind it (a Main Event's ring has two rows).
	for (int H = L.Main ? 1 : 0; H >= 0; --H)
	{
		const float Hr = K * (0.98f + 0.2f * Fl(H));
		const int N = 14 + H * 4;
		for (int I = 0; I < N; ++I)
		{
			const float T = TwoPiF * (Fl(I) + 0.5f * Fl(H)) / Fl(N);
			Pave(P, Hr * std::cos(T), Sv + 0.04f * K + Hr * 0.24f * std::sin(T), 0.036f + 0.004f * Fl(H), Hex(0xdff6ff));
		}
	}
	Gem(P, 0.0f, Sv, K, L.Stone);
	// Four prongs.
	for (int Side = -1; Side <= 1; Side += 2)
	{
		Disc(P, Fl(Side) * 0.62f * K, Sv - 0.3f * K, 0.03f, M.Hi);
		Disc(P, Fl(Side) * 0.25f * K, Sv - 0.55f * K, 0.025f, M.Hi);
	}
	Sparkles(P, Time, L.Main ? 5 : 3, 0.9f);
}
} // namespace eventart_detail

using namespace eventart_detail;

Glyph SeriesGlyph(const net::SeriesInfo& S)
{
	const std::string Slot = SlotOf(S.Id);
	if (Slot == "rcop")
	{
		return Glyph::Trophy;
	}
	if (Slot == "tco")
	{
		return Glyph::Bracelet;
	}
	if (Slot == "ring")
	{
		return Glyph::Ring;
	}
	if (Slot == "slam")
	{
		return Glyph::Sun;
	}
	if (Slot == "mm")
	{
		return Glyph::Bolt;
	}
	// The seasonal series: a motif from the name.
	struct Pick
	{
		const char* Word;
		Glyph G;
	};
	static const Pick Picks[] = {{"Frostbite", Glyph::Snowflake}, {"Black Ice", Glyph::Crystal}, {"Polar Night", Glyph::Moon}, {"Whiteout", Glyph::Snowflake},
		{"Ice Box", Glyph::Crystal}, {"Frost Line", Glyph::Thermometer}, {"Northern Lights", Glyph::Aurora}, {"Snowblind", Glyph::Eye}, {"Glacier", Glyph::Mountain},
		{"Long Night", Glyph::Moon}, {"Deep Freeze", Glyph::Thermometer}, {"Avalanche", Glyph::Mountain}, {"Permafrost", Glyph::Crystal}, {"Hailstorm", Glyph::Storm},
		{"Spring Fever", Glyph::Thermometer}, {"Bloom", Glyph::Flower}, {"April", Glyph::Umbrella}, {"Equinox", Glyph::Equinox}, {"Thunderhead", Glyph::Storm},
		{"Petal", Glyph::Flower}, {"Thaw", Glyph::Droplet}, {"Rainmaker", Glyph::RainCloud}, {"Greenlight", Glyph::TrafficLight}, {"May Day", Glyph::Sun},
		{"Wildflower", Glyph::Flower}, {"Cherry", Glyph::Cherries}, {"Monsoon", Glyph::Wave}, {"Sun Shower", Glyph::RainCloud}, {"Top Floor", Glyph::Skyline},
		{"Thin Air", Glyph::Mountain}, {"Skyline", Glyph::Skyline}, {"Vault", Glyph::Vault}, {"Black Card", Glyph::Card}, {"Gilded", Glyph::Crown},
		{"Diamond", Glyph::Diamond}, {"Crown Jewel", Glyph::Crown}, {"Altitude", Glyph::Mountain}, {"Velvet", Glyph::Rope}, {"Marble", Glyph::Columns},
		{"Ivory", Glyph::Tower}, {"Platinum", Glyph::Diamond}, {"Gold Coast", Glyph::Sun}, {"Heist", Glyph::Gift}, {"Snowed-In", Glyph::Snowflake},
		{"Midwinter", Glyph::Ornament}, {"Stocking", Glyph::Stocking}, {"Last Call", Glyph::Glass}, {"Twelve Nights", Glyph::Clock}, {"Silver Bells", Glyph::Bell},
		{"Fireside", Glyph::Flame}, {"Mistletoe", Glyph::Mistletoe}, {"Countdown", Glyph::Clock}, {"Candlelight", Glyph::Candle}, {"Tinsel", Glyph::Firework},
		{"Cocoa", Glyph::Cup}, {"Year-End", Glyph::Hourglass}, {"Northern Star", Glyph::Star}};
	for (const Pick& Pk : Picks)
	{
		if (Has(S.Name, Pk.Word))
		{
			return Pk.G;
		}
	}
	return Slot == "win" ? Glyph::Snowflake : Slot == "spr" ? Glyph::Flower : Slot == "hrs" ? Glyph::Diamond : Slot == "hol" ? Glyph::Gift : Glyph::Star;
}

void DrawGlyph(Canvas& C, Glyph G, float Cx, float Cy, float Size, uint32_t Jewel)
{
	const Tone White{Hex(0xf8fafc), Hex(0xc7d2e4), Hex(0x0b1220), Hex(Jewel), Hex(0xffffff)};
	const Pen P{&C, Cx, Cy, Size * 0.5f};
	Draw(P.At(0.0f, 0.06f, 1.0f), G, Shadowed(0.4f));
	Draw(P, G, White);
}

void Emblem(Canvas& C, const net::EventTemplate& T, float Cx, float Cy, float Size, double Time)
{
	if (!T.Series.empty())
	{
		const net::SeriesInfo* Sr = net::Shared().FindSeries(T.Series);
		CrestLook L;
		L.Shape = FrameOf(SlotOf(T.Series));
		L.Rim = T.Main ? Alloy::Gold : AlloyFor(T.BuyInCents);
		if (Sr)
		{
			Enamel(L, SlotOf(T.Series), Sr->Color, Sr->Color2);
			L.Motif = SeriesGlyph(*Sr);
		}
		const bool SeriesMain = Sr && Sr->MainEvent == T.Id;
		const bool HighRoller = Has(T.Name, "High Roller");
		L.Main = SeriesMain;
		L.Crown = T.Main && !SeriesMain;
		L.Bracelet = T.Bracelet;
		L.Ring = T.Ring && !SeriesMain;
		L.HighRoller = HighRoller;
		L.Seal = T.Main || HighRoller ? Glyph::Count : FormatGlyph(T);
		L.Banner = SeriesMain ? "MAIN EVENT"
			: T.Main ? (Has(T.Name, "Micro") ? "MICRO MAIN" : Has(T.Name, "Mini") ? "MINI MAIN" : "MAIN EVENT")
			: HighRoller ? (Has(T.Name, "Super") ? "SUPER HIGH ROLLER" : "HIGH ROLLER")
			: T.Bracelet ? "BRACELET"
			: T.Ring ? "RING EVENT"
					 : "EVENT #" + std::to_string(T.EventNo);
		Crest(C, L, Cx, Cy, Size, Time);
		return;
	}
	if (const Family* F = FamilyOf(T.Id))
	{
		const int Step = T.Id.rfind("step", 0) == 0 && T.Id.size() > 4 ? T.Id[4] - '0' : 0;
		Tile(C, F->G, F->A, F->B, F->Gem, T.Featured, Step, Cx, Cy, Size, Time);
		return;
	}
	// Anything else: its format, in the colours of its stakes.
	static const uint32_t ByTier[5][2] = {{0x0f9488, 0x0b2f4a}, {0x2f6fef, 0x152a6b}, {0x7c3aed, 0x1b1340}, {0xd97706, 0x3d1704}, {0x1f2937, 0x0b0f17}};
	const int Tr = static_cast<int>(net::TierOf(T.BuyInCents));
	Tile(C, FormatGlyph(T), ByTier[Tr][0], ByTier[Tr][1], 0xf2c14e, T.Featured, 0, Cx, Cy, Size, Time);
}

void SeriesCrest(Canvas& C, const net::SeriesInfo& S, float Cx, float Cy, float Size, double Time)
{
	const std::string Slot = SlotOf(S.Id);
	CrestLook L;
	L.Shape = FrameOf(Slot);
	L.Rim = Slot == "hrs" ? Alloy::Platinum : Slot == "rcop" || Slot == "tco" || Slot == "slam" || Slot == "ring" ? Alloy::Gold : Alloy::Silver;
	Enamel(L, Slot, S.Color, S.Color2);
	L.Motif = SeriesGlyph(S);
	L.Laurel = Slot == "rcop";
	L.Crown = Slot == "rcop";
	L.Bracelet = Slot == "tco";
	L.Ring = Slot == "ring";
	L.HighRoller = Slot == "hrs";
	// The banner: the year of a flagship ("2027"), the short name of a seasonal series.
	const size_t Sp = S.Name.rfind(' ');
	const std::string Last = Sp == std::string::npos ? std::string() : S.Name.substr(Sp + 1);
	const bool Year = Last.size() == 4 && std::isdigit(static_cast<unsigned char>(Last[0])) != 0;
	L.Banner = Year ? Last : S.Short;
	Crest(C, L, Cx, Cy, Size, Time);
}
void Trophy(Canvas& C, const world::Award& A, float Cx, float Cy, float Size, double Time)
{
	const Pen P{&C, Cx, Cy, Size * 0.5f};
	const TrophyLook L = LookOf(A);
	if (A.Ring)
	{
		RingArt(P.At(0.0f, -0.08f, 1.0f), L, Time);
	}
	else
	{
		BraceletArt(P.At(0.0f, -0.04f, 1.0f), L, Time);
	}
}

uint32_t TrophyStone(const world::Award& A)
{
	const TrophyLook L = LookOf(A);
	const Color S = A.Ring ? L.Stone : L.Plate;
	const auto Byte = [](float V) { return static_cast<uint32_t>(std::min(255.0f, std::max(0.0f, V * 255.0f + 0.5f))); };
	return Byte(S.R) << 16 | Byte(S.G) << 8 | Byte(S.B);
}

std::string TrophySeries(const world::Award& A)
{
	if (A.Series.empty())
	{
		return A.Ring ? "Grand Circuit" : "The Championship";
	}
	if (A.Online)
	{
		const net::SeriesInfo* Sr = net::Shared().FindSeries(A.Series);
		return Sr ? Sr->Name : A.Series;
	}
	return A.Series;
}

std::string TrophyEvent(const world::Award& A)
{
	// "TCO '27 #12: $215 Final Viper" or "The Championship 2027: $1,500 Bounty": the event itself.
	const size_t Colon = A.Event.find(": ");
	return Colon == std::string::npos ? A.Event : A.Event.substr(Colon + 2);
}

void Champion(AvatarSpec& Pic, const std::vector<world::Award>& Awards)
{
	int Bracelets = 0;
	int Rings = 0;
	const world::Award* LastRing = nullptr;
	const world::Award* LastBracelet = nullptr;
	for (const world::Award& A : Awards)
	{
		if (A.Ring)
		{
			++Rings;
			LastRing = &A;
		}
		else
		{
			++Bracelets;
			LastBracelet = &A;
		}
	}
	if (Bracelets + Rings == 0)
	{
		return;
	}
	// The player and the rival keep their glow under the champion's frame.
	Pic.Halo = Pic.Frame == AvatarFrame::Neon;
	Pic.Frame = Bracelets > 0 ? AvatarFrame::Bracelet : AvatarFrame::Gem;
	Pic.Bracelets = Bracelets;
	Pic.Rings = Rings;
	Pic.Stone = LastRing ? TrophyStone(*LastRing) : 0xdff6ff;
	Pic.Plate = LastBracelet ? TrophyStone(*LastBracelet) : 0x2a2a35;
}
} // namespace eventart
} // namespace ui
} // namespace ss
