// Update 3 (SPEC §6.1, §7.3, §10): Rerun 2.0's thumbnail compositor, ui-thumbs' file. A recipe (video::ThumbSpec, which
// core-videocontent's generator writes) drawn at any size: the archetype's composition in a 640 x 360 design space scaled
// into the tile, the background, the palette, the person (DrawPortrait with an expression, a pose and a cut-out halo),
// the props, the caption in its style, a sticker and a corner mark. Every digit drawn is a fact's: the captions (the
// generator fills them from facts, U7) and the cards the video's own Thumb carries; the compositor's own words are in
// Words() and carry none. Cached in Rerun's pool (Draw: replayed when Canvas::IsCached says so, else painted while the
// frame's Budget allows, else a skeleton). The LOD (tiles under LodWidth) drops hair strands, stipple, wrinkles, glows and
// half the outline runs. Vector only: no image is kept.
#include "ShortStack/UI/ThumbArt.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/StreamArt.h"
#include "ShortStack/UI/Ui.h"
#include "ShortStack/UI/VideoArt.h"
#include "ThumbPortrait.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace thumbart
{
namespace thumbart_detail
{
// The vocabulary is core-videocontent's (video::thumb, Game/VideoContent.h): thumbart's enums mirror it value for value.
namespace vt = video::thumb;
static_assert(static_cast<int>(Arch::Count) == vt::ArCount && static_cast<int>(Arch::Pov) == vt::ArPov && static_cast<int>(Arch::Mistake) == vt::ArMistake, "Arch mirrors video::thumb::ArchId");
static_assert(static_cast<int>(Layout::Count) == vt::LyCount && static_cast<int>(Bg::Count) == vt::BgCount && static_cast<int>(Bg::Scanlines) == vt::BgScanlines, "Layout and Bg mirror video::thumb");
static_assert(static_cast<int>(Pal::Site) == vt::PlSite && static_cast<int>(Pal::Brand) == vt::PlCount, "Pal mirrors video::thumb::PalId (Brand, P1, after it)");
static_assert(static_cast<int>(Subject::Count) == vt::SjCount && static_cast<int>(Subject::Object) == vt::SjObject, "Subject mirrors video::thumb::SubjId");
static_assert(static_cast<int>(CapStyle::Count) == vt::CsCount && static_cast<int>(CapStyle::None) == vt::CsNone, "CapStyle mirrors video::thumb::CapStyleId");
static_assert(static_cast<int>(Sticker::Count) == vt::StCount && static_cast<int>(Sticker::Bolt) == vt::StBolt && static_cast<int>(Sticker::Ghost) == vt::StGhost, "Sticker mirrors video::thumb::StickerId");
static_assert(static_cast<int>(Corner::Count) == vt::CnCount && static_cast<int>(Corner::None) == vt::CnNone, "Corner mirrors video::thumb::CornerId");
static_assert(static_cast<int>(Prop::Count) == vt::PrCount && static_cast<int>(Prop::Gear) == vt::PrGear && static_cast<int>(Prop::FeltTop) == vt::PrFeltTop &&
				  static_cast<int>(Prop::ProgressBar) == vt::PrProgressBar && static_cast<int>(Prop::FuseReel) == vt::PrFuseReel && static_cast<int>(Prop::CardBacks) == vt::PrCardBacks,
	"Prop mirrors video::thumb::PropId");
static_assert(vt::FactSep == '\x1f', "the facts block's separator");
static_assert(static_cast<int>(PortraitPose::FistPump) + 1 == vt::PsCount && vt::ExCount == 12, "Pose and Expr mirror Portrait.h");

using Pts = std::vector<Vec2>;
constexpr float DesignW = 640.0f;
constexpr float DesignH = 360.0f;
const uint32_t HotRed = 0xff2a2a;
const uint32_t Yellow = 0xffd23f;

// ------------------------------------------------------------------ small things

Color WithA(const Color& C, float A)
{
	return Color{C.R, C.G, C.B, C.A * A};
}

/** A deterministic 0..1 from the recipe's variant seed and a salt. */
float Hash01(uint32_t Seed, uint32_t Salt)
{
	uint32_t X = Seed ^ (Salt * 0x9E3779B1u + 0x7f4a7c15u);
	X ^= X >> 16;
	X *= 0x7feb352du;
	X ^= X >> 15;
	X *= 0x846ca68bu;
	X ^= X >> 16;
	return static_cast<float>(X & 0xffffffu) / 16777216.0f;
}

float Cross2(Vec2 O, Vec2 A, Vec2 B)
{
	return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
}

/** The convex hull (Andrew's monotone chain). */
Pts Hull(Pts P)
{
	std::sort(P.begin(), P.end(), [](const Vec2& A, const Vec2& B) { return A.X < B.X || (A.X == B.X && A.Y < B.Y); });
	if (P.size() < 3)
	{
		return P;
	}
	Pts H(P.size() * 2);
	size_t K = 0;
	for (size_t I = 0; I < P.size(); ++I)
	{
		while (K >= 2 && Cross2(H[K - 2], H[K - 1], P[I]) <= 0.0f)
		{
			--K;
		}
		H[K++] = P[I];
	}
	for (size_t I = P.size() - 1, T = K + 1; I-- > 0;)
	{
		while (K >= T && Cross2(H[K - 2], H[K - 1], P[I]) <= 0.0f)
		{
			--K;
		}
		H[K++] = P[I];
	}
	H.resize(K - 1);
	return H;
}

/** A convex piece grown by D (its Minkowski sum with an octagon): the halo's outline around the cut-out. */
Pts Grow(const Pts& P, float D)
{
	Pts Q;
	Q.reserve(P.size() * 8);
	for (const Vec2& V : P)
	{
		for (int K = 0; K < 8; ++K)
		{
			const float A = static_cast<float>(K) * Pi * 0.25f + Pi * 0.125f;
			Q.push_back({V.X + D * std::cos(A), V.Y + D * std::sin(A)});
		}
	}
	return Hull(Q);
}

/** Grow for a small tile: the piece's hull thinned to at most Keep corners first (S-T: the halo's share of a tile). */
Pts GrowLean(const Pts& P, float D, size_t Keep)
{
	const Pts H = Hull(P);
	if (H.size() <= Keep)
	{
		return Grow(H, D);
	}
	Pts Thin;
	for (size_t I = 0; I < Keep; ++I)
	{
		Thin.push_back(H[I * H.size() / Keep]);
	}
	return Grow(Thin, D * 1.15f);
}

/** A star (the number burst, a burst sticker): N points between R and R2. */
Pts Star(float Cx, float Cy, float R, float R2, int N, float Rot)
{
	Pts P;
	for (int I = 0; I < N * 2; ++I)
	{
		const float A = Rot + Pi * static_cast<float>(I) / static_cast<float>(N);
		const float Rr = I % 2 == 0 ? R : R2;
		P.push_back({Cx + Rr * std::cos(A), Cy + Rr * std::sin(A) * 0.86f});
	}
	return P;
}

Pts Quad(Vec2 A, Vec2 B, Vec2 C, Vec2 D)
{
	return {A, B, C, D};
}

// ------------------------------------------------------------------ the palettes (§6.1: 11 in P0, Brand in P1)

struct Palette
{
	Color A, B;   // the ground's gradient
	Color Acc;    // the accent: captions' first line, plates, glows
	Color Ink;    // words on the ground
	Color Halo;   // the cut-out's outline
	Color Dark;   // ink on a light plate
};

Palette Pal3(uint32_t A, uint32_t B, uint32_t Acc)
{
	Palette P;
	P.A = Hex(A);
	P.B = Hex(B);
	P.Acc = Hex(Acc);
	P.Ink = Hex(0xffffff);
	P.Halo = Hex(0xffffff);
	P.Dark = Mix(Hex(B), Hex(0x000000), 0.55f);
	return P;
}

Palette PaletteOf(int Which, int Site, uint32_t Variant)
{
	switch (static_cast<Pal>(Which))
	{
	case Pal::Felt: return Pal3(0x138a5a, 0x05301f, 0xf2d16b);
	case Pal::Money: return Pal3(0x21a35a, 0x0a3a1f, 0xffd479);
	case Pal::Ice:
	{
		Palette P = Pal3(0x49c4ff, 0x0a2748, 0xffffff);
		P.Dark = Hex(0x0a2748);
		return P;
	}
	case Pal::Neon:
	{
		// Magenta and cyan: clear of Kast's violet and lime.
		Palette P = Pal3(0xff2bd6, 0x25075a, 0x2bfff0);
		P.Halo = Hex(0x2bfff0);
		return P;
	}
	case Pal::GoldOnBlack:
	{
		Palette P = Pal3(0x2c2416, 0x050505, 0xf5c542);
		P.Halo = Hex(0xf5c542);
		return P;
	}
	case Pal::Sunrise: return Pal3(0xffb03a, 0xff3d77, 0xfff1a8);
	case Pal::Night: return Pal3(0x1d2b66, 0x060a1c, 0xffd479);
	case Pal::Mono:
	{
		// Mono and one accent, the seed's: marigold, red, cyan or lime.
		static const uint32_t Accents[4] = {0xf2a33a, 0xe0283c, 0x2fd3ff, 0xb6ff3a};
		return Pal3(0x4a4a4a, 0x101010, Accents[(Variant >> 8) & 3u]);
	}
	case Pal::Blood:
	{
		Palette P = Pal3(0xd0142a, 0x2a0006, 0xffffff);
		P.Dark = Hex(0x2a0006);
		return P;
	}
	case Pal::Site:
	{
		// The hand's site in its own colours (its 16 tokens: Bg 0, Accent 8, Gold 10).
		const net::SiteId S = Site > 0 && Site < static_cast<int>(net::SiteId::Count) ? static_cast<net::SiteId>(Site) : net::SiteId::RiverLine;
		const net::SiteDef& D = net::Def(S);
		Palette P;
		P.A = Mix(Hex(D.Palette[8]), Hex(D.Palette[0]), 0.25f);
		P.B = Mix(Hex(D.Palette[0]), Hex(0x000000), 0.3f);
		P.Acc = Hex(D.Palette[10] != 0 ? D.Palette[10] : 0xffd23f);
		P.Ink = Hex(0xffffff);
		P.Halo = Hex(0xffffff);
		P.Dark = Mix(Hex(D.Palette[0]), Hex(0x000000), 0.5f);
		return P;
	}
	case Pal::Brand:
	{
		// P1's brand kit: Rerun's own marigold on ink until it lands.
		Palette P = Pal3(videoart::Marigold, videoart::Ink, videoart::Cream);
		P.Ink = Hex(videoart::Ink);
		P.Dark = Hex(videoart::Ink);
		return P;
	}
	default: return Pal3(0xff4a1c, 0x6e0610, 0xffd23f); // Heat
	}
}

// ------------------------------------------------------------------ the recipe as it draws

struct Ctx
{
	Canvas* C = nullptr;
	const video::ThumbSpec* S = nullptr;
	const hero::Character* Look = nullptr;
	const video::Thumb* F = nullptr;
	bool Lod = false;
	// S-T's three tiers: 0 the hero (640 x 360 and anything 400 wide or more: the full portrait), 1 a tile (the coarse
	// portrait, cheap glows, a 4-way outline), 2 the LOD row (the lightest of everything, a 1-way outline).
	int Tier = 0;
	bool Mirror = false;
	bool Tight = false; // the zoom: tight draws the person bigger
	Palette P;
	std::vector<Card> Mine, Theirs, Board;
	uint32_t V = 0;
	std::string Cap1, Cap2; // the caption's two lines (Caption2 without its facts block)
	// The facts (video::thumb's keys; VideoContent.h): from Caption2's facts block, else the Thumb Compose was given.
	std::string Other;                  // o: the other face's name (empty: nobody)
	int Site = 0;                       // s
	int Marked = -1;                    // k: the board card the archetype marks
	std::string Big, From, To, Event, Mult, Graph; // n, f, t, e, m, g
	int Progress = -1;                  // p: 0-100
	std::vector<std::string> Chat, Rows; // h (two at most), r
	std::vector<std::string> Gear;      // d: the gear the player owns (gear::Item ids; C3 fix L7, Desk Setup)
};

float Mx(const Ctx& X, float V)
{
	return X.Mirror ? DesignW - V : V;
}

Align Ma(const Ctx& X, Align A)
{
	return !X.Mirror ? A : A == Align::Left ? Align::Right : A == Align::Right ? Align::Left : A;
}

/** A box's left edge after the mirror. */
float Mleft(const Ctx& X, float Left, float W)
{
	return X.Mirror ? DesignW - Left - W : Left;
}

float Hf(const Ctx& X, uint32_t Salt)
{
	return Hash01(X.V, Salt);
}

/** How many of a background's or a prop's repeated pieces the tier draws (hero, tile, row). */
int Many(const Ctx& X, int Hero, int Tile, int Row)
{
	return X.Tier == 0 ? Hero : X.Tier == 1 ? Tile : Row;
}

/** The zoom's scale on a person (tight: bigger, wide: smaller). */
float Zoom(const Ctx& X, float S)
{
	return S * (X.Tight ? 1.1f : 0.94f);
}

/** "mine/theirs/board" (theirs only if turned up). */
void SplitCards(const std::string& Field, std::vector<Card>& Mine, std::vector<Card>& Theirs, std::vector<Card>& Board)
{
	std::vector<std::string> Parts(1);
	for (const char Ch : Field)
	{
		if (Ch == '/')
		{
			Parts.emplace_back();
		}
		else
		{
			Parts.back().push_back(Ch);
		}
	}
	// Only real cards (a field that doesn't parse draws backs, never a wrong card).
	auto Valid = [](std::vector<Card> V) {
		V.erase(std::remove_if(V.begin(), V.end(), [](Card C) { return C < 0 || C > 51; }), V.end());
		return V;
	};
	Mine = Valid(ParseCards(Parts[0]));
	Theirs = Parts.size() > 1 ? Valid(ParseCards(Parts[1])) : std::vector<Card>();
	Board = Parts.size() > 2 ? Valid(ParseCards(Parts[2])) : std::vector<Card>();
}

/**
 * The facts a recipe draws (video::thumb's contract, VideoContent.h): Caption2's own line, then FactSep-separated
 * "key=value" pairs. A fact the block carries wins; one it leaves out comes from the Thumb (Update 2's cards, opponent and
 * site: video::ThumbFor never names anyone in storytime, and a world video's Thumb is empty), so Studio's options, a world
 * video and an old-style spec all draw true things.
 */
void ReadFacts(Ctx& X, const video::ThumbSpec& S, const video::Thumb& T)
{
	const char Sep = '\x1f';
	const size_t At = S.Caption2.find(Sep);
	X.Cap2 = At == std::string::npos ? S.Caption2 : S.Caption2.substr(0, At);
	std::string Cards = T.Cards;
	X.Other = T.Opponent;
	X.Site = T.Site;
	if (At != std::string::npos)
	{
		size_t P = At;
		while (P != std::string::npos)
		{
			const size_t Next = S.Caption2.find(Sep, P + 1);
			const std::string Kv = S.Caption2.substr(P + 1, Next == std::string::npos ? std::string::npos : Next - P - 1);
			P = Next;
			const size_t Eq = Kv.find('=');
			if (Eq == std::string::npos || Eq == 0)
			{
				continue;
			}
			const std::string Key = Kv.substr(0, Eq), Val = Kv.substr(Eq + 1);
			auto Int = [&Val](int Lo, int Hi, int Bad) {
				int N = 0;
				bool Any = false;
				for (const char Ch : Val)
				{
					if (Ch < '0' || Ch > '9' || N > 100000)
					{
						return Bad;
					}
					N = N * 10 + (Ch - '0');
					Any = true;
				}
				return Any && N >= Lo && N <= Hi ? N : Bad;
			};
			if (Key == "c")
			{
				Cards = Val;
			}
			else if (Key == "k")
			{
				X.Marked = Int(0, 4, -1);
			}
			else if (Key == "o")
			{
				X.Other = Val;
			}
			else if (Key == "n")
			{
				X.Big = Val;
			}
			else if (Key == "f")
			{
				X.From = Val;
			}
			else if (Key == "t")
			{
				X.To = Val;
			}
			else if (Key == "p")
			{
				X.Progress = Int(0, 100, -1);
			}
			else if (Key == "g")
			{
				X.Graph = Val;
			}
			else if (Key == "s")
			{
				X.Site = Int(0, 99, 0);
			}
			else if (Key == "e")
			{
				X.Event = Val;
			}
			else if (Key == "m")
			{
				X.Mult = Val;
			}
			else if (Key == "h" && X.Chat.size() < 2)
			{
				X.Chat.push_back(Val);
			}
			else if (Key == "r")
			{
				std::string Cur;
				for (const char Ch : Val)
				{
					if (Ch == '|')
					{
						X.Rows.push_back(Cur);
						Cur.clear();
					}
					else
					{
						Cur.push_back(Ch);
					}
				}
				X.Rows.push_back(Cur);
				if (X.Rows.size() > 5)
				{
					X.Rows.resize(5);
				}
			}
			else if (Key == "d")
			{
				// The gear the player owns, '|'-separated gear::Item ids (C3 fix L7: Desk Setup draws exactly these).
				std::string Cur;
				for (const char Ch : Val + "|")
				{
					if (Ch != '|')
					{
						Cur.push_back(Ch);
						continue;
					}
					if (!Cur.empty() && X.Gear.size() < 40)
					{
						X.Gear.push_back(Cur);
					}
					Cur.clear();
				}
			}
		}
	}
	SplitCards(Cards, X.Mine, X.Theirs, X.Board);
}

/** The caption with an arrow in it, split at the arrow ("$11 → $1,190" or "$11 -> $1,190"); false when it has none. */
bool SplitArrow(const std::string& S, std::string& Before, std::string& After)
{
	static const char* const Arrows[3] = {"\xE2\x86\x92", "->", ">"};
	for (const char* A : Arrows)
	{
		const size_t At = S.find(A);
		if (At != std::string::npos)
		{
			Before = S.substr(0, At);
			After = S.substr(At + std::strlen(A));
			auto Trim = [](std::string& T) {
				while (!T.empty() && T.front() == ' ')
				{
					T.erase(T.begin());
				}
				while (!T.empty() && T.back() == ' ')
				{
					T.pop_back();
				}
			};
			Trim(Before);
			Trim(After);
			return !Before.empty() && !After.empty();
		}
	}
	return false;
}

/** The money amounts in a caption ("$50 · $214 · $500" gives 50, 214, 500), in order. */
std::vector<double> Amounts(const std::string& S)
{
	std::vector<double> Out;
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (S[I] != '$')
		{
			continue;
		}
		double V = 0.0;
		double Frac = 0.0;
		bool Any = false;
		size_t J = I + 1;
		for (; J < S.size(); ++J)
		{
			const char Ch = S[J];
			if (Ch >= '0' && Ch <= '9')
			{
				if (Frac > 0.0)
				{
					V += (Ch - '0') * Frac;
					Frac *= 0.1;
				}
				else
				{
					V = V * 10.0 + (Ch - '0');
				}
				Any = true;
			}
			else if (Ch == ',' && Frac == 0.0)
			{
			}
			else if (Ch == '.' && Frac == 0.0)
			{
				Frac = 0.1;
			}
			else
			{
				break;
			}
		}
		if (J < S.size() && (S[J] == 'K' || S[J] == 'k'))
		{
			V *= 1000.0;
		}
		else if (J < S.size() && (S[J] == 'M' || S[J] == 'm'))
		{
			V *= 1000000.0;
		}
		if (Any)
		{
			Out.push_back(V);
		}
		I = J;
	}
	return Out;
}

bool Negative(const std::string& S)
{
	return !S.empty() && (S[0] == '-' || S.compare(0, 3, "\xE2\x88\x92") == 0 || S.find("DOWN") != std::string::npos || S.find("LOST") != std::string::npos);
}

// ------------------------------------------------------------------ text

TextStyle Heavy(float Size, const Color& Col, Align A, bool Italic)
{
	TextStyle T;
	T.Size = Size;
	T.Weight = 900;
	T.Col = Col;
	T.HAlign = A;
	T.VAlign = Baseline::Alphabetic;
	if (Italic)
	{
		T.Face = static_cast<int8_t>(Font::BlackItalic);
	}
	return T;
}

float Wide(Canvas& C, const std::string& S, float Size, bool Italic)
{
	return C.Measure(S, Size, Italic ? Font::BlackItalic : Font::Black);
}

/** The size that fits MaxW, quantised down to 2 units (§6.1). */
float Fit(Canvas& C, const std::string& S, float Size, float MaxW, bool Italic)
{
	float Sz = Size;
	const float W = Wide(C, S, Sz, Italic);
	if (MaxW > 0.0f && W > MaxW)
	{
		Sz = Size * MaxW / W;
	}
	return std::max(10.0f, std::floor(Sz * 0.5f) * 2.0f);
}

/**
 * Big words that read over anything: a drop shadow, the 8-way fake outline, the fill (10 runs a line; a tile draws the 4
 * diagonals, 6 runs; the LOD row one edge, 3 runs).
 */
void Loud(Ctx& X, const std::string& S, float Px, float Base, float Size, const Color& Fill, const Color& Edge, Align A, bool Italic)
{
	if (S.empty())
	{
		return;
	}
	Canvas& C = *X.C;
	TextStyle T = Heavy(Size, Color{0.0f, 0.0f, 0.0f, 0.45f}, A, Italic);
	const float E = std::max(1.5f, Size * 0.075f);
	C.Text(S, Px + Size * 0.05f, Base + Size * 0.07f, T);
	T.Col = Edge;
	static const Vec2 Eight[8] = {{1.0f, 1.0f}, {-1.0f, -1.0f}, {1.0f, -1.0f}, {-1.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f}};
	const int N = Many(X, 8, 4, 1);
	const float Ed = X.Tier == 0 ? E : E * 0.85f;
	for (int K = 0; K < N; ++K)
	{
		const float Dk = K < 4 && X.Tier == 0 ? 0.7071f : 1.0f;
		C.Text(S, Px + Eight[K].X * Ed * Dk, Base + Eight[K].Y * Ed * Dk, T);
	}
	T.Col = Fill;
	C.Text(S, Px, Base, T);
}

/** Plain heavy words (no outline): a plate's ink, a label. */
void Plain(Ctx& X, const std::string& S, float Px, float Base, float Size, const Color& Col, Align A, bool Italic = false)
{
	if (!S.empty())
	{
		X.C->Text(S, Px, Base, Heavy(Size, Col, A, Italic));
	}
}

/** A small label in the UI face (bold, not black). */
void Label(Ctx& X, const std::string& S, float Px, float Base, float Size, const Color& Col, Align A)
{
	TextStyle T;
	T.Size = Size;
	T.Weight = 700;
	T.Col = Col;
	T.HAlign = A;
	X.C->Text(S, Px, Base, T);
}

// ------------------------------------------------------------------ backgrounds (8)

/**
 * A soft round glow: Col at its middle, gone at R. S-T: a radial fill is six rings of vertices (a 640-unit spotlight about
 * 1,100), so glows are GlowRoundRect's core and linear falloff (about a tenth of that) at every tier.
 */
void Glow(Canvas& C, float X, float Y, float R, const Color& Col, float A)
{
	const float Core = R * 0.3f;
	C.GlowRoundRect({X - Core, Y - Core, Core * 2.0f, Core * 2.0f}, Core, WithA(Col, A), R * 0.55f);
}

/** Darkened edges over everything drawn so far: four linear bands (the hero keeps its radial vignette). */
void Vignette(Ctx& X, float Strength)
{
	Canvas& C = *X.C;
	if (X.Tier == 0)
	{
		C.FillCircle(320.0f, 180.0f, 520.0f, Paint::Radial({320.0f, 180.0f}, 0.0f, {320.0f, 180.0f}, 520.0f, Color{0.0f, 0.0f, 0.0f, 0.0f}, 0.62f, Color{0.0f, 0.0f, 0.0f, 0.12f * Strength},
												 Color{0.0f, 0.0f, 0.0f, 0.8f * Strength}));
		return;
	}
	const Color Dark{0.0f, 0.0f, 0.0f, 0.6f * Strength}, Clear{0.0f, 0.0f, 0.0f, 0.0f};
	C.FillRect({0.0f, 0.0f, DesignW, 90.0f}, Paint::Linear({0.0f, 0.0f}, {0.0f, 90.0f}, Dark, Clear));
	C.FillRect({0.0f, 270.0f, DesignW, 90.0f}, Paint::Linear({0.0f, 360.0f}, {0.0f, 270.0f}, Dark, Clear));
	C.FillRect({0.0f, 0.0f, 150.0f, DesignH}, Paint::Linear({0.0f, 0.0f}, {150.0f, 0.0f}, Dark, Clear));
	C.FillRect({490.0f, 0.0f, 150.0f, DesignH}, Paint::Linear({640.0f, 0.0f}, {490.0f, 0.0f}, Dark, Clear));
}

/** The recipe's background, its burst or spotlight on the composition's focus (Fx, Fy before the mirror). */
void Background(Ctx& X, float Fx, float Fy)
{
	Canvas& C = *X.C;
	const Palette& P = X.P;
	const float Cx = Mx(X, Fx);
	C.FillRect({0.0f, 0.0f, DesignW, DesignH}, Paint::Linear({Mx(X, 0.0f), 0.0f}, {Mx(X, DesignW), DesignH}, P.A, P.B));
	switch (static_cast<Bg>(X.S->Bg))
	{
	case Bg::RadialBurst:
	{
		const int N = Many(X, 22, 16, 12);
		const float Rot = Hf(X, 11) * Pi;
		for (int I = 0; I < N; I += 2)
		{
			const float A0 = Rot + 2.0f * Pi * static_cast<float>(I) / static_cast<float>(N);
			const float A1 = Rot + 2.0f * Pi * static_cast<float>(I + 1) / static_cast<float>(N);
			C.FillPolygon({{Cx, Fy}, {Cx + 900.0f * std::cos(A0), Fy + 900.0f * std::sin(A0)}, {Cx + 900.0f * std::cos(A1), Fy + 900.0f * std::sin(A1)}}, Color{1.0f, 1.0f, 1.0f, 0.1f});
		}
		Glow(C, Cx, Fy, 230.0f, P.Acc, 0.3f);
		break;
	}
	case Bg::SpeedLines:
	{
		const int N = Many(X, 26, 16, 8);
		for (int I = 0; I < N; ++I)
		{
			const float Y = Hf(X, 100u + static_cast<uint32_t>(I)) * 420.0f - 30.0f;
			const float Xs = Hf(X, 200u + static_cast<uint32_t>(I)) * 640.0f;
			const float L = 120.0f + Hf(X, 300u + static_cast<uint32_t>(I)) * 260.0f;
			const float Al = 0.05f + Hf(X, 400u + static_cast<uint32_t>(I)) * 0.13f;
			const float Dir = X.Mirror ? -1.0f : 1.0f;
			C.FillPolygon({{Xs, Y}, {Xs + Dir * L, Y - L * 0.25f}, {Xs + Dir * L, Y - L * 0.25f + 3.0f}, {Xs, Y + 5.0f}}, Color{1.0f, 1.0f, 1.0f, Al});
		}
		Glow(C, Cx, Fy, 200.0f, P.Acc, 0.18f);
		break;
	}
	case Bg::FeltBokeh:
	{
		if (X.Tier == 0)
		{
			C.FillCircle(320.0f, 220.0f, 520.0f, Paint::Radial({320.0f, 220.0f}, 0.0f, {320.0f, 220.0f}, 520.0f, Hex(0x1f7a52), 0.5f, Hex(0x0e4430), Hex(0x041a11)));
		}
		else
		{
			C.FillRect({0.0f, 0.0f, DesignW, DesignH}, Paint::Linear({0.0f, 0.0f}, {0.0f, DesignH}, Hex(0x0e4430), Hex(0x041a11)));
			Glow(C, 320.0f, 220.0f, 360.0f, Hex(0x1f7a52), 0.9f);
		}
		const int N = Many(X, 16, 8, 5);
		const Color Cols[3] = {P.Acc, Hex(0xffffff), Hex(0xffd479)};
		for (int I = 0; I < N; ++I)
		{
			const float Bx = Hf(X, 500u + static_cast<uint32_t>(I)) * 640.0f;
			const float By = Hf(X, 600u + static_cast<uint32_t>(I)) * 360.0f;
			const float R = 18.0f + Hf(X, 700u + static_cast<uint32_t>(I)) * 46.0f;
			Glow(C, Bx, By, R, Cols[I % 3], 0.22f);
		}
		break;
	}
	case Bg::MoneyRain:
	{
		const int N = Many(X, 22, 12, 8);
		for (int I = 0; I < N; ++I)
		{
			const float Bx = Hf(X, 800u + static_cast<uint32_t>(I)) * 640.0f;
			const float By = Hf(X, 900u + static_cast<uint32_t>(I)) * 360.0f;
			if (I % 3 == 0)
			{
				C.FillCircle(Bx, By, 12.0f, Hex(0xf5c542, 0.8f));
				C.StrokeEllipse(Bx, By, 12.0f, 12.0f, Hex(0xa07818, 0.8f), 2.0f);
				continue;
			}
			const float A = (Hf(X, 1000u + static_cast<uint32_t>(I)) - 0.5f) * 1.4f;
			const float Cs = std::cos(A), Sn = std::sin(A);
			auto Pt = [&](float U, float W) { return Vec2{Bx + U * Cs - W * Sn, By + U * Sn + W * Cs}; };
			C.FillPolygon({Pt(-26.0f, -12.0f), Pt(26.0f, -12.0f), Pt(26.0f, 12.0f), Pt(-26.0f, 12.0f)}, Hex(0x3fa86a, 0.6f));
			C.StrokePolyline({Pt(-26.0f, -12.0f), Pt(26.0f, -12.0f), Pt(26.0f, 12.0f), Pt(-26.0f, 12.0f)}, true, Hex(0x1f6a3e, 0.6f), 2.0f);
			C.StrokeEllipse(Bx, By, 6.5f, 6.5f, Hex(0x1f6a3e, 0.6f), 2.0f);
		}
		Glow(C, Cx, Fy, 220.0f, P.Acc, 0.16f);
		break;
	}
	case Bg::DiagonalSplit:
	{
		C.FillPolygon({{Mx(X, 0.0f), 0.0f}, {Mx(X, 380.0f), 0.0f}, {Mx(X, 260.0f), 360.0f}, {Mx(X, 0.0f), 360.0f}}, WithA(Mix(P.B, Hex(0x000000), 0.2f), 0.88f));
		C.FillPolygon({{Mx(X, 380.0f), 0.0f}, {Mx(X, 394.0f), 0.0f}, {Mx(X, 274.0f), 360.0f}, {Mx(X, 260.0f), 360.0f}}, WithA(P.Acc, 0.85f));
		break;
	}
	case Bg::Spotlight:
	{
		if (X.Tier == 0)
		{
			C.FillCircle(Cx, Fy, 640.0f, Paint::Radial({Cx, Fy}, 0.0f, {Cx, Fy}, 640.0f, Color{1.0f, 1.0f, 1.0f, 0.3f}, 0.32f, Color{1.0f, 1.0f, 1.0f, 0.04f}, Color{0.0f, 0.0f, 0.0f, 0.7f}));
		}
		else
		{
			Vignette(X, 1.0f);
			Glow(C, Cx, Fy, 300.0f, Hex(0xffffff), 0.3f);
		}
		break;
	}
	case Bg::Scanlines:
	{
		const float Step = static_cast<float>(Many(X, 6, 9, 12));
		for (float Y = 0.0f; Y < DesignH; Y += Step)
		{
			C.FillRect({0.0f, Y, DesignW, 2.0f}, Color{0.0f, 0.0f, 0.0f, 0.2f});
		}
		Glow(C, Cx, Fy, 260.0f, P.Acc, 0.2f);
		break;
	}
	default:
	{
		// The flat gradient: one soft light.
		Glow(C, Mx(X, 448.0f), 126.0f, 384.0f, Hex(0xffffff), 0.14f);
		break;
	}
	}
}

// ------------------------------------------------------------------ people

/**
 * S-T's row tier (a head about 20 pixels tall): the person as flat shapes in the Look's own colours (skin, hair, the
 * jacket, glasses, a hat), the expression in the brows, the eyes and the mouth, the pose's arms; about a tenth of the coarse
 * portrait. Units as DrawPortrait's (eyes on Ey, the head about 210 tall at Sc 1); Px after the mirror.
 */
void MiniPerson(Ctx& X, const hero::Character& Who, float Px, float Ey, float Sc, const PortraitExpression& E, PortraitPose Pose)
{
	Canvas& C = *X.C;
	const hero::Look& L = Who.Appearance;
	const Color Skin = Hex(hero::SkinTone(L.Skin));
	const Color Shade = Mix(Skin, Hex(0x3a1d22), 0.35f);
	const Color Hair = Hex(hero::HairToneAt(L, Who.Age));
	const Color Cloth = Hex(hero::OutfitTone(L.OutfitColor));
	const Color Ink = Mix(Skin, Hex(0x1a0c0c), 0.8f);
	auto P = [&](float U, float V) { return Vec2{Px + U * Sc, Ey + V * Sc}; };
	const float Wide = L.Build == 3 ? 1.12f : L.Build == 0 ? 0.92f : 1.0f;
	const Pts Torso = {P(-190.0f * Wide, 380.0f), P(-180.0f * Wide, 230.0f), P(-130.0f * Wide, 170.0f), P(-40.0f, 140.0f), P(40.0f, 140.0f), P(130.0f * Wide, 170.0f), P(180.0f * Wide, 230.0f),
		P(190.0f * Wide, 380.0f)};
	Pts Head;
	for (int I = 0; I < 14; ++I)
	{
		const float A = 2.0f * Pi * static_cast<float>(I) / 14.0f;
		Head.push_back(P(78.0f * std::cos(A), -8.0f + (A > Pi ? 122.0f : 108.0f) * std::sin(A)));
	}
	const float S = E.LookX < 0.0f ? -1.0f : 1.0f;
	// The arms (sleeves as thick strokes) and where the hands go.
	std::vector<std::pair<Vec2, Vec2>> Arms;
	switch (Pose)
	{
	case PortraitPose::HandsOnHead: Arms = {{P(-150.0f, 200.0f), P(-60.0f, -120.0f)}, {P(150.0f, 200.0f), P(60.0f, -120.0f)}}; break;
	case PortraitPose::Facepalm: Arms = {{P(S * 110.0f, 260.0f), P(S * 10.0f, 0.0f)}}; break;
	case PortraitPose::Point: Arms = {{P(S * 160.0f, 200.0f), P(S * 330.0f, 110.0f)}}; break;
	case PortraitPose::FistPump: Arms = {{P(S * 160.0f, 200.0f), P(S * 170.0f, -100.0f)}}; break;
	default: break;
	}
	// The halo: the head, the torso and the arms grown.
	std::vector<Pts> Pieces = {Grow(Head, 6.0f), Grow(Torso, 6.0f)};
	for (const auto& A : Arms)
	{
		Pieces.push_back(Grow({A.first, A.second}, 26.0f * Sc + 6.0f));
	}
	for (const Pts& G : Pieces)
	{
		C.FillPolygon(G, X.P.Halo);
	}
	const int Style = L.Hair;
	if (Style == 7 || Style == 8 || Style == 10 || Style == 11)
	{
		// Hair behind the head: an afro's cloud, long hair, braids, a bob.
		if (Style == 7)
		{
			C.FillEllipse(Px, Ey - 50.0f * Sc, 140.0f * Sc, 130.0f * Sc, Hair);
		}
		else
		{
			const float Low = Style == 11 ? 70.0f : 190.0f;
			C.FillRoundRect({Px - 104.0f * Sc, Ey - 100.0f * Sc, 208.0f * Sc, (Low + 100.0f) * Sc}, 60.0f * Sc, Hair);
		}
	}
	C.FillPolygon(Torso, Paint::Linear(P(-190.0f, 0.0f), P(190.0f, 0.0f), Mix(Cloth, Hex(0xffffff), 0.1f), Mix(Cloth, Hex(0x000000), 0.4f)));
	C.FillRect({Px - 34.0f * Sc, Ey + 60.0f * Sc, 68.0f * Sc, 90.0f * Sc}, Shade);
	for (const auto& A : Arms)
	{
		if (Pose == PortraitPose::HandsOnHead)
		{
			C.StrokePolyline({A.first, A.second}, false, Mix(Cloth, Hex(0x000000), 0.25f), 46.0f * Sc, true);
		}
	}
	C.FillPolygon(Head, Skin);
	// Hair on top (none for a shaved head), under any hat.
	if (Style != 0)
	{
		const float Thick = Style == 7 ? 40.0f : Style == 1 ? 4.0f : 16.0f;
		Pts Cap;
		for (int I = 0; I <= 10; ++I)
		{
			const float A = Pi + Pi * static_cast<float>(I) / 10.0f;
			Cap.push_back(P((80.0f + Thick) * std::cos(A), -8.0f + (122.0f + Thick) * std::sin(A)));
		}
		Cap.push_back(P(70.0f, -40.0f));
		Cap.push_back(P(0.0f, -62.0f));
		Cap.push_back(P(-70.0f, -40.0f));
		C.FillPolygon(Cap, Hair);
	}
	if (L.Hat != 0)
	{
		const Color HatCol = Hex(hero::HatTone(L, Who.Age));
		C.FillEllipse(Px, Ey - 96.0f * Sc, 90.0f * Sc, 52.0f * Sc, HatCol);
		C.FillRect({Px - 96.0f * Sc, Ey - 74.0f * Sc, 192.0f * Sc, 22.0f * Sc}, Mix(HatCol, Hex(0x000000), 0.25f));
	}
	// The face: brows, eyes, mouth.
	const float Lift = -std::clamp(E.BrowLift, -1.0f, 1.5f) * 9.0f;
	const float Knit = std::clamp(E.BrowKnit, -1.0f, 1.0f) * 9.0f;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float Sd = static_cast<float>(Side);
		const float Asym = -Sd * std::clamp(E.BrowAsym, -1.0f, 1.0f) * 7.0f;
		C.StrokePolyline({P(Sd * 14.0f, -26.0f + Lift + Asym + Knit), P(Sd * 52.0f, -30.0f + Lift + Asym - Knit * 0.4f)}, false, Mix(Hair, Hex(0x000000), 0.3f), 10.0f * Sc + 0.6f, true);
		const float Open = std::clamp(E.EyeOpen, 0.0f, 1.7f);
		const Vec2 Eye = P(Sd * 32.0f + E.LookX * 1.5f, 2.0f + E.LookY * 1.5f);
		if (Open < 0.3f)
		{
			C.StrokePolyline({P(Sd * 32.0f - 13.0f, 4.0f), P(Sd * 32.0f, E.Smile > 0.4f ? -4.0f : 6.0f), P(Sd * 32.0f + 13.0f, 4.0f)}, false, Ink, 7.0f * Sc + 0.5f, true);
			continue;
		}
		if (Open > 1.2f)
		{
			C.FillEllipse(Eye.X, Eye.Y, 15.0f * Sc, 17.0f * Sc, Hex(0xffffff));
		}
		C.FillEllipse(Eye.X, Eye.Y, 8.0f * Sc, 9.0f * Sc * std::min(1.0f, Open + 0.2f), Ink);
	}
	const float Smile = std::clamp(E.Smile, -1.0f, 1.0f), Mo = std::clamp(E.MouthOpen, 0.0f, 1.0f);
	if (Mo >= 0.15f)
	{
		C.FillEllipse(Px + E.Smirk * 6.0f * Sc, Ey + (62.0f + Mo * 10.0f) * Sc, (24.0f - (Smile < 0.25f ? Mo * 8.0f : -Smile * 6.0f)) * Sc, (6.0f + Mo * 18.0f) * Sc, Hex(0x3a0c10));
		if (E.Teeth)
		{
			C.FillRect({Px - 16.0f * Sc, Ey + (56.0f + Mo * 2.0f) * Sc, 32.0f * Sc, 6.0f * Sc}, Hex(0xf4efe6));
		}
	}
	else
	{
		const float Cy = Ey + 64.0f * Sc;
		C.StrokePolyline({P(-24.0f, 64.0f - Smile * 10.0f + E.Smirk * 4.0f), {Px, Cy + Smile * 6.0f}, P(24.0f, 64.0f - Smile * 10.0f - E.Smirk * 8.0f)}, false, Mix(Skin, Hex(0x6a1c22), 0.75f), 8.0f * Sc + 0.5f,
			true);
	}
	if (L.Glasses != 0)
	{
		for (int Side = -1; Side <= 1; Side += 2)
		{
			C.StrokeEllipse(Px + static_cast<float>(Side) * 32.0f * Sc, Ey + 2.0f * Sc, 22.0f * Sc, 18.0f * Sc, Hex(0x1b1b1b), 5.0f * Sc + 0.5f);
		}
	}
	// The arms in front (the hands: skin dots at their ends).
	for (const auto& A : Arms)
	{
		if (Pose != PortraitPose::HandsOnHead)
		{
			C.StrokePolyline({A.first, A.second}, false, Mix(Cloth, Hex(0x000000), 0.25f), 46.0f * Sc, true);
		}
		C.FillCircle(A.second.X, A.second.Y, 26.0f * Sc, Skin);
	}
}

void Ghost(Ctx& X, float Cx, float Cy, float S, float Alpha);

/**
 * The person: DrawPortrait with the recipe's expression and pose, looking (and pointing) toward the frame's middle, lit
 * with the palette's accent as the rim, inside a cut-out halo: each convex piece of the silhouette grown and filled in the
 * halo's colour over its glow (S-T: the pieces' seams are hidden under the fill; one feathered concave path would overlap
 * its own fringe).
 */
void Person(Ctx& X, const hero::Character& Who, float Cx, float Ey, float Sc, int Expr, int Pose, bool Second = false)
{
	Canvas& C = *X.C;
	if (Who.FirstName == RivalName && Who.LastName.empty())
	{
		// gh0stfold has no face (his own world videos' "you", hero::CharacterFor(RivalName)): his ghost where he'd stand
		// (review C3).
		Ghost(X, Cx, Ey + 40.0f * Sc, 170.0f * Sc, 0.95f);
		return;
	}
	const float Px = Mx(X, Cx);
	PortraitExpression E = ExpressionOf(Expr);
	const float Toward = Px <= DesignW * 0.5f ? 1.0f : -1.0f;
	E.LookX = Toward * std::max(std::fabs(E.LookX), 0.01f);
	const PortraitPose P = Pose > 0 && Pose < static_cast<int>(PortraitPose::Count) ? static_cast<PortraitPose>(Pose) : PortraitPose::None;
	if (X.Tier == 2)
	{
		MiniPerson(X, Who, Px, Ey, Sc, E, P);
		return;
	}
	const std::vector<std::vector<Vec2>> Pieces = thumbportrait::Silhouette(Who, Px, Ey, Sc, E, P);
	std::vector<Pts> Grown;
	for (const std::vector<Vec2>& Piece : Pieces)
	{
		Grown.push_back(X.Tier == 0 ? Grow(Piece, 6.5f) : GrowLean(Piece, 6.5f, 16));
	}
	if (X.Tier == 0)
	{
		for (const Pts& G : Grown)
		{
			C.GlowPolygon(G, WithA(X.P.Halo, 0.4f), 12.0f);
		}
	}
	for (const Pts& G : Grown)
	{
		C.FillPolygon(G, X.P.Halo);
	}
	PortraitLight L;
	L.Key = Hex(0xffe4c8);
	L.Rim = X.P.Acc;
	L.RimStrength = 0.9f;
	// S-T: under the hero tier the portrait draws coarse (DrawPortrait's LOD), about a third of its vertices; so does a
	// second face on a hero (VS, a collab, the reaction box), so one portrait's detail is the budget; and so does a small
	// face (under 0.42 of the design's scale: a head about 85 units tall), whose detail can't show (review C3: the dealt
	// The Pile reached 16k vertices at 640 x 360 against §10's 14k).
	DrawPortrait(C, Who, Px, Ey, Sc, 0.0, L, E, P, X.Tier > 0 || Second || Sc < 0.42f);
}

/** Nobody may be shown (storytime): a dark bust with a question mark. */
void Nobody(Ctx& X, float Cx, float Ey, float Sc)
{
	Canvas& C = *X.C;
	const float Px = Mx(X, Cx);
	const Color Edge = WithA(X.P.Halo, 0.85f);
	C.FillEllipse(Px, Ey + 250.0f * Sc, 190.0f * Sc, 130.0f * Sc, Paint::Linear({Px, Ey + 120.0f * Sc}, {Px, Ey + 380.0f * Sc}, Hex(0x34343e), Hex(0x15151a)));
	C.FillEllipse(Px, Ey, 84.0f * Sc, 104.0f * Sc, Paint::Linear({Px, Ey - 104.0f * Sc}, {Px, Ey + 104.0f * Sc}, Hex(0x3a3a46), Hex(0x18181e)));
	C.StrokeEllipse(Px, Ey, 84.0f * Sc, 104.0f * Sc, Edge, std::max(2.0f, 6.0f * Sc));
	Loud(X, "?", Px, Ey + 40.0f * Sc, 130.0f * Sc, Hex(0xffffff), Hex(0x000000), Align::Center, false);
}

/** gh0stfold's ghost (a sheet with a wavy hem), S about its half height. */
void Ghost(Ctx& X, float Cx, float Cy, float S, float Alpha)
{
	Canvas& C = *X.C;
	const float Px = Mx(X, Cx);
	auto P = [&](float U, float W) { return Vec2{Px + U * S, Cy + W * S}; };
	Pts Body;
	for (int I = 0; I <= 16; ++I)
	{
		const float A = Pi + Pi * static_cast<float>(I) / 16.0f;
		Body.push_back(P(0.62f * std::cos(A), -0.1f + 0.82f * std::sin(A)));
	}
	Body.push_back(P(0.66f, 0.9f));
	for (int I = 0; I <= 8; ++I)
	{
		const float U = 0.66f - 1.32f * static_cast<float>(I) / 8.0f;
		Body.push_back(P(U, I % 2 == 0 ? 0.9f : 1.12f));
	}
	if (X.Tier == 0)
	{
		C.GlowPolygon(Body, Color{0.8f, 0.9f, 1.0f, 0.35f * Alpha}, 18.0f * S / 100.0f);
	}
	C.FillPolygon(Body, Paint::Linear(P(0.0f, -0.9f), P(0.0f, 1.1f), Color{0.95f, 0.97f, 1.0f, Alpha}, Color{0.75f, 0.84f, 0.98f, Alpha}));
	C.FillEllipse(Px - 0.22f * S, Cy - 0.08f * S, 0.1f * S, 0.16f * S, Color{0.08f, 0.08f, 0.16f, Alpha});
	C.FillEllipse(Px + 0.22f * S, Cy - 0.08f * S, 0.1f * S, 0.16f * S, Color{0.08f, 0.08f, 0.16f, Alpha});
	C.StrokeArc(Px, Cy + 0.22f * S, 0.16f * S, 0.4f, 2.7f, Color{0.08f, 0.08f, 0.16f, Alpha}, std::max(1.5f, 0.05f * S), true);
}

/** The opponent: the procedural face for their name, gh0stfold's ghost, or nobody's silhouette (storytime, no facts). */
void Opponent(Ctx& X, float Cx, float Ey, float Sc, int Expr, bool Second = false)
{
	const std::string& Who = X.Other;
	if (Who.empty())
	{
		Nobody(X, Cx, Ey, Sc);
	}
	else if (Who == RivalName)
	{
		Ghost(X, Cx, Ey + 40.0f * Sc, 170.0f * Sc, 0.95f);
	}
	else
	{
		Person(X, hero::CharacterFor(Who), Cx, Ey, Sc, Expr, 0, Second);
	}
}

/** The archetype's person slot: whoever the recipe's subject is (nobody and an object leave the slot to the composition). */
void Subject(Ctx& X, float Cx, float Ey, float Sc)
{
	Sc = Zoom(X, Sc);
	switch (static_cast<thumbart::Subject>(X.S->Subj))
	{
	case thumbart::Subject::Opponent: Opponent(X, Cx, Ey, Sc, X.S->Expr); break;
	case thumbart::Subject::Ghost: Ghost(X, Cx, Ey + 40.0f * Sc, 170.0f * Sc, 0.95f); break;
	case thumbart::Subject::Nobody:
	case thumbart::Subject::Object: break;
	default: Person(X, *X.Look, Cx, Ey, Sc, X.S->Expr, X.S->Pose); break;
	}
}

/** Whether the slot holds a face (else the composition fills it). */
bool HasFace(const Ctx& X)
{
	const thumbart::Subject S = static_cast<thumbart::Subject>(X.S->Subj);
	return S != thumbart::Subject::Nobody && S != thumbart::Subject::Object;
}

// ------------------------------------------------------------------ cards

/** An upright card (its rank in the card's own face), placed before the mirror; Card < 0 draws the back. */
void Up(Ctx& X, int Cd, float L, float T, float W, float H, bool Glowing = false)
{
	const float Px = Mleft(X, L, W);
	if (Glowing && !X.Lod)
	{
		X.C->GlowRoundRect({Px, T, W, H}, W * 0.08f, WithA(X.P.Acc, 0.95f), W * 0.32f);
	}
	DrawCard(*X.C, Cd, Px, T, W, H);
}

/**
 * A group that must read left to right whatever the mirror (cards in their order, a board, a graph's time, a bar's fill,
 * before then after): drawn unflipped at its box's mirrored place (L .. R before the mirror).
 */
struct Upright
{
	Ctx& X;
	bool Was;
	Upright(Ctx& InX, float L, float R) : X(InX), Was(InX.Mirror)
	{
		X.C->Save();
		if (Was)
		{
			X.C->Translate(DesignW - L - R, 0.0f);
			X.Mirror = false;
		}
	}
	~Upright()
	{
		X.Mirror = Was;
		X.C->Restore();
	}
	Upright(const Upright&) = delete;
	Upright& operator=(const Upright&) = delete;
};

/** Where a point inside an Upright block (L .. R) is, as the before-the-mirror coordinate StickerAt and Mx expect. */
float Kept(const Ctx& X, float L, float R, float V)
{
	return X.Mirror ? L + R - V : V;
}

/** A row of upright cards in their order (Want of them: the facts' cards, backs for the rest). */
void UpRow(Ctx& X, const std::vector<Card>& Cards, size_t Want, float L, float T, float W, float Gap)
{
	if (Want == 0)
	{
		return;
	}
	const Upright Keep(X, L, L + static_cast<float>(Want) * (W + Gap) - Gap);
	for (size_t I = 0; I < Want; ++I)
	{
		Up(X, I < Cards.size() ? Cards[I] : -1, L + static_cast<float>(I) * (W + Gap), T, W, W * 1.4f);
	}
}

/** A vector rank glyph (text can't rotate): strokes in a box H tall and 0.62 H wide, top-left at (X0, Y0). */
void RankGlyph(Canvas& C, int Rank, float X0, float Y0, float H, const Color& Col)
{
	const float W = H * 0.62f;
	auto P = [&](float U, float V) { return Vec2{X0 + U * W, Y0 + V * H}; };
	const float Sw = H * 0.15f;
	auto Line = [&](std::initializer_list<Vec2> Uv) {
		Pts Q;
		for (const Vec2& T : Uv)
		{
			Q.push_back(P(T.X, T.Y));
		}
		C.StrokePolyline(Q, false, Col, Sw, true);
	};
	auto Loop = [&](float U, float V, float Ru, float Rv) {
		Pts Q;
		for (int I = 0; I <= 18; ++I)
		{
			const float A = 2.0f * Pi * static_cast<float>(I) / 18.0f;
			Q.push_back(P(U + Ru * std::cos(A), V + Rv * std::sin(A)));
		}
		C.StrokePolyline(Q, false, Col, Sw, true);
	};
	switch (Rank)
	{
	case 12: // A
		Line({{0.0f, 1.0f}, {0.5f, 0.0f}, {1.0f, 1.0f}});
		Line({{0.22f, 0.64f}, {0.78f, 0.64f}});
		break;
	case 11: // K
		Line({{0.06f, 0.0f}, {0.06f, 1.0f}});
		Line({{0.96f, 0.0f}, {0.1f, 0.6f}});
		Line({{0.36f, 0.42f}, {1.0f, 1.0f}});
		break;
	case 10: // Q
		Loop(0.5f, 0.48f, 0.46f, 0.46f);
		Line({{0.58f, 0.72f}, {1.02f, 1.04f}});
		break;
	case 9: // J
		Line({{0.3f, 0.0f}, {0.96f, 0.0f}});
		Line({{0.76f, 0.0f}, {0.76f, 0.74f}, {0.66f, 0.94f}, {0.44f, 1.0f}, {0.2f, 0.94f}, {0.06f, 0.78f}});
		break;
	case 8: // 10
		Line({{0.0f, 0.18f}, {0.2f, 0.0f}, {0.2f, 1.0f}});
		Loop(0.72f, 0.5f, 0.26f, 0.48f);
		break;
	case 7: // 9
		Loop(0.48f, 0.32f, 0.42f, 0.3f);
		Line({{0.9f, 0.32f}, {0.84f, 0.72f}, {0.6f, 0.98f}, {0.12f, 0.96f}});
		break;
	case 6: // 8
		Loop(0.5f, 0.26f, 0.36f, 0.24f);
		Loop(0.5f, 0.74f, 0.44f, 0.26f);
		break;
	case 5: // 7
		Line({{0.02f, 0.0f}, {0.98f, 0.0f}, {0.34f, 1.0f}});
		break;
	case 4: // 6
		Line({{0.86f, 0.06f}, {0.42f, 0.2f}, {0.12f, 0.58f}});
		Loop(0.5f, 0.7f, 0.42f, 0.29f);
		break;
	case 3: // 5
		Line({{0.92f, 0.0f}, {0.14f, 0.0f}, {0.08f, 0.44f}, {0.42f, 0.36f}, {0.8f, 0.44f}, {0.96f, 0.68f}, {0.82f, 0.92f}, {0.42f, 1.0f}, {0.04f, 0.9f}});
		break;
	case 2: // 4
		Line({{0.74f, 1.0f}, {0.74f, 0.0f}, {0.02f, 0.68f}, {1.0f, 0.68f}});
		break;
	case 1: // 3
		Line({{0.06f, 0.12f}, {0.46f, 0.0f}, {0.88f, 0.14f}, {0.86f, 0.34f}, {0.46f, 0.48f}});
		Line({{0.46f, 0.48f}, {0.92f, 0.62f}, {0.92f, 0.84f}, {0.62f, 0.99f}, {0.24f, 1.0f}, {0.02f, 0.88f}});
		break;
	default: // 2
		Line({{0.06f, 0.2f}, {0.3f, 0.02f}, {0.66f, 0.0f}, {0.92f, 0.16f}, {0.9f, 0.38f}, {0.02f, 1.0f}, {1.0f, 1.0f}});
		break;
	}
}

/** A card at any angle (a fan): its vector rank and suit; Card < 0 draws a back. Centre (Cx, Cy) after the mirror. */
void FanCard(Ctx& X, int Cd, float Cx, float Cy, float W, float H, float Angle, bool Glowing)
{
	Canvas& C = *X.C;
	C.Save();
	C.Translate(Cx, Cy);
	C.Rotate(Angle);
	if (Glowing && !X.Lod)
	{
		C.GlowRoundRect({-W * 0.5f, -H * 0.5f, W, H}, W * 0.1f, WithA(X.P.Acc, 0.9f), W * 0.3f);
	}
	if (Cd < 0)
	{
		C.FillRoundRect({-W * 0.5f, -H * 0.5f, W, H}, W * 0.1f, Hex(0xf4f6fa));
		C.FillRoundRect({-W * 0.43f, -H * 0.45f, W * 0.86f, H * 0.9f}, W * 0.06f, Paint::Linear({-W, -H}, {W, H}, Hex(0x0e7c72), Hex(0x0b3f5c)));
	}
	else
	{
		C.FillRoundRect({-W * 0.5f + 2.0f, -H * 0.5f + 4.0f, W, H}, W * 0.1f, Color{0.0f, 0.0f, 0.0f, 0.35f});
		C.FillRoundRect({-W * 0.5f, -H * 0.5f, W, H}, W * 0.1f, Paint::Linear({0.0f, -H * 0.5f}, {0.0f, H * 0.5f}, Hex(0xffffff), Hex(0xe9edf3)));
		C.StrokeRoundRect({-W * 0.5f, -H * 0.5f, W, H}, W * 0.1f, Color{0.0f, 0.0f, 0.0f, 0.2f}, 1.0f);
		const Color Col = SuitColor(SuitOf(Cd));
		RankGlyph(C, RankOf(Cd), -W * 0.38f, -H * 0.42f, H * 0.24f, Col);
		DrawSuit(C, SuitOf(Cd), -W * 0.39f, -H * 0.12f, H * 0.16f, Col);
		DrawSuit(C, SuitOf(Cd), W * 0.5f - H * 0.38f, H * 0.5f - H * 0.4f, H * 0.32f, Col);
	}
	C.Restore();
}

/** A fan of cards about (Cx, Cy) before the mirror, Spread radians between neighbours. */
void Fan(Ctx& X, const std::vector<Card>& Cards, float Cx, float Cy, float W, float Spread, bool Glowing)
{
	const size_t N = Cards.size();
	for (size_t I = 0; I < N; ++I)
	{
		// The fan moves with the mirror; its cards keep their order.
		const float K = static_cast<float>(I) - static_cast<float>(N - 1) * 0.5f;
		const float A = K * Spread;
		const float Px = Mx(X, Cx) + K * W * 0.62f;
		FanCard(X, Cards[I], Px, Cy + std::fabs(K) * W * 0.12f, W, W * 1.4f, A, Glowing);
	}
}

/** The cards that make the hand the video is about: the player's, the board's that pair them, then the highest others (5 at most). */
std::vector<Card> MadeHand(const Ctx& X)
{
	std::vector<Card> Out = X.Mine;
	std::vector<Card> Rest;
	for (const Card B : X.Board)
	{
		bool Match = false;
		for (const Card M : X.Mine)
		{
			Match = Match || RankOf(M) == RankOf(B);
		}
		(Match ? Out : Rest).push_back(B);
	}
	std::stable_sort(Rest.begin(), Rest.end(), [](Card A, Card B) { return RankOf(A) > RankOf(B); });
	for (const Card R : Rest)
	{
		Out.push_back(R);
	}
	if (Out.size() > 5)
	{
		Out.resize(5);
	}
	return Out;
}

// ------------------------------------------------------------------ annotations and stickers

/** The red circle (a white rim under it). Centre after the mirror. */
void RedCircle(Canvas& C, float Cx, float Cy, float Rx, float Ry, float W)
{
	C.StrokeEllipse(Cx, Cy, Rx + 1.0f, Ry + 1.0f, Color{1.0f, 1.0f, 1.0f, 0.85f}, W + 5.0f);
	C.StrokeEllipse(Cx, Cy, Rx, Ry, Hex(HotRed), W);
}

/** A curved arrow from (X0, Y0) to (X1, Y1) (after the mirror), bowed by Bend, with its head. */
void Arrow(Canvas& C, float X0, float Y0, float X1, float Y1, float Bend, const Color& Col, float W)
{
	const float Mxp = (X0 + X1) * 0.5f, Myp = (Y0 + Y1) * 0.5f + Bend;
	Pts Curve;
	for (int I = 0; I <= 14; ++I)
	{
		const float T = static_cast<float>(I) / 14.0f;
		const float U = 1.0f - T;
		Curve.push_back({U * U * X0 + 2.0f * U * T * Mxp + T * T * X1, U * U * Y0 + 2.0f * U * T * Myp + T * T * Y1});
	}
	const Vec2 End = Curve.back();
	const Vec2 Prev = Curve[Curve.size() - 3];
	const float A = std::atan2(End.Y - Prev.Y, End.X - Prev.X);
	const float Hl = W * 2.6f;
	Curve.pop_back();
	Curve.pop_back();
	C.StrokePolyline(Curve, false, Color{1.0f, 1.0f, 1.0f, 0.9f}, W + 5.0f, true);
	C.StrokePolyline(Curve, false, Col, W, true);
	const Pts Head = {End, {End.X - Hl * std::cos(A - 0.5f), End.Y - Hl * std::sin(A - 0.5f)}, {End.X - Hl * std::cos(A + 0.5f), End.Y - Hl * std::sin(A + 0.5f)}};
	C.FillPolygon(Grow(Head, 2.5f), Color{1.0f, 1.0f, 1.0f, 0.9f});
	C.FillPolygon(Head, Col);
}

/** The red X (after the mirror). */
void RedX(Canvas& C, float Cx, float Cy, float R, float W)
{
	for (int Pass = 0; Pass < 2; ++Pass)
	{
		const Color Col = Pass == 0 ? Color{1.0f, 1.0f, 1.0f, 0.95f} : Hex(HotRed);
		const float Ww = Pass == 0 ? W + 6.0f : W;
		C.StrokePolyline({{Cx - R, Cy - R}, {Cx + R, Cy + R}}, false, Col, Ww, true);
		C.StrokePolyline({{Cx + R, Cy - R}, {Cx - R, Cy + R}}, false, Col, Ww, true);
	}
}

/** A burst plate with words in it (the number burst, the x25 sticker). Centre after the mirror. */
void BurstPlate(Ctx& X, const std::string& S, float Cx, float Cy, float Size, const Color& Fill, const Color& InkCol, float MinR)
{
	Canvas& C = *X.C;
	const float Sz = Fit(C, S, Size, 300.0f, false);
	const float R = std::max(MinR, Wide(C, S, Sz, false) * 0.62f + Sz * 0.45f);
	C.FillPolygon(Star(Cx + 5.0f, Cy + 6.0f, R, R * 0.8f, 18, 0.1f), Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillPolygon(Star(Cx, Cy, R, R * 0.8f, 18, 0.1f), Hex(0xffffff));
	C.FillPolygon(Star(Cx, Cy, R - 6.0f, (R - 6.0f) * 0.8f, 18, 0.1f), Fill);
	Plain(X, S, Cx, Cy + Sz * 0.36f, Sz, InkCol, Align::Center);
}

/**
 * A stamp's dark plate: the palette's ground, darkened; a light ground (Sunrise's pink) darker still, so the red ink reads
 * (review-c3f-thumbs: HotRed on Sunrise's plate was about 2:1, Before -> After's stamped words on a crimson plate).
 */
Color StampPlate(const Ctx& X)
{
	const Color& B = X.P.B;
	const bool Light = 0.2126f * B.R + 0.7152f * B.G + 0.0722f * B.B > 0.3f;
	return WithA(Mix(B, Hex(0x000000), Light ? 0.72f : 0.4f), 0.92f);
}

/** A stamp: a double-ruled box around the words, in Col, on an optional plate. Centre after the mirror. */
void StampBox(Ctx& X, const std::string& S, float Cx, float Cy, float Size, const Color& Col, const Color* Plate)
{
	Canvas& C = *X.C;
	const float Sz = Fit(C, S, Size, 560.0f, false);
	const float W = Wide(C, S, Sz, false) + Sz * 0.9f;
	const float H = Sz * 1.45f;
	const Rect R{Cx - W * 0.5f, Cy - H * 0.5f, W, H};
	if (Plate)
	{
		C.FillRoundRect({R.X + 4.0f, R.Y + 5.0f, R.W, R.H}, 6.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
		C.FillRoundRect(R, 6.0f, *Plate);
	}
	C.StrokeRoundRect(R, 6.0f, Col, Sz * 0.12f);
	C.StrokeRoundRect({R.X + Sz * 0.2f, R.Y + Sz * 0.2f, R.W - Sz * 0.4f, R.H - Sz * 0.4f}, 3.0f, Col, std::max(1.0f, Sz * 0.045f));
	Plain(X, S, Cx, Cy + Sz * 0.36f, Sz, Col, Align::Center);
}

/** A small round face sticker (crying, shocked): vectors, never an emoji glyph. */
void FaceSticker(Canvas& C, float Cx, float Cy, float R, bool Crying)
{
	C.FillCircle(Cx + 3.0f, Cy + 4.0f, R, Color{0.0f, 0.0f, 0.0f, 0.3f});
	C.FillCircle(Cx, Cy, R + 3.0f, Hex(0xffffff));
	C.FillCircle(Cx, Cy, R, Paint::Radial({Cx - R * 0.3f, Cy - R * 0.3f}, 0.0f, {Cx, Cy}, R, Hex(0xffe56b), 0.6f, Hex(0xffd23f), Hex(0xf0a81c)));
	const Color Ink = Hex(0x3a2408);
	if (Crying)
	{
		C.StrokeArc(Cx - R * 0.34f, Cy - R * 0.12f, R * 0.18f, Pi * 1.1f, Pi * 1.9f, Ink, R * 0.09f, true);
		C.StrokeArc(Cx + R * 0.34f, Cy - R * 0.12f, R * 0.18f, Pi * 1.1f, Pi * 1.9f, Ink, R * 0.09f, true);
		C.StrokeArc(Cx, Cy + R * 0.52f, R * 0.3f, Pi * 1.15f, Pi * 1.85f, Ink, R * 0.1f, true);
		for (int S = -1; S <= 1; S += 2)
		{
			const float Tx = Cx + static_cast<float>(S) * R * 0.36f;
			C.FillPolygon({{Tx - R * 0.1f, Cy}, {Tx + R * 0.1f, Cy}, {Tx + R * 0.14f, Cy + R * 0.86f}, {Tx - R * 0.14f, Cy + R * 0.86f}}, Hex(0x6cc6f0));
		}
	}
	else
	{
		C.FillEllipse(Cx - R * 0.32f, Cy - R * 0.18f, R * 0.13f, R * 0.2f, Ink);
		C.FillEllipse(Cx + R * 0.32f, Cy - R * 0.18f, R * 0.13f, R * 0.2f, Ink);
		C.FillEllipse(Cx, Cy + R * 0.42f, R * 0.2f, R * 0.26f, Ink);
		C.StrokeArc(Cx - R * 0.34f, Cy - R * 0.52f, R * 0.18f, Pi * 1.15f, Pi * 1.85f, Ink, R * 0.07f, true);
		C.StrokeArc(Cx + R * 0.34f, Cy - R * 0.52f, R * 0.18f, Pi * 1.15f, Pi * 1.85f, Ink, R * 0.07f, true);
	}
}

void Crown(Canvas& C, float Cx, float Cy, float S, const Color& Col)
{
	const Pts P = {{Cx - S, Cy + S * 0.5f}, {Cx - S, Cy - S * 0.4f}, {Cx - S * 0.5f, Cy}, {Cx, Cy - S * 0.7f}, {Cx + S * 0.5f, Cy}, {Cx + S, Cy - S * 0.4f}, {Cx + S, Cy + S * 0.5f}};
	C.StrokePolyline(P, true, Hex(0xffffff), 5.0f, true);
	C.FillPolygon({P[0], P[1], P[2], P[4], P[5], P[6]}, Col);
	C.FillPolygon({P[2], P[3], P[4]}, Col);
	C.FillCircle(Cx, Cy - S * 0.7f, S * 0.13f, Hex(0xffffff));
	C.FillCircle(Cx - S, Cy - S * 0.4f, S * 0.11f, Hex(0xffffff));
	C.FillCircle(Cx + S, Cy - S * 0.4f, S * 0.11f, Hex(0xffffff));
	C.FillRect({Cx - S, Cy + S * 0.28f, S * 2.0f, S * 0.22f}, Mix(Col, Hex(0x000000), 0.25f));
}

void Skull(Canvas& C, float Cx, float Cy, float S)
{
	C.FillCircle(Cx, Cy - S * 0.1f, S * 0.62f, Hex(0xf4f1e8));
	C.FillRoundRect({Cx - S * 0.36f, Cy + S * 0.28f, S * 0.72f, S * 0.42f}, S * 0.1f, Hex(0xf4f1e8));
	C.FillEllipse(Cx - S * 0.24f, Cy - S * 0.04f, S * 0.17f, S * 0.2f, Hex(0x1a1a1a));
	C.FillEllipse(Cx + S * 0.24f, Cy - S * 0.04f, S * 0.17f, S * 0.2f, Hex(0x1a1a1a));
	C.FillPolygon({{Cx, Cy + S * 0.14f}, {Cx - S * 0.08f, Cy + S * 0.3f}, {Cx + S * 0.08f, Cy + S * 0.3f}}, Hex(0x1a1a1a));
	for (int K = -1; K <= 1; ++K)
	{
		C.FillRect({Cx + static_cast<float>(K) * S * 0.18f - S * 0.025f, Cy + S * 0.44f, S * 0.05f, S * 0.24f}, Hex(0x1a1a1a));
	}
}

/** The lightning bolt (§6.1's "fire" sticker as video::thumb deals it: C9 leaves the fire thread to Ignite). */
void Bolt(Canvas& C, float Cx, float Cy, float S)
{
	// Two convex halves (a concave outline's fill would cross itself), on a white edge.
	const Pts Top = {{Cx + S * 0.2f, Cy - S}, {Cx - S * 0.5f, Cy + S * 0.14f}, {Cx + S * 0.1f, Cy + S * 0.14f}};
	const Pts Low = {{Cx + S * 0.55f, Cy - S * 0.18f}, {Cx - S * 0.05f, Cy - S * 0.18f}, {Cx - S * 0.25f, Cy + S}};
	C.FillPolygon(Grow(Top, 3.0f), Hex(0xffffff));
	C.FillPolygon(Grow(Low, 3.0f), Hex(0xffffff));
	C.FillPolygon(Top, Paint::Linear({Cx, Cy - S}, {Cx, Cy + S}, Hex(0xfff06a), Hex(0xffb020)));
	C.FillPolygon(Low, Paint::Linear({Cx, Cy - S}, {Cx, Cy + S}, Hex(0xfff06a), Hex(0xffb020)));
}

void MoneyBag(Ctx& X, float Cx, float Cy, float S)
{
	Canvas& C = *X.C;
	C.FillEllipse(Cx, Cy + S * 0.25f, S * 0.62f, S * 0.6f, Paint::Linear({Cx - S, Cy}, {Cx + S, Cy + S}, Hex(0xd9b26a), Hex(0x8a6424)));
	C.FillPolygon({{Cx - S * 0.22f, Cy - S * 0.3f}, {Cx + S * 0.22f, Cy - S * 0.3f}, {Cx + S * 0.38f, Cy - S * 0.62f}, {Cx - S * 0.38f, Cy - S * 0.62f}}, Hex(0xc49a50));
	C.FillRect({Cx - S * 0.26f, Cy - S * 0.34f, S * 0.52f, S * 0.08f}, Hex(0x5a3a10));
	Plain(X, "$", Cx, Cy + S * 0.5f, S * 0.7f, Hex(0x3a2408), Align::Center);
}

void Siren(Canvas& C, float Cx, float Cy, float S)
{
	for (int K = 0; K < 5; ++K)
	{
		const float A = Pi * (1.1f + 0.2f * static_cast<float>(K));
		C.StrokePolyline({{Cx + S * 0.75f * std::cos(A), Cy - S * 0.1f + S * 0.75f * std::sin(A)}, {Cx + S * 1.05f * std::cos(A), Cy - S * 0.1f + S * 1.05f * std::sin(A)}}, false, Hex(0xffd23f), S * 0.08f, true);
	}
	C.FillRect({Cx - S * 0.62f, Cy + S * 0.3f, S * 1.24f, S * 0.24f}, Hex(0x2a2a30));
	C.FillPolygon({{Cx - S * 0.48f, Cy + S * 0.3f}, {Cx - S * 0.44f, Cy - S * 0.3f}, {Cx - S * 0.2f, Cy - S * 0.55f}, {Cx + S * 0.2f, Cy - S * 0.55f}, {Cx + S * 0.44f, Cy - S * 0.3f}, {Cx + S * 0.48f, Cy + S * 0.3f}},
		Paint::Linear({Cx - S, Cy}, {Cx + S, Cy}, Hex(0xff6a6a), Hex(0xb00818)));
	C.FillEllipse(Cx - S * 0.18f, Cy - S * 0.2f, S * 0.08f, S * 0.18f, Color{1.0f, 1.0f, 1.0f, 0.7f});
}

/** The recipe's sticker at the archetype's sticker slot (Ax, Ay), annotations aimed at its focus (Fx, Fy); before the mirror. */
void StickerAt(Ctx& X, float Ax, float Ay, float Fx, float Fy)
{
	Canvas& C = *X.C;
	const int S = X.S->Sticker;
	if (S < 0 || S >= static_cast<int>(Sticker::Count))
	{
		return;
	}
	const float Px = Mx(X, Ax + (Hf(X, 31) - 0.5f) * 16.0f), Py = Ay + (Hf(X, 32) - 0.5f) * 12.0f;
	const float Qx = Mx(X, Fx);
	switch (static_cast<Sticker>(S))
	{
	case Sticker::RedCircle: RedCircle(C, Qx, Fy, 58.0f, 58.0f, 8.0f); break;
	case Sticker::CurvedArrow: Arrow(C, Px, Py, Qx + (Px < Qx ? -42.0f : 42.0f), Fy, Py < Fy ? 40.0f : -40.0f, Hex(HotRed), 9.0f); break;
	case Sticker::RedX: RedX(C, Qx, Fy, 34.0f, 11.0f); break;
	case Sticker::Huh: Loud(X, "??", Px, Py + 26.0f, 72.0f, Hex(Yellow), Hex(0x000000), Align::Center, false); break;
	case Sticker::PercentChip:
	{
		// The video's own percentage, from a caption; none, no chip.
		std::string Pc;
		for (const std::string* Cap : {&X.Cap1, &X.Cap2})
		{
			const size_t At = Cap->find('%');
			if (At != std::string::npos && Pc.empty())
			{
				size_t B = At;
				while (B > 0 && (std::isdigit(static_cast<unsigned char>((*Cap)[B - 1])) != 0 || (*Cap)[B - 1] == '.'))
				{
					--B;
				}
				Pc = B < At ? Cap->substr(B, At - B + 1) : std::string();
			}
		}
		if (!Pc.empty())
		{
			const float W = Wide(C, Pc, 34.0f, false) + 30.0f;
			C.FillRoundRect({Px - W * 0.5f + 3.0f, Py - 24.0f + 4.0f, W, 48.0f}, 24.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
			C.FillRoundRect({Px - W * 0.5f, Py - 24.0f, W, 48.0f}, 24.0f, Hex(HotRed));
			C.StrokeRoundRect({Px - W * 0.5f, Py - 24.0f, W, 48.0f}, 24.0f, Hex(0xffffff), 3.0f);
			Plain(X, Pc, Px, Py + 12.0f, 34.0f, Hex(0xffffff), Align::Center);
		}
		break;
	}
	case Sticker::AllIn:
	{
		const Color Plate = Hex(0x111111, 0.85f);
		StampBox(X, "ALL IN", Px, Py, 30.0f, Hex(HotRed), &Plate);
		break;
	}
	case Sticker::Skull: Skull(C, Px, Py, 44.0f); break;
	case Sticker::Bolt: Bolt(C, Px, Py, 46.0f); break;
	case Sticker::MoneyBag: MoneyBag(X, Px, Py, 46.0f); break;
	case Sticker::Crying: FaceSticker(C, Px, Py, 34.0f, true); break;
	case Sticker::ShockedFace: FaceSticker(C, Px, Py, 34.0f, false); break;
	case Sticker::Siren: Siren(C, Px, Py, 42.0f); break;
	case Sticker::Crown: Crown(C, Px, Py, 34.0f, Hex(Yellow)); break;
	case Sticker::Ghost: Ghost(X, Ax, Ay, 48.0f, 0.95f); break;
	default: break;
	}
}

// ------------------------------------------------------------------ props

/** A sheet of paper: a title, ruled lines, an amount; Rotation none (its words can't turn). R before the mirror. */
void Paper(Ctx& X, Rect R, const std::string& Title, int Lines, const std::string& Amount)
{
	Canvas& C = *X.C;
	R.X = Mleft(X, R.X, R.W);
	C.FillRoundRect({R.X + 6.0f, R.Y + 8.0f, R.W, R.H}, 3.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect(R, 3.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0xfbf8f1), Hex(0xe9e2d2)));
	const float Ts = Fit(C, Title, R.W * 0.085f, R.W - 24.0f, false);
	Plain(X, Title, R.X + R.W * 0.5f, R.Y + Ts + 12.0f, Ts, Hex(0x1b1b1b), Align::Center);
	C.FillRect({R.X + 16.0f, R.Y + Ts + 22.0f, R.W - 32.0f, 2.0f}, Hex(0x1b1b1b));
	for (int I = 0; I < Lines; ++I)
	{
		const float Wd = (R.W - 32.0f) * (0.92f - 0.16f * static_cast<float>(I % 3));
		C.FillRoundRect({R.X + 16.0f, R.Y + Ts + 36.0f + static_cast<float>(I) * 16.0f, Wd, 6.0f}, 3.0f, Hex(0x9a9488));
	}
	if (!Amount.empty())
	{
		const float As = Fit(C, Amount, R.W * 0.17f, R.W - 24.0f, false);
		Plain(X, Amount, R.X + R.W * 0.5f, R.Y + R.H - 22.0f, As, Hex(0x1b1b1b), Align::Center);
	}
}

void Trophy(Canvas& C, float Cx, float Cy, float S, const Color& Gold)
{
	const Paint Metal = Paint::Linear({Cx - S, Cy - S}, {Cx + S, Cy + S}, Mix(Gold, Hex(0xffffff), 0.35f), Mix(Gold, Hex(0x5a3a00), 0.4f));
	C.StrokeArc(Cx - S * 0.5f, Cy - S * 0.45f, S * 0.24f, Pi * 0.5f, Pi * 1.5f, Gold, S * 0.08f);
	C.StrokeArc(Cx + S * 0.5f, Cy - S * 0.45f, S * 0.24f, -Pi * 0.5f, Pi * 0.5f, Gold, S * 0.08f);
	C.FillPolygon({{Cx - S * 0.52f, Cy - S * 0.72f}, {Cx + S * 0.52f, Cy - S * 0.72f}, {Cx + S * 0.44f, Cy - S * 0.2f}, {Cx + S * 0.16f, Cy + S * 0.06f}, {Cx - S * 0.16f, Cy + S * 0.06f}, {Cx - S * 0.44f, Cy - S * 0.2f}}, Metal);
	C.FillRect({Cx - S * 0.08f, Cy + S * 0.04f, S * 0.16f, S * 0.36f}, Metal);
	C.FillRoundRect({Cx - S * 0.36f, Cy + S * 0.36f, S * 0.72f, S * 0.12f}, S * 0.03f, Metal);
	C.FillRoundRect({Cx - S * 0.46f, Cy + S * 0.46f, S * 0.92f, S * 0.2f}, S * 0.03f, Hex(0x3a2a10));
	C.FillEllipse(Cx - S * 0.22f, Cy - S * 0.48f, S * 0.07f, S * 0.18f, Color{1.0f, 1.0f, 1.0f, 0.5f});
}

/** A chip seen from above at an angle: its edge, its face, the inlay's dashes (Fine: the hero's; a tile's ring is plain). */
void ChipAt(Canvas& C, float X, float Y, float R, const Color& Face, bool Fine)
{
	C.FillEllipse(X, Y + R * 0.28f, R, R * 0.44f, Mix(Face, Hex(0x000000), 0.45f));
	C.FillEllipse(X, Y, R, R * 0.44f, Face);
	C.StrokeEllipse(X, Y, R * 0.7f, R * 0.3f, Color{1.0f, 1.0f, 1.0f, 0.75f}, std::max(1.0f, R * 0.12f), Fine ? R * 0.3f : 0.0f);
}

const uint32_t ChipFaces[6] = {0xff6b57, 0x4fc3f7, 0xffd23f, 0x8b5cff, 0xf4f4f4, 0x2fbf71};

void ChipPile(Ctx& X, float Cx, float Cy, float S, int N)
{
	for (int I = 0; I < N; ++I)
	{
		const float Px = Mx(X, Cx + (Hf(X, 1200u + static_cast<uint32_t>(I)) - 0.5f) * S * 2.2f);
		const float Py = Cy + (Hf(X, 1300u + static_cast<uint32_t>(I)) - 0.5f) * S * 0.9f;
		ChipAt(*X.C, Px, Py, S * 0.24f, Hex(ChipFaces[I % 6]), X.Tier == 0);
	}
}

/** A stack of chips, leaning by Lean a chip (the bubble's teetering tower). Base centre after the mirror. */
void ChipTower(Canvas& C, float Cx, float By, float R, int N, float Lean, const Color& Face, bool Fine)
{
	for (int I = 0; I < N; ++I)
	{
		const float Off = Lean * static_cast<float>(I) * static_cast<float>(I) * 0.04f;
		// Only the top chip shows its inlay; the ones under it show their edges.
		ChipAt(C, Cx + Off, By - static_cast<float>(I) * R * 0.32f, R, I % 4 == 3 ? Hex(0xf4f4f4) : Face, Fine && I == N - 1);
	}
}

void CashStack(Canvas& C, float Cx, float By, float S, int N)
{
	for (int I = 0; I < N; ++I)
	{
		const float Y = By - static_cast<float>(I) * S * 0.12f;
		const float Dx = (static_cast<float>(I % 3) - 1.0f) * S * 0.03f;
		C.FillRoundRect({Cx - S * 0.6f + Dx, Y - S * 0.1f, S * 1.2f, S * 0.14f}, S * 0.02f, Hex(0x2f9a5c));
		C.FillRect({Cx - S * 0.6f + Dx, Y - S * 0.1f, S * 1.2f, S * 0.025f}, Hex(0x7fd39a));
	}
	const float Top = By - static_cast<float>(N) * S * 0.12f;
	C.FillRect({Cx - S * 0.16f, Top - S * 0.02f, S * 0.32f, static_cast<float>(N) * S * 0.12f + S * 0.06f}, Hex(0xf2e6b8));
}

void Envelope(Canvas& C, Rect R, bool Torn)
{
	C.FillRoundRect({R.X + 6.0f, R.Y + 8.0f, R.W, R.H}, 6.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect(R, 6.0f, Hex(0xf2ebdd));
	C.StrokeRoundRect(R, 6.0f, Hex(0xc9b48a), 3.0f);
	C.FillPolygon({{R.X, R.Y}, {R.X + R.W * 0.5f, R.Y + R.H * 0.55f}, {R.X + R.W, R.Y}}, Hex(0xe6dcc6));
	C.StrokePolyline({{R.X, R.Y}, {R.X + R.W * 0.5f, R.Y + R.H * 0.55f}, {R.X + R.W, R.Y}}, false, Hex(0xc9b48a), 3.0f);
	if (Torn)
	{
		C.FillPolygon({{R.X + R.W * 0.64f, R.Y}, {R.X + R.W * 0.72f, R.Y - 12.0f}, {R.X + R.W * 0.8f, R.Y + 4.0f}, {R.X + R.W * 0.88f, R.Y - 10.0f}, {R.X + R.W, R.Y}}, Hex(0x1a1a1a));
	}
}

void Laptop(Canvas& C, float Cx, float Cy, float S, const Color& Glow, int Tables)
{
	const Rect Screen{Cx - S * 0.8f, Cy - S * 0.55f, S * 1.6f, S * 1.0f};
	C.FillRoundRect({Screen.X - S * 0.05f, Screen.Y - S * 0.05f, Screen.W + S * 0.1f, Screen.H + S * 0.1f}, S * 0.06f, Hex(0x1c1f26));
	C.FillRect(Screen, Paint::Linear({Screen.X, Screen.Y}, {Screen.X, Screen.Y + Screen.H}, Mix(Glow, Hex(0x0a1220), 0.55f), Hex(0x081018)));
	const int Cols = Tables > 1 ? 2 : 1;
	const int Rows = Tables > 2 ? 2 : 1;
	for (int I = 0; I < Cols * Rows; ++I)
	{
		const float Cxx = Screen.X + Screen.W * ((static_cast<float>(I % Cols) + 0.5f) / static_cast<float>(Cols));
		const float Cyy = Screen.Y + Screen.H * ((static_cast<float>(I / Cols) + 0.5f) / static_cast<float>(Rows));
		const float Rx = Screen.W * 0.38f / static_cast<float>(Cols), Ry = Screen.H * 0.3f / static_cast<float>(Rows);
		C.FillEllipse(Cxx, Cyy, Rx + 2.0f, Ry + 2.0f, Hex(0x3b2412));
		C.FillEllipse(Cxx, Cyy, Rx, Ry, Hex(0x0f6a45));
	}
	C.FillPolygon({{Cx - S * 0.95f, Cy + S * 0.5f}, {Cx + S * 0.95f, Cy + S * 0.5f}, {Cx + S * 1.1f, Cy + S * 0.62f}, {Cx - S * 1.1f, Cy + S * 0.62f}}, Hex(0x2a2e37));
}

void Monitor(Canvas& C, Rect R, const Color& Screen)
{
	C.FillRoundRect(R, 6.0f, Hex(0x16181d));
	C.FillRect({R.X + 6.0f, R.Y + 6.0f, R.W - 12.0f, R.H - 12.0f}, Screen);
	for (int K = 0; K < 3; ++K)
	{
		C.FillRoundRect({R.X + 16.0f, R.Y + 16.0f + static_cast<float>(K) * R.H * 0.22f, R.W * 0.5f, R.H * 0.1f}, 3.0f, Color{1.0f, 1.0f, 1.0f, 0.25f});
	}
	C.FillRect({R.X + R.W * 0.45f, R.Y + R.H, R.W * 0.1f, R.H * 0.18f}, Hex(0x2a2e37));
	C.FillRoundRect({R.X + R.W * 0.3f, R.Y + R.H * 1.16f, R.W * 0.4f, R.H * 0.06f}, 3.0f, Hex(0x2a2e37));
}

void Whiteboard(Ctx& X, Rect R)
{
	Canvas& C = *X.C;
	R.X = Mleft(X, R.X, R.W);
	C.FillRoundRect({R.X + 6.0f, R.Y + 8.0f, R.W, R.H}, 8.0f, Color{0.0f, 0.0f, 0.0f, 0.3f});
	C.FillRoundRect(R, 8.0f, Hex(0xf4f6f8));
	C.StrokeRoundRect(R, 8.0f, Hex(0x9aa3b0), 6.0f);
	// A push range on the 13 x 13 grid (aces top left; suited above the diagonal): the pairs to fives, every suited ace,
	// suited broadways and connectors, the big offsuit aces in; the edge of it amber. A tile draws each row as two bands.
	const float Cell = std::min((R.W * 0.62f) / 13.0f, (R.H - 28.0f) / 13.0f);
	const float Gx = R.X + 14.0f, Gy = R.Y + 14.0f;
	C.FillRect({Gx - 3.0f, Gy - 3.0f, Cell * 13.0f + 6.0f, Cell * 13.0f + 6.0f}, Hex(0xffffff));
	auto Range = [](int I, int J) {
		if (I == J)
		{
			return I <= 9 ? 2 : I <= 10 ? 1 : 0;
		}
		if (J > I)
		{
			return I == 0 || (I <= 4 && J <= I + 3) ? 2 : I <= 6 && J <= I + 4 ? 1 : 0;
		}
		return (J == 0 && I <= 8) || (J <= 2 && I <= 4) ? 2 : J <= 1 && I <= 11 ? 1 : 0;
	};
	static const uint32_t Cols[3] = {0xd9dde3, 0xf2c14e, 0x2fbf71};
	for (int I = 0; I < 13; ++I)
	{
		if (X.Tier > 0)
		{
			int In = 0;
			for (int J = 0; J < 13; ++J)
			{
				In += Range(I, J) > 0 ? 1 : 0;
			}
			const float Wi = Cell * static_cast<float>(In);
			C.FillRect({Gx, Gy + static_cast<float>(I) * Cell, Wi, Cell - 1.0f}, Hex(Cols[2]));
			C.FillRect({Gx + Wi, Gy + static_cast<float>(I) * Cell, Cell * 13.0f - Wi, Cell - 1.0f}, Hex(Cols[0]));
			continue;
		}
		for (int J = 0; J < 13; ++J)
		{
			C.FillRect({Gx + static_cast<float>(J) * Cell, Gy + static_cast<float>(I) * Cell, Cell - 1.0f, Cell - 1.0f}, Hex(Cols[Range(I, J)]));
		}
	}
	if (X.Mine.size() >= 2)
	{
		// The hand the lesson is about, by its class ("AQs").
		Plain(X, HandClassLabel(HandClass(X.Mine[0], X.Mine[1])), R.X + R.W * 0.82f, R.Y + R.H * 0.36f, 30.0f, Hex(0x1d2b66), Align::Center);
	}
}

void Felt(Ctx& X, float Cx, float Cy, float Rx, float Ry)
{
	Canvas& C = *X.C;
	const float Px = Mx(X, Cx);
	C.FillEllipse(Px, Cy + 8.0f, Rx + 16.0f, Ry + 16.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillEllipse(Px, Cy, Rx + 16.0f, Ry + 16.0f, Paint::Linear({Px, Cy - Ry}, {Px, Cy + Ry}, Hex(0x5a3a22), Hex(0x2a170a)));
	// The cloth: a linear fall-off and one soft light (a radial fill would be six rings of vertices).
	C.FillEllipse(Px, Cy, Rx, Ry, Paint::Linear({Px, Cy - Ry}, {Px, Cy + Ry}, Hex(0x14845a), Hex(0x074a2f)));
	Glow(C, Px, Cy - Ry * 0.2f, Rx * 0.8f, Hex(0x2fbf71), 0.35f);
	C.StrokeEllipse(Px, Cy, Rx - 14.0f, Ry - 12.0f, Hex(0xffd479, 0.35f), 2.0f);
}

/** A bus from the side (the road trip). */
void Bus(Canvas& C, float L, float T, float W, const Color& Body)
{
	const float H = W * 0.42f;
	C.FillRoundRect({L + 6.0f, T + 10.0f, W, H}, H * 0.12f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect({L, T, W, H}, H * 0.12f, Paint::Linear({0.0f, T}, {0.0f, T + H}, Mix(Body, Hex(0xffffff), 0.15f), Mix(Body, Hex(0x000000), 0.35f)));
	for (int K = 0; K < 6; ++K)
	{
		C.FillRoundRect({L + W * 0.05f + static_cast<float>(K) * W * 0.145f, T + H * 0.14f, W * 0.12f, H * 0.34f}, 3.0f, Paint::Linear({0.0f, T}, {0.0f, T + H * 0.5f}, Hex(0xffe9a8), Hex(0xd9a440)));
	}
	C.FillRect({L, T + H * 0.6f, W, H * 0.06f}, Hex(0xffffff, 0.8f));
	for (const float Wx : {0.2f, 0.78f})
	{
		C.FillCircle(L + W * Wx, T + H, H * 0.17f, Hex(0x15151a));
		C.FillCircle(L + W * Wx, T + H, H * 0.08f, Hex(0x8a8f99));
	}
}

/** The marquee over the Embercrest's door: an arch of bulbs around the name. Centre after the mirror. */
void Marquee(Ctx& X, float Cx, float Top, float W)
{
	Canvas& C = *X.C;
	const float H = W * 0.32f;
	C.FillRoundRect({Cx - W * 0.5f, Top, W, H}, H * 0.3f, Paint::Linear({0.0f, Top}, {0.0f, Top + H}, Hex(0x3a1410), Hex(0x170605)));
	C.StrokeRoundRect({Cx - W * 0.5f, Top, W, H}, H * 0.3f, Hex(0xf5c542), 4.0f);
	const int N = Many(X, 22, 14, 8);
	for (int I = 0; I < N; ++I)
	{
		const float T = static_cast<float>(I) / static_cast<float>(N - 1);
		const float Bx = Cx - W * 0.46f + T * W * 0.92f;
		C.FillCircle(Bx, Top + H * 0.12f, 3.2f, Hex(0xffe9a8));
		C.FillCircle(Bx, Top + H * 0.88f, 3.2f, Hex(0xffe9a8));
	}
	const float Sz = Fit(C, "EMBERCREST", H * 0.46f, W * 0.84f, false);
	Loud(X, "EMBERCREST", Cx, Top + H * 0.5f + Sz * 0.36f, Sz, Hex(0xffd479), Hex(0x3a1410), Align::Center, false);
}

// ------------------------------------------------------------------ the fact-reading pieces (boxes before the mirror)

/** The challenge's bar: its frame, filled to the p fact (no fact: the frame alone, never an invented fill). */
void ProgressAt(Ctx& X, float L, float T, float W, float H)
{
	Canvas& C = *X.C;
	const float X0 = Mleft(X, L, W);
	C.FillRoundRect({X0, T, W, H}, H * 0.5f, Mix(X.P.B, Hex(0x000000), 0.35f));
	C.StrokeRoundRect({X0, T, W, H}, H * 0.5f, WithA(Hex(0xffffff), 0.5f), 2.0f);
	if (X.Progress >= 0)
	{
		const float In = H * 0.14f;
		const float Fw = std::max(H - In * 2.0f, (W - In * 2.0f) * static_cast<float>(X.Progress) / 100.0f);
		C.FillRoundRect({X.Mirror ? X0 + W - In - Fw : X0 + In, T + In, Fw, H - In * 2.0f}, (H - In * 2.0f) * 0.5f, X.P.Acc);
	}
}

/**
 * The bankroll line: the g fact's points ("u" or "d", then up to 8 digits 0-9, oldest first), else the captions' sign as
 * a plain rise or fall. Returns the line's last point (after the mirror) for a sticker to aim at.
 */
Vec2 GraphAt(Ctx& X, float L, float T, float W, float H)
{
	Canvas& C = *X.C;
	std::vector<float> Vals;
	bool Up = !(Negative(X.Cap1) || Negative(X.Cap2));
	if (!X.Graph.empty())
	{
		Up = X.Graph[0] != 'd';
		for (size_t I = 1; I < X.Graph.size() && Vals.size() < 8; ++I)
		{
			if (X.Graph[I] >= '0' && X.Graph[I] <= '9')
			{
				Vals.push_back(static_cast<float>(X.Graph[I] - '0') / 9.0f);
			}
		}
	}
	const Color Line = Up ? Hex(0x3dff8a) : Hex(0xff6b6b);
	const float X0 = Mleft(X, L, W);
	C.FillRoundRect({X0, T, W, H}, 8.0f, Color{0.0f, 0.0f, 0.0f, 0.38f});
	for (int K = 1; K < 4; ++K)
	{
		C.FillRect({X0, T + H * static_cast<float>(K) / 4.0f, W, 1.0f}, Color{1.0f, 1.0f, 1.0f, 0.12f});
	}
	Pts P;
	if (Vals.size() >= 2)
	{
		// The fact's points, joined by a little of the month's noise between them.
		const int Per = Many(X, 4, 3, 2);
		const int N = static_cast<int>(Vals.size() - 1) * Per;
		for (int I = 0; I <= N; ++I)
		{
			const int Seg = std::min(static_cast<int>(Vals.size()) - 2, I / Per);
			const float F = static_cast<float>(I - Seg * Per) / static_cast<float>(Per);
			const float Noise = I % Per == 0 ? 0.0f : (Hf(X, 1400u + static_cast<uint32_t>(I)) - 0.5f) * 0.12f;
			const float V = Vals[static_cast<size_t>(Seg)] + (Vals[static_cast<size_t>(Seg) + 1] - Vals[static_cast<size_t>(Seg)]) * F + Noise;
			const float Tt = static_cast<float>(I) / static_cast<float>(N);
			P.push_back({X0 + Tt * W,T + H - (0.08f + 0.84f * std::clamp(V, 0.0f, 1.0f)) * H});
		}
	}
	else
	{
		const int N = Many(X, 30, 20, 12);
		for (int I = 0; I <= N; ++I)
		{
			const float Tt = static_cast<float>(I) / static_cast<float>(N);
			const float Noise = (Hf(X, 1400u + static_cast<uint32_t>(I)) - 0.5f) * 0.2f;
			const float V = Up ? 0.15f + 0.7f * Tt + Noise : 0.85f - 0.72f * Tt + Noise;
			P.push_back({X0 + Tt * W,T + H - std::clamp(V, 0.03f, 0.97f) * H});
		}
	}
	// The area under the line, as strips (the polygon isn't convex).
	for (size_t I = 0; I + 1 < P.size(); ++I)
	{
		C.FillPolygon({P[I], P[I + 1], {P[I + 1].X, T + H}, {P[I].X, T + H}}, WithA(Line, 0.16f));
	}
	C.StrokePolyline(P, false, Color{0.0f, 0.0f, 0.0f, 0.35f}, H * 0.05f + 1.0f, true);
	C.StrokePolyline(P, false, Line, H * 0.032f + 0.6f, true);
	C.FillCircle(P.back().X, P.back().Y, H * 0.04f + 1.0f, Hex(0xffffff));
	C.FillCircle(P.back().X, P.back().Y, H * 0.022f + 0.6f, Line);
	return P.back();
}

/** A sniper's crosshair (after the mirror). */
void Crosshair(Canvas& C, float Cx, float Cy, float R)
{
	for (int Pass = 0; Pass < 2; ++Pass)
	{
		const Color Col = Pass == 0 ? Color{1.0f, 1.0f, 1.0f, 0.85f} : Hex(HotRed);
		const float W = (Pass == 0 ? 10.0f : 5.0f) * R / 110.0f + 0.5f;
		C.StrokeEllipse(Cx, Cy, R, R, Col, W);
		C.StrokeEllipse(Cx, Cy, R * 0.24f, R * 0.24f, Col, W * 0.8f);
		C.StrokePolyline({{Cx - R * 1.36f, Cy}, {Cx - R * 0.55f, Cy}}, false, Col, W);
		C.StrokePolyline({{Cx + R * 0.55f, Cy}, {Cx + R * 1.36f, Cy}}, false, Col, W);
		C.StrokePolyline({{Cx, Cy - R * 1.36f}, {Cx, Cy - R * 0.55f}}, false, Col, W);
		C.StrokePolyline({{Cx, Cy + R * 0.55f}, {Cx, Cy + R * 1.36f}}, false, Col, W);
	}
}

/** CALL and FOLD, side by side (Stacked: one above the other). */
void OptionCards(Ctx& X, float L, float T, float W, float H, bool Stacked = false)
{
	Canvas& C = *X.C;
	const Upright Keep(X, L, L + W); // CALL before FOLD on either side of the mirror
	for (int K = 0; K < 2; ++K)
	{
		const float Bw = Stacked ? W : W * 0.47f;
		const float Bh = Stacked ? H * 0.47f : H;
		const float Bl = Mleft(X, Stacked ? L : L + static_cast<float>(K) * W * 0.53f, Bw);
		const float Bt = Stacked ? T + static_cast<float>(K) * H * 0.53f : T;
		C.FillRoundRect({Bl + 3.0f, Bt + 4.0f, Bw, Bh}, Bh * 0.18f, Color{0.0f, 0.0f, 0.0f, 0.3f});
		C.FillRoundRect({Bl, Bt, Bw, Bh}, Bh * 0.18f, Hex(K == 0 ? 0x2fbf71 : 0xe0283c));
		C.StrokeRoundRect({Bl, Bt, Bw, Bh}, Bh * 0.18f, Hex(0xffffff), std::max(2.0f, Bh * 0.07f));
		const char* Word = K == 0 ? "CALL" : "FOLD";
		Plain(X, Word, Bl + Bw * 0.5f, Bt + Bh * 0.5f + Bh * 0.18f, Fit(C, Word, Bh * 0.5f, Bw * 0.8f, false), Hex(0xffffff), Align::Center);
	}
}

const uint32_t TierCols[5] = {0xff5a5a, 0xff9f40, 0xffd23f, 0x5ad46a, 0x4fc3f7};

/** The tier list's rows S, A, B, C (the r fact's names in them, else the video's cards, chips and trophies as icons). */
void TierRows(Ctx& X, float L, float T, float W, float H, int N)
{
	Canvas& C = *X.C;
	static const char* const Tiers[5] = {"S", "A", "B", "C", "D"};
	const float X0 = Mleft(X, L, W);
	const float Rh = H / static_cast<float>(N);
	for (int K = 0; K < N; ++K)
	{
		const float Ty = T + static_cast<float>(K) * Rh;
		C.FillRect({X0, Ty, W, Rh - 4.0f}, Color{0.0f, 0.0f, 0.0f, 0.4f});
		const float Lab = X.Mirror ? X0 + W - (Rh - 4.0f) : X0;
		C.FillRect({Lab, Ty, Rh - 4.0f, Rh - 4.0f}, Hex(TierCols[K]));
		Plain(X, Tiers[K], Lab + (Rh - 4.0f) * 0.5f, Ty + (Rh - 4.0f) * 0.74f, (Rh - 4.0f) * 0.7f, Hex(0x111111), Align::Center);
		const float Ix0 = X.Mirror ? X0 + 8.0f : X0 + Rh + 6.0f;
		if (static_cast<size_t>(K) < X.Rows.size())
		{
			const float Sz = Fit(C, X.Rows[static_cast<size_t>(K)], (Rh - 4.0f) * 0.52f, W - Rh - 16.0f, false);
			Plain(X, X.Rows[static_cast<size_t>(K)], X.Mirror ? X0 + W - Rh - 8.0f : Ix0, Ty + (Rh - 4.0f) * 0.68f, Sz, Hex(0xffffff), X.Mirror ? Align::Right : Align::Left);
			continue;
		}
		for (int I = 0; I < 4 - K / 2; ++I)
		{
			const float Ix = X.Mirror ? X0 + W - Rh - 8.0f - static_cast<float>(I + 1) * Rh * 0.9f : Ix0 + static_cast<float>(I) * Rh * 0.9f;
			const float Cy = Ty + (Rh - 4.0f) * 0.5f;
			const float U = (Rh - 4.0f) / 62.0f;
			switch ((K + I) % 3)
			{
			case 0: DrawCard(C, K == 0 && static_cast<size_t>(I) < X.Mine.size() ? X.Mine[static_cast<size_t>(I)] : -1, Ix, Ty + 6.0f * U, 36.0f * U, 50.0f * U); break;
			case 1: ChipAt(C, Ix + 18.0f * U, Cy, 17.0f * U, Hex(ChipFaces[(K + I) % 6]), X.Tier == 0); break;
			default: Trophy(C, Ix + 18.0f * U, Cy, 40.0f * U, Hex(0xf5c542)); break;
			}
		}
	}
}

/** Five numbered cells, a crown on the first; the r fact's rows in them (else blank bars). */
void NumberedCells(Ctx& X, float L, float T, float W, float H)
{
	Canvas& C = *X.C;
	static const char* const Nums[5] = {"1", "2", "3", "4", "5"};
	const float Rh = H / 5.0f;
	const float X0 = Mleft(X, L, W);
	for (int K = 0; K < 5; ++K)
	{
		const float Ty = T + static_cast<float>(K) * Rh;
		const float Cw = W - static_cast<float>(K) * W * 0.06f;
		const float Cl = X.Mirror ? X0 + W - Cw : X0;
		C.FillRoundRect({Cl, Ty, Cw, Rh * 0.84f}, Rh * 0.14f, K == 0 ? WithA(X.P.Acc, 0.95f) : Color{0.0f, 0.0f, 0.0f, 0.42f});
		const Color Ink = K == 0 ? Mix(X.P.B, Hex(0x000000), 0.3f) : Hex(0xffffff);
		const float Nx = X.Mirror ? Cl + Cw - Rh * 0.5f : Cl + Rh * 0.5f;
		Plain(X, Nums[K], Nx, Ty + Rh * 0.66f, Rh * 0.62f, Ink, Align::Center);
		if (static_cast<size_t>(K) < X.Rows.size())
		{
			const float Sz = Fit(C, X.Rows[static_cast<size_t>(K)], Rh * 0.46f, Cw - Rh * 1.3f, false);
			Plain(X, X.Rows[static_cast<size_t>(K)], X.Mirror ? Cl + Cw - Rh : Cl + Rh, Ty + Rh * 0.6f, Sz, Ink, X.Mirror ? Align::Right : Align::Left);
		}
		else
		{
			for (int I = 0; I < 3; ++I)
			{
				const float Bw = Cw * 0.18f - static_cast<float>(I) * Cw * 0.02f;
				const float Bx = X.Mirror ? Cl + Cw - Rh * 1.1f - static_cast<float>(I + 1) * Cw * 0.21f : Cl + Rh * 1.1f + static_cast<float>(I) * Cw * 0.21f;
				C.FillRoundRect({Bx, Ty + Rh * 0.32f, Bw, Rh * 0.22f}, Rh * 0.11f, K == 0 ? Color{0.0f, 0.0f, 0.0f, 0.3f} : Color{1.0f, 1.0f, 1.0f, 0.25f});
			}
		}
	}
	Crown(C, X.Mirror ? X0 + W - Rh * 0.5f : X0 + Rh * 0.5f, T - Rh * 0.12f, Rh * 0.32f, Hex(Yellow));
}

/** Confetti in a box (never a platform's plaque). */
void Confetti(Ctx& X, float L, float T, float W, float H, int N)
{
	Canvas& C = *X.C;
	const uint32_t Cols[5] = {0xff6b57, 0x4fc3f7, 0xffd23f, 0x8b5cff, 0x2fbf71};
	for (int I = 0; I < N; ++I)
	{
		const float Cx = Mx(X, L + Hf(X, 2000u + static_cast<uint32_t>(I)) * W);
		const float Cy = T + Hf(X, 2100u + static_cast<uint32_t>(I)) * H;
		const float A = Hf(X, 2200u + static_cast<uint32_t>(I)) * Pi;
		const float Cs = std::cos(A) * 7.0f, Sn = std::sin(A) * 7.0f;
		C.FillPolygon({{Cx - Cs, Cy - Sn}, {Cx + Sn * 0.4f, Cy - Cs * 0.4f}, {Cx + Cs, Cy + Sn}, {Cx - Sn * 0.4f, Cy + Cs * 0.4f}}, Hex(Cols[I % 5]));
	}
}

/** The FUSE reel: a lit window with the multiple (the m fact) big in it; no fact, the window dark. */
void Reel(Ctx& X, float L, float T, float W, float H, const std::string& Text, bool White = false)
{
	Canvas& C = *X.C;
	const float X0 = Mleft(X, L, W);
	C.FillRoundRect({X0, T, W, H}, H * 0.08f, Mix(X.P.B, Hex(0x000000), 0.3f));
	C.StrokeRoundRect({X0, T, W, H}, H * 0.08f, X.P.Acc, std::max(2.0f, H * 0.024f));
	C.FillRoundRect({X0 + W * 0.07f, T + H * 0.1f, W * 0.86f, H * 0.8f}, H * 0.05f, Mix(X.P.B, Hex(0x000000), 0.6f));
	const float Sz = Text.empty() ? H * 0.3f : Fit(C, Text, H * 0.32f, W * 0.8f, true);
	C.StrokeRoundRect({X0 + W * 0.07f, T + H * 0.5f - Sz * 0.62f, W * 0.86f, Sz * 1.24f}, 4.0f, Hex(0xff3d8b), std::max(2.0f, H * 0.02f));
	Loud(X, Text, X0 + W * 0.5f, T + H * 0.5f + Sz * 0.36f, Sz, White ? Hex(0xffffff) : X.P.Acc, Hex(0x000000), Align::Center, true);
}

/** The recipe's prop at a slot (centre before the mirror, S its size). */
void PropAt(Ctx& X, int Which, float Cx, float Cy, float S)
{
	Canvas& C = *X.C;
	const float Px = Mx(X, Cx);
	switch (static_cast<Prop>(Which))
	{
	case Prop::HoleCards: UpRow(X, X.Mine, 2, Cx - S * 0.48f, Cy - S * 0.4f, S * 0.44f, S * 0.06f); break;
	case Prop::ShownCards: UpRow(X, X.Theirs, 2, Cx - S * 0.48f, Cy - S * 0.4f, S * 0.44f, S * 0.06f); break;
	case Prop::CardBacks:
		UpRow(X, std::vector<Card>(), 2, Cx - S * 0.48f, Cy - S * 0.4f, S * 0.44f, S * 0.06f);
		Loud(X, "??", Px, Cy + S * 0.18f, S * 0.5f, Hex(Yellow), Hex(0x000000), Align::Center, false);
		break;
	case Prop::Board: UpRow(X, X.Board, std::max<size_t>(3, std::min<size_t>(5, X.Board.size())), Cx - S * 0.9f, Cy - S * 0.24f, S * 0.32f, S * 0.04f); break;
	case Prop::CardFan:
	{
		std::vector<Card> Hand = MadeHand(X);
		if (Hand.size() < 2)
		{
			Hand = {-1, -1};
		}
		Fan(X, Hand, Cx, Cy, S * 0.4f, 0.18f, false);
		break;
	}
	case Prop::ChipPile: ChipPile(X, Cx, Cy, S * 0.6f, Many(X, 12, 8, 5)); break;
	case Prop::CashStack: CashStack(C, Px, Cy + S * 0.3f, S, Many(X, 7, 5, 4)); break;
	case Prop::Trophy: Trophy(C, Px, Cy, S * 0.9f, Hex(0xf5c542)); break;
	case Prop::Bracelet:
		C.StrokeEllipse(Px, Cy, S * 0.5f, S * 0.28f, Hex(0xf5c542), S * 0.12f);
		C.FillCircle(Px, Cy - S * 0.28f, S * 0.12f, Hex(0x9fe8ff));
		break;
	case Prop::Ring:
		C.StrokeEllipse(Px, Cy + S * 0.1f, S * 0.32f, S * 0.32f, Hex(0xf5c542), S * 0.1f);
		C.FillPolygon({{Px, Cy - S * 0.42f}, {Px + S * 0.16f, Cy - S * 0.24f}, {Px, Cy - S * 0.12f}, {Px - S * 0.16f, Cy - S * 0.24f}}, Hex(0xbff3ff));
		break;
	case Prop::EventCrest:
	case Prop::Badge:
	case Prop::Shield:
	{
		// A crest (the event's name under it, the e fact), a badge, or a League shield with the division it reached (t).
		const Pts Shield = {{Px - S * 0.42f, Cy - S * 0.46f}, {Px + S * 0.42f, Cy - S * 0.46f}, {Px + S * 0.42f, Cy + S * 0.04f}, {Px, Cy + S * 0.5f}, {Px - S * 0.42f, Cy + S * 0.04f}};
		C.FillPolygon(Grow(Shield, 3.0f), Hex(0xffffff));
		C.FillPolygon(Shield, Paint::Linear({Px, Cy - S * 0.5f}, {Px, Cy + S * 0.5f}, Mix(X.P.Acc, Hex(0xffffff), 0.2f), Mix(X.P.Acc, Hex(0x000000), 0.4f)));
		C.FillPolygon(Star(Px, Cy - S * 0.04f, S * 0.22f, S * 0.09f, 5, -Pi * 0.5f), Hex(0xffffff));
		const std::string& Under = static_cast<Prop>(Which) == Prop::EventCrest ? X.Event : static_cast<Prop>(Which) == Prop::Shield ? X.To : std::string();
		// Not where the composition prints that fact already (Trophy Shot's base, Venue's marquee and Reaction Box's bottom
		// line carry e; Before -> After's panel carries t): one fact, told once (review C3).
		const Arch A = static_cast<Arch>(X.S->Arch);
		const bool Told = A == Arch::TrophyShot || A == Arch::Venue || A == Arch::ReactionBox || A == Arch::BeforeAfter;
		if (!Under.empty() && !Told)
		{
			Loud(X, Under, Px, Cy + S * 0.78f, Fit(C, Under, S * 0.2f, S * 2.2f, false), X.P.Acc, Hex(0x000000), Align::Center, false);
		}
		break;
	}
	case Prop::SiteMark:
	case Prop::SponsorMark:
		if (X.Site > 0 && X.Site < static_cast<int>(net::SiteId::Count) && static_cast<Prop>(Which) == Prop::SiteMark)
		{
			siteart::Logo(C, {Px - S * 0.45f, Cy - S * 0.45f, S * 0.9f, S * 0.9f}, static_cast<net::SiteId>(X.Site));
		}
		else
		{
			C.FillRoundRect({Px - S * 0.45f, Cy - S * 0.3f, S * 0.9f, S * 0.6f}, S * 0.12f, Hex(0xffffff));
			C.FillPolygon(Star(Px, Cy, S * 0.2f, S * 0.08f, 5, -Pi * 0.5f), X.P.B);
		}
		break;
	case Prop::VenueMark: Marquee(X, Px, Cy - S * 0.2f, S * 1.4f); break;
	case Prop::Gear:
		// The desk's gear: a monitor and a microphone on its arm.
		Monitor(C, {Px - S * 0.55f, Cy - S * 0.4f, S * 0.8f, S * 0.5f}, Hex(0x0f5a3c));
		C.StrokePolyline({{Px + S * 0.55f, Cy + S * 0.3f}, {Px + S * 0.42f, Cy - S * 0.05f}, {Px + S * 0.3f, Cy - S * 0.15f}}, false, Hex(0x2a2e37), S * 0.05f, true);
		C.FillRoundRect({Px + S * 0.2f, Cy - S * 0.42f, S * 0.18f, S * 0.32f}, S * 0.09f, Hex(0x3a3f4a));
		break;
	case Prop::Flag:
		C.FillRect({Px - S * 0.4f, Cy - S * 0.45f, S * 0.04f, S * 0.9f}, Hex(0xd0d4dc));
		C.FillPolygon({{Px - S * 0.36f, Cy - S * 0.45f}, {Px + S * 0.42f, Cy - S * 0.3f}, {Px - S * 0.36f, Cy - S * 0.12f}}, X.P.Acc);
		break;
	case Prop::EvictionNotice: Paper(X, {Cx - S * 0.5f, Cy - S * 0.62f, S, S * 1.24f}, "NOTICE TO VACATE", 5, std::string()); break;
	case Prop::PowerBill: Paper(X, {Cx - S * 0.5f, Cy - S * 0.62f, S, S * 1.24f}, "POWER BILL", 5, std::string()); break;
	case Prop::StickyNote:
		C.FillRect({Px - S * 0.4f + 4.0f, Cy - S * 0.4f + 5.0f, S * 0.8f, S * 0.8f}, Color{0.0f, 0.0f, 0.0f, 0.3f});
		C.FillRect({Px - S * 0.4f, Cy - S * 0.4f, S * 0.8f, S * 0.8f}, Hex(0xffe66b));
		for (int K = 0; K < 3; ++K)
		{
			C.FillRect({Px - S * 0.3f, Cy - S * 0.18f + static_cast<float>(K) * S * 0.18f, S * (0.6f - 0.1f * static_cast<float>(K)), 3.0f}, Hex(0x7a6a20));
		}
		break;
	case Prop::NeonSign:
		if (!X.Lod)
		{
			C.GlowRoundRect({Px - S * 0.5f, Cy - S * 0.3f, S, S * 0.6f}, S * 0.15f, WithA(X.P.Acc, 0.8f), 14.0f);
		}
		C.StrokeRoundRect({Px - S * 0.5f, Cy - S * 0.3f, S, S * 0.6f}, S * 0.15f, Mix(X.P.Acc, Hex(0xffffff), 0.5f), 4.0f);
		DrawSuit(C, 3, Px - S * 0.18f, Cy - S * 0.2f, S * 0.36f, Mix(X.P.Acc, Hex(0xffffff), 0.5f));
		break;
	case Prop::PawnTicket:
		C.FillRoundRect({Px - S * 0.45f, Cy - S * 0.22f, S * 0.9f, S * 0.44f}, 4.0f, Hex(0xf0d9a8));
		C.FillCircle(Px - S * 0.45f, Cy, S * 0.08f, X.P.B);
		C.FillCircle(Px + S * 0.45f, Cy, S * 0.08f, X.P.B);
		Plain(X, "PAWN", Px, Cy + S * 0.08f, S * 0.22f, Hex(0x5a3a10), Align::Center);
		break;
	case Prop::Bus: Bus(C, Px - S * 0.6f, Cy - S * 0.25f, S * 1.2f, Hex(0x2b6cb0)); break;
	case Prop::LaptopLobby: Laptop(C, Px, Cy, S * 0.5f, X.P.Acc, 4); break;
	case Prop::PhoneChat:
	{
		C.FillRoundRect({Px - S * 0.26f, Cy - S * 0.5f, S * 0.52f, S}, S * 0.08f, Hex(0x16181d));
		C.FillRect({Px - S * 0.22f, Cy - S * 0.42f, S * 0.44f, S * 0.84f}, Hex(0x1a2433));
		for (int K = 0; K < 3; ++K)
		{
			const float Y = Cy - S * 0.3f + static_cast<float>(K) * S * 0.24f;
			C.FillRoundRect({Px - S * 0.18f + (K == 1 ? S * 0.1f : 0.0f), Y, S * 0.26f, S * 0.14f}, S * 0.05f, K == 1 ? X.P.Acc : Hex(0x3a4a60));
		}
		break;
	}
	case Prop::Whiteboard: Whiteboard(X, {Cx - S * 0.6f, Cy - S * 0.42f, S * 1.2f, S * 0.84f}); break;
	case Prop::FeltTop: Felt(X, Cx, Cy, S * 0.7f, S * 0.36f); break;
	case Prop::MysteryEnvelope:
	{
		const Rect R{Mleft(X, Cx - S * 0.5f, S), Cy - S * 0.32f, S, S * 0.64f};
		Envelope(C, R, true);
		Plain(X, "?", R.X + R.W * 0.5f, R.Y + R.H * 0.86f, S * 0.36f, Hex(0xb08a3a), Align::Center);
		break;
	}
	case Prop::Statement: Paper(X, {Cx - S * 0.45f, Cy - S * 0.6f, S * 0.9f, S * 1.2f}, "STATEMENT", 6, std::string()); break;
	case Prop::ProgressBar: ProgressAt(X, Cx - S, Cy - S * 0.12f, S * 2.0f, S * 0.24f); break;
	case Prop::GraphLine: GraphAt(X, Cx - S, Cy - S * 0.6f, S * 2.0f, S * 1.2f); break;
	case Prop::Crosshair: Crosshair(C, Px, Cy, S * 0.6f); break;
	case Prop::OptionCards: OptionCards(X, Cx - S, Cy - S * 0.3f, S * 2.0f, S * 0.6f); break;
	case Prop::TierRows: TierRows(X, Cx - S, Cy - S * 0.6f, S * 2.0f, S * 1.2f, 4); break;
	case Prop::NumberedCells: NumberedCells(X, Cx - S, Cy - S * 0.6f, S * 2.0f, S * 1.2f); break;
	case Prop::Confetti: Confetti(X, Cx - S, Cy - S * 0.6f, S * 2.0f, S * 1.2f, Many(X, 16, 10, 6)); break;
	case Prop::FuseReel: Reel(X, Cx - S, Cy - S * 0.6f, S * 2.0f, S * 1.2f, X.Mult.empty() ? std::string() : "\xC3\x97" + X.Mult); break;
	default: break;
	}
}

/**
 * Whether the archetype's composition already draws this prop: the generator deals each archetype's defining prop as its
 * PropA (Classroom's whiteboard, Headhunter's crosshair, Road Trip's bus...), and drawn again at a prop slot it would be a
 * second, smaller copy of the thumbnail's own subject (review C3).
 */
bool Composed(const Ctx& X, int Which)
{
	const Prop P = static_cast<Prop>(Which);
	switch (static_cast<Arch>(X.S->Arch))
	{
	case Arch::GutPunch:
	case Arch::VsSplit: return P == Prop::HoleCards || P == Prop::ShownCards;
	case Arch::CircledRiver: return P == Prop::Board;
	case Arch::TheRead: return P == Prop::HoleCards || P == Prop::CardBacks;
	case Arch::NervesOfSteel:
	case Arch::Caught:
	case Arch::Mistake: return P == Prop::HoleCards;
	case Arch::TheFold: return P == Prop::HoleCards || P == Prop::OptionCards;
	case Arch::ThePile: return P == Prop::FeltTop || P == Prop::ChipPile;
	case Arch::TheBubble: return P == Prop::ChipPile; // its teetering stack is the short stack's chips
	case Arch::Headhunter: return P == Prop::Crosshair;
	case Arch::Envelope: return P == Prop::MysteryEnvelope;
	case Arch::TheGraph: return P == Prop::GraphLine;
	case Arch::DayCounter: return P == Prop::ProgressBar;
	case Arch::NightShift: return P == Prop::LaptopLobby;
	case Arch::Venue: return P == Prop::VenueMark;
	case Arch::RoadTrip: return P == Prop::Bus;
	case Arch::Classroom: return P == Prop::Whiteboard;
	case Arch::Quiz: return P == Prop::OptionCards;
	case Arch::TierList: return P == Prop::TierRows;
	case Arch::TopFive: return P == Prop::NumberedCells;
	case Arch::DeskSetup: return P == Prop::Gear || P == Prop::LaptopLobby; // the desk has the laptop (C3 fix L7)
	case Arch::Milestone: return P == Prop::Confetti;
	case Arch::MonsterHand: return P == Prop::CardFan;
	case Arch::SiteCard: return P == Prop::SiteMark || P == Prop::FuseReel;
	default: return false;
	}
}

/** PropA and PropB at the archetype's two prop slots (before the mirror); never one the composition already draws. */
void Props(Ctx& X, float Ax, float Ay, float As, float Bx, float By, float Bs)
{
	if (X.S->PropA >= 0 && X.S->PropA < static_cast<int>(Prop::Count) && !Composed(X, X.S->PropA))
	{
		PropAt(X, X.S->PropA, Ax, Ay, As);
	}
	if (X.S->PropB >= 0 && X.S->PropB < static_cast<int>(Prop::Count) && X.S->PropB != X.S->PropA && !Composed(X, X.S->PropB))
	{
		PropAt(X, X.S->PropB, Bx, By, Bs);
	}
}

// ------------------------------------------------------------------ captions (8 styles)

/** Where an archetype puts its words: line 1's baseline at (L .. L + W, Base), Size the most it wants. Before the mirror. */
struct Box
{
	float L = 0.0f, Base = 0.0f, W = 0.0f, Size = 60.0f;
	Align A = Align::Left;
};

/** Line 1 and line 2 in the recipe's caption style. */
void Captions(Ctx& X, const Box& B, const std::string& L1, const std::string& L2)
{
	Canvas& C = *X.C;
	const CapStyle St = static_cast<CapStyle>(X.S->CapStyle >= 0 && X.S->CapStyle < static_cast<int>(CapStyle::Count) ? X.S->CapStyle : 0);
	if (St == CapStyle::None || (L1.empty() && L2.empty()))
	{
		return;
	}
	const bool Italic = (X.V & 0x10u) != 0;
	const Align A = Ma(X, B.A);
	const float Left = Mleft(X, B.L, B.W);
	const float Ax = A == Align::Left ? Left : A == Align::Right ? Left + B.W : Left + B.W * 0.5f;
	// A plate (Boxed) or a marker bar is the words and half their size again: the words fit the box less that (review C3:
	// mirrored, a full-width line ran off the frame's edge).
	const float PlateK = St == CapStyle::Boxed ? 0.52f : St == CapStyle::Marker ? 0.5f : 0.0f;
	float S1 = Fit(C, L1, B.Size, B.W, Italic);
	S1 = PlateK > 0.0f ? Fit(C, L1, S1, B.W - PlateK * S1, Italic) : S1;
	float S2 = L2.empty() ? 0.0f : Fit(C, L2, std::min(B.Size * 0.56f, S1 * 0.72f), B.W, Italic);
	S2 = PlateK > 0.0f && !L2.empty() ? Fit(C, L2, S2, B.W - PlateK * S2, Italic) : S2;
	// Two lines that would run off the bottom lift together (a plate or a bar below line 2 counted).
	float Base1 = B.Base;
	if (!L2.empty())
	{
		const float Low = Base1 + S2 * 1.12f + S2 * (St == CapStyle::Boxed || St == CapStyle::Marker ? 0.62f : 0.3f);
		Base1 -= std::max(0.0f, Low - 352.0f);
	}
	const float Base2 = Base1 + S2 * 1.12f;
	auto Edges = [&](float W, float Size, float& X0) {
		X0 = A == Align::Left ? Ax : A == Align::Right ? Ax - W : Ax - W * 0.5f;
		(void)Size;
	};
	switch (St)
	{
	case CapStyle::TwoTone:
	{
		// The first tone is the accent; a white accent (Ice, Blood) gives yellow, so two tones never read as one (C3 fix M1:
		// on those palettes Two-tone drew as Outlined, and Studio's style picker changed nothing).
		const bool WhiteAcc = X.P.Acc.R > 0.95f && X.P.Acc.G > 0.95f && X.P.Acc.B > 0.95f;
		Loud(X, L1, Ax, Base1, S1, WhiteAcc ? Hex(Yellow) : X.P.Acc, Hex(0x000000), A, Italic);
		Loud(X, L2, Ax, Base2, S2, Hex(0xffffff), Hex(0x000000), A, Italic);
		break;
	}
	case CapStyle::Boxed:
	{
		const Color Plate = (X.V & 0x20u) != 0 ? X.P.Acc : Hex(0xffffff);
		const Color Ink = Mix(X.P.B, Hex(0x000000), 0.3f);
		for (int Line = 0; Line < 2; ++Line)
		{
			const std::string& S = Line == 0 ? L1 : L2;
			if (S.empty())
			{
				continue;
			}
			const float Sz = Line == 0 ? S1 : S2;
			const float Base = Line == 0 ? Base1 : Base2 + Sz * 0.3f;
			const float Pad = Sz * 0.26f;
			const float W = Wide(C, S, Sz, Italic) + Pad * 2.0f;
			float X0 = 0.0f;
			Edges(W, Sz, X0);
			C.FillRoundRect({X0 + 5.0f, Base - Sz * 0.9f + 5.0f, W, Sz * 1.18f}, 4.0f, Color{0.0f, 0.0f, 0.0f, 0.4f});
			C.FillRoundRect({X0, Base - Sz * 0.9f, W, Sz * 1.18f}, 4.0f, Line == 0 ? Plate : Hex(0x111111));
			Plain(X, S, X0 + Pad, Base + Sz * 0.02f, Sz, Line == 0 ? Ink : Hex(0xffffff), Align::Left, Italic);
		}
		break;
	}
	case CapStyle::Marker:
	{
		const Color Bar = (X.V & 0x20u) != 0 ? X.P.Acc : Hex(Yellow);
		for (int Line = 0; Line < 2; ++Line)
		{
			const std::string& S = Line == 0 ? L1 : L2;
			if (S.empty())
			{
				continue;
			}
			const float Sz = Line == 0 ? S1 : S2;
			const float Base = Line == 0 ? Base1 : Base2 + Sz * 0.35f;
			const float W = Wide(C, S, Sz, Italic) + Sz * 0.5f;
			float X0 = 0.0f;
			Edges(W, Sz, X0);
			C.FillPolygon({{X0 - Sz * 0.18f, Base - Sz * 0.84f}, {X0 + W, Base - Sz * 0.94f}, {X0 + W + Sz * 0.1f, Base + Sz * 0.2f}, {X0 - Sz * 0.08f, Base + Sz * 0.26f}}, Line == 0 ? Bar : Hex(0xffffff));
			Plain(X, S, X0 + Sz * 0.08f, Base, Sz, Hex(0x14101c), Align::Left, Italic);
		}
		break;
	}
	case CapStyle::Stamp:
	{
		// A white plate takes a deep red ink (every palette's accent is a light colour: on white it wouldn't read).
		const bool LightPlate = (X.V & 0x40u) != 0;
		const Color Col = LightPlate ? Hex(0xd0142a) : (X.V & 0x20u) != 0 ? X.P.Acc : Hex(HotRed);
		const Color Plate = LightPlate ? Color{1.0f, 1.0f, 1.0f, 0.9f} : StampPlate(X);
		const std::string Both = L2.empty() ? L1 : L1 + " " + L2;
		// The stamp's box is the words and 0.9 of their size: fit the words to the box less that.
		float Sz = Fit(C, Both, B.Size * 0.62f, B.W, false);
		Sz = Fit(C, Both, Sz, B.W - Sz * 0.9f, false);
		const float W = Wide(C, Both, Sz, false) + Sz * 0.9f;
		float X0 = 0.0f;
		Edges(W, Sz, X0);
		StampBox(X, Both, X0 + W * 0.5f, B.Base - Sz * 0.35f, Sz, Col, &Plate);
		break;
	}
	case CapStyle::NumberBurst:
	{
		// The burst (R from its middle to its points, 0.86 R tall) fits the box's width, and a burst with line 2 under it
		// fits the frame's height: a wide caption shrinks, a low box lifts (review C3: line 2 ran under the frame's bottom
		// edge). One line keeps its place: its words inside, its points free to run off the edge.
		float Sz = Fit(C, L1, B.Size * 0.8f, 280.0f, false);
		const float Per = Wide(C, L1, Sz, false) / Sz * 0.62f + 0.45f;
		const float Under = L2.empty() ? 0.0f : S2 * 1.15f;
		const float MaxSz = std::min(B.W * 0.5f, (DesignH - 14.0f - Under) / 1.72f) / Per;
		if (Sz > MaxSz)
		{
			Sz = std::max(10.0f, std::floor(MaxSz * 0.5f) * 2.0f);
		}
		const float R = std::max(40.0f, Wide(C, L1, Sz, false) * 0.62f + Sz * 0.45f);
		float X0 = 0.0f;
		Edges(R * 2.0f, Sz, X0);
		const float Cx = X0 + R;
		const float Lo = 6.0f + (L2.empty() ? Sz * 0.36f : R * 0.86f);
		const float Hi = L2.empty() ? DesignH - 8.0f - Sz * 0.36f : DesignH - 8.0f - R * 0.86f - Under;
		const float Cy = std::clamp(B.Base - Sz * 0.35f, Lo, std::max(Lo, Hi));
		BurstPlate(X, L1, Cx, Cy, Sz, X.P.Acc, Mix(X.P.B, Hex(0x000000), 0.35f), 40.0f);
		if (!L2.empty())
		{
			// Under the burst, inside the box.
			const float W2 = Wide(C, L2, S2, Italic);
			const float C2 = std::clamp(Cx, Left + W2 * 0.5f, std::max(Left + W2 * 0.5f, Left + B.W - W2 * 0.5f));
			Loud(X, L2, C2, Cy + R * 0.86f + S2 * 0.9f, S2, Hex(0xffffff), Hex(0x000000), Align::Center, Italic);
		}
		break;
	}
	case CapStyle::LowerThird:
	{
		const float Y = 300.0f;
		C.FillRect({0.0f, Y, DesignW, 60.0f}, WithA(Hex(0x111111), 0.92f));
		C.FillRect({Mleft(X, 0.0f, 10.0f), Y, 10.0f, 60.0f}, X.P.Acc);
		const float Sz = Fit(C, L1, 36.0f, L2.empty() ? 580.0f : 380.0f, false);
		// Two lines read line 1 then line 2, left to right, on either side of the mirror (review C3: mirrored, "WHY THE"
		// ... "RIVER" read "RIVER ... WHY THE"); one line follows the mirror.
		const bool Flip = X.Mirror && L2.empty();
		Plain(X, L1, Flip ? DesignW - 28.0f : 28.0f, Y + 30.0f + Sz * 0.36f, Sz, Hex(0xffffff), Flip ? Align::Right : Align::Left);
		if (!L2.empty())
		{
			const float Sz2 = Fit(C, L2, 24.0f, 170.0f, false);
			Plain(X, L2, 616.0f, Y + 30.0f + Sz2 * 0.36f, Sz2, X.P.Acc, Align::Right);
		}
		break;
	}
	default: // Outlined
	{
		// One line is white: in the accent it drew as Two-tone's one line does, so Studio's style picker changed nothing (C3
		// fix M1). Two lines keep the variant's accent.
		const Color Fill = (X.V & 0x20u) != 0 && !L2.empty() ? X.P.Acc : Hex(0xffffff);
		Loud(X, L1, Ax, Base1, S1, Fill, Hex(0x000000), A, Italic);
		Loud(X, L2, Ax, Base2, S2, Fill, Hex(0x000000), A, Italic);
		break;
	}
	}
}

void Captions(Ctx& X, const Box& B)
{
	Captions(X, B, X.Cap1, X.Cap2);
}

/**
 * Big words (a composition's number, its day, its multiple) in the recipe's caption style. C3 fix M1: Big Number, Milestone,
 * Day Counter and the multiples drew their words one way whatever the style, so Studio's style picker changed nothing on
 * them. Outlined (and None: the words are the composition) in Fill with the Edge outline, as they drew; Two-tone white on
 * the accent's dark edge; a number burst behind them; a plate (Boxed); a stamp; a marker bar; a lower third's band. Ax is the
 * anchor after the mirror (A its alignment), Base the baseline, the words fit MaxW. Returns the size drawn.
 */
float BigStyled(Ctx& X, const std::string& S, float Ax, float Base, float Size, float MaxW, Align A, bool Italic, const Color& Fill, const Color& Edge)
{
	if (S.empty())
	{
		return 0.0f;
	}
	Canvas& C = *X.C;
	auto LeftOf = [&](float W) { return A == Align::Left ? Ax : A == Align::Right ? Ax - W : Ax - W * 0.5f; };
	const Color Ink = Mix(X.P.B, Hex(0x000000), 0.35f);
	switch (static_cast<CapStyle>(X.S->CapStyle))
	{
	case CapStyle::NumberBurst:
	{
		// (The burst is the words' width and half their size again: at most 300 wide words, so it never swallows the frame.)
		const float Sz = Fit(C, S, Size * 0.72f, std::min(MaxW * 0.62f, 300.0f), false);
		const float W = Wide(C, S, Sz, false);
		const float R = W * 0.62f + Sz * 0.42f;
		const float Cx = LeftOf(W) + W * 0.5f, Cy = Base - Sz * 0.36f;
		C.FillPolygon(Star(Cx + 5.0f, Cy + 6.0f, R, R * 0.8f, 18, 0.1f), Color{0.0f, 0.0f, 0.0f, 0.35f});
		C.FillPolygon(Star(Cx, Cy, R, R * 0.8f, 18, 0.1f), Hex(0xffffff));
		C.FillPolygon(Star(Cx, Cy, R - 6.0f, (R - 6.0f) * 0.8f, 18, 0.1f), X.P.Acc);
		Plain(X, S, Cx, Base, Sz, Ink, Align::Center);
		return Sz;
	}
	case CapStyle::Boxed:
	case CapStyle::LowerThird:
	{
		const bool Band = static_cast<CapStyle>(X.S->CapStyle) == CapStyle::LowerThird;
		float Sz = Fit(C, S, Size * 0.86f, MaxW, false);
		Sz = Fit(C, S, Sz, MaxW - Sz * 0.6f, false);
		const float Pad = Sz * 0.28f;
		const float W = Wide(C, S, Sz, false) + Pad * 2.0f;
		const float L = LeftOf(W);
		const Rect P{L, Base - Sz * 0.92f, W, Sz * 1.2f};
		C.FillRoundRect({P.X + 5.0f, P.Y + 6.0f, P.W, P.H}, Band ? 0.0f : 6.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
		C.FillRoundRect(P, Band ? 0.0f : 6.0f, Band ? WithA(Hex(0x111111), 0.92f) : (X.V & 0x20u) != 0 ? X.P.Acc : Hex(0xffffff));
		if (Band)
		{
			C.FillRect({X.Mirror ? P.X + P.W - Pad * 0.5f : P.X, P.Y, Pad * 0.5f, P.H}, X.P.Acc);
		}
		Plain(X, S, L + Pad, Base - Sz * 0.02f, Sz, Band ? Hex(0xffffff) : Mix(X.P.B, Hex(0x000000), 0.3f), Align::Left);
		return Sz;
	}
	case CapStyle::Stamp:
	{
		const bool LightPlate = (X.V & 0x40u) != 0;
		const Color Col = LightPlate ? Hex(0xd0142a) : Hex(HotRed);
		const Color Plate = LightPlate ? Color{1.0f, 1.0f, 1.0f, 0.9f} : StampPlate(X);
		float Sz = Fit(C, S, Size * 0.7f, MaxW, false);
		Sz = Fit(C, S, Sz, MaxW - Sz * 0.9f, false);
		const float W = Wide(C, S, Sz, false) + Sz * 0.9f;
		StampBox(X, S, LeftOf(W) + W * 0.5f, Base - Sz * 0.36f, Sz, Col, &Plate);
		return Sz;
	}
	case CapStyle::Marker:
	{
		float Sz = Fit(C, S, Size * 0.86f, MaxW, false);
		Sz = Fit(C, S, Sz, MaxW - Sz * 0.5f, false);
		const float W = Wide(C, S, Sz, false) + Sz * 0.5f;
		const float L = LeftOf(W);
		C.FillPolygon({{L - Sz * 0.12f, Base - Sz * 0.84f}, {L + W, Base - Sz * 0.94f}, {L + W + Sz * 0.08f, Base + Sz * 0.2f}, {L - Sz * 0.06f, Base + Sz * 0.24f}},
			(X.V & 0x20u) != 0 ? X.P.Acc : Hex(Yellow));
		Plain(X, S, L + Sz * 0.12f, Base, Sz, Hex(0x14101c), Align::Left);
		return Sz;
	}
	case CapStyle::TwoTone:
	{
		const float Sz = Fit(C, S, Size, MaxW, Italic);
		Loud(X, S, Ax, Base, Sz, Hex(0xffffff), Mix(X.P.Acc, Hex(0x000000), 0.45f), A, Italic);
		return Sz;
	}
	default:
	{
		const float Sz = Fit(C, S, Size, MaxW, Italic);
		Loud(X, S, Ax, Base, Sz, Fill, Edge, A, Italic);
		return Sz;
	}
	}
}

/** The corner mark: the channel's initials, its mark, a colour bar, or none (top right, top left when mirrored). */
void CornerMark(Ctx& X)
{
	Canvas& C = *X.C;
	const int K = X.S->Corner;
	if (K == static_cast<int>(Corner::ColourBar))
	{
		C.FillRect({0.0f, 0.0f, DesignW, 8.0f}, X.P.Acc);
		return;
	}
	if (K != static_cast<int>(Corner::Initials) && K != static_cast<int>(Corner::Logo))
	{
		return;
	}
	// The initials: the first letter or digit of each part of the name (a handle's parts split at '_', '.', ' ').
	std::string Ini;
	const std::string Name = X.Look->LastName.empty() ? X.Look->FirstName : X.Look->FirstName + " " + X.Look->LastName;
	bool Start = true;
	for (const char Ch : Name)
	{
		const unsigned char U = static_cast<unsigned char>(Ch);
		if (Ch == '_' || Ch == '.' || Ch == ' ' || Ch == '-')
		{
			Start = true;
			continue;
		}
		if (Start && std::isalnum(U) != 0 && Ini.size() < 2)
		{
			Ini.push_back(static_cast<char>(std::toupper(U)));
		}
		Start = false;
	}
	if (Ini.empty())
	{
		Ini = "R";
	}
	// The top corner the composition keeps free: it moves with the mirror (its letters never turn). Review C3: fixed at the
	// right, a mirrored layout's words and cards ran under it.
	const float Bx = Mleft(X, 578.0f, 48.0f);
	if (K == static_cast<int>(Corner::Initials))
	{
		C.FillRoundRect({Bx, 14.0f, 48.0f, 34.0f}, 6.0f, Hex(videoart::Marigold));
		C.StrokeRoundRect({Bx, 14.0f, 48.0f, 34.0f}, 6.0f, Hex(videoart::Ink), 3.0f);
		Plain(X, Ini, Bx + 24.0f, 38.0f, 18.0f, Hex(videoart::Ink), Align::Center);
	}
	else
	{
		C.FillCircle(Bx + 26.0f, 34.0f, 22.0f, Hex(videoart::Ink));
		C.StrokeEllipse(Bx + 26.0f, 34.0f, 22.0f, 22.0f, X.P.Acc, 3.0f);
		DrawSuit(C, 3, Bx + 16.0f, 22.0f, 20.0f, X.P.Acc);
		Plain(X, Ini.substr(0, 1), Bx + 26.0f, 52.0f, 10.0f, Hex(0xffffff), Align::Center);
	}
}

/** The site's strip along the bottom (a rival hand's, never RiverLine naming a rival: the strip is the hand's own site). */
void SiteStrip(Ctx& X)
{
	if (X.Site > 0 && X.Site < static_cast<int>(net::SiteId::Count))
	{
		siteart::Strip(*X.C, {0.0f, 336.0f, DesignW, 24.0f}, static_cast<net::SiteId>(X.Site));
	}
}

// ------------------------------------------------------------------ the 35 archetypes (POV is P1)

void GutPunch(Ctx& X)
{
	Background(X, 190.0f, 200.0f);
	Subject(X, 180.0f, 165.0f, 0.6f);
	Captions(X, {350.0f, 150.0f, 270.0f, 112.0f, Align::Left});
	{
		// Yours, VS, theirs (backs unless turned up): read left to right on either side.
		const Upright Keep(X, 372.0f, 622.0f);
		UpRow(X, X.Mine, 2, 372.0f, 232.0f, 50.0f, 6.0f);
		Loud(X, "VS", Mx(X, 491.0f), 290.0f, 22.0f, Hex(0xffffff), Hex(0x000000), Align::Center, false);
		UpRow(X, X.Theirs, 2, 516.0f, 232.0f, 50.0f, 6.0f);
	}
	Props(X, 300.0f, 300.0f, 70.0f, 590.0f, 110.0f, 60.0f);
	// Aimed at their cards inside the upright row (Kept: the mirror moves the row, not the order in it).
	StickerAt(X, 560.0f, 70.0f, Kept(X, 372.0f, 622.0f, 568.0f), 266.0f);
}

void CircledRiver(Ctx& X)
{
	Background(X, 380.0f, 170.0f);
	const size_t N = std::max<size_t>(1, std::min<size_t>(5, X.Board.size()));
	const bool Has = !X.Board.empty();
	// The board huge, the marked card (the k fact: the card that turned it; else the last, the river) lifted and circled.
	const size_t Mark = X.Marked >= 0 && static_cast<size_t>(X.Marked) < N ? static_cast<size_t>(X.Marked) : N - 1;
	float Lx = 40.0f;
	{
		// The board in its order (flop, turn, river) on either side of the mirror.
		const Upright Keep(X, 40.0f, 600.0f);
		for (size_t I = 0; I < N; ++I)
		{
			const float L = 40.0f + static_cast<float>(I) * 104.0f + (I > Mark ? 20.0f : 0.0f);
			if (I == Mark)
			{
				Lx = L + 8.0f;
				continue;
			}
			Up(X, Has ? X.Board[I] : -1, L, 96.0f, 92.0f, 129.0f);
		}
		Up(X, Has ? X.Board[Mark] : -1, Lx, 64.0f, 112.0f, 157.0f, true);
		RedCircle(*X.C, Lx + 56.0f, 142.0f, 90.0f, 106.0f, 9.0f);
		const float Ax = Lx < 300.0f ? Lx + 240.0f : Lx - 120.0f;
		Arrow(*X.C, Ax, 330.0f, Lx + (Lx < 300.0f ? 120.0f : -20.0f), 214.0f, -40.0f, Hex(HotRed), 9.0f);
	}
	if (HasFace(X))
	{
		Subject(X, 92.0f, 280.0f, 0.24f);
	}
	Captions(X, {160.0f, 334.0f, 300.0f, 42.0f, Align::Left});
	Props(X, 580.0f, 300.0f, 60.0f, 60.0f, 40.0f, 50.0f);
	// The board's block is centred, so the circle sits where it was drawn on either side (StickerAt mirrors its focus).
	StickerAt(X, 600.0f, 300.0f, X.Mirror ? DesignW - (Lx + 56.0f) : Lx + 56.0f, 142.0f);
}

void TheRead(Ctx& X)
{
	Background(X, 300.0f, 150.0f);
	{
		const Upright Keep(X, 40.0f, 340.0f);
		Up(X, X.Mine.size() > 0 ? X.Mine[0] : -1, 40.0f, 80.0f, 84.0f, 118.0f);
		Up(X, X.Mine.size() > 1 ? X.Mine[1] : -1, 110.0f, 92.0f, 84.0f, 118.0f);
		// Their cards stay backs unless they were turned up.
		Up(X, X.Theirs.size() > 0 ? X.Theirs[0] : -1, 222.0f, 92.0f, 70.0f, 98.0f);
		Up(X, X.Theirs.size() > 1 ? X.Theirs[1] : -1, 270.0f, 100.0f, 70.0f, 98.0f);
		if (X.Theirs.empty())
		{
			Loud(X, "??", 290.0f, 156.0f, 64.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
		}
	}
	Subject(X, 482.0f, 150.0f, 0.56f);
	Captions(X, {36.0f, 322.0f, 380.0f, 70.0f, Align::Left});
	Props(X, 160.0f, 250.0f, 60.0f, 330.0f, 250.0f, 50.0f);
	StickerAt(X, 340.0f, 60.0f, Kept(X, 40.0f, 340.0f, 300.0f), 140.0f);
}

void NervesOfSteel(Ctx& X)
{
	Background(X, 300.0f, 160.0f);
	std::vector<Card> Hand = X.Mine;
	if (Hand.size() < 2)
	{
		Hand = {-1, -1};
	}
	Fan(X, Hand, 500.0f, 150.0f, 104.0f, 0.2f, true);
	Subject(X, 250.0f, 160.0f, 0.58f);
	Captions(X, {300.0f, 324.0f, 320.0f, 76.0f, Align::Left});
	Props(X, 80.0f, 290.0f, 70.0f, 590.0f, 280.0f, 50.0f);
	StickerAt(X, 250.0f, 40.0f, 500.0f, 150.0f);
}

void Caught(Ctx& X)
{
	Background(X, 420.0f, 150.0f);
	std::vector<Card> Hand = X.Mine;
	if (Hand.size() < 2)
	{
		Hand = {-1, -1};
	}
	{
		const Upright Keep(X, 340.0f, 560.0f);
		Up(X, Hand[0], 340.0f, 60.0f, 96.0f, 134.0f);
		Up(X, Hand[1], 410.0f, 74.0f, 96.0f, 134.0f);
		const Color Plate = Hex(0x111111, 0.85f);
		StampBox(X, "CALLED", 450.0f, 196.0f, 40.0f, Hex(HotRed), &Plate);
	}
	Subject(X, 170.0f, 165.0f, 0.58f);
	Captions(X, {320.0f, 300.0f, 300.0f, 56.0f, Align::Left});
	Props(X, 580.0f, 60.0f, 50.0f, 590.0f, 300.0f, 50.0f);
	StickerAt(X, 590.0f, 120.0f, Kept(X, 340.0f, 560.0f, 410.0f), 130.0f);
}

void TheFold(Ctx& X)
{
	Background(X, 300.0f, 140.0f);
	Subject(X, 150.0f, 150.0f, 0.56f);
	{
		const Upright Keep(X, 318.0f, 468.0f);
		Up(X, X.Mine.size() > 0 ? X.Mine[0] : -1, 318.0f, 54.0f, 96.0f, 134.0f);
		Up(X, X.Mine.size() > 1 ? X.Mine[1] : -1, 372.0f, 66.0f, 96.0f, 134.0f);
		RedX(*X.C, 418.0f, 128.0f, 52.0f, 14.0f);
	}
	OptionCards(X, 498.0f, 70.0f, 124.0f, 120.0f, true);
	Captions(X, {318.0f, 268.0f, 300.0f, 74.0f, Align::Left});
	Props(X, 560.0f, 300.0f, 60.0f, 60.0f, 300.0f, 50.0f);
	// The sticker beside the thinking face (the top corner is the corner mark's).
	StickerAt(X, 262.0f, 40.0f, Kept(X, 318.0f, 468.0f, 418.0f), 128.0f);
}

void ThePile(Ctx& X)
{
	Background(X, 300.0f, 170.0f);
	Felt(X, 290.0f, 196.0f, 246.0f, 124.0f);
	ChipPile(X, 290.0f, 190.0f, 70.0f, Many(X, 16, 12, 8));
	Arrow(*X.C, Mx(X, 120.0f), 336.0f, Mx(X, 200.0f), 268.0f, 10.0f, Hex(Yellow), 8.0f);
	if (HasFace(X))
	{
		Subject(X, 550.0f, 214.0f, 0.36f);
	}
	Captions(X, {420.0f, 104.0f, 200.0f, 54.0f, Align::Center});
	Props(X, 90.0f, 80.0f, 60.0f, 560.0f, 320.0f, 50.0f);
	StickerAt(X, 90.0f, 90.0f, 290.0f, 190.0f);
}

void BigNumber(Ctx& X)
{
	Background(X, 300.0f, 160.0f);
	Canvas& C = *X.C;
	// The number: the n fact when the recipe has one (the captions then go under it), else the caption itself.
	const std::string& Big = X.Big.empty() ? X.Cap1 : X.Big;
	const std::string& Under = X.Big.empty() ? X.Cap2 : X.Cap1;
	BigStyled(X, Big, Mx(X, 300.0f), 196.0f, 150.0f, 520.0f, Align::Center, (X.V & 0x10u) != 0, X.P.Acc, Hex(0x000000));
	const float S2 = Fit(C, Under, 46.0f, 460.0f, false);
	Loud(X, Under, Mx(X, 300.0f), 196.0f + S2 * 1.5f, S2, Hex(0xffffff), Hex(0x000000), Align::Center, false);
	if (HasFace(X))
	{
		Subject(X, 556.0f, 236.0f, 0.34f);
	}
	Props(X, 80.0f, 290.0f, 80.0f, 80.0f, 70.0f, 50.0f);
	StickerAt(X, 560.0f, 60.0f, 300.0f, 150.0f);
}

void BeforeAfter(Ctx& X)
{
	Background(X, 320.0f, 140.0f);
	Canvas& C = *X.C;
	std::string Before, After, Lower;
	if (!X.From.empty() || !X.To.empty())
	{
		// The f and t facts are the panels; the caption is the lower third.
		Before = X.From;
		After = X.To;
		Lower = X.Cap2.empty() ? X.Cap1 : X.Cap1 + " \xC2\xB7 " + X.Cap2;
	}
	else if (SplitArrow(X.Cap1, Before, After))
	{
		Lower = X.Cap2;
	}
	else if (SplitArrow(X.Cap2, Before, After))
	{
		Lower = X.Cap1;
	}
	else
	{
		Before = X.Cap1;
		After = X.Cap2;
	}
	{
		// Before, then after: left to right on either side of the mirror.
		const Upright Keep(X, 30.0f, 618.0f);
		C.FillRoundRect({30.0f, 60.0f, 200.0f, 150.0f}, 14.0f, Color{0.0f, 0.0f, 0.0f, 0.28f});
		C.FillRoundRect({358.0f, 46.0f, 260.0f, 176.0f}, 14.0f, Color{0.0f, 0.0f, 0.0f, 0.28f});
		const float Bs = Fit(C, Before, 74.0f, 176.0f, false), As = Fit(C, After, 76.0f, 236.0f, false);
		Loud(X, Before, 130.0f, 135.0f + Bs * 0.36f, Bs, Hex(0xffffff), Mix(X.P.B, Hex(0x000000), 0.4f), Align::Center, false);
		// With no lower third the caption is the panels, and After takes its style (C3 fix M1): stamped, or white outlined.
		const CapStyle St = static_cast<CapStyle>(X.S->CapStyle);
		if (Lower.empty() && St == CapStyle::Stamp)
		{
			const Color Plate = StampPlate(X);
			StampBox(X, After, 488.0f, 134.0f, Fit(C, After, As * 0.8f, 200.0f, false), Hex(HotRed), &Plate);
		}
		else
		{
			Loud(X, After, 488.0f, 134.0f + As * 0.36f, As, Lower.empty() && St == CapStyle::Outlined ? Hex(0xffffff) : X.P.Acc, Mix(X.P.B, Hex(0x000000), 0.4f), Align::Center, false);
		}
		C.FillPolygon({{244.0f, 120.0f}, {300.0f, 120.0f}, {300.0f, 152.0f}, {244.0f, 152.0f}}, Hex(0xffffff));
		C.FillPolygon({{300.0f, 96.0f}, {346.0f, 136.0f}, {300.0f, 176.0f}}, Hex(0xffffff));
	}
	if (HasFace(X))
	{
		Subject(X, 548.0f, 252.0f, 0.32f);
	}
	if (!Lower.empty())
	{
		// The words under the panels in the recipe's style (C3 fix M1: always a lower third, whatever Studio's style picker
		// chose); none is the lower third. A stamp takes its words at a size near the lower third's (review-c3f-thumbs: at
		// 0.62 of the box, they were 21 units against the band's 36).
		video::ThumbSpec S = *X.S;
		S.CapStyle = static_cast<int16_t>(S.CapStyle == static_cast<int16_t>(CapStyle::None) ? CapStyle::LowerThird : static_cast<CapStyle>(S.CapStyle));
		const video::ThumbSpec* Old = X.S;
		X.S = &S;
		Captions(X, {28.0f, 330.0f, 560.0f, S.CapStyle == static_cast<int16_t>(CapStyle::Stamp) ? 54.0f : 34.0f, Align::Left}, Lower, std::string());
		X.S = Old;
	}
	Props(X, 120.0f, 270.0f, 60.0f, 300.0f, 270.0f, 50.0f);
	StickerAt(X, 300.0f, 60.0f, Kept(X, 30.0f, 618.0f, 488.0f), 134.0f); // at the After panel on either side
}

void TrophyShot(Ctx& X)
{
	Background(X, 200.0f, 150.0f);
	Canvas& C = *X.C;
	const int Pa = X.S->PropA;
	const float Tx = Mx(X, 196.0f);
	if (Pa == static_cast<int>(Prop::Bracelet) || Pa == static_cast<int>(Prop::Ring))
	{
		PropAt(X, Pa, 196.0f, 150.0f, 170.0f);
	}
	else
	{
		Glow(C, Tx, 150.0f, 150.0f, Hex(0xf5c542), 0.35f);
		Trophy(C, Tx, 150.0f, 150.0f, Hex(0xf5c542));
	}
	if (!X.Event.empty())
	{
		// The event's name on the base (the e fact).
		Loud(X, X.Event, Tx, 268.0f, Fit(C, X.Event, 20.0f, 300.0f, true), Hex(0xf5c542), Hex(0x000000), Align::Center, true);
	}
	Subject(X, 480.0f, 160.0f, 0.56f);
	Captions(X, {30.0f, 334.0f, 360.0f, 62.0f, Align::Left});
	if (X.S->PropB >= 0 && X.S->PropB < static_cast<int>(Prop::Count))
	{
		PropAt(X, X.S->PropB, 330.0f, 70.0f, 60.0f);
	}
	StickerAt(X, 330.0f, 60.0f, 196.0f, 120.0f);
}

void VsSplit(Ctx& X)
{
	Background(X, 320.0f, 170.0f);
	Canvas& C = *X.C;
	Person(X, *X.Look, 150.0f, 172.0f, Zoom(X, 0.5f), X.S->Expr, X.S->Subj == static_cast<int>(thumbart::Subject::You) ? X.S->Pose : 0);
	if (X.S->Subj == static_cast<int>(thumbart::Subject::Ghost))
	{
		Ghost(X, 500.0f, 190.0f, 92.0f, 0.95f);
	}
	else
	{
		Opponent(X, 500.0f, 172.0f, Zoom(X, 0.5f), 8, true);
	}
	Pts Bolt = {{318.0f, 70.0f}, {298.0f, 168.0f}, {326.0f, 168.0f}, {304.0f, 270.0f}, {360.0f, 150.0f}, {330.0f, 150.0f}, {352.0f, 70.0f}};
	for (Vec2& P : Bolt)
	{
		P.X = Mx(X, P.X);
	}
	C.FillPolygon({Bolt[0], Bolt[1], Bolt[2], Bolt[5], Bolt[6]}, Hex(Yellow));
	C.FillPolygon({Bolt[2], Bolt[3], Bolt[4], Bolt[5]}, Hex(Yellow));
	C.StrokePolyline(Bolt, true, Hex(0x000000), 4.0f);
	Loud(X, "VS", Mx(X, 328.0f), 200.0f, 48.0f, Hex(0xffffff), Hex(0x000000), Align::Center, true);
	// The two hands over the faces, in from the top corners (the corner mark's).
	UpRow(X, X.Mine, 2, 76.0f, 24.0f, 40.0f, 4.0f);
	if (!X.Theirs.empty())
	{
		UpRow(X, X.Theirs, 2, 480.0f, 24.0f, 40.0f, 4.0f);
	}
	Captions(X, {120.0f, 340.0f, 400.0f, 58.0f, Align::Center});
	Props(X, 60.0f, 300.0f, 50.0f, 580.0f, 300.0f, 50.0f);
	StickerAt(X, 320.0f, 40.0f, 328.0f, 180.0f);
}

void TheBubble(Ctx& X)
{
	Background(X, 200.0f, 180.0f);
	Canvas& C = *X.C;
	// The teetering tower, a short stack's last chips (nine on the hero: C3 fix M1, with the heaviest of the 12 faces, Tilted,
	// twelve chips took the thumbnail to 14.3k vertices against §10's 14k).
	ChipTower(C, Mx(X, 168.0f), 300.0f, 34.0f, Many(X, 9, 9, 7), X.Mirror ? -3.0f : 3.0f, Hex(0xff6b57), X.Tier == 0);
	Subject(X, 470.0f, 158.0f, 0.56f);
	// (The words sit low enough to clear Studio's Duel letter on an option's top-left corner.)
	Captions(X, {28.0f, 112.0f, 300.0f, 84.0f, Align::Left});
	Props(X, 300.0f, 290.0f, 60.0f, 590.0f, 300.0f, 50.0f);
	StickerAt(X, 300.0f, 200.0f, 168.0f, 160.0f);
}

/**
 * The hunter: the player, wearing the recipe's face and pose (C3 fix M1: Headhunter's subject is the opponent, the busted
 * one, so the dealt face, Smug or Grin, was the player's and never drawn; Studio's FACE picker changed nothing).
 */
void Hunter(Ctx& X, float Cx, float Ey, float Sc)
{
	Person(X, *X.Look, Cx, Ey, Zoom(X, Sc), X.S->Expr, X.S->Pose);
}

void Headhunter(Ctx& X)
{
	// The split (its first layout): you on the left, the busted player on the right under the crosshair (their procedural
	// face, or nobody's), their half in shadow, the words along the bottom.
	Background(X, 450.0f, 160.0f);
	Canvas& C = *X.C;
	C.FillPolygon({{Mx(X, 316.0f), 0.0f}, {Mx(X, 640.0f), 0.0f}, {Mx(X, 640.0f), 360.0f}, {Mx(X, 270.0f), 360.0f}}, WithA(Mix(X.P.B, Hex(0x000000), 0.3f), 0.6f));
	// (Both busts run off the bottom at either zoom.)
	Opponent(X, 470.0f, 200.0f, Zoom(X, 0.46f), 5, true);
	Crosshair(C, Mx(X, 470.0f), 204.0f, 100.0f);
	Hunter(X, 140.0f, 200.0f, 0.46f);
	// The bounty: the n fact in a burst at the top when the recipe has it.
	if (!X.Big.empty())
	{
		BurstPlate(X, X.Big, Mx(X, 300.0f), 66.0f, 40.0f, X.P.Acc, Mix(X.P.B, Hex(0x000000), 0.35f), 46.0f);
	}
	Captions(X, {100.0f, 336.0f, 440.0f, 60.0f, Align::Center});
	Props(X, 300.0f, 200.0f, 50.0f, 590.0f, 310.0f, 46.0f);
	StickerAt(X, 590.0f, 260.0f, 470.0f, 204.0f);
}

/** Whether a caption line is the multiple itself ("25X", "x25", "×25"): the m fact already shows it big. */
bool IsMultiple(const std::string& Cap, const std::string& M)
{
	std::string S;
	for (size_t I = 0; I < Cap.size(); ++I)
	{
		if (static_cast<unsigned char>(Cap[I]) == 0xC3 && I + 1 < Cap.size() && static_cast<unsigned char>(Cap[I + 1]) == 0x97)
		{
			++I;
			continue;
		}
		if (Cap[I] != ' ' && Cap[I] != 'x' && Cap[I] != 'X')
		{
			S.push_back(Cap[I]);
		}
	}
	return !M.empty() && S == M;
}

/**
 * The words beside the multiple (Envelope, Site Card): with no m fact, line 2 (line 1 is the big one); with it, the caption
 * without the multiple twice: a line 1 that is the multiple gives way to line 2, else both lines (review C3: "25×" and
 * "25X" side by side, line 2 dropped).
 */
std::string Rest(const Ctx& X)
{
	if (X.Mult.empty() || IsMultiple(X.Cap1, X.Mult))
	{
		return X.Cap2;
	}
	return X.Cap2.empty() ? X.Cap1 : X.Cap1 + " " + X.Cap2;
}

void EnvelopeArch(Ctx& X)
{
	Background(X, 210.0f, 180.0f);
	Canvas& C = *X.C;
	const Rect R{Mleft(X, 60.0f, 290.0f), 90.0f, 290.0f, 186.0f};
	Envelope(C, R, true);
	// The multiple in gold on the envelope (the m fact, else the caption), the caption's other line in a burst.
	const std::string OnIt = X.Mult.empty() ? X.Cap1 : X.Mult + "\xC3\x97";
	const std::string Burst = Rest(X);
	// The caption's style (C3 fix M1): a number burst carries the other words, or the multiple itself when there are none;
	// outlined, the words are written and the multiple stays gold on the envelope.
	const bool Burster = X.S->CapStyle == static_cast<int16_t>(CapStyle::NumberBurst);
	if (Burster && Burst.empty())
	{
		BigStyled(X, OnIt, R.X + R.W * 0.5f, R.Y + R.H * 0.78f, 90.0f, 250.0f, Align::Center, false, Hex(0xf5c542), Hex(0x3a2a10));
	}
	else
	{
		const float Sz = Fit(C, OnIt, 78.0f, 250.0f, false);
		Loud(X, OnIt, R.X + R.W * 0.5f, R.Y + R.H * 0.82f, Sz, Hex(0xf5c542), Hex(0x3a2a10), Align::Center, false);
	}
	Subject(X, 512.0f, 164.0f, 0.55f);
	if (!Burst.empty())
	{
		if (Burster)
		{
			BurstPlate(X, Burst, Mx(X, 380.0f), 292.0f, 34.0f, X.P.Acc, Hex(0x050505), 50.0f);
		}
		else
		{
			Loud(X, Burst, Mx(X, 380.0f), 306.0f, Fit(C, Burst, 36.0f, 300.0f, false), Hex(0xffffff), Hex(0x000000), Align::Center, false);
		}
	}
	Props(X, 120.0f, 50.0f, 50.0f, 590.0f, 300.0f, 50.0f);
	StickerAt(X, 380.0f, 60.0f, 210.0f, 180.0f);
}

void TheGraph(Ctx& X)
{
	Background(X, 220.0f, 150.0f);
	const Vec2 End = GraphAt(X, 30.0f, 40.0f, 380.0f, 220.0f);
	Subject(X, 530.0f, 170.0f, 0.5f);
	Captions(X, {80.0f, 316.0f, 240.0f, 48.0f, Align::Center});
	Props(X, 590.0f, 50.0f, 50.0f, 460.0f, 300.0f, 50.0f);
	StickerAt(X, 440.0f, 60.0f, X.Mirror ? DesignW - End.X : End.X, End.Y);
}

void DayCounter(Ctx& X)
{
	Background(X, 220.0f, 140.0f);
	Canvas& C = *X.C;
	// The person first: a raised arm reaches back over the count and the bar, and the count reads over it.
	Subject(X, 524.0f, 170.0f, 0.5f);
	BigStyled(X, X.Cap1, Mx(X, 30.0f), 150.0f, 120.0f, 420.0f, X.Mirror ? Align::Right : Align::Left, true, Hex(0xffffff), Mix(X.P.B, Hex(0x000000), 0.4f));
	// The bar: the p fact's progress, else the challenge's own amounts in the caption (start, now and goal; or start and
	// now, filled). Neither: no bar.
	const std::vector<double> A = Amounts(X.Cap2);
	if (A.size() >= 2 || X.Progress >= 0)
	{
		// The bar fills left to right on either side of the mirror.
		const Upright Keep(X, 34.0f, 414.0f);
		const float L = Mleft(X, 34.0f, 380.0f);
		const double Fill = X.Progress >= 0 ? X.Progress / 100.0 : A.size() >= 3 && A[2] > A[0] ? std::clamp((A[1] - A[0]) / (A[2] - A[0]), 0.0, 1.0) : 1.0;
		const int Was = X.Progress;
		X.Progress = static_cast<int>(std::lround(Fill * 100.0));
		ProgressAt(X, 34.0f, 196.0f, 380.0f, 34.0f);
		X.Progress = Was;
		const float Fw = static_cast<float>(372.0 * Fill);
		std::string Parts[3];
		int K = 0;
		for (size_t I = 0; I < X.Cap2.size() && K < 3; ++I)
		{
			if (X.Cap2[I] == '$')
			{
				size_t J = I + 1;
				while (J < X.Cap2.size() && (std::isdigit(static_cast<unsigned char>(X.Cap2[J])) != 0 || X.Cap2[J] == ',' || X.Cap2[J] == '.' || X.Cap2[J] == 'K' || X.Cap2[J] == 'M'))
				{
					++J;
				}
				Parts[K++] = X.Cap2.substr(I, J - I);
				I = J;
			}
		}
		if (K == 1)
		{
			// One amount beside the p fact: today's ({Now}), where the fill has got to, not at the start (review C3).
			const float Wn = Wide(C, Parts[0], 26.0f, false);
			Loud(X, Parts[0], std::clamp(L + 4.0f + Fw, L + Wn * 0.5f, L + 380.0f - Wn * 0.5f), 262.0f, 26.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
		}
		else
		{
			Loud(X, Parts[0], X.Mirror ? L + 380.0f : L, 262.0f, 26.0f, Hex(0xffffff), Hex(0x000000), X.Mirror ? Align::Right : Align::Left, false);
		}
		if (K >= 3)
		{
			Loud(X, Parts[1], (X.Mirror ? L + 376.0f - Fw : L + 4.0f + Fw), 262.0f, 26.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
			Loud(X, Parts[2], X.Mirror ? L : L + 380.0f, 262.0f, 26.0f, Hex(0xffffff), Hex(0x000000), X.Mirror ? Align::Left : Align::Right, false);
		}
		else
		{
			Loud(X, Parts[1], X.Mirror ? L : L + 380.0f, 262.0f, 26.0f, X.P.Acc, Hex(0x000000), X.Mirror ? Align::Left : Align::Right, false);
		}
	}
	else if (!X.Cap2.empty())
	{
		Loud(X, X.Cap2, Mx(X, 34.0f), 236.0f, Fit(C, X.Cap2, 40.0f, 380.0f, false), X.P.Acc, Hex(0x000000), X.Mirror ? Align::Right : Align::Left, false);
	}
	Props(X, 100.0f, 300.0f, 50.0f, 300.0f, 300.0f, 50.0f);
	SiteStrip(X);
	StickerAt(X, 440.0f, 60.0f, 220.0f, 120.0f);
}

void NightShift(Ctx& X)
{
	Background(X, 230.0f, 190.0f);
	Canvas& C = *X.C;
	// The laptop's glow on a dark room, four tables open.
	Glow(C, Mx(X, 230.0f), 200.0f, 220.0f, Hex(0x6fb8ff), 0.3f);
	Laptop(C, Mx(X, 230.0f), 210.0f, 130.0f, Hex(0x6fb8ff), 4);
	// The moon in the window.
	C.FillCircle(Mx(X, 74.0f), 64.0f, 30.0f, Hex(0xfff3c4));
	C.FillCircle(Mx(X, 86.0f), 56.0f, 26.0f, Mix(X.P.A, X.P.B, 0.2f));
	Subject(X, 504.0f, 176.0f, 0.52f);
	Captions(X, {150.0f, 74.0f, 300.0f, 64.0f, Align::Left});
	Props(X, 380.0f, 300.0f, 50.0f, 60.0f, 300.0f, 50.0f);
	StickerAt(X, 380.0f, 120.0f, 230.0f, 200.0f);
}

void Venue(Ctx& X)
{
	Background(X, 320.0f, 120.0f);
	Canvas& C = *X.C;
	// C3 fix (L7): the marquee in from the top left corner, its name starting 148 units in: Studio's 120-wide option draws the
	// Duel's letter there (a 10-unit circle 14 units in, 128 design units at that size), and over a marquee at 30 it read
	// "BERCREST".
	Marquee(X, Mx(X, 270.0f), 30.0f, 290.0f);
	if (!X.Event.empty())
	{
		// Tonight's event under the marquee (the e fact).
		Loud(X, X.Event, Mx(X, 270.0f), 160.0f, Fit(C, X.Event, 24.0f, 300.0f, true), Hex(0xffd479), Hex(0x000000), Align::Center, true);
	}
	// The crowd: silhouettes along the bottom.
	const int N = Many(X, 12, 9, 6);
	for (int I = 0; I < N; ++I)
	{
		const float Px = static_cast<float>(I) * 640.0f / static_cast<float>(N - 1);
		const float H = 30.0f + Hf(X, 1500u + static_cast<uint32_t>(I)) * 26.0f;
		C.FillCircle(Px, 340.0f - H, 22.0f, Hex(0x0b0b10, 0.9f));
		C.FillEllipse(Px, 372.0f - H * 0.4f, 46.0f, 40.0f, Hex(0x0b0b10, 0.9f));
	}
	Subject(X, 470.0f, 172.0f, 0.52f);
	Captions(X, {30.0f, 250.0f, 340.0f, 56.0f, Align::Left});
	Props(X, 590.0f, 60.0f, 50.0f, 120.0f, 200.0f, 50.0f);
	StickerAt(X, 590.0f, 260.0f, 270.0f, 76.0f);
}

void RoadTrip(Ctx& X)
{
	Background(X, 300.0f, 120.0f);
	Canvas& C = *X.C;
	// City lights on the horizon, the bus on the road.
	const int N = Many(X, 16, 10, 8);
	for (int I = 0; I < N; ++I)
	{
		const float Bx = static_cast<float>(I) * 640.0f / static_cast<float>(N);
		const float H = 40.0f + Hf(X, 1600u + static_cast<uint32_t>(I)) * 90.0f;
		C.FillRect({Bx, 230.0f - H, 640.0f / static_cast<float>(N) - 4.0f, H}, Mix(X.P.B, Hex(0x000000), 0.45f));
		for (int K = 0; K < Many(X, 3, 1, 1); ++K)
		{
			C.FillRect({Bx + 6.0f + static_cast<float>(K) * 10.0f, 230.0f - H + 10.0f + Hf(X, 1700u + static_cast<uint32_t>(I * 3 + K)) * (H - 20.0f), 5.0f, 5.0f}, Hex(0xffe08a, 0.85f));
		}
	}
	C.FillRect({0.0f, 230.0f, DesignW, 130.0f}, Mix(X.P.B, Hex(0x000000), 0.6f));
	Bus(C, Mleft(X, 30.0f, 300.0f), 196.0f, 300.0f, Hex(0x2b6cb0));
	Subject(X, 490.0f, 168.0f, 0.52f);
	// (The words sit low enough to clear Studio's Duel letter on an option's top-left corner.)
	Captions(X, {30.0f, 100.0f, 360.0f, 64.0f, Align::Left});
	Props(X, 380.0f, 310.0f, 50.0f, 600.0f, 60.0f, 40.0f);
	StickerAt(X, 380.0f, 160.0f, 180.0f, 230.0f);
}

void RentLetter(Ctx& X)
{
	Background(X, 200.0f, 170.0f);
	// Which notice: the prop the recipe names, else the caption's own word.
	const std::string Both = X.Cap1 + " " + X.Cap2;
	const int Pa = X.S->PropA;
	std::string Title = "RENT DUE";
	if (Pa == static_cast<int>(Prop::EvictionNotice) || Both.find("EVICT") != std::string::npos || Both.find("VACATE") != std::string::npos)
	{
		Title = "NOTICE TO VACATE";
	}
	else if (Pa == static_cast<int>(Prop::PowerBill) || Both.find("POWER") != std::string::npos || Both.find("LIGHTS") != std::string::npos)
	{
		Title = "POWER BILL";
	}
	std::string Amount;
	for (const std::string* Cap : {&X.Cap1, &X.Cap2})
	{
		if (Amount.empty() && Cap->find('$') != std::string::npos)
		{
			const size_t At = Cap->find('$');
			size_t J = At + 1;
			while (J < Cap->size() && (std::isdigit(static_cast<unsigned char>((*Cap)[J])) != 0 || (*Cap)[J] == ','))
			{
				++J;
			}
			Amount = Cap->substr(At, J - At);
		}
	}
	Paper(X, {70.0f, 36.0f, 250.0f, 296.0f}, Title, 7, Amount);
	Subject(X, 482.0f, 160.0f, 0.56f);
	Captions(X, {340.0f, 300.0f, 280.0f, 46.0f, Align::Left});
	if (X.S->PropB >= 0 && X.S->PropB < static_cast<int>(Prop::Count))
	{
		PropAt(X, X.S->PropB, 360.0f, 70.0f, 60.0f);
	}
	StickerAt(X, 360.0f, 80.0f, 195.0f, 200.0f);
}

void Classroom(Ctx& X)
{
	Background(X, 200.0f, 150.0f);
	Whiteboard(X, {26.0f, 36.0f, 340.0f, 240.0f});
	Subject(X, 506.0f, 168.0f, 0.52f);
	Captions(X, {40.0f, 336.0f, 300.0f, 52.0f, Align::Left});
	Props(X, 400.0f, 300.0f, 50.0f, 600.0f, 60.0f, 40.0f);
	StickerAt(X, 400.0f, 70.0f, 140.0f, 130.0f);
}

void Quiz(Ctx& X)
{
	Background(X, 300.0f, 170.0f);
	Subject(X, 140.0f, 172.0f, 0.52f);
	OptionCards(X, 318.0f, 56.0f, 298.0f, 90.0f);
	Loud(X, "?", Mx(X, 466.0f), 268.0f, 140.0f, Hex(Yellow), Mix(X.P.B, Hex(0x000000), 0.3f), Align::Center, false);
	Captions(X, {290.0f, 314.0f, 330.0f, 60.0f, Align::Left});
	Props(X, 600.0f, 250.0f, 50.0f, 300.0f, 200.0f, 40.0f);
	StickerAt(X, 588.0f, 200.0f, 466.0f, 220.0f); // in from the edge: mirrored, a "??" ran off it
}

void TierList(Ctx& X)
{
	Background(X, 300.0f, 180.0f);
	// C3 fix (L7): the list and its title stand 58 in, clear of the Duel letter Studio puts on an option's top-left corner (24
	// in, the title's first letter sat under it in the 120 option).
	{
		const Upright Keep(X, 58.0f, 464.0f);
		TierRows(X, 58.0f, 94.0f, 406.0f, 264.0f, 4);
	}
	if (HasFace(X))
	{
		Subject(X, 554.0f, 214.0f, 0.36f);
	}
	Captions(X, {58.0f, 56.0f, 406.0f, 46.0f, Align::Left});
	Props(X, 560.0f, 60.0f, 50.0f, 560.0f, 330.0f, 40.0f);
	StickerAt(X, 520.0f, 70.0f, Kept(X, 58.0f, 464.0f, 60.0f), 100.0f);
}

void TopFive(Ctx& X)
{
	Background(X, 300.0f, 180.0f);
	{
		const Upright Keep(X, 30.0f, 390.0f);
		NumberedCells(X, 30.0f, 30.0f, 360.0f, 310.0f);
	}
	Subject(X, 520.0f, 186.0f, 0.5f);
	Captions(X, {392.0f, 76.0f, 168.0f, 48.0f, Align::Center});
	Props(X, 590.0f, 300.0f, 40.0f, 420.0f, 320.0f, 40.0f);
	StickerAt(X, 590.0f, 300.0f, Kept(X, 30.0f, 390.0f, 200.0f), 55.0f);
}

void DeskSetup(Ctx& X)
{
	Background(X, 300.0f, 150.0f);
	Canvas& C = *X.C;
	// C3 fix (L7): the desk as the player has it, streamart::Desk (§6.1's own "the desk with your owned gear") with the gear
	// the d fact names, each piece where the Kast desk puts it, framed like a photo of the room (it never flips: its sign's
	// words can't). No d fact: the bare desk and the laptop everyone has, never gear the player doesn't own (it drew two
	// monitors and a key light for everyone).
	gear::Owned Owned;
	for (const std::string& Id : X.Gear)
	{
		if (gear::Find(Id) != nullptr)
		{
			Owned[Id] = 0.0;
		}
	}
	const Rect Ph{Mleft(X, 30.0f, 430.0f), 30.0f, 430.0f, 262.0f};
	C.FillRoundRect({Ph.X - 2.0f, Ph.Y + 2.0f, Ph.W + 14.0f, Ph.H + 14.0f}, 6.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect({Ph.X - 7.0f, Ph.Y - 7.0f, Ph.W + 14.0f, Ph.H + 14.0f}, 6.0f, Hex(0xf4f1e8));
	streamart::Desk(C, Ph, Owned, false, 0.0);
	Subject(X, 550.0f, 196.0f, 0.44f);
	Captions(X, {30.0f, 340.0f, 420.0f, 40.0f, Align::Left});
	Props(X, 420.0f, 180.0f, 50.0f, 600.0f, 60.0f, 40.0f);
	StickerAt(X, 600.0f, 70.0f, 250.0f, 150.0f);
}

void CollabDuo(Ctx& X)
{
	Background(X, 320.0f, 170.0f);
	Person(X, *X.Look, 180.0f, 172.0f, Zoom(X, 0.5f), X.S->Expr, X.S->Pose);
	// The collaborator: the facts' named creator (a procedural face), never an invented one.
	if (!X.Other.empty())
	{
		Person(X, hero::CharacterFor(X.Other), 460.0f, 172.0f, Zoom(X, 0.5f), 2, 0, true);
	}
	else
	{
		Nobody(X, 460.0f, 172.0f, Zoom(X, 0.5f));
	}
	Loud(X, "+", Mx(X, 320.0f), 200.0f, 110.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
	Captions(X, {120.0f, 340.0f, 400.0f, 54.0f, Align::Center});
	Props(X, 320.0f, 60.0f, 50.0f, 600.0f, 60.0f, 40.0f);
	StickerAt(X, 320.0f, 60.0f, 320.0f, 170.0f);
}

void ReactionBox(Ctx& X)
{
	Background(X, 380.0f, 160.0f);
	Canvas& C = *X.C;
	// The world's result: its player's procedural face (the facts name them) or the trophy.
	if (X.Other == RivalName)
	{
		Opponent(X, 380.0f, 156.0f, Zoom(X, 0.56f), 3); // gh0stfold has no face: his ghost (review C3)
	}
	else if (!X.Other.empty())
	{
		Person(X, hero::CharacterFor(X.Other), 380.0f, 156.0f, Zoom(X, 0.56f), 3, 4);
	}
	else
	{
		Trophy(C, Mx(X, 380.0f), 160.0f, 150.0f, Hex(0xf5c542));
	}
	// Your face in a corner box.
	const float L = Mleft(X, 20.0f, 160.0f);
	C.FillRoundRect({L - 3.0f, 197.0f, 166.0f, 146.0f}, 9.0f, X.P.Acc);
	C.FillRoundRect({L, 200.0f, 160.0f, 140.0f}, 7.0f, Hex(0x2a2416));
	C.PushClip({L, 200.0f, 160.0f, 140.0f});
	if (HasFace(X))
	{
		Person(X, *X.Look, 100.0f, 256.0f, 0.28f, X.S->Expr, 0, true);
	}
	C.PopClip();
	Captions(X, {440.0f, 110.0f, 190.0f, 50.0f, Align::Center});
	if (!X.Event.empty())
	{
		// The result's event (the e fact) along the bottom.
		Loud(X, X.Event, Mx(X, 400.0f), 344.0f, Fit(C, X.Event, 26.0f, 380.0f, true), X.P.Acc, Hex(0x000000), Align::Center, true);
	}
	Props(X, 590.0f, 300.0f, 50.0f, 220.0f, 60.0f, 40.0f);
	StickerAt(X, 240.0f, 300.0f, 380.0f, 160.0f);
}

void StreamClip(Ctx& X)
{
	Background(X, 220.0f, 150.0f);
	Canvas& C = *X.C;
	// The lobby frame with the table and the board.
	const float L = Mleft(X, 24.0f, 400.0f);
	C.FillRoundRect({L, 30.0f, 400.0f, 250.0f}, 8.0f, Hex(0x0b1d2a));
	C.StrokeRoundRect({L, 30.0f, 400.0f, 250.0f}, 8.0f, X.P.Acc, 3.0f);
	C.FillRect({L, 30.0f, 400.0f, 24.0f}, Mix(X.P.Acc, Hex(0x000000), 0.45f));
	C.FillEllipse(L + 200.0f, 170.0f, 150.0f, 80.0f, Hex(0x3b2412));
	C.FillEllipse(L + 200.0f, 170.0f, 140.0f, 72.0f, Hex(0x0f5a3c));
	for (size_t I = 0; I < 5; ++I)
	{
		DrawCard(C, I < X.Board.size() ? X.Board[I] : -1, L + 130.0f + static_cast<float>(I) * 29.0f, 148.0f, 25.0f, 35.0f);
	}
	// The facecam inset, and the moment's chat: its real lines (the h facts), else the chat's shapes with no words.
	const float Fl = Mleft(X, 436.0f, 180.0f);
	C.FillRoundRect({Fl, 30.0f, 180.0f, 170.0f}, 8.0f, Hex(0x2a1240));
	C.PushClip({Fl, 30.0f, 180.0f, 170.0f});
	if (HasFace(X))
	{
		Person(X, *X.Look, X.Mirror ? 640.0f - (Fl + 90.0f) : Fl + 90.0f, 104.0f, 0.3f, X.S->Expr, X.S->Pose);
	}
	C.PopClip();
	C.StrokeRoundRect({Fl, 30.0f, 180.0f, 170.0f}, 8.0f, X.P.A, 4.0f);
	C.FillRoundRect({Fl, 208.0f, 180.0f, 72.0f}, 6.0f, Color{0.0f, 0.0f, 0.0f, 0.65f});
	if (!X.Chat.empty())
	{
		for (size_t K = 0; K < X.Chat.size(); ++K)
		{
			TextStyle T = Heavy(14.0f, K == 1 ? Hex(Yellow) : X.P.Acc, Align::Left, false);
			T.Weight = 800;
			T.MaxWidth = 164.0f;
			C.Text(X.Chat[K], Fl + 8.0f, 232.0f + static_cast<float>(K) * 24.0f, T);
		}
	}
	else
	{
		for (int K = 0; K < 3; ++K)
		{
			const float Y = 220.0f + static_cast<float>(K) * 18.0f;
			C.FillRoundRect({Fl + 10.0f, Y, 40.0f + Hf(X, 1800u + static_cast<uint32_t>(K)) * 20.0f, 9.0f}, 4.0f, K == 1 ? Hex(Yellow) : X.P.Acc);
			C.FillRoundRect({Fl + 70.0f, Y, 60.0f + Hf(X, 1900u + static_cast<uint32_t>(K)) * 40.0f, 9.0f}, 4.0f, Color{1.0f, 1.0f, 1.0f, 0.55f});
		}
	}
	Captions(X, {120.0f, 340.0f, 400.0f, 52.0f, Align::Center});
	StickerAt(X, 600.0f, 300.0f, 224.0f, 160.0f);
}

void Milestone(Ctx& X)
{
	Background(X, 320.0f, 160.0f);
	Canvas& C = *X.C;
	// Confetti (never a platform's plaque); the number (the n fact, else the caption) huge.
	Confetti(X, 0.0f, 0.0f, DesignW, DesignH, Many(X, 60, 30, 14));
	const std::string& Big = X.Big.empty() ? X.Cap1 : X.Big;
	const std::string& Under = X.Big.empty() ? X.Cap2 : X.Cap1;
	BigStyled(X, Big, Mx(X, 300.0f), 188.0f, 140.0f, 500.0f, Align::Center, (X.V & 0x10u) != 0, X.P.Acc, Hex(0x000000));
	if (!Under.empty())
	{
		const float S2 = Fit(C, Under, 44.0f, 460.0f, false);
		Loud(X, Under, Mx(X, 300.0f), 188.0f + S2 * 1.25f, S2, Hex(0xffffff), Hex(0x000000), Align::Center, false);
	}
	if (HasFace(X))
	{
		Subject(X, 560.0f, 250.0f, 0.32f);
	}
	Props(X, 80.0f, 290.0f, 60.0f, 80.0f, 70.0f, 50.0f);
	StickerAt(X, 560.0f, 70.0f, 300.0f, 150.0f);
}

void TextSlab(Ctx& X)
{
	Background(X, 520.0f, 160.0f);
	Canvas& C = *X.C;
	C.FillRect({Mleft(X, 0.0f, 410.0f), 0.0f, 410.0f, 300.0f}, X.P.Acc);
	// Two or three lines of huge words: the captions, split at their spaces when there's room.
	std::vector<std::string> Lines;
	for (const std::string* Cap : {&X.Cap1, &X.Cap2})
	{
		std::string Cur;
		for (const char Ch : *Cap)
		{
			if (Ch == ' ' && Lines.size() < 2 && !Cur.empty() && Cap == &X.Cap1 && X.Cap2.empty())
			{
				Lines.push_back(Cur);
				Cur.clear();
			}
			else
			{
				Cur.push_back(Ch);
			}
		}
		if (!Cur.empty())
		{
			Lines.push_back(Cur);
		}
	}
	if (Lines.size() > 3)
	{
		Lines.resize(3);
	}
	const float Lh = Lines.size() >= 3 ? 92.0f : 120.0f;
	// The slab's words in the caption's style (C3 fix M1: every style drew them the same): a lower third (and none) plain on
	// the block; a marker, each line on a white bar; boxed, each on a dark plate in white; the rest outlined in white.
	const CapStyle St = static_cast<CapStyle>(X.S->CapStyle);
	for (size_t I = 0; I < Lines.size(); ++I)
	{
		const float Base = 30.0f + Lh * static_cast<float>(I + 1) - Lh * 0.12f;
		const Align A = X.Mirror ? Align::Right : Align::Left;
		if (St == CapStyle::Marker || St == CapStyle::Boxed)
		{
			const float Sz = Fit(C, Lines[I], Lh * 0.78f, 330.0f, false);
			const float W = Wide(C, Lines[I], Sz, false);
			const float L = X.Mirror ? Mx(X, 30.0f) - W : Mx(X, 30.0f);
			const float P = Sz * 0.14f;
			if (St == CapStyle::Marker)
			{
				C.FillPolygon({{L - P, Base - Sz * 0.8f}, {L + W + P, Base - Sz * 0.88f}, {L + W + P * 1.4f, Base + Sz * 0.16f}, {L - P * 0.6f, Base + Sz * 0.2f}}, Hex(0xffffff));
			}
			else
			{
				C.FillRoundRect({L - P, Base - Sz * 0.86f, W + P * 2.0f, Sz * 1.1f}, 6.0f, Hex(0x111111));
			}
			Plain(X, Lines[I], Mx(X, 30.0f), Base, Sz, St == CapStyle::Marker ? Hex(0x14101c) : Hex(0xffffff), A);
		}
		else if (St == CapStyle::LowerThird || St == CapStyle::None)
		{
			const float Sz = Fit(C, Lines[I], Lh * 0.9f, 360.0f, false);
			Plain(X, Lines[I], Mx(X, 30.0f), Base, Sz, Mix(X.P.B, Hex(0x000000), 0.35f), A);
		}
		else
		{
			const float Sz = Fit(C, Lines[I], Lh * 0.86f, 350.0f, false);
			Loud(X, Lines[I], Mx(X, 30.0f), Base, Sz, Hex(0xffffff), Mix(X.P.B, Hex(0x000000), 0.35f), A, false);
		}
	}
	// A sticky note; a small face (storytime draws nobody but you).
	PropAt(X, static_cast<int>(Prop::StickyNote), 505.0f, 110.0f, 130.0f);
	if (HasFace(X))
	{
		Subject(X, 545.0f, 262.0f, 0.24f);
	}
	if (X.S->PropA >= 0 && X.S->PropA < static_cast<int>(Prop::Count) && X.S->PropA != static_cast<int>(Prop::StickyNote))
	{
		PropAt(X, X.S->PropA, 120.0f, 330.0f, 40.0f);
	}
	StickerAt(X, 440.0f, 40.0f, 200.0f, 150.0f);
}

void TheGhost(Ctx& X)
{
	Background(X, 440.0f, 140.0f);
	Ghost(X, 440.0f, 120.0f, 150.0f, 0.88f);
	Vignette(X, 0.85f);
	// The ghost is the subject (video::thumb deals Ghost here); the face under it is always the player's.
	if (HasFace(X))
	{
		Person(X, *X.Look, 228.0f, 172.0f, Zoom(X, 0.55f), X.S->Expr, X.S->Pose);
	}
	Captions(X, {130.0f, 330.0f, 380.0f, 54.0f, Align::Center});
	Props(X, 590.0f, 60.0f, 50.0f, 60.0f, 60.0f, 40.0f);
	StickerAt(X, 590.0f, 280.0f, 440.0f, 110.0f);
}

void Payday(Ctx& X)
{
	Background(X, 200.0f, 170.0f);
	Canvas& C = *X.C;
	std::string Amount;
	for (const std::string* Cap : {&X.Cap1, &X.Cap2})
	{
		if (Amount.empty() && Cap->find('$') != std::string::npos)
		{
			const size_t At = Cap->find('$');
			size_t J = At + 1;
			while (J < Cap->size() && (std::isdigit(static_cast<unsigned char>((*Cap)[J])) != 0 || (*Cap)[J] == ',' || (*Cap)[J] == '.'))
			{
				++J;
			}
			Amount = Cap->substr(At, J - At);
		}
	}
	Paper(X, {50.0f, 40.0f, 240.0f, 290.0f}, "STATEMENT", 8, Amount);
	CashStack(C, Mx(X, 340.0f), 320.0f, 90.0f, Many(X, 9, 6, 4));
	Subject(X, 506.0f, 168.0f, 0.52f);
	Captions(X, {330.0f, 96.0f, 290.0f, 50.0f, Align::Left});
	if (X.S->PropB >= 0 && X.S->PropB < static_cast<int>(Prop::Count))
	{
		PropAt(X, X.S->PropB, 600.0f, 300.0f, 40.0f);
	}
	StickerAt(X, 340.0f, 220.0f, 170.0f, 280.0f);
}

void MonsterHand(Ctx& X)
{
	Background(X, 320.0f, 200.0f);
	std::vector<Card> Hand = MadeHand(X);
	while (Hand.size() < 5)
	{
		Hand.push_back(-1);
	}
	Glow(*X.C, Mx(X, 300.0f), 210.0f, 230.0f, X.P.Acc, 0.35f);
	Fan(X, Hand, 290.0f, 220.0f, 96.0f, 0.16f, true);
	if (HasFace(X))
	{
		Subject(X, 568.0f, 250.0f, 0.3f);
	}
	Captions(X, {90.0f, 92.0f, 420.0f, 84.0f, Align::Center});
	Props(X, 70.0f, 300.0f, 50.0f, 590.0f, 70.0f, 40.0f);
	StickerAt(X, 590.0f, 80.0f, 290.0f, 220.0f);
}

void SiteCard(Ctx& X)
{
	Background(X, 220.0f, 170.0f);
	Canvas& C = *X.C;
	const bool Rival = X.Site > 0 && X.Site < static_cast<int>(net::SiteId::Count);
	// The reel: the multiple (the m fact, a FUSE night) big in its window, else the caption alone; the caption's lines
	// then ride in the burst.
	const std::string Window = X.Mult.empty() ? X.Cap1 : "\xC3\x97" + X.Mult;
	// The caption's style (C3 fix M1): a number burst puts the rest of the words in a burst and lights the window in the
	// accent; any other style writes them outlined and the window white.
	const bool Burster = X.S->CapStyle == static_cast<int16_t>(CapStyle::NumberBurst);
	Reel(X, 70.0f, 60.0f, 300.0f, 210.0f, Window, !Burster);
	if (Rival)
	{
		siteart::Logo(C, {Mleft(X, 70.0f, 300.0f) + 22.0f, 82.0f, 40.0f, 40.0f}, static_cast<net::SiteId>(X.Site));
	}
	Subject(X, 512.0f, 166.0f, 0.52f);
	const std::string Burst = Rest(X);
	if (!Burst.empty())
	{
		// C3 gate: with an initials badge or the logo in the top corner (578 .. 626), the burst's words stay left of it: a long
		// line ("SPECIAL GUEST", N5's deal) moves the burst in rather than running under the mark. (552: mirrored, it clears
		// Studio's Duel letter on an option's top-left corner.)
		float Bx = 552.0f;
		if (X.S->Corner == static_cast<int>(Corner::Initials) || X.S->Corner == static_cast<int>(Corner::Logo))
		{
			Bx = std::min(Bx, 562.0f - Wide(C, Burst, Fit(C, Burst, 30.0f, 300.0f, false), false) * 0.5f);
		}
		if (Burster)
		{
			BurstPlate(X, Burst, Mx(X, Bx), 60.0f, 30.0f, X.P.Acc, Mix(X.P.B, Hex(0x000000), 0.4f), 46.0f);
		}
		else
		{
			Loud(X, Burst, Mx(X, Bx), 70.0f, Fit(C, Burst, 30.0f, 300.0f, false), Hex(0xffffff), Hex(0x000000), Align::Center, false);
		}
	}
	Props(X, 590.0f, 300.0f, 40.0f, 420.0f, 300.0f, 40.0f);
	SiteStrip(X);
	StickerAt(X, 400.0f, 60.0f, 220.0f, 165.0f);
}

void Mistake(Ctx& X)
{
	Background(X, 200.0f, 160.0f);
	{
		const Upright Keep(X, 70.0f, 286.0f);
		Up(X, X.Mine.size() > 0 ? X.Mine[0] : -1, 70.0f, 50.0f, 116.0f, 162.0f);
		Up(X, X.Mine.size() > 1 ? X.Mine[1] : -1, 170.0f, 66.0f, 116.0f, 162.0f);
		RedX(*X.C, 186.0f, 140.0f, 70.0f, 16.0f);
	}
	Subject(X, 482.0f, 166.0f, 0.55f);
	Captions(X, {30.0f, 312.0f, 360.0f, 58.0f, Align::Left});
	Props(X, 340.0f, 60.0f, 50.0f, 600.0f, 300.0f, 40.0f);
	StickerAt(X, 340.0f, 90.0f, 186.0f, 140.0f);
}

// ------------------------------------------------------------------ C3 fix (L6): the second compositions
//
// Eighteen archetypes allow two layouts (video::thumb's table, VtTable in Game/VideoThumbs.cpp: the first is the default).
// The second drew the first's composition, so a recipe's Layout changed nothing on screen while rerun_content counted it as
// another look. Each composition below is its layout's own arrangement of the same true things: the same facts, captions,
// face, pose, props and sticker, with the same rules (the reading order of cards and boards kept through the mirror, the
// top corner left to the corner mark, an archetype's own prop drawn once).

/** The archetype's second layout (a video::thumb::LayoutId; VtTable's second column), -1 when it has only one. */
int SecondOf(int A)
{
	switch (static_cast<Arch>(A))
	{
	case Arch::GutPunch: return vt::LyCardsFace;
	case Arch::TheRead: return vt::LyFaceFan;
	case Arch::Caught: return vt::LyFaceFan;
	case Arch::TheFold: return vt::LyCardsFace;
	case Arch::TheBubble: return vt::LyNumberFace;
	case Arch::Headhunter: return vt::LyFaceWords;
	case Arch::Envelope: return vt::LyCollage;
	case Arch::DayCounter: return vt::LyGraph;
	case Arch::NightShift: return vt::LyFaceWords;
	case Arch::RoadTrip: return vt::LyFaceWords;
	case Arch::Classroom: return vt::LyCollage;
	case Arch::TopFive: return vt::LyCollage;
	case Arch::ReactionBox: return vt::LyFaceWords;
	case Arch::TheGhost: return vt::LySplitVs;
	case Arch::Payday: return vt::LyNumberFace;
	case Arch::MonsterHand: return vt::LyCardsFace;
	case Arch::SiteCard: return vt::LyNumberFace;
	case Arch::Mistake: return vt::LyFaceWords;
	default: return -1;
	}
}

/** Whether the recipe asks for its archetype's second composition (any other Layout, the first's included, draws the first). */
bool SecondLayout(const Ctx& X)
{
	const int L = SecondOf(X.S->Arch);
	return L >= 0 && X.S->Layout == L;
}

/** A collage's panel (before the mirror): a dark plate with the accent's edge. Returns its rect after the mirror. */
Rect CollagePanel(Ctx& X, float L, float T, float W, float H)
{
	Canvas& C = *X.C;
	const Rect R{Mleft(X, L, W), T, W, H};
	C.FillRoundRect({R.X + 4.0f, R.Y + 5.0f, R.W, R.H}, 10.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect(R, 10.0f, WithA(Mix(X.P.B, Hex(0x000000), 0.35f), 0.85f));
	C.StrokeRoundRect(R, 10.0f, X.P.Acc, 4.0f);
	return R;
}

/** The money amounts in a caption as written ("$50 · $214 · $500": "$50", "$214", "$500"), three at most. */
std::vector<std::string> DollarParts(const std::string& S)
{
	std::vector<std::string> Out;
	for (size_t I = 0; I < S.size() && Out.size() < 3; ++I)
	{
		if (S[I] != '$')
		{
			continue;
		}
		size_t J = I + 1;
		while (J < S.size() && (std::isdigit(static_cast<unsigned char>(S[J])) != 0 || S[J] == ',' || S[J] == '.' || S[J] == 'K' || S[J] == 'M'))
		{
			++J;
		}
		Out.push_back(S.substr(I, J - I));
		I = J;
	}
	return Out;
}

/** The lightning bolt between two faces with its VS (VS Split's), at Cx before the mirror. */
void VsBolt(Ctx& X, float Cx)
{
	Canvas& C = *X.C;
	Pts Zig = {{Cx - 10.0f, 70.0f}, {Cx - 30.0f, 168.0f}, {Cx - 2.0f, 168.0f}, {Cx - 24.0f, 270.0f}, {Cx + 32.0f, 150.0f}, {Cx + 2.0f, 150.0f}, {Cx + 24.0f, 70.0f}};
	for (Vec2& P : Zig)
	{
		P.X = Mx(X, P.X);
	}
	C.FillPolygon({Zig[0], Zig[1], Zig[2], Zig[5], Zig[6]}, Hex(Yellow));
	C.FillPolygon({Zig[2], Zig[3], Zig[4], Zig[5]}, Hex(Yellow));
	C.StrokePolyline(Zig, true, Hex(0x000000), 4.0f);
	Loud(X, "VS", Mx(X, Cx), 200.0f, 48.0f, Hex(0xffffff), Hex(0x000000), Align::Center, true);
}

/** Two cards for a fan: the facts' first two, else two backs. */
std::vector<Card> TwoOf(const std::vector<Card>& Cards)
{
	return Cards.size() >= 2 ? std::vector<Card>{Cards[0], Cards[1]} : std::vector<Card>{-1, -1};
}

void GutPunch2(Ctx& X)
{
	// Cards left, face right: yours big and lit, then theirs (backs unless turned up), the beat in words under them.
	Background(X, 470.0f, 190.0f);
	{
		const Upright Keep(X, 30.0f, 392.0f);
		Up(X, X.Mine.size() > 0 ? X.Mine[0] : -1, 30.0f, 40.0f, 88.0f, 123.0f, true);
		Up(X, X.Mine.size() > 1 ? X.Mine[1] : -1, 124.0f, 40.0f, 88.0f, 123.0f, true);
		Loud(X, "VS", 241.0f, 114.0f, 28.0f, Hex(0xffffff), Hex(0x000000), Align::Center, false);
		Up(X, X.Theirs.size() > 0 ? X.Theirs[0] : -1, 268.0f, 56.0f, 60.0f, 84.0f);
		Up(X, X.Theirs.size() > 1 ? X.Theirs[1] : -1, 332.0f, 56.0f, 60.0f, 84.0f);
	}
	Subject(X, 520.0f, 176.0f, 0.58f);
	Captions(X, {30.0f, 290.0f, 370.0f, 92.0f, Align::Left});
	Props(X, 470.0f, 320.0f, 60.0f, 590.0f, 110.0f, 50.0f);
	StickerAt(X, 432.0f, 62.0f, Kept(X, 30.0f, 392.0f, 330.0f), 98.0f);
}

void TheRead2(Ctx& X)
{
	// Face centre, the fans below: your two on one side, their two on the other ("??" over the backs until turned up).
	Background(X, 320.0f, 150.0f);
	// (Low enough that the bust runs off the bottom at either zoom: a centred face never floats.)
	Subject(X, 320.0f, 184.0f, 0.5f);
	Fan(X, TwoOf(X.Mine), 120.0f, 262.0f, 92.0f, 0.22f, true);
	Fan(X, TwoOf(X.Theirs), 520.0f, 262.0f, 92.0f, 0.22f, false);
	if (X.Theirs.empty())
	{
		Loud(X, "??", Mx(X, 520.0f), 290.0f, 62.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
	}
	// (The words sit low enough to clear Studio's Duel letter on an option's top-left corner.)
	Captions(X, {24.0f, 100.0f, 230.0f, 62.0f, Align::Left});
	Props(X, 120.0f, 262.0f, 60.0f, 520.0f, 262.0f, 60.0f);
	StickerAt(X, 440.0f, 64.0f, 520.0f, 262.0f);
}

void Caught2(Ctx& X)
{
	// Face centre (the facepalm), the bluff fanned across the chest under its CALLED stamp, the call's cards beside it.
	Background(X, 320.0f, 150.0f);
	Subject(X, 320.0f, 184.0f, 0.5f);
	Fan(X, TwoOf(X.Mine), 320.0f, 290.0f, 86.0f, 0.2f, false);
	const Color Plate = Hex(0x111111, 0.85f);
	StampBox(X, "CALLED", Mx(X, 320.0f), 304.0f, 40.0f, Hex(HotRed), &Plate);
	// (The words sit low enough to clear Studio's Duel letter on an option's top-left corner.)
	Captions(X, {24.0f, 102.0f, 236.0f, 60.0f, Align::Left});
	Props(X, 90.0f, 290.0f, 60.0f, 540.0f, 270.0f, 70.0f);
	StickerAt(X, 540.0f, 90.0f, 320.0f, 290.0f);
}

void TheFold2(Ctx& X)
{
	// Cards left, face right: the premium hand fanned and lit, struck through, the question under it.
	Background(X, 210.0f, 150.0f);
	Fan(X, TwoOf(X.Mine), 200.0f, 136.0f, 116.0f, 0.24f, true);
	RedX(*X.C, Mx(X, 200.0f), 140.0f, 64.0f, 15.0f);
	Subject(X, 500.0f, 168.0f, 0.56f);
	Captions(X, {36.0f, 318.0f, 340.0f, 76.0f, Align::Left});
	Props(X, 600.0f, 300.0f, 50.0f, 600.0f, 300.0f, 50.0f);
	StickerAt(X, 380.0f, 56.0f, 200.0f, 140.0f);
}

void TheBubble2(Ctx& X)
{
	// The number and a face: how far from the money in words huge, the short stack's last chips beside them.
	Background(X, 260.0f, 180.0f);
	ChipTower(*X.C, Mx(X, 64.0f), 334.0f, 28.0f, Many(X, 8, 7, 5), X.Mirror ? -2.5f : 2.5f, Hex(0xff6b57), X.Tier == 0);
	Subject(X, 530.0f, 206.0f, 0.44f);
	Captions(X, {100.0f, 200.0f, 330.0f, 124.0f, Align::Center});
	Props(X, 300.0f, 320.0f, 60.0f, 270.0f, 322.0f, 70.0f);
	StickerAt(X, 440.0f, 66.0f, 265.0f, 150.0f);
}

void Headhunter2(Ctx& X)
{
	// Face left, words right: you, and the bust as a target card under the words (the crosshair on their face, the bounty in
	// a burst at its corner).
	Background(X, 200.0f, 170.0f);
	Canvas& C = *X.C;
	Hunter(X, 170.0f, 168.0f, 0.56f);
	Captions(X, {330.0f, 128.0f, 290.0f, 74.0f, Align::Left});
	const Rect Tgt{Mleft(X, 372.0f, 236.0f), 204.0f, 236.0f, 140.0f};
	C.FillRoundRect({Tgt.X + 4.0f, Tgt.Y + 5.0f, Tgt.W, Tgt.H}, 8.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect(Tgt, 8.0f, Mix(X.P.B, Hex(0x000000), 0.45f));
	C.PushClip(Tgt);
	Opponent(X, 490.0f, 262.0f, Zoom(X, 0.3f), 5, true);
	C.PopClip();
	Crosshair(C, Mx(X, 490.0f), 270.0f, 60.0f);
	C.StrokeRoundRect(Tgt, 8.0f, Hex(HotRed), 4.0f);
	if (!X.Big.empty())
	{
		BurstPlate(X, X.Big, Mx(X, 594.0f), 222.0f, 30.0f, X.P.Acc, Mix(X.P.B, Hex(0x000000), 0.35f), 40.0f);
	}
	Props(X, 600.0f, 300.0f, 40.0f, 330.0f, 318.0f, 40.0f);
	StickerAt(X, 340.0f, 232.0f, 490.0f, 270.0f);
}

void Envelope2(Ctx& X)
{
	// A collage: the torn envelope with the multiple on it (the big panel), your face (top right), the rest of the words
	// (bottom right; with none, the face takes the column).
	Background(X, 180.0f, 180.0f);
	Canvas& C = *X.C;
	CollagePanel(X, 20.0f, 20.0f, 330.0f, 320.0f);
	const Rect R{Mleft(X, 48.0f, 274.0f), 120.0f, 274.0f, 176.0f};
	Envelope(C, R, true);
	const std::string OnIt = X.Mult.empty() ? X.Cap1 : X.Mult + "\xC3\x97";
	const std::string Tail = Rest(X);
	if (Tail.empty() && X.S->CapStyle == static_cast<int16_t>(CapStyle::NumberBurst))
	{
		BigStyled(X, OnIt, R.X + R.W * 0.5f, R.Y + R.H * 0.76f, 86.0f, 236.0f, Align::Center, false, Hex(0xf5c542), Hex(0x3a2a10));
	}
	else
	{
		const float Sz = Fit(C, OnIt, 74.0f, 236.0f, false);
		Loud(X, OnIt, R.X + R.W * 0.5f, R.Y + R.H * 0.8f, Sz, Hex(0xf5c542), Hex(0x3a2a10), Align::Center, false);
	}
	const Rect Fp = CollagePanel(X, 362.0f, 20.0f, 258.0f, Tail.empty() ? 320.0f : 196.0f);
	C.PushClip(Fp);
	Subject(X, 491.0f, Tail.empty() ? 170.0f : 122.0f, 0.5f); // the bust runs past the panel's bottom
	C.PopClip();
	if (!Tail.empty())
	{
		CollagePanel(X, 362.0f, 228.0f, 258.0f, 112.0f);
		Captions(X, {376.0f, 300.0f, 230.0f, 52.0f, Align::Center}, Tail, std::string());
	}
	Props(X, 185.0f, 200.0f, 50.0f, 110.0f, 64.0f, 46.0f);
	StickerAt(X, 280.0f, 66.0f, 185.0f, 200.0f);
}

void DayCounter2(Ctx& X)
{
	// The graph: the day over the road from the start to the goal's flag, travelled to where the challenge has got to (the p
	// fact, else the caption's amounts: start, now and goal, or start and now); with neither, the day and its words (no road).
	Background(X, 240.0f, 160.0f);
	Canvas& C = *X.C;
	Subject(X, 540.0f, 206.0f, 0.44f);
	// (52 in: the day clears Studio's Duel letter on an option's top-left corner.)
	BigStyled(X, X.Cap1, Mx(X, 52.0f), 86.0f, 72.0f, 360.0f, X.Mirror ? Align::Right : Align::Left, true, Hex(0xffffff), Mix(X.P.B, Hex(0x000000), 0.4f));
	const std::vector<double> A = Amounts(X.Cap2);
	if (A.size() >= 2 || X.Progress >= 0)
	{
		const float Fill = static_cast<float>(X.Progress >= 0 ? X.Progress / 100.0 : A.size() >= 3 && A[2] > A[0] ? std::clamp((A[1] - A[0]) / (A[2] - A[0]), 0.0, 1.0) : 1.0);
		// The road reads from the start to the goal on either side of the mirror.
		const Upright Keep(X, 40.0f, 470.0f);
		// (The flag stays under the day's words: mirrored, the road moves under them.)
		const Vec2 S0{64.0f, 290.0f}, G{396.0f, 176.0f};
		auto At = [&](float T) { return Vec2{S0.X + (G.X - S0.X) * T, S0.Y + (G.Y - S0.Y) * T}; };
		const Vec2 Now = At(Fill);
		const int Dashes = Many(X, 10, 7, 4);
		for (int K = 0; K < Dashes; ++K)
		{
			const float T0 = Fill + (1.0f - Fill) * static_cast<float>(K) / static_cast<float>(Dashes);
			const float T1 = T0 + (1.0f - Fill) * 0.55f / static_cast<float>(Dashes);
			C.StrokePolyline({At(T0), At(T1)}, false, WithA(Hex(0xffffff), 0.6f), 6.0f, true);
		}
		C.StrokePolyline({S0, Now}, false, Color{0.0f, 0.0f, 0.0f, 0.35f}, 16.0f, true);
		C.StrokePolyline({S0, Now}, false, X.P.Acc, 10.0f, true);
		C.FillCircle(S0.X, S0.Y, 9.0f, Hex(0xffffff));
		// The goal's flag.
		C.FillRect({G.X - 2.0f, G.Y - 58.0f, 5.0f, 62.0f}, Hex(0xf4f4f4));
		C.FillPolygon({{G.X + 3.0f, G.Y - 58.0f}, {G.X + 44.0f, G.Y - 45.0f}, {G.X + 3.0f, G.Y - 32.0f}}, Hex(HotRed));
		C.FillCircle(Now.X, Now.Y, 13.0f, Hex(0xffffff));
		C.FillCircle(Now.X, Now.Y, 8.0f, X.P.Acc);
		const std::vector<std::string> Parts = DollarParts(X.Cap2);
		if (Parts.size() == 1)
		{
			// One amount beside the p fact: today's, where the road has got to.
			Loud(X, Parts[0], Now.X, Now.Y - 22.0f, 26.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
		}
		else if (Parts.size() >= 2)
		{
			Loud(X, Parts[0], S0.X - 8.0f, S0.Y + 32.0f, 24.0f, Hex(0xffffff), Hex(0x000000), Align::Left, false);
			const std::string& Goal = Parts.size() >= 3 ? Parts[2] : Parts[1];
			if (Parts.size() >= 3)
			{
				Loud(X, Parts[1], Now.X, Now.Y - 22.0f, 26.0f, X.P.Acc, Hex(0x000000), Align::Center, false);
			}
			// The goal beside its flag, on the road's side (a raised arm reaches the other).
			Loud(X, Goal, G.X - 10.0f, G.Y - 24.0f, 24.0f, Parts.size() >= 3 ? Hex(0xffffff) : X.P.Acc, Hex(0x000000), Align::Right, false);
		}
	}
	else if (!X.Cap2.empty())
	{
		Loud(X, X.Cap2, Mx(X, 52.0f), 170.0f, Fit(C, X.Cap2, 48.0f, 400.0f, false), X.P.Acc, Hex(0x000000), X.Mirror ? Align::Right : Align::Left, false);
	}
	Props(X, 300.0f, 330.0f, 40.0f, 470.0f, 62.0f, 46.0f);
	SiteStrip(X);
	StickerAt(X, 440.0f, 60.0f, 240.0f, 230.0f);
}

void NightShift2(Ctx& X)
{
	// Face left, words right: the tired face lit from below by the screen, the hour big, the laptop with its tables, the moon.
	Background(X, 190.0f, 220.0f);
	Canvas& C = *X.C;
	Glow(C, Mx(X, 200.0f), 300.0f, 260.0f, Hex(0x6fb8ff), 0.32f);
	C.FillCircle(Mx(X, 400.0f), 56.0f, 24.0f, Hex(0xfff3c4));
	C.FillCircle(Mx(X, 410.0f), 49.0f, 21.0f, Mix(X.P.A, X.P.B, 0.2f));
	Subject(X, 190.0f, 166.0f, 0.58f);
	Captions(X, {330.0f, 180.0f, 290.0f, 92.0f, Align::Left});
	Laptop(C, Mx(X, 476.0f), 288.0f, 64.0f, Hex(0x6fb8ff), 4);
	Props(X, 476.0f, 288.0f, 50.0f, 600.0f, 296.0f, 48.0f);
	StickerAt(X, 330.0f, 290.0f, 190.0f, 166.0f);
}

void RoadTrip2(Ctx& X)
{
	// Face left, words right: you on the night road, the bus coming along it under the city's lights.
	Background(X, 200.0f, 140.0f);
	Canvas& C = *X.C;
	const int N = Many(X, 12, 8, 6);
	const float Bw = 340.0f / static_cast<float>(N);
	for (int I = 0; I < N; ++I)
	{
		const float H = 24.0f + Hf(X, 1600u + static_cast<uint32_t>(I)) * 50.0f;
		const float Bx = Mleft(X, 300.0f + static_cast<float>(I) * Bw, Bw - 4.0f);
		C.FillRect({Bx, 292.0f - H, Bw - 4.0f, H}, Mix(X.P.B, Hex(0x000000), 0.45f));
		for (int K = 0; K < Many(X, 2, 1, 1); ++K)
		{
			C.FillRect({Bx + 5.0f + static_cast<float>(K) * 9.0f, 292.0f - H + 6.0f + Hf(X, 1700u + static_cast<uint32_t>(I * 3 + K)) * (H - 14.0f), 4.0f, 4.0f}, Hex(0xffe08a, 0.85f));
		}
	}
	C.FillRect({0.0f, 292.0f, DesignW, 68.0f}, Mix(X.P.B, Hex(0x000000), 0.6f));
	for (int K = 0; K < 5; ++K)
	{
		C.FillRect({Mleft(X, 300.0f + static_cast<float>(K) * 68.0f, 34.0f), 330.0f, 34.0f, 4.0f}, Hex(0xffe08a, 0.7f));
	}
	Bus(C, Mleft(X, 390.0f, 200.0f), 222.0f, 200.0f, Hex(0x2b6cb0));
	Subject(X, 170.0f, 168.0f, 0.58f);
	Captions(X, {330.0f, 112.0f, 290.0f, 76.0f, Align::Left});
	Props(X, 600.0f, 312.0f, 40.0f, 600.0f, 312.0f, 40.0f);
	StickerAt(X, 330.0f, 190.0f, 490.0f, 268.0f);
}

void Classroom2(Ctx& X)
{
	// A collage: the whiteboard (the range and the hand), your face pointing in the panel beside it, the lesson's words in a
	// band below.
	Background(X, 220.0f, 140.0f);
	Canvas& C = *X.C;
	Whiteboard(X, {20.0f, 20.0f, 390.0f, 220.0f});
	const Rect Fp = CollagePanel(X, 424.0f, 20.0f, 196.0f, 220.0f);
	C.PushClip(Fp);
	Subject(X, 522.0f, 112.0f, 0.4f);
	C.PopClip();
	CollagePanel(X, 20.0f, 254.0f, 600.0f, 90.0f);
	Captions(X, {40.0f, 318.0f, 560.0f, 52.0f, Align::Center});
	Props(X, 330.0f, 190.0f, 40.0f, 300.0f, 190.0f, 60.0f);
	StickerAt(X, 600.0f, 300.0f, 140.0f, 130.0f);
}

void TopFive2(Ctx& X)
{
	// A collage of five: number one big with its crown and your face, two to five in a grid of four, the words in a band
	// below (the r fact's rows in the cells, else blank bars).
	Background(X, 300.0f, 180.0f);
	Canvas& C = *X.C;
	const Color Ink1 = Mix(X.P.B, Hex(0x000000), 0.3f);
	const Rect One{Mleft(X, 20.0f, 250.0f), 20.0f, 250.0f, 248.0f};
	C.FillRoundRect({One.X + 4.0f, One.Y + 5.0f, One.W, One.H}, 12.0f, Color{0.0f, 0.0f, 0.0f, 0.35f});
	C.FillRoundRect(One, 12.0f, WithA(X.P.Acc, 0.95f));
	// The panel's words read left to right whichever side it's on; your face sits in its lower right.
	C.PushClip(One);
	if (HasFace(X))
	{
		Subject(X, X.Mirror ? DesignW - (One.X + 168.0f) : One.X + 168.0f, 198.0f, 0.34f);
	}
	C.PopClip();
	Crown(C, One.X + 40.0f, One.Y + 30.0f, 18.0f, Hex(Yellow));
	Plain(X, "1", One.X + 40.0f, One.Y + 112.0f, 70.0f, Ink1, Align::Center);
	if (!X.Rows.empty())
	{
		const float Sz = Fit(C, X.Rows[0], 30.0f, 160.0f, false);
		Plain(X, X.Rows[0], One.X + 80.0f, One.Y + 100.0f, Sz, Ink1, Align::Left);
	}
	// Two to five in reading order on either side of the mirror (the block moves; its cells keep their order), each number
	// low in its cell, clear of the corner mark over the top row.
	static const char* const Nums[5] = {"1", "2", "3", "4", "5"};
	const float Gl = Mleft(X, 282.0f, 334.0f);
	for (int K = 1; K < 5; ++K)
	{
		const Rect Cell{Gl + static_cast<float>((K - 1) % 2) * 172.0f, 20.0f + static_cast<float>((K - 1) / 2) * 128.0f, 162.0f, 120.0f};
		C.FillRoundRect(Cell, 10.0f, Color{0.0f, 0.0f, 0.0f, 0.45f});
		C.StrokeRoundRect(Cell, 10.0f, WithA(X.P.Acc, 0.7f), 3.0f);
		Plain(X, Nums[K], Cell.X + 26.0f, Cell.Y + 80.0f, 40.0f, X.P.Acc, Align::Center);
		if (static_cast<size_t>(K) < X.Rows.size())
		{
			const float Sz = Fit(C, X.Rows[static_cast<size_t>(K)], 20.0f, 146.0f, false);
			Plain(X, X.Rows[static_cast<size_t>(K)], Cell.X + 81.0f, Cell.Y + 108.0f, Sz, Hex(0xffffff), Align::Center);
		}
		else
		{
			for (int I = 0; I < 2; ++I)
			{
				C.FillRoundRect({Cell.X + 52.0f, Cell.Y + 58.0f + static_cast<float>(I) * 22.0f, 90.0f - static_cast<float>(I) * 24.0f, 12.0f}, 6.0f, Color{1.0f, 1.0f, 1.0f, 0.25f});
			}
		}
	}
	CollagePanel(X, 20.0f, 276.0f, 600.0f, 72.0f);
	Captions(X, {40.0f, 320.0f, 560.0f, 44.0f, Align::Center});
	StickerAt(X, 236.0f, 236.0f, 145.0f, 150.0f);
}

void ReactionBox2(Ctx& X)
{
	// Face left, the news right: your reaction big, the result as a card (their face, his ghost or the trophy; the event),
	// the words on the card.
	Background(X, 200.0f, 170.0f);
	Canvas& C = *X.C;
	if (HasFace(X))
	{
		Person(X, *X.Look, 166.0f, 168.0f, Zoom(X, 0.56f), X.S->Expr, X.S->Pose);
	}
	const Rect News = CollagePanel(X, 336.0f, 24.0f, 284.0f, 318.0f);
	C.PushClip({News.X, News.Y, News.W, 190.0f});
	if (X.Other == RivalName)
	{
		Opponent(X, 478.0f, 112.0f, Zoom(X, 0.36f), 3, true);
	}
	else if (!X.Other.empty())
	{
		Person(X, hero::CharacterFor(X.Other), 478.0f, 112.0f, Zoom(X, 0.36f), 3, 4, true);
	}
	else
	{
		Trophy(C, Mx(X, 478.0f), 118.0f, 104.0f, Hex(0xf5c542));
	}
	C.PopClip();
	if (!X.Event.empty())
	{
		Loud(X, X.Event, Mx(X, 478.0f), 244.0f, Fit(C, X.Event, 24.0f, 250.0f, true), X.P.Acc, Hex(0x000000), Align::Center, true);
	}
	Captions(X, {350.0f, 300.0f, 256.0f, 46.0f, Align::Center});
	Props(X, 306.0f, 72.0f, 40.0f, 306.0f, 300.0f, 36.0f);
	StickerAt(X, 304.0f, 186.0f, 478.0f, 112.0f);
}

void TheGhost2(Ctx& X)
{
	// The split: you against him, his half in shadow, the bolt and its VS between, the words below.
	Background(X, 320.0f, 170.0f);
	Canvas& C = *X.C;
	C.FillPolygon({{Mx(X, 330.0f), 0.0f}, {Mx(X, 640.0f), 0.0f}, {Mx(X, 640.0f), 360.0f}, {Mx(X, 290.0f), 360.0f}}, WithA(Mix(X.P.B, Hex(0x000000), 0.3f), 0.7f));
	Ghost(X, 480.0f, 150.0f, 118.0f, 0.92f);
	if (HasFace(X))
	{
		Person(X, *X.Look, 160.0f, 172.0f, Zoom(X, 0.5f), X.S->Expr, X.S->Pose);
	}
	VsBolt(X, 318.0f);
	Captions(X, {120.0f, 342.0f, 400.0f, 56.0f, Align::Center});
	Props(X, 590.0f, 60.0f, 50.0f, 60.0f, 60.0f, 40.0f);
	StickerAt(X, 590.0f, 280.0f, 480.0f, 130.0f);
}

void Payday2(Ctx& X)
{
	// The number and a face: the payout's words huge, the statement and a cash stack under them.
	Background(X, 280.0f, 160.0f);
	Canvas& C = *X.C;
	Paper(X, {28.0f, 200.0f, 112.0f, 140.0f}, "STATEMENT", 4, std::string());
	CashStack(C, Mx(X, 262.0f), 340.0f, 84.0f, Many(X, 7, 5, 4));
	Subject(X, 530.0f, 190.0f, 0.46f);
	Captions(X, {150.0f, 168.0f, 320.0f, 116.0f, Align::Center});
	StickerAt(X, 370.0f, 300.0f, 262.0f, 300.0f);
}

void MonsterHand2(Ctx& X)
{
	// Cards left, face right: the five that make it, upright in their row and lit, the words under them.
	Background(X, 240.0f, 150.0f);
	std::vector<Card> Hand = MadeHand(X);
	while (Hand.size() < 5)
	{
		Hand.push_back(-1);
	}
	{
		const Upright Keep(X, 24.0f, 438.0f);
		for (size_t I = 0; I < 5; ++I)
		{
			Up(X, Hand[I], 24.0f + static_cast<float>(I) * 84.0f, 64.0f, 78.0f, 109.0f, true);
		}
	}
	Subject(X, 548.0f, 196.0f, 0.44f);
	Captions(X, {30.0f, 290.0f, 410.0f, 88.0f, Align::Center});
	Props(X, 600.0f, 300.0f, 40.0f, 590.0f, 70.0f, 40.0f);
	StickerAt(X, 470.0f, 56.0f, Kept(X, 24.0f, 438.0f, 231.0f), 118.0f);
}

void SiteCard2(Ctx& X)
{
	// The number and a face: the multiple (or the night's first words) huge in a burst, the site's mark over it, the rest of
	// the words under it, its strip along the bottom.
	Background(X, 250.0f, 160.0f);
	Canvas& C = *X.C;
	const bool Rival = X.Site > 0 && X.Site < static_cast<int>(net::SiteId::Count);
	const std::string Big = X.Mult.empty() ? X.Cap1 : "\xC3\x97" + X.Mult;
	const std::string Tail = Rest(X);
	BigStyled(X, Big, Mx(X, 236.0f), 178.0f, 100.0f, 330.0f, Align::Center, true, X.P.Acc, Hex(0x000000));
	if (Rival)
	{
		siteart::Logo(C, {Mleft(X, 30.0f, 64.0f), 28.0f, 64.0f, 64.0f}, static_cast<net::SiteId>(X.Site));
	}
	if (!Tail.empty())
	{
		Loud(X, Tail, Mx(X, 236.0f), 316.0f, Fit(C, Tail, 40.0f, 420.0f, false), Hex(0xffffff), Hex(0x000000), Align::Center, false);
	}
	Subject(X, 540.0f, 190.0f, 0.46f);
	Props(X, 590.0f, 300.0f, 40.0f, 420.0f, 300.0f, 40.0f);
	SiteStrip(X);
	StickerAt(X, 440.0f, 60.0f, 236.0f, 150.0f);
}

void Mistake2(Ctx& X)
{
	// Face left, words right: the facepalm big, the cards that cost it struck through under the words.
	Background(X, 200.0f, 160.0f);
	Subject(X, 180.0f, 168.0f, 0.58f);
	Captions(X, {330.0f, 132.0f, 290.0f, 80.0f, Align::Left});
	{
		const Upright Keep(X, 404.0f, 566.0f);
		Up(X, X.Mine.size() > 0 ? X.Mine[0] : -1, 404.0f, 206.0f, 84.0f, 118.0f);
		Up(X, X.Mine.size() > 1 ? X.Mine[1] : -1, 482.0f, 218.0f, 84.0f, 118.0f);
		RedX(*X.C, 490.0f, 274.0f, 54.0f, 13.0f);
	}
	Props(X, 600.0f, 300.0f, 40.0f, 600.0f, 300.0f, 40.0f);
	StickerAt(X, 600.0f, 200.0f, Kept(X, 404.0f, 566.0f, 490.0f), 274.0f);
}

/** The archetype's composition (out of range, and POV in P0: the text slab). */
void Compose(Ctx& X)
{
	// The second composition when the recipe's Layout is its archetype's second (C3 fix L6).
	const bool Two = SecondLayout(X);
	switch (static_cast<Arch>(X.S->Arch))
	{
	case Arch::GutPunch: Two ? GutPunch2(X) : GutPunch(X); break;
	case Arch::CircledRiver: CircledRiver(X); break;
	case Arch::TheRead: Two ? TheRead2(X) : TheRead(X); break;
	case Arch::NervesOfSteel: NervesOfSteel(X); break;
	case Arch::Caught: Two ? Caught2(X) : Caught(X); break;
	case Arch::TheFold: Two ? TheFold2(X) : TheFold(X); break;
	case Arch::ThePile: ThePile(X); break;
	case Arch::BigNumber: BigNumber(X); break;
	case Arch::BeforeAfter: BeforeAfter(X); break;
	case Arch::TrophyShot: TrophyShot(X); break;
	case Arch::VsSplit: VsSplit(X); break;
	case Arch::TheBubble: Two ? TheBubble2(X) : TheBubble(X); break;
	case Arch::Headhunter: Two ? Headhunter2(X) : Headhunter(X); break;
	case Arch::Envelope: Two ? Envelope2(X) : EnvelopeArch(X); break;
	case Arch::TheGraph: TheGraph(X); break;
	case Arch::DayCounter: Two ? DayCounter2(X) : DayCounter(X); break;
	case Arch::NightShift: Two ? NightShift2(X) : NightShift(X); break;
	case Arch::Venue: Venue(X); break;
	case Arch::RoadTrip: Two ? RoadTrip2(X) : RoadTrip(X); break;
	case Arch::RentLetter: RentLetter(X); break;
	case Arch::Classroom: Two ? Classroom2(X) : Classroom(X); break;
	case Arch::Quiz: Quiz(X); break;
	case Arch::TierList: TierList(X); break;
	case Arch::TopFive: Two ? TopFive2(X) : TopFive(X); break;
	case Arch::DeskSetup: DeskSetup(X); break;
	case Arch::CollabDuo: CollabDuo(X); break;
	case Arch::ReactionBox: Two ? ReactionBox2(X) : ReactionBox(X); break;
	case Arch::StreamClip: StreamClip(X); break;
	case Arch::Milestone: Milestone(X); break;
	case Arch::TheGhost: Two ? TheGhost2(X) : TheGhost(X); break;
	case Arch::Payday: Two ? Payday2(X) : Payday(X); break;
	case Arch::MonsterHand: Two ? MonsterHand2(X) : MonsterHand(X); break;
	case Arch::SiteCard: Two ? SiteCard2(X) : SiteCard(X); break;
	case Arch::Mistake: Two ? Mistake2(X) : Mistake(X); break;
	default: TextSlab(X); break;
	}
	CornerMark(X);
}

// ------------------------------------------------------------------ keys

uint64_t Fnv(uint64_t H, const void* Data, size_t N)
{
	const unsigned char* P = static_cast<const unsigned char*>(Data);
	for (size_t I = 0; I < N; ++I)
	{
		H ^= P[I];
		H *= 0x100000001b3ull;
	}
	return H;
}

uint64_t FnvStr(uint64_t H, const std::string& S)
{
	H = Fnv(H, S.data(), S.size());
	const unsigned char Sep = 0x1f;
	return Fnv(H, &Sep, 1);
}

template <typename T>
uint64_t FnvVal(uint64_t H, const T& V)
{
	return Fnv(H, &V, sizeof(V));
}
} // namespace thumbart_detail

// ---------------------------------------------------------------------------------------------------------------- API

const char* ArchName(int A)
{
	static const char* const Names[ArchCount] = {"Gut Punch", "Circled River", "The Read", "Nerves of Steel", "Caught", "The Fold", "The Pile", "Big Number", "Before \xE2\x86\x92 After",
		"Trophy Shot", "VS Split", "The Bubble", "Headhunter", "Envelope", "The Graph", "Day Counter", "Night Shift", "Venue", "Road Trip", "Rent Letter", "Classroom", "Quiz", "Tier List",
		"Top Five", "Desk Setup", "Collab Duo", "Reaction Box", "Stream Clip", "Milestone", "Text Slab", "The Ghost", "Payday", "Monster Hand", "POV", "Site Card", "Mistake"};
	return A >= 0 && A < ArchCount ? Names[A] : "";
}

void Compose(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, bool Lod)
{
	Compose(C, R, S, Look, Lod, video::Thumb());
}

void Compose(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, bool Lod, const video::Thumb& Facts)
{
	using namespace thumbart_detail;
	if (R.W <= 1.0f || R.H <= 1.0f)
	{
		return;
	}
	Ctx X;
	X.C = &C;
	X.S = &S;
	X.Look = &Look;
	X.F = &Facts;
	X.Lod = Lod;
	X.Tier = Lod ? 2 : R.W >= 400.0f ? 0 : 1;
	// Mirror: bit 0 the layout mirrored, bit 1 the wide zoom (video::thumb's contract).
	X.Mirror = (S.Mirror & 1) != 0;
	X.Tight = (S.Mirror & 2) == 0;
	X.V = S.Variant;
	X.Cap1 = S.Caption;
	ReadFacts(X, S, Facts);
	X.P = PaletteOf(S.Pal, X.Site, S.Variant);
	// The 640 x 360 design space, scaled evenly into R and centred (a tile is 16:9).
	const float K = std::min(R.W / DesignW, R.H / DesignH);
	C.PushClip(R);
	C.Save();
	C.Translate(R.X + (R.W - DesignW * K) * 0.5f, R.Y + (R.H - DesignH * K) * 0.5f);
	C.Scale(K, K);
	thumbart_detail::Compose(X);
	C.Restore();
	C.PopClip();
}

uint64_t KeyOf(const video::ThumbSpec& S, const hero::Character& Look, const video::Thumb& Facts, float W, float H, bool Lod)
{
	using namespace thumbart_detail;
	uint64_t K = 0xcbf29ce484222325ull ^ 0x7468756d62617274ull; // "thumbart"
	for (const int16_t V : {S.Arch, S.Layout, S.Mirror, S.Bg, S.Pal, S.Subj, S.Expr, S.Pose, S.PropA, S.PropB, S.CapStyle, S.Sticker, S.Corner, S.LookRef})
	{
		K = FnvVal(K, V);
	}
	K = FnvVal(K, S.Variant);
	K = FnvStr(K, S.Caption);
	K = FnvStr(K, S.Caption2);
	const hero::Look& L = Look.Appearance;
	for (const int V : {L.Body, L.Face, L.Skin, L.Eyes, L.Brows, L.Hair, L.HairColor, L.FacialHair, L.Build, L.Height, L.Outfit, L.OutfitColor, L.Glasses, L.Hat, Look.Age})
	{
		K = FnvVal(K, V);
	}
	K = FnvStr(K, Look.FirstName);
	K = FnvStr(K, Look.LastName);
	K = FnvStr(K, Facts.Cards);
	K = FnvStr(K, Facts.Opponent);
	K = FnvVal(K, Facts.Site);
	K = FnvVal(K, W);
	K = FnvVal(K, H);
	K = FnvVal(K, static_cast<uint8_t>(Lod ? 1 : 0));
	return K;
}

void Swatch(Canvas& C, Rect R, int Pal, int Site, uint32_t Variant)
{
	using namespace thumbart_detail;
	const Palette P = PaletteOf(Pal, Site, Variant);
	C.FillRoundRect(R, std::min(6.0f, R.H * 0.25f), Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, P.A, P.B));
	C.FillRoundRect({R.X + R.W * 0.16f, R.Y + R.H * 0.6f, R.W * 0.68f, std::max(3.0f, R.H * 0.18f)}, 1.5f, P.Acc);
}

void Skeleton(Canvas& C, Rect R, const video::ThumbSpec& S)
{
	using namespace thumbart_detail;
	const Palette P = PaletteOf(S.Pal, 0, S.Variant);
	const Color A = Mix(P.A, Hex(videoart::Ink), 0.62f), B = Mix(P.B, Hex(videoart::Ink), 0.7f);
	C.FillRoundRect(R, std::min(8.0f, R.W * 0.04f), Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, A, B));
	// Where the caption will be.
	const float K = R.W / DesignW;
	C.FillRoundRect({R.X + 30.0f * K, R.Y + R.H - 70.0f * K, 260.0f * K, 26.0f * K}, 6.0f * K, Color{1.0f, 1.0f, 1.0f, 0.1f});
	C.FillRoundRect({R.X + 30.0f * K, R.Y + R.H - 38.0f * K, 170.0f * K, 18.0f * K}, 6.0f * K, Color{1.0f, 1.0f, 1.0f, 0.07f});
}

bool Draw(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, const video::Thumb& Facts, Budget& B)
{
	return Draw(C, R, S, Look, Facts, B, LodFor(R.W));
}

bool Draw(Canvas& C, Rect R, const video::ThumbSpec& S, const hero::Character& Look, const video::Thumb& Facts, Budget& B, bool Lod)
{
	const uint64_t Key = KeyOf(S, Look, Facts, R.W, R.H, Lod);
	const Rect Local{0.0f, 0.0f, R.W, R.H};
	auto Paint = [&S, &Look, &Facts, Local, Lod](Canvas& Cv) { Compose(Cv, Local, S, Look, Lod, Facts); };
	if (C.IsCached(Key))
	{
		C.Cached(Key, R.X, R.Y, Paint, Canvas::CachePool::Rerun);
		return true;
	}
	if (B.Painted < B.Max)
	{
		++B.Painted;
		C.Cached(Key, R.X, R.Y, Paint, Canvas::CachePool::Rerun);
		return true;
	}
	++B.Waiting;
	Skeleton(C, R, S);
	return false;
}

hero::Character LookFor(const video::ThumbSpec& S, const video::Channel& Ch, const hero::Character& Current)
{
	hero::Character Out = Current;
	for (const video::FrozenLook& F : Ch.X.Looks)
	{
		if (S.LookRef >= 0 && F.Ref == S.LookRef)
		{
			for (int K = 0; K < hero::SlotCount; ++K)
			{
				Out.Appearance.Set(static_cast<hero::Slot>(K), F.Look[static_cast<size_t>(K)]);
			}
		}
	}
	return Out;
}

std::vector<std::string> Words()
{
	return {"VS", "??", "?", "+", "$", "CALL", "FOLD", "CALLED", "ALL IN", "EMBERCREST", "NOTICE TO VACATE", "POWER BILL", "RENT DUE", "STATEMENT", "PAWN", "S", "A", "B", "C", "D", "R", "ON AIR"};
}

bool VideoThumb(Canvas& C, Rect R, const video::Video& V, const video::Channel& Ch, const hero::Character& Current, Budget& B)
{
	if (V.X.On)
	{
		const video::ThumbSpec& S = V.X.Spec[static_cast<size_t>(std::clamp(static_cast<int>(V.X.DuelWin), 0, 2))];
		if (S.Arch >= 0)
		{
			return Draw(C, R, S, LookFor(S, Ch, Current), V.Th, B);
		}
	}
	// No recipe: Update 2's frame, exactly as before.
	videoart::Thumbnail(C, R, V.Th, &Current);
	return true;
}
} // namespace thumbart
} // namespace ui
} // namespace ss
