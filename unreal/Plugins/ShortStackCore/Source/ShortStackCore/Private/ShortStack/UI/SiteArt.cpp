#include "ShortStack/UI/SiteArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/SponsorArt.h"
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
// - RunItTwice: its patch's mark (sponsorart::Mark, brand "turnstone": two river cards, the second run dealt over the
//   first) in gold on slate; "RunIt" and "Twice" in gold.

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
const uint32_t SiteArtSlate = 0x334155;
const uint32_t SiteArtNight = 0x1c2433;
const uint32_t SiteArtGold = 0xd4a843;
const uint32_t SiteArtInk = 0xe6edf7;  // RiverLine's word mark
const uint32_t SiteArtFrost = 0xeef1f5; // RunItTwice's

/** The mark's square (centred in R, its side the shorter of R's). */
Rect SiteArtSquare(const Rect& R)
{
	const float S = std::min(R.W, R.H);
	return {R.X + (R.W - S) * 0.5f, R.Y + (R.H - S) * 0.5f, S, S};
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
	const float Sz = Sq.W;
	C.FillRoundRect(Sq, Sz * 0.26f, Paint::Linear({Sq.X, Sq.Y}, {Sq.X + Sz, Sq.Y + Sz}, Hex(SiteArtSlate), Hex(SiteArtNight)));
	// The patch's own mark (RunItTwice's brand keeps its id "turnstone"; update 2 §11.7).
	sponsorart::Mark(C, "turnstone", {Sq.X + Sz * 0.06f, Sq.Y + Sz * 0.06f, Sz * 0.88f, Sz * 0.88f});
}

/** The two halves of a site's word mark and their colours. */
struct SiteArtWords
{
	const char* First;
	const char* Second;
	uint32_t FirstInk;
	uint32_t SecondInk;
};

SiteArtWords SiteArtWordsOf(net::SiteId S)
{
	switch (S)
	{
	case net::SiteId::Ignite: return {"Ignite", "Poker", SiteArtCream, SiteArtCoral};
	case net::SiteId::RunItTwice: return {"RunIt", "Twice", SiteArtFrost, SiteArtGold};
	default: return {"River", "Line", SiteArtInk, SiteArtTeal};
	}
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
	while (Size > 8.0f && C.Measure(W.First, Size, 800) + C.Measure(W.Second, Size, 400) > Room)
	{
		Size -= 1.0f;
	}
	const float X = R.X + R.H * 1.28f;
	const float Base = R.Y + R.H * 0.5f + Size * 0.36f;
	const float Fw = C.Text(W.First, X, Base, Ts(Size, 800, Hex(W.FirstInk)));
	C.Text(W.Second, X + Fw, Base, Ts(Size, 400, Hex(W.SecondInk)));
}

void Strip(Canvas& C, Rect R, net::SiteId S)
{
	using namespace siteart_detail;
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	// A table tile's band: the site's colour along it, its mark at the left, its short name in capitals.
	const uint32_t Main = S == net::SiteId::Ignite ? SiteArtCoral : S == net::SiteId::RunItTwice ? SiteArtGold : SiteArtTeal;
	C.FillRoundRect(R, R.H * 0.5f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y}, Hex(Main, 0.28f), Hex(Main, 0.06f)));
	C.StrokeRoundRect(R, R.H * 0.5f, Hex(Main, 0.55f), 1.0f);
	Logo(C, {R.X + 2.0f, R.Y + 2.0f, R.H - 4.0f, R.H - 4.0f}, S);
	const char* Name = S == net::SiteId::Ignite ? "IGNITE" : S == net::SiteId::RunItTwice ? "RUNITTWICE" : "RIVERLINE";
	float Size = R.H * 0.56f;
	while (Size > 6.0f && C.Measure(Name, Size, 800) > R.W - R.H - 8.0f)
	{
		Size -= 0.5f;
	}
	C.Text(Name, R.X + R.H + 2.0f, R.Y + R.H * 0.5f + 0.5f, Ts(Size, 800, Hex(Main), Align::Left, Baseline::Middle));
}
} // namespace siteart
} // namespace ui
} // namespace ss
