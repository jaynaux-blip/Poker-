// The living world on its own: invariants, save and load, and long runs.
//
//   world_test                     the checks (ctest)
//   world_test years N [seed]      plays N years and prints the world's health every year
//   world_test npc NAME [days]     plays some days and describes someone
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>

using namespace ss;

namespace
{
int Failures = 0;

void Check(bool Ok, const std::string& What)
{
	if (!Ok)
	{
		++Failures;
		std::printf("FAIL: %s\n", What.c_str());
	}
}

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (char C : Line)
	{
		if (C == '\t')
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur += C;
		}
	}
	Out.push_back(Cur);
	return Out;
}

void Load(world::World& W, const std::string& Text)
{
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		W.Read(Tabs(Line));
	}
	W.Finish();
}

double Seconds(std::chrono::steady_clock::time_point Since)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - Since).count();
}

const double NightOne = 1440.0 + 127.0;

/** Everyone's numbers make sense. */
void Invariants(const world::World& W, const std::string& When)
{
	int Active = 0;
	std::set<std::string> Names;
	for (const world::Npc& N : W.People())
	{
		Active += N.Playing() ? 1 : 0;
		Check(N.Bankroll >= 0, When + ": " + N.Name + " has a negative bankroll");
		Check(!N.Name.empty() && Names.insert(N.Name).second, When + ": duplicate or empty name " + N.Name);
		for (float S : N.Skills)
		{
			Check(S > 0.0f && S < 1.0f && std::isfinite(S), When + ": " + N.Name + " skill out of range");
		}
		for (const world::Ledger& L : N.Totals)
		{
			Check(L.Wins <= L.FinalTables + 1 && L.Cashes >= 0 && L.Won >= 0 && L.Events >= 0, When + ": " + N.Name + " ledger is inconsistent");
		}
		Check(N.Tier >= 1 && N.Tier <= 4, When + ": " + N.Name + " tier");
		Check(N.Ties.size() <= 7, When + ": " + N.Name + " too many ties");
		Check(N.Recent.size() <= 10, When + ": " + N.Name + " too many recent results");
	}
	Check(Active > 1000, When + ": the world emptied out (" + std::to_string(Active) + " active)");
	Check(W.View().size() == W.People().size(), When + ": the network view is out of step");
}

int Checks()
{
	const auto T0 = std::chrono::steady_clock::now();
	world::World W;
	W.Create(1234u, NightOne);
	std::printf("created %zu people in %.2fs\n", W.People().size(), Seconds(T0));
	Invariants(W, "day one");
	// The cast and the rival are in it.
	for (const char* Name : {"Sal", "Mrs. Park", "Rick", "Mei", "Dre", "Big Lou", "Twitch", "gh0stfold", "VikingVolta", "MissFinch"})
	{
		Check(W.Find(Name) >= 0, std::string("missing ") + Name);
	}
	Check(W.Find("gh0stfold") == net::Shared().RivalIndex(), "the rival keeps the network's index");
	// Tonight's events have registrations, and the people in them can afford them.
	int Registered = 0;
	for (const net::EventInstance& E : net::Shared().Window(NightOne, NightOne + 600.0))
	{
		for (int Id : W.Registered(E.Id))
		{
			++Registered;
			const world::Npc* N = W.Get(Id);
			Check(N && N->Playing(), "registered someone who isn't playing");
		}
	}
	Check(Registered > 200, "tonight's events have the world's regulars in them (" + std::to_string(Registered) + ")");
	// The Riverside's Sunday has its cast.
	{
		const int Sunday = 6; // Sunday, October 11
		W.EnsurePlanned(Sunday);
		bool Planned = false;
		for (int Id : W.Registered("riverside@" + std::to_string(Sunday)))
		{
			Planned = Planned || W.Get(Id)->Name == "Sal";
		}
		Check(!Planned, "a Sunday five days out isn't planned yet");
	}

	// A month: results come back, the boards fill, the story starts.
	const auto T1 = std::chrono::steady_clock::now();
	W.Simulate(30);
	const double Month = Seconds(T1);
	std::printf("30 days in %.2fs\n", Month);
	Invariants(W, "day 30");
	int Finished = 0;
	int Anonymous = 0;
	int Ours = 0;
	for (const net::EventInstance& E : net::Shared().Window(W.Clock() - 2880.0, W.Clock() - 1440.0))
	{
		if (const net::EventResult* R = W.ResultOf(E.Id))
		{
			++Finished;
			for (const net::Placing& P : R->FinalTable)
			{
				Anonymous += P.Player == -2 ? 1 : 0;
				Ours += P.Player >= 0 ? 1 : 0;
				Check(P.Player != -2 || !P.Name.empty(), "an anonymous finisher has a name");
				Check(P.Player < 0 || W.Get(P.Player) != nullptr, "a finisher is someone");
			}
		}
	}
	Check(Finished > 50, "yesterday's events have results (" + std::to_string(Finished) + ")");
	Check(Ours > 0 && Anonymous > 0, "final tables mix the world's regulars and everyone else");
	std::printf("yesterday: %d results, %d regulars and %d others at final tables\n", Finished, Ours, Anonymous);
	Check(!W.Events().empty(), "things happened");
	Check(W.BoardValue(net::Board::Season, W.Find("gh0stfold"), W.Clock()) > 0.0, "the rival has season points");

	// Save and load: a world that's saved carries on exactly as one that isn't.
	std::string Saved;
	W.Write(Saved);
	std::printf("save: %zu KB\n", Saved.size() / 1024);
	Check(Saved.size() < 4u * 1024u * 1024u, "the save stays reasonable");
	world::World Copy;
	Load(Copy, Saved);
	std::string Again;
	Copy.Write(Again);
	Check(Again == Saved, "a loaded world writes the same save");
	W.Simulate(20);
	Copy.Simulate(20);
	std::string A;
	std::string B;
	W.Write(A);
	Copy.Write(B);
	Check(A == B, "a loaded world plays on exactly as the original");
	Invariants(Copy, "after load");

	// Different saves, different worlds.
	world::World Other;
	Other.Create(98765u, NightOne);
	Other.Simulate(50);
	std::string C;
	Other.Write(C);
	Check(C != A, "another save has another history");

	// The player: memories and greetings.
	{
		const int Sal = W.Find("Sal");
		W.BackRoomNight(W.Clock(), {"Sal", "Big Lou", "Twitch", "Mei"}, 4200);
		const world::Bond* Bd = W.BondWith(Sal);
		Check(Bd && !Bd->Memories.empty(), "Sal remembers a night at Dee's");
		Check(!W.Greeting(Sal, 1).empty(), "Sal has something to say");
	}
	std::printf("%s", W.Report().c_str());
	std::printf("%s", W.Describe(W.Find("gh0stfold")).c_str());
	return Failures;
}

int Years(int Count, uint32_t Seed)
{
	world::World W;
	W.Create(Seed, NightOne);
	std::printf("%s\n", W.Report().c_str());
	for (int Y = 1; Y <= Count; ++Y)
	{
		const auto T = std::chrono::steady_clock::now();
		W.Simulate(365);
		const double S = Seconds(T);
		std::string Save;
		W.Write(Save);
		std::printf("---- year %d: %.1fs simulated, save %zu KB\n%s", Y, S, Save.size() / 1024, W.Report().c_str());
		const std::array<double, 6>& F = W.Flows();
		std::printf("  money: prizes $%.0fK, buy-ins $%.0fK (net $%.0fK), deposits and pay $%.0fK, living $%.0fK, cash-outs $%.0fK, travel $%.0fK\n", F[0] / 1e3, F[1] / 1e3,
			(F[0] - F[1]) / 1e3, F[2] / 1e3, F[3] / 1e3, F[4] / 1e3, F[5] / 1e3);
		for (const auto& K : W.FlowsByKind())
		{
			std::printf("    %-22s %7.0f entries  prizes $%9.0fK  buy-ins $%9.0fK  roi %+.0f%%\n", K.first.c_str(), K.second[2], K.second[0] / 1e3, K.second[1] / 1e3,
				K.second[1] > 0 ? (K.second[0] / K.second[1] - 1.0) * 100.0 : 0.0);
		}
		W.ResetFlows();
		Invariants(W, "year " + std::to_string(Y));
		// Who's on top, what happened this year.
		std::map<int, int> Kinds;
		for (const world::WorldEvent& E : W.Events())
		{
			if (E.At >= W.Clock() - 365.0 * 1440.0)
			{
				++Kinds[static_cast<int>(E.Kind)];
			}
		}
		std::printf("  this year's events:");
		for (const auto& K : Kinds)
		{
			std::printf(" %s %d", world::EventKindName(static_cast<world::EventKind>(K.first)), K.second);
		}
		std::printf("\n  honors this year:\n");
		for (const world::Honor& H : W.Honors())
		{
			if (H.At >= W.Clock() - 365.0 * 1440.0 && (H.Title.find("Main Event") != std::string::npos || H.Title.find("Year") != std::string::npos || H.Title.find("Summit") != std::string::npos))
			{
				const world::Npc* N = H.Npc >= 0 ? W.Get(H.Npc) : nullptr;
				std::printf("    %d %s: %s (%s, skill %.2f) $%lld\n", H.Year, H.Title.c_str(), H.Name.c_str(), N ? world::IdentityName(N->Is) : H.Npc == -1 ? "you" : "unknown", N ? static_cast<double>(N->Overall()) : 0.0,
					static_cast<long long>(H.Prize / 100));
			}
		}
	}
	for (const char* Name : {"gh0stfold", "Mei", "Big Lou", "VikingVolta"})
	{
		std::printf("%s", W.Describe(W.Find(Name)).c_str());
	}
	return Failures;
}
} // namespace

int main(int Argc, char** Argv)
{
	if (Argc >= 3 && std::strcmp(Argv[1], "years") == 0)
	{
		return Years(std::atoi(Argv[2]), Argc >= 4 ? static_cast<uint32_t>(std::strtoul(Argv[3], nullptr, 10)) : 7u) > 0 ? 1 : 0;
	}
	if (Argc >= 3 && std::strcmp(Argv[1], "npc") == 0)
	{
		world::World W;
		W.Create(7u, NightOne);
		W.Simulate(Argc >= 4 ? std::atoi(Argv[3]) : 30);
		std::printf("%s", W.Describe(W.Find(Argv[2])).c_str());
		return 0;
	}
	const int F = Checks();
	std::printf(F == 0 ? "world: all checks passed\n" : "world: %d checks failed\n", F);
	return F == 0 ? 0 : 1;
}
