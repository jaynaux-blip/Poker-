// The people on the block (the realism update's spec, §7.8's town_test; people-core): the schedules over four weeks,
// the corner's signals and the 14's times, the crowd and the barber's queue, talk in ClerkLine's order, rumours told
// once, the keepers' lines, the save lines and their pruning, and the weather (shares, neighbors only, the look-back's
// cost, storms, wetness, Night One).
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace town_test
{
using namespace ss;

int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const std::string& What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

struct Hooks : SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Inbox;
	SaveData Last;
	void Text(const std::string& From, const std::string& Body) override { Inbox.emplace_back(From, Body); }
	void Save(const SaveData& D) override { Last = D; }
	bool Got(const std::string& From, const std::string& Part) const
	{
		for (const auto& M : Inbox)
		{
			if (M.first == From && M.second.find(Part) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}
};

hero::Character Someone(hero::Background B = hero::Background::DealersKid)
{
	hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Story = B;
	C.Created = true;
	return C;
}

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out(1);
	for (const char C : Line)
	{
		if (C == '\t')
		{
			Out.emplace_back();
		}
		else
		{
			Out.back() += C;
		}
	}
	return Out;
}

constexpr double Day = 1440.0;
double At(int D, int H, int M = 0)
{
	return static_cast<double>(D) * Day + H * 60 + M;
}

/** Moves a session's clock (and its world) forward to World. */
void Clock(Session& S, double World)
{
	S.LobbyMinutes = World - net::MinutesPerDay * net::NightOneDay;
	S.Living().AdvanceTo(World);
}

/** The first whole hour from From..To on day D when it isn't raining (and wasn't a quarter hour before): talk tests that
 *  check the order of a greeting do it out of the rain (a soaked player is remarked on first, rightly). */
double DryHour(const Session& S, int D, int From, int To)
{
	for (int H = From; H < To; ++H)
	{
		const double T = At(D, H);
		if (weather::At(T, S.WorldSeed()).Rain < 0.2 && weather::At(T - 15.0, S.WorldSeed()).Rain < 0.2)
		{
			return T;
		}
	}
	return At(D, From);
}

std::string Describe(const std::vector<town::Placement>& L)
{
	std::ostringstream O;
	for (const town::Placement& P : L)
	{
		O << P.Id << " " << P.Venue << " " << P.Station << " " << static_cast<int>(P.What) << " " << P.Since << "-" << P.Until << "\n";
	}
	return O.str();
}

bool WorldPerson(const std::string& Id)
{
	const town::Person* P = town::FindPerson(Id);
	return Id.rfind("npc:", 0) == 0 || (P && P->InWorld);
}

/** A seat or a spot that holds one person at a time (a stool, a booth, a place at the shelter). */
bool OneSeat(const std::string& Station)
{
	return Station.rfind("diner-stool", 0) == 0 || Station.rfind("diner-booth", 0) == 0 || Station.rfind("market-shelter", 0) == 0;
}

/** A day's plan: nobody in two places at once, and no seat taken by two. Returns how many problems it found. */
int Crowded(const std::vector<town::Placement>& Plan, const std::string& What)
{
	int Bad = 0;
	std::map<std::string, std::vector<std::pair<double, double>>> ByPerson;
	std::map<std::string, std::vector<std::pair<std::pair<double, double>, std::string>>> BySeat;
	for (const town::Placement& P : Plan)
	{
		ByPerson[P.Id].push_back({P.Since, P.Until});
		if (OneSeat(P.Station))
		{
			BySeat[P.Station].push_back({{P.Since, P.Until}, P.Id});
		}
	}
	for (auto& It : ByPerson)
	{
		std::sort(It.second.begin(), It.second.end());
		for (size_t K = 1; K < It.second.size(); ++K)
		{
			if (It.second[K].first < It.second[K - 1].second - 1e-9)
			{
				++Bad;
				Expect(false, "schedule: " + It.first + " is in one place at a time (" + What + ")");
			}
		}
	}
	for (auto& It : BySeat)
	{
		std::sort(It.second.begin(), It.second.end());
		for (size_t K = 1; K < It.second.size(); ++K)
		{
			if (It.second[K].second != It.second[K - 1].second && It.second[K].first.first < It.second[K - 1].first.second - 1e-9)
			{
				++Bad;
				Expect(false, "schedule: " + It.first + " holds one at a time (" + It.second[K - 1].second + " and " + It.second[K].second + ", " + What + ")");
			}
		}
	}
	return Bad;
}

/** The world's people in a Who, as where and what (the comparison a forecast is held to). */
std::set<std::string> WorldPeople(const std::vector<town::Placement>& Who)
{
	std::set<std::string> Out;
	for (const town::Placement& P : Who)
	{
		if (WorldPerson(P.Id))
		{
			Out.insert(P.Id + "@" + P.Venue + "/" + P.Station + "/" + std::to_string(static_cast<int>(P.What)));
		}
	}
	return Out;
}

// ------------------------------------------------------------------ the people

void People()
{
	std::set<std::string> Ids;
	for (const town::Person& P : town::People())
	{
		Expect(!P.Id.empty() && Ids.insert(P.Id).second, "people: ids are unique (" + P.Id + ")");
		Expect(town::FindPerson(P.Id) == &P, "people: found by id (" + P.Id + ")");
		Expect(!P.Name.empty() && P.Age >= 18 && P.Age <= 90, "people: a name and an age (" + P.Id + ")");
	}
	for (const char* Id : {"benny", "ray", "lonnie", "bea", "rosa", "ruby", "marlene", "augie", "cook", "dee", "inez", "marcus", "sal", "mei", "big-lou", "twitch", "mrs-park"})
	{
		Expect(town::FindPerson(Id) != nullptr, std::string("people: the cast has ") + Id);
	}
	Expect(town::FindPerson("augie")->Role == town::Job::Pawnbroker && town::FindPerson("augie")->Venue == "pawn", "people: Augie keeps the pawn");
	Expect(town::FindPerson("sal")->InWorld && !town::FindPerson("lonnie")->InWorld, "people: the world's people are marked");
	Expect(town::FindPerson("nobody") == nullptr, "people: no such person");
}

// ------------------------------------------------------------------ four weeks of schedules

void Schedules()
{
	Hooks H;
	Session S(H, "town-schedule");
	S.NewCareer(Someone());
	const int First = 2;  // Wednesday, October 7
	const int Days = 28;
	const std::vector<std::string> Keepers = {"lonnie", "bea", "rosa", "augie"};
	int Steps = 0;
	int StaffedChecks = 0;
	int GameNights = 0;
	int Seated = 0;
	int MostAtRubys = 0;
	int WorldSeen = 0;
	int Shelter = 0;
	int ChecksPicked = 0;
	std::set<std::string> RubysRegulars;
	double WhoSeconds = 0.0;
	int WhoCalls = 0;
	// What the plan said ten minutes ahead is what happens: nobody the world placed pops in or out when a result comes in
	// (people-ue builds people from the plan ahead, under black).
	std::vector<town::Placement> Forecast;
	double ForecastFor = -1.0;
	int Forecasts = 0;
	int Unforeseen = 0;
	for (int D = First; D < First + Days; ++D)
	{
		// The day's plan as the world has it planned: nobody is in two places at once, no seat taken twice.
		Clock(S, At(D, 0, 1));
		const std::vector<town::Placement> Plan = town::Plan(At(D, 0), At(D + 1, 0), S);
		for (const town::Placement& P : Plan)
		{
			Expect(P.Until > P.Since, "schedule: every stretch has a length (" + P.Id + ")");
		}
		Crowded(Plan, "day " + std::to_string(D));
		for (int Minute = 0; Minute < 1440; Minute += 10)
		{
			const double T = At(D, 0, Minute);
			Clock(S, T);
			const auto T0 = std::chrono::steady_clock::now();
			const std::vector<town::Placement> Who = town::Who(T, S);
			WhoSeconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
			++WhoCalls;
			++Steps;
			if (ForecastFor == T)
			{
				++Forecasts;
				const std::set<std::string> Said = WorldPeople(Forecast);
				const std::set<std::string> Is = WorldPeople(Who);
				if (Said != Is)
				{
					++Unforeseen;
					std::string Diff;
					for (const std::string& X : Said)
					{
						Diff += Is.count(X) ? "" : " -" + X;
					}
					for (const std::string& X : Is)
					{
						Diff += Said.count(X) ? "" : " +" + X;
					}
					Expect(false, "schedule: ten minutes ahead, the plan knew who'd be about (day " + std::to_string(D) + ", minute " + std::to_string(Minute) + ":" + Diff + ")");
				}
			}
			Forecast = town::Who(T + 10.0, S);
			ForecastFor = T + 10.0;
			std::set<std::string> Ids;
			int AtRubys = 0;
			for (const town::Placement& P : Who)
			{
				Expect(Ids.insert(P.Id).second, "schedule: one placement each");
				Expect(P.Since <= T && T < P.Until, "schedule: Who is now");
				if (WorldPerson(P.Id))
				{
					++WorldSeen;
					if (P.Venue == "diner")
					{
						++AtRubys;
						RubysRegulars.insert(P.Id);
					}
					Shelter += P.Venue == "market-stop" ? 1 : 0;
				}
			}
			MostAtRubys = std::max(MostAtRubys, AtRubys);
			// Dee's regulars are at the Wash & Fold at 21:30 on her nights.
			const int Wd = net::Weekday(D);
			if (Minute == 21 * 60 + 30 && (Wd == 1 || Wd == 3 || Wd == 5))
			{
				++GameNights;
				const std::vector<int> Reg = S.Living().RegisteredNow("dees@" + std::to_string(D));
				int Here = 0;
				for (const char* Name : {"Sal", "Mei", "Big Lou", "Twitch"})
				{
					const int Id = S.Living().Find(Name);
					if (std::find(Reg.begin(), Reg.end(), Id) == Reg.end())
					{
						continue;
					}
					const std::string Pid = Name == std::string("Big Lou") ? "big-lou" : Name == std::string("Sal") ? "sal" : Name == std::string("Mei") ? "mei" : "twitch";
					town::Placement P;
					const bool Found = town::Where(Pid, T, S, P);
					Expect(Found && P.Venue == "washfold" && P.What == town::Doing::Sit, std::string("dee's: ") + Name + " is at the table at 9:30 (day " + std::to_string(D) + ")");
					Here += Found ? 1 : 0;
				}
				Expect(Here >= 2, "dee's: her regulars come on every one of her nights (day " + std::to_string(D) + ", " + net::WeekdayName(D, true) + ")");
				Seated += Here;
			}
			Expect(AtRubys <= 2, "schedule: two of Ruby's regulars at most");
			// Every open venue is staffed while it's staffed, and only then.
			for (const shop::Venue& V : shop::Venues())
			{
				if (!V.Enterable)
				{
					continue;
				}
				const bool Staffed = shop::IsStaffed(V, T);
				const std::string Keeper = town::OnShift(V.Id, T);
				bool Working = false;
				for (const town::Placement& P : Who)
				{
					Working = Working || (P.Id == Keeper && P.Venue == V.Id && P.What == town::Doing::Work);
				}
				bool AnyStaff = false;
				for (const town::Placement& P : Who)
				{
					const town::Person* Pp = town::FindPerson(P.Id);
					AnyStaff = AnyStaff || (Pp && !Pp->InWorld && P.Venue == V.Id && P.What == town::Doing::Work && Pp->Role != town::Job::Cook && Pp->Role != town::Job::Host);
				}
				Expect(Staffed ? !Keeper.empty() && Working : Keeper.empty() && !AnyStaff, "schedule: " + V.Id + " is staffed exactly while it says (day " + std::to_string(D) + ", minute " + std::to_string(Minute) + ")");
				++StaffedChecks;
			}
			// Big Lou picks up the check on a Friday night when the player cashed lately.
			town::Placement Lou;
			if (town::Where("big-lou", T, S, Lou) && Lou.Venue == "diner" && Lou.What == town::Doing::Sit)
			{
				Expect(town::PaysYourCheck(S, T).empty(), "check: Lou pays only for a player who's cashed");
				life::LiveEntry E;
				E.Id = "embercrest-nightly@" + std::to_string(D - 2);
				E.State = life::LiveEntry::Finished;
				E.PrizeCents = 31000;
				E.FinishedAt = T - 2.0 * Day;
				S.Life.LiveEntries.push_back(E);
				Expect(town::PaysYourCheck(S, T) == "big-lou", "check: Big Lou picks it up");
				const std::string Key = town::CheckKey(S, T);
				Expect(Key == "check:big-lou:" + std::to_string(net::DayOf(Lou.Since)), "check: the night's key (" + Key + ")");
				S.Life.Said[Key] = net::DayOf(T);
				Expect(town::PaysYourCheck(S, T).empty() && town::PaysYourCheck(S, T + 20.0).empty(), "check: once a night");
				S.Life.Said.erase(Key);
				S.Life.LiveEntries.pop_back();
				++ChecksPicked;
			}
		}
	}
	Expect(GameNights == 12, "dee's: Tuesday, Thursday and Saturday for four weeks");
	Expect(MostAtRubys >= 1 && RubysRegulars.size() >= 3, "ruby's: the regulars come in (" + std::to_string(RubysRegulars.size()) + ")");
	Expect(Shelter > 0, "shelter: people wait for the 14");
	Expect(ChecksPicked > 0, "check: a Friday with Big Lou at Ruby's");
	Expect(WorldSeen > Seated + Shelter, "schedule: the world's people are about the block, not only at Dee's and the shelter");
	Expect(Forecasts > 4000 && Unforeseen == 0, "schedule: every forecast held (" + std::to_string(Unforeseen) + " of " + std::to_string(Forecasts) + " missed)");
	std::printf("schedules: %d days, %d steps, %d staffing checks, %d at Dee's table over %d nights, %zu of Ruby's regulars, %d shelter sightings, %d sightings of the world's people; "
				"Who %.3f ms a call; %d ten-minute forecasts, %d missed\n",
		Days, Steps, StaffedChecks, Seated, GameNights, RubysRegulars.size(), Shelter, WorldSeen, 1000.0 * WhoSeconds / std::max(1, WhoCalls), Forecasts, Unforeseen);

	// The same seed gives the same schedule; another career's people keep other minutes.
	Hooks H2;
	Session Same(H2, "town-schedule");
	Same.NewCareer(Someone());
	Hooks H3;
	Session Other(H3, "town-other");
	Other.NewCareer(Someone());
	Hooks H4;
	Session Again(H4, "town-schedule");
	Again.NewCareer(Someone());
	bool Equal = true;
	bool Differs = false;
	for (int D = First; D < First + 7; ++D)
	{
		Clock(Same, At(D, 0, 1));
		Clock(Other, At(D, 0, 1));
		Clock(Again, At(D, 0, 1));
		const std::string A = Describe(town::Plan(At(D, 0), At(D + 1, 0), Same));
		const std::string B = Describe(town::Plan(At(D, 0), At(D + 1, 0), Again));
		Equal = Equal && A == B && !A.empty();
		Differs = Differs || A != Describe(town::Plan(At(D, 0), At(D + 1, 0), Other));
	}
	Expect(Equal, "schedule: the same seed gives the same schedule");
	Expect(Differs, "schedule: another career keeps other minutes");
}

/** More careers, eight weeks each: nobody in two places, no seat taken twice (Twitch out of Dee's and into Ruby's, Mrs.
 *  Park at the shelter's first spot with the Warm-Up's riders further along). */
void Crowding()
{
	int Problems = 0;
	int Plans = 0;
	int ParkSundays = 0;
	int TwitchMornings = 0;
	for (const char* Career : {"town-crowd-a", "town-crowd-b", "town-crowd-c", "town-crowd-d"})
	{
		Hooks H;
		Session S(H, Career);
		S.NewCareer(Someone());
		for (int D = 2; D < 2 + 56; ++D)
		{
			Clock(S, At(D, 0, 1));
			const std::vector<town::Placement> Plan = town::Plan(At(D, 0), At(D + 1, 0), S);
			Problems += Crowded(Plan, std::string(Career) + ", day " + std::to_string(D));
			++Plans;
			for (const town::Placement& P : Plan)
			{
				ParkSundays += P.Id == "mrs-park" && P.Venue == "diner" && P.What == town::Doing::Sit ? 1 : 0;
				TwitchMornings += P.Id == "twitch" && P.Venue == "diner" && P.What == town::Doing::Sit && P.Since >= At(D, 3) && P.Since < At(D, 5) ? 1 : 0;
			}
		}
	}
	Expect(Problems == 0, "crowding: " + std::to_string(Problems) + " conflicts in " + std::to_string(Plans) + " days");
	std::printf("crowding: %d days over four careers, %d conflicts; Mrs. Park at Ruby's %d Sundays, Twitch at the counter %d mornings\n", Plans, Problems, ParkSundays, TwitchMornings);
}

// ------------------------------------------------------------------ the corner, the 14, the crowd, the queue

void Corner()
{
	// A full cycle from noon on a Tuesday, a quarter second at a time.
	const double Noon = At(8, 12);
	std::vector<std::pair<int, double>> Runs; // phase, seconds
	int Last = -1;
	bool WalkOk = true;
	bool CountOk = true;
	int LastCount = 999;
	for (int Q = 0; Q < 4 * 200; ++Q)
	{
		const double T = Noon + (Q * 0.25) / 60.0;
		const town::SignalState S = town::Signal(T);
		if (S.Phase != Last)
		{
			Runs.push_back({S.Phase, 0.0});
			Last = S.Phase;
			LastCount = 999;
		}
		Runs.back().second += 0.25;
		const double Into = (S.Phase == 1 ? 42.0 : S.Phase == 4 ? 26.0 : 0.0) - S.Left;
		if (S.Phase == 1)
		{
			WalkOk = WalkOk && S.FifthGo && !S.MarketGo && S.WalkMarket == (Into < 7.0) && !S.WalkFifth;
		}
		if (S.Phase == 4)
		{
			WalkOk = WalkOk && S.MarketGo && !S.FifthGo && S.WalkFifth == (Into < 7.0) && !S.WalkMarket;
		}
		if ((S.Phase == 1 || S.Phase == 4) && Into >= 7.0)
		{
			CountOk = CountOk && S.Countdown > 0 && S.Countdown <= LastCount && S.Countdown == static_cast<int>(std::ceil(S.Left));
			LastCount = S.Countdown;
		}
		if (S.Phase != 1 && S.Phase != 4)
		{
			WalkOk = WalkOk && !S.FifthGo && !S.MarketGo && !S.WalkFifth && !S.WalkMarket && S.Countdown == 0;
		}
	}
	// The middle runs are whole phases: 42, 4, 3, 26, 4, 3 (82 s).
	std::map<int, double> Whole;
	for (size_t K = 1; K + 1 < Runs.size(); ++K)
	{
		Whole[Runs[K].first] = Runs[K].second;
	}
	const double Lengths[7] = {0.0, 42.0, 4.0, 3.0, 26.0, 4.0, 3.0};
	double Cycle = 0.0;
	for (int P = 1; P <= 6; ++P)
	{
		Expect(Whole.count(P) && std::fabs(Whole[P] - Lengths[P]) < 0.3, "signal: phase " + std::to_string(P) + " lasts " + std::to_string(Lengths[P]) + " s");
		Cycle += Whole[P];
	}
	Expect(std::fabs(Cycle - 82.0) < 0.6, "signal: the cycle is 82 s");
	Expect(WalkOk, "signal: WALK for the first 7 s of the parallel green, the steady hand otherwise");
	Expect(CountOk, "signal: the flashing hand counts down to the amber");
	// Overnight it flashes; it finishes the cycle running at 1:00 first, and comes out of it into Fifth's green at 5:00.
	Expect(!town::Signal(At(8, 0, 59)).Flashing && !town::Signal(At(8, 1, 1)).Flashing, "signal: not flashing before the cycle running at 1:00 ends");
	for (const double T : {At(8, 1, 2), At(8, 2, 30), At(8, 4, 59)})
	{
		const town::SignalState S = town::Signal(T);
		Expect(S.Flashing && S.Phase == 0 && !S.WalkFifth && !S.WalkMarket && S.Countdown == 0, "signal: flashing overnight, the ped heads dark");
	}
	Expect(town::Signal(At(8, 2, 30)).FlashOn != town::Signal(At(8, 2, 30) + 0.5 / 60.0).FlashOn, "signal: the flash is on half of each second");
	Expect(town::Signal(At(8, 5, 0)).Phase == 1 && !town::Signal(At(8, 5, 0)).Flashing, "signal: 5:00 opens on Fifth's green");

	// The 14 (§2.6).
	auto Labels = [](const std::vector<double>& L) {
		std::string Out;
		for (const double T : L)
		{
			Out += net::TimeLabel(T) + ";";
		}
		return Out;
	};
	Expect(Labels(town::BusTimes("market-east", At(7, 5, 0), 4)) == "5:35 AM;6:05 AM;6:35 AM;7:05 AM;", "bus: eastbound mornings (" + Labels(town::BusTimes("market-east", At(7, 5, 0), 4)) + ")");
	Expect(Labels(town::BusTimes("market-east", At(7, 23, 20), 4)) == "11:35 PM;12:35 AM;1:35 AM;2:35 AM;", "bus: eastbound hourly overnight (" + Labels(town::BusTimes("market-east", At(7, 23, 20), 4)) + ")");
	Expect(Labels(town::BusTimes("market-west", At(7, 23, 30), 3)) == "11:50 PM;12:50 AM;1:50 AM;", "bus: westbound overnight at :50");
	Expect(Labels(town::BusTimes("market-west", At(7, 5, 51), 3)) == "6:20 AM;6:50 AM;7:20 AM;", "bus: westbound by day at :20 and :50");
	Expect(town::BusTimes("market-east", At(7, 6, 5), 1).front() == At(7, 6, 5), "bus: a departure now is the next");
	Expect(town::BusTimes("nowhere", At(7, 6, 0), 3).empty() && town::BusTimes("market-east", At(7, 6, 0), 0).empty(), "bus: no such stop");
	int Daily = 0;
	for (const double T : town::BusTimes("market-east", At(9, 0, 0), 60))
	{
		Daily += T < At(10, 0, 0) ? 1 : 0;
	}
	Expect(Daily == 42, "bus: 36 by day and 6 overnight (" + std::to_string(Daily) + ")");

	// The crowd: low 2:00-5:00, peaks at 8:00-9:00 and 17:00-19:00, Friday and Saturday nights to 2:00, rain takes 30% off.
	const int Tue = 8;
	Expect(town::CrowdLevel(At(Tue, 3, 30), 0.0) < 0.1, "crowd: empty at 3:30");
	Expect(town::CrowdLevel(At(Tue, 8, 30), 0.0) > 0.8 && town::CrowdLevel(At(Tue, 18, 0), 0.0) > 0.85, "crowd: the commutes");
	Expect(town::CrowdLevel(At(Tue, 8, 30), 0.0) > town::CrowdLevel(At(Tue, 14, 30), 0.0), "crowd: the morning peak beats the afternoon");
	Expect(town::CrowdLevel(At(11, 23, 30), 0.0) > town::CrowdLevel(At(Tue, 23, 30), 0.0) + 0.2, "crowd: Friday night is out");
	Expect(town::CrowdLevel(At(13, 1, 0), 0.0) > town::CrowdLevel(At(9, 1, 0), 0.0) + 0.2, "crowd: Saturday night's small hours are busier than Tuesday night's");
	Expect(std::fabs(town::CrowdLevel(At(Tue, 18, 0), 1.0) - 0.7 * town::CrowdLevel(At(Tue, 18, 0), 0.0)) < 1e-9, "crowd: rain takes 30% off");
	bool Smooth = true;
	for (int M = 0; M < 7 * 1440; M += 5)
	{
		const double A = town::CrowdLevel(At(7, 0, M), 0.0);
		const double B = town::CrowdLevel(At(7, 0, M + 5), 0.0);
		Smooth = Smooth && A >= 0.0 && A <= 1.0 && std::fabs(A - B) < 0.05;
	}
	Expect(Smooth, "crowd: 0..1 and no jumps (midnights included)");

	// The barber's queue: Saturday morning's 10-25, 0-10 otherwise, 0 while shut.
	bool Saturday = true;
	bool Weekday = true;
	for (int M = 10 * 60; M < 14 * 60; M += 15)
	{
		const int Q = town::BarberQueue(At(12, 0, M)); // Saturday, October 17
		Saturday = Saturday && Q >= 10 && Q <= 25 && Q % 5 == 0;
	}
	for (int M = 10 * 60; M < 22 * 60; M += 15)
	{
		const int Q = town::BarberQueue(At(Tue, 0, M));
		Weekday = Weekday && Q >= 0 && Q <= 10;
	}
	Expect(Saturday, "queue: Saturday 10:00-14:00 waits 10-25 minutes");
	Expect(Weekday, "queue: 0-10 minutes otherwise");
	Expect(town::BarberQueue(At(7, 12)) == 0 && town::BarberQueue(At(Tue, 23)) == 0, "queue: none while shut (Monday, after ten)");
	Expect(town::BarberQueue(At(13, 11, 7)) == town::BarberQueue(At(13, 11, 7)), "queue: the same minute, the same wait");

	// Who's on shift where the hours are people's own: the hand-offs are on the hour.
	Expect(town::OnShift("store", At(Tue, 9, 59)) == "benny" && town::OnShift("store", At(Tue, 10, 0)) == "ray" && town::OnShift("store", At(Tue, 22, 0)) == "benny", "shift: the Lucky Penny's clerks");
	Expect(town::OnShift("diner", At(Tue, 5, 59)) == "marlene" && town::OnShift("diner", At(Tue, 6, 0)) == "ruby" && town::OnShift("diner", At(Tue, 18, 0)) == "marlene", "shift: Ruby's waitresses");
	Expect(town::OnShift("barber", At(7, 12)).empty() && town::OnShift("barber", At(Tue, 12)) == "lonnie", "shift: the barber's closed Mondays");
	Expect(town::OnShift("gym", At(Tue, 14)).empty() && town::OnShift("gym", At(Tue, 17)) == "rosa", "shift: Rosa's split shift");
	Expect(town::OnShift("pawn", At(Tue, 9, 30)) == "augie" && town::OnShift("pawn", At(Tue, 20, 30)).empty(), "shift: Augie's hours");
	Expect(town::OnShift("washfold", At(Tue, 3)) == "inez" && town::OnShift("nowhere", At(Tue, 3)).empty(), "shift: the Wash & Fold, and nowhere");
}

// ------------------------------------------------------------------ talk

/** A Talk's every choice answers with something to say (or nothing, after "bye" and "service"). */
bool Answers(const town::TalkContext& C, const town::Talk& T)
{
	bool Ok = !T.Line.empty() && !T.Name.empty();
	for (const town::Choice& Ch : T.Choices)
	{
		const town::Talk A = town::Answer(C, Ch.Id);
		Ok = Ok && !Ch.Label.empty() && !A.Line.empty();
	}
	return Ok;
}

void TalkOrder()
{
	Hooks H;
	Session S(H, "town-talk");
	S.NewCareer(Someone(hero::Background::DealersKid));
	S.BankrollCents = 100000;

	// Sunday night, the barber shut: Lonnie's closed bark names the day it opens.
	Clock(S, At(6, 23, 0));
	{
		const auto C = town::ContextFor(S, "lonnie", "barber");
		Expect(town::Bark(*C, "closing") == "Closed, kid. Tuesday.", "bark: \"" + town::Bark(*C, "closing") + "\"");
		const auto Rosa = town::ContextFor(S, "rosa", "gym");
		Expect(town::Bark(*Rosa, "closing") == "Fob works. Lights work. I don't, till six.", "bark: Rosa's door at night (\"" + town::Bark(*Rosa, "closing") + "\")");
	}

	// Tuesday, a dry hour: the barber's open.
	Clock(S, DryHour(S, 8, 10, 21));
	const int Today = 8;
	const uint32_t Look = town::LookHash(S.Person.Appearance, S.Life.Closet);
	S.Life.Hunger = 85.0;
	S.Life.Energy = 80.0;
	S.Life.Thirst = 20.0;
	town::Bond Stiffed;
	Stiffed.Familiarity = 10;
	Stiffed.Visits = 2;
	Stiffed.LastDay = Today - 1;
	Stiffed.LookSeen = Look;
	Stiffed.Flags = town::Stiffed;
	S.Life.Town["lonnie"] = Stiffed;
	HistoryEntry Result;
	Result.Name = "Night Owl $5.50";
	Result.Place = 14;
	Result.Entrants = 312;
	Result.Prize = 1840;
	Result.EventId = "night-owl@" + std::to_string(static_cast<long long>(At(7, 23, 0)));
	S.History.push_back(Result);
	{
		const auto C = town::ContextFor(S, "lonnie", "barber");
		const town::Talk T = town::Open(*C);
		Expect(T.Topic == "look:hungry", "talk: hungry first (" + T.Topic + ": " + T.Line + ")");
		Expect(T.Name == "Lonnie" && T.Label == "Barber", "talk: the barber's name and label");
		Expect(Answers(*C, T), "talk: every choice answers");
		bool Cut = false;
		for (const town::Choice& Ch : T.Choices)
		{
			Cut = Cut || (Ch.Id == "service" && Ch.Label == "Get a cut");
		}
		Expect(Cut, "talk: Lonnie offers a cut at work");
	}
	S.Life.Hunger = 30.0;
	{
		const town::Talk T = town::Open(*town::ContextFor(S, "lonnie", "barber"));
		Expect(T.Topic == "bond:stiffed", "talk: then what he knows (" + T.Topic + ": " + T.Line + ")");
	}
	S.Life.Town["lonnie"].Flags = 0;
	{
		// He knows the player's background: once.
		const auto C = town::ContextFor(S, "lonnie", "barber");
		const town::Talk T = town::Open(*C);
		Expect(T.Topic == "bond:background" && T.SaidKey == "bg:lonnie", "talk: the dealer's kid (" + T.Topic + ")");
		S.Heard("lonnie", T);
	}
	{
		const town::Talk T = town::Open(*town::ContextFor(S, "lonnie", "barber"));
		Expect(T.Topic == "news" && T.SaidKey.rfind("result:night-owl@", 0) == 0, "talk: then the news (" + T.Topic + ": " + T.Line + ")");
		Expect(T.Line.find("Fourteenth") != std::string::npos, "talk: the player's place, said ('" + T.Line + "')");
		S.Heard("lonnie", T);
	}
	{
		const town::Talk T = town::Open(*town::ContextFor(S, "lonnie", "barber"));
		Expect(T.SaidKey.rfind("result:", 0) != 0, "talk: the news is told once");
		Expect(S.Life.Town["lonnie"].Familiarity >= 12 && S.Life.Town["lonnie"].LastDay == Today && S.Life.Town["lonnie"].LookSeen == Look, "talk: the bond remembers");
	}

	// A Talk's familiarity and look land on the bond; a stale loan's flag goes when the loan is forgotten.
	{
		const int Now = net::DayOf(S.WorldMinutes());
		S.Life.Said["loan:twitch"] = Now - town::SaidKeepDays - 5;
		S.Life.Town["twitch"].Familiarity = 3;
		S.Life.Town["twitch"].Flags = town::Lent;
		S.Heard("lonnie", town::Answer(*town::ContextFor(S, "lonnie", "barber"), "bye"));
		Expect(S.Life.Said.count("loan:twitch") == 0 && (S.Life.Town["twitch"].Flags & town::Lent) == 0, "lend: a loan forgotten after 90 days takes its flag with it");
		S.Life.Town.erase("twitch");
	}

	// Rumours: Dee's next game, then the Garage (on the calendar, with a text), each told once, whoever asks.
	std::vector<std::string> Keys;
	for (int K = 0; K < 4; ++K)
	{
		const auto C = town::ContextFor(S, "lonnie", "barber");
		const town::Talk T = town::Answer(*C, "games");
		Expect(!T.Line.empty(), "rumours: Lonnie answers about games");
		Keys.push_back(T.SaidKey);
		S.Heard("lonnie", T);
	}
	Expect(Keys[0].rfind("rumour:dees:", 0) == 0 && Keys[1] == "rumour:garage" && Keys[2].empty() && Keys[3].empty(), "rumours: Dee's game, then the Garage, then nothing new (" + Keys[0] + ", " + Keys[1] + ")");
	Expect(S.Life.Unlocks.count("rumour:garage") == 1 && H.Got("Calendar", "The Garage"), "rumours: the Garage goes on the calendar with a text");
	{
		Clock(S, At(9, 7, 30)); // Wednesday morning at Ruby's
		const auto Sal = town::ContextFor(S, "sal", "diner");
		bool Again = false;
		for (const town::Choice& Ch : town::Open(*Sal).Choices)
		{
			Again = Again || town::Answer(*Sal, Ch.Id).SaidKey == "rumour:garage";
		}
		Expect(!Again, "rumours: told once, whoever tells it");
	}

	// The world's people on the block remember the meeting (MemoryKind::Street): once a day, one memory for a run of them.
	{
		const int Sal = S.Living().Find("Sal");
		const size_t Before = S.Living().BondWith(Sal) ? S.Living().BondWith(Sal)->Memories.size() : 0;
		const auto C = town::ContextFor(S, "sal", "diner");
		const town::Talk T = town::Open(*C);
		Expect(T.Name == "Sal" && !T.Line.empty(), "street: Sal talks");
		S.Heard("sal", T);
		S.Heard("sal", town::Answer(*C, "news"));
		S.Heard("sal", T);
		const world::Bond* B = S.Living().BondWith(Sal);
		town::Placement Here;
		const std::string Place = town::Where("sal", S.WorldMinutes(), S, Here) && Here.Venue == "diner" ? "Ruby's" : "Fifth St.";
		Expect(B && B->Memories.size() == Before + 1 && B->Memories.back().Kind == world::MemoryKind::Street && B->Memories.back().Where == Place, "street: Sal remembers meeting at " + Place + ", once");
		Clock(S, At(10, 7, 40));
		S.Heard("sal", town::Open(*town::ContextFor(S, "sal", "diner")));
		Clock(S, At(11, 7, 40));
		S.Heard("sal", town::Open(*town::ContextFor(S, "sal", "diner")));
		B = S.Living().BondWith(Sal);
		Expect(B && B->Memories.back().Kind == world::MemoryKind::Street && B->Memories.back().Day == 11 && B->Encounters >= 3, "street: mornings at Ruby's are one memory, kept current");
		Expect(town::Tag("sal", S) == "Knows you", "street: his tag says he knows you (\"" + town::Tag("sal", S) + "\")");
		Expect(!S.Living().SpokenGreeting(Sal, 0).empty(), "street: the greeting pool knows the street");
	}

	// The keepers' lines, every moment of their screens.
	Clock(S, DryHour(S, 12, 10, 14)); // Saturday, the shops open
	struct Want
	{
		const char* Who;
		const char* Venue;
		std::vector<const char*> Moments;
	};
	const Want Keepers[] = {
		{"lonnie", "barber", {"greet", "browse", "pick", "pick:color", "grow", "wait", "pay", "declined", "tip", "stiff", "leave", "closing"}},
		{"bea", "thrift", {"greet", "saturday", "browse", "pick", "try", "pay", "declined", "donate", "leave", "closing"}},
		{"rosa", "gym", {"greet", "browse", "pick", "train", "pay", "declined", "leave", "closing"}},
		{"ruby", "diner", {"greet", "browse", "pick", "pay", "declined", "tip", "stiff", "coffee", "sit", "leave"}},
		{"marlene", "diner", {"greet", "browse", "pick", "pay", "declined", "tip", "stiff", "coffee", "sit", "leave"}},
		{"augie", "pawn",
			{"greet", "browse", "pick", "appraise", "appraise:gear", "appraise:cup", "appraise:award", "appraise:keepsake", "appraise:garment", "appraise:lot114", "offer", "refuse",
				"refuse:dee-chip", "refuse:furniture", "sell", "loan", "redeem", "renew", "forfeit", "guitars", "pay", "declined", "leave", "closing"}},
		{"ray", "store", {"greet", "browse", "pick", "pay", "declined", "leave"}},
		{"benny", "store", {"greet", "browse", "pick", "pay", "declined", "leave"}},
	};
	for (const Want& W : Keepers)
	{
		const auto C = town::ContextFor(S, W.Who, W.Venue);
		for (const char* M : W.Moments)
		{
			Expect(!town::KeeperLine(*C, M).empty(), std::string("keeper: ") + W.Who + " has a line for " + M);
		}
		Expect(town::KeeperLine(*C, "no-such-moment").empty(), std::string("keeper: ") + W.Who + " says nothing to nothing");
		Expect(Answers(*C, town::Open(*C)), std::string("talk: ") + W.Who + " answers every choice");
	}
	{
		const auto Augie = town::ContextFor(S, "augie", "pawn");
		Expect(town::KeeperLine(*Augie, "offer:24700:19700") == "Two-forty-seven to own it. One-ninety-seven to hold it, thirty days. The ticket's your receipt. Don't lose it.",
			"keeper: Augie's offer, as he says it (" + town::KeeperLine(*Augie, "offer:24700:19700") + ")");
		Expect(town::KeeperLine(*Augie, "refuse:dee-chip") == "Dee's? Put that back in your pocket. She'd have my license.", "keeper: Dee's chip, refused");
		Expect(town::KeeperLine(*town::ContextFor(S, "bea", "thrift"), "greet").find("half off today") != std::string::npos, "keeper: Bea's Saturday yellow tags");
	}
	Expect(town::SpokenDollars(247) == "two-forty-seven" && town::SpokenDollars(197) == "one-ninety-seven" && town::SpokenDollars(1370) == "thirteen-seventy" &&
			   town::SpokenDollars(4500) == "forty-five hundred" && town::SpokenDollars(63) == "sixty-three" && town::SpokenDollars(6000) == "six thousand" &&
			   town::SpokenDollars(205) == "two-oh-five" && town::SpokenDollars(2047) == "two thousand forty-seven" && town::SpokenDollars(12500) == "twelve thousand five hundred",
		"spoken: dollars as a pawnbroker says them");

	// Barks: strangers and people who know you; a stranger who recognizes the player.
	{
		const auto Cook = town::ContextFor(S, "cook", "diner");
		Expect(town::Bark(*Cook, "bump") == "Behind!", "bark: the cook");
		const town::Talk T = town::Open(*Cook);
		Expect(!T.Line.empty() && T.Choices.size() == 1 && T.Choices.front().Id == "bye", "talk: the cook has eggs going");
		std::string Local;
		for (const int Id : S.Living().EmbercrestRegulars())
		{
			if (!S.Living().BondWith(Id))
			{
				Local = "npc:" + S.Living().Get(Id)->Name;
				break;
			}
		}
		const auto Stranger = town::ContextFor(S, Local, "market-stop");
		Expect(town::Bark(*Stranger, "recognize").find(S.HeroName) != std::string::npos, "bark: a stranger recognizes the player");
		Expect(town::Bark(*town::ContextFor(S, "sal", "diner"), "recognize").empty(), "bark: Sal doesn't need to");
		Expect(!town::Bark(*Stranger, "rain").empty() && !town::Bark(*Stranger, "excuse").empty() && !town::Bark(*Stranger, "hello").empty(), "bark: rain, excuse me, hello");
		Expect(town::Open(*Stranger).Label == "Embercrest regular" && town::Tag(Local, S).empty(), "talk: a local's label, no tag till met");
	}

	// Tips are remembered.
	S.Tipped("marlene", 0);
	Expect((S.Life.Town["marlene"].Flags & town::Stiffed) != 0, "tips: a stiff is remembered");
	Expect(town::KeeperLine(*town::ContextFor(S, "marlene", "diner"), "greet").find("cash up front") != std::string::npos, "tips: Marlene remembers");
	S.Tipped("marlene", 20);
	Expect((S.Life.Town["marlene"].Flags & town::Stiffed) == 0 && (S.Life.Town["marlene"].Flags & town::Tipper) != 0, "tips: a good tip makes up for it");
	const Chips Bank = S.BankrollCents;
	const size_t Ledger = S.Life.Ledger.size();
	Expect(!S.Lend("sal", 2000).empty() && S.BankrollCents == Bank && S.Life.Ledger.size() == Ledger, "lend: only Twitch asks");

	// The save: town and said lines read back to the same text.
	{
		S.Save();
		const std::string Text = H.Last.Serialize();
		SaveData D;
		Expect(SaveData::Parse(Text, D) && D.Serialize() == Text, "save: the town's lines round-trip");
		Expect(Text.find("\ntown\tlonnie\t") != std::string::npos && Text.find("\nsaid\trumour:garage\t") != std::string::npos, "save: the bonds and what was said are in it");
		Expect(D.Life.Town.count("lonnie") == 1 && D.Life.Town["lonnie"].LookSeen == S.Life.Town["lonnie"].LookSeen, "save: a bond reads back");
	}
	const int GarageTold = S.Life.Said.count("rumour:garage") > 0 ? S.Life.Said["rumour:garage"] : -1;

	// Twitch's twenty: lent when the world has him broke, paid back from his next cash; and the next twenty too.
	bool Asked = false;
	bool Repaid = false;
	int Loans = 0;
	int Repayments = 0;
	int Days = 0;
	for (; Days < 360 && Repayments < 2; ++Days)
	{
		if (Repaid)
		{
			Asked = false;
			Repaid = false;
		}
		S.WorldSkip(1);
		const auto C = town::ContextFor(S, "twitch", "diner");
		const town::Talk T = town::Open(*C);
		if (!Asked)
		{
			bool CanLend = false;
			for (const town::Choice& Ch : T.Choices)
			{
				CanLend = CanLend || Ch.Id == "lend";
			}
			if (!CanLend)
			{
				Expect(!S.Lend("twitch", 2000).empty(), "lend: not while he's flush");
				continue;
			}
			Expect(!town::Answer(*C, "lend").Line.empty() && town::Answer(*C, "lend").Choices.size() == 2, "lend: he asks for twenty");
			const Chips Before = S.BankrollCents;
			Expect(!S.Lend("twitch", 500).empty(), "lend: he asked for twenty");
			Expect(S.Lend("twitch", 2000).empty() && S.BankrollCents == Before - 2000 && S.Life.Ledger.front().Amount == -2000 && S.Life.Ledger.front().Kind == 7, "lend: twenty through the ledger");
			Expect(!S.Lend("twitch", 2000).empty(), "lend: once");
			Expect((S.Life.Town["twitch"].Flags & town::Lent) != 0, "lend: remembered");
			const auto Mark = S.Life.Said.lower_bound("loan:twitch:won:");
			Expect(Mark != S.Life.Said.end() && Mark->first.rfind("loan:twitch:won:", 0) == 0, "lend: what he had won when he borrowed it is kept");
			Asked = true;
			++Loans;
			continue;
		}
		if (T.Topic == "bond:repaid")
		{
			const Chips Before = S.BankrollCents;
			S.Heard("twitch", T);
			Expect(S.BankrollCents == Before + 2000 && S.Life.Ledger.front().Amount == 2000 && S.Life.Said.count("loan:twitch") == 0 && (S.Life.Town["twitch"].Flags & town::Lent) == 0,
				"lend: he pays it back, through the ledger");
			const auto Mark = S.Life.Said.lower_bound("loan:twitch:won:");
			Expect(Mark == S.Life.Said.end() || Mark->first.rfind("loan:twitch:won:", 0) != 0, "lend: the loan's mark goes with it");
			Expect(town::Open(*town::ContextFor(S, "twitch", "diner")).Topic != "bond:repaid", "lend: paid back once");
			Expect(S.Life.Said.count("repaid:twitch") == 0, "lend: the payback isn't kept as a line said (the next loan comes back too)");
			const Chips After = S.BankrollCents;
			S.Heard("twitch", T);
			Expect(S.BankrollCents == After, "lend: the same payback heard twice pays once");
			Repaid = true;
			++Repayments;
		}
		else
		{
			Expect(T.Topic == "bond:owes" || T.Topic.rfind("look:", 0) == 0, "lend: he owes it till he cashes (" + T.Topic + ")");
		}
	}
	Expect(Loans >= 1, "lend: Twitch was broke and asked at some point");
	Expect(Repayments == Loans || (Repayments + 1 == Loans && Days == 360), "lend: he pays back every twenty from his next cash (" + std::to_string(Loans) + " lent, " + std::to_string(Repayments) + " back)");
	std::printf("talk: Twitch borrowed %d times and paid back %d in %d days\n", Loans, Repayments, Days);

	// The save again, months on: still the same text both ways, the bonds kept, and a rumour told more than 90 days ago
	// pruned (it can be told again).
	{
		S.Save();
		const std::string Text = H.Last.Serialize();
		SaveData D;
		Expect(SaveData::Parse(Text, D) && D.Serialize() == Text, "save: the town's lines round-trip months on");
		const int Now = net::DayOf(S.WorldMinutes());
		Expect(Text.find("\ntown\tlonnie\t") != std::string::npos, "save: the bonds stay");
		Expect(GarageTold >= 0 && (Now - GarageTold > town::SaidKeepDays) == (Text.find("\nsaid\trumour:garage\t") == std::string::npos),
			"save: a rumour told more than 90 days ago is pruned (told day " + std::to_string(GarageTold) + ", now " + std::to_string(Now) + ")");
	}

	// Forgotten in play as in the save: past 90 days the Garage is news again without a reload, and remembered from the new
	// day (a long session forgets what a reloaded one does).
	while (GarageTold >= 0 && net::DayOf(S.WorldMinutes()) - GarageTold <= town::SaidKeepDays)
	{
		S.WorldSkip(1);
	}
	{
		const int Now = net::DayOf(S.WorldMinutes());
		std::vector<std::string> Again;
		for (int K = 0; K < 3; ++K)
		{
			const town::Talk T = town::Answer(*town::ContextFor(S, "lonnie", "barber"), "games");
			Again.push_back(T.SaidKey);
			S.Heard("lonnie", T);
		}
		Expect(std::find(Again.begin(), Again.end(), "rumour:garage") != Again.end(), "rumours: past 90 days the Garage is news again");
		Expect(S.Life.Said.count("rumour:garage") == 1 && S.Life.Said["rumour:garage"] == Now, "rumours: and it's remembered from the day it was told again");
		bool Stale = false;
		for (const auto& It : S.Life.Said)
		{
			Stale = Stale || !town::SaidFresh(It.second, Now);
		}
		Expect(!Stale, "rumours: nothing older than 90 days is kept in play");
		S.Save();
		const std::string Text = H.Last.Serialize();
		SaveData D;
		Expect(SaveData::Parse(Text, D) && D.Life.Said == S.Life.Said, "save: what's remembered in play is what the save keeps");
	}
}

/** The rumours' words: tonight's Nightly and its bus, Dee's game being dealt past midnight, Dee on her own game, when a
 *  winner won, and the doors' "back tomorrow". */
void Rumours()
{
	Hooks H;
	Session S(H, "town-rumours");
	S.NewCareer(Someone(hero::Background::Newcomer));
	// A weekday afternoon off Dee's nights with a face the player would know in tonight's Nightly: Ray says so, and the bus.
	int D = 14; // Monday, October 19
	bool Tried = false;
	for (; D < 28 && !Tried; ++D)
	{
		const int Wd = net::Weekday(D);
		if (Wd != 0 && Wd != 2 && Wd != 4)
		{
			continue;
		}
		Clock(S, At(D, 15));
		bool Known = false;
		for (const int Id : S.Living().RegisteredNow("embercrest-nightly@" + std::to_string(D)))
		{
			const world::Npc* N = S.Living().Get(Id);
			Known = Known || (N && N->From == world::Origin::Cast);
		}
		if (!Known)
		{
			continue;
		}
		Tried = true;
		std::string Line;
		for (int K = 0; K < 12 && Line.empty(); ++K)
		{
			const town::Talk T = town::Answer(*town::ContextFor(S, "ray", "store"), "news");
			if (T.SaidKey.empty())
			{
				break;
			}
			if (T.SaidKey == "rumour:nightly:" + std::to_string(D))
			{
				Line = T.Line;
			}
			S.Heard("ray", T);
		}
		Expect(Line.find("in the Nightly tonight. Bus at six-thirty-five.") != std::string::npos && Line.find("'re ") == std::string::npos,
			"rumour: tonight's Nightly, who's in it and the 14 ('" + Line + "')");
		std::printf("rumours: Ray at three: \"%s\"\n", Line.c_str());
	}
	Expect(Tried, "rumour: a face the player knows in a Nightly within two weeks");

	// Past midnight on a Wednesday, Tuesday's game is still being dealt.
	while (net::Weekday(D) != 2)
	{
		++D;
	}
	Clock(S, At(D, 1));
	{
		const town::Talk T = town::Answer(*town::ContextFor(S, "marlene", "diner"), "games");
		Expect(T.SaidKey == "rumour:dees:" + std::to_string(D - 1) && T.Line.find("right now") != std::string::npos && T.Line.find("Doors at nine") == std::string::npos &&
				   T.Line.find("coming") == std::string::npos,
			"rumour: Dee's game, still going at one (" + T.SaidKey + ": '" + T.Line + "')");
		Expect(T.Line.size() <= 140, "rumour: Dee's table in a line the card holds (three names at most)");
		std::printf("rumours: Marlene at one: \"%s\"\n", T.Line.c_str());
	}
	// Dee on her own game, before the doors and while she deals.
	Clock(S, At(D + 1, 19));
	{
		const town::Talk T = town::Answer(*town::ContextFor(S, "dee", "washfold"), "games");
		Expect(T.SaidKey == "rumour:dees:" + std::to_string(D + 1) && T.Line.rfind("My game's tonight. Doors at nine.", 0) == 0, "rumour: Dee on her own game ('" + T.Line + "')");
		std::printf("rumours: Dee at seven: \"%s\"\n", T.Line.c_str());
	}
	Clock(S, At(D + 1, 22));
	{
		const town::Talk T = town::Answer(*town::ContextFor(S, "dee", "washfold"), "games");
		Expect(T.Line.rfind("We're dealing right now.", 0) == 0 && T.Line.find("Dee's") == std::string::npos, "rumour: Dee, dealing ('" + T.Line + "')");
		std::printf("rumours: Dee at ten: \"%s\"\n", T.Line.c_str());
	}

	// Sunday night: the Warm-Up was won today, not last night.
	while (net::Weekday(D) != 6)
	{
		++D;
	}
	Clock(S, At(D, 22, 30));
	{
		const std::string Warmup = "embercrest-warmup@" + std::to_string(D);
		const net::EventResult* R = S.Living().ResultOf(Warmup);
		std::string Line;
		std::string LastNight;
		for (int K = 0; K < 16; ++K)
		{
			const town::Talk T = town::Answer(*town::ContextFor(S, "marlene", "diner"), "news");
			if (T.SaidKey.empty())
			{
				break;
			}
			if (T.SaidKey == "rumour:winner:" + Warmup)
			{
				Line = T.Line;
			}
			if (T.SaidKey == "rumour:winner:embercrest-nightly@" + std::to_string(D - 1))
			{
				LastNight = T.Line;
			}
			S.Heard("marlene", T);
		}
		if (R && !R->FinalTable.empty() && R->FinalTable.front().Player != -1)
		{
			Expect(Line.find("Warm-Up today") != std::string::npos, "rumour: the Warm-Up's winner, today ('" + Line + "')");
			std::printf("rumours: Marlene on Sunday night: \"%s\" / \"%s\"\n", Line.c_str(), LastNight.c_str());
		}
		Expect(LastNight.empty() || LastNight.find("Nightly last night") != std::string::npos, "rumour: Saturday's Nightly, last night ('" + LastNight + "')");
	}

	// Monday: Rosa before the desk shuts at noon names four o'clock; Bea before nine names tomorrow.
	D += 1;
	Clock(S, At(D, 11, 55));
	Expect(town::KeeperLine(*town::ContextFor(S, "rosa", "gym"), "door") == "Fob works. Lights work. I don't, till four.", "keeper: Rosa, before the noon break");
	Clock(S, At(D, 20, 52));
	Expect(town::KeeperLine(*town::ContextFor(S, "bea", "thrift"), "closing") == "We close at nine, sweetheart. Bring it to the register or bring it back tomorrow.",
		"keeper: Bea at closing (\"" + town::KeeperLine(*town::ContextFor(S, "bea", "thrift"), "closing") + "\")");
}

void Lines()
{
	// Pruning: said keys older than 90 days and bonds with no familiarity aren't written; a said line with no day reads as told today.
	std::map<std::string, town::Bond> Town;
	Town["lonnie"].Familiarity = 7;
	Town["lonnie"].Visits = 3;
	Town["lonnie"].LastDay = 40;
	Town["lonnie"].LookSeen = 123456789u;
	Town["lonnie"].Flags = town::Tipper | town::Owed;
	Town["bea"].Visits = 1; // never talked to, no familiarity
	std::map<std::string, int> Said = {{"rumour:old", 9}, {"rumour:edge", 10}, {"rumour:new", 100}, {"rumour:legacy", town::SaidToday}};
	std::string Out;
	town::WriteLines(Town, Said, 100, Out);
	Expect(Out.find("town\tlonnie\t7\t3\t40\t123456789\t5\n") != std::string::npos, "lines: a bond's line (" + Out + ")");
	Expect(Out.find("bea") == std::string::npos, "lines: a bond with no familiarity isn't written");
	Expect(Out.find("rumour:old") == std::string::npos && Out.find("said\trumour:edge\t10\n") != std::string::npos && Out.find("said\trumour:new\t100\n") != std::string::npos,
		"lines: said keys older than 90 days go");
	Expect(Out.find("said\trumour:legacy\t100\n") != std::string::npos, "lines: a said key from before the days were kept is written as today");
	std::map<std::string, town::Bond> Town2;
	std::map<std::string, int> Said2;
	std::istringstream In(Out);
	std::string Line;
	while (std::getline(In, Line))
	{
		Expect(town::ReadLine(Tabs(Line), Town2, Said2), "lines: every line reads (" + Line + ")");
	}
	std::string Again;
	town::WriteLines(Town2, Said2, 100, Again);
	Expect(Again == Out, "lines: round-trip");
	Expect(town::ReadLine(Tabs("said\tsal-rumour"), Town2, Said2) && Said2["sal-rumour"] == town::SaidToday, "lines: an older said line with no day");
	Expect(!town::ReadLine(Tabs("closet\tseeded"), Town2, Said2) && !town::ReadLine(Tabs("town\tshort"), Town2, Said2), "lines: not ours");
	std::map<std::string, town::Bond> Odd;
	Odd["tab\tbed id"].Familiarity = 1;
	std::string OddOut;
	town::WriteLines(Odd, {}, 0, OddOut);
	std::map<std::string, town::Bond> OddBack;
	std::map<std::string, int> None;
	Expect(town::ReadLine(Tabs(OddOut.substr(0, OddOut.size() - 1)), OddBack, None) && OddBack.count("tab\tbed id") == 1, "lines: escaped ids");
}

// ------------------------------------------------------------------ the weather

void Weather()
{
	const double Career = 1440.0 + 127.0;
	const int64_t From = static_cast<int64_t>(std::floor(Career / weather::SlotMinutes));
	const int64_t Slots = 90 * 48;
	Hooks H;
	Session S(H, "town-weather");
	const uint64_t Seeds[3] = {S.WorldSeed(), 0x5eed5eed12345678ull, 9876543210123ull};
	const double Target[5] = {0.25, 0.35, 0.15, 0.20, 0.05};
	int Miss = 0;
	int WorstLook = 0;
	int WorstRoll = 0;
	int Swept = 0;
	int SweptLong = 0;
	int SweptWorst = 0;
	for (const uint64_t Seed : Seeds)
	{
		weather::Forget();
		std::vector<int> Sky;
		Sky.reserve(static_cast<size_t>(Slots));
		for (int64_t K = From; K < From + Slots; ++K)
		{
			Sky.push_back(static_cast<int>(weather::SlotSky(K, Seed)));
		}
		// Shares over 90 days from the career's first night.
		int Count[5] = {0, 0, 0, 0, 0};
		for (const int V : Sky)
		{
			++Count[V];
		}
		std::string Shares;
		bool Within = true;
		for (int I = 0; I < 5; ++I)
		{
			const double Share = static_cast<double>(Count[I]) / static_cast<double>(Slots);
			Within = Within && std::fabs(Share - Target[I]) <= 0.05;
			char B[32];
			std::snprintf(B, sizeof(B), "%s %.1f%% ", weather::Word(static_cast<weather::Sky>(I)), 100.0 * Share);
			Shares += B;
		}
		std::printf("weather %016llx: %s\n", static_cast<unsigned long long>(Seed), Shares.c_str());
		Expect(Within, "weather: October's shares within 5 points over 90 days (" + Shares + ")");
		// Neighbors only, across every slot boundary (midnights included); storms show 2 h at most.
		bool Neighbors = true;
		int Storm = 0;
		int LongestStorm = 0;
		for (size_t K = 0; K < Sky.size(); ++K)
		{
			if (K > 0)
			{
				Neighbors = Neighbors && std::abs(Sky[K] - Sky[K - 1]) <= 1;
			}
			Storm = Sky[K] == static_cast<int>(weather::Sky::Storm) ? Storm + 1 : 0;
			LongestStorm = std::max(LongestStorm, Storm);
		}
		Expect(Neighbors, "weather: neighbors only across every slot boundary, midnights included");
		Expect(LongestStorm >= 1 && LongestStorm <= 4, "weather: storms show 2 h at most (" + std::to_string(LongestStorm) + " slots)");
		// No state shorter than its slot: the same all through each one.
		bool Whole = true;
		for (int64_t K = From; K < From + 48 * 7; ++K)
		{
			const double T = static_cast<double>(K) * weather::SlotMinutes;
			Whole = Whole && weather::At(T + 0.5, Seed).State == weather::At(T + 29.5, Seed).State && weather::At(T + 15.0, Seed).State == weather::SlotSky(K, Seed);
		}
		Expect(Whole, "weather: no state shorter than 30 minutes");
		// A cache miss looks back 48 (or 96) coupled steps and rolls one step a slot from there; the next slot costs one.
		for (int Probe = 0; Probe < 200; ++Probe)
		{
			const int64_t K = From + 60 + (static_cast<int64_t>(Probe) * 2087) % (Slots - 120);
			weather::Forget();
			const weather::Now Cold = weather::At(static_cast<double>(K) * weather::SlotMinutes + 7.0, Seed);
			const weather::Cost C = weather::LastCost();
			Miss += C.Miss ? 1 : 0;
			WorstLook = std::max(WorstLook, C.LookBack);
			WorstRoll = std::max(WorstRoll, C.Roll);
			Expect(C.Miss && C.LookBack >= 48 && C.LookBack <= 96, "weather: a cold read looks back 48-96 steps (" + std::to_string(C.LookBack) + ")");
			Expect(Cold.State == static_cast<weather::Sky>(Sky[static_cast<size_t>(K - From)]), "weather: reads the same state whichever slot was asked first");
			(void)weather::At(static_cast<double>(K + 1) * weather::SlotMinutes + 1.0, Seed);
			const weather::Cost Next = weather::LastCost();
			Expect(!Next.Miss && Next.LookBack == 0 && Next.Roll == 1, "weather: the next slot is one step");
			(void)weather::At(static_cast<double>(K + 1) * weather::SlotMinutes + 20.0, Seed);
			Expect(weather::LastCost().Roll == 0 && !weather::LastCost().Miss, "weather: the same slot again is cached");
		}
		// Every slot of the 90 days read cold: the look-back's cost never grows with the career (the spec's fallback reaches
		// 384 at most), and the runs not meeting within 96 is a rare thing (measured over 1,200 seeds: about 3 cold reads
		// in a million).
		for (int64_t K = From; K < From + Slots; ++K)
		{
			weather::Forget();
			const int Shown = static_cast<int>(weather::SlotSky(K, Seed));
			const weather::Cost C = weather::LastCost();
			++Swept;
			SweptLong += C.LookBack > 96 ? 1 : 0;
			SweptWorst = std::max(SweptWorst, C.LookBack);
			if (Shown != Sky[static_cast<size_t>(K - From)])
			{
				Expect(false, "weather: a cold read of slot " + std::to_string(K) + " gives the state the warm chain had");
			}
		}
		// Asked backwards from cold, the same states.
		weather::Forget();
		bool Backwards = true;
		for (int64_t K = From + Slots - 1; K >= From + Slots - 500; K -= 7)
		{
			Backwards = Backwards && static_cast<int>(weather::SlotSky(K, Seed)) == Sky[static_cast<size_t>(K - From)];
		}
		Expect(Backwards, "weather: the same states asked in any order");
		// Wetness rises in rain and dries in 5-12 h; puddles fill in it.
		bool Rises = false;
		bool Dries = false;
		for (size_t K = 8; K + 1 < Sky.size(); ++K)
		{
			const double T = static_cast<double>(From + static_cast<int64_t>(K)) * weather::SlotMinutes + 29.0;
			bool Wet = true;
			for (size_t B = 0; B < 4; ++B)
			{
				Wet = Wet && Sky[K - B] >= static_cast<int>(weather::Sky::Rain);
			}
			if (Wet && !Rises)
			{
				const weather::Now N = weather::At(T, Seed);
				Expect(N.Wetness >= 0.95 && N.Puddle >= 0.9 && N.Rain >= 0.75, "weather: two hours of rain soak the street");
				Rises = true;
			}
			bool Dry = K >= 26;
			for (size_t B = 0; Dry && B < 26; ++B)
			{
				Dry = Sky[K - B] <= static_cast<int>(weather::Sky::Overcast);
			}
			if (Dry && !Dries)
			{
				const weather::Now N = weather::At(T, Seed);
				Expect(N.Wetness == 0.0 && N.Puddle == 0.0 && N.Rain == 0.0, "weather: 13 dry hours leave it dry");
				Dries = true;
			}
		}
		Expect(Rises && Dries, "weather: wet and dry spells both come");
		// Outputs in range, and deterministic.
		bool Range = true;
		for (int64_t K = From; K < From + 48 * 10; K += 3)
		{
			const double T = static_cast<double>(K) * weather::SlotMinutes + 4.0;
			const weather::Now A = weather::At(T, Seed, 100.0 + static_cast<double>(K));
			weather::Forget();
			const weather::Now B = weather::At(T, Seed, 100.0 + static_cast<double>(K));
			Range = Range && A.Rain >= 0.0 && A.Rain <= 1.0 && A.Cloud >= 0.2 && A.Cloud <= 1.0 && A.Wind >= 0.1 && A.Wind <= 0.6 && A.Fog >= 0.1 && A.Fog <= 0.6 && A.Wetness >= 0.0 &&
					A.Wetness <= 1.0 && A.Puddle >= 0.0 && A.Puddle <= 1.0 && A.Flash >= 0.0 && A.Flash <= 1.0;
			Expect(A.State == B.State && A.Rain == B.Rain && A.Wetness == B.Wetness && A.Puddle == B.Puddle && A.Wind == B.Wind && A.WindDir == B.WindDir && A.Fog == B.Fog && A.Flash == B.Flash &&
					   A.ThunderIn == B.ThunderIn,
				"weather: deterministic");
			Expect(A.State == weather::Sky::Storm || (A.Flash == 0.0 && A.ThunderIn < 0.0), "weather: lightning only in a storm");
		}
		Expect(Range, "weather: every output in its range");
	}
	// Night One rains: from the career's first minute for 12 hours, the street already soaked.
	for (const uint64_t Seed : Seeds)
	{
		bool Rains = true;
		for (double T = Career; T < Career + 12.0 * 60.0; T += 7.0)
		{
			const weather::Now N = weather::At(T, Seed);
			Rains = Rains && N.State == weather::Sky::Rain && N.Rain >= 0.74;
		}
		Expect(Rains, "weather: Night One rains");
		Expect(weather::At(Career, Seed).Wetness >= 0.99 && weather::At(Career, Seed).Puddle >= 0.99, "weather: the opening's street is soaked");
	}
	// A storm's lightning: a flash every 20-90 s, thunder 1-6 s behind.
	for (const uint64_t Seed : Seeds)
	{
		for (int64_t K = From; K < From + Slots; ++K)
		{
			if (weather::SlotSky(K, Seed) != weather::Sky::Storm)
			{
				continue;
			}
			const double T = static_cast<double>(K) * weather::SlotMinutes + 15.0;
			std::vector<double> Flashes;
			bool Thunder = true;
			bool Was = false;
			for (double R = 0.0; R < 600.0; R += 0.02)
			{
				const weather::Now N = weather::At(T, Seed, R);
				const bool On = N.Flash > 0.5;
				if (On && !Was && (Flashes.empty() || R - Flashes.back() > 1.0))
				{
					Flashes.push_back(R);
					Thunder = Thunder && N.ThunderIn >= 0.9 && N.ThunderIn <= 6.0;
				}
				Was = On;
			}
			bool Gaps = Flashes.size() >= 6;
			for (size_t I = 1; I < Flashes.size(); ++I)
			{
				const double Gap = Flashes[I] - Flashes[I - 1];
				Gaps = Gaps && Gap >= 19.5 && Gap <= 90.5;
			}
			Expect(Gaps, "weather: a flash every 20-90 s (" + std::to_string(Flashes.size()) + " in 10 minutes)");
			Expect(Thunder, "weather: thunder 1-6 s behind");
			break;
		}
	}
	Expect(std::string(weather::Word(weather::Sky::Drizzle)) == "Drizzle", "weather: the words");
	Expect(SweptWorst <= 384 && SweptLong * 10000 <= Swept, "weather: every slot read cold looks back at most 384 steps, and past 96 fewer than 1 in 10,000 (" +
															   std::to_string(SweptLong) + " of " + std::to_string(Swept) + ", the worst " + std::to_string(SweptWorst) + ")");
	std::printf("weather: %d cold reads, the look-back at most %d steps, the roll at most %d; every slot read cold: %d reads, %d past 96, the worst %d\n", Miss, WorstLook,
		WorstRoll, Swept, SweptLong, SweptWorst);
}

/** A fresh cut is one of Lonnie's (body::Sharp's rule, Groom.Cuts): the cut a career starts with, from before the city,
 *  never reads as fresh, however recent the clock makes it. */
void FreshCut()
{
	Hooks H;
	Session S(H, "town-fresh-cut");
	S.NewCareer(Someone());
	Expect(S.Life.Groom.Cuts == 0 && S.Life.Groom.CutAt > -1.0e8, "fresh cut: a career starts with a cut on record, none of Lonnie's");
	Clock(S, DryHour(S, 9, 10, 21)); // Wednesday, a dry hour
	S.Life.Hunger = 20.0;
	S.Life.Thirst = 20.0;
	S.Life.Energy = 80.0;
	town::Bond Known;
	Known.Familiarity = 10;
	Known.Visits = 3;
	Known.LastDay = 8;
	Known.LookSeen = town::LookHash(S.Person.Appearance, S.Life.Closet);
	S.Life.Town["sal"] = Known;
	S.Life.Groom.CutAt = S.WorldMinutes() - 1440.0; // a day ago...
	const town::Talk Seeded = town::Open(*town::ContextFor(S, "sal", "fifth"));
	Expect(Seeded.Topic != "look:cut" && !Seeded.Line.empty(), "fresh cut: a cut that wasn't Lonnie's isn't remarked on (" + Seeded.Topic + ")");
	S.Life.Groom.Cuts = 1; // ...in Lonnie's chair
	const town::Talk Fresh = town::Open(*town::ContextFor(S, "sal", "fifth"));
	Expect(Fresh.Topic == "look:cut", "fresh cut: one of Lonnie's from yesterday is (" + Fresh.Topic + ": " + Fresh.Line + ")");
	S.Life.Groom.CutAt = S.WorldMinutes() - 4.0 * 1440.0;
	Expect(town::Open(*town::ContextFor(S, "sal", "fifth")).Topic != "look:cut", "fresh cut: three days on, it's just hair");
}

void Renown()
{
	Hooks H;
	Session S(H, "town-renown");
	S.NewCareer(Someone());
	const double Fresh = town::Renown(S);
	Expect(Fresh >= 0.0 && Fresh < 0.1, "renown: nobody knows a new career");
	S.Channel.Followers = 25000;
	S.Life.LiveCashes = 6;
	const double Known = town::Renown(S);
	Expect(Known > Fresh + 0.3 && Known <= 1.0, "renown: followers and cashes");
	Expect(town::Tag("lonnie", S) == "Barber" && town::Tag("augie", S) == "Pawnbroker" && town::Tag("sal", S).empty() && town::Tag("nobody", S).empty(), "tags: roles, and strangers");
	const uint32_t A = town::LookHash(S.Person.Appearance, S.Life.Closet);
	hero::Look L = S.Person.Appearance;
	L.Hair = (L.Hair + 1) % hero::OptionCount(hero::Slot::Hair);
	Expect(A != 0 && A != town::LookHash(L, S.Life.Closet), "look: a new cut changes the hash");
}
} // namespace town_test

int main()
{
	using namespace town_test;
	const auto T0 = std::chrono::steady_clock::now();
	People();
	Lines();
	Corner();
	Renown();
	FreshCut();
	Weather();
	TalkOrder();
	Rumours();
	Schedules();
	Crowding();
	std::printf("town_test: %d checks, %d failed (%.1f s)\n", Checks, Failures, std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
	return Failures == 0 ? 0 : 1;
}
