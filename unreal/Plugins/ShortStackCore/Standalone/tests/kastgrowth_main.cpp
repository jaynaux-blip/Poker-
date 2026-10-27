// kast_growth (SPEC_CREATORS §9.1, suite 23): Kast's growth model and the Room. The pure functions (Unit's independence, the
// tables, Heat's closed form and split invariance, the night's gain and its caps, the forecast's arithmetic, tokens, the rings,
// the grant mask, the pending payout and the sponsor read rule, the save tails), then the Room and the careers on AdvanceTo's
// minute grid (kastgrowth_session.cpp) and the chat games (kastgrowth_play.cpp). Default run <= 60 s; "long" runs every
// archetype at 12 seeds and the levers (the gates' run). core-kastmath's file (main() and SegPure).
//
// SegPure: every function of KastGrowth.h at the table points of SPEC_CREATORS §2 (probes/synth/synth_kast.py's constants),
// the keyed draws' independence, the save lines' round trips and widths, and the copy (KastCopy.h): the accessors' text
// against §5.3's examples, the copy lint (tests/CopyLint.h), both trademark tiers (tests/Trademarks.h) and U1.
//
// SegGlue (after SegPlay): Session's Kast glue (SessionKast.cpp) on a real ss::Session, core-kastglue's checks moved here from
// night_one_session by the C-K gate's L14, and the gate's wave-2 glue fixes (L8's forecast preview, L10's why lines off air).
#include "KastGrowthSeg.h"

#include "CopyLint.h"
#include "Trademarks.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/KastCopy.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace session_kast_detail
{
// SessionKast.cpp (the Session glue's file-local helpers, read by SegGlue as night_one_session's econ checks read them).
double KastSeriesOf(const Tournament& Tn); // §2.6's Series of the event a table plays
void KastMigrate(kast::Channel& Ch, double World); // §8.3's seeding of an old channel at its first stream
int KastLastDayLive(const kast::Channel& Ch); // the last day a channel was live, from the days it kept and its log
bool KastHeatAtGoLive(const kast::Channel& Ch, int Update3LinesRead, double World, double& Heat, double& HeatAt, int& LastStreamDay); // L8's preview
std::vector<std::string> KastEndStings(uint32_t FirstsBefore, int RaidsInBefore, const kast::Channel& Ch, const kast::Summary& Sum); // K17's end-card stings
} // namespace session_kast_detail
} // namespace ss

namespace kastgrowth_test
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

namespace pure_seg
{
namespace g = ss::kast::growth;
namespace c = ss::kast::copy;

bool Near(double A, double B, double Tol)
{
	return std::fabs(A - B) <= Tol;
}

std::string Num(double V)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.6g", V);
	return Buf;
}

/** A check that a value sits at its table point: "<what>: <got> (want <want>)". */
void At(const char* What, double Got, double Want, double Tol = 1e-9)
{
	Expect(Near(Got, Want, Tol), std::string("kast growth: ") + What + ": " + Num(Got) + " (want " + Num(Want) + ")");
}

/** The Session's line split (session_detail::SplitTabs): empty fields kept. */
std::vector<std::string> Split(const std::string& Line)
{
	std::vector<std::string> Parts;
	std::string Cur;
	for (const char Ch : Line)
	{
		if (Ch == '\t')
		{
			Parts.push_back(Cur);
			Cur.clear();
		}
		else if (Ch != '\n')
		{
			Cur.push_back(Ch);
		}
	}
	Parts.push_back(Cur);
	return Parts;
}

// ------------------------------------------------------------------ keyed draws

void Draws()
{
	// Uniform in [0, 1), and the same arguments give the same draw.
	double Lo = 1.0, Hi = 0.0, Sum = 0.0;
	const int N = 20000;
	for (int I = 0; I < N; ++I)
	{
		const double U = g::Unit(12345, 7, g::Tag::Guest, static_cast<uint64_t>(I));
		Lo = std::min(Lo, U);
		Hi = std::max(Hi, U);
		Sum += U;
	}
	Expect(Lo >= 0.0 && Hi < 1.0 && Near(Sum / N, 0.5, 0.01), "kast growth: Unit is a uniform in [0, 1) (mean " + Num(Sum / N) + ")");
	Expect(g::Unit(1, 2, g::Tag::Slot, 3, 4) == g::Unit(1, 2, g::Tag::Slot, 3, 4), "kast growth: a keyed draw is a function of its key");
	Expect(g::Unit(1, 2, g::Tag::Slot, 3, 4) != g::Unit(1, 2, g::Tag::Slot, 3, 5) && g::Unit(1, 2, g::Tag::Slot, 3, 4) != g::Unit(1, 2, g::Tag::Ping, 3, 4) &&
			g::Unit(1, 2, g::Tag::Slot, 3, 4) != g::Unit(1, 3, g::Tag::Slot, 3, 4) && g::Unit(1, 2, g::Tag::Slot, 3, 4) != g::Unit(2, 2, g::Tag::Slot, 3, 4),
		"kast growth: every part of the key moves the draw (B, the tag, the stream, the seed)");

	// §9.1: 2,000 names x 10 slots. Consecutive-slot correlation within +-0.02, and P(leave next | left) within +-0.02 of
	// independence for a loyal regular's slot (Sm 119: leaves a slot with 1 - e^(-10/119) = 8.1%) and a new follower's (Sm 20).
	for (const double Sm : {119.0, 20.2})
	{
		const double Leave = 1.0 - std::exp(-10.0 / Sm);
		double Sx = 0, Sy = 0, Sxx = 0, Syy = 0, Sxy = 0;
		int Pairs = 0, Left = 0, LeftNext = 0;
		for (int Name = 0; Name < 2000; ++Name)
		{
			const uint64_t Key = g::NameKey("member_" + std::to_string(Name));
			double Prev = g::Unit(0x5eed, 12, g::Tag::Slot, Key, 0);
			for (uint64_t K = 1; K < 10; ++K)
			{
				const double U = g::Unit(0x5eed, 12, g::Tag::Slot, Key, K);
				Sx += Prev;
				Sy += U;
				Sxx += Prev * Prev;
				Syy += U * U;
				Sxy += Prev * U;
				++Pairs;
				if (Prev < Leave)
				{
					++Left;
					LeftNext += U < Leave ? 1 : 0;
				}
				Prev = U;
			}
		}
		const double Mx = Sx / Pairs, My = Sy / Pairs;
		const double Corr = (Sxy / Pairs - Mx * My) / std::sqrt((Sxx / Pairs - Mx * Mx) * (Syy / Pairs - My * My));
		const double Cond = Left > 0 ? static_cast<double>(LeftNext) / Left : 0.0;
		std::printf("kast growth: Unit over 2,000 names x 10 slots (Sm %.1f): consecutive-slot correlation %+.4f; P(leave next | left) %.4f against %.4f (%d leaves)\n", Sm,
			Corr, Cond, Leave, Left);
		Expect(std::fabs(Corr) <= 0.02, "kast growth: consecutive slots decorrelate (correlation " + Num(Corr) + ", want within 0.02)");
		Expect(std::fabs(Cond - Leave) <= 0.02,
			"kast growth: P(leave next | left) " + Num(Cond) + " is within 0.02 of independence (" + Num(Leave) + ", Sm " + Num(Sm) + ", " + std::to_string(Left) + " leaves)");
	}

	// The Ping's roll and the arrival delay of one member never share a draw.
	double Sxy = 0, Sx = 0, Sy = 0, Sxx = 0, Syy = 0;
	const double Names = 10000.0;
	for (int Name = 0; Name < 10000; ++Name)
	{
		const uint64_t Key = g::NameKey("x" + std::to_string(Name));
		const double A = g::Unit(99, 3, g::Tag::Ping, Key), B = g::Unit(99, 3, g::Tag::Arrive, Key);
		Sx += A;
		Sy += B;
		Sxx += A * A;
		Syy += B * B;
		Sxy += A * B;
	}
	const double Corr = (Sxy / Names - (Sx / Names) * (Sy / Names)) / std::sqrt((Sxx / Names - (Sx / Names) * (Sx / Names)) * (Syy / Names - (Sy / Names) * (Sy / Names)));
	Expect(std::fabs(Corr) <= 0.03, "kast growth: two tags on one name decorrelate (" + Num(Corr) + ")");

	// NameKey is Fnv1a64.
	Expect(g::NameKey("") == 14695981039346656037ull && g::NameKey("a") == 0xaf63dc4c8601ec8cull && g::NameKey("foobar") == 0x85944171f73967e8ull,
		"kast growth: NameKey is Fnv1a64");

	// The quantile helpers.
	At("ExpAt(3, 1 - 1/e)", g::ExpAt(3.0, 1.0 - std::exp(-1.0)), 3.0, 1e-12);
	At("ExpAt(5, 0)", g::ExpAt(5.0, 0.0), 0.0);
	Expect(std::isfinite(g::ExpAt(5.0, 0.9999999999999999)), "kast growth: ExpAt stays finite at the top of the unit");
	At("NormalAt(0.5)", g::NormalAt(0.5), 0.0, 1e-9);
	At("NormalAt(0.975)", g::NormalAt(0.975), 1.959963985, 1e-6);
	At("NormalAt(0.01)", g::NormalAt(0.01), -2.326347874, 1e-6);
	Expect(g::PoissonAt(0.0, 0.7) == 0 && g::PoissonAt(2.0, 0.0) == 0 && g::PoissonAt(2.0, 0.999) >= 6, "kast growth: PoissonAt's ends");
	for (const double Mean : {0.05, 0.4, 3.0, 20.0, 90.0})
	{
		double Total = 0;
		bool Monotone = true;
		int Last = 0;
		const int Steps = 20000;
		for (int I = 0; I < Steps; ++I)
		{
			const int K = g::PoissonAt(Mean, (I + 0.5) / Steps);
			Monotone = Monotone && K >= Last;
			Last = K;
			Total += K;
		}
		Expect(Monotone && Near(Total / Steps, Mean, 0.01 + 0.005 * Mean), "kast growth: PoissonAt's mean over the unit is its mean (" + Num(Total / Steps) + " for " + Num(Mean) + ")");
	}
}

// ------------------------------------------------------------------ the Ping and presence (§2.3)

void PingAndPresence()
{
	At("PingW 5 days", g::PingW(5, 0.1), 1.0);
	At("PingW 7 days", g::PingW(7, 0.1), 1.0);
	At("PingW 10 days", g::PingW(10, 0.1), 0.7);
	At("PingW 21 days", g::PingW(21, 0.1), 0.7);
	At("PingW 30 days", g::PingW(30, 0.1), 0.35);
	At("PingW 60 days", g::PingW(60, 0.1), 0.35);
	At("PingW 90 days", g::PingW(90, 0.1), 0.12);
	At("PingW a superfan 90 days", g::PingW(90, 0.7), 1.0);
	At("PingW a mod (L 1)", g::PingW(400, 1.0), 1.0);
	At("PingWHot(0.35, h 0.2)", g::PingWHot(0.35, 0.2), 0.415);
	At("PingWHot(0.12, h 1)", g::PingWHot(0.12, 1.0), 0.56);
	At("Honey(0)", g::Honey(0), 0.15);
	At("Honey(7)", g::Honey(7), 0.15 / std::exp(1.0));
	At("Appetite(0)", g::Appetite(0), 0.5);
	At("Appetite(2)", g::Appetite(2), 1.0 - 0.5 / std::exp(1.0));
	At("Budget(8)", g::Budget(8), 1.0);
	At("Budget(10)", g::Budget(10), std::exp(-0.2), 1e-12);
	At("Budget(20)", g::Budget(20), std::exp(-1.2), 1e-12);
	At("HwToday(10, 3, 10)", g::HwToday(10, 3, 10), 10.0 / std::exp(1.0));
	At("HwToday unset", g::HwToday(4, -1, 10), 4.0);
	g::PingTerms T;
	T.L = 0.8;
	At("Pa of a loyal regular, every term 1", g::Pa(T), 0.03 + 0.75 * std::pow(0.8, 1.15), 1e-12);
	T.Sched = 1.3;
	T.Recent = 1.15;
	T.WelcomeBack = 1.5;
	At("Pa's cap", g::Pa(T), 0.95);
	g::PingTerms New;
	New.L = 0.04;
	New.Honey = g::Honey(1);
	New.Appetite = g::Appetite(1);
	At("Pa of a new follower", g::Pa(New), (0.03 + 0.75 * std::pow(0.04, 1.15) + 0.15 * std::exp(-1.0 / 7.0)) * (1.0 - 0.5 * std::exp(-0.5)), 1e-12);
	At("FriendPa stream 1", g::FriendPa(1, 0.75, 0.4), 1.0);
	At("FriendPa stream 10", g::FriendPa(10, 0.75, 0.4), 0.9);
	At("FriendPa stream 11", g::FriendPa(11, 1.3, 1.0), 0.78, 1e-12);
	At("Dee on stream 1", g::FriendArrive(1, false, 0, 0.7), 0.25);
	At("Mei on stream 1, earliest", g::FriendArrive(1, true, 0.25, 0.0), 0.35, 1e-12);
	At("Mei on stream 1, latest", g::FriendArrive(1, true, 0.25, 1.0), 0.75, 1e-12);
	At("a friend on stream 3", g::FriendArrive(3, false, 0, 0.5), 1.15, 1e-12);
	At("a friend on stream 6 (not First Faces)", g::FriendArrive(6, false, 0, 0.5), -1.0);
	At("ArriveDelay on schedule", g::ArriveDelay(true, 1.0 - std::exp(-1.0)), 3.0, 1e-12);
	At("ArriveDelay off", g::ArriveDelay(false, 1.0 - std::exp(-1.0)), 6.0, 1e-12);
	At("StayMean a loyal regular", g::StayMean(0.8, false, 20, false), 119.0, 1e-12);
	At("StayMean a new follower", g::StayMean(0.04, false, 20, false), 20.2, 1e-12);
	At("StayMean a friend, stream 5", g::StayMean(0.85, true, 5, false), 400.0);
	At("StayMean a friend, stream 6", g::StayMean(0.85, true, 6, false), 95.0);
	At("StayMean a mod", g::StayMean(0.8, false, 20, true), 178.5, 1e-12);
	At("GapMean a loyal regular", g::GapMean(0.8, false, false, false, 0), 48.0, 1e-12);
	At("GapMean a new follower", g::GapMean(0.04, false, false, false, 0), 116.4, 1e-12);
	At("GapMean a friend", g::GapMean(0.85, true, false, false, 0), 40.0);
	At("GapMean a mod", g::GapMean(0.8, false, true, false, 0), 24.0, 1e-12);
	At("GapMean on the results screen", g::GapMean(0.8, false, false, true, 0), 72.0, 1e-12);
	At("GapMean at a final table (Pst2 1.4)", g::GapMean(0.8, false, false, false, 1.4), 48.0 / 6.6, 1e-12);
	At("Pst2 at a final table", g::Pst2(1.4, 1.0), 1.4);
	At("Pst2 floors P at 0.5", g::Pst2(0.3, 0.5), 0.125);
	At("Pst2 with no run", g::Pst2(2.0, 0.0), 0.0);
	// The slot step: here leaves with 1 - e^(-10/Sm), away returns with 1 - e^(-10/G).
	Expect(!g::SlotNext(true, 119, 48, 0.01) && g::SlotNext(true, 119, 48, 0.5) && g::SlotNext(false, 119, 48, 0.01) && !g::SlotNext(false, 119, 48, 0.9),
		"kast growth: SlotNext leaves on a low draw and returns on one");
	int Stays = 0, Backs = 0;
	for (int I = 0; I < 10000; ++I)
	{
		Stays += g::SlotNext(true, 119, 48, g::Unit(5, 1, g::Tag::Slot, 77, static_cast<uint64_t>(I))) ? 1 : 0;
		Backs += g::SlotNext(false, 119, 48, g::Unit(5, 1, g::Tag::Slot, 78, static_cast<uint64_t>(I))) ? 1 : 0;
	}
	At("the slot's stay share", Stays / 10000.0, std::exp(-10.0 / 119.0), 0.01);
	At("the slot's return share", Backs / 10000.0, 1.0 - std::exp(-10.0 / 48.0), 0.01);
	At("Presence at arrival", g::Presence(0, 119, 48), 1.0);
	At("Presence long after", g::Presence(1e6, 119, 48), 119.0 / 167.0, 1e-12);
	// A loyal regular has the stream on about 70% of the night (§2.3.4).
	At("a loyal regular's share of the night", g::Presence(1e6, g::StayMean(0.8, false, 20, false), g::GapMean(0.8, false, false, false, 0)), 0.70, 0.02);
}

// ------------------------------------------------------------------ the deepest run (§2.6)

void DeepRun()
{
	At("P, 300 runners at $3.30", g::Prestige(300, 330, 0), 0.37, 0.005);
	At("P, 800 runners at $3.30", g::Prestige(800, 330, 0), 0.87, 0.005);
	At("P, 980 runners at $11", g::Prestige(980, 1100, 0), 1.40, 0.005);
	At("P, 1,200 runners at $5.50", g::Prestige(1200, 550, 0), 1.25, 0.005);
	At("P, a Sunday major", g::Prestige(5000, 10900, 1.0), 4.0);
	At("P's floor", g::Prestige(45, 330, 0), 0.25);
	At("P with nothing known", g::Prestige(0, 0, 0), 0.25);
	At("depth at 9 entrants", g::Depth(9, 1), 0.0);
	At("depth at 5 entrants", g::Depth(5, 1), 0.0);
	At("depth at the start", g::Depth(100, 100), 0.0);
	At("depth at the final table", g::Depth(980, 9), 1.0, 1e-12);
	At("depth at the win", g::Depth(980, 1), 1.15);
	At("depth past the field", g::Depth(980, 2000), 0.0);
	At("depth at 150 of 1,000 (the bubble)", g::Depth(1000, 150), std::log(1000.0 / 150.0) / std::log(1000.0 / 9.0), 1e-12);
	Expect(!g::Armed(99) && g::Armed(100), "kast growth: the deep-run machinery arms at 100 entrants");
	Expect(g::EventMode(980, 18, 9) && !g::EventMode(980, 19, 9) && !g::EventMode(99, 5, 9) && !g::EventMode(980, 0, 9) && !g::EventMode(980, 5, 0),
		"kast growth: Event Mode from the last two tables of an armed run");
	At("Nov, the week's first final table", g::SweatNov(1), 1.0);
	At("Nov, none yet", g::SweatNov(0), 1.0);
	At("Nov, the second", g::SweatNov(2), 0.75);
	At("Nov, the fourth", g::SweatNov(4), 0.5);
	// §2.6's table: strangers at the final table, the week's first.
	At("Sweat, 300 runners", g::Sweat(g::Prestige(300, 330, 0), 1.0, 1.0), 2.5, 0.05);
	At("Sweat, 800 runners", g::Sweat(g::Prestige(800, 330, 0), 1.0, 1.0), 4.5, 0.05);
	At("Sweat, the Big Stack", g::Sweat(g::Prestige(980, 1100, 0), 1.0, 1.0), 6.6, 0.05);
	At("Sweat, 1,200 at $5.50", g::Sweat(g::Prestige(1200, 550, 0), 1.0, 1.0), 6.0, 0.05);
	At("Sweat, a Sunday major", g::Sweat(4.0, 1.0, 1.0), 17.0);
	At("Sweat with no run", g::Sweat(4.0, 0.0, 1.0), 1.0);
	// §2.6: after the week's first final-table night the stranger term reads Nov (the strong grinder's brake).
	At("Sweat with Nov, the Big Stack's fourth final table this week", g::Sweat(1.4, 1.0, g::SweatNov(4)), 1.0 + 4.0 * 1.4 * 0.5, 1e-12);
	At("Sweat with Nov at the bubble", g::Sweat(0.87, 0.5, g::SweatNov(2)), 1.0 + 4.0 * 0.87 * 0.25 * 0.75, 1e-12);
	At("Railbirds at a final table", g::Railbirds(50, 1.4, 1.0, 1.0), 56.0, 1e-12);
	At("Railbirds at depth 0.3", g::Railbirds(50, 1.4, 0.3, 1.0), 0.0);
	At("Railbirds at depth 0.5", g::Railbirds(0, 1.0, 0.5, 0.5), 30.0 * 0.125 * 0.5, 1e-12);
	At("LurkDeep at a final table", g::LurkDeep(1.4, 1.0), 3.8, 1e-12);
	At("LurkDeep caps P at 2", g::LurkDeep(4.0, 1.0), 5.0, 1e-12);
	At("DeepFollow", g::DeepFollow(1.4, 1.0), 1.42, 1e-12);
	At("DeepFollow with no run", g::DeepFollow(1.4, 0.0), 1.0);
	Expect(g::BroadcastSize(1.25) == 28 && g::BroadcastSize(2.5) == 79 && g::BroadcastSize(4.0) == 79 && g::BroadcastSize(1.40) == 33 && g::BroadcastSize(0.99) == 0,
		"kast growth: the Broadcast's size, 20 min(P, 2.5)^1.5 from P 1 (28 at 1.25, 79 at 2.5)");
	using RS = g::RunStage;
	At("RunValue bubble", g::RunValue(RS::Bubble), 0.15);
	At("RunValue ITM", g::RunValue(RS::Itm), 0.3);
	At("RunValue final table", g::RunValue(RS::FinalTable), 1.0);
	At("RunValue top 3", g::RunValue(RS::Top3), 1.3);
	At("RunValue win", g::RunValue(RS::Win), 2.0);
	At("RunValue none", g::RunValue(RS::None), 0.0);
	At("Pull bubble", g::Pull(RS::Bubble), 0.25);
	At("Pull ITM", g::Pull(RS::Itm), 0.20);
	At("Pull final table", g::Pull(RS::FinalTable), 0.50);
	At("Pull top 3", g::Pull(RS::Top3), 0.40);
	At("Pull win", g::Pull(RS::Win), 0.0);
	At("AlertBack", g::AlertBack(0.5, 1.0), 0.5);
	At("AlertFollower", g::AlertFollower(0.5, 0.12, 0.5, 4.0), 0.5 * 0.35 * 0.35, 1e-12);
	Expect(!g::RailCallOpen(1000, 0.39) && g::RailCallOpen(1000, 0.4) && !g::RailCallOpen(99, 0.9), "kast growth: RAIL CALL opens at depth 0.4 in an armed run");
	At("RailCallChance", g::RailCallChance(1.0, 0.8, 1.0, 1.0, 1.0, 1.0), 0.5 * (0.03 + 0.75 * std::pow(0.8, 1.15)), 1e-12);
	At("RailCallChance for a lapsed follower", g::RailCallChance(0.12, 0.1, 1.0, 1.0, 1.0, 1.0), 0.5 * 0.35 * (0.03 + 0.75 * std::pow(0.1, 1.15)), 1e-12);
	// v1.1 (Econ m5): the roll reads Appetite and the attention budget, so it is no second Ping for a heavy streamer.
	At("RailCallChance reads Appetite and the budget", g::RailCallChance(1.0, 0.8, 1.3, 0.8, g::Appetite(0), g::Budget(20)),
		0.5 * (0.03 + 0.75 * std::pow(0.8, 1.15)) * 1.3 * 0.8 * 0.5 * std::exp(-1.2), 1e-12);
}

// ------------------------------------------------------------------ Heat (§2.4)

void Heat()
{
	const double Day = ss::net::MinutesPerDay;
	At("Heat now", g::HeatAt(26, 0, 0), 26.0);
	At("Heat after one half-life (5 world days)", g::HeatAt(26, 0, 5 * Day), 17.0, 1e-12);
	At("Heat after two", g::HeatAt(100, 1000, 1000 + 10 * Day), 8.0 + 92.0 / 4.0, 1e-12);
	At("Heat a long time dark", g::HeatAt(100, 0, 3650 * Day), 8.0, 1e-9);
	At("Heat's bounds", g::HeatAt(150, 0, 0), 100.0);
	// Closed form: thirty one-day steps (re-based each day) against one thirty-day step.
	double Stepped = 64.0, StepAt = 0.0;
	for (int D = 1; D <= 30; ++D)
	{
		Stepped = g::HeatAt(Stepped, StepAt, D * Day);
		StepAt = D * Day;
	}
	At("Heat: 30 one-day steps against one 30-day step", Stepped, g::HeatAt(64.0, 0.0, 30 * Day), 1e-10);
	Expect(g::HeatAt(64.0, 0.0, 30 * Day) == g::HeatAt(64.0, 0.0, 30 * Day), "kast growth: Heat is a pure function of (Heat, HeatAt, now)");
	// Shown: >= 20 while the channel streamed in the last 7 world days; a channel that never streamed reads its start, 26.
	At("HeatShown, streamed 3 days ago", g::HeatShown(10, 0, 10 * Day, 7), 20.0);
	At("HeatShown, streamed 7 days ago", g::HeatShown(10, 0, 14 * Day, 7), 20.0);
	At("HeatShown, 8 days dark", g::HeatShown(10, 0, 15 * Day + 1.0, 7), 10.0 - 2.0 * (1.0 - std::pow(0.5, (15 * Day + 1.0) / (5 * Day))), 1e-9);
	At("HeatShown, hot and streamed", g::HeatShown(50, 0, 1 * Day, 0), g::HeatAt(50, 0, 1 * Day));
	At("HeatShown, a new channel", g::HeatShown(26, 0, 0, -1), 26.0);
	At("HeatShown, a new channel 30 days in", g::HeatShown(26, 0, 30 * Day, -1), 26.0);
	for (int Dark = 0; Dark <= 7; ++Dark)
	{
		for (const double Minute : {0.0, 600.0, 1439.5})
		{
			Expect(g::Tier(g::HeatShown(8.0, 0.0, (20 + Dark) * Day + Minute, 20)) >= 1,
				"kast growth: never Cold while streaming weekly (" + std::to_string(Dark) + " days on, minute " + Num(Minute) + ")");
		}
	}
	Expect(g::Tier(g::HeatShown(8.0, 0.0, 28 * Day, 20)) == 0 && g::Tier(g::HeatShown(19.0, 27 * Day, 28 * Day + 600.0, 20)) == 0,
		"kast growth: Cold after 7 dark world days");
	At("HeatH at 26", g::HeatH(26), 18.0 / 92.0, 1e-12);
	At("HeatS at h 0.2", g::HeatS(0.2), 1.0, 1e-12);
	At("HeatA at h 0.2", g::HeatA(0.2), 1.0, 1e-12);
	At("HeatS at the floor of 20", g::HeatS(g::HeatH(20)), 0.9, 0.005);
	At("HeatS at Heat 50", g::HeatS(g::HeatH(50)), 1.385, 0.005);
	At("HeatS at Heat 75", g::HeatS(g::HeatH(75)), 1.79, 0.005);
	At("HeatS's floor", g::HeatS(0.0), 0.8);
	At("HeatA's floor", g::HeatA(0.0), 0.8);
	// The night's gain.
	g::NightGain G = g::Gain(2.0, 0.0, 0.0, 0.0, 0.0, 30.0);
	At("gain: perf clamps at 1.2", G.Perf, 1.2);
	At("gain: 26 perf clamps at 25", G.Raw, 25.0);
	G = g::Gain(-1.0, 0.0, 0.0, 0.0, 0.0, 30.0);
	At("gain: perf clamps at -0.4", G.Perf, -0.4);
	At("gain: a bad night's floor", G.Gain, -9.0);
	G = g::Gain(0.1, 0.0, 1.4, 0.15, 0.0, 30.0);
	At("gain: news", G.News, 1.25, 1e-12);
	At("gain: the news term caps at 25 and sits on top", G.Raw, 2.6 + 25.0, 1e-12);
	At("gain: inside the week and below the ceiling", G.Gain, 27.6, 1e-12);
	G = g::Gain(0.1, 0.0, 1.4, 0.15, 20.0, 30.0);
	At("gain: the week's +35 cap", G.Gain, 15.0, 1e-12);
	G = g::Gain(0.1, 0.0, 1.4, 0.15, 35.0, 30.0);
	At("gain: a hot week can't climb", G.Gain, 0.0);
	G = g::Gain(0.1, 0.0, 1.4, 0.15, 0.0, 70.0);
	At("gain: the soft ceiling near the top", G.Gain, 27.6 * 0.4, 1e-12);
	G = g::Gain(0.1, 0.0, 1.4, 0.15, 0.0, 95.0);
	At("gain: none past 90", G.Gain, 0.0);
	// §2.4's order: the week's cap first (<= 35 - GainUsed), then the soft ceiling on what the cap leaves.
	G = g::Gain(0.1, 0.0, 1.4, 0.15, 20.0, 70.0);
	At("gain: the week's cap, then the soft ceiling", G.Gain, 15.0 * 0.4, 1e-12);
	G = g::Gain(-0.5, 0.0, 0.0, 0.0, 35.0, 95.0);
	At("gain: a loss ignores the caps", G.Gain, -9.0);
	G = g::Gain(0.3, 0.2, 0.0, 0.0, 0.0, 30.0);
	At("gain: perf reads against Bias", G.Perf, 0.1, 1e-12);
	G = g::Gain(0.0, 0.0, 0.1, 0.15, 0.0, 30.0);
	At("gain: a usual run is no news", G.News, 0.0);
	// Showing up: +3 a streaming day, at most 4 days a world week (day 0 is a Monday).
	At("ShowUp in a new week", g::ShowUp(-1, 0, 10), 3.0);
	At("ShowUp the same day twice", g::ShowUp(1, 1 << 3, 10), 0.0);
	At("ShowUp a fourth day", g::ShowUp(1, 0x7, 10), 3.0);
	At("ShowUp a fifth day", g::ShowUp(1, 0x17, 12), 0.0);
	At("ShowUp the next week", g::ShowUp(1, 0x1f, 14), 3.0);
	Expect(g::Tier(19.99) == 0 && g::Tier(20) == 1 && g::Tier(34.99) == 1 && g::Tier(35) == 2 && g::Tier(54.99) == 2 && g::Tier(55) == 3 && g::Tier(74.99) == 3 &&
			g::Tier(75) == 4 && g::Tier(100) == 4,
		"kast growth: the tier cuts 20 / 35 / 55 / 75");
	Expect(g::FrontRow(55, 0.29) && !g::FrontRow(54.9, 0.0) && !g::FrontRow(80, 0.3), "kast growth: Front Row at On a heater, 30% of streams");
}

// ------------------------------------------------------------------ discovery, stay, follow (§2.5), the community (§2.7-2.8)

void Discovery()
{
	At("Disc new", g::Disc(1, 0), 1.0);
	At("Disc level 11, streak 8", g::Disc(11, 8), 1.3 * 1.32, 1e-12);
	At("Disc's streak caps at 8", g::Disc(1, 20), 1.32, 1e-12);
	At("Lane stream 1", g::Lane(1), 1.8);
	At("Lane stream 7", g::Lane(7), 1.4, 1e-12);
	At("Lane stream 13", g::Lane(13), 1.0);
	At("Lane stream 40", g::Lane(40), 1.0);
	At("Rising before stream 4", g::Rising(3, 2), 1.0);
	At("Rising a 2-viewer channel", g::Rising(4, 2), 1.0 + 13.0 / 15.0, 1e-12);
	At("Rising at 15", g::Rising(20, 15), 1.0);
	At("Pond 600", g::Pond(600), std::sqrt(0.5), 1e-12);
	At("Pond 0", g::Pond(0), 1.0);
	At("Novelty 20 hours", g::Novelty(20), 1.0);
	At("Novelty 42 hours", g::Novelty(42), 0.649, 0.005);
	At("Taper inside 4 hours", g::Taper(240, 0), 1.0);
	At("Taper at 9 hours", g::Taper(540, 0), std::exp(-1.0), 1e-12);
	At("Taper's floor", g::Taper(900, 0), 0.25);
	At("Taper never touches a deep run", g::Taper(900, 0.31), 1.0);
	At("Taper at depth 0.3 tapers", g::Taper(900, 0.3), 0.25);
	At("Tide's median", g::Tide(0.5, false), 1.0, 1e-9);
	At("Tide in a series window", g::Tide(0.5, true), 1.3, 1e-9);
	At("Tide at +1 sd", g::Tide(0.841344746, false), std::exp(0.12), 1e-6);
	At("Buzz none", g::Buzz(0, 30), 1.0);
	At("Buzz full", g::Buzz(2000, 0), 1.35, 1e-12);
	At("Buzz reads the channel's size", g::Buzz(2000, 50), 1.0 + 0.35 * 0.5, 1e-12);
	At("HD 0.30", g::HD(0.30), 1.0, 0.005);
	At("HD 0.20", g::HD(0.20), 0.82, 0.005);
	At("HD 0.12", g::HD(0.12), 0.67, 0.005);
	At("HD 0.05", g::HD(0.05), 0.54, 0.005);
	At("HD 0.50", g::HD(0.50), 1.0);
	At("Sat 0", g::Sat(0), 1.0);
	At("Sat 3,000", g::Sat(3000), std::pow(2.0, -0.65), 1e-12);
	At("Stamina 25 hours", g::Stamina(25), 1.0);
	At("Stamina 38 hours", g::Stamina(38), 0.61, 1e-12);
	At("Stamina, a daily 6-hour streamer", g::Stamina(42), 0.6);
	At("RaidRate no tokens", g::RaidRate(0, 1.0, 0.0), 0.02);
	At("RaidRate 3 tokens", g::RaidRate(3, 1.0, 0.0), 0.02 + 0.03 * 1.05, 1e-12);
	At("RaidRate caps Disc at 2 and reads h", g::RaidRate(100, 5.0, 1.0), (0.02 + 0.3) * 2.0 * 2.0, 1e-12);
	At("RaidSize low", g::RaidSize(0, 0), 1.0);
	At("RaidSize high", g::RaidSize(1, 1), 0.9 * 57.0, 1e-12);
	Expect(g::EarnsToken(10, 3, 60) && !g::EarnsToken(10, 4, 60) && !g::EarnsToken(10, -1, 59.9) && g::EarnsToken(10, -1, 60),
		"kast growth: a raid out earns a token after 60 world minutes and once a 7 days a channel");
	At("MomentRaidChance ITM", g::MomentRaidChance(g::RunStage::Itm), 0.05);
	At("MomentRaidChance final table", g::MomentRaidChance(g::RunStage::FinalTable), 0.15);
	At("MomentRaidChance win", g::MomentRaidChance(g::RunStage::Win), 0.30);
	At("MomentRaidChance bubble", g::MomentRaidChance(g::RunStage::Bubble), 0.0);
	At("BigNameChance", g::BigNameChance(0.2, 1.0), 0.16, 1e-12);
	At("BigNameChance at P 2", g::BigNameChance(0.2, 2.0), 0.31, 1e-12);
	At("RaiderStay, a stayer", g::RaiderStay(0.1, 1.0 - std::exp(-1.0), false), 20.0, 1e-12);
	At("RaiderStay, a short stay", g::RaiderStay(0.5, 1.0 - std::exp(-1.0), false), 3.0, 1e-12);
	At("RaiderStay, welcomed", g::RaiderStay(0.1, 1.0 - std::exp(-1.0), true), 26.0, 1e-12);
	At("ClipReach", g::ClipReach(10, 0, 0, 0), 70.0);
	At("ClipReach at h 1 and a 50 hit", g::ClipReach(10, 0, 1.0, 50), 280.0, 1e-12);
	At("AffinityBump, Bonded", g::AffinityBump(0.5, 0.05), 0.525, 1e-12);
	At("AffinityBump at 1", g::AffinityBump(1.0, 0.05), 1.0);
	At("LurkerMean, 1,000 untracked", g::LurkerMean(1000, 1.0, 1.25, 0, 1.0), 6.0, 1e-12);
	At("LurkerMean with the away followers", g::LurkerMean(1000, 1.0, 1.25, 100, 1.0), 6.3, 1e-12);
	At("LurkerMean, the sponsored streamer (2,402 untracked, no schedule)", g::LurkerMean(2402, 0.75, 1.25, 0, 1.0), 10.8, 0.05);
	At("LurkerMean while deep", g::LurkerMean(1000, 1.0, 1.25, 0, 3.8), 22.8, 1e-12);
	Expect(g::Lurkers(2.3, 0.6) == 2 && g::Lurkers(2.3, 0.75) == 3 && g::Lurkers(0.0, 0.99) == 0, "kast growth: Lurkers is floor(mean + u)");
	double LurkSum = 0;
	for (int I = 0; I < 1000; ++I)
	{
		LurkSum += g::Lurkers(3.37, (I + 0.5) / 1000.0);
	}
	At("Lurkers' mean over the unit", LurkSum / 1000.0, 3.37, 0.002);
}

// ------------------------------------------------------------------ engagement and money (§2.9, §2.10)

void Money()
{
	At("EngageFade", g::EngageFade(0.3, 6.9), 0.3 / std::exp(1.0), 1e-12);
	At("ThankGain first", g::ThankGain(0), 0.25);
	At("ThankGain third", g::ThankGain(2), 0.09, 1e-12);
	At("QuestionRate", g::QuestionRate(10, 0, 0, false), 0.197, 1e-12);
	At("QuestionRate reads the room", g::QuestionRate(5, 10, 4, false), 0.0197 * 11.0, 1e-12);
	At("QuestionRate's cap", g::QuestionRate(100, 0, 0, false), 0.39);
	At("QuestionRate teaching", g::QuestionRate(100, 0, 0, true), 0.624, 1e-12);
	At("EngT at 0.30", g::EngT(0.30), 1.0, 1e-12);
	At("EngT at the passive floor", g::EngT(0.20), 0.77, 0.005);
	At("EngT heads-down", g::EngT(0.05), 0.42, 0.005);
	At("EngT past the cap", g::EngT(0.9), 1.0, 1e-12);
	At("EngS at 0.30", g::EngS(0.30), 1.0, 1e-12);
	At("Dil, nobody gave", g::Dil(0, 1e9), 1.0);
	At("Dil, 90 present, 21 days since", g::Dil(90, 21), std::sqrt(0.5) * (1.0 - std::exp(-1.0)), 1e-12);
	At("Dil, gave today", g::Dil(0, 0), 0.0);
	At("TipChance a superfan", g::TipChance(1.0, 0, 0.3, 1.0), 0.0184, 1e-12);
	At("TipChance at hype 50", g::TipChance(0.25, 50, 0.3, 1.0), 0.0184 * 0.125 * 1.5, 1e-12);
	At("CheerChance", g::CheerChance(0.5, 0, 0.3, 1.0), 0.010, 1e-12);
	At("SubChance at L 0", g::SubChance(0, 0, 0.3), 0.0025, 1e-12);
	At("SubChance a superfan at hype 150", g::SubChance(1, 150, 0.3), 0.0375 * 2.0, 1e-12);
	At("RenewChance", g::RenewChance(1.0, false), 0.85, 1e-12);
	At("RenewChance at L 0", g::RenewChance(0.0, false), 0.40);
	At("RenewChance a gift", g::RenewChance(1.0, true), 0.70, 1e-12);
	// Per-visit giving: the visit's chance spread over its expected minutes, bunched after big moments. A visit as long as
	// expected with 15% of its minutes after big moments integrates to its chance (x1.0125), and 37% of it lands there.
	const double P = g::TipChance(0.8, 30, 0.3, 1.0);
	double Total = 0, NearTotal = 0;
	for (int M = 0; M < 100; ++M)
	{
		const bool Near = M % 20 < 3;
		const double H = g::GiveHazard(0.8, 30, 0.3, 1.0, 100.0, Near);
		Total += H;
		NearTotal += Near ? H : 0.0;
	}
	At("GiveHazard integrates to the visit's chance", Total / P, 1.0125, 1e-9);
	At("GiveHazard bunches after big moments", NearTotal / Total, 0.375 / 1.0125, 1e-9);
	// C14: the payout and what waits.
	g::Payout Out = g::PayoutOf(500, 1000, 1);
	Expect(Out.Pay == 500 && Out.Pending == 1000, "kast growth: before the second rent the balance pays and the pending waits");
	Out = g::PayoutOf(500, 1000, 2);
	Expect(Out.Pay == 1500 && Out.Pending == 0, "kast growth: the first payout after the second rent brings the pending");
	Out = g::PayoutOf(0, 0, 3);
	Expect(Out.Pay == 0 && Out.Pending == 0, "kast growth: nothing pays nothing");
	// C13: a read pays once a deal a world day, after 30 world minutes live, with someone watching, within 1.5x the ask.
	const double Day = ss::net::MinutesPerDay;
	Expect(g::SponsorReadPays(10, 9 * Day + 600, 30, 1, 0, 8), "kast growth: a read pays after 30 live minutes with someone watching");
	Expect(!g::SponsorReadPays(10, 9 * Day + 600, 29.9, 1, 0, 8), "kast growth: no read pays inside 30 live minutes");
	Expect(!g::SponsorReadPays(10, 9 * Day + 600, 45, 0, 0, 8), "kast growth: no read pays to an empty room");
	Expect(!g::SponsorReadPays(10, 10 * Day + 5, 45, 3, 0, 8), "kast growth: one paid read a deal a world day");
	Expect(g::SponsorReadPays(10, -1e9, 45, 3, 0, 8), "kast growth: a deal never read pays its first");
	Expect(g::SponsorReadPays(10, 9 * Day, 45, 3, 11, 8) && !g::SponsorReadPays(10, 9 * Day, 45, 3, 12, 8), "kast growth: at most 1.5x the ask (12 of 8)");
	Expect(g::SponsorReadPays(10, 9 * Day, 45, 3, 6, 5) && !g::SponsorReadPays(10, 9 * Day, 45, 3, 7, 5), "kast growth: at most 1.5x the ask, rounded down (7 of 5)");
	// M1 (the C-K gate, the lead's decision): a deal that asks for no reads (Kast's old terms, a Team RiverLine deal migrated from
	// kast\tdeal) keeps one paid read a world day: 30 live minutes, someone watching, once a world day, and no 1.5x cap.
	Expect(g::SponsorReadPays(10, 9 * Day, 45, 3, 0, 0), "kast growth: a deal that asks for no reads (ReadsAsk 0) still pays a read a world day");
	Expect(g::SponsorReadPays(10, -1e9, 30, 1, 0, 0) && g::SponsorReadPays(10, 9 * Day, 45, 3, 40, 0) && g::SponsorReadPays(10, 9 * Day, 45, 3, 1000, -1),
		"kast growth: a deal that asks for no reads has no 1.5x cap (40 or 1,000 paid reads still pay the next day's)");
	Expect(!g::SponsorReadPays(10, 9 * Day + 600, 29.9, 1, 0, 0) && !g::SponsorReadPays(10, 9 * Day + 600, 45, 0, 0, 0) && !g::SponsorReadPays(10, 10 * Day + 5, 45, 3, 0, 0),
		"kast growth: a deal that asks for no reads still waits 30 live minutes, needs someone watching, and pays once a world day");
}

// ------------------------------------------------------------------ the rings, the grants, the forecast, the rank

void Rings()
{
	std::array<int, 7> Ring{};
	int RingDay = -1;
	At("H7 of an empty ring", g::H7(Ring, RingDay, 10), 0.0);
	g::LiveAdd(Ring, RingDay, 10, 120);
	g::LiveAdd(Ring, RingDay, 11, 60);
	g::LiveAdd(Ring, RingDay, 11, 30);
	At("H7 the next day", g::H7(Ring, RingDay, 11), 3.5);
	At("H7 six days on", g::H7(Ring, RingDay, 16), 3.5);
	At("H7 a week on", g::H7(Ring, RingDay, 17), 1.5);
	At("H7 eight days on", g::H7(Ring, RingDay, 18), 0.0);
	// Aging is closed form: a day-by-day walk (30 one-day adds of nothing) and one read at the end give the same ring reads.
	std::array<int, 7> Daily = Ring;
	int DailyDay = RingDay;
	bool Same = true;
	for (int D = 12; D <= 41; ++D)
	{
		g::LiveAdd(Daily, DailyDay, D, 0);
		Same = Same && g::H7(Daily, DailyDay, D) == g::H7(Ring, RingDay, D);
	}
	Expect(Same, "kast growth: the LiveMin ring ages the same in one step as in thirty");
	g::LiveAdd(Ring, RingDay, 30, 45);
	Expect(RingDay == 30 && g::H7(Ring, RingDay, 30) == 0.75, "kast growth: a stream after a week dark starts the ring over");
	g::LiveAdd(Ring, RingDay, 28, 15);
	At("H7 counts a late entry inside the week", g::H7(Ring, RingDay, 30), 1.0);
	// A gap of a few days inside the week clears the days it skipped: their slots held the week before's minutes.
	std::array<int, 7> Gap{};
	int GapDay = -1;
	g::LiveAdd(Gap, GapDay, 4, 100);
	g::LiveAdd(Gap, GapDay, 5, 100);
	g::LiveAdd(Gap, GapDay, 10, 120);
	g::LiveAdd(Gap, GapDay, 13, 60);
	At("H7 after a three-day gap counts only the week (days 7-13)", g::H7(Gap, GapDay, 13), 3.0);
	At("H7 the day after", g::H7(Gap, GapDay, 14), 3.0);
	// The grants: once a world day, a restart can't re-arm them.
	int GrantDay = -1;
	uint32_t Mask = 0;
	Expect(!g::Granted(GrantDay, Mask, 5, g::GrantRailCall), "kast growth: no grant used yet");
	g::UseGrant(GrantDay, Mask, 5, g::GrantRailCall);
	Expect(g::Granted(GrantDay, Mask, 5, g::GrantRailCall) && !g::Granted(GrantDay, Mask, 5, g::GrantBroadcast), "kast growth: a grant used today");
	g::UseGrant(GrantDay, Mask, 5, g::GrantAlertFinal);
	Expect(g::Granted(GrantDay, Mask, 5, g::GrantRailCall) && g::Granted(GrantDay, Mask, 5, g::GrantAlertFinal), "kast growth: grants add up through a day");
	Expect(!g::Granted(GrantDay, Mask, 6, g::GrantRailCall), "kast growth: a new world day re-arms them");
	g::UseGrant(GrantDay, Mask, 6, g::GrantClip1);
	Expect(GrantDay == 6 && Mask == g::GrantClip1, "kast growth: a mask from another day starts over");
}

void ForecastAndRank()
{
	// §2.11 by hand: two members (Pa 0.5, s 1 and 0) and a directory rate of 6 an hour at Q 0.29.
	const g::Band B = g::Forecast({0.5, 0.5}, {1.0, 0.0}, 6.0, 0.29, 0.0);
	const double Here = 0.5 + 0.125, Var = 0.25 + 0.125 * 0.875, Str = 0.1 * (4.0 + 6.0 * 0.29);
	const double Fc = Here + Str, Sd = std::sqrt(Var + Str);
	At("the forecast's centre", B.Centre, Fc, 1e-12);
	At("the forecast's low", B.Lo, (Fc - 0.85 * Sd) * std::exp(-0.10), 1e-12);
	At("the forecast's high", B.Hi, (Fc + 0.85 * Sd) * std::exp(0.10), 1e-12);
	const g::Band Biased = g::Forecast({0.5, 0.5}, {1.0, 0.0}, 6.0, 0.29, std::log(2.0));
	At("Bias moves the band, not the centre", Biased.Centre, Fc, 1e-12);
	At("Bias centres the band where forecasts land", Biased.Hi, (2.0 * Fc + 0.85 * Sd) * std::exp(0.10), 1e-12);
	const g::Band Empty = g::Forecast({}, {}, 0.0, 0.29, 0.0);
	Expect(Empty.Lo == 0.0 && Empty.Centre == 0.0 && Empty.Hi == 0.0, "kast growth: an empty forecast is 0-0");
	// A regular's night: 30 members and the directory, the band about where §2.11 puts it.
	std::vector<double> Pa, S;
	for (int I = 0; I < 30; ++I)
	{
		Pa.push_back(0.3 + 0.02 * I);
		S.push_back(0.6);
	}
	const g::Band Night = g::Forecast(Pa, S, 12.0, 0.29, 0.0);
	Expect(Night.Lo < Night.Centre && Night.Centre < Night.Hi && Night.Lo > 0.0, "kast growth: the band brackets its centre");
	// The rank: 1 + the live channels with more viewers; "of" counts them and you (at most 49).
	const double World = 3 * ss::net::MinutesPerDay + 21 * 60.0;
	const g::Standing Top = g::RankAt(World, 1000000);
	const g::Standing Bottom = g::RankAt(World, 0);
	Expect(Top.Rank == 1 && Top.Of >= 2 && Top.Of <= 49, "kast growth: the biggest stream is #1 (of " + std::to_string(Top.Of) + ")");
	Expect(Bottom.Rank == Bottom.Of && Bottom.Of == Top.Of, "kast growth: an empty stream is last (#" + std::to_string(Bottom.Rank) + " of " + std::to_string(Bottom.Of) + ")");
	int Live = 1;
	for (const ss::kast::Streamer& D : ss::kast::Directory())
	{
		Live += ss::kast::ViewersNow(D, World) > 0 ? 1 : 0;
	}
	Live += static_cast<int>(ss::kast::Network(World).size());
	Expect(Top.Of == Live, "kast growth: the rank counts the directory's named streamers and the network live now");
}

// ------------------------------------------------------------------ saves (§8.1)

g::SaveView Filled()
{
	g::SaveView V;
	V.Heat = 41.25;
	V.HeatAt = 31234.5;
	V.Norm = 12.75;
	V.Bias = -0.1234;
	V.RunNorm = 0.15;
	V.BestAvg = 19;
	V.PingAt = 31200.25;
	V.LastStreamDay = 21;
	V.ShowWeek = 3;
	V.ShowDays = 0x13;
	V.GainWeek = 3;
	V.GainUsed = 17.5;
	V.FtBase = 18;
	V.FtDays = 5;
	V.LiveDay = 21;
	V.LiveMin = {180, 0, 61, 0, 240, 0, 33};
	V.GrantDay = 21;
	V.GrantMask = g::GrantRailCall | g::GrantAlertFinal;
	V.WelcomeRaidDone = true;
	V.CotnDay = 20;
	V.PendingCents = 8600;
	V.LiveSince = -1;
	V.GainSoFar = 4.5;
	V.CarryFollow = 0.4321;
	V.CarryWord = 0.1;
	V.CarrySub = 0.25;
	V.CarryTip = 0.5;
	V.CarryCheer = 0.75;
	V.CarryRaid = 0.125;
	V.CarryGift = 0.0625;
	V.RerunHeat = {{30000.5, 6.0}, {31000.25, 10.0}};
	V.Firsts = g::FirstStrangerTip;
	return V;
}

bool SameView(const g::SaveView& A, const g::SaveView& B)
{
	return A.Version == B.Version && A.Heat == B.Heat && A.HeatAt == B.HeatAt && A.Norm == B.Norm && A.Bias == B.Bias && A.RunNorm == B.RunNorm && A.BestAvg == B.BestAvg &&
		A.PingAt == B.PingAt && A.LastStreamDay == B.LastStreamDay && A.ShowWeek == B.ShowWeek && A.ShowDays == B.ShowDays && A.GainWeek == B.GainWeek &&
		A.GainUsed == B.GainUsed && A.FtBase == B.FtBase && A.FtDays == B.FtDays && A.LiveDay == B.LiveDay && A.LiveMin == B.LiveMin && A.GrantDay == B.GrantDay &&
		A.GrantMask == B.GrantMask && A.WelcomeRaidDone == B.WelcomeRaidDone && A.CotnDay == B.CotnDay && A.PendingCents == B.PendingCents && A.LiveSince == B.LiveSince &&
		A.GainSoFar == B.GainSoFar && A.CarryFollow == B.CarryFollow && A.CarryWord == B.CarryWord && A.CarrySub == B.CarrySub && A.CarryTip == B.CarryTip &&
		A.CarryCheer == B.CarryCheer && A.CarryRaid == B.CarryRaid && A.CarryGift == B.CarryGift && A.RerunHeat == B.RerunHeat && A.Firsts == B.Firsts;
}

std::string MoText(const g::SaveView& V)
{
	std::ostringstream Out;
	g::WriteLines(Out, V);
	return Out.str();
}

/** A kast\tmem line as the Session writes it (Session.cpp's save block), then the tail. */
std::string MemLine(const ss::kast::Member& M, int Today)
{
	return "kast\tmem\t" + M.Name + "\t" + ss::Fixed(M.Loyalty, 4) + "\t" + ss::Fixed(M.Affinity, 4) + "\t" + ss::Fixed(M.FirstSeen, 2) + "\t" + ss::Fixed(M.LastSeen, 2) + "\t" +
		std::to_string(M.Streams) + "\t" + ss::Fixed(M.WatchMinutes, 1) + "\t" + std::to_string(M.Messages) + "\t" + std::to_string((M.Follower ? 1 : 0) | (M.Friend ? 2 : 0)) +
		"\t" + std::to_string(M.Given) + g::MemberTail(M, Today);
}

void Saves()
{
	// kast\tmo: nothing for a view that holds nothing; a filled one round-trips and writes back byte for byte, Firsts last.
	Expect(MoText(g::SaveView()).empty(), "kast growth: kast\\tmo writes nothing while it holds nothing");
	const g::SaveView V = Filled();
	const std::string Line = MoText(V);
	std::vector<std::string> P = Split(Line);
	Expect(Line.rfind("kast\tmo\t1\t", 0) == 0 && Line.back() == '\n' && P.size() == 41 && P.back() == "1",
		"kast growth: kast\\tmo is one line of 41 fields, version 1, Firsts last (" + std::to_string(P.size()) + ")");
	g::SaveView Back;
	Expect(g::ReadLine(P, Back) && SameView(Back, V), "kast growth: kast\\tmo round-trips every field");
	Expect(MoText(Back) == Line, "kast growth: kast\\tmo writes back byte for byte");
	Expect(P[39] == "30000.50,6.0;31000.25,10.0", "kast growth: RerunHeat's pairs (" + P[39] + ")");
	// The whole line, pinned: §8.1's field order with Firsts last, in fixture_tool's ProjectedMo widths (a writer and reader that
	// swapped two fields together would still round-trip).
	Expect(Line == "kast\tmo\t1\t41.25\t31234.50\t12.75\t-0.1234\t0.1500\t19\t31200.25\t21\t3\t19\t3\t17.50\t18\t5\t21\t180\t0\t61\t0\t240\t0\t33\t21\t17\t1\t20\t8600\t-1.00\t4.50"
				   "\t0.4321\t0.1000\t0.2500\t0.5000\t0.7500\t0.1250\t0.0625\t30000.50,6.0;31000.25,10.0\t1\n",
		"kast growth: kast\\tmo's text in §8.1's order (" + Line + ")");
	g::SaveView OneField = g::SaveView();
	OneField.Firsts = g::FirstStrangerTip;
	Expect(!MoText(OneField).empty(), "kast growth: one set field is something to write");
	std::vector<std::string> NoFirsts(P.begin(), P.end() - 1);
	g::SaveView Early;
	Expect(g::ReadLine(NoFirsts, Early) && Early.Firsts == 0 && Early.Heat == V.Heat, "kast growth: a line without Firsts reads (at least 40 fields)");
	std::vector<std::string> Short(P.begin(), P.end() - 2);
	g::SaveView Untouched = Filled();
	Expect(!g::ReadLine(Short, Untouched) && SameView(Untouched, Filled()), "kast growth: a short line isn't one and changes nothing");
	std::vector<std::string> Later = P;
	Later[2] = "2";
	Later.push_back("7");
	g::SaveView Next;
	Expect(g::ReadLine(Later, Next) && Next.Version == 2 && Next.Heat == V.Heat && Next.Firsts == V.Firsts, "kast growth: a later version's line reads its version 1 fields");
	Expect(!g::ReadLine(Split("kast\tgrowth\t0\t1260\t0.0"), Back), "kast growth: another kast line isn't kast\\tmo");
	g::SaveView Bare = Filled();
	Bare.RerunHeat.clear();
	std::vector<std::string> BareP = Split(MoText(Bare));
	g::SaveView BareBack;
	Expect(BareP.size() == 41 && BareP[39].empty() && g::ReadLine(BareP, BareBack) && BareBack.RerunHeat.empty() && BareBack.Firsts == Bare.Firsts,
		"kast growth: an empty RerunHeat keeps its field (Firsts stays last)");
	// The widest kast\tmo stays inside fixture_tool's ProjectedMo widths (its widest: 293 bytes).
	g::SaveView W;
	const int Today = 760;
	const double WideAt = Today * ss::net::MinutesPerDay + 23 * 60.0 + 59.5;
	W.Heat = 100.0;
	W.HeatAt = WideAt;
	W.Norm = 9999.99;
	W.Bias = -1.2345;
	W.RunNorm = 99.9999;
	W.BestAvg = 99999;
	W.PingAt = WideAt;
	W.LastStreamDay = W.FtBase = W.LiveDay = W.GrantDay = W.CotnDay = Today;
	W.ShowWeek = W.GainWeek = Today / 7;
	W.ShowDays = W.FtDays = 127;
	W.GainUsed = 35.0;
	W.LiveMin.fill(1440);
	W.GrantMask = 511;
	W.WelcomeRaidDone = true;
	W.PendingCents = 9999999;
	W.LiveSince = WideAt;
	W.GainSoFar = -99.99;
	W.CarryFollow = W.CarryWord = W.CarrySub = W.CarryTip = W.CarryCheer = W.CarryRaid = W.CarryGift = 0.9999;
	W.RerunHeat.assign(4, {WideAt, 12.0});
	W.Firsts = 1;
	const size_t WideMo = MoText(W).size();
	Expect(WideMo <= 293, "kast growth: the widest kast\\tmo is " + std::to_string(WideMo) + " bytes (fixture_tool's projection: 293)");

	// The member tail: empty when unset; the days written as days before the member's LastSeen day.
	ss::kast::Member M;
	M.Name = "kat_v";
	M.Loyalty = 0.5;
	M.FirstSeen = 10000.0;
	M.LastSeen = 21 * ss::net::MinutesPerDay + 1300.0;
	M.Follower = true;
	Expect(g::MemberTail(M, 21).empty(), "kast growth: a member with nothing new writes no tail");
	M.GaveDay = 21;
	M.Hw = 12.34;
	M.HwDay = 21;
	Expect(g::MemberTail(M, 21) == "\t0\t123\t0", "kast growth: the member tail \"" + g::MemberTail(M, 21) + "\" (want \"\\t0\\t123\\t0\")");
	ss::kast::Member R;
	std::vector<std::string> MP = Split(MemLine(M, 21));
	Expect(g::ReadMemberTail(MP, R) && R.GaveDay == 21 && R.Hw == 12.3 && R.HwDay == 21, "kast growth: the member tail round-trips (Hw in tenths)");
	M.Hw = 12.3;
	Expect(MemLine(M, 21) == "kast\tmem\tkat_v\t0.5000\t0.3000\t10000.00\t31540.00\t0\t0.0\t0\t1\t0\t0\t123\t0", "kast growth: the whole member line (" + MemLine(M, 21) + ")");
	M.GaveDay = 15;
	M.HwDay = 19;
	MP = Split(MemLine(M, 21));
	R = ss::kast::Member();
	Expect(g::ReadMemberTail(MP, R) && R.GaveDay == 15 && R.HwDay == 19 && MP[12] == "6" && MP[14] == "2", "kast growth: earlier days are offsets from LastSeen's day");
	ss::kast::Member Late = M;
	Late.LastSeen = 200 * ss::net::MinutesPerDay + 600.0;
	Late.GaveDay = 200 - 91;
	Late.Hw = 0.04;
	Late.HwDay = 200;
	Expect(g::MemberTail(Late, 200).empty(), "kast growth: a gift 91 days ago and 0.04 hours watched write nothing");
	// Hw is written while its *decayed* value is above 0.05 (§8.1): an hour watched 30 days ago is 0.014 today.
	ss::kast::Member Faded = Late;
	Faded.GaveDay = -1;
	Faded.Hw = 1.0;
	Faded.HwDay = 170;
	Expect(g::MemberTail(Faded, 200).empty(), "kast growth: an hour watched 30 days ago (0.014 today) writes no tail");
	Faded.HwDay = 190;
	Expect(g::MemberTail(Faded, 200) == "\t\t10\t10", "kast growth: an hour watched 10 days ago (0.24 today) writes its tail (" + g::MemberTail(Faded, 200) + ")");
	// The C-K gate: the test reads Hw as the save will hold it (tenths). 0.44 hours 15 days ago decays to 0.052, but the 0.4 the
	// save would keep decays to 0.047: no tail, or the next write would drop it and a parse-only round trip would differ. And every
	// tail, read back and written again the same day, is the same tail.
	{
		ss::kast::Member Edge = Faded;
		Edge.Hw = 0.44;
		Edge.HwDay = 185;
		Expect(g::MemberTail(Edge, 200).empty(), "kast growth: Hw's tail is tested in the tenths it's saved in (0.44 -> 0.4, 15 days: none)");
		int Unstable = 0;
		for (int H = 1; H <= 120; ++H)
		{
			for (int Days = 0; Days <= 40; ++Days)
			{
				ss::kast::Member A = Faded;
				A.Hw = 0.0137 * H;
				A.HwDay = 200 - Days;
				ss::kast::Member B = A;
				B.Hw = 0.0;
				B.HwDay = -1;
				const std::vector<std::string> AP = Split(MemLine(A, 200));
				g::ReadMemberTail(AP, B);
				Unstable += g::MemberTail(B, 200) == g::MemberTail(A, 200) ? 0 : 1;
			}
		}
		Expect(Unstable == 0, "kast growth: every member tail reads back and writes again as itself (" + std::to_string(Unstable) + " of 4,920 differ)");
	}
	Late.GaveDay = 200 - 90;
	MP = Split(MemLine(Late, 200));
	R = ss::kast::Member();
	Expect(g::MemberTail(Late, 200) == "\t90" && g::ReadMemberTail(MP, R) && R.GaveDay == 200 - 90 && R.HwDay == -1, "kast growth: a gift 90 days ago writes only its day");
	M.GaveDay = -1;
	M.Hw = 3.0;
	M.HwDay = 2;
	MP = Split(MemLine(M, 21));
	R = ss::kast::Member();
	Expect(g::MemberTail(M, 21) == "\t\t30\t19" && g::ReadMemberTail(MP, R) && R.GaveDay == -1 && R.Hw == 3.0 && R.HwDay == 2, "kast growth: hours watched alone");
	// LastSeen just before midnight writes as the next day: the base is the saved text's day, both ways.
	M.LastSeen = 2.0 * ss::net::MinutesPerDay - 0.001;
	M.GaveDay = 1;
	M.Hw = 0;
	M.HwDay = -1;
	MP = Split(MemLine(M, 1));
	R = ss::kast::Member();
	Expect(MP[6] == "2880.00" && MP[12] == "1" && g::ReadMemberTail(MP, R) && R.GaveDay == 1, "kast growth: the tail's base day is LastSeen as saved");
	ss::kast::Member Old;
	Expect(!g::ReadMemberTail(Split("kast\tmem\tdee\t0.8500\t0.9500\t1440.00\t1440.00\t1\t30.0\t4\t3\t500"), Old) && Old.GaveDay == -1,
		"kast growth: an old member line has no tail");
	// The widest member tail: "\t0\t999\t0" (8 bytes) for a member who gave and watched on the day last seen.
	ss::kast::Member Wide;
	Wide.LastSeen = WideAt;
	Wide.GaveDay = Today;
	Wide.Hw = 99.9;
	Wide.HwDay = Today;
	Expect(g::MemberTail(Wide, Today) == "\t0\t999\t0", "kast growth: the widest member's tail is 8 bytes (fixture_tool's projection: 12)");

	// The log tail.
	ss::kast::StreamLog Lg;
	Expect(g::LogTail(Lg).empty(), "kast growth: an old log row writes no tail");
	ss::kast::StreamLog Read;
	Expect(!g::ReadLogTail(Split("kast\tlog\t1440.00\t60.00\t3\t5\t1\t0\t0\tgrind\t2\t1"), Read) && Read.AvgOwn == -1, "kast growth: an old log row reads no tail");
	Lg.AvgOwn = 41;
	Lg.PeakAt = 128.5;
	Lg.PeakMoment = static_cast<int>(ss::kast::Moment::FinalTable);
	Lg.HeatAfter = 47.5;
	Expect(g::LogTail(Lg) == "\t41\t128.5\t9\t47.5", "kast growth: the log tail (" + g::LogTail(Lg) + ")");
	Expect(g::ReadLogTail(Split("kast\tlog\t1440.00\t60.00\t3\t5\t1\t0\t0\tgrind\t2\t1" + g::LogTail(Lg)), Read) && Read.AvgOwn == 41 && Read.PeakAt == 128.5 && Read.PeakMoment == 9 &&
			Read.HeatAfter == 47.5,
		"kast growth: the log tail round-trips");
	Lg.PeakMoment = ss::kast::PeakRaid; // a peak a network raid brought (the C-K PIE fix, F1; review-pie-room)
	Expect(g::LogTail(Lg) == "\t41\t128.5\t-2\t47.5" && g::ReadLogTail(Split("kast\tlog\t1440.00\t60.00\t3\t5\t1\t0\t0\tgrind\t2\t1" + g::LogTail(Lg)), Read) &&
			Read.PeakMoment == ss::kast::PeakRaid,
		"kast growth: the log tail round-trips a raid's peak (PeakRaid, -2)");
	ss::kast::StreamLog WideLog;
	WideLog.AvgOwn = 9999;
	WideLog.PeakAt = 1439.5;
	WideLog.PeakMoment = static_cast<int>(ss::kast::Moment::Count) - 1;
	WideLog.HeatAfter = 100.0;
	Expect(g::LogTail(WideLog).size() <= 21, "kast growth: the widest log tail is " + std::to_string(g::LogTail(WideLog).size()) + " bytes (projection 21)");
	// The goodwill tail.
	Expect(g::GoodwillTail(-1).empty() && g::GoodwillTail(760) == "\t760", "kast growth: the goodwill tail");
	Expect(g::ReadGoodwillTail(Split("kast\tgoodwill\tpocketqueens_pk\t2")) == -1 && g::ReadGoodwillTail(Split("kast\tgoodwill\tpocketqueens_pk\t2" + g::GoodwillTail(31))) == 31,
		"kast growth: the goodwill tail round-trips (an old line reads -1)");
}

// ------------------------------------------------------------------ seams the tables above don't pin (review-core-kastmath, wave 2)

/** One SaveView field set away from its default (the review's field-by-field check of kast\tmo). */
struct MoField
{
	const char* Name;
	void (*Set)(g::SaveView&);
};

void Seams()
{
	// Unit's two integers are mixed one at a time: (A, B) and (B, A), or a guest's (minute 5, index 0) and (minute 4, index 1),
	// are different keys. A single "A ^ B" step passed every other check here while making the Room's (minute, index) draws repeat.
	std::set<double> Grid;
	bool Swapped = true;
	for (uint64_t A = 0; A < 64; ++A)
	{
		for (uint64_t B = 0; B < 64; ++B)
		{
			const double U = g::Unit(0x5eed, 9, g::Tag::GuestStay, A, B);
			Grid.insert(U);
			Swapped = Swapped && (A == B || U != g::Unit(0x5eed, 9, g::Tag::GuestStay, B, A));
		}
	}
	Expect(Grid.size() == 64 * 64 && Swapped, "kast growth: Unit keeps A and B apart (" + std::to_string(Grid.size()) + " distinct draws of 4,096; (A, B) != (B, A))");

	// H7 read on the day before the ring's newest day (a go-live after an earlier stream today: the Room's RoomH7Day) never sums
	// the newest day's slot, which held the day a week before; a stream older than the ring holds changes nothing.
	std::array<int, 7> Ring{};
	int RingDay = -1;
	g::LiveAdd(Ring, RingDay, 11, 120);
	g::LiveAdd(Ring, RingDay, 17, 60);
	At("H7 the day before the ring's newest day", g::H7(Ring, RingDay, 16), 2.0);
	At("H7 on the ring's newest day", g::H7(Ring, RingDay, 17), 3.0);
	g::LiveAdd(Ring, RingDay, 10, 999);
	Expect(RingDay == 17 && g::H7(Ring, RingDay, 17) == 3.0 && g::H7(Ring, RingDay, 16) == 2.0, "kast growth: a stream older than the ring holds books nothing");

	// kast\tmo is written when any one field holds something, and each such line reads back to the same view (a channel whose
	// only news is pending money keeps it).
	static const MoField Fields[] = {{"Version", [](g::SaveView& V) { V.Version = 2; }}, {"Heat", [](g::SaveView& V) { V.Heat = 30.5; }},
		{"HeatAt", [](g::SaveView& V) { V.HeatAt = 2000.25; }}, {"Norm", [](g::SaveView& V) { V.Norm = 4.5; }}, {"Bias", [](g::SaveView& V) { V.Bias = 0.25; }},
		{"RunNorm", [](g::SaveView& V) { V.RunNorm = 0.15; }}, {"BestAvg", [](g::SaveView& V) { V.BestAvg = 3; }}, {"PingAt", [](g::SaveView& V) { V.PingAt = 1500.5; }},
		{"LastStreamDay", [](g::SaveView& V) { V.LastStreamDay = 4; }}, {"ShowWeek", [](g::SaveView& V) { V.ShowWeek = 0; }},
		{"ShowDays", [](g::SaveView& V) { V.ShowDays = 2; }}, {"GainWeek", [](g::SaveView& V) { V.GainWeek = 0; }}, {"GainUsed", [](g::SaveView& V) { V.GainUsed = 3.5; }},
		{"FtBase", [](g::SaveView& V) { V.FtBase = 4; }}, {"FtDays", [](g::SaveView& V) { V.FtDays = 1; }}, {"LiveDay", [](g::SaveView& V) { V.LiveDay = 4; }},
		{"LiveMin", [](g::SaveView& V) { V.LiveMin[3] = 75; }}, {"GrantDay", [](g::SaveView& V) { V.GrantDay = 4; }},
		{"GrantMask", [](g::SaveView& V) { V.GrantMask = g::GrantBroadcast; }}, {"WelcomeRaidDone", [](g::SaveView& V) { V.WelcomeRaidDone = true; }},
		{"CotnDay", [](g::SaveView& V) { V.CotnDay = 4; }}, {"PendingCents", [](g::SaveView& V) { V.PendingCents = 1350; }},
		{"LiveSince", [](g::SaveView& V) { V.LiveSince = 6000.5; }}, {"GainSoFar", [](g::SaveView& V) { V.GainSoFar = -2.5; }},
		{"CarryFollow", [](g::SaveView& V) { V.CarryFollow = 0.5; }}, {"CarryWord", [](g::SaveView& V) { V.CarryWord = 0.5; }},
		{"CarrySub", [](g::SaveView& V) { V.CarrySub = 0.5; }}, {"CarryTip", [](g::SaveView& V) { V.CarryTip = 0.5; }},
		{"CarryCheer", [](g::SaveView& V) { V.CarryCheer = 0.5; }}, {"CarryRaid", [](g::SaveView& V) { V.CarryRaid = 0.5; }},
		{"CarryGift", [](g::SaveView& V) { V.CarryGift = 0.5; }}, {"RerunHeat", [](g::SaveView& V) { V.RerunHeat = {{9000.5, 6.0}}; }},
		{"Firsts", [](g::SaveView& V) { V.Firsts = g::FirstStrangerTip; }}};
	std::string Silent;
	for (const MoField& F : Fields)
	{
		g::SaveView One;
		F.Set(One);
		const std::string Line = MoText(One);
		g::SaveView Back;
		if (Line.empty() || !g::ReadLine(Split(Line), Back) || !SameView(Back, One))
		{
			Silent += std::string(Silent.empty() ? "" : ", ") + F.Name;
		}
	}
	Expect(Silent.empty(), "kast growth: kast\\tmo writes and reads back a view with any one field set (" + (Silent.empty() ? std::string("all 33") : "not: " + Silent) + ")");

	// Presence between arrival and the long run: the slot process's closed form decays at 1/Sm + 1/G.
	At("Presence half an hour in", g::Presence(30, 119, 48), 119.0 / 167.0 + (48.0 / 167.0) * std::exp(-30.0 * (1.0 / 119.0 + 1.0 / 48.0)), 1e-12);

	// C13's "once a world day" is the world day: a read paid late last night doesn't hold this morning's.
	const double Day = ss::net::MinutesPerDay;
	Expect(g::SponsorReadPays(10, 9 * Day + 1430.0, 45, 3, 0, 8) && !g::SponsorReadPays(10, 10 * Day + 0.5, 45, 3, 0, 8),
		"kast growth: a read paid at 11:50 PM doesn't hold the next world day's; one paid at 12:00 AM holds the rest of that day");
	Expect(g::SponsorReadPays(10, 9 * Day + 1430.0, 45, 3, 7, 0) && !g::SponsorReadPays(10, 10 * Day + 0.5, 45, 3, 7, 0),
		"kast growth: the same world day for a deal that asks for no reads (M1)");

	// The Broadcast's teases name a price only where it's a fact: the cheapest nightly event (every day, no series) whose usual
	// field reaches the Broadcast's P 1 is a $5 event on RiverLine and Ignite ("features final tables from $5"); RunItTwice's
	// nightlies at $5.50 never do (its first is $33), so its tease and the fallback name no price.
	for (const ss::net::SiteId S : {ss::net::SiteId::RiverLine, ss::net::SiteId::Ignite, ss::net::SiteId::RunItTwice})
	{
		ss::Chips Cheapest = -1;
		for (const ss::net::EventTemplate& T : ss::net::Site(S).Templates())
		{
			if (T.Days != 0x7f || T.OnlyDay != -9999 || !T.Series.empty() || T.Fmt == ss::net::Format::Satellite || !g::Armed(T.Field))
			{
				continue;
			}
			const double P = g::Prestige(T.Field, T.BuyInCents, T.Main ? 1.0 : (T.Bracelet || T.Ring) ? 0.5 : 0.0);
			if (P >= 1.0 && g::BroadcastSize(P) > 0 && (Cheapest < 0 || T.BuyInCents < Cheapest))
			{
				Cheapest = T.BuyInCents;
			}
		}
		const std::string Tease = ss::kast::copy::BroadcastTease(static_cast<int>(S));
		const bool Priced = Tease.find("from $5") != std::string::npos;
		const bool Digit = Tease.find_first_of("0123456789") != std::string::npos;
		Expect(Priced ? Cheapest >= 500 && Cheapest < 600 : !Digit && (Cheapest < 0 || Cheapest >= 600),
			"kast growth: the Broadcast's tease \"" + Tease + "\" is a fact (the site's cheapest nightly Broadcast event: " + (Cheapest < 0 ? std::string("none") : ss::Money(Cheapest)) + ")");
	}
	Expect(ss::kast::copy::BroadcastTease(static_cast<int>(ss::net::SiteId::Halcyon)).find_first_of("0123456789") == std::string::npos,
		"kast growth: the fallback tease names no price");

	// The copy's signed numbers round to the number shown (Heat's ghost fill and the end card's move are the same commit).
	Expect(ss::kast::copy::GhostTonight(18.6) == "tonight +19 so far" && ss::kast::copy::GhostTonight(-3.5) == "tonight -4 so far" &&
			ss::kast::copy::HeatMove(1, 2, 17.5) == "Warming up \xE2\x86\x92 Running hot \xC2\xB7 +18",
		"kast growth: the copy rounds Heat's gain to the whole number shown (" + ss::kast::copy::GhostTonight(18.6) + ")");
}

// ------------------------------------------------------------------ the copy (KastCopy.h, §5.3, §5.5)

/** Every variable the templates use, at a realistic length. */
std::map<std::string, std::string> CopyVars()
{
	return {{"N", "1,150"}, {"Of", "640"}, {"Pct", "64"}, {"Watching", "12"}, {"Chatting", "5"}, {"Here", "2"}, {"Reached", "412"}, {"Followed", "31"},
		{"Rank", "23"}, {"Saw", "41"}, {"Came", "2"}, {"Gain", "+18"}, {"Tier", "Running hot"}, {"Heat", "41"}, {"When", "Last night's"}, {"Stage", "final table"},
		{"Runners", "1,150"}, {"Title", "kings, 82%\xE2\x80\xA6"}, {"Peak", "41"}, {"Size", "14"}, {"Hours", "38"}, {"Event", "$11 Big Stack"}, {"Site", "RiverLine"},
		{"From", "31"}, {"To", "6"}, {"Lo", "8"}, {"Hi", "14"}, {"Schedule", "on schedule"}, {"At15", "6"}, {"Holds", "71"}, {"Cracks", "29"}, {"Call", "HOLDS"},
		{"Right", "3"}, {"Points", "340"}, {"Calls", "4"}, {"At", "at the final table"}, {"A", "kat_v"}, {"B", "nitbox"}, {"More", "3"}, {"Tips", "$24"},
		{"Cheers", "$11"}, {"ToGo", "88"}, {"Goal", "5,000"}, {"Followers", "25"}, {"Minutes", "240"}, {"Days", "4"}, {"Average", "3"}, {"Tag", "directory"},
		{"Who", "nightrail_88"}, {"Amount", "$134"}, {"Brand", "TunnelRat"}, {"Day", "Tuesday"}, {"Time", "9:48 PM"}};
}

size_t KindMax(c::Kind K)
{
	switch (K)
	{
	case c::Kind::Chat: return copylint::TableTalk;
	case c::Kind::Letter: return copylint::LetterBody;
	default: return copylint::Pa;
	}
}

/** Whole lower-case words of a text (letters only). */
std::set<std::string> Words(const std::string& Text)
{
	std::set<std::string> Out;
	std::string Word;
	for (const char Ch : copylint::Lower(Text) + " ")
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Word.push_back(Ch);
			continue;
		}
		if (!Word.empty())
		{
			Out.insert(Word);
		}
		Word.clear();
	}
	return Out;
}

void Copy()
{
	const std::vector<c::Entry> All = c::Entries();
	const std::vector<std::string> Inv = ss::kast::Inventory();
	Expect(All.size() >= 250 && Inv.size() == All.size(), "kast growth: the copy inventory holds every template (" + std::to_string(All.size()) + ")");
	bool InvSame = Inv.size() == All.size();
	for (size_t I = 0; InvSame && I < All.size(); ++I)
	{
		InvSame = Inv[I] == All[I].Text;
	}
	Expect(InvSame, "kast growth: kast::Inventory() is the entries' texts");
	// Keys: unique but for the chat pools ("pool." keys), and every pool is chat.
	std::set<std::string> Keys;
	bool Unique = true, PoolsChat = true;
	for (const c::Entry& E : All)
	{
		const bool Pool = E.Key.rfind("pool.", 0) == 0;
		PoolsChat = PoolsChat && (!Pool || E.Type == c::Kind::Chat);
		if (!Pool)
		{
			Unique = Keys.insert(E.Key).second && Unique;
		}
	}
	Expect(Unique && PoolsChat, "kast growth: every copy key is unique (the chat pools apart)");
	// The copy lint: sentences as they are; chat, labels and toasts with no closing stop get one (the rest holds: filled, no
	// doubled space, nothing banned, within the kind's length).
	std::map<std::string, std::string> Vars = copylint::Samples();
	for (const auto& Kv : CopyVars())
	{
		Vars[Kv.first] = Kv.second;
	}
	int Problems = 0;
	for (const c::Entry& E : All)
	{
		const bool Sentence = E.Type == c::Kind::System || E.Type == c::Kind::Letter;
		const std::string Text = E.Text;
		const bool Stops = copylint::EndsWith(Text, ".") || copylint::EndsWith(Text, "!") || copylint::EndsWith(Text, "?") || copylint::EndsWith(Text, "\"") ||
			copylint::EndsWith(Text, "'") || copylint::EndsWith(Text, "\xE2\x80\x9D") || copylint::EndsWith(Text, "\xE2\x80\x99");
		const std::string Why = copylint::Check(Sentence || Stops ? Text : Text + ".", KindMax(E.Type), Vars);
		if (!Why.empty())
		{
			++Problems;
			std::printf("COPY (kast) %s: \"%s\": %s\n", E.Key.c_str(), E.Text.c_str(), Why.c_str());
		}
		if (Sentence && !Stops)
		{
			++Problems;
			std::printf("COPY (kast) %s: a sentence without its stop: \"%s\"\n", E.Key.c_str(), E.Text.c_str());
		}
	}
	Expect(Problems == 0, "kast growth: every Kast template passes the copy lint (" + std::to_string(Problems) + " problems)");
	std::printf("kast growth: copy: %zu templates (%zu keys), linted, both trademark tiers and U1 screened\n", All.size(), Keys.size());
	// The trademark screen, both tiers, over every template and every accessor's text below.
	int Marks = 0;
	for (const c::Entry& E : All)
	{
		const char* Hit = trademarks::Find(E.Text, trademarks::kSourceScan);
		const char* CopyHit = trademarks::Find(E.Text, trademarks::kCopyScan);
		if (Hit || CopyHit)
		{
			++Marks;
			std::printf("TRADEMARK (kast) %s: \"%s\" holds \"%s\"\n", E.Key.c_str(), E.Text.c_str(), Hit ? Hit : CopyHit);
		}
	}
	Expect(Marks == 0, "kast growth: no real name or platform feature in the Kast copy (both tiers)");
	// U1 and the house words: nothing says AI, bot or autoplay; "heads-down" is the only pace word; Heat is a thermometer (no
	// fire words; "spotlight" waits for kCopyScan but stays out of Kast's copy); chat never uses a tier word.
	int Banned = 0;
	for (const c::Entry& E : All)
	{
		const std::set<std::string> W = Words(E.Text);
		const std::string Low = copylint::Lower(E.Text);
		for (const char* Word : {"ai", "bot", "bots", "autoplay", "autopilot", "sprint", "sprinting", "skip", "skipping", "speed", "x", "flame", "flames", "fire", "fired",
				 "spark", "sparks", "blaze", "blazing", "burn", "burning", "spotlight", "drops"})
		{
			if (W.count(Word))
			{
				++Banned;
				std::printf("COPY (kast, U1/house words) %s: \"%s\" says \"%s\"\n", E.Key.c_str(), E.Text.c_str(), Word);
			}
		}
		for (const char* Pace : {"1x", "2x", "4x", "8x", "watch 4", "fast-forward"})
		{
			if (Low.find(Pace) != std::string::npos)
			{
				++Banned;
				std::printf("COPY (kast, pace) %s: \"%s\" names a pace (\"%s\")\n", E.Key.c_str(), E.Text.c_str(), Pace);
			}
		}
		if (E.Type == c::Kind::Chat)
		{
			for (const char* TierWord : {"warming up", "running hot", "on a heater", "sun run", "cold"})
			{
				if (trademarks::HasWord(Low, TierWord))
				{
					++Banned;
					std::printf("COPY (kast, chat tier word) %s: \"%s\" says \"%s\"\n", E.Key.c_str(), E.Text.c_str(), TierWord);
				}
			}
		}
	}
	Expect(Banned == 0, "kast growth: the Kast copy never says AI, bot, autoplay, a pace or a fire word, and chat never a tier word");

	// The accessors against §5.3's examples (every digit filled from its fact).
	const auto Is = [](const std::string& Got, const std::string& Want) {
		Expect(Got == Want, "kast growth: copy \"" + Got + "\" (want \"" + Want + "\")");
	};
	const std::string Dot = "\xC2\xB7";
	Is(c::Dollars(500), "$5");
	Is(c::Dollars(13400), "$134");
	Is(c::Dollars(520), "$5.20");
	Is(c::Dollars(118460), "$1,184.60");
	Is(c::Dollars(0), "$0");
	Is(c::Number(1150), "1,150");
	Is(c::WatchingChatting(12, 5), "12 watching " + Dot + " 5 chatting");
	Is(c::ModsWatching(2), "2 moderators are watching chat.");
	Is(c::ModsWatching(1), "1 moderator is watching chat.");
	Is(c::ModsHere(2, 3), "2 of 3 here");
	Is(c::HeadsDown(), "heads-down tonight: chat is quieter");
	Is(c::ChattedTonight(38), "38 people chatted tonight");
	Is(c::ChattedTonight(1), "1 person chatted tonight");
	Is(c::GoLive(412, 640), "You're live! Your ping reached 412 of 640 followers.");
	Is(c::GoLive(2, 2), "You're live! Your ping reached 2 of 2 followers.");
	Is(c::GoLive(1, 1), "You're live! Your ping reached 1 of 1 follower.");
	Is(c::GoLive(0, 0), "You're live! Share the link and say hi to chat.");
	Is(c::GoLiveAfterglow(214, 290, 31), "You're live! Your ping reached 214 of 290 " + Dot + " 31 followed you last night.");
	Is(c::PingToast(412, 640), "Your ping reached 412 of 640 followers (64%)");
	Is(c::DeeHi(), "LIVE!! the laundromat is watching");
	Is(c::MeiHi(), "mei is here");
	Is(c::DeeEmbercrest(), "how was the Embercrest??");
	Is(c::FirstFollower("nightrail_88"), "nightrail_88 followed: your first follower who isn't family");
	Is(c::EmptyRank(41, 49), "Your stream is #41 of 49 live in Poker.");
	Is(c::EmptySeen(41, 2), "41 people saw your stream in the directory in the last 30 minutes; 2 came in.");
	Is(c::EmptySeen(1, 1), "1 person saw your stream in the directory in the last 30 minutes; 1 came in.");
	Is(c::EmptySeen(41, 0), "41 people saw your stream in the directory in the last 30 minutes; none came in yet.");
	Is(c::EmptySeen(0, 0), "Nobody saw your stream in the directory in the last 30 minutes.");
	Is(c::EmptyFresh(), "New on Kast: you're on the Fresh shelf tonight.");
	Is(c::EmptyRising(), "Rising in Poker: Kast shows small channels to new viewers.");
	Is(c::SetTheScene(), "You talk to the camera about tonight's tournament.");
	Is(c::PingReach(64), "Ping reach last stream 64%");
	Is(c::RunAlertSent(256), "RUN ALERT sent to 256 followers");
	Is(c::RunAlertBack(12), "12 back so far");
	Is(c::RailCallReached(37), "Your rail call reached 37 followers.");
	Is(c::RailCallReached(1), "Your rail call reached 1 follower.");
	Is(c::WholeRailHere(), "Your whole rail is here");
	Is(c::DeeOmw(), "omw!!");
	Expect(c::RunAlertReturn(g::RunStage::FinalTable).front() == "back for the FT!" && c::RunAlertReturn(g::RunStage::Bubble).size() >= 3 &&
			c::RunAlertReturn(g::RunStage::Itm).size() >= 3 && c::RunAlertReturn(g::RunStage::Top3).size() >= 3,
		"kast growth: the Run Alert returners' lines, by stage");
	Expect(c::RailCallArrive().front() == "rail call got me", "kast growth: Rail Call's arrival lines");
	Is(c::TierName(0), "Cold");
	Is(c::TierName(1), "Warming up");
	Is(c::TierName(2), "Running hot");
	Is(c::TierName(3), "On a heater");
	Is(c::TierName(4), "Sun run");
	Is(c::GhostTonight(18.4), "tonight +18 so far");
	Is(c::GhostTonight(-4.2), "tonight -4 so far");
	Is(c::GhostSettling(), "tonight: settling in"); // M4 (fix-ui): the ghost's line before the night has settled
	Is(c::WatchingTier(12, 2), "12 watching " + Dot + " Running hot");
	Is(c::WhyRun(g::RunStage::FinalTable, 1150, 1, "Monday"), "Last night's final table (1,150 runners)");
	Is(c::WhyRun(g::RunStage::FinalTable, 980, 0, "Monday"), "Tonight's final table (980 runners)");
	Is(c::WhyRun(g::RunStage::Win, 5000, 3, "Sunday"), "Sunday's win (5,000 runners)");
	Is(c::WhyOverForecast(38), "38% over your forecast");
	Is(c::WhyShowingUp(4), "Showing up: 4 nights this week");
	Is(c::WhyCooling(9), "Cooling: 9 days dark");
	Is(c::WhyClipTop(), "Your clip hit the top of the Kast feed");
	Is(c::WhyVideo("kings, 82% of the time and other lies", false), "Your video kings, 82%\xE2\x80\xA6 got pushed hard");
	Is(c::WhyVideo("short title", true), "Your video short title broke out");
	Is(c::WhyHotWeek(), "Hot week: Heat can't climb any faster this week");
	Is(c::FrontRow(), "Kast put you in Front Row in Poker.");
	Is(c::StaminaLine(38), "Stamina: 38 hours live this week. Chat can tell.");
	Is(c::EventBar(true, "$11 Big Stack", 7, 980), "FINAL TABLE " + Dot + " $11 Big Stack " + Dot + " 7 left of 980");
	Is(c::EventBar(false, "$11 Big Stack", 14, 980), "LAST TWO TABLES " + Dot + " $11 Big Stack " + Dot + " 14 left of 980");
	Is(c::DirectoryEvent("$3.30 Headhunter PKO", 6), "FINAL TABLE " + Dot + " $3.30 Headhunter PKO " + Dot + " 6 left");
	Expect(c::SweatChat().size() >= 3 && c::SweatChat()[0] == "ICM spot here" && c::SweatChat()[2] == "THIS IS IT", "kast growth: chat sweats the spot");
	Is(c::Broadcast(static_cast<int>(ss::net::SiteId::RiverLine), 41), "RiverLine Live sent 41 viewers: one of our final tablists is streaming.");
	Is(c::Broadcast(static_cast<int>(ss::net::SiteId::Ignite), 41), "Ignite's Bandstand sent 41 viewers.");
	Is(c::Broadcast(static_cast<int>(ss::net::SiteId::RunItTwice), 41), "RunItTwice's Newsdesk sent 41 viewers.");
	Is(c::Broadcast(static_cast<int>(ss::net::SiteId::Halcyon), 41), "The site's broadcast sent 41 viewers.");
	Is(c::BroadcastTease(static_cast<int>(ss::net::SiteId::RiverLine)), "RiverLine Live features final tables from $5");
	Expect(c::BroadcastChat(static_cast<int>(ss::net::SiteId::RiverLine)).front() == "RiverLine Live brought me, gl" && !c::BroadcastChat(99).empty(),
		"kast growth: the Broadcast's raiders speak for their site");
	Is(c::RaidedWith("VikingVolta", 312), "VikingVolta raided with 312");
	Is(c::RankHeader(23, 49), "#23 of 49 in Poker");
	Is(c::RankClimb(31, 6), "You climbed from #31 to #6 in Poker tonight");
	Is(c::RankClimb(6, 6), "Best tonight: #6 in Poker");
	Is(c::RankNow(8), "You're #8 in Poker right now");
	Is(c::NewOnKastChip(), "NEW ON KAST");
	Is(c::RisingChip(), "RISING IN POKER");
	for (int S = 0; S < static_cast<int>(ss::kast::View::Source::Count); ++S)
	{
		Expect(!c::SourceTag(static_cast<ss::kast::View::Source>(S)).empty() && !c::SourceCause(static_cast<ss::kast::View::Source>(S)).empty(),
			"kast growth: every source has its words (" + std::to_string(S) + ")");
	}
	Is(c::SourceCount(ss::kast::View::Source::Directory, 4), "4 directory");
	Is(c::SourceCause(ss::kast::View::Source::Members), "the ping");
	Is(c::CameFromVideo("kings"), "came from the kings video");
	Is(c::ForecastBand(8.2, 13.8), "Tonight: likely 8-14");
	Is(c::ForecastBand(1.4, 1.2), "Tonight: likely 1");
	Is(c::ForecastContext("Tuesday", 1, 2), "Tuesday " + Dot + " on schedule " + Dot + " Running hot");
	Is(c::ForecastShort(41.4, 63.2), "forecast 41-63");
	g::Band Band;
	Band.Lo = 8;
	Band.Centre = 10;
	Band.Hi = 14;
	Is(c::ForecastVerdict(15, Band), "above your forecast");
	Is(c::ForecastVerdict(12, Band), "inside your forecast");
	Is(c::ForecastVerdict(7, Band), "under your forecast");
	Is(c::WelcomeTheRaidButton(), "WELCOME THE RAID");
	Is(c::WelcomeRaid("pocketqueens_pk", 8), "pocketqueens_pk raided with 8: welcome to Kast!");
	Is(c::RaidsOwed(3), "3 channels owe you a raid");
	Is(c::RaidsOwed(1), "1 channel owes you a raid");
	Is(c::RaidsOwedShort(2), "2 raids owed");
	Is(c::RaidedAgo("pocket_aces_pk", 3, "Friday"), "You raided pocket_aces_pk 3 days ago (no new token until Friday)");
	Is(c::RaidRetention("kat_v", 14, 6, 2), "kat_v raided with 14 " + Dot + " still here at 15 min: 6 " + Dot + " followed: 2");
	// The Broadcast isn't a raid: its end-card line says "sent", as K6's alert does (ui-kast's ask; review-ui-kast's finding).
	Is(c::BroadcastRetention("RiverLine Live", 41, 18, 3), "RiverLine Live sent 41 " + Dot + " still here at 15 min: 18 " + Dot + " followed: 3");
	Is(c::NodsLeft(2), "2 Nods left");
	Is(c::NodsLeft(1), "1 Nod left");
	Is(c::NodsLeft(0), "No Nods left");
	Is(c::ThankHeard(), "(chat heard you)");
	Expect(c::NodReply().front() == "he noticed me kastLove" && c::AnswerFollowed().front() == "thanks for answering! followed", "kast growth: Nod's and Answer's replies");
	Is(c::HoldOrCrackTitle(), "HOLD OR CRACK?");
	Is(c::HoldOrCrackCounts(2, 1), "2 HOLDS " + Dot + " 1 CRACKS");
	Is(c::HoldOrCrackShares(71, 29), "HOLDS 71% " + Dot + " CRACKS 29%");
	Is(c::HoldOrCrackShares(2, 1), "HOLDS 67% " + Dot + " CRACKS 33%");
	Is(c::YourEquity(0.64), "your equity 64%");
	Is(c::YourEquity(-1), "");
	Is(c::CallIt(true), "Call it: HOLDS");
	Is(c::CallIt(false), "Call it: CRACKS");
	Is(c::StreamerCall(true), "HOLDS. calling it");
	Is(c::RevealRight(71), "71% of chat called it.");
	Is(c::RevealLine(true, 71, 100, 340), "HOLDS! 71% of chat called it: 340 points.");
	Is(c::RevealLine(false, 3, 4, 120), "CRACKS! 3 of 4 called it: 120 points.");
	Is(c::RevealLine(true, 0, 4, 0), "HOLDS! Nobody in chat called it.");
	Expect(c::RevealWrong().front() == "the rail is in shambles" && !c::ChatCalls(true).empty() && !c::ChatCalls(false).empty() && !c::CalledItChat().empty() &&
			!c::MissedCallChat().empty(),
		"kast growth: Hold or Crack's chat");
	Is(c::BestCaller("kat_v", 4, 5), "kat_v called 4 of 5");
	Is(c::ClipOfTheNight("river nine", true), "Clip of the night: \xE2\x80\x9Criver nine\xE2\x80\x9D " + Dot + " in the Kast feed");
	Is(c::ClipItButton(), "Clip it");
	Is(c::MakeItACutButton(), "Make it a Cut");
	Is(c::ClipTopOfFeed(), "Your clip is at the top of the Kast feed");
	Is(c::WelcomeBack(14), "Welcome back: 14 familiar faces checked in.");
	Is(c::WelcomeBack(1), "Welcome back: 1 familiar face checked in.");
	Is(c::StreakRow(6), "Streak 6 weeks");
	Is(c::StreakRow(1), "Streak 1 week");
	Is(c::PeakLine(41, static_cast<int>(ss::kast::Moment::FinalTable), "9:48 PM"), "Peak 41 at the final table, 9:48 PM");
	Is(c::PeakLine(12, -1, "9:48 PM"), "Peak 12 at 9:48 PM");
	Is(c::PeakLine(30, ss::kast::PeakRaid, "9:48 PM"), "Peak 30 on a raid, 9:48 PM"); // the C-K PIE fix (F1)
	for (int M = 0; M < static_cast<int>(ss::kast::Moment::Count); ++M)
	{
		const std::string Line = c::PeakLine(5, M, "1:00 AM");
		Expect(Line.find("{") == std::string::npos && Line.find(" , ") == std::string::npos && Line.find("Peak 5 ") == 0, "kast growth: the peak line at moment " + std::to_string(M) + " (" + Line + ")");
	}
	Is(c::NewFollowersLine(46, true), "+46 " + Dot + " best night ever");
	Is(c::NewFollowersLine(3, false), "+3");
	Is(c::RollCall({"kat_v", "nitbox", "a", "b", "c"}), "kat_v, nitbox and 3 more sweated the whole run");
	Is(c::RollCall({"kat_v", "nitbox"}), "kat_v and nitbox sweated the whole run");
	Is(c::RollCall({"kat_v"}), "kat_v sweated the whole run");
	Is(c::RollCall({}), "");
	Is(c::HeatMove(1, 2, 18), "Warming up \xE2\x86\x92 Running hot " + Dot + " +18");
	Is(c::HeatMove(2, 2, 6), "Running hot " + Dot + " +6");
	Is(c::HeatMove(2, 2, -4), "Running hot " + Dot + " -4");
	Is(c::TipsTonight(2400, 1100, true), "Tonight: tips $24 " + Dot + " cheers $11");
	Is(c::TipsTonight(2400, 0, false), "Tonight: tips $24");
	Is(c::RenewalsAway(3800), "While you were away: renewals $38");
	Is(c::Tipped("Dee", 500), "Dee tipped $5");
	Is(c::PendingLine(8600), "pending: $86, paid after your next rent");
	Is(c::PendingNote(), "pending: paid after your next rent");
	Is(c::NextGoal(88, 5000), "88 to 5,000 followers");
	Is(c::FirstPayout(13400), "Your first Kast payout: $134");
	Is(c::FirstPayout(13437), "Your first Kast payout: $134.37");
	Is(c::SponsorReadLabel("TunnelRat"), "TunnelRat: stream read");
	Is(c::SponsorHoursLabel("TunnelRat", 6), "TunnelRat: 6 hours on stream");
	Is(c::SponsorHoursLabel("TunnelRat", 1), "TunnelRat: 1 hour on stream");
	Is(c::SponsorReadDone("TunnelRat"), "TunnelRat: read done today");
	Is(c::LedgerRead("TunnelRat", 1), "TunnelRat: stream read");
	Is(c::LedgerRead("TunnelRat", 2), "TunnelRat: 2 stream reads");
	Is(c::LedgerHours("TunnelRat"), "TunnelRat: live hours");
	Is(c::StingFollowers(25), "25 followers!");
	Is(c::StingStrangerTip(), "Your first tip from someone new!");
	Is(c::StingFirstRaid(), "Your first raid!");
	Is(c::StingAffiliate(), "You're a Kast Affiliate!");
	Is(c::StingPartner(), "You're a Kast Partner!");
	Is(c::MilestoneFollowers(5), "Milestone: 5 followers!");
	Is(c::FirstStrangerTip("lucia.mm", 200), "First tip from someone new: lucia.mm sent $2!");
	Is(c::FirstRaidIn("freeroll_fritz", 3), "Your first raid: freeroll_fritz brought 3!");
	Is(c::LetterFrom(), "Kast");
	Is(c::LetterSubject(), "New on Kast");
	Is(c::LetterBody(), "New on Kast: your ping, Heat and the Broadcast. Creator payouts now follow how people actually watch: per visit, not per hour.");
	Is(c::SponsorApart(), "Sponsor pay is its own ledger line, never part of the Kast payout.");
	Is(c::AffiliateBar(25, 240, 4, 3), "25 followers " + Dot + " 240 minutes " + Dot + " 4 days live " + Dot + " an average of 3, over 30 days");
	Is(c::AffiliateFriends(), "Dee and Mei subscribed the night it landed.");
	Is(c::AffiliateToGo(2, 25), "2 to go: Affiliate at 25");
	Is(c::SawYou(41), "41 saw you in the directory");
	// Every accessor's text is filled: no template variable left anywhere.
	const std::vector<std::string> Filled = {c::Watching(3), c::Chatting(2), c::NobodyHere(), c::GoLiveNoPing(), c::FirstFollowerSub(), c::SetTheSceneButton(),
		c::PingTitle(), c::PingPart(0), c::PingPart(1), c::PingPart(2), c::RunAlertStrip(12), c::RailCallButton(), c::MeiRail(), c::HeatTitle(), c::HeatEffects(41),
		c::ColdMeans(), c::WhyUnderForecast(21), c::WhyRecordPeak(41), c::WhyRaid("kat_v", 14), c::WhyMilestone(250), c::WhySteady(), c::WhyNew(),
		c::StaminaTitle(), c::StaminaNote(), c::FinalTableChip(), c::SweatingNowShelf(), c::SweatingNowSub(), c::FreshShelf(), c::FreshShelfSub(), c::RisingShelf(),
		c::RisingShelfSub(), c::CameBecauseOf(), c::FollowedFrom(), c::CameFromClip(), c::WelcomeRaidStreamer(), c::TokenRule(), c::NodButton(), c::DoneButton(),
		c::PartnerAverage(), c::RaidsOwed(0), c::RaidsOwedShort(0), c::RaidsOwedShort(1), c::RaidedAgo("x", 0, "Friday"), c::RaidedAgo("x", 1, "Friday"),
		c::WhyShowingUp(1), c::WhyCooling(1), c::RunAlertSent(1), c::PingToast(1, 1), c::EmptySeen(1, 0), c::BroadcastTease(1), c::BroadcastTease(2), c::BroadcastTease(3),
		c::ClipOfTheNight("x", false), c::WhyShort(), c::WhyShortCounted(), c::WhyShortUsual(), c::WhyShortBefore(), c::WhyCeiling(),
		c::WhyRunPlain(ss::kast::growth::RunStage::FinalTable, 0, ""), c::WhyRunPlain(ss::kast::growth::RunStage::Win, 4, "Friday"), c::PeakLine(5, ss::kast::PeakRaid, "1:00 AM")};
	bool AllFilled = true;
	for (const std::string& S : Filled)
	{
		AllFilled = AllFilled && !S.empty() && S.find('{') == std::string::npos && S.find('}') == std::string::npos;
	}
	for (const auto* Pool : {&c::FirstFollowerChat(), &c::TierUpChat(), &c::WelcomedChat(), &c::BroadcastChat(1), &c::BroadcastChat(2)})
	{
		AllFilled = AllFilled && !Pool->empty();
	}
	Expect(AllFilled, "kast growth: every accessor returns filled copy");
}
} // namespace pure_seg

void SegPure()
{
	const auto T0 = std::chrono::steady_clock::now();
	pure_seg::Draws();
	pure_seg::PingAndPresence();
	pure_seg::DeepRun();
	pure_seg::Heat();
	pure_seg::Discovery();
	pure_seg::Money();
	pure_seg::Rings();
	pure_seg::ForecastAndRank();
	pure_seg::Saves();
	pure_seg::Seams();
	pure_seg::Copy();
	std::printf("kast growth: Pure, %d checks so far, %d failed, %.2fs\n", Checks, Failures, std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

// ------------------------------------------------------------------ SegGlue: the Session glue (SessionKast.cpp)
//
// core-kastglue's checks of Session's Kast hooks, moved here from night_one_session (session_seg_econ.cpp) by the C-K gate's
// L14 (SPEC_CREATORS §9.3: no new checks there). They run on a real ss::Session as they did, with the same kit: the suite's
// Hooks, Wait and the committed fixtures (SS_TESTS_DIR), so their coverage is the same (each prints the same lines):
//   EconKastInputs    KastInputsExtra: the world seed, the rents paid, the tables (index; this one and the front one), the
//                     deepest armed run with its facts, the event's Series, the all-in equity, Attended with two tables, the
//                     series window, the live night, Rerun's Heat since the last go-live, each deal's paid reads and reads ask.
//   EconKastMigration KastBeforeStart and KastAfterStart: an old channel (update2_creator) seeded at its first stream under the
//                     new model (§8.3) and the one Kast letter (C5, Q-K3); a new career's channel (KastNewCareer) and one that
//                     never streamed get no letter and Heat as of now.
//   EconKastStop      KastAfterStop: tonight's first-time milestones as one KastShine through the cue queue, none on a night
//                     without one; the first payout's text when the payout releases what waited for the second rent (C14);
//                     C13's sponsor pay on its own ledger lines.
//   EconKastConsole   the verbs off air, KastForecast and KastRank, and ss.Kast.* (Describe, Grant, Heat, Rent, Advance on
//                     copies, the Room's verbs refused off air, the usage).
//   EconKastBehind    a table behind taking its turn (Session::Update's loop) is this table and never the front one.
//   EconKastRail      Rail Call's once-a-world-day grant is saved the moment it's used.
//   EconKastDaily     C14 offline: renewals held before the second rent under the new model, kept in the balance before it.
// And the C-K gate's wave-2 fixes (fix-mathglue):
//   GlueForecastPreview  L8: before an old save's first stream under this build, Studio's forecast reads the channel as that
//                     go-live will seed it (a channel 9 days dark: Heat 15, Cold, as it goes live), and nothing is touched.
//   GlueSeedAgrees    L8 (review): the preview's Heat on the numbers alone is KastMigrate's at the 7-day edge, any hour, days
//                     kept or the log; no preview for a channel under the new model or never live; a new career's go-live
//                     forecast is the one Studio showed.
//   GlueWhyOffline    L10: Heat's why lines off air (kast::copy::WhyOffline) at their edges, and ss.Kast.Describe prints them.
//   GlueEndStings     M3 (fix-room's live post): the end card stings only a first the Room didn't post live (Summary::Firsts);
//                     end to end, a career's first raid in through a real Session sounds once (review).
namespace glue_seg
{
namespace kast = ss::kast;

/** night_one_session's SessionHooks (tests/SessionSeg.h's Hooks, as the moved checks read it). */
struct Hooks : ss::SessionHooks
{
	std::map<ss::SoundId, int> Sounds;
	int Texts = 0;
	int Saves = 0;
	ss::SaveData Last;
	std::vector<std::pair<std::string, std::string>> Inbox;
	void Sound(ss::SoundId Id, double) override { ++Sounds[Id]; }
	void Text(const std::string& From, const std::string& Body) override
	{
		++Texts;
		Inbox.emplace_back(From, Body);
	}
	/** A text from From with Part in it, since the inbox held Since. */
	bool Got(size_t Since, const char* From, const char* Part) const
	{
		for (size_t I = Since; I < Inbox.size(); ++I)
		{
			if (Inbox[I].first == From && Inbox[I].second.find(Part) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
};

/** Runs the session for some real seconds at 30 frames a second (night_one_session's Wait). */
double Wait(ss::Session& S, double Now, double Seconds)
{
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}

namespace seams
{
/** A committed fixture's text, line endings as written (night_one_session's seams::Fixture). */
std::string Fixture(const char* Name)
{
	std::string Out;
	FILE* F = std::fopen((std::string(SS_TESTS_DIR) + "/fixtures/" + Name).c_str(), "rb");
	if (!F)
	{
		return Out;
	}
	char Buf[65536];
	size_t N = 0;
	while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
	{
		Out.append(Buf, N);
	}
	std::fclose(F);
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}
} // namespace seams

constexpr double EconDay = 1440.0;

double EconSecondsSince(const std::chrono::steady_clock::time_point& T0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
}

std::string EconLower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return S;
}

bool EconHas(const std::string& Line, const std::string& Part)
{
	return EconLower(Line).find(EconLower(Part)) != std::string::npos;
}

bool EconNear(double A, double B, double Tolerance = 1e-9)
{
	return std::fabs(A - B) <= Tolerance * std::max(1.0, std::fabs(B));
}

ss::autoplay::TableAuto EconPlan(ss::autoplay::Mode M, int Speed)
{
	ss::autoplay::TableAuto A;
	A.S = ss::style::Style::Gto;
	A.M = M;
	A.Speed = Speed;
	A.To = ss::autoplay::Target::Result;
	A.Stops = 0;
	return A;
}

ss::hero::Character EconWho()
{
	ss::hero::Character Who;
	Who.FirstName = "Jesse";
	Who.LastName = "Cole";
	Who.Created = true;
	return Who;
}

/** A session with a rig that streams, at a table. */
bool EconAtTable(ss::Session& S)
{
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	S.Register(1);
	return Rig && S.T && S.StreamInputs().Gear.CanStream();
}

/** A listing a session registers for (session_seams' SitesListing shape): Template a schedule template's id ("" none), at
 *  Night One's first listing's start. */
ss::LobbyEvent EconListing(const std::string& Template, const std::string& Name, ss::Chips BuyIn, int Entrants)
{
	ss::LobbyEvent L = ss::Lobby()[1];
	const long long Start = static_cast<long long>(std::llround(ss::net::MinutesPerDay * static_cast<double>(ss::net::NightOneDay) + L.Spec.StartClock));
	L.Spec.Id = (Template.empty() ? std::string("econ-glue-") + Name : Template) + "@" + std::to_string(Start);
	L.Spec.Name = Name;
	L.Name = Name;
	L.Spec.BuyInCents = BuyIn;
	L.BuyInCents = BuyIn;
	L.Spec.FeeCents = BuyIn / 10;
	L.Spec.GuaranteeCents = 0;
	L.Spec.BountyCents = 0;
	L.Spec.MysteryBounty = false;
	L.Spec.SeatValueCents = 0;
	L.Spec.SeatTicket.clear();
	L.Spec.Entrants = Entrants;
	L.Joinable = true;
	return L;
}

/** §2.6's Series of an event by its id, as KastInputsExtra reads it (a small field: only the id matters). */
double EconSeries(const std::string& Id)
{
	ss::TournamentSpec Spec = ss::Lobby()[1].Spec;
	Spec.Id = Id;
	Spec.Entrants = 18;
	const ss::Tournament Tn(Spec, "grinder_3c", "econ-series:" + Id);
	return ss::session_kast_detail::KastSeriesOf(Tn);
}

/** A running deal the stream reads for (Overclock's terms: $3 a live hour, $10 a read, eight reads asked a period). */
ss::sponsor::Contract EconDeal(double World, int ReadsPaid)
{
	ss::sponsor::Contract C;
	C.Id = "econ-glue-overclock";
	C.Brand = "overclock";
	C.St = ss::sponsor::State::Active;
	C.Since = World - EconDay;
	C.Until = World + 20.0 * EconDay;
	C.T.PerReadCents = 1000;
	C.T.PerLiveHourCents = 300;
	C.T.Asks[static_cast<size_t>(ss::sponsor::Ask::Reads)] = 8.0;
	C.Now.Reads = ReadsPaid;
	return C;
}

/** The letters Kast has sent (the one-time letter's Source). */
int EconKastLetters(const ss::life::State& L)
{
	int N = 0;
	for (const ss::life::Letter& M : L.Inbox)
	{
		N += M.Source == "kast" ? 1 : 0;
	}
	return N;
}

void EconKastInputs()
{
	Hooks H;
	ss::Session S(H, "econ-kast-inputs");
	const bool Ready = EconAtTable(S);
	const double World = S.WorldMinutes();
	const int Today = ss::net::DayOf(World);
	kast::Inputs In = S.StreamInputs();
	std::printf("  kast glue: one table: seed %llu, rents paid %d, this table %d, front %d, deep run %d, equity %.2f, attended %d, series \"%s\", Rerun's Heat %zu\n",
		static_cast<unsigned long long>(In.Seed), In.RentsPaid, In.ThisTable, In.FrontTable, In.DeepTable, In.AllInEquity, In.Attended ? 1 : 0, In.SeriesWindow.c_str(),
		In.RerunHeat.size());
	Expect(Ready && In.Seed == S.WorldSeed() && In.Seed != 0 && In.RentsPaid == S.Life.RentsPaid, "kast glue: the stream gets the world seed and the rents paid");
	Expect(In.ThisTable == 0 && In.FrontTable == 0 && S.FocusedTable() == 0, "kast glue: one table is this table and the front one (index 0)");
	Expect(In.AllInEquity == -1.0 && In.Attended && !In.LiveNightRecent, "kast glue: no all-in, no equity; Smart is attended; no live night");
	const ss::net::SeriesInfo* Ser = ss::net::Shared().CurrentSeries(World);
	const bool Inside = Ser && Today >= Ser->FirstDay && Today <= Ser->LastDay;
	Expect(In.SeriesWindow == (Inside ? (Ser->Short.empty() ? Ser->Name : Ser->Short) : std::string()), "kast glue: the series window is the calendar's series, only inside its days");
	Expect(In.RerunHeat == ss::video::algo::KastHeatSince(S.Life.Video, -1.0), "kast glue: Rerun's Heat bumps since the last go-live (a channel never live: all of them)");
	// Between two series the calendar names the next one (CurrentSeries returns it before its first day): no window yet. The
	// first day after the running series ends that no series covers, if the calendar has one.
	if (Inside)
	{
		const double LobbyWas = S.LobbyMinutes;
		const double Gap = static_cast<double>(Ser->LastDay + 1) * EconDay + 600.0;
		const ss::net::SeriesInfo* Next = ss::net::Shared().CurrentSeries(Gap);
		const bool Between = Next && ss::net::DayOf(Gap) < Next->FirstDay;
		S.LobbyMinutes = Gap - ss::net::MinutesPerDay * static_cast<double>(ss::net::NightOneDay);
		const std::string Window = S.StreamInputs().SeriesWindow;
		S.LobbyMinutes = LobbyWas;
		std::printf("  kast glue: the day after %s ends: the calendar names %s (%s); the series window \"%s\"\n", Ser->Short.c_str(), Next ? Next->Name.c_str() : "nothing",
			Between ? "not started" : "running", Window.c_str());
		Expect(!Between || Window.empty(), "kast glue: between two series, no series window (the next one hasn't started)");
	}

	// The all-in's equity is the hero's own seat's, as UpdateEquities cached it at the reveal (every seat in the pot has one).
	{
		const std::vector<ss::SeatVis> SeatsWas = S.Seats;
		int HeroSeat = -1;
		int Others = 0;
		for (size_t K = 0; K < S.Seats.size(); ++K)
		{
			ss::SeatVis& Sv = S.Seats[K];
			Sv.HasEquity = Sv.Present;
			Sv.Equity = Sv.IsHero ? 0.64 : 0.36;
			HeroSeat = Sv.Present && Sv.IsHero ? static_cast<int>(K) : HeroSeat;
			Others += Sv.Present && !Sv.IsHero ? 1 : 0;
		}
		S.HeroAllInReveal = true;
		const double Equity = S.StreamInputs().AllInEquity;
		// (Before UpdateEquities has run for the hero's seat there is none yet: a seat's stale number is no equity.)
		if (HeroSeat >= 0)
		{
			S.Seats[static_cast<size_t>(HeroSeat)].HasEquity = false;
		}
		const double NotYet = S.StreamInputs().AllInEquity;
		S.HeroAllInReveal = false;
		const double After = S.StreamInputs().AllInEquity;
		S.Seats = SeatsWas;
		Expect(HeroSeat >= 0 && Others > 0 && Equity == 0.64 && NotYet == -1.0 && After == -1.0,
			"kast glue: the all-in's equity is the hero's own seat's (not another seat's in the pot), none before the seat has one, and none after the reveal");
	}

	// Rerun's Heat bumps reach one go-live. After a reload a short stream left no log row (and a restart within 4 world hours no
	// ping), so its day is the floor: Rerun's bumps land on world midnights, and that day's go-live took them. A logged stream
	// later that day is later still (the bumps here sit off midnight only to tell the two apart).
	{
		const kast::Channel ChWas = S.Channel;
		const ss::video::Channel VidWas = S.Life.Video;
		const double Midnight = static_cast<double>(Today) * EconDay;
		ss::video::Video V;
		V.X.On = true;
		V.At = Midnight - 30.0 * EconDay;
		for (const double At : {Midnight - EconDay, Midnight, Midnight + 30.0, Midnight + 90.0})
		{
			ss::video::Comp K;
			K.T = At;
			K.P = 1.0;
			K.Kind = ss::video::CompKind::Breakout;
			V.X.Comps.push_back(K);
		}
		S.Life.Video.X.On = true;
		S.Life.Video.Videos.push_back(V);
		S.Life.Video.SteppedTo = std::max(S.Life.Video.SteppedTo, Midnight + 120.0);
		S.Channel.PingAt = Midnight - 120.0;
		S.Channel.LiveSince = -1.0;
		S.Channel.LastStreamDay = Today;
		const std::vector<std::pair<double, double>> Reloaded = S.StreamInputs().RerunHeat;
		kast::StreamLog Lg;
		Lg.Start = Midnight + 60.0;
		Lg.Minutes = 90.0;
		S.Channel.Log.insert(S.Channel.Log.begin(), Lg);
		const std::vector<std::pair<double, double>> Logged = S.StreamInputs().RerunHeat;
		const std::vector<std::pair<double, double>> SincePing = ss::video::algo::KastHeatSince(S.Life.Video, Midnight - 120.0);
		const std::vector<std::pair<double, double>> SinceDay = ss::video::algo::KastHeatSince(S.Life.Video, Midnight);
		const std::vector<std::pair<double, double>> SinceLog = ss::video::algo::KastHeatSince(S.Life.Video, Midnight + 60.0);
		S.Channel = ChWas;
		S.Life.Video = VidWas;
		std::printf("  kast glue: Rerun's Heat after a reload: %zu bump(s) handed after a short stream today (%zu since the ping), %zu after a logged one\n", Reloaded.size(),
			SincePing.size(), Logged.size());
		Expect(SincePing.size() == 3 && SinceDay.size() == 2 && SinceLog.size() == 1 && Reloaded == SinceDay && Logged == SinceLog,
			"kast glue: Rerun's Heat bumps reach one go-live: after a reload, none from before the last live day (its go-live took them) or the last logged go-live");
	}

	// The live night: an Embercrest night that cashed in the last 48 world hours, and nothing else.
	ss::life::LiveEntry E;
	E.Id = "embercrest-nightly@" + std::to_string(Today - 1);
	E.Name = "Embercrest Nightly $120";
	E.State = ss::life::LiveEntry::Finished;
	E.PrizeCents = 24000;
	E.FinishedAt = World - 600.0;
	S.Life.LiveEntries.push_back(E);
	const bool Cashed = S.StreamInputs().LiveNightRecent;
	S.Life.LiveEntries.back().FinishedAt = World - 47.0 * 60.0;
	const bool Cashed47 = S.StreamInputs().LiveNightRecent;
	S.Life.LiveEntries.back().FinishedAt = World - 49.0 * 60.0;
	const bool Stale49 = S.StreamInputs().LiveNightRecent;
	S.Life.LiveEntries.back().FinishedAt = World - 3.0 * EconDay;
	const bool Stale = S.StreamInputs().LiveNightRecent;
	S.Life.LiveEntries.back().FinishedAt = World - 600.0;
	S.Life.LiveEntries.back().PrizeCents = 0;
	const bool Busted = S.StreamInputs().LiveNightRecent;
	S.Life.LiveEntries.back().PrizeCents = 24000;
	S.Life.LiveEntries.back().Id = "deebackroom@" + std::to_string(Today);
	const bool Elsewhere = S.StreamInputs().LiveNightRecent;
	S.Life.LiveEntries.pop_back();
	Expect(Cashed && Cashed47 && !Stale49 && !Stale && !Busted && !Elsewhere,
		"kast glue: Dee's line: an Embercrest night that cashed in the last 48 hours (47 hours on yes, 49 or three days on not, not without a prize, not elsewhere)");

	// C13's cap: each deal's paid reads this period and its reads ask, from the book: the running contract's, not an older one
	// with the same brand that ended (a re-signed deal: the book keeps both).
	ss::sponsor::Contract Old = EconDeal(World - 60.0 * EconDay, 11);
	Old.Id = "econ-glue-overclock-old";
	Old.St = ss::sponsor::State::Ended;
	Old.Until = World - 30.0 * EconDay;
	Old.T.Asks[static_cast<size_t>(ss::sponsor::Ask::Reads)] = 4.0;
	S.Life.Sponsors.Contracts.push_back(Old);
	S.Life.Sponsors.Contracts.push_back(EconDeal(World, 3));
	In = S.StreamInputs();
	const kast::Deal* Dl = nullptr;
	for (const kast::Deal& D : In.Deals)
	{
		Dl = D.Id == "overclock" ? &D : Dl;
	}
	Expect(Dl && Dl->ReadsPaid == 3 && Dl->ReadsAsk == 8 && Dl->ReadCents == 1000,
		"kast glue: a deal's paid reads this period (3) and its reads ask (8) ride the stream's deal (the running contract's, not an ended one's of the same brand)");

	// §2.6's Series, by the event's template on its own site.
	const double Major = EconSeries("sunday-showdown@" + std::to_string(6 * 1440 + 900));
	const double Plain = EconSeries("bigstack-11@" + std::to_string(1440 + 1200));
	const double Mega = EconSeries("mega-rcop@" + std::to_string(5 * 1440 + 1080));
	const double Rival = EconSeries("runittwice.sunday-100k@" + std::to_string(6 * 1440 + 1140));
	const double None = EconSeries("econ-glue-nothing@100");
	double Main = -1.0;
	double Event = -1.0;
	for (const ss::net::EventTemplate& Tm : ss::net::Shared().Templates())
	{
		if (!Tm.Series.empty() && Main < 0.0 && Tm.Main)
		{
			Main = EconSeries(Tm.Id + "@100");
		}
		if (!Tm.Series.empty() && Event < 0.0 && !Tm.Main)
		{
			Event = EconSeries(Tm.Id + "@100");
		}
	}
	std::printf("  kast glue: Series: a Sunday major %.1f, the Big Stack %.1f, a mega satellite %.1f, RunItTwice's Sunday 100K %.1f, a series main %.1f, a series event %.1f, "
				"no template %.1f\n",
		Major, Plain, Mega, Rival, Main, Event, None);
	Expect(Major == 1.0 && Rival == 1.0 && Main == 1.0 && Event == 0.5 && Plain == 0.0 && Mega == 0.0 && None == 0.0,
		"kast glue: Series 1 a weekly major (any site's Sunday event) or a series main, 0.5 another series event, 0 the rest");

	// Two tables: the deeper armed run is the deep one (once kast::growth's maths is in), and one table watched at 4x makes the
	// night heads-down whichever is in front.
	Hooks H2;
	ss::Session Two(H2, "econ-kast-two-tables");
	Two.CurrentScreen = ss::Screen::Lobby;
	Two.BankrollCents = 500000;
	const bool Rig = Two.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	const std::string R1 = Two.RegisterEvent(EconListing("bigstack-11", "$11 Big Stack", 1100, 980));
	const std::string R2 = Two.RegisterEvent(EconListing("", "$3.30 Glue Turbo", 330, 300));
	if (!Rig || !R1.empty() || !R2.empty() || Two.TableCount() != 2)
	{
		std::printf("  kast glue: two tables didn't open (%s / %s, %d tables)\n", R1.c_str(), R2.c_str(), Two.TableCount());
		Expect(false, "kast glue: two tables open for the two-table checks");
		return;
	}
	const int Front = Two.FocusedTable();
	const int Behind = 1 - Front;
	const kast::Inputs Both = Two.StreamInputs();
	Two.SetAutoplay(Behind, EconPlan(ss::autoplay::Mode::Watch, 4));
	const bool BehindFast = Two.StreamInputs().Attended;
	Two.SetAutoplay(Behind, EconPlan(ss::autoplay::Mode::Off, 1));
	Two.SetAutoplay(Front, EconPlan(ss::autoplay::Mode::Watch, 2));
	const bool FrontTwo = Two.StreamInputs().Attended;
	Two.SetAutoplay(Front, EconPlan(ss::autoplay::Mode::Off, 1));
	Expect(Both.ThisTable == Front && Both.FrontTable == Front && Both.Attended && !BehindFast && FrontTwo,
		"kast glue: two tables: the front one is this table; the table behind watched at 4x is a heads-down night, the front one at 2x isn't");
	// Both fields whole (depth 0 each: prestige x depth ties at 0): the first table in index order is the deep run, as on any tie.
	for (int K = 0; K < Two.TableCount(); ++K)
	{
		Two.WithTable(K, [&Two]() { Two.T->Remaining = Two.T->Spec.Entrants; });
	}
	const kast::Inputs Tied = Two.StreamInputs();
	Expect(!kast::growth::Armed(300) || (Tied.DeepTable == 0 && Tied.Depth == 0.0), "kast glue: two armed runs tied at depth 0: the first table is the deep run");
	// The deep run: the 300-runner table down to 20 left, the 980 just started.
	int Small = -1;
	for (int K = 0; K < Two.TableCount(); ++K)
	{
		Small = Two.Glance(K).EventId.rfind("econ-glue-", 0) == 0 ? K : Small;
	}
	Two.WithTable(Small, [&Two]() { Two.T->Remaining = 20; });
	const kast::Inputs Deep = Two.StreamInputs();
	if (kast::growth::Armed(300))
	{
		const ss::Tournament* Tn = nullptr;
		Two.WithTable(Small, [&Two, &Tn]() { Tn = Two.T.get(); });
		const double D = kast::growth::Depth(300, 20);
		const double P = kast::growth::Prestige(300, 330, 0.0);
		Expect(Tn && Deep.DeepTable == Small && Deep.DeepEntrants == 300 && Deep.DeepRemaining == 20 && Deep.DeepEvent == "$3.30 Glue Turbo" && Deep.DeepTournament == Tn->Spec.Id &&
				   Deep.DeepTableSize == Tn->Spec.TableSize && Deep.DeepPaid == Tn->PaidPlaces() && Deep.BuyInCents == 330 && Deep.Series == 0.0 && Deep.Depth == D && Deep.Prestige == P &&
				   Deep.DeepSite == static_cast<int>(ss::net::SiteId::RiverLine),
			"kast glue: the deepest armed run is the table with the largest prestige x depth, with its event's facts");
	}
	else
	{
		Expect(Deep.DeepTable == -1 && Deep.Depth == 0.0 && Deep.DeepEntrants == 0, "kast glue: no deep run before kast::growth's maths is in (nothing is armed)");
	}
	std::printf("  kast glue: two tables: front %d, the deep run's table %d (%d of %d left, depth %.3f, prestige %.3f)\n", Front, Deep.DeepTable, Deep.DeepRemaining, Deep.DeepEntrants,
		Deep.Depth, Deep.Prestige);
	// A table whose hero is out is no run of the hero's: the deep run is the other armed table, however deep the field there.
	bool HeroFirst = false;
	Two.WithTable(Small, [&Two, &HeroFirst]() {
		HeroFirst = !Two.T->Players.empty() && &Two.T->Players[0] == &Two.T->Hero();
		if (HeroFirst)
		{
			Two.T->Players[0].Busted = true;
		}
	});
	const kast::Inputs Out = Two.StreamInputs();
	Two.WithTable(Small, [&Two, HeroFirst]() {
		if (HeroFirst)
		{
			Two.T->Players[0].Busted = false;
		}
	});
	Expect(HeroFirst && (!kast::growth::Armed(980) || (Out.DeepTable == 1 - Small && Out.DeepEntrants == 980 && Out.DeepEvent == "$11 Big Stack")),
		"kast glue: a table the hero is out of is no deep run: the other armed table is");
}

void EconKastMigration()
{
	// An old channel (update2_creator: 304 members, 60 log rows) at its first stream under the new model (§8.3).
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(seams::Fixture("update2_creator.ssave"), D);
	Hooks H;
	ss::Session S(H, "econ-kast-migrate", &D);
	S.CurrentScreen = ss::Screen::Lobby;
	if (!S.StreamInputs().Gear.CanStream())
	{
		S.BankrollCents += 300000;
		S.Buy(ss::gear::FirstPcUpgrade().Id);
	}
	const bool Unset = (S.Life.Update3LinesRead & ss::life::U3Kast) == 0;
	const kast::Channel Was = S.Channel;
	const int Letters0 = EconKastLetters(S.Life);
	const double World = S.WorldMinutes();
	const std::string Live = S.GoLive();
	int LastDay = -1;
	for (const int Dy : Was.DaysLive)
	{
		LastDay = std::max(LastDay, Dy);
	}
	if (!Was.Log.empty())
	{
		LastDay = std::max(LastDay, static_cast<int>(std::floor(Was.Log.front().Start / EconDay)));
	}
	const bool Recent = LastDay >= 0 && ss::net::DayOf(World) - LastDay <= 7;
	int Best = 0;
	for (const kast::StreamLog& Lg : Was.Log)
	{
		Best = std::max(Best, Lg.Avg);
	}
	const kast::Channel& Ch = S.Channel;
	const double HeatNow = kast::growth::HeatAt(Ch.Heat, Ch.HeatAt, World);
	std::printf("  kast glue: update2_creator (%d streams, %zu log rows, last live day %d, today %d) migrated at its go-live (\"%s\"): Heat %.1f (as of %.0f), Norm %.2f "
				"(the last ten %.2f), Bias %.2f, RunNorm %.2f, record average %d, welcome raid %s; %d Kast letter(s)\n",
		Was.Streams, Was.Log.size(), LastDay, ss::net::DayOf(World), Live.c_str(), HeatNow, Ch.HeatAt, Ch.Norm, Was.AvgViewers(), Ch.Bias, Ch.RunNorm, Ch.BestAvg,
		Ch.WelcomeRaidDone ? "done" : "to come", EconKastLetters(S.Life) - Letters0);
	Expect(Parsed && Unset && Live.empty() && S.Streaming() && (S.Life.Update3LinesRead & ss::life::U3Kast) != 0, "kast glue: an old channel's first stream under the new model sets U3Kast");
	// (Start's own Offline may hold money that came in while it was away, before the second rent: C14, the Room's.)
	const bool Seeded[] = {EconNear(HeatNow, Recent ? 26.0 : 15.0, 1e-9), EconNear(Ch.Norm, Was.AvgViewers(), 1e-12), Ch.Bias == 0.0, Ch.RunNorm == 0.15, Ch.BestAvg == Best,
		Ch.PendingCents == 0 || S.Life.RentsPaid < 2, Ch.WelcomeRaidDone == (Was.Streams >= 3), Ch.LastStreamDay >= LastDay};
	std::string SeededText;
	for (const bool B : Seeded)
	{
		SeededText += B ? "1" : "0";
	}
	std::printf("  kast glue: the seeding (Heat, Norm, Bias, RunNorm, record, pending, welcome raid, last day): %s\n", SeededText.c_str());
	Expect(SeededText.find('0') == std::string::npos, "kast glue: seeded from its log (§8.3): Heat 26 or 15 as of now, Norm the last ten, Bias 0, RunNorm 0.15, the record average, nothing pending");
	Expect(Ch.Followers >= Was.Followers && Ch.Members.size() >= Was.Members.size() && Ch.Subs.size() >= Was.Subs.size(), "kast glue: it keeps every follower, member and sub");
	const ss::life::Letter* Letter = nullptr;
	for (const ss::life::Letter& M : S.Life.Inbox)
	{
		Letter = M.Source == "kast" ? &M : Letter;
	}
	Expect(EconKastLetters(S.Life) == Letters0 + 1 && Letter && Letter->From == kast::copy::LetterFrom() && Letter->Subject == kast::copy::LetterSubject() &&
			   Letter->Body == kast::copy::LetterBody() && !Letter->Read,
		"kast glue: the one-time Kast letter, in the Mail app (C5, Q-K3)");
	S.EndStream();
	// (The model's own numbers from here on: the next go-live seeds nothing again.)
	S.Channel.Norm = 7.5;
	S.Channel.Bias = 0.25;
	S.Channel.RunNorm = 0.4;
	S.Channel.BestAvg = Best + 3;
	const bool Again = S.GoLive().empty();
	const bool Kept = S.Channel.Norm == 7.5 && S.Channel.Bias == 0.25 && S.Channel.RunNorm == 0.4 && S.Channel.BestAvg == Best + 3;
	S.EndStream();
	Expect(Again && EconKastLetters(S.Life) == Letters0 + 1, "kast glue: the letter comes once: the next stream sends none");
	Expect(Kept, "kast glue: the migration runs once: the next go-live keeps the channel's own Norm, Bias, RunNorm and record");

	// A new career is the new model's from the start: U3Kast, Heat as of now; its first stream is no migration and gets no letter.
	Hooks H2;
	ss::Session N(H2, "econ-kast-new");
	N.NewCareer(EconWho());
	const bool NewSet = (N.Life.Update3LinesRead & ss::life::U3Kast) != 0 && N.Channel.HeatAt == N.WorldMinutes() && N.Channel.Heat == 26.0;
	N.CurrentScreen = ss::Screen::Lobby;
	N.BankrollCents += 300000;
	const bool NewRig = N.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	// (Its first stream ten world days on, the rig bought late: a new channel still starts the night at 26, as of the go-live.)
	N.LobbyMinutes += 10.0 * EconDay;
	const double NewAt = N.WorldMinutes();
	const bool NewLive = N.GoLive().empty();
	std::printf("  kast glue: a new career's first stream ten days in: Heat %.2f as of %.0f (the go-live %.0f), %.2f raw now\n", N.Channel.Heat, N.Channel.HeatAt, NewAt,
		kast::growth::HeatAt(N.Channel.Heat, N.Channel.HeatAt, NewAt));
	Expect(NewSet && NewRig && NewLive && EconKastLetters(N.Life) == 0 && N.Channel.Heat == 26.0 && N.Channel.HeatAt == NewAt,
		"kast glue: a new career: U3Kast and Heat as of now, its first stream (days later) starts from 26 as of its go-live, and gets no letter");
	// A channel that never streamed (a session made without NewCareer) is new too: Heat as of its go-live, no letter.
	Hooks H3;
	ss::Session F(H3, "econ-kast-fresh");
	const bool FreshReady = EconAtTable(F);
	const bool FreshUnset = (F.Life.Update3LinesRead & ss::life::U3Kast) == 0;
	const double At = F.WorldMinutes();
	const bool FreshLive = FreshReady && F.GoLive().empty();
	Expect(FreshUnset && FreshLive && (F.Life.Update3LinesRead & ss::life::U3Kast) != 0 && F.Channel.HeatAt == At && EconNear(kast::growth::HeatAt(F.Channel.Heat, F.Channel.HeatAt, At), 26.0) &&
			   EconKastLetters(F.Life) == 0,
		"kast glue: a channel that never streamed is new, not migrated: Heat 26 as of its go-live, no letter");

	// "Streamed in the last 7 world days" by world day, as Heat's floor reads it: a week dark is still warm (26), a day more is not
	// (15); the last day live from the days kept or the log, whichever is later, seeded with it.
	auto MigratedAt = [](int DaysDark, bool ByLog, int& LastDay) {
		const double At = 300.0 * EconDay + 600.0;
		kast::Channel Ch;
		Ch.Streams = 5;
		const int Last = ss::net::DayOf(At) - DaysDark;
		if (ByLog)
		{
			kast::StreamLog Lg;
			Lg.Start = static_cast<double>(Last) * EconDay + 1200.0;
			Lg.Minutes = 90.0;
			Lg.Avg = 4;
			Ch.Log.push_back(Lg);
			Ch.DaysLive = {Last - 9};
		}
		else
		{
			Ch.DaysLive = {Last - 3, Last};
		}
		ss::session_kast_detail::KastMigrate(Ch, At);
		LastDay = Ch.LastStreamDay == Last && Ch.HeatAt == At ? Last : -1;
		return Ch.Heat;
	};
	int L7 = -1;
	int L8 = -1;
	int G7 = -1;
	int G8 = -1;
	int L0 = -1;
	const double Heats[] = {MigratedAt(7, false, L7), MigratedAt(8, false, L8), MigratedAt(7, true, G7), MigratedAt(8, true, G8), MigratedAt(0, false, L0)};
	std::printf("  kast glue: migrated Heat, 7 / 8 days dark (days kept) %.0f / %.0f, (the log) %.0f / %.0f, streamed today %.0f\n", Heats[0], Heats[1], Heats[2], Heats[3], Heats[4]);
	Expect(Heats[0] == 26.0 && Heats[1] == 15.0 && Heats[2] == 26.0 && Heats[3] == 15.0 && Heats[4] == 26.0 && L7 >= 0 && L8 >= 0 && G7 >= 0 && G8 >= 0 && L0 >= 0,
		"kast glue: a migrated channel a week dark is warm (Heat 26), eight days dark it's 15, as Heat's floor reads the days; its last day live seeded");
	// The rest of §8.3's seeding at its edges: the welcome raid done from stream 3 (not at 2); the first stranger's tip already
	// behind a channel that took tips beyond Dee's first-night $5 (a sting never claims a first that wasn't), not one that took
	// only hers; nothing pending.
	auto SeededAs = [](int Streams, ss::Chips Tips) {
		const double At = 300.0 * EconDay + 600.0;
		kast::Channel Ch;
		Ch.Streams = Streams;
		Ch.EarnedTips = Tips;
		Ch.PendingCents = 4200;
		Ch.DaysLive = {ss::net::DayOf(At) - 1};
		ss::session_kast_detail::KastMigrate(Ch, At);
		return Ch;
	};
	const kast::Channel Second = SeededAs(2, 500);
	const kast::Channel Third = SeededAs(3, 501);
	const bool Firsts[] = {(Second.Firsts & kast::growth::FirstStrangerTip) == 0, (Third.Firsts & kast::growth::FirstStrangerTip) != 0};
	std::printf("  kast glue: migrated at stream 2 / 3: welcome raid %s / %s, the first stranger's tip %s / %s, pending %s / %s\n", Second.WelcomeRaidDone ? "done" : "to come",
		Third.WelcomeRaidDone ? "done" : "to come", Firsts[0] ? "still to come" : "BEHIND", Firsts[1] ? "behind" : "STILL TO COME", ss::Money(Second.PendingCents).c_str(),
		ss::Money(Third.PendingCents).c_str());
	Expect(!Second.WelcomeRaidDone && Third.WelcomeRaidDone && Firsts[0] && Firsts[1] && Second.PendingCents == 0 && Third.PendingCents == 0,
		"kast glue: migrated: the welcome raid done from stream 3, the first stranger's tip behind a channel with tips beyond Dee's $5, nothing pending");
}

void EconKastStop()
{
	Hooks H;
	ss::Session S(H, "econ-kast-stop");
	const bool Ready = EconAtTable(S);
	const bool Live = Ready && S.GoLive().empty();
	const int Followers0 = S.Channel.Followers;
	// Tonight: 30 followers (5, 10 and 25 passed; the new ones are lurkers, so the Room's community is as it started), Affiliate,
	// the first raid in and the first stranger's tip.
	S.Channel.Followers += 28;
	S.Channel.Affiliate = true;
	S.Channel.RaidsIn += 1;
	S.Channel.Firsts |= kast::growth::FirstStrangerTip;
	const std::string Grant = std::to_string(S.Channel.Followers) + " followers, Affiliate, a raid in, a stranger's tip";
	const int Shine0 = H.Sounds[ss::SoundId::KastShine];
	S.EndStream();
	double Now = Wait(S, 0.0, 3.0);
	const int Shine1 = H.Sounds[ss::SoundId::KastShine];
	const bool Again = S.GoLive().empty();
	S.EndStream();
	Now = Wait(S, Now, 3.0);
	const int Shine2 = H.Sounds[ss::SoundId::KastShine];
	std::printf("  kast glue: %d followers -> %d (%s); KastShine %d -> %d, then %d after a night with nothing new\n", Followers0, S.Channel.Followers, Grant.c_str(), Shine0, Shine1, Shine2);
	Expect(Live && Followers0 < 5 && S.Channel.Followers >= 30 && S.Channel.Affiliate && Shine1 == Shine0 + 1,
		"kast glue: tonight's first-time milestones (5, 10, 25 followers, Affiliate, the first raid in, the first stranger's tip) sting once: KastShine through the cue queue");
	Expect(Again && Shine2 == Shine1, "kast glue: a night without a new milestone plays no sting");
	// The first raid in is a first once a career: a later night's raid stings nothing (nor does a later stranger's tip).
	const bool Third = S.GoLive().empty();
	S.Channel.RaidsIn += 1;
	S.EndStream();
	Now = Wait(S, Now, 3.0);
	const int Shine3 = H.Sounds[ss::SoundId::KastShine];
	Expect(Third && Shine3 == Shine2, "kast glue: a night with another raid in (not the first) plays no sting");

	// C14: money that waited for the second rent pays with the first payout after it, and that payout says so (once).
	auto KastTexts = [&H](size_t Since) {
		int N = 0;
		for (size_t I = Since; I < H.Inbox.size(); ++I)
		{
			N += H.Inbox[I].first == kast::copy::LetterFrom() ? 1 : 0;
		}
		return N;
	};
	S.Channel.PendingCents = 13400;
	S.Life.RentsPaid = 1;
	size_t Texts = H.Inbox.size();
	S.GoLive();
	// (A balance to pay tonight too: what's paid before the second rent is no "first payout" while the pending money waits.)
	S.Channel.UnpaidCents += 500;
	const ss::Chips Unpaid1 = S.Channel.UnpaidCents;
	const kast::growth::Payout Early = kast::growth::PayoutOf(Unpaid1, 13400, 1);
	S.EndStream();
	const bool Held = S.Channel.PendingCents == Early.Pending && KastTexts(Texts) == 0;
	S.Life.RentsPaid = 2;
	Texts = H.Inbox.size();
	const ss::Chips Paid0 = S.Channel.PaidCents;
	S.GoLive();
	// (Tonight's balance rides the same payout: the line's amount is all of it, every digit a fact, not the held part alone.)
	S.Channel.UnpaidCents += 700;
	S.EndStream();
	const ss::Chips Paid = S.Channel.PaidCents - Paid0;
	const bool Released = S.Channel.PendingCents == 0;
	const bool Said = H.Got(Texts, kast::copy::LetterFrom().c_str(), kast::copy::FirstPayout(Paid).c_str());
	std::printf("  kast glue: pending $134.00 before the second rent: %s; after it: %s, paid %s, %d text(s) from Kast\n", Held ? "held, nothing said" : "NOT HELD",
		Released ? "released" : "still waiting (kast::growth's payout isn't in)", ss::Money(Paid).c_str(), KastTexts(Texts));
	Expect(Held, "kast glue: before the second rent the pending money waits, and nothing is said");
	Expect(Released ? (Said && Paid >= 13400 + 700 && KastTexts(Texts) == 1) : KastTexts(Texts) == 0,
		"kast glue: the payout that releases it says so, with all it pays: \"Your first Kast payout\"");

	// C13: sponsor pay on its own ledger lines, once the Room books it apart from the Kast payout.
	Hooks H4;
	ss::Session P(H4, "econ-kast-sponsor");
	const bool PReady = EconAtTable(P);
	P.Life.Sponsors.Contracts.push_back(EconDeal(P.WorldMinutes(), 0));
	const bool PLive = PReady && P.GoLive().empty();
	double PNow = 0.0;
	P.LobbyMinutes += 40.0;
	P.Update(PNow += 0.1);
	const ss::Chips Read = P.StreamRead("overclock");
	P.LobbyMinutes += 30.0;
	P.Update(PNow += 0.1);
	const ss::Chips Bank0 = P.BankrollCents;
	const size_t Ledger0 = P.Life.Ledger.size();
	P.EndStream();
	const kast::Summary& Sum = P.Stream.Last;
	ss::Chips SponsorLines = 0;
	ss::Chips KastPayout = 0;
	int SponsorRows = 0;
	bool ReadLine = false;
	bool HoursLine = false;
	const ss::sponsor::Brand* Br = ss::sponsor::FindBrand("overclock");
	const std::string Short = Br ? Br->Short : std::string("overclock");
	for (size_t I = 0; I + Ledger0 < P.Life.Ledger.size(); ++I)
	{
		const ss::life::LedgerEntry& Le = P.Life.Ledger[I];
		SponsorLines += Le.Kind == ss::life::LedgerSponsors ? Le.Amount : 0;
		SponsorRows += Le.Kind == ss::life::LedgerSponsors ? 1 : 0;
		KastPayout += Le.Kind == 6 ? Le.Amount : 0;
		ReadLine = ReadLine || (Le.Kind == ss::life::LedgerSponsors && Le.Label == kast::copy::LedgerRead(Short, 1) && Le.Amount == 1000);
		HoursLine = HoursLine || (Le.Kind == ss::life::LedgerSponsors && Le.Label == kast::copy::LedgerHours(Short) && Le.Amount > 0);
	}
	// Each deal's pay is booked once: a line for its reads and one for its live hours, exactly what the night's summary paid.
	ss::Chips SummaryPay = 0;
	int SummaryParts = 0;
	for (const kast::Summary::SponsorPay& Sp : Sum.SponsorPaid)
	{
		SummaryPay += Sp.ReadCents + Sp.HourCents;
		SummaryParts += (Sp.ReadCents > 0 ? 1 : 0) + (Sp.HourCents > 0 ? 1 : 0);
	}
	Expect(SponsorLines == SummaryPay && SponsorRows == SummaryParts, "kast glue: the night's sponsor lines are the summary's sponsor pay, each part booked once");
	std::printf("  kast glue: a stream with a deal: read %s, tonight's sponsor pay %zu deal(s); ledger: sponsors %s, Kast payout %s; bankroll +%s\n", ss::Money(Read).c_str(),
		Sum.SponsorPaid.size(), ss::Money(SponsorLines).c_str(), ss::Money(KastPayout).c_str(), ss::Money(P.BankrollCents - Bank0).c_str());
	if (Sum.SponsorPaid.empty())
	{
		Expect(PLive && SponsorLines == 0 && P.BankrollCents - Bank0 == KastPayout, "kast glue: with no sponsor pay apart (the Room's C13 isn't in), the glue books no sponsor line");
	}
	else
	{
		Expect(PLive && ReadLine && HoursLine && SponsorLines > 1000 && P.BankrollCents - Bank0 == SponsorLines + KastPayout,
			"kast glue: C13: tonight's read and live hours are their own LedgerSponsors lines, paid beside the Kast payout, never inside it");
	}
}

void EconKastConsole()
{
	Hooks H;
	ss::Session S(H, "econ-kast-console");
	const bool Ready = EconAtTable(S);
	const double World = S.WorldMinutes();
	const bool Refuse = !S.StreamNod("dee_spincycle") && !S.StreamRailCall() && !S.StreamWelcomeRaid() && !S.StreamSetScene() && !S.StreamCallIt(true) && !S.StreamCallIt(false);
	Expect(Ready && Refuse, "kast glue: off air the studio's verbs refuse");
	const kast::growth::Standing Rk = S.KastRank();
	const kast::growth::Standing RkWant = kast::growth::RankAt(World, 0);
	const kast::growth::Band Fc = S.KastForecast();
	const kast::growth::Band FcWant = kast::Stream::Forecast(S.Channel, S.StreamInputs());
	Expect(Rk.Rank == RkWant.Rank && Rk.Of == RkWant.Of && Fc.Lo == FcWant.Lo && Fc.Centre == FcWant.Centre && Fc.Hi == FcWant.Hi,
		"kast glue: Studio's forecast and rank are the Stream's (a stream started now; the rank at the count)");
	const std::string Describe = S.DebugKast("describe");
	Expect(EconHas(Describe, "followers") && EconHas(Describe, "Heat") && EconHas(Describe, "Forecast") && EconHas(Describe, "pending"), "kast glue: ss.Kast.Describe");
	// L10 (the C-K gate): off air, Describe prints Heat's why lines as Studio's Heat tile shows them (a channel never live: new).
	Expect(EconHas(Describe, "Why (off air): " + kast::copy::WhyNew() + "."), "kast glue: ss.Kast.Describe off air prints Heat's why lines (a new channel's)");

	const std::string Grant = S.DebugKast("grant 60 40 3");
	int Regulars = 0;
	std::set<std::string> Names;
	for (const kast::Member& M : S.Channel.Members)
	{
		Regulars += M.Loyalty >= kast::RegularLoyalty ? 1 : 0;
		Names.insert(M.Name);
	}
	std::printf("  kast glue: %s\n", Grant.c_str());
	Expect(S.Channel.Followers >= 60 && Regulars >= 40 && S.Channel.ActiveSubs(World) >= 3 && S.Channel.Affiliate && Names.size() == S.Channel.Members.size(),
		"kast glue: ss.Kast.Grant 60 40 3: the followers, the regulars and the subs, members with names of their own");
	// (Granted before the channel's first stream: its first night's two friends are seated first, so First Faces still has them.)
	const kast::Member* Dee = S.Channel.FindMember("dee_spincycle");
	const kast::Member* Mei = S.Channel.FindMember("mei_ng");
	Expect(S.Channel.Streams == 0 && Dee && Dee->Friend && Mei && Mei->Friend, "kast glue: a grant before the first stream keeps Dee and Mei, the first night's friends");
	S.DebugKast("grant 1700");
	Expect(S.Channel.Followers >= 1700 && static_cast<int>(S.Channel.Members.size()) <= kast::MaxMembers, "kast glue: past the member cap the followers are the lurker pool");
	const std::string Heat = S.DebugKast("heat 55");
	const bool HeatSet = S.Channel.Heat == 55.0 && S.Channel.HeatAt == World;
	S.DebugKast("heat 500");
	Expect(HeatSet && S.Channel.Heat == 100.0 && EconHas(S.DebugKast("heat"), "<h>"), "kast glue: ss.Kast.Heat sets Heat as of now, from 8 to 100");
	S.DebugKast("rent");
	Expect(S.Life.RentsPaid >= 2, "kast glue: ss.Kast.Rent counts the second rent paid");
	const int Followers = S.Channel.Followers;
	const size_t Subs = S.Channel.Subs.size();
	const ss::Chips Unpaid = S.Channel.UnpaidCents;
	const double LastOffline = S.Channel.LastOffline;
	const std::string Advance = S.DebugKast("advance 30");
	std::printf("  kast glue: %s\n", Advance.c_str());
	// (§8.4: a skip of N days is N one-day steps to the byte; the copies start from the channel turned over to now, as the
	// session's offline branch keeps it, so a channel that never turned over compares like any other.)
	Expect(EconHas(Advance, "agree to the byte") && S.Channel.Followers == Followers && S.Channel.Subs.size() == Subs && S.Channel.UnpaidCents == Unpaid &&
			   S.Channel.LastOffline == LastOffline,
		"kast glue: ss.Kast.Advance 30: one step and 30 daily steps agree to the byte, on copies, and the channel is left as it was");
	Expect(EconHas(S.DebugKast("moment ft 1.40 980"), "go live first") && EconHas(S.DebugKast("raid 14"), "go live first") && EconHas(S.DebugKast("room"), "go live first"),
		"kast glue: the Room's verbs need a stream");
	Expect(EconHas(S.DebugKast("forecast"), "Forecast") && S.DebugKast("bogus").rfind("ss.Kast.<", 0) == 0, "kast glue: ss.Kast.Forecast off air; anything else prints the usage");
	const bool Live = S.GoLive().empty();
	const std::string Room = S.DebugKast("room");
	const std::string Moment = S.DebugKast("moment FT 1.40 980");
	const std::string LiveDescribe = S.DebugKast("describe");
	S.EndStream();
	Expect(Live && !Room.empty() && !Moment.empty() && EconHas(LiveDescribe, "watching") && !EconHas(LiveDescribe, "Why (off air)"),
		"kast glue: live, the Room's verbs answer, and Describe shows the room (and the room's why lines, not the off-air ones)");
}

/** What the stream's inputs say at a turn chime: this table, the front table as the inputs have it, the table in front. */
struct EconTurnSeen
{
	int This = -1;
	int Front = -1;
	int InFront = -1;
};

/** Hooks that read the stream's inputs at each turn chime: a table behind chimes its turn from inside its own turn. */
struct EconTurnHooks : Hooks
{
	const ss::Session* S = nullptr;
	int InFront = -1;
	std::vector<EconTurnSeen> Turns;
	void Sound(ss::SoundId Id, double Volume) override
	{
		Hooks::Sound(Id, Volume);
		if (Id == ss::SoundId::Turn && S)
		{
			const kast::Inputs In = S->StreamInputs();
			Turns.push_back({In.ThisTable, In.FrontTable, InFront});
		}
	}
};

/**
 * KastInputsExtra while a table behind takes its turn (Session::Update's loop): that table is this table, and nothing it
 * fires is the front table's (FrontTable -1: Hold or Crack opens only on the front table's all-in, §5.3 K12).
 */
void EconKastBehind()
{
	EconTurnHooks H;
	ss::Session S(H, "econ-kast-behind");
	H.S = &S;
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 500000;
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	const std::string R1 = S.RegisterEvent(EconListing("", "$1.10 Glue Sit A", 110, 18));
	const std::string R2 = S.RegisterEvent(EconListing("", "$1.10 Glue Sit B", 110, 18));
	if (!Rig || !R1.empty() || !R2.empty() || S.TableCount() != 2)
	{
		std::printf("  kast glue: two tables didn't open (%s / %s, %d tables)\n", R1.c_str(), R2.c_str(), S.TableCount());
		Expect(false, "kast glue: two tables open for the table-behind check");
		return;
	}
	double Now = 0.0;
	int Behind = 0;
	int InFront = 0;
	for (int F = 0; F < 20000 && (Behind < 2 || InFront < 2); ++F)
	{
		H.InFront = S.FocusedTable();
		const size_t Before = H.Turns.size();
		S.Update(Now += 0.1);
		for (size_t K = Before; K < H.Turns.size(); ++K)
		{
			Behind += H.Turns[K].This != H.Turns[K].InFront ? 1 : 0;
			InFront += H.Turns[K].This == H.Turns[K].InFront ? 1 : 0;
		}
	}
	bool Right = true;
	for (const EconTurnSeen& T : H.Turns)
	{
		Right = Right && T.This >= 0 && T.This < 2 && (T.This == T.InFront ? T.Front == T.InFront : T.Front == -1);
	}
	std::printf("  kast glue: turn chimes: %d at the table in front, %d at the table behind (%zu read)\n", InFront, Behind, H.Turns.size());
	Expect(Behind > 0 && InFront > 0 && Right, "kast glue: a table behind taking its turn is this table and not the front one (FrontTable -1); the table in front is both");
}

/** Rail Call's grant is saved at once (§2.3.6: once a world day; a reload the same day can't call the rail again). */
void EconKastRail()
{
	Hooks H;
	ss::Session S(H, "econ-kast-rail");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 500000;
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	const std::string R = S.RegisterEvent(EconListing("", "$3.30 Glue Rail", 330, 300));
	if (!Rig || !R.empty() || !S.T)
	{
		Expect(false, "kast glue: a 300-runner table for the Rail Call check");
		return;
	}
	S.T->Remaining = 20; // depth 0.77 of a 300-runner field: RAIL CALL's run
	// (The results screen ends the run, a won one too: no deep run there, so no Sweat or Event Mode on the results.)
	const kast::Inputs Deep = S.StreamInputs();
	const ss::Screen ScreenWas = S.CurrentScreen;
	S.CurrentScreen = ss::Screen::Results;
	const kast::Inputs Over = S.StreamInputs();
	S.CurrentScreen = ScreenWas;
	Expect(!kast::growth::Armed(300) || (Deep.DeepTable == 0 && Deep.Depth > 0.4 && Over.DeepTable == -1 && Over.Depth == 0.0 && Over.DeepEntrants == 0),
		"kast glue: the results screen ends the deep run (the hero still in, as after a win)");
	S.DebugKast("grant 40 30");
	const bool Live = S.GoLive().empty();
	S.LobbyMinutes += 3.0;
	const int Saves0 = H.Saves;
	const bool Called = S.StreamRailCall();
	const int Today = ss::net::DayOf(S.WorldMinutes());
	const bool Saved = H.Saves > Saves0 && kast::growth::Granted(H.Last.Channel.GrantDay, H.Last.Channel.GrantMask, Today, kast::growth::GrantRailCall);
	S.LobbyMinutes += 3.0;
	const bool Again = S.StreamRailCall();
	S.EndStream();
	std::printf("  kast glue: Rail Call: called %d, saved with the day's grant %d, again the same day %d\n", Called ? 1 : 0, Saved ? 1 : 0, Again ? 1 : 0);
	Expect(Live && Called && Saved && !Again, "kast glue: Rail Call's once-a-day grant is saved the moment it's used, and the rail can't be called again that day");
}

/**
 * C14 offline (§2.10.3; KastDaily, the C-K gate's ruling): before the second rent a channel under the new model holds its
 * renewals in PendingCents for the first payout after it; a channel this build hasn't migrated (no U3Kast: an old save that
 * hasn't streamed under it) writes no kast\tmo to hold them in, so its renewals wait in the balance, as they always did, and
 * none is lost at the next load. Ten subscribers renewing in half an hour, an hour of the calendar offline: first as an old
 * save (no U3Kast), then under the new model (the bit as NewCareer or a first stream under this build sets it).
 */
void EconKastDaily()
{
	Hooks H;
	ss::Session S(H, "econ-kast-daily");
	S.CurrentScreen = ss::Screen::Lobby;
	S.Life.RentsPaid = 1;
	S.DebugKast("grant 60 40 10");
	double Now = 0.0;
	S.Update(Now += 0.1); // the channel turns over once (Offline marks its day)
	for (const bool NewModel : {false, true})
	{
		if (NewModel)
		{
			S.Life.Update3LinesRead |= ss::life::U3Kast;
		}
		const double World = S.WorldMinutes();
		for (kast::Subscriber& Sub : S.Channel.Subs)
		{
			Sub.Renews = World + 30.0;
		}
		const ss::Chips Unpaid0 = S.Channel.UnpaidCents;
		const ss::Chips Pending0 = S.Channel.PendingCents;
		const ss::Chips Subs0 = S.Channel.EarnedSubs;
		S.LobbyMinutes += 60.0;
		S.Update(Now += 0.1);
		const ss::Chips Renewed = S.Channel.EarnedSubs - Subs0;
		const bool U3 = (S.Life.Update3LinesRead & ss::life::U3Kast) != 0;
		std::printf("  kast glue: offline before the second rent (%s): renewals %s; balance %s -> %s, pending %s -> %s\n", NewModel ? "the new model" : "an old save, not migrated",
			ss::Money(Renewed).c_str(), ss::Money(Unpaid0).c_str(), ss::Money(S.Channel.UnpaidCents).c_str(), ss::Money(Pending0).c_str(), ss::Money(S.Channel.PendingCents).c_str());
		if (NewModel)
		{
			Expect(U3 && Renewed > 0 && S.Channel.PendingCents > Pending0 && S.Channel.UnpaidCents == Unpaid0,
				"kast glue: a channel under the new model holds its renewals before the second rent (PendingCents, C14)");
		}
		else
		{
			Expect(!U3 && Renewed > 0 && S.Channel.PendingCents == 0 && S.Channel.UnpaidCents > Unpaid0,
				"kast glue: a channel not migrated yet (no U3Kast) keeps its renewals in the balance: nothing held where no save line keeps it");
		}
	}
}

/** The kast\t lines of a session's save as it stands (a hook's save, so nothing in the session moves). */
std::string GlueKastLines(ss::Session& S, Hooks& H)
{
	S.Save();
	std::string Out;
	const std::string Text = H.Last.Serialize();
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		if (Text.compare(Start, 5, "kast\t") == 0 || Text.compare(Start, 5, "life\t") == 0)
		{
			Out += Text.substr(Start, End - Start);
		}
		Start = End;
	}
	return Out;
}

/**
 * L8 (the C-K gate): before an old save's first stream under this build the forecast used the unmigrated numbers, so a channel
 * 8+ days dark read Warming up in Studio and went live Cold. KastForecast now previews what KastBeforeStart will seed (§8.3) on a
 * copy, and KastHeatAtGoLive gives Studio's Heat chip and why lines the same Heat on the numbers alone; neither touches the
 * channel. update2_creator (an old channel) nine world days dark: the preview is the migrated channel's forecast to the bit,
 * Heat 15 and Cold before the go-live as after it, and the go-live's own forecast (Start's, the end card's) is the preview's.
 */
void GlueForecastPreview()
{
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(seams::Fixture("update2_creator.ssave"), D);
	Hooks H;
	ss::Session S(H, "glue-forecast-preview", &D);
	S.CurrentScreen = ss::Screen::Lobby;
	if (!S.StreamInputs().Gear.CanStream())
	{
		S.BankrollCents += 300000;
		S.Buy(ss::gear::FirstPcUpgrade().Id);
	}
	// Nine world days after its last day live, at the same time of day (the fixture's own clock moved on, nothing else).
	const int LastDay = ss::session_kast_detail::KastLastDayLive(S.Channel);
	const int Dark = ss::net::DayOf(S.WorldMinutes()) - LastDay;
	if (LastDay >= 0 && Dark < 9)
	{
		S.LobbyMinutes += static_cast<double>(9 - Dark) * EconDay;
	}
	double Now = 0.0;
	S.Update(Now += 0.1); // the offline branch turns the channel over to now, as a session sitting in the lobby keeps it
	const double World = S.WorldMinutes();
	const bool Unset = (S.Life.Update3LinesRead & ss::life::U3Kast) == 0;
	const std::string Before = GlueKastLines(S, H);
	const kast::growth::Band Unmigrated = kast::Stream::Forecast(S.Channel, S.StreamInputs()); // what Studio read before L8
	const kast::growth::Band Fc = S.KastForecast();
	double SeedHeat = 0.0;
	double SeedAt = 0.0;
	int SeedDay = -2;
	const bool Preview = ss::session_kast_detail::KastHeatAtGoLive(S.Channel, S.Life.Update3LinesRead, World, SeedHeat, SeedAt, SeedDay);
	const std::string Describe = S.DebugKast("describe");
	const std::string After = GlueKastLines(S, H);
	kast::Channel Seeded = S.Channel;
	ss::session_kast_detail::KastMigrate(Seeded, World);
	const kast::growth::Band Want = kast::Stream::Forecast(Seeded, S.StreamInputs());
	const double RawShown = kast::growth::HeatShown(S.Channel.Heat, S.Channel.HeatAt, World, S.Channel.LastStreamDay);
	const double SeedShown = kast::growth::HeatShown(SeedHeat, SeedAt, World, SeedDay);
	const int Today = ss::net::DayOf(World);
	std::printf("  kast glue (L8): update2_creator %d days dark: unmigrated Heat %.1f (%s), forecast %.2f-%.2f (centre %.2f); the preview Heat %.1f (%s, last day %d), "
				"forecast %.2f-%.2f (centre %.2f), the migrated channel's %.2f-%.2f\n",
		Today - LastDay, RawShown, kast::copy::TierName(kast::growth::Tier(RawShown)).c_str(), Unmigrated.Lo, Unmigrated.Hi, Unmigrated.Centre, SeedHeat,
		kast::copy::TierName(kast::growth::Tier(SeedShown)).c_str(), SeedDay, Fc.Lo, Fc.Hi, Fc.Centre, Want.Lo, Want.Hi);
	Expect(Parsed && Unset && LastDay >= 0 && Today - LastDay >= 9 && Preview && SeedHeat == 15.0 && SeedAt == World && SeedDay == LastDay && kast::growth::Tier(SeedShown) == 0,
		"kast glue (L8): an old channel 9+ days dark previews the go-live's Heat 15 (Cold) and its last day live, before that go-live");
	Expect(Fc.Lo == Want.Lo && Fc.Centre == Want.Centre && Fc.Hi == Want.Hi, "kast glue (L8): Studio's forecast before an old save's first stream is the migrated channel's, to the bit");
	Expect(Before == After && (S.Life.Update3LinesRead & ss::life::U3Kast) == 0, "kast glue (L8): the preview touches nothing: the save's kast and life lines are byte-identical, U3Kast unset");
	Expect(EconHas(Describe, "next go-live seeds Heat 15.0 (" + kast::copy::TierName(0) + ")") && EconHas(Describe, "Why (off air): " + kast::copy::WhyCooling(Today - LastDay)),
		"kast glue (L8, L10): ss.Kast.Describe off air names the Heat the go-live seeds and the why line it will read (\"Cooling: N days dark\")");
	// What the preview costs (Studio asks once a world minute; printed, not banded): the copy of the community and the forecast,
	// against the Stream's forecast alone, on this channel and grown to the member cap.
	{
		const kast::Inputs In = S.StreamInputs();
		auto Time = [](int N, const auto& F) {
			const auto T0 = std::chrono::steady_clock::now();
			double Sum = 0.0;
			for (int K = 0; K < N; ++K)
			{
				Sum += F().Centre;
			}
			const double Us = EconSecondsSince(T0) * 1e6 / static_cast<double>(N);
			return Sum > -1.0 ? Us : -Us;
		};
		const double Preview0 = Time(200, [&S]() { return S.KastForecast(); });
		const double Plain0 = Time(200, [&S, &In]() { return kast::Stream::Forecast(S.Channel, In); });
		ss::SaveData DBig;
		ss::SaveData::Parse(seams::Fixture("update2_creator.ssave"), DBig);
		Hooks HBig;
		ss::Session Big(HBig, "glue-forecast-preview-big", &DBig);
		Big.LobbyMinutes = S.LobbyMinutes;
		Big.DebugKast("grant 1500");
		const size_t Members = Big.Channel.Members.size();
		const kast::Inputs InBig = Big.StreamInputs();
		const double Preview1 = Time(50, [&Big]() { return Big.KastForecast(); });
		const double Plain1 = Time(50, [&Big, &InBig]() { return kast::Stream::Forecast(Big.Channel, InBig); });
		std::printf("  kast glue (L8): the preview's cost, a forecast before the first stream: %.1f us (%zu members; the Stream's alone %.1f us), %.1f us at %zu members "
					"(alone %.1f us)\n",
			Preview0, S.Channel.Members.size(), Plain0, Preview1, Members, Plain1);
	}
	// The go-live seeds what the preview said, and its own forecast (Start's, read by Heat's lr and the end card) is the preview's.
	const std::string Live = S.GoLive();
	S.Update(Now += 0.1);
	const double LiveShown = kast::growth::HeatShown(S.Channel.Heat, S.Channel.HeatAt, World, S.Channel.LastStreamDay);
	const kast::growth::Band StartFc = S.Stream.GetView().Fc;
	const bool SeededAsShown = S.Channel.Heat == SeedHeat;
	Expect(EconNear(StartFc.Centre, Fc.Centre, 1e-9) && EconNear(StartFc.Lo, Fc.Lo, 1e-9) && EconNear(StartFc.Hi, Fc.Hi, 1e-9),
		"kast glue (L8): the go-live's own forecast is the one Studio showed");
	S.EndStream();
	// The night starts from the Heat the preview showed (the end card's "before", Heat's commit); live, the meter reads the floor
	// of a channel that streamed in the last 7 world days (20, §2.4: it is streaming today), as any night does.
	const double Before0 = S.Stream.Last.HeatBefore;
	std::printf("  kast glue (L8): live (\"%s\"): the night started at Heat %.1f (%s), the meter read %.1f (%s) while live; Start's forecast %.2f-%.2f (centre %.2f) against "
				"the preview's %.2f-%.2f\n",
		Live.c_str(), Before0, kast::copy::TierName(kast::growth::Tier(Before0)).c_str(), LiveShown, kast::copy::TierName(kast::growth::Tier(LiveShown)).c_str(), StartFc.Lo,
		StartFc.Hi, StartFc.Centre, Fc.Lo, Fc.Hi);
	Expect(Live.empty() && SeededAsShown && Before0 == SeedShown && kast::growth::Tier(Before0) == kast::growth::Tier(SeedShown) && LiveShown >= 20.0,
		"kast glue (L8): the go-live seeds the Heat the preview showed (Cold before, the night starts Cold), and live it reads the streaming floor");
	// After it, the channel is the new model's: KastForecast is the Stream's again (no preview, nothing copied).
	const kast::growth::Band Next = S.KastForecast();
	const kast::growth::Band NextWant = kast::Stream::Forecast(S.Channel, S.StreamInputs());
	double H2 = 0.0;
	double A2 = 0.0;
	int D2 = 0;
	Expect(!ss::session_kast_detail::KastHeatAtGoLive(S.Channel, S.Life.Update3LinesRead, S.WorldMinutes(), H2, A2, D2) && H2 == S.Channel.Heat && A2 == S.Channel.HeatAt &&
			   D2 == S.Channel.LastStreamDay && Next.Lo == NextWant.Lo && Next.Centre == NextWant.Centre && Next.Hi == NextWant.Hi,
		"kast glue (L8): once migrated, the forecast and the Heat are the channel's own (no preview)");
	// A channel that never went live previews nothing either: its Heat reads its start (26) until its first stream either way.
	Hooks H3;
	ss::Session F(H3, "glue-forecast-fresh");
	const bool Ready = EconAtTable(F);
	double H3Heat = 0.0;
	double H3At = 0.0;
	int H3Day = 0;
	const bool FreshPreview = ss::session_kast_detail::KastHeatAtGoLive(F.Channel, F.Life.Update3LinesRead, F.WorldMinutes(), H3Heat, H3At, H3Day);
	const kast::growth::Band FreshFc = F.KastForecast();
	const kast::growth::Band FreshWant = kast::Stream::Forecast(F.Channel, F.StreamInputs());
	Expect(Ready && !FreshPreview && FreshFc.Lo == FreshWant.Lo && FreshFc.Centre == FreshWant.Centre && FreshFc.Hi == FreshWant.Hi &&
			   kast::growth::HeatShown(H3Heat, H3At, F.WorldMinutes(), H3Day) == 26.0,
		"kast glue (L8): a channel that never went live is no migration: its forecast is the Stream's and its Heat reads 26");
}

/**
 * L8 (review-fix-mathglue): KastHeatAtGoLive restates KastMigrate's Heat lines on the numbers alone (Studio's Heat chip and why
 * lines, ss.Kast.Describe), so the two must agree wherever §8.3's rule turns: Heat 26 through the 7th world day after the last day
 * live and 15 from the 8th, at any hour of the go-live, from the days a channel kept or from its log alone (a channel with no
 * stream counted that kept a log is an old one too). A channel this build already runs (U3Kast) and one that never went live
 * preview nothing. And a never-live channel's go-live forecast is the one Studio showed: KastForecast skips its seeding because
 * the Heat it seeds is the start HeatShown already reads.
 */
void GlueSeedAgrees()
{
	int Cases = 0;
	int Agree = 0;
	int Recent = 0;
	for (const double Clock : {5.0, 900.0, 1435.0}) // 12:05 AM, 3 PM and 11:55 PM on world day 300
	{
		const double World = 300.0 * EconDay + Clock;
		const int Today = ss::net::DayOf(World);
		for (const int Dark : {0, 1, 6, 7, 8, 9, 40})
		{
			for (const bool FromLog : {false, true})
			{
				kast::Channel Ch;
				Ch.Streams = FromLog ? 0 : 12;
				Ch.Heat = 61.0;
				Ch.HeatAt = World - 50.0 * EconDay;
				if (FromLog)
				{
					kast::StreamLog Lg;
					Lg.Start = static_cast<double>(Today - Dark) * EconDay + 1.0;
					Lg.Minutes = 90.0;
					Ch.Log.push_back(Lg);
				}
				else
				{
					Ch.DaysLive = {Today - Dark - 3, Today - Dark};
				}
				double Heat = 0.0;
				double At = 0.0;
				int Day = -2;
				const bool Preview = ss::session_kast_detail::KastHeatAtGoLive(Ch, 0, World, Heat, At, Day);
				kast::Channel Migrated = Ch;
				ss::session_kast_detail::KastMigrate(Migrated, World);
				++Cases;
				Agree += Preview && Heat == Migrated.Heat && At == Migrated.HeatAt && Day == Migrated.LastStreamDay && Day == Today - Dark ? 1 : 0;
				Recent += Heat == 26.0 ? 1 : 0;
			}
		}
	}
	std::printf("  kast glue (L8): the go-live's Heat on the numbers alone against KastMigrate: %d of %d agree, %d at 26 (0-7 days dark)\n", Agree, Cases, Recent);
	Expect(Cases == 42 && Agree == Cases && Recent == 24,
		"kast glue (L8): Studio's preview of the go-live's Heat is KastMigrate's: 26 through the 7th world day dark, 15 from the 8th, at any hour, from the days kept or the log");
	const double World = 300.0 * EconDay + 900.0;
	kast::Channel Ours;
	Ours.Streams = 12;
	Ours.DaysLive = {290};
	Ours.Heat = 44.0;
	Ours.HeatAt = World - 600.0;
	Ours.LastStreamDay = 290;
	double Heat = 0.0;
	double At = 0.0;
	int Day = -2;
	const bool OursPreview = ss::session_kast_detail::KastHeatAtGoLive(Ours, ss::life::U3Kast, World, Heat, At, Day);
	const bool OursOwn = Heat == 44.0 && At == Ours.HeatAt && Day == 290;
	kast::Channel Never;
	const bool NeverPreview = ss::session_kast_detail::KastHeatAtGoLive(Never, 0, World, Heat, At, Day);
	Expect(!OursPreview && OursOwn && !NeverPreview && Heat == Never.Heat && At == Never.HeatAt && Day == -1,
		"kast glue (L8): no preview for a channel under the new model (U3Kast) or one that never went live: their own Heat, as of their own time");
	// A new career's first go-live: the forecast Studio showed is the one the night starts with.
	Hooks H;
	ss::Session F(H, "glue-seed-fresh");
	const bool Ready = EconAtTable(F);
	const kast::growth::Band Shown = F.KastForecast();
	const std::string Live = F.GoLive();
	double Now = 0.0;
	F.Update(Now += 0.1);
	const kast::growth::Band StartFc = F.Stream.GetView().Fc;
	F.EndStream();
	std::printf("  kast glue (L8): a new career's first go-live: Studio showed %.2f-%.2f (centre %.2f), the night started with %.2f-%.2f (centre %.2f)\n", Shown.Lo, Shown.Hi,
		Shown.Centre, StartFc.Lo, StartFc.Hi, StartFc.Centre);
	Expect(Ready && Live.empty() && EconNear(StartFc.Centre, Shown.Centre) && EconNear(StartFc.Lo, Shown.Lo) && EconNear(StartFc.Hi, Shown.Hi),
		"kast glue (L8): a never-live channel's go-live forecast is the one Studio showed (its seeding moves nothing the forecast reads)");
}

/** L10 (the C-K gate): Heat's why lines off air at their edges (kast::copy::WhyOffline, the lines Studio's Heat tile shows). */
void GlueWhyOffline()
{
	const double World = 300.0 * EconDay + 900.0;
	const int Today = ss::net::DayOf(World);
	const int Week = static_cast<int>(std::floor(static_cast<double>(Today) / 7.0));
	kast::Channel Ch;
	std::array<std::string, 2> W = kast::copy::WhyOffline(Ch, Ch.LastStreamDay, World);
	Expect(W[0] == kast::copy::WhyNew() && W[1].empty(), "kast glue (L10): a channel never live: \"New channel: Kast is getting to know you\"");
	Ch.Streams = 4;
	Ch.LastStreamDay = Today - 1;
	W = kast::copy::WhyOffline(Ch, Ch.LastStreamDay, World);
	Expect(W[0].empty() && W[1].empty(), "kast glue (L10): past stream 3, live yesterday, nothing else: no why line");
	W = kast::copy::WhyOffline(Ch, Today - 2, World);
	Expect(W[0] == kast::copy::WhyCooling(2) && W[1].empty(), "kast glue (L10): two world days dark: \"Cooling: 2 days dark\"");
	Ch.GainWeek = Week;
	Ch.GainUsed = 35.0;
	Ch.ShowWeek = Week;
	Ch.ShowDays = 1 | 4;
	W = kast::copy::WhyOffline(Ch, Today - 9, World);
	Expect(W[0] == kast::copy::WhyCooling(9) && W[1] == kast::copy::WhyHotWeek(), "kast glue (L10): the top two, in order: cooling, then the week's cap (showing up is third)");
	W = kast::copy::WhyOffline(Ch, Today - 1, World);
	Expect(W[0] == kast::copy::WhyHotWeek() && W[1] == kast::copy::WhyShowingUp(2), "kast glue (L10): the week's cap at 35, then \"Showing up: 2 nights this week\"");
	Ch.GainUsed = 34.9;
	Ch.ShowWeek = Week - 1;
	Ch.Streams = 3;
	W = kast::copy::WhyOffline(Ch, Today - 1, World);
	Expect(W[0] == kast::copy::WhyNew() && W[1].empty(), "kast glue (L10): under the cap, last week's showing up: nothing of this week, so stream 3 reads new");
}

/**
 * M3 (fix-room's live post): K17's end-card stings play only for a first the Room didn't post live. With the first stranger's tip
 * and the first raid in both new tonight: both sting when neither was posted live (today's Room), only the other one when one was
 * (Summary::Firsts holds its line), none when both were; a first already behind the channel never stings.
 */
void GlueEndStings()
{
	kast::Channel Ch;
	Ch.Firsts = kast::growth::FirstStrangerTip;
	Ch.RaidsIn = 1;
	kast::Summary Sum;
	const std::string Tip = kast::copy::StingStrangerTip();
	const std::string Raid = kast::copy::StingFirstRaid();
	const std::vector<std::string> Both = ss::session_kast_detail::KastEndStings(0, 0, Ch, Sum);
	Sum.Firsts = {kast::copy::FirstStrangerTip("lucia.mm", 200)};
	const std::vector<std::string> TipLive = ss::session_kast_detail::KastEndStings(0, 0, Ch, Sum);
	Sum.Firsts = {kast::copy::FirstRaidIn("kat_v", 14)};
	const std::vector<std::string> RaidLive = ss::session_kast_detail::KastEndStings(0, 0, Ch, Sum);
	Sum.Firsts = {kast::copy::FirstRaidIn("kat_v", 14), kast::copy::FirstStrangerTip("lucia.mm", 200)};
	const std::vector<std::string> BothLive = ss::session_kast_detail::KastEndStings(0, 0, Ch, Sum);
	Sum.Firsts.clear();
	const std::vector<std::string> Behind = ss::session_kast_detail::KastEndStings(kast::growth::FirstStrangerTip, 1, Ch, Sum);
	std::printf("  kast glue (M3): end-card stings: none live %zu, the tip live %zu, the raid live %zu, both live %zu, both behind %zu\n", Both.size(), TipLive.size(),
		RaidLive.size(), BothLive.size(), Behind.size());
	Expect(Both == std::vector<std::string>{Tip, Raid}, "kast glue (M3): nothing posted live: the end card stings the first stranger's tip and the first raid in");
	Expect(TipLive == std::vector<std::string>{Raid} && RaidLive == std::vector<std::string>{Tip} && BothLive.empty(),
		"kast glue (M3): a first the Room posted live tonight (Summary::Firsts) gets no end-card sting: one sound a milestone");
	Expect(Behind.empty(), "kast glue (M3): a first already behind the channel never stings");
	Expect(kast::copy::IsFirstStrangerTip(kast::copy::FirstStrangerTip("a", 500)) && kast::copy::IsFirstRaidIn(kast::copy::FirstRaidIn("b", 9)) &&
			   !kast::copy::IsFirstStrangerTip(kast::copy::FirstRaidIn("b", 9)) && !kast::copy::IsFirstRaidIn(kast::copy::FirstStrangerTip("a", 500)) &&
			   !kast::copy::IsFirstRaidIn(Raid) && !kast::copy::IsFirstStrangerTip(Tip),
		"kast glue (M3): the firsts' lines are told apart by their templates (and the stings are neither)");
	// End to end (review-fix-mathglue): a career's first raid in, through a real Session. Posted live by the Room (its line in
	// Summary::Firsts, its alert's KastShine) the end card stings nothing; not posted live, the end card's sting is its sound.
	// Either way the milestone sounds once (a first stranger's tip landing in the same seconds would be its own one).
	Hooks HR;
	ss::Session R(HR, "glue-first-raid");
	const bool RReady = EconAtTable(R);
	const bool RLive = RReady && R.GoLive().empty();
	double RNow = Wait(R, 0.0, 1.0);
	const uint32_t RFirsts0 = R.Channel.Firsts;
	const int RRaids0 = R.Channel.RaidsIn;
	const int RShine0 = HR.Sounds[ss::SoundId::KastShine];
	const std::string RSaid = R.DebugKast("raid 14");
	RNow = Wait(R, RNow, 2.0);
	R.EndStream();
	RNow = Wait(R, RNow, 4.0);
	const int RShine = HR.Sounds[ss::SoundId::KastShine] - RShine0;
	bool RPosted = false;
	for (const std::string& Line : R.Stream.Last.Firsts)
	{
		RPosted = RPosted || kast::copy::IsFirstRaidIn(Line);
	}
	const int RTip = (RFirsts0 & kast::growth::FirstStrangerTip) == 0 && (R.Channel.Firsts & kast::growth::FirstStrangerTip) != 0 ? 1 : 0;
	std::printf("  kast glue (M3): a career's first raid in (\"%s\"): raids in %d -> %d, posted live %s, KastShine x%d (a first stranger's tip too: %d)\n", RSaid.c_str(), RRaids0,
		R.Channel.RaidsIn, RPosted ? "yes" : "no", RShine, RTip);
	Expect(RLive && RRaids0 == 0 && R.Channel.RaidsIn == 1 && RShine == 1 + RTip,
		"kast glue (M3): a career's first raid in sounds once, live or at the end card, never both and never neither");
}

/** L11's Top clips flag survives a reload (patches/fix-room_to_lead_1.diff, applied by the C-K re-check): Clip::Lane rides
 *  kast\tclip as a 10th field, "1", only when set, the 9th (Rerun's candidate) then written even when empty. A clip outside the
 *  lane writes its line as before (8 or 9 fields), a line without the field reads false, and the save round-trips byte for byte. */
void GlueClipLane()
{
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(seams::Fixture("update2_creator.ssave"), D);
	kast::Clip A;
	A.At = 100.0;
	A.Views = 12.0;
	A.Reach = 40.0;
	A.Kind = 2;
	A.By = "kat_v";
	A.Title = "river nine";
	A.Lane = true;
	kast::Clip B = A;
	B.Title = "the fold";
	B.Lane = false;
	kast::Clip C = A;
	C.Title = "aces again";
	C.Candidate = "cand-7";
	D.Channel.Clips.insert(D.Channel.Clips.begin(), {A, B, C});
	const std::string Text = D.Serialize();
	ss::SaveData Back;
	const bool Again = ss::SaveData::Parse(Text, Back);
	auto Find = [&Back](const std::string& Title) -> const kast::Clip* {
		for (const kast::Clip& Cl : Back.Channel.Clips)
		{
			if (Cl.Title == Title)
			{
				return &Cl;
			}
		}
		return nullptr;
	};
	const kast::Clip* Ra = Find("river nine");
	const kast::Clip* Rb = Find("the fold");
	const kast::Clip* Rc = Find("aces again");
	const bool Lines = Text.find("kast\tclip\t100.00\t12.0\t40.0\t2\tkat_v\triver nine\t\t1\n") != std::string::npos &&
		Text.find("kast\tclip\t100.00\t12.0\t40.0\t2\tkat_v\tthe fold\n") != std::string::npos &&
		Text.find("kast\tclip\t100.00\t12.0\t40.0\t2\tkat_v\taces again\tcand-7\t1\n") != std::string::npos;
	const bool Read = Ra && Rb && Rc && Ra->Lane && Ra->Candidate.empty() && !Rb->Lane && Rb->Candidate.empty() && Rc->Lane && Rc->Candidate == "cand-7";
	std::printf("  kast glue (L11): the Clip Lane flag in kast\\tclip: lines %s, read back %s, round trip %s\n", Lines ? "yes" : "no", Read ? "yes" : "no",
		Again && Back.Serialize() == Text ? "byte for byte" : "MOVED");
	Expect(Parsed && Again && Lines && Read && Back.Serialize() == Text,
		"kast glue (L11): a Clip Lane clip keeps its Top clips flag through a save (kast\\tclip's 10th field), a clip outside the lane writes its old line");
}
} // namespace glue_seg

void SegGlue()
{
	const auto T0 = std::chrono::steady_clock::now();
	const int Checks0 = Checks;
	glue_seg::EconKastInputs();
	glue_seg::EconKastMigration();
	glue_seg::EconKastStop();
	glue_seg::EconKastConsole();
	glue_seg::EconKastBehind();
	glue_seg::EconKastRail();
	glue_seg::EconKastDaily();
	glue_seg::GlueForecastPreview();
	glue_seg::GlueSeedAgrees();
	glue_seg::GlueWhyOffline();
	glue_seg::GlueEndStings();
	glue_seg::GlueClipLane();
	std::printf("kast glue: the Session glue's checks (moved from night_one_session, and wave 2's), %d checks, %.2f s\n", Checks - Checks0, glue_seg::EconSecondsSince(T0));
}
} // namespace kastgrowth_test

int main(int Argc, char** Argv)
{
	kastgrowth_test::Long = Argc > 1 && std::string(Argv[1]) == "long";
	const auto T0 = std::chrono::steady_clock::now();
	kastgrowth_test::SegPure();
	kastgrowth_test::SegSession();
	kastgrowth_test::SegPlay();
	kastgrowth_test::SegGlue();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("kast growth tests: %d checks, %d failed in %.1fs\n", kastgrowth_test::Checks, kastgrowth_test::Failures, Seconds);
	return kastgrowth_test::Failures == 0 ? 0 : 1;
}
