// Update 3 (SPEC §3.4, §3.6, §7.2): RunItTwice's League page, League Night, the Callout drawer, the Watchlist, the
// Statement, the Luck Ledger panel and credentials, ui-rit's file. This file has no RiverLine member function of its own:
// it reaches the client's private state and its gatekeepers through rlrit_detail::Access (RiverLine.h's friend),
// forwarders defined here and declared in RitArt.h, where RiverLineRit.cpp's pages reach them too. G1 (§3.9): the
// forwarders only; the League and its loops are G2 (the rail hides LEAGUE until then).
#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"
#include "RitArt.h"

namespace ss
{
namespace ui
{
namespace rlrit_detail
{
Session& Access::S(RiverLine& RL)
{
	return RL.S;
}

Canvas& Access::C(RiverLine& RL)
{
	return *RL.C;
}

Ui& Access::UI(RiverLine& RL)
{
	return RL.UI;
}

double Access::World(const RiverLine& RL)
{
	return RL.World;
}

double Access::Dt(const RiverLine& RL)
{
	return RL.Dt;
}

bool Access::Previewing(const RiverLine& RL)
{
	return RL.Previewing;
}

double Access::PageAt(const RiverLine& RL)
{
	return RL.PageAt;
}

void Access::SetPage(RiverLine& RL, RiverLine::Page P)
{
	// The page a frame settles on (a page RunItTwice hasn't got at G1 goes to its Board): no OpenPage side effects.
	RL.PageShown = P;
}

double Access::EventAt(const RiverLine& RL)
{
	return RL.EventAt;
}

std::vector<std::string>& Access::Listed(RiverLine& RL)
{
	return RL.Listed;
}

RiverLine::Filter Access::FilterShown(const RiverLine& RL)
{
	return RL.FilterShown;
}

double Access::FilterAt(const RiverLine& RL)
{
	return RL.FilterAt;
}

bool Access::TakeScrollToSelected(RiverLine& RL)
{
	const bool Was = RL.ScrollToSelected;
	RL.ScrollToSelected = false;
	return Was;
}

float& Access::ListScroll(RiverLine& RL)
{
	return RL.ListScroll;
}

float& Access::ListScrollGoal(RiverLine& RL)
{
	return RL.ListScrollGoal;
}

net::Board Access::BoardShown(const RiverLine& RL)
{
	return RL.BoardShown;
}

double Access::BoardAt(const RiverLine& RL)
{
	return RL.BoardAt;
}

double Access::NewsSeenAt(const RiverLine& RL)
{
	return RL.NewsSeenAt;
}

std::vector<RitRow> Access::ScheduleRows(const RiverLine& RL)
{
	// The shared schedule (the gatekeeper: the filter, Joinable and its lock) as RiverLine's lobby builds it.
	std::vector<RitRow> Out;
	for (const RiverLine::ListRow& Row : RL.ScheduleRows())
	{
		RitRow R;
		R.E = Row.E;
		R.L = Row.L;
		R.Joinable = Row.Joinable;
		R.Lock = Row.Lock;
		Out.push_back(std::move(R));
	}
	return Out;
}

void Access::RegisterBlock(RiverLine& RL, const net::EventInstance& E, const net::LiveState& L, const Rect& R, double Now)
{
	RL.RegisterBlock(E, L, R, Now);
}

const std::vector<net::NewsItem>& Access::NewsFeed(RiverLine& RL)
{
	return RL.NewsFeed();
}

void Access::PlayerCard(RiverLine& RL, double Now)
{
	RL.PlayerCard(Now);
}

void Access::FinishedToasts(RiverLine& RL, double Now)
{
	RL.FinishedToasts(Now);
}
} // namespace rlrit_detail
} // namespace ui
} // namespace ss
