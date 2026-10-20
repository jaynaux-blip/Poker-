// night_one_session: core-sites' additions (update 2 SPEC §6.7): a rival event registers and pays into its wallet (the bank
// and its ledger don't move), the player's RiverLine numbers untouched by a rival's result (the boards, Night Shift points,
// Player of the Year), Restricted(site), the invitation answered and the Cashier through the session, Rewards on the 1st, the
// retention letter, and a save holding the sites' lines. Compiled into session_test and called from main() in session_main.cpp
// (the kit is SessionSeg.h); helpers in session_test::sites_seg. A backer's share off a rival wallet is core-deals' RivalPot
// (session_seg_stake.cpp), and the clause's warning and strike core-deals' Clause (session_seg_sponsor.cpp), in this suite.
#include "SessionSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <cstdio>
#include <string>
#include <vector>

namespace session_test
{
namespace sites_seg
{
namespace net = ss::net;
namespace site = ss::site;
using ss::Chips;

void Lobby(ss::Session& S, double& Now)
{
	S.CurrentScreen = ss::Screen::Lobby;
	Now = Wait(S, Now, 0.2);
}

/** A rival's event the player can join now (registering or in late registration), a freezeout at $5.50 or less. */
bool Joinable(const ss::Session& S, net::SiteId Site, ss::LobbyEvent& Out)
{
	const net::Network& N = net::Site(Site);
	const double World = S.WorldMinutes();
	for (const net::EventInstance& E : N.Window(World - 120.0, World + 600.0))
	{
		const net::EventTemplate& T = N.TemplateOf(E);
		const net::LiveState L = N.Live(E, World);
		std::string Lock;
		if (T.Fmt == net::Format::Freezeout && T.BuyInCents > 0 && T.BuyInCents <= 550 && (L.St == net::Status::Registering || L.St == net::Status::LateReg) &&
			N.Joinable(E, &Lock, S.Unlocks()))
		{
			Out = N.Listing(E, nullptr, S.Unlocks());
			return true;
		}
	}
	return false;
}

/** A RiverLine event the same way. */
bool JoinableHome(const ss::Session& S, ss::LobbyEvent& Out)
{
	const net::Network& N = net::Shared();
	const double World = S.WorldMinutes();
	for (const net::EventInstance& E : N.Window(World - 120.0, World + 600.0))
	{
		const net::EventTemplate& T = N.TemplateOf(E);
		const net::LiveState L = N.Live(E, World);
		if (T.JoinIndex < 0 && T.Fmt == net::Format::Freezeout && T.BuyInCents > 0 && T.BuyInCents <= 1100 && (L.St == net::Status::Registering || L.St == net::Status::LateReg) &&
			N.Joinable(E, nullptr, S.Unlocks()))
		{
			Out = N.Listing(E, nullptr, S.Unlocks());
			return true;
		}
	}
	return false;
}

Chips WalletLines(const site::Account& A)
{
	Chips Sum = 0;
	for (const site::Line& L : A.Lines)
	{
		Sum += L.Cents;
	}
	return Sum;
}

/** RunItTwice opens with Night One: an invitation, Accept, a deposit, a table, the results; the bank never hears of the event. */
void RivalEvent()
{
	Hooks H;
	ss::Session S(H, "sites-rival");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 100000;
	const double World = S.WorldMinutes();
	ss::LobbyEvent Ev;
	Expect(Joinable(S, net::SiteId::RunItTwice, Ev) && net::SiteOf(Ev.Spec.Id) == net::SiteId::RunItTwice, "sites: a RunItTwice table is registering on Night One");
	Expect(!S.CanAfford(Ev), "sites: no account, no RunItTwice table");
	Expect(!S.AnswerInvite(net::SiteId::RunItTwice, true).empty(), "sites: nothing to accept before the invitation");
	site::Invite(S.Life.Sites, S.Life, net::SiteId::RunItTwice, World, 0, false);
	Expect(S.AnswerInvite(net::SiteId::RunItTwice, true).empty() && site::WalletOf(S.Life.Sites, net::SiteId::RunItTwice) && S.Life.Inbox.back().Subject == "Your RunItTwice account is open",
		"sites: Accept opens the account, and the welcome letter comes at once");
	Expect(S.Deposit(net::SiteId::RunItTwice, 999, "").size() > 0 && S.Deposit(net::SiteId::RunItTwice, 4000, "card").empty() && S.BankrollCents == 100000 - 4100,
		"sites: the Cashier through the session: under $10 refused; $40 by card costs the bank $41.00");
	bool Noticed = false;
	for (const ss::life::Letter& M : S.Life.Inbox)
	{
		Noticed = Noticed || (M.Source == "site" && M.Thread == "riverline" && M.Subject == "We noticed");
	}
	Expect(Noticed && S.Life.Sites.RiverLine.BoostUntil > World, "sites: RiverLine notices the first deposit: \"We noticed.\" and a tier up");
	site::Account* A = site::AccountOf(S.Life.Sites, net::SiteId::RunItTwice);
	const Chips Bank = S.BankrollCents;
	const size_t Ledger = S.Life.Ledger.size();
	const Chips Rewards = S.Life.Sites.RiverLine.Rake;
	const net::HeroStats Before = net::StatsFrom(S.HeroName, S.History, World);
	const double Poy = S.Living().HeroSeasonPoints();
	Expect(S.CanAfford(Ev), "sites: with $40 on RunItTwice, its table can be afforded");
	S.RegisterEvent(Ev);
	Expect(S.T && S.T->Spec.Id == Ev.Spec.Id && A->Cents == 4000 - Ev.BuyInCents && S.BankrollCents == Bank && S.Life.Ledger.size() == Ledger &&
			A->Lines.back().Cents == -Ev.BuyInCents && A->RakeMonth == net::Def(net::SiteId::RunItTwice).Rake.Fee(Ev.BuyInCents),
		"sites: the buy-in comes off the RunItTwice wallet with its own line; the bank and its ledger don't move");
	ss::Rng Choice("sites-rival-choices");
	int Decisions = 0;
	Now = PlayOut(S, Choice, Now, Decisions);
	const ss::Results& R = S.LastResults;
	std::printf("  sites: RunItTwice %s, %s of %d, %s\n", Ev.Spec.Name.c_str(), ss::Ordinal(R.Place).c_str(), R.Entrants, ss::Money(R.PrizeCents + R.BountyCents).c_str());
	Expect(S.CurrentScreen == ss::Screen::Results && A->Cents == 4000 - Ev.BuyInCents + R.PrizeCents + R.BountyCents && A->Cents == WalletLines(*A) &&
			S.BankrollCents == Bank && S.Life.Ledger.size() == Ledger,
		"sites: the prize lands in the wallet with its lines; the bank and its ledger never moved");
	Expect(A->LifetimeEvents == 1 && A->Hero.Events == 1 && A->LifetimeWon == R.PrizeCents + R.BountyCents && S.Life.Sites.RiverLine.Rake == Rewards &&
			A->Hero.Points == net::Points(R.Place, R.Entrants, R.BuyInCents) && A->Hero.Won == R.PrizeCents + R.BountyCents,
		"sites: the player's record on RunItTwice counts it (its season's points too); RiverLine Rewards doesn't");
	// (Review.) The settle's record for a deep run, on a copy of the book (the session's table is still this event's): a win at
	// $3.30 of 44 adds its points, the win and the final table to the season there, and the prize and bounties to what it paid.
	if (S.T && net::SiteOf(S.T->Spec.Id) == net::SiteId::RunItTwice)
	{
		const site::Book Kept = S.Life.Sites;
		ss::Results Win;
		Win.EventName = "a deep run";
		Win.Place = 1;
		Win.Entrants = 44;
		Win.BuyInCents = 330;
		Win.PrizeCents = 4400;
		Win.BountyCents = 300;
		const site::Season Was = A->Hero;
		const Chips Won = A->LifetimeWon;
		S.SiteSettle(Win);
		Expect(A->Hero.Points == Was.Points + net::Points(1, 44, 330) && A->Hero.Wins == Was.Wins + 1 && A->Hero.FinalTables == Was.FinalTables + 1 &&
				A->Hero.Won == Was.Won + 4700 && A->Hero.Events == Was.Events + 1 && A->LifetimeWon == Won + 4700 && A->Lines.back().Cents == 4400 &&
				A->Lines[A->Lines.size() - 2].Cents == 300,
			"sites: a rival win settles its points, win, final table and money into the season there, one line each for prize and bounties");
		S.Life.Sites = Kept;
		A = site::AccountOf(S.Life.Sites, net::SiteId::RunItTwice);
	}
	const net::HeroStats After = net::StatsFrom(S.HeroName, S.History, S.WorldMinutes());
	Expect(S.History.size() == 1 && net::SiteOf(S.History.front().EventId) == net::SiteId::RunItTwice && After.Tournaments == Before.Tournaments && After.Earnings == Before.Earnings &&
			After.SeasonPoints == Before.SeasonPoints && After.NightPoints == Before.NightPoints && After.FinalTables == Before.FinalTables && S.Living().HeroSeasonPoints() == Poy,
		"sites: a RunItTwice result moves none of the player's RiverLine numbers (boards, Night Shift points, Player of the Year)");
	// A final table on a rival, as its result line: the RiverLine boards still read nothing of it.
	ss::HistoryEntry Deep = S.History.front();
	Deep.Place = 2;
	Deep.Prize = 9000;
	std::vector<ss::HistoryEntry> With = S.History;
	With.insert(With.begin(), Deep);
	const net::HeroStats Final = net::StatsFrom(S.HeroName, With, S.WorldMinutes());
	Expect(Final.FinalTables == Before.FinalTables && Final.Earnings == Before.Earnings && Final.SeasonPoints == Before.SeasonPoints && Final.NightPoints == Before.NightPoints,
		"sites: a rival final table moves no RiverLine board row or Night Shift point");
	// The save keeps the sites' lines, and reads back as it was.
	S.Save();
	const std::string Text = H.Last.Serialize();
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(Text, Back) && Back.Serialize() == Text && Text.find("site\tacct\t2\t") != std::string::npos && Text.find("site\tinvite\t2\t") != std::string::npos,
		"sites: the save holds the account, its lines and the invitation, and writes back byte for byte");
}

/** A RiverLine restriction is RiverLine's alone (§5.5): RunItTwice still takes the player's money. */
void Restrictions()
{
	Hooks H;
	ss::Session S(H, "sites-ban");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 100000;
	const double World = S.WorldMinutes();
	site::Invite(S.Life.Sites, S.Life, net::SiteId::RunItTwice, World, 0, false);
	S.AnswerInvite(net::SiteId::RunItTwice, true);
	S.Deposit(net::SiteId::RunItTwice, 2000, "");
	S.Life.BannedUntil = World + 3.0 * 1440.0;
	ss::LobbyEvent Rival;
	ss::LobbyEvent Home;
	Expect(S.Restricted(net::SiteId::RiverLine) && !S.Restricted(net::SiteId::RunItTwice) && !S.Restricted(net::SiteId::Ignite), "sites: the restriction is RiverLine's");
	Expect(Joinable(S, net::SiteId::RunItTwice, Rival) && S.CanAfford(Rival) && JoinableHome(S, Home) && !S.CanAfford(Home),
		"sites: restricted on RiverLine, the player can still register on RunItTwice");
	ss::stake::Pot P = S.PotFor(net::SiteId::RunItTwice);
	ss::stake::Pot Bank = S.PotFor(net::SiteId::RiverLine);
	ss::stake::Pot None = S.PotFor(net::SiteId::Ignite);
	Expect(P.Cents == site::WalletOf(S.Life.Sites, net::SiteId::RunItTwice) && P.Site == static_cast<int>(net::SiteId::RunItTwice) && Bank.Cents == &S.BankrollCents && Bank.Site == 0 &&
			None.Cents == nullptr,
		"sites: PotFor: the bankroll for RiverLine, the wallet for a rival, nothing where there's no account");
}

/** (Review.) A decline is dated when it's given: the site writes once more 30 days after the answer, not after its first letter. */
void Decline()
{
	Hooks H;
	ss::Session S(H, "sites-decline");
	double Now = 0.0;
	Lobby(S, Now);
	const double World = S.WorldMinutes();
	site::Invite(S.Life.Sites, S.Life, net::SiteId::Ignite, World - 25.0 * 1440.0, 0, false);
	Expect(S.AnswerInvite(net::SiteId::Ignite, false).empty() && S.Life.Sites.Invites.back().State == 2 && S.Life.Sites.Invites.back().At == World &&
			!site::AccountOf(S.Life.Sites, net::SiteId::Ignite) && !S.AnswerInvite(net::SiteId::Ignite, true).empty(),
		"sites: Decline is dated the day it's given (the one more letter comes 30 days after it), and there's nothing to accept till then");
}

/** RiverLine Rewards on the 1st, through the session's calendar step: a prize line in the bank (kind 4). */
void RewardsOnTheFirst()
{
	Hooks H;
	ss::Session S(H, "sites-rewards");
	double Now = 0.0;
	Lobby(S, Now);
	const Chips Bank = S.BankrollCents;
	ss::LobbyEvent Home;
	Expect(JoinableHome(S, Home), "sites: a RiverLine table to buy into");
	S.BankrollCents = 100000;
	S.RegisterEvent(Home);
	Expect(S.Life.Sites.RiverLine.Rake == Home.Spec.FeeCents && S.Life.Sites.RiverLine.MonthKey == "2026-10", "sites: a RiverLine buy-in's fee counts toward October's Rewards");
	(void)Bank;
	S.Life.Sites.RiverLine.Rake = 4000; // a $40 month: Silver
	const Chips Before = S.BankrollCents;
	const double First = ss::world::DayOn(2026, 11, 1) * 1440.0;
	S.SiteStep(First - 1.0, First + 1.0);
	S.SiteStep(First + 1.0, First + 1440.0);
	Expect(S.BankrollCents == Before + 320 && S.Life.Ledger.front().Kind == 4 && S.Life.Ledger.front().Label == "RiverLine Rewards: Silver, October",
		"sites: on Nov 1, Silver's 8% of October's $40 lands as one bank line, \"RiverLine Rewards: Silver, October\", once");
}
} // namespace sites_seg

void SegSites()
{
	sites_seg::RivalEvent();
	sites_seg::Restrictions();
	sites_seg::Decline();
	sites_seg::RewardsOnTheFirst();
}
} // namespace session_test
