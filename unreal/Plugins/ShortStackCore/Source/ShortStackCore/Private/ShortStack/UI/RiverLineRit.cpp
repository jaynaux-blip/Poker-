// Update 3 (SPEC §3.2-§3.5, §7.2; v1.2's red and black, §13): RunItTwice, the Exchange after the bell, ui-rit's file: the
// client frame (the red-lit carbon rail of numbered plates, the red tape, the header with its section slab), the Board with
// the Prospectus, Sunday Live, RANKINGS, the Desk, SERIES, the Newsdesk, the Club, the Cashier, the Settlement, the Member
// File and the octagon table's layer (the lower third, the Second Board). G1 (§3.9): every surface on today's data; the
// League and its loops are G2, so the rail shows 8 plates and LEAGUE stays hidden.
//
// ClientFrame routes every RunItTwice screen but Boot here. The shared gatekeepers stay single-sourced: the schedule's rows
// (RiverLine::ScheduleRows, Listed), the register button (RegisterBlock, with its two presses under a clause), the money
// calls (Session::Deposit and Withdraw), the table (Table(): Controls, SidePanel and Overlays, drawn in RunItTwice's
// SiteLook by ui-tablestyle). The pages reach the class's private state through rlrit_detail::Access (RitArt.h).
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "../Game/SitesDetail.h"
#include "RiverLineShared.h"
#include "RitArt.h"

#include "ShortStack/Evaluator.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/ChartArt.h"
#include "ShortStack/UI/SiteArt.h"
#include "ShortStack/UI/SiteMotion.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ss
{
namespace ui
{
namespace rlrit_detail
{
const net::SiteId RitSite = net::SiteId::RunItTwice;
constexpr float RitRailW = 216.0f;   // the rail
constexpr float RitRailMini = 64.0f; // the rail at a table
constexpr float RitClientH = 962.0f; // the client above the taskbar
constexpr float RitTop = 84.0f;      // under the tape and the header
/** G1: the League is G2's, so its plate stays hidden (the others close up and number 1-8). */
constexpr bool RitLeagueOpen = false;

/** Everything a page draws with: the laptop, its session, the canvas and pointer, this laptop's state, the clocks. */
struct RitCtx
{
	RiverLine& RL;
	Session& S;
	Canvas& C;
	Ui& UI;
	RitUi& U;
	double Now = 0.0;
	double World = 0.0;
	bool Preview = false;
};

/** A plate on the rail: its page, its label, its nav id (the page's index: nav0..nav8). */
struct RitPlateDef
{
	RiverLine::Page P;
	const char* Label;
	int Nav;
};
const RitPlateDef RitPlates[9] = {{RiverLine::Page::Lobby, "BOARD", 0}, {RiverLine::Page::Live, "SUNDAY LIVE", 1}, {RiverLine::Page::League, "LEAGUE", 2},
	{RiverLine::Page::Leaderboards, "RANKINGS", 3}, {RiverLine::Page::Desk, "THE DESK", 4}, {RiverLine::Page::Series, "SERIES", 5},
	{RiverLine::Page::News, "NEWSDESK", 6}, {RiverLine::Page::Promotions, "CLUB", 7}, {RiverLine::Page::Cashier, "CASHIER", 8}};

/** The section slab's words for a page (§3.4 Header). */
std::string RitPageName(RiverLine::Page P)
{
	switch (P)
	{
	case RiverLine::Page::Lobby: return "THE BOARD";
	case RiverLine::Page::Live: return "SUNDAY LIVE";
	case RiverLine::Page::League: return "THE LEAGUE";
	case RiverLine::Page::Leaderboards: return "RANKINGS";
	case RiverLine::Page::Desk: return "THE DESK";
	case RiverLine::Page::Series: return "SERIES";
	case RiverLine::Page::News: return "NEWSDESK";
	case RiverLine::Page::Promotions: return "THE CLUB";
	case RiverLine::Page::Cashier: return "CASHIER";
	case RiverLine::Page::Profile: return "MEMBER FILE";
	default: break;
	}
	return "THE BOARD";
}

/** The page a RunItTwice client shows for P at G1: its own nine (League hidden), the Member File; anything else, the Board. */
RiverLine::Page RitPageOf(RiverLine::Page P)
{
	switch (P)
	{
	case RiverLine::Page::Lobby:
	case RiverLine::Page::Live:
	case RiverLine::Page::Leaderboards:
	case RiverLine::Page::Desk:
	case RiverLine::Page::Series:
	case RiverLine::Page::News:
	case RiverLine::Page::Promotions:
	case RiverLine::Page::Cashier:
	case RiverLine::Page::Profile: return P;
	case RiverLine::Page::League: return RitLeagueOpen ? P : RiverLine::Page::Lobby;
	default: break;
	}
	return RiverLine::Page::Lobby;
}

/** A sound the frame crossed (never in the Kast preview, U1 and §1.11: a cue plays on a click or a stage crossed). */
void RitSound(RitCtx& X, SoundId Id, float Vol)
{
	if (!X.Preview)
	{
		X.S.UiSound(Id, Vol);
	}
}

/** The 24-hour clock of a world minute ("21:22"). */
std::string RitClock(double World)
{
	const long long M = static_cast<long long>(std::floor(std::fmod(World, net::MinutesPerDay)));
	const long long H = M / 60;
	const long long Mm = M % 60;
	return (H < 10 ? "0" : "") + std::to_string(H) + ":" + (Mm < 10 ? "0" : "") + std::to_string(Mm);
}

/** A countdown in the Board's words: "0:08", "1:38", "16H 29M", "1D 21H". */
std::string RitCountdown(double Minutes)
{
	const long long M = std::max(0LL, static_cast<long long>(std::floor(Minutes)));
	if (M >= 24 * 60)
	{
		return std::to_string(M / (24 * 60)) + "D " + std::to_string((M % (24 * 60)) / 60) + "H";
	}
	const long long Mm = M % 60;
	if (M >= 10 * 60)
	{
		// Ten hours or more out (a WATCH chip at night): "16:29" would read as a time on the site's 24-hour clock (g1-audit).
		return std::to_string(M / 60) + "H " + (Mm < 10 ? "0" : "") + std::to_string(Mm) + "M";
	}
	return std::to_string(M / 60) + ":" + (Mm < 10 ? "0" : "") + std::to_string(Mm);
}

/** A buy-in as the Board prints it: "$5.50", "$22.00", "$109", "$530". */
std::string RitBuyIn(Chips Cents)
{
	return Cents < 10000 ? Money(Cents) : rlnet_detail::NetMoney(Cents);
}

/** "Fri" and "13 NOV" for a world day. */
std::string RitDay(int Day)
{
	return RitUpper(net::WeekdayName(Day));
}

std::string RitDate(int Day)
{
	return RitUpper(net::DateLabel(Day));
}

/** The world minute the night session ends: the next 6 AM (the Night Shift's close). */
double RitNightEnds(double World)
{
	const double Midnight = std::floor(World / net::MinutesPerDay) * net::MinutesPerDay;
	const double SixAm = Midnight + 6.0 * 60.0;
	return World < SixAm ? SixAm : SixAm + net::MinutesPerDay;
}

/** Night service (§3.4 A): after 1 AM, until the Night Shift closes at 6. */
bool RitNightService(double World)
{
	const double Hour = std::fmod(World, net::MinutesPerDay) / 60.0;
	return Hour >= 1.0 && Hour < 6.0;
}

/** The player's RunItTwice account (null: none). */
const site::Account* RitAccount(const Session& S)
{
	return rlsites_detail::SitesAccount(S, RitSite);
}

/** The Club's tier name and rate ("Member", "15%"). */
std::string RitTierName(int Tier)
{
	const std::vector<site::TierInfo>& Club = site::SecondRunTiers();
	return Tier > 0 && Tier < static_cast<int>(Club.size()) ? std::string(Club[static_cast<size_t>(Tier)].Name) : std::string("No tier");
}

std::string RitTierPct(int Tier)
{
	const std::vector<site::TierInfo>& Club = site::SecondRunTiers();
	if (Tier <= 0 || Tier >= static_cast<int>(Club.size()))
	{
		return "0%";
	}
	const int P = Club[static_cast<size_t>(Tier)].Permille;
	return Fixed(static_cast<double>(P) / 10.0, P % 10 == 0 ? 0 : 1) + "%";
}

/** This month's accrual (site::show::ClubAccrual), or nothing without an account. */
site::show::Accrual RitAccrual(const Session& S)
{
	const site::Account* A = RitAccount(S);
	return A ? site::show::ClubAccrual(*A) : site::show::Accrual();
}

/** "$37.60 more to Regular (20%)", or what the top or no tier says. */
std::string RitToNextLine(const site::show::Accrual& Ac)
{
	if (Ac.Next > Ac.Tier && Ac.ToNext > 0)
	{
		return Money(Ac.ToNext) + " more to " + RitTierName(Ac.Next) + " (" + RitTierPct(Ac.Next) + ")";
	}
	return Ac.Tier > 0 ? std::string(RitTopTier) : std::string(RitNoClub);
}

/** A template id's family: "runittwice.the-nightcap@1530" -> "runittwice.the-nightcap". */
std::string RitTemplateOf(const std::string& EventId)
{
	const size_t At = EventId.find('@');
	return At == std::string::npos ? EventId : EventId.substr(0, At);
}

/** A table's event as the Exchange names it: the listing's "$16.50 The Long Game" without its price (the Board prints it). */
std::string RitEventName(const std::string& Name)
{
	const std::string Short = rlnet_detail::NetShortName(Name);
	if (!Short.empty() && Short[0] == '$')
	{
		const size_t Space = Short.find(' ');
		return Space == std::string::npos ? Short : Short.substr(Space + 1);
	}
	return Short;
}

/** The tags after an event's name: what kind of night it is. */
std::string RitTagOf(const net::EventTemplate& T)
{
	if (!T.Series.empty())
	{
		return "SERIES";
	}
	if (T.BuyInCents >= 20000)
	{
		return "HIGH ROLLER";
	}
	if (T.Featured)
	{
		return "FEATURED";
	}
	if (T.Fmt == net::Format::Bounty)
	{
		return "PKO";
	}
	if (T.Fmt == net::Format::Mystery)
	{
		return "MYSTERY";
	}
	if (T.TableSize == 6)
	{
		return "6-MAX";
	}
	if (T.Speed != "Regular")
	{
		return RitUpper(T.Speed);
	}
	return std::string();
}

/** An event's status as the Board's flap reads it (§3.4 A, Statuses). */
struct RitStatus
{
	std::string Text;
	RitTile Tile = RitTile::Gloss;
	uint32_t Ink = RitInk;
	bool Final = false; // FINAL CALL (it blinks)
	bool Dim = false;   // SCHEDULED, ARRIVED, LOCKED: the row's words in Muted
};
RitStatus RitStatusOf(const Session& S, const net::EventInstance& E, const net::LiveState& L, bool Joinable)
{
	RitStatus St;
	if (S.IsPlaying(E.Id))
	{
		St.Text = S.T && S.T->Spec.Id == E.Id && S.TableId > 0 ? "SEATED T" + std::to_string(S.TableId) : std::string("SEATED");
		St.Tile = RitTile::Chrome;
		St.Ink = RitOnyx;
		return St;
	}
	switch (L.St)
	{
	case net::Status::Announced: St.Text = "SCHEDULED"; St.Ink = RitMuted; St.Dim = true; break;
	case net::Status::Registering: St.Text = "BOARDING"; break;
	case net::Status::LateReg:
		if (L.LateRegLeft <= 10.0)
		{
			St.Text = "FINAL CALL";
			St.Tile = RitTile::Red;
			St.Ink = RitWhite;
			St.Final = true;
		}
		else
		{
			St.Text = "BOARDING";
		}
		break;
	case net::Status::Running:
	case net::Status::FinalTable: St.Text = "DEPARTED"; St.Ink = RitChrome; break;
	case net::Status::Finished: St.Text = "ARRIVED"; St.Ink = RitMuted; St.Dim = true; break;
	}
	const bool Open = L.St == net::Status::Announced || L.St == net::Status::Registering || L.St == net::Status::LateReg;
	if (Open && !Joinable)
	{
		St.Text = "LOCKED";
		St.Tile = RitTile::Gloss;
		St.Ink = RitMuted;
		St.Final = false;
		St.Dim = true;
	}
	return St;
}

/** The pointer: a click inside R this frame (immediate mode, through the laptop's Ui). */
Ui::ClickState RitClick(RitCtx& X, const std::string& Id, const Rect& R, bool Enabled = true)
{
	return X.UI.Clickable(Id, R, Enabled && !X.Preview);
}

// ------------------------------------------------------------------ the rail (§3.4 Chrome)

/** The open tables the rail lists (Seated): the session's, in order. */
std::vector<rlsites_detail::StripRow> RitSeated(RitCtx& X)
{
	return rlsites_detail::SitesTableStrip(X.RL, X.S);
}

void RitRailPlates(RitCtx& X, bool Collapsed)
{
	const RiverLine::Page Shown = X.RL.CurrentPage();
	const bool InLobby = X.S.CurrentScreen == Screen::Lobby;
	const bool OnResults = X.S.CurrentScreen == Screen::Results; // the Settlement: a plate leaves it as BACK TO THE BOARD does
	const bool Navigable = InLobby || OnResults || (X.S.CurrentScreen == Screen::Table && X.S.T);
	const net::Network& Net = net::Site(RitSite);
	const net::SeriesInfo* Sr = Net.CurrentSeries(X.World);
	const bool SeriesOn = Sr && net::DayOf(X.World) >= Sr->FirstDay;
	// Sunday Live's bug: a major is running today (Sunday's 100K, Grind or High Bridge in play).
	bool MajorLive = false;
	for (const net::EventInstance& E : Net.Window(X.World - 14.0 * 60.0, X.World))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::Status St = Net.Live(E, X.World).St;
		MajorLive = MajorLive || (T.Featured && (T.Days == (1 << 6) || T.Main) && (St == net::Status::LateReg || St == net::Status::Running || St == net::Status::FinalTable));
	}
	int Unread = 0;
	for (const net::NewsItem& It : Access::NewsFeed(X.RL))
	{
		Unread += It.At > Access::NewsSeenAt(X.RL) ? 1 : 0;
	}
	const bool Transit = rlsites_detail::SitesInTransit(X.S, RitSite) > 0;
	const float Light = static_cast<float>(Clamp01((X.Now - Access::PageAt(X.RL)) / 0.16));
	int Num = 0;
	for (const RitPlateDef& D : RitPlates)
	{
		if (D.P == RiverLine::Page::League && !RitLeagueOpen)
		{
			continue;
		}
		const float Y = 84.0f + 40.0f * static_cast<float>(Num);
		const Rect R = Collapsed ? Rect{10.0f, Y, 44.0f, 36.0f} : Rect{8.0f, Y, 200.0f, 36.0f};
		const Ui::ClickState St = RitClick(X, "nav" + std::to_string(D.Nav), R, Navigable);
		const bool Active = InLobby && Shown == D.P;
		if (Active)
		{
			// The plate's light: a Signal Red underglow crossing the rail's hairline into the content margin (x 208-232).
			RitGlow(X.C, Collapsed ? Rect{6.0f, Y + 2.0f, 70.0f, 32.0f} : Rect{4.0f, Y + 2.0f, 236.0f, 32.0f}, RitRed, 0.45f * (X.Preview ? 1.0f : Light), 18.0f);
			RitCutFill(X.C, R, 6.0f, RitC(RitRed));
			RitCutFill(X.C, {R.X, R.Y, R.W, R.H / 2.0f}, 6.0f, RitC(RitWhite, 0.06f));
			RitLedEdge(X.C, R.X, R.Y, R.W - 6.0f);
			if (!Collapsed)
			{
				RitTwinV(X.C, 217.0f, Y + 2.0f, 32.0f); // the twin stripe lights the plate's row where its light lands
			}
		}
		else
		{
			RitCutFill(X.C, R, 6.0f, RitGlossPaint(R));
			RitCutStroke(X.C, R, 6.0f, RitC(RitChrome, 0.35f), 1.0f);
		}
		if (St.Hover && !X.Preview)
		{
			const double T = std::fmod(X.Now, 1.4) / 0.4;
			if (T < 1.0)
			{
				RitSweep(X.C, R, static_cast<float>(-0.2 + 1.4 * T), 30.0f);
			}
		}
		RitRaceBox(X.C, R.X + (Collapsed ? 11.0f : 8.0f), Y + 7.0f, Num + 1, Active);
		if (!Collapsed)
		{
			RitText(X.C, D.Label, 48.0f, Y + 18.0f, 15.0f, RitC(Active ? RitWhite : RitInk), RitFace::Cond, Align::Left, Baseline::Middle);
			if (D.P == RiverLine::Page::Live && MajorLive)
			{
				// The LIVE bug: Hot White on Signal Red, its dot blinking at 1 Hz.
				RitCutFill(X.C, {156.0f, Y + 10.0f, 42.0f, 16.0f}, 4.0f, RitC(Active ? RitDeep : RitRed));
				if (X.Preview || std::fmod(X.Now, 1.0) < 0.5)
				{
					X.C.FillCircle(164.0f, Y + 18.0f, 2.6f, RitC(RitWhite));
				}
				RitText(X.C, "LIVE", 182.0f, Y + 18.0f, 11.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
			}
			else if (D.P == RiverLine::Page::Series && SeriesOn)
			{
				RitTag(X.C, 154.0f, Y + 10.0f, "OPEN", Active ? RitWhite : RitChrome, Active ? RitWhite : RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, true);
			}
			else if (D.P == RiverLine::Page::News && Unread > 0)
			{
				X.C.FillRoundRect({176.0f, Y + 10.0f, 22.0f, 16.0f}, 8.0f, RitC(RitWhite));
				// The count as it is ("9+" past nine: a capped "9" would be a number that isn't true; g1-audit, as Ignite's bell).
				RitText(X.C, Unread > 9 ? std::string("9+") : std::to_string(Unread), 187.0f, Y + 18.0f, Unread > 9 ? 11.0f : 12.0f, RitC(RitOnyx), RitFace::Cond, Align::Center,
					Baseline::Middle);
			}
			else if (D.P == RiverLine::Page::Cashier && Transit)
			{
				X.C.FillCircle(190.0f, Y + 18.0f, 4.0f, RitC(RitAmber));
			}
		}
		if (St.Clicked)
		{
			if (OnResults)
			{
				X.S.LeaveResults();
			}
			X.S.ShowLobby();
			X.RL.SiteShown = RitSite; // at a table, its site's pages
			X.RL.OpenPage(D.P, X.Now);
		}
		++Num;
	}
}

void RitRailSeated(RitCtx& X, bool Collapsed)
{
	const std::vector<rlsites_detail::StripRow> Rows = RitSeated(X);
	const bool Blink = X.Preview || std::fmod(X.Now, 1.0) < 0.5;
	if (Collapsed)
	{
		X.C.FillRect({8.0f, 452.0f, 48.0f, 1.0f}, RitC(RitHair));
		for (size_t I = 0; I < Rows.size() && I < 4; ++I)
		{
			const rlsites_detail::StripRow& Row = Rows[I];
			const float Y = 470.0f + 52.0f * static_cast<float>(I);
			const Rect R{12.0f, Y, 40.0f, 40.0f};
			const bool Here = X.S.FocusedTable() == Row.Table;
			const Ui::ClickState St = RitClick(X, "rtt" + std::to_string(I), R);
			RitCutFill(X.C, R, 6.0f, RitC(RitCarbon));
			RitCutStroke(X.C, R, 6.0f, RitC(Here ? RitWhite : RitHair), 1.0f);
			RitText(X.C, "T" + std::to_string(Row.Table + 1), 32.0f, Y + 16.0f, 12.0f, RitC(RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
			RitText(X.C, Fixed(std::floor(Row.StackBb), 0), 32.0f, Y + 30.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Center, Baseline::Middle);
			if (Row.YourTurn)
			{
				X.C.FillCircle(50.0f, Y + 3.0f, 7.0f, RitC(RitRed, 0.35f));
				if (Blink)
				{
					X.C.FillCircle(50.0f, Y + 3.0f, 4.5f, RitC(RitRed));
				}
			}
			else if (!Here)
			{
				X.C.FillCircle(50.0f, Y + 3.0f, 4.0f, RitC(RitUp));
			}
			if (Here)
			{
				X.C.FillRect({12.0f, Y + 38.0f, 40.0f, 2.0f}, RitC(RitWhite));
			}
			if (St.Clicked)
			{
				X.S.ShowTables();
				X.S.FocusTable(Row.Table);
			}
		}
		return;
	}
	RitLabel(X.C, "Seated", 16.0f, 484.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, std::to_string(Rows.size()), 200.0f, 484.0f, 12.0f, RitC(RitMuted), RitFace::Cond, Align::Right);
	if (Rows.empty())
	{
		RitCutFill(X.C, {8.0f, 494.0f, 200.0f, 52.0f}, 6.0f, RitC(RitCarbon));
		RitCutStroke(X.C, {8.0f, 494.0f, 200.0f, 52.0f}, 6.0f, RitC(RitHair), 1.0f);
		RitText(X.C, RitNoTables, 22.0f, 516.0f, 13.0f, RitC(RitMuted), RitFace::Body);
		RitText(X.C, RitBoardOpen, 22.0f, 534.0f, 13.0f, RitC(RitMuted), RitFace::Body);
		return;
	}
	const bool InBb = rlsites_detail::SitesBetsInBb(X.RL, rlsites_detail::SitesLook(RitSite));
	for (size_t I = 0; I < Rows.size() && I < 4; ++I)
	{
		const rlsites_detail::StripRow& Row = Rows[I];
		const TableGlance G = X.S.Glance(Row.Table);
		// Four rows end at y 696, above the member block's rule (700).
		const float Y = 490.0f + 52.0f * static_cast<float>(I);
		const Rect R{8.0f, Y, 200.0f, 50.0f};
		const Ui::ClickState St = RitClick(X, "rtt" + std::to_string(I), R);
		RitCutFill(X.C, R, 6.0f, RitC(St.Hover ? RitPanel2 : RitCarbon));
		RitCutStroke(X.C, R, 6.0f, RitC(RitHair), 1.0f);
		if (Row.YourTurn)
		{
			// Your turn: the row's left edge lit with the twin stripe, ACTION blinking at 1 Hz (Hot White on Signal Red).
			RitTwinV(X.C, 8.0f, Y, 50.0f, 0.5f);
			RitCutFill(X.C, {144.0f, Y + 7.0f, 56.0f, 17.0f}, 4.0f, RitC(Blink ? RitRed : RitFinalDim));
			RitText(X.C, "ACTION", 172.0f, Y + 15.5f, 11.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
		}
		else
		{
			X.C.FillRect({16.0f, Y + 9.0f, 3.0f, 34.0f}, RitC(RitUp)); // waiting on the others
		}
		RitText(X.C, RitUpper(RitEventName(Row.Short)), 24.0f, Y + 21.0f, 15.0f, RitC(RitInk), RitFace::Cond, Align::Left, Baseline::Alphabetic, Row.YourTurn ? 116.0f : 176.0f);
		const std::string Where = G.Busted ? std::string("OUT \xC2\xB7 RESULTS POSTING") : Ordinal(G.Rank) + " OF " + Grouped(G.Remaining);
		RitText(X.C, Where, 24.0f, Y + 42.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Left, Baseline::Alphabetic, 110.0f);
		RitText(X.C, InBb ? Fixed(Row.StackBb, Row.StackBb < 100.0 ? 1 : 0) + " BB" : ChipsText(static_cast<double>(G.Stack)), 200.0f, Y + 42.0f, 15.0f, RitC(RitWhite),
			RitFace::Cond, Align::Right);
		if (St.Clicked)
		{
			X.S.ShowTables();
			X.S.FocusTable(Row.Table);
		}
	}
}

void RitRailMember(RitCtx& X)
{
	const site::Account* A = RitAccount(X.S);
	const site::show::Accrual Ac = RitAccrual(X.S);
	X.C.FillRect({8.0f, 700.0f, 200.0f, 1.0f}, RitC(RitHair));
	RitSquareAvatar(X.C, 20.0f, 714.0f, 56.0f, X.S.HeroName, RitC(RitChrome));
	RitText(X.C, X.S.HeroName, 88.0f, 730.0f, 15.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 116.0f);
	// The Club tier as a mini card chip with its rate (the League's division and place join it at G2).
	X.C.FillRect({88.0f, 742.0f, 16.0f, 11.0f}, RitC(Ac.Tier >= 2 ? RitChromeMid : RitPanel2));
	X.C.StrokePolyline({{88.5f, 742.5f}, {103.5f, 742.5f}, {103.5f, 752.5f}, {88.5f, 752.5f}}, true, RitC(RitChromeDark), 0.8f);
	RitLabel(X.C, Ac.Tier > 0 ? RitTierName(Ac.Tier) + " \xC2\xB7 " + RitTierPct(Ac.Tier) : std::string("Club \xC2\xB7 no tier yet"), 110.0f, 752.0f, 12.0f, RitC(RitInk));
	const net::HeroStats Me = rlsites_detail::SitesHero(X.S, RitSite, net::Site(RitSite), X.World, net::Board::Season);
	RitLabel(X.C, "Season \xC2\xB7 " + Grouped(static_cast<int64_t>(std::llround(Me.SeasonPoints))) + " pts", 88.0f, 774.0f, 13.0f, RitC(RitChrome));
	(void)A;
	X.C.FillRect({20.0f, 796.0f, 176.0f, 1.0f}, RitC(RitHair));
	RitTwice(X.C, RitClock(X.World), 20.0f, 840.0f, 30.0f, RitC(RitWhite));
	const int Today = net::DayOf(X.World);
	RitText(X.C, RitDay(Today), 196.0f, 824.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	RitText(X.C, RitDate(Today), 196.0f, 840.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	const Rect File{12.0f, 860.0f, 192.0f, 24.0f};
	const Ui::ClickState St = RitClick(X, "ritfile", File);
	const bool Here = X.S.CurrentScreen == Screen::Lobby && X.RL.CurrentPage() == RiverLine::Page::Profile;
	RitText(X.C, "Member file", 20.0f, 876.0f, 13.0f, RitC(Here ? RitWhite : St.Hover ? RitInk : RitMuted), Here ? RitFace::Name : RitFace::Body);
	X.C.StrokePolyline({{192.0f, 868.0f}, {196.0f, 872.0f}, {192.0f, 876.0f}}, false, RitC(RitMuted), 1.4f);
	if (St.Clicked)
	{
		if (X.S.CurrentScreen == Screen::Results)
		{
			X.S.LeaveResults(); // from the Settlement, as BACK TO THE BOARD leaves it (else the click would land on nothing)
		}
		X.RL.OpenPage(RiverLine::Page::Profile, X.Now);
	}
	RitText(X.C, RitNoSpins, 20.0f, 922.0f, 12.0f, RitC(RitMuted), RitFace::Body);
	RitText(X.C, RitRakeBack, 20.0f, 940.0f, 12.0f, RitC(RitMuted), RitFace::Body);
}

void RitRail(RitCtx& X, bool Collapsed)
{
	const float W = Collapsed ? RitRailMini : RitRailW;
	RitPanelBegin(); // the rail is one panel (its active plate's twin stripe is its one)
	X.C.FillRect({0.0f, 0.0f, W, RitClientH}, RitC(RitBg2));
	RitTwill(X.C, {0.0f, 0.0f, W, RitClientH});
	X.C.FillRect({W - 1.0f, 0.0f, 1.0f, RitClientH}, RitC(RitHair));
	siteart::Logo(X.C, Collapsed ? Rect{12.0f, 14.0f, 40.0f, 40.0f} : Rect{16.0f, 14.0f, 184.0f, 44.0f}, RitSite);
	RitRailPlates(X, Collapsed);
	RitRailSeated(X, Collapsed);
	if (!Collapsed)
	{
		RitRailMember(X);
	}
}

// ------------------------------------------------------------------ the red tape (§3.4 Ticker)

/** The tape's items: world facts only, rebuilt each world minute (W5: nothing invented, no hashed counts). */
std::vector<RitTapeItem> RitTapeBuild(RitCtx& X)
{
	std::vector<RitTapeItem> Out = X.U.Extra; // the tables' own lines first: the latest wins
	const net::Network& Net = net::Site(RitSite);
	// Your tables: players left, and where you stand.
	for (const rlsites_detail::StripRow& Row : RitSeated(X))
	{
		const TableGlance G = X.S.Glance(Row.Table);
		if (Row.Site == RitSite && !G.Busted)
		{
			Out.push_back({"left:" + G.EventId, RitUpper(RitEventName(Row.Short)) + " \xC2\xB7 " + Grouped(G.Remaining) + " LEFT \xC2\xB7 YOU " + RitUpper(Ordinal(G.Rank)), RitInk, 0});
		}
	}
	// The running events (the Board's DEPARTED: past late registration), how many are seated in them (the KPI tile's count),
	// and the biggest field in play.
	int Running = 0;
	int Seated = 0;
	const net::EventInstance* Biggest = nullptr;
	net::LiveState BigL;
	const std::vector<net::EventInstance> Recent = Net.Window(X.World - 16.0 * 60.0, X.World + 12.0 * 60.0);
	for (const net::EventInstance& E : Recent)
	{
		const net::LiveState L = Net.Live(E, X.World);
		if (L.St == net::Status::Running || L.St == net::Status::FinalTable || L.St == net::Status::LateReg)
		{
			if (L.St != net::Status::LateReg)
			{
				++Running;
				Seated += L.Left;
			}
			if (!Biggest || L.Entries > BigL.Entries)
			{
				Biggest = &E;
				BigL = L;
			}
		}
	}
	if (Running > 0)
	{
		Out.push_back({"running", Grouped(Running) + " RUNNING \xC2\xB7 " + Grouped(Seated) + " PLAYERS SEATED", RitInk, 0});
	}
	if (Biggest)
	{
		const net::EventTemplate& T = Net.TemplateOf(*Biggest);
		std::string Line = RitUpper(rlnet_detail::NetShortName(T.Name)) + " \xC2\xB7 " + Grouped(BigL.Entries) + " ENTRIES";
		if (T.GtdCents > 0)
		{
			// SHORT and MET are facts only once late registration has closed, or when the entries so far already cover it.
			const Chips Each = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T)));
			const bool Closed = BigL.St != net::Status::LateReg;
			const bool Covered = Each * static_cast<Chips>(BigL.Entries) >= T.GtdCents;
			Line += " \xC2\xB7 GTD " + rlnet_detail::NetMoney(T.GtdCents) + (BigL.Overlay ? " SHORT" : Closed || Covered ? " MET" : "");
		}
		Out.push_back({"big:" + Biggest->Id, Line, RitInk, 0});
	}
	// Projected overlays, the biggest two (amber: a money opportunity, never red).
	std::vector<std::pair<Chips, std::string>> Shorts;
	for (const net::EventInstance& E : Recent)
	{
		const site::show::OverlayView V = site::show::OverlayOf(Net, E, X.World);
		if (V.Shown && !V.Actual && V.Cents > 0)
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			Shorts.push_back({V.Cents, "ov:" + E.Id});
			(void)T;
		}
	}
	std::sort(Shorts.begin(), Shorts.end(), [](const auto& A, const auto& B) { return A.first > B.first; });
	for (size_t I = 0; I < Shorts.size() && I < 2; ++I)
	{
		net::EventInstance E;
		if (Net.FindInstance(Shorts[I].second.substr(3), E))
		{
			const net::EventTemplate& T = Net.TemplateOf(E);
			Out.push_back({Shorts[I].second, RitUpper(rlnet_detail::NetShortName(T.Name)) + " \xC2\xB7 PROJ. SHORT " + Money(Shorts[I].first) + " \xC2\xB7 REG CLOSES " + RitClock(E.Start + E.LateReg),
				RitAmber, 0});
		}
	}
	// Team RunItTwice's pros in tonight's events (the world's registrations).
	const std::string Team = net::Def(RitSite).TeamName;
	const world::World& W = X.S.Living();
	int TeamLines = 0;
	for (const net::EventInstance& E : Recent)
	{
		const net::LiveState L = Net.Live(E, X.World);
		if (TeamLines >= 2 || !(L.St == net::Status::Registering || L.St == net::Status::LateReg || L.St == net::Status::Running))
		{
			continue;
		}
		for (const int Id : W.RegisteredNow(E.Id))
		{
			const world::Npc* N = W.Get(Id);
			if (N && N->Sponsor == Team && TeamLines < 2)
			{
				Out.push_back({"team:" + std::to_string(Id), "TEAM \xC2\xB7 " + RitUpper(N->Name) + " SEATED IN " + RitUpper(rlnet_detail::NetShortName(Net.TemplateOf(E).Name)), RitInk, 0});
				++TeamLines;
			}
		}
	}
	// Big results off the site's own news.
	int Wins = 0;
	for (const net::NewsItem& It : Access::NewsFeed(X.RL))
	{
		if (Wins < 2 && It.Kind == net::NewsKind::BigWin && X.World - It.At < 12.0 * 60.0)
		{
			Out.push_back({"news:" + std::to_string(static_cast<long long>(It.At)) + ":" + It.Title, RitUpper(It.Title) + (It.Amount > 0 ? " \xC2\xB7 " + Money(It.Amount) : std::string()), RitInk, 0});
			++Wins;
		}
	}
	if (Out.empty())
	{
		Out.push_back({"quiet", "THE BOARD IS OPEN \xC2\xB7 " + RitClock(X.World), RitInk, 0});
	}
	return Out;
}

void RitTapeBar(RitCtx& X, float X0)
{
	const Rect R{X0, 0.0f, 1600.0f - X0, 32.0f};
	RitPanelBegin();
	X.C.FillRect(R, RitC(RitTape));
	RitScanlines(X.C, R, RitRed, 0.22f);
	X.C.FillRect({X0, 31.0f, R.W, 1.0f}, RitC(RitHair));
	// The LIVE bug.
	RitCutFill(X.C, {X0 + 8.0f, 7.0f, 48.0f, 18.0f}, 4.0f, RitC(RitRed));
	if (X.Preview || std::fmod(X.Now, 1.0) < 0.5)
	{
		X.C.FillCircle(X0 + 17.0f, 16.0f, 3.0f, RitC(RitWhite));
	}
	RitText(X.C, "LIVE", X0 + 38.0f, 16.0f, 12.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
	// The items, rebuilt once a world minute; a new item or a changed figure flashes (one at a time, 160 ms).
	const long long Key = static_cast<long long>(std::floor(X.World)) * 8 + (X.S.CurrentScreen == Screen::Table ? 1 : 0);
	if (Key != X.U.TapeKey)
	{
		const bool First = X.U.TapeSeen.empty();
		X.U.TapeKey = Key;
		X.U.Tape = RitTapeBuild(X);
		for (const RitTapeItem& It : X.U.Tape)
		{
			const auto Was = X.U.TapeSeen.find(It.Key);
			if (!First && (Was == X.U.TapeSeen.end() || Was->second != It.Text) && X.U.FlashQueue.size() < 6)
			{
				X.U.FlashQueue.push_back(It.Key);
			}
			X.U.TapeSeen[It.Key] = It.Text;
		}
	}
	if (!X.Preview && (X.U.FlashKey.empty() || X.Now - X.U.FlashAt > 0.16) && !X.U.FlashQueue.empty())
	{
		X.U.FlashKey = X.U.FlashQueue.front();
		X.U.FlashQueue.erase(X.U.FlashQueue.begin());
		X.U.FlashAt = X.Now;
	}
	const bool Flashing = !X.Preview && !X.U.FlashKey.empty() && X.Now - X.U.FlashAt <= 0.16;
	const Rect Lane{X0 + 64.0f, 0.0f, R.W - 64.0f, 32.0f};
	// 40 units a second, paused under the pointer (and still in the Kast preview).
	if (!X.Preview && !X.UI.Hover(Lane))
	{
		X.U.TapeX += Access::Dt(X.RL) * 40.0;
	}
	float Total = 0.0f;
	std::vector<float> Widths;
	for (const RitTapeItem& It : X.U.Tape)
	{
		Widths.push_back(RitMeasure(X.C, It.Text, 15.0f));
		Total += Widths.back() + 40.0f;
	}
	if (Total <= 0.0f)
	{
		return;
	}
	X.U.TapeX = std::fmod(X.U.TapeX, static_cast<double>(Total));
	X.C.PushClip(Lane);
	float Px = Lane.X + 12.0f - static_cast<float>(X.U.TapeX);
	for (int Rep = 0; Rep < 3 && Px < Lane.X + Lane.W; ++Rep)
	{
		for (size_t I = 0; I < X.U.Tape.size(); ++I)
		{
			const RitTapeItem& It = X.U.Tape[I];
			const bool Lit = Flashing && It.Key == X.U.FlashKey;
			if (Px + Widths[I] > Lane.X && Px < Lane.X + Lane.W)
			{
				if (Lit)
				{
					RitCutFill(X.C, {Px - 7.0f, 5.0f, Widths[I] + 14.0f, 22.0f}, 4.0f, RitC(RitRed));
				}
				RitText(X.C, It.Text, Px, 16.0f, 15.0f, RitC(Lit ? RitWhite : It.Rgb), RitFace::Cond, Align::Left, Baseline::Middle);
			}
			Px += Widths[I] + 18.0f;
			RitTwinTick(X.C, Px - 4.0f, 9.0f, 14.0f, RitC(RitRed));
			Px += 22.0f;
		}
	}
	X.C.PopClip();
}

// ------------------------------------------------------------------ the header (§3.4 Header) and the section slab

void RitSectionSlab(RitCtx& X, float X0, const std::string& Name)
{
	X.C.FillPolygon({{X0, 32.0f}, {X0 + 300.0f, 32.0f}, {X0 + 283.0f, 84.0f}, {X0, 84.0f}}, RitC(RitRed));
	X.C.FillPolygon({{X0, 32.0f}, {X0 + 300.0f, 32.0f}, {X0 + 291.5f, 58.0f}, {X0, 58.0f}}, RitC(RitWhite, 0.05f));
	RitLedEdge(X.C, X0, 32.0f, 300.0f);
	// The page's name in BoldCondensed 30 caps, Hot White, twice-printed in Deep Red; a long name (a table's event) fits down
	// to 22, below 28 prints once, then ends with an ellipsis (the text box is x0+16 to x0+275).
	const std::string Words = RitUpper(Name);
	const float Size = RitFit(X.C, Words, 30.0f, 22.0f, 259.0f);
	const float Base = 58.0f + Size * 0.36f;
	if (Size >= RitTwiceFrom)
	{
		RitTwice(X.C, Words, X0 + 16.0f, Base, Size, RitC(RitWhite), RitDeep);
	}
	else
	{
		RitText(X.C, Words, X0 + 16.0f, Base, Size, RitC(RitWhite), RitFace::Cond, Align::Left, Baseline::Alphabetic, 259.0f);
	}
	RitTwinSlash(X.C, X0 + 304.0f, 32.0f, 52.0f);
}

void RitHeaderBar(RitCtx& X, float X0, const std::string& Name, bool AtTable)
{
	RitPanelBegin();
	X.C.FillRect({X0, 32.0f, 1600.0f - X0, 52.0f}, RitC(RitOnyx));
	X.C.FillRect({X0, 83.0f, 1600.0f - X0, 1.0f}, RitC(RitHair));
	RitSectionSlab(X, X0, Name);
	const net::Network& Net = net::Site(RitSite);
	if (!AtTable)
	{
		// The WATCH chips (G1: tonight's next three featured events with their countdowns; the Watchlist's stars are G2).
		std::vector<net::EventInstance> Next;
		for (const net::EventInstance& E : Net.Window(X.World, X.World + 24.0 * 60.0))
		{
			if (Next.size() < 3 && E.Start > X.World && Net.TemplateOf(E).Featured)
			{
				Next.push_back(E);
			}
		}
		for (size_t I = 0; I < Next.size() && I < 3; ++I)
		{
			const net::EventInstance& E = Next[I];
			const net::EventTemplate& T = Net.TemplateOf(E);
			const Rect R{548.0f + static_cast<float>(I) * 138.0f - (RitRailW - X0), 42.0f, 132.0f, 32.0f};
			const Ui::ClickState St = RitClick(X, "watch" + std::to_string(I), R);
			const bool Seated = X.S.IsPlaying(E.Id);
			RitCutFill(X.C, R, 6.0f, RitC(St.Hover ? RitPanel2 : RitCarbon));
			RitCutStroke(X.C, R, 6.0f, RitC(RitHair), 1.0f);
			RitStar(X.C, R.X + 11.0f, R.Y + 16.0f, 5.5f, false);
			const std::string When = Seated ? std::string("SEATED") : RitCountdown(E.Start - X.World);
			const float Tw = RitMeasure(X.C, When, 12.0f);
			RitText(X.C, When, R.X + R.W - 7.0f, R.Y + 16.0f, 12.0f, RitC(Seated ? RitChrome : RitMuted), RitFace::Cond, Align::Right, Baseline::Middle);
			RitText(X.C, RitUpper(rlnet_detail::NetShortName(T.Name)), R.X + 19.0f, R.Y + 16.0f, 12.0f, RitC(RitInk), RitFace::Cond, Align::Left, Baseline::Middle, R.W - 32.0f - Tw);
			if (St.Clicked)
			{
				if (X.S.CurrentScreen == Screen::Results)
				{
					X.S.LeaveResults(); // from the Settlement, as BACK TO THE BOARD leaves it
				}
				X.RL.OpenPage(RiverLine::Page::Lobby, X.Now);
				X.RL.SelectEvent(E.Id);
				RitSound(X, SoundId::RitBlip, 0.3f);
			}
		}
	}
	// CLUB +$31.86 (this month's accrual) and the balance, one run (a test pin).
	const std::string Bal = "Balance " + Money(rlsites_detail::SitesBalance(X.S, RitSite));
	const float Bw = RitText(X.C, Bal, 1576.0f, 58.0f, 20.0f, RitC(RitWhite), RitFace::Cond, Align::Right, Baseline::Middle);
	const float Sep = 1576.0f - Bw - 14.0f;
	X.C.FillRect({Sep, 46.0f, 1.0f, 24.0f}, RitC(RitHair));
	RitText(X.C, "CLUB +" + Money(RitAccrual(X.S).Accrued), Sep - 14.0f, 58.0f, 14.0f, RitC(RitChrome), RitFace::Mono, Align::Right, Baseline::Middle);
}

// ------------------------------------------------------------------ THE BOARD (Page::Lobby, §3.4 A)

/** The Board's columns (x: a figure's right edge, or a cell's left). */
constexpr float RitColTime = 260.0f;
constexpr float RitColEvent = 322.0f;
constexpr float RitColBuy = 610.0f;
constexpr float RitColGtd = 694.0f;
constexpr float RitColField = 728.0f;
constexpr float RitColBar = 734.0f;
constexpr float RitColOver = 860.0f;
constexpr float RitColPts = 922.0f;
constexpr float RitColStatus = 972.0f;
constexpr float RitRowPitch = 34.0f;
const Rect RitGrid{232.0f, 252.0f, 880.0f, 680.0f};

/** The Board's rows: the shared schedule's (the gatekeeper), the Board's own two toggles on top, in the chosen order. */
std::vector<RitRow> RitBoardRows(RitCtx& X)
{
	std::vector<RitRow> Rows = Access::ScheduleRows(X.RL);
	const net::Network& Net = net::Site(RitSite);
	if (X.U.GtdOnly || X.U.LateOnly)
	{
		Rows.erase(std::remove_if(Rows.begin(), Rows.end(),
					   [&](const RitRow& R) { return (X.U.GtdOnly && Net.TemplateOf(R.E).GtdCents <= 0) || (X.U.LateOnly && R.L.St != net::Status::LateReg); }),
			Rows.end());
	}
	if (X.U.Sort > 0)
	{
		std::vector<std::pair<double, size_t>> Keyed;
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			const net::EventTemplate& T = Net.TemplateOf(Rows[I].E);
			double K = 0.0;
			switch (X.U.Sort)
			{
			case 1: K = static_cast<double>(T.BuyInCents); break;
			case 2: K = static_cast<double>(T.GtdCents); break;
			case 3: K = static_cast<double>(Rows[I].L.Entries); break;
			default: K = static_cast<double>(site::show::OverlayOf(Net, Rows[I].E, X.World).Cents); break;
			}
			Keyed.push_back({K, I});
		}
		std::stable_sort(Keyed.begin(), Keyed.end(), [](const auto& A, const auto& B) { return A.first > B.first; });
		std::vector<RitRow> Sorted;
		for (const auto& K : Keyed)
		{
			Sorted.push_back(Rows[K.second]);
		}
		Rows.swap(Sorted);
	}
	return Rows;
}

/**
 * Tonight's arrivals (§3.4 A, g1-audit L2): with the RUNNING filter on, the events that have finished since the night began
 * (the last 6 AM), the latest first (at most 12), under the departures. The schedule's rows (ScheduleRows) are open and
 * running events only, so these aren't in Listed: the arrows never walk them and a click never selects one; the hover names
 * the winner.
 */
std::vector<RitRow> RitArrivals(RitCtx& X)
{
	std::vector<RitRow> Out;
	if (Access::FilterShown(X.RL) != RiverLine::Filter::Running || X.U.LateOnly)
	{
		return Out;
	}
	const net::Network& Net = net::Site(RitSite);
	const double Starts = RitNightEnds(X.World) - net::MinutesPerDay;
	for (const net::EventInstance& E : Net.Window(Starts - 16.0 * 60.0, X.World))
	{
		const net::LiveState L = Net.Live(E, X.World);
		if (L.St != net::Status::Finished || E.Start + E.Duration < Starts || (X.U.GtdOnly && Net.TemplateOf(E).GtdCents <= 0))
		{
			continue;
		}
		RitRow Row;
		Row.E = E;
		Row.L = L;
		Out.push_back(Row);
	}
	std::stable_sort(Out.begin(), Out.end(), [](const RitRow& A, const RitRow& B) { return A.E.Start + A.E.Duration > B.E.Start + B.E.Duration; });
	if (Out.size() > 12)
	{
		Out.resize(12);
	}
	return Out;
}

/** Who won an event, as the Board names them: the world's result's first place (Name empty: no result yet). */
struct RitWinner
{
	std::string Name;
	Chips Prize = 0;
	bool You = false;
};
RitWinner RitWinnerOf(RitCtx& X, const net::EventInstance& E)
{
	const net::Network& Net = net::Site(RitSite);
	RitWinner W;
	for (const net::Placing& P : Net.Result(E).FinalTable)
	{
		if (P.Place != 1)
		{
			continue;
		}
		W.You = P.Player == -1;
		W.Name = P.Player >= 0 && P.Player < static_cast<int>(Net.Players().size()) ? Net.Players()[static_cast<size_t>(P.Player)].Name : W.You ? X.S.HeroName : P.Name;
		W.Prize = P.Prize;
	}
	return W;
}

/** ARRIVED's hover (§3.4 A Statuses): the winner on a cut Carbon plate under the row (over it at the grid's foot). */
void RitArrivedTip(RitCtx& X, const RitRow& Row, const Rect& R)
{
	const RitWinner W = RitWinnerOf(X, Row.E);
	const Rect T{RitGrid.X + RitGrid.W - 348.0f, R.Y + R.H + 56.0f <= RitGrid.Y + RitGrid.H ? R.Y + R.H + 2.0f : R.Y - 54.0f, 340.0f, 52.0f};
	RitPanelBegin();
	RitGlow(X.C, {T.X, T.Y + 6.0f, T.W, T.H}, 0x000000, 0.6f, 14.0f);
	RitCutFill(X.C, T, 6.0f, RitC(RitCarbon));
	RitCutStroke(X.C, T, 6.0f, RitC(RitChromeDark), 1.0f);
	X.C.FillRect({T.X, T.Y, T.W - 6.0f, 2.0f}, RitC(RitRed));
	RitLabel(X.C, "Winner", T.X + 14.0f, T.Y + 21.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "of " + Grouped(Row.L.Entries), T.X + T.W - 14.0f, T.Y + 21.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	if (W.Name.empty())
	{
		RitText(X.C, RitNoWinner, T.X + 14.0f, T.Y + 42.0f, 13.0f, RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, T.W - 28.0f);
		return;
	}
	const float Pw = W.Prize > 0 ? RitText(X.C, Money(W.Prize), T.X + T.W - 14.0f, T.Y + 43.0f, 20.0f, RitC(RitWhite), RitFace::Cond, Align::Right) : 0.0f;
	RitText(X.C, W.Name, T.X + 14.0f, T.Y + 42.0f, 15.0f, RitC(W.You ? RitWhite : RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, T.W - 40.0f - Pw);
}

const char* const RitSortNames[5] = {"START", "BUY-IN", "GTD", "FIELD", "OVERLAY"};

void RitKpis(RitCtx& X)
{
	const net::Network& Net = net::Site(RitSite);
	const double Ends = RitNightEnds(X.World);
	const double Starts = Ends - net::MinutesPerDay;
	// Tonight: the session from the last 6 AM to the next.
	Chips Gtd = 0;
	int Guaranteed = 0;
	Chips Short = 0;
	int Shorts = 0;
	int Running = 0;
	int Seated = 0;
	for (const net::EventInstance& E : Net.Window(Starts, Ends))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		const net::LiveState L = Net.Live(E, X.World);
		if (T.GtdCents > 0 && T.BuyInCents > 0)
		{
			Gtd += T.GtdCents;
			++Guaranteed;
		}
		const site::show::OverlayView V = site::show::OverlayOf(Net, E, X.World);
		if (V.Shown && !V.Actual && V.Cents > 0)
		{
			Short += V.Cents;
			++Shorts;
		}
		if (L.St == net::Status::Running || L.St == net::Status::FinalTable)
		{
			++Running;
			Seated += L.Left;
		}
	}
	const site::Account* A = RitAccount(X.S);
	const Chips Rake = A && A->MonthKey == site::MonthKeyAt(X.World) ? A->RakeMonth : 0;
	const site::show::Accrual Ac = RitAccrual(X.S);
	struct Tile
	{
		std::string Label, Figure, After, Sub;
		uint32_t Rgb;
		bool Amber;
	};
	const Tile Tiles[4] = {
		{"Tonight GTD", rlnet_detail::NetMoney(Gtd), "", Grouped(Guaranteed) + (Guaranteed == 1 ? " guaranteed event" : " guaranteed events"), RitWhite, false},
		{"Overlay watch", Money(Short), "", Shorts == 0 ? std::string("Nothing projected short") : Grouped(Shorts) + (Shorts == 1 ? " event projected short" : " events projected short"), RitAmber, true},
		{"Running", Grouped(Running), "events", Grouped(Seated) + " players seated", RitInk, false},
		{"Your rake \xC2\xB7 " + RitUpper(net::DateLabel(net::DayOf(X.World))).substr(0, 3), Money(Rake), "", RitToNextLine(Ac), RitWhite, false},
	};
	for (int I = 0; I < 4; ++I)
	{
		const Tile& T = Tiles[I];
		const Rect R{232.0f + static_cast<float>(I) * 222.0f, 96.0f, 212.0f, 72.0f};
		const Ui::ClickState St = RitClick(X, "kpi" + std::to_string(I), R);
		RitPanelBegin();
		RitCutFill(X.C, R, 10.0f, RitGlossPaint(R));
		RitCutStroke(X.C, R, 10.0f, RitC(St.Hover ? RitChromeDark : RitHair), 1.0f);
		if (T.Amber)
		{
			X.C.FillRect({R.X, R.Y, R.W - 12.0f, 2.0f}, RitC(RitAmber));
			RitBell(X.C, R.X + 190.0f, R.Y + 20.0f, 13.0f, RitC(Shorts > 0 ? RitAmber : RitDim));
		}
		RitLabel(X.C, T.Label, R.X + 12.0f, R.Y + 18.0f, 12.0f, RitC(RitMuted));
		float Fw = 0.0f;
		if (T.Rgb == RitAmber)
		{
			Fw = RitText(X.C, T.Figure, R.X + 12.0f, R.Y + 54.0f, 34.0f, RitC(Shorts > 0 ? RitAmber : RitMuted)); // a coloured figure prints once
		}
		else
		{
			Fw = RitTwice(X.C, T.Figure, R.X + 12.0f, R.Y + 54.0f, 34.0f, RitC(T.Rgb));
		}
		if (!T.After.empty())
		{
			RitLabel(X.C, T.After, R.X + 20.0f + Fw, R.Y + 54.0f, 15.0f, RitC(RitMuted));
		}
		RitText(X.C, T.Sub, R.X + 12.0f, R.Y + 68.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, R.W - 20.0f);
		if (St.Clicked)
		{
			if (I == 0)
			{
				X.U.GtdOnly = !X.U.GtdOnly;
			}
			else if (I == 1)
			{
				X.U.Sort = X.U.Sort == 4 ? 0 : 4;
			}
			else if (I == 2)
			{
				X.RL.SetFilter(Access::FilterShown(X.RL) == RiverLine::Filter::Running ? RiverLine::Filter::All : RiverLine::Filter::Running, X.Now);
			}
			else
			{
				X.RL.OpenPage(RiverLine::Page::Promotions, X.Now);
			}
		}
	}
}

void RitFilters(RitCtx& X)
{
	// The shared filters as cut chips (the schedule's own), then the Board's two toggles and its sort.
	RitLabel(X.C, "Filter", 232.0f, 201.0f, 12.0f, RitC(RitMuted));
	struct Chip
	{
		const char* Label;
		RiverLine::Filter F;
	};
	const Chip Chips[8] = {{"ALL", RiverLine::Filter::All}, {"PLAYABLE", RiverLine::Filter::Playable}, {"MICRO", RiverLine::Filter::Micro}, {"LOW", RiverLine::Filter::Low},
		{"MID+", RiverLine::Filter::MidHigh}, {"BOUNTY", RiverLine::Filter::Bounty}, {"SERIES", RiverLine::Filter::Series}, {"RUNNING", RiverLine::Filter::Running}};
	float Cx = 272.0f;
	const auto Draw = [&](const std::string& Id, const std::string& Label, bool On) {
		const float W = RitMeasure(X.C, Label, 12.0f) + 20.0f;
		const Rect R{Cx, 182.0f, W, 28.0f};
		const Ui::ClickState St = RitClick(X, Id, R);
		if (On)
		{
			RitCutFill(X.C, R, 6.0f, RitC(RitRed));
			RitLedEdge(X.C, R.X, R.Y, W - 6.0f);
		}
		else
		{
			RitCutFill(X.C, R, 6.0f, RitC(St.Hover ? RitPanel2 : RitCarbon));
			RitCutStroke(X.C, R, 6.0f, RitC(RitHair), 1.0f);
		}
		RitText(X.C, Label, R.X + W / 2.0f, R.Y + 14.0f, 12.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
		Cx += W + 5.0f;
		return St.Clicked;
	};
	for (int I = 0; I < 8; ++I)
	{
		if (Draw("chip" + std::to_string(I), Chips[I].Label, Access::FilterShown(X.RL) == Chips[I].F))
		{
			X.RL.SetFilter(Chips[I].F, X.Now);
		}
	}
	Cx += 6.0f;
	if (Draw("chipgtd", "GTD", X.U.GtdOnly))
	{
		X.U.GtdOnly = !X.U.GtdOnly;
	}
	if (Draw("chiplate", "LATE REG", X.U.LateOnly))
	{
		X.U.LateOnly = !X.U.LateOnly;
	}
	// Sort: START, then BUY-IN, GTD, FIELD, OVERLAY on each press.
	const std::string Sort = RitSortNames[std::min(std::max(X.U.Sort, 0), 4)];
	const float Sw = RitMeasure(X.C, Sort, 13.0f);
	const Rect Sr{1098.0f - Sw - 52.0f, 182.0f, Sw + 66.0f, 28.0f};
	const Ui::ClickState St = RitClick(X, "boardsort", Sr);
	RitLabel(X.C, "Sort", 1098.0f - Sw - 8.0f, 201.0f, 12.0f, RitC(RitMuted), Align::Right);
	RitText(X.C, Sort, 1098.0f, 201.0f, 13.0f, RitC(St.Hover ? RitWhite : RitInk), RitFace::Cond, Align::Right);
	X.C.StrokePolyline({{1104.0f, 194.0f}, {1108.0f, 198.0f}, {1112.0f, 194.0f}}, false, RitC(RitChrome), 1.4f);
	if (St.Clicked)
	{
		X.U.Sort = (X.U.Sort + 1) % 5;
	}
}

/** One row of the grid; returns its STATUS flap's settled character count (the wave's). */
void RitBoardRow(RitCtx& X, const RitRow& Row, const Rect& R, int Visible, int Index, bool Selected)
{
	const net::Network& Net = net::Site(RitSite);
	const net::EventTemplate& T = Net.TemplateOf(Row.E);
	const net::LiveState& L = Row.L;
	const RitStatus St = RitStatusOf(X.S, Row.E, L, Row.Joinable);
	const bool Night = RitNightService(X.World);
	// The ground: zebra Carbon and Panel2 (night service's red cast on the Panel2 rows), or the selection's fixed ground.
	const uint32_t Ground = Selected ? RitSelRow : Index % 2 == 0 ? RitCarbon : Night ? RitNightRow : RitPanel2;
	if (Selected)
	{
		RitGlow(X.C, {R.X, R.Y + R.H - 6.0f, R.W, 8.0f}, RitRed, 0.30f, 8.0f);
	}
	X.C.FillRect(R, RitC(Ground));
	if (Selected)
	{
		X.C.FillRect({R.X, R.Y, 4.0f, R.H}, RitC(RitRed));
		X.C.FillRect({R.X + 4.0f, R.Y, R.W - 4.0f, 1.0f}, RitC(RitLed, 0.55f));
	}
	X.C.FillRect({R.X, R.Y + R.H - 1.0f, R.W, 1.0f}, RitC(0x000000, 0.3f));
	const Color Words = RitC(St.Dim ? RitMuted : RitInk);
	const float Mid = R.Y + R.H / 2.0f;
	const float Fig = R.Y + 17.0f + 15.0f * 0.36f;
	// TIME, a flap tile (24-hour).
	RitFlap(X.C, RitColTime, R.Y + 6.0f, RitClock(Row.E.Start), 14.0f, 22.0f, 5, RitTile::Gloss, Words);
	// EVENT and its tag.
	const std::string Name = rlnet_detail::NetShortName(T.Name);
	const float Nw = RitText(X.C, Name, RitColEvent, R.Y + 22.0f, 14.0f, Words, RitFace::Body, Align::Left, Baseline::Alphabetic, 196.0f);
	const std::string Tag = RitTagOf(T);
	if (!Tag.empty() && Nw < 186.0f)
	{
		const bool Lit = Tag == "FEATURED" || Tag == "SERIES" || Tag == "HIGH ROLLER";
		RitText(X.C, Tag, RitColEvent + Nw + 8.0f, R.Y + 21.5f, 11.0f, RitC(Lit ? RitChrome : RitMuted), RitFace::Cond, Align::Left, Baseline::Alphabetic, RitColBuy - 74.0f - (RitColEvent + Nw + 8.0f));
	}
	// BUY-IN, GTD, FIELD with its chrome bar (the count so far against the template's usual field).
	RitText(X.C, RitBuyIn(T.BuyInCents), RitColBuy, Fig, 15.0f, Words, RitFace::Cond, Align::Right);
	RitText(X.C, T.GtdCents > 0 ? rlnet_detail::NetMoney(T.GtdCents) : std::string("\xE2\x80\x94"), RitColGtd, Fig, 15.0f, RitC(T.GtdCents > 0 && !St.Dim ? RitWhite : RitMuted), RitFace::Cond,
		Align::Right);
	const bool Counted = L.St != net::Status::Announced;
	RitText(X.C, Counted ? Grouped(L.Entries) : std::string("\xE2\x80\x94"), RitColField, Fig, 15.0f, Counted ? Words : RitC(RitMuted), RitFace::Cond, Align::Right);
	if (Counted)
	{
		const float Frac = static_cast<float>(Clamp01(static_cast<double>(L.Entries) / static_cast<double>(std::max(1, T.Field))));
		X.C.FillRect({RitColBar, Mid - 2.0f, 50.0f, 4.0f}, RitC(RitHair));
		X.C.FillRect({RitColBar, Mid - 2.0f, 50.0f * Frac, 4.0f}, RitC(L.St == net::Status::Running ? RitChromeDark : RitChrome));
	}
	// OVERLAY: "PROJ. ±" (amber at $100 and 5% of the guarantee) until registration closes, then the actual overlay.
	const site::show::OverlayView V = site::show::OverlayOf(Net, Row.E, X.World);
	if (V.Shown && V.Cents > 0)
	{
		const Color Oc = RitC(V.Amber ? RitAmber : RitChrome);
		RitText(X.C, "-" + Money(V.Cents), RitColOver, Fig, 15.0f, Oc, RitFace::Cond, Align::Right);
		if (!V.Actual)
		{
			RitPlusMinus(X.C, RitColOver + 2.0f, Fig, 15.0f, Oc);
		}
	}
	else
	{
		X.C.FillRect({RitColOver - 9.0f, Mid - 0.5f, 8.0f, 1.5f}, RitC(RitDim));
	}
	// WIN PTS: the season points a win in this field earns today (net::Points, the network's own; the League's are G2).
	if (Counted && T.BuyInCents > 0)
	{
		RitText(X.C, Grouped(static_cast<int64_t>(std::llround(net::Points(1, std::max(2, L.Entries), T.BuyInCents)))), RitColPts, Fig, 15.0f, Words, RitFace::Cond, Align::Right);
	}
	else
	{
		X.C.FillRect({RitColPts - 9.0f, Mid - 0.5f, 8.0f, 1.5f}, RitC(RitDim));
	}
	// STATUS: the flap, settling in the wave on open and again whenever its value changes (the Kast preview draws it settled
	// and leaves the player's own memo alone).
	int Settled = static_cast<int>(St.Text.size());
	if (!X.Preview)
	{
		RitFlapMemo& Memo = X.U.Flaps[Row.E.Id];
		if (Memo.Text.empty() || Memo.Text != St.Text)
		{
			const bool Wave = X.Now - X.U.WaveAt < 1.0 && Memo.Text.empty();
			Memo.At = Wave ? X.U.WaveAt + 0.02 * static_cast<double>(std::max(Visible, 0)) : Memo.Text.empty() ? -100.0 : X.Now;
			Memo.Sounded = Memo.At < 0.0;
			Memo.Text = St.Text;
		}
		Settled = motion::Flap(X.Now, Memo.At, static_cast<int>(St.Text.size()));
		if (!Memo.Sounded && Settled > 0)
		{
			Memo.Sounded = true;
			RitSound(X, SoundId::RitFlap, 0.25f);
		}
	}
	RitTile Tile = St.Tile;
	if (St.Final && !X.Preview && std::fmod(X.Now, 1.0) >= 0.5)
	{
		Tile = RitTile::RedDim; // FINAL CALL blinks at 1 Hz between Signal Red and its dim phase
	}
	RitFlap(X.C, RitColStatus, R.Y + 6.0f, St.Text, 15.0f, 22.0f, Settled, Tile, RitC(St.Ink), 132.0f);
	if (St.Text == "LOCKED")
	{
		RitLock(X.C, 1092.0f, R.Y + 9.0f, RitC(RitMuted));
	}
}

void RitBoardGrid(RitCtx& X, const std::vector<RitRow>& Rows, const std::vector<RitRow>& Arrived, float& LeadY)
{
	LeadY = -1.0f;
	// The header row.
	X.C.FillRect({232.0f, 220.0f, 880.0f, 30.0f}, RitC(RitBg2));
	X.C.FillRect({232.0f, 249.0f, 880.0f, 1.0f}, RitC(RitHair));
	const float Hy = 235.0f + 12.0f * 0.36f;
	RitLabel(X.C, "Time", RitColTime, Hy, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Event", RitColEvent, Hy, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Buy-in", RitColBuy, Hy, 12.0f, RitC(RitMuted), Align::Right);
	RitLabel(X.C, "GTD", RitColGtd, Hy, 12.0f, RitC(RitMuted), Align::Right);
	RitLabel(X.C, "Field", RitColField - 24.0f, Hy, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Overlay", RitColOver + 12.0f, Hy, 12.0f, RitC(RitMuted), Align::Right);
	RitLabel(X.C, "Win pts", RitColPts, Hy, 12.0f, RitC(RitMuted), Align::Right);
	RitLabel(X.C, "Status", RitColStatus, Hy, 12.0f, RitC(RitMuted));
	// Scrolling: a row and a half a notch; Listed is the grid's order (the arrows walk it).
	float& Scroll = Access::ListScroll(X.RL);
	float& Goal = Access::ListScrollGoal(X.RL);
	const float Content = static_cast<float>(Rows.size() + Arrived.size()) * RitRowPitch;
	const float MaxScroll = std::max(0.0f, Content - RitGrid.H);
	if (!X.Preview && X.UI.Hover(RitGrid) && X.UI.Ptr.Wheel != 0.0f)
	{
		Goal += (X.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * RitRowPitch * 1.5f;
	}
	std::vector<std::string>& Listed = Access::Listed(X.RL);
	Listed.clear();
	int Sel = -1;
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		Listed.push_back(Rows[I].E.Id);
		Sel = Rows[I].E.Id == X.RL.SelectedEvent() ? static_cast<int>(I) : Sel;
	}
	if (Access::TakeScrollToSelected(X.RL) && Sel >= 0)
	{
		const float Top = static_cast<float>(Sel) * RitRowPitch;
		if (Top < Goal || Top + RitRowPitch > Goal + RitGrid.H)
		{
			Goal = Top - RitGrid.H * 0.4f;
		}
	}
	Goal = std::min(std::max(Goal, 0.0f), MaxScroll);
	Scroll += (Goal - Scroll) * rlnet_detail::NetFollow(Access::Dt(X.RL), 14.0);
	if (std::fabs(Goal - Scroll) < 0.3f || X.Preview)
	{
		Scroll = Goal;
	}
	X.C.PushClip(RitGrid);
	const int First = static_cast<int>(std::floor(Scroll / RitRowPitch));
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		const float Y = RitGrid.Y + static_cast<float>(I) * RitRowPitch - Scroll;
		if (Y + RitRowPitch < RitGrid.Y || Y > RitGrid.Y + RitGrid.H)
		{
			continue;
		}
		const Rect R{RitGrid.X, Y, RitGrid.W, RitRowPitch};
		const bool Selected = static_cast<int>(I) == Sel;
		const Ui::ClickState St = RitClick(X, "ev:" + Rows[I].E.Id, R);
		if (St.Clicked && !Selected)
		{
			X.RL.SelectEvent(Rows[I].E.Id);
			X.U.SelectAt = X.Now;
			RitSound(X, SoundId::RitBlip, 0.3f);
		}
		RitBoardRow(X, Rows[I], R, static_cast<int>(I) - First, static_cast<int>(I), Selected);
		if (Selected && Y >= RitGrid.Y && Y + RitRowPitch <= RitGrid.Y + RitGrid.H)
		{
			LeadY = Y + RitRowPitch / 2.0f;
		}
	}
	// Tonight's arrivals under the departures: no selection, the winner on hover (the Kast preview's still has no pointer).
	int Tip = -1;
	Rect TipRow{};
	for (size_t I = 0; I < Arrived.size(); ++I)
	{
		const size_t K = Rows.size() + I;
		const float Y = RitGrid.Y + static_cast<float>(K) * RitRowPitch - Scroll;
		if (Y + RitRowPitch < RitGrid.Y || Y > RitGrid.Y + RitGrid.H)
		{
			continue;
		}
		const Rect R{RitGrid.X, Y, RitGrid.W, RitRowPitch};
		RitBoardRow(X, Arrived[I], R, static_cast<int>(K) - First, static_cast<int>(K), false);
		if (!X.Preview && X.UI.Hover(R) && X.UI.Hover(RitGrid))
		{
			Tip = static_cast<int>(I);
			TipRow = R;
			X.C.FillRect({R.X, R.Y, 2.0f, R.H}, RitC(RitChrome)); // the row the plate speaks for
		}
	}
	// The scan line: LED Red with a 32-unit trail, down the grid every 4 s (every 8 s in night service). Never a still's.
	if (!X.Preview && !Rows.empty())
	{
		const double Period = RitNightService(X.World) ? 8.0 : 4.0;
		const float Sy = RitGrid.Y + static_cast<float>(std::fmod(X.Now, Period) / Period) * std::min(RitGrid.H, Content);
		X.C.FillRect({RitGrid.X, Sy - 32.0f, RitGrid.W, 32.0f}, Paint::Linear({0.0f, Sy - 32.0f}, {0.0f, Sy}, RitC(RitRed, 0.0f), RitC(RitRed, 0.22f)));
		X.C.FillRect({RitGrid.X, Sy, RitGrid.W, 1.0f}, RitC(RitLed));
	}
	X.C.PopClip();
	if (Rows.empty() && Arrived.empty())
	{
		RitText(X.C, RitBoardEmpty, RitGrid.X + RitGrid.W / 2.0f, RitGrid.Y + 120.0f, 15.0f, RitC(RitMuted), RitFace::Body, Align::Center);
	}
	if (MaxScroll > 0.0f)
	{
		const float Th = std::max(40.0f, RitGrid.H * RitGrid.H / Content);
		X.C.FillRect({RitGrid.X + RitGrid.W - 3.0f, RitGrid.Y + (RitGrid.H - Th) * Scroll / MaxScroll, 3.0f, Th}, RitC(RitChrome, 0.35f));
	}
	RitText(X.C, RitBoardFoot, 232.0f, 948.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, 880.0f);
	if (Tip >= 0)
	{
		RitArrivedTip(X, Arrived[static_cast<size_t>(Tip)], TipRow);
	}
}

/** "$101.37 + $7.63": the buy-in's prize part and the site's fee. */
std::string RitSplit(const net::Network& Net, const net::EventTemplate& T)
{
	const Chips Fee = Net.Definition().Rake.Fee(T.BuyInCents);
	return T.BuyInCents == 0 ? std::string("Free") : Money(T.BuyInCents - Fee) + " + " + Money(Fee);
}

/** A term sheet's line: a Chrome label, a dotted leader, the value right-aligned in Mono. */
void RitTerm(RitCtx& X, float L, float R, float Y, const std::string& Key, const std::string& Value, uint32_t Rgb)
{
	const float Kw = RitText(X.C, Key, L, Y, 12.0f, RitC(RitChrome), RitFace::Mono);
	const float Vw = RitText(X.C, Value, R, Y, 13.5f, RitC(Rgb), RitFace::Mono, Align::Right);
	for (float Dx = L + Kw + 8.0f; Dx < R - Vw - 8.0f; Dx += 4.0f)
	{
		X.C.FillRect({Dx, Y - 3.0f, 1.0f, 1.0f}, RitC(RitHair));
	}
}

void RitProspectus(RitCtx& X, const std::vector<RitRow>& Rows, float LeadY)
{
	const Rect P{1128.0f, 96.0f, 448.0f, 854.0f};
	const float L = P.X + 20.0f;
	const float Rx = P.X + P.W - 20.0f;
	RitPanel(X.C, P, 10.0f);
	X.C.FillPolygon({{P.X, P.Y}, {P.X + P.W - 10.0f, P.Y}, {P.X + P.W - 6.0f, P.Y + 4.0f}, {P.X, P.Y + 4.0f}}, RitC(RitRed));
	const net::Network& Net = net::Site(RitSite);
	const RitRow* Row = nullptr;
	for (const RitRow& R : Rows)
	{
		Row = R.E.Id == X.RL.SelectedEvent() ? &R : Row;
	}
	RitLabel(X.C, "Prospectus", L, 126.0f, 12.0f, RitC(RitMuted));
	if (!Row)
	{
		RitText(X.C, RitPickRow, P.X + P.W / 2.0f, 300.0f, 14.0f, RitC(RitMuted), RitFace::Body, Align::Center);
		return;
	}
	const net::EventInstance& E = Row->E;
	const net::LiveState& Lv = Row->L;
	const net::EventTemplate& T = Net.TemplateOf(E);
	// The lead line: the selected row's right end to a notch in the term sheet's left edge (it draws in over 120 ms).
	if (LeadY > 0.0f)
	{
		const float In = X.Preview ? 1.0f : static_cast<float>(Clamp01((X.Now - std::max(X.U.SelectAt, Access::EventAt(X.RL))) / 0.12));
		RitGlow(X.C, {1108.0f, LeadY - 3.0f, 30.0f * In, 6.0f}, RitRed, 0.5f, 4.0f);
		X.C.FillRect({1112.0f, LeadY - 1.0f, 22.0f * In, 2.0f}, RitC(RitRed));
		X.C.FillRect({P.X, LeadY - 6.0f, 6.0f, 12.0f}, RitC(RitRed));
	}
	const int StartMin = static_cast<int>(std::fmod(E.Start, net::MinutesPerDay));
	RitText(X.C, "RIT-" + std::to_string(StartMin / 60 * 100 + StartMin % 60), Rx, 126.0f, 12.0f, RitC(RitChrome), RitFace::Mono, Align::Right);
	// The name, twice-printed (fit down to 22), the twin stripe under it (the panel's one), FEATURED.
	const std::string Name = RitUpper(rlnet_detail::NetShortName(T.Name));
	const float Ns = RitFit(X.C, Name, 30.0f, 22.0f, Rx - L - (T.Featured ? 76.0f : 0.0f));
	RitTwice(X.C, Name, L, 164.0f, Ns, RitC(RitInk));
	RitTwinH(X.C, L, 174.0f, 72.0f, 0.5f);
	if (T.Featured)
	{
		RitTag(X.C, Rx - 66.0f, 148.0f, "FEATURED", RitChrome, RitChrome, 11.0f, 5.0f, 16.0f, 4.0f, true);
	}
	const std::string Runs = T.OnlyDay != -9999 ? std::string("One night") : T.Days == 0x7f ? std::string("Daily") : std::string("Weekly");
	RitText(X.C, Runs + " \xC2\xB7 " + T.Speed + " \xC2\xB7 " + rlnet_detail::NetFormat(T) + " \xC2\xB7 " + std::to_string(T.TableSize) + "-max", L, 202.0f, 13.0f, RitC(RitMuted),
		RitFace::Body, Align::Left, Baseline::Alphabetic, Rx - L);
	X.C.FillRect({L, 216.0f, Rx - L, 1.0f}, RitC(RitHair));
	X.C.FillRect({L, 219.0f, Rx - L, 1.0f}, RitC(RitHair));
	// The terms, in Mono: the machine's print.
	const Chips Fee = Net.Definition().Rake.Fee(T.BuyInCents);
	const int Bb = T.StartStack > 10000 ? 200 : 100;
	const bool Open = Lv.St == net::Status::Announced || Lv.St == net::Status::Registering || Lv.St == net::Status::LateReg;
	const RitStatus St = RitStatusOf(X.S, E, Lv, Row->Joinable);
	const site::show::OverlayView V = site::show::OverlayOf(Net, E, X.World);
	const Chips Each = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T)));
	const Chips PoolNow = Each * static_cast<Chips>(Lv.Entries);
	float Y = 244.0f;
	RitTerm(X, L, Rx, Y, "BUY-IN", RitSplit(Net, T), RitWhite);
	Y += 26.0f;
	RitTerm(X, L, Rx, Y, "FEE", T.BuyInCents > 0 ? Fixed(100.0 * static_cast<double>(Fee) / static_cast<double>(T.BuyInCents), 1) + "%" : std::string("0%"), RitInk);
	Y += 26.0f;
	RitTerm(X, L, Rx, Y, "STACK", Grouped(T.StartStack) + " \xC2\xB7 " + std::to_string(T.StartStack / Bb) + " BB", RitInk);
	Y += 26.0f;
	RitTerm(X, L, Rx, Y, "LEVELS", std::to_string(static_cast<int>(T.LevelMinutes)) + " min \xC2\xB7 " + RitUpper(T.Speed.substr(0, 1)) + T.Speed.substr(1), RitInk);
	Y += 26.0f;
	const std::string LeftWords = Lv.LateRegLeft < 60.0 ? std::to_string(static_cast<int>(std::ceil(Lv.LateRegLeft))) + " min left" : RitCountdown(Lv.LateRegLeft) + " left";
	const std::string Late = Open ? "to " + RitClock(E.Start + E.LateReg) + (Lv.St == net::Status::LateReg ? " \xC2\xB7 " + LeftWords : std::string())
								  : std::string("closed");
	RitTerm(X, L, Rx, Y, "LATE REG", Late, RitInk);
	if (St.Final)
	{
		X.C.FillCircle(Rx - RitMeasure(X.C, Late, 13.5f, RitFace::Mono) - 14.0f, Y - 4.5f, 3.2f, RitC(RitRed));
	}
	Y += 26.0f;
	const std::string Field = Lv.St == net::Status::Announced ? std::string("opens ") + RitClock(E.Start - T.OpensHours * 60.0)
		: V.Shown && !V.Actual && V.Entries > static_cast<double>(Lv.Entries) + 0.5
		? Grouped(Lv.Entries) + " + ~" + Grouped(static_cast<int64_t>(std::llround(V.Entries - static_cast<double>(Lv.Entries)))) + " late"
		: Grouped(Lv.Entries) + " entries";
	RitTerm(X, L, Rx, Y, "FIELD", Field, RitInk);
	Y += 26.0f;
	RitTerm(X, L, Rx, Y, "POOL", T.GtdCents > 0 ? Money(PoolNow) + " / " + rlnet_detail::NetMoney(T.GtdCents) : Money(PoolNow), RitWhite);
	Y += 20.0f;
	if (T.GtdCents > 0)
	{
		// The pool meter: an Onyx track, a Chrome fill, the projection's amber tick, a Hot White tick at the guarantee.
		const float Mw = Rx - L;
		const double Top = std::max(static_cast<double>(T.GtdCents), static_cast<double>(PoolNow)) * 1.0;
		X.C.FillRect({L, Y, Mw, 6.0f}, RitC(RitOnyx));
		X.C.FillRect({L, Y, Mw * static_cast<float>(Clamp01(static_cast<double>(PoolNow) / Top)), 6.0f}, RitC(RitChrome));
		const float Gx = L + Mw * static_cast<float>(Clamp01(static_cast<double>(T.GtdCents) / Top));
		X.C.FillRect({Gx - 2.0f, Y - 4.0f, 2.0f, 14.0f}, RitC(RitWhite));
		if (V.Shown && !V.Actual && V.Entries > 0.0)
		{
			// The projection's amber tick, its figure under the meter's left end (never on top of the guarantee's).
			const double Proj = V.Entries * static_cast<double>(Each);
			const float Px = L + Mw * static_cast<float>(Clamp01(Proj / std::max(Top, Proj)));
			X.C.FillRect({std::min(Px, Rx) - 1.0f, Y - 4.0f, 2.0f, 14.0f}, RitC(RitAmber));
			RitText(X.C, "proj. " + rlnet_detail::NetMoney(static_cast<Chips>(Proj)), L, Y + 22.0f, 11.0f, RitC(RitAmber), RitFace::Mono);
		}
		RitText(X.C, "GTD " + rlnet_detail::NetMoney(T.GtdCents), Rx, Y + 22.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	}
	Y += 40.0f;
	// The projected overlay (amber prints once): the most honest promotion on the site.
	const Rect Box{L, Y, Rx - L, 78.0f};
	RitCutFill(X.C, Box, 8.0f, RitC(T.GtdCents > 0 ? RitAmberGround : RitCarbon));
	RitCutStroke(X.C, Box, 8.0f, RitC(T.GtdCents > 0 ? RitAmber : RitHair, 0.6f), 1.0f);
	if (T.GtdCents > 0 && V.Shown && V.Cents > 0)
	{
		const float Lw = RitLabel(X.C, V.Actual ? "Overlay" : "Projected overlay", Box.X + 14.0f, Box.Y + 22.0f, 12.0f, RitC(RitAmber));
		if (!V.Actual)
		{
			RitPlusMinus(X.C, Box.X + 18.0f + Lw, Box.Y + 22.0f, 12.0f, RitC(RitAmber));
		}
		RitText(X.C, Money(V.Cents), Box.X + 14.0f, Box.Y + 64.0f, 44.0f, RitC(RitAmber));
		RitText(X.C, RitSitePays, Box.X + Box.W - 14.0f, Box.Y + 60.0f, 12.0f, RitC(RitMuted), RitFace::Body, Align::Right, Baseline::Alphabetic, 180.0f);
	}
	else
	{
		RitLabel(X.C, "Overlay watch", Box.X + 14.0f, Box.Y + 22.0f, 12.0f, RitC(T.GtdCents > 0 ? RitAmber : RitMuted));
		// "Covers it so far" only when the pool so far does; short of it, the projection is what clears it.
		const char* Why = T.GtdCents <= 0 ? RitNoGtd : PoolNow >= T.GtdCents ? RitNoOverlay : RitNoOverlayYet;
		RitParagraph(X.C, Why, Box.X + 14.0f, Box.Y + 46.0f, Box.W - 28.0f, 13.0f, RitC(RitMuted), 18.0f, 2);
	}
	Y += 104.0f;
	// PAYS: places paid, min cash and 1st, from the payout table the tables themselves deal (Tournament's pool and min cash
	// over the entries so far; "so far" while registration is open: §3.4 A's term sheet).
	std::string Pays = "as the field grows";
	if (Lv.Entries > 0)
	{
		const LobbyEvent PayListing = Net.Listing(E, nullptr, X.S.Unlocks());
		const TournamentSpec& PaySpec = PayListing.Spec;
		const Chips PayPart = PaySpec.BuyInCents - PaySpec.FeeCents - PaySpec.BountyCents;
		const Chips PayPool = std::max(PaySpec.GuaranteeCents, static_cast<Chips>(Lv.Entries) * PayPart);
		const std::string PaySoFar = Open ? " so far" : "";
		if (PaySpec.SeatValueCents > 0)
		{
			const Chips PaySeats = std::max<Chips>(1, PayPool / PaySpec.SeatValueCents);
			Pays = Grouped(PaySeats) + (PaySeats == 1 ? " seat" : " seats") + PaySoFar;
		}
		else
		{
			const Chips PayMin = PaySpec.BuyInCents > 0 ? static_cast<Chips>(std::llround(static_cast<double>(PayPart) * 1.4)) : 20;
			const std::vector<Chips> PayTable = PayoutTable(PayPool, Lv.Entries, PayMin);
			Pays = PayTable.empty() ? Pays
									: Grouped(static_cast<int64_t>(PayTable.size())) + " \xC2\xB7 min " + Money(PayTable.back()) + " \xC2\xB7 1st " + Money(PayTable.front()) + PaySoFar;
		}
	}
	RitTerm(X, L, Rx, Y, "PAYS", Pays, RitWhite);
	Y += 18.0f;
	X.C.FillRect({L, Y, Rx - L, 1.0f}, RitC(RitHair));
	Y += 26.0f;
	// YOUR RECORD HERE: History filtered to the template.
	int Played = 0;
	int Cashed = 0;
	int Best = 0;
	const std::string Family = RitTemplateOf(E.Id);
	for (const HistoryEntry& H : X.S.History)
	{
		if (RitTemplateOf(H.EventId) == Family)
		{
			++Played;
			Cashed += H.Prize > 0 ? 1 : 0;
			Best = Best == 0 || (H.Place > 0 && H.Place < Best) ? H.Place : Best;
		}
	}
	RitTerm(X, L, Rx, Y, "YOUR RECORD", Played == 0 ? std::string(RitNoRecord) : std::to_string(Played) + " played \xC2\xB7 " + std::to_string(Cashed) + " cashed \xC2\xB7 best " + Ordinal(Best),
		RitInk);
	Y += 22.0f;
	// The tabs (the panel's twin stripe is the one under the name: the active tab takes a single red bar).
	const char* const Tabs[4] = {"OVERVIEW", "STRUCTURE", "PAYOUTS", "ENTRANTS"};
	float Tx = L;
	for (int I = 0; I < 4; ++I)
	{
		const float Tw = RitMeasure(X.C, Tabs[I], 13.0f) + 18.0f;
		const Ui::ClickState Ts0 = RitClick(X, "ptab" + std::to_string(I), {Tx, Y, Tw, 28.0f});
		if (Ts0.Clicked)
		{
			X.U.Tab = I;
		}
		const bool On = X.U.Tab == I;
		if (On)
		{
			X.C.FillRect({Tx, Y + 24.0f, Tw, 3.0f}, RitC(RitRed));
		}
		RitText(X.C, Tabs[I], Tx + Tw / 2.0f, Y + 17.0f, 13.0f, RitC(On ? RitWhite : Ts0.Hover ? RitInk : RitMuted), RitFace::Cond, Align::Center);
		Tx += Tw + 4.0f;
	}
	X.C.FillRect({L, Y + 27.0f, Rx - L, 1.0f}, RitC(RitHair));
	const float By = Y + 52.0f;
	switch (X.U.Tab)
	{
	case 1:
	{
		const int LateLevels = static_cast<int>(std::ceil(E.LateReg / std::max(1.0, T.LevelMinutes)));
		RitTerm(X, L, Rx, By, "LEVEL 1", T.StartStack > 10000 ? std::string("100/200") : std::string("50/100"), RitInk);
		RitTerm(X, L, Rx, By + 24.0f, "LEVELS", "every " + std::to_string(static_cast<int>(T.LevelMinutes)) + " min", RitInk);
		RitTerm(X, L, Rx, By + 48.0f, "LATE REG", net::Countdown(E.LateReg) + " (to level " + std::to_string(LateLevels + 1) + ")", RitInk);
		RitTerm(X, L, Rx, By + 72.0f, "STARTS", RitDay(net::DayOf(E.Start)) + " " + RitClock(E.Start), RitInk);
		break;
	}
	case 2:
		RitTerm(X, L, Rx, By, "POOL SO FAR", Money(PoolNow), RitWhite);
		RitTerm(X, L, Rx, By + 24.0f, "PER ENTRY", Money(Each), RitInk);
		RitParagraph(X.C, RitPaysGrow, L, By + 50.0f, Rx - L, 13.0f, RitC(RitMuted), 18.0f, 3);
		break;
	case 3:
	{
		// Who's in: the world's registrations the player can know (Team RunItTwice's pros, the followed).
		const world::World& W = X.S.Living();
		const std::string Team = net::Def(RitSite).TeamName;
		int Shown = 0;
		for (const int Id : W.RegisteredNow(E.Id))
		{
			const world::Npc* N = W.Get(Id);
			if (!N || Shown >= 3 || (N->Sponsor != Team && N->Followers < 2000))
			{
				continue;
			}
			const float Ry = By + static_cast<float>(Shown) * 34.0f;
			RitSquareAvatar(X.C, L, Ry - 16.0f, 24.0f, N->Name, RitC(N->Sponsor == Team ? RitRed : RitChrome));
			const float Nw = RitText(X.C, N->Name, L + 34.0f, Ry, 14.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 200.0f);
			if (N->Sponsor == Team)
			{
				RitTag(X.C, L + 40.0f + Nw, Ry - 13.0f, "TEAM", RitRed, RitWhite, 11.0f);
			}
			RitText(X.C, N->Sponsor == Team ? std::string("Team RunItTwice") : Grouped(N->Followers) + " followers", Rx, Ry, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
			++Shown;
		}
		if (Shown == 0)
		{
			RitText(X.C, Grouped(Lv.Entries) + " registered so far.", L, By, 13.0f, RitC(RitMuted), RitFace::Body);
		}
		break;
	}
	default:
		RitParagraph(X.C, !T.Blurb.empty() ? T.Blurb : std::string(RitPaysGrow), L, By - 4.0f, Rx - L, 13.0f, RitC(RitMuted), 18.0f, 4);
		break;
	}
	// The register block {1128,830,448,110}: the line, the countdown flap, the shared RegisterBlock as a red slab.
	X.C.FillRect({P.X, 830.0f, P.W, 1.0f}, RitC(RitHair));
	if (!X.S.ConfirmRegister)
	{
		const std::string Closes = Open ? "Registration closes " + RitClock(E.Start + E.LateReg) : std::string("Registration closed");
		RitLabel(X.C, Closes, L, 852.0f, 13.0f, RitC(RitInk));
		if (Lv.St == net::Status::LateReg)
		{
			const std::string Left = RitCountdown(Lv.LateRegLeft);
			const float Fw = RitFlap(X.C, Rx - 50.0f, 838.0f, Left, 14.0f, 20.0f, static_cast<int>(Left.size()), RitTile::Red, RitC(RitWhite), 50.0f);
			(void)Fw;
			X.C.FillCircle(Rx - 60.0f, 848.0f, 3.2f, RitC(RitRed));
		}
	}
	Access::RegisterBlock(X.RL, E, Lv, {L, 862.0f, Rx - L, 56.0f}, X.Now);
	RitText(X.C, "Balance " + Money(rlsites_detail::SitesBalance(X.S, RitSite)) + " \xC2\xB7 after " + Money(rlsites_detail::SitesBalance(X.S, RitSite) - T.BuyInCents), L, 940.0f, 11.0f,
		RitC(RitMuted), RitFace::Body);
	RitText(X.C, RitSettles, Rx, 940.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Right);
}

void RitBoardPage(RitCtx& X)
{
	// The wave: on every open the visible statuses flip into place row by row.
	if (!X.Preview && (X.U.SeenPage != RiverLine::Page::Lobby || X.U.WaveKey != Access::PageAt(X.RL)))
	{
		X.U.WaveKey = Access::PageAt(X.RL);
		X.U.WaveAt = X.Now;
		X.U.Flaps.clear();
	}
	const std::vector<RitRow> Rows = RitBoardRows(X);
	// A selection the Board can show (the first FINAL CALL, else the first row) when there is none on it.
	bool Has = false;
	for (const RitRow& R : Rows)
	{
		Has = Has || R.E.Id == X.RL.SelectedEvent();
	}
	if (!Has && !Rows.empty())
	{
		const RitRow* Pick = &Rows.front();
		for (const RitRow& R : Rows)
		{
			if (R.L.St == net::Status::LateReg && R.L.LateRegLeft <= 10.0 && R.Joinable && Pick == &Rows.front())
			{
				Pick = &R;
			}
		}
		X.RL.SelectEvent(Pick->E.Id);
	}
	RitKpis(X);
	RitFilters(X);
	float LeadY = -1.0f;
	RitPanelBegin();
	RitBoardGrid(X, Rows, RitArrivals(X), LeadY);
	RitProspectus(X, Rows, LeadY);
}

// ------------------------------------------------------------------ SUNDAY LIVE (Page::Live, §3.4 B, R6): the match centre

/** Today's majors: Sunday's (the 100K, the Grind, High Bridge), else the series' main event, else the night's featured events. */
std::vector<net::EventInstance> RitMajors(double World)
{
	const net::Network& Net = net::Site(RitSite);
	const double Midnight = std::floor(World / net::MinutesPerDay) * net::MinutesPerDay;
	std::vector<net::EventInstance> Sunday, Main, Featured;
	for (const net::EventInstance& E : Net.Window(Midnight, Midnight + net::MinutesPerDay))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		if (T.Featured && T.Days == (1 << 6) && T.Series.empty())
		{
			Sunday.push_back(E);
		}
		else if (T.Main)
		{
			Main.push_back(E);
		}
		else if (T.Featured)
		{
			Featured.push_back(E);
		}
	}
	if (!Sunday.empty())
	{
		return Sunday;
	}
	if (!Main.empty())
	{
		return Main;
	}
	if (Featured.size() > 4)
	{
		Featured.resize(4);
	}
	return Featured;
}

/** The opening bell (§3.4 B): a Sunday major starting while RunItTwice is in front rings once (keyed by the instance). */
void RitOpeningBell(RitCtx& X)
{
	const net::Network& Net = net::Site(RitSite);
	const std::vector<net::EventInstance> Majors = RitMajors(X.World);
	for (size_t I = 0; I < Majors.size(); ++I)
	{
		const net::EventInstance& E = Majors[I];
		const net::EventTemplate& T = Net.TemplateOf(E);
		if (T.Days == (1 << 6) && X.World >= E.Start && X.World - E.Start < 3.0 && X.U.BellFor != E.Id && !X.Preview)
		{
			X.U.BellFor = E.Id;
			X.U.BellAt = X.Now;
			X.U.Live = static_cast<int>(I); // Sunday Live turns to the major that just opened
			RitSound(X, SoundId::RitBell, 0.5f);
		}
	}
}

void RitLivePage(RitCtx& X)
{
	const net::Network& Net = net::Site(RitSite);
	const std::vector<net::EventInstance> Majors = RitMajors(X.World);
	RitPanelBegin();
	// The selector: the day's majors as cut chips (the selected one Signal Red).
	float Cx = 232.0f;
	X.U.Live = std::min(std::max(X.U.Live, 0), std::max(0, static_cast<int>(Majors.size()) - 1));
	for (size_t I = 0; I < Majors.size(); ++I)
	{
		const net::EventTemplate& T = Net.TemplateOf(Majors[I]);
		const std::string Label = RitUpper(rlnet_detail::NetShortName(T.Name)) + " \xC2\xB7 " + RitClock(Majors[I].Start);
		const float W = RitMeasure(X.C, Label, 13.0f) + 24.0f;
		const Rect R{Cx, 96.0f, W, 32.0f};
		const Ui::ClickState St = RitClick(X, "live" + std::to_string(I), R);
		const bool On = static_cast<int>(I) == X.U.Live;
		RitCutFill(X.C, R, 6.0f, RitC(On ? RitRed : St.Hover ? RitPanel2 : RitCarbon));
		if (On)
		{
			RitLedEdge(X.C, R.X, R.Y, W - 6.0f);
		}
		else
		{
			RitCutStroke(X.C, R, 6.0f, RitC(RitHair), 1.0f);
		}
		RitText(X.C, Label, R.X + W / 2.0f, R.Y + 16.0f, 13.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
		if (St.Clicked && !On)
		{
			X.U.Live = static_cast<int>(I);
			X.U.FeedSeen = 0;
			RitSound(X, SoundId::RitBlip, 0.3f);
		}
		Cx += W + 8.0f;
	}
	if (Majors.empty())
	{
		RitText(X.C, RitNoMajor, 232.0f, 117.0f, 14.0f, RitC(RitMuted), RitFace::Body);
		return;
	}
	const net::EventInstance& E = Majors[static_cast<size_t>(X.U.Live)];
	const net::EventTemplate& T = Net.TemplateOf(E);
	const net::LiveState L = Net.Live(E, X.World);
	const site::show::OverlayView V = site::show::OverlayOf(Net, E, X.World);
	// The hero band: the name, buy-in and GTD twice-printed; the split-flap countdown; the state; the guarantee meter.
	const Rect Hb{232.0f, 136.0f, 1344.0f, 140.0f};
	RitPanel(X.C, Hb, 10.0f);
	RitTwice(X.C, RitUpper(rlnet_detail::NetShortName(T.Name)), Hb.X + 24.0f, Hb.Y + 52.0f, RitFit(X.C, RitUpper(rlnet_detail::NetShortName(T.Name)), 34.0f, 24.0f, 520.0f), RitC(RitWhite));
	RitText(X.C, RitBuyIn(T.BuyInCents) + " BUY-IN" + (T.GtdCents > 0 ? " \xC2\xB7 " + rlnet_detail::NetMoney(T.GtdCents) + " GTD" : std::string()), Hb.X + 24.0f, Hb.Y + 82.0f, 18.0f,
		RitC(RitChrome));
	std::string State;
	bool Red = false;
	std::string Clock;
	std::string Over;
	switch (L.St)
	{
	case net::Status::Announced: State = "SCHEDULED"; Clock = rlnet_detail::NetHms(L.RegOpensIn); Over = "Registration opens in"; break; // not open yet
	case net::Status::Registering: State = "REGISTERING"; Clock = rlnet_detail::NetHms(L.StartsIn); Over = "Starts in"; break;
	case net::Status::LateReg: State = "LATE REG"; Clock = rlnet_detail::NetHms(L.LateRegLeft); Over = "Late registration closes in"; break;
	case net::Status::Running: State = "RUNNING"; Red = true; Clock = Grouped(L.Left); Over = "Players left"; break;
	case net::Status::FinalTable: State = "FINAL TABLE"; Red = true; Clock = Grouped(L.Left); Over = "Players left"; break;
	case net::Status::Finished: State = "COMPLETE"; Clock = RitClock(E.Start + E.Duration); Over = "Finished at"; break;
	}
	RitLabel(X.C, Over, Hb.X + 620.0f, Hb.Y + 28.0f, 12.0f, RitC(RitMuted));
	const float Tw = RitTag(X.C, Hb.X + 24.0f, Hb.Y + 98.0f, State, Red ? RitRed : RitCarbon, Red ? RitWhite : RitInk, 13.0f, 8.0f, 24.0f, 6.0f);
	if (!Red)
	{
		RitCutStroke(X.C, {Hb.X + 24.0f, Hb.Y + 98.0f, Tw, 24.0f}, 6.0f, RitC(RitHair), 1.0f);
	}
	// The split-flap countdown: gloss-black cells, BoldCondensed 48 in Hot White, a 1-unit split.
	const float Fx = Hb.X + 620.0f;
	for (size_t I = 0; I < Clock.size() && I < 9; ++I)
	{
		const Rect Cell{Fx + static_cast<float>(I) * 48.0f, Hb.Y + 38.0f, 44.0f, 64.0f};
		X.C.FillRect({Cell.X, Cell.Y, Cell.W, Cell.H / 2.0f}, Paint::Linear({0.0f, Cell.Y}, {0.0f, Cell.Y + Cell.H / 2.0f}, RitC(RitFlapTop), RitC(RitTwillB)));
		X.C.FillRect({Cell.X, Cell.Y + Cell.H / 2.0f, Cell.W, Cell.H / 2.0f}, Paint::Linear({0.0f, Cell.Y + Cell.H / 2.0f}, {0.0f, Cell.Y + Cell.H}, RitC(0x070708), RitC(RitFlapLow)));
		RitText(X.C, std::string(1, Clock[I]), Cell.X + Cell.W / 2.0f, Cell.Y + Cell.H / 2.0f, 48.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
		X.C.FillRect({Cell.X, Cell.Y + Cell.H / 2.0f - 0.5f, Cell.W, 1.0f}, RitC(0x000000, 0.7f));
	}
	if (T.GtdCents > 0)
	{
		// The guarantee meter: a Carbon track, a Signal Red fill, a Hot White tick at the guarantee.
		const Rect M{Hb.X + 1110.0f, Hb.Y + 62.0f, 200.0f, 8.0f};
		const Chips Each = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * Net.Definition().Rake.PrizeShare(T)));
		const Chips Pool = Each * static_cast<Chips>(L.Entries);
		const double Top = std::max(static_cast<double>(T.GtdCents) * 1.15, static_cast<double>(Pool));
		X.C.FillRect(M, RitC(RitCarbon));
		X.C.FillRect({M.X, M.Y, M.W * static_cast<float>(Clamp01(static_cast<double>(Pool) / Top)), M.H}, RitC(RitRed));
		const float Gx = M.X + M.W * static_cast<float>(Clamp01(static_cast<double>(T.GtdCents) / Top));
		X.C.FillRect({Gx - 1.0f, M.Y - 4.0f, 2.0f, M.H + 8.0f}, RitC(RitWhite));
		RitLabel(X.C, "Pool " + rlnet_detail::NetMoney(Pool), M.X, M.Y - 12.0f, 13.0f, RitC(RitInk));
		if (V.Actual && V.Cents > 0)
		{
			// Registration closed short: the major flips to OVERLAY (an amber tile).
			RitTag(X.C, M.X, M.Y + 18.0f, "OVERLAY " + Money(V.Cents), RitAmber, RitOnyx, 13.0f, 8.0f, 22.0f, 6.0f);
		}
		else if (V.Shown && V.Cents > 0)
		{
			const float Pw = RitLabel(X.C, "Proj short " + Money(V.Cents), M.X, M.Y + 34.0f, 13.0f, RitC(RitAmber));
			RitPlusMinus(X.C, M.X + Pw + 4.0f, M.Y + 34.0f, 13.0f, RitC(RitAmber));
		}
		else if (V.Shown)
		{
			RitLabel(X.C, "Proj over the guarantee", M.X, M.Y + 34.0f, 13.0f, RitC(RitAmber));
		}
	}
	// The opening bell's band: "SUNDAY SESSION OPEN" across the hero band's top for 3 s.
	if (X.U.BellFor == E.Id && X.Now - X.U.BellAt < 3.0 && !X.Preview)
	{
		RitCutFill(X.C, {Hb.X, Hb.Y, Hb.W, 30.0f}, 10.0f, RitC(RitRed));
		RitBell(X.C, Hb.X + 22.0f, Hb.Y + 15.0f, 16.0f, RitC(RitWhite));
		RitText(X.C, "SUNDAY SESSION OPEN", Hb.X + 40.0f, Hb.Y + 15.0f, 15.0f, RitC(RitWhite), RitFace::Cond, Align::Left, Baseline::Middle);
	}
	// The story: registrations by minute (a Chrome line), the notable entries, or the result; never NPC chip counts (W5).
	const Rect Story{232.0f, 292.0f, 860.0f, 658.0f};
	RitPanel(X.C, Story, 10.0f);
	RitLabel(X.C, "Story", Story.X + 20.0f, Story.Y + 30.0f, 12.0f, RitC(RitMuted));
	const std::vector<site::show::LiveRow> Rows = site::show::SundayLive(Net, &X.S.Living(), E, X.World);
	const std::vector<site::show::RegPoint> Regs = site::show::Registrations(Net, E, X.World, 48);
	const Rect Plot{Story.X + 20.0f, Story.Y + 56.0f, Story.W - 40.0f, 200.0f};
	X.C.FillRect({Plot.X, Plot.Y + Plot.H, Plot.W, 1.0f}, RitC(RitHair));
	int Peak = 1;
	for (const site::show::RegPoint& P : Regs)
	{
		Peak = std::max(Peak, P.Entries);
	}
	if (Regs.size() >= 2)
	{
		std::vector<Vec2> Pts;
		for (size_t I = 0; I < Regs.size(); ++I)
		{
			Pts.push_back({Plot.X + Plot.W * static_cast<float>(I) / static_cast<float>(Regs.size() - 1), Plot.Y + Plot.H - Plot.H * static_cast<float>(Regs[I].Entries) / static_cast<float>(Peak)});
		}
		X.C.StrokePolyline(Pts, false, RitC(RitChrome), 2.0f);
		X.C.FillCircle(Pts.back().X, Pts.back().Y, 3.0f, RitC(RitRed));
		RitText(X.C, Grouped(Regs.back().Entries) + " registered", Plot.X, Plot.Y - 8.0f, 13.0f, RitC(RitInk), RitFace::Mono);
	}
	else
	{
		RitText(X.C, "Registration opens " + RitDay(net::DayOf(E.Start - T.OpensHours * 60.0)) + " " + RitClock(E.Start - T.OpensHours * 60.0) + ".", Plot.X, Plot.Y + 30.0f, 14.0f, RitC(RitMuted),
			RitFace::Body);
	}
	float Sy = Plot.Y + Plot.H + 40.0f;
	RitLabel(X.C, L.St == net::Status::Finished ? "The result" : "Notable entries", Story.X + 20.0f, Sy, 12.0f, RitC(RitMuted));
	Sy += 14.0f;
	const std::string Team = net::Def(RitSite).TeamName;
	int Shown = 0;
	const world::World& W = X.S.Living();
	for (const site::show::LiveRow& R : Rows)
	{
		const bool Want = L.St == net::Status::Finished ? R.Kind == site::show::LivePodium : (R.Kind == site::show::LiveTeam || R.Kind == site::show::LiveNotable);
		if (!Want || Shown >= 8)
		{
			continue;
		}
		const float Ry = Sy + 26.0f + static_cast<float>(Shown) * 40.0f;
		const world::Npc* N = W.Get(R.Npc);
		if (N)
		{
			RitSquareAvatar(X.C, Story.X + 20.0f, Ry - 18.0f, 28.0f, N->Name, RitC(R.Kind == site::show::LiveTeam ? RitRed : RitChrome));
		}
		const float Nx = Story.X + (N ? 60.0f : 20.0f);
		const float Lw = RitText(X.C, R.Text, Nx, Ry, 14.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, Story.W - 160.0f);
		if (R.Kind == site::show::LiveTeam)
		{
			RitTag(X.C, Nx + Lw + 8.0f, Ry - 13.0f, "TEAM", RitRed, RitWhite, 11.0f);
		}
		++Shown;
	}
	if (Shown == 0)
	{
		// (g1-audit: the notable entries' own empty line; the feed's "Nothing in the feed yet" belongs to the feed.)
		RitText(X.C, L.St == net::Status::Running || L.St == net::Status::FinalTable ? std::string(RitNoChipCounts) : L.St == net::Status::Finished ? std::string(RitNoFeed) : std::string(RitNoNotables),
			Story.X + 20.0f, Sy + 26.0f, 14.0f,
			RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, Story.W - 40.0f);
	}
	if ((L.St == net::Status::Running || L.St == net::Status::FinalTable) && X.S.IsPlaying(E.Id))
	{
		// Your own live rank and stack (the session's: the only chips the Board ever shows).
		for (int I = 0; I < X.S.TableCount(); ++I)
		{
			const TableGlance G = X.S.Glance(I);
			if (G.EventId == E.Id)
			{
				RitTwice(X.C, "YOU " + RitUpper(Ordinal(G.Rank)), Story.X + 20.0f, Story.Y + Story.H - 28.0f, 30.0f, RitC(RitWhite));
				RitText(X.C, "of " + Grouped(G.Remaining) + " \xC2\xB7 " + Fixed(G.StackBb, 1) + " BB", Story.X + 160.0f, Story.Y + Story.H - 30.0f, 15.0f, RitC(RitChrome), RitFace::Mono);
			}
		}
	}
	// The feed: time-stamped world facts (each new line a RitBlip and the tape's flash), and the seat.
	const Rect Feed{1108.0f, 292.0f, 468.0f, 658.0f};
	RitPanel(X.C, Feed, 10.0f);
	RitLabel(X.C, "Feed", Feed.X + 20.0f, Feed.Y + 30.0f, 12.0f, RitC(RitMuted));
	std::vector<const site::show::LiveRow*> Facts;
	for (const site::show::LiveRow& R : Rows)
	{
		if (R.Kind == site::show::LiveFact)
		{
			Facts.push_back(&R);
		}
	}
	if (!X.Preview && X.U.FeedFor != E.Id)
	{
		X.U.FeedFor = E.Id;
		X.U.FeedSeen = Facts.size();
	}
	if (!X.Preview && Facts.size() > X.U.FeedSeen)
	{
		X.U.FeedSeen = Facts.size();
		RitSound(X, SoundId::RitBlip, 0.3f);
		// The new line also runs on the tape, where it lands with the tape's flash (rebuilt this frame).
		std::string Line = Facts.back()->Text;
		Line = !Line.empty() && Line.back() == '.' ? Line.substr(0, Line.size() - 1) : Line;
		RitTapeItem Item;
		Item.Key = "feed:" + E.Id + ":" + std::to_string(Facts.size());
		Item.Text = RitUpper(rlnet_detail::NetShortName(T.Name)) + " \xC2\xB7 " + RitUpper(Line);
		X.U.Extra.insert(X.U.Extra.begin(), Item);
		if (X.U.Extra.size() > 3)
		{
			X.U.Extra.pop_back();
		}
		X.U.TapeKey = -1;
	}
	float Fy = Feed.Y + 60.0f;
	for (auto It = Facts.rbegin(); It != Facts.rend() && Fy < Feed.Y + Feed.H - 150.0f; ++It)
	{
		const bool Earlier = net::DayOf((*It)->At) != net::DayOf(X.World);
		RitText(X.C, (Earlier ? RitDay(net::DayOf((*It)->At)) + " " : std::string()) + RitClock((*It)->At), Feed.X + 20.0f, Fy, 12.0f, RitC(RitChrome), RitFace::Mono);
		Fy = RitParagraph(X.C, (*It)->Text, Feed.X + 100.0f, Fy, Feed.W - 120.0f, 13.0f, RitC(RitInk), 18.0f, 3) + 10.0f;
	}
	if (Facts.empty())
	{
		RitParagraph(X.C, RitNoFeed, Feed.X + 20.0f, Fy, Feed.W - 40.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
	}
	X.C.FillRect({Feed.X, Feed.Y + Feed.H - 126.0f, Feed.W, 1.0f}, RitC(RitHair));
	Access::RegisterBlock(X.RL, E, L, {Feed.X + 20.0f, Feed.Y + Feed.H - 96.0f, Feed.W - 40.0f, 56.0f}, X.Now);
}

// ------------------------------------------------------------------ RANKINGS (Page::Leaderboards, §3.4 D)

/** A board's value as the rankings print it ("777 pts", "3 titles"). */
std::string RitBoardValue(net::Board B, double V)
{
	const long long N = std::llround(V);
	switch (B)
	{
	case net::Board::Wins: return Grouped(static_cast<int64_t>(N)) + (N == 1 ? " title" : " titles");
	case net::Board::Earnings: return rlnet_detail::NetMoney(static_cast<Chips>(N));
	default: return Grouped(static_cast<int64_t>(N)) + " pts";
	}
}

/** The EVENTS column's right edge, from the table's left (between the names, which end by 670, and the form at 860). */
constexpr float RitRankEvents = 800.0f;

void RitRankingsPage(RitCtx& X)
{
	const net::Network& Net = net::Site(RitSite);
	const net::SeriesInfo* Sr = Net.CurrentSeries(X.World);
	const bool SeriesLive = Sr && net::DayOf(X.World) >= Sr->FirstDay;
	const std::vector<net::Board> Shown = rlsites_detail::SitesBoards(RitSite, SeriesLive);
	net::Board Board = Access::BoardShown(X.RL);
	if (std::find(Shown.begin(), Shown.end(), Board) == Shown.end())
	{
		Board = Shown.front();
		X.RL.ShowBoard(Board, X.Now);
	}
	const auto Name = [&](net::Board B) { return B == net::Board::Series && Sr ? Sr->Name : rlsites_detail::SitesBoardName(RitSite, B); };
	// The tabs: cut chips (board<Label>), the active one a Signal Red slab; each with your standing on it.
	RitPanelBegin();
	float Tx = 232.0f;
	for (const net::Board B : Shown)
	{
		const std::string Label = Name(B);
		const float W = RitMeasure(X.C, RitUpper(Label), 14.0f) + 28.0f;
		const Rect R{Tx, 96.0f, W, 32.0f};
		const Ui::ClickState St = RitClick(X, "board" + Label, R);
		const bool On = B == Board;
		RitCutFill(X.C, R, 6.0f, RitC(On ? RitRed : St.Hover ? RitPanel2 : RitCarbon));
		if (On)
		{
			RitLedEdge(X.C, R.X, R.Y, W - 6.0f);
		}
		else
		{
			RitCutStroke(X.C, R, 6.0f, RitC(RitHair), 1.0f);
		}
		RitText(X.C, RitUpper(Label), R.X + W / 2.0f, R.Y + 16.0f, 14.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
		if (St.Clicked && !On)
		{
			X.RL.ShowBoard(B, X.Now);
		}
		Tx += W + 8.0f;
	}
	// Your standings on every board, right of the tabs (the value its own run).
	float Sx = 1576.0f;
	for (auto It = Shown.rbegin(); It != Shown.rend(); ++It)
	{
		net::BoardRow Row;
		Net.Leaderboard(*It, X.World, rlsites_detail::SitesHero(X.S, RitSite, Net, X.World, *It), 0, &Row);
		const std::string Value = Row.Value > 0.0 ? RitBoardValue(*It, Row.Value) : std::string("No score");
		const float Vw = RitText(X.C, Value, Sx, 117.0f, 14.0f, RitC(RitWhite), RitFace::Mono, Align::Right);
		const std::string Rank = Row.Rank > 0 ? "#" + Grouped(Row.Rank) : std::string("\xE2\x80\x94");
		const float Rw = RitText(X.C, Rank, Sx - Vw - 8.0f, 117.0f, 14.0f, RitC(RitChrome), RitFace::Cond, Align::Right);
		const float Lw = RitLabel(X.C, Name(*It), Sx - Vw - Rw - 16.0f, 117.0f, 12.0f, RitC(RitMuted), Align::Right);
		Sx -= Vw + Rw + Lw + 40.0f;
	}
	RitText(X.C, rlsites_detail::SitesBoardAbout(X.S, RitSite, Board, X.World), 232.0f, 152.0f, 13.0f, RitC(RitMuted), RitFace::Body, Align::Left, Baseline::Alphabetic, 1344.0f);
	// The table: rank, movement, the player, the board's value, a sparkline of their form.
	net::BoardRow Mine;
	const net::HeroStats Hero = rlsites_detail::SitesHero(X.S, RitSite, Net, X.World, Board);
	const std::vector<net::BoardRow> Rows = Net.Leaderboard(Board, X.World, Hero, 22, &Mine);
	const Rect T{232.0f, 168.0f, 1344.0f, 728.0f};
	X.C.FillRect({T.X, T.Y, T.W, 30.0f}, RitC(RitBg2));
	X.C.FillRect({T.X, T.Y + 29.0f, T.W, 1.0f}, RitC(RitHair));
	const float Hy = T.Y + 15.0f + 12.0f * 0.36f;
	RitLabel(X.C, "Rank", T.X + 16.0f, Hy, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Player", T.X + 150.0f, Hy, 12.0f, RitC(RitMuted));
	// EVENTS (§3.4 D, g1-audit L3): this season's events on RunItTwice, the world's site record for each player (their Season
	// and Titles are this season's too; the Series race keeps no count, so its tab says whose season it is).
	RitLabel(X.C, Board == net::Board::Series ? "Season events" : "Events", T.X + RitRankEvents, Hy, 12.0f, RitC(RitMuted), Align::Right);
	const site::Account* RankAcct = RitAccount(X.S);
	const int HeroSeasonEvents = RankAcct && RankAcct->Hero.Year == world::YearOf(net::DayOf(X.World)) ? RankAcct->Hero.Events : 0;
	const world::World& RankWorld = X.S.Living();
	const bool WorldRows = &Net.Players() == &RankWorld.View(); // the board's rows are the world's people (index == npc id)
	RitLabel(X.C, "Form", T.X + 860.0f, Hy, 12.0f, RitC(RitMuted));
	RitLabel(X.C, Board == net::Board::Wins ? "Titles" : "Points", T.X + T.W - 20.0f, Hy, 12.0f, RitC(RitMuted), Align::Right);
	const int HeroRank = Mine.Rank;
	const int Was = X.U.RankSeen.count(static_cast<int>(Board)) ? X.U.RankSeen[static_cast<int>(Board)] : 0;
	if (HeroRank > 0 && Was > 0 && HeroRank < Was)
	{
		RitSound(X, SoundId::RitRank, 0.4f); // you climbed since you last looked
	}
	X.U.RankSeen[static_cast<int>(Board)] = HeroRank;
	if (Rows.empty())
	{
		RitText(X.C, "Nobody's scored yet.", T.X + T.W / 2.0f, T.Y + 120.0f, 15.0f, RitC(RitMuted), RitFace::Body, Align::Center);
	}
	const double Since = X.Now - std::max(Access::BoardAt(X.RL), Access::PageAt(X.RL));
	const chart::Theme FormTheme = chart::RunItTwice();
	for (size_t I = 0; I < Rows.size() && I < 20; ++I)
	{
		const net::BoardRow& Rw = Rows[I];
		const float Y = T.Y + 32.0f + static_cast<float>(I) * 34.0f;
		const bool You = Rw.Player < 0;
		const float In = X.Preview ? 1.0f : static_cast<float>(Clamp01((Since - 0.02 * static_cast<double>(I)) / 0.25));
		const float A0 = X.C.GetAlpha();
		X.C.SetAlpha(A0 * In);
		X.C.FillRect({T.X, Y, T.W, 34.0f}, RitC(I % 2 == 0 ? RitCarbon : RitPanel2));
		if (You)
		{
			X.C.FillRect({T.X, Y, T.W, 1.0f}, RitC(RitWhite));
			X.C.FillRect({T.X, Y + 33.0f, T.W, 1.0f}, RitC(RitWhite));
		}
		const float Fy = Y + 17.0f + 18.0f * 0.36f;
		RitText(X.C, Grouped(Rw.Rank), T.X + 40.0f, Fy, 18.0f, RitC(You ? RitWhite : RitInk), RitFace::Cond, Align::Right);
		if (Rw.Move != 0 && Rw.Move != -Rw.Rank && !You)
		{
			RitTri(X.C, T.X + 70.0f, Y + 17.0f, 9.0f, Rw.Move > 0, RitC(Rw.Move > 0 ? RitUp : RitDown));
			RitText(X.C, std::to_string(std::abs(Rw.Move)), T.X + 79.0f, Y + 17.0f, 13.0f, RitC(Rw.Move > 0 ? RitUp : RitDown), RitFace::Cond, Align::Left, Baseline::Middle);
		}
		else
		{
			X.C.FillRect({T.X + 66.0f, Y + 16.0f, 8.0f, 1.5f}, RitC(RitDim));
		}
		const std::string Nm = You ? X.S.HeroName : Net.Players()[static_cast<size_t>(Rw.Player)].Name;
		RitSquareAvatar(X.C, T.X + 116.0f, Y + 5.0f, 24.0f, Nm, RitC(RitChrome, 0.0f));
		RitText(X.C, Nm, T.X + 150.0f, Y + 22.0f, 14.0f, RitC(You ? RitWhite : RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 520.0f);
		const world::SiteRecord* SeasonRec = You || !WorldRows ? nullptr : RankWorld.SiteRecordOf(RitSite, Rw.Player);
		const int SeasonEvents = You ? HeroSeasonEvents : SeasonRec ? SeasonRec->Events : 0;
		RitText(X.C, SeasonEvents > 0 ? Grouped(SeasonEvents) : std::string("\xE2\x80\x94"), T.X + RitRankEvents, Y + 17.0f + 15.0f * 0.36f, 15.0f,
			RitC(SeasonEvents > 0 ? (You ? RitWhite : RitInk) : RitMuted), RitFace::Mono, Align::Right);
		if (!You)
		{
			// Their form from the world's records: ChartArt's sparkline, Chrome with the last point Signal Red.
			chart::SparkOpts So;
			So.Width = 1.6f;
			So.Area = 0.0f;
			So.Last = true;
			So.LastCol = RitC(RitRed);
			So.Zero = true;
			chart::Sparkline(X.C, {T.X + 860.0f, Y + 8.0f, 140.0f, 20.0f}, std::vector<double>(Rw.Form.begin(), Rw.Form.end()), RitC(RitChrome), So, FormTheme);
		}
		RitText(X.C, RitBoardValue(Board, Rw.Value), T.X + T.W - 20.0f, Fy, 15.0f, RitC(You ? RitWhite : RitInk), RitFace::Mono, Align::Right);
		X.C.SetAlpha(A0);
	}
	// The YOU bar: a Signal Red slab with Hot White type (its value's baseline in 915-930: the test's window).
	const Rect Yb{232.0f, 905.0f, 1344.0f, 34.0f};
	RitPanelBegin();
	RitCutFill(X.C, Yb, 8.0f, RitC(RitRed));
	RitLedEdge(X.C, Yb.X, Yb.Y, Yb.W - 8.0f);
	RitText(X.C, "YOU", Yb.X + 16.0f, Yb.Y + 17.0f, 15.0f, RitC(RitWhite), RitFace::Cond, Align::Left, Baseline::Middle);
	RitText(X.C, Mine.Rank > 0 ? "#" + Grouped(Mine.Rank) : std::string("\xE2\x80\x94"), Yb.X + 60.0f, Yb.Y + 17.0f, 18.0f, RitC(RitWhite), RitFace::Cond, Align::Left, Baseline::Middle);
	RitText(X.C, X.S.HeroName, Yb.X + 150.0f, Yb.Y + 17.0f, 14.0f, RitC(RitWhite), RitFace::Name, Align::Left, Baseline::Middle);
	RitText(X.C, HeroSeasonEvents > 0 ? Grouped(HeroSeasonEvents) : std::string("\xE2\x80\x94"), Yb.X + RitRankEvents, Yb.Y + 17.0f, 15.0f, RitC(RitWhite), RitFace::Mono, Align::Right, Baseline::Middle);
	RitText(X.C, Mine.Rank > 0 ? RitBoardValue(Board, Mine.Value) : std::string("No score"), Yb.X + Yb.W - 20.0f, Yb.Y + 17.0f, 15.0f, RitC(RitWhite), RitFace::Mono, Align::Right,
		Baseline::Middle);
}

// ------------------------------------------------------------------ THE DESK (Page::Desk, §3.4 E, R7: G1 from History)

/**
 * The Desk's chart through ui-rerun's chart kit (ChartArt's LineChart in RunItTwice's theme): the cumulative net as a 2-unit Hot
 * White line over a white area at 0.06, its last point a Signal Red dot with a red tick on the axis (§3.4 E).
 */
void RitDeskChart(RitCtx& X, const Rect& R, const std::vector<site::show::NetPoint>& Net)
{
	RitPanel(X.C, R, 10.0f);
	RitLabel(X.C, "Cumulative net", R.X + 20.0f, R.Y + 28.0f, 12.0f, RitC(RitMuted));
	chart::Series Line;
	chart::LineOpts O;
	int Shown = -1;
	for (const site::show::NetPoint& N : Net)
	{
		// A date under the first result of each day (several a day share one).
		const int PlayedOn = net::DayOf(N.At);
		Line.Y.push_back(static_cast<double>(N.Net));
		O.X.Labels.push_back(PlayedOn != Shown ? chart::AxisDate(PlayedOn) : std::string());
		Shown = PlayedOn;
	}
	Line.Col = RitC(RitWhite);
	Line.Width = 2.0f;
	Line.Area = 0.06f;
	Line.AreaFade = 1.0f;
	Line.Last = true;
	Line.LastCol = RitC(RitRed);
	Line.Name = "Net";
	O.Y = chart::Units::Money;
	O.Zero = true;
	// Nothing played yet: the empty note in the grid and no money axis (g1-reaudit: a range of one cent printed "$0.01" three
	// times over "$0.00" on a new account's Desk).
	O.YLabels = !Net.empty();
	O.Reveal = X.Preview ? 1.0f : static_cast<float>(Clamp01((X.Now - Access::PageAt(X.RL)) / 0.6));
	O.Mouse = X.Preview || !X.UI.Ptr.Active ? Vec2{-1.0f, -1.0f} : Vec2{X.UI.Ptr.X, X.UI.Ptr.Y};
	O.Empty = RitDeskEmpty; // cached (the default): ChartArt keys its layers by the data; the reveal's clip draws live
	const chart::Theme T = chart::RunItTwice();
	const Rect Plot{R.X + 16.0f, R.Y + 40.0f, R.W - 32.0f, R.H - 52.0f};
	const chart::LineOut Out = chart::LineChart(X.C, Plot, {Line}, O, T);
	if (Out.Hover >= 0 && Out.Hover < static_cast<int>(Net.size()))
	{
		const site::show::NetPoint& P = Net[static_cast<size_t>(Out.Hover)];
		chart::Tooltip(X.C, Out.HoverAt, chart::AxisDate(net::DayOf(P.At)), {{"Net", (P.Net < 0 ? "-" : "+") + Money(P.Net < 0 ? -P.Net : P.Net)}}, T, R);
	}
}

void RitDeskPage(RitCtx& X)
{
	const site::Account* A = RitAccount(X.S);
	RitPanelBegin();
	// The tabs: LEDGER and TAPE (the Luck Ledger is G2; SPLITS P1), the active one a Signal Red slab; the range.
	const char* const Tabs[2] = {"LEDGER", "TAPE"};
	float Tx = 232.0f;
	for (int I = 0; I < 2; ++I)
	{
		const float W = RitMeasure(X.C, Tabs[I], 14.0f) + 28.0f;
		const Rect R{Tx, 92.0f, W, 28.0f};
		const Ui::ClickState St = RitClick(X, std::string("desk") + Tabs[I], R);
		const bool On = X.U.DeskTab == I;
		RitCutFill(X.C, R, 6.0f, RitC(On ? RitRed : St.Hover ? RitPanel2 : RitCarbon));
		RitText(X.C, Tabs[I], R.X + W / 2.0f, R.Y + 14.0f, 14.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
		if (St.Clicked)
		{
			X.U.DeskTab = I;
		}
		Tx += W + 8.0f;
	}
	const char* const Ranges[2] = {"LAST 60 DAYS", "ALL"};
	float Rx = 1576.0f;
	for (int I = 1; I >= 0; --I)
	{
		const float W = RitMeasure(X.C, Ranges[I], 13.0f) + 24.0f;
		Rx -= W;
		const Rect R{Rx, 92.0f, W, 28.0f};
		const Ui::ClickState St = RitClick(X, std::string("deskrange") + std::to_string(I), R);
		const bool On = X.U.DeskAll == I;
		RitCutFill(X.C, R, 6.0f, RitC(On ? RitPanel2 : RitCarbon));
		RitCutStroke(X.C, R, 6.0f, RitC(On ? RitChrome : RitHair), 1.0f);
		RitText(X.C, Ranges[I], R.X + W / 2.0f, R.Y + 14.0f, 13.0f, RitC(On ? RitWhite : RitMuted), RitFace::Cond, Align::Center, Baseline::Middle);
		if (St.Clicked)
		{
			X.U.DeskAll = I;
		}
		Rx -= 8.0f;
	}
	const int Range = X.U.DeskAll == 1 ? 0 : 60;
	// No account yet: an empty one on RunItTwice (Account's default site is RiverLine, whose rows the Desk would count).
	site::Account Empty;
	Empty.Site = RitSite;
	const site::show::DeskKpis K = site::show::Desk(X.S.History, A ? *A : Empty, Range, X.World);
	const site::show::DeskKpis Recent = site::show::Desk(X.S.History, A ? *A : Empty, 30, X.World);
	// The KPI tiles: BoldCondensed 32, twice-printed, the last 30 days under each in Signal Up or Signal Down.
	struct Tile
	{
		const char* Label;
		std::string Figure;
		std::string Delta;
		int Sign;
	};
	const auto Pct = [](double V) { return Fixed(V * 100.0, 1) + "%"; };
	const auto Signed = [](Chips V) { return (V < 0 ? "-" : "+") + Money(V < 0 ? -V : V); };
	// RAKE is the rake actually paid (site::show::RakePaid, the Ledger's RAKE PAID too): the account's tally, all time, as the
	// CLUB tile is the month's; under it this month's (G1_AUDIT L5).
	const Chips RakeMonth = A && A->MonthKey == site::MonthKeyAt(X.World) ? A->RakeMonth : 0;
	const Tile Tiles[8] = {
		{"Events", Grouped(K.Events), Grouped(Recent.Events) + " in 30d", 0},
		{"ROI", Pct(K.Roi), Pct(Recent.Roi) + " 30d", Recent.Events > 0 ? (Recent.Roi >= 0.0 ? 1 : -1) : 0},
		{"ITM", Pct(K.Itm), Pct(Recent.Itm) + " 30d", 0},
		{"FT", Pct(K.Ft), Pct(Recent.Ft) + " 30d", 0},
		{"ABI", Money(K.Abi), Money(Recent.Abi) + " 30d", 0},
		{"Net", Signed(K.Net), Signed(Recent.Net) + " 30d", Recent.Events > 0 ? (Recent.Net >= 0 ? 1 : -1) : 0},
		{"Rake paid", Money(K.Rake), Money(RakeMonth) + " this month", 0},
		{"Club", Money(K.Club), "accruing this month", 0},
	};
	for (int I = 0; I < 8; ++I)
	{
		const Rect R{232.0f + static_cast<float>(I) * 169.0f, 130.0f, 161.0f, 76.0f};
		RitPanelBegin();
		RitCutFill(X.C, R, 10.0f, RitGlossPaint(R));
		RitCutStroke(X.C, R, 10.0f, RitC(RitHair), 1.0f);
		RitLabel(X.C, Tiles[I].Label, R.X + 12.0f, R.Y + 18.0f, 12.0f, RitC(RitMuted));
		const bool Loss = Tiles[I].Figure.size() > 0 && Tiles[I].Figure[0] == '-';
		if (Loss)
		{
			RitText(X.C, Tiles[I].Figure, R.X + 12.0f, R.Y + 52.0f, RitFit(X.C, Tiles[I].Figure, 32.0f, 22.0f, R.W - 24.0f), RitC(RitDown)); // a loss prints once, in Signal Down
		}
		else
		{
			RitTwice(X.C, Tiles[I].Figure, R.X + 12.0f, R.Y + 52.0f, RitFit(X.C, Tiles[I].Figure, 32.0f, 22.0f, R.W - 24.0f), RitC(RitWhite));
		}
		float Dx = R.X + 12.0f;
		if (Tiles[I].Sign != 0)
		{
			RitTri(X.C, Dx + 4.0f, R.Y + 66.0f, 8.0f, Tiles[I].Sign > 0, RitC(Tiles[I].Sign > 0 ? RitUp : RitDown));
			Dx += 12.0f;
		}
		RitText(X.C, Tiles[I].Delta, Dx, R.Y + 70.0f, 11.0f, RitC(Tiles[I].Sign > 0 ? RitUp : Tiles[I].Sign < 0 ? RitDown : RitMuted), RitFace::Mono, Align::Left, Baseline::Alphabetic, R.W - 24.0f);
	}
	// The chart: the cumulative net (ChartArt's LineChart once it lands; the placeholder line until then).
	RitDeskChart(X, {232.0f, 222.0f, 1344.0f, 300.0f}, site::show::DeskNet(X.S.History, RitSite, Range, X.World));
	// Results: History's RunItTwice rows.
	const Rect Res{232.0f, 538.0f, 664.0f, 412.0f};
	RitPanel(X.C, Res, 10.0f);
	RitLabel(X.C, "Results", Res.X + 20.0f, Res.Y + 28.0f, 12.0f, RitC(RitMuted));
	float Ry = Res.Y + 56.0f;
	int Listed = 0;
	for (const HistoryEntry& H : X.S.History)
	{
		if (net::SiteOf(H.EventId) != RitSite || Ry > Res.Y + Res.H - 20.0f)
		{
			continue;
		}
		const double At = [&H]() {
			const size_t P = H.EventId.find('@');
			return P == std::string::npos ? 0.0 : std::atof(H.EventId.c_str() + P + 1);
		}();
		const Chips Net = H.Prize - std::max<Chips>(0, H.BuyInCents);
		X.C.FillRect({Res.X + 12.0f, Ry - 18.0f, Res.W - 24.0f, 30.0f}, RitC(Listed % 2 == 0 ? RitCarbon : RitPanel2));
		RitText(X.C, RitDate(net::DayOf(At)), Res.X + 24.0f, Ry, 12.0f, RitC(RitChrome), RitFace::Mono);
		RitText(X.C, H.Name, Res.X + 100.0f, Ry, 14.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, 250.0f);
		RitText(X.C, Ordinal(H.Place) + " of " + Grouped(H.Entrants), Res.X + 470.0f, Ry, 13.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
		RitText(X.C, (Net < 0 ? "-" : "+") + Money(Net < 0 ? -Net : Net), Res.X + Res.W - 24.0f, Ry, 13.0f, RitC(Net < 0 ? RitDown : Net > 0 ? RitUp : RitMuted), RitFace::Mono, Align::Right);
		Ry += 32.0f;
		++Listed;
	}
	if (Listed == 0)
	{
		RitParagraph(X.C, RitDeskEmpty, Res.X + 20.0f, Ry, Res.W - 40.0f, 13.0f, RitC(RitMuted), 18.0f, 2);
	}
	// The right panel by tab: the ledger's best (LEDGER), or the hands kept for Rerun (TAPE).
	const Rect Side{912.0f, 538.0f, 664.0f, 412.0f};
	RitPanel(X.C, Side, 10.0f);
	if (X.U.DeskTab == 1)
	{
		RitLabel(X.C, "Tape", Side.X + 20.0f, Side.Y + 28.0f, 12.0f, RitC(RitMuted));
		int Kept = 0;
		float Ty = Side.Y + 60.0f;
		for (const video::Candidate& Cd : X.S.VideoCandidates())
		{
			if (Cd.Site != static_cast<int>(RitSite) || Kept >= 4)
			{
				continue;
			}
			for (size_t I = 0; I < Cd.Rec.Board.size() && I < 5; ++I)
			{
				DrawCard(X.C, Cd.Rec.Board[I], Side.X + 20.0f + static_cast<float>(I) * 30.0f, Ty, 26.0f, 36.0f);
			}
			RitText(X.C, Cd.Event, Side.X + 180.0f, Ty + 14.0f, 14.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, 300.0f);
			RitText(X.C, "pot " + ChipsText(static_cast<double>(Cd.Rec.Pot)) + " \xC2\xB7 " + (Cd.Rec.HeroNet >= 0 ? "+" : "") + ChipsText(static_cast<double>(Cd.Rec.HeroNet)), Side.X + 180.0f, Ty + 32.0f,
				12.0f, RitC(RitMuted), RitFace::Mono);
			const Rect Cut{Side.X + Side.W - 150.0f, Ty + 2.0f, 130.0f, 32.0f};
			const Ui::ClickState St = RitClick(X, "deskcut" + std::to_string(Kept), Cut);
			RitSlab(X.C, Cut, 6.0f, St.Hover && !X.Preview, X.Now);
			RitText(X.C, "CUT A VIDEO", Cut.X + Cut.W / 2.0f, Cut.Y + 16.0f, 13.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
			if (St.Clicked)
			{
				rlvideo_detail::VideoOpen(X.RL, 0, X.Now); // Rerun's Studio; PUBLISH keeps its clause warning
			}
			Ty += 56.0f;
			++Kept;
		}
		RitParagraph(X.C, Kept > 0 ? RitDeskTape : RitDeskNoTape, Side.X + 20.0f, Side.Y + Side.H - 40.0f, Side.W - 40.0f, 12.0f, RitC(RitMuted), 16.0f, 2);
	}
	else
	{
		RitLabel(X.C, "Ledger", Side.X + 20.0f, Side.Y + 28.0f, 12.0f, RitC(RitMuted));
		const site::show::DeskKpis All = site::show::Desk(X.S.History, A ? *A : Empty, 0, X.World);
		Chips Best = 0;
		int Wins = 0;
		int Fts = 0;
		for (const HistoryEntry& H : X.S.History)
		{
			if (net::SiteOf(H.EventId) == RitSite)
			{
				Best = std::max(Best, H.Prize);
				Wins += H.Place == 1 ? 1 : 0;
				Fts += site::show::MadeFinalTable(H) ? 1 : 0; // the event's last table: the last 8 at 8-max
			}
		}
		const std::pair<std::string, std::string> Lines[6] = {{"EVENTS, ALL TIME", Grouped(All.Events)}, {"NET, ALL TIME", Signed(All.Net)}, {"BIGGEST CASH", Money(Best)},
			{"FINAL TABLES", Grouped(Fts)}, {"WINS", Grouped(Wins)}, {"RAKE PAID", Money(All.Rake)}};
		for (int I = 0; I < 6; ++I)
		{
			RitTerm(X, Side.X + 20.0f, Side.X + Side.W - 20.0f, Side.Y + 64.0f + static_cast<float>(I) * 30.0f, Lines[I].first, Lines[I].second, I == 2 ? RitWhite : RitInk);
		}
	}
}

// ------------------------------------------------------------------ SERIES (Page::Series, §3.4 F): the programme

void RitSeriesPage(RitCtx& X)
{
	const net::Network& Net = net::Site(RitSite);
	const net::SeriesInfo* Sr = Net.CurrentSeries(X.World);
	RitPanelBegin();
	if (!Sr)
	{
		RitText(X.C, RitSeriesNone, 232.0f, 140.0f, 15.0f, RitC(RitMuted), RitFace::Body);
		return;
	}
	const int Today = net::DayOf(X.World);
	RitTwice(X.C, RitUpper(Sr->Name), 232.0f, 132.0f, 34.0f, RitC(RitWhite));
	RitText(X.C, RitDate(Sr->FirstDay) + " \xE2\x80\x93 " + RitDate(Sr->LastDay) + " \xC2\xB7 " + std::to_string(Sr->Events) + " EVENTS" + (Sr->GtdCents > 0 ? " \xC2\xB7 " + rlnet_detail::NetMoney(Sr->GtdCents) + " GTD" : std::string()),
		232.0f, 160.0f, 15.0f, RitC(RitChrome), RitFace::Cond);
	if (!Sr->Tagline.empty())
	{
		RitText(X.C, Sr->Tagline, 1576.0f, 132.0f, 14.0f, RitC(RitMuted), RitFace::Body, Align::Right, Baseline::Alphabetic, 640.0f);
	}
	// The programme: a column a day, each event's number in BoldCondensed and its short name.
	std::vector<const net::EventTemplate*> Events;
	const net::EventTemplate* Main = nullptr;
	for (const net::EventTemplate& T : Net.Templates())
	{
		if (T.Series == Sr->Id)
		{
			Events.push_back(&T);
			Main = T.Main || T.Id == Sr->MainEvent ? &T : Main;
		}
	}
	const int Span = std::max(1, Sr->LastDay - Sr->FirstDay + 1);
	const Rect Grid{232.0f, 180.0f, 1344.0f, Main ? 560.0f : 760.0f};
	RitPanel(X.C, Grid, 10.0f);
	const float Cw = (Grid.W - 20.0f) / static_cast<float>(Span);
	std::vector<int> Stack(static_cast<size_t>(Span), 0);
	for (int D = 0; D < Span; ++D)
	{
		const float Cx = Grid.X + 10.0f + Cw * static_cast<float>(D);
		const bool Now = Sr->FirstDay + D == Today;
		if (Now)
		{
			X.C.FillRect({Cx, Grid.Y + 8.0f, Cw - 2.0f, Grid.H - 16.0f}, RitC(RitSelRow));
			X.C.FillRect({Cx, Grid.Y + 8.0f, Cw - 2.0f, 3.0f}, RitC(RitRed));
		}
		RitText(X.C, RitDay(Sr->FirstDay + D).substr(0, 3), Cx + 6.0f, Grid.Y + 28.0f, 12.0f, RitC(Now ? RitWhite : RitMuted), RitFace::Cond);
		RitText(X.C, std::to_string(D + 1), Cx + Cw - 8.0f, Grid.Y + 28.0f, 12.0f, RitC(Now ? RitWhite : RitMuted), RitFace::Mono, Align::Right);
	}
	for (const net::EventTemplate* T : Events)
	{
		const int D = T->OnlyDay == -9999 ? 0 : std::min(std::max(T->OnlyDay - Sr->FirstDay, 0), Span - 1);
		const int K = Stack[static_cast<size_t>(D)]++;
		const float Cx = Grid.X + 10.0f + Cw * static_cast<float>(D);
		const float Ey = Grid.Y + 50.0f + static_cast<float>(K) * 44.0f;
		if (Ey > Grid.Y + Grid.H - 40.0f)
		{
			continue;
		}
		const bool Past = T->OnlyDay < Today;
		RitText(X.C, "#" + std::to_string(T->EventNo), Cx + 6.0f, Ey + 14.0f, 15.0f, RitC(Past ? RitMuted : RitWhite), RitFace::Cond);
		RitText(X.C, RitBuyIn(T->BuyInCents), Cx + Cw - 8.0f, Ey + 14.0f, 11.0f, RitC(RitChrome), RitFace::Mono, Align::Right);
		// The name without its price (the cell's corner already prints the buy-in); a played one in Muted, never Dim (Dim
		// carries no information, §3.3).
		RitText(X.C, RitEventName(T->Name), Cx + 6.0f, Ey + 30.0f, 11.0f, RitC(Past ? RitMuted : RitChrome), RitFace::Body, Align::Left, Baseline::Alphabetic, Cw - 12.0f);
	}
	if (Main)
	{
		// The main event as a framed card under a Signal Red top bar.
		const Rect Mc{232.0f, 756.0f, 1344.0f, 194.0f};
		RitPanel(X.C, Mc, 10.0f);
		X.C.FillRect({Mc.X, Mc.Y, Mc.W - 10.0f, 4.0f}, RitC(RitRed));
		RitLabel(X.C, "Main event \xC2\xB7 #" + std::to_string(Main->EventNo), Mc.X + 24.0f, Mc.Y + 32.0f, 12.0f, RitC(RitMuted));
		RitTwice(X.C, RitUpper(rlnet_detail::NetShortName(Main->Name)), Mc.X + 24.0f, Mc.Y + 80.0f, 40.0f, RitC(RitWhite));
		RitText(X.C, RitBuyIn(Main->BuyInCents) + " BUY-IN" + (Main->GtdCents > 0 ? " \xC2\xB7 " + rlnet_detail::NetMoney(Main->GtdCents) + " GUARANTEED" : std::string()), Mc.X + 24.0f, Mc.Y + 116.0f,
			18.0f, RitC(RitChrome), RitFace::Cond);
		if (Main->OnlyDay != -9999)
		{
			RitText(X.C, RitDay(Main->OnlyDay) + " " + RitDate(Main->OnlyDay) + " \xC2\xB7 " + RitClock(Main->StartMinute), Mc.X + 24.0f, Mc.Y + 144.0f, 14.0f, RitC(RitMuted), RitFace::Mono);
		}
		if (!Main->Blurb.empty())
		{
			RitParagraph(X.C, Main->Blurb, Mc.X + 700.0f, Mc.Y + 60.0f, 600.0f, 14.0f, RitC(RitMuted), 20.0f, 4);
		}
	}
}

// ------------------------------------------------------------------ NEWSDESK (Page::News, §3.4 F): the news, slug by slug

void RitNewsPage(RitCtx& X)
{
	const std::vector<net::NewsItem>& News = Access::NewsFeed(X.RL);
	RitPanelBegin();
	const Rect List{232.0f, 96.0f, 860.0f, 854.0f};
	RitPanel(X.C, List, 10.0f);
	if (News.empty())
	{
		RitText(X.C, RitNewsEmpty, List.X + 24.0f, List.Y + 40.0f, 15.0f, RitC(RitMuted), RitFace::Body);
		return;
	}
	X.U.NewsPick = std::min(std::max(X.U.NewsPick, 0), static_cast<int>(News.size()) - 1);
	float Y = List.Y + 12.0f;
	for (size_t I = 0; I < News.size() && Y + 52.0f < List.Y + List.H; ++I)
	{
		const net::NewsItem& It = News[I];
		const Rect R{List.X + 8.0f, Y, List.W - 16.0f, 50.0f};
		const Ui::ClickState St = RitClick(X, "news" + std::to_string(I), R);
		const bool On = static_cast<int>(I) == X.U.NewsPick;
		if (On || St.Hover)
		{
			X.C.FillRect(R, RitC(On ? RitSelRow : RitPanel2));
		}
		if (On)
		{
			X.C.FillRect({R.X, R.Y, 3.0f, R.H}, RitC(RitRed));
		}
		const bool Earlier = net::DayOf(It.At) != net::DayOf(X.World);
		RitText(X.C, (Earlier ? RitDay(net::DayOf(It.At)) + " " : std::string()) + RitClock(It.At), R.X + 16.0f, R.Y + 30.0f, 12.0f, RitC(RitChrome), RitFace::Mono);
		// The slug: RESULT and SERIES Chrome; an overlay amber; TEAM and SCALP Hot White on Signal Red; the rest an Ink outline.
		std::string Slug = "NEWS";
		uint32_t Fill = RitInk;
		uint32_t Ink = RitInk;
		bool Outline = true;
		const std::string Low = It.Title;
		switch (It.Kind)
		{
		case net::NewsKind::BigWin: Slug = "RESULT"; Fill = Ink = RitChrome; break;
		case net::NewsKind::Series: Slug = "SERIES"; Fill = Ink = RitChrome; break;
		case net::NewsKind::Schedule: Slug = "SCHEDULE"; break;
		case net::NewsKind::Record: Slug = "RECORD"; break;
		case net::NewsKind::Hero: Slug = "YOU"; Fill = Ink = RitWhite; break;
		case net::NewsKind::People: Slug = "PEOPLE"; break;
		}
		if (Low.find("overlay") != std::string::npos || Low.find("Overlay") != std::string::npos)
		{
			Slug = "OVERLAY";
			Fill = RitAmber;
			Ink = RitOnyx;
			Outline = false;
		}
		else if (Low.find(net::Def(RitSite).TeamName) != std::string::npos)
		{
			Slug = "TEAM";
			Fill = RitRed;
			Ink = RitWhite;
			Outline = false;
		}
		const float Sw = RitTag(X.C, R.X + 96.0f, R.Y + 17.0f, Slug, Fill, Ink, 11.0f, 5.0f, 16.0f, 4.0f, Outline);
		RitText(X.C, It.Title, R.X + 106.0f + Sw, R.Y + 30.0f, 14.0f, RitC(RitInk), RitFace::Body, Align::Left, Baseline::Alphabetic, R.W - 130.0f - Sw);
		if (St.Clicked && !On)
		{
			X.U.NewsPick = static_cast<int>(I);
			RitSound(X, SoundId::RitBlip, 0.3f);
		}
		Y += 52.0f;
	}
	// The selected item: its figure twice-printed, its line in full.
	const net::NewsItem& Pick = News[static_cast<size_t>(X.U.NewsPick)];
	const Rect Side{1108.0f, 96.0f, 468.0f, 854.0f};
	RitPanel(X.C, Side, 10.0f);
	RitText(X.C, RitDay(net::DayOf(Pick.At)) + " " + RitDate(net::DayOf(Pick.At)) + " \xC2\xB7 " + RitClock(Pick.At), Side.X + 24.0f, Side.Y + 36.0f, 12.0f, RitC(RitChrome), RitFace::Mono);
	float Py = RitParagraph(X.C, Pick.Title, Side.X + 24.0f, Side.Y + 76.0f, Side.W - 48.0f, 20.0f, RitC(RitInk), 26.0f, 4) + 10.0f;
	if (Pick.Amount > 0)
	{
		RitTwice(X.C, Money(Pick.Amount), Side.X + 24.0f, Py + 36.0f, 40.0f, RitC(RitWhite));
		Py += 64.0f;
	}
	RitParagraph(X.C, Pick.Body, Side.X + 24.0f, Py, Side.W - 48.0f, 14.0f, RitC(RitMuted), 20.0f, 20);
}

// ------------------------------------------------------------------ THE CLUB (Page::Promotions, §3.4 G, R5: the card and accrual)

/** A Club card's ground in the status ramp (§3.3): no tier a chrome outline, Member carbon twill, Regular brushed chrome, High
 * Table Status Gold, Inner Circle red lacquer with black twin stripes. Returns the ink its type takes. */
uint32_t RitCardGround(Canvas& C, const Rect& R, int Tier)
{
	switch (Tier)
	{
	case 1:
		RitCutFill(C, R, 10.0f, RitC(RitTwillA));
		C.PushClip(R);
		RitTwill(C, R);
		C.PopClip();
		RitCutStroke(C, R, 10.0f, RitC(RitChromeDark), 1.0f);
		return RitInk;
	case 2:
		RitCutFill(C, R, 10.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, RitC(RitChromeDark), RitC(RitChromeLight)));
		return RitOnyx;
	case 3:
		RitCutFill(C, R, 10.0f, Paint::Linear({R.X, R.Y}, {R.X + R.W, R.Y + R.H}, RitC(RitGoldDark), RitC(RitGoldLight)));
		return RitOnyx;
	case 4:
		RitCutFill(C, R, 10.0f, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, RitC(0xe3262c), RitC(RitDeep)));
		RitTwinV(C, R.X + R.W * 0.62f, R.Y, R.H, 1.6f, RitOnyx);
		return RitWhite;
	default:
		RitCutFill(C, R, 10.0f, RitC(RitOnyx));
		RitCutStroke(C, R, 10.0f, RitC(RitChrome), 1.5f);
		return RitInk;
	}
}

/** The member number, embossed on the card and the file: Fnv1a of the account's key. */
std::string RitMemberNo(const Session& S)
{
	const uint32_t H = Fnv1a(S.HeroName + ":" + net::KeyOf(RitSite));
	const std::string D = std::to_string(1000000000u + H % 900000000u);
	return D.substr(0, 4) + " " + D.substr(4, 4) + " " + D.substr(8);
}

void RitClubCard(RitCtx& X, const Rect& R, const site::show::Accrual& Ac)
{
	const site::Account* A = RitAccount(X.S);
	// A tier change re-mints the card (a white light sweep and RitStamp), once.
	if (X.U.CardTier >= 0 && X.U.CardTier != Ac.Tier)
	{
		X.U.MintAt = X.Now;
		RitSound(X, SoundId::RitStamp, 0.45f);
	}
	X.U.CardTier = Ac.Tier;
	const Ui::ClickState St = RitClick(X, "clubcard", R);
	if (St.Clicked && X.Now - X.U.CardAt > 0.3)
	{
		X.U.CardAt = X.Now;
		X.U.CardBack = !X.U.CardBack;
	}
	// The turn: a 0.3 s x-squash; the text swaps at the midpoint and is never drawn squashed (only the ground turns).
	const double T = X.Preview ? 1.0 : Clamp01((X.Now - X.U.CardAt) / 0.3);
	const bool Turning = T < 1.0;
	const float Squash = Turning ? static_cast<float>(std::fabs(std::cos(T * 3.14159265))) : 1.0f;
	const bool Back = Turning ? (T < 0.5 ? !X.U.CardBack : X.U.CardBack) : X.U.CardBack;
	const Rect G{R.X + R.W * (1.0f - Squash) / 2.0f, R.Y, std::max(2.0f, R.W * Squash), R.H};
	RitGlow(X.C, {G.X, G.Y + 10.0f, G.W, G.H}, 0x000000, 0.5f, 18.0f);
	const uint32_t Ink = RitCardGround(X.C, G, Ac.Tier);
	if (!Turning)
	{
		const std::string Tier = Ac.Tier > 0 ? RitUpper(RitTierName(Ac.Tier)) : std::string("NO TIER YET");
		if (!Back)
		{
			RitLabel(X.C, "RunItTwice \xC2\xB7 Second Run Club", R.X + 24.0f, R.Y + 34.0f, 13.0f, RitC(Ink, 0.85f));
			siteart::Logo(X.C, {R.X + R.W - 78.0f, R.Y + 18.0f, 54.0f, 54.0f}, RitSite);
			if (Ink == RitOnyx)
			{
				RitText(X.C, Tier, R.X + 24.0f, R.Y + 120.0f, 40.0f, RitC(Ink));
			}
			else
			{
				RitTwice(X.C, Tier, R.X + 24.0f, R.Y + 120.0f, 40.0f, RitC(Ink), Ac.Tier == 4 ? RitDeep : RitRed);
			}
			RitText(X.C, RitMemberNo(X.S), R.X + 24.0f, R.Y + 190.0f, 22.0f, RitC(Ink), RitFace::Mono);
			RitText(X.C, X.S.HeroName, R.X + 24.0f, R.Y + 236.0f, 16.0f, RitC(Ink), RitFace::Name);
			RitLabel(X.C, "Member since " + (A ? RitUpper(net::DateLabel(net::DayOf(A->JoinedAt))) : std::string("today")), R.X + R.W - 24.0f, R.Y + 262.0f, 12.0f, RitC(Ink, 0.85f), Align::Right);
		}
		else
		{
			RitLabel(X.C, "The rate", R.X + 24.0f, R.Y + 34.0f, 13.0f, RitC(Ink, 0.85f));
			if (Ink == RitOnyx)
			{
				RitText(X.C, RitTierPct(Ac.Tier) + " BACK", R.X + 24.0f, R.Y + 104.0f, 48.0f, RitC(Ink));
			}
			else
			{
				RitTwice(X.C, RitTierPct(Ac.Tier) + " BACK", R.X + 24.0f, R.Y + 104.0f, 48.0f, RitC(Ink), Ac.Tier == 4 ? RitDeep : RitRed);
			}
			RitText(X.C, RitClubPaid, R.X + 24.0f, R.Y + 136.0f, 14.0f, RitC(Ink), RitFace::Body);
			X.C.FillRect({R.X + 24.0f, R.Y + 230.0f, R.W - 48.0f, 1.0f}, RitC(Ink, 0.6f));
			RitText(X.C, X.S.HeroName, R.X + 30.0f, R.Y + 222.0f, 22.0f, RitC(Ink), RitFace::Name);
			RitLabel(X.C, "Signature", R.X + 24.0f, R.Y + 250.0f, 11.0f, RitC(Ink, 0.85f));
		}
	}
	if (!X.Preview && X.Now - X.U.MintAt < 0.4)
	{
		RitSweep(X.C, R, static_cast<float>(-0.2 + 1.4 * (X.Now - X.U.MintAt) / 0.4), 60.0f);
	}
	RitText(X.C, RitClubTurn, R.X + R.W, R.Y - 18.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Right);
}

/** The accrual meter's position: four equal segments, $50 / $250 / $1,000 / $4,000 at their ends. */
float RitClubMeter(Chips Rake)
{
	const double Marks[5] = {0.0, 5000.0, 25000.0, 100000.0, 400000.0};
	const double R = static_cast<double>(Rake);
	for (int I = 0; I < 4; ++I)
	{
		if (R < Marks[I + 1])
		{
			return static_cast<float>((static_cast<double>(I) + (R - Marks[I]) / (Marks[I + 1] - Marks[I])) / 4.0);
		}
	}
	return 1.0f;
}

void RitClubPage(RitCtx& X)
{
	const site::Account* A = RitAccount(X.S);
	const site::show::Accrual Ac = RitAccrual(X.S);
	RitPanelBegin();
	RitSpaced(X.C, "SECOND RUN CLUB", 232.0f, 118.0f, 13.0f, RitC(RitMuted), 1.5f); // the kicker the sites' test spells
	RitText(X.C, RitAttitude, 1576.0f, 118.0f, 14.0f, RitC(RitInk), RitFace::Body, Align::Right);
	RitClubCard(X, {232.0f, 136.0f, 520.0f, 290.0f}, Ac);
	// The accrual (G1: site::show::ClubAccrual, a pure function of the month's rake; provisional until the 1st).
	const Rect Acc{768.0f, 136.0f, 808.0f, 290.0f};
	RitPanel(X.C, Acc, 10.0f);
	const double NextMonth = site::NextMonthStart(X.World);
	RitLabel(X.C, Ac.Tier > 0 ? "Accrued at " + RitTierName(Ac.Tier) + " (" + RitTierPct(Ac.Tier) + ")" : std::string("Accrued this month"), Acc.X + 24.0f, Acc.Y + 34.0f, 13.0f, RitC(RitInk));
	RitText(X.C, "PROVISIONAL \xC2\xB7 PAID " + RitDate(net::DayOf(NextMonth)), Acc.X + Acc.W - 24.0f, Acc.Y + 34.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	RitTwice(X.C, Money(Ac.Accrued), Acc.X + 24.0f, Acc.Y + 92.0f, 34.0f, RitC(RitWhite));
	RitText(X.C, RitUpper(RitToNextLine(Ac)), Acc.X + Acc.W - 24.0f, Acc.Y + 86.0f, 15.0f, RitC(RitInk), RitFace::Cond, Align::Right, Baseline::Alphabetic, 420.0f);
	// The segmented meter: a Carbon track, a Signal Red fill, Hot White tier ticks.
	const Rect M{Acc.X + 24.0f, Acc.Y + 130.0f, Acc.W - 48.0f, 10.0f};
	const float Pos = RitClubMeter(Ac.Rake);
	// The track in Onyx, as the Settlement's meter: a Carbon track vanished into the Carbon card (g1-audit).
	X.C.FillRect(M, RitC(RitOnyx));
	X.C.FillRect({M.X, M.Y, M.W * Pos, M.H}, RitC(RitRed));
	RitLedEdge(X.C, M.X, M.Y, M.W * Pos);
	const char* const Ticks[4] = {"$50 MEMBER", "$250 REGULAR", "$1,000 HIGH TABLE", "$4,000 INNER CIRCLE"};
	for (int I = 0; I < 4; ++I)
	{
		const float Tx = M.X + M.W * static_cast<float>(I + 1) / 4.0f;
		X.C.FillRect({Tx - 1.0f, M.Y - 4.0f, 2.0f, M.H + 8.0f}, RitC(RitWhite));
		RitText(X.C, Ticks[I], Tx - 6.0f, M.Y + 30.0f, 11.0f, RitC(Ac.Tier > I ? RitInk : RitMuted), RitFace::Cond, Align::Right);
	}
	const float Mx = M.X + M.W * Pos;
	X.C.FillPolygon({{Mx - 5.0f, M.Y - 10.0f}, {Mx + 5.0f, M.Y - 10.0f}, {Mx, M.Y - 3.0f}}, RitC(RitWhite));
	RitText(X.C, "Rake this month " + Money(Ac.Rake), Acc.X + Acc.W - 24.0f, Acc.Y + Acc.H - 24.0f, 13.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	RitText(X.C, RitProvisional, Acc.X + 24.0f, Acc.Y + Acc.H - 24.0f, 12.0f, RitC(RitMuted), RitFace::Body);
	// The tiers (the Club's table, the player's marked) and Club Day's printout (the last month the Club paid).
	const Rect Tiers{232.0f, 442.0f, 660.0f, 300.0f};
	RitPanel(X.C, Tiers, 10.0f);
	RitLabel(X.C, "The tiers", Tiers.X + 24.0f, Tiers.Y + 32.0f, 12.0f, RitC(RitMuted));
	const std::vector<site::TierInfo>& Club = site::SecondRunTiers();
	for (int I = 1; I < static_cast<int>(Club.size()) && I < 5; ++I)
	{
		const float Ty = Tiers.Y + 56.0f + static_cast<float>(I - 1) * 56.0f;
		const bool Here = I == Ac.Tier;
		if (Here)
		{
			X.C.FillRect({Tiers.X + 12.0f, Ty, Tiers.W - 24.0f, 48.0f}, RitC(RitSelRow));
			X.C.FillRect({Tiers.X + 12.0f, Ty, 4.0f, 48.0f}, RitC(RitRed));
		}
		RitCardGround(X.C, {Tiers.X + 28.0f, Ty + 10.0f, 44.0f, 28.0f}, I);
		RitText(X.C, RitUpper(Club[static_cast<size_t>(I)].Name), Tiers.X + 88.0f, Ty + 30.0f, 18.0f, RitC(Here ? RitWhite : RitInk));
		RitText(X.C, "from " + rlnet_detail::NetMoney(Club[static_cast<size_t>(I)].From) + " of rake", Tiers.X + 300.0f, Ty + 30.0f, 13.0f, RitC(RitMuted), RitFace::Mono);
		RitText(X.C, RitTierPct(I), Tiers.X + Tiers.W - 28.0f, Ty + 30.0f, 18.0f, RitC(Here ? RitWhite : RitChrome), RitFace::Cond, Align::Right);
	}
	// Club Day: the last month the Club paid, printed on paper (its ledger line: "Second Run Club: Regular, October").
	const Rect Printout{908.0f, 442.0f, 668.0f, 300.0f};
	RitPanel(X.C, Printout, 10.0f);
	RitLabel(X.C, "Club Day", Printout.X + 24.0f, Printout.Y + 32.0f, 12.0f, RitC(RitMuted));
	const site::Line* Paid = nullptr;
	if (A)
	{
		for (const site::Line& Ln : A->Lines)
		{
			Paid = Ln.Label.rfind("Second Run Club", 0) == 0 && Ln.Cents > 0 ? &Ln : Paid;
		}
	}
	if (Paid)
	{
		const Rect P{Printout.X + 24.0f, Printout.Y + 50.0f, Printout.W - 48.0f, 228.0f};
		// It prints out of a slot (§3.4 G, g1-audit L1): the first time this laptop shows a Club Day, the slip feeds down out of
		// a slit across the panel's top over 0.6 s (the motion kit's ease), then PAID thunks (1.15 to 1.0 in 0.12 s, RitStamp
		// once), as the Settlement does. Real time only: the Kast preview draws the printed, stamped still and leaves the
		// player's own print to come.
		const std::string DayKey = std::to_string(static_cast<long long>(std::llround(Paid->At))) + "#" + std::to_string(Paid->Cents);
		if (!X.Preview && X.U.ClubDayFor != DayKey)
		{
			X.U.ClubDayFor = DayKey;
			X.U.ClubDayAt = X.Now;
			X.U.ClubDayStamped = false;
		}
		const double Fed = X.Preview ? 10.0 : X.Now - X.U.ClubDayAt;
		const float Out = static_cast<float>(motion::CountUp(0.0, 1.0, Fed, 0.0, 0.6));
		if (!X.Preview && Fed >= 0.6 && !X.U.ClubDayStamped)
		{
			X.U.ClubDayStamped = true;
			RitSound(X, SoundId::RitStamp, 0.45f);
		}
		X.C.FillRect({P.X - 8.0f, P.Y - 8.0f, P.W + 16.0f, 8.0f}, RitC(RitTape));
		X.C.FillRect({P.X - 8.0f, P.Y - 1.0f, P.W + 16.0f, 1.0f}, RitC(RitChrome, 0.5f));
		X.C.PushClip({P.X, P.Y, P.W, P.H});
		X.C.Save();
		X.C.Translate(0.0f, -(1.0f - Out) * P.H);
		X.C.FillRect(P, RitC(RitPaper));
		RitTwinH(X.C, P.X, P.Y, P.W, 0.5f);
		const std::string Head = "RUNITTWICE \xC2\xB7 CLUB DAY";
		RitText(X.C, Head, P.X + 22.0f, P.Y + 46.0f, 22.0f, RitC(RitRed));
		RitText(X.C, Head, P.X + 20.0f, P.Y + 44.0f, 22.0f, RitC(RitPaperInk));
		RitText(X.C, Paid->Label, P.X + 20.0f, P.Y + 84.0f, 15.0f, RitC(RitPaperInk), RitFace::Mono, Align::Left, Baseline::Alphabetic, P.W - 40.0f);
		RitText(X.C, RitDay(net::DayOf(Paid->At)) + " " + RitDate(net::DayOf(Paid->At)), P.X + 20.0f, P.Y + 110.0f, 12.0f, RitC(RitPaperMuted), RitFace::Mono);
		RitText(X.C, "+" + Money(Paid->Cents), P.X + P.W - 20.0f, P.Y + 170.0f, 40.0f, RitC(RitPaperInk), RitFace::Cond, Align::Right);
		if (Fed >= 0.6)
		{
			const double Inked = Fed - 0.6;
			const float Scale = static_cast<float>(Inked >= 0.12 ? 1.0 : 1.15 - 0.15 * Inked / 0.12);
			RitStampPaper(X.C, P.X + 120.0f, P.Y + 168.0f, "PAID", 24.0f, Scale, static_cast<float>(Clamp01(1.0 - Inked / 0.6)));
		}
		X.C.Restore();
		X.C.PopClip();
	}
	else
	{
		RitParagraph(X.C, RitClubDayNone, Printout.X + 24.0f, Printout.Y + 70.0f, Printout.W - 48.0f, 14.0f, RitC(RitMuted), 20.0f, 3);
	}
	// Welcome tickets and the site's letters.
	const Rect Wel{232.0f, 758.0f, 1344.0f, 192.0f};
	RitPanel(X.C, Wel, 10.0f);
	RitSpaced(X.C, "WELCOME TICKETS", Wel.X + 24.0f, Wel.Y + 32.0f, 13.0f, RitC(RitMuted), 1.5f); // the kicker the sites' test spells
	float Ty = Wel.Y + 64.0f;
	int Held = 0;
	const std::string Prefix = std::string(net::KeyOf(RitSite)) + ".";
	for (const auto& Tk : X.S.Life.Tickets)
	{
		if (Tk.second <= 0 || Tk.first.compare(0, Prefix.size(), Prefix) != 0 || Ty > Wel.Y + Wel.H - 16.0f)
		{
			continue;
		}
		const net::EventTemplate* T = net::Site(RitSite).FindTemplate(Tk.first);
		const std::string What = T ? rlnet_detail::NetShortName(T->Name) : Tk.first.substr(Prefix.size());
		const float Tw = RitTag(X.C, Wel.X + 24.0f, Ty - 13.0f, "TICKET", RitWhite, RitOnyx, 11.0f);
		RitText(X.C, What + (Tk.second > 1 ? " x" + std::to_string(Tk.second) : std::string()), Wel.X + 32.0f + Tw, Ty, 15.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 520.0f);
		Ty += 28.0f;
		++Held;
	}
	if (Held == 0 && A && A->Deposited > 0 && X.S.Life.Sites.Done.count("tickets:runittwice") == 0)
	{
		// Only a first deposit of $50 or more brings them: one under it had its chance.
		RitParagraph(X.C, rlsites_detail::SitesTicketsMissed, Wel.X + 24.0f, Ty, 600.0f, 14.0f, RitC(RitMuted), 19.0f, 2);
	}
	else if (Held == 0)
	{
		RitText(X.C, RitTicketsNone, Wel.X + 24.0f, Ty, 14.0f, RitC(RitMuted), RitFace::Body);
		RitParagraph(X.C, RitTicketsRule, Wel.X + 24.0f, Ty + 26.0f, 600.0f, 13.0f, RitC(RitMuted), 18.0f, 2);
	}
	X.C.FillRect({Wel.X + 680.0f, Wel.Y + 16.0f, 1.0f, Wel.H - 32.0f}, RitC(RitHair));
	RitLabel(X.C, "From RunItTwice", Wel.X + 704.0f, Wel.Y + 32.0f, 12.0f, RitC(RitMuted));
	float Ly = Wel.Y + 64.0f;
	int Letters = 0;
	for (auto It = X.S.Life.Inbox.rbegin(); It != X.S.Life.Inbox.rend() && Letters < 3; ++It)
	{
		if (rlsites_detail::SitesOfMail(It->Source, It->Thread, It->From) != RitSite)
		{
			continue;
		}
		const Rect Row{Wel.X + 696.0f, Ly - 20.0f, 624.0f, 34.0f};
		const Ui::ClickState St = RitClick(X, "clubletter" + std::to_string(Letters), Row);
		RitText(X.C, It->Subject, Row.X + 8.0f, Ly, 14.0f, RitC(St.Hover ? RitWhite : RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, 460.0f);
		RitText(X.C, RitDate(net::DayOf(It->At)), Row.X + Row.W - 8.0f, Ly, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
		if (St.Clicked)
		{
			X.RL.OpenApp(RiverLine::App::Mail, X.Now);
		}
		Ly += 38.0f;
		++Letters;
	}
	if (Letters == 0)
	{
		RitText(X.C, RitNoLetters, Wel.X + 704.0f, Ly, 14.0f, RitC(RitMuted), RitFace::Body);
	}
}

// ------------------------------------------------------------------ CASHIER (Page::Cashier, §3.4 H): Accounts

const Chips RitDepositSteps[4] = {1000, 5000, 10000, 50000};
const Chips RitWithdrawSteps[4] = {2000, 5000, 10000, 20000};

/** "Sat to Mon": a withdrawal's window in days (or a day and a time when it's one day). */
std::string RitWindow(double From, double HoursMin, double HoursMax)
{
	const int A = net::DayOf(From + HoursMin * 60.0);
	const int B = net::DayOf(From + HoursMax * 60.0);
	if (A == B)
	{
		return std::string(net::WeekdayName(A)) + " " + net::TimeLabel(From + HoursMin * 60.0);
	}
	return std::string(net::WeekdayName(A)) + " to " + net::WeekdayName(B);
}

void RitCashierPage(RitCtx& X)
{
	const site::Account* A = RitAccount(X.S);
	rlsites_detail::SitesUi& Su = rlsites_detail::SitesOf(&X.RL);
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesLook(RitSite);
	const net::CashMethod* In = site::MethodFor(RitSite, std::string(), false);
	const net::CashMethod* Out = site::MethodFor(RitSite, std::string(), true);
	const Chips Balance = A ? A->Cents : 0;
	const auto Answer = [&](const std::string& Why, const std::string& Done) {
		Su.Note = Why.empty() ? Done : Why;
		Su.NoteBad = !Why.empty();
		Su.NoteAt = X.Now;
	};
	// The head: the balance twice-printed (rolled up like an odometer), the methods, what's on its way.
	const Rect Head{232.0f, 96.0f, 1344.0f, 180.0f};
	RitPanel(X.C, Head, 10.0f);
	RitLabel(X.C, "Accounts", Head.X + 24.0f, Head.Y + 32.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "Your balance on RunItTwice", Head.X + 24.0f, Head.Y + 60.0f, 16.0f, RitC(RitMuted), RitFace::Body);
	const double Roll = X.Preview ? static_cast<double>(Balance) : motion::CountUp(0.0, static_cast<double>(Balance), X.Now, Access::PageAt(X.RL), 0.25);
	RitTwice(X.C, Money(static_cast<Chips>(Roll)), Head.X + 24.0f, Head.Y + 128.0f, 56.0f, RitC(RitWhite));
	RitText(X.C, RitStaysHere, Head.X + 24.0f, Head.Y + 160.0f, 13.0f, RitC(RitMuted), RitFace::Body);
	// The methods table.
	const float Mx = Head.X + 660.0f;
	RitLabel(X.C, "Method", Mx, Head.Y + 32.0f, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Fee", Mx + 230.0f, Head.Y + 32.0f, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Time", Mx + 340.0f, Head.Y + 32.0f, 12.0f, RitC(RitMuted));
	if (In)
	{
		RitText(X.C, "IN \xC2\xB7 " + In->Name, Mx, Head.Y + 60.0f, 13.0f, RitC(RitInk), RitFace::Mono);
		RitText(X.C, Fixed(static_cast<double>(In->FeePermille) / 10.0, In->FeePermille % 10 == 0 ? 0 : 1) + "%", Mx + 230.0f, Head.Y + 60.0f, 13.0f, RitC(RitInk), RitFace::Mono);
		RitText(X.C, "at once", Mx + 340.0f, Head.Y + 60.0f, 13.0f, RitC(RitInk), RitFace::Mono);
	}
	if (Out)
	{
		RitText(X.C, "OUT \xC2\xB7 " + Out->Name, Mx, Head.Y + 86.0f, 13.0f, RitC(RitInk), RitFace::Mono);
		RitText(X.C, Money(Out->WithdrawFeeCents), Mx + 230.0f, Head.Y + 86.0f, 13.0f, RitC(RitInk), RitFace::Mono);
		const std::string Hours = Out->WithdrawHoursMax > Out->WithdrawHoursMin ? Fixed(Out->WithdrawHoursMin, 0) + "-" + Fixed(Out->WithdrawHoursMax, 0) + " h" : Fixed(Out->WithdrawHoursMin, 0) + " h";
		RitText(X.C, Hours, Mx + 340.0f, Head.Y + 86.0f, 13.0f, RitC(RitInk), RitFace::Mono);
	}
	const Chips Coming = rlsites_detail::SitesInTransit(X.S, RitSite);
	RitLabel(X.C, "In transit", Head.X + Head.W - 24.0f, Head.Y + 120.0f, 12.0f, RitC(RitMuted), Align::Right);
	RitText(X.C, Coming > 0 ? Money(Coming) : std::string("\xE2\x80\x94"), Head.X + Head.W - 24.0f, Head.Y + 150.0f, 24.0f, RitC(Coming > 0 ? RitAmber : RitMuted), RitFace::Cond, Align::Right);
	RitText(X.C, "Bank " + Money(X.S.BankrollCents), Mx, Head.Y + 150.0f, 14.0f, RitC(RitInk), RitFace::Mono);
	// Deposit, its four steps at y 390-432 (the sites' test window).
	const Rect Dp{232.0f, 296.0f, 664.0f, 436.0f};
	RitPanel(X.C, Dp, 10.0f);
	RitTwice(X.C, "DEPOSIT", Dp.X + 24.0f, Dp.Y + 48.0f, 28.0f, RitC(RitWhite));
	RitText(X.C, RitDepositHow, Dp.X + 24.0f, Dp.Y + 74.0f, 13.5f, RitC(RitMuted), RitFace::Body);
	Su.Deposit = std::min(std::max(Su.Deposit, 0), 3);
	for (int K = 0; K < 4; ++K)
	{
		const float Cw = (Dp.W - 48.0f - 3.0f * 10.0f) / 4.0f;
		const Rect Ch{Dp.X + 24.0f + static_cast<float>(K) * (Cw + 10.0f), Dp.Y + 94.0f, Cw, 42.0f};
		const Ui::ClickState St = RitClick(X, "cashin" + std::to_string(K), Ch);
		if (St.Clicked)
		{
			Su.Deposit = K;
		}
		const bool On = Su.Deposit == K;
		RitCutFill(X.C, Ch, 6.0f, RitC(On ? RitRed : St.Hover ? RitPanel2 : RitCarbon));
		if (!On)
		{
			RitCutStroke(X.C, Ch, 6.0f, RitC(RitHair), 1.0f);
		}
		RitText(X.C, rlnet_detail::NetMoney(RitDepositSteps[K]), Ch.X + Ch.W / 2.0f, Ch.Y + 27.0f, 18.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Center);
	}
	const Chips Amount = RitDepositSteps[Su.Deposit];
	const Chips Fee = In ? static_cast<Chips>(std::llround(static_cast<double>(Amount) * static_cast<double>(In->FeePermille) / 1000.0)) : 0;
	const std::pair<std::string, std::string> DepRows[4] = {{"Lands on RunItTwice", Money(Amount)}, {"Card fee", Money(Fee)}, {"The bank pays", Money(Amount + Fee)},
		{"Bank after", Money(X.S.BankrollCents - Amount - Fee)}};
	for (int I = 0; I < 4; ++I)
	{
		RitTerm(X, Dp.X + 24.0f, Dp.X + Dp.W - 24.0f, Dp.Y + 176.0f + static_cast<float>(I) * 30.0f, RitUpper(DepRows[I].first), DepRows[I].second, I == 0 ? RitWhite : RitInk);
	}
	{
		std::string Why;
		if (!In)
		{
			Why = "No way in yet";
		}
		else if (In->MinCents > 0 && Amount < In->MinCents)
		{
			Why = "Minimum " + rlnet_detail::NetMoney(In->MinCents);
		}
		else if (X.S.BankrollCents < Amount + Fee)
		{
			Why = "Short " + Money(Amount + Fee - X.S.BankrollCents) + " in the bank";
		}
		ButtonOpts O;
		O.Kind = ButtonKind::Primary;
		O.Size = 18.0f;
		O.Enabled = Why.empty() && !X.Preview;
		O.Sub = Why.empty() ? (In ? In->Name + " \xC2\xB7 lands at once" : std::string()) : Why;
		if (rlsites_detail::SitesButton(X.UI, X.C, "cashdeposit", {Dp.X + 24.0f, Dp.Y + 314.0f, Dp.W - 48.0f, 60.0f}, "Deposit " + rlnet_detail::NetMoney(Amount), O, Look))
		{
			Answer(X.S.Deposit(RitSite, Amount, In->Id), "Deposited " + Money(Amount) + ". It's on RunItTwice now.");
		}
	}
	RitText(X.C, RitOnlyBank, Dp.X + 24.0f, Dp.Y + 410.0f, 12.5f, RitC(RitMuted), RitFace::Body);
	// Withdraw, its five steps on the same row.
	const Rect Wd{912.0f, 296.0f, 664.0f, 436.0f};
	RitPanel(X.C, Wd, 10.0f);
	RitTwice(X.C, "WITHDRAW", Wd.X + 24.0f, Wd.Y + 48.0f, 28.0f, RitC(RitWhite));
	RitText(X.C, RitWithdrawHow, Wd.X + 24.0f, Wd.Y + 74.0f, 13.5f, RitC(RitMuted), RitFace::Body);
	if (Su.Withdraw <= 0)
	{
		Su.Withdraw = RitWithdrawSteps[0];
	}
	for (int K = 0; K < 5; ++K)
	{
		const float Cw = (Wd.W - 48.0f - 4.0f * 10.0f) / 5.0f;
		const Rect Ch{Wd.X + 24.0f + static_cast<float>(K) * (Cw + 10.0f), Wd.Y + 94.0f, Cw, 42.0f};
		const Chips Step = K < 4 ? RitWithdrawSteps[K] : Balance;
		const Ui::ClickState St = RitClick(X, "cashout" + std::to_string(K), Ch);
		if (St.Clicked)
		{
			Su.Withdraw = Step;
		}
		const bool On = Su.Withdraw == Step && (K < 4 || Balance > 0);
		RitCutFill(X.C, Ch, 6.0f, RitC(On ? RitRed : St.Hover ? RitPanel2 : RitCarbon));
		if (!On)
		{
			RitCutStroke(X.C, Ch, 6.0f, RitC(RitHair), 1.0f);
		}
		RitText(X.C, K < 4 ? rlnet_detail::NetMoney(Step) : std::string("ALL"), Ch.X + Ch.W / 2.0f, Ch.Y + 27.0f, 18.0f, RitC(On ? RitWhite : RitInk), RitFace::Cond, Align::Center);
	}
	const Chips Take = Su.Withdraw;
	const Chips OutFee = Out ? Out->WithdrawFeeCents : 0;
	const std::string When = Out ? RitWindow(X.World, Out->WithdrawHoursMin, Out->WithdrawHoursMax) : std::string("\xE2\x80\x94");
	const std::pair<std::string, std::string> OutRows[4] = {{"Leaves RunItTwice now", Money(Take)}, {"Bank fee", Money(OutFee)}, {"Reaches your bank", Money(std::max<Chips>(0, Take - OutFee))},
		{"Expected", When}};
	for (int I = 0; I < 4; ++I)
	{
		RitTerm(X, Wd.X + 24.0f, Wd.X + Wd.W - 24.0f, Wd.Y + 176.0f + static_cast<float>(I) * 30.0f, RitUpper(OutRows[I].first), OutRows[I].second, I == 2 ? RitWhite : RitInk);
	}
	{
		std::string Why;
		const Chips Min = net::Def(RitSite).MinWithdraw;
		if (!Out)
		{
			Why = "No way out yet";
		}
		else if (Take <= 0 || Take > Balance)
		{
			Why = "Balance " + Money(Balance);
		}
		else if (Min > 0 && Take < Min)
		{
			Why = "Minimum " + rlnet_detail::NetMoney(Min);
		}
		ButtonOpts O;
		O.Kind = ButtonKind::Secondary;
		O.Size = 18.0f;
		O.Enabled = Why.empty() && !X.Preview;
		O.Sub = Why.empty() ? "Expected " + When : Why;
		if (rlsites_detail::SitesButton(X.UI, X.C, "cashwithdraw", {Wd.X + 24.0f, Wd.Y + 314.0f, Wd.W - 48.0f, 60.0f}, "Withdraw " + Money(Take), O, Look))
		{
			Answer(X.S.Withdraw(RitSite, Take, Out->Id), "Requested. " + Money(std::max<Chips>(0, Take - OutFee)) + " is on its way to your bank.");
			Su.Withdraw = RitWithdrawSteps[0];
		}
	}
	if (!Su.Note.empty() && X.Now - Su.NoteAt < 6.0)
	{
		RitText(X.C, Su.Note, Wd.X + 24.0f, Wd.Y + 410.0f, 13.0f, RitC(Su.NoteBad ? RitDown : RitUp), RitFace::Body, Align::Left, Baseline::Alphabetic, Wd.W - 48.0f);
	}
	// The statement: the wallet's lines with a running balance (Mono), and money on its way to the bank.
	const Rect Stmt{232.0f, 748.0f, 1344.0f, 202.0f};
	RitPanel(X.C, Stmt, 10.0f);
	RitLabel(X.C, "Statement", Stmt.X + 24.0f, Stmt.Y + 30.0f, 12.0f, RitC(RitMuted));
	RitLabel(X.C, "Balance", Stmt.X + Stmt.W - 24.0f, Stmt.Y + 30.0f, 12.0f, RitC(RitMuted), Align::Right);
	float Sy = Stmt.Y + 56.0f;
	int Moving = 0;
	for (const site::Transfer& T : X.S.Life.Sites.Transfers)
	{
		if (T.Site != RitSite || !T.Out || T.State != 0 || Moving >= 2)
		{
			continue;
		}
		RitText(X.C, "Withdrawal of " + Money(T.Cents) + " \xC2\xB7 requested " + net::WeekdayName(net::DayOf(T.RequestedAt)) + " " + net::TimeLabel(T.RequestedAt), Stmt.X + 24.0f, Sy, 13.0f,
			RitC(RitInk), RitFace::Mono);
		RitText(X.C, Money(std::max<Chips>(0, T.Cents - T.FeeCents)) + " arriving " + (Out ? RitWindow(T.RequestedAt, Out->WithdrawHoursMin, Out->WithdrawHoursMax) : std::string()),
			Stmt.X + Stmt.W - 24.0f, Sy, 13.0f, RitC(RitAmber), RitFace::Mono, Align::Right);
		Sy += 26.0f;
		++Moving;
	}
	if (!A || A->Lines.empty())
	{
		if (Moving == 0)
		{
			RitText(X.C, RitNoLines, Stmt.X + 24.0f, Sy, 13.0f, RitC(RitMuted), RitFace::Body);
		}
		return;
	}
	Chips Running = A->Cents;
	for (auto It = A->Lines.rbegin(); It != A->Lines.rend() && Sy < Stmt.Y + Stmt.H - 10.0f; ++It)
	{
		const site::Line& Ln = *It;
		RitText(X.C, RitDay(net::DayOf(Ln.At)) + " " + RitClock(Ln.At), Stmt.X + 24.0f, Sy, 12.0f, RitC(RitChrome), RitFace::Mono);
		RitText(X.C, Ln.Label, Stmt.X + 150.0f, Sy, 13.0f, RitC(RitInk), RitFace::Mono, Align::Left, Baseline::Alphabetic, 800.0f);
		RitText(X.C, Ln.Cents == 0 ? std::string("\xE2\x80\x94") : (Ln.Cents > 0 ? "+" : "-") + Money(Ln.Cents > 0 ? Ln.Cents : -Ln.Cents), Stmt.X + Stmt.W - 200.0f, Sy, 13.0f,
			RitC(Ln.Cents > 0 ? RitUp : Ln.Cents < 0 ? RitInk : RitMuted), RitFace::Mono, Align::Right);
		RitText(X.C, Money(Running), Stmt.X + Stmt.W - 24.0f, Sy, 13.0f, RitC(RitWhite), RitFace::Mono, Align::Right);
		Running -= Ln.Cents;
		Sy += 26.0f;
	}
}

// ------------------------------------------------------------------ MEMBER FILE (Page::Profile, §3.4 J: the shell)

void RitMemberPage(RitCtx& X)
{
	const site::Account* A = RitAccount(X.S);
	const site::show::Accrual Ac = RitAccrual(X.S);
	RitPanelBegin();
	const Rect L{232.0f, 96.0f, 420.0f, 854.0f};
	RitPanel(X.C, L, 10.0f);
	RitSquareAvatar(X.C, L.X + 24.0f, L.Y + 24.0f, 180.0f, X.S.HeroName, RitC(RitChrome));
	RitTwice(X.C, X.S.HeroName, L.X + 24.0f, L.Y + 252.0f, RitFit(X.C, X.S.HeroName, 30.0f, 20.0f, L.W - 48.0f), RitC(RitWhite));
	RitText(X.C, "No. " + RitMemberNo(X.S), L.X + 24.0f, L.Y + 284.0f, 14.0f, RitC(RitChrome), RitFace::Mono);
	RitCardGround(X.C, {L.X + 24.0f, L.Y + 306.0f, 64.0f, 40.0f}, Ac.Tier);
	RitLabel(X.C, Ac.Tier > 0 ? RitTierName(Ac.Tier) + " \xC2\xB7 " + RitTierPct(Ac.Tier) : std::string("Club \xC2\xB7 no tier yet"), L.X + 100.0f, L.Y + 332.0f, 15.0f, RitC(RitInk));
	RitText(X.C, "Joined RunItTwice " + (A ? net::DateLabel(net::DayOf(A->JoinedAt)) + ", " + std::to_string(world::YearOf(net::DayOf(A->JoinedAt))) : std::string("today")), L.X + 24.0f,
		L.Y + 384.0f, 15.0f, RitC(RitInk), RitFace::Body);
	RitParagraph(X.C, RitMemberNote, L.X + 24.0f, L.Y + 420.0f, L.W - 48.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
	// The summary grid: the Desk's numbers over everything, and the record's bests.
	const Rect R{668.0f, 96.0f, 908.0f, 854.0f};
	RitPanel(X.C, R, 10.0f);
	RitLabel(X.C, "Summary", R.X + 24.0f, R.Y + 32.0f, 12.0f, RitC(RitMuted));
	// No account yet: an empty one on RunItTwice (Account's default site is RiverLine, whose rows the Desk would count).
	site::Account Empty;
	Empty.Site = RitSite;
	const site::show::DeskKpis K = site::show::Desk(X.S.History, A ? *A : Empty, 0, X.World);
	Chips Best = 0;
	int Wins = 0;
	int Fts = 0;
	for (const HistoryEntry& H : X.S.History)
	{
		if (net::SiteOf(H.EventId) == RitSite)
		{
			Best = std::max(Best, H.Prize);
			Wins += H.Place == 1 ? 1 : 0;
			Fts += site::show::MadeFinalTable(H) ? 1 : 0; // the event's last table: the last 8 at 8-max
		}
	}
	const auto Pct = [](double V) { return Fixed(V * 100.0, 1) + "%"; };
	const std::pair<const char*, std::string> Cells[8] = {{"Events", Grouped(K.Events)}, {"ITM", Pct(K.Itm)}, {"ROI", Pct(K.Roi)}, {"ABI", Money(K.Abi)},
		{"Net", (K.Net < 0 ? "-" : "+") + Money(K.Net < 0 ? -K.Net : K.Net)}, {"Biggest cash", Money(Best)}, {"Final tables", Grouped(Fts)}, {"Wins", Grouped(Wins)}};
	for (int I = 0; I < 8; ++I)
	{
		const Rect C{R.X + 24.0f + static_cast<float>(I % 4) * 216.0f, R.Y + 52.0f + static_cast<float>(I / 4) * 104.0f, 204.0f, 92.0f};
		RitCutFill(X.C, C, 8.0f, RitGlossPaint(C));
		RitCutStroke(X.C, C, 8.0f, RitC(RitHair), 1.0f);
		RitLabel(X.C, Cells[I].first, C.X + 12.0f, C.Y + 22.0f, 12.0f, RitC(RitMuted));
		const bool Loss = !Cells[I].second.empty() && Cells[I].second[0] == '-';
		if (Loss)
		{
			RitText(X.C, Cells[I].second, C.X + 12.0f, C.Y + 70.0f, 34.0f, RitC(RitDown));
		}
		else
		{
			RitTwice(X.C, Cells[I].second, C.X + 12.0f, C.Y + 70.0f, RitFit(X.C, Cells[I].second, 34.0f, 22.0f, C.W - 24.0f), RitC(RitWhite));
		}
	}
	// Club history: the months the Club paid (its ledger lines), as tier bars.
	const float Hy = R.Y + 290.0f;
	RitLabel(X.C, "Club history", R.X + 24.0f, Hy, 12.0f, RitC(RitMuted));
	int Months = 0;
	if (A)
	{
		for (const site::Line& Ln : A->Lines)
		{
			if (Ln.Label.rfind("Second Run Club", 0) != 0 || Months >= 12)
			{
				continue;
			}
			int Tier = 1;
			const std::vector<site::TierInfo>& Club = site::SecondRunTiers();
			for (int T = 1; T < static_cast<int>(Club.size()); ++T)
			{
				Tier = Ln.Label.find(Club[static_cast<size_t>(T)].Name) != std::string::npos ? T : Tier;
			}
			const float Bx = R.X + 24.0f + static_cast<float>(Months) * 70.0f;
			const float Bh = 30.0f + 30.0f * static_cast<float>(Tier);
			RitCardGround(X.C, {Bx, Hy + 160.0f - Bh, 56.0f, Bh}, Tier);
			RitText(X.C, RitDate(net::DayOf(Ln.At)).substr(0, 3), Bx + 28.0f, Hy + 180.0f, 11.0f, RitC(RitMuted), RitFace::Cond, Align::Center);
			++Months;
		}
	}
	if (Months == 0)
	{
		RitParagraph(X.C, RitNoClubMonths, R.X + 24.0f, Hy + 30.0f, R.W - 48.0f, 13.0f, RitC(RitMuted), 18.0f, 2);
	}
}

// ------------------------------------------------------------------ the Settlement (results, §3.4 I): it prints, and stamps

const char* RitVerdict(int V)
{
	return V >= 2 ? "WON BOTH" : V == 1 ? "SPLIT IT" : "LOST BOTH";
}

/** RAN's word: what the real run paid the player (2 all of it, 1 part: a chop, or the main pot without the side, 0 none). */
const char* RitRanWord(int First)
{
	return First >= 2 ? "WON" : First == 1 ? "SPLIT" : "LOST";
}

/** One Second Board seen at this event: the all-in, the run, and the twice (for show; it never moved a chip). */
float RitSecondBlock(RitCtx& X, const RitSecond& B, float L, float Y, float W)
{
	RitText(X.C, RitClock(B.At), L, Y + 12.0f, 12.0f, RitC(RitChrome), RitFace::Mono);
	RitText(X.C, B.HandId.size() > 14 ? B.HandId.substr(B.HandId.size() - 14) : B.HandId, L + W, Y + 12.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	float Cx = L;
	for (const int Cd : B.Hero)
	{
		DrawCard(X.C, Cd, Cx, Y + 22.0f, 26.0f, 36.0f);
		Cx += 29.0f;
	}
	RitText(X.C, "v", Cx + 6.0f, Y + 45.0f, 12.0f, RitC(RitMuted), RitFace::Body);
	Cx += 20.0f;
	for (const int Cd : B.Villain)
	{
		if (Cd >= 0)
		{
			DrawCard(X.C, Cd, Cx, Y + 22.0f, 26.0f, 36.0f);
		}
		Cx += 29.0f;
	}
	if (B.Equity >= 0.0)
	{
		RitText(X.C, Fixed(B.Equity * 100.0, 0) + "% | " + Fixed(100.0 - B.Equity * 100.0, 0) + "%", L + W, Y + 45.0f, 12.0f, RitC(RitInk), RitFace::Mono, Align::Right);
	}
	RitLabel(X.C, "Ran", L, Y + 92.0f, 11.0f, RitC(RitMuted));
	for (size_t I = 0; I < B.Ran.size() && I < 5; ++I)
	{
		DrawCard(X.C, B.Ran[I], L + 36.0f + static_cast<float>(I) * 34.0f, Y + 68.0f, 30.0f, 42.0f);
	}
	RitText(X.C, RitRanWord(B.First), L + W, Y + 94.0f, 15.0f, RitC(B.First >= 2 ? RitUp : B.First == 1 ? RitInk : RitDown), RitFace::Cond, Align::Right);
	if (B.First == 0)
	{
		RitTri(X.C, L + W - RitMeasure(X.C, "LOST", 15.0f) - 8.0f, Y + 89.0f, 8.0f, false, RitC(RitDown));
	}
	const float Tw = RitParaTag(X.C, L, Y + 120.0f, "IF IT RAN TWICE", 11.0f, 18.0f);
	RitText(X.C, "you'd have", L + Tw + 8.0f, Y + 133.0f, 12.0f, RitC(RitInk), RitFace::Body);
	RitText(X.C, RitVerdict(B.Verdict), L + W, Y + 134.0f, 15.0f, RitC(RitWhite), RitFace::Cond, Align::Right);
	const float A0 = X.C.GetAlpha();
	X.C.SetAlpha(A0 * 0.6f);
	for (size_t I = 0; I < B.Twice.size(); ++I)
	{
		DrawCard(X.C, B.Twice[I], L + 36.0f + static_cast<float>(I) * 34.0f, Y + 146.0f, 30.0f, 42.0f);
	}
	X.C.SetAlpha(A0);
	return Y + 200.0f;
}

void RitClubMeterDraw(RitCtx& X, const Rect& M, const site::show::Accrual& Ac)
{
	const float Pos = RitClubMeter(Ac.Rake);
	X.C.FillRect(M, RitC(RitOnyx));
	X.C.FillRect({M.X, M.Y, M.W * Pos, M.H}, RitC(RitRed));
	RitLedEdge(X.C, M.X, M.Y, M.W * Pos);
	for (int I = 0; I < 4; ++I)
	{
		const float Tx = M.X + M.W * static_cast<float>(I + 1) / 4.0f;
		X.C.FillRect({Tx - 1.0f, M.Y - 3.0f, 2.0f, M.H + 6.0f}, RitC(RitWhite));
	}
	X.C.FillPolygon({{M.X + M.W * Pos - 5.0f, M.Y - 9.0f}, {M.X + M.W * Pos + 5.0f, M.Y - 9.0f}, {M.X + M.W * Pos, M.Y - 3.0f}}, RitC(RitWhite));
}

void RitSettlement(RitCtx& X)
{
	if (!X.S.HasResults)
	{
		return;
	}
	const Results& R = X.S.LastResults;
	// Its event: History's front when it is this result's (FUSE results never enter History: G2's).
	const std::string EventId = !X.S.History.empty() && X.S.History.front().Name == R.EventName ? X.S.History.front().EventId : std::string();
	const net::Network& Net = net::Site(RitSite);
	net::EventInstance E;
	const bool Found = Net.FindInstance(EventId, E);
	const net::EventTemplate* T = Found ? &Net.TemplateOf(E) : nullptr;
	const site::Account* A = RitAccount(X.S);
	// It prints (0.6 s out of its slot), then the stamp thunks (1.15 to 1.0 in 0.12 s; RitStamp, once).
	// The Kast preview draws the printed, stamped still and leaves the player's own print and stamp to come.
	const std::string Key = EventId + "#" + std::to_string(R.Place);
	if (!X.Preview && X.U.PrintFor != Key)
	{
		X.U.PrintFor = Key;
		X.U.PrintAt = X.Now;
		X.U.Stamped = false;
	}
	const double Since = X.Preview ? 10.0 : X.Now - X.U.PrintAt;
	const float Out = static_cast<float>(EaseOutCubic(Clamp01(Since / 0.6)));
	// FINAL TABLE is the event's last table: its seats (8 at the rival default; 9 when the event isn't known).
	const int FinalSeats = T ? std::max(2, T->TableSize) : 9;
	const std::string Word = R.Won || R.Place == 1 ? "WINNER" : R.Place >= 1 && R.Place <= FinalSeats ? "FINAL TABLE" : R.PrizeCents > 0 ? "CASHED" : std::string();
	if (!X.Preview && !Word.empty() && Since >= 0.6 && !X.U.Stamped)
	{
		X.U.Stamped = true;
		RitSound(X, SoundId::RitStamp, 0.5f);
	}
	// The left column: this event's Second Boards (its luck is G2's).
	const Rect Left{232.0f, 96.0f, 282.0f, 830.0f};
	RitPanel(X.C, Left, 10.0f);
	RitLabel(X.C, "Second boards", Left.X + 16.0f, Left.Y + 30.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "for show", Left.X + Left.W - 16.0f, Left.Y + 30.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Right);
	float By = Left.Y + 50.0f;
	int Boards = 0;
	// The session's log (HeroHandDone, both paths: a night skipped to its result keeps its all-ins), newest first.
	for (auto It = X.S.SecondBoards.rbegin(); It != X.S.SecondBoards.rend() && Boards < 3; ++It)
	{
		if (It->EventId == EventId)
		{
			By = RitSecondBlock(X, RitSecondOf(*It), Left.X + 16.0f, By, Left.W - 32.0f);
			++Boards;
		}
	}
	if (Boards == 0)
	{
		RitParagraph(X.C, RitNoSeconds, Left.X + 16.0f, By + 20.0f, Left.W - 32.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
	}
	RitParagraph(X.C, RitSecondHow, Left.X + 16.0f, Left.Y + Left.H - 40.0f, Left.W - 32.0f, 11.0f, RitC(RitMuted), 15.0f, 2);
	// The receipt: Paper and Paper Ink, a twin-stripe tear bar across its top, a zigzag bottom, out of a slot.
	const float X0 = 530.0f;
	const float W = 540.0f;
	const float L = X0 + 24.0f;
	const float Rx = X0 + W - 24.0f;
	X.C.FillRect({X0 - 14.0f, 86.0f, W + 28.0f, 12.0f}, RitC(RitTape));
	X.C.FillRect({X0 - 14.0f, 97.0f, W + 28.0f, 1.0f}, RitC(RitChrome, 0.5f));
	X.C.PushClip({X0 - 14.0f, 98.0f, W + 28.0f, 850.0f});
	X.C.Save();
	X.C.Translate(0.0f, -(1.0f - Out) * 830.0f);
	RitPanelBegin(); // the receipt: its tear bar and the net's double rule (§3.3: the one exception)
	std::vector<Vec2> Paper = {{X0, 98.0f}, {X0 + W, 98.0f}};
	const int Teeth = 45;
	for (int I = 0; I <= Teeth; ++I)
	{
		Paper.push_back({X0 + W - static_cast<float>(I) * W / static_cast<float>(Teeth), 926.0f - (I % 2 == 1 ? 6.0f : 0.0f)});
	}
	X.C.FillPolygon(Paper, RitC(RitPaper));
	X.C.FillRect({X0, 98.0f, W, 40.0f}, Paint::Linear({0.0f, 98.0f}, {0.0f, 138.0f}, RitC(0x000000, 0.10f), RitC(0x000000, 0.0f)));
	RitTwinH(X.C, X0, 98.0f, W);
	// The head, a misregistered print: Signal Red under Paper Ink (22 units, o = 2).
	const std::string Head = "RUNITTWICE \xC2\xB7 SETTLEMENT";
	RitText(X.C, Head, L + 2.0f, 152.0f, 22.0f, RitC(RitRed));
	RitText(X.C, Head, L, 150.0f, 22.0f, RitC(RitPaperInk));
	RitText(X.C, "No. " + std::to_string(Fnv1a(Key) % 100000000u), Rx, 142.0f, 11.0f, RitC(RitPaperMuted), RitFace::Mono, Align::Right);
	RitText(X.C, RitDay(net::DayOf(X.World)) + " " + RitDate(net::DayOf(X.World)) + " \xC2\xB7 " + RitClock(X.World), Rx, 157.0f, 11.0f, RitC(RitPaperMuted), RitFace::Mono, Align::Right);
	X.C.FillRect({L, 170.0f, Rx - L, 1.0f}, RitC(RitPaperInk));
	X.C.FillRect({L, 173.0f, Rx - L, 1.0f}, RitC(RitPaperInk));
	const std::string Name = RitUpper(rlnet_detail::NetShortName(R.EventName));
	RitTwice(X.C, Name, L, 216.0f, RitFit(X.C, Name, 34.0f, 24.0f, Rx - L), RitC(RitPaperInk));
	if (T)
	{
		RitText(X.C, T->Speed + " \xC2\xB7 " + rlnet_detail::NetFormat(*T) + " \xC2\xB7 " + std::to_string(T->TableSize) + "-max \xC2\xB7 " + RitDay(net::DayOf(E.Start)) + " " + RitClock(E.Start), L, 240.0f, 13.0f,
			RitC(RitPaperMuted), RitFace::Body);
	}
	const auto Leader = [&](float Y, const std::string& K, const std::string& V, bool Sub) {
		const float Indent = Sub ? 16.0f : 0.0f;
		const Color Col = RitC(Sub ? RitPaperMuted : RitPaperInk);
		const float Kw = RitText(X.C, K, L + Indent, Y, 14.0f, Col, RitFace::Mono);
		const float Vw = RitText(X.C, V, Rx, Y, 14.0f, Col, RitFace::Mono, Align::Right);
		for (float Dx = L + Indent + Kw + 8.0f; Dx < Rx - Vw - 8.0f; Dx += 4.0f)
		{
			X.C.FillRect({Dx, Y - 3.0f, 1.0f, 1.0f}, RitC(RitPaperMuted));
		}
	};
	float Y = 290.0f;
	Leader(Y, "Entrants", Grouped(R.Entrants), false);
	Y += 25.0f;
	if (Found && T && X.S.T && X.S.T->Spec.Id == EventId)
	{
		// The pool this table played for: its own tournament's (still the session's on this screen), never the listing's live
		// pool, which during late registration is the guarantee or a part field and wouldn't match the entrants above.
		const TournamentSpec& Played = X.S.T->Spec;
		const Chips Raised = static_cast<Chips>(R.Entrants) * (Played.BuyInCents - Played.FeeCents - Played.BountyCents);
		const bool Heads = Played.BountyCents > 0; // a bounty event's pool is its prize half; the bounties sit on the heads
		Leader(Y, Heads ? "Prize pool (bounties apart)" : "Prize pool",
			Money(X.S.T->PrizePoolCents) + (T->GtdCents > 0 && !Heads ? " \xC2\xB7 GTD " + rlnet_detail::NetMoney(T->GtdCents) : std::string()), false);
		Y += 25.0f;
		if (Played.GuaranteeCents > Raised && Raised > 0)
		{
			Leader(Y, "of which overlay", Money(Played.GuaranteeCents - Raised) + " (the house)", true);
			Y += 25.0f;
		}
	}
	else if (Found && T)
	{
		const net::LiveState Lv = Net.Live(E, X.World);
		Leader(Y, "Prize pool", Money(Lv.Pool) + (T->GtdCents > 0 ? " \xC2\xB7 GTD " + rlnet_detail::NetMoney(T->GtdCents) : std::string()), false);
		Y += 25.0f;
		const site::show::OverlayView V = site::show::OverlayOf(Net, E, X.World);
		if (V.Actual && V.Cents > 0)
		{
			Leader(Y, "of which overlay", Money(V.Cents) + " (the house)", true);
			Y += 25.0f;
		}
	}
	Leader(Y, "Finish", Ordinal(R.Place) + " of " + Grouped(R.Entrants), false);
	Y += 25.0f;
	Leader(Y, "Prize", Money(R.PrizeCents), false);
	Y += 25.0f;
	if (R.BountyCents > 0)
	{
		Leader(Y, "Bounties", Money(R.BountyCents) + " \xC2\xB7 " + std::to_string(R.Knockouts) + " KO", false);
		Y += 25.0f;
	}
	Leader(Y, "Buy-in", R.BuyInCents > 0 ? "-" + Money(R.BuyInCents) : std::string("ticket"), false);
	Y += 25.0f;
	const Chips Fee = T && R.BuyInCents > 0 ? Net.Definition().Rake.Fee(T->BuyInCents) : 0;
	if (Fee > 0)
	{
		Leader(Y, "of which fee", Money(Fee), true);
		Y += 25.0f;
	}
	// The net under the red double rule (the receipt's second twin stripe), BoldCondensed 40, misregistered.
	RitTwinH(X.C, L, Y + 2.0f, Rx - L, 0.5f);
	Y += 46.0f;
	RitLabel(X.C, "Net", L, Y - 8.0f, 16.0f, RitC(RitPaperInk));
	const Chips NetC = R.PrizeCents + R.BountyCents - R.BuyInCents;
	RitTwice(X.C, (NetC < 0 ? "-" : "+") + Money(NetC < 0 ? -NetC : NetC), Rx, Y, 40.0f, RitC(RitPaperInk), RitRed, Align::Right);
	Y += 18.0f;
	X.C.FillRect({L, Y, Rx - L, 1.0f}, RitC(RitPaperInk, 0.35f));
	Y += 26.0f;
	const site::show::Accrual Ac = RitAccrual(X.S);
	if (Fee > 0 && Ac.Tier > 0)
	{
		const std::vector<site::TierInfo>& Club = site::SecondRunTiers();
		const Chips Adds = static_cast<Chips>(std::llround(static_cast<double>(Fee) * static_cast<double>(Club[static_cast<size_t>(Ac.Tier)].Permille) / 1000.0));
		Leader(Y, "Club accrual", "+" + Money(Adds) + " \xC2\xB7 " + RitTierName(Ac.Tier) + " " + RitTierPct(Ac.Tier), false);
		Y += 32.0f;
	}
	// The pinned run, the twin of Ignite's.
	X.C.StrokePolyline({{L, Y - 21.0f}, {Rx, Y - 21.0f}, {Rx, Y + 11.0f}, {L, Y + 11.0f}}, true, RitC(RitPaperInk), 1.0f);
	RitText(X.C, "RunItTwice balance now " + Money(rlsites_detail::SitesBalance(X.S, RitSite)), L + 12.0f, Y, 15.0f, RitC(RitPaperInk), RitFace::Mono);
	// The settlement code, as bars.
	float Bx = L;
	for (int I = 0; I < 46; ++I)
	{
		const uint32_t H = Fnv1a(Key + ":" + std::to_string(I));
		const float Bw = 1.0f + static_cast<float>(H % 3u);
		if ((H >> 8) % 5u != 0u)
		{
			X.C.FillRect({Bx, 828.0f, Bw, 30.0f}, RitC(RitPaperInk));
		}
		Bx += Bw + 2.0f;
	}
	RitText(X.C, RitSettled, L, 900.0f, 12.0f, RitC(RitPaperMuted), RitFace::Body);
	if (!Word.empty() && Since >= 0.6)
	{
		// The stamp: Signal Red ink on paper, the word at full alpha; nothing for a bust.
		const double S0 = Since - 0.6;
		const float Scale = static_cast<float>(S0 >= 0.12 ? 1.0 : 1.15 - 0.15 * S0 / 0.12);
		const float Spread = static_cast<float>(Clamp01(1.0 - S0 / 0.6));
		RitStampPaper(X.C, 862.0f, 772.0f, Word, 28.0f, Scale, Spread);
	}
	X.C.Restore();
	X.C.PopClip();
	// The right column: the Club after this result, what's boarding now, the way back.
	const float Cx0 = 1086.0f;
	const float Cw = 490.0f;
	const Rect Club{Cx0, 96.0f, Cw, 220.0f};
	RitPanel(X.C, Club, 10.0f);
	RitLabel(X.C, "The Club \xC2\xB7 Second Run", Club.X + 18.0f, Club.Y + 28.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "PROVISIONAL", Club.X + Cw - 18.0f, Club.Y + 28.0f, 11.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	RitLabel(X.C, Ac.Tier > 0 ? "Accrued at " + RitTierName(Ac.Tier) + " (" + RitTierPct(Ac.Tier) + ")" : std::string("Accrued this month"), Club.X + 18.0f, Club.Y + 54.0f, 12.0f, RitC(RitInk));
	RitTwice(X.C, Money(Ac.Accrued), Club.X + 18.0f, Club.Y + 96.0f, 34.0f, RitC(RitWhite));
	RitText(X.C, RitToNextLine(Ac), Club.X + Cw - 18.0f, Club.Y + 96.0f, 13.0f, RitC(RitInk), RitFace::Body, Align::Right, Baseline::Alphabetic, 230.0f);
	RitClubMeterDraw(X, {Club.X + 18.0f, Club.Y + 130.0f, Cw - 36.0f, 8.0f}, Ac);
	RitText(X.C, "Rake this month " + Money(Ac.Rake), Club.X + Cw - 18.0f, Club.Y + 196.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Right);
	RitText(X.C, RitNoSpins, Club.X + 18.0f, Club.Y + 196.0f, 11.0f, RitC(RitMuted), RitFace::Body);
	(void)A;
	// Up next: three boarding events, each through the shared register button (the selection follows the pointer, so an
	// armed confirm always belongs to the row it shows on).
	const Rect Next{Cx0, 332.0f, Cw, 536.0f};
	RitPanel(X.C, Next, 10.0f);
	RitLabel(X.C, "Up next", Next.X + 18.0f, Next.Y + 28.0f, 12.0f, RitC(RitMuted));
	RitText(X.C, "boarding now", Next.X + Cw - 18.0f, Next.Y + 28.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Right);
	std::vector<RitRow> Boarding;
	for (const RitRow& Rw : Access::ScheduleRows(X.RL))
	{
		if (Boarding.size() < 3 && Rw.Joinable && (Rw.L.St == net::Status::LateReg || (Rw.L.St == net::Status::Registering && Rw.L.StartsIn <= 60.0)))
		{
			Boarding.push_back(Rw);
		}
	}
	float Uy = Next.Y + 48.0f;
	for (size_t I = 0; I < Boarding.size(); ++I)
	{
		const RitRow& Rw = Boarding[I];
		const net::EventTemplate& Tt = Net.TemplateOf(Rw.E);
		const Rect Row{Next.X + 12.0f, Uy, Cw - 24.0f, 150.0f};
		if (!X.Preview && X.UI.Hover(Row) && X.RL.SelectedEvent() != Rw.E.Id && !X.S.ConfirmRegister)
		{
			X.RL.SelectEvent(Rw.E.Id);
		}
		RitFlap(X.C, Row.X + 6.0f, Uy + 8.0f, RitClock(Rw.E.Start), 14.0f, 22.0f, 5, RitTile::Gloss, RitC(RitInk));
		RitText(X.C, rlnet_detail::NetShortName(Tt.Name), Row.X + 76.0f, Uy + 24.0f, 15.0f, RitC(RitInk), RitFace::Name, Align::Left, Baseline::Alphabetic, Cw - 120.0f);
		RitLabel(X.C, RitUpper(Tt.Speed) + " \xC2\xB7 " + RitBuyIn(Tt.BuyInCents) + (Rw.L.St == net::Status::LateReg ? " \xC2\xB7 LATE REG " + RitCountdown(Rw.L.LateRegLeft) : std::string(" \xC2\xB7 REGISTERING")),
			Row.X + 76.0f, Uy + 44.0f, 11.0f, RitC(RitMuted));
		if (!X.S.ConfirmRegister || X.RL.SelectedEvent() == Rw.E.Id)
		{
			const Rect Rb{Row.X + 6.0f, Uy + 62.0f, Row.W - 12.0f, 60.0f};
			if (!X.Preview && X.S.ConfirmRegister && X.S.CurrentScreen == Screen::Results)
			{
				// The confirming press (the confirm step's "regok", its left half): leave the Settlement first, as BACK TO THE
				// BOARD does, so the finished table is dropped and the new one opens alone (Session::RegisterEvent keeps every
				// run it finds open, and the finished one is still there until the results are left).
				if (X.UI.Clickable("regok", {Rb.X, Rb.Y, (Rb.W - 12.0f) / 2.0f, Rb.H}).Clicked)
				{
					X.S.LeaveResults();
				}
			}
			Access::RegisterBlock(X.RL, Rw.E, Rw.L, Rb, X.Now);
		}
		Uy += 160.0f;
	}
	if (Boarding.empty())
	{
		RitParagraph(X.C, RitNoUpNext, Next.X + 18.0f, Uy + 10.0f, Cw - 36.0f, 13.0f, RitC(RitMuted), 18.0f, 3);
	}
	// Back to the Board.
	const Rect Back{Cx0, 884.0f, Cw, 42.0f};
	const Ui::ClickState Bs = RitClick(X, "backlobby", Back);
	RitCutFill(X.C, Back, 8.0f, RitGlossPaint(Back));
	RitCutStroke(X.C, Back, 8.0f, RitC(RitChrome, Bs.Hover ? 1.0f : 0.6f), 1.0f);
	RitText(X.C, "BACK TO THE BOARD", Back.X + Cw / 2.0f, Back.Y + 21.0f, 15.0f, RitC(RitInk), RitFace::Cond, Align::Center, Baseline::Middle);
	if (Bs.Clicked)
	{
		X.S.LeaveResults();
		X.RL.SiteShown = RitSite;
		X.RL.OpenPage(RiverLine::Page::Lobby, X.Now);
	}
}

// ------------------------------------------------------------------ at a table: the header's own line, the bets pill

void RitTableHeader(RitCtx& X)
{
	if (!X.S.T)
	{
		return;
	}
	const Tournament& T = *X.S.T;
	const Level& Lv = T.CurrentLevel();
	RitText(X.C, "T" + std::to_string(X.S.TableId) + " \xC2\xB7 " + Grouped(T.Remaining) + " LEFT \xC2\xB7 " + std::to_string(T.PaidPlaces()) + " PAID \xC2\xB7 LVL " + std::to_string(T.LevelIndex + 1) + " " +
			ChipsText(static_cast<double>(Lv.Sb)) + "/" + ChipsText(static_cast<double>(Lv.Bb)),
		396.0f, 58.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Left, Baseline::Middle, 300.0f);
	// Another seated table where it's your turn: a chip with its own ACTION (Hot White on Signal Red).
	float Cx = 720.0f;
	for (const rlsites_detail::StripRow& Row : RitSeated(X))
	{
		if (Row.Table == X.S.FocusedTable() || !Row.YourTurn || Cx > 980.0f)
		{
			continue;
		}
		const std::string Nm = RitUpper(RitEventName(Row.Short));
		const float Nw = RitMeasure(X.C, Nm, 13.0f);
		const std::string Stack = Fixed(Row.StackBb, Row.StackBb < 100.0 ? 1 : 0) + " BB";
		const float Sw = RitMeasure(X.C, Stack, 12.0f, RitFace::Mono);
		const float Aw = RitMeasure(X.C, "ACTION", 11.0f) + 10.0f;
		const Rect R{Cx, 42.0f, Nw + Sw + Aw + 44.0f, 32.0f};
		const Ui::ClickState St = RitClick(X, "othertable" + std::to_string(Row.Table), R);
		RitCutFill(X.C, R, 6.0f, RitC(RitCarbon));
		RitCutStroke(X.C, R, 6.0f, RitC(RitRed), 1.2f);
		RitText(X.C, Nm, R.X + 12.0f, R.Y + 16.0f, 13.0f, RitC(RitInk), RitFace::Cond, Align::Left, Baseline::Middle);
		RitText(X.C, Stack, R.X + 20.0f + Nw, R.Y + 16.0f, 12.0f, RitC(RitMuted), RitFace::Mono, Align::Left, Baseline::Middle);
		RitTag(X.C, R.X + 30.0f + Nw + Sw, R.Y + 8.0f, "ACTION", RitRed, RitWhite, 11.0f);
		if (St.Clicked)
		{
			X.S.FocusTable(Row.Table);
		}
		Cx += R.W + 8.0f;
	}
	// Bets in BB or chips: the shared table's pill (ui-tablestyle's Controls) toggles it; the header says which.
	const bool Bb = rlsites_detail::SitesBetsInBb(X.RL, rlsites_detail::SitesLook(RitSite));
	RitLabel(X.C, Bb ? "Bets in big blinds" : "Bets in chips", 1150.0f, 58.0f + 11.0f * 0.36f, 11.0f, RitC(RitMuted), Align::Right);
}

// ------------------------------------------------------------------ the octagon's layer (§3.5, R8)

/** The broadcast lower third: Tape Black, the twin stripe, ALL IN, both players, their cards, the equity split. */
void RitLowerThird(RitCtx& X, const Rect& Band)
{
	const SeatVis* Hero = nullptr;
	const SeatVis* Them = nullptr;
	for (const SeatVis& V : X.S.Seats)
	{
		if (!V.Present || V.Folded || !V.HasEquity)
		{
			continue;
		}
		if (V.IsHero)
		{
			Hero = &V;
		}
		else if (!Them || V.Equity > Them->Equity)
		{
			Them = &V;
		}
	}
	if (!Them)
	{
		return;
	}
	const SeatVis* Left = Hero ? Hero : Them;
	const SeatVis* Right = Hero ? Them : nullptr;
	if (!Right)
	{
		// Two others all in (the player watching): the two biggest stakes.
		for (const SeatVis& V : X.S.Seats)
		{
			Right = V.Present && !V.Folded && V.HasEquity && &V != Left && (!Right || V.Equity > Right->Equity) ? &V : Right;
		}
		if (!Right)
		{
			return;
		}
	}
	const Rect B = Band;
	RitPanelBegin();
	RitCutFill(X.C, B, 6.0f, RitC(RitTape, 0.86f));
	RitCutStroke(X.C, B, 6.0f, RitC(RitHair), 1.0f);
	RitTwinV(X.C, B.X, B.Y, B.H);
	X.C.FillRect({B.X + 20.0f, B.Y, 58.0f, B.H}, RitC(RitRed));
	RitLedEdge(X.C, B.X + 20.0f, B.Y, 58.0f);
	RitText(X.C, "ALL IN", B.X + 49.0f, B.Y + B.H / 2.0f, 15.0f, RitC(RitWhite), RitFace::Cond, Align::Center, Baseline::Middle);
	const auto Side = [&](const SeatVis& V, float Nx, float Cx, bool NameLeft) {
		RitText(X.C, V.Name, Nx, B.Y + 23.0f, 13.0f, RitC(RitInk), RitFace::Name, NameLeft ? Align::Left : Align::Right, Baseline::Alphabetic, 80.0f);
		for (size_t I = 0; I < V.Hole.size() && I < 2; ++I)
		{
			DrawCard(X.C, V.Hole[I], Cx + static_cast<float>(I) * 24.0f, B.Y + 3.0f, 22.0f, 30.0f);
		}
	};
	Side(*Left, B.X + 88.0f, B.X + 166.0f, true);
	Side(*Right, B.X + B.W - 12.0f, B.X + B.W - 150.0f, false);
	// Each side's own equity (three or more all in: the two shown don't sum to 100, so the right figure is never 100 - left);
	// the bar splits the two shown in proportion (heads-up: exactly the two).
	const double LeftEq = Clamp01(Left->Equity);
	const double RightEq = Clamp01(Right->Equity);
	const double Eq = LeftEq + RightEq > 0.0 ? LeftEq / (LeftEq + RightEq) : 0.5;
	const float Bx = B.X + 264.0f;
	const float Bw = 222.0f;
	const char* Street = X.S.Board.size() >= 4 ? "TURN" : X.S.Board.size() >= 3 ? "FLOP" : "PREFLOP ALL-IN";
	RitText(X.C, Street, Bx + Bw / 2.0f, B.Y + 11.0f, 10.0f, RitC(RitMuted), RitFace::Cond, Align::Center);
	RitText(X.C, Fixed(LeftEq * 100.0, 0) + "%", Bx - 8.0f, B.Y + 24.0f, 15.0f, RitC(RitWhite), RitFace::Mono, Align::Right);
	X.C.FillRect({Bx, B.Y + 17.0f, Bw * static_cast<float>(Eq), 9.0f}, RitC(RitRed));
	RitLedEdge(X.C, Bx, B.Y + 17.0f, Bw * static_cast<float>(Eq));
	X.C.FillRect({Bx + Bw * static_cast<float>(Eq), B.Y + 17.0f, Bw * static_cast<float>(1.0 - Eq), 9.0f}, Paint::Linear({0.0f, B.Y + 17.0f}, {0.0f, B.Y + 26.0f}, RitC(RitChromeLight), RitC(RitChromeDark)));
	X.C.FillRect({Bx + Bw * static_cast<float>(Eq) - 1.0f, B.Y + 14.0f, 2.0f, 15.0f}, RitC(RitWhite));
	RitText(X.C, Fixed(RightEq * 100.0, 0) + "%", Bx + Bw + 8.0f, B.Y + 24.0f, 15.0f, RitC(RitWhite), RitFace::Mono);
}

/** The Second Board under the real one (5 small cards at 0.6, the red tag, the verdict), for 4 s after the hand. */
void RitSecondShow(RitCtx& X, const RitSecond& B, float Cx, float Top, double Age)
{
	const float In = X.Preview ? 1.0f : static_cast<float>(EaseOutCubic(Clamp01(Age / 0.3)));
	const float Fade = X.Preview ? 1.0f : static_cast<float>(Clamp01((4.0 - Age) / 0.4));
	const float A0 = X.C.GetAlpha();
	X.C.SetAlpha(A0 * Fade);
	// One broadcast strip under the real board, on its own dark plate so nothing on the felt (a bet, its chips) reads through:
	// the red tag and "for show", five small cards (34 x 48) at 0.6, the verdict, and the first time a stamp on paper.
	const float Cw = 34.0f;
	const float Pitch = 39.0f;
	const std::string Verdict = RitVerdict(B.Verdict);
	const float TagW = RitMeasure(X.C, "IF IT RAN TWICE", 13.0f) + 22.0f + 22.0f * 0.3249197f;
	const float VerdictW = std::max(RitMeasure(X.C, "you'd have", 13.0f, RitFace::Body), RitMeasure(X.C, Verdict, 22.0f));
	const bool Note = X.U.SecondFirst || Age < 0.0;
	const float NoteW = 124.0f;
	const float StripW = 12.0f + TagW + 12.0f + (5.0f * Pitch - 5.0f) + 12.0f + VerdictW + 14.0f + (Note ? NoteW + 6.0f : 0.0f);
	const Rect Plate{Cx - StripW / 2.0f + (1.0f - In) * 40.0f, Top - 6.0f, StripW, 60.0f};
	RitCutFill(X.C, Plate, 8.0f, RitC(RitOnyx, 0.94f));
	RitCutStroke(X.C, Plate, 8.0f, RitC(RitHair), 1.0f);
	RitParaTag(X.C, Plate.X + 12.0f, Top + 2.0f, "IF IT RAN TWICE", 13.0f, 22.0f);
	RitText(X.C, "for show", Plate.X + 12.0f + TagW, Top + 44.0f, 11.0f, RitC(RitMuted), RitFace::Body, Align::Right);
	const float Sx = Plate.X + 12.0f + TagW + 12.0f;
	X.C.SetAlpha(A0 * Fade * 0.6f);
	for (size_t I = 0; I < B.Twice.size(); ++I)
	{
		DrawCard(X.C, B.Twice[I], Sx + static_cast<float>(I) * Pitch, Top, Cw, 48.0f);
	}
	X.C.SetAlpha(A0 * Fade);
	const float Vx = Sx + 5.0f * Pitch - 5.0f + 12.0f;
	RitText(X.C, "you'd have", Vx, Top + 18.0f, 13.0f, RitC(RitInk), RitFace::Body);
	RitText(X.C, Verdict, Vx, Top + 42.0f, 22.0f, RitC(RitWhite), RitFace::Cond);
	if (Note)
	{
		// The first time: a small stamp on paper, at the strip's right end.
		const Rect P{Plate.X + Plate.W - 6.0f - NoteW, Top - 1.0f, NoteW, 50.0f};
		X.C.FillRect(P, RitC(RitPaper));
		X.C.FillRect({P.X, P.Y, P.W, 3.0f}, RitC(RitRed));
		X.C.FillRect({P.X, P.Y + 5.0f, P.W, 1.0f}, RitC(RitRed));
		RitText(X.C, "On RunItTwice,", P.X + P.W / 2.0f, P.Y + 20.0f, 11.0f, RitC(RitPaperInk), RitFace::Name, Align::Center);
		RitText(X.C, "we run it twice.", P.X + P.W / 2.0f, P.Y + 33.0f, 11.0f, RitC(RitPaperInk), RitFace::Name, Align::Center);
		RitText(X.C, RitJustToSee, P.X + P.W / 2.0f, P.Y + 45.0f, 11.0f, RitC(RitPaperMuted), RitFace::Body, Align::Center);
	}
	X.C.SetAlpha(A0);
}
} // namespace rlrit_detail

// ------------------------------------------------------------------ the frame

void RiverLine::RitFrame(double Now, bool Preview)
{
	using namespace rlrit_detail;
	RitUi& U = RitOf(this);
	RitCtx X{*this, S, *C, UI, U, Now, World, Preview};
	// The autoplay panel belongs to the tables (update 2 §2.6.1), as in RiverLine's frame.
	if (!Preview && S.CurrentScreen != Screen::Table && rlauto_detail::AutoPanelOpen(this))
	{
		rlauto_detail::AutoClosePanel(this);
	}
	if (S.CurrentScreen == Screen::Table && S.Tiled && S.TableCount() >= 2)
	{
		HomeFrame(Now, Preview); // the tiles keep their geometry, each tile in its own site's skin and shape
		return;
	}
	RitPageBegin();
	C->FillRect({0.0f, 0.0f, Width, RitClientH}, RitC(RitOnyx));
	RitOpeningBell(X);
	if (S.CurrentScreen == Screen::Table)
	{
		// The octagon under the collapsed rail's chrome: the table first (its seats start at x 70), the chrome over its edges.
		RitRail(X, true);
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
		Table(Now);
		UI.Ptr = Held;
		// The tape and the header before any overlay: the card's backdrop (from y 64) and the autoplay panel draw over them and
		// take the clicks where they overlap, as over RiverLine's top bar (a click on the scrim never reaches a header chip).
		RitTapeBar(X, RitRailMini);
		RitHeaderBar(X, RitRailMini, S.T ? RitEventName(S.T->Spec.Name) : std::string("THE TABLE"), true);
		if (!Preview)
		{
			U.SeenPage = Page::Embercrest; // not on a page: the Board waves again when it's opened
		}
		RitTableHeader(X);
		if (Card)
		{
			PlayerCard(Now);
		}
		else if (Auto)
		{
			const float Pw = 1108.0f;
			AutoPanel({rlsites_detail::SitesTableLook(S).Cx - Pw / 2.0f, RitTop + 14.0f, Pw, 850.0f}, Now);
		}
	}
	else
	{
		const bool Card = CardOpen();
		const Pointer Real = UI.Ptr;
		if (Card)
		{
			UI.Ptr.Pressed = false;
			UI.Ptr.Released = false;
			UI.Ptr.Wheel = 0.0f;
			UI.Ptr.X = -1.0f;
			UI.Ptr.Y = -1.0f;
		}
		std::string Name = "SETTLEMENT";
		if (S.CurrentScreen == Screen::Results)
		{
			RitSettlement(X);
			if (!Preview)
			{
				U.SeenPage = Page::Embercrest; // not on a page: the Board waves again when it's opened
			}
		}
		else
		{
			// The page: RunItTwice's own (League hidden until G2), each easing in over 160 ms.
			PageShown = RitPageOf(rlsites_detail::SitesPageHome(RitSite, PageShown));
			Name = RitPageName(PageShown);
			const float In = Preview ? 1.0f : static_cast<float>(EaseOutCubic(Clamp01((Now - PageAt) / 0.16)));
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * In);
			switch (PageShown)
			{
			case Page::Live: RitLivePage(X); break;
			case Page::Leaderboards: RitRankingsPage(X); break;
			case Page::Desk: RitDeskPage(X); break;
			case Page::Series: RitSeriesPage(X); break;
			case Page::News: RitNewsPage(X); break;
			case Page::Promotions: RitClubPage(X); break;
			case Page::Cashier: RitCashierPage(X); break;
			case Page::Profile: RitMemberPage(X); break;
			default: RitBoardPage(X); break;
			}
			C->SetAlpha(A0);
			if (!Preview)
			{
				U.SeenPage = PageShown; // the Kast preview's still never spends the player's own wave
			}
		}
		RitRail(X, false);
		RitTapeBar(X, RitRailW);
		RitHeaderBar(X, RitRailW, Name, false);
		if (Card)
		{
			UI.Ptr = Real;
			PlayerCard(Now);
		}
	}
	if (!Preview)
	{
		FinishedToasts(Now);
	}
}

void RiverLine::RitTableLayer(double Now)
{
	using namespace rlrit_detail;
	if (!S.T)
	{
		return;
	}
	RitUi& U = RitOf(this);
	RitCtx X{*this, S, *C, UI, U, Now, World, Previewing};
	const rlsites_detail::SiteLook& Look = rlsites_detail::SitesTableLook(S);
	// Team RunItTwice's pros at the table: a 3-unit Signal Red top bar and a TEAM tab (Hot White on Signal Red).
	const std::string Team = net::Def(RitSite).TeamName;
	const world::World& W = S.Living();
	for (const SeatVis& V : S.Seats)
	{
		if (!V.Present || V.IsHero)
		{
			continue;
		}
		const world::Npc* N = W.Get(W.Find(V.Name));
		if (N && N->Sponsor == Team)
		{
			const Rect Box = rlsites_detail::SitesSeatBox(Look, SlotPos(V.Seat), false);
			C->FillRect({Box.X, Box.Y, Box.W - 6.0f, 3.0f}, RitC(RitRed));
			RitTag(*C, Box.X + Box.W - 52.0f, Box.Y - 10.0f, "TEAM", RitRed, RitWhite, 12.0f, 7.0f, 18.0f);
		}
	}
	// Knockouts (§3.5, g1-audit L1): a seat that ended a hand with no chips and is gone when the next is dealt (BuildSeats empties
	// it) leaves as its card, desaturated, sliding out from the felt's centre and fading over 0.6 s (the motion kit's ease; a
	// 0.1 s beat first). Real time only: the Kast preview neither records nor draws them, so its still is the settled table.
	if (!Previewing)
	{
		const std::string SeatsKey = S.T->Spec.Id + "#" + std::to_string(S.TableId);
		if (U.SeatsFor != SeatsKey)
		{
			U.Knockouts.clear();
		}
		else if (U.SeatsSeen.size() == S.Seats.size() && Now - U.SeatsAt < 1.0)
		{
			for (size_t I = 0; I < S.Seats.size(); ++I)
			{
				const RitSeatSeen& Was = U.SeatsSeen[I];
				const SeatVis& V = S.Seats[I];
				if (Was.Present && !Was.Hero && Was.Stack <= 0 && (!V.Present || V.Id != Was.Id))
				{
					U.Knockouts.push_back({static_cast<int>(I), Was.Name, Now});
				}
			}
		}
		U.SeatsFor = SeatsKey;
		U.SeatsAt = Now;
		U.SeatsSeen.assign(S.Seats.size(), RitSeatSeen());
		for (size_t I = 0; I < S.Seats.size(); ++I)
		{
			const SeatVis& V = S.Seats[I];
			U.SeatsSeen[I] = {V.Present, V.IsHero, V.Id, V.Name, V.Stack};
		}
		U.Knockouts.erase(std::remove_if(U.Knockouts.begin(), U.Knockouts.end(), [Now](const RitKnockout& K) { return Now - K.At >= 0.7 || Now < K.At; }), U.Knockouts.end());
		for (const RitKnockout& K : U.Knockouts)
		{
			const Vec2 P = SlotPos(K.Seat);
			const Rect Box = rlsites_detail::SitesSeatBox(Look, P, false);
			const float Dx = P.X - Look.Cx;
			const float Dy = P.Y - Look.Cy;
			const float Len = std::max(1.0f, std::sqrt(Dx * Dx + Dy * Dy));
			const float Gone = static_cast<float>(motion::CountUp(0.0, 1.0, Now, K.At + 0.1, 0.6));
			const Rect G{Box.X + Dx / Len * 64.0f * Gone, Box.Y + Dy / Len * 64.0f * Gone, Box.W, Box.H};
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * (1.0f - Gone));
			C->FillPolygon(rlsites_detail::SitesCutPath(G, 6.0f), RitC(RitPanel2));
			C->StrokePolyline(rlsites_detail::SitesCutPath(G, 6.0f), true, RitC(RitHair), 1.0f);
			C->FillRect({G.X + 12.0f, G.Y + 12.0f, 40.0f, 40.0f}, RitC(RitHair)); // the square avatar, its colour gone
			RitText(*C, K.Name, G.X + 60.0f, G.Y + 28.0f, 14.0f, RitC(RitMuted), RitFace::Name, Align::Left, Baseline::Alphabetic, G.W - 70.0f);
			RitTag(*C, G.X + 60.0f, G.Y + 36.0f, "OUT", RitMuted, RitMuted, 11.0f, 5.0f, 16.0f, 4.0f, true);
			C->SetAlpha(A0);
		}
	}
	// The felt's centre line (§3.5 Centre; g1-audit): the event, the level, the blinds and the ante in Mono 13, Muted (6.69:1 on
	// the cloth). The shared table keeps the space under the board for the pot, the hand's name and the Second Board, so the
	// line sits on the felt's axis above the board, under the turnstone mark and clear of the top seat's bet (from y 303).
	{
		const Level& Lv = S.T->CurrentLevel();
		const std::string Centre = RitUpper(RitEventName(S.T->Spec.Name)) + " \xC2\xB7 LVL " + std::to_string(S.T->LevelIndex + 1) + " \xC2\xB7 " +
			ChipsText(static_cast<double>(Lv.Sb)) + "/" + ChipsText(static_cast<double>(Lv.Bb)) + (Lv.Ante > 0 ? " (" + ChipsText(static_cast<double>(Lv.Ante)) + ")" : std::string());
		RitText(*C, Centre, Look.Cx, Look.Cy - 140.0f, 13.0f, RitC(RitMuted), RitFace::Mono, Align::Center, Baseline::Alphabetic, 420.0f);
	}
	// The broadcast lower third while an all-in runs out (information after the action only, U1).
	if (rlsites_detail::SitesLowerThirdUp(S))
	{
		const Rect Band = Look.LowerThird.W > 0.0f ? Look.LowerThird : Rect{250.0f, 598.0f, 680.0f, 36.0f};
		RitLowerThird(X, Band);
	}
	// A win, once the replay awards it: the twin light sweeps under the winner's card (0.3 s, a Hot White flash at its head)
	// and a line goes to the tape with its flash; a pot of 50 BB or more is the POT OF THE SESSION. No confetti, ever.
	const SeatVis* Winner = nullptr;
	for (const SeatVis& V : S.Seats)
	{
		Winner = !Winner && V.Present && V.Winner ? &V : Winner;
	}
	if (Winner && S.CurHand && S.CurHand->bComplete)
	{
		const std::string WinKey = S.T->Spec.Id + "#" + std::to_string(S.TableId) + "#" + std::to_string(S.HandsPlayed);
		if (!Previewing && U.WinFor != WinKey) // the Kast preview draws the still and leaves the sweep and the tape line to the player's view
		{
			U.WinFor = WinKey;
			U.WinAt = Now;
			Chips Pot = 0;
			for (const PotResult& P : S.CurHand->PotResults)
			{
				Pot += P.Amount;
			}
			const Chips Bb = std::max<Chips>(1, S.T->CurrentLevel().Bb);
			const double InBb = static_cast<double>(Pot) / static_cast<double>(Bb);
			const bool Big = InBb >= 50.0;
			RitTapeItem Item;
			Item.Key = "win:" + WinKey;
			Item.Text = (Big ? std::string("POT OF THE SESSION \xC2\xB7 ") : std::string()) + RitUpper(Winner->Name) + " TAKES " + Fixed(InBb, InBb < 100.0 ? 1 : 0) + " BB \xC2\xB7 " +
				RitUpper(RitEventName(S.T->Spec.Name));
			Item.Rgb = Big ? RitWhite : RitInk;
			U.Extra.insert(U.Extra.begin(), Item);
			if (U.Extra.size() > 3)
			{
				U.Extra.pop_back();
			}
			U.TapeKey = -1; // rebuilt this frame: the new line flashes
		}
		const double Age = Previewing ? 1.0 : Now - U.WinAt;
		if (Age < 1.4)
		{
			const Rect Box = rlsites_detail::SitesSeatBox(Look, SlotPos(Winner->Seat), Winner->IsHero);
			const float P = static_cast<float>(Clamp01(Age / 0.3));
			const float A0 = C->GetAlpha();
			C->SetAlpha(A0 * static_cast<float>(Clamp01((1.4 - Age) / 0.4)));
			const float Ly = Box.Y + Box.H + 5.0f;
			RitGlow(*C, {Box.X, Ly - 3.0f, Box.W * P, 8.0f}, RitRed, 0.45f, 4.0f);
			C->FillRect({Box.X, Ly, Box.W * P, 2.0f}, RitC(RitRed));
			C->FillRect({Box.X, Ly + 0.5f, Box.W * P, 1.0f}, RitC(RitLed));
			C->FillRect({Box.X, Ly + 5.0f, Box.W * P * 0.82f, 1.0f}, RitC(RitRed));
			if (P < 1.0f)
			{
				const float Hx = Box.X + Box.W * P;
				C->GlowRoundRect({Hx - 9.0f, Ly - 8.0f, 18.0f, 18.0f}, 9.0f, RitC(RitWhite, 0.35f), 8.0f);
				C->FillRect({Hx - 10.0f, Ly, 12.0f, 2.0f}, RitC(RitWhite));
			}
			C->SetAlpha(A0);
		}
	}
	// The Second Board: once the hand is over and its last card is up, what a second run would have dealt (for show). An
	// all-in's run-out is a replay of a hand already complete, so "over" is the river up and the showdown printed (the lower
	// third down): before the river the flop's last card would pass for the board's.
	if (S.CurHand && S.CurHand->bComplete && S.Board.size() == 5 && S.BoardShownAt.size() == S.Board.size() && Now >= S.BoardShownAt.back() &&
		!rlsites_detail::SitesLowerThirdUp(S))
	{
		const std::string HandId = S.T->Spec.Id + "#" + std::to_string(S.TableId) + "#" + std::to_string(S.HandsPlayed);
		if (U.SecondHand != HandId)
		{
			int HeroIndex = -1;
			for (size_t I = 0; I < S.CurHand->Seats.size(); ++I)
			{
				HeroIndex = S.CurHand->Seats[I].Id == HeroId ? static_cast<int>(I) : HeroIndex;
			}
			if (Previewing)
			{
				// The Kast preview draws it as a still and leaves the player's own Second Board (its flap, its 4 s) unspent. The
				// ghost needs only the second run and the verdict, so a preview frame never pays for the record's equity.
				site::show::SecondBoard Out;
				if (HeroIndex >= 0 && site::show::SecondRun(*S.CurHand, HeroIndex, HandId, Out) && Out.HeroIn)
				{
					RitSecond Still;
					Still.Twice = Out.Cards;
					Still.Verdict = Out.Verdict;
					Still.First = Out.First;
					RitSecondShow(X, Still, Look.Cx, Look.Cy + 104.0f, 1.0);
				}
				return;
			}
			// The record is the finished hand's, whole (site::show::SecondRecordOf): this frame is the showdown, a beat before
			// the award marks the seats' Winner, and their equities have followed the run-out to the river's 100 / 0, so the
			// seats are never read for it (the lead's PIE, 2026-10-21: RAN "LOST" at "100% | 0%" under "you'd have WON BOTH").
			site::show::SecondRecord Rec;
			const bool Have = HeroIndex >= 0 && site::show::SecondRecordOf(*S.CurHand, HeroIndex, HandId, Rec);
			U.SecondHand = HandId;
			U.SecondShown = false;
			if (Have)
			{
				Rec.EventId = S.T->Spec.Id;
				Rec.At = World;
				U.Seconds.push_back(RitSecondOf(Rec));
				if (U.Seconds.size() > 12)
				{
					U.Seconds.erase(U.Seconds.begin());
				}
				U.SecondShown = true;
				U.SecondAt = Now;
				RitSound(X, SoundId::RitFlap, 0.3f);
			}
		}
		const double Age = Now - U.SecondAt;
		if (U.SecondShown && !U.Seconds.empty() && Age < 4.0)
		{
			RitSecondShow(X, U.Seconds.back(), Look.Cx, Look.Cy + 104.0f, Previewing ? 1.0 : Age); // the preview: its still while it's up
		}
		else if (!Previewing && U.SecondShown && Age >= 4.0)
		{
			U.SecondFirst = false;
		}
	}
}
} // namespace ui
} // namespace ss
