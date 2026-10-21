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
// safe defaults (no regulars, no past, no RiverLine posts), GetPopulation("soft"), and the skins (Skin, SkinScope). A3's
// checks build their rivals from stub SiteDefs (SitesStub), so they read NetworkSites.cpp's schedules without a launch week.
//
// B3 (core-sites, from RealSites on): Ignite Poker and RunItTwice as registered, their schedules, fields, overlays and launch,
// the money, the rent line, the gates, the promotions, the player's numbers by site, the rivals' News, the threading contract,
// the save lines, the marks, SiteStep's cost, the copy lint and the trademark screen, and the soft population's bound on 300
// sit-and-gos a site ("sites_test soft [count]" plays more).
//
// C3 (the gate audit's fixes): RiverLine's row never below its last hundred (HeroBySite), letters that carry money always go
// (PaidLetters), a rival's Flip & Run runs a hyper's full time (RealSchedules), overlay headlines in whole dollars (RivalNews),
// and no real site's format name in any string a player reads (SitesFormatNames).
//
// Update 3 G1 (core-sites-look, SPEC v1.2 §7.7): RealSites pins RunItTwice's 16 red-and-black tokens and Ignite's Afterburner
// palette; U3Moods, U3Overlay, U3Club, U3SecondBoard, U3Desk, U3SundayLive, U3Welcome and U3ShowCopy prove site::show's pure
// functions (the projected overlay against a hand-computed case, the Club's accrual at every tier edge, spike S-D's crafted
// second boards and a fuzz, the Desk from a fixture History); U3Marks the new siteart marks and RunItTwice's six recoloured
// emblem families.
#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"
#include "CopyLint.h"
#include "Trademarks.h"
#include "ShortStack/AI/Grading.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Lobby.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/Ui.h"
#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/SiteArt.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <thread>
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
	// B3, core-world's G3 subtraction (update 2 §5.9; core-world-g3's patch to this check, merged): RiverLine's value is the
	// world's record less each rival's (Earnings less the lifetime money there, the season's boards less this season's there;
	// Series, Night Shift and Live are RiverLine's already), and its ranks a day ago are the world's own (worked out from those
	// values). Forty days on, regulars have played the rivals, so the subtraction is exercised.
	bool Same = true;
	std::string Nonzero;
	bool Every = true;
	int Elsewhere = 0;
	for (int B = 0; B <= static_cast<int>(net::Board::Live); ++B)
	{
		const net::Board Bd = static_cast<net::Board>(B);
		int Ranked = 0;
		for (int Npc = 0; Npc < static_cast<int>(W.People().size()); ++Npc)
		{
			double Less = 0.0;
			for (const net::SiteId S : {net::SiteId::Ignite, net::SiteId::RunItTwice})
			{
				if (const ss::world::SiteRecord* R = W.SiteRecordOf(S, Npc))
				{
					Less += Bd == net::Board::Earnings ? static_cast<double>(R->Lifetime)
						: Bd == net::Board::Season	   ? R->Points
						: Bd == net::Board::Wins	   ? static_cast<double>(R->Wins)
						: Bd == net::Board::FinalTables ? static_cast<double>(R->FinalTables)
														: 0.0;
				}
			}
			Elsewhere += Less > 0.0 ? 1 : 0;
			const double Whole = W.BoardValue(Bd, Npc, W.Clock());
			const double Home = W.BoardValue(net::SiteId::RiverLine, Bd, Npc, W.Clock());
			const double Want = Whole - Less > 1e-6 ? Whole - Less : 0.0;
			Same = Same && W.RankBefore(net::SiteId::RiverLine, Bd, Npc) == W.RankBefore(Bd, Npc) && std::fabs(Home - Want) <= 1e-6 * std::max(1.0, Whole);
			Ranked += W.RankBefore(Bd, Npc) != 0 ? 1 : 0;
		}
		Every = Every && Ranked > 0;
		Nonzero += " " + std::to_string(Ranked);
	}
	Expect(Same && Every && Elsewhere > 0,
		"RiverLine's boards by site are the world's less the rivals', every regular, every board up to Live (" + std::to_string(Elsewhere) +
			" rows with rival play; ranked a day ago:" + Nonzero + ")");
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

// ================================================================== B3 (core-sites): the real rivals
//
// Ignite Poker and RunItTwice as registered (Sites.cpp) with their real schedules (NetworkSites.cpp), their fields and
// guarantees, the launch, the money (SiteMoney.cpp), the gates, the promotions, the player's numbers by site, the rivals'
// News, the threading contract, the copy and the trademark screen.

namespace site = ss::site;
using ss::life::State;

constexpr double SitesDay = 1440.0;

/** WCAG's contrast ratio between two colours (the laptop's skins: Ink on Panel and Muted on Bg at 4.5 or more). */
double SitesContrast(uint32_t A, uint32_t B)
{
	auto Lum = [](uint32_t H) {
		auto C = [](double V) {
			V /= 255.0;
			return V <= 0.03928 ? V / 12.92 : std::pow((V + 0.055) / 1.055, 2.4);
		};
		return 0.2126 * C(static_cast<double>((H >> 16) & 255)) + 0.7152 * C(static_cast<double>((H >> 8) & 255)) + 0.0722 * C(static_cast<double>(H & 255));
	};
	double La = Lum(A);
	double Lb = Lum(B);
	if (La < Lb)
	{
		std::swap(La, Lb);
	}
	return (La + 0.05) / (Lb + 0.05);
}

const net::SiteId SitesRivals[] = {net::SiteId::Ignite, net::SiteId::RunItTwice};

void RealSites()
{
	const net::SiteDef& Rl = net::Def(net::SiteId::RiverLine);
	Expect(net::Sites().size() == 3 && net::Sites()[0].Id == net::SiteId::RiverLine, "three sites registered, RiverLine first (Halcyon is P1)");
	Expect(&net::Def(net::SiteId::Halcyon) == &net::Sites().front() && !site::IsRival(net::SiteId::Halcyon) && net::Site(net::SiteId::Halcyon).Templates().empty(),
		"Halcyon isn't registered: its network is empty and it takes no account");
	for (net::SiteId S : SitesRivals)
	{
		const net::SiteDef& D = net::Def(S);
		const std::string K = D.Key;
		Expect(D.Id == S && D.Key == net::KeyOf(S) && !D.Linked && !D.P1 && !D.FieldFloor && site::IsRival(S), K + ": a registered rival with no field floor and a wallet");
		bool Apart = true;
		std::set<uint32_t> Own(D.Palette.begin(), D.Palette.end());
		for (uint32_t C : D.Palette)
		{
			Apart = Apart && std::find(Rl.Palette.begin(), Rl.Palette.end(), C) == Rl.Palette.end();
		}
		Expect(Apart && Own.size() == 16, K + ": all 16 of its colours differ from RiverLine's (the palette-leak check needs no exemption)");
		const double InkPanel = SitesContrast(D.Palette[5], D.Palette[2]);
		const double MutedBg = SitesContrast(D.Palette[6], D.Palette[0]);
		Expect(InkPanel >= 4.5 && MutedBg >= 4.5, K + ": Ink on Panel " + SitesNum(InkPanel) + ", Muted on Bg " + SitesNum(MutedBg) + " (4.5 or more)");
		// The rake: fee plus the pool's share is the buy-in to the cent, for every template, and every listing charges the rule's fee.
		const net::Network& N = net::Site(S);
		bool Cent = !N.Templates().empty();
		for (const net::EventTemplate& T : N.Templates())
		{
			const Chips Pool = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * D.Rake.PrizeShare(T)));
			Cent = Cent && D.Rake.Fee(T.BuyInCents) + Pool == T.BuyInCents && D.Rake.Fee(T.BuyInCents) > 0;
		}
		Expect(Cent, K + ": fee + prize pool = buy-in to the cent on every template (" + std::to_string(N.Templates().size()) + ")");
	}
	const net::SiteDef& Ig = net::Def(net::SiteId::Ignite);
	const net::SiteDef& Rt = net::Def(net::SiteId::RunItTwice);
	// Update 3 G1 (core-sites-look; SPEC §1.11, §3.3, §13.2 #1, §13.3): RunItTwice's 16 red-and-black tokens, pinned, and
	// Ignite's Afterburner palette for 00:00-06:00. Nobody else has a night palette.
	const std::array<uint32_t, 16> RedBlack = {0x0c0c0f, 0x111114, 0x16161a, 0x1e1e23, 0x2c2c33, 0xececef, 0xa0a0aa, 0x62626c, 0xd2141a, 0xb6bbc5, 0xffffff, 0xff3d3d, 0xf5a524,
		0x48c77a, 0x1a1a1f, 0x09090b};
	const double RtInk = SitesContrast(Rt.Palette[5], Rt.Palette[2]);
	const double RtMuted = SitesContrast(Rt.Palette[6], Rt.Palette[0]);
	std::printf("RunItTwice (red and black): Ink on Panel %.2f:1, Muted on Bg %.2f:1, Hot White on Signal Red %.2f:1, Signal Red on Bg %.2f:1\n", RtInk, RtMuted,
		SitesContrast(Rt.Palette[10], Rt.Palette[8]), SitesContrast(Rt.Palette[8], Rt.Palette[0]));
	Expect(Rt.Palette == RedBlack && std::fabs(RtInk - 15.30) < 0.005 && std::fabs(RtMuted - 7.54) < 0.005,
		"RunItTwice wears the 16 red-and-black tokens (Signal Red d2141a on Onyx 0c0c0f): Ink on Panel 15.30, Muted on Bg 7.54");
	Expect(std::fabs(SitesContrast(Rt.Palette[10], Rt.Palette[8]) - 5.45) < 0.005 && std::fabs(SitesContrast(Rt.Palette[8], Rt.Palette[0]) - 3.59) < 0.005,
		"Hot White on Signal Red reads 5.45:1 (slabs, tags); Signal Red on Onyx 3.59:1, so red is light, never ink");
	bool ApartFromIgnite = true;
	for (const uint32_t C : Rt.Palette)
	{
		ApartFromIgnite = ApartFromIgnite && std::find(Ig.Palette.begin(), Ig.Palette.end(), C) == Ig.Palette.end() &&
			std::find(Ig.NightPalette.begin(), Ig.NightPalette.end(), C) == Ig.NightPalette.end();
	}
	Expect(ApartFromIgnite, "none of RunItTwice's 16 is one of Ignite's tokens, day or night");
	const std::set<uint32_t> Night(Ig.NightPalette.begin(), Ig.NightPalette.end());
	bool NightApart = true;
	for (const uint32_t C : Ig.NightPalette)
	{
		NightApart = NightApart && std::find(Rl.Palette.begin(), Rl.Palette.end(), C) == Rl.Palette.end();
	}
	const double NightInk = SitesContrast(Ig.NightPalette[5], Ig.NightPalette[2]);
	const double NightMuted = SitesContrast(Ig.NightPalette[6], Ig.NightPalette[0]);
	std::printf("Ignite's Afterburner Hours: Ink on Panel %.2f:1, Muted on Bg %.2f:1\n", NightInk, NightMuted);
	Expect(Night.size() == 16 && NightApart && NightInk >= 4.5 && NightMuted >= 4.5 && Ig.NightPalette[0] == 0x120c1f && Ig.NightPalette[2] == 0x1a1230 &&
			Ig.NightPalette[5] == 0xf3eaff && Ig.NightPalette[8] == 0xb57bff && Ig.NightPalette[9] == 0x3ee6d0,
		"Ignite's Afterburner palette: 16 of its own (Bg 120c1f, Panel 1a1230, Accent b57bff, Accent2 3ee6d0, Ink f3eaff), none RiverLine's, 4.5:1 or more");
	const std::array<uint32_t, 16> None{};
	Expect(Ig.NightFromMin == 0 && Ig.NightToMin == 360 && Rl.NightPalette == None && Rt.NightPalette == None,
		"Afterburner Hours run 00:00-06:00 world time; RiverLine and RunItTwice have no night palette");
	Expect(Ig.Rake.Fee(550) == 61 && Ig.Rake.Fee(110) == 12 && Ig.Rake.Fee(2200) == 242 && Ig.Rake.Fee(0) == 0, "Ignite: 11% flat ($5.50 pays 61 cents)");
	Expect(Rt.Rake.Fee(330) == 26 && Rt.Rake.Fee(5500) == 440 && Rt.Rake.Fee(8250) == 660 && Rt.Rake.Fee(10900) == 763 && Rt.Rake.Fee(21500) == 1505 && Rt.Rake.Fee(53000) == 2915,
		"RunItTwice: 8% to $55, 7% from $109 to $215, 5.5% from $530");
	Expect(Ig.SecondsPerHand[0] == 30.0 && Ig.SecondsPerHand[1] == 38.0 && Rt.SecondsPerHand[0] == 36.0 && Rt.SecondsPerHand[1] == 42.0, "seconds a hand: Ignite 30/38, RunItTwice 36/42");
	Expect(Ig.Population[1] == "soft" && Ig.Population[2] == "soft" && Rt.Population[1] == "low" && Rt.Population[2] == "high" && Ig.Softness[1] == -0.06 && Rt.Softness[2] == 0.03,
		"tables: Ignite seats the soft mix (-0.06), RunItTwice one band tougher (+0.03)");
	Expect(Ig.Traffic.Scale == 0.30 && Ig.Traffic.Sunday == 1.2 && Ig.Traffic.PeakHour == 20 && Rt.Traffic.Scale == 0.55 && Rt.Traffic.Sunday == 1.5 && Rt.Traffic.PeakHour == 21,
		"traffic: Ignite 0.30 (Sunday x1.2, peak 8 PM), RunItTwice 0.55 (x1.5, 9 PM)");
	Expect(Ig.LaunchDay == ss::world::DayOn(2026, 11, 2) && Rt.LaunchDay == net::NightOneDay && Ig.LaunchGtd == 1.5 && Rt.TeamSize == 6 && Rt.TeamName == "Team RunItTwice",
		"Ignite opens Mon Nov 2 with a launch week x1.5; RunItTwice's lobby on Night One, its team of six");
	const net::CashMethod* Card = site::MethodFor(net::SiteId::Ignite, "", false);
	const net::CashMethod* Out = site::MethodFor(net::SiteId::Ignite, "", true);
	const net::CashMethod* RtOut = site::MethodFor(net::SiteId::RunItTwice, "", true);
	Expect(Card && Card->Id == "card" && Card->FeePermille == 30 && Card->MinCents == 1000 && Out && Out->WithdrawFeeCents == 300 && Out->WithdrawHoursMin == 72.0 &&
			Out->WithdrawHoursMax == 120.0 && RtOut && RtOut->WithdrawFeeCents == 500 && RtOut->WithdrawHoursMin == 48.0 && RtOut->WithdrawHoursMax == 48.0 &&
			site::MethodFor(net::SiteId::RunItTwice, "", false)->FeePermille == 25 && !site::MethodFor(net::SiteId::RunItTwice, "bank", false) &&
			!site::MethodFor(net::SiteId::RiverLine, "", false) && Ig.MinWithdraw == 2000 && Rt.MinWithdraw == 2000,
		"the Cashier: one method each way (card in at 3% / 2.5%, $10 minimum; bank out in 72-120 h for $3 / 48 h for $5, $20 minimum)");
}

void RealSchedules()
{
	struct Want
	{
		net::SiteId S;
		int Dailies;
		Chips Lo, Hi;
		int Sundays;
		size_t TemplatesLo, TemplatesHi;
	};
	const Want Wants[] = {{net::SiteId::Ignite, 18, 110, 2200, 1, 650, 900}, {net::SiteId::RunItTwice, 22, 330, 21500, 3, 1400, 1650}};
	std::set<std::string> Every;
	for (const net::EventTemplate& T : net::Shared().Templates())
	{
		Every.insert(T.Id);
	}
	for (const Want& W : Wants)
	{
		const net::Network& N = net::Site(W.S);
		const std::string K = net::KeyOf(W.S);
		int Dailies = 0;
		int Sundays = 0;
		Chips Lo = 1 << 30;
		Chips Hi = 0;
		bool Prefixed = true;
		size_t Fresh = 0;
		for (const net::EventTemplate& T : N.Templates())
		{
			const bool Daily = T.OnlyDay == -9999 && T.Days == 0x7f;
			Dailies += Daily ? 1 : 0;
			Sundays += T.OnlyDay == -9999 && T.Days == (1 << 6) ? 1 : 0;
			Lo = Daily ? std::min(Lo, T.BuyInCents) : Lo;
			Hi = Daily ? std::max(Hi, T.BuyInCents) : Hi;
			Prefixed = Prefixed && T.Id.rfind(K + ".", 0) == 0 && net::SiteOf(T.Id) == W.S && net::SiteOf(N.TicketOf(T.Id)) == W.S && N.FindTemplate(T.Id) == &T &&
				&net::ForEvent(T.Id + "@100") == &N;
			Fresh += Every.insert(T.Id).second ? 1 : 0;
		}
		Expect(Dailies == W.Dailies && Lo == W.Lo && Hi == W.Hi && Sundays == W.Sundays,
			K + ": " + std::to_string(Dailies) + " dailies from " + net::BuyIn(Lo) + " to " + net::BuyIn(Hi) + ", " + std::to_string(Sundays) + " Sunday majors");
		Expect(N.Templates().size() >= W.TemplatesLo && N.Templates().size() <= W.TemplatesHi, K + ": " + std::to_string(N.Templates().size()) + " templates through 2040");
		Expect(Prefixed && Fresh == N.Templates().size(), K + ": every template, ticket and series id is its own, unique across the three sites");
		// Deterministic: built again from the same def, the same schedule, instance for instance.
		const net::Network Again(net::Def(W.S));
		Expect(SitesDump(Again, 60) == SitesDump(N, 60), K + ": its schedule is the same every build");
	}
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	const net::EventTemplate* Spark = Ig.FindTemplate("ignite.spark-jackpot");
	const net::EventTemplate* Flip = Ig.FindTemplate("ignite.flip-go");
	const net::EventTemplate* Blaze = Ig.FindTemplate("ignite.sunday-blaze");
	Expect(Spark && Spark->BuyInCents == 550 && Spark->StartMinute == 20 * 60 && Spark->GtdCents > 0 && Spark->Featured, "the Spark Jackpot: $5.50 every night at 8 PM, guaranteed");
	Expect(Flip && Flip->Fmt == net::Format::Flip && Flip->EveryMinutes == 60 && Flip->Speed == "Hyper" && Ig.FindTemplate("ignite.flip-go-bounty") &&
			Ig.FindTemplate("ignite.flip-go-bounty")->Fmt == net::Format::Bounty,
		"Flip & Run every hour, and the Flip & Run Bounty");
	Expect(Blaze && Blaze->BuyInCents == 1100 && Blaze->Days == 1 << 6 && Blaze->GtdCents == 1600000, "the Sunday Blaze: $11 on Sundays, $16,000 guaranteed (" + (Blaze ? ss::Money(Blaze->GtdCents) : std::string()) + ")");
	const net::EventTemplate* Hundred = Rt.FindTemplate("runittwice.sunday-100k");
	const net::EventTemplate* Grind = Rt.FindTemplate("runittwice.sunday-grind");
	const net::EventTemplate* Bridge = Rt.FindTemplate("runittwice.high-bridge");
	Expect(Hundred && Hundred->BuyInCents == 10900 && Hundred->GtdCents == 10000000 && Grind && Grind->BuyInCents == 2200 && Bridge && Bridge->BuyInCents == 53000,
		"RunItTwice's Sundays: the $22 Sunday Grind, the $109 Sunday 100K with $100,000 guaranteed, the $530 High Bridge");
	Expect(Rt.FindTemplate("runittwice.supper-stack") && Rt.FindTemplate("runittwice.supper-stack")->BuyInCents == 550 && Rt.FindTemplate("runittwice.run-it-back") &&
			Rt.FindTemplate("runittwice.run-it-back")->BuyInCents == 3300,
		"the welcome tickets' $5.50 Supper Stack, and Run It Back at $33");
	// The series calendar.
	const net::SeriesInfo* Open = Rt.FindSeries("runittwice.open26");
	const net::SeriesInfo* Spring = Rt.FindSeries("runittwice.openspring27");
	const net::SeriesInfo* Iron = Rt.FindSeries("runittwice.ironworks27");
	const net::SeriesInfo* Feb = Ig.FindSeries("ignite.wildfeb27");
	const net::SeriesInfo* Aug = Ig.FindSeries("ignite.wildaug27");
	Expect(Open && Open->FirstDay == ss::world::DayOn(2026, 11, 13) && Open->LastDay == ss::world::DayOn(2026, 11, 29) && Open->Events == 40 && Rt.FindTemplate(Open->MainEvent) &&
			Rt.FindTemplate(Open->MainEvent)->BuyInCents == 53000 && Rt.FindTemplate("runittwice.open26-12"),
		"the RunItTwice Open 2026: Nov 13-29, 40 events, a $530 Main (and runittwice.open26-12)");
	bool Bounties = Iron != nullptr;
	for (const net::EventTemplate& T : Rt.Templates())
	{
		Bounties = Bounties && (!Iron || T.Series != Iron->Id || T.Fmt == net::Format::Bounty);
	}
	Expect(Spring && ss::world::DayOn(2027, 4, 1) <= Spring->FirstDay && Spring->FirstDay < ss::world::DayOn(2027, 5, 1) && Iron && Iron->Events == 24 && Bounties &&
			ss::world::DayOn(2027, 7, 1) <= Iron->FirstDay && Iron->FirstDay < ss::world::DayOn(2027, 8, 1),
		"the Open in April too, and Ironworks in July: 24 knockout events");
	Expect(Feb && Aug && Feb->Events == 24 && Aug->Events == 24 && ss::world::DayOn(2027, 2, 1) <= Feb->FirstDay && Feb->FirstDay < ss::world::DayOn(2027, 3, 1) &&
			ss::world::DayOn(2027, 8, 1) <= Aug->FirstDay && Aug->FirstDay < ss::world::DayOn(2027, 9, 1) && !Ig.FindSeries("ignite.wildaug26"),
		"Wildfire in February and August from 2027 (after Ignite opens), 24 events each");
	// (Review.) A rival series event costs a standard buy-in, as RiverLine's series do ($5.50, $11, $22, $55, $109...), never
	// a $67.90 or a $118.20.
	static const Chips Standard[] = {110, 220, 330, 440, 550, 770, 1100, 1650, 2200, 2750, 3300, 4400, 5500, 8250, 10900, 16200, 21500, 32000, 53000};
	bool Priced = true;
	std::set<Chips> Seen;
	for (const net::Network* Nw : {&Ig, &Rt})
	{
		for (const net::EventTemplate& T : Nw->Templates())
		{
			const bool Known = std::find(std::begin(Standard), std::end(Standard), T.BuyInCents) != std::end(Standard);
			Priced = Priced && (T.Series.empty() || Known);
			Seen.insert(T.Series.empty() ? 0 : T.BuyInCents);
		}
	}
	Expect(Priced && Seen.size() >= 12, "every rival series event is priced at a standard buy-in (" + std::to_string(Seen.size() - 1) + " of them in use)");
	// The lobby: a rival's Flip & Run plays (RiverLine's "Flip & Run unlocks later" is RiverLine's), its tables and pace its own.
	const int Day = ss::world::DayOn(2026, 11, 20);
	int Flips = 0;
	bool FlipsPlay = true;
	for (const net::EventInstance& E : Ig.Window(Day * SitesDay, (Day + 1) * SitesDay))
	{
		const net::EventTemplate& T = Ig.TemplateOf(E);
		if (T.Fmt == net::Format::Flip)
		{
			std::string Lock;
			const ss::LobbyEvent L = Ig.Listing(E, &Lock, 0);
			FlipsPlay = FlipsPlay && L.Joinable && Lock.empty() && L.Spec.StartingStack == 2000 && L.Spec.SecondsPerHand == 30.0 && L.Spec.Population == "soft";
			++Flips;
		}
	}
	std::string RlLock;
	bool RlLocked = false;
	for (const net::EventInstance& E : net::Shared().Window(Day * SitesDay, (Day + 1) * SitesDay))
	{
		if (net::Shared().TemplateOf(E).Fmt == net::Format::Flip && !RlLocked)
		{
			RlLocked = !net::Shared().Joinable(E, &RlLock, 7) && RlLock == "Flip & Run unlocks later in your career.";
		}
	}
	Expect(Flips == 24 && FlipsPlay && RlLocked, "Ignite's 24 Flip & Run a day play from a short stack at once; RiverLine's Flip Frenzy is still locked (" + std::to_string(Flips) + ")");
	// (C3, the audit's should-fix 3.) A rival's Flip & Run is a short-stacked eight-handed hyper, not RiverLine's all-in flip, so
	// the lobby gives it a hyper's full time: at RiverLine's quarter it called a running one Finished about 18 minutes in and
	// closed its late registration near 10. The engine plays one out; the lobby's clock covers it. RiverLine's keeps its quarter.
	{
		const net::EventInstance* Fr = nullptr;
		std::vector<net::EventInstance> Today = Ig.Window(Day * SitesDay + 18 * 60.0, Day * SitesDay + 19 * 60.0);
		for (const net::EventInstance& E : Today)
		{
			Fr = !Fr && Ig.TemplateOf(E).Fmt == net::Format::Flip ? &E : Fr;
		}
		double Played = 0.0;
		bool Running = false;
		if (Fr)
		{
			const ss::LobbyEvent L = Ig.Listing(*Fr, nullptr, 0);
			ss::Tournament T(L.Spec, "Hero", "sites-flip-length");
			for (int Guard = 0; !T.bFinished && Guard < 100000; ++Guard)
			{
				T.SimulateTick();
			}
			Played = T.ClockMinutes() - L.Spec.StartClock;
			Running = T.bFinished && Ig.Live(*Fr, Fr->Start + 0.9 * Played).St != net::Status::Finished;
		}
		const net::EventTemplate* Frenzy = net::Shared().FindTemplate("flip-1");
		bool Quarter = false;
		for (const net::EventInstance& E : net::Shared().Window(Day * SitesDay, (Day + 1) * SitesDay))
		{
			const net::EventTemplate& T = net::Shared().TemplateOf(E);
			Quarter = Quarter || (&T == Frenzy &&
									 E.Duration == T.LevelMinutes * (14.0 + 3.2 * std::log2(static_cast<double>(E.Entries) / static_cast<double>(T.TableSize) + 1.0)) * 0.25);
		}
		std::printf("flip & run: %d entries, the lobby's %.0f minutes (late registration %.0f) against the engine's %.0f\n", Fr ? Fr->Entries : 0, Fr ? Fr->Duration : 0.0,
			Fr ? Fr->LateReg : 0.0, Played);
		Expect(Fr && Played > 20.0 && Fr->Duration >= Played && Running && Quarter,
			"Ignite's Flip & Run runs the lobby's full hyper time (" + SitesNum(Fr ? Fr->Duration : 0.0) + " minutes against the engine's " + SitesNum(Played) +
				"), still running 90% of the way in; RiverLine's Flip Frenzy keeps its quarter");
	}
}

/** Whether an instance overlays: what its field puts in falls short of its guarantee. */
bool SitesOverlays(const net::Network& N, const net::EventInstance& E)
{
	const net::EventTemplate& T = N.TemplateOf(E);
	const Chips In = static_cast<Chips>(std::llround(static_cast<double>(T.BuyInCents) * N.Definition().Rake.PrizeShare(T))) * static_cast<Chips>(E.Entries);
	return T.GtdCents > 0 && T.BuyInCents > 0 && In < T.GtdCents;
}

void LaunchAndFields()
{
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	const int Launch = net::Def(net::SiteId::Ignite).LaunchDay;
	// No history before launch: nothing on Ignite before Nov 2 or on RunItTwice before Night One, detached or attached to a
	// world that opened them on their days; and an id from before can't be found.
	Expect(Ig.Window((Launch - 30) * SitesDay, Launch * SitesDay).empty() && !Ig.Window(Launch * SitesDay, (Launch + 1) * SitesDay).empty(), "Ignite lists nothing before Mon Nov 2");
	Expect(Rt.Window(-14 * SitesDay, net::NightOneDay * SitesDay).empty() && !Rt.Window(net::NightOneDay * SitesDay, (net::NightOneDay + 1) * SitesDay).empty(),
		"RunItTwice lists nothing before Night One");
	net::EventInstance Found;
	Expect(!Ig.FindInstance("ignite.spark-jackpot@" + std::to_string((Launch - 1) * 1440 + 1200), Found) &&
			Ig.FindInstance("ignite.spark-jackpot@" + std::to_string((Launch + 7) * 1440 + 1200), Found),
		"Ignite's Spark Jackpot the night before launch was never scheduled; a week after, it is");
	// Ignite's launch week: every guaranteed event those seven days is its own one-off, the guarantee half again as big; the
	// daily stays out; on Nov 9 the dailies are back.
	int Twins = 0;
	bool OnlyTwins = true;
	bool Raised = true;
	for (int D = Launch; D < Launch + 7; ++D)
	{
		for (const net::EventInstance& E : Ig.Window(D * SitesDay, (D + 1) * SitesDay))
		{
			const net::EventTemplate& T = Ig.TemplateOf(E);
			const std::string Base = net::netsites_detail::NetSitesLaunchBase(T.Id);
			if (T.GtdCents <= 0)
			{
				continue;
			}
			OnlyTwins = OnlyTwins && !Base.empty() && T.OnlyDay == D;
			const net::EventTemplate* Daily = Base.empty() ? nullptr : Ig.FindTemplate(Base);
			const double Ratio = Daily ? static_cast<double>(T.GtdCents) / static_cast<double>(Daily->GtdCents) : 0.0;
			Raised = Raised && Daily && Ratio > 1.35 && Ratio < 1.65 && T.Name == Daily->Name && T.StartMinute == Daily->StartMinute;
			++Twins;
		}
	}
	Expect(Twins >= 7 * 13 && OnlyTwins && Raised, "Ignite's launch week: " + std::to_string(Twins) + " guaranteed events run as one-offs at about 1.5x the guarantee");
	const net::EventTemplate* SundayLaunch = Ig.FindTemplate("ignite.sunday-blaze-launch7");
	Expect(SundayLaunch && SundayLaunch->OnlyDay == Launch + 6 && SundayLaunch->GtdCents == 2500000, "launch week's Sunday Blaze is the $25,000 Sunday");
	bool Back = false;
	for (const net::EventInstance& E : Ig.Window((Launch + 7) * SitesDay, (Launch + 8) * SitesDay))
	{
		Back = Back || E.Id.rfind("ignite.spark-jackpot@", 0) == 0;
	}
	Expect(Back, "on Nov 9 the Spark Jackpot is its daily self again");
	// Fields without the floor, guarantees at 0.9 of the median pool: 10-25% of guaranteed events overlay over a month; at least
	// half in Ignite's launch week.
	auto Rate = [](const net::Network& N, int From, int To, int& Gtd) {
		int Over = 0;
		Gtd = 0;
		for (const net::EventInstance& E : N.Window(From * SitesDay, To * SitesDay))
		{
			if (N.TemplateOf(E).GtdCents > 0)
			{
				++Gtd;
				Over += SitesOverlays(N, E) ? 1 : 0;
			}
		}
		return Gtd > 0 ? static_cast<double>(Over) / static_cast<double>(Gtd) : 0.0;
	};
	int G1 = 0;
	int G2 = 0;
	int G3 = 0;
	int G4 = 0;
	const double IgMonth = Rate(Ig, Launch + 7, Launch + 37, G1);
	const double IgWeek = Rate(Ig, Launch, Launch + 7, G2);
	const double RtMonth = Rate(Rt, net::NightOneDay, net::NightOneDay + 30, G3);
	const double RtOpen = Rate(Rt, ss::world::DayOn(2026, 11, 13), ss::world::DayOn(2026, 12, 13), G4);
	std::printf("overlays: Ignite %.1f%% of %d (launch week %.1f%% of %d); RunItTwice %.1f%% of %d (with the Open %.1f%% of %d)\n", 100.0 * IgMonth, G1, 100.0 * IgWeek, G2,
		100.0 * RtMonth, G3, 100.0 * RtOpen, G4);
	if (std::getenv("SS_SITES_OVERLAYS"))
	{
		for (const net::Network* N : {&Ig, &Rt})
		{
			std::map<std::string, std::pair<int, int>> By;
			for (const net::EventInstance& E : N->Window((Launch + 7) * SitesDay, (Launch + 67) * SitesDay))
			{
				const net::EventTemplate& T = N->TemplateOf(E);
				if (T.GtdCents > 0)
				{
					By[T.Series.empty() ? T.Id : T.Series].first += SitesOverlays(*N, E) ? 1 : 0;
					By[T.Series.empty() ? T.Id : T.Series].second += 1;
				}
			}
			for (const auto& It : By)
			{
				std::printf("  %-36s %3d / %3d  %5.1f%%  gtd %s\n", It.first.c_str(), It.second.first, It.second.second, 100.0 * It.second.first / It.second.second,
					N->FindTemplate(It.first) ? ss::Money(N->FindTemplate(It.first)->GtdCents).c_str() : "");
			}
		}
	}
	Expect(IgMonth >= 0.10 && IgMonth <= 0.25 && RtMonth >= 0.10 && RtMonth <= 0.25 && RtOpen >= 0.10 && RtOpen <= 0.25, "10-25% of a rival's guaranteed events overlay over a month");
	Expect(IgWeek >= 0.5, "at least half of Ignite's launch week overlays");
	// A rival's field is its template's x its traffic x the day x the draw: Sundays busier, no floor (some fields under the guarantee).
	int Sunday = 0;
	int Monday = 0;
	for (const net::EventInstance& E : Rt.Window(ss::world::DayOn(2026, 12, 6) * SitesDay, ss::world::DayOn(2026, 12, 8) * SitesDay))
	{
		(net::Weekday(net::DayOf(E.Start)) == 6 ? Sunday : Monday) += Rt.TemplateOf(E).OnlyDay == -9999 && Rt.TemplateOf(E).Days == 0x7f ? E.Entries : 0;
	}
	Expect(Sunday > Monday * 1.3, "RunItTwice's Sunday crowd: " + std::to_string(Sunday) + " daily entries against Monday's " + std::to_string(Monday));
}

// ------------------------------------------------------------------ the money

/** The sum of the bank ledger lines added since a ledger of Before lines (newest first). */
Chips SitesLedgerSince(const State& L, size_t Before)
{
	Chips Sum = 0;
	for (size_t I = 0; I + Before < L.Ledger.size(); ++I)
	{
		Sum += L.Ledger[I].Amount;
	}
	return Sum;
}

Chips SitesLines(const site::Account& A)
{
	Chips Sum = 0;
	for (const site::Line& X : A.Lines)
	{
		Sum += X.Cents;
	}
	return Sum;
}

/** The book's lines written and read back into a fresh state (the save's way). */
State SitesReload(const State& L)
{
	std::string Text;
	site::WriteLines(L, Text);
	State Back;
	for (size_t From = 0; From < Text.size();)
	{
		const size_t End = std::min(Text.find('\n', From), Text.size());
		std::vector<std::string> P;
		for (size_t Cell = From, K = From; K <= End; ++K)
		{
			if (K == End || Text[K] == '\t')
			{
				P.emplace_back(Text, Cell, K - Cell);
				Cell = K + 1;
			}
		}
		site::ReadLine(P, Back);
		From = End + 1;
	}
	return Back;
}

void Money()
{
	const double T0 = ss::world::DayOn(2026, 11, 13) * SitesDay + 23.0 * 60.0; // a Friday, 11 PM
	State L;
	Chips Bank = 100000;
	site::Book& B = L.Sites;
	Expect(!site::Deposit(B, L, Bank, net::SiteId::Ignite, 5000, "", T0).empty() && Bank == 100000, "no deposit without an account");
	Expect(!site::Deposit(B, L, Bank, net::SiteId::RiverLine, 5000, "", T0).empty() && site::WalletOf(B, net::SiteId::RiverLine) == nullptr, "RiverLine has no wallet: its money is the bank's");
	Expect(site::Join(B, net::SiteId::Ignite, T0).empty() && site::Join(B, net::SiteId::Ignite, T0).empty() && B.Accounts.size() == 1 && !site::Join(B, net::SiteId::Halcyon, T0).empty(),
		"joining opens one account a site (and none on a site that isn't registered)");
	const size_t Lines0 = L.Ledger.size();
	Expect(!site::Deposit(B, L, Bank, net::SiteId::Ignite, 999, "", T0).empty() && !site::Deposit(B, L, Bank, net::SiteId::Ignite, 5000, "bank", T0).empty() &&
			!site::Deposit(B, L, Bank, net::SiteId::Ignite, 99000, "card", T0).empty() && Bank == 100000 && L.Ledger.size() == Lines0,
		"refused: under the $10 minimum, by a method that doesn't take deposits, more than the bank holds with the fee");
	Expect(site::Deposit(B, L, Bank, net::SiteId::Ignite, 5000, "card", T0).empty(), "a $50 card deposit to Ignite");
	const site::Account* A = site::AccountOf(B, net::SiteId::Ignite);
	Expect(Bank == 100000 - 5150 && L.Ledger.size() == Lines0 + 2 && L.Ledger[1].Amount == -5000 && L.Ledger[1].Kind == 7 && L.Ledger[1].Label == "Deposit: Ignite" &&
			L.Ledger[0].Amount == -150 && L.Ledger[0].Kind == ss::life::LedgerSites && L.Ledger[0].Label == "Ignite card fee",
		"two bank lines: \"Deposit: Ignite\" -$50.00 (transfers) and \"Ignite card fee\" -$1.50 (poker sites)");
	Expect(A && A->Cents == 5000 && A->Deposited == 5000 && A->Lines.size() == 1 && A->Lines[0].Cents == 5000 && A->Lines[0].Label == "Deposit" && A->BonusPending == 5000 &&
			A->BonusExpires == T0 + 60 * SitesDay,
		"the wallet holds $50 with its own line, and $50 of welcome bonus pending for 60 days");
	Expect(B.RiverLine.BoostUntil == T0 + 60 * SitesDay && B.Done.count("retention") && !L.Inbox.empty() && L.Inbox.back().Thread == "riverline" && L.Inbox.back().Subject == "We noticed" &&
			L.Inbox.back().From == "RiverLine Player Relations",
		"RiverLine notices: \"We noticed.\" and Rewards one tier up for 60 days");
	// A withdrawal: out of the wallet now, in transit, then one bank line net of the fee 72 to 120 hours later.
	Expect(!site::Withdraw(B, net::SiteId::Ignite, 1999, "", T0).empty() && !site::Withdraw(B, net::SiteId::Ignite, 5001, "", T0).empty() &&
			!site::Withdraw(B, net::SiteId::Ignite, 3000, "card", T0).empty(),
		"refused: under the $20 minimum, more than the balance, by a method that doesn't pay out");
	const Chips Bank0 = Bank;
	const size_t Lines1 = L.Ledger.size();
	Expect(site::Withdraw(B, net::SiteId::Ignite, 4000, "", T0).empty() && A->Cents == 1000 && B.Transfers.size() == 1 && A->Lines.back().Cents == -4000 &&
			A->Lines.back().Label == "Withdrawal requested ($3.00 fee)" && Bank == Bank0 && L.Ledger.size() == Lines1,
		"a $40 withdrawal leaves the wallet at once; the bank hears nothing yet");
	const site::Transfer X = B.Transfers.front();
	const double Hours = (X.ArrivesAt - X.RequestedAt) / 60.0;
	Expect(X.Out && X.State == 0 && X.Cents == 4000 && X.FeeCents == 300 && Hours >= 72.0 && Hours <= 120.0 && std::floor(X.ArrivesAt) == X.ArrivesAt,
		"in transit: arrives " + SitesNum(Hours) + " hours later, to the minute");
	std::vector<std::pair<std::string, std::string>> Texts;
	const ss::rep::Public P;
	site::Step(B, L, Bank, P, 0, T0, X.ArrivesAt - 1.0, Texts);
	Expect(Bank == Bank0 && B.Transfers.front().State == 0, "a minute before it's due, still in transit");
	// The save in transit, and the same step on both: it lands once in each.
	State Saved = SitesReload(L);
	Chips SavedBank = Bank;
	site::Step(B, L, Bank, P, 0, X.ArrivesAt - 1.0, X.ArrivesAt, Texts);
	Expect(Bank == Bank0 + 3700 && B.Transfers.front().State == 1, "a step that ends on its minute lands it (review: to the minute, not a minute late)");
	site::Step(B, L, Bank, P, 0, X.ArrivesAt, X.ArrivesAt + 3 * SitesDay, Texts);
	site::Step(Saved.Sites, Saved, SavedBank, P, 0, X.ArrivesAt - 1.0, X.ArrivesAt + 2 * SitesDay, Texts);
	Expect(Bank == Bank0 + 3700 && SavedBank == Bank && L.Ledger.front().Amount == 3700 && L.Ledger.front().Kind == 7 && L.Ledger.front().Label == "Withdrawal: Ignite" &&
			L.Ledger.front().At == X.ArrivesAt && B.Transfers.front().State == 1,
		"it lands once as \"Withdrawal: Ignite\" +$37.00 at its minute, and once after a save in transit");
	State After = SitesReload(L);
	Chips AfterBank = Bank;
	site::Step(After.Sites, After, AfterBank, P, 0, X.ArrivesAt + 3 * SitesDay, X.ArrivesAt + 4 * SitesDay, Texts);
	Expect(AfterBank == Bank, "a save after it landed never lands it again");
	// Conservation (§1.2): the bank's change is its ledger's; the wallet's change its lines'; deposits out of the bank are the
	// wallet's credit and the fee; a withdrawal is what landed and its fee.
	Expect(Bank - 100000 == SitesLedgerSince(L, Lines0) && A->Cents == SitesLines(*A) && 5000 + 150 == A->Deposited + 150 && A->Withdrawn == 3700 + 300,
		"conservation to the cent: bank, wallet, deposit and withdrawal");
	// Thirty days after it landed, the transfer's record goes (the day's step); its money never comes back.
	site::Step(B, L, Bank, P, 0, X.ArrivesAt + 31 * SitesDay, X.ArrivesAt + 32 * SitesDay, Texts);
	Expect(B.Transfers.empty() && Bank == Bank0 + 3700, "a landed transfer's record goes after 30 days");
	// RunItTwice: card in at 2.5%; out by bank in exactly 48 hours for $5; the tickets on a first deposit of $50 or more only.
	State Small;
	Chips SmallBank = 100000;
	site::Join(Small.Sites, net::SiteId::RunItTwice, T0);
	site::Deposit(Small.Sites, Small, SmallBank, net::SiteId::RunItTwice, 4999, "", T0);
	site::Deposit(Small.Sites, Small, SmallBank, net::SiteId::RunItTwice, 6000, "", T0);
	Expect(SmallBank == 100000 - 4999 - 125 - 6000 - 150 && Small.Tickets.empty(), "RunItTwice: 2.5% card fees; a first deposit under $50 brings no tickets");
	State Big;
	Chips BigBank = 100000;
	site::Join(Big.Sites, net::SiteId::RunItTwice, T0);
	site::Deposit(Big.Sites, Big, BigBank, net::SiteId::RunItTwice, 5000, "", T0);
	Expect(Big.TicketsFor("runittwice.sunday-grind") == 1 && Big.TicketsFor("runittwice.supper-stack") == 2 &&
			net::ForEvent("runittwice.sunday-grind").TicketOf("runittwice.sunday-grind") == "runittwice.sunday-grind",
		"a first deposit of $50: a $22 Sunday Grind ticket and two $5.50 Supper Stack tickets ($33)");
	site::Withdraw(Big.Sites, net::SiteId::RunItTwice, 2000, "", T0);
	Expect(!Big.Sites.Transfers.empty() && Big.Sites.Transfers.front().ArrivesAt == T0 + 48 * 60 && Big.Sites.Transfers.front().FeeCents == 500, "RunItTwice pays out in 48 hours for $5");
	// (Review.) Ignite's wait is drawn once per request from its id: each request its own wait, all inside 72 to 120 hours, whole
	// minutes, the draw the id's own (so a save and a reload agree on it).
	State Many;
	Chips ManyBank = 1000000;
	site::Join(Many.Sites, net::SiteId::Ignite, T0);
	site::Deposit(Many.Sites, Many, ManyBank, net::SiteId::Ignite, 500000, "", T0);
	double Shortest = 1.0e9;
	double Longest = 0.0;
	bool Drawn = true;
	for (int K = 0; K < 24; ++K)
	{
		site::Withdraw(Many.Sites, net::SiteId::Ignite, 2000, "", T0 + 37.0 * K);
		const site::Transfer& Y = Many.Sites.Transfers.back();
		const double Wait = Y.ArrivesAt - Y.RequestedAt;
		const double Want = std::round((72.0 + 48.0 * static_cast<double>(ss::Fnv1a(Y.Id) % 10001u) / 10000.0) * 60.0);
		Shortest = std::min(Shortest, Wait / 60.0);
		Longest = std::max(Longest, Wait / 60.0);
		Drawn = Drawn && Wait == Want;
	}
	Expect(Drawn && Shortest >= 72.0 && Longest <= 120.0 && Longest - Shortest >= 24.0,
		"Ignite's withdrawals: a wait drawn per request from its id, " + SitesNum(Shortest) + " to " + SitesNum(Longest) + " hours over 24 requests");
	// (Review.) RiverLine notices once: a first deposit on the second rival brings no second "We noticed." and no fresh 60 days.
	State Both;
	Chips BothBank = 100000;
	site::Join(Both.Sites, net::SiteId::Ignite, T0);
	site::Join(Both.Sites, net::SiteId::RunItTwice, T0);
	site::Deposit(Both.Sites, Both, BothBank, net::SiteId::Ignite, 2000, "", T0);
	const double Boost = Both.Sites.RiverLine.BoostUntil;
	site::Deposit(Both.Sites, Both, BothBank, net::SiteId::RunItTwice, 2000, "", T0 + 3 * SitesDay);
	int Noticed = 0;
	for (const ss::life::Letter& M : Both.Inbox)
	{
		Noticed += M.Thread == "riverline" && M.Subject == "We noticed" ? 1 : 0;
	}
	Expect(Noticed == 1 && Both.Sites.RiverLine.BoostUntil == Boost && Boost == T0 + 60 * SitesDay, "RiverLine's \"We noticed.\" comes once, on the first deposit anywhere");
	// The buy-in and prize lines the session's settle writes: the wallet moved in the session, the lines say so.
	State Play;
	Chips PlayBank = 100000;
	site::Join(Play.Sites, net::SiteId::Ignite, T0);
	site::Deposit(Play.Sites, Play, PlayBank, net::SiteId::Ignite, 10000, "", T0);
	site::Account* Pa = site::AccountOf(Play.Sites, net::SiteId::Ignite);
	Pa->Cents -= 550;
	site::OnBuyIn(Play.Sites, net::SiteId::Ignite, 550, 61, "Buy-in: $5.50 Spark Jackpot", T0 + 60);
	site::Season Add;
	Add.Points = 120;
	Add.Won = 18600;
	Add.FinalTables = 1;
	Add.Events = 1;
	Pa->Cents += 18600;
	site::OnPrize(Play.Sites, net::SiteId::Ignite, 18600, "Prize: $5.50 Spark Jackpot", T0 + 300, Add);
	Expect(Pa->Cents == SitesLines(*Pa) && Pa->RakeMonth == 61 && Pa->RakeLife == 61 && Pa->LifetimeWon == 18600 && Pa->LifetimeEvents == 1 && Pa->Hero.FinalTables == 1 &&
			Pa->Hero.Points == 120.0 && Pa->Lines.size() == 3,
		"a rival's buy-in and prize: one line each, the rake and the record counted, the wallet equal to its lines");
}

void RentLine()
{
	const double Deadline = ss::world::DayOn(2026, 11, 28) * SitesDay; // Friday's midnight
	site::Book B;
	site::Account A;
	A.Site = net::SiteId::RunItTwice;
	A.Cents = 6400;
	B.Accounts.push_back(A);
	site::Transfer X;
	X.Id = "ignite:1";
	X.Site = net::SiteId::Ignite;
	X.Out = true;
	X.Method = "bank";
	X.Cents = 20000;
	X.FeeCents = 300;
	X.RequestedAt = Deadline - 26 * 60.0;
	X.ArrivesAt = X.RequestedAt + 96 * 60.0;
	B.Transfers.push_back(X);
	std::string Line;
	const Chips Due = ss::life::MonthlyRentCents;
	Expect(!site::RentWarning(B, 90000, Due, Deadline, Deadline - 72 * 60.0 - 1.0, Line), "the rent line waits until 72 hours before the deadline");
	Expect(site::RentWarning(B, 90000, Due, Deadline, Deadline - 72 * 60.0, Line), "72 hours out, the bank short and the bank and the sites enough: the line shows");
	Expect(Line == "Rent is due Fri. $197.00 is on its way from Ignite (Sun to Tue), and $64.00 sits on RunItTwice. Site money can't pay rent.", "the line: " + Line);
	Expect(!site::RentWarning(B, Due, Due, Deadline, Deadline - 60.0, Line) && !site::RentWarning(B, 50000, Due, Deadline, Deadline - 60.0, Line) &&
			!site::RentWarning(B, 90000, Due, Deadline, Deadline, Line),
		"no line when the bank covers it, when even the sites couldn't, or past the deadline");
}

void Invitations()
{
	const double After = (net::Def(net::SiteId::Ignite).LaunchDay + 3) * SitesDay + 600.0;
	ss::rep::Public P;
	P.OnlineEvents = 149;
	Expect(!site::GateMet(net::SiteId::Ignite, P, 2, After), "Ignite: 149 paid online events, no");
	P.OnlineEvents = 150;
	Expect(site::GateMet(net::SiteId::Ignite, P, 2, After) && !site::GateMet(net::SiteId::Ignite, P, 1, After), "Ignite: 150 and two rents paid, yes; one rent, no (Act I isn't over)");
	Expect(!site::GateMet(net::SiteId::Ignite, P, 2, (net::Def(net::SiteId::Ignite).LaunchDay - 1) * SitesDay + 600.0), "Ignite: never before it opens");
	{
		// A career older than the world's tracker (or a save with no world): its last hundred in History count (C3).
		ss::rep::Public Old;
		Old.Paid100 = 100;
		Expect(!site::GateMet(net::SiteId::Ignite, Old, 2, After), "Ignite: 100 in History and none tracked is still under 150");
		Old.OnlineEvents = 40;
		Old.Paid100 = 150;
		Expect(site::GateMet(net::SiteId::Ignite, Old, 2, After), "Ignite: the gate counts History's paid events when the tracker has fewer");
	}
	P.OnlineEvents = 300;
	P.Abi100 = 1099;
	int Why = -1;
	Expect(!site::GateMet(net::SiteId::RunItTwice, P, 2, After), "RunItTwice: 300 events at a $10.99 average, no");
	P.Abi100 = 1100;
	Expect(site::GateMet(net::SiteId::RunItTwice, P, 2, After, &Why) && Why == 0, "RunItTwice: 300 at $11.00, yes");
	P.OnlineEvents = 299;
	Expect(!site::GateMet(net::SiteId::RunItTwice, P, 2, After), "RunItTwice: 299 at $11.00, no");
	P.OnlineSeriesTitles = 1;
	Expect(site::GateMet(net::SiteId::RunItTwice, P, 2, After, &Why) && Why == 1 && !site::GateMet(net::SiteId::RunItTwice, P, 1, After), "RunItTwice: an online series title, yes (after Act I)");
	P.OnlineSeriesTitles = 0;
	P.Partner = true;
	Expect(site::GateMet(net::SiteId::RunItTwice, P, 2, After, &Why) && Why == 2, "RunItTwice: Kast Partner, yes");
	// (Review.) The record's last-hundred numbers from History (rep::FromHistory): Abi100 averages the paid events only (a
	// freeroll costs nothing), and a series title is a win in a series event on any site; a daily won, or a series event
	// cashed short of first, is no title.
	{
		std::vector<ss::HistoryEntry> Last;
		for (int K = 0; K < 100; ++K)
		{
			ss::HistoryEntry E;
			E.Name = "Somewhere";
			E.EventId = "ignite.spark-jackpot@" + std::to_string(40000 + K);
			E.Place = 50;
			E.BuyInCents = K < 40 ? 0 : K % 2 == 0 ? 1100 : 2200;
			Last.push_back(E);
		}
		Last[0].EventId = "runittwice.open26-12@" + std::to_string(ss::world::DayOn(2026, 11, 18) * 1440 + 900);
		Last[0].Place = 1;
		Last[1].EventId = "runittwice.open26-13@" + std::to_string(ss::world::DayOn(2026, 11, 18) * 1440 + 1020);
		Last[1].Place = 2;
		Last[2].Place = 1; // a daily won
		ss::rep::Public Q100;
		ss::rep::FromHistory(Q100, Last);
		Expect(Q100.Abi100 == 1650 && Q100.OnlineSeriesTitles == 1,
			"the gates' record from History: a $16.50 average of the 60 paid (" + ss::Money(Q100.Abi100) + "), one series title (" + std::to_string(Q100.OnlineSeriesTitles) + ")");
	}
	// The step mails it the first day the gate is met, once; a decline brings one more letter 30 days later, and no third.
	State L;
	L.RentsPaid = 2;
	Chips Bank = 0;
	std::vector<std::pair<std::string, std::string>> Texts;
	ss::rep::Public Q;
	Q.OnlineEvents = 150;
	site::Step(L.Sites, L, Bank, Q, 0, After, After + 60.0, Texts);
	Expect(L.Inbox.empty(), "the gates are checked once a day, over a midnight");
	site::Step(L.Sites, L, Bank, Q, 0, After + 60.0, After + SitesDay, Texts);
	auto Letters = [&L](const char* Thread, int Kind) {
		int N = 0;
		for (const ss::life::Letter& M : L.Inbox)
		{
			N += M.Source == "site" && M.Thread == Thread && M.Kind == Kind ? 1 : 0;
		}
		return N;
	};
	Expect(Letters("ignite", site::LetterInvite) == 1 && Letters("runittwice", site::LetterInvite) == 0 && L.Inbox.back().From == "Bree Okafor, Ignite Partnerships" &&
			ss::life::WantsAnswer(L.Inbox.back()),
		"Ignite's invitation, from Bree Okafor, wanting an answer");
	site::Step(L.Sites, L, Bank, Q, 0, After + SitesDay, After + 3 * SitesDay, Texts);
	Expect(Letters("ignite", site::LetterInvite) == 1, "once");
	L.Sites.Invites.front().State = 2; // declined (Session::AnswerInvite)
	L.Sites.Invites.front().At = After + 3 * SitesDay;
	site::Step(L.Sites, L, Bank, Q, 0, After + 3 * SitesDay, After + 32 * SitesDay, Texts);
	Expect(Letters("ignite", site::LetterInvite) == 1, "a decline: nothing for 30 days");
	site::Step(L.Sites, L, Bank, Q, 0, After + 32 * SitesDay, After + 34 * SitesDay, Texts);
	Expect(Letters("ignite", site::LetterInvite) == 2 && L.Sites.Invites.front().State == 3, "30 days after a decline, the second letter");
	L.Sites.Invites.front().State = 2;
	site::Step(L.Sites, L, Bank, Q, 0, After + 34 * SitesDay, After + 90 * SitesDay, Texts);
	Expect(Letters("ignite", site::LetterInvite) == 2, "and no third");
	// Accepted: the account, and the welcome letter at the next step; no invitation for a site already joined.
	State J;
	J.RentsPaid = 2;
	site::Join(J.Sites, net::SiteId::Ignite, After);
	site::Step(J.Sites, J, Bank, Q, 0, After, After, Texts);
	Expect(J.Inbox.size() == 1 && J.Inbox.front().Thread == "ignite" && J.Inbox.front().Kind == site::LetterRoutine && J.Inbox.front().Subject == "You're in",
		"an account opened: Ignite's welcome letter at once");
	// (Review.) One routine letter a week a company (§1.7): Ignite has written this week for seven days, and nobody else has.
	Expect(site::WroteThisWeek(J, net::SiteId::Ignite, After + 6.9 * SitesDay) && !site::WroteThisWeek(J, net::SiteId::Ignite, After + 7.1 * SitesDay) &&
			!site::WroteThisWeek(J, net::SiteId::RunItTwice, After + SitesDay),
		"a routine letter holds that company's next one for a week, no other's");
	site::Step(J.Sites, J, Bank, Q, 0, After, After + 2 * SitesDay, Texts);
	Expect(J.Inbox.size() == 1, "and no invitation from a site already joined");
}

void Promotions()
{
	const double T0 = ss::world::DayOn(2026, 11, 10) * SitesDay + 600.0;
	// Ignite's welcome: $5 into the wallet for each $20 of rake inside 60 days, never past the match; what's left expires.
	State L;
	Chips Bank = 100000;
	site::Join(L.Sites, net::SiteId::Ignite, T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::Ignite, 5000, "", T0);
	site::Account* A = site::AccountOf(L.Sites, net::SiteId::Ignite);
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 0, 1999, "Buy-in", T0 + 10);
	Expect(A->BonusReleased == 0 && A->Cents == 5000, "$19.99 of rake releases nothing");
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 0, 1, "Buy-in", T0 + 20);
	Expect(A->BonusReleased == 500 && A->BonusPending == 4500 && A->Cents == 5500 && A->Lines.back().Label == "Welcome bonus released" && A->Lines.back().Cents == 500,
		"$20.00 of rake releases $5 into the wallet, with its line");
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 0, 30000, "Buy-in", T0 + 30);
	Expect(A->BonusReleased == 5000 && A->BonusPending == 0 && A->Cents == 10000 && A->Cents == SitesLines(*A) + 0, "never more than the match ($50)");
	State E;
	Chips EBank = 100000;
	site::Join(E.Sites, net::SiteId::Ignite, T0);
	site::Deposit(E.Sites, E, EBank, net::SiteId::Ignite, 20000, "", T0);
	site::Account* Ea = site::AccountOf(E.Sites, net::SiteId::Ignite);
	Expect(Ea->BonusPending == 10000, "a $200 first deposit is matched up to $100");
	site::OnBuyIn(E.Sites, net::SiteId::Ignite, 0, 4000, "Buy-in", T0 + 60);
	std::vector<std::pair<std::string, std::string>> Texts;
	const ss::rep::Public P;
	site::Step(E.Sites, E, EBank, P, 0, T0 + 60, T0 + 60 * SitesDay + 1.0, Texts);
	site::OnBuyIn(E.Sites, net::SiteId::Ignite, 0, 4000, "Buy-in", T0 + 61 * SitesDay);
	Expect(Ea->BonusReleased == 1000 && Ea->BonusPending == 0, "after 60 days what's left ($90) expires, and rake releases nothing more");
	// The Second Run Club on the 1st, into RunItTwice's wallet: $49.99 nothing, $50 15%, $250 20%, $1,000 25%, $4,000 30%.
	const Chips Rakes[] = {4999, 5000, 25000, 100000, 400000};
	const Chips Pays[] = {0, 750, 5000, 25000, 120000};
	for (size_t K = 0; K < 5; ++K)
	{
		State R;
		Chips RBank = 0;
		site::Join(R.Sites, net::SiteId::RunItTwice, T0);
		site::Account* Ra = site::AccountOf(R.Sites, net::SiteId::RunItTwice);
		site::OnBuyIn(R.Sites, net::SiteId::RunItTwice, 0, Rakes[K], "Buy-in", T0);
		const Chips Before = Ra->Cents;
		const double First = site::NextMonthStart(T0);
		site::Step(R.Sites, R, RBank, P, 0, T0, First + 60.0, Texts);
		site::Step(R.Sites, R, RBank, P, 0, First + 60.0, First + 10 * SitesDay, Texts);
		Expect(Ra->Cents - Before == Pays[K] && RBank == 0 && Ra->RakeMonth == 0 && Ra->MonthKey == "2026-12" && (Pays[K] == 0 || Ra->Lines.back().Label == "Second Run Club: November") &&
				Ra->Vip == static_cast<int>(K),
			"Second Run Club: " + ss::Money(Rakes[K]) + " of November's rake pays " + ss::Money(Pays[K]) + " on Dec 1, once, into the wallet");
	}
	// RiverLine Rewards on the 1st, into the bank as a prize (kind 4): $39.99 Bronze nothing; $40 Silver 8%; Gold 14%; Platinum
	// 20%; Diamond 27%; and one tier up while the retention letter's boost runs.
	const Chips Rl[] = {3999, 4000, 20000, 80000, 300000};
	const Chips RlPay[] = {0, 320, 2800, 16000, 81000};
	for (size_t K = 0; K < 5; ++K)
	{
		for (const bool Boost : {false, true})
		{
			State R;
			Chips RBank = 0;
			R.Sites.RiverLine.BoostUntil = Boost ? T0 + 60 * SitesDay : 0.0;
			site::OnBuyIn(R.Sites, net::SiteId::RiverLine, 0, Rl[K], "Buy-in", T0);
			const double First = site::NextMonthStart(T0);
			const State Saved = SitesReload(R);
			site::Step(R.Sites, R, RBank, P, 0, T0, First + 60.0, Texts);
			State Again = Saved;
			Chips AgainBank = 0;
			site::Step(Again.Sites, Again, AgainBank, P, 0, T0, First + 2 * SitesDay, Texts);
			site::Step(R.Sites, R, RBank, P, 0, First + 60.0, First + 3 * SitesDay, Texts);
			const std::vector<site::TierInfo>& Tiers = site::RewardsTiers();
			const int Tier = std::min(4, static_cast<int>(K) + (Boost ? 1 : 0));
			const Chips Want = Boost ? static_cast<Chips>(std::llround(static_cast<double>(Rl[K]) * Tiers[static_cast<size_t>(Tier)].Permille / 1000.0)) : RlPay[K];
			const bool Line = Want == 0 || (!R.Ledger.empty() && R.Ledger.front().Kind == 4 && R.Ledger.front().Amount == Want &&
									   R.Ledger.front().Label == std::string("RiverLine Rewards: ") + Tiers[static_cast<size_t>(Tier)].Name + ", November");
			Expect(RBank == Want && AgainBank == Want && Line && R.Sites.RiverLine.Rake == 0 && R.Sites.RiverLine.Tier == Tier,
				std::string("RiverLine Rewards: ") + ss::Money(Rl[K]) + (Boost ? " with the boost" : "") + " pays " + ss::Money(RBank) + " on Dec 1, once (and once after a save)");
			if (K == 1 && !Boost)
			{
				// Its key is kept: the same month handed back can't be paid twice.
				R.Sites.RiverLine.MonthKey = "2026-11";
				R.Sites.RiverLine.Rake = Rl[K];
				const Chips Paid = RBank;
				site::Step(R.Sites, R, RBank, P, 0, First - 60.0, First + 60.0, Texts);
				Expect(RBank == Paid && R.Sites.Done.size() >= 1, "November's Rewards are paid once, by their key, even handed back");
			}
		}
	}
	// The month's rake is every RiverLine buy-in's fee; a rival's rake never counts toward it.
	State M;
	site::Join(M.Sites, net::SiteId::Ignite, T0);
	site::OnBuyIn(M.Sites, net::SiteId::RiverLine, 550, 50, "Buy-in", T0);
	site::OnBuyIn(M.Sites, net::SiteId::Ignite, 550, 61, "Buy-in", T0);
	Expect(M.Sites.RiverLine.Rake == 50 && M.Sites.RiverLine.MonthKey == "2026-11", "Rewards count RiverLine's rake only, by month");
}

/**
 * (C3, the audit's should-fix 2.) A letter that carries money always goes, even in a week its company has already written
 * (C2's N5 rule for Rerun's statements): RunItTwice's "Your tickets are in" minutes after its welcome, the first Second Run
 * statement and RiverLine's first Rewards statement inside the week of "We noticed". A letter with nothing in it still waits.
 */
void PaidLetters()
{
	const double T0 = ss::world::DayOn(2026, 11, 26) * SitesDay + 600.0; // Thursday, Nov 26: Dec 1 is five days on
	const ss::rep::Public P;
	std::vector<std::pair<std::string, std::string>> Texts;
	const auto Has = [](const State& L, const char* Thread, const std::string& Subject) {
		int N = 0;
		for (const ss::life::Letter& M : L.Inbox)
		{
			N += M.Source == "site" && M.Thread == Thread && M.Subject == Subject ? 1 : 0;
		}
		return N;
	};
	State R;
	Chips Bank = 100000;
	site::Join(R.Sites, net::SiteId::RunItTwice, T0);
	site::Step(R.Sites, R, Bank, P, 0, T0, T0, Texts); // the welcome, at once
	Expect(site::Deposit(R.Sites, R, Bank, net::SiteId::RunItTwice, 5000, "", T0 + 5.0).empty() && R.TicketsFor("runittwice.sunday-grind") == 1 &&
			R.TicketsFor("runittwice.supper-stack") == 2,
		"RunItTwice's welcome tickets on a $50 first deposit");
	Expect(Has(R, "runittwice", "Your RunItTwice account is open") == 1 && Has(R, "runittwice", "Your tickets are in") == 1,
		"\"Your tickets are in\" arrives with the tickets, five minutes after the welcome letter");
	Expect(Has(R, "riverline", "We noticed") == 1, "RiverLine notices the deposit");
	// November's rake: $60 on RunItTwice (Member, 15%) and $40 on RiverLine (Silver, a tier up with the boost: Gold, 14%).
	site::OnBuyIn(R.Sites, net::SiteId::RunItTwice, 0, 6000, "Buy-in", T0 + 60.0);
	site::OnBuyIn(R.Sites, net::SiteId::RiverLine, 0, 4000, "Buy-in", T0 + 60.0);
	const Chips Before = Bank;
	site::Step(R.Sites, R, Bank, P, 0, T0 + 60.0, site::NextMonthStart(T0) + 60.0, Texts);
	Expect(Bank == Before + 560 && Has(R, "riverline", "RiverLine Rewards: Gold, November") == 1,
		"RiverLine's first Rewards statement goes out with its $5.60, though \"We noticed\" came that week");
	Expect(site::AccountOf(R.Sites, net::SiteId::RunItTwice)->Lines.back().Cents == 900 && Has(R, "runittwice", "Second Run Club: November") == 1,
		"the first Second Run statement goes out with its $9.00, though RunItTwice wrote twice that week");
	// Nothing in it, it waits: Ignite's bonus-end letter in the week of its welcome.
	State I;
	Chips IBank = 100000;
	site::Join(I.Sites, net::SiteId::Ignite, T0);
	site::Step(I.Sites, I, IBank, P, 0, T0, T0, Texts);
	site::Deposit(I.Sites, I, IBank, net::SiteId::Ignite, 5000, "", T0 + 5.0);
	site::Account* Ia = site::AccountOf(I.Sites, net::SiteId::Ignite);
	Ia->BonusExpires = T0 + 2 * SitesDay;
	site::Step(I.Sites, I, IBank, P, 0, T0 + 5.0, T0 + 3 * SitesDay, Texts);
	Expect(Ia->BonusPending == 0 && Has(I, "ignite", "Your welcome bonus has ended") == 0 && Has(I, "ignite", "You're in") == 1,
		"a letter with no money in it still keeps to one a week (the bonus's end, two days after the welcome)");
}

void HeroBySite()
{
	// RiverLine's boards never move for a rival's result (bug 7): StatsFrom reads RiverLine's events only.
	const double Now = ss::world::DayOn(2026, 11, 20) * SitesDay + 6.0 * 60.0;
	std::vector<ss::HistoryEntry> History;
	for (const net::EventInstance& E : net::Shared().Window(Now - 600.0, Now - 300.0))
	{
		if (net::Shared().TemplateOf(E).BuyInCents > 0 && net::Shared().TemplateOf(E).BuyInCents <= 550)
		{
			ss::HistoryEntry H;
			H.Name = net::Shared().TemplateOf(E).Name;
			H.EventId = E.Id;
			H.Place = 3;
			H.Entrants = E.Entries;
			H.Prize = 4000;
			H.BuyInCents = net::Shared().TemplateOf(E).BuyInCents;
			History.push_back(H);
			break;
		}
	}
	std::vector<ss::HistoryEntry> With = History;
	for (const net::EventInstance& E : net::Site(net::SiteId::Ignite).Window(Now - 600.0, Now - 200.0))
	{
		ss::HistoryEntry H;
		H.Name = net::Site(net::SiteId::Ignite).TemplateOf(E).Name;
		H.EventId = E.Id;
		H.Place = 1;
		H.Entrants = E.Entries;
		H.Prize = 25000;
		H.BuyInCents = net::Site(net::SiteId::Ignite).TemplateOf(E).BuyInCents;
		With.insert(With.begin(), H);
		break;
	}
	const net::HeroStats A = net::StatsFrom("grinder_3c", History, Now);
	const net::HeroStats B = net::StatsFrom("grinder_3c", With, Now);
	Expect(With.size() == 2 && A.Earnings == B.Earnings && A.Wins == B.Wins && A.FinalTables == B.FinalTables && A.SeasonPoints == B.SeasonPoints && A.NightPoints == B.NightPoints &&
			A.Tournaments == B.Tournaments && A.SeasonWins == B.SeasonWins && A.Cashes == B.Cashes && A.Best == B.Best && A.SeriesTitles == B.SeriesTitles,
		"an Ignite win moves none of the player's RiverLine numbers (Earnings, Season, wins, Night Shift points)");
	site::Book Bk;
	site::Join(Bk, net::SiteId::Ignite, Now - SitesDay);
	site::Account* Acc = site::AccountOf(Bk, net::SiteId::Ignite);
	Acc->Hero.Year = 2026;
	Acc->Hero.Points = 80.0;
	Acc->Hero.Wins = 1;
	Acc->LifetimeWon = 25000;
	const net::HeroStats Ig = site::HeroOn(net::SiteId::Ignite, "grinder_3c", With, Bk, nullptr, Now);
	const net::HeroStats Rl = site::HeroOn(net::SiteId::RiverLine, "grinder_3c", With, Bk, nullptr, Now);
	Expect(Ig.Wins == 1 && Ig.SeasonPoints == 80.0 && Ig.SeasonWins == 1 && Ig.Earnings == 25000 && Ig.SiteBoard > 0.0 && Rl.Earnings == A.Earnings && Rl.Wins == 0,
		"site::HeroOn: Ignite's boards read the account and Ignite's results (this month's points " + SitesNum(Ig.SiteBoard) + "); RiverLine's, StatsFrom's");
	// (Review.) Ignite's Month board is this month's: in December, November's win counts nothing there.
	const net::HeroStats InDecember = site::HeroOn(net::SiteId::Ignite, "grinder_3c", With, Bk, nullptr, ss::world::DayOn(2026, 12, 3) * SitesDay + 600.0);
	Expect(InDecember.SiteBoard == 0.0 && InDecember.Wins == 1, "Ignite's Month board: November's points are gone in December (its win still counts)");
	// (Review.) Past the last hundred (History is full), RiverLine's lifetime numbers come from the world's tracker, which follows
	// the player everywhere, less every rival account's (bug 7): Earnings less LifetimeWon, Tournaments less LifetimeEvents,
	// Season the world's RiverLine-only HeroPoints. (C3: the hundred here are out of the money, no prize and no points, so the
	// tracker's numbers are the larger and the subtraction shows.)
	ss::world::World W;
	W.Create(92u, 1440.0 + 127.0);
	W.GrantHeroResults(160);
	ss::HistoryEntry OutOfTheMoney = History.front();
	OutOfTheMoney.Place = std::max(2, OutOfTheMoney.Entrants);
	OutOfTheMoney.Prize = 0;
	const std::vector<ss::HistoryEntry> Full(100, OutOfTheMoney);
	site::Book Two;
	site::Join(Two, net::SiteId::Ignite, Now - SitesDay);
	site::Join(Two, net::SiteId::RunItTwice, Now - SitesDay);
	site::AccountOf(Two, net::SiteId::Ignite)->LifetimeWon = 2500;
	site::AccountOf(Two, net::SiteId::Ignite)->LifetimeEvents = 4;
	site::AccountOf(Two, net::SiteId::RunItTwice)->LifetimeWon = 1500;
	site::AccountOf(Two, net::SiteId::RunItTwice)->LifetimeEvents = 3;
	const ss::world::Tracker& Tr = W.HeroStats();
	const ss::world::TrackLine& Lv = Tr.ByStake[static_cast<size_t>(ss::world::TrackStake::Live)];
	const net::HeroStats Whole = site::HeroOn(net::SiteId::RiverLine, "grinder_3c", Full, site::Book(), &W, Now);
	const net::HeroStats Less = site::HeroOn(net::SiteId::RiverLine, "grinder_3c", Full, Two, &W, Now);
	const net::HeroStats Short = site::HeroOn(net::SiteId::RiverLine, "grinder_3c", History, Two, &W, Now);
	Expect(Tr.Prizes - Lv.Prizes > 4000 && Whole.Earnings == Tr.Prizes - Lv.Prizes && Less.Earnings == Whole.Earnings - 4000 && Whole.Tournaments == Tr.Events - Lv.Events &&
			Less.Tournaments == Whole.Tournaments - 7 && Whole.SeasonPoints == W.HeroSeasonPoints() && Short.Earnings == A.Earnings && Short.Tournaments == A.Tournaments,
		"RiverLine's lifetime row from a full History: the tracker's " + ss::Money(Whole.Earnings) + " less the rivals' $40.00 (" + ss::Money(Less.Earnings) +
			"); a History short of a hundred reads StatsFrom");
	// (C3, the audit's should-fix 1.) Never below what History shows. The tracker only exists since abeb60b and nothing back-fills
	// it, so a career older than it (or a save that kept no world: the creator fixture) has a tracker short of its own last
	// hundred, and the YOU row fell to $0, 0 tournaments and 0 points. Each number is the larger of the two.
	{
		ss::world::World Before;
		Before.Create(92u, 1440.0 + 127.0); // the tracker knows nothing of the player
		const std::vector<ss::HistoryEntry> Cashed(100, History.front()); // a hundred $40 cashes on RiverLine
		const net::HeroStats Hist = net::StatsFrom("grinder_3c", Cashed, Now);
		const net::HeroStats Pre = site::HeroOn(net::SiteId::RiverLine, "grinder_3c", Cashed, Two, &Before, Now);
		Expect(Hist.Earnings == 400000 && Hist.Tournaments == 100 && Hist.SeasonPoints > 0.0 && Pre.Earnings == Hist.Earnings && Pre.Tournaments == Hist.Tournaments &&
				Pre.SeasonPoints == Hist.SeasonPoints,
			"a career older than the tracker: RiverLine's row is its last hundred's (" + ss::Money(Pre.Earnings) + ", " + std::to_string(Pre.Tournaments) + " events, " +
				SitesNum(Pre.SeasonPoints) + " points), never $0");
		// A tracker that began partway through: each number on its own, the larger of the two.
		const net::HeroStats Mixed = site::HeroOn(net::SiteId::RiverLine, "grinder_3c", Cashed, Two, &W, Now);
		Expect(Mixed.Earnings == std::max<Chips>(Hist.Earnings, Tr.Prizes - Lv.Prizes - 4000) && Mixed.Tournaments == std::max(Hist.Tournaments, Tr.Events - Lv.Events - 7) &&
				Mixed.SeasonPoints == std::max(Hist.SeasonPoints, W.HeroSeasonPoints()) && Mixed.Tournaments > Hist.Tournaments,
			"a tracker that began partway: Earnings " + ss::Money(Mixed.Earnings) + " (History's " + ss::Money(Hist.Earnings) + "), " + std::to_string(Mixed.Tournaments) +
				" events (the tracker's), each the larger");
	}
}

void RivalNews()
{
	const net::Network& Ig = net::Site(net::SiteId::Ignite);
	const net::Network& Rt = net::Site(net::SiteId::RunItTwice);
	const int Launch = net::Def(net::SiteId::Ignite).LaunchDay;
	net::HeroStats Hero;
	Hero.Name = "grinder_3c";
	Hero.Wins = 2;
	Hero.FinalTables = 4;
	Expect(Ig.News(Launch * SitesDay - 1.0, Hero, 40).empty(), "Ignite's News is empty before it opens (no history before launch)");
	const std::vector<net::NewsItem> Week = Ig.News((Launch + 6) * SitesDay + 23 * 60.0, Hero, 60);
	bool Opened = false;
	bool Promo = false;
	bool Overlay = false;
	bool Quiet = true;
	// (C3.) A headline's money is in whole dollars ("left the $67,000 guarantee $1,890 short"), never "$1,889.50".
	const auto Cents = [](const std::string& Text) {
		for (size_t At = Text.find('$'); At != std::string::npos; At = Text.find('$', At + 1))
		{
			size_t K = At + 1;
			while (K < Text.size() && ((Text[K] >= '0' && Text[K] <= '9') || Text[K] == ','))
			{
				++K;
			}
			if (K > At + 1 && K + 1 < Text.size() && Text[K] == '.' && Text[K + 1] >= '0' && Text[K + 1] <= '9')
			{
				return true;
			}
		}
		return false;
	};
	bool Whole = true;
	bool Figure = true;
	std::string OverlayLine;
	for (const net::NewsItem& It : Week)
	{
		Opened = Opened || (It.Title == "Ignite Poker is live" && It.Body.find("$25K Sunday Blaze") != std::string::npos && It.Tag == "LAUNCH");
		Promo = Promo || It.Tag == "PROMO";
		Overlay = Overlay || (It.Tag == "OVERLAY" && It.Title.find("overlays by") != std::string::npos && It.Body.find("Ignite covered the difference.") != std::string::npos);
		// (The event's own name keeps its buy-in, "$5.50 Spark Jackpot"; the money it's short is what's counted.)
		const size_t By = It.Title.find(" overlays by ");
		Whole = Whole && (It.Tag != "OVERLAY" || (By != std::string::npos && !Cents(It.Title.substr(By)) && !Cents(It.Body)));
		// (review-c3-fixes) The figure the item carries (the news card's big number, the ticker's "(...)") is the headline's
		// whole dollars, never "$983.61" beside "overlays by $984".
		Figure = Figure && (It.Tag != "OVERLAY" || (By != std::string::npos && It.Amount > 0 && It.Amount % 100 == 0 &&
								   It.Title.compare(By, std::string::npos, " overlays by $" + ss::Grouped(It.Amount / 100)) == 0));
		OverlayLine = It.Tag == "OVERLAY" && OverlayLine.empty() ? It.Title + ": " + It.Body + " [" + ss::Money(It.Amount) + "]" : OverlayLine;
		Quiet = Quiet && It.Tag != "RCOP 2026" && It.Tag != "SCHEDULE" && It.Tag != "RECORD" && It.Tag != "YOU" && It.At >= Launch * SitesDay;
	}
	Expect(Opened && Promo, "Ignite's News opens with its launch (\"a $25K Sunday Blaze\") and its welcome");
	Expect(Overlay, "Ignite's News reports the overlays it paid");
	Expect(Overlay && Whole, "an overlay's headline counts whole dollars (" + OverlayLine + ")");
	Expect(Overlay && Figure, "an overlay's figure beside the headline is the same whole dollars (" + OverlayLine + ")");
	Expect(Quiet, "nothing of RiverLine's on Ignite's page, nothing before it opened");
	bool Lobby = false;
	for (const net::NewsItem& It : Rt.News(net::NightOneDay * SitesDay + 127.0, Hero, 20))
	{
		Lobby = Lobby || (It.Title == "RunItTwice opens its new lobby" && It.Tag == "NEW LOBBY");
	}
	Expect(Lobby, "RunItTwice's News on Night One: its new lobby");
	bool Coming = false;
	for (const net::NewsItem& It : Rt.News(ss::world::DayOn(2026, 11, 8) * SitesDay + 12 * 60.0, Hero, 60))
	{
		Coming = Coming || (It.Kind == net::NewsKind::Series && It.Title == "RunItTwice Open 2026 starts Nov 13");
	}
	Expect(Coming, "a week out, the RunItTwice Open is announced");
}

/**
 * (Review.) The world's items go to the News of the site they name (§5.7, §5.10): a living world past Ignite's launch, each
 * site's page against the world's headlines of the last days by their site flags. None crosses over.
 */
void RivalNewsIsTheirs()
{
	ss::world::World W;
	W.Create(93u, 1440.0 + 127.0);
	net::AttachAll(&W);
	const double Now = ss::world::DayOn(2026, 11, 12) * SitesDay + 12.0 * 60.0;
	W.AdvanceTo(Now);
	std::map<int, std::set<std::string>> Titles;
	for (const ss::world::WorldEvent& E : W.Events())
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		if (E.At > Now - 4.0 * SitesDay && E.At <= Now && W.Headline(E, Title, Body, Tag))
		{
			Titles[static_cast<int>(ss::world::FlagSiteOf(E.Flags))].insert(Title);
		}
	}
	net::HeroStats Hero;
	Hero.Name = "grinder_3c";
	int Crossed = 0;
	int Own = 0;
	for (const net::SiteId S : {net::SiteId::RiverLine, net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		for (const net::NewsItem& It : net::Site(S).News(Now, Hero, 80))
		{
			for (const auto& By : Titles)
			{
				const bool Mine = By.first == static_cast<int>(S);
				Crossed += !Mine && By.second.count(It.Title) > 0 ? 1 : 0;
				Own += Mine && S != net::SiteId::RiverLine && By.second.count(It.Title) > 0 ? 1 : 0;
			}
		}
	}
	net::AttachAll(nullptr);
	const size_t Rival = Titles[static_cast<int>(net::SiteId::Ignite)].size() + Titles[static_cast<int>(net::SiteId::RunItTwice)].size();
	std::printf("rival news: %zu RiverLine and %zu rival world headlines in the last four days; %d on their own rival page, %d crossed\n",
		Titles[static_cast<int>(net::SiteId::RiverLine)].size(), Rival, Own, Crossed);
	Expect(Crossed == 0 && Rival > 0 && !Titles[static_cast<int>(net::SiteId::RiverLine)].empty(),
		"every site's News carries only the world's items that name it (" + std::to_string(Rival) + " rival headlines, " + std::to_string(Own) + " on their page)");
}

/** Every rival's window, live state and listing over a few days: what the desk's worker reads. */
uint64_t SitesRead(int From, int To)
{
	uint64_t H = 1469598103934665603ull;
	for (net::SiteId S : SitesRivals)
	{
		const net::Network& N = net::Site(S);
		for (const net::EventInstance& E : N.Window(From * SitesDay, To * SitesDay))
		{
			const net::LiveState L = N.Live(E, From * SitesDay + 600.0);
			H = SitesFnv(H, E.Id + "|" + std::to_string(E.Entries) + "|" + std::to_string(E.Pool) + "|" + std::to_string(L.Entries) + "|" + std::to_string(static_cast<int>(L.St)));
		}
	}
	return H;
}

void Threading()
{
	// The contract (§5.8): the registry is built and prewarmed on the game thread before the desk's worker runs; the worker's
	// world reads only const paths. A world loaded on a worker reads the rivals exactly as the game thread does.
	ss::world::World W;
	W.Create(91u, 1440.0 + 127.0);
	net::AttachAll(&W);
	const int Day = net::NightOneDay;
	const uint64_t Main = SitesRead(Day, Day + 3);
	std::string Text;
	W.Write(Text);
	uint64_t Worker = 0;
	double Launched = -2.0;
	std::thread T([&]() {
		ss::world::World Desk;
		SitesReadWorld(Text, Desk);
		Launched = Desk.SiteLaunched(net::SiteId::Ignite);
		Worker = SitesRead(Day, Day + 3);
	});
	const uint64_t Again = SitesRead(Day, Day + 3);
	T.join();
	net::AttachAll(nullptr);
	Expect(Worker == Main && Again == Main && Launched == W.SiteLaunched(net::SiteId::Ignite), "a desk-worker world reads the rivals identically while the game thread does");
}

void SaveLines()
{
	const double T0 = ss::world::DayOn(2026, 11, 13) * SitesDay + 600.0;
	State L;
	Chips Bank = 100000;
	Expect([&L]() {
		std::string S;
		site::WriteLines(L, S);
		return S.empty();
	}(),
		"an empty book writes nothing (an old save round-trips byte for byte)");
	site::Join(L.Sites, net::SiteId::Ignite, T0);
	site::Join(L.Sites, net::SiteId::RunItTwice, T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::Ignite, 7777, "", T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::RunItTwice, 5000, "", T0);
	site::Withdraw(L.Sites, net::SiteId::RunItTwice, 2500, "", T0 + 1.0 / 3.0);
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 550, 61, "Buy-in: tab\tand \\ backslash", T0 + 2);
	site::OnBuyIn(L.Sites, net::SiteId::RiverLine, 1100, 100, "Buy-in", T0 + 3);
	L.Sites.Invites.push_back({net::SiteId::Ignite, T0 - 0.125, 1});
	std::string A;
	site::WriteLines(L, A);
	const State Back = SitesReload(L);
	std::string B;
	site::WriteLines(Back, B);
	Expect(!A.empty() && A == B && Back.Sites.Accounts.size() == 2 && Back.Sites.Transfers.size() == 1 && Back.Sites.Accounts[0].Lines.back().Label == "Buy-in: tab\tand \\ backslash",
		"the site lines round-trip exactly (" + std::to_string(A.size()) + " bytes)");
	// A whole save: Serialize(Parse(S)) == S with the site lines in it.
	ss::SaveData D;
	D.Life = L;
	D.BankrollCents = Bank;
	const std::string S1 = D.Serialize();
	ss::SaveData E;
	Expect(ss::SaveData::Parse(S1, E) && E.Serialize() == S1 && S1.find("site\tacct\t1\t") != std::string::npos && S1.find("site\trewards\t2026-11\t100\t") != std::string::npos,
		"a save with an account, a transfer, an invitation and Rewards reads back and writes back byte for byte");
	// A new career sets every bit and holds nothing: no site line (RiverLine-only careers' saves are as they were). A month
	// closed with nothing still keeps its key, so a rewards line once written always reads back the same.
	State Fresh;
	Fresh.Update2LinesRead = ~0u;
	std::string None;
	site::WriteLines(Fresh, None);
	State R;
	std::vector<std::string> Line = {"site", "rewards", "2026-12", "0", "0", "0"};
	site::ReadLine(Line, R);
	std::string Out;
	site::WriteLines(R, Out);
	Expect(None.empty() && Out == "site\trewards\t2026-12\t0\t0\t0\n", "a new career writes no site line; a rewards line with only its month reads back the same");
}

void SitesMarks()
{
	const double T0 = ss::world::DayOn(2026, 11, 6) * SitesDay + 23 * 60.0;
	State L;
	Chips Bank = 100000;
	site::Join(L.Sites, net::SiteId::Ignite, T0);
	site::Join(L.Sites, net::SiteId::RunItTwice, T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::Ignite, 20000, "", T0);
	site::Withdraw(L.Sites, net::SiteId::Ignite, 20000, "", T0);
	site::OnBuyIn(L.Sites, net::SiteId::RiverLine, 550, 50, "Buy-in", T0);
	const std::vector<ss::life::Mark> M = site::Marks(L.Sites, T0, T0 + 70 * SitesDay);
	std::map<int, std::string> Seen;
	for (const ss::life::Mark& K : M)
	{
		Seen[K.Kind] = Seen[K.Kind].empty() ? K.Label : Seen[K.Kind];
	}
	Expect(Seen[site::MarkTransfer] == "Ignite withdrawal $197.00 arrives" && Seen[site::MarkRewards] == "RiverLine Rewards" && Seen[site::MarkBonus] == "Ignite welcome bonus ends" &&
			Seen[site::MarkSeries] == "RunItTwice Open 2026 starts",
		"the marks: a withdrawal arriving, Rewards day, the bonus's end, a joined rival's series");
	bool Ordered = std::is_sorted(M.begin(), M.end(), [](const ss::life::Mark& A, const ss::life::Mark& B) { return A.At < B.At; });
	Expect(Ordered && site::Marks(L.Sites, T0 + 200 * SitesDay, T0 + 201 * SitesDay).empty(), "in time order, and none where nothing falls");
}

void StepBudget()
{
	// §9: VideoStep + SiteStep over a 30-day skip <= 2 ms. A busy book: two accounts, a pending withdrawal each, Rewards, invites.
	const double T0 = ss::world::DayOn(2026, 11, 20) * SitesDay + 600.0;
	State L;
	L.RentsPaid = 2;
	Chips Bank = 500000;
	site::Join(L.Sites, net::SiteId::Ignite, T0);
	site::Join(L.Sites, net::SiteId::RunItTwice, T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::Ignite, 30000, "", T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::RunItTwice, 30000, "", T0);
	site::Withdraw(L.Sites, net::SiteId::Ignite, 5000, "", T0);
	site::Withdraw(L.Sites, net::SiteId::RunItTwice, 5000, "", T0);
	for (int K = 0; K < 40; ++K)
	{
		site::OnBuyIn(L.Sites, net::SiteId::RunItTwice, 1100, 88, "Buy-in", T0 + K);
		site::OnBuyIn(L.Sites, net::SiteId::RiverLine, 550, 50, "Buy-in", T0 + K);
	}
	ss::rep::Public P;
	P.OnlineEvents = 400;
	std::vector<std::pair<std::string, std::string>> Texts;
	State First = L;
	Chips FirstBank = Bank;
	const auto C0 = std::chrono::steady_clock::now();
	site::Step(First.Sites, First, FirstBank, P, 0, T0, T0 + 30 * SitesDay, Texts);
	const double Ms = SitesSeconds(C0) * 1000.0;
	State Daily = L;
	Chips DailyBank = Bank;
	const auto C1 = std::chrono::steady_clock::now();
	for (int D = 0; D < 30; ++D)
	{
		site::Step(Daily.Sites, Daily, DailyBank, P, 0, T0 + D * SitesDay, T0 + (D + 1) * SitesDay, Texts);
	}
	const double DailyMs = SitesSeconds(C1) * 1000.0;
	std::printf("SiteStep: a 30-day skip %.3f ms; 30 daily steps %.3f ms\n", Ms, DailyMs);
	Expect(Ms <= 2.0, "SiteStep over a 30-day skip takes " + SitesNum(Ms) + " ms (2 ms or less)");
	Expect(FirstBank == DailyBank && First.Ledger.size() == Daily.Ledger.size(), "a 30-day skip lands what 30 daily steps do, to the cent");
}

// ------------------------------------------------------------------ the soft population's bound (§5.3)

/**
 * Smart pace's auto-fold, as the style harness plays its grader heroes (styles_main.cpp, SmartFolds): junk facing a price
 * folds without a decision, as the session would have folded it.
 */
bool SitesSmartFolds(const ss::PlayerView& V)
{
	if (V.CurrentStreet != ss::Street::Preflop || V.CanCheck || V.Hole.size() != 2)
	{
		return false;
	}
	const double Price = static_cast<double>(V.ToCall) / static_cast<double>(std::max<Chips>(1, V.Pot + V.ToCall));
	if (Price < 0.25)
	{
		return false;
	}
	const double Pct = ss::HandPercentile(V.Hole[0], V.Hole[1]);
	const ss::StreetSummary Sum = ss::PreflopSummary(V);
	const ss::PublicSeat& Me = V.Seats[static_cast<size_t>(V.Me)];
	const double StackBb = static_cast<double>(Me.Stack + Me.StreetBet) / static_cast<double>(std::max<Chips>(1, V.BigBlind));
	if (StackBb <= 15.0 && Sum.Raises == 0)
	{
		return Pct > ss::PushRange(StackBb, ss::PlayersBehind(V, V.Me)) * 1.4;
	}
	if (Sum.Raises == 0)
	{
		static const double Open[9] = {0.55, 0.5, 1.0, 0.16, 0.18, 0.2, 0.24, 0.28, 0.36};
		return Pct > Open[static_cast<int>(ss::PositionOf(V, V.Me))];
	}
	return V.Me == V.BbIdx ? Pct > 0.5 : Pct > 0.22;
}

/** The grader's best option (the coach's best play). */
ss::PlayerAction SitesGraderBest(const ss::PlayerView& V, ss::Rng& GradeRng)
{
	const ss::DecisionAnalysis A = ss::AnalyzeDecision(V, GradeRng, 1200);
	if (A.Options.empty())
	{
		return V.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
	}
	size_t Best = 0;
	for (size_t I = 1; I < A.Options.size(); ++I)
	{
		Best = A.Options[I].Ev > A.Options[Best].Ev ? I : Best;
	}
	return A.Options[Best].Action;
}

/** One 9-player sit-and-go at a site's rake (50/30/20 of what's left), the grader-best hero against Pop: the hero's prize. */
Chips SitesSng(const std::string& Pop, const std::string& Seed, Chips BuyIn, Chips Fee)
{
	ss::TournamentSpec S;
	S.Id = "sites-sng";
	S.Name = "Sit and go";
	S.BuyInCents = BuyIn;
	S.FeeCents = Fee;
	S.Entrants = 9;
	S.StartingStack = 5000;
	S.LevelMinutes = 5.0;
	S.SecondsPerHand = 42.0;
	S.Population = Pop;
	S.Speed = "Turbo";
	S.TableSize = 9;
	ss::Tournament T(S, "Hero", Seed);
	const Chips Pool = 9 * (BuyIn - Fee);
	const Chips First = static_cast<Chips>(std::llround(0.5 * static_cast<double>(Pool)));
	const Chips Second = static_cast<Chips>(std::llround(0.3 * static_cast<double>(Pool)));
	T.Payouts = {First, Second, Pool - First - Second};
	ss::Rng GradeRng(Seed + ":grade");
	for (int Guard = 0; !T.bFinished && !T.Hero().Busted && Guard < 20000; ++Guard)
	{
		std::vector<ss::TEvent> Events;
		std::unique_ptr<ss::Hand> H = T.StartTick(Events);
		if (H)
		{
			for (int Steps = 0; !H->bComplete && Steps < 1000; ++Steps)
			{
				const int Idx = H->ToAct;
				ss::PlayerAction Act;
				if (H->Seats[static_cast<size_t>(Idx)].Id == ss::HeroId)
				{
					const ss::PlayerView V = ss::MakeView(*H, Idx);
					Act = T.HeroSitsOut || SitesSmartFolds(V) ? (V.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold()) : SitesGraderBest(V, GradeRng);
				}
				else
				{
					Act = T.BotDecisionFor(*H, true).Action;
				}
				if (!H->Act(Act))
				{
					const ss::LegalActions L = H->GetLegalActions();
					H->Act(L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold());
				}
			}
		}
		T.FinishTick(H.get(), nullptr);
	}
	return T.Hero().PrizeCents;
}

/**
 * The soft population is bounded (§5.3): the grader-best hero's after-rake ROI on Ignite (the "soft" tables, 11% rake) is at
 * most 1.5x RiverLine's (micro tables, FeeOf) at the same buy-in, $5.50. Sit-and-gos on the same seeds; Count of each.
 */
void SoftBound(int Count)
{
	const auto C0 = std::chrono::steady_clock::now();
	const Chips BuyIn = 550;
	const Chips IgFee = net::Def(net::SiteId::Ignite).Rake.Fee(BuyIn);
	const Chips RlFee = net::FeeOf(BuyIn);
	// Every sit-and-go stands alone (its own seed, tournament and grader), so three threads split them; the sums are taken in
	// seed order, so the result is the same however they split.
	std::vector<double> Soft(static_cast<size_t>(Count), 0.0);
	std::vector<double> Micro(static_cast<size_t>(Count), 0.0);
	const int Threads = std::max(1, std::min(3, Count));
	std::vector<std::thread> Pool;
	for (int Th = 0; Th < Threads; ++Th)
	{
		Pool.emplace_back([&, Th]() {
			for (int K = Th; K < Count; K += Threads)
			{
				const std::string Seed = "sites-soft:" + std::to_string(K);
				Soft[static_cast<size_t>(K)] = static_cast<double>(SitesSng("soft", Seed, BuyIn, IgFee)) / static_cast<double>(BuyIn) - 1.0;
				Micro[static_cast<size_t>(K)] = static_cast<double>(SitesSng("micro", Seed, BuyIn, RlFee)) / static_cast<double>(BuyIn) - 1.0;
			}
		});
	}
	for (std::thread& T : Pool)
	{
		T.join();
	}
	double SoftRoi = 0.0;
	double MicroRoi = 0.0;
	for (int K = 0; K < Count; ++K)
	{
		SoftRoi += Soft[static_cast<size_t>(K)];
		MicroRoi += Micro[static_cast<size_t>(K)];
	}
	SoftRoi /= std::max(1, Count);
	MicroRoi /= std::max(1, Count);
	std::printf("soft bound: the grader-best hero's ROI over %d $5.50 sit-and-gos: Ignite %+.1f%% (soft, 11%% rake), RiverLine %+.1f%% (micro, FeeOf), ratio %.2f; %.1fs\n",
		Count, 100.0 * SoftRoi, 100.0 * MicroRoi, MicroRoi > 0.0 ? SoftRoi / MicroRoi : 0.0, SitesSeconds(C0));
	const double Soft0 = SoftRoi;
	const double Micro0 = MicroRoi;
	Expect(Micro0 > 0.0 && Soft0 <= 1.5 * Micro0,
		"the soft population's bound: Ignite's after-rake ROI " + SitesNum(100.0 * Soft0) + "% is at most 1.5x RiverLine's " + SitesNum(100.0 * Micro0) + "%");
}

void SitesCopy()
{
	std::map<std::string, std::string> Vars = copylint::Samples();
	Vars.insert({{"Site", "Ignite"}, {"Month", "November"}, {"Tier", "Gold"}, {"Rate", "14%"}, {"Rake", "$212.40"}, {"Released", "$35.00"},
		{"Left", "$65.00"}, {"Min", "$10.00"}, {"Fee", "$1.50"}, {"Why", "A series title gets noticed around here."}, {"Money", "$197.00 is on its way from Ignite (Mon to Wed)."},
		{"Gtd", "$25K"}, {"Entries", "410"}, {"Events", "40"}, {"Days", "17"}, {"Series", "RunItTwice Open 2026"}, {"Buyin", "$530"}, {"Window", "Mon to Wed"}, {"Method", "card"}});
	std::vector<std::pair<std::string, std::string>> Sentences;
	for (const std::string& S : site::Inventory())
	{
		Sentences.emplace_back("sites", S);
	}
	int Problems = copylint::Run("sites", Sentences, copylint::LetterBody, Vars);
	// Titles, subjects and labels: no stop at the end, otherwise the same rules (filled, no doubled space, nothing banned).
	for (const std::string& S : site::CopyLabels())
	{
		const std::string Why = copylint::Check(S + ".", copylint::MailSubject, Vars);
		if (!Why.empty())
		{
			++Problems;
			std::printf("COPY (sites labels) \"%s\": %s\n", S.c_str(), Why.c_str());
		}
	}
	Expect(Problems == 0 && site::Inventory().size() >= 40, "every line the sites write passes the copy lint (" + std::to_string(site::Inventory().size()) + " sentences)");
	// In-world copy never says AI, bot or autoplay (U1).
	bool Silent = true;
	for (const std::string& S : site::Inventory())
	{
		std::string Word;
		for (const char Ch : copylint::Lower(S) + " ")
		{
			if (Ch >= 'a' && Ch <= 'z')
			{
				Word.push_back(Ch);
				continue;
			}
			Silent = Silent && Word != "ai" && Word != "bot" && Word != "bots" && Word != "autoplay";
			Word.clear();
		}
	}
	Expect(Silent, "the sites' copy never mentions AI, bots or autoplay");
	// The trademark screen over the sites, their reps, series, events, promotions and methods: no real site, wallet, platform,
	// format or product (tests/Trademarks.h, both tiers, whole words), none of the ten closed sites the world already tells of
	// and no drink brand (the screen's own list, matched anywhere as before).
	const char* Real[] = {"northpot", "bluffharbor", "aceline", "chipcity", "felt republic", "pocketrocket", "riverbend", "saltwater poker", "cardroom 88", "tiltproof",
		"monster", "red bull", "rockstar"};
	std::vector<std::string> Names;
	for (net::SiteId S : SitesRivals)
	{
		const net::SiteDef& D = net::Def(S);
		Names.insert(Names.end(), {D.Name, D.Tagline, D.Rep, D.RepTitle, D.TeamName});
		for (const net::CashMethod& M : D.Cashier)
		{
			Names.push_back(M.Name);
		}
		for (const net::EventTemplate& T : net::Site(S).Templates())
		{
			Names.push_back(T.Name + " " + T.Blurb);
		}
		for (const net::SeriesInfo& Sr : net::Site(S).Series())
		{
			Names.push_back(Sr.Name + " " + Sr.Tagline);
		}
	}
	for (const site::TierInfo& T : site::SecondRunTiers())
	{
		Names.push_back(T.Name);
	}
	Names.push_back("Ignite Poker Second Run Club RiverLine Rewards");
	std::string Hit;
	for (const std::string& N : Names)
	{
		for (const char* R : Real)
		{
			Hit += copylint::Lower(N).find(R) != std::string::npos ? std::string(R) + " in \"" + N + "\" " : std::string();
		}
		for (const char* R : {trademarks::Find(N, trademarks::kSourceScan), trademarks::Find(N, trademarks::kCopyScan)})
		{
			Hit += R ? std::string(R) + " in \"" + N + "\" " : std::string();
		}
	}
	Expect(Hit.empty(), "the trademark screen passes over the sites, reps, series, events and promotions" + (Hit.empty() ? std::string() : ": " + Hit));
}

/** The string literals in a source file (comments and character literals skipped, escapes read), with their lines. */
std::vector<std::pair<int, std::string>> SitesLiterals(const std::string& Src)
{
	std::vector<std::pair<int, std::string>> Out;
	int Line = 1;
	const size_t N = Src.size();
	for (size_t I = 0; I < N; ++I)
	{
		const char C = Src[I];
		if (C == '\n')
		{
			++Line;
		}
		else if (C == '/' && I + 1 < N && Src[I + 1] == '/')
		{
			while (I + 1 < N && Src[I + 1] != '\n')
			{
				++I;
			}
		}
		else if (C == '/' && I + 1 < N && Src[I + 1] == '*')
		{
			for (I += 2; I + 1 < N && !(Src[I] == '*' && Src[I + 1] == '/'); ++I)
			{
				Line += Src[I] == '\n' ? 1 : 0;
			}
			++I;
		}
		else if (C == '\'')
		{
			for (++I; I < N && Src[I] != '\'' && Src[I] != '\n'; ++I)
			{
				I += Src[I] == '\\' ? 1 : 0;
			}
		}
		else if (C == '"')
		{
			std::string S;
			for (++I; I < N && Src[I] != '"' && Src[I] != '\n'; ++I)
			{
				if (Src[I] == '\\' && I + 1 < N)
				{
					++I;
				}
				S += Src[I];
			}
			Out.emplace_back(Line, S);
		}
	}
	return Out;
}

/**
 * (C3, the lead's decision.) No real site's format name on any screen. "Flip & Go" is GGPoker's, and RiverLine already showed it
 * (its format label, the lobby's tag, the lock line), so every player-facing name is "Flip & Run" now: Ignite's Flip & Run and
 * Flip & Run Bounty, RiverLine's label and lock line, Bree's letter and Ignite's launch news. The ids ("flip-go", Format::Flip)
 * stay, as saves and tickets key on them. Every string literal in the core and the game module is screened, and the sites' copy.
 * Update 3 (§1.10, Phase A): the screen is tests/Trademarks.h's kSourceScan, matched on whole words: every real site, platform,
 * wallet, format, series, product, tool and creator that is no ordinary word. Two real events' names were on RiverLine
 * ("Bounty Builder", "Monster Stack"): Bounty Ladder and Mammoth Stack now, their ids kept.
 */
void SitesFormatNames()
{
	std::vector<std::pair<std::string, std::string>> Said;
	for (const std::string& S : site::Inventory())
	{
		Said.emplace_back("sites' copy", S);
	}
	for (const std::string& S : site::CopyLabels())
	{
		Said.emplace_back("sites' labels", S);
	}
	for (const net::SiteDef& D : net::Sites())
	{
		for (const net::EventTemplate& T : net::Site(D.Id).Templates())
		{
			Said.emplace_back(D.Key + " " + T.Id, T.Name + " " + T.Blurb);
		}
	}
	std::string Lock;
	for (const net::EventInstance& E : net::Shared().Window(5 * SitesDay, 6 * SitesDay))
	{
		std::string Why;
		Lock = net::Shared().TemplateOf(E).Fmt == net::Format::Flip && !net::Shared().Joinable(E, &Why, 0) ? Why : Lock;
	}
	Said.emplace_back("RiverLine's lock line", Lock);
	namespace fs = std::filesystem;
	const fs::path Roots[] = {fs::path(SS_TESTS_DIR) / ".." / "Source", fs::path(SS_TESTS_DIR) / ".." / ".." / ".." / "Source"};
	int Files = 0;
	for (const fs::path& Root : Roots)
	{
		std::error_code Ec;
		for (fs::recursive_directory_iterator It(Root, Ec), End; !Ec && It != End; It.increment(Ec))
		{
			const std::string Ext = It->path().extension().string();
			if (!It->is_regular_file() || (Ext != ".cpp" && Ext != ".h"))
			{
				continue;
			}
			std::FILE* F = std::fopen(It->path().string().c_str(), "rb");
			if (!F)
			{
				continue;
			}
			std::string Src;
			char Buf[65536];
			for (size_t Got = 0; (Got = std::fread(Buf, 1, sizeof(Buf), F)) > 0;)
			{
				Src.append(Buf, Got);
			}
			std::fclose(F);
			++Files;
			for (const std::pair<int, std::string>& L : SitesLiterals(Src))
			{
				Said.emplace_back(It->path().filename().string() + ":" + std::to_string(L.first), L.second);
			}
		}
	}
	std::string Hit;
	for (const std::pair<std::string, std::string>& S : Said)
	{
		const char* R = trademarks::Grandfathered(S.second) ? nullptr : trademarks::Find(S.second, trademarks::kSourceScan);
		Hit += R ? std::string(R) + " at " + S.first + " (\"" + S.second + "\") " : std::string();
	}
	std::printf("format names: %zu strings screened, from %d source files\n", Said.size(), Files);
	Expect(Files >= 200 && Lock == "Flip & Run unlocks later in your career." && Hit.empty(),
		"no real site's format name anywhere a player reads (Flip & Run, never GGPoker's)" + (Hit.empty() ? std::string() : ": " + Hit));
}

// ================================================================== Update 3 G1 (core-sites-look): site::show and the marks
//
// SPEC v1.2 §2.3, §3.3-§3.6, §7.3, §7.7: Ignite's hour, Overlay Watch, the Club's live accrual, the Second Board (spike S-D,
// durable/update3/S-D.md), the Desk, Sunday Live, the welcome bonus's view, their copy, and siteart's new marks with
// RunItTwice's six emblem families in red and black.

namespace show = ss::site::show;

/** The world minute of a calendar date at a clock hour. */
double U3At(int Year, int Month, int Day, double Hour)
{
	return static_cast<double>(ss::world::DayOn(Year, Month, Day)) * SitesDay + Hour * 60.0;
}

/** A drawn colour as 0xRRGGBB (rounded): exact palette checks on vertices and text. */
uint32_t U3Rgb(const ui::Color& C)
{
	auto Byte = [](float V) { return static_cast<uint32_t>(std::lround(std::min(1.0f, std::max(0.0f, V)) * 255.0f)); };
	return Byte(C.R) << 16 | Byte(C.G) << 8 | Byte(C.B);
}

/** Ignite's hour (§2.3): Afterburner Hours 00:00-06:00, then Wake-Up, Daytime, Prime Time, on any day. */
void U3Moods()
{
	struct U3MoodCase
	{
		double Hour;
		show::Mood Want;
	};
	const U3MoodCase Cases[] = {{0.0, show::Mood::Afterburner}, {2.0 + 7.0 / 60.0, show::Mood::Afterburner}, {5.99, show::Mood::Afterburner},
		{6.0, show::Mood::WakeUp}, {11.99, show::Mood::WakeUp}, {12.0, show::Mood::Daytime}, {17.99, show::Mood::Daytime}, {18.0, show::Mood::PrimeTime},
		{21.0, show::Mood::PrimeTime}, {23.99, show::Mood::PrimeTime}};
	int Wrong = 0;
	for (const double Day : {1.0, static_cast<double>(ss::world::DayOn(2026, 11, 14)), static_cast<double>(ss::world::DayOn(2027, 2, 3))})
	{
		for (const U3MoodCase& K : Cases)
		{
			Wrong += show::IgniteMood(Day * SitesDay + K.Hour * 60.0) == K.Want ? 0 : 1;
		}
	}
	Expect(Wrong == 0, "Ignite's mood: Afterburner 00:00-06:00, Wake-Up to noon, Daytime to 6 PM, Prime Time to midnight, every day");
	Expect(show::IgniteMood(SitesDay + 127.0) == show::Mood::Afterburner && show::IgniteMood(U3At(2026, 11, 14, 21.0)) == show::Mood::PrimeTime,
		"Night One's 2:07 AM is Afterburner Hours (the first visit, IG1); 9 PM is Prime Time (IG2)");
}

/** Overlay Watch (§3.6 R1): the projection by hand, the actual overlay once closed, and every guaranteed event of a week. */
void U3Overlay()
{
	const net::Network& N = net::Site(net::SiteId::RunItTwice);
	const net::SiteDef& D = net::Def(net::SiteId::RunItTwice);
	const double From = U3At(2026, 11, 16, 0.0); // a Monday
	const std::vector<net::EventInstance> Week = N.Window(From, From + 7.0 * SitesDay);
	const net::EventInstance* Cap = nullptr;
	for (const net::EventInstance& E : Week)
	{
		Cap = !Cap && E.Id.rfind("runittwice.the-nightcap@", 0) == 0 ? &E : Cap;
	}
	Expect(Cap != nullptr, "the Nightcap is on the Board every night");
	if (!Cap)
	{
		return;
	}
	const net::EventTemplate& T = N.TemplateOf(*Cap);
	// By hand: $109 at 7% is $7.63 of fee, so each entry puts $101.37 in the pool. At the start the median registration curve
	// stands at one half (the field doubles in late registration): 12 registered project 24.
	const Chips Each = 10137;
	Expect(T.BuyInCents == 10900 && D.Rake.Fee(10900) == 763 && T.GtdCents > 24 * Each, "the Nightcap: $109, $7.63 of it fee, guaranteed past 24 entries' worth");
	bool Actual = true;
	const Chips AtStart = show::ProjectedOverlay(N, *Cap, 12, Cap->Start, Actual);
	std::printf("overlay watch: the Nightcap guarantees %s; 12 in at the start project 24 and %s short\n", ss::Money(T.GtdCents).c_str(), ss::Money(AtStart).c_str());
	Expect(!Actual && AtStart == T.GtdCents - 24 * Each && show::LateRegMultiplier(N, *Cap, Cap->Start) == 2.0,
		"projected overlay at the start: the guarantee less 12 x 2 x $101.37 (PROJ.)");
	// Halfway through late registration the curve stands at 0.5 + 0.5 x 0.5^0.7 = 0.807786 (x 1.2379515): 20 in project 24.76.
	const double Half = Cap->Start + 0.5 * Cap->LateReg;
	const double Mult = 1.0 / (0.5 + 0.5 * std::pow(0.5, 0.7));
	const Chips AtHalf = show::ProjectedOverlay(N, *Cap, 20, Half, Actual);
	std::printf("overlay watch: halfway through late registration x%.7f (the curve's %.7f), 20 in leave %s short (by hand %s)\n", Mult,
		show::LateRegMultiplier(N, *Cap, Half), ss::Money(AtHalf).c_str(),
		ss::Money(std::max<Chips>(0, T.GtdCents - static_cast<Chips>(std::llround(20.0 * Mult * 10137.0)))).c_str());
	Expect(!Actual && std::fabs(Mult - 1.2379515) < 1e-7 && std::fabs(show::LateRegMultiplier(N, *Cap, Half) - Mult) < 1e-9 &&
			AtHalf == std::max<Chips>(0, T.GtdCents - static_cast<Chips>(std::llround(20.0 * Mult * 10137.0))),
		"projected overlay halfway through late registration: the guarantee less 20 x 1.2379515 x $101.37");
	// Before registration opens, or with nobody in: nothing to project from. The projection never reads the final field.
	const double Opens = Cap->Start - static_cast<double>(T.OpensHours) * 60.0;
	const Chips Early = show::ProjectedOverlay(N, *Cap, 12, Opens - 1.0, Actual);
	Expect(Early == 0 && !Actual && show::ProjectedOverlay(N, *Cap, 0, Cap->Start - 30.0, Actual) == 0 && show::LateRegMultiplier(N, *Cap, Opens - 1.0) == 0.0,
		"no projection before registration opens or while nobody is registered");
	net::EventInstance Other = *Cap;
	Other.Entries = Cap->Entries * 3 + 7;
	Other.Pool = Cap->Pool * 3;
	Expect(show::ProjectedOverlay(N, Other, 12, Cap->Start, Actual) == AtStart && show::ProjectedOverlay(N, Other, 20, Half, Actual) == AtHalf,
		"the projection reads the registrations so far, never the event's final field");
	// Closed: the actual overlay, the guarantee less what the final field put in (the network's own pool).
	const Chips Done = show::ProjectedOverlay(N, *Cap, 0, Cap->Start + Cap->LateReg + 1.0, Actual);
	Expect(Actual && Done == std::max<Chips>(0, T.GtdCents - Each * static_cast<Chips>(Cap->Entries)), "after registration closes: the actual overlay");
	// Every guaranteed event of the week on both rivals: the actual overlay is the network's; the amber rule; the projection is
	// the curve's median (within the instance's wobble at the start and halfway through late registration; centred over all).
	int Checked = 0;
	int Off = 0;
	int Short = 0;
	int Amber = 0;
	std::vector<double> Ratios;
	for (const net::SiteId S : SitesRivals)
	{
		const net::Network& Nw = net::Site(S);
		for (const net::EventInstance& E : Nw.Window(From, From + 7.0 * SitesDay))
		{
			const net::EventTemplate& Te = Nw.TemplateOf(E);
			if (Te.GtdCents <= 0 || Te.BuyInCents <= 0)
			{
				continue;
			}
			++Checked;
			const Chips Put = static_cast<Chips>(std::llround(static_cast<double>(Te.BuyInCents) * net::Def(S).Rake.PrizeShare(Te)));
			const show::OverlayView Close = show::OverlayOf(Nw, E, E.Start + E.LateReg + 1.0);
			Off += Close.Actual && Close.Shown && Close.Cents == std::max<Chips>(0, Te.GtdCents - Put * static_cast<Chips>(E.Entries)) &&
					(Te.JoinIndex >= 0 || Close.Cents == E.Pool - Put * static_cast<Chips>(E.Entries)) && Close.Entries == static_cast<double>(E.Entries)
				? 0
				: 1;
			Off += Close.Amber == (Close.Cents >= 10000 && Close.Cents * 20 >= Te.GtdCents) ? 0 : 1;
			Short += Close.Cents > 0 ? 1 : 0;
			Amber += Close.Amber ? 1 : 0;
			for (const double At : {E.Start, E.Start + 0.5 * E.LateReg})
			{
				const show::OverlayView V = show::OverlayOf(Nw, E, At);
				if (!V.Shown || V.Actual)
				{
					++Off;
					continue;
				}
				Off += std::fabs(V.Entries - static_cast<double>(E.Entries)) <= 0.1 * static_cast<double>(E.Entries) + 2.0 ? 0 : 1;
				Off += V.Registered == Nw.Live(E, At).Entries ? 0 : 1;
				Ratios.push_back(V.Entries / static_cast<double>(std::max(1, E.Entries)));
			}
			Off += show::OverlayOf(Nw, E, E.Start - static_cast<double>(Te.OpensHours) * 60.0 - 1.0).Shown ? 1 : 0;
		}
	}
	std::sort(Ratios.begin(), Ratios.end());
	const double Median = Ratios.empty() ? 0.0 : Ratios[Ratios.size() / 2];
	std::printf("overlay watch: %d guaranteed rival events in a week, %d closed short (%d amber); projected over final field, median %.3f\n", Checked, Short, Amber, Median);
	Expect(Checked > 50 && Off == 0 && Short > 0 && Median > 0.97 && Median < 1.03,
		"every guaranteed rival event: the actual overlay once closed, amber at $100 and 5%, the projection unbiased within the curve's wobble");
}

/** The Club's live accrual (§3.6 R5): every tier edge, the spec's worked card, and the same cents the 1st pays. */
void U3Club()
{
	const Chips Rakes[] = {0, 4999, 5000, 21240, 24999, 25000, 99999, 100000, 399999, 400000, 612345};
	const int Tiers[] = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 4};
	const int Nexts[] = {1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 4};
	const Chips Accrued[] = {0, 0, 750, 3186, 3750, 5000, 20000, 25000, 100000, 120000, 183704};
	const Chips ToNext[] = {5000, 1, 20000, 3760, 1, 75000, 1, 300000, 1, 0, 0};
	int Wrong = 0;
	for (size_t K = 0; K < 11; ++K)
	{
		site::Account A;
		A.Site = net::SiteId::RunItTwice;
		A.RakeMonth = Rakes[K];
		const show::Accrual X = show::ClubAccrual(A);
		const bool Ok = X.Tier == Tiers[K] && X.Next == Nexts[K] && X.Rake == Rakes[K] && X.Accrued == Accrued[K] && X.ToNext == ToNext[K];
		Wrong += Ok ? 0 : 1;
		if (!Ok)
		{
			std::printf("  club accrual at %s: tier %d next %d accrued %s to next %s\n", ss::Money(Rakes[K]).c_str(), X.Tier, X.Next, ss::Money(X.Accrued).c_str(),
				ss::Money(X.ToNext).c_str());
		}
	}
	Expect(Wrong == 0, "the Club's accrual at every tier edge ($49.99, $50, $249.99, $250, $999.99, $1,000, $3,999.99, $4,000; Inner Circle at the top)");
	site::Account Card;
	Card.Site = net::SiteId::RunItTwice;
	Card.RakeMonth = 21240;
	const show::Accrual W = show::ClubAccrual(Card);
	Expect(W.Tier == 1 && ss::Money(W.Accrued) == "$31.86" && ss::Money(W.ToNext) == "$37.60" && W.Next == 2,
		"the spec's card: $212.40 of rake is ACCRUED AT MEMBER (15%) $31.86, $37.60 MORE RAKE TO REGULAR (20%)");
	// The same cents the 1st pays (SiteCloseMonth, pinned by Promotions): the accrual is the payout to come.
	const double T0 = U3At(2026, 11, 10, 10.0);
	const ss::rep::Public P;
	std::vector<std::pair<std::string, std::string>> Texts;
	int Unequal = 0;
	for (const Chips Rake : {Chips(4999), Chips(5000), Chips(21240), Chips(25000), Chips(100000), Chips(400000)})
	{
		State R;
		Chips RBank = 0;
		site::Join(R.Sites, net::SiteId::RunItTwice, T0);
		site::Account* Ra = site::AccountOf(R.Sites, net::SiteId::RunItTwice);
		site::OnBuyIn(R.Sites, net::SiteId::RunItTwice, 0, Rake, "Buy-in", T0);
		const Chips Due = show::ClubAccrual(*Ra).Accrued;
		const Chips Before = Ra->Cents;
		const double First = site::NextMonthStart(T0);
		site::Step(R.Sites, R, RBank, P, 0, T0, First + 60.0, Texts);
		Unequal += Ra->Cents - Before == Due ? 0 : 1;
	}
	Expect(Unequal == 0, "what the card accrues is exactly what the Second Run Club pays on the 1st");
}

/** A finished hand's whole state as text: a read must leave every bit of it as it was. */
std::string U3HandText(const ss::Hand& H)
{
	std::string Out = std::to_string(H.DeckPosForShow()) + "|" + std::to_string(H.Events.size()) + "|" + std::to_string(static_cast<int>(H.CurrentStreet)) + "|";
	for (const ss::Card C : H.Board)
	{
		Out += ss::CardToString(C);
	}
	for (const ss::HandSeat& S : H.Seats)
	{
		Out += "|" + S.Id + ":" + std::to_string(S.Stack) + ":" + std::to_string(S.Committed) + (S.Folded ? "f" : "") + (S.AllIn ? "a" : "");
		for (const ss::Card C : S.Hole)
		{
			Out += ss::CardToString(C);
		}
	}
	for (const ss::PotResult& Pr : H.PotResults)
	{
		Out += "|" + std::to_string(Pr.Amount);
		for (size_t K = 0; K < Pr.Winners.size() && K < Pr.Shares.size(); ++K)
		{
			Out += "w" + std::to_string(Pr.Winners[K]) + ":" + std::to_string(Pr.Shares[K]);
		}
	}
	Out += "|";
	for (const ss::Card C : H.DeckForShow())
	{
		Out += ss::CardToString(C);
	}
	return Out;
}

std::string U3Board(const show::SecondBoard& B)
{
	std::string Out;
	for (const int C : B.Cards)
	{
		Out += ss::CardToString(C);
	}
	return Out;
}

/** A 52-card deck with Top on top, the rest after it in order. */
std::vector<ss::Card> U3Deck(const std::string& Top)
{
	std::vector<ss::Card> Deck = ss::ParseCards(Top);
	for (const ss::Card C : ss::FullDeck())
	{
		if (std::find(Deck.begin(), Deck.end(), C) == Deck.end())
		{
			Deck.push_back(C);
		}
	}
	return Deck;
}

int U3SeatOf(const ss::Hand& H, const std::string& Id)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == Id)
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

/** One of spike S-D's crafted hands: a fixed deck, scripted actions, and the second board and verdict worked by hand. */
struct U3SdCase
{
	std::string Name;
	ss::HandConfig Cfg;
	std::vector<ss::PlayerAction> Acts;
	bool Shown = true;
	int Shared = 0;
	std::string Cards; // the second board, all five
	int Second = 0, Verdict = 0;
	bool FromStub = true;
	bool HeroIn = true;
};

ss::HandConfig U3HeadsUp(Chips Hero, Chips Villain, const std::string& Top)
{
	ss::HandConfig Cfg;
	Cfg.Players = {{0, "hero", Hero}, {1, "v", Villain}};
	Cfg.ButtonSeat = 0; // heads-up: the button (hero) posts the small blind and acts first preflop
	Cfg.SmallBlind = 50;
	Cfg.BigBlind = 100;
	Cfg.Deck = U3Deck(Top);
	return Cfg;
}

/** Plays a case, reads it: the read changes nothing, repeats, reads the same from a copy, and matches the hand-worked answer. */
int U3RunSdCase(const U3SdCase& K)
{
	int Bad = 0;
	ss::Rng R("u3-sd:" + K.Name);
	ss::Hand H(K.Cfg, R);
	for (const ss::PlayerAction& A : K.Acts)
	{
		Bad += H.Act(A) ? 0 : 1;
	}
	Bad += H.bComplete ? 0 : 1;
	uint32_t Before[4];
	uint32_t After[4];
	R.GetState(Before);
	const std::string Text = U3HandText(H);
	const int Me = U3SeatOf(H, "hero");
	show::SecondBoard B;
	show::SecondBoard Again;
	show::SecondBoard Copied;
	const bool Shown = show::SecondRun(H, Me, "u3#" + K.Name, B);
	const bool Shown2 = show::SecondRun(H, Me, "u3#" + K.Name, Again);
	const ss::Hand Copy(H);
	const bool Shown3 = show::SecondRun(Copy, Me, "u3#" + K.Name, Copied);
	R.GetState(After);
	Bad += std::equal(Before, Before + 4, After) && U3HandText(H) == Text ? 0 : 1;
	Bad += Shown == Shown2 && Shown == Shown3 && U3Board(B) == U3Board(Again) && U3Board(B) == U3Board(Copied) && B.Verdict == Copied.Verdict && B.Second == Copied.Second ? 0 : 1;
	Bad += Shown == K.Shown ? 0 : 1;
	if (Shown && K.Shown)
	{
		Bad += B.Shared == K.Shared && B.FromStub == K.FromStub && B.HeroIn == K.HeroIn ? 0 : 1;
		Bad += K.Cards.empty() || U3Board(B) == K.Cards ? 0 : 1;
		Bad += !K.HeroIn || K.Cards.empty() || (B.Second == K.Second && B.Verdict == K.Verdict) ? 0 : 1;
		// The verdict is the pair's: the real run as the engine paid it (First) with the second (fix secondboard).
		Bad += !B.HeroIn || B.Verdict == show::VerdictOf(B.First, B.Second) ? 0 : 1;
		// The Luck Ledger's equity and the Second Board see the same all-in (the hand's first Reveal).
		Bad += (ss::autoplay::AllInEquity(H, Me) >= 0.0) == B.HeroIn ? 0 : 1;
	}
	if (Bad > 0)
	{
		std::printf("  second board \"%s\": shown %d, board %s, shared %d, second %d, verdict %d, %s, %s\n", K.Name.c_str(), Shown ? 1 : 0, U3Board(B).c_str(), B.Shared,
			B.Second, B.Verdict, B.FromStub ? "stub" : "keyed", B.HeroIn ? "in" : "out");
	}
	return Bad;
}

/** The Second Board (§3.6 R8, spike S-D): its crafted hands, the dealer's positions, the keyed fallback, and a fuzz. */
void U3SecondBoard()
{
	using ss::PlayerAction;
	std::vector<U3SdCase> Cases;
	// Heads-up deal order: v, hero, v, hero; then burn, flop x3, burn, turn, burn, river (DeckPos 12); the second run burns
	// Deck[12], flops Deck[13..15], burns 16, turns 17, burns 18, rivers 19.
	{
		U3SdCase K;
		K.Name = "HU all in preflop, lose then win";
		K.Cfg = U3HeadsUp(1000, 1000, "KsAsKdAd 2c Kh7c2d 3c 9h 4c Jd 5c 8s6d3h 5d Tc 5h 4d");
		K.Acts = {PlayerAction::RaiseTo(1000), PlayerAction::Call()};
		K.Cards = "8s6d3hTc4d";
		K.Second = 2;
		K.Verdict = 1;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU all in preflop, lost both";
		K.Cfg = U3HeadsUp(1000, 1000, "AsKsAdKd 2c 8h7c2d 3c 9h 4c Jd 5c 8s6d3h 5d Tc 5h 4d");
		K.Acts = {PlayerAction::RaiseTo(1000), PlayerAction::Call()};
		K.Cards = "8s6d3hTc4d";
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU all in on the flop";
		K.Cfg = U3HeadsUp(1000, 1000, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd 5c Kh 5d 2s");
		K.Acts = {PlayerAction::Call(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::RaiseTo(900), PlayerAction::Call()};
		K.Shared = 3;
		K.Cards = "8s6d3hKh2s";
		K.Verdict = 1;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU all in on the turn";
		K.Cfg = U3HeadsUp(1000, 1000, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd 5c 2s");
		K.Acts = {PlayerAction::Call(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::RaiseTo(900),
			PlayerAction::Call()};
		K.Shared = 4;
		K.Cards = "8s6d3h9h2s";
		K.Second = 2;
		K.Verdict = 2;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU all in on the river (no board)";
		K.Cfg = U3HeadsUp(1000, 1000, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd 5c 2s");
		K.Acts = {PlayerAction::Call(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(),
			PlayerAction::Check(), PlayerAction::RaiseTo(900), PlayerAction::Call()};
		K.Shown = false;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU folded on the flop (no board)";
		K.Cfg = U3HeadsUp(1000, 1000, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd");
		K.Acts = {PlayerAction::Call(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::RaiseTo(400), PlayerAction::Fold()};
		K.Shown = false;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU the blinds put both in";
		K.Cfg = U3HeadsUp(50, 100, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd 5c Kh7c2d 5d 2s 5h 2h");
		K.Cards = "Kh7c2d2s2h";
		K.Verdict = 1; // the aces held on the real board; the kings fill up on Kh 2d 2s 2h
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU the small blind all in, v checks";
		K.Cfg = U3HeadsUp(50, 1000, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd 5c Kh7c2d 5d 2s 5h 2h");
		K.Acts = {PlayerAction::Check()};
		K.Cards = "Kh7c2d2s2h";
		K.Verdict = 1;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU chop both runs";
		K.Cfg = U3HeadsUp(1000, 1000, "AcAsKhKd 2c 7d8c9s 2d 2h 6s 3s Tc 4h5cJs 6h 2s 6c 3c");
		K.Acts = {PlayerAction::RaiseTo(1000), PlayerAction::Call()};
		K.Cards = "4h5cJs2s3c";
		K.Second = 1;
		K.Verdict = 1;
		Cases.push_back(K);
	}
	{
		// Three ways, all in preflop: s 500 (AA), hero 1000 (KK), b 3000 (QQ). Main 1,500, side 1,000 (hero, b). DeckPos 14.
		U3SdCase K;
		K.Name = "3-way side pots, hero gets the side";
		K.Cfg.Players = {{0, "hero", 1000}, {1, "s", 500}, {2, "b", 3000}};
		K.Cfg.ButtonSeat = 2;
		K.Cfg.SmallBlind = 50;
		K.Cfg.BigBlind = 100;
		K.Cfg.Deck = U3Deck("KsAsQsKdAdQd 2c 7h8c3d 2d 9c 2h 4s 3c Qh5dJc 3h 6s 4h 2s");
		K.Acts = {PlayerAction::RaiseTo(3000), PlayerAction::Call(), PlayerAction::Call()};
		K.Cards = "Qh5dJc6s2s";
		K.Verdict = 1; // the first run paid hero the side pot: part of what hero could win; b's set takes both on the second
		Cases.push_back(K);
	}
	{
		// Hero 500 shoves, b1 and b2 call; on the flop b1 bets and b2 folds: only the turn and river run twice.
		U3SdCase K;
		K.Name = "3-way, a fold after the all-in";
		K.Cfg.Players = {{0, "hero", 500}, {1, "b1", 3000}, {2, "b2", 3000}};
		K.Cfg.ButtonSeat = 0;
		K.Cfg.SmallBlind = 50;
		K.Cfg.BigBlind = 100;
		K.Cfg.Deck = U3Deck("KsQsAsKdQdAd 2c 7h8c3d 2d 9c 2h 4s 3c Kh 3h 2s");
		K.Acts = {PlayerAction::RaiseTo(500), PlayerAction::Call(), PlayerAction::Call(), PlayerAction::RaiseTo(1000), PlayerAction::Fold()};
		K.Shared = 3;
		K.Cards = "7h8c3dKh2s";
		K.Verdict = 1;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "3-way, hero folded";
		K.Cfg.Players = {{0, "hero", 1000}, {1, "b1", 1000}, {2, "b2", 1000}};
		K.Cfg.ButtonSeat = 0;
		K.Cfg.SmallBlind = 50;
		K.Cfg.BigBlind = 100;
		K.Cfg.Deck = U3Deck("KsQsAsKdQdAd");
		K.Acts = {PlayerAction::Fold(), PlayerAction::RaiseTo(1000), PlayerAction::Call()};
		K.HeroIn = false;
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "9-max, nine all in preflop";
		for (int I = 0; I < 9; ++I)
		{
			K.Cfg.Players.push_back({I, I == 4 ? "hero" : "p" + std::to_string(I), 1000});
		}
		K.Cfg.ButtonSeat = 0;
		K.Cfg.SmallBlind = 50;
		K.Cfg.BigBlind = 100;
		K.Acts = {PlayerAction::RaiseTo(1000)};
		for (int I = 0; I < 8; ++I)
		{
			K.Acts.push_back(PlayerAction::Call());
		}
		Cases.push_back(K);
	}
	{
		U3SdCase K;
		K.Name = "HU short test deck (fallback)";
		K.Cfg = U3HeadsUp(1000, 1000, "KsAsKdAd2cKh7c2d3c9h4cJd");
		K.Cfg.Deck.resize(12);
		K.Acts = {PlayerAction::RaiseTo(1000), PlayerAction::Call()};
		K.FromStub = false;
		Cases.push_back(K);
	}
	int Bad = 0;
	for (const U3SdCase& K : Cases)
	{
		Bad += U3RunSdCase(K);
	}
	Expect(Bad == 0, "the Second Board on spike S-D's " + std::to_string(Cases.size()) +
						 " crafted hands: preflop, flop and turn all-ins, none after a river all-in or a fold, chops, side pots, a fold after the all-in, nine in, the fallback");
	// The nine-way all-in: the second board is exactly where a dealer deals it, Deck[27..29], [31], [33].
	{
		const U3SdCase& K = Cases[12];
		ss::Rng R("u3-sd:" + K.Name);
		ss::Hand H(K.Cfg, R);
		for (const ss::PlayerAction& A : K.Acts)
		{
			H.Act(A);
		}
		show::SecondBoard B;
		show::SecondRun(H, U3SeatOf(H, "hero"), "x", B);
		const std::vector<ss::Card>& Dk = H.DeckForShow();
		Expect(H.DeckPosForShow() == 26 && B.FromStub && B.Shared == 0 && B.Cards[0] == Dk[27] && B.Cards[1] == Dk[28] && B.Cards[2] == Dk[29] && B.Cards[3] == Dk[31] &&
				B.Cards[4] == Dk[33],
			"nine all in: the second run is Deck[27..29], [31], [33] (a burn before each street)");
	}
	// The fallback is keyed by the hand's id and deals none of the cards anyone held or saw.
	{
		const U3SdCase& K = Cases.back();
		ss::Rng R1("a");
		ss::Rng R2("b");
		ss::Hand H1(K.Cfg, R1);
		ss::Hand H2(K.Cfg, R2);
		for (const ss::PlayerAction& A : K.Acts)
		{
			H1.Act(A);
			H2.Act(A);
		}
		show::SecondBoard B1;
		show::SecondBoard B2;
		show::SecondBoard B3;
		show::SecondRun(H1, 0, "rit-9#41", B1);
		show::SecondRun(H2, 0, "rit-9#41", B2);
		show::SecondRun(H1, 0, "rit-9#42", B3);
		bool Fresh = true;
		for (const int Cd : B1.Cards)
		{
			bool Seen = std::find(H1.Board.begin(), H1.Board.end(), Cd) != H1.Board.end();
			for (const ss::HandSeat& S : H1.Seats)
			{
				Seen = Seen || std::find(S.Hole.begin(), S.Hole.end(), Cd) != S.Hole.end();
			}
			Fresh = Fresh && !Seen;
		}
		std::vector<int> Five(B1.Cards.begin(), B1.Cards.end());
		std::sort(Five.begin(), Five.end());
		Expect(!B1.FromStub && U3Board(B1) == U3Board(B2) && U3Board(B1) != U3Board(B3) && Fresh && std::adjacent_find(Five.begin(), Five.end()) == Five.end(),
			"the fallback (a short test deck only) is keyed by the hand id and deals no card anyone was dealt");
		// A short deck with a folded hand: the fallback deals none of the folded cards either, over many keys.
		ss::HandConfig Cfg;
		Cfg.Players = {{0, "f", 1000}, {1, "hero", 1000}, {2, "v", 1000}};
		Cfg.ButtonSeat = 0;
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Deck = U3Deck("2c2d2h2s3c3d4c5c6c7c8c9cTcJc");
		Cfg.Deck.resize(14);
		ss::Rng R3("c");
		ss::Hand H3(Cfg, R3);
		for (const ss::PlayerAction& A : {ss::PlayerAction::Fold(), ss::PlayerAction::RaiseTo(1000), ss::PlayerAction::Call()})
		{
			H3.Act(A);
		}
		const int Folder = U3SeatOf(H3, "f");
		bool NoneFolded = H3.bComplete && Folder >= 0 && H3.Seats[static_cast<size_t>(Folder)].Folded;
		for (int Key = 0; NoneFolded && Key < 64; ++Key)
		{
			show::SecondBoard Bf;
			NoneFolded = show::SecondRun(H3, U3SeatOf(H3, "hero"), "fold#" + std::to_string(Key), Bf) && !Bf.FromStub;
			for (const int Cd : Bf.Cards)
			{
				const std::vector<ss::Card>& Hole = H3.Seats[static_cast<size_t>(Folder)].Hole;
				NoneFolded = NoneFolded && std::find(Hole.begin(), Hole.end(), Cd) == Hole.end();
			}
		}
		Expect(NoneFolded, "the fallback never deals a folded hand's cards (64 keys)");
	}
	// A fuzz of 2,000 random hands: the stub past DeckPos with a burn before each street, never a dealt card, five distinct
	// cards, drawing nothing; the Luck Ledger's all-in equity exists exactly when the player is in.
	ss::Rng R("u3-sd-fuzz");
	int Hands = 0;
	int Shown = 0;
	int Wrong = 0;
	int ByShared[5] = {};
	for (int Trial = 0; Trial < 2000; ++Trial)
	{
		const int Count = 2 + R.Int(8);
		ss::HandConfig Cfg;
		for (int I = 0; I < Count; ++I)
		{
			const Chips Stack = R.Chance(0.1) ? 1 + R.Int(150) : 1 + R.Int(20000);
			Cfg.Players.push_back({I, "p" + std::to_string(I), Stack});
		}
		Cfg.ButtonSeat = R.Int(Count);
		Cfg.SmallBlind = 50;
		Cfg.BigBlind = 100;
		Cfg.Ante = R.Chance(0.3) ? 100 : 0;
		ss::Hand H(Cfg, R);
		for (int Guard = 0; !H.bComplete && Guard < 500; ++Guard)
		{
			const ss::LegalActions L = H.GetLegalActions();
			const double Roll = R.Next();
			ss::PlayerAction A = ss::PlayerAction::Call();
			if (Roll < 0.18)
			{
				A = L.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Fold();
			}
			else if (Roll >= 0.6 && Roll < 0.8 && L.CanRaise)
			{
				A = ss::PlayerAction::RaiseTo(static_cast<double>(L.MinRaiseTo));
			}
			else if (Roll >= 0.8 && L.CanRaise)
			{
				A = ss::PlayerAction::RaiseTo(static_cast<double>(L.MaxRaiseTo));
			}
			if (!H.Act(A))
			{
				break;
			}
		}
		if (!H.bComplete)
		{
			continue;
		}
		++Hands;
		const int Me = R.Int(Count); // drawn before the read: the read must leave R where it was
		uint32_t Before[4];
		uint32_t After[4];
		R.GetState(Before);
		const std::string Text = U3HandText(H);
		const int Pos = H.DeckPosForShow();
		const std::vector<ss::Card>& Dk = H.DeckForShow();
		int RevealBoard = -1;
		size_t SeenBoard = 0;
		for (const ss::HandEvent& Ev : H.Events)
		{
			RevealBoard = Ev.Type == ss::EventType::Reveal && RevealBoard < 0 ? static_cast<int>(SeenBoard) : RevealBoard;
			SeenBoard = Ev.Type == ss::EventType::Street ? Ev.Board.size() : SeenBoard;
		}
		show::SecondBoard B;
		const bool Is = show::SecondRun(H, Me, "fuzz#" + std::to_string(Trial), B);
		bool Ok = Is == (RevealBoard >= 0 && RevealBoard < 5 && H.Board.size() == 5);
		if (Is)
		{
			++Shown;
			++ByShared[B.Shared];
			Ok = Ok && B.FromStub && B.Shared == RevealBoard;
			for (int K = 0; Ok && K < B.Shared; ++K)
			{
				Ok = B.Cards[static_cast<size_t>(K)] == H.Board[static_cast<size_t>(K)];
			}
			const size_t P0 = static_cast<size_t>(Pos);
			const std::vector<size_t> Want = B.Shared == 0 ? std::vector<size_t>{P0 + 1, P0 + 2, P0 + 3, P0 + 5, P0 + 7}
								   : B.Shared == 3 ? std::vector<size_t>{P0 + 1, P0 + 3}
												   : std::vector<size_t>{P0 + 1};
			for (size_t K = 0; Ok && K < Want.size(); ++K)
			{
				Ok = Want[K] < Dk.size() && B.Cards[static_cast<size_t>(B.Shared) + K] == Dk[Want[K]];
			}
			for (int K = B.Shared; Ok && K < 5; ++K)
			{
				Ok = std::find(Dk.begin(), Dk.begin() + Pos, B.Cards[static_cast<size_t>(K)]) == Dk.begin() + Pos;
			}
			std::vector<int> Five(B.Cards.begin(), B.Cards.end());
			std::sort(Five.begin(), Five.end());
			Ok = Ok && std::adjacent_find(Five.begin(), Five.end()) == Five.end();
			Ok = Ok && (ss::autoplay::AllInEquity(H, Me) >= 0.0) == B.HeroIn;
			Ok = Ok && (!B.HeroIn || B.Verdict == show::VerdictOf(B.First, B.Second));
		}
		// The Settlement's record (fix secondboard): exactly when the player was in, and every fact the hand's own.
		show::SecondRecord Rec;
		const bool Recorded = show::SecondRecordOf(H, Me, "fuzz#" + std::to_string(Trial), Rec);
		Ok = Ok && Recorded == (Is && B.HeroIn);
		if (Recorded)
		{
			const ss::HandSeat& Mine = H.Seats[static_cast<size_t>(Me)];
			Ok = Ok && Rec.Hero[0] == Mine.Hole[0] && Rec.Hero[1] == Mine.Hole[1] && std::equal(H.Board.begin(), H.Board.end(), Rec.Ran.begin()) && Rec.Board.Cards == B.Cards &&
				Rec.Board.First == B.First && Rec.Board.Verdict == B.Verdict && Rec.Equity == ss::autoplay::AllInEquity(H, Me) && Rec.Villain[0] >= 0;
		}
		R.GetState(After);
		Ok = Ok && std::equal(Before, Before + 4, After) && U3HandText(H) == Text;
		Wrong += Ok ? 0 : 1;
	}
	std::printf("second board fuzz: %d hands, %d all-ins with cards to come (preflop %d, flop %d, turn %d), %d wrong\n", Hands, Shown, ByShared[0], ByShared[3], ByShared[4],
		Wrong);
	Expect(Hands > 1500 && Wrong == 0 && ByShared[0] > 0 && ByShared[3] > 0 && ByShared[4] > 0,
		"the Second Board over 2,000 random hands: the stub past DeckPos, a burn before each street, never a dealt card, drawing nothing");
}

/**
 * A heads-up preflop all-in's deck, the villain dealt first ("KsAsKdAd": the villain Ks Kd, the player As Ad): the real board
 * Run1, then past the hand's DeckPos the second run Run2, a burn before each street (S-D's walk); the burns are the first
 * cards nobody else uses.
 */
std::vector<ss::Card> U3TwoRuns(const std::string& Holes, const std::string& Run1, const std::string& Run2)
{
	const std::vector<ss::Card> Hole = ss::ParseCards(Holes);
	const std::vector<ss::Card> A = ss::ParseCards(Run1);
	const std::vector<ss::Card> B = ss::ParseCards(Run2);
	std::vector<ss::Card> Used = Hole;
	Used.insert(Used.end(), A.begin(), A.end());
	Used.insert(Used.end(), B.begin(), B.end());
	std::vector<ss::Card> Spare;
	for (const ss::Card C : ss::FullDeck())
	{
		if (std::find(Used.begin(), Used.end(), C) == Used.end())
		{
			Spare.push_back(C);
		}
	}
	size_t Next = 0;
	std::vector<ss::Card> Deck = Hole;
	const auto BurnAndDeal = [&](const std::vector<ss::Card>& Run, size_t From, size_t Count) {
		Deck.push_back(Spare[Next++]);
		Deck.insert(Deck.end(), Run.begin() + static_cast<std::ptrdiff_t>(From), Run.begin() + static_cast<std::ptrdiff_t>(From + Count));
	};
	BurnAndDeal(A, 0, 3);
	BurnAndDeal(A, 3, 1);
	BurnAndDeal(A, 4, 1);
	BurnAndDeal(B, 0, 3);
	BurnAndDeal(B, 3, 1);
	BurnAndDeal(B, 4, 1);
	Deck.insert(Deck.end(), Spare.begin() + static_cast<std::ptrdiff_t>(Next), Spare.end());
	return Deck;
}

/**
 * The Second Board's record (fix secondboard; the lead's PIE 2026-10-21: the Settlement printed AA v K5 "100% | 0%", RAN
 * "LOST", "you'd have WON BOTH"). VerdictOf for every pair of runs; SecondRecordOf on crafted all-ins for a win, a loss, a
 * split and the mixed pairs: both hands, the real board, the second run, what the first run paid, the all-in's equity
 * (AllInEquity, never the river's 1 / 0), the same from a copy (the skip path hands HeroHandDone a copy, LastHeroHand), and
 * nothing drawn; no record after a fold, a river all-in or with the player folded; NoteSecond's bounded log.
 */
void U3SecondRecord()
{
	using ss::PlayerAction;
	const int Pairs[][3] = {{2, 2, 2}, {2, 0, 1}, {0, 2, 1}, {0, 0, 0}, {1, 1, 1}, {1, 2, 1}, {2, 1, 1}, {1, 0, 1}, {0, 1, 1}};
	int PairsOk = 0;
	for (const auto& P : Pairs)
	{
		PairsOk += show::VerdictOf(P[0], P[1]) == P[2] ? 1 : 0;
	}
	Expect(PairsOk == 9, "VerdictOf: won both and lost both only when both runs went that way whole; every other pair (chops too) splits it");
	struct Case
	{
		const char* Name;
		const char* Run1;
		const char* Run2;
		int First, Second, Verdict;
	};
	// Win boards (the aces hold), loss boards (a third king), chop boards (a straight flush on board plays for both).
	const Case Cases[] = {
		{"win", "2c7h9d3s4c", "2h8cTd3d5s", 2, 2, 2},
		{"loss", "Kh7c2d3c9h", "Kc8d4d6cJs", 0, 0, 0},
		{"split", "5h6h7h8h9h", "TcJcQcKcAc", 1, 1, 1},
		{"won, then lost", "2c7h9d3s4c", "Kc8d4d6cJs", 2, 0, 1},
		{"lost, then won", "Kh7c2d3c9h", "2h8cTd3d5s", 0, 2, 1},
		{"split, then won", "5h6h7h8h9h", "2h8cTd3d5s", 1, 2, 1},
	};
	const auto Card = [](const char* Text) { return ss::ParseCards(Text)[0]; };
	const auto Cards = [](const std::vector<int>& V) {
		std::string Out;
		for (const int C : V)
		{
			Out += C >= 0 ? ss::CardToString(C) : std::string("--");
		}
		return Out;
	};
	int Good = 0;
	for (const Case& K : Cases)
	{
		ss::HandConfig Cfg = U3HeadsUp(4000, 4000, "");
		Cfg.Deck = U3TwoRuns("KsAsKdAd", K.Run1, K.Run2);
		ss::Rng R(std::string("u3-record:") + K.Name);
		ss::Hand H(Cfg, R);
		H.Act(PlayerAction::RaiseTo(4000));
		H.Act(PlayerAction::Call());
		const int Me = U3SeatOf(H, "hero");
		uint32_t Before[4];
		uint32_t After[4];
		R.GetState(Before);
		show::SecondRecord Rec;
		show::SecondRecord Copied;
		const bool Is = H.bComplete && show::SecondRecordOf(H, Me, std::string("rec#") + K.Name, Rec);
		const ss::Hand Copy(H);
		const bool Again = show::SecondRecordOf(Copy, Me, std::string("rec#") + K.Name, Copied);
		R.GetState(After);
		const double Eq = ss::autoplay::AllInEquity(H, Me);
		const std::vector<int> Ran(Rec.Ran.begin(), Rec.Ran.end());
		const std::vector<int> Twice(Rec.Board.Cards.begin(), Rec.Board.Cards.end());
		const bool Ok = Is && Again && Rec.Hero == std::array<int, 2>{{Card("As"), Card("Ad")}} && Rec.Villain == std::array<int, 2>{{Card("Ks"), Card("Kd")}} &&
			Cards(Ran) == K.Run1 && Cards(Twice) == K.Run2 && Rec.Board.First == K.First && Rec.Board.Second == K.Second && Rec.Board.Verdict == K.Verdict &&
			Rec.Equity == Eq && Eq > 0.75 && Eq < 0.9 && Copied.Equity == Rec.Equity && Copied.Board.Cards == Rec.Board.Cards && Copied.Board.Verdict == Rec.Board.Verdict &&
			Copied.Board.First == Rec.Board.First && Copied.Ran == Rec.Ran && Copied.Hero == Rec.Hero && Copied.Villain == Rec.Villain && std::equal(Before, Before + 4, After);
		std::printf("second board record %-16s %s v %s, %.3f at the all-in, ran %s (first %d), twice %s (second %d), verdict %d%s\n", K.Name, Cards({Rec.Hero[0], Rec.Hero[1]}).c_str(),
			Cards({Rec.Villain[0], Rec.Villain[1]}).c_str(), Rec.Equity, Cards(Ran).c_str(), Rec.Board.First, Cards(Twice).c_str(), Rec.Board.Second, Rec.Board.Verdict, Ok ? "" : "   WRONG");
		Good += Ok ? 1 : 0;
	}
	Expect(Good == static_cast<int>(sizeof(Cases) / sizeof(Cases[0])),
		"SecondRecordOf: a win, a loss, a split and the mixed pairs keep the hand's cards, the first run's pay, the second run, the verdict and the all-in's equity, the same from a copy, drawing nothing");
	// No record: a fold ends it, the all-in came on the river, or the player folded before the others went all in.
	{
		int None = 0;
		ss::Rng R("u3-record:none");
		ss::Hand Fold(U3HeadsUp(4000, 4000, "KsAsKdAd 2c 7d8c9s 2d 2h 6s 3s"), R);
		Fold.Act(PlayerAction::RaiseTo(400));
		Fold.Act(PlayerAction::Fold());
		show::SecondRecord Rec;
		None += Fold.bComplete && !show::SecondRecordOf(Fold, U3SeatOf(Fold, "hero"), "none#fold", Rec) ? 1 : 0;
		ss::Hand River(U3HeadsUp(1000, 1000, "KsAsKdAd 2c 8s6d3h 3c 9h 4c Jd 5c 2s"), R);
		for (const PlayerAction& A : {PlayerAction::Call(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(), PlayerAction::Check(),
				 PlayerAction::Check(), PlayerAction::RaiseTo(900), PlayerAction::Call()})
		{
			River.Act(A);
		}
		None += River.bComplete && !show::SecondRecordOf(River, U3SeatOf(River, "hero"), "none#river", Rec) ? 1 : 0;
		ss::HandConfig Three;
		Three.Players = {{0, "hero", 1000}, {1, "b1", 3000}, {2, "b2", 3000}};
		Three.ButtonSeat = 0;
		Three.SmallBlind = 50;
		Three.BigBlind = 100;
		Three.Deck = U3Deck("KsAsQsKdAdQd");
		ss::Hand Out(Three, R);
		Out.Act(PlayerAction::Fold());
		Out.Act(PlayerAction::RaiseTo(3000));
		Out.Act(PlayerAction::Call());
		show::SecondBoard B;
		None += Out.bComplete && show::SecondRun(Out, U3SeatOf(Out, "hero"), "none#out", B) && !B.HeroIn && !show::SecondRecordOf(Out, U3SeatOf(Out, "hero"), "none#out", Rec) ? 1 : 0;
		Expect(None == 3, "SecondRecordOf: no record after a fold, a river all-in, or with the player out of the pot");
	}
	// NoteSecond: the player found by HeroId, the record kept with its event and minute, a hand with no all-in skipped, the
	// oldest dropped past Keep; the log's record equals SecondRecordOf's for the same hand.
	{
		std::vector<show::SecondRecord> Log;
		int Added = 0;
		for (int I = 0; I < 14; ++I)
		{
			ss::HandConfig Cfg = U3HeadsUp(4000, 4000, "");
			Cfg.Deck = U3TwoRuns("KsAsKdAd", I % 2 ? "2c7h9d3s4c" : "Kh7c2d3c9h", "2h8cTd3d5s");
			ss::Rng R("u3-note:" + std::to_string(I));
			ss::Hand H(Cfg, R);
			H.Act(PlayerAction::RaiseTo(4000));
			H.Act(PlayerAction::Call());
			Added += show::NoteSecond(Log, H, "runittwice.hourly@47295", "runittwice.hourly@47295#7#" + std::to_string(I), 47300.0 + I) ? 1 : 0;
		}
		ss::Rng R("u3-note:fold");
		ss::Hand Fold(U3HeadsUp(4000, 4000, "KsAsKdAd 2c 7d8c9s 2d 2h 6s 3s"), R);
		Fold.Act(PlayerAction::RaiseTo(400));
		Fold.Act(PlayerAction::Fold());
		const bool Skipped = !show::NoteSecond(Log, Fold, "runittwice.hourly@47295", "runittwice.hourly@47295#7#99", 47400.0);
		const bool Kept = Log.size() == 12 && Log.front().HandId == "runittwice.hourly@47295#7#2" && Log.back().HandId == "runittwice.hourly@47295#7#13" &&
			Log.back().EventId == "runittwice.hourly@47295" && Log.back().At == 47313.0 && Log.back().Board.First == 2 && Log.front().Board.First == 0;
		Expect(Added == 14 && Skipped && Kept, "NoteSecond: every all-in the player was in, with its event and minute, at most 12, a fold skipped");
	}
}

/** The Desk (§3.4 E, §3.6 R7): KPIs and the net line from a fixture History, only the site's own rows, by range. */
void U3Desk()
{
	const double Now = U3At(2026, 12, 20, 22.0);
	auto Row = [Now](const char* Name, int Place, int Entrants, Chips Prize, Chips BuyIn, const std::string& Template, double DaysAgo) {
		ss::HistoryEntry E;
		E.Name = Name;
		E.Place = Place;
		E.Entrants = Entrants;
		E.Prize = Prize;
		E.BuyInCents = BuyIn;
		E.EventId = Template + "@" + std::to_string(static_cast<long long>(std::llround(Now - DaysAgo * SitesDay)));
		return E;
	};
	// Newest first, as the session keeps it.
	const std::vector<ss::HistoryEntry> H = {
		Row("The Hourly", 2, 40, 900, 330, "runittwice.hourly", -0.5), // starts after now: not on the Desk yet
		Row("The Sweat", 3, 120, 15000, 1100, "runittwice.the-sweat", 1.0),
		Row("Flip & Run", 2, 8, 300, 110, "ignite.flip-go-110", 1.2), // Ignite's: not RunItTwice's Desk
		Row("The Nightcap", 45, 80, 0, 10900, "runittwice.the-nightcap", 2.0),
		Row("Night Owl", 7, 300, 2200, 550, "night-owl", 2.5), // RiverLine's
		Row("The Hourly", 31, 60, 0, 0, "runittwice.hourly", 3.0), // a free entry: an event, not a paid one
		Row("Supper Stack", 12, 60, 900, 550, "runittwice.supper-stack", 40.0),
		Row("The Hourly", 1, 30, 4000, 330, "runittwice.hourly", 70.0),
	};
	site::Account A;
	A.Site = net::SiteId::RunItTwice;
	A.RakeMonth = 21240;
	// G1's fix pass (G1_AUDIT L5): the rake is what the account actually paid. The Nightcap came in on a ticket, so its $7.63 fee
	// was never paid: the account's tally is the Sweat's 88, Supper Stack's 44 and the old Hourly's 26 cents.
	A.RakeLife = 88 + 44 + 26;
	const show::DeskKpis K60 = show::Desk(H, A, 60, Now);
	// By hand, the last 60 days: 4 events (the Sweat, the Nightcap, the free Hourly, Supper Stack); $125.50 in, $159.00 back.
	Expect(K60.Events == 4 && K60.Net == 3350 && std::fabs(K60.Roi - 3350.0 / 12550.0) < 1e-12 && K60.Itm == 0.5 && K60.Ft == 0.25 && K60.Abi == 4183 &&
			K60.Rake == 158 && K60.Club == 3186,
		"the Desk, last 60 days: 4 events, net +$33.50, ROI 26.7%, ITM 50%, FT 25%, ABI $41.83 over the paid ones, rake paid $1.58, Club $31.86 accrued");
	const show::DeskKpis All = show::Desk(H, A, 0, Now);
	Expect(All.Events == 5 && All.Net == 7020 && All.Ft == 0.4 && All.Itm == 0.6 && All.Abi == 3220 && All.Rake == 158 && All.Rake == show::RakePaid(A),
		"the Desk over all of History: 5 events, net +$70.20, ABI $32.20 (nothing from Ignite, RiverLine or the future); rake paid $1.58, the account's");
	const std::vector<show::NetPoint> Line = show::DeskNet(H, net::SiteId::RunItTwice, 60, Now);
	Expect(Line.size() == 4 && Line[0].Net == 350 && Line[1].Net == 350 && Line[2].Net == -10550 && Line[3].Net == 3350 && Line[3].Net == K60.Net &&
			Line[0].At < Line[1].At && Line[2].At < Line[3].At,
		"the Desk's net line: oldest first, cumulative, ending at the KPIs' net");
	site::Account Ig;
	Ig.Site = net::SiteId::Ignite;
	Ig.RakeMonth = 21240;
	Ig.RakeLife = 12;
	const show::DeskKpis KI = show::Desk(H, Ig, 60, Now);
	Expect(KI.Events == 1 && KI.Net == 190 && KI.Rake == 12 && KI.Club == 0, "Ignite's numbers are Ignite's rows (and Ignite has no Club)");
	Expect(show::Desk({}, A, 30, Now).Events == 0 && show::Desk({}, A, 30, Now).Roi == 0.0 && show::DeskNet({}, net::SiteId::RunItTwice, 30, Now).empty(),
		"an empty History: zeros, no line");
}

/**
 * G1's fix pass (fix-rit, G1_AUDIT L5): one final table and one rake. A final table is the event's last table (8 seats at
 * the rivals' default, 6 at six-max, 9 when the event isn't known), and the rake is what the account actually paid: a
 * ticket's entry pays none, so its face value never shows as rake, and the Desk's figure is the Ledger's whatever the range.
 */
void U3DeskFix()
{
	const double Now = U3At(2026, 12, 20, 22.0);
	const auto Id = [Now](const std::string& Template, double DaysAgo) {
		return Template + "@" + std::to_string(static_cast<long long>(std::llround(Now - DaysAgo * SitesDay)));
	};
	Expect(show::FinalTableSeats(Id("runittwice.the-sweat", 1.0)) == 8 && show::FinalTableSeats(Id("runittwice.six-max-classic", 1.0)) == 6 &&
			show::FinalTableSeats(Id("night-owl", 1.0)) == 9 && show::FinalTableSeats("") == 9 && show::FinalTableSeats("runittwice.no-such-event@100") == 9,
		"final tables: an 8-max event's is 8 seats, a six-max's 6; RiverLine's Night Owl 9, and an unknown event 9");
	auto Row = [&](const char* Name, int Place, Chips Prize, Chips BuyIn, const std::string& Template, double DaysAgo) {
		ss::HistoryEntry E;
		E.Name = Name;
		E.Place = Place;
		E.Entrants = 120;
		E.Prize = Prize;
		E.BuyInCents = BuyIn;
		E.EventId = Id(Template, DaysAgo);
		return E;
	};
	const ss::HistoryEntry Eighth = Row("The Sweat", 8, 2200, 1100, "runittwice.the-sweat", 1.0);
	const ss::HistoryEntry Ninth = Row("The Sweat", 9, 1700, 1100, "runittwice.the-sweat", 2.0);
	const ss::HistoryEntry Sixth = Row("Six-Max Classic", 6, 4400, 2200, "runittwice.six-max-classic", 3.0);
	const ss::HistoryEntry Seventh = Row("Six-Max Classic", 7, 3300, 2200, "runittwice.six-max-classic", 4.0);
	Expect(show::MadeFinalTable(Eighth) && !show::MadeFinalTable(Ninth) && show::MadeFinalTable(Sixth) && !show::MadeFinalTable(Seventh),
		"final tables: 8th of an 8-max is one, 9th isn't; 6th of a six-max is one, 7th isn't");
	site::Account Empty;
	Empty.Site = net::SiteId::RunItTwice;
	Expect(show::Desk({Eighth, Ninth, Sixth, Seventh}, Empty, 0, Now).Ft == 0.5, "the Desk's FT: 2 of those 4 made the final table (was 3: 9th counted on an 8-max)");
	// The money path: the Sweat paid from the wallet (its $0.88 fee is rake), the Nightcap on a ticket (the session's ticket
	// branch writes a zero line and never calls OnBuyIn).
	site::Book B;
	site::Join(B, net::SiteId::RunItTwice, Now - 10.0 * SitesDay);
	site::Account* Acct = site::AccountOf(B, net::SiteId::RunItTwice);
	const net::SiteDef& Rit = net::Def(net::SiteId::RunItTwice);
	site::OnBuyIn(B, net::SiteId::RunItTwice, 1100, Rit.Rake.Fee(1100), "Buy-in: The Sweat", Now - 2.0 * SitesDay);
	site::AddLine(B, net::SiteId::RunItTwice, Now - 1.0 * SitesDay, 0, 0, "Ticket: The Nightcap");
	const std::vector<ss::HistoryEntry> H = {Row("The Nightcap", 30, 0, 10900, "runittwice.the-nightcap", 1.0), Row("The Sweat", 40, 0, 1100, "runittwice.the-sweat", 2.0)};
	const show::DeskKpis All = Acct ? show::Desk(H, *Acct, 0, Now) : show::DeskKpis();
	const show::DeskKpis Week = Acct ? show::Desk(H, *Acct, 7, Now) : show::DeskKpis();
	std::printf("  desk: rake paid %lld cents (the account's %lld; History's fees would say %lld)\n", static_cast<long long>(All.Rake), static_cast<long long>(Acct ? Acct->RakeLife : -1),
		static_cast<long long>(Rit.Rake.Fee(1100) + Rit.Rake.Fee(10900)));
	Expect(Acct && Acct->RakeLife == Rit.Rake.Fee(1100) && All.Rake == Acct->RakeLife && Week.Rake == All.Rake && All.Rake == show::RakePaid(*Acct) && All.Rake < Rit.Rake.Fee(1100) + Rit.Rake.Fee(10900),
		"the Desk's rake is the rake actually paid (the account's, the Ledger's RAKE PAID): a ticket's face value is never rake, whatever the range");
}

/** Sunday Live (§3.4 B, §3.6 R6): the story's rows from the network's own numbers and the world's, only what has happened. */
void U3SundayLive()
{
	const net::Network& N = net::Site(net::SiteId::RunItTwice);
	const double Sunday = U3At(2026, 11, 22, 0.0);
	net::EventInstance Major;
	bool Found = false;
	for (const net::EventInstance& E : N.Window(Sunday, Sunday + SitesDay))
	{
		if (!Found && E.Id.rfind("runittwice.sunday-100k@", 0) == 0)
		{
			Major = E;
			Found = true;
		}
	}
	Expect(Found, "the Sunday 100K runs on Sunday");
	if (!Found)
	{
		return;
	}
	const net::EventTemplate& T = N.TemplateOf(Major);
	const double Opens = Major.Start - static_cast<double>(T.OpensHours) * 60.0;
	const double Ends = Major.Start + Major.Duration;
	Expect(show::SundayLive(N, nullptr, Major, Opens - 1.0).empty(), "Sunday Live: nothing before registration opens");
	const std::vector<show::LiveRow> Before = show::SundayLive(N, nullptr, Major, Major.Start - 90.0);
	const std::string Gtd = T.GtdCents % 100 == 0 ? "$" + ss::Grouped(T.GtdCents / 100) : ss::Money(T.GtdCents);
	bool Counts = Before.size() == 4 && Before[0].At == Opens && Before[0].Kind == show::LiveFact &&
		Before[0].Text == "Registration opens for " + T.Name + ": $109 buy-in, " + Gtd + " guaranteed.";
	const double Marks[] = {360.0, 180.0, 120.0};
	for (size_t K = 1; Counts && K < Before.size(); ++K)
	{
		const double At = Major.Start - Marks[K - 1];
		Counts = Before[K].At == At && Before[K].Text == ss::Grouped(N.Live(Major, At).Entries) + " registered for " + T.Name + ".";
	}
	Expect(Counts, "before the start: registration's opening and the network's own count at 6, 3 and 2 hours out (" + std::to_string(Before.size()) + " rows)");
	// After the end: in time order, nothing past World, registration's close with the final field, the overlay iff there was
	// one, the winner and the podium from the result, and no chip count anywhere.
	auto Ending = [&](const std::vector<show::LiveRow>& Rows, const char* What) {
		bool Ordered = !Rows.empty() && Rows.size() <= 40;
		bool NoChips = true;
		int Closed = 0;
		int Overlay = 0;
		int Winner = 0;
		int Podium = 0;
		bool Actual = false;
		const Chips Short = show::ProjectedOverlay(N, Major, Major.Entries, Ends + 1.0, Actual);
		const net::EventResult& Res = N.Result(Major);
		std::string First;
		int Places = 0;
		for (const net::Placing& P : Res.FinalTable)
		{
			First = P.Place == 1 ? (P.Player >= 0 ? N.Players()[static_cast<size_t>(P.Player)].Name : P.Player == -1 ? N.HeroName() : P.Name) : First;
			Places += P.Place >= 1 && P.Place <= 3 ? 1 : 0;
		}
		for (size_t K = 0; K < Rows.size(); ++K)
		{
			const std::string& Tx = Rows[K].Text;
			Ordered = Ordered && Rows[K].At <= Ends + 1.0 && Rows[K].At >= Opens && (K == 0 || Rows[K - 1].At <= Rows[K].At);
			NoChips = NoChips && Tx.find("chips") == std::string::npos && Tx.find("stack") == std::string::npos;
			Closed += Tx.rfind("Registration closed at ", 0) == 0 && Tx.find(": " + ss::Grouped(Major.Entries) + " entries, ") != std::string::npos ? 1 : 0;
			Overlay += Tx.rfind("Overlay paid: ", 0) == 0 ? 1 : 0;
			Winner += !First.empty() && Tx.rfind(First + " wins " + T.Name + " for $", 0) == 0 ? 1 : 0;
			Podium += Rows[K].Kind == show::LivePodium ? 1 : 0;
		}
		std::printf("sunday live (%s): %zu rows by the end of %s (%d entries, overlay %s, %d places on the podium)\n", What, Rows.size(), T.Name.c_str(), Major.Entries,
			ss::Money(Short).c_str(), Places);
		Expect(Ordered && NoChips && Closed == 1 && Overlay == (Short > 0 ? 1 : 0) && Winner == (First.empty() ? 0 : 1) && Podium == Places,
			std::string("Sunday Live after the end (") + What + "): in time order, the close with the final field, the overlay iff there was one, the winner and the podium, no chip counts");
	};
	Ending(show::SundayLive(N, nullptr, Major, Ends + 1.0), "no world");
	// With a living world: the site's own team pros and the most-followed players in the field, from the world's plan.
	ss::world::World W;
	W.Create(95u, 1440.0 + 127.0);
	net::AttachAll(&W);
	W.AdvanceTo(Major.Start - 60.0);
	const std::vector<int> Planned = W.Registered(Major.Id);
	const std::vector<show::LiveRow> Field = show::SundayLive(N, &W, Major, Major.Start - 60.0);
	int TeamIn = 0;
	for (const int Id : Planned)
	{
		TeamIn += W.Get(Id) && W.Get(Id)->Sponsor == "Team RunItTwice" ? 1 : 0;
	}
	int TeamRows = 0;
	int NotableRows = 0;
	bool Honest = true;
	for (const show::LiveRow& Rw : Field)
	{
		const bool Listed = std::find(Planned.begin(), Planned.end(), Rw.Npc) != Planned.end();
		if (Rw.Kind == show::LiveTeam)
		{
			++TeamRows;
			Honest = Honest && Listed && W.Get(Rw.Npc)->Sponsor == "Team RunItTwice" && Rw.Text == W.Get(Rw.Npc)->Name + " of Team RunItTwice is registered.";
		}
		if (Rw.Kind == show::LiveNotable)
		{
			++NotableRows;
			Honest = Honest && Listed && W.Get(Rw.Npc)->Followers >= 2500;
		}
		Honest = Honest && Rw.At <= Major.Start - 60.0;
	}
	std::printf("sunday live with a world: %zu planned, %d of Team RunItTwice, %d notable rows\n", Planned.size(), TeamIn, NotableRows);
	Expect(Honest && TeamRows == std::min(6, TeamIn) && NotableRows <= 4, "the field's notable entries: the team's pros and the most-followed, all on the world's own list");
	W.AdvanceTo(Ends + 1.0);
	Ending(show::SundayLive(N, &W, Major, Ends + 1.0), "a living world");
	net::AttachAll(nullptr);
}

/** Ignite's welcome bonus as the page draws it (§2.6 F11): the meter's next $5 is exactly the rake SiteMoney releases it at. */
void U3Welcome()
{
	const double T0 = U3At(2026, 11, 10, 10.0);
	State L;
	Chips Bank = 100000;
	site::Join(L.Sites, net::SiteId::Ignite, T0);
	site::Deposit(L.Sites, L, Bank, net::SiteId::Ignite, 5000, "", T0);
	site::Account* A = site::AccountOf(L.Sites, net::SiteId::Ignite);
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 0, 6660, "Buy-in", T0 + 10.0);
	const show::WelcomeView V = show::Welcome(*A, T0 + 20.0);
	Expect(V.Pending == 3500 && V.Released == 1500 && V.NextIn == 1340 && std::fabs(V.NeedleT - 0.3) < 1e-12 && V.Ends == A->BonusExpires,
		"the welcome view: $35.00 pending, $15.00 released, next $5 in $13.40 of rake, the needle at 30%");
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 0, V.NextIn - 1, "Buy-in", T0 + 30.0);
	const Chips Held = A->BonusReleased;
	site::OnBuyIn(L.Sites, net::SiteId::Ignite, 0, 1, "Buy-in", T0 + 40.0);
	Expect(Held == 1500 && A->BonusReleased == 2000 && show::Welcome(*A, T0 + 50.0).NextIn == 2000, "a cent short of the meter releases nothing; the meter's cent releases $5");
	const show::WelcomeView Late = show::Welcome(*A, A->BonusExpires + 1.0);
	site::Account None;
	const show::WelcomeView Nil = show::Welcome(None, T0);
	Expect(Late.NextIn == 0 && Late.Released == 2000 && Nil.Pending == 0 && Nil.Released == 0 && Nil.NextIn == 0 && Nil.NeedleT == 0.0 && Nil.Ends == 0.0,
		"after the 60 days the meter stops; no bonus, no view");
}

/** site::show's copy (§1.10): linted, no real names, never AI, bot or autoplay. */
void U3ShowCopy()
{
	std::map<std::string, std::string> Vars = copylint::Samples();
	Vars.insert({{"Event", "RunItTwice Sunday 100K"}, {"Buyin", "$109"}, {"Gtd", "$100,000"}, {"Count", "1,204"}, {"Clock", "21:30"}, {"Pool", "$122,047"}, {"Paid", "152"},
		{"Amount", "$4,100"}, {"Left", "8"}, {"Who", "Ana Ribeiro"}, {"Place", "2nd"}, {"Team", "Team RunItTwice"}});
	std::vector<std::pair<std::string, std::string>> Lines;
	bool Clean = true;
	for (const std::string& S : show::Inventory())
	{
		Lines.emplace_back("site::show", S);
		Clean = Clean && !trademarks::Find(S, trademarks::kSourceScan) && !trademarks::Find(S, trademarks::kCopyScan);
		std::string Word;
		for (const char Ch : copylint::Lower(S) + " ")
		{
			if (Ch >= 'a' && Ch <= 'z')
			{
				Word.push_back(Ch);
				continue;
			}
			Clean = Clean && Word != "ai" && Word != "bot" && Word != "bots" && Word != "autoplay" && Word != "autopilot";
			Word.clear();
		}
	}
	const int Problems = copylint::Run("site::show", Lines, copylint::Pa, Vars);
	Expect(Problems == 0 && Clean && show::Inventory().size() >= 12, "Sunday Live's lines pass the copy lint and the trademark screen, and never say AI, bot or autoplay");
}

/** The marks (§3.4, §13.2 #13, §2.5): RunItTwice's logo and strip in red and black, the new siteart, the six emblem families. */
void U3Marks()
{
	const SitesNoText Measure;
	const std::set<uint32_t> RiverLine(net::Def(net::SiteId::RiverLine).Palette.begin(), net::Def(net::SiteId::RiverLine).Palette.end());
	const std::set<uint32_t> SlateGold = {0x334155, 0x1c2433, 0xd4a843, 0xeef1f5, 0x141a24, 0x243044, 0x0b0f17, 0xe5505e, 0xe88b45, 0xe8c25a, 0x5e6a7d, 0x8fa3bf, 0x1d4a3e};
	auto Colours = [](const ui::DrawList& L) {
		std::set<uint32_t> Out;
		for (const ui::Vertex& V : L.Vertices)
		{
			if (V.Col.A > 0.02f)
			{
				Out.insert(U3Rgb(V.Col));
			}
		}
		return Out;
	};
	auto Leaks = [&](const std::set<uint32_t>& Seen) {
		int Hits = 0;
		for (const uint32_t C : Seen)
		{
			Hits += RiverLine.count(C) + SlateGold.count(C) > 0 ? 1 : 0;
		}
		return Hits;
	};
	// The wide logo (the rail's {16,14,184,44}) and the square one (the poker button's 24).
	ui::DrawList Wide;
	{
		ui::Canvas C(Wide, Measure, 400.0f, 100.0f, 1.0f);
		ui::siteart::Logo(C, {16.0f, 14.0f, 184.0f, 44.0f}, net::SiteId::RunItTwice);
	}
	bool TwiceLed = false;
	bool RunItInk = false;
	bool RedInk = false;
	for (const ui::DrawCmd& Cmd : Wide.Cmds)
	{
		if (Cmd.Type != ui::DrawCmd::Kind::Text)
		{
			continue;
		}
		TwiceLed = TwiceLed || (Cmd.Text.Text == "Twice" && U3Rgb(Cmd.Text.Col) == 0xff2633);
		RunItInk = RunItInk || (Cmd.Text.Text == "RunIt" && U3Rgb(Cmd.Text.Col) == 0xececef);
		RedInk = RedInk || (Cmd.Text.Text != "Twice" && (U3Rgb(Cmd.Text.Col) == 0xd2141a || U3Rgb(Cmd.Text.Col) == 0xff2633));
	}
	const bool Words = TwiceLed && RunItInk;
	const std::set<uint32_t> WideCols = Colours(Wide);
	ui::DrawList Square;
	{
		ui::Canvas C(Square, Measure, 100.0f, 100.0f, 1.0f);
		ui::siteart::Logo(C, {6.0f, 3.0f, 24.0f, 24.0f}, net::SiteId::RunItTwice);
	}
	const std::set<uint32_t> SquareCols = Colours(Square);
	Expect(Words && !RedInk && WideCols.count(0xd2141a) && WideCols.count(0xffffff) && SquareCols.count(0xd2141a) && Leaks(WideCols) == 0 && Leaks(SquareCols) == 0,
		"RunItTwice's logo: the two cards in chrome and Signal Red on a gloss tile, \"RunIt\" in Ink, \"Twice\" in LED Red, no slate or gold, no RiverLine colour");
	ui::DrawList Band;
	{
		ui::Canvas C(Band, Measure, 200.0f, 40.0f, 1.0f);
		ui::siteart::Strip(C, {12.0f, 4.0f, 100.0f, 16.0f}, net::SiteId::RunItTwice);
	}
	bool Name = false;
	for (const ui::DrawCmd& Cmd : Band.Cmds)
	{
		Name = Name || (Cmd.Type == ui::DrawCmd::Kind::Text && Cmd.Text.Text == "RUNITTWICE" && U3Rgb(Cmd.Text.Col) == 0xffffff && Cmd.Text.Face == ui::Font::BoldCondensed);
	}
	const std::set<uint32_t> BandCols = Colours(Band);
	Expect(Name && BandCols.count(0xd2141a) && BandCols.count(0xff2633) && Leaks(BandCols) == 0,
		"RunItTwice's strip: a red band (Signal Red to Deep Red, an LED Red edge), the name in Hot White in its display face");
	// The Twin Stripe back's mark: Hot White cards with an Onyx edge, nothing else.
	ui::DrawList Back;
	{
		ui::Canvas C(Back, Measure, 100.0f, 100.0f, 1.0f);
		ui::siteart::TwinStripeMark(C, {30.0f, 30.0f, 40.0f, 40.0f});
	}
	const std::set<uint32_t> BackCols = Colours(Back);
	Expect(!Back.Vertices.empty() && BackCols.size() <= 2 && BackCols.count(0xffffff) && (BackCols.size() == 1 || BackCols.count(0x0c0c0f)),
		"the Twin Stripe back's mark: the two cards in Hot White, edged in Onyx");
	// Ignite's neon sign: BlackItalic, only colours' own alpha (the parent's carries through, untouched).
	ui::DrawList Neon;
	float NeonW = 0.0f;
	float AlphaAfter = 0.0f;
	{
		ui::Canvas C(Neon, Measure, 600.0f, 200.0f, 1.0f);
		C.SetAlpha(0.5f);
		NeonW = ui::siteart::NeonSign(C, 40.0f, 140.0f, 60.0f, 1.0f);
		AlphaAfter = C.GetAlpha();
	}
	int Runs = 0;
	bool Faint = true;
	for (const ui::DrawCmd& Cmd : Neon.Cmds)
	{
		if (Cmd.Type == ui::DrawCmd::Kind::Text)
		{
			++Runs;
			Faint = Faint && Cmd.Text.Text == "Ignite" && Cmd.Text.Face == ui::Font::BlackItalic && Cmd.Text.Col.A <= 0.5f * 0.22f + 1e-4f;
		}
	}
	for (const ui::Vertex& V : Neon.Vertices)
	{
		Faint = Faint && V.Col.A <= 0.5f * 0.22f + 1e-4f;
	}
	Expect(Runs == 4 && Faint && AlphaAfter == 0.5f && NeonW > 60.0f, "Ignite's neon sign: three soft copies under a crisp one, BlackItalic, at most 0.22 of the parent alpha, which it leaves alone");
	// The division crests in the status ramp, and the chrome plate.
	int Crests = 0;
	const uint32_t Metal[5] = {0xb6bbc5, 0x1e1e23, 0xf3f4f6, 0xe6c66a, 0xd2141a};
	for (int Div = 0; Div < 5; ++Div)
	{
		ui::DrawList L;
		{
			ui::Canvas C(L, Measure, 100.0f, 100.0f, 1.0f);
			ui::siteart::DivisionShield(C, 50.0f, 55.0f, 40.0f, Div, Div == 2 ? 3 : 0);
		}
		const std::set<uint32_t> Cols = Colours(L);
		Crests += Cols.count(Metal[Div]) && Leaks(Cols) == 0 ? 1 : 0;
	}
	ui::DrawList Plate;
	{
		ui::Canvas C(Plate, Measure, 300.0f, 100.0f, 1.0f);
		ui::siteart::ChromePlate(C, {20.0f, 20.0f, 200.0f, 36.0f}, 6.0f);
	}
	const std::set<uint32_t> PlateCols = Colours(Plate);
	Expect(Crests == 5 && PlateCols.count(0xf3f4f6) && PlateCols.count(0x8d929c) && Leaks(PlateCols) == 0,
		"the division crests wear their status metals (Open chrome, Iron carbon, Steel chrome, Gold, High Bridge red), the chrome plate its ramp");
	// RunItTwice's six emblem families (EventArt): carbon, gloss and Signal Red tiles, no slate or gold, no RiverLine colour.
	const net::Network& N = net::Site(net::SiteId::RunItTwice);
	int Families = 0;
	int Dirty = 0;
	int Accented = 0;
	const std::pair<const char*, uint32_t> Six[] = {{"hourly", 0xd2141a}, {"the-closer", 0xffffff}, {"first-light", 0xb6bbc5}, {"lunch-hour", 0xb6bbc5},
		{"the-rush", 0xb6bbc5}, {"supper-stack", 0xd2141a}};
	for (const std::pair<const char*, uint32_t>& Fam : Six)
	{
		const char* Stem = Fam.first;
		const net::EventTemplate* T = N.FindTemplate(std::string("runittwice.") + Stem);
		if (!T)
		{
			continue;
		}
		++Families;
		ui::DrawList L;
		{
			ui::Canvas C(L, Measure, 200.0f, 200.0f, 1.0f);
			ui::eventart::Emblem(C, *T, 100.0f, 100.0f, 96.0f, 0.0);
		}
		const std::set<uint32_t> Cols = Colours(L);
		const bool Clean = Leaks(Cols) == 0;
		Dirty += Clean ? 0 : 1;
		// Its accent is drawn (a glyph's stones, or a gradient that runs into it: within 16 of it on every channel).
		bool Near = false;
		for (const uint32_t C : Cols)
		{
			bool Close = true;
			for (const int Shift : {16, 8, 0})
			{
				Close = Close && std::abs(static_cast<int>((C >> Shift) & 255) - static_cast<int>((Fam.second >> Shift) & 255)) <= 16;
			}
			Near = Near || Close;
		}
		Accented += Near ? 1 : 0;
		if (!Near)
		{
			std::printf("  emblem %s: its accent %06x isn't drawn (%zu colours)\n", Stem, Fam.second, Cols.size());
		}
		if (!Clean)
		{
			std::string Hits;
			for (const uint32_t C : Cols)
			{
				char Hex[16];
				std::snprintf(Hex, sizeof(Hex), " %06x", C);
				Hits += RiverLine.count(C) + SlateGold.count(C) > 0 ? Hex : "";
			}
			std::printf("  emblem %s draws%s\n", Stem, Hits.c_str());
		}
	}
	std::printf("rit emblems: %d families, %d with their red, chrome or white accent drawn\n", Families, Accented);
	Expect(Families == 6 && Dirty == 0 && Accented == 6, "RunItTwice's six emblem families on Onyx and Carbon in red, chrome and white (no slate, no gold, no RiverLine colour)");
}
} // namespace sites_test

int main(int Argc, char** Argv)
{
	using namespace sites_test;
	const auto T0 = std::chrono::steady_clock::now();
	if (Argc > 1 && std::string(Argv[1]) == "soft")
	{
		// sites_test soft [count]: the soft population's bound on more sit-and-gos than the default run plays.
		SoftBound(Argc > 2 ? std::max(1, std::atoi(Argv[2])) : 2000);
		std::printf("sites tests: %d passed, %d failed in %.1fs\n", Passed, Failures, SitesSeconds(T0));
		return Failures == 0 ? 0 : 1;
	}
	RiverLineUnchanged();
	RiverLinePinned();
	RakeHook();
	Routing();
	RoutedCallSites();
	RivalDefaults();
	SoftPopulation();
	Skins();
	ReadyForB3();
	// B3 (core-sites): the real rivals.
	RealSites();
	RealSchedules();
	LaunchAndFields();
	Money();
	RentLine();
	Invitations();
	Promotions();
	PaidLetters();
	HeroBySite();
	RivalNews();
	RivalNewsIsTheirs();
	Threading();
	SaveLines();
	SitesMarks();
	StepBudget();
	SitesCopy();
	SitesFormatNames();
	// Update 3 G1 (core-sites-look): site::show and the marks.
	U3Moods();
	U3Overlay();
	U3Club();
	U3SecondBoard();
	U3SecondRecord();
	U3Desk();
	U3DeskFix();
	U3SundayLive();
	U3Welcome();
	U3ShowCopy();
	U3Marks();
	SoftBound(300);
	std::printf("sites tests: %d passed, %d failed in %.1fs\n", Passed, Failures, SitesSeconds(T0));
	return Failures == 0 ? 0 : 1;
}
