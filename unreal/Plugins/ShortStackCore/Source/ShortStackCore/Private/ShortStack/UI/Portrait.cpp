#include "ShortStack/UI/Portrait.h"
#include "../StrictFloat.h"
#include "ThumbPortrait.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace ss
{
namespace ui
{
namespace portrait_detail
{
using Pts = std::vector<Vec2>;
constexpr float Deg = 3.14159265358979f / 180.0f;

// Update 3 (SPEC §6.1, spike S-T): the small thumbnail's coarse drawing, switched on only while DrawPortraitX draws at the
// LOD (a guard puts it back). 0 for every other caller, so the default overload draws exactly as it always has: Spline
// keeps its density and Soft its radial falloff.
thread_local int PortraitCoarse = 0;

float Fl(int V)
{
	return static_cast<float>(V);
}

Vec2 Lerp2(Vec2 A, Vec2 B, float T)
{
	return {A.X + (B.X - A.X) * T, A.Y + (B.Y - A.Y) * T};
}

/** Catmull-Rom through the points (closed loop, or open with the ends kept). */
Pts Spline(const Pts& P, int Per, bool Closed)
{
	if (PortraitCoarse > 0)
	{
		// The small tile: a third of the points (an outline 60 pixels tall needs no more).
		Per = std::max(2, (Per + 1) / 3);
	}
	const int N = static_cast<int>(P.size());
	if (N < 3)
	{
		return P;
	}
	auto At = [&](int I) {
		const int K = Closed ? ((I % N) + N) % N : std::clamp(I, 0, N - 1);
		return P[static_cast<size_t>(K)];
	};
	Pts Out;
	const int Spans = Closed ? N : N - 1;
	for (int I = 0; I < Spans; ++I)
	{
		const Vec2 P0 = At(I - 1);
		const Vec2 P1 = At(I);
		const Vec2 P2 = At(I + 1);
		const Vec2 P3 = At(I + 2);
		for (int K = 0; K < Per; ++K)
		{
			const float T = Fl(K) / Fl(Per);
			const float T2 = T * T;
			const float T3 = T2 * T;
			auto Cr = [&](float A, float B, float C, float D) { return 0.5f * (2.0f * B + (C - A) * T + (2.0f * A - 5.0f * B + 4.0f * C - D) * T2 + (3.0f * B - A - 3.0f * C + D) * T3); };
			Out.push_back({Cr(P0.X, P1.X, P2.X, P3.X), Cr(P0.Y, P1.Y, P2.Y, P3.Y)});
		}
	}
	if (!Closed)
	{
		Out.push_back(P.back());
	}
	return Out;
}

/** A symmetric loop from its right half (top to bottom, x >= 0; points on the axis are shared). */
Pts Mirror(const Pts& Right)
{
	Pts Out = Right;
	for (size_t I = Right.size(); I-- > 0;)
	{
		if (std::fabs(Right[I].X) > 0.01f)
		{
			Out.push_back({-Right[I].X, Right[I].Y});
		}
	}
	return Out;
}

Pts Flip(const Pts& P)
{
	Pts Out;
	for (const Vec2& Q : P)
	{
		Out.push_back({-Q.X, Q.Y});
	}
	return Out;
}

Pts Moved(const Pts& P, float Dx, float Dy)
{
	Pts Out;
	for (const Vec2& Q : P)
	{
		Out.push_back({Q.X + Dx, Q.Y + Dy});
	}
	return Out;
}

Pts Ellipse(float Cx, float Cy, float Rx, float Ry, int Segs)
{
	if (PortraitCoarse > 0)
	{
		Segs = std::max(8, Segs / 2);
	}
	Pts Out;
	for (int I = 0; I < Segs; ++I)
	{
		const float A = 2.0f * 3.14159265358979f * Fl(I) / Fl(Segs);
		Out.push_back({Cx + Rx * std::cos(A), Cy + Ry * std::sin(A)});
	}
	return Out;
}

float Cross(Vec2 O, Vec2 A, Vec2 B)
{
	return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
}

float Area(const Pts& P)
{
	float S = 0.0f;
	for (size_t I = 0; I < P.size(); ++I)
	{
		const Vec2 A = P[I];
		const Vec2 B = P[(I + 1) % P.size()];
		S += A.X * B.Y - B.X * A.Y;
	}
	return S * 0.5f;
}

/** Sutherland-Hodgman: Subject clipped to a convex polygon. */
Pts ClipConvex(const Pts& Subject, const Pts& Clip)
{
	if (Subject.size() < 3 || Clip.size() < 3)
	{
		return {};
	}
	const float Sign = Area(Clip) >= 0.0f ? 1.0f : -1.0f;
	Pts Out = Subject;
	for (size_t E = 0; E < Clip.size() && !Out.empty(); ++E)
	{
		const Vec2 A = Clip[E];
		const Vec2 B = Clip[(E + 1) % Clip.size()];
		const Pts In = Out;
		Out.clear();
		for (size_t I = 0; I < In.size(); ++I)
		{
			const Vec2 P = In[I];
			const Vec2 Q = In[(I + 1) % In.size()];
			const float Dp = Cross(A, B, P) * Sign;
			const float Dq = Cross(A, B, Q) * Sign;
			if (Dp >= 0.0f)
			{
				Out.push_back(P);
			}
			if ((Dp >= 0.0f) != (Dq >= 0.0f))
			{
				const float T = Dp / (Dp - Dq);
				Out.push_back(Lerp2(P, Q, T));
			}
		}
	}
	return Out.size() >= 3 ? Out : Pts();
}

bool Inside(const Pts& P, Vec2 Q)
{
	bool In = false;
	for (size_t I = 0, J = P.size() - 1; I < P.size(); J = I++)
	{
		if ((P[I].Y > Q.Y) != (P[J].Y > Q.Y) && Q.X < (P[J].X - P[I].X) * (Q.Y - P[I].Y) / (P[J].Y - P[I].Y) + P[I].X)
		{
			In = !In;
		}
	}
	return In;
}

Color WithA(Color C, float A)
{
	C.A *= A;
	return C;
}

Color Lum(const Color& C, float K)
{
	return K >= 0.0f ? Mix(C, Hex(0xffffff), K) : Mix(C, Hex(0x000000), -K);
}

float Luma(const Color& C)
{
	return 0.299f * C.R + 0.587f * C.G + 0.114f * C.B;
}

/** A soft round glow or shadow: solid in the middle, gone at the edge. */
void Soft(Canvas& C, float X, float Y, float Rx, float Ry, const Color& Col, float A)
{
	if (A <= 0.003f)
	{
		return;
	}
	if (PortraitCoarse > 0)
	{
		// The small tile: the light or shade as one flat, faint ellipse (a radial fill is six rings of vertices).
		if (A >= 0.2f)
		{
			C.FillEllipse(X, Y, Rx * 0.7f, Ry * 0.7f, WithA(Col, A * 0.45f));
		}
		return;
	}
	const float R = std::max(Rx, Ry);
	C.Save();
	C.Translate(X, Y);
	C.Scale(Rx / R, Ry / R);
	C.FillCircle(0.0f, 0.0f, R, Paint::Radial({0.0f, 0.0f}, 0.0f, {0.0f, 0.0f}, R, WithA(Col, A), 0.45f, WithA(Col, A * 0.55f), WithA(Col, 0.0f)));
	C.Restore();
}

/** Small deterministic noise for textures (stubble, buzz cuts). */
float Hash(int I)
{
	uint32_t X = static_cast<uint32_t>(I) * 2654435761u;
	X ^= X >> 13;
	X *= 0x5bd1e995u;
	X ^= X >> 15;
	return static_cast<float>(X & 0xffffu) / 65535.0f;
}

// ------------------------------------------------------------------ the head

/** Proportions from the look (units: the head is about 210 tall, eyes on y = 0). */
struct Geo
{
	float CheekW = 72.0f;
	float TempleW = 68.0f;
	float JawW = 56.0f;
	float ChinW = 22.0f;
	float ChinY = 100.0f;
	float CrownY = -112.0f;
	float NeckW = 36.0f;
	float ShoulderW = 176.0f;
	float EyeX = 31.0f;
	float EyeW = 14.5f;
	float EyeH = 7.2f;
	float HairlineY = -66.0f;
	bool SoftFace = false;
	float Heavy = 0.0f;
};

Geo GeoOf(const hero::Look& L, int Age)
{
	struct Shape
	{
		float Cheek, Temple, Jaw, Chin, ChinY;
	};
	static const Shape Shapes[6] = {{72, 68, 56, 22, 100}, {74, 71, 67, 32, 96}, {77, 70, 63, 28, 94}, {67, 65, 54, 22, 110}, {75, 72, 47, 14, 100}, {71, 67, 60, 16, 104}};
	const Shape& S = Shapes[std::clamp(L.Face, 0, 5)];
	Geo G;
	G.CheekW = S.Cheek;
	G.TempleW = S.Temple;
	G.JawW = S.Jaw;
	G.ChinW = S.Chin;
	G.ChinY = S.ChinY;
	G.SoftFace = L.Body == 1;
	if (G.SoftFace)
	{
		G.CheekW -= 2.0f;
		G.TempleW -= 1.0f;
		G.JawW -= 6.0f;
		G.ChinW -= 3.0f;
		G.ChinY -= 3.0f;
		G.NeckW = 29.0f;
		G.ShoulderW = 150.0f;
		G.EyeW = 15.0f;
		G.EyeH = 7.8f;
	}
	switch (L.Build)
	{
	case 0:
		G.CheekW -= 3.0f;
		G.JawW -= 3.0f;
		G.NeckW -= 3.0f;
		G.ShoulderW -= 14.0f;
		break;
	case 2:
		G.JawW += 3.0f;
		G.NeckW += 5.0f;
		G.ShoulderW += 16.0f;
		break;
	case 3:
		G.CheekW += 6.0f;
		G.JawW += 9.0f;
		G.ChinW += 5.0f;
		G.ChinY += 3.0f;
		G.NeckW += 7.0f;
		G.ShoulderW += 18.0f;
		G.Heavy = 1.0f;
		break;
	default: break;
	}
	// A receding hairline for some, past fifty.
	if (!G.SoftFace && Age > 50)
	{
		G.HairlineY -= std::min(14.0f, Fl(Age - 50) * 0.8f);
	}
	return G;
}

struct Head
{
	Geo G;
	Pts Outline; // dense, closed
	Vec2 Center{0.0f, -8.0f};
	float Radius[360] = {};

	void Build()
	{
		const Pts Right = {{0.0f, G.CrownY},
			{G.TempleW * 0.58f, G.CrownY + 7.0f},
			{G.TempleW * 0.93f, G.CrownY + 36.0f},
			{G.TempleW, -40.0f},
			{G.CheekW, 4.0f},
			{G.CheekW * 0.5f + G.JawW * 0.5f + 1.0f, 36.0f},
			{G.JawW, 66.0f},
			{G.ChinW + (G.JawW - G.ChinW) * 0.42f, G.ChinY - 15.0f},
			{G.ChinW, G.ChinY - 3.0f},
			{0.0f, G.ChinY}};
		Outline = Spline(Mirror(Right), 8, true);
		// Polar radii from the center, one per degree (0 up, clockwise), by casting rays at the outline.
		for (int D = 0; D < 360; ++D)
		{
			const Vec2 Dir{std::sin(Fl(D) * Deg), -std::cos(Fl(D) * Deg)};
			float Best = 0.0f;
			for (size_t I = 0; I < Outline.size(); ++I)
			{
				const Vec2 A = Outline[I];
				const Vec2 B = Outline[(I + 1) % Outline.size()];
				const Vec2 E{B.X - A.X, B.Y - A.Y};
				const float Den = Dir.X * E.Y - Dir.Y * E.X;
				if (std::fabs(Den) < 1e-6f)
				{
					continue;
				}
				const Vec2 Ao{A.X - Center.X, A.Y - Center.Y};
				const float T = (Ao.X * E.Y - Ao.Y * E.X) / Den;
				const float U = (Ao.X * Dir.Y - Ao.Y * Dir.X) / Den;
				if (T > 0.0f && U >= 0.0f && U <= 1.0f)
				{
					Best = std::max(Best, T);
				}
			}
			Radius[D] = Best;
		}
	}

	float RadiusAt(float Degrees) const
	{
		float D = std::fmod(Degrees, 360.0f);
		if (D < 0.0f)
		{
			D += 360.0f;
		}
		const int I = static_cast<int>(D);
		const float F = D - Fl(I);
		return Radius[I % 360] * (1.0f - F) + Radius[(I + 1) % 360] * F;
	}

	/** The outline in a direction (degrees, 0 up, clockwise), pushed out by Off. */
	Vec2 Rim(float Degrees, float Off) const
	{
		const float R = RadiusAt(Degrees) + Off;
		return {Center.X + R * std::sin(Degrees * Deg), Center.Y - R * std::cos(Degrees * Deg)};
	}

	/** Outer edge from A0 to A1 degrees (A0 < A1), pushed out by Thick(angle). */
	Pts Arc(float A0, float A1, const std::function<float(float)>& Thick, float Step = 3.0f) const
	{
		if (PortraitCoarse > 0)
		{
			Step *= 3.0f; // the small tile's hair and hats: a point every 9 degrees or so
		}
		Pts Out;
		for (float A = A0; A < A1; A += Step)
		{
			Out.push_back(Rim(A, Thick(A)));
		}
		Out.push_back(Rim(A1, Thick(A1)));
		return Out;
	}
};

/**
 * Update 3 (S-T): the thumbnails' heads, built once per face shape (Build casts 360 rays at the outline: a fifth of a
 * small thumbnail's paint). Keyed by what GeoOf reads and the coarse switch; the default overload never asks for it.
 */
const Head& HeadFor(const hero::Look& L, int Age)
{
	struct Entry
	{
		uint64_t Key;
		Head H;
	};
	thread_local std::vector<Entry> Heads;
	const uint64_t Key = (static_cast<uint64_t>(std::clamp(L.Face, 0, 63)) << 0) | (static_cast<uint64_t>(std::clamp(L.Body, 0, 63)) << 6) | (static_cast<uint64_t>(std::clamp(L.Build, 0, 63)) << 12) |
		(static_cast<uint64_t>(std::clamp(Age, 0, 255)) << 18) | (static_cast<uint64_t>(PortraitCoarse > 0 ? 1 : 0) << 26);
	for (size_t I = 0; I < Heads.size(); ++I)
	{
		if (Heads[I].Key == Key)
		{
			return Heads[I].H;
		}
	}
	if (Heads.size() >= 12)
	{
		Heads.erase(Heads.begin());
	}
	Heads.push_back(Entry{Key, Head()});
	Head& H = Heads.back().H;
	H.G = GeoOf(L, Age);
	H.Build();
	return H;
}

/** Everything the parts of the drawing share. */
struct Ctx
{
	Canvas* C = nullptr;
	const hero::Character* Who = nullptr;
	Head H;
	Color Skin, SkinLight, SkinShade, SkinDeep, Blush;
	Color Hair, HairLight, HairDark, Brow, Beard;
	Color Cloth, ClothLight, ClothDark, Tee;
	PortraitLight Light;
	float Breath = 0.0f;
	float EyeOpen = 1.0f;
	float LookX = 0.0f;
	float Age = 24.0f;
	float Lines = 0.0f;
	float HatTop = 0.0f; // the hat's lowest edge on the forehead (0: no hat)
	// Update 3 (SPEC §6.1): the thumbnails' face and detail. Null and false for every screen that draws the default
	// overload, so its draw lists stay byte-identical (rerun_ui pins the ID card, the creator and the barber).
	const PortraitExpression* E = nullptr;
	bool Lod = false; // no hair strands, stipple, wrinkles or fine strokes (a small tile)
};

// ------------------------------------------------------------------ hair

int HairStyle(const Ctx& X)
{
	return X.Who->Appearance.Hair;
}

/** The hairline across the forehead, right half from the sideburn up to the middle. */
Pts HairlineRight(const Ctx& X, float Drop)
{
	const Geo& G = X.H.G;
	const Vec2 Burn = X.H.Rim(97.0f, -0.5f);
	return {Burn,
		{G.CheekW - 9.0f, Burn.Y - 2.0f},
		{G.TempleW * 0.9f, -40.0f},
		{G.TempleW * 0.72f, G.HairlineY + 12.0f + Drop * 0.5f},
		{G.TempleW * 0.36f, G.HairlineY + 2.0f + Drop},
		{0.0f, G.HairlineY + Drop}};
}

/** A cap of hair over the skull: outer edge Thick(angle) above the outline, inner edge the hairline. */
Pts Cap(const Ctx& X, const std::function<float(float)>& Thick, float Drop, const Pts* HairlineOverride = nullptr)
{
	Pts Out = X.H.Arc(-97.0f, 97.0f, Thick);
	const Pts Hr = HairlineOverride ? *HairlineOverride : HairlineRight(X, Drop);
	const Pts Smooth = Spline(Hr, 5, false);
	for (size_t I = 1; I < Smooth.size(); ++I)
	{
		Out.push_back(Smooth[I]);
	}
	const Pts Left = Flip(Smooth);
	for (size_t I = Left.size() - 1; I-- > 1;)
	{
		Out.push_back(Left[I]);
	}
	return Out;
}

void HairFill(Ctx& X, const Pts& P, float Alpha = 1.0f)
{
	const Geo& G = X.H.G;
	X.C->FillPolygon(P, Paint::Linear({-G.CheekW - 30.0f, -100.0f}, {G.CheekW + 30.0f, 40.0f}, WithA(X.HairLight, Alpha), WithA(X.HairDark, Alpha)));
}

/** Strands: soft strokes from the hairline toward the crown. */
void Strands(Ctx& X, float Thick, int Count, float Alpha, float Spread = 1.0f)
{
	if (X.Lod)
	{
		return;
	}
	const Geo& G = X.H.G;
	for (int I = 0; I < Count; ++I)
	{
		const float K = (Fl(I) + 0.5f) / Fl(Count) * 2.0f - 1.0f;
		const float A = K * 70.0f * Spread;
		const Vec2 Start{K * G.TempleW * 0.8f, G.HairlineY + 4.0f - (1.0f - std::fabs(K)) * 2.0f + std::fabs(K) * 12.0f};
		const Vec2 End = X.H.Rim(A * 0.55f - 8.0f, Thick * 0.45f);
		const Vec2 Mid = X.H.Rim(A * 0.8f, Thick * 0.75f);
		const Pts Curve = Spline({Start, Lerp2(Start, Mid, 0.5f), Mid, End}, 4, false);
		X.C->StrokePolyline(Curve, false, WithA(I % 2 == 0 ? X.HairLight : X.HairDark, Alpha), 1.4f, true);
	}
}

/** Stippled texture inside a shape (buzz cuts, stubble). */
void Stipple(Ctx& X, const Pts& Shape, const Color& Col, float Alpha, int Count, int Seed)
{
	if (X.Lod)
	{
		// The small tile: the shape's tone without the dots (its fill is already under it).
		return;
	}
	if (X.E)
	{
		// A thumbnail's face (a head 130 pixels tall at most): half the dots, a little darker (S-T's vertex budget).
		Count /= 2;
		Alpha = std::min(1.0f, Alpha * 1.25f);
	}
	float MinX = 1e9f, MinY = 1e9f, MaxX = -1e9f, MaxY = -1e9f;
	for (const Vec2& P : Shape)
	{
		MinX = std::min(MinX, P.X);
		MinY = std::min(MinY, P.Y);
		MaxX = std::max(MaxX, P.X);
		MaxY = std::max(MaxY, P.Y);
	}
	for (int I = 0; I < Count; ++I)
	{
		const Vec2 Q{MinX + (MaxX - MinX) * Hash(Seed + I * 2), MinY + (MaxY - MinY) * Hash(Seed + I * 2 + 1)};
		if (Inside(Shape, Q))
		{
			X.C->FillRect({Q.X - 0.6f, Q.Y - 0.6f, 1.3f, 1.3f}, WithA(Col, Alpha * (0.5f + 0.5f * Hash(Seed + I * 7))));
		}
	}
}

void HairRim(Ctx& X, const std::function<float(float)>& Thick, float A0 = 18.0f, float A1 = 95.0f)
{
	const Pts Edge = X.H.Arc(A0, A1, [&Thick](float A) { return Thick(A) - 0.5f; }, 4.0f);
	X.C->StrokePolyline(Edge, false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.0f, true);
}

/** What grows behind the head: long hair, an afro, the tail of a ponytail. */
void BackHair(Ctx& X)
{
	const Geo& G = X.H.G;
	const int Style = HairStyle(X);
	const bool Hat = X.Who->Appearance.Hat != 0;
	if (Style == 7)
	{
		// Afro: a round cloud, smaller under a hat.
		const float K = Hat ? 0.84f : 1.0f;
		Pts Cloud;
		for (int D = 0; D < 360; D += 4)
		{
			const float A = Fl(D) * Deg;
			const float Bump = 4.5f * std::sin(Fl(D) * 0.21f * 9.0f * Deg * 4.0f) + 2.5f * std::sin(Fl(D) * Deg * 13.0f);
			Cloud.push_back({(G.CheekW + 58.0f + Bump) * K * std::sin(A), -58.0f * K - (122.0f + Bump) * K * std::cos(A) + (D > 90 && D < 270 ? 20.0f : 0.0f)});
		}
		HairFill(X, Cloud);
		Soft(*X.C, -30.0f, -120.0f, 70.0f, 46.0f, X.HairLight, 0.22f);
		for (int I = 0; I < (X.Lod ? 0 : X.E ? 45 : 90); ++I)
		{
			const float A = Hash(I * 3) * 2.0f * 3.1415926f;
			const float R = (40.0f + 90.0f * Hash(I * 3 + 1)) * K;
			const float Cx = R * std::sin(A) * (G.CheekW + 58.0f) / 130.0f;
			const float Cy = -58.0f * K - R * std::cos(A);
			X.C->StrokeArc(Cx, Cy, 3.5f + 2.5f * Hash(I * 3 + 2), A, A + 2.4f, WithA(I % 2 ? X.HairLight : X.HairDark, 0.35f), 1.3f, true);
		}
		const Pts Edge = Moved(Cloud, 0.0f, 0.0f);
		Pts RimPart;
		for (size_t I = 3; I < Edge.size() / 2 - 2; ++I)
		{
			RimPart.push_back(Edge[I]);
		}
		X.C->StrokePolyline(RimPart, false, WithA(X.Light.Rim, 0.45f * X.Light.RimStrength), 2.0f, true);
		return;
	}
	if (Style == 8 || Style == 10 || Style == 11)
	{
		// Shoulder length, braids and a bob: a panel of hair behind the face.
		const float Bottom = Style == 11 ? 78.0f : 196.0f;
		const float Wide = Style == 11 ? G.CheekW + 14.0f : G.CheekW + 30.0f;
		Pts R = X.H.Arc(0.0f, 100.0f, [](float) { return 13.0f; }, 6.0f);
		R.push_back({Wide, 20.0f});
		R.push_back({Wide + (Style == 11 ? 4.0f : 8.0f), Bottom - 30.0f});
		R.push_back({Wide - 6.0f, Bottom});
		R.push_back({G.NeckW * 0.4f, Bottom + (Style == 11 ? -4.0f : 8.0f)});
		R.push_back({0.0f, Bottom + (Style == 11 ? -4.0f : 8.0f)});
		const Pts Back = Spline(Mirror(R), 3, true);
		HairFill(X, Back);
		X.C->FillPolygon(Back, Paint::Linear({0.0f, 40.0f}, {0.0f, Bottom}, WithA(X.HairDark, 0.0f), WithA(X.HairDark, 0.45f)));
		if (Style == 10 && !X.Lod)
		{
			for (int I = -7; I <= 7; ++I)
			{
				const float Bx = Fl(I) * Wide / 7.5f;
				Pts Braid;
				for (float Y = -40.0f; Y < Bottom - 4.0f; Y += 7.0f)
				{
					Braid.push_back({Bx + (static_cast<int>(Y / 7.0f) % 2 == 0 ? -2.2f : 2.2f), Y});
				}
				X.C->StrokePolyline(Braid, false, WithA(X.HairLight, 0.28f), 1.6f, true);
			}
		}
		Pts RimEdge;
		for (const Vec2& P : Back)
		{
			if (P.X > Wide * 0.55f && P.Y < Bottom - 10.0f)
			{
				RimEdge.push_back(P);
			}
		}
		X.C->StrokePolyline(RimEdge, false, WithA(X.Light.Rim, 0.5f * X.Light.RimStrength), 2.0f, true);
		return;
	}
	if (Style == 9)
	{
		if (Hat)
		{
			// Under a hat the tail is pulled through low at the back, so it falls from under the hat's edge.
			const Pts Tail = Spline({X.H.Rim(64.0f, 2.0f), {G.CheekW + 16.0f, -50.0f}, {G.CheekW + 28.0f, -6.0f}, {G.CheekW + 27.0f, 54.0f}, {G.CheekW + 18.0f, 130.0f}, {G.CheekW + 4.0f, 80.0f},
										{G.CheekW + 4.0f, 6.0f}, X.H.Rim(88.0f, -6.0f)},
				4, true);
			HairFill(X, Tail);
			X.C->StrokePolyline(Spline({{G.CheekW + 20.0f, -36.0f}, {G.CheekW + 26.0f, 10.0f}, {G.CheekW + 22.0f, 70.0f}}, 5, false), false, WithA(X.Light.Rim, 0.5f * X.Light.RimStrength), 2.0f, true);
			return;
		}
		// A high ponytail: the tie on the crown, the tail falling behind the right shoulder.
		const Pts Tail = Spline({X.H.Rim(28.0f, 4.0f), {G.CheekW + 18.0f, -96.0f}, {G.CheekW + 34.0f, -40.0f}, {G.CheekW + 30.0f, 40.0f}, {G.CheekW + 20.0f, 120.0f}, {G.CheekW + 4.0f, 70.0f},
									{G.CheekW + 6.0f, -20.0f}, X.H.Rim(60.0f, -6.0f)},
			4, true);
		HairFill(X, Tail);
		X.C->StrokePolyline(Spline({{G.CheekW + 22.0f, -80.0f}, {G.CheekW + 30.0f, -20.0f}, {G.CheekW + 24.0f, 60.0f}}, 5, false), false, WithA(X.Light.Rim, 0.5f * X.Light.RimStrength), 2.0f, true);
	}
}

/** The hood of a hoodie lying behind the neck. */
void HoodBack(Ctx& X)
{
	const Geo& G = X.H.G;
	const Pts Hood = Spline(Mirror({{0.0f, 92.0f}, {G.NeckW + 18.0f, 98.0f}, {G.NeckW + 40.0f, 116.0f}, {G.NeckW + 48.0f, 146.0f}, {G.NeckW + 34.0f, 176.0f}, {0.0f, 170.0f}}), 5, true);
	X.C->FillPolygon(Hood, Paint::Linear({-90.0f, 0.0f}, {90.0f, 0.0f}, Lum(X.Cloth, -0.05f), Lum(X.Cloth, -0.45f)));
	// The lining shows inside.
	const Pts Lining = Spline(Mirror({{0.0f, 102.0f}, {G.NeckW + 10.0f, 106.0f}, {G.NeckW + 28.0f, 122.0f}, {G.NeckW + 30.0f, 150.0f}, {0.0f, 160.0f}}), 5, true);
	X.C->FillPolygon(Lining, Lum(X.Cloth, -0.6f));
}

void FrontHair(Ctx& X)
{
	const Geo& G = X.H.G;
	const int Style = HairStyle(X);
	Canvas& C = *X.C;
	switch (Style)
	{
	case 0:
	{
		// Shaved: only the shadow of it, and the shine of the scalp.
		const Pts P = Cap(X, [](float) { return 0.4f; }, 0.0f);
		C.FillPolygon(P, WithA(X.HairDark, 0.14f));
		Soft(C, -20.0f, -92.0f, 30.0f, 12.0f, Hex(0xffffff), 0.12f);
		break;
	}
	case 1:
	{
		const Pts P = Cap(X, [](float) { return 2.2f; }, 2.0f);
		C.FillPolygon(P, WithA(X.Hair, 0.78f));
		Stipple(X, P, X.HairDark, 0.5f, 520, 11);
		HairRim(X, [](float) { return 2.2f; });
		break;
	}
	case 2:
	{
		auto Thick = [](float A) { return 6.0f + 1.5f * std::cos(A * Deg); };
		const Pts P = Cap(X, Thick, 4.0f);
		HairFill(X, P);
		Stipple(X, P, X.HairDark, 0.35f, 260, 21);
		Strands(X, 6.0f, 14, 0.18f);
		HairRim(X, Thick);
		break;
	}
	case 3:
	{
		// Textured crop: choppy on top, a ragged fringe.
		Pts Hr = HairlineRight(X, 0.0f);
		Pts Fr;
		Fr.push_back(Hr[0]);
		Fr.push_back(Hr[1]);
		Fr.push_back(Hr[2]);
		for (int I = 0; I < 6; ++I)
		{
			const float Px = G.TempleW * (0.7f - Fl(I) * 0.13f);
			Fr.push_back({Px, G.HairlineY + 22.0f + (I % 2 == 0 ? 7.0f : -2.0f)});
		}
		Fr.push_back({0.0f, G.HairlineY + 20.0f});
		auto Thick = [](float A) { return 13.0f + 3.0f * std::sin(A * 0.35f); };
		Pts Out = X.H.Arc(-97.0f, 97.0f, Thick, 4.0f);
		for (size_t I = 1; I < Fr.size(); ++I)
		{
			Out.push_back(Fr[I]);
		}
		for (size_t I = Fr.size() - 1; I-- > 1;)
		{
			Out.push_back({-Fr[I].X + (I % 2 == 0 ? 3.0f : 0.0f), Fr[I].Y + (I % 3 == 0 ? 4.0f : 0.0f)});
		}
		HairFill(X, Out);
		Strands(X, 13.0f, 18, 0.2f, 1.05f);
		HairRim(X, Thick);
		break;
	}
	case 4:
	{
		// Side part: more on the left, the part line, a sweep across the forehead.
		auto Thick = [](float A) { return A < -20.0f ? 17.0f : A < 0.0f ? 12.0f - A * 0.25f : 12.0f; };
		const Pts P = Cap(X, Thick, 6.0f);
		HairFill(X, P);
		Strands(X, 14.0f, 16, 0.2f);
		const Vec2 PartTop = X.H.Rim(-28.0f, 14.0f);
		C.StrokePolyline(Spline({{-G.TempleW * 0.42f, G.HairlineY + 2.0f}, {-G.TempleW * 0.44f, G.HairlineY - 18.0f}, PartTop}, 5, false), false, WithA(X.HairDark, 0.7f), 1.8f, true);
		const Pts Sweep = Spline({{-G.TempleW * 0.4f, G.HairlineY + 1.0f}, {0.0f, G.HairlineY - 8.0f}, {G.TempleW * 0.6f, G.HairlineY + 4.0f}, {G.TempleW * 0.86f, G.HairlineY + 18.0f}}, 5, false);
		C.StrokePolyline(Sweep, false, WithA(X.HairLight, 0.35f), 2.0f, true);
		HairRim(X, Thick);
		break;
	}
	case 5:
	{
		// Undercut: clipped sides, a slicked-back top.
		const Pts Sides = Cap(X, [](float) { return 2.0f; }, 3.0f);
		C.FillPolygon(Sides, WithA(X.Hair, 0.6f));
		Stipple(X, Sides, X.HairDark, 0.4f, 380, 31);
		auto TopThick = [](float A) { return 9.0f + 7.0f * std::cos(A * Deg * 1.4f); };
		Pts Top = X.H.Arc(-62.0f, 62.0f, TopThick, 4.0f);
		Top.push_back(X.H.Rim(62.0f, 1.0f));
		Top.push_back({G.TempleW * 0.6f, G.HairlineY - 2.0f});
		Top.push_back({0.0f, G.HairlineY - 7.0f});
		Top.push_back({-G.TempleW * 0.6f, G.HairlineY - 2.0f});
		Top.push_back(X.H.Rim(-62.0f, 1.0f));
		const Pts Smooth = Spline(Top, 3, true);
		HairFill(X, Smooth);
		for (int I = 0; I < (X.Lod ? 0 : 15); ++I)
		{
			const float K = Fl(I - 7) / 7.0f;
			C.StrokePolyline(Spline({{K * G.TempleW * 0.58f, G.HairlineY - 5.0f}, X.H.Rim(K * 36.0f, 12.0f), X.H.Rim(K * 44.0f + (K > 0 ? 10.0f : -10.0f), 6.0f)}, 5, false), false,
				WithA(I % 2 ? X.HairLight : X.HairDark, 0.32f), 1.3f, true);
		}
		C.StrokePolyline(Spline({{-G.TempleW * 0.5f, G.HairlineY - 4.0f}, {0.0f, G.HairlineY - 9.0f}, {G.TempleW * 0.5f, G.HairlineY - 4.0f}}, 5, false), false, WithA(X.HairLight, 0.4f), 2.0f, true);
		HairRim(X, TopThick, 15.0f, 60.0f);
		break;
	}
	case 6:
	case 7:
	{
		// Curls (and the front edge of an afro): a bumpy cap with ringlets.
		const bool Afro = Style == 7;
		const float T = Afro ? 6.0f : 19.0f;
		Pts Hr = HairlineRight(X, Afro ? -2.0f : 4.0f);
		const Pts P = Cap(X, [T](float A) { return T + 4.0f * std::sin(A * Deg * 14.0f); }, 0.0f, &Hr);
		HairFill(X, P);
		for (int I = 0; I < (X.Lod ? 0 : Afro ? 30 : 70); ++I)
		{
			const float A = -90.0f + 180.0f * Hash(I * 5 + 3);
			const Vec2 Q = X.H.Rim(A, T * (0.2f + 0.7f * Hash(I * 5 + 4)) - (Afro ? 4.0f : 0.0f));
			if (Q.Y > G.HairlineY + 6.0f && std::fabs(Q.X) < G.TempleW * 0.6f)
			{
				continue;
			}
			C.StrokeArc(Q.X, Q.Y, 3.0f + 2.0f * Hash(I), Hash(I * 2) * 6.0f, Hash(I * 2) * 6.0f + 3.6f, WithA(I % 2 ? X.HairLight : X.HairDark, 0.45f), 1.4f, true);
		}
		if (!Afro)
		{
			HairRim(X, [T](float A) { return T + 4.0f * std::sin(A * Deg * 14.0f); });
		}
		break;
	}
	case 8:
	{
		// Shoulder length: a center part and curtains over the temples.
		Pts Hr = {X.H.Rim(97.0f, -0.5f), {G.CheekW - 6.0f, -20.0f}, {G.TempleW * 0.8f, -50.0f}, {G.TempleW * 0.45f, G.HairlineY + 6.0f}, {G.TempleW * 0.12f, G.HairlineY - 2.0f}, {0.0f, G.HairlineY - 4.0f}};
		const Pts P = Cap(X, [](float) { return 13.0f; }, 0.0f, &Hr);
		HairFill(X, P);
		Strands(X, 12.0f, 16, 0.2f);
		C.StrokePolyline({{0.0f, G.HairlineY - 4.0f}, X.H.Rim(0.0f, 6.0f)}, false, WithA(X.HairDark, 0.6f), 1.6f, true);
		HairRim(X, [](float) { return 13.0f; });
		break;
	}
	case 9:
	{
		// Ponytail: pulled back tight, with a shine.
		Pts Hr = HairlineRight(X, -2.0f);
		const Pts P = Cap(X, [](float) { return 8.0f; }, 0.0f, &Hr);
		HairFill(X, P);
		for (int I = 0; I < (X.Lod ? 0 : 12); ++I)
		{
			const float K = Fl(I) / 11.0f * 2.0f - 1.0f;
			C.StrokePolyline(Spline({{K * G.TempleW * 0.85f, G.HairlineY + 4.0f + std::fabs(K) * 16.0f}, X.H.Rim(K * 50.0f, 6.0f), X.H.Rim(26.0f, 8.0f)}, 5, false), false, WithA(I % 2 ? X.HairLight : X.HairDark, 0.22f),
				1.3f, true);
		}
		C.StrokePolyline(X.H.Arc(-55.0f, -20.0f, [](float) { return 5.0f; }), false, WithA(Hex(0xffffff), 0.22f), 3.0f, true);
		if (X.Who->Appearance.Hat == 0)
		{
			// The tie on the crown (under a hat it's out of sight, and the tail comes out lower).
			const Vec2 Tie = X.H.Rim(26.0f, 8.0f);
			C.FillEllipse(Tie.X, Tie.Y, 7.0f, 5.0f, Lum(X.Cloth, -0.2f));
		}
		HairRim(X, [](float) { return 8.0f; });
		break;
	}
	case 10:
	{
		// Braids: a center part and rows running back.
		Pts Hr = HairlineRight(X, -2.0f);
		const Pts P = Cap(X, [](float) { return 9.0f; }, 0.0f, &Hr);
		HairFill(X, P);
		for (int I = 0; I < (X.Lod ? 0 : 10); ++I)
		{
			const float K = (Fl(I) + 0.5f) / 10.0f * 2.0f - 1.0f;
			const Vec2 From{K * G.TempleW * 0.86f, G.HairlineY + 4.0f + std::fabs(K) * 18.0f};
			const Vec2 To = X.H.Rim(K * 95.0f, 7.0f);
			Pts Row;
			for (int S = 0; S <= 10; ++S)
			{
				const Vec2 Q = Lerp2(From, To, Fl(S) / 10.0f);
				Row.push_back({Q.X + (S % 2 == 0 ? -1.6f : 1.6f), Q.Y});
			}
			C.StrokePolyline(Row, false, WithA(X.HairLight, 0.32f), 1.5f, true);
		}
		C.StrokePolyline({{0.0f, G.HairlineY}, X.H.Rim(0.0f, 4.0f)}, false, WithA(X.Skin, 0.5f), 1.4f, true);
		HairRim(X, [](float) { return 9.0f; });
		break;
	}
	case 11:
	{
		// Bob: blunt bangs above the brows.
		Pts Hr = {X.H.Rim(97.0f, -0.5f), {G.CheekW - 8.0f, -24.0f}, {G.TempleW * 0.8f, -34.0f}, {G.TempleW * 0.4f, -33.0f}, {0.0f, -32.0f}};
		const Pts P = Cap(X, [](float) { return 14.0f; }, 0.0f, &Hr);
		HairFill(X, P);
		for (int I = 0; I < (X.Lod ? 0 : 14); ++I)
		{
			const float Bx = (Fl(I) / 13.0f * 2.0f - 1.0f) * G.TempleW * 0.85f;
			C.StrokePolyline(Spline({{Bx, -34.0f}, {Bx * 0.92f, G.HairlineY}, X.H.Rim(Bx * 0.8f, 10.0f)}, 4, false), false, WithA(I % 2 ? X.HairLight : X.HairDark, 0.22f), 1.4f, true);
		}
		HairRim(X, [](float) { return 14.0f; });
		break;
	}
	default: break;
	}
}

/** Long hair falling in front of the shoulders, and the sides of a bob framing the face. */
void FrontLocks(Ctx& X)
{
	const Geo& G = X.H.G;
	const int Style = HairStyle(X);
	Canvas& C = *X.C;
	if (Style == 8)
	{
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			const Pts Lock = Spline({{S * (G.CheekW - 6.0f), -24.0f}, {S * (G.CheekW + 2.0f), 30.0f}, {S * (G.CheekW + 12.0f), 110.0f}, {S * (G.CheekW + 30.0f), 196.0f}, {S * (G.CheekW + 48.0f), 192.0f},
										{S * (G.CheekW + 34.0f), 110.0f}, {S * (G.CheekW + 22.0f), 30.0f}, {S * (G.CheekW + 10.0f), -30.0f}},
				4, true);
			HairFill(X, Lock);
			for (int I = 0; I < (X.Lod ? 0 : 4); ++I)
			{
				const float O = Fl(I) * 7.0f + 4.0f;
				C.StrokePolyline(Spline({{S * (G.CheekW - 2.0f + O * 0.4f), -10.0f}, {S * (G.CheekW + 6.0f + O * 0.6f), 60.0f}, {S * (G.CheekW + 18.0f + O), 150.0f}}, 5, false), false,
					WithA(I % 2 ? X.HairLight : X.HairDark, 0.3f), 1.3f, true);
			}
		}
	}
	else if (Style == 10)
	{
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			for (int B = 0; B < 3; ++B)
			{
				const float Bx = S * (G.CheekW + 4.0f + Fl(B) * 9.0f);
				Pts Braid;
				for (float Y = -30.0f; Y < 200.0f; Y += 6.0f)
				{
					Braid.push_back({Bx + S * (Y + 30.0f) * 0.08f + (static_cast<int>(Y / 6.0f) % 2 == 0 ? -2.0f : 2.0f), Y});
				}
				C.StrokePolyline(Braid, false, X.HairDark, 8.0f, true);
				for (size_t K = 0; K + 1 < (X.Lod ? size_t{1} : Braid.size()); K += (X.E ? 2 : 1))
				{
					const Vec2 Q = Lerp2(Braid[K], Braid[K + 1], 0.5f);
					C.FillEllipse(Q.X, Q.Y, 3.4f, 2.4f, K % 2 ? X.Hair : Lum(X.Hair, 0.06f));
					C.StrokeArc(Q.X, Q.Y + 0.6f, 3.0f, 3.5f, 5.9f, WithA(X.HairLight, 0.45f), 0.9f, true);
				}
			}
		}
	}
	else if (Style == 11)
	{
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			const Pts Cheek = Spline({{S * (G.CheekW - 8.0f), -30.0f}, {S * (G.CheekW - 11.0f), 20.0f}, {S * (G.CheekW - 9.0f), 76.0f}, {S * (G.CheekW + 4.0f), 80.0f}, {S * (G.CheekW + 13.0f), 70.0f},
										 {S * (G.CheekW + 14.0f), 20.0f}, {S * (G.CheekW + 10.0f), -40.0f}},
				4, true);
			HairFill(X, Cheek);
			C.FillPolygon(Cheek, Paint::Linear({0.0f, 20.0f}, {0.0f, 84.0f}, WithA(X.HairDark, 0.0f), WithA(X.HairDark, 0.35f)));
		}
	}
}

// ------------------------------------------------------------------ face

void Ear(Ctx& X, float Side)
{
	const Geo& G = X.H.G;
	const float Ex = Side * (G.CheekW - 3.0f);
	const Pts Shape = Spline({{Ex, -14.0f}, {Ex + Side * 9.0f, -16.0f}, {Ex + Side * 13.0f, -2.0f}, {Ex + Side * 10.0f, 20.0f}, {Ex + Side * 3.0f, 34.0f}, {Ex - Side * 4.0f, 30.0f}}, 5, true);
	const Color Base = Side < 0.0f ? Mix(X.Skin, X.SkinLight, 0.3f) : Mix(X.Skin, X.SkinShade, 0.55f);
	X.C->FillPolygon(Shape, Base);
	if (X.Lod)
	{
		return;
	}
	X.C->StrokePolyline(Spline({{Ex + Side * 5.0f, -8.0f}, {Ex + Side * 9.0f, 2.0f}, {Ex + Side * 6.0f, 18.0f}, {Ex + Side * 1.0f, 24.0f}}, 4, false), false, WithA(X.SkinDeep, 0.55f), 2.0f, true);
	if (Side > 0.0f)
	{
		X.C->StrokePolyline(Spline({{Ex + 9.0f, -14.0f}, {Ex + 13.5f, -2.0f}, {Ex + 10.5f, 20.0f}, {Ex + 4.0f, 33.0f}}, 4, false), false, WithA(X.Light.Rim, 0.5f * X.Light.RimStrength), 1.8f, true);
	}
}

void Neck(Ctx& X)
{
	const Geo& G = X.H.G;
	const Pts N = {{-G.NeckW * 0.9f, 30.0f}, {G.NeckW * 0.9f, 30.0f}, {G.NeckW * 1.06f, 160.0f}, {-G.NeckW * 1.06f, 160.0f}};
	X.C->FillPolygon(N, Paint::Linear({-G.NeckW, 0.0f}, {G.NeckW, 0.0f}, Mix(X.Skin, X.SkinLight, 0.1f), Mix(X.Skin, X.SkinShade, 0.7f)));
	// The jaw's shadow on the neck: the head moved down, cut to the neck.
	const Pts Shadow = ClipConvex(Moved(X.H.Outline, 2.0f, 14.0f), N);
	if (!Shadow.empty())
	{
		X.C->FillPolygon(Shadow, WithA(X.SkinDeep, 0.5f));
	}
	if (!G.SoftFace)
	{
		Soft(*X.C, -2.0f, 118.0f, 7.0f, 10.0f, X.SkinLight, 0.25f);
	}
	if (X.Lod)
	{
		return;
	}
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float S = Fl(Side);
		X.C->StrokePolyline(Spline({{S * 10.0f, 100.0f}, {S * 16.0f, 128.0f}, {S * 24.0f, 156.0f}}, 4, false), false, WithA(X.SkinDeep, 0.07f), 2.0f, true);
	}
	X.C->StrokePolyline({{G.NeckW * 0.9f, 60.0f}, {G.NeckW * 1.05f, 150.0f}}, false, WithA(X.Light.Rim, 0.45f * X.Light.RimStrength), 1.8f, true);
}

void HeadFill(Ctx& X)
{
	const Geo& G = X.H.G;
	Canvas& C = *X.C;
	C.FillPolygon(X.H.Outline, Paint::Linear({-G.CheekW, 0.0f}, {G.CheekW, 0.0f}, Mix(X.Skin, X.SkinLight, 0.55f), Mix(X.Skin, X.SkinShade, 0.6f)));
	// Form: forehead and cheekbone light, the turn of the jaw and the temple in shade.
	Soft(C, -18.0f, -62.0f, 34.0f, 22.0f, X.SkinLight, 0.4f);
	Soft(C, -40.0f, 14.0f, 22.0f, 16.0f, X.SkinLight, 0.35f);
	Soft(C, G.CheekW - 12.0f, 40.0f, 16.0f, 34.0f, X.SkinShade, 0.4f);
	Soft(C, G.TempleW - 8.0f, -44.0f, 14.0f, 26.0f, X.SkinShade, 0.35f);
	Soft(C, -36.0f, 32.0f, 17.0f, 11.0f, X.Blush, G.SoftFace ? 0.4f : 0.22f);
	Soft(C, 36.0f, 32.0f, 17.0f, 11.0f, X.Blush, G.SoftFace ? 0.34f : 0.18f);
	if (X.H.G.Heavy > 0.0f)
	{
		Soft(C, 0.0f, G.ChinY - 4.0f, 34.0f, 10.0f, X.SkinShade, 0.35f);
	}
	// Under the chin.
	Soft(C, 0.0f, G.ChinY - 2.0f, 20.0f, 6.0f, X.SkinShade, 0.3f);
}

/** The head's edge on the shadow side, caught by the rim light. */
void HeadRim(Ctx& X)
{
	Pts Edge;
	for (float A = 40.0f; A <= 150.0f; A += (X.Lod ? 10.0f : 3.0f))
	{
		Edge.push_back(X.H.Rim(A, -0.6f));
	}
	X.C->StrokePolyline(Edge, false, WithA(X.Light.Rim, 0.6f * X.Light.RimStrength), 2.4f, true);
}

void Eye(Ctx& X, float Side)
{
	const Geo& G = X.H.G;
	Canvas& C = *X.C;
	const float Cx = Side * G.EyeX;
	const float W = G.EyeW;
	float H = G.EyeH * X.EyeOpen;
	if (X.E)
	{
		// The expression's lids: wide with shock, a squint, half-closed when exhausted.
		H *= std::clamp(X.E->EyeOpen, 0.0f, 1.7f);
	}
	const float S = Side; // inner corner toward the nose
	const Vec2 Inner{Cx - S * W, 1.0f};
	const Vec2 Outer{Cx + S * W, -0.5f + (G.SoftFace ? -1.0f : 0.0f)};
	const Pts Upper = Spline({Inner, {Cx - S * W * 0.55f, 1.0f - H * 0.82f}, {Cx - S * W * 0.02f, 1.0f - H}, {Cx + S * W * 0.6f, 0.4f - H * 0.74f}, Outer}, 4, false);
	const Pts Lower = Spline({Outer, {Cx + S * W * 0.45f, 1.0f + H * 0.55f + 1.0f}, {Cx - S * W * 0.2f, 1.0f + H * 0.7f + 1.0f}, Inner}, 4, false);
	// Socket shading and the lid above.
	Soft(C, Cx, -3.0f, 21.0f, 13.0f, X.SkinShade, 0.28f);
	Soft(C, Cx - S * 9.0f, 3.0f, 8.0f, 6.0f, X.SkinDeep, 0.12f);
	if (X.EyeOpen < 0.25f)
	{
		C.StrokePolyline(Spline({Inner, {Cx, 2.5f}, Outer}, 4, false), false, Lum(X.SkinDeep, -0.3f), 2.2f, true);
		return;
	}
	if (X.E && H < G.EyeH * 0.25f)
	{
		// Shut by the expression: a laugh's lids curve up, anything else is a pressed line.
		const float Up = X.E->Smile > 0.4f ? -4.5f : 2.5f;
		C.StrokePolyline(Spline({Inner, {Cx, Up}, Outer}, 4, false), false, Lum(X.SkinDeep, -0.3f), 2.6f, true);
		return;
	}
	Pts Shape = Upper;
	for (size_t I = 1; I + 1 < Lower.size(); ++I)
	{
		Shape.push_back(Lower[I]);
	}
	C.FillPolygon(Shape, Paint::Linear({0.0f, -H}, {0.0f, H}, Mix(Hex(0xf2ede6), X.SkinShade, 0.35f), Hex(0xf4efe9)));
	// Iris and pupil, cut by the lids.
	const Color Iris = Hex(hero::EyeTone(X.Who->Appearance.Eyes));
	const float Ir = G.EyeH * 0.9f;
	float Ix = Cx + X.LookX;
	float Iy = 1.4f;
	float Pr = Ir * 0.42f;
	if (X.E)
	{
		// Where the expression looks, and a shocked stare's small pupils.
		Ix += std::clamp(X.E->LookX, -6.0f, 6.0f);
		Iy += std::clamp(X.E->LookY, -5.0f, 5.0f);
		Pr = X.E->EyeOpen > 1.2f ? Ir * 0.3f : Pr;
	}
	const Pts IrisPoly = ClipConvex(Ellipse(Ix, Iy, Ir, Ir, 24), Shape);
	if (!IrisPoly.empty())
	{
		C.FillPolygon(IrisPoly, Paint::Linear({0.0f, Iy - Ir}, {0.0f, Iy + Ir}, Lum(Iris, -0.45f), Lum(Iris, 0.12f)));
		const Pts Ring = X.Lod ? Pts() : ClipConvex(Ellipse(Ix, Iy, Ir - 0.3f, Ir - 0.3f, 24), Shape);
		if (!Ring.empty())
		{
			C.StrokePolyline(Ring, true, WithA(Lum(Iris, -0.6f), 0.6f), 1.0f);
		}
		const Pts Pupil = ClipConvex(Ellipse(Ix, Iy, Pr, Pr, 16), Shape);
		if (!Pupil.empty())
		{
			C.FillPolygon(Pupil, Hex(0x0b0907));
		}
		// The catchlights ride with the iris (the default face's literals exactly: its offset is +0).
		const float Dy = X.E ? Iy - 1.4f : 0.0f;
		C.FillCircle(Ix - 2.0f, -1.2f + Dy, 1.5f, WithA(Hex(0xffffff), 0.9f));
		if (!X.Lod)
		{
			C.FillCircle(Ix + 2.2f, 3.2f + Dy, 0.8f, WithA(Hex(0xffffff), 0.45f));
		}
	}
	// Lid shadow over the top of the eye, then the lash line and the crease.
	if (!X.Lod)
	{
		C.StrokePolyline(Upper, false, WithA(Lum(X.SkinDeep, -0.2f), 0.35f), 4.0f, true);
	}
	C.StrokePolyline(Upper, false, Lum(X.SkinDeep, -0.55f), G.SoftFace ? 2.6f : 2.0f, true);
	if (G.SoftFace && !X.Lod)
	{
		// A flick of liner and lashes at the outer corner.
		C.StrokePolyline({Outer, {Outer.X + S * 4.5f, Outer.Y - 3.0f}}, false, Lum(X.SkinDeep, -0.6f), 2.0f, true);
		for (int I = 0; I < 3; ++I)
		{
			const Vec2 B = Upper[Upper.size() - 2 - static_cast<size_t>(I) * 2];
			C.StrokePolyline({B, {B.X + S * 2.5f, B.Y - 3.0f}}, false, Lum(X.SkinDeep, -0.6f), 1.1f, true);
		}
	}
	if (!X.Lod)
	{
		C.StrokePolyline(Moved(Spline({{Cx - S * W * 0.7f, 1.0f - H * 0.7f}, {Cx, -H - 4.0f}, {Cx + S * W * 0.8f, -H * 0.6f}}, 4, false), 0.0f, -2.5f), false, WithA(X.SkinDeep, 0.35f), 1.3f, true);
		C.StrokePolyline(Lower, false, WithA(X.SkinDeep, 0.28f), 1.1f, true);
	}
	if (X.E)
	{
		// The expression's extras under the eye: a night's bags when the lids sag and the mouth falls, and the tear (the
		// character's right eye, on the left of the frame).
		if (X.E->EyeOpen < 0.75f && X.E->Smile < 0.0f)
		{
			C.StrokePolyline(Moved(Lower, 0.0f, 4.5f), false, WithA(Lum(X.SkinDeep, -0.1f), 0.45f), 2.2f, true);
			C.StrokePolyline(Moved(Lower, 0.0f, 8.0f), false, WithA(X.SkinDeep, 0.22f), 1.6f, true);
		}
		if (X.E->Tear && Side < 0.0f)
		{
			const float Tx = Cx + S * W * 0.35f;
			const Pts Drop = Spline({{Tx, 6.0f}, {Tx - 2.6f, 16.0f}, {Tx, 20.5f}, {Tx + 2.6f, 16.0f}}, 4, true);
			C.StrokePolyline({{Tx, 4.0f}, {Tx - 0.6f, 12.0f}}, false, WithA(Hex(0xbfe8ff), 0.55f), 2.0f, true);
			C.FillPolygon(Drop, Paint::Linear({Tx, 6.0f}, {Tx, 20.5f}, WithA(Hex(0xd8f3ff), 0.95f), WithA(Hex(0x6cc6f0), 0.95f)));
			C.FillCircle(Tx - 0.8f, 14.0f, 0.9f, WithA(Hex(0xffffff), 0.9f));
		}
	}
	if (X.Lod)
	{
		return;
	}
	// Age: crow's feet and bags.
	const float Crow = std::clamp((X.Age - 34.0f) / 26.0f, 0.0f, 1.0f) * 0.42f;
	if (Crow > 0.01f)
	{
		for (int I = 0; I < 3; ++I)
		{
			const float Dy = Fl(I - 1) * 4.0f;
			C.StrokePolyline({{Outer.X + S * 4.0f, Outer.Y + Dy * 0.6f}, {Outer.X + S * 10.0f, Outer.Y + Dy * 1.4f}}, false, WithA(X.SkinDeep, Crow), 1.0f, true);
		}
	}
	const float Bags = std::clamp((X.Age - 42.0f) / 22.0f, 0.0f, 1.0f) * 0.35f;
	if (Bags > 0.01f)
	{
		C.StrokePolyline(Moved(Lower, 0.0f, 4.0f), false, WithA(X.SkinDeep, Bags), 1.2f, true);
	}
}

void Brow(Ctx& X, float Side)
{
	const Geo& G = X.H.G;
	const int Kind = X.Who->Appearance.Brows;
	const float S = Side;
	const float Cx = S * G.EyeX;
	const float Thick = Kind == 0 ? 2.6f : Kind == 2 ? 6.4f : 4.4f;
	const float Arch = Kind == 3 ? 6.0f : 2.5f;
	const float Y0 = G.SoftFace ? -23.0f : -20.0f;
	Vec2 In{Cx - S * G.EyeW * 1.05f, Y0 - 1.0f};
	Vec2 Peak{Cx + S * G.EyeW * 0.35f, Y0 - 3.0f - Arch * 0.7f};
	Vec2 Out{Cx + S * G.EyeW * 1.2f, Y0 - 0.5f};
	if (X.E)
	{
		// The expression's brows: lifted, knit (inner ends down: anger; up: worry), one raised against the other.
		const float Knit = std::clamp(X.E->BrowKnit, -1.0f, 1.0f);
		const float Lift = -std::clamp(X.E->BrowLift, -1.0f, 1.5f) * 7.0f - S * std::clamp(X.E->BrowAsym, -1.0f, 1.0f) * 4.5f;
		In.Y += Lift + Knit * 6.5f;
		In.X -= S * Knit * 1.5f;
		Peak.Y += Lift + Knit * 1.2f;
		Out.Y += Lift - Knit * 1.8f;
		if (Knit > 0.35f && Side > 0.0f)
		{
			// The crease between knitted brows.
			for (int K = -1; K <= 1; K += 2)
			{
				const float Kx = Fl(K) * 4.5f;
				X.C->StrokePolyline({{Kx, Y0 + 1.0f}, {Kx * 0.8f, Y0 + 9.0f}}, false, WithA(X.SkinDeep, 0.45f * Knit), 1.4f, true);
			}
		}
	}
	const Pts Top = Spline({In, Lerp2(In, Peak, 0.5f), Peak, Out}, 4, false);
	Pts Shape = Moved(Top, 0.0f, -Thick * 0.5f);
	const Pts Bottom = Spline({Out, {Peak.X, Peak.Y + Thick * 0.85f}, {Lerp2(In, Peak, 0.5f).X, Lerp2(In, Peak, 0.5f).Y + Thick}, {In.X, In.Y + Thick * 0.9f}}, 4, false);
	for (const Vec2& P : Bottom)
	{
		Shape.push_back({P.X, P.Y - Thick * 0.5f});
	}
	X.C->FillPolygon(Shape, X.Brow);
	for (int I = 0; I < (X.Lod ? 0 : 6); ++I)
	{
		const Vec2 B = Lerp2(In, Out, Fl(I) / 6.0f + 0.04f);
		X.C->StrokePolyline({{B.X, B.Y - Thick * 0.1f}, {B.X + S * 3.5f, B.Y - Thick * 0.7f - (I < 3 ? Arch * 0.5f : 0.0f)}}, false, WithA(Lum(X.Brow, 0.2f), 0.4f), 0.9f, true);
	}
}

void Nose(Ctx& X)
{
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	const float Wd = G.Heavy > 0.0f ? 1.12f : G.SoftFace ? 0.9f : 1.0f;
	// Bridge: the shadow side.
	C.StrokePolyline(Spline({{5.0f, -6.0f}, {7.0f, 10.0f}, {9.0f * Wd, 26.0f}}, 4, false), false, WithA(X.SkinShade, 0.45f), 4.0f, true);
	Soft(C, -5.0f, 10.0f, 4.0f, 14.0f, X.SkinLight, 0.3f);
	// Tip, wings and nostrils.
	Soft(C, 0.0f, 33.0f, 11.0f * Wd, 8.0f, X.SkinShade, 0.3f);
	Soft(C, -2.5f, 31.0f, 5.0f, 4.0f, X.SkinLight, 0.55f);
	C.StrokeArc(-10.5f * Wd, 35.0f, 5.5f, 1.7f, 3.9f, WithA(X.SkinDeep, 0.45f), 1.5f, true);
	C.StrokeArc(10.5f * Wd, 35.0f, 5.5f, -0.75f, 1.45f, WithA(X.SkinDeep, 0.55f), 1.6f, true);
	C.FillEllipse(-5.5f * Wd, 39.5f, 3.2f, 1.7f, WithA(Lum(X.SkinDeep, -0.3f), 0.6f));
	C.FillEllipse(5.5f * Wd, 39.5f, 3.2f, 1.7f, WithA(Lum(X.SkinDeep, -0.3f), 0.7f));
	Soft(C, 0.0f, 44.0f, 10.0f, 3.0f, X.SkinShade, 0.35f);
	if (X.Lod)
	{
		return;
	}
	// Nasolabial folds, deeper with age.
	const float Fold = 0.12f + std::clamp((X.Age - 26.0f) / 30.0f, 0.0f, 1.0f) * 0.3f;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float S = Fl(Side);
		X.C->StrokePolyline(Spline({{S * 14.0f * Wd, 38.0f}, {S * 22.0f, 50.0f}, {S * 27.0f, 64.0f}}, 4, false), false, WithA(X.SkinDeep, Fold * (Side > 0 ? 1.2f : 0.8f)), 1.6f, true);
	}
}

/**
 * Update 3 (SPEC §6.1): the expression's mouth. The corners lift with a smile and fall with a frown, a smirk pulls one up,
 * the jaw drops it open (a dark mouth, the upper teeth, a tongue when it's wide); an O when it opens without a smile.
 */
void MouthX(Ctx& X)
{
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	const PortraitExpression& E = *X.E;
	const float Smile = std::clamp(E.Smile, -1.0f, 1.0f);
	const float Open = std::clamp(E.MouthOpen, 0.0f, 1.0f);
	const float Smirk = std::clamp(E.Smirk, -1.0f, 1.0f);
	const float My = 61.0f;
	const float Full = G.SoftFace ? 2.5f : 0.0f;
	const float Round = Smile < 0.25f ? Open : 0.0f; // an O: it opens without a smile and narrows
	const float Mw = (G.SoftFace ? 20.0f : 22.0f) * (1.0f + 0.2f * std::max(0.0f, Smile) - 0.38f * Round);
	const Color Lip = Mix(X.Skin, Hex(0xb04552), G.SoftFace ? 0.42f : 0.24f);
	const Color Line = WithA(Lum(X.SkinDeep, -0.4f), 0.8f);
	// The corners (the smirk lifts the frame's right one) and the middle of the lip line.
	const Vec2 L{-Mw, My - 7.0f * Smile + 1.5f * Smirk};
	const Vec2 R{Mw, My - 7.0f * Smile - 6.5f * Smirk};
	const float Mid = (L.Y + R.Y) * 0.5f + 5.0f * Smile + (Smile < 0.0f ? 2.0f * Smile : 0.0f);
	if (Open < 0.06f)
	{
		// Closed: the two lips along the curve, the line between them.
		const Pts Curve = Spline({L, {-Mw * 0.5f, (L.Y + Mid) * 0.5f + 1.0f}, {0.0f, Mid + 1.2f}, {Mw * 0.5f, (R.Y + Mid) * 0.5f + 1.0f}, R}, 4, false);
		Pts Upper = Spline({L, {-Mw * 0.5f, (L.Y + Mid) * 0.5f - 4.5f - Full * 0.4f}, {-3.0f, Mid - 4.6f - Full * 0.5f}, {0.0f, Mid - 3.4f}, {3.0f, Mid - 4.6f - Full * 0.5f},
								{Mw * 0.5f, (R.Y + Mid) * 0.5f - 4.5f - Full * 0.4f}, R},
			4, false);
		for (size_t I = Curve.size() - 1; I-- > 1;)
		{
			Upper.push_back(Curve[I]);
		}
		C.FillPolygon(Upper, Lum(Lip, -0.18f));
		Pts Lower = Curve;
		const Pts Under = Spline({R, {Mw * 0.55f, (R.Y + Mid) * 0.5f + 5.5f + Full}, {0.0f, Mid + 7.5f + Full}, {-Mw * 0.55f, (L.Y + Mid) * 0.5f + 5.5f + Full}, L}, 4, false);
		for (size_t I = 1; I + 1 < Under.size(); ++I)
		{
			Lower.push_back(Under[I]);
		}
		C.FillPolygon(Lower, Paint::Linear({-Mw, 0.0f}, {Mw, 0.0f}, Lum(Lip, 0.04f), Lum(Lip, -0.14f)));
		C.StrokePolyline(Curve, false, Line, 1.6f, true);
		if (std::fabs(Smirk) > 0.2f || Smile > 0.3f)
		{
			// The dimple at a lifted corner.
			const Vec2 D = Smirk > 0.2f || Smile > 0.3f ? R : L;
			C.StrokeArc(D.X + (D.X > 0.0f ? 2.0f : -2.0f), D.Y - 1.0f, 4.0f, D.X > 0.0f ? -1.2f : 1.9f, D.X > 0.0f ? 0.9f : 4.3f, WithA(X.SkinDeep, 0.45f), 1.3f, true);
		}
	}
	else
	{
		// Open: the top edge along the lip line, the bottom where the jaw dropped it.
		const float Drop = 4.0f + Open * 24.0f + std::max(0.0f, Smile) * 5.0f;
		const float TopMid = Mid - (Smile > 0.3f ? 1.0f : 3.0f + 3.0f * Round);
		const Pts Top = Spline({L, {-Mw * 0.55f, (L.Y + TopMid) * 0.5f - 1.5f}, {0.0f, TopMid}, {Mw * 0.55f, (R.Y + TopMid) * 0.5f - 1.5f}, R}, 5, false);
		const float BotMid = TopMid + Drop;
		const Pts Bot = Spline({R, {Mw * (0.62f - 0.12f * Round), (R.Y + BotMid) * 0.5f + Drop * 0.32f}, {0.0f, BotMid}, {-Mw * (0.62f - 0.12f * Round), (L.Y + BotMid) * 0.5f + Drop * 0.32f}, L}, 5, false);
		Pts Hole = Top;
		for (size_t I = 1; I + 1 < Bot.size(); ++I)
		{
			Hole.push_back(Bot[I]);
		}
		// The upper lip above the opening, the lower lip below it.
		Pts UpperLip = Top;
		for (size_t I = Top.size(); I-- > 0;)
		{
			UpperLip.push_back({Top[I].X, Top[I].Y - (3.6f + Full * 0.5f) * (1.0f - std::fabs(Top[I].X) / (Mw * 1.05f))});
		}
		C.FillPolygon(UpperLip, Lum(Lip, -0.16f));
		Pts LowerLip = Bot;
		for (size_t I = Bot.size(); I-- > 0;)
		{
			LowerLip.push_back({Bot[I].X, Bot[I].Y + (5.0f + Full) * (1.0f - std::fabs(Bot[I].X) / (Mw * 1.05f))});
		}
		C.FillPolygon(LowerLip, Paint::Linear({-Mw, 0.0f}, {Mw, 0.0f}, Lum(Lip, 0.06f), Lum(Lip, -0.12f)));
		C.FillPolygon(Hole, Paint::Linear({0.0f, TopMid}, {0.0f, BotMid}, Hex(0x2a0c10), Hex(0x4a1418)));
		if (Open > 0.45f)
		{
			// The tongue, low in a wide mouth.
			const Pts Tongue = ClipConvex(Ellipse(0.0f, BotMid - Drop * 0.12f, Mw * 0.55f, Drop * 0.32f, 18), Hole);
			if (!Tongue.empty())
			{
				C.FillPolygon(Tongue, Paint::Linear({0.0f, BotMid - Drop * 0.4f}, {0.0f, BotMid}, Hex(0xc85a64), Hex(0x8e3540)));
			}
		}
		if (E.Teeth)
		{
			// The upper teeth along the top edge (a grin shows them across, a scream at the middle).
			Pts Band = Top;
			const float Tooth = std::min(7.5f, Drop * 0.42f);
			for (size_t I = Top.size(); I-- > 0;)
			{
				const float K = 1.0f - std::pow(std::fabs(Top[I].X) / Mw, 3.0f);
				Band.push_back({Top[I].X * 0.97f, Top[I].Y + Tooth * K});
			}
			const Pts Teeth = Band;
			C.FillPolygon(Teeth, Paint::Linear({0.0f, TopMid}, {0.0f, TopMid + Tooth}, Hex(0xfbf7f0), Hex(0xd9d2c6)));
			if (!X.Lod)
			{
				for (int K = -2; K <= 2; ++K)
				{
					const float Tx = Fl(K) * Mw * 0.22f;
					C.StrokePolyline({{Tx, TopMid + 0.5f}, {Tx, TopMid + Tooth * 0.85f}}, false, WithA(Hex(0xb8ad9e), 0.6f), 0.8f, false);
				}
			}
		}
		C.StrokePolyline(Hole, true, Line, 1.4f, true);
	}
	// A smile lifts the cheeks; a hard frown pulls the chin.
	if (Smile > 0.35f)
	{
		Soft(C, -34.0f, 30.0f, 16.0f, 10.0f, X.SkinLight, 0.35f * Smile);
		Soft(C, 34.0f, 30.0f, 16.0f, 10.0f, X.SkinLight, 0.22f * Smile);
	}
	Soft(C, 0.0f, My + 15.0f + Open * 22.0f, 13.0f, 4.0f, X.SkinShade, 0.32f);
}

void Mouth(Ctx& X)
{
	if (X.E)
	{
		MouthX(X);
		return;
	}
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	const float Mw = G.SoftFace ? 20.0f : 22.0f;
	const float Full = G.SoftFace ? 2.5f : 0.0f;
	const float My = 61.0f;
	const Color Lip = Mix(X.Skin, Hex(0xb04552), G.SoftFace ? 0.42f : 0.24f);
	const Pts UpperLip = Spline({{-Mw, My}, {-Mw * 0.5f, My - 4.5f - Full * 0.4f}, {-3.0f, My - 5.6f - Full * 0.5f}, {0.0f, My - 4.2f}, {3.0f, My - 5.6f - Full * 0.5f}, {Mw * 0.5f, My - 4.5f - Full * 0.4f},
									{Mw, My}, {Mw * 0.5f, My + 0.6f}, {0.0f, My + 1.2f}, {-Mw * 0.5f, My + 0.6f}},
		4, true);
	C.FillPolygon(UpperLip, Lum(Lip, -0.18f));
	const Pts LowerLip = Spline({{Mw, My}, {Mw * 0.55f, My + 5.5f + Full}, {0.0f, My + 7.5f + Full}, {-Mw * 0.55f, My + 5.5f + Full}, {-Mw, My}, {-Mw * 0.5f, My + 0.8f}, {0.0f, My + 1.4f}, {Mw * 0.5f, My + 0.8f}}, 4,
		true);
	C.FillPolygon(LowerLip, Paint::Linear({-Mw, 0.0f}, {Mw, 0.0f}, Lum(Lip, 0.04f), Lum(Lip, -0.14f)));
	Soft(C, -5.0f, My + 4.0f, 6.0f, 2.0f, Hex(0xffffff), 0.22f);
	C.StrokePolyline(Spline({{-Mw - 1.0f, My - 0.4f}, {-Mw * 0.5f, My + 1.0f}, {0.0f, My + 1.4f}, {Mw * 0.5f, My + 1.0f}, {Mw + 1.0f, My - 0.4f}}, 4, false), false, WithA(Lum(X.SkinDeep, -0.4f), 0.75f), 1.5f, true);
	Soft(C, 0.0f, My + 14.0f, 13.0f, 4.0f, X.SkinShade, 0.32f);
}

void Wrinkles(Ctx& X)
{
	const float A = std::clamp((X.Age - 38.0f) / 25.0f, 0.0f, 1.0f) * 0.3f;
	if (A < 0.01f || X.Lod)
	{
		return;
	}
	for (int I = 0; I < 2; ++I)
	{
		const float Y = -52.0f - Fl(I) * 9.0f;
		X.C->StrokePolyline(Spline({{-34.0f, Y + 1.0f}, {-12.0f, Y - 1.5f}, {10.0f, Y + 0.5f}, {32.0f, Y - 1.0f}}, 4, false), false, WithA(X.SkinDeep, A * (I == 0 ? 1.0f : 0.7f)), 1.1f, true);
	}
}

// ------------------------------------------------------------------ facial hair

Pts Mustache(const Ctx& X)
{
	(void)X;
	return Spline({{-25.0f, 61.0f}, {-21.0f, 50.0f}, {-9.0f, 46.0f}, {0.0f, 48.0f}, {9.0f, 46.0f}, {21.0f, 50.0f}, {25.0f, 61.0f}, {18.0f, 56.5f}, {0.0f, 55.0f}, {-18.0f, 56.5f}}, 4, true);
}

/** The beard area: around the jaw from sideburn to sideburn, leaving the mouth. */
Pts BeardArea(const Ctx& X, float Out, float CheekLine)
{
	const Geo& G = X.H.G;
	Pts P;
	for (float A = 97.0f; A <= 263.0f; A += 5.0f)
	{
		const float Chin = std::max(0.0f, 1.0f - std::fabs(A - 180.0f) / 70.0f);
		P.push_back(X.H.Rim(A, 0.5f + Out * Chin));
	}
	const Pts Inner = Spline({{-G.CheekW + 10.0f, CheekLine}, {-G.CheekW + 20.0f, 36.0f}, {-30.0f, 56.0f}, {-24.0f, 70.0f}, {-10.0f, 74.0f}, {0.0f, 73.0f}, {10.0f, 74.0f}, {24.0f, 70.0f}, {30.0f, 56.0f},
								 {G.CheekW - 20.0f, 36.0f}, {G.CheekW - 10.0f, CheekLine}},
		4, false);
	for (const Vec2& Q : Inner)
	{
		P.push_back(Q);
	}
	return P;
}

void FacialHair(Ctx& X)
{
	const int Kind = X.Who->Appearance.FacialHair;
	if (Kind == 0)
	{
		return;
	}
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	const Paint Fill = Paint::Linear({-G.CheekW, 0.0f}, {G.CheekW, 0.0f}, Lum(X.Beard, 0.08f), Lum(X.Beard, -0.3f));
	switch (Kind)
	{
	case 1:
	{
		const Pts Area = BeardArea(X, 0.0f, 6.0f);
		C.FillPolygon(Area, WithA(Mix(X.Beard, X.SkinShade, 0.3f), 0.26f));
		C.FillPolygon(Mustache(X), WithA(Mix(X.Beard, X.SkinShade, 0.3f), 0.24f));
		Stipple(X, Area, X.Beard, 0.55f, 700, 101);
		Stipple(X, Mustache(X), X.Beard, 0.55f, 120, 151);
		break;
	}
	case 2:
		C.FillPolygon(Mustache(X), Fill);
		break;
	case 3:
	{
		C.FillPolygon(Mustache(X), Fill);
		const Pts Goatee = Spline({{-11.0f, 69.0f}, {-5.0f, 67.5f}, {5.0f, 67.5f}, {11.0f, 69.0f}, {G.ChinW * 0.55f + 3.0f, G.ChinY - 12.0f}, {0.0f, G.ChinY + 3.0f}, {-G.ChinW * 0.55f - 3.0f, G.ChinY - 12.0f}}, 4, true);
		C.FillPolygon(Goatee, Fill);
		Stipple(X, Goatee, Lum(X.Beard, 0.3f), 0.35f, 120, 211);
		C.FillPolygon(Spline({{-25.0f, 60.0f}, {-14.0f, 68.0f}, {-17.0f, 74.0f}, {-26.0f, 66.0f}}, 3, true), Fill);
		C.FillPolygon(Spline({{25.0f, 60.0f}, {14.0f, 68.0f}, {17.0f, 74.0f}, {26.0f, 66.0f}}, 3, true), Fill);
		break;
	}
	default:
	{
		const bool Full = Kind == 5;
		const Pts Area = BeardArea(X, Full ? 16.0f : 4.0f, Full ? 0.0f : 10.0f);
		C.FillPolygon(Area, Fill);
		C.FillPolygon(Mustache(X), Fill);
		for (int I = 0; I < (X.Lod ? 0 : Full ? 70 : 40); ++I)
		{
			const float A = 100.0f + 160.0f * Hash(I * 3 + 500);
			const Vec2 Q = X.H.Rim(A, -6.0f - 10.0f * Hash(I * 3 + 501));
			C.StrokePolyline({Q, {Q.X + (Q.X > 0 ? 1.5f : -1.5f), Q.Y + 5.0f}}, false, WithA(I % 2 ? Lum(X.Beard, 0.25f) : Lum(X.Beard, -0.35f), 0.4f), 1.1f, true);
		}
		// The rim catches the beard's edge.
		Pts Edge;
		for (float A = 100.0f; A <= 150.0f; A += 5.0f)
		{
			Edge.push_back(X.H.Rim(A, (Full ? 16.0f * std::max(0.0f, 1.0f - std::fabs(A - 180.0f) / 70.0f) : 4.0f * std::max(0.0f, 1.0f - std::fabs(A - 180.0f) / 70.0f)) - 0.5f));
		}
		C.StrokePolyline(Edge, false, WithA(X.Light.Rim, 0.45f * X.Light.RimStrength), 2.0f, true);
		break;
	}
	}
}

// ------------------------------------------------------------------ clothes

/** The torso's right half, as a convex outline (neckline to the bottom of the frame). */
Pts TorsoRight(const Ctx& X)
{
	const Geo& G = X.H.G;
	return {{0.0f, 160.0f}, {G.NeckW + 3.0f, 126.0f}, {G.NeckW + 30.0f, 140.0f}, {G.ShoulderW * 0.62f, 158.0f}, {G.ShoulderW * 0.9f, 180.0f}, {G.ShoulderW, 212.0f}, {G.ShoulderW + 8.0f, 260.0f},
		{G.ShoulderW + 12.0f, 360.0f}, {0.0f, 360.0f}};
}

Pts TorsoOutline(const Ctx& X)
{
	return Spline(Mirror(TorsoRight(X)), 4, true);
}

/** One side's panel of an open jacket: the torso half less the opening (Gap at the neck, Low at the bottom). */
Pts Panel(const Ctx& X, float Side, float Gap, float Low)
{
	Pts R = Spline(TorsoRight(X), 4, false);
	R.erase(R.begin());
	R.pop_back();
	Pts P;
	P.push_back({Gap, 152.0f});
	for (const Vec2& Q : R)
	{
		P.push_back(Q);
	}
	P.push_back({X.H.G.ShoulderW + 12.0f, 360.0f});
	P.push_back({Low, 360.0f});
	return Side < 0.0f ? Flip(P) : P;
}

void Shirt(Ctx& X)
{
	const Geo& G = X.H.G;
	Canvas& C = *X.C;
	C.FillPolygon(TorsoOutline(X), Paint::Linear({-G.ShoulderW, 0.0f}, {G.ShoulderW, 0.0f}, Lum(X.Tee, 0.05f), Lum(X.Tee, -0.35f)));
	C.StrokePolyline(Spline({{-G.NeckW * 0.95f, 132.0f}, {0.0f, 158.0f}, {G.NeckW * 0.95f, 132.0f}}, 6, false), false, Lum(X.Tee, -0.25f), 4.0f, true);
}

void ClothShade(Ctx& X, const Pts& P)
{
	const Geo& G = X.H.G;
	X.C->FillPolygon(P, Paint::Linear({-G.ShoulderW, 140.0f}, {G.ShoulderW, 200.0f}, X.ClothLight, X.ClothDark));
}

void Outfit(Ctx& X)
{
	const Geo& G = X.H.G;
	Canvas& C = *X.C;
	const int Kind = X.Who->Appearance.Outfit;
	const Pts Torso = TorsoOutline(X);
	const Color Seam = WithA(Lum(X.Cloth, -0.5f), 0.55f);
	switch (Kind)
	{
	case 0: // Hoodie
	{
		ClothShade(X, Torso);
		// The neck opening's ribbed edge and the drawstrings.
		const Pts Edge = Spline({{-G.NeckW - 6.0f, 128.0f}, {-G.NeckW * 0.5f, 150.0f}, {0.0f, 160.0f}, {G.NeckW * 0.5f, 150.0f}, {G.NeckW + 6.0f, 128.0f}}, 6, false);
		C.StrokePolyline(Edge, false, Lum(X.Cloth, -0.3f), 9.0f, true);
		C.StrokePolyline(Moved(Edge, 0.0f, -3.0f), false, WithA(X.ClothLight, 0.35f), 1.5f, true);
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			const Pts Cord = Spline({{S * 11.0f, 157.0f}, {S * 13.0f, 200.0f}, {S * (16.0f + S * 1.0f), 246.0f}}, 5, false);
			C.StrokePolyline(Cord, false, Hex(0xe9e4da), 2.6f, true);
			C.FillRoundRect({S * 16.0f - 2.0f, 244.0f, 4.0f, 11.0f}, 1.5f, Hex(0xb8b2a6));
		}
		C.StrokePolyline(Spline({{G.NeckW + 26.0f, 140.0f}, {G.ShoulderW * 0.7f, 220.0f}, {G.ShoulderW * 0.86f, 360.0f}}, 5, false), false, Seam, 1.4f, true);
		C.StrokePolyline(Spline({{-G.NeckW - 26.0f, 140.0f}, {-G.ShoulderW * 0.7f, 220.0f}, {-G.ShoulderW * 0.86f, 360.0f}}, 5, false), false, Seam, 1.4f, true);
		break;
	}
	case 1: // Bomber
	{
		ClothShade(X, Torso);
		Soft(C, -G.ShoulderW * 0.55f, 190.0f, 50.0f, 22.0f, Hex(0xffffff), 0.12f);
		// Ribbed collar, the zip, the sleeve pocket.
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			const Pts Band = Spline({{S * 4.0f, 172.0f}, {S * (G.NeckW * 0.7f), 150.0f}, {S * (G.NeckW + 4.0f), 122.0f}}, 5, false);
			C.StrokePolyline(Band, false, Lum(X.Cloth, -0.4f), 12.0f, true);
			for (size_t I = 0; I < Band.size(); I += 2)
			{
				C.StrokePolyline({{Band[I].X - 4.0f, Band[I].Y - 3.0f}, {Band[I].X + 4.0f, Band[I].Y + 3.0f}}, false, WithA(Lum(X.Cloth, -0.6f), 0.6f), 1.0f);
			}
		}
		C.StrokePolyline({{0.0f, 172.0f}, {0.0f, 360.0f}}, false, Hex(0x8c8f96), 2.2f);
		C.FillRoundRect({-3.0f, 186.0f, 6.0f, 14.0f}, 2.0f, Hex(0xb9bcc3));
		C.FillRoundRect({-G.ShoulderW * 0.86f, 238.0f, 26.0f, 30.0f}, 3.0f, WithA(Lum(X.Cloth, -0.25f), 0.8f));
		C.StrokePolyline({{-G.ShoulderW * 0.86f + 4.0f, 244.0f}, {-G.ShoulderW * 0.86f + 22.0f, 244.0f}}, false, Hex(0x9a9da4), 1.6f);
		break;
	}
	case 2: // Flannel
	{
		ClothShade(X, Torso);
		const Pts Half = TorsoRight(X);
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const Pts Clip = Side < 0 ? Flip(Half) : Half;
			for (float Gx = -260.0f; Gx < 260.0f; Gx += 30.0f)
			{
				const Pts Band = ClipConvex({{Gx, 100.0f}, {Gx + 11.0f, 100.0f}, {Gx + 11.0f, 380.0f}, {Gx, 380.0f}}, Clip);
				if (!Band.empty())
				{
					C.FillPolygon(Band, WithA(Lum(X.Cloth, -0.55f), 0.38f));
				}
				const Pts Thin = ClipConvex({{Gx + 18.0f, 100.0f}, {Gx + 20.0f, 100.0f}, {Gx + 20.0f, 380.0f}, {Gx + 18.0f, 380.0f}}, Clip);
				if (!Thin.empty() && !X.E)
				{
					C.FillPolygon(Thin, WithA(Hex(0xf3ead8), 0.22f));
				}
			}
			for (float Gy = 120.0f; Gy < 380.0f; Gy += 30.0f)
			{
				const Pts Band = ClipConvex({{-260.0f, Gy}, {260.0f, Gy}, {260.0f, Gy + 11.0f}, {-260.0f, Gy + 11.0f}}, Clip);
				if (!Band.empty())
				{
					C.FillPolygon(Band, WithA(Lum(X.Cloth, -0.55f), 0.32f));
				}
				const Pts Thin = ClipConvex({{-260.0f, Gy + 18.0f}, {260.0f, Gy + 18.0f}, {260.0f, Gy + 20.0f}, {-260.0f, Gy + 20.0f}}, Clip);
				if (!Thin.empty() && !X.E)
				{
					C.FillPolygon(Thin, WithA(Hex(0xf3ead8), 0.18f));
				}
			}
		}
		C.FillPolygon(Torso, Paint::Linear({-G.ShoulderW, 0.0f}, {G.ShoulderW, 0.0f}, WithA(Hex(0xffffff), 0.06f), WithA(Hex(0x000000), 0.35f)));
		// Placket, buttons and the collar.
		C.FillRect({-5.0f, 166.0f, 10.0f, 200.0f}, WithA(Lum(X.Cloth, -0.2f), 0.6f));
		for (float By = 196.0f; By < 360.0f; By += 40.0f)
		{
			C.FillCircle(0.0f, By, 3.0f, Hex(0xe8e0cc));
		}
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			const Pts Collar = {{S * 2.0f, 160.0f}, {S * (G.NeckW + 1.0f), 120.0f}, {S * (G.NeckW + 13.0f), 134.0f}, {S * 26.0f, 190.0f}};
			C.FillPolygon(Collar, Paint::Linear({0.0f, 120.0f}, {0.0f, 190.0f}, Lum(X.Cloth, 0.08f), Lum(X.Cloth, -0.35f)));
			C.StrokePolyline(Collar, true, WithA(Lum(X.Cloth, -0.6f), 0.5f), 1.2f);
		}
		break;
	}
	case 3: // Denim jacket, open over a tee
	case 4: // Leather jacket, open over a tee
	{
		Shirt(X);
		const bool Leather = Kind == 4;
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			const Pts P = Panel(X, S, Leather ? 22.0f : 17.0f, Leather ? 46.0f : 36.0f);
			ClothShade(X, P);
			if (Leather)
			{
				// Lapels, studs and the shine of the hide.
				const Pts Lapel = {{S * 22.0f, 152.0f}, {S * (G.NeckW + 4.0f), 122.0f}, {S * (G.NeckW + 30.0f), 150.0f}, {S * 62.0f, 214.0f}, {S * 36.0f, 250.0f}};
				C.FillPolygon(Lapel, Paint::Linear({0.0f, 120.0f}, {0.0f, 250.0f}, Lum(X.Cloth, 0.12f), Lum(X.Cloth, -0.25f)));
				C.StrokePolyline(Lapel, true, WithA(Lum(X.Cloth, -0.6f), 0.7f), 1.4f);
				C.FillCircle(S * 52.0f, 160.0f, 2.4f, Hex(0xc8cbd1));
				C.FillCircle(S * 58.0f, 206.0f, 2.4f, Hex(0xc8cbd1));
				Soft(C, S * G.ShoulderW * 0.7f, 186.0f, 34.0f, 12.0f, Hex(0xffffff), Side < 0 ? 0.22f : 0.08f);
				Soft(C, S * 90.0f, 280.0f, 14.0f, 40.0f, Hex(0xffffff), Side < 0 ? 0.14f : 0.05f);
				C.StrokePolyline({{S * 46.0f, 250.0f}, {S * 48.0f, 360.0f}}, false, Hex(0xa3a6ad), 1.6f);
			}
			else
			{
				// Denim: the turned-down collar, chest pockets and copper stitching.
				const Pts Collar = {{S * 17.0f, 154.0f}, {S * (G.NeckW + 2.0f), 120.0f}, {S * (G.NeckW + 24.0f), 138.0f}, {S * 40.0f, 196.0f}};
				C.FillPolygon(Collar, Paint::Linear({0.0f, 120.0f}, {0.0f, 196.0f}, Lum(X.Cloth, 0.1f), Lum(X.Cloth, -0.3f)));
				const Color Stitch = WithA(Hex(0xd9953b), 0.75f);
				C.StrokePolyline({{S * 44.0f, 228.0f}, {S * 92.0f, 228.0f}, {S * 92.0f, 268.0f}, {S * 44.0f, 268.0f}}, true, Stitch, 1.2f);
				C.FillPolygon({{S * 42.0f, 222.0f}, {S * 94.0f, 222.0f}, {S * 94.0f, 238.0f}, {S * 68.0f, 244.0f}, {S * 42.0f, 238.0f}}, Lum(X.Cloth, -0.12f));
				C.FillCircle(S * 68.0f, 236.0f, 2.6f, Hex(0xb07a3a));
				C.StrokePolyline(Spline({{S * 20.0f, 206.0f}, {S * 80.0f, 200.0f}, {S * (G.ShoulderW - 12.0f), 214.0f}}, 4, false), false, Stitch, 1.1f);
				for (float By = 200.0f; By < 360.0f; By += 44.0f)
				{
					C.FillCircle(S * 24.0f, By, 3.0f, Hex(0xb07a3a));
				}
			}
		}
		break;
	}
	case hero::OutfitNone: // No jacket: the tee alone, in the outfit color
	{
		C.FillPolygon(Torso, Paint::Linear({-G.ShoulderW, 140.0f}, {G.ShoulderW, 220.0f}, Lum(X.Tee, 0.1f), Lum(X.Tee, -0.38f)));
		// The crew neck's rib, a little stretched, and the shoulder seams where the sleeves start.
		const Pts Crew = Spline({{-G.NeckW * 1.04f, 126.0f}, {-G.NeckW * 0.55f, 150.0f}, {0.0f, 158.0f}, {G.NeckW * 0.55f, 150.0f}, {G.NeckW * 1.04f, 126.0f}}, 6, false);
		C.StrokePolyline(Crew, false, Lum(X.Tee, -0.28f), 7.0f, true);
		C.StrokePolyline(Moved(Crew, 0.0f, 4.5f), false, WithA(Lum(X.Tee, 0.25f), 0.3f), 1.4f, true);
		const Color TeeSeam = WithA(Lum(X.Tee, -0.5f), 0.5f);
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			C.StrokePolyline(Spline({{S * (G.ShoulderW * 0.74f), 150.0f}, {S * (G.ShoulderW * 0.82f), 220.0f}, {S * (G.ShoulderW * 0.86f), 300.0f}}, 4, false), false, TeeSeam, 1.3f, true);
		}
		// The cotton's soft folds.
		Soft(C, -G.ShoulderW * 0.32f, 262.0f, 34.0f, 70.0f, Hex(0xffffff), 0.07f);
		Soft(C, G.ShoulderW * 0.36f, 300.0f, 26.0f, 60.0f, Hex(0x000000), 0.12f);
		break;
	}
	default: // Track jacket
	{
		ClothShade(X, Torso);
		const Color Stripe = Luma(X.Cloth) > 0.55f ? Hex(0x15161a) : Hex(0xf1eee8);
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float S = Fl(Side);
			for (int K = 0; K < 2; ++K)
			{
				const float O = Fl(K) * 11.0f;
				C.StrokePolyline(Spline({{S * (G.NeckW + 30.0f + O * 0.4f), 142.0f + O}, {S * (G.ShoulderW * 0.86f + O * 0.2f), 180.0f + O}, {S * (G.ShoulderW + 2.0f - O * 0.3f), 230.0f},
										 {S * (G.ShoulderW + 8.0f - O * 0.5f), 360.0f}},
								 5, false),
					false, Stripe, 4.0f, true);
			}
			C.StrokePolyline(Spline({{S * 3.0f, 210.0f}, {S * 6.0f, 170.0f}, {S * (G.NeckW - 2.0f), 130.0f}, {S * (G.NeckW + 4.0f), 112.0f}}, 4, false), false, Lum(X.Cloth, -0.2f), 10.0f, true);
		}
		C.StrokePolyline({{0.0f, 210.0f}, {0.0f, 360.0f}}, false, Hex(0x9a9da4), 2.0f);
		C.FillPolygon({{-6.0f, 166.0f}, {6.0f, 166.0f}, {0.0f, 210.0f}}, X.Tee);
		break;
	}
	}
	// The rim on the right shoulder.
	Pts Edge = Spline(TorsoRight(X), 4, false);
	Pts Shoulder;
	for (const Vec2& P : Edge)
	{
		if (P.X > G.NeckW + 20.0f && P.Y < 350.0f)
		{
			Shoulder.push_back(P);
		}
	}
	C.StrokePolyline(Shoulder, false, WithA(X.Light.Rim, 0.6f * X.Light.RimStrength), 2.4f, true);
}

// ------------------------------------------------------------------ glasses and hats

void Glasses(Ctx& X)
{
	const int Kind = X.Who->Appearance.Glasses;
	if (Kind == 0)
	{
		return;
	}
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	const float Ex = G.EyeX;
	const Color Frame = Kind == 1 ? Hex(0x3a2418) : Kind == 2 ? Hex(0x141416) : Hex(0xc9a24d);
	const float Wd = Kind == 3 ? 1.5f : Kind == 4 ? 2.0f : 3.2f;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float S = Fl(Side);
		const float Cx = S * Ex;
		Pts Lens;
		if (Kind == 1)
		{
			Lens = Ellipse(Cx, 2.0f, 17.0f, 16.0f, 32);
		}
		else if (Kind == 2)
		{
			Lens = Canvas::RoundRectPath({Cx - 21.0f, -12.0f, 42.0f, 29.0f}, 6.0f, 4);
		}
		else if (Kind == 3)
		{
			Lens = Ellipse(Cx, 2.0f, 19.0f, 14.0f, 32);
		}
		else
		{
			Lens = Spline({{Cx - S * 20.0f, -10.0f}, {Cx + S * 4.0f, -12.0f}, {Cx + S * 21.0f, -9.0f}, {Cx + S * 18.0f, 12.0f}, {Cx + S * 2.0f, 21.0f}, {Cx - S * 16.0f, 12.0f}}, 5, true);
		}
		if (Kind == 4)
		{
			C.FillPolygon(Lens, Paint::Linear({0.0f, -12.0f}, {0.0f, 21.0f}, WithA(Hex(0x0d1116), 0.95f), WithA(Hex(0x26323c), 0.92f)));
			C.FillPolygon(ClipConvex({{Cx - 30.0f, 4.0f}, {Cx + 6.0f, -14.0f}, {Cx + 14.0f, -14.0f}, {Cx - 22.0f, 8.0f}}, Lens), WithA(X.Light.Rim, 0.4f));
		}
		else
		{
			C.FillPolygon(Lens, WithA(Hex(0xdfeef7), 0.08f));
			C.FillPolygon(ClipConvex({{Cx - 26.0f, 2.0f}, {Cx + 2.0f, -14.0f}, {Cx + 8.0f, -14.0f}, {Cx - 20.0f, 6.0f}}, Lens), WithA(Hex(0xffffff), 0.16f));
		}
		C.StrokePolyline(Lens, true, Frame, Wd);
		// The arm back to the ear.
		C.StrokePolyline({{Cx + S * (Kind == 1 ? 17.0f : 20.0f), -4.0f}, {S * (G.CheekW - 1.0f), -6.0f}}, false, Frame, Wd);
	}
	C.StrokePolyline(Spline({{-Ex + 16.0f, -1.0f}, {0.0f, -5.0f}, {Ex - 16.0f, -1.0f}}, 4, false), false, Frame, Wd);
}

/** How far each hairstyle stands off the skull at its fullest (FrontHair's cap, BackHair's panel): a hat sits on that. */
float HairOuter(int Style)
{
	static const float Outer[12] = {0.4f, 2.2f, 7.5f, 16.0f, 17.0f, 16.0f, 23.0f, 6.0f, 13.0f, 8.0f, 13.0f, 14.0f};
	return Outer[std::clamp(Style, 0, 11)];
}

/** A half-plane as a big convex quad: the side of the line through A and B that Keep is on. */
Pts HalfPlane(Vec2 A, Vec2 B, Vec2 Keep)
{
	const float Len = std::max(1e-4f, std::sqrt((B.X - A.X) * (B.X - A.X) + (B.Y - A.Y) * (B.Y - A.Y)));
	const Vec2 D{(B.X - A.X) / Len, (B.Y - A.Y) / Len};
	Vec2 N{-D.Y, D.X};
	if ((Keep.X - A.X) * N.X + (Keep.Y - A.Y) * N.Y < 0.0f)
	{
		N = {-N.X, -N.Y};
	}
	const float Far = 1000.0f;
	const Vec2 P0{A.X - D.X * Far, A.Y - D.Y * Far};
	const Vec2 P1{A.X + D.X * Far, A.Y + D.Y * Far};
	return {P0, P1, {P1.X + N.X * Far, P1.Y + N.Y * Far}, {P0.X + N.X * Far, P0.Y + N.Y * Far}};
}

void Hat(Ctx& X)
{
	const int Kind = X.Who->Appearance.Hat;
	if (Kind == 0)
	{
		return;
	}
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	// The hat goes with the jacket without matching it, and stands apart from the hair under it (hero::HatTone; the 3D
	// character wears the same).
	const Color HatTone = Hex(hero::HatTone(X.Who->Appearance, X.Who->Age));
	const Paint Fill = Paint::Linear({-G.TempleW - 20.0f, 0.0f}, {G.TempleW + 20.0f, 0.0f}, Lum(HatTone, 0.12f), Lum(HatTone, -0.45f));
	// It sits on the hair, not through it: every crown clears the style's fullest point.
	const float Hair = HairOuter(HairStyle(X)) + 1.5f;
	switch (Kind)
	{
	case 1: // Beanie
	{
		auto Thick = [Hair](float A) { return std::max(13.0f + 6.0f * std::cos(A * Deg), Hair + 2.0f * std::cos(A * Deg)); };
		const Pts Outer = X.H.Arc(-100.0f, 100.0f, Thick, 4.0f);
		const float Mx = G.TempleW * 0.62f;
		Pts Dome = Outer;
		Dome.push_back({Mx, -54.0f});
		Dome.push_back({0.0f, -56.0f});
		Dome.push_back({-Mx, -54.0f});
		C.FillPolygon(Dome, Fill);
		const Pts Cuff = ClipConvex(Dome, {{-300.0f, -82.0f}, {300.0f, -82.0f}, {300.0f, 0.0f}, {-300.0f, 0.0f}});
		if (!Cuff.empty())
		{
			C.FillPolygon(Cuff, Paint::Linear({-G.TempleW - 20.0f, 0.0f}, {G.TempleW + 20.0f, 0.0f}, Lum(HatTone, 0.2f), Lum(HatTone, -0.4f)));
			// The knit ribs. The cuff bends round the brow, so they're cut to its convex pieces (a concave window cuts
			// them away entirely): the band over the forehead, and a leg down each temple.
			std::vector<Pts> Pieces;
			Pieces.push_back(ClipConvex(Outer, {{-Mx, -82.0f}, {Mx, -82.0f}, {Mx, -56.0f}, {-Mx, -56.0f}}));
			for (int Side = -1; Side <= 1; Side += 2)
			{
				const float S = Fl(Side);
				const Vec2 Foot = Side > 0 ? Outer.back() : Outer.front();
				const Pts Leg = ClipConvex(Outer, {{S * Mx, -82.0f}, {S * 400.0f, -82.0f}, {S * 400.0f, 0.0f}, {S * Mx, 0.0f}});
				Pieces.push_back(ClipConvex(Leg, HalfPlane(Foot, {S * Mx, -54.0f}, {S * (G.TempleW + 8.0f), -76.0f})));
			}
			for (const Pts& Piece : Pieces)
			{
				if (Piece.empty() || X.Lod)
				{
					continue;
				}
				for (float Rx = -G.TempleW - 34.0f; Rx < G.TempleW + 34.0f; Rx += 6.0f)
				{
					const Pts Rib = ClipConvex({{Rx, -84.0f}, {Rx + 1.6f, -84.0f}, {Rx + 1.6f, 2.0f}, {Rx, 2.0f}}, Piece);
					if (!Rib.empty())
					{
						C.FillPolygon(Rib, WithA(Lum(HatTone, -0.5f), 0.45f));
					}
				}
			}
			C.StrokePolyline(Spline({{-G.TempleW - 14.0f, -82.0f}, {0.0f, -84.0f}, {G.TempleW + 14.0f, -82.0f}}, 4, false), false, WithA(Lum(HatTone, -0.6f), 0.6f), 1.6f, true);
		}
		C.StrokePolyline(X.H.Arc(20.0f, 96.0f, [&Thick](float A) { return Thick(A) - 0.6f; }, 4.0f), false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.2f, true);
		X.HatTop = -56.0f;
		break;
	}
	case 2: // Ball cap, brim forward
	case 3: // Ball cap, backwards
	{
		const bool Back = Kind == 3;
		auto Thick = [Hair](float A) { return std::max(7.0f + 4.0f * std::cos(A * Deg), Hair); };
		Pts Crown = X.H.Arc(-74.0f, 74.0f, Thick, 4.0f);
		Crown.push_back({G.TempleW * 0.7f, -60.0f});
		Crown.push_back({0.0f, -66.0f});
		Crown.push_back({-G.TempleW * 0.7f, -60.0f});
		C.FillPolygon(Crown, Fill);
		const Vec2 Top = X.H.Rim(0.0f, Thick(0.0f));
		C.StrokePolyline(Spline({Top, {0.0f, -92.0f}, {0.0f, -66.0f}}, 3, false), false, WithA(Lum(HatTone, -0.5f), 0.6f), 1.3f);
		C.FillEllipse(Top.X, Top.Y + 2.0f, 5.0f, 2.6f, Lum(HatTone, -0.2f));
		if (Back)
		{
			// The strap opening at the front.
			C.FillPolygon(Spline({{-15.0f, -64.0f}, {-13.0f, -80.0f}, {0.0f, -86.0f}, {13.0f, -80.0f}, {15.0f, -64.0f}}, 4, false), X.HairDark);
			C.FillRoundRect({-16.0f, -70.0f, 32.0f, 5.0f}, 2.0f, Lum(HatTone, -0.35f));
			C.FillRect({-5.0f, -71.0f, 10.0f, 7.0f}, Hex(0x9a9da4));
			X.HatTop = -64.0f;
		}
		else
		{
			// A chip on the front panel, then the brim and its shadow on the brow.
			C.FillCircle(0.0f, -84.0f, 9.0f, Lum(HatTone, 0.5f));
			C.FillCircle(0.0f, -84.0f, 5.5f, Lum(HatTone, -0.1f));
			for (int I = 0; I < 6; ++I)
			{
				const float A = Fl(I) * 3.1415926f / 3.0f;
				C.FillCircle(7.3f * std::cos(A), -84.0f + 7.3f * std::sin(A), 1.4f, Lum(HatTone, -0.1f));
			}
			const Pts Shade = ClipConvex({{-200.0f, -66.0f}, {200.0f, -66.0f}, {200.0f, -8.0f}, {-200.0f, -8.0f}}, X.H.Outline);
			if (!Shade.empty())
			{
				C.FillPolygon(Shade, Paint::Linear({0.0f, -66.0f}, {0.0f, -8.0f}, WithA(X.SkinDeep, 0.55f), WithA(X.SkinDeep, 0.0f)));
			}
			const float Bw = G.TempleW + 22.0f;
			const Pts Brim = Spline({{-Bw, -62.0f}, {-Bw * 0.6f, -64.0f}, {0.0f, -65.0f}, {Bw * 0.6f, -64.0f}, {Bw, -62.0f}, {Bw * 0.8f, -50.0f}, {0.0f, -40.0f}, {-Bw * 0.8f, -50.0f}}, 4, true);
			C.FillPolygon(Brim, Paint::Linear({0.0f, -66.0f}, {0.0f, -40.0f}, Lum(HatTone, 0.18f), Lum(HatTone, -0.5f)));
			C.StrokePolyline(Spline({{-Bw * 0.8f, -52.0f}, {0.0f, -43.0f}, {Bw * 0.8f, -52.0f}}, 4, false), false, WithA(Lum(HatTone, -0.65f), 0.7f), 1.2f, true);
			X.HatTop = -40.0f;
		}
		C.StrokePolyline(X.H.Arc(20.0f, 72.0f, [&Thick](float A) { return Thick(A) - 0.6f; }, 4.0f), false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.2f, true);
		break;
	}
	default: // Bucket hat
	{
		auto Thick = [Hair](float A) { return std::max(12.0f + 2.0f * std::cos(A * Deg), Hair); };
		Pts Crown = X.H.Arc(-94.0f, 94.0f, Thick, 4.0f);
		Crown.push_back({0.0f, -58.0f});
		C.FillPolygon(Crown, Fill);
		const Pts Shade = ClipConvex({{-200.0f, -60.0f}, {200.0f, -60.0f}, {200.0f, -4.0f}, {-200.0f, -4.0f}}, X.H.Outline);
		if (!Shade.empty())
		{
			C.FillPolygon(Shade, Paint::Linear({0.0f, -60.0f}, {0.0f, -4.0f}, WithA(X.SkinDeep, 0.5f), WithA(X.SkinDeep, 0.0f)));
		}
		const float Bw = G.TempleW + 34.0f;
		const Pts Brim = Spline({{-G.TempleW - 8.0f, -66.0f}, {0.0f, -64.0f}, {G.TempleW + 8.0f, -66.0f}, {Bw, -30.0f}, {Bw * 0.5f, -36.0f}, {0.0f, -38.0f}, {-Bw * 0.5f, -36.0f}, {-Bw, -30.0f}}, 4, true);
		C.FillPolygon(Brim, Paint::Linear({0.0f, -66.0f}, {0.0f, -30.0f}, Lum(HatTone, 0.1f), Lum(HatTone, -0.45f)));
		for (int I = 1; I <= 3; ++I)
		{
			const float T = Fl(I) / 4.0f;
			C.StrokePolyline(Spline({{-(G.TempleW + 8.0f) - (Bw - G.TempleW - 8.0f) * T, -66.0f + 36.0f * T}, {0.0f, -64.0f + 26.0f * T}, {(G.TempleW + 8.0f) + (Bw - G.TempleW - 8.0f) * T, -66.0f + 36.0f * T}}, 5,
								 false),
				false, WithA(Lum(HatTone, -0.55f), 0.5f), 1.0f, true);
		}
		C.StrokePolyline(X.H.Arc(20.0f, 92.0f, [&Thick](float A) { return Thick(A) - 0.6f; }, 4.0f), false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.2f, true);
		X.HatTop = -36.0f;
		break;
	}
	}
}
} // namespace portrait_detail

void DrawPortrait(Canvas& C, const hero::Character& Who, float X, float Y, float Scale, double Time, const PortraitLight& Light)
{
	using namespace portrait_detail;
	const hero::Look& L = Who.Appearance;
	Ctx Cx;
	Cx.C = &C;
	Cx.Who = &Who;
	Cx.Light = Light;
	Cx.Age = Fl(Who.Age);
	Cx.H.G = GeoOf(L, Who.Age);
	Cx.H.Build();

	// Palette: the skin under a warm key light, the hair greying with age, the jacket.
	Cx.Skin = Hex(hero::SkinTone(L.Skin));
	Cx.SkinLight = Mix(Mix(Cx.Skin, Light.Key, 0.35f), Hex(0xffffff), 0.12f);
	Cx.SkinShade = Mix(Cx.Skin, Hex(0x3a1d22), 0.36f);
	Cx.SkinDeep = Mix(Cx.Skin, Hex(0x2a1214), 0.62f);
	Cx.Blush = Mix(Cx.Skin, Hex(0xd8545c), 0.45f);
	// The hair as it shows at this age (hero::HairToneAt greys it the same way for the hat's color); greying hair
	// catches a cooler light.
	const Color HairBase = Hex(hero::HairToneAt(L, Who.Age));
	const float Grey = L.HairColor >= 6 ? 0.0f : std::clamp((Cx.Age - 44.0f) / 30.0f, 0.0f, 0.65f);
	Cx.Hair = HairBase;
	Cx.HairLight = Mix(HairBase, Mix(Hex(0xfff4e6), Hex(0xf2f5fa), Grey / 0.65f), Luma(HairBase) > 0.5f ? 0.25f : 0.2f);
	Cx.HairDark = Mix(HairBase, Hex(0x050404), 0.45f);
	Cx.Brow = L.HairColor >= 5 ? Mix(HairBase, Hex(0x6a5848), 0.45f) : Mix(HairBase, Hex(0x000000), 0.25f);
	Cx.Beard = Mix(HairBase, Hex(0x000000), 0.1f);
	Cx.Cloth = Hex(hero::OutfitTone(L.OutfitColor));
	Cx.ClothLight = Mix(Mix(Cx.Cloth, Light.Key, 0.12f), Hex(0xffffff), 0.06f);
	Cx.ClothDark = Mix(Cx.Cloth, Hex(0x000000), 0.5f);
	// Under a jacket the tee is white or charcoal against it; with no jacket (OutfitNone) it's the tee alone, in the
	// outfit color.
	Cx.Tee = L.Outfit == hero::OutfitNone ? Cx.Cloth : Luma(Cx.Cloth) < 0.33f ? Hex(0xe7e3dc) : Hex(0x26282d);

	// Alive: breathing, a blink every few seconds, the eyes settling.
	const float T = static_cast<float>(Time);
	Cx.Breath = Time > 0.0 ? std::sin(T * 1.6f) : 0.0f;
	const float Phase = std::fmod(T + 0.9f, 4.3f);
	Cx.EyeOpen = Time > 0.0 && Phase < 0.16f ? 1.0f - std::sin(Phase / 0.16f * 3.1415926f) : 1.0f;
	Cx.LookX = Time > 0.0 ? 0.9f * std::sin(T * 0.37f) : 0.0f;

	C.Save();
	C.Translate(X, Y);
	C.Scale(Scale, Scale);
	// The body breathes; the head rides a little less.
	C.Save();
	C.Translate(0.0f, Cx.Breath * 1.4f);
	if (L.Outfit == 0)
	{
		HoodBack(Cx);
	}
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 0.6f);
	BackHair(Cx);
	Neck(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 1.4f);
	Outfit(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 0.6f);
	Ear(Cx, -1.0f);
	Ear(Cx, 1.0f);
	HeadFill(Cx);
	Wrinkles(Cx);
	Nose(Cx);
	Mouth(Cx);
	Eye(Cx, -1.0f);
	Eye(Cx, 1.0f);
	Brow(Cx, -1.0f);
	Brow(Cx, 1.0f);
	HeadRim(Cx);
	FacialHair(Cx);
	FrontHair(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 1.0f);
	FrontLocks(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 0.6f);
	Glasses(Cx);
	Hat(Cx);
	C.Restore();
	C.Restore();
}

void DrawFigure(Canvas& C, const hero::Look& L, float X, float FeetY, float PxPerCm, const Color& Fill, const Color& Edge)
{
	using namespace portrait_detail;
	// Proportions in centimeters for a 175 cm figure, scaled to the height.
	const float H = Fl(L.Height) * PxPerCm;
	const float K = H / 175.0f;
	const float Wide = (L.Body == 1 ? 0.88f : 1.0f) * (L.Build == 0 ? 0.9f : L.Build == 2 ? 1.1f : L.Build == 3 ? 1.18f : 1.0f);
	const float Top = FeetY - H;
	const float HeadR = 11.5f * K;
	auto P = [&](float Cx, float Cy) { return Vec2{X + Cx * K * Wide, Top + Cy * K}; };
	const Pts Body = Spline(Mirror({{0.0f, 24.0f}, {6.0f, 25.0f}, {7.0f, 30.0f}, {21.0f, 34.0f}, {25.0f, 44.0f}, {26.0f, 70.0f}, {25.0f, 97.0f}, {21.5f, 100.0f}, {19.5f, 76.0f}, {18.0f, 52.0f}, {15.0f, 62.0f},
								 {16.5f, 92.0f}, {14.0f, 132.0f}, {12.0f, 171.0f}, {14.0f, 175.0f}, {3.0f, 175.0f}, {2.5f, 132.0f}, {0.0f, 97.0f}}),
		3, true);
	Pts Out;
	for (const Vec2& Q : Body)
	{
		Out.push_back(P(Q.X, Q.Y));
	}
	C.FillPolygon(Out, Fill);
	C.FillEllipse(X, Top + HeadR + 0.5f * K, HeadR * 0.88f * (L.Build == 3 ? 1.08f : 1.0f), HeadR, Fill);
	C.StrokePolyline(Out, true, Edge, 1.0f);
	C.StrokeEllipse(X, Top + HeadR + 0.5f * K, HeadR * 0.88f * (L.Build == 3 ? 1.08f : 1.0f), HeadR, Edge, 1.0f);
}

// ------------------------------------------------------------------ Update 3: the thumbnails' faces (ui-thumbs)
// SPEC §6.1, §7.2: 12 expressions, the hands layer with the 4 P0 poses, the LOD. The default overload above is untouched:
// the expression overload calls it whenever the face is the neutral one with no pose and no LOD (rerun_ui pins the ID
// card, the creator and the barber, and the overload's neutral face against them); otherwise DrawPortraitX below draws
// the same parts in the same order, with the expression's hooks (Ctx::E) and the LOD (Ctx::Lod) switched on.
namespace portrait_detail
{
/** One arm of a pose, in head units (eyes on y = 0): the sleeve from the shoulder through the elbow to the wrist, the hand. */
struct ArmPlan
{
	Vec2 Shoulder, Elbow, Wrist, Hand;
	Vec2 Dir;            // where the fingers (or the fist's knuckles) point
	float Width = 50.0f; // the sleeve at the shoulder
	float HandR = 25.0f;
	int Grip = 0;        // 0 an open hand, 1 a fist, 2 a pointing finger
	bool Behind = false; // the sleeve goes before the head (the hands always after it)
};

Vec2 UnitOf(Vec2 V)
{
	const float L = std::sqrt(V.X * V.X + V.Y * V.Y);
	return L > 1e-6f ? Vec2{V.X / L, V.Y / L} : Vec2{0.0f, -1.0f};
}

std::vector<ArmPlan> ArmsFor(const Geo& G, PortraitPose P, float Side)
{
	std::vector<ArmPlan> Out;
	ArmPlan A;
	const float S = Side < 0.0f ? -1.0f : 1.0f;
	switch (P)
	{
	case PortraitPose::HandsOnHead:
		for (int K = -1; K <= 1; K += 2)
		{
			const float Sk = Fl(K);
			A.Shoulder = {Sk * G.ShoulderW * 0.86f, 206.0f};
			A.Elbow = {Sk * (G.CheekW + 112.0f), -26.0f};
			A.Wrist = {Sk * (G.TempleW + 16.0f), -102.0f};
			A.Hand = {Sk * G.TempleW * 0.5f, -120.0f};
			A.Dir = UnitOf({-Sk, -0.18f});
			A.Width = 52.0f;
			A.HandR = 29.0f;
			A.Grip = 0;
			A.Behind = true;
			Out.push_back(A);
		}
		break;
	case PortraitPose::Facepalm:
		A.Shoulder = {S * G.ShoulderW * 0.72f, 222.0f};
		A.Elbow = {S * 64.0f, 300.0f};
		A.Wrist = {S * 22.0f, 82.0f};
		A.Hand = {S * 6.0f, 4.0f};
		A.Dir = UnitOf({-S * 0.2f, -1.0f});
		A.Width = 52.0f;
		A.HandR = 33.0f;
		A.Grip = 0;
		Out.push_back(A);
		break;
	case PortraitPose::Point:
		A.Shoulder = {S * G.ShoulderW * 0.9f, 206.0f};
		A.Elbow = {S * (G.ShoulderW + 66.0f), 178.0f};
		A.Wrist = {S * (G.ShoulderW + 144.0f), 124.0f};
		A.Hand = {S * (G.ShoulderW + 164.0f), 112.0f};
		A.Dir = UnitOf({S, -0.42f});
		A.Width = 50.0f;
		A.HandR = 24.0f;
		A.Grip = 2;
		Out.push_back(A);
		break;
	case PortraitPose::FistPump:
		A.Shoulder = {S * G.ShoulderW * 0.9f, 206.0f};
		A.Elbow = {S * (G.ShoulderW + 38.0f), 66.0f};
		A.Wrist = {S * (G.CheekW + 98.0f), -62.0f};
		A.Hand = {S * (G.CheekW + 94.0f), -94.0f};
		A.Dir = UnitOf({S * 0.12f, -1.0f});
		A.Width = 50.0f;
		A.HandR = 27.0f;
		A.Grip = 1;
		Out.push_back(A);
		break;
	default: break;
	}
	return Out;
}

/** A segment of a limb as a convex quad, W0 wide at P0 and W1 at P1. */
Pts Limb(Vec2 P0, Vec2 P1, float W0, float W1)
{
	const Vec2 D = UnitOf({P1.X - P0.X, P1.Y - P0.Y});
	const Vec2 N{-D.Y, D.X};
	return {{P0.X + N.X * W0 * 0.5f, P0.Y + N.Y * W0 * 0.5f}, {P1.X + N.X * W1 * 0.5f, P1.Y + N.Y * W1 * 0.5f}, {P1.X - N.X * W1 * 0.5f, P1.Y - N.Y * W1 * 0.5f}, {P0.X - N.X * W0 * 0.5f, P0.Y - N.Y * W0 * 0.5f}};
}

/** The sleeve: the jacket's (or the tee's, with no jacket), lit from the left, a crease at the elbow and a cuff. */
void Sleeve(Ctx& X, const ArmPlan& A)
{
	Canvas& C = *X.C;
	const Color Cloth = X.Who->Appearance.Outfit == hero::OutfitNone ? X.Tee : X.Cloth;
	const Color Hi = Mix(Mix(Cloth, X.Light.Key, 0.14f), Hex(0xffffff), 0.08f);
	const Color Lo = Mix(Cloth, Hex(0x000000), 0.48f);
	const Color Edge = WithA(Mix(Cloth, Hex(0x000000), 0.7f), 0.85f);
	const float W0 = A.Width, W1 = A.Width * 0.86f, W2 = A.Width * 0.68f;
	const Pts Upper = Limb(A.Shoulder, A.Elbow, W0, W1);
	const Pts Fore = Limb(A.Elbow, A.Wrist, W1, W2);
	C.StrokePolyline({A.Shoulder, A.Elbow, A.Wrist}, false, Edge, W1 + 3.0f, true);
	auto Across = [&](Vec2 P0, Vec2 P1, float Wd) {
		const Vec2 D = UnitOf({P1.X - P0.X, P1.Y - P0.Y});
		const Vec2 N{-D.Y, D.X};
		const Vec2 M{(P0.X + P1.X) * 0.5f, (P0.Y + P1.Y) * 0.5f};
		const float Sg = N.X <= 0.0f ? 1.0f : -1.0f; // light from the frame's left
		return Paint::Linear({M.X + N.X * Wd * 0.5f * Sg, M.Y + N.Y * Wd * 0.5f * Sg}, {M.X - N.X * Wd * 0.5f * Sg, M.Y - N.Y * Wd * 0.5f * Sg}, Hi, Lo);
	};
	C.FillPolygon(Upper, Across(A.Shoulder, A.Elbow, W0));
	C.FillCircle(A.Elbow.X, A.Elbow.Y, W1 * 0.5f, Mix(Hi, Lo, 0.5f));
	C.FillPolygon(Fore, Across(A.Elbow, A.Wrist, W1));
	// The crease where the elbow bends, the cuff at the wrist.
	const Vec2 Df = UnitOf({A.Wrist.X - A.Elbow.X, A.Wrist.Y - A.Elbow.Y});
	const Vec2 Nf{-Df.Y, Df.X};
	const Vec2 Ce{A.Elbow.X + Df.X * W1 * 0.4f, A.Elbow.Y + Df.Y * W1 * 0.4f};
	C.StrokePolyline({{Ce.X + Nf.X * W1 * 0.3f, Ce.Y + Nf.Y * W1 * 0.3f}, {Ce.X - Nf.X * W1 * 0.05f + Df.X * 6.0f, Ce.Y - Nf.Y * W1 * 0.05f + Df.Y * 6.0f}}, false, WithA(Lo, 0.8f), 2.0f, true);
	const Vec2 Cu{A.Wrist.X - Df.X * 7.0f, A.Wrist.Y - Df.Y * 7.0f};
	C.FillPolygon(Limb(Cu, A.Wrist, W2 + 2.0f, W2 + 2.0f), Mix(Lo, Cloth, 0.35f));
}

/** A finger (or the thumb): a rounded stroke with its edge. */
void Finger(Canvas& C, Vec2 From, Vec2 To, float W, const Color& Fill, const Color& Edge)
{
	C.StrokePolyline({From, To}, false, Edge, W + 2.4f, true);
	C.StrokePolyline({From, To}, false, Fill, W, true);
}

/** The hand at the end of an arm: an open hand, a fist, or a fist with the index finger out. */
void HandOf(Ctx& X, const ArmPlan& A)
{
	Canvas& C = *X.C;
	const float R = A.HandR;
	const Color Fill = Mix(X.Skin, X.SkinLight, 0.35f);
	const Color Shade = Mix(X.Skin, X.SkinShade, 0.55f);
	const Color Edge = WithA(Lum(X.SkinDeep, -0.25f), 0.75f);
	C.Save();
	C.Translate(A.Hand.X, A.Hand.Y);
	// Local up (0, -1) turns to Dir.
	C.Rotate(std::atan2(A.Dir.X, -A.Dir.Y));
	const Paint Palm = Paint::Linear({-R, 0.0f}, {R, 0.0f}, Fill, Shade);
	// The thumb sits on the side toward the body's middle; a hand on the frame's right has it on its local left.
	const float Th = A.Hand.X >= 0.0f ? -1.0f : 1.0f;
	if (A.Grip == 0)
	{
		static const float Len[4] = {0.9f, 1.06f, 1.0f, 0.8f};
		for (int K = 0; K < 4; ++K)
		{
			const float Fx = (-0.5f + 0.33f * Fl(K)) * R;
			const float Lean = (Fl(K) - 1.5f) * 0.09f;
			const float L = Len[K] * R * 1.05f;
			Finger(C, {Fx, -R * 0.45f}, {Fx + std::sin(Lean) * L, -R * 0.45f - std::cos(Lean) * L}, R * 0.3f, Fill, Edge);
		}
		C.FillEllipse(0.0f, 0.0f, R * 0.74f, R * 0.8f, Palm);
		C.StrokeEllipse(0.0f, 0.0f, R * 0.74f, R * 0.8f, Edge, 1.4f);
		Finger(C, {Th * R * 0.55f, R * 0.2f}, {Th * R * 1.02f, -R * 0.28f}, R * 0.32f, Fill, Edge);
		if (!X.Lod)
		{
			C.StrokeArc(0.0f, R * 0.15f, R * 0.4f, 3.6f, 5.2f, WithA(X.SkinDeep, 0.35f), 1.2f, true);
		}
	}
	else
	{
		if (A.Grip == 2)
		{
			// The index finger out along Dir.
			Finger(C, {-Th * R * 0.32f, -R * 0.55f}, {-Th * R * 0.36f, -R * 1.95f}, R * 0.3f, Fill, Edge);
		}
		C.FillRoundRect({-R * 0.8f, -R * 0.72f, R * 1.6f, R * 1.38f}, R * 0.38f, Palm);
		C.StrokeRoundRect({-R * 0.8f, -R * 0.72f, R * 1.6f, R * 1.38f}, R * 0.38f, Edge, 1.4f);
		const int First = A.Grip == 2 ? 1 : 0;
		for (int K = First; K < 4; ++K)
		{
			// Knuckles across the top, the fingers' creases under them.
			const float Kx = (-0.56f + 0.37f * Fl(K)) * R * -Th;
			C.FillEllipse(Kx, -R * 0.62f, R * 0.19f, R * 0.13f, Mix(Fill, Hex(0xffffff), 0.12f));
			if (!X.Lod)
			{
				C.StrokePolyline({{Kx - R * 0.15f, -R * 0.2f}, {Kx + R * 0.15f, -R * 0.18f}}, false, WithA(X.SkinDeep, 0.45f), 1.1f, true);
			}
		}
		Finger(C, {Th * R * 0.72f, R * 0.32f}, {-Th * R * 0.05f, -R * 0.05f}, R * 0.34f, Fill, Edge);
	}
	C.Restore();
}

/** Points rotated by A radians about Pivot. */
Pts Turned(const Pts& P, Vec2 Pivot, float A)
{
	const float Cs = std::cos(A), Sn = std::sin(A);
	Pts Out;
	for (const Vec2& Q : P)
	{
		const float Dx = Q.X - Pivot.X, Dy = Q.Y - Pivot.Y;
		Out.push_back({Pivot.X + Dx * Cs - Dy * Sn, Pivot.Y + Dx * Sn + Dy * Cs});
	}
	return Out;
}

/** The convex hull (Andrew's monotone chain), clockwise on screen. */
Pts Hull(Pts P)
{
	std::sort(P.begin(), P.end(), [](const Vec2& A, const Vec2& B) { return A.X < B.X || (A.X == B.X && A.Y < B.Y); });
	if (P.size() < 3)
	{
		return P;
	}
	Pts H(P.size() * 2);
	size_t K = 0;
	for (size_t I = 0; I < P.size(); ++I)
	{
		while (K >= 2 && Cross(H[K - 2], H[K - 1], P[I]) <= 0.0f)
		{
			--K;
		}
		H[K++] = P[I];
	}
	for (size_t I = P.size() - 1, T = K + 1; I-- > 0;)
	{
		while (K >= T && Cross(H[K - 2], H[K - 1], P[I]) <= 0.0f)
		{
			--K;
		}
		H[K++] = P[I];
	}
	H.resize(K - 1);
	return H;
}

/** The head with its hair and any hat, the outline a cut-out follows (head units, untilted). */
Pts HeadHull(const Ctx& X)
{
	const Geo& G = X.H.G;
	const hero::Look& L = X.Who->Appearance;
	const int Style = std::clamp(L.Hair, 0, 11);
	const float Hair = HairOuter(Style) + 2.0f;
	Pts P;
	for (int D = 0; D < 360; D += 6)
	{
		const float A = Fl(D);
		const bool Top = D <= 104 || D >= 256;
		P.push_back(X.H.Rim(A, Top ? Hair : 0.5f));
	}
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float S = Fl(Side);
		P.push_back({S * (G.CheekW + 13.0f), -4.0f});
		P.push_back({S * (G.CheekW + 10.0f), 22.0f});
		if (Style == 8 || Style == 10)
		{
			P.push_back({S * (G.CheekW + 40.0f), 20.0f});
			P.push_back({S * (G.CheekW + 46.0f), 190.0f});
		}
		if (L.Hat != 0)
		{
			P.push_back({S * (G.TempleW + 34.0f), -32.0f});
			P.push_back({S * (G.TempleW + 14.0f), G.CrownY - 18.0f});
		}
	}
	if (Style == 7)
	{
		for (int D = 0; D < 360; D += 20)
		{
			const float A = Fl(D) * Deg;
			P.push_back({(G.CheekW + 62.0f) * std::sin(A), -58.0f - 126.0f * std::cos(A)});
		}
	}
	if (L.Hat != 0)
	{
		P.push_back({0.0f, G.CrownY - 26.0f});
	}
	return Hull(P);
}

void DrawPortraitX(Canvas& C, const hero::Character& Who, float X, float Y, float Scale, double Time, const PortraitLight& Light, const PortraitExpression& E, PortraitPose P, bool Lod)
{
	// The LOD draws coarse (Spline, Ellipse and Soft read it), put back when this portrait is done.
	struct CoarseScope
	{
		int Old;
		explicit CoarseScope(int V) : Old(PortraitCoarse) { PortraitCoarse = V; }
		~CoarseScope() { PortraitCoarse = Old; }
		CoarseScope(const CoarseScope&) = delete;
		CoarseScope& operator=(const CoarseScope&) = delete;
	} Coarse(Lod ? 1 : 0);
	const hero::Look& L = Who.Appearance;
	Ctx Cx;
	Cx.C = &C;
	Cx.Who = &Who;
	Cx.Light = Light;
	Cx.Age = Fl(Who.Age);
	Cx.H = HeadFor(L, Who.Age);
	Cx.E = &E;
	Cx.Lod = Lod;
	// The palette, exactly as the default overload builds it.
	Cx.Skin = Hex(hero::SkinTone(L.Skin));
	Cx.SkinLight = Mix(Mix(Cx.Skin, Light.Key, 0.35f), Hex(0xffffff), 0.12f);
	Cx.SkinShade = Mix(Cx.Skin, Hex(0x3a1d22), 0.36f);
	Cx.SkinDeep = Mix(Cx.Skin, Hex(0x2a1214), 0.62f);
	Cx.Blush = Mix(Cx.Skin, Hex(0xd8545c), 0.45f);
	const Color HairBase = Hex(hero::HairToneAt(L, Who.Age));
	const float Grey = L.HairColor >= 6 ? 0.0f : std::clamp((Cx.Age - 44.0f) / 30.0f, 0.0f, 0.65f);
	Cx.Hair = HairBase;
	Cx.HairLight = Mix(HairBase, Mix(Hex(0xfff4e6), Hex(0xf2f5fa), Grey / 0.65f), Luma(HairBase) > 0.5f ? 0.25f : 0.2f);
	Cx.HairDark = Mix(HairBase, Hex(0x050404), 0.45f);
	Cx.Brow = L.HairColor >= 5 ? Mix(HairBase, Hex(0x6a5848), 0.45f) : Mix(HairBase, Hex(0x000000), 0.25f);
	Cx.Beard = Mix(HairBase, Hex(0x000000), 0.1f);
	Cx.Cloth = Hex(hero::OutfitTone(L.OutfitColor));
	Cx.ClothLight = Mix(Mix(Cx.Cloth, Light.Key, 0.12f), Hex(0xffffff), 0.06f);
	Cx.ClothDark = Mix(Cx.Cloth, Hex(0x000000), 0.5f);
	Cx.Tee = L.Outfit == hero::OutfitNone ? Cx.Cloth : Luma(Cx.Cloth) < 0.33f ? Hex(0xe7e3dc) : Hex(0x26282d);
	const float T = static_cast<float>(Time);
	Cx.Breath = Time > 0.0 ? std::sin(T * 1.6f) : 0.0f;
	const float Phase = std::fmod(T + 0.9f, 4.3f);
	Cx.EyeOpen = Time > 0.0 && Phase < 0.16f ? 1.0f - std::sin(Phase / 0.16f * 3.1415926f) : 1.0f;
	Cx.LookX = Time > 0.0 ? 0.9f * std::sin(T * 0.37f) : 0.0f;

	const float Tilt = std::clamp(E.HeadTilt, -20.0f, 20.0f) * Deg;
	const std::vector<ArmPlan> Arms = ArmsFor(Cx.H.G, P, thumbportrait::PoseSide(E));
	const bool OpenMouth = E.MouthOpen >= 0.06f;
	// The head turns about the top of the neck.
	auto HeadSpace = [&](float Dy) {
		C.Save();
		C.Translate(0.0f, Dy);
		if (Tilt != 0.0f)
		{
			C.Translate(0.0f, 70.0f);
			C.Rotate(Tilt);
			C.Translate(0.0f, -70.0f);
		}
	};
	C.Save();
	C.Translate(X, Y);
	C.Scale(Scale, Scale);
	C.Save();
	C.Translate(0.0f, Cx.Breath * 1.4f);
	if (L.Outfit == 0)
	{
		HoodBack(Cx);
	}
	C.Restore();
	HeadSpace(Cx.Breath * 0.6f);
	BackHair(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 0.6f);
	Neck(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 1.4f);
	Outfit(Cx);
	for (const ArmPlan& A : Arms)
	{
		if (A.Behind)
		{
			Sleeve(Cx, A);
		}
	}
	C.Restore();
	HeadSpace(Cx.Breath * 0.6f);
	Ear(Cx, -1.0f);
	Ear(Cx, 1.0f);
	HeadFill(Cx);
	if (E.BrowKnit > 0.6f && E.Smile < -0.2f)
	{
		// Tilted: the flush of it.
		Soft(C, 0.0f, -10.0f, Cx.H.G.CheekW * 0.95f, 80.0f, Hex(0xff3a2a), 0.22f);
	}
	if (E.Smile > 0.5f)
	{
		Soft(C, -36.0f, 30.0f, 17.0f, 11.0f, Cx.Blush, 0.3f);
		Soft(C, 36.0f, 30.0f, 17.0f, 11.0f, Cx.Blush, 0.24f);
	}
	Wrinkles(Cx);
	Nose(Cx);
	if (OpenMouth)
	{
		// An open mouth goes over the beard, which would otherwise close it.
		FacialHair(Cx);
		Mouth(Cx);
	}
	else
	{
		Mouth(Cx);
	}
	Eye(Cx, -1.0f);
	Eye(Cx, 1.0f);
	Brow(Cx, -1.0f);
	Brow(Cx, 1.0f);
	HeadRim(Cx);
	if (!OpenMouth)
	{
		FacialHair(Cx);
	}
	FrontHair(Cx);
	if (E.MouthOpen > 0.85f && E.BrowLift > 0.5f)
	{
		// A scream's sweat on the temple.
		const float Sx = Cx.H.G.TempleW * 0.72f;
		C.FillPolygon(Spline({{Sx, -44.0f}, {Sx - 4.5f, -30.0f}, {Sx, -24.0f}, {Sx + 4.5f, -30.0f}}, 4, true), Paint::Linear({Sx, -44.0f}, {Sx, -24.0f}, Hex(0xe4f6ff), Hex(0x76c8f0)));
	}
	C.Restore();
	HeadSpace(Cx.Breath * 1.0f);
	FrontLocks(Cx);
	C.Restore();
	HeadSpace(Cx.Breath * 0.6f);
	Glasses(Cx);
	Hat(Cx);
	C.Restore();
	C.Save();
	C.Translate(0.0f, Cx.Breath * 1.4f);
	for (const ArmPlan& A : Arms)
	{
		if (!A.Behind)
		{
			Sleeve(Cx, A);
		}
		HandOf(Cx, A);
	}
	C.Restore();
	C.Restore();
}

/** Whether E is the default face (every field at rest). */
bool AtRest(const PortraitExpression& E)
{
	return E.BrowLift == 0.0f && E.BrowKnit == 0.0f && E.BrowAsym == 0.0f && E.EyeOpen == 1.0f && E.LookX == 0.0f && E.LookY == 0.0f && E.Smile == 0.0f && E.MouthOpen == 0.0f && E.Smirk == 0.0f &&
		E.HeadTilt == 0.0f && !E.Teeth && !E.Tear;
}
} // namespace portrait_detail

void DrawPortrait(Canvas& C, const hero::Character& Who, float X, float Y, float Scale, double Time, const PortraitLight& Light, const PortraitExpression& E, PortraitPose P,
	bool Lod)
{
	using namespace portrait_detail;
	if (AtRest(E) && P == PortraitPose::None && !Lod)
	{
		// The default face: the default overload itself, byte for byte.
		DrawPortrait(C, Who, X, Y, Scale, Time, Light);
		return;
	}
	// The P1 poses (cards up, holding, shrug, thumbs down) draw no hands until they land.
	const PortraitPose Pose = P >= PortraitPose::CardsUp ? PortraitPose::None : P;
	DrawPortraitX(C, Who, X, Y, Scale, Time, Light, E, Pose, Lod);
}

const PortraitExpression& ExpressionOf(int Expr)
{
	// SPEC §6.1's 12, in its order (rerun's recipes store the index): Neutral, Smile, Grin, Laugh, Shocked, Scream,
	// Devastated, Tilted, Smug, Skeptical, Thinking, Exhausted.
	static const PortraitExpression Faces[12] = {
		[] { return PortraitExpression(); }(),
		[] {
			PortraitExpression F;
			F.Smile = 0.62f;
			F.BrowLift = 0.15f;
			F.EyeOpen = 0.92f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.Smile = 1.0f;
			F.MouthOpen = 0.3f;
			F.Teeth = true;
			F.EyeOpen = 0.8f;
			F.BrowLift = 0.25f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.Smile = 1.0f;
			F.MouthOpen = 0.78f;
			F.Teeth = true;
			F.EyeOpen = 0.18f;
			F.BrowLift = 0.55f;
			F.HeadTilt = -7.0f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowLift = 1.2f;
			F.EyeOpen = 1.45f;
			F.MouthOpen = 0.62f;
			F.Smile = -0.1f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowLift = 1.0f;
			F.BrowKnit = -0.5f;
			F.EyeOpen = 1.38f;
			F.MouthOpen = 1.0f;
			F.Teeth = true;
			F.Smile = -0.35f;
			F.HeadTilt = 5.0f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowKnit = -0.95f;
			F.BrowLift = 0.2f;
			F.EyeOpen = 0.72f;
			F.LookY = 2.5f;
			F.Smile = -0.85f;
			F.MouthOpen = 0.22f;
			F.Tear = true;
			F.HeadTilt = 6.0f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowKnit = 1.0f;
			F.BrowLift = -0.35f;
			F.EyeOpen = 0.66f;
			F.Smile = -0.55f;
			F.MouthOpen = 0.18f;
			F.Teeth = true;
			F.HeadTilt = -3.0f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowAsym = 0.65f;
			F.EyeOpen = 0.68f;
			F.Smirk = 0.95f;
			F.Smile = 0.2f;
			F.LookX = 2.5f;
			F.HeadTilt = 5.0f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowAsym = 1.0f;
			F.BrowKnit = 0.3f;
			F.EyeOpen = 0.74f;
			F.Smirk = -0.5f;
			F.Smile = -0.2f;
			F.LookX = -2.5f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.BrowKnit = 0.3f;
			F.BrowAsym = 0.45f;
			F.LookX = 3.5f;
			F.LookY = -3.5f;
			F.EyeOpen = 0.9f;
			F.Smirk = 0.35f;
			F.Smile = -0.1f;
			F.HeadTilt = -4.0f;
			return F;
		}(),
		[] {
			PortraitExpression F;
			F.EyeOpen = 0.55f;
			F.BrowKnit = -0.4f;
			F.BrowLift = -0.2f;
			F.Smile = -0.4f;
			F.MouthOpen = 0.1f;
			F.LookY = 2.0f;
			F.HeadTilt = 7.0f;
			return F;
		}(),
	};
	return Faces[Expr >= 0 && Expr < 12 ? Expr : 0];
}

namespace thumbportrait
{
float PoseSide(const PortraitExpression& E)
{
	return E.LookX < 0.0f ? -1.0f : 1.0f;
}

std::vector<std::vector<Vec2>> Silhouette(const hero::Character& Who, float X, float Y, float Scale, const PortraitExpression& E, PortraitPose P)
{
	using namespace portrait_detail;
	Ctx Cx;
	Cx.Who = &Who;
	Cx.H = HeadFor(Who.Appearance, Who.Age);
	const Geo& G = Cx.H.G;
	const float Tilt = std::clamp(E.HeadTilt, -20.0f, 20.0f) * Deg;
	std::vector<Pts> Pieces;
	Pieces.push_back(Turned(HeadHull(Cx), {0.0f, 70.0f}, Tilt));
	Pieces.push_back({{-G.NeckW * 1.06f, 40.0f}, {G.NeckW * 1.06f, 40.0f}, {G.NeckW * 1.1f, 170.0f}, {-G.NeckW * 1.1f, 170.0f}});
	Pts Torso = TorsoOutline(Cx);
	if (Who.Appearance.Outfit == 0)
	{
		Torso.push_back({G.NeckW + 46.0f, 140.0f});
		Torso.push_back({-G.NeckW - 46.0f, 140.0f});
	}
	Pieces.push_back(Hull(Torso));
	const PortraitPose Pose = P >= PortraitPose::CardsUp ? PortraitPose::None : P;
	for (const ArmPlan& A : ArmsFor(G, Pose, PoseSide(E)))
	{
		Pieces.push_back(Hull(Pts{Limb(A.Shoulder, A.Elbow, A.Width + 2.0f, A.Width * 0.86f + 2.0f)}));
		Pieces.push_back(Hull(Pts{Limb(A.Elbow, A.Wrist, A.Width * 0.86f + 2.0f, A.Width * 0.68f + 2.0f)}));
		Pts Hand = Ellipse(A.Hand.X, A.Hand.Y, A.HandR * 0.95f, A.HandR * 0.95f, 14);
		const float Reach = A.Grip == 0 ? 1.45f : A.Grip == 2 ? 1.95f : 0.75f;
		for (const Vec2& Q : Ellipse(A.Hand.X + A.Dir.X * A.HandR * Reach * 0.6f, A.Hand.Y + A.Dir.Y * A.HandR * Reach * 0.6f, A.HandR * 0.7f, A.HandR * 0.7f, 12))
		{
			Hand.push_back(Q);
		}
		Hand.push_back({A.Hand.X + A.Dir.X * A.HandR * Reach, A.Hand.Y + A.Dir.Y * A.HandR * Reach});
		Pieces.push_back(Hull(Hand));
	}
	std::vector<std::vector<Vec2>> Out;
	for (const Pts& Piece : Pieces)
	{
		std::vector<Vec2> Q;
		for (const Vec2& V : Piece)
		{
			Q.push_back({X + V.X * Scale, Y + V.Y * Scale});
		}
		Out.push_back(Q);
	}
	return Out;
}
} // namespace thumbportrait
} // namespace ui
} // namespace ss
