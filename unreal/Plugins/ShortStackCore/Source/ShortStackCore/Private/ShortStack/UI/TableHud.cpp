#include "ShortStack/UI/TableHud.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

// The table HUD, the table card and the room's clock screens (next_segment_spec §1.6, §6.3, §6.5). core-tableui.
//
// One logical canvas 1,080 tall; every panel anchors to an edge 58 units in and scales about that anchor with the HUD
// size setting. The room's palette is the Embercrest's (midnight navy, ash, ember); colour is never the only signal (a
// stack's depth is a colour and its number; the ALL IN rung says HOLD).

namespace ss
{
namespace ui
{
namespace tablehud_detail
{
using namespace frontend_detail;

const Color Ash = Hex(rlnet_detail::EcAsh);
const Color Ice = Hex(rlnet_detail::EcIce);
const Color Midnight = Hex(rlnet_detail::EcMidnight);
const Color Navy = Hex(rlnet_detail::EcNavy);
const Color Slate = Hex(rlnet_detail::EcSlate);
const Color Ember = Hex(rlnet_detail::EcEmber);
const Color Hot = Hex(rlnet_detail::EcEmberHot);
const Color Gold = Hex(rlnet_detail::EcGold);
const Color Flame = Hex(rlnet_detail::EcFlame);
const Color Teal = Hex(0x27d3c3);
const Color Green = Hex(0x3ecf6e);
const Color Red = Hex(0xef4d5a);
const float Margin = 58.0f;
const float Tall = 1080.0f;
const float PlateW = 188.0f;
const float LadderW = 340.0f;
const float CardW = 420.0f;

template <typename T>
float Fl(T V)
{
	return static_cast<float>(V);
}

Color Fa(Color C, float Alpha)
{
	C.A *= std::clamp(Alpha, 0.0f, 1.0f);
	return C;
}

std::string Up(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return S;
}

bool Dollars(Venue Where)
{
	return Where == Venue::BackRoom;
}

/** Chips as the table counts them: dollars at Dee's, tournament chips at the Embercrest. */
std::string Amount(Venue Where, Chips V)
{
	return Dollars(Where) ? (V < 0 ? "-$" + Grouped(-V) : "$" + Grouped(V)) : Grouped(V);
}

/** A prize pool or a payout in whole dollars ("$9,870"). */
std::string WholeDollars(Chips Cents)
{
	return "$" + Grouped((Cents + 50) / 100);
}

/** "14:32", "1:14:32". */
std::string Timer(double Seconds)
{
	const int S = std::max(0, static_cast<int>(std::floor(Seconds)));
	char Buf[32];
	if (S >= 3600)
	{
		std::snprintf(Buf, sizeof(Buf), "%d:%02d:%02d", S / 3600, (S / 60) % 60, S % 60);
	}
	else
	{
		std::snprintf(Buf, sizeof(Buf), "%d:%02d", S / 60, S % 60);
	}
	return Buf;
}

std::string Blinds(const Level& L)
{
	return Grouped(L.Sb) + " / " + Grouped(L.Bb) + (L.Ante > 0 ? " \xC2\xB7 ANTE " + Grouped(L.Ante) : std::string());
}

/** "≈ 24K", "≈ 8,500", "≈ 1.2M": a glance's estimate. */
std::string Approx(Chips V)
{
	std::string N;
	if (V >= 1000000)
	{
		N = Fixed(static_cast<double>(V) / 1000000.0, 1) + "M";
	}
	else if (V >= 10000)
	{
		N = Grouped((V + 500) / 1000) + "K";
	}
	else
	{
		N = Grouped((V + 50) / 100 * 100);
	}
	return "\xE2\x89\x88 " + N;
}

/** A stack's colour by its depth (§6.2 D): ash from 15 BB, ember gold under 15, ember hot under 8. */
Color Depth(Chips Stack, Chips Bb)
{
	if (Bb <= 0)
	{
		return Ash;
	}
	return Stack < 8 * Bb ? Hot : Stack < 15 * Bb ? Gold : Ash;
}

float Scale(const TableHudInfo& I)
{
	return std::clamp(I.Scale, 0.8f, 1.3f);
}

/** Draws inside a panel scaled about its anchor. */
struct Scaled
{
	Canvas& C;
	Scaled(Canvas& InC, float Ax, float Ay, float S) : C(InC)
	{
		C.Save();
		C.Translate(Ax, Ay);
		C.Scale(S, S);
		C.Translate(-Ax, -Ay);
	}
	~Scaled() { C.Restore(); }
	Scaled(const Scaled&) = delete;
	Scaled& operator=(const Scaled&) = delete;
};

float OverlapArea(const Rect& A, const Rect& B)
{
	const float W = std::min(A.X + A.W, B.X + B.W) - std::max(A.X, B.X);
	const float H = std::min(A.Y + A.H, B.Y + B.H) - std::max(A.Y, B.Y);
	return W > 0.0f && H > 0.0f ? W * H : 0.0f;
}

// ------------------------------------------------------------------ glyphs

/** The d-pad with one arm lit (UP, DOWN, LEFT, RIGHT), 28 square, its baseline at Y like Glyph. */
float DPadArm(Canvas& C, const std::string& Dir, float X, float Y, float Alpha)
{
	const float Top = Y - 21.0f;
	const float Cx = X + 14.0f;
	const float Cy = Top + 14.0f;
	const Color Rest = Rgba(238, 241, 246, 0.3f * Alpha);
	const Color Lit = Rgba(238, 241, 246, 0.95f * Alpha);
	C.FillRoundRect({Cx - 4.5f, Cy - 13.0f, 9.0f, 26.0f}, 2.0f, Paint(Rest));
	C.FillRoundRect({Cx - 13.0f, Cy - 4.5f, 26.0f, 9.0f}, 2.0f, Paint(Rest));
	if (Dir == "UP")
	{
		C.FillRoundRect({Cx - 4.5f, Cy - 13.0f, 9.0f, 10.0f}, 2.0f, Paint(Lit));
	}
	else if (Dir == "DOWN")
	{
		C.FillRoundRect({Cx - 4.5f, Cy + 3.0f, 9.0f, 10.0f}, 2.0f, Paint(Lit));
	}
	else if (Dir == "LEFT")
	{
		C.FillRoundRect({Cx - 13.0f, Cy - 4.5f, 10.0f, 9.0f}, 2.0f, Paint(Lit));
	}
	else
	{
		C.FillRoundRect({Cx + 3.0f, Cy - 4.5f, 10.0f, 9.0f}, 2.0f, Paint(Lit));
	}
	return 28.0f;
}

bool IsDPad(const std::string& Name)
{
	return Name == "UP" || Name == "DOWN" || Name == "LEFT" || Name == "RIGHT";
}

std::vector<std::string> Names(const char* Spaced)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char* P = Spaced ? Spaced : ""; *P; ++P)
	{
		if (*P == ' ')
		{
			if (!Cur.empty())
			{
				Out.push_back(Cur);
			}
			Cur.clear();
		}
		else
		{
			Cur.push_back(*P);
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

/** A keyboard key's box (frontend_detail::Glyph draws A, B, X and Y as the pad's face buttons). */
float KeyBox(Canvas& C, const std::string& Name, float X, float Y, float Alpha)
{
	const float Top = Y - 21.0f;
	const float Bw = std::max(28.0f, C.Measure(Name, 13.0f, 700) + 18.0f);
	C.StrokeRoundRect({X + 0.75f, Top + 0.75f, Bw - 1.5f, 26.5f}, 5.0f, Rgba(238, 241, 246, 0.55f * Alpha), 1.5f);
	C.Text(Name, X + Bw * 0.5f, Top + 14.0f, Ts(13.0f, 700, Rgba(238, 241, 246, 0.95f * Alpha), Align::Center, Baseline::Middle));
	return Bw;
}

bool IsFace(const std::string& Name)
{
	return Name == "A" || Name == "B" || Name == "X" || Name == "Y";
}

/** One button or key; on the pad the d-pad's arms are drawn as a d-pad, on the keyboard a letter is a key. */
float Key1(Canvas& C, const std::string& Name, bool Pad, float X, float Y, float Alpha)
{
	if (Pad && IsDPad(Name))
	{
		return DPadArm(C, Name, X, Y, Alpha);
	}
	if (!Pad && IsFace(Name))
	{
		return KeyBox(C, Name, X, Y, Alpha);
	}
	return Glyph(C, Name, X, Y, Alpha);
}

float Key1Width(Canvas& C, const std::string& Name, bool Pad)
{
	return Pad && IsDPad(Name) ? 28.0f : GlyphWidth(C, Name);
}

/** A control's glyphs (and HOLD for a held one), baseline at Y. Returns the width. */
float ControlGlyphs(Canvas& C, const Control& K, bool Pad, float X, float Y, float Alpha, bool DrawIt = true)
{
	float W = 0.0f;
	const std::vector<std::string> Parts = Names(Pad ? K.Pad : K.Key);
	for (size_t I = 0; I < Parts.size(); ++I)
	{
		W += (I > 0 ? 4.0f : 0.0f);
		W += DrawIt ? Key1(C, Parts[I], Pad, X + W, Y, Alpha) : Key1Width(C, Parts[I], Pad);
	}
	if (K.Hold)
	{
		W += 6.0f;
		if (DrawIt)
		{
			C.Text("HOLD", X + W, Y - 4.0f, Ts(13.0f, 700, Fa(Ice, 0.8f * Alpha)));
		}
		W += C.Measure("HOLD", 13.0f, 700);
	}
	return W;
}

/** A row of controls (glyphs and labels), left to right from X, baseline Y. Returns the width. */
float ControlRow(Canvas& C, ControlContext Ctx, const std::vector<std::string>& Ids, bool Pad, float X, float Y, float Alpha, bool DrawIt = true)
{
	float W = 0.0f;
	for (const std::string& Id : Ids)
	{
		const Control* K = FindControl(Ctx, Id);
		if (!K)
		{
			continue;
		}
		W += ControlGlyphs(C, *K, Pad, X + W, Y, Alpha, DrawIt) + 10.0f;
		W += DrawIt ? TrackedText(C, K->Label, X + W, Y - 2.0f, 13.0f, 700, Fa(MenuMuted, Alpha), 2.6f) : TrackedWidth(C, K->Label, 13.0f, 700, 2.6f);
		W += 30.0f;
	}
	return W > 0.0f ? W - 30.0f : 0.0f;
}

/** A small card (the HANDS page, the hero block, the last hand), top-left at X, Y. */
void MiniCard(Canvas& C, Card Cd, float X, float Y, float W, float Alpha)
{
	CardOpts O;
	O.Alpha = Alpha;
	if (Alpha < 0.999f || Cd < 0 || Cd > 51)
	{
		DrawCard(C, Cd, X, Y, W, W * 1.4f, O);
		return;
	}
	// Opaque cards are drawn once a size and replayed (the HANDS page shows seventy of them).
	const uint64_t Key = 0x4D494E4943415244ull ^ (static_cast<uint64_t>(Cd) << 20) ^ static_cast<uint64_t>(std::lround(W * 16.0f));
	C.Cached(Key, X, Y, [Cd, W, O](Canvas& Cv) { DrawCard(Cv, Cd, 0.0f, 0.0f, W, W * 1.4f, O); });
}

// ------------------------------------------------------------------ the panels' places

/** A long event name and the seat don't share the header's line: the seat goes under the name. */
bool TwoLineHeader(const TableHudInfo& I)
{
	return I.Clock && I.Clock->TableId > 0 && I.Event.size() > 16;
}

float ClockCardHeight(const TableHudInfo& I)
{
	if (!I.Clock)
	{
		return 0.0f;
	}
	const live::ClockFacts& F = *I.Clock;
	float H = 14.0f + 40.0f + (TwoLineHeader(I) ? 20.0f : 0.0f); // the header
	H += F.Funding.empty() ? 0.0f : 30.0f;
	const bool Pill = F.HandForHand || F.OnBreak || F.InTheMoney || !F.LateReg.empty();
	H += Pill ? 36.0f : 0.0f;
	H += 52.0f;         // the level and its clock
	H += 7.0f * 26.0f;  // the rows
	H += F.ColorUp.empty() ? 0.0f : 24.0f;
	return H + 12.0f;
}

struct Panels
{
	Rect Clock;
	bool HasClock = false;
	Rect Hero;
	Rect Ladder;
	Rect PotTag;
	bool HasPot = false;
};

Panels PanelsOf(const TableHudInfo& I, float W)
{
	const float S = Scale(I);
	Panels P;
	P.HasClock = I.Clock != nullptr && !I.Walking;
	P.Clock = {Margin, 52.0f, CardW * S, ClockCardHeight(I) * S};
	P.Hero = {Margin, Tall - Margin - 156.0f * S, 470.0f * S, 156.0f * S};
	P.Ladder = {W - Margin - LadderW * S, Tall - Margin - 330.0f * S, LadderW * S, 330.0f * S};
	P.HasPot = I.PotOnScreen && I.Pot > 0 && !I.Walking;
	P.PotTag = {I.PotAt.X - 110.0f * S, I.PotAt.Y - 18.0f * S, 220.0f * S, 36.0f * S};
	return P;
}

float PlateBodyH(const SeatPlate& P, StackReadout Readout)
{
	const bool Label = !P.Label.empty() || P.Away;
	if (Readout == StackReadout::Eyes)
	{
		return Label ? 54.0f : 36.0f;
	}
	return Label ? 78.0f : 60.0f;
}

float PlateAlpha(const TableHudInfo& I, const SeatPlate& P, double Now)
{
	const bool Strong = I.YourTurn || P.Looked || P.Acting || (Now - P.ActedAt >= 0.0 && Now - P.ActedAt < 2.0);
	float A = Strong ? 1.0f : 0.7f;
	A *= P.Folded || P.Away ? 0.55f : 1.0f;
	return A * std::clamp(P.Alpha, 0.0f, 1.0f);
}

/** Where the captions sit: their bottom edge and their widest (§1.6; above the lower third on the rail; the street's
 *  place on foot). */
struct CaptionPlace
{
	float Bottom = 904.0f;
	float MaxWidth = 980.0f;
};

CaptionPlace CaptionsAt(const TableHudInfo& I, float W)
{
	CaptionPlace P;
	if (I.Walking)
	{
		P.Bottom = I.OnFoot.Prompt.empty() ? Tall - 140.0f : Tall - 236.0f;
		P.MaxWidth = std::min(980.0f, W - 160.0f);
	}
	else
	{
		P.Bottom = I.OnAir ? Tall - Margin - 132.0f - 16.0f : 904.0f;
		P.MaxWidth = std::min(980.0f, W - 840.0f);
	}
	return P;
}

/** The room two caption boxes of two lines take (at the caption size), centred: plates keep out of it, so a line said
 *  never sits over a stack. Fixed, not the captions up this frame, so the plates don't move when someone talks. */
Rect CaptionZone(const TableHudInfo& I, float W)
{
	if (I.CaptionLook.Mode <= 0)
	{
		return {0.0f, 0.0f, 0.0f, 0.0f};
	}
	const CaptionPlace P = CaptionsAt(I, W);
	const float K = std::clamp(static_cast<float>(I.CaptionLook.Size), 80.0f, 200.0f) / 100.0f;
	const float H = 2.0f * (16.0f + 2.0f * 28.0f) * K + 8.0f * K;
	return {W * 0.5f - P.MaxWidth * 0.5f, P.Bottom - H, P.MaxWidth, H};
}

bool HasGlyph(const SeatPlate& P)
{
	return P.Glyph.K != badge::Kind::Count || P.BadgeCell >= 0;
}
} // namespace tablehud_detail

// ------------------------------------------------------------------ the plates' layout

std::vector<PlacedPlate> LayoutPlates(const TableHudInfo& I, float Width, double Now)
{
	using namespace tablehud_detail;
	const float S = Scale(I);
	const float W = Width;
	const float Pw = PlateW * S;
	const float TagH = 28.0f * S;
	const Panels P = PanelsOf(I, W);
	std::vector<Rect> Panel;
	if (P.HasClock)
	{
		Panel.push_back(P.Clock);
	}
	if (I.OnAir)
	{
		Panel.push_back({Margin, Tall - Margin - 132.0f, W - 2.0f * Margin, 132.0f}); // the lower third
	}
	else if (!I.Walking)
	{
		Panel.push_back(P.Hero);
		Panel.push_back(P.Ladder);
	}
	if (P.HasPot)
	{
		Panel.push_back(P.PotTag);
	}
	const Rect Said = CaptionZone(I, W);
	if (Said.W > 0.0f)
	{
		Panel.push_back(Said);
	}
	// The studying caption while it shows (StudyCaption: the name at 0.6 of the height, the note, the badge row, the
	// honorific): a studied face's stack under the zoomed lens gets a bottom-edge plate, which the captions' room pushes up
	// toward it.
	if (I.Study.Alpha > 0.01f && !I.Study.Name.empty())
	{
		Panel.push_back({W * 0.5f - 200.0f, Tall * 0.6f - 30.0f, 400.0f, 134.0f});
	}
	const float X0 = Margin;
	const float X1 = std::max(Margin, W - Margin - Pw);
	const float Y0 = 330.0f;
	const float Y1 = 760.0f;
	auto Clamp = [&](Rect R) {
		R.X = std::clamp(R.X, X0, X1);
		R.Y = std::clamp(R.Y, Y0, Y1);
		return R;
	};
	// The nearest seats first (lowest on the screen), then left to right: the same order every frame.
	std::vector<size_t> Order;
	for (size_t K = 0; K < I.Plates.size(); ++K)
	{
		if (!I.Plates[K].Hero)
		{
			Order.push_back(K);
		}
	}
	std::stable_sort(Order.begin(), Order.end(), [&I](size_t A, size_t B) {
		const SeatPlate& Pa = I.Plates[A];
		const SeatPlate& Pb = I.Plates[B];
		if (Pa.OnScreen != Pb.OnScreen)
		{
			return Pa.OnScreen;
		}
		return Pa.Y != Pb.Y ? Pa.Y > Pb.Y : Pa.X < Pb.X;
	});
	std::vector<PlacedPlate> Out;
	// What a spot covers: a face, the cards or the chips (KeepClear), the plates already down, the panels.
	auto Covers = [&I](const Rect& R) {
		float A = 0.0f;
		for (const Rect& Kc : I.KeepClear)
		{
			A += OverlapArea(R, Kc);
		}
		return A;
	};
	auto Crowds = [&Out](const Rect& R) {
		float A = 0.0f;
		for (const PlacedPlate& Done : Out)
		{
			A += OverlapArea(R, Done.Footprint);
		}
		return A;
	};
	auto Blocks = [&Panel](const Rect& R) {
		float A = 0.0f;
		for (const Rect& Pr : Panel)
		{
			A += OverlapArea(R, Pr);
		}
		return A;
	};
	for (const size_t K : Order)
	{
		const SeatPlate& Sp = I.Plates[K];
		const float Ph = PlateBodyH(Sp, I.Readout) * S;
		const float Fh = Ph + TagH;
		PlacedPlate Pl;
		Pl.Index = static_cast<int>(K);
		Pl.Alpha = PlateAlpha(I, Sp, Now);
		// The spots to try, the first preferred on a tie, and the point the plate wants to be by (its distance from there is
		// a cost too: drifting far from the stack reads as another seat's plate).
		std::vector<Rect> Cand;
		Vec2 Want = {Sp.X, Sp.Y};
		size_t PerStepIn = 0; // a side edge's candidates per step in from it: a step in costs more than sliding along
		if (!Sp.OnScreen)
		{
			// An edge plate: on the side the stack is off (behind the camera the host puts it at x 0 or W), or at the stack's
			// x on the bottom or top edge when it's further off below or above (a studied face's stack under the zoomed lens):
			// never the far side for a stack just under the screen. Slid along the edge, then in from it, off the faces.
			Pl.Edge = true;
			const float OffX = Sp.X < 0.0f ? -Sp.X : Sp.X > W ? Sp.X - W : 0.0f;
			const float OffY = Sp.Y < 0.0f ? -Sp.Y : Sp.Y > Tall ? Sp.Y - Tall : 0.0f;
			const bool Ends = OffY > OffX;
			Rect At;
			if (Ends)
			{
				Pl.TowardY = Sp.Y > Tall ? 1 : -1;
				At = Clamp({Sp.X - Pw * 0.5f, Pl.TowardY > 0 ? Y1 : Y0, Pw, Fh});
			}
			else
			{
				Pl.Toward = Sp.X < W * 0.5f ? -1 : 1;
				At = Clamp({Pl.Toward < 0 ? X0 : X1, Sp.Y - Fh * 0.5f, Pw, Fh});
			}
			Want = {At.X + Pw * 0.5f, At.Y + Fh * 0.5f};
			const float Along = Ends ? Pw * 0.5f + 4.0f * S : Fh * 0.5f + 3.0f * S;
			const float In = Ends ? Fh * 0.5f + 3.0f * S : Pw + 30.0f * S;
			const int Steps = Ends ? 12 : 16;
			PerStepIn = Ends ? 0 : static_cast<size_t>(Steps + 1);
			for (int Ring = 0; Ring < (Ends ? 4 : 3); ++Ring)
			{
				for (int Step = 0; Step <= Steps; ++Step)
				{
					// 0, +1, -1, +2, -2, ... steps along the edge.
					const float D = Fl((Step + 1) / 2) * (Step % 2 == 1 ? 1.0f : -1.0f) * Along;
					const float Back = Fl(Ring) * In;
					Cand.push_back(Ends ? Rect{At.X + D, At.Y - Back * Fl(Pl.TowardY), Pw, Fh} : Rect{At.X - Back * Fl(Pl.Toward), At.Y + D, Pw, Fh});
				}
			}
		}
		else
		{
			// By the stack: below it (toward the player), above it, beside it, each slid half a plate along, and a step
			// further out.
			const float Gap = 18.0f * S;
			const float Side = 26.0f * S;
			const float Far = 64.0f * S;
			Cand = {
				{Sp.X - Pw * 0.5f, Sp.Y + Gap, Pw, Fh},
				{Sp.X - Pw * 0.5f, Sp.Y - Gap - Fh, Pw, Fh},
				{Sp.X + Side, Sp.Y - Fh * 0.5f, Pw, Fh},
				{Sp.X - Side - Pw, Sp.Y - Fh * 0.5f, Pw, Fh},
			};
			for (const float Slide : {-0.5f, 0.5f})
			{
				Cand.push_back({Sp.X - Pw * 0.5f + Slide * Pw, Sp.Y + Gap, Pw, Fh});
				Cand.push_back({Sp.X - Pw * 0.5f + Slide * Pw, Sp.Y - Gap - Fh, Pw, Fh});
				Cand.push_back({Sp.X + Side, Sp.Y - Fh * 0.5f + Slide * Fh, Pw, Fh});
				Cand.push_back({Sp.X - Side - Pw, Sp.Y - Fh * 0.5f + Slide * Fh, Pw, Fh});
			}
			Cand.push_back({Sp.X - Pw * 0.5f, Sp.Y + Far, Pw, Fh});
			Cand.push_back({Sp.X - Pw * 0.5f, Sp.Y - Far - Fh, Pw, Fh});
		}
		// The spot that covers least wins: a face, the cards or the chips cost the most, then another plate, then a panel
		// (the ladder's area is only drawn on the player's turn), then the distance (and, by the stack, the order tried; off
		// a side edge, each step in from it: edge plates stack along their edge before one comes in).
		Rect Best;
		float BestScore = 1.0e30f;
		for (size_t C = 0; C < Cand.size(); ++C)
		{
			const Rect R = Clamp(Cand[C]);
			float Score = Sp.OnScreen ? Fl(C) * 6.0f : PerStepIn > 0 ? Fl(C / PerStepIn) * 60.0f : 0.0f;
			Score += Covers(R) * 100.0f + Crowds(R) * 40.0f + Blocks(R) * 3.0f;
			const float Dx = (R.X + Pw * 0.5f) - Want.X;
			const float Dy = (R.Y + Fh * 0.5f) - Want.Y;
			Score += std::sqrt(Dx * Dx + Dy * Dy) * 0.5f;
			if (Score < BestScore)
			{
				BestScore = Score;
				Best = R;
			}
		}
		// Overlaps left with the plates already down: the nearest free spot beside one of them (under it, over it, either
		// side; in line with this plate's spot or with that plate), another plate costing more than a face. A search, not a
		// nudge off the first plate hit: that bounced between two neighbours and could end where it began, two plates over
		// each other on one edge. Beside a plate leaves room for an edge plate's chevron.
		const float GapX = 30.0f * S;
		for (int Round = 0; Round < 3 && Crowds(Best) > 0.0f; ++Round)
		{
			const Rect From = Best;
			float Least = Crowds(From) * 1000.0f + Covers(From) * 100.0f + Blocks(From) * 3.0f;
			for (const PlacedPlate& Done : Out)
			{
				const Rect& F = Done.Footprint;
				const Rect Slots[8] = {
					{From.X, F.Y + F.H + 6.0f, Pw, Fh},
					{From.X, F.Y - Fh - 6.0f, Pw, Fh},
					{F.X + F.W + GapX, From.Y, Pw, Fh},
					{F.X - Pw - GapX, From.Y, Pw, Fh},
					{F.X, F.Y + F.H + 6.0f, Pw, Fh},
					{F.X, F.Y - Fh - 6.0f, Pw, Fh},
					{F.X + F.W + GapX, F.Y, Pw, Fh},
					{F.X - Pw - GapX, F.Y, Pw, Fh},
				};
				for (const Rect& Slot : Slots)
				{
					const Rect R = Clamp(Slot);
					const float Dx = R.X - From.X;
					const float Dy = R.Y - From.Y;
					const float Cost = Crowds(R) * 1000.0f + Covers(R) * 100.0f + Blocks(R) * 3.0f + std::sqrt(Dx * Dx + Dy * Dy) * 0.5f;
					if (Cost < Least)
					{
						Least = Cost;
						Best = R;
					}
				}
			}
			if (Best.X == From.X && Best.Y == From.Y)
			{
				break; // nowhere better
			}
		}
		Pl.Footprint = Best;
		Pl.Box = {Best.X, Best.Y, Pw, Ph};
		Pl.Glyph = {Best.X + 18.0f * S, Best.Y + 20.0f * S};
		Out.push_back(Pl);
	}
	return Out;
}

std::vector<GlyphSpot> HudGlyphs(const TableHudInfo& I, float Width, double Now)
{
	using namespace tablehud_detail;
	std::vector<GlyphSpot> Out;
	const float S = Scale(I);
	for (const PlacedPlate& Pl : LayoutPlates(I, Width, Now))
	{
		const SeatPlate& Sp = I.Plates[static_cast<size_t>(Pl.Index)];
		if (Sp.BadgeCell >= 0 && Sp.Glyph.K == badge::Kind::Count)
		{
			Out.push_back({Sp.BadgeCell, Pl.Glyph.X, Pl.Glyph.Y, 16.0f * S, Pl.Alpha});
		}
	}
	if (I.Study.Alpha > 0.01f && I.Study.Badges.empty() && !I.Study.BadgeCells.empty())
	{
		const size_t N = std::min<size_t>(3, I.Study.BadgeCells.size());
		const float Y = Tall * 0.6f + 62.0f;
		const float X0 = Width * 0.5f - (Fl(N) * 26.0f + Fl(N - 1) * 10.0f) * 0.5f + 13.0f;
		for (size_t K = 0; K < N; ++K)
		{
			Out.push_back({I.Study.BadgeCells[K], X0 + Fl(K) * 36.0f, Y, 26.0f, std::clamp(I.Study.Alpha, 0.0f, 1.0f)});
		}
	}
	return Out;
}

namespace tablehud_detail
{
// ------------------------------------------------------------------ the plates

void DrawPlate(Canvas& C, const TableHudInfo& I, const SeatPlate& Sp, const PlacedPlate& Pl, double Now)
{
	const float S = Scale(I);
	const float A = Pl.Alpha;
	const Rect B = Pl.Box;
	const Venue Where = I.Where;
	Scaled Sc(C, B.X, B.Y, S);
	const Rect R = {B.X, B.Y, PlateW, B.H / S};
	// What the plate says.
	const bool HasLabel = !Sp.Label.empty() || Sp.Away;
	const bool Counted = I.Readout == StackReadout::Room && Sp.Read.Exact && !Sp.AllIn;
	const std::string LabelText = Sp.Away ? std::string("AWAY") : Up(Sp.Label);
	std::string Big;
	std::string Small;
	Color BigCol = Ash;
	if (I.Readout != StackReadout::Eyes)
	{
		const Chips Bb = I.Bb;
		if (Sp.AllIn && Sp.Stack <= 0)
		{
			Big = "ALL IN";
			Small = Sp.Bet > 0 ? Amount(Where, Sp.Bet) : std::string();
			BigCol = Hot;
		}
		else if (I.Readout == StackReadout::Room && !Sp.Read.Exact)
		{
			const Chips Seen = Sp.Read.Shown;
			const std::string Bbs = Bb > 0 ? Grouped(Seen / Bb) + " BB" : std::string();
			if (I.Units == StackUnits::BigBlinds && !Dollars(Where) && !Bbs.empty())
			{
				Big = "\xE2\x89\x88 " + Bbs;
				Small = Approx(Seen).substr(4);
			}
			else
			{
				Big = Approx(Seen);
				Small = Bbs;
			}
		}
		else
		{
			const std::string Count = Amount(Where, Sp.Stack);
			const std::string Bbs = chipset::InBb(Sp.Stack, Bb);
			if (I.Units == StackUnits::BigBlinds && !Dollars(Where) && !Bbs.empty())
			{
				Big = Bbs;
				Small = Count;
			}
			else
			{
				Big = Count;
				Small = Bbs;
			}
			BigCol = Depth(Sp.Stack, Bb);
		}
	}
	const bool Glyphed = HasGlyph(Sp);
	const float H = R.H;
	// The body is drawn once for what it says and replayed at the plate's strength (plates change between hands, not frames).
	std::string Says = Sp.Name + '\x1f' + LabelText + '\x1f' + Big + '\x1f' + Small;
	Says += std::to_string(static_cast<int>(H)) + (Glyphed ? "g" : "") + (Sp.Button ? "b" : "") + (Counted ? "c" : "") + (Sp.Away ? "a" : "") + (HasLabel ? "l" : "");
	Says += std::to_string(static_cast<int>(BigCol.R * 255.0f)) + "," + std::to_string(static_cast<int>(BigCol.G * 255.0f));
	uint64_t Key = 0xcbf29ce484222325ull;
	for (const char Ch : Says)
	{
		Key = (Key ^ static_cast<unsigned char>(Ch)) * 0x100000001b3ull;
	}
	const std::string Name = Sp.Name;
	const bool Button = Sp.Button;
	const bool Away = Sp.Away;
	auto Body = [Name, LabelText, Big, Small, BigCol, Glyphed, Button, Counted, Away, HasLabel, H](Canvas& Cv) {
		const Rect L = {0.0f, 0.0f, PlateW, H};
		// The plate: midnight glass and a hairline.
		Cv.FillRoundRect(L, 10.0f, Paint(Fa(Midnight, 0.84f)));
		Cv.StrokeRoundRect({0.5f, 0.5f, L.W - 1.0f, L.H - 1.0f}, 10.0f, Fa(Ash, 0.14f), 1.0f);
		const float Nx = 12.0f + (Glyphed ? 20.0f : 0.0f); // the badge glyph's slot, left of the name
		float NameMax = L.W - 12.0f - Nx;
		if (Button)
		{
			// The button's disc, top right.
			const float Dx = L.W - 18.0f;
			Cv.FillCircle(Dx, 19.0f, 10.0f, Paint(Fa(Ash, 0.95f)));
			Cv.Text("D", Dx, 19.0f, Ts(13.0f, 900, Midnight, Align::Center, Baseline::Middle));
			NameMax -= 24.0f;
		}
		Cv.Text(Name, Nx, 26.0f, Ts(17.0f, 800, Ash, Align::Left, Baseline::Alphabetic, false, NameMax));
		float Y = 26.0f;
		if (HasLabel)
		{
			Y += 18.0f;
			const float Lmax = L.W - 24.0f - (Counted ? 76.0f : 0.0f);
			if (TrackedWidth(Cv, LabelText, 13.0f, 800, 1.6f) <= Lmax)
			{
				TrackedText(Cv, LabelText, 12.0f, Y, 13.0f, 800, Away ? Ice : Teal, 1.6f);
			}
			else
			{
				Cv.Text(LabelText, 12.0f, Y, Ts(13.0f, 800, Teal, Align::Left, Baseline::Alphabetic, false, Lmax));
			}
		}
		if (!Big.empty())
		{
			Y += 24.0f;
			Cv.Text(Big, 12.0f, Y, Ts(16.0f, 700, BigCol, Align::Left, Baseline::Alphabetic, true));
			if (!Small.empty())
			{
				const float Bw = Cv.Measure(Big, 16.0f, 700, true);
				Cv.Text(Small, L.W - 12.0f, Y, Ts(13.0f, 600, Ice, Align::Right, Baseline::Alphabetic, true, L.W - 36.0f - Bw));
			}
			if (Counted)
			{
				TrackedText(Cv, "COUNTED", L.W - 12.0f, 44.0f, 13.0f, 800, Gold, 1.4f, Align::Right);
			}
		}
	};
	{
		const float Was = C.GetAlpha();
		C.SetAlpha(Was * A);
		C.Cached(Key, R.X, R.Y, Body);
		C.SetAlpha(Was);
	}
	// The ember ring while the seat acts.
	if (Sp.Acting)
	{
		const float Pulse = 0.75f + 0.25f * static_cast<float>(std::sin(Now * 4.0));
		C.StrokeRoundRect({R.X - 1.0f, R.Y - 1.0f, R.W + 2.0f, R.H + 2.0f}, 11.0f, Fa(Hot, Pulse * A), 2.0f);
	}
	if (Glyphed && Sp.Glyph.K != badge::Kind::Count)
	{
		const float Was = C.GetAlpha();
		C.SetAlpha(A);
		badgeart::Draw(C, Sp.Glyph, R.X + 18.0f, R.Y + 20.0f, 16.0f, Now);
		C.SetAlpha(Was);
	}
	// The last action, under the plate, until the street ends.
	if (!Sp.LastAction.empty())
	{
		const std::string Act = Up(Sp.LastAction);
		std::string Tag = Act;
		const bool Sized = Act == "RAISE" || Act == "BET" || Act == "CALL" || Act == "ALL IN";
		if (Sized && Sp.LastTo > 0)
		{
			Tag += " " + Amount(Where, Sp.LastTo);
			if ((Act == "RAISE" || Act == "BET") && I.Bb > 0 && !Dollars(Where))
			{
				Tag += " \xC2\xB7 " + chipset::InBb(Sp.LastTo, I.Bb);
			}
		}
		const Color Col = Act == "FOLD" ? Hex(0x4d5b73) : Act == "CHECK" ? Hex(0x5f7fa6) : Act == "CALL" ? Hex(0x3b82f6) : Act == "ALL IN" ? Red : Hex(0xf28a3a);
		const float In = Ease((Now - Sp.ActedAt) / 0.2);
		const float Tw = std::min(R.W, C.Measure(Tag, 13.0f, 800) + 22.0f);
		const float Ty = R.Y + R.H + 5.0f;
		C.FillRoundRect({R.X + R.W * 0.5f - Tw * 0.5f, Ty, Tw, 22.0f}, 11.0f, Paint(Fa(Col, 0.92f * A * (Sp.ActedAt < -50.0 ? 1.0f : In))));
		C.Text(Tag, R.X + R.W * 0.5f, Ty + 16.0f, Ts(13.0f, 800, Fa(Hex(0xffffff), A), Align::Center, Baseline::Alphabetic, false, R.W - 16.0f));
	}
	// An edge plate points at its seat: beside it, toward the side the seat is off, or down (up) at a stack off the bottom
	// (top) of the screen.
	if (Pl.Edge && Pl.TowardY != 0)
	{
		const float Cx = R.X + R.W + 14.0f;
		const float Cy = R.Y + R.H * 0.5f;
		const float D = Pl.TowardY > 0 ? 1.0f : -1.0f;
		C.StrokePolyline({{Cx - 9.0f, Cy - D * 3.6f}, {Cx, Cy + D * 4.5f}, {Cx + 9.0f, Cy - D * 3.6f}}, false, Fa(Ash, 0.9f * A), 2.2f, true);
	}
	else if (Pl.Edge)
	{
		const float Cx = Pl.Toward < 0 ? R.X - 12.0f : R.X + R.W + 12.0f;
		Chevron(C, Cx, R.Y + R.H * 0.5f, 9.0f, Pl.Toward > 0, Fa(Ash, 0.9f * A));
	}
}

// ------------------------------------------------------------------ the clock card

/** FNV-1a, 64 bits: a cached panel's key from everything it says. */
uint64_t KeyOf(const std::string& S)
{
	uint64_t Key = 0xcbf29ce484222325ull;
	for (const char Ch : S)
	{
		Key = (Key ^ static_cast<unsigned char>(Ch)) * 0x100000001b3ull;
	}
	return Key;
}

/** The card's words, row by row (the cache's key is made of them). */
struct CardWords
{
	std::string Event, Seat, Funding, Pill, Level, Ticking, ColorUp;
	Color PillCol = Teal;
	bool Beats = false;
	std::pair<std::string, std::string> Rows[7];
	Color MoneyCol = Ash;
};

CardWords WordsOf(const TableHudInfo& I)
{
	const live::ClockFacts& F = *I.Clock;
	CardWords Out;
	Out.Event = I.Event.empty() ? std::string("THE EMBERCREST") : Up(I.Event);
	Out.Seat = F.TableId > 0 ? "TABLE " + std::to_string(F.TableId) + " \xC2\xB7 SEAT " + std::to_string(F.Seat) : std::string();
	Out.Funding = F.Funding;
	if (F.HandForHand)
	{
		Out.Pill = "HAND FOR HAND \xC2\xB7 " + std::to_string(F.ToTheMoney) + " FROM THE MONEY";
		Out.PillCol = Hot;
		Out.Beats = true;
	}
	else if (F.OnBreak)
	{
		Out.Pill = "BREAK \xC2\xB7 " + Timer(F.BreakSecondsLeft);
		Out.PillCol = Gold;
	}
	else if (F.InTheMoney)
	{
		Out.Pill = F.NextJumpAt > 0 ? "IN THE MONEY \xC2\xB7 NEXT JUMP AT " + std::to_string(F.NextJumpAt) + " LEFT +" + WholeDollars(F.NextJumpCents) : std::string("IN THE MONEY");
		Out.PillCol = Green;
	}
	else if (!F.LateReg.empty())
	{
		Out.Pill = F.LateReg;
		Out.PillCol = F.LateReg.rfind("LAST", 0) == 0 ? Hot : Ice;
	}
	Out.Level = F.OnBreak ? std::string("BREAK") : "LEVEL " + std::to_string(F.Level);
	Out.Ticking = Timer(F.OnBreak ? F.BreakSecondsLeft : F.LevelSecondsLeft);
	Out.ColorUp = F.ColorUp;
	const std::string Avg = Grouped(F.Average) + (F.Now.Bb > 0 ? " \xC2\xB7 " + chipset::InBb(F.Average, F.Now.Bb) : std::string());
	const std::string Pool = WholeDollars(F.PoolCents) + " \xC2\xB7 " + std::to_string(F.Paid) + " PAID" + (F.Projected ? " \xC2\xB7 PROJECTED" : "");
	std::string Money;
	if (F.InTheMoney)
	{
		Money = F.NextJumpAt > 0 ? "NEXT JUMP AT " + std::to_string(F.NextJumpAt) + " LEFT \xC2\xB7 +" + WholeDollars(F.NextJumpCents) : std::string("IN THE MONEY");
		Out.MoneyCol = Green;
	}
	else
	{
		Money = std::to_string(F.ToTheMoney) + " FROM THE MONEY \xC2\xB7 MIN CASH " + WholeDollars(F.MinCashCents);
		Out.MoneyCol = F.HandForHand ? Hot : Ash;
	}
	const std::string Brk = F.BreakBeforeLevel > 0 ? "BEFORE LEVEL " + std::to_string(F.BreakBeforeLevel) + " \xC2\xB7 " + Timer(F.SecondsToBreak) : std::string("\xE2\x80\x94");
	Out.Rows[0] = {F.OnBreak ? "NEXT UP" : "BLINDS", Blinds(F.Now)};
	Out.Rows[1] = {F.OnBreak ? "THEN" : "NEXT", Blinds(F.Next)};
	Out.Rows[2] = {"BREAK", Brk};
	Out.Rows[3] = {"PLAYERS", std::to_string(F.Remaining) + " / " + std::to_string(F.Entrants)};
	Out.Rows[4] = {"AVERAGE", Avg};
	Out.Rows[5] = {"PRIZE POOL", Pool};
	Out.Rows[6] = {"MONEY", Money};
	return Out;
}

/** The card at (X, Y): Static draws all of it but the clock and the pill's beat; otherwise only those two. */
void ClockCardPart(Canvas& C, const TableHudInfo& I, const CardWords& Wd, double Now, float X, float Y, bool Static)
{
	const live::ClockFacts& F = *I.Clock;
	const float W = CardW;
	const float H = ClockCardHeight(I);
	if (Static)
	{
		C.FillRoundRect({X, Y, W, H}, 14.0f, Paint::Linear({X, Y}, {X, Y + H}, Fa(Navy, 0.9f), Fa(Midnight, 0.9f)));
		C.StrokeRoundRect({X + 0.5f, Y + 0.5f, W - 1.0f, H - 1.0f}, 14.0f, Fa(Ash, 0.12f), 1.0f);
		C.FillRect({X + 16.0f, Y, 48.0f, 3.0f}, Paint(Hot));
		// The header: the crest, the event, the seat.
		rlnet_detail::NetEmbercrest(C, X + 30.0f, Y + 34.0f, 30.0f, Navy);
	}
	const bool TwoLines = TwoLineHeader(I);
	if (Static)
	{
		const float SeatW = Wd.Seat.empty() || TwoLines ? 0.0f : TrackedWidth(C, Wd.Seat, 13.0f, 800, 1.6f);
		C.Text(Wd.Event, X + 54.0f, Y + 39.0f, Ts(16.0f, 800, Ash, Align::Left, Baseline::Alphabetic, false, W - 54.0f - 16.0f - SeatW - (SeatW > 0.0f ? 12.0f : 0.0f)));
		if (!Wd.Seat.empty())
		{
			if (TwoLines)
			{
				TrackedText(C, Wd.Seat, X + 54.0f, Y + 60.0f, 13.0f, 800, Ice, 1.6f);
			}
			else
			{
				TrackedText(C, Wd.Seat, X + W - 16.0f, Y + 38.0f, 13.0f, 800, Ice, 1.6f, Align::Right);
			}
		}
	}
	float Ry = Y + 54.0f + (TwoLines ? 20.0f : 0.0f);
	// Who paid.
	if (!Wd.Funding.empty())
	{
		if (Static)
		{
			const float Fw = std::min(W - 32.0f, TrackedWidth(C, Wd.Funding, 13.0f, 800, 1.6f) + 24.0f);
			C.FillRoundRect({X + 16.0f, Ry, Fw, 24.0f}, 12.0f, Paint(Fa(Gold, 0.16f)));
			C.StrokeRoundRect({X + 16.5f, Ry + 0.5f, Fw - 1.0f, 23.0f}, 12.0f, Fa(Gold, 0.5f), 1.0f);
			if (TrackedWidth(C, Wd.Funding, 13.0f, 800, 1.6f) + 24.0f <= W - 32.0f)
			{
				TrackedText(C, Wd.Funding, X + 28.0f, Ry + 17.0f, 13.0f, 800, Gold, 1.6f);
			}
			else
			{
				C.Text(Wd.Funding, X + 28.0f, Ry + 17.0f, Ts(13.0f, 800, Gold, Align::Left, Baseline::Alphabetic, false, Fw - 24.0f));
			}
		}
		Ry += 30.0f;
	}
	// The night's state: hand for hand, the break, the money, late registration.
	if (!Wd.Pill.empty())
	{
		if (Static)
		{
			const float Pw = std::min(W - 32.0f, TrackedWidth(C, Wd.Pill, 13.0f, 800, 1.4f) + 36.0f);
			C.FillRoundRect({X + 16.0f, Ry, Pw, 28.0f}, 14.0f, Paint(Fa(Wd.PillCol, 0.2f)));
			if (TrackedWidth(C, Wd.Pill, 13.0f, 800, 1.4f) + 36.0f <= W - 32.0f)
			{
				TrackedText(C, Wd.Pill, X + 42.0f, Ry + 19.0f, 13.0f, 800, Wd.PillCol, 1.4f);
			}
			else
			{
				C.Text(Wd.Pill, X + 42.0f, Ry + 19.0f, Ts(13.0f, 800, Wd.PillCol, Align::Left, Baseline::Alphabetic, false, Pw - 54.0f));
			}
		}
		else
		{
			const float Beat = Wd.Beats ? 0.75f + 0.25f * static_cast<float>(std::sin(Now * 3.0)) : 1.0f;
			C.FillCircle(X + 30.0f, Ry + 14.0f, 4.0f, Paint(Fa(Wd.PillCol, Beat)));
		}
		Ry += 36.0f;
	}
	// The level and its clock.
	if (Static)
	{
		TrackedText(C, Wd.Level, X + 16.0f, Ry + 34.0f, 22.0f, 900, F.OnBreak ? Gold : Ash, 2.0f);
	}
	else
	{
		C.Text(Wd.Ticking, X + W - 16.0f, Ry + 36.0f, Ts(32.0f, 700, F.OnBreak ? Gold : Ash, Align::Right, Baseline::Alphabetic, true));
	}
	Ry += 52.0f;
	if (!Static)
	{
		return;
	}
	// The rows.
	for (int K = 0; K < 7; ++K)
	{
		C.FillRect({X + 16.0f, Ry + 1.0f, W - 32.0f, 1.0f}, Paint(Fa(Ash, 0.07f)));
		const float Lw = TrackedText(C, Wd.Rows[K].first, X + 16.0f, Ry + 19.0f, 13.0f, 800, Fa(Ice, 0.9f), 1.6f);
		const Color Vc = K == 6 ? Wd.MoneyCol : Ash;
		C.Text(Wd.Rows[K].second, X + W - 16.0f, Ry + 20.0f, Ts(15.0f, 700, Vc, Align::Right, Baseline::Alphabetic, false, W - 32.0f - Lw - 14.0f));
		Ry += 26.0f;
	}
	if (!Wd.ColorUp.empty())
	{
		C.Text(Wd.ColorUp, X + 16.0f, Ry + 18.0f, Ts(13.0f, 700, Fa(Gold, 0.95f), Align::Left, Baseline::Alphabetic, false, W - 32.0f));
	}
}

void ClockCard(Canvas& C, const TableHudInfo& I, double Now)
{
	const float S = Scale(I);
	Scaled Sc(C, Margin, 52.0f, S);
	const CardWords Wd = WordsOf(I);
	// Everything but the ticking clock is drawn once for what it says (the rows change between hands, not frames).
	std::string Sig = Wd.Event + '\x1f' + Wd.Seat + '\x1f' + Wd.Funding + '\x1f' + Wd.Pill + '\x1f' + Wd.Level + '\x1f' + Wd.ColorUp + (I.Clock->OnBreak ? "b" : "") +
		std::to_string(static_cast<int>(ClockCardHeight(I))) + (TwoLineHeader(I) ? "2" : "1");
	for (const std::pair<std::string, std::string>& R : Wd.Rows)
	{
		Sig += '\x1f' + R.first + '\x1e' + R.second;
	}
	Sig += std::to_string(static_cast<int>(Wd.MoneyCol.R * 255.0f)) + std::to_string(static_cast<int>(Wd.PillCol.G * 255.0f));
	C.Cached(KeyOf(Sig), Margin, 52.0f, [&I, &Wd, Now](Canvas& Cv) { ClockCardPart(Cv, I, Wd, Now, 0.0f, 0.0f, true); });
	ClockCardPart(C, I, Wd, Now, Margin, 52.0f, false);
}

// ------------------------------------------------------------------ the hero block, the ladder, the pot

void HeroBlock(Canvas& C, const TableHudInfo& I, double Now)
{
	const float S = Scale(I);
	const float X = Margin;
	const float B = Tall - Margin;
	Scaled Sc(C, X, B, S);
	const Venue Where = I.Where;
	const Chips Bb = I.Bb;
	// The stack: the big unit, the other beside it; its depth in colour and number.
	const std::string Count = Amount(Where, I.HeroStack);
	const std::string Bbs = chipset::InBb(I.HeroStack, Bb);
	const bool BbBig = I.Units == StackUnits::BigBlinds && !Dollars(Where) && !Bbs.empty();
	const std::string Big = BbBig ? Bbs : Count;
	const std::string Small = BbBig ? Count : Bbs;
	const Color Col = Dollars(Where) ? Ash : Depth(I.HeroStack, Bb);
	// A soft midnight shade under the block, so it reads over a lit sleeve or a bright felt (the clock card's ink, feathered
	// out; over the dark felt it hardly shows): as wide as the stack's row, the heart's or the Focus bar, whichever is widest.
	{
		float Row = C.Measure(Big, 40.0f, 900) + 18.0f + (Small.empty() ? 0.0f : C.Measure(Small, 20.0f, 700) + 16.0f) + Fl(std::min<size_t>(2, I.HeroCards.size())) * 36.0f;
		Row = std::max(Row, 226.0f + C.Measure(std::to_string(static_cast<int>(std::lround(I.Bpm))), 26.0f, 800, true));
		Row = std::max(Row, I.Focus < 0.995f || I.Studying ? 450.0f : 0.0f);
		const float Top = B - 120.0f - (I.Steadying ? 24.0f : 6.0f);
		C.GlowRoundRect({X - 32.0f, Top - 18.0f, Row + 50.0f, B - Top + 36.0f}, 24.0f, Fa(Midnight, 0.55f), 40.0f);
	}
	TrackedText(C, "YOUR STACK", X, B - 58.0f, 13.0f, 800, Fa(Ice, 0.9f), 2.4f);
	C.FillRoundRect({X - 14.0f, B - 52.0f, 6.0f, 46.0f}, 3.0f, Paint(Fa(Col, 0.9f)));
	const float Bw = C.Text(Big, X, B - 10.0f, Ts(40.0f, 900, Col));
	float Cx = X + Bw + 18.0f;
	if (!Small.empty())
	{
		Cx += C.Text(Small, Cx, B - 12.0f, Ts(20.0f, 700, Fa(Ash, 0.75f))) + 16.0f;
	}
	// The cards, once looked at.
	for (size_t K = 0; K < I.HeroCards.size() && K < 2; ++K)
	{
		MiniCard(C, I.HeroCards[K], Cx + Fl(K) * 36.0f, B - 50.0f, 32.0f, 0.95f);
	}
	// The heart: a trace that runs with it, bright when it races.
	const float Bpm = I.Bpm;
	const float Show = std::clamp((Bpm - 72.0f) / 18.0f, 0.3f, 1.0f);
	const float Tw = 190.0f;
	const float Th = 36.0f;
	const float Tx = X;
	const float Ty = B - 120.0f;
	const Color Line = Mix(Hex(0x9ef2c7), Red, std::clamp((Bpm - 84.0f) / 30.0f, 0.0f, 1.0f));
	if (I.Trace.size() >= 2)
	{
		// At most about 160 points across 190 units (the host samples the heart at 240 Hz): every sample's peak kept.
		std::vector<Vec2> Pts;
		const size_t Total = I.Trace.size();
		const size_t Every = std::max<size_t>(1, Total / 160);
		for (size_t K = 0; K < Total; K += Every)
		{
			float V = I.Trace[K];
			for (size_t J = K + 1; J < std::min(Total, K + Every); ++J)
			{
				V = std::fabs(I.Trace[J]) > std::fabs(V) ? I.Trace[J] : V;
			}
			Pts.push_back({Tx + Tw * Fl(K) / Fl(Total - 1), Ty + Th * (0.72f - 0.62f * std::clamp(V, -0.2f, 1.2f))});
		}
		const size_t N = Pts.size();
		// Older trace fades: drawn in four segments, oldest faintest.
		const size_t Seg = std::max<size_t>(2, N / 4);
		for (size_t From = 0; From + 1 < N; From += Seg)
		{
			const size_t To = std::min(N, From + Seg + 1);
			const std::vector<Vec2> Part(Pts.begin() + static_cast<std::ptrdiff_t>(From), Pts.begin() + static_cast<std::ptrdiff_t>(To));
			const float Fresh = Fl(To) / Fl(N);
			C.StrokePolyline(Part, false, Fa(Line, Show * 0.16f * Fresh * Fresh), 5.0f, true);
			C.StrokePolyline(Part, false, Fa(Line, Show * (0.15f + 0.85f * Fresh * Fresh)), 1.8f, true);
		}
	}
	else
	{
		C.FillRect({Tx, Ty + Th * 0.72f, Tw, 1.5f}, Paint(Fa(Line, 0.4f * Show)));
	}
	const float Beat = std::clamp(I.Trace.empty() ? 0.0f : I.Trace.back() * 1.4f, 0.0f, 1.0f);
	const float Hx = Tx + Tw + 18.0f;
	const float Hy = Ty + 16.0f - 2.0f * Beat;
	const float Hs = 8.0f + 2.5f * Beat;
	C.FillCircle(Hx - Hs * 0.5f, Hy - Hs * 0.2f, Hs * 0.55f, Paint(Fa(Line, Show)));
	C.FillCircle(Hx + Hs * 0.5f, Hy - Hs * 0.2f, Hs * 0.55f, Paint(Fa(Line, Show)));
	C.FillPolygon({{Hx - Hs * 1.04f, Hy}, {Hx + Hs * 1.04f, Hy}, {Hx, Hy + Hs * 1.1f}}, Paint(Fa(Line, Show)));
	C.Text(std::to_string(static_cast<int>(std::lround(Bpm))), Hx + 18.0f, Ty + 28.0f, Ts(26.0f, 800, Fa(Line, Show), Align::Left, Baseline::Alphabetic, true));
	if (I.Steadying)
	{
		C.Text("breathing slow", Tx, Ty - 10.0f, Ts(13.0f, 600, Fa(Ice, Show)));
	}
	// Focus left, when it isn't full or it's being spent.
	if (I.Focus < 0.995f || I.Studying)
	{
		const float Fx = X + 310.0f;
		const float Fy = Ty + 22.0f;
		TrackedText(C, "FOCUS", Fx, Fy - 8.0f, 13.0f, 800, Fa(Ice, 0.9f), 2.0f);
		C.FillRoundRect({Fx, Fy, 140.0f, 5.0f}, 2.5f, Paint(Rgba(0, 0, 0, 0.45f)));
		C.FillRoundRect({Fx, Fy, 140.0f * std::clamp(I.Focus, 0.0f, 1.0f), 5.0f}, 2.5f, Paint(Fa(Gold, 0.9f)));
	}
	(void)Now;
}

/** "#212 · Rick shows K K · you lose 18,400", over the hero block for 6 s after a hand the player played. */
void LastHandCard(Canvas& C, const TableHudInfo& I, double Now)
{
	const recall::Record& R = *I.LastHand;
	const double Age = Now - I.LastHandAt;
	if (Age < 0.0 || Age > 6.0)
	{
		return;
	}
	const float A = std::min(Ease(Age / 0.3), Ease((6.0 - Age) / 0.6));
	const float S = Scale(I);
	const float Y = Tall - Margin - 156.0f * S - 64.0f * S;
	Scaled Sc(C, Margin, Y, S);
	const float W = 470.0f;
	C.FillRoundRect({Margin, Y, W, 54.0f}, 12.0f, Paint(Fa(Midnight, 0.86f * A)));
	C.FillRect({Margin, Y + 10.0f, 3.0f, 34.0f}, Paint(Fa(R.HeroNet >= 0 ? Green : Hot, A)));
	float X = Margin + 16.0f;
	X += C.Text("#" + std::to_string(R.Number), X, Y + 33.0f, Ts(15.0f, 700, Fa(Ice, A), Align::Left, Baseline::Alphabetic, true)) + 12.0f;
	if (!R.Shown.empty())
	{
		const std::string Who = R.Shown.front().first + " shows";
		X += C.Text(Who, X, Y + 33.0f, Ts(15.0f, 600, Fa(Ash, A))) + 8.0f;
		for (size_t K = 0; K < R.Shown.front().second.size() && K < 2; ++K)
		{
			MiniCard(C, R.Shown.front().second[K], X, Y + 10.0f, 24.0f, A);
			X += 28.0f;
		}
		X += 6.0f;
	}
	const Chips Net = R.HeroNet;
	const std::string Res = Net > 0 ? "you win " + Amount(I.Where, Net) : Net < 0 ? "you lose " + Amount(I.Where, -Net) : std::string("you're even");
	C.Text(Res, X, Y + 33.0f, Ts(15.0f, 700, Fa(Net > 0 ? Green : Net < 0 ? Hot : Ash, A), Align::Left, Baseline::Alphabetic, false, Margin + W - 14.0f - X));
}

/** The ladder with its bottom-right corner at (Ax, Ay). */
void LadderAt(Canvas& C, const TableHudInfo& I, float Ax, float Ay, bool Wide)
{
	const float X = Ax - LadderW;
	const Venue Where = I.Where;
	const bool Pad = I.Gamepad;
	struct Rung
	{
		const char* Id;
		std::string Label;
		std::string Value;
		std::string Detail;
		bool Main;
	};
	std::vector<Rung> Rungs;
	Rungs.push_back({"fold", "FOLD", std::string(), std::string(), false});
	Rungs.push_back({"call", I.CanCheck ? "CHECK" : "CALL", I.CanCheck ? std::string() : Amount(Where, I.ToCall), std::string(), true});
	if (I.CanRaise)
	{
		const bool AllInSize = I.Spot.MaxTo > 0 && I.RaiseTo >= I.Spot.MaxTo;
		if (!AllInSize)
		{
			Rungs.push_back({"raise", sizing::Verb(I.Spot, I.RaiseTo), Amount(Where, I.RaiseTo), Dollars(Where) ? std::string() : sizing::Detail(I.Spot, I.RaiseTo), false});
		}
		Rungs.push_back({"allin", "ALL IN", Amount(Where, I.Spot.MaxTo > 0 ? I.Spot.MaxTo : I.HeroStack), std::string(), false});
	}
	const float Rh = 46.0f;
	const float Gap = 8.0f;
	float Ry = Ay - Fl(static_cast<int>(Rungs.size())) * (Rh + Gap) + Gap;
	const float RungsTop = Ry;
	// A soft midnight backing under the whole ladder, its header and presets too (the plates draw first): a plate the layout
	// couldn't keep off it (the spots beside it were over a face, which costs more) doesn't read through what the player is
	// about to press. (At the Nightly's first decision a plate read across "EFFECTIVE 100 BB" and the preset keys.)
	{
		const float Top0 = I.CanRaise && !I.Presets.empty() ? RungsTop - 12.0f - (Wide ? 42.0f : 30.0f) - (Pad ? 0.0f : 16.0f) : RungsTop;
		C.GlowRoundRect({X - 16.0f, Top0 - 64.0f, LadderW + 32.0f, Ay - Top0 + 80.0f}, 18.0f, Fa(Midnight, 0.8f), 22.0f);
	}
	for (const Rung& R : Rungs)
	{
		const Control* K = FindControl(ControlContext::Table, R.Id);
		C.FillRoundRect({X, Ry, LadderW, Rh}, 10.0f, Paint(R.Main ? Fa(Navy, 0.92f) : Fa(Midnight, 0.82f)));
		const bool AllIn = std::string(R.Id) == "allin";
		if (AllIn && I.AllInHold > 0.0f)
		{
			C.PushClip({X, Ry, LadderW * std::clamp(I.AllInHold, 0.0f, 1.0f), Rh});
			C.FillRoundRect({X, Ry, LadderW, Rh}, 10.0f, Paint(Fa(Hot, 0.42f)));
			C.PopClip();
		}
		if (R.Main)
		{
			C.StrokeRoundRect({X + 1.0f, Ry + 1.0f, LadderW - 2.0f, Rh - 2.0f}, 10.0f, Hot, 2.0f);
		}
		float Lx = X + 12.0f;
		if (K)
		{
			Control Shown = *K;
			Shown.Hold = false; // HOLD is said in the rung
			Lx += ControlGlyphs(C, Shown, Pad, Lx, Ry + 33.0f, 1.0f) + 12.0f;
		}
		Lx += TrackedText(C, R.Label, Lx, Ry + 29.0f, 16.0f, 800, Ash, 1.6f) + 10.0f;
		const float Vw = R.Value.empty() ? 0.0f : C.Measure(R.Value, 17.0f, 700, true);
		if (!R.Value.empty())
		{
			C.Text(R.Value, X + LadderW - 14.0f, Ry + 30.0f, Ts(17.0f, 700, R.Main ? Gold : Ash, Align::Right, Baseline::Alphabetic, true));
		}
		const float Room = X + LadderW - 14.0f - Vw - 10.0f - Lx;
		if (AllIn)
		{
			const bool Cancel = I.AllInHold > 0.0f && I.AllInHold < 1.0f;
			const std::string Hold = Cancel ? std::string("LET GO TO CANCEL") : std::string("HOLD");
			C.Text(Hold, Lx, Ry + 29.0f, Ts(13.0f, 700, Fa(Cancel ? Flame : Ice, 0.9f), Align::Left, Baseline::Alphabetic, false, std::max(10.0f, Room)));
		}
		else if (!R.Detail.empty() && Room > 30.0f)
		{
			C.Text(R.Detail, Lx, Ry + 29.0f, Ts(13.0f, 600, Fa(Ice, 0.95f), Align::Left, Baseline::Alphabetic, false, Room));
		}
		Ry += Rh + Gap;
	}
	float Top = RungsTop;
	// The presets above the rungs, the chosen one in ember.
	if (I.CanRaise && !I.Presets.empty())
	{
		const float Ph = Wide ? 42.0f : 30.0f;
		const float Py = RungsTop - 12.0f - Ph;
		const size_t N = I.Presets.size();
		const float Ends = Pad ? 34.0f : 0.0f;
		const float Pw = (LadderW - Ends * 2.0f - Fl(N - 1) * 5.0f) / Fl(N);
		if (Pad)
		{
			Glyph(C, "LB", X, Py + Ph * 0.5f + 10.0f, 0.9f);
			Glyph(C, "RB", X + LadderW - GlyphWidth(C, "RB"), Py + Ph * 0.5f + 10.0f, 0.9f);
		}
		for (size_t K = 0; K < N; ++K)
		{
			const sizing::Preset& P = I.Presets[K];
			const float Px = X + Ends + Fl(K) * (Pw + 5.0f);
			const bool On = static_cast<int>(K) == I.Preset;
			C.FillRoundRect({Px, Py, Pw, Ph}, 8.0f, Paint(On ? Fa(Hot, 0.9f) : Fa(Midnight, 0.82f)));
			if (!On)
			{
				C.StrokeRoundRect({Px + 0.5f, Py + 0.5f, Pw - 1.0f, Ph - 1.0f}, 8.0f, Fa(Ash, 0.14f), 1.0f);
			}
			const Color Tc = On ? Hex(0x14070a) : Ash;
			C.Text(P.Label, Px + Pw * 0.5f, Py + (Wide ? 19.0f : 20.0f), Ts(13.0f, 800, Tc, Align::Center, Baseline::Alphabetic, false, Pw - 6.0f));
			if (Wide)
			{
				std::string Sub = Dollars(Where) ? Amount(Where, P.To) : chipset::InBb(P.To, I.Bb);
				if (C.Measure(Sub, 13.0f, 600) > Pw - 6.0f)
				{
					Sub = Dollars(Where) ? Sub : chipset::BbNumber(P.To, I.Bb); // the number alone where " BB" won't fit
				}
				C.Text(Sub, Px + Pw * 0.5f, Py + 36.0f, Ts(13.0f, 600, On ? Fa(Hex(0x14070a), 0.8f) : Fa(Ice, 0.9f), Align::Center, Baseline::Alphabetic, false, Pw - 6.0f));
			}
			if (!Pad)
			{
				C.Text(std::to_string(K + 1), Px + Pw * 0.5f, Py - 5.0f, Ts(13.0f, 700, Fa(Ice, 0.7f), Align::Center));
			}
		}
		Top = Py - (Pad ? 0.0f : 16.0f);
	}
	// The header: what it costs, how deep it is, the pot.
	const std::string Head = I.ToCall > 0 ? Amount(Where, I.ToCall) + " to call" : std::string("Checks to you");
	C.Text(Head, Ax, Top - 34.0f, Ts(24.0f, 800, Gold, Align::Right));
	std::string Sub;
	if (I.Spot.Effective > 0 && I.Bb > 0 && !Dollars(Where))
	{
		Sub = "EFFECTIVE " + chipset::InBb(I.Spot.Effective, I.Bb);
	}
	if (I.Pot > 0)
	{
		Sub += (Sub.empty() ? "" : " \xC2\xB7 ") + std::string("POT ") + Amount(Where, I.Pot);
	}
	if (!Sub.empty())
	{
		TrackedText(C, Sub, Ax, Top - 12.0f, 13.0f, 800, Fa(Ice, 0.95f), 1.6f, Align::Right);
	}
}

void Ladder(Canvas& C, const TableHudInfo& I, double Now)
{
	(void)Now;
	const float S = Scale(I);
	const float Ax = C.Width() - Margin;
	const float Ay = Tall - Margin;
	Scaled Sc(C, Ax, Ay, S);
	const bool Wide = C.Width() >= 1600.0f;
	if (I.AllInHold > 0.0f)
	{
		LadderAt(C, I, Ax, Ay, Wide); // filling: drawn fresh each frame
		return;
	}
	// The ladder only changes when the player sizes the raise: drawn once for what it says.
	std::string Sig = std::to_string(I.ToCall) + "," + std::to_string(I.RaiseTo) + "," + std::to_string(I.Spot.MaxTo) + "," + std::to_string(I.Spot.Effective) + "," +
		std::to_string(I.Pot) + "," + std::to_string(I.Bb) + "," + std::to_string(I.HeroStack) + "," + std::to_string(I.Preset) + (I.CanCheck ? "k" : "") + (I.CanRaise ? "r" : "") +
		(I.Gamepad ? "p" : "") + (Wide ? "w" : "") + std::to_string(static_cast<int>(I.Where)) + sizing::Describe(I.Spot, I.RaiseTo);
	for (const sizing::Preset& P : I.Presets)
	{
		Sig += '\x1f' + P.Label + std::to_string(P.To);
	}
	C.Cached(KeyOf(Sig), Ax, Ay, [&I, Wide](Canvas& Cv) { LadderAt(Cv, I, 0.0f, 0.0f, Wide); });
}

void PotTag(Canvas& C, const TableHudInfo& I)
{
	const float S = Scale(I);
	Scaled Sc(C, I.PotAt.X, I.PotAt.Y, S);
	const std::string Main = "POT " + Amount(I.Where, I.Pot);
	const std::string Bbs = !Dollars(I.Where) && I.Bb > 0 ? chipset::InBb(I.Pot, I.Bb) : std::string();
	const float Mw = TrackedWidth(C, Main, 15.0f, 800, 1.4f);
	const float Bw = Bbs.empty() ? 0.0f : C.Measure(Bbs, 13.0f, 600) + 12.0f;
	const float W = Mw + Bw + 28.0f;
	const float X = I.PotAt.X - W * 0.5f;
	const float Y = I.PotAt.Y - 15.0f;
	C.FillRoundRect({X, Y, W, 30.0f}, 15.0f, Paint(Fa(Midnight, 0.8f)));
	TrackedText(C, Main, X + 14.0f, Y + 20.5f, 15.0f, 800, Ash, 1.4f);
	if (!Bbs.empty())
	{
		C.Text(Bbs, X + 14.0f + Mw + 12.0f, Y + 20.5f, Ts(13.0f, 600, Ice));
	}
}

// ------------------------------------------------------------------ the top right: the time, the toasts

void TimeBlock(Canvas& C, const TableHudInfo& I)
{
	const float S = Scale(I);
	const float Ax = C.Width() - Margin;
	Scaled Sc(C, Ax, 52.0f, S);
	if (!I.OnFoot.Clock.empty())
	{
		TrackedText(C, I.OnFoot.Clock, Ax, 82.0f, 24.0f, 900, Ash, 1.6f, Align::Right);
	}
	if (I.OnFoot.Life && I.OnFoot.Life->Energy < 30.0)
	{
		const std::string Tired = "TIRED \xC2\xB7 " + std::to_string(static_cast<int>(std::lround(I.OnFoot.Life->Energy))) + "%";
		TrackedText(C, Tired, Ax, 104.0f, 13.0f, 800, Hot, 1.6f, Align::Right);
	}
}

void Toasts(Canvas& C, const TableHudInfo& I, double Now)
{
	const float S = Scale(I);
	const float Ax = C.Width() - Margin;
	Scaled Sc(C, Ax, 112.0f, S);
	const float W = 360.0f;
	float Y = 124.0f;
	int Shown = 0;
	for (auto It = I.Toasts.rbegin(); It != I.Toasts.rend() && Shown < 2; ++It)
	{
		const double Age = Now - It->At;
		if (Age < 0.0 || Age > 6.0)
		{
			continue;
		}
		const float A = std::min(Ease(Age / 0.35), Ease((6.0 - Age) / 0.6));
		const Color Accent = It->Accent != 0 ? Hex(It->Accent) : Teal;
		const std::vector<std::string> Lines = WrapText(C, It->Line, W - 40.0f, 16.0f, 400);
		const size_t N = std::min<size_t>(Lines.size(), 3);
		const float H = 50.0f + Fl(N) * 22.0f;
		const float X = Ax - W + (1.0f - A) * 40.0f;
		C.FillRoundRect({X, Y, W, H}, 12.0f, Paint(Fa(Midnight, 0.88f * A)));
		C.FillRoundRect({X, Y, 4.0f, H}, 2.0f, Paint(Fa(Accent, A)));
		C.Text(Up(It->From), X + 20.0f, Y + 27.0f, Ts(13.0f, 800, Fa(Accent, A), Align::Left, Baseline::Alphabetic, false, W - 40.0f));
		float Ly = Y + 30.0f;
		for (size_t K = 0; K < N; ++K)
		{
			Ly += 22.0f;
			std::string L = Lines[K];
			if (K + 1 == N && Lines.size() > N)
			{
				L += "\xE2\x80\xA6";
			}
			C.Text(L, X + 20.0f, Ly, Ts(16.0f, 400, Fa(Ash, 0.92f * A), Align::Left, Baseline::Alphabetic, false, W - 40.0f));
		}
		Y += H + 10.0f;
		++Shown;
	}
}

// ------------------------------------------------------------------ the middle: the banner, whispers, notes

void Banner(Canvas& C, const TableHudInfo& I, double Now)
{
	const double Age = Now - I.BannerAt;
	if (I.Banner.empty() || Age < 0.0 || Age > 4.2)
	{
		return;
	}
	const float A = Ease(Age / 0.3) * (1.0f - static_cast<float>(Clamp01((Age - 3.4) / 0.8)));
	const float W = C.Width();
	const float Tw = std::min(W - 2.0f * Margin, TrackedWidth(C, I.Banner, 32.0f, 900, 4.0f));
	const float Fit = Tw < TrackedWidth(C, I.Banner, 32.0f, 900, 4.0f) ? Tw / TrackedWidth(C, I.Banner, 32.0f, 900, 4.0f) : 1.0f;
	C.FillRect({W * 0.5f - Tw * 0.5f - 30.0f, 66.0f, Tw + 60.0f, 2.0f}, Paint(Fa(Hot, 0.7f * A)));
	TrackedText(C, I.Banner, W * 0.5f, 106.0f, 32.0f * Fit, 900, Fa(Ash, A), 4.0f * Fit, Align::Center);
	C.FillRect({W * 0.5f - Tw * 0.5f - 30.0f, 124.0f, Tw + 60.0f, 2.0f}, Paint(Fa(Hot, 0.7f * A)));
}

void Whispers(Canvas& C, const TableHudInfo& I, double Now)
{
	std::vector<const StreetHudInfo::Subtitle*> Live;
	for (const StreetHudInfo::Subtitle& Wsp : I.Whispers)
	{
		const double Hold = 4.8 + 0.045 * static_cast<double>(Wsp.Line.size());
		if (Now - Wsp.At >= 0.0 && Now - Wsp.At < Hold)
		{
			Live.push_back(&Wsp);
		}
	}
	if (Live.size() > 3)
	{
		Live.erase(Live.begin(), Live.end() - 3);
	}
	float Y = 186.0f;
	for (const StreetHudInfo::Subtitle* Wsp : Live)
	{
		const double Hold = 4.8 + 0.045 * static_cast<double>(Wsp->Line.size());
		const double Age = Now - Wsp->At;
		const float A = std::min(Ease(Age / 0.4), Ease((Hold - Age) / 1.0));
		const bool Learned = Wsp->Line.rfind("READ LEARNED", 0) == 0;
		C.Text(Wsp->Line, C.Width() * 0.5f, Y, Ts(Learned ? 20.0f : 18.0f, Learned ? 800 : 600, Fa(Learned ? Gold : Hex(0xa3dbea), A), Align::Center, Baseline::Alphabetic, false, C.Width() - 2.0f * Margin));
		Y += 32.0f;
	}
}

void StudyCaption(Canvas& C, const TableHudInfo& I, double Now)
{
	const Studied& St = I.Study;
	const float A = std::clamp(St.Alpha, 0.0f, 1.0f);
	if (A <= 0.01f || St.Name.empty())
	{
		return;
	}
	const float W = C.Width();
	const float Y = Tall * 0.6f;
	TrackedText(C, Up(St.Name), W * 0.5f, Y, 22.0f, 800, Fa(Ash, A), 3.0f, Align::Center);
	if (!St.Label.empty())
	{
		C.Text(St.Label, W * 0.5f, Y + 32.0f, Ts(16.0f, 600, Fa(Gold, 0.95f * A), Align::Center));
	}
	const size_t N = std::min<size_t>(3, St.Badges.empty() ? St.BadgeCells.size() : St.Badges.size());
	if (!St.Badges.empty())
	{
		const float X0 = W * 0.5f - (Fl(N) * 26.0f + Fl(N - 1) * 10.0f) * 0.5f + 13.0f;
		const float Was = C.GetAlpha();
		C.SetAlpha(A);
		for (size_t K = 0; K < N; ++K)
		{
			badgeart::Draw(C, St.Badges[K], X0 + Fl(K) * 36.0f, Y + 62.0f, 26.0f, Now);
		}
		C.SetAlpha(Was);
	}
	if (!St.Honorific.empty())
	{
		C.Text(St.Honorific, W * 0.5f, Y + 96.0f, Ts(15.0f, 500, Fa(Ice, 0.9f * A), Align::Center));
	}
}

void Arrival(Canvas& C, const TableHudInfo& I, double Now)
{
	const double T = Now - I.Arrive.At;
	if (I.Arrive.Title.empty() || T < 0.0 || T > 6.4)
	{
		return;
	}
	const float A = static_cast<float>(Clamp01((T - 0.6) / 1.2) * (1.0 - Clamp01((T - 5.0) / 1.4)));
	const float W = C.Width();
	const float Y = Tall * 0.4f;
	TrackedText(C, Up(I.Arrive.Title), W * 0.5f, Y, 40.0f, 900, Fa(Ash, A), 10.0f, Align::Center);
	if (!I.Arrive.Sub.empty())
	{
		C.Text(I.Arrive.Sub, W * 0.5f, Y + 46.0f, Ts(22.0f, 500, Fa(Gold, A), Align::Center));
	}
	if (!I.Arrive.Day.empty())
	{
		C.Text(I.Arrive.Day, W * 0.5f, Y + 82.0f, Ts(17.0f, 400, Fa(Ice, A), Align::Center));
	}
}

/** Choices in a row, centred on X, baseline Y: "[Q] Stay and watch   [L] Go home now". */
void ChoiceRow(Canvas& C, const std::vector<Choice>& Choices, ControlContext Ctx, bool Pad, float Cx, float Y, float A)
{
	auto KeyFor = [&](const Choice& Ch) {
		Control K;
		const Control* Found = FindControl(Ctx, Ch.Control);
		if (Found)
		{
			K = *Found;
		}
		else
		{
			K.Key = Ch.Control.c_str();
			K.Pad = Ch.Control.c_str();
		}
		return K;
	};
	float Total = 0.0f;
	for (const Choice& Ch : Choices)
	{
		Total += ControlGlyphs(C, KeyFor(Ch), Pad, 0.0f, Y, A, false) + 10.0f + C.Measure(Ch.Label, 18.0f, 600) + 36.0f;
	}
	float X = Cx - (Total - 36.0f) * 0.5f;
	for (const Choice& Ch : Choices)
	{
		X += ControlGlyphs(C, KeyFor(Ch), Pad, X, Y + 4.0f, A) + 10.0f;
		X += C.Text(Ch.Label, X, Y, Ts(18.0f, 600, Fa(Ash, A))) + 36.0f;
	}
}

void NoteAndChoices(Canvas& C, const TableHudInfo& I, double Now)
{
	const double Age = Now - I.NoteAt;
	if (!I.Note.empty() && Age >= 0.0 && (I.NoteHolds || Age < 2.6))
	{
		const float A = I.NoteHolds ? Ease(Age / 0.3) : std::min(Ease(Age / 0.25), Ease((2.6 - Age) / 0.5));
		const float W = C.Width();
		const float Y = I.NoteHolds ? Tall * 0.42f : Tall * 0.3f;
		C.Text(I.Note, W * 0.5f, Y, Ts(I.NoteHolds ? 38.0f : 22.0f, I.NoteHolds ? 800 : 700, Fa(Gold, A), Align::Center, Baseline::Alphabetic, false, W - 2.0f * Margin));
		if (!I.NoteSub.empty())
		{
			C.Text(I.NoteSub, W * 0.5f, Y + (I.NoteHolds ? 40.0f : 30.0f), Ts(16.0f, 500, Fa(Ice, A), Align::Center, Baseline::Alphabetic, false, W - 2.0f * Margin));
		}
		if (!I.Choices.empty())
		{
			ChoiceRow(C, I.Choices, ControlContext::Table, I.Gamepad, W * 0.5f, Y + (I.NoteSub.empty() ? 52.0f : 84.0f), A);
		}
	}
	else if (!I.Choices.empty())
	{
		ChoiceRow(C, I.Choices, ControlContext::Table, I.Gamepad, C.Width() * 0.5f, Tall - 70.0f, 1.0f);
	}
}

void StatusLines(Canvas& C, const TableHudInfo& I)
{
	const float W = C.Width();
	if (!I.Status.empty())
	{
		C.Text(I.Status, W * 0.5f, Tall - 190.0f, Ts(17.0f, 600, Fa(Gold, 0.95f), Align::Center, Baseline::Alphabetic, false, W - 2.0f * Margin - 900.0f > 400.0f ? W - 900.0f : 520.0f));
	}
	if (!I.HoldLabel.empty() && I.HoldProgress > 0.0f)
	{
		const float Lw = std::min(520.0f, C.Measure(I.HoldLabel, 18.0f, 700) + 40.0f);
		C.Text(I.HoldLabel, W * 0.5f, Tall - 236.0f, Ts(18.0f, 700, Ash, Align::Center, Baseline::Alphabetic, false, 520.0f));
		C.FillRect({W * 0.5f - Lw * 0.5f, Tall - 222.0f, Lw, 3.0f}, Paint(Rgba(0, 0, 0, 0.45f)));
		C.FillRect({W * 0.5f - Lw * 0.5f, Tall - 222.0f, Lw * std::clamp(I.HoldProgress, 0.0f, 1.0f), 3.0f}, Paint(Hot));
	}
	// Out of the hand at a live table: the pace and the next hand.
	if (I.Pace >= 0 && !I.YourTurn && !I.InHand)
	{
		static const char* const Paces[3] = {"LIVE", "BRISK", "FAST"};
		if (I.Skipping)
		{
			C.Text("to the next hand\xE2\x80\xA6", W * 0.5f, Tall - 66.0f, Ts(15.0f, 600, Fa(Ice, 0.85f), Align::Center));
		}
		else
		{
			const float Row = ControlRow(C, ControlContext::Table, {"next", "pace"}, I.Gamepad, 0.0f, 0.0f, 1.0f, false);
			const std::string Pace = Paces[std::clamp(I.Pace, 0, 2)];
			const float Pw = TrackedWidth(C, Pace, 13.0f, 800, 2.6f) + 12.0f;
			const float X = W * 0.5f - (Row + Pw) * 0.5f;
			ControlRow(C, ControlContext::Table, {"next", "pace"}, I.Gamepad, X, Tall - 62.0f, 0.85f);
			TrackedText(C, Pace, X + Row + 12.0f, Tall - 64.0f, 13.0f, 800, Gold, 2.6f);
		}
	}
}

void Summary(Canvas& C, const TableHudInfo& I)
{
	const float A = std::clamp(I.SummaryAlpha, 0.0f, 1.0f);
	if (A <= 0.01f || I.Summary.empty())
	{
		return;
	}
	const float W = C.Width();
	const float Bw = 600.0f;
	const float Bh = 76.0f + 34.0f * Fl(I.Summary.size() - 1);
	const float Bx = W * 0.5f - Bw * 0.5f;
	const float By = Tall * 0.3f;
	C.FillRoundRect({Bx, By, Bw, Bh}, 14.0f, Paint(Fa(Midnight, 0.86f * A)));
	C.FillRect({Bx + 20.0f, By, 60.0f, 3.0f}, Paint(Fa(Hot, A)));
	float Ly = By + 50.0f;
	for (size_t K = 0; K < I.Summary.size(); ++K)
	{
		const std::string& L = I.Summary[K];
		if (K == 0)
		{
			TrackedText(C, Up(L), W * 0.5f, Ly, 22.0f, 900, Fa(Ash, A), 3.0f, Align::Center);
			Ly += 42.0f;
			continue;
		}
		const bool Net = L.rfind("Net", 0) == 0;
		const bool Read = L.rfind("Read", 0) == 0;
		const Color Col = Net ? (I.Net >= 0 ? Green : Hot) : Read ? Gold : K == 1 ? Ice : Ash;
		C.Text(L, W * 0.5f, Ly, Ts(Net ? 20.0f : 17.0f, Net ? 800 : 500, Fa(Col, A), Align::Center, Baseline::Alphabetic, false, Bw - 40.0f));
		Ly += 34.0f;
	}
}

// ------------------------------------------------------------------ on foot

void OnFoot(Canvas& C, const TableHudInfo& I, double Now)
{
	const float W = C.Width();
	// The clock in a line under the street's block: under its vitals, and below them a Penny Drop chip and a sponsor's duty
	// chip when they show.
	const StreetHudInfo& Ft = I.OnFoot;
	const bool Penny = Ft.Life && Ft.World >= 0.0 && !Ft.Life->Deliveries.empty();
	const bool Duty = !Ft.Due.Label.empty() && Ft.World >= 0.0 && Ft.World < Ft.Due.To;
	const float PillY = !Ft.Life ? 140.0f : Duty && Penny ? 312.0f : 278.0f;
	std::string Pill;
	if (I.Clock)
	{
		const live::ClockFacts& F = *I.Clock;
		Pill = F.OnBreak ? "BREAK " + Timer(F.BreakSecondsLeft) + " \xC2\xB7 LEVEL " + std::to_string(F.Level) + " " + Grouped(F.Now.Sb) + "/" + Grouped(F.Now.Bb)
						 : "LEVEL " + std::to_string(F.Level) + " \xC2\xB7 " + Timer(F.LevelSecondsLeft) + " \xC2\xB7 " + Grouped(F.Now.Sb) + "/" + Grouped(F.Now.Bb);
		if (F.HandForHand)
		{
			Pill += " \xC2\xB7 HAND FOR HAND";
		}
	}
	const float PillW = Pill.empty() ? 0.0f : TrackedWidth(C, Pill, 13.0f, 800, 1.6f) + 40.0f;
	// A soft midnight shade under the whole block (in place of the street's own, which stops above the clock's line), so the
	// needs and their words read over a face at the rail or a lit wall: the clock card's ink, feathered out.
	{
		const float Bottom = !Pill.empty() ? PillY + 28.0f : !Ft.Life ? 112.0f : Duty && Penny ? 300.0f : Duty || Penny ? 266.0f : 216.0f;
		const float Right = std::max({446.0f, 104.0f + TrackedWidth(C, Ft.Place, 15.0f, 800, 5.0f), Margin + PillW});
		C.GlowRoundRect({26.0f, 30.0f, Right + 18.0f - 26.0f, Bottom + 12.0f - 30.0f}, 24.0f, Fa(Midnight, 0.55f), 40.0f);
	}
	StreetHudInfo Foot = I.OnFoot;
	Foot.Subtitles.clear(); // the room's captions are drawn below, with the player's settings
	Foot.HintsAt = -100.0;  // the card room's own controls row
	Foot.Shaded = true;     // the shade above is the block's (two would stack)
	DrawStreetHud(C, Foot, Now);
	if (I.Clock)
	{
		const live::ClockFacts& F = *I.Clock;
		C.FillRoundRect({Margin, PillY, PillW, 28.0f}, 14.0f, Paint(Fa(Midnight, 0.82f)));
		C.FillCircle(Margin + 15.0f, PillY + 14.0f, 4.0f, Paint(F.OnBreak ? Gold : F.HandForHand ? Hot : Teal));
		TrackedText(C, Pill, Margin + 28.0f, PillY + 19.0f, 13.0f, 800, Ash, 1.6f);
	}
	// Where the player's seat is.
	const TableHudInfo::Marker& M = I.SeatMarker;
	if (!M.Text.empty())
	{
		const float Tw = TrackedWidth(C, M.Text, 15.0f, 800, 1.8f);
		const std::string Dist = M.Metres > 0.0f ? std::to_string(static_cast<int>(std::lround(M.Metres))) + " m" : std::string();
		const float Dw = Dist.empty() ? 0.0f : C.Measure(Dist, 15.0f, 700) + 34.0f;
		const float Sw = M.Sub.empty() ? 0.0f : TrackedWidth(C, M.Sub, 13.0f, 800, 1.4f);
		const float Bw = std::max(Tw + Dw, Sw) + 32.0f;
		const float Bh = M.Sub.empty() ? 36.0f : 58.0f;
		float X = M.X - Bw * 0.5f;
		float Y = M.Y - Bh - 14.0f;
		int Edge = 0;
		if (!M.OnScreen || X < Margin || X + Bw > W - Margin || Y < 120.0f || Y + Bh > Tall - 160.0f)
		{
			Edge = M.X < W * 0.5f ? -1 : 1;
		}
		X = std::clamp(X, Margin + 20.0f, W - Margin - Bw - 20.0f);
		Y = std::clamp(Y, 300.0f, Tall - 260.0f);
		C.FillRoundRect({X, Y, Bw, Bh}, 12.0f, Paint(Fa(Midnight, 0.84f)));
		C.StrokeRoundRect({X + 0.5f, Y + 0.5f, Bw - 1.0f, Bh - 1.0f}, 12.0f, Fa(Gold, 0.5f), 1.0f);
		TrackedText(C, M.Text, X + 16.0f, Y + 24.0f, 15.0f, 800, Ash, 1.8f);
		if (!Dist.empty())
		{
			// An arrow toward the seat, then the distance.
			const float Ax = X + 16.0f + Tw + 18.0f;
			const float Ay = Y + 19.0f;
			C.StrokePolyline({{Ax - 6.0f, Ay + 6.0f}, {Ax + 6.0f, Ay - 6.0f}}, false, Gold, 2.2f, true);
			C.StrokePolyline({{Ax - 1.0f, Ay - 6.0f}, {Ax + 6.0f, Ay - 6.0f}, {Ax + 6.0f, Ay + 1.0f}}, false, Gold, 2.2f, true);
			C.Text(Dist, Ax + 14.0f, Y + 24.0f, Ts(15.0f, 700, Gold));
		}
		if (!M.Sub.empty())
		{
			TrackedText(C, M.Sub, X + 16.0f, Y + 46.0f, 13.0f, 800, Ice, 1.4f);
		}
		if (Edge != 0)
		{
			Chevron(C, Edge < 0 ? X - 14.0f : X + Bw + 14.0f, Y + Bh * 0.5f, 10.0f, Edge > 0, Gold);
		}
		else
		{
			C.FillPolygon({{M.X - 7.0f, Y + Bh}, {M.X + 7.0f, Y + Bh}, {M.X, Y + Bh + 8.0f}}, Paint(Fa(Midnight, 0.84f)));
		}
	}
	// How the night stands while the player's up.
	if (!I.FootNote.empty())
	{
		C.Text(I.FootNote, W * 0.5f, Tall - 104.0f, Ts(16.0f, 500, Fa(Ice, 0.9f), Align::Center, Baseline::Alphabetic, false, W - 2.0f * Margin));
	}
	// The controls, the street's (MOVE, RUN, 1ST / 3RD PERSON, INTERACT, PAUSE) for the first while.
	const double Since = Now - I.HintsAt;
	if (Since >= 0.0 && Since < 14.0)
	{
		const float A = std::min(Ease(Since / 0.6), Ease((14.0 - Since) / 1.0));
		ControlRow(C, ControlContext::CardRoomOnFoot, {"move", "run", "view", "use", "pause"}, I.Gamepad, Margin, Tall - 62.0f, A);
	}
}

// ------------------------------------------------------------------ the rail's lower third

void LowerThird(Canvas& C, const TableHudInfo& I, double Now)
{
	const float W = C.Width();
	const float H = 132.0f;
	const float Y = Tall - Margin - H;
	const float X = Margin;
	const float Bw = W - 2.0f * Margin;
	C.FillRoundRect({X, Y, Bw, H}, 14.0f, Paint::Linear({X, Y}, {X + Bw, Y}, Fa(Midnight, 0.92f), Fa(Navy, 0.86f)));
	C.FillRect({X, Y, Bw, 3.0f}, Paint(Hot));
	const float Blink = 0.6f + 0.4f * static_cast<float>(std::sin(Now * 3.0));
	C.FillRoundRect({X + 18.0f, Y + 18.0f, 92.0f, 26.0f}, 13.0f, Paint(Red));
	C.FillCircle(X + 32.0f, Y + 31.0f, 4.5f, Paint(Fa(Hex(0xffffff), Blink)));
	TrackedText(C, "ON AIR", X + 42.0f, Y + 36.0f, 13.0f, 900, Hex(0xffffff), 2.0f);
	const std::string Title = I.Event.empty() ? std::string("THE FEATURE TABLE") : Up(I.Event) + " \xC2\xB7 FEATURE TABLE";
	TrackedText(C, Title, X + 126.0f, Y + 36.0f, 15.0f, 800, Ash, 2.0f);
	if (I.NextOutCents > 0)
	{
		TrackedText(C, "NEXT OUT " + WholeDollars(I.NextOutCents), X + Bw - 20.0f, Y + 36.0f, 15.0f, 800, Gold, 1.6f, Align::Right);
	}
	// The seats, in a row: name, count, depth.
	std::vector<const SeatPlate*> Seats;
	for (const SeatPlate& P : I.Plates)
	{
		if (!P.Away || P.Stack > 0)
		{
			Seats.push_back(&P);
		}
	}
	if (Seats.empty())
	{
		return;
	}
	const float Cw = (Bw - 36.0f) / Fl(std::max<size_t>(6, Seats.size()));
	for (size_t K = 0; K < Seats.size(); ++K)
	{
		const SeatPlate& P = *Seats[K];
		const float Cx = X + 18.0f + Fl(K) * Cw;
		const float A = P.Folded ? 0.6f : 1.0f;
		C.Text(P.Name, Cx, Y + 80.0f, Ts(16.0f, 800, Fa(Ash, A), Align::Left, Baseline::Alphabetic, false, Cw - 12.0f));
		if (I.Readout == StackReadout::Eyes)
		{
			continue; // YOUR EYES: the counts are on the stage's monitors, not over the player's view (§6.2 A)
		}
		const std::string Count = P.AllIn && P.Stack <= 0 ? std::string("ALL IN") : Grouped(P.Stack);
		const std::string Bbs = I.Bb > 0 && P.Stack > 0 ? chipset::InBb(P.Stack, I.Bb) : std::string();
		C.Text(Count + (Bbs.empty() ? "" : " \xC2\xB7 " + Bbs), Cx, Y + 106.0f, Ts(13.0f, 600, Fa(P.AllIn ? Hot : Depth(P.Stack, I.Bb), A), Align::Left, Baseline::Alphabetic, true, Cw - 12.0f));
	}
}

// ------------------------------------------------------------------ the first table's hints

void Hints(Canvas& C, const TableHudInfo& I, double Now)
{
	const double Since = Now - I.HintsAt;
	if (Since < 0.0 || Since >= 14.0)
	{
		return;
	}
	const float A = std::min(Ease(Since / 0.6), Ease((14.0 - Since) / 1.0)) * 0.9f;
	const std::vector<std::string> Ids = {"peek", "study", "breathe", "card", "view", "stand", "pace", "leave"};
	const float Right = C.Width() - Margin;
	const float RowH = I.Gamepad ? 34.0f : 32.0f;
	float Y = 360.0f;
	for (const std::string& Id : Ids)
	{
		const Control* K = FindControl(ControlContext::Table, Id);
		if (!K)
		{
			continue;
		}
		const float Lw = TrackedWidth(C, K->Label, 13.0f, 700, 2.4f);
		const float Gw = ControlGlyphs(C, *K, I.Gamepad, 0.0f, 0.0f, A, false);
		const float X = Right - Lw - 10.0f - Gw;
		C.FillRoundRect({X - 12.0f, Y - 24.0f, Right - X + 24.0f, 30.0f}, 8.0f, Paint(Fa(Midnight, 0.6f * A)));
		ControlGlyphs(C, *K, I.Gamepad, X, Y, A);
		TrackedText(C, K->Label, Right, Y - 3.0f, 13.0f, 700, Fa(Ash, A), 2.4f, Align::Right);
		Y += RowH;
	}
}
} // namespace tablehud_detail

// ------------------------------------------------------------------ the HUD

void DrawTableHud(Canvas& C, const TableHudInfo& I, double Now)
{
	using namespace tablehud_detail;
	const float W = C.Width();
	// Tired: the lids come down, soft-edged.
	if (I.Eyelids > 0.01f)
	{
		const float Lid = std::clamp(I.Eyelids, 0.0f, 1.0f) * Tall * 0.5f;
		C.FillRect({0.0f, 0.0f, W, Lid}, Paint(Hex(0x000000)));
		C.FillRect({0.0f, Tall - Lid, W, Lid}, Paint(Hex(0x000000)));
		C.FillRect({0.0f, Lid, W, 70.0f}, Paint::Linear({0.0f, Lid}, {0.0f, Lid + 70.0f}, Rgba(0, 0, 0, 1.0f), Rgba(0, 0, 0, 0.0f)));
		C.FillRect({0.0f, Tall - Lid - 70.0f, W, 70.0f}, Paint::Linear({0.0f, Tall - Lid - 70.0f}, {0.0f, Tall - Lid}, Rgba(0, 0, 0, 0.0f), Rgba(0, 0, 0, 1.0f)));
	}
	if (I.Walking)
	{
		OnFoot(C, I, Now);
		for (const PlacedPlate& Pl : LayoutPlates(I, W, Now))
		{
			DrawPlate(C, I, I.Plates[static_cast<size_t>(Pl.Index)], Pl, Now);
		}
		const CaptionPlace Cp = CaptionsAt(I, W);
		DrawCaptions(C, I.Captions, I.CaptionLook, Cp.Bottom, Cp.MaxWidth, Now);
		Banner(C, I, Now);
		Summary(C, I);
		NoteAndChoices(C, I, Now);
		Arrival(C, I, Now);
		return;
	}
	// In the world first: the plates and the pot, under the panels.
	for (const PlacedPlate& Pl : LayoutPlates(I, W, Now))
	{
		DrawPlate(C, I, I.Plates[static_cast<size_t>(Pl.Index)], Pl, Now);
	}
	if (I.PotOnScreen && I.Pot > 0)
	{
		PotTag(C, I);
	}
	// (Under the table card the clock card steps aside: the card covers its right column at 16:9 and 4:3, and says the
	// level and its clock in its own header.)
	if (I.Clock && !I.CardUp)
	{
		ClockCard(C, I, Now);
	}
	TimeBlock(C, I);
	if (!I.YourTurn)
	{
		Toasts(C, I, Now);
	}
	Banner(C, I, Now);
	Whispers(C, I, Now);
	StudyCaption(C, I, Now);
	if (I.OnAir)
	{
		LowerThird(C, I, Now);
	}
	else
	{
		HeroBlock(C, I, Now);
		if (I.LastHand)
		{
			LastHandCard(C, I, Now);
		}
		if (I.YourTurn)
		{
			Ladder(C, I, Now);
		}
	}
	// (Over the lower third on the rail.)
	const CaptionPlace Cp = CaptionsAt(I, W);
	DrawCaptions(C, I.Captions, I.CaptionLook, Cp.Bottom, Cp.MaxWidth, Now);
	if (!I.Prompt.empty())
	{
		C.Text(I.Prompt, W * 0.5f, Tall - 120.0f, Ts(18.0f, 700, Ash, Align::Center, Baseline::Alphabetic, false, W - 2.0f * Margin));
	}
	StatusLines(C, I);
	Summary(C, I);
	NoteAndChoices(C, I, Now);
	if (!I.YourTurn)
	{
		Hints(C, I, Now);
	}
	Arrival(C, I, Now);
}

// ------------------------------------------------------------------ the field's counts

std::vector<FieldRow> FieldNow(const Tournament& T, const std::vector<std::pair<std::string, std::string>>& Known, const std::vector<FieldRow>& AtBreak)
{
	std::vector<FieldRow> Out;
	const int HeroTable = T.Hero().Busted ? -1 : T.Hero().TableId;
	for (const std::pair<std::string, std::string>& K : Known)
	{
		for (const TPlayer& P : T.Players)
		{
			if (P.IsHero || P.Busted || P.Name != K.first)
			{
				continue;
			}
			FieldRow R;
			R.Name = K.first;
			R.Label = K.second;
			R.Table = P.TableId;
			R.Here = P.TableId == HeroTable;
			for (const FieldRow& B : AtBreak)
			{
				R.Count = B.Name == K.first ? B.Count : R.Count;
			}
			Out.push_back(R);
			break;
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const FieldRow& A, const FieldRow& B) { return A.Count > B.Count; });
	return Out;
}

std::vector<FieldRow> FieldCounts(const Tournament& T, const std::vector<std::pair<std::string, std::string>>& Known)
{
	std::vector<FieldRow> Out = FieldNow(T, Known, {});
	for (FieldRow& R : Out)
	{
		for (const TPlayer& P : T.Players)
		{
			R.Count = !P.IsHero && !P.Busted && P.Name == R.Name ? P.Stack : R.Count;
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const FieldRow& A, const FieldRow& B) { return A.Count > B.Count; });
	return Out;
}

// ------------------------------------------------------------------ the table card

const std::vector<std::string>& TableCardPages()
{
	static const std::vector<std::string> Pages = {"READS", "HANDS", "PAYOUTS", "FIELD", "NIGHT"};
	return Pages;
}

namespace tablehud_detail
{
/** "Rick raises to 3,200", "You call 13,200", "Mei is all in for 18,400". */
std::string Say(const recall::Line& L, Venue Where, const std::string& HeroName)
{
	const bool You = L.Who == HeroName || L.Who == "You";
	std::string Verb = L.What;
	if (L.What == "all in")
	{
		Verb = You ? "go all in for" : "is all in for";
	}
	else if (You)
	{
		Verb = L.What == "folds" ? "fold" : L.What == "checks" ? "check" : L.What == "calls" ? "call" : L.What == "bets" ? "bet" : L.What == "raises to" ? "raise to" : L.What;
	}
	std::string Out = (You ? std::string("You") : L.Who) + " " + Verb;
	if (L.To > 0 && L.What != "folds" && L.What != "checks")
	{
		Out += " " + Amount(Where, L.To);
	}
	return Out;
}

void ReadsPage(Canvas& C, const TableCardInfo& I, const Rect& R)
{
	if (I.Reads.empty())
	{
		const std::string Line = I.Gamepad ? "Nothing yet. Study a face with the right trigger while the hand plays out, then see what they turn over."
										   : "Nothing yet. Study a face with the right mouse button while the hand plays out, then see what they turn over.";
		float Y = R.Y + 40.0f;
		for (const std::string& L : WrapText(C, Line, R.W, 18.0f, 400))
		{
			C.Text(L, R.X, Y, Ts(18.0f, 400, Fa(Ash, 0.7f)));
			Y += 28.0f;
		}
		return;
	}
	const size_t PerCol = static_cast<size_t>(std::max(1.0f, std::floor(R.H / 32.0f)));
	const bool Two = I.Reads.size() > PerCol;
	const float Cw = Two ? (R.W - 30.0f) * 0.5f : R.W;
	for (size_t K = 0; K < I.Reads.size() && K < PerCol * 2; ++K)
	{
		const float X = R.X + (K >= PerCol ? Cw + 30.0f : 0.0f);
		const float Y = R.Y + 28.0f + Fl(K % PerCol) * 32.0f;
		const std::string& L = I.Reads[K];
		const bool Once = L.find("seen once") != std::string::npos;
		C.FillCircle(X + 5.0f, Y - 6.0f, 3.5f, Paint(Once ? Ice : Gold));
		C.Text(L, X + 18.0f, Y, Ts(17.0f, Once ? 400 : 600, Fa(Ash, Once ? 0.7f : 1.0f), Align::Left, Baseline::Alphabetic, false, Cw - 18.0f));
	}
}

void HandsPage(Canvas& C, const TableCardInfo& I, const Rect& R)
{
	if (I.Hands.empty())
	{
		C.Text("No hands yet tonight. The first one's coming.", R.X, R.Y + 40.0f, Ts(18.0f, 400, Fa(Ash, 0.7f)));
		return;
	}
	const float RowH = std::min(62.0f, R.H / 10.0f);
	float Y = R.Y;
	for (size_t K = 0; K < I.Hands.size() && K < 10; ++K)
	{
		const recall::Record& H = I.Hands[K];
		if (K % 2 == 0)
		{
			C.FillRoundRect({R.X - 10.0f, Y, R.W + 20.0f, RowH - 4.0f}, 8.0f, Paint(Fa(Ash, 0.04f)));
		}
		const float Base = Y + RowH * 0.5f + 5.0f;
		C.Text("#" + std::to_string(H.Number), R.X, Base - 8.0f, Ts(15.0f, 700, Ash, Align::Left, Baseline::Alphabetic, true));
		C.Text("L" + std::to_string(H.Level) + " \xC2\xB7 " + Grouped(H.Blinds.Sb) + "/" + Grouped(H.Blinds.Bb), R.X, Base + 12.0f, Ts(13.0f, 500, Ice));
		float X = R.X + 140.0f;
		const float Cw = std::min(28.0f, (RowH - 10.0f) / 1.4f);
		for (size_t Cd = 0; Cd < H.Mine.size() && Cd < 2; ++Cd)
		{
			MiniCard(C, H.Mine[Cd], X, Y + (RowH - 4.0f - Cw * 1.4f) * 0.5f, Cw, 1.0f);
			X += Cw + 4.0f;
		}
		X += 14.0f;
		for (size_t Cd = 0; Cd < 5; ++Cd)
		{
			if (Cd < H.Board.size())
			{
				MiniCard(C, H.Board[Cd], X, Y + (RowH - 4.0f - Cw * 1.4f) * 0.5f, Cw, 1.0f);
			}
			else
			{
				C.StrokeRoundRect({X, Y + (RowH - 4.0f - Cw * 1.4f) * 0.5f, Cw, Cw * 1.4f}, 4.0f, Fa(Ash, 0.12f), 1.0f);
			}
			X += Cw + 4.0f;
		}
		X += 18.0f;
		// The result, right; the story of the hand between.
		const Chips Net = H.HeroNet;
		std::string Res;
		if (I.Units == StackUnits::BigBlinds && I.Where != Venue::BackRoom && H.Blinds.Bb > 0)
		{
			Res = (Net > 0 ? "+" : Net < 0 ? "-" : "") + chipset::InBb(Net < 0 ? -Net : Net, H.Blinds.Bb);
		}
		else
		{
			Res = (Net > 0 ? "+" : Net < 0 ? "-" : "") + Amount(I.Where, Net < 0 ? -Net : Net);
		}
		const float Rw = C.Text(Net == 0 ? std::string("EVEN") : Res, R.X + R.W, Base + 2.0f, Ts(18.0f, 800, Net > 0 ? Green : Net < 0 ? Hot : Ice, Align::Right, Baseline::Alphabetic, true));
		std::string Story;
		if (!H.Shown.empty())
		{
			Story = H.Shown.front().first + " showed down.";
		}
		const recall::Line* Key = nullptr;
		for (const recall::Line& L : H.Lines)
		{
			if (L.What != "folds" && L.What != "checks")
			{
				Key = &L;
			}
		}
		std::string Last;
		if (Key)
		{
			static const char* const Streets[5] = {"Preflop", "Flop", "Turn", "River", "Showdown"};
			Last = std::string(Streets[std::clamp(static_cast<int>(Key->On), 0, 4)]) + ": " + Say(*Key, I.Where, I.HeroName) + ".";
		}
		const float Sw = R.X + R.W - Rw - 20.0f - X;
		if (Sw > 60.0f)
		{
			C.Text(Last.empty() ? std::string("Folded around.") : Last, X, Base - 6.0f, Ts(15.0f, 500, Ash, Align::Left, Baseline::Alphabetic, false, Sw));
			if (!Story.empty())
			{
				C.Text(Story, X, Base + 14.0f, Ts(13.0f, 500, Ice, Align::Left, Baseline::Alphabetic, false, Sw));
			}
		}
		Y += RowH;
	}
}

std::string Places(const live::PayRow& P)
{
	return P.From == P.To ? Ordinal(P.From) : std::to_string(P.From) + "\xE2\x80\x93" + std::to_string(P.To);
}

void PayoutsPage(Canvas& C, const TableCardInfo& I, const Rect& R)
{
	const live::ClockFacts* F = I.Clock;
	float Y = R.Y + 18.0f;
	if (F)
	{
		const std::string Head = std::to_string(F->Paid) + " PAID \xC2\xB7 PRIZE POOL " + WholeDollars(F->PoolCents) + " \xC2\xB7 " + (F->Projected ? "PROJECTED" : "POSTED");
		TrackedText(C, Head, R.X, Y, 15.0f, 800, Ash, 2.0f);
		std::string Sub;
		if (F->InTheMoney)
		{
			Sub = F->NextJumpAt > 0 ? "In the money. The next jump comes at " + std::to_string(F->NextJumpAt) + " left: +" + WholeDollars(F->NextJumpCents) + "." : std::string("In the money.");
		}
		else
		{
			Sub = std::to_string(F->ToTheMoney) + " more out and everyone left is paid. A min-cash is " + WholeDollars(F->MinCashCents) + ".";
		}
		C.Text(Sub, R.X, Y + 28.0f, Ts(16.0f, 400, Fa(Ice, 0.95f), Align::Left, Baseline::Alphabetic, false, R.W));
		Y += 58.0f;
	}
	if (I.Pay.empty())
	{
		C.Text("The payouts post when registration closes.", R.X, Y + 30.0f, Ts(18.0f, 400, Fa(Ash, 0.7f)));
		return;
	}
	const float RowH = 34.0f;
	const size_t PerCol = static_cast<size_t>(std::max(1.0f, std::floor((R.Y + R.H - Y - 40.0f) / RowH)));
	const size_t Cols = std::min<size_t>(3, (I.Pay.size() + PerCol - 1) / PerCol);
	const float Cw = std::min(440.0f, (R.W - Fl(Cols - 1) * 60.0f) / Fl(std::max<size_t>(1, Cols)));
	for (size_t K = 0; K < I.Pay.size() && K < PerCol * 3; ++K)
	{
		const live::PayRow& P = I.Pay[K];
		const float X = R.X + Fl(K / PerCol) * (Cw + 60.0f);
		const float Ry = Y + Fl(K % PerCol) * RowH;
		if (P.Here)
		{
			C.FillRoundRect({X - 8.0f, Ry, Cw + 16.0f, RowH - 4.0f}, 8.0f, Paint(Fa(Hot, 0.22f)));
			C.StrokeRoundRect({X - 7.5f, Ry + 0.5f, Cw + 15.0f, RowH - 5.0f}, 8.0f, Hot, 1.0f);
			TrackedText(C, "YOU", X + Cw * 0.45f, Ry + 21.0f, 13.0f, 900, Hot, 2.0f);
		}
		C.Text(Places(P), X, Ry + 21.0f, Ts(16.0f, 700, Ash));
		C.Text(WholeDollars(P.Cents), X + Cw, Ry + 21.0f, Ts(16.0f, 700, P.From == 1 ? Gold : Ash, Align::Right, Baseline::Alphabetic, true));
	}
	// Still short of the money: where the player stands.
	if (F && !F->InTheMoney && I.HeroPlace > 0)
	{
		const float Ry = R.Y + R.H - 34.0f;
		const std::string You = "YOU \xC2\xB7 " + Up(Ordinal(I.HeroPlace)) + " IF YOU WENT OUT NOW \xC2\xB7 " + std::to_string(F->ToTheMoney) + " FROM THE MONEY";
		C.FillRoundRect({R.X - 8.0f, Ry - 4.0f, R.W + 16.0f, 34.0f}, 8.0f, Paint(Fa(Hot, 0.16f)));
		TrackedText(C, You, R.X + 6.0f, Ry + 18.0f, 13.0f, 900, Hot, 1.6f);
	}
}

void FieldPage(Canvas& C, const TableCardInfo& I, const Rect& R)
{
	float Y = R.Y + 18.0f;
	if (I.Clock)
	{
		const live::ClockFacts& F = *I.Clock;
		const std::string Head = std::to_string(F.Remaining) + " OF " + std::to_string(F.Entrants) + " LEFT \xC2\xB7 AVERAGE " + Grouped(F.Average) +
			(F.Now.Bb > 0 ? " \xC2\xB7 " + chipset::InBb(F.Average, F.Now.Bb) : std::string());
		TrackedText(C, Head, R.X, Y, 15.0f, 800, Ash, 2.0f);
		Y += 30.0f;
	}
	const std::string Counts = I.CountsAt.empty() ? std::string("NO COUNTS UNTIL THE FIRST BREAK") : "COUNTS AT THE " + Up(I.CountsAt) + " BREAK";
	TrackedText(C, "PEOPLE YOU KNOW \xC2\xB7 " + Counts, R.X, Y + 10.0f, 13.0f, 800, Gold, 2.0f);
	Y += 30.0f;
	std::vector<FieldRow> Rows = I.Field;
	if (Rows.empty())
	{
		for (const std::pair<std::string, std::string>& K : I.Known)
		{
			FieldRow Fr;
			Fr.Name = K.first;
			Fr.Label = K.second;
			Rows.push_back(Fr);
		}
	}
	if (Rows.empty())
	{
		C.Text("Nobody you know is still in.", R.X, Y + 30.0f, Ts(18.0f, 400, Fa(Ash, 0.7f)));
		return;
	}
	const float RowH = 36.0f;
	const Chips Bb = I.Clock ? I.Clock->Now.Bb : 0;
	const size_t PerCol = static_cast<size_t>(std::max(1.0f, std::floor((R.Y + R.H - Y) / RowH)));
	const bool Two = Rows.size() > PerCol;
	const float Cw = std::min(620.0f, Two ? (R.W - 40.0f) * 0.5f : R.W);
	for (size_t K = 0; K < Rows.size() && K < PerCol * 2; ++K)
	{
		const FieldRow& Fr = Rows[K];
		const float X = R.X + (K >= PerCol ? Cw + 40.0f : 0.0f);
		const float Ry = Y + Fl(K % PerCol) * RowH + 22.0f;
		const float Nw = C.Text(Fr.Name, X, Ry, Ts(17.0f, 700, Ash, Align::Left, Baseline::Alphabetic, false, Cw * 0.4f));
		std::string Where;
		if (Fr.Here)
		{
			Where = "YOUR TABLE";
		}
		else if (Fr.Table > 0)
		{
			Where = "TABLE " + std::to_string(Fr.Table);
		}
		std::string Count;
		if (Fr.Count >= 0)
		{
			Count = Grouped(Fr.Count) + (Bb > 0 ? " \xC2\xB7 " + chipset::InBb(Fr.Count, Bb) : std::string());
		}
		else if (!Fr.Here && Fr.Table > 0)
		{
			Count = "NOT COUNTED YET";
		}
		C.Text(Count, X + Cw, Ry, Ts(15.0f, 700, Fr.Count >= 0 ? Ash : Ice, Align::Right, Baseline::Alphabetic, Fr.Count >= 0));
		const float Cwid = Count.empty() ? 0.0f : C.Measure(Count, 15.0f, 700, Fr.Count >= 0);
		if (!Where.empty())
		{
			C.Text(Where, X + Cw - Cwid - 18.0f, Ry, Ts(13.0f, 800, Fr.Here ? Gold : Ice, Align::Right));
		}
		// The label takes the room up to the TABLE column (cut short only when it truly doesn't fit there).
		if (!Fr.Label.empty())
		{
			const float Lx = X + Nw + 10.0f;
			const float Column = !Where.empty() ? X + Cw - Cwid - 18.0f - C.Measure(Where, 13.0f, 800) - 18.0f : X + Cw - Cwid - (Count.empty() ? 0.0f : 18.0f);
			C.Text(Up(Fr.Label), Lx, Ry, Ts(13.0f, 800, Teal, Align::Left, Baseline::Alphabetic, false, std::max(40.0f, Column - Lx)));
		}
	}
}
} // namespace tablehud_detail

void DrawTableCard(Canvas& C, const TableCardInfo& I, double Now)
{
	using namespace tablehud_detail;
	const float W = C.Width();
	const float Cw = std::min(1400.0f, W - 2.0f * Margin);
	const float Ch = 780.0f;
	const float X = W * 0.5f - Cw * 0.5f;
	const float Y = 150.0f;
	C.FillRect({0.0f, 0.0f, W, Tall}, Paint(Rgba(0, 0, 0, 0.38f)));
	C.FillRoundRect({X, Y, Cw, Ch}, 18.0f, Paint::Linear({X, Y}, {X, Y + Ch}, Fa(Navy, 0.97f), Fa(Midnight, 0.97f)));
	C.StrokeRoundRect({X + 0.5f, Y + 0.5f, Cw - 1.0f, Ch - 1.0f}, 18.0f, Fa(Ash, 0.14f), 1.0f);
	// The tabs.
	const std::vector<std::string>& Pages = TableCardPages();
	const int Page = std::clamp(I.Page, 0, static_cast<int>(Pages.size()) - 1);
	float Tx = X + 30.0f;
	const float TabY = Y + 52.0f;
	const Control* PageKey = FindControl(ControlContext::TableCard, "page");
	const std::vector<std::string> PageGlyphs = Names(I.Gamepad ? PageKey->Pad : PageKey->Key);
	Tx += Key1(C, PageGlyphs.front(), I.Gamepad, Tx, TabY + 4.0f, 0.9f) + 22.0f;
	for (size_t K = 0; K < Pages.size(); ++K)
	{
		const bool On = static_cast<int>(K) == Page;
		const float Tw = TrackedText(C, Pages[K], Tx, TabY, 17.0f, 900, On ? Ash : Fa(Ice, 0.7f), 3.0f);
		if (On)
		{
			C.FillRect({Tx, TabY + 12.0f, Tw, 3.0f}, Paint(Hot));
		}
		Tx += Tw + 36.0f;
	}
	Key1(C, PageGlyphs.back(), I.Gamepad, Tx - 16.0f, TabY + 4.0f, 0.9f);
	// The level and its clock, right of the tabs: the clock card steps aside while the card is up (TableHudInfo::CardUp).
	if (I.Clock)
	{
		const live::ClockFacts& F = *I.Clock;
		const std::string Ticking = Timer(F.OnBreak ? F.BreakSecondsLeft : F.LevelSecondsLeft);
		const std::string Level = F.OnBreak ? std::string("BREAK") : "LEVEL " + std::to_string(F.Level) + " \xC2\xB7 " + Blinds(F.Now);
		const float Right = X + Cw - 30.0f;
		const float Tw = C.Measure(Ticking, 24.0f, 700, true);
		C.Text(Ticking, Right, TabY + 2.0f, Ts(24.0f, 700, F.OnBreak ? Gold : Ash, Align::Right, Baseline::Alphabetic, true));
		if (TrackedWidth(C, Level, 13.0f, 800, 1.6f) <= Right - Tw - 16.0f - (Tx + 40.0f))
		{
			TrackedText(C, Level, Right - Tw - 16.0f, TabY, 13.0f, 800, Fa(Ice, 0.9f), 1.6f, Align::Right);
		}
	}
	C.FillRect({X + 30.0f, TabY + 16.0f, Cw - 60.0f, 1.0f}, Paint(Fa(Ash, 0.1f)));
	const Rect Body = {X + 40.0f, TabY + 40.0f, Cw - 80.0f, Ch - (TabY + 40.0f - Y) - 70.0f};
	switch (Page)
	{
	case 0: ReadsPage(C, I, Body); break;
	case 1: HandsPage(C, I, Body); break;
	case 2: PayoutsPage(C, I, Body); break;
	case 3: FieldPage(C, I, Body); break;
	default:
		DrawNightLog(C, I.Night, I.NightTitle.empty() ? std::string("THE NIGHT") : I.NightTitle, Body, Now);
		break;
	}
	// The footer: the units, and how to close it.
	const float Fy = Y + Ch - 26.0f;
	const Control* Units = FindControl(ControlContext::TableCard, "units");
	const Control* Close = FindControl(ControlContext::TableCard, "card");
	float Fx = X + 30.0f;
	Fx += ControlGlyphs(C, *Units, I.Gamepad, Fx, Fy, 0.9f) + 10.0f;
	Fx += TrackedText(C, I.Units == StackUnits::BigBlinds ? "SHOWING BIG BLINDS" : "SHOWING CHIPS", Fx, Fy - 2.0f, 13.0f, 700, Fa(Ice, 0.9f), 2.4f) + 34.0f;
	Control Let = *Close;
	Let.Hold = false;
	Fx += ControlGlyphs(C, Let, I.Gamepad, Fx, Fy, 0.9f) + 10.0f;
	TrackedText(C, Close->Label, Fx, Fy - 2.0f, 13.0f, 700, Fa(Ice, 0.9f), 2.4f);
	TrackedText(C, "YOUR DECISIONS WAIT", X + Cw - 30.0f, Fy - 2.0f, 13.0f, 700, Fa(Ice, 0.6f), 2.4f, Align::Right);
}

// ------------------------------------------------------------------ the room's clock screens

void DrawRoomClock(Canvas& C, const live::ClockFacts& F, int Page, const std::vector<std::string>& Today, const std::vector<life::RoomLeader>& Leaders,
	const std::string& HeroName, double Now, const std::string& Event, const std::vector<live::PayRow>& Pay)
{
	using namespace tablehud_detail;
	const float K = C.Height() / 576.0f;
	const float W = C.Width() / K;
	const float H = 576.0f;
	C.Save();
	C.Scale(K, K);
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Fa(Navy, 0.92f), Fa(Midnight, 0.96f)));
	// The header: the crest, the room, the event, the page.
	rlnet_detail::NetEmbercrest(C, 46.0f, 44.0f, 46.0f, Navy);
	TrackedText(C, "THE EMBERCREST", 82.0f, 52.0f, 20.0f, 900, Ash, 4.0f);
	if (!Event.empty())
	{
		TrackedText(C, Up(Event), W - 30.0f, 52.0f, 18.0f, 800, Gold, 2.4f, Align::Right);
	}
	C.FillRect({30.0f, 76.0f, W - 60.0f, 2.0f}, Paint(Fa(Hot, 0.85f)));
	const int P = std::clamp(Page, 0, 3);
	for (int Dot = 0; Dot < 4; ++Dot)
	{
		C.FillCircle(W * 0.5f - 27.0f + Fl(Dot) * 18.0f, H - 18.0f, 4.0f, Paint(Dot == P ? Hot : Fa(Ice, 0.4f)));
	}
	switch (P)
	{
	case 0:
	{
		const bool Brk = F.OnBreak;
		TrackedText(C, Brk ? "BREAK" : "LEVEL " + std::to_string(F.Level), 40.0f, 130.0f, 30.0f, 900, Brk ? Gold : Ash, 4.0f);
		if (F.HandForHand)
		{
			const float Beat = 0.7f + 0.3f * static_cast<float>(std::sin(Now * 3.0));
			C.FillRoundRect({W - 300.0f, 100.0f, 270.0f, 40.0f}, 20.0f, Paint(Fa(Hot, 0.85f * Beat)));
			TrackedText(C, "HAND FOR HAND", W - 165.0f, 127.0f, 17.0f, 900, Hex(0x14070a), 2.6f, Align::Center);
		}
		C.Text(Timer(Brk ? F.BreakSecondsLeft : F.LevelSecondsLeft), W * 0.5f, 300.0f, Ts(150.0f, 700, Brk ? Gold : Ash, Align::Center, Baseline::Alphabetic, true));
		const std::string Bl = Brk ? "LEVEL " + std::to_string(F.Level) + " \xC2\xB7 " + Blinds(F.Now) : Blinds(F.Now);
		TrackedText(C, Bl, W * 0.5f, 360.0f, 28.0f, 800, Ash, 2.4f, Align::Center);
		TrackedText(C, "NEXT \xC2\xB7 " + Blinds(F.Next), W * 0.5f, 398.0f, 18.0f, 700, Ice, 2.0f, Align::Center);
		if (!F.ColorUp.empty())
		{
			TrackedText(C, F.ColorUp, W * 0.5f, 428.0f, 15.0f, 800, Gold, 2.0f, Align::Center);
		}
		// The facts along the bottom.
		const std::pair<const char*, std::string> Cells[4] = {
			{"PLAYERS", std::to_string(F.Remaining) + " / " + std::to_string(F.Entrants)},
			{"AVERAGE", Grouped(F.Average)},
			{F.Projected ? "POOL \xC2\xB7 PROJECTED" : "PRIZE POOL", WholeDollars(F.PoolCents)},
			{F.BreakBeforeLevel > 0 ? "NEXT BREAK" : "PAID", F.BreakBeforeLevel > 0 ? Timer(F.SecondsToBreak) : std::to_string(F.Paid)},
		};
		const float Cw = (W - 60.0f) / 4.0f;
		for (int Cl = 0; Cl < 4; ++Cl)
		{
			const float Cx = 30.0f + Fl(Cl) * Cw + Cw * 0.5f;
			TrackedText(C, Cells[Cl].first, Cx, 476.0f, 13.0f, 800, Ice, 2.0f, Align::Center);
			C.Text(Cells[Cl].second, Cx, 512.0f, Ts(28.0f, 800, Ash, Align::Center, Baseline::Alphabetic, true, Cw - 10.0f));
		}
		break;
	}
	case 1:
	{
		const std::string Head = std::to_string(F.Paid) + " PAID \xC2\xB7 " + WholeDollars(F.PoolCents) + (F.Projected ? " \xC2\xB7 PROJECTED" : " \xC2\xB7 POSTED");
		TrackedText(C, Head, 40.0f, 124.0f, 22.0f, 900, Ash, 3.0f);
		if (Pay.empty())
		{
			TrackedText(C, "MIN CASH " + WholeDollars(F.MinCashCents), 40.0f, 180.0f, 22.0f, 800, Gold, 2.0f);
			break;
		}
		const size_t PerCol = 8;
		const float Cw = (W - 80.0f - 60.0f) / 3.0f;
		for (size_t Row = 0; Row < Pay.size() && Row < PerCol * 3; ++Row)
		{
			const live::PayRow& Pr = Pay[Row];
			const float X = 40.0f + Fl(Row / PerCol) * (Cw + 30.0f);
			const float Y = 172.0f + Fl(Row % PerCol) * 40.0f;
			C.Text(Places(Pr), X, Y, Ts(20.0f, 700, Ash));
			C.Text(WholeDollars(Pr.Cents), X + Cw, Y, Ts(20.0f, 800, Pr.From == 1 ? Gold : Ash, Align::Right, Baseline::Alphabetic, true));
		}
		break;
	}
	case 2:
	{
		TrackedText(C, "TODAY AT THE EMBERCREST", 40.0f, 124.0f, 22.0f, 900, Ash, 3.0f);
		float Y = 180.0f;
		for (const std::string& Line : Today)
		{
			std::vector<std::string> Cols;
			std::string Cur;
			for (const char Ch : Line)
			{
				if (Ch == '\t')
				{
					Cols.push_back(Cur);
					Cur.clear();
				}
				else
				{
					Cur.push_back(Ch);
				}
			}
			Cols.push_back(Cur);
			C.Text(Cols[0], 40.0f, Y, Ts(22.0f, 700, Ice, Align::Left, Baseline::Alphabetic, true));
			if (Cols.size() > 1)
			{
				C.Text(Up(Cols[1]), 210.0f, Y, Ts(22.0f, 800, Ash, Align::Left, Baseline::Alphabetic, false, W - 210.0f - 240.0f));
			}
			if (Cols.size() > 2)
			{
				const bool Running = Up(Cols[2]).find("RUNNING") != std::string::npos;
				TrackedText(C, Up(Cols[2]), W - 40.0f, Y, 15.0f, 800, Running ? Hot : Gold, 2.0f, Align::Right);
			}
			Y += 50.0f;
			if (Y > H - 60.0f)
			{
				break;
			}
		}
		if (Today.empty())
		{
			C.Text("The card room's dark today.", 40.0f, 200.0f, Ts(22.0f, 500, Ice));
		}
		break;
	}
	default:
		if (Leaders.empty())
		{
			TrackedText(C, "THIS MONTH'S LEADERS", 40.0f, 124.0f, 22.0f, 900, Ash, 3.0f);
			C.Text("No results posted this month yet.", 40.0f, 200.0f, Ts(22.0f, 500, Ice));
		}
		else
		{
			badgeart::RoomLeaders(C, {30.0f, 92.0f, W - 60.0f, H - 130.0f}, Leaders, "THIS MONTH", HeroName, Now);
		}
		break;
	}
	C.Restore();
}

// ------------------------------------------------------------------ the copy

namespace desk_detail
{
/** The desk's and the cage's own sentences (DeskScreen.cpp). */
std::vector<std::pair<std::string, std::string>> DeskCopy();
} // namespace desk_detail

std::string TableSettingHelp(const std::string& Key, bool Gamepad)
{
	if (Key == "voices")
	{
		return "The room's voices: the players at your table, the dealers, the floor on the PA and the rail.";
	}
	if (Key == "captions")
	{
		return "What the room says, on screen. Dialogue is your table, the floor and anyone talking to you. Everything adds the tables around you, the rail and the room's sounds.";
	}
	if (Key == "captionsize")
	{
		return "How big the captions draw.";
	}
	if (Key == "captionbg")
	{
		return "How dark the box behind a caption is. Lighter shows more of the room; darker reads easier.";
	}
	if (Key == "speakernames")
	{
		return "Who's talking, before what they say.";
	}
	if (Key == "tabletalk")
	{
		return "How much the people at a live table talk between hands.";
	}
	if (Key == "tablepace")
	{
		return std::string("At a live table: how long the other players take to act, and how quickly a hand you've folded plays out. Your own decisions always wait for you. ") +
			(Gamepad ? "Right on the d-pad changes it at the table." : "T changes it at the table.");
	}
	if (Key == "stackreadout")
	{
		return "SHOWN: every stack at your table, counted, all the time. THE ROOM'S WAY: what you could tell sitting there. A glance gets you close, a long look counts it down, and the dealer counts any all-in you face. YOUR EYES: no numbers; read the chips.";
	}
	if (Key == "stackunits")
	{
		return std::string("The big number on a stack. The other one sits beside it. ") + (Gamepad ? "Y flips it while the table card is up." : "U flips it at the table.");
	}
	if (Key == "turncue")
	{
		return "How you know the action's on you, beyond the buttons.";
	}
	if (Key == "hudscale")
	{
		return "How big the table's panels draw: the clock card, your stack, the ladder and the plates.";
	}
	return std::string();
}

std::vector<std::pair<std::string, std::string>> TableUiCopy()
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (const char* Key : {"voices", "captions", "captionsize", "captionbg", "speakernames", "tabletalk", "tablepace", "stackreadout", "stackunits", "turncue", "hudscale"})
	{
		Out.push_back({std::string("settings.") + Key + ".pad", TableSettingHelp(Key, true)});
		Out.push_back({std::string("settings.") + Key + ".keys", TableSettingHelp(Key, false)});
	}
	const std::vector<std::pair<std::string, std::string>> Lines = {
		{"controls.pad", ControlsNotice(true)},
		{"controls.keys", ControlsNotice(false)},
		{"reads.empty.pad", "Nothing yet. Study a face with the right trigger while the hand plays out, then see what they turn over."},
		{"reads.empty.keys", "Nothing yet. Study a face with the right mouse button while the hand plays out, then see what they turn over."},
		{"hands.empty", "No hands yet tonight. The first one's coming."},
		{"payouts.empty", "The payouts post when registration closes."},
		{"field.empty", "Nobody you know is still in."},
		{"night.empty", "Nothing said yet. The room's still finding its seats."},
		{"today.empty", "The card room's dark today."},
		{"leaders.empty", "No results posted this month yet."},
		{"payouts.money", "In the money. The next jump comes at 12 left: +$53."},
		{"payouts.short", "3 more out and everyone left is paid. A min-cash is $243."},
		{"hands.folded", "Folded around."},
		{"hands.showed", "Rick showed down."},
	};
	Out.insert(Out.end(), Lines.begin(), Lines.end());
	const std::vector<std::pair<std::string, std::string>> Desk = desk_detail::DeskCopy();
	Out.insert(Out.end(), Desk.begin(), Desk.end());
	return Out;
}
} // namespace ui
} // namespace ss
