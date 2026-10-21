// Update 3 (SPEC §2.3, §2.4): Ignite's art, ui-ignite's file (with its private IgniteArt.h): the palette constants (never in
// pal::, none a RiverLine hex), the hour's tones, the boardwalk band (a cached static skyline and a live layer), the
// Ferris wheel, cabinets, the flame, stickers, tickets, bulbs, the Fuel Gauge and the fuse. Pure drawing: nothing here reads
// the session, decides an outcome or keeps state; every "random" placement is a keyed hash (never rand()).
#include "IgniteArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ss
{
namespace ui
{
namespace igniteart
{
namespace
{
/** One cubic segment of a unit path: the end point and its two controls (the start is the previous end). */
struct IgniteCubic
{
	float C1x, C1y, C2x, C2y, Ex, Ey;
};
// The 24-point flame of siteart::Logo as unit cubics: the foot at (0, 0), the tip near (0, -1.42) (the mockups' ignite_art.py).
const IgniteCubic IgniteFlameOuter[] = {
	{-0.5f, 0.0f, -0.64f, -0.40f, -0.44f, -0.70f},
	{-0.34f, -0.86f, -0.32f, -1.0f, -0.38f, -1.15f},
	{-0.14f, -1.02f, -0.08f, -0.88f, -0.10f, -0.74f},
	{0.0f, -0.98f, 0.10f, -1.2f, 0.02f, -1.42f},
	{0.40f, -1.18f, 0.56f, -0.86f, 0.50f, -0.58f},
	{0.60f, -0.64f, 0.66f, -0.76f, 0.64f, -0.88f},
	{0.82f, -0.56f, 0.68f, -0.10f, 0.30f, -0.02f},
	{0.2f, 0.0f, 0.1f, 0.0f, 0.0f, 0.0f},
};
const IgniteCubic IgniteFlameInner[] = {
	{-0.30f, 0.0f, -0.38f, -0.26f, -0.26f, -0.46f},
	{-0.18f, -0.60f, -0.12f, -0.70f, -0.14f, -0.84f},
	{0.06f, -0.72f, 0.14f, -0.62f, 0.12f, -0.50f},
	{0.22f, -0.56f, 0.26f, -0.64f, 0.26f, -0.74f},
	{0.40f, -0.48f, 0.36f, -0.06f, 0.12f, -0.01f},
	{0.08f, 0.0f, 0.04f, 0.0f, 0.0f, 0.0f},
};

template <size_t N>
std::vector<Vec2> IgniteFlattened(const IgniteCubic (&Path)[N], float X, float Y, float S, int Steps)
{
	std::vector<Vec2> Out;
	float Px = 0.0f;
	float Py = 0.0f;
	for (const IgniteCubic& K : Path)
	{
		for (int I = 1; I <= Steps; ++I)
		{
			const float T = static_cast<float>(I) / static_cast<float>(Steps);
			const float U = 1.0f - T;
			const float Bx = U * U * U * Px + 3.0f * U * U * T * K.C1x + 3.0f * U * T * T * K.C2x + T * T * T * K.Ex;
			const float By = U * U * U * Py + 3.0f * U * U * T * K.C1y + 3.0f * U * T * T * K.C2y + T * T * T * K.Ey;
			Out.push_back({X + Bx * S, Y + By * S});
		}
		Px = K.Ex;
		Py = K.Ey;
	}
	return Out;
}

float IgniteF(double V)
{
	return static_cast<float>(V);
}

/** A keyed unit from an integer pair (the static layer's stars and windows). */
double IgniteHashUnit(uint32_t A, uint32_t B)
{
	return static_cast<double>(IgniteMix(A * 0x9E3779B1u ^ (B + 0x7F4A7C15u)) >> 8) / 16777216.0;
}

Color IgniteHexA(uint32_t Rgb, float A)
{
	return Hex(Rgb, A);
}
} // namespace

uint32_t IgniteMix(uint32_t H)
{
	H ^= H >> 16;
	H *= 0x85ebca6bu;
	H ^= H >> 13;
	H *= 0xc2b2ae35u;
	H ^= H >> 16;
	return H;
}

double IgniteUnit(const std::string& Key)
{
	return static_cast<double>(IgniteMix(Fnv1a(Key)) >> 8) / 16777216.0;
}

IgniteTones IgniteTonesFor(site::show::Mood Mood)
{
	IgniteTones K;
	K.Mood = Mood;
	K.Night = Mood == site::show::Mood::Afterburner;
	K.Bpm = K.Night ? 96.0 : 124.0;
	switch (Mood)
	{
	case site::show::Mood::Afterburner:
		K.Label = "AFTERBURNER HOURS";
		K.BandA = 0x6b3fd1;
		K.BandB = 0x2b1a5c;
		K.SkyTop = 0x07061a;
		K.SkyMid = 0x120c2e;
		K.SkyLow = 0x281a52;
		K.Glow = IgNightAccent;
		break;
	case site::show::Mood::WakeUp:
		K.Label = "WAKE-UP";
		K.BandA = IgTang;
		K.BandB = IgSky;
		K.SkyTop = 0x1c1638;
		K.SkyMid = 0x46285a;
		K.SkyLow = 0x8c4458;
		K.Glow = IgTang;
		break;
	case site::show::Mood::Daytime:
		K.Label = "DAYTIME";
		K.BandA = IgCoral;
		K.BandB = IgSky;
		K.SkyTop = 0x221a44;
		K.SkyMid = 0x42286a;
		K.SkyLow = 0x7a3866;
		K.Glow = IgCoral;
		break;
	case site::show::Mood::PrimeTime:
		K.Label = "PRIME TIME";
		K.BandA = IgPink;
		K.BandB = IgViolet;
		K.SkyTop = 0x12081f;
		K.SkyMid = 0x26103d;
		K.SkyLow = 0x4a1650;
		K.Glow = IgPink;
		break;
	}
	if (K.Night)
	{
		// The mockup's swap (ignite_afterburner.py): the coral family goes violet, sky goes teal, cream goes lilac.
		K.Coral = IgNightAccent;
		K.CoralHi = 0xc08cff;
		K.CoralDeep = 0x6b3fd1;
		K.CoralLight = 0xe2c8ff;
		K.CoralDark = 0x7a4ae0;
		K.Sky = IgNightAccent2;
		K.Cream = 0xf3eaff;
		K.Plum = 0x120c1f;
		K.Panel = 0x1a1230;
		K.PanelTop = 0x1e1636;
		K.PanelBot = 0x160f2a;
		K.Well = 0x1c1434;
		K.Rim = 0x3e2d6e;
		K.RimHi = 0x4d3a86;
		K.CabTop = 0x2e2260;
		K.CabBot = 0x1c1438;
	}
	else
	{
		K.Coral = IgCoral;
		K.CoralHi = 0xff7a5c;
		K.CoralDeep = 0xc73a5a;
		K.CoralLight = 0xffb08f;
		K.CoralDark = 0xd23d5f;
		K.Sky = IgSky;
		K.Cream = IgCream;
		K.Plum = IgPlum;
		K.Panel = 0x241532;
		K.PanelTop = 0x2c1a3f;
		K.PanelBot = 0x211330;
		K.Well = 0x24163a;
		K.Rim = 0x4b2d72;
		K.RimHi = 0x5a3a86;
		K.CabTop = 0x43256a;
		K.CabBot = 0x2a1645;
	}
	return K;
}

double IgniteBpmAt(double World)
{
	return IgniteTonesFor(site::show::IgniteMood(World)).Bpm;
}

// ------------------------------------------------------------------ type

TextStyle IgniteBi(float Size, const Color& Col, Align A, Baseline B, float MaxW)
{
	TextStyle St = Ts(Size, 900, Col, A, B, false, MaxW);
	St.Face = static_cast<int8_t>(Font::BlackItalic);
	return St;
}

TextStyle IgniteTx(float Size, int Weight, const Color& Col, Align A, Baseline B, float MaxW)
{
	return Ts(Size, Weight, Col, A, B, false, MaxW);
}

float IgniteCaps(Canvas& C, const std::string& Text, float X, float Y, float Size, int Weight, const Color& Col, float Track, Align A)
{
	if (Track <= 0.0f)
	{
		return C.Text(Text, X, Y, Ts(Size, Weight, Col, A));
	}
	std::vector<std::string> Glyphs;
	for (size_t I = 0; I < Text.size();)
	{
		const unsigned char Lead = static_cast<unsigned char>(Text[I]);
		const size_t Len = Lead >= 0xF0 ? 4 : Lead >= 0xE0 ? 3 : Lead >= 0xC0 ? 2 : 1;
		Glyphs.push_back(Text.substr(I, Len));
		I += Len;
	}
	float Total = 0.0f;
	for (const std::string& G : Glyphs)
	{
		Total += C.Measure(G, Size, Weight) + Track;
	}
	Total -= Glyphs.empty() ? 0.0f : Track;
	float Xx = X - (A == Align::Center ? Total / 2.0f : A == Align::Right ? Total : 0.0f);
	for (const std::string& G : Glyphs)
	{
		C.Text(G, Xx, Y, Ts(Size, Weight, Col));
		Xx += C.Measure(G, Size, Weight) + Track;
	}
	return Total;
}

Color IgniteA(const Color& C, float A)
{
	return {C.R, C.G, C.B, C.A * A};
}

// ------------------------------------------------------------------ the kit

void IgniteSpark(Canvas& C, float Cx, float Cy, float R, const Color& Col, float Thin)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 8; ++I)
	{
		const float A = (-90.0f + static_cast<float>(I) * 45.0f) * Pi / 180.0f;
		const float Rr = I % 2 == 0 ? R : R * Thin;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	C.FillPolygon(P, Col);
}

void IgniteFlame(Canvas& C, float X, float BaseY, float S, const Color& Outer, const Color& Mid, const Color& Core)
{
	const int Steps = S < 14.0f ? 3 : S < 30.0f ? 4 : 6;
	C.FillPolygon(IgniteFlattened(IgniteFlameOuter, X, BaseY, S, Steps), Outer);
	C.FillPolygon(IgniteFlattened(IgniteFlameInner, X, BaseY - S * 0.02f, S * 1.05f, Steps), Mid);
	C.FillPolygon(IgniteFlattened(IgniteFlameInner, X + S * 0.02f, BaseY - S * 0.03f, S * 0.62f, Steps), Core);
}

std::vector<Vec2> IgniteStar(float Cx, float Cy, float Ro, float Ri, int N, float RotDeg)
{
	std::vector<Vec2> P;
	for (int I = 0; I < N * 2; ++I)
	{
		const float A = (RotDeg + static_cast<float>(I) * 180.0f / static_cast<float>(N)) * Pi / 180.0f;
		const float R = I % 2 == 0 ? Ro : Ri;
		P.push_back({Cx + R * std::cos(A), Cy + R * std::sin(A)});
	}
	return P;
}

namespace
{
/** A sticker drawn round the origin (IgniteSticker places it). */
void IgniteStickerShape(Canvas& C, float R, const std::string& Label, const Color& Fill, const Color& Ink, float Size, int N, float Ri)
{
	if (R >= 24.0f)
	{
		// The drop shadow only under the big ones (a floor of small stickers would spend the page's vertices on it).
		C.FillPolygon(IgniteStar(0.0f, 2.0f, R + 3.0f, (R + 3.0f) * Ri, N), Rgba(0, 0, 0, 0.35f));
	}
	C.FillPolygon(IgniteStar(0.0f, 0.0f, R + 3.0f, (R + 3.0f) * Ri, N), Hex(IgCream));
	C.FillPolygon(IgniteStar(0.0f, 0.0f, R, R * Ri, N), Fill);
	std::vector<std::string> Lines;
	size_t Start = 0;
	while (Start <= Label.size())
	{
		const size_t End = std::min(Label.find('\n', Start), Label.size());
		Lines.push_back(Label.substr(Start, End - Start));
		Start = End + 1;
	}
	const float N0 = static_cast<float>(Lines.size());
	for (size_t I = 0; I < Lines.size(); ++I)
	{
		const float Y = Size * 0.36f + (static_cast<float>(I) - (N0 - 1.0f) / 2.0f) * Size * 1.05f;
		C.Text(Lines[I], 0.0f, Y, IgniteBi(Size, Ink, Align::Center));
	}
}
} // namespace

void IgniteSticker(Canvas& C, float Cx, float Cy, float R, const std::string& Label, const Color& Fill, const Color& Ink, float Size, int N, float Ri, float Scale)
{
	if (Scale <= 0.01f)
	{
		return;
	}
	if (Scale == 1.0f)
	{
		// A sticker at rest is the same wherever it sits: its concave stars replay from the cache (G1_AUDIT M3: a LATE
		// cabinet's sticker cost the floor more than the rest of the cabinet). One popping in scales, and draws.
		const auto Bits = [](float V) {
			uint32_t U = 0;
			std::memcpy(&U, &V, sizeof(U));
			return static_cast<uint64_t>(U);
		};
		uint64_t Key = static_cast<uint64_t>(Fnv1a("ignite:sticker|" + Label)) << 32;
		for (const float V : {R, Fill.R, Fill.G, Fill.B, Fill.A, Ink.R, Ink.G, Ink.B, Ink.A, Size, Ri, static_cast<float>(N)})
		{
			Key = (Key ^ Bits(V)) * 0x100000001b3ull;
		}
		C.Cached(Key, Cx, Cy, [R, &Label, &Fill, &Ink, Size, N, Ri](Canvas& At) { IgniteStickerShape(At, R, Label, Fill, Ink, Size, N, Ri); });
		return;
	}
	C.Save();
	C.Translate(Cx, Cy);
	C.Scale(Scale, Scale);
	IgniteStickerShape(C, R, Label, Fill, Ink, Size, N, Ri);
	C.Restore();
}

void IgniteCandyChip(Canvas& C, float X, float Y, float R, int Kind, float Alpha)
{
	// The table's candy chip in flight (SPEC §2.5 "Candy"): its ground and its six edge stripes, no shadow (it's in the air),
	// replayed from the cache by kind and size wherever it flies. G1_AUDIT N3: a 12-point ground and six quads for the stripes
	// (79 vertices; a dashed stroke round a full-resolution ellipse was 143), so the table's busiest frame (the fountain under the
	// confetti, with a full chat) stays in its 22k. Small and spinning past, the chip reads the same.
	static const uint32_t Pairs[6][2] = {{IgSun, 0x5a3a00}, {IgPink, IgCream}, {IgViolet, IgCream}, {IgSky, IgCream}, {IgLime, 0x2a3a10}, {IgCoral, IgCream}};
	const uint32_t* Pair = Pairs[((Kind % 6) + 6) % 6];
	const float A0 = C.GetAlpha();
	C.SetAlpha(A0 * Alpha);
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:candy-chip-12")) << 24) ^ (static_cast<uint64_t>(Pair[0]) ^ static_cast<uint64_t>(std::lround(R * 16.0f)) << 32), X, Y, [Pair, R](Canvas& At) {
		const float Ry = R * 0.55f;
		std::vector<Vec2> Ground;
		for (int I = 0; I < 12; ++I)
		{
			const float A = static_cast<float>(I) * Pi / 6.0f;
			Ground.push_back({R * std::cos(A), Ry * std::sin(A)});
		}
		At.FillPolygon(Ground, Hex(Pair[0]));
		for (int K = 0; K < 6; ++K)
		{
			// A stripe: 20 degrees of the rim, from the edge to 0.7 of the way out.
			const float Mid = (15.0f + 60.0f * static_cast<float>(K)) * Pi / 180.0f;
			const float Half = 10.0f * Pi / 180.0f;
			const float A0s = Mid - Half;
			const float A1s = Mid + Half;
			At.FillPolygon({{R * std::cos(A0s), Ry * std::sin(A0s)}, {R * std::cos(A1s), Ry * std::sin(A1s)}, {0.7f * R * std::cos(A1s), 0.7f * Ry * std::sin(A1s)},
							   {0.7f * R * std::cos(A0s), 0.7f * Ry * std::sin(A0s)}},
				Hex(Pair[1]));
		}
	});
	C.SetAlpha(A0);
}

void IgniteHexagon(Canvas& C, float Cx, float Cy, float R, const Paint& Fill)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 6; ++I)
	{
		const float A = (30.0f + 60.0f * static_cast<float>(I)) * Pi / 180.0f;
		P.push_back({Cx + R * std::cos(A), Cy + R * std::sin(A)});
	}
	C.FillPolygon(P, Fill);
}

void IgnitePill(Canvas& C, const Rect& R, const Paint& Fill, const Color& Rim, float RimW)
{
	C.FillRoundRect(R, R.H / 2.0f, Fill);
	if (RimW > 0.0f)
	{
		C.StrokeRoundRect(R, R.H / 2.0f, Rim, RimW);
	}
}

void IgniteTicket(Canvas& C, float X, float Y, float W, float H, const std::string& Top, const std::string& Bottom, const Color& Col, const Color& Ink)
{
	const float R = H * 0.18f;
	const float Cy = Y + H / 2.0f;
	const int Steps = 6;
	std::vector<Vec2> P;
	P.push_back({X, Y});
	P.push_back({X + W, Y});
	for (int K = 0; K <= Steps; ++K)
	{
		// The right notch, bitten in: from its top round its leftmost point to its bottom.
		const float A = -Pi / 2.0f - Pi * static_cast<float>(K) / static_cast<float>(Steps);
		P.push_back({X + W + R * std::cos(A), Cy + R * std::sin(A)});
	}
	P.push_back({X + W, Y + H});
	P.push_back({X, Y + H});
	for (int K = 0; K <= Steps; ++K)
	{
		const float A = Pi / 2.0f - Pi * static_cast<float>(K) / static_cast<float>(Steps);
		P.push_back({X + R * std::cos(A), Cy + R * std::sin(A)});
	}
	C.FillPolygon(P, Col);
	// The perforation (a faint line: a dotted one would cost a quad a dot), the face and the stub.
	C.FillRect({X + W * 0.72f - 0.5f, Y + 3.0f, 1.0f, H - 6.0f}, IgniteA(Ink, 0.3f));
	C.Text(Top, X + W * 0.36f, Y + H * 0.48f, IgniteBi(H * 0.32f, Ink, Align::Center, Baseline::Alphabetic, W * 0.68f));
	if (!Bottom.empty())
	{
		C.Text(Bottom, X + W * 0.36f, Y + H * 0.82f, Ts(H * 0.2f, 800, Ink, Align::Center, Baseline::Alphabetic, false, W * 0.68f));
	}
	IgniteSpark(C, X + W * 0.86f, Cy, H * 0.16f, IgniteA(Ink, 0.75f));
}

void IgniteBulb(Canvas& C, float X, float Y, float R, const Color& On, const Color& Off, float Lit)
{
	// A lit bulb: its halo (one translucent octagon) and the bulb; an unlit one is a dim pink pearl. Cheap on purpose: the
	// marquee holds 66 of them over every page, the table's included (22k vertices, SPEC §10), and the bandstand 23; at
	// r 3 a hexagon reads as round, and costs half a tessellated disc.
	const auto Gon = [X, Y](float Rr, int N) {
		std::vector<Vec2> P;
		P.reserve(static_cast<size_t>(N));
		for (int I = 0; I < N; ++I)
		{
			const float A = static_cast<float>(I) * 6.28318530f / static_cast<float>(N);
			P.push_back({X + Rr * std::cos(A), Y + Rr * std::sin(A)});
		}
		return P;
	};
	if (Lit > 0.3f)
	{
		C.FillPolygon(Gon(R * 2.2f, 8), IgniteA(On, 0.2f * Lit));
		C.FillPolygon(Gon(R, 6), Mix(Off, On, Lit));
	}
	else
	{
		C.FillPolygon(Gon(R * 0.95f, 6), IgniteA(Off, 0.35f + Lit));
	}
}

void IgniteBell(Canvas& C, float Cx, float Cy, float S, const Color& Col)
{
	const float K = S / 40.0f;
	std::vector<Vec2> P;
	// A bell: the crown, the shoulders flaring to the lip.
	for (int I = 0; I <= 10; ++I)
	{
		const float T = static_cast<float>(I) / 10.0f;
		const float A = Pi + Pi * T; // the dome's top half
		P.push_back({Cx + 11.0f * K * std::cos(A), Cy - 4.0f * K + 11.0f * K * std::sin(A)});
	}
	P.push_back({Cx + 12.0f * K, Cy + 6.0f * K});
	P.push_back({Cx + 15.0f * K, Cy + 10.0f * K});
	P.push_back({Cx - 15.0f * K, Cy + 10.0f * K});
	P.push_back({Cx - 12.0f * K, Cy + 6.0f * K});
	C.FillPolygon(P, Col);
	C.FillCircle(Cx, Cy + 13.0f * K, 3.5f * K, Col);
}

void IgniteDockIcon(Canvas& C, int Kind, float Cx, float Cy, const Color& Col, const Color& Cut)
{
	switch (Kind)
	{
	case 0: // PLAY
		C.FillPolygon({{Cx - 7.0f, Cy - 9.0f}, {Cx + 10.0f, Cy}, {Cx - 7.0f, Cy + 9.0f}}, Col);
		break;
	case 1: // REWARDS: a gift
		C.FillRoundRect({Cx - 10.0f, Cy - 3.0f, 20.0f, 13.0f}, 2.0f, Col);
		C.FillRoundRect({Cx - 12.0f, Cy - 8.0f, 24.0f, 6.0f}, 2.0f, Col);
		C.FillRect({Cx - 2.0f, Cy - 8.0f, 4.0f, 18.0f}, IgniteA(Cut, 0.6f));
		C.StrokeArc(Cx - 4.0f, Cy - 10.0f, 4.0f, 0.0f, 2.0f * Pi, Col, 2.0f);
		C.StrokeArc(Cx + 4.0f, Cy - 10.0f, 4.0f, 0.0f, 2.0f * Pi, Col, 2.0f);
		break;
	case 2: // BOARDS: a trophy
		C.FillPolygon({{Cx - 10.0f, Cy - 11.0f}, {Cx + 10.0f, Cy - 11.0f}, {Cx + 10.0f, Cy - 5.0f}, {Cx + 6.0f, Cy + 2.0f}, {Cx, Cy + 4.0f}, {Cx - 6.0f, Cy + 2.0f}, {Cx - 10.0f, Cy - 5.0f}}, Col);
		C.FillRect({Cx - 2.0f, Cy + 4.0f, 4.0f, 6.0f}, Col);
		C.FillRoundRect({Cx - 7.0f, Cy + 9.0f, 14.0f, 3.0f}, 1.0f, Col);
		C.StrokeArc(Cx - 10.0f, Cy - 6.0f, 4.0f, Pi * 0.5f, Pi * 1.5f, Col, 2.0f);
		C.StrokeArc(Cx + 10.0f, Cy - 6.0f, 4.0f, -Pi * 0.5f, Pi * 0.5f, Col, 2.0f);
		break;
	case 3: // WALLET
		C.FillRoundRect({Cx - 12.0f, Cy - 8.0f, 24.0f, 17.0f}, 3.0f, Col);
		C.FillRoundRect({Cx + 2.0f, Cy - 3.0f, 12.0f, 8.0f}, 3.0f, IgniteA(Cut, 0.7f));
		C.FillCircle(Cx + 7.0f, Cy + 1.0f, 1.8f, Col);
		break;
	default: // FLIP: a coin
		C.FillCircle(Cx, Cy, 12.0f, Col);
		C.StrokeArc(Cx, Cy, 8.5f, 0.0f, 2.0f * Pi, IgniteA(Cut, 0.8f), 2.0f);
		break;
	}
}

void IgniteClock(Canvas& C, float Cx, float Cy, float R, const Color& Col, double Minutes)
{
	C.StrokeArc(Cx, Cy, R, 0.0f, 2.0f * Pi, Col, std::max(1.2f, R * 0.18f));
	const double M = std::fmod(Minutes, 720.0);
	const float Ha = IgniteF(M / 720.0 * 2.0 * 3.14159265358979 - 3.14159265358979 / 2.0);
	const float Ma = IgniteF(std::fmod(Minutes, 60.0) / 60.0 * 2.0 * 3.14159265358979 - 3.14159265358979 / 2.0);
	C.StrokePolyline({{Cx, Cy}, {Cx + R * 0.5f * std::cos(Ha), Cy + R * 0.5f * std::sin(Ha)}}, false, Col, std::max(1.2f, R * 0.16f), true);
	C.StrokePolyline({{Cx, Cy}, {Cx + R * 0.75f * std::cos(Ma), Cy + R * 0.75f * std::sin(Ma)}}, false, Col, std::max(1.0f, R * 0.12f), true);
}

void IgniteFuelGauge(Canvas& C, float Cx, float Cy, float R, double T, const IgniteTones& K)
{
	const float Tt = IgniteF(Clamp01(T));
	const float W = std::max(4.0f, R * 0.16f);
	// The dial: dim track, then the lit share from empty (left) toward full (right) in tangerine to lime.
	C.StrokeArc(Cx, Cy, R, Pi, 2.0f * Pi, Hex(K.Rim), W, true);
	if (Tt > 0.005f)
	{
		const int Parts = 6;
		for (int I = 0; I < Parts; ++I)
		{
			const float A0 = Pi + Pi * Tt * static_cast<float>(I) / static_cast<float>(Parts);
			const float A1 = Pi + Pi * Tt * static_cast<float>(I + 1) / static_cast<float>(Parts);
			const float Mid = Tt * (static_cast<float>(I) + 0.5f) / static_cast<float>(Parts);
			C.StrokeArc(Cx, Cy, R, A0, A1 + 0.01f, Mix(Hex(IgTang), Hex(IgLime), Mid), W, I == 0);
		}
	}
	// Ticks every eighth, a longer one at the half.
	for (int I = 0; I <= 8; ++I)
	{
		const float A = Pi + Pi * static_cast<float>(I) / 8.0f;
		const float In = I % 4 == 0 ? R - W * 2.4f : R - W * 1.7f;
		C.StrokePolyline({{Cx + In * std::cos(A), Cy + In * std::sin(A)}, {Cx + (R - W) * std::cos(A), Cy + (R - W) * std::sin(A)}}, false, IgniteA(Hex(K.Cream), 0.5f), 1.4f);
	}
	// The needle.
	const float Na = Pi + Pi * Tt;
	C.StrokePolyline({{Cx, Cy}, {Cx + (R - W * 2.0f) * std::cos(Na), Cy + (R - W * 2.0f) * std::sin(Na)}}, false, Hex(K.Coral), std::max(2.0f, R * 0.07f), true);
	C.FillCircle(Cx, Cy, std::max(3.5f, R * 0.12f), Hex(K.Cream));
	C.FillCircle(Cx, Cy, std::max(1.6f, R * 0.05f), Hex(K.Coral));
}

void IgniteFuseRope(Canvas& C, float X0, float X1, float Y, float Amp, double T, const Color& Rope, const Color& Burnt, double Now, bool Spark)
{
	const float Tt = IgniteF(Clamp01(T));
	std::vector<Vec2> Pts;
	const int N = std::max(8, static_cast<int>((X1 - X0) / 6.0f));
	for (int I = 0; I <= N; ++I)
	{
		const float U = static_cast<float>(I) / static_cast<float>(N);
		Pts.push_back({X0 + (X1 - X0) * U, Y + Amp * std::sin(U * 14.0f)});
	}
	const size_t Head = static_cast<size_t>(std::lround(Tt * static_cast<float>(N)));
	if (Head > 0)
	{
		std::vector<Vec2> Done(Pts.begin(), Pts.begin() + static_cast<std::ptrdiff_t>(std::min(Head + 1, Pts.size())));
		C.StrokePolyline(Done, false, Burnt, 3.0f, true);
	}
	if (Head < Pts.size())
	{
		std::vector<Vec2> Left(Pts.begin() + static_cast<std::ptrdiff_t>(Head), Pts.end());
		if (Left.size() >= 2)
		{
			C.StrokePolyline(Left, false, Rope, 3.0f, true);
		}
	}
	if (Spark && Head < Pts.size())
	{
		IgniteFuseSpark(C, X0, X1, Y, Amp, T, Now);
	}
}

int IgniteFuseHead(float X0, float X1, double T)
{
	const int N = std::max(8, static_cast<int>((X1 - X0) / 6.0f));
	return static_cast<int>(std::lround(IgniteF(Clamp01(T)) * static_cast<float>(N)));
}

void IgniteFuseSpark(Canvas& C, float X0, float X1, float Y, float Amp, double T, double Now)
{
	const int N = std::max(8, static_cast<int>((X1 - X0) / 6.0f));
	const int Head = IgniteFuseHead(X0, X1, T);
	if (Head > N)
	{
		return;
	}
	const float U = static_cast<float>(Head) / static_cast<float>(N);
	const Vec2 P{X0 + (X1 - X0) * U, Y + Amp * std::sin(U * 14.0f)};
	const float Flick = 0.8f + 0.2f * IgniteF(std::sin(Now * 23.0));
	C.GlowRoundRect({P.X - 10.0f, P.Y - 10.0f, 20.0f, 20.0f}, 10.0f, IgniteA(Hex(IgTang), 0.45f * Flick), 9.0f);
	IgniteSpark(C, P.X, P.Y, 8.0f * Flick, Hex(IgSun));
	IgniteSpark(C, P.X, P.Y, 3.5f, Hex(0xffffff));
	for (int K = 0; K < 6; ++K)
	{
		const double A = IgniteHashUnit(static_cast<uint32_t>(K), 77u) * 6.283 + Now * 3.0;
		const double D = 7.0 + 9.0 * std::fmod(IgniteHashUnit(static_cast<uint32_t>(K), 78u) + Now * 1.7, 1.0);
		C.FillRect({P.X + IgniteF(D * std::cos(A)) - 1.0f, P.Y + IgniteF(D * std::sin(A)) - 1.0f, 2.0f, 2.0f}, IgniteA(Hex(IgSun), 0.8f));
	}
}

void IgniteNeonRoundRect(Canvas& C, const Rect& R, float Radius, const Color& Col, float Width, float Bloom)
{
	if (Bloom > 0.0f)
	{
		C.StrokeRoundRect(R, Radius, IgniteA(Col, 0.18f), Width + Bloom);
		C.StrokeRoundRect(R, Radius, IgniteA(Col, 0.3f), Width + Bloom * 0.45f);
	}
	C.StrokeRoundRect(R, Radius, Col, Width);
}

// ------------------------------------------------------------------ the Boardwalk

namespace
{
/** The water's sway in steps a turn (its strip replays from the Boardwalk pool at each). */
constexpr int IgniteWaterSteps = 64;

/** The bandstand's arch: a cubic from (190,170) up to (622,170) through (190,96) and (622,96). */
Vec2 IgniteArch(float T)
{
	const float U = 1.0f - T;
	const float X = U * U * U * 190.0f + 3.0f * U * U * T * 190.0f + 3.0f * U * T * T * 622.0f + T * T * T * 622.0f;
	const float Y = U * U * U * 170.0f + 3.0f * U * U * T * 96.0f + 3.0f * U * T * T * 96.0f + T * T * T * 170.0f;
	return {X, Y};
}

std::vector<Vec2> IgniteArchShape()
{
	std::vector<Vec2> P;
	P.push_back({190.0f, 300.0f});
	for (int I = 0; I <= 24; ++I)
	{
		P.push_back(IgniteArch(static_cast<float>(I) / 24.0f));
	}
	P.push_back({622.0f, 300.0f});
	return P;
}

void IgniteStringLights(Canvas& C, float X0, float Y0, float X1, float Y1, uint32_t Seed)
{
	const float Mx = (X0 + X1) / 2.0f;
	const float My = std::max(Y0, Y1) + 18.0f;
	std::vector<Vec2> Wire;
	for (int I = 0; I <= 10; ++I)
	{
		const float T = static_cast<float>(I) / 10.0f;
		Wire.push_back({(1 - T) * (1 - T) * X0 + 2 * (1 - T) * T * Mx + T * T * X1, (1 - T) * (1 - T) * Y0 + 2 * (1 - T) * T * My + T * T * Y1});
	}
	C.StrokePolyline(Wire, false, Hex(0x5a4680), 1.0f);
	const uint32_t Cols[5] = {IgSun, IgPink, IgSky, IgLime, IgCoral};
	for (int K = 1; K < 6; ++K)
	{
		const float T = static_cast<float>(K) / 6.0f;
		const float Bx = (1 - T) * (1 - T) * X0 + 2 * (1 - T) * T * Mx + T * T * X1;
		const float By = (1 - T) * (1 - T) * Y0 + 2 * (1 - T) * T * My + T * T * Y1 + 2.0f;
		const Color Cc = Hex(Cols[(static_cast<uint32_t>(K) + Seed) % 5u]);
		C.GlowRoundRect({Bx - 4.0f, By - 4.0f, 8.0f, 8.0f}, 4.0f, IgniteA(Cc, 0.3f), 4.0f);
		C.FillCircle(Bx, By, 1.8f, Cc);
	}
}
} // namespace

void IgniteBoardwalkStatic(Canvas& C, const IgniteBand& B)
{
	const IgniteTones& K = B.K;
	// The sky, in two bands (a vertex gradient holds two stops a run), and the horizon's glow.
	C.FillRect({0.0f, 72.0f, 1600.0f, 156.0f}, Paint::Linear({0.0f, 72.0f}, {0.0f, 228.0f}, Hex(K.SkyTop), Hex(K.SkyMid)));
	C.FillRect({0.0f, 228.0f, 1600.0f, 124.0f}, Paint::Linear({0.0f, 228.0f}, {0.0f, 352.0f}, Hex(K.SkyMid), Hex(K.SkyLow)));
	C.GlowRoundRect({-80.0f, 268.0f, 1760.0f, 64.0f}, 32.0f, IgniteHexA(K.Glow, 0.2f), 46.0f);
	// Stars: many at night, a few by day.
	const bool Dark = K.Mood == site::show::Mood::Afterburner || K.Mood == site::show::Mood::PrimeTime;
	const int Stars = Dark ? 64 : 28;
	for (int I = 0; I < Stars; ++I)
	{
		const float X = IgniteF(IgniteHashUnit(static_cast<uint32_t>(I), 1u) * 1600.0);
		const float Y = 76.0f + IgniteF(IgniteHashUnit(static_cast<uint32_t>(I), 2u) * 186.0);
		const float S = 1.0f + IgniteF(IgniteHashUnit(static_cast<uint32_t>(I), 3u) * 1.6);
		C.FillRect({X, Y, S, S}, Rgba(255, 255, 255, 0.25f + 0.6f * IgniteF(IgniteHashUnit(static_cast<uint32_t>(I), 4u))));
	}
	if (K.Night)
	{
		// The moon (Afterburner Hours): a glow, then the crescent, and three stars around it.
		const float Mx = 648.0f;
		const float My = 104.0f;
		const float Mr = 19.0f;
		C.GlowRoundRect({Mx - 34.0f, My - 34.0f, 68.0f, 68.0f}, 34.0f, Hex(0xe9e2ff, 0.16f), 26.0f);
		// The crescent, lit on its left: the moon's left half from the top round to the bottom, then back up an inner arc
		// that bulges left less far (the mockup's two arcs).
		std::vector<Vec2> Moon;
		for (int I = 0; I <= 16; ++I)
		{
			const float A = -Pi / 2.0f - Pi * static_cast<float>(I) / 16.0f;
			Moon.push_back({Mx + Mr * std::cos(A), My + Mr * std::sin(A)});
		}
		for (int I = 1; I < 16; ++I)
		{
			const float T = Pi * static_cast<float>(I) / 16.0f;
			Moon.push_back({Mx - Mr * 0.72f * std::sin(T), My + Mr * std::cos(T)});
		}
		C.FillPolygon(Moon, Hex(0xf1ecff));
		IgniteSpark(C, 672.0f, 86.0f, 4.0f, Rgba(255, 255, 255, 0.8f));
		IgniteSpark(C, 628.0f, 132.0f, 5.0f, Rgba(255, 255, 255, 0.8f));
		IgniteSpark(C, 690.0f, 120.0f, 6.0f, Rgba(255, 255, 255, 0.8f));
	}
	// The town across the bay, and its windows.
	{
		std::vector<Vec2> Town;
		Town.push_back({0.0f, 300.0f});
		float X = 0.0f;
		uint32_t I = 0;
		while (X < 1600.0f)
		{
			const float W = 18.0f + IgniteF(IgniteHashUnit(I, 11u) * 46.0);
			const float H = 14.0f + IgniteF(IgniteHashUnit(I, 12u) * 46.0);
			Town.push_back({X, 300.0f - H});
			Town.push_back({std::min(1600.0f, X + W), 300.0f - H});
			X += W;
			++I;
		}
		Town.push_back({1600.0f, 300.0f});
		C.FillPolygon(Town, Hex(K.Night ? 0x160c2c : 0x1f0f33, 0.95f));
		const uint32_t Lights[4] = {IgSun, 0xffd9a8, K.Sky, IgPink};
		for (uint32_t J = 0; J < 64; ++J)
		{
			const float Wx = IgniteF(IgniteHashUnit(J, 21u) * 1600.0);
			const float Wy = 262.0f + IgniteF(IgniteHashUnit(J, 22u) * 34.0);
			C.FillRect({Wx, Wy, 2.0f, 2.0f}, Hex(Lights[J % 4u], 0.45f));
		}
	}
	// The lighthouse: its rock, the striped tower, the gallery and the lantern room (the beam is live).
	C.FillPolygon({{30.0f, 316.0f}, {44.0f, 294.0f}, {70.0f, 288.0f}, {92.0f, 286.0f}, {118.0f, 290.0f}, {140.0f, 298.0f}, {150.0f, 316.0f}}, Hex(0x2a1a2e));
	for (int Band = 0; Band < 6; ++Band)
	{
		const float Y0 = 142.0f + 158.0f * static_cast<float>(Band) / 6.0f;
		const float Y1 = 142.0f + 158.0f * static_cast<float>(Band + 1) / 6.0f;
		const auto Xl = [](float Y) { return 74.0f - (Y - 142.0f) / 158.0f * 12.0f; };
		const auto Xr = [](float Y) { return 110.0f + (Y - 142.0f) / 158.0f * 12.0f; };
		C.FillPolygon({{Xl(Y0), Y0}, {Xr(Y0), Y0}, {Xr(Y1), Y1}, {Xl(Y1), Y1}}, Band % 2 == 0 ? Hex(K.Coral) : Hex(K.Cream));
	}
	C.FillPolygon({{74.0f, 142.0f}, {78.0f, 142.0f}, {68.0f, 300.0f}, {62.0f, 300.0f}}, Rgba(0, 0, 0, 0.18f));
	C.FillRoundRect({88.0f, 180.0f, 8.0f, 12.0f}, 3.0f, Hex(0x2a1238));
	C.FillRoundRect({88.0f, 232.0f, 8.0f, 12.0f}, 3.0f, Hex(0x2a1238));
	C.FillRoundRect({68.0f, 134.0f, 48.0f, 8.0f}, 2.0f, Hex(0x3a2252));
	for (int I = 0; I < 7; ++I)
	{
		C.FillRect({69.5f + static_cast<float>(I) * 7.3f, 126.0f, 1.4f, 8.0f}, Hex(0x6a4b8c));
	}
	C.FillRect({78.0f, 106.0f, 28.0f, 22.0f}, Hex(0xffe79a));
	C.StrokeRoundRect({78.0f, 106.0f, 28.0f, 22.0f}, 0.5f, Hex(0x3a2252), 2.0f);
	C.FillRect({91.25f, 106.0f, 1.5f, 22.0f}, Hex(0x3a2252));
	C.FillPolygon({{74.0f, 106.0f}, {110.0f, 106.0f}, {92.0f, 90.0f}}, Hex(K.Coral));
	C.FillCircle(92.0f, 88.0f, 2.5f, Hex(IgSun));
	// The bandstand's shell: the dome on its stage, its neon edge, the screen the signs draw on.
	C.FillPolygon({{214.0f, 300.0f}, {150.0f, 90.0f}, {250.0f, 90.0f}}, Hex(K.Sky, 0.06f));
	C.FillPolygon({{598.0f, 300.0f}, {560.0f, 90.0f}, {680.0f, 90.0f}}, Hex(IgPink, 0.07f));
	const std::vector<Vec2> Arch = IgniteArchShape();
	C.FillPolygon(Arch, Hex(K.Night ? 0x1e1440 : 0x2a1546));
	C.StrokePolyline(Arch, false, Hex(K.Coral, 0.2f), 9.0f, true);
	C.StrokePolyline(Arch, false, Hex(K.Coral, 0.9f), 3.0f, true);
	C.FillRoundRect({212.0f, 138.0f, 388.0f, 146.0f}, 14.0f, Hex(0x160b26));
	C.FillRoundRect({212.0f, 138.0f, 388.0f, 60.0f}, 14.0f, Paint::Linear({0.0f, 138.0f}, {0.0f, 198.0f}, Hex(IgPink, 0.18f), Hex(IgPink, 0.0f)));
	C.StrokeRoundRect({212.0f, 138.0f, 388.0f, 146.0f}, 14.0f, Hex(K.Rim), 1.5f);
	C.FillRoundRect({184.0f, 286.0f, 444.0f, 14.0f}, 3.0f, Hex(0x3a2252));
	C.FillRoundRect({184.0f, 286.0f, 444.0f, 3.0f}, 1.0f, Hex(K.Coral, 0.6f));
	// The Ferris wheel's legs and the violet glow behind it (the wheel turns in the live layer).
	C.FillPolygon({{786.0f, 214.0f}, {794.0f, 214.0f}, {856.0f, 330.0f}, {842.0f, 330.0f}}, Hex(0x4a3768));
	C.FillPolygon({{786.0f, 214.0f}, {794.0f, 214.0f}, {738.0f, 330.0f}, {724.0f, 330.0f}}, Hex(0x4a3768));
	C.FillRect({752.0f, 288.0f, 76.0f, 4.0f}, Hex(0x4a3768));
	C.GlowRoundRect({670.0f, 94.0f, 240.0f, 240.0f}, 120.0f, Hex(IgViolet, 0.1f), 40.0f);
	// The Fuse Booth: a striped tent with a scalloped valance, its body, its opening.
	C.FillRect({1081.0f, 80.0f, 2.0f, 22.0f}, Hex(0xc9b8e6));
	C.FillPolygon({{1083.0f, 80.0f}, {1104.0f, 86.0f}, {1083.0f, 92.0f}}, Hex(IgPink));
	for (int I = 0; I < 12; ++I)
	{
		const float A = 956.0f + 252.0f * static_cast<float>(I) / 12.0f;
		const float Bx = 956.0f + 252.0f * static_cast<float>(I + 1) / 12.0f;
		C.FillPolygon({{1082.0f, 102.0f}, {A, 172.0f}, {Bx, 172.0f}}, I % 2 == 0 ? Hex(K.Coral) : Hex(K.Cream));
	}
	C.FillPolygon({{1082.0f, 102.0f}, {956.0f, 172.0f}, {1010.0f, 172.0f}}, Rgba(0, 0, 0, 0.12f));
	for (int I = 0; I < 12; ++I)
	{
		const float A = 956.0f + 21.0f * static_cast<float>(I);
		std::vector<Vec2> Scallop;
		for (int J = 0; J <= 6; ++J)
		{
			const float T = Pi * static_cast<float>(J) / 6.0f;
			Scallop.push_back({A + 10.5f - 10.5f * std::cos(T), 172.0f + 8.75f * std::sin(T)});
		}
		C.FillPolygon(Scallop, I % 2 == 0 ? Hex(IgPink) : Hex(K.Cream));
	}
	C.FillRect({966.0f, 176.0f, 232.0f, 122.0f}, Hex(K.Night ? 0x1f1038 : 0x2b1240));
	for (int I = 0; I < 8; ++I)
	{
		C.FillRect({966.0f + static_cast<float>(I) * 29.0f, 176.0f, 14.5f, 122.0f}, Hex(K.Coral, 0.16f));
	}
	{
		std::vector<Vec2> Door;
		Door.push_back({992.0f, 298.0f});
		Door.push_back({992.0f, 214.0f});
		for (int I = 1; I < 12; ++I)
		{
			const float T = static_cast<float>(I) / 12.0f;
			const float U = 1.0f - T;
			Door.push_back({U * U * U * 992.0f + 3 * U * U * T * 992.0f + 3 * U * T * T * 1172.0f + T * T * T * 1172.0f, U * U * U * 214.0f + 3 * U * U * T * 190.0f + 3 * U * T * T * 190.0f + T * T * T * 214.0f});
		}
		Door.push_back({1172.0f, 214.0f});
		Door.push_back({1172.0f, 298.0f});
		C.FillPolygon(Door, Hex(0x12081c));
		C.StrokePolyline(Door, false, Hex(IgSun, 0.6f), 1.5f);
	}
	// The Prize Counter: its back wall, the striped awning, the counter.
	C.FillRect({1252.0f, 132.0f, 304.0f, 166.0f}, Hex(K.Night ? 0x1c1036 : 0x24123a));
	for (int I = 0; I < 10; ++I)
	{
		const float X = 1240.0f + static_cast<float>(I) * 33.0f;
		const Color Stripe = I % 2 == 0 ? Hex(IgPink) : Hex(0x6b3fd1);
		C.FillRect({X, 108.0f, 33.0f, 24.0f}, Stripe);
		std::vector<Vec2> Scallop;
		for (int J = 0; J <= 6; ++J)
		{
			const float T = Pi * static_cast<float>(J) / 6.0f;
			Scallop.push_back({X + 16.5f - 16.5f * std::cos(T), 132.0f + 10.0f * std::sin(T)});
		}
		C.FillPolygon(Scallop, Stripe);
	}
	C.FillRect({1240.0f, 108.0f, 330.0f, 5.0f}, Rgba(255, 255, 255, 0.18f));
	C.FillRoundRect({1244.0f, 246.0f, 320.0f, 52.0f}, 6.0f, Hex(IgGrape));
	C.FillRoundRect({1244.0f, 246.0f, 320.0f, 6.0f}, 3.0f, Rgba(255, 255, 255, 0.12f));
	// String lights between the attractions.
	IgniteStringLights(C, 150.0f, 258.0f, 186.0f, 262.0f, 0u);
	IgniteStringLights(C, 626.0f, 250.0f, 708.0f, 262.0f, 1u);
	IgniteStringLights(C, 872.0f, 262.0f, 960.0f, 246.0f, 2u);
	IgniteStringLights(C, 1200.0f, 246.0f, 1246.0f, 254.0f, 3u);
	// The railing, the deck's planks and fascia, the water and its pilings.
	C.FillRect({0.0f, 299.0f, 1600.0f, 2.0f}, Hex(0x5b3446));
	for (int I = 0; I < 1600; I += 64)
	{
		C.FillRect({static_cast<float>(I) + 6.75f, 300.0f, 2.5f, 14.0f}, Hex(0x5b3446));
	}
	C.FillRect({0.0f, 312.0f, 1600.0f, 8.0f}, Paint::Linear({0.0f, 312.0f}, {0.0f, 320.0f}, Hex(0x7a4634), Hex(0x5a3024)));
	C.FillRect({0.0f, 320.0f, 1600.0f, 9.0f}, Hex(0x3d2018));
	C.FillRect({0.0f, 320.0f, 1600.0f, 1.5f}, Hex(0xffb08f, 0.25f));
	C.FillRect({0.0f, 329.0f, 1600.0f, 23.0f}, Paint::Linear({0.0f, 329.0f}, {0.0f, 352.0f}, Hex(K.Night ? 0x140c30 : 0x1b1035), Hex(0x0f0920)));
	for (int I = 0; I < 1600; I += 64)
	{
		C.FillRect({static_cast<float>(I) + 20.0f, 329.0f, 6.0f, 23.0f}, Hex(0x2a1a1e, 0.9f));
	}
	C.FillRect({0.0f, 351.0f, 1600.0f, 1.0f}, Rgba(0, 0, 0, 0.6f));
}

void IgniteBoardwalkLive(Canvas& C, const IgniteBand& B)
{
	const IgniteTones& K = B.K;
	const double Now = B.Now;
	// The lighthouse's beam sweeps every 6 s: it swings out over the bay and back, brightest at its widest.
	{
		const double Phase = std::fmod(Now / 6.0, 1.0);
		const float Swing = IgniteF(std::sin(Phase * 2.0 * 3.14159265358979));
		const float Reach = 380.0f + 170.0f * Swing;
		const float A = 0.14f + 0.12f * std::max(0.0f, Swing);
		C.FillPolygon({{92.0f, 117.0f}, {92.0f + Reach, 64.0f + 6.0f * Swing}, {92.0f + Reach, 186.0f - 6.0f * Swing}}, Paint::Linear({92.0f, 0.0f}, {92.0f + Reach, 0.0f}, Hex(0xfff4c2, A), Hex(0xfff4c2, 0.0f)));
		C.GlowRoundRect({70.0f, 100.0f, 44.0f, 40.0f}, 20.0f, Hex(IgSun, 0.3f), 14.0f);
	}
	// The bandstand's bulbs, two of three lit and stepping on the beat. Each is on or off, so the row is one of the chase's
	// 23 steps (or all dark for the first moment of a beat): it replays from the Boardwalk pool by which bulbs are lit
	// (G1_AUDIT M3).
	{
		uint32_t Lit = 0;
		for (int I = 0; I < 23; ++I)
		{
			Lit |= motion::Chase(Now, I, 23, K.Bpm) > 0.3 ? 1u << I : 0u;
		}
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:bandstand-bulbs")) << 24) ^ Lit, 0.0f, 0.0f, [Lit](Canvas& At) {
			for (int I = 0; I < 23; ++I)
			{
				const Vec2 P = IgniteArch(static_cast<float>(I) / 22.0f);
				IgniteBulb(At, P.X, P.Y - 2.0f, 2.6f, Hex(IgSun), Hex(0x7a3060), (Lit >> I) & 1u ? 1.0f : 0.0f);
			}
		}, Canvas::CachePool::Boardwalk);
	}
	// The water: each sign's light trembling on the bay. Every line sways on one phase (Now x 1.3), so the strip replays
	// from the Boardwalk pool at 64 steps of it: a line lands within 0.4 units of where it would (G1_AUDIT M3).
	{
		const double Turn = std::fmod(Now * 1.3, 2.0 * 3.14159265358979);
		const int Step = static_cast<int>(std::lround(Turn / (2.0 * 3.14159265358979) * IgniteWaterSteps)) % IgniteWaterSteps;
		const uint32_t Lights[7] = {IgSun, IgPink, IgLime, K.Coral, IgSun, K.Sky, IgPink};
		const uint64_t Key = (static_cast<uint64_t>(Fnv1a("ignite:water")) << 32) ^ (static_cast<uint64_t>(K.Coral) << 8) ^ (static_cast<uint64_t>(K.Sky) << 16) ^ static_cast<uint64_t>(Step);
		C.Cached(Key, 0.0f, 0.0f, [&Lights, Step](Canvas& At) {
			const float Xs[7] = {92.0f, 406.0f, 790.0f, 1082.0f, 1405.0f, 300.0f, 1500.0f};
			const double Phase = static_cast<double>(Step) / IgniteWaterSteps * 2.0 * 3.14159265358979;
			for (int I = 0; I < 7; ++I)
			{
				for (int J = 0; J < 4; ++J)
				{
					const float W = 20.0f + IgniteF(IgniteHashUnit(static_cast<uint32_t>(I * 8 + J), 31u) * 50.0);
					const float Drift = IgniteF(std::sin(Phase + static_cast<double>(I * 3 + J)) * 8.0);
					const float X = Xs[I] - W / 2.0f + IgniteF((IgniteHashUnit(static_cast<uint32_t>(I * 8 + J), 32u) - 0.5) * 40.0) + Drift;
					At.FillRect({X, 333.0f + static_cast<float>(J) * 4.4f, W, 1.4f}, Hex(Lights[I], 0.28f - static_cast<float>(J) * 0.05f));
				}
			}
		}, Canvas::CachePool::Boardwalk);
	}
	// Snow, December to February: flakes on keyed paths, at most 120.
	if (B.Snow)
	{
		for (int I = 0; I < 110; ++I)
		{
			const double Speed = 14.0 + 16.0 * IgniteHashUnit(static_cast<uint32_t>(I), 41u);
			const double Y = std::fmod(IgniteHashUnit(static_cast<uint32_t>(I), 42u) * 280.0 + Now * Speed, 280.0);
			const double X = IgniteHashUnit(static_cast<uint32_t>(I), 43u) * 1600.0 + std::sin(Now * 0.8 + static_cast<double>(I)) * 6.0;
			const float S = 1.4f + IgniteF(IgniteHashUnit(static_cast<uint32_t>(I), 44u) * 1.4);
			C.FillRect({IgniteF(X), 72.0f + IgniteF(Y), S, S}, Rgba(255, 255, 255, 0.75f));
		}
	}
	// Fireworks over the water every 20 s while the Sunday Blaze runs.
	if (B.Fireworks)
	{
		const double Cycle = std::fmod(Now, 20.0);
		for (int Shell = 0; Shell < 2; ++Shell)
		{
			const double T = Cycle - static_cast<double>(Shell) * 0.8;
			if (T < 0.0 || T > 2.4)
			{
				continue;
			}
			const uint32_t Burst = static_cast<uint32_t>(std::floor(Now / 20.0)) * 2u + static_cast<uint32_t>(Shell);
			const float Cx = 300.0f + IgniteF(IgniteHashUnit(Burst, 51u) * 1000.0);
			const float Cy = 110.0f + IgniteF(IgniteHashUnit(Burst, 52u) * 70.0);
			const uint32_t Cols[4] = {IgPink, IgSun, IgLime, K.Sky};
			const Color Col = Hex(Cols[Burst % 4u]);
			const float Rr = IgniteF(std::min(1.0, T / 0.7) * 46.0);
			const float A = IgniteF(std::max(0.0, 1.0 - T / 2.4));
			for (int Ray = 0; Ray < 14; ++Ray)
			{
				const float Ang = static_cast<float>(Ray) * 2.0f * Pi / 14.0f;
				const float Drop = IgniteF(T * T * 6.0);
				C.FillRect({Cx + Rr * std::cos(Ang) - 1.5f, Cy + Rr * std::sin(Ang) + Drop - 1.5f, 3.0f, 3.0f}, IgniteA(Col, A));
			}
		}
	}
	// The series (Wildfire): bunting along the railing and bonfires on the deck.
	if (B.Wildfire)
	{
		const uint32_t Cols[3] = {IgTang, IgPink, IgSun};
		for (int I = 0; I < 40; ++I)
		{
			const float X = 10.0f + static_cast<float>(I) * 40.0f;
			C.FillPolygon({{X, 298.0f}, {X + 18.0f, 298.0f}, {X + 9.0f, 310.0f}}, Hex(Cols[I % 3], 0.9f));
		}
		for (int I = 0; I < 4; ++I)
		{
			const float X = 170.0f + static_cast<float>(I) * 420.0f;
			const float Flick = 1.0f + 0.12f * IgniteF(std::sin(Now * 9.0 + static_cast<double>(I)));
			IgniteFlame(C, X, 312.0f, 11.0f * Flick, Hex(IgTang), Hex(IgSun), Hex(IgCream));
		}
	}
}

IgnitePlacard IgnitePlacardAt(float Cx, float Cy, float R, float Angle, size_t L, float W, double Now)
{
	IgnitePlacard P;
	// Every fifth car, so the three ride apart round the wheel.
	const int I = static_cast<int>(L) * 5 + 1;
	const float A = Angle + static_cast<float>(I) * Pi / 8.0f;
	const float Px = Cx + R * std::cos(A);
	const float Py = Cy + R * std::sin(A) + 18.0f;
	const float Bob = IgniteF(std::sin(Now * 1.6 + static_cast<double>(L)) * 1.5);
	P.Car = I;
	P.Tag = {Px - W / 2.0f, Py + Bob, W, 18.0f};
	// A placard tucks away as its car swings down through the wheel's foot, and is gone before its rim (1.2 out) would reach
	// the wheel's sign: it fades over the 22 units above the sign's line, never under it (G1_AUDIT L6). Upright text can't
	// ride past the sign, so it waits for its car to come back up the other side.
	const float Clear = IgniteWheelSign.Y - IgnitePlacardGap - (P.Tag.Y + P.Tag.H + 1.2f);
	P.Show = IgniteF(Clamp01(static_cast<double>(Clear) / 22.0));
	return P;
}

void IgniteFerrisWheel(Canvas& C, float Cx, float Cy, float R, float Angle, const std::vector<std::string>& Labels, const IgniteTones& K, double Now)
{
	// Only the spokes turn; the rings, the hub and the cars (upright, so a car is the same shape wherever it hangs) replay
	// from the cache (G1_AUDIT M3: the wheel was PLAY's largest live drawing).
	const auto Pos = [](float V) { return static_cast<uint64_t>(std::lround(V * 4.0f)) & 0xffffu; };
	const uint64_t At = (Pos(Cx) << 32) ^ (Pos(Cy) << 16) ^ Pos(R);
	// The wheel's inner ring (unlit at G1: GLOW, SHINE and DAZZLE light it from G2) and its rim. The spokes draw over the rim
	// now, a 1.5-unit tip on a 2.5-unit line in a darker violet: not a pixel that reads.
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:wheel-rings")) << 32) ^ At, 0.0f, 0.0f, [Cx, Cy, R](Canvas& Cv) {
		Cv.StrokeArc(Cx, Cy, R * 0.6f, 0.0f, 2.0f * Pi, Hex(IgLime, 0.22f), 2.5f);
		Cv.StrokeArc(Cx, Cy, R, 0.0f, 2.0f * Pi, Hex(0x8a78b5), 2.5f);
	});
	// The sixteen spokes as eight diameters (the hub covers where two meet).
	for (int I = 0; I < 8; ++I)
	{
		const float A = Angle + static_cast<float>(I) * Pi / 8.0f;
		const float Dx = R * std::cos(A);
		const float Dy = R * std::sin(A);
		C.StrokePolyline({{Cx - Dx, Cy - Dy}, {Cx + Dx, Cy + Dy}}, false, Hex(0x6a5590, 0.9f), 1.5f);
	}
	// The gondolas, upright: each hangs from its spoke's end, the cars in the sign colours.
	const uint32_t Cars[8] = {K.Coral, K.Sky, IgSun, IgPink, IgLime, IgViolet, IgTang, K.Sky};
	for (int I = 0; I < 16; ++I)
	{
		const float A = Angle + static_cast<float>(I) * Pi / 8.0f;
		const uint32_t Car = Cars[I % 8];
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:wheel-car")) << 32) ^ Car, Cx + R * std::cos(A), Cy + R * std::sin(A), [Car](Canvas& Cv) {
			Cv.FillRect({-0.6f, 0.0f, 1.2f, 6.0f}, Hex(0x8a78b5));
			Cv.FillRoundRect({-7.0f, 5.0f, 14.0f, 11.0f}, 3.5f, Paint::Linear({0.0f, 5.0f}, {0.0f, 16.0f}, Hex(Car), Mix(Hex(Car), Hex(0x1a0f2a), 0.45f)));
		});
	}
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:wheel-hub")) << 32) ^ At, 0.0f, 0.0f, [Cx, Cy](Canvas& Cv) {
		Cv.FillCircle(Cx, Cy, 21.0f, Hex(0x8a78b5));
		Cv.FillCircle(Cx, Cy, 19.0f, Hex(0x2a1a46));
		IgniteSpark(Cv, Cx, Cy, 13.0f, Hex(IgLime));
	});
	// G1 (SPEC §2.4): three gondolas carry tonight's next featured starts, their placards hanging upright under the cars.
	for (size_t L = 0; L < Labels.size() && L < 3; ++L)
	{
		const IgnitePlacard P = IgnitePlacardAt(Cx, Cy, R, Angle, L, std::max(64.0f, C.Measure(Labels[L], 10.5f, 800) + 14.0f), Now);
		if (P.Show <= 0.01f)
		{
			continue;
		}
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * P.Show);
		const Rect& Tag = P.Tag;
		C.FillRoundRect({Tag.X - 1.2f, Tag.Y - 1.2f, Tag.W + 2.4f, Tag.H + 2.4f}, 10.2f, Hex(Cars[P.Car % 8], 0.9f));
		C.FillRoundRect(Tag, 9.0f, Hex(IgNight));
		C.Text(Labels[L], Tag.X + Tag.W / 2.0f, Tag.Y + 13.0f, Ts(10.5f, 800, Hex(K.Cream), Align::Center));
		C.SetAlpha(A0);
	}
}

void IgniteCabinetBody(Canvas& C, const Rect& R, const Color& Col, bool Selected, float Glow, const IgniteTones& K, bool Hot)
{
	if (Selected)
	{
		C.GlowRoundRect({R.X - 4.0f, R.Y - 4.0f, R.W + 8.0f, R.H + 8.0f}, 22.0f, IgniteA(Hex(K.Coral), 0.3f + 0.25f * Glow), 14.0f);
	}
	// The body: its top in the event's colour (the cabinet's marquee band) fading into grape, a lit line under the band.
	C.FillRoundRect(R, 18.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + 50.0f}, Mix(Hex(K.CabTop), Col, 0.32f), Hex(K.CabTop)));
	C.FillRect({R.X + 6.0f, R.Y + 50.0f, R.W - 12.0f, R.H - 64.0f}, Paint::Linear({0.0f, R.Y + 50.0f}, {0.0f, R.Y + R.H}, Hex(K.CabTop), Hex(K.CabBot)));
	C.FillRect({R.X, R.Y + 48.0f, R.W, 2.0f}, IgniteA(Col, 0.7f));
	if (Selected || Hot)
	{
		C.StrokeRoundRect(R, 18.0f, Hex(K.Coral), Selected ? 2.0f : 1.5f);
	}
}

namespace
{
/** Ignite's families on the floor (EventArt's glyphs; the colours the mockups give them). */
struct IgniteFamily
{
	const char* Stem;
	eventart::Glyph G;
	uint32_t Col;
};
const IgniteFamily IgniteFamilies[] = {
	{"flip-go-bounty", eventart::Glyph::Crosshair, IgCoral},
	{"flip-go", eventart::Glyph::Coin, IgCoral},
	{"two-am-afterburner", eventart::Glyph::Rocket, 0x6b3fd1},
	{"wake-up-call", eventart::Glyph::Bell, IgTang},
	{"coffee-crash", eventart::Glyph::Cup, 0xb0703a},
	{"lunch-break-pko", eventart::Glyph::Crosshair, IgPink},
	{"afternoon-fizz", eventart::Glyph::Glass, IgSky},
	{"deep-end", eventart::Glyph::Wave, IgViolet},
	{"coral-classic", eventart::Glyph::Star, IgCoral},
	{"plum-turbo", eventart::Glyph::Cherries, IgViolet},
	{"happy-hour-pko", eventart::Glyph::Glass, IgPink},
	{"sky-high", eventart::Glyph::Rocket, IgSky},
	{"spark-jackpot", eventart::Glyph::Sevens, 0xd4a017},
	{"confetti-cannon", eventart::Glyph::Firework, IgPink},
	{"big-wave", eventart::Glyph::Wave, 0x2f8fd6},
	{"mystery-pinata", eventart::Glyph::Gift, IgPink},
	{"night-glow", eventart::Glyph::Aurora, 0x2aa79b},
	{"after-party", eventart::Glyph::Glass, IgPink},
	{"sunday-blaze", eventart::Glyph::Flame, IgTang},
};

/** A template's stem: its id without the site's key and without a launch week's "-launchN". */
std::string IgniteStem(const std::string& Id)
{
	const size_t Dot = Id.find('.');
	std::string Stem = Dot == std::string::npos ? Id : Id.substr(Dot + 1);
	const size_t Launch = Stem.find("-launch");
	return Launch == std::string::npos ? Stem : Stem.substr(0, Launch);
}

const IgniteFamily* IgniteFamilyOf(const net::EventTemplate& T)
{
	const std::string Stem = IgniteStem(T.Id);
	for (const IgniteFamily& F : IgniteFamilies)
	{
		if (Stem == F.Stem)
		{
			return &F;
		}
	}
	return nullptr;
}

eventart::Glyph IgniteFormatGlyph(const net::EventTemplate& T)
{
	switch (T.Fmt)
	{
	case net::Format::Bounty: return eventart::Glyph::Crosshair;
	case net::Format::Mystery: return eventart::Glyph::Gift;
	case net::Format::Satellite: return eventart::Glyph::Ticket;
	case net::Format::Flip: return eventart::Glyph::Coin;
	default: return T.Speed == "Hyper" ? eventart::Glyph::Rocket : T.Speed == "Turbo" ? eventart::Glyph::Bolt : T.Speed == "Deep" ? eventart::Glyph::Wave : eventart::Glyph::Star;
	}
}
} // namespace

Color IgniteEventTint(const net::EventTemplate& T, const IgniteTones& K)
{
	if (!T.Series.empty())
	{
		if (const net::SeriesInfo* Sr = net::ForEvent(T.Id).FindSeries(T.Series))
		{
			return Hex(Sr->Color);
		}
	}
	if (const IgniteFamily* F = IgniteFamilyOf(T))
	{
		return F->Col == IgCoral ? Hex(K.Coral) : F->Col == IgSky ? Hex(K.Sky) : Hex(F->Col);
	}
	switch (net::TierOf(T.BuyInCents))
	{
	case net::Tier::Freeroll: return Hex(IgLime);
	case net::Tier::Micro: return Hex(IgViolet);
	case net::Tier::Low: return Hex(K.Sky);
	default: return Hex(IgSun);
	}
}

void IgniteEventMark(Canvas& C, const net::EventTemplate& T, float Cx, float Cy, float R, const IgniteTones& K)
{
	const Color Tint = IgniteEventTint(T, K);
	C.FillCircle(Cx, Cy, R, Paint::Linear({Cx, Cy - R}, {Cx, Cy + R}, Mix(Tint, Hex(0xffffff), 0.18f), Mix(Tint, Hex(0x000000), 0.22f)));
	eventart::Glyph G = IgniteFormatGlyph(T);
	if (!T.Series.empty())
	{
		if (const net::SeriesInfo* Sr = net::ForEvent(T.Id).FindSeries(T.Series))
		{
			G = eventart::SeriesGlyph(*Sr);
		}
	}
	else if (const IgniteFamily* F = IgniteFamilyOf(T))
	{
		G = F->G;
	}
	if (G == eventart::Glyph::Coin)
	{
		// The coin, drawn light (EventArt's is a ~500-vertex coin with its rim and shine).
		C.FillCircle(Cx, Cy, R * 0.62f, Hex(IgCream));
		C.StrokeArc(Cx, Cy, R * 0.45f, 0.0f, 2.0f * Pi, IgniteA(Tint, 0.9f), std::max(1.2f, R * 0.1f));
		C.Text("$", Cx, Cy + R * 0.3f, IgniteBi(R * 0.8f, Tint, Align::Center));
		return;
	}
	if (G == eventart::Glyph::Crosshair)
	{
		C.StrokeArc(Cx, Cy, R * 0.58f, 0.0f, 2.0f * Pi, Hex(IgCream), std::max(1.5f, R * 0.14f));
		C.StrokeArc(Cx, Cy, R * 0.28f, 0.0f, 2.0f * Pi, Hex(IgCream), std::max(1.5f, R * 0.14f));
		C.FillCircle(Cx, Cy, R * 0.09f, Hex(IgCream));
		return;
	}
	eventart::DrawGlyph(C, G, Cx, Cy, R * 1.22f, IgSun);
}
} // namespace igniteart
} // namespace ui
} // namespace ss
