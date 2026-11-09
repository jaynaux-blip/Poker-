// site_play: FUSE (update 3 SPEC §2.6 F7, §7.7; core-fuse's file). The tables at the chosen edge (the sum of p is 1 and of
// p x m is 3 x (1 - edge) per tier, x5 and up as the spec prints them), FeeOf, PrizePoolCents == m x buy-in at every row
// and tier, the payout's split, the ids, the keyed draw (reproducible, uniform enough, per tier ordinal with tickets
// counted), the seats, the record and its save line; then LIGHT IT in a Session: the fee the rake hooks see, the save
// committed at entry before the reveal, the ticket path, the refusals (the strike gate under either clause, two at once,
// no ticket, no account) changing nothing, FUSE never in History, the world's results or NPC memories and scoring no
// Season points, the session-paid split, the reveal's state, the dev force; and the calibration bands from S-J's place
// odds at the lead's knobs. "long" adds the place-odds simulation (2,000 games a hero) and every style at the 3-max table.
// Called from siteplay_main.cpp (the kit is SitePlaySeg.h).
#include "SitePlaySeg.h"

#include "CopyLint.h"
#include "Trademarks.h"

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Grading.h"
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace siteplay_test
{
namespace fuse_seg
{
using ss::Chips;

void Note(const std::string& What)
{
	std::printf("  fuse: %s\n", What.c_str());
}

std::string Num(double V, int Digits)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.*f", Digits, V);
	return Buf;
}

std::string Pct(double V)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%+.1f%%", 100.0 * V);
	return Buf;
}

/** "long" on the command line (siteplay_test long): main() takes no arguments into the segments, so read the CRT's. */
bool LongMode()
{
	for (int I = 1; I < __argc; ++I)
	{
		if (__argv[I] && std::strcmp(__argv[I], "long") == 0)
		{
			return true;
		}
	}
	return false;
}

double SecondsSince(std::chrono::steady_clock::time_point T0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
}

// ------------------------------------------------------------------ the analytic side

/** A place's share of the pool at a multiplier (fuse::Payout over the pool). */
double ShareOf(int Tier, int M, int Place)
{
	const std::array<Chips, 3> P = ss::fuse::Payout(Tier, M);
	const double Pool = static_cast<double>(M) * static_cast<double>(ss::fuse::BuyInOf(Tier));
	return Pool > 0 ? static_cast<double>(P[static_cast<size_t>(Place - 1)]) / Pool : 0.0;
}

/** ROI (in buy-ins) from place odds at a tier: E over the table of m x the place's share, less the buy-in. */
double RoiOf(double P1, double P2, double P3, int Tier)
{
	double Ev = 0.0;
	for (const ss::fuse::Row& R : ss::fuse::Table())
	{
		const int M = ss::fuse::MOf(R, Tier);
		Ev += ss::fuse::POf(R, Tier) * M * (P1 * ShareOf(Tier, M, 1) + P2 * ShareOf(Tier, M, 2) + P3 * ShareOf(Tier, M, 3));
	}
	return Ev - 1.0;
}

/** S-J's measured place odds at the chosen knobs (fillers "high", 300 chips; S-J.md §3, runs/batch3.txt and batch4.txt):
 *  coach-best over 100,000 paired games, every style over 20,000; and events a world hour at one table, to the hero's
 *  finish and with the table played out. */
struct Measured
{
	const char* Who;
	double P1, P2, P3;
	double PerHour, PerHourFull;
	bool Autoplay;
};
const Measured kOdds[] = {
	{"coach-best", 0.3992, 0.3207, 0.2801, 7.39, 6.31, false},
	{"Autopilot", 0.3283, 0.3275, 0.3442, 8.10, 6.56, true},
	{"Push/Fold", 0.3567, 0.3002, 0.3431, 9.42, 7.30, true},
	{"Maniac", 0.3789, 0.2609, 0.3603, 9.41, 7.18, true},
	{"GTO", 0.3575, 0.2994, 0.3432, 9.45, 7.31, true},
	{"LAG", 0.3407, 0.3055, 0.3538, 8.49, 6.76, true},
	{"ICM Shark", 0.3365, 0.3094, 0.3542, 9.45, 7.26, true},
	{"Balanced Reg", 0.3257, 0.3250, 0.3493, 8.19, 6.58, true},
	{"TAG", 0.3170, 0.3255, 0.3575, 8.33, 6.63, true},
	{"Exploitative", 0.3170, 0.3255, 0.3575, 8.33, 6.63, true},
	{"Nit", 0.2937, 0.3489, 0.3574, 8.12, 6.51, true},
};
/** S-J's MTT grind on Ignite at the same stakes (coach-best, real listings, EV a world hour at one table; S-J.md §4). */
const double kMttPerHour[3] = {1.32, 4.14, 8.40};
constexpr double kRentCents = 107500.0; // life::MonthlyRentCents ($1,075)

void Tables()
{
	const double Edge = ss::fuse::Edge();
	Expect(std::fabs(Edge - 0.10) < 1e-12, "fuse: the house edge is the lead's 10%");
	Expect(ss::fuse::FeeOf(0) == 11 && ss::fuse::FeeOf(1) == 33 && ss::fuse::FeeOf(2) == 55, "fuse: FeeOf at 10% is 11, 33 and 55 cents");
	Expect(ss::fuse::BuyInOf(0) == 110 && ss::fuse::BuyInOf(1) == 330 && ss::fuse::BuyInOf(2) == 550 && ss::fuse::BuyInOf(3) == 0 && ss::fuse::BuyInOf(-1) == 0,
		"fuse: three tiers, $1.10, $3.30 and $5.50");
	Expect(ss::fuse::TopOf(0) == 1000 && ss::fuse::TopOf(1) == 250 && ss::fuse::TopOf(2) == 250, "fuse: the top is x1000 at $1.10 and x250 above");
	Expect(ss::fuse::StartingStack() == 300 && std::string(ss::fuse::Fillers()) == "high", "fuse: the lead's calibration: 300 chips against the \"high\" fillers");
	bool Exact = true, Shape = true;
	for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
	{
		double Sum = 0.0, Mean = 0.0;
		for (const ss::fuse::Row& R : ss::fuse::Table())
		{
			const double P = ss::fuse::POf(R, Tier);
			Shape = Shape && P > 0.0 && P < 1.0 && R.Split == (R.M >= 25);
			Sum += P;
			Mean += P * ss::fuse::MOf(R, Tier);
		}
		Exact = Exact && std::fabs(Sum - 1.0) < 1e-12 && std::fabs(Mean - 3.0 * (1.0 - Edge)) < 1e-12;
	}
	Expect(Exact && Shape, "fuse: every tier's table sums to 1 and its mean is 3 x (1 - edge) exactly");
	// The x5-and-up rows as §2.6 F7 prints them; x2 and x3 re-solved for 10% (S-J.md §5's lead option).
	const std::array<ss::fuse::Row, 7>& T = ss::fuse::Table();
	auto Near = [](double A, double B) { return std::fabs(A - B) < 1e-9; };
	const bool Fixed = T[2].M == 5 && Near(T[2].P[0], 0.065) && Near(T[2].P[1], 0.065) && T[3].M == 10 && Near(T[3].P[0], 0.02) && T[4].M == 25 && Near(T[4].P[0], 0.004) &&
		T[5].M == 100 && Near(T[5].P[0], 0.0005) && Near(T[5].P[1], 0.0006) && T[6].M == 1000 && Near(T[6].P[0], 0.00005) && Near(T[6].P[1], 0.00005);
	const bool Solved = T[0].M == 2 && T[1].M == 3 && Near(T[0].P[0], 0.75635) && Near(T[1].P[0], 0.15410) && Near(T[0].P[1], 0.72855) && Near(T[1].P[1], 0.18180);
	Expect(Fixed && Solved, "fuse: x5 and up as the spec prints them; x2 75.635% / x3 15.410% at $1.10, 72.855% / 18.180% above");
	// At any edge: 8% gives the spec's printed table, 12% S-J's (S-J.md §5); each column sums to 1 and to 3 x (1 - edge).
	const std::array<ss::fuse::Row, 7> At8 = ss::fuse::TableAt(0.08), At12 = ss::fuse::TableAt(0.12);
	bool AnyEdge = Near(At8[0].P[0], 0.69635) && Near(At8[1].P[0], 0.21410) && Near(At8[0].P[1], 0.66855) && Near(At8[1].P[1], 0.24180) &&
		Near(At12[0].P[0], 0.81635) && Near(At12[1].P[0], 0.09410) && Near(At12[0].P[1], 0.78855) && Near(At12[1].P[1], 0.12180);
	for (const double E : {0.08, 0.09, 0.11, 0.12})
	{
		const std::array<ss::fuse::Row, 7> At = ss::fuse::TableAt(E);
		for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
		{
			double Sum = 0.0, Mean = 0.0;
			for (size_t I = 0; I < At.size(); ++I)
			{
				Sum += ss::fuse::POf(At[I], Tier);
				Mean += ss::fuse::POf(At[I], Tier) * ss::fuse::MOf(At[I], Tier);
				AnyEdge = AnyEdge && (I < 2 || (At[I].M == T[I].M && At[I].P[0] == T[I].P[0] && At[I].P[1] == T[I].P[1]));
			}
			AnyEdge = AnyEdge && std::fabs(Sum - 1.0) < 1e-12 && std::fabs(Mean - 3.0 * (1.0 - E)) < 1e-12;
		}
	}
	Expect(AnyEdge, "fuse: the table at any edge: 8% is the spec's (69.635% / 21.410%), 12% S-J's (81.635% / 9.410%); x5 and up never move");
	// The payout: winner takes all to x10, 80/12/8 from x25, in whole cents that sum to the pool.
	bool Paid = true;
	for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
	{
		for (const ss::fuse::Row& R : T)
		{
			const int M = ss::fuse::MOf(R, Tier);
			const Chips Pool = static_cast<Chips>(M) * ss::fuse::BuyInOf(Tier);
			const std::array<Chips, 3> P = ss::fuse::Payout(Tier, M);
			Paid = Paid && P[0] + P[1] + P[2] == Pool && P[0] >= P[1] && P[1] >= P[2] && P[2] >= 0;
			Paid = Paid && (M < 25 ? P[0] == Pool : (std::llabs(P[0] - Pool * 8 / 10) <= 1 && std::llabs(P[1] - Pool * 12 / 100) <= 1 && std::llabs(P[2] - Pool * 8 / 100) <= 1));
		}
	}
	Expect(Paid && ss::fuse::Payout(0, 25)[0] == 2200 && ss::fuse::Payout(0, 25)[1] == 330 && ss::fuse::Payout(0, 25)[2] == 220,
		"fuse: the top-tier split sums to the prize (x25 at $1.10: $22.00, $3.30, $2.20); winner takes all to x10");
	Expect(ss::fuse::RevealSeconds(2) == 1.5 && ss::fuse::RevealSeconds(10) == 3.4 && ss::fuse::RevealSeconds(25) == 6.3, "fuse: the reveal runs 1.5 s, 3.4 s, 6.3 s");
}

void Pool()
{
	// The synthetic event through the engine, no change to it: the pool is exactly m x buy-in at every row of every tier.
	bool All = true;
	std::string Bad;
	for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
	{
		for (const ss::fuse::Row& R : ss::fuse::Table())
		{
			const int M = ss::fuse::MOf(R, Tier);
			const ss::LobbyEvent L = ss::fuse::EventFor(Tier, M, 1200.0, 1);
			ss::Tournament Tn(L.Spec, "Hero", "fuse-pool");
			const Chips Pool = static_cast<Chips>(M) * ss::fuse::BuyInOf(Tier);
			const bool Ok = Tn.PrizePoolCents == Pool && Tn.Payouts.size() == 1 && Tn.Payouts[0] == Pool && Tn.Players.size() == 3 && Tn.Tables.size() == 1 && Tn.TableSize == 3 &&
				Tn.CurrentLevel().Bb == 20 && Tn.CurrentLevel().Sb == 10 && Tn.Players[0].Stack == 300 && L.Spec.FeeCents == L.Spec.BuyInCents && L.Spec.Population == "high" &&
				L.Spec.Speed == "Hyper" && L.Spec.Entrants == 3 && L.Name == ss::fuse::NameOf(Tier) && L.BuyInCents == ss::fuse::BuyInOf(Tier);
			All = All && Ok;
			Bad += Ok ? "" : " tier " + std::to_string(Tier) + " x" + std::to_string(M) + " pool " + std::to_string(Tn.PrizePoolCents);
		}
	}
	Expect(All, "fuse: PrizePoolCents == m x buy-in and Payouts == {pool} for every row and tier: one 3-seat table, 300 chips, 10/20" + Bad);
	// Why FeeCents is the whole buy-in: the engine's pool is max(GTD, entrants x (buy-in - fee)), a guarantee only a floor.
	ss::LobbyEvent Naive = ss::fuse::EventFor(0, 2, 1200.0, 1);
	Naive.Spec.FeeCents = ss::fuse::FeeOf(0);
	ss::Tournament Over(Naive.Spec, "Hero", "fuse-pool");
	Expect(Over.PrizePoolCents == 3 * (110 - ss::fuse::FeeOf(0)), "fuse: with the real fee as the spec's the x2 pool would overfill to " + ss::Money(Over.PrizePoolCents));
	// Six hands a 3-minute level at 30 s a hand.
	ss::Tournament Lv(ss::fuse::EventFor(0, 2, 1200.0, 1).Spec, "Hero", "fuse-levels");
	int Ticks = 0, FirstUp = 0;
	const ss::Profile Pilot = ss::SprintProfile();
	while (!Lv.bFinished && Ticks < 400)
	{
		const int Before = Lv.LevelIndex;
		Lv.SimulateTick(&Pilot);
		++Ticks;
		FirstUp = FirstUp == 0 && Lv.LevelIndex != Before ? Ticks : FirstUp;
	}
	const std::vector<ss::Level> Levels = ss::fuse::Levels();
	Expect(FirstUp == 6 && Levels.size() >= 20 && Levels[8].Bb == 200 && Levels[9].Bb == 250 && Levels[1].Sb == 15, "fuse: six hands a 3-minute level; 10/20 to 100/200, then +25% a level");
}

void Ids()
{
	const std::string A = ss::fuse::IdFor(0, 1234.6, 7), B = ss::fuse::IdFor(0, 1234.2, 8), C = ss::fuse::IdFor(2, 1234.0, 7);
	Expect(A == "ignite.fuse0@1234#7" && B == "ignite.fuse0@1234#8" && C == "ignite.fuse2@1234#7", "fuse: ids read ignite.fuse<tier>@<minute>#<n> (" + A + ")");
	Expect(ss::net::SiteOf(A) == ss::net::SiteId::Ignite && &ss::net::ForEvent(C) == &ss::net::Site(ss::net::SiteId::Ignite) &&
			ss::net::Site(ss::net::SiteId::Ignite).FindTemplate("ignite.fuse0") == nullptr,
		"fuse: SiteOf reads Ignite, ForEvent is Ignite's network, and no template has the id");
	Expect(ss::fuse::IsFuse(A) && ss::fuse::IsFuse(C) && !ss::fuse::IsFuse("ignite.flip-go@1380") && !ss::fuse::IsFuse("night-owl@1560") &&
			!ss::fuse::IsFuse(ss::fuse::TicketId(0)) && !ss::fuse::IsFuse("ignite.fuse3@1#1") && !ss::fuse::IsFuse(""),
		"fuse: IsFuse is a FUSE id's (never a ticket id, another event, or a tier past 2)");
	// The tier's digit ends the template part (review-core-fuse: a mutant that dropped the '@' rule passed every check).
	Expect(!ss::fuse::IsFuse("ignite.fuse0x") && !ss::fuse::IsFuse("ignite.fuse01@1200#1") && !ss::fuse::IsFuse("ignite.fuse1k@1200") && ss::fuse::IsFuse("ignite.fuse2") &&
			ss::fuse::IsFuse("ignite.fuse0@") && ss::fuse::TierOf("ignite.fuse2") == 2 && ss::fuse::TierOf("ignite.fuse0x") == -1,
		"fuse: only ignite.fuse<0-2> followed by '@' or the end is a FUSE id (ignite.fuse0x, ignite.fuse01@... are not)");
	Expect(ss::fuse::TierOf(C) == 2 && ss::fuse::OrdinalOf(A) == 7 && ss::fuse::TierOf("night-owl@1") == -1 && ss::fuse::OrdinalOf("night-owl@1") == -1,
		"fuse: TierOf and OrdinalOf read the id back");
	Expect(ss::fuse::TicketId(1) == "ignite.fusetkt1" && ss::fuse::TierOfTicket("ignite.fusetkt2") == 2 && ss::fuse::TierOfTicket("ignite.fuse0") == -1 &&
			ss::fuse::TierOfTicket("ignite.flip-go") == -1 && ss::net::Site(ss::net::SiteId::Ignite).TicketOf("ignite.fuse0") != ss::fuse::TicketId(0),
		"fuse: a FUSE ticket is held as ignite.fusetkt<tier>, never under the id RegisterEvent's ticket test reads");
}

void Draws()
{
	// Reproducible, per tier, and uniform enough to draw the table (S-J: the raw 32-bit Fnv1a failed; the fmix32 step).
	Expect(ss::fuse::Draw(7, 0, 41) == ss::fuse::Draw(7, 0, 41) && ss::fuse::Draw(99, 2, 5) == ss::fuse::Draw(99, 2, 5), "fuse: the n-th multiplier at a tier reproduces");
	const int Count = 200000;
	double Worst = 0.0;
	bool InTable = true;
	for (const uint64_t Seed : {7ull, 1234567ull, 987654321987ull})
	{
		for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
		{
			std::map<int, int> Got;
			for (int N = 1; N <= Count; ++N)
			{
				++Got[ss::fuse::Draw(Seed, Tier, N)];
			}
			double Chi = 0.0;
			int Rows = 0;
			for (const ss::fuse::Row& R : ss::fuse::Table())
			{
				const double E = ss::fuse::POf(R, Tier) * Count;
				const double O = Got[ss::fuse::MOf(R, Tier)];
				Chi += (O - E) * (O - E) / E;
				Rows += Got.count(ss::fuse::MOf(R, Tier)) ? 1 : 0;
			}
			InTable = InTable && static_cast<int>(Got.size()) <= 7;
			Worst = std::max(Worst, Chi);
		}
	}
	Expect(InTable && Worst < 22.5, "fuse: 200,000 draws a tier on three world seeds match the table (worst chi-square " + Num(Worst, 1) + " on 6 df; 22.5 is p 0.001)");
	// Tiers have their own sequences: the stakes can't be played against each other to pick a known multiplier.
	int Same = 0;
	for (int N = 1; N <= 2000; ++N)
	{
		Same += ss::fuse::Draw(7, 0, N) == ss::fuse::Draw(7, 1, N) ? 1 : 0;
	}
	Expect(Same > 1000 && Same < 1800, "fuse: each tier draws its own sequence (" + std::to_string(Same) + " of 2,000 ordinals agree, about what chance gives)");
}

void Record()
{
	ss::site::play::FuseRec R;
	Expect(!ss::fuse::HasRecord(R), "record: an untouched record holds nothing");
	std::string Empty;
	ss::fuse::WriteLine(R, Empty);
	Expect(Empty.empty(), "record: an untouched record writes no line (S10)");
	ss::fuse::NoteEntry(R, 0, 2, true);
	ss::fuse::NoteEntry(R, 0, 25, false);
	ss::fuse::NoteEntry(R, 2, 3, true);
	Expect(R.N[0] == 2 && R.N[1] == 0 && R.N[2] == 1 && R.Last10[0] == 3 && R.Last10[1] == 25 && R.Last10[2] == 2 && R.BestM == 25 && R.RecentBuyIns[0] == 550 &&
			R.RecentBuyIns[1] == 110 && R.RecentBuyIns[2] == 0 && R.NetCents == -660,
		"record: entries advance the tier's ordinal (tickets too), Last10 and the best; only paid ones add a buy-in and cost money");
	ss::site::play::SettleFacts F;
	F.Site = ss::net::SiteId::Ignite;
	F.Fuse = true;
	F.EventId = ss::fuse::IdFor(0, 3000.0, 2);
	F.FuseM = 25;
	F.Place = 2;
	F.Prize = 330;
	F.BuyIn = 110;
	F.FuseWinner = "lilavance";
	ss::fuse::NoteResult(R, F, 3000.0);
	Expect(R.Played == 1 && R.Won == 0 && R.NetCents == -330 && R.Wall.size() == 1 && R.Wall[0].M == 25 && R.Wall[0].Place == 2 && R.Wall[0].Prize == 330 &&
			R.Wall[0].Day == ss::net::DayOf(3000.0 - 360.0) && R.Hits.size() == 1 && R.Hits[0].Winner == "lilavance" && R.Hits[0].Prize == 2200,
		"record: a result counts, its prize goes into net, a x25 joins the Wall (place and prize) and Big hits (the winner and the first prize)");
	// The Wall keeps the biggest ten; Big hits the newest ten, and only with a known winner.
	for (int K = 0; K < 14; ++K)
	{
		ss::site::play::SettleFacts G = F;
		G.FuseM = K % 3 == 0 ? 100 : K % 3 == 1 ? 10 : 2;
		G.Place = 1;
		G.Prize = ss::fuse::Payout(0, G.FuseM)[0];
		G.FuseWinner = K == 13 ? std::string() : "Hero:A,B\tC\\";
		ss::fuse::NoteResult(R, G, 3000.0 + 1440.0 * K);
	}
	bool Ordered = R.Wall.size() == 10;
	for (size_t I = 1; I < R.Wall.size(); ++I)
	{
		Ordered = Ordered && R.Wall[I - 1].M >= R.Wall[I].M;
	}
	Expect(Ordered && R.Wall.front().M == 100 && R.Hits.size() == 10 && R.Hits.front().Winner == "Hero:A,B\tC\\" && R.Played == 15 && R.Won == 14,
		"record: the Wall keeps the ten biggest; Big hits the ten newest (an unknown winner makes none: never a guess)");
	// The save line round-trips byte for byte, the escapes included.
	std::string Line;
	ss::fuse::WriteLine(R, Line);
	std::vector<std::string> P;
	{
		std::string Cur;
		for (const char Ch : Line.substr(0, Line.size() - 1))
		{
			if (Ch == '\t')
			{
				P.push_back(Cur);
				Cur.clear();
			}
			else
			{
				Cur.push_back(Ch);
			}
		}
		P.push_back(Cur);
	}
	ss::site::play::FuseRec Back;
	const bool Read = ss::fuse::ReadLine(P, Back);
	std::string Again;
	ss::fuse::WriteLine(Back, Again);
	Expect(Read && Again == Line && Back.Hits.front().Winner == R.Hits.front().Winner && Back.Wall.size() == 10 && Back.RecentBuyIns[1] == 110 && P.size() == 14 &&
			Line.rfind("site\tfuse\tignite\t2\t0\t1\t15\t14\t", 0) == 0,
		"record: site\\tfuse\\tignite round-trips byte for byte (" + std::to_string(Line.size()) + " B on a full record)");
	Expect(!ss::fuse::ReadLine({"site", "fuse", "ignite", "1"}, Back) && !ss::fuse::ReadLine({"site", "spark", "ignite"}, Back), "record: a short or foreign line isn't read");
	// RecentBuyIns is BandOf's for every paid Ignite entry (core-siteplay pushes the MTTs'): an MTT-only band is saved too.
	ss::site::play::FuseRec Band;
	Band.RecentBuyIns[0] = 330;
	Band.RecentBuyIns[1] = 110;
	std::string BandLine;
	ss::fuse::WriteLine(Band, BandLine);
	ss::site::play::FuseRec BandBack;
	std::vector<std::string> Bp = {"site", "fuse", "ignite", "0", "0", "0", "0", "0", "0", "0", "", "", "", "330,110"};
	Expect(ss::fuse::HasRecord(Band) && BandLine == "site\tfuse\tignite\t0\t0\t0\t0\t0\t0\t0\t\t\t\t330,110\n" && ss::fuse::ReadLine(Bp, BandBack) &&
			BandBack.RecentBuyIns == Band.RecentBuyIns,
		"record: a band of MTT buy-ins alone is a record, and round-trips");
	// The heaviest record (§7.4's ~400 B; Ignite's whole play state <= 1.0 KB): ten wall rows, ten hits with long names, 20 buy-ins.
	ss::site::play::FuseRec Heavy;
	for (int K = 0; K < 20; ++K)
	{
		ss::fuse::NoteEntry(Heavy, 2, 1000, true);
	}
	for (int K = 0; K < 10; ++K)
	{
		ss::site::play::SettleFacts G = F;
		G.EventId = ss::fuse::IdFor(2, 900000.0, 5000 + K);
		G.FuseM = 250;
		G.Place = 1;
		G.Prize = ss::fuse::Payout(2, 250)[0];
		G.FuseWinner = "averylonghandle_" + std::to_string(K);
		ss::fuse::NoteResult(Heavy, G, 900000.0);
	}
	Heavy.N = {99999, 99999, 99999};
	std::string HeavyLine;
	ss::fuse::WriteLine(Heavy, HeavyLine);
	Note("the heaviest site\\tfuse line: " + std::to_string(HeavyLine.size()) + " B");
	Expect(HeavyLine.size() <= 700, "record: the heaviest fuse line stays at or under 700 B (" + std::to_string(HeavyLine.size()) + " B)");
	Expect(!ss::fuse::Describe(R).empty(), "record: Describe prints the record");
}

void Seats()
{
	// No world: no keyed members (NameField names the seats itself).
	Expect(ss::fuse::SeatsFor(nullptr, 7, 0, 1, 1200.0).empty(), "seats: no world, no members");
}

void Copy()
{
	std::vector<std::pair<std::string, std::string>> Lines;
	std::vector<std::string> All = ss::fuse::Inventory();
	const std::map<std::string, std::string> Vars = {{"Event", "FUSE $1.10"}, {"Amount", "$27.50"}, {"Blinds", "10/20"}, {"Level", "3"}, {"Mult", "25"}, {"Place", "1st"}};
	std::map<std::string, std::string> LabelVars = copylint::Samples();
	for (const auto& Kv : Vars)
	{
		LabelVars[Kv.first] = Kv.second;
	}
	int Problems = 0;
	for (const std::string& T : All)
	{
		const bool Sentence = !T.empty() && (T.back() == '.' || T.back() == '!' || T.back() == '?');
		if (Sentence)
		{
			Lines.push_back({"fuse", T});
		}
		else
		{
			// A label: the same rules without the stop (filled, no doubled space, nothing banned).
			const std::string Why = copylint::Check(T + ".", copylint::TableTalk, LabelVars);
			Problems += Why.empty() ? 0 : 1;
			if (!Why.empty())
			{
				std::printf("COPY (fuse labels) \"%s\": %s\n", T.c_str(), Why.c_str());
			}
		}
	}
	Problems += copylint::Run("fuse", Lines, copylint::TableTalk, Vars);
	for (const std::string& T : All)
	{
		const std::string Low = copylint::Lower(T);
		for (const char* Banned : {" ai ", " ai.", "bot", "autoplay", "autopilot", "jackpot", "ember"})
		{
			if (Low.find(Banned) != std::string::npos)
			{
				std::printf("COPY (fuse) \"%s\" says \"%s\"\n", T.c_str(), Banned);
				++Problems;
			}
		}
		if (const char* Hit = trademarks::Find(T, trademarks::kSourceScan))
		{
			std::printf("COPY (fuse) \"%s\": a real name (%s)\n", T.c_str(), Hit);
			++Problems;
		}
		if (const char* Hit = trademarks::Find(T, trademarks::kCopyScan))
		{
			std::printf("COPY (fuse) \"%s\": a real name (%s)\n", T.c_str(), Hit);
			++Problems;
		}
	}
	Expect(Problems == 0 && Lines.size() >= 5 && All.size() >= 8, "copy: FUSE's lines pass the lint, U1, \"jackpot\" and both trademark tiers (" + std::to_string(All.size()) + " strings)");
}

void Calibration()
{
	// ROI from S-J's place odds at the chosen knobs (the session pays 80/12/8 from x25, so P(2nd) and P(3rd) matter a little).
	const Measured& Coach = kOdds[0];
	double CoachRoi[3] = {0, 0, 0};
	double BestAuto = -1.0;
	std::string AutoRows;
	bool Band = true;
	for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
	{
		CoachRoi[Tier] = RoiOf(Coach.P1, Coach.P2, Coach.P3, Tier);
		Band = Band && CoachRoi[Tier] >= 0.04 && CoachRoi[Tier] <= 0.10;
	}
	bool AutoUnder = true;
	for (const Measured& M : kOdds)
	{
		if (!M.Autoplay)
		{
			continue;
		}
		const double R = std::max(RoiOf(M.P1, M.P2, M.P3, 0), std::max(RoiOf(M.P1, M.P2, M.P3, 1), RoiOf(M.P1, M.P2, M.P3, 2)));
		AutoUnder = AutoUnder && R <= 0.05;
		BestAuto = std::max(BestAuto, R);
		if (std::string(M.Who) == "Autopilot" || std::string(M.Who) == "Push/Fold" || std::string(M.Who) == "Maniac")
		{
			AutoRows += std::string(AutoRows.empty() ? "" : ", ") + M.Who + " " + Pct(R);
		}
	}
	Note("ROI from S-J's place odds: coach-best " + Pct(CoachRoi[0]) + " / " + Pct(CoachRoi[1]) + " / " + Pct(CoachRoi[2]) + "; " + AutoRows + "; best autoplay style " + Pct(BestAuto));
	Expect(Band, "calibration: coach-best ROI inside +4% to +10% at every tier (" + Pct(CoachRoi[0]) + ")");
	Expect(AutoUnder, "calibration: Autopilot, Push/Fold and every other autoplay style at or under +5% (best " + Pct(BestAuto) + ")");
	// EV a world hour against the same stake's MTT grind (one table, to the hero's finish).
	bool Hour = true;
	std::string Ratios;
	for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
	{
		const double PerHour = CoachRoi[Tier] * static_cast<double>(ss::fuse::BuyInOf(Tier)) / 100.0 * Coach.PerHour;
		const double Ratio = PerHour / kMttPerHour[Tier];
		Hour = Hour && Ratio <= 1.5;
		Ratios += std::string(Ratios.empty() ? "" : ", ") + "$" + Num(PerHour, 2) + " (" + Num(Ratio, 2) + "x)";
	}
	Expect(Hour, "calibration: FUSE EV a world hour at most 1.5x the same stake's MTT grind: " + Ratios);
	// The FUSE grinder: 30 x $5.50 a day for 30 days on autoplay nets at most 25% of a rent.
	const double Grinder = 30.0 * 30.0 * 550.0 * std::max(0.0, BestAuto);
	Expect(Grinder <= 0.25 * kRentCents, "calibration: a 30-a-day $5.50 grinder on the best autoplay style nets " + ss::Money(static_cast<Chips>(std::llround(Grinder))) +
		" a month, at most 25% of a rent");
	// A first prize of $500 or more inside a year at 30 FUSEs a week, at coach-best's P(1st).
	double Big[3] = {0, 0, 0};
	for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
	{
		for (const ss::fuse::Row& R : ss::fuse::Table())
		{
			Big[Tier] += ss::fuse::Payout(Tier, ss::fuse::MOf(R, Tier))[0] >= 50000 ? ss::fuse::POf(R, Tier) * Coach.P1 : 0.0;
		}
	}
	const double Year = 1.0 - std::pow(1.0 - std::max(Big[0], std::max(Big[1], Big[2])), 30.0 * 52.0);
	Expect(Year <= 0.05, "calibration: P(a first prize of $500+ in a year at 30 a week) " + Num(100.0 * Year, 1) + "%, at most 5%");
	// Autoplay's FUSE volume against manual (the table played out): the lead accepted the measured excess (it moves no money,
	// every autoplay style's ROI being under +5% and below coach-best's); the money form is what's asserted.
	double WorstVolume = 0.0;
	bool Money = true;
	for (const Measured& M : kOdds)
	{
		if (!M.Autoplay)
		{
			continue;
		}
		WorstVolume = std::max(WorstVolume, M.PerHourFull / Coach.PerHourFull);
		for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
		{
			const double AutoHour = RoiOf(M.P1, M.P2, M.P3, Tier) * M.PerHourFull;
			Money = Money && AutoHour < CoachRoi[Tier] * Coach.PerHourFull;
		}
	}
	Note("autoplay's FUSE events a world hour, the table played out: at most " + Num(WorstVolume, 3) + "x manual (accepted by the lead; S-J measured 1.12-1.15x at 400 chips)");
	Expect(Money && WorstVolume <= 1.20, "calibration: autoplay's FUSE money a world hour stays under manual's at every tier (volume " + Num(WorstVolume, 2) + "x)");
}

// ------------------------------------------------------------------ a session with Ignite

struct FuseHooks : ss::SessionHooks
{
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D.Serialize();
		LastRec = D.Life.SitePlay.Ig.Fuse;
		LastBits = D.Life.Update3LinesRead;
	}
	void Text(const std::string& From, const std::string& Body) override { Texts.push_back(From + ": " + Body); }
	int Saves = 0;
	std::string Last;
	ss::site::play::FuseRec LastRec; // the record as the last save carried it
	uint32_t LastBits = 0;
	std::vector<std::string> Texts;
};

struct Rig
{
	FuseHooks H;
	std::unique_ptr<ss::Session> S;
	double Now = 0.0;
};

void Pump(Rig& R, double Seconds)
{
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		R.Now += 1.0 / 30.0;
		R.S->Update(R.Now);
	}
}

std::unique_ptr<Rig> MakeRig(const std::string& Seed, Chips Wallet = 20000, bool Join = true)
{
	std::unique_ptr<Rig> R(new Rig());
	R->S.reset(new ss::Session(R->H, Seed));
	ss::Session& S = *R->S;
	S.CurrentScreen = ss::Screen::Lobby;
	Pump(*R, 0.2);
	S.BankrollCents = 500000;
	S.DebugSite("launch ignite");
	S.DebugSite("invite ignite");
	if (Join)
	{
		S.AnswerInvite(ss::net::SiteId::Ignite, true);
		S.Deposit(ss::net::SiteId::Ignite, Wallet, "card");
	}
	return R;
}

ss::site::Account* Wallet(Rig& R)
{
	for (ss::site::Account& A : R.S->Life.Sites.Accounts)
	{
		if (A.Site == ss::net::SiteId::Ignite)
		{
			return &A;
		}
	}
	return nullptr;
}

/** The front table played out at Sprint, every prompt answered check or call. */
void PlayOut(Rig& R)
{
	ss::Session& S = *R.S;
	S.SprintBudgetMs = 1.0e9;
	for (int F = 0; S.CurrentScreen == ss::Screen::Table && F < 400000; ++F)
	{
		S.CurrentPace = ss::Pace::Sprint;
		R.Now += 1.0 / 30.0;
		S.Update(R.Now);
		if (S.HasPrompt)
		{
			S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
		}
	}
}

int CountLines(const std::string& Text, const std::string& Prefix)
{
	int N = 0;
	size_t At = 0;
	while (At < Text.size())
	{
		const size_t Eol = Text.find('\n', At);
		N += Text.compare(At, Prefix.size(), Prefix) == 0 ? 1 : 0;
		At = Eol == std::string::npos ? Text.size() : Eol + 1;
	}
	return N;
}

std::string FuseLineOf(const std::string& Save)
{
	const size_t At = Save.find("site\tfuse\tignite\t");
	if (At == std::string::npos)
	{
		return std::string();
	}
	return Save.substr(At, Save.find('\n', At) - At);
}

void AddClause(ss::Session& S, const std::string& Only)
{
	ss::sponsor::Contract C;
	C.Id = "fuse-clause-" + Only;
	C.Brand = Only == "riverline" ? "riverline" : "turnstone";
	C.St = ss::sponsor::State::Active;
	C.T.Only = Only;
	C.Since = S.WorldMinutes() - 60.0;
	C.Until = S.WorldMinutes() + 90.0 * ss::net::MinutesPerDay;
	S.Life.Sponsors.Contracts.push_back(C);
}

void Light()
{
	std::unique_ptr<Rig> R = MakeRig("fuse-light");
	ss::Session& S = *R->S;
	ss::site::Account* A = Wallet(*R);
	Expect(A && A->Cents > 0, "light: an Ignite account with money on it");
	if (!A)
	{
		return;
	}
	// The welcome bonus releases $5 for each $20 of rake: 20 cents short of the next $20, the real fee (11 cents) releases
	// nothing where the spec's 110 would have.
	A->RakeLife = 2000 - 20;
	const Chips W0 = A->Cents, Rake0 = A->RakeLife, Month0 = A->RakeMonth, Released0 = A->BonusReleased, Pending0 = A->BonusPending;
	const int64_t Base0 = S.Life.SitePlay.Ig.S.BaseLife;
	const size_t History0 = S.History.size();
	std::string World0;
	S.Living().Write(World0);
	const int HeroResults0 = CountLines(World0, "world\theroresult");
	const auto Rev0 = S.Living().HeroRevision();
	const int Saves0 = R->H.Saves;
	const int Expected = ss::fuse::Draw(S.WorldSeed(), 0, 1);
	const double LitNow = S.Now;
	const std::string Why = S.LightFuse(0, false);
	A = Wallet(*R);
	Expect(Why.empty() && S.T && ss::fuse::IsFuse(S.T->Spec.Id) && ss::fuse::OrdinalOf(S.T->Spec.Id) == 1 && S.T->PrizePoolCents == Expected * 110 && A->Cents == W0 - 110,
		"light: LIGHT IT seats the first $1.10 FUSE at its keyed multiplier (x" + std::to_string(Expected) + "), the pool exactly m x $1.10, the wallet -$1.10 (" + Why + ")");
	if (!S.T)
	{
		return;
	}
	Expect(A->RakeLife - Rake0 == ss::fuse::FeeOf(0) && A->RakeMonth - Month0 == ss::fuse::FeeOf(0) && A->Lines.back().Label == "Buy-in: FUSE $1.10" && A->Lines.back().Cents == -110,
		"light: the rake hooks saw FeeOf(0) = 11 cents, never the spec's 110; the wallet's line \"Buy-in: FUSE $1.10\"");
	Expect(Pending0 > 0 && A->BonusReleased == Released0 && A->BonusPending == Pending0, "light: the welcome bonus releases no faster (11 cents of rake, not 110)");
	// OnPaidEntry heard the same fee: base Sparks are 10 a cent of it (x1.5 on FUSE NIGHT), once core-siteplay credits them.
	const int64_t BaseGot = S.Life.SitePlay.Ig.S.BaseLife - Base0;
	if (BaseGot == 0)
	{
		Note("light: no base Sparks credited yet (core-siteplay's OnPaidEntry)");
	}
	else
	{
		Expect(BaseGot == 10 * ss::fuse::FeeOf(0) || BaseGot == 15 * ss::fuse::FeeOf(0), "light: OnPaidEntry saw FeeOf(0): " + std::to_string(BaseGot) + " base Sparks");
	}
	// The commit: RegisterEvent's Save() carried the advanced ordinal before a hand was played (the reveal comes after).
	Expect(R->H.Saves > Saves0 && S.T->Tick <= 1 && R->H.LastRec.N[0] == 1 && R->H.LastRec.Last10[0] == Expected && (R->H.LastBits & ss::life::U3SitePlay) != 0 &&
			S.Life.SitePlay.Ig.Fuse.N[0] == 1,
		"light: the save committed the ordinal at entry, before the first hand (the book's bit set: it exists from here)");
	// The text too, through core-siteplay's writer (site::play::WriteLines -> fuse::WriteLine).
	const std::string Saved = FuseLineOf(R->H.Last);
	Expect(Saved.rfind("site\tfuse\tignite\t1\t0\t0\t", 0) == 0, "light: the save's text carries the fuse line with the ordinal (" + Saved + ")");
	// The reveal's state.
	const ss::fuse::Reveal Rv = S.FuseReveal(S.FocusedTable());
	Expect(Rv.M == Expected && Rv.Tier == 0 && Rv.PaidAt == LitNow && !Rv.Seen && S.FuseReveal(-1).M == Expected, "light: FuseReveal reads core's multiplier, the tier and the light's clock");
	// The seats: two keyed Ignite members, no registration in the world.
	const std::vector<std::string> Keyed = ss::fuse::SeatsFor(&S.Living(), S.WorldSeed(), 0, 1, S.WorldMinutes());
	std::vector<std::string> Seated;
	int Members = 0;
	for (const ss::TPlayer& P : S.T->Players)
	{
		if (!P.IsHero)
		{
			Seated.push_back(P.Name);
			Members += S.FieldNpc.count(P.Id) ? 1 : 0;
		}
	}
	bool OnIgnite = true;
	for (const std::string& Name : Seated)
	{
		const ss::world::Npc* N = S.Living().Get(S.Living().Find(Name));
		OnIgnite = OnIgnite && N && (N->Sites & (1u << static_cast<unsigned>(ss::net::SiteId::Ignite))) != 0 && !N->Rival;
	}
	Expect(Seated.size() == 2 && Keyed.size() == 2 && Seated == Keyed && Members == 2 && OnIgnite && S.Living().RegisteredNow(S.T->Spec.Id).empty(),
		"seats: the two others are SeatsFor's keyed Ignite members, in order; the world holds no registration for the FUSE");
	// Who SeatsFor may seat, over hours, tiers and ordinals (review-core-fuse: a mutant that seated the rival, the pros and the
	// high stakes passed every check, because a new world's Ignite holds none of them). A copy of this world read back from its
	// own text, with the pros, the rival and regulars above the low stakes given Ignite accounts too (an older world's Ignite
	// team and risen regulars): still two different people, each a playing Ignite member at the micro or low stakes.
	{
		const unsigned IgniteBit = 1u << static_cast<unsigned>(ss::net::SiteId::Ignite);
		std::set<int> Marked;
		for (const ss::world::Npc& P : S.Living().People())
		{
			if (P.Playing() && (P.Pro || P.Rival || P.Tier > static_cast<int>(ss::net::Tier::Low)) && Marked.size() < 80)
			{
				Marked.insert(P.Id);
			}
		}
		ss::world::World Crafted;
		for (size_t At = 0; At < World0.size();)
		{
			const size_t Eol = std::min(World0.find('\n', At), World0.size());
			std::string Line = World0.substr(At, Eol - At);
			At = Eol + 1;
			if (Line.rfind("world\tnpc\t", 0) == 0 && Marked.count(std::atoi(Line.c_str() + 10)) > 0)
			{
				// The npc line's last field is the sites they play (world version 4).
				const size_t Tab = Line.rfind('\t');
				Line = Line.substr(0, Tab + 1) + std::to_string(std::atoi(Line.c_str() + Tab + 1) | static_cast<int>(IgniteBit));
			}
			std::vector<std::string> Fields(1);
			for (const char Ch : Line)
			{
				if (Ch == '\t')
				{
					Fields.emplace_back();
				}
				else
				{
					Fields.back().push_back(Ch);
				}
			}
			Crafted.Read(Fields);
		}
		Crafted.Finish();
		int Widened = 0;
		for (const int Id : Marked)
		{
			const ss::world::Npc* P = Crafted.Get(Id);
			Widened += P && (P->Sites & IgniteBit) != 0 ? 1 : 0;
		}
		int Calls = 0, Wrong = 0;
		for (int Hour = 0; Hour < 24; Hour += 3)
		{
			for (int Tier = 0; Tier < ss::fuse::TierCount; ++Tier)
			{
				for (int N = 1; N <= 20; ++N)
				{
					const std::vector<std::string> Two = ss::fuse::SeatsFor(&Crafted, S.WorldSeed(), Tier, N, S.WorldMinutes() + 60.0 * Hour);
					++Calls;
					Wrong += Two.size() != 2 || Two[0] == Two[1] ? 1 : 0;
					for (const std::string& Name : Two)
					{
						const ss::world::Npc* P = Crafted.Get(Crafted.Find(Name));
						Wrong += !P || !P->Playing() || P->Rival || P->Pro || P->Tier > static_cast<int>(ss::net::Tier::Low) || (P->Sites & IgniteBit) == 0 ? 1 : 0;
					}
				}
			}
		}
		Expect(Widened >= 20 && Calls == 480 && Wrong == 0, "seats: SeatsFor seats two different playing Ignite members at the micro or low stakes, never the rival, a pro or a "
			"higher regular, though " + std::to_string(Widened) + " of them hold Ignite accounts (" + std::to_string(Wrong) + " wrong seats in " + std::to_string(Calls) + " draws)");
	}
	// Seen a second after the reveal's run (the table opens itself 1 s after the boom, §2.4 D), not before, on the
	// presentation clock (review-core-fuse: a mutant without the second passed).
	Pump(*R, ss::fuse::RevealSeconds(Expected) + 0.5);
	const bool SeenEarly = S.CurrentScreen == ss::Screen::Table && S.FuseReveal(S.FocusedTable()).Seen;
	Pump(*R, 0.7);
	Expect(!SeenEarly && (S.CurrentScreen != ss::Screen::Table || S.FuseReveal(S.FocusedTable()).Seen),
		"light: the reveal reads Seen once its run and a second have passed (not half a second after its run)");
	// Played out: the split, and the bypass.
	std::map<std::string, std::pair<int, size_t>> Bonds0;
	for (const std::string& Name : Seated)
	{
		const int Npc = S.Living().Find(Name);
		const ss::world::Bond* B = Npc >= 0 ? S.Living().BondWith(Npc) : nullptr;
		Bonds0[Name] = B ? std::make_pair(B->Encounters, B->Memories.size()) : std::make_pair(0, static_cast<size_t>(0));
	}
	PlayOut(*R);
	A = Wallet(*R);
	const int Place = S.LastResults.Place;
	const std::array<Chips, 3> Pay = ss::fuse::Payout(0, Expected);
	const Chips Prize = Place >= 1 && Place <= 3 ? Pay[static_cast<size_t>(Place - 1)] : -1;
	Expect(S.CurrentScreen == ss::Screen::Results && S.LastResults.PrizeCents == Prize && A->Cents == W0 - 110 + Prize, "split: the session paid place " + std::to_string(Place) + " its share");
	// Its wallet line: "FUSE x25: 1st" (§1.3; core-sites-g2's SiteSettle at G2), or day 0's "Prize: FUSE $1.10" in a day-0 tree.
	if (Prize > 0)
	{
		const std::string Want = "FUSE x" + std::to_string(Expected) + ": " + ss::Ordinal(Place);
		std::string Got;
		for (auto It = A->Lines.rbegin(); It != A->Lines.rend() && Got.empty(); ++It)
		{
			Got = It->Cents == Prize && (It->Label == Want || It->Label == "Prize: FUSE $1.10") ? It->Label : Got;
		}
		Expect(!Got.empty(), "split: the wallet's prize line \"" + Got + "\" +" + ss::Money(Prize));
		if (Got != Want)
		{
			Note("split: the prize line reads day 0's \"" + Got + "\", not G2's \"" + Want + "\" (core-sites-g2's SiteSettle)");
		}
	}
	std::string World1;
	S.Living().Write(World1);
	bool BondsSame = true;
	for (const std::string& Name : Seated)
	{
		const int Npc = S.Living().Find(Name);
		const ss::world::Bond* B = Npc >= 0 ? S.Living().BondWith(Npc) : nullptr;
		BondsSame = BondsSame && (B ? std::make_pair(B->Encounters, B->Memories.size()) : std::make_pair(0, static_cast<size_t>(0))) == Bonds0[Name];
	}
	Expect(S.History.size() == History0, "bypass: FUSE never enters History (Abi100, the gates and Career never see it)");
	Expect(CountLines(World1, "world\theroresult") == HeroResults0 && S.Living().HeroRevision() == Rev0, "bypass: no world\\theroresult; the world's hero revision unchanged");
	Expect(BondsSame, "bypass: no memory or encounter for the two members (no Remember of the raw id)");
	Expect(A->Hero.Points == 0.0 && A->Hero.FinalTables == 0 && A->Hero.Events == 1 && S.LastSiteResult == ss::net::SiteId::Ignite,
		"bypass: no Season points and no final table, one event on the record; the results draw in Ignite's frame (LastSiteResult)");
	// The result side of the record: core-siteplay's OnResult calls fuse::NoteResult.
	const ss::site::play::FuseRec& Rec = S.Life.SitePlay.Ig.Fuse;
	Expect(Rec.Played == 1 && Rec.Won == (Place == 1 ? 1 : 0) && Rec.NetCents == Prize - 110 && Rec.Last10[0] == Expected,
		"record: OnResult wrote the result through fuse::NoteResult (played 1, won " + std::to_string(Rec.Won) + ", net " + ss::Money(Rec.NetCents) + ")");
}

void Tickets()
{
	std::unique_ptr<Rig> R = MakeRig("fuse-ticket");
	ss::Session& S = *R->S;
	S.Life.Tickets[ss::fuse::TicketId(0)] = 1;
	ss::site::Account* A = Wallet(*R);
	const Chips W0 = A->Cents, Rake0 = A->RakeLife;
	// Holding a ticket, the paid button pays (and keeps the ticket).
	const std::string Paid = S.LightFuse(0, false);
	A = Wallet(*R);
	Expect(Paid.empty() && A->Cents == W0 - 110 && A->RakeLife == Rake0 + ss::fuse::FeeOf(0) && S.Life.TicketsFor(ss::fuse::TicketId(0)) == 1,
		"ticket: holding a FUSE ticket, LIGHT IT still pays from the wallet and keeps the ticket");
	PlayOut(*R);
	S.LeaveResults();
	A = Wallet(*R);
	const Chips W1 = A->Cents, Rake1 = A->RakeLife;
	const ss::site::play::FuseRec Before = S.Life.SitePlay.Ig.Fuse;
	const int64_t Base1 = S.Life.SitePlay.Ig.S.BaseLife, Sparks1 = S.Life.SitePlay.Ig.S.Balance;
	const int Expected = ss::fuse::Draw(S.WorldSeed(), 0, 2);
	const std::string Used = S.LightFuse(0, true);
	Expect(S.Life.SitePlay.Ig.S.BaseLife == Base1 && S.Life.SitePlay.Ig.S.Balance == Sparks1, "ticket: a FUSE on a ticket earns no Sparks (W4)");
	A = Wallet(*R);
	const ss::site::play::FuseRec& After = S.Life.SitePlay.Ig.Fuse;
	Expect(Used.empty() && A->Cents == W1 && A->RakeLife == Rake1 && S.Life.TicketsFor(ss::fuse::TicketId(0)) == 0 && S.Life.TicketsFor("ignite.fuse0") == 0 &&
			A->Lines.back().Label == "Ticket: FUSE $1.10" && A->Lines.back().Cents == 0,
		"ticket: USE A TICKET spends the ticket, no money and no rake; the wallet's line \"Ticket: FUSE $1.10\" $0.00 (" + Used + ")");
	Expect(After.N[0] == 2 && After.RecentBuyIns == Before.RecentBuyIns && After.Last10[0] == Expected && S.T && S.T->PrizePoolCents == Expected * 110,
		"ticket: it lights the next FUSE in the tier's sequence (n 2, x" + std::to_string(Expected) + ") and earns nothing (no buy-in for the band, no money)");
	const int Entries1 = S.Life.SitePlay.Ig.S.DayEntries;
	const std::array<int16_t, 3> Prog1 = S.Life.SitePlay.Ig.D.Prog;
	PlayOut(*R);
	// Its result moves no loop either (W4: core-siteplay's OnResult hears it unpaid): no wheel entry, no Dare progress, no Sparks.
	Expect(S.Life.SitePlay.Ig.S.DayEntries == Entries1 && S.Life.SitePlay.Ig.D.Prog == Prog1 && S.Life.SitePlay.Ig.S.BaseLife == Base1 &&
			S.Life.SitePlay.Ig.S.Balance == Sparks1 && S.Life.SitePlay.Ig.Fuse.Played == Before.Played + 1,
		"ticket: its result counts on the record and moves no loop (no wheel entry, Dare progress or Sparks)");
	S.LeaveResults();
	const std::string None = S.LightFuse(0, true);
	Expect(None == "There's no FUSE ticket at that stake on your account." && S.Life.SitePlay.Ig.Fuse.N[0] == 2 && S.TableCount() == 0,
		"ticket: no ticket left, USE A TICKET is refused (\"" + None + "\")");
	// A ticket left under the entry's own id (never by LightFuse) would turn a paid LIGHT IT into a ticket entry in RegisterEvent:
	// LightFuse moves it to the FUSE ticket id first, so the paid light pays (review-core-fuse: a mutant without the move passed).
	S.Life.Tickets["ignite.fuse0"] = 1;
	A = Wallet(*R);
	const Chips W2 = A->Cents;
	const std::string Stray = S.LightFuse(0, false);
	A = Wallet(*R);
	bool BuyLine = false;
	for (size_t K = A->Lines.size() >= 2 ? A->Lines.size() - 2 : 0; K < A->Lines.size(); ++K)
	{
		BuyLine = BuyLine || (A->Lines[K].Label == "Buy-in: FUSE $1.10" && A->Lines[K].Cents == -110);
	}
	Expect(Stray.empty() && A->Cents == W2 - 110 && BuyLine && S.Life.TicketsFor(ss::fuse::TicketId(0)) == 1 && S.Life.TicketsFor("ignite.fuse0") == 0 &&
			S.Life.SitePlay.Ig.Fuse.N[0] == 3,
		"ticket: a stray ticket under ignite.fuse0 never makes a paid LIGHT IT free; it moves to the FUSE ticket id (" + Stray + ")");
	PlayOut(*R);
	S.LeaveResults();
}

void Refusals()
{
	{
		std::unique_ptr<Rig> R = MakeRig("fuse-noacct", 20000, false);
		ss::Session& S = *R->S;
		const Chips Bank0 = S.BankrollCents;
		const std::string Why = S.LightFuse(0, false);
		Expect(!Why.empty() && S.TableCount() == 0 && S.BankrollCents == Bank0 && S.Life.SitePlay.Ig.Fuse.N[0] == 0 && (S.Life.Update3LinesRead & ss::life::U3SitePlay) == 0,
			"refused: no Ignite account, no FUSE (\"" + Why + "\"), and the book stays unwritten");
		Expect(!S.LightFuse(3, false).empty() && !S.LightFuse(-1, true).empty(), "refused: a tier past the three");
	}
	{
		// Two at once.
		std::unique_ptr<Rig> R = MakeRig("fuse-two");
		ss::Session& S = *R->S;
		const std::string One = S.LightFuse(0, false), Two = S.LightFuse(1, false);
		ss::site::Account* A = Wallet(*R);
		const Chips W0 = A->Cents;
		const int Saves0 = R->H.Saves;
		const std::string Three = S.LightFuse(2, false);
		Expect(One.empty() && Two.empty() && !Three.empty() && S.TableCount() == 2 && A->Cents == W0 && R->H.Saves == Saves0 && S.Life.SitePlay.Ig.Fuse.N[2] == 0,
			"refused: a third FUSE at once (\"" + Three + "\"): no table, no debit, no save, no ordinal");
	}
	for (const std::string Only : {"riverline", "runittwice"})
	{
		// The core strike gate: Team RiverLine's clause flags every rival, Team RunItTwice's flags Ignite.
		std::unique_ptr<Rig> R = MakeRig("fuse-gate-" + Only);
		ss::Session& S = *R->S;
		AddClause(S, Only);
		S.Life.Tickets[ss::fuse::TicketId(1)] = 1;
		ss::site::Account* A = Wallet(*R);
		const Chips W0 = A->Cents, Rake0 = A->RakeLife;
		const int Saves0 = R->H.Saves;
		const uint32_t Bits0 = S.Life.Update3LinesRead;
		const std::string Why = S.LightFuse(0, false);
		const std::string WhyTicket = S.LightFuse(1, true);
		const int Strikes0 = S.Life.Sponsors.Contracts.back().Strikes;
		const bool Nothing = S.TableCount() == 0 && A->Cents == W0 && A->RakeLife == Rake0 && Strikes0 == 0 && R->H.Saves == Saves0 && S.Life.SitePlay.Ig.Fuse.N[0] == 0 &&
			S.Life.SitePlay.Ig.Fuse.N[1] == 0 && S.Life.SitePlay.Ig.Fuse.Last10[0] == 0 && S.Life.Update3LinesRead == Bits0 && S.Life.TicketsFor(ss::fuse::TicketId(1)) == 1 &&
			S.Life.TicketsFor("ignite.fuse1") == 0;
		const std::string Took = S.LightFuse(0, false, ss::StrikeConfirm{true});
		A = Wallet(*R);
		Expect(!Why.empty() && !WhyTicket.empty() && Why.find("strike") != std::string::npos && Nothing && Took.empty() && S.Life.Sponsors.Contracts.back().Strikes == 1 &&
				A->Cents == W0 - 110 && S.Life.SitePlay.Ig.Fuse.N[0] == 1,
			"gate: only=" + Only + ": LIGHT IT and USE A TICKET refused without the token (\"" + Why + "\": no table, debit, rake, strike, save, ordinal or ticket), struck once with it");
	}
}

void Reload()
{
	// A reload after the light can't take the entry back: the save carries the ordinal, so the next light is the next draw.
	std::unique_ptr<Rig> R = MakeRig("fuse-reload");
	ss::Session& S = *R->S;
	const std::string Why = S.LightFuse(2, false);
	const std::string Saved = R->H.Last;
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(Saved, D);
	FuseHooks H2;
	ss::Session S2(H2, "fuse-reload", &D);
	S2.CurrentScreen = ss::Screen::Lobby;
	S2.Update(100.0);
	const int Next = ss::fuse::Draw(S2.WorldSeed(), 2, 2);
	const std::string Again = S2.LightFuse(2, false);
	Expect(Why.empty() && Parsed && D.Life.SitePlay.Ig.Fuse.N[2] == 1 && Again.empty() && S2.T && ss::fuse::OrdinalOf(S2.T->Spec.Id) == 2 && S2.T->PrizePoolCents == Next * 550,
		"reload: the saved ordinal stands; the next light after a reload is the tier's next draw, never a reroll");
	// BandOf (core-siteplay's) reads FUSE's paid buy-ins, so a FUSE-only player has a band: two $5.50s are band B.
	if (ss::site::play::LevelNeeds(2) > 0)
	{
		Expect(ss::site::play::BandOf(S2.Life.SitePlay, S2.History) == 1 && S2.History.empty(), "reload: BandOf sees FUSE's buy-ins (two $5.50s, no History: band B)");
	}
	// The save round-trips: the fuse line written back the same.
	ss::SaveData D2;
	const bool Parsed2 = ss::SaveData::Parse(H2.Last, D2);
	Expect(Parsed2 && D2.Serialize() == H2.Last && FuseLineOf(H2.Last).rfind("site\tfuse\tignite\t0\t0\t2\t", 0) == 0, "reload: the save with a FUSE record round-trips byte for byte");
}

/** The front table played out at Sprint as PlayOut does, noting the banners it raised and the system and dealer lines. */
void PlayOutNoting(Rig& R, std::vector<std::string>& Banners, std::vector<std::string>& Lines)
{
	ss::Session& S = *R.S;
	S.SprintBudgetMs = 1.0e9;
	std::string Last;
	size_t Seen = 0;
	for (int F = 0; S.CurrentScreen == ss::Screen::Table && F < 400000; ++F)
	{
		S.CurrentPace = ss::Pace::Sprint;
		R.Now += 1.0 / 30.0;
		S.Update(R.Now);
		if (S.HasPrompt)
		{
			S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
		}
		if (S.CurrentBanner.Active && S.CurrentBanner.Title + " / " + S.CurrentBanner.Sub != Last)
		{
			Last = S.CurrentBanner.Title + " / " + S.CurrentBanner.Sub;
			Banners.push_back(Last);
		}
		Seen = std::min(Seen, S.Chat.size());
		for (; Seen < S.Chat.size(); ++Seen)
		{
			if (S.Chat[Seen].Kind == ss::ChatKind::System || S.Chat[Seen].Kind == ss::ChatKind::Dealer)
			{
				Lines.push_back(S.Chat[Seen].Text);
			}
		}
	}
}

void Table3()
{
	// What the session says and pays at a 3-max table, and who won it (the lead's Session.cpp FUSE branches,
	// patches/core-fuse_to_lead_1.diff): no MTT copy, the paid split on the banners, no "You did WHAT?!" on every win, and the
	// table played out after the player's bust so Big hits can name the winner.
	std::unique_ptr<Rig> R = MakeRig("fuse-table3", 200000);
	ss::Session& S = *R->S;
	std::vector<std::string> Banners, Lines, Wrong;
	int Games = 0, Wins = 0, Decided = 0, Short = 0, Busts = 0;
	bool BannersPaid = true;
	for (int K = 0; K < 8; ++K)
	{
		if (!ss::fuse::DebugForce(S, 0, 25).empty() || !S.LightFuse(0, false).empty())
		{
			break;
		}
		std::vector<std::string> B;
		PlayOutNoting(*R, B, Lines);
		const int Place = S.LastResults.Place;
		++Games;
		Wins += Place == 1 ? 1 : 0;
		Busts += Place != 1 ? 1 : 0;
		bool Winner = false;
		for (const ss::TPlayer& P : S.T->Players)
		{
			Winner = Winner || P.Place == 1;
		}
		Decided += Winner ? 1 : 0;
		Short += !Winner && Place != 1 ? 1 : 0;
		// The banner at the end: the session's split, never the engine's whole pool or "Eliminated" for a paid place.
		const std::string Want = Place == 1 ? "YOU WON THE FUSE / $22.00" : Place == 2 ? "You finished 2nd / Won $3.30" : "You finished 3rd / Won $2.20";
		// (A win at Sprint raises no banner on the skip path, as for any event; a bust always does.)
		const bool Shown = std::find(B.begin(), B.end(), Want) != B.end();
		BannersPaid = BannersPaid && (Shown || (Place == 1 && std::none_of(B.begin(), B.end(), [](const std::string& X) { return X.find("WON") != std::string::npos; })));
		if (!Shown && Place != 1)
		{
			std::string All;
			for (const std::string& X : B)
			{
				All += " [" + X + "]";
			}
			Wrong.push_back("3-max: place " + std::to_string(Place) + " banners:" + All);
		}
		Banners.insert(Banners.end(), B.begin(), B.end());
		S.LeaveResults();
	}
	bool Welcome = false, Mtt = false;
	for (const std::string& L : Lines)
	{
		Welcome = Welcome || L == "Welcome to FUSE $1.10. Three players, a $27.50 prize.";
		Mtt = Mtt || L.find("bubble") != std::string::npos || L.find("Hand-for-hand") != std::string::npos || L.find("Final table") != std::string::npos ||
			L.find("1 paid") != std::string::npos || L.find("ante") != std::string::npos;
	}
	for (const std::string& B : Banners)
	{
		Mtt = Mtt || B.find("FINAL TABLE") != std::string::npos || B.find("HAND FOR HAND") != std::string::npos || B.find("BUBBLE") != std::string::npos ||
			B.find("IN THE MONEY") != std::string::npos || B.find("TOURNAMENT") != std::string::npos;
	}
	int Dee = 0;
	for (const std::string& T : R->H.Texts)
	{
		Dee += T.find("You did WHAT") != std::string::npos ? 1 : 0;
	}
	const ss::site::play::FuseRec& Rec = S.Life.SitePlay.Ig.Fuse;
	if (!Welcome)
	{
		Note("3-max: the session still speaks MTT at a FUSE table (" + std::to_string(Short) + " of " + std::to_string(Busts) +
			" busts left no winner): waits on patches/core-fuse_to_lead_1.diff (Session.cpp, the lead's)");
		return;
	}
	Expect(Games == 8 && !Mtt && Dee == 0, "3-max: eight x25 FUSEs: no hand for hand, bubble, final table, ante or \"1 paid\", and no \"You did WHAT?!\" text (" +
		std::to_string(Wins) + " won)");
	for (const std::string& W : Wrong)
	{
		Note(W);
	}
	Expect(BannersPaid, "3-max: every finish's banner shows the session's split (1st $22.00, 2nd $3.30, 3rd $2.20)");
	Expect(Decided == Games && Short == 0 && static_cast<int>(Rec.Hits.size()) == std::min(Games, 10) && !Rec.Hits.front().Winner.empty(),
		"3-max: the table plays out after the player's bust, so every x25 names its winner on Big hits (" + std::to_string(Rec.Hits.size()) + ")");
	Expect(Rec.Played == Games && Rec.Won == Wins && Rec.Wall.size() == static_cast<size_t>(std::min(Games, 10)) && Rec.BestM == 25,
		"3-max: the record: played " + std::to_string(Rec.Played) + ", won " + std::to_string(Rec.Won) + ", the Wall " + std::to_string(Rec.Wall.size()));
	// The watch path (Smart pace, the player answering every prompt): its win banner is the session's first prize too.
	int Watched = 0, Right = 0;
	for (int K = 0; K < 4; ++K)
	{
		if (!ss::fuse::DebugForce(S, 1, 25).empty() || !S.LightFuse(1, false).empty())
		{
			break;
		}
		std::vector<std::string> B;
		std::string Last;
		for (int F = 0; S.CurrentScreen == ss::Screen::Table && F < 600000; ++F)
		{
			R->Now += 0.1;
			S.Update(R->Now);
			if (S.HasPrompt)
			{
				S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
			}
			if (S.CurrentBanner.Active && S.CurrentBanner.Title + " / " + S.CurrentBanner.Sub != Last)
			{
				Last = S.CurrentBanner.Title + " / " + S.CurrentBanner.Sub;
				B.push_back(Last);
			}
		}
		const int Place = S.LastResults.Place;
		const std::string Want = Place == 1 ? "YOU WON THE FUSE / $66.00" : Place == 2 ? "You finished 2nd / Won $9.90" : "You finished 3rd / Won $6.60";
		++Watched;
		Right += std::find(B.begin(), B.end(), Want) != B.end() ? 1 : 0;
		S.LeaveResults();
	}
	Expect(Watched == 4 && Right == 4, "3-max: watched, every x25 at $3.30 ends on the session's split (1st $66.00, 2nd $9.90, 3rd $6.60): " + std::to_string(Right) + " of 4");
}

void Dev()
{
	std::unique_ptr<Rig> R = MakeRig("fuse-dev");
	ss::Session& S = *R->S;
	Expect(!ss::fuse::DebugForce(S, 0, 250).empty() && !ss::fuse::DebugForce(S, 1, 1000).empty() && !ss::fuse::DebugForce(S, 5, 2).empty(),
		"dev: a forced multiplier must be on the tier's table");
	Expect(ss::fuse::DebugForce(S, 0, 25).empty(), "dev: ss.Site.Play fuse 0 25 forces the next $1.10 FUSE");
	const std::string Why = S.LightFuse(0, false);
	bool Logged = false;
	for (const ss::ChatLine& L : S.Chat)
	{
		Logged = Logged || L.Text.find("set by hand") != std::string::npos;
	}
	Expect(Why.empty() && S.T && S.T->PrizePoolCents == 25 * 110 && S.FuseReveal(-1).M == 25 && S.Life.SitePlay.Ig.Fuse.N[0] == 1 && Logged,
		"dev: the forced FUSE plays for x25, its ordinal still advances, and its table says the multiplier was set by hand");
	PlayOut(*R);
	S.LeaveResults();
	const std::string Next = S.LightFuse(0, false);
	Expect(Next.empty() && S.T && S.T->PrizePoolCents == ss::fuse::Draw(S.WorldSeed(), 0, 2) * 110, "dev: a force is used once; the next FUSE is its own draw");
	PlayOut(*R);
	S.LeaveResults();
	// Forget (~Session's call) drops a force and the reveal clocks with the session's side.
	ss::fuse::DebugForce(S, 0, 1000);
	ss::fuse::Forget(&S);
	const std::string Third = S.LightFuse(0, false);
	Expect(Third.empty() && S.T && S.T->PrizePoolCents == ss::fuse::Draw(S.WorldSeed(), 0, 3) * 110 && S.FuseReveal(-1).M == ss::fuse::Draw(S.WorldSeed(), 0, 3),
		"dev: Forget drops the session's FUSE side (a pending force included)");
}

/** What a FUSE's table showed of its first hand, frame by frame, from the light (FirstDeal's probe). */
struct FirstHand
{
	std::string Id;
	double PaidAt = -1.0;   // the light's clock (FuseReveal's PaidAt)
	double SeenAt = -1.0;   // the first frame FuseReveal read Seen
	double DealtAt = -1.0;  // the hold's own record of when the first deal went (fuse::FirstDealAt; -1: TableStep never asked)
	double ShownAt = -1.0;  // the first frame anything of the hand was on the table (a blind, a card, a fold)
	double PromptAt = -1.0; // the player's first decision clock (Prompt.OpenedAt)
	double Deadline = -1.0; // and its deadline
	bool HeldEarly = false; // HoldFirstDeal answered "wait" right after the light
	bool Lit = false;
};

/** Anything of the front table's hand on show: a blind or a bet, a card, a fold, a board. */
bool HandOnShow(const ss::Session& S)
{
	bool On = !S.Board.empty() || S.AutoFolded || S.HasPrompt;
	for (const ss::SeatVis& Sv : S.Seats)
	{
		On = On || (Sv.Present && (Sv.HasCards || Sv.Bet > 0 || Sv.Folded || !Sv.LastAction.empty()));
	}
	return On;
}

/**
 * Lights a FUSE forced to x M at Tier and runs frames (1/30 s) until the player's first decision clock opens or Until seconds
 * pass. Each frame Before(Rig, seconds since the light) runs first (the laptop's part: TakeSeat, RevealShown, a plan).
 */
FirstHand ProbeFirstHand(const std::string& Seed, int Tier, int M, ss::Pace Pace, double Until, const std::function<void(Rig&, double)>& Before)
{
	FirstHand Out;
	std::unique_ptr<Rig> R = MakeRig(Seed, 20000);
	ss::Session& S = *R->S;
	S.CurrentPace = Pace;
	if (!ss::fuse::DebugForce(S, Tier, M).empty() || !S.LightFuse(Tier, false).empty() || !S.T)
	{
		return Out;
	}
	Out.Lit = true;
	Out.Id = S.T->Spec.Id;
	Out.PaidAt = S.FuseReveal(-1).PaidAt;
	Out.HeldEarly = ss::fuse::HoldFirstDeal(S);
	for (int F = 0; F < static_cast<int>(Until * 30.0) && S.CurrentScreen == ss::Screen::Table; ++F)
	{
		if (Before)
		{
			Before(*R, R->Now - Out.PaidAt);
		}
		R->Now += 1.0 / 30.0;
		S.Update(R->Now);
		if (Out.SeenAt < 0.0 && S.FuseReveal(-1).Seen)
		{
			Out.SeenAt = S.Now;
		}
		if (Out.ShownAt < 0.0 && HandOnShow(S))
		{
			Out.ShownAt = S.Now;
		}
		if (S.HasPrompt)
		{
			Out.PromptAt = S.Prompt.OpenedAt;
			Out.Deadline = S.Prompt.Deadline;
			break;
		}
		if (Out.ShownAt >= 0.0 && S.Autoplay(S.FocusedTable()).M != ss::autoplay::Mode::Off)
		{
			break; // a plan's hand: no clock of the player's to wait for
		}
	}
	Out.DealtAt = ss::fuse::FirstDealAt(S, Out.Id);
	return Out;
}

/**
 * G2 M3 (ui-ignite's review; §2.4 D): the FUSE table's first deal, and the player's first decision clock, wait until the
 * reveal has been seen: its run and a second (x2, x25 and each tier's top), TAKE YOUR SEAT at once, a laptop still drawing it
 * (the Odds drawer) for as long as it does. A plan or a skip deals at once (autoplay and skips unaffected). A FUSE behind another
 * table waits on its own reveal; later hands never wait; a hold that never lets go fails. Session.cpp's TableStep asks
 * fuse::HoldFirstDeal (patches/g2f-fuse_to_lead_1.diff); until that lands the end-to-end checks print a note.
 */
void FirstDeal()
{
	const double Frame = 1.0 / 30.0;
	const double Beat = 0.25;   // BeginHand's beat before a hand's first event, from the last frame the hold kept it
	const double Eps = 1e-6;    // the frames' clock is a running sum of 1/30 s
	bool GateIn = true;
	// The end-to-end checks run once TableStep asks the hold (the lead's patch): FirstDealAt is set when it lets a deal go. A hand
	// never dealt in the probe's time is a hold that never lets go, never a missing patch (without it the hand deals at once).
	auto Gate = [&GateIn](const FirstHand& H, const std::string& Name) {
		Expect(H.ShownAt >= 0.0, "first deal: " + Name + ": the hand is dealt (" + (H.ShownAt >= 0.0 ? Num(H.ShownAt - H.PaidAt, 2) + " s after the light" : std::string("never: a hold that doesn't let go")) + ")");
		GateIn = GateIn && (H.DealtAt >= 0.0 || H.ShownAt < 0.0);
		return H.DealtAt >= 0.0;
	};
	// The first event comes a beat after the last frame the hold kept: between Released - a frame + the beat and a frame later.
	auto OnBeat = [&](double Released, double Shown) { return Shown >= Released - Frame + Beat - Eps && Shown <= Released + Beat + Frame + Eps; };
	// x2, x25 and the top of the $1.10 and $5.50 tables, the player's own seat (Full: every hand to the player).
	struct Case
	{
		int Tier, M;
	};
	const Case Cases[] = {{0, 2}, {0, 25}, {0, ss::fuse::TopOf(0)}, {2, ss::fuse::TopOf(2)}};
	for (const Case& C : Cases)
	{
		const std::string Name = ss::Money(ss::fuse::BuyInOf(C.Tier)) + " x" + std::to_string(C.M);
		const FirstHand H = ProbeFirstHand("fuse-firstdeal-" + std::to_string(C.Tier) + "-" + std::to_string(C.M), C.Tier, C.M, ss::Pace::Full, 40.0, {});
		const double Run = ss::fuse::RevealSeconds(C.M) + 1.0;
		Expect(H.Lit && H.HeldEarly, "first deal: " + Name + ": HoldFirstDeal holds the table's first hand from the light");
		if (!H.Lit || !Gate(H, Name))
		{
			continue;
		}
		Expect(H.SeenAt >= H.PaidAt + Run - Eps && H.SeenAt <= H.PaidAt + Run + Frame + Eps && H.DealtAt == H.SeenAt,
			"first deal: " + Name + ": the reveal is seen " + Num(H.SeenAt - H.PaidAt, 2) + " s after the light (its run and a second, " + Num(Run, 1) +
				" s), and the first deal is let go on that frame");
		Expect(H.ShownAt > H.SeenAt && OnBeat(H.SeenAt, H.ShownAt), "first deal: " + Name + ": nothing of the first hand shows under the reveal; its blinds go in " +
			Num(H.ShownAt - H.PaidAt, 2) + " s after the light");
		Expect(H.PromptAt > H.SeenAt && H.PromptAt >= H.ShownAt && H.Deadline > H.PromptAt, "first deal: " + Name + ": the player's first decision clock opens " +
			Num(H.PromptAt - H.PaidAt, 2) + " s after the light, after the reveal (" + Num(H.Deadline - H.PromptAt, 1) + " s on it)");
		Note("first deal: " + Name + ": seen " + Num(H.SeenAt - H.PaidAt, 2) + " s, blinds in " + Num(H.ShownAt - H.PaidAt, 2) + " s, the player's clock " +
			Num(H.PromptAt - H.PaidAt, 2) + " s after the light");
	}
	// Smart pace: nothing is auto-folded under the reveal either.
	{
		const FirstHand H = ProbeFirstHand("fuse-firstdeal-smart", 0, 2, ss::Pace::Smart, 40.0, {});
		if (H.Lit && Gate(H, "Smart pace x2"))
		{
			Expect(H.ShownAt > H.SeenAt && H.SeenAt >= H.PaidAt + ss::fuse::RevealSeconds(2) + 1.0 - Eps,
				"first deal: Smart pace: no blind, card or auto-fold before the x2 reveal is seen (" + Num(H.ShownAt - H.PaidAt, 2) + " s)");
		}
	}
	// TAKE YOUR SEAT 2 s into a x25's 7.3 s: the hand deals on the next beat.
	{
		double TookAt = -1.0;
		const FirstHand H = ProbeFirstHand("fuse-firstdeal-seat", 0, 25, ss::Pace::Full, 40.0, [&TookAt](Rig& R, double Since) {
			if (TookAt < 0.0 && Since >= 2.0)
			{
				ss::fuse::TakeSeat(*R.S, R.S->T->Spec.Id);
				TookAt = R.S->Now;
			}
		});
		if (H.Lit && Gate(H, "TAKE YOUR SEAT at a x25"))
		{
			Expect(TookAt > 0.0 && H.SeenAt > TookAt && H.SeenAt <= TookAt + Frame + Eps && OnBeat(H.SeenAt, H.ShownAt) && H.PromptAt >= H.ShownAt,
				"first deal: TAKE YOUR SEAT 2 s into a x25 seats the player at once: seen " + Num(H.SeenAt - H.PaidAt, 2) + " s, blinds in " + Num(H.ShownAt - H.PaidAt, 2) +
					" s after the light, the clock after them");
		}
	}
	// The laptop still drawing a x2's reveal at 5 s (its Odds drawer open): no deal while it does; then a second, then the beat.
	{
		double LastShown = -1.0;
		const FirstHand H = ProbeFirstHand("fuse-firstdeal-odds", 0, 2, ss::Pace::Full, 40.0, [&LastShown](Rig& R, double Since) {
			if (Since < 5.0)
			{
				ss::fuse::RevealShown(*R.S, R.S->T->Spec.Id);
				LastShown = R.S->Now;
			}
		});
		if (H.Lit && Gate(H, "the Odds drawer at a x2"))
		{
			Expect(LastShown > 0.0 && H.SeenAt >= LastShown + 1.0 - Eps && H.SeenAt <= LastShown + 1.0 + Frame + Eps && OnBeat(H.SeenAt, H.ShownAt),
				"first deal: a reveal the laptop keeps up (the Odds drawer, 5 s at x2) holds the deal: seen a second after its last frame (" + Num(H.SeenAt - H.PaidAt, 2) +
					" s), blinds in " + Num(H.ShownAt - H.PaidAt, 2) + " s after the light");
		}
	}
	// A plan or a skip from the light deals at once (autoplay and skips unaffected): the blinds go in a beat after the light.
	{
		std::string PlanWhy = "not set";
		const FirstHand Plan = ProbeFirstHand("fuse-firstdeal-plan", 0, 25, ss::Pace::Smart, 20.0, [&PlanWhy](Rig& R, double Since) {
			if (Since <= 0.0)
			{
				ss::autoplay::TableAuto Au;
				Au.S = ss::style::Style::Autopilot;
				Au.M = ss::autoplay::Mode::Watch;
				Au.Speed = 1;
				Au.Stops = 0;
				PlanWhy = R.S->SetAutoplay(0, Au);
			}
		});
		Expect(PlanWhy.empty(), "first deal: the plan took the FUSE's seat at the light (" + PlanWhy + ")");
		const FirstHand Sprint = ProbeFirstHand("fuse-firstdeal-sprint", 0, 25, ss::Pace::Smart, 20.0, [](Rig& R, double Since) {
			if (Since <= 0.0)
			{
				R.S->CurrentPace = ss::Pace::Sprint;
			}
		});
		const bool PlanIn = Plan.Lit && Gate(Plan, "a plan at a x25");
		const bool SprintIn = Sprint.Lit && Gate(Sprint, "Sprint at a x25");
		if (PlanIn && SprintIn)
		{
			Expect(OnBeat(Plan.PaidAt + Frame, Plan.ShownAt) && OnBeat(Sprint.PaidAt + Frame, Sprint.ShownAt) && Plan.DealtAt <= Plan.PaidAt + Frame + Eps &&
					Sprint.DealtAt <= Sprint.PaidAt + Frame + Eps,
				"first deal: a plan (watched) or Sprint from the light deals at once, as before: blinds in " + Num(Plan.ShownAt - Plan.PaidAt, 2) + " s and " +
					Num(Sprint.ShownAt - Sprint.PaidAt, 2) + " s");
		}
	}
	// Never another table: a RiverLine or an Ignite MTT's first hand isn't held, whatever the clock.
	{
		std::unique_ptr<Rig> R = MakeRig("fuse-firstdeal-mtt", 20000);
		ss::Session& S = *R->S;
		bool Held = false;
		int Tables = 0;
		for (const ss::net::SiteId Site : {ss::net::SiteId::RiverLine, ss::net::SiteId::Ignite})
		{
			const ss::net::Network& Net = ss::net::Site(Site);
			for (const ss::net::EventInstance& I : Net.Window(S.WorldMinutes() - 30.0, S.WorldMinutes() + 600.0))
			{
				const ss::LobbyEvent L = Net.Listing(I, nullptr, S.Unlocks());
				if (Tables < 2 && L.Joinable && L.BuyInCents > 0 && L.BuyInCents <= 1100 && S.TableCount() == Tables && S.RegisterEvent(L).empty())
				{
					++Tables;
					Held = Held || ss::fuse::HoldFirstDeal(S);
					break;
				}
			}
		}
		Expect(!Held, "first deal: no other table's first hand is held (" + std::to_string(Tables) + " MTT tables opened)");
		if (Tables == 0)
		{
			Note("first deal: no RiverLine or Ignite MTT open to sit in this rig, so the other-tables check saw none");
		}
	}
	// A FUSE beside another open table (review-g2f-fuse): lit while a first FUSE deals, it's table 1. It waits on its own reveal,
	// in front and behind (the first table put in front a second in, as AutoFocusStep does on the player's turn there), never on
	// the other table's; then it deals on the beat.
	{
		std::unique_ptr<Rig> R = MakeRig("fuse-firstdeal-beside", 20000);
		ss::Session& S = *R->S;
		bool Ok = ss::fuse::DebugForce(S, 0, 2).empty() && S.LightFuse(0, false).empty() && S.T;
		const std::string FirstId = Ok ? S.T->Spec.Id : std::string();
		if (Ok)
		{
			ss::fuse::TakeSeat(S, FirstId);
			Pump(*R, 1.0);
		}
		Ok = Ok && ss::fuse::DebugForce(S, 0, 25).empty() && S.LightFuse(0, false).empty() && S.TableCount() == 2 && S.FocusedTable() == 1;
		FirstHand H;
		H.Id = Ok ? S.T->Spec.Id : std::string();
		H.PaidAt = Ok ? S.FuseReveal(1).PaidAt : 0.0;
		bool Behind = Ok;
		for (int F = 0; Ok && F < 450 && H.ShownAt < 0.0; ++F)
		{
			if (F == 30)
			{
				S.FocusTable(0);
			}
			R->Now += Frame;
			S.Update(R->Now);
			int K = -1;
			for (int T = 0; T < S.TableCount(); ++T)
			{
				K = S.Glance(T).EventId == H.Id ? T : K;
			}
			if (K < 0)
			{
				break;
			}
			Behind = Behind && (F < 30 || S.FocusedTable() != K || H.SeenAt >= 0.0);
			if (H.SeenAt < 0.0 && S.FuseReveal(K).Seen)
			{
				H.SeenAt = S.Now;
			}
			bool Show = false;
			S.WithTable(K, [&]() { Show = HandOnShow(S); });
			H.ShownAt = Show ? S.Now : H.ShownAt;
		}
		H.DealtAt = Ok ? ss::fuse::FirstDealAt(S, H.Id) : -1.0;
		Expect(Ok, "first deal: a second FUSE lights beside a first that's dealing (table 1, in front)");
		if (Ok && Gate(H, "a x25 beside another FUSE"))
		{
			const double Run = ss::fuse::RevealSeconds(25) + 1.0;
			Expect(Behind && H.SeenAt >= H.PaidAt + Run - Eps && H.SeenAt <= H.PaidAt + Run + Frame + Eps && H.DealtAt == H.SeenAt && OnBeat(H.SeenAt, H.ShownAt),
				"first deal: a FUSE beside another table waits on its own reveal, in front and behind: seen " +
					(H.SeenAt >= 0.0 ? Num(H.SeenAt - H.PaidAt, 2) + " s" : std::string("never")) + ", blinds in " + Num(H.ShownAt - H.PaidAt, 2) + " s after the light");
			Note("first deal: a x25 beside a dealing FUSE, behind from 1 s: seen " + Num(H.SeenAt - H.PaidAt, 2) + " s, blinds in " + Num(H.ShownAt - H.PaidAt, 2) +
				" s after the light");
		}
	}
	// A later hand never waits (review-g2f-fuse): a x2 whose first deal went by the clock (no seat taken), its reveal drawn again
	// once the first hand is on show (a laptop rebuilt mid-reveal draws it once more), deals its next hands as ever.
	{
		std::unique_ptr<Rig> R = MakeRig("fuse-firstdeal-later", 20000);
		ss::Session& S = *R->S;
		const bool Lit = ss::fuse::DebugForce(S, 0, 2).empty() && S.LightFuse(0, false).empty() && S.T;
		const std::string Id = Lit ? S.T->Spec.Id : std::string();
		FirstHand H;
		H.PaidAt = Lit ? S.FuseReveal(-1).PaidAt : 0.0;
		bool OnTable = false;
		double SecondAt = -1.0;
		for (int F = 0; Lit && F < 30 * 60 && S.CurrentScreen == ss::Screen::Table && H.ShownAt < 0.0; ++F)
		{
			OnTable = OnTable || HandOnShow(S);
			if (OnTable)
			{
				ss::fuse::RevealShown(S, Id);
			}
			R->Now += Frame;
			S.Update(R->Now);
			if (S.HasPrompt)
			{
				S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
			}
			SecondAt = SecondAt < 0.0 && S.HandsPlayed >= 2 ? S.Now : SecondAt;
			H.ShownAt = SecondAt >= 0.0 && HandOnShow(S) ? S.Now : -1.0;
		}
		H.DealtAt = Lit ? ss::fuse::FirstDealAt(S, Id) : -1.0;
		if (Lit && Gate(H, "a x2's second hand"))
		{
			Expect(SecondAt >= 0.0 && H.ShownAt >= SecondAt && H.ShownAt <= SecondAt + Beat + 2.0 * Frame + Eps,
				"first deal: only the first hand waits: with the reveal drawn again during the first, the second's blinds go in " +
					(SecondAt >= 0.0 && H.ShownAt >= 0.0 ? Num(H.ShownAt - SecondAt, 2) + " s after it's dealt" : std::string("never")));
		}
	}
	// L8 (with ui-ignite / ui-tablestyle): a FUSE has no ante at any level, so a head that prints the ante only while there is
	// one never prints "ante 0" at a FUSE.
	{
		std::unique_ptr<Rig> R = MakeRig("fuse-noante", 20000);
		ss::Session& S = *R->S;
		bool NoAnte = S.LightFuse(1, false).empty() && S.T && !S.T->Levels.empty() && !S.T->Spec.Levels.empty();
		for (const ss::Level& L : S.T ? S.T->Levels : std::vector<ss::Level>())
		{
			NoAnte = NoAnte && L.Ante == 0;
		}
		for (const ss::Level& L : S.T ? S.T->Spec.Levels : std::vector<ss::Level>())
		{
			NoAnte = NoAnte && L.Ante == 0;
		}
		Expect(NoAnte, "first deal: a FUSE table's every level has no ante (the side panel's head shows none, L8)");
	}
	if (!GateIn)
	{
		Note("first deal (M3): Session.cpp's TableStep never asked fuse::HoldFirstDeal, so the FUSE's first deal still comes under the reveal: apply "
			 "patches/g2f-fuse_to_lead_1.diff; the end-to-end checks above were skipped");
	}
}

// ------------------------------------------------------------------ long: the place odds and the styles

struct Hero
{
	enum class Kind
	{
		Style,
		CoachBest,
	};
	Kind K = Kind::Style;
	ss::style::Style S = ss::style::Style::Autopilot;
	double Sharp = 1.0;
	std::string Label;
};

struct Game
{
	int Place = 0, Ticks = 0, FullTicks = 0, Illegal = 0;
};

/** Smart pace's auto-fold of junk (styles_main.cpp's SmartFolds, as S-J played it). */
bool SmartFolds(const ss::PlayerView& V)
{
	if (V.CurrentStreet != ss::Street::Preflop || V.CanCheck || V.Hole.size() != 2)
	{
		return false;
	}
	const double Price = static_cast<double>(V.ToCall) / static_cast<double>(std::max<Chips>(1, V.Pot + V.ToCall));
	if (Price < 0.25)
	{
		return false;
	}
	const double Pct = ss::HandPercentile(V.Hole[0], V.Hole[1]);
	const ss::StreetSummary Sum = ss::PreflopSummary(V);
	const ss::PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double StackBb = static_cast<double>(Me.Stack + Me.StreetBet) / static_cast<double>(std::max<Chips>(1, V.BigBlind));
	if (StackBb <= 15.0 && Sum.Raises == 0)
	{
		return Pct > ss::PushRange(StackBb, ss::PlayersBehind(V, V.Me)) * 1.4;
	}
	if (Sum.Raises == 0)
	{
		static const double Open[9] = {0.55, 0.5, 1.0, 0.16, 0.18, 0.2, 0.24, 0.28, 0.36};
		return Pct > Open[static_cast<int>(ss::PositionOf(V, V.Me))];
	}
	if (V.Me == V.BbIdx)
	{
		return Pct > 0.5;
	}
	return Pct > 0.22;
}

ss::PlayerAction CoachChoice(const ss::PlayerView& V, ss::Rng& GradeRng)
{
	const ss::DecisionAnalysis A = ss::AnalyzeDecision(V, GradeRng, 1200);
	if (A.Options.empty())
	{
		return V.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
	}
	size_t Best = 0;
	for (size_t I = 1; I < A.Options.size(); ++I)
	{
		Best = A.Options[I].Ev > A.Options[Best].Ev ? I : Best;
	}
	return A.Options[Best].Action;
}

/** One FUSE to the hero's finish (full-strength bots, the tick order kept), then the other two played out. */
Game PlayFuse(const ss::TournamentSpec& Spec, const std::string& Seed, const Hero& Who)
{
	Game Out;
	ss::Tournament T(Spec, "Hero", Seed);
	T.FastField = false;
	ss::reads::Table Seen;
	ss::style::HeroBrain Brain;
	Brain.S = Who.S;
	Brain.Sharp = Who.Sharp;
	Brain.Seen = &Seen;
	ss::Rng GradeRng(Seed + ":grade");
	for (int Guard = 0; !T.bFinished && !T.Hero().Busted && Guard < 20000; ++Guard)
	{
		std::vector<ss::TEvent> Events;
		std::unique_ptr<ss::Hand> H = T.StartTick(Events);
		++Out.Ticks;
		if (H)
		{
			for (int Steps = 0; !H->bComplete && Steps < 1000; ++Steps)
			{
				const int Idx = H->ToAct;
				const bool IsHero = H->Seats[static_cast<size_t>(Idx)].Id == ss::HeroId;
				ss::BotDecision D;
				if (IsHero && !T.HeroSitsOut)
				{
					const ss::PlayerView V = ss::MakeView(*H, Idx);
					if (Who.K == Hero::Kind::Style)
					{
						D = T.HeroDecisionFor(*H, Brain);
					}
					else
					{
						D.Action = SmartFolds(V) ? ss::PlayerAction::Fold() : CoachChoice(V, GradeRng);
					}
				}
				else if (IsHero)
				{
					const ss::LegalActions L = H->GetLegalActions();
					D.Action = L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
				}
				else
				{
					D = T.BotDecisionFor(*H, false);
				}
				if (!H->Act(D.Action))
				{
					Out.Illegal += IsHero ? 1 : 0;
					const ss::LegalActions L = H->GetLegalActions();
					H->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				}
			}
			int HeroIdx = 0;
			for (size_t I = 0; I < H->Seats.size(); ++I)
			{
				HeroIdx = H->Seats[I].Id == ss::HeroId ? static_cast<int>(I) : HeroIdx;
			}
			ss::reads::Count(*H, HeroIdx, Seen);
		}
		T.FinishTick(H.get(), nullptr);
	}
	const ss::TPlayer& Me = T.Hero();
	Out.Place = Me.Place > 0 ? Me.Place : std::max(1, T.HeroRank());
	Out.FullTicks = Out.Ticks;
	const ss::Profile Pilot = ss::SprintProfile();
	for (int Guard = 0; !T.bFinished && Guard < 20000; ++Guard)
	{
		T.SimulateTick(&Pilot);
		++Out.FullTicks;
	}
	return Out;
}

template <typename Fn>
void ParallelFor(int Count, int Threads, Fn&& Body)
{
	std::atomic<int> Next{0};
	std::vector<std::thread> Pool;
	for (int T = 0; T < std::max(1, std::min(Threads, Count)); ++T)
	{
		Pool.emplace_back([&]() {
			for (;;)
			{
				const int I = Next++;
				if (I >= Count)
				{
					break;
				}
				Body(I);
			}
		});
	}
	for (std::thread& T : Pool)
	{
		T.join();
	}
}

struct Odds
{
	double P1 = 0, P2 = 0, P3 = 0, PerHourFull = 0;
	int Illegal = 0;
};

Odds Simulate(const Hero& H, int Games)
{
	std::vector<Game> G(static_cast<size_t>(Games));
	ParallelFor(Games, 3, [&](int K) {
		// S-J's seeds at its own setting ("sj:<fillers>:<stack>:<k>"), so the games pair with S-J's runs.
		const std::string Seed = std::string("sj:") + ss::fuse::Fillers() + ":" + std::to_string(ss::fuse::StartingStack()) + ":" + std::to_string(K);
		G[static_cast<size_t>(K)] = PlayFuse(ss::fuse::EventFor(0, 2, 1200.0, K).Spec, Seed, H);
	});
	Odds O;
	double Full = 0;
	for (const Game& X : G)
	{
		O.P1 += X.Place == 1 ? 1 : 0;
		O.P2 += X.Place == 2 ? 1 : 0;
		O.P3 += X.Place >= 3 ? 1 : 0;
		Full += X.FullTicks;
		O.Illegal += X.Illegal;
	}
	const double N = static_cast<double>(std::max(1, Games));
	O.P1 /= N;
	O.P2 /= N;
	O.P3 /= N;
	O.PerHourFull = 60.0 / std::max(0.01, Full / N * 0.5);
	return O;
}

void Long()
{
	const auto T0 = std::chrono::steady_clock::now();
	const int Games = 2000;
	Hero Coach;
	Coach.K = Hero::Kind::CoachBest;
	Coach.Label = "coach-best";
	Hero Pilot;
	Pilot.S = ss::style::Style::Autopilot;
	Pilot.Label = "Autopilot";
	Hero Push;
	ss::style::FromId("pushfold", Push.S);
	Push.Label = "Push/Fold";
	const Odds C = Simulate(Coach, Games), A = Simulate(Pilot, Games), P = Simulate(Push, Games);
	auto Row = [](const char* Who, const Odds& O, double Want) {
		const double Se = std::sqrt(Want * (1.0 - Want) / 2000.0);
		Note(std::string("long: ") + Who + " P(1st) " + Num(O.P1, 4) + " (S-J " + Num(Want, 4) + ", SE " + Num(Se, 4) + "), P(2nd) " + Num(O.P2, 4) + ", P(3rd) " +
			Num(O.P3, 4) + "; ROI " + Pct(RoiOf(O.P1, O.P2, O.P3, 0)) + " / " + Pct(RoiOf(O.P1, O.P2, O.P3, 2)) + "; " + Num(O.PerHourFull, 2) + " FUSEs a world hour played out");
		return std::fabs(O.P1 - Want) <= 3.5 * Se;
	};
	const bool Agree = Row("coach-best", C, kOdds[0].P1) && Row("Autopilot", A, kOdds[1].P1) && Row("Push/Fold", P, kOdds[2].P1);
	// The seeds are S-J's, so on an unchanged engine these are S-J's own first 2,000 games (runs/*_high_300_*.txt: firsts 796,
	// 637 and 689): a change in how the engine plays shows up here first.
	const bool Paired = std::lround(C.P1 * Games) == 796 && std::lround(A.P1 * Games) == 637 && std::lround(P.P1 * Games) == 689;
	Note(std::string("long: the games pair exactly with S-J's runs: ") + (Paired ? "yes" : "no (the engine plays differently from S-J's tree)"));
	Expect(Agree && C.Illegal == 0 && A.Illegal == 0 && P.Illegal == 0, "long: 2,000 games a hero reproduce S-J's P(1st) within 3.5 SE, no illegal action");
	// The bands on these games (2,000 games: SE about 2.9 ROI points, so the band is checked with that margin).
	const double CoachRoi = RoiOf(C.P1, C.P2, C.P3, 0);
	Expect(CoachRoi >= 0.04 - 0.06 && CoachRoi <= 0.10 + 0.06 && RoiOf(A.P1, A.P2, A.P3, 2) <= 0.05 && RoiOf(P.P1, P.P2, P.P3, 2) <= 0.05,
		"long: coach-best's ROI inside the band within two SE (" + Pct(CoachRoi) + "); Autopilot and Push/Fold at or under +5%");
	Note("long: autoplay's FUSE volume a world hour against coach-best's, played out: Autopilot " + Num(A.PerHourFull / C.PerHourFull, 3) + "x, Push/Fold " +
		Num(P.PerHourFull / C.PerHourFull, 3) + "x");
	// Every shown autoplay style plays the 3-max table legally (150 FUSEs each, at full and 0.8 Sharpness).
	int Illegal = 0, Played = 0;
	for (const ss::style::Info& I : ss::style::Catalog())
	{
		if (I.Hidden || I.BountyOnly)
		{
			continue;
		}
		for (const double Sharp : {1.0, 0.8})
		{
			Hero H;
			H.S = I.S;
			H.Sharp = Sharp;
			std::vector<Game> G(150);
			ParallelFor(150, 3, [&](int K) { G[static_cast<size_t>(K)] = PlayFuse(ss::fuse::EventFor(0, 2, 1200.0, K).Spec, std::string("fuse-style:") + I.Id + ":" + std::to_string(K), H); });
			for (const Game& X : G)
			{
				Illegal += X.Illegal;
				++Played;
			}
		}
	}
	Expect(Illegal == 0 && Played > 0, "long: every shown autoplay style plays the 3-max table legally (" + std::to_string(Played) + " FUSEs)");
	// In a session, watched and skipped, the same table ends the same way (the multiplier was drawn at entry either way).
	auto RunOne = [](ss::autoplay::Mode Mode, int& Place, std::string& Check) {
		std::unique_ptr<Rig> R = MakeRig("fuse-watch-skip");
		ss::Session& S = *R->S;
		if (!S.LightFuse(0, false).empty())
		{
			return;
		}
		ss::autoplay::TableAuto Au;
		Au.S = ss::style::Style::Autopilot;
		Au.M = Mode;
		Au.Speed = 8;
		Au.To = ss::autoplay::Target::Result;
		Au.Stops = 0;
		if (Mode == ss::autoplay::Mode::Skip)
		{
			S.SprintBudgetMs = 0.0;
		}
		if (!S.SetAutoplay(0, Au).empty())
		{
			return;
		}
		for (int F = 0; F < 600000 && S.CurrentScreen == ss::Screen::Table; ++F)
		{
			R->Now += 0.1;
			S.Update(R->Now);
			if (S.HasPrompt && S.CurHand)
			{
				S.HeroAct(ss::PlayerAction::Fold());
				if (Mode == ss::autoplay::Mode::Skip)
				{
					S.SkipAhead(0, ss::autoplay::Target::Result);
				}
			}
			if (S.T && Check.empty() && (S.T->bFinished || S.T->Hero().Busted))
			{
				Check = S.T->Checkpoint() + ":" + std::to_string(S.T->PrizePoolCents);
			}
		}
		Place = S.LastResults.Place;
	};
	int WatchPlace = 0, SkipPlace = 0;
	std::string WatchCheck, SkipCheck;
	RunOne(ss::autoplay::Mode::Watch, WatchPlace, WatchCheck);
	RunOne(ss::autoplay::Mode::Skip, SkipPlace, SkipCheck);
	Expect(!WatchCheck.empty() && WatchCheck == SkipCheck && WatchPlace == SkipPlace && WatchPlace >= 1, "long: a FUSE watched and skipped ends on the same checkpoint, pool and place");
	Note("long: " + Num(SecondsSince(T0), 1) + " s");
}
} // namespace fuse_seg

void SegFuse()
{
	namespace fs = fuse_seg;
	const auto T0 = std::chrono::steady_clock::now();
	fs::Tables();
	fs::Pool();
	fs::Ids();
	fs::Draws();
	fs::Record();
	fs::Seats();
	fs::Copy();
	fs::Calibration();
	fs::Light();
	fs::Tickets();
	fs::Refusals();
	fs::Reload();
	fs::Table3();
	fs::Dev();
	fs::FirstDeal();
	fs::Note("default run " + fs::Num(fs::SecondsSince(T0), 1) + " s");
	if (fs::LongMode())
	{
		fs::Long();
	}
}
} // namespace siteplay_test
