// rival_sites (update 2 SPEC §6.7): the rival sites. RiverLine unchanged with the registry built, no id collisions,
// schedules per site, fields and overlays, the money (deposits, withdrawals, fees, arrival times, conservation to the cent,
// idempotence across a save), the rent warning, restrictions by site, invitations at their gates, the promos' maths, the
// board split, the threading contract and the trademark screen. Phase A, day 1 (the lead): an empty main. core-sites owns
// this file from B3.
//
// A3 (the lead's seams, §7.2): the registry and the routing (SiteOf, ForEvent, Site, KeyOf, AttachAll, SetHeroAll), RiverLine
// byte for byte with the registry built and two rivals beside it (its templates, instances, listings, payouts, results,
// boards and news against a network built fresh, and through Network(SiteDef)), the rake hook (FeeOf and 0.915, bit for
// bit), the id rule (every rival id prefixed with its site's key, unique across the sites, routed home), a rival network's
// safe defaults (no regulars, no past, no RiverLine posts), GetPopulation("soft"), and the skins (Skin, SkinScope). The
// rivals here are A3's placeholder schedules (NetworkSites.cpp) built from stub SiteDefs; core-sites' real ones replace them.
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace sites_test
{
namespace net = ss::net;
namespace ui = ss::ui;
using ss::Chips;

int Failures = 0;
int Passed = 0;

void Expect(bool Ok, const std::string& What)
{
	if (Ok)
	{
		++Passed;
		return;
	}
	++Failures;
	std::printf("FAIL: %s\n", What.c_str());
}

double SitesSeconds(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - Since).count();
}

std::string SitesNum(double V)
{
	char B[64];
	std::snprintf(B, sizeof(B), "%.17g", V);
	return B;
}

/** A stub SiteDef for a rival (A3): its key, a rake, no field floor, its tables and pace, its palette (§1.8, §5.3). */
net::SiteDef SitesStub(net::SiteId S)
{
	net::SiteDef D;
	D.Id = S;
	D.Key = net::KeyOf(S);
	D.FieldFloor = false;
	D.Rake.Kind = 1;
	if (S == net::SiteId::Ignite)
	{
		D.Name = "Ignite";
		D.Rake.Flat = 0.11;
		D.SecondsPerHand[0] = 30;
		D.SecondsPerHand[1] = 38;
		D.Population = {"freeroll", "soft", "soft", "low", "high"};
		D.Palette = {0x1a1220, 0x20162a, 0x24182c, 0x2c1e36, 0x3a2a48, 0xfff4ec, 0xb8a6c2, 0x7e6a8c, 0xff6b57, 0x4fc3f7, 0xf2c14e, 0xef4d5a, 0xf28a3a, 0x3ecf6e, 0x5a2a4e,
			0x3a1a32};
	}
	else
	{
		D.Name = "RunItTwice";
		D.Rake.Flat = 0.08;
		D.Population = {"freeroll", "low", "high", "high", "high"};
		D.Palette = {0x141a24, 0x18202c, 0x1c2433, 0x232c3d, 0x334155, 0xe8ecf2, 0x8a96a8, 0x566174, 0xd4a843, 0x6b7f99, 0xd4a843, 0xef4d5a, 0xf28a3a, 0x3ecf6e, 0x1f4a3c,
			0x12302a};
	}
	return D;
}

/** Everything a network produces for RiverLine's screens and the world, as text (the probe's fields, in short). */
std::string SitesDump(const net::Network& N, int Days)
{
	std::string Out;
	Out.reserve(8 << 20);
	auto Add = [&Out](const std::string& S) {
		Out += S;
		Out += '\n';
	};
	for (const net::EventTemplate& T : N.Templates())
	{
		Add(T.Id + "|" + T.Name + "|" + T.Series + "|" + std::to_string(T.BuyInCents) + "|" + std::to_string(T.GtdCents) + "|" + std::to_string(static_cast<int>(T.Fmt)) + "|" + T.Speed + "|" +
			SitesNum(T.LevelMinutes) + "|" + std::to_string(T.Field) + "|" + std::to_string(T.StartMinute) + "|" + std::to_string(T.EveryMinutes) + "|" + std::to_string(T.OnlyDay) + "|" +
			std::to_string(T.SeatValueCents) + "|" + T.Ticket + "|" + std::to_string(T.TableSize) + "|" + std::to_string(T.StartStack));
	}
	for (const net::SeriesInfo& S : N.Series())
	{
		Add(S.Id + "|" + S.Name + "|" + std::to_string(S.FirstDay) + "|" + std::to_string(S.LastDay) + "|" + std::to_string(S.GtdCents) + "|" + S.MainEvent);
	}
	for (int Day = -14; Day < Days; ++Day)
	{
		for (const net::EventInstance& E : N.Window(Day * 1440.0, (Day + 1) * 1440.0))
		{
			Add(E.Id + "|" + std::to_string(E.Template) + "|" + std::to_string(E.Seed) + "|" + std::to_string(E.Entries) + "|" + std::to_string(E.Pool) + "|" + SitesNum(E.Duration) + "|" +
				SitesNum(E.LateReg));
			if (Day >= 0 && Day <= 3)
			{
				const ss::LobbyEvent L = N.Listing(E, nullptr, 7);
				std::string Pay;
				for (Chips C : N.Payouts(E))
				{
					Pay += std::to_string(C) + ",";
				}
				Add(L.Spec.Id + "|" + L.Spec.Name + "|" + std::to_string(L.Spec.FeeCents) + "|" + std::to_string(L.Spec.GuaranteeCents) + "|" + std::to_string(L.Spec.Entrants) + "|" +
					SitesNum(L.Spec.SecondsPerHand) + "|" + L.Spec.Population + "|" + std::to_string(L.Spec.BountyCents) + "|" + std::to_string(L.Spec.Levels.size()) + "|" + L.Status + "|" +
					L.GuaranteeLabel + "|" + std::to_string(L.Joinable) + "|" + Pay);
			}
			if (Day <= 2 && E.Start + E.Duration <= 3 * 1440.0)
			{
				std::string R;
				for (const net::Placing& P : N.Result(E).FinalTable)
				{
					R += std::to_string(P.Player) + ":" + std::to_string(P.Prize) + ":" + P.Name + ",";
				}
				Add(E.Id + " r " + R);
			}
		}
	}
	net::HeroStats Hero;
	Hero.Name = "grinder_3c";
	Hero.Earnings = 50000;
	Hero.SeasonPoints = 300.0;
	Hero.NightPoints = 20.0;
	for (double Now : {1440.0 + 127.0, 2 * 1440.0 + 900.0})
	{
		for (int B = 0; B <= static_cast<int>(net::Board::Live); ++B)
		{
			net::BoardRow Mine;
			std::string Row = "board " + std::to_string(B) + " ";
			for (const net::BoardRow& R : N.Leaderboard(static_cast<net::Board>(B), Now, Hero, 40, &Mine))
			{
				Row += std::to_string(R.Player) + ":" + SitesNum(R.Value) + ":" + std::to_string(R.Move) + ",";
			}
			Add(Row + " mine " + std::to_string(Mine.Rank));
		}
		for (const net::NewsItem& It : N.News(Now, Hero, 20))
		{
			Add("news " + It.Title + "|" + It.Tag);
		}
	}
	return Out;
}

// ------------------------------------------------------------------ RiverLine, byte for byte

void RiverLineUnchanged()
{
	const auto T0 = std::chrono::steady_clock::now();
	const std::string Before = SitesDump(net::Shared(), 30);
	const double DumpSeconds = SitesSeconds(T0);
	// The registry and two rivals built beside it, every network attached and detached: RiverLine's own state is untouched.
	const auto T1 = std::chrono::steady_clock::now();
	net::Network& Unregistered = net::Site(net::SiteId::Ignite);
	const double RegistrySeconds = SitesSeconds(T1);
	const auto T2 = std::chrono::steady_clock::now();
	const net::Network Fresh;
	const double RiverLineSeconds = SitesSeconds(T2);
	const auto T3 = std::chrono::steady_clock::now();
	const net::Network Ignite(SitesStub(net::SiteId::Ignite));
	const double IgniteSeconds = SitesSeconds(T3);
	const auto T4 = std::chrono::steady_clock::now();
	const net::Network RunItTwice(SitesStub(net::SiteId::RunItTwice));
	const double RunItTwiceSeconds = SitesSeconds(T4);
	net::AttachAll(nullptr);
	(void)Unregistered;
	const std::string After = SitesDump(net::Shared(), 30);
	Expect(Before == After, "RiverLine's network reads the same after the registry and two rivals are built");
	Expect(SitesDump(Fresh, 30) == Before, "a fresh default network is RiverLine's, byte for byte (" + std::to_string(Before.size()) + " bytes)");
	const net::Network ByDef(net::Def(net::SiteId::RiverLine));
	Expect(SitesDump(ByDef, 30) == Before, "Network(RiverLine's SiteDef) builds RiverLine's network, byte for byte");
	Expect(Fresh.Site() == net::SiteId::RiverLine && net::Shared().Site() == net::SiteId::RiverLine && ByDef.Site() == net::SiteId::RiverLine, "RiverLine's networks say so");
	Expect(Fresh.Templates().size() == net::Shared().Templates().size() && !Fresh.Founding().empty(), "RiverLine keeps its regulars and its templates");
	std::printf("riverline: %zu templates, %zu series, %zu regulars; dump %zu bytes in %.2fs; built in %.3fs (rivals: Ignite %zu templates in %.3fs, RunItTwice %zu in %.3fs, "
				"together %.0f%% of RiverLine's; registry %.4fs)\n",
		Fresh.Templates().size(), Fresh.Series().size(), Fresh.Founding().size(), Before.size(), DumpSeconds, RiverLineSeconds, Ignite.Templates().size(), IgniteSeconds,
		RunItTwice.Templates().size(), RunItTwiceSeconds, 100.0 * (IgniteSeconds + RunItTwiceSeconds) / std::max(1e-9, RiverLineSeconds), RegistrySeconds);
}

uint64_t SitesFnv(uint64_t H, const std::string& S)
{
	for (const char C : S)
	{
		H ^= static_cast<uint64_t>(static_cast<unsigned char>(C));
		H *= 1099511628211ull;
	}
	return H;
}

std::string SitesHex(uint64_t H)
{
	char B[32];
	std::snprintf(B, sizeof(B), "0x%016llx", static_cast<unsigned long long>(H));
	return B;
}

/**
 * RiverLine pinned (review of A3): RiverLineUnchanged compares RiverLine with itself in one process, so a change that moves
 * it the same way everywhere (a template id, an instance id or seed, a fee, a field, a pool) passes it. These are 5bc7d44's
 * numbers, from before the registry existed: every template's ids, buy-in, FeeOf, guarantee, field and shape; every
 * instance's id, seed, entries and pool for 414 days from two weeks before Night One; three weeks of listings (fee,
 * guarantee, entrants, pace, tables). RiverLine's schedule never changes (U9), so these never move. Integers and ids only.
 */
void RiverLinePinned()
{
	const net::Network& N = net::Shared();
	uint64_t Templates = 1469598103934665603ull;
	for (const net::EventTemplate& T : N.Templates())
	{
		Templates = SitesFnv(Templates, T.Id + "|" + T.Series + "|" + T.Ticket + "|" + std::to_string(T.BuyInCents) + "|" + std::to_string(net::FeeOf(T.BuyInCents)) + "|" +
			std::to_string(T.GtdCents) + "|" + std::to_string(T.Field) + "|" + std::to_string(static_cast<int>(T.Fmt)) + "|" + std::to_string(T.StartMinute) + "|" +
			std::to_string(T.EveryMinutes) + "|" + std::to_string(T.LastMinute) + "|" + std::to_string(T.Days) + "|" + std::to_string(T.OnlyDay) + "|" + std::to_string(T.TableSize) + "|" +
			std::to_string(T.StartStack) + "|" + std::to_string(T.LateRegMinutes) + "|" + std::to_string(T.SeatValueCents) + "\n");
	}
	uint64_t Instances = 1469598103934665603ull;
	int Count = 0;
	for (int Day = -14; Day < 400; ++Day)
	{
		for (const net::EventInstance& E : N.Window(Day * 1440.0, (Day + 1) * 1440.0))
		{
			Instances = SitesFnv(Instances, E.Id + "|" + std::to_string(E.Template) + "|" + std::to_string(E.Seed) + "|" + std::to_string(E.Entries) + "|" + std::to_string(E.Pool) + "\n");
			++Count;
		}
	}
	uint64_t Listings = 1469598103934665603ull;
	int Listed = 0;
	for (const net::EventInstance& E : N.Window(0.0, 21 * 1440.0))
	{
		const ss::LobbyEvent L = N.Listing(E, nullptr, 7);
		Listings = SitesFnv(Listings, L.Spec.Id + "|" + std::to_string(L.Spec.BuyInCents) + "|" + std::to_string(L.Spec.FeeCents) + "|" + std::to_string(L.Spec.GuaranteeCents) + "|" +
			std::to_string(L.Spec.Entrants) + "|" + std::to_string(static_cast<int>(L.Spec.SecondsPerHand)) + "|" + L.Spec.Population + "|" + std::to_string(L.Spec.BountyCents) + "|" +
			std::to_string(L.Spec.SeatValueCents) + "|" + L.GuaranteeLabel + "\n");
		++Listed;
	}
	Expect(N.Templates().size() == 8453 && Templates == 0x38f22b3af5ae3e70ull,
		"RiverLine's 8,453 templates are 5bc7d44's (ids, buy-ins, fees, guarantees, fields): " + std::to_string(N.Templates().size()) + " " + SitesHex(Templates));
	Expect(Count == 76862 && Instances == 0x79186cad5d88a08bull,
		"RiverLine's 76,862 instances over 414 days are 5bc7d44's (ids, seeds, entries, pools): " + std::to_string(Count) + " " + SitesHex(Instances));
	Expect(Listed == 3965 && Listings == 0x87b94ae6f0cbea6bull,
		"RiverLine's 3,965 listings over three weeks are 5bc7d44's (fees, guarantees, pace, tables): " + std::to_string(Listed) + " " + SitesHex(Listings));
}

// ------------------------------------------------------------------ the rake hook

void RakeHook()
{
	const net::SiteDef& Rl = net::Def(net::SiteId::RiverLine);
	Expect(Rl.Rake.Kind == 0 && Rl.Linked && Rl.FieldFloor, "RiverLine's rule is FeeOf with the 1.2x field floor, linked to the bank");
	Expect(Rl.SecondsPerHand[0] == 36.0 && Rl.SecondsPerHand[1] == 42.0, "RiverLine's tables run 36 and 42 seconds a hand");
	bool Fees = true;
	for (Chips C = 0; C <= 110000 && Fees; ++C)
	{
		Fees = Rl.Rake.Fee(C) == net::FeeOf(C);
	}
	Expect(Fees, "RiverLine's rake hook is FeeOf to the cent, $0 to $1,100");
	bool Shares = true;
	for (const net::EventTemplate& T : net::Shared().Templates())
	{
		const double Want = T.BuyInCents == 0 ? 0.0 : 0.915;
		const double Got = Rl.Rake.PrizeShare(T);
		Shares = Shares && std::memcmp(&Want, &Got, sizeof(double)) == 0 && Rl.Rake.Fee(T.BuyInCents) == net::FeeOf(T.BuyInCents);
	}
	Expect(Shares, "RiverLine's prize share is 0.915 bit for bit (0 for a freeroll), and its fee FeeOf, for every template");
	int Listings = 0;
	bool Same = true;
	const net::Network& N = net::Shared();
	for (const net::EventInstance& E : N.Window(1440.0, 4 * 1440.0))
	{
		const net::EventTemplate& T = N.TemplateOf(E);
		if (T.JoinIndex >= 0)
		{
			continue;
		}
		const ss::LobbyEvent L = N.Listing(E);
		const net::Tier Tr = net::TierOf(T.BuyInCents);
		const char* Pop = Tr == net::Tier::Freeroll ? "freeroll" : Tr == net::Tier::Micro ? "micro" : Tr == net::Tier::Low ? "low" : "high";
		Same = Same && L.Spec.FeeCents == net::FeeOf(T.BuyInCents) && L.Spec.SecondsPerHand == (T.Speed == "Hyper" ? 36.0 : 42.0) && L.Spec.Population == Pop;
		++Listings;
	}
	Expect(Same && Listings > 300, "every RiverLine listing keeps FeeOf, its seconds a hand and its tier's tables (" + std::to_string(Listings) + " listings)");
}

// ------------------------------------------------------------------ the id rule and the routing

void Routing()
{
	using net::SiteId;
	Expect(std::string(net::KeyOf(SiteId::RiverLine)) == "riverline" && std::string(net::KeyOf(SiteId::Ignite)) == "ignite" &&
			std::string(net::KeyOf(SiteId::RunItTwice)) == "runittwice" && std::string(net::KeyOf(SiteId::Halcyon)) == "halcyon" && std::string(net::KeyOf(SiteId::Count)).empty(),
		"the site keys are fixed with SiteId");
	for (const net::SiteDef& D : net::Sites())
	{
		Expect(D.Key == net::KeyOf(D.Id), "a registered site's Key is its id prefix: " + D.Key);
	}
	const std::pair<const char*, SiteId> Ids[] = {{"", SiteId::RiverLine}, {".", SiteId::RiverLine}, {".ignite", SiteId::RiverLine}, {"ignite", SiteId::RiverLine},
		{"ignite.spark-jackpot", SiteId::Ignite}, {"ignite.spark-jackpot@40320", SiteId::Ignite}, {"runittwice.open26-12", SiteId::RunItTwice}, {"runittwice.sunday-grind", SiteId::RunItTwice},
		{"halcyon.high-roller@3", SiteId::Halcyon}, {"riverline.hh-110", SiteId::RiverLine}, {"ignitex.y", SiteId::RiverLine}, {"Ignite.x", SiteId::RiverLine},
		{"hh-110@1500", SiteId::RiverLine}, {"night-owl@15.5", SiteId::RiverLine}, {"rcop-main", SiteId::RiverLine}, {"championship-main", SiteId::RiverLine},
		{"embercrest-nightly@9", SiteId::RiverLine}, {"x@ignite.y", SiteId::RiverLine}};
	for (const auto& It : Ids)
	{
		Expect(net::SiteOf(It.first) == It.second, std::string("SiteOf(\"") + It.first + "\")");
	}
	// RiverLine's ids are bare: no template, series, ticket or instance id of its reads as another site's.
	const net::Network& Rl = net::Shared();
	std::set<std::string> All;
	bool Bare = true;
	for (const net::EventTemplate& T : Rl.Templates())
	{
		Bare = Bare && T.Id.find('.') == std::string::npos && net::SiteOf(T.Id) == SiteId::RiverLine && net::SiteOf(Rl.TicketOf(T.Id)) == SiteId::RiverLine &&
			(T.Series.empty() || net::SiteOf(T.Series) == SiteId::RiverLine);
		All.insert(T.Id);
	}
	for (const net::SeriesInfo& S : Rl.Series())
	{
		Bare = Bare && S.Id.find('.') == std::string::npos;
	}
	for (int Day = -14; Day < 400 && Bare; Day += 3)
	{
		for (const net::EventInstance& E : Rl.Window(Day * 1440.0, (Day + 1) * 1440.0))
		{
			Bare = Bare && net::SiteOf(E.Id) == SiteId::RiverLine && &net::ForEvent(E.Id) == &net::Shared();
		}
	}
	Expect(Bare, "every RiverLine template, series, ticket and instance id is bare and routes to Shared()");
	Expect(&net::Site(SiteId::RiverLine) == &net::Shared() && &net::ForEvent("hh-110@1500") == &net::Shared(), "RiverLine's network is Shared()");

	// The registry: a network per site, never moved; one not registered yet is empty, so nothing routes to another site's.
	for (SiteId S : {SiteId::Ignite, SiteId::RunItTwice, SiteId::Halcyon})
	{
		net::Network& A = net::Site(S);
		net::Network& B = net::Site(S);
		bool Registered = false;
		for (const net::SiteDef& D : net::Sites())
		{
			Registered = Registered || D.Id == S;
		}
		Expect(&A == &B && &A != &net::Shared() && A.Site() == S, std::string("the registry holds ") + net::KeyOf(S) + "'s own network");
		Expect(Registered ? !A.Templates().empty() : A.Templates().empty() && A.Series().empty(), std::string(net::KeyOf(S)) + (Registered ? ": registered, scheduled" : ": not registered yet, empty"));
		const std::string Id = std::string(net::KeyOf(S)) + ".spark-jackpot@40320";
		Expect(&net::ForEvent(Id) == &A, std::string("a ") + net::KeyOf(S) + " id goes to its network");
		net::EventInstance E;
		Expect(Registered || (!net::ForEvent(Id).FindInstance(Id, E) && !net::Shared().FindInstance(Id, E)), "an unregistered site's id finds nothing anywhere");
	}
	net::Network& Nowhere = net::Site(static_cast<SiteId>(17));
	Expect(Nowhere.Templates().empty() && &Nowhere != &net::Shared(), "a SiteId out of range gets an empty network");

	// The rivals' placeholder schedules: every id prefixed, unique across the sites, at home on its own network.
	for (SiteId S : {SiteId::Ignite, SiteId::RunItTwice})
	{
		const net::Network N(SitesStub(S));
		const std::string Key = std::string(net::KeyOf(S)) + ".";
		bool Prefixed = !N.Templates().empty();
		size_t Fresh = 0;
		for (const net::EventTemplate& T : N.Templates())
		{
			Prefixed = Prefixed && T.Id.rfind(Key, 0) == 0 && net::SiteOf(T.Id) == S && net::SiteOf(N.TicketOf(T.Id)) == S && (T.Series.empty() || net::SiteOf(T.Series) == S) &&
				N.FindTemplate(T.Id) == &T && !Rl.FindTemplate(T.Id);
			Fresh += All.insert(T.Id).second ? 1 : 0;
		}
		for (const net::SeriesInfo& Sr : N.Series())
		{
			Prefixed = Prefixed && net::SiteOf(Sr.Id) == S && N.FindTemplate(Sr.MainEvent) != nullptr && N.FindSeries(Sr.Id) == &Sr;
		}
		Expect(Prefixed, Key + " every template, ticket and series id is prefixed, found at home and nowhere on RiverLine");
		Expect(Fresh == N.Templates().size(), Key + " no template id meets another site's");
		Expect(N.Site() == S && N.Founding().empty() && N.Definition().Key == net::KeyOf(S), Key + " no regulars of its own");
		const int Launch = ss::world::DayOn(2026, 11, 2);
		int Instances = 0;
		bool Home = true;
		for (int Day = Launch; Day < Launch + 7; ++Day)
		{
			for (const net::EventInstance& E : N.Window(Day * 1440.0, (Day + 1) * 1440.0))
			{
				net::EventInstance Back;
				Home = Home && net::SiteOf(E.Id) == S && N.FindInstance(E.Id, Back) && Back.Seed == E.Seed && !Rl.FindInstance(E.Id, Back);
				++Instances;
			}
		}
		Expect(Home && Instances > 7 * 40, Key + " a week's instances (" + std::to_string(Instances) + ") are its own");
		std::printf("%s placeholder: %zu templates, %zu series, %.1f instances a day\n", net::KeyOf(S), N.Templates().size(), N.Series().size(), Instances / 7.0);
	}
}

/**
 * Review of A3: two of the switched call sites, read through RiverLine's ids. No other suite notices if StatsFrom or
 * Session::TicketsFor looks a RiverLine id up on the wrong network (a series title outside MM and RCOP, an RCOP Main seat).
 */
void RoutedCallSites()
{
	const net::Network& Rl = net::Shared();
	// StatsFrom: a title in a series that its name doesn't give away is found through the template on the event's own site.
	bool Titled = false;
	for (int Day = 380; Day < 420 && !Titled; ++Day)
	{
		for (const net::EventInstance& E : Rl.Window(Day * 1440.0, (Day + 1) * 1440.0))
		{
			const net::EventTemplate& T = Rl.TemplateOf(E);
			if (T.Series.empty() || T.Name.rfind("MM #", 0) == 0 || T.Name.rfind("RCOP #", 0) == 0)
			{
				continue;
			}
			ss::HistoryEntry H;
			H.Name = T.Name;
			H.EventId = E.Id;
			H.Place = 1;
			H.Entrants = E.Entries;
			H.Prize = 10000;
			H.BuyInCents = T.BuyInCents;
			ss::HistoryEntry Second = H;
			Second.Place = 2;
			Expect(net::StatsFrom("grinder_3c", {H}, E.Start + E.Duration + 60.0).SeriesTitles == 1 &&
					net::StatsFrom("grinder_3c", {Second}, E.Start + E.Duration + 60.0).SeriesTitles == 0,
				"StatsFrom counts a RiverLine series title by its template (" + E.Id + ")");
			Titled = true;
			break;
		}
	}
	Expect(Titled, "a RiverLine series event outside MM and RCOP in the autumn of 2027");
	// Session::TicketsFor: a later year's RCOP Main takes the "rcop-main" seat, through the template on the event's own site.
	const net::EventTemplate* Main = nullptr;
	for (const net::EventTemplate& T : Rl.Templates())
	{
		Main = !Main && !T.Ticket.empty() && T.Ticket != T.Id && T.OnlyDay > 0 ? &T : Main;
	}
	Expect(Main != nullptr, "a RiverLine template whose seat is another's (a later RCOP Main)");
	if (Main)
	{
		ss::SessionHooks Hooks;
		ss::Session S(Hooks, "review-a3-tickets");
		bool Seated = false;
		for (const net::EventInstance& E : Rl.Window(Main->OnlyDay * 1440.0, (Main->OnlyDay + 1) * 1440.0))
		{
			if (&Rl.TemplateOf(E) != Main)
			{
				continue;
			}
			const ss::LobbyEvent L = Rl.Listing(E);
			const int Before = S.TicketsFor(L);
			S.Life.Tickets[Main->Ticket] += 2;
			Seated = Before == 0 && S.TicketsFor(L) == 2;
			S.Life.Tickets.erase(Main->Ticket);
		}
		Expect(Seated, "Session::TicketsFor finds the " + Main->Ticket + " seats for " + Main->Id);
	}

	// A living world through Nov 1 and then 40 days on. ExpireTickets (switched to the ticket's own site; it runs on the 1st)
	// keeps a regular's RCOP Main seat while the Main (Nov 8) is still ahead; RiverLine's boards by site (Network::Ranked reads
	// World::BoardValue and RankBefore by site) are the world's own, every regular, every board up to Live, until core-world's
	// G3 subtracts the rivals' tallies.
	ss::world::World W;
	W.Create(77u, 1440.0 + 127.0);
	W.AdvanceTo(27 * 1440.0 + 30.0);
	std::map<std::string, int> Held;
	for (const ss::world::Npc& P : W.People())
	{
		for (const auto& T : P.Tickets)
		{
			Held[T.first] += T.second;
		}
	}
	Expect(Held["rcop-main"] > 0, "regulars keep their RCOP Main seats through Nov 1's ticket expiry (" + std::to_string(Held["rcop-main"]) + " seats)");
	W.AdvanceTo(40 * 1440.0 + 60.0);
	bool Same = true;
	std::string Nonzero;
	bool Every = true;
	for (int B = 0; B <= static_cast<int>(net::Board::Live); ++B)
	{
		const net::Board Bd = static_cast<net::Board>(B);
		int Ranked = 0;
		for (int Npc = 0; Npc < static_cast<int>(W.People().size()); ++Npc)
		{
			Same = Same && W.RankBefore(net::SiteId::RiverLine, Bd, Npc) == W.RankBefore(Bd, Npc) &&
				W.BoardValue(net::SiteId::RiverLine, Bd, Npc, W.Clock()) == W.BoardValue(Bd, Npc, W.Clock());
			Ranked += W.RankBefore(Bd, Npc) != 0 ? 1 : 0;
		}
		Every = Every && Ranked > 0;
		Nonzero += " " + std::to_string(Ranked);
	}
	Expect(Same && Every, "RiverLine's boards by site are the world's, every regular, every board up to Live (ranked a day ago:" + Nonzero + ")");
}

// ------------------------------------------------------------------ a rival network's safe defaults

void RivalDefaults()
{
	const net::SiteDef Def = SitesStub(net::SiteId::Ignite);
	const net::Network N(Def);
	N.Prewarm(40 * 1440.0); // no past: nothing to do
	const int Day = ss::world::DayOn(2026, 11, 8);
	bool Listings = true;
	bool Results = true;
	int Seen = 0;
	for (const net::EventInstance& E : N.Window(Day * 1440.0, (Day + 1) * 1440.0))
	{
		const net::EventTemplate& T = N.TemplateOf(E);
		const ss::LobbyEvent L = N.Listing(E);
		const net::Tier Tr = net::TierOf(T.BuyInCents);
		Listings = Listings && L.Spec.FeeCents == Def.Rake.Fee(T.BuyInCents) && L.Spec.Population == Def.Population[static_cast<size_t>(Tr)] &&
			L.Spec.SecondsPerHand == (T.Speed == "Hyper" ? 30.0 : 38.0) && L.Spec.Id == E.Id;
		for (const net::Placing& P : N.Result(E).FinalTable)
		{
			Results = Results && P.Player == -2 && !P.Name.empty();
		}
		++Seen;
	}
	Expect(Seen > 0 && Listings, "a rival's listing takes its own rake, tables and pace (" + std::to_string(Seen) + " events)");
	Expect(Results, "a rival's result, detached, is a final table of handles nobody follows");
	net::HeroStats Hero;
	Hero.Name = "grinder_3c";
	Hero.Wins = 1;
	Hero.FinalTables = 2;
	bool Quiet = true;
	for (const net::NewsItem& It : N.News((Day + 1) * 1440.0, Hero, 40))
	{
		Quiet = Quiet && It.Tag != "RCOP 2026" && It.Tag != "SUMMER SLAM" && It.Tag != "MICRO MADNESS" && It.Tag != "SCHEDULE" && It.Tag != "RECORD" && It.Tag != "TEAM RIVERLINE" &&
			It.Body.find("RiverLine") == std::string::npos;
	}
	Expect(Quiet, "a rival's news carries none of RiverLine's posts");
	for (int B = 0; B <= static_cast<int>(net::Board::HighRollers); ++B)
	{
		net::BoardRow Mine;
		Expect(N.Leaderboard(static_cast<net::Board>(B), (Day + 1) * 1440.0, Hero, 20, &Mine).empty(), "a rival's board " + std::to_string(B) + " has nobody of its own detached");
	}
	// With a living world attached, a rival's boards read the world's site maps: empty until core-world fills them.
	ss::world::World W;
	W.Create(77u, 1440.0 + 127.0);
	net::Network Attached(Def);
	Attached.Attach(&W);
	for (int B = 0; B <= static_cast<int>(net::Board::HighRollers); ++B)
	{
		net::BoardRow Mine;
		Expect(Attached.Leaderboard(static_cast<net::Board>(B), W.Clock(), Hero, 20, &Mine).empty(), "a rival's board " + std::to_string(B) + " is empty in a world that hasn't played it");
	}
	for (int B = 0; B <= static_cast<int>(net::Board::HighRollers); ++B)
	{
		Expect(W.RankBefore(net::SiteId::Ignite, static_cast<net::Board>(B), 3) == 0, "no rank yesterday on a rival's board");
		Expect(B > static_cast<int>(net::Board::Live) ? W.RankBefore(net::SiteId::RiverLine, static_cast<net::Board>(B), 3) == 0
													   : W.RankBefore(net::SiteId::RiverLine, static_cast<net::Board>(B), 3) == W.RankBefore(static_cast<net::Board>(B), 3),
			"RiverLine's rank yesterday by site is today's, and never past Live");
	}

	// AttachAll and SetHeroAll reach every site's network.
	net::AttachAll(&W);
	bool Attached3 = net::Shared().Attached() == &W;
	for (net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice, net::SiteId::Halcyon})
	{
		Attached3 = Attached3 && net::Site(S).Attached() == &W;
	}
	Expect(Attached3, "AttachAll attaches every site's network to the world");
	std::vector<ss::HistoryEntry> History;
	for (const net::EventInstance& E : net::Shared().Window(1440.0 + 600.0, 1440.0 + 900.0))
	{
		ss::HistoryEntry H;
		H.EventId = E.Id;
		H.Place = 1;
		H.Prize = 4242;
		History.push_back(H);
		break;
	}
	ss::HistoryEntry Rival;
	Rival.EventId = "ignite.spark-jackpot@" + std::to_string(Day * 1440 + 1200);
	Rival.Place = 2;
	History.push_back(Rival);
	net::SetHeroAll("grinder_3c", History);
	Expect(net::Shared().HeroName() == "grinder_3c" && net::Site(net::SiteId::RunItTwice).HeroName() == "grinder_3c", "SetHeroAll names the player on every site");
	net::AttachAll(nullptr);
	bool Detached = net::Shared().Attached() == nullptr;
	for (net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice, net::SiteId::Halcyon})
	{
		Detached = Detached && net::Site(S).Attached() == nullptr;
	}
	Expect(Detached, "AttachAll(nullptr) detaches them all");
	Attached.Attach(nullptr);
}

// ------------------------------------------------------------------ the soft population

void SoftPopulation()
{
	using ss::Archetype;
	const ss::Population Soft = {{Archetype::Fish, 30}, {Archetype::Station, 20}, {Archetype::Nit, 14}, {Archetype::Tag, 14}, {Archetype::Lag, 7}, {Archetype::Maniac, 8}, {Archetype::Reg, 7}};
	const ss::Population Micro = {{Archetype::Fish, 26}, {Archetype::Station, 18}, {Archetype::Nit, 15}, {Archetype::Tag, 16}, {Archetype::Lag, 8}, {Archetype::Maniac, 7}, {Archetype::Reg, 10}};
	const ss::Population Freeroll = {{Archetype::Fish, 38}, {Archetype::Station, 18}, {Archetype::Maniac, 14}, {Archetype::Nit, 12}, {Archetype::Tag, 10}, {Archetype::Lag, 5}, {Archetype::Reg, 3}};
	const ss::Population Low = {{Archetype::Fish, 16}, {Archetype::Station, 12}, {Archetype::Nit, 16}, {Archetype::Tag, 24}, {Archetype::Lag, 12}, {Archetype::Maniac, 4}, {Archetype::Reg, 15}, {Archetype::Crusher, 1}};
	const ss::Population High = {{Archetype::Fish, 4}, {Archetype::Nit, 6}, {Archetype::Tag, 20}, {Archetype::Lag, 15}, {Archetype::Reg, 35}, {Archetype::Crusher, 20}};
	Expect(ss::GetPopulation("soft") == Soft, "\"soft\" is §5.3's mix: Fish 30, Station 20, Nit 14, TAG 14, LAG 7, Maniac 8, Reg 7");
	Expect(ss::GetPopulation("micro") == Micro && ss::GetPopulation("freeroll") == Freeroll && ss::GetPopulation("low") == Low && ss::GetPopulation("high") == High &&
			ss::GetPopulation("nobody") == Micro,
		"every existing name seats as before, and an unknown name still seats Micro");
	int SoftFish = 0;
	int MicroFish = 0;
	for (const auto& E : Soft)
	{
		SoftFish += E.first == Archetype::Fish || E.first == Archetype::Station ? E.second : 0;
	}
	for (const auto& E : Micro)
	{
		MicroFish += E.first == Archetype::Fish || E.first == Archetype::Station ? E.second : 0;
	}
	Expect(SoftFish > MicroFish && ss::GetPopulation("soft") != Freeroll, "softer than Micro, and not the freeroll mix");
}

// ------------------------------------------------------------------ the skins

bool SitesSame(const ui::Color& A, const ui::Color& B)
{
	return A.R == B.R && A.G == B.G && A.B == B.B && A.A == B.A;
}

bool SitesSame(const ui::Skin& A, const ui::Skin& B)
{
	const ui::Color* X[16] = {&A.Bg, &A.Bg2, &A.Panel, &A.Panel2, &A.Line, &A.Ink, &A.Muted, &A.Dim, &A.Accent, &A.Accent2, &A.Gold, &A.Red, &A.Orange, &A.Green, &A.Felt, &A.FeltDark};
	const ui::Color* Y[16] = {&B.Bg, &B.Bg2, &B.Panel, &B.Panel2, &B.Line, &B.Ink, &B.Muted, &B.Dim, &B.Accent, &B.Accent2, &B.Gold, &B.Red, &B.Orange, &B.Green, &B.Felt, &B.FeltDark};
	for (size_t I = 0; I < 16; ++I)
	{
		if (!SitesSame(*X[I], *Y[I]))
		{
			return false;
		}
	}
	return true;
}

void Skins()
{
	// RiverLine's 16, as Ui.h wrote them before A3 (web/src/client/canvasui.ts).
	const std::array<uint32_t, 16> RiverLine = {0x0a111c, 0x0e1726, 0x111c2e, 0x16233a, 0x223352, 0xe6edf7, 0x7b8aa3, 0x4d5b73, 0x27d3c3, 0x3b82f6, 0xf2c14e, 0xef4d5a, 0xf28a3a, 0x3ecf6e,
		0x0f6a4e, 0x083a2b};
	const ui::Skin Home = ui::Skin::FromHex(RiverLine);
	Expect(SitesSame(ui::Skin::Current(), Home), "outside any scope, pal:: is RiverLine's palette");
	Expect(SitesSame(ui::Skin::FromHex(net::Def(net::SiteId::RiverLine).Palette), Home), "RiverLine's SiteDef palette is pal::'s, token for token");
	const ui::Skin Ignite = ui::Skin::FromHex(SitesStub(net::SiteId::Ignite).Palette);
	const ui::Skin RunItTwice = ui::Skin::FromHex(SitesStub(net::SiteId::RunItTwice).Palette);
	{
		ui::SkinScope Outer(Ignite);
		Expect(SitesSame(ui::Skin::Current(), Ignite) && SitesSame(ui::pal::Bg, ui::Hex(0x1a1220)) && SitesSame(ui::pal::Accent, ui::Hex(0xff6b57)), "a scope swaps all 16 tokens");
		Expect(SitesSame(ui::Ts(12.0f).Col, ui::Hex(0xfff4ec)), "a default text colour inside a scope is the skin's Ink");
		{
			ui::SkinScope Inner(RunItTwice);
			Expect(SitesSame(ui::Skin::Current(), RunItTwice), "scopes nest");
		}
		Expect(SitesSame(ui::Skin::Current(), Ignite), "an inner scope puts the outer skin back");
	}
	Expect(SitesSame(ui::Skin::Current(), Home) && SitesSame(ui::Ts(12.0f).Col, ui::Hex(0xe6edf7)), "the outer scope puts RiverLine's palette back");

	// Review of A3: every way out of a scope puts the palette back, bit for bit (a return from inside one, a continue or a break
	// out of a loop body that opened one, a scope opened on one branch only, three deep), and a scope over the palette as it
	// stands changes nothing.
	auto Early = [&Ignite, &RunItTwice](int Way) -> bool {
		ui::SkinScope Outer(Ignite);
		if (Way == 0)
		{
			return SitesSame(ui::pal::Accent, ui::Hex(0xff6b57));
		}
		ui::SkinScope Inner(RunItTwice);
		if (Way == 1)
		{
			return SitesSame(ui::pal::Accent, ui::Hex(0xd4a843));
		}
		return false;
	};
	Expect(Early(0) && SitesSame(ui::Skin::Current(), Home), "a return from inside a scope restores RiverLine's palette");
	Expect(Early(1) && SitesSame(ui::Skin::Current(), Home), "a return from inside two scopes restores RiverLine's palette");
	Expect(!Early(2) && SitesSame(ui::Skin::Current(), Home), "falling off the end of two scopes restores RiverLine's palette");
	int Bodies = 0;
	for (int I = 0; I < 6; ++I)
	{
		ui::SkinScope Each(I % 2 == 0 ? Ignite : RunItTwice);
		++Bodies;
		if (I == 1)
		{
			continue;
		}
		if (I == 3)
		{
			break;
		}
	}
	Expect(Bodies == 4 && SitesSame(ui::Skin::Current(), Home), "a continue or a break out of a scoped loop body restores RiverLine's palette");
	for (const bool Rival : {false, true})
	{
		std::unique_ptr<ui::SkinScope> Maybe;
		if (Rival)
		{
			Maybe.reset(new ui::SkinScope(RunItTwice));
		}
		Expect(SitesSame(ui::Skin::Current(), Rival ? RunItTwice : Home), "a scope opened on one branch only applies there");
	}
	Expect(SitesSame(ui::Skin::Current(), Home), "a scope opened on one branch only restores RiverLine's palette");
	{
		ui::SkinScope A(Ignite);
		{
			ui::SkinScope B(RunItTwice);
			{
				ui::SkinScope C(Ignite);
				Expect(SitesSame(ui::Skin::Current(), Ignite), "three deep");
			}
			Expect(SitesSame(ui::Skin::Current(), RunItTwice), "three deep: the middle scope's skin is back");
		}
		Expect(SitesSame(ui::Skin::Current(), Ignite), "three deep: the outer scope's skin is back");
	}
	{
		ui::SkinScope Same(ui::Skin::Current());
		Expect(SitesSame(ui::Skin::Current(), Home), "a scope over the palette as it stands changes nothing");
	}
	bool Bits = true;
	const ui::Color* Now[16] = {&ui::pal::Bg, &ui::pal::Bg2, &ui::pal::Panel, &ui::pal::Panel2, &ui::pal::Line, &ui::pal::Ink, &ui::pal::Muted, &ui::pal::Dim, &ui::pal::Accent, &ui::pal::Accent2,
		&ui::pal::Gold, &ui::pal::Red, &ui::pal::Orange, &ui::pal::Green, &ui::pal::Felt, &ui::pal::FeltDark};
	for (size_t I = 0; I < 16; ++I)
	{
		const ui::Color Want = ui::Hex(RiverLine[I]);
		Bits = Bits && std::memcmp(&Want, Now[I], sizeof(ui::Color)) == 0;
	}
	Expect(Bits, "after every scope, all 16 pal:: tokens are RiverLine's bit for bit");
}

// ------------------------------------------------------------------ review of A3: what B3 builds on

/** A measurer for a draw that has no text (Canvas needs one). */
struct SitesNoText : ui::TextMeasurer
{
	float Width(const std::string& Text, ui::Font Face, float SizePx) const override
	{
		(void)Face;
		return static_cast<float>(Text.size()) * SizePx * 0.5f;
	}
	float Ascent(ui::Font Face, float SizePx) const override
	{
		(void)Face;
		return SizePx * 0.8f;
	}
};

/** The world's text as tab-split lines, read into a world (the desk worker's way). */
void SitesReadWorld(const std::string& Text, ss::world::World& W)
{
	std::vector<std::string> Fields;
	for (size_t From = 0; From < Text.size();)
	{
		const size_t End = std::min(Text.find('\n', From), Text.size());
		Fields.clear();
		for (size_t Cell = From, K = From; K <= End; ++K)
		{
			if (K == End || Text[K] == '\t')
			{
				Fields.emplace_back(Text, Cell, K - Cell);
				Cell = K + 1;
			}
		}
		W.Read(Fields);
		From = End + 1;
	}
	W.Finish();
}

void ReadyForB3()
{
	ss::world::World W;
	W.Create(77u, 1440.0 + 127.0);
	W.AdvanceTo(3 * 1440.0 + 60.0);

	// RanksYesterday ends at Live: the board-only RankBefore answers 0 for a rival's board instead of reading past it.
	bool Guarded = true;
	for (int B = static_cast<int>(net::Board::Today); B <= static_cast<int>(net::Board::HighRollers); ++B)
	{
		for (int Npc : {0, 3, 400})
		{
			Guarded = Guarded && W.RankBefore(static_cast<net::Board>(B), Npc) == 0 && W.BoardValue(static_cast<net::Board>(B), Npc, W.Clock()) == 0.0;
		}
	}
	Expect(Guarded, "World::RankBefore(Board) and BoardValue(Board) answer 0 for a rival's board (RanksYesterday ends at Live)");

	// HeroStats::SiteBoard: a rival's own board shows the player's value from it; RiverLine's seven boards never read it.
	net::HeroStats Hero;
	Hero.Name = "grinder_3c";
	Hero.Earnings = 50000;
	Hero.SeasonPoints = 300.0;
	Hero.NightPoints = 20.0;
	net::HeroStats Loud = Hero;
	Loud.SiteBoard = 1.0e9;
	bool Ignored = true;
	for (int B = 0; B <= static_cast<int>(net::Board::Live); ++B)
	{
		net::BoardRow A;
		net::BoardRow C;
		const std::vector<net::BoardRow> Ra = net::Shared().Leaderboard(static_cast<net::Board>(B), 1440.0 + 900.0, Hero, 30, &A);
		const std::vector<net::BoardRow> Rc = net::Shared().Leaderboard(static_cast<net::Board>(B), 1440.0 + 900.0, Loud, 30, &C);
		bool Same = Ra.size() == Rc.size() && A.Rank == C.Rank && A.Value == C.Value;
		for (size_t I = 0; Same && I < Ra.size(); ++I)
		{
			Same = Ra[I].Player == Rc[I].Player && Ra[I].Value == Rc[I].Value && Ra[I].Rank == Rc[I].Rank;
		}
		Ignored = Ignored && Same;
	}
	Expect(Ignored, "RiverLine's boards never read HeroStats::SiteBoard");
	{
		net::Network Rival(SitesStub(net::SiteId::Ignite));
		Rival.Attach(&W);
		net::HeroStats Mine = Hero;
		Mine.SiteBoard = 12.5;
		net::BoardRow Row;
		const std::vector<net::BoardRow> Rows = Rival.Leaderboard(net::Board::Month, W.Clock(), Mine, 10, &Row);
		Expect(Rows.size() == 1 && Rows[0].Player == -1 && Rows[0].Value == 12.5 && Row.Rank == 1 && Row.Value == 12.5,
			"a rival's Month board ranks the player by HeroStats::SiteBoard (" + std::to_string(Rows.size()) + " rows, rank " + std::to_string(Row.Rank) + ")");
		net::BoardRow None;
		Expect(Rival.Leaderboard(net::Board::Month, W.Clock(), Hero, 10, &None).empty() && None.Rank == 0, "and leaves the player off it with none");
		Rival.Attach(nullptr);
	}

	// A world event's site rides in its flags (FlagSiteBits), clear of every other flag, and survives the world's text.
	Expect((ss::world::FlagSiteBits & (ss::world::FlagQualifier | ss::world::FlagStaked | ss::world::FlagFirst | ss::world::FlagHero | ss::world::FlagLive | ss::world::FlagUnknown)) == 0 &&
			ss::world::FlagSiteOf(0) == net::SiteId::RiverLine && ss::world::FlagSiteOf(ss::world::FlagLive) == net::SiteId::RiverLine &&
			ss::world::FlagSiteOf(ss::world::FlagFirst | (static_cast<int>(net::SiteId::RunItTwice) << ss::world::FlagSiteShift)) == net::SiteId::RunItTwice &&
			ss::world::FlagSiteOf(static_cast<int>(net::SiteId::Halcyon) << ss::world::FlagSiteShift) == net::SiteId::Halcyon,
		"FlagSiteBits holds a SiteId beside the other flags");
	// The world a fortnight on: a headline RiverLine's News shows from its log, tagged as Ignite's in the world's text, leaves
	// RiverLine's News (§5.7's silence) and keeps its tag through a load.
	W.AdvanceTo(14 * 1440.0 + 600.0);
	const double Now = W.Clock();
	auto Titles = [&Hero, Now](const ss::world::World& At) {
		net::AttachAll(&At);
		std::map<std::string, int> T;
		for (const net::NewsItem& It : net::Shared().News(Now, Hero, 80))
		{
			++T[It.Title];
		}
		net::AttachAll(nullptr);
		return T;
	};
	// Compared as loaded from the world's text, which keeps the log's last 2,500 lines: the K-th event is the K-th log line.
	std::string Text;
	W.Write(Text);
	ss::world::World Plain;
	SitesReadWorld(Text, Plain);
	const std::map<std::string, int> Before = Titles(Plain);
	int Pick = -1;
	std::string Headline;
	for (size_t K = 0; K < Plain.Events().size() && Pick < 0; ++K)
	{
		const ss::world::WorldEvent& E = Plain.Events()[K];
		std::string Title;
		std::string Body;
		std::string Label;
		if (E.At > Now - 4.0 * 1440.0 && E.At <= Now && Plain.Headline(E, Title, Body, Label) && Before.count(Title))
		{
			Pick = static_cast<int>(K);
			Headline = Title;
		}
	}
	Expect(Pick >= 0, "RiverLine's News shows a headline from the world's log (" + Headline + ")");
	const std::string Tag = "world\tlog\t";
	size_t Line = Text.find(Tag);
	for (int K = 0; K < Pick && Line != std::string::npos; ++K)
	{
		Line = Text.find(Tag, Line + 1);
	}
	if (Pick >= 0 && Line != std::string::npos)
	{
		const size_t End = Text.find('\n', Line);
		const size_t Last = Text.rfind('\t', End);
		const int Flags = std::atoi(Text.c_str() + Last + 1);
		const int Tagged = Flags | (static_cast<int>(net::SiteId::Ignite) << ss::world::FlagSiteShift);
		std::string Edited = Text.substr(0, Last + 1) + std::to_string(Tagged) + Text.substr(End);
		ss::world::World Back;
		SitesReadWorld(Edited, Back);
		int Found = 0;
		for (const ss::world::WorldEvent& E : Back.Events())
		{
			Found += ss::world::FlagSiteOf(E.Flags) == net::SiteId::Ignite && (E.Flags & ~ss::world::FlagSiteBits) == Flags ? 1 : 0;
		}
		std::string Again;
		Back.Write(Again);
		Expect(Found == 1 && Again == Edited, "a log line's site bits survive the world's text (" + std::to_string(Found) + " tagged)");
		std::map<std::string, int> After = Titles(Back);
		std::map<std::string, int> Expected = Before;
		if (--Expected[Headline] == 0)
		{
			Expected.erase(Headline);
		}
		Expect(After == Expected, "a world headline tagged as Ignite's leaves RiverLine's News, and nothing else does");
	}

	// Skin::Key: one per palette; art cached from a paint that reads pal:: replays each skin's own colours when its key is mixed in.
	const ui::Skin Home = ui::Skin::Current();
	const ui::Skin Fj = ui::Skin::FromHex(SitesStub(net::SiteId::Ignite).Palette);
	Expect(Home.Key() == ui::Skin::FromHex(net::Def(net::SiteId::RiverLine).Palette).Key() && Home.Key() != Fj.Key() && Fj.Key() == ui::Skin::FromHex(SitesStub(net::SiteId::Ignite).Palette).Key(),
		"Skin::Key tells palettes apart and agrees for the same one");
	SitesNoText M;
	// Every vertex the fragment replays, as RGB (the fill's anti-aliased edge fades its alpha, never its colour).
	auto PaintOnce = [&M](uint64_t Key) {
		ui::DrawList L;
		ui::Canvas C(L, M, 64.0f, 64.0f, 1.0f);
		C.Cached(Key ^ ui::Skin::Current().Key(), 4.0f, 4.0f, [](ui::Canvas& Cc) { Cc.FillRect({0.0f, 0.0f, 10.0f, 10.0f}, ui::Paint(ui::pal::Ink)); });
		std::set<std::array<float, 3>> Rgb;
		for (const ui::Vertex& V : L.Vertices)
		{
			Rgb.insert({V.Col.R, V.Col.G, V.Col.B});
		}
		return Rgb;
	};
	auto Only = [](const std::set<std::array<float, 3>>& Rgb, const ui::Color& Want) {
		return Rgb.size() == 1 && *Rgb.begin() == std::array<float, 3>{Want.R, Want.G, Want.B};
	};
	const uint64_t Art = 0x5349544553524556ull; // a key only this check uses
	const std::set<std::array<float, 3>> First = PaintOnce(Art);
	std::set<std::array<float, 3>> Inside;
	{
		ui::SkinScope Scope(Fj);
		Inside = PaintOnce(Art);
	}
	const std::set<std::array<float, 3>> After = PaintOnce(Art);
	Expect(Only(First, Home.Ink) && Only(Inside, Fj.Ink) && Only(After, Home.Ink), "a cached fragment keyed by Skin::Key replays each skin's own colours");
}
} // namespace sites_test

int main(int Argc, char** Argv)
{
	(void)Argc;
	(void)Argv;
	using namespace sites_test;
	const auto T0 = std::chrono::steady_clock::now();
	RiverLineUnchanged();
	RiverLinePinned();
	RakeHook();
	Routing();
	RoutedCallSites();
	RivalDefaults();
	SoftPopulation();
	Skins();
	ReadyForB3();
	std::printf("sites tests: %d passed, %d failed in %.1fs\n", Passed, Failures, SitesSeconds(T0));
	return Failures == 0 ? 0 : 1;
}
