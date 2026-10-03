#include "ShortStack/Game/Live.h"

#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace ss
{
namespace live
{
namespace
{
constexpr int Mon = 1, Tue = 2, Wed = 4, Thu = 8, Fri = 16, Sat = 32, Sun = 64;

int WeekdayOfDay(int Day)
{
	return ((Day % 7) + 7) % 7; // day 0 is Monday, October 5, 2026
}

std::string OccurrenceId(const EventTemplate& T, int Day)
{
	// The Sunday keeps the id it had when it was the room's only event (saves and memories point at it).
	return std::string(T.Key) == "sunday" ? "riverside@" + std::to_string(Day) : "riverside-" + std::string(T.Key) + "@" + std::to_string(Day);
}

Occurrence Make(const EventTemplate& T, int Day)
{
	Occurrence O;
	O.T = &T;
	O.Day = Day;
	O.Id = OccurrenceId(T, Day);
	O.Start = static_cast<double>(Day) * net::MinutesPerDay + static_cast<double>(T.StartMinute);
	O.DeskOpens = O.Start - DeskOpensBefore;
	O.LateRegEnds = O.Start + static_cast<double>(T.LateRegLevels) * T.LevelMinutes;
	// The usual crowd for this event, from its own id: the same night in every save.
	const int Span = std::max(1, T.FieldHi - T.FieldLo + 1);
	O.Field = T.FieldLo + static_cast<int>(Fnv1a(O.Id) % static_cast<uint32_t>(Span));
	return O;
}

std::string Fixed2(double V)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.2f", V);
	return Buf;
}
} // namespace

const std::vector<EventTemplate>& Schedule()
{
	// Key, name, short, days, start, buy-in, fee, stack, level minutes, late-reg levels, break every, break minutes,
	// minimum, field, hours, speed. Two events Monday to Thursday and Sunday, three on Friday and Saturday.
	static const std::vector<EventTemplate> Events = {
		{"noon", "Noon Deepstack $80", "Noon Deepstack", Mon | Tue | Wed | Thu | Fri, 12 * 60, 8000, 1000, 15000, 20.0, 3, 6, 15.0, 18, 60, 80, 9.0, "Deepstack"},
		{"bigstack", "Saturday Big Stack $200", "Big Stack", Sat, 13 * 60, 20000, 2000, 30000, 25.0, 3, 6, 15.0, 18, 90, 120, 11.0, "Deepstack"},
		{"warmup", "Sunday Warm-Up $80", "Warm-Up", Sun, 12 * 60, 8000, 1000, 15000, 20.0, 3, 6, 15.0, 18, 60, 85, 9.0, "Deepstack"},
		{"nightly", "Riverside Nightly $120", "Nightly", Mon | Tue | Wed | Thu | Fri | Sat, 19 * 60, 12000, 1500, 20000, 20.0, 3, 6, 15.0, 18, 70, 100, 8.0, "Deepstack"},
		{"sunday", "Riverside Sunday $150", "Sunday", Sun, 19 * 60, 15000, 1500, 20000, 20.0, 3, 6, 15.0, 18, 90, 120, 9.0, "Deepstack"},
		{"turbo", "Midnight Turbo $60", "Midnight Turbo", Fri | Sat, 22 * 60 + 30, 6000, 800, 10000, 10.0, 4, 6, 10.0, 18, 60, 75, 4.5, "Turbo"},
	};
	return Events;
}

std::vector<Occurrence> Occurrences(int Day)
{
	std::vector<Occurrence> Out;
	const int Bit = 1 << WeekdayOfDay(Day);
	for (const EventTemplate& T : Schedule())
	{
		if ((T.Days & Bit) != 0)
		{
			Out.push_back(Make(T, Day));
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const Occurrence& A, const Occurrence& B) { return A.Start < B.Start; });
	return Out;
}

Occurrence FindOccurrence(const std::string& Id)
{
	const size_t At = Id.find('@');
	if (!IsRiverside(Id) || At == std::string::npos)
	{
		return Occurrence();
	}
	const int Day = std::atoi(Id.c_str() + At + 1);
	for (const Occurrence& O : Occurrences(Day))
	{
		if (O.Id == Id)
		{
			return O;
		}
	}
	return Occurrence();
}

bool IsRiverside(const std::string& EventId)
{
	return EventId.rfind("riverside@", 0) == 0 || EventId.rfind("riverside-", 0) == 0;
}

std::vector<Occurrence> Reachable(double World, double HoursAhead)
{
	std::vector<Occurrence> Out;
	const double Until = World + HoursAhead * 60.0;
	for (int Day = net::DayOf(World) - 1; Day <= net::DayOf(Until); ++Day)
	{
		for (const Occurrence& O : Occurrences(Day))
		{
			if (O.LateRegEnds >= World + TravelMinutes && O.Start <= Until)
			{
				Out.push_back(O);
			}
		}
	}
	return Out;
}

const std::vector<Level>& DeepstackLevels()
{
	static const std::vector<Level> Levels = [] {
		const Chips Blinds[][2] = {{100, 200}, {200, 300}, {200, 400}, {300, 500}, {300, 600}, {400, 800}, {500, 1000}, {600, 1200}, {800, 1600},
			{1000, 2000}, {1200, 2400}, {1500, 3000}, {2000, 4000}, {2500, 5000}, {3000, 6000}, {4000, 8000}, {5000, 10000}, {6000, 12000},
			{8000, 16000}, {10000, 20000}, {15000, 30000}, {20000, 40000}, {25000, 50000}, {30000, 60000}, {40000, 80000}, {50000, 100000},
			{60000, 120000}, {80000, 160000}, {100000, 200000}, {150000, 300000}};
		std::vector<Level> Out;
		for (size_t I = 0; I < sizeof(Blinds) / sizeof(Blinds[0]); ++I)
		{
			Level L;
			L.Sb = Blinds[I][0];
			L.Bb = Blinds[I][1];
			L.Ante = I >= 2 ? Blinds[I][1] : 0;
			Out.push_back(L);
		}
		return Out;
	}();
	return Levels;
}

TournamentSpec SpecFor(const Occurrence& O, int Entrants)
{
	TournamentSpec S;
	if (!O.Valid())
	{
		return S;
	}
	const EventTemplate& T = *O.T;
	S.Id = O.Id;
	S.Name = T.Name;
	S.BuyInCents = T.BuyInCents;
	S.FeeCents = T.FeeCents;
	S.Entrants = Entrants;
	S.StartingStack = T.StartingStack;
	S.LevelMinutes = T.LevelMinutes;
	// Live pace: about two and a half minutes a hand.
	S.SecondsPerHand = 150.0;
	S.Population = "low";
	S.Speed = T.Speed;
	S.StartClock = static_cast<double>(T.StartMinute);
	S.TableSize = RiversideTableSize;
	S.Levels = DeepstackLevels();
	S.BreakEvery = T.BreakEvery;
	S.BreakMinutes = T.BreakMinutes;
	return S;
}

std::unique_ptr<Tournament> MakeField(const Occurrence& O, int Entrants, const std::string& HeroName, const std::string& Seed, const std::vector<ReservedPlayer>& Known)
{
	// The world's people take their seats in the engine's draw like everyone else: whoever the draw puts at the
	// player's table is who they play with.
	std::vector<ReservedPlayer> Field = Known;
	const size_t Room = static_cast<size_t>(std::max(0, Entrants - 1));
	if (Field.size() > Room)
	{
		Field.resize(Room);
	}
	return std::unique_ptr<Tournament>(new Tournament(SpecFor(O, Entrants), HeroName, Seed, Field));
}

std::string SeedFor(const life::LiveEntry& E, const std::string& HeroName)
{
	return E.Id + ":" + HeroName + ":" + Fixed2(E.RegisteredAt);
}

// ------------------------------------------------------------------ the people with faces

const std::vector<CastMember>& RiversideCast()
{
	static const std::vector<CastMember> Cast = {
		{"npc:Sal", "Sal", Archetype::Nit},
		{"npc:Mrs. Park", "Mrs. Park", Archetype::Nit},
		{"npc:Rick", "Rick", Archetype::Lag},
		{"npc:Mei", "Mei", Archetype::Reg},
		{"npc:Dre", "Dre", Archetype::Station},
		{"npc:Big Lou", "Big Lou", Archetype::Station},
		{"npc:Twitch", "Twitch", Archetype::Maniac},
		{"npc:gh0stfold", RivalName, Archetype::Crusher},
	};
	return Cast;
}

int Habit(const std::string& Name, const Occurrence& O)
{
	if (!O.Valid())
	{
		return 0;
	}
	const std::string Key = O.T->Key;
	const int Wd = WeekdayOfDay(O.Day);
	const bool Evening = O.T->StartMinute >= 17 * 60;
	// Dee's game is Tuesday, Thursday and Saturday at nine: her regulars are at the laundromat those nights.
	const bool DeesNight = Wd == 1 || Wd == 3 || Wd == 5;
	const bool DeesRegular = Name == "Sal" || Name == "Big Lou" || Name == "Twitch" || Name == "Mei";
	if (DeesRegular && DeesNight && (Evening || Key == "bigstack"))
	{
		return 0;
	}
	if (Name == "Sal")
	{
		return Key == "nightly" ? 2 : Key == "sunday" ? 1 : 0;
	}
	if (Name == "Mrs. Park")
	{
		// Thirty years of the Sunday tournament, and the noon game every weekday since she retired.
		return Key == "noon" || Key == "sunday" ? 2 : Key == "warmup" ? 1 : 0;
	}
	if (Name == "Rick")
	{
		return Key == "turbo" || Key == "bigstack" ? 2 : Key == "nightly" ? 1 : 0;
	}
	if (Name == "Mei")
	{
		return Key == "sunday" ? 2 : Key == "nightly" || Key == "warmup" ? 1 : 0;
	}
	if (Name == "Dre")
	{
		return Key == "sunday" ? 2 : Key == "nightly" ? 1 : 0;
	}
	if (Name == "Big Lou")
	{
		return Key == "sunday" ? 2 : Key == "nightly" ? 1 : 0;
	}
	if (Name == "Twitch")
	{
		return Key == "turbo" ? 2 : Key == "sunday" ? 1 : 0;
	}
	if (Name == RivalName)
	{
		// An online grinder: the big Saturday and the Sunday, now and then.
		return Key == "sunday" || Key == "bigstack" ? 1 : 0;
	}
	return 0;
}

// ------------------------------------------------------------------ the player's entries

life::LiveEntry* EntryFor(life::State& L, const std::string& OccurrenceId)
{
	for (life::LiveEntry& E : L.LiveEntries)
	{
		if (E.Id == OccurrenceId)
		{
			return &E;
		}
	}
	return nullptr;
}

const life::LiveEntry* EntryFor(const life::State& L, const std::string& OccurrenceId)
{
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.Id == OccurrenceId)
		{
			return &E;
		}
	}
	return nullptr;
}

const life::LiveEntry* ActiveEntry(const life::State& L)
{
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State == life::LiveEntry::Registered)
		{
			return &E;
		}
	}
	return nullptr;
}

std::string CanRegister(Chips Bankroll, const life::State& L, const Occurrence& O, double World)
{
	if (!O.Valid())
	{
		return "There's no such event.";
	}
	if (const life::LiveEntry* Mine = EntryFor(L, O.Id))
	{
		if (Mine->State == life::LiveEntry::Registered)
		{
			return ""; // already in: registering again is a no-op
		}
		return Mine->State == life::LiveEntry::Refunded ? "That one was cancelled." : "You've played that one. It's a freezeout.";
	}
	if (const life::LiveEntry* Other = ActiveEntry(L))
	{
		return "You're registered for the " + Other->Name + ".";
	}
	if (World + TravelMinutes > O.LateRegEnds)
	{
		return "Registration's closed.";
	}
	if (World + TravelMinutes < O.DeskOpens)
	{
		return "The desk opens at " + net::TimeLabel(O.DeskOpens) + ".";
	}
	// The buy-in, and the bus there and back.
	const Chips Need = O.T->BuyInCents + 2 * BusFareCents;
	if (Bankroll < Need)
	{
		return std::string("The ") + O.T->Short + " is " + Money(O.T->BuyInCents) + " and the bus.";
	}
	return "";
}

std::string Register(Chips& Bankroll, life::State& L, const Occurrence& O, double World, int Entrants, const std::vector<std::pair<std::string, int>>& Roster)
{
	const std::string Why = CanRegister(Bankroll, L, O, World);
	if (!Why.empty())
	{
		return Why;
	}
	if (EntryFor(L, O.Id))
	{
		return ""; // already registered
	}
	const EventTemplate& T = *O.T;
	Bankroll -= T.BuyInCents;
	L.Record(World, std::string(T.Name) + ": buy-in", -(T.BuyInCents - T.FeeCents), 0);
	L.Record(World, std::string(T.Name) + ": entry fee", -T.FeeCents, 0);
	life::LiveEntry E;
	E.Id = O.Id;
	E.Name = T.Name;
	E.PaidCents = T.BuyInCents;
	E.FeeCents = T.FeeCents;
	E.RegisteredAt = World;
	E.Entrants = std::max(2, Entrants);
	E.Roster = Roster;
	L.LiveEntries.push_back(E);
	// The last sixty are kept (an entry still in play always is).
	while (L.LiveEntries.size() > 60)
	{
		auto Old = std::find_if(L.LiveEntries.begin(), L.LiveEntries.end(), [](const life::LiveEntry& X) { return X.State != life::LiveEntry::Registered; });
		if (Old == L.LiveEntries.end())
		{
			break;
		}
		L.LiveEntries.erase(Old);
	}
	return "";
}

bool PayFare(Chips& Bankroll, life::State& L, const std::string& Id, bool Home, double World)
{
	life::LiveEntry* E = EntryFor(L, Id);
	if (!E || (Home ? E->FareHome : E->FareThere))
	{
		return false;
	}
	(Home ? E->FareHome : E->FareThere) = true;
	Bankroll -= BusFareCents;
	L.Record(World, Home ? "The 14 bus home" : "The 14 bus to the Riverside", -BusFareCents, 3);
	return true;
}

bool Settle(Chips& Bankroll, life::State& L, const std::string& Id, int Place, int Field, Chips Prize, double World)
{
	life::LiveEntry* E = EntryFor(L, Id);
	if (!E || E->State != life::LiveEntry::Registered)
	{
		return false;
	}
	E->State = life::LiveEntry::Finished;
	E->Place = Place;
	E->Entrants = Field > 0 ? Field : E->Entrants;
	E->PrizeCents = std::max<Chips>(0, Prize);
	E->FinishedAt = World;
	if (E->PrizeCents > 0)
	{
		Bankroll += E->PrizeCents;
		L.Record(World, E->Name + ": " + Ordinal(Place) + " of " + std::to_string(E->Entrants), E->PrizeCents, 4);
	}
	L.LiveEvents += 1;
	L.LiveCashes += E->PrizeCents > 0 ? 1 : 0;
	L.LiveBestPlace = L.LiveBestPlace <= 0 ? Place : std::min(L.LiveBestPlace, Place);
	L.LiveWonCents += E->PrizeCents;
	return true;
}

bool Refund(Chips& Bankroll, life::State& L, const std::string& Id, double World, const std::string& Why)
{
	life::LiveEntry* E = EntryFor(L, Id);
	if (!E || E->State != life::LiveEntry::Registered)
	{
		return false;
	}
	E->State = life::LiveEntry::Refunded;
	E->FinishedAt = World;
	Bankroll += E->PaidCents;
	L.Record(World, E->Name + ": refunded" + (Why.empty() ? std::string() : " (" + Why + ")"), E->PaidCents, 0);
	return true;
}
} // namespace live
} // namespace ss
