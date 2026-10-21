// Update 3 (SPEC §1.3, §1.4, §7.3): the rival sites' play book, core-siteplay's file. The Value Governor and the Guest
// List, the hooks the Session calls (a paid entry, a result, a hand), the calendar's step, the save lines and the pages'
// view. Phase A, day 1: stubs that keep today's behaviour (nothing granted, held, written or read).
#include "ShortStack/Game/SitePlay.h"

#include "ShortStack/Game/Life.h"

#include <string>
#include <vector>

namespace ss
{
namespace site
{
namespace play
{
bool Grant(Book& /*B*/, life::State& /*L*/, net::SiteId /*S*/, Kind /*K*/, const std::string& /*What*/, Chips /*ValueCents*/, const std::string& /*Key*/, int /*Day*/, double /*World*/)
{
	return false;
}

double GivenBack30(const Book& /*B*/, net::SiteId /*S*/)
{
	return 0.0;
}

void OnPaidEntry(Book& /*B*/, life::State& /*L*/, net::SiteId /*S*/, Chips /*Fee*/, Chips /*BuyIn*/, int /*Format*/, const std::string& /*EventId*/, double /*World*/)
{
}

void OnResult(Book& /*B*/, life::State& /*L*/, const SettleFacts& /*F*/, double /*World*/)
{
}

void OnHand(Book& /*B*/, net::SiteId /*S*/, const HandNote& /*N*/, double /*World*/)
{
}

StepOut Step(Book& /*B*/, life::State& /*L*/, const site::Book& /*Sites*/, const world::World* /*W*/, const sponsor::Book& /*Deals*/, uint64_t /*WorldSeed*/, double /*From*/, double /*To*/)
{
	return StepOut();
}

View ViewOf(const Book& /*B*/, const life::State& /*L*/, const site::Book& /*Sites*/, const world::World* /*W*/, net::SiteId /*S*/, double /*World*/)
{
	return View();
}

std::vector<std::string> Inventory()
{
	return {};
}

void WriteLines(const life::State& /*L*/, std::string& /*Out*/)
{
}

bool ReadLine(const std::vector<std::string>& /*P*/, life::State& /*L*/)
{
	return false;
}
} // namespace play
} // namespace site
} // namespace ss
