#include "ShortStack/Game/Sites.h"

#include "SitesDetail.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>

// The poker sites' data (update 2 §5.3) and their rake (§5.8). RiverLine first, exactly as it always was: its rake is FeeOf
// with 0.915 of the rest to the pools, its fields keep the 1.2x floor, its tickers are today's, and it is linked to the bank
// (no wallet, no fees, no delays: U8). Then the two rivals that ship at G3: Ignite Poker, the soft and loud first rival, and
// RunItTwice, the tougher one with the team. Halcyon (P1) isn't registered, so its network stays empty and no id reaches it.
// The money rules are SiteMoney.cpp's; the routing lives in Network.cpp; the schedules in NetworkSites.cpp (core-sites).

namespace ss
{
namespace net
{
namespace sites_detail
{
SiteDef SitesRiverLine()
{
	SiteDef D;
	D.Id = SiteId::RiverLine;
	D.Key = "riverline";
	D.Name = "RiverLine";
	D.Tagline = "Tournaments around the clock.";
	D.Rep = "RiverLine Player Relations";
	D.RepTitle = "Player Relations";
	D.TeamName = "Team RiverLine";
	// pal:: in order (Ui.h): Bg, Bg2, Panel, Panel2, Line, Ink, Muted, Dim, Accent, Accent2, Gold, Red, Orange, Green, Felt, FeltDark.
	D.Palette = {0x0a111c, 0x0e1726, 0x111c2e, 0x16233a, 0x223352, 0xe6edf7, 0x7b8aa3, 0x4d5b73, 0x27d3c3, 0x3b82f6, 0xf2c14e, 0xef4d5a, 0xf28a3a, 0x3ecf6e, 0x0f6a4e,
		0x083a2b};
	D.Rake.Kind = 0;
	D.Linked = true; // the bank's site: no wallet, no fees, no delays (U8)
	return D;
}

/** A card: instant deposits at a fee (§5.5's P0 deposit method). */
CashMethod SitesCard(int FeePermille)
{
	CashMethod M;
	M.Id = "card";
	M.Name = "Card";
	M.FeePermille = FeePermille;
	M.MinCents = 1000;
	M.DepositHours = 0.0;
	M.Withdraws = false;
	return M;
}

/**
 * A bank transfer: the P0 way out (a fee, a wait drawn once per request between the two hours). Its deposits (24 hours, $50
 * minimum, free) are P1 and the method's P1 flag says so: a deposit refuses it until they land.
 */
CashMethod SitesBank(Chips WithdrawFee, double HoursMin, double HoursMax)
{
	CashMethod M;
	M.Id = "bank";
	M.Name = "Bank transfer";
	M.FeePermille = 0;
	M.MinCents = 5000;
	M.DepositHours = 24.0;
	M.Withdraws = true;
	M.WithdrawFeeCents = WithdrawFee;
	M.WithdrawHoursMin = HoursMin;
	M.WithdrawHoursMax = HoursMax;
	M.P1 = true; // its deposit side
	return M;
}

/**
 * Ignite Poker (§5.3): "Poker that's actually fun." A bright app that launches in the world on Mon Nov 2, 2026, with a week
 * of guarantees half again as big. Soft tables (the "soft" population, Pending.Strength -0.06), 11% rake on everything, 30
 * seconds a hand at the hypers, a third of RiverLine's traffic (busiest at 8 PM, Sundays x1.2). It invites after Act I and 150
 * paid online events; its welcome is 100% of the first deposit up to $100, released by rake. Coral and sky on plum: every one
 * of its 16 colours differs from RiverLine's, so the laptop's palette-leak check reads any RiverLine colour inside its skin as
 * a leak.
 *
 * Update 3 (SPEC §2.3, §1.11; core-sites-look, G1): from midnight to 6 AM world time the whole skin turns to Afterburner Hours,
 * a second 16-token palette (NightPalette, chosen by SitesSkin from the world minute inside NightFromMin..NightToMin): violet
 * on midnight plum, aqua for information, the same sunflower money, loss red, tangerine and lime as the day's, and a violet
 * velvet cloth. Its anchors are the design's (Bg 120c1f, Panel 1a1230, Accent b57bff, Accent2 3ee6d0, Ink f3eaff); the rest
 * are derived on the same steps as the day palette's. Every one differs from RiverLine's, so the same leak check holds at night.
 */
SiteDef SitesIgnite()
{
	SiteDef D;
	D.Id = SiteId::Ignite;
	D.Key = "ignite";
	D.Name = "Ignite";
	D.Tagline = "Poker that's actually fun.";
	D.Rep = "Bree Okafor";
	D.RepTitle = "Ignite Partnerships";
	D.Palette = {0x1a1220, 0x20162a, 0x24182c, 0x2e2038, 0x46325a, 0xfff4ec, 0xc4afc9, 0x8a7493, 0xff6b57, 0x4fc3f7, 0xffc94a, 0xff4d6d, 0xff9e3d, 0x5be08a, 0x4a2142,
		0x2c1228};
	// Afterburner Hours, in pal:: order: Bg, Bg2, Panel, Panel2, Line, Ink, Muted, Dim, Accent (violet), Accent2 (aqua), Gold, Red,
	// Orange, Green, Felt (violet velvet), FeltDark.
	D.NightPalette = {0x120c1f, 0x160f28, 0x1a1230, 0x221a3c, 0x382a5e, 0xf3eaff, 0xb4a6d6, 0x7a6c9a, 0xb57bff, 0x3ee6d0, 0xffc94a, 0xff4d6d, 0xff9e3d, 0x5be08a,
		0x3b2160, 0x22123a};
	D.NightFromMin = 0;   // 00:00
	D.NightToMin = 360;   // 06:00, the Night Shift's close
	D.Rake.Kind = 1;
	D.Rake.Flat = 0.11;
	D.Traffic.Scale = 0.30;
	D.Traffic.Sunday = 1.2;
	D.Traffic.PeakHour = 20;
	D.Traffic.OnlineBase = 95000;
	D.Traffic.OnlineSwing = 28000;
	D.Softness = {-0.06, -0.06, -0.06, -0.06, -0.06};
	D.Population = {"freeroll", "soft", "soft", "low", "high"};
	D.SecondsPerHand[0] = 30.0;
	D.SecondsPerHand[1] = 38.0;
	D.FieldFloor = false;
	D.GtdOfPool = 0.9;
	D.LaunchGtd = 1.5;
	D.TeamSize = 0; // Ignite Squad is P1
	D.Cashier = {SitesCard(30), SitesBank(300, 72.0, 120.0)};
	D.MinWithdraw = 2000;
	D.Gate.MinEvents = 150;
	D.Gate.MinRentsPaid = 2;
	D.LaunchDay = 28; // Mon Nov 2, 2026
	return D;
}

/**
 * RunItTwice (§5.3): "Tournaments for grown-ups." The rival the sponsor catalog already named (its brand keeps the id
 * "turnstone"): bigger guarantees, tougher fields (one band up from RiverLine at the same buy-in, Pending.Strength +0.03),
 * rake that falls at the top (8% to $55, 7% from $109, 5.5% from $530), 55% of RiverLine's traffic (busiest at 9 PM, Sundays
 * x1.5), its own team of six. Its new lobby and points season opened on Night One. It invites at 300 tracked online events
 * with an $11 average over the last hundred, or an online series title, or Kast Partner; its welcome is $33 in tickets.
 *
 * Its colours are the Exchange after the bell (Update 3, SPEC §3.3 and §13, the user's 2026-10-21 restyle; core-sites-look,
 * G1): red and black, all 16 tokens replaced. Onyx ground, carbon panels, frost ink; Signal Red as the one accent ("red is
 * light, never ink": fills, strokes and glows, never text on the dark), chrome as the second, Hot White for money, Signal Down
 * for losses, Caution Amber for projected overlays, Signal Up for gains, and black speed cloth. Ink on Panel 15.30:1, Muted on
 * Bg 7.54:1; every one of the 16 apart from RiverLine's and from Ignite's. The sponsor brand "turnstone" (the team's patch,
 * badges and book) keeps the heritage slate and gold: Sponsor.cpp and the patch atlas are frozen this update.
 */
SiteDef SitesRunItTwice()
{
	SiteDef D;
	D.Id = SiteId::RunItTwice;
	D.Key = "runittwice";
	D.Name = "RunItTwice";
	D.Tagline = "Tournaments for grown-ups.";
	D.Rep = "Ingrid Vale";
	D.RepTitle = "RunItTwice Player Development";
	D.TeamName = "Team RunItTwice";
	// Onyx, Bg2, Carbon, Panel2, Hairline, Frost, Muted, Dim, Signal Red, Chrome, Hot White, Signal Down, Caution Amber, Signal Up,
	// black cloth, its dark edge.
	D.Palette = {0x0c0c0f, 0x111114, 0x16161a, 0x1e1e23, 0x2c2c33, 0xececef, 0xa0a0aa, 0x62626c, 0xd2141a, 0xb6bbc5, 0xffffff, 0xff3d3d, 0xf5a524, 0x48c77a, 0x1a1a1f,
		0x09090b};
	D.Rake.Kind = 2;
	D.Rake.Steps = {{0, 0.08}, {10900, 0.07}, {53000, 0.055}};
	D.Traffic.Scale = 0.55;
	D.Traffic.Sunday = 1.5;
	D.Traffic.PeakHour = 21;
	D.Traffic.OnlineBase = 175000;
	D.Traffic.OnlineSwing = 52000;
	D.Softness = {0.03, 0.03, 0.03, 0.03, 0.03};
	D.Population = {"freeroll", "low", "high", "high", "high"};
	D.SecondsPerHand[0] = 36.0;
	D.SecondsPerHand[1] = 42.0;
	D.FieldFloor = false;
	D.GtdOfPool = 0.9;
	D.LaunchGtd = 1.0;
	D.TeamSize = 6;
	D.Cashier = {SitesCard(25), SitesBank(500, 48.0, 48.0)};
	D.MinWithdraw = 2000;
	D.Gate.MinEvents = 300;
	D.Gate.MinRentsPaid = 2; // no rival invites before Act I is over (U11)
	D.Gate.MinAbi100 = 1100;
	D.Gate.SeriesTitle = true;
	D.Gate.KastPartner = true;
	D.LaunchDay = NightOneDay; // its new lobby opened Tue Oct 6, 2026
	return D;
}
} // namespace sites_detail

Chips RakeRule::Fee(Chips BuyIn) const
{
	// Kind 0: RiverLine's (FeeOf), to the cent. Kind 1: a flat share of the buy-in. Kind 2: the share of the last step at or
	// under the buy-in. A rival's fee is whole cents, and its prize share is what's left of the buy-in (PrizeShare), so the fee
	// and the pool's part always add up to the buy-in.
	if (Kind == 0)
	{
		return FeeOf(BuyIn);
	}
	if (BuyIn <= 0)
	{
		return 0;
	}
	double Rate = Flat;
	if (Kind == 2)
	{
		for (const std::pair<Chips, double>& S : Steps)
		{
			Rate = BuyIn >= S.first ? S.second : Rate;
		}
	}
	return static_cast<Chips>(std::llround(static_cast<double>(BuyIn) * Rate));
}

double RakeRule::PrizeShare(const EventTemplate& T) const
{
	// Kind 0: RiverLine's (0.915 of a paid event's buy-in to its pool, bit for bit). A rival: what its fee leaves.
	if (T.BuyInCents == 0)
	{
		return 0.0;
	}
	if (Kind == 0)
	{
		return 0.915;
	}
	return static_cast<double>(T.BuyInCents - Fee(T.BuyInCents)) / static_cast<double>(T.BuyInCents);
}

const std::vector<SiteDef>& Sites()
{
	static const std::vector<SiteDef> All = {sites_detail::SitesRiverLine(), sites_detail::SitesIgnite(), sites_detail::SitesRunItTwice()};
	return All;
}

const SiteDef& Def(SiteId S)
{
	for (const SiteDef& D : Sites())
	{
		if (D.Id == S)
		{
			return D;
		}
	}
	return Sites().front();
}
} // namespace net

namespace site
{
net::HeroStats HeroOn(net::SiteId S, const std::string& Name, const std::vector<HistoryEntry>& History, const Book& B, const world::World* W, double Now)
{
	if (S == net::SiteId::RiverLine)
	{
		// RiverLine's boards (bug 7): its own events only (StatsFrom), and once History is full (the last hundred can't be the
		// whole record) the lifetime numbers by subtraction from the world's tracker, which follows the player everywhere.
		// Never below what History shows, though (C3): the tracker only exists since abeb60b and nothing back-fills it, so a
		// career older than it, or a save that kept no world (the creator fixture), has a tracker short of its own last
		// hundred, and the YOU row would fall to $0. Each number is the larger of the two.
		net::HeroStats H = net::StatsFrom(Name, History, Now);
		if (W && History.size() >= 100)
		{
			const world::Tracker& T = W->HeroStats();
			const world::TrackLine& Live = T.ByStake[static_cast<size_t>(world::TrackStake::Live)];
			Chips Rivals = 0;
			int RivalEvents = 0;
			for (const Account& A : B.Accounts)
			{
				Rivals += A.LifetimeWon;
				RivalEvents += A.LifetimeEvents;
			}
			H.Earnings = std::max<Chips>(H.Earnings, T.Prizes - Live.Prizes - Rivals);
			H.Tournaments = std::max(H.Tournaments, T.Events - Live.Events - RivalEvents);
			H.SeasonPoints = std::max(H.SeasonPoints, W->HeroSeasonPoints());
		}
		return H;
	}
	// A rival's boards: the account's record (its season and what it has paid), and from History (the last hundred) the
	// finishes, this month's points (Ignite's Month board reads them as SiteBoard) and the running series there.
	net::HeroStats H;
	H.Name = Name;
	const int Year = world::YearOf(net::DayOf(Now));
	if (const Account* A = AccountOf(B, S))
	{
		H.Earnings = A->LifetimeWon;
		H.Tournaments = A->LifetimeEvents;
		if (A->Hero.Year == Year)
		{
			H.SeasonPoints = A->Hero.Points;
			H.SeasonWins = A->Hero.Wins;
			H.SeasonFinalTables = A->Hero.FinalTables;
		}
	}
	const net::Network& Net = net::Site(S);
	const net::SeriesInfo* Running = Net.CurrentSeries(Now);
	const std::string Month = MonthKeyAt(Now);
	for (const HistoryEntry& E : History)
	{
		if (net::SiteOf(E.EventId) != S)
		{
			continue;
		}
		const double Points = net::Points(E.Place, E.Entrants, std::max<Chips>(0, E.BuyInCents));
		const double Start = net::EntryStart(E);
		H.Wins += E.Place == 1 ? 1 : 0;
		H.FinalTables += E.Place >= 1 && E.Place <= 9 ? 1 : 0;
		H.Cashes += E.Prize > 0 ? 1 : 0;
		H.Best = std::max(H.Best, E.Prize);
		H.SiteBoard += Start <= Now && MonthKeyAt(Start) == Month ? Points : 0.0;
		const size_t Cut = E.EventId.rfind('@');
		const net::EventTemplate* T = Cut == std::string::npos ? nullptr : Net.FindTemplate(E.EventId.substr(0, Cut));
		if (T && !T->Series.empty())
		{
			H.SeriesTitles += E.Place == 1 ? 1 : 0;
			H.SeriesPoints += Running && Running->Id == T->Series ? Points : 0.0;
		}
	}
	return H;
}
} // namespace site
} // namespace ss
