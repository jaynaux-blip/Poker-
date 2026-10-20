// The rival sites' schedules (update 2 §5.3, §5.8): Network::BuildSite, the rival side of Network(const SiteDef&). Every
// template and series id starts with the site's key ("runittwice.sunday-grind", "runittwice.open26-12"), so Make() derives
// ids and seeds that never meet RiverLine's, and net::SiteOf reads the site off any of them. Rivals have no regulars and
// no past: their people and results come from the living world, from the day their lobby opened.
//
// Ignite Poker: 18 dailies from $1.10 to $22 (Flip & Run every hour, the Flip & Run Bounty every two, the $5.50 Spark Jackpot
// at 8 PM), the $11 Sunday Blaze, and Wildfire in February and August (24 events each), about 800 templates through 2040 with
// its launch week. RunItTwice: 22 dailies from $3.30 to $215, three Sunday majors (the $22 Sunday Grind, the $109 Sunday 100K,
// the $530 High Bridge), the RunItTwice Open in April and November (40 events; Nov 13-29 in 2026) and the Ironworks PKO series
// in July (24), about 1,500.
//
// Fields and guarantees (§5.8). A template's Field is its RiverLine-sized field: Make() scales it by the site's traffic and
// the day (RiverLine's weekday factors, the site's own Sunday) and a draw, with no floor. Its guarantee is GtdOfPool (0.9) of
// the median pool that makes, rounded down to two figures as lobbies write them, so the weaker nights overlay (sites_test: 10
// to 25% over a month). Ignite's launch week (Nov 2-8, 2026) runs every guaranteed event as a one-off of its own carrying
// LaunchGtd (1.5x) the guarantee: the lobby and the screens read a template's guarantee, and an instance has none of its own.
// Network::Instances leaves the daily out on those days, and every rival's events start only once its lobby opened in the
// attached world (NetSitesLaunch), outside the schedule cache that every world shares.
#include "ShortStack/Game/Network.h"

#include "SitesDetail.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace net
{
namespace netsites_detail
{
/** A recurring tournament on a rival's lobby (a daily, or a weekly on Days). */
struct NetSitesDaily
{
	const char* Id;
	const char* Name;
	double BuyIn;
	Format Fmt;
	const char* Speed;
	int Entries;        // a typical weekday's entries on the site (a Sunday major's before the Sunday crowd)
	int Start;          // minutes after midnight
	int Every;          // repeats (0: once a day)
	int Last;           // the last start
	int Days;           // weekday mask, bit 0 = Monday
	bool Gtd;           // carries a guarantee
	bool Featured;
	int Stack;          // starting stack (0: by speed)
	int TableSize;      // 0: eight-handed
	const char* Blurb;  // "" for none
};

/** A series on a rival's calendar: where it falls in the year, how many events, its buy-ins and its main event. */
struct NetSitesSeries
{
	const char* Key;   // the series id's stem, before the year ("open", "wildfeb")
	const char* Name;  // with the year after it
	const char* Short;
	int Month;
	int Day;
	int Span;          // days
	int Events;
	double MinBuyIn, MaxBuyIn, MainBuyIn;
	int Entries;       // a typical event's entries on the site
	bool Bounty;       // a PKO series
	int FirstYear;     // the first year it runs
	const char* Tagline;
};

Chips NetSitesCents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}

int NetSitesAt(int H, int M)
{
	return H * 60 + M;
}

/**
 * A guarantee as a lobby writes it, rounded down to a clean number: two significant figures (whole dollars under $100). A
 * site sets its guarantees a little under what it expects, never over (the 1e-9 keeps an exact figure from falling a step).
 */
double NetSitesNice(double Dollars)
{
	if (Dollars < 100.0)
	{
		return std::floor(Dollars + 1e-9);
	}
	const double Step = std::pow(10.0, std::floor(std::log10(Dollars)) - 1.0);
	return std::floor(Dollars / Step + 1e-9) * Step;
}

/**
 * A series event's buy-in as a lobby prices it: the standard buy-in nearest the series' curve (by ratio) inside its range,
 * the way RiverLine's series take theirs from a ladder ($5.50, $11, $22, $55, $109, ...), never a $67.90 or a $118.20.
 */
double NetSitesStep(double Dollars, double Lo, double Hi)
{
	static const double Steps[] = {1.10, 2.20, 3.30, 4.40, 5.50, 7.70, 11.0, 16.50, 22.0, 27.50, 33.0, 44.0, 55.0, 82.50, 109.0, 162.0, 215.0, 320.0, 530.0};
	double Best = Lo;
	double Gap = 1.0e18;
	for (const double S : Steps)
	{
		const double G = std::fabs(std::log(S / std::max(Dollars, 0.01)));
		if (S >= Lo - 1e-9 && S <= Hi + 1e-9 && G < Gap)
		{
			Gap = G;
			Best = S;
		}
	}
	return Best;
}

/** Level length, starting stack and late registration by speed, the way RiverLine's builders pick them. */
void NetSitesShape(EventTemplate& T, const char* Speed)
{
	const std::string S = Speed;
	T.Speed = S;
	T.LevelMinutes = S == "Hyper" ? 3.0 : S == "Turbo" ? 6.0 : S == "Deep" ? 15.0 : 10.0;
	T.StartStack = T.LevelMinutes <= 3.0 ? 10000 : T.LevelMinutes <= 6.0 ? 20000 : T.LevelMinutes >= 14.0 ? 50000 : 30000;
	T.LateRegMinutes = S == "Hyper" ? 15 : S == "Turbo" ? 60 : S == "Deep" ? 180 : 120;
}

const int NetSitesEvery = 0x7f;
const int NetSitesSunday = 1 << 6;

// Ignite: bright, soft, loud. Flips around the clock, a jackpot at night, a blaze on Sundays. ("Flip & Run", never the real
// format name the internal ids "flip-go" come from: the ids stay, as saves and tickets key on them.)
const NetSitesDaily IgniteDailies[] = {
	{"flip-go", "Flip & Run", 1.10, Format::Flip, "Hyper", 60, 0, 60, NetSitesAt(23, 0), NetSitesEvery, false, false, 2000, 8,
		"Start short, flip fast, play poker with whoever's left. Every hour, on the hour."},
	{"flip-go-bounty", "Flip & Run Bounty", 3.30, Format::Bounty, "Hyper", 45, NetSitesAt(0, 30), 120, NetSitesAt(22, 30), NetSitesEvery, false, false, 2000, 8,
		"The short-stack flip with a price on every head. Every two hours."},
	{"two-am-afterburner", "2 AM Afterburner", 1.10, Format::Freezeout, "Hyper", 80, NetSitesAt(2, 0), 0, 1439, NetSitesEvery, false, false, 0, 0, ""},
	{"wake-up-call", "Wake-Up Call", 1.10, Format::Freezeout, "Turbo", 70, NetSitesAt(7, 0), 0, 1439, NetSitesEvery, false, false, 0, 0, ""},
	{"coffee-crash", "Coffee Crash", 2.20, Format::ReEntry, "Turbo", 90, NetSitesAt(9, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"lunch-break-pko", "Lunch Break PKO", 3.30, Format::Bounty, "Turbo", 110, NetSitesAt(12, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"afternoon-fizz", "Afternoon Fizz", 1.65, Format::Freezeout, "Regular", 90, NetSitesAt(14, 0), 0, 1439, NetSitesEvery, false, false, 0, 0, ""},
	{"deep-end", "The Deep End", 5.50, Format::ReEntry, "Deep", 120, NetSitesAt(15, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"coral-classic", "Coral Classic", 11.0, Format::ReEntry, "Regular", 150, NetSitesAt(16, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"plum-turbo", "Plum Turbo", 3.30, Format::Freezeout, "Turbo", 130, NetSitesAt(17, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"happy-hour-pko", "Happy Hour PKO", 5.50, Format::Bounty, "Regular", 190, NetSitesAt(18, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"sky-high", "Sky High", 22.0, Format::ReEntry, "Regular", 110, NetSitesAt(19, 0), 0, 1439, NetSitesEvery, true, true, 0, 0,
		"Ignite's top shelf. Twenty-two dollars, deep enough to play, and the regulars all show up for it."},
	{"spark-jackpot", "Spark Jackpot", 5.50, Format::Freezeout, "Regular", 410, NetSitesAt(20, 0), 0, 1439, NetSitesEvery, true, true, 0, 0,
		"The nightly headliner. Five-fifty in, a few hundred runners, and somebody's night changes at the final table."},
	{"confetti-cannon", "Confetti Cannon", 16.50, Format::Bounty, "Turbo", 90, NetSitesAt(20, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"big-wave", "The Big Wave", 11.0, Format::Bounty, "Regular", 230, NetSitesAt(21, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"mystery-pinata", "Mystery Pinata", 7.70, Format::Mystery, "Regular", 180, NetSitesAt(21, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"night-glow", "Night Glow", 2.20, Format::Freezeout, "Turbo", 150, NetSitesAt(22, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"after-party", "After Party", 3.30, Format::ReEntry, "Turbo", 120, NetSitesAt(23, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"sunday-blaze", "Sunday Blaze", 11.0, Format::ReEntry, "Regular", 1530, NetSitesAt(19, 0), 0, 1439, NetSitesSunday, true, true, 0, 0,
		"Ignite's Sunday. Eleven dollars, a guarantee the whole app talks about all week, and a couple of thousand people chasing it."},
};
const NetSitesSeries IgniteSeries[] = {
	{"wildfeb", "Wildfire Winter", "WILDFIRE", 2, 6, 12, 24, 1.10, 22.0, 33.0, 200, false, 2027, "Twenty-four events in twelve days, and every one of them loud."},
	{"wildaug", "Wildfire Summer", "WILDFIRE", 8, 8, 12, 24, 1.10, 22.0, 33.0, 200, false, 2027, "Twenty-four events in twelve days, and every one of them loud."},
};

// RunItTwice: tournaments for grown-ups. A grind all day, three Sunday majors, the Open twice a year and a PKO summer.
const NetSitesDaily RunItTwiceDailies[] = {
	{"hourly", "The Hourly", 3.30, Format::Freezeout, "Turbo", 45, NetSitesAt(0, 15), 60, NetSitesAt(23, 15), NetSitesEvery, false, false, 0, 0, ""},
	{"double-shot", "Double Shot", 5.50, Format::ReEntry, "Turbo", 60, NetSitesAt(1, 0), 120, NetSitesAt(23, 0), NetSitesEvery, true, false, 0, 0, ""},
	{"the-closer", "The Closer", 16.50, Format::Freezeout, "Turbo", 85, NetSitesAt(1, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"small-hours", "The Small Hours", 7.70, Format::ReEntry, "Turbo", 95, NetSitesAt(0, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"first-light", "First Light", 5.50, Format::Freezeout, "Turbo", 55, NetSitesAt(6, 0), 0, 1439, NetSitesEvery, false, false, 0, 0, ""},
	{"lunch-hour", "Lunch Hour", 3.30, Format::Freezeout, "Regular", 90, NetSitesAt(12, 0), 0, 1439, NetSitesEvery, false, false, 0, 0, ""},
	{"midday-classic", "Midday Classic", 11.0, Format::ReEntry, "Regular", 70, NetSitesAt(13, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"afternoon-deepstack", "Afternoon Deepstack", 27.50, Format::ReEntry, "Deep", 70, NetSitesAt(15, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"the-rush", "The Rush", 3.30, Format::Freezeout, "Hyper", 50, NetSitesAt(16, 30), 0, 1439, NetSitesEvery, false, false, 0, 0, ""},
	{"long-game", "The Long Game", 16.50, Format::ReEntry, "Deep", 80, NetSitesAt(17, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"supper-stack", "Supper Stack", 5.50, Format::Freezeout, "Regular", 140, NetSitesAt(18, 0), 0, 1439, NetSitesEvery, true, false, 0, 0,
		"Five-fifty at dinner time, guaranteed, and no easier for it."},
	{"six-max-classic", "Six-Max Classic", 22.0, Format::ReEntry, "Regular", 60, NetSitesAt(18, 30), 0, 1439, NetSitesEvery, true, false, 0, 6, ""},
	{"the-sweat", "The Sweat", 11.0, Format::ReEntry, "Regular", 170, NetSitesAt(19, 0), 0, 1439, NetSitesEvery, true, true, 0, 0,
		"The evening's eleven-dollar grind. Long enough to matter, short enough to sleep."},
	{"the-nightcap", "The Nightcap", 109.0, Format::ReEntry, "Regular", 60, NetSitesAt(19, 30), 0, 1439, NetSitesEvery, true, true, 0, 0,
		"The daily hundred-dollar game. The site's regulars treat it like a job."},
	{"run-it-back", "Run It Back", 33.0, Format::ReEntry, "Regular", 120, NetSitesAt(20, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"big-session", "The Big Session", 215.0, Format::ReEntry, "Regular", 40, NetSitesAt(20, 0), 0, 1439, NetSitesEvery, true, true, 0, 0,
		"Two hundred and fifteen a bullet, every night, and the rake drops the higher you play."},
	{"encore-pko", "Encore PKO", 22.0, Format::Bounty, "Regular", 130, NetSitesAt(20, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"the-fifty-five", "The Fifty-Five", 55.0, Format::ReEntry, "Regular", 90, NetSitesAt(21, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"headliner-pko", "Headliner PKO", 82.50, Format::Bounty, "Regular", 55, NetSitesAt(21, 15), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"sealed-envelopes", "Sealed Envelopes", 8.80, Format::Mystery, "Regular", 130, NetSitesAt(21, 30), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"twin-turbo-pko", "Twin Turbo PKO", 11.0, Format::Bounty, "Turbo", 110, NetSitesAt(22, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"last-orders", "Last Orders", 44.0, Format::Bounty, "Turbo", 65, NetSitesAt(23, 0), 0, 1439, NetSitesEvery, true, false, 0, 0, ""},
	{"sunday-grind", "Sunday Grind", 22.0, Format::ReEntry, "Regular", 450, NetSitesAt(17, 0), 0, 1439, NetSitesSunday, true, true, 0, 0,
		"Twenty-two dollars and the whole afternoon. The Sunday most of the site starts with."},
	{"sunday-100k", "RunItTwice Sunday 100K", 109.0, Format::ReEntry, "Regular", 706, NetSitesAt(19, 0), 0, 1439, NetSitesSunday, true, true, 0, 0,
		"The flagship. A hundred and nine to play for a hundred thousand guaranteed, and some Sundays it doesn't fill."},
	{"high-bridge", "High Bridge", 530.0, Format::ReEntry, "Regular", 97, NetSitesAt(20, 0), 0, 1439, NetSitesSunday, true, true, 0, 0,
		"The Sunday high roller. Small field, big names, and the lowest rake on the site."},
};
const NetSitesSeries RunItTwiceSeries[] = {
	{"openspring", "RunItTwice Open Spring", "OPEN", 4, 12, 17, 40, 3.30, 215.0, 530.0, 260, false, 2027, "Forty events in seventeen days, and a $530 Main Event to close it."},
	{"open", "RunItTwice Open", "OPEN", 11, 13, 17, 40, 3.30, 215.0, 530.0, 260, false, 2026, "Forty events in seventeen days, and a $530 Main Event to close it."},
	{"ironworks", "Ironworks", "IRON", 7, 10, 12, 24, 5.50, 109.0, 215.0, 200, true, 2027, "Twenty-four knockout events, a price on every head."},
};

/**
 * A template's expected field: its field on its median day (the median of the day factors of the days it runs: a Sunday
 * major's is Sunday's) at Make's median draw (0.82 to 1.25; a series' 0.9 to 1.15), scaled by the site's traffic.
 */
double NetSitesMedianField(const SiteDef& D, const EventTemplate& T)
{
	const bool Series = !T.Series.empty();
	std::vector<double> Factors;
	for (int Wd = 0; Wd < 7 && !Series; ++Wd)
	{
		const bool Runs = T.OnlyDay == -9999 ? (T.Days & (1 << Wd)) != 0 : Wd == Weekday(T.OnlyDay);
		if (Runs)
		{
			Factors.push_back(NetSitesDayFactor(D, Wd));
		}
	}
	std::sort(Factors.begin(), Factors.end());
	const double Day = Series || Factors.empty() ? 1.0 : Factors[Factors.size() / 2];
	const double Draw = Series ? (0.9 + 1.15) / 2.0 : (0.82 + 1.25) / 2.0;
	return static_cast<double>(T.Field) * D.Traffic.Scale * Day * Draw;
}

/** The guarantee a template carries: GtdOfPool of its median pool (Scale x LaunchGtd in a launch week), as lobbies write it. */
Chips NetSitesGtd(const SiteDef& D, const EventTemplate& T, double Scale)
{
	const Chips Contribution = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * D.Rake.PrizeShare(T)));
	const double Pool = NetSitesMedianField(D, T) * static_cast<double>(Contribution) / 100.0;
	return NetSitesCents(NetSitesNice(D.GtdOfPool * Pool * Scale));
}

double NetSitesDayFactor(const SiteDef& D, int Wd)
{
	// RiverLine's weekday factors (Network.cpp's DayFactor) with the site's own Sunday crowd.
	static const double F[7] = {0.92, 0.9, 0.94, 0.97, 1.02, 1.12, 1.38};
	const int W = ((Wd % 7) + 7) % 7;
	return W == 6 ? D.Traffic.Sunday : F[W];
}

double NetSitesLaunch(const Network& N)
{
	// The world says when this site's lobby opened (a save older than the site launches it the day after it loads); with no
	// world, or a world that doesn't say, the site's own launch day.
	const world::World* W = N.Attached();
	const double Said = W ? W->SiteLaunched(N.Site()) : -1.0;
	return Said >= 0.0 ? Said : static_cast<double>(N.Definition().LaunchDay) * MinutesPerDay;
}

std::string NetSitesLaunchBase(const std::string& Id)
{
	const size_t At = Id.rfind(NetSitesLaunchTag);
	const size_t Tag = std::string(NetSitesLaunchTag).size();
	if (At == std::string::npos || At + Tag + 1 != Id.size() || Id[At + Tag] < '1' || Id[At + Tag] > '7')
	{
		return std::string();
	}
	return Id.substr(0, At);
}
} // namespace netsites_detail

void Network::BuildSite()
{
	// A site with no key isn't registered yet: its network stays empty, so no id reaches another site's events.
	if (Rules->Key.empty())
	{
		return;
	}
	using namespace netsites_detail;
	const NetSitesDaily* Dailies = nullptr;
	size_t DailyCount = 0;
	const NetSitesSeries* Plans = nullptr;
	size_t PlanCount = 0;
	uint32_t Color = 0x27d3c3;
	uint32_t Color2 = 0x3b82f6;
	switch (NetSite)
	{
	case SiteId::Ignite:
		Dailies = IgniteDailies;
		DailyCount = sizeof(IgniteDailies) / sizeof(IgniteDailies[0]);
		Plans = IgniteSeries;
		PlanCount = sizeof(IgniteSeries) / sizeof(IgniteSeries[0]);
		Color = 0xff6b57;
		Color2 = 0x4fc3f7;
		break;
	case SiteId::RunItTwice:
		Dailies = RunItTwiceDailies;
		DailyCount = sizeof(RunItTwiceDailies) / sizeof(RunItTwiceDailies[0]);
		Plans = RunItTwiceSeries;
		PlanCount = sizeof(RunItTwiceSeries) / sizeof(RunItTwiceSeries[0]);
		Color = 0xd4a843;
		Color2 = 0x334155;
		break;
	case SiteId::RiverLine:
	case SiteId::Halcyon: // P1
	case SiteId::Count: break;
	}
	const SiteDef& D = *Rules;
	const std::string Prefix = std::string(KeyOf(NetSite)) + ".";
	const double Scale = D.Traffic.Scale > 0.0 ? D.Traffic.Scale : 1.0;

	// The lobby's recurring events.
	const size_t FirstDaily = Temps.size();
	for (size_t I = 0; I < DailyCount; ++I)
	{
		const NetSitesDaily& Dy = Dailies[I];
		EventTemplate T;
		T.Id = Prefix + Dy.Id;
		T.Name = Dy.Name;
		T.BuyInCents = NetSitesCents(Dy.BuyIn);
		T.Fmt = Dy.Fmt;
		NetSitesShape(T, Dy.Speed);
		T.StartStack = Dy.Stack > 0 ? Dy.Stack : T.StartStack;
		T.TableSize = Dy.TableSize > 0 ? Dy.TableSize : 8;
		T.Field = static_cast<int>(std::lround(static_cast<double>(Dy.Entries) / Scale));
		T.StartMinute = Dy.Start;
		T.EveryMinutes = Dy.Every;
		T.LastMinute = Dy.Last;
		T.Days = Dy.Days;
		T.Featured = Dy.Featured;
		T.OpensHours = Dy.Days == NetSitesEvery ? 18 : 72;
		T.Blurb = Dy.Blurb;
		T.GtdCents = Dy.Gtd ? NetSitesGtd(D, T, 1.0) : 0;
		Temps.push_back(T);
	}

	// The launch week (§5.3: Ignite's, guarantees x1.5): every guaranteed daily and weekly runs those seven days as a one-off of
	// its own ("ignite.spark-jackpot-launch3"), the guarantee raised; Instances leaves the daily out on those days.
	if (D.LaunchGtd > 1.0)
	{
		const size_t LastDaily = Temps.size();
		for (int K = 0; K < 7; ++K)
		{
			const int Day = D.LaunchDay + K;
			for (size_t I = FirstDaily; I < LastDaily; ++I)
			{
				const EventTemplate& Base = Temps[I];
				if (Base.GtdCents <= 0 || !(Base.Days & (1 << Weekday(Day))))
				{
					continue;
				}
				EventTemplate T = Base;
				T.Id = Base.Id + NetSitesLaunchTag + std::to_string(K + 1);
				T.OnlyDay = Day;
				T.EveryMinutes = 0;
				T.LastMinute = 1439;
				T.Days = NetSitesEvery;
				T.OpensHours = std::max(Base.OpensHours, 24 * (K + 1));
				T.GtdCents = NetSitesGtd(D, Base, D.LaunchGtd);
				T.Featured = Base.Featured || T.GtdCents >= NetSitesCents(1000.0);
				Temps.push_back(T);
			}
		}
	}

	// The series calendar, through 2040 (RiverLine's calendar runs as far).
	for (size_t P = 0; P < PlanCount; ++P)
	{
		const NetSitesSeries& Pl = Plans[P];
		for (int Year = Pl.FirstYear; Year <= 2040; ++Year)
		{
			const std::string Yy = std::to_string(Year % 100);
			SeriesInfo Sr;
			Sr.Id = Prefix + Pl.Key + (Yy.size() < 2 ? "0" + Yy : Yy);
			Sr.Name = std::string(Pl.Name) + " " + std::to_string(Year);
			Sr.Short = Pl.Short;
			Sr.FirstDay = world::DayOn(Year, Pl.Month, Pl.Day);
			Sr.LastDay = Sr.FirstDay + Pl.Span - 1;
			Sr.Events = Pl.Events;
			Sr.Color = Color;
			Sr.Color2 = Color2;
			Sr.Tagline = Pl.Tagline;
			Sr.MainEvent = Sr.Id + "-main";
			Chips Gtd = 0;
			for (int No = 1; No <= Pl.Events; ++No)
			{
				const bool Main = No == Pl.Events;
				EventTemplate T;
				T.Series = Sr.Id;
				T.EventNo = No;
				T.Main = Main;
				T.Id = Main ? Sr.MainEvent : Sr.Id + "-" + std::to_string(No);
				// Buy-ins climb through the series in steps between its least and its most, each a standard buy-in; the main closes it.
				const double Step = Pl.Events > 2 ? static_cast<double>((No * 7) % (Pl.Events - 1)) / static_cast<double>(Pl.Events - 2) : 0.0;
				const double Dollars = Main ? Pl.MainBuyIn : NetSitesStep(Pl.MinBuyIn + (Pl.MaxBuyIn - Pl.MinBuyIn) * Step * Step, Pl.MinBuyIn, Pl.MaxBuyIn);
				T.BuyInCents = NetSitesCents(std::max(Pl.MinBuyIn, Dollars));
				T.Fmt = Pl.Bounty ? Format::Bounty : No % 5 == 0 ? Format::Freezeout : Format::ReEntry;
				NetSitesShape(T, Main ? "Deep" : No % 4 == 0 ? "Turbo" : "Regular");
				T.TableSize = No % 6 == 0 ? 6 : 8;
				T.Field = static_cast<int>(std::lround(static_cast<double>(Main ? Pl.Entries * 3 : Pl.Entries) / Scale));
				T.OnlyDay = Sr.FirstDay + (No - 1) * Pl.Span / Pl.Events;
				T.StartMinute = Main ? NetSitesAt(18, 0) : NetSitesAt(15, 0) + ((No - 1) % 4) * 120;
				T.OpensHours = 72;
				T.Featured = Main;
				T.Name = Sr.Short + " #" + std::to_string(No) + ": " + BuyIn(T.BuyInCents) + (Main ? " Main Event" : Pl.Bounty ? " PKO" : " No-Limit Hold'em");
				T.GtdCents = NetSitesGtd(D, T, 1.0);
				Gtd += T.GtdCents;
				Temps.push_back(T);
			}
			Sr.GtdCents = Gtd;
			AllSeries.push_back(Sr);
		}
	}
}
} // namespace net
} // namespace ss
