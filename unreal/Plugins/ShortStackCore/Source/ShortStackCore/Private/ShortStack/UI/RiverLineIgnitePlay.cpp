// Update 3 (SPEC §2.4, §2.6, §7.2): Ignite's REWARDS and FUSE pages, ui-ignite's file: the Prize Counter, the Flame, Dares,
// the Spark Wheel, Crackers, the Wristband, the level-up banner, the Locker and the Sticker Book, the FUSE booth, its
// reveal and the 3-max table's marquee and fuse clock, the Fine print. This file has no RiverLine member function of its
// own: it reaches the client's private state and its gatekeepers through rlignite_detail::Access (RiverLine.h's friend; its
// forwarders are in IgniteArt.h).
//
// G2 (§2.9): the loops read only core's view (Session::SitePlayView, through IgniteLoopsFrom, the one adapter) and the
// claims go through the Session's methods on Clicked (SpinWheel, OpenCracker, ClaimDare, ClaimTier, BuyPrize, Equip,
// LightFuse); an overlay only animates the result core returned (W2). While the view isn't G2's (Live false) every surface
// keeps its G1 state: REWARDS is the Welcome bonus, the centre button reads FLIP.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "../Game/SitesDetail.h"
#include "IgniteArt.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace rlignite_detail
{
// ------------------------------------------------------------------ the loops: the one adapter

IgniteLoops IgniteLoopsFrom(const Session& S, double World)
{
	// core-siteplay's View (SPEC §7.3) is read here and nowhere else: the pages draw IgniteLoops, a copy in their shape. While
	// the View says the loops aren't on (Ig.On false) every surface keeps its G1 state.
	IgniteLoops L;
	const site::play::View V = S.SitePlayView(net::SiteId::Ignite);
	const site::play::IgniteView& I = V.Ig;
	if (!I.On)
	{
		return L;
	}
	L.Live = true;
	L.Started = I.Started;
	L.FuseOn = I.FuseOn;
	L.OnIce = I.OnIce;
	L.Band = I.Band;
	L.Sparks = I.Sparks;
	L.Streak = I.Streak;
	L.Best = I.Best;
	L.Matches = I.Matches;
	L.Mult = I.Mult;
	L.LitToday = I.LitToday;
	L.Nudge = I.Nudge;
	for (size_t D = 0; D < 7; ++D)
	{
		L.Week[D] = I.Week[D];
	}
	for (size_t K = 0; K < 3; ++K)
	{
		const site::play::DareView& Dv = I.Dares[K];
		IgniteDare& D = L.Dares[K];
		D.Id = Dv.Id;
		D.Tier = Dv.Tier;
		D.Label = Dv.Label;
		D.Prog = Dv.Prog;
		D.Needs = Dv.Needs;
		D.Entries = Dv.Entries;
		D.Sparks = Dv.Sparks;
		D.Done = Dv.Done;
		D.Claimed = Dv.Claimed;
		D.Held = Dv.Held;
	}
	L.DaresReset = I.DaresReset;
	L.WeekClears = I.WeekClears;
	L.ClearCracker = I.ClearCracker;
	L.ClearPaid = I.ClearPaid;
	L.EntriesToday = I.EntriesToday;
	L.RingsLit = I.RingsLit;
	L.SpinReady = I.SpinReady;
	L.Spun = I.Spun;
	L.SpunHeld = I.SpunHeld;
	L.SpunRing = I.SpunRing;
	L.SpunWedge = I.SpunWedge;
	L.MoreForNext = I.MoreForNext;
	L.NearRing = I.NearGoal;
	for (size_t R = 0; R < 3; ++R)
	{
		for (const site::play::WedgeView& Wv : I.Rings[R])
		{
			IgniteWedge W;
			W.Label = Wv.Label;
			W.Sparks = Wv.Sparks;
			W.TicketId = Wv.TicketId;
			W.FaceCents = Wv.FaceCents;
			W.P = Wv.P;
			L.Rings[R].push_back(W);
		}
	}
	L.Crackers = I.Crackers;
	for (size_t T = 0; T < 3; ++T)
	{
		L.CrackerOdds[T] = IgniteCrackerOddsFrom(I.CrackerOdds[T]);
	}
	for (const site::play::OpenView& Ov : I.Opened)
	{
		IgniteOpened O;
		O.BoxId = Ov.BoxId;
		O.Tier = Ov.Tier;
		O.Prize = Ov.Prize;
		O.Sparks = Ov.Sparks;
		O.TicketId = Ov.TicketId;
		O.LookSlot = Ov.LookSlot;
		O.Look = Ov.Look;
		O.Sticker = Ov.Sticker;
		O.Held = Ov.Held;
		L.Opened.push_back(O);
	}
	L.Tier = I.Tier;
	L.BandSparks = I.BandSparks;
	L.Theme = I.Theme;
	L.DaysLeft = I.DaysLeft;
	for (size_t T = 0; T < 30; ++T)
	{
		const site::play::TierView& Tv = I.Tiers[T];
		IgniteTier& Tr = L.Tiers[T];
		Tr.Tier = Tv.Tier;
		Tr.Needs = Tv.Needs;
		Tr.Reward = Tv.Reward;
		Tr.Kind = Tv.Kind;
		Tr.Reached = Tv.Reached;
		Tr.Claimed = Tv.Claimed;
		Tr.Held = Tv.Held;
	}
	L.Level = I.Level;
	L.Title = I.Title;
	L.LevelSparks = I.LevelSparks;
	L.LevelFrom = I.LevelFrom;
	L.LevelTo = I.LevelTo;
	for (const site::play::PrizeView& Pv : I.Prizes)
	{
		IgnitePrize Pz;
		Pz.Item = Pv.Item;
		Pz.Label = Pv.Label;
		Pz.Sparks = Pv.Sparks;
		Pz.TicketId = Pv.TicketId;
		Pz.FaceCents = Pv.FaceCents;
		Pz.LookSlot = Pv.LookSlot;
		Pz.Look = Pv.Look;
		Pz.Tab = Pv.Tab;
		Pz.Owned = Pv.Owned;
		L.Prizes.push_back(Pz);
	}
	L.Worn = I.Worn;
	for (const site::play::LookView& Lv : I.Locker)
	{
		IgniteLook Lk;
		Lk.Slot = Lv.Slot;
		Lk.Look = Lv.Look;
		Lk.Name = Lv.Name;
		Lk.How = Lv.How;
		Lk.ColorA = Lv.ColorA;
		Lk.ColorB = Lv.ColorB;
		Lk.Owned = Lv.Owned;
		Lk.Worn = Lv.Worn;
		L.Locker.push_back(Lk);
	}
	for (const site::play::StickerView& Sv : I.Book)
	{
		IgniteStickerRow St;
		St.Id = Sv.Id;
		St.Name = Sv.Name;
		St.How = Sv.How;
		St.Got = Sv.Got;
		St.Secret = Sv.Secret;
		L.Book.push_back(St);
	}
	L.Fuse = I.Fuse;
	L.FuseTicketIds = I.FuseTicketIds;
	for (int T = 0; T < 3; ++T)
	{
		if (L.FuseTicketIds[static_cast<size_t>(T)].empty())
		{
			L.FuseTicketIds[static_cast<size_t>(T)] = fuse::TicketId(T);
		}
	}
	L.Headliners = I.Headliners;
	L.Beefs = I.Beefs;
	L.BeefName = I.BeefName;
	L.ScalpLife = I.ScalpLife;
	L.Guest = I.Guest;
	L.CrowdKnows = I.CrowdKnows;
	for (const site::play::MomentView& Mv : I.Moments)
	{
		IgniteMoment M;
		M.Kind = Mv.Kind;
		M.Name = Mv.Name;
		M.HandId = Mv.HandId;
		M.Sparks = Mv.Sparks;
		M.Equity = Mv.Equity;
		M.World = Mv.World;
		M.Held = Mv.Held;
		L.Moments.push_back(M);
	}
	L.ThemeNight = I.ThemeNight;
	L.ThemeName = I.ThemeName;
	L.ThemeTwist = I.ThemeTwist;
	L.Last.EventId = I.Last.EventId;
	L.Last.Base = I.Last.Base;
	L.Last.Sparks = I.Last.Sparks;
	L.Last.Flame = I.Last.Flame;
	L.Last.Theme = I.Last.Theme;
	L.Last.Banked = I.Last.Banked;
	for (size_t K = 0; K < 3; ++K)
	{
		L.Last.DareTicks[K] = I.Last.DareTicks[K];
	}
	L.Last.RingLit = I.Last.RingLit;
	L.Last.LevelFrom = I.Last.LevelFrom;
	L.Last.LevelTo = I.Last.LevelTo;
	L.Last.TierFrom = I.Last.TierFrom;
	L.Last.TierTo = I.Last.TierTo;
	// The Governor on Ignite (the Fine print): V.Gov is the site asked for.
	const site::play::GovView& G = V.Gov;
	L.Rake30 = G.Rake30;
	L.Promo30 = G.Promo30;
	L.Share = G.Share;
	L.Ceiling = G.Ceiling;
	L.Welcome = G.Welcome;
	L.WeekFace = G.WeekFace;
	L.WeekCap = G.WeekCap;
	L.MondayBank = std::max(G.MondayBank, I.MondayBank);
	for (const site::play::HoldView& Hv : G.Holds)
	{
		IgniteHold H;
		H.What = Hv.What;
		H.Cents = Hv.Cents;
		H.Covered = Hv.Covered;
		H.RakeLeft = Hv.RakeLeft;
		H.Entries = Hv.Entries;
		H.Day = Hv.Day;
		H.Sparks = Hv.Sparks;
		H.SparksOwed = Hv.SparksOwed;
		H.Reset = Hv.Reset;
		L.Holds.push_back(H);
	}
	for (const site::play::GovMonth& Gm : G.Months)
	{
		L.Months.push_back({Gm.Name, Gm.Promo});
	}
	L.GovLine = G.Line;
	// The Flame's week (G2_AUDIT L9): core marks every past day of this week with no flame, rest day or match as missed, the
	// days before the player's first paid entry here included (a Wednesday start showed MON and TUE red). A day before the
	// first rake the Governor's ring holds (30 days, today last; every paid entry pays rake) was never a flame day to miss:
	// it reads as a plain day.
	{
		const int Today = site::play::PlayDay(World);
		const int Wd = net::Weekday(Today);
		int FirstRake = Today + 1;
		for (size_t K = 0; K < G.RakeByDay.size(); ++K)
		{
			if (G.RakeByDay[K] > 0)
			{
				FirstRake = G.FirstDay + static_cast<int>(K);
				break;
			}
		}
		for (int D = 0; D < Wd && D < 7; ++D)
		{
			if (L.Week[static_cast<size_t>(D)] == 3 && Today - (Wd - D) < FirstRake)
			{
				L.Week[static_cast<size_t>(D)] = 0;
			}
		}
	}
	return L;
}

IgniteCrackerOdds IgniteCrackerOddsFrom(const site::play::CrackerOddsView& V)
{
	IgniteCrackerOdds O;
	O.Tier = V.Tier;
	O.Name = V.Name;
	O.From = V.From;
	O.TicketP = V.TicketP;
	O.LookP = V.LookP;
	O.DuplicateSparks = V.DuplicateSparks;
	O.EvSparks = V.EvSparks;
	for (const site::play::CrackerOddsRow& Rv : V.Rows)
	{
		IgniteCrackerRow R;
		R.Part = Rv.Part;
		R.Label = Rv.Label;
		R.Sparks = Rv.Sparks;
		R.TicketId = Rv.TicketId;
		R.FaceCents = Rv.FaceCents;
		R.LookSlot = Rv.LookSlot;
		R.Look = Rv.Look;
		R.P = Rv.P;
		O.Rows.push_back(R);
	}
	return O;
}

const IgniteLoops& IgniteLoopsOf(RiverLine& RL, double Now)
{
	IgniteUi& U = IgniteOf(&RL);
	if (U.Faked)
	{
		return U.Fake;
	}
	if (U.LoopsAt != Now)
	{
		U.Loops = IgniteLoopsFrom(Access::S(RL), Access::World(RL));
		U.LoopsAt = Now;
	}
	return U.Loops;
}

void IgniteFakeLoops(const RiverLine* RL, const IgniteLoops* L)
{
	IgniteUi& U = IgniteOf(RL);
	if (U.Owner != &Access::SessionOf(*RL))
	{
		// The laptop's first frame would start its side state afresh (a new career's): start it now, so the stand-in stays.
		U = IgniteUi();
		U.Owner = &Access::SessionOf(*RL);
	}
	U.Faked = L != nullptr;
	U.Fake = L ? *L : IgniteLoops();
	U.LoopsAt = -1.0;
}

int64_t IgniteBaseSparks(Chips BuyInCents)
{
	// 10 Sparks for every cent of Ignite rake (SPEC §2.6 F1): $1.10 -> 120, $5.50 -> 610, $22 -> 2,420.
	return BuyInCents <= 0 ? 0 : static_cast<int64_t>(net::Site(net::SiteId::Ignite).Definition().Rake.Fee(BuyInCents)) * 10;
}

void IgniteNoteRefusal(IgniteUi& U, const std::string& Id, const std::string& Why, double Now)
{
	U.Note = Why;
	U.NoteId = Id;
	U.NoteAt = Now;
}

std::string IgniteNearRing(const IgniteLoops& L)
{
	if (!L.NearRing.empty())
	{
		return L.NearRing; // core's own words
	}
	return IgniteNearRingShort(L);
}

std::string IgniteNearRingShort(const IgniteLoops& L)
{
	if (L.RingsLit >= 3)
	{
		return std::string();
	}
	const int More = std::max(1, std::min(2, L.MoreForNext));
	return std::to_string(More) + (More == 1 ? " more entry lights " : " more light ") + igniteart::IgniteRingName(L.RingsLit + 1);
}

std::string IgniteNearGoal(const IgniteLoops& L, double World)
{
	(void)World;
	if (!L.Live || L.OnIce)
	{
		return std::string();
	}
	if (L.RingsLit < 3)
	{
		return IgniteNearRingShort(L);
	}
	const int64_t To = std::max<int64_t>(0, L.LevelTo - L.LevelSparks);
	return Grouped(To) + " Sparks to Level " + std::to_string(L.Level + 1);
}

namespace
{
// Helpers private to this file (Igp: a unity build joins this file with RiverLineIgnite.cpp, whose own are Ignite*).

float IgpEase(double T)
{
	return static_cast<float>(EaseOutCubic(Clamp01(T)));
}

/** Ease-out-back from 0.85 (SPEC §2.3: entrances). */
float IgpBack(double T)
{
	const double U = Clamp01(T);
	const double C1 = 1.70158;
	const double C3 = C1 + 1.0;
	return static_cast<float>(1.0 + C3 * std::pow(U - 1.0, 3.0) + C1 * std::pow(U - 1.0, 2.0));
}

/** A breathing pulse on the hour's beat (0..1). */
float IgpPulse(double Tm, const igniteart::IgniteTones& K)
{
	return static_cast<float>(0.5 + 0.5 * std::cos(motion::Beat(Tm, K.Bpm) * 2.0 * 3.14159265358979));
}

/** A rimmed card on the plum ground (the pages' panels). */
void IgpCard(Canvas& C, const Rect& R, const igniteart::IgniteTones& K, float Radius = 24.0f)
{
	C.FillRoundRect({R.X, R.Y + 5.0f, R.W, R.H}, Radius, Rgba(0, 0, 0, 0.28f));
	C.FillRoundRect(R, Radius, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(K.PanelTop), Hex(K.PanelBot)));
	C.StrokeRoundRect(R, Radius, Hex(K.Rim), 1.0f);
}

/** A pill button in Ignite's look (SitesButton's, so it squashes and reads the label ink by contrast). */
bool IgpButton(RiverLine& RL, const std::string& Id, const Rect& R, const std::string& Label, ButtonKind Kind, float Size, bool Enabled, const std::string& Sub = std::string())
{
	ButtonOpts O;
	O.Kind = Kind;
	O.Size = Size;
	O.Enabled = Enabled;
	O.Sub = Sub;
	return rlsites_detail::SitesButton(RL.UI, Access::C(RL), Id, R, Label, O, rlsites_detail::SitesLook(net::SiteId::Ignite));
}

/** The width IgpTimesText takes for figure N at Size (a pill sized before it's drawn). */
float IgpTimesWidth(const Canvas& C, float Size, const std::string& N)
{
	return Size * 0.46f + Size * 0.08f + C.Measure(N, Size, Font::BlackItalic);
}

/** "x1.20": a vector cross and the figure in BlackItalic (the glyph rule: no multiplication sign in strings). Returns the width. */
float IgpTimesText(Canvas& C, float X, float Y, float Size, const std::string& N, const Color& Col, Align A = Align::Left)
{
	using namespace igniteart;
	const float Xw = Size * 0.46f;
	const float W = IgpTimesWidth(C, Size, N);
	float X0 = X;
	if (A == Align::Center)
	{
		X0 = X - W / 2.0f;
	}
	else if (A == Align::Right)
	{
		X0 = X - W;
	}
	const float Cy = Y - Size * 0.32f;
	const float H = Xw * 0.5f;
	const float Th = std::max(1.5f, Size * 0.1f);
	C.StrokePolyline({{X0 + H * 0.2f + 0.0f, Cy - H}, {X0 + Xw - H * 0.2f, Cy + H}}, false, Col, Th, true);
	C.StrokePolyline({{X0 + Xw - H * 0.2f + Size * 0.03f, Cy - H}, {X0 + H * 0.2f - Size * 0.03f, Cy + H}}, false, Col, Th, true);
	C.Text(N, X0 + Xw + Size * 0.08f, Y, IgniteBi(Size, Col));
	return W;
}

/** "x25" with the number grouped ("x1,000"). */
float IgpTimes(Canvas& C, float X, float Y, float Size, int M, const Color& Col, Align A = Align::Left)
{
	return IgpTimesText(C, X, Y, Size, Grouped(M), Col, A);
}

/**
 * A wheel's prize riding upright in the band between two of its rings (G2_AUDIT L9: never on a ring, lit or dim): centred at
 * radius Mid on angle A, shrunk (to 0.6 of Size at most) until the text's reach along the radius stays within Room of Mid.
 */
void IgpWheelLabel(Canvas& C, const std::string& Label, float Cx, float Cy, float Mid, float Room, float A, float Size, const Color& Col)
{
	const float Reach = std::fabs(std::cos(A)) * C.Measure(Label, Size, Font::BlackItalic) * 0.5f + std::fabs(std::sin(A)) * Size * 0.36f;
	const float Sz = Reach > Room ? Size * std::max(0.6f, Room / Reach) : Size;
	C.Text(Label, Cx + Mid * std::cos(A), Cy + Mid * std::sin(A), igniteart::IgniteBi(Sz, Col, Align::Center, Baseline::Middle));
}

/** A refusal core gave for button Id, drawn under it for 3 s ("" when none is up). */
void IgpNote(Canvas& C, const IgniteUi& U, const std::string& Id, float X, float Y, float MaxW, double Now, Align A = Align::Left)
{
	if (U.NoteId != Id || U.Note.empty() || Now - U.NoteAt > 3.0 || Now < U.NoteAt)
	{
		return;
	}
	C.Text(U.Note, X, Y, igniteart::IgniteTx(12.5f, 700, Hex(igniteart::IgRed), A, Baseline::Alphabetic, MaxW));
}

/** The dark ink for a label on a light fill (cream on coral reads under 3:1). */
Color IgpInk()
{
	return Hex(0x2a0d14);
}

} // namespace

float IgniteTimes(Canvas& C, float X, float Y, float Size, int M, const Color& Col, Align A)
{
	return IgpTimes(C, X, Y, Size, M, Col, A);
}

float IgniteTimesText(Canvas& C, float X, float Y, float Size, const std::string& N, const Color& Col, Align A)
{
	return IgpTimesText(C, X, Y, Size, N, Col, A);
}

namespace
{
/** A multiplier in copy: an 'x' that starts a word, followed by a digit ("x25", "x1.20") or standing alone before one
 * ("610 x 1.20"); never "max5" or "6x". */
bool IgpTimesAt(const std::string& S, size_t I)
{
	if (S[I] != 'x' || (I > 0 && S[I - 1] != ' ' && S[I - 1] != '('))
	{
		return false;
	}
	const size_t J = I + 1 < S.size() && S[I + 1] == ' ' ? I + 2 : I + 1;
	return J < S.size() && S[J] >= '0' && S[J] <= '9';
}

float IgpRunWidth(const Canvas& C, const std::string& S, const TextStyle& St)
{
	return St.Face >= 0 ? C.Measure(S, St.Size, static_cast<Font>(St.Face)) : C.Measure(S, St.Size, St.Weight, St.Mono);
}
} // namespace

bool IgniteHasTimes(const std::string& S)
{
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (IgpTimesAt(S, I))
		{
			return true;
		}
	}
	return false;
}

float IgniteLineTimes(Canvas& C, const std::string& S, float X, float Y, const TextStyle& St)
{
	if (!IgniteHasTimes(S))
	{
		return C.Text(S, X, Y, St);
	}
	// The runs between the multipliers; each run after the first follows a vector cross (its 'x' dropped).
	std::vector<std::string> Runs(1);
	for (size_t I = 0; I < S.size(); ++I)
	{
		if (IgpTimesAt(S, I))
		{
			Runs.emplace_back();
		}
		else
		{
			Runs.back() += S[I];
		}
	}
	const float Xw = St.Size * 0.46f;
	const float Gap = St.Size * 0.08f;
	float W = 0.0f;
	for (size_t K = 0; K < Runs.size(); ++K)
	{
		W += (K > 0 ? Xw + Gap : 0.0f) + IgpRunWidth(C, Runs[K], St);
	}
	if (St.MaxWidth > 0.0f && W > St.MaxWidth)
	{
		return C.Text(S, X, Y, St); // too long for its room: the plain run, ellipsized as before
	}
	float X0 = St.HAlign == Align::Center ? X - W / 2.0f : St.HAlign == Align::Right ? X - W : X;
	const float Cy = St.VAlign == Baseline::Middle ? Y : Y - St.Size * 0.32f;
	const float H = Xw * 0.5f;
	const float Th = std::max(1.5f, St.Size * 0.1f);
	TextStyle Run = St;
	Run.HAlign = Align::Left;
	Run.MaxWidth = 0.0f;
	for (size_t K = 0; K < Runs.size(); ++K)
	{
		if (K > 0)
		{
			C.StrokePolyline({{X0 + H * 0.2f, Cy - H}, {X0 + Xw - H * 0.2f, Cy + H}}, false, St.Col, Th, true);
			C.StrokePolyline({{X0 + Xw - H * 0.2f, Cy - H}, {X0 + H * 0.2f, Cy + H}}, false, St.Col, Th, true);
			X0 += Xw + Gap;
		}
		if (!Runs[K].empty())
		{
			C.Text(Runs[K], X0, Y, Run);
			X0 += IgpRunWidth(C, Runs[K], St);
		}
	}
	return W;
}

float IgniteParagraphTimes(Canvas& C, const std::string& Text, float X, float Y, float MaxW, float Size, int Weight, const Color& Col, float LineH, int MaxLines)
{
	if (!IgniteHasTimes(Text))
	{
		return rlnet_detail::NetParagraph(C, Text, X, Y, MaxW, Size, Weight, Col, LineH, MaxLines);
	}
	// NetParagraph's wrap and its last line, each line drawn with its crosses.
	const std::vector<std::string> Lines = rlnet_detail::NetWrap(C, Text, MaxW, Size, Weight);
	for (size_t I = 0; I < Lines.size() && static_cast<int>(I) < MaxLines; ++I)
	{
		std::string Ln = Lines[I];
		if (static_cast<int>(I) == MaxLines - 1 && I + 1 < Lines.size())
		{
			Ln += " " + Lines[I + 1];
		}
		IgniteLineTimes(C, Ln, X, Y, Ts(Size, Weight, Col, Align::Left, Baseline::Alphabetic, false, MaxW));
		Y += LineH;
	}
	return Y;
}

namespace
{
const char* const IgpTierNames[3] = {"EASY", "MEDIUM", "HARD"};
const uint32_t IgpTierCols[3] = {igniteart::IgLime, igniteart::IgSky, igniteart::IgPink};
const char* const IgpCrackerNames[3] = {"Paper", "Neon", "Gold"};
const char* const IgpDays[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

/** "1 MORE ENTRY", "2 MORE ENTRIES": a Dare whose own target is met still waits on today's paid entries (core's floor). */
std::string IgpMoreEntries(int N)
{
	return std::to_string(N) + (N == 1 ? " MORE ENTRY" : " MORE ENTRIES");
}
} // namespace

// ------------------------------------------------------------------ the marquee's Sparks and Flame pills (SPEC §2.4 chrome)

void IgniteSparksPill(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview, double World)
{
	using namespace igniteart;
	(void)World;
	(void)K;
	Canvas& C = Access::C(RL);
	const Rect R{230.0f, 18.0f, 170.0f, 36.0f};
	const Ui::ClickState St = RL.UI.Clickable("igspk", R, !Preview);
	// "+610" floats up when Sparks land (the balance grew since the last frame that saw it).
	if (U.SparksSeen >= 0 && L.Sparks > U.SparksSeen && !Preview)
	{
		U.SparksDelta = L.Sparks - U.SparksSeen;
		U.SparksAt = Now;
	}
	U.SparksSeen = L.Sparks;
	IgnitePill(C, R, Hex(0x1b2610, St.Hover ? 1.0f : 0.92f), Hex(IgLime, 0.55f), 1.5f);
	IgniteSpark(C, 250.0f, 36.0f, 10.0f, Hex(IgLime));
	C.Text(Grouped(L.Sparks), 266.0f, 44.0f, IgniteBi(18.0f, Hex(IgLime), Align::Left, Baseline::Alphabetic, 84.0f));
	C.Text("SPARKS", 392.0f, 41.0f, IgniteTx(8.5f, 800, Hex(IgLime, 0.75f), Align::Right));
	const double Since = Now - U.SparksAt;
	if (Since >= 0.0 && Since < 1.4 && U.SparksDelta > 0 && !Preview)
	{
		const float Rise = IgpEase(Since / 1.4);
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * static_cast<float>(Clamp01((1.4 - Since) / 0.5)));
		C.Text("+" + Grouped(U.SparksDelta), 330.0f, 72.0f + 16.0f - 22.0f * Rise, IgniteBi(16.0f, Hex(IgLime), Align::Center));
		C.SetAlpha(A0);
	}
	if (St.Clicked)
	{
		U.Rewards = 2;
		Access::S(RL).ShowLobby();
		Access::OpenPage(RL, RiverLine::Page::Promotions, Now);
	}
}

void IgniteFlamePill(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview, double World)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const Rect R{412.0f, 18.0f, 120.0f, 36.0f};
	const Ui::ClickState St = RL.UI.Clickable("igflame", R, !Preview);
	const double Hour = std::fmod(std::max(0.0, World), net::MinutesPerDay) / 60.0;
	// After 6 PM with no flame day yet the flame flickers (the nudge, F2); on ice it wears a frost rim.
	const bool Nudge = L.Nudge || (!L.LitToday && !L.OnIce && Hour >= 18.0 && L.Streak > 0);
	const float Flick = Nudge && !Preview ? 0.75f + 0.25f * static_cast<float>(std::sin(Now * 17.0) * std::sin(Now * 5.3)) : 1.0f;
	IgnitePill(C, R, Hex(L.OnIce ? 0x16222e : 0x2b140f, St.Hover ? 1.0f : 0.92f), L.OnIce ? Hex(0xcfe8f6, 0.9f) : Hex(IgTang, 0.55f), L.OnIce ? 2.5f : 1.5f);
	const float A0 = C.GetAlpha();
	C.SetAlpha(A0 * Flick);
	IgniteFlameLook(C, 434.0f, 49.0f, 20.0f, L.Streak, L.OnIce);
	C.SetAlpha(A0);
	const float Dw = C.Text(std::to_string(L.Streak), 452.0f, 44.0f, IgniteBi(18.0f, Hex(K.Cream)));
	IgpTimesText(C, 458.0f + Dw, 43.0f, 13.0f, Fixed(L.Mult, 2), L.OnIce ? Hex(0xcfe8f6) : Hex(IgSun));
	if (St.Clicked)
	{
		U.Rewards = 0;
		Access::S(RL).ShowLobby();
		Access::OpenPage(RL, RiverLine::Page::Promotions, Now);
	}
}

// ------------------------------------------------------------------ the dock's strips (SPEC §2.4 chrome)

void IgniteDockBand(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const Rect R{24.0f, 884.0f, 340.0f, 64.0f};
	const Ui::ClickState St = RL.UI.Clickable("dockband", R, !Preview);
	C.FillRoundRect(R, 20.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(St.Hover ? K.Panel : K.PanelTop), Hex(K.PanelBot)));
	C.StrokeRoundRect(R, 20.0f, Hex(St.Hover ? K.Coral : K.Rim), 1.0f);
	const float Ww = C.Text("WRISTBAND", 44.0f, 908.0f, IgniteBi(14.0f, Hex(K.Cream)));
	IgniteHexagon(C, 64.0f + Ww, 903.0f, 11.0f, Hex(IgViolet));
	C.Text("T" + std::to_string(L.Tier), 64.0f + Ww, 903.5f, IgniteTx(9.5f, 900, Hex(IgCream), Align::Center, Baseline::Middle));
	C.Text(rlnet_detail::NetUpper(L.Theme) + " \xC2\xB7 " + std::to_string(L.DaysLeft) + (L.DaysLeft == 1 ? " DAY LEFT" : " DAYS LEFT"), 346.0f, 907.0f, IgniteTx(9.5f, 800, pal::Muted, Align::Right));
	// The rope to the next tier, the month's base Sparks over its need.
	int64_t From = 0;
	int64_t To = L.Tiers[0].Needs;
	for (const IgniteTier& T : L.Tiers)
	{
		if (T.Needs <= L.BandSparks)
		{
			From = T.Needs;
		}
		else
		{
			To = T.Needs;
			break;
		}
	}
	const double Frac = L.Tier >= 30 ? 1.0 : To > From ? Clamp01(static_cast<double>(L.BandSparks - From) / static_cast<double>(To - From)) : 0.0;
	IgniteRope(C, 44.0f, 248.0f, 930.0f, 3.0f, Frac, 7u);
	C.Text(L.Tier >= 30 ? std::string("ALL 30") : Grouped(L.BandSparks) + " / " + Grouped(To), 346.0f, 934.0f, IgniteTx(10.5f, 800, Hex(IgLime), Align::Right));
	if (St.Clicked)
	{
		U.Rewards = 1;
		Access::OpenPage(RL, RiverLine::Page::Promotions, Now);
	}
}

void IgniteDockWheel(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const Rect R{1236.0f, 884.0f, 340.0f, 64.0f};
	const Ui::ClickState Wh = RL.UI.Clickable("dockwheel", {R.X, R.Y, 220.0f, R.H}, !Preview);
	const Ui::ClickState Cr = RL.UI.Clickable("dockcrackers", {R.X + 220.0f, R.Y, 120.0f, R.H}, !Preview);
	C.FillRoundRect(R, 20.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(Wh.Hover || Cr.Hover ? K.Panel : K.PanelTop), Hex(K.PanelBot)));
	C.StrokeRoundRect(R, 20.0f, Hex(Wh.Hover || Cr.Hover ? K.Coral : K.Rim), 1.0f);
	// The mini wheel: its rings lit by today's paid entries, glowing while a spin waits.
	const float Pulse = L.SpinReady && !Preview ? IgpPulse(Now, K) : 0.0f;
	if (L.SpinReady)
	{
		C.GlowRoundRect({1252.0f, 896.0f, 40.0f, 40.0f}, 20.0f, Hex(IgLime, 0.2f + 0.25f * Pulse), 10.0f);
	}
	const uint32_t Lights[3] = {IgLime, IgSun, IgPink};
	for (int Ring = 0; Ring < 3; ++Ring)
	{
		const float Rr = 17.0f - static_cast<float>(Ring) * 5.0f;
		C.StrokeArc(1272.0f, 916.0f, Rr, 0.0f, 2.0f * Pi, Ring < L.RingsLit ? Hex(Lights[Ring]) : Hex(0x6a5590, 0.8f), Ring < L.RingsLit ? 2.6f : 1.5f);
	}
	IgniteSpark(C, 1272.0f, 916.0f, 4.0f, Hex(L.RingsLit > 0 ? IgLime : 0x8a78b5));
	const std::string Head = L.OnIce ? std::string("ON ICE") : L.SpinReady ? std::string("SPIN READY") : L.Spun ? std::string("SPUN TODAY") : L.RingsLit > 0 ? std::string(IgniteRingName(L.RingsLit)) + " LIT" : std::string("SPARK WHEEL");
	C.Text(Head, 1302.0f, 911.0f, IgniteBi(14.0f, L.SpinReady ? Hex(IgLime) : Hex(K.Cream), Align::Left, Baseline::Alphabetic, 150.0f));
	const std::string Sub = L.SpinReady ? std::string(IgniteRingName(L.RingsLit)) + " ring lit" : IgniteNearRingShort(L);
	C.Text(Sub.empty() ? std::string("all three rings lit") : Sub, 1302.0f, 931.0f, IgniteTx(10.5f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, 150.0f));
	// The crackers waiting on the shelf.
	IgniteCracker(C, 1514.0f, 908.0f, 64.0f, L.Crackers.empty() ? 0 : L.Crackers.front().Tier, 0.0f);
	const int N = static_cast<int>(L.Crackers.size());
	if (N > 0)
	{
		C.FillCircle(1548.0f, 896.0f, 10.0f, Hex(IgSun));
		C.Text(std::to_string(N), 1548.0f, 896.5f, IgniteTx(11.0f, 900, Hex(IgDarkInk), Align::Center, Baseline::Middle));
	}
	C.Text("CRACKERS", 1514.0f, 936.0f, IgniteTx(9.0f, 800, pal::Muted, Align::Center));
	if (Wh.Clicked || Cr.Clicked)
	{
		U.Rewards = 0;
		Access::OpenPage(RL, RiverLine::Page::Promotions, Now);
	}
}

// ------------------------------------------------------------------ the Today strip (SPEC §2.4 A)

void IgniteTodayLoops(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview, double World)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	bool Open = false;
	// The Flame {24,364,300,64}: the flame by streak, the days, the multiplier, the week's dots and the Spare Matches.
	{
		const Rect R{24.0f, 364.0f, 300.0f, 64.0f};
		const Ui::ClickState St = RL.UI.Clickable("tdflame", R, !Preview);
		Open = Open || St.Clicked;
		// Core's nudge (F2: 6 PM to the 6 AM close, a book with no flame day yet, not on ice): a player with no flame yet isn't
		// told a flame needs feeding.
		const bool Needs = L.Nudge;
		C.FillRoundRect(R, 18.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(L.OnIce ? 0x16222e : 0x3a1a14), Hex(K.PanelBot)));
		C.StrokeRoundRect(R, 18.0f, L.OnIce ? Hex(0xcfe8f6, 0.8f) : Needs ? Hex(IgTang) : Hex(St.Hover ? K.Coral : K.RimHi), L.OnIce || Needs ? 1.5f : 1.0f);
		IgniteFlameLook(C, 58.0f, 418.0f, 34.0f, L.Streak, L.OnIce);
		const std::string Days = std::to_string(L.Streak) + (L.Streak == 1 ? " DAY" : " DAYS");
		const float Dw = C.Text(Days, 88.0f, 394.0f, IgniteBi(22.0f, Hex(K.Cream)));
		// The multiplier with the vector cross (the glyph rule, G2_AUDIT L9), never a letter x.
		const std::string Mult = Fixed(L.Mult, 2);
		const float Mw = IgpTimesWidth(C, 12.0f, Mult) + 16.0f;
		C.FillRoundRect({96.0f + Dw, 378.0f, Mw, 20.0f}, 10.0f, L.OnIce ? Hex(0xcfe8f6) : Hex(IgSun));
		IgpTimesText(C, 96.0f + Dw + Mw / 2.0f, 392.0f, 12.0f, Mult, Hex(IgDarkInk), Align::Center);
		if (L.OnIce)
		{
			C.Text(IgniteOnIce, 88.0f, 416.0f, IgniteTx(11.5f, 700, Hex(0xcfe8f6), Align::Left, Baseline::Alphabetic, 226.0f));
		}
		else if (Needs)
		{
			C.Text(IgniteFlameNeeds, 88.0f, 416.0f, IgniteTx(11.5f, 700, Hex(IgTang), Align::Left, Baseline::Alphabetic, 226.0f));
		}
		else
		{
			// The week's seven dots (lit, rest day, missed, today, iced, a match burnt), then the Spare Matches held.
			for (int D = 0; D < 7; ++D)
			{
				const int W = L.Week[static_cast<size_t>(D)];
				const float X = 96.0f + static_cast<float>(D) * 22.0f;
				if (W == 1)
				{
					IgniteFlame(C, X, 420.0f, 8.0f, Hex(IgCoral), Hex(IgTang), Hex(IgSun));
				}
				else
				{
					const uint32_t Col = W == 2 ? 0x8a78b5 : W == 3 ? IgRed : W == 4 ? IgTang : W == 5 ? 0xcfe8f6 : W == 6 ? 0xe0c08a : 0x4a3a5c;
					C.StrokeArc(X, 413.0f, 5.5f, 0.0f, 2.0f * Pi, Hex(Col, W == 0 ? 0.6f : 1.0f), 1.5f);
				}
			}
			for (int M = 0; M < L.Matches && M < 2; ++M)
			{
				IgniteMatch(C, 268.0f + static_cast<float>(M) * 12.0f, 422.0f, 18.0f, false, false);
			}
		}
		C.Text("FLAME", 312.0f, 386.0f, IgniteTx(9.0f, 900, pal::Muted, Align::Right));
	}
	// Three Dare chips {340,364,880,64}.
	for (int I = 0; I < 3; ++I)
	{
		const IgniteDare& D = L.Dares[static_cast<size_t>(I)];
		const float W = (880.0f - 2.0f * 12.0f) / 3.0f;
		const Rect R{340.0f + static_cast<float>(I) * (W + 12.0f), 364.0f, W, 64.0f};
		const Ui::ClickState St = RL.UI.Clickable("tddare" + std::to_string(I), R, !Preview);
		Open = Open || St.Clicked;
		const uint32_t Tc = IgpTierCols[std::min(2, std::max(0, D.Tier))];
		const bool Ready = D.Done && !D.Claimed;
		C.FillRoundRect(R, 18.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(St.Hover ? K.Panel : K.PanelTop), Hex(K.PanelBot)));
		C.StrokeRoundRect(R, 18.0f, Ready || D.Claimed ? Hex(IgLime) : Hex(K.RimHi), Ready ? 2.0f : 1.0f);
		if (L.OnIce)
		{
			C.FillRoundRect(R, 18.0f, Hex(IgNight, 0.45f));
		}
		const float Cx = R.X + 30.0f;
		if (D.Done || D.Claimed)
		{
			C.FillCircle(Cx, 396.0f, 16.0f, Hex(IgLime));
			C.StrokePolyline({{Cx - 7.0f, 396.0f}, {Cx - 2.0f, 401.5f}, {Cx + 8.0f, 390.0f}}, false, Hex(IgDarkInk), 3.0f, true);
		}
		else
		{
			const float Fr = D.Needs > 0 ? static_cast<float>(Clamp01(static_cast<double>(D.Prog) / static_cast<double>(D.Needs))) : 0.0f;
			C.StrokeArc(Cx, 396.0f, 15.0f, 0.0f, 2.0f * Pi, Hex(K.Well), 3.0f);
			if (Fr > 0.0f)
			{
				C.StrokeArc(Cx, 396.0f, 15.0f, -Pi / 2.0f, -Pi / 2.0f + 2.0f * Pi * Fr, Hex(Tc), 3.0f);
			}
			C.Text(std::to_string(D.Prog) + "/" + std::to_string(D.Needs), Cx, 396.5f, IgniteTx(10.0f, 900, Hex(K.Cream), Align::Center, Baseline::Middle));
		}
		// Its target met but today's paid entries short of its floor: it says how many more (it can't be claimed yet).
		const int Short = IgniteDareShort(D, L.EntriesToday);
		if (Short > 0)
		{
			C.Text(std::string(IgpTierNames[std::min(2, std::max(0, D.Tier))]) + " DARE \xC2\xB7 " + IgpMoreEntries(Short), R.X + 58.0f, R.Y + 24.0f,
				IgniteTx(9.5f, 900, Hex(Tc), Align::Left, Baseline::Alphabetic, W - 100.0f));
		}
		else
		{
			C.Text(std::string(IgpTierNames[std::min(2, std::max(0, D.Tier))]) + " DARE", R.X + 58.0f, R.Y + 24.0f, IgniteTx(9.5f, 900, Hex(Tc)));
		}
		const float Lw = W - 146.0f;
		const float Ls = C.Measure(D.Label, 14.5f, 700) <= Lw ? 14.5f : 13.0f;
		C.Text(D.Id < 0 ? std::string("\xE2\x80\x94") : D.Label, R.X + 58.0f, R.Y + 46.0f, IgniteTx(Ls, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, Lw));
		C.Text(D.Claimed ? std::string("DONE") : "+" + Grouped(D.Sparks), R.X + W - 34.0f, R.Y + 42.0f, IgniteBi(19.0f, D.Claimed ? pal::Muted : Hex(IgLime), Align::Right));
		if (!D.Claimed)
		{
			IgniteSpark(C, R.X + W - 22.0f, R.Y + 30.0f, 6.0f, Hex(IgLime));
		}
	}
	// The near-goal line {1236,364,340,64}.
	{
		const Rect R{1236.0f, 364.0f, 340.0f, 64.0f};
		const Ui::ClickState St = RL.UI.Clickable("tdgoal", R, !Preview);
		Open = Open || St.Clicked;
		C.FillRoundRect(R, 18.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, Hex(IgSun, 0.14f), Hex(K.PanelBot)));
		C.StrokeRoundRect(R, 18.0f, Hex(IgSun, St.Hover ? 1.0f : 0.7f), 1.5f);
		const float Glow = L.SpinReady && !Preview ? IgpPulse(Now, K) : 0.0f;
		const uint32_t Lights[3] = {IgLime, IgSun, IgPink};
		for (int Ring = 0; Ring < 3; ++Ring)
		{
			const float Rr = 18.0f - static_cast<float>(Ring) * 6.0f;
			C.StrokeArc(R.X + 34.0f, 396.0f, Rr, 0.0f, 2.0f * Pi, Ring < L.RingsLit ? Hex(Lights[Ring], 0.75f + 0.25f * Glow) : Hex(0x6a5590), Ring < L.RingsLit ? 2.6f : 1.5f);
		}
		std::string Head = L.SpinReady && L.RingsLit >= 3 ? std::string(IgniteSpinReady) : IgniteNearGoal(L, World);
		if (L.RingsLit < 3 && !L.NearRing.empty() && C.Measure(L.NearRing, 16.0f, Font::BlackItalic) <= R.W - 80.0f)
		{
			Head = L.NearRing; // core's words when they fit the line
		}
		C.Text(Head, R.X + 64.0f, R.Y + 28.0f, IgniteBi(16.0f, Hex(IgSun), Align::Left, Baseline::Alphabetic, R.W - 80.0f));
		std::string Sub;
		if (L.RingsLit < 3)
		{
			Sub = Grouped(std::max<int64_t>(0, L.LevelTo - L.LevelSparks)) + " Sparks to Level " + std::to_string(L.Level + 1);
		}
		if (L.SpinReady)
		{
			Sub += std::string(Sub.empty() ? "" : " \xC2\xB7 ") + "spin waits till 6 AM";
		}
		C.Text(Sub, R.X + 64.0f, R.Y + 48.0f, IgniteTx(11.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, R.W - 80.0f));
	}
	if (Open)
	{
		U.Rewards = 0;
		Access::OpenPage(RL, RiverLine::Page::Promotions, Now);
	}
}

// ------------------------------------------------------------------ REWARDS (SPEC §2.4 H)

namespace
{
/** The Welcome bonus sub-page (G1's, F11): the bar, released and pending, the window, the Fuel Gauge, your game here. */
void IgpWelcome(RiverLine& RL, const igniteart::IgniteTones& K, double Now, bool Preview, float In)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const double World = Access::World(RL);
	const site::Account* A = rlsites_detail::SitesAccount(S, net::SiteId::Ignite);
	const IgniteWelcome W = IgniteWelcomeOf(S, World);
	{
		const Rect P{24.0f, 148.0f, 1552.0f, 420.0f};
		IgpCard(C, P, K, 26.0f);
		C.FillRoundRect(P, 26.0f, Paint::Linear({P.X, 0.0f}, {P.X + P.W, 0.0f}, Hex(K.Coral, 0.12f), Hex(IgViolet, 0.04f)));
		const float Lx = P.X + 40.0f;
		C.Text("WELCOME BONUS", Lx, P.Y + 50.0f, IgniteBi(17.0f, Hex(K.Coral)));
		C.Text("100% of your first deposit, up to $100", Lx, P.Y + 96.0f, IgniteBi(32.0f, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 920.0f));
		rlnet_detail::NetParagraph(C, IgniteBonusRule, Lx, P.Y + 130.0f, 900.0f, 16.0f, 500, IgniteA(Hex(K.Cream), 0.82f), 22.0f, 2);
		const Rect Bar{Lx, P.Y + 196.0f, 920.0f, 18.0f};
		C.FillRoundRect(Bar, 9.0f, Hex(K.Well));
		if (W.Started && !W.Closed && W.Released > 0)
		{
			const float Frac = static_cast<float>(Clamp01(static_cast<double>(W.Released) / static_cast<double>(W.Released + W.Pending)));
			C.FillRoundRect({Bar.X, Bar.Y, std::max(18.0f, Bar.W * Frac * In), Bar.H}, 9.0f, Paint::Linear({Bar.X, 0.0f}, {Bar.X + Bar.W, 0.0f}, Hex(IgTang), Hex(IgLime)));
		}
		if (W.Started)
		{
			C.Text(Money(W.Released) + " released", Lx, P.Y + 262.0f, IgniteBi(24.0f, Hex(IgLime)));
			const std::string State = W.Pending > 0 ? Money(W.Pending) + " still pending"
				: W.Closed ? rlsites_detail::SitesFill(IgniteWindowShut, {{"Date", net::DateLabel(net::DayOf(W.Ends))}})
						   : std::string("All of it's yours now.");
			C.Text(State, Lx + 300.0f, P.Y + 260.0f, IgniteTx(16.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 640.0f));
			if (W.Pending > 0 && A)
			{
				const double Left = A->BonusExpires - World;
				C.Text(Left > 0.0 ? "Expires " + std::string(net::WeekdayName(net::DayOf(A->BonusExpires), true)) + ", " + net::DateLabel(net::DayOf(A->BonusExpires)) + " \xC2\xB7 " + net::Countdown(Left) + " left"
								  : std::string("Expired"),
					Lx, P.Y + 300.0f, IgniteTx(14.0f, 700, Left > 0.0 && Left < 7.0 * net::MinutesPerDay ? Hex(IgTang) : pal::Muted));
				C.Text("$5 for every $20 of rake \xC2\xB7 " + Money(A->RakeLife) + " of rake paid here so far", Lx, P.Y + 330.0f, IgniteTx(14.0f, 600, pal::Muted));
			}
		}
		else
		{
			C.Text(IgniteStartsIt, Lx, P.Y + 262.0f, IgniteTx(18.0f, 700, Hex(K.Cream)));
		}
		const float Gx = P.X + P.W - 260.0f;
		const float Gy = P.Y + 250.0f;
		C.GlowRoundRect({Gx - 150.0f, Gy - 150.0f, 300.0f, 200.0f}, 100.0f, Hex(IgLime, 0.06f), 40.0f);
		IgniteFuelGauge(C, Gx, Gy, 130.0f, W.Started ? W.Needle * static_cast<double>(In) : 0.0, K);
		C.Text("FUEL GAUGE", Gx, Gy + 36.0f, IgniteTx(11.0f, 900, pal::Muted, Align::Center));
		if (W.Started && !W.Closed && !W.Done && W.NextIn > 0)
		{
			C.Text("next $5 in " + Money(W.NextIn) + " of rake", Gx, Gy + 76.0f, IgniteBi(19.0f, Hex(IgLime), Align::Center));
		}
		else if (W.Started)
		{
			C.Text(Money(W.Released) + " of " + Money(W.Released + W.Pending), Gx, Gy + 76.0f, IgniteBi(19.0f, Hex(IgLime), Align::Center));
		}
		else
		{
			C.Text("up to $100", Gx, Gy + 76.0f, IgniteBi(19.0f, Hex(IgLime), Align::Center));
		}
	}
	{
		const Rect P{24.0f, 584.0f, 1552.0f, 248.0f};
		IgpCard(C, P, K, 26.0f);
		C.Text("YOUR GAME ON IGNITE", P.X + 40.0f, P.Y + 44.0f, IgniteBi(17.0f, Hex(K.Cream)));
		const std::pair<std::string, std::string> Facts[4] = {
			{A ? std::to_string(A->LifetimeEvents) : std::string("0"), "TOURNAMENTS HERE"},
			{A ? Money(A->MonthKey == site::MonthKeyAt(World) ? A->RakeMonth : 0) : Money(0), "RAKE THIS MONTH"},
			{A ? Money(A->RakeLife) : Money(0), "RAKE ALL TIME"},
			{A ? Money(A->LifetimeWon) : Money(0), "WON HERE"},
		};
		for (int I = 0; I < 4; ++I)
		{
			const float Fx = P.X + 40.0f + static_cast<float>(I) * 300.0f;
			C.Text(Facts[I].first, Fx, P.Y + 108.0f, IgniteBi(34.0f, I == 3 ? Hex(IgSun) : Hex(K.Cream)));
			C.Text(Facts[I].second, Fx, P.Y + 132.0f, IgniteTx(10.5f, 900, pal::Muted));
		}
		C.Text(IgniteStaysHere, P.X + 40.0f, P.Y + 190.0f, IgniteTx(15.0f, 600, pal::Muted));
		if (IgpButton(RL, "rwwallet", {P.X + P.W - 300.0f, P.Y + 150.0f, 260.0f, 60.0f}, "Open the Wallet", ButtonKind::Primary, 17.0f, !Preview))
		{
			Access::OpenPage(RL, RiverLine::Page::Cashier, Now);
		}
	}
}

/** The three Dare cards (REWARDS › Today, {24,140,640,300}). */
void IgpDares(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const double World = Access::World(RL);
	const Rect P{24.0f, 140.0f, 640.0f, 300.0f};
	IgpCard(C, P, K);
	C.Text("DARES", P.X + 24.0f, P.Y + 38.0f, IgniteBi(22.0f, Hex(K.Cream)));
	C.Text("reset in " + net::Countdown(std::max(0.0, L.DaresReset - World)), P.X + P.W - 24.0f, P.Y + 36.0f, IgniteTx(13.0f, 700, pal::Muted, Align::Right));
	for (int I = 0; I < 3; ++I)
	{
		const IgniteDare& D = L.Dares[static_cast<size_t>(I)];
		const int T = std::min(2, std::max(0, D.Tier));
		const Rect R{P.X + 16.0f, P.Y + 56.0f + static_cast<float>(I) * 68.0f, P.W - 32.0f, 60.0f};
		C.FillRoundRect(R, 16.0f, Hex(D.Done || D.Claimed ? 0x1f2a12 : K.Well));
		C.FillRoundRect({R.X, R.Y, 6.0f, R.H}, 3.0f, Hex(IgpTierCols[T]));
		const float Cx = R.X + 34.0f;
		const float Cy = R.Y + R.H / 2.0f;
		if (D.Done || D.Claimed)
		{
			C.FillCircle(Cx, Cy, 15.0f, Hex(IgLime));
			C.StrokePolyline({{Cx - 7.0f, Cy}, {Cx - 2.0f, Cy + 5.5f}, {Cx + 8.0f, Cy - 6.0f}}, false, Hex(IgDarkInk), 3.0f, true);
		}
		else
		{
			C.StrokeArc(Cx, Cy, 14.0f, 0.0f, 2.0f * Pi, Hex(IgpTierCols[T], 0.4f), 3.0f);
		}
		C.Text(std::string(IgpTierNames[T]) + " \xC2\xB7 " + std::to_string(D.Prog) + " OF " + std::to_string(D.Needs), R.X + 62.0f, R.Y + 22.0f, IgniteTx(10.0f, 900, Hex(IgpTierCols[T])));
		C.Text(D.Id < 0 ? std::string("\xE2\x80\x94") : D.Label, R.X + 62.0f, R.Y + 44.0f, IgniteTx(15.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 300.0f));
		// Progress under the line.
		const float Fr = D.Needs > 0 ? static_cast<float>(Clamp01(static_cast<double>(D.Prog) / static_cast<double>(D.Needs))) : 0.0f;
		C.FillRoundRect({R.X + 62.0f, R.Y + 50.0f, 300.0f, 4.0f}, 2.0f, Hex(IgNight, 0.6f));
		if (Fr > 0.0f)
		{
			C.FillRoundRect({R.X + 62.0f, R.Y + 50.0f, 300.0f * Fr, 4.0f}, 2.0f, Hex(IgpTierCols[T]));
		}
		C.Text("+" + Grouped(D.Sparks), R.X + 440.0f, R.Y + 38.0f, IgniteBi(20.0f, D.Claimed ? pal::Muted : Hex(IgLime), Align::Right));
		const Rect B{R.X + R.W - 148.0f, R.Y + 10.0f, 136.0f, 40.0f};
		if (D.Claimed)
		{
			C.Text(D.Held ? std::string("ON THE GUEST LIST") : std::string("CLAIMED"), B.X + B.W / 2.0f, B.Y + 25.0f, IgniteTx(11.0f, 900, D.Held ? Hex(IgSun) : pal::Muted, Align::Center));
		}
		else if (D.Done)
		{
			if (IgpButton(RL, "dare" + std::to_string(I), B, "Claim", ButtonKind::Primary, 15.0f, !Preview))
			{
				const std::string Why = S.ClaimDare(I);
				U.LoopsAt = -1.0;
				if (Why.empty())
				{
					S.UiSound(SoundId::IgSpark, 0.4f);
				}
				else
				{
					IgniteNoteRefusal(U, "dare" + std::to_string(I), Why, Now);
				}
			}
			IgpNote(C, U, "dare" + std::to_string(I), B.X + B.W, B.Y + B.H + 14.0f, 300.0f, Now, Align::Right);
		}
		else if (const int Short = IgniteDareShort(D, L.EntriesToday))
		{
			// Its target met, today's paid entries short of its floor (core's Done waits on both): how many more, where CLAIM goes.
			C.Text(IgpMoreEntries(Short), B.X + B.W / 2.0f, B.Y + 25.0f, IgniteTx(11.0f, 900, Hex(IgSun), Align::Center));
		}
	}
	// The clear-all reward and Dare Week.
	const float Fy = P.Y + P.H - 24.0f;
	IgniteCracker(C, P.X + 42.0f, Fy - 5.0f, 44.0f, L.ClearCracker, 0.0f);
	C.Text(std::string("Clear all 3: a ") + IgpCrackerNames[std::min(2, std::max(0, L.ClearCracker))] + " Cracker" + (L.ClearPaid ? " \xC2\xB7 dropped" : ""), P.X + 72.0f, Fy, IgniteTx(13.0f, 700, Hex(K.Cream)));
	C.Text("Dare Week " + std::to_string(std::min(5, L.WeekClears)) + " of 5: a Gold Cracker", P.X + P.W - 24.0f, Fy, IgniteTx(13.0f, 700, Hex(IgSun), Align::Right));
	if (L.OnIce)
	{
		C.FillRoundRect(P, 24.0f, Hex(IgNight, 0.55f));
		C.Text(IgniteDaresGrey, P.X + P.W / 2.0f, P.Y + P.H / 2.0f, IgniteTx(15.0f, 800, Hex(IgRed), Align::Center, Baseline::Middle, P.W - 40.0f));
	}
}

/** The Flame panel (REWARDS › Today, {680,140,440,300}). */
void IgpFlame(RiverLine& RL, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const Rect P{680.0f, 140.0f, 440.0f, 300.0f};
	IgpCard(C, P, K);
	C.FillRoundRect(P, 24.0f, Paint::Linear({0.0f, P.Y}, {0.0f, P.Y + P.H}, Hex(L.OnIce ? 0xcfe8f6 : IgTang, 0.1f), Hex(K.PanelBot, 0.0f)));
	C.Text("THE FLAME", P.X + 24.0f, P.Y + 38.0f, IgniteBi(22.0f, Hex(K.Cream)));
	C.Text("Best " + std::to_string(L.Best) + (L.Best == 1 ? " day" : " days"), P.X + P.W - 24.0f, P.Y + 36.0f, IgniteTx(13.0f, 700, pal::Muted, Align::Right));
	const float Breath = Preview ? 1.0f : 1.0f + 0.04f * IgpPulse(Now, K);
	IgniteFlameLook(C, P.X + 80.0f, P.Y + 180.0f, 70.0f * Breath, L.Streak, L.OnIce);
	const float Nw = C.Text(std::to_string(L.Streak), P.X + 150.0f, P.Y + 150.0f, IgniteBi(56.0f, Hex(K.Cream)));
	C.Text(L.Streak == 1 ? "DAY" : "DAYS", P.X + 158.0f + Nw, P.Y + 150.0f, IgniteBi(20.0f, Hex(K.Cream)));
	IgpTimesText(C, P.X + 152.0f, P.Y + 184.0f, 22.0f, Fixed(L.Mult, 2), L.OnIce ? Hex(0xcfe8f6) : Hex(IgSun));
	C.Text(L.OnIce ? std::string(IgniteOnIce) : std::string("on base Sparks"), P.X + 240.0f, P.Y + 180.0f, IgniteTx(12.5f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, 180.0f));
	// The week: Monday to Sunday.
	for (int D = 0; D < 7; ++D)
	{
		const int W = L.Week[static_cast<size_t>(D)];
		const float X = P.X + 46.0f + static_cast<float>(D) * 58.0f;
		const float Y = P.Y + 232.0f;
		C.Text(IgpDays[D], X, P.Y + 266.0f, IgniteTx(9.5f, 800, pal::Muted, Align::Center));
		if (W == 1)
		{
			IgniteFlame(C, X, Y + 12.0f, 18.0f, Hex(IgCoral), Hex(IgTang), Hex(IgSun));
		}
		else if (W == 6)
		{
			IgniteMatch(C, X, Y + 12.0f, 22.0f, false, true);
		}
		else
		{
			const uint32_t Col = W == 2 ? 0x8a78b5 : W == 3 ? IgRed : W == 4 ? IgTang : W == 5 ? 0xcfe8f6 : 0x4a3a5c;
			const float Pulse = W == 4 && !Preview ? IgpPulse(Now, K) : 0.0f;
			C.StrokeArc(X, Y, 11.0f + 2.0f * Pulse, 0.0f, 2.0f * Pi, Hex(Col), W == 0 ? 1.5f : 2.5f);
			if (W == 2)
			{
				C.Text("REST", X, Y + 0.5f, IgniteTx(7.5f, 900, Hex(Col), Align::Center, Baseline::Middle));
			}
		}
	}
	// Spare Matches held (up to 2), under the best.
	C.Text("SPARE MATCHES", P.X + P.W - 24.0f, P.Y + 112.0f, IgniteTx(9.0f, 900, pal::Muted, Align::Right));
	for (int M = 0; M < 2; ++M)
	{
		IgniteMatch(C, P.X + P.W - 46.0f + static_cast<float>(M) * 16.0f, P.Y + 94.0f, 26.0f, false, M >= L.Matches);
	}
}

/** The Spark Wheel panel (REWARDS › Today, {1136,140,440,500}). */
void IgpWheel(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const Rect P{1136.0f, 140.0f, 440.0f, 500.0f};
	IgpCard(C, P, K);
	C.Text("SPARK WHEEL", P.X + 24.0f, P.Y + 38.0f, IgniteBi(22.0f, Hex(K.Cream)));
	{
		const Rect O{P.X + P.W - 104.0f, P.Y + 16.0f, 84.0f, 30.0f};
		const Ui::ClickState St = RL.UI.Clickable("wheelodds", O, !Preview);
		IgnitePill(C, O, Hex(St.Hover ? K.Panel : K.Well), Hex(K.RimHi), 1.2f);
		C.Text("ODDS", O.X + O.W / 2.0f, O.Y + 20.0f, IgniteTx(12.0f, 900, Hex(K.Cream), Align::Center));
		if (St.Clicked)
		{
			U.Drawer = 1;
			U.DrawerAt = Now;
		}
	}
	// The idle wheel (cached by what's lit and the hour's tones), glowing while a spin waits.
	const float Glow = L.SpinReady ? (Preview ? 1.0f : 0.5f + 0.5f * IgpPulse(Now, K)) : 0.0f;
	const int Lit = L.RingsLit;
	const std::vector<IgniteWedge>& Shown = L.Rings[static_cast<size_t>(std::max(0, std::min(2, Lit - 1)))];
	const size_t Wedges = std::max<size_t>(5, Shown.size());
	// The idle wheel with the wedges of the ring a spin would use, their prizes riding upright (cached by the ring, the band
	// and the hour's tones).
	std::string Labels;
	for (const IgniteWedge& W : Shown)
	{
		Labels += (W.TicketId.empty() ? Grouped(W.Sparks) : std::string("TICKET")) + "|";
	}
	C.Cached((static_cast<uint64_t>(Fnv1a("ignite:rw-wheel|" + Labels)) << 24) ^ (static_cast<uint64_t>(Lit) << 16) ^ (static_cast<uint64_t>(Wedges) << 8) ^ (static_cast<uint64_t>(K.Mood) << 1) ^
			(K.Night ? 1u : 0u),
		0.0f, 0.0f, [&K, Lit, Wedges, &P, &Shown](Canvas& At) {
			const float Cx = P.X + P.W / 2.0f;
			const float Cy = P.Y + 196.0f;
			IgniteWheelFace(At, Cx, Cy, 128.0f, static_cast<int>(Wedges), Lit, -Pi / 2.0f, K, 0.0f);
			const float Step = 2.0f * Pi / static_cast<float>(Wedges);
			for (size_t I = 0; I < Shown.size(); ++I)
			{
				const float A = -Pi / 2.0f + (static_cast<float>(I) + 0.5f) * Step;
				const IgniteWedge& W = Shown[I];
				// Between SHINE (0.7 R, its stroke 2 either side) and GLOW's rim (R + 6): off every ring, lit or not.
				IgpWheelLabel(At, W.TicketId.empty() ? Grouped(W.Sparks) : std::string("TICKET"), Cx, Cy, IgniteWheelBandMid(128.0f), IgniteWheelBandRoom(128.0f), A,
					W.TicketId.empty() ? 15.0f : 10.5f, W.TicketId.empty() ? Hex(IgCream, 0.85f) : Hex(IgSun));
			}
		});
	if (Glow > 0.0f)
	{
		const uint32_t Lights[3] = {IgLime, IgSun, IgPink};
		C.StrokeArc(P.X + P.W / 2.0f, P.Y + 196.0f, 140.0f, 0.0f, 2.0f * Pi, Hex(Lights[std::max(0, Lit - 1)], 0.25f * Glow), 12.0f);
	}
	// The three rings, lit or dim, and what lights each.
	const char* const Needs[3] = {"1st paid entry", "2nd paid entry", "4th paid entry"};
	const uint32_t Lights[3] = {IgLime, IgSun, IgPink};
	for (int Ring = 0; Ring < 3; ++Ring)
	{
		const float X = P.X + 24.0f + static_cast<float>(Ring) * 136.0f;
		const bool On = Ring < Lit;
		C.FillRoundRect({X, P.Y + 344.0f, 128.0f, 48.0f}, 14.0f, Hex(On ? IgNight : K.Well));
		C.StrokeRoundRect({X, P.Y + 344.0f, 128.0f, 48.0f}, 14.0f, Hex(Lights[Ring], On ? 1.0f : 0.3f), On ? 1.8f : 1.0f);
		C.Text(IgniteRingName(Ring + 1), X + 64.0f, P.Y + 366.0f, IgniteBi(15.0f, On ? Hex(Lights[Ring]) : pal::Muted, Align::Center));
		C.Text(Needs[Ring], X + 64.0f, P.Y + 384.0f, IgniteTx(9.5f, 700, pal::Muted, Align::Center));
	}
	// SPIN, on the highest ring lit.
	const Rect B{P.X + 24.0f, P.Y + 408.0f, P.W - 48.0f, 60.0f};
	const std::string Label = L.Spun ? std::string("Spun today") : L.RingsLit > 0 ? std::string("Spin \xC2\xB7 ") + IgniteRingName(L.RingsLit) : std::string("Spin");
	if (IgpButton(RL, "wheelspin", B, Label, L.SpinReady ? ButtonKind::Primary : ButtonKind::Secondary, 19.0f, L.SpinReady && !Preview))
	{
		const std::string Why = S.SpinWheel();
		U.LoopsAt = -1.0;
		if (Why.empty())
		{
			IgniteStartSpin(U, IgniteLoopsOf(RL, Now), Now);
		}
		else
		{
			IgniteNoteRefusal(U, "wheelspin", Why, Now);
		}
	}
	const std::string Near = IgniteNearRing(L);
	// Core's words end a sentence ("1 more entry lights SHINE."): ", or spin now" goes on before its full stop.
	const std::string Lead = !Near.empty() && Near.back() == '.' ? Near.substr(0, Near.size() - 1) : Near;
	const std::string Line = L.Spun ? std::string(IgniteSpinDone) : L.RingsLit == 0 ? std::string(IgniteSpinNone) : L.SpinReady && !Near.empty() ? Lead + ", or spin now" : Near.empty() ? std::string(IgniteSpinReady) : Near;
	C.Text(Line, P.X + P.W / 2.0f, P.Y + 490.0f - 6.0f, IgniteTx(12.5f, 700, L.SpinReady ? Hex(IgLime) : pal::Muted, Align::Center, Baseline::Alphabetic, P.W - 40.0f));
	IgpNote(C, U, "wheelspin", P.X + P.W / 2.0f, P.Y + 404.0f, P.W - 40.0f, Now, Align::Center);
}

/** The cracker shelf (REWARDS › Today, {24,456,1096,184}). */
void IgpCrackers(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const Rect P{24.0f, 456.0f, 1096.0f, 184.0f};
	IgpCard(C, P, K);
	C.Text("CRACKERS", P.X + 24.0f, P.Y + 36.0f, IgniteBi(20.0f, Hex(K.Cream)));
	const int N = static_cast<int>(std::min<size_t>(12, L.Crackers.size()));
	C.Text(std::to_string(N) + " on the shelf \xC2\xB7 it holds 12", P.X + 150.0f, P.Y + 34.0f, IgniteTx(12.5f, 700, pal::Muted));
	if (N == 0)
	{
		C.Text(IgniteNoCrackers, P.X + P.W / 2.0f, P.Y + 104.0f, IgniteTx(15.0f, 600, pal::Muted, Align::Center));
	}
	// ODDS (W5, G2_AUDIT M2: a cracker's prize is random, so its chances are one click away): the Crackers' odds drawer, the
	// wheel's pill in look; left of "Open all", or at the shelf's right edge when there's nothing to open all of.
	{
		const Rect O{P.X + P.W - (N > 1 ? 268.0f : 104.0f), P.Y + 17.0f, 84.0f, 30.0f};
		const Ui::ClickState St = RL.UI.Clickable("crkodds", O, !Preview);
		IgnitePill(C, O, Hex(St.Hover ? K.Panel : K.Well), Hex(K.RimHi), 1.2f);
		C.Text("ODDS", O.X + O.W / 2.0f, O.Y + 20.0f, IgniteTx(12.0f, 900, Hex(K.Cream), Align::Center));
		if (St.Clicked)
		{
			U.Drawer = 4;
			U.DrawerAt = Now;
		}
	}
	if (N > 1 && IgpButton(RL, "crkall", {P.X + P.W - 164.0f, P.Y + 14.0f, 144.0f, 36.0f}, "Open all", ButtonKind::Primary, 14.0f, !Preview))
	{
		U.Crack = true;
		U.CrackId = "*";
		U.CrackAt = -100.0;
		U.Cracked.clear();
	}
	for (int I = 0; I < N; ++I)
	{
		const site::play::Box& B = L.Crackers[static_cast<size_t>(I)];
		const float Cx = P.X + 64.0f + static_cast<float>(I) * 88.0f;
		const float Bob = Preview ? 0.0f : static_cast<float>(std::sin(Now * 2.0 + static_cast<double>(I)) * 1.5);
		IgniteCracker(C, Cx, P.Y + 92.0f + Bob, 78.0f, B.Tier, 0.0f);
		C.Text(IgpCrackerNames[std::min(2, std::max(0, static_cast<int>(B.Tier)))], Cx, P.Y + 128.0f, IgniteTx(10.0f, 800, pal::Muted, Align::Center));
		const Rect O{Cx - 34.0f, P.Y + 138.0f, 68.0f, 28.0f};
		const Ui::ClickState St = RL.UI.Clickable("crk" + std::to_string(I), O, !Preview);
		IgnitePill(C, O, St.Hover ? Hex(K.CoralLight) : Hex(K.Coral), Color(), 0.0f);
		C.Text("OPEN", O.X + O.W / 2.0f, O.Y + 19.0f, IgniteTx(11.5f, 900, IgpInk(), Align::Center));
		if (St.Clicked)
		{
			U.Crack = true;
			U.CrackId = B.Id;
			U.CrackAt = -100.0;
			U.Cracked.clear();
		}
	}
}

/** The theme nights' week (REWARDS › Today; F10). */
void IgpThemes(RiverLine& RL, const IgniteLoops& L, const igniteart::IgniteTones& K, const Rect& P)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	IgpCard(C, P, K);
	C.Text("THEME NIGHTS", P.X + 20.0f, P.Y + 30.0f, IgniteBi(16.0f, Hex(K.Cream)));
	const char* const Names[7] = {"FLIP\nMANIA", "BOUNTY\nNIGHT", "FUSE\nNIGHT", "TURBO\nTHURSDAY", "FIREWORKS\nFRIDAY", "AFTER\nDARK", "SUNDAY\nBLAZE"};
	const float W = (P.W - 40.0f) / 7.0f;
	for (int D = 0; D < 7; ++D)
	{
		const float X = P.X + 20.0f + static_cast<float>(D) * W;
		const bool Tonight = D == L.ThemeNight;
		const Rect R{X + 2.0f, P.Y + 44.0f, W - 4.0f, 76.0f};
		C.FillRoundRect(R, 10.0f, Hex(Tonight ? IgNight : K.Well));
		if (Tonight)
		{
			C.StrokeRoundRect(R, 10.0f, Hex(K.Coral), 1.8f);
		}
		C.Text(IgpDays[D], X + W / 2.0f, P.Y + 62.0f, IgniteTx(9.5f, 900, Tonight ? Hex(K.Coral) : pal::Muted, Align::Center));
		const std::string Nm = Names[D];
		const size_t Br = Nm.find('\n');
		C.Text(Nm.substr(0, Br), X + W / 2.0f, P.Y + 86.0f, IgniteTx(8.0f, 900, Hex(K.Cream), Align::Center, Baseline::Alphabetic, W - 6.0f));
		C.Text(Nm.substr(Br + 1), X + W / 2.0f, P.Y + 98.0f, IgniteTx(8.0f, 900, Hex(K.Cream), Align::Center, Baseline::Alphabetic, W - 6.0f));
	}
	C.Text(L.ThemeName.empty() ? std::string() : "Tonight: " + L.ThemeName + (L.ThemeTwist.empty() ? std::string() : " \xC2\xB7 " + L.ThemeTwist), P.X + 20.0f, P.Y + P.H - 20.0f,
		IgniteTx(12.0f, 700, Hex(IgSun), Align::Left, Baseline::Alphabetic, P.W - 40.0f));
}

/** A Wristband tier's node: lit when reached, a check when claimed, dim when locked; CLAIM on a reached one. */
void IgpTierNode(RiverLine& RL, IgniteUi& U, const IgniteTier& T, const igniteart::IgniteTones& K, float Cx, float Cy, float Sz, double Now, bool Preview, bool Labels)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const Rect R{Cx - Sz / 2.0f, Cy - Sz / 2.0f, Sz, Sz};
	// A tier with nothing on it (Kind 0) is done once it's reached: core never takes a claim for it (it says there's
	// nothing to claim), so it never reads Claimed and must never wear CLAIM.
	const bool Claimable = IgniteTierClaimable(T);
	const bool Done = T.Claimed || (T.Reached && T.Kind == 0);
	if (T.Tier == 30)
	{
		// The top: a Gold Cracker on a pedestal.
		C.FillRoundRect({Cx - Sz * 0.36f, Cy + Sz * 0.16f, Sz * 0.72f, Sz * 0.26f}, 6.0f, Hex(T.Reached ? IgSun : 0x4a3a5c));
		IgniteCracker(C, Cx, Cy - Sz * 0.06f, Sz * 1.05f, 2, 0.0f);
	}
	else
	{
		const Paint Fill = T.Reached ? Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, Hex(K.CoralLight), Hex(IgViolet)) : Paint(Hex(K.Well));
		C.FillRoundRect(R, Sz * 0.24f, Fill);
		C.StrokeRoundRect(R, Sz * 0.24f, T.Reached ? Hex(IgCream, 0.7f) : Hex(K.Rim), 1.2f);
		C.Text(std::to_string(T.Tier), Cx, Cy + Sz * 0.12f, IgniteBi(Sz * 0.36f, T.Reached ? Hex(IgCream) : pal::Muted, Align::Center));
		C.Text("T", Cx, Cy - Sz * 0.22f, IgniteTx(Sz * 0.12f, 900, T.Reached ? Hex(IgCream, 0.8f) : pal::Muted, Align::Center));
	}
	if (Done)
	{
		// A ticket tier whose ticket waits on the Guest List (core's TierView::Held, G2 fix M4) wears the sun, never the lime.
		C.FillCircle(R.X + R.W - 4.0f, R.Y + 4.0f, 11.0f, Hex(T.Held ? IgSun : IgLime));
		C.StrokePolyline({{R.X + R.W - 9.0f, R.Y + 4.0f}, {R.X + R.W - 5.0f, R.Y + 8.0f}, {R.X + R.W + 1.0f, R.Y - 1.0f}}, false, Hex(IgDarkInk), 2.2f, true);
	}
	if (Labels && !T.Reward.empty())
	{
		const std::vector<std::string> Lines = rlnet_detail::NetWrap(C, T.Reward, 136.0f, 11.0f, 700);
		const float Below = Claimable ? 34.0f : 18.0f; // clear of the CLAIM pill
		// Up to three lines: T30's reward ("The Spark $5.50 ticket, a Gold Cracker and the month's title") takes three.
		for (size_t I = 0; I < Lines.size() && I < 3; ++I)
		{
			C.Text(Lines[I], Cx, R.Y + R.H + Below + static_cast<float>(I) * 14.0f, IgniteTx(11.0f, 700, T.Reached ? Hex(K.Cream) : pal::Muted, Align::Center));
		}
		// Claimed but waiting (M4, W5): it says so under its reward, as the Dare card and the spin do.
		if (T.Held)
		{
			C.Text("ON THE GUEST LIST", Cx, R.Y + R.H + Below + static_cast<float>(std::min<size_t>(Lines.size(), 3)) * 14.0f, IgniteTx(10.0f, 900, Hex(IgSun), Align::Center));
		}
	}
	if (Claimable)
	{
		const Rect B{Cx - 42.0f, R.Y + R.H - 14.0f, 84.0f, 28.0f};
		const std::string Id = "tier" + std::to_string(T.Tier);
		const Ui::ClickState St = RL.UI.Clickable(Id, B, !Preview);
		IgnitePill(C, B, St.Hover ? Hex(IgCream) : Hex(IgLime), Hex(IgNight), 1.5f);
		C.Text("CLAIM", Cx, B.Y + 19.0f, IgniteTx(11.5f, 900, Hex(IgDarkInk), Align::Center));
		if (St.Clicked)
		{
			const std::string Why = S.ClaimTier(T.Tier);
			U.LoopsAt = -1.0;
			if (Why.empty())
			{
				S.UiSound(SoundId::IgPop, 0.45f);
			}
			else
			{
				IgniteNoteRefusal(U, Id, Why, Now);
			}
		}
		IgpNote(C, U, Id, Cx, B.Y + B.H + 14.0f, 200.0f, Now, Align::Center);
	}
}

/** The month's next eight tiers on the rope (REWARDS › Today's preview). */
void IgpBandPreview(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, const Rect& P, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	IgpCard(C, P, K);
	C.Text("WRISTBAND", P.X + 24.0f, P.Y + 34.0f, IgniteBi(20.0f, Hex(K.Cream)));
	C.Text(rlnet_detail::NetUpper(L.Theme) + " \xC2\xB7 T" + std::to_string(L.Tier) + " \xC2\xB7 " + std::to_string(L.DaysLeft) + (L.DaysLeft == 1 ? " day left" : " days left"), P.X + 160.0f, P.Y + 32.0f,
		IgniteTx(12.5f, 700, pal::Muted));
	// From the first tier waiting on a claim (a reward-less tier never waits), else the next one up; eight at a time.
	int First = std::min(L.Tier + 1, 30);
	for (const IgniteTier& T : L.Tiers)
	{
		if (IgniteTierClaimable(T))
		{
			First = T.Tier;
			break;
		}
	}
	First = std::max(1, std::min(First, 23));
	const float Pitch = (P.W - 80.0f) / 7.0f;
	const float Y = P.Y + 96.0f;
	int64_t Lo = First >= 2 ? L.Tiers[static_cast<size_t>(First - 2)].Needs : 0;
	const int64_t Hi = L.Tiers[static_cast<size_t>(First + 6)].Needs;
	const double Frac = Hi > Lo ? Clamp01(static_cast<double>(L.BandSparks - Lo) / static_cast<double>(Hi - Lo)) : 0.0;
	IgniteRope(C, P.X + 40.0f, P.X + P.W - 40.0f, Y, 5.0f, Frac, static_cast<uint32_t>(First));
	for (int I = 0; I < 8; ++I)
	{
		const IgniteTier& T = L.Tiers[static_cast<size_t>(First - 1 + I)];
		IgpTierNode(RL, U, T, K, P.X + 40.0f + static_cast<float>(I) * Pitch, Y, 44.0f, Now, Preview, false);
	}
	if (Lo == 0)
	{
		(void)Lo;
	}
}

/** REWARDS › Today. */
void IgpToday(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	IgpDares(RL, U, L, K, Now, Preview);
	IgpFlame(RL, L, K, Now, Preview);
	IgpWheel(RL, U, L, K, Now, Preview);
	IgpCrackers(RL, U, L, K, Now, Preview);
	IgpBandPreview(RL, U, L, K, {24.0f, 656.0f, 1096.0f, 176.0f}, Now, Preview);
	IgpThemes(RL, L, K, {1136.0f, 656.0f, 440.0f, 176.0f});
}

/** REWARDS › Wristband: 30 nodes over three rows of ten (SPEC §2.4 H). */
void IgpBand(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const Rect P{24.0f, 140.0f, 1552.0f, 692.0f};
	IgpCard(C, P, K);
	C.FillRoundRect(P, 24.0f, Paint::Linear({P.X, 0.0f}, {P.X + P.W, 0.0f}, Hex(IgViolet, 0.1f), Hex(K.Coral, 0.04f)));
	C.Text("WRISTBAND \xC2\xB7 " + rlnet_detail::NetUpper(L.Theme), P.X + 32.0f, P.Y + 48.0f, IgniteBi(28.0f, Hex(K.Cream)));
	C.Text("T" + std::to_string(L.Tier) + " \xC2\xB7 " + Grouped(L.BandSparks) + " base Sparks this month \xC2\xB7 " + std::to_string(L.DaysLeft) + (L.DaysLeft == 1 ? " day left" : " days left"), P.X + P.W - 32.0f, P.Y + 46.0f,
		IgniteTx(14.0f, 700, pal::Muted, Align::Right));
	C.Text("Filled by the base Sparks you earn at the tables this month.", P.X + 32.0f, P.Y + 74.0f, IgniteTx(13.0f, 600, pal::Muted));
	for (int Row = 0; Row < 3; ++Row)
	{
		const float Y = P.Y + 160.0f + static_cast<float>(Row) * 200.0f;
		int64_t Lo = Row == 0 ? 0 : L.Tiers[static_cast<size_t>(Row * 10 - 1)].Needs;
		const int64_t Hi = L.Tiers[static_cast<size_t>(Row * 10 + 9)].Needs;
		const double Frac = Hi > Lo ? Clamp01(static_cast<double>(L.BandSparks - Lo) / static_cast<double>(Hi - Lo)) : 0.0;
		IgniteRope(C, P.X + 76.0f, P.X + P.W - 76.0f, Y, 6.0f, Frac, static_cast<uint32_t>(Row + 3));
		for (int Col = 0; Col < 10; ++Col)
		{
			const IgniteTier& T = L.Tiers[static_cast<size_t>(Row * 10 + Col)];
			IgpTierNode(RL, U, T, K, P.X + 76.0f + static_cast<float>(Col) * ((P.W - 152.0f) / 9.0f), Y, 76.0f, Now, Preview, true);
		}
	}
}

/** REWARDS › Prize Counter: tabs Tickets, Looks, Extras; a 4 x 3 grid; BUY opens the confirm card. */
void IgpPrizes(RiverLine& RL, IgniteUi& U, const IgniteLoops& L, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	const char* const Tabs[3] = {"Tickets", "Looks", "Extras"};
	float Tx = 24.0f;
	for (int I = 0; I < 3; ++I)
	{
		const float W = C.Measure(Tabs[I], 14.0f, 800) + 34.0f;
		const Rect R{Tx, 140.0f, W, 34.0f};
		const Ui::ClickState St = RL.UI.Clickable("prizetab" + std::to_string(I), R, !Preview);
		if (St.Clicked)
		{
			U.PrizeTab = I;
		}
		const bool On = U.PrizeTab == I;
		IgnitePill(C, R, On ? Hex(K.Sky) : Hex(St.Hover ? K.Panel : K.Well), Hex(K.Rim), On ? 0.0f : 1.2f);
		C.Text(Tabs[I], R.X + W / 2.0f, R.Y + 22.0f, IgniteTx(14.0f, 800, On ? Hex(IgDarkInk) : Hex(K.Cream), Align::Center));
		Tx += W + 10.0f;
	}
	C.Text(IgniteSparksStay, 1576.0f, 162.0f, IgniteTx(13.0f, 600, pal::Muted, Align::Right));
	std::vector<const IgnitePrize*> Shown;
	for (const IgnitePrize& Pz : L.Prizes)
	{
		if (Pz.Tab == U.PrizeTab)
		{
			Shown.push_back(&Pz);
		}
	}
	for (size_t I = 0; I < Shown.size() && I < 12; ++I)
	{
		const IgnitePrize& Pz = *Shown[I];
		const float X = 24.0f + static_cast<float>(I % 4) * 394.0f;
		const float Y = 188.0f + static_cast<float>(I / 4) * 214.0f;
		const Rect R{X, Y, 370.0f, 200.0f};
		IgpCard(C, R, K, 20.0f);
		if (!Pz.TicketId.empty())
		{
			IgniteTicket(C, X + 24.0f, Y + 24.0f, 120.0f, 64.0f, Money(Pz.FaceCents), "TICKET", Hex(IgSun), Hex(IgDarkInk));
		}
		else if (Pz.LookSlot >= 0)
		{
			C.FillRoundRect({X + 24.0f, Y + 20.0f, 72.0f, 72.0f}, 14.0f, Paint::Linear({X + 24.0f, Y + 20.0f}, {X + 96.0f, Y + 92.0f}, Hex(K.Coral), Hex(IgViolet)));
			IgniteFlame(C, X + 60.0f, Y + 80.0f, 30.0f, Hex(IgCream), Hex(IgSun), Hex(IgCream));
		}
		else
		{
			IgniteMatch(C, X + 54.0f, Y + 90.0f, 60.0f, true, false);
		}
		const std::vector<std::string> Name = rlnet_detail::NetWrap(C, Pz.Label, 180.0f, 17.0f, 800);
		for (size_t Ln = 0; Ln < Name.size() && Ln < 2; ++Ln)
		{
			C.Text(Name[Ln], X + 166.0f, Y + 46.0f + static_cast<float>(Ln) * 21.0f, IgniteBi(17.0f, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 190.0f));
		}
		IgniteSpark(C, X + 174.0f, Y + 96.0f, 7.0f, Hex(IgLime));
		C.Text(Grouped(Pz.Sparks), X + 188.0f, Y + 103.0f, IgniteBi(20.0f, Hex(IgLime)));
		const Rect B{X + 24.0f, Y + 132.0f, 322.0f, 48.0f};
		const std::string Id = "shop" + std::to_string(Pz.Item);
		if (Pz.Owned)
		{
			// A look already owned is in the Locker; the Spare Match (Extras) reads Owned at its cap of two held (F2).
			C.Text(Pz.LookSlot >= 0 || Pz.Tab != 2 ? "In your Locker" : "2 held \xC2\xB7 the most", B.X + B.W / 2.0f, B.Y + 30.0f, IgniteTx(14.0f, 800, pal::Muted, Align::Center));
		}
		else if (L.Sparks < Pz.Sparks)
		{
			C.Text(Grouped(Pz.Sparks - L.Sparks) + " more Sparks", B.X + B.W / 2.0f, B.Y + 30.0f, IgniteTx(14.0f, 800, pal::Muted, Align::Center));
		}
		else if (IgpButton(RL, Id, B, "Buy", ButtonKind::Primary, 16.0f, !Preview))
		{
			U.BuyItem = Pz.Item;
			U.BuyAt = Now;
		}
		IgpNote(C, U, Id, B.X + B.W / 2.0f, B.Y - 6.0f, 320.0f, Now, Align::Center);
	}
}
} // namespace

void IgniteRewards(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const float In = Preview ? 1.0f : IgpEase((Now - Access::PageAt(RL)) / 0.6);
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	if (!L.Live)
	{
		// G1's REWARDS: the Welcome bonus sub-page alone.
		U.Rewards = 3;
		const char* Label = "Welcome bonus";
		const float W = C.Measure(Label, 15.0f, 800) + 40.0f;
		const Rect R{24.0f, 88.0f, W, 40.0f};
		const Ui::ClickState St = RL.UI.Clickable("rw3", R, !Preview);
		(void)St;
		IgnitePill(C, R, Hex(K.Coral), Color(), 0.0f);
		C.Text(Label, R.X + W / 2.0f, R.Y + 26.0f, IgniteTx(15.0f, 800, IgpInk(), Align::Center));
		C.Text("REWARDS", 1576.0f, 122.0f, IgniteBi(32.0f, Hex(K.Cream), Align::Right));
		IgpWelcome(RL, K, Now, Preview, In);
		return;
	}
	// The sub-pills (Today, Wristband, Prize Counter, and the Welcome bonus once it has started: its pinned copy includes the
	// window's close, "The 60 days ran out on ..."), the Sparks, the Fine print.
	const IgniteWelcome W = IgniteWelcomeOf(S, Access::World(RL));
	const bool Runs = W.Started;
	if (U.Rewards < 0 || U.Rewards > 3 || (U.Rewards == 3 && !Runs))
	{
		U.Rewards = 0;
	}
	const char* const Subs[4] = {"Today", "Wristband", "Prize Counter", "Welcome bonus"};
	float Px = 24.0f;
	for (int I = 0; I < 4; ++I)
	{
		if (I == 3 && !Runs)
		{
			continue;
		}
		const float Pw = C.Measure(Subs[I], 15.0f, 800) + 40.0f;
		const Rect R{Px, 88.0f, Pw, 40.0f};
		const Ui::ClickState St = RL.UI.Clickable("rw" + std::to_string(I), R, !Preview);
		if (St.Clicked)
		{
			U.Rewards = I;
		}
		const bool On = U.Rewards == I;
		IgnitePill(C, R, On ? Hex(K.Coral) : Hex(St.Hover ? K.Panel : K.Well), Hex(K.Rim), On ? 0.0f : 1.2f);
		C.Text(Subs[I], R.X + Pw / 2.0f, R.Y + 26.0f, IgniteTx(15.0f, 800, On ? IgpInk() : Hex(K.Cream), Align::Center));
		Px += Pw + 10.0f;
	}
	{
		const Rect F{1450.0f, 92.0f, 126.0f, 32.0f};
		const Ui::ClickState St = RL.UI.Clickable("fineprint", F, !Preview);
		IgnitePill(C, F, Hex(St.Hover ? K.Panel : K.Well), Hex(K.RimHi), 1.2f);
		C.Text("FINE PRINT", F.X + F.W / 2.0f, F.Y + 21.0f, IgniteTx(11.5f, 900, Hex(K.Cream), Align::Center));
		if (St.Clicked)
		{
			U.Drawer = 3;
			U.DrawerAt = Now;
		}
		IgniteSpark(C, 1218.0f, 108.0f, 10.0f, Hex(IgLime));
		C.Text(Grouped(L.Sparks), 1234.0f, 118.0f, IgniteBi(24.0f, Hex(IgLime)));
		C.Text("SPARKS", 1436.0f, 116.0f, IgniteTx(10.0f, 900, Hex(IgLime, 0.8f), Align::Right));
	}
	switch (U.Rewards)
	{
	case 1: IgpBand(RL, U, L, K, Now, Preview); break;
	case 2: IgpPrizes(RL, U, L, K, Now, Preview); break;
	case 3: IgpWelcome(RL, K, Now, Preview, In); break;
	default: IgpToday(RL, U, L, K, Now, Preview); break;
	}
}

// ------------------------------------------------------------------ FUSE: the Fuse Booth (SPEC §2.4 C)

namespace
{
/** A tier's buy-in (core-fuse's: $1.10, $3.30, $5.50). */
Chips IgpFuseBuyIn(int Tier)
{
	return fuse::BuyInOf(std::max(0, std::min(2, Tier)));
}

/** The FUSE table a LIGHT IT just sat (the newest table whose event is a FUSE), or -1. */
int IgpFuseTable(const Session& S)
{
	for (int I = S.TableCount() - 1; I >= 0; --I)
	{
		if (fuse::IsFuse(S.Glance(I).EventId))
		{
			return I;
		}
	}
	return -1;
}

/** LIGHT IT (or USE A TICKET) went through: the reveal starts on the table core sat. */
void IgpLit(RiverLine& RL, IgniteUi& U, const std::string& Id, const std::string& Why, double Now)
{
	Session& S = Access::S(RL);
	U.LoopsAt = -1.0;
	if (!Why.empty())
	{
		IgniteNoteRefusal(U, Id, Why, Now);
		return;
	}
	const int Table = IgpFuseTable(S);
	if (Table >= 0)
	{
		IgniteStartReveal(U, S, Table, Now);
	}
}
} // namespace

void IgniteFusePage(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	const std::string Strike = rlsites_detail::SitesClauseWarning(S, net::SiteId::Ignite, false, false);
	const bool Clause = !Strike.empty();
	// The title band on a speed plate.
	{
		C.FillPolygon({{40.0f, 92.0f}, {262.0f, 92.0f}, {244.0f, 156.0f}, {22.0f, 156.0f}}, Paint::Linear({22.0f, 92.0f}, {262.0f, 156.0f}, Hex(K.Coral), Hex(IgPink)));
		C.Text("FUSE", 136.0f, 145.0f, IgniteBi(56.0f, Hex(IgCream), Align::Center));
		C.Text(IgniteFuseLine, 284.0f, 132.0f, IgniteTx(17.0f, 600, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 840.0f));
	}
	// The three cabinets: buy-in, the top multiplier, the reel strip, LIGHT IT through the strike gate's two presses.
	const std::array<fuse::Row, 7>& Rows = fuse::Table();
	for (int I = 0; I < 3; ++I)
	{
		const Rect R{24.0f + static_cast<float>(I) * 372.0f, 176.0f, 356.0f, 470.0f};
		IgpCard(C, R, K);
		C.FillRoundRect(R, 24.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + 160.0f}, Hex(K.Coral, 0.14f), Hex(K.PanelBot, 0.0f)));
		C.Text(Money(IgpFuseBuyIn(I)), R.X + 28.0f, R.Y + 62.0f, IgniteBi(40.0f, Hex(K.Cream)));
		const float Uw = C.Text("UP TO", R.X + 30.0f, R.Y + 96.0f, IgniteTx(14.0f, 900, Hex(IgSun)));
		IgpTimes(C, R.X + 38.0f + Uw, R.Y + 97.0f, 17.0f, fuse::TopOf(I), Hex(IgSun));
		C.Text("#" + Grouped(L.Fuse.N[static_cast<size_t>(I)] + 1) + " next at this tier", R.X + R.W - 24.0f, R.Y + 96.0f, IgniteTx(11.5f, 700, pal::Muted, Align::Right));
		// The reel strip: every multiplier the tier can draw, the big ones lit.
		C.FillRoundRect({R.X + 24.0f, R.Y + 120.0f, R.W - 48.0f, 160.0f}, 16.0f, Hex(IgNight));
		for (int J = 0; J < 7; ++J)
		{
			const int M = std::max(2, fuse::MOf(Rows[static_cast<size_t>(J)], I));
			const float Cx = R.X + 24.0f + (R.W - 48.0f) * (static_cast<float>(J) + 0.5f) / 7.0f;
			C.FillRoundRect({Cx - 20.0f, R.Y + 132.0f, 40.0f, 136.0f}, 10.0f, Hex(K.Well));
			IgpTimes(C, Cx, R.Y + 206.0f, M >= 100 ? 11.0f : 14.0f, M, Hex(IgniteFuseTint(M, K)), Align::Center);
		}
		IgniteLineTimes(C, "80 / 12 / 8 from x25", R.X + R.W / 2.0f, R.Y + 304.0f, IgniteTx(11.5f, 700, pal::Muted, Align::Center));
		const Rect B{R.X + 24.0f, R.Y + 324.0f, R.W - 48.0f, 72.0f};
		const std::string Id = "fuse" + std::to_string(I);
		rlsites_detail::TwoPressOpts O;
		O.First.Kind = ButtonKind::Primary;
		O.First.Size = 22.0f;
		O.First.Enabled = L.FuseOn && !Preview;
		O.Strike = Strike;
		const rlsites_detail::TwoPress Tp = rlsites_detail::SitesTwoPress(C, RL.UI, rlsites_detail::SitesLook(net::SiteId::Ignite), Id, B, "Light it \xC2\xB7 " + Money(IgpFuseBuyIn(I)), Clause, O);
		if (Tp == rlsites_detail::TwoPress::Confirmed && !Preview)
		{
			StrikeConfirm Cf;
			Cf.Second = Clause;
			IgpLit(RL, U, Id, S.LightFuse(I, false, Cf), Now);
		}
		IgpNote(C, U, Id, B.X + B.W / 2.0f, B.Y + B.H + 18.0f, B.W, Now, Align::Center);
		const std::string& Tid = L.FuseTicketIds[static_cast<size_t>(I)];
		const int Held = Tid.empty() ? 0 : S.Life.TicketsFor(Tid);
		if (Held > 0)
		{
			const std::string Tk = "fusetkt" + std::to_string(I);
			rlsites_detail::TwoPressOpts To;
			To.First.Kind = ButtonKind::Gold;
			To.First.Size = 15.0f;
			To.First.Enabled = L.FuseOn && !Preview;
			To.Strike = Strike;
			const rlsites_detail::TwoPress Tt = rlsites_detail::SitesTwoPress(C, RL.UI, rlsites_detail::SitesLook(net::SiteId::Ignite), Tk, {B.X, B.Y + 82.0f, B.W, 44.0f},
				"Use a ticket \xC2\xB7 " + std::to_string(Held) + " held", Clause, To);
			if (Tt == rlsites_detail::TwoPress::Confirmed && !Preview)
			{
				StrikeConfirm Cf;
				Cf.Second = Clause;
				IgpLit(RL, U, Tk, S.LightFuse(I, true, Cf), Now);
			}
		}
		else
		{
			C.Text(IgniteTicketsEarn, R.X + R.W / 2.0f, R.Y + 440.0f, IgniteTx(11.5f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, R.W - 40.0f));
		}
	}
	// The last ten, coloured by multiplier.
	{
		const Rect R{24.0f, 662.0f, 1100.0f, 64.0f};
		IgpCard(C, R, K, 18.0f);
		C.Text("LAST 10", R.X + 22.0f, R.Y + 38.0f, IgniteBi(16.0f, Hex(K.Cream)));
		float X = R.X + 120.0f;
		int Shown = 0;
		for (const int16_t M : L.Fuse.Last10)
		{
			if (M <= 0)
			{
				continue;
			}
			const Rect Chip{X, R.Y + 16.0f, 78.0f, 32.0f};
			const uint32_t Tint = IgniteFuseTint(M, K);
			IgnitePill(C, Chip, Hex(IgNight), Hex(Tint), 1.8f);
			if (M >= 100 && !Preview)
			{
				C.GlowRoundRect(Chip, 16.0f, Hex(IgSun, 0.12f + 0.12f * IgpPulse(Now, K)), 8.0f);
			}
			IgpTimes(C, Chip.X + Chip.W / 2.0f, Chip.Y + 23.0f, 16.0f, M, Hex(Tint), Align::Center);
			X += 86.0f;
			++Shown;
		}
		if (Shown == 0)
		{
			C.Text("Your FUSEs land here as you light them.", R.X + 120.0f, R.Y + 38.0f, IgniteTx(14.0f, 600, pal::Muted));
		}
		const Rect O{R.X + R.W - 110.0f, R.Y + 16.0f, 92.0f, 32.0f};
		const Ui::ClickState St = RL.UI.Clickable("fuseodds", O, !Preview);
		IgnitePill(C, O, Hex(St.Hover ? K.Panel : K.Well), Hex(K.RimHi), 1.2f);
		C.Text("ODDS", O.X + O.W / 2.0f, O.Y + 21.0f, IgniteTx(12.0f, 900, Hex(K.Cream), Align::Center));
		if (St.Clicked)
		{
			U.Drawer = 2;
			U.DrawerAt = Now;
		}
	}
	// How it works.
	{
		const Rect R{24.0f, 742.0f, 1100.0f, 90.0f};
		IgpCard(C, R, K, 18.0f);
		C.Text("HOW IT WORKS", R.X + 22.0f, R.Y + 30.0f, IgniteBi(14.0f, Hex(IgSun)));
		// Core's figures: the starting stack and the first level (S-J's calibration moves them, so the copy reads them).
		const std::vector<Level> Lv = fuse::Levels();
		const std::string Blinds = Lv.empty() ? std::string("10/20") : Grouped(Lv.front().Sb) + "/" + Grouped(Lv.front().Bb);
		IgniteParagraphTimes(C, rlsites_detail::SitesFill(IgniteFuseHow, {{"Stack", Grouped(fuse::StartingStack())}, {"Blinds", Blinds}}), R.X + 22.0f, R.Y + 54.0f, R.W - 44.0f, 13.5f, 600,
			Hex(K.Cream, 0.85f), 18.0f, 2);
	}
	// The Fuse Wall and your numbers (yours only).
	IgniteFuseWallCard(RL, L, K, {1140.0f, 88.0f, 436.0f, 744.0f});
}

void IgniteFuseWallCard(RiverLine& RL, const IgniteLoops& L, const igniteart::IgniteTones& K, const Rect& R)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	{
		IgpCard(C, R, K);
		C.Text("THE FUSE WALL", R.X + 24.0f, R.Y + 42.0f, IgniteBi(24.0f, Hex(K.Cream)));
		const std::pair<std::string, std::string> Stats[4] = {
			{Grouped(L.Fuse.Played), "PLAYED"}, {Grouped(L.Fuse.Won), "WON"}, {(L.Fuse.NetCents >= 0 ? "+" : "") + Money(L.Fuse.NetCents), "NET"}, {L.Fuse.BestM > 0 ? std::string() : std::string("\xE2\x80\x94"), "BEST"}};
		for (int I = 0; I < 4; ++I)
		{
			const float X = R.X + 24.0f + static_cast<float>(I) * 100.0f;
			if (I == 3 && L.Fuse.BestM > 0)
			{
				IgpTimes(C, X, R.Y + 92.0f, 22.0f, L.Fuse.BestM, Hex(IgniteFuseTint(L.Fuse.BestM, K)));
			}
			else
			{
				C.Text(Stats[I].first, X, R.Y + 92.0f, IgniteBi(22.0f, I == 2 ? (L.Fuse.NetCents >= 0 ? Hex(IgLime) : Hex(IgRed)) : Hex(K.Cream), Align::Left, Baseline::Alphabetic, 96.0f));
			}
			C.Text(Stats[I].second, X, R.Y + 112.0f, IgniteTx(10.0f, 900, pal::Muted));
		}
		C.FillRect({R.X + 24.0f, R.Y + 132.0f, R.W - 48.0f, 1.0f}, Hex(K.Rim));
		if (L.Fuse.Wall.empty())
		{
			IgniteParagraphTimes(C, IgniteNoWall, R.X + 24.0f, R.Y + 170.0f, R.W - 48.0f, 14.0f, 600, pal::Muted, 20.0f, 3);
		}
		for (size_t I = 0; I < L.Fuse.Wall.size() && I < 10; ++I)
		{
			const site::play::WallRow& W = L.Fuse.Wall[I];
			const float Y = R.Y + 150.0f + static_cast<float>(I) * 58.0f;
			C.FillRoundRect({R.X + 16.0f, Y, R.W - 32.0f, 50.0f}, 14.0f, Hex(I % 2 == 0 ? K.Well : K.PanelBot));
			IgpTimes(C, R.X + 34.0f, Y + 34.0f, 24.0f, W.M, Hex(IgniteFuseTint(W.M, K)));
			C.Text(W.Place >= 1 ? Ordinal(W.Place) : std::string("\xE2\x80\x94"), R.X + 170.0f, Y + 32.0f, IgniteTx(14.0f, 800, W.Place == 1 ? Hex(IgSun) : Hex(K.Cream)));
			C.Text(net::DateLabel(W.Day), R.X + 230.0f, Y + 32.0f, IgniteTx(12.5f, 600, pal::Muted));
			C.Text(W.Prize > 0 ? "+" + Money(W.Prize) : std::string("\xE2\x80\x94"), R.X + R.W - 32.0f, Y + 33.0f, IgniteBi(17.0f, W.Prize > 0 ? Hex(IgLime) : pal::Muted, Align::Right));
		}
	}
}

// ------------------------------------------------------------------ the overlays (house law 3: one at a time, each holds the pointer)

namespace
{
/** The tier's seven multipliers, smallest first (core's rows: x1,000 at $1.10, x250 above). */
std::array<int, 7> IgpStrip(int Tier)
{
	std::array<int, 7> S{};
	for (size_t J = 0; J < 7; ++J)
	{
		S[J] = std::max(2, fuse::MOf(fuse::Table()[J], Tier));
	}
	return S;
}

int IgpStripIndex(const std::array<int, 7>& S, int M)
{
	int Index = 0;
	for (int I = 0; I < 7; ++I)
	{
		Index = S[static_cast<size_t>(I)] == M ? I : Index;
	}
	return Index;
}
} // namespace

int IgniteReelFinal(int M, int Tier)
{
	const int Loops = M >= 25 ? 4 : M >= 5 ? 3 : 2;
	return Loops * 7 + IgpStripIndex(IgpStrip(Tier), M);
}

int IgniteReelAt(int M, int Tier, int I)
{
	// W5: the reel never slows past a prize you didn't win. Its last six symbols before the result are the rungs below it,
	// climbing into it (padded with the smallest), so whatever passes slowly is smaller; the cycles before them run at speed.
	const std::array<int, 7> S = IgpStrip(Tier);
	const int Index = IgpStripIndex(S, M);
	const int Back = IgniteReelFinal(M, Tier) - I;
	if (Back == 0)
	{
		return M;
	}
	if (Back > 0 && Back <= 6)
	{
		return S[static_cast<size_t>(std::max(0, Index - Back))];
	}
	return S[static_cast<size_t>(((I % 7) + 7) % 7)];
}

IgniteRevealTimes IgniteRevealTimesFor(int M)
{
	IgniteRevealTimes T;
	if (M >= 25)
	{
		T.FuseEnd = 1.9;
		T.ReelEnd = 4.3;
		T.BoomEnd = 6.3;
	}
	else if (M >= 5)
	{
		T.FuseEnd = 1.2;
		T.ReelEnd = 2.4;
		T.BoomEnd = 3.4;
	}
	return T;
}

void IgniteStartSpin(IgniteUi& U, const IgniteLoops& L, double Now)
{
	U.Spin = true;
	U.SpinAt = Now;
	U.SpinRing = std::max(1, std::min(3, L.SpunRing > 0 ? L.SpunRing : L.RingsLit));
	U.SpinWedge = L.SpunWedge;
	U.Drawer = 0;
}

void IgniteStartCrack(IgniteUi& U, const IgniteLoops& L, double Now)
{
	U.Crack = true;
	U.CrackAt = Now;
	U.CrackShown = 0;
	U.Cracked = L.Opened;
}

void IgniteStartReveal(IgniteUi& U, const Session& S, int Table, double Now)
{
	U.RevealTable = Table;
	U.RevealEvent = Table >= 0 && Table < S.TableCount() ? S.Glance(Table).EventId : std::string();
	U.RevealAt = Now;
	U.RevealSkip = -1.0;
	U.RevealAway = false;
	U.Drawer = 0;
}

namespace
{
/** The reveal is over (its seat taken, its hand dealt while it was away, or the player's own clock open at its table): it never
 *  plays again. One that was up takes its Odds drawer with it (Drawer 2 is its own then; while it was away, whatever drawer is
 *  open is the page's and stays). */
void IgpRevealDone(IgniteUi& U)
{
	if (!U.RevealAway && U.Drawer == 2)
	{
		U.Drawer = 0;
	}
	U.RevealsDone.insert(U.RevealEvent);
	U.RevealEvent.clear();
	U.RevealTable = -1;
	U.RevealAway = false;
}

/**
 * Where the FUSE reveal stands this frame (PIE G2 P7); true while it's up. It's up only while its own table is the one in front
 * (Session::FocusedTable, as AutoFocusStep, a tab or a tile leaves it). With another table in front nothing of it draws or holds
 * the pointer, its Odds drawer closes with it, and the laptop stops telling core it's drawn (fuse::RevealShown), so core goes by
 * its clock alone: the reveal counts as seen its run and a second after the light, and the table deals by itself (never one
 * stuck with no prompt). Back in front before that first deal, it plays on from its clock, or from the boom if its reel landed
 * while it was away, and the first deal waits for it again (the laptop draws it: RevealShown). Back after the first deal went,
 * or the deal going while it's away, ends it (nothing covers a dealt hand; the marquee carries the multiplier). And whatever
 * brought its table forward, it never covers the player's own clock there, its Odds drawer included.
 */
bool IgpRevealSettle(RiverLine& RL, IgniteUi& U, double Now)
{
	Session& S = Access::S(RL);
	const bool AtTable = U.RevealTable >= 0 && U.RevealTable < S.TableCount() && S.Glance(U.RevealTable).EventId == U.RevealEvent;
	const bool Front = AtTable && S.FocusedTable() == U.RevealTable;
	const bool Dealt = fuse::FirstDealAt(S, U.RevealEvent) >= 0.0;
	if (!Front)
	{
		if (Dealt)
		{
			IgpRevealDone(U);
		}
		else if (!U.RevealAway)
		{
			U.RevealAway = true;
			U.Drawer = 0; // its Odds drawer goes with it (the only drawer the reveal lets up)
		}
		return false;
	}
	if (U.RevealAway)
	{
		if (Dealt)
		{
			IgpRevealDone(U);
			return false;
		}
		U.RevealAway = false;
		const IgniteRevealTimes Tm = IgniteRevealTimesFor(std::max(2, S.FuseReveal(U.RevealTable).M));
		if (Now - U.RevealAt >= Tm.ReelEnd)
		{
			U.RevealAt = Now - Tm.ReelEnd; // the boom again, then the table opens itself a second after it
		}
		U.RevealSkip = -1.0;
		U.Drawer = 0;
	}
	// The player's own clock open at its table (a plan dealt under it, M3's exception, and handed the seat back: K): it's over,
	// its Odds drawer with it.
	if (S.HasPrompt)
	{
		IgpRevealDone(U);
		return false;
	}
	return true;
}
} // namespace

int IgniteOverlay(RiverLine& RL, IgniteUi& U, double Now)
{
	if (!U.RevealEvent.empty() && IgpRevealSettle(RL, U, Now))
	{
		return 1;
	}
	if (U.Spin)
	{
		return 2;
	}
	if (U.Crack)
	{
		return 3;
	}
	if (U.BuyItem >= 0)
	{
		return 4;
	}
	if (U.Drawer > 0)
	{
		return 5;
	}
	return 0;
}

Rect IgniteOverlayRect(int Overlay, const IgniteUi& U)
{
	(void)U;
	switch (Overlay)
	{
	case 1: return {0.0f, 0.0f, 1600.0f, 962.0f};    // the reveal: any click is its own (a skip)
	case 2: return {500.0f, 150.0f, 600.0f, 720.0f};  // the wheel and its prize card
	case 3: return {480.0f, 230.0f, 640.0f, 540.0f};  // the cracker and its prize card
	case 4: return {560.0f, 330.0f, 480.0f, 260.0f};  // the confirm card
	case 5: return {1140.0f, 88.0f, 436.0f, 774.0f}; // a drawer
	default: return {0.0f, 0.0f, 0.0f, 0.0f};
	}
}

std::string IgniteRevealOdds(int M, int Tier)
{
	for (const fuse::Row& Row : fuse::Table())
	{
		if (Row.M <= 0 || fuse::MOf(Row, Tier) != M)
		{
			continue;
		}
		const double P = fuse::POf(Row, Tier);
		if (P <= 0.0)
		{
			break;
		}
		const std::string At = " at " + Money(fuse::BuyInOf(Tier));
		const double Inv = 1.0 / P;
		const double N = std::round(Inv);
		if (N >= 2.0 && std::fabs(Inv - N) <= 0.005 * Inv)
		{
			return "1 in " + Grouped(static_cast<int64_t>(N)) + At;
		}
		return Fixed(P * 100.0, P < 0.01 ? 2 : 1) + "%" + At;
	}
	return std::string();
}

namespace
{
/** The scrim under a modal overlay. */
void IgpScrim(Canvas& C, double Since, bool Preview, float A = 0.78f)
{
	const float Fade = Preview ? 1.0f : IgpEase(Since / 0.2);
	C.FillRect({0.0f, 0.0f, 1600.0f, 962.0f}, Hex(igniteart::IgNight, A * Fade));
}

void IgpDrawer(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim);

/** The FUSE reveal (SPEC §2.4 D): the fuse burns, the reel lands on core's multiplier from below, the boom. */
void IgpReveal(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim)
{
	using namespace igniteart;
	(void)Scrim;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const fuse::Reveal Rv = U.RevealTable >= 0 ? S.FuseReveal(U.RevealTable) : fuse::Reveal();
	const int M = std::max(2, Rv.M);
	const int Tier = std::max(0, std::min(2, Rv.Tier));
	const Chips BuyIn = IgpFuseBuyIn(Tier);
	const IgniteRevealTimes Tm = IgniteRevealTimesFor(M);
	double T = Preview ? Tm.BoomEnd : Now - U.RevealAt;
	// Any click after the fuse skips to the boom (swallowed: nothing under the reveal ever sees it).
	const bool Click = RL.UI.Ptr.Active && RL.UI.Ptr.Released;
	if (Click && T >= Tm.FuseEnd && T < Tm.ReelEnd && !Preview)
	{
		U.RevealAt = Now - Tm.ReelEnd;
		T = Tm.ReelEnd;
	}
	const std::string Key = "reveal:" + U.RevealEvent;
	// Core holds the table's first deal while the laptop draws its reveal (the Odds drawer can keep it past its run; G2 M3).
	if (!Preview)
	{
		fuse::RevealShown(S, U.RevealEvent);
	}
	IgpScrim(C, T, Preview, 0.82f);
	// The shake on x10 and up, the confetti from x25.
	Vec2 Sh{0.0f, 0.0f};
	if (M >= 10 && T >= Tm.ReelEnd && !Preview)
	{
		Sh = motion::Shake(Now, U.RevealAt + Tm.ReelEnd, Fnv1a(Key));
	}
	C.Save();
	C.Translate(Sh.X, Sh.Y);
	// The chosen cabinet's card at the left.
	{
		const Rect R{112.0f, 300.0f, 300.0f, 160.0f};
		C.FillRoundRect(R, 22.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x3a1f5c), Hex(0x1d1030)));
		IgniteNeonRoundRect(C, R, 22.0f, Hex(IgSun, 0.8f), 1.5f, 5.0f);
		C.Text(Money(BuyIn), R.X + 26.0f, R.Y + 56.0f, IgniteBi(36.0f, Hex(IgCream)));
		// This FUSE's own number at its tier (its id's ordinal): the newest one's is the tier's count, an older one's (a reveal
		// back in front after another was lit) its own.
		const int Ordinal = fuse::OrdinalOf(U.RevealEvent);
		C.Text("FUSE \xC2\xB7 #" + Grouped(std::max(1, Ordinal > 0 ? Ordinal : IgniteLoopsOf(RL, Now).Fuse.N[static_cast<size_t>(Tier)])) + " at this tier", R.X + 26.0f, R.Y + 88.0f,
			IgniteTx(13.0f, 700, Hex(K.Cream)));
		rlnet_detail::NetParagraph(C, IgniteFuseKeyed, R.X + 26.0f, R.Y + 116.0f, R.W - 52.0f, 12.0f, 700, Hex(IgLime), 16.0f, 2);
	}
	// The fuse: a wavy rope from (180,520) to (700,520), burning to the reel.
	{
		const double Burn = Clamp01((T - 0.15) / std::max(0.1, Tm.FuseEnd - 0.15));
		IgniteFuseRope(C, 180.0f, 600.0f, 520.0f, 5.0f, Burn, Hex(0xc9a46a), Hex(0x5a4060), Now, Burn < 1.0);
		if (T < Tm.FuseEnd)
		{
			IgniteCue(S, U, Key + ":fuse", SoundId::IgFuse, 0.4f, Preview);
		}
	}
	// The reel: one clipped window, the symbols approaching from below and slowing onto the true result.
	const double Final = static_cast<double>(IgniteReelFinal(M, Tier));
	const auto SymAt = [M, Tier](int I) { return IgniteReelAt(M, Tier, I); };
	double Pos = 0.0;
	if (T >= Tm.FuseEnd)
	{
		const double Uu = Clamp01((T - Tm.FuseEnd) / std::max(0.1, Tm.ReelEnd - Tm.FuseEnd));
		if (M >= 25)
		{
			// Slowing a second time over its last three symbols.
			Pos = Uu < 0.6 ? (Final - 3.0) * (1.0 - std::pow(1.0 - Uu / 0.6, 3.0)) : (Final - 3.0) + 3.0 * (1.0 - std::pow(1.0 - (Uu - 0.6) / 0.4, 3.0));
		}
		else
		{
			Pos = Final * (1.0 - std::pow(1.0 - Uu, 3.0));
		}
		const int Passed = static_cast<int>(std::floor(Pos));
		if (T < Tm.ReelEnd)
		{
			IgniteCue(S, U, Key + ":tick" + std::to_string(Passed), SoundId::IgTick, 0.3f, Preview);
		}
	}
	const Rect Win{600.0f, 360.0f, 400.0f, 240.0f};
	C.GlowRoundRect({Win.X - 8.0f, Win.Y - 8.0f, Win.W + 16.0f, Win.H + 16.0f}, 30.0f, Hex(IgSun, 0.35f), 18.0f);
	C.FillRoundRect(Win, 26.0f, Hex(0x0d0712));
	C.PushClip(Win);
	for (int D = -2; D <= 2; ++D)
	{
		const int I = static_cast<int>(std::floor(Pos)) + D;
		const double Off = static_cast<double>(I) - Pos;
		const float Y = 480.0f + static_cast<float>(Off) * 110.0f + 32.0f;
		const int Sym = SymAt(I);
		const bool Centre = std::fabs(Off) < 0.5;
		IgpTimes(C, 800.0f, Y, Centre && T >= Tm.ReelEnd ? 92.0f : 72.0f, Sym, Centre ? Hex(IgniteFuseTint(Sym, K)) : Hex(IgCream, 0.35f), Align::Center);
	}
	C.PopClip();
	C.StrokeRoundRect({Win.X + 8.0f, 424.0f, Win.W - 16.0f, 112.0f}, 16.0f, Hex(IgPink), 3.0f);
	C.FillPolygon({{Win.X - 6.0f, 468.0f}, {Win.X + 12.0f, 480.0f}, {Win.X - 6.0f, 492.0f}}, Hex(IgPink));
	C.FillPolygon({{Win.X + Win.W + 6.0f, 468.0f}, {Win.X + Win.W - 12.0f, 480.0f}, {Win.X + Win.W + 6.0f, 492.0f}}, Hex(IgPink));
	// The boom: the multiplier springs in, the pool counts up.
	if (T >= Tm.ReelEnd)
	{
		IgniteCue(S, U, Key + ":boom", SoundId::IgBoom, M >= 25 ? 0.6f : 0.45f, Preview);
		const float Pop = Preview ? 1.0f : static_cast<float>(motion::Spring(Now, U.RevealAt + Tm.ReelEnd));
		const Chips Pool = static_cast<Chips>(M) * BuyIn;
		const double Shown = Preview ? static_cast<double>(Pool) : motion::CountUp(0.0, static_cast<double>(Pool), Now, U.RevealAt + Tm.ReelEnd, std::max(0.4, Tm.BoomEnd - Tm.ReelEnd - 0.2));
		C.Save();
		C.Translate(800.0f, 230.0f);
		C.Scale(Pop, Pop);
		IgniteSticker(C, 0.0f, 0.0f, 120.0f, "", Hex(IgPink), Hex(IgDarkInk), 10.0f, 18, 0.78f);
		C.Text("PRIZE POOL", 0.0f, -34.0f, IgniteTx(14.0f, 900, Hex(IgCream), Align::Center));
		C.Text(Money(static_cast<Chips>(std::llround(Shown))), 0.0f, 28.0f, IgniteBi(52.0f, Hex(IgCream), Align::Center));
		C.Restore();
		// The mockup's column (ignite_fuse_reveal.html): the name, how rare this multiplier is at this stake (its own row of
		// core's table: "1 in 250 at $1.10"), how it pays and what each place gets; ODDS under it (after the shake, below).
		const std::array<Chips, 3> Pay = fuse::Payout(Tier, M);
		C.Text(M >= 25 ? "BIG FUSE" : "FUSE", 1060.0f, 420.0f, IgniteBi(30.0f, Hex(IgSun)));
		C.Text(IgniteRevealOdds(M, Tier), 1060.0f, 448.0f, IgniteTx(15.0f, 700, Hex(K.Cream)));
		C.Text(M >= 25 ? std::string("pays 80 / 12 / 8") : std::string("winner takes all"), 1060.0f, 472.0f, IgniteTx(15.0f, 700, Hex(K.Cream)));
		C.Text(M >= 25 ? Money(Pay[0]) + " \xC2\xB7 " + Money(Pay[1]) + " \xC2\xB7 " + Money(Pay[2]) : Money(Pay[0]), 1060.0f, 496.0f, IgniteTx(15.0f, 800, Hex(IgLime)));
		if (M >= 25 && !Preview)
		{
			motion::Confetti(C, Fnv1a(Key), U.RevealAt + Tm.ReelEnd, Now, M >= 100 ? 200 : 150, {0.0f, 0.0f, 1600.0f, 962.0f});
		}
		if (M >= 100 && !Preview)
		{
			const float Sh2 = static_cast<float>(std::fmod((Now - U.RevealAt) * 0.5, 1.0));
			C.FillRect({0.0f, 0.0f, 1600.0f, 962.0f}, Hex(IgSun, 0.06f * (1.0f - Sh2)));
		}
	}
	C.Restore();
	// The three players, then TAKE YOUR SEAT (or the table opens itself 1 s after the boom).
	if (T >= Tm.ReelEnd)
	{
		// ODDS (W5: the table this FUSE was drawn from, one click away; the mockup's pill under the payouts): FUSE's odds drawer
		// over the reveal. While it's up the reveal waits (the table doesn't open itself, TAKE YOUR SEAT is held) and a click
		// outside it closes it, swallowed; the table opens itself 1 s after it closes.
		const bool Odds = U.Drawer == 2;
		{
			const Rect O{1060.0f, 512.0f, 92.0f, 30.0f};
			const Ui::ClickState St = RL.UI.Clickable("revealodds", O, !Preview && !Odds);
			IgnitePill(C, O, Hex(St.Hover ? K.Panel : K.Well), Hex(K.RimHi), 1.2f);
			C.Text("ODDS", O.X + O.W / 2.0f, O.Y + 20.0f, IgniteTx(13.0f, 900, Hex(K.Cream), Align::Center));
			if (St.Clicked)
			{
				U.Drawer = 2;
				U.DrawerAt = Now;
			}
		}
		C.Text("THREE PLAYERS \xC2\xB7 ONE FUSE", 800.0f, 660.0f, IgniteTx(13.0f, 900, Hex(K.Cream, 0.8f), Align::Center));
		// The FUSE's own three, read at its own table (WithTable: the front table's seats are someone else's whenever its table
		// isn't the one in front; P7).
		std::vector<std::string> Names;
		if (U.RevealTable >= 0 && U.RevealTable < S.TableCount() && S.Glance(U.RevealTable).EventId == U.RevealEvent)
		{
			S.WithTable(U.RevealTable, [&S, &Names]() {
				for (const SeatVis& Sv : S.Seats)
				{
					if (Sv.Present)
					{
						Names.push_back(Sv.Name);
					}
				}
			});
		}
		for (size_t I = 0; I < Names.size() && I < 3; ++I)
		{
			const float X = 800.0f + (static_cast<float>(I) - static_cast<float>(std::min<size_t>(3, Names.size()) - 1) / 2.0f) * 160.0f;
			DrawAvatar(C, X, 712.0f, 30.0f, AvatarFor(Names[I]));
			C.StrokeArc(X, 712.0f, 32.0f, 0.0f, 2.0f * Pi, Names[I] == S.HeroName ? Hex(IgSun) : Hex(IgViolet), 3.0f);
			C.Text(Names[I], X, 764.0f, IgniteTx(13.5f, 800, Hex(K.Cream), Align::Center, Baseline::Alphabetic, 150.0f));
		}
		const bool Go = IgpButton(RL, "fuseseat", {620.0f, 800.0f, 360.0f, 72.0f}, "Take your seat", ButtonKind::Primary, 24.0f, !Preview && !Odds);
		C.Text(IgniteFuseSkip, 800.0f, 900.0f, IgniteTx(12.5f, 600, pal::Muted, Align::Center));
		const bool Auto = T >= Tm.BoomEnd + 1.0 && !Odds && (U.RevealSkip < 0.0 || Now >= U.RevealSkip + 1.0);
		if (Odds && !Preview)
		{
			const Rect Dr = IgniteOverlayRect(5, U);
			const bool Outside = RL.UI.Ptr.Active && RL.UI.Ptr.Released && !(RL.UI.Ptr.X >= Dr.X && RL.UI.Ptr.X < Dr.X + Dr.W && RL.UI.Ptr.Y >= Dr.Y && RL.UI.Ptr.Y < Dr.Y + Dr.H);
			IgpDrawer(RL, U, K, Now, Preview, Outside);
			if (U.Drawer != 2)
			{
				U.RevealSkip = Now; // closed: the table opens itself a second from now
			}
		}
		if ((Go || (Auto && !Preview)) && !Preview)
		{
			// The seat taken (or the table opening itself): its first hand deals on the usual beat (core's HoldFirstDeal).
			fuse::TakeSeat(S, U.RevealEvent);
			const int Table = U.RevealTable;
			IgpRevealDone(U);
			if (Table >= 0 && Table < S.TableCount())
			{
				S.ShowTables();
				S.FocusTable(Table);
			}
		}
	}
	else
	{
		C.Text(IgniteFuseSkip, 800.0f, 900.0f, IgniteTx(12.5f, 600, pal::Muted, Align::Center));
	}
}

/** The wheel's spin (SPEC §2.4 H): 3.2 s of deceleration onto core's wedge, labels upright, the prize card. */
void IgpSpin(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	const std::vector<IgniteWedge>& Ws = L.Rings[static_cast<size_t>(std::max(0, std::min(2, U.SpinRing - 1)))];
	const int N = std::max(1, static_cast<int>(Ws.size()));
	double T = Preview ? 3.2 : Now - U.SpinAt;
	const bool Landed = T >= 3.2;
	if (Scrim || (RL.UI.Ptr.Active && RL.UI.Ptr.Released && !Landed))
	{
		if (!Landed)
		{
			U.SpinAt = Now - 3.2; // a click skips to the landing
			T = 3.2;
		}
		else if (Scrim)
		{
			U.Spin = false;
			return;
		}
	}
	IgpScrim(C, T, Preview);
	const float Cx = 800.0f;
	const float Cy = 480.0f;
	const float R = 300.0f;
	const float Step = 2.0f * Pi / static_cast<float>(N);
	const int W = std::max(0, std::min(N - 1, U.SpinWedge));
	float Target = -Pi / 2.0f - (static_cast<float>(W) + 0.5f) * Step;
	Target = std::fmod(Target, 2.0f * Pi);
	Target = Target < 0.0f ? Target + 2.0f * Pi : Target;
	const float Theta1 = Target + 2.0f * Pi * 5.0f;
	const float Tt = static_cast<float>(Clamp01(T / 3.2));
	const float Theta = Theta1 - Theta1 * std::pow(1.0f - Tt, 3.0f);
	// The wedges and their labels riding upright, like gondolas.
	IgniteWheelFace(C, Cx, Cy, R, N, U.SpinRing, Theta, K, 1.0f);
	for (int I = 0; I < N && I < static_cast<int>(Ws.size()); ++I)
	{
		const float A = Theta + (static_cast<float>(I) + 0.5f) * Step;
		const IgniteWedge& Wd = Ws[static_cast<size_t>(I)];
		const std::string Lab = Wd.TicketId.empty() ? Grouped(Wd.Sparks) : std::string("TICKET");
		// In the band between SHINE and GLOW's rim, off the lit rings (G2_AUDIT L9).
		IgpWheelLabel(C, Lab, Cx, Cy, IgniteWheelBandMid(R), IgniteWheelBandRoom(R), A, Wd.TicketId.empty() ? 22.0f : 15.0f, Wd.TicketId.empty() ? Hex(IgCream) : Hex(IgSun));
	}
	// The flapper at the top; a tick a wedge under it.
	C.FillPolygon({{Cx - 16.0f, Cy - R - 34.0f}, {Cx + 16.0f, Cy - R - 34.0f}, {Cx, Cy - R + 4.0f}}, Hex(IgPink));
	if (!Landed)
	{
		const int Crossed = static_cast<int>(std::floor(Theta / Step));
		IgniteCue(S, U, "spin:" + std::to_string(static_cast<long long>(U.SpinAt * 1000.0)) + ":" + std::to_string(Crossed), SoundId::IgTick, 0.3f, Preview);
	}
	else if (U.SpinWedge >= 0 && U.SpinWedge < static_cast<int>(Ws.size()))
	{
		IgniteCue(S, U, "spin:" + std::to_string(static_cast<long long>(U.SpinAt * 1000.0)) + ":land", SoundId::IgPop, 0.45f, Preview);
		const IgniteWedge& Wd = Ws[static_cast<size_t>(U.SpinWedge)];
		const float Pop = Preview ? 1.0f : static_cast<float>(motion::Spring(Now, U.SpinAt + 3.2));
		C.Save();
		C.Translate(Cx, Cy + 40.0f);
		C.Scale(Pop, Pop);
		const Rect Card{-210.0f, -70.0f, 420.0f, 140.0f};
		C.FillRoundRect(Card, 24.0f, Paint::Linear({0.0f, Card.Y}, {0.0f, Card.Y + Card.H}, Hex(0x3a1f5c), Hex(0x1d1030)));
		IgniteNeonRoundRect(C, Card, 24.0f, Hex(IgLime, 0.9f), 2.0f, 6.0f);
		C.Text(std::string(IgniteRingName(U.SpinRing)) + " RING", 0.0f, -32.0f, IgniteTx(12.0f, 900, Hex(IgLime), Align::Center));
		C.Text(Wd.TicketId.empty() ? "+" + Grouped(Wd.Sparks) + " SPARKS" : Wd.Label, 0.0f, 16.0f, IgniteBi(Wd.TicketId.empty() ? 40.0f : 26.0f, Hex(IgCream), Align::Center, Baseline::Alphabetic, 390.0f));
		C.Text(L.SpunHeld ? std::string("ON THE GUEST LIST") : std::string("ADDED"), 0.0f, 48.0f, IgniteTx(12.0f, 900, L.SpunHeld ? Hex(IgSun) : pal::Muted, Align::Center));
		C.Restore();
		if (IgpButton(RL, "spinx", {700.0f, 820.0f, 200.0f, 56.0f}, "Nice", ButtonKind::Primary, 18.0f, !Preview))
		{
			U.Spin = false;
		}
	}
}

/** A cracker's open (SPEC §2.4 H): PULL, the tug, the bang, the prize card; OPEN ALL chains them 0.6 s apart. */
void IgpCrack(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	const bool Pulled = U.CrackAt > -50.0;
	const double T = Pulled ? (Preview ? 100.0 : Now - U.CrackAt) : 0.0;
	const size_t Count = U.Cracked.size();
	const bool Done = Pulled && (Count == 0 || T >= 0.6 * static_cast<double>(Count - 1) + 1.1);
	if (Scrim && (!Pulled || Done))
	{
		U.Crack = false;
		return;
	}
	IgpScrim(C, Pulled ? T + 1.0 : 1.0, Preview);
	// Which cracker is up: before the pull, the one asked for (or the first, for OPEN ALL); after, the chain's.
	int Tier = 0;
	if (!Pulled)
	{
		for (const site::play::Box& B : L.Crackers)
		{
			if (B.Id == U.CrackId || U.CrackId == "*")
			{
				Tier = B.Tier;
				break;
			}
		}
	}
	const size_t Up = Count == 0 ? 0 : std::min(Count - 1, static_cast<size_t>(std::max(0.0, T / 0.6)));
	if (Pulled && Count > 0)
	{
		Tier = U.Cracked[Up].Tier;
	}
	const double Local = Pulled ? T - 0.6 * static_cast<double>(Up) : 0.0;
	const float Tug = Pulled ? static_cast<float>(Clamp01(Local / 0.3)) : 0.0f;
	if (!Pulled || Local < 0.3)
	{
		IgniteCracker(C, 800.0f, 440.0f, 420.0f, Tier, Tug);
	}
	if (!Pulled)
	{
		C.Text(U.CrackId == "*" ? "PULL THEM ALL" : std::string(IgpCrackerNames[std::min(2, std::max(0, Tier))]) + " CRACKER", 800.0f, 330.0f, IgniteBi(28.0f, Hex(IgCream), Align::Center));
		if (IgpButton(RL, "crkpull", {640.0f, 560.0f, 320.0f, 72.0f}, "Pull", ButtonKind::Primary, 24.0f, !Preview))
		{
			const std::string Why = S.OpenCracker(U.CrackId);
			U.LoopsAt = -1.0;
			if (Why.empty())
			{
				IgniteStartCrack(U, IgniteLoopsOf(RL, Now), Now);
			}
			else
			{
				IgniteNoteRefusal(U, "crkpull", Why, Now);
			}
		}
		IgpNote(C, U, "crkpull", 800.0f, 660.0f, 400.0f, Now, Align::Center);
		if (IgpButton(RL, "crkno", {700.0f, 680.0f, 200.0f, 48.0f}, "Not now", ButtonKind::Ghost, 15.0f, !Preview))
		{
			U.Crack = false;
		}
		return;
	}
	if (Count == 0)
	{
		U.Crack = false;
		return;
	}
	const IgniteOpened& O = U.Cracked[Up];
	if (Local >= 0.3)
	{
		IgniteCue(S, U, "crack:" + std::to_string(static_cast<long long>(U.CrackAt * 1000.0)) + ":" + std::to_string(Up), SoundId::IgPop, 0.5f, Preview);
		if (!Preview)
		{
			motion::Confetti(C, Fnv1a(O.BoxId + ":crack"), U.CrackAt + 0.6 * static_cast<double>(Up) + 0.3, Now, 120, {460.0f, 260.0f, 680.0f, 420.0f});
		}
		const float Pop = Preview ? 1.0f : static_cast<float>(motion::Spring(Now, U.CrackAt + 0.6 * static_cast<double>(Up) + 0.3));
		C.Save();
		C.Translate(800.0f, 440.0f);
		C.Scale(Pop, Pop);
		const Rect Card{-220.0f, -80.0f, 440.0f, 160.0f};
		C.FillRoundRect(Card, 26.0f, Paint::Linear({0.0f, Card.Y}, {0.0f, Card.Y + Card.H}, Hex(0x3a1f5c), Hex(0x1d1030)));
		IgniteNeonRoundRect(C, Card, 26.0f, Hex(O.Tier == 2 ? IgSun : O.Tier == 1 ? IgPink : IgCream, 0.9f), 2.0f, 6.0f);
		C.Text(std::string(IgpCrackerNames[std::min(2, std::max(0, O.Tier))]) + " CRACKER", 0.0f, -40.0f, IgniteTx(12.0f, 900, pal::Muted, Align::Center));
		C.Text(O.Prize, 0.0f, 14.0f, IgniteBi(O.Sparks > 0 ? 40.0f : 26.0f, O.Sparks > 0 ? Hex(IgLime) : Hex(IgSun), Align::Center, Baseline::Alphabetic, 410.0f));
		C.Text(O.Held ? std::string("ON THE GUEST LIST") : O.Sticker ? std::string("AND A STICKER") : std::string(), 0.0f, 50.0f, IgniteTx(12.0f, 900, Hex(IgSun), Align::Center));
		C.Restore();
	}
	// OPEN ALL: what's come out so far, and SKIP.
	if (Count > 1)
	{
		std::string So;
		for (size_t I = 0; I <= Up; ++I)
		{
			So += (I ? " \xC2\xB7 " : "") + U.Cracked[I].Prize;
		}
		C.Text(std::to_string(Up + 1) + " of " + std::to_string(Count), 800.0f, 600.0f, IgniteTx(13.0f, 900, Hex(IgSun), Align::Center));
		C.Text(So, 800.0f, 626.0f, IgniteTx(13.0f, 600, Hex(K.Cream), Align::Center, Baseline::Alphabetic, 1100.0f));
		if (!Done && IgpButton(RL, "crkskip", {720.0f, 660.0f, 160.0f, 44.0f}, "Skip", ButtonKind::Ghost, 15.0f, !Preview))
		{
			U.CrackAt = Now - 0.6 * static_cast<double>(Count - 1) - 1.1;
		}
	}
	if (Done && IgpButton(RL, "crkdone", {700.0f, 720.0f, 200.0f, 56.0f}, "Nice", ButtonKind::Primary, 18.0f, !Preview))
	{
		U.Crack = false;
	}
}

/** The Prize Counter's confirm card: a BUY spends nothing until it's pressed here. */
void IgpBuy(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	if (Scrim)
	{
		U.BuyItem = -1;
		return;
	}
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	const IgnitePrize* Pz = nullptr;
	for (const IgnitePrize& P : L.Prizes)
	{
		Pz = P.Item == U.BuyItem ? &P : Pz;
	}
	if (!Pz)
	{
		U.BuyItem = -1;
		return;
	}
	IgpScrim(C, Now - U.BuyAt, Preview, 0.6f);
	const Rect R{560.0f, 330.0f, 480.0f, 260.0f};
	C.FillRoundRect(R, 24.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(K.PanelTop), Hex(K.PanelBot)));
	IgniteNeonRoundRect(C, R, 24.0f, Hex(IgLime, 0.8f), 1.5f, 5.0f);
	C.Text("PRIZE COUNTER", R.X + 28.0f, R.Y + 40.0f, IgniteTx(12.0f, 900, Hex(IgSun)));
	C.Text(Pz->Label, R.X + 28.0f, R.Y + 82.0f, IgniteBi(26.0f, Hex(K.Cream), Align::Left, Baseline::Alphabetic, R.W - 56.0f));
	IgniteSpark(C, R.X + 36.0f, R.Y + 110.0f, 8.0f, Hex(IgLime));
	C.Text(Grouped(Pz->Sparks) + " Sparks", R.X + 52.0f, R.Y + 118.0f, IgniteBi(20.0f, Hex(IgLime)));
	C.Text(IgniteBuyAsk, R.X + 28.0f, R.Y + 150.0f, IgniteTx(13.5f, 600, pal::Muted));
	if (IgpButton(RL, "buyok", {R.X + 24.0f, R.Y + 176.0f, 206.0f, 60.0f}, "Buy", ButtonKind::Primary, 19.0f, !Preview))
	{
		const std::string Why = S.BuyPrize(Pz->Item);
		U.LoopsAt = -1.0;
		if (Why.empty())
		{
			S.UiSound(SoundId::IgSpark, 0.45f);
			U.BuyItem = -1;
		}
		else
		{
			IgniteNoteRefusal(U, "buyok", Why, Now);
		}
	}
	if (IgpButton(RL, "buyno", {R.X + 250.0f, R.Y + 176.0f, 206.0f, 60.0f}, "Cancel", ButtonKind::Ghost, 17.0f, !Preview))
	{
		U.BuyItem = -1;
	}
	IgpNote(C, U, "buyok", R.X + R.W / 2.0f, R.Y + R.H + 22.0f, R.W, Now, Align::Center);
}

/** "about 4 Flip & Runs": the rake left in $1.10 entries (12 cents each), rounded up. */
std::string IgpFlips(Chips RakeLeft)
{
	const Chips Per = std::max<Chips>(1, net::Site(net::SiteId::Ignite).Definition().Rake.Fee(110));
	const Chips N = (RakeLeft + Per - 1) / Per;
	return "about " + Grouped(N) + (N == 1 ? " Flip & Run" : " Flip & Runs");
}

/** The right-hand drawers {1140,88,436,774}: the wheel's odds, FUSE's odds, the Fine print. */
void IgpDrawer(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim)
{
	using namespace igniteart;
	Canvas& C = Access::C(RL);
	if (Scrim)
	{
		U.Drawer = 0;
		return;
	}
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	const float In = Preview ? 1.0f : IgpEase((Now - U.DrawerAt) / 0.25);
	C.FillRect({0.0f, 72.0f, 1600.0f, 890.0f}, Hex(IgNight, 0.45f * In));
	C.Save();
	C.Translate((1.0f - In) * 460.0f, 0.0f);
	const Rect R{1140.0f, 88.0f, 436.0f, 774.0f};
	C.GlowRoundRect({R.X - 6.0f, R.Y, R.W, R.H}, 24.0f, Rgba(0, 0, 0, 0.5f), 24.0f);
	C.FillRoundRect(R, 24.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(K.PanelTop), Hex(K.PanelBot)));
	IgniteNeonRoundRect(C, R, 24.0f, Hex(K.Coral, 0.6f), 1.5f, 5.0f);
	const float X = R.X + 24.0f;
	float Y = R.Y + 44.0f;
	if (U.Drawer == 1)
	{
		C.Text("THE WHEEL'S ODDS", X, Y, IgniteBi(22.0f, Hex(K.Cream)));
		Y += 26.0f;
		Y = rlnet_detail::NetParagraph(C, IgniteWheelOdds, X, Y, R.W - 48.0f, 12.5f, 600, pal::Muted, 17.0f, 2) + 14.0f;
		for (int Ring = 0; Ring < 3; ++Ring)
		{
			C.Text(IgniteRingName(Ring + 1), X, Y, IgniteBi(16.0f, Ring < L.RingsLit ? Hex(IgLime) : pal::Muted));
			Y += 8.0f;
			for (const IgniteWedge& W : L.Rings[static_cast<size_t>(Ring)])
			{
				Y += 22.0f;
				C.Text(W.Label, X + 12.0f, Y, IgniteTx(13.0f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 280.0f));
				C.Text(Fixed(W.P * 100.0, W.P < 0.01 ? 2 : 1) + "%", R.X + R.W - 24.0f, Y, IgniteTx(13.0f, 800, Hex(IgSun), Align::Right));
			}
			Y += 30.0f;
		}
	}
	else if (U.Drawer == 2)
	{
		C.Text("FUSE ODDS", X, Y, IgniteBi(22.0f, Hex(K.Cream)));
		Y += 26.0f;
		Y = rlnet_detail::NetParagraph(C, IgniteFuseOdds, X, Y, R.W - 48.0f, 12.5f, 600, pal::Muted, 17.0f, 3) + 20.0f;
		C.Text("PRIZE", X, Y, IgniteTx(10.0f, 900, pal::Muted));
		C.Text("$1.10", X + 210.0f, Y, IgniteTx(10.0f, 900, pal::Muted, Align::Right));
		C.Text("$3.30 \xC2\xB7 $5.50", R.X + R.W - 24.0f, Y, IgniteTx(10.0f, 900, pal::Muted, Align::Right));
		for (const fuse::Row& Row : fuse::Table())
		{
			if (Row.M <= 0)
			{
				continue;
			}
			// The top row is x1,000 at $1.10 and x250 above (MOf and POf read it by tier).
			Y += 40.0f;
			const int M0 = fuse::MOf(Row, 0);
			const int M1 = fuse::MOf(Row, 1);
			const double P0 = fuse::POf(Row, 0);
			const double P1 = fuse::POf(Row, 1);
			C.FillRoundRect({X - 8.0f, Y - 26.0f, R.W - 32.0f, 36.0f}, 10.0f, Hex(K.Well));
			const float Mw = IgpTimes(C, X, Y, 20.0f, M0, Hex(IgniteFuseTint(M0, K)));
			if (M1 != M0)
			{
				C.Text("/", X + Mw + 4.0f, Y, IgniteTx(14.0f, 800, pal::Muted));
				IgpTimes(C, X + Mw + 14.0f, Y, 14.0f, M1, Hex(IgniteFuseTint(M1, K)));
			}
			C.Text(P0 > 0.0 ? Fixed(P0 * 100.0, P0 < 0.001 ? 3 : 2) + "%" : std::string("\xE2\x80\x94"), X + 210.0f, Y, IgniteTx(13.0f, 800, Hex(K.Cream), Align::Right));
			C.Text(P1 > 0.0 ? Fixed(P1 * 100.0, P1 < 0.001 ? 3 : 2) + "%" : std::string("\xE2\x80\x94"), R.X + R.W - 24.0f, Y, IgniteTx(13.0f, 800, Hex(K.Cream), Align::Right));
		}
		Y += 40.0f;
		IgniteLineTimes(C, "House edge " + Fixed(fuse::Edge() * 100.0, 0) + "% \xC2\xB7 x25 and up pay 80 / 12 / 8", X, Y, IgniteTx(12.5f, 700, Hex(IgSun), Align::Left, Baseline::Alphabetic, R.W - 48.0f));
	}
	else if (U.Drawer == 4)
	{
		// The Crackers' odds (F5, W5; G2_AUDIT M2): core's own tables, the ones an open draws from (site::play::CrackerOdds):
		// the Sparks (exactly one), what may come on top, where each cracker drops and what it's worth on average.
		C.Text("CRACKERS' ODDS", X, Y, IgniteBi(22.0f, Hex(K.Cream)));
		Y += 26.0f;
		Y = rlnet_detail::NetParagraph(C, IgniteCrackerOddsLine, X, Y, R.W - 48.0f, 12.5f, 600, pal::Muted, 17.0f, 3) + 2.0f;
		const uint32_t Tints[3] = {0xf2e6d8, IgPink, IgSun};
		const auto Pct = [](double P) { return Fixed(P * 100.0, P > 0.0 && P < 0.01 ? 2 : 1) + "%"; };
		int64_t Owned = 0;
		for (int T = 0; T < 3; ++T)
		{
			const IgniteCrackerOdds Od = L.CrackerOdds[static_cast<size_t>(T)].Rows.empty() ? IgniteCrackerOddsFrom(site::play::CrackerOdds(T)) : L.CrackerOdds[static_cast<size_t>(T)];
			Owned = std::max(Owned, Od.DuplicateSparks);
			Y += 32.0f;
			IgniteCracker(C, X + 15.0f, Y - 6.0f, 32.0f, T, 0.0f);
			C.Text(Od.Name, X + 38.0f, Y, IgniteBi(16.0f, Hex(Tints[T])));
			const double Ev = Od.EvSparks;
			const std::string Worth = std::fabs(Ev - std::round(Ev)) < 0.05 ? Grouped(static_cast<int64_t>(std::llround(Ev))) : Fixed(Ev, 1);
			C.Text("WORTH " + Worth + " SPARKS ON AVERAGE", R.X + R.W - 24.0f, Y - 1.0f, IgniteTx(9.5f, 900, pal::Muted, Align::Right));
			// Where it drops (core's words, wrapped to two lines).
			Y = rlnet_detail::NetParagraph(C, Od.From, X + 38.0f, Y + 16.0f, R.W - 86.0f, 11.0f, 600, pal::Muted, 14.0f, 2) - 14.0f;
			// The Sparks: exactly one of them, two to a line.
			int Col = 0;
			for (const IgniteCrackerRow& Rw : Od.Rows)
			{
				if (Rw.Part != 0)
				{
					continue;
				}
				Y += Col == 0 ? 21.0f : 0.0f;
				const float Cx = X + static_cast<float>(Col) * 198.0f;
				C.Text(Rw.Label, Cx, Y, IgniteTx(12.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, 120.0f));
				// The right column's figures end where the ticket's and the looks' do (the drawer's right margin).
				C.Text(Pct(Rw.P), Col == 0 ? Cx + 180.0f : R.X + R.W - 24.0f, Y, IgniteTx(12.5f, 800, Hex(IgSun), Align::Right));
				Col ^= 1;
			}
			// A ticket on top.
			for (const IgniteCrackerRow& Rw : Od.Rows)
			{
				if (Rw.Part == 1)
				{
					Y += 21.0f;
					C.Text("+ " + Rw.Label, X, Y, IgniteTx(12.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, R.W - 120.0f));
					C.Text(Pct(Rw.P), R.X + R.W - 24.0f, Y, IgniteTx(12.5f, 800, Hex(IgSun), Align::Right));
				}
			}
			// A look piece on top: one of the six, each the same share.
			std::string Looks;
			int NLooks = 0;
			double Each = 0.0;
			for (const IgniteCrackerRow& Rw : Od.Rows)
			{
				if (Rw.Part == 2)
				{
					Looks += (NLooks > 0 ? " \xC2\xB7 " : "") + Rw.Label;
					Each = Rw.P;
					++NLooks;
				}
			}
			if (NLooks > 0)
			{
				Y += 21.0f;
				C.Text("+ a look piece (one of " + std::to_string(NLooks) + ", " + Pct(Each) + " each)", X, Y, IgniteTx(12.5f, 700, Hex(K.Cream), Align::Left, Baseline::Alphabetic, R.W - 120.0f));
				C.Text(Pct(Od.LookP), R.X + R.W - 24.0f, Y, IgniteTx(12.5f, 800, Hex(IgSun), Align::Right));
				Y = rlnet_detail::NetParagraph(C, Looks, X + 14.0f, Y + 16.0f, R.W - 62.0f, 10.5f, 600, pal::Muted, 14.0f, 3) - 14.0f;
			}
		}
		if (Owned > 0)
		{
			Y += 28.0f;
			rlnet_detail::NetParagraph(C, rlsites_detail::SitesFill(IgniteCrackerOwned, {{"N", Grouped(Owned)}}), X, Y, R.W - 48.0f, 12.0f, 600, Hex(K.Cream, 0.85f), 16.0f, 2);
		}
	}
	else
	{
		C.Text("FINE PRINT", X, Y, IgniteBi(22.0f, Hex(K.Cream)));
		Y += 30.0f;
		const std::string Back = !L.GovLine.empty() ? L.GovLine
										: "Ignite gave back " + Money(L.Promo30) + " of your " + Money(L.Rake30) + " rake in the last 30 days (" + Fixed(L.Share * 100.0, 0) + "%).";
		Y = rlnet_detail::NetParagraph(C, Back, X, Y, R.W - 64.0f, 16.0f, 800, Hex(IgLime), 21.0f, 3) + 10.0f;
		// The share against its ceiling.
		C.FillRoundRect({X, Y, R.W - 48.0f, 12.0f}, 6.0f, Hex(K.Well));
		C.FillRoundRect({X, Y, std::max(6.0f, (R.W - 48.0f) * static_cast<float>(Clamp01(L.Share))), 12.0f}, 6.0f, Hex(IgLime));
		const float Cx = X + (R.W - 48.0f) * static_cast<float>(Clamp01(L.Ceiling));
		C.FillRect({Cx - 1.0f, Y - 5.0f, 2.0f, 22.0f}, Hex(IgSun));
		C.Text(Fixed(L.Ceiling * 100.0, 0) + "% CEILING", Cx, Y + 32.0f, IgniteTx(9.5f, 900, Hex(IgSun), Align::Center));
		Y += 56.0f;
		Y = rlnet_detail::NetParagraph(C, IgniteFineRule, X, Y, R.W - 64.0f, 12.5f, 600, Hex(K.Cream, 0.85f), 17.0f, 3) + 4.0f;
		if (L.Welcome)
		{
			Y = rlnet_detail::NetParagraph(C, IgniteFineWelcome, X, Y, R.W - 64.0f, 12.5f, 600, Hex(K.Cream, 0.85f), 17.0f, 2) + 4.0f;
		}
		Y += 10.0f;
		if (L.WeekCap > 0)
		{
			C.Text("This week: " + Money(L.WeekFace) + " of " + Money(L.WeekCap), X, Y, IgniteTx(13.0f, 800, Hex(K.Cream)));
			Y += 20.0f;
		}
		if (L.MondayBank > 0)
		{
			C.Text("Monday bonus: +" + Grouped(L.MondayBank) + " Sparks banked", X, Y, IgniteTx(13.0f, 800, Hex(IgLime)));
			Y += 18.0f;
			Y = rlnet_detail::NetParagraph(C, IgniteMondayGood, X, Y + 4.0f, R.W - 64.0f, 11.5f, 600, pal::Muted, 15.0f, 2) + 2.0f;
		}
		Y += 14.0f;
		C.Text("THE GUEST LIST", X, Y, IgniteBi(16.0f, Hex(IgSun)));
		Y += 8.0f;
		if (L.Holds.empty())
		{
			Y += 22.0f;
			C.Text(IgniteNoHolds, X, Y, IgniteTx(13.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, R.W - 48.0f));
		}
		for (size_t I = 0; I < L.Holds.size() && I < 8; ++I)
		{
			const IgniteHold& H = L.Holds[I];
			Y += 44.0f;
			C.FillRoundRect({X - 8.0f, Y - 28.0f, R.W - 32.0f, 40.0f}, 10.0f, Hex(K.Well));
			const float Fr = H.Cents > 0 ? static_cast<float>(Clamp01(static_cast<double>(H.Covered) / static_cast<double>(H.Cents))) : 0.0f;
			C.FillRoundRect({X - 8.0f, Y + 8.0f, (R.W - 32.0f) * Fr, 4.0f}, 2.0f, Hex(IgLime));
			C.Text(H.What, X, Y - 9.0f, IgniteTx(13.0f, 800, Hex(K.Cream), Align::Left, Baseline::Alphabetic, R.W - 150.0f));
			const std::string About = H.Entries > 0 ? "about " + Grouped(H.Entries) + (H.Entries == 1 ? " Flip & Run" : " Flip & Runs") : IgpFlips(H.RakeLeft);
			C.Text(H.RakeLeft > 0 ? "clears after " + Money(H.RakeLeft) + " more rake (" + About + ")" : std::string("clears at the next 6 AM"), X, Y + 5.0f,
				IgniteTx(10.5f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, R.W - 64.0f));
			C.Text(H.SparksOwed ? Grouped(H.Sparks) + " Sparks" : Money(H.Cents), R.X + R.W - 32.0f, Y - 9.0f, IgniteTx(12.5f, 800, Hex(IgSun), Align::Right));
		}
		Y += 34.0f;
		Y = rlnet_detail::NetParagraph(C, IgniteGuestRule, X, Y, R.W - 64.0f, 11.5f, 600, pal::Muted, 15.0f, 2);
		for (size_t I = 0; I < L.Months.size() && I < 3 && Y < R.Y + R.H - 70.0f; ++I)
		{
			Y += 20.0f;
			C.Text(L.Months[I].first + ": " + Money(L.Months[I].second) + " back", X, Y, IgniteTx(12.0f, 700, Hex(K.Cream)));
		}
		C.Text(IgniteStaysHere, X, R.Y + R.H - 70.0f, IgniteTx(11.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, R.W - 48.0f));
	}
	if (IgpButton(RL, "drawerx", {R.X + R.W - 150.0f, R.Y + R.H - 56.0f, 126.0f, 40.0f}, "Close", ButtonKind::Ghost, 15.0f, !Preview))
	{
		U.Drawer = 0;
	}
	C.Restore();
}
} // namespace

void IgniteOverlayDraw(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview, bool Scrim)
{
	switch (IgniteOverlay(RL, U, Now))
	{
	case 1: IgpReveal(RL, U, K, Now, Preview, Scrim); break;
	case 2: IgpSpin(RL, U, K, Now, Preview, Scrim); break;
	case 3: IgpCrack(RL, U, K, Now, Preview, Scrim); break;
	case 4: IgpBuy(RL, U, K, Now, Preview, Scrim); break;
	case 5: IgpDrawer(RL, U, K, Now, Preview, Scrim); break;
	default: break;
	}
}

// ------------------------------------------------------------------ the level-up banner (SPEC §2.4 H)

bool IgniteBannerUp(const IgniteUi& U, double Now)
{
	return U.LevelShown > 0 && Now >= U.LevelAt && Now - U.LevelAt < 3.5;
}

void IgniteBanner(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	if (Preview || !IgniteBannerUp(U, Now))
	{
		return;
	}
	Canvas& C = Access::C(RL);
	const double T = Now - U.LevelAt;
	const float Drop = IgpBack(T / 0.35);
	const float Fade = static_cast<float>(Clamp01((3.5 - T) / 0.3));
	const Rect R = IgniteBannerRect;
	const Ui::ClickState St = RL.UI.Clickable("levelx", R, true);
	const float A0 = C.GetAlpha();
	C.SetAlpha(A0 * Fade);
	C.Save();
	C.Translate(0.0f, (Drop - 1.0f) * 160.0f);
	C.GlowRoundRect({R.X, R.Y + 8.0f, R.W, R.H}, 28.0f, Rgba(0, 0, 0, 0.5f), 20.0f);
	C.FillRoundRect(R, 28.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, Hex(0x3a1f5c), Hex(0x1d1030)));
	IgniteNeonRoundRect(C, R, 28.0f, Hex(IgViolet), 2.0f, 6.0f);
	IgniteLevelBadge(C, R.X + 80.0f, R.Y + 75.0f, 46.0f, U.LevelShown);
	C.Text("LEVEL " + std::to_string(U.LevelShown), R.X + 150.0f, R.Y + 66.0f, IgniteBi(40.0f, Hex(IgCream)));
	const int Lv = U.LevelShown;
	const char* Title = Lv >= 50 ? "Icon" : Lv >= 40 ? "Legend" : Lv >= 30 ? "Star" : Lv >= 20 ? "Hotshot" : Lv >= 10 ? "Local" : "Rookie";
	const bool NewTitle = Lv % 10 == 0 && Lv >= 10;
	const int Box = Lv % 10 == 0 ? 2 : Lv % 5 == 0 ? 1 : 0;
	C.Text(NewTitle ? std::string("You're a ") + Title + " now" : std::string("A ") + IgpCrackerNames[Box] + " Cracker drops", R.X + 152.0f, R.Y + 102.0f, IgniteTx(16.0f, 700, NewTitle ? Hex(IgSun) : Hex(K.Cream)));
	IgniteCracker(C, R.X + R.W - 86.0f, R.Y + 75.0f, 110.0f, Box, 0.0f);
	C.Restore();
	C.SetAlpha(A0);
	if (St.Clicked)
	{
		U.LevelAt = Now - 3.5;
	}
}

// ------------------------------------------------------------------ the 3-max FUSE table (SPEC §2.5)

void IgniteFuseTable(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	if (!S.T || !fuse::IsFuse(S.T->Spec.Id))
	{
		return;
	}
	// The front table's own spec (FuseReveal(-1)); its multiplier is GTD / buy-in (fuse::MultiplierOf).
	const fuse::Reveal Rv = S.FuseReveal(-1);
	const int M = std::max(2, Rv.M > 0 ? Rv.M : fuse::MultiplierOf(S.T->Spec));
	const Chips Pool = static_cast<Chips>(M) * IgpFuseBuyIn(Rv.Tier);
	const double Tm = Preview ? 1000.0 : Now;
	// The prize marquee {410,88,360,82}: bulbs chasing round it (faster from x25).
	{
		const Rect R{410.0f, 88.0f, 360.0f, 82.0f};
		C.FillRoundRect(R, 22.0f, Hex(IgNight, 0.94f));
		IgniteNeonRoundRect(C, R, 22.0f, Hex(M >= 25 ? IgSun : K.Coral, 0.9f), 2.0f, 6.0f);
		const double Bpm = K.Bpm * (M >= 25 ? 2.0 : 1.0);
		for (int I = 0; I < 20; ++I)
		{
			const float X = R.X + 18.0f + static_cast<float>(I) * 17.0f;
			const float Lit = static_cast<float>(motion::Chase(Tm, I, 20, Bpm));
			IgniteBulb(C, X, R.Y + 6.0f, 2.4f, Hex(IgSun), Hex(IgPink), Lit);
			IgniteBulb(C, X, R.Y + R.H - 6.0f, 2.4f, Hex(IgSun), Hex(IgPink), static_cast<float>(motion::Chase(Tm, 19 - I, 20, Bpm)));
		}
		C.Text("PRIZE", R.X + 26.0f, R.Y + 36.0f, IgniteTx(12.0f, 900, Hex(K.Cream, 0.8f)));
		C.Text(Money(Pool), R.X + 26.0f, R.Y + 66.0f, IgniteBi(30.0f, Hex(IgSun)));
		const uint32_t Tint = IgniteFuseTint(M, K);
		const Rect Chip{R.X + R.W - 112.0f, R.Y + 24.0f, 92.0f, 36.0f};
		IgnitePill(C, Chip, Hex(IgNight), Hex(Tint), 2.0f);
		IgpTimes(C, Chip.X + Chip.W / 2.0f, Chip.Y + 26.0f, 20.0f, M, Hex(Tint), Align::Center);
	}
	// The fuse is the blind clock {300,210,580,40}: the spark at the level's progress; "BLINDS UP!" slams on each level.
	{
		const double LevelSecs = std::max(1.0, S.T->Spec.LevelMinutes * 60.0);
		const double Burn = Clamp01(1.0 - S.T->LevelSecondsLeft() / LevelSecs);
		const int Head = IgniteFuseHead(300.0f, 880.0f, Burn);
		C.Cached((static_cast<uint64_t>(Fnv1a("ignite:fuse-clock")) << 8) ^ static_cast<uint64_t>(Head), 0.0f, 0.0f, [Burn](Canvas& At) {
			IgniteFuseRope(At, 300.0f, 880.0f, 230.0f, 5.0f, Burn, Hex(0xc9a46a), Hex(0x5a4060), 0.0, false);
		});
		IgniteFuseSpark(C, 300.0f, 880.0f, 230.0f, 5.0f, Burn, Tm);
		const Level& Lv = S.T->CurrentLevel();
		C.Text("LEVEL " + std::to_string(S.T->LevelIndex + 1) + " \xC2\xB7 " + Grouped(Lv.Sb) + "/" + Grouped(Lv.Bb), 300.0f, 258.0f, IgniteTx(11.0f, 900, Hex(K.Cream, 0.75f)));
		if (U.BlindsTable != S.T->Spec.Id)
		{
			U.BlindsTable = S.T->Spec.Id;
			U.BlindsLevel = S.T->LevelIndex;
			U.BlindsAt = -100.0;
		}
		else if (S.T->LevelIndex != U.BlindsLevel)
		{
			U.BlindsLevel = S.T->LevelIndex;
			U.BlindsAt = Now;
		}
		const double Since = Now - U.BlindsAt;
		if (Since >= 0.0 && Since < 1.6 && !Preview)
		{
			IgniteCue(S, U, "blinds:" + S.T->Spec.Id + ":" + std::to_string(U.BlindsLevel), SoundId::IgPop, 0.45f, Preview);
			const float Pop = static_cast<float>(motion::Spring(Now, U.BlindsAt));
			const float A0 = C.GetAlpha();
			C.SetAlpha(A0 * static_cast<float>(Clamp01((1.6 - Since) / 0.3)));
			IgniteSticker(C, 880.0f, 230.0f, 54.0f, "BLINDS UP!\n" + Grouped(Lv.Sb) + "/" + Grouped(Lv.Bb), Hex(IgPink), Hex(IgDarkInk), 13.0f, 16, 0.8f, Pop);
			C.SetAlpha(A0);
		}
	}
}

// ------------------------------------------------------------------ the Player Card's Locker and Sticker Book (SPEC §2.4 L)

void IgniteLocker(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, const Rect& Box, double Now, bool Preview)
{
	using namespace igniteart;
	Session& S = Access::S(RL);
	Canvas& C = Access::C(RL);
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	const char* const Slots[5] = {"Card backs", "Felts", "Chip sets", "Frames", "Win effects"};
	float Tx = Box.X + 24.0f;
	for (int I = 0; I < 5; ++I)
	{
		const float W = C.Measure(Slots[I], 13.5f, 800) + 28.0f;
		const Rect R{Tx, Box.Y + 20.0f, W, 32.0f};
		const Ui::ClickState St = RL.UI.Clickable("locker" + std::to_string(I), R, !Preview);
		if (St.Clicked)
		{
			U.LockerSlot = I;
		}
		const bool On = U.LockerSlot == I;
		IgnitePill(C, R, On ? Hex(K.Sky) : Hex(St.Hover ? K.Panel : K.Well), Hex(K.Rim), On ? 0.0f : 1.0f);
		C.Text(Slots[I], R.X + W / 2.0f, R.Y + 21.0f, IgniteTx(13.5f, 800, On ? Hex(IgDarkInk) : Hex(K.Cream), Align::Center));
		Tx += W + 8.0f;
	}
	C.Text(IgniteLockerEmpty, Box.X + Box.W - 24.0f, Box.Y + 42.0f, IgniteTx(12.0f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, 330.0f));
	int N = 0;
	for (const IgniteLook& Lk : L.Locker)
	{
		if (Lk.Slot != U.LockerSlot)
		{
			continue;
		}
		const float X = Box.X + 24.0f + static_cast<float>(N % 4) * 244.0f;
		const float Y = Box.Y + 72.0f + static_cast<float>(N / 4) * 200.0f;
		const Rect R{X, Y, 228.0f, 186.0f};
		C.FillRoundRect(R, 18.0f, Hex(Lk.Worn ? 0x2a1640 : K.Well));
		C.StrokeRoundRect(R, 18.0f, Lk.Worn ? Hex(IgLime) : Hex(K.Rim), Lk.Worn ? 2.0f : 1.0f);
		// The look's swatch: its two colours by name's hash (the base designs, F6), a flame mark on top.
		// The preview draws what the table would wear (ui-tablestyle's look kit: the back, the felt, the chip set); a frame and
		// a win effect show their two colours (a Wristband month's theme, core's) under the flame. Locked looks are dimmed.
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * (Lk.Owned ? 1.0f : 0.4f));
		if (Lk.Slot == 0)
		{
			C.FillRoundRect({X + 18.0f, Y + 16.0f, R.W - 36.0f, 84.0f}, 14.0f, Hex(IgNight));
			CardOpts Back;
			Back.Back = rlsites_detail::SitesIgBack(Lk.Look);
			DrawCard(C, -1, X + R.W / 2.0f - 50.0f, Y + 20.0f, 52.0f, 76.0f, Back);
			Back.Rotate = 0.12f;
			DrawCard(C, -1, X + R.W / 2.0f - 4.0f, Y + 20.0f, 52.0f, 76.0f, Back);
		}
		else if (Lk.Slot == 1)
		{
			uint32_t Felt = 0, Light = 0, Dark = 0;
			rlsites_detail::SitesIgFelt(Lk.Look, Felt, Light, Dark);
			C.FillRoundRect({X + 18.0f, Y + 16.0f, R.W - 36.0f, 84.0f}, 42.0f, Paint::Linear({X, Y + 16.0f}, {X, Y + 100.0f}, Hex(Light), Hex(Dark)));
			C.FillEllipse(X + R.W / 2.0f, Y + 58.0f, 60.0f, 26.0f, Hex(Felt, 0.6f));
		}
		else if (Lk.Slot == 2)
		{
			C.FillRoundRect({X + 18.0f, Y + 16.0f, R.W - 36.0f, 84.0f}, 14.0f, Hex(IgNight));
			DrawChipStack(C, 1625, X + R.W / 2.0f - 30.0f, Y + 84.0f, 1.3f, 1.0f, rlsites_detail::SitesIgChipSet(Lk.Look));
			DrawChipStack(C, 525, X + R.W / 2.0f + 34.0f, Y + 84.0f, 1.3f, 1.0f, rlsites_detail::SitesIgChipSet(Lk.Look));
		}
		else
		{
			const uint32_t A = Lk.ColorA != 0 ? Lk.ColorA : (IgniteMix(Fnv1a(Lk.Name)) & 0x7f7f7fu) | 0x404040u;
			const uint32_t B = Lk.ColorB != 0 ? Lk.ColorB : IgViolet;
			C.FillRoundRect({X + 18.0f, Y + 16.0f, R.W - 36.0f, 84.0f}, 14.0f, Paint::Linear({X, Y}, {X + R.W, Y + 100.0f}, Hex(A), Hex(B)));
			IgniteFlame(C, X + R.W / 2.0f, Y + 86.0f, 34.0f, Hex(IgCream, 0.9f), Hex(IgSun, 0.9f), Hex(IgCream));
		}
		C.SetAlpha(A0);
		C.Text(Lk.Name, X + 18.0f, Y + 124.0f, IgniteTx(14.5f, 800, Lk.Owned ? Hex(K.Cream) : pal::Muted, Align::Left, Baseline::Alphabetic, R.W - 36.0f));
		if (Lk.Worn)
		{
			C.Text("WEARING", X + 18.0f, Y + 160.0f, IgniteTx(11.5f, 900, Hex(IgLime)));
		}
		else if (Lk.Owned)
		{
			const std::string Id = "equip" + std::to_string(Lk.Slot) + "_" + std::to_string(Lk.Look);
			if (IgpButton(RL, Id, {X + 18.0f, Y + 138.0f, 110.0f, 36.0f}, "Wear", ButtonKind::Primary, 14.0f, !Preview))
			{
				const std::string Why = S.Equip(Lk.Slot, Lk.Look);
				U.LoopsAt = -1.0;
				if (!Why.empty())
				{
					IgniteNoteRefusal(U, Id, Why, Now);
				}
			}
		}
		else
		{
			C.Text(Lk.How, X + 18.0f, Y + 160.0f, IgniteTx(11.5f, 700, pal::Muted, Align::Left, Baseline::Alphabetic, R.W - 36.0f));
		}
		++N;
	}
}

void IgniteStickerBook(RiverLine& RL, IgniteUi& U, const igniteart::IgniteTones& K, const Rect& Box, double Now, bool Preview)
{
	using namespace igniteart;
	(void)U;
	(void)Preview;
	Canvas& C = Access::C(RL);
	const IgniteLoops& L = IgniteLoopsOf(RL, Now);
	int Got = 0;
	for (const IgniteStickerRow& St : L.Book)
	{
		Got += St.Got ? 1 : 0;
	}
	C.Text("STICKER BOOK", Box.X + 24.0f, Box.Y + 40.0f, IgniteBi(20.0f, Hex(K.Cream)));
	C.Text(std::to_string(Got) + " of " + std::to_string(L.Book.size()), Box.X + Box.W - 24.0f, Box.Y + 38.0f, IgniteTx(13.0f, 700, pal::Muted, Align::Right));
	const uint32_t Fills[5] = {IgPink, K.Sky, IgSun, IgLime, IgNightAccent};
	for (size_t I = 0; I < L.Book.size() && I < 30; ++I)
	{
		const IgniteStickerRow& St = L.Book[I];
		const float Cx = Box.X + 84.0f + static_cast<float>(I % 6) * 160.0f;
		const float Cy = Box.Y + 112.0f + static_cast<float>(I / 6) * 112.0f;
		if (St.Got)
		{
			IgniteSticker(C, Cx, Cy, 34.0f, "", Hex(Fills[I % 5]), Hex(IgDarkInk), 10.0f, 16, 0.8f);
			IgniteSpark(C, Cx, Cy, 12.0f, Hex(IgDarkInk));
		}
		else
		{
			C.StrokePolyline(IgniteStar(Cx, Cy, 34.0f, 27.0f, 16), true, Hex(K.Rim), 1.5f);
			C.Text("?", Cx, Cy + 1.0f, IgniteBi(22.0f, pal::Muted, Align::Center, Baseline::Middle));
		}
		const std::string Name = St.Secret && !St.Got ? std::string("Secret") : St.Name;
		C.Text(Name, Cx, Cy + 52.0f, IgniteTx(11.5f, 800, St.Got ? Hex(K.Cream) : pal::Muted, Align::Center, Baseline::Alphabetic, 150.0f));
		if (!St.Got)
		{
			C.Text(St.Secret ? std::string(IgniteSecretSticker) : St.How, Cx, Cy + 66.0f, IgniteTx(9.5f, 600, pal::Muted, Align::Center, Baseline::Alphabetic, 150.0f));
		}
	}
}
} // namespace rlignite_detail
} // namespace ui
} // namespace ss
