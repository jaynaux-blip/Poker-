// Update 3 (SPEC §3.4, §3.6, §7.2): RunItTwice's League page, League Night, the Callout drawer, the Watchlist, the
// Statement, the Luck Ledger panel and credentials, ui-rit's file. This file has no RiverLine member function of its own:
// it reaches the client's private state and its gatekeepers through rlrit_detail::Access (RiverLine.h's friend),
// forwarders defined here and declared in RitArt.h, where RiverLineRit.cpp's pages reach them too.
//
// G2 (§3.9): every surface draws RitPlay (RitArt.h), the loops' model RitPlayOf fills from core's View: the League's table,
// zones and ranks, the Callout, Team Scalps, the Statement's share, the Luck Ledger's buckets and the credentials are
// core's numbers, drawn here. The UI keeps only what this laptop has seen (League Night played, a Briefing dismissed, a bell
// rung), never an outcome. Every overlay (League Night, the Callout drawer) holds the pointer for what draws under it
// (house law 3, SitesBlockUnder in RitFrame) and grants nothing while the Kast preview draws (Previewing).
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"
#include "RitArt.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/ChartArt.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace rlrit_detail
{
Session& Access::S(RiverLine& RL)
{
	return RL.S;
}

Canvas& Access::C(RiverLine& RL)
{
	return *RL.C;
}

Ui& Access::UI(RiverLine& RL)
{
	return RL.UI;
}

double Access::World(const RiverLine& RL)
{
	return RL.World;
}

double Access::Dt(const RiverLine& RL)
{
	return RL.Dt;
}

bool Access::Previewing(const RiverLine& RL)
{
	return RL.Previewing;
}

double Access::PageAt(const RiverLine& RL)
{
	return RL.PageAt;
}

void Access::SetPage(RiverLine& RL, RiverLine::Page P)
{
	// The page a frame settles on (a page RunItTwice hasn't got goes to its Board): no OpenPage side effects.
	RL.PageShown = P;
}

double Access::EventAt(const RiverLine& RL)
{
	return RL.EventAt;
}

std::vector<std::string>& Access::Listed(RiverLine& RL)
{
	return RL.Listed;
}

RiverLine::Filter Access::FilterShown(const RiverLine& RL)
{
	return RL.FilterShown;
}

double Access::FilterAt(const RiverLine& RL)
{
	return RL.FilterAt;
}

bool Access::TakeScrollToSelected(RiverLine& RL)
{
	const bool Was = RL.ScrollToSelected;
	RL.ScrollToSelected = false;
	return Was;
}

float& Access::ListScroll(RiverLine& RL)
{
	return RL.ListScroll;
}

float& Access::ListScrollGoal(RiverLine& RL)
{
	return RL.ListScrollGoal;
}

net::Board Access::BoardShown(const RiverLine& RL)
{
	return RL.BoardShown;
}

double Access::BoardAt(const RiverLine& RL)
{
	return RL.BoardAt;
}

double Access::NewsSeenAt(const RiverLine& RL)
{
	return RL.NewsSeenAt;
}

std::vector<RitRow> Access::ScheduleRows(const RiverLine& RL)
{
	// The shared schedule (the gatekeeper: the filter, Joinable and its lock) as RiverLine's lobby builds it.
	std::vector<RitRow> Out;
	for (const RiverLine::ListRow& Row : RL.ScheduleRows())
	{
		RitRow R;
		R.E = Row.E;
		R.L = Row.L;
		R.Joinable = Row.Joinable;
		R.Lock = Row.Lock;
		Out.push_back(std::move(R));
	}
	return Out;
}

void Access::RegisterBlock(RiverLine& RL, const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now)
{
	RL.RegisterBlock(E, L, R, Now);
}

const std::vector<net::NewsItem>& Access::NewsFeed(RiverLine& RL)
{
	return RL.NewsFeed();
}

void Access::PlayerCard(RiverLine& RL, double Now)
{
	RL.PlayerCard(Now);
}

void Access::FinishedToasts(RiverLine& RL, double Now)
{
	RL.FinishedToasts(Now);
}

// ------------------------------------------------------------------ the League's shared bits

/** "NOVEMBER" (or "NOV") for a MonthKey (Year * 12 + Month - 1). */
std::string RitMonthName(int Key, bool Short)
{
	static const char* const Names[12] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
	const std::string N = Names[((Key % 12) + 12) % 12];
	return Short ? N.substr(0, 3) : N;
}

/** A multiplier as the copy writes it: "1.5", "1.25", "2". */
std::string RitMult(float M)
{
	std::string S = Fixed(M, 2);
	while (!S.empty() && S.back() == '0')
	{
		S.pop_back();
	}
	if (!S.empty() && S.back() == '.')
	{
		S.pop_back();
	}
	return S;
}

/** "November" for a MonthKey. */
std::string RitMonthTitle(int Key)
{
	std::string M = RitMonthName(Key, false);
	for (size_t I = 1; I < M.size(); ++I)
	{
		M[I] = M[I] >= 'A' && M[I] <= 'Z' ? static_cast<char>(M[I] - 'A' + 'a') : M[I];
	}
	return M;
}

/** "STEEL G4" (the group's short form on the Prospectus, the tape and the Entrants tab). */
std::string RitGroupTag(const RitPlay& P)
{
	return RitUpper(RitDivName(P.Div)) + (P.Group > 0 ? " G" + std::to_string(P.Group) : std::string());
}

/** A division's status-metal pip (Open's an outline). */
void RitPip(Canvas& C, float X, float Y, float S, int Div)
{
	if (Div < 0)
	{
		C.StrokePolyline({{X + 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + S - 0.5f}, {X + 0.5f, Y + S - 0.5f}}, true, RitC(RitDim), 1.0f);
		return;
	}
	if (Div == 0)
	{
		C.StrokePolyline({{X + 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + S - 0.5f}, {X + 0.5f, Y + S - 0.5f}}, true, RitC(RitChrome), 1.2f);
		return;
	}
	C.FillRect({X, Y, S, S}, RitC(RitDivMetal(Div)));
	if (Div == 1)
	{
		C.StrokePolyline({{X + 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + 0.5f}, {X + S - 0.5f, Y + S - 0.5f}, {X + 0.5f, Y + S - 0.5f}}, true, RitC(RitChromeDark), 1.0f);
	}
}

/** The last 5 days' form: a filled square a final table, half a cash, an outline a blank, a dot no entry. */
void RitFormMarks(Canvas& C, float X, float Cy, const std::array<int8_t, 5>& Form)
{
	for (int I = 0; I < 5; ++I)
	{
		const float Sx = X + static_cast<float>(I) * 15.0f;
		const Rect Sq{Sx, Cy - 5.5f, 11.0f, 11.0f};
		switch (Form[static_cast<size_t>(I)])
		{
		case 2: C.FillRect(Sq, RitC(RitChrome)); break;
		case 1:
			C.FillRect({Sq.X, Sq.Y + Sq.H / 2.0f, Sq.W, Sq.H / 2.0f}, RitC(RitChrome));
			C.StrokePolyline({{Sq.X + 0.5f, Sq.Y + 0.5f}, {Sq.X + Sq.W - 0.5f, Sq.Y + 0.5f}, {Sq.X + Sq.W - 0.5f, Sq.Y + Sq.H - 0.5f}, {Sq.X + 0.5f, Sq.Y + Sq.H - 0.5f}}, true,
				RitC(RitChrome), 1.0f);
			break;
		case 0:
			C.StrokePolyline({{Sq.X + 0.5f, Sq.Y + 0.5f}, {Sq.X + Sq.W - 0.5f, Sq.Y + 0.5f}, {Sq.X + Sq.W - 0.5f, Sq.Y + Sq.H - 0.5f}, {Sq.X + 0.5f, Sq.Y + Sq.H - 0.5f}}, true,
				RitC(RitChromeDark), 1.0f);
			break;
		default: C.FillCircle(Sx + 5.5f, Cy, 1.5f, RitC(RitDim)); break;
		}
	}
}

/** The best-8 meter: the 8 counting days as cells, filled to their share of the group's best day (an empty slot an outline). */
void RitBestCells(Canvas& C, float X, float Cy, const std::array<float, 8>& Best, float Top, bool You)
{
	for (int I = 0; I < 8; ++I)
	{
		const float V = Best[static_cast<size_t>(I)];
		const float Cx = X + static_cast<float>(I) * 13.0f;
		const Rect Cell{Cx, Cy - 9.0f, 10.0f, 18.0f};
		if (V <= 0.0f)
		{
			C.StrokePolyline({{Cell.X + 0.5f, Cell.Y + 0.5f}, {Cell.X + Cell.W - 0.5f, Cell.Y + 0.5f}, {Cell.X + Cell.W - 0.5f, Cell.Y + Cell.H - 0.5f}, {Cell.X + 0.5f, Cell.Y + Cell.H - 0.5f}},
				true, RitC(RitHair), 1.0f);
			continue;
		}
		const float H = std::max(3.0f, Cell.H * std::min(1.0f, V / std::max(1.0f, Top)));
		C.FillRect({Cell.X, Cell.Y, Cell.W, Cell.H}, RitC(RitOnyx));
		C.FillRect({Cell.X, Cell.Y + Cell.H - H, Cell.W, H}, RitC(You ? RitWhite : RitChrome));
	}
}

/**
 * Whether anyone goes down from the player's group this month: core's close (SitePlayRit.cpp) relegates only above Open,
 * never in a Placement month, and only when the group holds more than its promotion and relegation places together.
 */
bool RitRelegates(const RitPlay& P)
{
	return P.Down > 0 && !P.Placement && P.Div > 0 && (P.Of <= 0 || P.Of > P.Up + P.Down);
}

/** High Bridge, where core's close moves nobody up: its top places win a Sunday 100K ticket instead (§3.6; review-ui-rit). */
bool RitTopDivision(const RitPlay& P)
{
	return P.Div >= RitDivisions - 1;
}

// ------------------------------------------------------------------ THE LEAGUE (Page::League, §3.4 C)

constexpr float RitLgX = 232.0f;
constexpr float RitLgW = 940.0f;
constexpr float RitLgRows = 210.0f;
constexpr float RitLgPitch = 34.0f;

/** One row of the group (rank order): its ground by zone, your row ruled in Hot White over the red underglow. */
void RitLeagueRowDraw(RitCtx& X, const RitLeagueRow& R, float Y, int Index, float Top, bool Night)
{
	const RitPlay& P = X.P;
	const bool You = R.Npc < 0;
	const Rect Row{RitLgX, Y, RitLgW, RitLgPitch};
	// A tinted row drops the zebra (over Panel2, Signal Down's move figures would fall to 4.18:1).
	const uint32_t Ground = R.Zone > 0 ? RitPromo : R.Zone < 0 ? 0x321b1e : Index % 2 == 0 ? RitCarbon : RitPanel2;
	if (You)
	{
		RitGlow(X.C, {Row.X, Row.Y + Row.H - 6.0f, Row.W, 8.0f}, RitRed, 0.30f, 8.0f);
	}
	X.C.FillRect(Row, RitC(Ground));
	if (You)
	{
		X.C.FillRect({Row.X, Row.Y, Row.W, 1.0f}, RitC(RitWhite));
		X.C.FillRect({Row.X, Row.Y + Row.H - 1.0f, Row.W, 1.0f}, RitC(RitWhite));
	}
	const float Mid = Y + RitLgPitch / 2.0f;
	const float Fig = Mid + 15.0f * 0.36f;
	const Color Ink = RitC(You ? RitWhite : RitInk);
	RitText(X.C, std::to_string(R.Rank), RitLgX + 38.0f, Mid + 18.0f * 0.36f, 18.0f, Ink, RitFace::Cond, Align::Right);
	if (R.Move != 0 && !Night)
	{
		const bool Up = R.Move > 0;
		RitTri(X.C, RitLgX + 54.0f, Mid, 8.0f, Up, RitC(Up ? RitUp : RitDown));
		RitText(X.C, std::to_string(std::abs(R.Move)), RitLgX + 62.0f, Mid, 13.0f, RitC(Up ? RitUp : RitDown), RitFace::Cond, Align::Left, Baseline::Middle);
	}
	else
	{
		X.C.FillRect({RitLgX + 50.0f, Mid - 0.75f, 8.0f, 1.5f}, RitC(RitDim));
	}
	const std::string Nm = You ? X.S.HeroName : R.Name;
	RitSquareAvatar(X.C, RitLgX + 84.0f, Y + 5.0f, 24.0f, Nm, RitC(RitChrome, 0.0f));
	const float Nw = RitText(X.C, Nm, RitLgX + 116.0f, Mid + 14.0f * 0.36f, 14.0f, Ink, RitFace::Name, Align::Left, Baseline::Alphabetic, 176.0f);
	float Tx = RitLgX + 122.0f + Nw;
	if (R.Team)
	{
		Tx += RitTag(X.C, Tx, Mid - 8.0f, "TEAM", RitRed, RitWhite, 11.0f) + 6.0f;
	}
	if (!R.Country.empty())
	{
		RitText(X.C, RitUpper(R.Country), RitLgX + 352.0f, Mid + 11.0f * 0.36f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	}
	(void)Tx;
	for (int I = 0; I < 3; ++I)
	{
		RitPip(X.C, RitLgX + 372.0f + static_cast<float>(I) * 14.0f, Mid - 5.0f, 10.0f, R.Path[static_cast<size_t>(I)]);
	}
	RitText(X.C, R.Days > 0 ? std::to_string(R.Days) : std::string("\xE2\x80\x94"), RitLgX + 468.0f, Fig, 14.0f, RitC(R.Days > 0 ? (You ? RitWhite : RitInk) : RitMuted), RitFace::Mono, Align::Right);
	RitBestCells(X.C, RitLgX + 486.0f, Mid, R.Best, Top, You);
	RitText(X.C, RitPts(R.Points), RitLgX + 680.0f, Fig, 15.0f, Ink, RitFace::Mono, Align::Right);
	RitFormMarks(X.C, RitLgX + 700.0f, Mid, R.Form);
	// The actions: CALL a member (once a Matchweek, core's CanCall), VS your current Callout (amber, the VS tab's colour).
	if (R.Callout)
	{
		RitTag(X.C, RitLgX + 812.0f, Mid - 9.0f, "VS", RitAmber, RitOnyx, 12.0f, 9.0f, 18.0f);
	}
	else if (R.CanCall && !Night)
	{
		const Rect Call{RitLgX + 812.0f, Mid - 11.0f, 64.0f, 22.0f};
		const Ui::ClickState St = RitClick(X, "call" + std::to_string(R.Npc), Call);
		RitCutFill(X.C, Call, 6.0f, RitC(St.Hover ? RitPanel2 : RitCarbon));
		RitCutStroke(X.C, Call, 6.0f, RitC(St.Hover ? RitWhite : RitChrome, St.Hover ? 1.0f : 0.6f), 1.0f);
		RitText(X.C, "CALL", Call.X + Call.W / 2.0f, Call.Y + 11.0f, 12.0f, RitC(RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
		if (St.Clicked)
		{
			X.U.CallNpc = R.Npc;
			X.U.CallAt = X.Now;
			X.U.CallSaid.clear();
			RitSound(X, SoundId::RitBlip, 0.3f);
		}
	}
	(void)P;
}

/** The division tabs (ldiv0..4): browse any division; the player's own marked; the active one a Signal Red slab. */
void RitLeagueTabs(RitCtx& X, int Shown)
{
	const float Tw = (RitLgW - 4.0f * 8.0f) / 5.0f;
	for (int D = 0; D < RitDivisions; ++D)
	{
		const Rect R{RitLgX + static_cast<float>(D) * (Tw + 8.0f), 96.0f, Tw, 32.0f};
		const Ui::ClickState St = RitClick(X, "ldiv" + std::to_string(D), R);
		const bool On = D == Shown;
		if (On)
		{
			RitSlab(X.C, R, 6.0f, false, X.Now);
		}
		else
		{
			RitCutFill(X.C, R, 6.0f, RitC(St.Hover ? RitPanel2 : RitCarbon));
			RitCutStroke(X.C, R, 6.0f, RitC(RitHair), 1.0f);
		}
		RitCrest(X.C, R.X + 20.0f, R.Y + 17.0f, 20.0f, D);
		RitText(X.C, RitUpper(RitDivName(D)), R.X + 38.0f, R.Y + 16.0f, 14.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Left, Baseline::Middle);
		if (D == X.P.Div && (X.P.InLeague || X.P.Started))
		{
			RitTag(X.C, R.X + R.W - 44.0f, R.Y + 8.0f, "YOU", On ? RitWhite : RitChrome, On ? RitWhite : RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, true);
		}
		if (St.Clicked)
		{
			X.U.LeagueTab = D == X.P.Div ? -1 : D;
			RitSound(X, SoundId::RitBlip, 0.25f);
		}
	}
}

/** A division the player isn't in: what it is (its floor, its fixtures, what promotion into it pays, its members). */
void RitDivisionCard(RitCtx& X, int D)
{
	const RitDivision& Dv = X.P.Divisions[static_cast<size_t>(D)];
	const Rect R{RitLgX, 176.0f, RitLgW, 734.0f};
	RitPanel(X.C, R, 10.0f);
	RitCrest(X.C, R.X + 110.0f, R.Y + 150.0f, 140.0f, D);
	RitTwice(X.C, RitUpper(RitDivName(D)), R.X + 220.0f, R.Y + 120.0f, 56.0f, RitC(RitWhite));
	RitText(X.C, Dv.Members > 0 ? Grouped(Dv.Members) + (Dv.Members == 1 ? " member" : " members") + " in this division" : std::string(RitDivNoCount), R.X + 220.0f, R.Y + 156.0f, 15.0f,
		RitC(RitInk), RitFace::Body);
	float Y = R.Y + 230.0f;
	const auto Term = [&](const std::string& K, const std::string& V) {
		RitTerm(X, R.X + 220.0f, R.X + R.W - 40.0f, Y, K, V, RitInk);
		Y += 30.0f;
	};
	Term("SCORES FROM", Dv.Floor > 0 ? RitBuyIn(Dv.Floor) + " buy-ins" : std::string("any buy-in"));
	std::string Fixtures;
	for (const std::string& F : Dv.Matchdays)
	{
		Fixtures += (Fixtures.empty() ? "" : ", ") + F;
	}
	Term("MATCHDAYS", Fixtures.empty() ? std::string("\xE2\x80\x94") : Fixtures);
	Term("PROMOTION INTO IT", Dv.Reward.empty() ? std::string("\xE2\x80\x94") : Dv.Reward);
	RitParagraph(X.C, RitLeagueOther, R.X + 220.0f, Y + 30.0f, R.W - 260.0f, 14.0f, RitC(RitMuted), 20.0f, 3);
	if (X.P.InLeague)
	{
		RitParagraph(X.C, "Your group plays in " + std::string(RitDivName(X.P.Div)) + ".", R.X + 220.0f, Y + 100.0f, R.W - 260.0f, 14.0f, RitC(RitInk), 20.0f, 1);
	}
}

/** The side column (§3.4 C Side): your season, the Callout panel, the Watchlist feed. */
void RitLeagueSide(RitCtx& X)
{
	const RitPlay& P = X.P;
	// Your season: the crest in its metal with its stars, the path, what promotion and safety need.
	const Rect S{1188.0f, 176.0f, 388.0f, 300.0f};
	RitPanel(X.C, S, 10.0f);
	RitLabel(X.C, "Your season", S.X + 20.0f, S.Y + 28.0f, 12.0f, RitC(RitMuted));
	if (P.MonthKey >= 0)
	{
		RitText(X.C, RitMonthName(P.MonthKey, false), S.X + S.W - 20.0f, S.Y + 28.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	}
	RitCrest(X.C, S.X + 64.0f, S.Y + 104.0f, 76.0f, P.Div, P.Crest);
	RitTwice(X.C, RitUpper(RitDivName(P.Div)), S.X + 118.0f, S.Y + 96.0f, RitFit(X.C, RitUpper(RitDivName(P.Div)), 34.0f, 24.0f, S.W - 138.0f), RitC(RitWhite));
	RitLabel(X.C, P.InLeague ? "Group " + std::to_string(P.Group) + " \xC2\xB7 " + RitPlace(P.Rank) + " of " + std::to_string(P.Of) : std::string("No group yet"), S.X + 118.0f, S.Y + 122.0f, 14.0f,
		RitC(RitInk));
	// The path: the last 3 closed months and this one.
	float Px = S.X + 118.0f;
	for (int I = static_cast<int>(std::min<size_t>(3, P.History.size())) - 1; I >= 0; --I)
	{
		RitPip(X.C, Px, S.Y + 136.0f, 12.0f, P.History[static_cast<size_t>(I)].Div);
		Px += 18.0f;
	}
	RitPip(X.C, Px, S.Y + 136.0f, 12.0f, P.Div);
	RitText(X.C, "now", Px + 18.0f, S.Y + 146.0f, 11.0f, RitC(RitMuted), RitFace::Body);
	float Y = S.Y + 190.0f;
	if (P.InLeague)
	{
		RitTerm(X, S.X + 20.0f, S.X + S.W - 20.0f, Y, "POINTS", RitPts(P.Points), RitWhite);
		Y += 28.0f;
		RitTerm(X, S.X + 20.0f, S.X + S.W - 20.0f, Y, RitTopDivision(P) ? "TO THE TOP " + std::to_string(P.Up) : std::string("TO GO UP"),
			P.Rank > 0 && P.Rank <= P.Up ? std::string("in the places") : RitPts(P.ToPromotion) + " pts", RitInk);
		Y += 28.0f;
		// Core's ToSafety is the points still short of the safe places (0: safe), as ToPromotion is (review-ui-rit: it was drawn
		// as "pts clear", telling a member in the drop zone he was clear of it).
		RitTerm(X, S.X + 20.0f, S.X + S.W - 20.0f, Y, "TO STAY UP", !RitRelegates(P) ? std::string("nobody goes down") : P.ToSafety > 0.0f ? RitPts(P.ToSafety) + " pts short" : std::string("clear"), RitInk);
		Y += 30.0f;
	}
	if (P.Leave)
	{
		RitTag(X.C, S.X + 20.0f, Y - 12.0f, "ON LEAVE", RitChrome, RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, true);
	}
	else if (P.Placement)
	{
		RitTag(X.C, S.X + 20.0f, Y - 12.0f, "PLACEMENT MONTH", RitChrome, RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, true);
	}
	else if (P.Bounce)
	{
		RitTag(X.C, S.X + 20.0f, Y - 12.0f, "BOUNCE BACK", RitWhite, RitOnyx, 11.0f);
		RitParagraph(X.C, RitLeagueBounce, S.X + 20.0f, Y + 16.0f, S.W - 40.0f, 12.0f, RitC(RitMuted), 16.0f, 2);
	}
	// The Callout panel (callout): this week's duel, or how to start one.
	const Rect Co{1188.0f, 488.0f, 388.0f, 196.0f};
	RitPanel(X.C, Co, 10.0f);
	const Ui::ClickState Cs = RitClick(X, "callout", Co, P.Callout.On);
	(void)Cs;
	RitLabel(X.C, "The callout", Co.X + 20.0f, Co.Y + 28.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "Record " + std::to_string(P.Callout.W) + "-" + std::to_string(P.Callout.L), Co.X + Co.W - 20.0f, Co.Y + 28.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	if (P.Callout.On)
	{
		RitSquareAvatar(X.C, Co.X + 20.0f, Co.Y + 44.0f, 40.0f, P.Callout.Name, RitC(RitAmber));
		RitText(X.C, P.Callout.Name, Co.X + 72.0f, Co.Y + 62.0f, 15.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 200.0f);
		RitLabel(X.C, (P.Callout.ByThem ? std::string("Called you out") : std::string("You called them out")) + (P.Callout.Country.empty() ? std::string() : " \xC2\xB7 " + P.Callout.Country), Co.X + 72.0f,
			Co.Y + 80.0f, 11.0f, RitC(P.Callout.ByThem ? RitAmber : RitMuted));
		RitTag(X.C, Co.X + Co.W - 52.0f, Co.Y + 48.0f, "VS", RitAmber, RitOnyx, 12.0f, 9.0f, 18.0f);
		// The week so far: yours against theirs, by the League's own scoring.
		const bool Lead = P.Callout.Mine >= P.Callout.Theirs;
		RitTwice(X.C, RitPts(P.Callout.Mine), Co.X + 20.0f, Co.Y + 140.0f, 32.0f, RitC(RitWhite));
		RitLabel(X.C, "you", Co.X + 20.0f, Co.Y + 158.0f, 11.0f, RitC(RitMuted));
		RitText(X.C, RitPts(P.Callout.Theirs), Co.X + Co.W - 20.0f, Co.Y + 140.0f, 32.0f, RitC(RitInk), RitFace::Cond, Align::Right);
		RitLabel(X.C, "them", Co.X + Co.W - 20.0f, Co.Y + 158.0f, 11.0f, RitC(RitMuted), Align::Right);
		RitText(X.C, Lead ? "LEADING" : "TRAILING", Co.X + Co.W / 2.0f, Co.Y + 132.0f, 13.0f, RitC(Lead ? RitUp : RitInk), RitFace::Cond, Align::Center);
		RitText(X.C, "Ends in " + RitCountdown(P.Callout.Ends - X.World), Co.X + 20.0f, Co.Y + 184.0f, 11.0f, RitC(RitMuted), RitFace::Mono);
		if (P.Callout.Tonight)
		{
			RitText(X.C, "registered tonight", Co.X + Co.W - 20.0f, Co.Y + 184.0f, 11.0f, RitC(RitAmber), RitFace::Body, Align::Right);
		}
	}
	else
	{
		// Under the clause core refuses a call (Session::CallOut) even in a month played here: say so rather than invite it.
		bool AnyCall = false;
		for (const RitLeagueRow& Row : P.Table)
		{
			AnyCall = AnyCall || Row.CanCall;
		}
		const char* Why = P.Leave ? RitCalloutLeave : P.Clause && !AnyCall ? RitCalloutClause : P.InLeague ? RitCalloutNone : RitCalloutNoGroup;
		RitParagraph(X.C, Why, Co.X + 20.0f, Co.Y + 62.0f, Co.W - 40.0f, 13.0f, RitC(RitMuted), 18.0f, 4);
		RitParagraph(X.C, RitCalloutRule, Co.X + 20.0f, Co.Y + 150.0f, Co.W - 40.0f, 11.0f, RitC(RitMuted), 15.0f, 2);
	}
	RitWatchFeed(X, {1188.0f, 696.0f, 388.0f, 214.0f});
}

void RitLeaguePage(RitCtx& X)
{
	const RitPlay& P = X.P;
	RitPanelBegin();
	const int Shown = X.U.LeagueTab >= 0 && X.U.LeagueTab < RitDivisions ? X.U.LeagueTab : P.Div;
	RitLeagueTabs(X, Shown);
	// The Briefing, pinned (R9), over the side column; the summary line.
	RitBriefingBand(X, {1188.0f, 96.0f, 388.0f, 72.0f}, true);
	RitPanelBegin();
	X.C.FillRect({RitLgX, 136.0f, RitLgW, 32.0f}, RitC(RitBg2));
	if (P.Leave)
	{
		RitLabel(X.C, "On leave \xC2\xB7 your division is kept", RitLgX + 12.0f, 157.0f, 15.0f, RitC(RitInk));
	}
	else if (P.InLeague)
	{
		const std::string Sum = RitUpper(RitDivName(P.Div)) + " \xC2\xB7 GROUP " + std::to_string(P.Group) + " \xC2\xB7 YOU " + RitPlace(P.Rank) + " \xC2\xB7 " + RitPts(P.Points) +
			" PTS \xC2\xB7 BEST 8 DAYS COUNT \xC2\xB7 TOP " + std::to_string(P.Up) + (RitTopDivision(P) ? " WIN A SUNDAY 100K TICKET" : " UP") +
			(RitRelegates(P) ? " \xC2\xB7 BOTTOM " + std::to_string(P.Down) + " DOWN" : std::string(" \xC2\xB7 NOBODY GOES DOWN"));
		RitText(X.C, Sum, RitLgX + 12.0f, 157.0f, RitFit(X.C, Sum, 15.0f, 12.0f, 640.0f), RitC(RitInk), RitFace::Cond, Align::Left, Baseline::Alphabetic, 640.0f);
	}
	else
	{
		RitLabel(X.C, RitUpper(RitDivName(P.Div)) + " \xC2\xB7 no group yet", RitLgX + 12.0f, 157.0f, 15.0f, RitC(RitMuted));
	}
	if (P.MonthKey >= 0 || P.Closes > 0.0)
	{
		const std::string Right = (P.Deadline ? std::string("DEADLINE DAY") : "MATCHWEEK " + std::to_string(P.Matchweek) + " OF " + std::to_string(P.Matchweeks)) + " \xC2\xB7 " +
			(P.Deadline ? RitCountdown(P.Closes - X.World) + " LEFT" : "DEADLINE " + RitCountdown(P.Closes - X.World));
		RitText(X.C, Right, RitLgX + RitLgW - 12.0f, 157.0f, 15.0f, RitC(P.Deadline ? RitWhite : RitChrome), RitFace::Cond, Align::Right);
	}
	if (Shown != P.Div)
	{
		RitDivisionCard(X, Shown);
	}
	else
	{
		// The table: header, 20 rows (pitch 34), the zones closed by twin stripes in Signal Up and Signal Down (rules, not the
		// brand device), your row ruled in Hot White.
		X.C.FillRect({RitLgX, 176.0f, RitLgW, 30.0f}, RitC(RitBg2));
		X.C.FillRect({RitLgX, 205.0f, RitLgW, 1.0f}, RitC(RitHair));
		const float Hy = 191.0f + 12.0f * 0.36f;
		RitLabel(X.C, "Pos", RitLgX + 12.0f, Hy, 12.0f, RitC(RitMuted));
		RitLabel(X.C, "Player", RitLgX + 116.0f, Hy, 12.0f, RitC(RitMuted));
		RitLabel(X.C, "Path", RitLgX + 372.0f, Hy, 12.0f, RitC(RitMuted));
		RitLabel(X.C, "Days", RitLgX + 468.0f, Hy, 12.0f, RitC(RitMuted), Align::Right);
		RitLabel(X.C, "Best 8", RitLgX + 486.0f, Hy, 12.0f, RitC(RitMuted));
		RitLabel(X.C, "Points", RitLgX + 680.0f, Hy, 12.0f, RitC(RitMuted), Align::Right);
		RitLabel(X.C, "Form", RitLgX + 700.0f, Hy, 12.0f, RitC(RitMuted));
		if (P.Table.empty())
		{
			RitParagraph(X.C, P.Started ? RitLeagueNoGroup : RitLeagueNone, RitLgX + 40.0f, 300.0f, RitLgW - 80.0f, 15.0f, RitC(RitMuted), 22.0f, 3);
			RitParagraph(X.C, RitLeagueRule, RitLgX + 40.0f, 380.0f, RitLgW - 80.0f, 13.0f, RitC(RitMuted), 19.0f, 3);
		}
		float Top = 0.0f;
		for (const RitLeagueRow& R : P.Table)
		{
			Top = std::max(Top, R.Best[0]);
		}
		const bool Night = X.U.NightOn;
		for (size_t I = 0; I < P.Table.size() && I < 20; ++I)
		{
			const float Y = RitLgRows + static_cast<float>(I) * RitLgPitch;
			RitLeagueRowDraw(X, P.Table[I], Y, static_cast<int>(I), Top, Night);
			// The promotion line under the last promoted row, the relegation line over the first relegated one.
			const int Rank = static_cast<int>(I) + 1;
			if (Rank == P.Up && I + 1 < P.Table.size())
			{
				RitTwinH(X.C, RitLgX, Y + RitLgPitch - 3.0f, RitLgW, 0.5f, RitUp);
			}
			if (RitRelegates(P) && Rank == static_cast<int>(P.Table.size()) - P.Down && I + 1 < P.Table.size())
			{
				RitTwinH(X.C, RitLgX, Y + RitLgPitch - 3.0f, RitLgW, 0.5f, RitDown);
			}
		}
	}
	RitLeagueSide(X);
	// The footer: this week's Matchdays and Deadline Day.
	const Rect F{RitLgX, 916.0f, RitLgW, 34.0f};
	X.C.FillRect(F, RitC(RitBg2));
	RitLabel(X.C, "This week", F.X + 12.0f, F.Y + 22.0f, 12.0f, RitC(RitMuted));
	float Fx = F.X + 96.0f;
	if (P.Fixtures.empty())
	{
		RitText(X.C, RitNoFixtures, Fx, F.Y + 22.0f, 12.0f, RitC(RitMuted), RitFace::Body);
	}
	for (const RitFixture& Fx0 : P.Fixtures)
	{
		if (Fx > F.X + F.W - 160.0f)
		{
			break;
		}
		const bool Today = Fx0.Day == P.Today;
		Fx += RitTag(X.C, Fx, F.Y + 9.0f, Fx0.Deadline ? "DEADLINE" : "MD", Today ? RitWhite : RitChrome, Today ? RitOnyx : RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, !Today) + 6.0f;
		const std::string Line = RitUpper(net::WeekdayName(Fx0.Day)) + " " + RitUpper(Fx0.Name) + " x" + RitMult(Fx0.Mult);
		Fx += RitText(X.C, Line, Fx, F.Y + 22.0f, 12.0f, RitC(Today ? RitWhite : RitInk), RitFace::Cond) + 18.0f;
	}
}

// ------------------------------------------------------------------ League Night (§3.6 R2): the season finale

bool RitLeagueNight(RitCtx& X, bool Scrim)
{
	RitUi& U = X.U;
	const RitNight& N = X.P.Night;
	if (!U.NightOn)
	{
		return false;
	}
	const double T = X.Now - U.NightAt;
	// About 3 s, skippable (a click anywhere: the scrim's is swallowed), pointer-blocked from the frame's top.
	const Rect Panel{232.0f, 96.0f, 1344.0f, 854.0f};
	const Ui::ClickState Skip = X.UI.Clickable("nightskip", Panel);
	if (!N.On || Scrim || Skip.Clicked || T >= 3.6)
	{
		U.NightOn = false;
		return false;
	}
	RitPanelBegin();
	X.C.FillRect({0.0f, 0.0f, 1600.0f, 962.0f}, RitC(RitTape, 0.80f * static_cast<float>(Clamp01(T / 0.2))));
	RitPanel(X.C, Panel, 10.0f, RitOnyx);
	X.C.FillRect({Panel.X, Panel.Y, Panel.W - 10.0f, 4.0f}, RitC(RitRed));
	RitTwice(X.C, "LEAGUE NIGHT", Panel.X + 24.0f, Panel.Y + 52.0f, 34.0f, RitC(RitWhite));
	RitLabel(X.C, RitMonthName(N.Month, false) + " \xC2\xB7 " + RitDivName(N.From) + " \xC2\xB7 the final standings", Panel.X + 24.0f, Panel.Y + 78.0f, 13.0f, RitC(RitMuted));
	RitText(X.C, RitNightSkip, Panel.X + Panel.W - 24.0f, Panel.Y + 40.0f, 12.0f, RitC(RitMuted), RitFace::Body, Align::Right);
	// The re-sort: each row from its place on the close's eve to its final place (staggered 20 ms), the player's row last;
	// promoted rows rise through the green line under a white light sweep, relegated ones sink through the red.
	const float Ty = Panel.Y + 110.0f;
	const float Pitch = 34.0f;
	const float Lx = Panel.X + 24.0f;
	const float Lw = 900.0f;
	if (N.Final.empty())
	{
		// The book keeps the player's own close (its place of the group), not the world's closed table (the world records only
		// this month's group): the group's places as an empty ladder, the zone lines, and your row landing on your place.
		const int Of = std::max(1, std::min(20, std::max(N.Of, N.Rank)));
		for (int I = 0; I < Of; ++I)
		{
			const float Y = Ty + static_cast<float>(I) * Pitch;
			X.C.FillRect({Lx, Y, Lw, Pitch}, RitC(I % 2 == 0 ? RitCarbon : RitPanel2));
			RitText(X.C, std::to_string(I + 1), Lx + 34.0f, Y + Pitch / 2.0f + 18.0f * 0.36f, 18.0f, RitC(RitMuted), RitFace::Cond, Align::Right);
		}
		const int Up = std::max(0, std::min(Of, X.P.Up));
		if (Up > 0 && Up < Of)
		{
			RitTwinH(X.C, Lx, Ty + Pitch * static_cast<float>(Up) - 3.0f, Lw, 0.5f, RitUp);
		}
		if (X.P.Down > 0 && N.From > 0 && Of - X.P.Down > 0) // Open never relegates (RitClose)
		{
			RitTwinH(X.C, Lx, Ty + Pitch * static_cast<float>(Of - X.P.Down) - 3.0f, Lw, 0.5f, RitDown);
		}
		if (N.Rank >= 1 && N.Rank <= Of)
		{
			const float K = static_cast<float>(motion::CountUp(0.0, 1.0, T, 1.0, 0.5));
			const float Y = Ty + static_cast<float>(N.Rank - 1) * Pitch;
			X.C.PushClip({Lx, Ty, Lw, Pitch * static_cast<float>(Of)});
			const float Rx0 = Lx + (1.0f - K) * Lw;
			X.C.FillRect({Rx0, Y, Lw, Pitch}, RitC(N.To > N.From ? RitPromo : N.To < N.From ? 0x321b1e : RitCarbon));
			X.C.FillRect({Rx0, Y, Lw, 1.0f}, RitC(RitWhite));
			X.C.FillRect({Rx0, Y + Pitch - 1.0f, Lw, 1.0f}, RitC(RitWhite));
			RitText(X.C, std::to_string(N.Rank), Rx0 + 34.0f, Y + Pitch / 2.0f + 18.0f * 0.36f, 18.0f, RitC(RitWhite), RitFace::Cond, Align::Right);
			RitText(X.C, X.S.HeroName, Rx0 + 60.0f, Y + Pitch / 2.0f + 14.0f * 0.36f, 14.0f, RitC(RitWhite), RitFace::Name);
			X.C.PopClip();
		}
	}
	const int Rows = static_cast<int>(std::min<size_t>(20, N.Final.size()));
	const int Up = std::max(0, std::min(Rows, X.P.Up));
	X.C.PushClip({Lx, Ty, Lw, Pitch * static_cast<float>(std::max(Rows, 1))});
	// The others first, then the player's row over them (it lands last).
	std::vector<int> Order;
	for (int I = 0; I < Rows; ++I)
	{
		if (N.Final[static_cast<size_t>(I)].Npc >= 0)
		{
			Order.push_back(I);
		}
	}
	for (int I = 0; I < Rows; ++I)
	{
		if (N.Final[static_cast<size_t>(I)].Npc < 0)
		{
			Order.push_back(I);
		}
	}
	for (const int I : Order)
	{
		const RitLeagueRow& R = N.Final[static_cast<size_t>(I)];
		int From = I;
		for (size_t J = 0; J < N.Was.size(); ++J)
		{
			From = N.Was[J].Npc == R.Npc && N.Was[J].Name == R.Name ? static_cast<int>(J) : From;
		}
		const bool You = R.Npc < 0;
		const double Start = You ? 1.7 : 0.3 + 0.02 * static_cast<double>(I);
		const double K = motion::CountUp(0.0, 1.0, T, Start, You ? 0.5 : 0.4);
		const float Pos = static_cast<float>(From) + (static_cast<float>(I) - static_cast<float>(From)) * static_cast<float>(K);
		const float Y = Ty + Pos * Pitch;
		const uint32_t Ground = R.Zone > 0 && K >= 1.0 ? RitPromo : R.Zone < 0 && K >= 1.0 ? 0x321b1e : I % 2 == 0 ? RitCarbon : RitPanel2;
		X.C.FillRect({Lx, Y, Lw, Pitch}, RitC(Ground));
		if (You)
		{
			X.C.FillRect({Lx, Y, Lw, 1.0f}, RitC(RitWhite));
			X.C.FillRect({Lx, Y + Pitch - 1.0f, Lw, 1.0f}, RitC(RitWhite));
			// One RitRank tick per place gained as the row passes it.
			const int Passed = static_cast<int>(std::floor(static_cast<float>(From) - Pos + 0.0001f));
			while (U.NightRankTicks < Passed && U.NightRankTicks < 19)
			{
				++U.NightRankTicks;
				RitSound(X, SoundId::RitRank, 0.3f);
			}
		}
		if (R.Zone > 0 && K > 0.0 && K < 1.0)
		{
			RitSweep(X.C, {Lx, Y, Lw, Pitch}, static_cast<float>(-0.2 + 1.4 * K), 60.0f);
		}
		const float Mid = Y + Pitch / 2.0f;
		RitText(X.C, std::to_string(static_cast<int>(std::floor(Pos + 0.5f)) + 1), Lx + 34.0f, Mid + 18.0f * 0.36f, 18.0f, RitC(You ? RitWhite : RitInk), RitFace::Cond, Align::Right);
		RitText(X.C, You ? X.S.HeroName : R.Name, Lx + 60.0f, Mid + 14.0f * 0.36f, 14.0f, RitC(You ? RitWhite : RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 400.0f);
		RitText(X.C, RitPts(R.Points), Lx + Lw - 20.0f, Mid + 15.0f * 0.36f, 15.0f, RitC(You ? RitWhite : RitInk), RitFace::Mono, Align::Right);
	}
	X.C.PopClip();
	if (Up > 0 && Up < Rows)
	{
		RitTwinH(X.C, Lx, Ty + Pitch * static_cast<float>(Up) - 3.0f, Lw, 0.5f, RitUp);
	}
	if (X.P.Down > 0 && N.From > 0 && Rows - X.P.Down > 0)
	{
		RitTwinH(X.C, Lx, Ty + Pitch * static_cast<float>(Rows - X.P.Down) - 3.0f, Lw, 0.5f, RitDown);
	}
	// The crest re-forges: the old shield splits along its chevron, the new one assembles (RitRank: rising for a promotion,
	// falling for a relegation, once).
	const float Cx = Panel.X + 1140.0f;
	const float Cy = Panel.Y + 360.0f;
	const bool Moved = N.To != N.From;
	if (T < 2.2 || !Moved)
	{
		RitCrest(X.C, Cx, Cy, 170.0f, N.From, 0, Moved ? static_cast<float>(Clamp01((T - 1.8) / 0.4)) : 0.0f);
	}
	else
	{
		RitCrest(X.C, Cx, Cy, 170.0f, N.To, 0, static_cast<float>(1.0 - Clamp01((T - 2.2) / 0.4)));
		if (U.NightRankTicks < 100)
		{
			U.NightRankTicks = 100;
			RitSound(X, SoundId::RitRank, N.To > N.From ? 0.55f : 0.45f);
		}
	}
	const std::string Word = N.To > N.From ? "PROMOTED TO " + RitUpper(RitDivName(N.To)) : N.To < N.From ? "RELEGATED TO " + RitUpper(RitDivName(N.To)) : "STAYED IN " + RitUpper(RitDivName(N.To));
	if (T >= 2.4)
	{
		RitText(X.C, Word, Cx, Cy + 140.0f, RitFit(X.C, Word, 24.0f, 16.0f, 380.0f), RitC(N.To > N.From ? RitUp : N.To < N.From ? RitInk : RitInk), RitFace::Cond, Align::Center);
		RitText(X.C, RitPlace(N.Rank) + " OF " + std::to_string(N.Of), Cx, Cy + 168.0f, 15.0f, RitC(RitChrome), RitFace::Cond, Align::Center);
		if (!N.Ticket.empty())
		{
			RitText(X.C, N.Ticket + (N.Held ? " \xC2\xB7 on the Guest List" : " \xC2\xB7 on the Club"), Cx, Cy + 194.0f, 13.0f, RitC(RitInk), RitFace::Body, Align::Center, Baseline::Alphabetic, 380.0f);
		}
	}
	return true;
}

// ------------------------------------------------------------------ the Callout drawer (R3) and INCOMING CALLOUT

/** The drawer's rect: from the right edge over the side column. */
Rect RitCallRect(const RitUi& U, double Now, bool Preview)
{
	const float In = Preview ? 1.0f : static_cast<float>(motion::CountUp(0.0, 1.0, Now, U.CallAt, 0.16));
	return {1600.0f - 424.0f * In, 84.0f, 424.0f, 878.0f};
}

void RitCalloutDrawer(RitCtx& X, bool Scrim)
{
	RitUi& U = X.U;
	if (U.CallNpc < 0)
	{
		return;
	}
	const RitLeagueRow* Who = nullptr;
	for (const RitLeagueRow& R : X.P.Table)
	{
		Who = R.Npc == U.CallNpc ? &R : Who;
	}
	if (!Who || Scrim || X.Preview)
	{
		U.CallNpc = Scrim || !Who ? -1 : U.CallNpc;
		if (X.Preview || !Who || Scrim)
		{
			return;
		}
	}
	const Rect D = RitCallRect(U, X.Now, X.Preview);
	RitPanelBegin();
	X.C.FillRect({0.0f, 84.0f, D.X, 878.0f}, RitC(RitTape, 0.80f));
	RitGlow(X.C, {D.X - 6.0f, D.Y, 12.0f, D.H}, 0x000000, 0.6f, 14.0f);
	X.C.FillRect(D, RitC(RitBg2));
	X.C.FillRect({D.X, D.Y, 1.0f, D.H}, RitC(RitHair));
	X.C.FillRect({D.X, D.Y, 4.0f, D.H}, RitC(RitRed));
	const float L = D.X + 28.0f;
	const float Rx = D.X + D.W - 28.0f;
	RitTwice(X.C, "CALL OUT", L, D.Y + 64.0f, 34.0f, RitC(RitWhite));
	RitTwinH(X.C, L, D.Y + 78.0f, 72.0f, 0.5f);
	RitLabel(X.C, RitGroupTag(X.P) + " \xC2\xB7 this matchweek", L, D.Y + 110.0f, 12.0f, RitC(RitMuted));
	RitSquareAvatar(X.C, L, D.Y + 132.0f, 64.0f, Who->Name, RitC(RitChrome));
	RitText(X.C, Who->Name, L + 80.0f, D.Y + 158.0f, 18.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, Rx - L - 80.0f);
	RitLabel(X.C, RitPlace(Who->Rank) + " in the group" + (Who->Country.empty() ? std::string() : " \xC2\xB7 " + Who->Country), L + 80.0f, D.Y + 180.0f, 12.0f, RitC(RitMuted));
	if (Who->Team)
	{
		RitTag(X.C, L + 80.0f, D.Y + 186.0f, "TEAM", RitRed, RitWhite, 11.0f);
	}
	float Y = D.Y + 240.0f;
	RitTerm(X, L, Rx, Y, "THEIR POINTS", RitPts(Who->Points), RitInk);
	Y += 28.0f;
	RitTerm(X, L, Rx, Y, "DAYS WITH A RESULT", std::to_string(Who->Days), RitInk);
	Y += 28.0f;
	const RitLeagueRow* Me = RitHeroRow(X.P);
	RitTerm(X, L, Rx, Y, "YOURS", Me ? RitPts(Me->Points) : std::string("\xE2\x80\x94"), RitWhite);
	Y += 40.0f;
	Y = RitParagraph(X.C, RitCallDrawerWhat, L, Y, Rx - L, 14.0f, RitC(RitInk), 20.0f, 4) + 8.0f;
	Y = RitParagraph(X.C, RitCalloutRule, L, Y, Rx - L, 13.0f, RitC(RitMuted), 18.0f, 3) + 4.0f;
	RitParagraph(X.C, RitCallOnce, L, Y, Rx - L, 13.0f, RitC(RitMuted), 18.0f, 2);
	// CALL OUT (a red slab) and CANCEL; core decides (Session::CallOut: "" called, else why not).
	const Rect Ok{L, D.Y + D.H - 170.0f, Rx - L, 56.0f};
	const Rect No{L, D.Y + D.H - 104.0f, Rx - L, 44.0f};
	const Ui::ClickState Os = RitClick(X, "callok", Ok);
	const Ui::ClickState Ns = RitClick(X, "callno", No);
	RitSlab(X.C, Ok, 8.0f, Os.Hover, X.Now);
	const std::string Label = "CALL OUT " + RitUpper(Who->Name);
	RitText(X.C, Label, Ok.X + Ok.W / 2.0f, Ok.Y + Ok.H / 2.0f, RitFit(X.C, Label, 20.0f, 13.0f, Ok.W - 24.0f), RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
	RitCutFill(X.C, No, 8.0f, RitGlossPaint(No));
	RitCutStroke(X.C, No, 8.0f, RitC(RitChrome, Ns.Hover ? 1.0f : 0.6f), 1.0f);
	RitText(X.C, "CANCEL", No.X + No.W / 2.0f, No.Y + No.H / 2.0f, 15.0f, RitC(RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
	if (!U.CallSaid.empty())
	{
		RitText(X.C, U.CallSaid, L, D.Y + D.H - 34.0f, 12.0f, RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, Rx - L);
	}
	if (Os.Clicked)
	{
		const std::string Why = X.S.CallOut(U.CallNpc);
		if (Why.empty())
		{
			RitSound(X, SoundId::RitBlip, 0.4f);
			RitTapeItem Item;
			Item.Key = "callout:" + std::to_string(U.CallNpc);
			Item.Text = "CALLOUT \xC2\xB7 YOU CALLED OUT " + RitUpper(Who->Name);
			X.U.Extra.insert(X.U.Extra.begin(), Item);
			if (X.U.Extra.size() > 3)
			{
				X.U.Extra.pop_back();
			}
			X.U.TapeKey = -1;
			U.CallNpc = -1;
		}
		else
		{
			U.CallSaid = Why;
			U.CallSaidAt = X.Now;
		}
	}
	else if (Ns.Clicked)
	{
		U.CallNpc = -1;
	}
}

/** The alert's rect while it shows (the frame holds the pointer under it): 6 s from its first sight. */
Rect RitIncomingRect(float X0)
{
	return {X0 + 300.0f, 92.0f, 760.0f, 40.0f};
}

void RitIncoming(RitCtx& X, float X0)
{
	const RitCallout& Co = X.P.Callout;
	if (X.Preview || !Co.On || !Co.ByThem)
	{
		return;
	}
	if (X.U.IncomingSeen != Co.Week && !Co.Seen)
	{
		X.U.IncomingSeen = Co.Week;
		X.U.IncomingAt = X.Now;
		RitSound(X, SoundId::RitBell, 0.35f);
		// G2 fix L6: the save keeps the mark (once a Matchweek, not once a launch).
		if (site::play::SeeIncomingCallout(X.S.Life, Co.Week, X.World))
		{
			X.S.Save();
		}
	}
	const double T = X.Now - X.U.IncomingAt;
	if (T < 0.0 || T > 6.0)
	{
		return;
	}
	const Rect R0 = RitIncomingRect(X0);
	const float In = static_cast<float>(motion::CountUp(0.0, 1.0, X.Now, X.U.IncomingAt, 0.16)) * static_cast<float>(Clamp01((6.0 - T) / 0.3));
	const Rect R{R0.X, R0.Y - (1.0f - In) * 40.0f, R0.W, R0.H};
	const float A0 = X.C.GetAlpha();
	X.C.SetAlpha(A0 * In);
	RitGlow(X.C, {R.X, R.Y + 4.0f, R.W, R.H}, RitRed, 0.45f, 14.0f);
	RitCutFill(X.C, R, 8.0f, RitC(RitRed));
	RitLedEdge(X.C, R.X, R.Y, R.W - 8.0f);
	std::string Line = "INCOMING CALLOUT \xC2\xB7 " + RitUpper(Co.Name);
	const std::string Paren = RitUpper(RitDivName(X.P.Div)) + (Co.Country.empty() ? std::string() : ", " + RitUpper(Co.Country));
	Line += " (" + Paren + ")";
	RitText(X.C, Line, R.X + 18.0f, R.Y + R.H / 2.0f, 18.0f, RitC(RitWhite), RitFace::Cond, Align::Left, Baseline::Middle, R.W - 130.0f);
	const Rect View{R.X + R.W - 96.0f, R.Y + 8.0f, 84.0f, 24.0f};
	const Ui::ClickState St = RitClick(X, "incoming", View);
	RitCutFill(X.C, View, 6.0f, RitC(RitDeep));
	RitText(X.C, "THE LEAGUE", View.X + View.W / 2.0f, View.Y + 12.0f, 12.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
	X.C.SetAlpha(A0);
	if (St.Clicked)
	{
		X.U.IncomingAt = X.Now - 6.0;
		if (X.S.CurrentScreen == Screen::Table)
		{
			X.S.ShowLobby(); // from a table, as a rail plate does: the tables keep running
			X.RL.SiteShown = RitSite;
		}
		X.RL.OpenPage(RiverLine::Page::League, X.Now);
	}
}

// ------------------------------------------------------------------ the Watchlist's feed

void RitWatchFeed(RitCtx& X, const Rect& R)
{
	RitPanel(X.C, R, 10.0f);
	RitLabel(X.C, "The watchlist", R.X + 20.0f, R.Y + 28.0f, 12.0f, RitC(RitMuted));
	RitStar(X.C, R.X + R.W - 26.0f, R.Y + 24.0f, 6.0f, !X.P.Watch.empty());
	if (X.P.Watch.empty())
	{
		RitParagraph(X.C, RitWatchNone, R.X + 20.0f, R.Y + 60.0f, R.W - 40.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
		return;
	}
	// The next instance of each starred template still open on the network (the world's schedule).
	const net::Network& Net = net::Site(RitSite);
	std::vector<std::pair<double, net::EventInstance>> Next;
	for (const net::EventInstance& E : Net.Window(X.World - 16.0 * 60.0, X.World + 48.0 * 60.0))
	{
		const net::LiveState L = Net.Live(E, X.World);
		const bool Open = L.St == net::Status::Announced || L.St == net::Status::Registering || L.St == net::Status::LateReg;
		if (!Open || std::find(X.P.Watch.begin(), X.P.Watch.end(), RitTemplateOf(E.Id)) == X.P.Watch.end())
		{
			continue;
		}
		bool Have = false;
		for (const auto& N : Next)
		{
			Have = Have || RitTemplateOf(N.second.Id) == RitTemplateOf(E.Id);
		}
		if (!Have)
		{
			Next.push_back({L.St == net::Status::LateReg ? E.Start + E.LateReg : E.Start, E});
		}
	}
	std::sort(Next.begin(), Next.end(), [](const auto& A, const auto& B) { return A.first < B.first; });
	float Y = R.Y + 46.0f;
	for (size_t I = 0; I < Next.size() && I < 5; ++I)
	{
		const net::EventInstance& E = Next[I].second;
		const net::EventTemplate& T = Net.TemplateOf(E);
		RitFlap(X.C, R.X + 20.0f, Y, RitClock(E.Start), 13.0f, 20.0f, 5, RitTile::Gloss, RitC(RitInk));
		RitText(X.C, rlnet_detail::NetShortName(T.Name), R.X + 84.0f, Y + 15.0f, 14.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, 180.0f);
		const bool Late = Net.Live(E, X.World).St == net::Status::LateReg;
		RitText(X.C, X.S.IsPlaying(E.Id) ? std::string("SEATED") : (Late ? "CLOSES " : "IN ") + RitCountdown(Next[I].first - X.World), R.X + R.W - 20.0f, Y + 15.0f, 12.0f,
			RitC(X.S.IsPlaying(E.Id) ? RitChrome : RitMuted), RitFace::Cond, Align::Right);
		Y += 32.0f;
	}
	if (Next.empty())
	{
		RitParagraph(X.C, RitWatchQuiet, R.X + 20.0f, R.Y + 60.0f, R.W - 40.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
	}
}

// ------------------------------------------------------------------ the Briefing (R9)

/** The Briefing's close (off the League page): dismissed for the day (this laptop's state; the next 6 AM brings a new one). */
void RitBriefingClose(RitCtx& X, const Rect& R, bool Pinned)
{
	if (Pinned)
	{
		return;
	}
	const Rect Close{R.X + R.W - 34.0f, R.Y + 8.0f, 24.0f, 24.0f};
	const Ui::ClickState St = RitClick(X, "briefingx", Close);
	const Color Xc = RitC(St.Hover ? RitWhite : RitMuted);
	X.C.StrokePolyline({{Close.X + 7.0f, Close.Y + 7.0f}, {Close.X + 17.0f, Close.Y + 17.0f}}, false, Xc, 1.6f);
	X.C.StrokePolyline({{Close.X + 17.0f, Close.Y + 7.0f}, {Close.X + 7.0f, Close.Y + 17.0f}}, false, Xc, 1.6f);
	if (St.Clicked)
	{
		X.U.BriefingGone = X.P.Briefing.Day;
	}
}

bool RitBriefingBand(RitCtx& X, const Rect& R, bool Pinned)
{
	const RitBriefing& B = X.P.Briefing;
	if (!B.On || (!Pinned && X.U.BriefingGone == B.Day))
	{
		return false;
	}
	RitPanelBegin();
	RitCutFill(X.C, R, 10.0f, RitC(RitCarbon));
	RitCutStroke(X.C, R, 10.0f, RitC(RitHair), 1.0f);
	const float Tw = RitTag(X.C, R.X + 12.0f, R.Y + 10.0f, "BRIEFING", RitRed, RitWhite, 12.0f, 7.0f, 18.0f);
	if (!B.Line.empty())
	{
		// Core's own line (its figures, its words), wrapped beside and under the tag.
		RitParagraph(X.C, B.Line, R.X + 20.0f + Tw, R.Y + 24.0f, R.W - Tw - (Pinned ? 32.0f : 64.0f), 13.0f, RitC(RitInk), 20.0f, 2);
		RitBriefingClose(X, R, Pinned);
		return true;
	}
	// Yesterday's line, then today's stakes: every figure core's, the arrow a vector.
	std::string Yesterday = "Yesterday " + std::to_string(B.Events) + (B.Events == 1 ? " event, " : " events, ") + (B.Net < 0 ? "-" : "+") + Money(B.Net < 0 ? -B.Net : B.Net);
	float Lx = R.X + 20.0f + Tw;
	const float Ly = R.Y + 24.0f;
	Lx += RitText(X.C, B.Events > 0 ? Yesterday : std::string("No RunItTwice entry yesterday"), Lx, Ly, 13.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, R.W - Tw - 40.0f);
	float Y2 = R.Y + 48.0f;
	float L2 = R.X + 12.0f;
	const float Max = R.X + R.W - 12.0f;
	const auto Part = [&](const std::string& S, uint32_t Rgb) {
		if (L2 > Max - 40.0f)
		{
			return;
		}
		L2 += RitText(X.C, S, L2, Y2, 13.0f, RitC(Rgb), RitFace::Cond, Align::Left, Baseline::Alphabetic, Max - L2) + 8.0f;
	};
	if (B.RankNow > 0)
	{
		Part("LEAGUE " + RitPlace(B.RankWas > 0 ? B.RankWas : B.RankNow), RitInk);
		if (B.RankWas > 0 && B.RankWas != B.RankNow)
		{
			RitArrow(X.C, L2 - 4.0f, Y2 - 4.5f, 14.0f, RitC(B.RankNow < B.RankWas ? RitUp : RitDown), 1.6f);
			L2 += 16.0f;
			Part(RitPlace(B.RankNow), B.RankNow < B.RankWas ? RitUp : RitInk);
		}
		Part(B.ToPromotion > 0.0f ? "\xC2\xB7 " + RitPts(B.ToPromotion) + " PTS TO PROMOTION" : std::string("\xC2\xB7 IN THE PROMOTION PLACES"), RitInk);
	}
	if (!B.Matchday.empty())
	{
		Part("\xC2\xB7 MATCHDAY TONIGHT: " + RitUpper(B.Matchday), RitWhite);
	}
	if (!B.Callout.empty())
	{
		Part("\xC2\xB7 INCOMING CALLOUT: " + RitUpper(B.Callout), RitAmber);
	}
	RitBriefingClose(X, R, Pinned);
	return true;
}

// ------------------------------------------------------------------ the Club's G2 panels: Team Scalps and the Statement

void RitScalpsPanel(RitCtx& X, const Rect& R)
{
	const RitScalps& Sc = X.P.Scalps;
	RitPanel(X.C, R, 10.0f);
	RitSpaced(X.C, "TEAM SCALPS", R.X + 24.0f, R.Y + 32.0f, 13.0f, RitC(RitMuted), 1.5f);
	if (Sc.Bounty)
	{
		RitCrosshair(X.C, R.X + 70.0f, R.Y + 140.0f, 30.0f);
		RitTwice(X.C, "YOU'RE THE BOUNTY", R.X + 130.0f, R.Y + 132.0f, 30.0f, RitC(RitWhite));
		RitParagraph(X.C, RitBounty, R.X + 130.0f, R.Y + 166.0f, R.W - 160.0f, 14.0f, RitC(RitInk), 20.0f, 3);
		return;
	}
	RitText(X.C, std::to_string(Sc.Taken) + " OF 6 TAKEN", R.X + R.W - 24.0f, R.Y + 32.0f, 15.0f, RitC(Sc.Taken > 0 ? RitWhite : RitMuted), RitFace::Cond, Align::Right);
	// The six: square portraits on data cards; a taken one desaturated under the dark TAKEN stamp (a filled Signal Red
	// octagon, the word in Hot White: red ink is for paper).
	const float Cw = (R.W - 48.0f - 5.0f * 10.0f) / 6.0f;
	for (size_t I = 0; I < 6; ++I)
	{
		const Rect Card{R.X + 24.0f + static_cast<float>(I) * (Cw + 10.0f), R.Y + 52.0f, Cw, 132.0f};
		RitCutFill(X.C, Card, 6.0f, RitGlossPaint(Card));
		RitCutStroke(X.C, Card, 6.0f, RitC(RitHair), 1.0f);
		X.C.FillRect({Card.X, Card.Y, Card.W - 6.0f, 3.0f}, RitC(RitRed));
		if (I >= Sc.Six.size())
		{
			RitText(X.C, "\xE2\x80\x94", Card.X + Card.W / 2.0f, Card.Y + 70.0f, 15.0f, RitC(RitMuted), RitFace::Cond, Align::Center);
			continue;
		}
		const RitScalpPro& Pro = Sc.Six[I];
		const float Av = Cw - 28.0f;
		if (Pro.Taken)
		{
			X.C.FillRect({Card.X + 14.0f, Card.Y + 14.0f, Av, Av}, RitC(RitHair));
		}
		else
		{
			RitSquareAvatar(X.C, Card.X + 14.0f, Card.Y + 14.0f, Av, Pro.Name, RitC(RitChrome, 0.5f));
		}
		RitText(X.C, Pro.Name, Card.X + Card.W / 2.0f, Card.Y + Av + 32.0f, 12.0f, RitC(Pro.Taken ? RitMuted : RitInk), RitFace::Name, Align::Center, Baseline::Alphabetic, Card.W - 8.0f);
		if (Pro.Taken)
		{
			RitStampDark(X.C, Card.X + Card.W / 2.0f, Card.Y + 14.0f + Av / 2.0f, "TAKEN", 13.0f);
		}
	}
	// The ticket ladder and the week's cap; FULL SET.
	float Y = R.Y + 208.0f;
	RitLabel(X.C, "A scalp pays", R.X + 24.0f, Y, 12.0f, RitC(RitMuted));
	RitText(X.C, "by the event's buy-in", R.X + 120.0f, Y, 11.0f, RitC(RitMuted), RitFace::Body);
	Y += 20.0f;
	// The ladder, core's (R4): "under $109", "$109+", "$530+", each with what it pays.
	const float Col = (R.W - 48.0f) / 3.0f;
	for (size_t I = 0; I < Sc.Ladder.size() && I < 3; ++I)
	{
		const std::string From = I == 0 && Sc.Ladder.size() > 1 ? "under " + RitBuyIn(Sc.Ladder[1].first) : RitBuyIn(Sc.Ladder[I].first) + "+";
		const float Cx = R.X + 24.0f + static_cast<float>(I) * Col;
		const float Fw = RitText(X.C, From, Cx, Y, 12.0f, RitC(RitChrome), RitFace::Mono);
		RitText(X.C, Sc.Ladder[I].second, Cx + Fw + 8.0f, Y, 12.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, Col - Fw - 16.0f);
	}
	if (Sc.Ladder.empty())
	{
		RitText(X.C, RitScalpsLadder, R.X + 24.0f, Y, 12.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, R.W - 48.0f);
	}
	Y += 22.0f;
	RitText(X.C, "This week " + std::to_string(Sc.Week) + " of " + std::to_string(Sc.WeekCap) + " \xC2\xB7 lifetime " + std::to_string(Sc.Lifetime), R.X + 24.0f, Y, 12.0f, RitC(RitMuted), RitFace::Mono);
	if (Sc.FullSet)
	{
		RitTag(X.C, R.X + R.W - 100.0f, Y - 13.0f, "FULL SET", RitWhite, RitOnyx, 12.0f);
	}
	Y += 26.0f;
	RitParagraph(X.C, Sc.Six.empty() ? RitScalpsNone : RitScalpsRule, R.X + 24.0f, Y, R.W - 48.0f, 12.0f, RitC(RitMuted), 16.0f, 2);
}

void RitStatementPanel(RitCtx& X, const Rect& R)
{
	const RitStatement& St = X.P.Statement;
	RitPanel(X.C, R, 10.0f);
	RitLabel(X.C, "The statement \xC2\xB7 last 30 days", R.X + 20.0f, R.Y + 28.0f, 12.0f, RitC(RitMuted));
	// Given back as a share of rake against the Governor's ceiling (its own GivenBack30, Second Run booked to its month).
	// The figure is the Governor's own rounding (SitePlay.cpp's GovView line: llround(Share x 100)), so the big percentage and
	// the sentence beside it never disagree (review-ui-rit: 4.6% beside "(5%)", 24% beside "(25%)" at a half).
	const long long Pct = std::llround(St.Share * 100.0);
	const bool Raked = St.Rake30 > 0;
	RitTwice(X.C, Raked ? std::to_string(Pct) + "%" : std::string("\xE2\x80\x94"), R.X + 20.0f, R.Y + 82.0f, 40.0f, RitC(RitWhite));
	RitText(X.C, "given back", R.X + 20.0f, R.Y + 100.0f, 12.0f, RitC(RitMuted), RitFace::Body);
	if (Raked)
	{
		// The Governor's own sentence (its figures), else the same words from its ring.
		const std::string Says = !St.Line.empty() ? St.Line : "RunItTwice gave back " + Money(St.Promo30) + " of your " + Money(St.Rake30) + " rake in the last 30 days.";
		RitParagraph(X.C, Says, R.X + 150.0f, R.Y + 62.0f, R.W - 170.0f, 13.0f, RitC(RitInk), 18.0f, 2);
	}
	else
	{
		RitParagraph(X.C, RitRakeNone, R.X + 150.0f, R.Y + 62.0f, R.W - 170.0f, 13.0f, RitC(RitMuted), 18.0f, 2);
	}
	// The bar: the share against the ceiling (Hot White tick at it).
	const Rect Bar{R.X + 20.0f, R.Y + 118.0f, R.W - 40.0f, 8.0f};
	const double Top = std::max(St.Ceiling * 1.2, St.Share);
	X.C.FillRect(Bar, RitC(RitOnyx));
	X.C.FillRect({Bar.X, Bar.Y, Bar.W * static_cast<float>(Clamp01(St.Share / std::max(0.0001, Top))), Bar.H}, RitC(RitChrome));
	const float Cx = Bar.X + Bar.W * static_cast<float>(Clamp01(St.Ceiling / Top));
	X.C.FillRect({Cx - 1.0f, Bar.Y - 4.0f, 2.0f, Bar.H + 8.0f}, RitC(RitWhite));
	RitText(X.C, "the ceiling, " + Fixed(St.Ceiling * 100.0, 0) + "%", Cx, Bar.Y + 24.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	// The lines, each dated by the month that earned it; then the Guest List.
	float Y = R.Y + 168.0f;
	int Shown = 0;
	for (const RitStatementLine& Ln : St.Lines)
	{
		if (Shown >= 3)
		{
			break;
		}
		const std::string Month = !Ln.Month.empty() ? RitUpper(Ln.Month.substr(0, 3)) : Ln.MonthKey >= 0 ? RitMonthName(Ln.MonthKey, true) : std::string();
		const std::string Key = RitUpper(Ln.Label) + (Month.empty() ? std::string() : " \xC2\xB7 " + Month);
		RitTerm(X, R.X + 20.0f, R.X + R.W - 20.0f, Y, Key, "+" + Money(Ln.Cents), RitWhite);
		Y += 24.0f;
		++Shown;
	}
	if (St.Lines.empty())
	{
		Y = RitParagraph(X.C, RitStatementNone, R.X + 20.0f, Y - 4.0f, R.W - 40.0f, 12.0f, RitC(RitMuted), 16.0f, 3) + 4.0f;
	}
	for (size_t I = 0; I < St.Holds.size() && I < 2 && Y < R.Y + R.H - 16.0f; ++I)
	{
		const RitHold& H = St.Holds[I];
		RitTag(X.C, R.X + 20.0f, Y - 12.0f, "WAITING", RitChrome, RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, true);
		// Core's RakeLeft 0 means it clears at the next step: Ignite's Guest List words it "clears at the next 6 AM" (one thing,
		// one wording; review-ui-rit: it read "clears after $0.00 more rake").
		RitText(X.C, H.What + " \xC2\xB7 " + (H.RakeToGo > 0 ? "clears after " + Money(H.RakeToGo) + " more rake" : std::string("clears at the next 6 AM")), R.X + 92.0f, Y, 12.0f, RitC(RitInk),
			RitFace::Body, Align::Left, Baseline::Alphabetic, R.W - 112.0f);
		Y += 22.0f;
	}
}

// ------------------------------------------------------------------ the Desk's Luck Ledger (R7)

void RitLuckPanel(RitCtx& X, const Rect& R)
{
	const RitLuck& K = X.P.Luck;
	RitLabel(X.C, "Luck ledger", R.X + 20.0f, R.Y + 28.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "paid RunItTwice events", R.X + R.W - 20.0f, R.Y + 28.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Right);
	if (K.AllIns == 0)
	{
		RitParagraph(X.C, RitLuckNone, R.X + 20.0f, R.Y + 70.0f, R.W - 40.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
		return;
	}
	// All-ins, expected wins, actual wins; the gap in plain words.
	const struct
	{
		const char* Label;
		std::string Figure;
	} Figs[3] = {{"All-ins", std::to_string(K.AllIns)}, {"Expected", Fixed(K.Expected, 2)}, {"Won", std::to_string(K.Won)}};
	for (int I = 0; I < 3; ++I)
	{
		const float Fx = R.X + 20.0f + static_cast<float>(I) * 150.0f;
		RitLabel(X.C, Figs[I].Label, Fx, R.Y + 56.0f, 11.0f, RitC(RitMuted));
		RitTwice(X.C, Figs[I].Figure, Fx, R.Y + 92.0f, 32.0f, RitC(RitWhite));
	}
	const double Gap = K.Expected > 0.0 ? (static_cast<double>(K.Won) - K.Expected) / K.Expected : 0.0;
	const bool Even = std::fabs(Gap) < 0.0005;
	const std::string Line = Even ? std::string(RitLuckEven) : "You are running " + Fixed(std::fabs(Gap) * 100.0, 1) + "% " + (Gap > 0.0 ? "above" : "below") + " expectation.";
	if (!Even)
	{
		RitTri(X.C, R.X + 26.0f, R.Y + 120.0f, 9.0f, Gap > 0.0, RitC(Gap > 0.0 ? RitUp : RitDown));
	}
	RitText(X.C, Line, R.X + (Even ? 20.0f : 38.0f), R.Y + 125.0f, 14.0f, RitC(RitInk), RitFace::Body);
	// The key: expected (Chrome) against won (Hot White), by equity when the chips went in.
	const float Kx = R.X + R.W - 20.0f;
	const float Ww = RitLabel(X.C, "Won", Kx, R.Y + 125.0f, 11.0f, RitC(RitMuted), Align::Right);
	X.C.FillRect({Kx - Ww - 14.0f, R.Y + 116.0f, 9.0f, 9.0f}, RitC(RitWhite));
	const float Ew = RitLabel(X.C, "Expected", Kx - Ww - 24.0f, R.Y + 125.0f, 11.0f, RitC(RitMuted), Align::Right);
	X.C.FillRect({Kx - Ww - 24.0f - Ew - 14.0f, R.Y + 116.0f, 9.0f, 9.0f}, RitC(RitChromeDark));
	// The buckets: expected against won, through ChartArt's bars (Chrome expected, Hot White won).
	chart::BarSet Exp;
	chart::BarSet Won;
	chart::BarOpts O;
	for (int I = 0; I < 10; ++I)
	{
		Exp.V.push_back(K.Exp[static_cast<size_t>(I)]);
		Won.V.push_back(static_cast<double>(K.Wins[static_cast<size_t>(I)]));
		O.X.Labels.push_back(std::to_string(I * 10) + "%");
	}
	Exp.Col = RitC(RitChromeDark);
	Exp.Name = "Expected";
	Won.Col = RitC(RitWhite);
	Won.Name = "Won";
	O.Gap = 0.3f;
	O.YTicks = 3;
	O.Reveal = X.Preview ? 1.0f : static_cast<float>(Clamp01((X.Now - Access::PageAt(X.RL)) / 0.4));
	O.Mouse = X.Preview || !X.UI.Ptr.Active ? Vec2{-1.0f, -1.0f} : Vec2{X.UI.Ptr.X, X.UI.Ptr.Y};
	const chart::Theme T = chart::RunItTwice();
	const Rect Plot{R.X + 16.0f, R.Y + 144.0f, R.W - 32.0f, R.H - 144.0f - (K.Seconds ? 48.0f : 24.0f)};
	const chart::BarOut Out = chart::Bars(X.C, Plot, {Exp, Won}, O, T);
	if (Out.Hover >= 0 && Out.Hover < 10)
	{
		const size_t H = static_cast<size_t>(Out.Hover);
		chart::Tooltip(X.C, {Out.HoverSlot.X + Out.HoverSlot.W / 2.0f, Out.HoverSlot.Y}, std::to_string(Out.Hover * 10) + "-" + std::to_string(Out.Hover * 10 + 10) + "% equity",
			{{"All-ins", std::to_string(K.N[H])}, {"Expected", Fixed(K.Exp[H], 2)}, {"Won", std::to_string(K.Wins[H])}}, T, R);
	}
	RitText(X.C, RitLuckHow, R.X + 20.0f, R.Y + R.H - (K.Seconds ? 34.0f : 12.0f), 11.0f, RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, R.W - 40.0f);
	if (K.Seconds && K.SecondOf > 0)
	{
		RitText(X.C, "You'd have won " + std::to_string(K.SecondWon) + " of " + std::to_string(K.SecondOf) + " second runs.", R.X + 20.0f, R.Y + R.H - 12.0f, 12.0f, RitC(RitInk), RitFace::Body);
	}
}

// ------------------------------------------------------------------ the Member File's G2: credentials, League history, H2H

void RitCredentials(RitCtx& X, const Rect& R)
{
	RitLabel(X.C, "Credentials", R.X, R.Y, 12.0f, RitC(RitMuted));
	int Earned = 0;
	for (const RitCred& Cr : X.P.Creds)
	{
		Earned += Cr.Earned ? 1 : 0;
	}
	RitText(X.C, std::to_string(Earned) + " of " + std::to_string(X.P.Creds.size()), R.X + R.W, R.Y, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	if (X.P.Creds.empty())
	{
		RitParagraph(X.C, RitCredsNone, R.X, R.Y + 24.0f, R.W, 13.0f, RitC(RitMuted), 18.0f, 2);
		return;
	}
	// A credential newly earned since this laptop last looked stamps once (RitStamp; never in the Kast preview).
	std::vector<int> Now;
	for (const RitCred& Cr : X.P.Creds)
	{
		if (Cr.Earned)
		{
			Now.push_back(Cr.Id);
		}
	}
	if (!X.Preview)
	{
		if (X.U.CredsInit)
		{
			for (const int Id : Now)
			{
				if (std::find(X.U.CredsSeen.begin(), X.U.CredsSeen.end(), Id) == X.U.CredsSeen.end())
				{
					X.U.CredNew = Id;
					X.U.CredAt = X.Now;
					RitSound(X, SoundId::RitStamp, 0.4f);
				}
			}
		}
		X.U.CredsSeen = Now;
		X.U.CredsInit = true;
	}
	const float Pw = (R.W - 10.0f) / 2.0f;
	for (size_t I = 0; I < X.P.Creds.size() && I < 12; ++I)
	{
		const RitCred& Cr = X.P.Creds[I];
		const Rect Plate{R.X + static_cast<float>(I % 2) * (Pw + 10.0f), R.Y + 16.0f + static_cast<float>(I / 2) * 44.0f, Pw, 38.0f};
		std::string Line = Cr.Earned ? (Cr.Day >= 0 ? RitDate(Cr.Day) : std::string()) + (Cr.Event.empty() ? std::string() : (Cr.Day >= 0 ? " \xC2\xB7 " : "") + Cr.Event) : std::string("Not yet");
		Line = Line.empty() ? std::string("Earned") : Line;
		RitCredPlate(X.C, Plate, Cr.Name, Line, Cr.Earned);
		if (!X.Preview && Cr.Id == X.U.CredNew && X.Now - X.U.CredAt < 0.4)
		{
			RitSweep(X.C, Plate, static_cast<float>(-0.2 + 1.4 * (X.Now - X.U.CredAt) / 0.4), 30.0f);
		}
	}
}

void RitMemberLeague(RitCtx& X, const Rect& R)
{
	const RitPlay& P = X.P;
	// League history: 12 months as division bars in their metals, the move marked.
	RitLabel(X.C, "League history", R.X, R.Y, 12.0f, RitC(RitMuted));
	if (P.History.empty())
	{
		RitParagraph(X.C, RitHistNone, R.X, R.Y + 24.0f, R.W, 13.0f, RitC(RitMuted), 18.0f, 2);
	}
	for (size_t I = 0; I < P.History.size() && I < 12; ++I)
	{
		const RitPlayHist& H = P.History[P.History.size() - 1 - std::min<size_t>(P.History.size() - 1, I)];
		const float Bx = R.X + static_cast<float>(I) * 70.0f;
		const float Bh = 22.0f + 22.0f * static_cast<float>(H.Div);
		const float By = R.Y + 140.0f - Bh;
		if (H.Div == 0)
		{
			X.C.StrokePolyline({{Bx + 0.5f, By + 0.5f}, {Bx + 55.5f, By + 0.5f}, {Bx + 55.5f, By + Bh}, {Bx + 0.5f, By + Bh}}, true, RitC(RitChrome), 1.2f);
		}
		else
		{
			X.C.FillRect({Bx, By, 56.0f, Bh}, RitC(RitDivMetal(H.Div)));
			if (H.Div == 1)
			{
				// Iron's carbon on the Carbon panel: its chrome hairline carries it (as the crest's).
				X.C.StrokePolyline({{Bx + 0.5f, By + 0.5f}, {Bx + 55.5f, By + 0.5f}, {Bx + 55.5f, By + Bh}, {Bx + 0.5f, By + Bh}}, true, RitC(RitChromeDark), 1.0f);
			}
		}
		if (H.Move != 0)
		{
			RitTri(X.C, Bx + 28.0f, By - 9.0f, 9.0f, H.Move > 0, RitC(H.Move > 0 ? RitUp : RitDown));
		}
		RitText(X.C, RitMonthName(H.Month, true), Bx + 28.0f, R.Y + 158.0f, 11.0f, RitC(RitMuted), RitFace::Cond, Align::Center);
		RitText(X.C, RitPlace(H.Rank), Bx + 28.0f, R.Y + 174.0f, 11.0f, RitC(RitInk), RitFace::Mono, Align::Center);
	}
	// The Callout record and the head-to-head records.
	const float Y = R.Y + 214.0f;
	RitLabel(X.C, "Callouts", R.X, Y, 12.0f, RitC(RitMuted));
	RitText(X.C, std::to_string(P.Callout.W) + " won \xC2\xB7 " + std::to_string(P.Callout.L) + " lost", R.X + 120.0f, Y, 13.0f, RitC(RitInk), RitFace::Mono);
	RitLabel(X.C, "Head to head", R.X, Y + 34.0f, 12.0f, RitC(RitMuted));
	if (P.H2H.empty())
	{
		RitText(X.C, RitH2HNone, R.X, Y + 58.0f, 13.0f, RitC(RitMuted), RitFace::Body);
	}
	for (size_t I = 0; I < P.H2H.size() && I < 8; ++I)
	{
		const RitH2H& H = P.H2H[I];
		const float Hx = R.X + static_cast<float>(I % 2) * (R.W / 2.0f + 6.0f);
		const float Hy = Y + 60.0f + static_cast<float>(I / 2) * 28.0f;
		RitText(X.C, H.Name, Hx, Hy, 13.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 170.0f);
		RitText(X.C, "callouts " + std::to_string(H.W) + "-" + std::to_string(H.L) + " \xC2\xB7 busts " + std::to_string(H.YouKo) + "-" + std::to_string(H.TheyKo), Hx + R.W / 2.0f - 16.0f, Hy, 11.0f,
			RitC(RitMuted), RitFace::Mono, Align::Right);
	}
}

// ------------------------------------------------------------------ the table's LEAGUE tab (§3.5 Side panel)

void RitSideLeague(RiverLine& RL, Canvas& C, const Rect& Body, double Now)
{
	Session& S = Access::S(RL);
	RitCtx X{RL, S, C, Access::UI(RL), RitOf(&RL), Now, Access::World(RL), Access::Previewing(RL), RitPlayOf(&RL, S)};
	const RitPlay& P = X.P;
	float Y = Body.Y + 14.0f;
	if (!P.InLeague || P.Table.empty())
	{
		RitParagraph(X.C, P.Leave ? std::string(RitLeagueLeave) : std::string(RitSideNoLeague), Body.X, Y + 10.0f, Body.W, 13.0f, RitC(RitMuted), 18.0f, 4);
		return;
	}
	if (P.Deadline)
	{
		RitTag(X.C, Body.X, Y - 12.0f, "DEADLINE DAY", RitRed, RitWhite, 12.0f, 7.0f, 18.0f);
		RitText(X.C, RitCountdown(P.Closes - X.World) + " LEFT", Body.X + Body.W, Y + 1.0f, 12.0f, RitC(RitInk), RitFace::Cond, Align::Right);
		Y += 22.0f;
	}
	RitLabel(X.C, RitUpper(RitDivName(P.Div)) + " \xC2\xB7 GROUP " + std::to_string(P.Group), Body.X, Y + 6.0f, 13.0f, RitC(RitInk));
	Y += 22.0f;
	RitText(X.C, RitMonthTitle(P.MonthKey) + " \xC2\xB7 your best 8 days count", Body.X, Y, 12.0f, RitC(RitMuted), RitFace::Body);
	Y += 40.0f;
	RitTwice(X.C, RitPlace(P.Rank), Body.X, Y, 40.0f, RitC(RitWhite));
	RitText(X.C, RitPts(P.Points) + " PTS", Body.X + Body.W, Y, 18.0f, RitC(RitInk), RitFace::Cond, Align::Right);
	Y += 20.0f;
	RitText(X.C, P.Rank <= P.Up ? std::string("In the promotion places (top ") + std::to_string(P.Up) + ")" : RitPts(P.ToPromotion) + " pts short of promotion (top " + std::to_string(P.Up) + ")", Body.X, Y, 12.0f,
		RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, Body.W);
	Y += 28.0f;
	// This event, projected: the place a cash, a final table and a win would give, by core's LeagueScore.
	const Tournament* T = S.T.get();
	if (T)
	{
		const std::string Template = RitTemplateOf(T->Spec.Id);
		// Whether this event counts (core's Counting: the open tables' events that would score if they finished now).
		const bool CountKnown = P.On;
		const bool Counts = !CountKnown || std::find(P.Counting.begin(), P.Counting.end(), T->Spec.Id) != P.Counting.end();
		const int Entrants = std::max(static_cast<int>(T->Players.size()), 2);
		const int Paid = std::max(1, T->PaidPlaces());
		const int Ft = std::min(Entrants, std::max(2, T->TableSize));
		// A RunItTwice series event counts x1.25, as core scores the result (SitePlayRit.cpp: the template's series), and as the
		// Board's LEAGUE column and the Prospectus already project it (review-ui-rit: the tab passed false).
		const net::EventTemplate* Tm = net::Site(RitSite).FindTemplate(Template);
		const bool Series = Tm && !Tm->Series.empty();
		const auto Score = [&](int Place) { return site::play::LeagueScore(P.Div, Template, T->Spec.BuyInCents, P.Today, net::Points(Place, Entrants, T->Spec.BuyInCents), Series, P.Deadline); };
		const bool Cashed = Counts && T->Remaining > 0 && T->Remaining <= Paid && !S.Glance(S.FocusedTable()).Busted;
		const int IfCash = RitRankIf(P, Score(Paid));
		if (P.Deadline && Cashed && IfCash > 0 && IfCash <= P.Up)
		{
			// Deadline Day's flip (§3.6 R2): PROMOTION ZONE the moment you cash, when a cash puts you in the places.
			const Rect Z{Body.X, Y - 16.0f, Body.W, 30.0f};
			X.C.FillRect(Z, RitC(RitPromo));
			RitTwinH(X.C, Z.X, Z.Y + Z.H - 3.0f, Z.W, 0.5f, RitUp);
			RitText(X.C, "PROMOTION ZONE", Z.X + 10.0f, Z.Y + 15.0f, 15.0f, RitC(RitUp), RitFace::Cond, Align::Left, Baseline::Middle);
			Y += 30.0f;
		}
		const bool Known = IfCash > 0;
		RitLabel(X.C, Counts && Known ? "This event, projected" : "This event", Body.X, Y, 12.0f, RitC(RitMuted));
		Y += 22.0f;
		if (Counts && !Known)
		{
			Y = RitParagraph(X.C, RitSideFlip, Body.X, Y - 4.0f, Body.W, 11.0f, RitC(RitMuted), 15.0f, 2) + 8.0f;
		}
		else if (!Counts)
		{
			Y = RitParagraph(X.C, RitSideNotCounting, Body.X, Y - 4.0f, Body.W, 12.0f, RitC(RitMuted), 16.0f, 3) + 8.0f;
		}
		else
		{
			const std::pair<const char*, int> Outs[3] = {{"cash", Paid}, {"final table", Ft}, {"win", 1}};
			for (const auto& O : Outs)
			{
				const int Rank = RitRankIf(P, Score(O.second));
				RitText(X.C, O.first, Body.X, Y, 13.0f, RitC(RitInk), RitFace::Body);
				const bool Up = Rank <= P.Up;
				if (Up)
				{
					RitText(X.C, RitTopDivision(P) ? "100K TICKET" : "PROMOTED", Body.X + Body.W, Y, 12.0f, RitC(RitUp), RitFace::Cond, Align::Right);
				}
				RitText(X.C, RitPlace(Rank), Body.X + Body.W - (Up ? 74.0f : 0.0f), Y, 15.0f, RitC(Up ? RitUp : RitWhite), RitFace::Cond, Align::Right);
				Y += 22.0f;
			}
			Y = RitParagraph(X.C, RitSideFlip, Body.X, Y, Body.W, 11.0f, RitC(RitMuted), 15.0f, 2) + 8.0f;
		}
	}
	// The group, live: who's at this table.
	RitLabel(X.C, "Group " + std::to_string(P.Group) + " \xC2\xB7 live", Body.X, Y + 6.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "PTS", Body.X + Body.W, Y + 6.0f, 11.0f, RitC(RitMuted), RitFace::Cond, Align::Right);
	Y += 24.0f;
	for (size_t I = 0; I < P.Table.size() && Y < Body.Y + Body.H - 8.0f; ++I)
	{
		const RitLeagueRow& R = P.Table[I];
		const bool You = R.Npc < 0;
		bool Here = false;
		for (const SeatVis& V : S.Seats)
		{
			Here = Here || (V.Present && !V.IsHero && !You && V.Name == R.Name);
		}
		// The zones in their tints, your row ruled in Hot White (never a red tint: red rows are relegation's; as the League
		// page and runittwice_table.html draw it; review-ui-rit).
		if (R.Zone != 0)
		{
			X.C.FillRect({Body.X - 4.0f, Y - 15.0f, Body.W + 8.0f, 22.0f}, RitC(R.Zone > 0 ? RitPromo : 0x321b1e));
		}
		if (You)
		{
			X.C.FillRect({Body.X - 4.0f, Y - 15.0f, Body.W + 8.0f, 1.0f}, RitC(RitWhite));
			X.C.FillRect({Body.X - 4.0f, Y + 6.0f, Body.W + 8.0f, 1.0f}, RitC(RitWhite));
		}
		RitText(X.C, std::to_string(R.Rank), Body.X + 18.0f, Y, 13.0f, RitC(You ? RitWhite : RitInk), RitFace::Cond, Align::Right);
		const float Nw = RitText(X.C, You ? S.HeroName : R.Name, Body.X + 28.0f, Y, 13.0f, RitC(You ? RitWhite : RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 130.0f);
		float Tx = Body.X + 34.0f + Nw;
		if (R.Team)
		{
			Tx += RitTag(X.C, Tx, Y - 12.0f, "TEAM", RitRed, RitWhite, 10.0f, 4.0f, 15.0f) + 4.0f;
		}
		if (R.Callout)
		{
			Tx += RitTag(X.C, Tx, Y - 12.0f, "VS", RitAmber, RitOnyx, 10.0f, 4.0f, 15.0f) + 4.0f;
		}
		if (Here)
		{
			RitText(X.C, "AT TABLE", Tx, Y, 10.0f, RitC(RitChrome), RitFace::Cond);
		}
		RitText(X.C, RitPts(R.Points), Body.X + Body.W, Y, 13.0f, RitC(You ? RitWhite : RitInk), RitFace::Mono, Align::Right);
		Y += 22.0f;
	}
}
} // namespace rlrit_detail
} // namespace ui
} // namespace ss
