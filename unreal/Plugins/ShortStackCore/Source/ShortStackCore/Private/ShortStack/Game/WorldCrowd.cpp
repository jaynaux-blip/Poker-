// Update 4, the crowd (SPEC v1.7 §5.2-§5.6, §5.9): the record, arrivals into the crowd, the day weights, final-table seats and cash
// places in Resolve, the weekly pass, forgetting and compaction, the name index and the light card (H/WorldCrowd.h). S-crowd's file
// from G4.
//
// Phase A (§6 PA-19, PA-23; the lead): the crowd is empty (Count 0), so every hook is a no-op or answers today's way (FtSeat false:
// Anonymous names the seat), the light card is empty and FindCrowd null. CrowdBook's Get and Edit are the pages' plain access (Edit
// clones a shared page first); nothing calls them before G4. crowd::Access is defined here (§6.2).
#include "ShortStack/Game/WorldCrowd.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

#include <atomic>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace world
{
const CrowdRec& CrowdBook::Get(int I) const
{
	return (*Pages[static_cast<size_t>(I / PageSize)])[static_cast<size_t>(I % PageSize)];
}

CrowdRec& CrowdBook::Edit(int I)
{
	std::shared_ptr<std::array<CrowdRec, PageSize>>& Page = Pages[static_cast<size_t>(I / PageSize)];
	if (Page.use_count() > 1)
	{
		Page = std::make_shared<std::array<CrowdRec, PageSize>>(*Page); // copy-on-write: a snapshot still holds the old page
	}
	std::atomic_thread_fence(std::memory_order_acquire); // use_count() reads relaxed: the saver's release happens before our writes (Q-14)
	return (*Page)[static_cast<size_t>(I % PageSize)];
}

namespace crowd
{
/** World's friend for the crowd modules (§6.2): the book World keeps for them (S-crowd's reads and writes from G4). */
struct Access
{
	static CrowdBook& Book(World& W) { return W.Crowd; }
	static const CrowdBook& Book(const World& W) { return W.Crowd; }
};

void Week(World& /*W*/, int /*Day*/)
{
	// Phase A: no crowd to walk.
}

void Review(World& /*W*/, int /*Day*/)
{
	// Phase A: no crowd member to review.
}

void OnPlaced(World& /*W*/, const Pending& /*P*/, int /*Field*/, const std::vector<Chips>& /*Pay*/, std::set<int>& /*Taken*/)
{
	// Phase A: the crowd takes no cash place (G4: WCash's draws by place, §5.4 R2).
}

bool FtSeat(World& /*W*/, const Pending& /*P*/, int /*Place*/, net::Placing& /*Pc*/)
{
	// Phase A: no crowd holder, so the seat stays the anonymous one today's code names.
	return false;
}

CrowdRec FromNpc(const Npc& /*N*/)
{
	// Phase A: unused (G4: Overall the skills' mean, traits quantized, the Growth record compacted, §5.2 R1).
	return CrowdRec();
}
} // namespace crowd

// ---- the session's calls (§5.4 R4, §5.5, §5.9)

void World::CrowdJoin(const std::string& /*EventId*/, uint32_t /*Serial*/)
{
}

void World::CrowdFinished(const std::string& /*EventId*/, const std::vector<std::pair<uint32_t, int>>& /*Places*/, const std::vector<uint32_t>& /*StillIn*/)
{
}

void World::CrowdMet(uint32_t /*Serial*/, int /*Day*/)
{
}

CrowdProfile World::CrowdProfileOf(const std::string& /*Name*/) const
{
	return CrowdProfile();
}

const CrowdRec* World::FindCrowd(const std::string& /*Name*/) const
{
	return nullptr;
}
} // namespace world
} // namespace ss
