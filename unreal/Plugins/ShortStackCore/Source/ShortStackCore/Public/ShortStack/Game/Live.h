#pragma once

#include "ShortStack/Game/Life.h"
#include "ShortStack/Tournament.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct SaveData;
namespace world
{
class World;
struct Npc;
}

namespace live
{
/**
 * The Embercrest Casino's card room: the poker room of the mountain resort casino on the ridge above the city, home
 * of the Embercrest Poker Series. Two or three freezeouts every day for fields of sixty to a hundred and twenty, 6-max
 * and deep (docs/LIVE_TOURNAMENTS.md §4).
 *
 * The schedule is data. Each day's events are occurrences with ids that are the same in every save, so the living
 * world plans who plays them (LiveCalendar reads them from here), and the player's entry, the world's result and
 * the host's night all name the same event. The field is the world's: the people it registered, seated by the
 * engine's draw among the room's anonymous regulars. Nobody is moved anywhere but by the engine's balancing.
 */
constexpr int EmbercrestTableSize = 6;
/** The 14 bus, each way: it climbs from the city up the ridge to the resort in twenty minutes. */
constexpr Chips BusFareCents = 290;
constexpr double TravelMinutes = 20.0;
/** The desk takes registrations from three hours before cards. */
constexpr double DeskOpensBefore = 180.0;
/** The floor's last call for registration: this long before late registration closes. */
constexpr double LastCallMinutes = 15.0;

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
/**
 * The next event still open to someone Arrive minutes away (late registration not closed when they get there; 0 for
 * someone already in the room), starting within a day of World, Except skipped (the one the player is in). Invalid
 * when there's none.
 */
SHORTSTACKCORE_API Occurrence NextOpen(double World, double Arrive = TravelMinutes, const std::string& Except = std::string());
/** Minutes until late registration closes, from World (0 or less: closed). */
inline double LateRegLeft(const Occurrence& O, double World)
{
	return O.Valid() ? O.LateRegEnds - World : 0.0;
}
/** Late registration is still open but closes within the floor's last call. */
inline bool LastCall(const Occurrence& O, double World)
{
	const double Left = LateRegLeft(O, World);
	return Left > 0.0 && Left <= LastCallMinutes;
}
/** When the floor makes its last call for registration (world minutes): BackRoomLive.cpp:146-148's rule. */
inline double LastCallAt(double LateRegEnds)
{
	return LateRegEnds - LastCallMinutes;
}
/** About how long a deep run takes with this field: cards to the last hand, breaks included (minutes). */
SHORTSTACKCORE_API double DeepRunMinutes(const Occurrence& O, int Entrants);
/** When it is, said from World: "tonight", "today", "tomorrow night", "tomorrow", "last night", "yesterday", "on Saturday". */
SHORTSTACKCORE_API std::string DayWord(const Occurrence& O, double World);

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
/**
 * Why the player can't register for O at World ("" when they can, or when they already have). F: who pays (§1.2). The
 * player's own money needs the buy-in and the bus there and back (less what pieces sold brought in, F.Cents); a backer,
 * a package or a sponsor pays the buy-in, so only the bus is needed, and nothing when the sponsor covers the fares too.
 */
SHORTSTACKCORE_API std::string CanRegister(Chips Bankroll, const life::State& L, const Occurrence& O, double World, const life::Funding& F = {});
/**
 * Registers the player: the funding's line first when someone else pays (sponsor::OnRegister, then stake::OnRegister:
 * a backer's stake, a sponsor's cover, the pieces' money), then the buy-in and the fee as two ledger lines (none in the
 * player's ledger when a package's roll pays: its own ledger takes it), and the field as it stands. Registering again
 * for the same occurrence changes nothing and charges nothing. Returns why not ("" when registered).
 */
SHORTSTACKCORE_API std::string Register(Chips& Bankroll, life::State& L, const Occurrence& O, double World, int Entrants,
	const std::vector<std::pair<std::string, int>>& Roster, const life::Funding& F = {});
/**
 * The bus there or home, once each per entry. False when already paid (or no entry). A sponsor covering the fares
 * (Fund.Fares) writes the fare and its cover as two lines that net to nothing.
 */
SHORTSTACKCORE_API bool PayFare(Chips& Bankroll, life::State& L, const std::string& Id, bool Home, double World);
/**
 * The result, once, in this order (§1.2): the prize to the bankroll and its ledger line; sponsor::Settled (a cut, a
 * bonus); stake::OnSettle (the backer's share, what pieces and swaps owe); then the live record (LiveWins, LiveSpentCents,
 * the day in LiveDays). The night's snapshot (voices, room, badges, leaders, the world's patches) is cleared with the
 * checkpoint. False when already settled (or not registered).
 */
SHORTSTACKCORE_API bool Settle(Chips& Bankroll, life::State& L, const std::string& Id, int Place, int Field, Chips Prize, double World);
/**
 * A cancelled event: the buy-in and the fee back in full, once, as two lines that reverse the registration's (none when
 * a package paid), then stake::OnRefund and sponsor::OnRefund reverse the funding's. The snapshot is cleared.
 */
SHORTSTACKCORE_API bool Refund(Chips& Bankroll, life::State& L, const std::string& Id, double World, const std::string& Why);

// ------------------------------------------------------------------ the field (§1.8)

/** How one of the world's people plays at the table: the engine's bot closest to them (the session's and the desk's). */
SHORTSTACKCORE_API Archetype PlayStyle(const world::Npc& N);
/** What registration writes on an entry about its field, as the world has it at World. */
struct FieldNotes
{
	int Planned = 0; // the world's planned field without the player (the occurrence's own crowd without a world)
	std::vector<std::pair<std::string, int>> Roster; // the world's people in it: name, how they play (an ss::Archetype)
	std::vector<life::LiveFace> Faces;               // the room's regulars and whoever remembers the player
	std::vector<std::string> Board;                  // the champions' board by the desk, newest first
	std::vector<life::LiveVoice> Voices;             // room::VoiceOf for each of the world's people (§4)
	std::vector<std::pair<std::string, std::string>> Badges; // name -> badge::Pack (badge::FieldBadges, §5)
	std::vector<std::pair<std::string, std::string>> Patches; // the world's sponsored people: name -> brand id (§3)
	std::vector<life::RoomLeader> Leaders;           // the month's room leaders (badge::FieldLeaders, §5)
};
/**
 * The one place the field is described (Session::GoToLive and the desk's worker, which builds a world from the save's
 * text for every signup): it replaces Session.cpp's and BackRoomLive.cpp's copies, which tests/room_field.cpp proves
 * gave the same notes. Before the player is registered (the world's planning is lazy, hence a World it may plan in).
 * The caller registers with Entrants = max(Planned, Roster.size()) + 1, tells the world (HeroEntered), then Apply.
 */
SHORTSTACKCORE_API FieldNotes DescribeField(world::World& W, const Occurrence& O, const std::string& HeroName, const life::State& Hero, double World);
/** Writes the notes on the entry (all but the roster, which Register took): faces, board, voices, badges, patches, leaders. */
SHORTSTACKCORE_API void Apply(const FieldNotes& N, life::LiveEntry& E);
/** The funding in a line for the desk, the event card and the clock card: "Big Lou is putting you in the Nightly." ("" for
 *  the player's own money). */
SHORTSTACKCORE_API std::string FundingLine(const life::Funding& F, const std::string& EventShort);

// ------------------------------------------------------------------ the save (§7.4, §7.5)

/** An entry's segment lines, after its livecheck: liveroom, livefund, livevoice, livebadge, liveleader, livepatch (each only when it has something). */
SHORTSTACKCORE_API void WriteEntryLines(const life::LiveEntry& E, std::string& Out);
/** Reads one of WriteEntryLines' lines (split on tabs) onto its entry in L; false when it isn't one. */
SHORTSTACKCORE_API bool ReadEntryLine(const std::vector<std::string>& Fields, life::State& L);
/**
 * After a save is read (SaveData::Parse's AfterParse chain, first): what an older save lacked, by SegmentLinesRead's
 * bits: LiveWins and LiveSpentCents, the LiveDays rebuilt from the kept entries, LastShiftAt from the ledger (worked out
 * again at every load, and written once the segment keeps them: a registration, a settle, a shift), and voices
 * (room::VoicesFor) for an entry still registered without them.
 */
SHORTSTACKCORE_API void AfterParse(SaveData& D);

// ------------------------------------------------------------------ the clock (LiveClock.cpp: core-tableui)

/**
 * One set of clock facts (next_segment_spec §6.2 C) for the clock card, the room screens, the pause menu, the dealer's
 * answers and the PA, so they never disagree. Seconds are the host's level and break timers.
 */
struct ClockFacts
{
	int Level = 1; // 1-based
	ss::Level Now;
	ss::Level Next;
	double LevelSecondsLeft = 0.0;
	bool OnBreak = false;
	double BreakSecondsLeft = 0.0;
	int BreakBeforeLevel = 0; // the next break comes before this level
	double SecondsToBreak = 0.0;
	int Entrants = 0;
	int Remaining = 0;
	int Paid = 0;
	int ToTheMoney = 0; // players to bust before the money; 0 in it
	Chips PoolCents = 0;
	Chips MinCashCents = 0;
	Chips Average = 0;
	double AverageBb = 0.0;
	bool HandForHand = false;
	bool InTheMoney = false;
	bool Projected = true; // the pool and the payouts until late registration closes
	int NextJumpAt = 0;    // players left at the next pay jump
	Chips NextJumpCents = 0;
	std::string LateReg; // "LATE REG TO 8:00 PM" / "LAST CALL · REG TO 8:00 PM"; empty once closed
	std::string ColorUp;
	std::string Funding; // "BACKED · BIG LOU 100%"; empty for the player's own money
	int TableId = 0;
	int Seat = 0;
};
/** The facts for T at World (E: the player's entry, or null on the rail and the room screens). */
SHORTSTACKCORE_API ClockFacts Clock(const Tournament& T, const Occurrence& O, const life::LiveEntry* E, double World,
	double LevelSecondsLeft, bool OnBreak, double BreakSecondsLeft);

/** A row of the payout ladder: places From..To pay Cents each; Here is the player's place or the next jump. */
struct PayRow
{
	int From = 0;
	int To = 0;
	Chips Cents = 0;
	bool Here = false;
};
SHORTSTACKCORE_API std::vector<PayRow> PayLadder(const Tournament& T);

/** "Table 6, seat 3" style seat labels start at 1. */
inline int SeatLabel(int Seat) { return Seat + 1; }
} // namespace live
} // namespace ss
