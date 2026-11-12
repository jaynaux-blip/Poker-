// Update 4, Festival Season's look (SPEC v1.7 §2.7): the ten looks, the palette swap, the trophies, belts, pins, word-marks and felt
// marks (UH/FestivalArt.h), and the art's strings (FestArtInventory). F-look's file from G1.
//
// Phase A (§6 PA-44; the lead): stubs that keep today's behaviour. Every look is empty, LookFor finds none, Dressed returns its base
// and the art draws nothing; nothing calls them before G1.
// G1 FL-1 (F-look): the ten looks (§2.7.1, §2.7.2's table), LookFor (PLAN_FLOOK QF-3: the brand from the series id's slot, through
// EventArt's own SlotOf) and Dressed (§2.7.3's six tokens). Data only: nothing outside tests reads a look until FL-2 (crests) and FL-4
// (the takeover), so no page moves. The palette rules are tests/u4_ui_look.cpp's FestLookDistance, FestContrast and FestLeak, twins
// of durable/update4/model/fest_brands/palette_check.py.
#include "ShortStack/UI/FestivalArt.h"

#include "ShortStack/Common.h"

#include <array>

namespace ss
{
namespace ui
{
namespace eventart
{
namespace eventart_detail
{
// UI/EventArt.cpp's (QF-3: one slot rule for crests and looks; FL-2 strips the "<site key>." prefix there, §2.7.4).
std::string SlotOf(const std::string& SeriesId);
} // namespace eventart_detail
} // namespace eventart

namespace festart
{
namespace festart_detail
{
// eventart's private Frame enum (UI/EventArt.cpp:1610-1628) by value, as FestivalLook::Frame holds it (§2.7.1): the three existing
// frames the RiverLine looks keep and Phase A's six appends. FL-2 reads Frame back in UI/EventArt.cpp (and asserts these values there).
constexpr int FestFrameShield = 0, FestFrameSunburst = 2, FestFrameMedallion = 3;
constexpr int FestFrameArch = 8, FestFrameTower = 9, FestFrameOctagon = 10, FestFrameProscenium = 11, FestFramePavilion = 12, FestFrameKeyhole = 13;
// "Tracked wide" (WRIT, REIGN): the gap between letters as a share of the size (RunItTwice's kickers: 1.5 at 13, RitSpaced).
constexpr float FestTrackWide = 0.12f;

/** The six swapped tokens and the art-only Kit colour of one row (§2.7.2), as 0xRRGGBB. */
struct FestLookInks
{
	uint32_t Accent, Accent2, Gold, Felt, FeltDark, Bg2, Kit;
};

/** One row of §2.7.2's table; Key = Fnv1a("fest:" + the brand key) (§2.7.1). Face: a ui::Font, or -1 for the site's title face. */
FestivalLook FestLookRow(fest::Brand B, const FestLookInks& I, int Frame, eventart::Glyph G, Rim R, int Face, bool Caps, bool Twice, float Tracking, Motion M,
	SoundId Sting, const char* Trophy)
{
	FestivalLook L;
	L.Which = B;
	L.Accent = I.Accent;
	L.Accent2 = I.Accent2;
	L.Gold = I.Gold;
	L.Felt = I.Felt;
	L.FeltDark = I.FeltDark;
	L.Bg2 = I.Bg2;
	L.Kit = I.Kit;
	L.Frame = Frame;
	L.Glyph = G;
	L.RimKind = R;
	L.WordFace = static_cast<int8_t>(Face);
	L.Caps = Caps;
	L.Twice = Twice;
	L.Tracking = Tracking;
	L.Move = M;
	L.Sting = Sting;
	L.Trophy = Trophy;
	L.Key = Fnv1a(std::string("fest:") + fest::BrandKey(B));
	return L;
}

/**
 * §2.7.2's ten rows, indexed by fest::Brand (None: an empty look). RiverLine's four wear the site's title face as written (no caps)
 * and its sting; Ignite's three BlackItalic and FestIgnite; RunItTwice's three BoldCondensed, twice-printed, with FestRit, keeping
 * Signal Red as Accent and Hot White as Gold (§2.7.3: red is light, never ink); REIGN keeps the club's own black cloth and Bg2 and
 * swaps Accent2 only. Trophy: §2.11's P1-P12, tier 0 (FL-3 or FL-4 checks them against fest::TrophyName once F-net's body is in, R-3).
 */
std::array<FestivalLook, static_cast<size_t>(fest::Brand::Count)> FestLookTable()
{
	using fest::Brand;
	using eventart::Glyph;
	const int FaceSite = -1;
	const int FaceBlackItalic = static_cast<int>(Font::BlackItalic);
	const int FaceBoldCondensed = static_cast<int>(Font::BoldCondensed);
	std::array<FestivalLook, static_cast<size_t>(Brand::Count)> T{};
	const auto Put = [&T](const FestivalLook& L) { T[static_cast<size_t>(L.Which)] = L; };
	// RiverLine: RCOP keeps its Shield, laurel, crown and trophy glyph and gains the silver wave band (a crest layer, FL-2).
	Put(FestLookRow(Brand::Rcop, {0xa78bfa, 0xc9d3e6, 0xf2c14e, 0x1c1550, 0x0e0a30, 0x120f2a, 0}, FestFrameShield, Glyph::Trophy, Rim::ByStakes, FaceSite, false, false, 0.0f,
		Motion::WaveRise, SoundId::FestRiver, "The RCOP Cup"));
	Put(FestLookRow(Brand::RingRush, {0xe0566e, 0xd9a441, 0xf2c14e, 0x4a0f1d, 0x2a0811, 0x1a0d14, 0}, FestFrameMedallion, Glyph::Ring, Rim::ByStakes, FaceSite, false, false,
		0.0f, Motion::RingTurn, SoundId::FestRiver, "the ring"));
	Put(FestLookRow(Brand::Tco, {0x7b96ff, 0xcfd6e3, 0xf0c75e, 0x13286b, 0x0a1640, 0x0c1430, 0}, FestFrameMedallion, Glyph::Bracelet, Rim::ByStakes, FaceSite, false, false,
		0.0f, Motion::Clasp, SoundId::FestRiver, "the bracelet"));
	Put(FestLookRow(Brand::Slam, {0xff8a3d, 0x3cc8ff, 0xffd45c, 0x5a2a14, 0x341607, 0x1c120c, 0}, FestFrameSunburst, Glyph::Sun, Rim::ByStakes, FaceSite, false, false, 0.0f,
		Motion::RaysTurn, SoundId::FestRiver, "The Slam Plate"));
	// Ignite.
	Put(FestLookRow(Brand::Feign, {0xff4fa3, 0x5ff2c9, 0xffe45c, 0x3d1752, 0x220c30, 0x2a1238, 0}, FestFrameArch, Glyph::Mirror, Rim::CandyStripe, FaceBlackItalic, true,
		false, 0.0f, Motion::Bob, SoundId::FestIgnite, "The Funhouse Mirror"));
	Put(FestLookRow(Brand::Push, {0xf2a93b, 0x7fd4ff, 0xffd76a, 0x1f1b4a, 0x110f2a, 0x1d1430, 0}, FestFrameTower, Glyph::PushButton, Rim::Brass, FaceBlackItalic, true, false,
		0.0f, Motion::Gauge, SoundId::FestIgnite, "The Big Red Button"));
	Put(FestLookRow(Brand::Pennant, {0x21d958, 0xff6b57, 0xffe9a8, 0x0d3b45, 0x06222a, 0x101a2e, 0}, FestFramePavilion, Glyph::Pennant, Rim::Neon, FaceBlackItalic, true,
		false, 0.0f, Motion::PavilionChase, SoundId::FestIgnite, "The Gold Pennant"));
	// RunItTwice.
	Put(FestLookRow(Brand::Writ, {0xd2141a, 0xe3e6ec, 0xffffff, 0x23252b, 0x101114, 0x0e0f13, 0xc9ced8}, FestFrameOctagon, Glyph::Seal, Rim::Chrome, FaceBoldCondensed, true,
		true, FestTrackWide, Motion::Flap, SoundId::FestRit, "The Seal"));
	Put(FestLookRow(Brand::Grit, {0xd2141a, 0xf3e9d2, 0xffffff, 0x4a0b12, 0x24060a, 0x140a0c, 0xd9b25f}, FestFrameProscenium, Glyph::TwinStars, Rim::GoldLeaf,
		FaceBoldCondensed, true, true, 0.0f, Motion::Curtains, SoundId::FestRit, "The Grindstone"));
	Put(FestLookRow(Brand::Reign, {0xd2141a, 0xe8b4a0, 0xffffff, 0x1a1a1f, 0x09090b, 0x111114, 0xe8b4a0}, FestFrameKeyhole, Glyph::Sceptre, Rim::Chrome, FaceBoldCondensed,
		true, true, FestTrackWide, Motion::KeyTurn, SoundId::FestRit, "The Sceptre"));
	return T;
}
} // namespace festart_detail
} // namespace festart
} // namespace ui

namespace fest
{
// Step A-2 (LEAD_CALLS Q-5; moved here from G/Festival.cpp by the integrator): the words the festival art prints, empty in Phase A.
// F-look writes the body at G1.
std::vector<std::string> FestArtInventory()
{
	return {};
}
} // namespace fest

namespace ui
{
namespace festart
{
const FestivalLook& LookOf(fest::Brand B)
{
	static const std::array<FestivalLook, static_cast<size_t>(fest::Brand::Count)> FestLooks = festart_detail::FestLookTable();
	static const FestivalLook FestNoLook;
	return B > fest::Brand::None && B < fest::Brand::Count ? FestLooks[static_cast<size_t>(B)] : FestNoLook;
}

const FestivalLook* LookFor(const net::SeriesInfo& S)
{
	// QF-3 (accepted): the brand from the series id's slot ("rcop27" -> "rcop"; "ignite.feign26" -> "feign" once FL-2 strips the site
	// key), so a look needs no F-net data and "SlotOf unstripped" fails FestCrestLowDetail (FL-2). A plain series' slot is no brand key.
	const fest::Brand B = fest::BrandOf(eventart::eventart_detail::SlotOf(S.Id));
	return B == fest::Brand::None ? nullptr : &LookOf(B);
}

Skin Dressed(const Skin& Base, const FestivalLook& L)
{
	if (L.Which == fest::Brand::None)
	{
		return Base; // the empty look dresses nothing
	}
	// §2.7.3: the six tokens in pal:: order; Bg, Panel, Panel2, Line, Ink, Muted, Dim, Red, Orange and Green stay the host's.
	Skin S = Base;
	S.Bg2 = Hex(L.Bg2);
	S.Accent = Hex(L.Accent);
	S.Accent2 = Hex(L.Accent2);
	S.Gold = Hex(L.Gold);
	S.Felt = Hex(L.Felt);
	S.FeltDark = Hex(L.FeltDark);
	return S;
}

void FestTrophy(Canvas& /*C*/, fest::Brand /*B*/, float /*Cx*/, float /*Cy*/, float /*Size*/, double /*Time*/, bool /*Lod*/, const std::vector<int>& /*Fields*/, int /*Tier*/)
{
}

void FestBelt(Canvas& /*C*/, fest::Brand /*B*/, float /*Cx*/, float /*Cy*/, float /*Size*/, double /*Time*/, bool /*Lod*/)
{
}

void FestPin(Canvas& /*C*/, fest::Brand /*B*/, int /*Grade*/, world::FestMark /*Mark*/, float /*Cx*/, float /*Cy*/, float /*Size*/)
{
}

void FestWordMark(Canvas& /*C*/, const FestivalLook& /*L*/, const std::string& /*Text*/, float /*X*/, float /*Y*/, float /*Size*/, double /*Time*/, bool /*Still*/)
{
}

void FeltMark(Canvas& /*C*/, const FestivalLook& /*L*/, float /*Cx*/, float /*Cy*/, float /*Size*/, bool /*Suppressed*/)
{
}

// G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): stubs until F-look's bodies (FestCardBack and DevLook at FL-4,
// FestBanner and the FestWordMark overload at FL-7). No back today's tables draw reaches kBackFestival (the highest is
// SitesIgBackLooks + a Locker look) and nothing calls the others, so they draw nothing and keep nothing.

void FestCardBack(Canvas& /*C*/, int /*Back*/, float /*X*/, float /*Y*/, float /*W*/, float /*H*/)
{
}

void DevLook(fest::Brand /*B*/)
{
}

fest::Brand DevLook()
{
	return fest::Brand::None;
}

void FestBanner(Canvas& /*C*/, const FestivalLook& /*L*/, const Rect& /*R*/, double /*SinceOpen*/, float /*DayProgress*/, bool /*MainDay*/, bool /*Still*/)
{
}

void FestWordMark(Canvas& /*C*/, const FestivalLook& /*L*/, const std::string& /*Text*/, float /*X*/, float /*Y*/, float /*Size*/, double /*Time*/, float /*DayProgress*/, bool /*Still*/)
{
}
} // namespace festart
} // namespace ui
} // namespace ss
