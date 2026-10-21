#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Lobby.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
class World;
}

/**
 * Update 3 (SPEC §2.6 F7, §7.3): FUSE, Ignite's three-player lottery hyper. LIGHT IT, the fuse burns, the reel lands on
 * the prize: x2 to x1,000 the buy-in at $1.10 (x250 at $3.30 and $5.50). The multiplier is drawn from a key on the tier
 * and its ordinal (every FUSE entered at the tier, paid or on a ticket), saved at entry before the reveal; the pool is
 * exactly the multiplier times the buy-in. Session-only: never planned by the world, never in its records or History.
 * Declared by the lead in Phase A; bodies by core-fuse (Fuse.cpp; SessionFuse.cpp's glue after spike S-J).
 */
namespace fuse
{
/** One row of the prize table: the multiplier and its probability at $1.10, and at $3.30 and $5.50. */
struct Row
{
	int M = 0;
	double P[2] = {0, 0};
	bool Split = false; // x25 and up: 80/12/8
};
/** Seven rows; the sum of p x m is 3 x (1 - edge) exactly per tier. */
SHORTSTACKCORE_API const std::array<Row, 7>& Table();
/** The house edge: 0.08 to start, S-J's knob up to 0.12. */
SHORTSTACKCORE_API double Edge();
/** The edge's share of the buy-in: what the rake hooks see (never the tournament spec's FeeCents). */
SHORTSTACKCORE_API Chips FeeOf(int Tier);
SHORTSTACKCORE_API bool IsFuse(const std::string& EventId);
/** The N-th multiplier at a tier (N counts every FUSE entered there, paid or on a ticket). */
SHORTSTACKCORE_API int Draw(uint64_t WorldSeed, int Tier, int N);
/** "ignite.fuse<tier>@<minute>#<n>". */
SHORTSTACKCORE_API std::string IdFor(int Tier, double World, int N);
/** The synthetic event: 3 entrants, Freezeout, "Hyper", TableSize 3, FeeCents = BuyIn, GTD = m x buy-in (the pool exact). */
SHORTSTACKCORE_API ss::LobbyEvent EventFor(int Tier, int M, double World, int N);
/** The places' prizes: winner takes all to x10; 80/12/8 from x25. */
SHORTSTACKCORE_API std::array<Chips, 3> Payout(int Tier, int M);
/** The two Ignite members online that hour who take the other seats (keyed, never NameField's own Rng). */
SHORTSTACKCORE_API std::vector<std::string> SeatsFor(const world::World* W, uint64_t WorldSeed, int Tier, int N, double World);
/** What the reveal shows (Session::FuseReveal). */
struct Reveal
{
	int M = 0;
	double PaidAt = 0;
	int Tier = 0;
	bool Seen = false;
};
} // namespace fuse
} // namespace ss
