// Update 3 (SPEC §4-§6, §7.2): Rerun 2.0's Session methods, core-videocontent's file: the ideas from the player's own career
// (the IdeaCtx the Session builds), Studio's title and thumbnail options, repackaging, the ad load, the launch actions and
// their TimeSkip ("video-act:"), Make a Cut's Cut, and one video's analytics.
// core-videoalgo's patch (core-videoalgo_to_core-videocontent_2.diff): SetAdLoad, LaunchAction and SettleVideoAct, Repackage's
// mechanics (the rescale component, the next check-in skipped) and VideoStatsOf, on the core's model (VideoAlgoPriv.h's
// readings of the saved fields). The ideas, the options and Repackage's new packaging score are core-videocontent's.
#include "ShortStack/Game/Session.h"

#include "VideoAlgoPriv.h"
#include "VideoContentPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/SiteData.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace ss
{
namespace video
{
namespace video_detail
{
bool EventKind(Kind K); // Video.cpp
} // namespace video_detail
} // namespace video

namespace session_video3_detail
{
/** A kept Update 3 video by id (nullptr: none, folded or legacy). */
video::Video* Kept(video::Channel& C, const std::string& Id)
{
	for (video::Video& V : C.Videos)
	{
		if (V.Id == Id)
		{
			return &V;
		}
	}
	return nullptr;
}

/** The launch action's world minutes (§4.6.9): Make a Cut 20, a teaser 10, a reply round 20; Kast is the stream itself. */
double ActMinutes(video::Launch A)
{
	return A == video::Launch::Teaser ? 10.0 : A == video::Launch::Kast ? 0.0 : 20.0;
}

const char* ActKey(video::Launch A)
{
	switch (A)
	{
	case video::Launch::Cut: return "cut";
	case video::Launch::Kast: return "kast";
	case video::Launch::Teaser: return "teaser";
	default: return "reply";
	}
}

bool Has(const std::string& Low, const char* Word)
{
	return Low.find(Word) != std::string::npos;
}

/** A result's flags (video::IdeaFlag) from its name, place and prize: the schedule names what it is. */
uint32_t FlagsOf(const std::string& Name, int Place, Chips Prize, bool Live)
{
	std::string Low = Name;
	for (char& Ch : Low)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	uint32_t F = Live ? video::IfLive : 0u;
	F |= Has(Low, "turbo") ? video::IfTurbo : 0u;
	F |= Has(Low, "hyper") ? video::IfHyper : 0u;
	F |= Has(Low, "mystery") ? video::IfMystery : (Has(Low, "pko") || Has(Low, "bounty") || Has(Low, "knockout")) ? video::IfBounty : 0u;
	F |= Has(Low, "omaha") || Has(Low, "plo") ? video::IfOmaha : 0u;
	F |= Has(Low, "freeroll") ? video::IfFreeroll : 0u;
	F |= Has(Low, "satellite") || Has(Low, "step ") ? video::IfSatellite : 0u;
	F |= Has(Low, "sunday blaze") || Has(Low, "sunday 100k") || Has(Low, "embercrest sunday") || Has(Low, "main event") ? video::IfMajor : 0u;
	F |= Has(Low, "the spark") ? video::IfSpark : 0u;
	F |= Has(Low, "madness") || Has(Low, "series") || Has(Low, "classic") || Has(Low, "#") ? video::IfSeries : 0u;
	F |= Place >= 1 && Place <= 9 ? video::IfFinal : 0u;
	F |= (F & video::IfSatellite) != 0 && Prize > 0 && Place >= 1 ? video::IfSeatWon : 0u;
	return F;
}

/** When a scheduled id's event ran ("mm-26@1530": the world minute after '@'), -1 when the id doesn't say. */
double MinuteOf(const std::string& EventId)
{
	const size_t At = EventId.rfind('@');
	if (At == std::string::npos || At + 1 >= EventId.size())
	{
		return -1.0;
	}
	char* End = nullptr;
	const double M = std::strtod(EventId.c_str() + At + 1, &End);
	return End && End != EventId.c_str() + At + 1 ? M : -1.0;
}

/** The player's cards in a coach's sentence ("A-T off, 22 big blinds, ..."), as words ("ace-ten"); "" when unreadable. */
std::string HandOfWorst(const std::string& Line)
{
	static const char Ranks[] = "23456789TJQKA";
	static const char* const Words[] = {"deuce", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "jack", "queen", "king", "ace"};
	static const char* const Pairs[] = {"deuces", "threes", "fours", "fives", "sixes", "sevens", "eights", "nines", "tens", "jacks", "queens", "kings", "aces"};
	if (Line.size() < 3 || Line[1] != '-')
	{
		return std::string();
	}
	const char* A = std::strchr(Ranks, Line[0]);
	const char* B = std::strchr(Ranks, Line[2]);
	if (!A || !B || !*A || !*B)
	{
		return std::string();
	}
	const int Ia = static_cast<int>(A - Ranks), Ib = static_cast<int>(B - Ranks);
	return Ia == Ib ? Pairs[Ia] : std::string(Words[std::max(Ia, Ib)]) + "-" + Words[std::min(Ia, Ib)];
}

/** What the ideas read (§6.3), from the session's state: the inputs a member passes in (its private members included). */
struct CtxIn
{
	const life::State* L = nullptr;
	const std::vector<HistoryEntry>* History = nullptr;
	const world::World* W = nullptr;
	const kast::Channel* K = nullptr;
	const gear::Owned* Gear = nullptr;
	const std::set<std::string>* Told = nullptr;
	Chips Bankroll = 0;
	std::string Hero;
	bool Evicted = false;
	double World = 0;
};

video::IdeaCtx BuildCtx(const CtxIn& In)
{
	using namespace video;
	IdeaCtx X;
	const life::State& L = *In.L;
	const double World = In.World;
	X.World = World;
	X.Hero = In.Hero;
	X.Chan = &L.Video;
	// Poker: History's results (dated by their scheduled ids), the live entries, the Tracker's lifetime.
	for (const HistoryEntry& H : *In.History)
	{
		IdeaResult R;
		R.Name = H.Name;
		R.Id = H.EventId.empty() ? H.Name : H.EventId;
		R.At = MinuteOf(H.EventId);
		if (R.At < 0.0)
		{
			continue; // undated (a save from before the schedule): no idea can say when
		}
		R.Site = static_cast<int>(net::SiteOf(H.EventId));
		R.Place = H.Place;
		R.Field = H.Entrants;
		R.BuyIn = std::max<Chips>(0, H.BuyInCents);
		R.Prize = H.Prize;
		R.Flags = FlagsOf(H.Name, H.Place, H.Prize, false) | (H.BuyInCents == 0 ? IfFreeroll : 0u);
		X.Results.push_back(R);
	}
	for (const life::LiveEntry& E : L.LiveEntries)
	{
		if (E.State != life::LiveEntry::Finished)
		{
			continue;
		}
		IdeaResult R;
		R.Name = E.Name;
		R.Id = E.Id;
		R.At = E.RegisteredAt;
		R.Place = E.Place;
		R.Field = E.Entrants;
		R.BuyIn = E.PaidCents;
		R.Prize = E.PrizeCents;
		R.Flags = FlagsOf(E.Name, E.Place, E.PrizeCents, true);
		X.Results.push_back(R);
		X.FirstLiveAt = X.FirstLiveAt < 0.0 ? E.RegisteredAt : std::min(X.FirstLiveAt, E.RegisteredAt);
	}
	std::stable_sort(X.Results.begin(), X.Results.end(), [](const IdeaResult& A, const IdeaResult& B) { return A.At > B.At; });
	const world::Tracker& T = In.W->HeroStats();
	X.Events = T.Events;
	X.Cashes = T.Cashes;
	X.FinalTables = T.FinalTables;
	X.Wins = T.Wins;
	X.BuyIns = T.BuyIns;
	X.Prizes = T.Prizes;
	for (const IdeaResult& R : X.Results)
	{
		X.BestPrize = std::max(X.BestPrize, R.Prize);
		X.Freerolls += (R.Flags & IfFreeroll) != 0 ? 1 : 0;
	}
	// The coach's worst decisions since the last review (each spot's), as graded mistakes: what it cost, the hand, the street.
	for (int S = 0; S < coach::SpotCount; ++S)
	{
		const coach::Worst& Wo = L.Study.Worsts[static_cast<size_t>(S)];
		if (Wo.Hand.empty() || Wo.LossMilliBb <= 0)
		{
			continue;
		}
		IdeaGrade G;
		G.At = std::max(L.Study.LastReview, World - 3.0 * 1440.0);
		G.Grade = 3;
		G.Hand = HandOfWorst(Wo.Hand);
		const coach::Spot Sp = static_cast<coach::Spot>(S);
		G.Street = Sp == coach::Spot::RiverValue || Sp == coach::Spot::RiverCall ? "river" : Sp == coach::Spot::Turn ? "turn" :
			Sp == coach::Spot::Flop || Sp == coach::Spot::FacingFlop ? "flop" : "preflop";
		G.River = G.Street == "river";
		G.Shove = Sp == coach::Spot::PushFold || Sp == coach::Spot::CallShove;
		G.Bubble = Sp == coach::Spot::Bubble;
		G.CostBb = static_cast<double>(Wo.LossMilliBb) / 1000.0;
		if (!G.Hand.empty())
		{
			X.Grades.push_back(G);
		}
	}
	X.DeesNights = L.BackRoomNights;
	X.LiveEvents = L.LiveEvents;
	// Money and life.
	X.Bankroll = In.Bankroll;
	X.RentDue = L.RentDueCents;
	X.RentDeadline = L.RentDeadline;
	X.RentStage = static_cast<int>(L.RentStage);
	X.RentsPaid = L.RentsPaid;
	X.Evictions = L.Evictions;
	X.Evicted = In.Evicted;
	X.Shifts = L.Shifts;
	X.LastShiftAt = L.LastShiftAt;
	X.Home = L.Evictions > 0 ? 2 : 1;
	for (const std::string& Key : vcontent::StoryKeys())
	{
		if (In.Told->count(Key) > 0)
		{
			X.Stories.push_back(Key);
		}
	}
	// The gear the player owns (bought outright or rented): the names the titles may say, never a moderation bot's.
	for (const auto& Kv : *In.Gear)
	{
		const gear::Item* I = gear::Find(Kv.first);
		if (!I || I->Name.empty() || I->Name.find("Bot") != std::string::npos || I->Name.find("bot") != std::string::npos)
		{
			continue;
		}
		X.Gear.push_back({I->Name, I->PriceCents});
		X.NewestGear = I->Name;
	}
	// Kast: the followers, the streams' log, the clips.
	X.KastFollowers = In.K->Followers;
	X.KastStreams = In.K->Streams;
	X.RaidsIn = In.K->RaidsIn;
	for (const kast::StreamLog& S : In.K->Log)
	{
		IdeaStream R;
		R.Start = S.Start;
		R.Minutes = S.Minutes;
		R.Avg = S.Avg;
		R.Peak = S.Peak;
		R.Follows = S.Follows;
		X.Streams.push_back(R);
	}
	for (const kast::Clip& C : In.K->Clips)
	{
		IdeaClip R;
		R.Title = C.Title;
		R.Candidate = C.Candidate;
		R.At = C.At;
		R.Views = C.Views;
		R.Moment = C.Kind;
		X.Clips.push_back(R);
	}
	// The ledger (the newest 60 lines): the last rent paid, the shifts (the night cashier's in 14 days, the Wash & Fold's),
	// the gear bought this week and the newest camera; the coach's reviews this week. (What the ledger has lost is unknown:
	// an idea that needs it stays locked rather than guess.)
	{
		const life::Activity* Night = life::Find("quikstop");
		const life::Activity* Wash = life::Find("washfold");
		const std::string NightShift = Night ? Night->Place + " shift" : std::string(), WashShift = Wash ? Wash->Place + " shift" : std::string();
		std::string NewestCam;
		for (const life::LedgerEntry& E : L.Ledger)
		{
			if (E.At > World)
			{
				continue;
			}
			if (E.Kind == 3 && E.Amount < 0 && (E.Label.rfind("Rent", 0) == 0 || E.Label.rfind("Back rent", 0) == 0))
			{
				X.LastRentAt = std::max(X.LastRentAt, E.At);
			}
			X.NightShifts14 += E.Kind == 1 && !NightShift.empty() && E.Label == NightShift && E.At > World - 14.0 * 1440.0 ? 1 : 0;
			X.WashFold += E.Kind == 1 && !WashShift.empty() && E.Label == WashShift ? 1 : 0;
			if (E.Kind == 5 && E.Amount < 0 && E.Label.find("(renewal)") == std::string::npos && E.At > World - 7.0 * 1440.0)
			{
				X.GearBought7 += -E.Amount;
			}
			// The newest camera bought (a webcam's purchase line, "GearDrop: 1080p60 webcam"), within 14 days.
			if (NewestCam.empty() && E.Kind == 5 && E.Amount < 0 && E.At > World - 14.0 * 1440.0 && E.Label.find("(renewal)") == std::string::npos)
			{
				for (const auto& Kv : *In.Gear)
				{
					const gear::Item* I = gear::Find(Kv.first);
					if (I && I->Where == gear::Slot::Camera && E.Label == "GearDrop: " + I->Name)
					{
						NewestCam = I->Id;
					}
				}
			}
		}
		// The webcam's tier now, and before that newest camera (the best of the others the player owns).
		for (const auto& Kv : *In.Gear)
		{
			const gear::Item* I = gear::Find(Kv.first);
			if (!I || I->Where != gear::Slot::Camera)
			{
				continue;
			}
			const int Tier = I->Id == "webcam-720" ? 1 : I->Id == "webcam-1080" ? 2 : 3;
			X.Webcam = std::max(X.Webcam, Tier);
			X.WebcamWas = I->Id == NewestCam ? X.WebcamWas : std::max(X.WebcamWas, Tier);
		}
		X.WebcamWas = NewestCam.empty() ? X.Webcam : X.WebcamWas;
		X.StudyWeek = L.Study.Reviews > 0 && L.Study.LastReview > World - 7.0 * 1440.0 && L.Study.LastReview <= World ? 1 : 0;
	}
	// The channel's subscribers a week ago and its comments over 30 days, from its day rows (Rerun 2.0's rings).
	X.SubsWeekAgo = L.Video.Subs;
	{
		const int Today = static_cast<int>(std::floor(World / 1440.0));
		for (const video::DayRow& R : L.Video.X.Days)
		{
			X.SubsWeekAgo -= R.Key > Today - 7 ? static_cast<double>(R.Gained - R.Lost) : 0.0;
			X.Comments30 += R.Key > Today - 30 ? static_cast<int>(std::lround(R.Comments)) : 0;
		}
	}
	// The world, at least a day old (U5), with its people's names.
	for (const world::WorldEvent& E : In.W->Events())
	{
		if (E.At > World - 1440.0 || E.At < World - 45.0 * 1440.0)
		{
			continue;
		}
		IdeaWorldEvent R;
		R.At = E.At;
		R.Kind = static_cast<int>(E.Kind);
		const world::Npc* A = In.W->Get(E.Npc);
		const world::Npc* B = In.W->Get(E.Other);
		R.Who = A ? A->Name : std::string();
		R.Other = B ? B->Name : std::string();
		R.What = E.What;
		R.Amount = E.Amount;
		R.Value = E.Value;
		R.Site = static_cast<int>(world::FlagSiteOf(E.Flags));
		X.WorldLog.push_back(R);
	}
	// The rival sites.
	X.Joined.push_back(static_cast<int>(net::SiteId::RiverLine));
	for (const site::Account& A : L.Sites.Accounts)
	{
		if (A.Site != net::SiteId::RiverLine)
		{
			X.Joined.push_back(static_cast<int>(A.Site));
			X.WelcomePending = X.WelcomePending || A.BonusPending > 0;
			X.RewardsTier = std::max(X.RewardsTier, A.Vip);
		}
	}
	// The rival sites' play book (site::play): the Flame, the best FUSE, a Beef paid back, Team Scalps.
	const site::play::Book& B = L.SitePlay;
	X.Flame = B.Ig.F.Streak;
	X.FuseBest = B.Ig.Fuse.BestM;
	for (const site::play::Beef& Bf : B.Ig.Beefs)
	{
		if (Bf.TheyKo >= 2 && Bf.YouKo > 0 && Bf.TheyKo >= X.BeefKo)
		{
			X.Beef = Bf.Name;
			X.BeefKo = Bf.TheyKo;
			X.BeefPaid = true;
		}
	}
	X.TeamScalps = (B.Rt.Sc.Taken & 0x3Fu) == 0x3Fu;
	for (const IdeaResult& R : X.Results)
	{
		bool Seen = false;
		for (auto& Kv : X.FirstSession)
		{
			Seen = Seen || Kv.first == R.Site;
			Kv.second = Kv.first == R.Site ? std::min(Kv.second, R.At) : Kv.second;
		}
		if (!Seen && R.Site != static_cast<int>(net::SiteId::RiverLine))
		{
			X.FirstSession.push_back({R.Site, R.At});
		}
	}
	return X;
}

/** After an upload lands (SettleEdit): a series' episode counted, its playlist and its challenge's state (§6.3 P). */
void AfterUpload(life::State& L, const std::vector<HistoryEntry>& History, const world::World& W, const kast::Channel& K, const gear::Owned& G,
	const std::set<std::string>& Told, Chips Bankroll, const std::string& Hero, bool Evicted, double World, const video::Choice& C,
	const std::vector<video::Candidate>& Picked, const std::string& VideoId)
{
	int Series = C.Series;
	for (const video::Candidate& X : Picked)
	{
		if (Series < 0 && X.Idea / 100 == 16)
		{
			for (const video::Challenge& S : L.Video.X.Challenges)
			{
				Series = S.Kind == X.Idea && S.State == 0 ? S.Id : Series;
			}
		}
	}
	if (Series < 0)
	{
		return;
	}
	CtxIn In;
	In.L = &L;
	In.History = &History;
	In.W = &W;
	In.K = &K;
	In.Gear = &G;
	In.Told = &Told;
	In.Bankroll = Bankroll;
	In.Hero = Hero;
	In.Evicted = Evicted;
	In.World = World;
	const video::IdeaCtx X = BuildCtx(In);
	video::vcontent::NoteEpisode(L.Video, X, Series, VideoId);
}

/** Room in MOMENTS for one more (at most 30): the lowest drama goes, never Keep. */
void MakeRoom(video::Channel& C, const std::string& Keep)
{
	while (C.Open.size() > 30)
	{
		auto Low = C.Open.end();
		for (auto It = C.Open.begin(); It != C.Open.end(); ++It)
		{
			if (It->Id != Keep && (Low == C.Open.end() || It->Drama < Low->Drama))
			{
				Low = It;
			}
		}
		if (Low == C.Open.end())
		{
			return;
		}
		C.Open.erase(Low);
	}
}
} // namespace session_video3_detail

namespace video
{
double EditMinutesOf(const Choice& Ch)
{
	// Update 2's choices keep Update 2's edit; Update 3's: the length option's, a Duel's extra contenders (15 minutes each),
	// Clean edit's 15 minutes.
	const bool U3 = !Ch.TitleTpl.empty() || !Ch.Thumbs.empty();
	if (!U3)
	{
		return EditMinutes(Ch.F);
	}
	const int Contenders = std::max<int>(1, static_cast<int>(Ch.Thumbs.size()));
	return valgo::EditMinutesFor(Ch.F, Ch.Len) + 15.0 * static_cast<double>(Contenders - 1) + (Ch.CleanEdit ? valgo::VaCleanEditMinutes : 0.0);
}

double EditEnergyOf(const Choice& Ch)
{
	const bool U3 = !Ch.TitleTpl.empty() || !Ch.Thumbs.empty();
	if (!U3)
	{
		return EditEnergy(Ch.F);
	}
	const int Contenders = std::max<int>(1, static_cast<int>(Ch.Thumbs.size()));
	return valgo::EditEnergyFor(Ch.F, Ch.Len) + 1.0 * static_cast<double>(Contenders - 1);
}
} // namespace video

std::vector<video::IdeaCard> Session::VideoIdeas() const
{
	using namespace session_video3_detail;
	CtxIn In;
	In.L = &Life;
	In.History = &History;
	In.W = &LivingWorld;
	In.K = &Channel;
	In.Gear = &Gear;
	In.Told = &TextsSeen;
	In.Bankroll = BankrollCents;
	In.Hero = HeroName;
	In.Evicted = Evicted();
	In.World = WorldMinutes();
	const video::IdeaCtx X = BuildCtx(In);
	return video::Ideas(X, Life.Video, In.World);
}

std::string Session::ChooseIdea(int IdeaId)
{
	// §6.3: a ready idea becomes a candidate in MOMENTS (Source 5); a group A idea tags its moment instead (the hand's own
	// candidate, Idea set and the idea's facts beside its own); a group P idea declares its series first.
	using namespace session_video3_detail;
	CtxIn In;
	In.L = &Life;
	In.History = &History;
	In.W = &LivingWorld;
	In.K = &Channel;
	In.Gear = &Gear;
	In.Told = &TextsSeen;
	In.Bankroll = BankrollCents;
	In.Hero = HeroName;
	In.Evicted = Evicted();
	In.World = WorldMinutes();
	const video::IdeaCtx X = BuildCtx(In);
	const double World = In.World;
	if (IdeaId / 100 == 1)
	{
		const std::string Src = video::vcontent::IdeaMoment(X, Life.Video, IdeaId, World);
		for (video::Candidate& M : Life.Video.Open)
		{
			if (!Src.empty() && M.Id == Src)
			{
				M.Idea = IdeaId;
				const std::string Facts = video::vcontent::MomentIdeaFacts(IdeaId);
				M.IdeaFacts = M.IdeaFacts.empty() ? Facts : M.IdeaFacts + ";" + Facts;
				Save();
				return "";
			}
		}
		for (const video::IdeaCard& Card : video::Ideas(X, Life.Video, World))
		{
			if (Card.Id == IdeaId)
			{
				return Card.Hint.empty() ? std::string("Not yet.") : Card.Hint;
			}
		}
		return "Not yet.";
	}
	video::Candidate N;
	std::string Why;
	if (IdeaId / 100 == 16)
	{
		// A series is declared only when its idea is ready (a locked one says what to do and declares nothing).
		if (!video::vcontent::MakeIdeaCandidateOn(X, Life.Video, IdeaId, World, N, Why))
		{
			return Why.empty() ? std::string("Not yet.") : Why;
		}
		video::vcontent::DeclareSeries(Life.Video, X, IdeaId, World);
	}
	if (!video::vcontent::MakeIdeaCandidateOn(X, Life.Video, IdeaId, World, N, Why))
	{
		return Why.empty() ? std::string("Not yet.") : Why;
	}
	Life.Video.Open.erase(std::remove_if(Life.Video.Open.begin(), Life.Video.Open.end(), [&N](const video::Candidate& C) { return C.Id == N.Id; }),
		Life.Video.Open.end());
	Life.Video.Open.push_back(N);
	MakeRoom(Life.Video, N.Id);
	Life.Update2LinesRead |= life::U2Video;
	Save();
	return "";
}

std::vector<video::TitleOption> Session::TitleOptions(const video::Choice& C, int Shuffle) const
{
	return video::TitleOptions(C, Life.Video.Open, Life.Video, Shuffle, 3);
}

std::vector<video::ThumbSpec> Session::ThumbOptions(const video::Choice& C, int Shuffle) const
{
	return video::ThumbOptions(C, Life.Video.Open, Life.Video, Shuffle, 6);
}

std::string Session::Repackage(const std::string& VideoId, const video::Repack& Rp)
{
	// §4.6.7: up to 28 days old, at most twice and 3 days apart, a slot free. The new CTR c' comes from the same packaging
	// formula; an exact component rescales the rest of the curve (views never go down); the next check-in is skipped.
	using namespace session_video3_detail;
	video::Video* V = Kept(Life.Video, VideoId);
	const double World = WorldMinutes();
	const int Today = static_cast<int>(std::floor(World / 1440.0));
	if (!V || !V->X.On)
	{
		return "This video was published before repackaging.";
	}
	if (World > V->At + 28.0 * 1440.0)
	{
		return "Rerun has stopped testing this video's packaging.";
	}
	if (V->X.Repacks >= 2 || !video::valgo::CompFits(*V, Today))
	{
		return "This video has changed enough.";
	}
	if (V->X.LastRepackDay >= 0 && Today < V->X.LastRepackDay + 3)
	{
		return "Give the last change three days.";
	}
	// The packaging ratio: Upload's own packaging term (the recipe's fit and traits, the title's traits, promise over payoff)
	// for the new title and thumbnail against the shown ones, on the video's own kind (the moment it was cut from is gone
	// from MOMENTS by now; the kind and the thumbnail are what's left of it). A new title moves the CTR as a new thumbnail
	// does, and its promise is the one the later check-ins read (§4.6.7).
	const video::Candidate Lead = video::vcontent::RepackLead(*V);
	const int Group = video::valgo::KindGroup(V->K);
	const video::ThumbSpec& Was = V->X.Spec[static_cast<size_t>(std::max<int>(0, V->X.DuelWin))];
	uint16_t OldTraits = 0;
	for (const video::vcontent::Tpl& Tp : video::vcontent::Templates3())
	{
		OldTraits = Tp.Key == V->X.TitleTpl ? video::vcontent::TraitsOf(Tp, V->Title) : OldTraits;
	}
	int NewPromise = V->X.Promise;
	uint16_t NewTraits = OldTraits;
	std::string NewTitle;
	if (!Rp.TitleKey.empty())
	{
		// The new title's words: the template over what the video keeps of its moment (RepackTitles' lead), any deal of it.
		for (int S = 0; S < 48 && NewTitle.empty(); ++S)
		{
			for (const video::TitleOption& O : video::RepackTitles(*V, Life.Video, S, 3))
			{
				if (O.Key == Rp.TitleKey)
				{
					NewTitle = O.Text;
					NewTraits = O.Traits;
					NewPromise = O.Promise;
				}
			}
		}
		if (NewTitle.empty())
		{
			return "That title doesn't fit this video any more.";
		}
	}
	auto Pack = [&](const video::ThumbSpec& S, uint16_t Traits, int TitlePromise) {
		const int Promise = std::max(TitlePromise, static_cast<int>(video::ThumbPromise(S)));
		const int Over = std::max(0, Promise - static_cast<int>(V->X.Payoff)), Under = std::max(0, static_cast<int>(V->X.Payoff) - Promise);
		return std::clamp(video::ArchetypeFit(S.Arch, Group, Lead) * video::ThumbTraits(S, Lead, Life.Video) * (Traits != 0 ? video::valgo::TitleTraitCtr(Traits) : 1.0) *
				(1.0 + 0.12 * static_cast<double>(Over)) * (1.0 - 0.04 * static_cast<double>(Under)),
			0.70, 1.45);
	};
	const double Old = Pack(Was, OldTraits, V->X.Promise);
	const double New = Pack(Rp.Spec.Arch >= 0 ? Rp.Spec : Was, NewTraits, NewPromise);
	const double NewCtr = std::clamp(V->X.Ctr * New / std::max(1e-9, Old), 0.02, 0.14);
	video::Comp K = video::algo::RepackComp(*V, World, NewCtr);
	V->X.Comps.push_back(K);
	V->X.S0 += 0.9 * std::log(NewCtr / V->X.Ctr);
	V->X.Ctr = NewCtr;
	if (Rp.Spec.Arch >= 0)
	{
		V->X.Spec[static_cast<size_t>(std::max<int>(0, V->X.DuelWin))] = Rp.Spec;
	}
	if (!Rp.TitleKey.empty())
	{
		V->Title = NewTitle;
		V->X.TitleTpl = Rp.TitleKey;
		V->X.Promise = static_cast<uint8_t>(NewPromise);
	}
	++V->X.Repacks;
	V->X.LastRepackDay = Today;
	Save();
	return "";
}

std::string Session::SetAdLoad(const std::string& VideoId, video::AdLoad L)
{
	// §4.5.5: once every 7 days, effective the next midnight; refused on an Update 2 video (no VideoX), a folded one and a
	// Cut (pooled ads). A switch waits in Ads until AdsChangedDay; DayStep moves the video's rates at that day's start.
	using namespace session_video3_detail;
	video::Video* V = Kept(Life.Video, VideoId);
	const int Today = static_cast<int>(std::floor(WorldMinutes() / 1440.0));
	if (!V || !V->X.On)
	{
		return "This video was published before ad settings.";
	}
	if (V->F == video::Format::Short)
	{
		return "Cuts share the feed's ads: there's nothing to set.";
	}
	if (V->X.Ads != V->X.AdsAtUpload || (V->X.AdsChangedDay >= 0 && Today < V->X.AdsChangedDay + 6))
	{
		return "You can change this video's ads once a week.";
	}
	if (L == V->X.Ads)
	{
		return "That's its setting already.";
	}
	V->X.Ads = L;
	V->X.AdsChangedDay = Today + 1;
	Save();
	return "";
}

std::string Session::LaunchAction(const std::string& VideoId, video::Launch A)
{
	// §4.6.9: inside the 48-hour test, each once a video, each costing world time (a TimeSkip "video-act:<a>:<id>"). Going
	// live on Kast is the stream itself (Live Drop, SPEC_CREATORS §3.6: video::algo::NoteStream at its end).
	using namespace session_video3_detail;
	video::Video* V = Kept(Life.Video, VideoId);
	const double World = WorldMinutes();
	const uint8_t Bit = static_cast<uint8_t>(1u << static_cast<int>(A));
	if (!V || !V->X.On)
	{
		return "This video was published before launch actions.";
	}
	if (World > V->At + 48.0 * 60.0)
	{
		return "Its first 48 hours are over.";
	}
	if ((V->X.Launch & Bit) != 0)
	{
		return "Done already.";
	}
	if (A == video::Launch::Kast)
	{
		return Stream.Live ? "You're live: keep going for 30 minutes." :
			World > V->At + 6.0 * 60.0 ? "Live Drop needed a stream within 6 hours of the drop." :
										   "Go live on Kast within 6 hours of the drop.";
	}
	if (A == video::Launch::Cut && V->F == video::Format::Short)
	{
		return "It's a Cut already.";
	}
	if (A == video::Launch::Cut)
	{
		// A Cut is one hand: a video that keeps none (a run's Recap, an idea told without a hand) has nothing to cut, and the
		// twenty minutes and the energy aren't spent on nothing.
		const size_t Slash = V->Th.Cards.find('/');
		if (Slash == std::string::npos || ParseCards(V->Th.Cards.substr(0, Slash)).size() < 2)
		{
			return "There's no single hand in this video to cut.";
		}
	}
	if (TimeSkip.Active)
	{
		return video::PublishWhy(1);
	}
	// (The other three are a TimeSkip like the edit's: Publish's refusals hold, a table open or a stream live never skips
	// the clock, and a Cut's edit needs the editor's energy.)
	if (T || TableCount() > 0)
	{
		return video::PublishWhy(2);
	}
	if (Stream.Live)
	{
		return video::PublishWhy(3);
	}
	if (A == video::Launch::Cut && Life.Energy < 10.0)
	{
		return video::PublishWhy(4);
	}
	TimeSkip = Skip();
	TimeSkip.Active = true;
	TimeSkip.From = World;
	TimeSkip.To = World + ActMinutes(A);
	TimeSkip.RealStart = Now;
	TimeSkip.RealSeconds = 2.0;
	TimeSkip.Rest = 1.0;
	TimeSkip.Couch = Evicted();
	TimeSkip.Label = A == video::Launch::Cut ? "Cutting" : A == video::Launch::Teaser ? "Posting a teaser" : "Replying";
	TimeSkip.Result.ActivityId = std::string("video-act:") + ActKey(A) + ":" + VideoId;
	TimeSkip.Result.Start = World;
	TimeSkip.Result.End = TimeSkip.To;
	TimeSkip.Result.Energy = A == video::Launch::Cut ? -2.0 : 0.0;
	HasOutcome = false;
	return "";
}

std::string Session::SettleVideoAct(const std::string& ActivityId)
{
	using namespace session_video3_detail;
	const std::string Body = ActivityId.rfind("video-act:", 0) == 0 ? ActivityId.substr(10) : std::string();
	const size_t Colon = Body.find(':');
	if (Colon == std::string::npos)
	{
		return "Not yet.";
	}
	const std::string Key = Body.substr(0, Colon), Id = Body.substr(Colon + 1);
	video::Video* V = Kept(Life.Video, Id);
	if (!V || !V->X.On)
	{
		return "That video is gone.";
	}
	for (int I = 0; I < static_cast<int>(video::Launch::Count); ++I)
	{
		if (Key != ActKey(static_cast<video::Launch>(I)))
		{
			continue;
		}
		V->X.Launch = static_cast<uint8_t>(V->X.Launch | (1u << I));
		if (static_cast<video::Launch>(I) == video::Launch::Cut)
		{
			// Make a Cut (§4.6.9): a Cut of the long video's moment, its own candidate from the long video's facts (its kind, its
			// cards, its title's event), up at once with the long one as its Next-up link (video::algo's FromCut pays it back).
			const video::Video Long = *V;
			video::Candidate C;
			C.Id = "cut:" + Long.Id;
			// (Its kind is what the long video shows of the hand, never more: an idea's fold stays a fold, CutKindOf.)
			C.K = video::vcontent::CutKindOf(Long);
			C.Source = 0;
			C.At = WorldMinutes();
			C.Site = Long.Site;
			C.Drama = 0.5;
			C.Expires = C.At + 1440.0;
			C.Idea = Long.X.Idea / 100 == 1 ? Long.X.Idea : -1;
			const std::string& Cards = Long.Th.Cards;
			const size_t A1 = Cards.find('/'), A2 = A1 == std::string::npos ? std::string::npos : Cards.find('/', A1 + 1);
			if (A1 != std::string::npos && A2 != std::string::npos)
			{
				C.Rec.Mine = ParseCards(Cards.substr(0, A1));
				const std::vector<Card> Theirs = ParseCards(Cards.substr(A1 + 1, A2 - A1 - 1));
				if (Theirs.size() >= 2 && !Long.Th.Opponent.empty())
				{
					C.Rec.Shown.push_back({Long.Th.Opponent, Theirs});
				}
				C.Rec.Board = ParseCards(Cards.substr(A2 + 1));
				C.Opponent = Long.Th.Opponent;
			}
			if (C.Rec.Mine.size() < 2)
			{
				Save();
				return ""; // a video with no hand to cut (an idea, a run): the launch counts, there's no Cut to make
			}
			Life.Video.Open.erase(std::remove_if(Life.Video.Open.begin(), Life.Video.Open.end(), [&C](const video::Candidate& X) { return X.Id == C.Id; }),
				Life.Video.Open.end());
			Life.Video.Open.push_back(C);
			MakeRoom(Life.Video, C.Id);
			video::Choice Ch;
			Ch.Candidates = {C.Id};
			Ch.F = video::Format::Short;
			Ch.T = video::TitleStyle::Story;
			Ch.NextUp = Long.Id;
			for (const video::TitleOption& O : video::TitleOptions(Ch, Life.Video.Open, Life.Video, static_cast<int>(Fnv1a(Long.Id) % 97u), 1))
			{
				Ch.TitleTpl = O.Key;
			}
			video::UploadFacts F;
			F.GearQuality = Fx.Quality;
			F.Sharp = static_cast<int>(std::lround(Sharp() * 100.0));
			F.KastFollowers = Channel.Followers;
			F.Energy = Life.Energy;
			F.RentsPaid = Life.RentsPaid;
			F.ExtraHours7 = video::algo::ExtraHours7(Life.Video, WorldMinutes());
			if (!Ch.TitleTpl.empty())
			{
				video::Upload(Life.Video, Ch, WorldMinutes(), SeedBase, F);
			}
		}
		Save();
		return "";
	}
	return "Not yet.";
}

video::VideoStats Session::VideoStatsOf(const std::string& VideoId) const
{
	for (const video::Video& V : Life.Video.Videos)
	{
		if (V.Id == VideoId)
		{
			return video::StatsOf(Life.Video, V, WorldMinutes());
		}
	}
	return video::VideoStats();
}

namespace session_video3_detail
{
/** C3 PIE fix F2: the TextsSeen key that holds Studio's seen ideas, "rerun-ideas-seen:1101,1601" (ids ascending; at most the
 *  library's 148, about 5 bytes each). Absent until the IDEAS list first shows a ready idea: an old save writes back as read. */
const std::string& IdeasSeenPrefix()
{
	static const std::string Prefix = "rerun-ideas-seen:";
	return Prefix;
}

std::set<std::string>::const_iterator IdeasSeenKeyIn(const std::set<std::string>& Told)
{
	const std::string& Pre = IdeasSeenPrefix();
	const auto It = Told.lower_bound(Pre);
	return It != Told.end() && It->compare(0, Pre.size(), Pre) == 0 ? It : Told.end();
}
} // namespace session_video3_detail

std::vector<int> Session::IdeasSeen() const
{
	using namespace session_video3_detail;
	std::vector<int> Out;
	const auto It = IdeasSeenKeyIn(TextsSeen);
	if (It == TextsSeen.end())
	{
		return Out;
	}
	const char* P = It->c_str() + IdeasSeenPrefix().size();
	while (*P != '\0')
	{
		char* End = nullptr;
		const long Id = std::strtol(P, &End, 10);
		if (End == P)
		{
			++P; // a stray character: skip it
			continue;
		}
		if (Id > 0 && Id < 100000)
		{
			Out.push_back(static_cast<int>(Id));
		}
		P = End;
	}
	std::sort(Out.begin(), Out.end());
	Out.erase(std::unique(Out.begin(), Out.end()), Out.end());
	return Out;
}

void Session::SeeIdeas(const std::vector<int>& IdeaIds)
{
	using namespace session_video3_detail;
	std::vector<int> All = IdeasSeen();
	const size_t Had = All.size();
	for (const int Id : IdeaIds)
	{
		if (Id > 0 && Id < 100000 && !std::binary_search(All.begin(), All.begin() + static_cast<std::ptrdiff_t>(Had), Id))
		{
			All.push_back(Id);
		}
	}
	if (All.size() == Had)
	{
		return; // nothing new seen: the save stays as it is
	}
	std::sort(All.begin(), All.end());
	All.erase(std::unique(All.begin(), All.end()), All.end());
	std::string Key = IdeasSeenPrefix();
	for (size_t K = 0; K < All.size(); ++K)
	{
		Key += (K > 0 ? "," : "") + std::to_string(All[K]);
	}
	const auto It = IdeasSeenKeyIn(TextsSeen);
	if (It != TextsSeen.end())
	{
		TextsSeen.erase(It);
	}
	TextsSeen.insert(Key);
	Save();
}

namespace video
{
std::vector<Trend> WeekTrends(const Session& S, double World)
{
	// The week's every half-day from its Monday 00:00, each listing what Trends lists then (the world's events before that
	// Monday, the calendar's windows): a trend's heat this week is its mean over the 14.
	const int Today = static_cast<int>(std::floor(World / 1440.0));
	const double Monday = static_cast<double>(Today - ((Today % 7) + 7) % 7) * 1440.0;
	std::vector<Trend> Out;
	for (int K = 0; K < 14; ++K)
	{
		for (const Trend& T : valgo::TrendsNow(S.Life, &S.Living(), Monday + 720.0 * static_cast<double>(K)))
		{
			auto It = std::find_if(Out.begin(), Out.end(), [&T](const Trend& X) { return X.Key == T.Key; });
			if (It == Out.end())
			{
				Trend N = T;
				N.Heat = 0.0;
				N.Mine = 0;
				Out.push_back(N);
				It = Out.end() - 1;
			}
			It->Heat += T.Heat / 14.0;
			It->Mine = std::max(It->Mine, T.Mine);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const Trend& A, const Trend& B) { return (A.Strength - 1.0) * A.Heat > (B.Strength - 1.0) * B.Heat; });
	return Out;
}

std::vector<SearchRow> SearchesOf(const Session& S, const std::vector<IdeaCard>& Ideas, int Count)
{
	// The table reads the world's week and the player's last 60 days of formats (History's events, dated by their scheduled
	// ids, and the live entries), the kept videos and the IDEAS board: nothing else of the career.
	const double World = S.WorldMinutes();
	IdeaCtx X;
	X.World = World;
	X.Chan = &S.Life.Video;
	for (const HistoryEntry& H : S.History)
	{
		const double At = session_video3_detail::MinuteOf(H.EventId);
		if (At < World - 60.0 * 1440.0 || At > World)
		{
			continue;
		}
		IdeaResult R;
		R.Name = H.Name;
		R.At = At;
		R.Flags = session_video3_detail::FlagsOf(H.Name, H.Place, H.Prize, false) | (H.BuyInCents == 0 ? IfFreeroll : 0u);
		X.Results.push_back(R);
	}
	for (const life::LiveEntry& E : S.Life.LiveEntries)
	{
		if (E.State == life::LiveEntry::Finished && E.RegisteredAt >= World - 60.0 * 1440.0 && E.RegisteredAt <= World)
		{
			IdeaResult R;
			R.Name = E.Name;
			R.At = E.RegisteredAt;
			R.Flags = session_video3_detail::FlagsOf(E.Name, E.Place, E.PrizeCents, true);
			X.Results.push_back(R);
		}
	}
	return Searches(X, S.Life.Video, Ideas, WeekTrends(S, World), WeekTrends(S, World - 7.0 * 1440.0), World, Count);
}
} // namespace video
} // namespace ss
