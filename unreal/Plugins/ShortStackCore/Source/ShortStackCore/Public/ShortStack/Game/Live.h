#pragma once

#include "ShortStack/Game/Life.h"
#include "ShortStack/Tournament.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace live
{
/**
 * The Embercrest Casino's card room: the city's poker room, in the old showroom of a riverboat casino moored for
 * good in 1994. Two or three freezeouts every day for fields of sixty to a hundred and twenty, 6-max and deep
 * (docs/LIVE_TOURNAMENTS.md §4).
 *
 * The schedule is data. Each day's events are occurrences with ids that are the same in every save, so the living
 * world plans who plays them (LiveCalendar reads them from here), and the player's entry, the world's result and
 * the host's night all name the same event. The field is the world's: the people it registered, seated by the
 * engine's draw among the room's anonymous regulars. Nobody is moved anywhere but by the engine's balancing.
 */
constexpr int EmbercrestTableSize = 6;
/** The 14 bus across town, each way. */
constexpr Chips BusFareCents = 290;
constexpr double TravelMinutes = 20.0;
/** The desk takes registrations from three hours before cards. */
constexpr double DeskOpensBefore = 180.0;

struct EventTemplate
{
	const char* Key;   // occurrence ids: "embercrest-<Key>@<day>" ("embercrest@<day>" for the Sunday)
	const char* Name;  // "Embercrest Nightly $120"
	const char* Short; // "Nightly"
	int Days;          // weekdays it runs (bit 0 Monday .. bit 6 Sunday)
	int StartMinute;   // after midnight
	Chips BuyInCents;  // total, fee included
	Chips FeeCents;
	Chips StartingStack;
	double LevelMinutes;
	int LateRegLevels;
	int BreakEvery; // levels between breaks
	double BreakMinutes;
	int MinEntrants; // fewer and it's cancelled, every entry refunded
	int FieldLo;     // the usual field
	int FieldHi;
	double Hours; // cards to the last hand, about
	const char* Speed;
};
SHORTSTACKCORE_API const std::vector<EventTemplate>& Schedule();

/** One day's event. */
struct Occurrence
{
	std::string Id;
	const EventTemplate* T = nullptr;
	int Day = 0;
	double Start = 0.0; // world minutes
	double DeskOpens = 0.0;
	double LateRegEnds = 0.0;
	int Field = 0; // planned entrants
	bool Valid() const { return T != nullptr; }
};
/** The day's events, in start order. */
SHORTSTACKCORE_API std::vector<Occurrence> Occurrences(int Day);
SHORTSTACKCORE_API Occurrence FindOccurrence(const std::string& Id);
SHORTSTACKCORE_API bool IsEmbercrest(const std::string& EventId);
/** The events the player could still make from World (there before late registration closes), in start order. */
SHORTSTACKCORE_API std::vector<Occurrence> Reachable(double World, double HoursAhead);

/** Levels from 100/200, a big-blind ante from level 3. */
SHORTSTACKCORE_API const std::vector<Level>& DeepstackLevels();
SHORTSTACKCORE_API TournamentSpec SpecFor(const Occurrence& O, int Entrants);
/** The field: Known (the world's people, as they play) and anonymous regulars to Entrants, the player among them. */
SHORTSTACKCORE_API std::unique_ptr<Tournament> MakeField(const Occurrence& O, int Entrants, const std::string& HeroName, const std::string& Seed,
	const std::vector<ReservedPlayer>& Known);
/** Fewer entrants than the event runs with: it's cancelled and every entry refunded (no fabricated field). */
inline bool BelowMinimum(const Occurrence& O, int Entrants) { return O.Valid() && Entrants < O.T->MinEntrants; }
/** The draw's seed for an entry: the same field and the same seats however often the night is opened. */
SHORTSTACKCORE_API std::string SeedFor(const life::LiveEntry& E, const std::string& HeroName);
/** The night as the engine drew it for an entry (MakeField from SeedFor), back where its checkpoint left it when
 *  bRestore and it fits (bRestored says whether it did). A checkpoint from before the casino became the Embercrest
 *  was drawn under the occurrence's old id: the night is drawn again under that id for it. */
SHORTSTACKCORE_API std::unique_ptr<Tournament> ResumeField(const Occurrence& O, const life::LiveEntry& E, int Entrants, const std::string& HeroName,
	const std::vector<ReservedPlayer>& Known, bool bRestore, bool& bRestored);

// ------------------------------------------------------------------ the people with faces

/** Someone you can meet at the Embercrest: their name in the field and how they play. */
struct CastMember
{
	const char* Id;   // the tournament player id ("npc:" + Name)
	const char* Name; // as the field knows them
	Archetype Type;
};
SHORTSTACKCORE_API const std::vector<CastMember>& EmbercrestCast();
/**
 * The Embercrest's own regulars: locals who play its card room (the names on the cash list), in the living world like
 * everyone else (world::Origin::Local). Woman: which of the room's bodies they get.
 */
struct RoomLocal
{
	const char* Name;
	bool Woman;
	int Age;
	double Skill;
};
SHORTSTACKCORE_API const std::vector<RoomLocal>& RoomLocals();
SHORTSTACKCORE_API const RoomLocal* FindRoomLocal(const std::string& Name);
/**
 * How much one of the cast likes an occurrence: 0 never, 1 sometimes, 2 it's their game. Mrs. Park plays the noon
 * game, Rick the turbos, Dee's regulars keep her Tuesday, Thursday and Saturday nights.
 */
SHORTSTACKCORE_API int Habit(const std::string& Name, const Occurrence& O);
/** A name's own number (FNV-1a): who is a regular, their nights, the same in every save. */
SHORTSTACKCORE_API uint32_t FaceHash(const std::string& Name);
/**
 * The same for one of the room's regulars (world::World::EmbercrestRegulars): most play the Nightly on two or
 * three nights of their own, some the noon game, some only the weekend's bigger ones; everyone turns up for a
 * Sunday now and then.
 */
SHORTSTACKCORE_API int RegularHabit(const std::string& Name, const Occurrence& O);

// ------------------------------------------------------------------ the player's entries

SHORTSTACKCORE_API life::LiveEntry* EntryFor(life::State& L, const std::string& OccurrenceId);
SHORTSTACKCORE_API const life::LiveEntry* EntryFor(const life::State& L, const std::string& OccurrenceId);
/** The entry the player holds and hasn't finished (nullptr: none). */
SHORTSTACKCORE_API const life::LiveEntry* ActiveEntry(const life::State& L);
/** Why the player can't register for O at World ("" when they can, or when they already have). */
SHORTSTACKCORE_API std::string CanRegister(Chips Bankroll, const life::State& L, const Occurrence& O, double World);
/**
 * Registers the player: the buy-in and the fee as two ledger lines, and the field as it stands. Registering again
 * for the same occurrence changes nothing and charges nothing. Returns why not ("" when registered).
 */
SHORTSTACKCORE_API std::string Register(Chips& Bankroll, life::State& L, const Occurrence& O, double World, int Entrants,
	const std::vector<std::pair<std::string, int>>& Roster);
/** The bus there or home, once each per entry. False when already paid (or no entry). */
SHORTSTACKCORE_API bool PayFare(Chips& Bankroll, life::State& L, const std::string& Id, bool Home, double World);
/** The result, once: the prize to the bankroll and its ledger line. False when already settled (or not registered). */
SHORTSTACKCORE_API bool Settle(Chips& Bankroll, life::State& L, const std::string& Id, int Place, int Field, Chips Prize, double World);
/** A cancelled event: the buy-in and fee back, once. */
SHORTSTACKCORE_API bool Refund(Chips& Bankroll, life::State& L, const std::string& Id, double World, const std::string& Why);

/** "Table 6, seat 3" style seat labels start at 1. */
inline int SeatLabel(int Seat) { return Seat + 1; }
} // namespace live
} // namespace ss
