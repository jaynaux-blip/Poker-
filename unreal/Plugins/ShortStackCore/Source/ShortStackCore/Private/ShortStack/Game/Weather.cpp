#include "ShortStack/Game/Weather.h"
#include "../StrictFloat.h"

// Phase A stubs (the realism update's spec, §7.1), written by core-shops and handed to people-core: At returns a dry,
// clear default, so ue-world and people-ue build against Weather.h from day 1. people-core replaces it with §3.3's
// model.

namespace ss
{
namespace weather
{
Now At(double /*World*/, uint64_t /*Seed*/, double /*RealSeconds*/)
{
	return Now();
}

const char* Word(Sky S)
{
	switch (S)
	{
	case Sky::Clear: return "Clear";
	case Sky::Overcast: return "Overcast";
	case Sky::Drizzle: return "Drizzle";
	case Sky::Rain: return "Rain";
	case Sky::Storm: return "Storm";
	}
	return "";
}
} // namespace weather
} // namespace ss
