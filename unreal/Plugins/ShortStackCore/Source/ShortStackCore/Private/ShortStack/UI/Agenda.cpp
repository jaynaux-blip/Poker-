#include "ShortStack/UI/SecondScreen.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"

// The screens' agenda (next_segment_spec §1.3): the session's merged marks. Owned by the lead (SecondScreen.cpp, which
// draws it, is ui-laptop's).

namespace ss
{
namespace ui
{
Agenda AgendaOf(const Session& S, double World, double Days)
{
	Agenda A;
	A.Marks = S.Marks(World, World + Days * net::MinutesPerDay);
	return A;
}
} // namespace ui
} // namespace ss
