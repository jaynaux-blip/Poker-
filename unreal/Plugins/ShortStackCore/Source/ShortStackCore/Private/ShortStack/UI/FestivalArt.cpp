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
#include "ShortStack/UI/FestivalArt.h"

#include "RitArt.h"
#include "RiverLineShared.h"
#include "ShortStack/Common.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

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

/** RCOP's waves (WaveRise, §2.7.2): five lines across the banner, the top one in Crest Silver, at the day's mark (Day 0..1: the line
 *  climbs through the day), Drop units lower while they rise; clear of the corners. */
void FestBannerWave(Canvas& C, const FestivalLook& L, float W, float H, float Day, float Drop)
{
	const float In = FestBannerInset(L);
	const float Low = H - In - 4.0f * 12.0f - 8.0f - 2.0f;
	const float High = H * 0.3f;
	const float Y0 = Low + (High - Low) * Day + Drop;
	const float Step = 24.0f;
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
		C.StrokePolyline(Line, false, K == 0 ? Hex(L.Accent2, 0.5f) : Hex(L.Accent, 0.16f), K == 0 ? 2.0f : 1.4f);
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
} // namespace festart_detail
} // namespace festart
} // namespace ui

namespace fest
{
// Step A-2 (LEAD_CALLS Q-5; moved here from G/Festival.cpp by the integrator): the words the festival art prints, empty in Phase A.
// G1 FL-5 (F-look): each entry with its kind (§2.11: label, sentence, ...), as FestCopyUi's art half reads them.
std::vector<std::string> FestArtInventory()
{
	return {std::string("label\t") + ui::festart::festart_detail::FestHubSign()};
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

void FeltMark(Canvas& /*C*/, const FestivalLook& /*L*/, float /*Cx*/, float /*Cy*/, float /*Size*/, bool /*Suppressed*/)
{
}

// G1's appends (the lead's pre-wave commit H0, PLAN_G1 §4.2): stubs until F-look's bodies (FestCardBack and DevLook at FL-4,
// FestBanner and the FestWordMark overload at FL-7, below). No back today's tables draw reaches kBackFestival (the highest is
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

void FestBanner(Canvas& C, const FestivalLook& L, const Rect& R, double SinceOpen, float DayProgress, bool MainDay, bool Still)
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
		if (Risen >= 1.0)
		{
			C.Cached(fd::FestBannerKey(L, R, fd::FestLayerWave, Minute), R.X, R.Y, [&](Canvas& At) { fd::FestBannerWave(At, L, W, H, Day, 0.0f); });
		}
		else
		{
			C.PushClip(R);
			fd::FestLive(C, R.X, R.Y, 1.0f, [&](Canvas& At) { fd::FestBannerWave(At, L, W, H, Day, static_cast<float>((1.0 - Risen) * H * 0.6)); });
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
