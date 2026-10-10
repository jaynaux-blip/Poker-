#include "ShortStack/Game/Badges.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Session.h"

// Pro badges (next_segment_spec §5). core-badges.
//
// Phase A, day 1: stubs that compile and change nothing: nobody holds a badge, nothing is packed, and the save hooks
// read and write nothing.

namespace ss
{
namespace badge
{
const Badge* Set::Find(Kind K) const
{
	for (const Badge& B : Held)
	{
		if (B.K == K)
		{
			return &B;
		}
	}
	return nullptr;
}

uint32_t Set::Mask() const
{
	uint32_t Bits = 0;
	for (const Badge& B : Held)
	{
		Bits |= 1u << static_cast<uint32_t>(B.K);
	}
	return Bits;
}

Holder Of(const world::World&, int, int)
{
	return Holder();
}

Holder OfHero(const Session&)
{
	return Holder();
}

Set Earned(const Holder&, int)
{
	return Set();
}

const Badge* Top(const Set&, uint8_t, uint8_t)
{
	return nullptr;
}

int ClubTier(int, int)
{
	return 0;
}

int VisitsIn(const std::vector<int>&, int, int)
{
	return 0;
}

bool VerifiedNow(const Holder&)
{
	return false;
}

double HallScore(const world::Npc&, const world::World&, int)
{
	return 0.0;
}

std::string Label(const Badge&)
{
	return std::string();
}

std::string Title(const Badge&)
{
	return std::string();
}

std::string HowEarned(const Badge&, const Holder&)
{
	return std::string();
}

std::string Honorific(const Set&, uint8_t)
{
	return std::string();
}

std::string DeskLine(int, bool, const std::string&)
{
	return std::string();
}

std::vector<Progress> Next(const Session&, int)
{
	return {};
}

std::vector<std::pair<std::string, std::string>> FieldBadges(const world::World&, const std::vector<int>&, int)
{
	return {};
}

std::vector<life::RoomLeader> FieldLeaders(const world::World&, int)
{
	return {};
}

const Set& Index::Of(const world::World&, int, int)
{
	static const Set Nothing{};
	return Nothing;
}

Index& Shared()
{
	static Index Built;
	return Built;
}

std::string Pack(const Set&)
{
	return std::string();
}

Set Unpack(const std::string&)
{
	return Set();
}

void WriteLines(const life::State&, std::string&)
{
}

bool ReadLine(const std::vector<std::string>&, life::State&)
{
	return false;
}
} // namespace badge
} // namespace ss
