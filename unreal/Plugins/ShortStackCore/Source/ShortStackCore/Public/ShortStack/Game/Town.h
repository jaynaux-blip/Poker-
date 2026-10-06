#pragma once

#include "ShortStack/Common.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ss
{
class Session;

/**
 * Who is where on the block (the realism update's spec, §6.2): everyone who works or lives on Northside, their
 * schedules by the world clock, the corner's signals and the 14's times, and the player's bonds with them (saved,
 * §7.3). It forward-declares Session and includes neither Life.h nor Session.h (Life.h includes this header for Bond).
 *
 * Owned by people-core. Phase A (core-shops): the contract only; the bodies return defaults (nobody, no schedule)
 * until people-core fills them.
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
	int Familiarity = 0;
	int Visits = 0;
	int LastDay = -1;
	uint32_t LookSeen = 0;
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
SHORTSTACKCORE_API std::vector<Placement> Who(double World, const Session& S);
SHORTSTACKCORE_API bool Where(const std::string& Id, double World, const Session& S, Placement& Out);
SHORTSTACKCORE_API std::string OnShift(const std::string& Venue, double World); // "benny", "ray", ...
SHORTSTACKCORE_API double CrowdLevel(double World, double Rain);

struct SignalState
{
	int Phase = 0;
	double Left = 0.0;
	bool FifthGo = false, MarketGo = false, WalkFifth = false, WalkMarket = false, Flashing = false;
};
SHORTSTACKCORE_API SignalState Signal(double World);
SHORTSTACKCORE_API std::vector<double> BusTimes(const std::string& Stop, double World, int N); // "market-east", "market-west"
SHORTSTACKCORE_API int BarberQueue(double World);
SHORTSTACKCORE_API double Renown(const Session& S); // 0..1

/** Said: key -> the day it was told. WriteLines prunes keys older than 90 days and bonds with no familiarity. */
SHORTSTACKCORE_API void WriteLines(const std::map<std::string, Bond>& Town, const std::map<std::string, int>& Said, int Today, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& Fields, std::map<std::string, Bond>& Town, std::map<std::string, int>& Said);
} // namespace town
} // namespace ss
