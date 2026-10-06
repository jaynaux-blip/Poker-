#include "ShortStack/Game/Body.h"
#include "../StrictFloat.h"

// Phase A stubs (the realism update's spec, §7.1): every declaration links and returns its default. Effects{} and
// TableEffects{} change nothing (the Session's hooks fold them in already, §7.2), the barber may cut anything, and
// nothing is gained or lost. core-shops' Phase B replaces these bodies with §5.3's fitness and §5.1's grooming.

namespace ss
{
namespace body
{
Effects EffectsOf(const Fitness& /*F*/, bool /*Member*/)
{
	return Effects();
}

TableEffects TableOf(const Fitness& /*F*/, double /*World*/)
{
	return TableEffects();
}

double Gain(const Fitness& /*F*/, double /*Base*/, double /*Energy*/, double /*World*/)
{
	return 0.0;
}

void Decay(Fitness& /*F*/, double /*World*/)
{
}

int HairRank(int /*HairStyle*/)
{
	return 0;
}

int BeardRank(int /*FacialHair*/)
{
	return 0;
}

int HairAllowed(const Grooming& /*G*/, const hero::Look& /*L*/, double /*World*/)
{
	return 6; // §5.1's cap: anything goes until the length rule lands
}

int BeardAllowed(const Grooming& /*G*/, const hero::Look& /*L*/, double /*World*/)
{
	return 4; // a full beard: anything goes until the length rule lands
}

double Sharp(const Grooming& /*G*/, double /*World*/)
{
	return 0.0;
}
} // namespace body
} // namespace ss
