// The creator update (SPEC_CREATORS §2.9, §5.1, §5.3 K2, K3, K10-K12): Kast's chat games, core-kastplay's file. Hold or Crack,
// Nod, Welcome the raid, Set the scene, Rail Call's press and its named arrivals, First Follower's line, the action slot and the
// toast line's one-a-minute rule, all through the RoomView Kast.cpp implements (KastPlay.h). Every word is KastCopy.h's.
//
// The two clocks (R3, R4): what moves the room (a Nodded guest's follow, Rail Call's arrivals, the raid's welcome, the scene's
// bank, the reveal's Hype, a call's Engage) happens in the verbs (the player's clicks: inputs) and in OnMoment / OnMinute (the
// world-minute grid) from the inputs and keyed draws, growth::Unit(Room.Seed(), Room.StreamNo(), Tag, ...). OnChat is display:
// chat's calls, the replies, the reveal's words and the toasts, from the stream's own R; it is handed no Channel, and nothing
// the room reads depends on it (the reveal's Hype doesn't read how chat called), so a frame rate never moves the room.
#include "KastPlay.h"

#include "KastCopy.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ss
{
namespace kast
{
// The file's private helpers (Unreal's unity builds paste files together: every name here is prefixed).
namespace kastplay_detail
{
/** A Nod lights a name for this many world minutes (§5.3 K11). */
constexpr double KastPlayGlowMinutes = 1.0;
/** Nods a stream (§2.9). */
constexpr int KastPlayNods = 3;
/** A Nodded guest follows with this x growth::Sat (§2.9, synth_kast.py's NOD_FOLLOW). */
constexpr double KastPlayNodFollow = 0.10;
/** A member's one-time Affinity bump when the streamer Nods or answers them (§2.8.2: +3% of the gap). */
constexpr double KastPlayNoticedShare = 0.03;
/** Hold or Crack: 2+ watching (§5.3 K12, REVIEW_PLAYER M9), Hype +8 at the reveal, Engage +0.3 for the streamer's call, and a
 *  reveal that never came closes after this many world minutes (a runout takes seconds). */
constexpr int KastPlayHocWatching = 2;
constexpr double KastPlayHocHype = 8.0;
constexpr double KastPlayHocEngage = 0.3;
constexpr double KastPlayHocStaleMinutes = 5.0;
/** Welcome the raid: within 2 world minutes of the raid, stays x1.3 and follows x1.5 (§2.7.1). */
constexpr double KastPlayWelcomeMinutes = 2.0;
constexpr double KastPlayWelcomeStay = 1.3;
constexpr double KastPlayWelcomeFollow = 1.5;
/** Set the scene: streams 1-20 at a true 0; the next 3 arrivals stay x1.2 (§2.9, §5.3 K2). */
constexpr uint64_t KastPlaySceneStreams = 20;
constexpr int KastPlaySceneArrivals = 3;
constexpr double KastPlaySceneStay = 1.2;
/** Rail Call's arrivals come at +Exp(6) world minutes (§2.3.6). */
constexpr double KastPlayRailDelay = 6.0;
/** The toast line: one a real minute (§5.1). */
constexpr double KastPlayToastSeconds = 60.0;
/** The two friends by their handles (Kast.cpp's Start adds them on the first stream): their Rail Call lines are their own. */
const char* const KastPlayDee = "dee_spincycle";
const char* const KastPlayMei = "mei_ng";

/** The games' chat lines that wait for display time (PlayState::Line::Kind). */
enum KastPlayLineKind : int
{
	KastPlayLineNodReply = 1,
	KastPlayLineRailArrival,
	KastPlayLineRailDee,
	KastPlayLineRailMei,
	KastPlayLineFirstCheer,
	KastPlayLineCalledIt,
	KastPlayLineMissed,
	KastPlayLineShambles,
	KastPlayLineWelcomed,
};

/** A waiting line's words: KastCopy's, a pool's variant picked by the stream's R (display only). */
std::string KastPlayLineText(const PlayState::Line& L, Rng& R)
{
	switch (L.Kind)
	{
	case KastPlayLineNodReply: return R.Pick(copy::NodReply());
	case KastPlayLineRailArrival: return R.Pick(copy::RailCallArrive());
	case KastPlayLineRailDee: return copy::DeeOmw();
	case KastPlayLineRailMei: return copy::MeiRail();
	case KastPlayLineFirstCheer: return R.Pick(copy::FirstFollowerChat());
	case KastPlayLineCalledIt: return R.Pick(copy::CalledItChat());
	case KastPlayLineMissed: return R.Pick(copy::MissedCallChat());
	case KastPlayLineShambles: return R.Pick(copy::RevealWrong());
	case KastPlayLineWelcomed: return R.Pick(copy::WelcomedChat());
	default: return std::string();
	}
}

bool KastPlayHas(const std::vector<std::string>& Names, const std::string& Name)
{
	return std::find(Names.begin(), Names.end(), Name) != Names.end();
}

/** The room's minute as a draw key. */
uint64_t KastPlayMinuteKey(double Minute)
{
	return static_cast<uint64_t>(std::max(0.0, std::floor(Minute)));
}

/** The welcome window is open now: a raid came, it isn't welcomed yet, and it's under 2 world minutes old. */
bool KastPlayWelcomeOpen(const PlayState& P, double Now)
{
	return P.RaidAt >= 0.0 && P.WelcomedAt != P.RaidAt && Now < P.RaidAt + KastPlayWelcomeMinutes;
}

/** Followers not attending tonight (Rail Call's reach): Dee and Mei included, the untracked pool not (§2.3.6). */
int KastPlayRailReach(const RoomView& Room, const Channel& Ch)
{
	int Reach = 0;
	for (const Member& M : Ch.Members)
	{
		Reach += M.Follower && !Room.Attending(M.Name) ? 1 : 0;
	}
	return Reach;
}

/** How many of the named people present call it (display only): everyone in a small room, a third or so of a big one. */
int KastPlayHocTarget(size_t Named)
{
	if (Named <= 3)
	{
		return static_cast<int>(Named);
	}
	const double N = static_cast<double>(Named);
	const double Share = std::min(1.0, std::max(0.3, 0.3 + 1.2 / std::sqrt(N)));
	return std::min(300, static_cast<int>(std::ceil(N * Share)));
}

/** A caller's chance of saying HOLDS: chat reads the equity, and roots for the streamer a little (display only). */
double KastPlayHoldsChance(double Equity)
{
	const double E = Equity >= 0.0 ? Equity : 0.47;
	return std::min(0.95, std::max(0.05, 0.15 + 0.85 * E));
}

void KastPlayPostToast(PlayState& P, RoomView& Room, const PlayState::Toast& T)
{
	if (play::ToastFree(P) && P.Toasts.empty())
	{
		Room.PostAlert(T.A);
		Room.PostNotice(T.N);
		play::ToastShown(P);
		return;
	}
	P.Toasts.push_back(T);
}

void KastPlayQueue(PlayState& P, int Kind, std::vector<std::string> Speakers, double Delay)
{
	PlayState::Line L;
	L.Kind = Kind;
	L.Speakers = std::move(Speakers);
	L.Due = P.RealT + Delay;
	P.Pending.push_back(std::move(L));
}

/** Hold or Crack's reveal: Hype +8 whatever chat called, then chat's result, the night's tally and KastCall. */
void KastPlayReveal(PlayState& P, RoomView& Room, bool Holds)
{
	Room.AddHype(KastPlayHocHype);
	int Right = 0;
	int Points = 0;
	std::vector<std::string> RightNames;
	std::vector<std::string> WrongNames;
	// Tonight's tally, one lookup a call: a big room's 300 callers scanned against the night's 1,000+ callers one by one took
	// about a millisecond of the reveal's frame (review). New callers join in calling order (Close's "called first" tie).
	std::unordered_map<std::string, size_t> TallyAt;
	TallyAt.reserve(P.Callers.size() + P.HoC.Calls.size());
	for (size_t I = 0; I < P.Callers.size(); ++I)
	{
		TallyAt.emplace(P.Callers[I].Name, I);
	}
	for (const PlayState::Call& C : P.HoC.Calls)
	{
		const bool Was = C.Holds == Holds;
		Right += Was ? 1 : 0;
		Points += Was ? C.Points : 0;
		const auto Found = TallyAt.emplace(C.Who, P.Callers.size());
		if (Found.second)
		{
			PlayState::Caller Fresh;
			Fresh.Name = C.Who;
			P.Callers.push_back(Fresh);
		}
		PlayState::Caller& Tally = P.Callers[Found.first->second];
		++Tally.Of;
		Tally.Right += Was ? 1 : 0;
		(Was ? RightNames : WrongNames).push_back(C.Who);
	}
	const int Of = static_cast<int>(P.HoC.Calls.size());
	if (Of > 0)
	{
		// The room erupts (§5.3 K12): the result, then the callers themselves (each line from someone still here at its time).
		Room.Speak(std::string(), copy::RevealLine(Holds, Right, Of, Points), LineKind::System);
		Notice N;
		N.Type = Notice::Kind::Chime;
		N.Sound = static_cast<int>(SoundId::KastCall);
		Room.PostNotice(N);
		const size_t Cheers = std::min<size_t>(3, RightNames.size());
		for (size_t I = 0; I < Cheers; ++I)
		{
			KastPlayQueue(P, KastPlayLineCalledIt, {RightNames[I]}, 0.5 + 0.45 * static_cast<double>(I));
		}
		if (!WrongNames.empty())
		{
			// Most of chat called it wrong: the rail is in shambles; otherwise one groan.
			KastPlayQueue(P, 2 * Right < Of ? KastPlayLineShambles : KastPlayLineMissed, WrongNames, 0.8 + 0.45 * static_cast<double>(Cheers));
		}
	}
	P.HoC = PlayState::HoldCrack();
}

/** Chat calls HOLDS or CRACKS while Hold or Crack is open: the named people present at the first display step after the open,
 *  shuffled once by R, each calls once, fast enough to land while the board runs (display only). */
void KastPlayCalls(PlayState& P, RoomView& Room, double DReal, Rng& R)
{
	PlayState::HoldCrack& H = P.HoC;
	if (!H.Queued)
	{
		H.Queued = true;
		H.Queue = Room.PresentNames();
		R.Shuffle(H.Queue);
		H.Target = KastPlayHocTarget(H.Queue.size());
		if (static_cast<int>(H.Queue.size()) > H.Target)
		{
			H.Queue.resize(static_cast<size_t>(H.Target));
		}
		H.Debt = 0.4; // the first call comes within the second
	}
	const double Rate = std::max(1.5, static_cast<double>(H.Target) / 3.0); // calls a real second
	H.Debt += std::max(0.0, DReal) * Rate;
	while (H.Debt >= 1.0 && !H.Queue.empty())
	{
		H.Debt -= 1.0;
		const std::string Who = H.Queue.back();
		H.Queue.pop_back();
		if (!Room.Present(Who))
		{
			continue; // left before calling it
		}
		PlayState::Call C;
		C.Who = Who;
		C.Holds = R.Chance(KastPlayHoldsChance(H.Equity));
		C.Points = (1 + R.Int(10)) * 100;
		if (Room.Speak(Who, R.Pick(copy::ChatCalls(C.Holds)), LineKind::Chat))
		{
			H.Calls.push_back(C);
		}
	}
}
} // namespace kastplay_detail

namespace play
{
// ------------------------------------------------------------------ the verbs

bool Nod(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name)
{
	if (!P.Live || P.NodsLeft <= 0 || Name.empty() || WasNodded(P, Name) || !Room.Present(Name))
	{
		return false;
	}
	--P.NodsLeft;
	P.Nodded.push_back(Name);
	PlayState::Glow G;
	G.Name = Name;
	G.Until = std::max(P.World, Room.Minute()) + kastplay_detail::KastPlayGlowMinutes;
	P.Lit.push_back(G);
	if (Member* M = Ch.FindMember(Name))
	{
		// A member: Noticed once ever (+3% of the Affinity gap), and loyalty x1.6 for the rest of tonight (the Room's).
		if (!M->Noticed)
		{
			M->Noticed = true;
			M->Affinity = growth::AffinityBump(M->Affinity, kastplay_detail::KastPlayNoticedShare);
		}
		Room.MarkNoticed(Name);
	}
	else
	{
		// A guest: follows at 0.10 x Sat, a keyed draw on the name and the room's minute (§2.9, Tag::Nod).
		const double U = growth::Unit(Room.Seed(), Room.StreamNo(), growth::Tag::Nod, growth::NameKey(Name), kastplay_detail::KastPlayMinuteKey(Room.Minute()));
		if (U < kastplay_detail::KastPlayNodFollow * growth::Sat(Ch.Followers))
		{
			Room.Follow(Name);
		}
	}
	kastplay_detail::KastPlayQueue(P, kastplay_detail::KastPlayLineNodReply, {Name}, 0.9);
	return true;
}

bool RailCall(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In)
{
	if (!P.Live || !P.RailOpen || P.RailCalled || !In.Attended)
	{
		return false;
	}
	const int Today = net::DayOf(In.World);
	if (growth::Granted(Ch.GrantDay, Ch.GrantMask, Today, growth::GrantRailCall))
	{
		return false;
	}
	const uint64_t Seed = Room.Seed();
	const uint64_t StreamNo = Room.StreamNo();
	const double Sched = Room.Sched();
	const double ToD = Room.ToD();
	int Reach = 0;
	std::vector<std::string> Sent;
	std::vector<double> SentAt;
	for (const Member& M : Ch.Members)
	{
		if (!M.Follower || Room.Attending(M.Name))
		{
			continue;
		}
		++Reach;
		// The second roll (§2.3.6): it reads recency, appetite and the attention budget like the Ping; mods' recency reads 1.
		const double DaysSince = std::max(0.0, In.World - M.LastSeen) / net::MinutesPerDay;
		const double W = growth::PingW(DaysSince, Ch.IsMod(M.Name) ? 1.0 : M.Loyalty);
		const double Chance = growth::RailCallChance(W, M.Loyalty, Sched, ToD, growth::Appetite(DaysSince), growth::Budget(growth::HwToday(M.Hw, M.HwDay, Today)));
		const uint64_t Key = growth::NameKey(M.Name);
		if (growth::Unit(Seed, StreamNo, growth::Tag::RailCall, Key) < Chance)
		{
			Sent.push_back(M.Name);
			SentAt.push_back(In.World + growth::ExpAt(kastplay_detail::KastPlayRailDelay, growth::Unit(Seed, StreamNo, growth::Tag::RailCall, Key, 1)));
		}
	}
	if (Reach == 0)
	{
		// Everyone is here already: nothing to send ("Your whole rail is here"), and no grant used.
		P.WholeRail = true;
		return false;
	}
	// The arrivals go to the Room after the scan (the community isn't walked while the Room books anything).
	for (size_t I = 0; I < Sent.size(); ++I)
	{
		Room.ArriveAt(Sent[I], SentAt[I], View::Source::Members);
	}
	growth::UseGrant(Ch.GrantDay, Ch.GrantMask, Today, growth::GrantRailCall);
	P.RailCalled = true;
	P.RailOpen = false;
	P.WholeRail = false;
	P.RailReach = Reach;
	P.RailSent = std::move(Sent);
	Room.Speak(std::string(), copy::RailCallReached(Reach), LineKind::System);
	return true;
}

bool WelcomeRaid(PlayState& P, RoomView& Room, Channel& /*Ch*/)
{
	if (!P.Live || !Room.Attended())
	{
		return false;
	}
	P.RaidAt = Room.RaidAt();
	if (!kastplay_detail::KastPlayWelcomeOpen(P, std::max(P.World, Room.Minute())))
	{
		return false;
	}
	P.WelcomedAt = P.RaidAt;
	Room.WelcomeRaiders(kastplay_detail::KastPlayWelcomeStay, kastplay_detail::KastPlayWelcomeFollow);
	Room.Speak(std::string(), copy::WelcomeRaidStreamer(), LineKind::Streamer);
	// Two of the raid's named people say thanks (each only if still here when the line comes up).
	const std::vector<std::string> Raiders = Room.RaidNamesHere();
	for (size_t I = 0; I < std::min<size_t>(2, Raiders.size()); ++I)
	{
		kastplay_detail::KastPlayQueue(P, kastplay_detail::KastPlayLineWelcomed, {Raiders[I]}, 0.9 + 0.6 * static_cast<double>(I));
	}
	return true;
}

bool SetTheScene(PlayState& P, RoomView& Room, Channel& /*Ch*/)
{
	if (!P.Live || P.SceneUsed || !Room.Attended() || Room.Watching() != 0 || Room.StreamNo() > kastplay_detail::KastPlaySceneStreams)
	{
		return false;
	}
	P.SceneUsed = true;
	P.SceneOpen = false;
	Room.BankArrivals(kastplay_detail::KastPlaySceneArrivals, kastplay_detail::KastPlaySceneStay);
	Room.Speak(std::string(), copy::SetTheScene(), LineKind::System);
	return true;
}

bool CallIt(PlayState& P, RoomView& Room, Channel& /*Ch*/, bool Holds)
{
	// Attended only (§2.9, §1.4): heads-down (the pace went to watch 4x, 8x, a skip or a sprint mid-runout) the call buttons
	// aren't drawn (Fill) and a press is refused, so no Engage.
	if (!P.Live || !P.HoC.Open || P.HoC.Called != 0 || !Room.Attended())
	{
		return false;
	}
	P.HoC.Called = Holds ? 1 : 2;
	Room.AddEngage(kastplay_detail::KastPlayHocEngage);
	Room.Speak(std::string(), copy::StreamerCall(Holds), LineKind::Streamer);
	return true;
}

// ------------------------------------------------------------------ the Room's hooks

void Begin(PlayState& P, const Channel& /*Ch*/, const Inputs& In)
{
	P = PlayState();
	P.Live = true;
	P.StartWorld = In.World;
	P.World = In.World;
	P.Attended = In.Attended;
	P.NodsLeft = kastplay_detail::KastPlayNods;
}

void OnMoment(PlayState& P, RoomView& Room, Channel& /*Ch*/, const Inputs& In, Moment M)
{
	if (!P.Live)
	{
		return;
	}
	P.World = std::max(P.World, In.World);
	P.Attended = In.Attended;
	// RAIL CALL goes with the run (§2.3.6: "until it is pressed or the run ends"): a bust's own inputs show no armed run at depth
	// 0.4, so the button goes at once, not at the next grid minute (13 real seconds at Smart). Another table's deep run, or
	// ss.Kast.Moment's held run (the moment's inputs carry it), keeps it.
	if (P.RailOpen && !growth::RailCallOpen(In.DeepEntrants, In.Depth))
	{
		P.RailOpen = false;
		P.WholeRail = false;
	}
	if (M == Moment::AllIn)
	{
		// Opens on the front table's all-in with cards to come (the reveal before the river; a known equity of 0 or 1 is a
		// decided hand), 2+ watching, at an attended minute (heads-down, the prompt simply doesn't open).
		const bool Undecided = In.AllInEquity < 0.0 || (In.AllInEquity > 0.0 && In.AllInEquity < 1.0);
		if (In.ThisTable == In.FrontTable && Undecided && In.Attended && Room.Watching() >= kastplay_detail::KastPlayHocWatching)
		{
			P.HoC = PlayState::HoldCrack();
			P.HoC.Open = true;
			P.HoC.Table = In.ThisTable;
			P.HoC.Equity = In.AllInEquity;
			P.HoC.OpenedAt = In.World;
		}
		return;
	}
	if ((M == Moment::WonAllIn || M == Moment::LostAllIn || M == Moment::BadBeat) && P.HoC.Open && In.ThisTable == P.HoC.Table)
	{
		kastplay_detail::KastPlayReveal(P, Room, M == Moment::WonAllIn);
	}
}

void OnMinute(PlayState& P, RoomView& Room, Channel& Ch, const Inputs& In, int Minute)
{
	if (!P.Live)
	{
		return;
	}
	// The grid's index is the stream minute: its world minute is StartWorld + Minute (the Room's RunMinute).
	const double Now = P.StartWorld + static_cast<double>(Minute);
	P.World = std::max(P.World, Now);
	P.Attended = In.Attended;
	P.RaidAt = Room.RaidAt();
	P.Lit.erase(std::remove_if(P.Lit.begin(), P.Lit.end(), [Now](const PlayState::Glow& G) { return G.Until <= Now; }), P.Lit.end());
	if (P.HoC.Open && Now - P.HoC.OpenedAt >= kastplay_detail::KastPlayHocStaleMinutes)
	{
		P.HoC = PlayState::HoldCrack(); // the reveal never came (the hand went elsewhere): no result, no Hype
	}
	// RAIL CALL (§2.3.6): offered at the first attended minute an armed run is at depth 0.4 while today's grant is unused; it
	// stays (heads-down it just isn't drawn) until pressed or until the deepest run is no longer at depth 0.4 (it ended).
	const bool Deep = growth::RailCallOpen(In.DeepEntrants, In.Depth);
	if (P.RailOpen && !Deep)
	{
		P.RailOpen = false;
		P.WholeRail = false;
	}
	else if (!P.RailOpen && !P.RailCalled && Deep && In.Attended &&
			 !growth::Granted(Ch.GrantDay, Ch.GrantMask, net::DayOf(Now), growth::GrantRailCall))
	{
		P.RailOpen = true;
	}
	if (P.RailOpen && In.Attended)
	{
		P.WholeRail = kastplay_detail::KastPlayRailReach(Room, Ch) == 0;
	}
}

void OnChat(PlayState& P, RoomView& Room, const Inputs& In, double DReal, Rng& R)
{
	if (!P.Live)
	{
		return;
	}
	P.RealT += std::max(0.0, DReal);
	P.World = std::max(P.World, In.World);
	P.Attended = In.Attended;
	P.RaidAt = Room.RaidAt();
	P.SceneOpen = !P.SceneUsed && P.Attended && Room.Watching() == 0 && Room.StreamNo() <= kastplay_detail::KastPlaySceneStreams;
	if (P.HoC.Open)
	{
		kastplay_detail::KastPlayCalls(P, Room, DReal, R);
	}
	// The games' waiting lines: each from the first of its speakers still here when it comes up (R1), or from nobody.
	for (size_t I = 0; I < P.Pending.size();)
	{
		if (P.Pending[I].Due > P.RealT)
		{
			++I;
			continue;
		}
		const PlayState::Line L = P.Pending[I];
		P.Pending.erase(P.Pending.begin() + static_cast<std::ptrdiff_t>(I));
		for (const std::string& Who : L.Speakers)
		{
			if (Room.Present(Who))
			{
				Room.Speak(Who, kastplay_detail::KastPlayLineText(L, R), LineKind::Chat);
				break;
			}
		}
	}
	while (!P.Toasts.empty() && ToastFree(P))
	{
		Room.PostAlert(P.Toasts.front().A);
		Room.PostNotice(P.Toasts.front().N);
		P.Toasts.erase(P.Toasts.begin());
		ToastShown(P);
	}
}

void OnArrive(PlayState& P, RoomView& /*Room*/, Channel& /*Ch*/, const std::string& Name, View::Source /*From*/)
{
	if (!P.Live)
	{
		return;
	}
	const auto It = std::find(P.RailSent.begin(), P.RailSent.end(), Name);
	if (It == P.RailSent.end())
	{
		return;
	}
	P.RailSent.erase(It);
	++P.RailArrived;
	const int Kind = Name == kastplay_detail::KastPlayDee ? kastplay_detail::KastPlayLineRailDee
					 : Name == kastplay_detail::KastPlayMei ? kastplay_detail::KastPlayLineRailMei
															: kastplay_detail::KastPlayLineRailArrival;
	kastplay_detail::KastPlayQueue(P, Kind, {Name}, 0.6);
}

void OnFollow(PlayState& P, RoomView& Room, Channel& Ch, const std::string& Name, bool FirstFollower)
{
	if (!P.Live || !FirstFollower || Name.empty())
	{
		return;
	}
	Room.Speak(std::string(), copy::FirstFollower(Name), LineKind::System);
	PlayState::Toast T;
	T.A.Kind = AlertKind::Follow;
	T.A.Who = Name;
	T.A.Text = copy::FirstFollowerSub();
	T.N.Type = Notice::Kind::Milestone;
	T.N.From = Name;
	T.N.Body = copy::FirstFollower(Name);
	T.N.Sound = static_cast<int>(SoundId::KastShine);
	kastplay_detail::KastPlayPostToast(P, Room, T);
	// Dee cheers (Mei if Dee isn't here): the friends, in the community's order.
	std::vector<std::string> Friends;
	for (const Member& M : Ch.Members)
	{
		if (M.Friend && M.Name != Name)
		{
			Friends.push_back(M.Name);
		}
	}
	if (!Friends.empty())
	{
		kastplay_detail::KastPlayQueue(P, kastplay_detail::KastPlayLineFirstCheer, Friends, 1.2);
	}
}

void Fill(const PlayState& P, View& V)
{
	// The attended buttons (§2.9's table) are drawn only while the pace is attended: Hold or Crack, Welcome the raid, Rail Call
	// (and Rail Call's "Your whole rail is here" in its place), Set the scene (OnChat's SceneOpen). Nod is drawn at every pace.
	// Heads-down mid-runout the Hold or Crack bar and its call buttons go at once (the press is refused: CallIt), while chat's
	// game runs on to its reveal as at any pace (the room never reads the pace, R3: Hype +8, KastCall, the result line); back at
	// an attended pace before the reveal, the bar shows again.
	const bool Hoc = P.HoC.Open && P.Attended;
	V.HoC = View::HoldCrack();
	if (Hoc)
	{
		V.HoC.Open = true;
		for (const PlayState::Call& C : P.HoC.Calls)
		{
			(C.Holds ? V.HoC.Holds : V.HoC.Cracks) += 1;
		}
		V.HoC.Equity = P.HoC.Equity;
		V.HoC.Called = P.HoC.Called;
	}
	V.NodsLeft = P.Live ? P.NodsLeft : 0;
	V.Lit.clear();
	for (const PlayState::Glow& G : P.Lit)
	{
		if (G.Until > P.World)
		{
			V.Lit.push_back(G.Name);
		}
	}
	V.SceneOpen = P.SceneOpen;
	V.RailReach = P.RailReach;
	V.RailArrived = P.RailArrived;
	// "Your whole rail is here" stands in RAIL CALL's place, so it follows the button: attended only (heads-down the reach scan
	// doesn't run, OnMinute, so the line could be stale).
	V.WholeRail = P.Attended && P.RailOpen && P.WholeRail;
	// The one action slot (§5.1): Hold or Crack (seconds) > Welcome the raid (2 world minutes) > Rail Call; the others wait as
	// chips. None of them is drawn heads-down.
	const bool Welcome = P.Attended && kastplay_detail::KastPlayWelcomeOpen(P, P.World);
	const bool Rail = P.Attended && P.RailOpen && !P.RailCalled && !P.WholeRail;
	V.Action = Hoc ? View::Slot::HoldCrack : Welcome ? View::Slot::WelcomeRaid : Rail ? View::Slot::RailCall : View::Slot::None;
	V.Waiting = 0;
	if (Welcome && V.Action != View::Slot::WelcomeRaid)
	{
		V.Waiting |= 1 << static_cast<int>(View::Slot::WelcomeRaid);
	}
	if (Rail && V.Action != View::Slot::RailCall)
	{
		V.Waiting |= 1 << static_cast<int>(View::Slot::RailCall);
	}
}

void Close(PlayState& P, Summary& S)
{
	P.HoC = PlayState::HoldCrack(); // no reveal after the stream
	const PlayState::Caller* Best = nullptr;
	for (const PlayState::Caller& C : P.Callers)
	{
		// Most right; then fewer wrong; then whoever called first tonight.
		if (C.Right > 0 && (!Best || C.Right > Best->Right || (C.Right == Best->Right && C.Of - C.Right < Best->Of - Best->Right)))
		{
			Best = &C;
		}
	}
	if (Best)
	{
		S.BestCaller = Best->Name;
		S.BestCalls = Best->Right;
		S.BestCallsOf = Best->Of;
	}
	S.RailCalled = P.RailCalled;
	S.RailReach = P.RailReach;
	S.RailArrived = P.RailArrived;
	P.Pending.clear();
	P.Toasts.clear();
	P.Live = false;
}

bool WasNodded(const PlayState& P, const std::string& Name)
{
	return kastplay_detail::KastPlayHas(P.Nodded, Name);
}

bool ToastFree(const PlayState& P)
{
	return !P.HoC.Open && P.RealT - P.ToastAt >= kastplay_detail::KastPlayToastSeconds;
}

void ToastShown(PlayState& P)
{
	P.ToastAt = P.RealT;
}
} // namespace play
} // namespace kast
} // namespace ss
