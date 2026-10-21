#include "ShortStack/UI/SiteArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>
#include <vector>

// The poker sites' marks (update 2 §1.8, §5.3). ui-laptop.
//
// siteart::Logo is the one source of a site's logo: in a square it's the site's mark on its tile (the poker button, Mail,
// the Cashier's head, a table tile's corner); in a wide rectangle the mark sits at the left with the word mark beside it
// (the client's top bar, the boot card, the watermark on the felt). Every mark is vector, in the site's own fixed colours
// (never pal::, so a logo reads the same in any skin), readable from 12 px.
//
// - RiverLine: its two river lines in white on teal running to blue, "River" and "Line" (RiverLine::Logo's own shapes).
// - Ignite Poker: a cream flame with a sky-blue spark at its heart, on coral running to a deep ember; "Ignite" and
//   "Poker" in coral.
// - RunItTwice (Update 3 G1, the user's 2026-10-21 red-and-black restyle, SPEC §3.4 and §13.2 #13; core-sites-look): a
//   gloss-black tile with one 45-degree cut on its top-right corner and a chrome hairline, the turnstone mark (two river cards,
//   the second run dealt over the first) drawn here, the first card in chrome, the second in Signal Red with a white edge;
//   "RunIt" in Ink and "Twice" in LED Red. The sponsor brand's own mark (sponsorart::Mark("turnstone"), the team's patch)
//   keeps the heritage slate and gold. Its Strip is a red band with the name in Hot White.
//
// Update 3's additions (G1, core-sites-look): TwinCards and the Twin Stripe back's mark, Ignite's neon sign (the felt's
// watermark), the League's division crests in the status ramp (carbon, chrome, gold, red) and the chrome plate.

namespace ss
{
namespace ui
{
namespace siteart
{
namespace siteart_detail
{
const uint32_t SiteArtTeal = 0x27d3c3;
const uint32_t SiteArtBlue = 0x1a8fd8;
const uint32_t SiteArtCoral = 0xff6b57;
const uint32_t SiteArtEmber = 0xc2364a;
const uint32_t SiteArtSky = 0x4fc3f7;
const uint32_t SiteArtCream = 0xfff4ec;
const uint32_t SiteArtHotPink = 0xff3d8b; // Ignite's neon tube's end (the neon sign's glow)
const uint32_t SiteArtInk = 0xe6edf7;     // RiverLine's word mark
// RunItTwice, after the bell (SPEC §3.3): RitArt.cpp's constants, restated for the marks (none is a RiverLine hex).
const uint32_t SiteArtOnyx = 0x0c0c0f;
const uint32_t SiteArtGloss = 0x202026;
const uint32_t SiteArtCarbon2 = 0x1e1e23; // Panel2: the Iron crest
const uint32_t SiteArtFrost = 0xececef;   // Ink: "RunIt"
const uint32_t SiteArtSignal = 0xd2141a;  // Signal Red
const uint32_t SiteArtDeep = 0x7a0a12;    // Deep Red: red lacquer's dark side
const uint32_t SiteArtLacquer = 0xe3262c; // red lacquer's lit top
const uint32_t SiteArtLed = 0xff2633;     // LED Red: "Twice", 1-unit cores
const uint32_t SiteArtChrome = 0xb6bbc5;  // Accent2
const uint32_t SiteArtChromeD = 0x8d929c; // the chrome ramp: dark, light, mid
const uint32_t SiteArtChromeL = 0xf3f4f6;
const uint32_t SiteArtChromeM = 0xaab0ba;
const uint32_t SiteArtGoldD = 0xa8822f; // Status Gold: the Gold division and the High Table card only
const uint32_t SiteArtGoldL = 0xe6c66a;
const uint32_t SiteArtGoldM = 0xc9a24a;
const uint32_t SiteArtWhite = 0xffffff; // Hot White

/** The mark's square (centred in R, its side the shorter of R's). */
Rect SiteArtSquare(const Rect& R)
{
	const float S = std::min(R.W, R.H);
	return {R.X + (R.W - S) * 0.5f, R.Y + (R.H - S) * 0.5f, S, S};
}

/** R with one 45-degree cut of Cut on its top-right corner (RunItTwice's one-cut corner, SiteLook::Cut's shape). */
std::vector<Vec2> SiteArtCutRect(const Rect& R, float Cut)
{
	const float K = std::max(0.0f, std::min(Cut, std::min(R.W, R.H)));
	return {{R.X, R.Y}, {R.X + R.W - K, R.Y}, {R.X + R.W, R.Y + K}, {R.X + R.W, R.Y + R.H}, {R.X, R.Y + R.H}};
}

/** The part of a convex polygon on one side of the line y = Y (Above: y <= Y), as Sutherland-Hodgman clips it. */
std::vector<Vec2> SiteArtClipY(const std::vector<Vec2>& Pts, float Y, bool Above)
{
	std::vector<Vec2> Out;
	const size_t N = Pts.size();
	for (size_t I = 0; I < N; ++I)
	{
		const Vec2 A = Pts[I];
		const Vec2 B = Pts[(I + 1) % N];
		const bool InA = Above ? A.Y <= Y : A.Y >= Y;
		const bool InB = Above ? B.Y <= Y : B.Y >= Y;
		if (InA)
		{
			Out.push_back(A);
		}
		if (InA != InB && B.Y != A.Y)
		{
			const float T = (Y - A.Y) / (B.Y - A.Y);
			Out.push_back({A.X + (B.X - A.X) * T, Y});
		}
	}
	return Out;
}

/**
 * A convex polygon filled with a three-stop vertical ramp (Top at Y0, Mid at Y0 + MidT of the height, Bottom at Y1): Paint
 * is evaluated per vertex, so the middle stop needs the polygon split at its height.
 */
void SiteArtRamp(Canvas& C, const std::vector<Vec2>& Pts, float Y0, float Y1, uint32_t Top, float MidT, uint32_t Mid, uint32_t Bottom)
{
	const float Ym = Y0 + (Y1 - Y0) * MidT;
	const std::vector<Vec2> Upper = SiteArtClipY(Pts, Ym, true);
	const std::vector<Vec2> Lower = SiteArtClipY(Pts, Ym, false);
	if (Upper.size() >= 3)
	{
		C.FillPolygon(Upper, Paint::Linear({0.0f, Y0}, {0.0f, Ym}, Hex(Top), Hex(Mid)));
	}
	if (Lower.size() >= 3)
	{
		C.FillPolygon(Lower, Paint::Linear({0.0f, Ym}, {0.0f, Y1}, Hex(Mid), Hex(Bottom)));
	}
}

/** One of the turnstone's river cards, a small rounded card: Fill (or the chrome ramp when Chrome), an Edge stroke. */
void SiteArtCard(Canvas& C, const Rect& R, const Color& Fill, bool Chrome, const Color& Edge, float EdgeW)
{
	const float Rad = R.W * 0.1f;
	const std::vector<Vec2> Shape = Canvas::RoundRectPath(R, Rad, 2);
	if (Chrome)
	{
		SiteArtRamp(C, Shape, R.Y, R.Y + R.H, SiteArtChromeD, 0.45f, SiteArtChromeL, SiteArtChromeM);
	}
	else
	{
		C.FillPolygon(Shape, Fill);
	}
	if (EdgeW > 0.0f && Edge.A > 0.0f)
	{
		C.StrokeRoundRect(R, Rad, Edge, EdgeW);
	}
}

/**
 * The turnstone mark at (X, Y), S its unit (the mockups' rit_art.mark): the first card (0.40 x 0.56) low and left, the second
 * dealt over it 0.30 across and 0.16 up, edged. Its box is 0.70 S wide and 0.72 S tall.
 */
void SiteArtTwin(Canvas& C, float X, float Y, float S, const Color& First, bool FirstChrome, const Color& Second, const Color& Edge)
{
	const float Cw = S * 0.40f;
	const float Ch = S * 0.56f;
	SiteArtCard(C, {X, Y + S * 0.16f, Cw, Ch}, First, FirstChrome, Hex(0, 0.0f), 0.0f);
	SiteArtCard(C, {X + S * 0.30f, Y, Cw, Ch}, Second, false, Edge, std::max(1.0f, S * 0.035f));
}

void SiteArtRiverLine(Canvas& C, const Rect& Sq)
{
	const float X = Sq.X;
	const float Y = Sq.Y;
	const float Sz = Sq.W;
	C.FillRoundRect(Sq, Sz * 0.26f, Paint::Linear({X, Y}, {X + Sz, Y + Sz}, Hex(SiteArtTeal), Hex(SiteArtBlue)));
	for (int K = 0; K < 2; ++K)
	{
		std::vector<Vec2> Wave;
		for (int I = 0; I <= 12; ++I)
		{
			const float T = static_cast<float>(I) / 12.0f;
			Wave.push_back({X + Sz * (0.18f + 0.64f * T), Y + Sz * (0.4f + 0.2f * static_cast<float>(K)) + std::sin(T * Pi * 2.0f) * Sz * 0.08f});
		}
		const Color W = Hex(0xffffff, K == 0 ? 1.0f : 0.6f);
		C.StrokePolyline(Wave, false, W, Sz * 0.09f, true);
	}
}

/** A teardrop flame standing on (Cx, Base), H tall. */
void SiteArtFlame(Canvas& C, float Cx, float Base, float H, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I <= 24; ++I)
	{
		const float T = static_cast<float>(I) / 24.0f;
		const float A = T * Pi * 2.0f;
		// Round at the bottom, drawn up to a point that leans a little: a flame, not a drop.
		const float R = H * 0.34f * (1.0f - 0.6f * (0.5f - 0.5f * std::cos(A)));
		const float Up = std::cos(A) > 0.0f ? 1.0f : 0.6f;
		P.push_back({Cx + std::sin(A) * R + (1.0f - std::cos(A)) * H * 0.03f, Base - H * 0.34f - std::cos(A) * H * 0.56f * Up});
	}
	C.FillPolygon(P, Col);
}

void SiteArtIgnite(Canvas& C, const Rect& Sq)
{
	const float X = Sq.X;
	const float Y = Sq.Y;
	const float Sz = Sq.W;
	C.FillRoundRect(Sq, Sz * 0.26f, Paint::Linear({X, Y}, {X + Sz, Y + Sz}, Hex(SiteArtCoral), Hex(SiteArtEmber)));
	const float Cx = X + Sz * 0.5f;
	const float Base = Y + Sz * 0.86f;
	SiteArtFlame(C, Cx, Base, Sz * 0.72f, Hex(SiteArtCream));
	SiteArtFlame(C, Cx + Sz * 0.01f, Base - Sz * 0.04f, Sz * 0.38f, Hex(SiteArtCoral));
	// The spark: a small four-point star in sky blue at the flame's heart.
	const float Sx = Cx + Sz * 0.01f;
	const float Sy = Base - Sz * 0.2f;
	const float R = Sz * 0.11f;
	std::vector<Vec2> Star;
	for (int I = 0; I < 8; ++I)
	{
		const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 4.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.36f;
		Star.push_back({Sx + Rr * std::cos(A), Sy + Rr * std::sin(A)});
	}
	C.FillPolygon(Star, Hex(SiteArtSky));
}

void SiteArtRunItTwice(Canvas& C, const Rect& Sq)
{
	// The gloss-black tile with its one cut and a chrome hairline, then the turnstone's two cards drawn here (chrome, and Signal
	// Red with a white edge). The sponsor brand's patch (sponsorart::Mark("turnstone")) keeps its own slate and gold.
	const float Sz = Sq.W;
	const std::vector<Vec2> Tile = SiteArtCutRect(Sq, Sz * 0.18f);
	C.FillPolygon(Tile, Paint::Linear({Sq.X, Sq.Y}, {Sq.X + Sz, Sq.Y + Sz}, Hex(SiteArtGloss), Hex(SiteArtOnyx)));
	// The hairline runs half its width inside the tile's edge, so the mark never reaches past its rect.
	const float Hair = Sz >= 20.0f ? 1.0f : 0.75f;
	const float In = Hair * 0.5f;
	C.StrokePolyline(SiteArtCutRect({Sq.X + In, Sq.Y + In, Sz - Hair, Sz - Hair}, Sz * 0.18f - In * 0.4142f), true, Hex(SiteArtChrome, 0.35f), Hair);
	SiteArtTwin(C, Sq.X + Sz * 0.20f, Sq.Y + Sz * 0.16f, Sz * 0.62f, Hex(SiteArtChrome), true, Hex(SiteArtSignal), Hex(SiteArtWhite));
}

/** The two halves of a site's word mark, their colours and weights. */
struct SiteArtWords
{
	const char* First;
	const char* Second;
	uint32_t FirstInk;
	uint32_t SecondInk;
	int SecondWeight;
};

SiteArtWords SiteArtWordsOf(net::SiteId S)
{
	switch (S)
	{
	case net::SiteId::Ignite: return {"Ignite", "Poker", SiteArtCream, SiteArtCoral, 400};
	case net::SiteId::RunItTwice: return {"RunIt", "Twice", SiteArtFrost, SiteArtLed, 800}; // "Twice" in LED Red, the logo's one red word
	default: return {"River", "Line", SiteArtInk, SiteArtTeal, 400};
	}
}

/** A five-point star of outer radius R (the crests' group-winner stars: Hot White, never red). */
void SiteArtStar(Canvas& C, float Cx, float Cy, float R, const Color& Col)
{
	std::vector<Vec2> P;
	for (int I = 0; I < 10; ++I)
	{
		const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
		const float Rr = I % 2 == 0 ? R : R * 0.42f;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A)});
	}
	C.FillPolygon(P, Col);
}

/** The crest's outline: a flat top, straight sides to 0.45 of its height, then curving in to the point (rit_art.crest). */
std::vector<Vec2> SiteArtShield(float Cx, float Cy, float H)
{
	const float W = H * 0.8f;
	const float X0 = Cx - W * 0.5f;
	const float Y0 = Cy - H * 0.5f;
	std::vector<Vec2> Out = {{X0, Y0}, {X0 + W, Y0}, {X0 + W, Y0 + H * 0.45f}};
	// The right side's curve down to the point, then the left side's mirror image back up.
	const int Steps = 8;
	for (int I = 1; I <= Steps; ++I)
	{
		const float T = static_cast<float>(I) / static_cast<float>(Steps);
		const float U = 1.0f - T;
		// A cubic from the side (W, 0.45H) bulging out and down to the point (W/2, H).
		const float Bx = U * U * U * W + 3.0f * U * U * T * W + 3.0f * U * T * T * (W * 0.70f) + T * T * T * (W * 0.5f);
		const float By = U * U * U * (H * 0.45f) + 3.0f * U * U * T * (H * 0.75f) + 3.0f * U * T * T * (H * 0.92f) + T * T * T * H;
		Out.push_back({X0 + Bx, Y0 + By});
	}
	for (int I = Steps - 1; I >= 0; --I)
	{
		const Vec2 Rp = Out[static_cast<size_t>(2 + I)];
		Out.push_back({X0 + W - (Rp.X - X0), Rp.Y});
	}
	return Out;
}
} // namespace siteart_detail

void Logo(Canvas& C, Rect R, net::SiteId S)
{
	using namespace siteart_detail;
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	const bool Wide = R.W >= R.H * 1.8f;
	const Rect Sq = Wide ? Rect{R.X, R.Y, R.H, R.H} : SiteArtSquare(R);
	switch (S)
	{
	case net::SiteId::Ignite: SiteArtIgnite(C, Sq); break;
	case net::SiteId::RunItTwice: SiteArtRunItTwice(C, Sq); break;
	default: SiteArtRiverLine(C, Sq); break;
	}
	if (!Wide)
	{
		return;
	}
	// The word mark beside it, fitted to what's left.
	const SiteArtWords W = SiteArtWordsOf(S);
	const float Room = R.W - R.H * 1.28f;
	float Size = R.H * 0.74f;
	while (Size > 8.0f && C.Measure(W.First, Size, 800) + C.Measure(W.Second, Size, W.SecondWeight) > Room)
	{
		Size -= 1.0f;
	}
	const float X = R.X + R.H * 1.28f;
	const float Base = R.Y + R.H * 0.5f + Size * 0.36f;
	const float Fw = C.Text(W.First, X, Base, Ts(Size, 800, Hex(W.FirstInk)));
	C.Text(W.Second, X + Fw, Base, Ts(Size, W.SecondWeight, Hex(W.SecondInk)));
}

void Strip(Canvas& C, Rect R, net::SiteId S)
{
	using namespace siteart_detail;
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	if (S == net::SiteId::RunItTwice)
	{
		// RunItTwice's band (v1.2): Signal Red running to Deep Red under one cut corner, a 1-unit LED Red top edge, the mark's
		// tile at the left and the name in Hot White capitals in its display face (5.45:1 on Signal Red, more as it deepens).
		const std::vector<Vec2> Band = SiteArtCutRect(R, R.H * 0.35f);
		C.FillPolygon(Band, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y}, Hex(SiteArtSignal), Hex(SiteArtDeep)));
		C.FillRect({R.X, R.Y, R.W - R.H * 0.35f, 1.0f}, Hex(SiteArtLed));
		Logo(C, {R.X + 2.0f, R.Y + 2.0f, R.H - 4.0f, R.H - 4.0f}, S);
		const char* Word = "RUNITTWICE";
		float Px = R.H * 0.62f;
		while (Px > 6.0f && C.Measure(Word, Px, Font::BoldCondensed) > R.W - R.H - 8.0f)
		{
			Px -= 0.5f;
		}
		TextStyle St = Ts(Px, 700, Hex(SiteArtWhite), Align::Left, Baseline::Middle);
		St.Face = static_cast<int8_t>(Font::BoldCondensed);
		C.Text(Word, R.X + R.H + 2.0f, R.Y + R.H * 0.5f + 0.5f, St);
		return;
	}
	// A table tile's band: the site's colour along it, its mark at the left, its short name in capitals.
	const uint32_t Main = S == net::SiteId::Ignite ? SiteArtCoral : SiteArtTeal;
	C.FillRoundRect(R, R.H * 0.5f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y}, Hex(Main, 0.28f), Hex(Main, 0.06f)));
	C.StrokeRoundRect(R, R.H * 0.5f, Hex(Main, 0.55f), 1.0f);
	Logo(C, {R.X + 2.0f, R.Y + 2.0f, R.H - 4.0f, R.H - 4.0f}, S);
	const char* Name = S == net::SiteId::Ignite ? "IGNITE" : "RIVERLINE";
	float Size = R.H * 0.56f;
	while (Size > 6.0f && C.Measure(Name, Size, 800) > R.W - R.H - 8.0f)
	{
		Size -= 0.5f;
	}
	C.Text(Name, R.X + R.H + 2.0f, R.Y + R.H * 0.5f + 0.5f, Ts(Size, 800, Hex(Main), Align::Left, Baseline::Middle));
}

// ------------------------------------------------------------------ Update 3's additions (G1, core-sites-look)

void TwinCards(Canvas& C, Rect R, const Color& First, const Color& Second, const Color& Edge)
{
	using namespace siteart_detail;
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	const float S = std::min(R.W / 0.70f, R.H / 0.72f);
	SiteArtTwin(C, R.X + (R.W - S * 0.70f) * 0.5f, R.Y + (R.H - S * 0.72f) * 0.5f, S, First, false, Second, Edge);
}

void TwinStripeMark(Canvas& C, Rect R)
{
	using namespace siteart_detail;
	// The Twin Stripe back's centre (§3.5): both cards in Hot White, the second edged in Onyx so it reads over the first.
	TwinCards(C, R, Hex(SiteArtWhite), Hex(SiteArtWhite), Hex(SiteArtOnyx));
}

float NeonSign(Canvas& C, float X, float Base, float Size, float Strength)
{
	using namespace siteart_detail;
	if (Size <= 0.0f || Strength <= 0.0f)
	{
		return 0.0f;
	}
	// Ignite's flame and its name as a lit tube (§2.5's watermark): three soft copies in Hot Pink, wider and fainter as they go
	// out (alpha 0.16, 0.10, 0.06 times Strength), each once, a little off the tube (the felt's own watermark offsets, scaled
	// by Size), under the crisp one in coral and cream (0.22). Never SetAlpha: every layer is a colour's own alpha, so the
	// parent alpha (a fade, the Kast preview) carries through.
	const float FlameH = Size * 0.72f;
	const float Fx = X + Size * 0.30f;
	const float Tx = X + Size * 0.66f;
	const float Px = Size * 0.92f;
	TextStyle Word = Ts(Px, 900, Hex(SiteArtCoral), Align::Left, Baseline::Alphabetic);
	Word.Face = static_cast<int8_t>(Font::BlackItalic);
	const float Ww = C.Measure("Ignite", Px, Font::BlackItalic);
	const float GlowAlpha[3] = {0.06f, 0.10f, 0.16f};
	const float GlowSpread[3] = {3.0f, 1.5f, 0.5f};
	const Vec2 GlowOff[3] = {{-0.05f, -0.033f}, {0.05f, 0.033f}, {0.0f, 0.05f}};
	for (int I = 0; I < 3; ++I)
	{
		const Color Glow = Hex(SiteArtHotPink, std::min(1.0f, GlowAlpha[I] * Strength));
		const float Dx = GlowOff[I].X * Size;
		const float Dy = GlowOff[I].Y * Size;
		SiteArtFlame(C, Fx + Dx, Base + Dy, FlameH + GlowSpread[I] * 2.0f, Glow);
		TextStyle G = Word;
		G.Col = Glow;
		C.Text("Ignite", Tx + Dx, Base - Size * 0.04f + Dy, G);
	}
	const float Crisp = std::min(1.0f, 0.22f * Strength);
	SiteArtFlame(C, Fx, Base, FlameH, Hex(SiteArtCoral, Crisp));
	SiteArtFlame(C, Fx + Size * 0.01f, Base - Size * 0.04f, FlameH * 0.52f, Hex(SiteArtCream, Crisp));
	Word.Col = Hex(SiteArtCoral, Crisp);
	C.Text("Ignite", Tx, Base - Size * 0.04f, Word);
	return Tx - X + Ww;
}

void DivisionShield(Canvas& C, float Cx, float Cy, float Size, int Div, int Stars)
{
	using namespace siteart_detail;
	if (Size <= 0.0f)
	{
		return;
	}
	const int D = std::max(0, std::min(4, Div));
	const std::vector<Vec2> Shape = SiteArtShield(Cx, Cy, Size);
	const float Y0 = Cy - Size * 0.5f;
	const float Y1 = Cy + Size * 0.5f;
	const float W = Size * 0.8f;
	uint32_t Bar = SiteArtOnyx;
	switch (D)
	{
	case 0: // Open: a chrome outline
		C.StrokePolyline(Shape, true, Hex(SiteArtChrome), std::max(1.0f, Size * 0.05f));
		Bar = SiteArtChrome;
		break;
	case 1: // Iron: carbon, a dark chrome edge
		C.FillPolygon(Shape, Hex(SiteArtCarbon2));
		C.StrokePolyline(Shape, true, Hex(SiteArtChromeD), std::max(1.0f, Size * 0.05f));
		Bar = SiteArtChromeD;
		break;
	case 2: // Steel: chrome
		SiteArtRamp(C, Shape, Y0, Y1, SiteArtChromeD, 0.45f, SiteArtChromeL, SiteArtChromeM);
		break;
	case 3: // Gold: Status Gold (a status metal, never money)
		SiteArtRamp(C, Shape, Y0, Y1, SiteArtGoldD, 0.5f, SiteArtGoldL, SiteArtGoldM);
		break;
	default: // High Bridge: red lacquer and its arch
		SiteArtRamp(C, Shape, Y0, Y1, SiteArtLacquer, 0.5f, SiteArtSignal, SiteArtDeep);
		C.StrokeArc(Cx, Cy + Size * 0.1f, W * 0.28f, Pi, Pi * 2.0f, Hex(SiteArtWhite), std::max(1.0f, Size * 0.08f));
		break;
	}
	// The bars: one for Open to four for Gold (High Bridge wears its arch instead).
	const int Bars = D == 4 ? 0 : D + 1;
	for (int I = 0; I < Bars; ++I)
	{
		C.FillRect({Cx - W * 0.3f, Cy - Size * 0.28f + static_cast<float>(I) * Size * 0.13f, W * 0.6f, Size * 0.07f}, Hex(Bar));
	}
	// A group winner's stars above the crest (up to five, Hot White).
	const int Count = std::max(0, std::min(5, Stars));
	const float Sr = Size * 0.09f;
	for (int I = 0; I < Count; ++I)
	{
		const float Sx = Cx + (static_cast<float>(I) - static_cast<float>(Count - 1) * 0.5f) * Sr * 2.4f;
		SiteArtStar(C, Sx, Y0 - Sr * 1.6f, Sr, Hex(SiteArtWhite));
	}
}

void ChromePlate(Canvas& C, Rect R, float Cut)
{
	using namespace siteart_detail;
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	// Brushed chrome (§3.3's chrome ramp, dark to light at 45% to mid), one cut corner, a lit top edge and a darker rim; the
	// caller engraves it in Onyx (6.25:1 on the ramp's darkest).
	const std::vector<Vec2> Plate = SiteArtCutRect(R, Cut);
	SiteArtRamp(C, Plate, R.Y, R.Y + R.H, SiteArtChromeD, 0.45f, SiteArtChromeL, SiteArtChromeM);
	const float K = std::max(0.0f, std::min(Cut, std::min(R.W, R.H)));
	C.FillRect({R.X + 1.0f, R.Y + 1.0f, std::max(0.0f, R.W - K - 2.0f), 1.0f}, Hex(SiteArtWhite, 0.55f));
	C.StrokePolyline(Plate, true, Hex(SiteArtChromeD), 1.0f);
}
} // namespace siteart
} // namespace ui
} // namespace ss
