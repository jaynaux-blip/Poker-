// kast_growth: the Room and the careers (SPEC_CREATORS §9.1; core-kastroom's file), through the real kast::Channel and Stream
// driven on AdvanceTo's world-minute grid. HonestCount (the moved kast_count checks, strengthened: every named line's speaker
// present at its world minute and the count at least one when it posts, spam included, at every pace), PaceInvariance, FrameRate,
// FirstMinutes, LongMtt, the exploit closures of §7, Saves (the SaveView round trip, 1 x 30 offline days == 30 x 1), and the
// careers behind S-K and the bands. Called from kastgrowth_main.cpp (the kit is KastGrowthSeg.h). The Room's state is read
// through RoomState::Of (KastRoom.h, by its relative path).
#include "KastGrowthSeg.h"

#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/KastCopy.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/KastRoom.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <future>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace kastgrowth_test
{
namespace room_seg
{
namespace kast = ss::kast;
namespace growth = ss::kast::growth;

constexpr double RoomDay = 1440.0;

/** The stream kit StreamGrind uses: a desktop, fiber, a 1080p cam, a mic, a light, an overlay, a second monitor. */
ss::gear::Effects RoomRig()
{
	ss::gear::Owned Own;
	for (const char* Id : {"tower-mid", "fiber", "webcam-1080", "mic-usb", "ring-light", "overlay-pack", "monitor-24"})
	{
		Own[Id] = 0.0;
	}
	return ss::gear::Sum(Own);
}

/** The inputs of a night at a table, World its go-live. */
kast::Inputs RoomInputs(double World, uint64_t Seed)
{
	kast::Inputs In;
	In.World = World;
	In.Real = 1000.0;
	In.AtTable = true;
	In.Tables = 1;
	In.Hero = "grinder_3c";
	In.EventName = "Night Owl";
	In.Gear = RoomRig();
	In.Seed = Seed;
	In.RentsPaid = 2;
	In.ThisTable = 0;
	In.FrontTable = 0;
	return In;
}

/** A channel some streams in with this many members (casual to superfan, seen a day ago) and Followers in all (the rest lurk). */
kast::Channel RoomCommunity(int Members, int Followers, int Streams, double World)
{
	kast::Channel Ch;
	for (int K = 0; K < Members; ++K)
	{
		kast::Member M;
		M.Name = "kastfan_" + std::to_string(K);
		M.Loyalty = 0.05 + 0.85 * static_cast<double>(K % 10) / 9.0;
		M.Affinity = 0.9;
		M.Follower = true;
		M.Streams = 10;
		M.FirstSeen = World - 60.0 * RoomDay;
		M.LastSeen = World - RoomDay;
		Ch.Members.push_back(M);
	}
	Ch.Followers = std::max(Members, Followers);
	Ch.Streams = Streams;
	Ch.Milestone = 10000000; // no milestone alerts for a channel that got here off screen
	Ch.LastStreamDay = static_cast<int>(std::floor(World / RoomDay)) - 1;
	Ch.HeatAt = World - RoomDay;
	Ch.ProcessedDay = static_cast<int>(std::floor(World / RoomDay));
	Ch.LastOffline = World - 1.0;
	return Ch;
}

struct RoomMoment
{
	double At = 0.0; // uptime
	kast::Moment Kind = kast::Moment::AllIn;
	int Table = 0;
};

/** A pace as the frames see it: world minutes a frame (0: a sprint's hand-sized frames), real seconds a world minute. */
struct RoomPace
{
	const char* Name = "";
	double FrameWorld = 1.0 / 32.0;
	double RealPerWorld = 13.1;
	bool Attended = true;
	int WatchSpeed = 1;
	bool Sprinting = false;
};

double RoomFrame(const RoomPace& P, int Step)
{
	if (P.FrameWorld > 0.0)
	{
		return P.FrameWorld;
	}
	static const double Hands[] = {0.75, 1.5, 2.25, 1.0, 0.5, 1.75, 1.25, 2.0};
	return Hands[static_cast<size_t>(Step) % 8];
}

/** What the HonestCount checks saw. */
struct RoomWatch
{
	int Frames = 0;
	int Lines = 0;       // named lines that went up
	int Absent = 0;      // ...whose speaker wasn't in the room when it went up
	int AtZero = 0;      // ...while the count was 0
	int Landed = 0;      // named lines that landed (At passed)
	int LandedBad = 0;   // ...with the speaker gone or the count 0 at that frame
	int ChatOver = 0;    // frames where "chatting" exceeded "watching"
	int TimedOutSpoke = 0;
	int Spam = 0;
	int CountMismatch = 0; // frames where the View, the old mirror and the Room disagreed
	std::string First;
	int LastId = 0;
	double LastReal = -1.0e9;
	std::map<std::string, double> OutAt; // name -> the real second of their timeout notice
};

bool RoomNamed(const kast::ChatMsg& M)
{
	return !M.Who.empty() && M.Kind != kast::LineKind::Streamer && M.Kind != kast::LineKind::System && M.Kind != kast::LineKind::Raid;
}

/** After a frame (or a moment, or a verb): the lines that went up since the last look, and the ones that landed. */
void RoomObserve(const kast::Stream& St, RoomWatch& W, double Real)
{
	const kast::RoomState* R = kast::RoomState::Of(St);
	if (!St.Live || !R)
	{
		return;
	}
	int MaxId = W.LastId;
	for (const kast::ChatMsg& M : St.Chat)
	{
		MaxId = std::max(MaxId, M.Id);
		if (M.Kind == kast::LineKind::System && M.Id > W.LastId)
		{
			const size_t Cut = std::min(M.Text.find(" has been timed out"), M.Text.find(" was timed out by "));
			if (Cut != std::string::npos)
			{
				W.OutAt[M.Text.substr(0, Cut)] = M.At;
			}
		}
		if (!RoomNamed(M))
		{
			continue;
		}
		if (M.Id > W.LastId)
		{
			++W.Lines;
			W.Spam += M.Spam ? 1 : 0;
			const bool Here = R->Present(M.Who);
			W.Absent += Here ? 0 : 1;
			W.AtZero += R->Watchers < 1 ? 1 : 0;
			if ((!Here || R->Watchers < 1) && W.First.empty())
			{
				W.First = M.Who + ": " + M.Text;
			}
			const auto It = W.OutAt.find(M.Who);
			if (It != W.OutAt.end() && M.At > It->second && M.At - It->second < 600.0)
			{
				++W.TimedOutSpoke;
			}
		}
		if (M.At > W.LastReal && M.At <= Real)
		{
			++W.Landed;
			W.LandedBad += !R->Present(M.Who) || R->Watchers < 1 ? 1 : 0;
		}
	}
	W.LastId = MaxId;
	W.LastReal = Real;
	++W.Frames;
	const kast::View& V = St.GetView();
	W.ChatOver += V.Chatting > V.Count ? 1 : 0;
	W.CountMismatch += V.Count != R->Watchers || static_cast<int>(St.Viewers) != R->Watchers ? 1 : 0;
}

/** The View's facts against their plain definitions, right after a Tick (review, 2026-10-25 evening: the frame-budget rewrite
 *  traded the name sets for index stamps, flags and caches; each fact is recomputed here the obvious way). Presence is
 *  PresentNames' (the visits, guests and raiders walked directly), not Present's name lookups, so a lookup that drifts from the
 *  room shows up too. Nodded is the test's own record of its Nods. */
struct RoomViewRef
{
	int Frames = 0;
	int ChatOff = 0;    // "N chatting" != the last two real minutes' speakers still here, one a person, at most the count
	int FlagOff = 0;    // MemberFlags != each member's bits (1 regular, 2 mod, 4 Nodded, 8 here)
	int PresentOff = 0; // Present(name) != (name in PresentNames), over the members and the speakers
	int BadgeOff = 0;   // a member's new line: the VIP badge != Affiliate, not a mod, loyalty 0.7+ (names near the line skipped)
	int Badged = 0;     // member lines whose badges were checked
	int LastId = 0;
	std::string First;
};

void RoomViewCheck(const kast::Stream& St, const kast::Channel& Ch, double Real, const std::set<std::string>& Nodded, RoomViewRef& Ref)
{
	const kast::RoomState* R = kast::RoomState::Of(St);
	if (!St.Live || !R || !R->RoomLive)
	{
		return;
	}
	++Ref.Frames;
	auto Note = [&Ref](int& Counter, const std::string& What) {
		++Counter;
		if (Ref.First.empty())
		{
			Ref.First = What;
		}
	};
	const std::vector<std::string> Names = R->PresentNames();
	std::set<std::string> Here(Names.begin(), Names.end());
	{
		// PresentNames lists the people the games may pick (members, strangers, named raiders); trolls and spam bots are here too
		// (they talk, and Present knows them): their names from the guest list, the pool's through PoolAt turned around.
		std::vector<const std::string*> PoolName(R->PoolAt.size(), nullptr);
		for (const auto& P : R->PoolAt)
		{
			if (P.second >= 0 && static_cast<size_t>(P.second) < PoolName.size())
			{
				PoolName[static_cast<size_t>(P.second)] = &P.first;
			}
		}
		for (const kast::RoomState::Guest& G : R->Guests)
		{
			if (G.Kind != 0 && R->GuestHere(G, R->Clock))
			{
				const std::string* N = G.Pool >= 0 && static_cast<size_t>(G.Pool) < PoolName.size() ? PoolName[static_cast<size_t>(G.Pool)] : G.Pool < 0 ? &G.Bot : nullptr;
				if (N && !N->empty())
				{
					Here.insert(*N);
				}
			}
		}
	}
	const kast::View& V = St.GetView();
	std::set<std::string> Chatting;
	for (const kast::RoomState::Spoken& P : R->Spoke)
	{
		if (P.Who.empty())
		{
			continue;
		}
		const bool In = Here.count(P.Who) != 0;
		if (R->Present(P.Who) != In)
		{
			Note(Ref.PresentOff, "present: " + P.Who);
		}
		if (P.At <= Real && P.At >= Real - 120.0 && In)
		{
			Chatting.insert(P.Who);
		}
	}
	const int Want = std::min(R->Watchers, static_cast<int>(Chatting.size()));
	if (V.Chatting != Want)
	{
		Note(Ref.ChatOff, "chatting " + std::to_string(V.Chatting) + ", the speakers still here " + std::to_string(Want));
	}
	if (V.MemberFlags.size() != Ch.Members.size())
	{
		Note(Ref.FlagOff, "member flags: " + std::to_string(V.MemberFlags.size()) + " for " + std::to_string(Ch.Members.size()) + " members");
	}
	else
	{
		for (size_t I = 0; I < Ch.Members.size(); ++I)
		{
			const kast::Member& M = Ch.Members[I];
			uint8_t F = M.Loyalty >= kast::RegularLoyalty && !M.Friend ? 1 : 0;
			const int Vi = I < R->VisitOf.size() ? R->VisitOf[I] : -1;
			if (Vi >= 0)
			{
				const kast::RoomState::Visit& Vs = R->Visits[static_cast<size_t>(Vi)];
				F |= Vs.Mod ? 2 : 0;
				F |= Nodded.count(M.Name) != 0 ? 4 : 0;
				F |= kast::RoomState::HereAt(Vs, R->Clock) ? 8 : 0;
			}
			else if (Ch.IsMod(M.Name))
			{
				F |= 2;
			}
			if (V.MemberFlags[I] != F)
			{
				Note(Ref.FlagOff, "member flags: " + M.Name + " " + std::to_string(V.MemberFlags[I]) + ", the member's bits " + std::to_string(F));
				break;
			}
			if (R->Present(M.Name) != (Here.count(M.Name) != 0))
			{
				Note(Ref.PresentOff, "present: member " + M.Name);
				break;
			}
		}
	}
	int MaxId = Ref.LastId;
	for (const kast::ChatMsg& M : St.Chat)
	{
		MaxId = std::max(MaxId, M.Id);
		if (M.Id <= Ref.LastId || !RoomNamed(M) || (M.Kind != kast::LineKind::Chat && M.Kind != kast::LineKind::Question))
		{
			continue; // a sub's line carries its own badges (BadgeSub, the gifter's)
		}
		const kast::Member* Mb = Ch.FindMember(M.Who);
		if (!Mb || Ch.IsMod(M.Who) || std::fabs(Mb->Loyalty - kast::SuperfanLoyalty) < 0.02)
		{
			continue;
		}
		++Ref.Badged;
		const bool Vip = (M.Badges & kast::BadgeVip) != 0;
		if (Vip != (Ch.Affiliate && Mb->Loyalty >= kast::SuperfanLoyalty))
		{
			Note(Ref.BadgeOff, "badges: " + M.Who + ": " + M.Text);
		}
	}
	Ref.LastId = MaxId;
}

/** Everything the channel saves (and a few counters it doesn't), every double to the bit: what PaceInvariance and FrameRate
 *  compare. */
std::string RoomDigest(const kast::Channel& Ch)
{
	std::string S;
	char Buf[96];
	auto N = [&](double V) {
		std::snprintf(Buf, sizeof(Buf), "%.17g|", V);
		S += Buf;
	};
	for (const double V : {static_cast<double>(Ch.Followers), Ch.FollowFrac, static_cast<double>(Ch.Streams), Ch.Xp, Ch.Heat, Ch.HeatAt, Ch.Norm, Ch.Bias, Ch.RunNorm,
			 static_cast<double>(Ch.BestAvg), Ch.PingAt, static_cast<double>(Ch.LastStreamDay), static_cast<double>(Ch.ShowWeek), static_cast<double>(Ch.ShowDays),
			 static_cast<double>(Ch.GainWeek), Ch.GainUsed, static_cast<double>(Ch.FtDays), static_cast<double>(Ch.FtBase), static_cast<double>(Ch.LiveDay),
			 static_cast<double>(Ch.GrantDay), static_cast<double>(Ch.GrantMask), Ch.WelcomeRaidDone ? 1.0 : 0.0, static_cast<double>(Ch.CotnDay),
			 static_cast<double>(Ch.PendingCents), static_cast<double>(Ch.UnpaidCents), static_cast<double>(Ch.EarnedSubs), static_cast<double>(Ch.EarnedBits),
			 static_cast<double>(Ch.EarnedTips), static_cast<double>(Ch.EarnedAds), static_cast<double>(Ch.EarnedSponsors), Ch.CarryFollow, Ch.CarryWord, Ch.CarrySub,
			 Ch.CarryTip, Ch.CarryCheer, Ch.CarryRaid, Ch.CarryGift, static_cast<double>(Ch.Firsts), static_cast<double>(Ch.RaidsIn), static_cast<double>(Ch.RaidsOut),
			 static_cast<double>(Ch.Milestone), static_cast<double>(Ch.Peak), Ch.MinutesLive, Ch.ViewerMinutes, static_cast<double>(Ch.Stage), Ch.Affiliate ? 1.0 : 0.0,
			 Ch.Partner ? 1.0 : 0.0, static_cast<double>(Ch.Streak), static_cast<double>(Ch.Week), static_cast<double>(Ch.WeekStreams), Ch.WeekMinutes,
			 static_cast<double>(Ch.WeekAnswers), static_cast<double>(Ch.WeekRegulars), static_cast<double>(Ch.GiftedSubs)})
	{
		N(V);
	}
	for (const int V : Ch.LiveMin)
	{
		N(V);
	}
	for (const kast::Member& M : Ch.Members)
	{
		S += M.Name + ":";
		for (const double V : {M.Loyalty, M.Affinity, M.FirstSeen, M.LastSeen, static_cast<double>(M.Streams), M.WatchMinutes, static_cast<double>(M.Messages),
				 M.Follower ? 1.0 : 0.0, M.Friend ? 1.0 : 0.0, static_cast<double>(M.Given), M.Bonded ? 1.0 : 0.0, M.Noticed ? 1.0 : 0.0, static_cast<double>(M.GaveDay),
				 M.Hw, static_cast<double>(M.HwDay)})
		{
			N(V);
		}
	}
	for (const kast::Subscriber& Sb : Ch.Subs)
	{
		S += Sb.Name + ":";
		N(Sb.Since);
		N(Sb.Renews);
		N(Sb.Months);
		N(Sb.Gift ? 1.0 : 0.0);
	}
	for (const kast::Clip& C : Ch.Clips)
	{
		S += C.Title + ":" + C.By + ":";
		N(C.At);
		N(C.Views);
		N(C.Reach);
		N(C.Kind);
	}
	for (const kast::StreamLog& L : Ch.Log)
	{
		for (const double V : {L.Start, L.Minutes, static_cast<double>(L.Avg), static_cast<double>(L.Peak), static_cast<double>(L.Follows), static_cast<double>(L.Subs),
				 static_cast<double>(L.Cents), static_cast<double>(L.Returning), L.OnSchedule ? 1.0 : 0.0, static_cast<double>(L.AvgOwn), L.PeakAt,
				 static_cast<double>(L.PeakMoment), L.HeatAfter})
		{
			N(V);
		}
	}
	for (const auto& G : Ch.Goodwill)
	{
		S += G.first + "=" + std::to_string(G.second) + ";";
	}
	for (const auto& G : Ch.RaidedDay)
	{
		S += G.first + "@" + std::to_string(G.second) + ";";
	}
	for (const kast::Moderator& M : Ch.Mods)
	{
		S += M.Name + "#" + std::to_string(M.Actions) + ";";
	}
	return S;
}

/** The room at a world minute: who is here by kind. */
struct RoomAt
{
	int Members = 0, Guests = 0, Crowd = 0, Raiders = 0, Lurkers = 0, Count = 0;
	bool operator==(const RoomAt& O) const
	{
		return Members == O.Members && Guests == O.Guests && Crowd == O.Crowd && Raiders == O.Raiders && Lurkers == O.Lurkers && Count == O.Count;
	}
};

/** The grid's whole state at a minute (the debugging view PaceInvariance prints on a difference). */
std::string RoomGridDigest(const kast::Stream& St, const kast::Channel& Ch)
{
	const kast::RoomState* R = kast::RoomState::Of(St);
	std::string S;
	char Buf[160];
	std::snprintf(Buf, sizeof(Buf), "hype %.17g engage %.17g health %.17g followers %d word %.17g tip %.17g done %d|", St.Hype, St.Engage, St.Health, Ch.Followers, Ch.CarryWord,
		Ch.CarryTip, R ? R->Done : -1);
	S += Buf;
	if (!R)
	{
		return S;
	}
	for (const kast::RoomState::Visit& V : R->Visits)
	{
		std::snprintf(Buf, sizeof(Buf), "v%d %d %.17g %.17g %.17g %.17g|", V.Member, V.Here0 ? 1 : 0, V.SlotStart, V.Flip, V.Pending, V.Watched);
		S += Buf;
	}
	for (const kast::RoomState::Guest& G : R->Guests)
	{
		std::snprintf(Buf, sizeof(Buf), "g%d %.17g %.17g %d %d|", G.Pool, G.Arrive, G.Leave, G.FollowAt, G.Gone ? 1 : 0);
		S += Buf;
	}
	return S;
}

RoomAt RoomNow(const kast::Stream& St)
{
	const kast::RoomState* R = kast::RoomState::Of(St);
	RoomAt A;
	if (R)
	{
		A.Members = R->MembersHere;
		A.Guests = R->GuestsHere;
		A.Crowd = R->CrowdHere;
		A.Raiders = R->RaidersHere;
		A.Lurkers = R->LurkersNow;
		A.Count = R->Watchers;
	}
	return A;
}

/** What a night run returns: the room at every whole minute a frame ended on, the checks, the summary. */
struct RoomNight
{
	std::map<int, RoomAt> ByMinute;
	std::map<int, std::string> Grid; // the grid's state at each whole minute a frame ended on (when traced)
	RoomWatch Watch;
	RoomViewRef Ref; // the View's facts after every Tick (when observed)
	kast::Summary Last;
	std::string Digest;
};

/** One night from World0: frames at the pace (split at each moment's stamp, each Engage stamp and every 30-minute mark, where
 *  Hold may change the inputs), the checks after every frame and every moment. Engage is the streamer's: it changes only at its
 *  stamps (after the frame that ends there, as a click would), and the room fades it on the world clock in between. */
RoomNight RoomRunNight(kast::Channel& Ch, kast::Stream& St, kast::Inputs In, const RoomPace& P, const std::vector<RoomMoment>& Moments, double Minutes,
	const std::function<void(kast::Inputs&, kast::Stream&, double)>& Hold, bool Observe = true, bool Display = true, bool Trace = false,
	const std::vector<std::pair<double, double>>& EngageAt = {})
{
	RoomNight Out;
	const double W0 = In.World;
	In.Attended = P.Attended;
	In.WatchSpeed = P.WatchSpeed;
	In.Sprinting = P.Sprinting;
	Hold(In, St, 0.0);
	St.Start(Ch, In);
	double T = 0.0;
	size_t Mi = 0;
	size_t Ei = 0;
	int Step = 0;
	while (Ei < EngageAt.size() && EngageAt[Ei].first <= 0.0)
	{
		St.Engage = EngageAt[Ei++].second;
	}
	if (Observe)
	{
		RoomObserve(St, Out.Watch, In.Real);
	}
	while (T < Minutes - 1.0e-12)
	{
		double Next = T + RoomFrame(P, Step++);
		Next = std::min(Next, std::floor(T / 30.0 + 1.0e-9) * 30.0 + 30.0);
		if (Mi < Moments.size())
		{
			Next = std::min(Next, std::max(T, Moments[Mi].At));
		}
		if (Ei < EngageAt.size())
		{
			Next = std::min(Next, std::max(T, EngageAt[Ei].first));
		}
		Next = std::min(Next, Minutes);
		const double DT = Next - T;
		In.World = W0 + Next;
		In.Real += DT * P.RealPerWorld;
		Hold(In, St, Next);
		if (Display)
		{
			St.Tick(Ch, In, DT, DT * P.RealPerWorld);
		}
		else
		{
			St.AdvanceTo(Ch, In);
		}
		if (Observe)
		{
			RoomObserve(St, Out.Watch, In.Real);
			if (Display)
			{
				RoomViewCheck(St, Ch, In.Real, {}, Out.Ref);
			}
		}
		if (std::floor(Next) == Next)
		{
			Out.ByMinute[static_cast<int>(Next)] = RoomNow(St);
			if (Trace)
			{
				Out.Grid[static_cast<int>(Next)] = RoomGridDigest(St, Ch);
			}
		}
		T = Next;
		while (Ei < EngageAt.size() && EngageAt[Ei].first <= T + 1.0e-12)
		{
			St.Engage = EngageAt[Ei++].second;
		}
		while (Mi < Moments.size() && Moments[Mi].At <= T + 1.0e-12)
		{
			In.ThisTable = Moments[Mi].Table;
			St.OnMoment(Ch, In, Moments[Mi].Kind, "somebody", 1.0);
			In.ThisTable = In.FrontTable;
			if (Observe)
			{
				RoomObserve(St, Out.Watch, In.Real);
			}
			++Mi;
		}
		St.Notices.clear();
		St.Deals.clear();
	}
	Out.Last = St.Stop(Ch, In);
	Out.Digest = RoomDigest(Ch);
	return Out;
}

/** A night's moments: a register, all-ins and their results, big pots, a knockout, levels; a deep run's stages when Deep. */
std::vector<RoomMoment> RoomScript(double Minutes, bool Deep, int Tables, uint64_t Seed)
{
	std::vector<RoomMoment> V;
	V.push_back({0.5, kast::Moment::Register, 0});
	ss::Rng R("kast-room-script-" + std::to_string(Seed));
	for (double T = 3.0; T < Minutes - 1.0; T += 2.0 + std::floor(R.Next() * 16.0) / 4.0)
	{
		const int Table = Tables > 1 ? R.Int(Tables) : 0;
		const double U = R.Next();
		if (U < 0.35)
		{
			V.push_back({T, kast::Moment::AllIn, Table});
			V.push_back({T + 0.25, R.Chance(0.6) ? kast::Moment::WonAllIn : kast::Moment::LostAllIn, Table});
		}
		else if (U < 0.55)
		{
			V.push_back({T, kast::Moment::BigPot, Table});
		}
		else if (U < 0.7)
		{
			V.push_back({T, kast::Moment::Knockout, Table});
		}
		else
		{
			V.push_back({T, kast::Moment::LevelUp, Table});
		}
	}
	if (Deep)
	{
		V.push_back({Minutes * 0.45, kast::Moment::Bubble, 0});
		V.push_back({Minutes * 0.5, kast::Moment::InTheMoney, 0});
		V.push_back({Minutes * 0.75, kast::Moment::FinalTable, 0});
		V.push_back({Minutes * 0.85, kast::Moment::Top3, 0});
	}
	std::stable_sort(V.begin(), V.end(), [](const RoomMoment& A, const RoomMoment& B) { return A.At < B.At; });
	return V;
}

/** The deepest run as the night goes, for Deep scripts: an 800-runner $3.30 field reaching the final table at 75%. The field
 *  moves on each half hour of uptime (Coarse: the inputs stay piecewise constant between frame boundaries every pace shares, as
 *  PaceInvariance's traces must), or on every frame. */
void RoomDeepRun(kast::Inputs& In, double T, double Minutes, bool Coarse = false)
{
	const int N = 800;
	// Coarse: a frame that ends on a half hour still carries the half hour before it (the minutes it runs are that one's).
	const double At = Coarse ? std::max(0.0, std::floor((T - 1.0e-9) / 30.0) * 30.0) : T;
	const double Frac = std::min(1.0, At / (Minutes * 0.9));
	const int Left = std::max(3, static_cast<int>(std::lround(static_cast<double>(N) * std::pow(static_cast<double>(3) / N, Frac))));
	In.DeepTable = 0;
	In.DeepEntrants = N;
	In.Entrants = N;
	In.DeepRemaining = Left;
	In.Remaining = Left;
	In.DeepTableSize = 9;
	In.DeepEvent = "$3.30 Headhunter";
	In.BuyInCents = 330;
	In.Prestige = growth::Prestige(N, 330, 0.0);
	In.Depth = growth::Depth(N, Left);
}

// ------------------------------------------------------------------ HonestCount

void HonestCount()
{
	const double W0 = 1440.0 * 12.0 + 21.0 * 60.0; // a Friday night, 9 PM
	const RoomPace Smart{"smart", 0.0, 13.1, true, 1, false};
	struct Case
	{
		const char* Name;
		RoomPace Pace;
		int Members, Followers, Streams;
		bool Deep;
		double Minutes;
	};
	RoomPace SmartP = Smart;
	SmartP.FrameWorld = 1.0 / 30.0 / 13.1;
	RoomPace Full{"full", 1.0 / 30.0 / 33.6, 33.6, true, 1, false};
	RoomPace W4{"watch4x", 1.0 / 30.0 / 3.37, 3.37, false, 4, false};
	RoomPace W8{"watch8x", 1.0 / 30.0 / 1.1, 1.1, false, 8, true};
	RoomPace Skip{"skip", 1.0 / 30.0 / 0.7, 0.7, false, 1, true};
	RoomPace Sprint{"sprint", 0.0, 0.7, false, 1, true};
	const Case Cases[] = {
		{"first stream", SmartP, 0, 0, 0, false, 75.0},
		{"small, watch 4x", W4, 12, 40, 30, true, 180.0},
		{"small, a skip", Skip, 12, 40, 30, true, 180.0},
		{"community, 8x", W8, 60, 300, 40, true, 200.0},
		{"community, sprint", Sprint, 60, 300, 40, true, 300.0},
		{"big, full", Full, 300, 3000, 120, false, 40.0},
		{"lurkers and mods, smart", SmartP, 0, 400, 30, false, 120.0},
		{"empty room, smart", SmartP, 0, 0, 30, false, 60.0},
	};
	for (const Case& C : Cases)
	{
		kast::Channel Ch = C.Members > 0 || C.Followers > 0 || C.Streams > 0 ? RoomCommunity(C.Members, C.Followers, C.Streams, W0) : kast::Channel();
		if (std::string(C.Name).find("mods") != std::string::npos)
		{
			for (int K = 0; K < 2; ++K)
			{
				kast::Moderator M;
				M.Name = "kastmod_" + std::to_string(K);
				M.Online = 0.95;
				Ch.Mods.push_back(M);
			}
		}
		kast::Stream St("kast-honest");
		const std::vector<RoomMoment> Moments = RoomScript(C.Minutes, C.Deep, 1, 7);
		const double Minutes = C.Minutes;
		const bool Deep = C.Deep;
		RoomNight Night = RoomRunNight(Ch, St, RoomInputs(W0, 77), C.Pace, Moments, C.Minutes, [&](kast::Inputs& In, kast::Stream&, double T) {
			if (Deep)
			{
				RoomDeepRun(In, T, Minutes);
			}
		});
		const RoomWatch& W = Night.Watch;
		std::printf("  honest count (%s): %d frames, %d named lines (%d spam), %d landed; absent %d, at 0 %d, landed bad %d, chatting > watching %d, timed out and spoke %d, "
					"count mismatches %d; peak %d%s%s\n",
			C.Name, W.Frames, W.Lines, W.Spam, W.Landed, W.Absent, W.AtZero, W.LandedBad, W.ChatOver, W.TimedOutSpoke, W.CountMismatch, Night.Last.Peak,
			W.First.empty() ? "" : "; first: ", W.First.c_str());
		Expect(W.Absent == 0 && W.AtZero == 0, std::string("honest count: every named line's speaker is here, and the count is 1+, when it goes up (") + C.Name + ")");
		Expect(W.LandedBad == 0, std::string("honest count: and when it lands (") + C.Name + ")");
		Expect(W.ChatOver == 0, std::string("honest count: chatting never exceeds watching (") + C.Name + ")");
		Expect(W.TimedOutSpoke == 0, std::string("honest count: a timed-out name never speaks in its ten minutes (") + C.Name + ")");
		Expect(W.CountMismatch == 0, std::string("honest count: the View, the old mirror and the Room draw one integer (") + C.Name + ")");
		const RoomViewRef& Rf = Night.Ref;
		if (Rf.ChatOff + Rf.FlagOff + Rf.PresentOff + Rf.BadgeOff > 0)
		{
			std::printf("    the View's facts (%s): chatting off %d, member flags off %d, presence off %d, badges off %d of %d frames; first: %s\n", C.Name, Rf.ChatOff, Rf.FlagOff,
				Rf.PresentOff, Rf.BadgeOff, Rf.Frames, Rf.First.c_str());
		}
		Expect(Rf.Frames > 0 && Rf.ChatOff == 0 && Rf.FlagOff == 0, std::string("honest count: \"N chatting\" and the member flags are their plain definitions every frame (") + C.Name + ")");
		Expect(Rf.PresentOff == 0 && Rf.BadgeOff == 0, std::string("honest count: Present is the room's presence; a member's VIP badge is theirs (") + C.Name + ")");
		if (std::string(C.Name) == "community, sprint" || std::string(C.Name) == "big, full")
		{
			Expect(W.Lines > 50, std::string("honest count: chat talks (") + C.Name + ")");
		}
	}
	// The grid alone (AdvanceTo with no display step between, as a moment's Recount is): the lines the grid sets off go up only
	// from people here (the display's purge of a line whose speaker left is the second guard, not the first). (Review, 2026-10-25.)
	{
		kast::Channel Ch = RoomCommunity(60, 300, 40, W0);
		kast::Stream St("kast-honest-grid");
		const RoomNight Night = RoomRunNight(Ch, St, RoomInputs(W0, 78), Sprint, RoomScript(300.0, true, 1, 8), 300.0,
			[&](kast::Inputs& In, kast::Stream&, double T) { RoomDeepRun(In, T, 300.0); }, true, false);
		std::printf("  honest count (the grid alone, sprint): %d named lines; absent %d, at 0 %d\n", Night.Watch.Lines, Night.Watch.Absent, Night.Watch.AtZero);
		Expect(Night.Watch.Lines > 20 && Night.Watch.Absent == 0 && Night.Watch.AtZero == 0, "honest count: with no display step between, every named line the grid puts up is from someone here");
	}
	// An empty room, the moment the stream starts: every kind of moment, the prediction resolving; nobody is there to say anything.
	{
		kast::Channel Ch = RoomCommunity(0, 0, 30, W0);
		kast::Stream St("kast-honest-empty");
		kast::Inputs In = RoomInputs(W0, 5);
		St.Start(Ch, In);
		RoomWatch W;
		const int Before = static_cast<int>(St.Chat.size());
		for (int M = 0; M < static_cast<int>(kast::Moment::Count); ++M)
		{
			St.OnMoment(Ch, In, static_cast<kast::Moment>(M), "somebody");
			RoomObserve(St, W, In.Real);
		}
		St.Resolve(Ch, In, true);
		RoomObserve(St, W, In.Real);
		int Named = 0;
		for (const kast::ChatMsg& M : St.Chat)
		{
			Named += RoomNamed(M) ? 1 : 0;
		}
		Expect(St.Count() == 0 && Named == 0 && W.Absent == 0 && static_cast<int>(St.Chat.size()) >= Before, "honest count: an empty room reacts to nothing (no moment, no prediction line, Top3 included)");
	}
	// Recount before OnMoment: a moment brings the room to its world minute first (F5c).
	{
		kast::Channel Ch = RoomCommunity(20, 40, 30, W0);
		kast::Stream St("kast-honest-recount");
		kast::Inputs In = RoomInputs(W0, 9);
		St.Start(Ch, In);
		In.World = W0 + 37.5;
		In.Real += 37.5 * 13.1;
		St.OnMoment(Ch, In, kast::Moment::BigPot, "somebody");
		const kast::RoomState* R = kast::RoomState::Of(St);
		Expect(R && R->Clock == 37.5 && R->Done == 38, "honest count: a moment runs the grid to its own world minute before it fires (Recount)");
	}
}

// ------------------------------------------------------------------ PaceInvariance and FrameRate

void PaceInvariance()
{
	const double W0 = 1440.0 * 15.0 + 20.0 * 60.0 + 30.0; // a Monday, 8:30 PM
	const RoomPace Paces[] = {
		{"1x", 1.0 / 16.0, 13.1, true, 1, false},
		{"full", 1.0 / 64.0, 33.6, true, 1, false},
		{"watch 2x", 1.0 / 8.0, 6.6, true, 2, false},
		{"watch 4x", 1.0 / 4.0, 3.37, true, 4, false},
		{"watch 8x", 1.0, 1.1, true, 8, true},
		{"skip", 4.0, 0.7, true, 1, true},
		{"sprint", 0.0, 0.7, true, 1, true},
	};
	std::string Digest0;
	std::map<int, RoomAt> Room0;
	bool AllSame = true;
	bool RoomSame = true;
	std::string Diff;
	std::set<int> AlertsSent, AlertsBack;
	int ViewFrames = 0, ViewOff = 0, ViewBadged = 0;
	std::string ViewFirst;
	for (const RoomPace& P : Paces)
	{
		kast::Channel Ch = RoomCommunity(40, 160, 25, W0);
		Ch.Affiliate = true;
		Ch.ScheduleDays = 0x7f;
		Ch.ScheduleStart = 20 * 60 + 30;
		Ch.Goodwill["somebody_small"] = 2;
		kast::Stream St("kast-pace");
		const std::vector<RoomMoment> Moments = RoomScript(240.0, true, 1, 3);
		// The traces: the deep run moves each half hour; the inputs are the same at every pace (the pace's own Attended is the
		// session's, not the trace's); Engage is 0.3 from the go-live and 0.05 from minute 120, fading on the world clock between.
		const RoomNight Night = RoomRunNight(
			Ch, St, RoomInputs(W0, 4242), P, Moments, 240.0,
			[&](kast::Inputs& In, kast::Stream&, double T) {
				RoomDeepRun(In, T, 240.0, true);
				In.Attended = true;
				In.WatchSpeed = 1;
				In.Sprinting = false;
			},
			true, true, false, {{0.0, 0.3}, {120.0, 0.05}});
		// The end card's Run Alert facts are the room's at the end, the same at every pace (Close counts every returner).
		AlertsSent.insert(Night.Last.RunAlerts);
		AlertsBack.insert(Night.Last.RunAlertsBack);
		// The View's facts after every Tick (an Affiliate channel: the VIP badges too).
		ViewFrames += Night.Ref.Frames;
		ViewOff += Night.Ref.ChatOff + Night.Ref.FlagOff + Night.Ref.PresentOff + Night.Ref.BadgeOff;
		ViewBadged += Night.Ref.Badged;
		if (ViewFirst.empty() && !Night.Ref.First.empty())
		{
			ViewFirst = std::string(P.Name) + ": " + Night.Ref.First;
		}
		if (Digest0.empty())
		{
			Digest0 = Night.Digest;
			Room0 = Night.ByMinute;
			continue;
		}
		const bool Same = Night.Digest == Digest0;
		AllSame = AllSame && Same;
		for (const auto& M : Night.ByMinute)
		{
			const auto It = Room0.find(M.first);
			if (It != Room0.end() && !(It->second == M.second))
			{
				RoomSame = false;
				if (Diff.empty())
				{
					char Buf[256];
					std::snprintf(Buf, sizeof(Buf), " at minute %d: members %d/%d guests %d/%d crowd %d/%d raiders %d/%d lurkers %d/%d", M.first, M.second.Members, It->second.Members,
						M.second.Guests, It->second.Guests, M.second.Crowd, It->second.Crowd, M.second.Raiders, It->second.Raiders, M.second.Lurkers, It->second.Lurkers);
					Diff = std::string(P.Name) + Buf;
				}
			}
		}
		if (!Same && Diff.empty())
		{
			size_t I = 0;
			while (I < Digest0.size() && I < Night.Digest.size() && Digest0[I] == Night.Digest[I])
			{
				++I;
			}
			Diff = std::string(P.Name) + ": the channel differs at byte " + std::to_string(I) + " (" + Night.Digest.substr(I > 40 ? I - 40 : 0, 80) + ")";
		}
		std::printf("  pace invariance (%s): %zu minutes compared, channel %s, room %s\n", P.Name, Night.ByMinute.size(), Same ? "identical" : "DIFFERS", RoomSame ? "identical" : "DIFFERS");
	}
	if (!Diff.empty())
	{
		std::printf("  pace invariance: first difference: %s\n", Diff.c_str());
		// Which half leaks: the grid alone (no display chat) at 1x and a skip, and where the grid's state first parts.
		std::string D[2];
		std::map<int, std::string> G[2];
		for (int K = 0; K < 2; ++K)
		{
			kast::Channel Ch = RoomCommunity(40, 160, 25, W0);
			Ch.Affiliate = true;
			Ch.ScheduleDays = 0x7f;
			Ch.ScheduleStart = 20 * 60 + 30;
			Ch.Goodwill["somebody_small"] = 2;
			kast::Stream St("kast-pace");
			const RoomNight N = RoomRunNight(
				Ch, St, RoomInputs(W0, 4242), Paces[K == 0 ? 0 : 5], RoomScript(240.0, true, 1, 3), 240.0,
				[&](kast::Inputs& In, kast::Stream&, double T) {
					RoomDeepRun(In, T, 240.0, true);
					In.Attended = true;
					In.WatchSpeed = 1;
					In.Sprinting = false;
				},
				false, false, true, {{0.0, 0.3}, {120.0, 0.05}});
			D[K] = N.Digest;
			G[K] = N.Grid;
		}
		std::printf("  pace invariance: the grid alone (no display) %s\n", D[0] == D[1] ? "identical: display leaks" : "DIFFERS: the grid leaks");
		for (const auto& M : G[1])
		{
			const auto It = G[0].find(M.first);
			if (It != G[0].end() && It->second != M.second)
			{
				size_t I = 0;
				while (I < M.second.size() && I < It->second.size() && M.second[I] == It->second[I])
				{
					++I;
				}
				std::printf("  pace invariance: the grid first differs at minute %d, byte %zu:\n    1x: %s\n    skip: %s\n", M.first, I, It->second.substr(I > 60 ? I - 60 : 0, 160).c_str(),
					M.second.substr(I > 60 ? I - 60 : 0, 160).c_str());
				break;
			}
		}
	}
	std::printf("  pace invariance: the end card's Run Alerts %d sent, %d back, at every pace (%zu and %zu distinct values)\n", AlertsSent.empty() ? -1 : *AlertsSent.begin(),
		AlertsBack.empty() ? -1 : *AlertsBack.begin(), AlertsSent.size(), AlertsBack.size());
	Expect(AlertsSent.size() == 1 && AlertsBack.size() == 1 && *AlertsSent.begin() > 0,
		"pace invariance: the end card's Run Alerts (sent, and who came back) are the same at every pace");
	std::printf("  pace invariance: the View's facts after %d frames at every pace: %d off (%d member lines' badges checked)%s%s\n", ViewFrames, ViewOff, ViewBadged,
		ViewFirst.empty() ? "" : "; first: ", ViewFirst.c_str());
	Expect(ViewFrames > 0 && ViewOff == 0 && ViewBadged > 100,
		"pace invariance: \"N chatting\", the member flags, presence and the VIP badges are their plain definitions after every frame at every pace");
	Expect(AllSame, "pace invariance: every Channel byte is identical at 1x, Full, watch 2x/4x/8x, a skip and a sprint");
	Expect(RoomSame, "pace invariance: the room by world minute (members, guests, crowd, raiders, lurkers) is identical at every pace");
}

void FrameRate()
{
	const double W0 = 1440.0 * 16.0 + 21.0 * 60.0;
	auto Night = [&](const RoomPace& P, int Tables, const std::vector<RoomMoment>& Moments, double Minutes) {
		kast::Channel Ch = RoomCommunity(30, 90, 20, W0);
		kast::Stream St("kast-frames");
		kast::Inputs In = RoomInputs(W0, 99);
		In.Tables = Tables;
		return RoomRunNight(Ch, St, In, P, Moments, Minutes, [&](kast::Inputs&, kast::Stream&, double) {}, false, true, false, {{0.0, 0.25}});
	};
	const std::vector<RoomMoment> Moments = RoomScript(120.0, false, 1, 11);
	const RoomNight At30 = Night({"30 fps", 1.0 / 30.0 / 13.1, 13.1, true, 1, false}, 1, Moments, 120.0);
	const RoomNight At60 = Night({"60 fps", 1.0 / 60.0 / 13.1, 13.1, true, 1, false}, 1, Moments, 120.0);
	Expect(At30.Digest == At60.Digest, "frame rate: 30 fps and 60 fps leave identical Channel bytes (display chat changes nothing)");
	const RoomNight Big = Night({"one frame", 105.0, 13.1, true, 1, false}, 1, {}, 105.0);
	const RoomNight Small = Night({"105 frames", 1.0, 13.1, true, 1, false}, 1, {}, 105.0);
	Expect(Big.Digest == Small.Digest, "frame rate: one 105-world-minute frame and 105 one-minute frames leave identical Channel bytes");
	if (Big.Digest != Small.Digest)
	{
		size_t I = 0;
		while (I < Big.Digest.size() && I < Small.Digest.size() && Big.Digest[I] == Small.Digest[I])
		{
			++I;
		}
		std::printf("    one frame: %s\n    105 frames: %s\n", Big.Digest.substr(I > 60 ? I - 60 : 0, 140).c_str(), Small.Digest.substr(I > 60 ? I - 60 : 0, 140).c_str());
	}
	// Two tables: the same moment stamps from both tables, however the frames split.
	const std::vector<RoomMoment> Two = RoomScript(120.0, false, 2, 13);
	const RoomNight TwoA = Night({"two tables, 1/8", 1.0 / 8.0, 13.1, true, 1, false}, 2, Two, 120.0);
	const RoomNight TwoB = Night({"two tables, sprint", 0.0, 0.7, true, 1, false}, 2, Two, 120.0);
	Expect(TwoA.Digest == TwoB.Digest, "frame rate: with two tables, identical moment stamps leave identical Channel bytes");
	std::printf("  frame rate: 30 vs 60 fps %s, 1 x 105 vs 105 x 1 %s, two tables %s\n", At30.Digest == At60.Digest ? "identical" : "DIFFER",
		Big.Digest == Small.Digest ? "identical" : "DIFFER", TwoA.Digest == TwoB.Digest ? "identical" : "DIFFER");
	// The Run Alert's returners are the room's, not the frames' (§1.4, §2.3.5: "12 back so far" is a fact when shown): a bubble, an
	// ITM and a final table's alerts 90 minutes apart, the stream ending 4 or 60 minutes after the last, as 1/16-minute frames,
	// one-minute frames or one frame a segment, bring the same people back, on the live strip and the end card alike. The oracle is
	// the test's own: every visit an alert sent for, from the moment the test first sees it sent for, counts once it has been here
	// since (watched past what it had then, or here now). (Review round 2, 2026-10-25: the strip counted only the returners a frame
	// saw, so an hour as one frame read 9 of 11.)
	{
		struct AlertNight
		{
			int Sent = 0, Live = 0, Back = 0, Oracle = 0, Pulled = 0, Again = 0;
		};
		auto Alerted = [&](double Frame, double Tail, uint64_t Seed) {
			kast::Channel Ch = RoomCommunity(400, 1200, 40, W0); // big enough that a later alert sends again for earlier returners
			kast::Stream St("kast-alert-back");
			kast::Inputs In = RoomInputs(W0, Seed);
			St.Start(Ch, In);
			const kast::RoomState* R = kast::RoomState::Of(St);
			std::map<size_t, double> Since; // visit -> its minutes here when the test first saw an alert sent for it
			auto Segment = [&](double Minutes) {
				const int N = static_cast<int>(std::lround(Minutes / std::min(Frame, Minutes)));
				const double DW = Minutes / static_cast<double>(N);
				for (int K = 0; K < N; ++K)
				{
					In.World += DW;
					In.Real += DW * 13.1;
					St.Tick(Ch, In, DW, std::min(1.0, DW * 13.1));
					St.Notices.clear();
				}
			};
			std::map<size_t, int> StageOf; // visit -> the stage whose alert last sent for it
			int Again = 0;
			auto Alert = [&](const char* Stage) {
				St.DebugRoom(Ch, In, std::string("moment ") + Stage);
				for (size_t I = 0; I < R->Visits.size(); ++I)
				{
					if (R->Visits[I].Pulled && Since.count(I) == 0)
					{
						Since[I] = kast::RoomState::WatchedTo(R->Visits[I], R->Clock);
					}
					else if (R->Visits[I].Pulled && StageOf[I] != R->Visits[I].PullStage)
					{
						++Again; // sent for again: an earlier alert's returner who had left
					}
					StageOf[I] = R->Visits[I].Pulled ? R->Visits[I].PullStage : -1;
				}
			};
			Segment(30.0);
			Alert("bubble");
			Segment(90.0); // long enough for some of the bubble's returners to leave again before the next alert
			Alert("itm");
			Segment(90.0);
			Alert("ft");
			Segment(Tail);
			AlertNight A;
			A.Live = St.GetView().RunAlertBack;
			A.Pulled = static_cast<int>(Since.size());
			A.Again = Again;
			for (const auto& P : Since)
			{
				const kast::RoomState::Visit& V = R->Visits[P.first];
				A.Oracle += kast::RoomState::HereAt(V, R->Clock) || kast::RoomState::WatchedTo(V, R->Clock) > P.second ? 1 : 0;
			}
			const kast::Summary S = St.Stop(Ch, In);
			A.Sent = S.RunAlerts;
			A.Back = S.RunAlertsBack;
			return A;
		};
		for (const double Tail : {4.0, 60.0})
		{
			const AlertNight Fine = Alerted(1.0 / 16.0, Tail, 4401), Steps = Alerted(1.0, Tail, 4401), One = Alerted(1.0e9, Tail, 4401);
			const std::string Why = " (the stream ending " + std::to_string(static_cast<int>(Tail)) + " minutes after the final table's alert)";
			std::printf("  frame rate: three Run Alerts, %.0f minutes after the last: %d sent, %d sent for (%d again); back by the end card %d / %d / %d (1/16-minute, one-minute, one frame a "
						"segment), the live strip %d / %d / %d, the test's own count %d / %d / %d\n",
				Tail, Steps.Sent, Steps.Pulled, Steps.Again, Fine.Back, Steps.Back, One.Back, Fine.Live, Steps.Live, One.Live, Fine.Oracle, Steps.Oracle, One.Oracle);
			Expect(Steps.Sent > 0 && Fine.Sent == Steps.Sent && One.Sent == Steps.Sent && Steps.Back > 0 && (Tail > 10.0 || Steps.Back < Steps.Pulled),
				"frame rate: the Run Alerts go out the same at every framing; some of those sent for are back (not all of them 4 minutes on)" + Why);
			Expect(Fine.Back == Fine.Oracle && Steps.Back == Steps.Oracle && One.Back == One.Oracle && Fine.Back == Steps.Back && One.Back == Steps.Back,
				"frame rate: the end card's Run Alert returners are everyone sent for who has been here since, at every framing" + Why);
			Expect(Fine.Live == Fine.Back && Steps.Live == Steps.Back && One.Live == One.Back,
				"frame rate: the live strip's \"back so far\" is the room's count at every framing (a long frame doesn't drop a returner who came and went)" + Why);
		}
		// More nights, the stream ending a minute after the final table's alert: an earlier alert's returner who had left and is sent
		// for again still counts from the first alert (the oracle's rule), back again or not.
		int Again = 0, Off = 0, Nights = 0;
		for (uint64_t Seed = 4402; Seed < 4408; ++Seed)
		{
			const AlertNight A = Alerted(1.0, 1.0, Seed);
			Again += A.Again;
			Off += A.Back != A.Oracle || A.Live != A.Back ? 1 : 0;
			++Nights;
		}
		std::printf("  frame rate: %d more nights ending a minute after the final table's alert: %d sent for again; end card or strip off the room's count on %d\n", Nights, Again, Off);
		Expect(Again > 0 && Off == 0, "frame rate: a returner sent for again counts from the first alert, on the end card and the strip");
	}
}

// ------------------------------------------------------------------ FirstMinutes

void FirstMinutes()
{
	const double W0 = 1440.0 * 3.0 + 20.0 * 60.0; // a Thursday, 8 PM (a first night)
	const int Seeds = Long ? 200 : 60;
	int DeeBy = 0, MeiAfter = 0, Both30 = 0, Zero5 = 0, FirstFollower = 0, StayedTen = 0, Ended0 = 0;
	for (int Seed = 0; Seed < Seeds; ++Seed)
	{
		kast::Channel Ch;
		kast::Stream St("kast-first-" + std::to_string(Seed));
		kast::Inputs In = RoomInputs(W0, 1000 + static_cast<uint64_t>(Seed));
		In.AtTable = false; // from the lobby
		St.Start(Ch, In);
		auto Step = [&](double T) {
			In.Real += (W0 + T - In.World) * 60.0;
			In.World = W0 + T;
			St.Tick(Ch, In, 0.0, 1.0);
		};
		Step(0.4);
		const kast::RoomState* R = kast::RoomState::Of(St);
		const bool Dee = R->Present("dee_spincycle");
		DeeBy += Dee ? 1 : 0;
		const int DeeV = R->VisitOf[static_cast<size_t>(R->MemberAt.at("dee_spincycle"))];
		const int MeiV = R->VisitOf[static_cast<size_t>(R->MemberAt.at("mei_ng"))];
		MeiAfter += MeiV >= 0 && DeeV >= 0 && R->Visits[static_cast<size_t>(MeiV)].Pending >= R->Visits[static_cast<size_t>(DeeV)].Pending ? 1 : 0;
		In.AtTable = true;
		Step(5.0);
		Zero5 += St.Count() == 0 ? 1 : 0;
		Step(30.0);
		Both30 += R->Present("dee_spincycle") && R->Present("mei_ng") ? 1 : 0;
		for (double T = 31.0; T <= 90.0; T += 1.0)
		{
			Step(T);
		}
		// The first guest who stayed 10+ world minutes (with the time to follow inside the night) followed.
		const bool Stayed = R->FirstFollowerAt >= 0 && R->FirstFollowerAt <= 90;
		StayedTen += Stayed ? 1 : 0;
		FirstFollower += Stayed && R->FirstFollowed ? 1 : 0;
		const kast::Summary S = St.Stop(Ch, In);
		Ended0 += S.Follows == 0 ? 1 : 0;
	}
	// The seed count (C-K's L7): §9.1 asks 200 first streams; the default run (ctest's, inside its 60 s) takes 60 and kast_growth long
	// the 200, so the gate's run is the spec's.
	std::printf("  first minutes (%d first streams%s): Dee by 0.4 %d, Mei after her %d, both at 30 %d, count 0 at 5 %d, a guest stayed 10+ %d, First Follower %d, ended +0 %d\n", Seeds,
		Long ? ", the spec's 200" : "; the spec's 200 run in kast_growth long", DeeBy, MeiAfter, Both30, Zero5, StayedTen, FirstFollower, Ended0);
	Expect(DeeBy == Seeds && MeiAfter == Seeds, "first minutes: Dee is here by world minute 0.4 and Mei comes after her, on every first stream");
	Expect(Both30 >= Seeds * 9 / 10, "first minutes: both friends are here at world minute 30 on 90% of first streams");
	Expect(Zero5 * 20 <= Seeds, "first minutes: the count is 0 at world minute 5 on at most 5% of first streams");
	Expect(FirstFollower == StayedTen, "first minutes: First Follower on every first stream with a guest who stayed 10+ world minutes");
}

// ------------------------------------------------------------------ Saves and the offline split

void OfflineSplit()
{
	// 1 x 30 offline days == 30 x 1 to the byte (§8.4): renewals and clip followers keyed on the world seed, the days in order.
	const double W0 = 1440.0 * 40.0 + 23.0 * 60.0;
	auto Make = [&]() {
		kast::Channel Ch = RoomCommunity(120, 900, 60, W0);
		Ch.Affiliate = true;
		Ch.ScheduleDays = 0x1f;
		for (int K = 0; K < 40; ++K)
		{
			kast::Subscriber S;
			S.Name = "kastfan_" + std::to_string(K * 3);
			S.Since = W0 - 20.0 * RoomDay;
			S.Renews = W0 + static_cast<double>(K % 30) * RoomDay * 0.9 + 37.0;
			S.Gift = K % 7 == 0;
			Ch.Subs.push_back(S);
		}
		for (int K = 0; K < 6; ++K)
		{
			kast::Clip C;
			C.Title = "clip " + std::to_string(K);
			C.By = "kastfan_1";
			C.At = W0 - static_cast<double>(K) * 600.0;
			C.Reach = 4000.0 + 900.0 * K;
			Ch.Clips.push_back(C);
		}
		return Ch;
	};
	ss::Rng R("kast-offline-split");
	kast::Channel One = Make();
	kast::Offline(One, W0 + 30.0 * RoomDay, R, 31337, 1);
	kast::Channel Steps = Make();
	for (int K = 1; K <= 30; ++K)
	{
		kast::Offline(Steps, W0 + static_cast<double>(K) * RoomDay, R, 31337, 1);
	}
	const std::string A = RoomDigest(One);
	const std::string B = RoomDigest(Steps);
	std::printf("  offline: 1 x 30 days and 30 x 1 %s; %d followers, %zu members, %zu subs, pending $%.2f, unpaid $%.2f\n", A == B ? "identical" : "DIFFER", One.Followers,
		One.Members.size(), One.Subs.size(), static_cast<double>(One.PendingCents) / 100.0, static_cast<double>(One.UnpaidCents) / 100.0);
	Expect(A == B, "offline: a skip of 30 days is 30 one-day steps to the byte (keyed renewals and clip followers, the days in order)");
	Expect(One.PendingCents > 0 && One.UnpaidCents == 0, "offline: before the second rent the renewals wait in PendingCents (C14)");
	// The SaveView round trip: every field of the Channel's appended state out and back.
	kast::Channel Ch = Make();
	Ch.Heat = 41.5;
	Ch.HeatAt = W0 - 33.0;
	Ch.Norm = 12.25;
	Ch.Bias = -0.125;
	Ch.RunNorm = 0.3;
	Ch.BestAvg = 19;
	Ch.PingAt = W0 - 100.0;
	Ch.LastStreamDay = 39;
	Ch.ShowWeek = 5;
	Ch.ShowDays = 0x15;
	Ch.GainWeek = 5;
	Ch.GainUsed = 22.5;
	Ch.FtBase = 38;
	Ch.FtDays = 5;
	Ch.LiveDay = 39;
	Ch.LiveMin = {10, 20, 30, 40, 50, 60, 70};
	Ch.GrantDay = 39;
	Ch.GrantMask = 0x55;
	Ch.WelcomeRaidDone = true;
	Ch.CotnDay = 38;
	Ch.PendingCents = 12345;
	Ch.LiveSince = W0 - 50.0;
	Ch.GainSoFar = 7.5;
	Ch.CarryFollow = 0.25;
	Ch.CarryWord = 0.5;
	Ch.CarrySub = 0.75;
	Ch.CarryTip = 0.125;
	Ch.CarryCheer = 0.375;
	Ch.CarryRaid = 0.625;
	Ch.CarryGift = 0.875;
	Ch.RerunHeat = {{W0 - 300.0, 6.0}, {W0 - 30.0, 10.0}};
	Ch.Firsts = growth::FirstStrangerTip;
	kast::Channel Back = Make();
	kast::ApplySaveView(Back, kast::SaveViewOf(Ch));
	Expect(RoomDigest(Back) == RoomDigest(Ch) && Back.RerunHeat == Ch.RerunHeat && Back.LiveSince == Ch.LiveSince && Back.GainSoFar == Ch.GainSoFar,
		"saves: SaveViewOf and ApplySaveView carry every field of the appended state");
}

// ------------------------------------------------------------------ §7: the exploits, closed

/** Whole world minutes at a table: the grid's own steps (Smart's real seconds). */
void RoomRun(kast::Stream& St, kast::Channel& Ch, kast::Inputs& In, double Minutes)
{
	for (double T = 0.0; T < Minutes - 1.0e-9; T += 1.0)
	{
		In.World += 1.0;
		In.Real += 13.1;
		St.AdvanceTo(Ch, In);
	}
}

/** The night's next go-live, Days on (real seconds move with it). */
void RoomNext(kast::Inputs& In, double World)
{
	In.Real += 4.0 * 3600.0 + (World - In.World) * 13.1;
	In.World = World;
}

void Exploits()
{
	const double W0 = RoomDay * 50.0 + 20.0 * 60.0;
	// One Ping per 4 world hours (§2.3.1) and the 60-minute rule (§7, M2): a restart 20 minutes after a stream pings nobody, and a
	// stream under 60 world minutes writes no log row, moves no Norm, Bias, RunNorm, minutes or streak and earns no token.
	{
		kast::Channel Ch = RoomCommunity(60, 400, 30, W0);
		kast::Stream St("kast-exploit-ping");
		kast::Inputs In = RoomInputs(W0, 7001);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 90.0);
		const kast::Summary LongNight = St.Stop(Ch, In);
		const size_t Logged = Ch.Log.size();
		const double Norm = Ch.Norm, Bias = Ch.Bias, RunNorm = Ch.RunNorm, Minutes = Ch.MinutesLive;
		const int Streams = Ch.Streams, WeekStreams = Ch.WeekStreams;
		RoomNext(In, In.World + 20.0);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 3.0);
		const kast::Summary ShortNight = St.Stop(Ch, In, "chipleader_carla");
		std::printf("  exploits: the go-live's ping reached %d of %d; the restart's %d (%zu log rows, then %zu)\n", LongNight.PingReached, LongNight.PingOf, ShortNight.PingReached, Logged, Ch.Log.size());
		Expect(LongNight.PingReached > 0 && ShortNight.PingReached == 0, "exploits: one Ping per 4 world hours (a restart pings nobody)");
		Expect(Ch.Log.size() == Logged && Ch.Norm == Norm && Ch.Bias == Bias && Ch.RunNorm == RunNorm && Ch.MinutesLive == Minutes && Ch.WeekStreams == WeekStreams &&
				   Ch.Streams == Streams + 1,
			"exploits: a stream under 60 world minutes counts, but writes no log row and moves no statistic or streak");
		Expect(ShortNight.RaidedOut == "chipleader_carla" && Ch.Goodwill.count("chipleader_carla") == 0 && Ch.RaidedDay.count("chipleader_carla") == 0,
			"exploits: a short stream's raid out earns no token");
	}
	// The token and the 7-day rule (§2.7.1): a 60+ minute stream that raids X gives X a token; again inside 7 world days, none.
	{
		kast::Channel Ch = RoomCommunity(60, 400, 30, W0);
		kast::Stream St("kast-exploit-token");
		kast::Inputs In = RoomInputs(W0, 7002);
		const std::string X = "chipleader_carla";
		int Given[3] = {}; // the tokens each raid out gave (a raid back from X during a stream may spend one; that's before Stop)
		const double TokenDays[3] = {0.0, 1.0, 9.0};
		const auto Tokens = [&Ch, &X]() {
			const auto It = Ch.Goodwill.find(X);
			return It == Ch.Goodwill.end() ? 0 : It->second;
		};
		for (int K = 0; K < 3; ++K)
		{
			RoomNext(In, W0 + TokenDays[K] * RoomDay);
			St.Start(Ch, In);
			RoomRun(St, Ch, In, 70.0);
			const int Before = Tokens();
			St.Stop(Ch, In, X);
			Given[K] = Tokens() - Before;
		}
		std::printf("  exploits: tokens %s got from raids out on days 0, 1 and 9: %d, %d, %d\n", X.c_str(), Given[0], Given[1], Given[2]);
		Expect(Given[0] == 1 && Given[1] == 0 && Given[2] == 1, "exploits: a raid out gives a token, and none for the same channel inside 7 world days");
	}
	// The welcome raid comes once (§2.7.1): the first stream from stream 3 that reaches world minute 40.
	{
		kast::Channel Ch = RoomCommunity(20, 60, 4, W0);
		kast::Stream St("kast-exploit-welcome");
		kast::Inputs In = RoomInputs(W0, 7003);
		int Welcomes = 0;
		for (int K = 0; K < 3; ++K)
		{
			RoomNext(In, W0 + static_cast<double>(K) * RoomDay);
			St.Start(Ch, In);
			RoomRun(St, Ch, In, 45.0);
			const int Before = Welcomes;
			const kast::Summary S = St.Stop(Ch, In);
			for (const kast::Summary::RaidHeld& H : S.RaidsHeld)
			{
				Welcomes += H.Size == 8 && !H.Broadcast ? 1 : 0;
			}
			Expect(K > 0 || Welcomes == Before + 1, "exploits: the welcome raid comes on the first stream from stream 3 that reaches minute 40");
		}
		Expect(Welcomes == 1 && Ch.WelcomeRaidDone, "exploits: the welcome raid comes once");
	}
	// The deep-run machinery needs 100+ entrants (§2.6), and its grants are once a world day (§7): a final table at a 50-runner
	// event sends no Run Alert, brings no moment raid and no Heat news; at 980 runners it does, and a second stream the same day
	// sends no second Run Alert for the stage.
	{
		kast::Channel Ch = RoomCommunity(120, 900, 30, W0);
		kast::Stream St("kast-exploit-small");
		kast::Inputs In = RoomInputs(W0, 7004);
		In.Entrants = 50;
		In.Remaining = 9;
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 30.0);
		St.OnMoment(Ch, In, kast::Moment::FinalTable, "somebody", 1.0);
		RoomRun(St, Ch, In, 40.0);
		const kast::Summary SmallFt = St.Stop(Ch, In);
		const double SmallNorm = Ch.RunNorm;
		RoomNext(In, In.World + 30.0);
		In.Entrants = 980;
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 30.0);
		St.DebugRoom(Ch, In, "moment ft 1.40 980");
		RoomRun(St, Ch, In, 40.0);
		const kast::Summary DeepFt = St.Stop(Ch, In);
		RoomNext(In, In.World + 5.0); // 22:55, the same world day (the final table comes at 23:25)
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 30.0);
		St.DebugRoom(Ch, In, "moment ft 1.40 980");
		RoomRun(St, Ch, In, 40.0);
		const kast::Summary AgainFt = St.Stop(Ch, In);
		std::printf("  exploits: a 50-runner final table: Run Alert %d, RunNorm %.3f; at 980: Run Alert %d, RunNorm %.3f; again the same day: Run Alert %d\n", SmallFt.RunAlerts, SmallNorm,
			DeepFt.RunAlerts, Ch.RunNorm, AgainFt.RunAlerts);
		Expect(SmallFt.RunAlerts == 0 && SmallNorm == 0.0, "exploits: a small field's final table is a poker moment, not a deep run (no Run Alert, no Heat news)");
		Expect(DeepFt.RunAlerts > 0 && AgainFt.RunAlerts == 0, "exploits: a stage's Run Alert goes out once a world day, restarts or not");
	}
	// C13: a sponsor read pays once a deal a world day, after 30 world minutes live with someone watching, up to 1.5x the deal's
	// reads ask a contract period, as the deal's own pay (Summary::SponsorPaid), never the Kast payout.
	{
		kast::Channel Ch = RoomCommunity(60, 400, 30, W0);
		kast::Stream St("kast-exploit-read");
		kast::Inputs In = RoomInputs(W0, 7005);
		kast::Deal D;
		D.Id = "riverline";
		D.Since = W0 - 10.0 * RoomDay;
		D.Until = W0 + 40.0 * RoomDay;
		D.ReadCents = 1000;
		D.ReadsAsk = 2;
		In.Deals = {D};
		std::vector<ss::Chips> Paid;
		int Early = -1, Twice = -1;
		ss::Chips Unpaid = 0;
		for (int K = 0; K < 4; ++K)
		{
			RoomNext(In, W0 + static_cast<double>(K) * RoomDay);
			St.Start(Ch, In);
			RoomRun(St, Ch, In, 20.0);
			const ss::Chips Soon = St.SponsorRead(Ch, In, "riverline");
			Early = K == 0 ? static_cast<int>(Soon) : Early;
			RoomRun(St, Ch, In, 15.0);
			Unpaid = Ch.UnpaidCents;
			Paid.push_back(St.SponsorRead(Ch, In, "riverline"));
			Twice = K == 0 ? static_cast<int>(St.SponsorRead(Ch, In, "riverline")) : Twice;
			const bool Booked = Ch.UnpaidCents == Unpaid;
			In.Deals = St.Deals; // the book's view carries the read (ReadAt, ReadsPaid)
			const kast::Summary S = St.Stop(Ch, In);
			Expect(Booked && (Paid.back() == 0 || (!S.SponsorPaid.empty() && S.SponsorPaid.front().Reads == 1 && S.SponsorPaid.front().ReadCents == Paid.back())),
				"exploits: a read is the deal's own pay (Summary::SponsorPaid), never the Kast payout");
		}
		std::printf("  exploits: reads at 20 minutes %d, then %lld/%lld/%lld/%lld cents on four days (the second the same day: %d)\n", Early, static_cast<long long>(Paid[0]),
			static_cast<long long>(Paid[1]), static_cast<long long>(Paid[2]), static_cast<long long>(Paid[3]), Twice);
		Expect(Early == 0 && Twice == 0 && Paid[0] == 1000 && Paid[1] == 1000 && Paid[2] == 1000 && Paid[3] == 0,
			"exploits: a read pays after 30 live minutes, once a world day, up to 1.5x the deal's reads ask");
	}
	// Affinity moves by at most +8% of its gap, ever (§2.8.2): Bonded (a final table at 100+ runners) and Noticed (a Nod) once each.
	{
		kast::Channel Ch = RoomCommunity(40, 200, 30, W0);
		for (kast::Member& M : Ch.Members)
		{
			M.Affinity = 0.3;
			M.Loyalty = 0.9;
		}
		kast::Stream St("kast-exploit-affinity");
		kast::Inputs In = RoomInputs(W0, 7006);
		In.Entrants = 980;
		for (int K = 0; K < 4; ++K)
		{
			RoomNext(In, W0 + static_cast<double>(K) * RoomDay);
			St.Start(Ch, In);
			RoomRun(St, Ch, In, 30.0);
			for (const std::string& Who : kast::RoomState::Of(St)->PresentNames())
			{
				St.Nod(Ch, Who);
			}
			St.DebugRoom(Ch, In, "moment ft 1.40 980");
			RoomRun(St, Ch, In, 40.0);
			St.Stop(Ch, In);
		}
		double Most = 0.0;
		for (const kast::Member& M : Ch.Members)
		{
			Most = M.Name.rfind("kastfan_", 0) == 0 ? std::max(Most, (M.Affinity - 0.3) / 0.7) : Most; // the 40 who were there (new members' Affinity is keyed)
		}
		std::printf("  exploits: four nights of Nods and final tables: Affinity moved at most %.1f%% of its gap\n", 100.0 * Most);
		Expect(Most > 0.0 && Most <= 0.08 + 1.0e-9, "exploits: Affinity moves by at most +8% of its gap, ever (Bonded and Noticed once each)");
	}
}

// ------------------------------------------------------------------ the review's checks (review-core-kastroom, 2026-10-25)

/** R1's spam rule on its own (the HonestCount checks can't see it: a bot counts itself): a spam bot only ever arrives in a
 *  grid minute that began with somebody in the room. Quiet small hours on an empty channel, so the room is often at 0. */
void SpamGate()
{
	const double W0 = 1440.0 * 18.0 + 2.0 * 60.0; // 2 AM
	int Spam = 0, AtZero = 0, Empty = 0, Minutes = 0;
	for (int Seed = 0; Seed < 40; ++Seed)
	{
		kast::Channel Ch = RoomCommunity(0, 0, 30, W0);
		kast::Stream St("kast-spam-" + std::to_string(Seed));
		kast::Inputs In = RoomInputs(W0, 6100 + static_cast<uint64_t>(Seed));
		In.Gear.ModBot = 0.0;
		St.Start(Ch, In);
		const kast::RoomState* R = kast::RoomState::Of(St);
		std::set<std::pair<double, std::string>> Seen;
		for (int M = 0; M < 600; ++M)
		{
			if (M > 0)
			{
				In.World = W0 + static_cast<double>(M);
				In.Real += 13.1;
				St.AdvanceTo(Ch, In);
			}
			// Minute M has run: the count now is the count its trolls' draw read (the guests it brought arrive after M).
			const int CountAtM = R->Watchers;
			Empty += CountAtM == 0 ? 1 : 0;
			++Minutes;
			for (const kast::RoomState::Guest& G : R->Guests)
			{
				if (G.Kind == 2 && G.Arrive >= static_cast<double>(M) && G.Arrive < static_cast<double>(M + 1) && Seen.insert({G.Arrive, G.Bot}).second)
				{
					++Spam;
					AtZero += CountAtM < 1 ? 1 : 0;
				}
			}
		}
		St.Stop(Ch, In);
	}
	std::printf("  spam gate: %d spam bots over %d minutes (%d of them at an empty room's minute, %d empty minutes)\n", Spam, Minutes, AtZero, Empty);
	Expect(Spam > 0 && Empty > Minutes / 5, "spam gate: the quiet rooms are often empty and still see spam when someone is there");
	Expect(AtZero == 0, "spam gate: a spam bot never arrives in a minute that began with nobody in the room (R1: bots go where viewers are)");
}

/** C6 with the pace's own flags (PaceInvariance pins Attended for every pace, so a pace term on the room would pass it): Engage
 *  held above every pace's floor each minute, the room and every Channel byte identical at 1x, watch 4x, 8x, a skip and a sprint
 *  with their real Attended, WatchSpeed and Sprinting. */
void PaceFlags()
{
	const double W0 = 1440.0 * 15.0 + 20.0 * 60.0 + 30.0;
	const RoomPace Paces[] = {
		{"1x", 1.0 / 16.0, 13.1, true, 1, false},
		{"watch 4x", 1.0 / 4.0, 3.37, false, 4, false},
		{"watch 8x", 1.0, 1.1, false, 8, true},
		{"skip", 4.0, 0.7, false, 1, true},
		{"sprint", 0.0, 0.7, false, 1, true},
	};
	std::vector<std::pair<double, double>> Engage;
	for (int M = 0; M <= 180; ++M)
	{
		Engage.emplace_back(static_cast<double>(M), 0.3);
	}
	bool Same = true;
	std::string First;
	for (int Seed = 0; Seed < 3; ++Seed)
	{
		std::string Digest0;
		for (const RoomPace& P : Paces)
		{
			kast::Channel Ch = RoomCommunity(40, 160, 25, W0);
			Ch.Affiliate = true;
			Ch.ScheduleDays = 0x7f;
			Ch.ScheduleStart = 20 * 60 + 30;
			Ch.Goodwill["somebody_small"] = 2;
			kast::Stream St("kast-pace-flags");
			const RoomNight Night = RoomRunNight(Ch, St, RoomInputs(W0, 9000 + static_cast<uint64_t>(Seed)), P, RoomScript(180.0, true, 1, 100 + static_cast<uint64_t>(Seed)), 180.0,
				[&](kast::Inputs& In, kast::Stream&, double T) { RoomDeepRun(In, T, 180.0, true); }, false, true, false, Engage);
			if (Digest0.empty())
			{
				Digest0 = Night.Digest;
				continue;
			}
			if (Night.Digest != Digest0 && First.empty())
			{
				First = std::string(P.Name) + " (seed " + std::to_string(Seed) + ")";
			}
			Same = Same && Night.Digest == Digest0;
		}
	}
	std::printf("  pace flags: the room with each pace's own flags (3 seeds) %s%s\n", Same ? "identical" : "DIFFERS at ", First.c_str());
	Expect(Same, "pace flags: with each pace's real Attended/WatchSpeed/Sprinting and Engage above every floor, every Channel byte is identical (no pace term on the room)");
}

/** The streamer's verbs and R1 (§1.5 F1, F5a): Answer's thanks only from an asker who is still here; a timeout ends the guest's
 *  or member's visit and the name says nothing more that night. */
void PlayerVerbs()
{
	const double W0 = 1440.0 * 22.0 + 20.0 * 60.0;
	int AnsweredGone = 0, ThanksFromGone = 0, AnsweredHere = 0, ThanksHere = 0, TimedOut = 0, StayedAfter = 0;
	RoomWatch W;
	for (int Seed = 0; Seed < 8 && (AnsweredGone == 0 || AnsweredHere == 0 || TimedOut < 2); ++Seed)
	{
		kast::Channel Ch = RoomCommunity(40, 200, 40, W0);
		kast::Stream St("kast-verbs-" + std::to_string(Seed));
		kast::Inputs In = RoomInputs(W0, 6200 + static_cast<uint64_t>(Seed));
		St.Start(Ch, In);
		const kast::RoomState* R = kast::RoomState::Of(St);
		W = RoomWatch();
		RoomObserve(St, W, In.Real);
		std::map<int, std::string> Asked;                    // question id -> asker
		std::map<std::string, std::pair<double, bool>> Out; // timed out tonight -> (uptime, a member)
		bool GuestOut = false, MemberOut = false;
		for (double T = 0.25; T <= 150.0; T += 0.25)
		{
			In.World = W0 + T;
			In.Real += 0.25 * 13.1;
			St.Tick(Ch, In, 0.25, 0.25 * 13.1);
			RoomObserve(St, W, In.Real);
			for (const kast::ChatMsg& M : St.Chat)
			{
				if (M.Kind == kast::LineKind::Question && !M.Answered)
				{
					Asked.emplace(M.Id, M.Who);
				}
			}
			for (auto It = Asked.begin(); It != Asked.end();)
			{
				const bool Here = R->Present(It->second);
				if ((Here && AnsweredHere < 3) || (!Here && AnsweredGone < 3))
				{
					int Before = 0;
					for (const kast::ChatMsg& M : St.Chat)
					{
						Before = std::max(Before, M.Id);
					}
					if (St.Answer(Ch, In, It->first))
					{
						bool Thanked = false;
						for (const kast::ChatMsg& M : St.Chat)
						{
							Thanked = Thanked || (M.Id > Before && M.Who == It->second && M.Kind == kast::LineKind::Chat);
						}
						(Here ? AnsweredHere : AnsweredGone) += 1;
						(Here ? ThanksHere : ThanksFromGone) += Thanked ? 1 : 0;
						RoomObserve(St, W, In.Real);
					}
					It = Asked.erase(It);
				}
				else
				{
					++It;
				}
			}
			// One guest and one member who said something, timed out (in the night's first hours).
			if (T >= 20.0 && (!GuestOut || !MemberOut))
			{
				for (const kast::ChatMsg& M : St.Chat)
				{
					if (M.Kind != kast::LineKind::Chat || M.Deleted || !R->Present(M.Who) || M.At > In.Real)
					{
						continue;
					}
					const bool IsMember = Ch.FindMember(M.Who) != nullptr;
					if ((IsMember && !MemberOut) || (!IsMember && !GuestOut))
					{
						const std::string Who = M.Who;
						if (St.Timeout(Ch, In, M.Id))
						{
							(IsMember ? MemberOut : GuestOut) = true;
							Out[Who] = {T, IsMember};
							++TimedOut;
							StayedAfter += R->Present(Who) ? 1 : 0;
							RoomObserve(St, W, In.Real);
						}
						break;
					}
				}
			}
			// A member is out for the night; a guest's name for the timeout (10 real minutes at Smart, then the pool may reuse it).
			for (const auto& O : Out)
			{
				StayedAfter += (O.second.second || T - O.second.first < 40.0) && R->Present(O.first) ? 1 : 0;
			}
		}
		St.Stop(Ch, In);
		Expect(W.Absent == 0 && W.AtZero == 0 && W.TimedOutSpoke == 0, "player verbs: every named line, answers and timeouts included, is from someone here (R1)");
	}
	std::printf("  player verbs: answered %d askers who were here (%d thanked) and %d who had left (%d thanked); %d timed out, still here after %d\n", AnsweredHere, ThanksHere, AnsweredGone,
		ThanksFromGone, TimedOut, StayedAfter);
	Expect(AnsweredGone > 0 && ThanksFromGone == 0, "player verbs: Answer's thanks never comes from an asker who left (F5a)");
	Expect(AnsweredHere > 0 && ThanksHere == AnsweredHere, "player verbs: an asker who is here says thanks");
	Expect(TimedOut >= 2 && StayedAfter == 0, "player verbs: a timeout ends the guest's or member's visit for the night (F1)");
}

/** The View's facts with the streamer's verbs in play (review round 2, 2026-10-25): two mods, three Nods, a member and a guest
 *  timed out, questions answered (a guest the answer wins follows; the asker thanks with their own badges): "N chatting", the member
 *  flags (Nodded included), presence and the VIP badges are their plain definitions after every frame (RoomViewCheck). */
void ViewVerbs()
{
	const double W0 = 1440.0 * 23.0 + 20.0 * 60.0;
	RoomViewRef Ref;
	int Nods = 0, NodsOfMembers = 0, Outs = 0, Answers = 0;
	for (int Seed = 0; Seed < 2; ++Seed)
	{
		kast::Channel Ch = RoomCommunity(80, 300, 40, W0);
		Ch.Affiliate = true;
		for (int K = 0; K < 2; ++K)
		{
			kast::Moderator Mod;
			Mod.Name = "kastfan_" + std::to_string(10 * K + 7);
			Mod.Online = 0.95;
			Ch.Mods.push_back(Mod);
		}
		kast::Stream St("kast-view-verbs-" + std::to_string(Seed));
		kast::Inputs In = RoomInputs(W0, 6500 + static_cast<uint64_t>(Seed));
		St.Start(Ch, In);
		const kast::RoomState* R = kast::RoomState::Of(St);
		std::set<std::string> Nodded;
		Ref.LastId = 0;
		bool MemberOut = false, GuestOut = false;
		for (double T = 0.25; T <= 120.0; T += 0.25)
		{
			In.World = W0 + T;
			In.Real += 0.25 * 13.1;
			St.Tick(Ch, In, 0.25, 0.25 * 13.1);
			RoomViewCheck(St, Ch, In.Real, Nodded, Ref);
			// The verbs after the check (the next Tick's View carries them): a Nod at minutes 10, 20 and 30 (a member first).
			if (Nodded.size() < 3 && T >= 10.0 * static_cast<double>(Nodded.size() + 1))
			{
				std::vector<std::string> Names = R->PresentNames();
				std::stable_partition(Names.begin(), Names.end(), [&Ch](const std::string& N) { return Ch.FindMember(N) != nullptr; });
				for (const std::string& Name : Names)
				{
					if (Nodded.count(Name) == 0 && St.Nod(Ch, Name))
					{
						Nodded.insert(Name);
						++Nods;
						NodsOfMembers += Ch.FindMember(Name) ? 1 : 0;
						break;
					}
				}
			}
			int Ask = -1;
			for (const kast::ChatMsg& M : St.Chat)
			{
				Ask = Ask < 0 && M.Kind == kast::LineKind::Question && !M.Answered && !M.Deleted && M.At <= In.Real ? M.Id : Ask;
			}
			if (Ask >= 0 && St.Answer(Ch, In, Ask))
			{
				++Answers;
			}
			if (T >= 40.0 && (!MemberOut || !GuestOut))
			{
				int Id = -1;
				bool IsMember = false;
				for (const kast::ChatMsg& M : St.Chat)
				{
					if (Id < 0 && M.Kind == kast::LineKind::Chat && !M.Deleted && M.At <= In.Real && R->Present(M.Who) && !Ch.IsMod(M.Who) && Nodded.count(M.Who) == 0)
					{
						IsMember = Ch.FindMember(M.Who) != nullptr;
						Id = (IsMember && !MemberOut) || (!IsMember && !GuestOut) ? M.Id : -1;
					}
				}
				if (Id >= 0 && St.Timeout(Ch, In, Id))
				{
					(IsMember ? MemberOut : GuestOut) = true;
					++Outs;
				}
			}
			St.Notices.clear();
			St.Deals.clear();
		}
		St.Stop(Ch, In);
	}
	std::printf("  view verbs: %d frames, %d Nods (%d of members), %d answers, %d timeouts, %d member lines' badges checked; off: chatting %d, flags %d, presence %d, badges %d%s%s\n",
		Ref.Frames, Nods, NodsOfMembers, Answers, Outs, Ref.Badged, Ref.ChatOff, Ref.FlagOff, Ref.PresentOff, Ref.BadgeOff, Ref.First.empty() ? "" : "; first: ", Ref.First.c_str());
	Expect(Nods >= 4 && NodsOfMembers >= 2 && Answers >= 2 && Outs >= 2 && Ref.Badged > 50, "view verbs: the night has Nods, answers and timeouts in it");
	Expect(Ref.ChatOff == 0 && Ref.FlagOff == 0 && Ref.PresentOff == 0 && Ref.BadgeOff == 0,
		"view verbs: with Nods, answers and timeouts, \"N chatting\", the member flags, presence and the VIP badges are their plain definitions every frame");
}

/** A guest who follows is the same person (Display's hellos, review round 2, 2026-10-25): the member a present guest becomes says
 *  no hello as if they had just walked in. Guests here for two frames running are followed one at a time (RoomState::FollowGuest,
 *  as an answered question's follow is); none of them greets the room in the next two frames with a hello only an arrival says. */
void HelloOnce()
{
	const double W0 = 1440.0 * 27.0 + 21.0 * 60.0;
	static const std::set<std::string> ArrivalOnly = {"evening", "what did I miss", "gl tonight"}; // Display's hellos no other pool has
	int Followed = 0, Greeted = 0;
	kast::Channel Ch = RoomCommunity(40, 600, 40, W0);
	kast::Stream St("kast-hello-once");
	kast::Inputs In = RoomInputs(W0, 6700);
	St.Start(Ch, In);
	const kast::RoomState* R = kast::RoomState::Of(St);
	std::set<int> HereBefore;                 // guests here at the last frame
	std::map<std::string, double> Watch;      // followed name -> the real second they followed
	std::set<int> Counted;                    // greeting lines counted (by id)
	for (double T = 0.25; T <= 240.0 && Followed < 60; T += 0.25)
	{
		In.World = W0 + T;
		In.Real += 0.25 * 13.1;
		St.Tick(Ch, In, 0.25, 0.25 * 13.1);
		for (const kast::ChatMsg& M : St.Chat)
		{
			const auto It = Watch.find(M.Who);
			if (It != Watch.end() && M.At > It->second && M.At <= It->second + 2.0 * 0.25 * 13.1 + 2.0 && ArrivalOnly.count(M.Text) != 0 && Counted.insert(M.Id).second)
			{
				++Greeted;
			}
		}
		for (auto It = Watch.begin(); It != Watch.end();)
		{
			It = In.Real - It->second > 2.0 * 0.25 * 13.1 + 2.0 ? Watch.erase(It) : std::next(It);
		}
		std::set<int> HereNow;
		int Pick = -1;
		for (size_t I = 0; I < R->Guests.size(); ++I)
		{
			const kast::RoomState::Guest& G = R->Guests[I];
			if (G.Kind == 0 && G.Pool >= 0 && R->GuestHere(G, R->Clock))
			{
				HereNow.insert(static_cast<int>(I));
				Pick = Pick < 0 && HereBefore.count(static_cast<int>(I)) != 0 ? static_cast<int>(I) : Pick;
			}
		}
		HereBefore = HereNow;
		if (Pick >= 0 && Watch.empty())
		{
			const size_t Before = Ch.Members.size();
			kast::RoomState::FollowGuest(St, Ch, Pick, R->Clock, true);
			if (Ch.Members.size() == Before + 1)
			{
				++Followed;
				Watch[Ch.Members.back().Name] = In.Real;
				HereBefore.erase(Pick);
			}
		}
		St.Notices.clear();
		St.Deals.clear();
	}
	St.Stop(Ch, In);
	std::printf("  hello once: %d present guests followed; %d of them greeted the room as an arrival\n", Followed, Greeted);
	Expect(Followed >= 40 && Greeted == 0, "hello once: a guest who follows is the same person, with no second hello as a member walking in");
}

/** The guest pool rebuilt for a new world seed on the same stream (a fresh start in the same session): PoolAt follows it, so the
 *  guests of the second night are present by their names (RoomViewCheck's presence), as the first night's were. */
void PoolReseed()
{
	const double W0 = 1440.0 * 28.0 + 21.0 * 60.0;
	const RoomPace Smart{"smart", 1.0 / 4.0, 13.1, true, 1, false};
	kast::Stream St("kast-pool-reseed");
	RoomViewRef Off[2];
	int Lines[2] = {0, 0};
	for (int K = 0; K < 2; ++K)
	{
		kast::Channel Ch = RoomCommunity(30, 300, 40, W0 + RoomDay * K);
		const RoomNight Night = RoomRunNight(Ch, St, RoomInputs(W0 + RoomDay * K, 7001 + static_cast<uint64_t>(K)), Smart, RoomScript(90.0, false, 1, 31), 90.0,
			[&](kast::Inputs&, kast::Stream&, double) {});
		Off[K] = Night.Ref;
		Lines[K] = Night.Watch.Lines;
	}
	std::printf("  pool reseed: two world seeds on one stream: %d and %d named lines; presence off %d and %d, chatting off %d and %d\n", Lines[0], Lines[1], Off[0].PresentOff,
		Off[1].PresentOff, Off[0].ChatOff, Off[1].ChatOff);
	Expect(Lines[1] > 20 && Off[0].PresentOff + Off[1].PresentOff + Off[0].ChatOff + Off[1].ChatOff + Off[1].FlagOff == 0,
		"pool reseed: a new world seed's pool is the room's pool (its guests are present by name, PoolAt rebuilt)");
}

/** The money rules on the live path: C14 (Affiliate revenue earned before the second rent waits in PendingCents; tips pay at
 *  once) and C13 (a sponsor read pays only with someone watching). */
void MoneyRules()
{
	const double W0 = 1440.0 * 24.0 + 20.0 * 60.0;
	for (const int Rents : {1, 2})
	{
		kast::Channel Ch = RoomCommunity(80, 600, 60, W0);
		Ch.Affiliate = true;
		for (kast::Member& M : Ch.Members)
		{
			M.Affinity = 1.0;
			M.Loyalty = 0.9;
		}
		kast::Stream St("kast-money-" + std::to_string(Rents));
		kast::Inputs In = RoomInputs(W0, 6300);
		In.RentsPaid = Rents;
		// Nights until both kinds have come in (a tip is a rare thing a visit); nothing is paid out in between.
		for (int Night = 0; Night < 12 && (Ch.EarnedTips == 0 || Ch.EarnedBits + Ch.EarnedSubs == 0); ++Night)
		{
			RoomNext(In, W0 + static_cast<double>(Night) * RoomDay);
			St.Start(Ch, In);
			RoomRun(St, Ch, In, 240.0);
			St.Stop(Ch, In);
		}
		const ss::Chips Affiliate = Ch.EarnedSubs + Ch.EarnedBits + Ch.EarnedAds;
		std::printf("  money rules (rents paid %d): tips $%.2f, Affiliate revenue $%.2f; unpaid $%.2f, pending $%.2f\n", Rents, static_cast<double>(Ch.EarnedTips) / 100.0,
			static_cast<double>(Affiliate) / 100.0, static_cast<double>(Ch.UnpaidCents) / 100.0, static_cast<double>(Ch.PendingCents) / 100.0);
		if (Rents < 2)
		{
			Expect(Affiliate > 0 && Ch.EarnedTips > 0 && Ch.PendingCents == Affiliate && Ch.UnpaidCents == Ch.EarnedTips,
				"money rules: before the second rent, live Affiliate revenue waits in PendingCents and tips pay at once (C14)");
		}
		else
		{
			Expect(Ch.PendingCents == 0 && Ch.UnpaidCents == Affiliate + Ch.EarnedTips, "money rules: after the second rent, everything earned live is payable");
		}
	}
	// C13: nobody watching, no read (an empty channel in the small hours: the room is often at 0 at minute 35).
	int Empty = 0, EmptyPaid = 0, Watched = 0, WatchedPaid = 0;
	for (int Seed = 0; Seed < 24 && (Empty == 0 || Watched == 0); ++Seed)
	{
		const double W1 = 1440.0 * 26.0 + 4.0 * 60.0;
		kast::Channel Ch = RoomCommunity(0, 0, 30, W1);
		kast::Stream St("kast-read-" + std::to_string(Seed));
		kast::Inputs In = RoomInputs(W1, 6400 + static_cast<uint64_t>(Seed));
		kast::Deal D;
		D.Id = "riverline";
		D.Since = W1 - 10.0 * RoomDay;
		D.Until = W1 + 40.0 * RoomDay;
		D.ReadCents = 1000;
		D.ReadsAsk = 4;
		In.Deals = {D};
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 35.0);
		const bool Nobody = kast::RoomState::Of(St)->Watchers == 0;
		const ss::Chips Paid = St.SponsorRead(Ch, In, "riverline");
		(Nobody ? Empty : Watched) += 1;
		(Nobody ? EmptyPaid : WatchedPaid) += Paid > 0 ? 1 : 0;
		St.Stop(Ch, In);
	}
	std::printf("  money rules: reads at 35 minutes: %d empty rooms (%d paid), %d with someone watching (%d paid)\n", Empty, EmptyPaid, Watched, WatchedPaid);
	Expect(Empty > 0 && EmptyPaid == 0, "money rules: a sponsor read pays nothing with nobody watching (C13)");
	Expect(Watched > 0 && WatchedPaid == Watched, "money rules: a read with someone watching after 30 minutes pays");
}

/** The defects this review fixed, each pinned: the restart's View carries no ping; a welcomed raider's name stays theirs while
 *  they stay; Rerun's +12 a world week spans go-lives; a stream lost to a reload closes with min(GainSoFar, 0) and books nothing
 *  else (§8.4). */
void ReviewFixes()
{
	const double W0 = RoomDay * 56.0 + 20.0 * 60.0; // day 56: the first day of a world week (KastWeekOf: day / 7)
	// The ping: the go-live's View carries it; a restart inside 4 world hours pinged nobody, so its View says so.
	{
		kast::Channel Ch = RoomCommunity(60, 400, 30, W0);
		kast::Stream St("kast-fix-ping");
		kast::Inputs In = RoomInputs(W0, 6501);
		St.Start(Ch, In);
		const int Reached = St.GetView().PingReached, Of = St.GetView().PingOf;
		RoomRun(St, Ch, In, 70.0);
		St.Stop(Ch, In);
		RoomNext(In, In.World + 15.0);
		St.Start(Ch, In);
		const kast::View Again = St.GetView();
		RoomRun(St, Ch, In, 5.0);
		St.Stop(Ch, In);
		Expect(Reached > 0 && Of > 0 && Again.PingReached == 0 && Again.PingOf == 0, "review: the restart's View carries no ping (no \"Your ping reached\" toast for a ping that never went out)");
	}
	// A welcomed raider's name stays busy as long as they stay.
	{
		kast::Channel Ch = RoomCommunity(20, 80, 30, W0);
		kast::Stream St("kast-fix-raid");
		kast::Inputs In = RoomInputs(W0, 6502);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 10.0);
		St.DebugRoom(Ch, In, "raid 24");
		const bool Welcomed = St.WelcomeRaid(Ch);
		const kast::RoomState* R = kast::RoomState::Of(St);
		int Named = 0, Free = 0;
		for (const kast::RoomState::Raider& X : R->Raiders)
		{
			if (X.Pool >= 0)
			{
				++Named;
				Free += R->PoolUntil[static_cast<size_t>(X.Pool)] < X.Leave ? 1 : 0;
			}
		}
		St.Stop(Ch, In);
		Expect(Welcomed && Named > 0 && Free == 0, "review: a welcomed raider's name stays theirs while they stay (no guest walks in wearing it)");
	}
	// Rerun's Heat: at most +12 a world week across go-lives (two breakouts in a week give 10, then 2).
	{
		kast::Channel Ch = RoomCommunity(20, 80, 30, W0);
		kast::Stream St("kast-fix-rerun");
		kast::Inputs In = RoomInputs(W0, 6503);
		In.RerunHeat = {{W0 - 600.0, 10.0}};
		const double Before1 = growth::HeatShown(Ch.Heat, Ch.HeatAt, W0, Ch.LastStreamDay);
		St.Start(Ch, In);
		const double Gave1 = kast::RoomState::Of(St)->HeatStart - Before1;
		RoomRun(St, Ch, In, 70.0);
		St.Stop(Ch, In);
		const double W1 = W0 + RoomDay;
		kast::Channel Ctrl = Ch;
		In.RerunHeat = {{W1 - 600.0, 10.0}};
		RoomNext(In, W1);
		St.Start(Ch, In);
		const double With = kast::RoomState::Of(St)->HeatStart;
		St.Stop(Ch, In);
		kast::Stream St2("kast-fix-rerun-ctrl");
		kast::Inputs In2 = In;
		In2.RerunHeat.clear();
		St2.Start(Ctrl, In2);
		const double Without = kast::RoomState::Of(St2)->HeatStart;
		St2.Stop(Ctrl, In2);
		std::printf("  review: Rerun's Heat: the week's first breakout gave %.2f, the second %.2f (cap 12)\n", Gave1, With - Without);
		Expect(Gave1 > 9.0 && With - Without <= 2.0 + 1.0e-9 && With - Without > 1.0, "review: Rerun's Heat gives at most +12 a world week, across go-lives (§2.2, §3.3)");
	}
	// A stream lost to a reload (the save carries LiveSince and GainSoFar): the next go-live closes it with min(GainSoFar, 0), and
	// it books no show-up, no log row and no live minutes.
	{
		kast::Channel Ch = RoomCommunity(40, 160, 30, W0);
		Ch.Bias = 1.5; // forecasts here usually land far high: tonight reads as a bad night, so the ghost gain is negative
		kast::Stream St("kast-fix-reload");
		kast::Inputs In = RoomInputs(W0, 6504);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 90.0);
		kast::Channel Saved = Ch; // the save made while live (the stream itself is never saved)
		kast::ApplySaveView(Saved, kast::SaveViewOf(Ch));
		const double W1 = W0 + RoomDay;
		const double Shown = growth::HeatShown(Saved.Heat, Saved.HeatAt, W1, Saved.LastStreamDay);
		const double Want = growth::HeatShown(std::min(100.0, std::max(8.0, Shown + std::min(0.0, Saved.GainSoFar))), W1, W1, Saved.LastStreamDay);
		const size_t Logged = Saved.Log.size();
		const std::array<int, 7> Ring = Saved.LiveMin;
		const int ShowDays = Saved.ShowDays;
		kast::Stream Reloaded("kast-fix-reload-2");
		kast::Inputs In2 = In;
		In2.World = W1;
		In2.Real += 3600.0;
		Reloaded.Start(Saved, In2);
		const double Got = kast::RoomState::Of(Reloaded)->HeatStart;
		const bool Booked = Saved.Log.size() != Logged || Saved.LiveMin != Ring || Saved.ShowDays != ShowDays;
		Reloaded.Stop(Saved, In2);
		std::printf("  review: a stream lost to a reload: ghost gain %.2f, the next go-live's Heat %.2f (want %.2f)\n", Ch.GainSoFar, Got, Want);
		Expect(Ch.LiveSince >= 0.0 && Ch.GainSoFar < 0.0 && std::fabs(Got - Want) < 1.0e-9 && !Booked,
			"review: a stream lost to a reload closes at the next go-live with min(GainSoFar, 0) and books nothing else (§8.4)");
	}
	// Members pushed into the channel while it's live (ss.Kast.Grant on a live stream) join the room before anything walks the
	// community: the per-member arrays grow with them, the slot's lurker walk and a final table's follower alert stay inside them,
	// and the View flags every member (review round 2, 2026-10-25).
	{
		kast::Channel Ch = RoomCommunity(60, 300, 40, W0);
		kast::Stream St("kast-behind-the-back");
		kast::Inputs In = RoomInputs(W0, 6600);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 15.0);
		for (int K = 0; K < 400; ++K)
		{
			kast::Member M;
			M.Name = "granted_" + std::to_string(K);
			M.Loyalty = 0.2 + 0.6 * static_cast<double>(K % 4) / 3.0;
			M.Affinity = 0.8;
			M.Follower = true;
			M.FirstSeen = In.World - 30.0 * RoomDay;
			M.LastSeen = In.World - RoomDay;
			Ch.Members.push_back(M);
			++Ch.Followers;
		}
		RoomRun(St, Ch, In, 20.0); // across a slot: the lurker walk
		const kast::RoomState* R = kast::RoomState::Of(St);
		const size_t N = Ch.Members.size();
		const bool Sized = R->VisitOf.size() == N && R->MemberKey.size() == N && R->MemberPa.size() == N && R->MemberCanon.size() == N && R->MemberAt.size() == N;
		St.DebugRoom(Ch, In, "moment ft"); // the follower alert walks every member
		RoomRun(St, Ch, In, 30.0);
		In.World += 0.25;
		In.Real += 0.25 * 13.1;
		St.Tick(Ch, In, 0.25, 0.25 * 13.1);
		int GrantedHere = 0;
		for (const kast::RoomState::Visit& V : R->Visits)
		{
			GrantedHere += V.Member >= 60 && kast::RoomState::HereAt(V, R->Clock) ? 1 : 0;
		}
		const bool Still = R->VisitOf.size() == Ch.Members.size() && St.GetView().MemberFlags.size() == Ch.Members.size();
		St.Stop(Ch, In);
		std::printf("  review: 400 members added while live: the room's arrays %s; %d of them here after a final table's alert\n", Sized && Still ? "grew with them" : "DID NOT GROW",
			GrantedHere);
		Expect(Sized && Still, "review: members added to a live channel behind the room's back join its per-member arrays before any walk of the community");
	}
}

// ------------------------------------------------------------------ the Room's frame budget (§9.4 #13)

/** The giving terms' shortcuts (Kast.cpp's GivingAt reads them once a minute or once a visit, not once a member-minute): growth::Dil
 *  is its room factor times its member factor, and TipChance and CheerChance are their Dilution-1 values times the dilution, each to
 *  the bit. If KastGrowth.cpp's formulas change shape, this fails before the room's numbers quietly move. */
void GiveTerms()
{
	int Checked = 0, Off = 0;
	std::string First;
	ss::Rng R("kast-give-terms");
	for (int K = 0; K < 4000; ++K)
	{
		const double L = K % 7 == 0 ? static_cast<double>(K % 21) / 20.0 : R.Next();
		const double H = K % 5 == 0 ? 14.0 : 14.0 + 180.0 * R.Next();
		const double E = K % 3 == 0 ? 0.3 : 1.1 * R.Next();
		const double A = K % 11 == 0 ? 0.0 : 1500.0 * R.Next() * R.Next();
		const double D = K % 4 == 0 ? 1.0e6 : static_cast<double>(R.Int(400));
		const double Dil = growth::Dil(A, D);
		const bool Same = Dil == growth::Dil(A, 1.0e6) * growth::Dil(0.0, D) && growth::TipChance(L, H, E, Dil) == growth::TipChance(L, H, E, 1.0) * Dil &&
			growth::CheerChance(L, H, E, Dil) == growth::CheerChance(L, H, E, 1.0) * Dil;
		++Checked;
		if (!Same)
		{
			++Off;
			if (First.empty())
			{
				char Buf[160];
				std::snprintf(Buf, sizeof(Buf), " (first at L %.17g, hype %.17g, E %.17g, present %.17g, days %.17g)", L, H, E, A, D);
				First = Buf;
			}
		}
	}
	Expect(Off == 0, "give terms: Dil = its room factor x its member factor; TipChance and CheerChance = their Dilution-1 value x the dilution, to the bit, on " +
		std::to_string(Checked) + " draws" + First);
}

/** GivingAt's caches change nothing (review round 2, 2026-10-25): the same night with every visit's cached giving terms and Dil
 *  factor thrown away before each one-minute frame (a frame runs exactly one grid minute, so nothing is reused) leaves every
 *  Channel byte, every chat line and the end card's money as the cached night does. Two nights: heads-down, so Engage sits at its
 *  floor and the terms move with the room's hype (the deep run's stages); and a generous room (every member loyal, Engage held
 *  high), where a member's own gift moves Dil's days since they gave inside a visit. */
void GiveCache()
{
	const double W0 = RoomDay * 40.0 + 21.0 * 60.0;
	struct CacheNight
	{
		std::string Digest, Lines;
		ss::Chips Tips = 0, Bits = 0, Subs = 0;
		int Gifts = 0;
	};
	auto Night = [&](bool Fresh, bool Generous) {
		kast::Channel Ch = RoomCommunity(400, 1200, 80, W0);
		for (kast::Member& Mb : Ch.Members)
		{
			Mb.Loyalty = Generous ? 0.9 : Mb.Loyalty;
			Mb.Affinity = Generous ? 1.0 : Mb.Affinity;
		}
		Ch.Affiliate = true;
		Ch.ScheduleDays = 0x7f;
		Ch.ScheduleStart = 21 * 60;
		kast::Stream St("kast-give-cache");
		kast::Inputs In = RoomInputs(W0, 5150);
		In.Attended = Generous;
		In.Sprinting = !Generous;
		RoomDeepRun(In, 0.0, 240.0);
		St.Start(Ch, In);
		kast::RoomState* R = kast::RoomState::Of(St);
		const std::vector<RoomMoment> Moments = RoomScript(240.0, true, 1, 21);
		size_t Mi = 0;
		int LastId = 0;
		CacheNight Out;
		for (int M = 1; M <= 240; ++M)
		{
			if (Fresh)
			{
				for (kast::RoomState::Visit& V : R->Visits)
				{
					V.GiveL = V.GiveHype = V.GiveE = -1.0;
					V.DilDays = -1.0;
				}
			}
			In.World = W0 + static_cast<double>(M);
			In.Real += 13.1;
			RoomDeepRun(In, static_cast<double>(M), 240.0);
			if (Generous)
			{
				St.Engage = 0.9;
			}
			St.Tick(Ch, In, 1.0, 1.0);
			while (Mi < Moments.size() && Moments[Mi].At <= static_cast<double>(M))
			{
				St.OnMoment(Ch, In, Moments[Mi].Kind, "somebody", 1.0);
				++Mi;
			}
			for (const kast::ChatMsg& L : St.Chat)
			{
				if (L.Id > LastId)
				{
					LastId = L.Id;
					Out.Lines += L.Who + "|" + L.Text + "|" + std::to_string(static_cast<int>(L.Kind)) + "\n";
				}
			}
			St.Notices.clear();
			St.Deals.clear();
		}
		R->GiveSyncAll(St, Ch); // lazy giving carries a visit's sums only when they can cross: made exact to read them (C-K's M2)
		for (const kast::RoomState::Visit& V : R->Visits)
		{
			Out.Gifts += (V.Gave[0] ? 1 : 0) + (V.Gave[1] ? 1 : 0) + (V.Gave[2] ? 1 : 0);
			// Each visit's integrated hazards, to the bit: a cache that drifts shows here even when no gift crosses its threshold.
			char Buf[96];
			std::snprintf(Buf, sizeof(Buf), "%.17g %.17g %.17g|", V.GiveH[0], V.GiveH[1], V.GiveH[2]);
			Out.Digest += Buf;
		}
		const kast::Summary S = St.Stop(Ch, In);
		Out.Tips = S.TipCents;
		Out.Bits = S.BitCents;
		Out.Subs = S.SubCents;
		Out.Digest += RoomDigest(Ch);
		return Out;
	};
	for (const bool Generous : {false, true})
	{
		const CacheNight Cached = Night(false, Generous), Fresh = Night(true, Generous);
		const char* Kind = Generous ? "a generous room" : "heads-down, a deep run";
		std::printf("  give cache (%s): %d gifts from members' visits (tips $%.2f, cheers $%.2f, subs $%.2f); with the caches thrown away each minute: %s\n", Kind, Cached.Gifts,
			static_cast<double>(Cached.Tips) / 100.0, static_cast<double>(Cached.Bits) / 100.0, static_cast<double>(Cached.Subs) / 100.0,
			Cached.Digest == Fresh.Digest && Cached.Lines == Fresh.Lines ? "identical" : "DIFFERS");
		Expect(Cached.Gifts >= (Generous ? 60 : 10), std::string("give cache: the night gives (members tip, cheer and subscribe; ") + Kind + ")");
		Expect(Cached.Digest == Fresh.Digest && Cached.Lines == Fresh.Lines && Cached.Tips == Fresh.Tips && Cached.Bits == Fresh.Bits && Cached.Subs == Fresh.Subs,
			std::string("give cache: GivingAt's cached terms and Dil factor leave every Channel byte, chat line and the end card's money as fresh ones do (") + Kind + ")");
	}
}

/** GivingAt skips a visit with nothing left to give (the frame budget); whatever a visit may still give, it gives (review round
 *  2, 2026-10-25): after a tip and a cheer, a new sub; after a tip and a sub, a cheer; after a cheer and a sub, a tip; off
 *  Affiliate, a tip. Each on a visit 31+ world minutes in that stays through the next minute, its threshold made tiny: it must fire
 *  on that minute. */
void GiveSkip()
{
	const double W0 = RoomDay * 42.0 + 21.0 * 60.0;
	struct Case
	{
		const char* Name;
		bool Aff;
		std::array<bool, 3> Gave;
		int Fires;
	};
	const Case Cases[] = {
		{"a new sub after a tip and a cheer", true, {{true, true, false}}, 2},
		{"a cheer after a tip and a sub", true, {{true, false, true}}, 1},
		{"a tip after a cheer and a sub", true, {{false, true, true}}, 0},
		{"a tip off Affiliate", false, {{false, true, true}}, 0},
	};
	for (const Case& C : Cases)
	{
		kast::Channel Ch = RoomCommunity(60, 200, 40, W0);
		Ch.Affiliate = C.Aff;
		kast::Stream St("kast-give-skip");
		kast::Inputs In = RoomInputs(W0, 5200);
		St.Start(Ch, In);
		kast::RoomState* R = kast::RoomState::Of(St);
		for (int M = 1; M <= 41; ++M)
		{
			In.World = W0 + static_cast<double>(M);
			In.Real += 13.1;
			St.AdvanceTo(Ch, In);
		}
		int Vi = -1;
		for (size_t I = 0; I < R->Visits.size() && Vi < 0; ++I)
		{
			const kast::RoomState::Visit& V = R->Visits[I];
			const kast::Member& Mb = Ch.Members[static_cast<size_t>(V.Member)];
			if (kast::RoomState::HereAt(V, 41.0) && kast::RoomState::HereAt(V, 42.0) && kast::RoomState::WatchedTo(V, 41.0) >= 31.0 && !V.Sub && !V.Gave[0] && !V.Gave[1] &&
				!V.Gave[2] && Mb.GaveDay < 0 && Mb.Loyalty > 0.3 && !Ch.IsSub(Mb.Name, In.World))
			{
				Vi = static_cast<int>(I);
			}
		}
		bool Fired = false;
		if (Vi >= 0)
		{
			R->GiveSync(St, Ch, Vi); // exact to now before the test changes it, and bounded again after (lazy giving, C-K's M2)
			kast::RoomState::Visit& V = R->Visits[static_cast<size_t>(Vi)];
			V.Gave = C.Gave;
			V.Need[static_cast<size_t>(C.Fires)] = 1.0e-12;
			R->GiveAnchor(Ch, Vi);
			In.World = W0 + 42.0;
			In.Real += 13.1;
			St.AdvanceTo(Ch, In);
			Fired = R->Visits[static_cast<size_t>(Vi)].Gave[static_cast<size_t>(C.Fires)];
		}
		St.Stop(Ch, In);
		Expect(Vi >= 0 && Fired, std::string("give skip: ") + C.Name + " still comes (a visit is skipped only when it has nothing left to give)");
	}
}

// ------------------------------------------------------------------ the C-K fixes (fix-room, 2026-10-25): the event-driven Room (M2),
// the firsts posted live (M3), the Room's lines (L5), the forecast's lurkers (L8), the View's new facts (L11)

/** The Room's fast keyed draws and rank (C-K's M2): RoomState::U is growth::Unit to the bit for every tag (its first three mixes
 *  done once a stream), and RoomRank is growth::RankAt (each channel's night hash kept for the world day) at every world time and
 *  viewer count tried, across midnights; ChannelsLive is RankAt's field without you. */
void FastDraws()
{
	kast::RoomState R;
	int Off = 0, Checked = 0;
	ss::Rng Rg("kast-fast-draws");
	for (int K = 0; K < 400; ++K)
	{
		R.SeedK = (static_cast<uint64_t>(Rg.NextUint32()) << 32) ^ Rg.NextUint32();
		R.StreamK = static_cast<uint64_t>(Rg.Int(600));
		R.KeyTags();
		for (int T = 0; T < static_cast<int>(growth::Tag::Count); ++T)
		{
			const uint64_t A = (static_cast<uint64_t>(Rg.NextUint32()) << 32) ^ Rg.NextUint32();
			const uint64_t B = K % 3 == 0 ? 0 : static_cast<uint64_t>(Rg.NextUint32());
			++Checked;
			Off += R.U(static_cast<growth::Tag>(T), A, B) != growth::Unit(R.SeedK, R.StreamK, static_cast<growth::Tag>(T), A, B) ? 1 : 0;
		}
	}
	int RankOff = 0, Ranked = 0, LiveOff = 0;
	kast::RoomState Rr;
	for (double W = RoomDay * 3.0 + 0.37; W < RoomDay * 12.0; W += 7.3)
	{
		for (const int V : {0, 1, 3, 12, 40, 150, 900, 3000})
		{
			const growth::Standing A = Rr.RoomRank(W, V), B = growth::RankAt(W, V);
			++Ranked;
			RankOff += A.Rank != B.Rank || A.Of != B.Of ? 1 : 0;
		}
		LiveOff += kast::ChannelsLive(W) != growth::RankAt(W, 0).Of - 1 ? 1 : 0;
	}
	// A room whose day cache was built later than the time it's asked about (a test's clock going back) reads RankAt's answer too.
	RankOff += Rr.RoomRank(RoomDay * 3.5, 7).Rank != growth::RankAt(RoomDay * 3.5, 7).Rank ? 1 : 0;
	std::printf("  fast draws: %d keyed draws against growth::Unit, %d off; %d ranks against growth::RankAt, %d off; channels live off %d\n", Checked, Off, Ranked, RankOff, LiveOff);
	Expect(Off == 0, "fast draws: RoomState::U is growth::Unit to the bit, every tag");
	Expect(RankOff == 0 && LiveOff == 0, "fast draws: RoomRank is growth::RankAt at every world time and count tried; ChannelsLive is its field without you");
}

/** The event-driven Room changes no result (C-K's M2): each night run with RoomState::EventDriven on and off (lazy giving, presence
 *  by transition, against the eager walks) leaves every Channel byte, every chat line, the end card and every frame's counts,
 *  sources, chatting and member flags identical; and no replay ever found a gift before the minute that asked for it. The nights:
 *  a heads-down deep run on two tables (stage hype moving every minute), a generous attended room with the streamer's verbs (Nods,
 *  answers, timeouts and a promotion: loyalty writes), a raid and a big name, lurkers and mods, a first stream, a 1,500-member
 *  channel skipped in long frames, and lapsed followers crossing the Ping's 7-, 21- and 60-day steps tonight with an hour on the
 *  results screen (the slot chance's and the lurker sum's caches against a walk every slot; review-fix-room). */
void EventDriven()
{
	const double W0 = RoomDay * 44.0 + 21.0 * 60.0;
	struct Night
	{
		const char* Name;
		int Members, Followers, Streams;
		bool Deep, Affiliate, Verbs;
		RoomPace Pace;
		double Minutes;
		double Loyal; // every member's loyalty (0: the community's own)
		int Mods;
		bool Lapsed = false; // followers last seen just inside each PingW step (they cross it tonight), the results screen 100-160
	};
	const RoomPace Sprint{"sprint", 0.0, 0.7, false, 1, true};
	const RoomPace Smart{"smart", 0.5, 13.1, true, 1, false};
	const RoomPace Skip{"skip", 25.0, 0.7, false, 1, true};
	const Night Nights[] = {
		{"heads-down deep run", 400, 1200, 80, true, true, false, Sprint, 300.0, 0.0, 0},
		{"generous room with verbs", 120, 400, 60, false, true, true, Smart, 150.0, 0.9, 2},
		{"lurkers and mods", 0, 400, 30, false, false, false, Smart, 120.0, 0.0, 2},
		{"first stream", -1, 0, 0, false, false, false, Smart, 75.0, 0.0, 0},
		{"1,500 members skipped", 1500, 4000, 300, true, true, false, Skip, 260.0, 0.0, 3},
		{"lapsed followers, the results screen", 600, 1500, 50, false, true, false, Sprint, 300.0, 0.0, 0, true},
	};
	const uint64_t LateBefore = kast::RoomState::GiveLate.load();
	bool AllSame = true;
	std::string Differs;
	for (const Night& N : Nights)
	{
		auto Run = [&](bool Event) {
			kast::RoomState::EventDriven = Event;
			kast::Channel Ch = N.Members >= 0 ? RoomCommunity(N.Members, N.Followers, N.Streams, W0) : kast::Channel();
			for (kast::Member& Mb : Ch.Members)
			{
				Mb.Loyalty = N.Loyal > 0.0 ? N.Loyal : Mb.Loyalty;
			}
			if (N.Lapsed)
			{
				// Three in four last seen just inside the 7-, 21- and 60-day steps (each crosses it 72, 144 or 216 world minutes in; the
				// loyal few come, the rest are the lurker sum's), the fourth spread over 70 days.
				static const double Days[] = {6.95, 20.9, 59.85};
				for (size_t K = 0; K < Ch.Members.size(); ++K)
				{
					kast::Member& Mb = Ch.Members[K];
					Mb.LastSeen = W0 - RoomDay * (K % 4 < 3 ? Days[K % 4] : 0.5 + std::fmod(static_cast<double>(K) * 0.73, 69.5));
					Mb.FirstSeen = std::min(Mb.FirstSeen, Mb.LastSeen);
				}
			}
			Ch.Affiliate = N.Affiliate;
			for (int K = 0; K < N.Mods; ++K)
			{
				kast::Moderator Mod;
				Mod.Name = N.Members > 0 ? "kastfan_" + std::to_string(K * 11 + 3) : "kastmod_" + std::to_string(K);
				Mod.Online = 0.95;
				Ch.Mods.push_back(Mod);
			}
			kast::Stream St("kast-event-driven");
			kast::Inputs In = RoomInputs(W0, 8100);
			In.Attended = N.Pace.Attended;
			In.Sprinting = N.Pace.Sprinting;
			In.Tables = N.Deep ? 2 : 1;
			if (N.Deep)
			{
				RoomDeepRun(In, 0.0, N.Minutes);
			}
			const std::vector<RoomMoment> Moments = RoomScript(N.Minutes, N.Deep, 2, 31);
			St.Start(Ch, In);
			const kast::RoomState* R = kast::RoomState::Of(St);
			std::string Out;
			int LastId = 0, Step = 0;
			size_t Mi = 0;
			double T = 0.0;
			bool Promoted = false, Raided = false, OutMember = false, OutGuest = false;
			int Nods = 0;
			auto Facts = [&]() {
				const kast::View& V = St.GetView();
				char Buf[160];
				std::snprintf(Buf, sizeof(Buf), "%d %d %d %d %d %d|", V.Count, V.ShownCount, V.Chatting, V.WelcomeRaidSecs, V.Impressions, V.ImpressionsCame);
				Out += Buf;
				for (const int Sv : V.Sources)
				{
					Out += std::to_string(Sv) + ",";
				}
				Out.append(reinterpret_cast<const char*>(V.MemberFlags.data()), V.MemberFlags.size());
				Out += V.Why[0] + "|" + V.Why[1] + "\n";
				for (const kast::ChatMsg& M : St.Chat)
				{
					if (M.Id > LastId)
					{
						LastId = M.Id;
						Out += M.Who + ":" + M.Text + "\n";
					}
				}
			};
			while (T < N.Minutes - 1.0e-12)
			{
				double Next = std::min(N.Minutes, T + (N.Pace.FrameWorld > 0.0 && T >= 30.0 ? N.Pace.FrameWorld : N.Pace.FrameWorld > 0.0 ? std::min(1.0, N.Pace.FrameWorld) : RoomFrame(N.Pace, Step++)));
				if (Mi < Moments.size())
				{
					Next = std::min(Next, std::max(T, Moments[Mi].At));
				}
				In.World = W0 + Next;
				In.Real += (Next - T) * N.Pace.RealPerWorld;
				if (N.Deep)
				{
					RoomDeepRun(In, Next, N.Minutes);
				}
				if (N.Lapsed)
				{
					In.Results = Next >= 100.0 && Next < 160.0; // the results screen for an hour: the gap means move
				}
				St.Tick(Ch, In, Next - T, std::min(1.0, (Next - T) * N.Pace.RealPerWorld));
				Facts();
				T = Next;
				while (Mi < Moments.size() && Moments[Mi].At <= T + 1.0e-12)
				{
					In.ThisTable = Moments[Mi].Table;
					St.OnMoment(Ch, In, Moments[Mi].Kind, "somebody", 1.0);
					In.ThisTable = In.FrontTable;
					++Mi;
				}
				if (N.Verbs)
				{
					// The streamer's verbs, at fixed minutes: Nods, an answer, a member and a guest timed out, a member promoted, a raid.
					if (Nods < 3 && T >= 10.0 * (Nods + 1))
					{
						for (const std::string& Name : R->PresentNames())
						{
							if (St.Nod(Ch, Name))
							{
								++Nods;
								break;
							}
						}
					}
					for (const kast::ChatMsg& M : St.Chat)
					{
						if (M.Kind == kast::LineKind::Question && !M.Answered && !M.Deleted && M.At <= In.Real && St.Answer(Ch, In, M.Id))
						{
							break;
						}
					}
					if (T >= 40.0 && (!OutMember || !OutGuest))
					{
						for (const kast::ChatMsg& M : St.Chat)
						{
							const bool IsMember = Ch.FindMember(M.Who) != nullptr;
							if (M.Kind == kast::LineKind::Chat && !M.Deleted && M.At <= In.Real && R->Present(M.Who) && !Ch.IsMod(M.Who) && ((IsMember && !OutMember) || (!IsMember && !OutGuest)) &&
								St.Timeout(Ch, In, M.Id))
							{
								(IsMember ? OutMember : OutGuest) = true;
								break;
							}
						}
					}
					if (T >= 50.0 && !Promoted)
					{
						for (const std::string& Name : R->PresentNames())
						{
							if (Ch.FindMember(Name) && !Ch.IsMod(Name) && St.Promote(Ch, In, Name))
							{
								Promoted = true;
								break;
							}
						}
					}
					if (T >= 60.0 && !Raided)
					{
						Raided = true;
						St.DebugRoom(Ch, In, "raid 14");
						kast::RoomState::AddRaid(St, Ch, In, "VikingVolta", 0xf97316, 40, false, kast::RoomState::Of(St)->Clock, 3);
					}
				}
				St.Notices.clear();
				St.Deals.clear();
			}
			const kast::Summary S = St.Stop(Ch, In);
			char Buf[200];
			std::snprintf(Buf, sizeof(Buf), "end %d %d %d %d %lld %lld %lld %lld %d|", S.Avg, S.Peak, S.Follows, S.Subs, static_cast<long long>(S.TipCents), static_cast<long long>(S.BitCents),
				static_cast<long long>(S.SubCents), static_cast<long long>(S.AdCents), S.Chatted);
			Out += Buf;
			for (const std::string& F : S.Firsts)
			{
				Out += F + "|";
			}
			Out += S.Why[0] + "|" + S.Why[1] + "\n" + RoomDigest(Ch);
			kast::RoomState::EventDriven = true;
			return Out;
		};
		const std::string Eager = Run(false), Event = Run(true);
		const bool Same = Eager == Event;
		AllSame = AllSame && Same;
		if (!Same && Differs.empty())
		{
			size_t At = 0;
			while (At < Eager.size() && At < Event.size() && Eager[At] == Event[At])
			{
				++At;
			}
			Differs = std::string(N.Name) + " at byte " + std::to_string(At) + " of " + std::to_string(Eager.size());
		}
		std::printf("  event-driven room (%s): %zu bytes of facts, eager and event-driven %s\n", N.Name, Eager.size(), Same ? "identical" : "DIFFER");
	}
	const uint64_t Late = kast::RoomState::GiveLate.load() - LateBefore;
	Expect(AllSame, "event-driven room: lazy giving and presence by transition leave every Channel byte, chat line, end card and frame's counts and flags as the eager walks do" +
		(Differs.empty() ? std::string() : " (" + Differs + ")"));
	Expect(Late == 0, "event-driven room: no replay found a gift before the minute that asked for it (" + std::to_string(Late) + ")");
}

/** The lurker sum's cache (C-K's M2; review-fix-room): at every slot where no visit, member or loyalty has moved since the Room last
 *  walked the followers not coming tonight, the sum it keeps is that walk's at the slot (PingW by days since last seen, in member
 *  order), on a quiet night whose lapsed followers cross the Ping's 7-, 21- and 60-day steps while live (the A/B's busy nights move
 *  a visit almost every slot, which walks it again anyway). */
void LurkCache()
{
	const double W0 = RoomDay * 45.0 + 21.0 * 60.0;
	kast::Channel Ch = RoomCommunity(600, 600, 50, W0);
	static const double Days[] = {6.95, 20.9, 59.85};
	for (size_t K = 0; K < Ch.Members.size(); ++K)
	{
		kast::Member& Mb = Ch.Members[K];
		Mb.Loyalty = 0.05 + 0.1 * static_cast<double>(K % 5) / 4.0; // nobody a superfan: every term is PingW's step
		Mb.LastSeen = W0 - RoomDay * (K % 4 < 3 ? Days[K % 4] : 0.5 + std::fmod(static_cast<double>(K) * 0.73, 69.5));
		Mb.FirstSeen = std::min(Mb.FirstSeen, Mb.LastSeen);
	}
	kast::Stream St("kast-lurk-cache");
	kast::Inputs In = RoomInputs(W0, 8150);
	In.Attended = false;
	In.Sprinting = true;
	St.Start(Ch, In);
	const kast::RoomState* R = kast::RoomState::Of(St);
	int Compared = 0, Off = 0, Stepped = 0;
	for (int M = 1; M <= 300; ++M)
	{
		In.World = W0 + M;
		In.Real += 0.7;
		St.Tick(Ch, In, 1.0, 0.7);
		St.Notices.clear();
		St.Deals.clear();
		const int Last = R->Done - 1; // the grid minute this frame ran last
		if (Last % 10 != 0 || R->AwayVisits != R->Visits.size() || R->AwayMembers != Ch.Members.size() || R->AwayLoyalty != R->LoyaltyWrites)
		{
			continue;
		}
		double Away = 0.0;
		for (size_t I = 0; I < Ch.Members.size(); ++I)
		{
			const kast::Member& Mb = Ch.Members[I];
			if (Mb.Follower && R->VisitOf[I] < 0)
			{
				const double D = std::max(0.0, (St.StartWorld + static_cast<double>(Last) - Mb.LastSeen) / RoomDay);
				Away += growth::PingW(D, Mb.Loyalty);
				const double D0 = std::max(0.0, (St.StartWorld + static_cast<double>(Last - 10) - Mb.LastSeen) / RoomDay);
				Stepped += growth::PingW(D, Mb.Loyalty) != growth::PingW(D0, Mb.Loyalty) ? 1 : 0;
			}
		}
		++Compared;
		Off += Away != R->AwaySum ? 1 : 0;
	}
	St.Stop(Ch, In);
	std::printf("  lurker cache: %d quiet slots compared with a fresh walk, %d off; %d steps crossed in the slot before one\n", Compared, Off, Stepped);
	Expect(Off == 0 && Compared >= 10 && Stepped > 0, "lurker cache: the kept sum is the walk's at every quiet slot, steps crossed while live included");
}

/** C-K's M3: the first stranger's tip and the first raid in are posted the moment they happen (§5.3 K17): a Milestone alert with the
 *  KastCopy line and KastShine through the cue queue, a system line in chat, the end card's Summary::Firsts; once a channel. */
void FirstsLive()
{
	const double W0 = RoomDay * 46.0 + 21.0 * 60.0;
	kast::Channel Ch = RoomCommunity(30, 300, 20, W0);
	Ch.CarryTip = 0.999;        // a stranger's tip is due as soon as a named stranger is here
	Ch.WelcomeRaidDone = true; // the first raid in is the test's own (not the welcome raid at minute 40)
	kast::Stream St("kast-firsts-live");
	kast::Inputs In = RoomInputs(W0, 8300);
	St.Start(Ch, In);
	int TipAlerts = 0, TipShine = 0, RaidAlerts = 0, RaidShine = 0, Lines = 0;
	const std::string TipHead = "First tip from someone new: ";
	// A frame's own alerts are the newest few in the feed (newest first); its notices are all the frame's (cleared before each).
	auto Look = [&](const std::string& RaidLine) {
		for (size_t I = 0; I < St.Feed.size() && I < 8; ++I)
		{
			const kast::Alert& A = St.Feed[I];
			TipAlerts += RaidLine.empty() && A.Kind == kast::AlertKind::Milestone && A.Text.rfind(TipHead, 0) == 0 ? 1 : 0;
			RaidAlerts += !RaidLine.empty() && A.Kind == kast::AlertKind::Milestone && A.Text == RaidLine ? 1 : 0;
		}
		for (const kast::Notice& N : St.Notices)
		{
			const bool Shine = N.Sound == static_cast<int>(ss::SoundId::KastShine);
			TipShine += Shine && RaidLine.empty() ? 1 : 0;
			RaidShine += Shine && !RaidLine.empty() ? 1 : 0;
		}
	};
	for (int M = 1; M <= 90 && (Ch.Firsts & growth::FirstStrangerTip) == 0; ++M)
	{
		In.World = W0 + M;
		In.Real += 13.1;
		St.Notices.clear();
		St.Tick(Ch, In, 1.0, 1.0);
		if ((Ch.Firsts & growth::FirstStrangerTip) != 0)
		{
			Look(std::string());
		}
	}
	// One more minute: nothing posts the first again (no KastShine; the channel's milestones are out of reach).
	In.World += 1.0;
	In.Real += 13.1;
	St.Notices.clear();
	St.Tick(Ch, In, 1.0, 1.0);
	for (const kast::Notice& N : St.Notices)
	{
		TipShine += N.Sound == static_cast<int>(ss::SoundId::KastShine) ? 1 : 0;
	}
	for (const kast::ChatMsg& L : St.Chat)
	{
		Lines += L.Kind == kast::LineKind::System && L.Text.rfind(TipHead, 0) == 0 ? 1 : 0;
	}
	const std::vector<kast::SmallChannel> Net = kast::Network(In.World);
	const std::string From = Net.empty() ? std::string("kat_v") : Net.front().Name;
	const std::string RaidLine = ss::kast::copy::FirstRaidIn(From, 9);
	St.Notices.clear();
	St.DebugRoom(Ch, In, "raid 9");
	Look(RaidLine);
	St.Notices.clear();
	St.DebugRoom(Ch, In, "raid 5"); // a second raid in is no first: its line never posts, nor a KastShine
	Look(ss::kast::copy::FirstRaidIn(From, 5));
	const kast::Summary S = St.Stop(Ch, In);
	const bool CardTip = std::any_of(S.Firsts.begin(), S.Firsts.end(), [&](const std::string& F) { return F.rfind(TipHead, 0) == 0; });
	const bool CardRaid = std::find(S.Firsts.begin(), S.Firsts.end(), RaidLine) != S.Firsts.end();
	std::printf("  firsts live: the first stranger's tip %d alert, %d KastShine, %d line; the first raid in %d alert, %d KastShine; the end card's firsts %zu\n", TipAlerts, TipShine, Lines,
		RaidAlerts, RaidShine, S.Firsts.size());
	Expect(TipAlerts == 1 && TipShine == 1 && Lines == 1 && CardTip, "firsts live: the first stranger's tip posts its Milestone alert with KastShine, a line and the end card's mention, once");
	Expect(RaidAlerts == 1 && RaidShine == 1 && CardRaid && S.Firsts.size() == 2, "firsts live: the first raid in posts its alert with KastShine and the end card's mention; a second raid none");
}

/** C-K's L11: the View's new facts. WELCOME THE RAID counts its 2 world minutes down in world seconds while it's offered (and reads
 *  0 once it isn't); the directory's impressions and the arrivals of the last 30 grid minutes are R6's sums; LiveChannels is the
 *  rank's field; tonight's Nods by name, a guest's included; Top clips' Clip Lane flag (a Lane clip, Clip of the Night). */
void ViewFacts()
{
	const double W0 = RoomDay * 47.0 + 21.0 * 60.0;
	kast::Channel Ch = RoomCommunity(40, 300, 30, W0);
	Ch.WelcomeRaidDone = true; // one raid, the test's (no welcome raid at minute 40)
	kast::Stream St("kast-view-facts");
	kast::Inputs In = RoomInputs(W0, 8400);
	St.Start(Ch, In);
	const kast::RoomState* R = kast::RoomState::Of(St);
	std::vector<int> Secs;
	bool Raided = false, NoddedGuest = false, Impressions = true, Live = true;
	std::string Guest;
	double RaidWorld = -1.0;
	int SecsOff = 0;
	for (double T = 0.25; T <= 40.0; T += 0.25)
	{
		In.World = W0 + T;
		In.Real += 0.25 * 13.1;
		St.Tick(Ch, In, 0.25, 0.25 * 13.1);
		const kast::View& V = St.GetView();
		double Seen = 0.0;
		int Came = 0;
		for (size_t I = 0; I < 30 && static_cast<int>(I) < R->Done; ++I)
		{
			Seen += R->SeenRing[I];
			Came += R->CameRing[I];
		}
		Impressions = Impressions && V.Impressions == static_cast<int>(std::lround(Seen)) && V.ImpressionsCame == Came && V.Impressions >= 0;
		Live = Live && V.LiveChannels == R->RankOf && V.LiveChannels == growth::RankAt(W0 + static_cast<double>(R->Done - 1), R->LastMinuteCount).Of;
		if (Raided)
		{
			Secs.push_back(V.WelcomeRaidSecs);
			// The world seconds left in the raid's 2-minute window, rounded up, and 0 once it's gone (review-fix-room: the number
			// itself, frame by frame; these frames are a quarter minute apart, so 105, 90, ... 15, then 0).
			const double Left = (RaidWorld + 2.0 - In.World) * 60.0;
			const int Want = Left > 0.0 ? static_cast<int>(std::ceil(Left - 1.0e-9)) : 0;
			SecsOff += V.WelcomeRaidSecs != Want ? 1 : 0;
		}
		if (T >= 10.0 && !Raided)
		{
			Raided = true;
			St.DebugRoom(Ch, In, "raid 12");
			RaidWorld = R->RaidAt();
		}
		if (T >= 20.0 && !NoddedGuest)
		{
			for (const std::string& Name : R->PresentNames())
			{
				if (!Ch.FindMember(Name) && St.Nod(Ch, Name))
				{
					NoddedGuest = true;
					Guest = Name;
					break;
				}
			}
		}
		St.Notices.clear();
		St.Deals.clear();
	}
	const kast::View& V = St.GetView();
	const bool GuestListed = NoddedGuest && std::find(V.Nodded.begin(), V.Nodded.end(), Guest) != V.Nodded.end();
	// The countdown: from at most 120 down, never up, never 0 while offered; 0 for good once the 2 minutes are gone.
	bool Down = !Secs.empty() && Secs.front() > 0 && Secs.front() <= 120;
	int Opened = 0;
	for (size_t I = 0; I < Secs.size(); ++I)
	{
		Opened += Secs[I] > 0 ? 1 : 0;
		Down = Down && (I == 0 || Secs[I] <= Secs[I - 1] || Secs[I - 1] == 0) && (Secs[I] == 0 || I < 12);
	}
	St.Stop(Ch, In);
	// Top clips' flag: Clip of the Night is a Lane clip.
	bool Lane = false;
	if (St.Last.ClipItOpen)
	{
		In.World += 1.0;
		Lane = St.ClipIt(Ch, In) && !Ch.Clips.empty() && Ch.Clips.front().Lane;
	}
	std::printf("  view facts: WELCOME THE RAID %d frames offered (first %d s, last %d s); impressions and arrivals %s; live channels %s; a Nodded guest listed %s; Clip of the Night in the Lane %s\n",
		Opened, Secs.empty() ? -1 : Secs.front(), Opened > 0 ? Secs[static_cast<size_t>(Opened - 1)] : -1, Impressions ? "match R6" : "OFF", Live ? "the rank's field" : "OFF", GuestListed ? "yes" : "NO",
		Lane ? "yes" : "NO");
	Expect(Down && Opened >= 6, "view facts: WELCOME THE RAID counts down from 120 world seconds while offered, never 0 then, and reads 0 after");
	Expect(SecsOff == 0 && Opened == 7, "view facts: WELCOME THE RAID's countdown is the world seconds left in its 2 minutes, every frame (" + std::to_string(SecsOff) + " off)");
	Expect(Impressions && Live, "view facts: the impressions line is R6's sums; live channels is the rank's field");
	Expect(GuestListed, "view facts: a guest Nodded tonight is in View::Nodded");
	Expect(Lane, "view facts: Clip of the Night carries the Clip Lane flag");
}

/** C-K's L5: the Room speaks KastCopy's lines where §5 puts them: a big name's raid in its own words, Heat's why lines for a record
 *  peak, a raid, Rerun's push, a milestone and a night under its forecast (each a fact, in order), the end card's why lines (the
 *  View's at Stop), and a guest from Rerun naming its video (only with the title). */
void RoomLines()
{
	const double W0 = RoomDay * 48.0 + 21.0 * 60.0;
	int VideoLines = 0, VideoLinesNoTitle = 0, Absent = 0;
	for (const bool Titled : {true, false})
	{
		kast::Channel Ch = RoomCommunity(20, 200, 30, W0);
		kast::Stream St("kast-room-lines");
		kast::Inputs In = RoomInputs(W0, 8500);
		In.VideoBuzz = 1.0e6; // Rerun's buzz at its cap: some guests come from the video shelf
		In.VideoTitle = Titled ? "kings, 82% preflop" : std::string();
		St.Start(Ch, In);
		const kast::RoomState* R = kast::RoomState::Of(St);
		const std::string Line = ss::kast::copy::CameFromVideo("kings, 82% preflop");
		RoomWatch W;
		for (int M = 1; M <= 180; ++M)
		{
			In.World = W0 + M;
			In.Real += 13.1;
			St.Tick(Ch, In, 1.0, 1.0);
			RoomObserve(St, W, In.Real);
			St.Notices.clear();
		}
		for (const kast::ChatMsg& L : St.Chat)
		{
			(Titled ? VideoLines : VideoLinesNoTitle) += L.Text == Line || L.Text.rfind("came from the", 0) == 0 ? 1 : 0;
		}
		Absent += W.Absent;
		if (Titled)
		{
			// A big name's raid alert says it in KastCopy's words.
			const size_t Feed0 = St.Feed.size();
			kast::RoomState::AddRaid(St, Ch, In, "VikingVolta", 0xf97316, 312, false, R->Clock, 3);
			const bool Big = St.Feed.size() > Feed0 && St.Feed.front().Kind == kast::AlertKind::Raid && St.Feed.front().Text == ss::kast::copy::RaidedWith("VikingVolta", 312);
			Expect(Big, "room lines: a big name's raid alert reads RaidedWith (\"VikingVolta raided with 312\")");
			// Heat's why lines follow the room's facts live (review-fix-room): the raid (a moment raid of 5+, a fifth of the room) is
			// named from the next frame on (FillView builds them again when one of their facts moves).
			In.World += 1.0;
			In.Real += 13.1;
			St.Tick(Ch, In, 1.0, 1.0);
			const std::array<std::string, 2> Why = St.GetView().Why;
			const std::string RaidWhy = ss::kast::copy::WhyRaid("VikingVolta", 312);
			std::printf("  room lines: the frame after the raid, why \"%s\" / \"%s\"\n", Why[0].c_str(), Why[1].c_str());
			Expect(Why[0] == RaidWhy || Why[1] == RaidWhy, "room lines: the raid is in Heat's why lines from the next frame (\"" + RaidWhy + "\")");
		}
		St.Stop(Ch, In);
	}
	// Heat's why lines from their facts, in order (the room's own facts set by hand, today's world day).
	{
		kast::Channel Ch = RoomCommunity(20, 200, 30, W0);
		kast::Stream St("kast-why-lines");
		kast::Inputs In = RoomInputs(W0, 8600);
		St.Start(Ch, In);
		kast::RoomState* R = kast::RoomState::Of(St);
		const int Today = static_cast<int>(std::floor(W0 / RoomDay));
		R->WhyRecordPeak = 41;
		R->WhyRecordDay = Today;
		R->WhyRaidFrom = "kat_v";
		R->WhyRaidSize = 14;
		R->WhyRaidDay = Today - 1;
		const std::array<std::string, 2> A = R->WhyNow(Ch, W0 + 5.0);
		R->WhyRecordDay = Today - 3; // too old now
		R->WhyRaidDay = -1;
		R->WhyVideoTitle = "kings, 82% preflop";
		R->WhyVideoBreakout = true;
		R->WhyVideoDay = Today;
		R->WhyMilestone = 250;
		R->WhyMilestoneDay = Today;
		const std::array<std::string, 2> B = R->WhyNow(Ch, W0 + 5.0);
		R->WhyVideoDay = -1;
		R->WhyMilestoneDay = -1;
		R->WhyOverPct = -22;
		R->WhyUnderPct = 21;
		R->WhyOverDay = Today;
		const std::array<std::string, 2> C = R->WhyNow(Ch, W0 + 5.0);
		const bool Ok = A[0] == ss::kast::copy::WhyRecordPeak(41) && A[1] == ss::kast::copy::WhyRaid("kat_v", 14) && B[0] == ss::kast::copy::WhyVideo("kings, 82% preflop", true) &&
			B[1] == ss::kast::copy::WhyMilestone(250) && C[0] == ss::kast::copy::WhyUnderForecast(21);
		std::printf("  room lines: why \"%s\" / \"%s\"; \"%s\" / \"%s\"; \"%s\"\n", A[0].c_str(), A[1].c_str(), B[0].c_str(), B[1].c_str(), C[0].c_str());
		Expect(Ok, "room lines: Heat's why lines name a record peak, a raid, Rerun's push, a milestone and a night under its forecast, each in its days");
		// "Steady: nights near your forecast" only from a fact (the C-K re-check): a room with no night against its forecast in 2
		// days (this session's facts are gone after a reload, or the channel was dark) says nothing; a night within 10% reads it.
		R->WhyOverPct = 0;
		R->WhyUnderPct = 0;
		R->WhyOverDay = -1;
		const std::array<std::string, 2> E = R->WhyNow(Ch, W0 + 5.0);
		R->WhyOverPct = 3;
		R->WhyUnderPct = -3;
		R->WhyOverDay = Today - 1;
		const std::array<std::string, 2> F = R->WhyNow(Ch, W0 + 5.0);
		R->WhyOverDay = Today - 3;
		const std::array<std::string, 2> G = R->WhyNow(Ch, W0 + 5.0);
		std::printf("  room lines: no forecast fact \"%s\" / \"%s\"; near it yesterday \"%s\"; 3 days back \"%s\"\n", E[0].c_str(), E[1].c_str(), F[0].c_str(), G[0].c_str());
		Expect(E[0].empty() && E[1].empty() && F[0] == ss::kast::copy::WhySteady() && G[0].empty(),
			"room lines: \"Steady\" only when the last night read near its forecast in 2 days, never from no facts (a reload)");
		R->WhyOverPct = -22;
		R->WhyUnderPct = 21;
		R->WhyOverDay = Today;
		++R->WhyEpoch; // the facts moved (the room bumps it with each one; FillView builds the lines again)
		In.World = W0 + 70.0;
		In.Real += 70.0 * 13.1;
		St.Tick(Ch, In, 70.0, 1.0);
		const std::array<std::string, 2> Live = St.GetView().Why;
		const kast::Summary S = St.Stop(Ch, In);
		Expect(!S.Why[0].empty() && !Live[0].empty(), "room lines: the end card carries Heat's why lines after the commit (Summary::Why)");
	}
	std::printf("  room lines: guests from Rerun named the video %d times (with the title), %d times without one; absent speakers %d\n", VideoLines, VideoLinesNoTitle, Absent);
	Expect(VideoLines > 0 && VideoLinesNoTitle == 0 && Absent == 0, "room lines: a guest from Rerun says \"came from the ... video\" only with its title, and only while here");
}

/** C-K's L8: the forecast counts the lurker pool (§2.8.6's expected count; §2.11 left it out): an old channel of 2,402 untracked
 *  followers and no schedule (pre_update2_sponsored_streamer's shape) forecasts about the room it then has, where the band read
 *  1-3 against an average of 13; a new channel's first stream is unchanged (no followers beyond Dee and Mei). */
void ForecastLurkers()
{
	const double W0 = RoomDay * 49.0 + 21.0 * 60.0;
	kast::Channel Ch;
	Ch.Followers = 2402;
	Ch.Streams = 60;
	Ch.Milestone = 1000;
	Ch.LastStreamDay = static_cast<int>(std::floor(W0 / RoomDay)) - 2;
	Ch.HeatAt = W0 - RoomDay;
	Ch.ProcessedDay = static_cast<int>(std::floor(W0 / RoomDay));
	Ch.LastOffline = W0 - 1.0;
	kast::Inputs In = RoomInputs(W0, 8700);
	const growth::Band B = kast::Stream::Forecast(Ch, In);
	kast::Stream St("kast-forecast-lurkers");
	St.Start(Ch, In);
	double Sum = 0.0;
	for (int M = 1; M <= 120; ++M)
	{
		In.World = W0 + M;
		In.Real += 13.1;
		St.Tick(Ch, In, 1.0, 1.0);
		Sum += St.Count();
		St.Notices.clear();
	}
	const double Avg = Sum / 120.0;
	St.Stop(Ch, In);
	kast::Channel Fresh;
	const growth::Band First = kast::Stream::Forecast(Fresh, RoomInputs(W0, 8701));
	std::printf("  forecast lurkers: 2,402 untracked followers: band %.1f-%.1f (centre %.1f) against an average of %.1f; a first stream's band %.1f-%.1f\n", B.Lo, B.Hi, B.Centre, Avg,
		First.Lo, First.Hi);
	Expect(std::fabs(B.Centre - Avg) <= 0.4 * Avg && B.Hi >= 0.8 * Avg, "forecast lurkers: an old channel's untracked followers are in its forecast (its centre within 40% of the night's average)");
	Expect(First.Hi < 10.0, "forecast lurkers: a first stream's forecast has no lurkers in it");
}

// ------------------------------------------------------------------ the C-K PIE fixes (pie-room: F1, F4)

/** A deepest run at the $11 Big Stack's final table (980 runners, P 1.40), its tournament's id Id ("" none). */
void PieDeep(kast::Inputs& In, const std::string& Id)
{
	In.DeepTable = 0;
	In.DeepTournament = Id;
	In.DeepEvent = "$11 Big Stack";
	In.DeepEntrants = 980;
	In.Entrants = 980;
	In.DeepRemaining = 9;
	In.Remaining = 9;
	In.DeepTableSize = 9;
	In.BuyInCents = 1100;
	In.Prestige = growth::Prestige(980, 1100, 0.0);
	In.Depth = growth::Depth(980, 9);
}

/**
 * F1 (PIE_CK_NOTES): a raid's alert and the count change together. A night at a table whose world clock moves a hand at a time
 * (the session's: a tournament's clock steps when a hand ends, so the frames between see no world time pass), at 1x, Full and
 * watch 4x, with the Broadcast at a final table (ss.Kast.Moment ft, mid-hand, as the PIE typed it), a moment raid mid-hand and the
 * welcome raid on the grid. The first call that holds a raid has its alert in the feed and every one of its party here and in the
 * count, the View draws that count, and no named line goes up from anyone not here (HonestCount's observer).
 */
void PieAlertCount()
{
	const double W0 = RoomDay * 61.0 + 21.0 * 60.0;
	struct Pace
	{
		const char* Name;
		double RealPerWorld;
		bool Attended;
		int Watch;
	};
	const Pace Paces[] = {{"1x", 13.1, true, 1}, {"full", 33.6, true, 1}, {"watch 4x", 3.37, false, 4}};
	static const double Hands[] = {1.25, 0.75, 1.5, 1.0, 2.0, 0.5, 1.75};
	for (const Pace& P : Paces)
	{
		kast::Channel Ch = RoomCommunity(6, 12, 30, W0); // a small room: the Broadcast is most of it
		kast::Stream St("kast-pie-alert");
		kast::Inputs In = RoomInputs(W0, 6101);
		In.Attended = P.Attended;
		In.WatchSpeed = P.Watch;
		St.Start(Ch, In);
		const kast::RoomState* R = kast::RoomState::Of(St);
		size_t Seen = 0;
		int Raids = 0, AlertOff = 0, PartyOff = 0, ViewOff = 0, Broadcasts = 0, MomentRaids = 0, Welcomes = 0;
		std::string First, Sizes;
		RoomWatch W;
		auto Look = [&](const char* Where) {
			RoomObserve(St, W, In.Real);
			ViewOff += St.GetView().Count == R->Watchers ? 0 : 1;
			for (; Seen < R->Raids.size(); ++Seen)
			{
				const kast::RoomState::Raid& Rd = R->Raids[Seen];
				++Raids;
				bool Posted = false;
				for (const kast::Alert& A : St.Feed)
				{
					Posted = Posted || (A.Kind == kast::AlertKind::Raid && A.Who == Rd.From && A.Count == Rd.Size);
				}
				// The party: every one in by now (none still to come), and everyone whose stay hasn't ended is in the count. (A frame that
				// spans minutes, a hand's step past the welcome raid's minute, can see a short stayer come and go inside it.)
				int Party = 0, Here = 0, Due = 0, Stayed = 0;
				for (const kast::RoomState::Raider& X : R->Raiders)
				{
					if (X.RaidIx == static_cast<int>(Seen))
					{
						++Party;
						Here += R->RaiderHere(X, R->Clock) ? 1 : 0;
						Due += X.Arrive > R->Clock ? 1 : 0;
						Stayed += X.Leave > R->Clock ? 1 : 0;
					}
				}
				const bool Whole = Party == Rd.Size && Due == 0 && Here == Stayed && R->RaidersHere >= Here && (Rd.At < R->Clock || Here == Rd.Size);
				AlertOff += Posted ? 0 : 1;
				PartyOff += Whole ? 0 : 1;
				Broadcasts += Rd.Broadcast ? 1 : 0;
				MomentRaids += Rd.From == "kat_v" ? 1 : 0;
				Welcomes += Rd.Size == 8 && Rd.Cause == kast::PeakRaid && Rd.AtMinute == 40 ? 1 : 0;
				Sizes += (Sizes.empty() ? "" : ", ") + Rd.From + " " + std::to_string(Rd.Size);
				if ((!Posted || !Whole) && First.empty())
				{
					First = std::string(Where) + ": " + Rd.From + "'s alert " + (Posted ? "posted" : "not posted") + " with " + std::to_string(Here) + " of " + std::to_string(Rd.Size) +
						" here (" + std::to_string(Due) + " still to come) at uptime " + std::to_string(R->Clock);
				}
			}
		};
		double T = 0.0;
		int H = 0;
		bool Ft = false, MomentRaid = false;
		while (T < 50.0)
		{
			const double Hand = Hands[static_cast<size_t>(H++) % 7];
			const double RealHand = Hand * P.RealPerWorld;
			for (double Rs = 0.25; Rs <= RealHand + 1.0e-9; Rs += 0.25)
			{
				In.Real += 0.25;
				St.Tick(Ch, In, 0.0, 0.25);
				Look("a frame");
				if (!Ft && T >= 36.0 && Rs >= 0.5 * RealHand)
				{
					Ft = true;
					St.DebugRoom(Ch, In, "moment ft 1.40 980");
					Look("the final table");
				}
				if (!MomentRaid && T >= 44.0 && Rs >= 0.5 * RealHand)
				{
					// A moment raid as MomentStep adds one (Kind 1), then the session's next call into the room.
					MomentRaid = true;
					kast::RoomState::AddRaid(St, Ch, In, "kat_v", 0x8b5cf6, 12, false, R->Clock, 1);
					St.AdvanceTo(Ch, In);
					Look("a moment raid");
				}
				St.Notices.clear();
			}
			T += Hand;
			In.World = W0 + T; // the hand ends: the table's clock steps
		}
		St.Stop(Ch, In);
		std::printf("  pie (F1) alert and count, %s: %d raids (%s); alert not posted %d, party not all here and counted %d, View off the count %d; named lines from "
					"nobody here %d%s%s\n",
			P.Name, Raids, Sizes.c_str(), AlertOff, PartyOff, ViewOff, W.Absent + W.AtZero, First.empty() ? "" : "; first: ", First.c_str());
		Expect(Broadcasts == 1 && MomentRaids == 1 && Welcomes == 1, std::string("pie (F1): the night holds the Broadcast, a moment raid and the welcome raid (") + P.Name + ")");
		Expect(AlertOff == 0 && PartyOff == 0, std::string("pie (F1): each raid's alert and its whole party land on the same call, in the count (") + P.Name + ")");
		Expect(ViewOff == 0 && W.Absent == 0 && W.AtZero == 0, std::string("pie (F1): the View draws the room's count, and no named raider line comes before the raider (") + P.Name + ")");
	}
}

/**
 * F1: the end card's peak names what brought the room up. A small room's final table whose Broadcast fills it, then a big pot and
 * an all-in before the clock moves on (the PIE night ended "57 on a big pot"): "at the final table". A raid that makes the peak,
 * with a big pot after it: "on a raid". A night with neither keeps the moment the peak came at.
 */
void PiePeakLabel()
{
	const double W0 = RoomDay * 62.0 + 21.0 * 60.0;
	auto Night = [&](const char* Seed, bool Ft, bool Raid) {
		kast::Channel Ch = RoomCommunity(6, 12, 30, W0);
		Ch.WelcomeRaidDone = true;
		kast::Stream St(Seed);
		kast::Inputs In = RoomInputs(W0, 6111);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 36.0);
		In.World += 0.25; // mid-hand
		if (Ft)
		{
			St.DebugRoom(Ch, In, "moment ft 1.40 980");
		}
		if (Raid)
		{
			St.DebugRoom(Ch, In, "raid 30");
		}
		St.OnMoment(Ch, In, kast::Moment::BigPot, "somebody", 1.0);
		St.OnMoment(Ch, In, kast::Moment::AllIn, "somebody", 1.0);
		In.World -= 0.25;
		RoomRun(St, Ch, In, 9.0);
		return St.Stop(Ch, In);
	};
	const kast::Summary A = Night("kast-pie-peak-ft", true, false);
	const kast::Summary B = Night("kast-pie-peak-raid", false, true);
	const std::string La = kast::copy::PeakLine(A.Peak, A.PeakMoment, "9:37 PM");
	const std::string Lb = kast::copy::PeakLine(B.Peak, B.PeakMoment, "9:37 PM");
	std::printf("  pie (F1) the peak's label: the Broadcast's final table then a big pot and an all-in: \"%s\" (minute %.0f); a raid of 30 then a big pot: \"%s\"\n", La.c_str(),
		A.PeakAt, Lb.c_str());
	Expect(A.PeakMoment == static_cast<int>(kast::Moment::FinalTable) && A.PeakAt >= 36.0 && La.find("at the final table") != std::string::npos,
		"pie (F1): a final table's Broadcast peak reads \"at the final table\", not the big pot or all-in after it");
	Expect(B.PeakMoment == kast::PeakRaid && B.PeakAt >= 36.0 && Lb.find("on a raid") != std::string::npos, "pie (F1): a raid's peak reads \"on a raid\"");
	// The cause, unit by unit: a deep run's stage in the last 10 minutes, else the moment the peak came at, else none.
	{
		kast::RoomState Rs;
		Rs.LastMomentKind = static_cast<int>(kast::Moment::BigPot);
		Rs.LastMomentAt = 40.0;
		Expect(Rs.PeakCause(41.0) == static_cast<int>(kast::Moment::BigPot) && Rs.PeakCause(51.0) == -1, "pie (F1): no raid and no stage: the moment the peak came at (in 10 minutes), as before");
		Rs.StageCauseKind = static_cast<int>(kast::Moment::InTheMoney);
		Rs.StageCauseAt = 35.0;
		Expect(Rs.PeakCause(41.0) == static_cast<int>(kast::Moment::InTheMoney) && Rs.PeakCause(46.0) == static_cast<int>(kast::Moment::BigPot),
			"pie (F1): a deep run's stage in the last 10 minutes names the peak over a moment after it (past 10, the moment again)");
	}
}

/**
 * F4 (PIE_CK_NOTES): a short stream with a real final table moves Heat by its news, once a tournament. A young channel (stream 3)
 * goes live at the $11 Big Stack's final table for 36 world minutes: its news commits (+25: Running hot), the 60-minute rule still
 * holds back the rest (no log row, no statistics, no showing up), the end card's line is the run ("Tonight's final table (980
 * runners)") and off air the channel never reads "New channel" after it. Restarts farm nothing: five 3-minute restarts, the same
 * final table again, then a deeper stage adds only its own part; a long stream after it adds no second news; another tournament's
 * final table is its own news; a run with no tournament id keeps the rule; ss.Kast.Moment's held run is keyed for its day.
 */
void PieShortRun()
{
	const double W0 = RoomDay * 70.0 + 20.0 * 60.0; // a Monday, 8 PM: every stream below is in one world week
	kast::Channel Ch = RoomCommunity(8, 20, 2, W0);
	kast::Stream St("kast-pie-short");
	kast::Inputs In = RoomInputs(W0, 6201);
	const std::string IdA = "bigstack@100800", IdB = "bigstack@100860";
	auto Shown = [&](double World) { return growth::HeatShown(Ch.Heat, Ch.HeatAt, World, Ch.LastStreamDay); };
	// 1. Live at the final table for 36 world minutes.
	PieDeep(In, IdA);
	const size_t Logged = Ch.Log.size();
	const double Norm = Ch.Norm, Bias = Ch.Bias, RunNorm = Ch.RunNorm;
	const int ShowDays = Ch.ShowDays, Streak = Ch.Streak, WeekStreams = Ch.WeekStreams;
	const double Minutes = Ch.MinutesLive;
	St.Start(Ch, In);
	const double Before = kast::RoomState::Of(St)->HeatStart;
	RoomRun(St, Ch, In, 4.0);
	St.OnMoment(Ch, In, kast::Moment::FinalTable, "$11 Big Stack", 1.0);
	RoomRun(St, Ch, In, 32.0);
	const kast::Summary S1 = St.Stop(Ch, In);
	const double After1 = Shown(In.World);
	const std::array<std::string, 2> Off1 = kast::copy::WhyOffline(Ch, Ch.LastStreamDay, In.World);
	std::printf("  pie (F4) live at a final table for %.0f world minutes: Heat %.1f -> %.1f (%s, +%.1f committed), the card's line \"%s\", off air \"%s\"\n", S1.Minutes, Before,
		After1, kast::copy::TierName(growth::Tier(After1)).c_str(), S1.GainSoFar, S1.Why[0].c_str(), Off1[0].c_str());
	Expect(S1.Minutes < 60.0 && S1.GainSoFar >= 24.5 && growth::Tier(After1) >= 2 && Ch.GainUsed >= 24.5,
		"pie (F4): a final table on a stream under 60 world minutes commits its Heat news (Running hot)");
	Expect(Ch.Log.size() == Logged && Ch.Norm == Norm && Ch.Bias == Bias && Ch.RunNorm == RunNorm && Ch.MinutesLive == Minutes && Ch.ShowDays == ShowDays && Ch.Streak == Streak &&
			   Ch.WeekStreams == WeekStreams,
		"pie (F4): the 60-minute rule still holds back the performance part, the statistics, the log row, showing up and the streak");
	Expect(S1.Why[0] == kast::copy::WhyRun(growth::RunStage::FinalTable, 980, 0, std::string()) && S1.Why[1].empty(),
		"pie (F4): the short night's card names its run (\"Tonight's final table (980 runners)\")");
	Expect(Off1[0] == kast::copy::WhyRunPlain(growth::RunStage::FinalTable, 0, std::string()) && Off1[0] != kast::copy::WhyNew() && Off1[1] != kast::copy::WhyNew(),
		"pie (F4): off air after a final table the channel never reads \"New channel\" (\"Tonight's final table\")");
	// 2. Restart farming: five 3-minute restarts in the same tournament (its final table already fired): nothing, and the card says why.
	const double Used1 = Ch.GainUsed;
	double Farmed = 0.0;
	bool WhyShort = true;
	std::string Live2;
	for (int K = 0; K < 5; ++K)
	{
		RoomNext(In, In.World + 2.0);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 3.0);
		if (K == 0)
		{
			St.Tick(Ch, In, 0.0, 0.1);
			Live2 = St.GetView().Why[0];
		}
		const kast::Summary S = St.Stop(Ch, In);
		Farmed += S.GainSoFar;
		WhyShort = WhyShort && S.Why[0] == kast::copy::WhyShort();
	}
	std::printf("  pie (F4) five 3-minute restarts: Heat committed %+.1f, the week's gains %.1f -> %.1f; the next live why line \"%s\"\n", Farmed, Used1, Ch.GainUsed, Live2.c_str());
	Expect(Farmed == 0.0 && Ch.GainUsed == Used1 && WhyShort, "pie (F4): restart farming gains nothing (and each card says why: \"Under an hour: ...\")");
	Expect(Live2 == kast::copy::WhyRun(growth::RunStage::FinalTable, 980, 0, std::string()), "pie (F4): the next stream's live why line names the run that counted");
	// 3. The same final table again (its key): nothing.
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.OnMoment(Ch, In, kast::Moment::FinalTable, "$11 Big Stack", 1.0);
	RoomRun(St, Ch, In, 3.0);
	const kast::Summary S3 = St.Stop(Ch, In);
	Expect(S3.GainSoFar == 0.0 && Ch.GainUsed == Used1 && S3.Why[0] == kast::copy::WhyShortCounted(), "pie (F4): a tournament's news counts once (\"This run's Heat already counted\")");
	// 4. Another tournament's final table is its own news (the week's cap leaves 10).
	PieDeep(In, IdB);
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.OnMoment(Ch, In, kast::Moment::FinalTable, "$11 Big Stack", 1.0);
	RoomRun(St, Ch, In, 3.0);
	const kast::Summary S4 = St.Stop(Ch, In);
	Expect(S4.GainSoFar > 0.5 && Ch.GainUsed <= 35.0 + 1.0e-9 && S4.Why[0] == kast::copy::WhyRun(growth::RunStage::FinalTable, 980, 0, std::string()),
		"pie (F4): another tournament's final table is its own news, within the week's cap");
	// 5. A deeper stage of the first run adds only its own part: Top 3 (1.3 P) past the final table (1.0 P) that counted.
	Ch.GainWeek = -1; // the week's cap aside, to see the part itself
	Ch.GainUsed = 0.0;
	PieDeep(In, IdA);
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.OnMoment(Ch, In, kast::Moment::Top3, "$11 Big Stack", 1.0);
	RoomRun(St, Ch, In, 3.0);
	const double P = growth::Prestige(980, 1100, 0.0);
	const double Ft = growth::RunValue(growth::RunStage::FinalTable) * P, Top3 = growth::RunValue(growth::RunStage::Top3) * P;
	const double Heat5 = Shown(In.World);
	const double Part = growth::Gain(Ch.Bias, Ch.Bias, Ch.RunNorm + (Top3 - std::max(Ch.RunNorm, Ft)), Ch.RunNorm, 0.0, Heat5).Gain;
	const double Whole = growth::Gain(Ch.Bias, Ch.Bias, Top3, Ch.RunNorm, 0.0, Heat5).Gain;
	const kast::Summary S5 = St.Stop(Ch, In);
	std::printf("  pie (F4) the same run's top 3 on a later short stream: +%.2f (its own part %.2f; the whole run's news would be %.2f)\n", S5.GainSoFar, Part, Whole);
	Expect(std::fabs(S5.GainSoFar - Part) < 1.0e-9 && Part + 1.0 < Whole, "pie (F4): a deeper stage of a run that counted adds only its own part");
	// 6. A long stream of the same run (its final table again, 65 world minutes): no second news, only the night itself.
	Ch.GainWeek = -1;
	Ch.GainUsed = 0.0;
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.OnMoment(Ch, In, kast::Moment::FinalTable, "$11 Big Stack", 1.0);
	RoomRun(St, Ch, In, 63.0);
	const kast::RoomState* R6 = kast::RoomState::Of(St);
	const double FcRaw = R6->FcRaw;
	const double Bias6 = Ch.Bias, RunNorm6 = Ch.RunNorm;
	const double Used6 = Ch.GainWeek == static_cast<int>(std::floor(In.World / (7.0 * RoomDay))) ? Ch.GainUsed : 0.0;
	const double Heat6 = Shown(In.World);
	const double Show6 = growth::ShowUp(Ch.ShowWeek, Ch.ShowDays, static_cast<int>(std::floor(In.World / RoomDay)));
	const kast::Summary S6 = St.Stop(Ch, In);
	const double Lr = std::log((static_cast<double>(S6.Avg) + 1.0) / (FcRaw + 1.0));
	const double NoNews = growth::Gain(Lr, Bias6, 0.0, RunNorm6, Used6, Heat6).Gain + Show6;
	const double WithNews = growth::Gain(Lr, Bias6, Ft, RunNorm6, Used6, Heat6).Gain + Show6;
	std::printf("  pie (F4) a long stream of the same run: +%.2f committed (the night alone %.2f; with the run's news again %.2f)\n", S6.GainSoFar, NoNews, WithNews);
	Expect(std::fabs(S6.GainSoFar - NoNews) < 1.0e-9 && WithNews > NoNews + 1.0, "pie (F4): a long stream of a run whose news counted adds no second news");
	// 7. A run with no tournament id keeps the 60-minute rule.
	Ch.GainWeek = -1;
	Ch.GainUsed = 0.0;
	PieDeep(In, std::string());
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.OnMoment(Ch, In, kast::Moment::Win, "$11 Big Stack", 1.0);
	RoomRun(St, Ch, In, 3.0);
	const kast::Summary S7 = St.Stop(Ch, In);
	Expect(S7.GainSoFar == 0.0 && Ch.GainUsed == 0.0, "pie (F4): a run with no tournament id commits nothing under 60 minutes (the rule as it was)");
	// 8. ss.Kast.Moment's held run (no real one): keyed for its world day, so a restart's second press adds nothing.
	In.DeepTable = -1;
	In.DeepEntrants = 0;
	In.Entrants = 120;
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.DebugRoom(Ch, In, "moment ft 1.40 980");
	RoomRun(St, Ch, In, 3.0);
	const kast::Summary S8 = St.Stop(Ch, In);
	RoomNext(In, In.World + 2.0);
	St.Start(Ch, In);
	RoomRun(St, Ch, In, 2.0);
	St.DebugRoom(Ch, In, "moment ft 1.40 980");
	RoomRun(St, Ch, In, 3.0);
	const kast::Summary S9 = St.Stop(Ch, In);
	std::printf("  pie (F4) ss.Kast.Moment ft on short streams: +%.1f, then again after a restart +%.1f (\"%s\")\n", S8.GainSoFar, S9.GainSoFar, S9.Why[0].c_str());
	Expect(S8.GainSoFar > 0.5 && S9.GainSoFar == 0.0 && S9.Why[0] == kast::copy::WhyShortCounted(), "pie (F4): ss.Kast.Moment's held run counts once a world day");
}

/** F4's copy: the short night's lines and the off-air final table, each a fact, against their examples. */
void PieCopy()
{
	Expect(kast::copy::WhyShort() == "Under an hour: only a deep run counts" && kast::copy::WhyShortCounted() == "This run's Heat already counted" &&
			   kast::copy::WhyShortUsual() == "Runs this deep are your usual now" && kast::copy::WhyCeiling() == "Heat is at its ceiling" &&
			   kast::copy::WhyRunPlain(growth::RunStage::FinalTable, 1, "Sunday") == "Last night's final table" &&
			   kast::copy::WhyRunPlain(growth::RunStage::FinalTable, 3, "Sunday") == "Sunday's final table",
		"pie (F4): the short night's why lines and the off-air final table read as written");
	// Off air: a final table 6 world days ago still reads, 7 days ago it doesn't; the most recent day wins.
	const double World = 300.0 * RoomDay + 900.0;
	const int Today = static_cast<int>(std::floor(World / RoomDay));
	kast::Channel Ch;
	Ch.Streams = 2;
	Ch.LastStreamDay = Today;
	Ch.FtBase = Today - 1;
	Ch.FtDays = 1 | 8;
	std::array<std::string, 2> W = kast::copy::WhyOffline(Ch, Ch.LastStreamDay, World);
	Expect(W[0] == kast::copy::WhyRunPlain(growth::RunStage::FinalTable, 1, std::string()) && W[1].empty(), "pie (F4): off air, last night's final table first (no \"New channel\")");
	Ch.FtBase = Today - 6;
	Ch.FtDays = 1;
	W = kast::copy::WhyOffline(Ch, Ch.LastStreamDay, World);
	Expect(W[0] == kast::copy::WhyRunPlain(growth::RunStage::FinalTable, 6, ss::net::WeekdayName(Today - 6, true)), "pie (F4): off air, a final table 6 days ago by its weekday");
	Ch.FtBase = Today - 7;
	W = kast::copy::WhyOffline(Ch, Ch.LastStreamDay, World);
	Expect(W[0] == kast::copy::WhyNew(), "pie (F4): a final table 7 days ago is no longer a reason (a new channel reads new)");
}

/**
 * review-pie-room (C-K PIE F1, F4): what the checks above leave open. The network's raids on a world clock that moves a little a
 * frame (a hand-stepped night always jumps past half a minute, so it can't see a party that comes half a minute after its alert):
 * the welcome raid's alert and its whole party land in the same call on the frame that stops on its minute. PeakCause at its
 * limits, unit by unit: a raid party under a fifth of the room isn't the cause (a fifth is), nor one older than 10 world minutes;
 * a live deep run's latest stage names a later peak. The ghost of a later stream of a run whose news counted. A short stream at a
 * final table that formed before the go-live says so.
 */
void PieReviewRoom()
{
	{
		const double W0 = RoomDay * 63.0 + 21.0 * 60.0;
		kast::Channel Ch = RoomCommunity(6, 12, 30, W0);
		kast::Stream St("kast-pie-review-welcome");
		kast::Inputs In = RoomInputs(W0, 6131);
		St.Start(Ch, In);
		const kast::RoomState* R = kast::RoomState::Of(St);
		RoomWatch W;
		size_t Seen = 0;
		int Raids = 0, Welcomes = 0, Off = 0;
		std::string First;
		for (int F = 1; F <= 41 * 50 && St.Live; ++F)
		{
			In.World = W0 + static_cast<double>(F) / 50.0; // a fiftieth of a world minute a frame: one frame stops on uptime 40.0
			In.Real += 0.672;                              // Full's 33.6 real s a world minute
			St.Tick(Ch, In, 0.0, 0.672);
			RoomObserve(St, W, In.Real);
			for (; Seen < R->Raids.size(); ++Seen)
			{
				const kast::RoomState::Raid& Rd = R->Raids[Seen];
				bool Posted = false;
				for (const kast::Alert& A : St.Feed)
				{
					Posted = Posted || (A.Kind == kast::AlertKind::Raid && A.Who == Rd.From && A.Count == Rd.Size);
				}
				int Party = 0, Here = 0;
				for (const kast::RoomState::Raider& X : R->Raiders)
				{
					if (X.RaidIx == static_cast<int>(Seen))
					{
						++Party;
						Here += R->RaiderHere(X, R->Clock) ? 1 : 0;
					}
				}
				++Raids;
				Welcomes += Rd.Size == 8 && Rd.Cause == kast::PeakRaid && Rd.AtMinute == 40 ? 1 : 0;
				const bool Ok = Posted && Party == Rd.Size && Here == Rd.Size && St.GetView().Count == R->Watchers;
				Off += Ok ? 0 : 1;
				if (!Ok && First.empty())
				{
					First = Rd.From + "'s alert " + (Posted ? "posted" : "not posted") + " at uptime " + std::to_string(R->Clock) + " with " + std::to_string(Here) + " of " +
						std::to_string(Rd.Size) + " here";
				}
			}
			St.Notices.clear();
		}
		St.Stop(Ch, In);
		std::printf("  pie review (F1) the welcome raid on a clock a fiftieth of a world minute a frame: %d raid(s), %d not in with the alert%s%s; named lines from nobody "
					"here %d\n",
			Raids, Off, First.empty() ? "" : "; first: ", First.c_str(), W.Absent + W.AtZero);
		Expect(Welcomes == 1 && Off == 0 && W.Absent == 0 && W.AtZero == 0,
			"pie review (F1): the network's raid is in the count with its alert on the frame that stops on its minute (no line before it)");
	}
	{
		kast::RoomState Rs;
		Rs.Watchers = 30;
		Rs.StageCauseKind = static_cast<int>(kast::Moment::InTheMoney);
		Rs.StageCauseAt = 35.0;
		Rs.LastMomentKind = static_cast<int>(kast::Moment::BigPot);
		Rs.LastMomentAt = 38.0;
		kast::RoomState::Raid Rd;
		Rd.At = 36.0;
		Rd.AtMinute = 36;
		Rd.Size = 6;
		Rd.Cause = kast::PeakRaid;
		Rs.Raids.push_back(Rd);
		for (int I = 0; I < 6; ++I)
		{
			kast::RoomState::Raider X;
			X.RaidIx = 0;
			X.Arrive = 36.0;
			X.Leave = 90.0;
			X.Gone = I == 0; // one followed: a member now, not a raider
			Rs.Raiders.push_back(X);
		}
		const int Under = Rs.PeakCause(40.0); // 5 of 30 here
		Rs.Raiders[0].Gone = false;
		const int Fifth = Rs.PeakCause(40.0); // 6 of 30
		const int Edge = Rs.PeakCause(46.0);  // the party 10 world minutes in
		const int Late = Rs.PeakCause(46.5);  // past 10 (the stage too, at 35): the moment at 38 names it
		Expect(Under == static_cast<int>(kast::Moment::InTheMoney) && Fifth == kast::PeakRaid && Edge == kast::PeakRaid && Late == static_cast<int>(kast::Moment::BigPot),
			"pie review (F1): a raid party is the peak's cause from a fifth of the room and for 10 world minutes (else the stage, else the moment)");
		// While the deep run is still live (its Sweat brings the room up with its depth), its latest stage names a peak past those 10
		// minutes; a raid party of a fifth still comes first.
		Rs.RunDepth = 0.6;
		const int Live = Rs.PeakCause(46.5);
		const int LiveRaid = Rs.PeakCause(40.0);
		Rs.RunDepth = 0.0;
		const int Over = Rs.PeakCause(46.5);
		Expect(Live == static_cast<int>(kast::Moment::InTheMoney) && LiveRaid == kast::PeakRaid && Over == static_cast<int>(kast::Moment::BigPot),
			"pie review (F1): a live deep run's latest stage names a later peak (a raid party of a fifth first; the run over, the moment again)");
	}
	{
		// F4: the ghost of a later stream of a run whose news counted shows that stage's own part, as Stop commits it, not the whole
		// run's news again. The channel's Bias is held high so the night's own part sits at its floor (-9) and the week's cap stays
		// out of the way (at the cap the two ghosts read the same).
		const double W0 = RoomDay * 77.0 + 20.0 * 60.0;
		kast::Channel Ch = RoomCommunity(8, 20, 4, W0);
		Ch.Bias = 3.0;
		kast::Stream St("kast-pie-review-ghost");
		kast::Inputs In = RoomInputs(W0, 6141);
		PieDeep(In, "bigstack@110880");
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 2.0);
		St.OnMoment(Ch, In, kast::Moment::FinalTable, "$11 Big Stack", 1.0);
		RoomRun(St, Ch, In, 3.0);
		const kast::Summary S1 = St.Stop(Ch, In);
		Ch.GainWeek = -1; // the week's cap aside
		Ch.GainUsed = 0.0;
		RoomNext(In, In.World + 2.0);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 2.0);
		St.OnMoment(Ch, In, kast::Moment::Top3, "$11 Big Stack", 1.0);
		RoomRun(St, Ch, In, 3.0);
		const kast::RoomState* R = kast::RoomState::Of(St);
		const double P = growth::Prestige(980, 1100, 0.0);
		const double Ft = growth::RunValue(growth::RunStage::FinalTable) * P, Top3 = growth::RunValue(growth::RunStage::Top3) * P;
		// The last grid minute's ghost: the running average against Start's forecast, the Heat shown at that minute.
		const double Lr = std::log((R->VM / static_cast<double>(R->Done) + 1.0) / (R->FcRaw + 1.0));
		const double Heat = growth::HeatShown(Ch.Heat, Ch.HeatAt, St.StartWorld + static_cast<double>(R->Done - 1), Ch.LastStreamDay);
		const double Part = growth::Gain(Lr, Ch.Bias, Ch.RunNorm + (Top3 - std::max(Ch.RunNorm, Ft)), Ch.RunNorm, 0.0, Heat).Gain;
		const double Whole = growth::Gain(Lr, Ch.Bias, Top3, Ch.RunNorm, 0.0, Heat).Gain;
		const double Ghost = R->GhostNow;
		const kast::Summary S2 = St.Stop(Ch, In);
		std::printf("  pie review (F4) a run's final table (+%.1f), then its top 3 on a later short stream: the ghost %+.2f (the stage's part %+.2f; the whole run's %+.2f), "
					"committed %+.2f\n",
			S1.GainSoFar, Ghost, Part, Whole, S2.GainSoFar);
		Expect(S1.GainSoFar > 0.5 && std::fabs(Ghost - Part) < 1.0e-9 && Whole > Part + 1.0, "pie review (F4): the ghost of a run whose news counted shows only the deeper stage's part");
	}
	{
		// F4: live at a final table that had formed before the go-live: no stage fires on stream, so no news (the stages are the
		// stream's moments), and the card says so rather than "only a deep run counts"; the same short stream at a run not yet in
		// the money keeps that line.
		const double W0 = RoomDay * 84.0 + 20.0 * 60.0;
		kast::Channel Ch = RoomCommunity(8, 20, 4, W0);
		kast::Stream St("kast-pie-review-formed");
		kast::Inputs In = RoomInputs(W0, 6151);
		PieDeep(In, "bigstack@120960");
		In.DeepPaid = 135;
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 30.0);
		const kast::Summary Formed = St.Stop(Ch, In);
		In.DeepRemaining = 400;
		In.Remaining = 400;
		In.Depth = growth::Depth(980, 400);
		RoomNext(In, In.World + 2.0);
		St.Start(Ch, In);
		RoomRun(St, Ch, In, 30.0);
		const kast::Summary Early = St.Stop(Ch, In);
		std::printf("  pie review (F4) live at a final table that had formed, 30 world minutes: Heat %+.1f, \"%s\"; at 400 left of 980 (not yet paid): %+.1f, \"%s\"\n",
			Formed.GainSoFar, Formed.Why[0].c_str(), Early.GainSoFar, Early.Why[0].c_str());
		Expect(kast::copy::WhyShortBefore() == "Stages before going live don't count" && Formed.GainSoFar == 0.0 && Formed.Why[0] == kast::copy::WhyShortBefore() &&
				   Early.GainSoFar == 0.0 && Early.Why[0] == kast::copy::WhyShort(),
			"pie review (F4): a short stream at a final table that formed before it says its stages came before the go-live");
	}
}

/** Tick's wall time on a 1,500-member channel with three mods (§9.4 #13: <= 0.1 ms during a sprint frame): 1x at 60 fps, hand-
 *  sized sprint frames (0.5-2.25 world minutes, the sprint pace's), then a 10-minute and a 100-minute skip frame. Information only
 *  (the machine's load moves it): the default run times one night, KAST_BENCH=1 three and prints the best (KAST_BENCH=only runs
 *  nothing else). The night's fingerprint (every View the frames left, every chat line, the Channel's bytes) is printed with it: a
 *  change made for speed must leave it as it was. KAST_BENCH also times the same nights with a deep run's bubble at the sprint's
 *  start (C-K's M2: the room's hype then moves every minute, as it does on most MTT nights; its own fingerprint). */
void RoomBenchNights(bool Full, int Variant);
void RoomBench()
{
	const char* Env = std::getenv("KAST_BENCH");
	const bool Full = Env != nullptr;
	for (int Variant = 0; Variant < (Full ? 3 : 1); ++Variant)
	{
		RoomBenchNights(Full, Variant);
	}
}

/** Variant 0 the bench's night; 1 with a bubble's hype moving; 2 the decided shape (C-K's M2: 1,500 members and 300 subs, as
 *  ss.Kast.Grant 1500 300 makes them; review-fix-room), the followers last seen 0.5-30 world days ago, so some cross the Ping's
 *  7- and 21-day steps while live. */
void RoomBenchNights(bool Full, int Variant)
{
	const bool Stage = Variant == 1;
	const bool Grant = Variant == 2;
	const char* Label = Stage ? ", a bubble's hype moving" : Grant ? ", 300 subs, last seen 0.5-30 days ago" : "";
	const double W0 = RoomDay * 30.0 + 21.0 * 60.0; // a Wednesday, 9 PM
	double Best[5] = {1.0e9, 1.0e9, 1.0e9, 1.0e9, 1.0e9};
	int Here = 0, Members = 0;
	uint64_t Print = 1469598103934665603ull;
	auto Mix = [&](const void* P, size_t N) {
		const unsigned char* B = static_cast<const unsigned char*>(P);
		for (size_t I = 0; I < N; ++I)
		{
			Print = (Print ^ B[I]) * 1099511628211ull;
		}
	};
	for (int Night = 0; Night < (Full ? 3 : 1); ++Night)
	{
		kast::Channel Ch = RoomCommunity(1500, 4000, 300, W0);
		Ch.Affiliate = true;
		Ch.ScheduleDays = 0x7f;
		Ch.ScheduleStart = 21 * 60;
		for (int K = 0; K < 3; ++K)
		{
			kast::Moderator Mod;
			Mod.Name = "kastfan_" + std::to_string(K * 37 + 9);
			Ch.Mods.push_back(Mod);
		}
		if (Grant)
		{
			for (size_t K = 0; K < Ch.Members.size(); ++K)
			{
				Ch.Members[K].LastSeen = W0 - RoomDay * (0.5 + std::fmod(static_cast<double>(K) * 0.37, 29.5));
			}
			for (int K = 0; K < 300; ++K)
			{
				kast::Subscriber Sb;
				Sb.Name = "kastfan_" + std::to_string(K * 5);
				Sb.Since = W0 - RoomDay * static_cast<double>(1 + K % 25);
				Sb.Renews = Sb.Since + 30.0 * RoomDay;
				Ch.Subs.push_back(Sb);
			}
		}
		kast::Stream St("kast-bench");
		kast::Inputs In = RoomInputs(W0, 7700 + static_cast<uint64_t>(Night));
		St.Start(Ch, In);
		int LastId = 0;
		double Took = 0.0;
		auto Frame = [&](double DW, double DR) {
			In.World += DW;
			In.Real += DR;
			const auto A = std::chrono::steady_clock::now();
			St.Tick(Ch, In, DW, DR);
			Took += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - A).count();
			St.Notices.clear();
			St.Deals.clear();
			const kast::View& V = St.GetView();
			Mix(&V.Count, sizeof(V.Count));
			Mix(&V.ShownCount, sizeof(V.ShownCount));
			Mix(&V.Chatting, sizeof(V.Chatting));
			Mix(V.Sources.data(), sizeof(int) * V.Sources.size());
			Mix(V.MemberFlags.data(), V.MemberFlags.size());
			for (const kast::ChatMsg& M : St.Chat)
			{
				if (M.Id > LastId)
				{
					LastId = M.Id;
					Mix(M.Who.data(), M.Who.size());
					Mix(M.Text.data(), M.Text.size());
					Mix(&M.At, sizeof(M.At));
				}
			}
		};
		for (int K = 0; K < 30; ++K)
		{
			Frame(1.0, 13.1); // the room fills
		}
		// 1x at 60 fps: 1/60 real s a frame, 13.1 real s a world minute.
		{
			const int N = Full ? 1572 : 786;
			Took = 0.0;
			for (int K = 0; K < N; ++K)
			{
				Frame(1.0 / 60.0 / 13.1, 1.0 / 60.0);
			}
			Best[0] = std::min(Best[0], Took / N);
		}
		// Sprint: heads-down, hand-sized frames.
		In.Attended = false;
		In.Sprinting = true;
		if (Stage)
		{
			// A 980-runner field's bubble on stream: the room's hype takes the stage hit and decays from here on.
			kast::Inputs Deep = In;
			Deep.DeepEntrants = Deep.Entrants = 980;
			Deep.DeepRemaining = Deep.Remaining = 150;
			Deep.Depth = 0.3;
			Deep.Prestige = 1.0;
			Deep.DeepTable = 0;
			St.OnMoment(Ch, Deep, kast::Moment::Bubble);
			St.Notices.clear();
		}
		{
			static const double Hands[] = {0.75, 1.5, 2.25, 1.0, 0.5, 1.75, 1.25, 2.0};
			const int N = Full ? 64 : 32;
			double Minutes = 0.0;
			Took = 0.0;
			for (int K = 0; K < N; ++K)
			{
				Frame(Hands[K % 8], 1.0 / 60.0);
				Minutes += Hands[K % 8];
			}
			Best[1] = std::min(Best[1], Took / N);
			Best[2] = std::min(Best[2], Took / Minutes);
		}
		Took = 0.0;
		Frame(10.0, 1.0 / 60.0);
		Best[3] = std::min(Best[3], Took);
		Took = 0.0;
		Frame(100.0, 1.0 / 60.0);
		Best[4] = std::min(Best[4], Took);
		Here = St.Count();
		Members = kast::RoomState::Of(St)->MembersHere;
		St.Stop(Ch, In);
		const std::string D = RoomDigest(Ch);
		Mix(D.data(), D.size());
	}
	std::printf("  room bench%s (1,500 members, %d watching, %d of them members%s): Tick %.4f ms at 1x 60 fps; a sprint frame (1.25 world minutes on average) %.4f ms, "
				"%.4f ms a grid minute; a 10-minute frame %.3f ms, a 100-minute frame %.3f ms\n",
		Label, Here, Members, Full ? ", best of three nights" : "", Best[0], Best[1], Best[2], Best[3], Best[4]);
	std::printf("  room bench%s: the night's fingerprint %016llx%s\n", Label, static_cast<unsigned long long>(Print), Full ? " (three nights)" : "");
}
} // namespace room_seg

// ------------------------------------------------------------------ the careers (§6, §9.2's C++ side)

namespace room_career
{
namespace kast = ss::kast;
namespace growth = ss::kast::growth;

constexpr double CareerDay = 1440.0;

/** An archetype as synth_kast.py plays it (ARCH): the schedule, the tables and fields, the clicks at their allowed rates. */
struct CareerArch
{
	std::string Name = "regular";
	int Days = 0x1f;            // weekdays streamed (bit 0 Monday)
	double Hours = 3.0;
	int Start = 21 * 60;
	int Jitter = 0;             // +- world minutes, at random
	bool Sched = true;          // a posted schedule
	double ThankEvery = 600.0;  // real seconds between Thank presses at attended minutes (0: never)
	double Answer = 0.6;        // the share of chat's questions the streamer takes
	bool RaidOut = true;
	int Tables = 2;
	int Field = 800;
	double FieldSd = 0.35;
	int FieldMin = 50;
	double BuyIn = 3.30;
	double Itm = 0.13, Ft = 0.016, Win = 0.002; // P(ITM), P(final table), P(win) an MTT
	double DScale = 1.0;
	bool Majors = false;        // a Sunday major on table 0 (5,000 runners, $109)
	int Nods = 1;               // member Nods a stream (attended)
	int QDay1 = 60, QDay2 = 1000000;
	double Q0 = 0.29, Q1 = 0.59, Q2 = 1.0;
	double W4 = 0.0, Skip = 0.0; // shares of streams at watch 4x and skipped
	bool Stops = true;          // autoplay's default stops (the bubble, depth >= 0.9) are attended
	bool Welcome = true;        // every raid welcomed
	bool RailCall = true;       // RAIL CALL pressed when offered
	int Micro = 0;              // 2-minute raid-out restarts after each stream (the restart lever)
	double ThankLever = 0.0;    // the spam lever: Thank every this many real seconds at every pace (0: off)
	int SpamNods = 0;           // Nods a stream on guests (the spam lever)
	bool Trace = false;         // record the room minute by minute (LongMtt)
	bool NoDeep = false;        // fields too small to arm (LongMtt's non-deep nights)
	int Title = 0;              // Channel::Title (0: the default)
};

CareerArch CareerArchOf(const std::string& Name)
{
	CareerArch A;
	A.Name = Name;
	const std::string N = Name;
	if (N == "casual" || N == "casual_micro")
	{
		A.Days = (1 << 1) | (1 << 5);
		A.Hours = 2.0;
		A.Start = 20 * 60;
		A.Jitter = 150;
		A.Sched = false;
		A.ThankEvery = 0.0;
		A.Answer = 0.2;
		A.RaidOut = false;
		A.Tables = 1;
		A.Nods = 0;
		A.QDay1 = 1000000;
		A.Welcome = false;
		A.Micro = N == "casual_micro" ? 3 : 0;
	}
	else if (N == "grinder" || N == "grinder_strong" || N == "grinder_micro")
	{
		A.Days = 0x7f;
		A.Hours = 6.0;
		A.Start = 19 * 60;
		A.Tables = 4;
		A.QDay1 = 30;
		A.QDay2 = 150;
		if (N == "grinder_strong")
		{
			A.Itm = 0.20;
			A.Ft = 0.045;
			A.Win = 0.009;
		}
		A.Micro = N == "grinder_micro" ? 10 : 0;
	}
	else if (N == "deeprun")
	{
		A.Days = 0x0f | (1 << 6);
		A.Tables = 1;
		A.Field = 1200;
		A.BuyIn = 5.50;
		A.Itm = 0.20;
		A.Ft = 0.045;
		A.Win = 0.009;
		A.Majors = true;
	}
	else if (N == "autoplay")
	{
		A.W4 = 0.5;
		A.Skip = 0.5;
	}
	else if (N == "lever_spam")
	{
		A.ThankLever = 20.0;
		A.SpamNods = 3;
	}
	else if (N == "lever_silent")
	{
		A.ThankEvery = 0.0;
		A.Answer = 0.0;
		A.Nods = 0;
	}
	else if (N == "lever_noraid")
	{
		A.RaidOut = false;
	}
	else if (N == "lever_answer_all")
	{
		A.Answer = 1.0;
		A.Title = 3; // "ICM explained live": the teaching title (questions x1.6)
	}
	else if (N == "lever_answer_def")
	{
		A.Answer = 1.0; // every question answered on the default title (S-K: the answering apart from the Teaching title's pull)
	}
	else if (N == "lever_teaching")
	{
		// The regular on the Teaching title, answering as the regular does (60%): answer-all's base in §9.1's Levers (the C-K gate),
		// so the lever measures the answering and not the title's own follow pull (x1.13 of the default's, a title choice the model
		// leaves out: S-K.md §3 #7).
		A.Title = 3;
	}
	else if (N == "lever_micro")
	{
		A.Micro = 3;
	}
	else if (N == "lever_hourly")
	{
		A.Tables = 4;
		A.Field = 50;
		A.FieldMin = 40;
		A.FieldSd = 0.15;
		A.DScale = 0.4;
		A.Itm = 0.25;
		A.Ft = 0.25;
		A.Win = 0.04;
	}
	else if (N == "lever_4table")
	{
		A.Tables = 4;
	}
	else if (N == "trace5h" || N == "trace6h")
	{
		A.Hours = N == "trace5h" ? 5.0 : 6.0;
		A.Tables = 1;
		A.Trace = true;
		A.NoDeep = true;
	}
	return A;
}

/** One MTT on a table (plan_tracks): its start, length, field, prestige, the hero's place and when each stage comes. */
struct CareerMtt
{
	double T0 = 0.0, D = 0.0, End = 0.0;
	int N = 0;
	double P = 0.0, BuyIn = 3.3, Series = 0.0;
	int Place = 0;
	double LnN = 0.0;
	double Bubble = -1.0, Itm = -1.0, Ft = -1.0, Top3 = -1.0, Win = -1.0;
};

std::vector<std::vector<CareerMtt>> CareerTracks(const CareerArch& A, int Day, double H, ss::Rng& R)
{
	std::vector<std::vector<CareerMtt>> Tracks;
	for (int K = 0; K < A.Tables; ++K)
	{
		std::vector<CareerMtt> Mtts;
		double T = 5.0 + 3.0 * K;
		bool First = true;
		while (T < H)
		{
			CareerMtt M;
			const bool Major = A.Majors && Day % 7 == 6 && First && K == 0;
			M.N = Major ? 5000 : std::max(A.FieldMin, static_cast<int>(static_cast<double>(A.Field) * std::exp(R.Gauss(0.0, A.FieldSd))));
			M.BuyIn = Major ? 109.0 : A.BuyIn;
			M.Series = Major ? 1.0 : 0.0;
			M.P = growth::Prestige(M.N, static_cast<ss::Chips>(std::lround(M.BuyIn * 100.0)), M.Series);
			M.D = std::max(30.0, (150.0 + 55.0 * std::log2(static_cast<double>(M.N) / 100.0)) * A.DScale);
			const double U = R.Next();
			if (U < A.Win)
			{
				M.Place = 1;
			}
			else if (U < A.Ft)
			{
				M.Place = 2 + R.Int(8);
			}
			else if (U < A.Itm)
			{
				const int Hi = std::max(11, static_cast<int>(0.15 * M.N));
				M.Place = 10 + R.Int(std::max(1, Hi - 10));
			}
			else
			{
				const int Lo = static_cast<int>(0.15 * M.N) + 1;
				M.Place = Lo + R.Int(std::max(1, M.N + 1 - Lo));
			}
			M.LnN = std::log(static_cast<double>(M.N));
			auto Tf = [&](double Rk) { return std::pow(std::log(static_cast<double>(M.N) / Rk) / M.LnN, 0.55); };
			M.T0 = T;
			M.End = T + (M.Place == 1 ? M.D : M.D * Tf(M.Place));
			const double Nd = static_cast<double>(M.N);
			M.Bubble = M.Place <= 0.155 * Nd ? T + M.D * Tf(0.155 * Nd) : -1.0;
			M.Itm = M.Place <= 0.15 * Nd ? T + M.D * Tf(0.15 * Nd) : -1.0;
			M.Ft = M.Place <= 9 ? T + M.D * Tf(9.0) : -1.0;
			M.Top3 = M.Place <= 3 ? T + M.D * Tf(3.0) : -1.0;
			M.Win = M.Place == 1 ? T + M.D : -1.0;
			Mtts.push_back(M);
			T = M.End + 5.0;
			First = false;
		}
		Tracks.push_back(Mtts);
	}
	return Tracks;
}

/** A stream's record (the momentum, forecast and room statistics read these). */
struct CareerStream
{
	int Day = 0;
	int Avg = 0, Peak = 0, Follows = 0;
	double HeatStart = 0.0, HeatEnd = 0.0;
	double MaxP = 0.0;        // the best prestige of an armed final table or win tonight (0: none)
	bool Deep = false;        // a run past depth 0.3 tonight
	double FcLo = 0.0, FcHi = 0.0, FcCentre = 0.0;
	int ZeroMinutes = 0, Minutes = 0;
	std::vector<std::array<double, 2>> ByHour; // (count, members) by world hour (traced)
	int Stream = 0;
	std::array<double, 7> Parts{}; // the night's mean members present, strangers (guests and crowd), raiders, lurkers; Hype; Engage;
	                               // chat Health
	std::array<double, 9> Ping{};  // members at the go-live, coming at the Ping, their Pa summed, coming by the end, Start's raw
	                               // forecast, the night's lr, its Heat gain, regulars and mean loyalty after
};

struct CareerSnap
{
	int Followers = 0, Subs = 0, Regulars = 0, Members = 0;
	double Avg30 = 0.0, Money30 = 0.0, Heat = 0.0, RealH = 0.0;
	std::array<double, 5> Earned{}; // cumulative dollars earned: tips, cheers, subs live, subs offline (renewals), ads
};

struct CareerRun
{
	std::map<int, CareerSnap> At;
	int AffiliateDay = 999, PartnerDay = 999;
	double Act1Worst = 0.0;
	std::vector<CareerStream> Streams;
	double RealSeconds = 0.0; // real time spent live (and 3 minutes a stream in the lobby)
	std::vector<std::pair<double, double>> Paid; // (world minute, dollars) payouts
	std::array<double, 12> FollowSrc{};          // the career's follows by View::Source, and [11] those made offline (clips)
};

/** A career of Days world days: the archetype's streams on the real Stream, the session's payout rule after each. */
CareerRun CareerRunOf(const CareerArch& A, int Seed, int Days)
{
	CareerRun Out;
	kast::Channel Ch;
	Ch.Title = A.Title;
	if (A.Sched)
	{
		Ch.ScheduleDays = A.Days;
		Ch.ScheduleStart = A.Start;
	}
	kast::Stream St("kast-career-" + A.Name + "-" + std::to_string(Seed));
	// The levers share their base's plans (paired seeds, as synth_kast.py's seedname): the regular's, the casual's, the grinder's.
	const std::string Base = A.Name.rfind("lever_", 0) == 0 ? std::string("regular") : A.Name == "casual_micro" ? std::string("casual") : A.Name == "grinder_micro" ? std::string("grinder") : A.Name;
	ss::Rng R("kast-career:" + Base + ":" + std::to_string(Seed));
	ss::Rng Clicks("kast-career-clicks:" + A.Name + ":" + std::to_string(Seed));
	const uint64_t SeedK = 0x5eed0000ull + static_cast<uint64_t>(Seed) * 7919ull;
	// The rig plus Warden, the chat filter bot (gear "modbot", 75% of trolls and spam): synth_kast.py holds chat Health at 0.97,
	// and an unmoderated room of 300 drops it to 0.1 (strangers and follows x0.64), which no streamer who talks to chat lives with.
	ss::gear::Effects Rig = room_seg::RoomRig();
	Rig.ModBot = std::max(Rig.ModBot, 0.75);
	ss::Rng Off("kast-career-offline");
	double LiveSubs = 0.0; // dollars of subs booked live (the rest of EarnedSubs is renewals)
	auto Book = [&](const kast::Summary& Sm) {
		LiveSubs += static_cast<double>(Sm.SubCents) / 100.0;
		for (size_t K = 0; K < Sm.FollowSources.size(); ++K)
		{
			Out.FollowSrc[K] += static_cast<double>(Sm.FollowSources[K]);
		}
	};
	auto PayOut = [&](double World, int RentsPaid) {
		const growth::Payout P = growth::PayoutOf(Ch.UnpaidCents, Ch.PendingCents, RentsPaid);
		if (P.Pay > 0)
		{
			Out.Paid.emplace_back(World, static_cast<double>(P.Pay) / 100.0);
			Ch.PaidCents += P.Pay;
		}
		Ch.UnpaidCents = 0;
		Ch.PendingCents = P.Pending;
	};
	auto Snapshot = [&](int Day) {
		const double W = static_cast<double>(Day + 1) * CareerDay;
		kast::Offline(Ch, W - 0.5, Off, SeedK, Day < 27 ? 1 : 2);
		double Minutes = 0.0;
		int LiveDays = 0;
		double Avg = 0.0;
		Ch.Window30(W, Minutes, LiveDays, Avg);
		CareerSnap S;
		S.Followers = Ch.Followers;
		S.Avg30 = Avg;
		S.Subs = Ch.ActiveSubs(W);
		S.Regulars = Ch.Regulars();
		S.Members = static_cast<int>(Ch.Members.size());
		S.Heat = growth::HeatShown(Ch.Heat, Ch.HeatAt, W, Ch.LastStreamDay);
		S.RealH = Out.RealSeconds / 3600.0;
		S.Earned = {static_cast<double>(Ch.EarnedTips) / 100.0, static_cast<double>(Ch.EarnedBits) / 100.0, LiveSubs, static_cast<double>(Ch.EarnedSubs) / 100.0 - LiveSubs,
			static_cast<double>(Ch.EarnedAds) / 100.0};
		for (const auto& P : Out.Paid)
		{
			S.Money30 += P.first > W - 30.0 * CareerDay && P.first <= W ? P.second : 0.0;
		}
		Out.At[Day] = S;
	};
	Ch.HeatAt = 0.0;
	for (int Day = 0; Day < Days; ++Day)
	{
		const int RentsPaid = Day < 27 ? 1 : 2;
		const bool Streams = ((A.Days >> (Day % 7)) & 1) != 0;
		if (!Streams)
		{
			kast::Offline(Ch, static_cast<double>(Day) * CareerDay + 23.0 * 60.0, Off, SeedK, RentsPaid);
		}
		else
		{
			const double Jit = A.Jitter > 0 ? std::floor((R.Next() * 2.0 - 1.0) * static_cast<double>(A.Jitter)) : 0.0;
			const double W0 = static_cast<double>(Day) * CareerDay + static_cast<double>(A.Start) + Jit;
			const double H = A.Hours * 60.0;
			const std::vector<std::vector<CareerMtt>> Tracks = CareerTracks(A, Day, H, R);
			double End = H;
			for (const std::vector<CareerMtt>& Mtts : Tracks)
			{
				for (const CareerMtt& M : Mtts)
				{
					if (M.T0 < H && H <= M.End && M.Place <= 0.15 * M.N)
					{
						End = std::max(End, std::min(H + 240.0, M.End));
					}
				}
			}
			// The pace: the autoplayer's streams are half watch 4x, half skipped.
			const double Up = R.Next();
			const int Pace = Up < A.W4 ? 1 : Up < A.W4 + A.Skip ? 2 : 0; // 0 Smart, 1 watch 4x, 2 skip
			const double RealPer[] = {13.1, 3.37, 0.7};
			// The moments: registers, all-ins and their results, big pots, knockouts on every table; the stages; busts.
			struct Ev
			{
				double At;
				kast::Moment Kind;
				int Table;
				int Mtt;
			};
			std::vector<Ev> Evs;
			for (int K = 0; K < static_cast<int>(Tracks.size()); ++K)
			{
				const std::vector<CareerMtt>& Mtts = Tracks[static_cast<size_t>(K)];
				for (int I = 0; I < static_cast<int>(Mtts.size()); ++I)
				{
					const CareerMtt& M = Mtts[static_cast<size_t>(I)];
					if (M.T0 >= End)
					{
						continue;
					}
					Evs.push_back({M.T0 + 0.1, kast::Moment::Register, K, I});
					const std::pair<double, kast::Moment> Stages[] = {{M.Bubble, kast::Moment::Bubble}, {M.Itm, kast::Moment::InTheMoney}, {M.Ft, kast::Moment::FinalTable},
						{M.Top3, kast::Moment::Top3}, {M.Win, kast::Moment::Win}};
					for (const auto& S : Stages)
					{
						if (S.first >= 0.0 && S.first < End)
						{
							Evs.push_back({S.first, S.second, K, I});
						}
					}
					if (M.End < End && M.Place > 1)
					{
						Evs.push_back({M.End, M.Place <= 0.15 * M.N ? kast::Moment::Cashed : kast::Moment::Bust, K, I});
					}
				}
				for (double T = R.Next() * 60.0 / 1.4; T < End; T += -std::log(std::max(1.0e-9, 1.0 - R.Next())) * 60.0 / 1.4)
				{
					Evs.push_back({T, kast::Moment::AllIn, K, -1});
					Evs.push_back({T + 0.25, R.Chance(0.6) ? kast::Moment::WonAllIn : kast::Moment::LostAllIn, K, -1});
				}
				for (double T = R.Next() * 60.0; T < End; T += 30.0 + R.Next() * 60.0)
				{
					Evs.push_back({T, R.Chance(0.7) ? kast::Moment::BigPot : kast::Moment::Knockout, K, -1});
				}
			}
			std::stable_sort(Evs.begin(), Evs.end(), [](const Ev& X, const Ev& Y) { return X.At < Y.At; });
			// The inputs at an uptime: the deepest armed run, the front table's field, the pace and the stops.
			kast::Inputs In = room_seg::RoomInputs(W0, SeedK);
			In.Tables = A.Tables;
			In.RentsPaid = RentsPaid;
			In.Gear = Rig;
			In.Gear.Quality = Day < A.QDay1 ? A.Q0 : Day < A.QDay2 ? A.Q1 : A.Q2;
			In.Gear.StreamTables = 4;
			In.Gear.Follow = In.Gear.Stay = In.Gear.Tips = In.Gear.Hype = 0.0;
			In.Real = static_cast<double>(Day) * 100000.0;
			double MaxP = 0.0;
			bool DeepNight = false;
			auto Fill = [&](double T, bool Stop) {
				In.DeepTable = -1;
				In.DeepEntrants = 0;
				In.DeepRemaining = 0;
				In.Depth = 0.0;
				In.Prestige = 0.0;
				double Best = -1.0;
				for (int K = 0; K < static_cast<int>(Tracks.size()); ++K)
				{
					for (const CareerMtt& M : Tracks[static_cast<size_t>(K)])
					{
						if (M.T0 <= T && T < M.End)
						{
							const double Frac = std::min(1.0, (T - M.T0) / M.D);
							const int Left = std::max(1, static_cast<int>(std::lround(std::pow(static_cast<double>(M.N), 1.0 - std::pow(Frac, 1.0 / 0.55)))));
							if (K == 0)
							{
								In.Entrants = M.N;
								In.Remaining = Left;
							}
							if (growth::Armed(M.N) && !A.NoDeep)
							{
								const double Dp = growth::Depth(M.N, Left);
								if (M.P * Dp > Best)
								{
									Best = M.P * Dp;
									In.DeepTable = K;
									In.DeepEntrants = M.N;
									In.DeepRemaining = Left;
									In.DeepTableSize = 9;
									In.Depth = Dp;
									In.Prestige = M.P;
									In.BuyInCents = static_cast<ss::Chips>(std::lround(M.BuyIn * 100.0));
									In.Series = M.Series;
								}
							}
						}
					}
				}
				DeepNight = DeepNight || In.Depth > 0.3;
				const bool AtStop = A.Stops && (Stop || In.Depth >= 0.9);
				In.Attended = Pace == 0 || AtStop;
				In.WatchSpeed = Pace == 1 && !AtStop ? 4 : 1;
				In.Sprinting = Pace == 2 && !AtStop;
			};
			Fill(0.0, false);
			St.Start(Ch, In);
			const kast::RoomState* Rm = kast::RoomState::Of(St);
			CareerStream Rec;
			Rec.Day = Day;
			Rec.Stream = Ch.Streams;
			Rec.HeatStart = Rm->HeatStart;
			Rec.FcLo = Rm->Fc.Lo;
			Rec.FcHi = Rm->Fc.Hi;
			Rec.FcCentre = Rm->Fc.Centre;
			Rec.Ping[0] = static_cast<double>(Ch.Members.size());
			Rec.Ping[1] = static_cast<double>(Rm->Visits.size());
			for (const double P : Rm->MemberPa)
			{
				Rec.Ping[2] += P;
			}
			Rec.Ping[4] = Rm->FcRaw;
			double NextThank = In.Real + A.ThankEvery;
			int LastQuestion = 0;
			size_t Raids = 0;
			bool Nodded = false;
			int SpamNods = A.SpamNods;
			size_t Ei = 0;
			std::vector<double> HourCount, HourMembers;
			for (int M = 0; static_cast<double>(M) < End; ++M)
			{
				const double T1 = std::min(End, static_cast<double>(M + 1));
				bool Stop = false;
				// The moments inside this minute, at their stamps.
				while (Ei < Evs.size() && Evs[Ei].At < T1)
				{
					const Ev& E = Evs[Ei++];
					Stop = Stop || E.Kind == kast::Moment::Bubble;
					Fill(E.At, Stop);
					In.Real += (W0 + E.At - In.World) * RealPer[In.Attended ? 0 : Pace];
					In.World = W0 + E.At;
					In.ThisTable = E.Table;
					In.FrontTable = 0;
					if (E.Mtt >= 0)
					{
						In.Entrants = Tracks[static_cast<size_t>(E.Table)][static_cast<size_t>(E.Mtt)].N;
					}
					St.OnMoment(Ch, In, E.Kind, "somebody", 1.0);
					if (E.Kind == kast::Moment::FinalTable || E.Kind == kast::Moment::Win)
					{
						const int N = E.Mtt >= 0 ? Tracks[static_cast<size_t>(E.Table)][static_cast<size_t>(E.Mtt)].N : 0;
						MaxP = growth::Armed(N) && !A.NoDeep ? std::max(MaxP, Tracks[static_cast<size_t>(E.Table)][static_cast<size_t>(E.Mtt)].P) : MaxP;
					}
					In.ThisTable = 0;
				}
				Fill(T1, Stop);
				const double RealNow = RealPer[In.Attended ? 0 : Pace];
				In.Real += (W0 + T1 - In.World) * RealNow;
				In.World = W0 + T1;
				St.AdvanceTo(Ch, In);
				St.ChatStep(Ch, In, 0.02);
				// The streamer's clicks at their allowed rates (§9.2): Thank, answers, one Nod, welcoming raids, the rail call.
				// Clicks only at attended minutes: synth_kast.py's autoplayer holds Engage at watch 4x's 0.12 (its answers there move
				// no Engage), and an answer here is Engage +0.35, so answering at 4x would be a different archetype.
				const bool Hands = In.Attended;
				if ((A.ThankEvery > 0.0 && In.Attended && In.Real >= NextThank) || (A.ThankLever > 0.0 && In.Real >= NextThank))
				{
					St.Thank(Ch, In);
					NextThank = In.Real + (A.ThankLever > 0.0 ? A.ThankLever : A.ThankEvery);
				}
				for (const kast::ChatMsg& Q : St.Chat)
				{
					if (Q.Id > LastQuestion && Q.Kind == kast::LineKind::Question)
					{
						LastQuestion = Q.Id;
						if (Hands && Clicks.Next() < A.Answer * (In.Attended ? 1.0 : 0.4))
						{
							St.Answer(Ch, In, Q.Id);
						}
					}
				}
				if (Rm->Raids.size() > Raids)
				{
					Raids = Rm->Raids.size();
					if (A.Welcome && In.Attended)
					{
						St.WelcomeRaid(Ch);
					}
				}
				if (!Nodded && A.Nods > 0 && In.Attended && M == 30)
				{
					Nodded = true;
					for (const kast::RoomState::Visit& V : Rm->Visits)
					{
						const kast::Member& Mb = Ch.Members[static_cast<size_t>(V.Member)];
						if (!Mb.Friend && Mb.Loyalty >= 0.2 && Mb.Loyalty <= 0.35 && kast::RoomState::HereAt(V, Rm->Clock))
						{
							St.Nod(Ch, Mb.Name);
							break;
						}
					}
				}
				if (SpamNods > 0 && In.Attended && M % 20 == 5)
				{
					for (const std::string& Who : Rm->PresentNames())
					{
						if (!Ch.FindMember(Who))
						{
							--SpamNods;
							St.Nod(Ch, Who); // the spam lever's Nods go to guests (§7)
							break;
						}
					}
				}
				const kast::PlayState* Pl = kast::RoomState::PlayOf(St);
				if (A.RailCall && Pl && Pl->RailOpen && !Pl->RailCalled && In.Attended)
				{
					St.RailCall(Ch, In);
				}
				Rec.ZeroMinutes += Rm->Watchers == 0 ? 1 : 0;
				++Rec.Minutes;
				Rec.Parts[0] += Rm->MembersHere;
				Rec.Parts[1] += Rm->GuestsHere + Rm->CrowdHere;
				Rec.Parts[2] += Rm->RaidersHere;
				Rec.Parts[3] += Rm->LurkersNow;
				Rec.Parts[4] += St.Hype;
				Rec.Parts[5] += St.Engage;
				Rec.Parts[6] += St.Health;
				if (A.Trace)
				{
					const size_t Hr = static_cast<size_t>(M / 60);
					if (HourCount.size() <= Hr)
					{
						HourCount.resize(Hr + 1, 0.0);
						HourMembers.resize(Hr + 1, 0.0);
					}
					HourCount[Hr] += Rm->Watchers / 60.0;
					HourMembers[Hr] += Rm->MembersHere / 60.0;
				}
				St.Notices.clear();
				St.Deals.clear();
				if (St.Chat.size() > 140)
				{
					St.Chat.erase(St.Chat.begin(), St.Chat.begin() + static_cast<std::ptrdiff_t>(St.Chat.size() - 140));
				}
			}
			Out.RealSeconds += End * RealPer[Pace] + 180.0;
			std::string To;
			if (A.RaidOut && End >= 60.0)
			{
				const int Today = static_cast<int>(std::floor(In.World / CareerDay));
				for (const kast::SmallChannel& C : kast::Network(In.World))
				{
					const auto It = Ch.RaidedDay.find(C.Name);
					if (To.empty() && (It == Ch.RaidedDay.end() || Today - It->second >= 7))
					{
						To = C.Name;
					}
				}
			}
			const kast::Summary S = St.Stop(Ch, In, To);
			Book(S);
			PayOut(In.World, RentsPaid);
			if (S.ClipItOpen)
			{
				St.ClipIt(Ch, In); // the end card's Clip of the Night (synth_kast.py takes it whenever it's offered)
			}
			Rec.Avg = S.Avg;
			Rec.Peak = S.Peak;
			Rec.Follows = S.Follows;
			Rec.HeatEnd = growth::HeatShown(Ch.Heat, Ch.HeatAt, In.World, Ch.LastStreamDay);
			Rec.MaxP = MaxP;
			Rec.Deep = DeepNight;
			Rec.Ping[3] = static_cast<double>(Rm->Visits.size());
			Rec.Ping[5] = std::log((static_cast<double>(S.Avg) + 1.0) / (Rec.Ping[4] + 1.0));
			Rec.Ping[6] = S.GainSoFar;
			Rec.Ping[7] = static_cast<double>(Ch.Regulars());
			for (const kast::Member& Mb : Ch.Members)
			{
				Rec.Ping[8] += Mb.Loyalty / static_cast<double>(std::max<size_t>(1, Ch.Members.size()));
			}
			for (size_t Hr = 0; Hr < HourCount.size(); ++Hr)
			{
				Rec.ByHour.push_back({HourCount[Hr], HourMembers[Hr]});
			}
			Out.Streams.push_back(Rec);
			if (Ch.Affiliate && Out.AffiliateDay == 999)
			{
				Out.AffiliateDay = Day;
			}
			// Two moderators once regulars qualify (the Mods tab's candidates): every archetype keeps its chat moderated, as the
			// model's constant Health has it (the levers stay paired).
			if (Ch.Mods.size() < 2)
			{
				const std::vector<std::string> Cands = St.ModCandidates(Ch, In.World);
				if (!Cands.empty())
				{
					St.Promote(Ch, In, Cands.front());
				}
			}
			if (Ch.Partner && Out.PartnerDay == 999)
			{
				Out.PartnerDay = Day;
			}
			// The restart lever: 2-minute raid-out streams after the night (no new Ping inside 4 hours).
			for (int J = 0; J < A.Micro; ++J)
			{
				kast::Inputs Mi = In;
				Mi.World = W0 + End + 30.0 + 10.0 * J;
				Mi.Real += 600.0;
				Mi.Tables = 1;
				Mi.DeepTable = -1;
				Mi.Depth = 0.0;
				Mi.Prestige = 0.0;
				Mi.DeepEntrants = 0;
				Mi.Attended = true;
				Mi.WatchSpeed = 1;
				Mi.Sprinting = false;
				St.Start(Ch, Mi);
				Mi.World += 2.0;
				Mi.Real += 26.2;
				St.AdvanceTo(Ch, Mi);
				const std::vector<kast::SmallChannel> Net = kast::Network(Mi.World);
				Book(St.Stop(Ch, Mi, Net.empty() ? std::string() : Net.front().Name));
				PayOut(Mi.World, RentsPaid);
			}
		}
		if (Day == 7 || Day == 30 || Day == 60 || Day == 90 || Day == 150 || Day == 180 || Day == 335 || Day == 365)
		{
			Snapshot(Day);
		}
	}
	{
		double Sum = 0.0;
		for (size_t K = 0; K < 11; ++K)
		{
			Sum += Out.FollowSrc[K];
		}
		Out.FollowSrc[11] = static_cast<double>(Ch.Followers) - 2.0 - Sum; // Dee and Mei aside
	}
	// Act I: the worst 30-day window of Kast payouts ending before the second rent (days 7-27).
	for (int D = 7; D < 28; ++D)
	{
		double Sum = 0.0;
		for (const auto& P : Out.Paid)
		{
			Sum += P.first > static_cast<double>(D) * CareerDay - 30.0 * CareerDay && P.first <= static_cast<double>(D) * CareerDay ? P.second : 0.0;
		}
		Out.Act1Worst = std::max(Out.Act1Worst, Sum);
	}
	return Out;
}

double CareerMedian(std::vector<double> V)
{
	if (V.empty())
	{
		return 0.0;
	}
	std::sort(V.begin(), V.end());
	return V[std::min(V.size() - 1, V.size() / 2)];
}

/** LongMtt (§1.6, §9.1): on non-deep nights, a 5-hour one-table session's hour 4 against hour 1 (everyone and the members),
 *  streams 10-60 and 60-200; a 6-hour session's members at hour 6 against hour 2. */
void LongMtt()
{
	const int Seeds = Long ? 4 : 1;
	for (const char* Name : {"trace5h", "trace6h"})
	{
		const CareerArch A = CareerArchOf(Name);
		std::map<std::string, std::vector<double>> R41, M41, M62;
		for (int Seed = 0; Seed < Seeds; ++Seed)
		{
			const CareerRun Run = CareerRunOf(A, Seed, Long ? 290 : 150);
			for (const CareerStream& S : Run.Streams)
			{
				const std::string Band = S.Stream >= 10 && S.Stream < 60 ? "10-60" : S.Stream >= 60 && S.Stream < 200 ? "60-200" : "";
				if (Band.empty() || S.Deep || S.ByHour.size() < static_cast<size_t>(A.Hours))
				{
					continue;
				}
				if (S.ByHour[0][0] > 0.5)
				{
					R41[Band].push_back(S.ByHour[3][0] / S.ByHour[0][0]);
				}
				if (S.ByHour[0][1] > 0.5)
				{
					M41[Band].push_back(S.ByHour[3][1] / S.ByHour[0][1]);
				}
				if (A.Hours >= 6.0 && S.ByHour[1][1] > 0.5)
				{
					M62[Band].push_back(S.ByHour[5][1] / S.ByHour[1][1]);
				}
			}
		}
		for (const auto& B : R41)
		{
			const double All = CareerMedian(B.second);
			const double Mem = CareerMedian(M41[B.first]);
			std::printf("  long mtt (%s, streams %s, %zu nights): hour 4 / hour 1 x%.2f, members x%.2f", Name, B.first.c_str(), B.second.size(), All, Mem);
			if (A.Hours >= 6.0)
			{
				std::printf("; members hour 6 / hour 2 x%.2f", CareerMedian(M62[B.first]));
			}
			std::printf("\n");
			Expect(All >= 0.6 && Mem >= 0.6, std::string("long mtt: a 5-hour session's hour 4 holds 0.6x hour 1, everyone and members (") + Name + ", " + B.first + ")");
			if (A.Hours >= 6.0)
			{
				Expect(CareerMedian(M62[B.first]) >= 0.8, std::string("long mtt: a 6-hour session's members at hour 6 hold 0.8x hour 2 (") + B.first + ")");
			}
		}
	}
}

// ------------------------------------------------------------------ the bands (§6.4), Momentum and Forecast (§9.1), after S-K

/** A median over the seeds and its bootstrap SE (fixed draws: deterministic for fixed seeds). */
struct CareerStat
{
	double Median = 0.0, Se = 0.0;
};

CareerStat CareerMedianSe(const std::vector<double>& V)
{
	CareerStat S;
	S.Median = CareerMedian(V);
	if (V.size() < 2)
	{
		return S;
	}
	ss::Rng R("kast-careers-bootstrap");
	double Sum = 0.0, Sum2 = 0.0;
	constexpr int Draws = 400;
	for (int K = 0; K < Draws; ++K)
	{
		std::vector<double> B;
		for (size_t I = 0; I < V.size(); ++I)
		{
			B.push_back(V[static_cast<size_t>(R.Int(static_cast<int>(V.size())))]);
		}
		const double M = CareerMedian(B);
		Sum += M;
		Sum2 += M * M;
	}
	const double Mean = Sum / Draws;
	S.Se = std::sqrt(std::max(0.0, Sum2 / Draws - Mean * Mean));
	return S;
}

/** The median inside [Lo, Hi], the band widened by 2 SE of that median (§6.4: each checked with its seed noise). */
bool CareerIn(const CareerStat& S, double Lo, double Hi)
{
	return S.Median + 2.0 * S.Se >= Lo && S.Median - 2.0 * S.Se <= Hi;
}

/** A lever's paired ratio of medians (the lever's careers over the base's, the same seeds in the same order; §9.1's Levers)
 *  and its bootstrap SE (the seeds resampled in pairs; fixed draws). */
CareerStat CareerRatioSe(const std::vector<double>& Lever, const std::vector<double>& Base)
{
	CareerStat S;
	const size_t N = std::min(Lever.size(), Base.size());
	if (N == 0)
	{
		return S;
	}
	auto Ratio = [](const std::vector<double>& L, const std::vector<double>& B) { return CareerMedian(L) / std::max(1.0e-9, CareerMedian(B)); };
	S.Median = Ratio(std::vector<double>(Lever.begin(), Lever.begin() + static_cast<std::ptrdiff_t>(N)),
		std::vector<double>(Base.begin(), Base.begin() + static_cast<std::ptrdiff_t>(N)));
	ss::Rng R("kast-careers-levers-bootstrap");
	double Sum = 0.0, Sum2 = 0.0;
	constexpr int Draws = 400;
	for (int K = 0; K < Draws; ++K)
	{
		std::vector<double> L, B;
		for (size_t I = 0; I < N; ++I)
		{
			const size_t J = static_cast<size_t>(R.Int(static_cast<int>(N)));
			L.push_back(Lever[J]);
			B.push_back(Base[J]);
		}
		const double X = Ratio(L, B);
		Sum += X;
		Sum2 += X * X;
	}
	const double Mean = Sum / Draws;
	S.Se = std::sqrt(std::max(0.0, Sum2 / Draws - Mean * Mean));
	return S;
}

std::vector<double> CareerAt(const std::vector<CareerRun>& Runs, int Day, double CareerSnap::*Field)
{
	std::vector<double> V;
	for (const CareerRun& R : Runs)
	{
		const auto It = R.At.find(Day);
		V.push_back(It != R.At.end() ? It->second.*Field : 0.0);
	}
	return V;
}

std::vector<double> CareerFollowersAt(const std::vector<CareerRun>& Runs, int Day)
{
	std::vector<double> V;
	for (const CareerRun& R : Runs)
	{
		const auto It = R.At.find(Day);
		V.push_back(It != R.At.end() ? static_cast<double>(It->second.Followers) : 0.0);
	}
	return V;
}

/** A 30-day window's Kast money per real hour live (and in the lobby) to Day. */
std::vector<double> CareerPerHour(const std::vector<CareerRun>& Runs, int Day, int From)
{
	std::vector<double> V;
	for (const CareerRun& R : Runs)
	{
		const auto A = R.At.find(Day);
		const auto B = R.At.find(From);
		const double H = A != R.At.end() && B != R.At.end() ? A->second.RealH - B->second.RealH : 0.0;
		V.push_back(H > 0.0 ? A->second.Money30 / H : 0.0);
	}
	return V;
}

constexpr double CareerRent = 1075.0;

/** What a career's streams say about momentum, Heat's tiers and the forecast (synth_kast.py's momentum_stats, heat_tiers,
 *  ft_nights and forecast_honesty, from day 30 where it reads so). */
struct CareerFeel
{
	double After = 0.0, Other = 0.0;           // the next 3 streams against the 3 before: after a final table or win (or a night
	                                           // gain of 12+), and after other nights (medians)
	double FtPeak[3] = {0.0, 0.0, 0.0};        // a final-table night's peak against the 10 streams before: field P <= 1, 1-2.5, a major
	int FtNights[3] = {0, 0, 0};
	double HotNightsMonth = 0.0;               // go-lives at Running hot or above, a 30-day month (day 30 on)
	double HeaterShare = 0.0;                  // go-lives at On a heater or above
	double HeatP50 = 0.0, HeatP90 = 0.0;
	double FtEndHeater = 0.0;                  // final-table nights that end On a heater or Sun run
	int ColdAfterStreaming = 0;                // go-lives that read Cold with a stream in the 7 days before
	double FcInside = 0.0;                     // streams whose average lands inside the forecast's band (day 30 on)
};

CareerFeel CareerFeelOf(const std::vector<CareerRun>& Runs)
{
	CareerFeel F;
	std::vector<double> After, Other, Heat, Pk[3];
	int Hot = 0, Heater = 0, Nights = 0, Ft = 0, FtHeater = 0, Inside = 0, Fc = 0;
	double Days = 0.0;
	for (const CareerRun& R : Runs)
	{
		const std::vector<CareerStream>& S = R.Streams;
		for (size_t I = 3; I + 3 < S.size(); ++I)
		{
			const double Pre = (S[I - 3].Avg + S[I - 2].Avg + S[I - 1].Avg) / 3.0 + 0.5;
			const double Post = (S[I + 1].Avg + S[I + 2].Avg + S[I + 3].Avg) / 3.0 + 0.5;
			(S[I].MaxP > 0.0 || S[I].Ping[6] >= 12.0 ? After : Other).push_back(Post / Pre);
		}
		for (size_t I = 10; I + 3 < S.size(); ++I)
		{
			if (S[I].MaxP <= 0.0)
			{
				continue;
			}
			double Base = 1.0;
			for (size_t J = I - 10; J < I; ++J)
			{
				Base += S[J].Avg / 10.0;
			}
			const int Bin = S[I].MaxP <= 1.0 ? 0 : S[I].MaxP <= 2.5 ? 1 : 2;
			Pk[Bin].push_back((S[I].Peak + 1.0) / Base);
		}
		int Last = -100;
		for (const CareerStream& X : S)
		{
			if (X.Day >= 30)
			{
				++Nights;
				Heat.push_back(X.HeatStart);
				Hot += X.HeatStart >= 35.0 ? 1 : 0;
				Heater += X.HeatStart >= 55.0 ? 1 : 0;
				if (X.MaxP > 0.0)
				{
					++Ft;
					FtHeater += X.HeatEnd >= 55.0 ? 1 : 0;
				}
				++Fc;
				Inside += X.Avg >= X.FcLo && X.Avg <= X.FcHi ? 1 : 0;
			}
			F.ColdAfterStreaming += X.Stream > 1 && X.Day - Last <= 7 && X.HeatStart < 20.0 - 1.0e-9 ? 1 : 0;
			Last = X.Day;
		}
		Days += S.empty() ? 0.0 : std::max(1.0, static_cast<double>(S.back().Day - 30));
	}
	F.After = CareerMedian(After);
	F.Other = CareerMedian(Other);
	for (int B = 0; B < 3; ++B)
	{
		F.FtPeak[B] = CareerMedian(Pk[B]);
		F.FtNights[B] = static_cast<int>(Pk[B].size());
	}
	F.HotNightsMonth = Days > 0.0 ? 30.0 * static_cast<double>(Hot) / Days : 0.0;
	F.HeaterShare = Nights > 0 ? static_cast<double>(Heater) / Nights : 0.0;
	std::sort(Heat.begin(), Heat.end());
	F.HeatP50 = Heat.empty() ? 0.0 : Heat[Heat.size() / 2];
	F.HeatP90 = Heat.empty() ? 0.0 : Heat[std::min(Heat.size() - 1, static_cast<size_t>(0.9 * static_cast<double>(Heat.size())))];
	F.FtEndHeater = Ft > 0 ? static_cast<double>(FtHeater) / Ft : 0.0;
	F.FcInside = Fc > 0 ? static_cast<double>(Inside) / Fc : 0.0;
	return F;
}

std::vector<CareerRun> CareerRuns(const std::string& Arch, int Seed0, int Seeds, int Days)
{
	std::vector<CareerRun> Out;
	const CareerArch A = CareerArchOf(Arch);
	for (int Seed = Seed0; Seed < Seed0 + Seeds; ++Seed)
	{
		Out.push_back(CareerRunOf(A, Seed, Days));
	}
	return Out;
}

/** An archetype's careers on Chunks threads (a career touches no shared mutable state: the core's tables are const statics,
 *  every draw keyed or on the career's own Rng; Expect runs on the calling thread only). Seeds in order. */
std::vector<std::future<std::vector<CareerRun>>> CareerLaunch(const std::string& Arch, int Seeds, int Chunks)
{
	std::vector<std::future<std::vector<CareerRun>>> Out;
	const int Per = (Seeds + Chunks - 1) / Chunks;
	for (int S0 = 0; S0 < Seeds; S0 += Per)
	{
		Out.push_back(std::async(std::launch::async, [Arch, S0, Per, Seeds]() { return CareerRuns(Arch, S0, std::min(Per, Seeds - S0), 366); }));
	}
	return Out;
}

std::vector<CareerRun> CareerJoin(std::vector<std::future<std::vector<CareerRun>>>& Parts)
{
	std::vector<CareerRun> Out;
	for (std::future<std::vector<CareerRun>>& P : Parts)
	{
		std::vector<CareerRun> Some = P.get();
		Out.insert(Out.end(), Some.begin(), Some.end());
	}
	return Out;
}

/**
 * The bands (§6.4; frozen after S-K, 2026-10-25, E:/ShortStack/durable/creator/S-K.md), Forecast and Momentum (§9.1). The
 * default run: the regular and the casual at 2 seeds; `kast_growth long`: every archetype at 12 seeds, each median checked
 * with its band widened by 2 x that median's bootstrap SE.
 */
void Careers()
{
	const int Seeds = Long ? 12 : 2;
	const auto T0 = std::chrono::steady_clock::now();
	// Every archetype at once (long: the grinders on three threads each, the others on two).
	auto RegularParts = CareerLaunch("regular", Seeds, 2);
	auto CasualParts = CareerLaunch("casual", Seeds, 1);
	std::vector<std::future<std::vector<CareerRun>>> GrinderParts, StrongParts, DeepParts, AutoParts;
	// §9.1's Levers (the C-K gate, 2026-10-25): each lever is its base archetype with one change, on the base's seeds.
	static const char* const LeverNames[] = {"lever_spam", "lever_silent", "lever_answer_all", "lever_teaching", "lever_noraid", "lever_micro", "casual_micro",
		"grinder_micro", "lever_hourly", "lever_4table"};
	std::map<std::string, std::vector<std::future<std::vector<CareerRun>>>> LeverParts;
	if (Long)
	{
		GrinderParts = CareerLaunch("grinder", Seeds, 3);
		StrongParts = CareerLaunch("grinder_strong", Seeds, 3);
		DeepParts = CareerLaunch("deeprun", Seeds, 2);
		AutoParts = CareerLaunch("autoplay", Seeds, 2);
		for (const char* L : LeverNames)
		{
			const std::string N = L;
			LeverParts[N] = CareerLaunch(N, Seeds, N == "grinder_micro" || N == "lever_hourly" || N == "lever_4table" ? 3 : N == "casual_micro" ? 1 : 2);
		}
	}
	const std::vector<CareerRun> Regular = CareerJoin(RegularParts);
	const std::vector<CareerRun> Casual = CareerJoin(CasualParts);
	auto Print = [](const char* Name, const std::vector<CareerRun>& Runs) {
		std::printf("  careers (%s, %zu seeds):", Name, Runs.size());
		for (const int D : {30, 90, 180, 365})
		{
			std::printf(" d%d %.0f followers, avg %.1f, $%.0f;", D, CareerMedian(CareerFollowersAt(Runs, D)), CareerMedian(CareerAt(Runs, D, &CareerSnap::Avg30)),
				CareerMedian(CareerAt(Runs, D, &CareerSnap::Money30)));
		}
		std::vector<double> Aff, Par, Act;
		for (const CareerRun& R : Runs)
		{
			Aff.push_back(R.AffiliateDay);
			Par.push_back(R.PartnerDay);
			Act.push_back(R.Act1Worst);
		}
		std::printf(" Affiliate day %.0f, Partner day %.0f, Act I worst $%.0f\n", CareerMedian(Aff), CareerMedian(Par), *std::max_element(Act.begin(), Act.end()));
	};
	Print("regular", Regular);
	Print("casual", Casual);
	// The regular (§6.4's first row).
	{
		std::vector<double> Aff, Par;
		for (const CareerRun& R : Regular)
		{
			Aff.push_back(R.AffiliateDay);
			Par.push_back(R.PartnerDay);
		}
		Expect(CareerIn(CareerMedianSe(CareerFollowersAt(Regular, 30)), 80, 200) && CareerIn(CareerMedianSe(CareerAt(Regular, 30, &CareerSnap::Avg30)), 8, 20) &&
				   CareerIn(CareerMedianSe(Aff), 0, 20),
			"careers: the regular at d30: followers 80-200, average 8-20, Affiliate by day 20");
		Expect(CareerIn(CareerMedianSe(CareerFollowersAt(Regular, 90)), 330, 800) && CareerIn(CareerMedianSe(CareerAt(Regular, 90, &CareerSnap::Avg30)), 40, 95) &&
				   CareerIn(CareerMedianSe(CareerAt(Regular, 90, &CareerSnap::Money30)), 0, 0.5 * CareerRent),
			"careers: the regular at d90: followers 330-800, average 40-95, money <= 0.5 rent");
		Expect(CareerIn(CareerMedianSe(CareerFollowersAt(Regular, 180)), 1100, 2600) && CareerIn(CareerMedianSe(CareerAt(Regular, 180, &CareerSnap::Avg30)), 140, 330) &&
				   CareerIn(CareerMedianSe(Par), 60, 150),
			"careers: the regular at d180: followers 1,100-2,600, average 140-330, Partner days 60-150");
		Expect(CareerIn(CareerMedianSe(CareerFollowersAt(Regular, 365)), 2700, 6500) && CareerIn(CareerMedianSe(CareerAt(Regular, 365, &CareerSnap::Avg30)), 200, 420) &&
				   CareerIn(CareerMedianSe(CareerAt(Regular, 365, &CareerSnap::Money30)), 1.0 * CareerRent, 2.0 * CareerRent),
			"careers: the regular at d365: followers 2,700-6,500, average 200-420, money 1.0-2.0 rents");
		// §6.4's first row (C-K's L7): the regular's empty room over its first 20 streams, at most 3% of their world minutes (the
		// casual's 10% is below). Each seed's share; the median with its noise, as every band here.
		std::vector<double> Empty;
		for (const CareerRun& R : Regular)
		{
			int Zero = 0, Mins = 0;
			for (size_t I = 0; I < R.Streams.size() && I < 20; ++I)
			{
				Zero += R.Streams[I].ZeroMinutes;
				Mins += R.Streams[I].Minutes;
			}
			Empty.push_back(Mins > 0 ? static_cast<double>(Zero) / Mins : 0.0);
		}
		std::printf("  careers: the regular's empty room over its first 20 streams: median %.2f%% of world minutes (worst seed %.2f%%)\n", 100.0 * CareerMedian(Empty),
			100.0 * *std::max_element(Empty.begin(), Empty.end()));
		Expect(CareerIn(CareerMedianSe(Empty), 0, 0.03), "careers: the regular's empty room <= 3% of its first 20 streams");
	}
	// The casual.
	{
		std::vector<double> Empty;
		for (const CareerRun& R : Casual)
		{
			int Zero = 0, Mins = 0;
			for (size_t I = 0; I < R.Streams.size() && I < 20; ++I)
			{
				Zero += R.Streams[I].ZeroMinutes;
				Mins += R.Streams[I].Minutes;
			}
			Empty.push_back(Mins > 0 ? static_cast<double>(Zero) / Mins : 0.0);
		}
		Expect(CareerIn(CareerMedianSe(CareerFollowersAt(Casual, 365)), 110, 300) && CareerIn(CareerMedianSe(CareerAt(Casual, 365, &CareerSnap::Avg30)), 2.5, 7) &&
				   CareerIn(CareerMedianSe(CareerAt(Casual, 365, &CareerSnap::Money30)), 0, 0.1 * CareerRent) && CareerIn(CareerMedianSe(Empty), 0, 0.10),
			"careers: the casual at d365: followers 110-300, average 2.5-7, money <= 0.1 rent; the empty room <= 10% of its first 20 streams");
	}
	// Act I, every archetype run here: no 30-day window of Kast payouts at 30% of a rent before the second rent.
	auto ActI = [](const std::vector<CareerRun>& Runs) {
		double Worst = 0.0;
		for (const CareerRun& R : Runs)
		{
			Worst = std::max(Worst, R.Act1Worst);
		}
		return Worst;
	};
	Expect(ActI(Regular) < 0.3 * CareerRent && ActI(Casual) < 0.3 * CareerRent, "careers: Act I: no 30-day window of Kast payouts at 30% of a rent before the second rent (regular, casual)");
	// Forecast (§9.1): the regular's average lands inside the band on 70-90% of streams (model 83%).
	const CareerFeel RegularFeel = CareerFeelOf(Regular);
	std::printf("  forecast: the regular's average inside the band on %.0f%% of streams from day 30\n", 100.0 * RegularFeel.FcInside);
	Expect(RegularFeel.FcInside >= 0.70 && RegularFeel.FcInside <= 0.90, "forecast: the regular's average lands inside the band on 70-90% of streams");
	if (!Long)
	{
		std::printf("  careers: %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
		return;
	}
	// kast_growth long: the rest of §6.4 and Momentum.
	const std::vector<CareerRun> Grinder = CareerJoin(GrinderParts);
	const std::vector<CareerRun> Strong = CareerJoin(StrongParts);
	const std::vector<CareerRun> Deep = CareerJoin(DeepParts);
	const std::vector<CareerRun> Auto = CareerJoin(AutoParts);
	Print("grinder", Grinder);
	Print("grinder_strong", Strong);
	Print("deeprun", Deep);
	Print("autoplay", Auto);
	Expect(CareerIn(CareerMedianSe(CareerAt(Grinder, 90, &CareerSnap::Money30)), 0, 1.5 * CareerRent) &&
			   CareerIn(CareerMedianSe(CareerAt(Grinder, 180, &CareerSnap::Money30)), 0, 3.0 * CareerRent) &&
			   CareerIn(CareerMedianSe(CareerAt(Grinder, 365, &CareerSnap::Money30)), 0, 4.5 * CareerRent),
		"careers: the grinder's money <= 1.5 rents at d90, 3.0 at d180, 4.5 at d365");
	Expect(CareerMedian(CareerPerHour(Grinder, 180, 150)) < CareerMedian(CareerPerHour(Regular, 180, 150)) &&
			   CareerMedian(CareerPerHour(Grinder, 365, 335)) < CareerMedian(CareerPerHour(Regular, 365, 335)),
		"careers: the grinder's money per real hour below the regular's at d180 and d365");
	Expect(CareerIn(CareerMedianSe(CareerAt(Strong, 90, &CareerSnap::Money30)), 0, 2.5 * CareerRent) &&
			   CareerIn(CareerMedianSe(CareerAt(Strong, 180, &CareerSnap::Money30)), 0, 3.2 * CareerRent) &&
			   CareerIn(CareerMedianSe(CareerAt(Strong, 365, &CareerSnap::Money30)), 0, 4.5 * CareerRent),
		"careers: the strong grinder's money <= 2.5 rents at d90, 3.2 at d180, 4.5 at d365");
	{
		const double RegM = CareerMedian(CareerAt(Regular, 365, &CareerSnap::Money30));
		const double DeepM = CareerMedian(CareerAt(Deep, 365, &CareerSnap::Money30));
		Expect(CareerIn(CareerMedianSe(CareerAt(Deep, 90, &CareerSnap::Money30)), 0, 1.5 * CareerRent) && DeepM >= 0.9 * RegM && DeepM <= 1.4 * RegM,
			"careers: the deep-run streamer's money <= 1.5 rents at d90, 0.9-1.4x the regular's at d365");
		// Autoplay-heavy against the regular, per world day (the same days): followers 0.5-0.8x, money 0.5-0.85x.
		const double F = CareerMedian(CareerFollowersAt(Auto, 365)) / std::max(1.0, CareerMedian(CareerFollowersAt(Regular, 365)));
		const double M = CareerMedian(CareerAt(Auto, 365, &CareerSnap::Money30)) / std::max(1.0, RegM);
		std::printf("  careers: autoplay-heavy against the regular at d365: followers x%.2f, money x%.2f; the deep-run streamer's money x%.2f\n", F, M, DeepM / std::max(1.0, RegM));
		Expect(F >= 0.5 && F <= 0.8 && M >= 0.5 && M <= 0.85, "careers: autoplay-heavy at d365: followers 0.5-0.8x and money 0.5-0.85x the regular's");
	}
	Expect(ActI(Grinder) < 0.3 * CareerRent && ActI(Strong) < 0.3 * CareerRent && ActI(Deep) < 0.3 * CareerRent && ActI(Auto) < 0.3 * CareerRent,
		"careers: Act I: no 30-day window at 30% of a rent before the second rent (grinders, deep-run, autoplay-heavy)");
	// Momentum (§9.1).
	const CareerFeel GrinderFeel = CareerFeelOf(Grinder);
	const CareerFeel StrongFeel = CareerFeelOf(Strong);
	const CareerFeel DeepFeel = CareerFeelOf(Deep);
	std::printf("  momentum: after a big night x%.2f against x%.2f (regular), x%.2f against x%.2f (deep-run); final-table peaks x%.2f the 10-stream average (regular, %d "
				"nights), x%.2f at 1,000+ fields and x%.2f at majors (deep-run, %d and %d nights)\n",
		RegularFeel.After, RegularFeel.Other, DeepFeel.After, DeepFeel.Other, RegularFeel.FtPeak[0], RegularFeel.FtNights[0], DeepFeel.FtPeak[1], DeepFeel.FtPeak[2], DeepFeel.FtNights[1],
		DeepFeel.FtNights[2]);
	std::printf("  momentum: Heat: the regular Running hot+ %.1f nights a month (p50 %.0f); On a heater+: grinder %.0f%%, strong grinder %.0f%%, deep-run %.0f%% (p90 %.0f), "
				"final tables ending On a heater+ %.0f%%; Cold go-lives after streaming: %d\n",
		RegularFeel.HotNightsMonth, RegularFeel.HeatP50, 100.0 * GrinderFeel.HeaterShare, 100.0 * StrongFeel.HeaterShare, 100.0 * DeepFeel.HeaterShare, DeepFeel.HeatP90,
		100.0 * DeepFeel.FtEndHeater, RegularFeel.ColdAfterStreaming + GrinderFeel.ColdAfterStreaming + StrongFeel.ColdAfterStreaming + DeepFeel.ColdAfterStreaming);
	Expect(RegularFeel.After >= 1.08 * RegularFeel.Other && DeepFeel.After >= 1.08 * DeepFeel.Other, "momentum: after a final table or a win the next 3 streams run >= x1.08 of other nights'");
	Expect(RegularFeel.FtPeak[0] >= 1.9 && DeepFeel.FtPeak[1] >= 2.2 && (DeepFeel.FtNights[2] == 0 || DeepFeel.FtPeak[2] >= 3.0),
		"momentum: final-table peaks: the regular's >= x1.9 its 10-stream average, the deep-run streamer's 1,000+ field >= x2.2 and a major >= x3");
	Expect(RegularFeel.HeatP50 >= 20.0 && RegularFeel.HeatP50 < 35.0 && RegularFeel.HotNightsMonth >= 3.0 && RegularFeel.HotNightsMonth <= 10.0,
		"momentum: the regular's Heat p50 in Warming up, Running hot or above 3-10 nights a month");
	Expect(GrinderFeel.HeaterShare <= 0.25 && StrongFeel.HeaterShare <= 0.25, "momentum: the grinders On a heater or above on <= 25% of nights");
	Expect(DeepFeel.HeatP90 >= 45.0 && DeepFeel.HeaterShare >= 0.03 && DeepFeel.HeaterShare <= 0.20 && DeepFeel.FtEndHeater >= 0.5,
		"momentum: the deep-run streamer's Heat p90 >= 45, On a heater 3-20% of nights, a final-table night ending On a heater or Sun run >= 50%");
	Expect(RegularFeel.ColdAfterStreaming + GrinderFeel.ColdAfterStreaming + StrongFeel.ColdAfterStreaming + DeepFeel.ColdAfterStreaming == 0,
		"momentum: no go-live reads Cold while the channel streamed in the last 7 days");
	// Every archetype's ceilings (§6.4's last row): at most 1.5 rents at d90 and 3.0 at d180 (the strong grinder's own row above).
	{
		bool Under = true;
		std::string Over;
		for (const std::pair<const char*, const std::vector<CareerRun>*>& A :
			std::vector<std::pair<const char*, const std::vector<CareerRun>*>>{{"regular", &Regular}, {"casual", &Casual}, {"deep-run", &Deep}, {"autoplay-heavy", &Auto}})
		{
			const bool Ok = CareerIn(CareerMedianSe(CareerAt(*A.second, 90, &CareerSnap::Money30)), 0, 1.5 * CareerRent) &&
				CareerIn(CareerMedianSe(CareerAt(*A.second, 180, &CareerSnap::Money30)), 0, 3.0 * CareerRent);
			Under = Under && Ok;
			Over += Ok ? "" : std::string(" ") + A.first;
		}
		Expect(Under, "careers: every archetype's money <= 1.5 rents at d90 and 3.0 at d180 (regular, casual, deep-run, autoplay-heavy)" + Over);
	}
	// Levers (§9.1; S-K.md §4 measured them at 24 seeds): paired ratios of medians on the base's seeds, each band widened by 2 SE of
	// the ratio. Followers and money at d90 and d365 (the casual's money is never compared: pennies).
	{
		std::map<std::string, std::vector<CareerRun>> Lv;
		for (const char* L : LeverNames)
		{
			Lv[L] = CareerJoin(LeverParts[L]);
		}
		struct Pair
		{
			CareerStat F90, F365, M90, M365;
		};
		auto Paired = [](const std::vector<CareerRun>& L, const std::vector<CareerRun>& B) {
			Pair P;
			P.F90 = CareerRatioSe(CareerFollowersAt(L, 90), CareerFollowersAt(B, 90));
			P.F365 = CareerRatioSe(CareerFollowersAt(L, 365), CareerFollowersAt(B, 365));
			P.M90 = CareerRatioSe(CareerAt(L, 90, &CareerSnap::Money30), CareerAt(B, 90, &CareerSnap::Money30));
			P.M365 = CareerRatioSe(CareerAt(L, 365, &CareerSnap::Money30), CareerAt(B, 365, &CareerSnap::Money30));
			std::printf(" followers x%.2f (SE %.2f) / x%.2f (%.2f), money x%.2f (%.2f) / x%.2f (%.2f)\n", P.F90.Median, P.F90.Se, P.F365.Median, P.F365.Se, P.M90.Median, P.M90.Se,
				P.M365.Median, P.M365.Se);
			return P;
		};
		auto Show = [](const char* What) { std::printf("  levers (%s):", What); };
		Show("spam: Thank every 20 real s + 3 Nods on guests, against the regular");
		const Pair Spam = Paired(Lv["lever_spam"], Regular);
		Show("silent, against the regular");
		const Pair Silent = Paired(Lv["lever_silent"], Regular);
		Show("answer every question on the Teaching title, against the regular (information: the title's own pull is in it)");
		(void)Paired(Lv["lever_answer_all"], Regular);
		Show("the Teaching title alone, against the regular (information)");
		(void)Paired(Lv["lever_teaching"], Regular);
		Show("answer every question on the Teaching title, against the regular on the Teaching title");
		const Pair Answer = Paired(Lv["lever_answer_all"], Lv["lever_teaching"]);
		Show("never raid out, against the regular");
		const Pair NoRaid = Paired(Lv["lever_noraid"], Regular);
		Show("three 2-minute restarts after each stream, against the regular");
		const Pair Micro = Paired(Lv["lever_micro"], Regular);
		Show("the casual's restarts, against the casual");
		const Pair CasualMicro = Paired(Lv["casual_micro"], Casual);
		Show("the grinder's restarts, against the grinder");
		const Pair GrinderMicro = Paired(Lv["grinder_micro"], Grinder);
		Show("four tables of 45-60-runner hourlies, against four ordinary tables");
		const Pair Hourly = Paired(Lv["lever_hourly"], Lv["lever_4table"]);
		auto Le = [](const CareerStat& S, double Hi) { return CareerIn(S, 0.0, Hi); };
		auto Within = [](const CareerStat& S, double Lo, double Hi) { return CareerIn(S, Lo, Hi); };
		Expect(Le(Spam.F90, 1.15) && Le(Spam.F365, 1.15) && Le(Spam.M90, 1.15) && Le(Spam.M365, 1.15), "levers: Thank spam and Nods <= x1.15 followers and money");
		Expect(Within(Silent.F90, 0.7, 0.95) && Within(Silent.F365, 0.7, 0.95), "levers: never talking to chat 0.7-0.95 followers");
		Expect(Le(Answer.F90, 1.15) && Le(Answer.F365, 1.15), "levers: answering every question on the Teaching title <= x1.15 followers (against the same title)");
		Expect(Within(NoRaid.F90, 0.85, 1.15) && Within(NoRaid.F365, 0.85, 1.15) && Within(NoRaid.M90, 0.85, 1.15) && Within(NoRaid.M365, 0.85, 1.15),
			"levers: never raiding out 0.85-1.15 followers and money");
		Expect(Le(Micro.F90, 1.10) && Le(Micro.F365, 1.10) && Le(Micro.M90, 1.10) && Le(Micro.M365, 1.10) && Le(CasualMicro.F90, 1.10) && Le(CasualMicro.F365, 1.10) &&
				   Le(GrinderMicro.F90, 1.10) && Le(GrinderMicro.F365, 1.10) && Le(GrinderMicro.M90, 1.10) && Le(GrinderMicro.M365, 1.10),
			"levers: short-stream restarts <= x1.10 (regular, casual, grinder)");
		Expect(Le(Hourly.F90, 1.0) && Le(Hourly.F365, 1.0) && Le(Hourly.M90, 1.0) && Le(Hourly.M365, 1.0), "levers: four tables of hourlies <= x1.0 four ordinary tables");
	}
	std::printf("  careers (long): %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

/** The event-driven Room over careers (C-K's M2): the regular's first 45 world days (long: the grinder's and the deep-run
 *  streamer's 75 too) with RoomState::EventDriven on and off give the same snapshots and stream records to the bit. Run alone, after
 *  the threaded careers (the switch is read at each go-live). S-K's paired re-run of every archetype over 366 days is the gate's. */
void CareersEventDriven()
{
	auto Digest = [](const CareerRun& R) {
		std::string Out;
		char Buf[256];
		for (const auto& At : R.At)
		{
			const CareerSnap& S = At.second;
			std::snprintf(Buf, sizeof(Buf), "%d %d %d %d %d %.17g %.17g %.17g %.17g|", At.first, S.Followers, S.Subs, S.Regulars, S.Members, S.Avg30, S.Money30, S.Heat, S.RealH);
			Out += Buf;
		}
		for (const CareerStream& S : R.Streams)
		{
			std::snprintf(Buf, sizeof(Buf), "%d %d %d %d %.17g %.17g %.17g %.17g %d %d|", S.Day, S.Avg, S.Peak, S.Follows, S.HeatStart, S.HeatEnd, S.FcLo, S.FcHi, S.ZeroMinutes, S.Minutes);
			Out += Buf;
		}
		for (const std::pair<double, double>& P : R.Paid)
		{
			std::snprintf(Buf, sizeof(Buf), "%.17g %.17g|", P.first, P.second);
			Out += Buf;
		}
		return Out;
	};
	std::vector<std::pair<std::string, int>> Runs = {{"regular", 45}};
	if (Long)
	{
		Runs.push_back({"grinder", 75});
		Runs.push_back({"deeprun", 75});
	}
	bool Same = true;
	std::string Which;
	for (const std::pair<std::string, int>& Run : Runs)
	{
		kast::RoomState::EventDriven = false;
		const CareerRun Eager = CareerRunOf(CareerArchOf(Run.first), 3, Run.second);
		kast::RoomState::EventDriven = true;
		const CareerRun Event = CareerRunOf(CareerArchOf(Run.first), 3, Run.second);
		const bool Ok = Digest(Eager) == Digest(Event);
		Same = Same && Ok;
		Which += " " + Run.first + (Ok ? " identical" : " DIFFERS");
		std::printf("  careers, event-driven against eager: the %s's first %d days (%zu streams) %s\n", Run.first.c_str(), Run.second, Event.Streams.size(), Ok ? "identical" : "DIFFER");
	}
	Expect(Same, "careers: the event-driven Room's careers are the eager walks' to the bit:" + Which);
}

/** The careers' table (S-K and the bands): each seed's snapshots, for the medians and their bootstrap noise. */
void CareersTable(const std::string& Arch, int Seed0, int Seeds, int Days, FILE* To)
{
	const CareerArch A = CareerArchOf(Arch);
	for (int Seed = Seed0; Seed < Seed0 + Seeds; ++Seed)
	{
		const auto T0 = std::chrono::steady_clock::now();
		const CareerRun Run = CareerRunOf(A, Seed, Days);
		const double Secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
		std::fprintf(To, "%s\t%d", Arch.c_str(), Seed);
		for (const int D : {7, 30, 90, 180, 365})
		{
			const auto It = Run.At.find(D);
			const CareerSnap S = It != Run.At.end() ? It->second : CareerSnap();
			std::fprintf(To, "\t%d\t%.3f\t%d\t%.2f\t%d\t%.2f\t%.2f", S.Followers, S.Avg30, S.Subs, S.Money30, S.Regulars, S.Heat, S.RealH);
		}
		int Zero = 0, Mins = 0;
		for (size_t I = 0; I < Run.Streams.size() && I < 20; ++I)
		{
			Zero += Run.Streams[I].ZeroMinutes;
			Mins += Run.Streams[I].Minutes;
		}
		std::fprintf(To, "\t%d\t%d\t%.2f\t%.4f\t%.1f", Run.AffiliateDay, Run.PartnerDay, Run.Act1Worst, Mins > 0 ? static_cast<double>(Zero) / Mins : 0.0, Secs);
		// The money by source in the 30 days to d90 and d365 (tips, cheers, live subs, renewals, ads), then the follows by source.
		for (const std::pair<int, int> W : {std::make_pair(60, 90), std::make_pair(335, 365)})
		{
			const auto A0 = Run.At.find(W.first);
			const auto A1 = Run.At.find(W.second);
			for (size_t K = 0; K < 5; ++K)
			{
				const double V = A0 != Run.At.end() && A1 != Run.At.end() ? A1->second.Earned[K] - A0->second.Earned[K] : 0.0;
				std::fprintf(To, "\t%.2f", V);
			}
		}
		for (const double F : Run.FollowSrc)
		{
			std::fprintf(To, "\t%.0f", F);
		}
		std::fprintf(To, "\n");
		std::fflush(To);
		// Stream by stream (S-K's diagnosis): KAST_SK_STREAMS=<.tsv> appends each stream's day, average, peak, follows and the
		// night's mean members, strangers, raiders, lurkers and hype.
		if (const char* Path = std::getenv("KAST_SK_STREAMS"))
		{
			if (FILE* F = std::fopen(Path, "a"))
			{
				for (const CareerStream& S : Run.Streams)
				{
					const double Mn = static_cast<double>(std::max(1, S.Minutes));
					std::fprintf(F, "%s\t%d\t%d\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.1f\t%.1f", Arch.c_str(), Seed, S.Stream, S.Day, S.Avg, S.Peak, S.Follows, S.Parts[0] / Mn,
						S.Parts[1] / Mn, S.Parts[2] / Mn, S.Parts[3] / Mn, S.Parts[4] / Mn, S.HeatStart, S.HeatEnd);
					std::fprintf(F, "\t%.3f\t%.3f", S.Parts[5] / Mn, S.Parts[6] / Mn);
					for (const double P : S.Ping)
					{
						std::fprintf(F, "\t%.3f", P);
					}
					std::fprintf(F, "\n");
				}
				std::fclose(F);
			}
		}
		std::fflush(To);
	}
}
} // namespace room_career

void SegSession()
{
	// S-K's C++ side (§9.2): KAST_SK=<archetype>,<first seed>,<seeds>,<days>,<out .tsv> runs only the careers' table.
	if (const char* Sk = std::getenv("KAST_SK"))
	{
		char Arch[64] = {0};
		char Path[512] = {0};
		int Seed0 = 0, Seeds = 1, Days = 366;
		if (std::sscanf(Sk, "%63[^,],%d,%d,%d,%511s", Arch, &Seed0, &Seeds, &Days, Path) == 5)
		{
			if (FILE* F = std::fopen(Path, "w"))
			{
				room_career::CareersTable(Arch, Seed0, Seeds, Days, F);
				std::fclose(F);
			}
		}
		return;
	}
	const auto T0 = std::chrono::steady_clock::now();
	if (const char* Bench = std::getenv("KAST_BENCH"))
	{
		if (std::string(Bench) == "only")
		{
			room_seg::RoomBench();
			return;
		}
	}
	room_seg::HonestCount();
	room_seg::PaceInvariance();
	room_seg::FrameRate();
	room_seg::FirstMinutes();
	room_seg::OfflineSplit();
	room_seg::Exploits();
	room_seg::SpamGate();
	room_seg::PaceFlags();
	room_seg::PlayerVerbs();
	room_seg::ViewVerbs();
	room_seg::HelloOnce();
	room_seg::PoolReseed();
	room_seg::MoneyRules();
	room_seg::ReviewFixes();
	room_seg::GiveTerms();
	room_seg::GiveCache();
	room_seg::GiveSkip();
	room_seg::FastDraws();
	room_seg::EventDriven();
	room_seg::LurkCache();
	room_seg::FirstsLive();
	room_seg::ViewFacts();
	room_seg::RoomLines();
	room_seg::ForecastLurkers();
	room_seg::PieAlertCount();
	room_seg::PiePeakLabel();
	room_seg::PieShortRun();
	room_seg::PieCopy();
	room_seg::PieReviewRoom();
	room_seg::RoomBench();
	room_career::LongMtt();
	room_career::Careers();
	room_career::CareersEventDriven();
	// Lazy giving's bound held on every stream of this run, the careers' included (review-fix-room: a replay that found a gift
	// before the minute that asked for it would give later than the eager walk, a result change the bands would not see).
	const uint64_t Late = ss::kast::RoomState::GiveLate.load();
	std::printf("  kast room: lazy giving's replays that found a gift late over the whole run, careers included: %llu\n", static_cast<unsigned long long>(Late));
	Expect(Late == 0, "kast room: lazy giving's bound held on every stream of the run, the careers' included");
	std::printf("  kast room: %.1fs\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace kastgrowth_test
