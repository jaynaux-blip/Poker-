#include "ShortStack/Game/Live.h"

#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ss
{
namespace live
{
namespace
{
constexpr int Mon = 1, Tue = 2, Wed = 4, Thu = 8, Fri = 16, Sat = 32, Sun = 64;
/** Live pace: about two and a half minutes a hand. */
constexpr double LiveSecondsPerHand = 150.0;

int WeekdayOfDay(int Day)
{
	return ((Day % 7) + 7) % 7; // day 0 is Monday, October 5, 2026
}

std::string OccurrenceId(const EventTemplate& T, int Day)
{
	// The Sunday keeps the id it had when it was the room's only event (saves and memories point at it).
	return std::string(T.Key) == "sunday" ? "embercrest@" + std::to_string(Day) : "embercrest-" + std::string(T.Key) + "@" + std::to_string(Day);
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

// The next segment's save lines and the live record (next_segment_spec §7.4, §7.5). Named, not anonymous: Unreal's
// unity builds put several .cpp files in one translation unit.
namespace live_save_detail
{
/** Tabs, newlines and backslashes escaped, as Session.cpp's lines are (its Escape is private to that file). */
std::string Esc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
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

std::string Unesc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

/** A voice's ties as one field, "name:kind;name:kind" (a backslash keeps a ':', a ';' or a backslash in a name). */
std::string TiesText(const std::vector<std::pair<std::string, int>>& Ties)
{
	std::string Out;
	for (const std::pair<std::string, int>& T : Ties)
	{
		if (!Out.empty())
		{
			Out.push_back(';');
		}
		for (const char Ch : T.first)
		{
			if (Ch == ':' || Ch == ';' || Ch == '\\')
			{
				Out.push_back('\\');
			}
			Out.push_back(Ch);
		}
		Out += ":" + std::to_string(T.second);
	}
	return Out;
}

std::vector<std::pair<std::string, int>> ReadTies(const std::string& Text)
{
	std::vector<std::pair<std::string, int>> Out;
	std::string Name;
	std::string Kind;
	bool InKind = false;
	auto Flush = [&]() {
		if (!Name.empty() || !Kind.empty())
		{
			Out.push_back({Name, std::atoi(Kind.c_str())});
		}
		Name.clear();
		Kind.clear();
		InKind = false;
	};
	for (size_t I = 0; I < Text.size(); ++I)
	{
		const char Ch = Text[I];
		if (Ch == '\\' && I + 1 < Text.size())
		{
			(InKind ? Kind : Name).push_back(Text[++I]);
		}
		else if (Ch == ':' && !InKind)
		{
			InKind = true;
		}
		else if (Ch == ';')
		{
			Flush();
		}
		else
		{
			(InKind ? Kind : Name).push_back(Ch);
		}
	}
	Flush();
	return Out;
}

std::string Num3(float V)
{
	return Fixed(static_cast<double>(V), 3);
}

/** A finished entry's day in the live record: its occurrence's (the finish's, for an id the schedule doesn't know). */
void AddDay(std::vector<life::LiveDay>& Days, const life::LiveEntry& E)
{
	const Occurrence O = FindOccurrence(E.Id);
	const int Day = O.Valid() ? O.Day : net::DayOf(E.FinishedAt);
	auto It = std::lower_bound(Days.begin(), Days.end(), Day, [](const life::LiveDay& D, int Want) { return D.Day < Want; });
	if (It == Days.end() || It->Day != Day)
	{
		life::LiveDay Fresh;
		Fresh.Day = Day;
		It = Days.insert(It, Fresh);
	}
	const bool Cashed = E.PrizeCents > 0;
	It->Events += 1;
	It->Cashes += Cashed ? 1 : 0;
	if (IsEmbercrest(E.Id))
	{
		It->Embercrest += 1;
		It->EmbercrestCashes += Cashed ? 1 : 0;
	}
	It->SpentCents += E.PaidCents;
	It->WonCents += E.PrizeCents;
	// The last LiveDaysKept days with play, counted back from the newest.
	const int From = Days.back().Day - life::LiveDaysKept;
	Days.erase(Days.begin(), std::find_if(Days.begin(), Days.end(), [From](const life::LiveDay& D) { return D.Day > From; }));
}

/** What the record keeps of a settled entry (live::Settle): a title, what it cost, its day. */
void Keep(life::State& L, const life::LiveEntry& E)
{
	L.LiveWins += E.Place == 1 ? 1 : 0;
	L.LiveSpentCents += E.PaidCents;
	AddDay(L.LiveDays, E);
}

/** The night's snapshot goes with the checkpoint at settle and refund (§1.8); the player's own patch rows stay. */
void ClearNight(life::LiveEntry& E)
{
	E.Voices.clear();
	E.Room.clear();
	E.Badges.clear();
	E.Leaders.clear();
	E.Patches.erase(std::remove_if(E.Patches.begin(), E.Patches.end(),
						[&E](const std::pair<std::string, std::string>& P) {
							return std::any_of(E.Roster.begin(), E.Roster.end(), [&P](const std::pair<std::string, int>& R) { return R.first == P.first; });
						}),
		E.Patches.end());
}
} // namespace live_save_detail

const std::vector<EventTemplate>& Schedule()
{
	// Key, name, short, days, start, buy-in, fee, stack, level minutes, late-reg levels, break every, break minutes,
	// minimum, field, hours, speed. Two events Monday to Thursday and Sunday, three on Friday and Saturday. The biggest
	// fields stop at 114 so that, the player in, the room's twenty tables seat everyone (6-max).
	static const std::vector<EventTemplate> Events = {
		{"noon", "Noon Deepstack $80", "Noon Deepstack", Mon | Tue | Wed | Thu | Fri, 12 * 60, 8000, 1000, 15000, 20.0, 3, 6, 15.0, 18, 60, 80, 9.0, "Deepstack"},
		{"bigstack", "Saturday Big Stack $200", "Big Stack", Sat, 13 * 60, 20000, 2000, 30000, 25.0, 3, 6, 15.0, 18, 90, 114, 11.0, "Deepstack"},
		{"warmup", "Sunday Warm-Up $80", "Warm-Up", Sun, 12 * 60, 8000, 1000, 15000, 20.0, 3, 6, 15.0, 18, 60, 85, 9.0, "Deepstack"},
		{"nightly", "Embercrest Nightly $120", "Nightly", Mon | Tue | Wed | Thu | Fri | Sat, 19 * 60, 12000, 1500, 20000, 20.0, 3, 6, 15.0, 18, 70, 100, 8.0, "Deepstack"},
		{"sunday", "Embercrest Sunday $150", "Sunday", Sun, 19 * 60, 15000, 1500, 20000, 20.0, 3, 6, 15.0, 18, 90, 114, 9.0, "Deepstack"},
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
	if (!IsEmbercrest(Id) || At == std::string::npos)
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

bool IsEmbercrest(const std::string& EventId)
{
	return EventId.rfind("embercrest@", 0) == 0 || EventId.rfind("embercrest-", 0) == 0;
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

Occurrence NextOpen(double World, double Arrive, const std::string& Except)
{
	const double There = World + std::max(0.0, Arrive);
	for (int Day = net::DayOf(World) - 1; Day <= net::DayOf(World) + 1; ++Day)
	{
		for (const Occurrence& O : Occurrences(Day))
		{
			if (O.Id != Except && O.LateRegEnds >= There && O.Start <= World + net::MinutesPerDay)
			{
				return O;
			}
		}
	}
	return Occurrence();
}

double DeepRunMinutes(const Occurrence& O, int Entrants)
{
	if (!O.Valid())
	{
		return 0.0;
	}
	// The last hand comes at about the big blind that the chips in play cover 3.4 times for every hand of a level (the
	// more hands a level, the more of the field goes out in each), between two levels by the ratio of their blinds:
	// measured against the engine's own nights (session_test). The breaks fall on their level boundaries.
	const EventTemplate& T = *O.T;
	const double InPlay = static_cast<double>(std::max(2, Entrants)) * static_cast<double>(T.StartingStack);
	const double Last = InPlay / (3.4 * T.LevelMinutes * 60.0 / LiveSecondsPerHand);
	const std::vector<Level>& Levels = DeepstackLevels();
	double Played = static_cast<double>(Levels.size());
	for (size_t I = 0; I < Levels.size(); ++I)
	{
		const double Bb = static_cast<double>(Levels[I].Bb);
		if (Bb >= Last)
		{
			const double Below = I > 0 ? static_cast<double>(Levels[I - 1].Bb) : Bb / 2.0;
			Played = static_cast<double>(I) - 1.0 + std::log(Last / Below) / std::log(Bb / Below);
			break;
		}
	}
	Played = std::max(0.5, Played);
	const double Breaks = T.BreakEvery > 0 ? std::floor(Played / static_cast<double>(T.BreakEvery)) : 0.0;
	return Played * T.LevelMinutes + Breaks * T.BreakMinutes;
}

std::string DayWord(const Occurrence& O, double World)
{
	if (!O.Valid())
	{
		return std::string();
	}
	const bool Evening = O.T->StartMinute >= 17 * 60;
	const int Days = O.Day - net::DayOf(World);
	switch (Days)
	{
	case 0: return Evening ? "tonight" : "today";
	case 1: return Evening ? "tomorrow night" : "tomorrow";
	case -1: return Evening ? "last night" : "yesterday";
	default: break;
	}
	return Days > -7 && Days < 7 ? std::string("on ") + net::WeekdayName(O.Day, true) : "on " + net::DateLabel(O.Day);
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
	S.SecondsPerHand = LiveSecondsPerHand;
	S.Population = "low";
	S.Speed = T.Speed;
	S.StartClock = static_cast<double>(T.StartMinute);
	S.TableSize = EmbercrestTableSize;
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

std::unique_ptr<Tournament> ResumeField(const Occurrence& O, const life::LiveEntry& E, int Entrants, const std::string& HeroName,
	const std::vector<ReservedPlayer>& Known, bool bRestore, bool& bRestored)
{
	std::unique_ptr<Tournament> Night = MakeField(O, Entrants, HeroName, SeedFor(E, HeroName), Known);
	bRestored = false;
	if (!bRestore || E.Checkpoint.empty())
	{
		return Night;
	}
	bRestored = Night->Restore(E.Checkpoint);
	// The ids were renamed (the seed and the checkpoint's field print both carry them): an old night's checkpoint
	// only fits the field drawn under its old id.
	static const std::string Now = "embercrest", Was = "riverside";
	if (!bRestored && E.Id.rfind(Now, 0) == 0 && O.Id.rfind(Now, 0) == 0)
	{
		Occurrence Legacy = O;
		Legacy.Id = Was + O.Id.substr(Now.size());
		life::LiveEntry LegacyEntry = E;
		LegacyEntry.Id = Was + E.Id.substr(Now.size());
		std::unique_ptr<Tournament> Old = MakeField(Legacy, Entrants, HeroName, SeedFor(LegacyEntry, HeroName), Known);
		if (Old->Restore(E.Checkpoint))
		{
			bRestored = true;
			return Old;
		}
	}
	return Night;
}

// ------------------------------------------------------------------ the people with faces

const std::vector<CastMember>& EmbercrestCast()
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

const std::vector<RoomLocal>& RoomLocals()
{
	static const std::vector<RoomLocal> Locals = {
		// The cash list's names first.
		{"Boots", false, 61, 0.44}, {"T.J.", false, 38, 0.5}, {"Maria G.", true, 47, 0.46}, {"Omar", false, 33, 0.52}, {"Lin", true, 29, 0.55},
		{"Duke", false, 66, 0.4}, {"Bev", true, 58, 0.42}, {"Sonny", false, 44, 0.45}, {"J.P.", false, 51, 0.48}, {"Rosie", true, 63, 0.43},
		{"Hector", false, 40, 0.47}, {"Gloria", true, 55, 0.44}, {"Vince", false, 49, 0.41}, {"Tammy", true, 42, 0.39}, {"Earl", false, 72, 0.43},
		{"Danny Q.", false, 27, 0.53}, {"Priya", true, 31, 0.5}, {"Walt", false, 68, 0.45}, {"Connie", true, 52, 0.4}, {"Nina", true, 36, 0.49},
		{"Big Sam", false, 45, 0.38}, {"Kevin", false, 24, 0.46}, {"Lorraine", true, 60, 0.42}, {"Teddy", false, 35, 0.44}, {"Arturo", false, 57, 0.47},
		{"Shirley", true, 70, 0.41}, {"Moe", false, 64, 0.46}, {"Fitz", false, 39, 0.5}, {"Carla", true, 34, 0.45}, {"Jimmy Two", false, 50, 0.37},
		{"Yolanda", true, 46, 0.44}, {"Rusty", false, 59, 0.39}, {"Deb", true, 53, 0.43}, {"Ozzie", false, 30, 0.48}, {"Frankie", false, 43, 0.42},
		{"Gus", false, 74, 0.44}, {"Ines", true, 28, 0.51}, {"Ray-Ray", false, 26, 0.4}, {"Patty", true, 62, 0.4},
	};
	return Locals;
}

const RoomLocal* FindRoomLocal(const std::string& Name)
{
	for (const RoomLocal& L : RoomLocals())
	{
		if (Name == L.Name)
		{
			return &L;
		}
	}
	return nullptr;
}

uint32_t FaceHash(const std::string& Name)
{
	uint32_t H = 2166136261u;
	for (const char Ch : Name)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
	}
	return H;
}

int RegularHabit(const std::string& Name, const Occurrence& O)
{
	if (!O.Valid())
	{
		return 0;
	}
	const uint32_t H = FaceHash(Name);
	static const char* Games[10] = {"nightly", "nightly", "nightly", "nightly", "noon", "noon", "sunday", "sunday", "bigstack", "turbo"};
	const std::string Mine = Games[H % 10u];
	const std::string Also = Games[(H / 10u) % 10u];
	// Their nights, for the games that run most days: two, sometimes three.
	const int Days = (1 << ((H >> 8) % 7u)) | (1 << ((H >> 12) % 7u)) | ((H >> 16) % 3u == 0 ? 1 << ((H >> 20) % 7u) : 0);
	const std::string Key = O.T->Key;
	const bool MostDays = Key == "nightly" || Key == "noon";
	if (Key == Mine && (!MostDays || (Days & (1 << WeekdayOfDay(O.Day))) != 0))
	{
		return 2;
	}
	if (Key == Mine || Key == Also)
	{
		return 1;
	}
	// The Sunday is the room's big night.
	return Key == "sunday" ? 1 : 0;
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

std::string CanRegister(Chips Bankroll, const life::State& L, const Occurrence& O, double World, const life::Funding& F)
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
	// The buy-in and the bus there and back. The player's own money pays the buy-in (less what pieces sold brought in);
	// anyone else's pays it for them; a sponsor covering the fares pays the bus too (§1.2).
	const bool Own = F.From == life::Paid::Own;
	const Chips Need = (Own ? std::max<Chips>(0, O.T->BuyInCents - F.Cents) : 0) + (F.Fares ? 0 : 2 * BusFareCents);
	if (Bankroll < Need)
	{
		return Own ? std::string("The ") + O.T->Short + " is " + Money(O.T->BuyInCents) + " and the bus." : "The bus up the ridge and back is " + Money(2 * BusFareCents) + ".";
	}
	return "";
}

std::string Register(Chips& Bankroll, life::State& L, const Occurrence& O, double World, int Entrants, const std::vector<std::pair<std::string, int>>& Roster,
	const life::Funding& F)
{
	const std::string Why = CanRegister(Bankroll, L, O, World, F);
	if (!Why.empty())
	{
		return Why;
	}
	if (EntryFor(L, O.Id))
	{
		return ""; // already registered
	}
	const EventTemplate& T = *O.T;
	life::LiveEntry E;
	E.Id = O.Id;
	E.Name = T.Name;
	E.PaidCents = T.BuyInCents;
	E.FeeCents = T.FeeCents;
	E.RegisteredAt = World;
	E.Entrants = std::max(2, Entrants);
	E.Roster = Roster;
	E.Fund = F;
	L.LiveEntries.push_back(E);
	// Someone else's money first, one line before the buy-in (§1.2): a sponsor's cover, a backer's stake, or the one line
	// for the pieces a listing on this event sold. Each feature writes its own line once, keyed by the entry, and nothing
	// when it has nothing on it (the stubs write nothing). They get the entry as registered (E: a copy, so what they're
	// handed can't move under them).
	sponsor::OnRegister(L.Sponsors, L, Bankroll, E, World);
	stake::OnRegister(L.Stakes, Bankroll, L, E, World);
	if (F.From == life::Paid::Package)
	{
		// The package's roll pays (stake::OnRegister took it): nothing in the player's ledger.
	}
	else if (F.From == life::Paid::Ticket)
	{
		L.Record(World, std::string(T.Name) + ": ticket", 0, 0);
	}
	else
	{
		Bankroll -= T.BuyInCents;
		L.Record(World, std::string(T.Name) + ": buy-in", -(T.BuyInCents - T.FeeCents), 0);
		L.Record(World, std::string(T.Name) + ": entry fee", -T.FeeCents, 0);
	}
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
	// From here on the live record is kept (an older save's was worked out from the kept entries, which this may thin).
	L.SegmentLinesRead |= life::SegLiveWins | life::SegLiveRecord;
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
	L.Record(World, Home ? "The 14 bus home" : "The 14 bus up to the Embercrest", -BusFareCents, 3);
	if (E->Fund.Fares)
	{
		// A sponsor covering the bus: the fare and its cover, two lines that net to nothing (R4's travel lines survive).
		Bankroll += BusFareCents;
		L.Record(World, (E->Fund.Who.empty() ? std::string("Your sponsor") : E->Fund.Who) + ": covered the bus", BusFareCents, life::LedgerSponsors);
	}
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
	E->Checkpoint.clear();
	E->CheckpointHost.clear();
	E->Place = Place;
	E->Entrants = Field > 0 ? Field : E->Entrants;
	E->PrizeCents = std::max<Chips>(0, Prize);
	E->FinishedAt = World;
	if (E->PrizeCents > 0)
	{
		Bankroll += E->PrizeCents;
		L.Record(World, E->Name + ": " + Ordinal(Place) + " of " + std::to_string(E->Entrants), E->PrizeCents, 4);
	}
	// Then the sponsor (the contract is senior: a backer buys action on what the player keeps), then the backer (§1.2).
	// Each settles once, keyed by the entry; they get it as settled (a copy: what they're handed can't move under them).
	const life::LiveEntry Settled = *E;
	sponsor::Settled(L.Sponsors, L, Bankroll, Settled);
	stake::OnSettle(L.Stakes, Bankroll, L, Settled, Settled.PrizeCents, World);
	// Then the record: the room's counters, the titles, what the entries cost and the day.
	L.LiveEvents += 1;
	L.LiveCashes += Settled.PrizeCents > 0 ? 1 : 0;
	L.LiveBestPlace = L.LiveBestPlace <= 0 ? Place : std::min(L.LiveBestPlace, Place);
	L.LiveWonCents += Settled.PrizeCents;
	live_save_detail::Keep(L, Settled);
	L.SegmentLinesRead |= life::SegLiveWins | life::SegLiveRecord;
	if (life::LiveEntry* Done = EntryFor(L, Id))
	{
		live_save_detail::ClearNight(*Done);
	}
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
	E->Checkpoint.clear();
	E->CheckpointHost.clear();
	E->FinishedAt = World;
	// The registration's two lines, reversed (the bank counts ": refunded" lines against the buy-ins); a package's roll
	// and a ticket wrote none in the player's ledger.
	if (E->Fund.From != life::Paid::Package && E->Fund.From != life::Paid::Ticket)
	{
		Bankroll += E->PaidCents;
		L.Record(World, E->Name + ": refunded buy-in" + (Why.empty() ? std::string() : " (" + Why + ")"), E->PaidCents - E->FeeCents, 0);
		if (E->FeeCents > 0)
		{
			L.Record(World, E->Name + ": refunded entry fee", E->FeeCents, 0);
		}
	}
	// Then the funding's lines, reversed: the backer and the pieces' buyers repaid (markup included), a swap called off,
	// the cover returned (and not counted as used). Each once, keyed by the entry.
	const life::LiveEntry Refunded = *E;
	stake::OnRefund(L.Stakes, Bankroll, L, Refunded, World);
	sponsor::OnRefund(L.Sponsors, L, Bankroll, Refunded, World);
	if (life::LiveEntry* Done = EntryFor(L, Id))
	{
		live_save_detail::ClearNight(*Done);
	}
	return true;
}

// ------------------------------------------------------------------ the field (§1.8)

Archetype PlayStyle(const world::Npc& N)
{
	const float Sk = N.Overall();
	const float Aggro = N.TraitOf(world::Trait::Aggro);
	const float Risk = N.TraitOf(world::Trait::Risk);
	if (Sk >= 0.72f)
	{
		return Aggro > 0.6f ? Archetype::Crusher : Archetype::Reg;
	}
	if (Sk >= 0.58f)
	{
		return Aggro > 0.62f ? Archetype::Lag : Aggro < 0.35f ? Archetype::Tag : Archetype::Reg;
	}
	if (Sk >= 0.45f)
	{
		return Aggro > 0.8f && Risk > 0.7f ? Archetype::Maniac : Aggro < 0.3f ? Archetype::Nit : Archetype::Tag;
	}
	return Aggro > 0.8f ? Archetype::Maniac : Aggro < 0.3f ? Archetype::Station : N.TraitOf(world::Trait::Patience) > 0.7f ? Archetype::Nit : Archetype::Fish;
}

FieldNotes DescribeField(world::World& W, const Occurrence& O, const std::string& HeroName, const life::State& Hero, double World)
{
	(void)Hero; // the player's own patches are sponsor::NoteWorn's, after registration
	FieldNotes N;
	N.Planned = O.Field;
	if (!O.Valid())
	{
		return N;
	}
	// The field as the world has it: the people it registered (the room's faces as the Back Room knows them play their
	// own way), and everyone else the room expects. Among them, the faces: the room's regulars, and whoever remembers
	// the player (what they'll say sitting down).
	if (W.Ready())
	{
		N.Planned = std::max(1, W.PlannedEntries(O.Id));
		const std::vector<int> Regulars = W.EmbercrestRegulars();
		std::vector<int> Field;
		for (int Npc : W.Registered(O.Id))
		{
			const world::Npc* P = W.Get(Npc);
			if (!P || P->Name == HeroName)
			{
				continue;
			}
			int Type = static_cast<int>(PlayStyle(*P));
			for (const CastMember& Cm : EmbercrestCast())
			{
				if (P->Name == Cm.Name)
				{
					Type = static_cast<int>(Cm.Type);
				}
			}
			N.Roster.push_back({P->Name, Type});
			Field.push_back(Npc);
			// How they sound and what they can honestly talk about (§4): only once core-room makes voices.
			life::LiveVoice V = room::VoiceOf(W, Npc, HeroName);
			if (!V.Name.empty())
			{
				N.Voices.push_back(std::move(V));
			}
			// The world's sponsored people wear their brand's patch (§3; Npc::Sponsor holds the brand's name).
			if (!P->Sponsor.empty())
			{
				if (const sponsor::Brand* B = sponsor::FindBrandByName(P->Sponsor))
				{
					N.Patches.push_back({P->Name, B->Id});
				}
			}
			const world::Bond* Bd = W.BondWith(Npc);
			const bool Regular = std::find(Regulars.begin(), Regulars.end(), Npc) != Regulars.end();
			const std::string Known = Bd ? Bd->Label() : std::string();
			if (!Regular && Known.empty() && !(Bd && !Bd->Memories.empty()))
			{
				continue;
			}
			life::LiveFace F;
			F.Name = P->Name;
			F.Label = !Known.empty() ? Known : Bd && !Bd->Memories.empty() ? std::string("Has met you") : std::string("Embercrest regular");
			F.Line = W.SpokenGreeting(Npc, FaceHash(O.Id + P->Name));
			N.Faces.push_back(F);
		}
		// Their badges and the month's room leaders (§5).
		const int Day = net::DayOf(World);
		N.Badges = badge::FieldBadges(W, Field, Day);
		N.Leaders = badge::FieldLeaders(W, Day);
	}
	// The champions' board by the desk: the room's last winners, newest first, from the world's results.
	static const char* Days[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
	for (int Day = O.Day; Day >= O.Day - 10 && N.Board.size() < 8; --Day)
	{
		const std::vector<Occurrence> Os = Occurrences(Day);
		for (auto It = Os.rbegin(); It != Os.rend() && N.Board.size() < 8; ++It)
		{
			const net::EventResult* Res = It->Start < World ? W.ResultOf(It->Id) : nullptr;
			if (!Res)
			{
				continue;
			}
			for (const net::Placing& Pl : Res->FinalTable)
			{
				if (Pl.Place != 1)
				{
					continue;
				}
				const world::Npc* P = Pl.Player >= 0 ? W.Get(Pl.Player) : nullptr;
				const std::string Who = Pl.Player == -1 ? HeroName : P ? P->Name : Pl.Name;
				std::string Short = It->T->Short;
				for (char& Ch : Short)
				{
					Ch = static_cast<char>(std::toupper(static_cast<unsigned char>(Ch)));
				}
				N.Board.push_back(std::string(Days[((Day % 7) + 7) % 7]) + " " + Short + "\t" + Who + "\t" + std::to_string(Pl.Prize));
			}
		}
	}
	return N;
}

void Apply(const FieldNotes& N, life::LiveEntry& E)
{
	E.Faces = N.Faces;
	E.Board = N.Board;
	E.Voices = N.Voices;
	E.Badges = N.Badges;
	E.Leaders = N.Leaders;
	E.Patches.insert(E.Patches.end(), N.Patches.begin(), N.Patches.end());
}

std::string FundingLine(const life::Funding& F, const std::string& EventShort)
{
	const std::string Event = EventShort.empty() ? std::string("event") : EventShort;
	switch (F.From)
	{
	case life::Paid::Own:
	{
		if (F.Permille <= 0)
		{
			return std::string();
		}
		const std::string Share = F.Permille % 10 == 0 ? std::to_string(F.Permille / 10) : std::to_string(F.Permille / 10) + "." + std::to_string(F.Permille % 10);
		return "You've sold " + Share + "% of your " + Event + ".";
	}
	case life::Paid::Backer: return (F.Who.empty() ? std::string("Your backer") : F.Who) + " is putting you in the " + Event + ".";
	case life::Paid::Package: return (F.Who.empty() ? std::string("The package") : F.Who) + " pays for the " + Event + " from the roll.";
	case life::Paid::Sponsor: return (F.Who.empty() ? std::string("Your sponsor") : F.Who) + " covers the " + Event + (F.Fares ? ", and the bus." : ".");
	case life::Paid::Ticket: return "A ticket pays for the " + Event + ".";
	}
	return std::string();
}

// ------------------------------------------------------------------ the save (§7.4, §7.5)

void WriteEntryLines(const life::LiveEntry& E, std::string& Out)
{
	using live_save_detail::Esc;
	using live_save_detail::Num3;
	const std::string Id = Esc(E.Id);
	if (!E.Room.empty())
	{
		Out += "liveroom\t" + Id + "\t" + Esc(E.Room) + "\n";
	}
	if (!life::IsOwnMoney(E.Fund))
	{
		const life::Funding& F = E.Fund;
		Out += "livefund\t" + Id + "\t" + std::to_string(static_cast<int>(F.From)) + "\t" + Esc(F.Ref) + "\t" + Esc(F.Who) + "\t" + std::to_string(F.Permille) + "\t" +
			std::to_string(F.Cents) + "\t" + std::to_string(F.CutPct) + "\t" + (F.Fares ? "1" : "0") + "\n";
	}
	for (const life::LiveVoice& V : E.Voices)
	{
		Out += "livevoice\t" + Id + "\t" + Esc(V.Name) + "\t" + std::to_string(V.Style) + "\t" + Num3(V.Social) + "\t" + Num3(V.Aggro) + "\t" + Num3(V.Grit) + "\t" +
			std::to_string(V.Mood) + "\t" + std::to_string(V.Is) + "\t" + std::to_string(V.Age) + "\t" + (V.Pro ? "1" : "0") + "\t" + (V.Staked ? "1" : "0") + "\t" +
			Num3(V.Rivalry) + "\t" + Num3(V.Familiarity) + "\t" + Num3(V.Respect) + "\t" + Esc(V.Story) + "\t" + Esc(live_save_detail::TiesText(V.Ties)) + "\n";
	}
	for (const std::pair<std::string, std::string>& B : E.Badges)
	{
		Out += "livebadge\t" + Id + "\t" + Esc(B.first) + "\t" + Esc(B.second) + "\n";
	}
	for (const life::RoomLeader& Ld : E.Leaders)
	{
		Out += "liveleader\t" + Id + "\t" + std::to_string(Ld.Rank) + "\t" + Esc(Ld.Name) + "\t" + Fixed(Ld.Points, 2) + "\t" + Esc(Ld.Badge) + "\n";
	}
	for (const std::pair<std::string, std::string>& P : E.Patches)
	{
		Out += "livepatch\t" + Id + "\t" + Esc(P.first) + "\t" + Esc(P.second) + "\n";
	}
}

bool ReadEntryLine(const std::vector<std::string>& P, life::State& L)
{
	if (P.empty())
	{
		return false;
	}
	const std::string& Key = P[0];
	const bool Ours = Key == "liveroom" || Key == "livefund" || Key == "livevoice" || Key == "livebadge" || Key == "liveleader" || Key == "livepatch";
	if (!Ours)
	{
		return false;
	}
	using live_save_detail::Unesc;
	L.SegmentLinesRead |= life::SegLiveLines;
	life::LiveEntry* E = P.size() >= 2 ? EntryFor(L, Unesc(P[1])) : nullptr;
	if (!E)
	{
		return true; // its entry wasn't kept: the line goes with it
	}
	// Every line parses with at least its fields, so later fields can be appended (N6).
	auto Num = [&P](size_t I) { return I < P.size() ? std::atof(P[I].c_str()) : 0.0; };
	auto Int = [&P](size_t I) { return I < P.size() ? std::atoi(P[I].c_str()) : 0; };
	auto Flt = [&P](size_t I) { return I < P.size() ? static_cast<float>(std::atof(P[I].c_str())) : 0.0f; };
	if (Key == "liveroom" && P.size() >= 3)
	{
		E->Room = Unesc(P[2]);
	}
	else if (Key == "livefund" && P.size() >= 9)
	{
		life::Funding F;
		F.From = static_cast<life::Paid>(std::clamp(Int(2), 0, static_cast<int>(life::Paid::Ticket)));
		F.Ref = Unesc(P[3]);
		F.Who = Unesc(P[4]);
		F.Permille = Int(5);
		F.Cents = static_cast<Chips>(std::strtoll(P[6].c_str(), nullptr, 10));
		F.CutPct = Int(7);
		F.Fares = P[8] == "1";
		E->Fund = F;
	}
	else if (Key == "livevoice" && P.size() >= 17)
	{
		life::LiveVoice V;
		V.Name = Unesc(P[2]);
		V.Style = Int(3);
		V.Social = Flt(4);
		V.Aggro = Flt(5);
		V.Grit = Flt(6);
		V.Mood = Int(7);
		V.Is = Int(8);
		V.Age = Int(9);
		V.Pro = P[10] == "1";
		V.Staked = P[11] == "1";
		V.Rivalry = Flt(12);
		V.Familiarity = Flt(13);
		V.Respect = Flt(14);
		V.Story = Unesc(P[15]);
		V.Ties = live_save_detail::ReadTies(Unesc(P[16]));
		E->Voices.push_back(V);
	}
	else if (Key == "livebadge" && P.size() >= 4)
	{
		E->Badges.push_back({Unesc(P[2]), Unesc(P[3])});
	}
	else if (Key == "liveleader" && P.size() >= 6)
	{
		life::RoomLeader Ld;
		Ld.Rank = Int(2);
		Ld.Name = Unesc(P[3]);
		Ld.Points = Num(4);
		Ld.Badge = Unesc(P[5]);
		E->Leaders.push_back(Ld);
	}
	else if (Key == "livepatch" && P.size() >= 4)
	{
		E->Patches.push_back({Unesc(P[2]), Unesc(P[3])});
	}
	return true;
}

void AfterParse(SaveData& D)
{
	life::State& L = D.Life;
	// Titles: the finished entries won (a win older than the sixty kept still counts once, from the best place).
	if ((L.SegmentLinesRead & life::SegLiveWins) == 0)
	{
		int Wins = 0;
		for (const life::LiveEntry& E : L.LiveEntries)
		{
			Wins += E.State == life::LiveEntry::Finished && E.Place == 1 ? 1 : 0;
		}
		L.LiveWins = Wins == 0 && L.LiveBestPlace == 1 ? 1 : Wins;
	}
	if ((L.SegmentLinesRead & life::SegLiveRecord) == 0)
	{
		// What the entries cost: the kept ones' buy-ins, and their average for the events the sixty kept don't cover (an
		// estimate, the only one an older save allows). The days come from the kept entries only: older days are unknown.
		Chips Kept = 0;
		int Played = 0;
		L.LiveDays.clear();
		for (const life::LiveEntry& E : L.LiveEntries)
		{
			if (E.State == life::LiveEntry::Finished)
			{
				Kept += E.PaidCents;
				++Played;
				live_save_detail::AddDay(L.LiveDays, E);
			}
		}
		const int Missing = std::max(0, L.LiveEvents - Played);
		L.LiveSpentCents = Kept + (Played > 0 ? Kept * static_cast<Chips>(Missing) / static_cast<Chips>(Played) : 0);
	}
	if ((L.SegmentLinesRead & life::SegShift) == 0)
	{
		// The last shift: the newest shift line in the ledger (newest first); failing that, with shifts on the record, the
		// oldest line the ledger kept (sixty lines: the shift was before it).
		L.LastShiftAt = -1.0e9;
		const auto Shift = std::find_if(L.Ledger.begin(), L.Ledger.end(), [](const life::LedgerEntry& X) { return X.Kind == 1; });
		if (Shift != L.Ledger.end())
		{
			L.LastShiftAt = Shift->At;
		}
		else if (L.Shifts > 0 && !L.Ledger.empty())
		{
			L.LastShiftAt = L.Ledger.back().At;
		}
	}
	// An entry still in play from before the room kept voices: the roster's, the cast's own and the faces' (no story, so
	// nobody claims a result). Finished entries are never touched; no livefund was the player's own money, which is right.
	for (life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State == life::LiveEntry::Registered && E.Voices.empty())
		{
			E.Voices = room::VoicesFor(E);
		}
	}
}
} // namespace live
} // namespace ss
