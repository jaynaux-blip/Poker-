// kast_growth: the chat games (SPEC_CREATORS §2.9, §5.1, §5.3 K2, K3, K10-K12, §9.1; core-kastplay's file). Hold or Crack opens
// only with an all-in on the front table, cards to come, 2+ watching and an attended minute, counts its callers (each a person
// present, each once), reveals from the same table with Hype +8 whatever chat called, and moves no money; Nod (three a stream,
// one a person, a member Noticed once ever, a guest's follow on its keyed draw), Welcome the raid (2 world minutes, attended,
// once a raid), Set the scene (a true 0, streams 1-20, attended, once), Rail Call (offered at an attended minute at depth 0.4,
// its reach and its keyed second roll, once a world day, "Your whole rail is here"), First Follower's line, the one action
// slot and the toast line; every named line from someone present; and the room effects identical whatever the frame rate and
// the stream's R. Driven through a RoomView double of its own (FakeRoom). Called from kastgrowth_main.cpp (the kit is
// KastGrowthSeg.h).
#include "KastGrowthSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/KastCopy.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/KastPlay.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace kastgrowth_test
{
namespace play_seg
{
using ss::kast::Alert;
using ss::kast::Channel;
using ss::kast::Inputs;
using ss::kast::LineKind;
using ss::kast::Member;
using ss::kast::Moment;
using ss::kast::Notice;
using ss::kast::PlayState;
using ss::kast::Summary;
using ss::kast::View;
namespace growth = ss::kast::growth;
namespace play = ss::kast::play;
namespace kcopy = ss::kast::copy;

constexpr uint64_t PlaySeed = 0x5eed2026ull;
constexpr double PlayDay = 1440.0;
const char* const PlayDee = "dee_spincycle";
const char* const PlayMei = "mei_ng";

/** The Room as the games see it, with every effect written down. */
class FakeRoom : public ss::kast::RoomView
{
public:
	struct Said
	{
		std::string Who;
		std::string Text;
		LineKind Kind = LineKind::Chat;
		double Minute = 0.0;
	};
	struct Arrival
	{
		std::string Name;
		double At = 0.0;
		View::Source From = View::Source::Members;
	};

	double MinuteV = 0.0;
	uint64_t SeedV = PlaySeed;
	uint64_t StreamV = 7;
	bool AttendedV = true;
	double SchedV = 1.3;
	double ToDV = 1.0;
	int Extra = 0; // lurkers and crowd: in the count, never speaking
	std::vector<std::string> Here;
	std::set<std::string> AttendingAway; // attending tonight but away on the slot process
	double RaidAtV = -1.0;
	std::vector<std::string> Raiders; // the newest raid's named people (RaidNamesHere returns the ones present)
	// What the games did.
	std::vector<Said> Lines;
	int Refused = 0; // lines offered for someone not present (the games should never try)
	std::vector<Alert> Alerts;
	std::vector<Notice> Notices;
	double EngageV = 0.0;
	double HypeV = 0.0;
	std::vector<std::string> Follows;
	std::vector<std::string> Noticed;
	std::vector<Arrival> Arrivals;
	std::vector<std::pair<double, double>> Welcomes;
	std::vector<std::pair<int, double>> Banks;

	double Minute() const override { return MinuteV; }
	uint64_t Seed() const override { return SeedV; }
	uint64_t StreamNo() const override { return StreamV; }
	bool Attended() const override { return AttendedV; }
	double Sched() const override { return SchedV; }
	double ToD() const override { return ToDV; }
	int Watching() const override { return static_cast<int>(Here.size()) + Extra; }
	bool Present(const std::string& Name) const override { return std::find(Here.begin(), Here.end(), Name) != Here.end(); }
	std::vector<std::string> PresentNames() const override { return Here; }
	bool Attending(const std::string& Name) const override { return Present(Name) || AttendingAway.count(Name) != 0; }
	bool Speak(const std::string& Who, const std::string& Text, LineKind Kind) override
	{
		if (Kind == LineKind::Streamer)
		{
			Lines.push_back({"<streamer>", Text, Kind, MinuteV});
			return true;
		}
		if (Who.empty())
		{
			Lines.push_back({std::string(), Text, Kind, MinuteV});
			return true;
		}
		if (!Present(Who))
		{
			++Refused;
			return false;
		}
		Lines.push_back({Who, Text, Kind, MinuteV});
		return true;
	}
	void PostAlert(const Alert& A) override { Alerts.push_back(A); }
	void PostNotice(const Notice& N) override { Notices.push_back(N); }
	double Engage() const override { return EngageV; }
	void AddEngage(double By) override { EngageV += By; }
	void AddHype(double By) override { HypeV += By; }
	bool Follow(const std::string& Name) override
	{
		Follows.push_back(Name);
		return true;
	}
	void MarkNoticed(const std::string& Name) override { Noticed.push_back(Name); }
	void ArriveAt(const std::string& Name, double At, View::Source From) override { Arrivals.push_back({Name, At, From}); }
	double RaidAt() const override { return RaidAtV; }
	std::vector<std::string> RaidNamesHere() const override
	{
		std::vector<std::string> Out;
		for (const std::string& N : Raiders)
		{
			if (Present(N))
			{
				Out.push_back(N);
			}
		}
		return Out;
	}
	void WelcomeRaiders(double StayBy, double FollowBy) override { Welcomes.emplace_back(StayBy, FollowBy); }
	void BankArrivals(int N, double StayBy) override { Banks.emplace_back(N, StayBy); }

	/** Everything the games did to the room (not the display): two runs that agree here moved the room the same way. */
	std::string Effects() const
	{
		std::ostringstream O;
		O.precision(17);
		O << "E " << EngageV << " H " << HypeV << " |F";
		for (const std::string& N : Follows)
		{
			O << ' ' << N;
		}
		O << " |N";
		for (const std::string& N : Noticed)
		{
			O << ' ' << N;
		}
		O << " |A";
		for (const Arrival& A : Arrivals)
		{
			O << ' ' << A.Name << '@' << A.At;
		}
		O << " |W " << Welcomes.size() << " |B " << Banks.size();
		return O.str();
	}
	int LinesFrom(const std::string& Who) const
	{
		int N = 0;
		for (const Said& S : Lines)
		{
			N += S.Who == Who ? 1 : 0;
		}
		return N;
	}
	const Said* LastSystem() const
	{
		for (auto It = Lines.rbegin(); It != Lines.rend(); ++It)
		{
			if (It->Who.empty())
			{
				return &*It;
			}
		}
		return nullptr;
	}
	bool Said_(const std::string& Text) const
	{
		for (const Said& S : Lines)
		{
			if (S.Text == Text)
			{
				return true;
			}
		}
		return false;
	}
};

int PlayRefusedTotal = 0;
// The measured note's numbers (printed once at the end of SegPlay).
double PlayNodRate = 0.0;
int PlayBigCalls = 0;
int PlayBigHolds = 0;
int PlayRailReach = 0;
size_t PlayRailComing = 0;

Member PlayMember(const std::string& Name, double L, double LastSeen, bool Follower, bool Friend)
{
	Member M;
	M.Name = Name;
	M.Loyalty = L;
	M.Affinity = 0.3;
	M.FirstSeen = std::max(0.0, LastSeen - 30.0 * PlayDay);
	M.LastSeen = LastSeen;
	M.Follower = Follower;
	M.Friend = Friend;
	return M;
}

/** Dee and Mei, then N fans (loyalty spread 0.05..0.95, last seen 0..39 days ago), every one a follower, and a few
 *  watchers who never followed. */
Channel PlayChannel(int Fans, double World)
{
	Channel Ch;
	Ch.Members.push_back(PlayMember(PlayDee, 0.7, World - PlayDay, true, true));
	Ch.Members.push_back(PlayMember(PlayMei, 0.7, World - 2.0 * PlayDay, true, true));
	for (int I = 0; I < Fans; ++I)
	{
		Member M = PlayMember("fan_" + std::to_string(I), 0.05 + 0.1 * static_cast<double>(I % 10), World - static_cast<double>(I % 40) * PlayDay, true, false);
		M.Hw = static_cast<double>(I % 25);
		M.HwDay = ss::net::DayOf(World) - (I % 5);
		Ch.Members.push_back(M);
	}
	for (int I = 0; I < 3; ++I)
	{
		Ch.Members.push_back(PlayMember("watcher_" + std::to_string(I), 0.1, World - PlayDay, false, false));
	}
	Ch.Followers = Fans + 2 + 40; // and 40 untracked
	return Ch;
}

Inputs PlayIn(double World, bool Attended = true)
{
	Inputs In;
	In.World = World;
	In.Real = World * 13.1;
	In.Attended = Attended;
	In.FrontTable = 0;
	In.ThisTable = 0;
	In.AllInEquity = 0.64;
	In.Seed = PlaySeed;
	return In;
}

/** The inputs with a deep run on (980 runners at depth 0.55): RAIL CALL's run, which every moment carries while it lives. */
Inputs PlayDeep(double World, bool Attended = true)
{
	Inputs In = PlayIn(World, Attended);
	In.DeepEntrants = 980;
	In.Depth = 0.55;
	return In;
}

/** Display time: N steps of DReal seconds at world minute World. */
void PlayChat(PlayState& P, FakeRoom& Room, double World, double Seconds, double DReal, ss::Rng& R, bool Attended = true)
{
	const Inputs In = PlayIn(World, Attended);
	for (double T = 0.0; T < Seconds - 1e-9; T += DReal)
	{
		play::OnChat(P, Room, In, DReal, R);
	}
}

/** The money and the community, as a string: the games must never move it (Hold or Crack moves no money). */
std::string PlayMoney(const Channel& Ch)
{
	std::ostringstream O;
	O.precision(17);
	O << Ch.Followers << ' ' << Ch.EarnedSubs << ' ' << Ch.EarnedBits << ' ' << Ch.EarnedTips << ' ' << Ch.EarnedAds << ' ' << Ch.EarnedSponsors << ' '
	  << Ch.UnpaidCents << ' ' << Ch.PaidCents << ' ' << Ch.PendingCents << ' ' << Ch.Subs.size() << ' ' << Ch.GrantMask << ' ' << Ch.GrantDay;
	for (const Member& M : Ch.Members)
	{
		O << ' ' << M.Name << ':' << M.Loyalty << ':' << M.Affinity << ':' << M.Given << ':' << M.Noticed << ':' << M.Bonded;
	}
	return O.str();
}

// ------------------------------------------------------------------ Hold or Crack

void HocGates()
{
	const double W0 = 20.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(10, W0);
	const auto OpensWith = [&](const char* What, int Named, int Extra, bool Attended, int This, double Equity, Moment M) {
		FakeRoom Room;
		Room.MinuteV = W0;
		for (int I = 0; I < Named; ++I)
		{
			Room.Here.push_back("fan_" + std::to_string(I));
		}
		Room.Extra = Extra;
		PlayState P;
		play::Begin(P, Ch, PlayIn(W0));
		Inputs In = PlayIn(W0 + 0.4, Attended);
		In.ThisTable = This;
		In.AllInEquity = Equity;
		play::OnMoment(P, Room, Ch, In, M);
		View V;
		play::Fill(P, V);
		Expect(V.HoC.Open == P.HoC.Open && (V.Action == View::Slot::HoldCrack) == P.HoC.Open, std::string("hold or crack: the view shows what's open (") + What + ")");
		PlayRefusedTotal += Room.Refused;
		return P.HoC.Open;
	};
	Expect(OpensWith("2 watching", 2, 0, true, 0, 0.64, Moment::AllIn), "hold or crack: opens on the front table's all-in with 2 watching, attended");
	Expect(OpensWith("1 named + 1 lurker", 1, 1, true, 0, 0.64, Moment::AllIn), "hold or crack: 2 watching counts whole people (a lurker too)");
	Expect(!OpensWith("1 watching", 1, 0, true, 0, 0.64, Moment::AllIn), "hold or crack: 1 watching doesn't open it");
	Expect(!OpensWith("0 watching", 0, 0, true, 0, 0.64, Moment::AllIn), "hold or crack: an empty room doesn't open it");
	Expect(!OpensWith("heads-down", 5, 0, false, 0, 0.64, Moment::AllIn), "hold or crack: an unattended minute doesn't open it");
	Expect(!OpensWith("a table behind", 5, 0, true, 1, 0.64, Moment::AllIn), "hold or crack: a background table's all-in doesn't open it");
	Expect(!OpensWith("drawing dead", 5, 0, true, 0, 0.0, Moment::AllIn), "hold or crack: a decided hand (equity 0) doesn't open it");
	Expect(!OpensWith("a lock", 5, 0, true, 0, 1.0, Moment::AllIn), "hold or crack: a decided hand (equity 1) doesn't open it");
	Expect(OpensWith("equity unknown", 5, 0, true, 0, -1.0, Moment::AllIn), "hold or crack: an unknown equity still opens it (cards to come)");
	Expect(!OpensWith("a big pot", 5, 0, true, 0, 0.64, Moment::BigPot), "hold or crack: only the all-in opens it");
	// Not live: before Begin nothing opens.
	{
		FakeRoom Room;
		Room.Here = {"fan_1", "fan_2", "fan_3"};
		PlayState P;
		play::OnMoment(P, Room, Ch, PlayIn(W0), Moment::AllIn);
		Expect(!P.HoC.Open, "hold or crack: nothing opens before the stream begins");
	}
}

void HocCallsAndReveal()
{
	const double W0 = 21.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(60, W0);
	const std::string MoneyBefore = PlayMoney(Ch);
	// A small room: everyone named calls, once each, and nobody calls twice.
	{
		FakeRoom Room;
		Room.MinuteV = W0;
		Room.Here = {PlayDee, PlayMei, "fan_3"};
		Room.Extra = 2;
		PlayState P;
		ss::Rng R("kastplay:small");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 0.3), Moment::AllIn);
		PlayChat(P, Room, W0 + 0.3, 4.0, 1.0 / 60.0, R);
		View V;
		play::Fill(P, V);
		Expect(V.HoC.Holds + V.HoC.Cracks == 3, "hold or crack: in a room of 3 named people, all 3 call it (lurkers never do)");
		Expect(V.HoC.Equity == 0.64, "hold or crack: the bar's marker is the hero's real equity");
		std::set<std::string> Callers;
		for (const PlayState::Call& C : P.HoC.Calls)
		{
			Callers.insert(C.Who);
		}
		Expect(Callers.size() == P.HoC.Calls.size(), "hold or crack: nobody calls it twice");
		int CallLines = 0;
		for (const FakeRoom::Said& S : Room.Lines)
		{
			CallLines += S.Kind == LineKind::Chat && !S.Who.empty() ? 1 : 0;
		}
		Expect(CallLines == V.HoC.Holds + V.HoC.Cracks, "hold or crack: every call counted is a line in chat from its caller");
		// The streamer calls it once.
		Expect(play::CallIt(P, Room, Ch, true), "hold or crack: the streamer can call it while it's open");
		Expect(std::abs(Room.EngageV - 0.3) < 1e-12, "hold or crack: the streamer's call is Engage +0.3");
		Expect(!play::CallIt(P, Room, Ch, false), "hold or crack: the streamer calls it once");
		play::Fill(P, V);
		Expect(V.HoC.Called == 1, "hold or crack: the view carries the streamer's call");
		// A reveal from another table changes nothing; the front table's closes it with Hype +8 and KastCall.
		Inputs Other = PlayIn(W0 + 0.5);
		Other.ThisTable = 2;
		play::OnMoment(P, Room, Ch, Other, Moment::WonAllIn);
		Expect(P.HoC.Open && Room.HypeV == 0.0, "hold or crack: another table's result doesn't reveal it");
		const size_t NoticesBefore = Room.Notices.size();
		const std::vector<PlayState::Call> CallsAtReveal = P.HoC.Calls;
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 0.5), Moment::WonAllIn);
		Expect(!P.HoC.Open, "hold or crack: the front table's result reveals it");
		Expect(Room.HypeV == 8.0, "hold or crack: the reveal is Hype +8");
		Expect(Room.Notices.size() == NoticesBefore + 1 && Room.Notices.back().Sound == static_cast<int>(ss::SoundId::KastCall),
			"hold or crack: the reveal plays KastCall through the cue queue");
		const FakeRoom::Said* Result = Room.LastSystem();
		int Right = 0;
		int Points = 0;
		for (const PlayState::Call& C : CallsAtReveal)
		{
			Right += C.Holds ? 1 : 0;
			Points += C.Holds ? C.Points : 0;
		}
		Expect(Result && Result->Text == kcopy::RevealLine(true, Right, static_cast<int>(CallsAtReveal.size()), Points),
			"hold or crack: a won all-in reveals HOLDS with chat's own counts and points");
		int Tallied = 0;
		for (const PlayState::Caller& C : P.Callers)
		{
			Tallied += C.Right;
		}
		Expect(Tallied == Right && P.Callers.size() == CallsAtReveal.size(), "hold or crack: the night's tally takes every call");
		play::Fill(P, V);
		Expect(!V.HoC.Open && V.Action != View::Slot::HoldCrack, "hold or crack: the slot frees after the reveal");
		PlayChat(P, Room, W0 + 0.6, 3.0, 1.0 / 60.0, R);
		Expect(Room.Refused == 0, "hold or crack: every caller and every cheer was in the room when they spoke");
		PlayRefusedTotal += Room.Refused;
	}
	// The reveal's words: counts under 10 callers, a share from 10, "Nobody" when nobody called it right; points are chat's.
	{
		FakeRoom Room;
		Room.Here = {"kat_v", "nitbox", PlayDee};
		PlayState P;
		play::Begin(P, Ch, PlayIn(W0));
		const auto Reveal = [&](std::vector<std::pair<std::string, bool>> Calls, Moment M) {
			play::OnMoment(P, Room, Ch, PlayIn(W0 + 1.0), Moment::AllIn);
			P.HoC.Queued = true; // the calls below, not chat's draws
			for (size_t I = 0; I < Calls.size(); ++I)
			{
				PlayState::Call C;
				C.Who = Calls[I].first;
				C.Holds = Calls[I].second;
				C.Points = 100 * static_cast<int>(I + 1);
				P.HoC.Calls.push_back(C);
			}
			play::OnMoment(P, Room, Ch, PlayIn(W0 + 1.2), M);
			const FakeRoom::Said* Line = Room.LastSystem();
			return Line ? Line->Text : std::string();
		};
		// The numbers are the games' (who called what, the points staked by the right ones); the words are KastCopy's.
		const std::string Few = Reveal({{"kat_v", true}, {"nitbox", false}, {PlayDee, true}}, Moment::WonAllIn);
		Expect(Few == kcopy::RevealLine(true, 2, 3, 400) && Few.find("2 of 3") != std::string::npos && Few.find("400") != std::string::npos,
			"hold or crack: under 10 callers the reveal counts them (\"2 of 3 called it\": " + Few + ")");
		std::vector<std::pair<std::string, bool>> Seventeen;
		for (int I = 0; I < 17; ++I)
		{
			Seventeen.emplace_back(I == 0 ? std::string("kat_v") : "caller_" + std::to_string(I), I < 13);
		}
		const std::string Share = Reveal(Seventeen, Moment::WonAllIn);
		Expect(Share == kcopy::RevealLine(true, 13, 17, 9100) && Share.find("76%") != std::string::npos && Share.find("9,100") != std::string::npos,
			"hold or crack: from 10 callers the reveal is a share (13 of 17: " + Share + ")");
		Expect(Reveal({{"kat_v", false}, {"nitbox", true}}, Moment::BadBeat) == kcopy::RevealLine(false, 1, 2, 100), "hold or crack: a bad beat reveals CRACKS");
		const std::string None = Reveal({{"kat_v", true}, {"nitbox", true}}, Moment::LostAllIn);
		Expect(None == kcopy::RevealLine(false, 0, 2, 0) && None.find('%') == std::string::npos, "hold or crack: when nobody called it right the reveal says so");
		Expect(Reveal({{"kat_v", false}}, Moment::LostAllIn) == kcopy::RevealLine(false, 1, 1, 100), "hold or crack: one caller is counted too");
		Summary S;
		play::Close(P, S);
		Expect(S.BestCaller == "kat_v" && S.BestCalls == 4 && S.BestCallsOf == 5, "hold or crack: the end card's best caller (\"kat_v called 4 of 5\")");
		Expect(Room.HypeV == 5.0 * 8.0, "hold or crack: each reveal is Hype +8");
	}
	// The best caller: the most right, then the fewest wrong, then the first; nobody right, nobody named.
	{
		PlayState Q;
		Q.Callers = {{"spammy", 2, 6}, {"kat_v", 4, 5}, {"tie_a", 4, 7}, {"dee", 3, 3}, {"twin", 4, 5}};
		Summary S;
		play::Close(Q, S);
		Expect(S.BestCaller == "kat_v" && S.BestCalls == 4 && S.BestCallsOf == 5, "hold or crack: the best caller is the most right, then the fewest wrong, then the first");
		PlayState None;
		None.Callers = {{"nitbox", 0, 3}};
		Summary S2;
		play::Close(None, S2);
		Expect(S2.BestCaller.empty() && S2.BestCalls == 0, "hold or crack: with nobody right there's no best caller");
	}
	// A big room: a share of it calls (each once, each present), fast enough to land while the board runs.
	{
		FakeRoom Room;
		for (int I = 0; I < 60; ++I)
		{
			Room.Here.push_back("fan_" + std::to_string(I));
		}
		PlayState P;
		ss::Rng R("kastplay:big");
		play::Begin(P, Ch, PlayIn(W0));
		Inputs In = PlayIn(W0 + 2.0);
		In.AllInEquity = 0.81;
		play::OnMoment(P, Room, Ch, In, Moment::AllIn);
		// Halfway through, ten callers-to-be leave: none of them speaks after leaving.
		PlayChat(P, Room, W0 + 2.0, 1.0, 1.0 / 60.0, R);
		std::vector<std::string> Gone(Room.Here.begin(), Room.Here.begin() + 10);
		Room.Here.erase(Room.Here.begin(), Room.Here.begin() + 10);
		const size_t LinesAtLeave = Room.Lines.size();
		PlayChat(P, Room, W0 + 2.1, 4.0, 1.0 / 60.0, R);
		bool GoneQuiet = true;
		for (size_t I = LinesAtLeave; I < Room.Lines.size(); ++I)
		{
			GoneQuiet = GoneQuiet && std::find(Gone.begin(), Gone.end(), Room.Lines[I].Who) == Gone.end();
		}
		Expect(GoneQuiet, "hold or crack: a caller who left never calls (R1)");
		const int Calls = static_cast<int>(P.HoC.Calls.size());
		PlayBigCalls = Calls;
		Expect(Calls >= 15 && Calls <= 50, "hold or crack: a share of a 60-name room calls it (" + std::to_string(Calls) + ")");
		int Holds = 0;
		for (const PlayState::Call& C : P.HoC.Calls)
		{
			Holds += C.Holds ? 1 : 0;
		}
		Expect(Holds * 2 > Calls, "hold or crack: at 81% equity most of chat says HOLDS (" + std::to_string(Holds) + " of " + std::to_string(Calls) + ")");
		PlayBigHolds = Holds;
		Expect(Room.Refused == 0, "hold or crack: no call was offered for someone not present");
		PlayRefusedTotal += Room.Refused;
	}
	// No callers (only lurkers watching): the reveal's Hype is the same, and nothing is said for nobody.
	{
		FakeRoom Room;
		Room.Extra = 3;
		PlayState P;
		ss::Rng R("kastplay:lurkers");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 3.0), Moment::AllIn);
		Expect(P.HoC.Open, "hold or crack: 3 lurkers are 3 watching");
		PlayChat(P, Room, W0 + 3.0, 3.0, 1.0 / 60.0, R);
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 3.3), Moment::WonAllIn);
		Expect(Room.HypeV == 8.0 && Room.Lines.empty() && Room.Notices.empty(), "hold or crack: with nobody named the reveal is Hype +8, silent");
	}
	// A reveal that never comes: closed after 5 world minutes, no result, no Hype.
	{
		FakeRoom Room;
		Room.Here = {"fan_1", "fan_2"};
		PlayState P;
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.0), Moment::AllIn);
		play::OnMinute(P, Room, Ch, PlayIn(W0 + 8.0), 8); // the grid's index: stream minutes since Begin's world minute
		Expect(P.HoC.Open, "hold or crack: still open 4 world minutes in");
		play::OnMinute(P, Room, Ch, PlayIn(W0 + 9.0), 9);
		Expect(!P.HoC.Open && Room.HypeV == 0.0, "hold or crack: a reveal that never came closes at 5 world minutes, no Hype");
		Expect(!play::CallIt(P, Room, Ch, true), "hold or crack: nothing to call once it's closed");
	}
	// A second all-in before any reveal (review: the first one's result never came): a fresh Hold or Crack, its bar empty, the
	// streamer's call free again, and the reveal tallies only the fresh calls.
	{
		FakeRoom Room;
		Room.Here = {PlayDee, PlayMei, "fan_3", "fan_4"};
		PlayState P;
		ss::Rng R("kastplay:again");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 5.0), Moment::AllIn);
		PlayChat(P, Room, W0 + 5.0, 4.0, 1.0 / 60.0, R);
		play::CallIt(P, Room, Ch, true);
		Expect(P.HoC.Calls.size() == 4 && P.HoC.Called == 1, "hold or crack: the first all-in's four calls and the streamer's (the fresh-open check's start)");
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 5.5), Moment::AllIn);
		View V;
		play::Fill(P, V);
		Expect(V.HoC.Open && V.HoC.Holds + V.HoC.Cracks == 0 && V.HoC.Called == 0 && !P.HoC.Queued,
			"hold or crack: a new all-in opens a fresh one (no calls carried over, the streamer can call again)");
		PlayChat(P, Room, W0 + 5.5, 4.0, 1.0 / 60.0, R);
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 6.0), Moment::WonAllIn);
		int Of = 0;
		for (const PlayState::Caller& C : P.Callers)
		{
			Of += C.Of;
		}
		Expect(Of == 4 && P.Callers.size() == 4, "hold or crack: the reveal tallies the fresh calls only (" + std::to_string(Of) + " calls)");
		PlayRefusedTotal += Room.Refused;
	}
	Expect(PlayMoney(Ch) == MoneyBefore, "hold or crack: chat's calls and the reveals move no money and no member");
}

/** Heads-down mid-runout (§2.9, §1.4, R3, R4; the C-K audit's L2): the pace goes to watch 4x, 8x, a skip or a sprint while
 *  Hold or Crack is open. The bar and its call buttons go at once and a press is refused with no Engage (the streamer's say in
 *  it is all the pace takes away); chat's game runs on to its reveal as at any pace (Hype +8, KastCall, the result line: the
 *  room never reads the pace), and back at an attended pace before the reveal the bar shows again. One night, with or without
 *  the drop, display time sliced by Step: what it did to the room. */
std::string HocHeadsDownNight(double Step, const char* RSeed, bool Drop, int& Shown)
{
	const double W0 = 21.5 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(10, W0);
	FakeRoom Room;
	Room.MinuteV = W0 + 4.0;
	Room.Here = {PlayDee, PlayMei, "fan_1", "fan_2", "fan_3", "fan_4"};
	PlayState P;
	ss::Rng R(RSeed);
	play::Begin(P, Ch, PlayIn(W0));
	Shown = 0;
	play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.0), Moment::AllIn);
	PlayChat(P, Room, W0 + 4.0, 1.0, Step, R);
	Room.AttendedV = !Drop;
	PlayChat(P, Room, W0 + 4.1, 1.0, Step, R, !Drop);
	if (Drop)
	{
		View V;
		play::Fill(P, V);
		Shown += V.HoC.Open || V.Action == View::Slot::HoldCrack ? 1 : 0;
		play::CallIt(P, Room, Ch, true); // refused
	}
	Room.MinuteV = W0 + 5.0;
	play::OnMinute(P, Room, Ch, PlayIn(W0 + 5.0, !Drop), 5);
	play::OnMoment(P, Room, Ch, PlayIn(W0 + 5.2, !Drop), Moment::WonAllIn);
	PlayChat(P, Room, W0 + 5.3, 2.0, Step, R, !Drop);
	PlayRefusedTotal += Room.Refused;
	std::ostringstream O;
	O << Room.Effects() << " |K " << Room.Notices.size() << " |O " << P.HoC.Open;
	return O.str();
}

void HocHeadsDown()
{
	const double W0 = 21.5 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(10, W0);
	const std::string MoneyBefore = PlayMoney(Ch);
	const auto Room6 = [&]() {
		FakeRoom Room;
		Room.MinuteV = W0 + 4.0;
		Room.Here = {PlayDee, PlayMei, "fan_1", "fan_2", "fan_3", "fan_4"};
		return Room;
	};
	// Open at Smart, chat calling; the pace drops to 8x mid-runout, then comes back.
	{
		FakeRoom Room = Room6();
		PlayState P;
		ss::Rng R("kastplay:headsdown");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.0), Moment::AllIn);
		PlayChat(P, Room, W0 + 4.0, 0.5, 1.0 / 60.0, R);
		View V;
		play::Fill(P, V);
		Expect(V.HoC.Open && V.Action == View::Slot::HoldCrack && !P.HoC.Calls.empty(), "hold or crack heads-down: open at Smart with chat calling (the check's start)");
		Room.AttendedV = false;
		PlayChat(P, Room, W0 + 4.05, 1.5, 1.0 / 60.0, R, false);
		play::Fill(P, V);
		Expect(P.HoC.Open && !V.HoC.Open && V.Action != View::Slot::HoldCrack && V.HoC.Holds + V.HoC.Cracks == 0 && V.HoC.Called == 0,
			"hold or crack heads-down: the bar and the call buttons go the moment the pace drops (inside the world minute)");
		const size_t LinesBefore = Room.Lines.size();
		Expect(!play::CallIt(P, Room, Ch, true) && !play::CallIt(P, Room, Ch, false) && Room.EngageV == 0.0 && Room.Lines.size() == LinesBefore && P.HoC.Called == 0,
			"hold or crack heads-down: a press is refused (no Engage, no streamer's line, no call booked)");
		// A heads-down grid minute leaves the game alone (the room never reads the pace); back at Smart the bar shows again.
		Room.MinuteV = W0 + 5.0;
		play::OnMinute(P, Room, Ch, PlayIn(W0 + 5.0, false), 5);
		Expect(P.HoC.Open, "hold or crack heads-down: a heads-down grid minute leaves chat's game open");
		Room.AttendedV = true;
		PlayChat(P, Room, W0 + 5.1, 0.2, 1.0 / 60.0, R);
		play::Fill(P, V);
		Expect(V.HoC.Open && V.Action == View::Slot::HoldCrack && V.HoC.Holds + V.HoC.Cracks == static_cast<int>(P.HoC.Calls.size()),
			"hold or crack heads-down: back at an attended pace before the reveal, it shows again with chat's calls");
		Expect(play::CallIt(P, Room, Ch, false) && std::abs(Room.EngageV - 0.3) < 1e-12, "hold or crack heads-down: and the call works again (Engage +0.3)");
		PlayRefusedTotal += Room.Refused;
	}
	// The reveal comes at a heads-down moment: chat's game reveals as at any pace.
	{
		FakeRoom Room = Room6();
		PlayState P;
		ss::Rng R("kastplay:headsdown:reveal");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.0), Moment::AllIn);
		PlayChat(P, Room, W0 + 4.0, 1.0, 1.0 / 60.0, R);
		Expect(play::CallIt(P, Room, Ch, true), "hold or crack heads-down: the streamer called it at Smart");
		Room.AttendedV = false;
		PlayChat(P, Room, W0 + 4.1, 0.5, 1.0 / 60.0, R, false);
		{
			// Heads-down the View's Hold or Crack is all defaults (review): the streamer's call and the equity don't leak past Open.
			View V;
			play::Fill(P, V);
			const View::HoldCrack Blank;
			Expect(P.HoC.Open && P.HoC.Called == 1 && !V.HoC.Open && V.HoC.Holds == Blank.Holds && V.HoC.Cracks == Blank.Cracks &&
					   V.HoC.Equity == Blank.Equity && V.HoC.Called == Blank.Called && V.Action == View::Slot::None,
				"hold or crack heads-down: after a call at Smart, the View's Hold or Crack is all defaults (no call, no equity, no slot)");
		}
		const std::vector<PlayState::Call> CallsAtReveal = P.HoC.Calls;
		int Right = 0;
		int Points = 0;
		for (const PlayState::Call& C : CallsAtReveal)
		{
			Right += C.Holds ? 1 : 0;
			Points += C.Holds ? C.Points : 0;
		}
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.2, false), Moment::WonAllIn);
		const FakeRoom::Said* Result = Room.LastSystem();
		Expect(!CallsAtReveal.empty() && !P.HoC.Open && Room.HypeV == 8.0 && Room.Notices.size() == 1 && Room.Notices[0].Sound == static_cast<int>(ss::SoundId::KastCall) &&
				   Result && Result->Text == kcopy::RevealLine(true, Right, static_cast<int>(CallsAtReveal.size()), Points) && P.Callers.size() == CallsAtReveal.size(),
			"hold or crack heads-down: a reveal at a heads-down moment is chat's as at any pace (Hype +8, KastCall, the result, the tally)");
		Expect(std::abs(Room.EngageV - 0.3) < 1e-12, "hold or crack heads-down: the streamer's call at Smart is the only Engage it gave");
		PlayChat(P, Room, W0 + 4.3, 3.0, 1.0 / 60.0, R, false);
		Room.AttendedV = true;
		PlayChat(P, Room, W0 + 4.5, 0.5, 1.0 / 60.0, R);
		View V;
		play::Fill(P, V);
		Expect(!V.HoC.Open && V.Action == View::Slot::None, "hold or crack heads-down: the slot is free after the reveal");
		PlayRefusedTotal += Room.Refused;
	}
	// The pace changes only the streamer (R3, R4): the same night with and without the drop moves the room the same way (the
	// streamer's refused press is the only difference, and it gives nothing), and the effects don't read the frame rate.
	{
		int ShownA = 0;
		int ShownB = 0;
		int ShownC = 0;
		int ShownD = 0;
		const std::string A = HocHeadsDownNight(1.0 / 60.0, "kastplay:hd:a", true, ShownA);
		const std::string B = HocHeadsDownNight(1.0 / 30.0, "kastplay:hd:b", true, ShownB);
		const std::string C = HocHeadsDownNight(0.5, "kastplay:hd:c", true, ShownC);
		const std::string D = HocHeadsDownNight(1.0 / 60.0, "kastplay:hd:a", false, ShownD);
		Expect(A == B && A == C && ShownA + ShownB + ShownC == 0, "hold or crack heads-down: the room's effects are the same at 60, 30 and 2 fps, the bar never shown (" + A + ")");
		Expect(A == D && A.find("H 8 ") != std::string::npos, "hold or crack heads-down: the drop moves the room exactly as the attended night does (" + D + ")");
		if (A != B || A != C || A != D)
		{
			std::printf("  60 fps: %s\n  30 fps: %s\n  2 fps: %s\n  attended: %s\n", A.c_str(), B.c_str(), C.c_str(), D.c_str());
		}
	}
	Expect(PlayMoney(Ch) == MoneyBefore, "hold or crack heads-down: moves no money and no member");
}

// ------------------------------------------------------------------ Nod

void NodChecks()
{
	const double W0 = 22.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(20, W0);
	FakeRoom Room;
	Room.MinuteV = W0 + 12.0;
	Room.Here = {PlayDee, "fan_4", "guest_a", "guest_b", "guest_c"};
	PlayState P;
	ss::Rng R("kastplay:nod");
	play::Begin(P, Ch, PlayIn(W0));
	P.World = W0 + 12.2;
	Expect(!play::Nod(P, Room, Ch, "fan_9"), "nod: someone who isn't here can't be Nodded");
	Expect(!play::Nod(P, Room, Ch, std::string()), "nod: a system line can't be Nodded");
	Member* Fan = Ch.FindMember("fan_4");
	const double AffinityBefore = Fan->Affinity;
	Expect(play::Nod(P, Room, Ch, "fan_4"), "nod: a member here can be Nodded");
	Expect(Fan->Noticed && Fan->Affinity == growth::AffinityBump(AffinityBefore, 0.03), "nod: a member is Noticed once (Affinity +3% of the gap)");
	Expect(Room.Noticed.size() == 1 && Room.Noticed[0] == "fan_4", "nod: a member's loyalty runs x1.6 tonight (the Room's MarkNoticed)");
	Expect(!play::Nod(P, Room, Ch, "fan_4"), "nod: one a person a stream");
	// A guest follows on its keyed draw: Unit(Seed, Stream, Nod, NameKey, the room's minute) < 0.10 x Sat.
	const double Bar = 0.10 * growth::Sat(Ch.Followers);
	const bool GuestFollows = growth::Unit(PlaySeed, 7, growth::Tag::Nod, growth::NameKey("guest_a"), static_cast<uint64_t>(W0 + 12.0)) < Bar;
	Expect(play::Nod(P, Room, Ch, "guest_a"), "nod: a guest here can be Nodded");
	Expect((std::find(Room.Follows.begin(), Room.Follows.end(), "guest_a") != Room.Follows.end()) == GuestFollows,
		"nod: a Nodded guest follows exactly when its keyed draw is under 0.10 x Sat");
	Expect(Room.Noticed.size() == 1, "nod: a guest isn't Noticed (loyalty x1.6 tonight is a member's; review)");
	Expect(play::Nod(P, Room, Ch, PlayDee), "nod: the third Nod");
	Expect(!play::Nod(P, Room, Ch, "guest_b"), "nod: three a stream");
	View V;
	play::Fill(P, V);
	Expect(V.NodsLeft == 0, "nod: the view counts the Nods left");
	Expect(V.Lit.size() == 3 && std::find(V.Lit.begin(), V.Lit.end(), "guest_a") != V.Lit.end(), "nod: a Nodded name lights on the overlay");
	Expect(play::WasNodded(P, "fan_4") && play::WasNodded(P, "guest_a") && !play::WasNodded(P, "guest_b"), "nod: WasNodded for the Room's member flags");
	// The replies come from the Nodded people themselves, once each, while they're here; Dee leaves first and says nothing.
	Room.Here.erase(std::find(Room.Here.begin(), Room.Here.end(), PlayDee));
	PlayChat(P, Room, W0 + 12.4, 2.0, 1.0 / 60.0, R);
	Expect(Room.LinesFrom("fan_4") == 1 && Room.LinesFrom("guest_a") == 1 && Room.LinesFrom(PlayDee) == 0,
		"nod: each Nodded person answers once, and only if they're still here");
	// A world minute later the names go dark: on the display clock at once (a frame inside the grid minute), and the list
	// empties at the next grid minute.
	PlayChat(P, Room, W0 + 13.5, 0.1, 1.0 / 60.0, R);
	play::Fill(P, V);
	Expect(V.Lit.empty() && P.Lit.size() == 3, "nod: the glow lasts a world minute (dark on the next frame past it)");
	play::OnMinute(P, Room, Ch, PlayIn(W0 + 14.0), 14);
	play::Fill(P, V);
	Expect(V.Lit.empty() && P.Lit.empty(), "nod: the grid minute clears the dark names");
	// Noticed is once ever: a second stream's Nod doesn't bump Affinity again.
	const double AffinityOnce = Fan->Affinity;
	play::Begin(P, Ch, PlayIn(W0 + PlayDay));
	Room.Here.push_back("fan_4");
	Expect(play::Nod(P, Room, Ch, "fan_4") && Fan->Affinity == AffinityOnce, "nod: a new stream has three Nods; Noticed's bump is once ever");
	Summary S;
	play::Close(P, S);
	Expect(!play::Nod(P, Room, Ch, "guest_c"), "nod: nothing after the stream ends");
	PlayRefusedTotal += Room.Refused;
	// The rate: 4,000 guests across streams follow at 0.10 x Sat (Sat 1 at 0 followers), within three binomial deviations.
	{
		Channel Empty;
		int Followed = 0;
		const int Guests = 4000;
		for (int I = 0; I < Guests; ++I)
		{
			FakeRoom G;
			G.StreamV = static_cast<uint64_t>(1 + I % 40);
			G.MinuteV = 100.0 + static_cast<double>(I % 97);
			G.Here = {"rate_guest_" + std::to_string(I)};
			PlayState Q;
			play::Begin(Q, Empty, PlayIn(G.MinuteV));
			play::Nod(Q, G, Empty, G.Here[0]);
			Followed += static_cast<int>(G.Follows.size());
		}
		const double Rate = static_cast<double>(Followed) / Guests;
		PlayNodRate = Rate;
		Expect(std::abs(Rate - 0.10) < 0.015, "nod: Nodded guests follow at 0.10 at 0 followers (" + std::to_string(Rate) + ")");
	}
	// The key is the room's grid minute, never the display clock: 400 guests Nodded with display time 3.7 minutes ahead.
	{
		Channel Empty;
		int Agree = 0;
		for (int I = 0; I < 400; ++I)
		{
			FakeRoom G;
			G.MinuteV = 500.0 + static_cast<double>(I);
			const std::string Name = "key_guest_" + std::to_string(I);
			G.Here = {Name};
			PlayState Q;
			play::Begin(Q, Empty, PlayIn(G.MinuteV));
			Q.World = G.MinuteV + 3.7;
			play::Nod(Q, G, Empty, Name);
			const bool Oracle = growth::Unit(PlaySeed, G.StreamV, growth::Tag::Nod, growth::NameKey(Name), static_cast<uint64_t>(G.MinuteV)) < 0.10;
			Agree += (G.Follows.size() == 1) == Oracle ? 1 : 0;
		}
		Expect(Agree == 400, "nod: a guest's follow is keyed on the room's minute, not the display clock");
	}
	// Sat scales the bar (review): on a 3,000-follower channel a Nodded guest follows exactly when the draw is under
	// 0.10 x Sat(3,000) (about 0.064); the guests whose draw falls between that and 0.10 are the ones Sat turns away.
	{
		Channel Big;
		Big.Followers = 3000;
		const double BigBar = 0.10 * growth::Sat(Big.Followers);
		int Agree = 0;
		int Between = 0;
		for (int I = 0; I < 600; ++I)
		{
			FakeRoom G;
			G.MinuteV = 900.0 + static_cast<double>(I % 50);
			const std::string Name = "sat_guest_" + std::to_string(I);
			G.Here = {Name};
			PlayState Q;
			play::Begin(Q, Big, PlayIn(G.MinuteV));
			play::Nod(Q, G, Big, Name);
			const double U = growth::Unit(PlaySeed, G.StreamV, growth::Tag::Nod, growth::NameKey(Name), static_cast<uint64_t>(G.MinuteV));
			Between += U >= BigBar && U < 0.10 ? 1 : 0;
			Agree += (G.Follows.size() == 1) == (U < BigBar) ? 1 : 0;
		}
		Expect(BigBar < 0.07 && Between > 0 && Agree == 600,
			"nod: a guest's bar is 0.10 x Sat (3,000 followers: " + std::to_string(BigBar) + ", " + std::to_string(Between) + " of 600 turned away by Sat)");
	}
}

// ------------------------------------------------------------------ Welcome the raid, Set the scene

void WelcomeAndScene()
{
	const double W0 = 23.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(10, W0);
	{
		FakeRoom Room;
		Room.MinuteV = W0 + 30.0;
		Room.Here = {PlayDee, "fan_2", "raider_1"};
		PlayState P;
		ss::Rng R("kastplay:welcome");
		play::Begin(P, Ch, PlayIn(W0));
		Expect(!play::WelcomeRaid(P, Room, Ch), "welcome the raid: nothing to welcome before a raid");
		Room.RaidAtV = W0 + 30.0;
		PlayChat(P, Room, W0 + 30.2, 0.5, 1.0 / 60.0, R);
		View V;
		play::Fill(P, V);
		Expect(V.Action == View::Slot::WelcomeRaid, "welcome the raid: the slot offers it for the raid's first 2 world minutes");
		Room.AttendedV = false;
		Expect(!play::WelcomeRaid(P, Room, Ch), "welcome the raid: not at a heads-down minute");
		PlayChat(P, Room, W0 + 30.3, 0.5, 1.0 / 60.0, R, false);
		play::Fill(P, V);
		Expect(V.Action == View::Slot::None, "welcome the raid: heads-down, the button isn't drawn");
		Room.AttendedV = true;
		PlayChat(P, Room, W0 + 31.5, 0.5, 1.0 / 60.0, R);
		Expect(play::WelcomeRaid(P, Room, Ch), "welcome the raid: within 2 world minutes, attended");
		Expect(Room.Welcomes.size() == 1 && Room.Welcomes[0].first == 1.3 && Room.Welcomes[0].second == 1.5, "welcome the raid: stays x1.3, follows x1.5");
		Expect(!play::WelcomeRaid(P, Room, Ch), "welcome the raid: once a raid");
		Expect(Room.Said_(kcopy::WelcomeRaidStreamer()) && Room.Lines.back().Who == "<streamer>", "welcome the raid: the streamer says hi");
		play::Fill(P, V);
		Expect(V.Action == View::Slot::None, "welcome the raid: the slot frees once welcomed");
		// The next raid can be welcomed, but not past its window.
		Room.RaidAtV = W0 + 50.0;
		Room.MinuteV = W0 + 52.0;
		PlayChat(P, Room, W0 + 52.0, 0.5, 1.0 / 60.0, R);
		Expect(!play::WelcomeRaid(P, Room, Ch), "welcome the raid: 2 world minutes after it, too late");
		Room.RaidAtV = W0 + 60.0;
		Room.MinuteV = W0 + 60.0;
		PlayChat(P, Room, W0 + 60.5, 0.5, 1.0 / 60.0, R);
		Room.Raiders = {"raider_gone", "raider_1"};
		Expect(play::WelcomeRaid(P, Room, Ch) && Room.Welcomes.size() == 2, "welcome the raid: a new raid, a new welcome");
		PlayChat(P, Room, W0 + 60.6, 3.0, 1.0 / 60.0, R);
		const std::vector<std::string>& Thanks = kcopy::WelcomedChat();
		Expect(Room.LinesFrom("raider_1") == 1 && std::find(Thanks.begin(), Thanks.end(), Room.Lines.back().Text) != Thanks.end() &&
				   Room.LinesFrom("raider_gone") == 0 && Room.LinesFrom(PlayDee) == 0,
			"welcome the raid: a raider who's here says thanks (nobody else, nobody gone)");
		Expect(Room.Refused == 0, "welcome the raid: every line from someone here");
		PlayRefusedTotal += Room.Refused;
	}
	{
		FakeRoom Room;
		Room.StreamV = 1;
		Room.MinuteV = W0 + 3.0;
		PlayState P;
		ss::Rng R("kastplay:scene");
		play::Begin(P, Ch, PlayIn(W0));
		Room.Here = {"fan_1"};
		PlayChat(P, Room, W0 + 3.0, 0.2, 1.0 / 60.0, R);
		View V;
		play::Fill(P, V);
		Expect(!V.SceneOpen && !play::SetTheScene(P, Room, Ch), "set the scene: only at a true 0");
		Room.Here.clear();
		Room.AttendedV = false;
		PlayChat(P, Room, W0 + 3.1, 0.2, 1.0 / 60.0, R, false);
		play::Fill(P, V);
		Expect(!V.SceneOpen && !play::SetTheScene(P, Room, Ch), "set the scene: not heads-down");
		Room.AttendedV = true;
		PlayChat(P, Room, W0 + 3.2, 0.2, 1.0 / 60.0, R);
		play::Fill(P, V);
		Expect(V.SceneOpen, "set the scene: offered at a true 0 on stream 1, attended");
		Expect(play::SetTheScene(P, Room, Ch), "set the scene: pressed");
		Expect(Room.Banks.size() == 1 && Room.Banks[0].first == 3 && Room.Banks[0].second == 1.2, "set the scene: the next 3 arrivals stay x1.2");
		Expect(Room.Said_(kcopy::SetTheScene()) && Room.Lines.back().Who.empty(), "set the scene: its line, a system line");
		PlayChat(P, Room, W0 + 3.3, 0.2, 1.0 / 60.0, R);
		play::Fill(P, V);
		Expect(!V.SceneOpen && !play::SetTheScene(P, Room, Ch), "set the scene: once a stream");
		PlayState Late;
		play::Begin(Late, Ch, PlayIn(W0));
		Room.StreamV = 21;
		Expect(!play::SetTheScene(Late, Room, Ch), "set the scene: streams 1-20 only");
		Room.StreamV = 20;
		Expect(play::SetTheScene(Late, Room, Ch), "set the scene: stream 20 still has it");
		// The button follows the press's rule (review): an empty, attended room offers it on stream 20 and not on stream 21, so
		// Studio never draws a button that refuses.
		for (const uint64_t StreamNo : {uint64_t{21}, uint64_t{20}})
		{
			FakeRoom Quiet;
			Quiet.StreamV = StreamNo;
			Quiet.MinuteV = W0 + 3.0;
			PlayState Q;
			play::Begin(Q, Ch, PlayIn(W0));
			PlayChat(Q, Quiet, W0 + 3.0, 0.2, 1.0 / 60.0, R);
			View Vq;
			play::Fill(Q, Vq);
			const bool Offered = Vq.SceneOpen;
			Expect(Offered == (StreamNo <= 20) && Offered == play::SetTheScene(Q, Quiet, Ch),
				"set the scene: the button shows exactly when the press works (stream " + std::to_string(StreamNo) + ")");
		}
	}
}

// ------------------------------------------------------------------ Rail Call

void RailCallChecks()
{
	const double W0 = 24.0 * PlayDay + 21.0 * 60.0;
	const int Today = ss::net::DayOf(W0);
	Channel Ch = PlayChannel(80, W0);
	FakeRoom Room;
	Room.MinuteV = W0 + 100.0;
	Room.SchedV = 1.3;
	Room.ToDV = 1.0;
	for (int I = 0; I < 20; ++I)
	{
		Room.Here.push_back("fan_" + std::to_string(I));
	}
	for (int I = 20; I < 30; ++I)
	{
		Room.AttendingAway.insert("fan_" + std::to_string(I));
	}
	PlayState P;
	ss::Rng R("kastplay:rail");
	play::Begin(P, Ch, PlayIn(W0));
	Inputs Shallow = PlayIn(W0 + 100.0);
	Shallow.DeepEntrants = 800;
	Shallow.Depth = 0.3;
	play::OnMinute(P, Room, Ch, Shallow, 100);
	Expect(!P.RailOpen, "rail call: not offered below depth 0.4");
	Inputs Small = Shallow;
	Small.DeepEntrants = 60;
	Small.Depth = 0.8;
	play::OnMinute(P, Room, Ch, Small, 101);
	Expect(!P.RailOpen, "rail call: not for a field under 100 (the hourlies' \"final tables\")");
	Inputs Deep = Shallow;
	Deep.Depth = 0.45;
	Deep.Attended = false;
	play::OnMinute(P, Room, Ch, Deep, 102);
	Expect(!P.RailOpen, "rail call: not offered at a heads-down minute");
	Deep.Attended = true;
	Deep.World = W0 + 103.0;
	play::OnMinute(P, Room, Ch, Deep, 103);
	Expect(P.RailOpen && !P.WholeRail, "rail call: offered at the first attended minute at depth 0.4");
	View V;
	P.Attended = true;
	play::Fill(P, V);
	Expect(V.Action == View::Slot::RailCall, "rail call: the action slot shows RAIL CALL");
	// Heads-down it stays offered but isn't drawn, and can't be pressed.
	Inputs DeepDown = Deep;
	DeepDown.Attended = false;
	DeepDown.World = W0 + 104.0;
	play::OnMinute(P, Room, Ch, DeepDown, 104);
	play::Fill(P, V);
	Expect(P.RailOpen && V.Action == View::Slot::None, "rail call: heads-down it stays offered, not drawn");
	Expect(!play::RailCall(P, Room, Ch, DeepDown), "rail call: heads-down it can't be pressed");
	// The press: every follower not attending (Dee and Mei included, watchers who never followed not) rolls its keyed draw.
	Room.Here.erase(std::find(Room.Here.begin(), Room.Here.end(), "fan_0"));
	Inputs Press = Deep;
	Press.World = W0 + 105.5;
	int Expected = 0;
	std::vector<std::pair<std::string, double>> Coming;
	for (const Member& M : Ch.Members)
	{
		if (!M.Follower || Room.Attending(M.Name))
		{
			continue;
		}
		++Expected;
		const double Days = std::max(0.0, Press.World - M.LastSeen) / PlayDay;
		const double Chance = growth::RailCallChance(growth::PingW(Days, Ch.IsMod(M.Name) ? 1.0 : M.Loyalty), M.Loyalty, 1.3, 1.0, growth::Appetite(Days),
			growth::Budget(growth::HwToday(M.Hw, M.HwDay, Today)));
		const uint64_t Key = growth::NameKey(M.Name);
		if (growth::Unit(PlaySeed, 7, growth::Tag::RailCall, Key) < Chance)
		{
			Coming.emplace_back(M.Name, Press.World + growth::ExpAt(6.0, growth::Unit(PlaySeed, 7, growth::Tag::RailCall, Key, 1)));
		}
	}
	Expect(Expected == 80 - 30 + 2 + 1, "rail call: the test's rail is 53 followers not attending (fan_0 left the room: away, not attending)");
	Expect(play::RailCall(P, Room, Ch, Press), "rail call: pressed at an attended minute");
	Expect(P.RailReach == Expected, "rail call: its reach is every follower not attending (" + std::to_string(P.RailReach) + ")");
	bool Same = Room.Arrivals.size() == Coming.size();
	for (size_t I = 0; Same && I < Coming.size(); ++I)
	{
		Same = Room.Arrivals[I].Name == Coming[I].first && Room.Arrivals[I].At == Coming[I].second && Room.Arrivals[I].From == View::Source::Members;
	}
	Expect(Same, "rail call: each arrival is the keyed second roll, at +Exp(6) on its own draw (" + std::to_string(Coming.size()) + " coming)");
	PlayRailReach = P.RailReach;
	PlayRailComing = Coming.size();
	Expect(Room.Said_(kcopy::RailCallReached(Expected)) && kcopy::RailCallReached(Expected).find(std::to_string(Expected)) != std::string::npos,
		"rail call: \"Your rail call reached N followers\", N its reach");
	Expect(growth::Granted(Ch.GrantDay, Ch.GrantMask, Today, growth::GrantRailCall), "rail call: today's grant is used");
	Expect(!play::RailCall(P, Room, Ch, Press), "rail call: once");
	play::Fill(P, V);
	Expect(V.Action == View::Slot::None && V.RailReach == Expected, "rail call: the slot frees, the view keeps the reach");
	// The named arrivals: each one the call brought says so once, a friend her own way; anyone else's arrival is the Room's.
	const std::string First = Coming.empty() ? std::string() : Coming[0].first;
	if (!First.empty())
	{
		Room.Here.push_back(First);
		play::OnArrive(P, Room, Ch, First, View::Source::Members);
		play::OnArrive(P, Room, Ch, First, View::Source::Members);
		play::OnArrive(P, Room, Ch, "fan_5", View::Source::Members);
		PlayChat(P, Room, W0 + 107.0, 1.5, 1.0 / 60.0, R);
		Expect(P.RailArrived == 1 && Room.LinesFrom(First) == 1, "rail call: an arrival it brought is counted and speaks once");
		const std::string& Said = Room.Lines.back().Text;
		const std::vector<std::string>& Pool = kcopy::RailCallArrive();
		const bool Right = First == PlayDee ? Said == kcopy::DeeOmw()
						   : First == PlayMei ? Said == kcopy::MeiRail()
											  : std::find(Pool.begin(), Pool.end(), Said) != Pool.end();
		Expect(Right, "rail call: the arrival's line (\"rail call got me\", Dee's \"omw!!\"): " + Said);
		play::Fill(P, V);
		Expect(V.RailArrived == 1, "rail call: the view counts the arrivals");
	}
	// Dee and Mei are in the call.
	{
		FakeRoom Friends;
		Friends.Here = {"fan_1"};
		Channel Two = PlayChannel(0, W0);
		PlayState Q;
		play::Begin(Q, Two, PlayIn(W0));
		Q.RailOpen = true;
		Expect(play::RailCall(Q, Friends, Two, Press) && Q.RailReach == 2, "rail call: Dee and Mei are in it (their reach is 2)");
	}
	// Mods read the Ping's recency as 1.0 (§2.3.1, review): a low-loyalty mod last seen 100 days ago rolls at max(1.0, 0.35), not
	// at the lapsed follower's max(0.12, 0.35). A stream number whose draw falls between the two bars tells them apart.
	{
		const std::string ModName = "mod_lapsed";
		const auto Lapsed = [&](bool IsMod) {
			Channel C;
			C.Members.push_back(PlayMember(ModName, 0.1, W0 - 100.0 * PlayDay, true, false));
			if (IsMod)
			{
				ss::kast::Moderator Md;
				Md.Name = ModName;
				C.Mods.push_back(Md);
			}
			C.Followers = 1;
			return C;
		};
		const double Days = (Press.World - (W0 - 100.0 * PlayDay)) / PlayDay;
		const double Budget = growth::Budget(0.0);
		const double ModBar = growth::RailCallChance(growth::PingW(Days, 1.0), 0.1, 1.3, 1.0, growth::Appetite(Days), Budget);
		const double FanBar = growth::RailCallChance(growth::PingW(Days, 0.1), 0.1, 1.3, 1.0, growth::Appetite(Days), Budget);
		uint64_t Between = 0;
		for (uint64_t S = 1; S < 4000 && Between == 0; ++S)
		{
			const double U = growth::Unit(PlaySeed, S, growth::Tag::RailCall, growth::NameKey(ModName));
			Between = U >= FanBar && U < ModBar ? S : 0;
		}
		Expect(FanBar < ModBar && Between != 0, "rail call: the mod check has a stream whose draw sits between the two bars");
		for (const bool IsMod : {true, false})
		{
			Channel C = Lapsed(IsMod);
			FakeRoom Empty;
			Empty.StreamV = Between;
			PlayState Q;
			play::Begin(Q, C, PlayIn(W0));
			Q.RailOpen = true;
			Expect(play::RailCall(Q, Empty, C, Press) && Q.RailReach == 1 && Empty.Arrivals.size() == (IsMod ? 1u : 0u),
				std::string("rail call: a lapsed mod reads recency 1.0 and comes; the same follower who isn't a mod doesn't (") + (IsMod ? "mod" : "not a mod") + ")");
		}
	}
	// The attention budget reads tonight's rested hours, growth::HwToday (review): a follower whose 30 hours were booked 20 days
	// ago has rested (Budget 1), where the saved figure read raw would turn them away. A stream whose draw sits between the bars.
	{
		const std::string Rested = "rested_fan";
		const auto RestedChannel = [&]() {
			Channel C;
			Member M = PlayMember(Rested, 0.6, Press.World - 3.0 * PlayDay, true, false);
			M.Hw = 30.0;
			M.HwDay = Today - 20;
			C.Members.push_back(M);
			C.Followers = 1;
			return C;
		};
		const double Days = 3.0;
		const double W = growth::PingW(Days, 0.6);
		const double Decayed = growth::RailCallChance(W, 0.6, 1.3, 1.0, growth::Appetite(Days), growth::Budget(growth::HwToday(30.0, Today - 20, Today)));
		const double Raw = growth::RailCallChance(W, 0.6, 1.3, 1.0, growth::Appetite(Days), growth::Budget(30.0));
		uint64_t Between = 0;
		for (uint64_t S = 1; S < 4000 && Between == 0; ++S)
		{
			const double U = growth::Unit(PlaySeed, S, growth::Tag::RailCall, growth::NameKey(Rested));
			Between = U >= Raw && U < Decayed ? S : 0;
		}
		Channel C = RestedChannel();
		FakeRoom Empty;
		Empty.StreamV = Between;
		PlayState Q;
		play::Begin(Q, C, PlayIn(W0));
		Q.RailOpen = true;
		Expect(Raw < Decayed && Between != 0 && play::RailCall(Q, Empty, C, Press) && Empty.Arrivals.size() == 1,
			"rail call: the attention budget reads tonight's rested hours (HwToday), not the saved ones");
	}
	// Recency is read at the press's own world time, In.World (review): a follower last seen 7 days and a moment before the press
	// reads PingW 0.7, not the 1.0 of the room's last grid minute half a minute earlier.
	{
		const std::string Week = "week_fan";
		Channel C;
		C.Members.push_back(PlayMember(Week, 0.4, Press.World - 7.0 * PlayDay - 0.25, true, false));
		C.Followers = 1;
		const double DaysPress = (7.0 * PlayDay + 0.25) / PlayDay;
		const double DaysRoom = (7.0 * PlayDay - 0.25) / PlayDay;
		const double AtPress = growth::RailCallChance(growth::PingW(DaysPress, 0.4), 0.4, 1.3, 1.0, growth::Appetite(DaysPress), growth::Budget(0.0));
		const double AtRoom = growth::RailCallChance(growth::PingW(DaysRoom, 0.4), 0.4, 1.3, 1.0, growth::Appetite(DaysRoom), growth::Budget(0.0));
		uint64_t Between = 0;
		for (uint64_t S = 1; S < 4000 && Between == 0; ++S)
		{
			const double U = growth::Unit(PlaySeed, S, growth::Tag::RailCall, growth::NameKey(Week));
			Between = U >= AtPress && U < AtRoom ? S : 0;
		}
		FakeRoom Behind;
		Behind.StreamV = Between;
		Behind.MinuteV = Press.World - 0.5;
		PlayState Q;
		play::Begin(Q, C, PlayIn(W0));
		Q.RailOpen = true;
		Expect(AtPress < AtRoom && Between != 0 && play::RailCall(Q, Behind, C, Press) && Q.RailReach == 1 && Behind.Arrivals.empty(),
			"rail call: recency is read at the press's world time (7 days and a moment ago: PingW 0.7, not the room minute's 1.0)");
	}
	// Dee and Mei answer the call in their own words (review): "omw!!" and Mei's line; anyone else from the pool.
	{
		FakeRoom Friends;
		Friends.Here = {PlayDee, PlayMei, "fan_7"};
		PlayState Q;
		ss::Rng Rq("kastplay:friends");
		play::Begin(Q, Ch, PlayIn(W0));
		Q.RailSent = {PlayDee, PlayMei, "fan_7"};
		for (const char* Who : {PlayDee, PlayMei, "fan_7"})
		{
			play::OnArrive(Q, Friends, Ch, Who, View::Source::Members);
		}
		PlayChat(Q, Friends, W0 + 106.0, 1.5, 1.0 / 60.0, Rq);
		const std::vector<std::string>& Pool = kcopy::RailCallArrive();
		bool Own = Friends.Lines.size() == 3;
		for (const FakeRoom::Said& Line : Friends.Lines)
		{
			Own = Own && (Line.Who == PlayDee ? Line.Text == kcopy::DeeOmw()
						  : Line.Who == PlayMei ? Line.Text == kcopy::MeiRail()
												: std::find(Pool.begin(), Pool.end(), Line.Text) != Pool.end());
		}
		Expect(Q.RailArrived == 3 && Own, "rail call: Dee says \"omw!!\", Mei her own line, a fan the pool's (" + std::to_string(Friends.Lines.size()) + " lines)");
		PlayRefusedTotal += Friends.Refused;
	}
	Summary S;
	play::Close(P, S);
	Expect(S.RailCalled && S.RailReach == Expected && S.RailArrived == (First.empty() ? 0 : 1), "rail call: the end card's facts");
	// Once a world day: a restart today isn't offered it again; tomorrow is.
	PlayState Again;
	play::Begin(Again, Ch, PlayIn(W0 + 110.0));
	Deep.World = W0 + 111.0;
	Expect(ss::net::DayOf(Deep.World) == Today, "rail call: the restart is the same world day");
	play::OnMinute(Again, Room, Ch, Deep, 1);
	Expect(!Again.RailOpen, "rail call: once a world day (a restart doesn't re-arm it)");
	Inputs Tomorrow = Deep;
	Tomorrow.World = W0 + PlayDay;
	play::Begin(Again, Ch, Tomorrow);
	play::OnMinute(Again, Room, Ch, Tomorrow, 0);
	Expect(Again.RailOpen, "rail call: offered again the next world day");
	// The run ends: withdrawn.
	Inputs Over = Tomorrow;
	Over.DeepEntrants = 0;
	Over.Depth = 0.0;
	play::OnMinute(Again, Room, Ch, Over, 1);
	Expect(!Again.RailOpen, "rail call: withdrawn when the run ends");
	// Your whole rail is here: every follower attending, no button, nothing to press.
	{
		FakeRoom All;
		Channel Two = PlayChannel(2, W0);
		for (const Member& M : Two.Members)
		{
			All.Here.push_back(M.Name);
		}
		PlayState Q;
		play::Begin(Q, Two, Tomorrow);
		play::OnMinute(Q, All, Two, Tomorrow, 0);
		play::Fill(Q, V);
		Expect(V.WholeRail && V.Action == View::Slot::None, "rail call: \"Your whole rail is here\" in place of the button");
		Expect(!play::RailCall(Q, All, Two, Tomorrow), "rail call: nothing to press when the whole rail is here");
		Expect(All.Arrivals.empty() && !growth::Granted(Two.GrantDay, Two.GrantMask, ss::net::DayOf(Tomorrow.World), growth::GrantRailCall),
			"rail call: the refused press uses no grant");
	}
	PlayRefusedTotal += Room.Refused;
}

// ------------------------------------------------------------------ the action slot, the toast line, First Follower

void SlotAndToasts()
{
	const double W0 = 25.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(10, W0);
	FakeRoom Room;
	Room.MinuteV = W0 + 50.0;
	Room.Here = {PlayDee, PlayMei, "fan_1", "fan_2"};
	PlayState P;
	ss::Rng R("kastplay:slot");
	play::Begin(P, Ch, PlayIn(W0));
	P.RailOpen = true;
	Room.RaidAtV = W0 + 50.0;
	PlayChat(P, Room, W0 + 50.1, 0.2, 1.0 / 60.0, R);
	// The moments carry the deep run RAIL CALL is offered on (a moment without one is the run's end: it withdraws RAIL CALL).
	play::OnMoment(P, Room, Ch, PlayDeep(W0 + 50.2), Moment::AllIn);
	View V;
	play::Fill(P, V);
	const int WelcomeBit = 1 << static_cast<int>(View::Slot::WelcomeRaid);
	const int RailBit = 1 << static_cast<int>(View::Slot::RailCall);
	Expect(V.Action == View::Slot::HoldCrack && V.Waiting == (WelcomeBit | RailBit), "action slot: Hold or Crack first, the others wait as chips");
	play::OnMoment(P, Room, Ch, PlayDeep(W0 + 50.4), Moment::WonAllIn);
	play::Fill(P, V);
	Expect(V.Action == View::Slot::WelcomeRaid && V.Waiting == RailBit, "action slot: then Welcome the raid");
	play::WelcomeRaid(P, Room, Ch);
	play::Fill(P, V);
	Expect(V.Action == View::Slot::RailCall && V.Waiting == 0, "action slot: then Rail Call");
	// First Follower: the line, the alert through the toast line, KastShine, Dee's cheer.
	PlayState F;
	FakeRoom First;
	First.StreamV = 1;
	First.Here = {PlayDee, PlayMei, "nightrail_88"};
	play::Begin(F, Ch, PlayIn(W0));
	play::OnFollow(F, First, Ch, "fan_1", false);
	Expect(First.Lines.empty() && First.Alerts.empty(), "first follower: an ordinary follow is the Room's alone");
	play::OnFollow(F, First, Ch, "nightrail_88", true);
	Expect(First.Said_(kcopy::FirstFollower("nightrail_88")) && First.Lines.front().Who.empty(), "first follower: the line (a system line)");
	Expect(First.Alerts.size() == 1 && First.Alerts[0].Who == "nightrail_88" && First.Alerts[0].Text == kcopy::FirstFollowerSub(),
		"first follower: the alert");
	Expect(First.Notices.size() == 1 && First.Notices[0].Sound == static_cast<int>(ss::SoundId::KastShine) && First.Notices[0].Type == Notice::Kind::Milestone,
		"first follower: KastShine through the cue queue");
	PlayChat(F, First, W0 + 11.0, 2.0, 1.0 / 60.0, R);
	Expect(First.LinesFrom(PlayDee) == 1, "first follower: Dee cheers");
	// The toast line: one a real minute, none while Hold or Crack is open; a toast that waits isn't lost.
	Expect(!play::ToastFree(F), "toast line: busy right after a toast");
	play::OnFollow(F, First, Ch, "second_88", true);
	Expect(First.Alerts.size() == 1 && F.Toasts.size() == 1, "toast line: a second toast waits");
	PlayChat(F, First, W0 + 11.5, 30.0, 0.25, R);
	Expect(First.Alerts.size() == 1, "toast line: still waiting half a minute on");
	play::OnMoment(F, First, Ch, PlayIn(W0 + 12.0), Moment::AllIn);
	PlayChat(F, First, W0 + 12.0, 40.0, 0.25, R);
	Expect(F.HoC.Open && First.Alerts.size() == 1, "toast line: none while Hold or Crack is open");
	play::OnMoment(F, First, Ch, PlayIn(W0 + 12.5), Moment::LostAllIn);
	PlayChat(F, First, W0 + 12.6, 0.5, 0.25, R);
	Expect(First.Alerts.size() == 2 && F.Toasts.empty() && !play::ToastFree(F), "toast line: the waiting toast goes up once the line frees");
	PlayRefusedTotal += Room.Refused + First.Refused;
	// Toasts keep their order (review): a toast that arrives the instant the line frees (Hold or Crack's reveal, before the next
	// display step) goes up behind the one already waiting, not ahead of it.
	{
		PlayState Q;
		FakeRoom T;
		T.StreamV = 1;
		T.Here = {PlayDee, PlayMei, "first_a", "first_b", "first_c"};
		play::Begin(Q, Ch, PlayIn(W0));
		play::OnFollow(Q, T, Ch, "first_a", true); // up at once
		play::OnFollow(Q, T, Ch, "first_b", true); // waits for the line
		play::OnMoment(Q, T, Ch, PlayIn(W0 + 13.0), Moment::AllIn);
		PlayChat(Q, T, W0 + 13.0, 70.0, 0.25, R); // a minute passes, but Hold or Crack holds the line
		play::OnMoment(Q, T, Ch, PlayIn(W0 + 13.4), Moment::WonAllIn);
		play::OnFollow(Q, T, Ch, "first_c", true); // the line is free this instant, and first_b is still waiting
		PlayChat(Q, T, W0 + 13.5, 0.5, 0.25, R);
		Expect(T.Alerts.size() == 2 && T.Alerts[1].Who == "first_b" && Q.Toasts.size() == 1 && Q.Toasts[0].A.Who == "first_c",
			"toast line: toasts go up in order (the one waiting first, a newer one behind it)");
		PlayRefusedTotal += T.Refused;
	}
}

/** RAIL CALL goes with the run (review): a bust, whose inputs no longer carry an armed run at depth 0.4, withdraws it at once
 *  (not at the next grid minute, 13 real seconds later at Smart); a moment while the run lives, or another table's deep run,
 *  keeps it. */
void RailGoesWithTheRun()
{
	const double W0 = 28.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(10, W0);
	FakeRoom Room;
	Room.MinuteV = W0 + 30.0;
	Room.Here = {PlayDee, "fan_1"};
	PlayState P;
	play::Begin(P, Ch, PlayIn(W0));
	play::OnMinute(P, Room, Ch, PlayDeep(W0 + 30.0), 30);
	View V;
	play::Fill(P, V);
	Expect(P.RailOpen && V.Action == View::Slot::RailCall, "rail call: offered on the deep run (the run's-end check's start)");
	play::OnMoment(P, Room, Ch, PlayDeep(W0 + 30.3), Moment::BigPot);
	play::Fill(P, V);
	Expect(P.RailOpen && V.Action == View::Slot::RailCall, "rail call: a moment while the run lives keeps RAIL CALL");
	Inputs OtherTable = PlayIn(W0 + 30.5); // this run is over, but a second table's 800-runner run is at depth 0.45
	OtherTable.DeepEntrants = 800;
	OtherTable.Depth = 0.45;
	play::OnMoment(P, Room, Ch, OtherTable, Moment::Bust);
	Expect(P.RailOpen, "rail call: a bust while another table's run is at depth 0.4 keeps RAIL CALL (the deepest run)");
	play::OnMoment(P, Room, Ch, PlayIn(W0 + 30.6), Moment::Bust);
	play::Fill(P, V);
	Expect(!P.RailOpen && V.Action == View::Slot::None && !V.WholeRail, "rail call: the bust withdraws RAIL CALL at once, inside the grid minute");
	Expect(!play::RailCall(P, Room, Ch, PlayIn(W0 + 30.7)) && Room.Arrivals.empty() &&
			   !growth::Granted(Ch.GrantDay, Ch.GrantMask, ss::net::DayOf(W0 + 30.7), growth::GrantRailCall),
		"rail call: nothing to press after the bust (nobody sent, no grant spent)");
	PlayRefusedTotal += Room.Refused;
}

/** §2.9's table, verb by verb, at an attended pace and heads-down (the C-K audit's review of every play:: verb): each button is
 *  drawn exactly when its press works. Nod draws and works at every pace; Welcome the raid, Rail Call, Set the scene and Hold
 *  or Crack's call only while attended; "Your whole rail is here" stands in RAIL CALL's place and follows it. */
void VerbsByPace()
{
	const double W0 = 29.0 * PlayDay + 21.0 * 60.0;
	const int WelcomeBit = 1 << static_cast<int>(View::Slot::WelcomeRaid);
	const int RailBit = 1 << static_cast<int>(View::Slot::RailCall);
	struct Case
	{
		bool Drawn = false;
		bool Works = false;
	};
	const auto Nod = [&](bool Attended) {
		Channel Ch = PlayChannel(10, W0);
		FakeRoom Room;
		Room.MinuteV = W0 + 30.0;
		Room.Here = {"guest_pace"};
		PlayState P;
		ss::Rng R("kastplay:pace:nod");
		play::Begin(P, Ch, PlayIn(W0));
		Room.AttendedV = Attended;
		PlayChat(P, Room, W0 + 30.2, 0.2, 1.0 / 60.0, R, Attended);
		View V;
		play::Fill(P, V);
		Case C;
		C.Drawn = V.NodsLeft > 0;
		C.Works = play::Nod(P, Room, Ch, "guest_pace");
		PlayRefusedTotal += Room.Refused;
		return C;
	};
	const auto Welcome = [&](bool Attended) {
		Channel Ch = PlayChannel(10, W0);
		FakeRoom Room;
		Room.MinuteV = W0 + 30.0;
		Room.Here = {PlayDee, "fan_1"};
		Room.RaidAtV = W0 + 30.0;
		PlayState P;
		ss::Rng R("kastplay:pace:welcome");
		play::Begin(P, Ch, PlayIn(W0));
		Room.AttendedV = Attended;
		PlayChat(P, Room, W0 + 30.2, 0.2, 1.0 / 60.0, R, Attended);
		View V;
		play::Fill(P, V);
		Case C;
		C.Drawn = V.Action == View::Slot::WelcomeRaid || (V.Waiting & WelcomeBit) != 0;
		C.Works = play::WelcomeRaid(P, Room, Ch);
		PlayRefusedTotal += Room.Refused;
		return C;
	};
	const auto Rail = [&](bool Attended) {
		Channel Ch = PlayChannel(10, W0);
		FakeRoom Room;
		Room.MinuteV = W0 + 30.0;
		Room.Here = {PlayDee, "fan_1"};
		PlayState P;
		ss::Rng R("kastplay:pace:rail");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMinute(P, Room, Ch, PlayDeep(W0 + 30.0), 30); // offered at an attended minute
		Room.AttendedV = Attended;
		play::OnMinute(P, Room, Ch, PlayDeep(W0 + 31.0, Attended), 31);
		PlayChat(P, Room, W0 + 31.2, 0.2, 1.0 / 60.0, R, Attended);
		View V;
		play::Fill(P, V);
		Case C;
		C.Drawn = V.Action == View::Slot::RailCall || (V.Waiting & RailBit) != 0;
		C.Works = play::RailCall(P, Room, Ch, PlayDeep(W0 + 31.5, Attended));
		PlayRefusedTotal += Room.Refused;
		return C;
	};
	const auto Scene = [&](bool Attended) {
		Channel Ch = PlayChannel(10, W0);
		FakeRoom Room;
		Room.StreamV = 1;
		Room.MinuteV = W0 + 3.0;
		PlayState P;
		ss::Rng R("kastplay:pace:scene");
		play::Begin(P, Ch, PlayIn(W0));
		Room.AttendedV = Attended;
		PlayChat(P, Room, W0 + 3.0, 0.2, 1.0 / 60.0, R, Attended);
		View V;
		play::Fill(P, V);
		Case C;
		C.Drawn = V.SceneOpen;
		C.Works = play::SetTheScene(P, Room, Ch);
		PlayRefusedTotal += Room.Refused;
		return C;
	};
	const auto Call = [&](bool Attended) {
		Channel Ch = PlayChannel(10, W0);
		FakeRoom Room;
		Room.MinuteV = W0 + 30.0;
		Room.Here = {PlayDee, PlayMei, "fan_1"};
		PlayState P;
		ss::Rng R("kastplay:pace:call");
		play::Begin(P, Ch, PlayIn(W0));
		play::OnMoment(P, Room, Ch, PlayIn(W0 + 30.0), Moment::AllIn); // opened at Smart, then the pace is set
		Room.AttendedV = Attended;
		PlayChat(P, Room, W0 + 30.1, 0.5, 1.0 / 60.0, R, Attended);
		View V;
		play::Fill(P, V);
		Case C;
		C.Drawn = V.HoC.Open && V.Action == View::Slot::HoldCrack;
		const double Before = Room.EngageV;
		C.Works = play::CallIt(P, Room, Ch, true);
		C.Works = C.Works && Room.EngageV > Before;
		PlayRefusedTotal += Room.Refused;
		return C;
	};
	const auto WholeRail = [&](bool Attended) {
		Channel Two = PlayChannel(2, W0);
		FakeRoom All;
		for (const Member& M : Two.Members)
		{
			All.Here.push_back(M.Name);
		}
		PlayState P;
		ss::Rng R("kastplay:pace:whole");
		play::Begin(P, Two, PlayIn(W0));
		play::OnMinute(P, All, Two, PlayDeep(W0 + 30.0), 30);
		All.AttendedV = Attended;
		play::OnMinute(P, All, Two, PlayDeep(W0 + 31.0, Attended), 31);
		PlayChat(P, All, W0 + 31.2, 0.2, 1.0 / 60.0, R, Attended);
		View V;
		play::Fill(P, V);
		return V.WholeRail;
	};
	struct Row
	{
		const char* Name;
		std::function<Case(bool)> Run;
		bool HeadsDown; // §2.9: draws heads-down
	};
	const Row Rows[] = {{"Nod", Nod, true}, {"Welcome the raid", Welcome, false}, {"Rail Call", Rail, false}, {"Set the scene", Scene, false},
		{"Hold or Crack's call", Call, false}};
	for (const Row& Rw : Rows)
	{
		const Case At = Rw.Run(true);
		const Case Down = Rw.Run(false);
		Expect(At.Drawn && At.Works, std::string("verbs by pace: ") + Rw.Name + " is drawn and works at an attended pace");
		Expect(Down.Drawn == Rw.HeadsDown && Down.Works == Rw.HeadsDown,
			std::string("verbs by pace: ") + Rw.Name + (Rw.HeadsDown ? " is drawn and works heads-down" : " is neither drawn nor pressable heads-down") + " (drawn " +
				std::to_string(Down.Drawn) + ", works " + std::to_string(Down.Works) + ")");
	}
	Expect(WholeRail(true) && !WholeRail(false), "verbs by pace: \"Your whole rail is here\" follows RAIL CALL (attended only)");
}

// ------------------------------------------------------------------ the room never reads display time

/** One scripted night: the same clicks and moments, display time sliced by Step and its words drawn from R. */
std::string PlayNight(double Step, const char* RSeed, std::string& Channel_, int& CallsSeen)
{
	CallsSeen = 0;
	const double W0 = 26.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(40, W0);
	FakeRoom Room;
	for (int I = 0; I < 12; ++I)
	{
		Room.Here.push_back("fan_" + std::to_string(I));
	}
	for (int I = 0; I < 6; ++I)
	{
		Room.Here.push_back("guest_" + std::to_string(I));
	}
	Room.Extra = 4;
	PlayState P;
	ss::Rng R(RSeed);
	play::Begin(P, Ch, PlayIn(W0));
	const auto Minutes = [&](int From, int To, const Inputs& In) {
		for (int M = From; M <= To; ++M)
		{
			Room.MinuteV = W0 + static_cast<double>(M);
			play::OnMinute(P, Room, Ch, In, M);
		}
	};
	Inputs In = PlayIn(W0 + 3.0);
	Minutes(0, 3, In);
	PlayChat(P, Room, W0 + 3.0, 2.0, Step, R);
	play::Nod(P, Room, Ch, "guest_2");
	play::Nod(P, Room, Ch, "fan_3");
	play::Nod(P, Room, Ch, "guest_4");
	PlayChat(P, Room, W0 + 3.4, 2.0, Step, R);
	play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.0), Moment::AllIn);
	PlayChat(P, Room, W0 + 4.0, 1.0, Step, R);
	play::CallIt(P, Room, Ch, true);
	// Four people leave mid-call: how many of chat's calls land now depends on R's order and the frame rate (display only).
	for (int I = 0; I < 4; ++I)
	{
		Room.Here.erase(std::find(Room.Here.begin(), Room.Here.end(), "guest_" + std::to_string(I)));
	}
	PlayChat(P, Room, W0 + 4.1, 2.5, Step, R);
	CallsSeen += static_cast<int>(P.HoC.Calls.size()) * 1000 + static_cast<int>(Room.Lines.size());
	play::OnMoment(P, Room, Ch, PlayIn(W0 + 4.3), Moment::BadBeat);
	PlayChat(P, Room, W0 + 4.4, 3.0, Step, R);
	Room.RaidAtV = W0 + 5.0;
	Minutes(4, 5, PlayIn(W0 + 5.0));
	PlayChat(P, Room, W0 + 5.2, 1.0, Step, R);
	play::WelcomeRaid(P, Room, Ch);
	Inputs Deep = PlayIn(W0 + 6.0);
	Deep.DeepEntrants = 980;
	Deep.Depth = 0.55;
	Minutes(6, 6, Deep);
	PlayChat(P, Room, W0 + 6.2, 1.0, Step, R);
	Deep.World = W0 + 6.5;
	play::RailCall(P, Room, Ch, Deep);
	for (size_t I = 0; I < Room.Arrivals.size() && I < 4; ++I)
	{
		Room.Here.push_back(Room.Arrivals[I].Name);
		play::OnArrive(P, Room, Ch, Room.Arrivals[I].Name, View::Source::Members);
	}
	PlayChat(P, Room, W0 + 7.0, 3.0, Step, R);
	// A sprint frame: 40 world minutes at once.
	Minutes(7, 47, Deep);
	PlayChat(P, Room, W0 + 47.0, 1.0, Step, R);
	Summary S;
	play::Close(P, S);
	PlayRefusedTotal += Room.Refused;
	Channel_ = PlayMoney(Ch);
	std::ostringstream O;
	O << Room.Effects() << " |P " << P.NodsLeft << ' ' << P.RailReach << ' ' << P.RailArrived << ' ' << P.RailCalled << ' ' << P.WelcomedAt << " |S "
	  << S.RailReach << ' ' << S.RailArrived;
	return O.str();
}

void DisplayChangesNothing()
{
	std::string ChA;
	std::string ChB;
	std::string ChC;
	int SeenA = 0;
	int SeenB = 0;
	int SeenC = 0;
	const std::string A = PlayNight(1.0 / 60.0, "kastplay:night:a", ChA, SeenA);
	const std::string B = PlayNight(1.0 / 30.0, "kastplay:night:b", ChB, SeenB);
	const std::string C = PlayNight(0.5, "kastplay:night:c", ChC, SeenC);
	Expect(SeenA != SeenB || SeenA != SeenC, "the games: the three nights' displays differ (the check below means something)");
	Expect(A == B && A == C, "the games: the room's effects are the same at 60 fps, 30 fps and 2 fps with another R");
	Expect(ChA == ChB && ChA == ChC, "the games: the channel's bytes are the same at every frame rate");
	if (A != B || A != C)
	{
		std::printf("  60 fps: %s\n  30 fps: %s\n  2 fps: %s\n", A.c_str(), B.c_str(), C.c_str());
	}
}
// ------------------------------------------------------------------ the seam, end to end through the real Stream and its Room

/** A first stream through kast::Stream's public verbs (the Room is core-kastroom's RoomView): Dee and Mei watching, an all-in
 *  at the table opens Hold or Crack, chat calls it, the streamer calls it, the reveal, a Nod on Dee, the end card. */
void ThroughTheStream()
{
	const double W0 = 10.0 * PlayDay + 21.0 * 60.0;
	Channel Ch;
	ss::kast::Stream St("kastplay:seam");
	Inputs In = PlayIn(W0);
	In.Hero = "grinder_3c";
	In.AtTable = true;
	In.Tables = 1;
	In.RentsPaid = 2;
	{
		// StreamGrind's kit (a desktop, fiber, a 1080p cam, a mic, a light, an overlay, a second monitor): enough to go live.
		ss::gear::Owned Own;
		for (const char* Id : {"tower-mid", "fiber", "webcam-1080", "mic-usb", "ring-light", "overlay-pack", "monitor-24"})
		{
			Own[Id] = 0.0;
		}
		In.Gear = ss::gear::Sum(Own);
	}
	St.Start(Ch, In);
	Expect(St.Live, "through the stream: live");
	const double RealPerWorld = 13.1; // Smart
	const auto Run = [&](double ToWorld) {
		const double DReal = 1.0 / 30.0;
		const double DWorld = DReal / RealPerWorld;
		while (In.World < ToWorld - 1e-9)
		{
			In.World = std::min(ToWorld, In.World + DWorld);
			In.Real += DReal;
			St.Tick(Ch, In, DWorld, DReal);
		}
	};
	Run(W0 + 4.0);
	Expect(St.Count() >= 2, "through the stream: Dee and Mei watch the first stream (" + std::to_string(St.Count()) + " watching)");
	St.OnMoment(Ch, In, Moment::AllIn, std::string(), 1.0);
	Run(W0 + 4.3);
	const View Open = St.GetView();
	Expect(Open.HoC.Open && Open.Action == View::Slot::HoldCrack, "through the stream: the all-in opens Hold or Crack in the action slot");
	Expect(Open.HoC.Holds + Open.HoC.Cracks >= 1 && Open.HoC.Holds + Open.HoC.Cracks <= St.Count(), "through the stream: chat calls it, never more callers than watching");
	const double EngageBefore = St.Engage;
	Expect(St.CallIt(Ch, true) && St.Engage > EngageBefore, "through the stream: the streamer's call (Engage up)");
	St.OnMoment(Ch, In, Moment::WonAllIn, "1,200", 1.0);
	Run(W0 + 4.6);
	Expect(!St.GetView().HoC.Open, "through the stream: the won all-in reveals it");
	bool RevealLine = false;
	for (const ss::kast::ChatMsg& M : St.Chat)
	{
		RevealLine = RevealLine || (M.Kind == LineKind::System && M.Who.empty() && M.Text.rfind(kcopy::RevealLine(true, 0, 1, 0).substr(0, 5), 0) == 0);
	}
	Expect(RevealLine, "through the stream: the reveal's system line is in chat");
	bool Call = false;
	for (const Notice& N : St.Notices)
	{
		Call = Call || N.Sound == static_cast<int>(ss::SoundId::KastCall);
	}
	Expect(Call, "through the stream: KastCall goes to the session's cue queue");
	Expect(St.Nod(Ch, PlayDee), "through the stream: a Nod on Dee");
	Run(W0 + 4.9);
	const View Nodded = St.GetView();
	Expect(Nodded.NodsLeft == 2 && std::find(Nodded.Lit.begin(), Nodded.Lit.end(), PlayDee) != Nodded.Lit.end(), "through the stream: 2 Nods left, Dee's name lit");
	const std::vector<std::string>& Replies = kcopy::NodReply();
	bool Replied = false;
	for (const ss::kast::ChatMsg& M : St.Chat)
	{
		Replied = Replied || (M.Who == PlayDee && std::find(Replies.begin(), Replies.end(), M.Text) != Replies.end());
	}
	Expect(Replied, "through the stream: Dee answers the Nod");
	Run(W0 + 6.5);
	Expect(St.GetView().Lit.empty(), "through the stream: the glow is gone a world minute on");
	// A raid in (ss.Kast.Raid's path): WELCOME THE RAID takes the slot, and the welcome lands once.
	St.DebugRoom(Ch, In, "raid 14");
	Run(W0 + 6.8);
	Expect(St.GetView().Action == View::Slot::WelcomeRaid, "through the stream: a raid puts WELCOME THE RAID in the slot");
	Expect(St.WelcomeRaid(Ch) && !St.WelcomeRaid(Ch), "through the stream: the raid is welcomed, once");
	Run(W0 + 7.2);
	Expect(St.GetView().Action != View::Slot::WelcomeRaid, "through the stream: the slot frees once welcomed");
	const std::vector<std::string>& Thanks = kcopy::WelcomedChat();
	int Thanked = 0;
	for (const ss::kast::ChatMsg& M : St.Chat)
	{
		Thanked += !M.Who.empty() && M.Who != PlayDee && M.Who != PlayMei && std::find(Thanks.begin(), Thanks.end(), M.Text) != Thanks.end() ? 1 : 0;
	}
	Expect(Thanked >= 1 && Thanked <= 2, "through the stream: the raid's own people thank the welcome (" + std::to_string(Thanked) + ")");
	// A deep run at depth 0.4 with only Dee and Mei following, both here: no RAIL CALL, "Your whole rail is here".
	In.DeepEntrants = 800;
	In.Depth = 0.45;
	In.DeepTable = 0;
	Run(W0 + 9.0);
	const View Rail = St.GetView();
	Expect(Rail.WholeRail && Rail.Action != View::Slot::RailCall && !St.RailCall(Ch, In), "through the stream: the whole rail is here, nothing to call");
	const Summary S = St.Stop(Ch, In);
	Expect(S.BestCallsOf >= 0 && S.BestCalls <= S.BestCallsOf, "through the stream: the end card's best caller is a count of real calls");
}

/** The audit's L2 through the real Stream (PIE §9.4 #5's "refused at 8x"): Hold or Crack opens at Smart and the player goes to
 *  watch 8x mid-runout. The next frame's View has no bar and no slot, Stream::CallIt is refused with Engage untouched, a grid
 *  minute at 8x leaves the game open (back at Smart the bar shows and the call works), and a reveal at 8x is chat's as at any
 *  pace (the result line, KastCall). */
void HeadsDownThroughTheStream()
{
	const double W0 = 11.0 * PlayDay + 21.0 * 60.0;
	Channel Ch;
	ss::kast::Stream St("kastplay:seam:headsdown");
	Inputs In = PlayIn(W0);
	In.Hero = "grinder_3c";
	In.AtTable = true;
	In.Tables = 1;
	In.RentsPaid = 2;
	{
		ss::gear::Owned Own;
		for (const char* Id : {"tower-mid", "fiber", "webcam-1080", "mic-usb", "ring-light", "overlay-pack", "monitor-24"})
		{
			Own[Id] = 0.0;
		}
		In.Gear = ss::gear::Sum(Own);
	}
	St.Start(Ch, In);
	const auto Run = [&](double ToWorld, double RealPerWorld) {
		const double DReal = 1.0 / 30.0;
		const double DWorld = DReal / RealPerWorld;
		while (In.World < ToWorld - 1e-9)
		{
			In.World = std::min(ToWorld, In.World + DWorld);
			In.Real += DReal;
			St.Tick(Ch, In, DWorld, DReal);
		}
	};
	const auto Pace = [&](bool Attended) {
		In.Attended = Attended; // watch 8x: 8x counts as Sprinting (Kast.h's Inputs)
		In.WatchSpeed = Attended ? 1 : 8;
		In.Sprinting = !Attended;
	};
	const auto Count = [&](int Sound) {
		int N = 0;
		for (const Notice& Nt : St.Notices)
		{
			N += Nt.Sound == Sound ? 1 : 0;
		}
		return N;
	};
	const auto RevealLines = [&]() {
		int N = 0;
		for (const ss::kast::ChatMsg& M : St.Chat)
		{
			N += M.Kind == LineKind::System && M.Who.empty() && M.Text.rfind(kcopy::RevealLine(true, 0, 1, 0).substr(0, 5), 0) == 0 ? 1 : 0;
		}
		return N;
	};
	Run(W0 + 4.0, 13.1);
	St.OnMoment(Ch, In, Moment::AllIn, std::string(), 1.0);
	Run(W0 + 4.2, 13.1);
	Expect(St.Count() >= 2 && St.GetView().HoC.Open && St.GetView().Action == View::Slot::HoldCrack, "through the stream heads-down: open at Smart (the check's start)");
	// Watch 8x, mid-runout, inside the world minute: the next frame's View has no bar and no slot, and the press is refused.
	Pace(false);
	Run(W0 + 4.25, 0.8);
	const View Down = St.GetView();
	Expect(!Down.HoC.Open && Down.Action != View::Slot::HoldCrack, "through the stream heads-down: at 8x the bar and the call buttons go on the next frame");
	const double EngageBefore = St.Engage;
	Expect(!St.CallIt(Ch, true) && !St.CallIt(Ch, false) && St.Engage == EngageBefore, "through the stream heads-down: Stream::CallIt is refused at 8x, Engage untouched");
	// A grid minute at 8x leaves chat's game open: back at Smart the bar shows again and the call works.
	Run(W0 + 5.1, 0.8);
	Pace(true);
	Run(W0 + 5.2, 13.1);
	const double EngageAtSmart = St.Engage;
	Expect(St.GetView().HoC.Open && St.CallIt(Ch, true) && St.Engage > EngageAtSmart, "through the stream heads-down: back at Smart before the reveal, the bar shows and the call works");
	// The reveal at 8x: chat's game resolves as at any pace (the result line and KastCall).
	Pace(false);
	Run(W0 + 5.3, 0.8);
	const int CallsBefore = Count(static_cast<int>(ss::SoundId::KastCall));
	const int RevealsBefore = RevealLines();
	St.OnMoment(Ch, In, Moment::WonAllIn, "1,200", 1.0);
	Run(W0 + 5.6, 0.8);
	Expect(Count(static_cast<int>(ss::SoundId::KastCall)) == CallsBefore + 1 && RevealLines() == RevealsBefore + 1 && !St.GetView().HoC.Open,
		"through the stream heads-down: the reveal at 8x is chat's as at any pace (the result line, KastCall)");
	St.Stop(Ch, In);
}

// ------------------------------------------------------------------ the games' share of the Room's frame (review)

/** §9.4 #13's budget (the Room's Tick <= 0.1 ms on a 1,500-member channel during a sprint frame): the games' part of a sprint
 *  frame (60 grid minutes heads-down with RAIL CALL offered, then Fill) and, for the record, an attended minute's "whole rail"
 *  scan and a Rail Call press over the 1,500. Printed, not asserted (the PC is shared); the check only guards the scan's order. */
double PlaySprintUs = 0.0;
double PlayScanUs = 0.0;
double PlayPressUs = 0.0;
double PlayRevealUs = 0.0;
size_t PlayRevealCallers = 0;

void GamesBudget()
{
	const double W0 = 27.0 * PlayDay + 21.0 * 60.0;
	Channel Ch = PlayChannel(1500, W0);
	FakeRoom Room;
	for (int I = 0; I < 40; ++I)
	{
		Room.Here.push_back("fan_" + std::to_string(I));
	}
	for (int I = 40; I < 700; ++I)
	{
		Room.AttendingAway.insert("fan_" + std::to_string(I));
	}
	PlayState P;
	play::Begin(P, Ch, PlayIn(W0));
	Inputs Deep = PlayIn(W0 + 10.0);
	Deep.DeepEntrants = 980;
	Deep.Depth = 0.6;
	play::OnMinute(P, Room, Ch, Deep, 10);
	Expect(P.RailOpen && !P.WholeRail, "the games' budget: RAIL CALL offered on the 1,500-member channel");
	using Clock = std::chrono::steady_clock;
	const int Frames = 50;
	Inputs Sprint = Deep;
	Sprint.Attended = false;
	View V;
	auto T0 = Clock::now();
	for (int F = 0; F < Frames; ++F)
	{
		for (int M = 0; M < 60; ++M)
		{
			play::OnMinute(P, Room, Ch, Sprint, 11 + F * 60 + M);
		}
		play::Fill(P, V);
	}
	PlaySprintUs = std::chrono::duration<double, std::micro>(Clock::now() - T0).count() / Frames;
	T0 = Clock::now();
	for (int F = 0; F < Frames; ++F)
	{
		play::OnMinute(P, Room, Ch, Deep, 3100 + F);
	}
	PlayScanUs = std::chrono::duration<double, std::micro>(Clock::now() - T0).count() / Frames;
	Deep.World = W0 + 3200.0;
	T0 = Clock::now();
	const bool Pressed = play::RailCall(P, Room, Ch, Deep);
	PlayPressUs = std::chrono::duration<double, std::micro>(Clock::now() - T0).count();
	Expect(Pressed && P.RailReach == 1500 + 2 - 700, "the games' budget: the press reaches the 802 followers not attending");
	Expect(PlaySprintUs < PlayScanUs, "the games' budget: a heads-down sprint frame never walks the community (" + std::to_string(PlaySprintUs) + " us against an attended minute's " +
		std::to_string(PlayScanUs) + " us)");
	// Hold or Crack's reveal on the big channel (review): six all-ins of 300 calls each from 1,500 people; the last reveal tallies
	// its calls against the night's 1,000+ callers (timed and printed; the check guards the tally: every call once, one entry a
	// caller).
	{
		FakeRoom Big;
		for (int I = 0; I < 1500; ++I)
		{
			Big.Here.push_back("fan_" + std::to_string(I));
		}
		PlayState Q;
		play::Begin(Q, Ch, PlayIn(W0));
		int Calls = 0;
		for (int H = 0; H < 6; ++H)
		{
			play::OnMoment(Q, Big, Ch, PlayIn(W0 + 20.0 + static_cast<double>(H)), Moment::AllIn);
			Q.HoC.Queued = true; // the calls below, not chat's draws
			for (int I = 0; I < 300; ++I)
			{
				PlayState::Call C;
				C.Who = "fan_" + std::to_string((I * 7 + H * 211) % 1500);
				C.Holds = (I + H) % 3 != 0;
				C.Points = 100;
				Q.HoC.Calls.push_back(C);
			}
			Calls += 300;
			T0 = Clock::now();
			play::OnMoment(Q, Big, Ch, PlayIn(W0 + 20.5 + static_cast<double>(H)), Moment::WonAllIn);
			PlayRevealUs = std::chrono::duration<double, std::micro>(Clock::now() - T0).count();
		}
		int Of = 0;
		std::set<std::string> Names;
		for (const PlayState::Caller& C : Q.Callers)
		{
			Of += C.Of;
			Names.insert(C.Name);
		}
		PlayRevealCallers = Q.Callers.size();
		Expect(Of == Calls && Names.size() == Q.Callers.size() && Q.Callers.size() > 1000,
			"the games' budget: six big reveals tally every call once, one entry a caller (" + std::to_string(Q.Callers.size()) + " callers)");
	}
}

// ------------------------------------------------------------------ the games' cues through the Session's queue

/** A SessionHooks that writes down every sound with the Session's clock. */
struct PlayCueHooks : ss::SessionHooks
{
	const ss::Session* S = nullptr;
	struct Played
	{
		ss::SoundId Id = ss::SoundId::Click;
		double Volume = 0.0;
		double At = 0.0;
	};
	std::vector<Played> Sounds;
	void Sound(ss::SoundId Id, double Volume) override { Sounds.push_back({Id, Volume, S ? S->Now : -1.0}); }
};

/** The cue queue against the old 1.2-s throttle (the C-K audit's L4; patches/fix-play_to_lead_1.diff, Session.cpp's
 *  PlayStreamNotices): the games' cues (the reveal's KastCall, First Follower's KastShine) keep §5.1's order, KastCall >
 *  KastAlert > KastShine > the raid sound > KastRise, when a raid and a follow land the instant the throttle frees. The raid's
 *  Win waits its turn in the queue, a follow's or a tip's sound never takes the slot from a waiting cue, sounds stay 1.2 s apart,
 *  and with nothing waiting a follow still chimes at its old volume. */
void CueQueueFirst()
{
	PlayCueHooks H;
	ss::Session S(H, "kastplay-cues");
	H.S = &S;
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 500000;
	double Now = 50.0;
	S.Update(Now);
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	const bool Live = Rig && S.GoLive().empty();
	Expect(Live, "cue queue: live from the lobby (the check's start)");
	if (!Live)
	{
		return;
	}
	const auto Step = [&](double To) {
		while (Now < To - 1e-9)
		{
			Now = std::min(To, Now + 0.05);
			S.Update(Now);
		}
	};
	const auto Post = [&](Notice::Kind K, ss::SoundId Cue) {
		Notice N;
		N.Type = K;
		N.Sound = Cue == ss::SoundId::Click ? -1 : static_cast<int>(Cue);
		S.Stream.Notices.push_back(N);
	};
	const ss::SoundId Plain = ss::SoundId::Click; // the notice's kind's own sound (Notice::Sound -1)
	Step(Now + 3.0); // go-live's own sounds clear the throttle
	const size_t From = H.Sounds.size();
	// A follow takes the free slot; the reveal's KastCall and a first follower's KastShine queue behind the throttle.
	Post(Notice::Kind::Chime, Plain);
	Step(Now + 0.05);
	const double ChimeAt = Now;
	Step(Now + 0.5);
	Post(Notice::Kind::Chime, ss::SoundId::KastCall);
	Post(Notice::Kind::Milestone, ss::SoundId::KastShine);
	// The instant the throttle frees, a raid, a follow and a tip land in the same frame, ahead of the queue in the notices.
	Step(ChimeAt + 1.15);
	Post(Notice::Kind::Raid, Plain);
	Post(Notice::Kind::Chime, Plain);
	Post(Notice::Kind::Tip, Plain);
	Now = ChimeAt + 1.22; // the first frame past the throttle (a whole 0.05 step could land a hair under 1.2)
	S.Update(Now);
	Step(ChimeAt + 5.0);
	// Nothing waiting: a follow still chimes, at its old volume.
	Post(Notice::Kind::Chime, Plain);
	Step(Now + 0.05);
	const size_t To = H.Sounds.size();
	S.EndStream();
	std::vector<PlayCueHooks::Played> Got;
	for (size_t I = From; I < To; ++I)
	{
		const ss::SoundId Id = H.Sounds[I].Id;
		if (Id == ss::SoundId::Alert || Id == ss::SoundId::KastCall || Id == ss::SoundId::KastShine || Id == ss::SoundId::Win || Id == ss::SoundId::Cash)
		{
			Got.push_back(H.Sounds[I]);
		}
	}
	std::string Order;
	bool Apart = true;
	for (size_t I = 0; I < Got.size(); ++I)
	{
		Order += std::string(I ? ", " : "") + ss::SoundName(Got[I].Id) + "@" + std::to_string(Got[I].At - ChimeAt).substr(0, 4);
		Apart = Apart && (I == 0 || Got[I].At - Got[I - 1].At >= 1.2 - 1e-9);
	}
	const bool Shape = Got.size() == 5 && Got[0].Id == ss::SoundId::Alert && Got[1].Id == ss::SoundId::KastCall && Got[2].Id == ss::SoundId::KastShine &&
					   Got[3].Id == ss::SoundId::Win && Got[4].Id == ss::SoundId::Alert && std::abs(Got[4].Volume - 0.35) < 1e-9 && std::abs(Got[3].Volume - 0.7) < 1e-9;
	Expect(Shape && Apart, "cue queue: KastCall, then KastShine, then the raid's Win, 1.2 s apart; the follow and the tip that landed with the raid don't jump "
						   "the queue; with nothing waiting a follow still chimes (" + Order + ")");
	std::printf("kast play: the cue queue with a raid, a follow and a tip landing as the throttle frees: %s\n", Order.c_str());
}
} // namespace play_seg

void SegPlay()
{
	const auto PlayT0 = std::chrono::steady_clock::now();
	const int PlayChecks0 = Checks;
	play_seg::HocGates();
	play_seg::HocCallsAndReveal();
	play_seg::HocHeadsDown();
	play_seg::NodChecks();
	play_seg::WelcomeAndScene();
	play_seg::RailCallChecks();
	play_seg::SlotAndToasts();
	play_seg::RailGoesWithTheRun();
	play_seg::VerbsByPace();
	play_seg::DisplayChangesNothing();
	play_seg::ThroughTheStream();
	play_seg::HeadsDownThroughTheStream();
	play_seg::CueQueueFirst();
	play_seg::GamesBudget();
	Expect(play_seg::PlayRefusedTotal == 0, "the games: no line was ever offered for someone not in the room (R1)");
	std::printf("kast play: a Nodded guest followed at %.4f (4,000 guests, Sat 1; 0.10 asked); hold or crack in a 60-name room: %d callers, "
				"%d HOLDS at 81%% equity; rail call: %d reached, %zu coming\n",
		play_seg::PlayNodRate, play_seg::PlayBigCalls, play_seg::PlayBigHolds, play_seg::PlayRailReach, play_seg::PlayRailComing);
	std::printf("kast play: the games on a 1,500-member channel (through the test's RoomView double): a sprint frame (60 grid minutes heads-down + "
				"Fill) %.1f us; an attended minute's whole-rail scan %.1f us; a Rail Call press %.1f us\n",
		play_seg::PlaySprintUs, play_seg::PlayScanUs, play_seg::PlayPressUs);
	std::printf("kast play: a Hold or Crack reveal of 300 calls against %zu callers tonight: %.1f us\n", play_seg::PlayRevealCallers, play_seg::PlayRevealUs);
	std::printf("kast play: %d checks in %.2fs\n", Checks - PlayChecks0, std::chrono::duration<double>(std::chrono::steady_clock::now() - PlayT0).count());
}
} // namespace kastgrowth_test
