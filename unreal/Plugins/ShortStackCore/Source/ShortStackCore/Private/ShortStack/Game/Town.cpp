#include "ShortStack/Game/Town.h"
#include "../StrictFloat.h"

// Phase A stubs (the realism update's spec, §7.1), written by core-shops and handed to people-core: every declaration
// links and returns its default (nobody on the block, no schedule, the signals at rest, no buses), so people-ue and
// ue-world build against Town.h from day 1. people-core replaces these bodies with §6.2's town.

namespace ss
{
namespace town
{
const std::vector<Person>& People()
{
	static const std::vector<Person> None;
	return None;
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

std::vector<Placement> Who(double /*World*/, const Session& /*S*/)
{
	return {};
}

bool Where(const std::string& /*Id*/, double /*World*/, const Session& /*S*/, Placement& /*Out*/)
{
	return false;
}

std::string OnShift(const std::string& /*Venue*/, double /*World*/)
{
	return std::string();
}

double CrowdLevel(double /*World*/, double /*Rain*/)
{
	return 1.0; // today's crowd, until the town says otherwise
}

SignalState Signal(double /*World*/)
{
	return SignalState();
}

std::vector<double> BusTimes(const std::string& /*Stop*/, double /*World*/, int /*N*/)
{
	return {};
}

int BarberQueue(double /*World*/)
{
	return 0;
}

double Renown(const Session& /*S*/)
{
	return 0.0;
}

void WriteLines(const std::map<std::string, Bond>& /*Town*/, const std::map<std::string, int>& /*Said*/, int /*Today*/, std::string& /*Out*/)
{
}

bool ReadLine(const std::vector<std::string>& /*Fields*/, std::map<std::string, Bond>& /*Town*/, std::map<std::string, int>& /*Said*/)
{
	return false;
}
} // namespace town
} // namespace ss
