// Update 3 (SPEC §6.1, §7.2): a face for a name, ui-thumbs' file: hero::CharacterFor hashes a Look from a name, so the
// world's creators and the player's opponents look the same on every thumbnail. The draw is the character creator's own
// RANDOMIZE (hero::Random) on a generator seeded by the name alone: a plausible person of an age and a look the creator
// could make, the same every time and on every machine (Rng is the shared sfc32). The name on the Character is the handle
// itself (FirstName; LastName empty), so a thumbnail's initials read the handle.
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Rng.h"

#include <string>

namespace ss
{
namespace hero
{
Character CharacterFor(const std::string& Name)
{
	Rng R("rerun-face:" + Name);
	Character C = Random(R);
	C.FirstName = Name;
	C.LastName.clear();
	return C;
}
} // namespace hero
} // namespace ss
