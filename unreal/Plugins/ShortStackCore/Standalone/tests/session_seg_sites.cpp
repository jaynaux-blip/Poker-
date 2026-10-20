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
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace session_test
{
namespace seams
{
std::string Fixture(const char* Name); // session_seams.cpp (the lead's): a committed fixture's text
} // namespace seams

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

// ------------------------------------------------------------------ C3: the gate audit's fixes (c3-fixes)

int SitesLetters(const ss::Session& S, const char* Thread, const std::string& Subject)
{
	int N = 0;
	for (const ss::life::Letter& M : S.Life.Inbox)
	{
		N += M.Source == "site" && M.Thread == Thread && M.Subject == Subject ? 1 : 0;
	}
	return N;
}

/**
 * Should-fix 1: the creator fixture kept no world, so the world's tracker never saw its hundred results. RiverLine's YOU row
 * (site::HeroOn, as RiverLineNet.cpp reads it) is its History's, not $0, 0 tournaments and 0 points.
 */
void CreatorNumbers()
{
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(seams::Fixture("update2_creator.ssave"), D);
	Hooks H;
	ss::Session S(H, "sites-creator", &D);
	const double World = S.WorldMinutes();
	const net::HeroStats Hist = net::StatsFrom(S.HeroName, S.History, World);
	const net::HeroStats You = site::HeroOn(net::SiteId::RiverLine, S.HeroName, S.History, S.Life.Sites, &S.Living(), World);
	std::printf("  sites: the creator's RiverLine row: %s, %d events, %.0f points (History's %s, %d, %.0f; the tracker's %d events)\n", ss::Money(You.Earnings).c_str(),
		You.Tournaments, You.SeasonPoints, ss::Money(Hist.Earnings).c_str(), Hist.Tournaments, Hist.SeasonPoints, S.Living().HeroStats().Events);
	Expect(Parsed && S.History.size() >= 100 && Hist.Earnings > 0 && Hist.SeasonPoints > 0.0 && You.Earnings >= Hist.Earnings && You.Tournaments >= Hist.Tournaments &&
			You.SeasonPoints >= Hist.SeasonPoints,
		"sites: the creator's RiverLine row never falls below its last hundred (a save that kept no world)");
}

/** Should-fix 2, through the session: "Your tickets are in" arrives with the tickets, minutes after the welcome letter. */
void TicketsLetter()
{
	Hooks H;
	ss::Session S(H, "sites-tickets");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 100000;
	site::Invite(S.Life.Sites, S.Life, net::SiteId::RunItTwice, S.WorldMinutes(), 0, false);
	Expect(S.AnswerInvite(net::SiteId::RunItTwice, true).empty() && S.Deposit(net::SiteId::RunItTwice, 5000, "card").empty() &&
			S.Life.TicketsFor("runittwice.sunday-grind") == 1 && S.Life.TicketsFor("runittwice.supper-stack") == 2,
		"sites: a $50 first deposit on RunItTwice puts the $33 in tickets on the account");
	Expect(SitesLetters(S, "runittwice", "Your RunItTwice account is open") == 1 && SitesLetters(S, "runittwice", "Your tickets are in") == 1,
		"sites: and \"Your tickets are in\" says so, though the welcome letter came minutes before");
}

/**
 * The nice-to-haves on Team RunItTwice: the player's pro badge on their seat (Session.cpp's seat, as under Team RiverLine), and
 * should-fix 4: the block no longer says it never heard of a live site with a team.
 */
void TeamRunItTwice()
{
	Hooks H;
	ss::Session S(H, "sites-team");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 200000;
	site::Invite(S.Life.Sites, S.Life, net::SiteId::RunItTwice, S.WorldMinutes(), 0, false);
	S.AnswerInvite(net::SiteId::RunItTwice, true);
	S.Deposit(net::SiteId::RunItTwice, 5000, "card");
	const std::string Signed = S.DebugSponsor("sign turnstone");
	Expect(ss::sponsor::Active(S.Life.Sponsors, "turnstone", S.WorldMinutes()) && !S.TeamRiverLine(), ("sites: Team RunItTwice signed (" + Signed + ")").c_str());
	ss::LobbyEvent Ev;
	bool Pro = false;
	if (Joinable(S, net::SiteId::RunItTwice, Ev) && S.CanAfford(Ev))
	{
		S.RegisterEvent(Ev);
		Now = Wait(S, Now, 2.0);
		for (const ss::SeatVis& V : S.Seats)
		{
			Pro = Pro || (V.IsHero && V.Pro);
		}
	}
	Expect(S.T && Pro, "sites: under Team RunItTwice the player's seat wears the pro badge, as under Team RiverLine");
	// The patch on the chest: the block remarks on it, and knows the site is real.
	Expect(S.SewPatch(0, "turnstone").empty(), "sites: Team RunItTwice's patch sewn on");
	S.Life.Hunger = 0.0;
	S.Life.Thirst = 0.0;
	S.Life.Energy = 100.0;
	const ss::town::Talk T = ss::town::Open(*ss::town::ContextFor(S, "mei", ""));
	Expect(T.Topic == "look:patch:turnstone" && T.Line.find("RunItTwice") != std::string::npos && T.Line.find("Never heard") == std::string::npos,
		("sites: the block notices the RunItTwice patch and knows the site (" + T.Topic + ": " + T.Line + ")").c_str());
}

/** The nice-to-have on ss.Site.Advance: what it lands is dated now, the clock where it was; nothing sits in the future. */
void AdvanceDating()
{
	Hooks H;
	ss::Session S(H, "sites-advance");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 100000;
	site::Invite(S.Life.Sites, S.Life, net::SiteId::RunItTwice, S.WorldMinutes(), 0, false);
	S.AnswerInvite(net::SiteId::RunItTwice, true);
	S.Deposit(net::SiteId::RunItTwice, 5000, "card");
	Expect(S.Withdraw(net::SiteId::RunItTwice, 4000, "").empty(), "sites: $40 withdrawn from RunItTwice (48 hours, $5)");
	// (review-c3-fixes) A month's close inside the span too, so every kind of thing the advance lands is seen dated: October's
	// $60 of RunItTwice rake pays the Second Run Club's $9.00 on Nov 1, a wallet line and its letter, beside the bank line.
	site::OnBuyIn(S.Life.Sites, net::SiteId::RunItTwice, 0, 6000, "Buy-in", S.WorldMinutes());
	const size_t Letters = S.Life.Inbox.size();
	const size_t Lines = site::AccountOf(S.Life.Sites, net::SiteId::RunItTwice)->Lines.size();
	const double World = S.WorldMinutes();
	const Chips Bank = S.BankrollCents;
	S.DebugSite("advance 720");
	const site::Account* Rit = site::AccountOf(S.Life.Sites, net::SiteId::RunItTwice);
	const bool Paid = Rit && Rit->Lines.size() == Lines + 1 && Rit->Lines.back().Cents == 900 && S.Life.Inbox.size() > Letters;
	bool Past = S.WorldMinutes() == World;
	for (const site::Transfer& X : S.Life.Sites.Transfers)
	{
		Past = Past && X.State != 0 && X.ArrivesAt <= World;
	}
	for (const ss::life::LedgerEntry& E : S.Life.Ledger)
	{
		Past = Past && E.At <= World;
	}
	for (const ss::life::Letter& M : S.Life.Inbox)
	{
		Past = Past && M.At <= World;
	}
	for (const site::Account& A : S.Life.Sites.Accounts)
	{
		for (const site::Line& L : A.Lines)
		{
			Past = Past && L.At <= World;
		}
	}
	Expect(S.BankrollCents == Bank + 3500 && S.Life.Ledger.front().Label == "Withdrawal: RunItTwice" && S.Life.Ledger.front().Amount == 3500 && Paid && Past,
		"sites: ss.Site.Advance 720 lands the $35.00 and Nov 1's $9.00 rakeback now, dated now; no bank line, wallet line, letter or transfer after the clock");
}

/**
 * The nice-to-haves on a launch in this world (World::SiteLaunched): ss.Site.Launch opens Ignite mid-day on Night One; its News
 * has its launch at that minute, its invitation gate counts from it, Describe says it's open, and Rick's text comes a few nights
 * after it (not after Nov 2).
 */
void LaunchedHere()
{
	Hooks H;
	ss::Session S(H, "sites-launch");
	double Now = 0.0;
	Lobby(S, Now);
	ss::rep::Public P;
	P.OnlineEvents = 150;
	const double Night = S.WorldMinutes();
	Expect(!site::GateMet(net::SiteId::Ignite, P, 2, Night + 60.0), "sites: before Ignite opens in this world, its gate can't be met");
	const std::string Said = S.DebugSite("launch ignite");
	const double Opened = S.Living().SiteLaunched(net::SiteId::Ignite);
	Expect(Opened >= 0.0 && Opened <= S.WorldMinutes() && std::fmod(Opened, 1440.0) != 0.0, ("sites: ss.Site.Launch opens Ignite now, mid-day (" + Said + ")").c_str());
	net::HeroStats Hero;
	Hero.Name = S.HeroName;
	bool Live = false;
	for (const net::NewsItem& It : net::Site(net::SiteId::Ignite).News(S.WorldMinutes(), Hero, 20))
	{
		Live = Live || (It.Title == "Ignite Poker is live" && It.At == Opened);
	}
	Expect(Live, "sites: Ignite's News has its launch at the minute it opened, not nine hours on");
	Expect(site::GateMet(net::SiteId::Ignite, P, 2, S.WorldMinutes() + 60.0), "sites: Ignite's gate counts from when it opened in this world");
	// (review-c3-fixes) Ignite's own line: RunItTwice's, open since Night One, carries the same date.
	const std::string Describe = S.DebugSite("describe");
	const size_t Gate = Describe.find("Ignite's gate (");
	const std::string IgniteLine = Gate == std::string::npos ? std::string() : Describe.substr(Gate, Describe.find('\n', Gate) - Gate);
	Expect(IgniteLine.find("open since " + net::DateLabel(net::DayOf(Opened))) != std::string::npos,
		("sites: ss.Site.Describe says Ignite's open since it opened here (" + IgniteLine + ")").c_str());
	ss::life::State L = S.Life;
	L.BackRoomNights = 1;
	Chips Bank = 0;
	std::vector<std::pair<std::string, std::string>> Texts;
	site::Step(L.Sites, L, Bank, ss::rep::Public(), 0, Opened + 4.0 * 1440.0 + 21.0 * 60.0, Opened + 5.0 * 1440.0, Texts);
	bool Rick = false;
	for (const std::pair<std::string, std::string>& Tx : Texts)
	{
		Rick = Rick || (Tx.first == "Rick" && Tx.second.find("ignite") != std::string::npos);
	}
	Expect(Rick, "sites: Rick texts about Ignite a few nights after it opened in this world");
}
} // namespace sites_seg

void SegSites()
{
	sites_seg::RivalEvent();
	sites_seg::Restrictions();
	sites_seg::Decline();
	sites_seg::RewardsOnTheFirst();
	// C3 (c3-fixes): the gate audit's should-fixes and nice-to-haves.
	sites_seg::CreatorNumbers();
	sites_seg::TicketsLetter();
	sites_seg::TeamRunItTwice();
	sites_seg::AdvanceDating();
	sites_seg::LaunchedHere();
}
} // namespace session_test
