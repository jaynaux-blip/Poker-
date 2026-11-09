// site_play (update 3 SPEC §7.7, suite 18): the revamped sites' loops, the Value Governor and FUSE. Every Ignite pattern and
// RunItTwice profile under its bound, no grant past the Governor's share at its instant, the Guest List's holds clearing,
// keyed draws that reproduce across reloads and watch and skip, nothing paid twice, the League, FUSE's exact pool and
// odds, the strike gate's refusals. Default run <= 30 s; "long" runs the slow modes. core-siteplay's file.
//
// This file holds the book's own checks (no Session): the catalogs' static rules (2 and 3), the Governor's mechanics, the
// loops one at a time, the save's round trip, and the C++ model harness that reproduces SPEC §2.7 (Ignite's patterns,
// design_model_v11.py's, driven through the real hooks and claims) and §3.7 (RunItTwice's League through the real
// LeagueScore, its profiles through the real Governor and the real Second Run).
#include "SitePlaySeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitePlayPriv.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"

#include "CopyLint.h"
#include "Trademarks.h"

#include "ShortStack/Common.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace siteplay_test
{
int Failures = 0;
int Checks = 0;
bool Long = false;

void Expect(bool Condition, const char* What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

void Expect(bool Condition, const std::string& What)
{
	Expect(Condition, What.c_str());
}

namespace core_seg
{
namespace play = ss::site::play;
namespace detail = ss::site::play::siteplay_detail;
namespace net = ss::net;
namespace site = ss::site;
using ss::Chips;

constexpr double DayMin = 1440.0;
constexpr int Dec1 = 57; // world day 57: Tuesday, December 1, 2026

/** A life with the rival accounts open and money on them (no deposit: no welcome window, the ceiling 50%). */
ss::life::State Fresh(double World)
{
	ss::life::State L;
	site::Join(L.Sites, net::SiteId::Ignite, World - DayMin);
	site::Join(L.Sites, net::SiteId::RunItTwice, World - DayMin);
	for (site::Account& A : L.Sites.Accounts)
	{
		A.Cents = 100000000;
	}
	return L;
}

Chips FeeOn(net::SiteId S, Chips BuyIn)
{
	return net::Def(S).Rake.Fee(BuyIn);
}

/** One paid entry as RegisterEvent pays it: the wallet and OnBuyIn's line and rake, then the loops' hook. */
void Pay(ss::life::State& L, net::SiteId S, const std::string& EventId, Chips BuyIn, Chips Fee, double World)
{
	if (site::Account* A = site::AccountOf(L.Sites, S))
	{
		A->Cents -= BuyIn;
	}
	site::OnBuyIn(L.Sites, S, BuyIn, Fee, "Buy-in: " + EventId, World);
	play::OnPaidEntry(L.SitePlay, L, S, Fee, BuyIn, 0, EventId, World);
}

std::string IdAt(const char* Template, double Start)
{
	return std::string(Template) + "@" + std::to_string(static_cast<long long>(std::llround(Start)));
}

/** (Promo30 + Banked) / Rake30 in the ring's own units. */
double Ratio(const play::Book& B, net::SiteId S)
{
	const play::GovRing& G = B.Gov[static_cast<size_t>(detail::SitePlayGovIndex(S))];
	const int64_t Rake = detail::SitePlayRake30(G);
	return Rake > 0 ? static_cast<double>(detail::SitePlayPromo30(G) + G.Banked) / static_cast<double>(Rake) : 0.0;
}

std::string Lines(const ss::life::State& L)
{
	std::string Out;
	play::WriteLines(L, Out);
	return Out;
}

/** The book's lines read into a fresh life (the site book's done keys come along: they're the site\tdone lines). */
ss::life::State Reload(const ss::life::State& L)
{
	ss::life::State Back;
	Back.Sites = L.Sites;
	Back.Sponsors = L.Sponsors;
	const std::string Text = Lines(L);
	size_t From = 0;
	while (From < Text.size())
	{
		const size_t End = Text.find('\n', From);
		const std::string Line = Text.substr(From, End - From);
		From = End == std::string::npos ? Text.size() : End + 1;
		std::vector<std::string> P;
		size_t A = 0;
		while (true)
		{
			const size_t T = Line.find('\t', A);
			P.push_back(Line.substr(A, T == std::string::npos ? std::string::npos : T - A));
			if (T == std::string::npos)
			{
				break;
			}
			A = T + 1;
		}
		Back.Update3LinesRead |= ss::life::U3SitePlay;
		play::ReadLine(P, Back);
	}
	return Back;
}

// ------------------------------------------------------------------ the catalogs (rules 2 and 3), levels, the Wristband

void Catalogs()
{
	const std::vector<play::DareDef>& D = play::DareCatalog();
	int ByTier[3] = {0, 0, 0};
	std::map<int, int> Ids;
	for (const play::DareDef& X : D)
	{
		++ByTier[std::clamp<int>(X.Tier, 0, 2)];
		++Ids[X.Id];
		Expect(X.Needs == X.Tier + 1, std::string("dares: a tier's Dares need its paid entries (1, 2, 3): ") + X.Label);
	}
	Expect(D.size() == 24 && ByTier[0] == 8 && ByTier[1] == 8 && ByTier[2] == 8 && Ids.size() == 24, "dares: the P0 catalog is 24, 8 a tier, ids unique");
	// Rule 2 (F3): each Dare's Sparks at face <= half the least rake its entry floor needs at its band. The band's cheapest
	// paid entry: A a FUSE $1.10's fee (or a $1.10 MTT's), B a $2.21 average's, C the first stake above B ($7.70).
	const Chips MinRake[3] = {std::min(ss::fuse::FeeOf(0), FeeOn(net::SiteId::Ignite, 110)), FeeOn(net::SiteId::Ignite, 221), FeeOn(net::SiteId::Ignite, 770)};
	const int64_t Rewards[3][3] = {{60, 100, 200}, {120, 200, 400}, {200, 400, 800}};
	double Worst = 0.0;
	for (int T = 0; T < 3; ++T)
	{
		for (int Band = 0; Band < 3; ++Band)
		{
			const double Face = static_cast<double>(play::SparksValue(Rewards[T][Band])) / static_cast<double>(play::ValuePerCent);
			const double Ceiling = 0.5 * static_cast<double>((T + 1) * MinRake[Band]);
			Worst = std::max(Worst, Face / Ceiling);
			Expect(Face <= Ceiling, "dares: rule 2 (a Dare's face <= half the least rake its floor needs) at tier " + std::to_string(T) + " band " + std::to_string(Band));
		}
	}
	std::printf("  rule 2: the worst Dare pays %.1f%% of its ceiling (every row passes by %.1fx or more)\n", Worst * 100.0, 1.0 / Worst);
	// Rule 3 (F4): each ring's EV at face <= 10% of its threshold's rake at the band's lowest average rake.
	const int Threshold[3] = {1, 2, 4};
	for (int Band = 0; Band < 3; ++Band)
	{
		for (int R = 0; R < 3; ++R)
		{
			const std::vector<std::pair<int, double>>& T = play::WheelTable(R, static_cast<int8_t>(Band));
			double Sum = 0.0, Ev = 0.0;
			for (const auto& W : T)
			{
				Sum += W.second;
				Ev += W.second * (W.first > 0 ? static_cast<double>(play::SparksValue(W.first)) / static_cast<double>(play::ValuePerCent) : 0.0);
			}
			Expect(std::fabs(Sum - 1.0) < 1e-12, "wheel: a ring's table sums to 1, band " + std::to_string(Band) + " ring " + std::to_string(R));
			const double Cap = 0.10 * static_cast<double>(Threshold[R] * MinRake[Band]);
			Expect(Ev <= Cap, "wheel: rule 3 (a ring's Sparks EV <= 10% of its threshold's rake), band " + std::to_string(Band) + " ring " + std::to_string(R));
		}
	}
	// The full EV with the tickets at face, from the View's wedges (band A's: 45, 106, 251 Sparks a spin, model v11).
	{
		ss::life::State L = Fresh(Dec1 * DayMin + 600.0);
		const play::View V = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite, Dec1 * DayMin + 600.0);
		const double ExpectSparks[3] = {45.0, 106.0, 251.0};
		for (int R = 0; R < 3; ++R)
		{
			double Ev = 0.0;
			for (const play::WedgeView& W : V.Ig.Rings[static_cast<size_t>(R)])
			{
				Ev += W.P * (W.Sparks > 0 ? static_cast<double>(W.Sparks) : static_cast<double>(W.FaceCents) * 7000.0 / 110.0);
			}
			Expect(std::fabs(Ev - ExpectSparks[R]) < 1.0, "wheel: band A's ring EV is the model's " + std::to_string(static_cast<int>(ExpectSparks[R])) + " Sparks");
			const double Pct = Ev * 110.0 / 7000.0 / static_cast<double>(Threshold[R] * MinRake[0]) * 100.0;
			Expect(Pct <= 10.0, "wheel: rule 3 with the tickets at face, band A ring " + std::to_string(R) + " (" + ss::Fixed(Pct, 1) + "%)");
		}
	}
	// Ignite Levels (F6): 100 x (n - 1)^2.4.
	const std::pair<int, int64_t> Levels[] = {{2, 100}, {3, 527}, {5, 2785}, {10, 19506}, {15, 56325}, {20, 117221}, {30, 323411}, {40, 658498}};
	for (const auto& Lv : Levels)
	{
		Expect(play::LevelNeeds(Lv.first) == Lv.second && play::LevelOf(Lv.second) == Lv.first && play::LevelOf(Lv.second - 1) == Lv.first - 1,
			"levels: L" + std::to_string(Lv.first) + " needs " + std::to_string(Lv.second));
	}
	// The Wristband's tiers: 7,000 at T10, 35,000 at T20, 105,000 at T30 (the View's tier costs).
	ss::life::State L = Fresh(Dec1 * DayMin + 600.0);
	const play::View V = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite, Dec1 * DayMin + 600.0);
	Expect(V.Ig.Tiers[9].Needs == 7000 && V.Ig.Tiers[19].Needs == 35000 && V.Ig.Tiers[29].Needs == 105000 && V.Ig.Tiers[0].Needs == 700,
		"wristband: T1 700, T10 7,000, T20 35,000, T30 105,000");
	Expect(V.Ig.Tiers[9].Kind == 1 && V.Ig.Tiers[19].Kind == 1 && V.Ig.Tiers[29].Kind == 1 && V.Ig.Tiers[4].Kind == 2 && V.Ig.Tiers[7].Kind == 3 && V.Ig.Tiers[11].Kind == 4,
		"wristband: tickets at T10/20/30, a Cracker at T5, a Spare Match at T8, the month's card back at T12");
	// The Prize Counter: 7,000 a $1.10 ticket, its tickets' faces 110 / 7000 cents a Spark.
	for (const play::PrizeDef& P : play::PrizeCounter())
	{
		if (!P.TicketId.empty())
		{
			Expect(P.Sparks % 7000 == 0, std::string("prize counter: a ticket costs 7,000 Sparks a $1.10 of face: ") + P.Label);
		}
	}
}

// ------------------------------------------------------------------ LeagueScore (shared with the world)

void LeagueRule()
{
	const int Tue = Dec1; // Tuesday
	Expect(play::LeagueScore(2, "the-sweat", 1100, Tue, 100.0, false, false) == 150.0, "league: Steel's Matchday (The Sweat on a Tuesday) scores x1.5");
	Expect(play::LeagueScore(2, "runittwice.the-sweat@123456", 1100, Tue + 1, 100.0, false, false) == 100.0, "league: the same event on another night scores x1, any id form");
	Expect(play::LeagueScore(2, "double-shot", 550, Tue, 100.0, false, false) == 0.0, "league: below Steel's $11 floor scores nothing");
	Expect(play::LeagueScore(0, "double-shot", 550, Tue + 3, 100.0, false, false) == 150.0, "league: Open's Friday Double Shot is its Matchday");
	Expect(play::LeagueScore(4, "the-nightcap", 10900, Tue + 1, 80.0, false, true) == 80.0 * 1.25, "league: Deadline Day counts a featured event x1.25 (the Nightcap, a Wednesday)");
	Expect(play::LeagueScore(4, "headliner-pko", 8250, Tue + 1, 80.0, false, true) == 80.0, "league: Deadline Day leaves an unfeatured event at x1");
	Expect(play::LeagueScore(3, "encore-pko", 2200, Tue, 33.33, true, false) == std::round(33.33 * 1.5 * 1.25 * 10.0) / 10.0, "league: Matchday and series stack, rounded to a tenth");
	Expect(play::LeagueScore(1, "supper-stack", 550, Tue, 0.0, false, false) == 0.0, "league: a blank scores nothing");
	Expect(play::DivisionFloor(0) == 0 && play::DivisionFloor(1) == 550 && play::DivisionFloor(2) == 1100 && play::DivisionFloor(3) == 2200 && play::DivisionFloor(4) == 5500,
		"league: the floors Open any, Iron $5.50, Steel $11, Gold $22, High Bridge $55");
	Expect(play::PlacementDivision(549) == 0 && play::PlacementDivision(550) == 1 && play::PlacementDivision(1100) == 2 && play::PlacementDivision(2200) == 3 &&
			   play::PlacementDivision(5500) == 4,
		"league: placement by Abi100");
	Expect(play::LeagueDivOf(play::LeaguePack(3, 2)) == 3 && play::LeaguePaceOf(play::LeaguePack(3, 2)) == 2 && play::LeaguePaceOf(play::LeaguePack(4, 0)) == 3,
		"league: the world's book packs the pace with the division");
}

// ------------------------------------------------------------------ the Governor's mechanics

void Governor()
{
	const double T0 = Dec1 * DayMin + 6.0 * 60.0 + 600.0; // Tuesday 4 PM
	ss::life::State L = Fresh(T0);
	play::Book& B = L.SitePlay;
	Expect(Lines(L).empty() && !detail::SitePlayIgniteStarted(B), "gov: an untouched book writes no line, even with the bit set");
	const Chips Fee = FeeOn(net::SiteId::Ignite, 110);
	Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", T0 + 60.0), 110, Fee, T0);
	Expect(B.Ig.S.Balance == 10 * Fee && B.Ig.S.BaseLife == 10 * Fee && B.Ig.F.Streak == 1 && B.Ig.S.DayEntries == 1 && (L.Update3LinesRead & ss::life::U3SitePlay) != 0,
		"gov: a first $1.10 entry credits " + std::to_string(10 * Fee) + " base Sparks, lights the Flame and the day");
	Expect(play::IgniteLevel(B) == 2 && B.Ig.Boxes.size() == 1 && B.Ig.Boxes[0].Id == "lvl:2" && B.Ig.Boxes[0].Tier == 0,
		"gov: the first entry is a level-up and a Paper Cracker (F6)");
	Expect(play::RakeCents30(B, net::SiteId::Ignite) == Fee && play::PromoCents30(B, net::SiteId::Ignite) == play::SparksValue(10 * Fee) / play::ValuePerCent,
		"gov: the ring books the rake and the base Sparks' face");
	Expect(play::PaidEntry(L, IdAt("ignite.flip-go", T0 + 60.0)) && !play::PaidEntry(L, IdAt("ignite.flip-go", T0 + 120.0)), "gov: the paid entry's marker is there, a ticket's isn't");
	// A grant past the Room waits on the Guest List; a later one queues behind it (senior); rake covers them, oldest first.
	const int Day = play::PlayDay(T0);
	Expect(!play::Grant(B, L, net::SiteId::Ignite, play::Kind::Ticket, "Night Glow $2.20 ticket", 220, "ignite.night-glow|test:1", Day, T0), "gov: a $2.20 ticket past the Room is held");
	Expect(!play::Grant(B, L, net::SiteId::Ignite, play::Kind::DareSparks, "Dare", 1, "test:2", Day, T0) && B.Holds.size() == 2 && B.Holds[1].SparksOwed,
		"gov: a small Sparks grant queues behind it on the one \"Sparks owed\" line (the Guest List is senior)");
	Expect(!play::Grant(B, L, net::SiteId::Ignite, play::Kind::DareSparks, "Dare", 1, "test:3", Day, T0) && B.Holds.size() == 2, "gov: Sparks grants share the owed line");
	const play::View V0 = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, T0);
	Expect(V0.Gov.Holds.size() == 2 && V0.Gov.Holds[0].Cents == 220 && V0.Gov.Holds[0].RakeLeft > 0 && V0.Gov.Holds[0].Entries > 0 && V0.Gov.Holds[1].Sparks > 0,
		"gov: the view shows the hold, the rake still to pay and about how many Flip & Runs");
	const int64_t Tickets0 = L.TicketsFor("ignite.night-glow");
	double Now = T0;
	int Entries = 0;
	while (!B.Holds.empty() && Entries < 400)
	{
		Now += 5.0;
		Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", Now + 30.0), 110, Fee, Now);
		play::OnResult(B, L, play::SettleFacts{net::SiteId::Ignite, IdAt("ignite.flip-go", Now + 30.0), "flip-go", 30, 60, 0, 110}, Now);
		++Entries;
		Expect(Ratio(B, net::SiteId::Ignite) <= 0.5 + 1e-9, "gov: the ring stays inside 50% while holds clear");
	}
	Expect(B.Holds.empty() && L.TicketsFor("ignite.night-glow") == Tickets0 + 1, "gov: rake covers the holds and they clear, the ticket into Life.Tickets (" + std::to_string(Entries) + " entries)");
	const site::Account* Ig = site::AccountOf(L.Sites, net::SiteId::Ignite);
	bool Line = false;
	for (const site::Line& Ln : Ig->Lines)
	{
		Line = Line || (Ln.Cents == 0 && Ln.Label.compare(0, 9, "Rewards: ") == 0);
	}
	Expect(Line, "gov: a granted ticket writes the day's one aggregated 0-cent line");
	// The week's band cap (micro: $12 a week) banks base Sparks for Monday, never lost.
	ss::life::State H = Fresh(T0);
	play::Book& Hb = H.SitePlay;
	double At = T0;
	for (int K = 0; K < 200; ++K)
	{
		At += 2.0;
		Pay(H, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", At + 30.0), 550, FeeOn(net::SiteId::Ignite, 550), At);
	}
	const int64_t Deferred = Hb.Ig.S.Deferred;
	const int64_t Balance = Hb.Ig.S.Balance;
	Expect(Deferred > 0 && Hb.Gov[1].Banked > 0 && Hb.Gov[1].WeekFace <= 1200 * play::ValuePerCent, "gov: past the micro cap ($12 a week) base Sparks are banked for Monday");
	Expect(Hb.Ig.S.BaseLife == Balance + Deferred, "gov: banked Sparks still count for progress (the Level, the Wristband)");
	const play::View Vh = play::ViewOf(Hb, H, H.Sites, nullptr, net::SiteId::Ignite, At);
	Expect(Vh.Gov.MondayBank == Deferred && Vh.Ig.MondayBank == Deferred, "gov: the view's Monday bonus is the banked Sparks");
	const int Monday = (play::PlayDay(At) / 7 + 1) * 7;
	play::Step(Hb, H, H.Sites, nullptr, H.Sponsors, 7, At, play::PlayDayStart(Monday) + 1.0);
	Expect(Hb.Ig.S.Deferred == 0 && Hb.Gov[1].Banked == 0 && Hb.Ig.S.Balance >= Balance + Deferred, "gov: Monday 6 AM credits the bank, outside the new week's cap");
	// The welcome window's ceiling is 75%.
	ss::life::State Wl = Fresh(T0);
	site::AccountOf(Wl.Sites, net::SiteId::Ignite)->BonusExpires = T0 + 60.0 * DayMin;
	const play::View Vw = play::ViewOf(Wl.SitePlay, Wl, Wl.Sites, nullptr, net::SiteId::Ignite, T0);
	Expect(Vw.Gov.Welcome && Vw.Gov.Ceiling == 0.75, "gov: inside Ignite's 60-day welcome window the ceiling is 75%");
	// Idempotence: a step twice, overlapping windows, a skip's catch-up: the same book.
	const std::string Before = Lines(L);
	const double From = Now;
	play::Step(B, L, L.Sites, nullptr, L.Sponsors, 7, From, From + 3.0 * DayMin);
	const std::string Once = Lines(L);
	play::Step(B, L, L.Sites, nullptr, L.Sponsors, 7, From + DayMin, From + 3.0 * DayMin);
	play::Step(B, L, L.Sites, nullptr, L.Sponsors, 7, From, From + 3.0 * DayMin);
	Expect(Lines(L) == Once && Once != Before, "gov: overlapping steps (the rent loop's double window) and a repeat pay nothing twice");
	// The save round-trips byte for byte.
	const ss::life::State Back = Reload(L);
	Expect(Lines(Back) == Lines(L) && !Lines(L).empty(), "save: the book's lines read back and write out byte for byte");
	// The Guest List never passes 15 lines; a full claim list makes a claimed ticket wait in place.
	ss::life::State F = Fresh(T0);
	play::Book& Fb = F.SitePlay;
	Pay(F, net::SiteId::Ignite, IdAt("ignite.flip-go", T0 + 60.0), 110, Fee, T0);
	for (int K = 0; K < 20 && detail::SitePlayHasItemRoom(Fb); ++K)
	{
		// As the claims do: a claimed ticket queues only while a claim line is free.
		play::Grant(Fb, F, net::SiteId::Ignite, play::Kind::Ticket, "Flip & Run $1.10 ticket", 110, "ignite.flip-go|fill:" + std::to_string(K), Day, T0);
	}
	Expect(!detail::SitePlayHasItemRoom(Fb), "gov: with 11 claim lines and the owed line's slot, a claimed ticket waits in place");
	// A full list never drops a Dare or a tier: done Dares credit at 6 AM onto the owed line; an unclaimed Wristband tier on
	// the 1st takes a reset slot; the list stays inside 15.
	const int64_t OwedBefore = [&] {
		for (const play::Hold& Hd : Fb.Holds)
		{
			if (Hd.SparksOwed)
			{
				return detail::SitePlayInt(Hd.Key.substr(4));
			}
		}
		return int64_t(0);
	}();
	Fb.Ig.D.Prog = {20, 20, 20};
	Fb.Ig.S.DayEntries = 3;
	Fb.Ig.S.BaseMonth = 7000; // T10 reached
	const int Next1st = ss::world::MonthStart(ss::world::MonthStart(Day) + 32);
	play::Step(Fb, F, F.Sites, nullptr, F.Sponsors, 7, T0, play::PlayDayStart(Next1st) + 1.0);
	int64_t OwedAfter = 0;
	int Resets = 0;
	for (const play::Hold& Hd : Fb.Holds)
	{
		OwedAfter = Hd.SparksOwed ? detail::SitePlayInt(Hd.Key.substr(4)) : OwedAfter;
		Resets += Hd.Reset ? 1 : 0;
	}
	Expect((Fb.Ig.D.DayKey != Day) && OwedAfter >= OwedBefore + 60 + 120 + 200, "gov: the day's done Dares credit at 6 AM onto the owed line with the list full");
	Expect(Resets >= 1 && Fb.Holds.size() <= 15, "gov: the 1st's unclaimed T10 ticket takes a reset slot; the list stays inside 15 (" + std::to_string(Fb.Holds.size()) + ")");
	// The view's cost (§10: SitePlayView <= 0.2 ms), with a world for the Headliners and the League's names.
	ss::world::World W;
	W.Create(77u, T0);
	const auto C0 = std::chrono::steady_clock::now();
	for (int K = 0; K < 50; ++K)
	{
		const play::View Vi = play::ViewOf(B, L, L.Sites, &W, net::SiteId::Ignite, Now);
		const play::View Vr = play::ViewOf(B, L, L.Sites, &W, net::SiteId::RunItTwice, Now);
		(void)Vi;
		(void)Vr;
	}
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - C0).count() / 100.0;
	std::printf("  view: SitePlayView %.3f ms a site (budget 0.2 ms)\n", Ms);
	Expect(Ms <= 0.2 * 3.0, "view: SitePlayView inside its budget (0.2 ms; x3 for a loaded test machine)");
}

// ------------------------------------------------------------------ the loops one at a time

/** The world day the month after Day's starts (its 1st). */
int world_first_of_next(int Day)
{
	return ss::world::MonthStart(ss::world::MonthStart(Day) + 32);
}

void Loops()
{
	const double Mon = (Dec1 + 6) * DayMin + 360.0; // Monday, December 7, 6 AM
	ss::life::State L = Fresh(Mon);
	play::Book& B = L.SitePlay;
	const uint64_t Seed = 4242;
	const Chips Fee = FeeOn(net::SiteId::Ignite, 110);
	// The wheel: nothing lit, then GLOW; the spin lands on the key's wedge; a second spin is refused; a reload replays it.
	Expect(!play::Spin(B, L, Seed, Mon + 60.0).empty(), "wheel: no paid entry today, no spin");
	Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", Mon + 120.0), 110, Fee, Mon + 61.0);
	const play::View Lit = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 62.0);
	Expect(Lit.Ig.RingsLit == 1 && Lit.Ig.SpinReady && Lit.Ig.NearGoal == "1 more entry lights SHINE.", "wheel: one paid entry lights GLOW, \"1 more entry lights SHINE.\"");
	Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", Mon + 180.0), 110, Fee, Mon + 121.0);
	const play::View Shine = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 122.0);
	Expect(Shine.Ig.RingsLit == 2 && Shine.Ig.MoreForNext == 2, "wheel: a second lights SHINE, 2 more for DAZZLE");
	const int64_t Sparks0 = B.Ig.S.Balance;
	Expect(play::Spin(B, L, Seed, Mon + 130.0).empty() && B.Ig.S.WheelDay == play::PlayDay(Mon) && B.Ig.S.WheelRing == 2, "wheel: SPIN takes today's SHINE");
	Expect(!play::Spin(B, L, Seed, Mon + 140.0).empty(), "wheel: a second spin in a day is refused in core");
	const play::View Spun = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 141.0);
	const play::WedgeView& W = Spun.Ig.Rings[1][static_cast<size_t>(std::max(0, Spun.Ig.SpunWedge))];
	Expect(Spun.Ig.Spun && Spun.Ig.SpunRing == 2 && Spun.Ig.SpunWedge >= 0 && (W.Sparks == 0 || B.Ig.S.Balance == Sparks0 + W.Sparks || !B.Holds.empty()),
		"wheel: the view's wedge is the one the press paid");
	const ss::life::State Back = Reload(L);
	const play::View Again = play::ViewOf(Back.SitePlay, Back, Back.Sites, nullptr, net::SiteId::Ignite, Mon + 141.0);
	Expect(Again.Ig.SpunWedge == Spun.Ig.SpunWedge && Again.Ig.Spun, "wheel: a reload replays the same wedge");
	// Crackers: contents from the key, the same before and after a reload; OPEN pays exactly that.
	Expect(!B.Ig.Boxes.empty(), "crackers: the level-up's Paper Cracker is on the shelf");
	const play::OpenView Peek = play::CrackerContents(Seed, B.Ig.Boxes.front());
	const play::OpenView PeekBack = play::CrackerContents(Seed, Back.SitePlay.Ig.Boxes.front());
	Expect(Peek.Sparks == PeekBack.Sparks && Peek.Prize == PeekBack.Prize && Peek.Sparks >= 100, "crackers: the contents come from the key (a reload never rerolls)");
	const int64_t Before = B.Ig.S.Balance;
	const size_t Holds = B.Holds.size();
	Expect(play::Open(B, L, Seed, B.Ig.Boxes.front().Id, Mon + 150.0).empty() && (B.Ig.S.Balance == Before + Peek.Sparks || B.Holds.size() > Holds),
		"crackers: OPEN pays the drawn Sparks (or queues them)");
	// The shelf holds 12: a 13th opens itself.
	for (int K = 0; K < 14; ++K)
	{
		detail::IgniteDrop(B, L, Seed, 0, "test:" + std::to_string(K), play::PlayDay(Mon), Mon + 160.0);
	}
	Expect(B.Ig.Boxes.size() == 12, "crackers: the shelf holds 12, a 13th opens itself into Sparks");
	// Dares: today's three, refused before done, claimed once; the clear-all Cracker.
	const play::View Dv = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 170.0);
	int Claimed = 0;
	for (int K = 0; K < 3; ++K)
	{
		const play::DareView& D = Dv.Ig.Dares[static_cast<size_t>(K)];
		Expect(D.Id > 0 && !D.Label.empty() && D.Sparks > 0, "dares: today's card has three Dares with rewards");
		const std::string Why = play::ClaimDare(B, L, K, Mon + 171.0);
		Expect(D.Done == Why.empty(), "dares: a claim goes through exactly when the Dare is done: " + D.Label);
		Claimed += Why.empty() ? 1 : 0;
		if (Why.empty())
		{
			Expect(!play::ClaimDare(B, L, K, Mon + 172.0).empty(), "dares: a Dare is claimed once");
		}
	}
	std::printf("  dares: today's card %d / %d / %d, %d claimable after two Flip & Runs\n", Dv.Ig.Dares[0].Id, Dv.Ig.Dares[1].Id, Dv.Ig.Dares[2].Id, Claimed);
	// The Flame (F2): two rest days a week free, then a Spare Match burns, then the streak goes.
	ss::life::State Fl = Fresh(Mon);
	play::Book& Fb = Fl.SitePlay;
	for (int Dd = 0; Dd < 5; ++Dd)
	{
		Pay(Fl, net::SiteId::Ignite, IdAt("ignite.flip-go", Mon + Dd * DayMin + 700.0), 110, Fee, Mon + Dd * DayMin + 650.0);
	}
	Expect(Fb.Ig.F.Streak == 5 && Fb.Ig.F.Matches == 1, "flame: five flame days in a row, and the 5-day run's Spare Match");
	const play::View Fv = play::ViewOf(Fb, Fl, Fl.Sites, nullptr, net::SiteId::Ignite, Mon + 4 * DayMin + 700.0);
	Expect(std::fabs(Fv.Ig.Mult - 1.20) < 1e-9 && Fv.Ig.LitToday && Fv.Ig.Week[4] == 1 && Fv.Ig.Week[0] == 1, "flame: x1.20 from day 5; the week's row reads lit");
	// Saturday, Sunday: the two rest days; next Monday's week, a fresh two.
	play::Step(Fb, Fl, Fl.Sites, nullptr, Fl.Sponsors, Seed, Mon + 4 * DayMin + 700.0, Mon + 7 * DayMin + 1.0);
	Expect(Fb.Ig.F.Streak == 5 && Fb.Ig.F.Matches == 1, "flame: two missed days in a week are free rest days");
	play::Step(Fb, Fl, Fl.Sites, nullptr, Fl.Sponsors, Seed, Mon + 7 * DayMin + 1.0, Mon + 10 * DayMin + 1.0);
	Expect(Fb.Ig.F.Streak == 5 && Fb.Ig.F.Matches == 0, "flame: the third miss in a week burns the Spare Match, and the flame survives");
	play::Step(Fb, Fl, Fl.Sites, nullptr, Fl.Sponsors, Seed, Mon + 10 * DayMin + 1.0, Mon + 11 * DayMin + 1.0);
	Expect(Fb.Ig.F.Streak == 0, "flame: the fourth miss with no match left puts it out");
	// On ice under the clause (W7): misses don't count.
	ss::life::State Ice = Fresh(Mon);
	Pay(Ice, net::SiteId::Ignite, IdAt("ignite.flip-go", Mon + 700.0), 110, Fee, Mon + 650.0);
	ss::sponsor::Contract C;
	C.Id = "riverline@1";
	C.Brand = "riverline";
	C.St = ss::sponsor::State::Active;
	C.T.Only = "riverline";
	C.Since = Mon;
	C.Until = Mon + 365.0 * DayMin;
	Ice.Sponsors.Contracts.push_back(C);
	play::Step(Ice.SitePlay, Ice, Ice.Sites, nullptr, Ice.Sponsors, Seed, Mon + 700.0, Mon + 20 * DayMin + 1.0);
	const play::View Iv = play::ViewOf(Ice.SitePlay, Ice, Ice.Sites, nullptr, net::SiteId::Ignite, Mon + 20 * DayMin + 2.0);
	Expect(Ice.SitePlay.Ig.F.Streak == 1 && Ice.SitePlay.Ig.F.IceSince >= 0 && Iv.Ig.OnIce, "flame: on ice under Team RiverLine's clause, 20 missed days and the flame waits");
	// While the book is empty, on ice is derived at read time and nothing is written (S10).
	ss::life::State Empty = Fresh(Mon);
	Empty.Sponsors.Contracts.push_back(C);
	const play::View Ev = play::ViewOf(Empty.SitePlay, Empty, Empty.Sites, nullptr, net::SiteId::Ignite, Mon + 10.0);
	const play::View Er = play::ViewOf(Empty.SitePlay, Empty, Empty.Sites, nullptr, net::SiteId::RunItTwice, Mon + 10.0);
	play::Step(Empty.SitePlay, Empty, Empty.Sites, nullptr, Empty.Sponsors, Seed, Mon, Mon + 40 * DayMin);
	Expect(Ev.Ig.OnIce && Er.Rt.OnLeave && !Ev.Ig.Started && Lines(Empty).empty() && Empty.Sites.Done.empty(),
		"clause: on ice and on leave derived while the book is empty; a 40-day step writes nothing");
	// The Wristband: a $5.50 regular's month reaches its tiers; claimed tiers pay once; the 1st credits the rest.
	ss::life::State Wb = Fresh(Dec1 * DayMin + 400.0);
	double At = Dec1 * DayMin + 400.0;
	for (int K = 0; K < 70; ++K)
	{
		At += 30.0;
		Pay(Wb, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", At + 30.0), 550, FeeOn(net::SiteId::Ignite, 550), At);
	}
	const play::View Wv = play::ViewOf(Wb.SitePlay, Wb, Wb.Sites, nullptr, net::SiteId::Ignite, At);
	Expect(Wv.Ig.Tier >= 10 && Wv.Ig.Tiers[9].Reached, "wristband: 70 entries at $5.50 reach T10 (tier " + std::to_string(Wv.Ig.Tier) + ")");
	const int64_t Matches = Wb.SitePlay.Ig.F.Matches;
	Expect(play::ClaimTier(Wb.SitePlay, Wb, 8, At).empty() && Wb.SitePlay.Ig.F.Matches == std::min<int64_t>(2, Matches + 1) && !play::ClaimTier(Wb.SitePlay, Wb, 8, At).empty(),
		"wristband: T8's Spare Match claims once");
	Expect(!play::ClaimTier(Wb.SitePlay, Wb, 30, At).empty(), "wristband: an unreached tier can't be claimed");
	const int TicketsT10 = Wb.TicketsFor("ignite.flip-go");
	play::Step(Wb.SitePlay, Wb, Wb.Sites, nullptr, Wb.Sponsors, Seed, At, play::PlayDayStart(world_first_of_next(Dec1)) + 1.0);
	bool Queued = false;
	for (const play::Hold& Hd : Wb.SitePlay.Holds)
	{
		Queued = Queued || Hd.Key.find("band:") != std::string::npos;
	}
	Expect(Wb.TicketsFor("ignite.flip-go") == TicketsT10 + 1 || Queued, "wristband: the unclaimed T10 ticket credits on the 1st (paid or on the Guest List)");
	Expect(Wb.SitePlay.Ig.BandClaimed == 0 && Wb.SitePlay.Ig.S.BaseMonth == 0, "wristband: the new month's band starts over");
}

// ------------------------------------------------------------------ the caps, W4, the League against the real world

size_t RitBookBytes = 0; // LeagueWorld's book, for the save budget (IgnitePatterns)
size_t BeefFullBytes = 0; // G2Fixes' full Beef window (24 names), for the same budget (G2 fix M1)

void Caps()
{
	const double T0 = (Dec1 + 6) * DayMin + 360.0 + 600.0; // Monday 4 PM
	ss::life::State L = Fresh(T0);
	play::Book& B = L.SitePlay;
	const Chips Fee = FeeOn(net::SiteId::Ignite, 110);
	const std::string Id = IdAt("ignite.flip-go", T0 + 30.0);
	Pay(L, net::SiteId::Ignite, Id, 110, Fee, T0);
	// The Ouch Bonus: an all-in lost at 80% or better, 3 a day at most.
	int Ouches = 0;
	for (int K = 0; K < 5; ++K)
	{
		play::HandNote N;
		N.Paid = true;
		N.EventId = Id;
		N.HandId = Id + "#ouch" + std::to_string(K);
		N.AllIn = true;
		N.Lost = true;
		N.Equity = 0.87;
		play::OnHandPaid(B, L, net::SiteId::Ignite, N, T0 + 40.0 + K);
		Ouches = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, T0 + 50.0).Ig.OuchToday;
	}
	Expect(Ouches == 3, "caps: the Ouch Bonus pays 3 a day at most (" + std::to_string(Ouches) + ")");
	// A 79% loss isn't a bad beat.
	{
		play::HandNote N;
		N.Paid = true;
		N.EventId = Id;
		N.HandId = Id + "#79";
		N.AllIn = true;
		N.Lost = true;
		N.Equity = 0.79;
		ss::life::State C = Fresh(T0);
		Pay(C, net::SiteId::Ignite, Id, 110, Fee, T0);
		play::OnHandPaid(C.SitePlay, C, net::SiteId::Ignite, N, T0 + 40.0);
		Expect(C.SitePlay.Ig.S.OuchCount % 256 == 0, "caps: an all-in lost at 79% pays no Ouch");
	}
	// Beef and Payback: two busts by one name make the Beef; knocking it out pays once a day.
	for (int K = 0; K < 2; ++K)
	{
		play::HandNote N;
		N.Paid = true;
		N.EventId = Id;
		N.HandId = Id + "#bust" + std::to_string(K);
		N.BustedBy = "maxk0";
		play::OnHandPaid(B, L, net::SiteId::Ignite, N, T0 + 60.0 + K);
	}
	Expect(play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, T0 + 70.0).Ig.BeefName == "maxk0", "beef: two busts by maxk0 make him the Beef");
	const int64_t Before = B.Ig.S.Balance;
	for (int K = 0; K < 2; ++K)
	{
		play::HandNote N;
		N.Paid = true;
		N.EventId = Id;
		N.HandId = Id + "#payback" + std::to_string(K);
		N.Knocked = {"maxk0"};
		N.KnockedNpcs = {-1};
		play::OnHandPaid(B, L, net::SiteId::Ignite, N, T0 + 80.0 + K);
	}
	const play::View Pv = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, T0 + 90.0);
	int Paybacks = 0;
	for (const play::MomentView& M : Pv.Ig.Moments)
	{
		Paybacks += M.Kind == 0 ? 1 : 0;
	}
	Expect(Paybacks == 1 && (B.Ig.S.Balance >= Before + 500 || !B.Holds.empty()), "beef: Payback pays +500 Sparks once a day");
	// Headliner scalps: 3 a week (Monday here: no Bounty Night).
	int Scalps = 0;
	for (int K = 0; K < 5; ++K)
	{
		play::HandNote N;
		N.Paid = true;
		N.EventId = Id;
		N.HandId = Id + "#scalp" + std::to_string(K);
		N.Knocked = {"headliner" + std::to_string(K)};
		N.KnockedNpcs = {-1};
		N.Headliners = N.Knocked;
		play::OnHandPaid(B, L, net::SiteId::Ignite, N, T0 + 100.0 + K);
	}
	Scalps = B.Ig.St.ScalpCount & 15;
	Expect(Scalps == 3 && B.Ig.St.ScalpLife == 3, "caps: Headliner scalps pay 3 a week");
	// W4: a result and a hand with no paid entry behind them move nothing.
	ss::life::State U = Fresh(T0);
	Pay(U, net::SiteId::Ignite, Id, 110, Fee, T0);
	const std::string Was = Lines(U);
	play::SettleFacts F;
	F.Site = net::SiteId::Ignite;
	F.EventId = IdAt("ignite.flip-go", T0 + 90.0); // a ticket's entry: no paid marker
	F.Place = 1;
	F.Entrants = 60;
	F.Prize = 2000;
	F.BuyIn = 110;
	play::OnResult(U.SitePlay, U, F, T0 + 200.0);
	play::HandNote N;
	N.Paid = false;
	N.Won = true;
	N.Showdown = true;
	play::OnHandPaid(U.SitePlay, U, net::SiteId::Ignite, N, T0 + 210.0);
	Expect(Lines(U) == Was, "W4: a ticket's or freeroll's result and hands add nothing to any loop");
	// A paid result counts once: its marker goes with it.
	F.EventId = Id;
	play::OnResult(U.SitePlay, U, F, T0 + 220.0);
	const std::string Once = Lines(U);
	play::OnResult(U.SitePlay, U, F, T0 + 221.0);
	Expect(Lines(U) == Once && !play::PaidEntry(U, Id), "W4: a paid result counts once (its marker is taken)");
}

void LeagueWorld()
{
	// The League against the real world (core-world's recording through LeagueScore): the first paid RunItTwice entry asks for
	// a group; the step draws it from the world's members; the world records their results; the page's scores are the world's;
	// the 1st closes the month by the table.
	const double T0 = (Dec1 + 2) * DayMin + 19.0 * 60.0; // Thursday, December 3, 7 PM
	ss::world::World W;
	W.Create(4242u, T0 - 40.0 * DayMin);
	W.AdvanceTo(T0);
	ss::life::State L = Fresh(T0);
	play::Book& B = L.SitePlay;
	const std::string Id = IdAt("runittwice.the-sweat", T0 + 30.0);
	Pay(L, net::SiteId::RunItTwice, Id, 1100, FeeOn(net::SiteId::RunItTwice, 1100), T0);
	Expect(B.Rt.L.Month == -2 && B.Rt.L.Placement, "league: the first paid RunItTwice entry asks for a Placement group");
	B.Rt.L.Div = static_cast<int8_t>(play::PlacementDivision(1100));
	play::StepOut Out = play::Step(B, L, L.Sites, &W, L.Sponsors, 99, T0, T0 + 5.0);
	Expect(Out.Roster && Out.MonthKey == play::PlayMonth(play::PlayDay(T0)) && Out.Npcs.size() >= 10 && play::LeagueDivOf(Out.Div) == 2,
		"league: the step draws Steel's group from the world's members (" + std::to_string(Out.Npcs.size()) + " members, pace " + std::to_string(play::LeaguePaceOf(Out.Div)) + ")");
	W.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
	// The draw reproduces.
	{
		ss::life::State L2 = Fresh(T0);
		Pay(L2, net::SiteId::RunItTwice, Id, 1100, FeeOn(net::SiteId::RunItTwice, 1100), T0);
		L2.SitePlay.Rt.L.Div = 2;
		const play::StepOut Again = play::Step(L2.SitePlay, L2, L2.Sites, &W, L2.Sponsors, 99, T0, T0 + 5.0);
		Expect(Again.Npcs == Out.Npcs, "league: the draw reproduces from its key");
	}
	// The player's result scores through the same rule.
	play::SettleFacts F;
	F.Site = net::SiteId::RunItTwice;
	F.EventId = Id;
	F.Place = 5;
	F.Entrants = 94;
	F.Prize = 4000;
	F.BuyIn = 1100;
	play::OnResult(B, L, F, T0 + 200.0);
	const double Mine = play::LeagueScore(2, "the-sweat", 1100, play::PlayDay(T0 + 200.0), net::Points(5, 94, 1100), false, false);
	const play::View V0 = play::ViewOf(B, L, L.Sites, &W, net::SiteId::RunItTwice, T0 + 210.0);
	Expect(V0.Rt.League && std::fabs(V0.Rt.Points - Mine) < 0.051 && V0.Rt.Of == static_cast<int>(Out.Npcs.size()) + 1 && V0.Rt.Table.size() == Out.Npcs.size() + 1,
		"league: the player's day scores through LeagueScore (" + ss::Fixed(Mine, 1) + " pts) and the table holds the group");
	// The world plays ten days; the page's member scores are the world's own recorded bests.
	W.AdvanceTo(T0 + 10.0 * DayMin);
	const play::View V = play::ViewOf(B, L, L.Sites, &W, net::SiteId::RunItTwice, T0 + 10.0 * DayMin);
	const ss::world::LeagueBook* Lb = W.League(net::SiteId::RunItTwice);
	bool Same = Lb != nullptr;
	int Scoring = 0;
	for (const play::LeagueRowView& R : V.Rt.Table)
	{
		if (R.Hero || !Lb)
		{
			continue;
		}
		double Theirs = 0.0;
		for (const ss::world::LeagueMember& M : Lb->Members)
		{
			if (M.Npc == R.Npc)
			{
				for (const auto& P : M.Best)
				{
					Theirs += P.second / 10.0;
				}
			}
		}
		Same = Same && std::fabs(Theirs - R.Points) < 1e-9;
		Scoring += R.Points > 0.0 ? 1 : 0;
	}
	std::printf("  league: after ten world days %d of %zu members have scored; the player is %d of %d\n", Scoring, V.Rt.Table.size() - 1, V.Rt.Rank, V.Rt.Of);
	Expect(Same, "league: every member's score on the page is the world's recorded best days (W5)");
	// The 1st closes the month by the table: a history row, and a new group for January.
	const int First = ss::world::MonthStart(ss::world::MonthStart(play::PlayDay(T0)) + 32);
	W.AdvanceTo(play::PlayDayStart(First) - 60.0);
	Out = play::Step(B, L, L.Sites, &W, L.Sponsors, 99, T0 + 10.0 * DayMin, play::PlayDayStart(First) + 1.0);
	Expect(B.Rt.Hist.size() == 1 && B.Rt.Hist[0].Of == static_cast<int>(V.Rt.Table.size()) && B.Rt.L.Month == play::PlayMonth(First) && Out.Roster && !B.Rt.L.Placement,
		"league: the 1st closes the month (" + std::to_string(B.Rt.Hist.empty() ? 0 : B.Rt.Hist[0].Rank) + " of " + std::to_string(B.Rt.Hist.empty() ? 0 : B.Rt.Hist[0].Of) +
			", move " + std::to_string(B.Rt.Hist.empty() ? 0 : B.Rt.Hist[0].Move) + ") and draws the next group");
	Expect(B.Rt.Hist.empty() || B.Rt.Hist[0].Move >= 0, "league: a Placement month never relegates");
	// The save round-trips with the League, its history and its world.
	RitBookBytes = Lines(L).size();
	const ss::life::State Back = Reload(L);
	Expect(Lines(Back) == Lines(L) && Lines(L).find("site\tleague\trunittwice\t") != std::string::npos && Lines(L).find("site\tlhist\trunittwice\t") != std::string::npos,
		"league: the League's lines read back byte for byte");
}

// ------------------------------------------------------------------ the C++ model harness (SPEC §2.7)

struct PatternDef
{
	const char* Name;
	int Days;                       // weekday bits, Monday bit 0
	int Kind;                       // 0 the plan below, 1 night owl (1-2 x $1.10), 2 typical (by weekday)
	std::vector<std::pair<bool, Chips>> Plan; // (FUSE, buy-in)
};

std::vector<std::pair<bool, Chips>> Repeat(bool Fuse, Chips BuyIn, int N)
{
	return std::vector<std::pair<bool, Chips>>(static_cast<size_t>(N), {Fuse, BuyIn});
}

const char* TemplateFor(Chips BuyIn)
{
	return BuyIn <= 110 ? "ignite.flip-go" : BuyIn <= 330 ? "ignite.plum-turbo" : BuyIn <= 550 ? "ignite.spark-jackpot" : "ignite.big-wave";
}

int TierOfBuyIn(Chips BuyIn)
{
	return BuyIn <= 110 ? 0 : BuyIn <= 330 ? 1 : 2;
}

struct PatternRun
{
	double Share1 = 0, Share3 = 0, RingMax = 0, GrantMax = 0;
	std::vector<int> Waits;
	int OpenOld = 0; // holds made by day 60 still waiting at the end
	int Level = 0, Tier = 0;
	Chips Rake30 = 0, Promo30 = 0;
	int MaxHolds = 0;
	std::map<std::string, double> By; // month 1's value by source, cents
	size_t SaveBytes = 0;             // the book's lines at the end
};

/** A ticket's face by its id (the loops' tickets). */
Chips TicketFace(const std::string& Id)
{
	static const std::map<std::string, Chips> Faces = {{"ignite.flip-go", 110}, {"ignite.night-glow", 220}, {"ignite.plum-turbo", 330}, {"ignite.spark-jackpot", 550},
		{"ignite.big-wave", 1100}, {"ignite.sunday-blaze", 1100}, {"ignite.sky-high", 2200}, {"ignite.mystery-pinata", 770}, {"runittwice.supper-stack", 550},
		{"runittwice.the-sweat", 1100}, {"runittwice.sunday-grind", 2200}, {"runittwice.the-fifty-five", 5500}, {"runittwice.sunday-100k", 10900},
		{"runittwice.afternoon-deepstack", 2750}};
	const auto It = Faces.find(Id);
	if (It != Faces.end())
	{
		return It->second;
	}
	const int Tier = ss::fuse::TierOfTicket(Id);
	return Tier >= 0 ? ss::fuse::BuyInOf(Tier) : 0;
}

/** Everything the loops have handed the player so far, at face, in cents: Sparks credited (spent or not) and tickets. */
double ValueReceived(const ss::life::State& L)
{
	double Cents = static_cast<double>(play::SparksValue(L.SitePlay.Ig.S.Balance + L.SitePlay.Ig.S.SpentLife)) / static_cast<double>(play::ValuePerCent);
	for (const auto& T : L.Tickets)
	{
		Cents += static_cast<double>(T.second * TicketFace(T.first));
	}
	return Cents;
}

/** One seed of a pattern over Days days through the real hooks and claims. */
PatternRun RunPattern(const PatternDef& P, uint64_t Seed, int Days)
{
	PatternRun Out;
	const double Start = play::PlayDayStart(Dec1);
	ss::life::State L = Fresh(Start);
	play::Book& B = L.SitePlay;
	ss::Rng R("siteplay-pattern:" + std::string(P.Name) + ":" + std::to_string(Seed));
	std::array<int, 3> FuseN{};
	std::map<std::string, int> Made;
	// The people at the player's tables: a regular pool the size of a micro site's evening (busts and knockouts drawn from it).
	std::vector<std::string> Regs;
	for (int K = 0; K < 60; ++K)
	{
		Regs.push_back("reg" + std::to_string(K));
	}
	double Clock = Start;
	auto Track = [&](int DayIx) {
		std::map<std::string, int> Now;
		for (const play::Hold& H : B.Holds)
		{
			const std::string Id = H.SparksOwed ? std::string("owed") : H.Key;
			Now[Id] = Made.count(Id) ? Made[Id] : DayIx;
		}
		for (const auto& M : Made)
		{
			if (!Now.count(M.first))
			{
				Out.Waits.push_back(DayIx - M.second);
			}
		}
		Made = Now;
		Out.MaxHolds = std::max(Out.MaxHolds, static_cast<int>(B.Holds.size()));
	};
	// A grant-capable call keeps the ring at or under the ceiling at its instant (or, when the ring drifted above it as old
	// rake left the window, adds nothing).
	// The share is what the player received (Sparks at face, tickets at face) over the rake paid in the same days (the
	// model's accounting): the ring spreads a grant over the rake that earned it, so its sum isn't a period's value.
	double RakeCum = 0.0, Value60 = 0.0, Rake60 = 0.0;
	// In month 1 the ring drops nothing (it starts empty), so its promo's growth by call is the value by source.
	int64_t LastPromo = 0;
	int DayNow = 0;
	auto Guard = [&](double Before, const char* What = "") {
		const double After = Ratio(B, net::SiteId::Ignite);
		Out.GrantMax = std::max(Out.GrantMax, After > Before + 1e-9 ? After : 0.0);
		const int64_t Promo = detail::SitePlayPromo30(B.Gov[1]);
		if (DayNow < 30 && *What)
		{
			Out.By[What] += static_cast<double>(Promo - LastPromo) / static_cast<double>(play::ValuePerCent);
		}
		LastPromo = Promo;
	};
	for (int D = 0; D < Days; ++D)
	{
		const int Day = Dec1 + D;
		const double DayStart = play::PlayDayStart(Day);
		{
			ss::life::State Probe = L; // the 6 AM roll alone (old rake leaving can lift the ratio with no grant)
			detail::SitePlayRoll(Probe.SitePlay, net::SiteId::Ignite, Day);
			const double Rolled = Ratio(Probe.SitePlay, net::SiteId::Ignite);
			DayNow = D;
			play::Step(B, L, L.Sites, nullptr, L.Sponsors, Seed, Clock, DayStart + 1.0);
			Guard(Rolled, "6 AM (Dares, the 1st, Monday, Guest List)");
			Clock = DayStart + 1.0;
		}
		Track(D);
		const int Wd = ss::net::Weekday(Day);
		const bool Off = P.Kind == 3 && (D / 10) % 2 == 1; // stop-start: ten days on, ten off
		if ((P.Days & (1 << Wd)) != 0 && !Off)
		{
			std::vector<std::pair<bool, Chips>> Today = P.Plan;
			if (P.Kind == 1)
			{
				Today = Repeat(false, 110, R.Next() < 0.5 ? 1 : 2);
			}
			else if (P.Kind == 2)
			{
				Today = Wd == 0 ? std::vector<std::pair<bool, Chips>>{{false, 110}, {false, 330}} : Wd == 2 ? std::vector<std::pair<bool, Chips>>{{false, 330}, {false, 550}}
					: std::vector<std::pair<bool, Chips>>{{false, 550}};
			}
			const double Spacing = std::min(25.0, 600.0 / static_cast<double>(Today.size()));
			for (size_t K = 0; K < Today.size(); ++K)
			{
				const bool Fuse = Today[K].first;
				const Chips BuyIn = Today[K].second;
				const double At = DayStart + 12.0 * 60.0 + Spacing * static_cast<double>(K);
				std::string Id;
				Chips Fee = 0;
				int M = 0;
				const int Tier = TierOfBuyIn(BuyIn);
				if (Fuse)
				{
					const int N = FuseN[static_cast<size_t>(Tier)]++;
					M = ss::fuse::Draw(Seed, Tier, N);
					Id = ss::fuse::IdFor(Tier, At, N);
					Fee = ss::fuse::FeeOf(Tier);
					B.Ig.Fuse.N[static_cast<size_t>(Tier)] = FuseN[static_cast<size_t>(Tier)];
					ss::fuse::NoteEntry(B.Ig.Fuse, Tier, M, true);
				}
				else
				{
					Id = IdAt(TemplateFor(BuyIn), At + 5.0);
					Fee = FeeOn(net::SiteId::Ignite, BuyIn);
				}
				const double Pre = Ratio(B, net::SiteId::Ignite);
				Pay(L, net::SiteId::Ignite, Id, BuyIn, Fee, At);
				RakeCum += static_cast<double>(Fee);
				Guard(Pre, "base (and holds cleared at entry)");
				const ss::net::EventTemplate* T = Fuse ? nullptr : net::Site(net::SiteId::Ignite).FindTemplate(TemplateFor(BuyIn));
				const int Field = Fuse ? 3 : std::max(10, T ? T->Field : 60);
				// The result first decides how the hands go: a win, a cash, a bust; the hands carry the knockouts.
				int Place = 0;
				if (Fuse)
				{
					const double U = R.Next();
					Place = U < 0.37 ? 1 : U < 0.685 ? 2 : 3;
				}
				else
				{
					const double Pct = std::pow(R.Next(), 1.15);
					Place = std::max(1, static_cast<int>(std::ceil(Pct * Field)));
				}
				const int Hands = Fuse ? 6 : 12;
				const double U = R.Next();
				int Kos = U < 0.06 ? 3 : U < 0.2 ? 2 : U < 0.5 ? 1 : 0;
				for (int Hd = 0; Hd < Hands; ++Hd)
				{
					play::HandNote N;
					N.Paid = true;
					N.Own = true;
					N.EventId = Id;
					N.HandId = Id + "#" + std::to_string(Hd);
					N.Won = R.Next() < 0.2;
					N.Showdown = N.Won && R.Next() < 0.5;
					if (R.Next() < 0.04)
					{
						N.AllIn = true;
						N.Equity = 0.2 + 0.7 * R.Next();
						N.Preflop = R.Next() < 0.5;
						N.Won = R.Next() < N.Equity;
						N.Lost = !N.Won;
					}
					N.Graded = 2;
					N.GoodGrades = (R.Next() < 0.8 ? 1 : 0) + (R.Next() < 0.8 ? 1 : 0);
					static const char Ranks[] = "23456789TJQKA";
					N.HoleCards = std::string(1, Ranks[R.Int(13)]) + "h" + std::string(1, Ranks[R.Int(13)]) + "c";
					if (Kos > 0 && Hd >= Hands - 4)
					{
						N.Knocked.push_back(Regs[static_cast<size_t>(R.Int(60))]);
						N.KnockedNpcs.push_back(-1);
						if (R.Next() < 0.005) // a Headliner among them (the model's 0.4% an entry)
						{
							N.Headliners.push_back(N.Knocked.back());
						}
						--Kos;
					}
					if (Hd == Hands - 1 && Place > 1)
					{
						const int Who = R.Int(60);
						N.BustedBy = Regs[static_cast<size_t>(Who)];
						N.BustedByNpc = Who; // the evening's regulars are world members (G2 fix M1: only they take rows past five)
					}
					const double PreH = Ratio(B, net::SiteId::Ignite);
					play::OnHandPaid(B, L, net::SiteId::Ignite, N, At + 10.0 + Hd);
					Guard(PreH, "hands (Ouch, Payback, scalps)");
				}
				play::SettleFacts F;
				F.Site = net::SiteId::Ignite;
				F.EventId = Id;
				F.Template = Fuse ? std::string() : ss::site::play::siteplay_detail::SitePlayTemplate(Id);
				F.Place = Place;
				F.Entrants = Field;
				F.BuyIn = BuyIn;
				F.Fuse = Fuse;
				F.FuseM = M;
				F.Win = Place == 1;
				F.Final = !Fuse && Place <= 9;
				const int Paid = std::max(1, static_cast<int>(std::lround(0.15 * Field)));
				F.Prize = Fuse ? ss::fuse::Payout(Tier, M)[static_cast<size_t>(Place - 1)]
					: Place <= Paid ? static_cast<Chips>(static_cast<double>(BuyIn) * std::max(1.5, 0.2 * Field / std::pow(static_cast<double>(Place), 0.9))) : 0;
				F.Points = F.Prize > 0 && !Fuse ? net::Points(Place, Field, BuyIn) : 0.0;
				const double PreR = Ratio(B, net::SiteId::Ignite);
				play::OnResult(B, L, F, At + (Fuse ? 15.0 : 90.0));
				Guard(PreR, "results (holds cleared)");
				Clock = std::max(Clock, At + (Fuse ? 15.0 : 90.0));
			}
		}
		// The day's end (5:30 AM): every claim a player makes on the REWARDS page.
		const double End = DayStart + 23.5 * 60.0;
		{
			const double Pre = Ratio(B, net::SiteId::Ignite);
			play::Step(B, L, L.Sites, nullptr, L.Sponsors, Seed, Clock, End);
			Guard(Pre, "the day's steps (Guest List)");
		}
		Clock = End;
		auto Claim = [&](const std::string& Why) {
			(void)Why;
		};
		for (int K = 0; K < 3; ++K)
		{
			const double Pre = Ratio(B, net::SiteId::Ignite);
			Claim(play::ClaimDare(B, L, K, End));
			Guard(Pre, "Dares");
		}
		{
			const double Pre = Ratio(B, net::SiteId::Ignite);
			Claim(play::Spin(B, L, Seed, End));
			Guard(Pre, "the wheel");
		}
		for (int T = 1; T <= 30; ++T)
		{
			const double Pre = Ratio(B, net::SiteId::Ignite);
			Claim(play::ClaimTier(B, L, T, End));
			Guard(Pre, "Wristband tickets");
		}
		{
			const double Pre = Ratio(B, net::SiteId::Ignite);
			Claim(play::Open(B, L, Seed, "*", End));
			Guard(Pre, "Crackers");
		}
		Track(D);
		if (D >= 29)
		{
			Out.RingMax = std::max(Out.RingMax, Ratio(B, net::SiteId::Ignite) - static_cast<double>(B.Gov[1].Banked) / std::max<double>(1.0, static_cast<double>(detail::SitePlayRake30(B.Gov[1]))));
		}
		if (D == 59)
		{
			Value60 = ValueReceived(L);
			Rake60 = RakeCum;
		}
		if (D == 29)
		{
			Out.Rake30 = static_cast<Chips>(std::llround(RakeCum));
			Out.Promo30 = static_cast<Chips>(std::llround(ValueReceived(L)));
			Out.Share1 = RakeCum > 0.0 ? ValueReceived(L) / RakeCum : 0.0;
			Out.Level = play::IgniteLevel(B);
			const play::View V = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, End);
			Out.Tier = V.Ig.Tier;
		}
		if (D == 89)
		{
			Out.Share3 = RakeCum > Rake60 ? (ValueReceived(L) - Value60) / (RakeCum - Rake60) : 0.0;
		}
	}
	for (const auto& M : Made)
	{
		Out.OpenOld += M.second <= Days - 60 ? 1 : 0;
		Out.Waits.push_back(Days - M.second); // still waiting: counted at its wait so far
	}
	Out.SaveBytes = Lines(L).size();
	return Out;
}

double Median(std::vector<double> V)
{
	std::sort(V.begin(), V.end());
	return V.empty() ? 0.0 : V[V.size() / 2];
}

void IgnitePatterns()
{
	const std::vector<PatternDef> Patterns = {
		{"night owl (1-2 x $1.10, 5 nights)", (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6), 1, {}},
		{"typical (5 mixed a week, 3 nights)", (1 << 0) | (1 << 2) | (1 << 5), 2, {}},
		{"casual ($3.30, 3 nights)", (1 << 1) | (1 << 4) | (1 << 6), 0, Repeat(false, 330, 2)},
		{"farmer (one $1.10 a day)", 0x7f, 0, Repeat(false, 110, 1)},
		{"regular (3 a day at $3.30)", 0x7f, 0, Repeat(false, 330, 3)},
		{"heavy (10 MTT $3.30 + 4 FUSE a day)", 0x7f, 0, [] {
			 std::vector<std::pair<bool, Chips>> P = Repeat(false, 330, 10);
			 const std::vector<std::pair<bool, Chips>> F = Repeat(true, 110, 4);
			 P.insert(P.end(), F.begin(), F.end());
			 return P;
		 }()},
		{"FUSE grinder (30 x $1.10 a day)", 0x7f, 0, Repeat(true, 110, 30)},
		{"FUSE grinder (30 x $5.50 a day)", 0x7f, 0, Repeat(true, 550, 30)},
		{"mid (4 a day at $11)", 0x7f, 0, Repeat(false, 1100, 4)},
		{"stop-start (3 a day at $3.30, 10 on, 10 off)", 0x7f, 3, Repeat(false, 330, 3)},
	};
	const int Seeds = Long ? 21 : 5;
	const int Days = 120;
	size_t MostBytes = 0;
	std::printf("  Ignite's patterns (SPEC §2.7) through the real hooks and claims, %d seeds, %d days (median share month 1 / month 3; worst grant\n"
				"  instant; ring at any day's end; hold waits p90 / max; Level and Wristband tier at day 30; most Guest List lines):\n",
		Seeds, Days);
	for (const PatternDef& P : Patterns)
	{
		std::vector<double> S1, S3;
		double RingMax = 0, GrantMax = 0;
		std::vector<int> Waits;
		int OpenOld = 0, MaxHolds = 0;
		std::vector<double> Levels, Tiers, Rake, Promo;
		std::map<std::string, double> By;
		for (int S = 0; S < Seeds; ++S)
		{
			const PatternRun Run = RunPattern(P, 1000u + static_cast<uint64_t>(S), Days);
			MostBytes = std::max(MostBytes, Run.SaveBytes);
			for (const auto& Kv : Run.By)
			{
				By[Kv.first] += Kv.second / Seeds;
			}
			S1.push_back(Run.Share1);
			S3.push_back(Run.Share3);
			RingMax = std::max(RingMax, Run.RingMax);
			GrantMax = std::max(GrantMax, Run.GrantMax);
			Waits.insert(Waits.end(), Run.Waits.begin(), Run.Waits.end());
			OpenOld += Run.OpenOld;
			MaxHolds = std::max(MaxHolds, Run.MaxHolds);
			Levels.push_back(Run.Level);
			Tiers.push_back(Run.Tier);
			Rake.push_back(static_cast<double>(Run.Rake30));
			Promo.push_back(static_cast<double>(Run.Promo30));
		}
		std::sort(Waits.begin(), Waits.end());
		const int P90 = Waits.empty() ? 0 : Waits[static_cast<size_t>(std::min<double>(static_cast<double>(Waits.size()) - 1, std::floor(0.9 * static_cast<double>(Waits.size()))))];
		const int Max = Waits.empty() ? 0 : Waits.back();
		std::printf("    %-38s rake $%7.2f value $%6.2f  share %3.0f%% / %3.0f%%  grant max %3.0f%%  ring max %3.0f%%  waits %2d / %2d d  L%2.0f T%2.0f  lines %d\n", P.Name,
			Median(Rake) / 100.0, Median(Promo) / 100.0, Median(S1) * 100.0, Median(S3) * 100.0, GrantMax * 100.0, RingMax * 100.0, P90, Max, Median(Levels), Median(Tiers),
			MaxHolds);
		std::string Parts = "      month 1 by source:";
		for (const auto& Kv : By)
		{
			Parts += " " + Kv.first + " $" + ss::Fixed(Kv.second / 100.0, 2) + ";";
		}
		std::printf("%s\n", Parts.c_str());
		const std::string Who = std::string(" (") + P.Name + ")";
		Expect(Median(S1) <= 0.45 && Median(S3) <= 0.45, "patterns: the median share <= 45% in months 1 and 3" + Who);
		Expect(GrantMax <= 0.50 + 1e-6, "patterns: no grant past 50% of the ring at its instant, on any seed" + Who);
		Expect(RingMax <= 0.55, "patterns: the ring <= 55% at every day's end, on every seed" + Who);
		Expect(P90 <= 30 && Max <= 60 && OpenOld == 0, "patterns: holds clear, 90% within 30 days and all within 60" + Who);
		Expect(MaxHolds <= 15, "patterns: the Guest List never passes 15 lines" + Who);
	}
	// §1.8, §7.4: the sites' Update 3 lines in a heavy save <= 4.4 KB (the heaviest Ignite book here, with RunItTwice's League
	// book from LeagueWorld beside it; the Governor's rings are in both).
	std::printf("  save: the heaviest Ignite book writes %zu bytes of play lines; RunItTwice's League book %zu; together %zu (budget 4,400)\n", MostBytes, RitBookBytes,
		MostBytes + RitBookBytes);
	Expect(MostBytes + RitBookBytes <= 4400, "save: the sites' Update 3 lines in a heavy save stay inside 4.4 KB");
	// G2 fix M1: the Beef window counts up to 24 names (the patterns bust nobody), so the worst case adds a full window.
	std::printf("  save: with a full Beef window (%zu B of beef lines) %zu (budget 4,400)\n", BeefFullBytes, MostBytes + RitBookBytes + BeefFullBytes);
	Expect(BeefFullBytes > 0 && MostBytes + RitBookBytes + BeefFullBytes <= 4400, "save: a heavy save with a full Beef window stays inside 4.4 KB");
}

// ------------------------------------------------------------------ the League through LeagueScore (SPEC §3.6 R2, §3.7)

struct RitT
{
	std::string Id;
	Chips BuyIn;
	int Field;
	int Days; // weekday mask
};

std::vector<RitT> RitMenu(int Div)
{
	static const Chips Top[5] = {1100, 1650, 3300, 10900, 53000};
	std::vector<RitT> Out;
	for (const ss::net::EventTemplate& T : net::Site(net::SiteId::RunItTwice).Templates())
	{
		if (!T.Series.empty() || T.OnlyDay != -9999 || T.BuyInCents < play::DivisionFloor(Div) || T.BuyInCents > Top[Div])
		{
			continue;
		}
		Out.push_back({detail::SitePlayTemplate(T.Id), T.BuyInCents, std::max(10, static_cast<int>(T.Field * net::Def(net::SiteId::RunItTwice).Traffic.Scale)), T.Days});
	}
	return Out;
}

/** A month's League points: the first k entries a day at or above the floor, their daily best, the best 8 days (R2). */
double LeagueMonth(ss::Rng& R, int Div, int Events, double Edge, bool Fixtures, int K, int FirstDay, double& Rake)
{
	const std::vector<RitT> Menu = RitMenu(Div);
	if (Menu.empty())
	{
		return 0.0;
	}
	std::vector<int> Per(30, 0);
	for (int E = 0; E < Events; ++E)
	{
		Per[static_cast<size_t>(Events >= 30 ? E % 30 : R.Int(30))] += 1;
	}
	std::vector<double> Daily;
	for (int D = 0; D < 30; ++D)
	{
		const int Day = FirstDay + D;
		const int Wd = ss::net::Weekday(Day);
		std::vector<const RitT*> Today;
		for (const RitT& T : Menu)
		{
			if ((T.Days & (1 << Wd)) != 0)
			{
				Today.push_back(&T);
			}
		}
		double Best = 0.0;
		for (int I = 0; I < Per[static_cast<size_t>(D)] && !Today.empty(); ++I)
		{
			const RitT* Pick = Today[static_cast<size_t>(R.Int(static_cast<int>(Today.size())))];
			if (Fixtures && I == 0)
			{
				for (const RitT* T : Today)
				{
					Pick = play::MatchdayMult(Div, T->Id, Day) > 1.0 ? T : Pick;
				}
			}
			Rake += static_cast<double>(FeeOn(net::SiteId::RunItTwice, Pick->BuyIn));
			if (I >= K)
			{
				continue;
			}
			const int Field = std::max(2, static_cast<int>(Pick->Field * R.Range(0.82, 1.25)));
			const double Pct = std::pow(R.Next(), 1.0 + Edge);
			const int Place = std::max(1, static_cast<int>(std::ceil(Pct * Field)));
			Best = std::max(Best, play::LeagueScore(Div, Pick->Id, Pick->BuyIn, Day, net::Points(Place, Field, Pick->BuyIn), false, false));
		}
		if (Best > 0.0)
		{
			Daily.push_back(Best);
		}
	}
	std::sort(Daily.begin(), Daily.end(), [](double A, double B) { return A > B; });
	double Sum = 0.0;
	for (size_t I = 0; I < Daily.size() && I < 8; ++I)
	{
		Sum += Daily[I];
	}
	return Sum;
}

struct LeagueOdds
{
	double Promoted = 0, Bottom = 0, Take = 0;
};

LeagueOdds LeagueRun(int Div, int Events, double Edge, bool Matched, int Seeds)
{
	static const Chips Ticket[5] = {550, 1100, 2200, 5500, 10900};
	LeagueOdds O;
	double Rake = 0.0, Take = 0.0;
	for (int S = 0; S < Seeds; ++S)
	{
		ss::Rng R("siteplay-league:" + std::to_string(Div) + ":" + std::to_string(Events) + ":" + std::to_string(S) + (Matched ? ":m" : ":w"));
		std::vector<int> Vols;
		for (int K = 0; K < 19; ++K)
		{
			Vols.push_back(Matched ? std::max(4, static_cast<int>(Events * R.Range(0.5, 2.0))) : 10 + R.Int(36));
		}
		std::vector<double> Per;
		for (const int V : Vols)
		{
			Per.push_back(V >= 30 ? V / 30.0 : 1.0);
		}
		std::sort(Per.begin(), Per.end());
		const int K = Matched ? 3 : std::clamp(static_cast<int>(std::lround(Per[9])), 1, 3);
		double Unused = 0.0;
		std::vector<double> Npcs;
		for (const int V : Vols)
		{
			Npcs.push_back(LeagueMonth(R, Div, V, R.Range(-0.05, 0.10), R.Next() < 0.5, K, Dec1, Unused));
		}
		double MyRake = 0.0;
		const double Me = LeagueMonth(R, Div, Events, Edge, true, K, Dec1, MyRake);
		int Place = 1;
		for (const double N : Npcs)
		{
			Place += N > Me ? 1 : 0;
		}
		O.Promoted += Place <= 3 ? 1.0 : 0.0;
		O.Bottom += Place >= 17 ? 1.0 : 0.0;
		Take += Place <= 3 ? static_cast<double>(Ticket[Div]) : 0.0;
		Rake += MyRake;
	}
	O.Promoted /= Seeds;
	O.Bottom /= Seeds;
	O.Take = Rake > 0.0 ? Take / Rake : 0.0;
	return O;
}

/** League()'s P(promoted) at each §3.7 profile's volume and division (Step-up, Regular, Pro): RitProfiles draws its promotions at
 *  these (G2 fix M6: the real scoring's rate, not an assumed 30%). */
double ProfilePromoted[3] = {0.30, 0.35, 0.30};

void League()
{
	const int Seeds = Long ? 201 : 61;
	std::printf("  the League through LeagueScore over RunItTwice's own templates and fields (%d seeds a row; volume-matched groups):\n", Seeds);
	for (int Div = 1; Div <= 4; ++Div)
	{
		for (const std::pair<int, double>& Case : std::vector<std::pair<int, double>>{{30, 0.0}, {30, 0.15}, {60, 0.15}, {120, 0.15}})
		{
			const LeagueOdds O = LeagueRun(Div, Case.first, Case.second, true, Seeds);
			std::printf("    %-11s events %3d edge %.2f  take/rake %5.1f%%  P(promoted) %5.1f%%  P(bottom 4) %5.1f%%\n", play::DivisionName(Div), Case.first, Case.second,
				O.Take * 100.0, O.Promoted * 100.0, O.Bottom * 100.0);
			if (Case.first == 120 && Case.second > 0.1)
			{
				Expect(O.Promoted <= 0.60, std::string("league: P(promoted) <= 60% for a 120-event player with an edge, ") + play::DivisionName(Div));
			}
			if (Case.first == 30 && Case.second == 0.0)
			{
				Expect(O.Promoted >= 0.03, std::string("league: P(promoted) >= 3% at 30 events with no edge, ") + play::DivisionName(Div));
			}
		}
	}
	if (Long)
	{
		std::printf("  the group-pace case (the world's 10-45 a month):\n");
		for (int Div = 1; Div <= 4; ++Div)
		{
			const LeagueOdds O = LeagueRun(Div, 120, 0.15, false, Seeds);
			std::printf("    %-11s events 120 edge 0.15  take/rake %5.1f%%  P(promoted) %5.1f%%\n", play::DivisionName(Div), O.Take * 100.0, O.Promoted * 100.0);
		}
	}
	// §3.7's profiles: the League's take at their own volume and division.
	struct Profile
	{
		const char* Name;
		int Div;
		int Events;
	};
	int Ix = 0;
	for (const Profile& P : {Profile{"Step-up (Steel, 20 a week at $11)", 2, 86}, Profile{"Regular (Gold, 40 a week at $22)", 3, 172}, Profile{"Pro (High Bridge, 30 a week at $109)", 4, 129}})
	{
		const LeagueOdds O = LeagueRun(P.Div, P.Events, 0.15, true, Seeds);
		std::printf("    profile %-36s League take %4.1f%% of rake (P(promoted) %4.1f%%)\n", P.Name, O.Take * 100.0, O.Promoted * 100.0);
		Expect(O.Take <= 0.20, std::string("league: the take <= 20% of RunItTwice rake, ") + P.Name);
		ProfilePromoted[Ix++] = O.Promoted;
	}
}

// ------------------------------------------------------------------ RunItTwice's profiles through the real Governor (§3.7)

struct RitProfileDef
{
	const char* Name;
	int PerWeek;
	Chips BuyIn;
	int Div;
	double PromoteP;
};

/** One profile's 150 days on one seed (seed 0 is the old single run's key). */
struct RitProfileRun
{
	double Tickets = 0, Total = 0;
	int MaxWait = 0;
	double RingMax = 0;
	bool Governed = false;
};

RitProfileRun RitProfileOn(const RitProfileDef& P, int Seed)
{
	RitProfileRun Run;
	{
		const double Start = play::PlayDayStart(Dec1);
		ss::life::State L = Fresh(Start);
		play::Book& B = L.SitePlay;
		ss::Rng R("siteplay-rit:" + std::string(P.Name) + (Seed == 0 ? std::string() : ":" + std::to_string(Seed)));
		Chips Bank = 0;
		double Clock = Start;
		int MaxWait = 0;
		std::map<std::string, int> Made;
		double RingMax = 0.0;
		Chips SecondRun = 0;
		double RakeAll = 0.0;
		const Chips Fee = FeeOn(net::SiteId::RunItTwice, P.BuyIn);
		for (int D = 0; D < 150; ++D)
		{
			const int Day = Dec1 + D;
			const double DayStart = play::PlayDayStart(Day);
			std::vector<std::pair<std::string, std::string>> Texts;
			const Chips Before = site::AccountOf(L.Sites, net::SiteId::RunItTwice)->Cents;
			site::Step(L.Sites, L, Bank, ss::rep::Public(), 0, Clock, DayStart + 1.0, Texts);
			SecondRun += site::AccountOf(L.Sites, net::SiteId::RunItTwice)->Cents - Before;
			const bool First = play::PlayMonth(Day) != play::PlayMonth(Day - 1);
			play::Step(B, L, L.Sites, nullptr, L.Sponsors, 11, Clock, DayStart + 1.0);
			if (First && D > 0 && R.Next() < P.PromoteP)
			{
				static const Chips Ticket[5] = {550, 1100, 2200, 5500, 10900};
				detail::SitePlayPayload Pay;
				Pay.TicketId = "runittwice.the-sweat";
				Pay.Label = "League ticket";
				detail::SitePlayGrant(B, L, net::SiteId::RunItTwice, play::Kind::LeagueTicket, "League ticket", Ticket[P.Div] * play::ValuePerCent, Pay,
					"league:" + std::to_string(play::PlayMonth(Day - 1)), Day, DayStart + 1.0, true);
			}
			Clock = DayStart + 1.0;
			const int Today = P.PerWeek / 7 + (R.Int(7) < P.PerWeek % 7 ? 1 : 0);
			for (int K = 0; K < Today; ++K)
			{
				const double At = DayStart + 12.0 * 60.0 + 20.0 * K;
				Pay(L, net::SiteId::RunItTwice, IdAt("runittwice.the-sweat", At + 5.0), P.BuyIn, Fee, At);
				RakeAll += static_cast<double>(Fee);
				play::SettleFacts F;
				F.Site = net::SiteId::RunItTwice;
				F.EventId = IdAt("runittwice.the-sweat", At + 5.0);
				F.Place = 50;
				F.Entrants = 100;
				F.BuyIn = P.BuyIn;
				play::OnResult(B, L, F, At + 100.0);
				Clock = std::max(Clock, At + 100.0);
			}
			// About 0.4-1 Team scalps a month, at the profile's stake.
			if (R.Next() < 0.7 / 30.0)
			{
				detail::SitePlayPayload Pay;
				Pay.TicketId = "runittwice.supper-stack";
				Pay.Label = "Team Scalp ticket";
				detail::SitePlayGrant(B, L, net::SiteId::RunItTwice, play::Kind::ScalpTicket, "Team Scalp ticket", (P.BuyIn >= 10900 ? 1100 : 550) * play::ValuePerCent, Pay,
					"scalp:" + std::to_string(D), Day, Clock, true);
			}
			std::map<std::string, int> Now;
			for (const play::Hold& H : B.Holds)
			{
				Now[H.Key] = Made.count(H.Key) ? Made[H.Key] : D;
			}
			for (const auto& M : Made)
			{
				if (!Now.count(M.first))
				{
					MaxWait = std::max(MaxWait, D - M.second);
				}
			}
			Made = Now;
			if (D >= 30)
			{
				const double Ring = Ratio(B, net::SiteId::RunItTwice);
				if (Ring > RingMax && Ring > 0.55)
				{
					std::printf("      (day %d: the ring %.1f%%: rake30 $%.2f promo30 $%.2f, holds %zu)\n", D, Ring * 100.0,
						static_cast<double>(play::RakeCents30(B, net::SiteId::RunItTwice)) / 100.0, static_cast<double>(play::PromoCents30(B, net::SiteId::RunItTwice)) / 100.0,
						B.Holds.size());
				}
				RingMax = std::max(RingMax, Ring);
			}
		}
		for (const auto& M : Made)
		{
			MaxWait = std::max(MaxWait, 150 - M.second);
		}
		const Chips Rake = play::RakeCents30(B, net::SiteId::RunItTwice);
		const double Tickets = ValueReceived(L);
		const double Total = (static_cast<double>(SecondRun) + Tickets) / std::max(1.0, RakeAll);
		if (Seed == 0)
		{
			std::printf("    RunItTwice %-18s seed 0: rake30 $%7.2f; over 150 days rake $%.2f, Second Run $%.2f, League and Scalp tickets $%.2f: given back %4.1f%% "
						"(tickets %4.1f%%); longest wait %d days; ring max %.0f%%\n",
				P.Name, static_cast<double>(Rake) / 100.0, RakeAll / 100.0, static_cast<double>(SecondRun) / 100.0, Tickets / 100.0, Total * 100.0,
				Tickets / std::max(1.0, RakeAll) * 100.0, MaxWait, RingMax * 100.0);
		}
		Run.Tickets = Tickets / std::max(1.0, RakeAll);
		Run.Total = Total;
		Run.MaxWait = MaxWait;
		Run.RingMax = RingMax;
		Run.Governed = Total <= 0.50 && SecondRun > 0;
	}
	return Run;
}

void RitProfiles()
{
	// G2 fix M6: every profile over Seeds seeds (seed 0 is the old single run's), each month's promotion drawn at League()'s
	// P(promoted) for that profile; the bound is §1.13's "League tickets + Team Scalps <= 20% of RunItTwice rake" at the median
	// and the mean (C2 read 20.2% off one seed); every seed keeps the Governor's checks (total, waits, ring).
	const int Seeds = Long ? 48 : 24;
	const RitProfileDef Profiles[] = {{"Step-up (Steel)", 20, 1100, 2, ProfilePromoted[0]}, {"Regular (Gold)", 40, 2200, 3, ProfilePromoted[1]},
		{"Pro (High Bridge)", 30, 10900, 4, ProfilePromoted[2]}};
	for (const RitProfileDef& P : Profiles)
	{
		std::vector<double> TicketShare, TotalShare;
		int WaitMax = 0;
		double RingWorst = 0.0;
		int Over = 0;
		for (int Seed = 0; Seed < Seeds; ++Seed)
		{
			const RitProfileRun Run = RitProfileOn(P, Seed);
			TicketShare.push_back(Run.Tickets);
			TotalShare.push_back(Run.Total);
			WaitMax = std::max(WaitMax, Run.MaxWait);
			RingWorst = std::max(RingWorst, Run.RingMax);
			Over += Run.Governed ? 0 : 1;
		}
		auto Mean = [](const std::vector<double>& V) {
			double S = 0.0;
			for (const double X : V)
			{
				S += X;
			}
			return V.empty() ? 0.0 : S / static_cast<double>(V.size());
		};
		auto Median = [](std::vector<double> V) {
			std::sort(V.begin(), V.end());
			return V.empty() ? 0.0 : V.size() % 2 ? V[V.size() / 2] : (V[V.size() / 2 - 1] + V[V.size() / 2]) / 2.0;
		};
		const double TicketsMax = *std::max_element(TicketShare.begin(), TicketShare.end());
		std::printf("    RunItTwice %-18s %d seeds, promoted at %4.1f%% a month: tickets median %4.1f%% mean %4.1f%% max %4.1f%% of rake; given back median %4.1f%% "
					"mean %4.1f%% max %4.1f%%; longest wait %d days; ring max %.0f%%\n",
			P.Name, Seeds, P.PromoteP * 100.0, Median(TicketShare) * 100.0, Mean(TicketShare) * 100.0, TicketsMax * 100.0, Median(TotalShare) * 100.0,
			Mean(TotalShare) * 100.0, *std::max_element(TotalShare.begin(), TotalShare.end()) * 100.0, WaitMax, RingWorst * 100.0);
		// (The League's take against its rake is League()'s check, through LeagueScore at the profile's volume: here a
		// promotion is a draw at League()'s rate, so this harness checks the Governor's side: the total, the waits, the ring,
		// and §1.13's ticket line at the median and the mean.)
		Expect(Over == 0, std::string("rit: given back under the 50% Governor, Second Run booked, every seed: ") + P.Name);
		Expect(WaitMax <= 60, std::string("rit: every hold clears within 60 days, every seed: ") + P.Name);
		Expect(RingWorst <= 0.55, std::string("rit: the ring <= 55% at every day's end, every seed: ") + P.Name);
		Expect(Median(TicketShare) <= 0.20 && Mean(TicketShare) <= 0.20, std::string("rit: League tickets and Team Scalps <= 20% of RunItTwice rake (median, mean): ") + P.Name);
	}
}

// ------------------------------------------------------------------ the copy: the lint, U1, the trademark screen

void Copy()
{
	int Problems = 0;
	std::map<std::string, std::string> Vars = copylint::Samples();
	for (const char* K : {"Site", "Promo", "Rake", "Pct", "Entries", "Short", "Sparks", "Days", "Day", "Level", "Won", "Of", "Place", "Div", "Month", "NewDiv", "Ticket", "Count"})
	{
		Vars[K] = "Ignite";
	}
	Vars["Pct"] = "87%";
	Vars["Sparks"] = "2,140";
	Vars["Days"] = "6";
	std::vector<std::pair<std::string, std::string>> Sentences;
	for (const std::string& S : play::Inventory())
	{
		Sentences.emplace_back("site play", S);
	}
	Problems += copylint::Run("site play", Sentences, copylint::LetterBody, Vars);
	for (const std::string& L : play::InventoryLabels())
	{
		Problems += L.find("  ") != std::string::npos ? 1 : 0;
	}
	int Banned = 0;
	int Marks = 0;
	std::vector<std::string> All = play::Inventory();
	const std::vector<std::string> Labels = play::InventoryLabels();
	All.insert(All.end(), Labels.begin(), Labels.end());
	for (const std::string& S : All)
	{
		const std::string Low = trademarks::Lower(S);
		for (const char* U1 : {"ai", "bot", "bots", "autoplay", "autopilot", "jackpot"})
		{
			if (trademarks::HasWord(Low, U1))
			{
				++Banned;
				std::printf("COPY (site play) U1: \"%s\"\n", S.c_str());
			}
		}
		for (const char* Glyph : {"\xE2\x86\x92", "\xC3\x97", "\xE2\x98\x85"})
		{
			Banned += S.find(Glyph) != std::string::npos ? 1 : 0; // arrows, the multiplication sign, stars: vectors, never text
		}
		const char* Hit = trademarks::Find(S, trademarks::kSourceScan);
		const char* HitCopy = trademarks::Find(S, trademarks::kCopyScan);
		if (Hit || HitCopy)
		{
			++Marks;
			std::printf("COPY (site play) trademark \"%s\": \"%s\"\n", Hit ? Hit : HitCopy, S.c_str());
		}
	}
	Expect(Problems == 0, "copy: site::play::Inventory() passes the copy lint");
	Expect(Banned == 0, "copy: no AI, bot, autoplay or jackpot, and only the measured glyphs, in the loops' copy");
	Expect(Marks == 0, "copy: the trademark screen (both tiers) over the loops' copy");
	std::printf("  copy: %zu sentences, %zu labels\n", play::Inventory().size(), Labels.size());
}

// ------------------------------------------------------------------ the review's checks (review-core-siteplay, G2)

ss::sponsor::Contract RiverLineDeal(double Since, double Until)
{
	ss::sponsor::Contract C;
	C.Id = "riverline@1";
	C.Brand = "riverline";
	C.St = ss::sponsor::State::Active;
	C.T.Only = "riverline";
	C.Since = Since;
	C.Until = Until;
	return C;
}

/** Sets every League member's month to Points on day 3 (the world's book, as its recording would leave it). */
void LeagueMembersAt(ss::world::World& W, int16_t Tenths)
{
	ss::world::LeagueBook* Lb = const_cast<ss::world::LeagueBook*>(W.League(net::SiteId::RunItTwice));
	if (!Lb)
	{
		return;
	}
	for (ss::world::LeagueMember& M : Lb->Members)
	{
		M.Best = {};
		M.Best[0].first = static_cast<int8_t>(3);
		M.Best[0].second = Tenths;
		M.Days = 1;
	}
}

/** A blank RunItTwice result for the player (a day played, no points). */
void RitBlank(ss::life::State& L, const std::string& Id, double World)
{
	play::SettleFacts F;
	F.Site = net::SiteId::RunItTwice;
	F.EventId = Id;
	F.Place = 90;
	F.Entrants = 94;
	F.BuyIn = 1100;
	play::OnResult(L.SitePlay, L, F, World);
}

void Hardening()
{
	const uint64_t Seed = 4242;
	const double Mon = (Dec1 + 6) * DayMin + 360.0; // Monday, December 7, 6 AM
	// Dares are dealt past the formats the player hasn't unlocked (F3, Joinable's locks; review: "Play a bounty or mystery
	// event" was dealt before the first cash opened them): none before the unlock; after it, only the days that drew it change,
	// and the book's card is the page's (a bounty entry moves the open player's Dare, not the locked player's).
	{
		ss::life::State Locked = Fresh(Mon);
		ss::life::State Open = Locked;
		Open.Unlocks.insert("bounty");
		int Drew = -1, Other = 0, Same = 0;
		bool Clean = true;
		for (int K = 0; K < 60; ++K)
		{
			const double At = Mon + K * DayMin + 600.0;
			const play::View Vl = play::ViewOf(Locked.SitePlay, Locked, Locked.Sites, nullptr, net::SiteId::Ignite, At);
			const play::View Vo = play::ViewOf(Open.SitePlay, Open, Open.Sites, nullptr, net::SiteId::Ignite, At);
			Clean = Clean && Vl.Ig.Dares[0].Id != 104;
			if (Vo.Ig.Dares[0].Id == 104)
			{
				Drew = Drew < 0 ? K : Drew;
				continue;
			}
			++Other;
			Same += Vo.Ig.Dares[0].Id == Vl.Ig.Dares[0].Id && Vo.Ig.Dares[1].Id == Vl.Ig.Dares[1].Id && Vo.Ig.Dares[2].Id == Vl.Ig.Dares[2].Id ? 1 : 0;
		}
		Expect(Clean && Drew >= 0 && Same == Other, "dares: no bounty Dare before the unlock, and only the days that drew it change after (" + std::to_string(Drew) + ", " +
			std::to_string(Same) + " of " + std::to_string(Other) + ")");
		if (Drew >= 0)
		{
			const double Start = Mon + Drew * DayMin + 510.0; // 2:30 PM: a Flip & Run Bounty (no guarantee, not before noon, not evening)
			for (ss::life::State* L : {&Locked, &Open})
			{
				Pay(*L, net::SiteId::Ignite, IdAt("ignite.flip-go-bounty", Start), 330, FeeOn(net::SiteId::Ignite, 330), Start - 5.0);
			}
			const play::View Vl = play::ViewOf(Locked.SitePlay, Locked, Locked.Sites, nullptr, net::SiteId::Ignite, Start);
			const play::View Vo = play::ViewOf(Open.SitePlay, Open, Open.Sites, nullptr, net::SiteId::Ignite, Start);
			Expect(Vo.Ig.Dares[0].Id == 104 && Vo.Ig.Dares[0].Done && Vl.Ig.Dares[0].Id != 104 && !Vl.Ig.Dares[0].Done,
				"dares: the book deals the page's card (a bounty entry clears the open player's bounty Dare only)");
		}
	}
	// Team Scalps (R4): each of the year's six once, a ticket for at most 2 a week (the rest are taken without one), FULL SET
	// once a year; signed with Team RunItTwice the player is the bounty and scalps are off (review: the weekly cap was untested).
	{
		const double At0 = Mon + 600.0; // Monday, December 7, 4 PM
		const std::string Rid = IdAt("runittwice.the-sweat", At0 + 30.0);
		auto Scalp = [&](ss::life::State& L, int Pro, double At, int First = 101) {
			play::HandNote N;
			N.Paid = true;
			N.EventId = Rid;
			N.HandId = Rid + "#team" + std::to_string(Pro) + ":" + std::to_string(static_cast<long long>(At));
			N.BuyIn = 1100;
			N.TeamSix = {First, 102, 103, 104, 105, 106};
			N.TeamPros = {Pro};
			play::OnHandPaid(L.SitePlay, L, net::SiteId::RunItTwice, N, At);
		};
		auto Tickets = [](const ss::life::State& L, const std::string& Id) {
			int N = 0;
			const auto It = L.Tickets.find(Id);
			N += It == L.Tickets.end() ? 0 : It->second;
			for (const play::Hold& H : L.SitePlay.Holds)
			{
				N += H.Key.find(":t:" + Id + ":") != std::string::npos ? 1 : 0;
			}
			return N;
		};
		ss::life::State L = Fresh(At0);
		Pay(L, net::SiteId::RunItTwice, Rid, 1100, FeeOn(net::SiteId::RunItTwice, 1100), At0);
		for (int K = 0; K < 20; ++K) // rake enough that every ticket pays at once (a held one dedupes by its key)
		{
			Pay(L, net::SiteId::RunItTwice, IdAt("runittwice.sunday-100k", At0 - 600.0 + K), 10900, FeeOn(net::SiteId::RunItTwice, 10900), At0 - 300.0 + K);
		}
		for (int Pro = 101; Pro <= 104; ++Pro)
		{
			Scalp(L, Pro, At0 + Pro - 100);
		}
		const play::Scalps& Sc = L.SitePlay.Rt.Sc;
		Expect(Sc.Lifetime == 4 && Sc.Taken == 0x0F && Tickets(L, "runittwice.supper-stack") == 2, "team scalps: four in a week are all taken, two with a ticket");
		const double Next = At0 + 7.0 * DayMin;
		Pay(L, net::SiteId::RunItTwice, Rid + "1", 1100, FeeOn(net::SiteId::RunItTwice, 1100), Next);
		Scalp(L, 105, Next + 1.0);
		Scalp(L, 106, Next + 2.0);
		Scalp(L, 101, Next + 3.0);
		Expect(Sc.Lifetime == 6 && Sc.Taken == 0x3F && Tickets(L, "runittwice.supper-stack") == 4 && Tickets(L, "runittwice.the-sweat") == 1 && (L.SitePlay.Rt.Creds & (1u << 8)) != 0,
			"team scalps: the next week's two pay again, the set's sixth pays FULL SET once, a taken pro pays nothing");
		Scalp(L, 107, Next + 4.0, 107); // a newly signed pro in the departed 101's slot: his scalp, the set whole again
		Expect(Sc.Lifetime == 7 && Sc.Taken == 0x3F && Tickets(L, "runittwice.supper-stack") == 4 && Tickets(L, "runittwice.the-sweat") == 1 && L.SitePlay.Holds.empty(),
			"team scalps: a new pro in a departed pro's slot is a scalp, and the set whole again pays no second FULL SET that year");
		ss::life::State T = Fresh(At0);
		ss::sponsor::Contract Team = RiverLineDeal(At0 - DayMin, At0 + 30.0 * DayMin);
		Team.Id = "turnstone@1";
		Team.Brand = "turnstone";
		Team.T.Only = "runittwice";
		T.Sponsors.Contracts.push_back(Team);
		Pay(T, net::SiteId::RunItTwice, Rid, 1100, FeeOn(net::SiteId::RunItTwice, 1100), At0);
		Scalp(T, 101, At0 + 1.0);
		Expect(T.SitePlay.Rt.Sc.Lifetime == 0 && Tickets(T, "runittwice.supper-stack") == 0, "team scalps: off while signed with Team RunItTwice (you're the bounty)");
	}
	// A skip of N days is N one-day steps (§1.1, U2): one 40-day step and 40 one-day steps leave the same book, done keys and
	// tickets, with the day's done Dares credited on their 6 AM, the Wristband's 1st, the anniversary's Cracker in the gap,
	// and each day's spare Room on the Guest List. Then an overlapping window pays nothing twice.
	{
		ss::life::State A = Fresh(Mon);
		for (int K = 0; K < 3; ++K)
		{
			Pay(A, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", Mon + 600.0 + 30.0 * K), 550, FeeOn(net::SiteId::Ignite, 550), Mon + 600.0 + 30.0 * K);
		}
		A.SitePlay.Ig.D.Prog = {20, 20, 20};
		A.SitePlay.Ig.S.BaseMonth = 7000; // T10 reached, unclaimed
		ss::life::State S = A;
		const double From = Mon + 700.0;
		const int Day0 = play::PlayDay(Mon);
		play::Step(A.SitePlay, A, A.Sites, nullptr, A.Sponsors, Seed, From, play::PlayDayStart(Day0 + 40) + 1.0);
		double C = From;
		for (int D = 1; D <= 40; ++D)
		{
			const double To = play::PlayDayStart(Day0 + D) + 1.0;
			play::Step(S.SitePlay, S, S.Sites, nullptr, S.Sponsors, Seed, C, To);
			C = To;
		}
		bool Anniv = false;
		for (const play::Box& X : A.SitePlay.Ig.Boxes)
		{
			Anniv = Anniv || X.Id.compare(0, 6, "anniv:") == 0;
		}
		Expect(Lines(A) == Lines(S) && A.Sites.Done == S.Sites.Done && A.Tickets == S.Tickets && A.SitePlay.Ig.S.Balance == S.SitePlay.Ig.S.Balance,
			"skip: one 40-day step leaves the book 40 one-day steps leave (Dares on their 6 AM, the 1st, each day's Guest List)");
		Expect(Anniv, "skip: the Ignite anniversary's Neon Cracker inside a skipped gap still lands on the shelf");
		const std::string Once = Lines(A);
		play::Step(A.SitePlay, A, A.Sites, nullptr, A.Sponsors, Seed, From + 20.0 * DayMin, play::PlayDayStart(Day0 + 40) + 1.0);
		play::Step(A.SitePlay, A, A.Sites, nullptr, A.Sponsors, Seed, From, play::PlayDayStart(Day0 + 40) + 1.0);
		Expect(Lines(A) == Once, "skip: an overlapping window over the skip's days pays nothing twice");
	}
	// The Governor's own door is keyed: a second grant under one key pays nothing.
	{
		ss::life::State L = Fresh(Mon);
		for (int K = 0; K < 40; ++K)
		{
			Pay(L, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", Mon + 600.0 + K), 550, FeeOn(net::SiteId::Ignite, 550), Mon + 600.0 + K);
		}
		const int64_t T0 = L.TicketsFor("ignite.flip-go");
		const int Day = play::PlayDay(Mon + 700.0);
		const bool First = play::Grant(L.SitePlay, L, net::SiteId::Ignite, play::Kind::Ticket, "Flip & Run $1.10 ticket", 110, "ignite.flip-go|once:1", Day, Mon + 700.0);
		const bool Second = play::Grant(L.SitePlay, L, net::SiteId::Ignite, play::Kind::Ticket, "Flip & Run $1.10 ticket", 110, "ignite.flip-go|once:1", Day, Mon + 701.0);
		Expect(First && !Second && L.TicketsFor("ignite.flip-go") == T0 + 1, "gov: Grant is keyed (a second call with the same key pays nothing)");
	}
	// The Guest List moves inside the week's band cap (§1.4: the spare Room "inside the week's cap"): with the micro cap full,
	// a hold covers only what the cap had left until Monday.
	{
		const double T0 = Dec1 * DayMin + 6.0 * 60.0 + 600.0; // Tuesday 4 PM
		ss::life::State H = Fresh(T0);
		play::Book& Hb = H.SitePlay;
		const Chips Fee = FeeOn(net::SiteId::Ignite, 550);
		double At = T0;
		for (int K = 0; K < 200; ++K)
		{
			At += 2.0;
			Pay(H, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", At + 30.0), 550, Fee, At);
		}
		const int64_t Cap = 1200 * play::ValuePerCent;
		const int64_t Left = Cap - Hb.Gov[1].WeekFace;
		const bool Held = !play::Grant(Hb, H, net::SiteId::Ignite, play::Kind::Ticket, "Flip & Run $1.10 ticket", 110, "ignite.flip-go|cap:1", play::PlayDay(At), At);
		for (int K = 0; K < 100; ++K)
		{
			At += 2.0;
			Pay(H, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", At + 30.0), 550, Fee, At);
		}
		Expect(Held && Left >= 0 && Hb.Holds.size() == 1 && Hb.Holds[0].Covered <= Left && Hb.Gov[1].WeekFace <= Cap,
			"gov: with the week's cap full a hold covers no more than the cap had left (the Guest List clears inside the cap)");
	}
	// Ignite's welcome release counts in the ring (§1.3, §1.4: never held, booked on the rake's day; the 75% ceiling).
	{
		const double T0 = Dec1 * DayMin + 6.0 * 60.0 + 600.0;
		ss::life::State Wl = Fresh(T0);
		site::Account* A = site::AccountOf(Wl.Sites, net::SiteId::Ignite);
		A->BonusPending = 2000;
		A->BonusExpires = T0 + 60.0 * DayMin;
		double At = T0;
		int N = 0;
		while (A->BonusReleased == 0 && N < 200)
		{
			At += 2.0;
			Pay(Wl, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", At + 30.0), 550, FeeOn(net::SiteId::Ignite, 550), At);
			++N;
		}
		const int64_t Base = N > 0 ? Wl.SitePlay.Ig.S.BaseLife / N : 0;
		const int64_t Expected = N * play::SparksValue(Base) + A->BonusReleased * play::ValuePerCent;
		Expect(A->BonusReleased == 500 && Wl.SitePlay.Ig.S.BaseLife == N * Base && detail::SitePlayPromo30(Wl.SitePlay.Gov[1]) == Expected,
			"gov: Ignite's $5 welcome release is counted in the ring on its rake's day (" + std::to_string(N) + " entries)");
	}
	// RunItTwice's Room reserves the month's accruing Second Run (booked on the 1st into the month that earned it).
	{
		const double T0 = Dec1 * DayMin + 6.0 * 60.0 + 600.0;
		ss::life::State R = Fresh(T0);
		site::Account* A = site::AccountOf(R.Sites, net::SiteId::RunItTwice);
		double At = T0;
		for (int K = 0; K < 200 && A->RakeMonth < 5000; ++K)
		{
			At += 2.0;
			Pay(R, net::SiteId::RunItTwice, IdAt("runittwice.sunday-grind", At + 30.0), 2200, FeeOn(net::SiteId::RunItTwice, 2200), At);
		}
		const std::vector<site::TierInfo>& Tiers = site::SecondRunTiers();
		const int Permille = Tiers[static_cast<size_t>(site::TierFor(Tiers, A->RakeMonth))].Permille;
		const int64_t Rake = detail::SitePlayRake30(R.SitePlay.Gov[2]);
		const int64_t Room = detail::SitePlayRoom(R.SitePlay, R, net::SiteId::RunItTwice, At);
		Expect(Permille > 0 && Room == Rake / 2 - Rake * Permille / 1000, "gov: RunItTwice's Room keeps the month's Second Run (" + std::to_string(Permille / 10) + "%) aside as it accrues");
	}
	// The desk bell rings for a starred event whether or not there's a League month (R1 is the Watchlist's).
	{
		net::Network& Net = net::Site(net::SiteId::RunItTwice);
		std::string Tmpl;
		double At = 0.0;
		for (const net::EventInstance& E : Net.Window(Mon, Mon + 3.0 * DayMin))
		{
			const double Closes = E.Start + E.LateReg;
			const ss::site::show::OverlayView Ov = ss::site::show::OverlayOf(Net, E, Closes - 10.0);
			if (!Ov.Actual && Ov.Cents >= 30000 && E.Template >= 0 && static_cast<size_t>(E.Template) < Net.Templates().size())
			{
				Tmpl = Net.Templates()[static_cast<size_t>(E.Template)].Id;
				At = Closes - 10.0;
				break;
			}
		}
		if (Tmpl.empty())
		{
			std::printf("  bell: no RunItTwice event projected $300 short in three days: the no-League bell check is skipped\n");
		}
		else
		{
			ss::life::State L = Fresh(Mon);
			Expect(play::Star(L.SitePlay, Tmpl, true).empty(), "bell: a template stars");
			play::Step(L.SitePlay, L, L.Sites, nullptr, L.Sponsors, Seed, At - 1.0, At);
			Expect(L.SitePlay.Rt.BellKeys.size() == 1 && L.SitePlay.Rt.L.Month == -1, "bell: a starred event projected $300 short rings with no League month (" + Tmpl + ")");
		}
	}
	// The League's close against a world book: a Placement month never relegates, the next month at the bottom does, and a
	// month a Team RiverLine deal covered (it ended on the 20th) with no RunItTwice entry is on leave: the division kept (W7).
	{
		const double T0 = (Dec1 + 2) * DayMin + 19.0 * 60.0; // Thursday, December 3, 7 PM
		ss::world::World W;
		W.Create(4242u, T0);
		ss::life::State L = Fresh(T0);
		play::Book& B = L.SitePlay;
		Pay(L, net::SiteId::RunItTwice, IdAt("runittwice.the-sweat", T0 + 30.0), 1100, FeeOn(net::SiteId::RunItTwice, 1100), T0);
		B.Rt.L.Div = 2;
		play::StepOut Out = play::Step(B, L, L.Sites, &W, L.Sponsors, 99, T0, T0 + 5.0);
		if (!Out.Roster || Out.Npcs.size() < 14)
		{
			Expect(false, "league close: a group of 14 or more drawn from a fresh world (" + std::to_string(Out.Npcs.size()) + ")");
			return;
		}
		W.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
		LeagueMembersAt(W, 500);
		RitBlank(L, IdAt("runittwice.the-sweat", T0 + 30.0), T0 + 200.0);
		const int Jan1 = world_first_of_next(play::PlayDay(T0));
		Out = play::Step(B, L, L.Sites, &W, L.Sponsors, 99, T0 + 300.0, play::PlayDayStart(Jan1) + 1.0);
		Expect(B.Rt.Hist.size() == 1 && B.Rt.Hist[0].Rank == B.Rt.Hist[0].Of && B.Rt.Hist[0].Move == 0 && B.Rt.L.Div == 2 && !B.Rt.L.Placement,
			"league close: last in a Placement month and kept in Steel");
		W.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
		LeagueMembersAt(W, 500);
		const double Jan5 = play::PlayDayStart(Jan1 + 4) + 19.0 * 60.0;
		Pay(L, net::SiteId::RunItTwice, IdAt("runittwice.the-sweat", Jan5 + 30.0), 1100, FeeOn(net::SiteId::RunItTwice, 1100), Jan5);
		RitBlank(L, IdAt("runittwice.the-sweat", Jan5 + 30.0), Jan5 + 200.0);
		const int Feb1 = world_first_of_next(Jan1);
		Out = play::Step(B, L, L.Sites, &W, L.Sponsors, 99, Jan5 + 300.0, play::PlayDayStart(Feb1) + 1.0);
		Expect(B.Rt.Hist.size() == 2 && B.Rt.Hist[1].Move == -1 && B.Rt.L.Div == 1, "league close: last in a month after Placement goes down a division");
		W.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
		LeagueMembersAt(W, 500);
		L.Sponsors.Contracts.push_back(RiverLineDeal(play::PlayDayStart(Feb1) - DayMin, play::PlayDayStart(Feb1 + 19)));
		play::Step(B, L, L.Sites, &W, L.Sponsors, 99, play::PlayDayStart(Feb1) + 1.0, play::PlayDayStart(Feb1 + 9) + 1.0);
		L.Sponsors.Contracts.back().St = ss::sponsor::State::Ended; // the deal ran to the 20th
		const int Mar1 = world_first_of_next(Feb1);
		const std::string Before = Lines(L);
		play::Step(B, L, L.Sites, &W, L.Sponsors, 99, play::PlayDayStart(Feb1 + 9) + 1.0, play::PlayDayStart(Mar1) + 1.0);
		Expect(B.Rt.Hist.size() == 3 && B.Rt.Hist[2].Move == 0 && B.Rt.L.Div == 1, "league close: a month the deal covered with no RunItTwice entry is on leave (Iron kept)");
		const std::string After = Lines(L);
		play::Step(B, L, L.Sites, &W, L.Sponsors, 99, play::PlayDayStart(Feb1 + 9) + 1.0, play::PlayDayStart(Mar1) + 1.0);
		Expect(After != Before && Lines(L) == After, "league close: the 1st's step again over an overlapping window changes nothing");
	}
}
// ------------------------------------------------------------------ the G2 fix round (g2f-siteplay: M1, M2, M4, L6, L11)

/** A bust of the player on Ignite through the per-hand hook (Npc: the world's id, -1 a screen name). */
void FixBust(ss::life::State& L, const std::string& Id, const std::string& By, int Npc, double At)
{
	play::HandNote N;
	N.Paid = true;
	N.EventId = Id;
	N.HandId = Id + "#" + By + ":" + std::to_string(static_cast<long long>(At));
	N.BustedBy = By;
	N.BustedByNpc = Npc;
	play::OnHandPaid(L.SitePlay, L, net::SiteId::Ignite, N, At);
}

/** The player knocks a name out on Ignite through the per-hand hook (review-g2f-siteplay: a count both ways). */
void FixKnock(ss::life::State& L, const std::string& Id, const std::string& Who, int Npc, double At)
{
	play::HandNote N;
	N.Paid = true;
	N.EventId = Id;
	N.HandId = Id + "#ko:" + Who + ":" + std::to_string(static_cast<long long>(At));
	N.Knocked.push_back(Who);
	N.KnockedNpcs.push_back(Npc);
	play::OnHandPaid(L.SitePlay, L, net::SiteId::Ignite, N, At);
}

const play::Beef* FixBeefRow(const play::Book& B, const std::string& Name)
{
	for (const play::Beef& Bf : B.Ig.Beefs)
	{
		if (Bf.Name == Name)
		{
			return &Bf;
		}
	}
	return nullptr;
}

void G2Fixes()
{
	const double Mon = (Dec1 + 6) * DayMin + 360.0; // Monday, December 7, 6 AM
	const Chips Fee = FeeOn(net::SiteId::Ignite, 110);
	// M1: the Beef counts the window past five names. A second bust after nine others still makes the Beef (the old list kept
	// five and evicted the stalest, so the first was gone); RIVALS shows five, the Beef first.
	{
		ss::life::State L = Fresh(Mon);
		const std::string Id = IdAt("ignite.flip-go", Mon + 630.0);
		Pay(L, net::SiteId::Ignite, Id, 110, Fee, Mon + 600.0);
		FixBust(L, Id, "first_one", 1, Mon + 601.0);
		for (int K = 0; K < 9; ++K)
		{
			FixBust(L, Id, "member_" + std::to_string(K), 10 + K, Mon + 602.0 + K);
		}
		FixBust(L, Id, "first_one", 1, Mon + 620.0);
		const play::View V = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 621.0);
		Expect(V.Ig.BeefName == "first_one" && L.SitePlay.Ig.Beefs.size() == 10 && V.Ig.Beefs.size() == 5 && V.Ig.Beefs[0].Name == "first_one" && V.Ig.Beefs[0].TheyKo == 2,
			"beef (M1): a second bust after nine others makes the Beef; the window keeps all ten names, RIVALS shows five with the Beef first");
		// A screen name (not a world member) takes no row once five are listed: it can't sit with the player again.
		FixBust(L, Id, "drifter_x", -1, Mon + 622.0);
		Expect(L.SitePlay.Ig.Beefs.size() == 10 && !FixBeefRow(L.SitePlay, "drifter_x"), "beef (M1): a screen name takes no row once five names are listed");
		ss::life::State Short = Fresh(Mon);
		Pay(Short, net::SiteId::Ignite, Id, 110, Fee, Mon + 600.0);
		FixBust(Short, Id, "drifter_x", -1, Mon + 601.0);
		Expect(FixBeefRow(Short.SitePlay, "drifter_x") != nullptr, "beef (M1): while the list is short of five, a screen name shows on RIVALS");
		// (review-g2f-siteplay) The Beef heads RIVALS even when a name the player keeps busting has the bigger count both ways
		// (the Beef counts only the busts of the player): busy_one is 3-1, the Beef 0-2.
		ss::life::State Both = Fresh(Mon);
		Pay(Both, net::SiteId::Ignite, Id, 110, Fee, Mon + 600.0);
		FixBust(Both, Id, "the_beef", 3, Mon + 601.0);
		FixBust(Both, Id, "busy_one", 4, Mon + 602.0);
		FixBust(Both, Id, "the_beef", 3, Mon + 603.0);
		for (int K = 0; K < 3; ++K)
		{
			FixKnock(Both, Id, "busy_one", 4, Mon + 604.0 + K);
		}
		const play::View Vb = play::ViewOf(Both.SitePlay, Both, Both.Sites, nullptr, net::SiteId::Ignite, Mon + 610.0);
		Expect(Vb.Ig.BeefName == "the_beef" && Vb.Ig.Beefs.size() == 2 && Vb.Ig.Beefs[0].Name == "the_beef" && Vb.Ig.Beefs[1].Name == "busy_one" &&
				   Vb.Ig.Beefs[1].YouKo == 3 && Vb.Ig.Beefs[1].TheyKo == 1,
			"beef (M1): RIVALS lists the Beef first even when another name has more busts both ways");
	}
	// M1: full at 24 names, the lowest count makes way first, then the stalest: a name with two busts outlasts the singles
	// even when it's the stalest; the window still drops it 30 days after its last bust; the save carries all 24 and back.
	{
		ss::life::State L = Fresh(Mon);
		const int Day0 = play::PlayDay(Mon);
		Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", Mon + 630.0), 110, Fee, Mon + 600.0);
		FixBust(L, "e0", "rival", 7, Mon + 601.0);
		FixBust(L, "e0", "rival", 7, Mon + 602.0);
		for (int K = 1; K <= 23; ++K)
		{
			FixBust(L, "e" + std::to_string(K), "single_" + std::to_string(K), 100 + K, play::PlayDayStart(Day0 + K) + 600.0);
		}
		Expect(L.SitePlay.Ig.Beefs.size() == 24, "beef (M1): the window counts 24 names");
		const std::string Before = Lines(L);
		const ss::life::State Back = Reload(L);
		Expect(Lines(Back) == Before && Back.SitePlay.Ig.Beefs.size() == 24, "beef (M1): 24 beef lines round-trip byte for byte");
		ss::life::State Over = Reload(L);
		play::ReadLine({"site", "beef", "ignite", "one_too_many", "1", "0", std::to_string(Day0 + 23)}, Over);
		Expect(Over.SitePlay.Ig.Beefs.size() == 24 && !FixBeefRow(Over.SitePlay, "one_too_many"), "beef (M1): a save's 25th beef line isn't read (the window's bound, S7)");
		size_t BeefBytes = 0;
		for (size_t At = Before.find("site\tbeef\t"); At != std::string::npos; At = Before.find("site\tbeef\t", At + 1))
		{
			BeefBytes += Before.find('\n', At) + 1 - At;
		}
		FixBust(L, "e24", "single_24", 124, play::PlayDayStart(Day0 + 24) + 600.0);
		Expect(L.SitePlay.Ig.Beefs.size() == 24 && FixBeefRow(L.SitePlay, "rival") && !FixBeefRow(L.SitePlay, "single_1") && FixBeefRow(L.SitePlay, "single_24"),
			"beef (M1): a 25th name evicts the lowest count, then the stalest (single_1), never the two-bust rival though it's older");
		const play::View V = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite, play::PlayDayStart(Day0 + 24) + 601.0);
		Expect(V.Ig.BeefName == "rival", "beef (M1): the two-bust rival is still the Beef 24 days on");
		FixBust(L, "e31", "single_31", 131, play::PlayDayStart(Day0 + 31) + 600.0);
		Expect(!FixBeefRow(L.SitePlay, "rival") && FixBeefRow(L.SitePlay, "single_2") && play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite,
			play::PlayDayStart(Day0 + 31) + 601.0).Ig.BeefName.empty(), "beef (M1): 31 days after its last bust the rival leaves the window, and nobody is the Beef");
		BeefFullBytes = BeefBytes;
		std::printf("  beef (M1): 24 names in the window write %zu B of beef lines\n", BeefBytes);
		Expect(BeefBytes <= 24 * 48, "beef (M1): the window's 24 beef lines stay under about 1.1 KB");
	}
	// M2 (W5): each Cracker's Odds are the draw's own table. The rows' chances sum to 1, the EV is the spec's (162.5, 820,
	// 4,100), and 6,000 keyed draws of each land on the rows at their chances (within 4 sigma), never off them.
	{
		const double Ev[3] = {162.5, 820.0, 4100.0};
		for (int T = 0; T < 3; ++T)
		{
			const play::CrackerOddsView O = play::CrackerOdds(T);
			double Sum = 0.0;
			std::map<int64_t, double> SparksP;
			double TicketP = 0.0;
			std::string TicketId;
			std::map<std::pair<int, int>, double> LookP;
			for (const play::CrackerOddsRow& R : O.Rows)
			{
				Sum += R.Part == 0 ? R.P : 0.0;
				if (R.Part == 0)
				{
					SparksP[R.Sparks] = R.P;
				}
				else if (R.Part == 1)
				{
					TicketP = R.P;
					TicketId = R.TicketId;
				}
				else
				{
					LookP[{R.LookSlot, R.Look}] = R.P;
				}
			}
			std::map<int64_t, int> Got;
			int Tickets = 0, Looks = 0, Off = 0;
			std::map<std::pair<int, int>, int> GotLook;
			const int N = 6000;
			for (int K = 0; K < N; ++K)
			{
				play::Box X;
				X.Id = "odds:" + std::to_string(T) + ":" + std::to_string(K);
				X.Tier = static_cast<int8_t>(T);
				const play::OpenView Ov = play::CrackerContents(4242, X);
				Got[Ov.Sparks] += 1;
				Off += SparksP.count(Ov.Sparks) ? 0 : 1;
				if (!Ov.TicketId.empty())
				{
					++Tickets;
					Off += Ov.TicketId == TicketId ? 0 : 1;
				}
				if (Ov.LookSlot >= 0)
				{
					++Looks;
					GotLook[{Ov.LookSlot, Ov.Look}] += 1;
					Off += LookP.count({Ov.LookSlot, Ov.Look}) ? 0 : 1;
				}
			}
			auto Near = [N](int Count, double P) { return std::fabs(Count - N * P) <= 4.0 * std::sqrt(std::max(1.0, N * P * (1.0 - P))); };
			bool Rows = Near(Tickets, TicketP) && Near(Looks, O.LookP);
			for (const auto& Kv : SparksP)
			{
				Rows = Rows && Near(Got[Kv.first], Kv.second);
			}
			for (const auto& Kv : LookP)
			{
				Rows = Rows && Near(GotLook[Kv.first], Kv.second);
			}
			Expect(std::fabs(Sum - 1.0) < 1e-12 && std::fabs(O.EvSparks - Ev[T]) < 1e-6 && !O.From.empty() && O.Name == std::string(T == 0 ? "Paper Cracker" : T == 1 ? "Neon Cracker" : "Gold Cracker"),
				std::string("crackers (M2): ") + O.Name + "'s Odds: the Sparks rows sum to 1 and the EV is the spec's");
			std::string Seen;
			for (const auto& Kv : SparksP)
			{
				Seen += " " + std::to_string(Kv.first) + ":" + std::to_string(Got[Kv.first]) + "/" + std::to_string(static_cast<int>(std::lround(N * Kv.second)));
			}
			for (const auto& Kv : LookP)
			{
				Seen += " look" + std::to_string(Kv.first.first) + "." + std::to_string(Kv.first.second) + ":" + std::to_string(GotLook[Kv.first]);
			}
			std::printf("  crackers (M2): %s, %d keyed draws:%s, off the table %d\n", O.Name.c_str(), N, Seen.c_str(), Off);
			Expect(Off == 0 && Rows, std::string("crackers (M2): ") + O.Name + "'s keyed draws land on its Odds rows at their chances (" + std::to_string(Tickets) + " tickets, " +
				std::to_string(Looks) + " looks of " + std::to_string(N) + ")");
		}
		ss::life::State L = Fresh(Mon);
		const play::View V = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 600.0);
		Expect(V.Ig.CrackerOdds[1].Rows.size() == play::CrackerOdds(1).Rows.size() && V.Ig.CrackerOdds[2].EvSparks == play::CrackerOdds(2).EvSparks,
			"crackers (M2): the view carries the three Odds tables, even before the book starts");
		// The Spark Wheel draws by the same unit (its Odds are WedgeView::P): 6,000 play days of each ring at each band land
		// on the wedges at their chances. The raw Fnv1a unit (SitePlayUnit) is printed beside it: it clusters by the day's
		// last digit, which is why the prize draws mix it (SitePlayDraw).
		int Bad = 0;
		std::string Raw;
		for (int Ring = 0; Ring < 3; ++Ring)
		{
			for (int8_t Band = 0; Band < 3; ++Band)
			{
				const std::vector<std::pair<int, double>>& Table = play::WheelTable(Ring, Band);
				auto Wedge = [&Table](double U) {
					size_t W = Table.size() - 1;
					for (size_t K = 0; K < Table.size(); ++K)
					{
						U -= Table[K].second;
						if (U < 0.0)
						{
							W = K;
							break;
						}
					}
					return W;
				};
				std::vector<int> Unmixed(Table.size()), Mixed(Table.size());
				const int N = 6000;
				for (int D = 0; D < N; ++D)
				{
					const std::string Key = "ignite:wheel:" + std::to_string(Dec1 + D);
					Unmixed[Wedge(detail::SitePlayUnit(4242, Key))] += 1;
					Mixed[Wedge(detail::SitePlayDraw(4242, Key))] += 1;
				}
				for (size_t K = 0; K < Table.size(); ++K)
				{
					const double P = Table[K].second;
					Bad += std::fabs(Mixed[K] - N * P) <= 4.0 * std::sqrt(std::max(1.0, N * P * (1.0 - P))) ? 0 : 1;
					if (Ring == 0 && Band == 0)
					{
						Raw += " " + std::to_string(Table[K].first) + ":" + std::to_string(Unmixed[K]) + "->" + std::to_string(Mixed[K]) + "/" +
							std::to_string(static_cast<int>(std::lround(N * P)));
					}
				}
			}
		}
		std::printf("  wheel (M2): GLOW at band A over 6,000 play days, raw -> mixed / expected:%s\n", Raw.c_str());
		Expect(Bad == 0, "wheel (M2): 6,000 days of each ring at each band land on the wedges at their chances (" + std::to_string(Bad) + " wedges off)");
	}
	// M4: a claim whose value waits on the Guest List reads Held (never just CLAIMED): a Dare's Sparks and the spin's on
	// "Sparks owed", a Wristband ticket on its own hold; the flags survive a reload and go when the holds clear.
	{
		ss::life::State L = Fresh(Mon);
		play::Book& B = L.SitePlay;
		const double At = Mon + 600.0;
		for (int K = 0; K < 3; ++K)
		{
			Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", At + 30.0 + K), 110, Fee, At + K);
		}
		const int Day = play::PlayDay(At);
		Expect(!play::Grant(B, L, net::SiteId::Ignite, play::Kind::Ticket, "Night Glow $2.20 ticket", 220, "ignite.night-glow|fix:m4", Day, At + 5.0),
			"held (M4): a $2.20 ticket past the Room waits (the Guest List is senior from here)");
		B.Ig.D.Prog = {20, 20, 20};
		B.Ig.S.BaseMonth = 7000; // Wristband T10 (a Flip & Run ticket)
		Expect(play::ClaimDare(B, L, 0, At + 6.0).empty() && play::ClaimTier(B, L, 10, At + 7.0).empty() && play::Spin(B, L, 4242, At + 8.0).empty(),
			"held (M4): the Dare, the T10 ticket and the spin are claimed while the Guest List waits");
		const play::View V = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, At + 9.0);
		Expect(V.Ig.Dares[0].Claimed && V.Ig.Dares[0].Held && !V.Ig.Dares[1].Held && V.Ig.Tiers[9].Claimed && V.Ig.Tiers[9].Held && !V.Ig.Tiers[4].Held && V.Ig.Spun &&
				V.Ig.SpunHeld,
			"held (M4): the Dare, the T10 tier and the spin read Held (ON THE GUEST LIST), not CLAIMED");
		const ss::life::State Back = Reload(L);
		const play::View Vb = play::ViewOf(Back.SitePlay, Back, Back.Sites, nullptr, net::SiteId::Ignite, At + 9.0);
		Expect(Vb.Ig.Dares[0].Held && Vb.Ig.Tiers[9].Held && Vb.Ig.SpunHeld && Lines(Back) == Lines(L), "held (M4): the Held flags survive a reload, byte for byte");
		double Now = At + 10.0;
		for (int K = 0; K < 400 && !B.Holds.empty(); ++K)
		{
			Now += 1.0;
			Pay(L, net::SiteId::Ignite, IdAt("ignite.spark-jackpot", Now + 30.0), 550, FeeOn(net::SiteId::Ignite, 550), Now);
		}
		const play::View Va = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, Now + 1.0);
		Expect(B.Holds.empty() && Va.Ig.Dares[0].Claimed && !Va.Ig.Dares[0].Held && Va.Ig.Tiers[9].Claimed && !Va.Ig.Tiers[9].Held && Va.Ig.Spun && !Va.Ig.SpunHeld,
			"held (M4): once the Guest List clears they read claimed and paid");
		Expect(play::PlayDay(Now) == Day, "held (M4): it all cleared the same play day (the check reads today's card)");
		// (review-g2f-siteplay) A later ticket that waits is its own hold: the T10 ticket and the spin, paid, don't read Held for it.
		Expect(!play::Grant(B, L, net::SiteId::Ignite, play::Kind::Ticket, "The Big Wave $11 ticket", 1100, "ignite.big-wave|review:m4", Day, Now + 2.0) &&
				   B.Holds.size() == 1,
			"held (M4): an $11 ticket past the Room waits on its own hold");
		const play::View Vl = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, Now + 3.0);
		Expect(Vl.Ig.Tiers[9].Claimed && !Vl.Ig.Tiers[9].Held && !Vl.Ig.Dares[0].Held && Vl.Ig.Spun && !Vl.Ig.SpunHeld,
			"held (M4): another claim's waiting ticket never marks a paid tier or spin Held (each hold by its own key)");
	}
	// M4 (review-g2f-siteplay): Held is per claim. A Dare paid before the Guest List waited reads CLAIMED beside one that queued
	// behind it; a spin that lands on a ticket while the list waits reads Held through the ticket's own hold, not the owed bit.
	{
		ss::life::State L = Fresh(Mon);
		play::Book& B = L.SitePlay;
		const double At = Mon + 600.0;
		for (int K = 0; K < 3; ++K)
		{
			Pay(L, net::SiteId::Ignite, IdAt("ignite.flip-go", At + 30.0 + K), 110, Fee, At + K);
		}
		const int Day = play::PlayDay(At);
		B.Ig.D.Prog = {20, 20, 20};
		const bool Paid0 = play::ClaimDare(B, L, 0, At + 4.0).empty() && B.Holds.empty();
		const bool Waits = !play::Grant(B, L, net::SiteId::Ignite, play::Kind::Ticket, "Night Glow $2.20 ticket", 220, "ignite.night-glow|review:m4b", Day, At + 5.0);
		const bool Queued1 = play::ClaimDare(B, L, 1, At + 6.0).empty();
		// A world seed whose spin today, on the lit ring at this band, lands on a ticket wedge (the draw is SitePlayDraw's).
		const play::View Vs = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, At + 7.0);
		const std::vector<std::pair<int, double>>& Table = play::WheelTable(Vs.Ig.RingsLit - 1, Vs.Ig.Band);
		uint64_t Seed = 0;
		for (uint64_t S = 1; S < 100000 && Seed == 0 && Vs.Ig.RingsLit > 0; ++S)
		{
			double U = detail::SitePlayDraw(S, "ignite:wheel:" + std::to_string(Day));
			int Prize = Table.back().first;
			for (const std::pair<int, double>& Wg : Table)
			{
				U -= Wg.second;
				if (U < 0.0)
				{
					Prize = Wg.first;
					break;
				}
			}
			Seed = Prize < 0 ? S : 0;
		}
		const bool Spun = Seed != 0 && play::Spin(B, L, Seed, At + 8.0).empty();
		const play::View V = play::ViewOf(B, L, L.Sites, nullptr, net::SiteId::Ignite, At + 9.0);
		Expect(Paid0 && Waits && Queued1 && V.Ig.Dares[0].Claimed && !V.Ig.Dares[0].Held && V.Ig.Dares[1].Claimed && V.Ig.Dares[1].Held && !V.Ig.Dares[2].Claimed,
			"held (M4): a Dare paid at once reads CLAIMED beside one whose Sparks queued behind the Guest List");
		Expect(Spun && V.Ig.Spun && V.Ig.SpunHeld && (B.Ig.S.WheelRing & 16) == 0 && B.Holds.size() == 3,
			"held (M4): a spin that lands on a ticket while the Guest List waits reads Held by its own hold");
	}
	// L6: League Night's and INCOMING CALLOUT's seen marks are saved (one dated done key each, the older dropped), never written
	// into a book with no RunItTwice state, written once however often the page asks.
	{
		ss::life::State L = Fresh(Mon);
		const size_t Done0 = L.Sites.Done.size();
		Expect(!play::SeeLeagueNight(L, 24321, Mon) && !play::SeeIncomingCallout(L, 194577, Mon) && L.Sites.Done.size() == Done0,
			"seen (L6): no mark is written into a save with no RunItTwice book (S10)");
		Pay(L, net::SiteId::RunItTwice, IdAt("runittwice.the-sweat", Mon + 630.0), 1100, FeeOn(net::SiteId::RunItTwice, 1100), Mon + 600.0);
		const size_t Done1 = L.Sites.Done.size();
		const bool First = play::SeeLeagueNight(L, 24321, Mon + 601.0);
		const bool Again = play::SeeLeagueNight(L, 24321, Mon + 602.0);
		const bool Call = play::SeeIncomingCallout(L, 194577, Mon + 603.0);
		const play::View V = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::RunItTwice, Mon + 604.0);
		Expect(First && !Again && Call && L.Sites.Done.size() == Done1 + 2 && V.Rt.NightSeen == 24321 && V.Rt.IncomingSeen == 194577,
			"seen (L6): League Night's and the incoming Callout's marks are written once and read back by the view");
		const bool Next = play::SeeLeagueNight(L, 24322, Mon + 605.0);
		const play::View V2 = play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::RunItTwice, Mon + 606.0);
		Expect(Next && L.Sites.Done.size() == Done1 + 2 && V2.Rt.NightSeen == 24322 && V2.Rt.IncomingSeen == 194577 &&
				   play::ViewOf(L.SitePlay, L, L.Sites, nullptr, net::SiteId::Ignite, Mon + 606.0).Rt.NightSeen == -1,
			"seen (L6): the next month's mark replaces the last (one key of each kind)");
	}
	// L11: one 75% welcome window: the Governor's ceiling is 0.75 exactly where site::WelcomeWindow says (SiteMoney.cpp's).
	{
		const double T0 = Dec1 * DayMin + 6.0 * 60.0 + 600.0;
		ss::life::State Wl = Fresh(T0);
		site::Account* A = site::AccountOf(Wl.Sites, net::SiteId::Ignite);
		A->BonusPending = 2000;
		A->BonusExpires = T0 + 60.0 * DayMin;
		int Same = 0, Of = 0;
		for (const double At : {T0 - 1.0, T0, T0 + 30.0 * DayMin, T0 + 60.0 * DayMin, T0 + 60.0 * DayMin + 1.0})
		{
			++Of;
			Same += (detail::SitePlayCeiling(Wl, net::SiteId::Ignite, At) == 0.75) == site::WelcomeWindow(*A, At) ? 1 : 0;
		}
		Expect(Same == Of && detail::SitePlayCeiling(Wl, net::SiteId::Ignite, T0 - 1.0) == 0.50 && detail::SitePlayCeiling(Wl, net::SiteId::RunItTwice, T0) == 0.50,
			"welcome (L11): the Governor's 75% ceiling is site::WelcomeWindow's window, to the minute (one definition)");
	}
}
} // namespace core_seg

void SegCore()
{
	core_seg::Catalogs();
	core_seg::LeagueRule();
	core_seg::Governor();
	core_seg::Loops();
	core_seg::Caps();
	core_seg::LeagueWorld();
	core_seg::Hardening();
	core_seg::G2Fixes();
	core_seg::Copy();
	core_seg::IgnitePatterns();
	core_seg::League();
	core_seg::RitProfiles();
}
} // namespace siteplay_test

int main(int Argc, char** Argv)
{
	siteplay_test::Long = Argc > 1 && std::strcmp(Argv[1], "long") == 0;
	const auto T0 = std::chrono::steady_clock::now();
	if (Argc > 1 && std::strcmp(Argv[1], "beef") == 0)
	{
		// G2 fix M1's measure (not a suite mode): update3_siteplay's career run long, every bust logged, the book's Beef followed.
		siteplay_test::SegBeef(Argc > 2 ? std::atoi(Argv[2]) : 60, Argc > 3 ? std::atoi(Argv[3]) : 3);
		std::printf("site play beef: %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
		return 0;
	}
	siteplay_test::SegCore();
	siteplay_test::SegSession();
	siteplay_test::SegFuse();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("site play tests: %d checks, %d failed in %.1fs\n", siteplay_test::Checks, siteplay_test::Failures, Seconds);
	return siteplay_test::Failures == 0 ? 0 : 1;
}
