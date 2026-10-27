// Update 3 (SPEC §6.5, §7.3): the world's creators on Rerun (Home, Next up, Research), core-videocontent's file: their
// videos from the world log and the calendar, telling only the world's truth (U5): a fresh world shows only what happened.
//
// P0's channels: the 12 Kast directory creators (kast::Directory()) and Overlay Weekly, a weekly poker news show built from
// the world log. A creator's uploads are (a) a reaction to a world event the log holds (a major's champion or final table,
// the Player of the Year race, the Hall of Fame, a rival site's lobby or overlay, the Season board, a rivalry), each event
// taken by one creator, titled by the same engine as the player's (the news ideas' rows, H1-H9) with its facts, and (b) a
// weekly show of their own (words from their Kast stream, no numbers). Views are deterministic: followers x 0.03 x
// Luck(Fnv1a(channel | video)) on their own curve. Nothing is saved; a day's list is cached until the day or the log moves.
#include "ShortStack/Game/VideoContent.h"

#include "VideoContentPriv.h"

#include "ShortStack/Common.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ss
{
namespace video
{
namespace vcontent
{
namespace
{
constexpr double VwDay = 1440.0;
constexpr int VwWindowDays = 28; // what Home and Research show: the last four weeks
constexpr int VwOverlayFollowers = 52000;

/** A creator's weekly show of their own (no numbers: nothing to be true about but the stream). */
const std::vector<const char*>& VwShows(const std::string& Name)
{
	static const std::vector<std::pair<const char*, std::vector<const char*>>> All = {
		{"VikingVolta", {"the bankroll challenge: this week's best hands", "I played the biggest tournaments of the week", "my worst beat of the week, and my best"}},
		{"HighRollerHana", {"high roller Thursday: every big pot", "Sunday at the high roller tables", "the high roller hand I can't stop thinking about"}},
		{"MissFinch", {"every spot explained: a Sunday grind", "the hands I'd play differently this week", "a calm session, and every decision in it"}},
		{"BluffSquadTV", {"three pros, twelve tables, one long night", "the crew's best bluffs of the week", "who busted first? a night with the squad"}},
		{"SuitedConnor", {"micro to mid: this week's progress", "why I'm not moving up yet", "the bankroll rules I didn't break this week"}},
		{"LaReinaDelRio", {"a week of tournament grinding", "the final table I almost made", "late night tournaments and a deep run"}},
		{"KatOnTheRiver", {"bounties only: the week's best knockouts", "PKO night with chat", "the bounty I should never have chased"}},
		{"ThePokerMonk", {"a silent study session", "one hand, one hour of study", "a quiet night of tournaments"}},
		{"NitNation", {"I folded for six hours. here's the one hand I played", "the tightest session of the week", "chat counted my folds again"}},
		{"chipleader_carla", {"road to the Sunday Major", "this week's tournaments, ranked", "the Sunday I've been waiting for"}},
		{"FoldEquityFred", {"ICM homework: the bubble, explained", "push or fold: this week's quiz", "beginner questions, honest answers"}},
		{"gh0stfold", {"never logs off.", "every pot this week.", "the tables are quiet tonight."}},
	};
	static const std::vector<const char*> None;
	for (const auto& Kv : All)
	{
		if (Name == Kv.first)
		{
			return Kv.second;
		}
	}
	return None;
}

/** The idea whose look a creator's own show wears (its thumbnail's archetypes). */
int VwShowIdea(const std::string& Name)
{
	if (Name == "MissFinch" || Name == "ThePokerMonk")
	{
		return 601;
	}
	if (Name == "FoldEquityFred")
	{
		return 603;
	}
	if (Name == "KatOnTheRiver")
	{
		return 209;
	}
	if (Name == "VikingVolta" || Name == "SuitedConnor")
	{
		return 1601;
	}
	if (Name == "NitNation")
	{
		return 612;
	}
	if (Name == "gh0stfold")
	{
		return 1202;
	}
	return Name == "HighRollerHana" || Name == "chipleader_carla" ? 202 : 201;
}

std::string VwNpc(const world::World& W, int Id)
{
	const world::Npc* N = W.Get(Id);
	return N ? N->Name : std::string();
}

/** A news event's idea and facts (the H rows'), or 0 when the log line isn't news a channel would cover. */
int VwNews(const world::World& W, const world::WorldEvent& E, std::string& Facts)
{
	using world::EventKind;
	const std::string Who = VwNpc(W, E.Npc), Other = VwNpc(W, E.Other);
	auto Kv = [](const char* K, const std::string& V) { return V.empty() ? std::string() : std::string(K) + "=" + FactEsc(V) + ";"; };
	const int Day = static_cast<int>(std::floor(E.At / VwDay));
	const std::string Site = world::FlagSiteOf(E.Flags) != net::SiteId::RiverLine ? net::Def(world::FlagSiteOf(E.Flags)).Name : std::string();
	switch (E.Kind)
	{
	case EventKind::Champion:
		if (Who.empty() || E.What.empty())
		{
			return 0;
		}
		Facts = Kv("Winner", Who) + Kv("Major", E.What) + (E.Amount > 0 ? Kv("Prize", Cash(E.Amount)) : std::string());
		return 801;
	case EventKind::FinalTable:
		if (Who.empty() || E.What.empty())
		{
			return 0;
		}
		Facts = Kv("Npc", Who) + Kv("Major", E.What);
		return 802;
	case EventKind::PlayerOfYear:
		if (Who.empty())
		{
			return 0;
		}
		Facts = Kv("Leader", Who) + Kv("Year", std::to_string(world::YearOf(Day)));
		return 803;
	case EventKind::HallOfFame:
		if (Who.empty())
		{
			return 0;
		}
		Facts = Kv("Npc", Who) + Kv("Year", std::to_string(E.Value > 0 ? E.Value : world::YearOf(Day)));
		return 804;
	case EventKind::SiteLaunched:
		if (Site.empty())
		{
			return 0;
		}
		Facts = Kv("Site", Site);
		return 805;
	case EventKind::SiteOverlay:
		if (Site.empty() || E.What.empty() || E.Amount <= 0)
		{
			return 0;
		}
		Facts = Kv("Site", Site) + Kv("Event", E.What) + Kv("Overlay", Cash(E.Amount));
		return 807;
	case EventKind::BoardReport:
		if (Who.empty())
		{
			return 0;
		}
		Facts = Kv("Leader", Who);
		return 808;
	case EventKind::Rivalry:
		if (Who.empty() || Other.empty())
		{
			return 0;
		}
		Facts = Kv("Npc", Who) + Kv("Other", Other);
		return 809;
	default: break;
	}
	return 0;
}

/** Overlay Weekly's words for a log line ("the overlay", "the Sunday Blaze", "the Season race"), "" when it isn't news. */
std::string VwHeadline(const world::World& W, const world::WorldEvent& E)
{
	using world::EventKind;
	switch (E.Kind)
	{
	case EventKind::Champion: return E.What.empty() ? std::string() : "the " + E.What;
	case EventKind::SiteOverlay: return "the overlay";
	case EventKind::SiteLaunched: return "a new site";
	case EventKind::BoardReport: return "the Season race";
	case EventKind::PlayerOfYear: return "the Player of the Year";
	case EventKind::HallOfFame: return "the Hall of Fame";
	case EventKind::Rivalry:
	{
		const std::string A = VwNpc(W, E.Npc), B = VwNpc(W, E.Other);
		return A.empty() || B.empty() ? std::string() : A + " against " + B;
	}
	default: break;
	}
	return std::string();
}

/** A video's own views at a world minute: followers x 0.03 x its luck, risen on the creator's curve. */
double VwViews(int Followers, uint32_t Luck, double At, double World)
{
	const double U = static_cast<double>(Luck % 10000u) / 10000.0;
	const double L = 0.35 + 1.9 * U * U; // most videos do about typical; a few break out
	const double AgeDays = std::max(0.0, (World - At) / VwDay);
	return std::floor(static_cast<double>(Followers) * 0.03 * L * (1.0 - std::exp(-AgeDays / 1.6)));
}

struct VwItem
{
	WorldVideo V;
	int Followers = 0;
	uint32_t Luck = 0;
};

/** The title and the thumbnail of a candidate, through the player's own engines (an empty channel: no recent templates). */
void VwPackage(const Candidate& C, uint32_t H, WorldVideo& Out)
{
	static const Channel None;
	Choice Ch;
	Ch.Candidates = {C.Id};
	Ch.F = Format::Breakdown;
	Ch.T = static_cast<TitleStyle>(H % 3u == 0 ? 0 : H % 3u == 1 ? 3 : 4); // Story, Question or Plain: news, not Loud
	const std::vector<Candidate> Open = {C};
	for (const TitleOption& O : TitleOptions(Ch, Open, None, static_cast<int>(H % 97u), 1))
	{
		Out.Title = O.Text;
	}
	for (const ThumbSpec& S : ThumbOptions(Ch, Open, None, static_cast<int>(H % 89u), 1))
	{
		Out.Spec = S;
	}
}

std::vector<VwItem> VwBuild(const world::World& W, double World)
{
	// The whole of today is built (the cache lives a day); what isn't up yet is left out at read time.
	std::vector<VwItem> Out;
	const std::vector<kast::Streamer>& Dir = kast::Directory();
	if (Dir.empty())
	{
		return Out;
	}
	const int Today = static_cast<int>(std::floor(World / VwDay));
	const double From = World - static_cast<double>(VwWindowDays) * VwDay, Horizon = static_cast<double>(Today + 1) * VwDay;
	// (a) The news: each log line a channel would cover goes to one creator (not gh0stfold, who covers nobody but you).
	for (const world::WorldEvent& E : W.Events())
	{
		if (E.At < From || E.At > World)
		{
			continue;
		}
		std::string Facts;
		const int Idea = VwNews(W, E, Facts);
		if (Idea == 0)
		{
			continue;
		}
		const uint32_t H = Fnv1a("world:" + std::to_string(static_cast<long long>(std::llround(E.At))) + ":" + std::to_string(static_cast<int>(E.Kind)) + ":" +
								 std::to_string(E.Npc));
		size_t Who = H % Dir.size();
		Who = Dir[Who].Rival ? (Who + 1) % Dir.size() : Who;
		const kast::Streamer& S = Dir[Who];
		Candidate C;
		C.Id = "world:" + S.Name + ":" + std::to_string(H);
		C.K = Kind::Idea;
		C.Source = 5;
		C.Idea = Idea;
		C.IdeaFacts = Facts;
		C.At = E.At;
		VwItem I;
		I.V.Creator = S.Name;
		I.V.At = E.At + 60.0 * static_cast<double>(6 + H % 18u); // up a few hours after the news
		if (I.V.At >= Horizon)
		{
			continue;
		}
		VwPackage(C, H, I.V);
		if (I.V.Title.empty())
		{
			continue;
		}
		I.Followers = S.Followers;
		I.Luck = Fnv1a(S.Name + "|" + C.Id);
		Out.push_back(I);
	}
	// (b) Each creator's own weekly show, on a weekday of their own (only days the world has lived).
	for (const kast::Streamer& S : Dir)
	{
		const std::vector<const char*>& Shows = VwShows(S.Name);
		if (Shows.empty())
		{
			continue;
		}
		const int Weekday = static_cast<int>(Fnv1a("show:" + S.Name) % 7u);
		for (int D = Today; D > Today - VwWindowDays && D >= 0; --D)
		{
			if (net::Weekday(D) != Weekday)
			{
				continue;
			}
			const uint32_t H = Fnv1a("show:" + S.Name + ":" + std::to_string(D));
			VwItem I;
			I.V.Creator = S.Name;
			I.V.At = static_cast<double>(D) * VwDay + 60.0 * static_cast<double>(12 + H % 10u);
			if (I.V.At >= Horizon)
			{
				continue;
			}
			I.V.Title = Shows[(H >> 4) % Shows.size()];
			Candidate C;
			C.Id = "show:" + S.Name + ":" + std::to_string(D);
			C.K = Kind::Idea;
			C.Source = 5;
			C.Idea = VwShowIdea(S.Name);
			C.At = I.V.At;
			static const Channel None;
			Choice Ch;
			Ch.Candidates = {C.Id};
			Ch.F = Format::Breakdown;
			for (const ThumbSpec& T : ThumbOptions(Ch, {C}, None, static_cast<int>(H % 89u), 1))
			{
				I.V.Spec = T;
			}
			I.Followers = S.Followers;
			I.Luck = Fnv1a(S.Name + "|" + C.Id);
			Out.push_back(I);
		}
	}
	// (c) Overlay Weekly: Monday's show of the week before's news, the three biggest stories (none in a quiet week).
	for (int D = Today; D > Today - VwWindowDays && D >= 7; --D)
	{
		if (net::Weekday(D) != 0)
		{
			continue;
		}
		std::vector<std::string> Lines;
		for (auto It = W.Events().rbegin(); It != W.Events().rend() && Lines.size() < 3; ++It)
		{
			if (It->At < static_cast<double>(D - 7) * VwDay || It->At >= static_cast<double>(D) * VwDay)
			{
				continue;
			}
			const std::string L = VwHeadline(W, *It);
			if (!L.empty() && std::find(Lines.begin(), Lines.end(), L) == Lines.end())
			{
				Lines.push_back(L);
			}
		}
		if (Lines.empty())
		{
			continue;
		}
		int Y = 0, M = 0, Dm = 0;
		world::CivilDate(D, Y, M, Dm);
		const int Week = (D - world::DayOn(Y, 1, 1)) / 7 + 1;
		std::string Title = "Overlay Weekly, week " + std::to_string(Week) + ": " + Lines[0];
		for (size_t K = 1; K < Lines.size(); ++K)
		{
			Title += (K + 1 == Lines.size() ? ", and " : ", ") + Lines[K];
		}
		if (Title.size() > 100)
		{
			Title = "Overlay Weekly, week " + std::to_string(Week) + ": " + Lines[0];
		}
		VwItem I;
		I.V.Creator = "Overlay Weekly";
		I.V.At = static_cast<double>(D) * VwDay + 9.0 * 60.0;
		if (I.V.At >= Horizon)
		{
			continue;
		}
		I.V.Title = Title;
		Candidate C;
		C.Id = "overlay-weekly:" + std::to_string(D);
		C.K = Kind::Idea;
		C.Source = 5;
		C.Idea = 808;
		C.IdeaFacts = "Count=3;";
		static const Channel None;
		Choice Ch;
		Ch.Candidates = {C.Id};
		Ch.F = Format::Breakdown;
		for (const ThumbSpec& T : ThumbOptions(Ch, {C}, None, D % 89, 1))
		{
			I.V.Spec = T;
		}
		I.Followers = VwOverlayFollowers;
		I.Luck = Fnv1a("Overlay Weekly|" + C.Id);
		Out.push_back(I);
	}
	std::stable_sort(Out.begin(), Out.end(), [](const VwItem& A, const VwItem& B) { return A.V.At > B.V.At; });
	return Out;
}
} // namespace

std::vector<std::string> WorldShowInventory()
{
	std::vector<std::string> Out;
	for (const kast::Streamer& S : kast::Directory())
	{
		for (const char* T : VwShows(S.Name))
		{
			Out.push_back(T);
		}
	}
	return Out;
}
} // namespace vcontent

std::vector<WorldVideo> WorldVideos(const world::World& W, double World, int Count)
{
	using namespace vcontent;
	// The day's list, cached until the day or the log moves (Home asks every frame); the views move with the minute.
	struct Cache
	{
		int Day = -1;
		size_t Log = 0;
		double LastAt = -1;
		const world::World* Of = nullptr;
		std::vector<VwItem> Items;
	};
	static Cache K;
	const int Day = static_cast<int>(std::floor(World / VwDay));
	const double LastAt = W.Events().empty() ? -1.0 : W.Events().back().At;
	if (K.Day != Day || K.Log != W.Events().size() || K.LastAt != LastAt || K.Of != &W)
	{
		K.Day = Day;
		K.Log = W.Events().size();
		K.LastAt = LastAt;
		K.Of = &W;
		K.Items = VwBuild(W, World);
	}
	std::vector<WorldVideo> Out;
	for (const VwItem& I : K.Items)
	{
		if (static_cast<int>(Out.size()) >= Count)
		{
			break;
		}
		if (I.V.At > World)
		{
			continue; // not up yet
		}
		WorldVideo V = I.V;
		V.Views = VwViews(I.Followers, I.Luck, I.V.At, World);
		Out.push_back(V);
	}
	return Out;
}
} // namespace video
} // namespace ss
