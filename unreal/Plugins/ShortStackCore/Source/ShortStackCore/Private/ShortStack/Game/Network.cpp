#include "ShortStack/Game/Network.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Handles.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Structure.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>

namespace ss
{
namespace net
{
namespace network_detail
{
const char* const Days[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const char* const DaysLong[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
const char* const Months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const int MonthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Players on the network beyond the regulars simulated here (for the ranks of everyone else).
const double NetworkPlayers = 412000.0;
// The simulated past: results before this are folded into each regular's season totals.
const int SimFirstDay = -14;

double Clamp(double V, double Lo, double Hi)
{
	return V < Lo ? Lo : V > Hi ? Hi : V;
}

Chips Cents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}

/** Entries swing with the day of the week: Sunday is the big day. */
double DayFactor(int Day)
{
	static const double F[7] = {0.92, 0.9, 0.94, 0.97, 1.02, 1.12, 1.38};
	return F[Weekday(Day)];
}

double PrizeShare(const EventTemplate& T)
{
	return T.BuyInCents == 0 ? 0.0 : 0.915;
}
} // namespace network_detail

using namespace network_detail;

// ------------------------------------------------------------------ calendar

int Weekday(int Day)
{
	return ((Day % 7) + 7) % 7;
}

const char* WeekdayName(int Day, bool Long)
{
	return Long ? DaysLong[Weekday(Day)] : Days[Weekday(Day)];
}

std::string DateLabel(int Day)
{
	// Day 0 is October 5, 2026.
	int Month = 9;
	int D = 5 + Day;
	while (D < 1)
	{
		Month = (Month + 11) % 12;
		D += MonthDays[Month];
	}
	while (D > MonthDays[Month])
	{
		D -= MonthDays[Month];
		Month = (Month + 1) % 12;
	}
	return std::string(Months[Month]) + " " + std::to_string(D);
}

std::string TimeLabel(double WorldMinutes)
{
	const double InDay = WorldMinutes - std::floor(WorldMinutes / MinutesPerDay) * MinutesPerDay;
	return ClockString(InDay);
}

std::string Countdown(double Minutes)
{
	if (Minutes < 1.0)
	{
		return std::to_string(std::max(0, static_cast<int>(std::floor(Minutes * 60.0)))) + "s";
	}
	const int M = static_cast<int>(std::floor(Minutes));
	if (M < 60)
	{
		return std::to_string(M) + "m";
	}
	if (M < 24 * 60)
	{
		return std::to_string(M / 60) + "h " + std::to_string(M % 60) + "m";
	}
	return std::to_string(M / (24 * 60)) + "d " + std::to_string((M / 60) % 24) + "h";
}

std::string MoneyShort(Chips Cents)
{
	const double D = static_cast<double>(Cents) / 100.0;
	auto Trim = [](double V) {
		std::string S = Fixed(V, V < 10.0 ? 1 : 0);
		if (S.size() > 2 && S.substr(S.size() - 2) == ".0")
		{
			S = S.substr(0, S.size() - 2);
		}
		return S;
	};
	if (D >= 1e6)
	{
		return "$" + Trim(D / 1e6) + "M";
	}
	if (D >= 1e4)
	{
		return "$" + Trim(D / 1e3) + "K";
	}
	if (D >= 1e3)
	{
		return "$" + Fixed(D / 1e3, 1) + "K";
	}
	return "$" + Grouped(static_cast<int64_t>(std::llround(D)));
}

std::string BuyIn(Chips Cents)
{
	if (Cents == 0)
	{
		return "Free";
	}
	if (Cents % 100 == 0)
	{
		return "$" + Grouped(Cents / 100);
	}
	return Money(Cents);
}

Tier TierOf(Chips BuyInCents)
{
	return BuyInCents == 0 ? Tier::Freeroll : BuyInCents <= 550 ? Tier::Micro : BuyInCents <= 5500 ? Tier::Low : BuyInCents <= 53000 ? Tier::Mid : Tier::High;
}

Chips FeeOf(Chips BuyInCents)
{
	if (BuyInCents <= 0)
	{
		return 0;
	}
	// The prize part is a round number; the rake shrinks as the stakes rise.
	static const double Rates[6] = {0.10, 0.09, 0.08, 0.075, 0.06, 0.05};
	for (double Rate : Rates)
	{
		const double Prize = static_cast<double>(BuyInCents) / (1.0 + Rate);
		long long P = std::llround(Prize);
		if (std::fabs(Prize - static_cast<double>(P)) > 0.02 || P <= 0)
		{
			continue;
		}
		const long long Whole = P;
		while (P % 10 == 0)
		{
			P /= 10;
		}
		if (P < 100)
		{
			return BuyInCents - static_cast<Chips>(Whole);
		}
	}
	return std::max<Chips>(1, static_cast<Chips>(std::llround(static_cast<double>(BuyInCents) * 0.08)));
}

const char* TierName(Tier T)
{
	switch (T)
	{
	case Tier::Freeroll: return "Freeroll";
	case Tier::Micro: return "Micro";
	case Tier::Low: return "Low";
	case Tier::Mid: return "Mid";
	case Tier::High: return "High";
	}
	return "";
}

const char* StatusName(Status S)
{
	switch (S)
	{
	case Status::Announced: return "Announced";
	case Status::Registering: return "Registering";
	case Status::LateReg: return "Late Reg";
	case Status::Running: return "Running";
	case Status::FinalTable: return "Final Table";
	case Status::Finished: return "Finished";
	}
	return "";
}

double Points(int Place, int Entries, Chips BuyInCents)
{
	if (Place <= 0 || Entries <= 1)
	{
		return 0.0;
	}
	const double Paid = std::max(1.0, std::round(static_cast<double>(Entries) * 0.15));
	if (static_cast<double>(Place) > Paid)
	{
		return 0.0;
	}
	const double Stake = 1.0 + std::log10(1.0 + static_cast<double>(BuyInCents) / 100.0);
	return std::round(10.0 * std::sqrt(static_cast<double>(Entries)) / std::sqrt(static_cast<double>(Place)) * Stake);
}

double EntryStart(const HistoryEntry& E)
{
	const size_t At = E.EventId.rfind('@');
	return At == std::string::npos ? MinutesPerDay + 120.0 : std::atof(E.EventId.c_str() + At + 1);
}

HeroStats StatsFrom(const std::string& Name, const std::vector<HistoryEntry>& History, double Now)
{
	HeroStats H;
	H.Name = Name;
	const double Night = life::NightShiftStart(Now);
	const int ThisYear = world::YearOf(DayOf(Now));
	for (const HistoryEntry& E : History)
	{
		++H.Tournaments;
		H.Earnings += E.Prize;
		H.Best = std::max(H.Best, E.Prize);
		H.Wins += E.Place == 1 ? 1 : 0;
		H.FinalTables += E.Place >= 1 && E.Place <= 9 ? 1 : 0;
		H.Cashes += E.Prize > 0 ? 1 : 0;
		const bool Season = world::YearOf(DayOf(EntryStart(E))) == ThisYear;
		H.SeasonWins += Season && E.Place == 1 ? 1 : 0;
		H.SeasonFinalTables += Season && E.Place >= 1 && E.Place <= 9 ? 1 : 0;
		Chips BuyInCents = E.BuyInCents;
		for (const LobbyEvent& L : Lobby())
		{
			if (BuyInCents < 0 && L.Joinable && (L.Spec.Name == E.Name || L.Name == E.Name))
			{
				BuyInCents = L.BuyInCents;
			}
		}
		BuyInCents = std::max<Chips>(0, BuyInCents);
		const double P = Points(E.Place, E.Entrants, BuyInCents);
		H.SeasonPoints += Season ? P : 0.0;
		const double Start = EntryStart(E);
		if (BuyInCents <= 550 && Start >= Night && Start < Night + 12.0 * 60.0 && Start <= Now)
		{
			H.NightPoints += P;
		}
		if (E.Name.rfind("MM #", 0) == 0)
		{
			H.SeriesPoints += P;
			H.SeriesTitles += E.Place == 1 ? 1 : 0;
		}
	}
	return H;
}

// ------------------------------------------------------------------ the network

Network::Network()
{
	BuildPlayers();
	BuildSeries();
	BuildSchedule();
}

void Network::BuildPlayers()
{
	Rng R("riverline-network-v2");
	const int N = 1600;
	People.resize(static_cast<size_t>(N));
	std::vector<std::pair<double, int>> Bankroll;
	for (int I = 0; I < N; ++I)
	{
		Player& P = People[static_cast<size_t>(I)];
		P.Country = handles::PickCountry(R);
		P.Hue = R.Int(360);
		P.Skill = static_cast<float>(Clamp(0.5 + 0.17 * R.Gauss(0.0, 1.0), 0.05, 0.98));
		P.Volume = static_cast<float>(0.15 + 0.85 * std::pow(R.Next(), 1.5));
		Bankroll.push_back({static_cast<double>(P.Skill) * 0.7 + R.Next() * 0.3, I});
	}
	std::sort(Bankroll.begin(), Bankroll.end(), [](const std::pair<double, int>& A, const std::pair<double, int>& B) { return A.first > B.first; });
	int ProCount = 0;
	for (size_t K = 0; K < Bankroll.size(); ++K)
	{
		Player& P = People[static_cast<size_t>(Bankroll[K].second)];
		const double Q = static_cast<double>(K) / static_cast<double>(N);
		P.Stake = Q < 0.035 ? Tier::High : Q < 0.155 ? Tier::Mid : Q < 0.455 ? Tier::Low : Tier::Micro;
		if (P.Stake == Tier::High && ProCount < 10)
		{
			P.Pro = true;
			++ProCount;
		}
	}
	// The rival: a crusher who grinds the micros and low stakes every night.
	Rival = N - 1;
	{
		Player& P = People[static_cast<size_t>(Rival)];
		P.Rival = true;
		P.Name = RivalName;
		P.Country = "CA";
		P.Hue = 268;
		P.Skill = 0.93f;
		P.Volume = 0.97f;
		P.Stake = Tier::Low;
		P.Pro = false;
	}
	for (int I = 0; I < N; ++I)
	{
		Player& P = People[static_cast<size_t>(I)];
		const double Sk = 0.6 + static_cast<double>(P.Skill);
		const double Vol = static_cast<double>(P.Volume);
		double Median = 9000.0;
		double WinScale = 10.0;
		double WeeklyPoints = 250.0;
		switch (P.Stake)
		{
		case Tier::High: Median = 2.2e6; WinScale = 60.0; WeeklyPoints = 900.0; break;
		case Tier::Mid: Median = 3.0e5; WinScale = 40.0; WeeklyPoints = 620.0; break;
		case Tier::Low: Median = 4.0e4; WinScale = 26.0; WeeklyPoints = 420.0; break;
		default: break;
		}
		const double Dollars = Median * std::exp(R.Gauss(0.0, 0.85)) * Sk * (0.4 + Vol);
		P.Earnings = Cents(Dollars);
		P.Wins = std::max(0, static_cast<int>(std::round(WinScale * Sk * (0.3 + Vol) * std::exp(R.Gauss(0.0, 0.45)))));
		P.FinalTables = P.Wins * (5 + R.Int(4)) + R.Int(6);
		P.Cashes = P.FinalTables * (8 + R.Int(8)) + R.Int(30);
		P.Best = Cents(Dollars * R.Range(0.07, 0.28));
		// The 2026 season up to the simulated window (38 weeks).
		double Season = 0.0;
		for (int W = 0; W < 38; ++W)
		{
			Season += std::max(0.0, WeeklyPoints * Vol * Sk * std::exp(R.Gauss(0.0, 0.6)) - 40.0);
		}
		P.SeasonPoints = std::round(Season);
		P.SeasonEarnings = Cents(Dollars * R.Range(0.1, 0.24));
		P.SeasonWins = std::max(0, static_cast<int>(std::round(static_cast<double>(P.Wins) * R.Range(0.12, 0.26))));
		P.SeasonFinalTables = P.SeasonWins * (4 + R.Int(4)) + R.Int(4);
		for (size_t W = 0; W < P.Form.size(); ++W)
		{
			P.Form[W] = static_cast<float>(std::max(0.0, WeeklyPoints * Vol * Sk * std::exp(R.Gauss(0.0, 0.55))));
		}
	}
	// The rival's numbers are part of the story: well known on the night shift.
	{
		Player& P = People[static_cast<size_t>(Rival)];
		P.Earnings = Cents(184612.0);
		P.Wins = 61;
		P.FinalTables = 402;
		P.Cashes = 5120;
		P.Best = Cents(18406.0);
	}
	// Screen names last, once the stakes are known: high-stakes regulars lean to understated names.
	std::set<std::string> Used = {"gh0stfold", "grinder_3c"};
	for (int I = 0; I < N; ++I)
	{
		Player& P = People[static_cast<size_t>(I)];
		if (!P.Rival)
		{
			std::string Name = handles::Make(R, P.Country, P.Stake == Tier::High);
			for (int Try = 0; Used.count(Name) > 0 || Name.size() > 17; ++Try)
			{
				Name = Try < 8 ? handles::Make(R, P.Country, P.Stake == Tier::High) : Name.substr(0, 14) + std::to_string(R.Int(100));
			}
			P.Name = Name;
		}
		Used.insert(P.Name);
		ByName[P.Name] = I;
	}
	// Who makes final tables at each level: skill (squared, and then some), volume, and playing at those stakes.
	for (size_t T = 0; T < TierWeights.size(); ++T)
	{
		const int EventTier = std::max(static_cast<int>(T), static_cast<int>(Tier::Micro));
		std::vector<double>& W = TierWeights[T];
		W.resize(People.size());
		for (size_t I = 0; I < People.size(); ++I)
		{
			const Player& P = People[I];
			const int Diff = std::abs(static_cast<int>(P.Stake) - EventTier);
			double Match = Diff == 0 ? 1.0 : Diff == 1 ? 0.3 : Diff == 2 ? 0.04 : 0.004;
			if (P.Rival && (EventTier == static_cast<int>(Tier::Micro) || EventTier == static_cast<int>(Tier::Low)))
			{
				Match = 1.0;
			}
			W[I] = std::pow(static_cast<double>(P.Skill), 2.2) * (0.3 + static_cast<double>(P.Volume)) * Match;
		}
	}
}

void Network::BuildSeries()
{
	SeriesInfo Mm;
	Mm.Id = "mm";
	Mm.Name = "Micro Madness";
	Mm.Short = "MM";
	Mm.FirstDay = -4;
	Mm.LastDay = 9;
	Mm.Events = 70;
	Mm.GtdCents = Cents(5.0e6);
	Mm.Color = 0xb8ff2e;
	Mm.Color2 = 0x27d3c3;
	Mm.Tagline = "Two weeks. Seventy events from 55\xC2\xA2. Everyone's a shot.";
	Mm.MainEvent = "mm-main";
	AllSeries.push_back(Mm);

	SeriesInfo Rc;
	Rc.Id = "rcop";
	Rc.Name = "RCOP 2026";
	Rc.Short = "RCOP";
	Rc.FirstDay = 13;
	Rc.LastDay = 34;
	Rc.Events = 154;
	Rc.GtdCents = Cents(1.0e8);
	Rc.Color = 0xf2c14e;
	Rc.Color2 = 0x8b5cf6;
	Rc.Tagline = "The RiverLine Championship of Online Poker. $100,000,000 guaranteed.";
	Rc.MainEvent = "rcop-main";
	AllSeries.push_back(Rc);

	SeriesInfo Sl;
	Sl.Id = "slam";
	Sl.Name = "Summer Slam";
	Sl.Short = "SLAM";
	Sl.FirstDay = -46;
	Sl.LastDay = -30;
	Sl.Events = 82;
	Sl.GtdCents = Cents(3.0e7);
	Sl.Color = 0xf28a3a;
	Sl.Color2 = 0xef4d5a;
	Sl.Tagline = "The summer festival: 82 events, $30,000,000 guaranteed.";
	Sl.MainEvent = "slam-main";
	AllSeries.push_back(Sl);
}

void Network::BuildSchedule()
{
	auto Add = [this](const std::string& Id, const std::string& Name, double BuyIn, double Gtd, Format F, const char* Speed, double Level, int Field, int StartMin,
				   int Every = 0, int LateReg = 120) -> EventTemplate& {
		EventTemplate T;
		T.Id = Id;
		T.Name = Name;
		T.BuyInCents = Cents(BuyIn);
		T.GtdCents = Cents(Gtd);
		T.Fmt = F;
		T.Speed = Speed;
		T.LevelMinutes = Level;
		T.Field = Field;
		T.StartMinute = StartMin;
		T.EveryMinutes = Every;
		T.LateRegMinutes = LateReg;
		T.StartStack = Level <= 3.0 ? 10000 : Level <= 6.0 ? 20000 : Level >= 14.0 ? 50000 : 30000;
		Temps.push_back(T);
		return Temps.back();
	};
	auto At = [](int H, int M) { return H * 60 + M; };

	// Tonight's playable events (Lobby() indices), repeating so there is always one to join.
	{
		EventTemplate& T = Add("night-owl", "Night Owl Turbo", 1.10, 1000, Format::Freezeout, "Turbo", 5.0, 1000, At(23, 0), 90, 40);
		T.LastMinute = At(6, 30);
		T.JoinIndex = 0;
		T.TableSize = 9;
		T.StartStack = 10000;
		T.Featured = true;
		T.Blurb = "The big one on the night shift. 1,000 grinders, 150 paid, $178 to the winner.";
		// Before midnight it runs from 23:00; the mask covers every day.
	}
	{
		EventTemplate& T = Add("hyper-sprint", "Hyper Sprint", 0.25, 0, Format::Freezeout, "Hyper", 3.0, 180, 0, 30, 15);
		T.JoinIndex = 1;
		T.TableSize = 9;
		T.StartStack = 10000;
		T.Blurb = "Quick and wild. 180 players, blinds up every 3 minutes. Every half hour.";
	}
	{
		EventTemplate& T = Add("freeroll", "Midnight Freeroll", 0.0, 50, Format::Freezeout, "Turbo", 4.0, 1000, At(1, 45), 0, 45);
		T.JoinIndex = 2;
		T.TableSize = 9;
		T.StartStack = 10000;
		T.Blurb = "Free to enter. Everyone shoves. Somebody has to win the $9.";
	}
	{
		EventTemplate& T = Add("insomniac", "Insomniac Freeroll", 0.0, 30, Format::Freezeout, "Turbo", 4.0, 800, At(3, 30), 120, 45);
		T.LastMinute = At(7, 0);
		T.JoinIndex = 7;
		T.TableSize = 9;
		T.StartStack = 10000;
		T.Blurb = "For the ones still up. Free, fast, and $30 to fight over.";
	}

	// Recurring daily events across the stakes.
	Add("hh-110", "Headhunter PKO", 1.10, 1000, Format::Bounty, "Turbo", 6.0, 900, At(1, 0), 120, 90);
	Add("hh-330", "Headhunter PKO", 3.30, 2500, Format::Bounty, "Regular", 8.0, 800, At(0, 30), 180, 120);
	Add("hh-540", "Headhunter PKO", 5.40, 5000, Format::Bounty, "Regular", 10.0, 950, At(0, 0), 120, 150);
	Add("hh-1080", "Headhunter PKO", 10.80, 10000, Format::Bounty, "Regular", 10.0, 1100, At(2, 0), 240, 150);
	Add("hh-2160", "Headhunter PKO", 21.60, 20000, Format::Bounty, "Regular", 10.0, 1050, At(4, 0), 480, 150);
	Add("hh-54", "Headhunter PKO", 54, 50000, Format::Bounty, "Regular", 12.0, 1150, At(18, 0), 0, 180);
	Add("hh-108", "Headhunter PKO", 108, 80000, Format::Bounty, "Regular", 12.0, 820, At(19, 30), 0, 180);
	Add("hh-215", "Headhunter High Roller", 215, 100000, Format::Bounty, "Regular", 12.0, 520, At(20, 30), 0, 180).Featured = true;
	Add("grind-330", "Daily Grind", 3.30, 2000, Format::ReEntry, "Regular", 8.0, 700, At(1, 30), 180, 120);
	Add("grind-11", "Daily Grind", 11, 5000, Format::ReEntry, "Regular", 10.0, 560, At(12, 0), 360, 150);
	Add("grind-33", "Daily Grind", 33, 10000, Format::ReEntry, "Regular", 10.0, 380, At(16, 0), 0, 150);
	{
		EventTemplate& T = Add("daily-main", "Daily Main Event", 109, 50000, Format::ReEntry, "Regular", 12.0, 560, At(20, 0), 0, 210);
		T.Featured = true;
		T.Blurb = "The daily centrepiece. Deep enough to play, big enough to matter.";
	}
	Add("daily-hr", "Daily High Roller", 525, 100000, Format::ReEntry, "Regular", 12.0, 205, At(21, 0), 0, 180);
	Add("deep-22", "Deep River", 22, 15000, Format::ReEntry, "Deep", 15.0, 720, At(17, 0), 0, 240);
	Add("deep-55", "Deep River", 55, 30000, Format::ReEntry, "Deep", 15.0, 560, At(15, 0), 0, 240);
	Add("mystery-1080", "Mystery Madness", 10.80, 40000, Format::Mystery, "Regular", 10.0, 3900, At(22, 0), 0, 180).Featured = true;
	Add("flip-1", "Flip Frenzy", 1.0, 0, Format::Flip, "Hyper", 2.0, 300, At(0, 15), 60, 10);
	Add("flip-5", "Flip Frenzy", 5.0, 0, Format::Flip, "Hyper", 2.0, 180, At(0, 45), 120, 10);
	Add("shot-540", "Shotclock Turbo 6-Max", 5.40, 2000, Format::Bounty, "Turbo", 5.0, 520, At(1, 30), 240, 60).TableSize = 6;
	Add("shot-22", "Shotclock Turbo 6-Max", 22, 8000, Format::Bounty, "Turbo", 5.0, 380, At(23, 0), 0, 60).TableSize = 6;
	Add("plo-550", "Omaha Circuit", 5.50, 1500, Format::Freezeout, "Regular", 8.0, 260, At(2, 30), 0, 90).Omaha = true;
	Add("plo-22", "Omaha Circuit", 22, 10000, Format::Bounty, "Regular", 10.0, 420, At(19, 0), 0, 150).Omaha = true;
	Add("bigstack-11", "Big Stack", 11, 10000, Format::Freezeout, "Deep", 12.0, 980, At(20, 0), 0, 180);
	Add("bounty-builder", "Bounty Builder", 5.50, 3000, Format::Bounty, "Regular", 10.0, 612, At(4, 0), 0, 120);
	Add("graveyard", "Graveyard Grind", 3.30, 2000, Format::ReEntry, "Turbo", 6.0, 640, At(3, 0), 0, 90);
	Add("sunrise", "Sunrise Special", 5.50, 3000, Format::Freezeout, "Turbo", 6.0, 540, At(6, 0), 0, 90);
	Add("monster-550", "Monster Stack", 5.50, 20000, Format::ReEntry, "Deep", 12.0, 4100, At(23, 30), 0, 240);
	// Satellites: the steps to the RCOP Main Event.
	{
		EventTemplate& T = Add("step1", "Step 1 \xC2\xB7 RCOP Main", 2.20, 0, Format::Satellite, "Turbo", 5.0, 90, At(1, 30), 240, 30);
		T.Seats = "Step 2 tickets ($11)";
		T.SeatTicket = "step2";
		T.SeatValueCents = Cents(11.0);
		T.Blurb = "Win a ticket to Step 2. Five steps from $2.20 to a $5,250 seat in the Main Event.";
	}
	auto Step = [&](const std::string& Id, const std::string& Name, double BuyIn, double Level, int Field, int StartMin, int Every, const char* Seats, const char* Ticket, double SeatValue) {
		EventTemplate& T = Add(Id, Name, BuyIn, 0, Format::Satellite, Level >= 8.0 ? "Regular" : "Turbo", Level, Field, StartMin, Every, 30);
		T.Seats = Seats;
		T.SeatTicket = Ticket;
		T.SeatValueCents = Cents(SeatValue);
	};
	Step("step2", "Step 2 \xC2\xB7 RCOP Main", 11, 6.0, 60, At(3, 0), 360, "Step 3 tickets ($55)", "step3", 55.0);
	Step("step3", "Step 3 \xC2\xB7 RCOP Main", 55, 6.0, 45, At(12, 0), 480, "Step 4 tickets ($215)", "step4", 215.0);
	Step("step4", "Step 4 \xC2\xB7 RCOP Main", 215, 8.0, 36, At(20, 30), 0, "RCOP Main Event seats", "rcop-main", 5250.0);

	// Weekly majors.
	auto Weekly = [&](const std::string& Id, const std::string& Name, double BuyIn, double Gtd, Format F, const char* Speed, double Level, int Field, int StartMin, int Day,
					  bool Featured, const std::string& Blurb) {
		EventTemplate& T = Add(Id, Name, BuyIn, Gtd, F, Speed, Level, Field, StartMin, 0, 240);
		T.Days = 1 << Day;
		T.OpensHours = 72;
		T.Featured = Featured;
		T.Blurb = Blurb;
	};
	Weekly("monday-madness", "Monday Madness", 54, 100000, Format::Bounty, "Regular", 12.0, 2100, At(20, 0), 0, true, "Start the week with $100K guaranteed and bounties on every head.");
	Weekly("turbo-tuesday", "Turbo Tuesday", 22, 40000, Format::ReEntry, "Turbo", 6.0, 2050, At(21, 0), 1, false, "");
	Weekly("mystery-108", "Mystery Madness High", 108, 150000, Format::Mystery, "Regular", 12.0, 1500, At(21, 0), 2, true, "Mystery bounties up to $25,000 in the envelopes.");
	Weekly("thursday-heater", "Thursday Heater", 215, 250000, Format::ReEntry, "Regular", 12.0, 1260, At(20, 0), 3, true, "Midweek's biggest guarantee.");
	Weekly("friday-fight", "Friday Night Fight", 33, 80000, Format::Bounty, "Regular", 10.0, 2700, At(21, 0), 4, false, "");
	Weekly("saturday-six", "Saturday Six-Max", 109, 150000, Format::ReEntry, "Regular", 12.0, 1500, At(19, 0), 5, false, "");
	Weekly("millions", "RiverLine Millions", 1050, 3000000, Format::ReEntry, "Regular", 15.0, 3100, At(13, 0), 6, true,
		"The Sunday flagship. $3,000,000 guaranteed, 15-minute levels, and the best in the world at every table.");
	Weekly("sunday-showdown", "Sunday Showdown", 215, 1000000, Format::ReEntry, "Regular", 12.0, 5400, At(15, 0), 6, true,
		"The people's major. $1M guaranteed every Sunday; last week 7,412 entries built a $1.47M prize pool.");
	Weekly("sunday-special", "Sunday Special", 54, 300000, Format::Bounty, "Regular", 12.0, 6200, At(16, 0), 6, false, "");
	Weekly("sunday-storm", "Sunday Storm", 11, 150000, Format::ReEntry, "Regular", 10.0, 15600, At(17, 0), 6, true, "The biggest micro-stakes tournament of the week.");
	Weekly("sunday-shr", "Super High Roller Bounty", 2100, 300000, Format::Bounty, "Regular", 15.0, 150, At(18, 0), 6, false, "");
	Weekly("mega-rcop", "RCOP Main Mega Satellite", 109, 0, Format::Satellite, "Turbo", 8.0, 1400, At(18, 0), 5, true, "25 seats to the $5,250 RCOP Main Event guaranteed.");
	Temps.back().Seats = "25 Main Event seats";
	Temps.back().SeatTicket = "rcop-main";
	Temps.back().SeatValueCents = Cents(5250.0);

	// Micro Madness: five events a day, and a Main Event on Sunday, October 11.
	{
		const double Buys[7] = {0.55, 1.10, 2.20, 3.30, 5.50, 11.0, 22.0};
		const char* Kinds[10] = {"Kickoff", "PKO", "Turbo", "Deepstack", "Mystery Bounty", "Six-Max", "Hyper", "Omaha", "Big Stack", "Freezeout"};
		const int Times[5] = {At(0, 30), At(13, 0), At(16, 0), At(19, 0), At(22, 0)};
		int No = 1;
		for (int Day = -4; Day <= 9; ++Day)
		{
			for (int K = 0; K < 5; ++K, ++No)
			{
				EventTemplate T;
				T.Series = "mm";
				T.EventNo = No;
				T.OnlyDay = Day;
				T.StartMinute = Times[K];
				T.OpensHours = 24 * 10;
				T.TableSize = 9;
				if (Day == 6 && K == 2)
				{
					T.Id = "mm-main";
					T.Name = "MM #" + std::to_string(No) + ": Main Event";
					T.BuyInCents = Cents(11.0);
					T.GtdCents = Cents(1.0e6);
					T.Fmt = Format::ReEntry;
					T.LevelMinutes = 12.0;
					T.StartStack = 50000;
					T.Field = 108000;
					T.LateRegMinutes = 360;
					T.StartMinute = At(15, 0);
					T.Featured = true;
					T.Blurb = "$11 to play for a million. The biggest micro-stakes tournament of the year.";
				}
				else if (Day == 1 && K == 0)
				{
					// Tonight's series event, playable.
					T.Id = "mm-26";
					T.Name = "MM #" + std::to_string(No) + ": Night Crawler";
					T.BuyInCents = Cents(2.20);
					T.GtdCents = Cents(2000.0);
					T.Fmt = Format::Freezeout;
					T.Speed = "Turbo";
					T.LevelMinutes = 5.0;
					T.StartStack = 10000;
					T.Field = 1184;
					T.StartMinute = At(1, 30);
					T.LateRegMinutes = 100;
					T.JoinIndex = 8;
					T.Featured = true;
					T.Blurb = "A Micro Madness title for $2.20. Late registration is open until 3:10 AM.";
				}
				else
				{
					const int Kind = ((No * 3 + Day * 7) % 10 + 10) % 10;
					const double B = Buys[(No * 3 + K) % 7];
					T.Id = "mm-" + std::to_string(No);
					T.Name = "MM #" + std::to_string(No) + ": " + BuyIn(Cents(B)) + " " + Kinds[Kind];
					T.BuyInCents = Cents(B);
					T.GtdCents = Cents(B * (K == 0 ? 1200.0 : 2600.0));
					T.Fmt = Kind == 1 ? Format::Bounty : Kind == 4 ? Format::Mystery : Format::ReEntry;
					T.Omaha = Kind == 7;
					T.Speed = Kind == 2 ? "Turbo" : Kind == 6 ? "Hyper" : Kind == 3 ? "Deep" : "Regular";
					T.LevelMinutes = Kind == 2 ? 5.0 : Kind == 6 ? 3.0 : Kind == 3 ? 15.0 : 10.0;
					T.StartStack = Kind == 3 ? 50000 : 20000;
					T.TableSize = Kind == 5 ? 6 : 8;
					T.Field = static_cast<int>((K == 0 ? 1300.0 : 3000.0) * (1.0 + 0.4 * std::sin(static_cast<double>(No))));
					T.LateRegMinutes = Kind == 6 ? 30 : 150;
				}
				Temps.push_back(T);
			}
		}
		// High Roller on Saturday, October 10.
		EventTemplate Hr;
		Hr.Id = "mm-hr";
		Hr.Series = "mm";
		Hr.EventNo = No;
		Hr.Name = "MM #" + std::to_string(No) + ": $109 High Roller";
		Hr.BuyInCents = Cents(109.0);
		Hr.GtdCents = Cents(250000.0);
		Hr.Fmt = Format::ReEntry;
		Hr.LevelMinutes = 12.0;
		Hr.StartStack = 50000;
		Hr.Field = 2600;
		Hr.OnlyDay = 5;
		Hr.StartMinute = At(18, 0);
		Hr.OpensHours = 24 * 10;
		Hr.Featured = true;
		Temps.push_back(Hr);
	}

	// RCOP 2026: seven events a day from October 18 to November 8.
	{
		const double Buys[9] = {5.50, 22.0, 55.0, 109.0, 215.0, 530.0, 1050.0, 2100.0, 5250.0};
		const char* Kinds[10] = {"Opener", "Bounty Hunter PKO", "Six-Max", "Turbo", "Omaha", "Mystery Bounty", "Deepstack", "Marathon", "Big Game", "Championship"};
		const int Times[7] = {At(11, 0), At(13, 0), At(15, 0), At(17, 0), At(18, 30), At(20, 0), At(21, 30)};
		int No = 1;
		for (int Day = 13; Day <= 34; ++Day)
		{
			for (int K = 0; K < 7; ++K, ++No)
			{
				EventTemplate T;
				T.Series = "rcop";
				T.EventNo = No;
				T.OnlyDay = Day;
				T.StartMinute = Times[K];
				T.OpensHours = 24 * 21;
				T.TableSize = 8;
				T.LateRegMinutes = 300;
				T.LevelMinutes = 12.0;
				T.StartStack = 50000;
				T.Fmt = Format::ReEntry;
				if (Day == 34 && K == 2)
				{
					T.Id = "rcop-main";
					T.Name = "RCOP #" + std::to_string(No) + ": $5,250 Main Event";
					T.BuyInCents = Cents(5250.0);
					T.GtdCents = Cents(2.5e7);
					T.LevelMinutes = 15.0;
					T.StartStack = 100000;
					T.Field = 5200;
					T.OpensHours = 24 * 40;
					T.Featured = true;
					T.Blurb = "The one that changes a life. $25,000,000 guaranteed; last year's champion won $4.1M.";
				}
				else if (Day == 33 && K == 2)
				{
					T.Id = "rcop-mini";
					T.Name = "RCOP #" + std::to_string(No) + ": $530 Mini Main Event";
					T.BuyInCents = Cents(530.0);
					T.GtdCents = Cents(5.0e6);
					T.Field = 10600;
					T.Featured = true;
				}
				else if (Day == 32 && K == 2)
				{
					T.Id = "rcop-micro";
					T.Name = "RCOP #" + std::to_string(No) + ": $55 Micro Main Event";
					T.BuyInCents = Cents(55.0);
					T.GtdCents = Cents(2.0e6);
					T.Field = 41000;
					T.Featured = true;
				}
				else
				{
					const int Kind = (No * 3 + K) % 10;
					const double B = Buys[(No * 5 + K * 2) % 9];
					T.Id = "rcop-" + std::to_string(No);
					T.Name = "RCOP #" + std::to_string(No) + ": " + BuyIn(Cents(B)) + " " + Kinds[Kind];
					T.BuyInCents = Cents(B);
					T.GtdCents = Cents(B * (B >= 2000.0 ? 180.0 : B >= 500.0 ? 600.0 : 1400.0));
					T.Fmt = Kind == 1 ? Format::Bounty : Kind == 5 ? Format::Mystery : Format::ReEntry;
					T.Omaha = Kind == 4;
					T.TableSize = Kind == 2 ? 6 : 8;
					T.Speed = Kind == 3 ? "Turbo" : Kind == 6 || Kind == 7 ? "Deep" : "Regular";
					T.LevelMinutes = Kind == 3 ? 6.0 : Kind == 6 || Kind == 7 ? 15.0 : 12.0;
					T.Field = static_cast<int>(static_cast<double>(T.GtdCents) / 100.0 / (B * 0.915) * 1.1);
				}
				Temps.push_back(T);
			}
		}
	}

	// Summer Slam, finished in September (its results are history).
	{
		EventTemplate T;
		T.Id = "slam-main";
		T.Series = "slam";
		T.EventNo = 82;
		T.Name = "SLAM #82: $1,050 Main Event";
		T.BuyInCents = Cents(1050.0);
		T.GtdCents = Cents(1.0e7);
		T.Fmt = Format::ReEntry;
		T.LevelMinutes = 15.0;
		T.StartStack = 100000;
		T.Field = 11200;
		T.OnlyDay = -30;
		T.StartMinute = At(14, 0);
		Temps.push_back(T);
	}
}

int Network::FindPlayer(const std::string& Name) const
{
	if (Living)
	{
		return Living->Find(Name);
	}
	const auto It = ByName.find(Name);
	return It == ByName.end() ? -1 : It->second;
}

const SeriesInfo* Network::FindSeries(const std::string& Id) const
{
	for (const SeriesInfo& S : AllSeries)
	{
		if (S.Id == Id)
		{
			return &S;
		}
	}
	return nullptr;
}

const SeriesInfo* Network::CurrentSeries(double Now) const
{
	const int Day = DayOf(Now);
	const SeriesInfo* Next = nullptr;
	for (const SeriesInfo& S : AllSeries)
	{
		if (Day >= S.FirstDay && Day <= S.LastDay)
		{
			return &S;
		}
		if (S.FirstDay > Day && (!Next || S.FirstDay < Next->FirstDay))
		{
			Next = &S;
		}
	}
	return Next;
}

EventInstance Network::Make(int TemplateIndex, double Start) const
{
	const EventTemplate& T = Temps[static_cast<size_t>(TemplateIndex)];
	EventInstance E;
	E.Template = TemplateIndex;
	E.Start = Start;
	E.Id = T.Id + "@" + std::to_string(static_cast<long long>(std::llround(Start)));
	E.Seed = Fnv1a(E.Id);
	Rng R(E.Id);
	if (T.JoinIndex >= 0)
	{
		const TournamentSpec& Sp = Lobby()[static_cast<size_t>(T.JoinIndex)].Spec;
		E.Entries = Sp.Entrants;
		E.Pool = std::max(Sp.GuaranteeCents, (Sp.BuyInCents - Sp.FeeCents) * static_cast<Chips>(Sp.Entrants));
	}
	else
	{
		const bool Series = !T.Series.empty();
		const double Factor = Series ? R.Range(0.9, 1.15) : DayFactor(DayOf(Start)) * R.Range(0.82, 1.25);
		const Chips Contribution = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * PrizeShare(T)));
		// Guarantees are set to be beaten: the usual field clears one by a fifth, so overlays are rare.
		double Field = static_cast<double>(T.Field);
		if (T.GtdCents > 0 && Contribution > 0)
		{
			Field = std::max(Field, 1.2 * static_cast<double>(T.GtdCents) / static_cast<double>(Contribution));
		}
		E.Entries = std::max(8, static_cast<int>(std::round(Field * Factor)));
		E.Pool = T.BuyInCents == 0 ? T.GtdCents : std::max(T.GtdCents, Contribution * static_cast<Chips>(E.Entries));
	}
	const double Speed = T.Fmt == Format::Flip ? 0.25 : 1.0;
	E.Duration = T.LevelMinutes * (14.0 + 3.2 * std::log2(static_cast<double>(E.Entries) / static_cast<double>(T.TableSize) + 1.0)) * Speed;
	E.LateReg = std::min(static_cast<double>(T.LateRegMinutes), E.Duration * 0.55);
	return E;
}

void Network::Instances(int Day, std::vector<EventInstance>& Out) const
{
	auto Found = DayCache.find(Day);
	if (Found == DayCache.end())
	{
		std::vector<EventInstance> List;
		const int Wd = Weekday(Day);
		for (size_t I = 0; I < Temps.size(); ++I)
		{
			const EventTemplate& T = Temps[I];
			if (T.OnlyDay != -9999)
			{
				if (T.OnlyDay != Day)
				{
					continue;
				}
			}
			else if (!(T.Days & (1 << Wd)))
			{
				continue;
			}
			const double DayStart = static_cast<double>(Day) * MinutesPerDay;
			if (T.EveryMinutes > 0)
			{
				// Repeats run through the night: a last start before the first means it wraps past midnight.
				const int Last = T.LastMinute >= T.StartMinute ? T.LastMinute : T.LastMinute + 1440;
				for (int M = T.StartMinute; M <= Last && M < T.StartMinute + 1440; M += T.EveryMinutes)
				{
					const int InDay = M % 1440;
					const double Start = DayStart + static_cast<double>(M >= 1440 ? InDay + 1440 : InDay);
					if (Start < DayStart + MinutesPerDay)
					{
						List.push_back(Make(static_cast<int>(I), Start));
					}
				}
				if (T.LastMinute < T.StartMinute)
				{
					// The early-morning repeats of yesterday's run.
					for (int M = T.StartMinute - 1440; M <= T.LastMinute; M += T.EveryMinutes)
					{
						if (M >= 0)
						{
							List.push_back(Make(static_cast<int>(I), DayStart + static_cast<double>(M)));
						}
					}
				}
			}
			else
			{
				List.push_back(Make(static_cast<int>(I), DayStart + static_cast<double>(T.StartMinute)));
			}
		}
		std::sort(List.begin(), List.end(), [](const EventInstance& A, const EventInstance& B) { return A.Start < B.Start || (A.Start == B.Start && A.Id < B.Id); });
		Found = DayCache.emplace(Day, std::move(List)).first;
	}
	Out.insert(Out.end(), Found->second.begin(), Found->second.end());
}

std::vector<EventInstance> Network::Window(double From, double To) const
{
	std::vector<EventInstance> All;
	for (int Day = DayOf(From); Day <= DayOf(To); ++Day)
	{
		Instances(Day, All);
	}
	std::vector<EventInstance> Out;
	for (EventInstance& E : All)
	{
		if (E.Start >= From && E.Start < To)
		{
			Out.push_back(std::move(E));
		}
	}
	return Out;
}

LiveState Network::Live(const EventInstance& E, double Now) const
{
	const EventTemplate& T = TemplateOf(E);
	LiveState L;
	const double Opens = E.Start - static_cast<double>(T.OpensHours) * 60.0;
	const double Since = Now - E.Start;
	L.StartsIn = -Since;
	Rng R(E.Id + "/live");
	const double Wobble = R.Range(0.9, 1.1);
	if (Now < Opens)
	{
		L.St = Status::Announced;
		L.RegOpensIn = Opens - Now;
		L.Pool = std::max(T.GtdCents, static_cast<Chips>(0));
		return L;
	}
	const Chips PerEntry = T.BuyInCents == 0 ? 0 : E.Pool / std::max(1, E.Entries);
	auto PoolFor = [&](int Entries) { return T.BuyInCents == 0 ? E.Pool : std::max(T.GtdCents, PerEntry * static_cast<Chips>(Entries)); };
	if (Since < 0.0)
	{
		// Registration builds slowly, then rushes in the last hour.
		const double Span = std::max(1.0, E.Start - Opens);
		const double X = Clamp((Now - Opens) / Span, 0.0, 1.0);
		const double Share = 0.5 * Wobble * (0.25 * X + 0.75 * std::pow(X, 6.0));
		L.St = Status::Registering;
		L.Entries = std::max(0, static_cast<int>(std::round(static_cast<double>(E.Entries) * Share)));
		L.Pool = PoolFor(L.Entries);
		L.Overlay = false;
		return L;
	}
	const double Late = E.LateReg;
	L.Level = static_cast<int>(std::floor(Since / std::max(1.0, T.LevelMinutes))) + 1;
	L.Progress = Clamp(Since / std::max(1.0, E.Duration), 0.0, 1.0);
	if (Since >= E.Duration)
	{
		L.St = Status::Finished;
		L.Entries = E.Entries;
		L.Pool = E.Pool;
		L.Left = 0;
		L.Progress = 1.0;
		L.Overlay = T.BuyInCents > 0 && PerEntry * static_cast<Chips>(E.Entries) < T.GtdCents;
		return L;
	}
	if (Since < Late)
	{
		const double X = Clamp(Since / std::max(1.0, Late), 0.0, 1.0);
		const double Share = std::min(1.0, 0.5 * Wobble + (1.0 - 0.5 * Wobble) * std::pow(X, 0.7));
		L.St = Status::LateReg;
		L.Entries = static_cast<int>(std::round(static_cast<double>(E.Entries) * Share));
		L.LateRegLeft = Late - Since;
	}
	else
	{
		L.St = Status::Running;
		L.Entries = E.Entries;
	}
	L.Pool = PoolFor(L.Entries);
	L.Overlay = T.BuyInCents > 0 && Since >= Late && PerEntry * static_cast<Chips>(L.Entries) < T.GtdCents;
	// The field falls away slowly, then fast after the bubble.
	const double Remain = std::pow(1.0 - L.Progress, 1.7);
	L.Left = std::max(2, static_cast<int>(std::round(static_cast<double>(L.Entries) * Remain * 0.94)));
	if (L.Progress > 0.86 || L.Left <= T.TableSize)
	{
		L.St = Status::FinalTable;
		L.Left = std::max(2, std::min(T.TableSize, static_cast<int>(std::ceil(static_cast<double>(T.TableSize) * (1.0 - L.Progress) / 0.14))));
	}
	return L;
}

void Network::Attach(const world::World* W)
{
	Living = W;
	LivingRev = -1;
	RankCache.clear();
	Unknown.clear();
}

const std::vector<Player>& Network::LivingRows() const
{
	return Living->View();
}

const EventResult& Network::Result(const EventInstance& E) const
{
	if (Living)
	{
		if (const EventResult* R = Living->ResultOf(E.Id))
		{
			return *R;
		}
		// Before the world began, the network's own past; after, an event nobody the world follows played.
		return E.Start + E.Duration <= Living->StartedAt() ? Deterministic(E) : Unknowns(E);
	}
	return Deterministic(E);
}

const EventResult& Network::Unknowns(const EventInstance& E) const
{
	auto Found = Unknown.find(E.Id);
	if (Found != Unknown.end())
	{
		return Found->second;
	}
	if (Unknown.size() > 4096)
	{
		Unknown.clear();
	}
	const EventTemplate& T = TemplateOf(E);
	const int Places = std::min(T.TableSize >= 8 ? 9 : T.TableSize, std::max(2, E.Entries));
	const std::vector<Chips> Pay = Payouts(E);
	auto HeroIt = HeroFinishes.find(E.Id);
	const int HeroPlace = HeroIt != HeroFinishes.end() ? HeroIt->second.first : 0;
	Rng R(E.Id + "/unknowns");
	EventResult Res;
	for (int Place = 1; Place <= Places; ++Place)
	{
		Placing P;
		P.Place = Place;
		P.Prize = static_cast<size_t>(Place - 1) < Pay.size() ? Pay[static_cast<size_t>(Place - 1)] : 0;
		if (Place == HeroPlace)
		{
			P.Player = -1;
			P.Prize = HeroIt->second.second;
		}
		else
		{
			P.Player = -2;
			P.Country = handles::PickCountry(R);
			P.Name = handles::Make(R, P.Country, TierOf(T.BuyInCents) >= Tier::Mid);
			if (Living->Find(P.Name) >= 0)
			{
				P.Name += std::to_string(R.Int(90) + 10);
			}
		}
		Res.FinalTable.push_back(P);
	}
	return Unknown.emplace(E.Id, std::move(Res)).first->second;
}

const EventResult& Network::Deterministic(const EventInstance& E) const
{
	auto Found = Results.find(E.Id);
	if (Found != Results.end())
	{
		return Found->second;
	}
	const EventTemplate& T = TemplateOf(E);
	std::vector<double> Weight = TierWeights[static_cast<size_t>(TierOf(T.BuyInCents))];
	double Total = 0.0;
	for (double W : Weight)
	{
		Total += W;
	}
	Rng R(E.Id + "/result");
	const int Places = std::min(T.TableSize >= 8 ? 9 : T.TableSize, std::max(2, E.Entries));
	const std::vector<Chips> Pay = Payouts(E);
	auto HeroIt = HeroFinishes.find(E.Id);
	const int HeroPlace = HeroIt != HeroFinishes.end() ? HeroIt->second.first : 0;
	EventResult Res;
	for (int Place = 1; Place <= Places; ++Place)
	{
		Placing P;
		P.Place = Place;
		P.Prize = static_cast<size_t>(Place - 1) < Pay.size() ? Pay[static_cast<size_t>(Place - 1)] : 0;
		if (Place == HeroPlace)
		{
			P.Player = -1;
			P.Prize = HeroIt->second.second;
			Res.FinalTable.push_back(P);
			continue;
		}
		double Pick = R.Next() * Total;
		size_t Chosen = 0;
		for (size_t I = 0; I < Weight.size(); ++I)
		{
			if (Weight[I] > 0.0)
			{
				Chosen = I;
				if ((Pick -= Weight[I]) < 0.0)
				{
					break;
				}
			}
		}
		Total -= Weight[Chosen];
		Weight[Chosen] = 0.0;
		P.Player = static_cast<int>(Chosen);
		Res.FinalTable.push_back(P);
	}
	return Results.emplace(E.Id, std::move(Res)).first->second;
}

void Network::Prewarm(double Now) const
{
	// With a living world, only the network's own past needs working out.
	const double To = Living ? std::min(Now, Living->StartedAt()) : Now;
	for (const EventInstance& E : Finished(static_cast<double>(SimFirstDay) * MinutesPerDay, To))
	{
		Deterministic(E);
	}
}

std::vector<Chips> Network::Payouts(const EventInstance& E) const
{
	const EventTemplate& T = TemplateOf(E);
	const Chips MinCash = T.BuyInCents == 0 ? 1 : T.BuyInCents * 3 / 2;
	return PayoutTable(E.Pool, E.Entries, MinCash);
}

LobbyEvent Network::Listing(const EventInstance& E, std::string* Lock, int Unlocks) const
{
	const EventTemplate& T = TemplateOf(E);
	const double Day1 = static_cast<double>(NightOneDay) * MinutesPerDay;
	LobbyEvent L;
	if (T.JoinIndex >= 0)
	{
		L = Lobby()[static_cast<size_t>(T.JoinIndex)];
	}
	else
	{
		L.Name = T.Name;
		L.BuyInCents = T.BuyInCents;
		L.BuyInLabel = BuyIn(T.BuyInCents);
		L.Game = T.Omaha ? "PL Omaha" : T.Fmt == Format::Bounty ? "NL Hold'em PKO" : T.Fmt == Format::Satellite ? "Satellite" : "NL Hold'em";
		L.Speed = T.Speed + " \xC2\xB7 " + std::to_string(static_cast<int>(T.LevelMinutes)) + " min";
		L.Blurb = T.Blurb;
		L.Joinable = Joinable(E, nullptr, Unlocks);
		TournamentSpec& Sp = L.Spec;
		Sp.Id = T.Id;
		Sp.Name = !T.Series.empty() ? T.Name : BuyIn(T.BuyInCents) + " " + T.Name;
		Sp.BuyInCents = T.BuyInCents;
		Sp.FeeCents = FeeOf(T.BuyInCents);
		Sp.GuaranteeCents = T.GtdCents;
		Sp.Entrants = E.Entries;
		Sp.StartingStack = T.StartStack;
		Sp.LevelMinutes = T.LevelMinutes;
		Sp.SecondsPerHand = T.Speed == "Hyper" ? 36.0 : 42.0;
		const Tier Tr = TierOf(T.BuyInCents);
		Sp.Population = Tr == Tier::Freeroll ? "freeroll" : Tr == Tier::Micro ? "micro" : Tr == Tier::Low ? "low" : "high";
		Sp.Speed = T.Speed;
		Sp.TableSize = T.TableSize;
		if (T.Fmt == Format::Bounty || T.Fmt == Format::Mystery)
		{
			// Half of the prize part of the buy-in goes on heads; the guarantee covers both halves.
			Sp.BountyCents = (Sp.BuyInCents - Sp.FeeCents) / 2;
			Sp.GuaranteeCents = T.GtdCents / 2;
			Sp.MysteryBounty = T.Fmt == Format::Mystery;
		}
		if (T.Fmt == Format::Satellite)
		{
			Sp.SeatValueCents = T.SeatValueCents;
			Sp.SeatTicket = T.SeatTicket;
		}
		// Deeper stacks start at 100/200 so the blinds keep pace (100 big blinds or more to start).
		if (T.StartStack > 10000)
		{
			for (Level Lv : StandardLevels())
			{
				Lv.Sb *= 2;
				Lv.Bb *= 2;
				Lv.Ante *= 2;
				Sp.Levels.push_back(Lv);
			}
		}
	}
	L.Spec.Id = E.Id;
	// Minutes after midnight on Night One (negative for yesterday's events still in late registration).
	L.Spec.StartClock = E.Start - Day1;
	L.Start = TimeLabel(E.Start);
	L.EntrantsLabel = Grouped(E.Entries);
	L.GuaranteeLabel = T.GtdCents > 0 ? MoneyShort(T.GtdCents) + " GTD" : MoneyShort(E.Pool) + " pool";
	L.Status = StatusName(Live(E, Day1 + 127.0).St);
	if (Lock)
	{
		Joinable(E, Lock, Unlocks);
	}
	return L;
}

bool Network::Joinable(const EventInstance& E, std::string* Lock, int Unlocks) const
{
	const EventTemplate& T = TemplateOf(E);
	// Tonight's story events always run. Otherwise the tables play Hold'em, full ring or six-max, with fields the
	// engine deals every hand of; bounties, satellites and six-max open up as the player's career does.
	const bool Bounty = T.Fmt == Format::Bounty || T.Fmt == Format::Mystery;
	const char* Why = T.JoinIndex >= 0 ? nullptr
		: T.Omaha ? "Pot-Limit Omaha unlocks later in your career."
		: T.Fmt == Format::Flip ? "Flip & Go unlocks later in your career."
		: E.Entries > 3000 ? "Fields this big unlock later in your career."
		: Bounty && !(Unlocks & UnlockBounty) ? "Cash in any tournament to unlock bounty events."
		: T.Fmt == Format::Satellite && !(Unlocks & UnlockSatellite) ? "Make a final table to unlock satellites."
		: T.TableSize < 8 && !(Unlocks & UnlockSixMax) ? "Win a tournament to unlock six-max."
		: nullptr;
	if (Lock)
	{
		*Lock = Why ? Why : "";
	}
	return Why == nullptr;
}

bool Network::FindInstance(const std::string& Id, EventInstance& Out) const
{
	const size_t At = Id.rfind('@');
	if (At == std::string::npos)
	{
		return false;
	}
	const double Start = std::atof(Id.c_str() + At + 1);
	std::vector<EventInstance> Day;
	Instances(DayOf(Start), Day);
	for (const EventInstance& E : Day)
	{
		if (E.Id == Id)
		{
			Out = E;
			return true;
		}
	}
	return false;
}

bool Network::Next(const std::string& TemplateId, double From, EventInstance& Out) const
{
	int Index = -1;
	for (size_t I = 0; I < Temps.size(); ++I)
	{
		if (Temps[I].Id == TemplateId)
		{
			Index = static_cast<int>(I);
			break;
		}
	}
	if (Index < 0)
	{
		return false;
	}
	const EventTemplate& T = Temps[static_cast<size_t>(Index)];
	const int First = T.OnlyDay != -9999 ? T.OnlyDay : DayOf(From);
	const int Last = T.OnlyDay != -9999 ? T.OnlyDay : First + 14;
	for (int Day = First; Day <= Last; ++Day)
	{
		std::vector<EventInstance> List;
		Instances(Day, List);
		for (EventInstance& E : List)
		{
			if (E.Template == Index && E.Start >= From)
			{
				Out = std::move(E);
				return true;
			}
		}
	}
	return false;
}

std::vector<EventInstance> Network::Upcoming(double Now, double Horizon) const
{
	std::vector<EventInstance> Out;
	for (EventInstance& E : Window(Now, Now + Horizon))
	{
		const EventTemplate& T = TemplateOf(E);
		// The majors: weekly and series events, not the featured dailies.
		const bool Daily = T.OnlyDay == -9999 && T.Days == 0x7f;
		if (T.Featured && T.JoinIndex < 0 && !Daily && E.Start > Now)
		{
			Out.push_back(std::move(E));
		}
	}
	return Out;
}

void Network::SetHero(const std::string& Name, const std::vector<HistoryEntry>& History)
{
	std::map<std::string, std::pair<int, Chips>> Finishes;
	for (const HistoryEntry& H : History)
	{
		if (!H.EventId.empty())
		{
			Finishes[H.EventId] = {H.Place, H.Prize};
		}
	}
	if (Name == YouName && Finishes == HeroFinishes)
	{
		return;
	}
	for (const auto& F : Finishes)
	{
		Results.erase(F.first);
	}
	for (const auto& F : HeroFinishes)
	{
		Results.erase(F.first);
	}
	YouName = Name;
	HeroFinishes = std::move(Finishes);
	RankCache.clear();
	TallyCache.clear();
}

std::vector<EventInstance> Network::Finished(double From, double To) const
{
	std::vector<EventInstance> All;
	// Deep events can run half a day past their start.
	for (int Day = DayOf(From) - 1; Day <= DayOf(To); ++Day)
	{
		Instances(Day, All);
	}
	std::vector<EventInstance> Out;
	for (EventInstance& E : All)
	{
		const double End = E.Start + E.Duration;
		const EventTemplate& T = TemplateOf(E);
		if (End >= From && End < To && T.Fmt != Format::Satellite && T.BuyInCents > 0)
		{
			Out.push_back(std::move(E));
		}
	}
	return Out;
}

Network::Totals Network::Tally(double From, double To, const std::string& SeriesId, bool MicroOnly) const
{
	const size_t N = People.size();
	Totals T;
	const double DayStart = std::floor(To / MinutesPerDay) * MinutesPerDay;
	double Rest = From;
	if (DayStart > From)
	{
		const std::string Key = std::to_string(std::llround(From)) + "/" + std::to_string(std::llround(DayStart)) + "/" + SeriesId + (MicroOnly ? "/micro" : "");
		auto Found = TallyCache.find(Key);
		if (Found == TallyCache.end())
		{
			Totals Base;
			Base.Points.assign(N, 0.0);
			Base.Money.assign(N, 0);
			Base.Wins.assign(N, 0);
			Base.FinalTables.assign(N, 0);
			Accumulate(Base, From, DayStart, SeriesId, MicroOnly);
			Found = TallyCache.emplace(Key, std::move(Base)).first;
		}
		T = Found->second;
		Rest = DayStart;
	}
	else
	{
		T.Points.assign(N, 0.0);
		T.Money.assign(N, 0);
		T.Wins.assign(N, 0);
		T.FinalTables.assign(N, 0);
	}
	Accumulate(T, Rest, To, SeriesId, MicroOnly);
	return T;
}

void Network::Accumulate(Totals& T, double From, double To, const std::string& SeriesId, bool MicroOnly) const
{
	if (To <= From)
	{
		return;
	}
	for (const EventInstance& E : Finished(From, To))
	{
		const EventTemplate& Tp = TemplateOf(E);
		if (!SeriesId.empty() && Tp.Series != SeriesId)
		{
			continue;
		}
		if (MicroOnly && TierOf(Tp.BuyInCents) != Tier::Micro)
		{
			continue;
		}
		for (const Placing& P : Deterministic(E).FinalTable)
		{
			if (P.Player < 0)
			{
				continue;
			}
			const size_t I = static_cast<size_t>(P.Player);
			T.Points[I] += Points(P.Place, E.Entries, Tp.BuyInCents);
			T.Money[I] += P.Prize;
			T.Wins[I] += P.Place == 1 ? 1 : 0;
			T.FinalTables[I] += 1;
		}
	}
}

void Network::FoundingTally(double To, std::vector<double>& Points, std::vector<Chips>& Money, std::vector<int>& Wins, std::vector<int>& FinalTables) const
{
	const Totals T = Tally(static_cast<double>(SimFirstDay) * MinutesPerDay, To, "", false);
	Points = T.Points;
	Money = T.Money;
	Wins = T.Wins;
	FinalTables = T.FinalTables;
}

Chips Network::NightShiftPrize(int Rank)
{
	static const Chips Top[10] = {25000, 15000, 10000, 7500, 6000, 5000, 4500, 4000, 3500, 3000};
	if (Rank >= 1 && Rank <= 10)
	{
		return Top[Rank - 1];
	}
	return Rank <= 20 && Rank > 10 ? 1650 : 0;
}

const Network::Ranking& Network::Ranked(Board B, double Now) const
{
	if (Living && Living->Revision() != LivingRev)
	{
		// The world moved on: yesterday's rankings of it are stale.
		LivingRev = Living->Revision();
		RankCache.clear();
	}
	const auto Key = std::make_pair(static_cast<int>(B), static_cast<long long>(std::floor(Now)));
	auto Found = RankCache.find(Key);
	if (Found != RankCache.end())
	{
		return Found->second;
	}
	if (RankCache.size() > 64)
	{
		RankCache.clear();
	}
	const double SimStart = static_cast<double>(SimFirstDay) * MinutesPerDay;
	if (Living)
	{
		// The living world keeps the boards itself (and where everyone stood a day, or an hour, ago).
		const size_t Count = Living->View().size();
		Ranking Rk;
		Rk.Values.assign(Count, 0.0);
		for (size_t I = 0; I < Count; ++I)
		{
			Rk.Values[I] = Living->BoardValue(B, static_cast<int>(I), Now);
		}
		Rk.Order.resize(Count);
		for (size_t I = 0; I < Count; ++I)
		{
			Rk.Order[I] = static_cast<int>(I);
		}
		std::stable_sort(Rk.Order.begin(), Rk.Order.end(), [&](int A, int C) { return Rk.Values[static_cast<size_t>(A)] > Rk.Values[static_cast<size_t>(C)]; });
		Rk.RankBefore.assign(Count, 0);
		for (size_t I = 0; I < Count; ++I)
		{
			Rk.RankBefore[I] = Living->RankBefore(B, static_cast<int>(I));
		}
		return RankCache.emplace(Key, std::move(Rk)).first->second;
	}
	const size_t N = People.size();
	auto Values = [&](double At) {
		std::vector<double> V(N, 0.0);
		switch (B)
		{
		case Board::Earnings:
		case Board::Season:
		case Board::Wins:
		case Board::FinalTables:
		{
			const Totals T = Tally(SimStart, At, "", false);
			for (size_t I = 0; I < N; ++I)
			{
				const Player& P = People[I];
				V[I] = B == Board::Earnings ? static_cast<double>(P.Earnings + P.SeasonEarnings + T.Money[I])
					: B == Board::Season ? P.SeasonPoints + T.Points[I]
					: B == Board::Wins ? static_cast<double>(P.SeasonWins + T.Wins[I])
					: static_cast<double>(P.SeasonFinalTables + T.FinalTables[I]);
			}
			break;
		}
		case Board::Series:
		{
			const SeriesInfo* S = CurrentSeries(At);
			if (S && DayOf(At) >= S->FirstDay)
			{
				V = Tally(static_cast<double>(S->FirstDay) * MinutesPerDay, At, S->Id, false).Points;
			}
			break;
		}
		case Board::NightShift:
		{
			const double Start = life::NightShiftStart(Now);
			if (At > Start)
			{
				V = Tally(Start, std::min(At, Start + 12.0 * 60.0), "", true).Points;
			}
			break;
		}
		case Board::Live: break; // a living world's board
		}
		return V;
	};
	auto Sorted = [&](const std::vector<double>& V) {
		std::vector<int> Order(N);
		for (size_t I = 0; I < N; ++I)
		{
			Order[I] = static_cast<int>(I);
		}
		std::stable_sort(Order.begin(), Order.end(), [&](int A, int C) { return V[static_cast<size_t>(A)] > V[static_cast<size_t>(C)]; });
		return Order;
	};
	Ranking Rk;
	Rk.Values = Values(Now);
	Rk.Order = Sorted(Rk.Values);
	// Rank a day earlier (an hour for the Night Shift) for the movement arrows.
	const std::vector<int> OrderBefore = Sorted(Values(Now - (B == Board::NightShift ? 60.0 : MinutesPerDay)));
	Rk.RankBefore.assign(N, 0);
	for (size_t K = 0; K < N; ++K)
	{
		Rk.RankBefore[static_cast<size_t>(OrderBefore[K])] = static_cast<int>(K) + 1;
	}
	return RankCache.emplace(Key, std::move(Rk)).first->second;
}

std::vector<BoardRow> Network::Leaderboard(Board B, double Now, const HeroStats& Hero, int Count, BoardRow* HeroRow) const
{
	const std::vector<Player>& Everyone = Players();
	const size_t N = Everyone.size();
	const Ranking& Rk = Ranked(B, Now);
	const std::vector<double>& V = Rk.Values;
	const std::vector<int>& Order = Rk.Order;
	const std::vector<int>& RankBefore = Rk.RankBefore;
	double HeroValue = B == Board::Earnings ? static_cast<double>(Hero.Earnings)
		: B == Board::Season ? Hero.SeasonPoints
		: B == Board::Wins ? static_cast<double>(Hero.SeasonWins)
		: B == Board::FinalTables ? static_cast<double>(Hero.SeasonFinalTables)
		: B == Board::Series ? Hero.SeriesPoints
		: B == Board::Live ? Hero.LivePoints
		: Hero.NightPoints;
	std::vector<BoardRow> Rows;
	int HeroRank = 0;
	int Rank = 1;
	for (size_t K = 0; K < N && static_cast<int>(Rows.size()) < Count; ++K)
	{
		const size_t I = static_cast<size_t>(Order[K]);
		if (HeroRank == 0 && HeroValue > 0.0 && HeroValue >= V[I])
		{
			HeroRank = Rank;
			BoardRow Hr;
			Hr.Rank = Rank++;
			Hr.Player = -1;
			Hr.Value = HeroValue;
			if (B == Board::NightShift)
			{
				Hr.Prize = NightShiftPrize(Hr.Rank);
			}
			Rows.push_back(Hr);
			if (static_cast<int>(Rows.size()) >= Count)
			{
				break;
			}
		}
		if (V[I] <= 0.0 && B != Board::Earnings)
		{
			break;
		}
		BoardRow Row;
		Row.Rank = Rank++;
		Row.Player = static_cast<int>(I);
		Row.Value = V[I];
		Row.Move = RankBefore[I] - Row.Rank;
		Row.Form = Everyone[I].Form;
		if (B == Board::NightShift)
		{
			Row.Prize = NightShiftPrize(Row.Rank);
		}
		Rows.push_back(Row);
	}
	if (HeroRow)
	{
		BoardRow Hr;
		Hr.Player = -1;
		Hr.Value = HeroValue;
		if (HeroRank > 0)
		{
			Hr.Rank = HeroRank;
		}
		else if (HeroValue <= 0.0)
		{
			Hr.Rank = 0; // unranked
		}
		else
		{
			// Among the regulars, or estimated across the whole network below them.
			int Above = 0;
			for (double X : V)
			{
				Above += X > HeroValue ? 1 : 0;
			}
			double Floor = 1e18;
			for (double X : V)
			{
				if (X > 0.0)
				{
					Floor = std::min(Floor, X);
				}
			}
			if (HeroValue >= Floor || B == Board::NightShift || B == Board::Series)
			{
				Hr.Rank = Above + 1;
			}
			else
			{
				const double Tail = NetworkPlayers - static_cast<double>(N);
				Hr.Rank = static_cast<int>(static_cast<double>(N) + Tail * (1.0 - std::pow(HeroValue / Floor, 0.35)));
			}
		}
		if (B == Board::NightShift)
		{
			Hr.Prize = NightShiftPrize(Hr.Rank);
		}
		*HeroRow = Hr;
	}
	return Rows;
}

std::vector<NewsItem> Network::News(double Now, const HeroStats& Hero, int Count) const
{
	std::vector<NewsItem> Items;
	auto Post = [&](double At, NewsKind K, const std::string& Title, const std::string& Body, const std::string& Tag, Chips Amount = 0, int PlayerIndex = -1) {
		if (At <= Now)
		{
			NewsItem It;
			It.At = At;
			It.Kind = K;
			It.Title = Title;
			It.Body = Body;
			It.Tag = Tag;
			It.Amount = Amount;
			It.Player = PlayerIndex;
			Items.push_back(It);
		}
	};
	auto DayAt = [](int Day, int H, int M) { return static_cast<double>(Day) * MinutesPerDay + static_cast<double>(H * 60 + M); };
	Post(DayAt(-30, 23, 40), NewsKind::Series, std::string(RivalName) + " wins Summer Slam #41",
		"The night-shift regular takes down the $22 Six-Max for $18,406, his biggest score to date. \"The micros are a job. I clock in.\"", "SUMMER SLAM", Cents(18406.0), Rival);
	Post(DayAt(-7, 12, 0), NewsKind::Schedule, "Mystery Madness goes nightly",
		"Every night at 10:00 PM: $10.80 to enter, $40,000 guaranteed, mystery bounties up to $10,000 in the envelopes.", "SCHEDULE");
	Post(DayAt(-6, 10, 0), NewsKind::Series, "RCOP 2026: the schedule",
		"154 events. $100,000,000 guaranteed. October 18 to November 8. Satellites run every day from $2.20.", "RCOP 2026");
	Post(DayAt(-5, 15, 0), NewsKind::Series, "Micro Madness starts Thursday",
		"Seventy events from 55\xC2\xA2 with $5,000,000 guaranteed. The Main Event on October 11: $11 to play for $1,000,000.", "MICRO MADNESS");
	Post(DayAt(-1, 23, 55), NewsKind::Record, "Record night for the Sunday Showdown",
		"7,412 entries crushed the $1,000,000 guarantee: a $1.47M prize pool and $214,880 to the winner.", "RECORD");
	Post(DayAt(0, 18, 0), NewsKind::Schedule, "RCOP Main Event guarantee raised to $25M",
		"Record satellite numbers push the guarantee up from $20M. The steps to a $5,250 seat start at $2.20.", "RCOP 2026");
	Post(DayAt(1, 1, 30), NewsKind::Series, "Micro Madness, day six: the Night Crawler is running",
		"Event #26 at 1:30 AM: $2.20 buy-in, $2,000 guaranteed, late registration until 3:10 AM. Title, trophy and leaderboard points to the winner.", "MICRO MADNESS");
	// The biggest results of the last three days.
	for (const EventInstance& E : Finished(Now - 3.0 * MinutesPerDay, Now))
	{
		const EventTemplate& T = TemplateOf(E);
		const EventResult& R = Result(E);
		if (R.FinalTable.empty())
		{
			continue;
		}
		const Placing& W = R.FinalTable.front();
		const bool Series = !T.Series.empty();
		if (W.Prize < Cents(Series ? 400.0 : 9000.0) && W.Player != -1)
		{
			continue;
		}
		auto NameOf = [&](const Placing& P) { return P.Player == -1 ? YouName : P.Player == -2 ? P.Name : Players()[static_cast<size_t>(P.Player)].Name; };
		const std::string EventName = T.Name.find("MM #") == 0 || T.Name.find("RCOP #") == 0 ? T.Name : BuyIn(T.BuyInCents) + " " + T.Name;
		Post(E.Start + E.Duration, W.Player == -1 ? NewsKind::Hero : NewsKind::BigWin, NameOf(W) + " wins " + EventName,
			Grouped(E.Entries) + " entries \xC2\xB7 " + MoneyShort(E.Pool) + " prize pool. Runner-up: " + NameOf(R.FinalTable[1]) + " (" + Money(R.FinalTable[1].Prize) + ").",
			W.Player == -1 ? "YOU" : Series ? (T.Series == "mm" ? "MICRO MADNESS" : "SERIES") : "BIG WIN", W.Prize, W.Player == -2 ? -1 : W.Player);
		if (W.Player == -2)
		{
			Items.back().Player = -2;
		}
	}
	// A living world's people: live titles, careers, comebacks, retirements (its online wins are above).
	if (Living)
	{
		const std::vector<world::WorldEvent>& Log = Living->Events();
		for (auto It = Log.rbegin(); It != Log.rend() && It->At > Now - 4.0 * MinutesPerDay; ++It)
		{
			const bool OnlineWin = (It->Kind == world::EventKind::Won || It->Kind == world::EventKind::Champion) && (It->Flags & world::FlagLive) == 0;
			std::string Title;
			std::string Body;
			std::string Tag;
			if (OnlineWin || It->At > Now || !Living->Headline(*It, Title, Body, Tag))
			{
				continue;
			}
			const bool Win = It->Kind == world::EventKind::Won || It->Kind == world::EventKind::Champion || It->Kind == world::EventKind::Discovered || It->Kind == world::EventKind::Breakout;
			const NewsKind K = Win ? NewsKind::BigWin : It->Kind == world::EventKind::PlayerOfYear || It->Kind == world::EventKind::Milestone ? NewsKind::Record : NewsKind::People;
			Post(It->At, K, Title, Body, Tag, It->Amount, It->Npc);
		}
	}
	// The player's own milestones (when no result of theirs made the news).
	const bool HeroNews = std::any_of(Items.begin(), Items.end(), [](const NewsItem& It) { return It.Kind == NewsKind::Hero; });
	if (!HeroNews && Hero.Wins > 0)
	{
		Post(Now - 1.0, NewsKind::Hero, Hero.Name + " takes a title", "A first tournament win on RiverLine. The regulars have started to notice the name.", "YOU");
	}
	else if (!HeroNews && Hero.FinalTables > 0)
	{
		Post(Now - 1.0, NewsKind::Hero, Hero.Name + " reaches a final table", "Nine left, and one of them is you. It's a start.", "YOU");
	}
	std::stable_sort(Items.begin(), Items.end(), [](const NewsItem& A, const NewsItem& B) { return A.At > B.At; });
	if (static_cast<int>(Items.size()) > Count)
	{
		Items.resize(static_cast<size_t>(Count));
	}
	return Items;
}

Network& Shared()
{
	static Network N;
	return N;
}
} // namespace net
} // namespace ss
