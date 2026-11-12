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
// 34k / 400 is FL-final's, once F-hubs fills the band), FestBandstand's arch half (the arch in the Prize Counter's place only while FEIGN
// is Live, the tower, the pavilion, the foot sign, the puck) and FestCopyUi's art half (FestArtInventory by kind through the copy rules;
// every word the attractions paint is listed). Draws the band straight onto its own canvases, emits nothing.
// G1 FL-5, fix round 1: FestCopyUi's art half screens §1.2 item 5's avoided words after the fill (FestArtInventory's only screen for
// them), and FestBandstand finds no booth back wall under the arch. Test-only.
#include "UiSeg.h"
#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/FestivalArt.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/IgniteArt.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
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
} // namespace festart_detail
} // namespace festart
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
 * Planted bug: SlotOf unstripped ("ignite.feign26" reads "ignite": every rival festival loses its look).
 */
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
 * to the band, the Boardwalk pool's share of the chase rows, and PLAY: Ignite's PLAY at 34k vertices and 400 runs with an attraction
 * needs F-hubs' band fill (FH-3b), so FL-final asserts it.
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
	std::printf("  band: PLAY: needs F-hubs' band fill (FH-3b fills IgniteBand's Fest, FestDay and FestDays); FL-final asserts PLAY <= 34k vertices and 400 runs "
				"with each attraction (the attraction adds the vertices above and at most one text run)\n");
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
}
} // namespace u4_ui_test
