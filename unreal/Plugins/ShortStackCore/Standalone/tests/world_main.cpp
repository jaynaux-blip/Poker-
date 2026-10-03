// The living world on its own: invariants, save and load, and long runs.
//
//   world_test                     the checks (ctest)
//   world_test years N [seed]      plays N years and prints the world's health every year
//   world_test npc NAME [days]     plays some days and describes someone
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
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
		int Rings = 0;
		for (const world::Award& A : N.Awards)
		{
			Rings += A.Ring ? 1 : 0;
		}
		Check(Rings == N.Rings && static_cast<int>(N.Awards.size()) - Rings == N.Bracelets, When + ": " + N.Name + "'s trophy case holds every bracelet and ring");
		if (!N.Faded)
		{
			// The stats page: every tournament (online and live), every buy-in, every prize (and seats at their value).
			const world::Tracker& T = N.Stats;
			const world::Ledger& On = N.Totals[0];
			const world::Ledger& Lv = N.Totals[1];
			Check(T.Events == On.Events + Lv.Events && T.BuyIns == On.Spent + Lv.Spent && T.Prizes >= On.Won + Lv.Won, When + ": " + N.Name + "'s stats page adds up");
			int Lines = 0;
			for (const world::TrackLine& L : T.ByStake)
			{
				Lines += L.Events;
			}
			Check(Lines == T.Events && T.Cashes <= T.Events && T.Wins <= T.FinalTables && static_cast<int>(T.Curve.size()) < world::Tracker::CurveMax && T.Net == T.Prizes - T.BuyIns,
				When + ": " + N.Name + "'s stats page is consistent");
		}
	}
	Check(Active > 1000, When + ": the world emptied out (" + std::to_string(Active) + " active)");
	Check(W.View().size() == W.People().size(), When + ": the network view is out of step");
}

/** RiverLine's online calendar: a series in every season of every year, each name used once. */
void Calendar()
{
	const net::Network& Net = net::Shared();
	std::vector<const net::SeriesInfo*> All;
	std::set<std::string> SeriesNames;
	std::map<int, int> PerYear;
	for (const net::SeriesInfo& Sr : Net.Series())
	{
		All.push_back(&Sr);
		Check(SeriesNames.insert(Sr.Name).second, "a series name is used once: " + Sr.Name);
		++PerYear[world::YearOf(Sr.FirstDay)];
		const net::EventTemplate* Main = Net.FindTemplate(Sr.MainEvent);
		Check(Main && Main->Series == Sr.Id, Sr.Name + " has its Main Event");
		int Events = 0;
		int Awards = 0;
		for (const net::EventTemplate& T : Net.Templates())
		{
			if (T.Series == Sr.Id)
			{
				++Events;
				Awards += T.Bracelet || T.Ring ? 1 : 0;
				Check(T.OnlyDay >= Sr.FirstDay && T.OnlyDay <= Sr.LastDay, T.Name + " is inside its series");
			}
		}
		// (2026's three were written by hand: Summer Slam's events are history, only its Main Event is kept.)
		Check(Events == Sr.Events || Sr.Id == "mm" || Sr.Id == "rcop" || Sr.Id == "slam", Sr.Name + " lists its events");
		Check(Awards == Sr.Bracelets + Sr.Rings, Sr.Name + " counts its rings and bracelets");
		if (Sr.Short == "TCO")
		{
			Check(Sr.Bracelets >= 30, Sr.Name + " has its bracelet events");
		}
		if (Sr.Short == "RING")
		{
			Check(Sr.Rings >= 20, Sr.Name + " has its ring events");
		}
	}
	for (int Y = 2027; Y <= 2040; ++Y)
	{
		Check(PerYear[Y] == 9, "nine series in " + std::to_string(Y));
	}
	Check(PerYear[2026] >= 4, "December 2026 has a series of its own");
	std::sort(All.begin(), All.end(), [](const net::SeriesInfo* A, const net::SeriesInfo* B) { return A->FirstDay < B->FirstDay; });
	for (size_t K = 1; K < All.size(); ++K)
	{
		Check(All[K]->FirstDay > All[K - 1]->LastDay, All[K]->Name + " doesn't overlap " + All[K - 1]->Name);
	}
	// Every event in every series has a name of its own; every template its own id.
	std::set<std::string> Names;
	std::set<std::string> Ids;
	int SeriesEvents = 0;
	for (const net::EventTemplate& T : Net.Templates())
	{
		Check(Ids.insert(T.Id).second, "a template id is used once: " + T.Id);
		if (!T.Series.empty())
		{
			++SeriesEvents;
			Check(Names.insert(T.Name).second, "an event name is used once: " + T.Name);
		}
	}
	// Seats into the RCOP Main carry over to every year's Main Event.
	Check(Net.TicketOf("rcop27-main") == "rcop-main", "every RCOP Main takes an RCOP Main seat");
	net::EventInstance Next;
	Check(Net.NextFor("rcop-main", static_cast<double>(world::YearStart(2027)) * net::MinutesPerDay, Next) && Net.TemplateOf(Next).Id == "rcop27-main",
		"a seat won in 2027 is for RCOP 2027");
	Check(net::DateLabel(world::DayOn(2028, 2, 29)) == "Feb 29" && net::DateLabel(world::DayOn(2028, 3, 1)) == "Mar 1", "leap days are on the calendar");
	std::printf("calendar: %zu series, %d series events, %zu templates\n", All.size(), SeriesEvents, Net.Templates().size());
}

int Checks()
{
	Calendar();
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
	// Newcomers: new names keep arriving, each with a story and a journey that starts the day they joined.
	{
		int Newcomers = 0;
		std::set<int> Kinds;
		for (const world::Npc& N : W.People())
		{
			if (N.Came == world::Arrival::None)
			{
				continue;
			}
			++Newcomers;
			Kinds.insert(static_cast<int>(N.Came));
			Check(!N.Path.empty() && N.Path.front().Kind == world::StepKind::Joined && N.Path.front().Day == N.Arrived, N.Name + "'s journey starts the day they joined");
			const world::Profile P = W.ProfileOf(N.Id);
			Check(!P.Came.empty() && !P.Journey.empty() && P.Arrived == N.Arrived, N.Name + "'s card tells how they got here");
		}
		Check(Newcomers >= 10 && Kinds.size() >= 4, "newcomers arrive, in different ways (" + std::to_string(Newcomers) + ")");
		const world::Profile Rival = W.ProfileOf(W.Find("gh0stfold"));
		Check(!Rival.Came.empty() && !Rival.Journey.empty() && Rival.Arrived < 0, "an old hand's card starts before the story did");
	}

	// Save and load: a world that's saved carries on exactly as one that isn't (the trophy cases too).
	W.GrantAward(-1, false, true);
	W.GrantAward(W.Find("Mei"), true, false);
	Check(W.HeroAwards().size() == 1 && W.HeroAwards().front().Main && !W.HeroAwards().front().Ring && !W.HeroAwards().front().Series.empty(), "the player's bracelet is in their trophy case");
	Check(!W.Get(W.Find("Mei"))->Awards.empty() && W.Get(W.Find("Mei"))->Awards.back().Ring, "a ring for Mei");
	std::string Saved;
	W.Write(Saved);
	std::printf("save: %zu KB\n", Saved.size() / 1024);
	Check(Saved.size() < 4u * 1024u * 1024u, "the save stays reasonable");
	world::World Copy;
	Load(Copy, Saved);
	std::string Again;
	Copy.Write(Again);
	Check(Again == Saved, "a loaded world writes the same save");
	Check(Copy.HeroAwards().size() == 1 && Copy.HeroAwards().front().Series == W.HeroAwards().front().Series, "the player's trophy case loads");
	{
		// A save from before the trophy cases (counts only): the titles in history fill them in.
		std::string Old;
		size_t At = 0;
		while (At < Saved.size())
		{
			const size_t End = Saved.find('\n', At);
			const std::string Ln = Saved.substr(At, End == std::string::npos ? std::string::npos : End - At + 1);
			if (Ln.rfind("world\tawards\t", 0) != 0 && Ln.rfind("world\theroawards\t", 0) != 0)
			{
				Old += Ln;
			}
			At = End == std::string::npos ? Saved.size() : End + 1;
		}
		world::World Before;
		Load(Before, Old);
		Invariants(Before, "an older save");
	}
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
	// Every online bracelet that has been played for is in the history books too.
	for (const net::SeriesInfo& Sr : net::Shared().Series())
	{
		if (Sr.Bracelets == 0 || static_cast<double>(Sr.LastDay + 3) * net::MinutesPerDay > W.Clock() || static_cast<double>(Sr.FirstDay) * net::MinutesPerDay < W.StartedAt())
		{
			continue;
		}
		int Won = 0;
		for (const world::Honor& H : W.Honors())
		{
			const net::EventTemplate* T = net::Shared().FindTemplate(H.EventId.substr(0, H.EventId.find('@')));
			Won += T && T->Series == Sr.Id && T->Bracelet ? 1 : 0;
		}
		Check(Won == Sr.Bracelets, Sr.Name + ": every bracelet has a winner (" + std::to_string(Won) + " of " + std::to_string(Sr.Bracelets) + ")");
	}
	// Every big one that has finished has a champion in the history books.
	for (int Day = static_cast<int>(W.StartedAt() / 1440.0); Day < static_cast<int>(W.Clock() / 1440.0); ++Day)
	{
		for (const world::LiveEvent& E : world::LiveCalendar(Day))
		{
			if ((E.Kind == world::LiveKind::Summit || E.Kind == world::LiveKind::ChampionshipMain) && E.Start + E.Duration + 2.0 * 1440.0 < W.Clock())
			{
				bool Crowned = false;
				for (const world::Honor& H : W.Honors())
				{
					Crowned = Crowned || H.EventId == E.Id;
				}
				Check(Crowned, E.Name + " has a champion");
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
