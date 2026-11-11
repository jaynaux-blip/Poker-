// Update 4, the Big Field's core (SPEC v1.7 §4.2-§4.15): the strip's estimator, the lock line, the keyed mystery deck, the copy and
// YOUR RUN's line. B-session's file from G3.
//
// Phase A (§6 PA-42; the lead): stubs that keep today's behaviour. Nothing calls them before G3; ReadLine claims a run\t line and
// keeps nothing, exactly as the session dropped any line it didn't know.
#include "ShortStack/Game/BigField.h"
#include "ShortStack/Game/Life.h"

namespace ss
{
namespace bigfield
{
Pace Estimate(int /*Entries*/, double /*LevelMinutes*/)
{
	return Pace();
}

std::string LockLine()
{
	return std::string();
}

std::vector<Chips> MysteryDeck(uint64_t /*WorldSeed*/, const std::string& /*EventId*/, int /*Entries*/, Chips /*Bounty*/, int /*Paid*/, double /*Share*/)
{
	return std::vector<Chips>();
}

std::vector<std::string> Inventory()
{
	return std::vector<std::string>();
}

bool ReadLine(const std::vector<std::string>& Fields, life::State& /*L*/)
{
	// The run\t tag is reserved in the session's dispatch (§6 PA-43); G3 keeps the record.
	return !Fields.empty() && Fields[0] == "run";
}

void WriteLines(const life::State& /*L*/, std::string& /*Out*/)
{
}
} // namespace bigfield
} // namespace ss
