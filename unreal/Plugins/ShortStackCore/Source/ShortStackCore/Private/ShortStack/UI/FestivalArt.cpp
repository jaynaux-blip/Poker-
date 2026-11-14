// Update 4, Festival Season's look (SPEC v1.7 §2.7): the ten looks, the palette swap, the trophies, belts, pins, word-marks and felt
// marks (UH/FestivalArt.h), and the art's strings (FestArtInventory). F-look's file from G1.
//
// Phase A (§6 PA-44; the lead): stubs that keep today's behaviour. Every look is empty, LookFor finds none, Dressed returns its base
// and the art draws nothing; nothing calls them before G1.
// G1 FL-1 (F-look): the ten looks (§2.7.1, §2.7.2's table), LookFor (PLAN_FLOOK QF-3: the brand from the series id's slot, through
// EventArt's own SlotOf) and Dressed (§2.7.3's six tokens). Data only: nothing outside tests reads a look until FL-2 (crests) and FL-4
// (the takeover), so no page moves. The palette rules are tests/u4_ui_look.cpp's FestLookDistance, FestContrast and FestLeak, twins
// of durable/update4/model/fest_brands/palette_check.py.
// G1 FL-2 (F-look): the crests read the looks (UI/EventArt.cpp: FrameFor, SeriesGlyph and FestDress take LookFor first; SlotOf strips
// a rival's site key), so RCOP's crest gains its wave band and a rival festival's series wears its own frame, motif and rim.
// G1 FL-3 (F-look): each look's Trophy is fest::TrophyName's (LEAD_CALLS R-3; F-net's body, merged with FN-2a), and FestTrophy,
// FestBelt and FestPin have their bodies in UI/EventArt.cpp beside the crests' pens (their stubs left this file).
// G1 FL-5 (F-look): FestArtInventory holds the art's words (§2.11; every entry carries its kind): U32's foot sign "STEP INSIDE · THE HUB",
// painted by UI/IgniteArt.cpp on FEIGN's arch and PENNANT's pavilion from the same literal (FestHubSign).
// G1 FL-7 (F-look): the word-marks and the motions (§2.7.1, §2.7.2's word-mark and motion columns, §2.7.7). FestWordMark (both overloads)
// sets the festival's name in its look's face, caps and tracking, twice-printed for RunItTwice's three, with PENNANT's stitched underline
// and PUSH's gauge underline; FestBanner draws the hub banner's ground (the Bg2 -> Felt gradient in the host's frame) with the look's
// motif and its banner-level motions. Each motion is a closed form of the seconds since the hub opened and draws its settled frame while
// Still: WRIT's letters flap in one by one, FEIGN's bob on a slow sine (its still: the frame at 1.2 s), PUSH's gauge fills to the day's
// progress, GRIT's curtains part, RCOP's waves rise to the day's mark, Summer Slam's rays turn (live), REIGN's door-light comes up on Main
// day. Settled frames are cached fragments, the gauge and the wave's mark per world minute (§2.7.7). The crest motions (the ring's turn,
// the dome's chase, the key's turn, the seal's press) and the Clasp go through SeriesCrest's and FestTrophy's Time in UI/EventArt.cpp.
// Nothing outside tests calls these until F-hubs' hub, so no page moves.
// G1 FL-4 (F-look): the takeover (§2.7.3, §2.7.4, §2.16). DevLook holds ss.Fest.Look's brand (presentation only, never saved), which
// SitesSkin and SitesTableLook read ahead of the calendar; FeltMark draws the look's low-detail crest at alpha 0.14 (one cached fragment
// a look and size; Suppressed draws nothing); FestCardBack draws the host's back pattern in the look's Accent and Accent2 with the
// crest at its centre (one cached fragment a look and size). The low-detail crest at any size is SeriesCrest's own 30-unit path drawn
// at 26 and scaled (FestLowCrest), so it stays the frame's silhouette and the motif ("drop detail, don't shrink"); on a card back,
// drawn fourteen times at a table, the crest drops its motif too and is its frame's silhouette in Accent2 (the low-detail crest alone
// measures 227-827 vertices, the back's budget is 300: FestTableBudgets prints each).
// UI/RiverLineSites.cpp dresses the skin and the festival tables; UI/RiverLine.cpp draws the felt mark and REIGN's rail.
// G1 FL-9 (F-look): FestArtInventory gains U39, the shelf's empty plinth plate "{SHORT} · not yet" (§2.11, §9.2; PLAN_FLOOK's
// Corrections item 12, cov2 C14), which the game module's shelf (game/FestShelf.cpp) engraves with {SHORT} the brand's series Short.
// G1 polish (key g1p-hub, after G1's PIE: RCOP's waves struck through the hub's tagline): FestBannerClear is FestBanner with a list of
// boxes the waves pass behind (the hub's header text, its flap row and the Main card); each wave line is broken where it would cross
// one (FestWaveLines), at every minute of the day's climb and through the rise, and the settled fragment's key takes the boxes. With
// no boxes it draws exactly what FestBanner draws (FestBanner calls it so). FestWordWidth is the word-mark's set-out width.
#include "ShortStack/UI/FestivalArt.h"

#include "RitArt.h"
#include "RiverLineShared.h"
#include "ShortStack/Common.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

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
// G1 FL-4: the private Frame enum (opaque here; FestivalLook::Frame holds its values) and a frame's outline, for a card back's crest.
enum class Frame : int;
std::vector<Vec2> FramePts(Frame F, float K);
} // namespace eventart_detail
} // namespace eventart

namespace festart
{
namespace festart_detail
{
// eventart's private Frame enum (UI/EventArt.cpp:1610-1628) by value, as FestivalLook::Frame holds it (§2.7.1): the three existing
// frames the RiverLine looks keep and Phase A's six appends. UI/EventArt.cpp's FrameFor reads Frame back and static_asserts these values.
constexpr int FestFrameShield = 0, FestFrameSunburst = 2, FestFrameMedallion = 3;
constexpr int FestFrameArch = 8, FestFrameTower = 9, FestFrameOctagon = 10, FestFrameProscenium = 11, FestFramePavilion = 12, FestFrameKeyhole = 13;
// "Tracked wide" (WRIT, REIGN): the gap between letters as a share of the size. G1 FL-7: 0.28, the word-mark the mockups set (mockups
// 02 and 11, mockups/_gen/u4.py's wordmark: 0.28 of the size; FL-1's 0.12 was RunItTwice's kickers, 1.5 at 13, a label's tracking).
constexpr float FestTrackWide = 0.28f;

/** The six swapped tokens and the art-only Kit colour of one row (§2.7.2), as 0xRRGGBB. */
struct FestLookInks
{
	uint32_t Accent, Accent2, Gold, Felt, FeltDark, Bg2, Kit;
};

/** One row of §2.7.2's table; Key = Fnv1a("fest:" + the brand key) (§2.7.1). Face: a ui::Font, or -1 for the site's title face. */
FestivalLook FestLookRow(fest::Brand B, const FestLookInks& I, int Frame, eventart::Glyph G, Rim R, int Face, bool Caps, bool Twice, float Tracking, Motion M,
	SoundId Sting)
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
	L.Trophy = fest::TrophyName(B); // §2.11's P1-P10 as F-net prints them (R-3)
	L.Key = Fnv1a(std::string("fest:") + fest::BrandKey(B));
	return L;
}

/**
 * §2.7.2's ten rows, indexed by fest::Brand (None: an empty look). RiverLine's four wear the site's title face as written (no caps)
 * and its sting; Ignite's three BlackItalic and FestIgnite; RunItTwice's three BoldCondensed, twice-printed, with FestRit, keeping
 * Signal Red as Accent and Hot White as Gold (§2.7.3: red is light, never ink); REIGN keeps the club's own black cloth and Bg2 and
 * swaps Accent2 only. Trophy: fest::TrophyName(B), tier 0 (FL-3, R-3).
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
		Motion::WaveRise, SoundId::FestRiver));
	Put(FestLookRow(Brand::RingRush, {0xe0566e, 0xd9a441, 0xf2c14e, 0x4a0f1d, 0x2a0811, 0x1a0d14, 0}, FestFrameMedallion, Glyph::Ring, Rim::ByStakes, FaceSite, false, false,
		0.0f, Motion::RingTurn, SoundId::FestRiver));
	Put(FestLookRow(Brand::Tco, {0x7b96ff, 0xcfd6e3, 0xf0c75e, 0x13286b, 0x0a1640, 0x0c1430, 0}, FestFrameMedallion, Glyph::Bracelet, Rim::ByStakes, FaceSite, false, false,
		0.0f, Motion::Clasp, SoundId::FestRiver));
	Put(FestLookRow(Brand::Slam, {0xff8a3d, 0x3cc8ff, 0xffd45c, 0x5a2a14, 0x341607, 0x1c120c, 0}, FestFrameSunburst, Glyph::Sun, Rim::ByStakes, FaceSite, false, false, 0.0f,
		Motion::RaysTurn, SoundId::FestRiver));
	// Ignite.
	Put(FestLookRow(Brand::Feign, {0xff4fa3, 0x5ff2c9, 0xffe45c, 0x3d1752, 0x220c30, 0x2a1238, 0}, FestFrameArch, Glyph::Mirror, Rim::CandyStripe, FaceBlackItalic, true,
		false, 0.0f, Motion::Bob, SoundId::FestIgnite));
	Put(FestLookRow(Brand::Push, {0xf2a93b, 0x7fd4ff, 0xffd76a, 0x1f1b4a, 0x110f2a, 0x1d1430, 0}, FestFrameTower, Glyph::PushButton, Rim::Brass, FaceBlackItalic, true, false,
		0.0f, Motion::Gauge, SoundId::FestIgnite));
	Put(FestLookRow(Brand::Pennant, {0x21d958, 0xff6b57, 0xffe9a8, 0x0d3b45, 0x06222a, 0x101a2e, 0}, FestFramePavilion, Glyph::Pennant, Rim::Neon, FaceBlackItalic, true,
		false, 0.0f, Motion::PavilionChase, SoundId::FestIgnite));
	// RunItTwice.
	Put(FestLookRow(Brand::Writ, {0xd2141a, 0xe3e6ec, 0xffffff, 0x23252b, 0x101114, 0x0e0f13, 0xc9ced8}, FestFrameOctagon, Glyph::Seal, Rim::Chrome, FaceBoldCondensed, true,
		true, FestTrackWide, Motion::Flap, SoundId::FestRit));
	Put(FestLookRow(Brand::Grit, {0xd2141a, 0xf3e9d2, 0xffffff, 0x4a0b12, 0x24060a, 0x140a0c, 0xd9b25f}, FestFrameProscenium, Glyph::TwinStars, Rim::GoldLeaf,
		FaceBoldCondensed, true, true, 0.0f, Motion::Curtains, SoundId::FestRit));
	Put(FestLookRow(Brand::Reign, {0xd2141a, 0xe8b4a0, 0xffffff, 0x1a1a1f, 0x09090b, 0x111114, 0xe8b4a0}, FestFrameKeyhole, Glyph::Sceptre, Rim::Chrome, FaceBoldCondensed,
		true, true, FestTrackWide, Motion::KeyTurn, SoundId::FestRit));
	return T;
}

/** U32's foot sign on FEIGN's arch and PENNANT's pavilion (§2.11, §2.7.4): UI/IgniteArt.cpp paints it, FestArtInventory lists it. */
const char* FestHubSign()
{
	return "STEP INSIDE \xC2\xB7 THE HUB";
}

// ------------------------------------------------------------------ G1 FL-7: the word-marks, the banner and their motions (§2.7.7)

// The motions' clocks, in real seconds since the hub opened (each a closed form of Now - At; a still is the settled frame).
constexpr double FestFlapFirst = 0.25, FestFlapEach = 0.05; // motion::Flap's: the first letter settles at 0.25 s, each next 50 ms later
constexpr double FestBobPeriod = 2.4;                        // FEIGN's slow sine: one bob
constexpr double FestBobStill = 1.2;                         // its still: the frame 1.2 s after the hub opened (mockup 01's)
constexpr float FestBobLift = 0.08f;                         // the bob's height, a share of the size
constexpr float FestBobStep = 0.9f;                          // the phase between neighbouring letters
constexpr double FestGaugeSeconds = 0.9;                     // PUSH's gauge fills to the day's progress
constexpr double FestCurtainSeconds = 1.4;                   // GRIT's curtains part
constexpr double FestWaveSeconds = 1.6;                      // RCOP's waves rise to the day's mark
constexpr double FestDoorSeconds = 0.8;                      // REIGN's door-light comes up on Main day
constexpr double FestRaysTurn = 0.05;                        // Summer Slam's rays: radians a second, live (banner only)
constexpr int FestDayMinutes = 1440;                         // the gauge and the wave's mark move once a world minute
// The banner's cached layers (FestBannerKey).
constexpr int FestLayerGround = 0, FestLayerWave = 1, FestLayerCurtain = 2, FestLayerDoor = 3;

/** FNV-1a over raw bytes (the word-mark's and the banner's cache keys). */
uint64_t FestMix(uint64_t H, const void* Data, size_t Size)
{
	const unsigned char* B = static_cast<const unsigned char*>(Data);
	for (size_t K = 0; K < Size; ++K)
	{
		H = (H ^ B[K]) * 0x100000001b3ull;
	}
	return H;
}

/** A look colour moved toward another (0xRRGGBB), at alpha A. */
Color FestToward(uint32_t Rgb, uint32_t To, float T, float A = 1.0f)
{
	Color C = Mix(Hex(Rgb), Hex(To), T);
	C.A = A;
	return C;
}

/** The brand's host (§2.7.2's site column): its frame (radius or cut) is the banner's. */
net::SiteId FestHostOf(fest::Brand B)
{
	switch (B)
	{
	case fest::Brand::Feign:
	case fest::Brand::Push:
	case fest::Brand::Pennant: return net::SiteId::Ignite;
	case fest::Brand::Writ:
	case fest::Brand::Grit:
	case fest::Brand::Reign: return net::SiteId::RunItTwice;
	default: return net::SiteId::RiverLine;
	}
}

/** Capitals, ASCII only (a festival's name is ASCII; anything else stays as typed). */
std::string FestUpper(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'a' && Ch <= 'z' ? static_cast<char>(Ch - 'a' + 'A') : Ch;
	}
	return Out;
}

/** S split into its UTF-8 glyphs. */
std::vector<std::string> FestGlyphs(const std::string& S)
{
	std::vector<std::string> Out;
	for (size_t I = 0; I < S.size();)
	{
		const unsigned char B = static_cast<unsigned char>(S[I]);
		const size_t Len = std::min(S.size() - I, static_cast<size_t>(B >= 0xF0 ? 4 : B >= 0xE0 ? 3 : B >= 0xC0 ? 2 : 1));
		Out.push_back(S.substr(I, Len));
		I += Len;
	}
	return Out;
}

/** The world minute of the day's progress (0..1440): the gauge and the wave's mark change once a world minute (§2.7.7). */
int FestMinuteOf(float DayProgress)
{
	const float P = DayProgress > 0.0f ? (DayProgress < 1.0f ? DayProgress : 1.0f) : 0.0f; // NaN reads 0
	return static_cast<int>(std::lround(P * static_cast<float>(FestDayMinutes)));
}

/** The word-mark's face (§2.7.1's WordFace; -1, the site's title face: RiverLine's bold titles), weight 800. */
TextStyle FestWordStyle(const FestivalLook& L, float Size, const Color& Col)
{
	TextStyle St;
	St.Size = Size;
	St.Weight = 800;
	St.Col = Col;
	St.Face = L.WordFace;
	return St;
}

float FestWordMeasure(const Canvas& C, const FestivalLook& L, const std::string& S, float Size)
{
	return L.WordFace >= 0 ? C.Measure(S, Size, static_cast<Font>(L.WordFace)) : C.Measure(S, Size, 800, false);
}

/** A word-mark set out: one run, or letter by letter (tracked, bobbing or flapping) with each glyph's x and advance. */
struct FestWordSet
{
	std::vector<std::string> Glyphs;
	std::vector<float> At, Wide;
	float Width = 0.0f;
	bool PerGlyph = false;
};

FestWordSet FestWordLay(const Canvas& C, const FestivalLook& L, const std::string& Shown, float Size)
{
	FestWordSet W;
	W.PerGlyph = L.Tracking > 0.0f || L.Move == Motion::Bob || L.Move == Motion::Flap;
	if (!W.PerGlyph)
	{
		W.Width = FestWordMeasure(C, L, Shown, Size);
		return W;
	}
	const float Track = L.Tracking * Size;
	float X = 0.0f;
	for (const std::string& G : FestGlyphs(Shown))
	{
		const float Adv = FestWordMeasure(C, L, G, Size);
		W.Glyphs.push_back(G);
		W.At.push_back(X);
		W.Wide.push_back(Adv);
		X += Adv + Track;
	}
	W.Width = W.Glyphs.empty() ? 0.0f : X - Track;
	return W;
}

/** WRIT's flap (§2.7.2): a letter still turning is a split-flap tile its own width, Vault-dark with the split across its middle. */
void FestFlapTile(Canvas& C, const FestivalLook& L, float X, float Y, float Wide, float Size)
{
	const Rect Tile{X - 0.02f * Size, Y - 0.76f * Size, Wide + 0.04f * Size, 0.9f * Size};
	C.FillRoundRect(Tile, 0.06f * Size, Paint::Linear({Tile.X, Tile.Y}, {Tile.X, Tile.Y + Tile.H}, FestToward(L.Bg2, 0xffffff, 0.14f), Hex(L.Bg2)));
	C.FillRect({Tile.X, Tile.Y + Tile.H * 0.5f - 0.5f, Tile.W, 1.0f}, Rgba(0, 0, 0, 0.7f));
	C.StrokeRoundRect(Tile, 0.06f * Size, Hex(L.Accent2, 0.25f), 1.0f);
}

/** The underline under the mark (§2.7.2): PENNANT's stitched (an Outfield Green bar under coral stitches; no bulbs), PUSH's gauge (a Sky
 *  Bulb track filled in Hot Brass to Fill, quarter ticks, a lit needle at the fill); none for the other eight. */
void FestWordUnder(Canvas& C, const FestivalLook& L, float X, float Y, float Size, float Width, float Fill)
{
	const Rect Bar{X + 0.04f * Size, Y + 0.13f * Size, Width - 0.1f * Size, 0.085f * Size};
	if (Bar.W <= 1.0f || (L.Which != fest::Brand::Pennant && L.Move != Motion::Gauge))
	{
		return;
	}
	if (L.Which == fest::Brand::Pennant)
	{
		C.FillRoundRect(Bar, 0.04f * Size, Hex(L.Accent));
		const float Sy = Bar.Y + Bar.H * 0.5f;
		C.StrokeDashed({{X + 0.1f * Size, Sy}, {X + Width - 0.14f * Size, Sy}}, Hex(L.Accent2), 0.03f * Size, 0.09f * Size, 0.06f * Size);
		return;
	}
	const float Half = Bar.H * 0.5f;
	const float F = Fill > 0.0f ? (Fill < 1.0f ? Fill : 1.0f) : 0.0f;
	C.FillRoundRect(Bar, Half, Hex(L.Accent2, 0.16f));
	C.StrokeRoundRect(Bar, Half, Hex(L.Accent2, 0.55f), 1.0f);
	if (Bar.W * F >= 1.0f)
	{
		C.FillRoundRect({Bar.X, Bar.Y, Bar.W * F, Bar.H}, std::min(Half, Bar.W * F * 0.5f), Paint::Linear({Bar.X, Bar.Y}, {Bar.X + Bar.W, Bar.Y}, Hex(L.Accent), Hex(L.Gold)));
	}
	for (int K = 1; K < 4; ++K)
	{
		C.FillRect({Bar.X + Bar.W * static_cast<float>(K) / 4.0f - 0.5f, Bar.Y + 1.0f, 1.0f, std::max(0.0f, Bar.H - 2.0f)}, Rgba(0, 0, 0, 0.35f));
	}
	C.FillCircle(Bar.X + Bar.W * F, Bar.Y + Half, Half * 1.5f, Hex(L.Accent2));
}

/** The letters: one run or glyph by glyph; RunItTwice's twice print (a Signal Red under-copy at the site's offset, from its 28-unit
 *  floor) under Hot White; FEIGN's funhouse letters (pink and mint over a Lemon Drop offset) bobbing; a letter past Settled a flap tile. */
void FestWordLetters(Canvas& C, const FestivalLook& L, const FestWordSet& W, const std::string& Shown, float X, float Y, float Size, double T, int Settled)
{
	const Color White = Hex(0xffffff);
	const bool Twice = L.Twice && Size >= rlrit_detail::RitTwiceFrom;
	const float O = Twice ? rlrit_detail::RitTwiceOffset(Size) : 0.0f;
	if (!W.PerGlyph)
	{
		if (Twice)
		{
			C.Text(Shown, X + O, Y + O, FestWordStyle(L, Size, Hex(L.Accent)));
		}
		C.Text(Shown, X, Y, FestWordStyle(L, Size, White));
		return;
	}
	const bool Funhouse = L.Which == fest::Brand::Feign;
	for (size_t I = 0; I < W.Glyphs.size(); ++I)
	{
		const std::string& G = W.Glyphs[I];
		if (G == " ")
		{
			continue;
		}
		const float Gx = X + W.At[I];
		float Gy = Y;
		if (L.Move == Motion::Bob)
		{
			const double Phase = 6.283185307179586 * T / FestBobPeriod + static_cast<double>(FestBobStep) * static_cast<double>(I);
			Gy -= FestBobLift * Size * static_cast<float>(std::sin(Phase));
		}
		if (static_cast<int>(I) >= Settled)
		{
			FestFlapTile(C, L, Gx, Y, W.Wide[I], Size);
			continue;
		}
		if (Funhouse)
		{
			C.Text(G, Gx + 0.05f * Size, Gy + 0.05f * Size, FestWordStyle(L, Size, Hex(L.Gold)));
			C.Text(G, Gx, Gy, FestWordStyle(L, Size, Hex(I % 2 == 0 ? L.Accent : L.Accent2)));
			continue;
		}
		if (Twice)
		{
			C.Text(G, Gx + O, Gy + O, FestWordStyle(L, Size, Hex(L.Accent)));
		}
		C.Text(G, Gx, Gy, FestWordStyle(L, Size, White));
	}
}

/** One frame of the word-mark at T (Moving: the flap still running), its gauge filled to Fill. */
void FestWordPaint(Canvas& C, const FestivalLook& L, const std::string& Shown, float X, float Y, float Size, double T, float Fill, bool Moving)
{
	const FestWordSet W = FestWordLay(C, L, Shown, Size);
	const int All = static_cast<int>(W.Glyphs.size());
	const int Settled = Moving && L.Move == Motion::Flap ? motion::Flap(T, 0.0, All) : All;
	FestWordUnder(C, L, X, Y, Size, W.Width, Fill);
	FestWordLetters(C, L, W, Shown, X, Y, Size, T, Settled);
}

/** When the word-mark's motion settles (its still's clock): the flap's last letter (with a step to spare), FEIGN's still at 1.2 s, the
 *  gauge's fill; 0 for a look whose word-mark doesn't move. */
double FestWordSettle(const FestivalLook& L, const std::string& Text)
{
	switch (L.Move)
	{
	case Motion::Flap: return FestFlapFirst + FestFlapEach * static_cast<double>(FestGlyphs(Text).size());
	case Motion::Bob: return FestBobStill;
	case Motion::Gauge: return FestGaugeSeconds;
	default: return 0.0;
	}
}

/** The settled word-mark's fragment: the look, the skin, the text as shown, the size and (the gauge) the world minute. */
uint64_t FestWordMarkKey(const FestivalLook& L, const std::string& Shown, float Size, int Minute)
{
	uint64_t H = FestMix(0xcbf29ce484222325ull, "festword1", 9);
	const uint64_t SkinKey = Skin::Current().Key();
	H = FestMix(H, &L.Key, sizeof(L.Key));
	H = FestMix(H, &SkinKey, sizeof(SkinKey));
	H = FestMix(H, &Size, sizeof(Size));
	H = FestMix(H, &Minute, sizeof(Minute));
	return FestMix(H, Shown.data(), Shown.size());
}

/** When the banner's motions settle (its still's clock): the curtains parted, the waves risen, the door-light up; Summer Slam's rays turn
 *  for good, so their still is the frame at 0. */
double FestBannerSettle(const FestivalLook& L)
{
	if (L.Which == fest::Brand::Reign)
	{
		return FestDoorSeconds;
	}
	return L.Move == Motion::Curtains ? FestCurtainSeconds : L.Move == Motion::WaveRise ? FestWaveSeconds : 0.0;
}

/** A banner layer's fragment: the look, the skin, the rect's size, the layer and (the wave) the world minute. */
uint64_t FestBannerKey(const FestivalLook& L, const Rect& R, int Layer, int Minute)
{
	uint64_t H = FestMix(0xcbf29ce484222325ull, "festbanner1", 11);
	const uint64_t SkinKey = Skin::Current().Key();
	const float Box[2] = {R.W, R.H};
	const int Ints[2] = {Layer, Minute};
	H = FestMix(H, &L.Key, sizeof(L.Key));
	H = FestMix(H, &SkinKey, sizeof(SkinKey));
	H = FestMix(H, Box, sizeof(Box));
	return FestMix(H, Ints, sizeof(Ints));
}

/** g1p-hub: FestBannerKey with the boxes the waves pass behind mixed in (none: FestBannerKey itself). */
uint64_t FestBannerKeyClear(const FestivalLook& L, const Rect& R, int Layer, int Minute, const std::vector<Rect>& Clear)
{
	uint64_t H = FestBannerKey(L, R, Layer, Minute);
	for (const Rect& C : Clear)
	{
		const float Q[4] = {C.X, C.Y, C.W, C.H};
		H = FestMix(H, Q, sizeof(Q));
	}
	return H;
}

/** The banner's outline in R: the host's frame (RiverLine's and Ignite's panel radius, RunItTwice's one cut corner). */
std::vector<Vec2> FestBannerOutline(const FestivalLook& L, const Rect& R)
{
	const rlsites_detail::SiteLook& S = rlsites_detail::SitesLook(FestHostOf(L.Which));
	if (S.Cut > 0.0f)
	{
		return rlrit_detail::RitCutPath(R, S.Cut);
	}
	return Canvas::RoundRectPath(R, std::min(S.PanelRadius, std::min(R.W, R.H) * 0.5f), 6);
}

/** The inset that keeps a motif clear of the host's rounded corners. */
float FestBannerInset(const FestivalLook& L)
{
	return rlsites_detail::SitesLook(FestHostOf(L.Which)).PanelRadius;
}

/** A soft light (a Radial paint, Light's centre colour fading out) filling the ellipse at (Cx, Cy) with radii Rx, Ry; the callers keep the
 *  ellipse inside the banner's frame (no clip inside a cached fragment), so it never spills onto the page. */
void FestGlow(Canvas& C, float Cx, float Cy, float Rx, float Ry, uint32_t Rgb, float Alpha)
{
	// A circle of radius Rx squashed to Ry, so the light fades to nothing on the whole rim (a Radial paint is round).
	const Vec2 O{0.0f, 0.0f};
	C.Save();
	C.Translate(Cx, Cy);
	C.Scale(1.0f, Ry / Rx);
	C.FillEllipse(0.0f, 0.0f, Rx, Rx, Paint::Radial(O, 0.0f, O, Rx, Hex(Rgb, Alpha), 0.45f, Hex(Rgb, Alpha * 0.4f), Hex(Rgb, 0.0f)));
	C.Restore();
}

/** A pennant line (PENNANT, §2.7.2: felt pennants in coral and Outfield Green, no bulbs): a sagging string with N pennants hung on it. */
void FestPennantLine(Canvas& C, const FestivalLook& L, float X0, float X1, float Y, int N)
{
	const float Sag = 9.0f;
	std::vector<Vec2> String;
	for (int K = 0; K <= 16; ++K)
	{
		const float T = static_cast<float>(K) / 16.0f;
		String.push_back({X0 + (X1 - X0) * T, Y + 4.0f * Sag * T * (1.0f - T)});
	}
	C.StrokePolyline(String, false, Rgba(255, 255, 255, 0.28f), 1.0f);
	for (int K = 0; K < N; ++K)
	{
		const float T = (static_cast<float>(K) + 0.5f) / static_cast<float>(N);
		const float Px = X0 + (X1 - X0) * T;
		const float Py = Y + 4.0f * Sag * T * (1.0f - T);
		C.FillPolygon({{Px - 5.0f, Py}, {Px + 5.0f, Py}, {Px, Py + 12.0f}}, Hex(K % 2 == 0 ? L.Accent2 : L.Accent, 0.88f));
	}
}

/** The banner's ground (one cached fragment, drawn at the origin): the Bg2 -> Felt gradient in the host's frame, the look's light behind
 *  the crest (F-hubs centres it at 90 units in, §2.8.2), and the look's motif, faint, clear of the stats and the corners. */
void FestBannerGround(Canvas& C, const FestivalLook& L, float W, float H)
{
	using fest::Brand;
	const Rect R0{0.0f, 0.0f, W, H};
	const std::vector<Vec2> Outline = FestBannerOutline(L, R0);
	const Vec2 Crest{90.0f, H * 0.5f};
	C.FillPolygon(Outline, Paint::Linear({0.0f, 0.0f}, {W, H}, Hex(L.Bg2), Hex(L.Felt)));
	FestGlow(C, Crest.X + 70.0f, Crest.Y, Crest.X + 66.0f, H * 0.5f - 4.0f, L.Accent, 0.16f); // the look's light behind the crest, spreading right
	const float In = FestBannerInset(L);
	switch (L.Which)
	{
	case Brand::RingRush:
		// Rings in Ring Rush's gold, a garnet set in each.
		C.StrokeEllipse(W * 0.685f, H * 0.52f, 62.0f, 62.0f, Hex(L.Accent2, 0.12f), 6.0f);
		C.StrokeEllipse(W * 0.685f + 70.0f, H * 0.52f + 14.0f, 44.0f, 44.0f, Hex(L.Accent2, 0.09f), 5.0f);
		C.FillCircle(W * 0.685f, H * 0.52f - 62.0f, 8.0f, Hex(L.Accent, 0.24f));
		C.FillCircle(W * 0.685f + 70.0f, H * 0.52f + 14.0f - 44.0f, 6.0f, Hex(L.Accent, 0.2f));
		break;
	case Brand::Tco:
		// A bracelet's links in platinum across the foot.
		for (int K = 0; K < 14; ++K)
		{
			const float X = W * 0.42f + static_cast<float>(K) * 22.0f;
			C.StrokeRoundRect({X, H * 0.8f - 8.0f + (K % 2 == 0 ? -2.0f : 2.0f), 30.0f, 16.0f}, 8.0f, Hex(L.Accent2, 0.11f), 3.0f);
		}
		break;
	case Brand::Feign:
		// Candy stripes on the right, and the funhouse's warped mirror lines under the stats.
		for (float X = W * 0.52f; X + 26.0f + H * 0.6f <= W - In; X += 56.0f)
		{
			C.FillPolygon({{X, H}, {X + 26.0f, H}, {X + 26.0f + H * 0.6f, 0.0f}, {X + H * 0.6f, 0.0f}}, Hex(L.Accent, 0.06f));
		}
		for (int K = 0; K < 2; ++K)
		{
			std::vector<Vec2> Line;
			for (int I = 0; I <= 40; ++I)
			{
				const float T = static_cast<float>(I) / 40.0f;
				Line.push_back({W * (0.12f + 0.5f * T), H * (0.86f + 0.035f * static_cast<float>(K)) + (2.0f + 7.0f * T) * std::sin(T * 18.0f + static_cast<float>(K))});
			}
			C.StrokePolyline(Line, false, Hex(L.Accent2, 0.13f), 1.5f);
		}
		break;
	case Brand::Push:
		// The pier's bulbs along the top and a strength tester's scale beside the stats.
		for (float X = W * 0.14f; X <= W * 0.7f; X += 30.0f)
		{
			C.FillCircle(X, 9.0f, 5.5f, Hex(L.Gold, 0.14f));
			C.FillCircle(X, 9.0f, 2.2f, Hex(L.Gold, 0.6f));
		}
		C.FillRect({W * 0.705f, H * 0.2f, 3.0f, H * 0.65f}, Hex(L.Accent2, 0.16f));
		for (int K = 0; K <= 8; ++K)
		{
			C.FillRect({W * 0.705f - 4.0f, H * 0.2f + H * 0.65f * static_cast<float>(K) / 8.0f - 1.0f, 11.0f, 2.0f}, Hex(L.Accent2, 0.22f));
		}
		break;
	case Brand::Pennant:
		// The infield's chalk lines, faint, and two pennant lines across the top (no bulbs: §2.7.2).
		for (int K = 0; K < 5; ++K)
		{
			const float X = W * (0.35f + 0.077f * static_cast<float>(K));
			C.StrokePolyline({{X, H - 2.0f}, {X + W * 0.13f, 2.0f}}, false, Rgba(255, 255, 255, 0.05f), 1.0f);
		}
		FestPennantLine(C, L, W * 0.11f, W * 0.42f, 3.0f, 15);
		FestPennantLine(C, L, W * 0.42f, W * 0.72f, 3.0f, 14);
		break;
	case Brand::Writ:
	{
		// RunItTwice's red pinline along the top (to the cut) and an octagon in Seal Silver, faint.
		const float Cut = rlsites_detail::SitesLook(net::SiteId::RunItTwice).Cut;
		C.FillPolygon({{0.0f, 0.0f}, {W - Cut, 0.0f}, {W - Cut + 4.0f, 4.0f}, {0.0f, 4.0f}}, Hex(L.Accent));
		std::vector<Vec2> Oct;
		for (int K = 0; K < 8; ++K)
		{
			const float A = 0.3926991f + 0.7853982f * static_cast<float>(K);
			Oct.push_back({W * 0.58f + H * 0.44f * std::cos(A), H * 0.5f + H * 0.44f * std::sin(A)});
		}
		C.StrokePolyline(Oct, true, Hex(L.Kit, 0.07f), 3.0f);
		break;
	}
	case Brand::Reign:
		// The club's Rose Brass rail along the foot of its black cloth.
		C.FillRect({0.0f, H - 5.0f, W, 2.5f}, Hex(L.Accent2, 0.75f));
		C.FillRect({0.0f, H - 5.0f, W, 0.8f}, Rgba(255, 255, 255, 0.25f));
		break;
	default: break; // RCOP's waves, Summer Slam's rays and GRIT's curtains are their own layers
	}
}

/** g1p-hub: the part of segment A-B inside box R as [T0, T1] of its length (Liang-Barsky); false when it misses the box. */
bool FestSegIn(const Vec2& A, const Vec2& B, const Rect& R, float& T0, float& T1)
{
	const float Dx = B.X - A.X;
	const float Dy = B.Y - A.Y;
	const float P[4] = {-Dx, Dx, -Dy, Dy};
	const float Q[4] = {A.X - R.X, R.X + R.W - A.X, A.Y - R.Y, R.Y + R.H - A.Y};
	T0 = 0.0f;
	T1 = 1.0f;
	for (int I = 0; I < 4; ++I)
	{
		if (P[I] == 0.0f)
		{
			if (Q[I] < 0.0f)
			{
				return false;
			}
			continue;
		}
		const float T = Q[I] / P[I];
		if (P[I] < 0.0f)
		{
			T0 = std::max(T0, T);
		}
		else
		{
			T1 = std::min(T1, T);
		}
	}
	return T0 < T1;
}

/** g1p-hub: a polyline with the parts inside any box left out, as the pieces between them (each two points or more). */
std::vector<std::vector<Vec2>> FestClipLine(const std::vector<Vec2>& Line, const std::vector<Rect>& Clear)
{
	std::vector<std::vector<Vec2>> Out;
	std::vector<Vec2> Cur;
	bool Open = false; // the last segment's kept part reached its end, so the next one's first part carries on the same piece
	auto Flush = [&Out, &Cur]() {
		if (Cur.size() >= 2)
		{
			Out.push_back(Cur);
		}
		Cur.clear();
	};
	for (size_t I = 0; I + 1 < Line.size(); ++I)
	{
		const Vec2 A = Line[I];
		const Vec2 B = Line[I + 1];
		std::vector<std::pair<float, float>> In;
		for (const Rect& R : Clear)
		{
			float T0 = 0.0f;
			float T1 = 1.0f;
			if (FestSegIn(A, B, R, T0, T1))
			{
				In.push_back({T0, T1});
			}
		}
		std::sort(In.begin(), In.end());
		auto Keep = [&](float From, float To) {
			const Vec2 P{A.X + (B.X - A.X) * From, A.Y + (B.Y - A.Y) * From};
			const Vec2 Q{A.X + (B.X - A.X) * To, A.Y + (B.Y - A.Y) * To};
			if (!(Open && From <= 0.0f))
			{
				Flush();
				Cur.push_back(P);
			}
			Cur.push_back(Q);
			Open = To >= 1.0f;
		};
		float T = 0.0f;
		bool Kept = false;
		for (const std::pair<float, float>& Gap : In)
		{
			if (Gap.first > T)
			{
				Keep(T, Gap.first);
				Kept = true;
			}
			T = std::max(T, Gap.second);
		}
		if (T < 1.0f)
		{
			Keep(T, 1.0f);
			Kept = true;
		}
		Open = Open && Kept && T < 1.0f;
	}
	Flush();
	return Out;
}

/** RCOP's waves (WaveRise, §2.7.2): five lines across the banner, the top one in Crest Silver, at the day's mark (Day 0..1: the line
 *  climbs through the day), Drop units lower while they rise; clear of the corners. g1p-hub: each line's points, in the banner's own
 *  units, broken around Clear (boxes in the same units: FestClipLine); with no boxes each line is one piece, as before. */
std::vector<std::vector<Vec2>> FestWaveLinesIn(const FestivalLook& L, float W, float H, float Day, float Drop, const std::vector<Rect>& Clear, std::vector<int>& Kinds)
{
	const float In = FestBannerInset(L);
	const float Low = H - In - 4.0f * 12.0f - 8.0f - 2.0f;
	const float High = H * 0.3f;
	const float Y0 = Low + (High - Low) * Day + Drop;
	const float Step = 24.0f;
	std::vector<std::vector<Vec2>> Out;
	Kinds.clear();
	for (int K = 0; K < 5; ++K)
	{
		std::vector<Vec2> Line;
		for (float X = In;; X += Step)
		{
			const float Xc = std::min(X, W - In);
			Line.push_back({Xc, Y0 + 12.0f * static_cast<float>(K) + 8.0f * std::sin(Xc / 150.0f + 0.7f * static_cast<float>(K))});
			if (Xc >= W - In)
			{
				break;
			}
		}
		if (Clear.empty())
		{
			Out.push_back(std::move(Line));
			Kinds.push_back(K);
			continue;
		}
		for (std::vector<Vec2>& Piece : FestClipLine(Line, Clear))
		{
			Out.push_back(std::move(Piece));
			Kinds.push_back(K);
		}
	}
	return Out;
}

void FestBannerWave(Canvas& C, const FestivalLook& L, float W, float H, float Day, float Drop, const std::vector<Rect>& Clear = {})
{
	std::vector<int> Kinds;
	const std::vector<std::vector<Vec2>> Lines = FestWaveLinesIn(L, W, H, Day, Drop, Clear, Kinds);
	for (size_t I = 0; I < Lines.size(); ++I)
	{
		const int K = Kinds[I];
		C.StrokePolyline(Lines[I], false, K == 0 ? Hex(L.Accent2, 0.5f) : Hex(L.Accent, 0.16f), K == 0 ? 2.0f : 1.4f);
	}
}

/** Summer Slam's rays (RaysTurn, §2.7.2: live rays, banner only): 24 soft wedges from behind the crest, turning slowly. */
void FestBannerRays(Canvas& C, const FestivalLook& L, float W, float H, double T)
{
	const Vec2 O{90.0f, H * 0.5f};
	const float Reach = W * 0.45f;
	const double Turn = std::fmod(T * FestRaysTurn, 6.283185307179586);
	for (int K = 0; K < 24; ++K)
	{
		const float A = static_cast<float>(Turn) + 0.2617994f * static_cast<float>(K);
		const float Half = K % 2 == 0 ? 0.05f : 0.035f;
		const Color Ink = K % 2 == 0 ? Hex(L.Accent, 0.1f) : Hex(L.Gold, 0.07f);
		Color Gone = Ink;
		Gone.A = 0.0f;
		const Vec2 Far{O.X + Reach * std::cos(A), O.Y + Reach * std::sin(A)};
		C.FillPolygon({O, {O.X + Reach * std::cos(A - Half), O.Y + Reach * std::sin(A - Half)}, {O.X + Reach * std::cos(A + Half), O.Y + Reach * std::sin(A + Half)}},
			Paint::Linear(O, Far, Ink, Gone));
	}
}

/** GRIT's curtains (Curtains, §2.7.2): velvet in its crimson under a valance with a Gold Leaf trim; Part 0 closed across the banner, 1
 *  parted and tied back at both ends (the settled frame). Inside the banner by construction (the valance follows RunItTwice's cut). */
void FestBannerCurtains(Canvas& C, const FestivalLook& L, float W, float H, float Part)
{
	const Color Cloth = FestToward(L.Felt, 0xffffff, 0.18f);
	const Color Deep = Mix(Cloth, Hex(0x000000), 0.45f);
	const uint32_t Trim = L.Kit != 0 ? L.Kit : L.Gold;
	const float Cut = rlsites_detail::SitesLook(FestHostOf(L.Which)).Cut;
	const float Top = H * 0.1f;
	const float E = Part; // CountUp's ease: the cloth slows as it gathers
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const auto Mx = [Side, W](float X) { return Side < 0 ? X : W - X; };
		const float XTop = W * 0.5f + (W * 0.055f - W * 0.5f) * E;
		const float XTie = W * 0.5f + (W * 0.03f - W * 0.5f) * E;
		const float XFoot = W * 0.5f + (W * 0.05f - W * 0.5f) * E;
		std::vector<Vec2> Drape = {{Mx(0.0f), Side > 0 ? std::max(Top, Cut) : Top}, {Mx(XTop), Top}};
		for (int K = 1; K <= 10; ++K)
		{
			const float T = static_cast<float>(K) / 10.0f;
			const float U = 1.0f - T;
			Drape.push_back({Mx(U * U * XTop + 2.0f * U * T * XTie + T * T * XFoot), Top + (H - Top) * T});
		}
		Drape.push_back({Mx(0.0f), H});
		C.FillPolygon(Drape, Paint::Linear({Mx(0.0f), 0.0f}, {Mx(XTop), 0.0f}, Deep, Cloth));
		for (int F = 1; F <= 3; ++F)
		{
			const float K = static_cast<float>(F) / 4.0f;
			C.StrokePolyline({{Mx(XTop * K), Top}, {Mx(XTie * K), Top + (H - Top) * 0.6f}, {Mx(XFoot * K), H - 1.0f}}, false, Hex(0x000000, 0.28f), 1.5f);
		}
	}
	C.FillPolygon({{0.0f, 0.0f}, {W - Cut, 0.0f}, {W, Cut}, {W, Top}, {0.0f, Top}}, Paint::Linear({0.0f, 0.0f}, {0.0f, Top}, FestToward(L.Felt, 0xffffff, 0.26f), Deep));
	C.FillRect({0.0f, Top - 1.5f, W, 2.0f}, Hex(Trim, 0.9f));
}

/** REIGN's door-light (§2.7.2: it glows on Main day): Rose Brass light behind the keyhole crest and spilling down from it, kept inside
 *  the banner's frame. */
void FestBannerDoor(Canvas& C, const FestivalLook& L, float H)
{
	const Vec2 Crest{90.0f, H * 0.5f};
	FestGlow(C, Crest.X, Crest.Y, Crest.X - 4.0f, H * 0.5f - 4.0f, L.Accent2, 0.42f);
	FestGlow(C, Crest.X + 110.0f, Crest.Y, Crest.X + 106.0f, H * 0.5f - 4.0f, L.Accent2, 0.16f);
	C.FillPolygon({{Crest.X - 16.0f, Crest.Y}, {Crest.X + 16.0f, Crest.Y}, {Crest.X + 58.0f, H}, {Crest.X - 58.0f, H}},
		Paint::Linear(Crest, {Crest.X, H}, Hex(L.Accent2, 0.14f), Hex(L.Accent2, 0.0f)));
}

/** A layer drawn live at (X, Y) at alpha K of the canvas's own (restored after; SetAlpha is absolute). */
void FestLive(Canvas& C, float X, float Y, float K, const std::function<void(Canvas&)>& Paint)
{
	const float Was = C.GetAlpha();
	C.Save();
	C.Translate(X, Y);
	C.SetAlpha(Was * K);
	Paint(C);
	C.SetAlpha(Was);
	C.Restore();
}

// ---- G1 FL-4: the takeover's art (§2.7.4) and the dev look (§2.16).

/** The felt mark's alpha (§2.7.4), and the size the low-detail crest is drawn at before it is scaled (eventart's 30-unit path). */
constexpr float FestFeltMarkAlpha = 0.14f;
constexpr float FestLowCrestAt = 26.0f;
/** The back patterns' own inks (UI/Ui.cpp's): RiverLine's light border, Ignite's cream border, RunItTwice's gloss black. */
constexpr uint32_t FestBackRiverRim = 0xf4f6fa, FestBackCream = 0xfff4ec, FestBackGloss = 0x202026, FestBackOnyx = 0x0c0c0f;

/** ss.Fest.Look's brand (presentation only, never saved): one value for the program, read from any thread. */
std::atomic<uint8_t>& FestDevSlot()
{
	static std::atomic<uint8_t> FestDevBrand{0};
	return FestDevBrand;
}

/** The look's series as the crest reads it: the brand key as its slot (LookFor finds the look), the look's accents as its enamel. */
net::SeriesInfo FestLookSeries(const FestivalLook& L)
{
	net::SeriesInfo S;
	S.Id = fest::BrandKey(L.Which);
	S.Color = L.Accent;
	S.Color2 = L.Accent2;
	S.Festival = true;
	S.Brand = fest::BrandKey(L.Which);
	return S;
}

/** The look's low-detail crest centred at (Cx, Cy), Size across: SeriesCrest's own path at 26 units, scaled, so it stays the frame's
 *  silhouette and the motif at any size (§2.7.4). A still: the low-detail path draws no motion. */
void FestLowCrest(Canvas& C, const FestivalLook& L, float Cx, float Cy, float Size)
{
	const float K = Size / FestLowCrestAt;
	C.Save();
	C.Translate(Cx, Cy);
	C.Scale(K, K);
	eventart::SeriesCrest(C, FestLookSeries(L), 0.0f, 0.0f, FestLowCrestAt, 0.0);
	C.Restore();
}

/** The felt mark's fragment: its look and size (the alpha is applied as it replays). */
uint64_t FestFeltMarkKey(const FestivalLook& L, float Size)
{
	uint64_t H = FestMix(0xcbf29ce484222325ull, "feltmark", 8);
	H = FestMix(H, &L.Key, sizeof(L.Key));
	return FestMix(H, &Size, sizeof(Size));
}

/** A card back's fragment: its look and size. */
uint64_t FestCardBackKey(const FestivalLook& L, float W, float H)
{
	uint64_t K = FestMix(0xcbf29ce484222325ull, "cardback", 8);
	K = FestMix(K, &L.Key, sizeof(L.Key));
	K = FestMix(K, &W, sizeof(W));
	return FestMix(K, &H, sizeof(H));
}

/** Sutherland-Hodgman: a convex polygon cut to a rect (a back's bands and rays stay inside its panel). */
std::vector<Vec2> FestClipToRect(const std::vector<Vec2>& Poly, const Rect& Box)
{
	std::vector<Vec2> Kept = Poly;
	for (int Side = 0; Side < 4 && !Kept.empty(); ++Side)
	{
		const auto Edge = [&](const Vec2& P) { return Side == 0 ? P.X - Box.X : Side == 1 ? Box.X + Box.W - P.X : Side == 2 ? P.Y - Box.Y : Box.Y + Box.H - P.Y; };
		const std::vector<Vec2> From = Kept;
		Kept.clear();
		for (size_t I = 0; I < From.size(); ++I)
		{
			const Vec2& A = From[I];
			const Vec2& B = From[(I + 1) % From.size()];
			const float Ea = Edge(A);
			const float Eb = Edge(B);
			if ((Ea >= 0.0f) != (Eb >= 0.0f))
			{
				const float T = Ea / (Ea - Eb);
				Kept.push_back({A.X + (B.X - A.X) * T, A.Y + (B.Y - A.Y) * T});
			}
			if (Eb >= 0.0f)
			{
				Kept.push_back(B);
			}
		}
	}
	return Kept;
}

/** A card back's crest (§2.7.4 at a back's size, within its 300 vertices): the look's frame silhouette, flat, in Accent2, Size across;
 *  an outline point closer than 2.5 units to the last one kept is dropped (a round frame's 70 points are more than 37 units need). */
void FestBackCrest(Canvas& C, const FestivalLook& L, float Cx, float Cy, float Size)
{
	const std::vector<Vec2> Outline = eventart::eventart_detail::FramePts(static_cast<eventart::eventart_detail::Frame>(L.Frame), 1.0f);
	const float R = Size * 0.43f; // the crest's pen (UI/EventArt.cpp's FestCrestLow: Size x 0.43)
	std::vector<Vec2> Pts;
	for (const Vec2& P : Outline)
	{
		const Vec2 Q{Cx + P.X * R, Cy + P.Y * R};
		if (Pts.empty() || std::hypot(Q.X - Pts.back().X, Q.Y - Pts.back().Y) >= 2.5f)
		{
			Pts.push_back(Q);
		}
	}
	C.FillPolygon(Pts, Hex(L.Accent2));
}

/**
 * A festival's card back at the origin, W x H (§2.7.4): its host's back pattern (UI/Ui.cpp's TblBack and CardBack) in the look's Accent
 * and Accent2, with the crest at its centre where the host's own mark sits (FestBackCrest: its frame's silhouette). RiverLine's: the
 * light border round a panel in the accents darkened (a rival deck's rule) with its hatching; Ignite's Sunset: the cream border round the
 * look's felt, the rays and the band pair (Accent, Accent2) under a disc; RunItTwice's Twin Stripe: the gloss-black ground, the wide
 * stripe in Accent (Signal Red), the narrow one and the inset border in Accent2, under a disc. A small back (58 units or less, TblBack's
 * rule) drops the hatching and the rays.
 */
void FestBackPaint(Canvas& C, const FestivalLook& L, float W, float H)
{
	const float Rad = W * 0.1f;
	const bool Lod = W <= 58.0f;
	const Rect In{W * 0.07f, W * 0.07f, W - W * 0.14f, H - W * 0.14f};
	const Vec2 Mid{W * 0.5f, H * 0.5f};
	const net::SiteId Host = FestHostOf(L.Which);
	if (Host == net::SiteId::Ignite)
	{
		C.FillRoundRect({0.0f, 0.0f, W, H}, Rad, Hex(FestBackCream));
		C.FillRoundRect(In, W * 0.06f, Paint::Linear({In.X, In.Y}, {In.X + In.W, In.Y + In.H}, Hex(L.Felt), Hex(L.FeltDark)));
		if (!Lod)
		{
			const float Reach = In.W + In.H;
			for (int K = 0; K < 12; K += 2)
			{
				const float A = static_cast<float>(K) * 0.5235988f;
				const std::vector<Vec2> Ray = FestClipToRect({Mid, {Mid.X + Reach * std::cos(A - 0.09f), Mid.Y + Reach * std::sin(A - 0.09f)},
					{Mid.X + Reach * std::cos(A + 0.09f), Mid.Y + Reach * std::sin(A + 0.09f)}}, In);
				if (Ray.size() >= 3)
				{
					C.FillPolygon(Ray, Hex(FestBackCream, 0.14f));
				}
			}
		}
		const float Bw = In.W * 0.22f;
		const float Lean = In.H * 0.55f;
		const std::vector<Vec2> First = FestClipToRect({{Mid.X - Bw * 1.1f - Lean, In.Y + In.H}, {Mid.X - Bw * 0.1f - Lean, In.Y + In.H}, {Mid.X - Bw * 0.1f + Lean, In.Y},
			{Mid.X - Bw * 1.1f + Lean, In.Y}}, In);
		const std::vector<Vec2> Second = FestClipToRect({{Mid.X + Bw * 0.1f - Lean, In.Y + In.H}, {Mid.X + Bw * 0.7f - Lean, In.Y + In.H}, {Mid.X + Bw * 0.7f + Lean, In.Y},
			{Mid.X + Bw * 0.1f + Lean, In.Y}}, In);
		if (First.size() >= 3)
		{
			C.FillPolygon(First, Hex(L.Accent));
		}
		if (Second.size() >= 3)
		{
			C.FillPolygon(Second, Hex(L.Accent2));
		}
		C.FillCircle(Mid.X, Mid.Y, W * 0.25f, Hex(L.FeltDark));
	}
	else if (Host == net::SiteId::RunItTwice)
	{
		C.FillRoundRect({0.0f, 0.0f, W, H}, Rad, Paint::Linear({0.0f, 0.0f}, {W, H}, Hex(FestBackGloss), Hex(FestBackOnyx)));
		C.FillRect({W * 0.30f, 0.5f, W * 0.16f, H - 1.0f}, Hex(L.Accent));
		C.FillRect({W * 0.50f, 0.5f, W * 0.06f, H - 1.0f}, Hex(L.Accent2));
		C.FillCircle(Mid.X, Mid.Y, W * 0.25f, Hex(FestBackOnyx));
		C.StrokeRoundRect({W * 0.04f, W * 0.04f, W - W * 0.08f, H - W * 0.08f}, Rad * 0.6f, Hex(L.Accent2, 0.8f), 1.0f);
	}
	else
	{
		C.FillRoundRect({0.0f, 0.0f, W, H}, Rad, Hex(FestBackRiverRim));
		C.FillRoundRect(In, W * 0.06f, Paint::Linear({0.0f, 0.0f}, {W, H}, Mix(Hex(L.Accent), Hex(0x000000), 0.5f), Mix(Hex(L.Accent2), Hex(0x000000), 0.62f)));
		if (!Lod)
		{
			// The hatching, every 12 units down to the right, each line cut to the panel (its two ends where it crosses In).
			for (float I = In.X - In.H + 6.0f; I < In.X + In.W; I += 12.0f)
			{
				const float X0 = std::max(I, In.X);
				const float X1 = std::min(I + In.H, In.X + In.W);
				if (X1 - X0 > 1.0f)
				{
					C.StrokePolyline({{X0, In.Y + (X0 - I)}, {X1, In.Y + (X1 - I)}}, false, Hex(0xffffff, 0.12f), 2.0f);
				}
			}
		}
	}
	FestBackCrest(C, L, Mid.X, Mid.Y, W * 0.44f);
}
} // namespace festart_detail
} // namespace festart
} // namespace ui

namespace fest
{
// Step A-2 (LEAD_CALLS Q-5; moved here from G/Festival.cpp by the integrator): the words the festival art prints, empty in Phase A.
// G1 FL-5 (F-look): each entry with its kind (§2.11: label, sentence, ...), as FestCopyUi's art half reads them.
// G1 FL-9 (F-look): U39, the empty plinth's brass plate ("RCOP · not yet"): the game module's shelf finds it by its {SHORT} lead and
// its "not yet", and fills {SHORT}.
std::vector<std::string> FestArtInventory()
{
	return {std::string("label\t") + ui::festart::festart_detail::FestHubSign(), std::string("label\t{SHORT} \xC2\xB7 not yet")};
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
	// QF-3 (accepted): the brand from the series id's slot ("rcop27" -> "rcop"; "ignite.feign26" -> "feign": SlotOf strips the site key,
	// FL-2), so a look needs no F-net data and "SlotOf unstripped" fails FestCrestLowDetail. A plain series' slot is no brand key.
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

// FestTrophy, FestBelt and FestPin: UI/EventArt.cpp (G1 FL-3).

void FestWordMark(Canvas& C, const FestivalLook& L, const std::string& Text, float X, float Y, float Size, double Time, bool Still)
{
	// G1 FL-7: the overload without a day draws PUSH's gauge full (a word-mark shown outside a festival day reads finished, not empty).
	FestWordMark(C, L, Text, X, Y, Size, Time, 1.0f, Still);
}

void FeltMark(Canvas& C, const FestivalLook& L, float Cx, float Cy, float Size, bool Suppressed)
{
	// G1 FL-4 (§2.7.4): the look's low-detail crest, Size across, centred at (Cx, Cy), at alpha 0.14 of the canvas's own; one cached
	// fragment a look and size (drawn opaque, the alpha applied as it replays). Suppressed (G3's B-ui: INTRODUCTIONS or a lower third on
	// RunItTwice) draws nothing.
	namespace fd = festart_detail;
	if (Suppressed || L.Which == fest::Brand::None || !(Size > 0.0f))
	{
		return;
	}
	const float Was = C.GetAlpha();
	C.SetAlpha(Was * fd::FestFeltMarkAlpha);
	C.Cached(fd::FestFeltMarkKey(L, Size), Cx, Cy, [&L, Size](Canvas& At) { fd::FestLowCrest(At, L, 0.0f, 0.0f, Size); });
	C.SetAlpha(Was);
}

// G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): FestCardBack and DevLook have F-look's bodies from FL-4, FestBanner and
// the FestWordMark overload from FL-7 (below).

void FestCardBack(Canvas& C, int Back, float X, float Y, float W, float H)
{
	// G1 FL-4 (§2.7.4): Back = kBackFestival + the brand; anything else above kBackFestival draws nothing. One cached fragment a look and
	// size (TblBack's caller caches the back by id and size too, so a table replays it either way).
	namespace fd = festart_detail;
	const int Brand = Back - kBackFestival;
	if (Brand <= static_cast<int>(fest::Brand::None) || Brand >= static_cast<int>(fest::Brand::Count) || !(W > 0.0f) || !(H > 0.0f))
	{
		return;
	}
	const FestivalLook& L = LookOf(static_cast<fest::Brand>(Brand));
	C.Cached(fd::FestCardBackKey(L, W, H), X, Y, [&L, W, H](Canvas& At) { fd::FestBackPaint(At, L, W, H); });
}

void DevLook(fest::Brand B)
{
	// G1 FL-4 (§2.16): ss.Fest.Look's brand; Brand::None (or anything out of range) turns it off.
	const bool Real = B > fest::Brand::None && B < fest::Brand::Count;
	festart_detail::FestDevSlot().store(static_cast<uint8_t>(Real ? B : fest::Brand::None), std::memory_order_relaxed);
}

fest::Brand DevLook()
{
	return static_cast<fest::Brand>(festart_detail::FestDevSlot().load(std::memory_order_relaxed));
}

void FestBannerClear(Canvas& C, const FestivalLook& L, const Rect& R, double SinceOpen, float DayProgress, bool MainDay, bool Still, const std::vector<Rect>& Clear);

void FestBanner(Canvas& C, const FestivalLook& L, const Rect& R, double SinceOpen, float DayProgress, bool MainDay, bool Still)
{
	FestBannerClear(C, L, R, SinceOpen, DayProgress, MainDay, Still, {});
}

/** g1p-hub: the waves as FestBanner draws them for a minute of the day's progress and a drop through the rise, in the banner's own
 *  units (Clear: boxes in canvas units, moved by the banner's origin R.X, R.Y); Kinds[i] is piece i's line, 0 the Crest Silver one.
 *  u4_ui's FestWaveClear checks that no piece enters a box at any minute or drop. */
std::vector<std::vector<Vec2>> FestWaveLines(const FestivalLook& L, const Rect& R, float Day, float Drop, const std::vector<Rect>& Clear, std::vector<int>& Kinds)
{
	std::vector<Rect> Local;
	for (const Rect& Q : Clear)
	{
		Local.push_back({Q.X - R.X, Q.Y - R.Y, Q.W, Q.H});
	}
	return festart_detail::FestWaveLinesIn(L, R.W, R.H, Day, Drop, Local, Kinds);
}

/** g1p-hub: FestWordMark's set-out width for Text at Size (its caps and tracking; per glyph when it moves letter by letter). */
float FestWordWidth(const Canvas& C, const FestivalLook& L, const std::string& Text, float Size)
{
	namespace fd = festart_detail;
	return fd::FestWordLay(C, L, L.Caps ? fd::FestUpper(Text) : Text, Size).Width;
}

/** g1p-hub (PIE: RCOP's waves through the hub's tagline): FestBanner whose waves pass behind Clear (canvas units: the header's text
 *  boxes, its flap row, the Main card). Every other layer is FestBanner's own; with no boxes the draw is FestBanner's, byte for byte. */
void FestBannerClear(Canvas& C, const FestivalLook& L, const Rect& R, double SinceOpen, float DayProgress, bool MainDay, bool Still, const std::vector<Rect>& Clear)
{
	// G1 FL-7 (§2.8.2's banner ground, §2.7.2, §2.7.7): F-hubs draws the crest, the word-mark and the stats over it. Settled layers are
	// cached fragments (the ground once; RCOP's waves per world minute; the curtains and the door-light once); a motion still running
	// draws live; Still draws every layer settled whatever SinceOpen says.
	namespace fd = festart_detail;
	if (L.Which == fest::Brand::None || !(R.W > 0.0f) || !(R.H > 0.0f))
	{
		return;
	}
	const double T = Still ? fd::FestBannerSettle(L) : SinceOpen;
	const float W = R.W;
	const float H = R.H;
	C.Cached(fd::FestBannerKey(L, R, fd::FestLayerGround, 0), R.X, R.Y, [&](Canvas& At) { fd::FestBannerGround(At, L, W, H); });
	if (L.Move == Motion::WaveRise)
	{
		const int Minute = fd::FestMinuteOf(DayProgress);
		const float Day = static_cast<float>(Minute) / static_cast<float>(fd::FestDayMinutes);
		const double Risen = motion::CountUp(0.0, 1.0, T, 0.0, fd::FestWaveSeconds);
		std::vector<Rect> Local;
		for (const Rect& Q : Clear)
		{
			Local.push_back({Q.X - R.X, Q.Y - R.Y, Q.W, Q.H});
		}
		if (Risen >= 1.0)
		{
			C.Cached(fd::FestBannerKeyClear(L, R, fd::FestLayerWave, Minute, Local), R.X, R.Y, [&](Canvas& At) { fd::FestBannerWave(At, L, W, H, Day, 0.0f, Local); });
		}
		else
		{
			C.PushClip(R);
			fd::FestLive(C, R.X, R.Y, 1.0f, [&](Canvas& At) { fd::FestBannerWave(At, L, W, H, Day, static_cast<float>((1.0 - Risen) * H * 0.6), Local); });
			C.PopClip();
		}
	}
	if (L.Move == Motion::RaysTurn)
	{
		C.PushClip(R);
		fd::FestLive(C, R.X, R.Y, 1.0f, [&](Canvas& At) { fd::FestBannerRays(At, L, W, H, T); });
		C.PopClip();
	}
	if (L.Move == Motion::Curtains)
	{
		const double Part = motion::CountUp(0.0, 1.0, T, 0.0, fd::FestCurtainSeconds);
		if (Part >= 1.0)
		{
			C.Cached(fd::FestBannerKey(L, R, fd::FestLayerCurtain, 0), R.X, R.Y, [&](Canvas& At) { fd::FestBannerCurtains(At, L, W, H, 1.0f); });
		}
		else
		{
			fd::FestLive(C, R.X, R.Y, 1.0f, [&](Canvas& At) { fd::FestBannerCurtains(At, L, W, H, static_cast<float>(Part)); });
		}
	}
	if (L.Which == fest::Brand::Reign && MainDay)
	{
		const double Up = motion::CountUp(0.0, 1.0, T, 0.0, fd::FestDoorSeconds);
		if (Up >= 1.0)
		{
			C.Cached(fd::FestBannerKey(L, R, fd::FestLayerDoor, 0), R.X, R.Y, [&](Canvas& At) { fd::FestBannerDoor(At, L, H); });
		}
		else
		{
			fd::FestLive(C, R.X, R.Y, static_cast<float>(Up), [&](Canvas& At) { fd::FestBannerDoor(At, L, H); });
		}
	}
	// The rim, over every layer, in the look's accent.
	C.StrokePolyline(fd::FestBannerOutline(L, R), true, Hex(L.Accent, 0.45f), 1.2f);
}

void FestWordMark(Canvas& C, const FestivalLook& L, const std::string& Text, float X, float Y, float Size, double Time, float DayProgress, bool Still)
{
	// G1 FL-7 (§2.7.1, §2.7.2's word-mark column, §2.7.7): X the left edge, Y the baseline. The settled mark is one cached fragment (PUSH's
	// per world minute of the day's progress); WRIT's flap and PUSH's fill draw live until they settle, FEIGN's bob always (its still: 1.2 s).
	namespace fd = festart_detail;
	if (L.Which == fest::Brand::None || Text.empty() || !(Size > 0.0f))
	{
		return;
	}
	const std::string Shown = L.Caps ? fd::FestUpper(Text) : Text;
	const double Settle = fd::FestWordSettle(L, Shown);
	const double T = Still ? Settle : Time;
	const int Minute = L.Move == Motion::Gauge ? fd::FestMinuteOf(DayProgress) : 0;
	const float Day = static_cast<float>(Minute) / static_cast<float>(fd::FestDayMinutes);
	if (L.Move == Motion::Bob || ((L.Move == Motion::Flap || L.Move == Motion::Gauge) && T < Settle))
	{
		const float Fill = L.Move == Motion::Gauge ? static_cast<float>(motion::CountUp(0.0, Day, T, 0.0, fd::FestGaugeSeconds)) : Day;
		fd::FestWordPaint(C, L, Shown, X, Y, Size, T, Fill, true);
		return;
	}
	C.Cached(fd::FestWordMarkKey(L, Shown, Size, Minute), X, Y, [&](Canvas& At) { fd::FestWordPaint(At, L, Shown, 0.0f, 0.0f, Size, Settle, Day, false); });
}
} // namespace festart
} // namespace ui
} // namespace ss
