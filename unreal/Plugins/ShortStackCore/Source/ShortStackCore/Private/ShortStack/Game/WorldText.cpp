// The living world in words, for the debug tools: one person inside out, and the world's health at a glance.
#include "ShortStack/Game/World.h"

#include "ShortStack/Game/Format.h"
#include "WorldSim.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ss
{
namespace world
{
namespace worldtext_detail
{
std::string Pct(double V)
{
	return Fixed(V * 100.0, 1) + "%";
}

const char* VenueName(int V)
{
	return V == 0 ? "Online" : V == 1 ? "Live" : "Underground";
}

const char* LiveName(LiveLevel L)
{
	switch (L)
	{
	case LiveLevel::None: return "none";
	case LiveLevel::Local: return "local";
	case LiveLevel::Regional: return "regional";
	case LiveLevel::Circuit: return "circuit";
	case LiveLevel::Championship: return "championship";
	case LiveLevel::HighRoller: return "high roller";
	}
	return "";
}

const char* OriginName(Origin O)
{
	switch (O)
	{
	case Origin::Founding: return "network regular";
	case Origin::Cast: return "Riverside / Back Room";
	case Origin::Directory: return "Kast streamer";
	case Origin::Rookie: return "newcomer";
	case Origin::Discovered: return "discovered";
	}
	return "";
}

double Percentile(std::vector<double> V, double P)
{
	if (V.empty())
	{
		return 0.0;
	}
	std::sort(V.begin(), V.end());
	const size_t K = static_cast<size_t>(sim::Clamp(P, 0.0, 1.0) * static_cast<double>(V.size() - 1));
	return V[K];
}
} // namespace worldtext_detail

using namespace worldtext_detail;

std::string World::Describe(int Id) const
{
	const Npc* P = Get(Id);
	if (!P)
	{
		return "no such person\n";
	}
	const Npc& N = *P;
	const int Day = sim::DayAt(Now);
	std::ostringstream O;
	O << "#" << N.Id << " " << N.Name << " (" << N.Country << ", " << N.Age(Day) << ")  " << IdentityName(N.Is) << " - started as " << IdentityName(N.Began) << ", " << OriginName(N.From)
	  << "\n";
	O << "  status " << StatusName(N.St);
	if (N.St == Status::Break)
	{
		O << " until " << net::DateLabel(N.Until);
	}
	O << "  mood " << MomentumName(N.Mood) << " (since " << net::DateLabel(N.MoodSince) << ")  confidence " << Fixed(static_cast<double>(N.Confidence), 2) << "  fatigue "
	  << Fixed(static_cast<double>(N.Fatigue), 2) << "  tilt " << Fixed(static_cast<double>(N.Tilt), 2) << "\n";
	O << "  bankroll " << Money(N.Bankroll) << " (peak " << Money(N.PeakRoll) << ")  " << (N.Professional ? "professional, living " + Money(N.Living) + "/wk" : "job " + Money(N.Income) + "/wk");
	if (N.Professional && N.Income > 0)
	{
		O << ", income " << Money(N.Income) << "/wk";
	}
	if (N.Backer != -1)
	{
		O << "  backed by " << (N.Backer >= 0 ? Roster[static_cast<size_t>(N.Backer)].Name : std::string("a staking stable")) << " (makeup " << Money(N.Makeup) << ")";
	}
	O << "\n  stakes " << net::TierName(static_cast<net::Tier>(sim::TierIndex(N.Tier))) << " (comfortable at " << Money(Sim::Comfort(N)) << ")  live " << LiveName(N.Live)
	  << "  plays " << PlanName(N.Schedule) << ", " << Pct(static_cast<double>(N.OnlineShare)) << " online";
	if (N.TripUntil >= Day)
	{
		O << "  trip: " << N.Trip << " " << net::DateLabel(N.TripFrom) << "-" << net::DateLabel(N.TripUntil);
	}
	O << "\n  skill " << Fixed(static_cast<double>(N.Overall()), 3) << " (peak " << Fixed(static_cast<double>(N.Peak), 2) << ", potential " << Fixed(static_cast<double>(N.Potential), 2)
	  << "):";
	for (int K = 0; K < SkillCount; ++K)
	{
		O << (K % 5 == 0 ? "\n    " : "  ") << SkillName(static_cast<Skill>(K)) << " " << Fixed(static_cast<double>(N.Skills[static_cast<size_t>(K)]), 2);
	}
	static const char* const Traits[TraitCount] = {"discipline", "ambition", "risk", "patience", "sociability", "grit", "aggression"};
	O << "\n  traits:";
	for (int K = 0; K < TraitCount; ++K)
	{
		O << " " << Traits[K] << " " << Fixed(static_cast<double>(N.Traits[static_cast<size_t>(K)]), 2);
	}
	O << "\n  reputation:";
	for (int K = 0; K < RepCount; ++K)
	{
		O << " " << RepName(static_cast<Rep>(K)) << " " << static_cast<int>(std::lround(static_cast<double>(N.Reps[static_cast<size_t>(K)])));
	}
	if (N.Streams || N.Followers > 0)
	{
		O << "\n  streaming " << (N.Streams ? "on" : "stopped") << ", " << Grouped(N.Followers) << " followers";
	}
	if (!N.Sponsor.empty())
	{
		O << "  sponsor " << N.Sponsor;
	}
	O << "\n  career " << Money(N.CareerWon()) << "  bracelets " << N.Bracelets << "  rings " << N.Rings << "  series titles " << N.Titles << "  majors " << N.Majors << "\n";
	for (int V = 0; V < VenueCount; ++V)
	{
		const Ledger& L = N.Totals[static_cast<size_t>(V)];
		if (L.Events == 0)
		{
			continue;
		}
		O << "    " << VenueName(V) << ": " << Grouped(L.Events) << " events, won " << Money(L.Won) << ", spent " << Money(L.Spent) << ", " << L.Cashes << " cashes, " << L.FinalTables
		  << " final tables, " << L.Wins << " wins, best " << Money(L.Best) << "\n";
	}
	if (!N.BestEvent.empty())
	{
		O << "  best: " << N.BestEvent << " (" << net::DateLabel(N.BestDay) << ")\n";
	}
	O << "  season " << N.ThisSeason.Year << ": " << Fixed(N.ThisSeason.Points, 0) << " pts, " << Money(N.ThisSeason.Won) << ", " << N.ThisSeason.Wins << " wins, live "
	  << Fixed(N.ThisSeason.LivePoints, 0) << " pts\n";
	for (const Year& Y : N.Years)
	{
		O << "    " << Y.Number << ": online " << Money(Y.Online) << ", live " << Money(Y.Live) << ", net " << Money(Y.Net) << ", " << Y.Wins << " wins in " << Y.Events << "\n";
	}
	if (!N.Recent.empty())
	{
		O << "  recent:\n";
		for (const world::Finish& F : N.Recent)
		{
			O << "    " << net::DateLabel(F.Day) << " " << Ordinal(F.Place) << " of " << Grouped(F.Entries) << " " << F.Event << " " << Money(F.Prize) << "\n";
		}
	}
	if (!N.Ties.empty())
	{
		O << "  knows:";
		for (const Tie& T : N.Ties)
		{
			const Npc* Other = Get(T.Other);
			O << " " << TieName(T.Kind) << " " << (Other ? Other->Name : "?") << " (" << Fixed(static_cast<double>(T.Strength), 2) << ")";
		}
		O << "\n";
	}
	if (!N.Tickets.empty())
	{
		O << "  tickets:";
		for (const auto& T : N.Tickets)
		{
			O << " " << T.first << " x" << T.second;
		}
		O << "\n";
	}
	if (const Bond* B = BondWith(N.Id))
	{
		O << "  about you: " << (B->Label().empty() ? "a stranger" : B->Label()) << " (familiar " << Fixed(static_cast<double>(B->Familiarity), 2) << ", respect "
		  << Fixed(static_cast<double>(B->Respect), 2) << ", trust " << Fixed(static_cast<double>(B->Trust), 2) << ", rivalry " << Fixed(static_cast<double>(B->Rivalry), 2) << ", resentment "
		  << Fixed(static_cast<double>(B->Resentment), 2) << ")\n";
		for (const Memory& M : B->Memories)
		{
			O << "    " << net::DateLabel(M.Day) << " " << MemoryText(M.Kind) << " (" << M.Where << ")\n";
		}
	}
	// What they're registered for.
	int Shown = 0;
	for (const Pending& Q : Queue)
	{
		for (const Entry& E : Q.Who)
		{
			if (E.Npc == N.Id && Shown < 6)
			{
				O << (Shown == 0 ? "  playing: " : ", ") << Q.Name << " " << net::TimeLabel(Q.Start);
				++Shown;
			}
		}
	}
	if (Shown > 0)
	{
		O << "\n";
	}
	int Story = 0;
	for (auto It = Log.rbegin(); It != Log.rend() && Story < 8; ++It)
	{
		if (It->Npc == N.Id || It->Other == N.Id)
		{
			O << (Story == 0 ? "  history:\n" : "") << "    " << net::DateLabel(sim::DayAt(It->At)) << " " << YearOf(sim::DayAt(It->At)) << " " << EventKindName(It->Kind) << " " << It->What
			  << (It->Amount ? " " + Money(It->Amount) : "") << "\n";
			++Story;
		}
	}
	return O.str();
}

std::string World::Report() const
{
	const int Day = sim::DayAt(Now);
	std::ostringstream O;
	O << "World " << WorldSeed << " on " << net::DateLabel(Day) << " " << YearOf(Day) << " (day " << Day << ", " << Fixed((Now - Origin) / 1440.0 / 365.0, 2) << " years in)\n";
	int ByStatus[4] = {0, 0, 0, 0};
	int ByTier[5] = {0, 0, 0, 0, 0};
	int ByMood[static_cast<int>(Momentum::Count)] = {};
	int ByIdentity[static_cast<int>(Identity::Count)] = {};
	int ByOrigin[5] = {0, 0, 0, 0, 0};
	int ByLive[6] = {0, 0, 0, 0, 0, 0};
	int Pros = 0;
	int Streamers = 0;
	int Backed = 0;
	int Elite = 0;
	std::vector<double> Rolls[5];
	std::vector<double> Skills;
	std::vector<double> Ages;
	Chips Total = 0;
	for (const Npc& N : Roster)
	{
		++ByStatus[static_cast<int>(N.St)];
		++ByOrigin[static_cast<int>(N.From)];
		if (!N.Playing())
		{
			continue;
		}
		const int T = sim::TierIndex(N.Tier);
		++ByTier[T];
		++ByMood[static_cast<int>(N.Mood)];
		++ByIdentity[static_cast<int>(N.Is)];
		++ByLive[static_cast<int>(N.Live)];
		Pros += N.Professional ? 1 : 0;
		Streamers += N.Streams ? 1 : 0;
		Backed += N.Backer != -1 ? 1 : 0;
		Elite += N.Overall() >= 0.75f ? 1 : 0;
		Rolls[T].push_back(sim::Dollars(N.Bankroll));
		Skills.push_back(static_cast<double>(N.Overall()));
		Ages.push_back(static_cast<double>(N.Age(Day)));
		Total += N.Bankroll;
	}
	O << "  people " << Roster.size() << ": active " << ByStatus[0] << ", break " << ByStatus[1] << ", broke " << ByStatus[2] << ", retired " << ByStatus[3] << "  (target " << Target << ")\n";
	O << "  origins: founding " << ByOrigin[0] << ", cast " << ByOrigin[1] << ", streamers " << ByOrigin[2] << ", newcomers " << ByOrigin[3] << ", discovered " << ByOrigin[4] << "\n";
	O << "  active by stakes: micro " << ByTier[1] << ", low " << ByTier[2] << ", mid " << ByTier[3] << ", high " << ByTier[4] << "   live: none " << ByLive[0] << ", local " << ByLive[1]
	  << ", regional " << ByLive[2] << ", circuit " << ByLive[3] << ", championship " << ByLive[4] << ", high roller " << ByLive[5] << "\n";
	O << "  professionals " << Pros << ", streamers " << Streamers << ", backed " << Backed << ", skill >= .75: " << Elite << "\n";
	O << "  skill: median " << Fixed(Percentile(Skills, 0.5), 3) << ", 10% " << Fixed(Percentile(Skills, 0.1), 3) << ", 90% " << Fixed(Percentile(Skills, 0.9), 3) << ", max "
	  << Fixed(Percentile(Skills, 1.0), 3) << "   age median " << Fixed(Percentile(Ages, 0.5), 0) << "\n";
	O << "  bankrolls (median / 90%):";
	static const char* const Tiers[5] = {"", "micro", "low", "mid", "high"};
	for (int T = 1; T < 5; ++T)
	{
		O << "  " << Tiers[T] << " " << Money(sim::Cents(Percentile(Rolls[T], 0.5))) << " / " << Money(sim::Cents(Percentile(Rolls[T], 0.9)));
	}
	O << "\n  money in active bankrolls " << Money(Total) << "\n";
	O << "  field strength:";
	for (int T = 1; T < 5; ++T)
	{
		O << " " << Tiers[T] << " " << Fixed(Field[static_cast<size_t>(T)], 3);
	}
	O << "\n  moods:";
	for (int K = 0; K < static_cast<int>(Momentum::Count); ++K)
	{
		O << " " << MomentumName(static_cast<Momentum>(K)) << " " << ByMood[K];
	}
	O << "\n  identities:";
	for (int K = 0; K < static_cast<int>(Identity::Count); ++K)
	{
		if (ByIdentity[K] > 0)
		{
			O << " " << IdentityName(static_cast<Identity>(K)) << " " << ByIdentity[K] << ";";
		}
	}
	O << "\n  reputation leaders:";
	for (int Id : Leaders(Rep::Overall, 5))
	{
		const Npc& N = Roster[static_cast<size_t>(Id)];
		O << " " << N.Name << " " << static_cast<int>(std::lround(static_cast<double>(N.RepOf(Rep::Overall))));
	}
	O << "\n  events logged " << Log.size() << ", honors " << Titles.size() << ", results kept " << Results.size() << ", queued " << Queue.size() << "\n";
	return O.str();
}
} // namespace world
} // namespace ss
