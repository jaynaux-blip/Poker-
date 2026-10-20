// The rival sites' schedules (update 2 §5.3, §5.8): Network::BuildSite, the rival side of Network(const SiteDef&). Every
// template and series id starts with the site's key ("runittwice.sunday-grind", "runittwice.open26-12"), so Make() derives
// ids and seeds that never meet RiverLine's, and net::SiteOf reads the site off any of them. Rivals have no regulars and
// no past: their people and results come from the living world.
//
// A3's placeholder (the lead): schedules of the planned sizes and shapes, for spike S-6 and the id rules' tests. Ignite:
// 18 dailies from $1.10 to $22 (the hourly Flip & Go hypers, the $5.50 Spark Jackpot), the Sunday Blaze, and Wildfire in
// February and August (24 events each), about 690 templates through 2040. RunItTwice: 22 dailies from $3.30 to $215, the
// Sunday Grind, the Sunday 100K and the High Bridge, the RunItTwice Open in April and November (40 events; Nov 13-29 in
// 2026) and the Ironworks PKO series in July (24), about 1,520. The names, buy-ins, times, fields and guarantees are
// stand-ins: core-sites owns this file from B3 and replaces these tables with the real schedules (§5.3's guarantees at 0.9
// of the expected pool, the launch week's × 1.5). Nothing a player can reach uses them until core-sites registers the
// sites in Sites(): an unregistered site's network is built without a key and has no schedule.
#include "ShortStack/Game/Network.h"

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
	int Field;          // typical entries, the site's traffic already in
	int Start;          // minutes after midnight
	int Every;          // repeats (0: once a day)
	int Last;           // the last start
	int Days;           // weekday mask, bit 0 = Monday
	bool Gtd;           // carries a guarantee
	bool Featured;
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
	int Field;         // a typical event's entries
	bool Bounty;       // a PKO series
	int FirstYear;     // the first year it runs
};

Chips NetSitesCents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}

int NetSitesAt(int H, int M)
{
	return H * 60 + M;
}

/** A guarantee as a lobby writes it: two significant figures. */
double NetSitesNice(double Dollars)
{
	if (Dollars < 100.0)
	{
		return std::round(Dollars);
	}
	const double Step = std::pow(10.0, std::floor(std::log10(Dollars)) - 1.0);
	return std::round(Dollars / Step) * Step;
}

/** Level length and starting stack by speed, the way RiverLine's builders pick them. */
void NetSitesShape(EventTemplate& T, const char* Speed)
{
	const std::string S = Speed;
	T.Speed = S;
	T.LevelMinutes = S == "Hyper" ? 3.0 : S == "Turbo" ? 6.0 : S == "Deep" ? 15.0 : 10.0;
	T.StartStack = T.LevelMinutes <= 3.0 ? 10000 : T.LevelMinutes <= 6.0 ? 20000 : T.LevelMinutes >= 14.0 ? 50000 : 30000;
	T.LateRegMinutes = S == "Hyper" ? 15 : S == "Turbo" ? 60 : S == "Deep" ? 180 : 120;
}

// Ignite: bright, soft, loud. Hypers around the clock, a jackpot at night, a blaze on Sundays.
const NetSitesDaily IgniteDailies[] = {
	{"flip-go-110", "Flip & Go Bounty", 1.10, Format::Flip, "Hyper", 90, 0, 60, NetSitesAt(23, 0), 0x7f, false, false},
	{"flip-go-330", "Flip & Go Bounty Plus", 3.30, Format::Flip, "Hyper", 60, NetSitesAt(0, 30), 120, NetSitesAt(22, 30), 0x7f, false, false},
	{"spark-jackpot", "Spark Jackpot", 5.50, Format::Freezeout, "Regular", 410, NetSitesAt(20, 0), 0, 1439, 0x7f, true, true},
	{"sunrise-spin", "Sunrise Spin", 1.10, Format::Freezeout, "Turbo", 120, NetSitesAt(7, 0), 0, 1439, 0x7f, false, false},
	{"coffee-crash", "Coffee Crash", 2.20, Format::ReEntry, "Turbo", 140, NetSitesAt(9, 0), 0, 1439, 0x7f, true, false},
	{"lunch-blaze", "Lunch Blaze", 3.30, Format::Bounty, "Turbo", 160, NetSitesAt(12, 0), 0, 1439, 0x7f, true, false},
	{"afternoon-fizz", "Afternoon Fizz", 1.65, Format::Freezeout, "Regular", 130, NetSitesAt(14, 0), 0, 1439, 0x7f, false, false},
	{"coral-classic", "Coral Classic", 11.0, Format::ReEntry, "Regular", 220, NetSitesAt(16, 0), 0, 1439, 0x7f, true, false},
	{"plum-turbo", "Plum Turbo", 3.30, Format::Freezeout, "Turbo", 180, NetSitesAt(17, 30), 0, 1439, 0x7f, true, false},
	{"happy-hour-pko", "Happy Hour PKO", 5.50, Format::Bounty, "Regular", 260, NetSitesAt(18, 0), 0, 1439, 0x7f, true, false},
	{"sky-high", "Sky High", 22.0, Format::ReEntry, "Regular", 140, NetSitesAt(19, 0), 0, 1439, 0x7f, true, true},
	{"big-wave", "The Big Wave", 11.0, Format::Bounty, "Regular", 300, NetSitesAt(21, 0), 0, 1439, 0x7f, true, false},
	{"mystery-pinata", "Mystery Pinata", 7.70, Format::Mystery, "Regular", 240, NetSitesAt(21, 30), 0, 1439, 0x7f, true, false},
	{"night-glow", "Night Glow", 2.20, Format::Freezeout, "Turbo", 200, NetSitesAt(22, 0), 0, 1439, 0x7f, true, false},
	{"after-party", "After Party", 3.30, Format::ReEntry, "Turbo", 170, NetSitesAt(23, 0), 0, 1439, 0x7f, true, false},
	{"two-am-twister", "2 AM Twister", 1.10, Format::Freezeout, "Hyper", 110, NetSitesAt(2, 0), 0, 1439, 0x7f, false, false},
	{"deep-end", "The Deep End", 5.50, Format::ReEntry, "Deep", 150, NetSitesAt(15, 0), 0, 1439, 0x7f, true, false},
	{"confetti-cannon", "Confetti Cannon", 16.5, Format::Bounty, "Turbo", 120, NetSitesAt(20, 30), 0, 1439, 0x7f, true, false},
	{"sunday-blaze", "Sunday Blaze", 11.0, Format::ReEntry, "Regular", 1200, NetSitesAt(19, 0), 0, 1439, 1 << 6, true, true},
};
const NetSitesSeries IgniteSeries[] = {
	{"wildfeb", "Wildfire Winter", "WILDFIRE", 2, 6, 12, 24, 1.10, 22.0, 33.0, 300, false, 2027},
	{"wildaug", "Wildfire Summer", "WILDFIRE", 8, 8, 12, 24, 1.10, 22.0, 33.0, 300, false, 2027},
};

// RunItTwice: tournaments for grown-ups. A grind all day, three Sunday majors, the Open twice a year and a PKO summer.
const NetSitesDaily RunItTwiceDailies[] = {
	{"iron-hour", "Iron Hour", 3.30, Format::Freezeout, "Turbo", 70, NetSitesAt(0, 15), 60, NetSitesAt(23, 15), 0x7f, false, false},
	{"slate-turbo", "Slate Turbo", 5.50, Format::ReEntry, "Turbo", 90, NetSitesAt(1, 0), 120, NetSitesAt(23, 0), 0x7f, true, false},
	{"daily-grind", "The Daily Grind", 11.0, Format::ReEntry, "Regular", 260, NetSitesAt(19, 0), 0, 1439, 0x7f, true, true},
	{"run-it-back", "Run It Back", 33.0, Format::ReEntry, "Regular", 180, NetSitesAt(20, 0), 0, 1439, 0x7f, true, false},
	{"foundry-pko", "Foundry PKO", 22.0, Format::Bounty, "Regular", 200, NetSitesAt(20, 30), 0, 1439, 0x7f, true, false},
	{"bridge-55", "The Bridge", 55.0, Format::ReEntry, "Regular", 140, NetSitesAt(21, 0), 0, 1439, 0x7f, true, true},
	{"mortar", "Mortar", 5.50, Format::Freezeout, "Regular", 210, NetSitesAt(18, 0), 0, 1439, 0x7f, true, false},
	{"cornerstone", "Cornerstone", 16.5, Format::ReEntry, "Deep", 120, NetSitesAt(17, 0), 0, 1439, 0x7f, true, false},
	{"lintel", "Lintel Turbo", 11.0, Format::Bounty, "Turbo", 160, NetSitesAt(22, 0), 0, 1439, 0x7f, true, false},
	{"granite", "Granite", 109.0, Format::ReEntry, "Regular", 90, NetSitesAt(19, 30), 0, 1439, 0x7f, true, true},
	{"quarry", "The Quarry", 8.80, Format::Mystery, "Regular", 190, NetSitesAt(21, 30), 0, 1439, 0x7f, true, false},
	{"ashlar", "Ashlar Deepstack", 27.5, Format::ReEntry, "Deep", 110, NetSitesAt(15, 0), 0, 1439, 0x7f, true, false},
	{"plumb-line", "Plumb Line", 3.30, Format::Freezeout, "Regular", 150, NetSitesAt(12, 0), 0, 1439, 0x7f, false, false},
	{"arch", "The Arch", 44.0, Format::Bounty, "Turbo", 100, NetSitesAt(23, 0), 0, 1439, 0x7f, true, false},
	{"night-mason", "Night Mason", 7.70, Format::ReEntry, "Turbo", 140, NetSitesAt(0, 30), 0, 1439, 0x7f, true, false},
	{"early-shift", "Early Shift", 5.50, Format::Freezeout, "Turbo", 90, NetSitesAt(6, 0), 0, 1439, 0x7f, false, false},
	{"midday-mill", "Midday Mill", 11.0, Format::ReEntry, "Regular", 110, NetSitesAt(13, 0), 0, 1439, 0x7f, true, false},
	{"steelyard", "Steelyard", 215.0, Format::ReEntry, "Regular", 60, NetSitesAt(20, 0), 0, 1439, 0x7f, true, true},
	{"rivet", "Rivet Hyper", 3.30, Format::Freezeout, "Hyper", 80, NetSitesAt(16, 30), 0, 1439, 0x7f, false, false},
	{"girder", "Girder Six-Max", 22.0, Format::ReEntry, "Regular", 90, NetSitesAt(18, 30), 0, 1439, 0x7f, true, false},
	{"tower-pko", "Tower PKO", 82.5, Format::Bounty, "Regular", 80, NetSitesAt(21, 15), 0, 1439, 0x7f, true, false},
	{"last-brick", "The Last Brick", 16.5, Format::Freezeout, "Turbo", 130, NetSitesAt(1, 30), 0, 1439, 0x7f, true, false},
	{"sunday-grind", "Sunday Grind", 22.0, Format::ReEntry, "Regular", 900, NetSitesAt(17, 0), 0, 1439, 1 << 6, true, true},
	{"sunday-100k", "RunItTwice Sunday 100K", 109.0, Format::ReEntry, "Regular", 950, NetSitesAt(19, 0), 0, 1439, 1 << 6, true, true},
	{"high-bridge", "High Bridge", 530.0, Format::ReEntry, "Regular", 180, NetSitesAt(20, 0), 0, 1439, 1 << 6, true, true},
};
const NetSitesSeries RunItTwiceSeries[] = {
	{"openspring", "RunItTwice Open Spring", "OPEN", 4, 12, 17, 40, 3.30, 215.0, 530.0, 380, false, 2027},
	{"open", "RunItTwice Open", "OPEN", 11, 13, 17, 40, 3.30, 215.0, 530.0, 380, false, 2026},
	{"ironworks", "Ironworks", "IRON", 7, 10, 12, 24, 5.50, 109.0, 215.0, 300, true, 2027},
};

/** The guarantee a template carries: 0.9 of its typical pool, rounded the way lobbies write them (§5.8's rule, roughly). */
Chips NetSitesGtd(const SiteDef& D, const EventTemplate& T)
{
	const double Pool = static_cast<double>(T.Field) * static_cast<double>(T.BuyInCents) / 100.0 * D.Rake.PrizeShare(T);
	return NetSitesCents(NetSitesNice(D.GtdOfPool * Pool));
}
} // namespace netsites_detail

void Network::BuildSite()
{
	// A site with no key isn't registered yet: its network stays empty, so no id reaches another site's events.
	if (Rules->Key.empty())
	{
		return;
	}
	const netsites_detail::NetSitesDaily* Dailies = nullptr;
	size_t DailyCount = 0;
	const netsites_detail::NetSitesSeries* Plans = nullptr;
	size_t PlanCount = 0;
	uint32_t Color = 0x27d3c3;
	uint32_t Color2 = 0x3b82f6;
	switch (NetSite)
	{
	case SiteId::Ignite:
		Dailies = netsites_detail::IgniteDailies;
		DailyCount = sizeof(netsites_detail::IgniteDailies) / sizeof(netsites_detail::IgniteDailies[0]);
		Plans = netsites_detail::IgniteSeries;
		PlanCount = sizeof(netsites_detail::IgniteSeries) / sizeof(netsites_detail::IgniteSeries[0]);
		Color = 0xff6b57;
		Color2 = 0x4fc3f7;
		break;
	case SiteId::RunItTwice:
		Dailies = netsites_detail::RunItTwiceDailies;
		DailyCount = sizeof(netsites_detail::RunItTwiceDailies) / sizeof(netsites_detail::RunItTwiceDailies[0]);
		Plans = netsites_detail::RunItTwiceSeries;
		PlanCount = sizeof(netsites_detail::RunItTwiceSeries) / sizeof(netsites_detail::RunItTwiceSeries[0]);
		Color = 0xd4a843;
		Color2 = 0x334155;
		break;
	case SiteId::RiverLine:
	case SiteId::Halcyon: // P1
	case SiteId::Count: break;
	}
	const std::string Prefix = std::string(KeyOf(NetSite)) + ".";

	// The lobby's recurring events.
	for (size_t I = 0; I < DailyCount; ++I)
	{
		const netsites_detail::NetSitesDaily& Dy = Dailies[I];
		EventTemplate T;
		T.Id = Prefix + Dy.Id;
		T.Name = Dy.Name;
		T.BuyInCents = netsites_detail::NetSitesCents(Dy.BuyIn);
		T.Fmt = Dy.Fmt;
		netsites_detail::NetSitesShape(T, Dy.Speed);
		T.TableSize = Dy.Fmt == Format::Flip ? 6 : std::string(Dy.Name).find("Six-Max") != std::string::npos ? 6 : 8;
		T.Field = Dy.Field;
		T.StartMinute = Dy.Start;
		T.EveryMinutes = Dy.Every;
		T.LastMinute = Dy.Last;
		T.Days = Dy.Days;
		T.Featured = Dy.Featured;
		T.OpensHours = Dy.Days == 0x7f ? 18 : 72;
		T.GtdCents = Dy.Gtd ? netsites_detail::NetSitesGtd(*Rules, T) : 0;
		Temps.push_back(T);
	}

	// The series calendar, through 2040 (RiverLine's calendar runs as far).
	for (size_t P = 0; P < PlanCount; ++P)
	{
		const netsites_detail::NetSitesSeries& Pl = Plans[P];
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
			Sr.Tagline = std::to_string(Pl.Events) + " events in " + std::to_string(Pl.Span) + " days.";
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
				// Buy-ins climb through the series in steps between its least and its most; the main closes it.
				const double Step = Pl.Events > 2 ? static_cast<double>((No * 7) % (Pl.Events - 1)) / static_cast<double>(Pl.Events - 2) : 0.0;
				const double Dollars = Main ? Pl.MainBuyIn : std::round((Pl.MinBuyIn + (Pl.MaxBuyIn - Pl.MinBuyIn) * Step * Step) * 10.0) / 10.0;
				T.BuyInCents = netsites_detail::NetSitesCents(std::max(Pl.MinBuyIn, Dollars));
				T.Fmt = Pl.Bounty ? Format::Bounty : No % 5 == 0 ? Format::Freezeout : Format::ReEntry;
				netsites_detail::NetSitesShape(T, Main ? "Deep" : No % 4 == 0 ? "Turbo" : "Regular");
				T.TableSize = No % 6 == 0 ? 6 : 8;
				T.Field = Main ? Pl.Field * 3 : Pl.Field;
				T.OnlyDay = Sr.FirstDay + (No - 1) * Pl.Span / Pl.Events;
				T.StartMinute = Main ? netsites_detail::NetSitesAt(18, 0) : netsites_detail::NetSitesAt(15, 0) + ((No - 1) % 4) * 120;
				T.OpensHours = 72;
				T.Featured = Main;
				T.Name = Sr.Short + " #" + std::to_string(No) + ": " + BuyIn(T.BuyInCents) + (Main ? " Main Event" : Pl.Bounty ? " PKO" : " No-Limit Hold'em");
				T.GtdCents = netsites_detail::NetSitesGtd(*Rules, T);
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
