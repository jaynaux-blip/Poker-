// u4_ui: the festival looks (update 4 SPEC v1.7 §2.15.2; F-look's file): the ten looks, the takeover, felts, backs, crests,
// trophies, belts and pins. Called from u4_ui_main.cpp (the kit).
// Phase A (update 4 SPEC v1.7 §6 PA-48): registered, empty.
// G1 FL-1 (F-look): the palette rules on the ten looks' tokens (§2.7.2, §2.7.3), the C++ twins of
// durable/update4/model/fest_brands/palette_check.py: FestLookDistance (R-12), FestContrast's token half and FestLeak's table half
// (FL-4 adds both checks' page halves). No page is drawn or emitted here yet. Helpers sit in an anonymous namespace with a Look
// suffix (the u4_ui binary links F-hubs' files beside this one).
#include "UiSeg.h"
#include "ShortStack/Common.h"
#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/UI/FestivalArt.h"
#include "ShortStack/UI/Ui.h"
#include "../../Source/ShortStackCore/Private/ShortStack/UI/RiverLineShared.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
} // namespace

namespace u4_ui_test
{
void SegLook()
{
	FestLookDistance();
	FestContrast();
	FestLeak();
}
} // namespace u4_ui_test
