#include "ShortStack/Game/Town.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>

// Who is where on the block (the realism update's spec, §6.2; Town.h has the rules). people-core.
//
// Picks recorded here (the spec's ranges and the details it leaves open): the staff's secondary stations (Bea folds at
// the tees table :20-:45 on even hours, Rosa is on the floor :15-:45 on odd hours, Augie at his bench :35-:50, never in a
// shift's first or last half hour; Benny restocks the cooler 4:10-4:40; Lonnie smokes 12 minutes at the door after
// closing); Dee's regulars' arrivals jitter +-3 minutes a night, their leaving is spread 3:00-4:55 at least 6 minutes
// apart (Twitch busts early, 1:30-3:20, on about a third of his nights); Ruby's stools and booths: Sal stool 1, Twitch
// stool 6, Mei booth 2a, Mrs. Park booth 1a, Big Lou booth 4a; Twitch's 4 AM coffee is over before Dee's night is
// settled at five; the shelter takes the world's people on the bus 25 minutes before the event's cards (the 20-minute
// ride and 5 to find a seat), arriving 5-13 minutes before it leaves, four at most (Mrs. Park, from Ruby's, at the first
// spot on a Sunday).
//
// What the world decides is read so it holds for the length of a placement: who sat down to an event comes from
// World::Entered (kept after the result comes in), and nothing someone is in the middle of turns on a number that moves
// while they're there (Twitch's early night at Dee's is his own, not his bankroll's).

namespace ss
{
namespace town
{
namespace town_detail
{
constexpr double Day = 1440.0;

uint64_t Mix(uint64_t Z)
{
	Z += 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

double Unit(uint64_t Seed, int64_t K, uint64_t Salt)
{
	const uint64_t H = Mix(Seed ^ Mix(static_cast<uint64_t>(K) ^ (Salt * 0x632be59bd9b4e019ull)));
	return static_cast<double>(H >> 11) * (1.0 / 9007199254740992.0);
}

uint64_t Salt(const std::string& Name)
{
	return live::FaceHash(Name);
}

int64_t DayAt(double World)
{
	return static_cast<int64_t>(std::floor(World / Day));
}

int Weekday(int64_t D)
{
	return net::Weekday(static_cast<int>(D));
}

/** Minutes jittered by a seeded amount in [Lo, Hi]. */
double Jitter(uint64_t Seed, int64_t D, const std::string& Who, uint64_t What, double Lo, double Hi)
{
	return std::floor(Lo + (Hi - Lo + 1.0) * Unit(Seed ^ Salt(Who), D, What));
}

void Add(std::vector<Placement>& Out, const std::string& Id, const std::string& Venue, const std::string& Station, Doing What, double Since, double Until)
{
	if (Until > Since)
	{
		Placement P;
		P.Id = Id;
		P.Venue = Venue;
		P.Station = Station;
		P.What = What;
		P.Since = Since;
		P.Until = Until;
		Out.push_back(P);
	}
}

/** The staff with their own shifts in the venues that never close (§5.0), minutes after midnight (To <= From wraps). */
struct Shift
{
	const char* Person;
	const char* Venue;
	const char* Station;
	int From;
	int To;
};
const Shift Shifts[] = {
	{"ray", "store", "store-counter", 10 * 60, 22 * 60},
	{"benny", "store", "store-counter", 22 * 60, 10 * 60},
	{"ruby", "diner", "diner-counter", 6 * 60, 18 * 60},
	{"marlene", "diner", "diner-counter", 18 * 60, 6 * 60},
	{"cook", "diner", "diner-pass", 0, 24 * 60},
	{"inez", "washfold", "washfold-counter", 0, 24 * 60},
};

/** The keepers of the shops with hours: on whenever their venue is staffed (shop::Venues()), at their station, and now
 *  and then at another (the minutes past the hour, on even, odd or every hour). */
struct Keeper
{
	const char* Person;
	const char* Venue;
	const char* Station;
	const char* Other; // nullptr: none
	int OtherFrom;
	int OtherTo;
	int Parity; // 0 even hours, 1 odd, 2 every hour
};
const Keeper Keepers[] = {
	{"lonnie", "barber", "barber-chair1", nullptr, 0, 0, 2},
	{"bea", "thrift", "thrift-register", "thrift-fold", 20, 45, 0},
	{"rosa", "gym", "gym-desk", "gym-floor", 15, 45, 1},
	{"augie", "pawn", "pawn-counter", "pawn-bench", 35, 50, 2},
};

const std::vector<shop::Window>& HoursOf(const shop::Venue& V)
{
	return V.Staffed.empty() ? V.Open : V.Staffed;
}

/** The venue's staffed windows that open on day D, as world minutes. */
std::vector<std::pair<double, double>> WindowsOn(const shop::Venue& V, int64_t D)
{
	std::vector<std::pair<double, double>> Out;
	const int Bit = 1 << Weekday(D);
	for (const shop::Window& W : HoursOf(V))
	{
		if ((W.Days & Bit) == 0)
		{
			continue;
		}
		const double From = static_cast<double>(D) * Day + W.Opens;
		const double To = static_cast<double>(D) * Day + (W.Closes <= W.Opens ? W.Closes + Day : W.Closes);
		Out.push_back({From, To});
	}
	std::sort(Out.begin(), Out.end());
	return Out;
}

/** The staff's day D: every stretch that starts on it. */
void StaffDay(int64_t D, std::vector<Placement>& Out)
{
	const double D0 = static_cast<double>(D) * Day;
	for (const Shift& S : Shifts)
	{
		const double From = D0 + S.From;
		const double To = D0 + (S.To <= S.From ? S.To + Day : S.To);
		if (std::string(S.Person) == "benny")
		{
			// The dead hour: he restocks the cooler.
			const double Stock = D0 + Day + 4 * 60 + 10;
			Add(Out, S.Person, S.Venue, S.Station, Doing::Work, From, Stock);
			Add(Out, S.Person, S.Venue, "store-stock", Doing::Work, Stock, Stock + 30.0);
			Add(Out, S.Person, S.Venue, S.Station, Doing::Work, Stock + 30.0, To);
			continue;
		}
		Add(Out, S.Person, S.Venue, S.Station, Doing::Work, From, To);
	}
	for (const Keeper& K : Keepers)
	{
		const shop::Venue* V = shop::FindVenue(K.Venue);
		if (!V)
		{
			continue;
		}
		for (const std::pair<double, double>& W : WindowsOn(*V, D))
		{
			double At = W.first;
			if (K.Other)
			{
				for (double Hour = std::ceil(W.first / 60.0) * 60.0; Hour < W.second; Hour += 60.0)
				{
					const long long H = static_cast<long long>(std::llround(Hour / 60.0)) % 24;
					const bool Fits = K.Parity == 2 || (H % 2) == K.Parity;
					const double A = Hour + K.OtherFrom;
					const double B = Hour + K.OtherTo;
					if (!Fits || A < W.first + 30.0 || B > W.second - 30.0)
					{
						continue;
					}
					Add(Out, K.Person, K.Venue, K.Station, Doing::Work, At, A);
					Add(Out, K.Person, K.Venue, K.Other, Doing::Work, A, B);
					At = B;
				}
			}
			Add(Out, K.Person, K.Venue, K.Station, Doing::Work, At, W.second);
			if (std::string(K.Person) == "lonnie" && !shop::IsStaffed(*V, W.second + 1.0))
			{
				// Closing: the broom, then a smoke by the pole.
				Add(Out, K.Person, K.Venue, "barber-door", Doing::Smoke, W.second, W.second + 12.0);
			}
		}
	}
}

// ---- the living world's people on the block

const world::Npc* Npc(const world::World& W, const char* Name)
{
	return W.Get(W.Find(Name));
}

bool Away(const world::Npc* N, int64_t D)
{
	return !N || N->Faded || (N->TripUntil >= D && D >= N->TripFrom);
}

bool Down(const world::Npc* N)
{
	return N && (N->St == world::Status::Broke || N->Mood == world::Momentum::Downswing || N->Mood == world::Momentum::MajorDownswing);
}

bool DeesNight(int64_t D)
{
	const int Wd = Weekday(D);
	return Wd == 1 || Wd == 3 || Wd == 5;
}

/** At one of the Embercrest's evening events on day D (17:00 on): registered, or played it (World::Entered). */
bool AtTheRidge(const world::World& W, int Npc, int64_t D)
{
	for (const live::Occurrence& O : live::Occurrences(static_cast<int>(D)))
	{
		if (O.Start - static_cast<double>(D) * Day < 17 * 60)
		{
			continue;
		}
		const std::vector<int> Who = W.Entered(O.Id);
		if (std::find(Who.begin(), Who.end(), Npc) != Who.end())
		{
			return true;
		}
	}
	return false;
}

/** Dee's regulars as the world knows them, and their ids on the block. */
struct Regular
{
	const char* World;
	const char* Id;
	int Arrives;   // minutes after midnight
	int Walk;      // minutes walking up from the stop
};
const Regular DeesRegulars[] = {
	{"Sal", "sal", 20 * 60 + 40, 4},
	{"Mei", "mei", 20 * 60 + 50, 4},
	{"Big Lou", "big-lou", 20 * 60 + 55, 3},
	{"Twitch", "twitch", 21 * 60 + 10, 2},
};

/** When each of Dee's regulars the world registered for night D leaves (world minutes): one by one, 3:00-4:55. */
std::vector<std::pair<std::string, double>> Leaving(const world::World& W, uint64_t Seed, int64_t D)
{
	std::vector<std::pair<std::string, double>> Out;
	if (!DeesNight(D))
	{
		return Out;
	}
	const std::vector<int> Who = W.Entered("dees@" + std::to_string(D));
	const double Next = static_cast<double>(D + 1) * Day;
	for (const Regular& R : DeesRegulars)
	{
		const int Id = W.Find(R.World);
		if (Id < 0 || std::find(Who.begin(), Who.end(), Id) == Who.end())
		{
			continue;
		}
		double At = Next + 180.0 + Jitter(Seed, D, R.Id, 3, 0.0, 110.0);
		if (std::string(R.Id) == "twitch" && Unit(Seed ^ Salt(R.Id), D, 4) < 0.35)
		{
			// Busted early, as he does. Seeded by the night, not read from his bankroll: that moves while he's at the table
			// (an online result, the night itself), and his leaving (and everyone's after his) must not move with it.
			At = Next + 90.0 + Jitter(Seed, D, R.Id, 5, 0.0, 110.0);
		}
		Out.push_back({R.Id, At});
	}
	std::sort(Out.begin(), Out.end(), [](const std::pair<std::string, double>& A, const std::pair<std::string, double>& B) { return A.second < B.second || (A.second == B.second && A.first < B.first); });
	for (size_t I = 1; I < Out.size(); ++I)
	{
		Out[I].second = std::min(Next + 295.0, std::max(Out[I].second, Out[I - 1].second + 6.0));
	}
	return Out;
}

/** Night D at Dee's: the regulars walking up, Sal's smoke, the table, and their leaving. Dee herself at the table. */
void DeesDay(const world::World& W, uint64_t Seed, int64_t D, std::vector<Placement>& Out)
{
	if (!DeesNight(D))
	{
		return;
	}
	const double D0 = static_cast<double>(D) * Day;
	Add(Out, "dee", "washfold", "dee-table", Doing::Work, D0 + 20 * 60 + 30, D0 + Day + 5 * 60 + 15);
	for (const std::pair<std::string, double>& L : Leaving(W, Seed, D))
	{
		for (const Regular& R : DeesRegulars)
		{
			if (L.first != R.Id)
			{
				continue;
			}
			const double At = D0 + R.Arrives + Jitter(Seed, D, R.Id, 2, -3.0, 3.0);
			Add(Out, R.Id, "washfold", "washfold-door", Doing::Arrive, At - R.Walk, At);
			double Sits = At;
			if (L.first == "sal")
			{
				Sits = D0 + 20 * 60 + 58;
				Add(Out, R.Id, "washfold", "washfold-door", Doing::Smoke, At, Sits);
			}
			Add(Out, R.Id, "washfold", "dee-table", Doing::Sit, Sits, L.second);
			Add(Out, R.Id, "washfold", "washfold-door", Doing::Leave, L.second, L.second + 5.0);
		}
	}
}

/** A visit: walking in, sitting, walking out. */
void Visit(std::vector<Placement>& Out, const std::string& Id, const std::string& Venue, const std::string& Seat, double From, double To, double Walk)
{
	Add(Out, Id, Venue, Seat, Doing::Arrive, From - Walk, From);
	Add(Out, Id, Venue, Seat, Doing::Sit, From, To);
	Add(Out, Id, Venue, Venue == "diner" ? "diner-door" : Seat, Doing::Leave, To, To + Walk);
}

/** Mrs. Park at Ruby's before the Sunday Warm-Up (then the 11:35 from the shelter's first spot). */
bool ParkAtRubys(const world::World& W, int64_t D)
{
	const int ParkId = W.Find("Mrs. Park");
	return Weekday(D) == 6 && !Away(W.Get(ParkId), D) && W.NextEmbercrest(ParkId, static_cast<double>(D) * Day + 600.0) == "embercrest-warmup@" + std::to_string(D);
}

/** Ruby's regulars on day D (§6.3): never more than two at once. */
void RubysDay(const world::World& W, uint64_t Seed, int64_t D, std::vector<Placement>& Out)
{
	const double D0 = static_cast<double>(D) * Day;
	const int Wd = Weekday(D);
	// Sal at the counter with the paper, weekday mornings.
	const world::Npc* Sal = Npc(W, "Sal");
	if (Wd <= 4 && !Away(Sal, D))
	{
		Visit(Out, "sal", "diner", "diner-stool1", D0 + 7 * 60 + Jitter(Seed, D, "sal", 10, -5.0, 10.0), D0 + 9 * 60 + 30 + Jitter(Seed, D, "sal", 11, -10.0, 10.0), 3.0);
	}
	// Mrs. Park before the Sunday Warm-Up, then the 11:35 to the ridge.
	if (ParkAtRubys(W, D))
	{
		Visit(Out, "mrs-park", "diner", "diner-booth1a", D0 + 10 * 60 + 30, D0 + 11 * 60 + 30, 3.0);
		Out.pop_back(); // she leaves for the stop, not up the street
		Add(Out, "mrs-park", "market-stop", "market-shelter1", Doing::Walk, D0 + 11 * 60 + 30, D0 + 11 * 60 + 33);
		Add(Out, "mrs-park", "market-stop", "market-shelter1", Doing::Wait, D0 + 11 * 60 + 33, D0 + 11 * 60 + 35);
	}
	// Mei in a booth on her online nights (weeknights she isn't at Dee's): the early hours of Monday, Tuesday, Thursday.
	const int MeiId = W.Find("Mei");
	const world::Npc* Mei = W.Get(MeiId);
	if ((Wd == 0 || Wd == 1 || Wd == 3) && !Away(Mei, D) && Mei->Playing() && Mei->Schedule == world::Plan::Nights && !AtTheRidge(W, MeiId, D - 1))
	{
		Visit(Out, "mei", "diner", "diner-booth2a", D0 + 2 * 60 + Jitter(Seed, D, "mei", 10, -5.0, 10.0), D0 + 5 * 60 + Jitter(Seed, D, "mei", 11, -15.0, 5.0), 3.0);
	}
	// Twitch about 4:00 after a bust, when the world has him broke (not every night): a coffee he can't pay for, gone
	// before five (when Dee's night is settled and his money moves). From Dee's table only if he's out of her door first.
	const world::Npc* Twitch = Npc(W, "Twitch");
	if (!Away(Twitch, D) && Down(Twitch) && Unit(Seed ^ Salt("twitch"), D, 12) < 0.45)
	{
		constexpr double Walk = 2.0;
		const double Sits = D0 + 3 * 60 + 55 + Jitter(Seed, D, "twitch", 13, -5.0, 10.0);
		bool Free = true;
		for (const std::pair<std::string, double>& L : Leaving(W, Seed, D - 1))
		{
			Free = Free && !(L.first == "twitch" && L.second + 5.0 > Sits - Walk); // his walk out of the Wash & Fold comes first
		}
		if (Free)
		{
			Visit(Out, "twitch", "diner", "diner-stool6", Sits, D0 + 4 * 60 + 45 + Jitter(Seed, D, "twitch", 14, -5.0, 5.0), Walk);
		}
	}
	// Big Lou on a Friday night, when he isn't up at the ridge.
	const int LouId = W.Find("Big Lou");
	const world::Npc* Lou = W.Get(LouId);
	if (Wd == 4 && !Away(Lou, D) && !AtTheRidge(W, LouId, D))
	{
		Visit(Out, "big-lou", "diner", "diner-booth4a", D0 + 23 * 60 + Jitter(Seed, D, "big-lou", 10, -10.0, 10.0), D0 + Day + 60 + Jitter(Seed, D, "big-lou", 11, -10.0, 15.0), 3.0);
	}
}

/** The world's people waiting at the 14's shelter for the Embercrest's events on day D. */
void ShelterDay(const world::World& W, uint64_t Seed, int64_t D, const std::vector<int>& Regulars, std::vector<Placement>& Out)
{
	for (const live::Occurrence& O : live::Occurrences(static_cast<int>(D)))
	{
		const double Leaves = BusFor(O.Start);
		if (Leaves < 0.0)
		{
			continue;
		}
		const std::vector<int> Who = W.Entered(O.Id);
		std::vector<std::string> Riders;
		// Mrs. Park walks over from Ruby's to the shelter's first spot (RubysDay): the others wait further along.
		const size_t Taken = std::string(O.T->Key) == "warmup" && ParkAtRubys(W, D) ? size_t{1} : size_t{0};
		// The block's own first (they live here), then 1-3 of the room's regulars.
		for (const Person& P : People())
		{
			if (!P.InWorld)
			{
				continue;
			}
			const int Id = W.Find(P.Name);
			if (Id < 0 || std::find(Who.begin(), Who.end(), Id) == Who.end())
			{
				continue;
			}
			if (P.Id == "mrs-park" && Taken > 0)
			{
				continue; // from Ruby's (RubysDay)
			}
			Riders.push_back(P.Id);
		}
		std::vector<std::pair<double, int>> Locals;
		for (const int Id : Regulars)
		{
			if (std::find(Who.begin(), Who.end(), Id) != Who.end())
			{
				Locals.push_back({Unit(Seed, D * 4096 + Id, 15), Id});
			}
		}
		std::sort(Locals.begin(), Locals.end());
		const size_t Want = 1 + static_cast<size_t>(std::floor(3.0 * Unit(Seed ^ Salt(O.Id), D, 16)));
		for (size_t I = 0; I < Locals.size() && I < Want; ++I)
		{
			Riders.push_back("npc:" + W.Get(Locals[I].second)->Name);
		}
		for (size_t I = 0; I < Riders.size() && I + Taken < 4; ++I)
		{
			const std::string Station = "market-shelter" + std::to_string(I + Taken + 1);
			const double At = Leaves - Jitter(Seed, D, Riders[I] + O.Id, 17, 5.0, 13.0);
			Add(Out, Riders[I], "market-stop", Station, Doing::Arrive, At - 3.0, At);
			Add(Out, Riders[I], "market-stop", Station, Doing::Wait, At, Leaves);
		}
	}
}

/** Everyone's stretches that start on day D. */
void DayPlan(const Session& S, int64_t D, const std::vector<int>& Regulars, std::vector<Placement>& Out)
{
	StaffDay(D, Out);
	const world::World& W = S.Living();
	if (!W.Ready())
	{
		return;
	}
	const uint64_t Seed = S.WorldSeed();
	DeesDay(W, Seed, D, Out);
	RubysDay(W, Seed, D, Out);
	ShelterDay(W, Seed, D, Regulars, Out);
}

/** Linear between the hours' values (each at its half hour). */
double CrowdAt(int64_t D, int Hour)
{
	while (Hour < 0)
	{
		Hour += 24;
		--D;
	}
	while (Hour >= 24)
	{
		Hour -= 24;
		++D;
	}
	// Monday 0 .. Sunday 6.
	const int Wd = Weekday(D);
	static const double Workday[24] = {0.22, 0.14, 0.08, 0.05, 0.05, 0.09, 0.22, 0.5, 0.85, 0.8, 0.55, 0.58, 0.7, 0.66, 0.52, 0.56, 0.68, 0.92, 0.95, 0.78, 0.58, 0.46, 0.38, 0.3};
	static const double Friday[24] = {0.22, 0.14, 0.08, 0.05, 0.05, 0.09, 0.22, 0.5, 0.85, 0.8, 0.55, 0.58, 0.7, 0.66, 0.52, 0.58, 0.72, 0.95, 0.96, 0.84, 0.8, 0.76, 0.72, 0.66};
	static const double Saturday[24] = {0.58, 0.45, 0.2, 0.08, 0.06, 0.08, 0.12, 0.2, 0.32, 0.45, 0.6, 0.72, 0.8, 0.82, 0.8, 0.78, 0.75, 0.78, 0.8, 0.82, 0.8, 0.76, 0.72, 0.68};
	static const double Sunday[24] = {0.6, 0.46, 0.2, 0.08, 0.06, 0.07, 0.1, 0.15, 0.25, 0.38, 0.52, 0.62, 0.68, 0.66, 0.62, 0.6, 0.58, 0.55, 0.5, 0.45, 0.38, 0.3, 0.24, 0.18};
	static const double Monday[24] = {0.14, 0.1, 0.06, 0.05, 0.05, 0.09, 0.22, 0.5, 0.85, 0.8, 0.55, 0.58, 0.7, 0.66, 0.52, 0.56, 0.68, 0.92, 0.95, 0.78, 0.58, 0.46, 0.38, 0.3};
	const double* T = Wd == 0 ? Monday : Wd == 4 ? Friday : Wd == 5 ? Saturday : Wd == 6 ? Sunday : Workday;
	return T[Hour];
}

// ---- the save lines' strings (Session.cpp's escaping, which is private to it)

std::string Escape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\')
		{
			Out += "\\\\";
		}
		else if (Ch == '\n')
		{
			Out += "\\n";
		}
		else if (Ch == '\t')
		{
			Out += "\\t";
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::string Unescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char Next = In[++I];
			Out.push_back(Next == 'n' ? '\n' : Next == 't' ? '\t' : Next);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

int Int(const std::string& S)
{
	return static_cast<int>(std::strtol(S.c_str(), nullptr, 10));
}
} // namespace town_detail

using namespace town_detail;

const std::vector<Person>& People()
{
	static const std::vector<Person> All = [] {
		std::vector<Person> L;
		auto P = [&L](const char* Id, const char* Name, Job Role, const char* Venue, const char* Cast, bool Woman, int Age, bool InWorld) {
			Person X;
			X.Id = Id;
			X.Name = Name;
			X.Role = Role;
			X.Venue = Venue;
			X.Cast = Cast;
			X.Woman = Woman;
			X.Age = Age;
			X.InWorld = InWorld;
			L.push_back(X);
		};
		// The block's cast (§6.1). Cast: the build their face comes from as CastLooks assigns it (a staff build until it
		// lands borrows a reserved extra there).
		P("benny", "Benny", Job::Clerk, "store", "ExtraB", false, 34, false);
		P("ray", "Ray", Job::Clerk, "store", "Ray", false, 56, false);
		P("lonnie", "Lonnie", Job::Barber, "barber", "Lonnie", false, 64, false);
		P("bea", "Bea", Job::Thrift, "thrift", "Bea", true, 71, false);
		P("rosa", "Rosa", Job::Trainer, "gym", "Rosa", true, 46, false);
		P("ruby", "Ruby", Job::Waitress, "diner", "Ruby", true, 63, false);
		P("marlene", "Marlene", Job::Waitress, "diner", "Marlene", true, 54, false);
		P("augie", "Augie", Job::Pawnbroker, "pawn", "ExtraH", false, 67, false);
		P("cook", "Cook", Job::Cook, "diner", "", false, 49, false);
		P("inez", "Inez", Job::Attendant, "washfold", "ExtraK", true, 41, false);
		P("dee", "Dee", Job::Host, "washfold", "Dee", true, 56, false);
		P("marcus", "Marcus", Job::Local, "", "", false, 31, false);
		// The living world's people met on the block (their table MetaHumans, §6.3).
		P("sal", "Sal", Job::Regular, "", "Sal", false, 58, true);
		P("mei", "Mei", Job::Regular, "", "Mei", true, 24, true);
		P("big-lou", "Big Lou", Job::Regular, "", "BigLou", false, 52, true);
		P("twitch", "Twitch", Job::Regular, "", "Twitch", false, 22, true);
		P("mrs-park", "Mrs. Park", Job::Local, "", "MrsPark", true, 67, true);
		return L;
	}();
	return All;
}

const Person* FindPerson(const std::string& Id)
{
	for (const Person& P : People())
	{
		if (P.Id == Id)
		{
			return &P;
		}
	}
	return nullptr;
}

std::vector<Placement> Plan(double From, double To, const Session& S)
{
	std::vector<Placement> All;
	const std::vector<int> Regulars = S.Living().Ready() ? S.Living().EmbercrestRegulars() : std::vector<int>();
	for (int64_t D = DayAt(From) - 1; D <= DayAt(To); ++D)
	{
		DayPlan(S, D, Regulars, All);
	}
	std::vector<Placement> Out;
	for (const Placement& P : All)
	{
		if (P.Until > From && P.Since <= To)
		{
			Out.push_back(P);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const Placement& A, const Placement& B) { return A.Since < B.Since; });
	return Out;
}

std::vector<Placement> Who(double World, const Session& S)
{
	std::vector<Placement> Out;
	std::set<std::string> Seen;
	for (const Placement& P : Plan(World, World, S))
	{
		if (P.Since <= World && World < P.Until && Seen.insert(P.Id).second)
		{
			Out.push_back(P);
		}
	}
	return Out;
}

bool Where(const std::string& Id, double World, const Session& S, Placement& Out)
{
	for (const Placement& P : Who(World, S))
	{
		if (P.Id == Id)
		{
			Out = P;
			return true;
		}
	}
	return false;
}

std::string OnShift(const std::string& Venue, double World)
{
	const double Minute = World - static_cast<double>(DayAt(World)) * Day;
	for (const Shift& S : Shifts)
	{
		if (Venue != S.Venue || std::string(S.Person) == "cook")
		{
			continue;
		}
		const bool On = S.To > S.From ? Minute >= S.From && Minute < S.To : Minute >= S.From || Minute < S.To;
		if (On)
		{
			return S.Person;
		}
	}
	for (const Keeper& K : Keepers)
	{
		if (Venue == K.Venue)
		{
			const shop::Venue* V = shop::FindVenue(K.Venue);
			return V && shop::IsStaffed(*V, World) ? K.Person : "";
		}
	}
	return "";
}

double CrowdLevel(double World, double Rain)
{
	const double Hours = (World - static_cast<double>(DayAt(World)) * Day) / 60.0 - 0.5;
	const int H = static_cast<int>(std::floor(Hours));
	const double F = Hours - static_cast<double>(H);
	const int64_t D = DayAt(World);
	const double Level = CrowdAt(D, H) + (CrowdAt(D, H + 1) - CrowdAt(D, H)) * F;
	return std::min(1.0, std::max(0.0, Level * (1.0 - 0.3 * std::min(1.0, std::max(0.0, Rain)))));
}

SignalState Signal(double World)
{
	static const double Phases[6] = {42.0, 4.0, 3.0, 26.0, 4.0, 3.0};
	constexpr double Cycle = 82.0;
	constexpr double Walk = 7.0;
	SignalState Out;
	const double Seconds = World * 60.0;
	const double Midnight = static_cast<double>(DayAt(World)) * 86400.0;
	double SinceFive = Seconds - Midnight - 5.0 * 3600.0;
	if (SinceFive < 0.0)
	{
		SinceFive += 86400.0;
	}
	// The cycle runs from 5:00 AM; the one running at 1:00 AM finishes, then the corner flashes until 5:00.
	const double RunEnds = Cycle * std::ceil((20.0 * 3600.0) / Cycle);
	if (SinceFive >= RunEnds)
	{
		const double Half = std::fmod(SinceFive, 1.0);
		Out.Phase = 0;
		Out.Flashing = true;
		Out.FlashOn = Half < 0.5;
		Out.Left = Half < 0.5 ? 0.5 - Half : 1.0 - Half;
		return Out;
	}
	double C = std::fmod(SinceFive, Cycle);
	int Phase = 0;
	while (Phase < 5 && C >= Phases[Phase])
	{
		C -= Phases[Phase];
		++Phase;
	}
	Out.Phase = Phase + 1;
	Out.Left = Phases[Phase] - C;
	Out.FifthGo = Out.Phase == 1;
	Out.MarketGo = Out.Phase == 4;
	Out.WalkMarket = Out.Phase == 1 && C < Walk;
	Out.WalkFifth = Out.Phase == 4 && C < Walk;
	if ((Out.Phase == 1 || Out.Phase == 4) && C >= Walk)
	{
		Out.Countdown = static_cast<int>(std::ceil(Out.Left));
	}
	return Out;
}

std::vector<double> BusTimes(const std::string& Stop, double World, int N)
{
	std::vector<double> Out;
	const bool East = Stop == "market-east";
	if ((!East && Stop != "market-west") || N <= 0)
	{
		return Out;
	}
	// Minutes after midnight: half-hourly through the day, hourly overnight.
	std::vector<int> Daily;
	const int First = East ? 6 * 60 + 5 : 6 * 60 + 20;
	for (int T = First; T < 24 * 60; T += 30)
	{
		Daily.push_back(T);
	}
	for (int H = 0; H < 6; ++H)
	{
		Daily.push_back(H * 60 + (East ? 35 : 50));
	}
	std::sort(Daily.begin(), Daily.end());
	for (int64_t D = DayAt(World); static_cast<int>(Out.size()) < N; ++D)
	{
		for (const int T : Daily)
		{
			const double At = static_cast<double>(D) * Day + T;
			if (At >= World && static_cast<int>(Out.size()) < N)
			{
				Out.push_back(At);
			}
		}
	}
	return Out;
}

double BusFor(double Start)
{
	double Best = -1.0;
	for (const double T : BusTimes("market-east", Start - 120.0, 8))
	{
		if (T + live::TravelMinutes + 5.0 <= Start)
		{
			Best = T;
		}
	}
	return Best;
}

int BarberQueue(double World)
{
	const shop::Venue* V = shop::FindVenue("barber");
	if (!V || !shop::IsOpen(*V, World))
	{
		return 0;
	}
	const int64_t Quarter = static_cast<int64_t>(std::floor(World / 15.0));
	const double U = Unit(0x6669667468626172ull, Quarter, 1); // "fifthbar"
	const int64_t D = DayAt(World);
	const double Hour = (World - static_cast<double>(D) * Day) / 60.0;
	const int Wd = Weekday(D);
	if (Wd == 5 && Hour >= 10.0 && Hour < 14.0)
	{
		return 10 + 5 * static_cast<int>(std::floor(U * 4.0)); // Saturday morning: 10-25
	}
	const bool Busy = (Hour >= 12.0 && Hour < 14.0) || (Hour >= 17.0 && Hour < 19.0) || ((Wd == 4 || Wd == 5) && Hour >= 19.0);
	const double Chance = Busy ? 0.5 : 0.18;
	if (U >= Chance)
	{
		return 0;
	}
	return U < Chance * 0.4 ? 10 : 5;
}

double Renown(const Session& S)
{
	// The public record's formula (next_segment_spec §1.1), so the town and talk keep their behavior: the titles on the
	// wall are the wins among the entries kept (the last sixty), as the town has always counted them. rep::Public::Titles
	// counts them for life (Life.LiveWins), which differs once a career passes sixty live entries; moving the town onto
	// that is a behavior change, not a Phase A seam.
	rep::Public P = S.Record();
	int Wall = 0;
	for (const life::LiveEntry& E : S.Life.LiveEntries)
	{
		Wall += E.State == life::LiveEntry::Finished && E.Place == 1 ? 1 : 0;
	}
	P.Titles = Wall;
	return rep::RenownOf(P);
}

uint32_t LookHash(const hero::Look& L, const wardrobe::Closet& C)
{
	uint32_t H = 2166136261u;
	auto Feed = [&H](int V) {
		for (int B = 0; B < 4; ++B)
		{
			H = (H ^ static_cast<uint32_t>((static_cast<unsigned>(V) >> (8 * B)) & 0xffu)) * 16777619u;
		}
	};
	for (const int V : {L.Hair, L.HairColor, L.FacialHair, L.Brows, L.Glasses, L.Hat, L.Outfit, L.OutfitColor})
	{
		Feed(V);
	}
	for (const std::string& Worn : C.Worn)
	{
		for (const char Ch : Worn)
		{
			H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
		}
		H = (H ^ 0x2fu) * 16777619u;
	}
	return H == 0 ? 1u : H;
}

std::string Tag(const std::string& Id, const Session& S)
{
	const Person* P = FindPerson(Id);
	const bool Local = Id.rfind("npc:", 0) == 0;
	if ((P && P->InWorld) || Local)
	{
		const world::World& W = S.Living();
		const world::Bond* B = W.BondWith(W.Find(Local ? Id.substr(4) : P->Name));
		return B ? B->Label() : std::string();
	}
	if (!P)
	{
		return "";
	}
	switch (P->Role)
	{
	case Job::Clerk: return Id == "ray" ? "Owner" : "Night clerk";
	case Job::Barber: return "Barber";
	case Job::Thrift: return "Volunteer";
	case Job::Trainer: return "Trainer";
	case Job::Waitress: return Id == "ruby" ? "Owner" : "Waitress";
	case Job::Cook: return "Cook";
	case Job::Pawnbroker: return "Pawnbroker";
	case Job::Host: return "Runs the game";
	case Job::Attendant: return "Attendant";
	default: break;
	}
	return "";
}

std::string CheckKey(const Session& S, double World)
{
	Placement P;
	if (!Where("big-lou", World, S, P) || P.Venue != "diner" || P.What != Doing::Sit)
	{
		return "";
	}
	return "check:big-lou:" + std::to_string(DayAt(P.Since));
}

std::string PaysYourCheck(const Session& S, double World)
{
	// The cheap question first (a live cash in the last 7 days), then where he is: the shops price an order with this
	// every frame, and the town's plan is the dear part.
	const bool Cashed = std::any_of(S.Life.LiveEntries.begin(), S.Life.LiveEntries.end(), [World](const life::LiveEntry& E) {
		return E.State == life::LiveEntry::Finished && E.PrizeCents > 0 && E.FinishedAt >= World - 7.0 * Day && E.FinishedAt <= World;
	});
	if (!Cashed)
	{
		return "";
	}
	const std::string Key = CheckKey(S, World);
	if (Key.empty() || S.Life.Said.count(Key) > 0)
	{
		return ""; // not here, or he's had the one already tonight
	}
	return "big-lou";
}

void WriteLines(const std::map<std::string, Bond>& Town, const std::map<std::string, int>& Said, int Today, std::string& Out)
{
	for (const auto& It : Town)
	{
		const Bond& B = It.second;
		if (B.Familiarity <= 0)
		{
			continue; // nobody remembers a stranger
		}
		Out += "town\t" + Escape(It.first) + "\t" + std::to_string(B.Familiarity) + "\t" + std::to_string(B.Visits) + "\t" + std::to_string(B.LastDay) + "\t" +
			   std::to_string(B.LookSeen) + "\t" + std::to_string(B.Flags) + "\n";
	}
	for (const auto& It : Said)
	{
		const int When = std::min(It.second, Today); // SaidToday (a line from before the day was kept) reads as today
		if (!SaidFresh(When, Today))
		{
			continue;
		}
		Out += "said\t" + Escape(It.first) + "\t" + std::to_string(When) + "\n";
	}
}

bool ReadLine(const std::vector<std::string>& Fields, std::map<std::string, Bond>& Town, std::map<std::string, int>& Said)
{
	if (Fields.empty())
	{
		return false;
	}
	if (Fields[0] == "town" && Fields.size() >= 7)
	{
		Bond B;
		B.Familiarity = Int(Fields[2]);
		B.Visits = Int(Fields[3]);
		B.LastDay = Int(Fields[4]);
		B.LookSeen = static_cast<uint32_t>(std::strtoul(Fields[5].c_str(), nullptr, 10));
		B.Flags = Int(Fields[6]);
		Town[Unescape(Fields[1])] = B;
		return true;
	}
	if (Fields[0] == "said" && Fields.size() >= 2)
	{
		Said[Unescape(Fields[1])] = Fields.size() >= 3 && !Fields[2].empty() ? Int(Fields[2]) : SaidToday;
		return true;
	}
	return false;
}
} // namespace town
} // namespace ss
