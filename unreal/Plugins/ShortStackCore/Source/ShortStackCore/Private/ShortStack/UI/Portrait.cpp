#include "ShortStack/UI/Portrait.h"
#include "../StrictFloat.h"

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
		Pts Out;
		for (float A = A0; A < A1; A += Step)
		{
			Out.push_back(Rim(A, Thick(A)));
		}
		Out.push_back(Rim(A1, Thick(A1)));
		return Out;
	}
};

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
		for (int I = 0; I < 90; ++I)
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
		if (Style == 10)
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
		for (int I = 0; I < 15; ++I)
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
		for (int I = 0; I < (Afro ? 30 : 70); ++I)
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
		for (int I = 0; I < 12; ++I)
		{
			const float K = Fl(I) / 11.0f * 2.0f - 1.0f;
			C.StrokePolyline(Spline({{K * G.TempleW * 0.85f, G.HairlineY + 4.0f + std::fabs(K) * 16.0f}, X.H.Rim(K * 50.0f, 6.0f), X.H.Rim(26.0f, 8.0f)}, 5, false), false, WithA(I % 2 ? X.HairLight : X.HairDark, 0.22f),
				1.3f, true);
		}
		C.StrokePolyline(X.H.Arc(-55.0f, -20.0f, [](float) { return 5.0f; }), false, WithA(Hex(0xffffff), 0.22f), 3.0f, true);
		const Vec2 Tie = X.H.Rim(26.0f, 8.0f);
		C.FillEllipse(Tie.X, Tie.Y, 7.0f, 5.0f, Lum(X.Cloth, -0.2f));
		HairRim(X, [](float) { return 8.0f; });
		break;
	}
	case 10:
	{
		// Braids: a center part and rows running back.
		Pts Hr = HairlineRight(X, -2.0f);
		const Pts P = Cap(X, [](float) { return 9.0f; }, 0.0f, &Hr);
		HairFill(X, P);
		for (int I = 0; I < 10; ++I)
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
		for (int I = 0; I < 14; ++I)
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
			for (int I = 0; I < 4; ++I)
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
				for (size_t K = 0; K + 1 < Braid.size(); ++K)
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
	for (float A = 40.0f; A <= 150.0f; A += 3.0f)
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
	const float H = G.EyeH * X.EyeOpen;
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
	Pts Shape = Upper;
	for (size_t I = 1; I + 1 < Lower.size(); ++I)
	{
		Shape.push_back(Lower[I]);
	}
	C.FillPolygon(Shape, Paint::Linear({0.0f, -H}, {0.0f, H}, Mix(Hex(0xf2ede6), X.SkinShade, 0.35f), Hex(0xf4efe9)));
	// Iris and pupil, cut by the lids.
	const Color Iris = Hex(hero::EyeTone(X.Who->Appearance.Eyes));
	const float Ir = G.EyeH * 0.9f;
	const float Ix = Cx + X.LookX;
	const Pts IrisPoly = ClipConvex(Ellipse(Ix, 1.4f, Ir, Ir, 24), Shape);
	if (!IrisPoly.empty())
	{
		C.FillPolygon(IrisPoly, Paint::Linear({0.0f, 1.4f - Ir}, {0.0f, 1.4f + Ir}, Lum(Iris, -0.45f), Lum(Iris, 0.12f)));
		const Pts Ring = ClipConvex(Ellipse(Ix, 1.4f, Ir - 0.3f, Ir - 0.3f, 24), Shape);
		if (!Ring.empty())
		{
			C.StrokePolyline(Ring, true, WithA(Lum(Iris, -0.6f), 0.6f), 1.0f);
		}
		const Pts Pupil = ClipConvex(Ellipse(Ix, 1.4f, Ir * 0.42f, Ir * 0.42f, 16), Shape);
		if (!Pupil.empty())
		{
			C.FillPolygon(Pupil, Hex(0x0b0907));
		}
		C.FillCircle(Ix - 2.0f, -1.2f, 1.5f, WithA(Hex(0xffffff), 0.9f));
		C.FillCircle(Ix + 2.2f, 3.2f, 0.8f, WithA(Hex(0xffffff), 0.45f));
	}
	// Lid shadow over the top of the eye, then the lash line and the crease.
	C.StrokePolyline(Upper, false, WithA(Lum(X.SkinDeep, -0.2f), 0.35f), 4.0f, true);
	C.StrokePolyline(Upper, false, Lum(X.SkinDeep, -0.55f), G.SoftFace ? 2.6f : 2.0f, true);
	if (G.SoftFace)
	{
		// A flick of liner and lashes at the outer corner.
		C.StrokePolyline({Outer, {Outer.X + S * 4.5f, Outer.Y - 3.0f}}, false, Lum(X.SkinDeep, -0.6f), 2.0f, true);
		for (int I = 0; I < 3; ++I)
		{
			const Vec2 B = Upper[Upper.size() - 2 - static_cast<size_t>(I) * 2];
			C.StrokePolyline({B, {B.X + S * 2.5f, B.Y - 3.0f}}, false, Lum(X.SkinDeep, -0.6f), 1.1f, true);
		}
	}
	C.StrokePolyline(Moved(Spline({{Cx - S * W * 0.7f, 1.0f - H * 0.7f}, {Cx, -H - 4.0f}, {Cx + S * W * 0.8f, -H * 0.6f}}, 4, false), 0.0f, -2.5f), false, WithA(X.SkinDeep, 0.35f), 1.3f, true);
	C.StrokePolyline(Lower, false, WithA(X.SkinDeep, 0.28f), 1.1f, true);
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
	const Vec2 In{Cx - S * G.EyeW * 1.05f, Y0 - 1.0f};
	const Vec2 Peak{Cx + S * G.EyeW * 0.35f, Y0 - 3.0f - Arch * 0.7f};
	const Vec2 Out{Cx + S * G.EyeW * 1.2f, Y0 - 0.5f};
	const Pts Top = Spline({In, Lerp2(In, Peak, 0.5f), Peak, Out}, 4, false);
	Pts Shape = Moved(Top, 0.0f, -Thick * 0.5f);
	const Pts Bottom = Spline({Out, {Peak.X, Peak.Y + Thick * 0.85f}, {Lerp2(In, Peak, 0.5f).X, Lerp2(In, Peak, 0.5f).Y + Thick}, {In.X, In.Y + Thick * 0.9f}}, 4, false);
	for (const Vec2& P : Bottom)
	{
		Shape.push_back({P.X, P.Y - Thick * 0.5f});
	}
	X.C->FillPolygon(Shape, X.Brow);
	for (int I = 0; I < 6; ++I)
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
	// Nasolabial folds, deeper with age.
	const float Fold = 0.12f + std::clamp((X.Age - 26.0f) / 30.0f, 0.0f, 1.0f) * 0.3f;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float S = Fl(Side);
		X.C->StrokePolyline(Spline({{S * 14.0f * Wd, 38.0f}, {S * 22.0f, 50.0f}, {S * 27.0f, 64.0f}}, 4, false), false, WithA(X.SkinDeep, Fold * (Side > 0 ? 1.2f : 0.8f)), 1.6f, true);
	}
}

void Mouth(Ctx& X)
{
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
	if (A < 0.01f)
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
		for (int I = 0; I < (Full ? 70 : 40); ++I)
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
				if (!Thin.empty())
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
				if (!Thin.empty())
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

void Hat(Ctx& X)
{
	const int Kind = X.Who->Appearance.Hat;
	if (Kind == 0)
	{
		return;
	}
	Canvas& C = *X.C;
	const Geo& G = X.H.G;
	// The hat goes with the jacket without matching it.
	static const int Partner[8] = {7, 4, 5, 0, 1, 3, 0, 5};
	const Color HatTone = Hex(hero::OutfitTone(Partner[std::clamp(X.Who->Appearance.OutfitColor, 0, 7)]));
	const Paint Fill = Paint::Linear({-G.TempleW - 20.0f, 0.0f}, {G.TempleW + 20.0f, 0.0f}, Lum(HatTone, 0.12f), Lum(HatTone, -0.45f));
	const int Style = HairStyle(X);
	const float Puff = Style == 6 ? 12.0f : Style == 7 ? 6.0f : Style == 3 || Style == 4 || Style == 5 ? 6.0f : 0.0f;
	switch (Kind)
	{
	case 1: // Beanie
	{
		Pts Dome = X.H.Arc(-100.0f, 100.0f, [Puff](float A) { return 13.0f + Puff + 6.0f * std::cos(A * Deg); }, 4.0f);
		Dome.push_back({G.TempleW * 0.62f, -54.0f});
		Dome.push_back({0.0f, -56.0f});
		Dome.push_back({-G.TempleW * 0.62f, -54.0f});
		C.FillPolygon(Dome, Fill);
		const Pts Cuff = ClipConvex(Dome, {{-300.0f, -82.0f}, {300.0f, -82.0f}, {300.0f, 0.0f}, {-300.0f, 0.0f}});
		if (!Cuff.empty())
		{
			C.FillPolygon(Cuff, Paint::Linear({-G.TempleW - 20.0f, 0.0f}, {G.TempleW + 20.0f, 0.0f}, Lum(HatTone, 0.2f), Lum(HatTone, -0.4f)));
			for (float Rx = -G.TempleW - 14.0f; Rx < G.TempleW + 14.0f; Rx += 6.0f)
			{
				const Pts Rib = ClipConvex({{Rx, -84.0f}, {Rx + 1.6f, -84.0f}, {Rx + 1.6f, -40.0f}, {Rx, -40.0f}}, Cuff);
				if (!Rib.empty())
				{
					C.FillPolygon(Rib, WithA(Lum(HatTone, -0.5f), 0.45f));
				}
			}
			C.StrokePolyline(Spline({{-G.TempleW - 14.0f, -82.0f}, {0.0f, -84.0f}, {G.TempleW + 14.0f, -82.0f}}, 4, false), false, WithA(Lum(HatTone, -0.6f), 0.6f), 1.6f, true);
		}
		C.StrokePolyline(X.H.Arc(20.0f, 96.0f, [Puff](float A) { return 13.0f + Puff + 6.0f * std::cos(A * Deg) - 0.6f; }, 4.0f), false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.2f, true);
		X.HatTop = -56.0f;
		break;
	}
	case 2: // Ball cap, brim forward
	case 3: // Ball cap, backwards
	{
		const bool Back = Kind == 3;
		Pts Crown = X.H.Arc(-74.0f, 74.0f, [Puff](float A) { return 7.0f + Puff * 0.7f + 4.0f * std::cos(A * Deg); }, 4.0f);
		Crown.push_back({G.TempleW * 0.7f, -60.0f});
		Crown.push_back({0.0f, -66.0f});
		Crown.push_back({-G.TempleW * 0.7f, -60.0f});
		C.FillPolygon(Crown, Fill);
		const Vec2 Top = X.H.Rim(0.0f, 11.0f + Puff * 0.7f);
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
		C.StrokePolyline(X.H.Arc(20.0f, 72.0f, [Puff](float A) { return 7.0f + Puff * 0.7f + 4.0f * std::cos(A * Deg) - 0.6f; }, 4.0f), false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.2f,
			true);
		break;
	}
	default: // Bucket hat
	{
		Pts Crown = X.H.Arc(-94.0f, 94.0f, [Puff](float A) { return 12.0f + Puff * 0.6f + 2.0f * std::cos(A * Deg); }, 4.0f);
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
		C.StrokePolyline(X.H.Arc(20.0f, 92.0f, [Puff](float A) { return 12.0f + Puff * 0.6f + 2.0f * std::cos(A * Deg) - 0.6f; }, 4.0f), false, WithA(X.Light.Rim, 0.55f * X.Light.RimStrength), 2.2f,
			true);
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
	Color HairBase = Hex(hero::HairTone(L.HairColor));
	const float Grey = L.HairColor >= 6 ? 0.0f : std::clamp((Cx.Age - 44.0f) / 30.0f, 0.0f, 0.65f);
	HairBase = Mix(HairBase, Hex(0xb8b5ae), Grey);
	Cx.Hair = HairBase;
	Cx.HairLight = Mix(HairBase, Hex(0xfff4e6), Luma(HairBase) > 0.5f ? 0.25f : 0.2f);
	Cx.HairDark = Mix(HairBase, Hex(0x050404), 0.45f);
	Cx.Brow = L.HairColor >= 5 ? Mix(HairBase, Hex(0x6a5848), 0.45f) : Mix(HairBase, Hex(0x000000), 0.25f);
	Cx.Beard = Mix(HairBase, Hex(0x000000), 0.1f);
	Cx.Cloth = Hex(hero::OutfitTone(L.OutfitColor));
	Cx.ClothLight = Mix(Mix(Cx.Cloth, Light.Key, 0.12f), Hex(0xffffff), 0.06f);
	Cx.ClothDark = Mix(Cx.Cloth, Hex(0x000000), 0.5f);
	Cx.Tee = Luma(Cx.Cloth) < 0.33f ? Hex(0xe7e3dc) : Hex(0x26282d);

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
} // namespace ui
} // namespace ss
