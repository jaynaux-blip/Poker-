// The living world's labels and its live calendar: the local weeklies, the back-room games, the regional
// festivals, the Grand Circuit's stops, the summer Championship and the Summit. The calendar is the same in
// every save (it's the world's schedule); who plays it, and how they do, is each save's own.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace world
{
namespace worldcal_detail
{
/** Days since 1970-01-01 for a civil date (proleptic Gregorian). */
int CivilDays(int Y, int M, int D)
{
	Y -= M <= 2 ? 1 : 0;
	const int Era = (Y >= 0 ? Y : Y - 399) / 400;
	const int Yoe = Y - Era * 400;
	const int Doy = (153 * (M + (M > 2 ? -3 : 9)) + 2) / 5 + D - 1;
	const int Doe = Yoe * 365 + Yoe / 4 - Yoe / 100 + Doy;
	return Era * 146097 + Doe - 719468;
}

/** World day 0 is Monday, October 5, 2026. */
int Epoch()
{
	static const int E = CivilDays(2026, 10, 5);
	return E;
}

void CivilFrom(int Serial, int& Y, int& M, int& D)
{
	const int Z = Serial + 719468;
	const int Era = (Z >= 0 ? Z : Z - 146096) / 146097;
	const int Doe = Z - Era * 146097;
	const int Yoe = (Doe - Doe / 1460 + Doe / 36524 - Doe / 146096) / 365;
	const int Doy = Doe - (365 * Yoe + Yoe / 4 - Yoe / 100);
	const int Mp = (5 * Doy + 2) / 153;
	D = Doy - (153 * Mp + 2) / 5 + 1;
	M = Mp + (Mp < 10 ? 3 : -9);
	Y = Yoe + Era * 400 + (M <= 2 ? 1 : 0);
}

int WeekdayOf(int Day)
{
	return ((Day % 7) + 7) % 7; // 0 Monday
}

std::string Slug(const std::string& S)
{
	std::string Out;
	for (char C : S)
	{
		if ((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9'))
		{
			Out += C;
		}
		else if (C >= 'A' && C <= 'Z')
		{
			Out += static_cast<char>(C - 'A' + 'a');
		}
		else if (!Out.empty() && Out.back() != '-')
		{
			Out += '-';
		}
	}
	while (!Out.empty() && Out.back() == '-')
	{
		Out.pop_back();
	}
	return Out;
}

LiveEvent Make(const std::string& Name, const std::string& City, const std::string& Series, Region Where, LiveKind Kind, LiveLevel Level, double Dollars, int Field, int Day, int StartMinute,
	double Hours)
{
	LiveEvent E;
	E.Name = Name;
	E.City = City;
	E.Series = Series;
	E.Where = Where;
	E.Kind = Kind;
	E.Level = Level;
	E.BuyIn = static_cast<Chips>(std::llround(Dollars * 100.0));
	E.Field = Field;
	E.Start = static_cast<double>(Day) * 1440.0 + static_cast<double>(StartMinute);
	E.Duration = Hours * 60.0;
	E.Id = Slug(Series.empty() ? Name : Series + " " + Name) + "@" + std::to_string(static_cast<long long>(std::llround(E.Start)));
	return E;
}

/** A field size that moves a little from week to week (the same number in every save). */
int Vary(int Base, int Day, int Salt, int Spread)
{
	const uint32_t H = Fnv1a(std::to_string(Day) + ":" + std::to_string(Salt));
	return Base + static_cast<int>(H % static_cast<uint32_t>(std::max(1, Spread)));
}

struct Stop
{
	int Doy;
	const char* City;
	Region Where;
};
// The Grand Circuit's seven stops a year (day of the year they open).
const Stop CircuitStops[7] = {{18, "Melbourne", Region::AsiaPacific}, {60, "London", Region::Europe}, {102, "Atlantic City", Region::Americas}, {230, "Vienna", Region::Europe},
	{262, "Montreal", Region::Americas}, {294, "Macau", Region::AsiaPacific}, {326, "S\xC3\xA3o Paulo", Region::Americas}};
// The summer Championship in Las Vegas: bracelet events from June 1, the Main Event from June 24.
const int ChampionshipFirstDoy = 151;
const int ChampionshipDays = 36;
const int ChampionshipMainDoy = 174;
// The Summit, mid-December.
const int SummitDoy = 345;
} // namespace worldcal_detail

using namespace worldcal_detail;

namespace sim
{
int MonthKey(int Day)
{
	int Y = 0;
	int M = 0;
	int D = 0;
	CivilFrom(Epoch() + Day, Y, M, D);
	return Y * 12 + (M - 1);
}

int DayOfYear(int Day)
{
	return Day - YearStart(YearOf(Day));
}

int MonthDay(int Day)
{
	int Y = 0;
	int M = 0;
	int D = 0;
	CivilFrom(Epoch() + Day, Y, M, D);
	return D;
}
} // namespace sim

int YearOf(int Day)
{
	int Y = 0;
	int M = 0;
	int D = 0;
	CivilFrom(Epoch() + Day, Y, M, D);
	return Y;
}

int YearStart(int Year)
{
	return CivilDays(Year, 1, 1) - Epoch();
}

void CivilDate(int Day, int& Year, int& Month, int& DayOfMonth)
{
	CivilFrom(Epoch() + Day, Year, Month, DayOfMonth);
}

int DayOn(int Year, int Month, int DayOfMonth)
{
	return CivilDays(Year, Month, DayOfMonth) - Epoch();
}

const char* SkillName(Skill S)
{
	static const char* const Names[SkillCount] = {"Preflop", "Postflop", "Aggression", "Bluffing", "Value betting", "ICM", "Short stack", "Deep stack", "Heads-up", "Bounties",
		"Satellites", "Live reads", "Adjusting", "Emotional control"};
	const int I = static_cast<int>(S);
	return I >= 0 && I < SkillCount ? Names[I] : "";
}

const char* ArrivalName(Arrival A)
{
	switch (A)
	{
	case Arrival::None: return "None";
	case Arrival::FirstTimer: return "First timer";
	case Arrival::CameOfAge: return "Came of age";
	case Arrival::SiteClosed: return "Site closed";
	case Arrival::LiveCrossover: return "Live crossover";
	case Arrival::Comeback: return "Comeback";
	case Arrival::HomeGame: return "Home game";
	case Arrival::Watched: return "Watched";
	case Arrival::Streamer: return "Streamer";
	case Arrival::Count: break;
	}
	return "";
}

const char* IdentityName(Identity I)
{
	static const char* const Names[static_cast<int>(Identity::Count)] = {"Online grinder", "Live regular", "Recreational player", "Young crusher", "Disciplined pro",
		"Satellite specialist", "Bounty specialist", "Streamer", "High-stakes gambler", "Bankroll nit", "Wealthy amateur", "Underground specialist", "Veteran pro",
		"Rising prospect"};
	const int K = static_cast<int>(I);
	return K >= 0 && K < static_cast<int>(Identity::Count) ? Names[K] : "";
}

const char* StatusName(Status S)
{
	switch (S)
	{
	case Status::Active: return "Active";
	case Status::Break: return "On a break";
	case Status::Broke: return "Broke";
	case Status::Retired: return "Retired";
	}
	return "";
}

const char* MomentumName(Momentum M)
{
	static const char* const Names[static_cast<int>(Momentum::Count)] = {"Normal", "Heater", "Downswing", "Major downswing", "Breakout", "Rebuilding", "Confident", "Burnout"};
	const int K = static_cast<int>(M);
	return K >= 0 && K < static_cast<int>(Momentum::Count) ? Names[K] : "";
}

const char* RepName(Rep R)
{
	static const char* const Names[RepCount] = {"Online", "Live", "Underground", "Streaming", "High stakes", "Overall"};
	const int K = static_cast<int>(R);
	return K >= 0 && K < RepCount ? Names[K] : "";
}

const char* PlanName(Plan P)
{
	static const char* const Names[static_cast<int>(Plan::Count)] = {"Grinds most days", "Plays nights", "Plays weekends", "Plays now and then", "Plays the series",
		"Plays the big ones", "Travels the live circuit"};
	const int K = static_cast<int>(P);
	return K >= 0 && K < static_cast<int>(Plan::Count) ? Names[K] : "";
}

const char* TieName(TieKind K)
{
	static const char* const Names[static_cast<int>(TieKind::Count)] = {"Friend", "Rival", "Training partner", "Backs", "Backed by", "Travel partner", "Stream collab",
		"Home-game regular"};
	const int I = static_cast<int>(K);
	return I >= 0 && I < static_cast<int>(TieKind::Count) ? Names[I] : "";
}

const char* MemoryText(MemoryKind K)
{
	switch (K)
	{
	case MemoryKind::Met: return "First met at the tables";
	case MemoryKind::KnockedOutHero: return "Knocked you out";
	case MemoryKind::HeroKnockedOut: return "You knocked them out";
	case MemoryKind::BigPotWon: return "You won a big pot from them";
	case MemoryKind::BigPotLost: return "They won a big pot from you";
	case MemoryKind::FinalTable: return "Final table together";
	case MemoryKind::HeadsUpWon: return "You beat them heads-up for the title";
	case MemoryKind::HeadsUpLost: return "They beat you heads-up for the title";
	case MemoryKind::BackRoom: return "A night at Dee's game";
	case MemoryKind::Embercrest: return "The Embercrest Sunday";
	case MemoryKind::ShowedBluff: return "Showed you a bluff";
	default: return "";
	}
}

const char* EventKindName(EventKind K)
{
	static const char* const Names[static_cast<int>(EventKind::Count)] = {"NPCWonTournament", "NPCReachedFinalTable", "NPCWentBroke", "NPCMovedUpStakes", "NPCMovedDownStakes",
		"NPCRetired", "NPCReturned", "NPCBreakout", "NPCQualifiedForMajor", "NPCEarnedSponsorship", "NPCReachedCareerMilestone", "NPCStartedStreaming", "NPCStreamMilestone",
		"NPCDebut", "NPCDiscovered", "NPCBacked", "NPCRivalry", "NPCChampion", "NPCPlayerOfTheYear", "NPCTookBreak"};
	const int I = static_cast<int>(K);
	return I >= 0 && I < static_cast<int>(EventKind::Count) ? Names[I] : "";
}

Region RegionOf(const std::string& Country)
{
	static const char* const Europe[] = {"GB", "DE", "NL", "SE", "FR", "ES", "IT", "RU", "UA", "PL", "IE", "FI", "AT", "PT", "BE", "DK", "NO", "CZ", "HU", "RO", "GR", "CH"};
	static const char* const Asia[] = {"JP", "KR", "AU", "PH", "IN", "CN", "NZ", "TH", "VN", "SG", "MY", "ID", "TW", "HK"};
	for (const char* C : Europe)
	{
		if (Country == C)
		{
			return Region::Europe;
		}
	}
	for (const char* C : Asia)
	{
		if (Country == C)
		{
			return Region::AsiaPacific;
		}
	}
	return Region::Americas;
}

std::vector<LiveEvent> LiveCalendar(int Day)
{
	std::vector<LiveEvent> Out;
	const int Wd = WeekdayOf(Day);
	int Y = 0;
	int M = 0;
	int D = 0;
	CivilFrom(Epoch() + Day, Y, M, D);
	const int Doy = Day - YearStart(Y);
	auto At = [](int H, int Mi) { return H * 60 + Mi; };

	// The Embercrest's card room, the one the player can take the bus to: two or three a day (live::Schedule).
	for (const live::Occurrence& O : live::Occurrences(Day))
	{
		LiveEvent E = Make(O.T->Name, "the Embercrest", "", Region::Americas, LiveKind::Local, LiveLevel::Local, static_cast<double>(O.T->BuyInCents) / 100.0, O.Field, Day,
			O.T->StartMinute, O.T->Hours);
		E.Id = O.Id;
		Out.push_back(E);
	}
	// The other local weeklies, in other cities.
	if (Wd == 3)
	{
		Out.push_back(Make("Lone Star Thursday $200", "Austin", "", Region::Americas, LiveKind::Local, LiveLevel::Local, 200.0, Vary(60, Day, 1, 30), Day, At(19, 0), 8.0));
	}
	if (Wd == 4)
	{
		Out.push_back(Make("Camden Friday $150", "London", "", Region::Europe, LiveKind::Local, LiveLevel::Local, 150.0, Vary(50, Day, 2, 30), Day, At(19, 30), 8.0));
	}
	if (Wd == 5)
	{
		Out.push_back(Make("Spree Saturday $200", "Berlin", "", Region::Europe, LiveKind::Local, LiveLevel::Local, 200.0, Vary(55, Day, 3, 35), Day, At(18, 0), 8.0));
		Out.push_back(Make("Harbour Saturday $150", "Sydney", "", Region::AsiaPacific, LiveKind::Local, LiveLevel::Local, 150.0, Vary(45, Day, 4, 25), Day, At(18, 0), 8.0));
	}
	if (Wd == 2)
	{
		Out.push_back(Make("Han River Wednesday $200", "Seoul", "", Region::AsiaPacific, LiveKind::Local, LiveLevel::Local, 200.0, Vary(40, Day, 5, 25), Day, At(19, 0), 8.0));
	}

	// The games that aren't on any calendar.
	auto Game = [&](const std::string& Name, const std::string& City, Region Where, double BigBlind, int Seats, int H) {
		LiveEvent E = Make(Name, City, "", Where, LiveKind::Underground, LiveLevel::Local, 0.0, Seats, Day, At(H, 0), 7.0);
		E.Stakes = static_cast<Chips>(std::llround(BigBlind * 100.0));
		Out.push_back(E);
	};
	if (Wd == 1)
	{
		// Dee's game, in the back room of the laundromat across the street (the player's own).
		Game("Dee's game", "the laundromat", Region::Americas, 2.0, 9, 21);
		Out.back().Id = "dees@" + std::to_string(Day);
	}
	if (Wd == 3)
	{
		Game("The Garage", "across the river", Region::Americas, 5.0, 9, 22);
	}
	if (Wd == 5)
	{
		Game("The Basement", "downtown", Region::Americas, 20.0, 8, 23);
		Game("The Parlor", "Macau", Region::AsiaPacific, 10.0, 8, 22);
	}
	if (Wd == 2)
	{
		Game("The Cellar", "London", Region::Europe, 5.0, 9, 22);
	}

	// Regional festivals, every month in each region: a deepstack, the main, a high roller.
	struct Fest
	{
		const char* Name;
		const char* City;
		Region Where;
		int FirstDate;
	};
	const Fest Fests[3] = {{"River City Classic", "River City", Region::Americas, 10}, {"Midlands Open", "Birmingham", Region::Europe, 17}, {"Pacific Cup", "Auckland", Region::AsiaPacific, 24}};
	for (const Fest& F : Fests)
	{
		const int K = D - F.FirstDate;
		if (K < 0 || K > 2)
		{
			continue;
		}
		const std::string Series = std::string(F.Name) + " " + std::to_string(Y);
		if (K == 0)
		{
			Out.push_back(Make("$350 Deepstack", F.City, Series, F.Where, LiveKind::Regional, LiveLevel::Regional, 350.0, Vary(260, Day, 10, 140), Day, At(12, 0), 12.0));
			if (M >= 4 && M <= 6)
			{
				// Summer: the Championship Main Event packages.
				LiveEvent S = Make("$330 Championship Satellite", F.City, Series, F.Where, LiveKind::Regional, LiveLevel::Regional, 330.0, Vary(150, Day, 11, 90), Day, At(20, 0), 6.0);
				S.Ticket = "championship-main";
				Out.push_back(S);
			}
		}
		if (K == 1)
		{
			LiveEvent E = Make("$550 Main Event", F.City, Series, F.Where, LiveKind::Regional, LiveLevel::Regional, 550.0, Vary(380, Day, 12, 260), Day, At(12, 0), 26.0);
			E.Main = true;
			Out.push_back(E);
		}
		if (K == 2)
		{
			Out.push_back(Make("$1,100 High Roller", F.City, Series, F.Where, LiveKind::Regional, LiveLevel::Regional, 1100.0, Vary(70, Day, 13, 50), Day, At(14, 0), 10.0));
		}
	}

	// The Grand Circuit: a stop every six to eight weeks, rings for every event.
	for (const Stop& S : CircuitStops)
	{
		const int K = Doy - S.Doy;
		if (K < 0 || K > 5)
		{
			continue;
		}
		const std::string Series = std::string("Grand Circuit ") + S.City + " " + std::to_string(Y);
		LiveEvent E;
		switch (K)
		{
		case 0: E = Make("$580 Opener", S.City, Series, S.Where, LiveKind::Circuit, LiveLevel::Circuit, 580.0, Vary(1100, Day, 20, 600), Day, At(12, 0), 14.0); break;
		case 1: E = Make("$1,100 Bounty", S.City, Series, S.Where, LiveKind::Circuit, LiveLevel::Circuit, 1100.0, Vary(600, Day, 21, 300), Day, At(12, 0), 13.0); break;
		case 2:
			E = Make("$1,700 Main Event", S.City, Series, S.Where, LiveKind::Circuit, LiveLevel::Circuit, 1700.0, Vary(950, Day, 22, 650), Day, At(12, 0), 54.0);
			E.Main = true;
			break;
		case 3: E = Make("$5,300 Championship", S.City, Series, S.Where, LiveKind::Circuit, LiveLevel::Circuit, 5300.0, Vary(150, Day, 23, 90), Day, At(13, 0), 24.0); break;
		case 4: E = Make("$10,300 High Roller", S.City, Series, S.Where, LiveKind::HighRoller, LiveLevel::HighRoller, 10300.0, Vary(65, Day, 24, 45), Day, At(13, 0), 20.0); break;
		default: E = Make("$25,500 Super High Roller", S.City, Series, S.Where, LiveKind::HighRoller, LiveLevel::HighRoller, 25500.0, Vary(28, Day, 25, 20), Day, At(14, 0), 18.0); break;
		}
		E.Ring = true;
		Out.push_back(E);
	}

	// The Championship: five weeks of bracelets in Las Vegas, the $10,000 Main Event, satellites every night.
	if (Doy >= ChampionshipFirstDoy && Doy < ChampionshipFirstDoy + ChampionshipDays)
	{
		const std::string Series = "The Championship " + std::to_string(Y);
		const int K = Doy - ChampionshipFirstDoy;
		struct Bracelet
		{
			double BuyIn;
			const char* Name;
			int Field;
		};
		static const Bracelet Events[] = {{1000, "$1,000 Mega Stack", 5200}, {1500, "$1,500 No-Limit Hold'em", 2300}, {3000, "$3,000 Six-Max", 1150},
			{1500, "$1,500 Bounty", 2400}, {5000, "$5,000 No-Limit Hold'em", 650}, {2500, "$2,500 Freezeout", 1300}, {10000, "$10,000 Heads-Up Championship", 120},
			{25000, "$25,000 High Roller", 190}, {1000, "$1,000 Seniors", 4100}, {3000, "$3,000 Turbo", 1000}, {50000, "$50,000 High Roller", 95},
			{1500, "$1,500 Monster Stack", 6200}, {10000, "$10,000 Short Deck", 110}, {100000, "$100,000 High Roller", 70}};
		if (K < 22 || (K >= 23 && K % 3 == 0))
		{
			const Bracelet& B = Events[static_cast<size_t>(K) % (sizeof(Events) / sizeof(Events[0]))];
			LiveEvent E = Make(B.Name, "Las Vegas", Series, Region::Americas, B.BuyIn >= 25000.0 ? LiveKind::HighRoller : LiveKind::Championship,
				B.BuyIn >= 25000.0 ? LiveLevel::HighRoller : LiveLevel::Championship, B.BuyIn, Vary(B.Field, Day, 30, B.Field / 5), Day, At(11, 0), B.Field > 2000 ? 40.0 : 26.0);
			E.Bracelet = true;
			Out.push_back(E);
		}
		if (Doy < ChampionshipMainDoy)
		{
			LiveEvent S = Make("$1,100 Main Event Satellite", "Las Vegas", Series, Region::Americas, LiveKind::Championship, LiveLevel::Regional, 1100.0, Vary(220, Day, 31, 120), Day,
				At(21, 0), 6.0);
			S.Ticket = "championship-main";
			Out.push_back(S);
		}
		if (Doy == ChampionshipMainDoy)
		{
			LiveEvent E = Make("$10,000 Main Event", "Las Vegas", Series, Region::Americas, LiveKind::ChampionshipMain, LiveLevel::Championship, 10000.0, Vary(8600, Day, 32, 2400),
				Day, At(11, 0), 11.0 * 24.0);
			E.Bracelet = true;
			E.Main = true;
			E.Id = "championship-main@" + std::to_string(static_cast<long long>(std::llround(E.Start)));
			Out.push_back(E);
		}
	}

	// The Summit: the year's best, by invitation, $1,000,000 a seat (most of them sell their action).
	if (Doy == SummitDoy)
	{
		LiveEvent E = Make("The Summit", "Monte Carlo", "The Summit " + std::to_string(Y), Region::Europe, LiveKind::Summit, LiveLevel::HighRoller, 1000000.0, 36, Day, At(15, 0), 3.0 * 24.0);
		E.Main = true;
		Out.push_back(E);
	}
	std::stable_sort(Out.begin(), Out.end(), [](const LiveEvent& A, const LiveEvent& B) { return A.Start < B.Start; });
	return Out;
}

std::vector<LiveEvent> LiveFestivals(int Day, int Span)
{
	std::vector<LiveEvent> Out;
	for (int K = Day; K < Day + Span; ++K)
	{
		for (LiveEvent& E : LiveCalendar(K))
		{
			if (!E.Series.empty())
			{
				Out.push_back(std::move(E));
			}
		}
	}
	return Out;
}
} // namespace world
} // namespace ss
