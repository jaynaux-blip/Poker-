#pragma once

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/WorldFest.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/Ui.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
/**
 * Update 4, Festival Season's look (SPEC v1.7 §2.7; Phase A's declarations, §6 PA-44): each festival's dress, the palette swap, the
 * trophies, belts and pins, the word-mark and the felt mark. Data in net::SeriesInfo, looks here (gaps C12); keyed by fest::Brand and
 * never saved. Bodies in UI/FestivalArt.cpp (F-look's at G1). Frozen after Phase A (append-only, by patch to the lead).
 *
 * Phase A: stubs. Every brand's look is empty, LookFor finds none, Dressed returns its base, and the art draws nothing; nothing calls
 * them before G1.
 */
namespace festart
{
/** A festival's card backs are kBackFestival + its brand (CardOpts::Back, SiteLook::CardBack): above Ignite's worn looks
 *  (SitesIgBackLooks 64 + a look index), so no back today's tables draw collides with one. TblBack's festival branch is F-look's (G1). */
constexpr int kBackFestival = 256;

/** The crest's rim: ByStakes is today's (AlloyFor); Neon (v1.6) is PENNANT's. */
enum class Rim : uint8_t
{
	ByStakes,
	Chrome,
	Brass,
	GoldLeaf,
	CandyStripe,
	Neon,
};
/** The look's motion, each a closed form of Now - At and a still while previewing (§2.7.7); v1.6 appends the last two. */
enum class Motion : uint8_t
{
	None,
	RingTurn,
	Clasp,
	RaysTurn,
	Flap,
	Bob,
	Gauge,
	Curtains,
	WaveRise,
	PavilionChase,
	KeyTurn,
};

/** One festival's dress (§2.7.1, §2.7.2's table). */
struct FestivalLook
{
	fest::Brand Which = fest::Brand::None;
	uint32_t Accent = 0, Accent2 = 0, Gold = 0, Felt = 0, FeltDark = 0, Bg2 = 0; // the six swapped tokens (§2.7.2)
	uint32_t Kit = 0;      // an art-only colour (Seal Silver, Gold Leaf): crests and trophies, never a pal:: token
	int Frame = 0;         // eventart's private Frame enum, appended: Arch, Tower, Octagon, Proscenium, Pavilion, Keyhole (§2.7.4)
	eventart::Glyph Glyph = eventart::Glyph::Star;
	Rim RimKind = Rim::ByStakes;
	int8_t WordFace = -1;  // ui::Font (BlackItalic, BoldCondensed) or -1: the site's title face
	bool Caps = true, Twice = false; // Twice: RunItTwice's twice print
	float Tracking = 0.0f;
	Motion Move = Motion::None;
	SoundId Sting = SoundId::Click; // FestRiver / FestIgnite / FestRit (§2.7.6)
	const char* Trophy = "";        // "The RCOP Cup", ...
	uint64_t Key = 0;               // Fnv1a("fest:" + brand key): mixed into every cache key with Skin::Current().Key()
};
/** A static table of ten (None: an empty look). Phase A: every brand's look is empty. */
SHORTSTACKCORE_API const FestivalLook& LookOf(fest::Brand B);
/** The look of a festival series (nullptr for a plain series). Phase A: nullptr. */
SHORTSTACKCORE_API const FestivalLook* LookFor(const net::SeriesInfo& S);
/** Base with the six tokens replaced (§2.7.3: SitesSkin's festival branch). Phase A: Base. */
SHORTSTACKCORE_API Skin Dressed(const Skin& Base, const FestivalLook& L);

// The art (§2.7.5, §2.8.7; F-look's bodies, each cached; Lod at 30 units or less draws the silhouette only). Phase A: nothing drawn.
/** Tier 1 (v1.6): a championship's title object; the RCOP Cup engraves one line per edition at its Main field's height (Fields). */
SHORTSTACKCORE_API void FestTrophy(Canvas& C, fest::Brand B, float Cx, float Cy, float Size, double Time, bool Lod, const std::vector<int>& Fields = {}, int Tier = 0);
SHORTSTACKCORE_API void FestBelt(Canvas& C, fest::Brand B, float Cx, float Cy, float Size, double Time, bool Lod);
SHORTSTACKCORE_API void FestPin(Canvas& C, fest::Brand B, int Grade, world::FestMark Mark, float Cx, float Cy, float Size);
SHORTSTACKCORE_API void FestWordMark(Canvas& C, const FestivalLook& L, const std::string& Text, float X, float Y, float Size, double Time, bool Still);
/** The low-detail crest at alpha 0.14 on the oval; Suppressed (G3's B-ui) while INTRODUCTIONS or a lower third draws on RunItTwice. */
SHORTSTACKCORE_API void FeltMark(Canvas& C, const FestivalLook& L, float Cx, float Cy, float Size, bool Suppressed);

// G1's appends (the lead's pre-wave commit H0; LEAD_CALLS R-2; PLAN_G1 §4.2), each a stub in UI/FestivalArt.cpp that draws or keeps
// nothing until F-look's bodies (FestCardBack and DevLook at FL-4, FestBanner and the FestWordMark overload at FL-7). Crest motions
// go through eventart::SeriesCrest's Time (a still is its settled Time), so there is no FestCrestMotion.
/** A festival's card back, Back = kBackFestival + its brand (§2.7.4: the host's back pattern in the look's Accent and Accent2, the
 *  low-detail crest at its centre), in {X, Y, W, H}; TblBack's festival branch (UI/Ui.cpp) calls it. */
SHORTSTACKCORE_API void FestCardBack(Canvas& C, int Back, float X, float Y, float W, float H);
/** ss.Fest.Look <brand or off> (§2.16; presentation only, never saved, logged as dev by the game module): the look SitesSkin and
 *  SitesTableLook dress the site in front in, ahead of the calendar; Brand::None turns it off. */
SHORTSTACKCORE_API void DevLook(fest::Brand B);
/** The dev look DevLook(B) set (Brand::None: off), read first by SitesSkin and SitesTableLook. */
SHORTSTACKCORE_API fest::Brand DevLook();
/** The hub's banner in R (§2.8.2): the Bg2 -> Felt gradient, the look's motif and its banner-level motions (RaysTurn, Curtains,
 *  WaveRise, the door-light on Main day; §2.7.2, §2.7.7). SinceOpen: real seconds since the hub opened; DayProgress: the world day's
 *  fraction (changed once a world minute); Still: the settled frame (previews). */
SHORTSTACKCORE_API void FestBanner(Canvas& C, const FestivalLook& L, const Rect& R, double SinceOpen, float DayProgress, bool MainDay, bool Still);
/** FestWordMark with the day's progress (Gauge fills to it); Time is seconds since the hub opened (Flap, Bob). */
SHORTSTACKCORE_API void FestWordMark(Canvas& C, const FestivalLook& L, const std::string& Text, float X, float Y, float Size, double Time, float DayProgress, bool Still);
} // namespace festart
} // namespace ui
} // namespace ss
