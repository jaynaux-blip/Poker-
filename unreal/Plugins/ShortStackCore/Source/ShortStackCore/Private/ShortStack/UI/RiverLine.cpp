#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sizing.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Structure.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/BadgeArt.h"
#include "ShortStack/UI/SiteArt.h"
#include "IgniteArt.h"
#include "RiverLineShared.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace ss
{
namespace ui
{
namespace riverline_detail
{
const float RlW = RiverLine::Width;
const float RlH = RiverLine::Height;
const float TableCx = 590.0f;
const float TableCy = 430.0f;
const float TableRx = 420.0f;
const float TableRy = 226.0f;
const float RlTop = 64.0f;
const float RlSideX = 1188.0f;

Color GradeColor(Grade G)
{
	switch (G)
	{
	case Grade::Best: return pal::Green;
	case Grade::Good: return pal::Accent;
	case Grade::Inaccuracy: return pal::Gold;
	case Grade::Mistake: return pal::Orange;
	default: return pal::Red;
	}
}

const char* GradeIconText(Grade G)
{
	switch (G)
	{
	case Grade::Inaccuracy: return "?!";
	case Grade::Mistake: return "?";
	case Grade::Blunder: return "??";
	default: return "";
	}
}

float BaselineOf(float Y, const TextStyle& S)
{
	switch (S.VAlign)
	{
	case Baseline::Middle: return Y + 0.29f * S.Size;
	case Baseline::Top: return Y + 0.79f * S.Size;
	case Baseline::Bottom: return Y - 0.21f * S.Size;
	default: return Y;
	}
}

std::string Lower(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return Out;
}

std::string Pad2(int V)
{
	return (V < 10 ? "0" : "") + std::to_string(V);
}

bool StartsWith(const std::string& S, const char* Prefix)
{
	return S.rfind(Prefix, 0) == 0;
}

Color NetChipFill(const Color& Cl)
{
	return {Cl.R, Cl.G, Cl.B, 0.12f};
}

/**
 * Who else's money a result was (next_segment_spec §2.12), for the results screen: a package's roll took the prize
 * ("Prize $410 went into Lantern Row Staking's roll."), or the buyers took their pieces ("Prize $410 · buyers' share
 * −$123.00 · yours $287.00"). "" when it was all the player's.
 */
std::string ResultSplit(const Session& S, const Results& R)
{
	if (R.PrizeCents <= 0 || S.History.empty() || S.History.front().Name != R.EventName)
	{
		return std::string();
	}
	const std::string& Id = S.History.front().EventId;
	const stake::Book& B = S.Life.Stakes;
	for (const stake::Fund& Fd : B.Funds)
	{
		if (Fd.EventId == Id && Fd.Settled)
		{
			for (const stake::Deal& D : B.Deals)
			{
				if (D.Id == Fd.DealId)
				{
					return "Prize " + Money(R.PrizeCents) + " went into " + stake::BackerName(D) + "'s roll. Your cut comes at the chop.";
				}
			}
		}
	}
	Chips Theirs = 0;
	for (const stake::Piece& P : B.Pieces)
	{
		Theirs += P.EntryId == Id && P.Settled ? P.Share : 0;
	}
	if (Theirs <= 0)
	{
		return std::string();
	}
	return "Prize " + Money(R.PrizeCents) + " \xC2\xB7 buyers' share \xE2\x88\x92" + Money(Theirs) + " \xC2\xB7 yours " + Money(R.PrizeCents - Theirs);
}

/** The player's sizing spot in the hand being played (core's sizing, as the card room's); online sizes aren't rounded. */
sizing::Spot OnlineSpot(const Session& S)
{
	sizing::Spot Spot;
	if (S.CurHand)
	{
		for (const HandSeat& Hs : S.CurHand->Seats)
		{
			if (Hs.Id == HeroId)
			{
				Spot = sizing::SpotOf(*S.CurHand, Hs.Seat);
				break;
			}
		}
	}
	Spot.Unit = 1;
	return Spot;
}

/** A badge being celebrated on a laptop (C10: Session::Unseen, taken once a frame) and when its toast came up. */
struct BadgeCheer
{
	badge::Badge B;
	double At = 0.0;
};
constexpr double BadgeToastSeconds = 6.0;

std::map<const RiverLine*, std::vector<BadgeCheer>>& BadgeCheers()
{
	static std::map<const RiverLine*, std::vector<BadgeCheer>> ByLaptop;
	return ByLaptop;
}

/**
 * The badge toasts (§5.4 C10, §5.7): top right under the closed tables' toasts, a glyph at 32, BADGE EARNED and the
 * badge's title, six seconds each, at most three at once.
 */
void BadgeToasts(Ui& UI, Canvas& C, const Session& S, std::vector<BadgeCheer>& List, bool Lobby, double Now)
{
	List.erase(std::remove_if(List.begin(), List.end(), [Now](const BadgeCheer& Ch) { return Now - Ch.At > BadgeToastSeconds; }), List.end());
	// Below FinishedToasts' lane (RiverLine's own app only), as far down as it reaches this frame.
	float Y = 72.0f;
	for (const FinishedTable& F : S.Finished)
	{
		const double Age = Now - F.At;
		if (Lobby && Age >= 0.0 && Age <= 9.0)
		{
			Y += (78.0f + 10.0f) * static_cast<float>(Clamp01((9.0 - Age) / 0.6));
		}
	}
	for (size_t I = 0; I < List.size() && I < 3; ++I)
	{
		const double Age = Now - List[I].At;
		if (Age < 0.0)
		{
			continue;
		}
		const float In = rlnet_detail::NetEase(Age / 0.35);
		const float Out = static_cast<float>(Clamp01((BadgeToastSeconds - Age) / 0.6));
		const float W = 384.0f;
		const float H = 64.0f;
		const Rect R{RlW - W - 16.0f + (1.0f - In) * 60.0f, Y, W, H};
		const Color Col = pal::Gold;
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * In * Out);
		C.GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, 12.0f, Rgba(0, 0, 0, 0.5f), 16.0f);
		UI.RRect(R, 12.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Mix(rlsites_detail::SitesTone(0x0f1a2b), Col, 0.12f), rlsites_detail::SitesTone(0x0d1626)), rlnet_detail::NetA(Col, 0.55f));
		UI.RRect({R.X, R.Y, 5.0f, R.H}, 2.5f, Col);
		badgeart::Draw(C, List[I].B, R.X + 38.0f, R.Y + H / 2.0f, 32.0f, Now);
		rlnet_detail::NetSpaced(C, "BADGE EARNED", R.X + 68.0f, R.Y + 24.0f, 10.0f, 800, pal::Muted, 1.2f);
		UI.Text(badge::Title(List[I].B), R.X + 68.0f, R.Y + 47.0f, Ts(17.0f, 800, Col, Align::Left, Baseline::Alphabetic, false, R.W - 86.0f));
		C.SetAlpha(A0);
		Y += (H + 10.0f) * Out;
	}
}

// ---- Update 3, G1 (ui-tablestyle; §2.5, §3.5, §13): the rivals' felts, rails and seats through SiteLook. RiverLine's oval
// keeps today's code path exactly; these draw only for a look with a seat list (Ignite's capsule, RunItTwice's octagon).
using TblLook = rlsites_detail::SiteLook;

Color TblA(uint32_t Rgb, float A)
{
	Color Cl = Hex(Rgb);
	Cl.A = A;
	return Cl;
}

/** The felt's rect inset by D units (outward when D < 0). */
Rect TblFeltRect(const TblLook& L, float D)
{
	return {L.Cx - L.Rx + D, L.Cy - L.Ry + D, 2.0f * (L.Rx - D), 2.0f * (L.Ry - D)};
}

/** The octagon inset by D (the chamfer's run shrinking as it goes in), clockwise from the top edge's left end. */
std::vector<Vec2> TblOctagon(const TblLook& L, float D)
{
	const Rect R = TblFeltRect(L, D);
	const float K = std::max(0.0f, L.Chamfer - D * (2.0f - std::sqrt(2.0f)));
	return {{R.X + K, R.Y}, {R.X + R.W - K, R.Y}, {R.X + R.W, R.Y + K}, {R.X + R.W, R.Y + R.H - K}, {R.X + R.W - K, R.Y + R.H}, {R.X + K, R.Y + R.H}, {R.X, R.Y + R.H - K}, {R.X, R.Y + K}};
}

std::vector<Vec2> TblClosed(std::vector<Vec2> P)
{
	if (!P.empty())
	{
		P.push_back(P.front());
	}
	return P;
}

/** The felt's centre column for a rival look: the pot's chips, beside the pot pill under the board (§2.5, §3.5). */
Vec2 TblPotSpot(const TblLook& L)
{
	return {L.Cx - 118.0f, L.Cy + 56.0f};
}

/** A look's bet spot: 0.56 of the way in from the seat to the felt's centre (§2.5, §3.5). */
Vec2 TblBetSpot(const TblLook& L, Vec2 P)
{
	return {P.X + (L.Cx - P.X) * 0.56f, P.Y + (L.Cy - P.Y) * 0.56f};
}

/** A look's dealer button: a third of the way in from the seat, 40 units to the seat's left as it faces the felt. */
Vec2 TblDealerSpot(const TblLook& L, Vec2 P)
{
	const float Dx = L.Cx - P.X;
	const float Dy = L.Cy - P.Y;
	const float Len = std::max(1.0f, std::sqrt(Dx * Dx + Dy * Dy));
	return {P.X + Dx * 0.35f + Dy / Len * 40.0f, P.Y + Dy * 0.35f - Dx / Len * 40.0f};
}

/** The chips in an action line ("Raise 1,200", "Call 2.45M"): 0 when it names none. */
Chips TblActionChips(const std::string& Tag)
{
	double V = 0.0;
	double Frac = 0.0;
	bool Digits = false;
	for (const char Ch : Tag)
	{
		if (Ch >= '0' && Ch <= '9')
		{
			Digits = true;
			if (Frac > 0.0)
			{
				V += (Ch - '0') * Frac;
				Frac /= 10.0;
			}
			else
			{
				V = V * 10.0 + (Ch - '0');
			}
		}
		else if (Ch == '.' && Digits)
		{
			Frac = 0.1;
		}
		else if (Ch == 'M' && Digits)
		{
			V *= 1000000.0;
		}
	}
	return static_cast<Chips>(JsRound(V));
}

/**
 * Ignite's capsule in its neon tube (§2.5): a Night rail, a coral-to-pink band with its glow and a cream highlight on the
 * inner edge (breathing on the beat at a final table), the Plum Velvet felt with its radial highlight and a faint scatter of
 * sparks (cached by look and skin). No betting line: §2.5 has none, and its dashes cost the table 1.8k vertices (§10).
 */
void TblCapsule(Canvas& C, const TblLook& L, double Now, bool Still, bool FinalTable, double Bpm)
{
	const Rect F = TblFeltRect(L, 0.0f);
	const float Rr = L.Chamfer;
	const auto Grow = [&F](float D) { return Rect{F.X - D, F.Y - D, F.W + 2.0f * D, F.H + 2.0f * D}; };
	const Rect Shade = Grow(24.0f);
	C.FillRoundRect({Shade.X, Shade.Y + 6.0f, Shade.W, Shade.H}, Rr + 24.0f, Rgba(0, 0, 0, 0.45f));
	C.FillRoundRect(Shade, Rr + 24.0f, Hex(rlsites_detail::SitesIgNight));
	// The beat breathes the tube at a final table (Bpm: Ignite's, 96 in Afterburner Hours and 124 the rest of the day, SPEC
	// §2.3, G1_AUDIT L1); a still while previewing.
	const float Breath = FinalTable && !Still ? 0.7f + 0.3f * static_cast<float>(0.5 + 0.5 * std::sin(Now * 2.0 * 3.14159265 * Bpm / 60.0)) : 1.0f;
	C.GlowRoundRect(Grow(12.0f), Rr + 12.0f, TblA(rlsites_detail::SitesIgPink, 0.35f * Breath), 16.0f);
	C.FillRoundRect(Grow(17.0f), Rr + 17.0f, Paint::Linear({F.X, 0.0f}, {F.X + F.W, 0.0f}, Hex(rlsites_detail::SitesIgCoral), Hex(rlsites_detail::SitesIgPink)));
	C.FillRoundRect(Grow(7.0f), Rr + 7.0f, Hex(rlsites_detail::SitesIgNight));
	C.StrokeRoundRect(Grow(6.5f), Rr + 6.5f, TblA(rlsites_detail::SitesIgCream, 0.55f), 2.0f);
	// The felt and its highlight: a soft lighter pool (a glow's falloff: a fifth of a radial's vertices, §10).
	C.FillRoundRect(F, Rr, Hex(L.Felt));
	C.GlowRoundRect({L.Cx - L.Rx * 0.62f, L.Cy - L.Ry * 0.45f - 12.0f, L.Rx * 1.24f, L.Ry * 0.9f}, L.Ry * 0.45f, TblA(L.FeltLight, 0.5f), 80.0f);
	// Forty sparks at 0.06, scattered by a fixed sequence (no draw).
	const uint64_t Key = L.Key ^ 0x5ca77e125ca77e12ull ^ Skin::Current().Key();
	C.Cached(Key, L.Cx, L.Cy, [&L](Canvas& Cv) {
		uint32_t H = 2166136261u;
		int Drawn = 0;
		for (int K = 0; K < 400 && Drawn < 40; ++K)
		{
			H = (H ^ static_cast<uint32_t>(K)) * 16777619u;
			const float U = static_cast<float>(H & 0xffff) / 65535.0f * 2.0f - 1.0f;
			const float V = static_cast<float>((H >> 16) & 0xffff) / 65535.0f * 2.0f - 1.0f;
			if (U * U + V * V > 0.86f)
			{
				continue;
			}
			rlsites_detail::SitesSpark(Cv, U * (L.Rx - 40.0f), V * (L.Ry - 30.0f), 2.5f + static_cast<float>((H >> 8) % 4), TblA(rlsites_detail::SitesIgCream, 0.06f));
			++Drawn;
		}
	});
}

/**
 * RunItTwice's octagon, ringed in red light (§3.5, v1.2): the room's faint red spill, a gloss-black rail 22 units out with a
 * chrome hairline and a lacquer highlight, the black speed cloth with its downlight pool, the twin light (a 2-unit Signal Red
 * line with an LED core, a 3-unit gap, a 1-unit line) under a halo that spills onto the cloth, pulsing on the hero's side on
 * the player's turn and breathing at a final table; a dashed chrome betting line; the turnstone at 0.06 above the board.
 */
void TblOctagonTable(Canvas& C, const TblLook& L, double Now, bool Still, bool HeroTurn, bool FinalTable)
{
	C.FillEllipse(L.Cx, L.Cy, 560.0f, 320.0f, Paint::Radial({L.Cx, L.Cy}, 0.0f, {L.Cx, L.Cy}, 560.0f, TblA(rlsites_detail::SitesRitRed, 0.07f), 0.6f, TblA(rlsites_detail::SitesRitRed, 0.035f), TblA(rlsites_detail::SitesRitRed, 0.0f)));
	const std::vector<Vec2> Rail = TblOctagon(L, -22.0f);
	std::vector<Vec2> Drop = Rail;
	for (Vec2& P : Drop)
	{
		P.Y += 8.0f;
	}
	C.GlowPolygon(Drop, Rgba(0, 0, 0, 0.6f), 10.0f);
	const Rect Rr = TblFeltRect(L, -22.0f);
	C.FillPolygon(Rail, Paint::Linear({0.0f, Rr.Y}, {0.0f, Rr.Y + Rr.H}, Hex(rlsites_detail::SitesRitGloss), Hex(0x0c0c0f)));
	C.StrokePolyline(Rail, true, TblA(rlsites_detail::SitesRitChrome, 0.55f), 1.0f);
	{
		const std::vector<Vec2> Hi = TblOctagon(L, -19.5f);
		C.StrokePolyline({Hi[7], Hi[0], Hi[1], Hi[2]}, false, Rgba(255, 255, 255, 0.06f), 3.0f);
	}
	// The cloth: its dark edge, then the pool (an elliptical radial that reaches the edge's colour at its rim).
	C.FillPolygon(TblOctagon(L, 0.0f), Hex(L.FeltDark));
	C.Save();
	C.Translate(L.Cx, L.Cy - 10.0f);
	C.Scale(1.0f, L.Ry / L.Rx);
	C.FillCircle(0.0f, 0.0f, L.Rx, Paint::Radial({0.0f, -30.0f}, 0.0f, {0.0f, 0.0f}, L.Rx, Hex(L.FeltLight), 0.55f, Hex(L.Felt), Hex(L.FeltDark)));
	C.Restore();
	// The light: a halo spilling onto the cloth, then the twin light.
	const float Breath = FinalTable && !Still ? 0.8f + 0.2f * static_cast<float>(std::sin(Now * 2.0 * 3.14159265 * 0.25)) : 1.0f;
	const std::vector<Vec2> Halo = TblOctagon(L, 4.0f);
	C.StrokePolyline(Halo, true, TblA(rlsites_detail::SitesRitRed, 0.05f * Breath), 26.0f);
	C.StrokePolyline(Halo, true, TblA(rlsites_detail::SitesRitRed, 0.08f * Breath), 16.0f);
	C.StrokePolyline(Halo, true, TblA(rlsites_detail::SitesRitRed, 0.13f * Breath), 8.0f);
	const std::vector<Vec2> Light = TblOctagon(L, 1.0f);
	C.StrokePolyline(Light, true, TblA(rlsites_detail::SitesRitRed, Breath), 2.0f);
	C.StrokePolyline(Light, true, TblA(rlsites_detail::SitesRitLed, Breath), 1.0f);
	C.StrokePolyline(TblOctagon(L, 5.5f), true, TblA(rlsites_detail::SitesRitRed, Breath), 1.0f);
	if (HeroTurn)
	{
		// The hero's side pulses 0.6 to 1.0 at 1 Hz on the player's turn (held at full while previewing).
		const float Pulse = Still ? 1.0f : 0.6f + 0.4f * static_cast<float>(0.5 + 0.5 * std::sin(Now * 2.0 * 3.14159265));
		const Vec2 A = Light[5];
		const Vec2 B = Light[4];
		C.GlowPolygon({{A.X, A.Y - 9.0f}, {B.X, B.Y - 9.0f}, {B.X, B.Y + 3.0f}, {A.X, A.Y + 3.0f}}, TblA(rlsites_detail::SitesRitRed, 0.45f * Pulse), 10.0f);
		C.StrokePolyline({A, B}, false, TblA(rlsites_detail::SitesRitLed, Pulse), 2.0f);
	}
	C.StrokeDashed(TblClosed(TblOctagon(L, 56.0f)), TblA(rlsites_detail::SitesRitChrome, 0.22f), 1.0f, 6.0f, 6.0f);
	rlsites_detail::SitesTurnstone(C, L.Cx - 30.0f, L.Cy - 198.0f, 60.0f, Rgba(255, 255, 255, 0.06f), Rgba(255, 255, 255, 0.06f), Rgba(12, 12, 15, 0.06f));
}

bool TblOverlaps(const Rect& A, const Rect& B)
{
	return A.X < B.X + B.W && B.X < A.X + A.W && A.Y < B.Y + B.H && B.Y < A.Y + A.H;
}

/**
 * A listed seat's hole cards (§2.5, §3.5): the hero's big ones up and to the right of Ignite's bubble, or right-aligned over
 * RunItTwice's data card; everyone else's small at the seat's top right (faces at a showdown). While RunItTwice's lower third
 * is up (Band), no card is drawn inside it: the band shows the all-in hands itself.
 */
void TblHoleCards(Canvas& C, const TblLook& L, const SeatVis& Seat, Vec2 P, const Rect& Box, bool Band, double Now, const std::vector<int>& Fading, double FadeAt)
{
	const bool Bubble = L.Seat == rlsites_detail::SeatStyle::Bubble;
	const bool HeroBig = Seat.IsHero && (Seat.Hole.size() == 2 || !Fading.empty());
	if (HeroBig)
	{
		const float Cw = Bubble ? 64.0f : 50.0f;
		const float Ch = Bubble ? 90.0f : 70.0f;
		const float X0 = Bubble ? P.X + 44.0f : Box.X + Box.W - 2.0f * Cw - 4.0f;
		const float Y0 = Bubble ? P.Y - 126.0f : Box.Y - Ch + 12.0f;
		if (Band && TblOverlaps({X0, Y0 - 4.0f, 2.0f * Cw + 4.0f, Ch + 4.0f}, L.LowerThird))
		{
			return;
		}
		const bool Live = Seat.HasCards && !Seat.Folded && Seat.Hole.size() == 2;
		const std::vector<int> Cards = Live ? std::vector<int>{Seat.Hole[0], Seat.Hole[1]} : Fading;
		const double Dealt = Now - Seat.DealtAt;
		for (size_t I = 0; I < Cards.size() && I < 2; ++I)
		{
			CardOpts O;
			if (Live)
			{
				O.Flip = static_cast<float>(Clamp01((Dealt - 0.35 - static_cast<double>(I) * 0.1) / 0.25));
				O.Highlight = Seat.Winner;
			}
			else
			{
				O.Alpha = static_cast<float>(1.0 - (Now - FadeAt) / 1.5);
				O.Dim = true;
			}
			O.Rotate = Bubble ? (static_cast<float>(I) - 0.5f) * 0.08f : 0.0f;
			O.Back = L.CardBack;
			const float X = Bubble ? X0 + static_cast<float>(I) * 40.0f : X0 + static_cast<float>(I) * (Cw + 4.0f);
			const float Y = Bubble ? Y0 - static_cast<float>(I) * 4.0f : Y0;
			DrawCard(C, Cards[I], X, Y, Cw, Ch, O);
		}
		return;
	}
	if (!Seat.HasCards || Seat.Folded || Seat.IsHero)
	{
		return;
	}
	const bool Faces = Seat.Hole.size() == 2;
	if (!Faces && Now - Seat.DealtAt <= 0.4)
	{
		return;
	}
	const float Cw = Faces ? 40.0f : 30.0f;
	const float Ch = Faces ? 56.0f : 42.0f;
	const float Step = Faces ? Cw + 4.0f : 16.0f;
	const float X0 = Bubble ? P.X + 8.0f : Box.X + Box.W - Cw - Step - 8.0f;
	const float Y0 = Bubble ? P.Y - 32.0f - Ch + 12.0f : Box.Y - Ch + 12.0f;
	if (Band && TblOverlaps({X0, Y0, Cw + Step, Ch}, L.LowerThird))
	{
		return;
	}
	for (int I = 0; I < 2; ++I)
	{
		CardOpts O;
		O.Rotate = Faces ? 0.0f : (static_cast<float>(I) - 0.5f) * 0.15f;
		O.Highlight = Seat.Winner;
		O.Back = L.CardBack;
		DrawCard(C, Faces ? Seat.Hole[static_cast<size_t>(I)] : -1, X0 + static_cast<float>(I) * Step, Y0, Cw, Ch, O);
	}
}

/** Ignite's action sticker (§2.5): a 16-point starburst with a cream outline over the bubble, the verb and the amount. */
void TblSticker(Canvas& C, float Cx, float Cy, float R, const Color& Fill, const std::string& Top, const std::string& Low)
{
	const auto Burst = [&](float Rr, const Color& Col, float Dy) {
		std::vector<Vec2> Pts;
		for (int K = 0; K < 32; ++K)
		{
			const float A = static_cast<float>(K) * Pi / 16.0f;
			const float Rk = K % 2 == 0 ? Rr : Rr * 0.82f;
			Pts.push_back({Cx + Rk * std::cos(A), Cy + Dy + Rk * std::sin(A)});
		}
		C.FillPolygon(Pts, Col);
	};
	Burst(R + 3.0f, Rgba(0, 0, 0, 0.35f), 2.0f);
	Burst(R + 3.0f, Hex(rlsites_detail::SitesIgCream), 0.0f);
	Burst(R, Fill, 0.0f);
	TextStyle St = Ts(Low.empty() ? R * 0.46f : R * 0.42f, 900, Hex(rlsites_detail::SitesIgPlumInk), Align::Center, Baseline::Middle);
	St.Face = static_cast<int8_t>(Font::BlackItalic);
	if (Low.empty())
	{
		C.Text(Top, Cx, Cy, St);
		return;
	}
	C.Text(Top, Cx, Cy - R * 0.22f, St);
	St.Size = R * 0.36f;
	C.Text(Low, Cx, Cy + R * 0.26f, St);
}

/** A RunItTwice tag: a cut slab with BoldCondensed caps (Hot White on Signal Red reads 5.45:1). Returns its width. */
float TblTag(Canvas& C, float X, float Y, const std::string& Text, const Color& Fill, const Color& Ink, float Size, float H)
{
	const float W = C.Measure(Text, Size, Font::BoldCondensed) + 12.0f;
	C.FillPolygon(rlsites_detail::SitesCutPath({X, Y, W, H}, 4.0f), Fill);
	TextStyle St = Ts(Size, 700, Ink, Align::Center, Baseline::Middle);
	St.Face = static_cast<int8_t>(Font::BoldCondensed);
	C.Text(Text, X + W / 2.0f, Y + H / 2.0f, St);
	return W;
}

/** The action bar's roles in a rival's look (§2.5, §3.5): FOLD, CALL, RAISE and ALL IN each have their own fill. */
enum class TblRole : uint8_t
{
	Fold,
	Call,
	Raise,
	AllIn,
};

/**
 * A rival look's action button (§2.5 Action bar, §3.5 Action bar): Ignite's 72-tall pills (FOLD grape, CALL sky, RAISE a
 * sunset, ALL IN sunflower; plum ink), RunItTwice's 60-tall slabs cut 8 (FOLD Carbon with Chrome type, CALL the chrome ramp
 * with Onyx, RAISE and ALL IN Signal Red with Hot White and an LED top edge), labels in the look's face. The press is
 * Ui::Clickable's under the same id, so the decisions stay Controls()'s.
 */
bool TblActionButton(Ui& UI, Canvas& C, const TblLook& L, const std::string& Id, const Rect& R, const std::string& Label, const std::string& Sub, const std::string& Hotkey, TblRole Role)
{
	const Ui::ClickState St = UI.Clickable(Id, R);
	const float Shift = St.Down ? 2.0f : 0.0f;
	const Rect Body{R.X, R.Y + Shift, R.W, R.H};
	const bool Slab = L.Button == rlsites_detail::ButtonShape::Slab;
	Color Ink = Hex(rlsites_detail::SitesIgPlumInk);
	Paint Fill(Hex(0x3a1f5c));
	if (Slab)
	{
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(Body, std::min(R.H / 2.0f, L.Cut * 0.8f));
		if (Role == TblRole::Raise || Role == TblRole::AllIn)
		{
			C.GlowRoundRect({Body.X, Body.Y + 8.0f, Body.W, Body.H - 8.0f}, 0.0f, TblA(rlsites_detail::SitesRitRed, St.Hover ? 0.45f : 0.3f), 12.0f);
		}
		Fill = Role == TblRole::Fold ? Paint(pal::Panel)
			: Role == TblRole::Call  ? Paint::Linear({0.0f, Body.Y}, {0.0f, Body.Y + Body.H}, Hex(0x8d929c), Hex(0xaab0ba))
									 : Paint(St.Hover ? Hex(0xd62c31) : pal::Accent);
		C.FillPolygon(Path, Fill);
		if (Role == TblRole::Fold)
		{
			C.StrokePolyline(Path, true, pal::Line, 1.0f);
		}
		if (Role == TblRole::Raise || Role == TblRole::AllIn)
		{
			C.FillRect({Body.X, Body.Y, Body.W - std::min(R.H / 2.0f, L.Cut * 0.8f), 1.0f}, Hex(rlsites_detail::SitesRitLed));
		}
		Ink = Role == TblRole::Fold ? pal::Accent2 : Role == TblRole::Call ? Hex(rlsites_detail::SitesRitOnyx) : pal::Gold;
	}
	else
	{
		const float Rr = R.H / 2.0f;
		switch (Role)
		{
		case TblRole::Fold:
			Fill = Paint(St.Hover ? Hex(0x4a2a72) : Hex(0x3a1f5c));
			Ink = Hex(rlsites_detail::SitesIgCream);
			break;
		case TblRole::Call: Fill = Paint(St.Hover ? Hex(0x7dd3fa) : Hex(rlsites_detail::SitesIgSky)); Ink = Hex(0x0b2a3a); break;
		case TblRole::Raise: Fill = Paint::Linear({Body.X, 0.0f}, {Body.X + Body.W, 0.0f}, Hex(rlsites_detail::SitesIgCoral), Hex(rlsites_detail::SitesIgPink)); break;
		case TblRole::AllIn: Fill = Paint(Hex(rlsites_detail::SitesIgSun)); break;
		}
		if (Role == TblRole::Raise || Role == TblRole::AllIn)
		{
			C.GlowRoundRect({Body.X + 10.0f, Body.Y + Body.H - 14.0f, Body.W - 20.0f, 16.0f}, 8.0f, TblA(rlsites_detail::SitesIgPink, 0.45f), 12.0f);
		}
		C.FillRoundRect(Body, Rr, Fill);
		if (Role == TblRole::Fold)
		{
			C.StrokeRoundRect(Body, Rr, Hex(0x6a4a9a), 2.0f);
		}
		C.FillRoundRect({Body.X + 10.0f, Body.Y + 4.0f, Body.W - 20.0f, 10.0f}, 5.0f, Rgba(255, 255, 255, Role == TblRole::Fold ? 0.06f : 0.22f));
	}
	TextStyle Big = Ts(Slab ? 20.0f : 22.0f, 900, Ink, Align::Center, Sub.empty() ? Baseline::Middle : Baseline::Alphabetic);
	Big.Face = L.TitleFace;
	const std::string Caption = L.LabelCaps || !Slab ? rlnet_detail::NetUpper(Label) : Label;
	const float Cy = Body.Y + Body.H / 2.0f;
	if (Sub.empty())
	{
		C.Text(Caption, Body.X + Body.W / 2.0f, Cy + 1.0f, Big);
	}
	else
	{
		C.Text(Caption, Body.X + Body.W / 2.0f, Cy - 3.0f, Big);
		TextStyle Amount = Big;
		Amount.Size = Slab ? 15.0f : 16.0f;
		C.Text(Sub, Body.X + Body.W / 2.0f, Cy + 18.0f, Amount);
	}
	if (!Hotkey.empty())
	{
		C.Text(Hotkey, Body.X + Body.W - 14.0f, Body.Y + 16.0f, Ts(12.0f, 700, Ink, Align::Right, Baseline::Middle, true));
	}
	return St.Clicked;
}

/** "36.5 bb": Ignite's stacks, in big blinds, lower case (§2.5). */
std::string TblBbLower(Chips Amount, Chips BigBlind)
{
	const double Bb = static_cast<double>(Amount) / static_cast<double>(std::max<Chips>(1, BigBlind));
	return Fixed(Bb, Bb < 100.0 ? 1 : 0) + " bb";
}

// ---- G1 fix (fix-tablestyle; g1-audit M2 and M1; §2.5, §3.5, §9.6 "not a reskin: share no chrome"): the table's left-hand
// controls and its side panel in each rival's own language. RiverLine's look never reaches anything below: its code paths in
// Controls and SidePanel are as they were, so its draw lists don't move.

/** A label in a rival look's own face: RunItTwice's BoldCondensed, Ignite's BlackItalic (the look's title face). */
TextStyle TblFaced(const TblLook& L, float Size, const Color& Col, Align A = Align::Left, Baseline B = Baseline::Alphabetic)
{
	TextStyle St = Ts(Size, 800, Col, A, B);
	St.Face = L.TitleFace;
	return St;
}

/** Text in a face, its size brought down until it fits MaxW (never truncated: these are short labels). */
void TblFitText(Canvas& C, const std::string& Text, float X, float Y, TextStyle St, float MaxW)
{
	const float W = C.Measure(Text, St.Size, static_cast<Font>(St.Face));
	if (W > MaxW && W > 0.0f)
	{
		St.Size *= MaxW / W;
	}
	C.Text(Text, X, Y, St);
}

/** Words wrapped to MaxW in a face (a word longer than the line stays whole). */
std::vector<std::string> TblWrap(const Canvas& C, const std::string& Text, float MaxW, float Size, Font Face)
{
	std::vector<std::string> Lines;
	std::string Line;
	size_t At = 0;
	while (At <= Text.size())
	{
		const size_t Sp = Text.find(' ', At);
		const std::string Word = Text.substr(At, Sp == std::string::npos ? std::string::npos : Sp - At);
		const std::string Try = Line.empty() ? Word : Line + " " + Word;
		if (!Line.empty() && C.Measure(Try, Size, Face) > MaxW)
		{
			Lines.push_back(Line);
			Line = Word;
		}
		else
		{
			Line = Try;
		}
		if (Sp == std::string::npos)
		{
			break;
		}
		At = Sp + 1;
	}
	if (!Line.empty())
	{
		Lines.push_back(Line);
	}
	return Lines;
}

/**
 * The Composure meter in a rival look (g1-audit M2): RunItTwice's terminal bar, 20 square cells lit in Signal Up, Caution Amber
 * or (an alert that needs you) Signal Red; Ignite's candy pill on Night, a lime, sunflower or red-to-pink fill with a gloss and
 * a cream spark at its head. W wide, 10 tall, its top-left at (X, Y).
 */
void TblComposure(Canvas& C, const TblLook& L, float Comp, float X, float Y, float W)
{
	Comp = std::min(1.0f, std::max(0.0f, Comp));
	if (L.Cut > 0.0f)
	{
		const int Cells = 20;
		const float Cw = (W - static_cast<float>(Cells - 1)) / static_cast<float>(Cells);
		const int OnCells = static_cast<int>(JsRound(static_cast<double>(Comp) * Cells));
		const Color Lit = Comp > 0.6f ? pal::Green : Comp > 0.35f ? pal::Orange : pal::Accent;
		for (int I = 0; I < Cells; ++I)
		{
			C.FillRect({X + static_cast<float>(I) * (Cw + 1.0f), Y, Cw, 10.0f}, I < OnCells ? Lit : pal::Panel2);
		}
		return;
	}
	C.FillRoundRect({X, Y, W, 10.0f}, 5.0f, Hex(rlsites_detail::SitesIgNight));
	C.StrokeRoundRect({X, Y, W, 10.0f}, 5.0f, pal::Line, 1.0f);
	const float Fw = std::max(10.0f, W * Comp);
	const Paint Fill = Comp > 0.6f ? Paint::Linear({X, 0.0f}, {X + W, 0.0f}, Hex(rlsites_detail::SitesIgLime), pal::Green)
		: Comp > 0.35f			   ? Paint::Linear({X, 0.0f}, {X + W, 0.0f}, Hex(rlsites_detail::SitesIgSun), Hex(rlsites_detail::SitesIgTang))
								   : Paint::Linear({X, 0.0f}, {X + W, 0.0f}, Hex(rlsites_detail::SitesIgRed), Hex(rlsites_detail::SitesIgPink));
	C.FillRoundRect({X, Y, Fw, 10.0f}, 5.0f, Fill);
	C.FillRoundRect({X + 4.0f, Y + 2.0f, std::max(2.0f, Fw - 8.0f), 2.5f}, 1.25f, Rgba(255, 255, 255, 0.3f));
	rlsites_detail::SitesSpark(C, X + Fw - 3.0f, Y + 5.0f, 7.0f, Hex(rlsites_detail::SitesIgCream));
}

/**
 * The raise slider in a rival look (g1-audit M2): Ui::Slider's input (a press on it, a drag that holds while the button is
 * down, the wheel a step; Drag is the laptop's held state) under the same id, with the look's own track and knob. RunItTwice:
 * a hairline rail with quarter ticks, a Signal Red fill with an LED core and a cut chrome slab for the knob. Ignite: a candy
 * pill on Night, a coral-to-pink fill and a cream knob ringed in coral with a spark.
 */
double TblSlider(Ui& UI, Canvas& C, const TblLook& L, bool& Drag, const std::string& Id, const Rect& R, double Value, double Lo, double Hi, double Step)
{
	const Ui::ClickState St = UI.Clickable(Id, {R.X - 10.0f, R.Y - 12.0f, R.W + 20.0f, R.H + 24.0f});
	Drag = UI.Ptr.Down && (Drag || St.Down);
	double V = Value;
	if (Drag && Hi > Lo)
	{
		double Frac = (static_cast<double>(UI.Ptr.X) - R.X) / R.W;
		Frac = Frac < 0.0 ? 0.0 : Frac > 1.0 ? 1.0 : Frac;
		V = std::round((Lo + Frac * (Hi - Lo)) / Step) * Step;
	}
	if (St.Hover && UI.Ptr.Wheel != 0.0f)
	{
		V -= (UI.Ptr.Wheel > 0.0f ? 1.0 : -1.0) * Step;
	}
	V = V < Lo ? Lo : V > Hi ? Hi : V;
	const float Share = Hi > Lo ? static_cast<float>((V - Lo) / (Hi - Lo)) : 0.0f;
	const float Cy = R.Y + R.H / 2.0f;
	const float Kx = R.X + R.W * Share;
	if (L.Cut > 0.0f)
	{
		C.FillRect({R.X, Cy - 2.0f, R.W, 4.0f}, pal::Line);
		for (int Q = 1; Q < 4; ++Q)
		{
			C.FillRect({R.X + R.W * static_cast<float>(Q) / 4.0f - 0.5f, Cy - 6.0f, 1.0f, 12.0f}, TblA(rlsites_detail::SitesRitChrome, 0.35f));
		}
		C.FillRect({R.X, Cy - 2.0f, R.W * Share, 4.0f}, pal::Accent);
		C.FillRect({R.X, Cy - 0.5f, R.W * Share, 1.0f}, Hex(rlsites_detail::SitesRitLed));
		const Rect Knob{Kx - 7.0f, Cy - 12.0f, 14.0f, 24.0f};
		if (St.Hover || Drag)
		{
			C.GlowRoundRect(Knob, 0.0f, TblA(rlsites_detail::SitesRitRed, 0.4f), 8.0f);
		}
		C.FillPolygon(rlsites_detail::SitesCutPath(Knob, 4.0f), Paint::Linear({0.0f, Knob.Y}, {0.0f, Knob.Y + Knob.H}, Hex(0xf3f4f6), Hex(0x8d929c)));
		C.FillRect({Kx - 1.0f, Knob.Y + 5.0f, 2.0f, Knob.H - 10.0f}, pal::Accent);
		return V;
	}
	C.FillRoundRect({R.X, Cy - 5.0f, R.W, 10.0f}, 5.0f, Hex(rlsites_detail::SitesIgNight));
	C.StrokeRoundRect({R.X, Cy - 5.0f, R.W, 10.0f}, 5.0f, pal::Line, 1.0f);
	C.FillRoundRect({R.X, Cy - 5.0f, std::max(10.0f, R.W * Share), 10.0f}, 5.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(rlsites_detail::SitesIgCoral), Hex(rlsites_detail::SitesIgPink)));
	if (St.Hover || Drag)
	{
		C.GlowRoundRect({Kx - 13.0f, Cy - 13.0f, 26.0f, 26.0f}, 13.0f, TblA(rlsites_detail::SitesIgPink, 0.45f), 10.0f);
	}
	C.FillCircle(Kx, Cy + 2.0f, 13.0f, Rgba(0, 0, 0, 0.35f));
	C.FillCircle(Kx, Cy, 13.0f, Hex(rlsites_detail::SitesIgCream));
	C.StrokeArc(Kx, Cy, 11.5f, 0.0f, 2.0f * Pi, Hex(rlsites_detail::SitesIgCoral), 3.0f);
	rlsites_detail::SitesSpark(C, Kx, Cy, 6.0f, Hex(rlsites_detail::SitesIgCoral));
	return V;
}

/**
 * The session's own colours (the banner's gold, red, green and orange are RiverLine's literals, Session.cpp) as the skin's
 * tokens, so a rival's overlay never draws RiverLine's (g1-audit M2); any other literal through SitesTone.
 */
Color TblSkinned(uint32_t Rgb)
{
	switch (Rgb)
	{
	case 0xf2c14e: return pal::Gold;
	case 0xef4d5a: return pal::Red;
	case 0x3ecf6e: return pal::Green;
	case 0xf28a3a: return pal::Orange;
	case 0x27d3c3: return pal::Accent;
	case 0x3b82f6: return pal::Accent2;
	default: return rlsites_detail::SitesTone(Rgb);
	}
}

/**
 * A rival's card over the felt (the banner, table balancing; g1-audit M2): RunItTwice's Tape Black slab cut 10 with an Edge
 * hairline and the twin stripe down its left end; Ignite's Night card rounded 22 with an Edge rim and its glow.
 */
void TblOverlayCard(Canvas& C, const TblLook& L, const Rect& R, const Color& Edge)
{
	if (L.Cut > 0.0f)
	{
		const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(R, 10.0f);
		C.FillPolygon(Path, TblA(rlsites_detail::SitesRitTape, 0.94f));
		C.StrokePolyline(Path, true, Edge, 1.5f);
		C.FillRect({R.X + 14.0f, R.Y + 14.0f, 8.0f, R.H - 28.0f}, pal::Accent);
		C.FillRect({R.X + 26.0f, R.Y + 14.0f, 3.0f, R.H - 28.0f}, pal::Accent);
		return;
	}
	C.GlowRoundRect(R, 22.0f, Color{Edge.R, Edge.G, Edge.B, 0.3f}, 14.0f);
	C.FillRoundRect(R, 22.0f, TblA(rlsites_detail::SitesIgNight, 0.94f));
	C.StrokeRoundRect(R, 22.0f, Edge, 2.0f);
}

/** "TABLE 4 → TABLE 7" centred on Cx in a look's face, the arrow a vector mark (§3.3: no arrow glyph in any face). */
void TblArrowLine(Canvas& C, const TblLook& L, const std::string& From, const std::string& To, float Cx, float Y, float Size, const Color& Col)
{
	TextStyle St = TblFaced(L, Size, Col);
	const Font Face = static_cast<Font>(L.TitleFace);
	const float Lw = C.Measure(From, Size, Face);
	const float Rw = C.Measure(To, Size, Face);
	const float Aw = Size * 0.8f;
	const float Gap = Size * 0.4f;
	const float X0 = Cx - (Lw + Gap + Aw + Gap + Rw) / 2.0f;
	C.Text(From, X0, Y, St);
	const float Ay = Y - Size * 0.33f;
	const float Ax = X0 + Lw + Gap;
	const float Lwid = std::max(1.2f, Size * 0.09f);
	C.StrokePolyline({{Ax, Ay}, {Ax + Aw, Ay}}, false, Col, Lwid);
	C.StrokePolyline({{Ax + Aw - Size * 0.28f, Ay - Size * 0.24f}, {Ax + Aw, Ay}, {Ax + Aw - Size * 0.28f, Ay + Size * 0.24f}}, false, Col, Lwid);
	C.Text(To, Ax + Aw + Gap, Y, St);
}

/** The action area's lines in a rival look ("Waiting for …", the pot odds): RunItTwice's Mono, Ignite's Bold, both Muted. */
TextStyle TblReadout(const TblLook& L, Align A)
{
	return L.Cut > 0.0f ? Ts(13.0f, 500, pal::Muted, A, Baseline::Alphabetic, true) : Ts(14.0f, 700, pal::Muted, A);
}

/** The prize jump ahead of the hero: the next place up that pays more than finishing now (0 when none). */
int TblNextJump(const Tournament& T)
{
	const Chips Here = T.PrizeFor(T.Remaining);
	for (int P = T.Remaining - 1; P >= 1; --P)
	{
		if (T.PrizeFor(P) > Here)
		{
			return P;
		}
	}
	return 0;
}

/** "100/200 (25)": blinds and ante as the RunItTwice centre line and INFO print them. */
std::string TblBlinds(const Level& Lv)
{
	return ChipsText(static_cast<double>(Lv.Sb)) + "/" + ChipsText(static_cast<double>(Lv.Bb)) + (Lv.Ante > 0 ? " (" + ChipsText(static_cast<double>(Lv.Ante)) + ")" : std::string());
}

/**
 * RunItTwice's INFO tab (§3.5): the hero's place twice-printed, then level, blinds, next level, players, the average in BB, the
 * next pay jump, first and the ICM value (Structure's Icm() for the hero, walked over frames by the caller's cache) in Mono, and
 * the chip leaders with your row ruled in Hot White over the red underglow. Beyond Icm()'s exact range (more than two million
 * finishing orders: 10 left paying 9, 12 paying 7, any field before its last tables) the engine's figure is the chip share of
 * the prizes left, and the row is labelled CHIP SHARE, in Ink (g1-audit N7); a dash until the first value is known.
 */
void TblRitInfo(Canvas& C, const Tournament& T, const Rect& Box, const rlsites_detail::SitesIcmRead& Icm, bool InBb, bool LevelFlash, double LevelLeft)
{
	const float L = Box.X;
	const float R = Box.X + Box.W;
	const Level& Lv = T.CurrentLevel();
	const Chips Bb = std::max<Chips>(1, Lv.Bb);
	const TPlayer& Me = T.Hero();
	float Y = Box.Y + 8.0f;
	C.Text("YOUR PLACE", L, Y + 10.0f, TblFaced(rlsites_detail::SitesLook(net::SiteId::RunItTwice), 12.0f, pal::Muted));
	const std::string Place = Me.Busted ? std::string("OUT") : rlnet_detail::NetUpper(Ordinal(T.HeroRank()));
	TextStyle Big = Ts(48.0f, 700, pal::Accent);
	Big.Face = static_cast<int8_t>(Font::BoldCondensed);
	// The twice print (§3.3): a Signal Red under-copy at (+3, +3), then the readable copy in Ink.
	C.Text(Place, L + 3.0f, Y + 62.0f, Big);
	Big.Col = pal::Ink;
	C.Text(Place, L, Y + 59.0f, Big);
	const float Pw = C.Measure(Place, 48.0f, Font::BoldCondensed);
	TextStyle Of = Ts(20.0f, 700, pal::Ink);
	Of.Face = static_cast<int8_t>(Font::BoldCondensed);
	C.Text("OF " + Grouped(T.Remaining), L + Pw + 16.0f, Y + 36.0f, Of);
	C.Text(Me.Busted ? std::string("\xE2\x80\x94") : rlsites_detail::SitesBbText(Me.Stack, Bb) + " \xC2\xB7 " + ChipsText(static_cast<double>(Me.Stack)) + " chips", L + Pw + 16.0f, Y + 56.0f, Ts(12.0f, 500, pal::Muted));
	Y += 84.0f;
	C.FillRect({L, Y, Box.W, 1.0f}, pal::Line);
	Y += 24.0f;
	const int Secs = static_cast<int>(std::max(0.0, JsRound(LevelLeft)));
	const int Jump = TblNextJump(T);
	const double Avg = T.AverageStack();
	struct InfoRow
	{
		const char* K;
		std::string V;
		Color Col;
	};
	const InfoRow Rows[] = {
		{"Level", std::to_string(T.LevelIndex + 1) + " \xC2\xB7 " + TblBlinds(Lv), LevelFlash ? pal::Gold : pal::Ink},
		{"Next level", TblBlinds(T.NextLevel()) + " in " + std::to_string(Secs / 60) + ":" + Pad2(Secs % 60), pal::Ink},
		{"Players", Grouped(T.Remaining) + " of " + Grouped(T.Spec.Entrants) + " \xC2\xB7 " + std::to_string(T.PaidPlaces()) + " paid", pal::Ink},
		{"Average", (InBb ? rlsites_detail::SitesBbText(static_cast<Chips>(JsRound(Avg)), Bb) : ChipsText(JsRound(Avg))) + (Me.Busted ? std::string() : " \xC2\xB7 you " + Ordinal(T.HeroRank())), pal::Ink},
		{T.InTheMoney() ? "Next pay jump" : "To the money", T.InTheMoney() ? (Jump > 0 ? Ordinal(Jump) + " \xC2\xB7 " + Money(T.PrizeFor(Jump)) : std::string("\xE2\x80\x94")) : std::to_string(T.Remaining - T.PaidPlaces()) + (T.Remaining - T.PaidPlaces() == 1 ? " player" : " players") + " \xC2\xB7 min " + Money(T.Payouts.back()),
			T.InTheMoney() ? pal::Gold : T.HandForHand() ? pal::Orange : pal::Ink},
		{"First", Money(T.Payouts.front()), pal::Gold},
		{Icm.ChipShare ? "Chip share" : "ICM value", Me.Busted || !Icm.Known ? std::string("\xE2\x80\x94") : Money(static_cast<Chips>(JsRound(Icm.Cents))), Icm.ChipShare ? pal::Ink : pal::Gold},
	};
	const TblLook& Look = rlsites_detail::SitesLook(net::SiteId::RunItTwice);
	for (const InfoRow& Row : Rows)
	{
		C.Text(rlnet_detail::NetUpper(Row.K), L, Y, TblFaced(Look, 12.0f, pal::Muted));
		C.Text(Row.V, R, Y, Ts(13.0f, 500, Row.Col, Align::Right, Baseline::Alphabetic, true, Box.W - 110.0f));
		Y += 24.0f;
	}
	Y += 6.0f;
	C.FillRect({L, Y, Box.W, 1.0f}, pal::Line);
	Y += 26.0f;
	C.Text("CHIP LEADERS", L, Y, TblFaced(Look, 12.0f, pal::Muted));
	C.Text(InBb ? "BB" : "CHIPS", R, Y, TblFaced(Look, 12.0f, pal::Muted, Align::Right));
	Y += 10.0f;
	const std::vector<const TPlayer*> St = T.Standings();
	const int HeroRank = Me.Busted ? 0 : T.HeroRank();
	const auto LeaderRow = [&](int Rank, const TPlayer& Pl, float Ry, bool Zebra) {
		const bool You = Pl.IsHero;
		if (You)
		{
			C.GlowRoundRect({L, Ry + 18.0f, Box.W, 8.0f}, 0.0f, TblA(rlsites_detail::SitesRitRed, 0.35f), 4.0f);
		}
		C.FillRect({L, Ry, Box.W, 26.0f}, Zebra ? pal::Panel2 : pal::Panel);
		if (You)
		{
			C.FillRect({L, Ry, Box.W, 1.0f}, pal::Gold);
			C.FillRect({L, Ry + 25.0f, Box.W, 1.0f}, pal::Gold);
		}
		TextStyle Rk = Ts(15.0f, 700, You ? pal::Gold : pal::Ink, Align::Center);
		Rk.Face = static_cast<int8_t>(Font::BoldCondensed);
		C.Text(std::to_string(Rank), L + 16.0f, Ry + 19.0f, Rk);
		C.Text(Pl.Name, L + 36.0f, Ry + 18.0f, Ts(13.0f, 700, You ? pal::Gold : pal::Ink, Align::Left, Baseline::Alphabetic, false, Box.W - 140.0f));
		C.Text(InBb ? rlsites_detail::SitesBbText(Pl.Stack, Bb) : ChipsText(static_cast<double>(Pl.Stack)), R - 8.0f, Ry + 18.0f, Ts(13.0f, 500, pal::Ink, Align::Right, Baseline::Alphabetic, true));
	};
	const int Shown = static_cast<int>(std::min<size_t>(St.size(), 8));
	for (int I = 0; I < Shown && Y + 26.0f <= Box.Y + Box.H; ++I)
	{
		LeaderRow(I + 1, *St[static_cast<size_t>(I)], Y, I % 2 == 1);
		Y += 26.0f;
	}
	if (HeroRank > Shown && Y + 34.0f <= Box.Y + Box.H)
	{
		LeaderRow(HeroRank, Me, Y + 8.0f, false);
	}
}

/** RunItTwice's PAYOUTS tab (§3.5): places and prizes in Mono on zebra rows, the places reached in Ink and Hot White. */
void TblRitPayouts(Canvas& C, const Tournament& T, const Rect& Box)
{
	const TblLook& Look = rlsites_detail::SitesLook(net::SiteId::RunItTwice);
	const float L = Box.X;
	const float R = Box.X + Box.W;
	float Y = Box.Y + 18.0f;
	C.Text(std::to_string(T.PaidPlaces()) + " PAID \xC2\xB7 MIN CASH " + Money(T.Payouts.back()), L, Y, TblFaced(Look, 13.0f, pal::Ink));
	Y += 24.0f;
	C.Text("PLACE", L, Y, TblFaced(Look, 12.0f, pal::Muted));
	C.Text("PRIZE", R, Y, TblFaced(Look, 12.0f, pal::Muted, Align::Right));
	Y += 8.0f;
	C.FillRect({L, Y, Box.W, 1.0f}, pal::Line);
	Y += 4.0f;
	const int HeroRank = T.Hero().Busted ? 0 : T.HeroRank();
	int Row = 0;
	for (const std::pair<int, int>& Band : PayoutBands(static_cast<int>(T.Payouts.size())))
	{
		if (Y + 26.0f > Box.Y + Box.H)
		{
			break;
		}
		const bool Reached = T.Remaining <= Band.second;
		const bool You = HeroRank >= Band.first && HeroRank <= Band.second;
		if (You)
		{
			C.GlowRoundRect({L, Y + 18.0f, Box.W, 8.0f}, 0.0f, TblA(rlsites_detail::SitesRitRed, 0.35f), 4.0f);
		}
		C.FillRect({L, Y, Box.W, 26.0f}, Row % 2 == 1 ? pal::Panel2 : pal::Panel);
		if (You)
		{
			C.FillRect({L, Y, Box.W, 1.0f}, pal::Gold);
			C.FillRect({L, Y + 25.0f, Box.W, 1.0f}, pal::Gold);
		}
		const std::string Label = Band.first == Band.second ? Ordinal(Band.first) : Ordinal(Band.first) + "\xE2\x80\x93" + Ordinal(Band.second);
		C.Text(Label, L + 10.0f, Y + 18.0f, Ts(13.0f, 500, Reached || You ? pal::Ink : pal::Muted, Align::Left, Baseline::Alphabetic, true));
		C.Text(Money(T.Payouts[static_cast<size_t>(Band.first - 1)]), R - 10.0f, Y + 18.0f, Ts(13.0f, 500, Reached || You ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
		Y += 26.0f;
		++Row;
	}
}

/** RunItTwice's CHAT tab (§3.5): compact Mono, newest at the bottom; names in Chrome (yours Hot White, the rival's amber). */
void TblRitChat(Canvas& C, const std::vector<ChatLine>& Chat, const Rect& Box)
{
	const float Size = 12.0f;
	const float Lh = 17.0f;
	float Y = Box.Y + Box.H - 12.0f;
	for (size_t Ii = Chat.size(); Ii-- > 0 && Y > Box.Y + 8.0f;)
	{
		const ChatLine& Ln = Chat[Ii];
		const Color WhoCol = Ln.Kind == ChatKind::Hero ? pal::Gold : Ln.Kind == ChatKind::Rival ? pal::Orange : Ln.Kind == ChatKind::Dealer ? pal::Muted : pal::Accent2;
		const Color Col = Ln.Kind == ChatKind::Dealer ? pal::Muted : Ln.Kind == ChatKind::System ? pal::Accent2 : pal::Ink;
		const std::string Prefix = Ln.Who.empty() ? std::string() : Ln.Who + ": ";
		const std::vector<std::string> Wrapped = TblWrap(C, Prefix + Ln.Text, Box.W - 8.0f, Size, Font::Mono);
		for (size_t J = Wrapped.size(); J-- > 0 && Y > Box.Y + 8.0f;)
		{
			if (J == 0 && !Prefix.empty() && Wrapped[0].size() >= Prefix.size())
			{
				const float Pw = C.Text(Prefix, Box.X, Y, Ts(Size, 500, WhoCol, Align::Left, Baseline::Alphabetic, true));
				C.Text(Wrapped[0].substr(Prefix.size()), Box.X + Pw, Y, Ts(Size, 500, Col, Align::Left, Baseline::Alphabetic, true));
			}
			else
			{
				C.Text(Wrapped[J], Box.X, Y, Ts(Size, 500, Col, Align::Left, Baseline::Alphabetic, true));
			}
			Y -= Lh;
		}
		Y -= 4.0f;
	}
}

/**
 * Ignite's panel head (§2.5 in the Ignite look, the mockup's event header): a sunflower emblem, the event in BlackItalic, its
 * buy-in, guarantee and field; the level; "BLINDS UP IN m:ss" over a fuse burning towards the next level (its spark at the
 * level's elapsed share); and four stats (left, paid or the next jump, your rank, the average in bb).
 */
void TblIgHead(Canvas& C, const Tournament& T, const Rect& Head, bool LevelFlash, double LevelLeft)
{
	const float X = Head.X;
	const float W = Head.W;
	const float Y = Head.Y;
	const Level& Lv = T.CurrentLevel();
	const Chips Bb = std::max<Chips>(1, Lv.Bb);
	C.FillRoundRect(Head, 22.0f, Paint::Linear({X, Y}, {X + W, Y + Head.H}, TblA(rlsites_detail::SitesIgPink, 0.20f), TblA(rlsites_detail::SitesIgViolet, 0.10f)));
	C.FillCircle(X + 40.0f, Y + 42.0f, 22.0f, Hex(rlsites_detail::SitesIgSun));
	rlsites_detail::SitesSpark(C, X + 40.0f, Y + 42.0f, 13.0f, Hex(rlsites_detail::SitesIgCream), 0.26f);
	TextStyle Title = Ts(24.0f, 900, pal::Gold);
	Title.Face = static_cast<int8_t>(Font::BlackItalic);
	TblFitText(C, T.Spec.Name, X + 72.0f, Y + 44.0f, Title, W - 72.0f - 128.0f);
	// The level and blinds on the right, over the event's buy-in, guarantee and field (a line of its own, the panel's width).
	TextStyle Lvl = Ts(14.0f, 900, pal::Ink, Align::Right);
	Lvl.Face = static_cast<int8_t>(Font::BlackItalic);
	C.Text("LEVEL " + std::to_string(T.LevelIndex + 1), X + W - 20.0f, Y + 34.0f, Lvl);
	C.Text(ChipsText(static_cast<double>(Lv.Sb)) + "/" + ChipsText(static_cast<double>(Lv.Bb)) + " \xC2\xB7 ante " + ChipsText(static_cast<double>(Lv.Ante)), X + W - 20.0f, Y + 52.0f, Ts(12.0f, 700, pal::Muted, Align::Right));
	std::string Sub = Money(T.Spec.BuyInCents);
	Sub += T.Spec.GuaranteeCents > 0 ? " \xC2\xB7 GTD " + net::MoneyShort(T.Spec.GuaranteeCents) : std::string();
	Sub += " \xC2\xB7 " + Grouped(T.Spec.Entrants) + " entries";
	C.Text(Sub, X + 72.0f, Y + 70.0f, Ts(13.0f, 600, pal::Ink, Align::Left, Baseline::Alphabetic, false, W - 72.0f - 22.0f));
	// The blind clock as a fuse: burnt behind the spark, rope ahead of it.
	const int Secs = static_cast<int>(std::max(0.0, JsRound(LevelLeft)));
	const double LevelLong = std::max(1.0, T.Spec.LevelMinutes * 60.0);
	const float Burnt = static_cast<float>(std::min(1.0, std::max(0.0, 1.0 - LevelLeft / LevelLong)));
	const Level& Nx = T.NextLevel();
	C.Text(LevelFlash ? "BLINDS UP!" : "BLINDS UP IN " + std::to_string(Secs / 60) + ":" + Pad2(Secs % 60), X + 22.0f, Y + 92.0f, Ts(11.0f, 900, LevelFlash ? pal::Gold : pal::Orange));
	C.Text("next " + ChipsText(static_cast<double>(Nx.Sb)) + "/" + ChipsText(static_cast<double>(Nx.Bb)), X + W - 20.0f, Y + 92.0f, Ts(11.0f, 700, pal::Muted, Align::Right));
	const int Pts = 24;
	std::vector<Vec2> Rope;
	for (int I = 0; I <= Pts; ++I)
	{
		const float F = static_cast<float>(I) / static_cast<float>(Pts);
		Rope.push_back({X + 22.0f + F * (W - 44.0f), Y + 106.0f + 2.5f * std::sin(static_cast<float>(I) * 1.6f)});
	}
	const size_t Head0 = static_cast<size_t>(JsRound(static_cast<double>(Burnt) * Pts));
	const std::vector<Vec2> Gone(Rope.begin(), Rope.begin() + static_cast<std::ptrdiff_t>(Head0 + 1));
	const std::vector<Vec2> Ahead(Rope.begin() + static_cast<std::ptrdiff_t>(Head0), Rope.end());
	if (Gone.size() >= 2)
	{
		C.StrokePolyline(Gone, false, pal::Line, 3.0f);
	}
	if (Ahead.size() >= 2)
	{
		C.StrokePolyline(Ahead, false, Hex(rlsites_detail::SitesIgRope), 3.0f);
	}
	const Vec2 Sp = Rope[Head0];
	C.FillCircle(Sp.X, Sp.Y, 7.0f, TblA(rlsites_detail::SitesIgTang, 0.45f));
	rlsites_detail::SitesSpark(C, Sp.X, Sp.Y, 6.0f, Hex(rlsites_detail::SitesIgSun));
	// The four stats.
	const TPlayer& Me = T.Hero();
	const bool Itm = T.InTheMoney();
	const int Jump = TblNextJump(T);
	const std::pair<std::string, std::string> Stats[4] = {
		{Grouped(T.Remaining), "LEFT OF " + Grouped(T.Spec.Entrants)},
		{Itm ? (Jump > 0 ? Money(T.PrizeFor(Jump)) : std::string("\xE2\x80\x94")) : std::to_string(T.PaidPlaces()), Itm ? std::string("NEXT JUMP") : "PAID \xC2\xB7 MIN " + Money(T.Payouts.back())},
		{Me.Busted ? std::string("\xE2\x80\x94") : Ordinal(T.HeroRank()), "YOUR RANK"},
		{TblBbLower(static_cast<Chips>(JsRound(T.AverageStack())), Bb), "AVERAGE"},
	};
	const float StatW = (W - 44.0f) / 4.0f;
	for (int I = 0; I < 4; ++I)
	{
		const float Sx = X + 22.0f + static_cast<float>(I) * StatW;
		TextStyle V = Ts(16.0f, 900, pal::Ink);
		V.Face = static_cast<int8_t>(Font::BlackItalic);
		TblFitText(C, Stats[I].first, Sx, Y + 138.0f, V, StatW - 8.0f);
		C.Text(Stats[I].second, Sx, Y + 152.0f, Ts(8.5f, 800, pal::Muted, Align::Left, Baseline::Alphabetic, false, StatW - 6.0f));
	}
}

/** Ignite's PAYOUTS tab (§2.5 "Payouts with medals"): 1st, 2nd and 3rd on sunflower, silver and bronze medals; prizes in BlackItalic. */
void TblIgPayouts(Canvas& C, const Tournament& T, const Rect& Box)
{
	float Y = Box.Y + 16.0f;
	C.Text(std::to_string(T.PaidPlaces()) + " PAID \xC2\xB7 MIN CASH " + Money(T.Payouts.back()), Box.X + 4.0f, Y, Ts(12.0f, 800, pal::Muted));
	Y += 12.0f;
	static const uint32_t Medals[3] = {rlsites_detail::SitesIgSun, 0xdcd3ec, 0xe0915a};
	const int HeroRank = T.Hero().Busted ? 0 : T.HeroRank();
	for (const std::pair<int, int>& Band : PayoutBands(static_cast<int>(T.Payouts.size())))
	{
		if (Y + 34.0f > Box.Y + Box.H)
		{
			break;
		}
		const bool Reached = T.Remaining <= Band.second;
		const bool You = HeroRank >= Band.first && HeroRank <= Band.second;
		const bool Medal = Band.first == Band.second && Band.first <= 3;
		// The podium's three rows on pills and yours glowing coral; the rest bare (the table's budget, §10: 22k).
		const Rect Row{Box.X, Y, Box.W, 30.0f};
		if (You || Medal)
		{
			C.FillRoundRect(Row, 15.0f, You ? TblA(rlsites_detail::SitesIgCoral, 0.2f) : pal::Panel2);
		}
		const float Mx = Box.X + 18.0f;
		const float My = Y + 15.0f;
		if (Medal)
		{
			C.FillCircle(Mx, My, 11.0f, Hex(Medals[Band.first - 1]));
			TextStyle Nm = Ts(12.0f, 900, Hex(rlsites_detail::SitesIgPlumInk), Align::Center, Baseline::Middle);
			Nm.Face = static_cast<int8_t>(Font::BlackItalic);
			C.Text(std::to_string(Band.first), Mx, My + 0.5f, Nm);
		}
		const std::string Label = Band.first == Band.second ? Ordinal(Band.first) : Ordinal(Band.first) + "\xE2\x80\x93" + Ordinal(Band.second);
		C.Text(Label, Box.X + 38.0f, Y + 20.0f, Ts(14.0f, 700, Reached || You ? pal::Ink : pal::Muted));
		TextStyle Pz = Ts(16.0f, 900, Reached || You ? pal::Gold : pal::Muted, Align::Right);
		Pz.Face = static_cast<int8_t>(Font::BlackItalic);
		C.Text(Money(T.Payouts[static_cast<size_t>(Band.first - 1)]), Box.X + Box.W - 14.0f, Y + 21.0f, Pz);
		Y += 34.0f;
	}
}

} // namespace riverline_detail

using namespace riverline_detail;

namespace rlmail_detail
{
void ForgetMail(const RiverLine* Who);
}
namespace backing_detail
{
void Forget(const Ui& UI);
}
namespace rlrit_detail
{
void ForgetRit(const RiverLine* RL); // RitArt.cpp (ui-rit): RunItTwice's screen state, by laptop
}
namespace rlignite_detail
{
void IgniteForget(const RiverLine* RL); // RiverLineIgnite.cpp (ui-ignite): Ignite's screen state, by laptop
}

RiverLine::~RiverLine()
{
	rlnet_detail::ForgetNet(this);
	rlmail_detail::ForgetMail(this);
	backing_detail::Forget(UI);
	rlauto_detail::ForgetAuto(this);
	rlvideo_detail::ForgetVideo(this);
	rlsites_detail::ForgetSites(this);
	rlrit_detail::ForgetRit(this);
	rlignite_detail::IgniteForget(this);
	rlkast_detail::Forget(this); // RiverLineKastGrowth.cpp (ui-kast): the creator update's Kast widgets, by laptop
	BadgeCheers().erase(this);
}

// ------------------------------------------------------------------ frame

void RiverLine::Draw(Canvas& Cv, double Now)
{
	C = &Cv;
	UI.Begin(Cv, Now);
	NetFrame(Now);
	rlnet_detail::NetTipOf(this).Up = false;
	C->Save();
	// Update 2 (§1.8, §5.11): the poker client wears the colours of the site in front (the lobby's site, the table in
	// front's, a result's event's); RiverLine's own are pal::'s values, so at home nothing changes. The taskbar, the toasts
	// and the other apps stay outside the scope.
	const net::SiteId Wears = AppShown == App::RiverLine ? rlsites_detail::SitesClient(*this, S) : net::SiteId::RiverLine;
	{
		const SkinScope Ground(rlsites_detail::SitesSkin(Wears, World));
		C->FillRect({0.0f, 0.0f, RlW, RlH}, Paint::Linear({0.0f, 0.0f}, {0.0f, RlH}, pal::Bg2, pal::Bg));
	}
	// While the clock races (a shift, a run, sleep) or a result is up, the screen underneath takes no input.
	const bool Modal = S.TimeSkip.Active || S.HasOutcome;
	const Pointer Real = UI.Ptr;
	if (Modal)
	{
		UI.Ptr.Pressed = false;
		UI.Ptr.Released = false;
		UI.Ptr.Wheel = 0.0f;
		UI.Ptr.Active = false;
	}
	// The poker button's list of sites is up (over the screen's foot): what's under it takes no input there.
	// So is what's under the Sleep menu: a press on one of its buttons was also a press on the app's button beneath
	// (a night's sleep gave notice to a sponsor in Mail).
	const bool UnderMenu = !Modal && ((rlsites_detail::SitesMenuOpen(this) && UI.Hover(rlsites_detail::SitesMenuRect(S))) || (SleepOpen && UI.Hover(SleepMenuRect())));
	if (UnderMenu)
	{
		UI.Ptr.Pressed = false;
		UI.Ptr.Released = false;
		UI.Ptr.Wheel = 0.0f;
		UI.Ptr.Active = false;
	}
	if (AppShown == App::RiverLine)
	{
		const SkinScope Client(rlsites_detail::SitesSkin(Wears, World));
		ClientFrame(Now, false);
	}
	else if (AppShown == App::ShiftLink)
	{
		ShiftLinkApp(Now);
	}
	else if (AppShown == App::Burner)
	{
		BurnerApp(Now);
	}
	else if (AppShown == App::GearDrop)
	{
		GearDropApp(Now);
	}
	else if (AppShown == App::Kast)
	{
		KastApp(Now);
	}
	else if (AppShown == App::PennyDrop)
	{
		PennyDropApp(Now);
	}
	else if (AppShown == App::Mail)
	{
		MailApp(Now);
	}
	else if (AppShown == App::Video)
	{
		VideoApp(Now); // Rerun (update 2 §4.8; RiverLineVideo.cpp, ui-video's)
	}
	else
	{
		BankApp(Now);
	}
	// Badges earned since the laptop last looked (C10): taken once a frame, never under a time skip or a result card; each
	// gets its toast (the session plays the sound and tells Kast's chat). Nor before the player logs in: the host draws the laptop
	// behind the title screen too (the save loaded, nobody at the desk), where the toasts and their alert would go unseen.
	std::vector<BadgeCheer>& Cheering = BadgeCheers()[this];
	if (!Modal && (S.CurrentScreen != Screen::Boot || AppShown != App::RiverLine))
	{
		// Three at a time, half a second apart; more (an old save's first look) wait their turn in waves.
		for (const badge::Badge& B : S.Unseen())
		{
			const size_t K = Cheering.size();
			Cheering.push_back({B, Now + static_cast<double>(K / 3) * (BadgeToastSeconds + 0.2) + static_cast<double>(K % 3) * 0.5});
		}
	}
	BadgeToasts(UI, *C, S, Cheering, AppShown == App::RiverLine, Now);
	if (UnderMenu)
	{
		UI.Ptr = Real; // the list itself (drawn with the taskbar) takes the pointer
	}
	Taskbar({0.0f, RlH - 38.0f, RlW, 38.0f}, Now);
	if (SleepOpen)
	{
		SleepMenu(Now);
	}
	if (Modal)
	{
		const bool Cursor = UI.CursorIsPointer;
		UI.Ptr = Real;
		UI.CursorIsPointer = Cursor;
		if (S.TimeSkip.Active)
		{
			SkipOverlay(Now);
		}
		else
		{
			OutcomeCard(Now);
		}
	}
	// A badge under the pointer: what it is, over everything else (next_segment_spec §5.6).
	if (rlnet_detail::NetTipOf(this).Up && !Modal)
	{
		const rlnet_detail::NetTip Tip = rlnet_detail::NetTipOf(this);
		BadgeTip(Tip.B, Tip.X, Tip.Y, Now);
	}
	DrawCursor();
	C->Restore();
	UI.End();
	C = nullptr;
}

void RiverLine::ClientFrame(double Now, bool Preview)
{
	// Update 3 (W1, §1.11): one dispatch for the poker client, inside the client's SkinScope. A revamped rival draws its own
	// frame (its layout, navigation, chrome, pages and table layer); Boot stays RiverLine's.
	if (S.CurrentScreen != Screen::Boot)
	{
		const net::SiteId Client = rlsites_detail::SitesClient(*this, S);
		if (Client == net::SiteId::Ignite)
		{
			IgniteFrame(Now, Preview);
			return;
		}
		if (Client == net::SiteId::RunItTwice)
		{
			RitFrame(Now, Preview);
			return;
		}
	}
	HomeFrame(Now, Preview);
}

void RiverLine::HomeFrame(double Now, bool Preview)
{
	// The top bar and the screen. The stream preview (Preview) shows the client as it is and nothing that belongs to the
	// player's desk: no player card or autoplay panel over the table, no toasts, and it never closes the panel (U1: the
	// stream never shows the plan).
	if (!Preview && S.CurrentScreen != Screen::Table && rlauto_detail::AutoPanelOpen(this))
	{
		rlauto_detail::AutoClosePanel(this); // the autoplay panel belongs to the tables (update 2 §2.6.1)
	}
	TopBar(Now);
	switch (S.CurrentScreen)
	{
	case Screen::Boot: Boot(Now); break;
	case Screen::Lobby: LobbyPages(Now); break;
	case Screen::Table:
	{
		// A player card (a name at the table caught the player's eye) holds the table's input while it's up, and so does
		// the autoplay panel (update 2 §2.6.1).
		const bool Card = !Preview && CardOpen();
		const bool Auto = !Preview && !Card && rlauto_detail::AutoPanelOpen(this);
		const Pointer Held = UI.Ptr;
		if (Card || Auto)
		{
			UI.Ptr.Pressed = false;
			UI.Ptr.Released = false;
			UI.Ptr.Wheel = 0.0f;
			UI.Ptr.Active = false;
		}
		const bool Tiled = S.Tiled && S.TableCount() >= 2;
		if (Tiled)
		{
			Tiles(Now);
		}
		else
		{
			Table(Now);
		}
		if (Card)
		{
			UI.Ptr = Held;
			PlayerCard(Now);
		}
		else if (Auto)
		{
			UI.Ptr = Held;
			// Over the felt (one table), or the middle of the screen (the tiles).
			const float Pw = 1108.0f;
			AutoPanel({Tiled ? (RlW - Pw) / 2.0f : TableCx - Pw / 2.0f, RlTop + 14.0f, Pw, 870.0f}, Now);
		}
		break;
	}
	case Screen::Results: ResultsScreen(Now); break;
	}
	if (!Preview)
	{
		FinishedToasts(Now);
	}
}

// ------------------------------------------------------------------ small drawing helpers

float RiverLine::GradeLabel(Grade G, const std::string& Rest, float X, float Y, const TextStyle& Style, bool Draw)
{
	const float Sz = Style.Size;
	const bool Shape = G == Grade::Best || G == Grade::Good;
	const std::string IconText = GradeIconText(G);
	// Measured in the face drawn: a rival look's (Style.Face, G1 fix) or, as always, the weight's.
	const auto RunW = [&](const std::string& Run) { return Style.Face >= 0 ? C->Measure(Run, Sz, static_cast<Font>(Style.Face)) : C->Measure(Run, Sz, Style.Weight, Style.Mono); };
	const float IconW = Shape ? Sz * 0.95f : RunW(IconText);
	const float Gap = RunW(" ");
	const float RestW = RunW(Rest);
	const float Total = IconW + Gap + RestW;
	if (!Draw)
	{
		return Total;
	}
	const float X0 = X - (Style.HAlign == Align::Center ? Total / 2.0f : Style.HAlign == Align::Right ? Total : 0.0f);
	const float Base = BaselineOf(Y, Style);
	TextStyle Left = Style;
	Left.HAlign = Align::Left;
	if (G == Grade::Best)
	{
		std::vector<Vec2> Star;
		const float Cx = X0 + Sz * 0.47f;
		const float Cy = Base - Sz * 0.37f;
		for (int I = 0; I < 10; ++I)
		{
			const float A = -Pi / 2.0f + static_cast<float>(I) * Pi / 5.0f;
			const float R = (I % 2 == 0) ? Sz * 0.46f : Sz * 0.19f;
			Star.push_back({Cx + R * std::cos(A), Cy + R * std::sin(A)});
		}
		C->FillPolygon(Star, Style.Col);
	}
	else if (G == Grade::Good)
	{
		C->StrokePolyline({{X0 + Sz * 0.1f, Base - Sz * 0.37f}, {X0 + Sz * 0.38f, Base - Sz * 0.1f}, {X0 + Sz * 0.9f, Base - Sz * 0.7f}}, false, Style.Col, Sz * 0.15f);
	}
	else
	{
		C->Text(IconText, X0, Y, Left);
	}
	C->Text(Rest, X0 + IconW + Gap, Y, Left);
	return Total;
}

float RiverLine::ArrowText(const std::string& LeftText, const std::string& RightText, float X, float Y, const TextStyle& Style)
{
	const float Sz = Style.Size;
	const float Lw = C->Measure(LeftText, Sz, Style.Weight, Style.Mono);
	const float Rw = C->Measure(RightText, Sz, Style.Weight, Style.Mono);
	const float Aw = Sz * 0.8f;
	const float Gap = Sz * 0.4f;
	const float Total = Lw + Gap + Aw + Gap + Rw;
	const float X0 = X - (Style.HAlign == Align::Center ? Total / 2.0f : Style.HAlign == Align::Right ? Total : 0.0f);
	TextStyle Left = Style;
	Left.HAlign = Align::Left;
	C->Text(LeftText, X0, Y, Left);
	const float Base = BaselineOf(Y, Style);
	const float Ay = Base - Sz * 0.33f;
	const float Ax = X0 + Lw + Gap;
	const float Lw2 = Sz * 0.09f > 1.2f ? Sz * 0.09f : 1.2f;
	C->StrokePolyline({{Ax, Ay}, {Ax + Aw, Ay}}, false, Style.Col, Lw2);
	C->StrokePolyline({{Ax + Aw - Sz * 0.28f, Ay - Sz * 0.24f}, {Ax + Aw, Ay}, {Ax + Aw - Sz * 0.28f, Ay + Sz * 0.24f}}, false, Style.Col, Lw2);
	C->Text(RightText, Ax + Aw + Gap, Y, Left);
	return Total;
}

void RiverLine::Ghost(float Cx, float Cy, float Sz)
{
	const Color Body = Rgba(255, 255, 255, 0.92f);
	C->FillCircle(Cx, Cy - Sz * 0.12f, Sz * 0.34f, Body);
	C->FillRect({Cx - Sz * 0.34f, Cy - Sz * 0.12f, Sz * 0.68f, Sz * 0.36f}, Body);
	for (int I = 0; I < 3; ++I)
	{
		const float X0 = Cx - Sz * 0.34f + static_cast<float>(I) * Sz * 0.2267f;
		C->FillPolygon({{X0, Cy + Sz * 0.23f}, {X0 + Sz * 0.2267f, Cy + Sz * 0.23f}, {X0 + Sz * 0.113f, Cy + Sz * 0.4f}}, Body);
	}
	C->FillEllipse(Cx - Sz * 0.12f, Cy - Sz * 0.14f, Sz * 0.06f, Sz * 0.09f, Hex(0x2a0d14));
	C->FillEllipse(Cx + Sz * 0.12f, Cy - Sz * 0.14f, Sz * 0.06f, Sz * 0.09f, Hex(0x2a0d14));
}

// ------------------------------------------------------------------ chrome

void RiverLine::Logo(float X, float Y, float Scale)
{
	C->Save();
	C->Translate(X, Y);
	C->Scale(Scale, Scale);
	const float Prev = C->GetAlpha();
	for (int K = 0; K < 2; ++K)
	{
		std::vector<Vec2> Pts;
		for (int I = 0; I <= 36; ++I)
		{
			Pts.push_back({static_cast<float>(I), 6.0f + static_cast<float>(K) * 11.0f + std::sin((static_cast<float>(I) / 36.0f) * Pi * 2.0f) * 5.0f});
		}
		C->SetAlpha(Prev * (K == 0 ? 1.0f : 0.55f));
		C->StrokePolyline(Pts, false, pal::Accent, 5.0f, true);
	}
	C->SetAlpha(Prev);
	UI.Text("River", 48.0f, 26.0f, Ts(28.0f, 800, pal::Ink));
	UI.Text("Line", 48.0f + UI.Measure("River", 28.0f, 800), 26.0f, Ts(28.0f, 400, pal::Accent));
	C->Restore();
}

void RiverLine::TopBar(double Now)
{
	// The site in front's logo and its own balance (update 2 §5.11: a rival's is its wallet; RiverLine's the bank).
	const net::SiteId Site = rlsites_detail::SitesClient(*this, S);
	C->FillRect({0.0f, 0.0f, RlW, RlTop}, rlsites_detail::SitesTone(0x08101b));
	C->FillRect({0.0f, RlTop - 1.0f, RlW, 1.0f}, pal::Line);
	if (Site == net::SiteId::RiverLine)
	{
		Logo(24.0f, 16.0f, 1.0f);
	}
	else
	{
		siteart::Logo(*C, {24.0f, 14.0f, 214.0f, 36.0f}, Site);
	}
	if (S.CurrentScreen != Screen::Boot)
	{
		NavTabs(Now);
	}
	// Balance and clock.
	UI.Text(ClockString(S.ClockMinutes()), RlW - 24.0f, 34.0f, Ts(17.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
	UI.Text(std::string(net::WeekdayName(net::DayOf(World))) + ", " + net::DateLabel(net::DayOf(World)), RlW - 24.0f, 52.0f, Ts(12.0f, 600, pal::Muted, Align::Right));
	const std::string Bal = "Balance " + Money(rlsites_detail::SitesBalance(S, Site));
	const float Bw = UI.Measure(Bal, 17.0f, 700) + 28.0f;
	UI.RRect({RlW - 140.0f - Bw, 16.0f, Bw, 32.0f}, 16.0f, rlsites_detail::SitesTone(0x10213a), pal::Line);
	UI.Text(Bal, RlW - 140.0f - Bw / 2.0f, 38.0f, Ts(17.0f, 700, pal::Gold, Align::Center));
	if (S.Streaming())
	{
		LivePill(RlW - 150.0f - Bw, 17.0f, Now); // on Kast: viewers, a click away from the studio
	}
	// Connection dot.
	C->FillCircle(RlW - 128.0f, 32.0f, 4.0f, std::sin(Now * 2.0) > -0.9 ? pal::Green : pal::Dim);
}

Chips RiverLine::PotTotal() const
{
	Chips Total = S.PotChips;
	for (const SeatVis& Seat : S.Seats)
	{
		if (Seat.Present)
		{
			Total += Seat.Bet;
		}
	}
	return Total;
}

void RiverLine::DrawCursor()
{
	const Pointer& P = UI.Ptr;
	if (!P.Active)
	{
		return;
	}
	C->Save();
	C->Translate(P.X, P.Y);
	if (UI.CursorIsPointer)
	{
		C->FillCircle(0.0f, 0.0f, 9.0f, rlnet_detail::NetA(pal::Accent, 0.25f));
		C->StrokeArc(0.0f, 0.0f, 9.0f, 0.0f, 2.0f * Pi, Hex(0xffffff), 2.0f);
	}
	else
	{
		const std::vector<Vec2> Arrow = {{0, 0}, {0, 26}, {7, 20}, {12, 31}, {16, 29}, {11, 18}, {19, 18}};
		C->FillPolygon(Arrow, Hex(0xffffff));
		C->StrokePolyline(Arrow, true, Hex(0x000000), 1.5f);
	}
	C->Restore();
}

// ------------------------------------------------------------------ boot

void RiverLine::Boot(double Now)
{
	// Animated waves.
	const float Prev = C->GetAlpha();
	C->SetAlpha(Prev * 0.18f);
	for (int K = 0; K < 6; ++K)
	{
		std::vector<Vec2> Pts;
		for (float X = 0.0f; X <= RlW; X += 8.0f)
		{
			Pts.push_back({X, 640.0f + static_cast<float>(K) * 34.0f + static_cast<float>(std::sin(X / 140.0f + Now * 0.6 + K)) * 18.0f});
		}
		C->StrokePolyline(Pts, false, K % 2 ? pal::Accent : pal::Accent2, 2.0f);
	}
	C->SetAlpha(Prev);
	const Rect Card{RlW / 2.0f - 280.0f, 200.0f, 560.0f, 420.0f};
	UI.RRect(Card, 18.0f, rlnet_detail::NetA(pal::Panel, 0.92f), pal::Line);
	Logo(Card.X + 150.0f, Card.Y + 50.0f, 1.6f);
	UI.Text("Welcome back, " + S.HeroName, RlW / 2.0f, Card.Y + 175.0f, Ts(26.0f, 700, pal::Ink, Align::Center));
	UI.Text("Account balance", RlW / 2.0f, Card.Y + 225.0f, Ts(16.0f, 500, pal::Muted, Align::Center));
	UI.Text(Money(S.BankrollCents), RlW / 2.0f, Card.Y + 275.0f, Ts(44.0f, 800, pal::Gold, Align::Center, Baseline::Alphabetic, true));
	ButtonOpts Login;
	Login.Kind = ButtonKind::Primary;
	Login.Size = 24.0f;
	if (rlsites_detail::SitesButton(UI, *C, "login", {Card.X + 60.0f, Card.Y + 320.0f, Card.W - 120.0f, 64.0f}, "Log in", Login))
	{
		S.CurrentScreen = Screen::Lobby;
		S.OnBoot();
	}
	UI.Text("Play responsibly. RiverLine is a fictional site.", RlW / 2.0f, Card.Y + Card.H + 40.0f, Ts(14.0f, 500, pal::Dim, Align::Center));
}

float RiverLine::Wrap(const std::string& Text, float X, float Y, float MaxW, float Size, const Color& Col, float LineHeight)
{
	float Yy = Y;
	for (const std::string& Line : WrapLines(Text, MaxW, Size))
	{
		UI.Text(Line, X, Yy, Ts(Size, 500, Col));
		Yy += LineHeight;
	}
	return Yy;
}

std::vector<std::string> RiverLine::WrapLines(const std::string& Text, float MaxW, float Size)
{
	std::vector<std::string> Out;
	std::string Line;
	size_t Start = 0;
	while (Start <= Text.size())
	{
		size_t End = Text.find(' ', Start);
		if (End == std::string::npos)
		{
			End = Text.size();
		}
		const std::string Word = Text.substr(Start, End - Start);
		const std::string Test = Line.empty() ? Word : Line + " " + Word;
		if (UI.Measure(Test, Size) > MaxW && !Line.empty())
		{
			Out.push_back(Line);
			Line = Word;
		}
		else
		{
			Line = Test;
		}
		Start = End + 1;
	}
	if (!Line.empty())
	{
		Out.push_back(Line);
	}
	return Out;
}

// ------------------------------------------------------------------ table

int RiverLine::HeroSeatNo() const
{
	for (size_t I = 0; I < S.Seats.size(); ++I)
	{
		if (S.Seats[I].Present && S.Seats[I].IsHero)
		{
			return static_cast<int>(I);
		}
	}
	return 0;
}

Vec2 RiverLine::SlotPos(int Seat) const
{
	// Seats around the oval from the hero at the bottom (nine-handed, or six-max). Update 3 (§2.5, §3.5): a site's look may
	// place them itself, for 8, 6 and 3 seats (RiverLine's has no list: the ring, as always).
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	const int N = std::max(2, static_cast<int>(S.Seats.size()));
	const int Slot = (Seat - HeroSeatNo() + N) % N;
	if (Look.SeatList && (N == 8 || N == 6 || N == 3))
	{
		return N == 8 ? Look.Seat8[static_cast<size_t>(Slot)] : N == 6 ? Look.Seat6[static_cast<size_t>(Slot)] : Look.Seat3[static_cast<size_t>(Slot)];
	}
	const float A = (90.0f + static_cast<float>(Slot) * 360.0f / static_cast<float>(N)) * Pi / 180.0f;
	return {Look.Cx + (Look.Rx + 50.0f) * std::cos(A), Look.Cy + (Look.Ry + 64.0f) * std::sin(A)};
}

Vec2 RiverLine::BetPos(int Seat) const
{
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	const Vec2 P = SlotPos(Seat);
	if (Look.SeatList)
	{
		return TblBetSpot(Look, P); // G1 (§2.5, §3.5): 0.56 of the way in from a listed seat
	}
	return {Look.Cx + (P.X - Look.Cx) * 0.56f, Look.Cy + (P.Y - Look.Cy) * 0.5f};
}

void RiverLine::Table(double Now)
{
	if (!S.T)
	{
		return;
	}
	const Tournament& T = *S.T;
	// Update 3 (§1.11): the shared table in its site's look: the felt's centre and radii, its paints, the chips, the backs and
	// the dealer button (RiverLine's: today's oval and literals; the capsule, the octagon and the rivals' art are
	// ui-tablestyle's).
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	const float FeltCx = Look.Cx;
	const float FeltCy = Look.Cy;
	const float FeltRx = Look.Rx;
	const float FeltRy = Look.Ry;
	const Color FeltMid = Look.Felt ? Hex(Look.Felt) : pal::Felt;
	const Color FeltHi = Look.FeltLight ? Hex(Look.FeltLight) : rlsites_detail::SitesFeltLight();
	const Color FeltLo = Look.FeltDark ? Hex(Look.FeltDark) : pal::FeltDark;
	// G1 (§1.11, §0.6 #12): a rival's look draws its own felt, rail, pot and seats, and puts the parent's alpha back where
	// RiverLine's sets 1 (identical at home, whose parent alpha is 1).
	const bool Rival = Look.SeatList;
	const float Alpha0 = Look.RestoreAlpha ? C->GetAlpha() : 1.0f;
	const bool Still = Previewing;
	const bool FinalTable = T.Remaining <= std::max(2, T.Spec.TableSize);
	const net::SiteId Site = rlsites_detail::SitesOfTable(S);
	const bool InBb = rlsites_detail::SitesBetsInBb(*this, Look);
	const Chips TableBb = S.CurHand ? std::max<Chips>(1, S.CurHand->BigBlind) : std::max<Chips>(1, T.CurrentLevel().Bb);

	if (!Rival)
	{
		// Felt and rail.
		C->FillEllipse(FeltCx, FeltCy + 10.0f, FeltRx + 34.0f, FeltRy + 34.0f, Rgba(0, 0, 0, 0.5f));
		C->FillEllipse(FeltCx, FeltCy, FeltRx + 30.0f, FeltRy + 30.0f, Paint::Linear({0.0f, FeltCy - FeltRy}, {0.0f, FeltCy + FeltRy}, rlsites_detail::SitesTone(0x2a3342), rlsites_detail::SitesTone(0x10151d)));
		C->FillEllipse(FeltCx, FeltCy, FeltRx, FeltRy, Paint::Radial({FeltCx, FeltCy - 40.0f}, 30.0f, {FeltCx, FeltCy}, FeltRx, FeltHi, 0.7f, FeltMid, FeltLo));
		C->StrokeEllipse(FeltCx, FeltCy, FeltRx - 40.0f, FeltRy - 34.0f, Rgba(255, 255, 255, 0.08f), 2.0f);
		C->SetAlpha(0.14f);
		// The site's own mark on its felt (update 2 §5.11).
		if (Site == net::SiteId::RiverLine)
		{
			Logo(FeltCx - 95.0f, FeltCy + 88.0f, 1.25f);
		}
		else
		{
			siteart::Logo(*C, {FeltCx - 125.0f, FeltCy + 92.0f, 250.0f, 42.0f}, Site);
		}
		C->SetAlpha(1.0f);
	}
	else
	{
		const bool HeroTurn = S.HasPrompt;
		if (Look.Shape == rlsites_detail::TableShape::Capsule)
		{
			TblCapsule(*C, Look, Now, Still, FinalTable, igniteart::IgniteBpmAt(World));
			// The flame and "Ignite" as a neon sign (§2.5): siteart's NeonSign, three soft Hot Pink copies at 0.06/0.10/0.16 under
			// a crisp one at 0.22, in colours' own alpha (the parent alpha carries through), centred under the board.
			const float SignSize = 40.0f;
			const float SignW = SignSize * 0.66f + C->Measure("Ignite", SignSize * 0.92f, Font::BlackItalic);
			siteart::NeonSign(*C, FeltCx - SignW / 2.0f, FeltCy + 134.0f, SignSize, 1.0f);
		}
		else
		{
			TblOctagonTable(*C, Look, Now, Still, HeroTurn, FinalTable);
		}
	}

	// Pot and board: the stack is what's been gathered in; the label counts the bets out in front too.
	if (!Rival)
	{
		if (S.PotChips > 0)
		{
			DrawChipStack(*C, S.PotChips, FeltCx, FeltCy - 75.0f, 1.0f, 1.0f, Look.ChipSet);
		}
		if (const Chips Total = PotTotal(); Total > 0)
		{
			UI.RRect({FeltCx - 80.0f, FeltCy - 128.0f, 160.0f, 30.0f}, 15.0f, Rgba(0, 0, 0, 0.45f));
			UI.Text("Pot " + ChipsText(static_cast<double>(Total)), FeltCx, FeltCy - 107.0f, Ts(17.0f, 700, pal::Ink, Align::Center));
		}
	}
	else
	{
		// A rival's pot sits under the board (the top seat's bet lands above it), its chips beside the pill.
		if (S.PotChips > 0)
		{
			const Vec2 Ps = TblPotSpot(Look);
			DrawChipStack(*C, S.PotChips, Ps.X, Ps.Y, 0.9f, 1.0f, Look.ChipSet);
		}
		if (const Chips Total = PotTotal(); Total > 0)
		{
			const Rect Pp{FeltCx - 80.0f, FeltCy + 40.0f, 160.0f, 28.0f};
			if (Look.Seat == rlsites_detail::SeatStyle::DataCard)
			{
				// RunItTwice: a Tape Black slab, POT in caps and the figure in big blinds (and chips) in BoldCondensed.
				const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(Pp, 6.0f);
				C->FillPolygon(Path, Hex(rlsites_detail::SitesRitTape));
				C->StrokePolyline(Path, true, Hex(rlsites_detail::SitesRitHair), 1.0f);
				TextStyle Lb = Ts(11.0f, 700, pal::Muted, Align::Left, Baseline::Middle);
				Lb.Face = static_cast<int8_t>(Font::BoldCondensed);
				UI.Text("POT", Pp.X + 12.0f, Pp.Y + Pp.H / 2.0f, Lb);
				TextStyle Fg = Ts(15.0f, 700, pal::Gold, Align::Right, Baseline::Middle);
				Fg.Face = static_cast<int8_t>(Font::BoldCondensed);
				UI.Text(InBb ? rlsites_detail::SitesBbText(Total, TableBb) : ChipsText(static_cast<double>(Total)), Pp.X + Pp.W - 12.0f, Pp.Y + Pp.H / 2.0f, Fg);
			}
			else
			{
				// Ignite: a Night pill with a coral rim, the figure in BlackItalic; it pulses as the pot grows (G1_AUDIT N4,
				// RiverLineIgnite.cpp).
				rlignite_detail::IgniteTablePotPill(*C, *this, Pp, Total, Now);
			}
		}
	}
	const float Bw = Rival ? 72.0f : 84.0f;
	const float Bh = Rival ? 100.0f : 118.0f;
	const float Bx0 = FeltCx - (5.0f * Bw + 4.0f * 10.0f) / 2.0f;
	const float By0 = Rival ? FeltCy - 70.0f : FeltCy - 45.0f;
	const std::vector<Card> Winners = WinningCards();
	auto IsWinner = [&](Card Cd) { return std::find(Winners.begin(), Winners.end(), Cd) != Winners.end(); };
	for (size_t I = 0; I < S.Board.size(); ++I)
	{
		const double At = I < S.BoardShownAt.size() ? S.BoardShownAt[I] : 0.0;
		if (Now < At)
		{
			continue;
		}
		CardOpts O;
		O.Flip = static_cast<float>(std::min(1.0, (Now - At) / 0.25));
		O.Highlight = IsWinner(S.Board[I]);
		O.Dim = !Winners.empty() && !IsWinner(S.Board[I]);
		O.Back = Look.CardBack;
		DrawCard(*C, S.Board[I], Bx0 + static_cast<float>(I) * (Bw + 10.0f), By0, Bw, Bh, O);
	}

	// Dealer button (Look.Dealer > 0: a site's own, between its seat and the felt's centre; RiverLine's is the white disc).
	const Vec2 Bp = SlotPos(S.ButtonSeat);
	if (Look.Dealer > 0)
	{
		const Vec2 Dp = TblDealerSpot(Look, Bp);
		rlsites_detail::SitesDealerButton(*C, Dp.X, Dp.Y, 15.0f, Look.Dealer);
	}
	else
	{
		const float Dbx = FeltCx + (Bp.X - FeltCx) * 0.74f + 34.0f;
		const float Dby = FeltCy + (Bp.Y - FeltCy) * 0.7f;
		C->FillCircle(Dbx, Dby, 15.0f, Hex(0xf4f4f4));
		UI.Text("D", Dbx, Dby + 1.0f, Ts(16.0f, 800, Hex(0x111111), Align::Center, Baseline::Middle));
	}

	// Bets.
	for (const SeatVis& Seat : S.Seats)
	{
		if (!Seat.Present || Seat.Bet <= 0)
		{
			continue;
		}
		const Vec2 P = BetPos(Seat.Seat);
		DrawChipStack(*C, Seat.Bet, P.X, P.Y, 0.85f, 1.0f, Look.ChipSet);
		if (!Rival)
		{
			UI.Text(ChipsText(static_cast<double>(Seat.Bet)), P.X, P.Y + 30.0f, Ts(15.0f, 700, Hex(0xffffff), Align::Center));
		}
		else if (Look.Seat == rlsites_detail::SeatStyle::DataCard)
		{
			// RunItTwice: in big blinds by default (§3.5; the toggle shows chips), BoldCondensed, Hot White.
			TextStyle Bt = Ts(15.0f, 700, pal::Gold, Align::Center);
			Bt.Face = static_cast<int8_t>(Font::BoldCondensed);
			UI.Text(InBb ? rlsites_detail::SitesBbText(Seat.Bet, TableBb) : ChipsText(static_cast<double>(Seat.Bet)), P.X, P.Y + 26.0f, Bt);
		}
		else
		{
			// Ignite: the figure in a Night pill under the chips, BlackItalic.
			const std::string Amt = ChipsText(static_cast<double>(Seat.Bet));
			TextStyle Bt = Ts(13.0f, 900, Hex(rlsites_detail::SitesIgCream), Align::Center, Baseline::Middle);
			Bt.Face = static_cast<int8_t>(Font::BlackItalic);
			const float Wd = std::max(56.0f, C->Measure(Amt, 13.0f, Font::BlackItalic) + 22.0f);
			UI.RRect({P.X - Wd / 2.0f, P.Y + 14.0f, Wd, 20.0f}, 10.0f, TblA(rlsites_detail::SitesIgNight, 0.92f), Hex(0x5a3a86), 1.0f);
			UI.Text(Amt, P.X, P.Y + 24.5f, Bt);
		}
	}

	// Seats.
	for (const SeatVis& Seat : S.Seats)
	{
		if (Seat.Present)
		{
			DrawSeat(Seat, Now);
		}
	}
	for (size_t I = 0; I < S.Seats.size(); ++I)
	{
		if (S.Seats[I].Present)
		{
			continue;
		}
		const Vec2 P = SlotPos(static_cast<int>(I));
		if (Look.Seat == rlsites_detail::SeatStyle::Bubble)
		{
			// Ignite: an empty bubble, a dashed ring.
			C->StrokeDashed(TblClosed(Canvas::EllipsePath(P.X, P.Y, 30.0f, 30.0f, 0.0f, 2.0f * Pi, 36, false)), Rgba(255, 244, 236, 0.18f), 1.5f, 4.0f, 4.0f);
			UI.Text("Empty", P.X, P.Y + 5.0f, Ts(13.0f, 600, pal::Dim, Align::Center));
			continue;
		}
		if (Look.Seat == rlsites_detail::SeatStyle::DataCard)
		{
			// RunItTwice: an open seat's card outline.
			const Rect Box = rlsites_detail::SitesSeatBox(Look, P, false);
			C->FillPolygon(rlsites_detail::SitesCutPath(Box, 6.0f), Color{pal::Bg.R, pal::Bg.G, pal::Bg.B, 0.7f});
			C->StrokePolyline(rlsites_detail::SitesCutPath(Box, 6.0f), true, Rgba(255, 255, 255, 0.08f), 1.0f);
			TextStyle Op = Ts(13.0f, 700, pal::Dim, Align::Center, Baseline::Middle);
			Op.Face = static_cast<int8_t>(Font::BoldCondensed);
			UI.Text("OPEN SEAT", P.X, P.Y, Op);
			continue;
		}
		UI.RRect({P.X - 60.0f, P.Y - 22.0f, 120.0f, 44.0f}, 22.0f, Rgba(255, 255, 255, 0.03f), Rgba(255, 255, 255, 0.08f));
		UI.Text("Empty", P.X, P.Y + 6.0f, Ts(14.0f, 500, pal::Dim, Align::Center));
	}

	// Flights.
	for (const Flight& F : S.Flights)
	{
		DrawFlight(F, Now);
	}

	// Hero hand strength.
	const SeatVis* Hero = nullptr;
	for (const SeatVis& Seat : S.Seats)
	{
		Hero = Seat.Present && Seat.IsHero ? &Seat : Hero;
	}
	if (Hero && Hero->Hole.size() == 2 && !Hero->Folded && S.Board.size() >= 3)
	{
		std::vector<Card> Shown;
		for (size_t I = 0; I < S.Board.size(); ++I)
		{
			if (Now >= (I < S.BoardShownAt.size() ? S.BoardShownAt[I] : 0.0))
			{
				Shown.push_back(S.Board[I]);
			}
		}
		if (Shown.size() >= 3)
		{
			const std::string Label = Describe(EvaluateHand(Hero->Hole, Shown));
			const Vec2 Hp = SlotPos(Hero->Seat);
			if (!Rival)
			{
				UI.RRect({Hp.X - 110.0f, Hp.Y - 190.0f, 220.0f, 28.0f}, 14.0f, Rgba(0, 0, 0, 0.55f));
				UI.Text(Label, Hp.X, Hp.Y - 170.0f, Ts(15.0f, 700, pal::Gold, Align::Center));
			}
			else
			{
				// A rival's: on the felt between the pot and the hero's bet, in the look's face.
				TextStyle Hs = Ts(15.0f, 700, pal::Gold, Align::Center, Baseline::Middle);
				Hs.Face = Look.TitleFace;
				const std::string Shown2 = Look.LabelCaps ? rlnet_detail::NetUpper(Label) : Label;
				const float Wd = C->Measure(Shown2, 15.0f, static_cast<Font>(Look.TitleFace)) + 28.0f;
				if (Look.Cut > 0.0f)
				{
					// RunItTwice's slab ends at Cy+96, clear of the Second Board's plate under it (from Cy+98, RitTableLayer).
					C->FillPolygon(rlsites_detail::SitesCutPath({FeltCx - Wd / 2.0f, FeltCy + 72.0f, Wd, 24.0f}, 6.0f), Rgba(6, 6, 7, 0.8f));
					UI.Text(Shown2, FeltCx, FeltCy + 84.0f, Hs);
				}
				else
				{
					UI.RRect({FeltCx - Wd / 2.0f, FeltCy + 76.0f, Wd, 26.0f}, 13.0f, Rgba(0, 0, 0, 0.55f));
					UI.Text(Shown2, FeltCx, FeltCy + 89.0f, Hs);
				}
			}
		}
	}

	// The format: knockouts and seats.
	if (T.Spec.BountyCents > 0 || T.Spec.SeatValueCents > 0)
	{
		std::string Chip;
		if (T.Spec.SeatValueCents > 0)
		{
			Chip = "SATELLITE \xC2\xB7 " + std::to_string(S.SeatsInPlay()) + " seats \xC2\xB7 " + Grouped(T.Remaining) + " left";
		}
		else if (T.Spec.MysteryBounty)
		{
			Chip = "MYSTERY BOUNTY \xC2\xB7 envelopes open in the money \xC2\xB7 " + std::to_string(S.Knockouts) + " KOs \xC2\xB7 " + Money(S.BountyWon);
		}
		else
		{
			const auto Mine = S.Bounties.find(HeroId);
			Chip = "PKO \xC2\xB7 your bounty " + Money(Mine == S.Bounties.end() ? 0 : Mine->second) + " \xC2\xB7 " + std::to_string(S.Knockouts) + " KOs \xC2\xB7 won " + Money(S.BountyWon);
		}
		const Color Cc = T.Spec.SeatValueCents > 0 ? pal::Gold : pal::Orange;
		if (Rival)
		{
			// A rival's frame owns more chrome (Ignite's marquee, RunItTwice's collapsed rail and header): the chip sits below it,
			// in the look's own shape and face (g1-audit M2: RunItTwice's cut slab in BoldCondensed caps, Ignite's pill in
			// BlackItalic).
			const std::string Text = Look.LabelCaps ? rlnet_detail::NetUpper(Chip) : Chip;
			const TextStyle St = TblFaced(Look, 14.0f, Cc, Align::Center, Baseline::Middle);
			const Rect Cr{Look.Cut > 0.0f ? 80.0f : 24.0f, Look.ChromeH + 14.0f, C->Measure(Text, 14.0f, static_cast<Font>(Look.TitleFace)) + 28.0f, 30.0f};
			if (Look.Cut > 0.0f)
			{
				const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(Cr, 6.0f);
				C->FillPolygon(Path, NetChipFill(Cc));
				C->StrokePolyline(Path, true, Cc, 1.0f);
			}
			else
			{
				UI.RRect(Cr, Cr.H / 2.0f, NetChipFill(Cc), Cc);
			}
			UI.Text(Text, Cr.X + Cr.W / 2.0f, Cr.Y + Cr.H / 2.0f + 1.0f, St);
		}
		else
		{
			const float W = UI.Measure(Chip, 13.0f, 800) + 28.0f;
			UI.RRect({24.0f, RlTop + 14.0f, W, 30.0f}, 15.0f, NetChipFill(Cc), Cc);
			UI.Text(Chip, 24.0f + W / 2.0f, RlTop + 14.0f + 16.0f, Ts(13.0f, 800, Cc, Align::Center, Baseline::Middle));
		}
	}
	if (Now - S.LastBountyAt < 2.6)
	{
		const float K = static_cast<float>((Now - S.LastBountyAt) / 2.6);
		const Vec2 Hp = SlotPos(HeroSeatNo());
		C->SetAlpha(Alpha0 * (K < 0.75f ? 1.0f : (1.0f - K) * 4.0f));
		const float Y = Hp.Y - 40.0f - K * 50.0f;
		if (Rival)
		{
			// g1-audit M2: in the look's own figures (RunItTwice's BoldCondensed, printed twice; Ignite's BlackItalic).
			TextStyle Fig = TblFaced(Look, 34.0f, Look.LabelCaps ? pal::Accent : pal::Gold);
			if (Look.LabelCaps)
			{
				UI.Text("+" + Money(S.LastBountyCents), Hp.X + 142.0f, Y + 2.0f, Fig);
				Fig.Col = pal::Gold;
			}
			UI.Text("+" + Money(S.LastBountyCents), Hp.X + 140.0f, Y, Fig);
			UI.Text((T.Spec.MysteryBounty ? "MYSTERY BOUNTY \xC2\xB7 " : "BOUNTY \xC2\xB7 ") + S.LastBountyName, Hp.X + 140.0f, Y + 22.0f, TblFaced(Look, 13.0f, pal::Orange));
		}
		else
		{
			UI.Text("+" + Money(S.LastBountyCents), Hp.X + 140.0f, Y, Ts(34.0f, 900, pal::Gold, Align::Left, Baseline::Alphabetic, true));
			UI.Text((T.Spec.MysteryBounty ? "MYSTERY BOUNTY \xC2\xB7 " : "BOUNTY \xC2\xB7 ") + S.LastBountyName, Hp.X + 140.0f, Y + 22.0f, Ts(13.0f, 800, pal::Orange));
		}
		C->SetAlpha(Alpha0);
	}

	GradeBadges(Now);
	Controls(Now);
	SidePanel(Now);
	// Update 3 (§2.5, §3.5): a revamped rival's own layer over the shared table (Ignite's pins, stickers and Ouch bubble;
	// RunItTwice's lower third, the Second Board and its tabs), under the table's overlays (the banner, table balancing, the
	// bounty and the skip card, which dims the table under it: fix2-auto, g1-audit N1) and the stream's alerts.
	if (Site == net::SiteId::Ignite)
	{
		IgniteTableLayer(Now);
	}
	else if (Site == net::SiteId::RunItTwice)
	{
		RitTableLayer(Now);
	}
	Overlays(Now);
	if (S.Streaming() && !Previewing)
	{
		StreamAlert(FeltCx, FeltCy - FeltRy + 24.0f, 0.72f, Now); // follows, subs, tips as they come in, on the felt
	}
}

std::vector<Card> RiverLine::WinningCards() const
{
	std::vector<Card> Out;
	if (!S.CurHand || !S.CurHand->bComplete || S.Board.size() < 5)
	{
		return Out;
	}
	const SeatVis* Winner = nullptr;
	for (const SeatVis& Seat : S.Seats)
	{
		if (!Winner && Seat.Present && Seat.Winner && !Seat.HandLabel.empty())
		{
			Winner = &Seat;
		}
	}
	if (!Winner || Winner->Hole.size() != 2)
	{
		return Out;
	}
	// Highlight the five cards that make the winning hand.
	std::vector<Card> All = Winner->Hole;
	All.insert(All.end(), S.Board.begin(), S.Board.end());
	const int Target = EvaluateHand(Winner->Hole, S.Board);
	for (int A = 0; A < 7; ++A)
	{
		for (int B = A + 1; B < 7; ++B)
		{
			std::vector<Card> Five;
			for (int I = 0; I < 7; ++I)
			{
				if (I != A && I != B)
				{
					Five.push_back(All[static_cast<size_t>(I)]);
				}
			}
			if (Evaluate(Five.data(), 5) == Target)
			{
				return Five;
			}
		}
	}
	return Out;
}

void RiverLine::DrawSeat(const SeatVis& Seat, double Now)
{
	const Vec2 P = SlotPos(Seat.Seat);
	const bool IsHero = Seat.IsHero;
	const float Pw = IsHero ? 230.0f : 190.0f;
	const float Ph = 64.0f;
	const Rect Plate{P.X - Pw / 2.0f, P.Y - Ph / 2.0f, Pw, Ph};
	const bool Dim = Seat.Folded && !Seat.Winner;
	const bool HandOver = S.CurHand && S.CurHand->bComplete;
	// Update 3 (§1.11): the seat in its table's look (its card backs, the felt's middle; Ignite's bubbles and RunItTwice's data
	// cards below, RiverLine's plate after them, as always).
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	if (Look.Seat != rlsites_detail::SeatStyle::Plate)
	{
		const bool Bubble = Look.Seat == rlsites_detail::SeatStyle::Bubble;
		const float A0 = C->GetAlpha();
		const Rect Box = rlsites_detail::SitesSeatBox(Look, P, IsHero);
		const bool Band = Look.LowerThird.W > 0.0f && rlsites_detail::SitesLowerThirdUp(S);
		const Chips Bb = S.CurHand ? std::max<Chips>(1, S.CurHand->BigBlind) : (S.T ? std::max<Chips>(1, S.T->CurrentLevel().Bb) : 1);
		const bool InBb = rlsites_detail::SitesBetsInBb(*this, Look);
		const bool Fading = IsHero && Seat.Folded && S.AutoFolded && Now - S.AutoFoldedAt < 1.5;
		TblHoleCards(*C, Look, Seat, P, Box, Band, Now, Fading ? S.AutoFoldedCards : std::vector<Card>(), S.AutoFoldedAt);
		AvatarSpec Pic = AvatarFor(Seat.Name);
		if (IsHero)
		{
			Pic.Frame = AvatarFrame::Neon;
			Pic.Rim = rlnet_detail::NetRgb(pal::Accent);
		}
		else if (Seat.Pro)
		{
			Pic.Frame = AvatarFrame::Gold;
		}
		rlnet_detail::NetChampion(Pic, Seat.Name, IsHero);
		const auto Known = IsHero ? S.FieldNpc.end() : S.FieldNpc.find(Seat.Id);
		bool NameHover = false;
		if (Known != S.FieldNpc.end())
		{
			const Ui::ClickState St = UI.Clickable("seatcard" + Seat.Id, Box);
			NameHover = St.Hover;
			if (St.Clicked)
			{
				ShowPlayer(Known->second, Now);
			}
		}
		// The HUD line (g1-audit L10): on by default at a look that says so (RunItTwice's pro default), else Session::Hud.
		const TPlayer* Pl = rlsites_detail::SitesHudOn(*this, S, Look) && !IsHero ? S.PlayerById(Seat.Id) : nullptr;
		const std::string Hud = Pl && Pl->Hands >= 3 ? "VPIP " + std::to_string(static_cast<int>(JsRound(static_cast<double>(Pl->VpipHands) / Pl->Hands * 100.0))) + " \xC2\xB7 PFR " +
				std::to_string(static_cast<int>(JsRound(static_cast<double>(Pl->PfrHands) / Pl->Hands * 100.0))) + " \xC2\xB7 " + std::to_string(Pl->Hands) + "h"
														: std::string();
		// The action clock's share left (the time bank's, once it runs), as the plate's bar counts it.
		double Frac = 0.0;
		double Left = 0.0;
		if (Seat.Acting)
		{
			const double Total = Seat.ActEnd - Seat.ActStart;
			Left = std::max(0.0, Seat.ActEnd - Now);
			Frac = Total > 0.0 ? Left / Total : 0.0;
			if (IsHero && S.HasPrompt && S.Prompt.TimeBankUntil > 0.0)
			{
				Left = std::max(0.0, S.Prompt.TimeBankUntil - Now);
				Frac = Left / std::max(1.0, S.TimeBank);
			}
		}
		const bool Shown = !Seat.LastAction.empty() && Now - Seat.LastActionAt < 30.0 && Seat.LastAction != "Fold";
		const bool Labelled = !Seat.HandLabel.empty() && HandOver && !Seat.Folded;
		C->SetAlpha(A0 * (Dim ? 0.55f : 1.0f));
		if (Bubble)
		{
			// Ignite's bubble (§2.5): a 64 avatar (the hero's 76) in its ring, a cream name pill, the stack in BlackItalic.
			const float R = IsHero ? 38.0f : 32.0f;
			if (Seat.Winner && HandOver)
			{
				C->GlowRoundRect({P.X - R - 14.0f, P.Y - R - 14.0f, 2.0f * R + 28.0f, 2.0f * R + 28.0f}, R + 14.0f, TblA(rlsites_detail::SitesIgSun, 0.5f), 18.0f);
			}
			C->FillCircle(P.X, P.Y + 3.0f, R + 5.0f, Rgba(0, 0, 0, 0.4f));
			C->FillCircle(P.X, P.Y, R + 5.0f, Hex(0x3b2a55)); // the level ring's track round the avatar (its arc is G2's levels)
			DrawAvatar(*C, P.X, P.Y, R - 1.0f, Pic);
			if (Seat.IsRival)
			{
				C->StrokeArc(P.X, P.Y, R + 5.0f, 0.0f, 2.0f * Pi, Hex(0xb8324a), 3.0f);
			}
			if (Seat.Winner && HandOver)
			{
				C->StrokeArc(P.X, P.Y, R + 6.0f, 0.0f, 2.0f * Pi, Hex(rlsites_detail::SitesIgSun), 3.5f);
			}
			if (Seat.Acting)
			{
				// The fuse burning round the ring (from 360 degrees down to 0) with a spark at its head; it reddens under 5 s.
				const float Rf = R + 9.0f;
				C->StrokeArc(P.X, P.Y, Rf, 0.0f, 2.0f * Pi, Hex(rlsites_detail::SitesIgTrack), 5.0f);
				const float A1 = -Pi / 2.0f + 2.0f * Pi * static_cast<float>(Frac);
				if (Frac > 0.002)
				{
					C->StrokeArc(P.X, P.Y, Rf, -Pi / 2.0f, A1, Hex(Left < 5.0 ? rlsites_detail::SitesIgRed : rlsites_detail::SitesIgRope), 5.0f, true);
				}
				const float Hx = P.X + Rf * std::cos(A1);
				const float Hy = P.Y + Rf * std::sin(A1);
				C->FillCircle(Hx, Hy, 10.0f, TblA(rlsites_detail::SitesIgTang, 0.45f));
				rlsites_detail::SitesSpark(*C, Hx, Hy, 8.0f, Hex(rlsites_detail::SitesIgSun));
				rlsites_detail::SitesSpark(*C, Hx, Hy, 3.5f, Hex(0xffffff));
			}
			if (!Seat.Country.empty())
			{
				const Rect Fl{P.X + R * 0.42f, P.Y + R * 0.5f, 15.0f, 10.0f};
				C->FillRoundRect({Fl.X - 2.0f, Fl.Y - 2.0f, Fl.W + 4.0f, Fl.H + 4.0f}, 3.5f, Hex(0x1d1230));
				rlnet_detail::NetFlag(*C, Seat.Country, Fl.X, Fl.Y, Fl.W, Fl.H);
			}
			// The name pill and the stack ("36.5 bb"; the hero's rides in a wider pill).
			const float Py = P.Y + R + 8.0f;
			const std::string Stack = Seat.Stack <= 0 && Seat.AllIn ? std::string("ALL IN") : TblBbLower(Seat.Stack, Bb);
			TextStyle Fig = Ts(IsHero ? 17.0f : 20.0f, 900, IsHero ? Hex(rlsites_detail::SitesIgPlumInk) : Hex(rlsites_detail::SitesIgCream), IsHero ? Align::Right : Align::Center);
			Fig.Face = static_cast<int8_t>(Font::BlackItalic);
			const Color NameInk = Seat.IsRival ? Hex(0x8a1d33) : NameHover ? Hex(0x6a2d5e) : Hex(rlsites_detail::SitesIgPlumInk);
			if (IsHero)
			{
				UI.RRect({P.X - 92.0f, Py, 184.0f, 26.0f}, 13.0f, Hex(rlsites_detail::SitesIgCream));
				UI.Text(Seat.Name, P.X - 80.0f, Py + 18.0f, Ts(13.0f, 800, NameInk, Align::Left, Baseline::Alphabetic, false, 92.0f));
				UI.Text(Stack, P.X + 82.0f, Py + 19.5f, Fig);
			}
			else
			{
				const float Nw = std::min(Look.CardW, std::max(70.0f, UI.Measure(Seat.Name, 12.5f, 800) + 22.0f));
				UI.RRect({P.X - Nw / 2.0f, Py, Nw, 22.0f}, 11.0f, Hex(rlsites_detail::SitesIgCream));
				UI.Text(Seat.Name, P.X, Py + 15.5f, Ts(12.5f, 800, NameInk, Align::Center, Baseline::Alphabetic, false, Nw - 14.0f));
				// ALL IN stays cream like every stack (review fix: in Hot Pink it vanished into the neon tube at the capsule's ends,
				// and read 3.96:1 on the felt); the pink ALL IN sticker marks it.
				UI.Text(Stack, P.X, Py + 42.0f, Fig);
				if (!Hud.empty())
				{
					UI.Text(Hud, P.X, Py + 58.0f, Ts(11.0f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, true));
				}
			}
			if (S.T && S.T->Spec.BountyCents > 0 && !S.T->Spec.MysteryBounty)
			{
				const auto Head = S.Bounties.find(Seat.Id);
				const std::string Tag = Money(Head == S.Bounties.end() ? 0 : Head->second);
				const float Tw = UI.Measure(Tag, 12.0f, 800) + 16.0f;
				UI.RRect({P.X + R * 0.3f, P.Y - R - 10.0f, Tw, 22.0f}, 11.0f, Hex(0x3a1f08), pal::Orange);
				UI.Text(Tag, P.X + R * 0.3f + Tw / 2.0f, P.Y - R + 1.0f, Ts(12.0f, 800, pal::Orange, Align::Center, Baseline::Middle, true));
			}
			if (Labelled)
			{
				const float Tw = UI.Measure(Seat.HandLabel, 13.0f, 800) + 20.0f;
				const float Ly = IsHero ? Py + 30.0f : Py + 48.0f + (Hud.empty() ? 0.0f : 14.0f);
				UI.RRect({P.X - Tw / 2.0f, Ly, Tw, 22.0f}, 11.0f, Seat.Winner ? Hex(rlsites_detail::SitesIgSun) : TblA(rlsites_detail::SitesIgNight, 0.9f));
				UI.Text(Seat.HandLabel, P.X, Ly + 16.0f, Ts(13.0f, 800, Seat.Winner ? Hex(rlsites_detail::SitesIgPlumInk) : Hex(rlsites_detail::SitesIgCream), Align::Center));
			}
			if (Shown)
			{
				// The action as a starburst sticker over the bubble's left shoulder: CALL in sky, a raise in coral, ALL IN
				// bigger in pink, CHECK small (§2.5). A fold only dims the bubble.
				const std::string& Tag = Seat.LastAction;
				const size_t Sp = Tag.find(' ');
				const std::string Verb = rlnet_detail::NetUpper(Tag.substr(0, Sp));
				const std::string Amt = Sp == std::string::npos ? std::string() : Tag.substr(Sp + 1);
				const bool AllIn = Tag == "All-in";
				const float Rs = AllIn ? 30.0f : Tag == "Check" ? 20.0f : 26.0f;
				const Color Fill = AllIn ? Hex(rlsites_detail::SitesIgPink) : Tag == "Check" || StartsWith(Tag, "Call") ? Hex(rlsites_detail::SitesIgSky) : Hex(rlsites_detail::SitesIgCoral);
				TblSticker(*C, P.X - R * 0.95f, P.Y - R - 6.0f, Rs, Fill, AllIn ? std::string("ALL IN") : Verb, AllIn ? std::string() : Amt);
			}
			if (Seat.HasEquity && !Seat.Folded && S.CurHand && !S.CurHand->bComplete)
			{
				const double Eq = Seat.Equity;
				const float Ex = P.X + R + 46.0f;
				const float Ey = P.Y - R - 4.0f;
				UI.RRect({Ex - 36.0f, Ey - 18.0f, 72.0f, 30.0f}, 15.0f, TblA(rlsites_detail::SitesIgNight, 0.85f), Eq >= 0.5 ? Hex(rlsites_detail::SitesIgLime) : Hex(rlsites_detail::SitesIgRed), 1.5f);
				TextStyle Es = Ts(17.0f, 900, Eq >= 0.5 ? Hex(rlsites_detail::SitesIgLime) : Hex(rlsites_detail::SitesIgRed), Align::Center);
				Es.Face = static_cast<int8_t>(Font::BlackItalic);
				UI.Text(std::to_string(static_cast<int>(JsRound(Eq * 100.0))) + "%", Ex, Ey + 4.0f, Es);
			}
		}
		else
		{
			// RunItTwice's data card (§3.5): Carbon under a gloss top band, a chrome hairline, a square avatar, the name, the
			// stack in big blinds in BoldCondensed (the hero's twice-printed) with the chips in Mono, the HUD line on by default.
			const float X = Box.X;
			const float Y = Box.Y;
			const float W = Box.W;
			const float H = Box.H;
			const std::vector<Vec2> CardPath = rlsites_detail::SitesCutPath(Box, 6.0f);
			C->FillPolygon(rlsites_detail::SitesCutPath({X, Y + 4.0f, W, H}, 6.0f), Rgba(0, 0, 0, 0.5f));
			if (Seat.Winner && HandOver)
			{
				// The twin light under the winner's card.
				C->GlowRoundRect({X, Y + H + 1.0f, W, 10.0f}, 0.0f, TblA(rlsites_detail::SitesRitRed, 0.45f), 10.0f);
				C->FillRect({X, Y + H + 5.0f, W, 2.0f}, Hex(rlsites_detail::SitesRitRed));
				C->FillRect({X, Y + H + 5.5f, W, 1.0f}, Hex(rlsites_detail::SitesRitLed));
				C->FillRect({X, Y + H + 10.0f, W * 0.82f, 1.0f}, Hex(rlsites_detail::SitesRitRed));
				C->FillRect({X + W * 0.82f - 10.0f, Y + H + 5.0f, 12.0f, 2.0f}, Hex(0xffffff));
			}
			C->FillPolygon(CardPath, pal::Panel);
			C->FillPolygon(rlsites_detail::SitesCutPath({X, Y, W, 22.0f}, 6.0f), Paint::Linear({0.0f, Y}, {0.0f, Y + 22.0f}, Hex(rlsites_detail::SitesRitGloss), pal::Panel));
			const Color Edge = Seat.IsRival ? Hex(0xb8324a) : Seat.Acting ? pal::Accent : IsHero ? Rgba(255, 255, 255, 0.6f) : TblA(rlsites_detail::SitesRitChrome, 0.3f);
			C->StrokePolyline(CardPath, true, Edge, Seat.IsRival || Seat.Acting ? 1.5f : 1.0f);
			// Team RunItTwice's pros wear a 3-unit Signal Red top bar (their TEAM tab is RitTableLayer's).
			if (Known != S.FieldNpc.end())
			{
				const world::Npc* Who = S.Living().Get(Known->second);
				if (Who && !Who->Sponsor.empty() && Who->Sponsor == net::Def(net::SiteId::RunItTwice).TeamName)
				{
					C->FillPolygon({{X, Y}, {X + W - 6.0f, Y}, {X + W - 3.0f, Y + 3.0f}, {X, Y + 3.0f}}, pal::Accent);
				}
			}
			const float Sq = IsHero ? 42.0f : 36.0f;
			const float Ax = X + 11.0f;
			const float Ay = Y + 8.0f;
			C->FillRect({Ax, Ay, Sq, Sq}, Hex(rlsites_detail::SitesRitPool));
			C->PushClip({Ax, Ay, Sq, Sq});
			DrawAvatar(*C, Ax + Sq / 2.0f, Ay + Sq * 0.56f, Sq * 0.62f, Pic);
			C->PopClip();
			C->StrokePolyline({{Ax - 0.5f, Ay - 0.5f}, {Ax + Sq + 0.5f, Ay - 0.5f}, {Ax + Sq + 0.5f, Ay + Sq + 0.5f}, {Ax - 0.5f, Ay + Sq + 0.5f}}, true, TblA(rlsites_detail::SitesRitChrome, 0.8f), 1.0f);
			if (!Seat.Country.empty())
			{
				const Rect Fl{Ax + Sq - 13.0f, Ay + Sq - 8.0f, 15.0f, 10.0f};
				C->FillRect({Fl.X - 1.5f, Fl.Y - 1.5f, Fl.W + 3.0f, Fl.H + 3.0f}, pal::Panel);
				rlnet_detail::NetFlag(*C, Seat.Country, Fl.X, Fl.Y, Fl.W, Fl.H);
			}
			const float Tx = Ax + Sq + 10.0f;
			UI.Text(Seat.Name, Tx, Y + (IsHero ? 23.0f : 20.0f), Ts(14.0f, 700, Seat.IsRival ? Hex(0xff8da0) : NameHover ? pal::Accent2 : pal::Ink, Align::Left, Baseline::Alphabetic, false, X + W - Tx - 10.0f));
			const float Sb = Y + (IsHero ? 52.0f : 44.0f);
			const float Size = IsHero ? 28.0f : 24.0f;
			TextStyle Fig = Ts(Size, 700, IsHero ? pal::Gold : pal::Ink);
			Fig.Face = static_cast<int8_t>(Font::BoldCondensed);
			const bool Out = Seat.Stack <= 0 && Seat.AllIn;
			const double StackBb = static_cast<double>(Seat.Stack) / static_cast<double>(Bb);
			const std::string Big = Out ? std::string("ALL IN") : InBb ? Fixed(StackBb, StackBb < 100.0 ? 1 : 0) : ChipsText(static_cast<double>(Seat.Stack));
			if (IsHero)
			{
				// The twice print (§3.3): a Signal Red copy at (+2, +2) under the Hot White one.
				TextStyle Under = Fig;
				Under.Col = pal::Accent;
				UI.Text(Big, Tx + 2.0f, Sb + 2.0f, Under);
			}
			const float Bw2 = UI.Text(Big, Tx, Sb, Fig);
			if (!Out)
			{
				TextStyle Unit = Ts(IsHero ? 12.0f : 11.0f, 700, pal::Muted);
				Unit.Face = static_cast<int8_t>(Font::BoldCondensed);
				UI.Text(InBb ? "BB" : "CHIPS", Tx + Bw2 + (IsHero ? 6.0f : 4.0f), Sb, Unit);
				UI.Text(InBb ? ChipsText(static_cast<double>(Seat.Stack)) : Fixed(StackBb, StackBb < 100.0 ? 1 : 0) + " BB", X + W - 10.0f, Sb, Ts(12.0f, 500, pal::Muted, Align::Right, Baseline::Alphabetic, true));
			}
			if (!Hud.empty())
			{
				UI.Text(Hud, X + 11.0f, Y + H - 6.0f, Ts(10.5f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, true, W - 20.0f));
			}
			if (Seat.Acting)
			{
				// The clock: a 2-unit Signal Red line draining along the card's foot with an LED head, blinking at 2 Hz under 5 s.
				const bool Blink = Left < 5.0 && !Previewing && std::fmod(Now * 2.0, 1.0) >= 0.5;
				if (!Blink && Frac > 0.002)
				{
					const float Lw = (W - 2.0f) * static_cast<float>(Frac);
					C->FillRect({X + 1.0f, Y + H - 3.0f, Lw, 2.0f}, pal::Accent);
					C->FillRect({X + Lw - 2.0f, Y + H - 3.0f, 3.0f, 2.0f}, Hex(rlsites_detail::SitesRitLed));
				}
			}
			if (S.T && S.T->Spec.BountyCents > 0 && !S.T->Spec.MysteryBounty)
			{
				const auto Head = S.Bounties.find(Seat.Id);
				TblTag(*C, X + W - 70.0f, Y - 10.0f, Money(Head == S.Bounties.end() ? 0 : Head->second), pal::Orange, Hex(rlsites_detail::SitesRitOnyx), 12.0f, 18.0f);
			}
			if (Labelled)
			{
				// At a showdown the hand's name; the winner's on Signal Red (Hot White, 5.45:1). It sits under the card, past the
				// winner's twin light (review fix: over the card a long name, "TWO PAIR, NINES AND SEVENS", covered the shown cards
				// standing at the card's top right).
				TblTag(*C, X, Y + H + 14.0f, rlnet_detail::NetUpper(Seat.HandLabel), Seat.Winner ? pal::Accent : Hex(rlsites_detail::SitesRitTape), Seat.Winner ? pal::Gold : pal::Ink, 13.0f, 18.0f);
			}
			else if (Shown)
			{
				// The action as a slab tag over the card: a raise in Signal Red, a call in Chrome, a check in Carbon; its
				// amount in big blinds by default.
				const std::string& Tag = Seat.LastAction;
				const size_t Sp = Tag.find(' ');
				const bool AllIn = Tag == "All-in";
				std::string TagText = AllIn ? std::string("ALL IN") : rlnet_detail::NetUpper(Tag.substr(0, Sp));
				if (!AllIn && Sp != std::string::npos)
				{
					TagText += " " + (InBb ? rlsites_detail::SitesBbText(TblActionChips(Tag), Bb) : Tag.substr(Sp + 1));
				}
				const bool Hot = AllIn || StartsWith(Tag, "Raise") || StartsWith(Tag, "Bet");
				const bool Calls = StartsWith(Tag, "Call");
				TblTag(*C, X, Y - 22.0f, TagText, Hot ? pal::Accent : Calls ? pal::Accent2 : pal::Panel2, Hot ? pal::Gold : Calls ? Hex(rlsites_detail::SitesRitOnyx) : pal::Ink, 13.0f, 18.0f);
			}
		}
		// The one badge at the avatar's top right (next_segment_spec §5.6), as the plate pins it.
		{
			const int Npc = IsHero ? -1 : Known != S.FieldNpc.end() ? Known->second : -2;
			const uint8_t Frame = rlnet_detail::NetBadgeFrame(Pic);
			badge::Badge Pin;
			if (Npc >= -1)
			{
				if (const badge::Badge* B = badge::Top(rlnet_detail::NetBadgesOf(S, Npc), badge::Network, Frame))
				{
					Pin = *B;
				}
			}
			if (Pin.K != badge::Kind::Count)
			{
				const float Gx = Bubble ? P.X + (IsHero ? 38.0f : 32.0f) * 0.72f : Box.X + 11.0f + (IsHero ? 42.0f : 36.0f) - 2.0f;
				const float Gy = Bubble ? P.Y - (IsHero ? 38.0f : 32.0f) * 0.72f : Box.Y + 10.0f;
				C->FillCircle(Gx, Gy, 9.0f, Bubble ? Hex(0x1d1230) : pal::Panel);
				badgeart::Draw(*C, Pin, Gx, Gy, 13.0f, Now);
				if (Npc >= -1 && UI.Hover({Gx - 8.0f, Gy - 8.0f, 16.0f, 16.0f}))
				{
					rlnet_detail::NetTip& Tip = rlnet_detail::NetTipOf(this);
					Tip.Up = true;
					Tip.B = Pin;
					Tip.Npc = Npc;
					Tip.X = Gx;
					Tip.Y = Gy + 8.0f;
				}
			}
		}
		C->SetAlpha(A0);
		return;
	}

	// Cards.
	if (Seat.HasCards && !Seat.Folded)
	{
		const double Dealt = Now - Seat.DealtAt;
		if (IsHero && Seat.Hole.size() == 2)
		{
			const float Cw = 80.0f;
			const float Ch = 112.0f;
			for (size_t I = 0; I < 2; ++I)
			{
				CardOpts O;
				O.Flip = static_cast<float>(Clamp01((Dealt - 0.35 - static_cast<double>(I) * 0.1) / 0.25));
				O.Rotate = (static_cast<float>(I) - 0.5f) * 0.06f;
				O.Highlight = Seat.Winner;
				O.Back = Look.CardBack;
				DrawCard(*C, Seat.Hole[I], P.X - Cw - 4.0f + static_cast<float>(I) * (Cw + 8.0f), P.Y - Ph / 2.0f - Ch + 16.0f, Cw, Ch, O);
			}
		}
		else if (Seat.Hole.size() == 2)
		{
			const float Cw = 58.0f;
			const float Ch = 82.0f;
			for (size_t I = 0; I < 2; ++I)
			{
				CardOpts O;
				O.Highlight = Seat.Winner;
				O.Back = Look.CardBack;
				DrawCard(*C, Seat.Hole[I], P.X - Cw - 2.0f + static_cast<float>(I) * (Cw + 4.0f), P.Y - Ph / 2.0f - Ch + 14.0f, Cw, Ch, O);
			}
		}
		else if (Dealt > 0.4)
		{
			const float Cw = 44.0f;
			const float Ch = 62.0f;
			for (int I = 0; I < 2; ++I)
			{
				CardOpts O;
				O.Rotate = (static_cast<float>(I) - 0.5f) * 0.15f;
				O.Back = Look.CardBack;
				DrawCard(*C, -1, P.X - 30.0f + static_cast<float>(I) * 18.0f, P.Y - Ph / 2.0f - Ch + 18.0f, Cw, Ch, O);
			}
		}
	}
	else if (IsHero && Seat.Folded && S.AutoFolded && Now - S.AutoFoldedAt < 1.5)
	{
		for (size_t I = 0; I < S.AutoFoldedCards.size(); ++I)
		{
			CardOpts O;
			O.Alpha = static_cast<float>(1.0 - (Now - S.AutoFoldedAt) / 1.5);
			O.Dim = true;
			O.Back = Look.CardBack;
			DrawCard(*C, S.AutoFoldedCards[I], P.X - 84.0f + static_cast<float>(I) * 88.0f, P.Y - Ph / 2.0f - 100.0f, 80.0f, 112.0f, O);
		}
	}

	// Plate.
	C->SetAlpha(Dim ? 0.55f : 1.0f);
	if (Seat.Acting)
	{
		UI.RRectStroke({Plate.X - 4.0f, Plate.Y - 4.0f, Plate.W + 8.0f, Plate.H + 8.0f}, 16.0f, pal::Accent, 2.5f);
	}
	if (Seat.Winner && HandOver)
	{
		C->GlowRoundRect(Plate, 14.0f, rlnet_detail::NetA(pal::Gold, 0.9f), 24.0f);
		UI.RRect(Plate, 14.0f, Hex(0x2d2410));
	}
	const Paint PlateBg = Paint::Linear({0.0f, Plate.Y}, {0.0f, Plate.Y + Plate.H}, IsHero ? rlsites_detail::SitesTone(0x1b3552) : rlsites_detail::SitesTone(0x1a2536), IsHero ? rlsites_detail::SitesTone(0x10223a) : rlsites_detail::SitesTone(0x0f1726));
	const Color Edge = Seat.IsRival ? Hex(0xb8324a) : Seat.Winner && HandOver ? pal::Gold : Rgba(255, 255, 255, 0.1f);
	UI.RRect(Plate, 14.0f, PlateBg, Edge, Seat.IsRival ? 2.0f : 1.0f);
	// Avatar, with the country flag pinned to it.
	AvatarSpec Pic = AvatarFor(Seat.Name);
	if (IsHero)
	{
		Pic.Frame = AvatarFrame::Neon;
		Pic.Rim = rlnet_detail::NetRgb(pal::Accent);
	}
	else if (Seat.Pro)
	{
		Pic.Frame = AvatarFrame::Gold;
	}
	rlnet_detail::NetChampion(Pic, Seat.Name, IsHero);
	DrawAvatar(*C, Plate.X + 32.0f, P.Y, 21.0f, Pic);
	if (!Seat.Country.empty())
	{
		const Rect Fl{Plate.X + 40.0f, P.Y + 10.0f, 15.0f, 10.0f};
		C->FillRoundRect({Fl.X - 2.0f, Fl.Y - 2.0f, Fl.W + 4.0f, Fl.H + 4.0f}, 3.5f, rlsites_detail::SitesTone(0x0b1220));
		rlnet_detail::NetFlag(*C, Seat.Country, Fl.X, Fl.Y, Fl.W, Fl.H);
	}
	// Someone the world knows: their name opens their card.
	const auto Known = IsHero ? S.FieldNpc.end() : S.FieldNpc.find(Seat.Id);
	// One badge pinned at the avatar's top right (next_segment_spec §5.6), clear of the flag below it and the bounty tag
	// at the plate's corner; what the avatar's frame already says is left out (C2).
	{
		const int Npc = IsHero ? -1 : Known != S.FieldNpc.end() ? Known->second : -2;
		const uint8_t Frame = rlnet_detail::NetBadgeFrame(Pic);
		badge::Badge Pin;
		if (Npc >= -1)
		{
			if (const badge::Badge* B = badge::Top(rlnet_detail::NetBadgesOf(S, Npc), badge::Network, Frame))
			{
				Pin = *B;
			}
		}
		else if (Seat.Top.K != badge::Kind::Count)
		{
			// The session's pick for someone the world doesn't follow (it can't know the frame): the frame wins.
			const bool Said = (Frame == badge::FrameGold && Seat.Top.K == badge::Kind::TeamRiverLine) ||
				(Frame == badge::FrameChampion && (Seat.Top.K == badge::Kind::Bracelet || Seat.Top.K == badge::Kind::Ring));
			Pin = Said ? badge::Badge() : Seat.Top;
		}
		if (Pin.K != badge::Kind::Count)
		{
			const float Gx = Plate.X + 46.0f;
			const float Gy = P.Y - 16.0f;
			C->FillCircle(Gx, Gy, 9.0f, rlsites_detail::SitesTone(0x0b1220));
			badgeart::Draw(*C, Pin, Gx, Gy, 13.0f, Now);
			if (Npc >= -1 && UI.Hover({Gx - 8.0f, Gy - 8.0f, 16.0f, 16.0f}))
			{
				rlnet_detail::NetTip& Tip = rlnet_detail::NetTipOf(this);
				Tip.Up = true;
				Tip.B = Pin;
				Tip.Npc = Npc;
				Tip.X = Gx;
				Tip.Y = Gy + 8.0f;
			}
		}
	}
	bool NameHover = false;
	if (Known != S.FieldNpc.end())
	{
		const Ui::ClickState St = UI.Clickable("seatcard" + Seat.Id, Plate);
		NameHover = St.Hover;
		if (St.Clicked)
		{
			ShowPlayer(Known->second, Now);
		}
	}
	UI.Text(Seat.Name, Plate.X + 62.0f, P.Y - 6.0f, Ts(16.0f, 700, Seat.IsRival ? Hex(0xff8da0) : NameHover ? pal::Accent : pal::Ink, Align::Left, Baseline::Alphabetic, false, Pw - 72.0f));
	if (S.T && S.T->Spec.BountyCents > 0 && !S.T->Spec.MysteryBounty)
	{
		const auto Head = S.Bounties.find(Seat.Id);
		const std::string Tag = Money(Head == S.Bounties.end() ? 0 : Head->second);
		const float Tw = UI.Measure(Tag, 12.0f, 800) + 16.0f;
		UI.RRect({Plate.X + Pw - Tw - 6.0f, Plate.Y - 11.0f, Tw, 22.0f}, 11.0f, Hex(0x3a1f08), pal::Orange);
		UI.Text(Tag, Plate.X + Pw - Tw / 2.0f - 6.0f, Plate.Y + 0.5f, Ts(12.0f, 800, pal::Orange, Align::Center, Baseline::Middle, true));
	}
	const double Bb = S.CurHand ? static_cast<double>(S.CurHand->BigBlind) : 1.0;
	const std::string StackText = Seat.Stack <= 0 && Seat.AllIn ? std::string("ALL-IN") : ChipsText(static_cast<double>(Seat.Stack));
	UI.Text(StackText, Plate.X + 62.0f, P.Y + 19.0f, Ts(16.0f, 700, Seat.AllIn ? pal::Red : pal::Gold, Align::Left, Baseline::Alphabetic, true));
	if (Seat.Stack > 0)
	{
		const double InBB = static_cast<double>(Seat.Stack) / Bb;
		UI.Text(Fixed(InBB, InBB < 10.0 ? 1 : 0) + " BB", Plate.X + Pw - 12.0f, P.Y + 19.0f, Ts(13.0f, 500, pal::Muted, Align::Right));
	}
	C->SetAlpha(1.0f);

	// Timer bar.
	if (Seat.Acting)
	{
		const double Total = Seat.ActEnd - Seat.ActStart;
		const double Left = std::max(0.0, Seat.ActEnd - Now);
		double Frac = Total > 0.0 ? Left / Total : 0.0;
		UI.RRect({Plate.X + 8.0f, Plate.Y + Plate.H + 5.0f, Plate.W - 16.0f, 6.0f}, 3.0f, Rgba(0, 0, 0, 0.5f));
		const bool Bank = IsHero && S.HasPrompt && S.Prompt.TimeBankUntil > 0.0;
		const Color Col = Bank ? pal::Orange : Frac < 0.25 ? pal::Red : pal::Accent;
		if (Bank)
		{
			Frac = std::max(0.0, (S.Prompt.TimeBankUntil - Now) / std::max(1.0, S.TimeBank));
		}
		UI.RRect({Plate.X + 8.0f, Plate.Y + Plate.H + 5.0f, (Plate.W - 16.0f) * static_cast<float>(Frac), 6.0f}, 3.0f, Col);
	}

	// Last action tag.
	if (!Seat.LastAction.empty() && Now - Seat.LastActionAt < 30.0)
	{
		const std::string& Tag = Seat.LastAction;
		const Color Col = Tag == "Fold" ? pal::Dim : Tag == "Check" ? Hex(0x5f7fa6) : StartsWith(Tag, "Call") ? pal::Accent2 : Tag == "All-in" ? pal::Red : pal::Orange;
		const float Tw = UI.Measure(Tag, 14.0f, 800) + 20.0f;
		const float Ty = Plate.Y + Plate.H + (Seat.Acting ? 16.0f : 8.0f);
		UI.RRect({P.X - Tw / 2.0f, Ty, Tw, 24.0f}, 12.0f, Col);
		UI.Text(Tag, P.X, Ty + 17.0f, Ts(14.0f, 800, Hex(0xffffff), Align::Center));
	}

	// All-in equity (broadcast style).
	if (Seat.HasEquity && !Seat.Folded && S.CurHand && !S.CurHand->bComplete)
	{
		const double Eq = Seat.Equity;
		const float Ey = Plate.Y - 28.0f;
		const float Ex = P.X + (IsHero ? 128.0f : 100.0f);
		UI.RRect({Ex - 36.0f, Ey - 18.0f, 72.0f, 30.0f}, 8.0f, Rgba(0, 0, 0, 0.7f), Eq >= 0.5 ? pal::Green : pal::Red, 1.5f);
		UI.Text(std::to_string(static_cast<int>(JsRound(Eq * 100.0))) + "%", Ex, Ey + 4.0f, Ts(17.0f, 800, Eq >= 0.5 ? pal::Green : Hex(0xff9aa4), Align::Center, Baseline::Alphabetic, true));
	}
	if (!Seat.HandLabel.empty() && HandOver && !Seat.Folded)
	{
		const float Tw = UI.Measure(Seat.HandLabel, 13.0f, 700) + 16.0f;
		UI.RRect({P.X - Tw / 2.0f, Plate.Y - 20.0f, Tw, 22.0f}, 11.0f, Seat.Winner ? pal::Gold : Rgba(0, 0, 0, 0.7f));
		UI.Text(Seat.HandLabel, P.X, Plate.Y - 4.0f, Ts(13.0f, 700, Seat.Winner ? rlsites_detail::SitesOnGold() : pal::Ink, Align::Center));
	}

	// HUD.
	if (S.Hud && !IsHero)
	{
		const TPlayer* Pl = S.PlayerById(Seat.Id);
		if (Pl && Pl->Hands >= 3)
		{
			const int Vp = static_cast<int>(JsRound(static_cast<double>(Pl->VpipHands) / Pl->Hands * 100.0));
			const int Pf = static_cast<int>(JsRound(static_cast<double>(Pl->PfrHands) / Pl->Hands * 100.0));
			const std::string Hud = std::to_string(Vp) + "/" + std::to_string(Pf) + " \xC2\xB7 " + std::to_string(Pl->Hands) + "h";
			const Color Col = Vp >= 45 ? Hex(0xff9aa4) : Vp <= 14 ? Hex(0x8fb7ff) : pal::Muted;
			float Hy = Plate.Y - (Seat.HasCards && !Seat.Folded ? 72.0f : 8.0f);
			float Hx = P.X;
			Align Ha = Align::Center;
			if (Hy < RlTop + 20.0f)
			{
				// The seats along the top: beside their cards, toward the middle, clear of the top bar.
				Hy = Plate.Y - 30.0f;
				Hx = P.X + (P.X < Look.Cx ? 66.0f : -66.0f);
				Ha = P.X < Look.Cx ? Align::Left : Align::Right;
			}
			UI.Text(Hud, Hx, Hy, Ts(13.0f, 700, Col, Ha, Baseline::Alphabetic, true));
		}
	}
}

Vec2 RiverLine::FlightPoint(const FlightEnd& End) const
{
	// The felt's centre is the table's look's (update 3, §1.11).
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	switch (End.Type)
	{
	case FlightEnd::Kind::Pot: return Look.SeatList ? TblPotSpot(Look) : Vec2{Look.Cx, Look.Cy - 75.0f};
	case FlightEnd::Kind::Deck: return {Look.Cx, Look.Cy - 160.0f};
	case FlightEnd::Kind::Muck: return {Look.Cx, Look.Cy - 20.0f};
	case FlightEnd::Kind::Bet: return BetPos(End.Seat);
	default:
	{
		const Vec2 P = SlotPos(End.Seat);
		return {P.X, P.Y - 30.0f};
	}
	}
}

void RiverLine::DrawFlight(const Flight& F, double Now)
{
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	// Ignite (§2.5 "at showdown the chips fountain to the winner"; review-fix-ignite #3, g1-reaudit): a pot's chips on their way
	// to the winner are IgniteTableLayer's fountain, which it draws for every such flight unless previewing. The pot's stack
	// sliding along the straight line under it showed the chips twice and took the busiest frame (an all-in won in a bounty)
	// past §10's 22k. A preview's still keeps the stack.
	if (F.IsChips && F.Amount > 0 && F.From.Type == FlightEnd::Kind::Pot && F.To.Type == FlightEnd::Kind::Seat && !Previewing &&
		Look.Key == rlsites_detail::SitesLook(net::SiteId::Ignite).Key)
	{
		return;
	}
	// RunItTwice (§3.5 Bets, g1-audit L1): chips slide straight, no hop and no bounce, in its look's 160 ms (EaseSeconds),
	// eased out through the motion kit; its cards and every other look's flights are today's. (fix-rit's patch
	// fix-rit_to_fix-tablestyle_1, applied.)
	const bool Slide = F.IsChips && Look.Key == rlsites_detail::SitesLook(net::SiteId::RunItTwice).Key;
	// Ignite (§2.5 "Bets and pots", g1-audit L1): a bet's chips hop to the bet on a parabola and bounce once where they land,
	// over the flight and the 0.05 s Session keeps it after it (0.35 s, the spec's). Its other flights are today's.
	const bool Hop = F.IsChips && F.To.Type == FlightEnd::Kind::Bet && Look.Key == rlsites_detail::SitesLook(net::SiteId::Ignite).Key;
	const double Dur = Slide ? std::min(F.Dur, static_cast<double>(Look.EaseSeconds)) : Hop ? F.Dur + 0.05 : F.Dur;
	const double T = (Now - F.Start) / Dur;
	if (T < 0.0 || T > 1.0)
	{
		return;
	}
	const float K = static_cast<float>(Slide ? motion::CountUp(0.0, 1.0, Now, F.Start, Dur) : Hop ? std::min(1.0, T / 0.8) : EaseOutCubic(T));
	const Vec2 A = FlightPoint(F.From);
	const Vec2 B = FlightPoint(F.To);
	const float X = A.X + (B.X - A.X) * K;
	float Y = A.Y + (B.Y - A.Y) * K - std::sin(K * Pi) * 20.0f;
	if (Slide)
	{
		Y = A.Y + (B.Y - A.Y) * K;
	}
	if (Hop)
	{
		// The parabola (36 units high) lands at 80% of the hop; one 6-unit bounce takes the rest.
		const float V = static_cast<float>(std::max(0.0, (T - 0.8) / 0.2));
		Y = A.Y + (B.Y - A.Y) * K - (T < 0.8 ? 144.0f * K * (1.0f - K) : 24.0f * V * (1.0f - V));
	}
	if (F.IsChips)
	{
		DrawChipStack(*C, F.Amount, X, Y, 0.75f, static_cast<float>(1.0 - std::max(0.0, T - (Hop ? 0.92 : 0.85)) * (Hop ? 12.0 : 5.0)), Look.ChipSet);
	}
	else
	{
		CardOpts O;
		O.Rotate = K * 3.0f;
		O.Alpha = static_cast<float>(1.0 - std::max(0.0, T - 0.8) * 4.0);
		O.Back = Look.CardBack;
		DrawCard(*C, -1, X - 20.0f, Y - 28.0f, 40.0f, 56.0f, O);
	}
}

void RiverLine::GradeBadges(double Now)
{
	const SeatVis* Hero = nullptr;
	for (const SeatVis& Seat : S.Seats)
	{
		Hero = Seat.Present && Seat.IsHero ? &Seat : Hero;
	}
	if (!Hero)
	{
		return;
	}
	const Vec2 P = SlotPos(Hero->Seat);
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	// G1 (§0.6 #12): a rival's look puts the parent's alpha back (RiverLine's sets 1, as always).
	const float Alpha0 = Look.RestoreAlpha ? C->GetAlpha() : 1.0f;
	for (size_t I = 0; I < S.Badges.size(); ++I)
	{
		const GradeBadge& Bd = S.Badges[I];
		const double Age = Now - Bd.At;
		const double Alpha = std::min(1.0, Age * 5.0) * (1.0 - std::max(0.0, Age - 2.6) / 0.6);
		const float Y = P.Y - 70.0f - static_cast<float>(I) * 6.0f - static_cast<float>(EaseOutCubic(Age * 2.0)) * 30.0f;
		C->SetAlpha(Alpha0 * static_cast<float>(std::max(0.0, Alpha)));
		const float X = P.X + 150.0f;
		if (Look.SeatList)
		{
			// G1 fix (fix-tablestyle review; g1-audit M2, §9.6 "share no chrome"): a rival's grade in its own shape and face
			// (RunItTwice's cut slab in BoldCondensed caps, Ignite's pill in BlackItalic), its label in whichever of the dark ink
			// and Ink reads higher on the fill (§3.4: RunItTwice's Signal Red takes Ink, never the dark mix at 2.75:1).
			const Color Fill = GradeColor(Bd.G.Result);
			TextStyle St = Ts(17.0f, 800, rlsites_detail::SitesLabelInk(rlsites_detail::SitesTone(0x08101b), Fill, Fill), Align::Center, Baseline::Middle);
			St.Face = Look.TitleFace;
			const std::string Name = Look.LabelCaps ? rlnet_detail::NetUpper(GradeName(Bd.G.Result)) : std::string(GradeName(Bd.G.Result));
			const float Wd = GradeLabel(Bd.G.Result, Name, 0.0f, 0.0f, St, false) + 28.0f;
			if (Look.Cut > 0.0f)
			{
				C->FillPolygon(rlsites_detail::SitesCutPath({X, Y - 20.0f, Wd, 34.0f}, 6.0f), Fill);
			}
			else
			{
				UI.RRect({X, Y - 20.0f, Wd, 34.0f}, 17.0f, Fill);
			}
			GradeLabel(Bd.G.Result, Name, X + Wd / 2.0f, Y + 3.0f, St);
			C->SetAlpha(Alpha0);
			continue;
		}
		const TextStyle St = Ts(17.0f, 800, rlsites_detail::SitesTone(0x08101b), Align::Center, Baseline::Middle);
		const std::string Name = GradeName(Bd.G.Result);
		const float Wd = GradeLabel(Bd.G.Result, Name, 0.0f, 0.0f, St, false) + 28.0f;
		UI.RRect({X, Y - 20.0f, Wd, 34.0f}, 17.0f, GradeColor(Bd.G.Result));
		GradeLabel(Bd.G.Result, Name, X + Wd / 2.0f, Y + 3.0f, St);
		C->SetAlpha(Alpha0);
	}
	// Explanation of the latest graded decision under the action area.
	if (!S.Grades.empty() && !S.Badges.empty() && Now - S.Badges.back().At < 3.2 && !S.HasPrompt)
	{
		const DecisionGrade& Last = S.Grades.back();
		const std::vector<std::string> Lines = WrapLines(Last.Note, 520.0f, 16.0f);
		// RunItTwice (§3.3 "red is light, never ink"): a Good note in Chrome, not Signal Red, and a Blunder's in Ink (Signal Down
		// is for losses beside a minus); every other grade, and every other look, in the grade's colour as always.
		const bool Rit = Look.SeatList && Look.Cut > 0.0f;
		const Color NoteCol = Rit && Last.Result == Grade::Good ? pal::Accent2 : Rit && Last.Result == Grade::Blunder ? pal::Ink : GradeColor(Last.Result);
		for (size_t I = 0; I < Lines.size() && I < 2; ++I)
		{
			UI.Text(Lines[I], 1170.0f, 930.0f + static_cast<float>(I) * 24.0f, Ts(16.0f, 600, NoteCol, Align::Right));
		}
	}
}

// ------------------------------------------------------------------ controls

void RiverLine::Key(const std::string& K)
{
	if (S.CurrentScreen == Screen::Lobby && PageShown == Page::Lobby && (K == "ArrowUp" || K == "ArrowDown") && !Listed.empty())
	{
		const auto Found = std::find(Listed.begin(), Listed.end(), EventId);
		int I = Found == Listed.end() ? -1 : static_cast<int>(Found - Listed.begin());
		I = std::min(std::max(I + (K == "ArrowUp" ? -1 : 1), 0), static_cast<int>(Listed.size()) - 1);
		SelectEvent(Listed[static_cast<size_t>(I)]);
		return;
	}
	// Autoplay's keys (update 2 §2.6.1): K and Backspace, and an action key while the plan thinks (it takes the wheel).
	if (AppShown == App::RiverLine && rlauto_detail::AutoKey(this, S, K))
	{
		return;
	}
	if (!S.HasPrompt || rlauto_detail::AutoPanelOpen(this))
	{
		return;
	}
	HeroPrompt& P = S.Prompt;
	const std::string Lk = Lower(K);
	if (Lk == "f")
	{
		S.HeroAct(PlayerAction::Fold());
	}
	else if (Lk == "c" || Lk == "x")
	{
		S.HeroAct(P.CanCheck ? PlayerAction::Check() : PlayerAction::Call());
	}
	else if (Lk == "r" || Lk == "b")
	{
		S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(P.RaiseTo)));
	}
	else if (Lk == "a")
	{
		S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(P.MaxRaise)));
	}
	else if (K == "ArrowUp" || K == "ArrowDown")
	{
		// Fine steps as the card room has them (sizing::Step: a big blind under 20 BB, five above).
		const Chips To = sizing::Step(OnlineSpot(S), P.RaiseTo, K == "ArrowUp" ? 1 : -1);
		P.RaiseTo = MaxChips(P.MinRaise, MinChips(P.MaxRaise, To));
	}
}

double RiverLine::PotFrac(const HeroPrompt& P, double F) const
{
	const double Cur = S.CurHand ? static_cast<double>(S.CurHand->CurrentBet) : 0.0;
	return JsRound(Cur + static_cast<double>(P.Pot + P.ToCall) * F);
}

double RiverLine::PotRaise(const HeroPrompt& P) const
{
	const double Cur = S.CurHand ? static_cast<double>(S.CurHand->CurrentBet) : 0.0;
	return JsRound(Cur + static_cast<double>(P.Pot + P.ToCall));
}

void RiverLine::Controls(double Now)
{
	const float Ax = 640.0f;
	const float Aw = 1170.0f - Ax;

	// Pace and HUD (bottom-left): FULL · SMART · AUTO (update 2 §2.6.1). AUTO opens the autoplay panel; it's lit while the
	// plan holds or skips this table, and FULL or SMART hands the seat back.
	const int Front = S.FocusedTable();
	// On a stream (Kast's preview) the laptop shows none of it: nothing in the world ever says the plan played (U1).
	const autoplay::Panel Plan = Previewing ? autoplay::Panel() : S.AutoPanel(Front);
	const std::pair<Pace, const char*> Paces[2] = {{Pace::Full, "Full"}, {Pace::Smart, "Smart"}};
	// The table's look (update 3, §1.11). RiverLine's draws its controls as it always has (the else below); a rival's draws the
	// same controls at the same places and under the same ids in its own language (g1-audit M2; §2.5, §3.5, §9.6 "share no
	// chrome"): Ignite's candy pills, RunItTwice's cut slabs, each look's meter, amount box and slider.
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	const bool Styled = Look.SeatList;
	// A pace press: AUTO opens the autoplay panel; FULL or SMART takes the seat back from the plan.
	const auto PacePress = [&](int I, bool IsAuto, bool Active) {
		if (IsAuto)
		{
			rlauto_detail::AutoOpenPanel(this, S, Front, Now);
		}
		else if (!Active)
		{
			if (Plan.On)
			{
				autoplay::TableAuto Off = Plan.Now;
				Off.M = autoplay::Mode::Off;
				S.SetAutoplay(Front, Off);
			}
			S.CurrentPace = Paces[I].first;
		}
	};
	const double Tilt = S.HeroTilt;
	const float Comp = static_cast<float>(1.0 - Tilt);
	if (Styled)
	{
		// G1 (§3.4 "At a table"): RunItTwice's rail stays 64 wide at its tables, so its controls start right of it.
		const float Lx = Look.Cut > 0.0f ? 56.0f : 0.0f;
		const TextStyle Caption = TblFaced(Look, Look.Cut > 0.0f ? 13.0f : 14.0f, pal::Muted);
		UI.Text("PACE", 24.0f + Lx, 866.0f, Caption);
		for (int I = 0; I < 3; ++I)
		{
			const Rect R{24.0f + Lx + static_cast<float>(I) * 96.0f, 874.0f, 92.0f, 36.0f};
			const bool IsAuto = I == 2;
			const std::string Label = IsAuto ? std::string("AUTO") : std::string(Paces[I].second);
			const bool Active = IsAuto ? Plan.On || rlauto_detail::AutoPanelOpen(this) : !Plan.On && S.CurrentPace == Paces[I].first;
			if (rlsites_detail::SitesControl(UI, *C, Look, "pace" + Label, R, Label, Active, pal::Accent))
			{
				PacePress(I, IsAuto, Active);
			}
		}
		// The HUD (g1-audit L10: on by default where the look says so, RunItTwice's pro default; the session's switch elsewhere).
		const bool HudOn = rlsites_detail::SitesHudOn(*this, S, Look);
		if (rlsites_detail::SitesControl(UI, *C, Look, "hud", {24.0f + Lx, 918.0f, 130.0f, 30.0f}, HudOn ? "HUD ON" : "HUD OFF", HudOn, pal::Accent2))
		{
			rlsites_detail::SitesToggleHud(*this, S, Look);
		}
		if (rlsites_detail::SitesControl(UI, *C, Look, "lean", {164.0f + Lx, 918.0f, 148.0f, 30.0f}, "Lean back (Space)", false, pal::Accent))
		{
			LeanBackRequested = true;
		}
		// G1 (§3.5): a look that shows bets in big blinds (RunItTwice) toggles them to chips here (UI state, never saved).
		if (Look.BetsInBb)
		{
			const Rect BetR{322.0f + Lx, 918.0f, 128.0f, 30.0f};
			if (UI.Clickable("betunit", BetR).Clicked)
			{
				rlsites_detail::SitesToggleBets(*this);
			}
			const bool InBb = rlsites_detail::SitesBetsInBb(*this, Look);
			C->FillPolygon(rlsites_detail::SitesCutPath(BetR, 6.0f), pal::Panel);
			C->StrokePolyline(rlsites_detail::SitesCutPath(BetR, 6.0f), true, pal::Line, 1.0f);
			const Rect On{InBb ? BetR.X + 2.0f : BetR.X + 50.0f, BetR.Y + 2.0f, InBb ? 46.0f : 66.0f, BetR.H - 4.0f};
			C->FillRect(On, pal::Accent);
			// The lit half's label: whichever of the dark mix and Ink reads higher on the accent (§3.4).
			const Color Lit = rlsites_detail::SitesLabelInk(Mix(pal::Accent, Hex(0x000000), 0.86f), pal::Accent, pal::Accent);
			TextStyle Ux = Ts(13.0f, 700, InBb ? Lit : pal::Muted, Align::Center, Baseline::Middle);
			Ux.Face = static_cast<int8_t>(Font::BoldCondensed);
			UI.Text("BB", BetR.X + 25.0f, BetR.Y + BetR.H / 2.0f, Ux);
			Ux.Col = InBb ? pal::Muted : Lit;
			UI.Text("CHIPS", BetR.X + 83.0f, BetR.Y + BetR.H / 2.0f, Ux);
		}
		// Composure, in the look's own meter.
		UI.Text("COMPOSURE", 24.0f + Lx, 812.0f, Caption);
		const float Mx = 24.0f + Lx + UI.Measure("COMPOSURE", Caption.Size, static_cast<Font>(Caption.Face)) + 14.0f;
		TblComposure(*C, Look, Comp, Mx, 802.0f, 170.0f);
		if (Tilt > 0.3 && !S.SitOutNext)
		{
			ButtonOpts O;
			O.Kind = ButtonKind::Ghost;
			O.Size = 13.0f;
			if (rlsites_detail::SitesButton(UI, *C, "sitout", {24.0f + Lx, 824.0f, 258.0f, 26.0f}, "Step away: sit out next hand", O, Look))
			{
				S.RequestSitOut();
			}
		}
	}
	else
	{
		UI.Text("Pace", 24.0f, 866.0f, Ts(13.0f, 700, pal::Muted));
		for (int I = 0; I < 3; ++I)
		{
			const Rect R{24.0f + static_cast<float>(I) * 96.0f, 874.0f, 92.0f, 36.0f};
			const bool IsAuto = I == 2;
			const std::string Label = IsAuto ? std::string("AUTO") : std::string(Paces[I].second);
			const Ui::ClickState St = UI.Clickable("pace" + Label, R);
			const bool Active = IsAuto ? Plan.On || rlauto_detail::AutoPanelOpen(this) : !Plan.On && S.CurrentPace == Paces[I].first;
			UI.RRect(R, 9.0f, Active ? rlsites_detail::SitesTone(0x1f4d63) : St.Hover ? rlsites_detail::SitesTone(0x172a42) : pal::Panel, Active ? pal::Accent : pal::Line);
			UI.Text(Label, R.X + R.W / 2.0f, R.Y + 24.0f, Ts(15.0f, IsAuto ? 800 : 700, Active ? pal::Ink : pal::Muted, Align::Center));
			if (St.Clicked)
			{
				PacePress(I, IsAuto, Active);
			}
		}
		// RiverLine's look has no HUD default (g1-audit L10): SitesHudOn is Session::Hud here, as it always was.
		const Rect HudR{24.0f, 918.0f, 130.0f, 30.0f};
		if (UI.Clickable("hud", HudR).Clicked)
		{
			S.Hud = !S.Hud;
		}
		UI.RRect(HudR, 8.0f, S.Hud ? rlsites_detail::SitesTone(0x16354a) : pal::Panel, S.Hud ? pal::Accent : pal::Line);
		UI.Text(S.Hud ? "HUD ON" : "HUD OFF", HudR.X + HudR.W / 2.0f, HudR.Y + 21.0f, Ts(13.0f, 700, S.Hud ? pal::Ink : pal::Muted, Align::Center));
		const Rect LeanR{164.0f, 918.0f, 148.0f, 30.0f};
		if (UI.Clickable("lean", LeanR).Clicked)
		{
			LeanBackRequested = true;
		}
		UI.RRect(LeanR, 8.0f, pal::Panel, pal::Line);
		UI.Text("Lean back (Space)", LeanR.X + LeanR.W / 2.0f, LeanR.Y + 21.0f, Ts(13.0f, 700, pal::Muted, Align::Center));

		// Composure meter.
		UI.Text("Composure", 24.0f, 812.0f, Ts(13.0f, 700, pal::Muted));
		UI.RRect({112.0f, 802.0f, 170.0f, 10.0f}, 5.0f, rlsites_detail::SitesTone(0x0b1422), pal::Line);
		UI.RRect({112.0f, 802.0f, 170.0f * Comp, 10.0f}, 5.0f, Comp > 0.6f ? pal::Green : Comp > 0.35f ? pal::Gold : pal::Red);
		if (Tilt > 0.3 && !S.SitOutNext)
		{
			ButtonOpts O;
			O.Kind = ButtonKind::Ghost;
			O.Size = 13.0f;
			if (rlsites_detail::SitesButton(UI, *C, "sitout", {24.0f, 824.0f, 258.0f, 26.0f}, "Step away: sit out next hand", O))
			{
				S.RequestSitOut();
			}
		}
	}

	if (Plan.On && !S.Sprinting && S.HasPrompt)
	{
		// A hand taken back from the plan (K, or a stop rule): one line over the player's own controls.
		AutoStrip({712.0f, 752.0f, Ax + Aw - 712.0f, 26.0f}, Now);
	}
	if (!S.HasPrompt && Plan.On && !S.Sprinting)
	{
		// The plan holds the seat (§2.6.1): the watch strip where the player's buttons go.
		AutoStrip({Ax, 778.0f, Aw, 154.0f}, Now);
		return;
	}
	if (!S.HasPrompt)
	{
		// Waiting state.
		const bool NoteUp = !S.Grades.empty() && !S.Badges.empty() && Now - S.Badges.back().At < 3.2; // GradeBadges explains the last decision here
		if (S.CurHand && !S.CurHand->bComplete && !S.Sprinting && S.CurHand->ToAct >= 0 && !NoteUp)
		{
			const HandSeat& ToAct = S.CurHand->Seats[static_cast<size_t>(S.CurHand->ToAct)];
			if (ToAct.Id != HeroId)
			{
				const SeatVis& V = S.Seats[static_cast<size_t>(ToAct.Seat)];
				UI.Text("Waiting for " + (V.Present ? V.Name : std::string("\xE2\x80\xA6")), 1170.0f, 930.0f, Styled ? TblReadout(Look, Align::Right) : Ts(15.0f, 500, pal::Dim, Align::Right));
			}
		}
		return;
	}
	HeroPrompt& P = S.Prompt;
	const bool InBb = rlsites_detail::SitesBetsInBb(*this, Look);

	// Bet sizing: the presets for this spot from core's sizing (next_segment_spec §6.2 D), the same rule the card room
	// uses: MIN 2.2x 2.5x 3x POT ALL IN unopened with an ante, 1/3 to POT on an unbet street, and so on. Online chips
	// aren't rounded to a chip colour.
	if (P.CanRaise)
	{
		const sizing::Spot Spot = OnlineSpot(S);
		const std::vector<sizing::Preset> Presets = sizing::Presets(Spot);
		// Up to six buttons in the 370 units left of the amount box.
		const float Pitch = Presets.size() > 5 ? 62.0f : 72.0f;
		for (size_t I = 0; I < Presets.size(); ++I)
		{
			const Rect R{Ax + static_cast<float>(I) * Pitch, 786.0f, Pitch - 6.0f, 34.0f};
			const Chips Val = std::max(P.MinRaise, std::min(P.MaxRaise, Presets[I].To));
			ButtonOpts O;
			O.Kind = P.RaiseTo == Val ? ButtonKind::Primary : ButtonKind::Secondary;
			O.Size = Presets.size() > 5 ? 13.5f : 15.0f;
			// G1 (§2.5, §3.5 Action bar; review fix): in the table's look, Ignite's small pills and RunItTwice's cut slabs in caps
			// (RiverLine's look is the overload's own, so its presets don't move).
			if (rlsites_detail::SitesButton(UI, *C, "pre" + std::to_string(I), R, Presets[I].Label, O, Look))
			{
				P.RaiseTo = Val;
			}
		}
		const Rect Box{Ax + 376.0f, 786.0f, Aw - 376.0f, 34.0f};
		const double Step = std::max(1.0, JsRound(static_cast<double>(P.BigBlind) / 4.0));
		if (Styled)
		{
			// g1-audit M2: the amount box and the slider in the look's language. RunItTwice: a Tape Black box cut 6, the
			// amount in BoldCondensed Hot White (in big blinds where its bets are, the chips beside the size in Mono);
			// Ignite: a Night pill, the amount in BlackItalic cream.
			const bool Slab = Look.Cut > 0.0f;
			const std::string Figure = InBb ? rlsites_detail::SitesBbText(P.RaiseTo, std::max<Chips>(1, P.BigBlind)) : ChipsText(static_cast<double>(P.RaiseTo));
			if (Slab)
			{
				const std::vector<Vec2> Path = rlsites_detail::SitesCutPath(Box, 6.0f);
				C->FillPolygon(Path, Hex(rlsites_detail::SitesRitTape));
				C->StrokePolyline(Path, true, pal::Line, 1.0f);
			}
			else
			{
				UI.RRect(Box, Box.H / 2.0f, Hex(rlsites_detail::SitesIgNight), pal::Line);
			}
			TextStyle Fig = Ts(20.0f, 800, Slab ? pal::Gold : pal::Ink, Align::Right);
			Fig.Face = Look.NumberFace;
			UI.Text(Figure, Box.X + Box.W - 12.0f, Box.Y + 24.0f, Fig);
			std::string Detail = sizing::Detail(Spot, P.RaiseTo);
			const size_t Half = Detail.find(" \xC2\xB7 ");
			if (InBb)
			{
				Detail = (Half != std::string::npos ? Detail.substr(0, Half) + " \xC2\xB7 " : std::string()) + ChipsText(static_cast<double>(P.RaiseTo));
			}
			Detail = Detail.empty() ? Fixed(static_cast<double>(P.RaiseTo) / static_cast<double>(std::max<Chips>(1, P.BigBlind)), 1) + " BB" : Detail;
			const float DetailW = Box.W - 34.0f - UI.Measure(Figure, 20.0f, static_cast<Font>(Look.NumberFace));
			const size_t Cut = Detail.find(" \xC2\xB7 ");
			Detail = Cut != std::string::npos && UI.Measure(Detail, 12.0f, 600, Slab) > DetailW ? Detail.substr(0, Cut) : Detail;
			UI.Text(Detail, Box.X + 12.0f, Box.Y + 22.0f, Ts(12.0f, Slab ? 500 : 700, pal::Muted, Align::Left, Baseline::Alphabetic, Slab, DetailW));
			bool Held = false;
			bool& Drag = Previewing ? Held : rlsites_detail::SitesOf(this).RaiseDrag;
			P.RaiseTo = static_cast<Chips>(TblSlider(UI, *C, Look, Drag, "raise", {Ax + 10.0f, 826.0f, Aw - 20.0f, 26.0f}, static_cast<double>(P.RaiseTo), static_cast<double>(P.MinRaise), static_cast<double>(P.MaxRaise), Step));
		}
		else
		{
			UI.RRect(Box, 8.0f, rlsites_detail::SitesTone(0x0b1422), pal::Line);
			UI.Text(ChipsText(static_cast<double>(P.RaiseTo)), Box.X + Box.W - 12.0f, Box.Y + 24.0f, Ts(18.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
			// The size as a player reads it: "2.2x", "67% POT", or big blinds. When "2.5x · 23.3 BB" won't clear the
			// amount, the first half says it.
			std::string Detail = sizing::Detail(Spot, P.RaiseTo);
			Detail = Detail.empty() ? Fixed(static_cast<double>(P.RaiseTo) / static_cast<double>(P.BigBlind), 1) + " BB" : Detail;
			const float DetailW = Box.W - 34.0f - UI.Measure(ChipsText(static_cast<double>(P.RaiseTo)), 18.0f, 700, true);
			const size_t Half = Detail.find(" \xC2\xB7 ");
			Detail = Half != std::string::npos && UI.Measure(Detail, 12.5f, 600) > DetailW ? Detail.substr(0, Half) : Detail;
			UI.Text(Detail, Box.X + 10.0f, Box.Y + 23.0f, Ts(12.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, DetailW));
			P.RaiseTo = static_cast<Chips>(UI.Slider("raise", {Ax + 10.0f, 826.0f, Aw - 20.0f, 26.0f}, static_cast<double>(P.RaiseTo), static_cast<double>(P.MinRaise), static_cast<double>(P.MaxRaise), Step));
		}
	}

	// Main buttons (each may end the prompt, so re-check it), in the table's look (update 3, §1.11: its corners and height;
	// RiverLine's 66).
	const float Bw = (Aw - 20.0f) / 3.0f;
	const float By = 862.0f;
	ButtonOpts FoldO;
	FoldO.Kind = ButtonKind::Danger;
	FoldO.Hotkey = "F";
	const bool CanCheck = P.CanCheck;
	const bool CanRaise = P.CanRaise;
	const Chips ToCall = P.ToCall;
	const Chips RaiseTo = P.RaiseTo;
	const Chips MaxRaise = P.MaxRaise;
	const bool IsBet = P.IsBet;
	const Chips PotNow = P.Pot;
	// G1 (§2.5, §3.5): a rival's look draws its own pills or slabs (amounts in big blinds where its bets are); the presses and
	// what they do are the same.
	const auto Amount = [&](Chips V) { return InBb ? rlsites_detail::SitesBbText(V, std::max<Chips>(1, P.BigBlind)) : ChipsText(static_cast<double>(V)); };
	if (Styled ? TblActionButton(UI, *C, Look, "fold", {Ax, By, Bw, Look.ButtonH}, "Fold", std::string(), "F", TblRole::Fold)
			   : rlsites_detail::SitesButton(UI, *C, "fold", {Ax, By, Bw, Look.ButtonH}, "Fold", FoldO, Look))
	{
		S.HeroAct(PlayerAction::Fold());
	}
	Chips HeroStack = 0;
	if (S.CurHand)
	{
		for (const HandSeat& Hs : S.CurHand->Seats)
		{
			HeroStack = Hs.Id == HeroId ? Hs.Stack : HeroStack;
		}
	}
	const std::string CallLabel = CanCheck ? "Check" : ToCall >= HeroStack ? "Call all-in" : "Call";
	ButtonOpts CallO;
	CallO.Hotkey = "C";
	CallO.Sub = CanCheck ? std::string() : ChipsText(static_cast<double>(ToCall));
	if ((Styled ? TblActionButton(UI, *C, Look, "call", {Ax + Bw + 10.0f, By, Bw, Look.ButtonH}, CallLabel, CanCheck ? std::string() : Amount(ToCall), "C", TblRole::Call)
				: rlsites_detail::SitesButton(UI, *C, "call", {Ax + Bw + 10.0f, By, Bw, Look.ButtonH}, CallLabel, CallO, Look)) &&
		S.HasPrompt)
	{
		S.HeroAct(CanCheck ? PlayerAction::Check() : PlayerAction::Call());
	}
	if (CanRaise)
	{
		const bool AllIn = RaiseTo >= MaxRaise;
		ButtonOpts RaiseO;
		RaiseO.Kind = AllIn ? ButtonKind::Gold : ButtonKind::Primary;
		RaiseO.Sub = ChipsText(static_cast<double>(RaiseTo));
		RaiseO.Hotkey = "R";
		const std::string RaiseLabel = AllIn ? "All-in" : IsBet ? "Bet" : "Raise to";
		if ((Styled ? TblActionButton(UI, *C, Look, "raise", {Ax + (Bw + 10.0f) * 2.0f, By, Bw, Look.ButtonH}, RaiseLabel, Amount(RaiseTo), "R", AllIn ? TblRole::AllIn : TblRole::Raise)
					: rlsites_detail::SitesButton(UI, *C, "raise", {Ax + (Bw + 10.0f) * 2.0f, By, Bw, Look.ButtonH}, RaiseLabel, RaiseO, Look)) &&
			S.HasPrompt)
		{
			S.HeroAct(PlayerAction::RaiseTo(static_cast<double>(RaiseTo)));
		}
	}
	// Pot odds readout (Math skill, level 1).
	if (!CanCheck && ToCall > 0)
	{
		const double Odds = static_cast<double>(ToCall) / static_cast<double>(PotNow + ToCall);
		UI.Text("Call " + ChipsText(static_cast<double>(ToCall)) + " to win " + ChipsText(static_cast<double>(PotNow)) + " \xC2\xB7 you need " + std::to_string(static_cast<int>(JsRound(Odds * 100.0))) + "% equity", 1170.0f, 948.0f,
			Styled ? TblReadout(Look, Align::Right) : Ts(15.0f, 600, pal::Muted, Align::Right));
	}
}

// ------------------------------------------------------------------ side panel

void RiverLine::SidePanel(double Now)
{
	const Tournament& T = *S.T;
	const float X = RlSideX;
	const float Wd = RlW - X - 16.0f;
	// g1-audit M1 (§2.5, §3.5 Side panel, §9.6 "share no chrome"): a rival's table has a panel of its own under its own chrome
	// (Ignite's marquee, RunItTwice's tape and header), the same tabs' logic in its own language. RiverLine's is after it, as
	// it always was.
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	if (Look.SeatList)
	{
		const bool Rit = Look.Cut > 0.0f;
		const Rect Panel{X, Look.ChromeH + 12.0f, Wd, RlH - 38.0f - 12.0f - (Look.ChromeH + 12.0f)};
		const bool Live = S.Streaming();
		const RightTab Shown = !Live && S.Tab == RightTab::Stream ? RightTab::Chat : S.Tab;
		const bool LevelFlash = Now - S.LastLevelUpAt < 2.0;
		const float Blink = Previewing ? 1.0f : 0.6f + 0.4f * static_cast<float>(std::sin(Now * 4.0));
		if (Rit)
		{
			// RunItTwice (§3.5): one Carbon panel cut 10; tabs INFO, PAYOUTS, CHAT (and STREAM while live on Kast) in
			// BoldCondensed caps, the active one on Panel2 over the panel's one twin stripe (half scale: 4 and 1.5, gap 2), Hot
			// White. The LEAGUE tab is G2's, as the rail's LEAGUE plate.
			const std::vector<Vec2> Outline = rlsites_detail::SitesCutPath(Panel, 10.0f);
			C->FillPolygon(Outline, pal::Panel);
			const std::pair<RightTab, const char*> Tabs[4] = {{RightTab::Info, "Info"}, {RightTab::Payouts, "Payouts"}, {RightTab::Chat, "Chat"}, {RightTab::Stream, "Stream"}};
			const int TabCount = Live ? 4 : 3;
			const float TabW = Wd / static_cast<float>(TabCount);
			for (int I = 0; I < TabCount; ++I)
			{
				const Rect R{X + static_cast<float>(I) * TabW, Panel.Y, TabW, 40.0f};
				const Ui::ClickState St = UI.Clickable(std::string("tab") + Tabs[I].second, R);
				if (St.Clicked)
				{
					S.Tab = Tabs[I].first;
				}
				const bool Active = Shown == Tabs[I].first;
				if (Active)
				{
					if (I == TabCount - 1)
					{
						C->FillPolygon(rlsites_detail::SitesCutPath(R, 10.0f), pal::Panel2);
					}
					else
					{
						C->FillRect(R, pal::Panel2);
					}
					C->FillRect({R.X + 14.0f, R.Y + 29.0f, R.W - 28.0f, 4.0f}, pal::Accent);
					C->FillRect({R.X + 14.0f, R.Y + 35.0f, R.W - 28.0f, 1.5f}, pal::Accent);
				}
				UI.Text(rlnet_detail::NetUpper(Tabs[I].second), R.X + R.W / 2.0f, R.Y + 18.0f, TblFaced(Look, 13.0f, Active ? pal::Gold : St.Hover ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle));
				if (Tabs[I].first == RightTab::Stream)
				{
					C->FillRect({R.X + 12.0f, R.Y + 14.0f, 7.0f, 7.0f}, rlnet_detail::NetA(pal::Accent, Blink));
				}
			}
			C->FillRect({X, Panel.Y + 40.0f, Wd, 1.0f}, pal::Line);
			C->StrokePolyline(Outline, true, pal::Line, 1.0f);
			if (Shown == RightTab::Stream)
			{
				StreamSide({X, Panel.Y + 48.0f, Wd, Panel.H - 48.0f}, Now);
				return;
			}
			const Rect Body{X + 16.0f, Panel.Y + 52.0f, Wd - 32.0f, Panel.H - 52.0f - 14.0f};
			C->PushClip(Body);
			if (Shown == RightTab::Info)
			{
				// The ICM value from Structure's Icm(), started once a hand has ended and walked a frame's budget at a time (§3.5, §10;
				// g1-audit N2: no frame pays for a whole final table).
				TblRitInfo(*C, T, Body, rlsites_detail::SitesIcmCached(*this, T, Now), rlsites_detail::SitesBetsInBb(*this, Look), LevelFlash, T.LevelSecondsLeft());
			}
			else if (Shown == RightTab::Payouts)
			{
				TblRitPayouts(*C, T, Body);
			}
			else
			{
				TblRitChat(*C, S.Chat, Body);
			}
			C->PopClip();
			return;
		}
		// Ignite (§2.5): a rounded panel, the event's head with the blind clock as a fuse, candy tabs (the active one in the
		// accent with plum ink), chat bubbles, Payouts with medals, Standings with level rings.
		C->FillRoundRect(Panel, Look.PanelRadius, Paint::Linear({0.0f, Panel.Y}, {0.0f, Panel.Y + Panel.H}, pal::Panel, pal::Bg));
		C->StrokeRoundRect(Panel, Look.PanelRadius, pal::Line, 1.0f);
		TblIgHead(*C, T, {X, Panel.Y, Wd, 162.0f}, LevelFlash, T.LevelSecondsLeft());
		const std::pair<RightTab, const char*> Tabs[4] = {{RightTab::Chat, "Chat"}, {RightTab::Stream, "Stream"}, {RightTab::Payouts, "Payouts"}, {RightTab::Info, "Standings"}};
		const int TabCount = Live ? 4 : 3;
		const float Ty = Panel.Y + 172.0f;
		const float TabW = (Wd - 40.0f - 8.0f * static_cast<float>(TabCount - 1)) / static_cast<float>(TabCount);
		for (int I = 0, Slot = 0; I < 4; ++I)
		{
			if (!Live && Tabs[I].first == RightTab::Stream)
			{
				continue;
			}
			const Rect R{X + 20.0f + static_cast<float>(Slot++) * (TabW + 8.0f), Ty, TabW, 34.0f};
			const Ui::ClickState St = UI.Clickable(std::string("tab") + Tabs[I].second, R);
			if (St.Clicked)
			{
				S.Tab = Tabs[I].first;
			}
			const bool Active = Shown == Tabs[I].first;
			if (Active)
			{
				C->FillRoundRect(R, R.H / 2.0f, pal::Accent);
				C->FillRoundRect({R.X + 12.0f, R.Y + 3.0f, R.W - 24.0f, 8.0f}, 4.0f, Rgba(255, 255, 255, 0.2f));
			}
			else
			{
				C->FillRoundRect(R, R.H / 2.0f, St.Hover ? pal::Line : pal::Panel2);
			}
			TblFitText(*C, rlnet_detail::NetUpper(Tabs[I].second), R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f,
				TblFaced(Look, 13.0f, Active ? Hex(rlsites_detail::SitesIgPlumInk) : pal::Muted, Align::Center, Baseline::Middle), R.W - 14.0f);
			if (Tabs[I].first == RightTab::Stream)
			{
				C->FillCircle(R.X + 14.0f, R.Y + R.H / 2.0f, 4.0f, rlnet_detail::NetA(Hex(rlsites_detail::SitesIgPink), Blink));
			}
		}
		if (Shown == RightTab::Stream)
		{
			StreamSide({X, Ty + 44.0f, Wd, Panel.Y + Panel.H - (Ty + 44.0f)}, Now);
			return;
		}
		const Rect Body{X + 20.0f, Ty + 48.0f, Wd - 40.0f, Panel.Y + Panel.H - 16.0f - (Ty + 48.0f)};
		C->PushClip(Body);
		if (Shown == RightTab::Payouts)
		{
			TblIgPayouts(*C, T, Body);
		}
		else if (Shown == RightTab::Info)
		{
			rlignite_detail::IgniteTableStandings(*C, T, Body); // level rings (G1_AUDIT N5)
		}
		else
		{
			rlignite_detail::IgniteTableChat(*C, this, S.Chat, Body); // bubbles capped and cached (G1_AUDIT N3)
		}
		C->PopClip();
		return;
	}
	const Rect Card{X, RlTop + 14.0f, Wd, 300.0f};
	UI.RRect(Card, 14.0f, pal::Panel, pal::Line);
	UI.Text(T.Spec.Name, X + 18.0f, Card.Y + 34.0f, Ts(19.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, Wd - 120.0f));
	UI.Text("Table " + std::to_string(S.TableId), X + Wd - 18.0f, Card.Y + 34.0f, Ts(15.0f, 700, pal::Muted, Align::Right));
	const Level& Lvl = T.CurrentLevel();
	const int Secs = static_cast<int>(std::max(0.0, JsRound(T.LevelSecondsLeft())));
	const Color Flash = Now - S.LastLevelUpAt < 2.0 ? pal::Gold : pal::Ink;
	UI.Text("Level " + std::to_string(T.LevelIndex + 1), X + 18.0f, Card.Y + 68.0f, Ts(15.0f, 500, pal::Muted));
	UI.Text(ChipsText(static_cast<double>(Lvl.Sb)) + "/" + ChipsText(static_cast<double>(Lvl.Bb)) + " \xC2\xB7 ante " + ChipsText(static_cast<double>(Lvl.Ante)), X + Wd - 18.0f, Card.Y + 68.0f,
		Ts(17.0f, 700, Flash, Align::Right, Baseline::Alphabetic, true));
	UI.Text("Next level", X + 18.0f, Card.Y + 96.0f, Ts(15.0f, 500, pal::Muted));
	ArrowText(std::to_string(Secs / 60) + ":" + Pad2(Secs % 60), ChipsText(static_cast<double>(T.NextLevel().Sb)) + "/" + ChipsText(static_cast<double>(T.NextLevel().Bb)), X + Wd - 18.0f, Card.Y + 96.0f,
		Ts(15.0f, 600, pal::Ink, Align::Right, Baseline::Alphabetic, true));
	const double Avg = T.AverageStack();
	const Hand* Hh = S.CurHand.get();
	(void)Hh;
	struct Row
	{
		std::string K;
		std::string V;
		bool HasColor;
		Color Col;
	};
	std::vector<Row> Rows;
	Rows.push_back({"Players left", ChipsText(T.Remaining) + " / " + ChipsText(T.Spec.Entrants), false, pal::Ink});
	Rows.push_back({"Average stack", ChipsText(JsRound(Avg)) + " (" + std::to_string(static_cast<int>(JsRound(Avg / static_cast<double>(Lvl.Bb)))) + " BB)", false, pal::Ink});
	Rows.push_back({"Your rank", T.Hero().Busted ? std::string("\xE2\x80\x94") : Ordinal(T.HeroRank()) + " of " + ChipsText(T.Remaining), false, pal::Ink});
	if (T.InTheMoney())
	{
		const Chips Jump = T.PrizeFor(T.Remaining - 1) != 0 ? T.PrizeFor(T.Remaining - 1) : T.PrizeFor(1);
		Rows.push_back({"In the money", "next jump " + Money(Jump), true, pal::Green});
	}
	else
	{
		Rows.push_back({"To the money", ChipsText(T.Remaining - T.PaidPlaces()) + " players", T.HandForHand(), pal::Orange});
	}
	Rows.push_back({"Min cash / 1st", Money(T.Payouts.back()) + " / " + Money(T.Payouts.front()), false, pal::Ink});
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		const float Y = Card.Y + 134.0f + static_cast<float>(I) * 32.0f;
		UI.Text(Rows[I].K, X + 18.0f, Y, Ts(15.0f, 500, pal::Muted));
		UI.Text(Rows[I].V, X + Wd - 18.0f, Y, Ts(15.0f, 700, Rows[I].HasColor ? Rows[I].Col : pal::Ink, Align::Right));
	}

	// Tabs.
	// Live on Kast: the stream gets a tab of its own.
	const bool Live = S.Streaming();
	const RightTab Shown = !Live && S.Tab == RightTab::Stream ? RightTab::Chat : S.Tab;
	const std::pair<RightTab, const char*> Tabs[4] = {{RightTab::Chat, "Chat"}, {RightTab::Stream, "Stream"}, {RightTab::Payouts, "Payouts"}, {RightTab::Info, "Standings"}};
	const int TabCount = Live ? 4 : 3;
	const float Ty = Card.Y + Card.H + 14.0f;
	for (int I = 0, Slot = 0; I < 4; ++I)
	{
		if (!Live && Tabs[I].first == RightTab::Stream)
		{
			continue;
		}
		const float TabW = Wd / static_cast<float>(TabCount);
		const Rect R{X + static_cast<float>(Slot++) * TabW, Ty, TabW - 6.0f, 36.0f};
		const Ui::ClickState St = UI.Clickable(std::string("tab") + Tabs[I].second, R);
		if (St.Clicked)
		{
			S.Tab = Tabs[I].first;
		}
		const bool Active = Shown == Tabs[I].first;
		if (Active)
		{
			UI.RRect(R, 9.0f, pal::Panel2, pal::Line);
		}
		UI.Text(Tabs[I].second, R.X + R.W / 2.0f, R.Y + 24.0f, Ts(15.0f, 700, Active ? pal::Ink : pal::Muted, Align::Center));
		if (Tabs[I].first == RightTab::Stream)
		{
			C->FillCircle(R.X + 12.0f, R.Y + 18.0f, 4.0f, rlnet_detail::NetA(pal::Red, 0.6f + 0.4f * static_cast<float>(std::sin(Now * 4.0))));
		}
	}
	const Rect Box{X, Ty + 44.0f, Wd, RlH - 38.0f - (Ty + 44.0f) - 12.0f};
	if (Shown == RightTab::Stream)
	{
		StreamSide(Box, Now);
		return;
	}
	UI.RRect(Box, 14.0f, rlsites_detail::SitesTone(0x0c1524), pal::Line);
	C->PushClip(Box);
	if (Shown == RightTab::Chat)
	{
		// Wrap from the bottom up.
		float Y = Box.Y + Box.H - 14.0f;
		for (size_t Ii = S.Chat.size(); Ii-- > 0 && Y > Box.Y + 10.0f;)
		{
			const ChatLine& L = S.Chat[Ii];
			const Color Col = L.Kind == ChatKind::Dealer ? rlsites_detail::SitesTone(0x8fa0bb) : L.Kind == ChatKind::System ? pal::Gold : L.Kind == ChatKind::Rival ? Hex(0xff8da0) : pal::Ink;
			const Color WhoCol = L.Kind == ChatKind::Rival ? Hex(0xff5c7a) : L.Kind == ChatKind::Dealer ? Hex(0x5f6f88) : pal::Accent;
			const std::string Prefix = L.Who.empty() ? std::string() : L.Who + ": ";
			const std::vector<std::string> Wrapped = WrapLines(Prefix + L.Text, Box.W - 28.0f, 14.0f);
			for (size_t J = Wrapped.size(); J-- > 0 && Y > Box.Y + 10.0f;)
			{
				if (J == 0 && !Prefix.empty())
				{
					const float Pw = UI.Text(Prefix, Box.X + 14.0f, Y, Ts(14.0f, 700, WhoCol));
					UI.Text(Wrapped[0].size() > Prefix.size() ? Wrapped[0].substr(Prefix.size()) : std::string(), Box.X + 14.0f + Pw, Y, Ts(14.0f, 500, Col));
				}
				else
				{
					UI.Text(Wrapped[J], Box.X + 14.0f, Y, Ts(14.0f, 500, Col));
				}
				Y -= 20.0f;
			}
		}
	}
	else if (S.Tab == RightTab::Payouts)
	{
		float Y = Box.Y + 30.0f;
		for (const std::pair<int, int>& Band : PayoutBands(static_cast<int>(T.Payouts.size())))
		{
			if (Y > Box.Y + Box.H - 10.0f)
			{
				break;
			}
			const std::string Label = Band.first == Band.second ? Ordinal(Band.first) : Ordinal(Band.first) + "\xE2\x80\x93" + Ordinal(Band.second);
			const bool Reached = T.Remaining <= Band.second;
			UI.Text(Label, Box.X + 16.0f, Y, Ts(14.0f, 500, Reached ? pal::Ink : pal::Muted));
			UI.Text(Money(T.Payouts[static_cast<size_t>(Band.first - 1)]), Box.X + Box.W - 16.0f, Y, Ts(14.0f, 700, Reached ? pal::Gold : pal::Muted, Align::Right, Baseline::Alphabetic, true));
			Y += 22.0f;
		}
	}
	else
	{
		const std::vector<const TPlayer*> St = T.Standings();
		const int HeroRank = T.HeroRank();
		float Y = Box.Y + 30.0f;
		for (size_t I = 0; I < St.size() && I < 12; ++I)
		{
			const TPlayer& Pl = *St[I];
			UI.Text(std::to_string(I + 1) + ".", Box.X + 16.0f, Y, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(Pl.Name, Box.X + 56.0f, Y, Ts(14.0f, Pl.IsHero ? 800 : 500, Pl.IsHero ? pal::Accent : Pl.Name == RivalName ? Hex(0xff8da0) : pal::Ink, Align::Left, Baseline::Alphabetic, false, 190.0f));
			UI.Text(ChipsText(static_cast<double>(Pl.Stack)), Box.X + Box.W - 16.0f, Y, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
			Y += 22.0f;
		}
		if (!T.Hero().Busted && HeroRank > 12)
		{
			Y += 8.0f;
			UI.Text(std::to_string(HeroRank) + ".", Box.X + 16.0f, Y, Ts(14.0f, 500, pal::Muted, Align::Left, Baseline::Alphabetic, true));
			UI.Text(T.Hero().Name, Box.X + 56.0f, Y, Ts(14.0f, 800, pal::Accent));
			UI.Text(ChipsText(static_cast<double>(T.Hero().Stack)), Box.X + Box.W - 16.0f, Y, Ts(14.0f, 700, pal::Ink, Align::Right, Baseline::Alphabetic, true));
		}
	}
	C->PopClip();
}

// ------------------------------------------------------------------ overlays

void RiverLine::Overlays(double Now)
{
	// Over the felt's centre, the table's look's (update 3, §1.11).
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	// G1 (§0.6 #12): a rival's look puts the parent's alpha back (RiverLine's sets 1, as always).
	const float Alpha0 = Look.RestoreAlpha ? C->GetAlpha() : 1.0f;
	if (S.Moving)
	{
		const double Age = Now - S.MovingAt;
		const double A = std::min(1.0, Age * 4.0) * (1.0 - std::max(0.0, Age - 1.6) / 0.6);
		C->SetAlpha(Alpha0 * static_cast<float>(std::max(0.0, A)));
		if (Look.SeatList)
		{
			// g1-audit M2: a rival's own card, in its face (RunItTwice's caps; the move in its print, not a twice print: 26).
			TblOverlayCard(*C, Look, {Look.Cx - 230.0f, Look.Cy - 70.0f, 460.0f, 110.0f}, pal::Accent2);
			C->Text(Look.LabelCaps ? std::string("TABLE BALANCING") : std::string("Table balancing"), Look.Cx, Look.Cy - 30.0f, TblFaced(Look, 16.0f, pal::Muted, Align::Center));
			TblArrowLine(*C, Look, Look.LabelCaps ? "TABLE " + std::to_string(S.MovingFrom) : "Table " + std::to_string(S.MovingFrom),
				Look.LabelCaps ? "TABLE " + std::to_string(S.MovingTo) : "Table " + std::to_string(S.MovingTo), Look.Cx, Look.Cy + 14.0f, 26.0f, pal::Ink);
		}
		else
		{
			UI.RRect({Look.Cx - 230.0f, Look.Cy - 70.0f, 460.0f, 110.0f}, 16.0f, Rgba(8, 16, 27, 0.94f), pal::Accent2);
			UI.Text("Table balancing", Look.Cx, Look.Cy - 30.0f, Ts(17.0f, 500, pal::Muted, Align::Center));
			ArrowText("Table " + std::to_string(S.MovingFrom), "Table " + std::to_string(S.MovingTo), Look.Cx, Look.Cy + 12.0f, Ts(28.0f, 800, pal::Ink, Align::Center));
		}
		C->SetAlpha(Alpha0);
	}
	if (S.CurrentBanner.Active)
	{
		const Banner& B = S.CurrentBanner;
		const double Age = Now - B.At;
		const double A = std::min(1.0, Age * 3.0) * (1.0 - std::max(0.0, Age - 2.6) / 0.6);
		// RunItTwice's card settles without the overshoot (§3.3: "nothing bounces"); every other look's as always.
		const double Grow = std::min(1.0, Age * 2.5);
		const float Sc = static_cast<float>(0.9 + 0.1 * (Look.SeatList && Look.Cut > 0.0f ? EaseOutCubic(Grow) : EaseOutBack(Grow)));
		const Color Col = Hex(B.Color);
		C->Save();
		C->SetAlpha(Alpha0 * static_cast<float>(std::max(0.0, A)));
		C->Translate(Look.Cx, Look.Cy - 10.0f);
		C->Scale(Sc, Sc);
		if (Look.SeatList)
		{
			// g1-audit M2: a rival's banner in its own card and type, the session's colour as the skin's own token. RunItTwice
			// prints the title twice in caps (a Signal Red under-copy, then Hot White: red is light, never ink); Ignite's is
			// BlackItalic in the colour.
			const Color Own = TblSkinned(B.Color);
			TblOverlayCard(*C, Look, {-330.0f, -80.0f, 660.0f, 150.0f}, Own);
			if (Look.LabelCaps)
			{
				TextStyle Title = TblFaced(Look, 42.0f, pal::Accent, Align::Center);
				const std::string Caps = rlnet_detail::NetUpper(B.Title);
				C->Text(Caps, 3.0f, -9.0f, Title);
				Title.Col = pal::Gold;
				C->Text(Caps, 0.0f, -12.0f, Title);
				C->Text(B.Sub, 0.0f, 36.0f, Ts(20.0f, 500, pal::Ink, Align::Center));
			}
			else
			{
				C->Text(B.Title, 0.0f, -12.0f, TblFaced(Look, 42.0f, Own, Align::Center));
				C->Text(B.Sub, 0.0f, 36.0f, Ts(20.0f, 700, pal::Ink, Align::Center));
			}
		}
		else
		{
			UI.RRect({-330.0f, -80.0f, 660.0f, 150.0f}, 20.0f, Rgba(6, 10, 18, 0.92f), Col, 2.0f);
			UI.Text(B.Title, 0.0f, -12.0f, Ts(42.0f, 900, Col, Align::Center));
			UI.Text(B.Sub, 0.0f, 36.0f, Ts(20.0f, 600, pal::Ink, Align::Center));
		}
		C->Restore();
	}
	if (S.Sprinting && S.T)
	{
		// The skip card (update 2 §2.6.1): "SKIPPING · <STYLE>" over the dimmed table, where Sprint's card was. On a stream
		// the table only dims: nothing in the world ever says the plan played (U1).
		if (Previewing)
		{
			C->FillRect({0.0f, RlTop, RlSideX - 8.0f, RlH - RlTop}, Rgba(5, 9, 16, 0.78f));
		}
		else
		{
			SkipCard({0.0f, RlTop, RlSideX - 8.0f, RlH - RlTop}, Now);
		}
	}
}

// ------------------------------------------------------------------ results

void RiverLine::ResultsScreen(double Now)
{
	if (!S.HasResults)
	{
		return;
	}
	const Results& R = S.LastResults;
	const bool Seat = !R.SeatWon.empty();
	const bool Cashed = R.PrizeCents > 0 || R.BountyCents > 0 || Seat;
	const Color Col = R.Won || Seat ? pal::Gold : Cashed ? pal::Green : pal::Red;
	UI.Text(R.EventName, 60.0f, RlTop + 60.0f, Ts(20.0f, 600, pal::Muted));
	UI.Text(Seat ? std::string("Seat won.") : R.Won ? std::string("Champion.") : "You finished " + Ordinal(R.Place), 60.0f, RlTop + 130.0f, Ts(60.0f, 900, Col));
	UI.Text("of " + ChipsText(R.Entrants) + " players", 60.0f, RlTop + 172.0f, Ts(22.0f, 500, pal::Muted));
	if (Seat)
	{
		const std::string To = R.SeatWon == "rcop-main" ? "the RCOP Main Event" : R.SeatWon == "step2" ? "Step 2" : R.SeatWon == "step3" ? "Step 3" : R.SeatWon == "step4" ? "Step 4" : R.SeatWon;
		UI.Text("A " + Money(R.SeatValueCents) + " ticket to " + To, 60.0f, RlTop + 250.0f, Ts(40.0f, 800, pal::Gold, Align::Left, Baseline::Alphabetic, false, 1060.0f));
	}
	else if (R.PrizeCents > 0 || R.BountyCents > 0)
	{
		const float Pw = UI.Text("+" + Money(R.PrizeCents + R.BountyCents), 60.0f, RlTop + 250.0f, Ts(46.0f, 800, pal::Gold, Align::Left, Baseline::Alphabetic, true));
		if (R.BountyCents > 0)
		{
			UI.Text(Money(R.BountyCents) + " in bounties \xC2\xB7 " + std::to_string(R.Knockouts) + (R.Knockouts == 1 ? " knockout" : " knockouts"), 80.0f + Pw, RlTop + 248.0f, Ts(18.0f, 700, pal::Orange));
		}
	}
	else
	{
		UI.Text("No cash this time", 60.0f, RlTop + 250.0f, Ts(40.0f, 800, pal::Muted));
	}
	// Whose money it was (next_segment_spec §2.12): a package's roll takes the prize; buyers take their pieces of it.
	const std::string Split = ResultSplit(S, R);
	const float Shift = Split.empty() ? 0.0f : 30.0f;
	if (!Split.empty())
	{
		UI.Text(Split, 60.0f, RlTop + 290.0f, Ts(17.0f, 700, Hex(0xf2a93b), Align::Left, Baseline::Alphabetic, false, 1060.0f));
	}
	UI.Text("Buy-in " + (R.BuyInCents ? Money(R.BuyInCents) : std::string("free")) + " \xC2\xB7 " + std::to_string(R.Hands) + (R.Hands == 1 ? " hand" : " hands") + " \xC2\xB7 biggest pot won " + ChipsText(static_cast<double>(R.BiggestPot)), 60.0f, RlTop + 292.0f + Shift, Ts(17.0f, 500, pal::Muted));
	// A rival's prize lands on its wallet, never the bank (update 2 §5.5): the balance shown is the one the result moved.
	const net::SiteId PaidOn = rlsites_detail::SitesClient(*this, S);
	const std::string BalNow = PaidOn == net::SiteId::RiverLine ? "Balance now " + Money(S.BankrollCents)
															  : rlsites_detail::SitesShortName(PaidOn) + " balance now " + Money(rlsites_detail::SitesBalance(S, PaidOn));
	const float BalW = UI.Text(BalNow, 60.0f, RlTop + 330.0f + Shift, Ts(20.0f, 700, pal::Ink));
	if (R.SessionEvents > 1)
	{
		// Multi-tabling: the whole sitting, every buy-in against every prize.
		const std::string Net = (R.SessionNetCents >= 0 ? "+" : "") + Money(R.SessionNetCents);
		const float Lw = UI.Text("This sitting: " + std::to_string(R.SessionEvents) + " tournaments \xC2\xB7 net ", 84.0f + BalW, RlTop + 330.0f + Shift, Ts(17.0f, 600, pal::Muted));
		UI.Text(Net, 84.0f + BalW + Lw, RlTop + 330.0f + Shift, Ts(17.0f, 800, R.SessionNetCents >= 0 ? pal::Green : pal::Red, Align::Left, Baseline::Alphabetic, true));
	}

	// Accuracy gauge.
	const float Gx = 1240.0f;
	const float Gy = RlTop + 200.0f;
	const double Acc = R.AccuracyPct;
	const double Shown = Acc * (Now >= S.ResultsAt ? EaseOutCubic((Now - S.ResultsAt) / 1.4) : 1.0);
	C->StrokeArc(Gx, Gy, 110.0f, Pi * 0.75f, Pi * 2.25f, rlsites_detail::SitesTone(0x13213a), 18.0f, true);
	if (Shown > 0.1)
	{
		const Color GaugeCol = Acc >= 85.0 ? pal::Green : Acc >= 70.0 ? pal::Accent : Acc >= 55.0 ? pal::Gold : pal::Red;
		C->StrokeArc(Gx, Gy, 110.0f, Pi * 0.75f, Pi * 0.75f + (Pi * 1.5f * static_cast<float>(Shown)) / 100.0f, GaugeCol, 18.0f, true);
	}
	UI.Text(R.Grades.empty() ? std::string("\xE2\x80\x94") : Fixed(Acc, 1), Gx, Gy + 14.0f, Ts(52.0f, 900, pal::Ink, Align::Center, Baseline::Alphabetic, true));
	UI.Text("Decision accuracy", Gx, Gy + 60.0f, Ts(16.0f, 500, pal::Muted, Align::Center));
	// Update 2 (§2.9, §2.11): who played it. The accuracy is the player's own decisions only; the plan's are never graded.
	const autoplay::Played* PlanRec = rlauto_detail::AutoPlayedFor(S, R);
	const autoplay::Played* ByPlan = Previewing ? nullptr : PlanRec; // never on a stream (U1)
	if (ByPlan)
	{
		UI.Text(autoplay::PlayedLine(*ByPlan), Gx, Gy + 112.0f, Ts(15.0f, 700, pal::Accent, Align::Center, Baseline::Alphabetic, false, 2.0f * (RlW - Gx) - 40.0f));
	}
	// Skill XP is the player's own (U4, C1's copy pass): once the plan played any of the event, only the player's graded
	// decisions earn it (the cash was the plan's work too, so no cash bonus), and an event the plan played whole earns
	// none. A stream shows the same figure, without the line that names the plan.
	const double OwnXp = static_cast<double>(R.Grades.size()) * (Acc / 100.0) * 12.0;
	const int Xp = static_cast<int>(JsRound(OwnXp + (Cashed && !PlanRec ? 60.0 : 0.0)));
	if (PlanRec && R.Grades.empty())
	{
		UI.Text("No skill XP", Gx, Gy + 150.0f, Ts(22.0f, 800, pal::Muted, Align::Center));
	}
	else
	{
		UI.Text("Skill XP +" + std::to_string(Xp), Gx, Gy + 150.0f, Ts(22.0f, 800, pal::Accent, Align::Center));
	}
	UI.Text(ByPlan ? rlauto_detail::AutoXpOwn : "XP comes from decisions, not results.", Gx, Gy + 178.0f, Ts(14.0f, 500, pal::Muted, Align::Center));

	// Grade distribution.
	const Grade Order[5] = {Grade::Best, Grade::Good, Grade::Inaccuracy, Grade::Mistake, Grade::Blunder};
	int Counts[5] = {0, 0, 0, 0, 0};
	for (const DecisionGrade& G : R.Grades)
	{
		++Counts[static_cast<int>(G.Result)];
	}
	const float Total = static_cast<float>(R.Grades.empty() ? 1 : R.Grades.size());
	float Bx = 60.0f;
	const float By = RlTop + 390.0f;
	for (int I = 0; I < 5; ++I)
	{
		const float Wd = (static_cast<float>(Counts[I]) / Total) * 1000.0f;
		if (Wd > 0.0f)
		{
			UI.RRect({Bx, By, std::max(4.0f, Wd - 3.0f), 22.0f}, 5.0f, GradeColor(Order[I]));
		}
		Bx += Wd;
	}
	for (int I = 0; I < 5; ++I)
	{
		GradeLabel(Order[I], std::string(GradeName(Order[I])) + " " + std::to_string(Counts[I]), 60.0f + static_cast<float>(I) * 200.0f, By + 52.0f, Ts(16.0f, 700, GradeColor(Order[I])));
	}

	// Biggest mistakes.
	std::vector<DecisionGrade> Worst;
	for (const DecisionGrade& G : R.Grades)
	{
		if (G.Result != Grade::Best && G.Result != Grade::Good)
		{
			Worst.push_back(G);
		}
	}
	std::stable_sort(Worst.begin(), Worst.end(), [](const DecisionGrade& A, const DecisionGrade& B) { return A.LossBB > B.LossBB; });
	if (Worst.size() > 4)
	{
		Worst.resize(4);
	}
	UI.Text("Hand review", 60.0f, By + 110.0f, Ts(22.0f, 800));
	if (Worst.empty())
	{
		UI.Text(R.Grades.empty() ? (ByPlan ? rlauto_detail::AutoEveryDecision : "No decisions graded.") : "Clean game. No significant mistakes found.", 60.0f, By + 144.0f, Ts(17.0f, 500, pal::Muted));
	}
	for (size_t I = 0; I < Worst.size(); ++I)
	{
		const DecisionGrade& G = Worst[I];
		const float Y = By + 144.0f + static_cast<float>(I) * 56.0f;
		UI.RRect({60.0f, Y - 26.0f, 1000.0f, 48.0f}, 10.0f, pal::Panel, pal::Line);
		GradeLabel(G.Result, GradeName(G.Result), 78.0f, Y + 4.0f, Ts(16.0f, 800, GradeColor(G.Result)));
		UI.Text(std::string(StreetTitle(G.OnStreet)) + " \xC2\xB7 you chose " + Lower(G.Chosen.Label), 220.0f, Y + 4.0f, Ts(16.0f, 600));
		UI.Text(G.Note, 1045.0f, Y + 4.0f, Ts(15.0f, 500, pal::Muted, Align::Right, Baseline::Alphabetic, false, 480.0f));
	}

	ButtonOpts Back;
	Back.Kind = ButtonKind::Primary;
	if (rlsites_detail::SitesButton(UI, *C, "backlobby", {1120.0f, RlH - 120.0f, 420.0f, 70.0f}, "Back to lobby", Back))
	{
		S.LeaveResults();
	}
}

bool RiverLine::DebugOpen(const std::string& Args)
{
	// Update 3 (§7.6): ss.Laptop.Open <app> [page] [site], so a PIE shot never depends on where a button sits: "riverline lobby
	// ignite", "riverline league runittwice", "kast channel", "video studio", "burner sam". Names are case-insensitive; when
	// one names nothing (or a page the app hasn't got yet), nothing opens and it says so.
	std::vector<std::string> Words;
	for (size_t I = 0; I < Args.size();)
	{
		const size_t End = std::min(Args.find(' ', I), Args.size());
		if (End > I)
		{
			Words.push_back(Lower(Args.substr(I, End - I)));
		}
		I = End + 1;
	}
	if (Words.empty())
	{
		return false;
	}
	const double Now = LastFrame < 0.0 ? 0.0 : LastFrame;
	const std::string& Name = Words[0];
	const size_t More = Words.size() - 1;
	if (Name == "riverline" || Name == "poker")
	{
		static const std::pair<const char*, Page> OpenPages[] = {{"lobby", Page::Lobby}, {"series", Page::Series}, {"leaderboards", Page::Leaderboards}, {"boards", Page::Leaderboards},
			{"news", Page::News}, {"career", Page::Career}, {"embercrest", Page::Embercrest}, {"backing", Page::Backing}, {"promotions", Page::Promotions},
			{"cashier", Page::Cashier}, {"fuse", Page::Fuse}, {"profile", Page::Profile}, {"live", Page::Live}, {"league", Page::League}, {"desk", Page::Desk}};
		bool HasPage = false;
		bool HasSite = false;
		Page Want = Page::Lobby;
		net::SiteId Site = net::SiteId::RiverLine;
		for (size_t K = 1; K < Words.size(); ++K)
		{
			bool Known = false;
			for (const std::pair<const char*, Page>& P : OpenPages)
			{
				if (!HasPage && !HasSite && Words[K] == P.first)
				{
					Want = P.second;
					HasPage = Known = true;
				}
			}
			for (int X = 0; X < static_cast<int>(net::SiteId::Count) && !Known; ++X)
			{
				if (!HasSite && Words[K] == net::KeyOf(static_cast<net::SiteId>(X)))
				{
					Site = static_cast<net::SiteId>(X);
					HasSite = Known = true;
				}
			}
			if (!Known)
			{
				return false;
			}
		}
		if (HasSite)
		{
			SiteShown = Site; // the lobby shows it once the player holds an account there (rlsites_detail::SitesShown)
		}
		OpenApp(App::RiverLine, Now);
		if (HasPage || HasSite)
		{
			if (S.CurrentScreen != Screen::Boot)
			{
				S.ShowLobby();
			}
			OpenPage(Want, Now);
		}
		return true;
	}
	if (Name == "kast")
	{
		static const std::pair<const char*, KastPage> OpenKast[] = {{"studio", KastPage::Studio}, {"community", KastPage::Community}, {"channel", KastPage::Channel}, {"browse", KastPage::Browse}};
		const std::pair<const char*, KastPage>* Found = nullptr;
		for (const std::pair<const char*, KastPage>& P : OpenKast)
		{
			Found = More == 1 && Words[1] == P.first ? &P : Found;
		}
		if (More > 1 || (More == 1 && !Found))
		{
			return false;
		}
		if (Found)
		{
			ShowKastPage(Found->second);
		}
		OpenApp(App::Kast, Now);
		return true;
	}
	if (Name == "video" || Name == "rerun")
	{
		if (More >= 1 && Words[1] == "sheet")
		{
			// ss.Video.Sheet <group> <n> (Update3Commands.cpp opens "video sheet <group> <n>" after the session's answer):
			// ui-rerun's contact sheet of n generated thumbnails for a kind group, 24 when n is left out.
			int N = More >= 3 ? 0 : 24;
			for (const char Ch : More >= 3 ? Words[3] : std::string())
			{
				N = Ch >= '0' && Ch <= '9' ? std::min(N * 10 + (Ch - '0'), 999) : N;
			}
			return More <= 3 && rlvideo_detail::VideoOpenSheet(*this, More >= 2 ? Words[2] : std::string("beats"), N, Now);
		}
		const int P = More == 1 ? rlvideo_detail::VideoPageNamed(Words[1]) : 0;
		if (More > 1 || P < 0)
		{
			return false;
		}
		if (More == 1)
		{
			rlvideo_detail::VideoOpen(*this, P, Now);
		}
		else
		{
			OpenApp(App::Video, Now);
		}
		return true;
	}
	if (Name == "burner")
	{
		if (More > 1 || (More == 1 && Words[1] != "marcus" && Words[1] != "sam"))
		{
			return false;
		}
		if (More == 1)
		{
			ShowContact(Words[1] == "sam" ? 1 : 0);
		}
		OpenApp(App::Burner, Now);
		return true;
	}
	static const std::pair<const char*, App> OpenApps[] = {{"shiftlink", App::ShiftLink}, {"bank", App::Bank}, {"geardrop", App::GearDrop}, {"pennydrop", App::PennyDrop}, {"mail", App::Mail}};
	for (const std::pair<const char*, App>& A : OpenApps)
	{
		if (Name == A.first && More == 0)
		{
			OpenApp(A.second, Now);
			return true;
		}
	}
	return false;
}
} // namespace ui
} // namespace ss
