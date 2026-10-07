#pragma once

#include "ShortStack/Common.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ss
{
class Session;
namespace hero
{
struct Look;
}
namespace wardrobe
{
struct Closet;
}

/**
 * Who is where on the block (the realism update's spec, §6.2): everyone who works or lives on Northside, their
 * schedules by the world clock, the corner's signals and the 14's times, and the player's bonds with them (saved,
 * §7.3). It forward-declares Session and includes neither Life.h nor Session.h (Life.h includes this header for Bond).
 * Owned by people-core; pure core, deterministic from the clock, the career's seed and the living world.
 *
 * **The people.** People() is the block's cast (§6.1: Benny and Ray at the Lucky Penny, Lonnie, Bea, Rosa, Ruby and
 * Marlene, Augie, the cook), Dee, Inez at the Wash & Fold, Marcus (no street presence this update), and the living
 * world's people met on the block (InWorld: Sal, Mei, Big Lou, Twitch, Mrs. Park; their table MetaHumans are Cast).
 * The Embercrest's regulars who wait for the 14 are placed as "npc:<Name>" (the world's name; not in People()).
 *
 * **Placements.** Who and Plan give one Placement per stretch of someone's day: where (Venue, Station) and what
 * (Doing), Since..Until in world minutes. The staff work by their venue's hours (shop::Venues(): Lonnie, Bea, Rosa and
 * Augie whenever theirs is staffed) or their own shifts in the venues that never close (Ray 10:00-22:00 and Benny
 * 22:00-10:00 at the store, Ruby 06:00-18:00 and Marlene 18:00-06:00 at Ruby's, the cook and Inez around the clock).
 * Hand-offs are on the hour. The world's people come from what the world has planned (World::Entered, NextEmbercrest,
 * their plans and moods): Dee's regulars on her nights (Tuesday, Thursday, Saturday) walk up from the Market St stop
 * (Sal about 20:40, then a smoke by the Wash & Fold's door; Mei 20:50; Big Lou 20:55; Twitch about 21:10) and leave
 * one by one between 3:00 and 5:00; Ruby's regulars (two at most at once): Sal at the counter with the paper on weekday
 * mornings, Mrs. Park on a Sunday before the Warm-Up (then the 11:35 bus), Mei in a booth on her online nights, Twitch
 * about 4:00 when the world has him broke, Big Lou on Friday nights; and 30-40 minutes before each of the Embercrest's
 * events, the people the world registered for it wait at the 14's shelter for their bus.
 *
 * Doing: Work at a station; Arrive, walking in to Station (from the 14's stop or the walk limits) until Until; Leave,
 * walking off from Station (vanishing unseen); Sit and Wait at it; Smoke by it; Walk along to it.
 *
 * Stations beyond §2.10's: "washfold-door" (the sidewalk by the Wash & Fold's door), "dee-table" (Dee's back room:
 * off the street, so whoever is there is inside), "diner-door" (Ruby's way out). Venues are shop::Venues()' ids, plus
 * "market-stop" (the 14's shelter on Market St).
 */
namespace town
{
enum class Job : int
{
	Clerk,
	Barber,
	Thrift,
	Trainer,
	Waitress,
	Cook,
	Pawnbroker,
	Host,
	Attendant,
	Regular,
	Local,
	Fan,
	Pedestrian,
};

enum class Doing : int
{
	Work,
	Arrive,
	Leave,
	Sit,
	Wait,
	Smoke,
	Walk,
};

struct Person
{
	std::string Id;
	std::string Name;
	Job Role = Job::Pedestrian;
	std::string Venue;
	std::string Cast;
	bool Woman = false;
	int Age = 40;
	bool InWorld = false;
};

struct Placement
{
	std::string Id;
	std::string Venue;
	std::string Station;
	Doing What = Doing::Work;
	double Since = 0.0;
	double Until = 0.0;
};

struct Bond
{
	int Familiarity = 0; // 0..100: a talk adds 1-2, a visit or a tip 1
	int Visits = 0;      // days the player came by
	int LastDay = -1;
	uint32_t LookSeen = 0; // LookHash of how the player looked the last time this person saw them (0: never)
	int Flags = 0;
};

enum BondFlag : int
{
	Tipper = 1,
	Stiffed = 2,
	Owed = 4,
	Lent = 8,
	Donor = 16,
};

SHORTSTACKCORE_API const std::vector<Person>& People();
SHORTSTACKCORE_API const Person* FindPerson(const std::string& Id);
/** Everyone on the block now: the staff on shift, the world's people, the walkers due to pass (one placement each). */
SHORTSTACKCORE_API std::vector<Placement> Who(double World, const Session& S);
SHORTSTACKCORE_API bool Where(const std::string& Id, double World, const Session& S, Placement& Out);
/**
 * Every placement that overlaps From..To, in time order per person: who's due in the next hour (people-ue builds people
 * under black before they're needed, §6.7), and the schedule town_test checks. The world's people only as far as the
 * world has planned (today and tomorrow).
 */
SHORTSTACKCORE_API std::vector<Placement> Plan(double From, double To, const Session& S);
SHORTSTACKCORE_API std::string OnShift(const std::string& Venue, double World); // "benny", "ray", ...
/** How many pedestrians the hour wants, 0..1: low 02:00-05:00, peaks 08:00-09:00 and 17:00-19:00, Friday and
 *  Saturday nights busy to 02:00, weekends late to rise; rain takes up to 30% off. */
SHORTSTACKCORE_API double CrowdLevel(double World, double Rain);

/**
 * The signals at Fifth & Market (§2.5), a pure function of the clock (a skip holds the World it started at). The
 * 82-second cycle from 5:00 AM: 1 Fifth green 42 s, 2 Fifth amber 4 s, 3 all red 3 s, 4 Market green 26 s, 5 Market
 * amber 4 s, 6 all red 3 s (§2.5's phases sum to 80; the cycle is 82, so each all-red clears for 3 s). The crossing
 * parallel to a green shows WALK for its first 7 s (WalkMarket: across Market's mouth with Fifth's green; WalkFifth:
 * across Fifth with Market's), then the flashing hand counting Countdown down to the amber; otherwise the steady hand.
 * From the end of the cycle running at 1:00 AM to 5:00 AM the corner flashes (Phase 0, Flashing): Fifth amber and Market
 * red together, lit while FlashOn (once a second), the ped heads dark.
 */
struct SignalState
{
	int Phase = 0;
	double Left = 0.0; // seconds left in the phase (in a flash, in this half second)
	bool FifthGo = false, MarketGo = false, WalkFifth = false, WalkMarket = false, Flashing = false;
	int Countdown = 0;    // the ped heads' number while the hand flashes (0: none showing)
	bool FlashOn = false; // overnight: the flashing lamps are lit
};
SHORTSTACKCORE_API SignalState Signal(double World);
/** The next N departures at or after World (world minutes): "market-east" (to the Embercrest: :05 and :35 from 6:05 AM
 *  to 11:35 PM, then 0:35 to 5:35 hourly) and "market-west" (the return: :20 and :50, hourly at :50 overnight). */
SHORTSTACKCORE_API std::vector<double> BusTimes(const std::string& Stop, double World, int N); // "market-east", "market-west"
/** The last eastbound 14 that gets a rider up to the Embercrest for an event at Start (the 20-minute ride and five minutes
 *  to find a seat): its departure in world minutes, or -1 when none does. The shelter's riders take it; talk names it. */
SHORTSTACKCORE_API double BusFor(double Start);
/** The wait at Fifth St. Barbers in minutes, by the quarter hour: 10-25 on Saturday 10:00-14:00, 0-10 otherwise, 0 shut. */
SHORTSTACKCORE_API int BarberQueue(double World);
/** How well known the player is, 0..1: Kast followers and superfans, live cashes, the Embercrest's wall, awards. */
SHORTSTACKCORE_API double Renown(const Session& S); // 0..1

/** A hash of how the player looks (the hair, beard, brows, glasses and hat, what's worn): keepers notice a change. */
SHORTSTACKCORE_API uint32_t LookHash(const hero::Look& L, const wardrobe::Closet& C);
/** A person's name tag's second half: their bond's label ("Knows you", "Holds a grudge"), else their role ("Barber"); ""
 *  for a stranger the player has no reason to know. */
SHORTSTACKCORE_API std::string Tag(const std::string& Id, const Session& S);
/**
 * Whoever picks up the player's check at Ruby's now ("big-lou" on a Friday night when the player has cashed live in the
 * last 7 days), or "". Once a night: the diner that lets him pay records Life.Said[CheckKey(S, World)] with the day, and
 * he's covered his one.
 */
SHORTSTACKCORE_API std::string PaysYourCheck(const Session& S, double World);
/** The night's key for a check Big Lou covered ("check:big-lou:<the day his visit began>"), or "" when he isn't at Ruby's. */
SHORTSTACKCORE_API std::string CheckKey(const Session& S, double World);

/** Said: key -> the day it was told. WriteLines prunes keys older than 90 days and bonds with no familiarity. */
SHORTSTACKCORE_API void WriteLines(const std::map<std::string, Bond>& Town, const std::map<std::string, int>& Said, int Today, std::string& Out);
/** A "said" line from before the day was kept reads as told on the day it's next written (SaidToday until then). */
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, std::map<std::string, Bond>& Town, std::map<std::string, int>& Said);
constexpr int SaidToday = 0x7fffffff;
constexpr int SaidKeepDays = 90;
/** Whether what was told on Day is still remembered on Today (SaidToday always is): the rule WriteLines keeps, and the one
 *  talk and Session::Heard read by, so a long session forgets on the same day a reloaded one does. */
inline bool SaidFresh(int Day, int Today)
{
	return Day >= Today || Today - Day <= SaidKeepDays;
}
} // namespace town
} // namespace ss
