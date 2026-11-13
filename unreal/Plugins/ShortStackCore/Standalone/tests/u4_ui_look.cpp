// u4_ui: the festival looks (update 4 SPEC v1.7 §2.15.2; F-look's file): the ten looks, the takeover, felts, backs, crests,
// trophies, belts and pins. Called from u4_ui_main.cpp (the kit).
// Phase A (update 4 SPEC v1.7 §6 PA-48): registered, empty.
// G1 FL-1 (F-look): the palette rules on the ten looks' tokens (§2.7.2, §2.7.3), the C++ twins of
// durable/update4/model/fest_brands/palette_check.py: FestLookDistance (R-12), FestContrast's token half and FestLeak's table half
// (FL-4 adds both checks' page halves). No page is drawn or emitted here yet. Helpers sit in an anonymous namespace with a Look
// suffix (the u4_ui binary links F-hubs' files beside this one).
// G1 FL-2 (F-look): FestCrestLowDetail, the crests (§2.7.4): the low-detail path's and the full crest's budgets at both scales, each
// festival's frame, motif and RCOP's wave band, a rival festival unlike a plain rival series, the crest cache counted. Draws, emits
// nothing.
// G1 FL-3 (F-look): FestTrophyCached, the trophies, title objects, belts and pins (§2.7.5, §2.8.7): each one cached fragment (asked of
// Canvas::IsCached through UI/EventArt.cpp's keys), their budgets at both scales, the championships' 1.15, the RCOP Cup's engraving,
// R-3's names. Draws, emits nothing.
// G1 FL-5 (F-look): Ignite's band with the festival's attraction (§2.7.4): FestBandBudget (the band with each attraction within 15k at
// both scales, its bulbs one cached row a chase step, asked of Canvas::IsCached through UI/IgniteArt.cpp's IgniteAttrBulbsKey; PLAY's
// 34k / 400 is FL-final's FestBandPlayLook), FestBandstand's arch half (the arch in the Prize Counter's place only while FEIGN
// is Live, the tower, the pavilion, the foot sign, the puck) and FestCopyUi's art half (FestArtInventory by kind through the copy rules;
// every word the attractions paint is listed). Draws the band straight onto its own canvases, emits nothing.
// G1 FL-5, fix round 1: FestCopyUi's art half screens §1.2 item 5's avoided words after the fill (FestArtInventory's only screen for
// them), and FestBandstand finds no booth back wall under the arch. Test-only.
// G1 FL-7 (F-look): FestCrestLowDetail's FL-7 half (QF-10, no new check name), FestMotionsLook: the stills sub-assertion (with Still every
// word-mark and banner draws its settled frame; every crest motion rests outside its run), the motions moving and settling (WRIT's flap,
// FEIGN's bob, PUSH's gauge per world minute, GRIT's curtains, RCOP's waves per world minute, Summer Slam's rays, REIGN's door-light, the
// crests' ring, key, seal and dome chase, the Clasp), the word-marks (face, caps, twice print, tracking, FEIGN's letters, the underlines,
// 1k vertices and 40 runs at 46) and the banner (inside its rect, 6k vertices settled, cached ground). Draws, emits nothing.
// G1 FL-4 (F-look): the takeover (§2.7.3, §2.7.4, §2.15.2). FestSkinRule (every hour of days 0-400 on the three hosts against the rule
// worked out from the calendar; RCOP 2026's edges; Afterburner over FEIGN; the dev look; a bench line), FestTableBudgets (the felt marks
// and card backs of the ten looks, cached, within 1.5k and 300; the three festival tables within 22k / 19k wearing their looks; no
// sponsor mark on REIGN's), and the page halves of FestLeak and FestContrast. Emits F-look's seven pages (QF-18): fest_takeover_rl_lobby,
// fest_takeover_ig_play, fest_takeover_ig_afterburner, fest_takeover_rit_board, fest_table_rl, fest_table_ig and fest_table_rit.
// G1 FL-final (F-look; PLAN_G1 item 17): FestBandBudget's PLAY half, FestBandPlayLook: Ignite's PLAY at most 34k vertices and 400
// text runs with each attraction (FEIGN's arch, PUSH's tower, PENNANT's pavilion) on every evening of its first edition, on one
// laptop visiting the three in date order. Asserted wherever the attraction is on the page (F-hubs' band fill, FH-3b, merged: the
// integration's trees); a tree without the fill prints PLAY today and stops after FEIGN. Runs last (after the takeover pages).
// G1 FL-final: FestSkinRule's Afterburner page reads the skin without the attraction's foot sign (FH-3b's fill puts FEIGN's arch
// and its Mirror Mint sign on FEIGN's day 2 at 02:00; the attraction keeps its look every hour, only the palette yields). Test-only.
#include "UiSeg.h"
#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/FestivalArt.h"
#include "ShortStack/UI/SponsorArt.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/IgniteArt.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// UI/EventArt.cpp's private frame outline (external linkage, as UI/FestivalArt.cpp reaches its SlotOf): FestCrestLowDetail checks the
// six festival frames draw their own outlines. The enum is declared opaque here; its values are FestivalLook::Frame's (static_asserted
// in UI/EventArt.cpp). And the festival art's cache keys (G1 FL-3): FestTrophyCached asks Canvas::IsCached for them.
namespace ss
{
namespace ui
{
namespace eventart
{
namespace eventart_detail
{
enum class Frame : int;
std::vector<Vec2> FramePts(Frame F, float K);
uint64_t FestTrophyKey(fest::Brand B, float Size, bool Lod, const std::vector<int>& Fields, int Tier);
uint64_t FestBeltKey(fest::Brand B, float Size, bool Lod);
uint64_t FestPinKey(fest::Brand B, int Grade, world::FestMark Mark, float Size);
// G1 FL-7: the crest motions' and the Clasp's clocks (FestCrestLowDetail's stills sub-assertion reads where each rests).
float FestCrestPhaseOf(fest::Brand B, double Time);
double FestCrestSeconds(fest::Brand B);
double FestClaspSeconds(fest::Brand B, int Tier);
} // namespace eventart_detail
} // namespace eventart
// UI/IgniteArt.cpp's (G1 FL-5): the cache key of the attraction's bulbs row for a band (FestBandBudget asks Canvas::IsCached for it), and
// UI/FestivalArt.cpp's U32 foot sign (FestBandstand finds it on the band).
namespace igniteart
{
uint64_t IgniteAttrBulbsKey(const IgniteBand& B);
} // namespace igniteart
namespace festart
{
namespace festart_detail
{
const char* FestHubSign();
// G1 FL-7: when the word-mark's and the banner's motions settle (their stills' clocks), and their settled fragments' cache keys.
double FestWordSettle(const FestivalLook& L, const std::string& Text);
double FestBannerSettle(const FestivalLook& L);
uint64_t FestWordMarkKey(const FestivalLook& L, const std::string& Shown, float Size, int Minute);
uint64_t FestBannerKey(const FestivalLook& L, const Rect& R, int Layer, int Minute);
// G1 FL-4: the felt mark's and the card back's cache keys (FestTableBudgets asks Canvas::IsCached for them).
uint64_t FestFeltMarkKey(const FestivalLook& L, float Size);
uint64_t FestCardBackKey(const FestivalLook& L, float W, float H);
} // namespace festart_detail
} // namespace festart
// UI/RiverLineSites.cpp's (G1 FL-4, LEAD_CALLS R-18): a festival's step ticket, which WELCOME TICKETS leaves to the festival.
namespace rlsites_detail
{
bool SitesFestStepTicket(const std::string& Key);
} // namespace rlsites_detail
} // namespace ui
} // namespace ss

namespace
{
using ss::fest::Brand;
using ss::net::SiteId;
namespace festart = ss::ui::festart;

const Brand AllLook[] = {Brand::Rcop, Brand::RingRush, Brand::Tco, Brand::Slam, Brand::Feign, Brand::Push, Brand::Writ, Brand::Grit, Brand::Pennant, Brand::Reign};

/** §2.7.2's host of each look (the table's site column; fest::All is F-net's, empty at H0). */
SiteId HostLook(Brand B)
{
	switch (B)
	{
	case Brand::Feign:
	case Brand::Push:
	case Brand::Pennant: return SiteId::Ignite;
	case Brand::Writ:
	case Brand::Grit:
	case Brand::Reign: return SiteId::RunItTwice;
	default: return SiteId::RiverLine;
	}
}

/** The host's sting (§2.7.6: four stings, one per site family plus the champion's). */
ss::SoundId StingLook(SiteId S)
{
	return S == SiteId::Ignite ? ss::SoundId::FestIgnite : S == SiteId::RunItTwice ? ss::SoundId::FestRit : ss::SoundId::FestRiver;
}

/** ui_test::Expect (the kit's, counted once by u4_ui_main.cpp) for a built message. */
void ExpectLook(bool Condition, const std::string& What)
{
	ui_test::Expect(Condition, What.c_str());
}

const char* NameLook(Brand B)
{
	static const char* const Names[] = {"None", "RCOP", "Ring Rush", "TCO", "Summer Slam", "FEIGN", "PUSH", "WRIT", "GRIT", "PENNANT", "REIGN"};
	return Names[static_cast<int>(B)];
}

uint32_t RgbLook(const ss::ui::Color& C)
{
	const auto Byte = [](float V) { return static_cast<uint32_t>(std::min(255.0f, std::max(0.0f, V * 255.0f + 0.5f))); };
	return Byte(C.R) << 16 | Byte(C.G) << 8 | Byte(C.B);
}

/** The 16 tokens of a skin as 0xRRGGBB, in pal:: order. */
std::array<uint32_t, 16> HexesLook(const ss::ui::Skin& K)
{
	const ss::ui::Color* T[16] = {&K.Bg, &K.Bg2, &K.Panel, &K.Panel2, &K.Line, &K.Ink, &K.Muted, &K.Dim, &K.Accent, &K.Accent2, &K.Gold, &K.Red, &K.Orange, &K.Green, &K.Felt, &K.FeltDark};
	std::array<uint32_t, 16> Out{};
	for (size_t I = 0; I < 16; ++I)
	{
		Out[I] = RgbLook(*T[I]);
	}
	return Out;
}

/** The look's host skin, dressed (§2.7.3's festival branch). */
ss::ui::Skin DressedLook(Brand B)
{
	return festart::Dressed(ss::ui::Skin::FromHex(ss::net::Def(HostLook(B)).Palette), festart::LookOf(B));
}

/** WCAG 2 contrast (the engine's SitesInkContrast, which SitesLabelInk uses). */
double ContrastLook(uint32_t A, uint32_t B)
{
	return ss::ui::rlsites_detail::SitesInkContrast(ss::ui::Hex(A), ss::ui::Hex(B));
}

/** SitesMixHex (UI/RiverLineSites.cpp:1349): FeltLight = MixHexLook(Felt, Accent2, 0.18) (§2.7.2). */
uint32_t MixHexLook(uint32_t A, uint32_t B, float T)
{
	uint32_t Out = 0;
	for (int Shift = 16; Shift >= 0; Shift -= 8)
	{
		const float Ca = static_cast<float>((A >> Shift) & 0xff);
		const float Cb = static_cast<float>((B >> Shift) & 0xff);
		Out |= static_cast<uint32_t>(std::lround(Ca + (Cb - Ca) * T)) << Shift;
	}
	return Out;
}

/** "win27" -> "win" (a series id's slot, as EventArt's SlotOf reads a RiverLine id). */
std::string SlotLook(const std::string& Id)
{
	size_t N = 0;
	while (N < Id.size() && std::isalpha(static_cast<unsigned char>(Id[N])))
	{
		++N;
	}
	return Id.substr(0, N);
}

struct LabLook
{
	double L, A, B;
};

/** sRGB to CIELAB (D65), champs/champ_revise.py's srgb2lab. */
LabLook ToLabLook(uint32_t C)
{
	double V[3] = {((C >> 16) & 255) / 255.0, ((C >> 8) & 255) / 255.0, (C & 255) / 255.0};
	for (double& X : V)
	{
		X = X <= 0.04045 ? X / 12.92 : std::pow((X + 0.055) / 1.055, 2.4);
	}
	const double X = (0.4124 * V[0] + 0.3576 * V[1] + 0.1805 * V[2]) / 0.95047;
	const double Y = 0.2126 * V[0] + 0.7152 * V[1] + 0.0722 * V[2];
	const double Z = (0.0193 * V[0] + 0.1192 * V[1] + 0.9505 * V[2]) / 1.08883;
	const auto F = [](double T) { return T > 0.008856 ? std::cbrt(T) : 7.787 * T + 16.0 / 116.0; };
	return {116.0 * F(Y) - 16.0, 500.0 * (F(X) - F(Y)), 200.0 * (F(Y) - F(Z))};
}

/** CIEDE2000, ported line for line from champs/champ_revise.py:186-212 (§2.7.2's distance; R-12). */
double De2000Look(uint32_t C1, uint32_t C2)
{
	const double Pi = 3.14159265358979323846;
	const auto Rad = [Pi](double D) { return D * Pi / 180.0; };
	const auto Deg = [Pi](double R) { return R * 180.0 / Pi; };
	const auto Wrap = [](double D) { return std::fmod(std::fmod(D, 360.0) + 360.0, 360.0); };
	const LabLook P = ToLabLook(C1);
	const LabLook Q = ToLabLook(C2);
	const double Ca = std::hypot(P.A, P.B), Cb = std::hypot(Q.A, Q.B), Cm = (Ca + Cb) / 2.0;
	const double G = 0.5 * (1.0 - std::sqrt(std::pow(Cm, 7) / (std::pow(Cm, 7) + std::pow(25.0, 7))));
	const double A1 = P.A * (1.0 + G), A2 = Q.A * (1.0 + G);
	const double C1p = std::hypot(A1, P.B), C2p = std::hypot(A2, Q.B);
	const double H1 = Wrap(Deg(std::atan2(P.B, A1))), H2 = Wrap(Deg(std::atan2(Q.B, A2)));
	const double DL = Q.L - P.L, DC = C2p - C1p;
	double Dh = H2 - H1;
	if (C1p * C2p == 0.0)
	{
		Dh = 0.0;
	}
	else if (Dh > 180.0)
	{
		Dh -= 360.0;
	}
	else if (Dh < -180.0)
	{
		Dh += 360.0;
	}
	const double DH = 2.0 * std::sqrt(C1p * C2p) * std::sin(Rad(Dh / 2.0));
	const double Lm = (P.L + Q.L) / 2.0, Cmp = (C1p + C2p) / 2.0;
	double Hm = 0.0;
	if (C1p * C2p == 0.0)
	{
		Hm = H1 + H2;
	}
	else if (std::fabs(H1 - H2) <= 180.0)
	{
		Hm = (H1 + H2) / 2.0;
	}
	else if (H1 + H2 < 360.0)
	{
		Hm = (H1 + H2 + 360.0) / 2.0;
	}
	else
	{
		Hm = (H1 + H2 - 360.0) / 2.0;
	}
	const double T = 1.0 - 0.17 * std::cos(Rad(Hm - 30.0)) + 0.24 * std::cos(Rad(2.0 * Hm)) + 0.32 * std::cos(Rad(3.0 * Hm + 6.0)) - 0.20 * std::cos(Rad(4.0 * Hm - 63.0));
	const double Dth = 30.0 * std::exp(-std::pow((Hm - 275.0) / 25.0, 2.0));
	const double Rc = 2.0 * std::sqrt(std::pow(Cmp, 7) / (std::pow(Cmp, 7) + std::pow(25.0, 7)));
	const double Sl = 1.0 + 0.015 * (Lm - 50.0) * (Lm - 50.0) / std::sqrt(20.0 + (Lm - 50.0) * (Lm - 50.0));
	const double Sc = 1.0 + 0.045 * Cmp;
	const double Sh = 1.0 + 0.015 * Cmp * T;
	const double Rt = -std::sin(Rad(2.0 * Dth)) * Rc;
	return std::sqrt((DL / Sl) * (DL / Sl) + (DC / Sc) * (DC / Sc) + (DH / Sh) * (DH / Sh) + Rt * (DC / Sc) * (DH / Sh));
}

/** A hex of the host's own palette (Ignite's night palette too: Afterburner Hours are never dressed, §2.7.3). */
bool HostHexLook(SiteId Host, uint32_t C)
{
	const ss::net::SiteDef& D = ss::net::Def(Host);
	return std::find(D.Palette.begin(), D.Palette.end(), C) != D.Palette.end() ||
		(Host == SiteId::Ignite && std::find(D.NightPalette.begin(), D.NightPalette.end(), C) != D.NightPalette.end());
}

/** A look's Accent, Accent2 and Felt (the three tokens the distance rule compares), with their names. */
std::vector<std::pair<const char*, uint32_t>> TriadLook(Brand B)
{
	const festart::FestivalLook& L = festart::LookOf(B);
	return {{"Accent", L.Accent}, {"Accent2", L.Accent2}, {"Felt", L.Felt}};
}

/**
 * FestLookDistance (§2.7.2's distance rule, §2.15.2; R-12, PLAN_G1 C-12): v1.6's three new tokens (PENNANT's Accent and Felt,
 * REIGN's Accent2) each >= 12 CIEDE2000 from every other look's Accent, Accent2 and Felt and every series colour on any site.
 * Exempt: the token's own host palette (Ignite's night palette with it), a look token that is its own host's palette hex
 * (RunItTwice's unchanged red, REIGN's black cloth) and the gold tokens as one family. Skipped: the token's own brand's series (none
 * before F-net's calendar) and RiverLine's seasonal plans (win, spr, hol: RiverLine's palette family in champ_revise §7's basis, the
 * basis of §2.7.2's 17.1/12.4/16.8; printed, not asserted). The whole matrix and v1.0's pairs are printed, not asserted.
 * Planted bug: PENNANT's Accent set to v1.0's Marquee Blue 5b8cff (4.8 from TCO's 7b96ff).
 */
void FestLookDistance()
{
	std::set<uint32_t> Golds;
	for (const Brand B : AllLook)
	{
		Golds.insert(festart::LookOf(B).Gold);
	}
	struct NewToken
	{
		Brand B;
		const char* Field;
		uint32_t Hex;
	};
	const NewToken Tokens[] = {{Brand::Pennant, "Accent", festart::LookOf(Brand::Pennant).Accent}, {Brand::Pennant, "Felt", festart::LookOf(Brand::Pennant).Felt},
		{Brand::Reign, "Accent2", festart::LookOf(Brand::Reign).Accent2}};
	const SiteId Sites[] = {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice};
	for (const NewToken& N : Tokens)
	{
		const SiteId Host = HostLook(N.B);
		const auto Exempt = [&](uint32_t C) { return HostHexLook(Host, C) || Golds.count(C) > 0; };
		std::vector<std::pair<double, std::string>> Near, Seasonal;
		char Buf[160];
		for (const Brand B : AllLook)
		{
			if (B == N.B)
			{
				continue;
			}
			for (const auto& T : TriadLook(B))
			{
				if (!Exempt(T.second) && !HostHexLook(HostLook(B), T.second))
				{
					std::snprintf(Buf, sizeof(Buf), "%s %s %06x", NameLook(B), T.first, T.second);
					Near.push_back({De2000Look(N.Hex, T.second), Buf});
				}
			}
		}
		int SeriesSeen = 0;
		for (const SiteId S : Sites)
		{
			for (const ss::net::SeriesInfo& Sr : ss::net::Site(S).Series())
			{
				const festart::FestivalLook* Own = festart::LookFor(Sr);
				if ((Own != nullptr && Own->Which == N.B) || Sr.Brand == ss::fest::BrandKey(N.B))
				{
					continue; // the token's own festival's series
				}
				const std::string Slot = SlotLook(Sr.Id);
				const bool Season = S == SiteId::RiverLine && (Slot == "win" || Slot == "spr" || Slot == "hol");
				++SeriesSeen;
				for (const uint32_t C : {Sr.Color, Sr.Color2})
				{
					if (Exempt(C))
					{
						continue;
					}
					std::snprintf(Buf, sizeof(Buf), "series %s %06x", Sr.Id.c_str(), C);
					(Season ? Seasonal : Near).push_back({De2000Look(N.Hex, C), Buf});
				}
			}
		}
		std::sort(Near.begin(), Near.end());
		std::sort(Seasonal.begin(), Seasonal.end());
		std::printf("  look distance: %s %s %06x against %d colours (%d series): nearest", NameLook(N.B), N.Field, N.Hex, static_cast<int>(Near.size()), SeriesSeen);
		for (size_t K = 0; K < Near.size() && K < 3; ++K)
		{
			std::printf("%s %s dE %.1f", K ? ";" : "", Near[K].second.c_str(), Near[K].first);
		}
		if (!Seasonal.empty())
		{
			std::printf(" | RiverLine's seasonal plans, not asserted: %s dE %.1f", Seasonal.front().second.c_str(), Seasonal.front().first);
		}
		std::printf("\n");
		ExpectLook(!Near.empty() && Near.front().first >= 12.0,
			std::string("FestLookDistance: ") + NameLook(N.B) + "'s new " + N.Field + " is at least 12 (CIEDE2000) from every other look's Accent, Accent2 and Felt and every series colour");
	}
	// The whole matrix (QF-2): every look's Accent, Accent2 and Felt against every other look's.
	std::printf("  look distance matrix (CIEDE2000; rows and columns: each look's Accent, Accent2, Felt in fest::Brand order):\n");
	for (const Brand Row : AllLook)
	{
		for (const auto& R : TriadLook(Row))
		{
			std::printf("    %-11s %-7s %06x:", NameLook(Row), R.first, R.second);
			for (const Brand Col : AllLook)
			{
				for (const auto& C : TriadLook(Col))
				{
					if (Col == Row)
					{
						std::printf("     -");
					}
					else
					{
						std::printf(" %5.1f", De2000Look(R.second, C.second));
					}
				}
			}
			std::printf("\n");
		}
	}
	// v1.0's looks predate the rule (R-12): printed, not asserted. Basis (qa2's): the eight v1.0 looks' Accent, Accent2 and Felt, an
	// unordered pair once, a token that is its own host's palette hex exempt (RunItTwice's unchanged red), the golds left out.
	const Brand V10[] = {Brand::Rcop, Brand::RingRush, Brand::Tco, Brand::Slam, Brand::Feign, Brand::Push, Brand::Writ, Brand::Grit};
	int Pairs = 0;
	for (size_t I = 0; I < 8; ++I)
	{
		for (size_t J = I + 1; J < 8; ++J)
		{
			for (const auto& A : TriadLook(V10[I]))
			{
				for (const auto& B : TriadLook(V10[J]))
				{
					if (HostHexLook(HostLook(V10[I]), A.second) || HostHexLook(HostLook(V10[J]), B.second))
					{
						continue;
					}
					const double D = De2000Look(A.second, B.second);
					if (D < 12.0)
					{
						++Pairs;
						std::printf("    v1.0 pair under 12 (printed, not asserted): %s %s %06x / %s %s %06x dE %.1f\n", NameLook(V10[I]), A.first, A.second, NameLook(V10[J]), B.first, B.second, D);
					}
				}
			}
		}
	}
	std::printf("  look distance: %d unordered sub-12 Accent/Accent2/Felt pairs among v1.0's eight looks (§2.7.2: 12; not asserted)\n", Pairs);
}

/**
 * FestContrast, the token half (§2.15.2; FL-4 adds the pages at both scales): for the ten looks on their hosts, accent text on Bg
 * >= 4.5 on RiverLine and Ignite looks; the label on a filled accent >= 4.5 through SitesLabelInk (the accent's dark mix or Ink,
 * whichever reads higher, as SitesOnAccent and the solid pills pick it inside a skin); Ink on Panel and Muted on Bg >= 4.5 (host
 * tokens, never swapped). As data (§2.7.3): RunItTwice's looks keep Signal Red d2141a as Accent and Hot White ffffff as Gold (red
 * is light, never ink); REIGN swaps Accent2 only and keeps the club's own black cloth and Bg2.
 * Planted bug: Ring Rush's accent c84a5e (4.14:1 on Bg 0a111c).
 */
void FestContrast()
{
	double Lowest = 99.0;
	Brand LowestB = Brand::None;
	for (const Brand B : AllLook)
	{
		const festart::FestivalLook& L = festart::LookOf(B);
		const SiteId Host = HostLook(B);
		const ss::ui::Skin K = DressedLook(B);
		const std::array<uint32_t, 16> H = HexesLook(K);
		const uint32_t Bg = H[0], Panel = H[2], Ink = H[5], Muted = H[6], Accent = H[8];
		uint32_t Label = 0;
		{
			ss::ui::SkinScope Scope(K);
			Label = RgbLook(ss::ui::rlsites_detail::SitesLabelInk(ss::ui::rlsites_detail::SitesOnAccent(), ss::ui::pal::Accent, ss::ui::pal::Accent));
		}
		const double OnBg = ContrastLook(Accent, Bg), Lab = ContrastLook(Label, Accent);
		std::printf("  look contrast: %-11s %-10s accent %06x on Bg %.2f, on Panel %.2f | label %06x on accent %.2f | Ink on Panel %.2f | Muted on Bg %.2f\n", NameLook(B),
			ss::net::Def(Host).Name.c_str(), Accent, OnBg, ContrastLook(Accent, Panel), Label, Lab, ContrastLook(Ink, Panel), ContrastLook(Muted, Bg));
		const std::string Who = std::string(" (") + NameLook(B) + ")";
		if (Host != SiteId::RunItTwice)
		{
			ExpectLook(OnBg >= 4.5, "FestContrast: accent text on Bg reads at 4.5:1 or more on a RiverLine or Ignite look" + Who);
			if (OnBg < Lowest)
			{
				Lowest = OnBg;
				LowestB = B;
			}
		}
		ExpectLook(Lab >= 4.5, "FestContrast: the label on a filled accent reads at 4.5:1 or more through SitesLabelInk" + Who);
		ExpectLook(ContrastLook(Ink, Panel) >= 4.5 && ContrastLook(Muted, Bg) >= 4.5, "FestContrast: Ink on Panel and Muted on Bg read at 4.5:1 or more" + Who);
		if (Host == SiteId::RunItTwice)
		{
			ExpectLook(L.Accent == 0xd2141au && L.Gold == 0xffffffu && Label != 0xd2141au,
				"FestContrast: a RunItTwice look keeps Signal Red as Accent and Hot White as Gold, and red is never the ink" + Who);
		}
	}
	const ss::net::SiteDef& Rit = ss::net::Def(SiteId::RunItTwice);
	const festart::FestivalLook& Reign = festart::LookOf(Brand::Reign);
	ExpectLook(Reign.Felt == Rit.Palette[14] && Reign.FeltDark == Rit.Palette[15] && Reign.Bg2 == Rit.Palette[1],
		"FestContrast: REIGN keeps the club's own black cloth and Bg2 and swaps Accent2 only (§2.7.3)");
	std::printf("  look contrast: lowest accent on Bg among RiverLine and Ignite looks: %s %.2f:1 (§2.7.2: 5.16)\n", NameLook(LowestB), Lowest);
}

/**
 * FestLeak, the table half (§2.15.2; FL-4 adds SitesNoLeak on the festival pages): no Ignite or RunItTwice look carries a RiverLine
 * palette hex in its dressed skin's 16 tokens, its FeltLight or its Kit colour. And no look carries another's identity: each row is
 * its brand's, its cache Key is Fnv1a("fest:" + the brand key) and unique (a shared key would replay one festival's cached art in
 * another's), its sting is its host's (§2.7.6), the empty look dresses nothing and a plain series has no look.
 * Planted bug: FEIGN's Bg2 0e1726 (RiverLine's Bg2).
 */
void FestLeak()
{
	const std::array<uint32_t, 16>& Home = ss::net::Def(SiteId::RiverLine).Palette;
	std::set<uint64_t> Keys;
	for (const Brand B : AllLook)
	{
		const festart::FestivalLook& L = festart::LookOf(B);
		const SiteId Host = HostLook(B);
		ExpectLook(L.Which == B && L.Key == ss::Fnv1a(std::string("fest:") + ss::fest::BrandKey(B)) && Keys.insert(L.Key).second,
			std::string("FestLeak: the look is its brand's own, keyed Fnv1a(\"fest:\" + its key), no key shared (") + NameLook(B) + ")");
		ExpectLook(L.Sting == StingLook(Host), std::string("FestLeak: the look plays its host's sting (") + NameLook(B) + ")");
		// Dressed swaps the six tokens and keeps the host's other ten (§2.7.3), so nothing of another palette rides along.
		const std::array<uint32_t, 16> Worn = HexesLook(DressedLook(B));
		const std::array<uint32_t, 16>& Own = ss::net::Def(Host).Palette;
		bool Exact = Worn[1] == L.Bg2 && Worn[8] == L.Accent && Worn[9] == L.Accent2 && Worn[10] == L.Gold && Worn[14] == L.Felt && Worn[15] == L.FeltDark;
		const size_t Kept[] = {0, 2, 3, 4, 5, 6, 7, 11, 12, 13}; // Bg, Panel, Panel2, Line, Ink, Muted, Dim, Red, Orange, Green
		for (const size_t I : Kept)
		{
			Exact = Exact && Worn[I] == Own[I];
		}
		ExpectLook(Exact, std::string("FestLeak: the look dresses its host in exactly its six tokens and keeps the other ten (") + NameLook(B) + ")");
		if (Host == SiteId::RiverLine)
		{
			continue;
		}
		std::vector<uint32_t> Tokens;
		for (const uint32_t C : HexesLook(DressedLook(B)))
		{
			Tokens.push_back(C);
		}
		Tokens.push_back(MixHexLook(L.Felt, L.Accent2, 0.18f));
		if (L.Kit != 0)
		{
			Tokens.push_back(L.Kit);
		}
		std::string Leaked;
		for (const uint32_t C : Tokens)
		{
			if (std::find(Home.begin(), Home.end(), C) != Home.end())
			{
				char Buf[12];
				std::snprintf(Buf, sizeof(Buf), " %06x", C);
				Leaked += Buf;
			}
		}
		std::printf("  look leak: %-8s on %-10s: %s\n", NameLook(B), ss::net::Def(Host).Name.c_str(), Leaked.empty() ? "no RiverLine hex" : ("RiverLine hexes" + Leaked).c_str());
		ExpectLook(Leaked.empty(), std::string("FestLeak: no RiverLine palette hex in a rival festival's look (") + NameLook(B) + ")");
	}
	const ss::ui::Skin Base = ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).Palette);
	ss::net::SeriesInfo Plain;
	Plain.Id = "mm";
	ss::net::SeriesInfo Rcop;
	Rcop.Id = "rcop27";
	ExpectLook(festart::LookOf(Brand::None).Which == Brand::None && festart::Dressed(Base, festart::LookOf(Brand::None)).Key() == Base.Key() &&
			festart::LookFor(Plain) == nullptr && festart::LookFor(Rcop) == &festart::LookOf(Brand::Rcop),
		"FestLeak: the empty look dresses nothing, a plain series has no look and a festival's series wears its own");
}

/** FNV-1a over raw bytes (a draw list's fingerprint). */
uint64_t FnvLook(uint64_t H, const void* Data, size_t Size)
{
	const unsigned char* B = static_cast<const unsigned char*>(Data);
	for (size_t K = 0; K < Size; ++K)
	{
		H = (H ^ B[K]) * 0x100000001b3ull;
	}
	return H;
}

/** What one drawing put on a fresh canvas: its vertices, a fingerprint of everything drawn (geometry, colours, text), its visible
 *  height and whether a vertex wears Mark exactly (a thin line's alpha follows its width). */
struct CrestRunLook
{
	size_t Vertices = 0;
	uint64_t Hash = 0;
	float Width = 0.0f;
	float Height = 0.0f;
	bool Mark = false;
};

CrestRunLook RunCrestLook(const std::function<void(ss::ui::Canvas&)>& Paint, float Scale, uint32_t Mark = 0)
{
	ui_test::TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 400.0f, 400.0f, Scale);
	Paint(C);
	CrestRunLook R;
	R.Vertices = L.Vertices.size();
	uint64_t H = 0xcbf29ce484222325ull;
	float Top = 1e9f;
	float Bottom = -1e9f;
	float Left = 1e9f;
	float Right = -1e9f;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		const float F[6] = {V.X, V.Y, V.Col.R, V.Col.G, V.Col.B, V.Col.A};
		H = FnvLook(H, F, sizeof(F));
		if (V.Col.A > 0.02f)
		{
			Top = std::min(Top, V.Y);
			Bottom = std::max(Bottom, V.Y);
			Left = std::min(Left, V.X);
			Right = std::max(Right, V.X);
		}
		R.Mark = R.Mark || (Mark != 0 && V.Col.A > 0.2f && RgbLook(V.Col) == Mark);
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			H = FnvLook(H, Cmd.Text.Text.data(), Cmd.Text.Text.size());
		}
	}
	R.Hash = H;
	R.Height = Bottom > Top ? Bottom - Top : 0.0f;
	R.Width = Right > Left ? Right - Left : 0.0f;
	return R;
}

/** A look's series as the crests see it: RiverLine's four from the network (as the pages draw them); the six rival festivals as F-net
 *  lists them (fest::All is F-net's, empty here): "<site key>.<brand key><yy>", Color and Color2 the look's Accent and Accent2. */
ss::net::SeriesInfo CrestSeriesLook(Brand B)
{
	static const char* const Ids[] = {"", "rcop27", "ring27", "tco27", "slam27", "ignite.feign26", "ignite.push27", "runittwice.writ27", "runittwice.grit27",
		"ignite.pennant27", "runittwice.reign27"};
	const char* Id = Ids[static_cast<int>(B)];
	if (const ss::net::SeriesInfo* Real = ss::net::Site(SiteId::RiverLine).FindSeries(Id))
	{
		return *Real;
	}
	ss::net::SeriesInfo S;
	S.Id = Id;
	S.Name = std::string(NameLook(B)) + (B == Brand::Feign ? " 2026" : " 2027");
	S.Short = NameLook(B);
	S.Color = festart::LookOf(B).Accent;
	S.Color2 = festart::LookOf(B).Accent2;
	S.Festival = true;
	S.Brand = ss::fest::BrandKey(B);
	return S;
}

/** An outline's width and height. */
std::pair<float, float> ExtentLook(const std::vector<ss::ui::Vec2>& P)
{
	float X0 = 1e9f, X1 = -1e9f, Y0 = 1e9f, Y1 = -1e9f;
	for (const ss::ui::Vec2& Q : P)
	{
		X0 = std::min(X0, Q.X);
		X1 = std::max(X1, Q.X);
		Y0 = std::min(Y0, Q.Y);
		Y1 = std::max(Y1, Q.Y);
	}
	return {X1 - X0, Y1 - Y0};
}

/** The shoelace area of a closed outline. */
float AreaLook(const std::vector<ss::ui::Vec2>& P)
{
	float A = 0.0f;
	for (size_t I = 0; I < P.size(); ++I)
	{
		const ss::ui::Vec2& Q = P[I];
		const ss::ui::Vec2& R = P[(I + 1) % P.size()];
		A += Q.X * R.Y - R.X * Q.Y;
	}
	return std::fabs(A) * 0.5f;
}

/** Whether a closed outline holds the point (0, 0) (the crest's centre, where the motif sits). */
bool HoldsCentreLook(const std::vector<ss::ui::Vec2>& P)
{
	bool In = false;
	for (size_t I = 0, J = P.size() - 1; I < P.size(); J = I++)
	{
		if ((P[I].Y > 0.0f) != (P[J].Y > 0.0f) && 0.0f < (P[J].X - P[I].X) * (0.0f - P[I].Y) / (P[J].Y - P[I].Y) + P[I].X)
		{
			In = !In;
		}
	}
	return In;
}

/**
 * FestCrestLowDetail (§2.7.4's Crests and Cache rows, §2.15.2, §8.5): every look's crest, at both scales (1 and 1.5 pixels a unit), is
 * at most 0.9k vertices at 26 units (the low-detail path: the frame's silhouette and the motif, filling its box: "drop detail, don't
 * shrink") and at most 5.5k at 150. A festival's series finds its look (SlotOf strips a rival's site key) and wears its frame and motif;
 * a rival festival never wears the generic star and medallion a plain rival series does (its crest differs from its plain twin's); the
 * ten looks' crests are pairwise distinct (frame and motif, and as drawn); RCOP's crest carries its silver wave band and no other look's
 * does; the six festival glyphs and frames draw their own bodies (not the star, not the medallion). Printed: each look's counts and the
 * crest cache (3 sizes x 10 looks x 2 scales). QF-3: a series F-net flags as a festival finds the look of its own brand (vacuous until
 * F-net's calendar).
 * G1 FL-7 (QF-10, accepted: no new check name): the stills sub-assertion and the word-marks, FestMotionsLook below.
 * Planted bug: SlotOf unstripped ("ignite.feign26" reads "ignite": every rival festival loses its look). FL-7's: Curtains animating while
 * Still.
 */
void FestMotionsLook();

void FestCrestLowDetail()
{
	namespace ea = ss::ui::eventart;
	namespace ead = ss::ui::eventart::eventart_detail;
	const float Scales[2] = {1.0f, 1.5f};
	const uint32_t Silver = festart::LookOf(Brand::Rcop).Accent2; // RCOP's Crest Silver: the wave band's middle line
	std::vector<uint64_t> Drawn;
	std::set<std::pair<int, int>> Kinds;
	// The low-detail crest is its frame's silhouette (the motif inside it), so its extents are its frame's outline at one scale a unit:
	// RCOP's shield (its frame by slot as by look) sets that scale, and every look's 26-unit crest must match its own frame's outline.
	const ss::net::SeriesInfo RcopSeries = CrestSeriesLook(Brand::Rcop);
	const float ShieldHigh = ExtentLook(ead::FramePts(static_cast<ead::Frame>(festart::LookOf(Brand::Rcop).Frame), 1.0f)).second;
	size_t CacheVertices = 0;
	int CacheCrests = 0;
	for (const Brand B : AllLook)
	{
		const festart::FestivalLook& L = festart::LookOf(B);
		const ss::net::SeriesInfo S = CrestSeriesLook(B);
		const std::string Who = std::string(" (") + NameLook(B) + ", " + S.Id + ")";
		ExpectLook(festart::LookFor(S) == &L && ea::SeriesGlyph(S) == L.Glyph, "FestCrestLowDetail: a festival's series finds its look and wears its motif" + Who);
		Kinds.insert({L.Frame, static_cast<int>(L.Glyph)});
		size_t Small[2] = {0, 0};
		size_t Big[2] = {0, 0};
		for (int K = 0; K < 2; ++K)
		{
			const CrestRunLook At26 = RunCrestLook([&S](ss::ui::Canvas& C) { ea::SeriesCrest(C, S, 200.0f, 200.0f, 26.0f, 0.0); }, Scales[K], Silver);
			const CrestRunLook At46 = RunCrestLook([&S](ss::ui::Canvas& C) { ea::SeriesCrest(C, S, 200.0f, 200.0f, 46.0f, 0.0); }, Scales[K], Silver);
			const CrestRunLook At150 = RunCrestLook([&S](ss::ui::Canvas& C) { ea::SeriesCrest(C, S, 200.0f, 200.0f, 150.0f, 0.0); }, Scales[K], Silver);
			Small[K] = At26.Vertices;
			Big[K] = At150.Vertices;
			CacheVertices += At26.Vertices + At46.Vertices + At150.Vertices;
			CacheCrests += 3;
			char Scale[24];
			std::snprintf(Scale, sizeof(Scale), ", scale %.1f", static_cast<double>(Scales[K]));
			ExpectLook(At26.Vertices <= 900, "FestCrestLowDetail: a festival crest at 26 is at most 0.9k vertices" + Who + Scale);
			ExpectLook(At150.Vertices <= 5500, "FestCrestLowDetail: a festival crest at 150 is at most 5.5k vertices" + Who + Scale);
			ExpectLook(At26.Height >= 0.78f * 26.0f, "FestCrestLowDetail: the low-detail crest fills its box (drop detail, don't shrink)" + Who + Scale);
			const float Unit = RunCrestLook([&RcopSeries](ss::ui::Canvas& C) { ea::SeriesCrest(C, RcopSeries, 200.0f, 200.0f, 26.0f, 0.0); }, Scales[K]).Height / ShieldHigh;
			const std::pair<float, float> Own = ExtentLook(ead::FramePts(static_cast<ead::Frame>(L.Frame), 1.0f));
			ExpectLook(std::fabs(At26.Width - Own.first * Unit) <= 0.03f && std::fabs(At26.Height - Own.second * Unit) <= 0.03f,
				"FestCrestLowDetail: a festival crest wears its look's frame (its silhouette's extents)" + Who + Scale);
			ExpectLook((At150.Mark && At46.Mark) == (B == Brand::Rcop) && !At26.Mark,
				"FestCrestLowDetail: RCOP's crest carries its silver wave band above 30 units, no other look's does" + Who + Scale);
			if (K == 0)
			{
				Drawn.push_back(At150.Hash);
			}
			if (HostLook(B) != SiteId::RiverLine)
			{
				// Its plain twin: the same series under a slot no festival owns draws the generic medallion and star.
				ss::net::SeriesInfo Twin = S;
				Twin.Id = (HostLook(B) == SiteId::Ignite ? "ignite.plain" : "runittwice.plain") + S.Id.substr(S.Id.size() - 2);
				Twin.Festival = false;
				Twin.Brand.clear();
				const CrestRunLook Twin26 = RunCrestLook([&Twin](ss::ui::Canvas& C) { ea::SeriesCrest(C, Twin, 200.0f, 200.0f, 26.0f, 0.0); }, Scales[K]);
				const CrestRunLook Twin150 = RunCrestLook([&Twin](ss::ui::Canvas& C) { ea::SeriesCrest(C, Twin, 200.0f, 200.0f, 150.0f, 0.0); }, Scales[K]);
				ExpectLook(ea::SeriesGlyph(Twin) == ea::Glyph::Star && L.Glyph != ea::Glyph::Star && L.Frame != 3 && Twin26.Hash != At26.Hash && Twin150.Hash != At150.Hash,
					"FestCrestLowDetail: a rival festival's crest is not the generic star and medallion of a plain rival series" + Who + Scale);
			}
		}
		std::printf("  crest: %-11s %-19s frame %2d glyph %2d rim %d | at 26: %zu, %zu vertices | at 150: %zu, %zu (scales 1, 1.5)\n", NameLook(B), S.Id.c_str(), L.Frame,
			static_cast<int>(L.Glyph), static_cast<int>(L.RimKind), Small[0], Small[1], Big[0], Big[1]);
	}
	ExpectLook(Kinds.size() == 10 && std::set<uint64_t>(Drawn.begin(), Drawn.end()).size() == Drawn.size(),
		"FestCrestLowDetail: the ten looks' crests are pairwise distinct (frame and motif, and as drawn)");
	std::printf("  crest cache: %d crests (3 sizes x 10 looks x 2 scales) drew %zu vertices, the cached layers' upper bound (Shared pool: 1.5M; §2.7.4: ~188k)\n",
		CacheCrests, CacheVertices);

	// The six motifs draw their own bodies, each unlike the star they were drawn as before G1 and unlike each other.
	const auto GlyphRun = [](ea::Glyph G) { return RunCrestLook([G](ss::ui::Canvas& C) { ea::DrawGlyph(C, G, 200.0f, 200.0f, 72.0f); }, 1.0f); };
	const uint64_t Star = GlyphRun(ea::Glyph::Star).Hash;
	std::set<uint64_t> Motifs;
	for (int G = static_cast<int>(ea::Glyph::Mirror); G < static_cast<int>(ea::Glyph::Count); ++G)
	{
		const CrestRunLook R = GlyphRun(static_cast<ea::Glyph>(G));
		std::printf("  crest motif %d: %zu vertices at 72\n", G, R.Vertices);
		ExpectLook(R.Hash != Star && Motifs.insert(R.Hash).second, "FestCrestLowDetail: each festival motif draws its own body, not the star (glyph " + std::to_string(G) + ")");
	}

	// The six frames draw their own outlines: not the medallion's, each holding the centre with room for the motif, inside the unit box.
	const std::vector<ss::ui::Vec2> Medallion = ead::FramePts(static_cast<ead::Frame>(3), 1.0f);
	std::vector<std::vector<ss::ui::Vec2>> Outlines;
	for (const Brand B : AllLook)
	{
		if (HostLook(B) == SiteId::RiverLine)
		{
			continue;
		}
		const std::vector<ss::ui::Vec2> P = ead::FramePts(static_cast<ead::Frame>(festart::LookOf(B).Frame), 1.0f);
		float Reach = 0.0f;
		for (const ss::ui::Vec2& Q : P)
		{
			Reach = std::max(Reach, std::max(std::fabs(Q.X), std::fabs(Q.Y)));
		}
		const bool Own = P.size() != Medallion.size() || !std::equal(P.begin(), P.end(), Medallion.begin(), [](const ss::ui::Vec2& A, const ss::ui::Vec2& C) {
			return A.X == C.X && A.Y == C.Y;
		});
		bool New = true;
		for (const std::vector<ss::ui::Vec2>& O : Outlines)
		{
			New = New && !(O.size() == P.size() && std::equal(O.begin(), O.end(), P.begin(), [](const ss::ui::Vec2& A, const ss::ui::Vec2& C) { return A.X == C.X && A.Y == C.Y; }));
		}
		Outlines.push_back(P);
		std::printf("  crest frame %d (%s): %zu points, area %.2f, reach %.2f\n", festart::LookOf(B).Frame, NameLook(B), P.size(), static_cast<double>(AreaLook(P)), static_cast<double>(Reach));
		ExpectLook(Own && New && HoldsCentreLook(P) && AreaLook(P) >= 1.8f && Reach <= 1.06f,
			std::string("FestCrestLowDetail: a festival frame draws its own outline, round the centre, inside the unit box (") + NameLook(B) + ")");
	}

	// QF-3: every series F-net flags as a festival finds the look of its own brand (vacuous before F-net's calendar).
	int Flagged = 0;
	for (const SiteId SiteK : {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice})
	{
		for (const ss::net::SeriesInfo& Sr : ss::net::Site(SiteK).Series())
		{
			if (!Sr.Festival)
			{
				continue;
			}
			++Flagged;
			const festart::FestivalLook* Own = festart::LookFor(Sr);
			ExpectLook(Own != nullptr && Own->Which == ss::fest::BrandOf(Sr.Brand), "FestCrestLowDetail: a festival series finds its own brand's look (" + Sr.Id + ")");
		}
	}
	std::printf("  crest: %d series flagged Festival (F-net's data) each find their own brand's look\n", Flagged);
	FestMotionsLook();
}

/** One drawing of a cached festival piece on a fresh canvas at Scale: whether its Key was cached before and after the drawing, whether
 *  a second drawing on another canvas put down exactly the same (the copy), its vertices, a fingerprint, its measured height (the
 *  vertices at alpha 0.25 or more: solid art, not its anti-aliased fringe or a faint glow) and the topmost vertex wearing Mark. */
struct ArtRunLook
{
	size_t Vertices = 0;
	uint64_t Hash = 0;
	float Height = 0.0f;
	float MarkTop = 1e9f;
	bool Fresh = false;
	bool Cached = false;
	bool Copy = false;
};

uint64_t ListPrintLook(const ss::ui::DrawList& L)
{
	uint64_t H = 0xcbf29ce484222325ull;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		const float F[6] = {V.X, V.Y, V.Col.R, V.Col.G, V.Col.B, V.Col.A};
		H = FnvLook(H, F, sizeof(F));
	}
	return L.Indices.empty() ? H : FnvLook(H, L.Indices.data(), L.Indices.size() * sizeof(uint32_t));
}

ArtRunLook RunArtLook(const std::function<void(ss::ui::Canvas&)>& Paint, uint64_t Key, float Scale, uint32_t Mark = 0)
{
	ui_test::TableMeasurer M;
	ss::ui::DrawList First;
	ss::ui::DrawList Second;
	ss::ui::Canvas C1(First, M, 400.0f, 400.0f, Scale);
	ArtRunLook R;
	R.Fresh = !C1.IsCached(Key);
	Paint(C1);
	R.Cached = C1.IsCached(Key);
	ss::ui::Canvas C2(Second, M, 400.0f, 400.0f, Scale);
	Paint(C2);
	R.Vertices = First.Vertices.size();
	R.Hash = ListPrintLook(First);
	R.Copy = Second.Vertices.size() == R.Vertices && ListPrintLook(Second) == R.Hash;
	float Top = 1e9f;
	float Bottom = -1e9f;
	for (const ss::ui::Vertex& V : First.Vertices)
	{
		if (V.Col.A >= 0.25f)
		{
			Top = std::min(Top, V.Y);
			Bottom = std::max(Bottom, V.Y);
		}
		if (Mark != 0 && V.Col.A >= 0.2f && RgbLook(V.Col) == Mark)
		{
			R.MarkTop = std::min(R.MarkTop, V.Y);
		}
	}
	R.Height = Bottom > Top ? Bottom - Top : 0.0f;
	return R;
}

// ------------------------------------------------------------------ G1 FL-7: the word-marks, the banner and the motions (QF-10)

/** One drawing on a fresh 1600 x 962 canvas at Scale: its vertices and text runs (kept) and a fingerprint of all of it (geometry, colours,
 *  indices, and each run's text, place, size, face and colour: a letter that bobs or flaps changes it). */
struct MarkRunLook
{
	std::vector<ss::ui::Vertex> Verts;
	std::vector<ss::ui::TextItem> Runs;
	uint64_t Hash = 0;
};

MarkRunLook RunMarkLook(const std::function<void(ss::ui::Canvas&)>& Paint, float Scale = 1.0f)
{
	ui_test::TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1600.0f, 962.0f, Scale);
	Paint(C);
	MarkRunLook R;
	uint64_t H = ListPrintLook(L);
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		const ss::ui::TextItem& T = Cmd.Text;
		const float F[8] = {T.X, T.Y, T.BaselineY, T.SizePx, T.Col.R, T.Col.G, T.Col.B, T.Col.A};
		const int Face = static_cast<int>(T.Face);
		H = FnvLook(H, F, sizeof(F));
		H = FnvLook(H, &Face, sizeof(Face));
		H = FnvLook(H, T.Text.data(), T.Text.size());
		R.Runs.push_back(T);
	}
	R.Verts = L.Vertices;
	R.Hash = H;
	return R;
}

/** Whether B differs from A by more than float rounding: another vertex or run count, or a vertex or run moved by more than 0.02 units or
 *  recoloured by more than 0.004. Both are drawn live (a motion's frame against the same motion a hair from its rest or its end): a cached
 *  copy can be tessellated differently from a live drawing of the same frame (another origin), so no motion is judged against one. */
bool MovedLook(const MarkRunLook& A, const MarkRunLook& B)
{
	if (A.Verts.size() != B.Verts.size() || A.Runs.size() != B.Runs.size())
	{
		return true;
	}
	const auto Recoloured = [](const ss::ui::Color& P, const ss::ui::Color& Q) {
		return std::fabs(P.R - Q.R) > 0.004f || std::fabs(P.G - Q.G) > 0.004f || std::fabs(P.B - Q.B) > 0.004f || std::fabs(P.A - Q.A) > 0.004f;
	};
	for (size_t I = 0; I < A.Verts.size(); ++I)
	{
		const ss::ui::Vertex& P = A.Verts[I];
		const ss::ui::Vertex& Q = B.Verts[I];
		if (std::fabs(P.X - Q.X) > 0.02f || std::fabs(P.Y - Q.Y) > 0.02f || Recoloured(P.Col, Q.Col))
		{
			return true;
		}
	}
	for (size_t I = 0; I < A.Runs.size(); ++I)
	{
		const ss::ui::TextItem& P = A.Runs[I];
		const ss::ui::TextItem& Q = B.Runs[I];
		if (P.Text != Q.Text || std::fabs(P.X - Q.X) > 0.02f || std::fabs(P.BaselineY - Q.BaselineY) > 0.02f || Recoloured(P.Col, Q.Col))
		{
			return true;
		}
	}
	return false;
}

/** A festival's name per look, as F-net prints them (the word-mark prints the festival's name). */
std::string MarkNameLook(Brand B)
{
	static const char* const Names[] = {"", "RCOP 2026", "Ring Rush 2027", "The Championship Online 2027", "Summer Slam 2027", "FEIGN 2026", "PUSH 2027",
		"WRIT 2027", "GRIT 2027", "PENNANT 2027", "REIGN 2027"};
	return Names[static_cast<int>(B)];
}

/** §2.8.2's banner rect in the look's host (RunItTwice's in brackets there). */
ss::ui::Rect BannerRectLook(Brand B)
{
	return HostLook(B) == SiteId::RunItTwice ? ss::ui::Rect{232.0f, 144.0f, 1344.0f, 196.0f} : ss::ui::Rect{24.0f, 132.0f, 1552.0f, 204.0f};
}

/** The word-mark at the hub banner's size and place (§2.8.2: 46 at x 210, baseline 196). */
MarkRunLook MarkLook(Brand B, double Time, float Day, bool Still, float Scale = 1.0f)
{
	const std::string Name = MarkNameLook(B);
	return RunMarkLook([&](ss::ui::Canvas& C) { festart::FestWordMark(C, festart::LookOf(B), Name, 210.0f, 196.0f, 46.0f, Time, Day, Still); }, Scale);
}

MarkRunLook BannerLook(Brand B, double SinceOpen, float Day, bool MainDay, bool Still, float Scale = 1.0f)
{
	const ss::ui::Rect R = BannerRectLook(B);
	return RunMarkLook([&](ss::ui::Canvas& C) { festart::FestBanner(C, festart::LookOf(B), R, SinceOpen, Day, MainDay, Still); }, Scale);
}

MarkRunLook CrestAtLook(Brand B, float Size, double Time)
{
	const ss::net::SeriesInfo S = CrestSeriesLook(B);
	return RunMarkLook([&](ss::ui::Canvas& C) { ss::ui::eventart::SeriesCrest(C, S, 200.0f, 200.0f, Size, Time); });
}

MarkRunLook TrophyAtLook(Brand B, float Size, double Time, bool Lod)
{
	return RunMarkLook([&](ss::ui::Canvas& C) { festart::FestTrophy(C, B, 200.0f, 200.0f, Size, Time, Lod); });
}

/** A word-mark's readable runs (Hot White, or FEIGN's pink and mint letters), left to right. */
std::vector<ss::ui::TextItem> ReadableLook(const MarkRunLook& R, const festart::FestivalLook& L)
{
	std::vector<ss::ui::TextItem> Out;
	for (const ss::ui::TextItem& T : R.Runs)
	{
		const uint32_t C = RgbLook(T.Col);
		if (C == 0xffffff || (L.Which == Brand::Feign && (C == L.Accent || C == L.Accent2)))
		{
			Out.push_back(T);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const ss::ui::TextItem& A, const ss::ui::TextItem& B) { return A.X < B.X; });
	return Out;
}

/** The vertices wearing Rgb at alpha MinA or more: how many, and their mean x and y. */
struct InkAtLook
{
	int N = 0;
	float X = 0.0f, Y = 0.0f;
};

InkAtLook InkLook(const MarkRunLook& R, uint32_t Rgb, float MinA)
{
	InkAtLook Out;
	for (const ss::ui::Vertex& V : R.Verts)
	{
		if (V.Col.A >= MinA && RgbLook(V.Col) == Rgb)
		{
			++Out.N;
			Out.X += V.X;
			Out.Y += V.Y;
		}
	}
	if (Out.N > 0)
	{
		Out.X /= static_cast<float>(Out.N);
		Out.Y /= static_cast<float>(Out.N);
	}
	return Out;
}

/** Whether Key is cached at scale 1 (a fresh canvas asks the thread's cache). */
bool CachedLook(uint64_t Key)
{
	ui_test::TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1600.0f, 962.0f, 1.0f);
	return C.IsCached(Key);
}

/**
 * FestCrestLowDetail's FL-7 half (QF-10, accepted; §2.7.1, §2.7.2's word-mark and motion columns, §2.7.7; PLAN_FLOOK FL-7):
 * - The stills: with Still, every word-mark and banner draws its settled frame (the draw list at its motion's end) at any clock, on Main
 *   day and off; every crest motion (Ring Rush's ring, PENNANT's dome, REIGN's key, WRIT's seal) rests before it starts and once it has
 *   run (a crest's still is a settled Time), and only The Championship Online's trophy moves (the Clasp).
 * - The motions move and settle: WRIT's letters flap in one by one; FEIGN's bob on a slow sine within 8% of the size; PUSH's gauge fills on
 *   open to the day's progress (its needle at the day's share of the bar), moving once a world minute, each minute's settled mark one
 *   cached fragment; GRIT's curtains part; RCOP's waves rise and climb through the day, cached per world minute; Summer Slam's rays turn
 *   for good; REIGN's door-light comes up on Main day only; the crests' motions run once and come back to the crest at rest (and a
 *   festival event's emblem never moves); PENNANT's dome chase lights its bulbs one by one; the Clasp glints over the cached bracelet and
 *   never on the silhouette.
 * - The word-marks (§2.7.2): each look's face (BlackItalic, BoldCondensed, or the site's bold title face), the festival's name spelled in
 *   capitals where the look sets caps and as written for RiverLine's four, RunItTwice's three twice-printed (a Signal Red under-copy at
 *   the site's offset), WRIT and REIGN tracked wide (0.28 of the size), FEIGN's funhouse letters, PENNANT's stitched underline and PUSH's
 *   gauge, no underline on the other eight, at most 1k vertices and 40 runs at 46 (printed at both scales: F-hubs' hub budget).
 * - The banner: inside its rect (Summer Slam's rays are clipped to it), at most 6k vertices settled at both scales (printed), its ground
 *   one cached fragment, the ten looks' banners distinct, no text.
 * Planted bug: Curtains animating while Still (FestBanner reads SinceOpen for GRIT's curtains).
 */
void FestMotionsLook()
{
	namespace fd = festart::festart_detail;
	namespace ead = ss::ui::eventart::eventart_detail;
	const double Clocks[] = {-1.0, 0.0, 0.1, 0.3, 0.55, 0.9, 1.25, 2.0, 7.3};
	const float Day = 0.4f; // 9:36 AM (world minute 576)

	// The stills.
	for (const Brand B : AllLook)
	{
		const festart::FestivalLook& L = festart::LookOf(B);
		const std::string Who = std::string(" (") + NameLook(B) + ")";
		const double WordEnd = fd::FestWordSettle(L, MarkNameLook(B));
		const double BannerEnd = fd::FestBannerSettle(L);
		const uint64_t Word = MarkLook(B, WordEnd, Day, false).Hash;
		const uint64_t Banner[2] = {BannerLook(B, BannerEnd, Day, false, false).Hash, BannerLook(B, BannerEnd, Day, true, false).Hash};
		bool WordStill = true;
		bool BannerStill = true;
		for (const double T : Clocks)
		{
			WordStill = WordStill && MarkLook(B, T, Day, true).Hash == Word;
			for (int Main = 0; Main < 2; ++Main)
			{
				BannerStill = BannerStill && BannerLook(B, T, Day, Main == 1, true).Hash == Banner[Main];
			}
		}
		ExpectLook(WordStill, "FestCrestLowDetail (stills): with Still the word-mark draws its settled frame (the one at its motion's end) at any clock" + Who);
		ExpectLook(BannerStill, "FestCrestLowDetail (stills): with Still the banner draws its settled frame (the one at its motions' end) at any clock, on Main day and off" + Who);
		const double CrestEnd = ead::FestCrestSeconds(B);
		const bool Rests = ead::FestCrestPhaseOf(B, -1.0) == 0.0f && ead::FestCrestPhaseOf(B, 0.0) == 0.0f && ead::FestCrestPhaseOf(B, CrestEnd) == 0.0f &&
			ead::FestCrestPhaseOf(B, CrestEnd + 5.0) == 0.0f;
		const bool Moves = CrestEnd > 0.0 && ead::FestCrestPhaseOf(B, CrestEnd * 0.5) > 0.0f;
		const bool Owns = B == Brand::RingRush || B == Brand::Pennant || B == Brand::Reign || B == Brand::Writ;
		ExpectLook(Rests && Moves == Owns,
			"FestCrestLowDetail (stills): the crest's motion rests before it starts and once it has run, and runs between (Ring Rush's ring, PENNANT's dome, REIGN's key, "
			"WRIT's seal; no other crest moves)" + Who);
		const double ClaspEnd = ead::FestClaspSeconds(B, 0);
		ExpectLook((ClaspEnd > 0.0) == (B == Brand::Tco) && ead::FestClaspSeconds(B, 1) == 0.0, "FestCrestLowDetail (stills): only The Championship Online's trophy moves (the Clasp)" + Who);
		std::printf("  stills: %-11s word-mark settles at %.2f s, banner at %.2f s, crest motion runs %.2f s, trophy %.2f s\n", NameLook(B), WordEnd, BannerEnd, CrestEnd, ClaspEnd);
	}

	// WRIT's flap: tiles first, the letters flapping in one by one, then the mark holds.
	{
		const festart::FestivalLook& L = festart::LookOf(Brand::Writ);
		const double End = fd::FestWordSettle(L, MarkNameLook(Brand::Writ));
		const MarkRunLook Before = MarkLook(Brand::Writ, -0.5, Day, false);
		const MarkRunLook Early = MarkLook(Brand::Writ, 0.3, Day, false);
		const MarkRunLook Settled = MarkLook(Brand::Writ, End, Day, false);
		const size_t Shown = ReadableLook(Early, L).size();
		const size_t All = ReadableLook(Settled, L).size();
		std::printf("  motions: WRIT's flap: %zu of %zu letters at 0.3 s (%zu vertices of tiles), all at %.2f s\n", Shown, All, Early.Verts.size(), End);
		ExpectLook(ReadableLook(Before, L).empty() && !Before.Verts.empty() && Shown > 0 && Shown < All && !Early.Verts.empty() && Settled.Verts.empty() &&
				MarkLook(Brand::Writ, End + 3.0, Day, false).Hash == Settled.Hash,
			"FestCrestLowDetail (motions): WRIT's word-mark flaps in letter by letter (split-flap tiles first), then holds");
	}
	// FEIGN's bob: the letters ride a slow sine, within 8% of the size of the baseline.
	{
		const festart::FestivalLook& L = festart::LookOf(Brand::Feign);
		const MarkRunLook Still = MarkLook(Brand::Feign, 1.2, Day, false);
		float Lo = 1e9f;
		float Hi = -1e9f;
		float Far = 0.0f;
		for (const ss::ui::TextItem& T : ReadableLook(Still, L))
		{
			Lo = std::min(Lo, T.BaselineY);
			Hi = std::max(Hi, T.BaselineY);
			Far = std::max(Far, std::fabs(T.BaselineY - 196.0f));
		}
		std::printf("  motions: FEIGN's bob: baselines %.2f to %.2f at 1.2 s (at most %.2f off)\n", static_cast<double>(Lo), static_cast<double>(Hi), static_cast<double>(Far));
		ExpectLook(MovedLook(MarkLook(Brand::Feign, 0.4, Day, false), Still) && Hi - Lo > 1.0f && Far <= 0.08f * 46.0f + 0.01f,
			"FestCrestLowDetail (motions): FEIGN's letters bob on a slow sine, within 8% of the size");
	}
	// PUSH's gauge: it fills on open to the day's progress, which moves once a world minute, each minute's mark one cached fragment.
	{
		const festart::FestivalLook& L = festart::LookOf(Brand::Push);
		const std::string Name = MarkNameLook(Brand::Push);
		const double End = fd::FestWordSettle(L, Name);
		const MarkRunLook Full = MarkLook(Brand::Push, End, Day, false);
		ExpectLook(MovedLook(MarkLook(Brand::Push, 0.3, Day, false), MarkLook(Brand::Push, End - 1e-6, Day, false)) && MarkLook(Brand::Push, End + 4.0, Day, false).Hash == Full.Hash,
			"FestCrestLowDetail (motions): PUSH's gauge fills on hub open, then holds");
		ExpectLook(MarkLook(Brand::Push, End, Day + 0.2f / 1440.0f, false).Hash == Full.Hash && MarkLook(Brand::Push, End, Day + 1.0f / 1440.0f, false).Hash != Full.Hash &&
				CachedLook(fd::FestWordMarkKey(L, Name, 46.0f, 576)) && !CachedLook(fd::FestWordMarkKey(L, Name, 46.0f, 1439)),
			"FestCrestLowDetail (motions): PUSH's gauge moves once a world minute, each minute's settled mark one cached fragment");
		ui_test::TableMeasurer M;
		const float Bar0 = 210.0f + 0.04f * 46.0f;
		const float BarW = M.Width(Name, ss::ui::Font::BlackItalic, 46.0f) - 0.1f * 46.0f;
		const float At25 = InkLook(MarkLook(Brand::Push, End, 0.25f, false), L.Accent2, 0.99f).X;
		const float At75 = InkLook(MarkLook(Brand::Push, End, 0.75f, false), L.Accent2, 0.99f).X;
		std::printf("  motions: PUSH's gauge needle at %.1f (day 0.25) and %.1f (day 0.75) on a bar %.1f to %.1f\n", static_cast<double>(At25), static_cast<double>(At75),
			static_cast<double>(Bar0), static_cast<double>(Bar0 + BarW));
		ExpectLook(std::fabs(At25 - (Bar0 + 0.25f * BarW)) <= 0.6f && std::fabs(At75 - (Bar0 + 0.75f * BarW)) <= 0.6f,
			"FestCrestLowDetail (motions): PUSH's gauge fills to the day's progress (its needle at the day's share of the bar)");
		const MarkRunLook Old = RunMarkLook([&](ss::ui::Canvas& C) { festart::FestWordMark(C, L, Name, 210.0f, 196.0f, 46.0f, End, false); });
		ExpectLook(Old.Hash == MarkLook(Brand::Push, End, 1.0f, false).Hash, "FestCrestLowDetail (motions): the word-mark without a day draws PUSH's gauge full");
	}
	// The banner's motions.
	{
		ExpectLook(MovedLook(BannerLook(Brand::Grit, 0.3, Day, false, false), BannerLook(Brand::Grit, 1.4 - 1e-6, Day, false, false)) &&
				BannerLook(Brand::Grit, 1.4, Day, false, false).Hash == BannerLook(Brand::Grit, 6.0, Day, false, false).Hash,
			"FestCrestLowDetail (motions): GRIT's curtains part on hub open, then hold");
		const festart::FestivalLook& Rcop = festart::LookOf(Brand::Rcop);
		const MarkRunLook Risen = BannerLook(Brand::Rcop, 1.6, Day, false, false);
		const float Dawn = InkLook(BannerLook(Brand::Rcop, 1.6, 0.1f, false, false), Rcop.Accent2, 0.4f).Y;
		const float Dusk = InkLook(BannerLook(Brand::Rcop, 1.6, 0.9f, false, false), Rcop.Accent2, 0.4f).Y;
		std::printf("  motions: RCOP's top wave at y %.1f (day 0.1) and %.1f (day 0.9)\n", static_cast<double>(Dawn), static_cast<double>(Dusk));
		ExpectLook(MovedLook(BannerLook(Brand::Rcop, 0.5, Day, false, false), BannerLook(Brand::Rcop, 1.6 - 1e-6, Day, false, false)) && BannerLook(Brand::Rcop, 9.0, Day, false, false).Hash == Risen.Hash && Dawn > Dusk + 20.0f,
			"FestCrestLowDetail (motions): RCOP's waves rise on hub open to the day's mark, which climbs through the day");
		ExpectLook(BannerLook(Brand::Rcop, 1.6, Day + 0.2f / 1440.0f, false, false).Hash == Risen.Hash && BannerLook(Brand::Rcop, 1.6, Day + 1.0f / 1440.0f, false, false).Hash != Risen.Hash &&
				CachedLook(fd::FestBannerKey(Rcop, BannerRectLook(Brand::Rcop), 1, 576)),
			"FestCrestLowDetail (motions): RCOP's wave moves once a world minute, each minute's settled wave one cached fragment");
		ExpectLook(MovedLook(BannerLook(Brand::Slam, 0.0, Day, false, false), BannerLook(Brand::Slam, 5.0, Day, false, false)),
			"FestCrestLowDetail (motions): Summer Slam's rays turn for good (live)");
		const uint64_t Lit = BannerLook(Brand::Reign, 0.8, Day, true, false).Hash;
		bool MainOnly = true;
		for (const Brand B : AllLook)
		{
			MainOnly = MainOnly && (B == Brand::Reign) == (BannerLook(B, 5.0, Day, true, false).Hash != BannerLook(B, 5.0, Day, false, false).Hash);
		}
		ExpectLook(MovedLook(BannerLook(Brand::Reign, 0.3, Day, true, false), BannerLook(Brand::Reign, 0.8 - 1e-6, Day, true, false)) && BannerLook(Brand::Reign, 4.0, Day, true, false).Hash == Lit && MainOnly,
			"FestCrestLowDetail (motions): REIGN's door-light comes up on Main day (and only REIGN's banner knows Main day)");
	}
	// The crests' motions: once round and back to the crest at rest (40 units: no glints twinkle on the clock).
	for (const Brand B : {Brand::RingRush, Brand::Reign, Brand::Writ})
	{
		const double End = ead::FestCrestSeconds(B);
		const uint64_t Rest = CrestAtLook(B, 40.0f, 0.0).Hash;
		const MarkRunLook Starting = CrestAtLook(B, 40.0f, End * 1e-6); // live, a hair into the run: the rest pose through the live path
		ExpectLook(MovedLook(CrestAtLook(B, 40.0f, End * 0.5), Starting) && MovedLook(CrestAtLook(B, 40.0f, End * 0.2), Starting) && CrestAtLook(B, 40.0f, End).Hash == Rest &&
				CrestAtLook(B, 40.0f, End + 3.7).Hash == Rest && CrestAtLook(B, 40.0f, -2.0).Hash == Rest,
			std::string("FestCrestLowDetail (motions): the crest's motion runs once and comes back to the crest at rest (") + NameLook(B) + ")");
	}
	{
		// Event emblems never move: a Ring Rush ring event's emblem part-way through the ring's turn is the emblem at rest.
		ss::net::EventTemplate T;
		T.Id = "ring27-fl7";
		T.Name = "$215 Ring Event";
		T.Series = "ring27";
		T.BuyInCents = 21500;
		T.Ring = true;
		const auto EmblemAt = [&T](double Time) { return RunMarkLook([&](ss::ui::Canvas& C) { ss::ui::eventart::Emblem(C, T, 200.0f, 200.0f, 40.0f, Time); }).Hash; };
		ExpectLook(ss::net::Site(SiteId::RiverLine).FindSeries("ring27") != nullptr && EmblemAt(0.6) == EmblemAt(0.0) && EmblemAt(0.3) == EmblemAt(0.0),
			"FestCrestLowDetail (motions): a festival event's emblem never moves (the series crest alone does)");
	}
	{
		// PENNANT's dome chase (bulbs are drawn above 44 units): a halo runs round the dome bulb by bulb while it runs, none at rest.
		const festart::FestivalLook& L = festart::LookOf(Brand::Pennant);
		const uint32_t Halo = RgbLook(ss::ui::Mix(ss::ui::Hex(L.Gold), ss::ui::Hex(0xffffff), 0.85f));
		const double End = ead::FestCrestSeconds(Brand::Pennant);
		const InkAtLook Early = InkLook(CrestAtLook(Brand::Pennant, 150.0f, End * 0.3), Halo, 0.05f);
		const InkAtLook Late = InkLook(CrestAtLook(Brand::Pennant, 150.0f, End * 0.7), Halo, 0.05f);
		std::printf("  motions: PENNANT's dome chase: %d halo vertices at x %.1f (0.3 of the run), %d at x %.1f (0.7)\n", Early.N, static_cast<double>(Early.X), Late.N,
			static_cast<double>(Late.X));
		ExpectLook(Early.N > 0 && Late.N > 0 && std::fabs(Early.X - Late.X) > 10.0f && InkLook(CrestAtLook(Brand::Pennant, 150.0f, 0.0), Halo, 0.05f).N == 0 &&
				InkLook(CrestAtLook(Brand::Pennant, 150.0f, End), Halo, 0.05f).N == 0,
			"FestCrestLowDetail (motions): PENNANT's dome bulbs chase once, bulb by bulb, and rest unlit by the chase");
		// The Clasp: a glint over the cached bracelet while it runs; at rest, and on the silhouette, the cached fragment alone.
		const double Clasp = ead::FestClaspSeconds(Brand::Tco, 0);
		const MarkRunLook RestRun = TrophyAtLook(Brand::Tco, 54.0f, 0.0, false);
		const uint64_t Rest = RestRun.Hash;
		ExpectLook(MovedLook(TrophyAtLook(Brand::Tco, 54.0f, Clasp * 0.5, false), RestRun) && TrophyAtLook(Brand::Tco, 54.0f, Clasp, false).Hash == Rest &&
				TrophyAtLook(Brand::Tco, 54.0f, Clasp + 2.0, false).Hash == Rest && TrophyAtLook(Brand::Tco, 30.0f, Clasp * 0.5, true).Hash == TrophyAtLook(Brand::Tco, 30.0f, 0.0, true).Hash &&
				TrophyAtLook(Brand::Rcop, 54.0f, Clasp * 0.5, false).Hash == TrophyAtLook(Brand::Rcop, 54.0f, 0.0, false).Hash,
			"FestCrestLowDetail (motions): The Championship Online's Clasp glints once over the cached bracelet (never on the silhouette, on no other trophy)");
	}

	// The word-marks.
	ui_test::TableMeasurer M;
	for (const Brand B : AllLook)
	{
		const festart::FestivalLook& L = festart::LookOf(B);
		const std::string Who = std::string(" (") + NameLook(B) + ")";
		const std::string Name = MarkNameLook(B);
		std::string Shown = Name;
		if (L.Caps)
		{
			std::transform(Shown.begin(), Shown.end(), Shown.begin(), [](char Ch) { return Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch; });
		}
		const double End = fd::FestWordSettle(L, Name);
		const MarkRunLook At1 = MarkLook(B, End, Day, false);
		const MarkRunLook At15 = MarkLook(B, End, Day, false, 1.5f);
		std::printf("  word-mark: %-11s \"%s\" at 46: %zu, %zu vertices and %zu, %zu runs (scales 1, 1.5)\n", NameLook(B), Shown.c_str(), At1.Verts.size(), At15.Verts.size(),
			At1.Runs.size(), At15.Runs.size());
		ExpectLook(At1.Verts.size() <= 1000 && At15.Verts.size() <= 1000 && At1.Runs.size() <= 40 && At15.Runs.size() <= 40,
			"FestCrestLowDetail (word-mark): at most 1k vertices and 40 runs at the banner's 46, both scales" + Who);
		const ss::ui::Font Face = L.WordFace >= 0 ? static_cast<ss::ui::Font>(L.WordFace) : ss::ui::Font::Bold;
		bool Faced = !At1.Runs.empty();
		for (const ss::ui::TextItem& T : At1.Runs)
		{
			Faced = Faced && T.Face == Face;
		}
		ExpectLook(Faced, "FestCrestLowDetail (word-mark): set in the look's face (BlackItalic, BoldCondensed, or the site's bold title face)" + Who);
		const std::vector<ss::ui::TextItem> Read = ReadableLook(At1, L);
		std::string Spelled;
		for (const ss::ui::TextItem& T : Read)
		{
			Spelled += T.Text;
		}
		Spelled.erase(std::remove(Spelled.begin(), Spelled.end(), ' '), Spelled.end());
		std::string Want = Shown;
		Want.erase(std::remove(Want.begin(), Want.end(), ' '), Want.end());
		// The same name typed in lower case: capitals where the look sets caps, as typed where it doesn't.
		std::string Lower = Name;
		std::transform(Lower.begin(), Lower.end(), Lower.begin(), [](char Ch) { return Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch; });
		const MarkRunLook Typed = RunMarkLook([&](ss::ui::Canvas& C) { festart::FestWordMark(C, L, Lower, 210.0f, 196.0f, 46.0f, End, Day, false); });
		std::string TypedSpelled;
		for (const ss::ui::TextItem& T : ReadableLook(Typed, L))
		{
			TypedSpelled += T.Text;
		}
		TypedSpelled.erase(std::remove(TypedSpelled.begin(), TypedSpelled.end(), ' '), TypedSpelled.end());
		std::string TypedWant = Lower;
		if (L.Caps)
		{
			std::transform(TypedWant.begin(), TypedWant.end(), TypedWant.begin(), [](char Ch) { return Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch; });
		}
		TypedWant.erase(std::remove(TypedWant.begin(), TypedWant.end(), ' '), TypedWant.end());
		ExpectLook(Spelled == Want && TypedSpelled == TypedWant && (L.Caps == (HostLook(B) != SiteId::RiverLine)),
			"FestCrestLowDetail (word-mark): spells the festival's name, in capitals where the look sets caps (the rivals' six), as typed for RiverLine's four" + Who);
		// The twice print: a Signal Red under-copy of every letter at the site's offset (round(46 / 16), 2-6: 3).
		const float O = std::min(6.0f, std::max(2.0f, std::floor(46.0f / 16.0f + 0.5f)));
		size_t Unders = 0;
		for (const ss::ui::TextItem& T : Read)
		{
			for (const ss::ui::TextItem& U : At1.Runs)
			{
				if (U.Text == T.Text && RgbLook(U.Col) == 0xd2141a && std::fabs(U.X - T.X - O) < 0.01f && std::fabs(U.BaselineY - T.BaselineY - O) < 0.01f)
				{
					++Unders;
					break;
				}
			}
		}
		ExpectLook(L.Twice ? (!Read.empty() && Unders == Read.size()) : Unders == 0,
			"FestCrestLowDetail (word-mark): RunItTwice's three are twice-printed (a Signal Red under-copy at the site's offset under every letter), the other seven print once" + Who);
		// Letter by letter where it's tracked (WRIT, REIGN: 0.28 of the size apart), bobs (FEIGN) or flaps (WRIT); one run otherwise.
		bool Set = true;
		if (L.Tracking > 0.0f || L.Move == festart::Motion::Bob || L.Move == festart::Motion::Flap)
		{
			float X = 210.0f;
			size_t K = 0;
			const ss::ui::Font F = Face;
			for (size_t I = 0; I < Shown.size(); ++I)
			{
				const std::string G = Shown.substr(I, 1);
				if (G != " ")
				{
					Set = Set && K < Read.size() && Read[K].Text == G && std::fabs(Read[K].X - X) < 0.02f;
					++K;
				}
				X += M.Width(G, F, 46.0f) + L.Tracking * 46.0f;
			}
			Set = Set && K == Read.size();
		}
		else
		{
			Set = Read.size() == 1;
		}
		ExpectLook(Set && ((B == Brand::Writ || B == Brand::Reign) == (L.Tracking >= 0.28f * 0.999f)),
			"FestCrestLowDetail (word-mark): WRIT and REIGN tracked wide (letter by letter, 0.28 of the size apart), a plain mark one run" + Who);
		if (B == Brand::Feign)
		{
			bool Funhouse = Read.size() >= 2;
			for (size_t K = 0; K < Read.size(); ++K)
			{
				bool Shadow = false;
				for (const ss::ui::TextItem& U : At1.Runs)
				{
					Shadow = Shadow || (U.Text == Read[K].Text && RgbLook(U.Col) == L.Gold && std::fabs(U.X - Read[K].X - 0.05f * 46.0f) < 0.01f);
				}
				Funhouse = Funhouse && Shadow;
			}
			bool Pink = false;
			bool Mint = false;
			for (const ss::ui::TextItem& T : Read)
			{
				Pink = Pink || RgbLook(T.Col) == L.Accent;
				Mint = Mint || RgbLook(T.Col) == L.Accent2;
			}
			ExpectLook(Funhouse && Pink && Mint, "FestCrestLowDetail (word-mark): FEIGN's funhouse letters, pink and mint over a Lemon Drop offset");
		}
		// The underline: PENNANT's stitched (an Outfield Green bar under coral stitches), PUSH's gauge; letters alone for the other eight.
		bool Below = true;
		for (const ss::ui::Vertex& V : At1.Verts)
		{
			Below = Below && V.Y >= 196.0f + 0.13f * 46.0f - 1.5f * 46.0f * 0.085f;
		}
		const bool Under = B == Brand::Pennant ? InkLook(At1, L.Accent, 0.9f).N > 0 && InkLook(At1, L.Accent2, 0.5f).N > 0 && Below
			: B == Brand::Push                 ? InkLook(At1, L.Accent2, 0.99f).N > 0 && Below
											   : At1.Verts.empty();
		ExpectLook(Under, "FestCrestLowDetail (word-mark): PENNANT's stitched underline and PUSH's gauge under the letters, no underline on the other eight" + Who);
	}

	// The banner.
	std::set<uint64_t> Banners;
	for (const Brand B : AllLook)
	{
		const festart::FestivalLook& L = festart::LookOf(B);
		const std::string Who = std::string(" (") + NameLook(B) + ")";
		const ss::ui::Rect R = BannerRectLook(B);
		const double End = fd::FestBannerSettle(L);
		size_t Count[2] = {0, 0};
		bool Inside = true;
		bool Quiet = true;
		for (int K = 0; K < 2; ++K)
		{
			const MarkRunLook Bn = BannerLook(B, End, Day, B == Brand::Reign, false, K == 0 ? 1.0f : 1.5f);
			Count[K] = Bn.Verts.size();
			Quiet = Quiet && Bn.Runs.empty();
			if (B != Brand::Slam)
			{
				for (const ss::ui::Vertex& V : Bn.Verts)
				{
					Inside = Inside && V.X >= R.X - 2.0f && V.X <= R.X + R.W + 2.0f && V.Y >= R.Y - 2.0f && V.Y <= R.Y + R.H + 2.0f;
				}
			}
			if (K == 0)
			{
				Banners.insert(Bn.Hash);
			}
		}
		std::printf("  banner: %-11s {%.0f, %.0f, %.0f, %.0f} settled: %zu, %zu vertices (scales 1, 1.5)\n", NameLook(B), static_cast<double>(R.X), static_cast<double>(R.Y),
			static_cast<double>(R.W), static_cast<double>(R.H), Count[0], Count[1]);
		ExpectLook(Count[0] <= 6000 && Count[1] <= 6000 && Inside && Quiet && CachedLook(fd::FestBannerKey(L, R, 0, 0)),
			"FestCrestLowDetail (banner): the banner's ground inside its rect (Summer Slam's rays clipped to it), at most 6k vertices settled, one cached fragment, no text" + Who);
	}
	ExpectLook(Banners.size() == 10, "FestCrestLowDetail (banner): the ten looks' banners are distinct");
}

/**
 * FestTrophyCached (§2.7.5, §2.8.7, §2.15.2; §8.5's "Crests; trophies" row): for every brand, FestTrophy and FestBelt are one cached
 * fragment each (Canvas::IsCached holds their key after the first drawing and not before; a second drawing copies exactly what the
 * first put down), at most 1.8k vertices at 54 units and 400 with Lod at 30, at both scales (1 and 1.5 pixels a unit); the two title
 * objects (Tier 1: The Pennant Ribbon, The Spade Finial) at most 0.9k, cached, unlike their trophies, and no other brand draws one;
 * a trophy fills its box at every size (its measured height 0.78 of Size, within 0.03, so a fragment cached at one size is never
 * replayed at another) and each championship's trophy (RCOP, PENNANT, REIGN) stands 1.15 times each of its site's other festival
 * trophies (measured heights, within 1.12-1.18) at 30 (Lod), 54 and 96; the RCOP Cup engraves one line per edition (each field
 * changes it, a bigger field's line sits higher, twelve lines within the budget); FestPin for every grade (0, the empty socket, to 3)
 * and mark at most 0.4k at 40 (ten slots inside §2.8.7's 4k), cached, each its own; the ten trophies, the ten belts and each brand's
 * pins pairwise distinct; each look's Trophy is fest::TrophyName's (R-3). Printed: every count, height and ratio. Drawn under a skin
 * no page wears, so its keys are its own.
 * Planted bug: the trophy drawn live (uncached).
 */
void FestTrophyCached()
{
	namespace fa = ss::ui::festart;
	namespace ead = ss::ui::eventart::eventart_detail;
	using ss::world::FestMark;
	const float Scales[2] = {1.0f, 1.5f};
	ss::ui::Skin Probe = ss::ui::Skin::Current();
	Probe.Gold = ss::ui::Hex(0xe6b422); // a gold no page wears: every key below is this check's own (Skin::Key)
	const ss::ui::SkinScope ProbeScope(Probe);
	const std::vector<int> NoFields;
	std::vector<uint64_t> Trophies;
	std::vector<uint64_t> Belts;
	std::vector<uint64_t> GoldPins;
	size_t CacheVertices = 0;
	for (const Brand B : AllLook)
	{
		const std::string Who = std::string(" (") + NameLook(B) + ")";
		ExpectLook(std::strcmp(festart::LookOf(B).Trophy, ss::fest::TrophyName(B)) == 0 && ss::fest::TrophyName(B)[0] != '\0',
			"FestTrophyCached: the look's Trophy is fest::TrophyName's (R-3)" + Who);
		const bool Titled = ss::fest::TrophyName(B, 1)[0] != '\0';
		size_t Counts[4][2] = {};
		for (int K = 0; K < 2; ++K)
		{
			const float S = Scales[K];
			char Scale[24];
			std::snprintf(Scale, sizeof(Scale), ", scale %.1f", static_cast<double>(S));
			const ArtRunLook Trophy = RunArtLook([B](ss::ui::Canvas& C) { fa::FestTrophy(C, B, 200.0f, 200.0f, 54.0f, 0.0, false); },
				ead::FestTrophyKey(B, 54.0f, false, NoFields, 0), S);
			const ArtRunLook TrophyLod = RunArtLook([B](ss::ui::Canvas& C) { fa::FestTrophy(C, B, 200.0f, 200.0f, 30.0f, 0.0, true); },
				ead::FestTrophyKey(B, 30.0f, true, NoFields, 0), S);
			const ArtRunLook Belt = RunArtLook([B](ss::ui::Canvas& C) { fa::FestBelt(C, B, 200.0f, 200.0f, 54.0f, 0.0, false); }, ead::FestBeltKey(B, 54.0f, false), S);
			const ArtRunLook BeltLod = RunArtLook([B](ss::ui::Canvas& C) { fa::FestBelt(C, B, 200.0f, 200.0f, 30.0f, 0.0, true); }, ead::FestBeltKey(B, 30.0f, true), S);
			const ArtRunLook Title = RunArtLook([B](ss::ui::Canvas& C) { fa::FestTrophy(C, B, 200.0f, 200.0f, 54.0f, 0.0, false, {}, 1); },
				ead::FestTrophyKey(B, 54.0f, false, NoFields, 1), S);
			Counts[0][K] = Trophy.Vertices;
			Counts[1][K] = TrophyLod.Vertices;
			Counts[2][K] = Belt.Vertices;
			Counts[3][K] = BeltLod.Vertices;
			CacheVertices += Trophy.Vertices + TrophyLod.Vertices + Belt.Vertices + BeltLod.Vertices + Title.Vertices;
			ExpectLook(Trophy.Fresh && Trophy.Cached && Trophy.Copy && TrophyLod.Fresh && TrophyLod.Cached && TrophyLod.Copy,
				"FestTrophyCached: the trophy is one cached fragment and a second drawing adds only the copy" + Who + Scale);
			ExpectLook(Belt.Fresh && Belt.Cached && Belt.Copy && BeltLod.Fresh && BeltLod.Cached && BeltLod.Copy,
				"FestTrophyCached: the belt is one cached fragment and a second drawing adds only the copy" + Who + Scale);
			ExpectLook(Trophy.Vertices > 0 && Trophy.Vertices <= 1800 && TrophyLod.Vertices > 0 && TrophyLod.Vertices <= 400,
				"FestTrophyCached: a trophy is at most 1.8k vertices at 54 and 400 with Lod at 30" + Who + Scale);
			ExpectLook(Belt.Vertices > 0 && Belt.Vertices <= 1800 && BeltLod.Vertices > 0 && BeltLod.Vertices <= 400,
				"FestTrophyCached: a belt is at most 1.8k vertices at 54 and 400 with Lod at 30" + Who + Scale);
			if (Titled)
			{
				ExpectLook(Title.Fresh && Title.Cached && Title.Copy && Title.Vertices > 0 && Title.Vertices <= 900 && Title.Hash != Trophy.Hash,
					"FestTrophyCached: the title object is its own cached piece of at most 0.9k vertices at 54" + Who + Scale);
				std::printf("  trophy: %-11s title object '%s' at 54: %zu vertices (scale %.1f)\n", NameLook(B), ss::fest::TrophyName(B, 1), Title.Vertices, static_cast<double>(S));
			}
			else
			{
				ExpectLook(Title.Vertices == 0, "FestTrophyCached: a brand without a title object draws none at Tier 1" + Who + Scale);
			}
			if (K == 0)
			{
				Trophies.push_back(Trophy.Hash);
				Belts.push_back(Belt.Hash);
			}
			// The pins: every grade and mark at the pin wall's 40.
			std::set<uint64_t> Pins;
			size_t PinMost = 0;
			bool PinsCached = true;
			for (int Grade = 0; Grade <= 3; ++Grade)
			{
				for (int Mk = static_cast<int>(FestMark::None); Mk < static_cast<int>(FestMark::Count); ++Mk)
				{
					const FestMark Mark = static_cast<FestMark>(Mk);
					const ArtRunLook Pin = RunArtLook([B, Grade, Mark](ss::ui::Canvas& C) { fa::FestPin(C, B, Grade, Mark, 200.0f, 200.0f, 40.0f); },
						ead::FestPinKey(B, Grade, Mark, 40.0f), S);
					PinMost = std::max(PinMost, Pin.Vertices);
					PinsCached = PinsCached && Pin.Fresh && Pin.Cached && Pin.Copy && Pin.Vertices > 0;
					if (Grade > 0)
					{
						Pins.insert(Pin.Hash);
					}
					if (K == 0 && Grade == 3 && Mark == FestMark::None)
					{
						GoldPins.push_back(Pin.Hash);
					}
				}
			}
			ExpectLook(PinsCached && PinMost <= 400, "FestTrophyCached: every pin (each grade and mark) is cached and at most 0.4k vertices at 40" + Who + Scale);
			ExpectLook(Pins.size() == 3u * static_cast<size_t>(FestMark::Count), "FestTrophyCached: each grade and mark draws its own pin" + Who + Scale);
			if (K == 1)
			{
				std::printf("  trophy: %-11s '%s' at 54: %zu, %zu vertices; Lod at 30: %zu, %zu | belt at 54: %zu, %zu; Lod at 30: %zu, %zu | pins at 40: at most %zu\n",
					NameLook(B), festart::LookOf(B).Trophy, Counts[0][0], Counts[0][1], Counts[1][0], Counts[1][1], Counts[2][0], Counts[2][1], Counts[3][0], Counts[3][1],
					PinMost);
			}
		}
	}
	ExpectLook(std::set<uint64_t>(Trophies.begin(), Trophies.end()).size() == 10 && std::set<uint64_t>(Belts.begin(), Belts.end()).size() == 10 &&
			std::set<uint64_t>(GoldPins.begin(), GoldPins.end()).size() == 10,
		"FestTrophyCached: the ten trophies, the ten belts and the ten gold pins are pairwise distinct");
	std::printf("  trophy cache: 10 trophies, 10 belts at 54 and 30 and 2 title objects, 2 scales: %zu vertices (Shared pool: 1.5M)\n", CacheVertices);

	// A trophy fills its box at every size (0.78 of it), and a championship's outranks its site's other festival trophies: 1.15 times
	// their height at every size (§2.6.4, §2.7.5).
	struct SiteTrophies
	{
		Brand Championship;
		Brand Others[3];
	};
	const SiteTrophies Sites[3] = {{Brand::Rcop, {Brand::RingRush, Brand::Tco, Brand::Slam}}, {Brand::Pennant, {Brand::Feign, Brand::Push, Brand::None}},
		{Brand::Reign, {Brand::Writ, Brand::Grit, Brand::None}}};
	for (const SiteTrophies& Site : Sites)
	{
		for (const float Size : {30.0f, 54.0f, 96.0f})
		{
			const bool Lod = Size <= 30.0f;
			for (int K = 0; K < 2; ++K)
			{
				const auto HeightOf = [&](Brand B) {
					const float H = RunArtLook([B, Size, Lod](ss::ui::Canvas& C) { fa::FestTrophy(C, B, 200.0f, 200.0f, Size, 0.0, Lod); },
						ead::FestTrophyKey(B, Size, Lod, NoFields, 0), Scales[K])
										.Height;
					const float Fill = 0.78f * (ss::fest::IsChampionship(B) ? 1.15f : 1.0f);
					char At[64];
					std::snprintf(At, sizeof(At), " (%s at %.0f%s, scale %.1f: %.3f)", NameLook(B), static_cast<double>(Size), Lod ? " Lod" : "",
						static_cast<double>(Scales[K]), static_cast<double>(H / Size));
					ExpectLook(std::fabs(H / Size - Fill) <= 0.03f, std::string("FestTrophyCached: a trophy fills its box at every size (0.78 of it, a championship's 0.90)") + At);
					return H;
				};
				const float Top = HeightOf(Site.Championship);
				std::string Line;
				bool Outranks = true;
				for (const Brand O : Site.Others)
				{
					if (O == Brand::None)
					{
						continue;
					}
					const float Ratio = Top / std::max(0.01f, HeightOf(O));
					Outranks = Outranks && Ratio >= 1.12f && Ratio <= 1.18f;
					char Part[48];
					std::snprintf(Part, sizeof(Part), " %s %.3f", NameLook(O), static_cast<double>(Ratio));
					Line += Part;
				}
				char Where[64];
				std::snprintf(Where, sizeof(Where), " (%s at %.0f%s, scale %.1f)", NameLook(Site.Championship), static_cast<double>(Size), Lod ? " Lod" : "",
					static_cast<double>(Scales[K]));
				std::printf("  trophy: %s height %.2f; over its site's others:%s\n", Where, static_cast<double>(Top), Line.c_str());
				ExpectLook(Outranks, std::string("FestTrophyCached: a championship's trophy stands 1.15 times its site's other festival trophies") + Where);
			}
		}
	}

	// The RCOP Cup engraves one line per edition at the height of its Main's field (UI/EventArt.cpp's FestCupEngraving, d8def0).
	const uint32_t Engraving = 0xd8def0;
	const auto Cup = [Engraving](const std::vector<int>& Fields, float Size) {
		return RunArtLook([&Fields, Size](ss::ui::Canvas& C) { fa::FestTrophy(C, Brand::Rcop, 200.0f, 200.0f, Size, 0.0, false, Fields); },
			ss::ui::eventart::eventart_detail::FestTrophyKey(Brand::Rcop, Size, false, Fields, 0), 1.0f, Engraving);
	};
	const ArtRunLook Bare = Cup({}, 96.0f);
	const ArtRunLook Small = Cup({6052}, 96.0f);
	const ArtRunLook Large = Cup({9000}, 96.0f);
	const ArtRunLook Both = Cup({9000, 6052}, 96.0f);
	const std::vector<int> Twelve{6052, 6400, 6800, 7100, 7500, 7900, 8200, 8600, 9000, 9400, 9800, 10400, 10900, 11500};
	const ArtRunLook Many = Cup(Twelve, 54.0f);
	const ArtRunLook Many15 = RunArtLook([&Twelve](ss::ui::Canvas& C) { fa::FestTrophy(C, Brand::Rcop, 200.0f, 200.0f, 54.0f, 0.0, false, Twelve); },
		ead::FestTrophyKey(Brand::Rcop, 54.0f, false, Twelve, 0), 1.5f);
	std::printf("  trophy: the RCOP Cup at 96: no field %zu vertices, 6,052 %zu (line top %.1f), 9,000 %zu (line top %.1f), both %zu; 14 fields (12 drawn) at 54: %zu, %zu\n",
		Bare.Vertices, Small.Vertices, static_cast<double>(Small.MarkTop), Large.Vertices, static_cast<double>(Large.MarkTop), Both.Vertices, Many.Vertices, Many15.Vertices);
	ExpectLook(Bare.MarkTop > 1e8f && Small.MarkTop < 1e8f && Large.MarkTop < Small.MarkTop && Both.Vertices > Small.Vertices &&
			std::set<uint64_t>{Bare.Hash, Small.Hash, Large.Hash, Both.Hash}.size() == 4 && Small.Cached && Both.Cached,
		"FestTrophyCached: the RCOP Cup engraves a line per edition, a bigger Main field's higher");
	ExpectLook(Many.Vertices <= 1800 && Many15.Vertices <= 1800 && Many.Cached, "FestTrophyCached: the RCOP Cup with its lines is at most 1.8k vertices at 54");
}

// ------------------------------------------------------------------ Ignite's band: the attractions (G1 FL-5)

namespace ia = ss::ui::igniteart;

/** A band as F-hubs fills it (FH-3b): the hour's tones, the clocks, the weather and the festival's three fields. */
ia::IgniteBand BandLook(Brand Fest, int Day, int Days, ss::site::show::Mood Mood, double Now, bool Busy)
{
	ia::IgniteBand B;
	B.K = ia::IgniteTonesFor(Mood);
	B.Now = Now;
	B.World = 60.0 * 24.0 * 30.0 + 20.0 * 60.0;
	B.Snow = Busy;
	B.Fireworks = Busy;
	B.Wildfire = Busy;
	B.Fest = Fest;
	B.FestDay = Day;
	B.FestDays = Days;
	return B;
}

/** The band's static layer, its live layer or both, drawn straight onto a fresh canvas at Scale (as F-hubs' IgniteBoardwalk draws
 *  them: the static layer through its cache, the live one directly; the vertices are the same either way). */
ss::ui::DrawList DrawBandLook(const ia::IgniteBand& B, float Scale, bool Static, bool Live)
{
	ui_test::TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, Scale);
	if (Static)
	{
		ia::IgniteBoardwalkStatic(C, B);
	}
	if (Live)
	{
		ia::IgniteBoardwalkLive(C, B);
	}
	return L;
}

/** How many vertices wear Rgb (at alpha MinA or more) inside R. */
size_t WearsLook(const ss::ui::DrawList& L, uint32_t Rgb, const ss::ui::Rect& R, float MinA = 0.5f)
{
	size_t N = 0;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		N += V.Col.A >= MinA && RgbLook(V.Col) == Rgb && V.X >= R.X && V.X <= R.X + R.W && V.Y >= R.Y && V.Y <= R.Y + R.H ? 1u : 0u;
	}
	return N;
}

/** The text runs of a draw list. */
std::vector<ss::ui::TextItem> RunsLook(const ss::ui::DrawList& L)
{
	std::vector<ss::ui::TextItem> Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Out.push_back(Cmd.Text);
		}
	}
	return Out;
}

/** FestArtInventory's entries without their kind. */
std::vector<std::string> ArtTextsLook()
{
	std::vector<std::string> Out;
	for (const std::string& E : ss::fest::FestArtInventory())
	{
		const size_t Tab = E.find('\t');
		Out.push_back(Tab == std::string::npos ? E : E.substr(Tab + 1));
	}
	return Out;
}

// The rects the checks read (UI/IgniteArt.cpp's geometry): the Prize Counter booth (its awning, back wall and counter), FEIGN's arch,
// PUSH's tower with its foot, PENNANT's pavilion and the bandstand's shell it replaces, the puck's track.
constexpr ss::ui::Rect BoothLook{1240.0f, 104.0f, 332.0f, 196.0f};
constexpr ss::ui::Rect ArchLook{1258.0f, 98.0f, 312.0f, 204.0f};
constexpr ss::ui::Rect TowerLook{908.0f, 74.0f, 52.0f, 228.0f};
constexpr ss::ui::Rect PavilionLook{166.0f, 70.0f, 480.0f, 232.0f};
constexpr ss::ui::Rect PuckTrackLook{928.0f, 100.0f, 12.0f, 170.0f};
constexpr uint32_t BoothAwningLook = 0x6b3fd1; // the Prize Counter's violet awning stripe (its body alone wears it on the band)
constexpr uint32_t BoothWallLook[2] = {0x24123a, 0x1c1036}; // the booth's back wall {1252, 132, 304, 166}: Prime Time's, Afterburner's
const Brand AttractionsLook[] = {Brand::Feign, Brand::Push, Brand::Pennant};

/**
 * FestBandBudget (§2.7.4's Ignite band row, §2.8.10 / §8.5's "Ignite's band with an attraction", §2.15.2): Ignite's band, static and
 * live layers, with each attraction (FEIGN's arch, PUSH's tower, PENNANT's pavilion) is at most 15k vertices at both scales, in the
 * day's and Afterburner's tones, at its busiest (snow, the Sunday Blaze's fireworks and the series' bunting at once) on four beats; the
 * attraction's bulbs are one cached row a chase step (Canvas::IsCached holds IgniteAttrBulbsKey after a drawing and not before, a
 * second drawing copies the first exactly, two steps two keys, no attraction no key). Printed: every count, what the attraction adds
 * to the band and the Boardwalk pool's share of the chase rows. PLAY's half (34k vertices and 400 runs with each attraction) is
 * FestBandPlayLook (FL-final), run after the takeover pages.
 * Planted bug: the arch's bulbs uncached.
 */
void FestBandBudget()
{
	const float Scales[2] = {1.0f, 1.5f};
	const ss::site::show::Mood Moods[2] = {ss::site::show::Mood::PrimeTime, ss::site::show::Mood::Afterburner};
	const double Beats[4] = {41.0, 41.26, 41.5, 41.77};
	size_t Plain[2][2] = {};
	for (int Mi = 0; Mi < 2; ++Mi)
	{
		for (int K = 0; K < 2; ++K)
		{
			for (const double Now : Beats)
			{
				Plain[Mi][K] = std::max(Plain[Mi][K], DrawBandLook(BandLook(Brand::None, 0, 0, Moods[Mi], Now, true), Scales[K], true, true).Vertices.size());
			}
		}
	}
	std::printf("  band: Ignite's band today (busiest of four beats): %zu, %zu vertices (Prime Time, scale 1, 1.5); %zu, %zu (Afterburner)\n", Plain[0][0],
		Plain[0][1], Plain[1][0], Plain[1][1]);
	for (const Brand B : AttractionsLook)
	{
		const int Days = B == Brand::Pennant ? 13 : 10;
		for (int Mi = 0; Mi < 2; ++Mi)
		{
			size_t Most[2] = {};
			size_t Static[2] = {};
			for (int K = 0; K < 2; ++K)
			{
				for (const double Now : Beats)
				{
					const ia::IgniteBand Band = BandLook(B, 3, Days, Moods[Mi], Now, true);
					Most[K] = std::max(Most[K], DrawBandLook(Band, Scales[K], true, true).Vertices.size());
					Static[K] = DrawBandLook(Band, Scales[K], true, false).Vertices.size();
				}
			}
			std::printf("  band: %-7s %-11s static %zu, %zu; with the live layer %zu, %zu vertices (scale 1, 1.5); the attraction adds %+d, %+d\n", NameLook(B),
				Mi == 0 ? "Prime Time" : "Afterburner", Static[0], Static[1], Most[0], Most[1], static_cast<int>(Most[0]) - static_cast<int>(Plain[Mi][0]),
				static_cast<int>(Most[1]) - static_cast<int>(Plain[Mi][1]));
			ExpectLook(Most[0] > 0 && Most[0] <= 15000 && Most[1] <= 15000,
				std::string("FestBandBudget: Ignite's band with the attraction is at most 15k vertices at both scales (") + NameLook(B) + (Mi == 0 ? ", Prime Time)" : ", Afterburner)"));
		}
		// The bulbs: one cached row a chase step, replayed exactly; two steps, two rows.
		for (int K = 0; K < 2; ++K)
		{
			const ia::IgniteBand Band = BandLook(B, 3, Days, ss::site::show::Mood::PrimeTime, 977.0 + 0.37 * static_cast<double>(static_cast<int>(B)), false);
			const uint64_t Key = ia::IgniteAttrBulbsKey(Band);
			ss::ui::Canvas::SetPoolBudget(ss::ui::Canvas::CachePool::Boardwalk, 1); // empties the pool (least recently used first)
			ss::ui::Canvas::SetPoolBudget(ss::ui::Canvas::CachePool::Boardwalk, 0); // and puts its 120k back
			ui_test::TableMeasurer M;
			ss::ui::DrawList First;
			ss::ui::DrawList Second;
			ss::ui::Canvas C1(First, M, 1600.0f, 1000.0f, Scales[K]);
			const bool Fresh = Key != 0 && !C1.IsCached(Key);
			ia::IgniteBoardwalkLive(C1, Band);
			const bool Cached = C1.IsCached(Key);
			ss::ui::Canvas C2(Second, M, 1600.0f, 1000.0f, Scales[K]);
			ia::IgniteBoardwalkLive(C2, Band);
			const bool Copy = Second.Vertices.size() == First.Vertices.size() && ListPrintLook(Second) == ListPrintLook(First);
			const ia::IgniteBand Next = BandLook(B, 3, Days, ss::site::show::Mood::PrimeTime, Band.Now + 60.0 / Band.K.Bpm, false);
			char Scale[48];
			std::snprintf(Scale, sizeof(Scale), " (%s, scale %.1f)", NameLook(B), static_cast<double>(Scales[K]));
			ExpectLook(Fresh && Cached && Copy && ia::IgniteAttrBulbsKey(Next) != Key,
				std::string("FestBandBudget: the attraction's bulbs are one cached row a chase step, replayed exactly") + Scale);
		}
		// The chase's rows in the Boardwalk pool (printed; the pool is 120k, least recently used): one per lit pattern in a cycle, and
		// what the live layer gains over today's (the attraction's row, PUSH's puck; under the pavilion the dome's row replaces the
		// bandstand's).
		std::set<uint64_t> Rows;
		for (int Step = 0; Step < 32; ++Step)
		{
			Rows.insert(ia::IgniteAttrBulbsKey(BandLook(B, 3, Days, ss::site::show::Mood::PrimeTime, (static_cast<double>(Step) + 0.5) * 60.0 / 124.0, false)));
		}
		const ia::IgniteBand Lit = BandLook(B, 3, Days, ss::site::show::Mood::PrimeTime, 0.6 * 60.0 / 124.0, false);
		const int Gain = static_cast<int>(DrawBandLook(Lit, 1.0f, false, true).Vertices.size()) -
			static_cast<int>(DrawBandLook(BandLook(Brand::None, 0, 0, ss::site::show::Mood::PrimeTime, Lit.Now, false), 1.0f, false, true).Vertices.size());
		std::printf("  band: %-7s bulbs: %zu chase rows a cycle, each one cached row; the live layer gains %+d vertices over today's\n", NameLook(B), Rows.size(), Gain);
	}
	ExpectLook(ia::IgniteAttrBulbsKey(BandLook(Brand::None, 0, 0, ss::site::show::Mood::PrimeTime, 41.0, false)) == 0 &&
			ia::IgniteAttrBulbsKey(BandLook(Brand::Feign, 0, 10, ss::site::show::Mood::PrimeTime, 41.0, false)) == 0,
		"FestBandBudget: no attraction, no bulbs row (Fest None, or FEIGN before its first day)");
}

/**
 * FestBandstand, the arch's half (§2.7.4, §2.15.2; F-hubs' FestBandstandPick holds the bandstand's pick): FEIGN's arch stands, and the
 * Prize Counter booth does not (no awning, back wall or counter colour inside the booth's rect), only while FEIGN is Live (day 1 to its
 * last); before its first day, after its last and for every other brand the band is today's byte for byte, both layers. PUSH stands
 * its tower and PENNANT its pavilion over the bandstand (in place of the shell, its dome carrying the bulbs), each keeping the Prize
 * Counter; the arch and the pavilion carry U32's foot sign once, inside their own rects, and it fits; PUSH's puck rises with the
 * festival's days (the top on the last); no attraction wears a RiverLine hex (SitesNoLeak's rule). In the day's and Afterburner's tones.
 * Planted bugs: the Prize Counter still drawn under the arch; its back wall alone drawn behind the arch.
 */
void FestBandstand()
{
	const ss::site::show::Mood Moods[2] = {ss::site::show::Mood::PrimeTime, ss::site::show::Mood::Afterburner};
	const std::array<uint32_t, 16>& Home = ss::net::Def(SiteId::RiverLine).Palette;
	const std::string Sign = ss::ui::festart::festart_detail::FestHubSign();
	for (int Mi = 0; Mi < 2; ++Mi)
	{
		const char* Tone = Mi == 0 ? "Prime Time" : "Afterburner";
		const ss::ui::DrawList Today = DrawBandLook(BandLook(Brand::None, 0, 0, Moods[Mi], 41.0, false), 1.0f, true, true);
		const uint64_t TodayPrint = ListPrintLook(Today);
		const uint32_t Shell = Mi == 0 ? 0x2a1546 : 0x1e1440;
		const auto SignsIn = [&Sign](const ss::ui::DrawList& L, const ss::ui::Rect& R) {
			int Inside = 0;
			int All = 0;
			for (const ss::ui::TextItem& T : RunsLook(L))
			{
				All += T.Text == Sign ? 1 : 0;
				Inside += T.Text == Sign && T.X >= R.X && T.X <= R.X + R.W && T.BaselineY >= R.Y && T.BaselineY <= R.Y + R.H ? 1 : 0;
			}
			return std::make_pair(Inside, All);
		};
		// Today's band: the booth (awning and back wall), the shell, no attraction, no sign.
		ExpectLook(WearsLook(Today, BoothAwningLook, BoothLook) > 0 && WearsLook(Today, BoothWallLook[Mi], BoothLook) > 0 && WearsLook(Today, Shell, PavilionLook) > 0 &&
				SignsIn(Today, PavilionLook).second == 0 &&
				WearsLook(Today, festart::LookOf(Brand::Feign).Accent2, ArchLook) == 0 && WearsLook(Today, festart::LookOf(Brand::Push).Accent, TowerLook) == 0,
			std::string("FestBandstand: today's band keeps the Prize Counter and the bandstand's shell and stands no attraction (") + Tone + ")");
		// Only while Live: before day 1, after the last day and for any other brand the band is today's, byte for byte.
		bool Same = true;
		std::string Off;
		const std::pair<Brand, int> NotLive[] = {{Brand::Feign, 0}, {Brand::Feign, 11}, {Brand::Push, 0}, {Brand::Pennant, 14}, {Brand::Rcop, 3}, {Brand::Writ, 3},
			{Brand::Reign, 3}, {Brand::Slam, 3}};
		for (const auto& Case : NotLive)
		{
			const int Days = Case.first == Brand::Pennant || Case.first == Brand::Reign ? 13 : 10;
			const bool Equal = ListPrintLook(DrawBandLook(BandLook(Case.first, Case.second, Days, Moods[Mi], 41.0, false), 1.0f, true, true)) == TodayPrint;
			Same = Same && Equal;
			Off += Equal ? "" : std::string(" ") + NameLook(Case.first) + " day " + std::to_string(Case.second);
		}
		ExpectLook(Same, std::string("FestBandstand: outside a festival's days, and for every brand without an attraction, the band is today's byte for byte (") + Tone + ")" + Off);
		// FEIGN Live: the arch, no booth, the sign once inside the arch.
		for (const int Day : {1, 3, 10})
		{
			const ss::ui::DrawList L = DrawBandLook(BandLook(Brand::Feign, Day, 10, Moods[Mi], 41.0, false), 1.0f, true, true);
			const auto Signs = SignsIn(L, ArchLook);
			const size_t Mint = WearsLook(L, festart::LookOf(Brand::Feign).Accent2, ArchLook);
			const size_t Pink = WearsLook(L, festart::LookOf(Brand::Feign).Accent, ArchLook);
			char Where[96];
			std::snprintf(Where, sizeof(Where), " (day %d of 10, %s: %zu mint, %zu pink, signs %d/%d)", Day, Tone, Mint, Pink, Signs.first, Signs.second);
			ExpectLook(Mint >= 40 && Pink >= 40 && Signs.first == 1 && Signs.second == 1,
				std::string("FestBandstand: FEIGN's arch stands at the boardwalk's end with its foot sign while FEIGN is Live") + Where);
			ExpectLook(WearsLook(L, BoothAwningLook, BoothLook) == 0 && WearsLook(L, ss::ui::igniteart::IgGrape, BoothLook) == 0 &&
					WearsLook(L, BoothWallLook[0], BoothLook) == 0 && WearsLook(L, BoothWallLook[1], BoothLook) == 0,
				std::string("FestBandstand: the arch replaces the Prize Counter booth (none of the booth drawn under it: awning, back wall, counter)") + Where);
		}
		// PUSH: the tower, the booth kept, no sign; PENNANT: the pavilion in the shell's place, the booth kept, the sign once inside it.
		const ss::ui::DrawList Push = DrawBandLook(BandLook(Brand::Push, 3, 10, Moods[Mi], 41.0, false), 1.0f, true, true);
		const size_t Brass = WearsLook(Push, festart::LookOf(Brand::Push).Accent, TowerLook);
		ExpectLook(Brass >= 24 && WearsLook(Push, BoothAwningLook, BoothLook) > 0 && WearsLook(Push, Shell, PavilionLook) > 0 && SignsIn(Push, PavilionLook).second == 0,
			std::string("FestBandstand: PUSH stands its tower and keeps the Prize Counter and the bandstand (") + Tone + ": " + std::to_string(Brass) + " brass)");
		const ss::ui::DrawList Pennant = DrawBandLook(BandLook(Brand::Pennant, 3, 13, Moods[Mi], 41.0, false), 1.0f, true, true);
		const auto PavSigns = SignsIn(Pennant, PavilionLook);
		const size_t Green = WearsLook(Pennant, festart::LookOf(Brand::Pennant).Accent, PavilionLook);
		const size_t Bulbs = WearsLook(Pennant, festart::LookOf(Brand::Pennant).Gold, PavilionLook);
		char Pav[96];
		std::snprintf(Pav, sizeof(Pav), " (%s: %zu green, %zu bulb white, signs %d/%d)", Tone, Green, Bulbs, PavSigns.first, PavSigns.second);
		ExpectLook(Green >= 40 && Bulbs >= 20 && WearsLook(Pennant, Shell, PavilionLook) == 0 && WearsLook(Pennant, BoothAwningLook, BoothLook) > 0 && PavSigns.first == 1 &&
				PavSigns.second == 1,
			std::string("FestBandstand: PENNANT's pavilion stands over the bandstand in the shell's place with its foot sign, the Prize Counter kept") + Pav);
		// No attraction wears a RiverLine hex (the rival sites' leak rule, on the band alone).
		for (const Brand B : AttractionsLook)
		{
			const ss::ui::DrawList L = DrawBandLook(BandLook(B, 3, B == Brand::Pennant ? 13 : 10, Moods[Mi], 41.0, true), 1.0f, true, true);
			uint32_t Leak = 0;
			for (const ss::ui::Vertex& V : L.Vertices)
			{
				const uint32_t C = RgbLook(V.Col);
				Leak = Leak == 0 && V.Col.A > 0.02f && std::find(Home.begin(), Home.end(), C) != Home.end() ? C : Leak;
			}
			char Hit[96];
			std::snprintf(Hit, sizeof(Hit), " (%s, %s%s%06x)", NameLook(B), Tone, Leak ? ": " : "", Leak);
			ExpectLook(Leak == 0, std::string("FestBandstand: the attraction wears no RiverLine palette hex") + Hit);
		}
	}
	// The sign fits its plate (the arch's 192, the pavilion's 176, 8 units of margin a side) at the sign's 11.5 Black.
	{
		ui_test::TableMeasurer M;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 1600.0f, 1000.0f, 1.0f);
		const float W = C.Measure(Sign, 11.5f, 900);
		std::printf("  bandstand: the foot sign '%s' measures %.1f at 11.5 Black (plates 192 and 176)\n", Sign.c_str(), static_cast<double>(W));
		ExpectLook(W > 0.0f && W <= 176.0f - 16.0f, "FestBandstand: the foot sign fits the arch's and the pavilion's plates");
	}
	// PUSH's puck rises with the festival: day 1 low, day 5 higher, the top on the last day.
	{
		float Prev = 1e9f;
		bool Rises = true;
		std::string Line;
		for (const int Day : {1, 5, 10})
		{
			const ss::ui::DrawList L = DrawBandLook(BandLook(Brand::Push, Day, 10, ss::site::show::Mood::PrimeTime, 41.0, false), 1.0f, false, true);
			float Sum = 0.0f;
			int N = 0;
			for (const ss::ui::Vertex& V : L.Vertices)
			{
				if (V.Col.A >= 0.9f && RgbLook(V.Col) == festart::LookOf(Brand::Push).Gold && V.X >= PuckTrackLook.X && V.X <= PuckTrackLook.X + PuckTrackLook.W &&
					V.Y >= PuckTrackLook.Y && V.Y <= PuckTrackLook.Y + PuckTrackLook.H)
				{
					Sum += V.Y;
					++N;
				}
			}
			const float Y = N > 0 ? Sum / static_cast<float>(N) : 1e9f;
			Rises = Rises && N > 0 && Y < Prev - 10.0f;
			Prev = Y;
			char Part[40];
			std::snprintf(Part, sizeof(Part), " day %d: y %.1f", Day, static_cast<double>(Y));
			Line += Part;
		}
		std::printf("  bandstand: PUSH's puck%s\n", Line.c_str());
		ExpectLook(Rises && std::fabs(Prev - 122.0f) <= 4.0f, "FestBandstand: PUSH's puck rises with the festival's days, the top on its last" + Line);
	}
}

/** FestChampWord's site rule (§2.11, F-net's FestNetChampWord): "Ignite championship", "Ignite's championship", or Ignite within three
 *  words of "championship". */
bool ChampWordLook(const std::string& Text)
{
	const std::string Low = trademarks::Lower(Text);
	if (Low.find("ignite championship") != std::string::npos || Low.find("ignite's championship") != std::string::npos)
	{
		return true;
	}
	std::vector<std::string> Words;
	std::string W;
	for (const char C : Low + " ")
	{
		if ((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '\'')
		{
			W += C;
		}
		else if (!W.empty())
		{
			Words.push_back(W);
			W.clear();
		}
	}
	for (size_t I = 0; I < Words.size(); ++I)
	{
		for (size_t J = I >= 3 ? I - 3 : 0; Words[I].rfind("championship", 0) == 0 && J < std::min(Words.size(), I + 4); ++J)
		{
			if (J != I && Words[J].rfind("ignite", 0) == 0)
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * FestCopyUi, the art half (§1.2 items 1-5, 9, 12-13, §2.11, §2.15.2; F-hubs' half reads FestUiInventory): every FestArtInventory entry
 * carries a known kind and passes the copy lint by it (a label has no closing stop, 200; a sentence its stop), both trademark tiers, the
 * design words after the sample fill, the avoided words after the fill (§1.2 item 5's list, tests/festivals_main.cpp's FestCopy list:
 * spark ... premier, whole words, any case; F-hubs' half screens FestUiInventory, so this is FestArtInventory's only avoided-word
 * screen), U1 ("AI", "bot(s)", "autoplay", "autopilot"), X1 ("players" in a count), "marquee" and "pavilion" (art names, never copy),
 * "championship" (FestChampWord: the arch's and the pavilion's words are Ignite's, which never print it), "reign cabinet"
 * (FestReignCopy), the glyph rule (ASCII and · — – … é ’) and the article and year rules (each {Festival} filled again with "The
 * Championship Online 2027": no "the The"). And the art draws only listed words: every text run the band gains with an attraction is
 * an entry.
 * Planted bugs: "bot" in a line; an avoided word in a line.
 */
void FestCopyUiLook()
{
	std::map<std::string, std::string> Vars = copylint::Samples();
	const std::map<std::string, std::string> Extra = {{"Festival", "PENNANT 2027"}, {"SHORT", "PENNANT"}, {"Short", "PENNANT"}, {"No", "6"}, {"Year", "2027"},
		{"Site", "Ignite"}};
	for (const auto& Kv : Extra)
	{
		Vars[Kv.first] = Kv.second;
	}
	// §1.2 item 5 (other features own them), as tests/festivals_main.cpp's FestCopy screens fest::Inventory().
	const char* const AvoidedLook[] = {"spark", "flame", "blaze", "wildfire", "ember", "crest", "heat", "cold", "stamina", "momentum", "energy", "board", "watchlist",
		"headliners", "beef", "hall", "class", "boom", "millions", "jackpot", "run good", "supernova", "premier"};
	int Entries = 0;
	int Problems = 0;
	std::string First;
	for (const std::string& Entry : ss::fest::FestArtInventory())
	{
		++Entries;
		const size_t Tab = Entry.find('\t');
		const std::string Kind = Tab == std::string::npos ? std::string() : Entry.substr(0, Tab);
		const std::string Text = Tab == std::string::npos ? Entry : Entry.substr(Tab + 1);
		const bool Known = Kind == "label" || Kind == "sentence" || Kind == "text" || Kind == "subject" || Kind == "letter" || Kind == "headtitle" || Kind == "headbody" ||
			Kind == "journey";
		const bool Stop = !(Kind == "label" || Kind == "subject" || Kind == "headtitle" || Kind == "text");
		const size_t Cap = Kind == "label" || Kind == "subject" ? copylint::MailSubject : Kind == "headtitle" ? copylint::kHeadTitleMax : Kind == "text" ? copylint::TableTalk
			: Kind == "headbody" ? copylint::kHeadBodyMax : Kind == "journey" ? copylint::kJourneyMax : copylint::LetterBody;
		const std::string Filled = copylint::Fill(Text, Vars);
		const bool Ends = copylint::EndsWith(Filled, ".") || copylint::EndsWith(Filled, "!") || copylint::EndsWith(Filled, "?");
		std::string Why = Known ? std::string() : " an unknown kind \"" + Kind + "\"";
		const std::string Lint = copylint::Check(Filled + (Stop || Ends ? "" : "."), Cap + (Stop || Ends ? 0 : 1), {});
		Why += Lint.empty() ? std::string() : " " + Lint;
		Why += !Stop && Ends && Kind != "text" ? std::string(" a label ends in a stop") : std::string();
		for (const char* Hit : {trademarks::Find(Filled, trademarks::kSourceScan), trademarks::Find(Filled, trademarks::kCopyScan), trademarks::Find(Filled, trademarks::kDesignWords)})
		{
			Why += Hit ? std::string(" trademark or design word: ") + Hit : std::string();
		}
		const std::string Low = trademarks::Lower(Filled);
		for (const char* W : {"ai", "bot", "bots", "autoplay", "autopilot", "players", "marquee", "pavilion"})
		{
			Why += trademarks::HasWord(Low, W) ? std::string(" U1/X1/art name: ") + W : std::string();
		}
		for (const char* W : AvoidedLook)
		{
			Why += trademarks::HasWord(Low, W) ? std::string(" avoided word: ") + W : std::string();
		}
		Why += Low.find("championship") != std::string::npos || ChampWordLook(Filled) ? " championship (FestChampWord)" : "";
		Why += Low.find("reign cabinet") != std::string::npos ? " \"reign cabinet\" (FestReignCopy)" : "";
		for (size_t K = 0; K < Filled.size(); ++K)
		{
			if (static_cast<unsigned char>(Filled[K]) < 0x80)
			{
				continue;
			}
			const std::string G2 = Filled.substr(K, 2);
			const std::string G3 = Filled.substr(K, 3);
			if (G2 == "\xC2\xB7" || G2 == "\xC3\xA9")
			{
				++K;
			}
			else if (G3 == "\xE2\x80\x94" || G3 == "\xE2\x80\x93" || G3 == "\xE2\x80\xA6" || G3 == "\xE2\x80\x99")
			{
				K += 2;
			}
			else
			{
				Why += " a glyph outside the rule";
				break;
			}
		}
		std::map<std::string, std::string> Again = Vars;
		Again["Festival"] = "The Championship Online 2027";
		Why += trademarks::Lower(copylint::Fill(Text, Again)).find("the the") != std::string::npos ? " \"the The\" with The Championship Online" : "";
		if (!Why.empty())
		{
			++Problems;
			First = First.empty() ? ": \"" + Filled + "\" [" + Kind + "]" + Why : First;
			std::printf("COPY (u4_ui art) \"%s\" [%s]:%s\n", Filled.c_str(), Kind.c_str(), Why.c_str());
		}
	}
	ExpectLook(Entries > 0 && Problems == 0,
		"FestCopyUi: FestArtInventory's " + std::to_string(Entries) + " entries pass the copy lint by kind, both trademark tiers, the design words, the avoided words "
		"(" + std::to_string(sizeof(AvoidedLook) / sizeof(AvoidedLook[0])) + ", after the fill), U1, X1, the art names, FestChampWord, FestReignCopy's phrase, the glyph rule and the "
		"article rule (" + std::to_string(Problems) + " problems)" + First);
	// The art draws only listed words: what each attraction adds to the band's text is in FestArtInventory.
	const std::vector<std::string> Listed = ArtTextsLook();
	std::set<std::string> Today;
	for (const ss::ui::TextItem& T : RunsLook(DrawBandLook(BandLook(Brand::None, 0, 0, ss::site::show::Mood::PrimeTime, 41.0, false), 1.0f, true, true)))
	{
		Today.insert(T.Text);
	}
	int Drawn = 0;
	std::string Unlisted;
	for (const Brand B : AttractionsLook)
	{
		for (const ss::ui::TextItem& T : RunsLook(DrawBandLook(BandLook(B, 3, B == Brand::Pennant ? 13 : 10, ss::site::show::Mood::PrimeTime, 41.0, false), 1.0f, true, true)))
		{
			if (Today.count(T.Text) == 0)
			{
				++Drawn;
				Unlisted += std::find(Listed.begin(), Listed.end(), T.Text) == Listed.end() ? " \"" + T.Text + "\" (" + NameLook(B) + ")" : std::string();
			}
		}
	}
	ExpectLook(Drawn >= 2 && Unlisted.empty(), "FestCopyUi: every word the attractions paint on the band is a FestArtInventory entry (" + std::to_string(Drawn) + " runs)" + Unlisted);
}

// ------------------------------------------------------------------ G1 FL-4: the takeover (§2.7.3, §2.7.4, §2.15.2)

/** Hooks that let the laptop go out (a site's deposit) and stay quiet otherwise. */
struct TakeHooksLook : ui_test::QuietHooks
{
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

/** A session and its laptop on a site at a world minute (the rivals' kits' pattern, this file's own copy). */
struct LaptopLook
{
	TakeHooksLook H;
	ss::Session S;
	ss::ui::RiverLine RL;
	ui_test::TableMeasurer M;
	double Now = 1.0;
	SiteId Site = SiteId::RiverLine;
	LaptopLook(const std::string& Seed, SiteId On, double World) : S(H, Seed), RL(S), Site(On)
	{
		// The clock is set before the session's first update, so its world starts there (no days to catch up).
		S.CurrentScreen = ss::Screen::Lobby;
		At(World);
		S.BankrollCents = 2000000;
		if (On != SiteId::RiverLine)
		{
			ss::site::Join(S.Life.Sites, On, S.WorldMinutes());
			if (ss::site::Account* A = ss::site::AccountOf(S.Life.Sites, On))
			{
				A->Cents = 1000000;
			}
			S.Life.Update2LinesRead |= ss::life::U2Sites;
		}
	}
	~LaptopLook() { ss::ui::rlignite_detail::IgniteForget(&RL); }
	LaptopLook(const LaptopLook&) = delete;
	LaptopLook& operator=(const LaptopLook&) = delete;
	void At(double World)
	{
		if (World > S.WorldMinutes())
		{
			S.LobbyMinutes += World - S.WorldMinutes();
		}
		Now = ui_test::Step(S, Now, 0.2);
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 80.0;
	}
	void Open(ss::ui::RiverLine::Page P)
	{
		RL.SiteShown = Site;
		RL.OpenApp(ss::ui::RiverLine::App::RiverLine, Now);
		S.ShowLobby();
		RL.OpenPage(P, Now);
	}
	ss::ui::DrawList Frame(float Scale = 1.0f)
	{
		RL.UI.Ptr.Active = false;
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, Scale);
		RL.Draw(C, Now);
		RL.UI.Ptr.EndFrame();
		return L;
	}
	ss::ui::DrawList Settled(float Scale = 1.0f)
	{
		Now += 1.5;
		Frame();
		Now += 0.05;
		return Frame(Scale);
	}
};

/** The first edition on a site (of a brand, if given) in the calendar (fest::All, F-net's). */
const ss::fest::Festival* FirstOnLook(SiteId Site, Brand Want = Brand::None)
{
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Site == Site && (Want == Brand::None || F.Which == Want))
		{
			return &F;
		}
	}
	return nullptr;
}

/** The skin §2.7.3 gives Site at world minute M, worked out from the calendar and the site's palettes (not SitesSkin's cache): the
 *  night window first, then any edition on the site from its FirstDay 00:00 to 06:00 on LastDay + 1, else the day palette. */
ss::ui::Skin RuleSkinLook(SiteId Site, double M)
{
	const ss::net::SiteDef& D = ss::net::Def(Site);
	const long long Min = static_cast<long long>(std::floor(M));
	const long long OfDay = ((Min % 1440) + 1440) % 1440;
	const bool HasNight = std::any_of(D.NightPalette.begin(), D.NightPalette.end(), [](uint32_t V) { return V != 0; }) && D.NightFromMin != D.NightToMin;
	if (HasNight && (D.NightFromMin < D.NightToMin ? OfDay >= D.NightFromMin && OfDay < D.NightToMin : OfDay >= D.NightFromMin || OfDay < D.NightToMin))
	{
		return ss::ui::Skin::FromHex(D.NightPalette);
	}
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.Site == Site && Min >= static_cast<long long>(F.FirstDay) * 1440 && Min < (static_cast<long long>(F.LastDay) + 1) * 1440 + 360)
		{
			return festart::Dressed(ss::ui::Skin::FromHex(D.Palette), festart::LookOf(F.Which));
		}
	}
	return ss::ui::Skin::FromHex(D.Palette);
}

bool SameSkinLook(const ss::ui::Skin& A, const ss::ui::Skin& B)
{
	return HexesLook(A) == HexesLook(B);
}

/**
 * FestSkinRule (§2.15.2, §2.7.3): SitesSkin dresses a site in its festival's six tokens from the edition's FirstDay 00:00 to 06:00 on
 * LastDay + 1 (RCOP 2026: day 13 00:00 to day 35 06:00); Ignite's 00:00-06:00 returns its NightPalette during FEIGN (Afterburner Hours
 * win); every other hour of days 0-400 on all three sites is today's palette byte for byte. ss.Fest.Look's dev look dresses ahead of
 * the calendar, never over Afterburner, and goes when turned off. The branch's cost: a bench line over 10,000 calls (printed).
 * Planted bug: the festival branch before the night window (festival over Afterburner).
 */
void FestSkinRule()
{
	using ss::ui::rlsites_detail::SitesSkin;
	{
		// LEAD_CALLS R-18 (U37): a festival's step ticket ("<series>-s2a", "-s3a", a two-day step's "-s2b") is left out of WELCOME
		// TICKETS; an event's own ticket, a championship's Main ticket and a "-s2a" of no festival are not step tickets.
		using ss::ui::rlsites_detail::SitesFestStepTicket;
		const ss::fest::Festival* Reign = FirstOnLook(SiteId::RunItTwice, Brand::Reign);
		const ss::fest::Festival* Pennant = FirstOnLook(SiteId::Ignite, Brand::Pennant);
		const bool Steps = Reign && Pennant && SitesFestStepTicket(Reign->SeriesId + "-s2a") && SitesFestStepTicket(Reign->SeriesId + "-s3a") &&
			SitesFestStepTicket(Pennant->SeriesId + "-s2a") && SitesFestStepTicket(Pennant->SeriesId + "-s2b") &&
			ss::net::Site(SiteId::RunItTwice).FindTemplate(Reign->SeriesId + "-s2a") != nullptr;
		const bool Others = Reign && !SitesFestStepTicket(Reign->SeriesId + "-1") && !SitesFestStepTicket("runittwice.reign-main") && !SitesFestStepTicket("runittwice.nofest27-s2a") &&
			!SitesFestStepTicket(Reign->SeriesId + "-s") && !SitesFestStepTicket(Reign->SeriesId + "-s2c") && !SitesFestStepTicket("s2a");
		ExpectLook(Steps && Others, "FestCopyUi (R-18): WELCOME TICKETS leaves out a festival's step tickets and only those");
	}
	const SiteId Hosts[] = {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice};
	int Shown = 0;
	for (const ss::fest::Festival& F : ss::fest::All())
	{
		if (F.FirstDay <= 400 && Shown < 12)
		{
			std::printf("  takeover calendar: %-22s on %-10s days %3d-%3d (Main day %d)\n", F.Name.c_str(), ss::net::Def(F.Site).Name.c_str(), F.FirstDay, F.LastDay, F.MainDay);
			++Shown;
		}
	}
	int Hours = 0, Wrong = 0, Dressed = 0, Night = 0;
	std::string First;
	for (const SiteId Site : Hosts)
	{
		for (int Day = 0; Day <= 400; ++Day)
		{
			for (int H = 0; H < 24; ++H)
			{
				const double M = Day * 1440.0 + H * 60.0 + 30.0;
				const ss::ui::Skin Want = RuleSkinLook(Site, M);
				const ss::ui::Skin Got = SitesSkin(Site, M);
				++Hours;
				Dressed += SameSkinLook(Want, ss::ui::Skin::FromHex(ss::net::Def(Site).Palette)) ? 0 : 1;
				if (!SameSkinLook(Want, Got))
				{
					++Wrong;
					if (First.empty())
					{
						char Buf[96];
						std::snprintf(Buf, sizeof(Buf), " (first: %s day %d %02d:30)", ss::net::Def(Site).Name.c_str(), Day, H);
						First = Buf;
					}
				}
			}
		}
	}
	std::printf("  takeover skin rule: %d site-hours of days 0-400, %d dressed or at night, %d wrong%s\n", Hours, Dressed, Wrong, First.c_str());
	ExpectLook(Hours == 3 * 401 * 24 && Wrong == 0, "FestSkinRule: every hour of days 0-400 on all three sites is the rule's skin (the festival's window, else today's byte for byte)" + First);
	// RCOP 2026's edges, the SPEC's example.
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	const ss::ui::Skin RlDay = ss::ui::Skin::FromHex(ss::net::Def(SiteId::RiverLine).Palette);
	const ss::ui::Skin RlRcop = festart::Dressed(RlDay, festart::LookOf(Brand::Rcop));
	ExpectLook(Rcop && Rcop->FirstDay == 13 && Rcop->LastDay == 34 && SameSkinLook(SitesSkin(SiteId::RiverLine, 13 * 1440.0 - 0.5), RlDay) &&
			SameSkinLook(SitesSkin(SiteId::RiverLine, 13 * 1440.0), RlRcop) && SameSkinLook(SitesSkin(SiteId::RiverLine, 35 * 1440.0 + 359.5), RlRcop) &&
			SameSkinLook(SitesSkin(SiteId::RiverLine, 35 * 1440.0 + 360.0), RlDay),
		"FestSkinRule: RCOP 2026 dresses RiverLine from day 13 00:00 to day 35 06:00 and not a minute outside");
	// FEIGN on Ignite: Afterburner Hours win; the festival dresses 06:00-24:00.
	const ss::fest::Festival* Feign = FirstOnLook(SiteId::Ignite, Brand::Feign);
	if (Feign)
	{
		const ss::net::SiteDef& Ig = ss::net::Def(SiteId::Ignite);
		const double Day2 = (Feign->FirstDay + 1) * 1440.0;
		const ss::ui::Skin Ignited = festart::Dressed(ss::ui::Skin::FromHex(Ig.Palette), festart::LookOf(Brand::Feign));
		++Night;
		ExpectLook(SameSkinLook(SitesSkin(SiteId::Ignite, Day2 + 120.0), ss::ui::Skin::FromHex(Ig.NightPalette)) &&
				SameSkinLook(SitesSkin(SiteId::Ignite, Day2 + 359.0), ss::ui::Skin::FromHex(Ig.NightPalette)) && SameSkinLook(SitesSkin(SiteId::Ignite, Day2 + 360.0), Ignited) &&
				SameSkinLook(SitesSkin(SiteId::Ignite, Day2 + 21 * 60.0), Ignited),
			"FestSkinRule: during FEIGN Ignite's 00:00-06:00 returns its NightPalette untouched (Afterburner Hours win) and FEIGN's dress from 06:00");
	}
	ExpectLook(Feign != nullptr && Night == 1, "FestSkinRule: the calendar has an FEIGN edition on Ignite");
	// A RunItTwice edition keeps Signal Red and Hot White (§2.7.3).
	const ss::fest::Festival* RitFest = FirstOnLook(SiteId::RunItTwice);
	if (RitFest)
	{
		const ss::ui::Skin K = SitesSkin(SiteId::RunItTwice, RitFest->FirstDay * 1440.0 + 20 * 60.0);
		const std::array<uint32_t, 16> X = HexesLook(K);
		const std::array<uint32_t, 16>& Own = ss::net::Def(SiteId::RunItTwice).Palette;
		ExpectLook(X[8] == Own[8] && X[10] == Own[10] && X[9] == festart::LookOf(RitFest->Which).Accent2,
			std::string("FestSkinRule: a RunItTwice festival keeps Signal Red and Hot White and swaps its Accent2 (") + RitFest->Name + ")");
	}
	// The dev look (§2.16): ahead of the calendar, never over Afterburner, gone when turned off.
	const double Plain = 5 * 1440.0 + 12 * 60.0;
	festart::DevLook(Brand::Pennant);
	const bool DevDressed = SameSkinLook(SitesSkin(SiteId::RiverLine, Plain), festart::Dressed(RlDay, festart::LookOf(Brand::Pennant))) &&
		SameSkinLook(SitesSkin(SiteId::RiverLine, 20 * 1440.0 + 12 * 60.0), festart::Dressed(RlDay, festart::LookOf(Brand::Pennant)));
	const bool DevNight = SameSkinLook(SitesSkin(SiteId::Ignite, Plain - 10 * 60.0), ss::ui::Skin::FromHex(ss::net::Def(SiteId::Ignite).NightPalette));
	const bool DevRead = festart::DevLook() == Brand::Pennant;
	festart::DevLook(Brand::None);
	ExpectLook(DevRead && DevDressed && DevNight && festart::DevLook() == Brand::None && SameSkinLook(SitesSkin(SiteId::RiverLine, Plain), RlDay) &&
			SameSkinLook(SitesSkin(SiteId::RiverLine, 20 * 1440.0 + 12 * 60.0), RlRcop),
		"FestSkinRule: ss.Fest.Look's dev look dresses ahead of the calendar, never over Afterburner, and off puts the calendar's back");
	// The bench line (§2.7.3: a compare a frame once the site and day are known).
	const auto T0 = std::chrono::steady_clock::now();
	uint64_t Sum = 0;
	for (int K = 0; K < 10000; ++K)
	{
		Sum += HexesLook(SitesSkin(SiteId::RiverLine, 20 * 1440.0 + 600.0 + K * 0.001))[8];
	}
	const double Us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - T0).count();
	std::printf("  takeover bench: SitesSkin x 10,000 on one site and day (RCOP live): %.0f us, %.3f us a call (%llu)\n", Us, Us / 10000.0, static_cast<unsigned long long>(Sum % 7));
}

/** What a page drew: its vertices, runs (text commands) and draw commands. */
struct PageCountLook
{
	size_t Vertices = 0, Runs = 0, Cmds = 0;
};

PageCountLook CountLook(const ss::ui::DrawList& L)
{
	PageCountLook P;
	P.Vertices = L.Vertices.size();
	P.Cmds = L.Cmds.size();
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		P.Runs += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
	}
	return P;
}

/** Whether a vertex that shows (or a text run) wears Rgb exactly. */
bool WearsLook(const ss::ui::DrawList& L, uint32_t Rgb)
{
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		if (V.Col.A > 0.05f && RgbLook(V.Col) == Rgb)
		{
			return true;
		}
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && RgbLook(Cmd.Text.Col) == Rgb)
		{
			return true;
		}
	}
	return false;
}

/** SitesNoLeak's rule (tests/ui_seg_sites.cpp:274, riverline_ui's): colours drawn in Area (the client area unless given: vertices that
 *  show, text runs) that are one of RiverLine's 16 tokens and not one of the host's own; a speck under 12 by 12 is shared art's ink. */
int LeaksLook(const ss::ui::DrawList& L, SiteId Host, std::string& Seen, const ss::ui::Rect& Area = {0.0f, 0.0f, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height - 38.0f})
{
	const std::array<uint32_t, 16>& HomeA = ss::net::Def(SiteId::RiverLine).Palette;
	const std::array<uint32_t, 16>& OwnA = ss::net::Def(Host).Palette;
	const std::set<uint32_t> Home(HomeA.begin(), HomeA.end());
	const std::set<uint32_t> Own(OwnA.begin(), OwnA.end());
	const auto Inside = [&Area](float X, float Y) { return X >= Area.X && X <= Area.X + Area.W && Y >= Area.Y && Y <= Area.Y + Area.H; };
	std::set<uint32_t> Found;
	int Count = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			const uint32_t Rgb = RgbLook(Cmd.Text.Col);
			if (Cmd.Text.Col.A > 0.02f && Inside(Cmd.Text.X, Cmd.Text.BaselineY) && Home.count(Rgb) && !Own.count(Rgb))
			{
				++Count;
				Found.insert(Rgb);
			}
			continue;
		}
		if (Cmd.Type != ss::ui::DrawCmd::Kind::Triangles)
		{
			continue;
		}
		int Here = 0;
		float X0 = 1.0e9f, Y0 = 1.0e9f, X1 = -1.0e9f, Y1 = -1.0e9f;
		std::set<uint32_t> Mine;
		for (uint32_t K = 0; K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
			const uint32_t Rgb = RgbLook(V.Col);
			if (V.Col.A > 0.02f && Inside(V.X, V.Y) && Home.count(Rgb) && !Own.count(Rgb))
			{
				++Here;
				Mine.insert(Rgb);
				X0 = std::min(X0, V.X);
				Y0 = std::min(Y0, V.Y);
				X1 = std::max(X1, V.X);
				Y1 = std::max(Y1, V.Y);
			}
		}
		if (Here > 0 && (X1 - X0 > 12.0f || Y1 - Y0 > 12.0f))
		{
			Count += Here;
			Found.insert(Mine.begin(), Mine.end());
		}
	}
	for (const uint32_t H : Found)
	{
		char Buf[12];
		std::snprintf(Buf, sizeof(Buf), " %06x", H);
		Seen += Buf;
	}
	return Count;
}

/** One of the dev look's draws (ss.Fest.Look): a host's lobby in one look, at both scales. */
struct LookDrawLook
{
	Brand B = Brand::None;
	SiteId Host = SiteId::RiverLine;
	ss::ui::DrawList L1, L15;
};

/** One festival page drawn at both scales: printed (vertices, runs; SS_UI_BENCH: ms a draw), emitted at scale 1. */
struct TakePageLook
{
	std::string Name;
	SiteId Host = SiteId::RiverLine;
	Brand Look = Brand::None; // the look it should wear (None: Afterburner's night)
	ss::ui::DrawList L1, L15;
	bool Drawn = false;
	uint32_t Felt = 0, FeltLight = 0, FeltDark = 0; // a table's: SitesTableLook's felt and card back as it drew
	int Back = 0;
	bool Marked = false; // a table's felt mark was drawn (cached at its host's size)
	double Seconds = 0.0;
};

/** What the takeover drew: the seven pages (the calendar's), each look's host page (the dev look) and REIGN's table (the dev look on
 *  WRIT's). */
struct TakeoverLook
{
	std::vector<TakePageLook> Pages;
	std::vector<LookDrawLook> Looks;
	TakePageLook ReignTable;
};

void MeasureLook(TakePageLook& P, LaptopLook& Lap, bool Emit = true)
{
	P.L1 = Lap.Settled(1.0f);
	P.L15 = Lap.Frame(1.5f);
	P.Drawn = P.L1.Vertices.size() > 100;
	const PageCountLook A = CountLook(P.L1), B = CountLook(P.L15);
	std::printf("  takeover page %-28s %6zu vertices %4zu runs at 1, %6zu vertices %4zu runs at 1.5\n", P.Name.c_str(), A.Vertices, A.Runs, B.Vertices, B.Runs);
	if (std::getenv("SS_UI_BENCH"))
	{
		const auto T0 = std::chrono::steady_clock::now();
		for (int K = 0; K < 40; ++K)
		{
			Lap.Frame(1.0f);
		}
		std::printf("UI BENCH %-28s %6.2f ms/draw  %6zu vertices\n", P.Name.c_str(), std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 40.0,
			A.Vertices);
	}
	if (Emit)
	{
		ui_test::WriteList(P.Name, P.L1);
	}
}

/** The first numbered event of the edition the tables can run, in late registration from From on, at an hour outside Afterburner's
 *  night (07:00 or later); false if none. */
bool FestEventLook(const ss::fest::Festival& F, double From, ss::net::EventInstance& Out, double& World)
{
	const ss::net::Network& Net = ss::net::Site(F.Site);
	for (const ss::net::EventInstance& E : Net.Window(std::max(From - 60.0, F.FirstDay * 1440.0), (F.LastDay + 1) * 1440.0))
	{
		const ss::net::EventTemplate& T = Net.TemplateOf(E);
		const double At = E.Start + 2.0;
		if (T.Series.empty() || ss::fest::FestivalOf(T.Series) != &F || At < From)
		{
			continue;
		}
		const int Hour = static_cast<int>(std::fmod(At, 1440.0) / 60.0);
		const ss::LobbyEvent Ev = Net.Listing(E);
		if (Hour >= 7 && Ev.Joinable && Ev.BuyInCents > 0 && Ev.BuyInCents <= 500000 && Ev.Spec.Entrants <= 3000 && Net.Live(E, At).St == ss::net::Status::LateReg)
		{
			Out = E;
			World = At;
			return true;
		}
	}
	return false;
}

/** Registers Lap's session for E (late registration) and deals: false (printed) when it didn't seat. */
bool SeatLook(LaptopLook& Lap, const ss::fest::Festival& F, const ss::net::EventInstance& E)
{
	const std::string Why = Lap.S.RegisterEvent(ss::net::Site(F.Site).Listing(E, nullptr, Lap.S.Unlocks()));
	Lap.S.CurrentPace = ss::Pace::Full;
	Lap.Now = ui_test::Step(Lap.S, Lap.Now, 3.0);
	const double W = Lap.S.WorldMinutes();
	std::printf("  takeover table: %s (%s) at day %d %02d:%02d%s%s\n", E.Id.c_str(), F.Name.c_str(), static_cast<int>(W / 1440.0), static_cast<int>(std::fmod(W, 1440.0) / 60.0),
		static_cast<int>(std::fmod(W, 60.0)), Why.empty() ? "" : ": ", Why.c_str());
	return Lap.S.T && Lap.S.CurrentScreen == ss::Screen::Table;
}

ss::ui::DrawList OnCanvasLook(const std::function<void(ss::ui::Canvas&)>& Paint, float Scale);

/** The table in front as drawn now (measured; emitted when Emit), with the look it wears read back from SitesTableLook and whether its
 *  felt mark was drawn (cached at its host's size). */
void TableDrawLook(TakePageLook& P, LaptopLook& Lap, bool Emit = true)
{
	MeasureLook(P, Lap, Emit);
	const ss::ui::rlsites_detail::SiteLook& Worn = ss::ui::rlsites_detail::SitesTableLook(Lap.S);
	P.Felt = Worn.Felt;
	P.FeltLight = Worn.FeltLight;
	P.FeltDark = Worn.FeltDark;
	P.Back = Worn.CardBack;
	const float Size = P.Host == SiteId::RunItTwice ? 110.0f : 120.0f;
	OnCanvasLook([&](ss::ui::Canvas& C) { P.Marked = C.IsCached(festart::festart_detail::FestFeltMarkKey(festart::LookOf(P.Look), Size)); }, 1.0f);
}

/** The laptop's ground: the top colour of the first fill RiverLine::Draw makes (its Bg2 over Bg gradient). */
uint32_t GroundTopLook(const ss::ui::DrawList& L)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Triangles && Cmd.VertexCount > 0)
		{
			return RgbLook(L.Vertices[Cmd.VertexStart].Col);
		}
	}
	return 0;
}

/** Text runs and drawn colours that are a sponsor's (name, short name, patch or brand colours) inside R. */
int SponsorMarksLook(const ss::ui::DrawList& L, const ss::ui::Rect& R)
{
	std::set<std::string> Names;
	std::set<uint32_t> Inks;
	for (const ss::sponsor::Brand& B : ss::sponsor::Brands())
	{
		Names.insert(B.Name);
		Names.insert(B.Short);
		uint32_t Ground = 0, Thread = 0;
		if (ss::ui::sponsorart::PatchColors(B.Id, Ground, Thread))
		{
			Inks.insert(Ground);
			Inks.insert(Thread);
		}
		Inks.insert(B.Color);
		Inks.insert(B.Ink);
	}
	const auto Inside = [&R](float X, float Y) { return X >= R.X && X <= R.X + R.W && Y >= R.Y && Y <= R.Y + R.H; };
	int Count = 0;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text)
		{
			Count += Inside(Cmd.Text.X, Cmd.Text.BaselineY) && (Names.count(Cmd.Text.Text) || Inks.count(RgbLook(Cmd.Text.Col))) ? 1 : 0;
			continue;
		}
		for (uint32_t K = 0; Cmd.Type == ss::ui::DrawCmd::Kind::Triangles && K < Cmd.VertexCount; ++K)
		{
			const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + K];
			Count += V.Col.A > 0.05f && Inside(V.X, V.Y) && Inks.count(RgbLook(V.Col)) ? 1 : 0;
		}
	}
	return Count;
}

/** One drawing on a fresh canvas at a scale: its draw list (the art's fragments stay cached for the thread). */
ss::ui::DrawList OnCanvasLook(const std::function<void(ss::ui::Canvas&)>& Paint, float Scale)
{
	ui_test::TableMeasurer M;
	ss::ui::DrawList L;
	ss::ui::Canvas C(L, M, 1280.0f, 800.0f, Scale);
	Paint(C);
	return L;
}

/** The felt mark's size and centre offset on each host's table (UI/RiverLine.cpp's Table). */
float MarkSizeLook(SiteId Host)
{
	return Host == SiteId::RunItTwice ? 110.0f : 120.0f;
}

/**
 * FestTableBudgets (§2.15.2, §2.7.4, §2.8.10): festival tables within 22k (Ignite) and 19k (RunItTwice) with the felt mark (RiverLine's
 * printed); every look's felt mark at its host's size within 1.5k at both scales, one cached fragment that replays the same, nothing
 * while Suppressed; every look's card back at the tables' sizes within 300 at both scales, cached per look and size; the tables wear
 * the look (felt, back, mark; REIGN its own cloth and the Rose Brass rail); the REIGN felt mark and card back draw no sponsor mark
 * (FestReignCopy's F-look half). Planted bugs: the mark at full detail; a sponsor patch drawn on the REIGN felt.
 */
void FestTableBudgets(const TakeoverLook& T)
{
	size_t MarkMax = 0, BackMax = 0;
	std::string MarkWorst, BackWorst;
	bool MarkCached = true, MarkSame = true, MarkGone = true, BackCached = true;
	const float BackSizes[3][2] = {{44.0f, 62.0f}, {72.0f, 100.0f}, {84.0f, 118.0f}};
	std::string PerLook;
	for (const Brand B : AllLook)
	{
		size_t LookMark = 0, LookBack = 0;
		const festart::FestivalLook& L = festart::LookOf(B);
		const float Size = MarkSizeLook(HostLook(B));
		for (const float Scale : {1.0f, 1.5f})
		{
			const ss::ui::DrawList First = OnCanvasLook([&](ss::ui::Canvas& C) { festart::FeltMark(C, L, 400.0f, 300.0f, Size, false); }, Scale);
			bool Cached = false;
			const ss::ui::DrawList Again = OnCanvasLook([&](ss::ui::Canvas& C) {
				Cached = C.IsCached(festart::festart_detail::FestFeltMarkKey(L, Size));
				festart::FeltMark(C, L, 400.0f, 300.0f, Size, false);
			}, Scale);
			const ss::ui::DrawList Hidden = OnCanvasLook([&](ss::ui::Canvas& C) { festart::FeltMark(C, L, 400.0f, 300.0f, Size, true); }, Scale);
			MarkCached = MarkCached && Cached;
			MarkSame = MarkSame && First.Vertices.size() == Again.Vertices.size() && First.Vertices.size() > 20;
			MarkGone = MarkGone && Hidden.Vertices.empty() && Hidden.Cmds.empty();
			LookMark = std::max(LookMark, First.Vertices.size());
			if (First.Vertices.size() > MarkMax)
			{
				MarkMax = First.Vertices.size();
				MarkWorst = std::string(NameLook(B)) + (Scale > 1.0f ? " at 1.5" : " at 1");
			}
			for (const auto& Wh : BackSizes)
			{
				const int Back = festart::kBackFestival + static_cast<int>(B);
				const ss::ui::DrawList Bl = OnCanvasLook([&](ss::ui::Canvas& C) { festart::FestCardBack(C, Back, 100.0f, 100.0f, Wh[0], Wh[1]); }, Scale);
				bool BCached = false;
				OnCanvasLook([&](ss::ui::Canvas& C) { BCached = C.IsCached(festart::festart_detail::FestCardBackKey(L, Wh[0], Wh[1])); }, Scale);
				BackCached = BackCached && BCached && Bl.Vertices.size() > 20;
				LookBack = std::max(LookBack, Bl.Vertices.size());
				if (Bl.Vertices.size() > BackMax)
				{
					BackMax = Bl.Vertices.size();
					char Buf[64];
					std::snprintf(Buf, sizeof(Buf), "%s %.0fx%.0f at %.1f", NameLook(B), Wh[0], Wh[1], Scale);
					BackWorst = Buf;
				}
			}
		}
		PerLook += std::string(" ") + NameLook(B) + " " + std::to_string(LookMark) + "/" + std::to_string(LookBack) + ";";
	}
	std::printf("  takeover felt mark / card back, most vertices at either scale:%s\n", PerLook.c_str());
	std::printf("  takeover felt marks: at most %zu vertices (%s); card backs: at most %zu (%s)\n", MarkMax, MarkWorst.c_str(), BackMax, BackWorst.c_str());
	ExpectLook(MarkMax <= 1500 && MarkSame && MarkCached, "FestTableBudgets: every look's felt mark is within 1.5k vertices at both scales, one cached fragment replayed the same (worst " + MarkWorst + ")");
	ExpectLook(MarkGone, "FestTableBudgets: a Suppressed felt mark draws nothing");
	ExpectLook(BackMax <= 300 && BackCached, "FestTableBudgets: every look's card back is within 300 vertices at the tables' sizes and both scales, cached per look and size (worst " + BackWorst + ")");
	// The REIGN felt mark and card back carry no sponsor mark (no sponsor's name, patch or brand colour).
	const festart::FestivalLook& Reign = festart::LookOf(Brand::Reign);
	const ss::ui::DrawList ReignMark = OnCanvasLook([&](ss::ui::Canvas& C) { festart::FeltMark(C, Reign, 400.0f, 300.0f, 110.0f, false); }, 1.0f);
	const ss::ui::DrawList ReignBack = OnCanvasLook([&](ss::ui::Canvas& C) { festart::FestCardBack(C, festart::kBackFestival + static_cast<int>(Brand::Reign), 100.0f, 100.0f, 72.0f, 100.0f); }, 1.0f);
	const ss::ui::Rect Everywhere{0.0f, 0.0f, 1280.0f, 800.0f};
	int ReignTexts = 0;
	for (const ss::ui::DrawList* L : {&ReignMark, &ReignBack})
	{
		for (const ss::ui::DrawCmd& Cmd : L->Cmds)
		{
			ReignTexts += Cmd.Type == ss::ui::DrawCmd::Kind::Text ? 1 : 0;
		}
	}
	ExpectLook(ReignTexts == 0 && SponsorMarksLook(ReignMark, Everywhere) == 0 && SponsorMarksLook(ReignBack, Everywhere) == 0,
		"FestTableBudgets: the REIGN felt mark and card back draw no sponsor mark (no words, no sponsor's colours)");
	// The three tables (pages fest_table_*) and REIGN's (the dev look on WRIT's): the budget with the mark, and each wears its look.
	std::vector<const TakePageLook*> Tables;
	for (const TakePageLook& P : T.Pages)
	{
		if (P.Name.rfind("fest_table_", 0) == 0)
		{
			Tables.push_back(&P);
		}
	}
	Tables.push_back(&T.ReignTable);
	for (const TakePageLook* Pp : Tables)
	{
		const TakePageLook& P = *Pp;
		const festart::FestivalLook& L = festart::LookOf(P.Look);
		const size_t V1 = P.L1.Vertices.size(), V15 = P.L15.Vertices.size();
		const size_t Cap = P.Host == SiteId::Ignite ? 22000 : P.Host == SiteId::RunItTwice ? 19000 : 0;
		std::printf("  takeover table %-22s %-7s: %zu / %zu vertices at 1 / 1.5 (budget %s at 1, as rivals_ui measures)\n", P.Name.c_str(), NameLook(P.Look), V1, V15,
			Cap ? std::to_string(Cap).c_str() : "none listed");
		ExpectLook(P.Drawn && (Cap == 0 || V1 <= Cap), "FestTableBudgets: the festival table is within its budget with the felt mark (" + P.Name + ")");
		const ss::ui::rlsites_detail::SiteLook& Own = ss::ui::rlsites_detail::SitesLook(P.Host);
		// Its three felts (§2.7.2: FeltLight is the Felt mixed 0.18 toward Accent2); REIGN's are the club's own three, its pool included.
		const bool Felt = P.Look == Brand::Reign ? P.Felt == Own.Felt && P.FeltLight == Own.FeltLight && P.FeltDark == Own.FeltDark
												 : P.Felt == L.Felt && P.FeltLight == MixHexLook(L.Felt, L.Accent2, 0.18f) && P.FeltDark == L.FeltDark;
		ExpectLook(P.Drawn && Felt && P.Back == festart::kBackFestival + static_cast<int>(P.Look) && P.Marked,
			"FestTableBudgets: the festival table wears its look's felt (REIGN: the club's own cloth), its card back and its felt mark (" + P.Name + ", " + NameLook(P.Look) + ")");
	}
	// REIGN's table: the rail's hairline in Rose Brass (alpha 0.85, the octagon's edges; WRIT's table has none) and no sponsor mark on its
	// felt (FestReignCopy's F-look half).
	const auto RailInk = [](const ss::ui::DrawList& L) {
		int N = 0;
		for (const ss::ui::Vertex& V : L.Vertices)
		{
			N += RgbLook(V.Col) == 0xe8b4a0u && std::fabs(V.Col.A - 0.85f) < 0.01f ? 1 : 0;
		}
		return N;
	};
	const TakePageLook* Writ = Tables.size() >= 2 ? Tables[Tables.size() - 2] : nullptr;
	const int ReignRail = RailInk(T.ReignTable.L1);
	const int WritRail = Writ ? RailInk(Writ->L1) : -1;
	const ss::ui::Rect FeltR{170.0f, 210.0f, 840.0f, 440.0f};
	std::printf("  takeover REIGN table: %d Rose Brass rail vertices (WRIT's %d); sponsor marks on the felt %d\n", ReignRail, WritRail, SponsorMarksLook(T.ReignTable.L1, FeltR));
	ExpectLook(T.ReignTable.Drawn && ReignRail >= 16 && WritRail == 0, "FestTableBudgets: a REIGN table's rail is Rose Brass on the club's own cloth, a WRIT table's is not");
	ExpectLook(T.ReignTable.Drawn && SponsorMarksLook(T.ReignTable.L1, FeltR) == 0, "FestTableBudgets: no sponsor mark on the REIGN felt");
}

/** Each look of a host through the dev look (ss.Fest.Look) on the page Lap shows, at both scales; the dev look off after. */
void DevPagesLook(TakeoverLook& T, LaptopLook& Lap, SiteId Host)
{
	for (const Brand B : AllLook)
	{
		if (HostLook(B) != Host)
		{
			continue;
		}
		festart::DevLook(B);
		LookDrawLook D;
		D.B = B;
		D.Host = Host;
		D.L1 = Lap.Settled(1.0f);
		D.L15 = Lap.Frame(1.5f);
		T.Looks.push_back(std::move(D));
	}
	festart::DevLook(Brand::None);
}

/** The vertices that show, and the text runs, wearing one of Rgbs. */
int CountRgbLook(const ss::ui::DrawList& L, const std::set<uint32_t>& Rgbs)
{
	int N = 0;
	for (const ss::ui::Vertex& V : L.Vertices)
	{
		N += V.Col.A > 0.05f && Rgbs.count(RgbLook(V.Col)) ? 1 : 0;
	}
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		N += Cmd.Type == ss::ui::DrawCmd::Kind::Text && Rgbs.count(RgbLook(Cmd.Text.Col)) ? 1 : 0;
	}
	return N;
}

/** Whether a text run wears Rgb (a page's own tokens; the crests draw no text). */
bool TextWearsLook(const ss::ui::DrawList& L, uint32_t Rgb)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Col.A > 0.5f && RgbLook(Cmd.Text.Col) == Rgb)
		{
			return true;
		}
	}
	return false;
}

/** TextWearsLook for the page's skin: the attraction's foot sign ("STEP INSIDE · THE HUB", §2.11 U32) left out. The attraction stands
 *  in its look's colours every hour its festival is Live (§2.7.4), Afterburner Hours included; only the palette yields to the night
 *  (§2.7.3). G1 FL-final: with F-hubs' band fill (FH-3b) merged, FEIGN's day 2 at 02:00 shows the arch and its Mirror Mint sign. */
bool SkinTextWearsLook(const ss::ui::DrawList& L, uint32_t Rgb)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Col.A > 0.5f && RgbLook(Cmd.Text.Col) == Rgb && Cmd.Text.Text != festart::festart_detail::FestHubSign())
		{
			return true;
		}
	}
	return false;
}

/**
 * The seven pages (§2.15.2, QF-18: F-look's fest_takeover_* and fest_table_*), each drawn at both scales and emitted, on one session a
 * host (its world starts at the festival's first event: no days to catch up): RiverLine's lobby during RCOP 2026 (its 29.9k + 2k
 * printed; F-hubs asserts it, FH-7) and an RCOP event's table; Ignite's PLAY during FEIGN, at 02:00 the next night (Afterburner wins)
 * and a FEIGN event's table; RunItTwice's THE BOARD during its first edition (WRIT 2027) and a WRIT event's table. On the same
 * sessions: each look's host page through the dev look (FestLeak's and FestContrast's page halves) and REIGN's table (the dev look on
 * WRIT's: its own cloth, the Rose Brass rail, the keyhole mark).
 */
TakeoverLook TakeoverPagesLook()
{
	using Page = ss::ui::RiverLine::Page;
	TakeoverLook T;
	const ss::fest::Festival* Rcop = ss::fest::FestivalOf("rcop");
	const ss::fest::Festival* Feign = FirstOnLook(SiteId::Ignite, Brand::Feign);
	const ss::fest::Festival* Rit = FirstOnLook(SiteId::RunItTwice);
	ExpectLook(Rcop && Feign && Rit, "FestTableBudgets: the calendar has RCOP 2026, FEIGN and a RunItTwice edition");
	if (!Rcop || !Feign || !Rit)
	{
		return T;
	}
	auto T0 = std::chrono::steady_clock::now();
	const auto Clock = [&T0](TakePageLook& P) {
		const auto T1 = std::chrono::steady_clock::now();
		P.Seconds = std::chrono::duration<double>(T1 - T0).count();
		T0 = T1;
	};
	ss::net::EventInstance E;
	double World = 0.0;
	// RiverLine: RCOP 2026's first numbered event in late registration.
	if (FestEventLook(*Rcop, Rcop->FirstDay * 1440.0, E, World))
	{
		LaptopLook Lap("fest-takeover-rl", SiteId::RiverLine, World);
		Lap.Open(Page::Lobby);
		TakePageLook P{"fest_takeover_rl_lobby", SiteId::RiverLine, Brand::Rcop};
		MeasureLook(P, Lap);
		std::printf("  takeover: RiverLine's lobby with festival crests: %zu / %zu vertices at 1 / 1.5 (its measured 29.9k + 2k: F-hubs asserts, FH-7)\n", P.L1.Vertices.size(),
			P.L15.Vertices.size());
		DevPagesLook(T, Lap, SiteId::RiverLine);
		Clock(P);
		T.Pages.push_back(std::move(P));
		TakePageLook Tb{"fest_table_rl", SiteId::RiverLine, Brand::Rcop};
		if (SeatLook(Lap, *Rcop, E))
		{
			TableDrawLook(Tb, Lap);
		}
		Clock(Tb);
		T.Pages.push_back(std::move(Tb));
		// The laptop's ground (RiverLine::Draw's first fill): RCOP's Bg2 behind the client, RiverLine's own behind another app (the
		// Bank): the festival dresses the poker client only; the other apps stay outside the scope.
		const ss::ui::Skin RlDay = ss::ui::Skin::FromHex(ss::net::Def(SiteId::RiverLine).Palette);
		uint32_t DayBg2 = 0, RcopBg2 = 0;
		{
			const ss::ui::SkinScope In(RlDay);
			DayBg2 = RgbLook(ss::ui::pal::Bg2);
		}
		{
			const ss::ui::SkinScope In(festart::Dressed(RlDay, festart::LookOf(Brand::Rcop)));
			RcopBg2 = RgbLook(ss::ui::pal::Bg2);
		}
		Lap.Open(Page::Lobby);
		const uint32_t ClientGround = GroundTopLook(Lap.Settled());
		Lap.RL.OpenApp(ss::ui::RiverLine::App::Bank, Lap.Now);
		const uint32_t OtherGround = GroundTopLook(Lap.Settled());
		std::printf("  takeover ground: %06x behind the client, %06x behind the Bank (RiverLine's Bg2 %06x, RCOP's %06x)\n", ClientGround, OtherGround, DayBg2, RcopBg2);
		ExpectLook(DayBg2 != RcopBg2 && ClientGround == RcopBg2 && OtherGround == DayBg2,
			"FestSkinRule: during RCOP the laptop's ground is RCOP's behind the client and RiverLine's own behind another app (the Bank)");
	}
	// Ignite: FEIGN's first event; Afterburner at 02:00 the next night; the next event's table.
	if (FestEventLook(*Feign, Feign->FirstDay * 1440.0, E, World))
	{
		LaptopLook Lap("fest-takeover-ig", SiteId::Ignite, World);
		Lap.Open(Page::Lobby);
		TakePageLook P{"fest_takeover_ig_play", SiteId::Ignite, Brand::Feign};
		MeasureLook(P, Lap);
		DevPagesLook(T, Lap, SiteId::Ignite);
		Clock(P);
		T.Pages.push_back(std::move(P));
		const double Night = (std::floor(World / 1440.0) + 1.0) * 1440.0 + 120.0;
		Lap.At(Night);
		TakePageLook Ab{"fest_takeover_ig_afterburner", SiteId::Ignite, Brand::None};
		MeasureLook(Ab, Lap);
		Clock(Ab);
		T.Pages.push_back(std::move(Ab));
		TakePageLook Tb{"fest_table_ig", SiteId::Ignite, Brand::Feign};
		if (FestEventLook(*Feign, Night, E, World))
		{
			Lap.At(World);
			if (SeatLook(Lap, *Feign, E))
			{
				TableDrawLook(Tb, Lap);
			}
		}
		Clock(Tb);
		T.Pages.push_back(std::move(Tb));
	}
	// RunItTwice: its first edition's first event (WRIT 2027): THE BOARD, the table, REIGN's table through the dev look.
	if (FestEventLook(*Rit, Rit->FirstDay * 1440.0, E, World))
	{
		LaptopLook Lap("fest-takeover-rit", SiteId::RunItTwice, World);
		Lap.Open(Page::Lobby);
		TakePageLook P{"fest_takeover_rit_board", SiteId::RunItTwice, Rit->Which};
		MeasureLook(P, Lap);
		DevPagesLook(T, Lap, SiteId::RunItTwice);
		Clock(P);
		T.Pages.push_back(std::move(P));
		TakePageLook Tb{"fest_table_rit", SiteId::RunItTwice, Rit->Which};
		T.ReignTable = TakePageLook{"REIGN (dev look)", SiteId::RunItTwice, Brand::Reign};
		if (SeatLook(Lap, *Rit, E))
		{
			TableDrawLook(Tb, Lap);
			festart::DevLook(Brand::Reign);
			TableDrawLook(T.ReignTable, Lap, false);
			festart::DevLook(Brand::None);
		}
		Clock(Tb);
		T.Pages.push_back(std::move(Tb));
	}
	ExpectLook(T.Pages.size() == 7, "FestTableBudgets: the seven takeover pages were drawn");
	// Each takeover page wears its look in its own text (accent and label tokens; the crests draw none): RunItTwice's in Accent2 (its
	// Accent stays Signal Red); the afterburner page wears Afterburner's accent and none of FEIGN's.
	const uint32_t NightAccent = ss::net::Def(SiteId::Ignite).NightPalette[8];
	for (const TakePageLook& P : T.Pages)
	{
		std::printf("  takeover time %-28s %.2f s\n", P.Name.c_str(), P.Seconds);
		if (P.Name.rfind("fest_takeover_", 0) != 0)
		{
			continue;
		}
		const festart::FestivalLook& F = festart::LookOf(P.Look == Brand::None ? Brand::Feign : P.Look);
		// Afterburner's page: the night's accent on its text and none of FEIGN's skin (the arch's own sign wears FEIGN's look while Live).
		bool Worn = TextWearsLook(P.L1, NightAccent) && !SkinTextWearsLook(P.L1, F.Accent) && !SkinTextWearsLook(P.L1, F.Accent2) && !SkinTextWearsLook(P.L1, F.Gold);
		if (P.Look != Brand::None)
		{
			// The look's swapped tokens (those it changes on its host: REIGN's Accent2 alone).
			const std::array<uint32_t, 16>& Own = ss::net::Def(P.Host).Palette;
			std::set<uint32_t> Mark;
			for (const uint32_t C : {F.Accent, F.Accent2, F.Gold, F.Felt, F.FeltDark, F.Bg2})
			{
				if (std::find(Own.begin(), Own.end(), C) == Own.end())
				{
					Mark.insert(C);
				}
			}
			int Other = -1;
			for (const LookDrawLook& D : T.Looks)
			{
				if (Other < 0 && D.Host == P.Host && D.B != P.Look)
				{
					Other = CountRgbLook(D.L1, Mark);
				}
			}
			const int Mine = CountRgbLook(P.L1, Mark);
			std::printf("  takeover page %s: %d vertices and runs in its look's %zu swapped tokens (%d in another look's)\n", P.Name.c_str(), Mine, Mark.size(), Other);
			Worn = Other >= 0 && Mine > Other;
		}
		ExpectLook(P.Drawn && Worn, "FestSkinRule: the page wears its festival's look (" + P.Name + (P.Look == Brand::None ? ": Afterburner's night, not FEIGN" : std::string(": ") + NameLook(P.Look)) + ")");
	}
	return T;
}

/**
 * FestLeak's and FestContrast's page halves (§2.15.2): SitesNoLeak's rule over every Ignite and RunItTwice festival page (the calendar's
 * pages and tables, and each host's page in each of its looks through the dev look) at both scales, and on every look's host page at
 * both scales the drawn text holds: accent runs on Bg >= 4.5 on RiverLine and Ignite looks, Ink runs on Panel and Muted runs on Bg
 * >= 4.5; a RunItTwice look changes no run to Signal Red (its own red runs are the host's light, the same in every look). Planted bugs:
 * as FL-1's (FEIGN's Bg2 0e1726; Ring Rush's accent c84a5e).
 */
void FestPagesLeakContrast(const TakeoverLook& T)
{
	int Checked = 0, Leaky = 0;
	std::string Leaks;
	const auto Leak = [&](const ss::ui::DrawList& L, SiteId Host, const std::string& What, bool Table) {
		std::string Seen;
		const ss::ui::rlsites_detail::SiteLook& K = ss::ui::rlsites_detail::SitesLook(Host);
		const int N = Table ? LeaksLook(L, Host, Seen, {K.Cx - K.Rx, K.Cy - K.Ry, 2.0f * K.Rx, 2.0f * K.Ry}) : LeaksLook(L, Host, Seen);
		++Checked;
		if (N > 0)
		{
			++Leaky;
			Leaks += " " + What + ":" + Seen;
		}
	};
	for (const TakePageLook& P : T.Pages)
	{
		if (P.Host != SiteId::RiverLine && P.Drawn)
		{
			const bool Table = P.Name.rfind("fest_table_", 0) == 0;
			Leak(P.L1, P.Host, P.Name, Table);
			Leak(P.L15, P.Host, P.Name + "@1.5", Table);
		}
	}
	for (const Brand B : AllLook)
	{
		if (HostLook(B) == SiteId::RiverLine)
		{
			continue;
		}
		const festart::FestivalLook& L = festart::LookOf(B);
		for (const float Scale : {1.0f, 1.5f})
		{
			Leak(OnCanvasLook([&](ss::ui::Canvas& C) { festart::FeltMark(C, L, 600.0f, 400.0f, MarkSizeLook(HostLook(B)), false); }, Scale), HostLook(B),
				std::string("mark/") + NameLook(B), false);
			Leak(OnCanvasLook([&](ss::ui::Canvas& C) { festart::FestCardBack(C, festart::kBackFestival + static_cast<int>(B), 400.0f, 300.0f, 84.0f, 118.0f); }, Scale), HostLook(B),
				std::string("back/") + NameLook(B), false);
		}
	}
	int Accents = 0, Inks = 0, Low = 0, Looks = 0;
	std::string LowWhat;
	std::map<Brand, std::multiset<std::string>> Red;
	for (const LookDrawLook& D : T.Looks)
	{
		++Looks;
		const std::array<uint32_t, 16> X = HexesLook(DressedLook(D.B));
		const uint32_t Bg = X[0], Panel = X[2], Ink = X[5], Muted = X[6], Accent = X[8];
		for (const ss::ui::DrawList* L : {&D.L1, &D.L15})
		{
			if (D.Host != SiteId::RiverLine)
			{
				Leak(*L, D.Host, std::string("page/") + NameLook(D.B), false);
			}
			for (const ss::ui::DrawCmd& Cmd : L->Cmds)
			{
				if (Cmd.Type != ss::ui::DrawCmd::Kind::Text || Cmd.Text.Col.A < 0.9f || Cmd.Text.BaselineY > ss::ui::RiverLine::Height - 38.0f)
				{
					continue;
				}
				const uint32_t C = RgbLook(Cmd.Text.Col);
				double Ratio = 99.0;
				if (C == Accent && D.Host != SiteId::RunItTwice)
				{
					++Accents;
					Ratio = ContrastLook(Accent, Bg);
				}
				else if (C == Ink)
				{
					++Inks;
					Ratio = ContrastLook(Ink, Panel);
				}
				else if (C == Muted)
				{
					++Inks;
					Ratio = ContrastLook(Muted, Bg);
				}
				if (D.Host == SiteId::RunItTwice && C == 0xd2141au && L == &D.L1)
				{
					Red[D.B].insert(Cmd.Text.Text);
				}
				if (Ratio < 4.5)
				{
					++Low;
					LowWhat = LowWhat.empty() ? std::string(" (first: ") + NameLook(D.B) + " \"" + Cmd.Text.Text + "\")" : LowWhat;
				}
			}
		}
	}
	bool RedSame = Red.size() == 3;
	for (const auto& R : Red)
	{
		RedSame = RedSame && R.second == Red.begin()->second;
	}
	std::string RedWhat;
	for (const std::string& S : std::set<std::string>(Red.empty() ? std::set<std::string>() : std::set<std::string>(Red.begin()->second.begin(), Red.begin()->second.end())))
	{
		RedWhat += (RedWhat.empty() ? "" : ", ") + ("\"" + S + "\"");
	}
	std::printf("  takeover pages: leak-checked %d draws (%d leaky%s); %d looks' pages: contrast over %d accent runs and %d Ink or Muted runs (%d under 4.5); RunItTwice's "
				"red runs, the same in its three looks: %s\n",
		Checked, Leaky, Leaks.c_str(), Looks, Accents, Inks, Low, RedWhat.c_str());
	ExpectLook(Checked >= 40 && Leaky == 0, "FestLeak: no RiverLine palette colour on any Ignite or RunItTwice festival page (a table's felt), felt mark or card back, in any of the six rival looks" + Leaks);
	ExpectLook(Looks == 10 && Accents > 0 && Inks > 0 && Low == 0, "FestContrast: on every look's pages at both scales, accent text on Bg, Ink on Panel and Muted on Bg read at 4.5:1 or more" + LowWhat);
	ExpectLook(RedSame, "FestContrast: a RunItTwice look sets no run in Signal Red its other looks don't (red is light, never ink)");
}

/** Every bulbs row Ignite's band can cache for attraction B (IgniteAttrBulbsKey over 30 beats, a 32nd of a beat apart: which steps are
 *  lit depends on the beat alone, and 30 beats hold every step of the three chases, 30, 9 and 27 long). */
std::set<uint64_t> AttrRowsLook(Brand B)
{
	std::set<uint64_t> Keys;
	for (int Step = 0; Step < 32 * 30; ++Step)
	{
		Keys.insert(ia::IgniteAttrBulbsKey(BandLook(B, 1, 10, ss::site::show::Mood::PrimeTime, static_cast<double>(Step) / 32.0 * 60.0 / 124.0, false)));
	}
	Keys.erase(0);
	return Keys;
}

/** Empties the Boardwalk pool (least recently used first) and puts its 120k back, so a drawing's rows are the only ones cached. */
void EmptyBoardwalkLook()
{
	ss::ui::Canvas::SetPoolBudget(ss::ui::Canvas::CachePool::Boardwalk, 1);
	ss::ui::Canvas::SetPoolBudget(ss::ui::Canvas::CachePool::Boardwalk, 0);
}

/** Whether a drawing (made just after EmptyBoardwalkLook) put attraction B on the page: one of its bulbs rows is cached, or (the arch
 *  and the pavilion) its foot sign is lettered on it, so an uncached row can't hide the attraction from the budget. */
bool AttrOnPageLook(Brand B, const std::set<uint64_t>& Rows, const ss::ui::DrawList& L, ui_test::TableMeasurer& M)
{
	ss::ui::DrawList Ask;
	ss::ui::Canvas C(Ask, M, 1600.0f, 1000.0f, 1.0f);
	for (const uint64_t K : Rows)
	{
		if (C.IsCached(K))
		{
			return true;
		}
	}
	if (B != Brand::Push)
	{
		for (const ss::ui::TextItem& T : RunsLook(L))
		{
			if (T.Text == festart::festart_detail::FestHubSign())
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * FestBandBudget's PLAY half (§2.15.2's row: "Ignite's PLAY ≤ 34k / 400 runs with the arch, with the tower and with the pavilion";
 * §2.8.10 / §8.5; G1 FL-final, PLAN_G1 item 17): Ignite's PLAY (the lobby, scale 1, settled, as tests/ui_seg_ignite.cpp's bench draws
 * it) during each attraction's first edition on Ignite (FEIGN's arch, PUSH's tower, PENNANT's pavilion), on one laptop that visits
 * the three in date order (its world starts at the first; each later edition is a day's catching up, not a new world's): on day 1 at
 * 12:00, day 2 at 02:00 (Afterburner) and every day at 19:30, 20:30 and 21:00 (the evening's busiest, the bench's hour). Wherever the
 * attraction is on the page (AttrOnPageLook: F-hubs' band fill, FH-3b, in the tree) PLAY is at most 34,000 vertices and 400 text
 * runs, and the attraction is on the page at every moment. A tree without the fill (F-look's branch alone) has no attraction on PLAY:
 * the counts of PLAY today are printed, and after FEIGN's edition the laptop stops (PUSH's and PENNANT's would cost the same nothing).
 * Once an edition has shown its attraction (the fill is in the tree), a later edition's attraction is owed at every moment too.
 * The probe itself is checked in every tree on the band drawn directly. Printed: each edition's busiest counts and their moments.
 * Planted bugs (run where the fill is merged): the arch's candy stripe in 0.6-unit dashes (inside the band's 15k, over PLAY's 34k);
 * the pavilion's pennant lines ten times as dense; the arch's foot sign lettered a run a glyph at each of its bulbs (the runs).
 */
void FestBandPlayLook()
{
	using Page = ss::ui::RiverLine::Page;
	const auto T0 = std::chrono::steady_clock::now();
	const char* What[3] = {"FEIGN's arch", "PUSH's tower", "PENNANT's pavilion"};
	std::set<uint64_t> Rows[3];
	// The probe on the band drawn directly: with the attraction one of its rows is cached, with today's band none.
	for (int Ai = 0; Ai < 3; ++Ai)
	{
		const Brand B = AttractionsLook[Ai];
		Rows[Ai] = AttrRowsLook(B);
		ui_test::TableMeasurer M;
		EmptyBoardwalkLook();
		const bool With = AttrOnPageLook(B, Rows[Ai], DrawBandLook(BandLook(B, 2, 10, ss::site::show::Mood::PrimeTime, 977.3, false), 1.0f, true, true), M);
		EmptyBoardwalkLook();
		const ss::ui::DrawList Today = DrawBandLook(BandLook(Brand::None, 0, 0, ss::site::show::Mood::PrimeTime, 977.3, false), 1.0f, true, true);
		bool Without = false;
		{
			ss::ui::DrawList Ask;
			ss::ui::Canvas C(Ask, M, 1600.0f, 1000.0f, 1.0f);
			for (const uint64_t K : Rows[Ai])
			{
				Without = Without || C.IsCached(K);
			}
		}
		ExpectLook(With && !Without && Rows[Ai].size() >= 2,
			std::string("FestBandBudget: PLAY's probe finds ") + What[Ai] + " when the band draws it (a bulbs row cached), and no row with today's band");
	}
	// The three editions in date order.
	std::vector<std::pair<int, int>> Order;
	for (int Ai = 0; Ai < 3; ++Ai)
	{
		const ss::fest::Festival* F = FirstOnLook(SiteId::Ignite, AttractionsLook[Ai]);
		ExpectLook(F != nullptr, std::string("FestBandBudget: the calendar has a ") + NameLook(AttractionsLook[Ai]) + " edition on Ignite (PLAY with " + What[Ai] + ")");
		if (F)
		{
			Order.push_back({F->FirstDay, Ai});
		}
	}
	std::sort(Order.begin(), Order.end());
	std::unique_ptr<LaptopLook> Lap;
	bool Fill = true;
	bool FillSeen = false; // an earlier edition had its attraction on PLAY
	for (const std::pair<int, int>& O : Order)
	{
		const int Ai = O.second;
		const Brand B = AttractionsLook[Ai];
		const ss::fest::Festival& F = *FirstOnLook(SiteId::Ignite, B);
		const int Days = F.LastDay - F.FirstDay + 1;
		if (!Fill)
		{
			std::printf("  band: PLAY with %s (%s): not measured, no band fill in this tree (FEIGN's arch was on no page)\n", What[Ai], F.Name.c_str());
			continue;
		}
		// Day D's moments, in the order of the clock: 02:00 (day 2), 12:00 (day 1), 19:30, 20:30, 21:00.
		std::vector<double> At;
		for (int D = 1; D <= Days; ++D)
		{
			const double Day = static_cast<double>(F.FirstDay + D - 1) * 1440.0;
			if (D == 2)
			{
				At.push_back(Day + 120.0);
			}
			if (D == 1)
			{
				At.push_back(Day + 720.0);
			}
			for (const double Min : {1170.0, 1230.0, 1260.0})
			{
				At.push_back(Day + Min);
			}
		}
		if (!Lap)
		{
			Lap.reset(new LaptopLook("fest-band-play", SiteId::Ignite, At.front()));
		}
		size_t MostV = 0;
		size_t MostR = 0;
		double MostVAt = 0.0;
		double MostRAt = 0.0;
		int On = 0;
		bool Pages = true;
		for (const double W : At)
		{
			Lap->At(W);
			Lap->Open(Page::Lobby);
			EmptyBoardwalkLook();
			const ss::ui::DrawList Play = Lap->Settled();
			On += AttrOnPageLook(B, Rows[Ai], Play, Lap->M) ? 1 : 0;
			const size_t V = Play.Vertices.size();
			const size_t R = RunsLook(Play).size();
			Pages = Pages && V > 8000 && R > 0;
			if (V > MostV)
			{
				MostV = V;
				MostVAt = W;
			}
			if (R > MostR)
			{
				MostR = R;
				MostRAt = W;
			}
		}
		const auto When = [&F](double W) {
			const int Min = static_cast<int>(W) % 1440;
			char Buf[48];
			std::snprintf(Buf, sizeof(Buf), "day %d %02d:%02d", static_cast<int>(W / 1440.0) - F.FirstDay + 1, Min / 60, Min % 60);
			return std::string(Buf);
		};
		const int N = static_cast<int>(At.size());
		ExpectLook(Pages, std::string("FestBandBudget: Ignite's PLAY draws at every moment of ") + F.Name);
		if (On == 0 && !FillSeen)
		{
			// No band fill in this tree (F-look's branch alone): PLAY today, printed.
			std::printf("  band: PLAY during %s (%d days, %d moments): %s on no page (no band fill in this tree: F-hubs' FH-3b fills IgniteBand); PLAY today "
						"at its busiest %zu vertices (%s), %zu runs (%s), printed only\n",
				F.Name.c_str(), Days, N, What[Ai], MostV, When(MostVAt).c_str(), MostR, When(MostRAt).c_str());
			Fill = false;
			continue;
		}
		FillSeen = true;
		std::printf("  band: PLAY with %s during %s (%d days, %d moments, on the page at %d): busiest %zu vertices (%s), %zu runs (%s); budget 34,000 and 400\n",
			What[Ai], F.Name.c_str(), Days, N, On, MostV, When(MostVAt).c_str(), MostR, When(MostRAt).c_str());
		ExpectLook(On == N, std::string("FestBandBudget: ") + What[Ai] + " is on PLAY at every moment of " + F.Name + " (its days are Live)");
		ExpectLook(MostV <= 34000 && MostR <= 400,
			std::string("FestBandBudget: Ignite's PLAY with ") + What[Ai] + " is at most 34k vertices and 400 text runs (" + F.Name + ", busiest of " + std::to_string(N) + " moments)");
	}
	std::printf("  band: PLAY's half took %.2f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace

namespace u4_ui_test
{
void SegLook()
{
	FestLookDistance();
	FestContrast();
	FestLeak();
	FestCrestLowDetail();
	FestTrophyCached();
	FestBandBudget();
	FestBandstand();
	FestCopyUiLook();
	FestSkinRule();
	const TakeoverLook Takeover = TakeoverPagesLook();
	FestTableBudgets(Takeover);
	FestPagesLeakContrast(Takeover);
	FestBandPlayLook();
}
} // namespace u4_ui_test
