#pragma once

#include "ShortStack/Game/Video.h"
#include "ShortStack/UI/Canvas.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace hero
{
struct Character;
}
namespace ui
{
/**
 * Update 3 (SPEC §6.1, §7.3): Rerun 2.0's thumbnails. Compose draws a ThumbSpec (a recipe core-videocontent generated) into
 * R: the archetype's layout, background, palette, the subject with an expression and a pose, props, the caption in its
 * style, a sticker. Every caption is the facts' own (U7). Lod draws the small-tile version (S-T decides where faces drop).
 * Cached per spec, size and Look in Rerun's cache pool (Canvas::Cached with CachePool::Rerun). Bodies: ui-thumbs
 * (ThumbArt.cpp). Phase A, day 1: a stub that draws nothing.
 */
namespace thumbart
{
SHORTSTACKCORE_API void Compose(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, bool Lod);

// ---------------------------------------------------------------------------------------------------------------- C3
// ui-thumbs' appends (C3; SPEC §6.1, §10, spike S-T in durable/creator/S-T.md). Append-only from here.
//
// THE RECIPE'S VALUES are core-videocontent's vocabulary, video::thumb in Game/VideoContent.h (the generator's header; its
// comment is the contract): the enums below mirror it value for value (ThumbArt.cpp static_asserts every count and the
// values that differ in name: Sticker 7 is the lightning bolt, C9). Expr is ui::ExpressionOf's index (0 Neutral .. 11
// Exhausted); Pose is ui::PortraitPose (0 None .. 4 FistPump; 5-8 are P1 and draw as None). Mirror: bit 0 the layout
// mirrored (never the words), bit 1 the wide zoom (else tight). Variant: the compositor's small choices (0-999).
// Captions: Caption and Caption2's line (<= 14 characters each); the compositor's FACTS ride at the end of Caption2 after
// thumb::FactSep ('\x1f') as "key=value" (c the cards "mine/theirs/board", k the marked board card 0-4, o the other face's
// name, n a big number, f/t Before -> After's two panels, p a progress 0-100, g a graph "u"/"d" + digits, s the site, e the
// event's name, m a multiple, h a chat line (two at most), r a list's rows '|'-separated). With no facts block the facts
// come from the video::Thumb Compose is given (Update 2's Thumb: cards, opponent, site). Out-of-range values draw as the
// field's first value (Arch: Text Slab), never a crash. Without a fact a composition draws backs, nobody, or no number.

/** The 36 archetypes (§6.1's # minus 1). POV (33) is P1: it draws as Text Slab in P0. */
enum class Arch : int16_t
{
	GutPunch,
	CircledRiver,
	TheRead,
	NervesOfSteel,
	Caught,
	TheFold,
	ThePile,
	BigNumber,
	BeforeAfter,
	TrophyShot,
	VsSplit,
	TheBubble,
	Headhunter,
	Envelope,
	TheGraph,
	DayCounter,
	NightShift,
	Venue,
	RoadTrip,
	RentLetter,
	Classroom,
	Quiz,
	TierList,
	TopFive,
	DeskSetup,
	CollabDuo,
	ReactionBox,
	StreamClip,
	Milestone,
	TextSlab,
	TheGhost,
	Payday,
	MonsterHand,
	Pov,
	SiteCard,
	Mistake,
	Count,
};
/**
 * The 18 landscape layouts (§6.1). Each archetype draws its own composition; Layout picks among its variants (the
 * archetype's first allowed layout is its default) and Mirror flips the composition (never the words).
 */
enum class Layout : int16_t
{
	FaceLeftWordsRight,
	CardsLeftFaceRight,
	FaceCentreFanBelow,
	RiverCircled,
	SplitVs,
	BigNumberFace,
	BeforeAfter,
	TableTopDown,
	TrophyHero,
	StreamClip,
	Graph,
	TextSlab,
	Collage,
	TierList,
	Desk,
	Letter,
	Venue,
	Question,
	Count,
};
/** The 8 backgrounds. */
enum class Bg : int16_t
{
	FlatGradient,
	RadialBurst,
	SpeedLines,
	FeltBokeh,
	MoneyRain,
	DiagonalSplit,
	Spotlight,
	Scanlines,
	Count,
};
/** The 11 P0 palettes (+ Brand, P1: drawn in Rerun's marigold on ink until the brand kit lands). Site reads Facts.Site. */
enum class Pal : int16_t
{
	Heat,
	Felt,
	Money,
	Ice,
	Neon,
	GoldOnBlack,
	Sunrise,
	Night,
	Mono,
	Blood,
	Site,
	Brand,
	Count,
};
/**
 * Who is on it. You: the Look Compose is given. Opponent: hero::CharacterFor(Facts.Opponent), or nobody's silhouette when
 * the facts name nobody (storytime). Creator and WorldPlayer: the Look again (a world video's page passes the creator's
 * hero::CharacterFor). Ghost: gh0stfold's ghost. Nobody: cards only. Object: the archetype's object, no face.
 */
enum class Subject : int16_t
{
	You,
	Opponent,
	Creator,
	WorldPlayer,
	Ghost,
	Nobody,
	Object,
	Count,
};
/** The 8 caption styles. */
enum class CapStyle : int16_t
{
	Outlined, // the 8-way fake outline plus a drop shadow
	Boxed,    // a plate
	TwoTone,  // line 1 big in the accent, line 2 smaller in white
	Marker,   // a highlighter bar
	Stamp,
	NumberBurst,
	LowerThird,
	None,
	Count,
};
/** The 14 stickers and annotations (Sticker -1: none). Vectors, never emoji. */
enum class Sticker : int16_t
{
	RedCircle,
	CurvedArrow,
	RedX,
	Huh, // "??"
	PercentChip,
	AllIn,
	Skull,
	Bolt, // §6.1's "fire" is a lightning bolt (C9: the fire thread is Ignite's)
	MoneyBag,
	Crying,
	ShockedFace,
	Siren,
	Crown,
	Ghost,
	Count,
};
/** The 4 corner marks. */
enum class Corner : int16_t
{
	Initials,
	Logo,
	ColourBar,
	None,
	Count,
};
/**
 * The 38 props (PropA, PropB; -1 none; video::thumb::PropId's order), drawn in the archetype's prop slot. Cards draw only
 * the facts' cards (backs otherwise); a prop that reads a fact (p, g, r, f/t, m) draws its frame alone without it.
 */
enum class Prop : int16_t
{
	HoleCards,
	ShownCards,
	Board,
	CardBacks,
	ChipPile,
	CashStack,
	Trophy,
	Bracelet,
	Ring,
	EventCrest,
	SiteMark,
	VenueMark,
	SponsorMark,
	Badge,
	Gear,
	Flag,
	EvictionNotice,
	PowerBill,
	StickyNote,
	NeonSign,
	PawnTicket,
	Bus,
	LaptopLobby,
	PhoneChat,
	Whiteboard,
	FeltTop,
	CardFan,
	MysteryEnvelope,
	Statement,
	ProgressBar,
	GraphLine,
	Crosshair,
	OptionCards,
	TierRows,
	NumberedCells,
	Confetti,
	Shield,
	FuseReel,
	Count,
};
constexpr int ArchCount = static_cast<int>(Arch::Count);
/** The archetype's name ("Gut Punch"); "" out of range. */
SHORTSTACKCORE_API const char* ArchName(int A);

/**
 * Compose with the facts a recipe draws besides its captions: the video's Thumb (Video::Th; Studio's options:
 * video::ThumbFor(Choice, Open)): Cards "mine/theirs/board" (theirs only when turned up), Opponent (empty: nobody), Site.
 * The overload without facts draws card backs and nobody for the opponent. Draws inside R (clipped to it), any size.
 */
SHORTSTACKCORE_API void Compose(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, bool Lod, const video::Thumb& Facts);
/** Tiles narrower than this draw the LOD (S-T: the 211-wide row; no hair strands, stipple or wrinkles). */
constexpr float LodWidth = 211.0f;
/** Whether a tile W wide draws the LOD. */
inline bool LodFor(float W)
{
	return W < LodWidth;
}
/** The key a thumbnail caches under: the spec (captions included), the facts, the Look, W and H, the LOD. */
SHORTSTACKCORE_API uint64_t KeyOf(const video::ThumbSpec& S, const hero::Character& Look, const video::Thumb& Facts, float W, float H, bool Lod);
/**
 * Rerun's thumbnail pool, in vertices (S-T, durable/creator/S-T.md): every Rerun page's thumbnails at once (Studio's six
 * options and its preview 32k, Home's 12 tiles 44k, the Content table's 60 rows 61k, Channel's 12 tiles 42k, Research and
 * the launch card about 40k) twice over, so Studio's recent deals stay warm: about 20 MB at the 41 bytes a cached vertex
 * takes with its indices. Canvas.cpp's default for CachePool::Rerun (the lead's patch ui-thumbs_to_lead_1).
 */
constexpr size_t PoolVertices = 480000;
/** One frame's paint allowance (V12): at most Max thumbnails painted uncached; the others wait as skeletons. */
struct Budget
{
	int Max = 2;
	int Painted = 0; // painted uncached this frame
	int Waiting = 0; // drawn as skeletons this frame (the page asks for another frame while any wait)
};
/**
 * A thumbnail as a page draws it (progressive paint, §6.1): replayed from Rerun's pool when Canvas::IsCached says so; else
 * painted into the pool while the frame's Budget allows; else a skeleton until its turn. True when the real one drew.
 */
SHORTSTACKCORE_API bool Draw(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, const video::Thumb& Facts, Budget& B);
/**
 * Draw with the LOD chosen by the page instead of LodFor(R.W): S-T's fail-over for a page whose tiles must be lighter (a
 * Home grid drawn with Lod true: the flat faces, about 1,600 vertices a 375-wide tile instead of about 3,400).
 */
SHORTSTACKCORE_API bool Draw(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, const video::Thumb& Facts, Budget& B, bool Lod);
/** The placeholder a tile shows until its thumbnail is painted: the recipe's palette, dimmed, with a caption's bar. */
SHORTSTACKCORE_API void Skeleton(Canvas& C, Rect R, const video::ThumbSpec& S);
/** The Look a recipe draws for "you": the channel's frozen Look for S.LookRef (ChannelX::Looks), else Current. */
SHORTSTACKCORE_API hero::Character LookFor(const video::ThumbSpec& S, const video::Channel& Ch, const hero::Character& Current);
/**
 * A video's thumbnail as a page draws it: an Update 3 video's shown recipe (VideoX::Spec[max(0, DuelWin)]) through Draw
 * with its frozen Look; a video with no recipe through Update 2's frame (videoart::Thumbnail with Current), pixel-identical.
 */
SHORTSTACKCORE_API bool VideoThumb(Canvas& C, Rect R, const video::Video& V, const video::Channel& Ch, const hero::Character& Current, Budget& B);
/**
 * Every word the compositor draws of its own (the archetypes' fixed labels: "CALL", "FOLD", "VS", the notices' titles...),
 * for the copy and trademark screens. None carries a digit: every digit on a thumbnail is a caption's or a card's (Top
 * Five's list positions 1-5 are its only numerals, the list's own order).
 */
SHORTSTACKCORE_API std::vector<std::string> Words();
/** A palette's chip (Studio's PALETTE picker, update 3 §6.6.2; c3f-ui): the palette's ground as a recipe draws it and its
 *  accent as a bar. Site: the moment's site for the Site palette (0: RiverLine's); Variant: Mono's accent. */
SHORTSTACKCORE_API void Swatch(Canvas& C, Rect R, int Pal, int Site, uint32_t Variant);
} // namespace thumbart
} // namespace ui
} // namespace ss
