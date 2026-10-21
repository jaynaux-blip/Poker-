#include "ShortStack/Game/Video.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
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
	case Kind::Cooler: return 0;
	case Kind::Bluff:
	case Kind::BluffCaught:
	case Kind::HeroCall:
	case Kind::HeroFold: return 1;
	case Kind::BigPot:
	case Kind::Knockout:
	case Kind::Bubble: return 2;
	case Kind::DeepRun:
	case Kind::FinalTable:
	case Kind::Win: return 3;
	case Kind::Rival: return 4;
	case Kind::Story:
	case Kind::StreamMoment:
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

/** Views an evergreen stub gains between T0 and T1. */
double StubGain(const Stub& S, double T0, double T1)
{
	const double From = std::max(T0, S.At);
	const double To = std::min(T1, S.At + EvergreenDays * Day);
	return To > From ? S.E * (To - From) / Day : 0.0;
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
	for (const Video& V : C.Videos)
	{
		const double Gain = std::max(0.0, Views(V, T1) - Views(V, T0));
		G.Views += Gain;
		G.Subs += V.Conv * Gain;
		G.Ad += Paying ? V.Rpm / 1000.0 * Gain : 0.0;
		G.Hours += RowOf(V.F).Hours ? Gain * V.LengthMin / 60.0 * V.Retention : 0.0;
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

/** A video leaves the newest Keep: its curve folds into its class's tail as of C.SteppedTo; an evergreen video keeps a stub. */
void Fold(Channel& C, size_t Keep)
{
	while (C.Videos.size() > Keep && C.Videos.front().At <= C.SteppedTo)
	{
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

/** One world day, D0 (a midnight) to D0 + a day. */
void DayStep(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double D0, size_t Keep)
{
	const double D1 = D0 + Day;
	Fold(C, Keep);
	const bool Paying = C.PartnerAt >= 0.0 && D0 >= C.PartnerAt - 1e-6;
	// The videos kept whole: their milestones too.
	Gains G;
	for (const Video& V : C.Videos)
	{
		const double Was = Views(V, D0);
		const double Now = Views(V, D1);
		const double Gain = std::max(0.0, Now - Was);
		G.Views += Gain;
		G.Subs += V.Conv * Gain;
		G.Ad += Paying ? V.Rpm / 1000.0 * Gain : 0.0;
		G.Hours += RowOf(V.F).Hours ? Gain * V.LengthMin / 60.0 * V.Retention : 0.0;
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
		}
	}
	// The evergreen stubs; a year old, they fold as a constant.
	for (size_t I = 0; I < C.Stubs.size();)
	{
		const Stub& S = C.Stubs[I];
		const double Gain = StubGain(S, D0, D1);
		G.Views += Gain;
		G.Subs += S.Conv * Gain;
		G.Ad += Paying ? S.Rpm / 1000.0 * Gain : 0.0;
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
	if (Paying)
	{
		C.BalanceCents += static_cast<Chips>(std::llround(G.Ad * 100.0));
	}
	const int DayNo = DayIndex(D0);
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
	// The partner gate (§4.6): the trailing 365 days, both bars; a week's review; then the ads count.
	if (C.ReviewAt < 0.0 && C.PartnerAt < 0.0)
	{
		const double Hours = RingSum(C, DayNo - 364, DayNo) + C.GrantedHours;
		if (C.Subs >= GateSubs && Hours >= GateHours)
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
				Mail(L, D1, "rerun:pay", LetterStatement, video_copy::StatementSubject(Month, Pays), video_copy::StatementBody(Month, Cents, C.BalanceCents, Pays));
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
	C.SteppedTo = D1;
	CountUploads(C, D1);
}

/** Step with Keep videos kept whole (Step's 60; the fold test runs the same channel unfolded). */
void StepKeep(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To, size_t Keep)
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
	const bool Empty = C.Videos.empty() && C.Stubs.empty() && C.Open.empty() && C.ReviewAt < 0.0 && C.PartnerAt < 0.0 && C.BalanceCents == 0 &&
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
		DayStep(C, L, Bankroll, K, D0, Keep);
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

/**
 * A thumbnail recipe in one field: "<14 ints, comma-joined>;<variant>;<caption>;<caption2>". A default recipe (a Duel's
 * contender that isn't there) is the empty field.
 */
std::string U3PackSpec(const ThumbSpec& S)
{
	ThumbSpec Copy = S;
	ThumbSpec Blank;
	const std::array<int16_t*, 14> Given = U3SpecInts(Copy);
	const std::array<int16_t*, 14> Unset = U3SpecInts(Blank);
	bool Same = S.Variant == Blank.Variant && S.Caption.empty() && S.Caption2.empty();
	std::string Ints;
	for (size_t I = 0; I < Given.size(); ++I)
	{
		Same = Same && *Given[I] == *Unset[I];
		Ints += (I > 0 ? "," : "") + std::to_string(static_cast<int>(*Given[I]));
	}
	return Same ? std::string() : Ints + ";" + std::to_string(S.Variant) + ";" + U3SemiEsc(S.Caption) + ";" + U3SemiEsc(S.Caption2);
}

ThumbSpec U3UnpackSpec(const std::string& Field)
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
	size_t At = 0;
	for (size_t I = 0; I < Ints.size() && At <= Parts[0].size(); ++I)
	{
		const size_t Comma = std::min(Parts[0].find(',', At), Parts[0].size());
		*Ints[I] = static_cast<int16_t>(ToInt(Parts[0].substr(At, Comma - At)));
		At = Comma + 1;
	}
	S.Variant = Parts.size() > 1 ? static_cast<uint32_t>(ToInt(Parts[1])) : 0u;
	S.Caption = Parts.size() > 2 ? Parts[2] : std::string();
	S.Caption2 = Parts.size() > 3 ? Parts[3] : std::string();
	return S;
}

/** A float field as the shortest text that reads back as exactly it. */
std::string U3Flt(float X)
{
	return Num(static_cast<double>(X));
}

/** An enum saved as an int, read back clamped to its Count. */
template <typename E>
E U3Enum(const std::string& Field)
{
	return static_cast<E>(std::clamp(static_cast<int>(ToInt(Field)), 0, static_cast<int>(E::Count) - 1));
}

/** vid's Update 3 block, after the title (P[28]): "3", then the fields in VideoX's saved order (§7.4); the Comps are vcomp lines. */
void U3WriteVideoX(const VideoX& X, std::string& Out)
{
	Out += "\t3\t" + std::to_string(X.Fate) + "\t" + Num(X.Ctr) + "\t" + Num(X.CtrNoise) + "\t" + Num(X.Sigma) + "\t" + Num(X.KastShare) + "\t" + Num(X.S0) + "\t" + Num(X.S0Base) +
		"\t" + Num(X.P0) + "\t" + std::to_string(X.Idea) + "\t" + std::to_string(X.Series) + "\t" + Esc(X.TitleTpl) + "\t" + std::to_string(X.Repacks) + "\t" +
		std::to_string(X.LastRepackDay) + "\t" + std::to_string(static_cast<int>(X.Promise)) + "\t" + std::to_string(static_cast<int>(X.Payoff)) + "\t" +
		std::to_string(static_cast<int>(X.C)) + "\t" + std::to_string(static_cast<int>(X.Ads)) + "\t" + std::to_string(static_cast<int>(X.AdsAtUpload)) + "\t" +
		std::to_string(static_cast<int>(X.Len)) + "\t" + std::to_string(X.AdsChangedDay) + "\t" + std::to_string(static_cast<int>(X.Launch)) + "\t" +
		std::to_string(static_cast<int>(X.Duels)) + "\t" + U3Flt(X.DuelCtr[0]) + "\t" + U3Flt(X.DuelCtr[1]) + "\t" + U3Flt(X.DuelCtr[2]) + "\t" +
		std::to_string(static_cast<int>(X.DuelWin)) + "\t" + std::to_string(static_cast<int>(X.Verdict)) + "\t" + U3Flt(X.SettledS) + "\t" + Num(X.Earned) + "\t" +
		Esc(X.NextUp);
	for (const ThumbSpec& S : X.Spec)
	{
		Out += "\t" + Esc(U3PackSpec(S));
	}
}

/** The fields of vid's Update 3 block (P[29] .. P[62]). */
constexpr size_t U3VideoFields = 63;

void U3ReadVideoX(const std::vector<std::string>& P, VideoX& X)
{
	X = VideoX();
	X.On = true;
	X.Fate = static_cast<uint32_t>(ToInt(P[30]));
	X.Ctr = ToNum(P[31]);
	X.CtrNoise = ToNum(P[32]);
	X.Sigma = ToNum(P[33]);
	X.KastShare = ToNum(P[34]);
	X.S0 = ToNum(P[35]);
	X.S0Base = ToNum(P[36]);
	X.P0 = ToNum(P[37]);
	X.Idea = static_cast<int>(ToInt(P[38]));
	X.Series = static_cast<int>(ToInt(P[39]));
	X.TitleTpl = Unesc(P[40]);
	X.Repacks = static_cast<int>(ToInt(P[41]));
	X.LastRepackDay = static_cast<int>(ToInt(P[42]));
	X.Promise = static_cast<uint8_t>(ToInt(P[43]));
	X.Payoff = static_cast<uint8_t>(ToInt(P[44]));
	X.C = U3Enum<Coin>(P[45]);
	X.Ads = U3Enum<AdLoad>(P[46]);
	X.AdsAtUpload = U3Enum<AdLoad>(P[47]);
	X.Len = U3Enum<LengthOpt>(P[48]);
	X.AdsChangedDay = static_cast<int>(ToInt(P[49]));
	X.Launch = static_cast<uint8_t>(ToInt(P[50]));
	X.Duels = static_cast<int8_t>(ToInt(P[51]));
	for (size_t K = 0; K < 3; ++K)
	{
		X.DuelCtr[K] = static_cast<float>(ToNum(P[52 + K]));
	}
	X.DuelWin = static_cast<int8_t>(ToInt(P[55]));
	X.Verdict = static_cast<int8_t>(ToInt(P[56]));
	X.SettledS = static_cast<float>(ToNum(P[57]));
	X.Earned = ToNum(P[58]);
	X.NextUp = Unesc(P[59]);
	for (size_t K = 0; K < 3; ++K)
	{
		X.Spec[K] = U3UnpackSpec(Unesc(P[60 + K]));
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
		U3WriteVideoX(V.X, Out);
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
	if (P.size() >= U3VideoFields && P[29] == "3")
	{
		U3ReadVideoX(P, V.X);
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
 * Update 2's upload of a choice set (update 3 §4.4: extracted verbatim from Upload at Phase A): the new video with its id,
 * its four draws, its reach and click-through (P), retention, conversion, class and money, everything but its title and
 * thumbnail. Upload makes the video from it, UploadU2 reads its P; nothing here changes the channel, which the caller has
 * anchored (Anchor) and checked (CanUpload).
 */
Video UploadPlanU2(const Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F)
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
	const double Base = (1200.0 + 180.0 * std::sqrt(Sat) + 3.5 * Sat + 0.25 * std::min(static_cast<double>(F.KastFollowers), 20000.0)) * (1.0 + 0.1 * std::log2(1.0 + Library)) *
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
	int Same = 1, Last = 1;
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

std::string Upload(Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F)
{
	using namespace video_detail;
	if (!CanUpload(Ch, C.Open).empty())
	{
		return std::string();
	}
	Anchor(C, World);
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
	const double Days = (World - V.At) / video_detail::Day;
	if (Days <= 0.0)
	{
		return 0.0;
	}
	return V.P * (V.A * (1.0 - std::exp(-Days / V.Tau1)) + (1.0 - V.A) * (1.0 - std::exp(-Days / V.Tau2))) + V.E * std::min(Days, video_detail::EvergreenDays);
}

void Step(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To, const world::World* /*W*/)
{
	video_detail::StepKeep(C, L, Bankroll, K, From, To, video_detail::KeepVideos);
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
		V.CentsByDay[static_cast<size_t>(I)] = G.Ad * 100.0;
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
	// components of each Update 3 video (its block rides on its vid line), then the channel's X when it's on.
	using namespace video_detail;
	if ((L.Update3LinesRead & life::U3Video) == 0)
	{
		return;
	}
	const Channel& C = L.Video;
	for (const Video& V : C.Videos)
	{
		for (size_t K = 0; V.X.On && K < V.X.Comps.size(); ++K)
		{
			const Comp& Cp = V.X.Comps[K];
			Out += "video\tvcomp\t" + Esc(V.Id) + "\t" + Num(Cp.T) + "\t" + Num(Cp.P) + "\t" + Num(Cp.A) + "\t" + std::to_string(static_cast<int>(Cp.Kind)) + "\t" +
				std::to_string(static_cast<int>(Cp.SrcMix)) + "\t" + U3Flt(Cp.CtrF) + "\n";
		}
	}
	const ChannelX& X = C.X;
	if (!X.On)
	{
		return;
	}
	// The lifetime row and the recommender's standing always (they say the channel's X is on), the rest when they hold any.
	Out += "video\tvlife" + U3RowText(X.Life) + "\t" + std::to_string(X.Since) + "\n";
	Out += "video\tvmom\t" + Num(X.Momentum) + "\t" + std::to_string(X.SettledN);
	for (const float S : X.Settled)
	{
		Out += "\t" + U3Flt(S);
	}
	Out += "\t" + std::to_string(X.LastUploadDay) + "\t" + (X.WelcomeBack ? "1" : "0") + "\t" + std::to_string(X.TrendWeek) + "\n";
	for (const DayRow& R : X.Days)
	{
		Out += "video\tvday\t" + std::to_string(R.Key) + U3RowText(R) + "\n";
	}
	for (const DayRow& R : X.Months)
	{
		Out += "video\tvmonth\t" + std::to_string(R.Key) + U3RowText(R) + "\n";
	}
	for (const TopRow& R : X.Top)
	{
		Out += "video\tvtop\t" + Esc(R.Id) + "\t" + Num(R.Views) + "\t" + std::to_string(R.Cents) + "\t" + Num(R.At) + "\t" + Esc(R.Title) + "\n";
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
			DayRow R;
			R.Key = static_cast<int>(ToInt(P[2]));
			U3ReadRow(P, 3, R);
			(Tag == "vday" ? X.Days : X.Months).push_back(R);
			U3 = true;
		}
		else if (Tag == "vlife" && P.size() >= 12)
		{
			U3ReadRow(P, 2, X.Life);
			X.Since = static_cast<int>(ToInt(P[11]));
			U3 = true;
		}
		else if (Tag == "vmom" && P.size() >= 12)
		{
			X.Momentum = ToNum(P[2]);
			X.SettledN = static_cast<int>(ToInt(P[3]));
			for (size_t K = 0; K < X.Settled.size(); ++K)
			{
				X.Settled[K] = static_cast<float>(ToNum(P[4 + K]));
			}
			X.LastUploadDay = static_cast<int>(ToInt(P[10]));
			X.WelcomeBack = P[11] == "1";
			X.TrendWeek = P.size() >= 13 ? static_cast<int>(ToInt(P[12])) : -1;
			U3 = true;
		}
		else if (Tag == "vtop" && P.size() >= 7)
		{
			TopRow R;
			R.Id = Unesc(P[2]);
			R.Views = ToNum(P[3]);
			R.Cents = ToInt(P[4]);
			R.At = ToNum(P[5]);
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
