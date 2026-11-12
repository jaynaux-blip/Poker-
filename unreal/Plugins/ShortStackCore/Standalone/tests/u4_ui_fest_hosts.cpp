// u4_ui: the festival half's host pages (update 4 SPEC v1.7 §2.15.2; F-hubs' stream B, LEAD_CALLS R-1): the hub's routing on the
// three hosts, the takeover's words on Ignite and RunItTwice, the register sheets' YOUR strip, slide 0, the tickers, the card's pill
// and the trophy case (UI/RiverLineNet.cpp, UI/RiverLineIgnite.cpp, UI/RiverLineRit.cpp, UI/RiverLineCard.cpp, UI/SecondScreen.cpp).
// Called from u4_ui_main.cpp (the kit), after SegFest.
// G1 (the lead's pre-wave commit H0, PLAN_G1 §4.2): registered, empty.
// G1 FH-1c: FestHostsRouting, the hub's routing (§2.8.1: rlnet_detail::NetHubPick, NetHubEdition and NetHubRect in
// UI/RiverLineNet.cpp). F-net's calendar (fest::All()) is empty before FN-1, so the choice is checked on constructed editions and the
// real path on every series the three hosts list; FestHubBudgets (stream A, FH-7) later draws every state on every host.
#include "UiSeg.h"

#include "ShortStack/Game/Festival.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlnet_detail
{
// UI/RiverLineNet.cpp (FH-1c): the festival hub's routing.
Rect NetHubRect(net::SiteId Site);
const fest::Festival* NetHubPick(net::SiteId Site, bool HasPick, const fest::Festival* Picked, const std::vector<std::pair<const fest::Festival*, fest::Phase>>& Near);
const fest::Festival* NetHubEdition(net::SiteId Site, const std::string& SeriesId, double World);
} // namespace rlnet_detail
} // namespace ui
} // namespace ss

namespace
{
using HostsNear = std::vector<std::pair<const ss::fest::Festival*, ss::fest::Phase>>;

ss::fest::Festival HostsEdition(ss::net::SiteId Site, ss::fest::Brand Which, const char* SeriesId, int FirstDay, int LastDay)
{
	ss::fest::Festival F;
	F.Site = Site;
	F.Which = Which;
	F.SeriesId = SeriesId;
	F.FirstDay = FirstDay;
	F.LastDay = LastDay;
	F.MainDay = LastDay;
	return F;
}

bool HostsSameRect(const ss::ui::Rect& A, const ss::ui::Rect& B)
{
	return A.X == B.X && A.Y == B.Y && A.W == B.W && A.H == B.H;
}

/** FestHostsRouting (FH-1c): a host's SERIES page is a festival edition's hub, in the host's rect, exactly when §2.8.1 says. Plants:
 *  NetHubPick's site test dropped (Ignite's FEIGN opens on RiverLine's page); a picked plain series falling through to the window (a
 *  live festival takes the page from the series the player picked). */
void FestHostsRouting()
{
	using ss::fest::Brand;
	using ss::fest::Phase;
	using ss::net::SiteId;
	namespace rn = ss::ui::rlnet_detail;
	const ss::fest::Festival Rcop = HostsEdition(SiteId::RiverLine, Brand::Rcop, "rcop", 13, 34);
	const ss::fest::Festival Tco = HostsEdition(SiteId::RiverLine, Brand::Tco, "tco27", 250, 263);
	const ss::fest::Festival Feign = HostsEdition(SiteId::Ignite, Brand::Feign, "ignite.feign26", 56, 65);
	const ss::fest::Festival Grit = HostsEdition(SiteId::RunItTwice, Brand::Grit, "runittwice.grit27", 222, 233);

	// A picked series of the host's decides alone.
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, true, &Rcop, HostsNear{}) == &Rcop, "FestHostsRouting: a picked festival series opens its hub");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, true, &Rcop, HostsNear{{&Rcop, Phase::Off}}) == &Rcop, "FestHostsRouting: a picked festival series opens its hub in any phase");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, true, nullptr, HostsNear{{&Rcop, Phase::Live}}) == nullptr,
		"FestHostsRouting: a picked plain series keeps today's page while the host's festival is live");
	ui_test::Expect(rn::NetHubPick(SiteId::Ignite, true, &Rcop, HostsNear{}) == nullptr, "FestHostsRouting: another site's edition never opens on this host");

	// None picked: the host's current or next edition inside its Announced-to-Afterglow window, else today's page.
	for (const Phase P : {Phase::Announced, Phase::Buildup, Phase::Live, Phase::MainDay, Phase::Closing, Phase::Afterglow})
	{
		ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, P}}) == &Rcop,
			("FestHostsRouting: every phase from Announced to Afterglow opens the host's hub (phase " + std::to_string(static_cast<int>(P)) + ")").c_str());
	}
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, Phase::Off}}) == nullptr, "FestHostsRouting: an edition that is Off leaves today's page");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{}) == nullptr, "FestHostsRouting: no edition near leaves today's page");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Feign, Phase::Live}}) == nullptr, "FestHostsRouting: Ignite's FEIGN never opens on RiverLine's page");
	ui_test::Expect(rn::NetHubPick(SiteId::Ignite, false, nullptr, HostsNear{{&Rcop, Phase::Afterglow}, {&Feign, Phase::Announced}}) == &Feign,
		"FestHostsRouting: Ignite shows FEIGN announced, not RiverLine's RCOP in its Afterglow");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, Phase::Afterglow}, {&Tco, Phase::Announced}}) == &Rcop,
		"FestHostsRouting: the edition on stage comes before the next one announced");
	ui_test::Expect(rn::NetHubPick(SiteId::RiverLine, false, nullptr, HostsNear{{&Rcop, Phase::Off}, {&Tco, Phase::Announced}}) == &Tco,
		"FestHostsRouting: after the Afterglow the next edition announced takes the page");
	ui_test::Expect(rn::NetHubPick(SiteId::RunItTwice, false, nullptr, HostsNear{{&Feign, Phase::Live}, {&Grit, Phase::MainDay}}) == &Grit, "FestHostsRouting: RunItTwice shows GRIT");

	// The hosts' content rects (§2.8.1; Ignite's ends at y 870, above the dock, L7).
	ui_test::Expect(HostsSameRect(rn::NetHubRect(SiteId::RiverLine), {24.0f, 84.0f, 1552.0f, 866.0f}), "FestHostsRouting: RiverLine's hub rect is {24, 84, 1552, 866}");
	ui_test::Expect(HostsSameRect(rn::NetHubRect(SiteId::Ignite), {24.0f, 84.0f, 1552.0f, 786.0f}), "FestHostsRouting: Ignite's hub rect is {24, 84, 1552, 786}");
	ui_test::Expect(HostsSameRect(rn::NetHubRect(SiteId::RunItTwice), {232.0f, 96.0f, 1344.0f, 854.0f}), "FestHostsRouting: RunItTwice's hub rect is {232, 96, 1344, 854}");

	// The real path: every series a host lists, picked, routes to exactly its own edition (fest::FestivalOf), a plain series (every
	// series before F-net's FN-1) to today's page.
	int SeriesSeen = 0;
	int HubsSeen = 0;
	for (const SiteId Host : {SiteId::RiverLine, SiteId::Ignite, SiteId::RunItTwice})
	{
		for (const ss::net::SeriesInfo& Sr : ss::net::Site(Host).Series())
		{
			const ss::fest::Festival* Of = ss::fest::FestivalOf(Sr.Id);
			const ss::fest::Festival* Want = Of && Of->Site == Host ? Of : nullptr;
			const ss::fest::Festival* Got = rn::NetHubEdition(Host, Sr.Id, 0.0);
			++SeriesSeen;
			HubsSeen += Got ? 1 : 0;
			if (Got != Want)
			{
				const std::string What = "FestHostsRouting: series " + Sr.Id + " routes to " + (Got ? Got->Name : std::string("today's page")) + ", not " +
					(Want ? Want->Name : std::string("today's page"));
				ui_test::Expect(false, What.c_str());
			}
		}
	}
	ui_test::Expect(SeriesSeen > 0, "FestHostsRouting: the three hosts list their series");
	std::printf("FestHostsRouting: %d series on the three hosts, %d of them routed to a festival hub\n", SeriesSeen, HubsSeen);
}
} // namespace

namespace u4_ui_test
{
void SegFestHosts()
{
	FestHostsRouting();
}
} // namespace u4_ui_test
