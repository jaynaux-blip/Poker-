// Update 3 (SPEC §6.1, §7.2): a face for a name, ui-thumbs' file: hero::CharacterFor hashes a Look from a name, so the
// world's creators and the player's opponents look the same on every thumbnail. Phase A, day 1: a stub (the default
// person for every name).
#include "ShortStack/Game/Hero.h"

#include <string>

namespace ss
{
namespace hero
{
Character CharacterFor(const std::string& /*Name*/)
{
	return Character();
}
} // namespace hero
} // namespace ss
