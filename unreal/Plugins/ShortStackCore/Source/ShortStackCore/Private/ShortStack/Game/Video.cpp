#include "ShortStack/Game/Video.h"

#include "VideoAlgoPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

// Rerun (update 2 §4): candidates, their kinds and Drama; the upload's roll; the closed-form views, subscribers, the day
// ring, the partner gate and the monthly payout; the folded tails and the evergreen stubs; the save lines. core-video.
// The titles, the thumbnails' words and every other line of copy are VideoCopy.cpp's.
//
// The model steps by whole world days (midnight to midnight): a step that ends mid-day leaves the rest of the day for the
// next one, so any split of a span (a frame at a time, a night away, a 30-day skip) steps the same days and pays the same
// cents. Between steps the channel's numbers are read in closed form (SubsAt, View).
//
// Update 3 (core-videoalgo, with the creator update's §3): Rerun 2.0 on a channel whose ChannelX is on. Its uploads are
// Update 3 videos (VideoX): Update 2's P for the same choices (UploadPlanU2, with Fresh on Rerun's addend), then Update 3's
// compressed levers, MomC, the upload streak and the CTR noise; a curve of components (the first wave P0, then each
// decision's kick from VideoAlgo.cpp); per-day money at that day's ad load times the season; the rings, the engagement tails
// and the day-bucketed evergreen stubs. A legacy video (no VideoX) and a channel whose X is off step exactly as Update 2.

namespace ss
{
namespace video
{
// VideoCopy.cpp's lines (core-video's own file; declared here, not in a header, so nothing else depends on them).
namespace video_copy
{
std::string ReviewSubject();
std::string ReviewBody(double Subs, double Hours);
std::string WelcomeSubject();
std::string WelcomeBody();
std::string StatementSubject(const std::string& Month, bool Paid);
std::string StatementBody(const std::string& Month, Chips PaidCents, Chips BalanceCents, bool Paid);
std::string BellViews(const std::string& Title, long long Views);
std::string BellSubs(long long Subs);
std::string BellReview();
std::string BellPartner();
std::string PayoutLabel(const std::string& Month);
std::string MarkPayoutLabel();
std::string MarkReviewLabel();
std::string MonthName(int Month);
std::string GrantTitle();
std::string StorytimeEvent();
std::string WhyNot(int Reason);
} // namespace video_copy
// core-videocontent's itemized statement (VideoContentPriv.h's declaration, VideoCopy.cpp's body; §4.5.7).
namespace vcontent
{
std::string StatementU3(const std::string& Month, Chips WatchCents, double Rpm, double Cpm, Chips CutsCents, Chips PaidCents, Chips BalanceCents, bool Paid);
} // namespace vcontent

namespace video_detail
{
constexpr double Day = 1440.0;
constexpr size_t KeepOpen = 30;
constexpr double OpenDays = 21.0;
constexpr size_t KeepVideos = 60;
constexpr size_t KeepBell = 20;
constexpr size_t KeepPayKeys = 24;
constexpr double GateSubs = 1000.0;
constexpr double GateHours = 4000.0;
constexpr double ReviewDays = 7.0;
constexpr Chips PayoutMinCents = 10000;
constexpr int PayDay = 21;
static_assert(PayDay == valgo::VaPayDay, "the top-earner rate's pay month ends where the payout is (VideoAlgoPriv.h)");
constexpr double EvergreenDays = 365.0;

struct Shape
{
	double A, Tau1, Tau2;
};
Shape ShapeOf(Class C)
{
	switch (C)
	{
	case Class::Strategy: return {0.45, 2.0, 30.0};
	case Class::Short: return {0.90, 1.0, 6.0};
	case Class::News: break;
	}
	return {0.75, 1.5, 12.0};
}

/**
 * A format's row (§4.5, §4.6). Subs is FormatSubs in c (v1.2, the G2 audit's S2): a Short's viewers subscribe far less per
 * view than a long video's. 0.5 is the lowest value that keeps §4.6's three-a-week floors (the subscribers on day 90, the
 * gate's day): below it a creator whose rotation includes Shorts falls under them.
 */
struct FormatRow
{
	double LengthMin, Retention, EditMin, Energy, Rpm, Subs;
	bool Hours;
};
FormatRow RowOf(Format F)
{
	switch (F)
	{
	case Format::Short: return {0.75, 0.70, 20.0, 2.0, 0.03, 0.5, false};
	case Format::Vlog: return {18.0, 0.35, 180.0, 15.0, 1.4, 1.0, true};
	case Format::Recap: return {14.0, 0.40, 150.0, 12.0, 1.4, 1.0, true};
	case Format::Breakdown: break;
	}
	return {8.0, 0.45, 90.0, 8.0, 1.0, 1.0, true};
}

struct TitleRow
{
	double Ctr, Ret, Subs, AdFit;
};
TitleRow TitleOf(TitleStyle T)
{
	switch (T)
	{
	case TitleStyle::Hype: return {1.30, 0.85, 0.90, 0.9};
	case TitleStyle::Teaching: return {0.85, 1.25, 1.30, 1.2};
	case TitleStyle::Question: return {1.10, 1.00, 1.10, 1.0};
	case TitleStyle::Plain: return {0.90, 1.00, 1.00, 1.0};
	case TitleStyle::Story: break;
	}
	return {1.00, 1.10, 1.15, 1.0};
}

/** The thumbnail table's columns (§4.5): beats, reads, pots, runs, the rival, stories. */
int GroupOf(Kind K)
{
	switch (K)
	{
	case Kind::BadBeat:
	case Kind::Suckout:
	case Kind::Cooler:
	case Kind::SetOverSet:   // Update 3's kinds by what they are: a cooler,
	case Kind::RunnerRunner: return 0; // a suckout,
	case Kind::Bluff:
	case Kind::BluffCaught:
	case Kind::HeroCall:
	case Kind::HeroFold:
	case Kind::LadderFold: return 1; // a hero fold,
	case Kind::BigPot:
	case Kind::Knockout:
	case Kind::Bubble:
	case Kind::Monster:
	case Kind::DoubleKo:
	case Kind::MinCash:
	case Kind::Envelope:
	case Kind::BiggestPot: return 2; // pots, knockouts and the bubble,
	case Kind::DeepRun:
	case Kind::FinalTable:
	case Kind::Win:
	case Kind::HeadsUp: return 3; // a run for the title
	case Kind::Rival: return 4;
	case Kind::Story:
	case Kind::StreamMoment:
	case Kind::LastHand:
	case Kind::Idea:
	case Kind::Count: break;
	}
	return 5;
}

double ThumbFit(Frame F, int Group)
{
	static const double Fit[5][6] = {
		{1.3, 1.1, 1.1, 1.1, 1.0, 1.2}, // Reaction
		{1.2, 1.3, 1.0, 0.8, 0.9, 0.7}, // Cards
		{0.9, 0.8, 1.3, 1.2, 0.9, 1.0}, // Number
		{1.0, 1.0, 1.1, 0.9, 1.4, 0.8}, // Versus
		{0.7, 0.7, 0.8, 1.4, 0.9, 1.0}, // Trophy
	};
	const int Fr = std::clamp(static_cast<int>(F), 0, 4);
	return Fit[Fr][std::clamp(Group, 0, 5)];
}

/** The event, not a hand (§4.4: tournament-level). */
bool EventKind(Kind K)
{
	return K == Kind::DeepRun || K == Kind::FinalTable || K == Kind::Win;
}

double Quantize(double X, double Scale)
{
	return std::round(X * Scale) / Scale;
}

/** X to 10 significant digits: what a saved running sum keeps (the stub buckets, the engagement tails), so a reload reads the
 *  very numbers it wrote and the line stays short. */
double Sig10(double X)
{
	if (X == 0.0 || !std::isfinite(X))
	{
		return X;
	}
	const double Scale = std::pow(10.0, 9.0 - std::floor(std::log10(std::fabs(X))));
	return std::round(X * Scale) / Scale;
}

int DayIndex(double World)
{
	return static_cast<int>(std::floor(World / Day + 1e-9));
}

/** The night a moment belongs to (a night runs to 10 AM): a Vlog's moments are one night's. */
int NightKey(double At)
{
	return DayIndex(At - 10.0 * 60.0);
}

/**
 * "rl:mm-26@1530:214" -> "rl:mm-26@1530": the event a candidate came from (a Recap's hands are one event's). A live hand
 * the plan played through a skip is "lv:<entry>:skip:<n>" (BackRoomAuto.cpp): the same entry's, so ":skip" goes too.
 */
std::string EventKey(const std::string& Id)
{
	const size_t Cut = Id.rfind(':');
	std::string Key = Cut == std::string::npos ? Id : Id.substr(0, Cut);
	const size_t Skip = Key.size() >= 5 ? Key.size() - 5 : std::string::npos;
	if (Skip != std::string::npos && Key.compare(Skip, 5, ":skip") == 0)
	{
		Key.resize(Skip);
	}
	return Key;
}

double PotBbOf(const Candidate& C)
{
	if (EventKind(C.K) || C.Rec.Blinds.Bb <= 0 || C.Rec.Pot <= 0)
	{
		return 0.0;
	}
	return static_cast<double>(C.Rec.Pot) / static_cast<double>(C.Rec.Blinds.Bb);
}

/** Drama (§4.4): Studio's sort. */
double DramaOf(const Candidate& C)
{
	const double PotBb = PotBbOf(C);
	double D = 0.35 * KindWeight(C.K);
	if (PotBb > 0.0)
	{
		D += 0.25 * std::log10(PotBb / 20.0);
	}
	D += (C.Bubble || C.Final || C.Series) ? 0.10 : 0.0;
	D += C.Rival ? 0.10 : 0.0;
	D += (C.Stream.Viewers > 0 || C.Stream.ClipAt >= 0.0) ? 0.15 : 0.0;
	return Quantize(std::clamp(D, 0.0, 1.0), 10000.0);
}

// ---------------------------------------------------------------- reading a hand (only what the player saw)

/** What the hand's lines say about the player's part in it ("You" is the player, as the live recall writes it). */
struct HandRead
{
	bool HeroIn = false;          // dealt, with a line of their own
	bool HeroFolded = false;
	bool HeroAllIn = false;       // the player's chips all in (a shove, or calling one off)
	bool HeroLastAggressor = false; // the hand's last bet or raise was the player's
	bool HeroAggrOnRiver = false;
	bool HeroCalledRiverBet = false;
	bool HeroSawFlop = false;
	bool HeroFoldedToBig = false; // folded facing an all-in, or a bet of half the pot or more after the flop
	std::string Villain;          // the opponent most in it with the player
};

HandRead Read(const recall::Record& Rec)
{
	HandRead R;
	std::map<std::string, Chips> Put;   // street bets
	std::map<std::string, Chips> Total; // the hand's
	Chips Max = Rec.Blinds.Bb;
	Street Cur = Street::Preflop;
	double Pot = static_cast<double>(Rec.Blinds.Sb + Rec.Blinds.Bb + Rec.Blinds.Ante);
	bool LastAggrHero = false;
	bool LastAggrAllIn = false;
	double LastAggrAdded = 0.0, LastAggrPotBefore = 0.0;
	Street LastAggrStreet = Street::Preflop;
	std::string LastAggrWho;
	for (const recall::Line& L : Rec.Lines)
	{
		if (L.On != Cur)
		{
			Cur = L.On;
			Put.clear();
			Max = 0;
		}
		const bool Hero = L.Who == "You";
		const Chips Before = Put[L.Who];
		const bool Folds = L.What == "folds";
		const bool Checks = L.What == "checks";
		bool Aggr = L.What == "bets" || L.What == "raises to";
		bool Calls = L.What == "calls";
		if (L.What == "all in")
		{
			Aggr = L.To > Max;
			Calls = !Aggr;
		}
		const Chips Added = Folds || Checks ? 0 : std::max<Chips>(0, L.To - Before);
		if (Hero)
		{
			R.HeroIn = true;
			R.HeroSawFlop = R.HeroSawFlop || (L.On >= Street::Flop && !R.HeroFolded);
			R.HeroAllIn = R.HeroAllIn || L.What == "all in";
			if (Folds)
			{
				R.HeroFolded = true;
				const bool Facing = LastAggrStreet == L.On && !LastAggrWho.empty() && LastAggrWho != "You";
				if (Facing && (LastAggrAllIn || (L.On >= Street::Flop && LastAggrPotBefore > 0.0 && LastAggrAdded >= 0.5 * LastAggrPotBefore)))
				{
					R.HeroFoldedToBig = true;
				}
			}
			if (Calls && L.On == Street::River && LastAggrStreet == Street::River && !LastAggrHero && !LastAggrWho.empty())
			{
				R.HeroCalledRiverBet = true;
			}
		}
		if (Aggr)
		{
			LastAggrHero = Hero;
			LastAggrAllIn = L.What == "all in";
			LastAggrAdded = static_cast<double>(Added);
			LastAggrPotBefore = Pot;
			LastAggrStreet = L.On;
			LastAggrWho = L.Who;
			if (Hero && L.On == Street::River)
			{
				R.HeroAggrOnRiver = true;
			}
		}
		Pot += static_cast<double>(Added);
		Put[L.Who] = std::max(Before, Folds || Checks ? Before : L.To);
		Total[L.Who] += Added;
		Max = std::max(Max, L.To);
	}
	R.HeroLastAggressor = LastAggrHero && !LastAggrWho.empty();
	R.HeroSawFlop = R.HeroSawFlop || (R.HeroIn && !R.HeroFolded && Rec.Board.size() >= 3);
	// The villain: a hand shown down first, else whoever put the most in beside the player.
	if (!Rec.Shown.empty())
	{
		R.Villain = Rec.Shown.front().first;
	}
	else
	{
		Chips Most = -1;
		for (const auto& T : Total)
		{
			if (T.first != "You" && T.second > Most)
			{
				Most = T.second;
				R.Villain = T.first;
			}
		}
	}
	return R;
}

/** The player's own made hand on the board: 0 nothing of their own, 1 bottom pair or an underpair, 2 one better pair, 3 two pair or better. */
int MadeLevel(const std::vector<Card>& Mine, const std::vector<Card>& Board)
{
	if (Mine.size() < 2 || Board.size() < 3)
	{
		return 0;
	}
	const int Score = EvaluateHand(Mine, Board);
	if (Board.size() >= 5 && Score <= Evaluate(Board.data(), 5))
	{
		return 0; // playing the board
	}
	int BoardCount[13] = {};
	int Low = 12, High = 0;
	for (const Card C : Board)
	{
		++BoardCount[RankOf(C)];
		Low = std::min(Low, RankOf(C));
		High = std::max(High, RankOf(C));
	}
	bool BoardPaired = false;
	for (const int N : BoardCount)
	{
		BoardPaired = BoardPaired || N >= 2;
	}
	const int Cat = CategoryOf(Score);
	if (Cat >= static_cast<int>(Category::TwoPair) && !(Cat == static_cast<int>(Category::TwoPair) && BoardPaired))
	{
		return 3;
	}
	if (Cat == static_cast<int>(Category::HighCard))
	{
		return 0;
	}
	// One pair (or two pair with one of them on the board): the player's own pair.
	const int A = RankOf(Mine[0]), B = RankOf(Mine[1]);
	if (A == B)
	{
		return A < Low ? 1 : 2;
	}
	int Own = -1;
	for (const int R : {A, B})
	{
		if (BoardCount[R] >= 1)
		{
			Own = std::max(Own, R);
		}
	}
	if (Own < 0)
	{
		return 0;
	}
	return Own == Low ? 1 : 2;
}

/** The category of a hand made with its own cards (0 when it only plays the board). */
int OwnCategory(const std::vector<Card>& Hole, const std::vector<Card>& Board)
{
	if (Hole.size() < 2 || Board.size() < 3)
	{
		return 0;
	}
	const int Score = EvaluateHand(Hole, Board);
	if (Board.size() >= 5 && Score <= Evaluate(Board.data(), 5))
	{
		return 0;
	}
	return CategoryOf(Score);
}

/** Equity at the all-in, stateless (§2.5): exact from the flop on, 2,000 samples on a seed of the hand's own preflop. */
double EquityAt(const std::vector<Card>& Mine, const std::vector<std::vector<Card>>& Theirs, const std::vector<Card>& Board, const std::string& Seed)
{
	if (Mine.size() < 2 || Theirs.empty())
	{
		return -1.0;
	}
	std::vector<std::vector<Card>> Hands{Mine};
	for (const std::vector<Card>& H : Theirs)
	{
		if (H.size() < 2)
		{
			return -1.0;
		}
		Hands.push_back(H);
	}
	Rng R("video:eq:" + Seed);
	const std::vector<double> Eq = EquityKnownHands(Hands, Board, R, 2000);
	return Eq.empty() ? -1.0 : Quantize(Eq[0], 10000.0);
}

// ---------------------------------------------------------------- the channel's sums

/** Views gained between T0 and T1 by the folded tails (their sums are as of C.SteppedTo). */
void TailGain(const Channel& C, double T0, double T1, double Out[3])
{
	// Read back before the last step, the tails' closed form holds only once every folded video was up: from the oldest
	// kept upload on (every folded video went up before it). A channel posting more than two a day folds videos younger
	// than a month, and read back past their uploads the curves run backwards (e^(age/tau) of views that never happened).
	// That stretch is left out: a little under for such a channel, never over. Forward reads (the steps, SubsAt) and every
	// channel whose folded videos are older than the read are as they were, to the bit.
	double Exact = C.SteppedTo;
	for (const Video& V : C.Videos)
	{
		Exact = std::min(Exact, V.At);
	}
	T0 = std::max(T0, Exact);
	if (T1 <= T0)
	{
		return;
	}
	for (int K = 0; K < 3; ++K)
	{
		const Shape S = ShapeOf(static_cast<Class>(K));
		const Tail& T = C.Tails[static_cast<size_t>(K)];
		for (int J = 0; J < 2; ++J)
		{
			const double Tau = J == 0 ? S.Tau1 : S.Tau2;
			const double F0 = std::exp(-(T0 - C.SteppedTo) / Day / Tau);
			const double F1 = std::exp(-(T1 - C.SteppedTo) / Day / Tau);
			for (int W = 0; W < 3; ++W)
			{
				Out[W] += T.Decay[J][W] * (F0 - F1);
			}
		}
	}
}

/** Update 3: likes, dislikes, comments and impressions the folded engagement tails gain between T0 and T1 (TailGain's rule). */
void TailXGain(const Channel& C, double T0, double T1, double Out[4])
{
	double Exact = C.SteppedTo;
	for (const Video& V : C.Videos)
	{
		Exact = std::min(Exact, V.At);
	}
	T0 = std::max(T0, Exact);
	if (T1 <= T0)
	{
		return;
	}
	for (int K = 0; K < 3; ++K)
	{
		const Shape S = ShapeOf(static_cast<Class>(K));
		const TailX& T = C.X.TailsX[static_cast<size_t>(K)];
		for (int J = 0; J < 2; ++J)
		{
			const double Tau = J == 0 ? S.Tau1 : S.Tau2;
			const double F0 = std::exp(-(T0 - C.SteppedTo) / Day / Tau);
			const double F1 = std::exp(-(T1 - C.SteppedTo) / Day / Tau);
			for (int W = 0; W < 4; ++W)
			{
				Out[W] += T.Decay[J][W] * (F0 - F1);
			}
		}
	}
}

/** Views an evergreen stub gains between T0 and T1. */
double StubGain(const Stub& S, double T0, double T1)
{
	const double From = std::max(T0, S.At);
	const double To = std::min(T1, S.At + EvergreenDays * Day);
	return To > From ? S.E * (To - From) / Day : 0.0;
}

/** The part of [T0, T1] (in days) that falls on world days From .. To - 1. */
double DaysCovered(double T0, double T1, int From, int To)
{
	const double A = std::max(T0, static_cast<double>(From) * Day), B = std::min(T1, static_cast<double>(To) * Day);
	return B > A ? (B - A) / Day : 0.0;
}

/**
 * Update 3 (§4.8's legacy plan, vstubd): the evergreen videos that folded on one upload day, summed. Each runs for 365 days
 * from its own minute, so over whole days the bucket is exact: its upload day gains E - EF, every day after it E, and the
 * 365th day after it EF (the F sums weight each video by its minute offset / 1,440). Inside a first or last day it reads
 * linearly. Out += views, subscribers, the RPM weight.
 */
void StubDayGain(const StubDay& S, double T0, double T1, double Out[3])
{
	const double Whole = DaysCovered(T0, T1, S.Day, S.Day + 365);
	const double First = DaysCovered(T0, T1, S.Day, S.Day + 1);
	const double Last = DaysCovered(T0, T1, S.Day + 365, S.Day + 366);
	Out[0] += S.E * Whole - S.EF * First + S.EF * Last;
	Out[1] += S.EConv * Whole - S.EConvF * First + S.EConvF * Last;
	Out[2] += S.ERpm * Whole - S.ERpmF * First + S.ERpmF * Last;
}

/** Views, subscribers, ad dollars (when Paying) and watch hours gained between T0 and T1, over everything on the channel. */
struct Gains
{
	double Views = 0, Subs = 0, Ad = 0, Hours = 0;
};
Gains GainsBetween(const Channel& C, double T0, double T1, bool Paying)
{
	Gains G;
	if (T1 <= T0)
	{
		return G;
	}
	// (An Update 3 video's rates are its load's on T0's day: exact for a day's step, the reads' closed form across a switch.)
	const int Day0 = DayIndex(T0);
	for (const Video& V : C.Videos)
	{
		const double Gain = std::max(0.0, Views(V, T1) - Views(V, T0));
		G.Views += Gain;
		G.Subs += valgo::ConvOn(V, Day0) * Gain;
		G.Ad += Paying ? valgo::RpmOn(V, Day0) / 1000.0 * Gain : 0.0;
		G.Hours += RowOf(V.F).Hours ? Gain * V.LengthMin / 60.0 * valgo::RetentionOn(V, Day0) : 0.0;
	}
	double Tail[3] = {0.0, 0.0, 0.0};
	TailGain(C, T0, T1, Tail);
	G.Views += Tail[0];
	G.Subs += Tail[1];
	G.Ad += Paying ? Tail[2] / 1000.0 : 0.0;
	for (const Stub& S : C.Stubs)
	{
		const double Gain = StubGain(S, T0, T1);
		G.Views += Gain;
		G.Subs += S.Conv * Gain;
		G.Ad += Paying ? S.Rpm / 1000.0 * Gain : 0.0;
	}
	double Bucket[3] = {0.0, 0.0, 0.0};
	for (const StubDay& S : C.X.StubDays)
	{
		StubDayGain(S, T0, T1, Bucket);
	}
	G.Views += Bucket[0];
	G.Subs += Bucket[1];
	G.Ad += Paying ? Bucket[2] / 1000.0 : 0.0;
	return G;
}

/** The watch hours of days From..To (world days, inclusive) still in the ring. */
double RingSum(const Channel& C, int From, int To)
{
	double Sum = 0.0;
	for (int D = std::max(From, C.HoursDay0); D <= To && D <= C.HoursDay0 + 365; ++D)
	{
		Sum += static_cast<double>(C.HoursByDay[static_cast<size_t>(D - C.HoursDay0)]);
	}
	return Sum;
}

void RingAdd(Channel& C, int DayNo, double Hours)
{
	if (DayNo > C.HoursDay0 + 365)
	{
		const int Shift = DayNo - (C.HoursDay0 + 365);
		if (Shift >= 366)
		{
			C.HoursByDay.fill(0.0f);
		}
		else
		{
			for (int I = 0; I < 366; ++I)
			{
				C.HoursByDay[static_cast<size_t>(I)] = I + Shift < 366 ? C.HoursByDay[static_cast<size_t>(I + Shift)] : 0.0f;
			}
		}
		C.HoursDay0 += Shift;
	}
	if (DayNo >= C.HoursDay0 && Hours > 0.0)
	{
		C.HoursByDay[static_cast<size_t>(DayNo - C.HoursDay0)] += static_cast<float>(Hours);
	}
}

/** First touch: the day the channel's steps start from (nothing before it was ever on the channel). */
void Anchor(Channel& C, double World)
{
	if (C.SteppedTo <= 0.0)
	{
		C.SteppedTo = static_cast<double>(DayIndex(World)) * Day;
		C.HoursDay0 = std::max(0, DayIndex(World) - 365);
	}
}

/** Every component of an Update 3 video has started by C.SteppedTo (a fold takes a curve's ages from then). */
bool StartedBy(const Video& V, double T)
{
	return V.At <= T && std::all_of(V.X.Comps.begin(), V.X.Comps.end(), [T](const Comp& K) { return K.T <= T; });
}

/**
 * Update 3 (§4.4 "The fold"): an Update 3 video folds every part of its curve (the first wave and each component) with its
 * own age into its class's tails: Update 2's (views, subscribers, the RPM weight, at the video's rates on the fold's day)
 * and the engagement tails (likes, dislikes, comments, impressions). An evergreen one joins its upload day's stub bucket;
 * the lifetime top 20 keeps it by name.
 */
void FoldU3(Channel& C, const Video& V)
{
	const double At = C.SteppedTo;
	const int Today = DayIndex(At);
	const Shape S = ShapeOf(V.C);
	Tail& T = C.Tails[static_cast<size_t>(V.C)];
	TailX& Tx = C.X.TailsX[static_cast<size_t>(V.C)];
	const double Conv = valgo::ConvOn(V, Today), Rpm = valgo::RpmOn(V, Today), Ret = valgo::RetentionOn(V, Today);
	const int Over = valgo::OverOf(V);
	const valgo::Rates R = valgo::RatesOf(V, valgo::LoadOn(V.X, Today), Ret, Over);
	const double W[3] = {1.0, Conv, Rpm};
	double Total = 0.0;
	auto Part = [&](double Start, double P, double A, uint8_t Mix, float CtrF) {
		const double Age = (At - Start) / Day;
		const double Fast = A * std::exp(-Age / S.Tau1), Slow = (1.0 - A) * std::exp(-Age / S.Tau2);
		for (int K = 0; K < 3; ++K)
		{
			T.Total[K] += P * W[K];
			T.Decay[0][K] += P * W[K] * Fast;
			T.Decay[1][K] += P * W[K] * Slow;
		}
		const double X4[4] = {R.Like, R.Dislike(), R.Comment, valgo::ImprPerView(V, Mix, CtrF)};
		for (int K = 0; K < 4; ++K)
		{
			Tx.Total[K] += P * X4[K];
			Tx.Decay[0][K] += P * X4[K] * Fast;
			Tx.Decay[1][K] += P * X4[K] * Slow;
		}
		Total += P;
	};
	Part(V.At, V.X.P0, S.A, 0, 1.0f);
	for (const Comp& K : V.X.Comps)
	{
		Part(K.T, K.P, K.A, K.SrcMix, K.CtrF);
	}
	// (The engagement tails kept to 10 significant digits, as vtailx writes them: C3's compaction.)
	for (int K = 0; K < 4; ++K)
	{
		Tx.Total[K] = Sig10(Tx.Total[K]);
		Tx.Decay[0][K] = Sig10(Tx.Decay[0][K]);
		Tx.Decay[1][K] = Sig10(Tx.Decay[1][K]);
	}
	if (V.E > 0.0)
	{
		const double Age = (At - V.At) / Day;
		if (Age >= EvergreenDays)
		{
			for (int K = 0; K < 3; ++K)
			{
				T.Total[K] += V.E * EvergreenDays * W[K];
			}
		}
		else
		{
			const int UpDay = DayIndex(V.At);
			auto It = std::find_if(C.X.StubDays.begin(), C.X.StubDays.end(), [UpDay](const StubDay& Sd) { return Sd.Day == UpDay; });
			if (It == C.X.StubDays.end())
			{
				StubDay Sd;
				Sd.Day = UpDay;
				It = C.X.StubDays.insert(std::upper_bound(C.X.StubDays.begin(), C.X.StubDays.end(), Sd, [](const StubDay& A, const StubDay& B) { return A.Day < B.Day; }), Sd);
			}
			const double Off = (V.At - static_cast<double>(UpDay) * Day) / Day;
			// (Each sum kept to 8 significant digits, so the line stays short and a reload reads the very numbers it wrote: C3's
			// compaction from 10; a stub's year of views moves by under 1e-7 of itself.)
			auto Sig = [](double X) {
				if (X == 0.0)
				{
					return 0.0;
				}
				const double Scale = std::pow(10.0, 7.0 - std::floor(std::log10(std::fabs(X))));
				return std::round(X * Scale) / Scale;
			};
			It->E = Sig(It->E + V.E);
			It->EConv = Sig(It->EConv + V.E * Conv);
			It->ERpm = Sig(It->ERpm + V.E * Rpm);
			It->EF = Sig(It->EF + V.E * Off);
			It->EConvF = Sig(It->EConvF + V.E * Conv * Off);
			It->ERpmF = Sig(It->ERpmF + V.E * Rpm * Off);
		}
		Total += V.E * EvergreenDays;
	}
	if (!valgo::IsGrant(V))
	{
		TopRow Row;
		Row.Id = V.Id;
		Row.Title = V.Title;
		Row.Views = Quantize(Total, 100.0);
		Row.Cents = static_cast<Chips>(std::llround(V.X.Earned));
		Row.At = V.At;
		C.X.Top.push_back(Row);
		std::stable_sort(C.X.Top.begin(), C.X.Top.end(), [](const TopRow& A, const TopRow& B) { return A.Views > B.Views; });
		if (C.X.Top.size() > 20)
		{
			C.X.Top.resize(20);
		}
	}
}

/** A video leaves the newest Keep: its curve folds into its class's tail as of C.SteppedTo; an evergreen video keeps a stub. */
void Fold(Channel& C, size_t Keep)
{
	while (C.Videos.size() > Keep && C.Videos.front().At <= C.SteppedTo)
	{
		if (C.Videos.front().X.On)
		{
			if (!StartedBy(C.Videos.front(), C.SteppedTo))
			{
				break; // (a component still to start: it folds at the next day's step)
			}
			const Video V = C.Videos.front();
			C.Videos.erase(C.Videos.begin());
			FoldU3(C, V);
			continue;
		}
		const Video V = C.Videos.front();
		C.Videos.erase(C.Videos.begin());
		const double Age = (C.SteppedTo - V.At) / Day;
		Tail& T = C.Tails[static_cast<size_t>(V.C)];
		const double W[3] = {1.0, V.Conv, V.Rpm};
		for (int K = 0; K < 3; ++K)
		{
			T.Total[K] += V.P * W[K];
			T.Decay[0][K] += V.P * W[K] * V.A * std::exp(-Age / V.Tau1);
			T.Decay[1][K] += V.P * W[K] * (1.0 - V.A) * std::exp(-Age / V.Tau2);
		}
		if (V.E > 0.0)
		{
			if (Age >= EvergreenDays)
			{
				for (int K = 0; K < 3; ++K)
				{
					T.Total[K] += V.E * EvergreenDays * W[K];
				}
			}
			else
			{
				Stub S;
				S.Id = V.Id;
				S.At = V.At;
				S.E = V.E;
				S.Conv = V.Conv;
				S.Rpm = V.Rpm;
				C.Stubs.push_back(S);
			}
		}
	}
}

void Bell(Channel& C, double At, const std::string& Line)
{
	C.Bell.push_back({At, Line});
	if (C.Bell.size() > KeepBell)
	{
		C.Bell.erase(C.Bell.begin(), C.Bell.begin() + static_cast<std::ptrdiff_t>(C.Bell.size() - KeepBell));
	}
}

void Mail(life::State& L, double At, const std::string& Thread, int Kind, const std::string& Subject, const std::string& Body)
{
	life::Letter M;
	M.At = At;
	M.Thread = Thread;
	M.Source = "video";
	M.From = "Rerun";
	M.Subject = Subject;
	M.Body = Body;
	M.Kind = Kind;
	life::Post(L, M);
}

/** A routine letter went out from Rerun less than a week before At (§1.7: one a week at most). */
bool WroteThisWeek(const life::State& L, double At)
{
	for (const life::Letter& M : L.Inbox)
	{
		if (M.Source == "video" && M.At > At - 7.0 * Day)
		{
			return true;
		}
	}
	return false;
}

void CountUploads(Channel& C, double World)
{
	C.Uploads14 = 0;
	C.Uploads7 = 0;
	for (const Video& V : C.Videos)
	{
		if (V.At > World - 14.0 * Day && V.At <= World && V.Title != video_copy::GrantTitle())
		{
			++C.Uploads14;
			C.Uploads7 += V.At > World - 7.0 * Day ? 1 : 0;
		}
	}
}

/**
 * Update 3: the day's engagement for the rings (likes, dislikes, comments, impressions, subscribers lost), and its ad money's
 * split (§4.5.7, §5.6): the Cuts feed's dollars (Class::Short, kept and folded: the tails are per class, so the split is exact),
 * the watch page's from the kept long videos with their ad impressions and monetized playbacks, and the folded long tails' and
 * stubs' dollars, whose impressions the day's kept long videos' ratio stands for (a folded video keeps no ads of its own).
 */
struct DayX
{
	double Likes = 0, Dislikes = 0, Comments = 0, Impr = 0, Lost = 0, Watch = 0;
	double CutAd = 0, LongAd = 0, LongImpr = 0, LongMon = 0, FoldedAd = 0; // dollars at season 1 (Paying days only)
	double CutViews = 0;                                                   // the Cuts' views (kept and folded)
	Chips CutCents = 0;
	double AdImpr = 0, Monetized = 0; // the day's, once DayStep has its cents
	Chips Held = 0, HeldCut = 0;      // what the top-earner rate held back of the day's cents, and of the Cuts feed's (C3 fix H1)
};

/** X rounded as a ring keeps it: whole numbers from 100 up, tenths below (display numbers; the save stays short). */
double RowRound(double X)
{
	return Quantize(X, std::fabs(X) >= 100.0 ? 1.0 : 10.0);
}

/** A ring row's fields from a day's sums (U3RowText's order), kept to a tenth (display numbers; the save stays short). */
void AddToRow(DayRow& R, const Gains& G, const DayX& Dx, Chips Cents)
{
	auto Add = [](float& Field, double X) { Field = static_cast<float>(RowRound(static_cast<double>(Field) + X)); };
	Add(R.Views, G.Views);
	Add(R.WatchH, Dx.Watch);
	Add(R.Gained, G.Subs + Dx.Lost);
	Add(R.Lost, Dx.Lost);
	Add(R.Likes, Dx.Likes);
	Add(R.Dislikes, Dx.Dislikes);
	Add(R.Impr, Dx.Impr);
	Add(R.Comments, Dx.Comments);
	R.Cents += Cents;
	if (Cents > 0 || Dx.CutCents > 0 || Dx.Held > 0)
	{
		valgo::VaSetRowSplit(R, valgo::VaRowCut(R) + Dx.CutCents, static_cast<float>(RowRound(valgo::VaRowCutViews(R) + Dx.CutViews)),
			static_cast<float>(RowRound(valgo::VaRowImpr(R) + Dx.AdImpr)), static_cast<float>(RowRound(valgo::VaRowMonetized(R) + Dx.Monetized)));
	}
	if (Dx.Held > 0)
	{
		valgo::VaSetRowHeld(R, valgo::VaRowHeld(R) + Dx.Held, valgo::VaRowHeldCut(R) + Dx.HeldCut);
	}
}

/**
 * TrendCool's bound (12, Video.h): past it, the entry whose window closes first goes (a trend over already, a count or a bell
 * mark, before one still open), never simply the oldest: a week's bells must not push out a long trend's count, or its third
 * matching upload would ride it again (§4.6.10).
 */
void TrimTrendCool(ChannelX& X)
{
	while (X.TrendCool.size() > 12)
	{
		X.TrendCool.erase(std::min_element(X.TrendCool.begin(), X.TrendCool.end(), [](const Cool& A, const Cool& B) { return A.LastDay < B.LastDay; }));
	}
}

/**
 * Update 3's midnight (§4.7's steps 2 and 5-7, with SPEC_CREATORS §3.3): the rings (a row a day for 90 days, a row a month
 * for 24, the lifetime row), the verdicts and check-ins (two-phase, VideoAlgo.cpp), the upload streak on a Monday (a
 * missed week halves it unless a Rain Check covers it), and Momentum's drift after 21 idle days.
 */
void MidnightU3(Channel& C, life::State& L, double D0, const Gains& G, const DayX& Dx, Chips DayCents, const world::World* W)
{
	ChannelX& X = C.X;
	const int DayNo = DayIndex(D0);
	const int Next = DayNo + 1;
	DayRow Row;
	Row.Key = DayNo;
	AddToRow(Row, G, Dx, DayCents);
	X.Days.push_back(Row);
	if (X.Days.size() > 90)
	{
		X.Days.erase(X.Days.begin(), X.Days.begin() + static_cast<std::ptrdiff_t>(X.Days.size() - 90));
	}
	int Y = 0, Mo = 0, Dm = 0;
	world::CivilDate(DayNo, Y, Mo, Dm);
	const int MonthKey = Y * 12 + (Mo - 1);
	if (X.Months.empty() || X.Months.back().Key != MonthKey)
	{
		DayRow M;
		M.Key = MonthKey;
		X.Months.push_back(M);
		if (X.Months.size() > 24)
		{
			X.Months.erase(X.Months.begin());
		}
	}
	AddToRow(X.Months.back(), G, Dx, DayCents);
	AddToRow(X.Life, G, Dx, DayCents);
	const double D1 = static_cast<double>(Next) * Day;
	{
		// The Binge wave's and Late Bloomer's bells (C3 fix L4) ring at the midnight that ends their first day, with what it
		// really did: the views the wave's parts added on the older videos, and the woken video's day against its usual one (the
		// 7 days before). Both come from the parts that started at this day's first midnight: nothing saved, a skip the same.
		int Binged = 0;
		double BingeViews = 0.0;
		for (const Video& V : C.Videos)
		{
			if (!V.X.On)
			{
				continue;
			}
			const valgo::VaShape S = valgo::ShapeU3(V.C);
			bool Took = false, Woke = false;
			for (const Comp& K : V.X.Comps)
			{
				if (K.T != D0)
				{
					continue;
				}
				if (K.Kind == CompKind::Binge)
				{
					BingeViews += K.P * valgo::CurveFrac(K.A, S.Tau1, S.Tau2, 1.0);
					Took = true;
				}
				Woke = Woke || K.Kind == CompKind::LateBloom;
			}
			Binged += Took ? 1 : 0;
			if (Woke)
			{
				const double Usual = (Views(V, D0) - Views(V, D0 - 7.0 * Day)) / 7.0;
				Bell(C, D1, valgo::BellBloom(V.Title, Usual > 0.0 ? (Views(V, D1) - Views(V, D0)) / Usual : 0.0));
			}
		}
		if (Binged > 0)
		{
			Bell(C, D1, valgo::BellBinge(Binged, BingeViews));
		}
	}
	// The decisions of the midnight that ends this day.
	algo::DecideDay(C, L, Next, nullptr);
	for (size_t I = 0; I < C.Videos.size(); ++I)
	{
		const Video& V = C.Videos[I];
		if (!V.X.On || valgo::IsGrant(V) || V.At + Day > D1 || V.At + Day <= D1 - Day)
		{
			continue;
		}
		// Its first day just ended (§6.6.5): better at 24 hours than 8 of the last 10 uploads were at theirs is bell-worthy.
		const double Mine = Views(V, V.At + Day);
		int Seen = 0, Beat = 0;
		for (size_t J = I; J-- > 0 && Seen < 10;)
		{
			if (valgo::IsGrant(C.Videos[J]))
			{
				continue;
			}
			++Seen;
			Beat += Mine > Views(C.Videos[J], C.Videos[J].At + Day) ? 1 : 0;
		}
		if (Seen >= 5 && Beat * 10 >= Seen * 8)
		{
			Bell(C, D1, valgo::BellEarly(V.Title));
		}
	}
	// The first Limited ads coin brings one Mail notice, ever (§4.5.4; its key in video\tpaid).
	for (const Video& V : C.Videos)
	{
		if (V.X.On && V.X.C == Coin::Limited && V.At <= D1 && C.Paid.insert("notice:limited").second)
		{
			Mail(L, D1, "rerun:limited", valgo::VaLetterLimited, valgo::LimitedSubject(), valgo::LimitedBody(V.Title));
		}
	}
	// A new trend rings the bell, one a day (§6.6.5), the hottest first: one whose window opened in the last week (the
	// calendar's and the network's) or that this week's list took in from the world's log, once (TrendCool keeps a "bell:"
	// mark for it until its window has closed).
	for (const Trend& T : valgo::TrendsNow(L, W, D1))
	{
		valgo::VaTrend Tv;
		const bool FromLog = T.Key.rfind("champ:", 0) == 0 || T.Key.rfind("final:", 0) == 0 || T.Key.rfind("site:", 0) == 0 || T.Key.rfind("overlay:", 0) == 0;
		if (!valgo::TrendOfKey(T.Key, Tv) || (!FromLog && Tv.Start < D1 - 7.0 * Day))
		{
			continue;
		}
		const int Mark = valgo::TrendCoolKey("bell:" + T.Key);
		if (std::any_of(X.TrendCool.begin(), X.TrendCool.end(), [Mark](const Cool& Cl) { return Cl.Key == Mark; }))
		{
			continue;
		}
		Bell(C, D1, valgo::BellTrend(T.Label));
		Cool Rung;
		Rung.Key = Mark;
		Rung.LastDay = DayIndex(Tv.End - 1.0);
		X.TrendCool.push_back(Rung);
		TrimTrendCool(X);
		break;
	}
	// The upload streak (SPEC_CREATORS §3.3): Monday 00:00 counts the week that just ended.
	if (Next % 7 == 0)
	{
		const int Week = Next / 7 - 1;
		if (X.StreakWeek < Week)
		{
			const bool Uploaded = X.LastUploadDay >= 0 && X.LastUploadDay / 7 == Week;
			if (Uploaded)
			{
				const int Held = X.RainChecks;
				X.StreakW = std::min(valgo::VaStreakMax, X.StreakW + 1);
				X.RainChecks = X.StreakW % valgo::VaRainEvery == 0 ? std::min(valgo::VaRainMax, X.RainChecks + 1) : X.RainChecks;
				if (X.RainChecks > Held)
				{
					Bell(C, D1, valgo::BellRainCheck(X.StreakW));
				}
			}
			else if (X.RainChecks > 0)
			{
				--X.RainChecks;
			}
			else
			{
				X.StreakW /= 2;
			}
			X.StreakWeek = Week;
		}
		// The week's trends start over (the cooldowns count a trend's matching uploads inside its window).
		X.TrendCool.erase(std::remove_if(X.TrendCool.begin(), X.TrendCool.end(), [Next](const Cool& Cl) { return Cl.LastDay < Next - 14; }), X.TrendCool.end());
		X.TrendWeek = Next / 7;
	}
	// Dormancy: after 21 idle days, H drifts 5% a day toward 0 ("Rerun hasn't seen anything from you lately").
	if (X.LastUploadDay >= 0 && Next - X.LastUploadDay > valgo::VaDormantDays)
	{
		X.MomH = valgo::VaRound(X.MomH * valgo::VaMomDormant, 1e6);
		X.Momentum = valgo::MomCOf(X.MomH);
	}
}

/** One world day, D0 (a midnight) to D0 + a day. */
void DayStep(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double D0, size_t Keep, const world::World* TheWorld)
{
	const double D1 = D0 + Day;
	const int DayNo = DayIndex(D0);
	const bool U3 = C.X.On;
	if (U3)
	{
		// Update 3 (§4.5.5): an ad-load switch takes effect at the start of its day, before that day's gains.
		for (Video& V : C.Videos)
		{
			valgo::ApplyLoad(V, DayNo);
		}
	}
	Fold(C, Keep);
	const bool Paying = C.PartnerAt >= 0.0 && D0 >= C.PartnerAt - 1e-6;
	// Update 3 (§4.5.3): Rerun 2.0's ads pay the day's season (one factor for the day, so the folded tails stay exact).
	const double Sea = U3 ? algo::Season(DayNo) : 1.0;
	DayX Dx;
	// The videos kept whole: their milestones too.
	Gains G;
	// (Each kept Update 3 video's double cents for the day, added once the day's total is known: the top-earner rate scales a
	// day past T for every video alike. Under T the add is today's, to the bit.)
	std::vector<std::pair<Video*, double>> Earn;
	if (Paying && U3)
	{
		Earn.reserve(C.Videos.size());
	}
	for (Video& V : C.Videos)
	{
		// (An Update 3 video's curve is read once for its views at both ends and its impressions: SpanU3, ViewsU3's own numbers.)
		double Was = 0.0, Now = 0.0, SpanImpr = -1.0;
		if (V.X.On)
		{
			valgo::SpanU3(V, D0, D1, Was, Now, SpanImpr);
		}
		else
		{
			Was = Views(V, D0);
			Now = Views(V, D1);
		}
		const double Gain = std::max(0.0, Now - Was);
		G.Views += Gain;
		G.Subs += valgo::ConvOn(V, DayNo) * Gain;
		G.Ad += Paying ? valgo::RpmOn(V, DayNo) / 1000.0 * Gain : 0.0;
		G.Hours += RowOf(V.F).Hours ? Gain * V.LengthMin / 60.0 * valgo::RetentionOn(V, DayNo) : 0.0;
		if (U3 && Gain > 0.0)
		{
			// The video's own double cents (§4.5.2) and the rings' engagement: counts arrive with the views.
			if (Paying && V.X.On)
			{
				Earn.push_back({&V, valgo::RpmOn(V, DayNo) / 1000.0 * Gain * 100.0 * Sea});
			}
			if (Paying)
			{
				// The split (§4.5.7): a Cut's money is the Cuts feed's; a long video's carries its ad impressions and monetized
				// playbacks at the day's load and coin (MoneyOf's MonetizedRate and AdsPerPlayback).
				const double Ad = valgo::RpmOn(V, DayNo) / 1000.0 * Gain;
				if (V.C == Class::Short)
				{
					Dx.CutAd += Ad;
					Dx.CutViews += Gain;
				}
				else
				{
					const double Mon = V.X.On && V.X.C == Coin::Limited ? valgo::VaMonetizedLimited : valgo::VaMonetized;
					const double Per = algo::AdsPerPlayback(V.F, V.X.On ? V.X.Len : LengthOpt::Standard, V.X.On ? valgo::LoadOn(V.X, DayNo) : AdLoad::Standard);
					Dx.LongAd += Ad;
					Dx.LongImpr += Gain * Mon * Per;
					Dx.LongMon += Gain * Mon;
				}
			}
			const int Over = valgo::OverOf(V);
			const double Ret = valgo::RetentionOn(V, DayNo);
			const AdLoad Load = V.X.On ? valgo::LoadOn(V.X, DayNo) : AdLoad::Standard;
			const valgo::Rates R = valgo::RatesOf(V, Load, Ret, Over);
			Dx.Likes += R.Like * Gain;
			Dx.Dislikes += R.Dislike() * Gain;
			Dx.Comments += R.Comment * Gain;
			Dx.Impr += SpanImpr >= 0.0 ? SpanImpr : valgo::ImprGain(V, D0, D1);
			Dx.Lost += valgo::LostShare(valgo::QOf(V, R, Ret, Over), Over, Load == AdLoad::Heavy) * valgo::ConvOn(V, DayNo) * Gain;
			Dx.Watch += Gain * V.LengthMin / 60.0 * Ret;
		}
		for (const double M : {100.0, 1000.0, 10000.0, 100000.0})
		{
			if (Was < M && Now >= M && V.Title != video_copy::GrantTitle())
			{
				Bell(C, D1, video_copy::BellViews(V.Title, static_cast<long long>(M)));
			}
		}
	}
	// The tails, rebased to D1.
	for (int Cl = 0; Cl < 3; ++Cl)
	{
		const Shape S = ShapeOf(static_cast<Class>(Cl));
		Tail& T = C.Tails[static_cast<size_t>(Cl)];
		TailX& Tx = C.X.TailsX[static_cast<size_t>(Cl)];
		for (int J = 0; J < 2; ++J)
		{
			const double F = std::exp(-1.0 / (J == 0 ? S.Tau1 : S.Tau2));
			double Gain[3] = {0.0, 0.0, 0.0};
			for (int W = 0; W < 3; ++W)
			{
				Gain[W] = T.Decay[J][W] * (1.0 - F);
				T.Decay[J][W] *= F;
			}
			G.Views += Gain[0];
			G.Subs += Gain[1];
			G.Ad += Paying ? Gain[2] / 1000.0 : 0.0;
			if (U3)
			{
				(Cl == static_cast<int>(Class::Short) ? Dx.CutAd : Dx.FoldedAd) += Paying ? Gain[2] / 1000.0 : 0.0;
				Dx.CutViews += Paying && Cl == static_cast<int>(Class::Short) ? Gain[0] : 0.0;
				double* const Into[4] = {&Dx.Likes, &Dx.Dislikes, &Dx.Comments, &Dx.Impr};
				for (int W = 0; W < 4; ++W)
				{
					*Into[W] += Tx.Decay[J][W] * (1.0 - F);
					Tx.Decay[J][W] *= F;
				}
				// (A part that has decayed below a billionth of a view is gone: the line keeps no 1e-37s.)
				for (int W = 0; W < 3; ++W)
				{
					T.Decay[J][W] = std::fabs(T.Decay[J][W]) < 1e-9 ? 0.0 : T.Decay[J][W];
				}
				for (int W = 0; W < 4; ++W)
				{
					Tx.Decay[J][W] = std::fabs(Tx.Decay[J][W]) < 1e-9 ? 0.0 : Sig10(Tx.Decay[J][W]); // (10 digits: vtailx's)
				}
			}
		}
	}
	// The day-bucketed evergreen stubs (Update 3): a year past their upload day, they fold as constants.
	for (size_t I = 0; I < C.X.StubDays.size();)
	{
		const StubDay& S = C.X.StubDays[I];
		double Gain[3] = {0.0, 0.0, 0.0};
		StubDayGain(S, D0, D1, Gain);
		G.Views += Gain[0];
		G.Subs += Gain[1];
		G.Ad += Paying ? Gain[2] / 1000.0 : 0.0;
		Dx.FoldedAd += Paying ? Gain[2] / 1000.0 : 0.0;
		if (DayIndex(D1) > S.Day + 365)
		{
			Tail& T = C.Tails[static_cast<size_t>(Class::Strategy)];
			T.Total[0] += S.E * EvergreenDays;
			T.Total[1] += S.EConv * EvergreenDays;
			T.Total[2] += S.ERpm * EvergreenDays;
			C.X.StubDays.erase(C.X.StubDays.begin() + static_cast<std::ptrdiff_t>(I));
			continue;
		}
		++I;
	}
	// The evergreen stubs; a year old, they fold as a constant.
	for (size_t I = 0; I < C.Stubs.size();)
	{
		const Stub& S = C.Stubs[I];
		const double Gain = StubGain(S, D0, D1);
		G.Views += Gain;
		G.Subs += S.Conv * Gain;
		G.Ad += Paying ? S.Rpm / 1000.0 * Gain : 0.0;
		Dx.FoldedAd += Paying ? S.Rpm / 1000.0 * Gain : 0.0;
		if (D1 >= S.At + EvergreenDays * Day)
		{
			Tail& T = C.Tails[static_cast<size_t>(Class::Strategy)];
			const double W[3] = {1.0, S.Conv, S.Rpm};
			for (int Kx = 0; Kx < 3; ++Kx)
			{
				T.Total[Kx] += S.E * EvergreenDays * W[Kx];
			}
			C.Stubs.erase(C.Stubs.begin() + static_cast<std::ptrdiff_t>(I));
			continue;
		}
		++I;
	}
	const double SubsWere = C.Subs;
	C.Subs += G.Subs;
	// Subscribers follow the player to Kast (§4.7): Kast's offline turnover makes whole follows of it.
	K.FollowFrac += 0.08 * G.Subs + 0.0005 * G.Views;
	Chips DayCents = 0;
	if (Paying)
	{
		DayCents = static_cast<Chips>(std::llround(G.Ad * 100.0 * Sea));
		// The top-earner rate (the user's soft ceiling, C3 fix H1): a Rerun 2.0 channel's pay month pays in full up to T, then
		// closes on T + C; the day's share Kd scales every part of the day alike (the videos' cents, the Cuts feed's) and the
		// rings keep what it held back. At or under T, Kd is exactly 1 and nothing below changes.
		const Chips Earned = DayCents;
		double Kd = 1.0;
		if (U3)
		{
			DayCents = valgo::TopRatePay(valgo::PaidThisMonth(C, DayNo), Earned);
			Kd = DayCents < Earned ? static_cast<double>(DayCents) / std::max(1e-9, G.Ad * 100.0 * Sea) : 1.0;
			Dx.Held = Earned - DayCents;
		}
		for (const std::pair<Video*, double>& E : Earn)
		{
			E.first->X.Earned = Kd == 1.0 ? Quantize(E.first->X.Earned + E.second, 1e4) : Quantize(E.first->X.Earned + E.second * Kd, 1e4); // (to 1/10,000 of a cent)
		}
		C.BalanceCents += DayCents;
		if (U3)
		{
			// The day's split (§4.5.7): the Cuts feed's cents rounded on their own, the watch page the rest (they sum to the day's);
			// the folded long tails' impressions at the kept long videos' ratio that day (a neutral Breakdown's when none earned).
			// (Past T the Cuts feed's part is Kd of its own, and what the rate held back of it is the held cents' Cuts part.)
			Dx.CutCents = std::clamp<Chips>(static_cast<Chips>(std::llround(Kd == 1.0 ? Dx.CutAd * 100.0 * Sea : Dx.CutAd * 100.0 * Sea * Kd)), 0, DayCents);
			Dx.HeldCut = Dx.Held > 0 ? std::clamp<Chips>(static_cast<Chips>(std::llround(Dx.CutAd * 100.0 * Sea)) - Dx.CutCents, 0, Dx.Held) : 0;
			const double ImprPerDollar = Dx.LongAd > 0.0 ? Dx.LongImpr / Dx.LongAd : 1000.0 / (valgo::VaBaseCpm * valgo::VaCreatorShare);
			const double MonPerDollar = Dx.LongAd > 0.0 ? Dx.LongMon / Dx.LongAd : 1000.0 / (valgo::VaBaseCpm * valgo::VaCreatorShare * 1.20);
			Dx.AdImpr = Dx.LongImpr + Dx.FoldedAd * ImprPerDollar;
			Dx.Monetized = Dx.LongMon + Dx.FoldedAd * MonPerDollar;
		}
	}
	if (C.PartnerAt < 0.0)
	{
		RingAdd(C, DayNo, G.Hours);
	}
	for (const double M : {100.0, 1000.0, 10000.0})
	{
		if (SubsWere < M && C.Subs >= M)
		{
			Bell(C, D1, video_copy::BellSubs(static_cast<long long>(M)));
		}
	}
	// The partner gate (§4.6): the trailing 365 days, both bars; a week's review; then the ads count. Rerun 2.0's review also
	// waits for the second rent (update 3 W11: Act I stays quiet).
	if (C.ReviewAt < 0.0 && C.PartnerAt < 0.0)
	{
		const double Hours = RingSum(C, DayNo - 364, DayNo) + C.GrantedHours;
		if (C.Subs >= GateSubs && Hours >= GateHours && (!U3 || L.RentsPaid >= 2))
		{
			C.ReviewAt = D1;
			Mail(L, D1, "rerun:partner", LetterReview, video_copy::ReviewSubject(), video_copy::ReviewBody(C.Subs, Hours));
			Bell(C, D1, video_copy::BellReview());
		}
	}
	else if (C.PartnerAt < 0.0 && D1 >= C.ReviewAt + ReviewDays * Day - 1e-6)
	{
		C.PartnerAt = D1;
		C.HoursByDay.fill(0.0f); // the ring is only for the gate
		Mail(L, D1, "rerun:partner", LetterWelcome, video_copy::WelcomeSubject(), video_copy::WelcomeBody());
		Bell(C, D1, video_copy::BellPartner());
	}
	// Payday: the 21st, keyed by month, once.
	int Y = 0, Mo = 0, Dm = 0;
	world::CivilDate(DayIndex(D1), Y, Mo, Dm);
	if (Dm == PayDay && C.PartnerAt >= 0.0 && C.PartnerAt <= D1)
	{
		char Key[24];
		std::snprintf(Key, sizeof(Key), "pay:%04d-%02d", Y, Mo);
		if (C.Paid.insert(Key).second)
		{
			const std::string Month = video_copy::MonthName(Mo);
			const bool Pays = C.BalanceCents >= PayoutMinCents;
			const Chips Cents = Pays ? C.BalanceCents : 0;
			// Rerun 2.0's statement is itemized (§4.5.7): the balance's days split into the watch page (its RPM and CPM) and the
			// Cuts feed (today's row joins the rings at the midnight, so it's added here); VideoCopy.cpp words it.
			valgo::VaSplit Split;
			const Chips BalanceNow = C.BalanceCents;
			if (U3)
			{
				Split = valgo::SplitCovering(C, std::max<Chips>(0, C.BalanceCents - DayCents));
				if (DayCents > 0 || Dx.Held > 0)
				{
					Split.Cents += DayCents;
					Split.CutCents += Dx.CutCents;
					Split.Views += G.Views;
					Split.CutViews += Dx.CutViews;
					Split.AdImpr += Dx.AdImpr;
					Split.Monetized += Dx.Monetized;
					Split.ToDay = DayNo;
					// (Today's held cents join the rings' only where the rings keep theirs: DayRow's HeldCents, the lead's patch.)
					Split.Held += valgo::VaHasHeld ? Dx.Held : 0;
					Split.HeldCut += valgo::VaHasHeld ? Dx.HeldCut : 0;
				}
			}
			// The top-earner rate's line (C3 fix H1): the month this payout covers passed T (its paid cents, today's included).
			const bool TopRate = U3 && valgo::PaidThisMonth(C, DayNo) + DayCents > valgo::TopRateFrom();
			if (Pays)
			{
				Bankroll += Cents;
				L.Record(D1, video_copy::PayoutLabel(Month), Cents, life::LedgerVideo);
				C.PaidCents += Cents;
				C.BalanceCents = 0;
			}
			// A payout always comes with its statement (the bank line never arrives unexplained); a month that rolls over
			// keeps to the one routine letter a week (§1.7).
			if (Pays || !WroteThisWeek(L, D1))
			{
				// (Itemized when the rings account for the whole balance: a channel that turned Rerun 2.0 on with an Update 2 balance
				// keeps the plain letter until its balance is all Rerun 2.0's, so the parts always add up to what's paid.)
				// (The statement's CPM is the advertisers' price, the top-earner rate's share never in it: StatementCpm.)
				const std::string Body = (U3 && Split.Has && Split.Cents > 0 && Split.Cents == BalanceNow
						? vcontent::StatementU3(Month, Split.WatchCents(), Split.WatchRpm(), valgo::StatementCpm(C, Split, valgo::PayMonthOf(DayNo)), Split.CutCents, Cents,
							  C.BalanceCents, Pays)
						: video_copy::StatementBody(Month, Cents, C.BalanceCents, Pays)) +
					(TopRate ? " " + valgo::TopRateLine() : std::string());
				Mail(L, D1, "rerun:pay", LetterStatement, video_copy::StatementSubject(Month, Pays), Body);
			}
			// Old payout keys go (the day's step never comes round again); the story keys stay.
			std::vector<std::string> PayKeys;
			for (const std::string& P : C.Paid)
			{
				if (P.rfind("pay:", 0) == 0)
				{
					PayKeys.push_back(P);
				}
			}
			for (size_t I = 0; I + KeepPayKeys < PayKeys.size(); ++I)
			{
				C.Paid.erase(PayKeys[I]);
			}
		}
	}
	// Candidates go stale after three weeks.
	C.Open.erase(std::remove_if(C.Open.begin(), C.Open.end(), [D1](const Candidate& X) { return X.Expires <= D1; }), C.Open.end());
	if (U3)
	{
		MidnightU3(C, L, D0, G, Dx, DayCents, TheWorld);
	}
	C.SteppedTo = D1;
	CountUploads(C, D1);
}

/** Step with Keep videos kept whole (Step's 60; the fold test runs the same channel unfolded). */
void StepKeep(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To, size_t Keep, const world::World* W = nullptr)
{
	(void)From;
	const double End = static_cast<double>(DayIndex(To)) * Day;
	if (C.SteppedTo <= 0.0)
	{
		// Untouched: nothing has ever been on the channel, so there's nothing to step.
		C.SteppedTo = End;
		C.HoursDay0 = std::max(0, DayIndex(End) - 365);
		return;
	}
	if (End <= C.SteppedTo)
	{
		return;
	}
	const bool Empty = !C.X.On && C.Videos.empty() && C.Stubs.empty() && C.Open.empty() && C.ReviewAt < 0.0 && C.PartnerAt < 0.0 && C.BalanceCents == 0 &&
		std::all_of(C.Tails.begin(), C.Tails.end(), [](const Tail& T) { return T.Decay[0][0] == 0.0 && T.Decay[1][0] == 0.0 && T.Decay[0][1] == 0.0 && T.Decay[1][1] == 0.0; });
	if (Empty && C.Subs < GateSubs)
	{
		// Nothing to gain, review or pay: the days pass in one go.
		C.SteppedTo = End;
		RingAdd(C, DayIndex(End) - 1, 0.0);
		CountUploads(C, End);
		return;
	}
	for (double D0 = C.SteppedTo; D0 < End - 1e-6; D0 += Day)
	{
		DayStep(C, L, Bankroll, K, D0, Keep, W);
	}
}

// ---------------------------------------------------------------- the save

std::string Esc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
	for (const char Ch : In)
	{
		switch (Ch)
		{
		case '\\': Out += "\\\\"; break;
		case '\t': Out += "\\t"; break;
		case '\n': Out += "\\n"; break;
		case '|': Out += "\\b"; break;
		default: Out.push_back(Ch); break;
		}
	}
	return Out;
}

std::string Unesc(const std::string& In)
{
	std::string Out;
	Out.reserve(In.size());
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 't' ? '\t' : N == 'n' ? '\n' : N == 'b' ? '|' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

/** The shortest text that reads back as exactly X. */
std::string Num(double X)
{
	if (X == 0.0)
	{
		return "0";
	}
	char Buf[40];
	for (int Precision = 15; Precision <= 17; ++Precision)
	{
		std::snprintf(Buf, sizeof(Buf), "%.*g", Precision, X);
		if (std::strtod(Buf, nullptr) == X)
		{
			break;
		}
	}
	return Buf;
}

double ToNum(const std::string& S)
{
	return S.empty() ? 0.0 : std::strtod(S.c_str(), nullptr);
}

long long ToInt(const std::string& S)
{
	return S.empty() ? 0 : std::strtoll(S.c_str(), nullptr, 10);
}

std::string JoinList(const std::vector<std::string>& Items)
{
	std::string Out;
	for (size_t I = 0; I < Items.size(); ++I)
	{
		Out += (I > 0 ? "|" : "") + Esc(Items[I]);
	}
	return Out;
}

std::vector<std::string> SplitList(const std::string& Field)
{
	std::vector<std::string> Out;
	if (Field.empty())
	{
		return Out;
	}
	std::string Cur;
	for (size_t I = 0; I < Field.size(); ++I)
	{
		if (Field[I] == '\\' && I + 1 < Field.size())
		{
			Cur.push_back(Field[I]);
			Cur.push_back(Field[++I]);
		}
		else if (Field[I] == '|')
		{
			Out.push_back(Unesc(Cur));
			Cur.clear();
		}
		else
		{
			Cur.push_back(Field[I]);
		}
	}
	Out.push_back(Unesc(Cur));
	return Out;
}

// ---------------------------------------------------------------- Update 3's fields (update 3 SPEC §7.4; the lead, Phase A)
// Written only for what holds Update 3 state (an Update 3 video's block, an idea candidate, the channel's X), so a save
// nobody touched since Update 2 writes back byte for byte; every saved enum is read clamped to its Count.

/** The 14 small ints of a recipe, in ThumbSpec's order. */
std::array<int16_t*, 14> U3SpecInts(ThumbSpec& S)
{
	return {&S.Arch, &S.Layout, &S.Mirror, &S.Bg, &S.Pal, &S.Subj, &S.Expr, &S.Pose, &S.PropA, &S.PropB, &S.CapStyle, &S.Sticker, &S.Corner, &S.LookRef};
}

/** A caption inside a recipe's field: '\' and ';' escaped, so the field splits on its own ';'. */
std::string U3SemiEsc(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\' || Ch == ';')
		{
			Out.push_back('\\');
		}
		Out.push_back(Ch);
	}
	return Out;
}

/** The compact ints' alphabet: a value from -1 to 60 as one character (62 places; review-core-videoalgo: 61 read the NUL). */
constexpr const char* U3IntChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/**
 * The compositor's facts that repeat the video's own Thumb (VideoContent.h: Caption2's facts after thumb::FactSep; c the cards,
 * o the other face, s the site, as Video::Th keeps them): written "key=" + U3SameAsThumb, read back from the video's Thumb.
 */
constexpr char U3SameAsThumb = '\x1e';
std::string U3FactOfThumb(char Key, const Thumb& Th)
{
	return Key == 'c' ? Th.Cards : Key == 'o' ? Th.Opponent : Key == 's' ? std::to_string(Th.Site) : std::string();
}

/** Caption2 with each repeated fact elided (Pack) or restored (!Pack): the facts' order and every other byte kept. */
std::string U3ThumbFacts(const std::string& Caption2, const Thumb& Th, bool Pack)
{
	std::string Out;
	size_t At = 0;
	const size_t First = Caption2.find(thumb::FactSep);
	if (First == std::string::npos)
	{
		return Caption2;
	}
	Out = Caption2.substr(0, First);
	At = First;
	while (At < Caption2.size())
	{
		const size_t Next = std::min(Caption2.find(thumb::FactSep, At + 1), Caption2.size());
		std::string Fact = Caption2.substr(At + 1, Next - At - 1);
		if (Fact.size() >= 2 && Fact[1] == '=' && (Fact[0] == 'c' || Fact[0] == 'o' || Fact[0] == 's'))
		{
			const std::string Own = U3FactOfThumb(Fact[0], Th);
			if (Pack && Fact.substr(2) == Own && !Own.empty())
			{
				Fact = Fact.substr(0, 2) + U3SameAsThumb;
			}
			else if (!Pack && Fact.size() == 3 && Fact[2] == U3SameAsThumb)
			{
				Fact = Fact.substr(0, 2) + Own;
			}
		}
		Out += thumb::FactSep + Fact;
		At = Next;
	}
	return Out;
}

/**
 * A thumbnail recipe in one field: "<14 ints>;<variant>;<caption>;<caption2>". The ints are "~" and a character each while they
 * fit -1 .. 60 (C3's compaction), else comma-joined (the older form, still read); Caption2's facts that repeat the video's Thumb
 * are elided. A default recipe (a Duel's contender that isn't there) is the empty field.
 */
std::string U3PackSpec(const ThumbSpec& S, const Thumb& Th)
{
	ThumbSpec Copy = S;
	ThumbSpec Blank;
	const std::array<int16_t*, 14> Given = U3SpecInts(Copy);
	const std::array<int16_t*, 14> Unset = U3SpecInts(Blank);
	bool Same = S.Variant == Blank.Variant && S.Caption.empty() && S.Caption2.empty();
	bool Compact = true;
	std::string Ints, Chars = "~";
	for (size_t I = 0; I < Given.size(); ++I)
	{
		const int V = static_cast<int>(*Given[I]);
		Same = Same && *Given[I] == *Unset[I];
		Ints += (I > 0 ? "," : "") + std::to_string(V);
		Compact = Compact && V >= -1 && V <= 60;
		Chars += Compact ? U3IntChars[V + 1] : '0';
	}
	return Same ? std::string() : (Compact ? Chars : Ints) + ";" + std::to_string(S.Variant) + ";" + U3SemiEsc(S.Caption) + ";" + U3SemiEsc(U3ThumbFacts(S.Caption2, Th, true));
}

ThumbSpec U3UnpackSpec(const std::string& Field, const Thumb& Th)
{
	ThumbSpec S;
	if (Field.empty())
	{
		return S;
	}
	std::vector<std::string> Parts(1);
	for (size_t I = 0; I < Field.size(); ++I)
	{
		if (Field[I] == '\\' && I + 1 < Field.size())
		{
			Parts.back().push_back(Field[++I]);
		}
		else if (Field[I] == ';')
		{
			Parts.emplace_back();
		}
		else
		{
			Parts.back().push_back(Field[I]);
		}
	}
	const std::array<int16_t*, 14> Ints = U3SpecInts(S);
	if (!Parts[0].empty() && Parts[0][0] == '~')
	{
		// The compact form: a character a value (U3IntChars' place less one).
		const std::string Alphabet = U3IntChars;
		for (size_t I = 0; I < Ints.size() && I + 1 < Parts[0].size(); ++I)
		{
			const size_t Place = Alphabet.find(Parts[0][I + 1]);
			*Ints[I] = static_cast<int16_t>(Place == std::string::npos ? 0 : static_cast<int>(Place) - 1);
		}
	}
	else
	{
		size_t At = 0;
		for (size_t I = 0; I < Ints.size() && At <= Parts[0].size(); ++I)
		{
			const size_t Comma = std::min(Parts[0].find(',', At), Parts[0].size());
			*Ints[I] = static_cast<int16_t>(ToInt(Parts[0].substr(At, Comma - At)));
			At = Comma + 1;
		}
	}
	S.Variant = Parts.size() > 1 ? static_cast<uint32_t>(ToInt(Parts[1])) : 0u;
	S.Caption = Parts.size() > 2 ? Parts[2] : std::string();
	S.Caption2 = Parts.size() > 3 ? U3ThumbFacts(Parts[3], Th, false) : std::string();
	return S;
}

/** A float field as the shortest text that reads back as exactly it (as a float: at most 9 significant digits). */
std::string U3Flt(float X)
{
	if (X == 0.0f)
	{
		return "0";
	}
	char Buf[32];
	for (int Precision = 6; Precision <= 9; ++Precision)
	{
		std::snprintf(Buf, sizeof(Buf), "%.*g", Precision, static_cast<double>(X));
		if (static_cast<float>(std::strtod(Buf, nullptr)) == X)
		{
			break;
		}
	}
	return Buf;
}

/** An enum saved as an int, read back clamped to its Count. */
template <typename E>
E U3Enum(const std::string& Field)
{
	return static_cast<E>(std::clamp(static_cast<int>(ToInt(Field)), 0, static_cast<int>(E::Count) - 1));
}

/**
 * A video's components in one field (the creator update's compaction of §7.4's vcomp lines, to keep the heavy creator inside
 * 22 KB): "<T>:<P>:<A>:<kind>:<ctrF>" joined by ';', each number the shortest text that reads back as exactly it. A is empty
 * when it is its kind's own (KindA) and the source mix is the kind's (MixFor), so neither is written.
 */
/** A component's start: "d<day>" at a world midnight (every decision's), the minute otherwise (a repackage's, a Live Drop's). */
std::string U3CompTime(double T)
{
	const double Days = T / Day;
	return Days == std::floor(Days) && Days >= 0.0 && Days < 1e7 ? "d" + std::to_string(static_cast<long long>(Days)) : Num(T);
}

double U3ReadCompTime(const std::string& F)
{
	return !F.empty() && F[0] == 'd' ? static_cast<double>(ToInt(F.substr(1))) * Day : ToNum(F);
}

std::string U3PackComps(const Video& V)
{
	// (C3's compaction: the start as a day at a midnight, CtrF empty when 1.)
	std::string Out;
	for (size_t K = 0; K < V.X.Comps.size(); ++K)
	{
		const Comp& Cp = V.X.Comps[K];
		double Own = 0.0;
		const bool Fixed = valgo::KindA(V, Cp.Kind, Own) && Own == Cp.A;
		Out += (K > 0 ? ";" : "") + U3CompTime(Cp.T) + ":" + Num(Cp.P) + ":" + (Fixed ? std::string() : Num(Cp.A)) + ":" + std::to_string(static_cast<int>(Cp.Kind)) + ":" +
			(Cp.CtrF == 1.0f ? std::string() : U3Flt(Cp.CtrF));
	}
	return Out;
}

void U3UnpackComps(const std::string& Field, Video& V)
{
	size_t At = 0;
	while (At < Field.size())
	{
		const size_t End = std::min(Field.find(';', At), Field.size());
		std::vector<std::string> F;
		size_t From = At;
		while (From <= End)
		{
			const size_t Colon = std::min(Field.find(':', From), End);
			F.push_back(Field.substr(From, Colon - From));
			From = Colon + 1;
		}
		if (F.size() >= 5)
		{
			Comp Cp;
			Cp.T = U3ReadCompTime(F[0]);
			Cp.P = ToNum(F[1]);
			Cp.Kind = U3Enum<CompKind>(F[3]);
			double Own = 0.0;
			Cp.A = F[2].empty() && valgo::KindA(V, Cp.Kind, Own) ? Own : ToNum(F[2]);
			Cp.SrcMix = valgo::MixFor(Cp.Kind);
			Cp.CtrF = F[4].empty() ? 1.0f : static_cast<float>(ToNum(F[4]));
			V.X.Comps.push_back(Cp);
		}
		At = End + 1;
	}
}

/** vid's Update 3 block, after the title (P[28]): "3", then the fields in VideoX's saved order (§7.4), LuckLn and the components. */
/** An int field, empty at its default (C3's compaction of vid's block: most videos keep most defaults). */
std::string U3IntOr(long long V, long long Default)
{
	return V == Default ? std::string() : std::to_string(V);
}

long long U3ReadIntOr(const std::string& F, long long Default)
{
	return F.empty() ? Default : ToInt(F);
}

void U3WriteVideoX(const Video& V, std::string& Out)
{
	const VideoX& X = V.X;
	// (s0Base is written empty when it equals s0: every Update 2 choice set's. C3: every field at VideoX's default is empty.)
	auto Flt0 = [](float F) { return F == 0.0f ? std::string() : U3Flt(F); };
	auto Num0 = [](double D) { return D == 0.0 ? std::string() : Num(D); };
	constexpr long long Std = static_cast<long long>(AdLoad::Standard);
	Out += "\t3\t" + std::to_string(X.Fate) + "\t" + Num(X.Ctr) + "\t" + Num(X.CtrNoise) + "\t" + Num(X.Sigma) + "\t" + Num0(X.KastShare) + "\t" + Num(X.S0) + "\t" +
		(X.S0Base == X.S0 ? std::string() : Num(X.S0Base)) + "\t" + Num(X.P0) + "\t" + U3IntOr(X.Idea, -1) + "\t" + U3IntOr(X.Series, -1) + "\t" + Esc(X.TitleTpl) + "\t" +
		U3IntOr(X.Repacks, 0) + "\t" + U3IntOr(X.LastRepackDay, -1) + "\t" + U3IntOr(X.Promise, 0) + "\t" + U3IntOr(X.Payoff, 0) + "\t" + U3IntOr(static_cast<long long>(X.C), 0) +
		"\t" + U3IntOr(static_cast<long long>(X.Ads), Std) + "\t" + U3IntOr(static_cast<long long>(X.AdsAtUpload), Std) + "\t" +
		U3IntOr(static_cast<long long>(X.Len), static_cast<long long>(LengthOpt::Standard)) + "\t" + U3IntOr(X.AdsChangedDay, -1) + "\t" + U3IntOr(X.Launch, 0) + "\t" +
		U3IntOr(X.Duels, 1) + "\t" + Flt0(X.DuelCtr[0]) + "\t" + Flt0(X.DuelCtr[1]) + "\t" + Flt0(X.DuelCtr[2]) + "\t" + U3IntOr(X.DuelWin, -1) + "\t" + U3IntOr(X.Verdict, -1) +
		"\t" + Flt0(X.SettledS) + "\t" + Num0(X.Earned) + "\t" + Esc(X.NextUp);
	for (const ThumbSpec& S : X.Spec)
	{
		Out += "\t" + Esc(U3PackSpec(S, V.Th));
	}
	// The creator update (SPEC_CREATORS §8.1): the upload's own luck, ln(Luck x Viral), and ":1" when its viral roll hit; then
	// the components (U3PackComps; an empty field when there are none).
	Out += "\t" + U3Flt(X.LuckLn) + (X.Viral ? ":1" : "") + "\t" + U3PackComps(V);
}

/** The fields of vid's Update 3 block (P[29] .. P[64]); a block of 63 (before LuckLn) or 64 (before the packed components,
 *  which then come as vcomp lines) still reads. */
constexpr size_t U3VideoFields = 65;

void U3ReadVideoX(const std::vector<std::string>& P, Video& V)
{
	VideoX& X = V.X;
	X = VideoX();
	X.On = true;
	X.Fate = static_cast<uint32_t>(ToInt(P[30]));
	X.Ctr = ToNum(P[31]);
	X.CtrNoise = ToNum(P[32]);
	X.Sigma = ToNum(P[33]);
	X.KastShare = ToNum(P[34]);
	X.S0 = ToNum(P[35]);
	X.S0Base = P[36].empty() ? X.S0 : ToNum(P[36]);
	X.P0 = ToNum(P[37]);
	// (Each field empty at VideoX's default reads as it: C3's compaction.)
	auto EnumOr = [](const std::string& F, auto Default) { return F.empty() ? Default : U3Enum<decltype(Default)>(F); };
	X.Idea = static_cast<int>(U3ReadIntOr(P[38], -1));
	X.Series = static_cast<int>(U3ReadIntOr(P[39], -1));
	X.TitleTpl = Unesc(P[40]);
	X.Repacks = static_cast<int>(U3ReadIntOr(P[41], 0));
	X.LastRepackDay = static_cast<int>(U3ReadIntOr(P[42], -1));
	X.Promise = static_cast<uint8_t>(U3ReadIntOr(P[43], 0));
	X.Payoff = static_cast<uint8_t>(U3ReadIntOr(P[44], 0));
	X.C = EnumOr(P[45], Coin::Full);
	X.Ads = EnumOr(P[46], AdLoad::Standard);
	X.AdsAtUpload = EnumOr(P[47], AdLoad::Standard);
	X.Len = EnumOr(P[48], LengthOpt::Standard);
	X.AdsChangedDay = static_cast<int>(U3ReadIntOr(P[49], -1));
	X.Launch = static_cast<uint8_t>(U3ReadIntOr(P[50], 0));
	X.Duels = static_cast<int8_t>(U3ReadIntOr(P[51], 1));
	for (size_t K = 0; K < 3; ++K)
	{
		X.DuelCtr[K] = static_cast<float>(ToNum(P[52 + K]));
	}
	X.DuelWin = static_cast<int8_t>(U3ReadIntOr(P[55], -1));
	X.Verdict = static_cast<int8_t>(U3ReadIntOr(P[56], -1));
	X.SettledS = static_cast<float>(ToNum(P[57]));
	X.Earned = ToNum(P[58]);
	X.NextUp = Unesc(P[59]);
	for (size_t K = 0; K < 3; ++K)
	{
		X.Spec[K] = U3UnpackSpec(Unesc(P[60 + K]), V.Th);
	}
	if (P.size() >= U3VideoFields - 1)
	{
		const std::string& Luck = P[63];
		const size_t Colon = Luck.find(':');
		X.LuckLn = static_cast<float>(ToNum(Luck.substr(0, Colon)));
		X.Viral = Colon != std::string::npos && Luck.substr(Colon + 1) == "1";
	}
	if (P.size() >= U3VideoFields)
	{
		U3UnpackComps(P[64], V);
	}
}

/** A ring row's numbers (vday, vmonth, vlife: the key is the caller's). */
std::string U3RowText(const DayRow& R)
{
	return "\t" + U3Flt(R.Views) + "\t" + U3Flt(R.WatchH) + "\t" + U3Flt(R.Gained) + "\t" + U3Flt(R.Lost) + "\t" + U3Flt(R.Likes) + "\t" + U3Flt(R.Dislikes) + "\t" +
		U3Flt(R.Impr) + "\t" + U3Flt(R.Comments) + "\t" + std::to_string(R.Cents);
}

/** Reads U3RowText's nine fields from P[At]. */
void U3ReadRow(const std::vector<std::string>& P, size_t At, DayRow& R)
{
	float* const Fields[] = {&R.Views, &R.WatchH, &R.Gained, &R.Lost, &R.Likes, &R.Dislikes, &R.Impr, &R.Comments};
	for (size_t K = 0; K < 8; ++K)
	{
		*Fields[K] = static_cast<float>(ToNum(P[At + K]));
	}
	R.Cents = ToInt(P[At + 8]);
}

/** A row carries the money's split (a day ads paid, once DayRow has the fields: VideoAlgoPriv.h's VaRowSplit). */
bool U3RowSplits(const DayRow& R)
{
	return valgo::VaHasSplit && (valgo::VaRowCut(R) != 0 || valgo::VaRowCutViews(R) != 0.0 || valgo::VaRowImpr(R) != 0.0 || valgo::VaRowMonetized(R) != 0.0);
}

/** The split's four fields (cutCents, cutViews, adImpr, monetized), after a row's nine. */
constexpr size_t U3SplitFields = 4;
std::string U3SplitText(const DayRow& R)
{
	return "\t" + std::to_string(valgo::VaRowCut(R)) + "\t" + U3Flt(static_cast<float>(valgo::VaRowCutViews(R))) + "\t" + U3Flt(static_cast<float>(valgo::VaRowImpr(R))) + "\t" +
		U3Flt(static_cast<float>(valgo::VaRowMonetized(R)));
}

void U3ReadSplit(const std::vector<std::string>& P, size_t At, DayRow& R)
{
	valgo::VaSetRowSplit(R, static_cast<Chips>(ToInt(P[At])), static_cast<float>(ToNum(P[At + 1])), static_cast<float>(ToNum(P[At + 2])), static_cast<float>(ToNum(P[At + 3])));
}

/** A row carries what the top-earner rate held back (a day past T, once DayRow keeps it: c3f-algo_to_lead_1.diff). Such a row
 *  writes its split too: a run of its own keyed "*", fifteen fields a row (vlife: two after its split). Nothing else moves. */
bool U3RowHolds(const DayRow& R)
{
	return valgo::VaHasHeld && (valgo::VaRowHeld(R) != 0 || valgo::VaRowHeldCut(R) != 0);
}

constexpr size_t U3HeldFields = 2;
std::string U3HeldText(const DayRow& R)
{
	return "\t" + std::to_string(valgo::VaRowHeld(R)) + "\t" + std::to_string(valgo::VaRowHeldCut(R));
}

void U3ReadHeld(const std::vector<std::string>& P, size_t At, DayRow& R)
{
	valgo::VaSetRowHeld(R, static_cast<Chips>(ToInt(P[At])), static_cast<Chips>(ToInt(P[At + 1])));
}

void WriteCandidate(const Candidate& C, std::string& Out)
{
	const int Flags = (C.Bubble ? 1 : 0) | (C.Final ? 2 : 0) | (C.Series ? 4 : 0);
	Out += "video\tcand\t" + Esc(C.Id) + "\t" + Num(C.At) + "\t" + std::to_string(C.Source) + "\t" + std::to_string(C.Site) + "\t" + std::to_string(static_cast<int>(C.K)) +
		"\t" + Num(C.Drama) + "\t" + Num(C.Expires) + "\t" + (C.Auto ? "1" : "0") + "\t" + std::to_string(C.Style) + "\t" + (C.Storytime ? "1" : "0") + "\t" +
		Num(C.EquityAtAllIn) + "\t" + Esc(C.Opponent) + "\t" + std::to_string(C.OpponentNpc) + "\t" + (C.Rival ? "1" : "0") + "\t" + Esc(C.Event) + "\t" +
		std::to_string(C.BuyIn) + "\t" + std::to_string(C.Field) + "\t" + std::to_string(C.PlaceSoFar) + "\t" + std::to_string(Flags) + "\t" +
		std::to_string(C.Stream.Viewers) + "\t" + Num(C.Stream.HypeAfter) + "\t" + std::to_string(C.Stream.Face) + "\t" + Num(C.Stream.ClipAt) + "\t" +
		JoinList(C.Stream.Chat) + "\t" + Esc(recall::Write(C.Rec));
	// Update 3 (§7.4): an idea candidate (Source 5) carries its idea and its facts after the recall record.
	if (C.Idea >= 0)
	{
		Out += "\t" + std::to_string(C.Idea) + "\t" + Esc(C.IdeaFacts);
	}
	Out += "\n";
}

bool ReadCandidate(const std::vector<std::string>& P, Candidate& C)
{
	if (P.size() < 27)
	{
		return false;
	}
	C.Id = Unesc(P[2]);
	C.At = ToNum(P[3]);
	C.Source = static_cast<int>(ToInt(P[4]));
	C.Site = static_cast<int>(ToInt(P[5]));
	C.K = static_cast<Kind>(std::clamp(static_cast<int>(ToInt(P[6])), 0, static_cast<int>(Kind::Count) - 1));
	C.Drama = ToNum(P[7]);
	C.Expires = ToNum(P[8]);
	C.Auto = P[9] == "1";
	C.Style = static_cast<int>(ToInt(P[10]));
	C.Storytime = P[11] == "1";
	C.EquityAtAllIn = ToNum(P[12]);
	C.Opponent = Unesc(P[13]);
	C.OpponentNpc = static_cast<int>(ToInt(P[14]));
	C.Rival = P[15] == "1";
	C.Event = Unesc(P[16]);
	C.BuyIn = ToInt(P[17]);
	C.Field = static_cast<int>(ToInt(P[18]));
	C.PlaceSoFar = static_cast<int>(ToInt(P[19]));
	const int Flags = static_cast<int>(ToInt(P[20]));
	C.Bubble = (Flags & 1) != 0;
	C.Final = (Flags & 2) != 0;
	C.Series = (Flags & 4) != 0;
	C.Stream.Viewers = static_cast<int>(ToInt(P[21]));
	C.Stream.HypeAfter = ToNum(P[22]);
	C.Stream.Face = static_cast<int>(ToInt(P[23]));
	C.Stream.ClipAt = ToNum(P[24]);
	C.Stream.Chat = SplitList(P[25]);
	if (P.size() >= 29)
	{
		C.Idea = static_cast<int>(ToInt(P[27]));
		C.IdeaFacts = Unesc(P[28]);
	}
	return recall::Read(Unesc(P[26]), C.Rec);
}

void WriteVideo(const Video& V, std::string& Out)
{
	Out += "video\tvid\t" + Esc(V.Id) + "\t" + Num(V.At) + "\t" + std::to_string(static_cast<int>(V.K)) + "\t" + std::to_string(static_cast<int>(V.F)) + "\t" +
		std::to_string(static_cast<int>(V.T)) + "\t" + std::to_string(static_cast<int>(V.Fr)) + "\t" + std::to_string(static_cast<int>(V.C)) + "\t" + Num(V.P) + "\t" + Num(V.A) +
		"\t" + Num(V.Tau1) + "\t" + Num(V.Tau2) + "\t" + Num(V.E) + "\t" + Num(V.Conv) + "\t" + Num(V.Retention) + "\t" + Num(V.LengthMin) + "\t" + Num(V.Rpm) + "\t" +
		Esc(V.Sponsor) + "\t" + (V.Auto ? "1" : "0") + "\t" + std::to_string(V.Site) + "\t" + std::to_string(static_cast<int>(V.Th.Fr)) + "\t" + std::to_string(V.Th.Mood) +
		"\t" + Esc(V.Th.Cards) + "\t" + Esc(V.Th.BigText) + "\t" + std::to_string(V.Th.Accent) + "\t" + Esc(V.Th.Opponent) + "\t" + std::to_string(V.Th.Site) + "\t" +
		Esc(V.Title);
	// Update 3 (§7.4, S4): an Update 3 video's block after the title (an older build reads the line to the title and drops it).
	if (V.X.On)
	{
		U3WriteVideoX(V, Out);
	}
	Out += "\n";
}

bool ReadVideo(const std::vector<std::string>& P, Video& V)
{
	if (P.size() < 29)
	{
		return false;
	}
	V.Id = Unesc(P[2]);
	V.At = ToNum(P[3]);
	V.K = static_cast<Kind>(std::clamp(static_cast<int>(ToInt(P[4])), 0, static_cast<int>(Kind::Count) - 1));
	// (Update 3, §7.3: each saved enum clamped to its Count, so an appended value reads as itself; every old value as before.)
	V.F = static_cast<Format>(std::clamp(static_cast<int>(ToInt(P[5])), 0, static_cast<int>(Format::Count) - 1));
	V.T = static_cast<TitleStyle>(std::clamp(static_cast<int>(ToInt(P[6])), 0, static_cast<int>(TitleStyle::Count) - 1));
	V.Fr = static_cast<Frame>(std::clamp(static_cast<int>(ToInt(P[7])), 0, static_cast<int>(Frame::Count) - 1));
	V.C = static_cast<Class>(std::clamp(static_cast<int>(ToInt(P[8])), 0, static_cast<int>(Class::Count) - 1));
	V.P = ToNum(P[9]);
	V.A = ToNum(P[10]);
	V.Tau1 = ToNum(P[11]);
	V.Tau2 = ToNum(P[12]);
	V.E = ToNum(P[13]);
	V.Conv = ToNum(P[14]);
	V.Retention = ToNum(P[15]);
	V.LengthMin = ToNum(P[16]);
	V.Rpm = ToNum(P[17]);
	V.Sponsor = Unesc(P[18]);
	V.Auto = P[19] == "1";
	V.Site = static_cast<int>(ToInt(P[20]));
	V.Th.Fr = static_cast<Frame>(std::clamp(static_cast<int>(ToInt(P[21])), 0, static_cast<int>(Frame::Count) - 1));
	V.Th.Mood = static_cast<int>(ToInt(P[22]));
	V.Th.Cards = Unesc(P[23]);
	V.Th.BigText = Unesc(P[24]);
	V.Th.Accent = static_cast<uint32_t>(ToInt(P[25]));
	V.Th.Opponent = Unesc(P[26]);
	V.Th.Site = static_cast<int>(ToInt(P[27]));
	V.Title = Unesc(P[28]);
	if (P.size() >= U3VideoFields - 2 && P[29] == "3")
	{
		U3ReadVideoX(P, V);
	}
	return V.Tau1 > 0.0 && V.Tau2 > 0.0;
}

/** A candidate's id never carries the separators the edit's activity id uses. */
std::string CleanId(std::string Id)
{
	for (char& Ch : Id)
	{
		Ch = Ch == '|' || Ch == '\t' || Ch == '\n' ? '/' : Ch;
	}
	return Id;
}

/** Storytime (§4.4): nobody at Dee's table is named. Names become "Seat 1", "Seat 2" by first appearance. */
void Anonymize(recall::Record& Rec, std::string& Opponent)
{
	std::map<std::string, std::string> As;
	auto Label = [&As](const std::string& Name) {
		if (Name == "You")
		{
			return Name;
		}
		auto It = As.find(Name);
		if (It == As.end())
		{
			It = As.emplace(Name, "Seat " + std::to_string(As.size() + 1)).first;
		}
		return It->second;
	};
	for (recall::Line& L : Rec.Lines)
	{
		L.Who = Label(L.Who);
	}
	for (auto& S : Rec.Shown)
	{
		S.first = Label(S.first);
	}
	Opponent = Opponent.empty() ? std::string() : Label(Opponent);
}
} // namespace video_detail

// ================================================================== Rerun 2.0's shared helpers (VideoAlgoPriv.h)

namespace valgo
{
VaShape ShapeU3(Class C)
{
	const video_detail::Shape S = video_detail::ShapeOf(C);
	return {S.A, S.Tau1, S.Tau2};
}

double CurveFrac(double A, double Tau1, double Tau2, double Days)
{
	return Days > 0.0 ? A * (1.0 - std::exp(-Days / Tau1)) + (1.0 - A) * (1.0 - std::exp(-Days / Tau2)) : 0.0;
}

double ViewsU3(const Video& V, double World)
{
	const double Days = (World - V.At) / VaDay;
	if (Days <= 0.0)
	{
		return 0.0;
	}
	const VaShape S = ShapeU3(V.C);
	double Sum = V.X.P0 * CurveFrac(S.A, S.Tau1, S.Tau2, Days);
	for (const Comp& K : V.X.Comps)
	{
		Sum += K.P * CurveFrac(K.A, S.Tau1, S.Tau2, (World - K.T) / VaDay);
	}
	return Sum + V.E * std::min(Days, video_detail::EvergreenDays);
}

void SpanU3(const Video& V, double T0, double T1, double& Was, double& Now, double& Impr)
{
	// ViewsU3 at both ends and ImprGain over the span, each part's curve read once (the same expressions in the same order, so
	// the numbers are theirs to the bit): DayStep's hot loop.
	const VaShape S = ShapeU3(V.C);
	const double Days0 = (T0 - V.At) / VaDay, Days1 = (T1 - V.At) / VaDay;
	const double F0 = CurveFrac(S.A, S.Tau1, S.Tau2, Days0), F1 = CurveFrac(S.A, S.Tau1, S.Tau2, Days1);
	double Sum0 = V.X.P0 * F0, Sum1 = V.X.P0 * F1;
	double Out = (F1 - F0) * V.X.P0 * ImprPerView(V, 0, 1.0f);
	for (const Comp& K : V.X.Comps)
	{
		const double K0 = CurveFrac(K.A, S.Tau1, S.Tau2, (T0 - K.T) / VaDay), K1 = CurveFrac(K.A, S.Tau1, S.Tau2, (T1 - K.T) / VaDay);
		Sum0 += K.P * K0;
		Sum1 += K.P * K1;
		const double Part = (K1 - K0) * K.P;
		Out += Part * ImprPerView(V, K.SrcMix, K.CtrF);
	}
	Was = Days0 <= 0.0 ? 0.0 : Sum0 + V.E * std::min(Days0, video_detail::EvergreenDays);
	Now = Days1 <= 0.0 ? 0.0 : Sum1 + V.E * std::min(Days1, video_detail::EvergreenDays);
	Impr = std::max(0.0, Out);
}

double RemainingU3(const Video& V, double World)
{
	const VaShape S = ShapeU3(V.C);
	auto Left = [&S, World](double T, double P, double A) {
		const double Age = std::max(0.0, (World - T) / VaDay);
		return P * (A * std::exp(-Age / S.Tau1) + (1.0 - A) * std::exp(-Age / S.Tau2));
	};
	double Sum = Left(V.At, V.X.P0, S.A);
	for (const Comp& K : V.X.Comps)
	{
		Sum += Left(K.T, K.P, K.A);
	}
	return std::max(0.0, Sum);
}

int DayOf(double World)
{
	return video_detail::DayIndex(World);
}

int KindGroup(Kind K)
{
	return video_detail::GroupOf(K);
}

bool Travels(Kind K)
{
	return K == Kind::BadBeat || K == Kind::Win || K == Kind::Bluff;
}

double FormatRetention(Format F)
{
	return video_detail::RowOf(F).Retention;
}

double FormatRpmRow(Format F)
{
	return video_detail::RowOf(F).Rpm;
}

double StyleCtr(TitleStyle T)
{
	return video_detail::TitleOf(T).Ctr;
}

double StyleRet(TitleStyle T)
{
	return video_detail::TitleOf(T).Ret;
}

double StyleAdFit(TitleStyle T)
{
	return video_detail::TitleOf(T).AdFit;
}

double FrameFit(Frame F, int Group)
{
	return video_detail::ThumbFit(F, Group);
}

bool IsGrant(const Video& V)
{
	// (Grant's video is "g<minute>", every upload "v<minute>": the id rules out the title's compare almost always.)
	return !V.Id.empty() && V.Id[0] == 'g' && V.Title == video_copy::GrantTitle();
}

void PushBell(Channel& C, double At, const std::string& Line)
{
	video_detail::Bell(C, At, Line);
}
} // namespace valgo

// ================================================================== candidates

double KindWeight(Kind K)
{
	switch (K)
	{
	case Kind::BadBeat: return 2.0;
	case Kind::Suckout: return 1.4;
	case Kind::Cooler: return 1.8;
	case Kind::Bluff: return 1.9;
	case Kind::BluffCaught: return 1.5;
	case Kind::HeroCall: return 1.8;
	case Kind::HeroFold: return 1.6;
	case Kind::BigPot: return 1.2;
	case Kind::Knockout: return 1.0;
	case Kind::Bubble: return 1.6;
	case Kind::DeepRun: return 1.3;
	case Kind::FinalTable: return 1.7;
	case Kind::Win: return 2.2;
	case Kind::Rival: return 1.8;
	case Kind::Story: return 1.5;
	case Kind::StreamMoment: return 1.3;
	// Update 3's hand kinds (§6.3's Topic column, DESIGN §4.9.3 A15-A24; their Classify rules are core-videocontent's): an idea
	// video's weight comes from its facts (Upload), so Idea reads 1.0 here.
	case Kind::Monster: return 1.7;
	case Kind::SetOverSet: return 1.9;
	case Kind::RunnerRunner: return 1.6;
	case Kind::DoubleKo: return 1.4;
	case Kind::LadderFold: return 1.6;
	case Kind::MinCash: return 1.3;
	case Kind::Envelope: return 1.6;
	case Kind::HeadsUp: return 1.8;
	case Kind::BiggestPot: return 1.6;
	case Kind::LastHand: return 1.0;
	case Kind::Idea:
	case Kind::Count: break;
	}
	return 1.0;
}

Kind Classify(const recall::Record& Rec, double EquityAtAllIn, bool Bubble, bool Showdown, double AvgStackBb, bool& Is)
{
	using namespace video_detail;
	Is = false;
	Kind Best = Kind::BigPot;
	auto Take = [&](Kind K) {
		if (!Is || KindWeight(K) > KindWeight(Best))
		{
			Best = K;
		}
		Is = true;
	};
	const HandRead H = Read(Rec);
	if (!H.HeroIn || Rec.Mine.size() < 2)
	{
		return Best;
	}
	const double PotBb = Rec.Blinds.Bb > 0 ? static_cast<double>(Rec.Pot) / static_cast<double>(Rec.Blinds.Bb) : 0.0;
	const bool Won = Rec.HeroNet > 0;
	const bool Lost = Rec.HeroNet < 0;
	const int Mine = MadeLevel(Rec.Mine, Rec.Board);
	if (EquityAtAllIn >= 0.0 && !H.HeroFolded)
	{
		if (EquityAtAllIn >= 0.6 && Lost)
		{
			Take(Kind::BadBeat);
		}
		if (EquityAtAllIn <= 0.4 && Won)
		{
			Take(Kind::Suckout);
		}
	}
	// A cooler moves the pot one way: a split (net about nothing) is two good hands, not one beaten by another.
	const bool OneWay = std::fabs(static_cast<double>(Rec.HeroNet)) * 4.0 >= static_cast<double>(Rec.Pot);
	if (Showdown && !H.HeroFolded && OneWay && PotBb >= 40.0 && OwnCategory(Rec.Mine, Rec.Board) >= static_cast<int>(Category::TwoPair))
	{
		for (const auto& S : Rec.Shown)
		{
			if (OwnCategory(S.second, Rec.Board) >= static_cast<int>(Category::TwoPair))
			{
				Take(Kind::Cooler);
				break;
			}
		}
	}
	if (!Showdown && Won && PotBb >= 15.0 && H.HeroSawFlop && Rec.Board.size() >= 3 && H.HeroLastAggressor && Mine <= 1)
	{
		Take(Kind::Bluff);
	}
	if (Showdown && Lost && H.HeroLastAggressor && H.HeroAggrOnRiver && Mine <= 1)
	{
		Take(Kind::BluffCaught);
	}
	if (Showdown && Won && H.HeroCalledRiverBet && Mine <= 2)
	{
		Take(Kind::HeroCall);
	}
	if (H.HeroFolded && H.HeroFoldedToBig && HandPercentile(Rec.Mine[0], Rec.Mine[1]) <= 0.10)
	{
		Take(Kind::HeroFold);
	}
	if (Won && PotBb >= 30.0 && (AvgStackBb > 0.0 ? PotBb >= 1.5 * AvgStackBb : PotBb >= 50.0))
	{
		Take(Kind::BigPot);
	}
	if (Bubble && H.HeroAllIn)
	{
		Take(Kind::Bubble);
	}
	return Best;
}

void NoteHand(Channel& C, const Candidate& Cand)
{
	using namespace video_detail;
	if (Cand.Id.empty())
	{
		return;
	}
	Anchor(C, Cand.At);
	C.Open.erase(std::remove_if(C.Open.begin(), C.Open.end(), [&Cand](const Candidate& X) { return X.Expires <= Cand.At; }), C.Open.end());
	for (const Candidate& X : C.Open)
	{
		if (X.Id == Cand.Id)
		{
			return; // noted already (the same hand from the other path)
		}
	}
	Candidate N = Cand;
	N.Id = CleanId(N.Id);
	// A hand turned up at the all-in and again at the showdown is one hand shown.
	std::vector<std::pair<std::string, std::vector<Card>>> Shown;
	for (const auto& S : N.Rec.Shown)
	{
		if (std::find(Shown.begin(), Shown.end(), S) == Shown.end())
		{
			Shown.push_back(S);
		}
	}
	N.Rec.Shown = Shown;
	// Rerun 2.0's hand kinds a record shows by itself (RecordKinds: a monster, set over set, runner-runner) on a channel that
	// runs it: the weightiest kind names the moment (an Update 2 channel's candidates keep Update 2's kinds: dormant until its
	// first Rerun 2.0 upload). The kinds that need the event's context are the Session's and NoteLiveHand's.
	if (C.X.On && (N.Source == 0 || N.Source == 1 || N.Source == 3) && N.Rec.Mine.size() >= 2 && !EventKind(N.K) && N.K != Kind::Idea)
	{
		for (const Kind Nk : RecordKinds(N.Rec, !N.Rec.Shown.empty()))
		{
			N.K = KindWeight(Nk) > KindWeight(N.K) ? Nk : N.K;
		}
	}
	N.At = Quantize(N.At, 100.0);
	N.Expires = N.Expires > N.At ? Quantize(N.Expires, 100.0) : N.At + OpenDays * Day;
	N.Drama = DramaOf(N);
	if (N.Storytime)
	{
		Anonymize(N.Rec, N.Opponent);
		N.OpponentNpc = -1;
		N.Event = video_copy::StorytimeEvent();
	}
	C.Open.push_back(N);
	while (C.Open.size() > KeepOpen)
	{
		// The lowest Drama goes first (the oldest of a tie).
		auto Low = C.Open.begin();
		for (auto It = C.Open.begin(); It != C.Open.end(); ++It)
		{
			if (It->Drama < Low->Drama)
			{
				Low = It;
			}
		}
		C.Open.erase(Low);
	}
}

void NoteLiveHand(life::State& L, const recall::Record& Rec, const room::HandNote* Note, const LiveContext& Ctx)
{
	using namespace video_detail;
	if (Rec.Mine.size() < 2)
	{
		return;
	}
	const bool Showdown = !Rec.Shown.empty() || (Note && Note->Shown.size() >= 2);
	// The all-in's equity: the board as it stood when the chips went in, against the hands turned up.
	double Eq = -1.0;
	bool HeroFolded = false;
	for (const recall::Line& X : Rec.Lines)
	{
		HeroFolded = HeroFolded || (X.Who == "You" && X.What == "folds");
	}
	// Without the room's note (a hand played through live::SkipSlice), the board at the all-in is the board as it stood when
	// the betting closed: the street of the last action, with the player all in and someone's hand turned up.
	bool HeroAllIn = false;
	Street Closed = Street::Preflop;
	for (const recall::Line& X : Rec.Lines)
	{
		HeroAllIn = HeroAllIn || (X.Who == "You" && X.What == "all in");
		Closed = X.On;
	}
	const bool AllInCalled = Note ? (Note->AllIn && Note->Called) : (HeroAllIn && Closed < Street::River);
	if (AllInCalled && !HeroFolded && !Rec.Shown.empty())
	{
		std::vector<std::vector<Card>> Theirs;
		for (const auto& S : Rec.Shown)
		{
			if (std::find(Theirs.begin(), Theirs.end(), S.second) == Theirs.end())
			{
				Theirs.push_back(S.second);
			}
		}
		const size_t Dealt = Closed == Street::Flop ? 3 : Closed == Street::Turn ? 4 : 0;
		const std::vector<Card> Board =
			Note ? Note->BoardAtAllIn : std::vector<Card>(Rec.Board.begin(), Rec.Board.begin() + static_cast<std::ptrdiff_t>(std::min(Dealt, Rec.Board.size())));
		Eq = EquityAt(Rec.Mine, Theirs, Board, Ctx.EntryId + ":" + std::to_string(Rec.Number));
	}
	bool Is = false;
	Kind K = Classify(Rec, Eq, Ctx.Bubble, Showdown, 0.0, Is);
	// The room saw the player's bluff turned over and lose (HandNote::HeroShowedBluff, §4.4).
	if (Note && Note->HeroShowedBluff && Rec.HeroNet < 0 && (!Is || KindWeight(Kind::BluffCaught) > KindWeight(K)))
	{
		K = Kind::BluffCaught;
		Is = true;
	}
	// The player sent someone home (the room's note names the busted).
	if (Note && !Note->Busted.empty() && Rec.HeroNet > 0 && (!Is || KindWeight(Kind::Knockout) > KindWeight(K)))
	{
		K = Kind::Knockout;
		Is = true;
	}
	// Rerun 2.0's hand kinds (§6.3's unlock column; their copy is core-videocontent's), on a channel that runs it: what the record
	// shows by itself (RecordKinds: a monster, set over set, runner-runner) and what the room saw (two sent home at once; the
	// heads-up all-in that decided the title). The weightiest kind wins, as Classify's own do.
	if (L.Video.X.On)
	{
		auto Take = [&K, &Is](Kind Nk) {
			if (!Is || KindWeight(Nk) > KindWeight(K))
			{
				K = Nk;
				Is = true;
			}
		};
		for (const Kind Nk : RecordKinds(Rec, Showdown))
		{
			Take(Nk);
		}
		if (Note && Note->Busted.size() >= 2 && Rec.HeroNet > 0)
		{
			Take(Kind::DoubleKo);
		}
		if (Ctx.Final && Ctx.Remaining == 2 && AllInCalled && Note && !Note->Busted.empty())
		{
			Take(Kind::HeadsUp);
		}
	}
	if (!Is)
	{
		return;
	}
	Candidate C;
	const bool Dees = Ctx.Source == 2;
	C.Id = Dees ? "dee:" + std::to_string(DayIndex(Ctx.World)) + ":" + std::to_string(Rec.Number) : "lv:" + Ctx.EntryId + ":" + std::to_string(Rec.Number);
	C.At = Ctx.World;
	C.Source = Dees ? 2 : 1;
	C.K = K;
	C.Rec = Rec;
	C.EquityAtAllIn = Eq;
	C.Opponent = Read(Rec).Villain;
	C.Event = Ctx.Event;
	C.BuyIn = Ctx.BuyIn;
	C.Field = Ctx.Field;
	C.Bubble = Ctx.Bubble;
	C.Final = Ctx.Final;
	C.Auto = Ctx.Auto;
	C.Style = Ctx.Style;
	C.Storytime = Dees;
	C.Expires = Ctx.World + OpenDays * Day;
	NoteHand(L.Video, C);
	L.Update2LinesRead |= life::U2Video;
}

void NoteLiveRecap(life::State& L, const LiveContext& Ctx, int Place, Chips PrizeCents)
{
	using namespace video_detail;
	if (Place <= 0 || Ctx.Source == 2)
	{
		return; // Dee's is a cash game: nothing to recap
	}
	const bool Won = Place == 1;
	// The Embercrest's events are 6-max: seventh is the last table's bubble, not a seat at it.
	const bool Final = Place <= live::EmbercrestTableSize && Ctx.Field > live::EmbercrestTableSize;
	const bool Deep = PrizeCents >= 3 * std::max<Chips>(1, Ctx.BuyIn) && PrizeCents > 0;
	if (!Won && !Final && !Deep)
	{
		return;
	}
	Candidate C;
	C.Id = "lv:" + Ctx.EntryId + ":recap";
	C.At = Ctx.World;
	C.Source = 1;
	C.K = Won ? Kind::Win : Final ? Kind::FinalTable : Kind::DeepRun;
	C.Rec.Pot = PrizeCents;
	C.Rec.HeroNet = PrizeCents - Ctx.BuyIn;
	C.Event = Ctx.Event;
	C.BuyIn = Ctx.BuyIn;
	C.Field = Ctx.Field;
	C.PlaceSoFar = Place;
	C.Final = Final || Won;
	C.Auto = Ctx.Auto;
	C.Style = Ctx.Style;
	C.Expires = Ctx.World + OpenDays * Day;
	NoteHand(L.Video, C);
	L.Update2LinesRead |= life::U2Video;
}

void NoteStory(Channel& C, const std::string& Key, double World)
{
	using namespace video_detail;
	if (Key.empty() || C.Paid.count("story:" + Key) > 0)
	{
		return;
	}
	C.Paid.insert("story:" + Key);
	Candidate S;
	S.Id = "story:" + Key;
	S.At = World;
	S.Source = 4;
	S.K = Kind::Story;
	S.Event = Key;
	S.Expires = World + OpenDays * Day;
	NoteHand(C, S);
}

// ================================================================== the upload

std::vector<Candidate> Picked(const Choice& Ch, const std::vector<Candidate>& Open)
{
	using namespace video_detail;
	std::vector<Candidate> Out;
	for (const std::string& Id : Ch.Candidates)
	{
		for (const Candidate& C : Open)
		{
			if (C.Id == Id)
			{
				Out.push_back(C);
				break;
			}
		}
	}
	if (Ch.F == Format::Recap)
	{
		std::stable_partition(Out.begin(), Out.end(), [](const Candidate& C) { return EventKind(C.K); });
	}
	return Out;
}

std::string CanUpload(const Choice& Ch, const std::vector<Candidate>& Open)
{
	using namespace video_detail;
	const std::vector<Candidate> P = Picked(Ch, Open);
	if (Ch.Candidates.empty())
	{
		return video_copy::WhyNot(0);
	}
	if (P.size() != Ch.Candidates.size())
	{
		return video_copy::WhyNot(1);
	}
	for (size_t I = 0; I < Ch.Candidates.size(); ++I)
	{
		for (size_t J = I + 1; J < Ch.Candidates.size(); ++J)
		{
			if (Ch.Candidates[I] == Ch.Candidates[J])
			{
				return video_copy::WhyNot(2);
			}
		}
	}
	// An idea is a video on its own (§6.3): alone, it goes up as any format its facts allow ("fmt=", the formats a single idea
	// may go up as: IDEAS sets the format to the card's first), without a night's or an event's hands, and as no other
	// (WhyNot(10); core-videocontent_to_core-videoalgo_2.diff's rule, review-core-videoalgo). An idea without "fmt=" keeps
	// Update 2's rules below.
	if (P.size() == 1 && P.front().K == Kind::Idea)
	{
		const std::vector<std::string> Fmt = valgo::FactValues(P.front().IdeaFacts, "fmt");
		if (!Fmt.empty())
		{
			return Fmt.front().find(static_cast<char>('0' + static_cast<int>(Ch.F))) != std::string::npos ? std::string() : video_copy::WhyNot(10);
		}
	}
	if (Ch.F == Format::Vlog || Ch.F == Format::Recap)
	{
		for (const Candidate& C : P)
		{
			if (C.K == Kind::Story)
			{
				return video_copy::WhyNot(9);
			}
		}
	}
	switch (Ch.F)
	{
	case Format::Short:
	case Format::Breakdown:
		if (P.size() != 1)
		{
			return video_copy::WhyNot(Ch.F == Format::Short ? 3 : 4);
		}
		break;
	case Format::Vlog:
	{
		if (P.size() < 3 || P.size() > 8)
		{
			return video_copy::WhyNot(5);
		}
		for (const Candidate& C : P)
		{
			if (NightKey(C.At) != NightKey(P.front().At))
			{
				return video_copy::WhyNot(6);
			}
		}
		break;
	}
	case Format::Recap:
	{
		int Events = 0;
		for (const Candidate& C : P)
		{
			Events += EventKind(C.K) ? 1 : 0;
		}
		if (Events != 1 || P.size() < 3 || P.size() > 7)
		{
			return video_copy::WhyNot(7);
		}
		for (const Candidate& C : P)
		{
			if (EventKey(C.Id) != EventKey(P.front().Id))
			{
				return video_copy::WhyNot(8);
			}
		}
		break;
	}
	}
	return std::string();
}

double EditMinutes(Format F)
{
	return video_detail::RowOf(F).EditMin;
}

double EditEnergy(Format F)
{
	return video_detail::RowOf(F).Energy;
}

namespace video_detail
{
/**
 * Update 3 (the creator update, SPEC_CREATORS §3.2-§3.3): what Upload's Update 3 path reads of Update 2's computation, and
 * nothing Update 2 itself uses. The upload's own draws (Luck x Viral, saved as VideoX::LuckLn so Momentum reads how the
 * upload really did) and Update 2's terms for the same choices.
 */
struct U2Plan
{
	Candidate Lead;
	double Luck = 1, Viral = 1;
	double CtrU2 = 0, Quality = 1, Face = 1;
	double RetU2 = 0, TitleSubs = 1, Niche = 1, Consistency = 1, RowSubs = 1, Tier = 1;
	double BaseCore = 1, BaseSubs = 0, BaseKast = 0; // Base's constant + Fresh + the subscriber and Kast terms
	bool OnNiche = true; // the lead's kind group is the channel's niche (Niche's group, when it has one): trend tourists' test
};

/**
 * Update 2's upload of a choice set (update 3 §4.4: extracted verbatim from Upload at Phase A): the new video with its id,
 * its four draws, its reach and click-through (P), retention, conversion, class and money, everything but its title and
 * thumbnail. Upload makes the video from it, UploadU2 reads its P; nothing here changes the channel, which the caller has
 * anchored (Anchor) and checked (CanUpload). The creator update: Fresh is Fresh on Rerun's addend to Base's constant (0 for
 * Update 2's choice sets and UploadU2, so the five vectors stay pinned); Out, when given, gets the terms Update 3 reads.
 */
Video UploadPlanU2(const Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F, double Fresh = 0.0, U2Plan* Out = nullptr)
{
	const std::vector<Candidate> P = Picked(Ch, C.Open);
	// The lead: a Recap's event, else the most dramatic moment.
	const Candidate* Lead = &P.front();
	for (const Candidate& X : P)
	{
		if (Ch.F != Format::Recap && (EventKind(X.K) ? 10.0 : 0.0) + X.Drama > (EventKind(Lead->K) ? 10.0 : 0.0) + Lead->Drama)
		{
			Lead = &X;
		}
	}
	Video V;
	V.At = Quantize(World, 100.0);
	V.Id = "v" + std::to_string(static_cast<long long>(std::llround(World)));
	for (int N = 2; std::any_of(C.Videos.begin(), C.Videos.end(), [&V](const Video& X) { return X.Id == V.Id; }); ++N)
	{
		V.Id = "v" + std::to_string(static_cast<long long>(std::llround(World))) + "-" + std::to_string(N);
	}
	Rng R(Seed + ":video:" + V.Id);
	// Reach (§4.6): the channel, the cadence and the week's fatigue.
	const double Subs = SubsAt(C, World);
	const double Sat = Subs / (1.0 + Subs / 20000.0);
	int N14 = 0, N7 = 0;
	for (const Video& X : C.Videos)
	{
		if (X.At > World - 14.0 * Day && X.Title != video_copy::GrantTitle())
		{
			++N14;
			N7 += X.At > World - 7.0 * Day ? 1 : 0;
		}
	}
	// The library's term counts the newest 60 at most (what the channel keeps whole), so folding never changes a roll.
	const double Library =
		std::min(static_cast<double>(KeepVideos), static_cast<double>(std::count_if(C.Videos.begin(), C.Videos.end(), [](const Video& X) { return X.Title != video_copy::GrantTitle(); })));
	const double Cadence = 1.0 + 0.05 * static_cast<double>(std::min(6, N14));
	const double Fatigue = std::sqrt(std::min(1.0, 3.0 / static_cast<double>(N7 + 1)));
	const double Base = (1200.0 + Fresh + 180.0 * std::sqrt(Sat) + 3.5 * Sat + 0.25 * std::min(static_cast<double>(F.KastFollowers), 20000.0)) * (1.0 + 0.1 * std::log2(1.0 + Library)) *
		Cadence * Fatigue;
	// Topic: the kind, the stakes, a series, a name people know, the stream that night.
	bool Series = false, Named = false;
	int Viewers = 0;
	for (const Candidate& X : P)
	{
		Series = Series || X.Series;
		Named = Named || X.Rival;
		Viewers = std::max(Viewers, X.Stream.Viewers);
	}
	const double Stakes = Lead->BuyIn > 0 ? std::clamp(1.0 + 0.12 * std::log2(static_cast<double>(Lead->BuyIn) / 550.0), 0.8, 1.6) : 1.0;
	const double Topic = KindWeight(Lead->K) * Stakes * (Series ? 1.3 : 1.0) * (Named ? 1.25 : 1.0) * (1.0 + std::min(0.5, static_cast<double>(Viewers) / 200.0));
	// Luck: lognormal (Box-Muller on two draws), then the rare video that travels.
	const double U1 = std::max(1e-12, R.Next());
	const double U2 = R.Next();
	const double Gauss = std::sqrt(-2.0 * std::log(U1)) * std::cos(2.0 * 3.14159265358979323846 * U2);
	const double Luck = std::exp(0.9 * Gauss);
	const bool Travels = Lead->K == Kind::BadBeat || Lead->K == Kind::Win || Lead->K == Kind::Bluff;
	const double ViralRoll = R.Next();
	const double ViralSize = R.Range(20.0, 50.0);
	const double Viral = ViralRoll < (Travels ? 0.01 : 0.005) ? ViralSize : 1.0;
	const double Impressions = Base * Topic * Luck * Viral;
	// Click-through: the frame for this kind (a portrait for a live hand), the title, the rig, the haircut.
	const bool Portrait = Lead->Source == 1 || Lead->Source == 2;
	const double Thumb = ThumbFit(Ch.Fr, GroupOf(Lead->K)) * (Ch.Fr == Frame::Reaction && Portrait ? 0.9 : 1.0);
	const TitleRow T = TitleOf(Ch.T);
	const double Quality = 0.7 + 0.5 * std::clamp(F.GearQuality, 0.0, 1.0);
	const double Face = 1.0 + 0.05 * std::clamp(static_cast<double>(F.Sharp) / 100.0, 0.0, 1.0);
	const double Ctr = std::clamp(0.05 * Thumb * T.Ctr * Quality * Face, 0.02, 0.14);
	// Retention, and who subscribes.
	const FormatRow Row = RowOf(Ch.F);
	const double PotBb = PotBbOf(*Lead);
	const bool SmallHand = PotBb < 40.0 && !(Lead->K == Kind::FinalTable || Lead->K == Kind::Win || Lead->Final);
	const double TitleRet = Ch.T == TitleStyle::Hype && SmallHand ? 0.6 : T.Ret;
	const double Retention = Row.Retention * TitleRet * (0.85 + 0.3 * std::clamp(F.GearQuality, 0.0, 1.0));
	int Same = 1, Last = 1, LeadSame = 1;
	{
		std::map<int, int> Groups;
		++Groups[GroupOf(Lead->K)];
		for (auto It = C.Videos.rbegin(); It != C.Videos.rend() && Last < 10; ++It)
		{
			if (It->Title == video_copy::GrantTitle())
			{
				continue;
			}
			++Groups[GroupOf(It->K)];
			++Last;
		}
		for (const auto& Gx : Groups)
		{
			Same = std::max(Same, Gx.second);
		}
		LeadSame = Groups[GroupOf(Lead->K)];
	}
	const double Niche = static_cast<double>(Same) >= 0.6 * static_cast<double>(Last) ? 1.15 : 1.0;
	const double Consistency = N14 >= 2 ? 1.1 : 1.0;
	// FormatSubs (v1.2): Shorts hold people best and convert worst; without it a channel of Shorts outgrows every other plan.
	const double Conv = std::clamp(0.0065 * T.Subs * (Retention / 0.45) * Niche * Consistency * Row.Subs, 0.002, 0.012);
	// The class: a Teaching Breakdown of a decision is evergreen (§4.5).
	const bool Decision = Lead->K == Kind::Bluff || Lead->K == Kind::HeroCall || Lead->K == Kind::HeroFold || Lead->K == Kind::Bubble;
	V.C = Ch.F == Format::Short ? Class::Short : (Ch.F == Format::Breakdown && Ch.T == TitleStyle::Teaching && Decision) ? Class::Strategy : Class::News;
	const Shape S = ShapeOf(V.C);
	V.K = Lead->K;
	V.F = Ch.F;
	V.T = Ch.T;
	V.Fr = Ch.Fr;
	V.P = Quantize(Impressions * Ctr, 100.0);
	V.A = S.A;
	V.Tau1 = S.Tau1;
	V.Tau2 = S.Tau2;
	V.E = V.C == Class::Strategy ? Quantize(V.P * 0.0015, 10000.0) : 0.0;
	V.Conv = Quantize(Conv, 1e7);
	V.Retention = Quantize(Retention, 1e4);
	V.LengthMin = Row.LengthMin;
	const double Tier = 1.0 + 0.15 * std::min(1.0, Subs / 100000.0);
	V.Rpm = Quantize(2.5 * Row.Rpm * T.AdFit * Tier, 1e4);
	V.Auto = std::any_of(P.begin(), P.end(), [](const Candidate& X) { return X.Auto; });
	V.Site = Lead->Site;
	if (Out)
	{
		Out->Lead = *Lead;
		Out->Luck = Luck;
		Out->Viral = Viral;
		Out->CtrU2 = Ctr;
		Out->Quality = Quality;
		Out->Face = Face;
		Out->RetU2 = Retention;
		Out->TitleSubs = T.Subs;
		Out->Niche = Niche;
		Out->OnNiche = Niche > 1.0 && LeadSame == Same;
		Out->Consistency = Consistency;
		Out->RowSubs = Row.Subs;
		Out->Tier = Tier;
		Out->BaseSubs = 180.0 * std::sqrt(Sat) + 3.5 * Sat;
		Out->BaseKast = 0.25 * std::min(static_cast<double>(F.KastFollowers), 20000.0);
		Out->BaseCore = 1200.0 + Fresh + Out->BaseSubs + Out->BaseKast;
	}
	return V;
}
} // namespace video_detail

double UploadU2(const Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F)
{
	// Update 3 (§4.4, V2): P_U2, Update 2's impressions x click-through for these choices, exactly as Upload computes it
	// (a channel never stepped is anchored first, on a copy, as Upload anchors the real one). 0 for a choice Upload refuses.
	using namespace video_detail;
	if (!CanUpload(Ch, C.Open).empty())
	{
		return 0.0;
	}
	if (C.SteppedTo <= 0.0)
	{
		Channel First = C;
		Anchor(First, World);
		return UploadPlanU2(First, Ch, World, Seed, F).P;
	}
	return UploadPlanU2(C, Ch, World, Seed, F).P;
}

namespace video_detail
{
/**
 * Update 3 (§7.5's migration, SPEC_CREATORS §8.3): a channel's first Update 3 upload turns its X on. The old videos keep
 * their numbers (V6); Fresh on Rerun's count starts at the videos kept; the streak counts from the kept uploads' last 8
 * weeks (no new draw); Momentum starts at H 0.
 */
void TurnOnU3(Channel& C, double World)
{
	ChannelX& X = C.X;
	X.On = true;
	const int Today = DayIndex(World);
	X.Since = Today;
	X.UploadsEver = 0;
	X.LastUploadDay = -1;
	std::set<int> Weeks;
	for (const Video& V : C.Videos)
	{
		if (valgo::IsGrant(V))
		{
			continue;
		}
		++X.UploadsEver;
		X.LastUploadDay = std::max(X.LastUploadDay, DayIndex(V.At));
		Weeks.insert(DayIndex(V.At) / 7);
	}
	const int Last = Today / 7 - 1; // the last whole week
	X.StreakW = 0;
	for (int W = Last; W > Last - valgo::VaStreakMax && Weeks.count(W) > 0; --W)
	{
		++X.StreakW;
	}
	X.StreakWeek = Last;
	X.RainChecks = 0;
	X.MomH = 0.0;
	X.Momentum = valgo::MomCOf(0.0);
	X.MomShown = 1.0;
}

/**
 * Payoff (§4.6.5): what the content can back up, 0-3: the drama, +1 for a prize of 20x the buy-in, 2 for a story, and a pot
 * in the top 10% of the player's own pots pays off at 3 (the history the channel keeps: the open moments, at least 10).
 */
uint8_t PayoffOf(const Candidate& Lead, const std::vector<Candidate>& Open)
{
	int P = Lead.Drama < 0.45 ? 0 : Lead.Drama < 0.6 ? 1 : Lead.Drama < 0.75 ? 2 : 3;
	if (EventKind(Lead.K) && Lead.BuyIn > 0 && Lead.Rec.Pot >= 20 * Lead.BuyIn)
	{
		++P;
	}
	const double Mine = PotBbOf(Lead);
	if (Mine > 0.0)
	{
		int Pots = 0, Bigger = 0;
		for (const Candidate& X : Open)
		{
			const double Bb = PotBbOf(X);
			Pots += Bb > 0.0 ? 1 : 0;
			Bigger += Bb > Mine ? 1 : 0;
		}
		P = Pots >= valgo::VaPotHistoryMin && Bigger * 10 < Pots ? 3 : P;
	}
	if (Lead.K == Kind::Story)
	{
		P = 2;
	}
	return static_cast<uint8_t>(std::clamp(P, 0, 3));
}

/** A style's promise without a template (§4.6.5): Plain 0; Story, Question and Teaching 1; Loud 2. */
uint8_t StylePromise(TitleStyle T)
{
	return static_cast<uint8_t>(T == TitleStyle::Plain ? 0 : T == TitleStyle::Hype ? 2 : 1);
}

/** Sameness (§4.6.2): an archetype in 3 of the last 5 uploads x0.90, in 4 or 5 of them x0.82. */
double NoveltyOf(const Channel& C, int Arch)
{
	int Seen = 0, Count = 0;
	for (auto It = C.Videos.rbegin(); It != C.Videos.rend() && Count < 5; ++It)
	{
		if (!It->X.On || It->Fr != Frame::Recipe)
		{
			continue;
		}
		++Count;
		Seen += It->X.Spec[static_cast<size_t>(std::clamp<int>(It->X.DuelWin, 0, 2))].Arch == Arch ? 1 : 0;
	}
	return Seen >= 4 ? 0.82 : Seen >= 3 ? 0.90 : 1.0;
}

/**
 * The trend an upload rides (§4.6.10): the hottest one its lead matches (TrendsFor: a Sunday major, a series week, the
 * trends an idea's facts name), TrendFit = 1 + (strength - 1) x heat, x1.10 for a topic few of the world's creators have
 * covered ("few=1"). Every match counts toward its trend's cooldown (TrendCool, the window's last day kept so a long
 * window's count lasts it out); a trend's third and later matching upload gets no factor. Heat: the trend's heat (0: none).
 */
double TrendFitOf(Channel& C, const Candidate& Lead, double At, double& Heat)
{
	Heat = 0.0;
	double Fit = 1.0;
	const bool Few = !valgo::FactValues(Lead.IdeaFacts, "few").empty() && valgo::FactValues(Lead.IdeaFacts, "few").front() == "1";
	for (const valgo::VaTrend& T : valgo::TrendsFor(Lead, At))
	{
		const int Key = valgo::TrendCoolKey(T.Key);
		auto It = std::find_if(C.X.TrendCool.begin(), C.X.TrendCool.end(), [Key](const Cool& Cl) { return Cl.Key == Key; });
		if (It == C.X.TrendCool.end())
		{
			Cool Cl;
			Cl.Key = Key;
			C.X.TrendCool.push_back(Cl);
			It = C.X.TrendCool.end() - 1;
		}
		It->LastDay = std::max(It->LastDay, DayIndex(T.End - 1.0));
		++It->Count;
		const double H = valgo::TrendHeat(T, At);
		const double F = It->Count >= 3 ? 1.0 : (1.0 + (T.Strength - 1.0) * H) * (Few ? valgo::VaTrendFew : 1.0);
		if (F > Fit)
		{
			Fit = F;
			Heat = H;
		}
		TrimTrendCool(C.X);
	}
	return Fit;
}

/**
 * An Update 3 upload (§4.4 with SPEC_CREATORS §3.2-§3.3):
 *   P  = P_U2(Base + Fresh) x Mc3 x MomC x Streak x CtrNoise,  P0 = P / (1 + kappa(S0))
 * Update 2's computation is UploadPlanU2's, its four draws untouched; Update 3's rolls come from a fork of their own
 * (":u3": Fate, the CTR noise, the Duel's contenders). Any Update 2 choice set has Ctr = Ctr_U2, S0 = S0_base (the timing
 * window aside) and so M3 = 1: its reach moves only by MomC, the streak, Fresh and the noise (C2: nobody gets slower).
 */
Video UploadU3(Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F)
{
	ChannelX& X = C.X;
	if (!X.On)
	{
		TurnOnU3(C, World);
	}
	const int Today = DayIndex(World);
	// Welcome back: the first upload after 21 or more dark days, once.
	X.WelcomeBack = X.LastUploadDay >= 0 && Today - X.LastUploadDay >= valgo::VaDormantDays;
	if (X.WelcomeBack)
	{
		X.MomH = valgo::VaRound(X.MomH + valgo::VaMomWelcome, 1e6);
	}
	X.Momentum = valgo::MomCOf(X.MomH);
	const bool Recipe = !Ch.Thumbs.empty();
	// Update 2's half: a recipe's own Update 2 frame (FrameOf) sets the Update 2 CTR.
	Choice AsU2 = Ch;
	if (Recipe)
	{
		AsU2.Fr = FrameOf(Ch.Thumbs.front().Arch);
	}
	const double Fresh = valgo::VaFresh * std::max(0.0, 1.0 - static_cast<double>(X.UploadsEver) / static_cast<double>(valgo::VaFreshUploads));
	U2Plan Plan;
	Video V = UploadPlanU2(C, AsU2, World, Seed, F, Fresh, &Plan);
	const Candidate& Lead = Plan.Lead;
	const int Group = GroupOf(Lead.K);
	// An idea that is a Strategy lesson ("cls=S", §6.3's S: core-videocontent's facts) is the evergreen class, a year of search
	// tail, in any long format (a Cut stays a Cut).
	if (Lead.K == Kind::Idea && Ch.F != Format::Short)
	{
		const std::vector<std::string> Cls = valgo::FactValues(Lead.IdeaFacts, "cls");
		if (!Cls.empty() && Cls.front() == "S")
		{
			V.C = Class::Strategy;
			const Shape Sh = ShapeOf(V.C);
			V.A = Sh.A;
			V.Tau1 = Sh.Tau1;
			V.Tau2 = Sh.Tau2;
		}
	}
	VideoX& Vx = V.X;
	Vx = VideoX();
	Vx.On = true;
	Rng U(Seed + ":video:" + V.Id + ":u3");
	Vx.Fate = U.NextUint32();
	auto Normal = [&U]() {
		const double U1 = std::max(1e-12, U.Next());
		const double U2v = U.Next();
		return std::sqrt(-2.0 * std::log(U1)) * std::cos(2.0 * 3.14159265358979323846 * U2v);
	};
	Vx.CtrNoise = Quantize(std::exp(0.15 * Normal() - 0.01125), 1e4); // (to 1e-4, as the block keeps it: C3's compaction)
	double DuelNoise[3] = {0.0, 0.0, 0.0};
	for (double& N : DuelNoise)
	{
		N = Normal();
	}
	// The packaging: the title template's promise and traits, the recipes (1-3: a Duel), the levers.
	TitleOption Option;
	bool Matched = false;
	if (!Ch.TitleTpl.empty())
	{
		for (const TitleOption& O : TitleOptions(Ch, video::Picked(Ch, C.Open), C, Ch.Shuffle, 3))
		{
			if (O.Key == Ch.TitleTpl)
			{
				Option = O;
				Matched = true;
			}
		}
	}
	V.Title = Matched ? Option.Text : Title(Ch, C.Open, 0);
	Vx.TitleTpl = Ch.TitleTpl;
	Vx.Promise = Matched ? Option.Promise : StylePromise(Ch.T);
	Vx.Payoff = PayoffOf(Lead, C.Open);
	Vx.Idea = Lead.Idea;
	Vx.Series = Ch.Series;
	Vx.NextUp = Ch.NextUp;
	Vx.Len = Ch.F == Format::Short ? LengthOpt::Standard : Ch.Len;
	Vx.Ads = Ch.F == Format::Short ? AdLoad::Standard : Ch.Ads;
	Vx.AdsAtUpload = Vx.Ads;
	const size_t Contenders = Recipe ? std::min<size_t>(3, Ch.Thumbs.size()) : 0;
	for (size_t K = 0; K < Contenders; ++K)
	{
		Vx.Spec[K] = Ch.Thumbs[K];
	}
	Vx.Duels = static_cast<int8_t>(Contenders >= 2 ? Contenders : 1);
	// The thumbnail's facts (the cards, the opponent, the site: ui-thumbs' compositor reads them from Video::Th for a recipe
	// too, ThumbArt.h's contract), with the frame marked Recipe (the video's ThumbSpec draws it).
	V.Th = ThumbFor(Ch, C.Open);
	if (Recipe)
	{
		V.Fr = Frame::Recipe;
		V.Th.Fr = Frame::Recipe;
	}
	// Suitability (§4.5.4): an idea that promotes a real-money site is Limited and 18+ (core-videocontent marks it in its
	// facts: "ltd=1"); strong language (a tilted face with a Loud title) is Limited unless the edit is clean (+15 minutes).
	const std::vector<std::string> Ltd = valgo::FactValues(Lead.IdeaFacts, "ltd");
	const bool Promo = !Ltd.empty() && Ltd.front() == "1";
	const bool Strong = valgo::StrongLanguage(V);
	Vx.C = Promo || (Strong && !Ch.CleanEdit) ? Coin::Limited : Coin::Full;
	// The idea's advertiser category ("ad="), in place of the title style's fit (§4.5.4); 0: the style's.
	const std::vector<std::string> Ad = valgo::FactValues(Lead.IdeaFacts, "ad");
	const double IdeaFit = Ad.empty() ? 0.0 : valgo::AdCategoryFit(Ad.front());
	// An idea video's Topic (§4.4, §6.3): the idea's own weight ("w=", inside Update 2's 0.9-2.4) in place of the kind's 1.0,
	// x0.5 inside its cooldown ("cool=" days since the same idea's last upload; 7 when unsaid). The upload records the use.
	double IdeaTopic = 1.0;
	if (Lead.K == Kind::Idea && Lead.Idea >= 0)
	{
		const std::vector<std::string> Wf = valgo::FactValues(Lead.IdeaFacts, "w"), Cf = valgo::FactValues(Lead.IdeaFacts, "cool");
		IdeaTopic = Wf.empty() ? 1.0 : std::clamp(std::strtod(Wf.front().c_str(), nullptr), 0.9, 2.4);
		const int CoolDays = Cf.empty() ? valgo::VaIdeaCoolDays : std::max(0, static_cast<int>(std::strtol(Cf.front().c_str(), nullptr, 10)));
		auto It = std::find_if(X.IdeaCool.begin(), X.IdeaCool.end(), [&Lead](const Cool& Cl) { return Cl.Key == Lead.Idea; });
		if (It != X.IdeaCool.end() && It->LastDay >= 0 && Today - It->LastDay < CoolDays)
		{
			IdeaTopic *= 0.5;
		}
		if (It == X.IdeaCool.end())
		{
			Cool Cl;
			Cl.Key = Lead.Idea;
			X.IdeaCool.push_back(Cl);
			It = X.IdeaCool.end() - 1;
		}
		It->LastDay = Today;
		++It->Count;
		if (X.IdeaCool.size() > 40)
		{
			X.IdeaCool.erase(std::min_element(X.IdeaCool.begin(), X.IdeaCool.end(), [](const Cool& A, const Cool& B) { return A.LastDay < B.LastDay; }));
		}
	}
	// The packaging CTR: Update 2's for an Update 2 choice set (exactly); a recipe's own fit otherwise, compressed later.
	const int Over0 = valgo::OverOf(V);
	const int Under0 = valgo::UnderOf(V);
	// (The saved doubles are kept short: the CTRs to 1e-7, the scores to 1e-6, the shares to 1e-4, P0 to the cent. Reach reads
	// the unrounded ratio, so an Update 2 choice set's M3 stays exactly 1.)
	double CtrPack = Plan.CtrU2;
	if (!Recipe)
	{
		Vx.Ctr = Quantize(Plan.CtrU2, 1e7);
	}
	else
	{
		const double TraitCtr = Matched ? valgo::TitleTraitCtr(Option.Traits) : 1.0;
		double Sum = 0.0;
		for (size_t K = 0; K < Contenders; ++K)
		{
			const ThumbSpec& S = Vx.Spec[K];
			const int Promise = std::max(static_cast<int>(Vx.Promise), static_cast<int>(ThumbPromise(S)));
			const int Over = std::max(0, Promise - static_cast<int>(Vx.Payoff)), Under = std::max(0, static_cast<int>(Vx.Payoff) - Promise);
			const double Pack = std::clamp(ArchetypeFit(S.Arch, Group, Lead) * ThumbTraits(S, Lead, C) * valgo::StyleCtr(Ch.T) * TraitCtr * NoveltyOf(C, S.Arch) *
					(1.0 + 0.12 * static_cast<double>(Over)) * (1.0 - 0.04 * static_cast<double>(Under)),
				0.70, 1.45);
			double Ctr = std::clamp(0.05 * Pack * Plan.Quality * Plan.Face, 0.02, 0.14);
			if (Contenders >= 2)
			{
				// (Each contender's CTR to 1e-6, as the block keeps it: the served mean is the saved contenders' own.)
				Ctr = Quantize(Ctr * std::exp(0.12 * DuelNoise[K]), 1e6);
				Vx.DuelCtr[K] = static_cast<float>(Ctr);
			}
			Sum += Ctr;
		}
		CtrPack = Sum / static_cast<double>(Contenders);
		Vx.Ctr = Quantize(CtrPack, 1e7);
	}
	// Retention (§4.4): Update 2's three terms, then length, the ad load, honesty, burnout (Update 3's extra editing only).
	const double RetU2 = Recipe ? valgo::FormatRetention(Ch.F) * valgo::StyleRet(Ch.T) * (0.85 + 0.3 * std::clamp(F.GearQuality, 0.0, 1.0)) : Plan.RetU2;
	const double Extra = (15.0 * static_cast<double>(std::max(0, Vx.Duels - 1)) +
							 (Vx.Len == LengthOpt::Extended ? valgo::EditMinutesFor(Ch.F, LengthOpt::Extended) - valgo::EditMinutesFor(Ch.F, LengthOpt::Standard) : 0.0) +
							 (Strong && Ch.CleanEdit ? valgo::VaCleanEditMinutes : 0.0)) /
		60.0;
	const double Hours7 = algo::ExtraHours7(C, World) + Extra;
	const double Burn = std::clamp(1.0 - valgo::VaBurnSlope * std::max(0.0, Hours7 - valgo::VaBurnFree), valgo::VaBurnFloor, 1.0);
	// Burnout's warning (§4.6.6, §6.6.5): past 4 extra hours this week, at most once a week.
	if (Hours7 > valgo::VaBurnFree)
	{
		const std::string Prefix = valgo::BellBurnout(0.0).substr(0, 11); // "You've put "
		const bool Warned = std::any_of(C.Bell.begin(), C.Bell.end(), [&](const std::pair<double, std::string>& B) { return B.first > V.At - 7.0 * Day && B.second.rfind(Prefix, 0) == 0; });
		if (!Warned)
		{
			valgo::PushBell(C, V.At, valgo::BellBurnout(Hours7));
		}
	}
	const double Ret = RetU2 * valgo::LengthRet(Ch.F, Vx.Len) * valgo::AdRet(Ch.F, Vx.Len, Vx.Ads) * (1.0 - 0.10 * static_cast<double>(Over0)) *
		(1.0 + 0.02 * static_cast<double>(Under0)) * Burn * (Matched ? valgo::TitleTraitRet(Option.Traits) : 1.0);
	V.Retention = Quantize(Ret, 1e4);
	V.LengthMin = valgo::LengthMinFor(Ch.F, Vx.Len);
	// The trend it rides (§4.6.10): bonus-only reach and score; an off-niche trend video's trend viewers subscribe at x0.70
	// (off-niche: its kind group isn't the channel's niche, a reaction on a channel of bad beats included, or there's none).
	double TrendHeat = 0.0;
	const double TrendFit = TrendFitOf(C, Lead, V.At, TrendHeat);
	const double Tourists = TrendFit > 1.0 && !Plan.OnNiche ? 1.0 - (1.0 - valgo::VaTrendTourists) * (TrendFit - 1.0) / TrendFit : 1.0;
	// Conversion: Update 2's formula with the Update 3 retention, then the ad load's term for the views it serves.
	const double Conv = std::clamp(0.0065 * Plan.TitleSubs * (Ret / 0.45) * Plan.Niche * Plan.Consistency * Plan.RowSubs, 0.002, 0.012) * valgo::AdConv(Ch.F, Vx.Len, Vx.Ads) *
		Tourists;
	V.Conv = Quantize(Conv, 1e7);
	// Money (§4.5.2): RPM = CPM x monetized x ads per playback x 55%; an Update 2 choice set keeps Update 2's RPM exactly.
	const double Pays = 1.0 + 0.6 * (Ret / RetU2 - 1.0);
	const bool Plain = Vx.Ads == AdLoad::Standard && Vx.Len == LengthOpt::Standard && Vx.C == Coin::Full && Ret == RetU2 && IdeaFit <= 0.0;
	if (Ch.F != Format::Short && !Plain)
	{
		const double Limited = Vx.C == Coin::Limited ? valgo::VaLimitedCpm * valgo::VaMonetizedLimited / valgo::VaMonetized : 1.0;
		const double Fit = IdeaFit > 0.0 ? IdeaFit : valgo::StyleAdFit(Ch.T);
		V.Rpm = Quantize(2.5 * algo::AdsPerPlayback(Ch.F, Vx.Len, Vx.Ads) / 1.2 * Fit * Plan.Tier * Limited * Pays, 1e4);
	}
	// The score at the upload and the same upload with Update 3's levers neutral (§4.4: S0, S0_base).
	Vx.Sigma = Plan.BaseCore > 0.0 ? Quantize(Plan.BaseSubs / Plan.BaseCore, 1e4) : 0.0;
	Vx.KastShare = Plan.BaseCore > 0.0 ? Quantize(0.93 * Plan.BaseKast / Plan.BaseCore, 1e4) : 0.0;
	const double TimeFit = valgo::TimeFitAt(C, V.At);
	const double Tr = TrendFit > 1.0 ? valgo::VaTrendTr * TrendHeat : 0.0;
	const algo::Score S0 = algo::ScoreAt(V, C, World, 0);
	Vx.S0 = Quantize(std::clamp(S0.S + Tr, -2.0, 2.0), 1e5);
	// (r_U2 read as the save keeps a retention, and Ctr_U2 as the block keeps a CTR, so an Update 2 choice set's score is the
	// same number on both sides.)
	const double RetU2Kept = Quantize(RetU2, 1e4);
	const valgo::Rates Neutral = valgo::RatesOf(V, AdLoad::Standard, RetU2Kept, 0);
	Vx.S0Base = Quantize(std::clamp(0.9 * std::log(Quantize(Plan.CtrU2, 1e7) / 0.0475) + valgo::QOf(V, Neutral, RetU2Kept, 0), -2.0, 2.0), 1e5);
	// Reach: Update 3's per-video levers compressed (LMAX 0.30), then MomC, the streak and the CTR noise.
	const double M3 = CtrPack / Plan.CtrU2 * TimeFit * TrendFit * (1.0 + algo::Kappa(Vx.S0)) / (1.0 + algo::Kappa(Vx.S0Base));
	const double Streak = 1.0 + valgo::VaStreakStep * static_cast<double>(X.StreakW);
	// Site promotion is 18+ (§4.5.4): Home shows it to adults only (x0.6) and the Cuts feed not at all (a real platform's rule,
	// not a packaging lever: outside the compression).
	const double Feed = std::max(0.0, 0.93 - Vx.KastShare) * (1.0 - Vx.Sigma);
	const double Adult = !Promo ? 1.0 : Ch.F == Format::Short ? 1.0 - Feed : 1.0 - (1.0 - valgo::VaAdultHome) * Feed;
	V.P = Quantize(V.P * IdeaTopic * algo::Compress(M3, valgo::VaLmaxVideo) * X.Momentum * Streak * Vx.CtrNoise * Adult, 100.0);
	Vx.P0 = valgo::VaViews(V.P / (1.0 + algo::Kappa(Vx.S0)));
	V.E = V.C == Class::Strategy ? Quantize(V.P * 0.0015, 10000.0) : 0.0;
	// The trend's search viewers (§4.4's Trend component, at most one): from the first midnight after the drop, a decision
	// made at upload from the trend it rides, so nothing later needs to remember the match.
	if (TrendFit > 1.0)
	{
		Comp K;
		K.T = static_cast<double>(Today + 1) * Day;
		K.P = valgo::VaViews(valgo::VaTrendComp * (TrendFit - 1.0) / valgo::VaTrendCompAt * Vx.P0);
		K.A = valgo::VaTrendA;
		K.Kind = CompKind::Trend;
		K.SrcMix = valgo::MixFor(CompKind::Trend);
		K.CtrF = 1.0f;
		Vx.Comps.push_back(K);
	}
	// The upload's own luck (SPEC_CREATORS §3.3): Momentum reads it at the verdict; a reload can't change it. SettledS keeps
	// the MomC this upload got (the launch card's "Riding your breakout: +38% reach", true at upload; VideoAlgoPriv.h).
	Vx.LuckLn = static_cast<float>(Quantize(std::log(Plan.Luck * Plan.Viral), 1e4));
	Vx.Viral = Plan.Viral > 1.0;
	Vx.SettledS = static_cast<float>(Quantize(X.Momentum, 1e4));
	++X.UploadsEver;
	X.LastUploadDay = Today;
	return V;
}
} // namespace video_detail

namespace valgo
{
uint8_t PayoffFor(const Candidate& Lead, const std::vector<Candidate>& Open)
{
	return video_detail::PayoffOf(Lead, Open);
}

double NoveltyFor(const Channel& C, int Arch)
{
	return video_detail::NoveltyOf(C, Arch);
}
} // namespace valgo

std::string Upload(Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F)
{
	using namespace video_detail;
	if (!CanUpload(Ch, C.Open).empty())
	{
		return std::string();
	}
	Anchor(C, World);
	// Update 3: a Rerun 2.0 channel's upload, or any upload with a recipe or a title template, is an Update 3 video.
	if (C.X.On || !Ch.TitleTpl.empty() || !Ch.Thumbs.empty())
	{
		Video V = UploadU3(C, Ch, World, Seed, F);
		C.Open.erase(std::remove_if(C.Open.begin(), C.Open.end(),
						 [&Ch](const Candidate& X) { return std::find(Ch.Candidates.begin(), Ch.Candidates.end(), X.Id) != Ch.Candidates.end(); }),
			C.Open.end());
		C.Videos.push_back(V);
		CountUploads(C, World);
		return V.Id;
	}
	// Update 2's numbers for these choices (UploadU2's), then the title and the thumbnail.
	Video V = UploadPlanU2(C, Ch, World, Seed, F);
	V.Title = Title(Ch, C.Open, 0);
	V.Th = ThumbFor(Ch, C.Open);
	// The moments are used up; the video keeps its own copy of what it needs.
	C.Open.erase(std::remove_if(C.Open.begin(), C.Open.end(),
					 [&Ch](const Candidate& X) { return std::find(Ch.Candidates.begin(), Ch.Candidates.end(), X.Id) != Ch.Candidates.end(); }),
		C.Open.end());
	// (The oldest leaves the 60 at the next day's step: folding goes by the stepped clock.)
	C.Videos.push_back(V);
	CountUploads(C, World);
	return V.Id;
}

double Views(const Video& V, double World)
{
	if (V.X.On)
	{
		return valgo::ViewsU3(V, World); // Update 3: the first wave and its components (V1)
	}
	const double Days = (World - V.At) / video_detail::Day;
	if (Days <= 0.0)
	{
		return 0.0;
	}
	return V.P * (V.A * (1.0 - std::exp(-Days / V.Tau1)) + (1.0 - V.A) * (1.0 - std::exp(-Days / V.Tau2))) + V.E * std::min(Days, video_detail::EvergreenDays);
}

void Step(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To, const world::World* W)
{
	video_detail::StepKeep(C, L, Bankroll, K, From, To, video_detail::KeepVideos, W);
}

// The fold test's seam (video_test declares it): the same step with every video kept whole.
namespace video_test_seam
{
void StepUnfolded(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To)
{
	video_detail::StepKeep(C, L, Bankroll, K, From, To, static_cast<size_t>(1) << 30);
}
} // namespace video_test_seam

// ================================================================== reading the channel

double SubsAt(const Channel& C, double World)
{
	if (World <= C.SteppedTo || C.SteppedTo <= 0.0)
	{
		return C.Subs;
	}
	return C.Subs + video_detail::GainsBetween(C, C.SteppedTo, World, false).Subs;
}

double TotalViews(const Channel& C, double World)
{
	using namespace video_detail;
	double Sum = 0.0;
	for (const Video& V : C.Videos)
	{
		Sum += Views(V, World);
	}
	for (int K = 0; K < 3; ++K)
	{
		const Shape S = ShapeOf(static_cast<Class>(K));
		const Tail& T = C.Tails[static_cast<size_t>(K)];
		double Left = 0.0;
		for (int J = 0; J < 2; ++J)
		{
			Left += T.Decay[J][0] * std::exp(-(World - C.SteppedTo) / Day / (J == 0 ? S.Tau1 : S.Tau2));
		}
		Sum += T.Total[0] - Left;
	}
	for (const Stub& S : C.Stubs)
	{
		Sum += StubGain(S, S.At, World);
	}
	for (const StubDay& S : C.X.StubDays)
	{
		double Gain[3] = {0.0, 0.0, 0.0};
		StubDayGain(S, static_cast<double>(S.Day) * Day, World, Gain);
		Sum += Gain[0];
	}
	return std::max(0.0, Sum);
}

double Buzz(const Channel& C, double World)
{
	return video_detail::GainsBetween(C, World - 2.0 * video_detail::Day, World, false).Views;
}

ChannelView View(const Channel& C, double World)
{
	using namespace video_detail;
	ChannelView V;
	V.Subs = SubsAt(C, World);
	V.Partner = C.PartnerAt >= 0.0 && C.PartnerAt <= World;
	V.Balance = C.BalanceCents;
	if (C.PartnerAt < 0.0)
	{
		const int Today = DayIndex(World);
		V.WatchHours365 = RingSum(C, Today - 364, Today) + C.GrantedHours;
		if (World > C.SteppedTo && C.SteppedTo > 0.0)
		{
			V.WatchHours365 += GainsBetween(C, std::max(C.SteppedTo, static_cast<double>(Today) * Day), World, false).Hours;
		}
	}
	else
	{
		V.WatchHours365 = C.GrantedHours;
	}
	// The 28-day chart: today last, today so far.
	const int Today = DayIndex(World);
	for (int I = 0; I < 28; ++I)
	{
		const double D0 = static_cast<double>(Today - 27 + I) * Day;
		const double D1 = std::min(D0 + Day, World);
		const bool Paying = C.PartnerAt >= 0.0 && D0 >= C.PartnerAt - 1e-6;
		const Gains G = GainsBetween(C, D0, D1, Paying);
		V.ViewsByDay[static_cast<size_t>(I)] = G.Views;
		V.SubsByDay[static_cast<size_t>(I)] = G.Subs;
		V.CentsByDay[static_cast<size_t>(I)] = G.Ad * 100.0 * (C.X.On ? algo::Season(Today - 27 + I) : 1.0);
		V.Views28 += G.Views;
	}
	return V;
}

std::vector<life::Mark> Marks(const Channel& C, double From, double To)
{
	using namespace video_detail;
	std::vector<life::Mark> Out;
	if (C.PartnerAt < 0.0 && C.ReviewAt >= 0.0)
	{
		const double At = C.ReviewAt + ReviewDays * Day;
		if (At >= From && At < To)
		{
			life::Mark M;
			M.At = At;
			M.Kind = MarkReview;
			M.Label = video_copy::MarkReviewLabel();
			M.Where = "Rerun";
			Out.push_back(M);
		}
	}
	// A payday is marked only when it will pay: a Partner (the welcome letter came) with $100 or more already in the balance.
	// The review week and a month that would roll over get no mark.
	if (C.PartnerAt >= 0.0 && C.BalanceCents >= PayoutMinCents)
	{
		for (int D = DayIndex(From); D <= DayIndex(To); ++D)
		{
			int Y = 0, Mo = 0, Dm = 0;
			world::CivilDate(D, Y, Mo, Dm);
			const double At = static_cast<double>(D) * Day;
			if (Dm == PayDay && At >= From && At < To)
			{
				life::Mark M;
				M.At = At;
				M.Kind = MarkPayout;
				M.Label = video_copy::MarkPayoutLabel();
				M.Where = "Rerun";
				Out.push_back(M);
			}
		}
	}
	return Out;
}

void Grant(Channel& C, double Subs, double Hours, double ViewCount, double World)
{
	using namespace video_detail;
	Anchor(C, World);
	C.Subs += std::max(0.0, Subs);
	C.GrantedHours += std::max(0.0, Hours);
	if (ViewCount > 0.0)
	{
		// A synthetic video that gains the views over the next month (News's slow curve, so it folds exactly).
		Video V;
		V.Id = "g" + std::to_string(static_cast<long long>(std::llround(World)));
		V.At = Quantize(World, 100.0);
		V.K = Kind::BigPot;
		V.F = Format::Vlog;
		V.T = TitleStyle::Plain;
		V.C = Class::News;
		V.P = Quantize(ViewCount, 100.0);
		V.A = 0.0;
		V.Tau1 = ShapeOf(Class::News).Tau1;
		V.Tau2 = ShapeOf(Class::News).Tau2;
		V.Rpm = 2.5 * RowOf(Format::Vlog).Rpm;
		V.Title = video_copy::GrantTitle();
		C.Videos.push_back(V);
	}
}

// ================================================================== the save (§6.4)

void WriteLines(const life::State& L, std::string& Out)
{
	using namespace video_detail;
	if ((L.Update2LinesRead & life::U2Video) == 0)
	{
		return; // an untouched old save writes back as it was read
	}
	const Channel& C = L.Video;
	Out += "video\tch\t" + Num(C.Subs) + "\t" + Num(C.PartnerAt) + "\t" + Num(C.ReviewAt) + "\t" + std::to_string(C.BalanceCents) + "\t" + std::to_string(C.PaidCents) + "\t" +
		Num(C.SteppedTo) + "\t" + std::to_string(C.Uploads14) + "\t" + std::to_string(C.Uploads7) + "\n";
	for (const Video& V : C.Videos)
	{
		WriteVideo(V, Out);
	}
	for (int K = 0; K < 3; ++K)
	{
		const Tail& T = C.Tails[static_cast<size_t>(K)];
		bool Any = false;
		for (int W = 0; W < 3; ++W)
		{
			Any = Any || T.Total[W] != 0.0 || T.Decay[0][W] != 0.0 || T.Decay[1][W] != 0.0;
		}
		if (Any)
		{
			Out += "video\ttail\t" + std::to_string(K);
			for (int W = 0; W < 3; ++W)
			{
				Out += "\t" + Num(T.Total[W]);
			}
			for (int J = 0; J < 2; ++J)
			{
				for (int W = 0; W < 3; ++W)
				{
					Out += "\t" + Num(T.Decay[J][W]);
				}
			}
			Out += "\n";
		}
	}
	for (const Stub& S : C.Stubs)
	{
		Out += "video\tstub\t" + Esc(S.Id) + "\t" + Num(S.At) + "\t" + Num(S.E) + "\t" + Num(S.Conv) + "\t" + Num(S.Rpm) + "\n";
	}
	if (C.PartnerAt < 0.0)
	{
		int LastSet = -1;
		for (int I = 0; I < 366; ++I)
		{
			LastSet = C.HoursByDay[static_cast<size_t>(I)] != 0.0f ? I : LastSet;
		}
		if (LastSet >= 0 || C.HoursDay0 != 0)
		{
			Out += "video\thours\t" + std::to_string(C.HoursDay0);
			for (int I = 0; I <= LastSet; ++I)
			{
				Out += "\t" + Num(static_cast<double>(C.HoursByDay[static_cast<size_t>(I)]));
			}
			Out += "\n";
		}
	}
	for (const Candidate& X : C.Open)
	{
		WriteCandidate(X, Out);
	}
	for (const std::string& P : C.Paid)
	{
		Out += "video\tpaid\t" + Esc(P) + "\n";
	}
	for (const auto& B : C.Bell)
	{
		Out += "video\tbell\t" + Num(B.first) + "\t" + Esc(B.second) + "\n";
	}
	if (C.GrantedHours > 0.0)
	{
		Out += "video\tgrant\t" + Num(C.GrantedHours) + "\n";
	}
	WriteLinesU3(L, Out);
}

void WriteLinesU3(const life::State& L, std::string& Out)
{
	// Update 3 (§7.4): Rerun 2.0's lines, after Update 2's, only once U3Video is set and only what holds something: the
	// channel's X when it's on (each Update 3 video's block and its components ride on its vid line).
	using namespace video_detail;
	// (A Rerun 2.0 channel writes its state even before the Session sets the bit: its first Update 3 upload is its own; the
	// reader sets U3Video from these lines, so the next save is the same.)
	if ((L.Update3LinesRead & life::U3Video) == 0 && !L.Video.X.On)
	{
		return;
	}
	const Channel& C = L.Video;
	const ChannelX& X = C.X;
	if (!X.On)
	{
		return;
	}
	// The lifetime row and the recommender's standing always (they say the channel's X is on), the rest when they hold any.
	// (A row's money split, §4.5.7, follows its nine fields once ads have paid: vlife's after since.)
	Out += "video\tvlife" + U3RowText(X.Life) + "\t" + std::to_string(X.Since) + (U3RowSplits(X.Life) || U3RowHolds(X.Life) ? U3SplitText(X.Life) : std::string()) +
		(U3RowHolds(X.Life) ? U3HeldText(X.Life) : std::string()) + "\n";
	// vmom, redefined by the creator update (SPEC_CREATORS §8.1; G3 never shipped): MomH, MomC, MomShown, n, the last six
	// perf values, lastUploadDay, welcomeBack, the streak (weeks, its last counted week, Rain Checks), UploadsEver, TrendWeek.
	Out += "video\tvmom\t" + Num(X.MomH) + "\t" + Num(X.Momentum) + "\t" + Num(X.MomShown) + "\t" + std::to_string(X.SettledN);
	for (const float S : X.Settled)
	{
		Out += "\t" + U3Flt(S);
	}
	Out += "\t" + std::to_string(X.LastUploadDay) + "\t" + (X.WelcomeBack ? "1" : "0") + "\t" + std::to_string(X.StreakW) + "\t" + std::to_string(X.StreakWeek) + "\t" +
		std::to_string(X.RainChecks) + "\t" + std::to_string(X.UploadsEver) + "\t" + std::to_string(X.TrendWeek) + "\n";
	// The rings: a run of consecutive keys on one line (the first key, then nine fields a row), a new line at any gap. Rows that
	// carry the money's split (the days ads paid) run on a line of their own, its key marked "+", thirteen fields a row; rows the
	// top-earner rate held back on (C3 fix H1, once DayRow keeps it) the same with the held cents, keyed "*", fifteen a row.
	auto Rings = [&Out](const char* Tag, const std::vector<DayRow>& Rows) {
		auto KindOf = [](const DayRow& R) { return U3RowHolds(R) ? 2 : U3RowSplits(R) ? 1 : 0; };
		for (size_t I = 0; I < Rows.size(); ++I)
		{
			const int Kind = KindOf(Rows[I]);
			const bool Starts = I == 0 || Rows[I].Key != Rows[I - 1].Key + 1 || Kind != KindOf(Rows[I - 1]);
			Out += Starts ? (I > 0 ? "\n" : "") + std::string("video\t") + Tag + "\t" + std::to_string(Rows[I].Key) + (Kind == 2 ? "*" : Kind == 1 ? "+" : "") : std::string();
			Out += U3RowText(Rows[I]) + (Kind >= 1 ? U3SplitText(Rows[I]) : std::string()) + (Kind == 2 ? U3HeldText(Rows[I]) : std::string());
		}
		Out += Rows.empty() ? "" : "\n";
	};
	Rings("vday", X.Days);
	Rings("vmonth", X.Months);
	for (const TopRow& R : X.Top)
	{
		// (At is empty when the id says it: an upload's id is "v<its minute>", C3's compaction.)
		const bool AtInId = R.Id == "v" + Num(R.At);
		Out += "video\tvtop\t" + Esc(R.Id) + "\t" + Num(R.Views) + "\t" + std::to_string(R.Cents) + "\t" + (AtInId ? std::string() : Num(R.At)) + "\t" + Esc(R.Title) + "\n";
	}
	for (size_t K = 0; K < X.TailsX.size(); ++K)
	{
		const TailX& T = X.TailsX[K];
		bool Any = false;
		std::string TailLine = "video\tvtailx\t" + std::to_string(K);
		for (int W = 0; W < 4; ++W)
		{
			Any = Any || T.Total[W] != 0.0;
			TailLine += "\t" + Num(T.Total[W]);
		}
		for (int J = 0; J < 2; ++J)
		{
			for (int W = 0; W < 4; ++W)
			{
				Any = Any || T.Decay[J][W] != 0.0;
				TailLine += "\t" + Num(T.Decay[J][W]);
			}
		}
		Out += Any ? TailLine + "\n" : std::string();
	}
	for (const FrozenLook& Frozen : X.Looks)
	{
		Out += "video\tvlook\t" + std::to_string(Frozen.Ref);
		for (const int16_t Part : Frozen.Look)
		{
			Out += "\t" + std::to_string(static_cast<int>(Part));
		}
		Out += "\n";
	}
	for (const Playlist& Pl : X.Lists)
	{
		Out += "video\tvlist\t" + Esc(Pl.Id) + "\t" + Esc(Pl.Name) + "\t" + JoinList(Pl.Videos) + "\n";
	}
	for (const Challenge& Ch : X.Challenges)
	{
		Out += "video\tvchal\t" + std::to_string(Ch.Id) + "\t" + std::to_string(Ch.Kind) + "\t" + std::to_string(Ch.Site) + "\t" + std::to_string(Ch.Start) + "\t" +
			std::to_string(Ch.Goal) + "\t" + Num(Ch.StartedAt) + "\t" + std::to_string(Ch.State) + "\t" + std::to_string(Ch.Episodes) + "\n";
	}
	for (const Cool& Cl : X.IdeaCool)
	{
		Out += "video\tvidea\t" + std::to_string(Cl.Key) + "\t" + std::to_string(Cl.LastDay) + "\t" + std::to_string(Cl.Count) + "\n";
	}
	for (const Cool& Cl : X.TrendCool)
	{
		Out += "video\tvtrend\t" + std::to_string(Cl.Key) + "\t" + std::to_string(Cl.Count) + "\t" + std::to_string(Cl.LastDay) + "\n";
	}
	for (const Snap& Sn : X.Snaps)
	{
		Out += "video\tvsnap\t" + std::to_string(Sn.Month) + "\t" + std::to_string(Sn.Events) + "\t" + std::to_string(Sn.Cashes) + "\t" + std::to_string(Sn.Net) + "\t" +
			std::to_string(Sn.Rake) + "\t" + std::to_string(Sn.Abi) + "\t" + std::to_string(Sn.RoiMilli) + "\t" + U3Flt(Sn.Hours) + "\n";
	}
	for (const StubDay& Sd : X.StubDays)
	{
		Out += "video\tvstubd\t" + std::to_string(Sd.Day) + "\t" + Num(Sd.E) + "\t" + Num(Sd.EConv) + "\t" + Num(Sd.ERpm) + "\t" + Num(Sd.EF) + "\t" + Num(Sd.EConvF) + "\t" +
			Num(Sd.ERpmF) + "\n";
	}
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	using namespace video_detail;
	if (P.size() < 2 || P[0] != "video")
	{
		return false;
	}
	Channel& C = L.Video;
	const std::string& Tag = P[1];
	bool Ok = false;
	if (Tag == "ch" && P.size() >= 10)
	{
		C.Subs = ToNum(P[2]);
		C.PartnerAt = ToNum(P[3]);
		C.ReviewAt = ToNum(P[4]);
		C.BalanceCents = ToInt(P[5]);
		C.PaidCents = ToInt(P[6]);
		C.SteppedTo = ToNum(P[7]);
		C.Uploads14 = static_cast<int>(ToInt(P[8]));
		C.Uploads7 = static_cast<int>(ToInt(P[9]));
		Ok = true;
	}
	else if (Tag == "vid")
	{
		Video V;
		if (ReadVideo(P, V))
		{
			// (An Update 3 video's block: the save had a line of Rerun 2.0's family.)
			L.Update3LinesRead |= V.X.On ? static_cast<uint32_t>(life::U3Video) : 0u;
			C.Videos.push_back(V);
			Ok = true;
		}
	}
	else if (Tag == "tail" && P.size() >= 12)
	{
		const int K = static_cast<int>(ToInt(P[2]));
		if (K >= 0 && K < 3)
		{
			Tail& T = C.Tails[static_cast<size_t>(K)];
			for (int W = 0; W < 3; ++W)
			{
				T.Total[W] = ToNum(P[static_cast<size_t>(3 + W)]);
				T.Decay[0][W] = ToNum(P[static_cast<size_t>(6 + W)]);
				T.Decay[1][W] = ToNum(P[static_cast<size_t>(9 + W)]);
			}
			Ok = true;
		}
	}
	else if (Tag == "stub" && P.size() >= 7)
	{
		Stub S;
		S.Id = Unesc(P[2]);
		S.At = ToNum(P[3]);
		S.E = ToNum(P[4]);
		S.Conv = ToNum(P[5]);
		S.Rpm = ToNum(P[6]);
		C.Stubs.push_back(S);
		Ok = true;
	}
	else if (Tag == "hours" && P.size() >= 3)
	{
		C.HoursDay0 = static_cast<int>(ToInt(P[2]));
		C.HoursByDay.fill(0.0f);
		for (size_t I = 3; I < P.size() && I - 3 < 366; ++I)
		{
			C.HoursByDay[I - 3] = static_cast<float>(ToNum(P[I]));
		}
		Ok = true;
	}
	else if (Tag == "cand")
	{
		Candidate X;
		if (ReadCandidate(P, X))
		{
			L.Update3LinesRead |= X.Idea >= 0 ? static_cast<uint32_t>(life::U3Video) : 0u;
			C.Open.push_back(X);
			Ok = true;
		}
	}
	else if (Tag == "paid" && P.size() >= 3)
	{
		C.Paid.insert(Unesc(P[2]));
		Ok = true;
	}
	else if (Tag == "bell" && P.size() >= 4)
	{
		C.Bell.push_back({ToNum(P[2]), Unesc(P[3])});
		Ok = true;
	}
	else if (Tag == "grant" && P.size() >= 3)
	{
		C.GrantedHours = ToNum(P[2]);
		Ok = true;
	}
	else if (Tag.size() > 1 && Tag[0] == 'v')
	{
		// Update 3 (§7.4): Rerun 2.0's sub-tags (WriteLinesU3's). A video's components follow its vid line; the rest are
		// the channel's X, which any of them turns on.
		bool U3 = false;
		ChannelX& X = C.X;
		if (Tag == "vcomp" && P.size() >= 9)
		{
			const std::string Id = Unesc(P[2]);
			for (Video& V : C.Videos)
			{
				if (V.Id == Id && V.X.On)
				{
					Comp Cp;
					Cp.T = ToNum(P[3]);
					Cp.P = ToNum(P[4]);
					Cp.A = ToNum(P[5]);
					Cp.Kind = U3Enum<CompKind>(P[6]);
					Cp.SrcMix = static_cast<uint8_t>(ToInt(P[7]));
					Cp.CtrF = static_cast<float>(ToNum(P[8]));
					V.X.Comps.push_back(Cp);
				}
			}
			L.Update3LinesRead |= life::U3Video;
			Ok = true;
		}
		else if ((Tag == "vday" || Tag == "vmonth") && P.size() >= 12)
		{
			// The first key, then nine fields a row (thirteen with the money's split: the key ends in "+"; fifteen with the top-earner
			// rate's held cents too: "*"), the keys consecutive.
			const int First = static_cast<int>(ToInt(P[2]));
			const bool Held = !P[2].empty() && P[2].back() == '*';
			const bool Wide = Held || (!P[2].empty() && P[2].back() == '+');
			const size_t Width = Held ? 9 + U3SplitFields + U3HeldFields : Wide ? 9 + U3SplitFields : 9;
			for (size_t At = 3; At + Width <= P.size(); At += Width)
			{
				DayRow R;
				R.Key = First + static_cast<int>((At - 3) / Width);
				U3ReadRow(P, At, R);
				if (Wide)
				{
					U3ReadSplit(P, At + 9, R);
				}
				if (Held)
				{
					U3ReadHeld(P, At + 9 + U3SplitFields, R);
				}
				(Tag == "vday" ? X.Days : X.Months).push_back(R);
			}
			U3 = true;
		}
		else if (Tag == "vlife" && P.size() >= 12)
		{
			U3ReadRow(P, 2, X.Life);
			X.Since = static_cast<int>(ToInt(P[11]));
			if (P.size() >= 12 + U3SplitFields)
			{
				U3ReadSplit(P, 12, X.Life);
			}
			if (P.size() >= 12 + U3SplitFields + U3HeldFields)
			{
				U3ReadHeld(P, 12 + U3SplitFields, X.Life);
			}
			U3 = true;
		}
		else if (Tag == "vmom" && P.size() >= 18)
		{
			X.MomH = ToNum(P[2]);
			X.Momentum = ToNum(P[3]);
			X.MomShown = ToNum(P[4]);
			X.SettledN = static_cast<int>(ToInt(P[5]));
			for (size_t K = 0; K < X.Settled.size(); ++K)
			{
				X.Settled[K] = static_cast<float>(ToNum(P[6 + K]));
			}
			X.LastUploadDay = static_cast<int>(ToInt(P[12]));
			X.WelcomeBack = P[13] == "1";
			X.StreakW = static_cast<int>(ToInt(P[14]));
			X.StreakWeek = static_cast<int>(ToInt(P[15]));
			X.RainChecks = static_cast<int>(ToInt(P[16]));
			X.UploadsEver = static_cast<int>(ToInt(P[17]));
			X.TrendWeek = P.size() >= 19 ? static_cast<int>(ToInt(P[18])) : -1;
			U3 = true;
		}
		else if (Tag == "vtop" && P.size() >= 7)
		{
			TopRow R;
			R.Id = Unesc(P[2]);
			R.Views = ToNum(P[3]);
			R.Cents = ToInt(P[4]);
			R.At = P[5].empty() && R.Id.size() > 1 && R.Id[0] == 'v' ? ToNum(R.Id.substr(1)) : ToNum(P[5]);
			R.Title = Unesc(P[6]);
			X.Top.push_back(R);
			U3 = true;
		}
		else if (Tag == "vtailx" && P.size() >= 15)
		{
			const int K = static_cast<int>(ToInt(P[2]));
			if (K >= 0 && K < static_cast<int>(X.TailsX.size()))
			{
				TailX& T = X.TailsX[static_cast<size_t>(K)];
				for (int W = 0; W < 4; ++W)
				{
					T.Total[W] = ToNum(P[static_cast<size_t>(3 + W)]);
					T.Decay[0][W] = ToNum(P[static_cast<size_t>(7 + W)]);
					T.Decay[1][W] = ToNum(P[static_cast<size_t>(11 + W)]);
				}
			}
			U3 = true;
		}
		else if (Tag == "vlook" && P.size() >= 17)
		{
			FrozenLook Frozen;
			Frozen.Ref = static_cast<int>(ToInt(P[2]));
			for (size_t K = 0; K < Frozen.Look.size(); ++K)
			{
				Frozen.Look[K] = static_cast<int16_t>(ToInt(P[3 + K]));
			}
			X.Looks.push_back(Frozen);
			U3 = true;
		}
		else if (Tag == "vlist" && P.size() >= 5)
		{
			Playlist Pl;
			Pl.Id = Unesc(P[2]);
			Pl.Name = Unesc(P[3]);
			Pl.Videos = SplitList(P[4]);
			X.Lists.push_back(Pl);
			U3 = true;
		}
		else if (Tag == "vchal" && P.size() >= 10)
		{
			Challenge Ch;
			Ch.Id = static_cast<int>(ToInt(P[2]));
			Ch.Kind = static_cast<int>(ToInt(P[3]));
			Ch.Site = static_cast<int>(ToInt(P[4]));
			Ch.Start = ToInt(P[5]);
			Ch.Goal = ToInt(P[6]);
			Ch.StartedAt = ToNum(P[7]);
			Ch.State = static_cast<int>(ToInt(P[8]));
			Ch.Episodes = static_cast<int>(ToInt(P[9]));
			X.Challenges.push_back(Ch);
			U3 = true;
		}
		else if ((Tag == "videa" || Tag == "vtrend") && P.size() >= 5)
		{
			// videa: idea, last day, count; vtrend: key, count, week.
			const bool IsIdea = Tag == "videa";
			Cool Cl;
			Cl.Key = static_cast<int>(ToInt(P[2]));
			Cl.LastDay = static_cast<int>(ToInt(P[IsIdea ? 3 : 4]));
			Cl.Count = static_cast<int>(ToInt(P[IsIdea ? 4 : 3]));
			(IsIdea ? X.IdeaCool : X.TrendCool).push_back(Cl);
			U3 = true;
		}
		else if (Tag == "vsnap" && P.size() >= 10)
		{
			Snap Sn;
			Sn.Month = static_cast<int>(ToInt(P[2]));
			Sn.Events = static_cast<int>(ToInt(P[3]));
			Sn.Cashes = static_cast<int>(ToInt(P[4]));
			Sn.Net = ToInt(P[5]);
			Sn.Rake = ToInt(P[6]);
			Sn.Abi = ToInt(P[7]);
			Sn.RoiMilli = static_cast<int>(ToInt(P[8]));
			Sn.Hours = static_cast<float>(ToNum(P[9]));
			X.Snaps.push_back(Sn);
			U3 = true;
		}
		else if (Tag == "vstubd" && P.size() >= 9)
		{
			StubDay Sd;
			Sd.Day = static_cast<int>(ToInt(P[2]));
			Sd.E = ToNum(P[3]);
			Sd.EConv = ToNum(P[4]);
			Sd.ERpm = ToNum(P[5]);
			Sd.EF = ToNum(P[6]);
			Sd.EConvF = ToNum(P[7]);
			Sd.ERpmF = ToNum(P[8]);
			X.StubDays.push_back(Sd);
			U3 = true;
		}
		if (U3)
		{
			X.On = true;
			L.Update3LinesRead |= life::U3Video;
			Ok = true;
		}
	}
	if (Ok)
	{
		L.Update2LinesRead |= life::U2Video;
	}
	// A video line this build can't read is still the channel's: claimed, so it isn't read as something else.
	return true;
}

void AfterParse(SaveData& D)
{
	// A save from before Rerun (no video line): an empty channel, which is what the state already holds. No candidate is
	// invented for hands before the update (none were kept), and Kast's clips stay Kast's.
	(void)D;
}
} // namespace video
} // namespace ss
