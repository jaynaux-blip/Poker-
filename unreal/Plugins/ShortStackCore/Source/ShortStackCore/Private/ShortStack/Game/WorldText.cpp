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

std::string Short(Chips C)
{
	return net::MoneyShort(C);
}

std::string StyleOf(const Npc& N)
{
	const float Aggro = N.TraitOf(Trait::Aggro);
	const float Patience = N.TraitOf(Trait::Patience);
	if (Aggro > 0.8f && N.TraitOf(Trait::Risk) > 0.75f)
	{
		return "Wild: raises everything";
	}
	if (Aggro > 0.62f)
	{
		return Patience > 0.55f ? "Tight and aggressive" : "Loose and aggressive";
	}
	if (Aggro < 0.3f)
	{
		return Patience > 0.6f ? "Tight and patient" : "Calls a lot";
	}
	return Patience > 0.6f ? "Solid and patient" : "Balanced";
}

std::string LiveText(const Npc& N)
{
	switch (N.Live)
	{
	case LiveLevel::None: return "Online only";
	case LiveLevel::Local: return "Plays the local weeklies";
	case LiveLevel::Regional: return "Plays regional festivals";
	case LiveLevel::Circuit: return "Travels the Grand Circuit";
	case LiveLevel::Championship: return "Plays the Championship";
	case LiveLevel::HighRoller: return "Plays the high rollers";
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

Profile World::ProfileOf(int Id) const
{
	Profile Pr;
	const Npc* P = Get(Id);
	if (!P)
	{
		return Pr;
	}
	const Npc& N = *P;
	const int Day = sim::DayAt(Now);
	Pr.Id = N.Id;
	Pr.Name = N.Name;
	Pr.Country = N.Country;
	Pr.Age = N.Age(Day);
	Pr.Known = IdentityName(N.Is);
	Pr.Status = N.St == Status::Broke ? "Away from the tables" : StatusName(N.St);
	Pr.Stakes = std::string(net::TierName(static_cast<net::Tier>(sim::TierIndex(N.Tier)))) + " stakes online";
	Pr.Live = LiveText(N);
	Pr.Style = StyleOf(N);
	Pr.Since = YearOf(N.Joined);
	for (int K = 0; K < RepCount; ++K)
	{
		Pr.Reps[static_cast<size_t>(K)] = static_cast<int>(std::lround(static_cast<double>(N.Reps[static_cast<size_t>(K)])));
	}
	Pr.Totals = N.Totals;
	for (Ledger& L : Pr.Totals)
	{
		L.Spent = 0;
	}
	Pr.Bracelets = N.Bracelets;
	Pr.Rings = N.Rings;
	Pr.Titles = N.Titles;
	Pr.Majors = N.Majors;
	Pr.BestEvent = N.BestEvent;
	Pr.Best = std::max({N.Totals[0].Best, N.Totals[1].Best, N.Totals[2].Best});
	Pr.BestDay = N.BestDay;
	Pr.Recent = N.Recent;
	Pr.Years = N.Years;
	for (Year& Y : Pr.Years)
	{
		Y.Net = 0;
	}
	Pr.Form = N.Weekly;
	Pr.ThisSeason = N.ThisSeason;
	Pr.Streams = N.Streams;
	Pr.Followers = N.Followers;
	Pr.Sponsor = N.Sponsor;
	Pr.Pro = N.Pro;
	Pr.Rival = N.Rival;
	for (const Tie& T : N.Ties)
	{
		// Who backs whom stays between them.
		if (T.Kind != TieKind::Backer && T.Kind != TieKind::Backed && T.Strength >= 0.2f)
		{
			if (const Npc* O = Get(T.Other))
			{
				Pr.Knows.push_back({TieName(T.Kind), O->Name});
			}
		}
	}
	if (const Bond* B = BondWith(N.Id))
	{
		Pr.Bond = B->Label();
		for (auto It = B->Memories.rbegin(); It != B->Memories.rend(); ++It)
		{
			Pr.Memories.push_back(net::DateLabel(It->Day) + ", " + std::to_string(YearOf(It->Day)) + ": " + MemoryText(It->Kind) + (It->Where.empty() ? "" : " (" + It->Where + ")"));
		}
	}
	for (auto It = Log.rbegin(); It != Log.rend() && Pr.Story.size() < 8; ++It)
	{
		std::string Title;
		std::string Body;
		std::string Tag;
		if ((It->Npc == N.Id || It->Other == N.Id) && Headline(*It, Title, Body, Tag))
		{
			Pr.Story.push_back(net::DateLabel(sim::DayAt(It->At)) + ", " + std::to_string(YearOf(sim::DayAt(It->At))) + ": " + Title);
		}
	}
	return Pr;
}

bool World::Headline(const WorldEvent& E, std::string& Title, std::string& Body, std::string& Tag) const
{
	const Npc* N = Get(E.Npc);
	if (!N)
	{
		return false;
	}
	const Npc* O = Get(E.Other);
	const std::string& Who = N->Name;
	const bool Known = N->RepOf(Rep::Overall) >= 15.0f || N->Anchored || N->Pro || N->Streams;
	const bool Live = (E.Flags & FlagLive) != 0;
	const int Day = sim::DayAt(E.At);
	switch (E.Kind)
	{
	case EventKind::Won:
	case EventKind::Champion:
	{
		const bool Qualifier = (E.Flags & FlagQualifier) != 0;
		Title = Who + " wins " + E.What;
		Body = Grouped(E.Value) + " entries Â· " + Money(E.Amount) + " to the winner.";
		if (Qualifier)
		{
			Body = "From a satellite seat to the title: " + Body;
		}
		if (O)
		{
			Body += " Runner-up: " + O->Name + ".";
		}
		if ((E.Flags & FlagFirst) != 0)
		{
			Body += " A first title.";
		}
		Tag = E.Kind == EventKind::Champion ? "CHAMPION" : Live ? "LIVE" : "BIG WIN";
		return true;
	}
	case EventKind::FinalTable:
		Title = Who + " makes the final table of " + E.What;
		Body = Ordinal(E.Value) + " place, " + Money(E.Amount) + ".";
		Tag = Live ? "LIVE" : "FINAL TABLE";
		return true;
	case EventKind::Discovered:
		Title = "Unknown " + Who + " wins " + E.What;
		Body = std::string((E.Flags & FlagQualifier) != 0 ? "A satellite qualifier nobody had heard of" : "A name nobody had heard of") + " takes " + Money(E.Amount) + " from " + Grouped(E.Value) +
			" entries.";
		Tag = "NEW NAME";
		return true;
	case EventKind::Breakout:
		Title = Who + "'s breakout";
		Body = Money(E.Amount) + " for " + Ordinal(E.Value) + " in " + E.What + ": the score of a career so far.";
		Tag = "BREAKOUT";
		return true;
	case EventKind::Qualified:
		if (!Known)
		{
			return false;
		}
		Title = Who + " wins a seat to the " + E.What;
		Body = "A " + Money(E.Amount) + " seat from a satellite.";
		Tag = "QUALIFIED";
		return true;
	case EventKind::MovedUp:
		if (!Known && E.Value < 4)
		{
			return false;
		}
		Title = Who + " moves up to " + E.What + " stakes";
		Body = "A new level, new regulars, bigger swings.";
		Tag = "MOVES";
		return true;
	case EventKind::MovedDown:
		if (!Known)
		{
			return false;
		}
		Title = Who + " steps down to " + E.What + " stakes";
		Body = "Rebuilding after a rough stretch.";
		Tag = "MOVES";
		return true;
	case EventKind::Retired:
		Title = Who + " retires";
		Body = "After " + std::to_string(std::max(1, (Day - N->Joined) / 365)) + " years and " + Short(E.Amount) + " in winnings, " + Who + " steps away from the game.";
		Tag = "RETIRED";
		return true;
	case EventKind::Returned:
		Title = Who + " is back";
		Body = E.What == "retired" ? "Out of retirement after " + std::to_string(std::max(1, E.Value / 30)) + " months away."
			: E.What == "broke" ? "Back at the tables after time away."
			: E.Value >= 14 ? "Back at the tables after " + std::to_string(E.Value / 7) + " weeks away." : "Back at the tables.";
		Tag = "COMEBACK";
		return Known || E.What == "retired";
	case EventKind::Break:
		if (!Known)
		{
			return false;
		}
		Title = Who + " takes a break";
		Body = "Stepping away for a while.";
		Tag = "AWAY";
		return true;
	case EventKind::Sponsored:
		Title = Who + " joins " + E.What;
		Body = E.What == "Team RiverLine" ? "The site's newest sponsored pro." : "A sponsorship deal for the Kast channel.";
		Tag = "SPONSOR";
		return true;
	case EventKind::Milestone:
		if (E.What == "career-won")
		{
			Title = Who + " passes " + Short(E.Amount) + " in career winnings";
			Body = "A milestone few reach.";
		}
		else if (E.What == "first-title")
		{
			Title = Who + " wins a first major title";
			Body = Money(E.Amount) + ".";
		}
		else if (E.What == "turned-pro")
		{
			if (!Known)
			{
				return false;
			}
			Title = Who + " goes pro";
			Body = "The day job is over.";
		}
		else
		{
			return false;
		}
		Tag = "MILESTONE";
		return true;
	case EventKind::StartedStreaming:
		Title = Who + " starts streaming on Kast";
		Body = "Another regular goes live.";
		Tag = "KAST";
		return Known;
	case EventKind::StreamMilestone:
		Title = Who + " reaches " + Grouped(E.Value) + " followers on Kast";
		Body = "The channel keeps growing.";
		Tag = "KAST";
		return true;
	case EventKind::Debut:
		Title = "A new name: " + Who;
		Body = "Fresh on the scene and already turning heads.";
		Tag = "NEW FACE";
		return true;
	case EventKind::Rivalry:
		if (!O)
		{
			return false;
		}
		Title = Who + " vs " + O->Name;
		Body = "They keep meeting heads-up. Neither is letting it go.";
		Tag = "RIVALRY";
		return true;
	case EventKind::PlayerOfYear:
		Title = Who + " is " + E.What + " " + std::to_string(YearOf(Day));
		Body = Grouped(E.Value) + " points.";
		Tag = "PLAYER OF THE YEAR";
		return true;
	default: return false; // going broke, taking a stake: nobody's business
	}
}

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
