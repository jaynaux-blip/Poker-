#include "ShortStack/Game/Coach.h"
#include "../StrictFloat.h"

#include "ShortStack/AI/Grading.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Game/Format.h"

#include <algorithm>
#include <cstdlib>

// Coaching (next_segment_spec §2.11). core-stake.
//
// What a coach can honestly know: the graded online decisions, tallied by spot (the card room and Dee's game grade
// nothing), and the worst real hand in each spot since the last review. The review names the two spots that cost the
// most per decision, opens on a hand of the player's own, and leaves one piece of homework that the next review checks.
// Composure is a factor on the background's tilt gain (x0.95 a session, never under x0.80, fading back 2.5% a month
// without one); reads get one more confirmation for a week after Dee's morning-after (P2).

namespace ss
{
namespace coach
{
namespace coach_detail
{
constexpr double Day = 1440.0;

std::string Esc(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\')
		{
			Out += "\\\\";
		}
		else if (Ch == '\n')
		{
			Out += "\\n";
		}
		else if (Ch == '\t')
		{
			Out += "\\t";
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::string Unesc(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

const char* const Names[SpotCount] = {
	"Opening the pot",
	"Defending the blinds",
	"Facing three-bets",
	"Push or fold",
	"Calling shoves",
	"Betting the flop",
	"Facing flop bets",
	"Turn play",
	"River value",
	"River calls",
	"Bubble calls",
	"Bounty calls",
};

// The homework for each spot: one thing to fix, in Fred's voice.
const char* const Homework[SpotCount] = {
	"Open tighter from the first two seats. Your early-position range is a late-position range.",
	"Defend the big blind wider against a min-raise and fold more from the small blind.",
	"Against three-bets, stop flatting the hands that can't stand a four-bet. Shove them or fold them.",
	"Under fifteen blinds, shove first in or fold. The limp and the small raise are the leak.",
	"Call shoves with the range the price asks for, not the hand you hoped for.",
	"Check more flops in position on boards that hit the caller. You're betting into their range.",
	"Stop folding to one flop bet with the backdoors and overcards the price pays for.",
	"Your turn barrels need a reason: equity or a card that's good for your range.",
	"Bet thinner on the river when they check to you. You're leaving value behind.",
	"On the river, fold the bluff-catchers that beat nothing they'd bet.",
	"On the bubble, fold the marginal calls. The money jump is worth more than the chip.",
	"A bounty pays for some calls, not all of them. Count it as chips, then decide.",
};

const char* const RankNames[13] = {"deuces", "threes", "fours", "fives", "sixes", "sevens", "eights", "nines", "tens", "jacks", "queens", "kings", "aces"};
const char RankChars[14] = "23456789TJQKA";

std::string HandName(const std::vector<Card>& Hole)
{
	if (Hole.size() < 2)
	{
		return "a hand";
	}
	const int A = Hole[0] >> 2;
	const int B = Hole[1] >> 2;
	if (A == B)
	{
		return std::string("pocket ") + RankNames[A];
	}
	const int Hi = std::max(A, B);
	const int Lo = std::min(A, B);
	const bool Suited = (Hole[0] & 3) == (Hole[1] & 3);
	return std::string(1, RankChars[Hi]) + "-" + std::string(1, RankChars[Lo]) + (Suited ? " suited" : " off");
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return S;
}

std::string Bb(double V)
{
	return Fixed(V, 1);
}

/** The hand SpotOf saw last (the grading hook calls SpotOf, then Add, on the game thread): what Add keeps as a real hand. */
struct Seen
{
	int SpotIndex = -1;
	std::string Hand;
};
Seen& LastSeen()
{
	static Seen S;
	return S;
}
} // namespace coach_detail

using namespace coach_detail;

Spot SpotOf(const PlayerView& V, const DecisionGrade& G, bool Bubble, bool Bounty)
{
	const PublicSeat* Me = V.Me >= 0 && V.Me < static_cast<int>(V.Seats.size()) ? &V.Seats[static_cast<size_t>(V.Me)] : nullptr;
	const double BigBlind = static_cast<double>(std::max<Chips>(1, V.BigBlind));
	const double Stack = Me ? static_cast<double>(Me->Stack + Me->StreetBet) / BigBlind : 0.0;
	// Facing an all-in: someone's bet is all they had.
	bool FacingShove = false;
	for (const PublicSeat& S : V.Seats)
	{
		FacingShove = FacingShove || (S.AllIn && !S.Folded && S.StreetBet > 0 && V.ToCall > 0 && (!Me || S.Seat != Me->Seat));
	}
	Spot Out = Spot::Open;
	if (V.CurrentStreet == Street::Preflop)
	{
		const StreetSummary Pre = PreflopSummary(V);
		const bool InBlind = V.Me == V.BbIdx || V.Me == V.SbIdx;
		if (FacingShove && V.ToCall > 0)
		{
			Out = Bounty ? Spot::Bounty : Spot::CallShove;
		}
		else if (Pre.Raises >= 2 && V.ToCall > 0)
		{
			Out = Spot::Vs3Bet;
		}
		else if (Stack <= 15.0 && Pre.Raises == 0)
		{
			Out = Spot::PushFold;
		}
		else if (InBlind && Pre.Raises >= 1 && V.ToCall > 0)
		{
			Out = Spot::DefendBlind;
		}
		else
		{
			Out = Spot::Open;
		}
	}
	else if (V.CurrentStreet == Street::Flop)
	{
		Out = V.ToCall > 0 ? Spot::FacingFlop : Spot::Flop;
	}
	else if (V.CurrentStreet == Street::Turn)
	{
		Out = Spot::Turn;
	}
	else
	{
		Out = V.ToCall > 0 ? Spot::RiverCall : Spot::RiverValue;
	}
	if (Bubble && (Out == Spot::CallShove || Out == Spot::PushFold || Out == Spot::Vs3Bet))
	{
		Out = Spot::Bubble;
	}
	// The hand, as a coach would put it: the cards, the stack, what it faced, what was done and what was better.
	std::string Hand = HandName(V.Hole) + ", " + Bb(Stack) + " big blinds";
	if (FacingShove && V.ToCall > 0)
	{
		Hand += ", facing a " + Bb(static_cast<double>(V.ToCall) / BigBlind) + "-blind shove";
	}
	else if (V.ToCall > 0)
	{
		Hand += ", facing " + Bb(static_cast<double>(V.ToCall) / BigBlind) + " to call";
	}
	if (Bubble)
	{
		Hand += " on the bubble";
	}
	if (V.CurrentStreet != Street::Preflop)
	{
		Hand += std::string(" on the ") + Lower(StreetTitle(V.CurrentStreet));
	}
	Hand += ": " + Lower(G.Chosen.Label.empty() ? std::string("you acted") : "you chose " + G.Chosen.Label) + "; " + Lower(G.Best.Label.empty() ? std::string("the best play") : G.Best.Label) +
		" was " + Bb(G.LossBB) + " bb better.";
	LastSeen().SpotIndex = static_cast<int>(Out);
	LastSeen().Hand = Hand;
	return Out;
}

void Add(Notes& N, Spot S, const DecisionGrade& G)
{
	const int I = static_cast<int>(S);
	if (I < 0 || I >= SpotCount)
	{
		return;
	}
	const long long Loss = static_cast<long long>(std::max(0.0, G.LossBB) * 1000.0 + 0.5);
	const bool Blunder = G.Result == Grade::Blunder;
	for (Tally* T : {&N.All[static_cast<size_t>(I)], &N.SinceReview[static_cast<size_t>(I)]})
	{
		T->Decisions += 1;
		T->Blunders += Blunder ? 1 : 0;
		T->LossMilliBb += Loss;
	}
	Worst& W = N.Worsts[static_cast<size_t>(I)];
	if (Loss > W.LossMilliBb && LastSeen().SpotIndex == I)
	{
		W.LossMilliBb = Loss;
		W.Hand = LastSeen().Hand;
	}
	LastSeen().SpotIndex = -1;
}

const char* SpotName(Spot S)
{
	const int I = static_cast<int>(S);
	return I >= 0 && I < SpotCount ? Names[I] : "Everything else";
}

Review ReviewOf(const Notes& N, const std::string& Coach)
{
	Review R;
	for (const Tally& T : N.SinceReview)
	{
		R.Decisions += T.Decisions;
	}
	const std::string Who = Coach.empty() ? std::string("Your coach") : Coach;
	R.Enough = R.Decisions >= ReviewDecisions;
	if (!R.Enough)
	{
		R.Subject = Who + "'s review: not yet";
		R.Body = R.Decisions == 0 && N.Reviews == 0
			? "Play me two hundred hands and we'll talk. There's nothing graded to look at yet, and I don't review vibes."
			: "Play me two hundred hands and we'll talk. You've got " + std::to_string(R.Decisions) +
				" graded decisions since the last review, and the numbers don't mean anything until there are two hundred.";
		return R;
	}
	// The two spots that cost the most per decision, among the ones with enough of a sample to say so.
	int Best[2] = {-1, -1};
	double Per[2] = {0.0, 0.0};
	for (int I = 0; I < SpotCount; ++I)
	{
		const Tally& T = N.SinceReview[static_cast<size_t>(I)];
		if (T.Decisions < 15)
		{
			continue;
		}
		const double P = static_cast<double>(T.LossMilliBb) / 1000.0 / static_cast<double>(T.Decisions);
		if (Best[0] < 0 || P > Per[0])
		{
			Best[1] = Best[0];
			Per[1] = Per[0];
			Best[0] = I;
			Per[0] = P;
		}
		else if (Best[1] < 0 || P > Per[1])
		{
			Best[1] = I;
			Per[1] = P;
		}
	}
	if (Best[0] < 0)
	{
		Best[0] = 0;
	}
	if (Best[1] < 0)
	{
		Best[1] = Best[0];
	}
	R.Worst = static_cast<Spot>(Best[0]);
	R.Next = static_cast<Spot>(Best[1]);
	R.BbPer[0] = Per[0];
	R.BbPer[1] = Per[1];
	R.Counts[0] = N.SinceReview[static_cast<size_t>(Best[0])].Decisions;
	R.Counts[1] = N.SinceReview[static_cast<size_t>(Best[1])].Decisions;
	R.Hand = N.Worsts[static_cast<size_t>(Best[0])].Hand;
	R.Homework = Homework[Best[0]];
	R.Subject = Who + "'s review: " + Lower(SpotName(R.Worst));
	std::string Body = std::string(SpotName(R.Worst)) + ": " + std::to_string(R.Counts[0]) + " decisions, -" + Bb(R.BbPer[0]) + " bb each.";
	if (R.Next != R.Worst)
	{
		Body += " " + std::string(SpotName(R.Next)) + ": " + std::to_string(R.Counts[1]) + " decisions, -" + Bb(R.BbPer[1]) + " bb each.";
	}
	if (!R.Hand.empty())
	{
		std::string Hand = R.Hand;
		Hand[0] = static_cast<char>(Hand[0] >= 'a' && Hand[0] <= 'z' ? Hand[0] - 'a' + 'A' : Hand[0]);
		Body += " The one that cost you most: " + Hand;
	}
	if (N.Homework >= 0 && N.Homework < SpotCount)
	{
		const Tally& T = N.SinceReview[static_cast<size_t>(N.Homework)];
		const double Before = static_cast<double>(N.HomeworkMilliBb) / 1000.0;
		if (T.Decisions >= 15)
		{
			const double Now = static_cast<double>(T.LossMilliBb) / 1000.0 / static_cast<double>(T.Decisions);
			Body += " Last month's homework, " + Lower(Names[N.Homework]) + ": " + Bb(Before) + " bb a decision then, " + Bb(Now) + " now." +
				(Now + 0.05 < Before ? " That's the work paying." : " Still there. Keep at it.");
		}
		else
		{
			Body += " Last month's homework, " + Lower(Names[N.Homework]) + ": not enough hands in it to tell yet.";
		}
	}
	Body += " Homework: " + R.Homework;
	R.Body = Body.size() > 600 ? Body.substr(0, Body.rfind(". ", 596) + 1) : Body;
	return R;
}

void Reviewed(Notes& N, const Review& R, double World)
{
	N.LastReview = World;
	N.Reviews += 1;
	if (R.Enough)
	{
		N.Homework = static_cast<int>(R.Worst);
		N.HomeworkMilliBb = static_cast<long long>(R.BbPer[0] * 1000.0 + 0.5);
		N.SinceReview = std::array<Tally, 12>{};
		N.Worsts = std::array<Worst, 12>{};
	}
}

double TiltGainNow(const Notes& N, double Background, double World)
{
	// Composure fades back 2.5% for every 30 days without a session; a coach never takes it under x0.80.
	double Factor = N.TiltFactor;
	if (Factor < 1.0 && N.TiltSince > 0.0 && World > N.TiltSince)
	{
		const double Months = static_cast<double>(static_cast<long long>((World - N.TiltSince) / (30.0 * Day)));
		Factor = std::min(1.0, Factor + 0.025 * Months);
	}
	return Background * std::max(0.80, std::min(1.0, Factor));
}

int ReadWeightNow(const Notes& N, int Background, double World)
{
	return Background + (World < N.ReadsUntil ? 1 : 0);
}

std::vector<std::pair<std::string, std::string>> Inventory()
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (int I = 0; I < SpotCount; ++I)
	{
		Out.push_back({std::string("homework/") + Names[I], Homework[I]});
		Out.push_back({std::string("spot/") + Names[I], std::string(Names[I]) + ": {Count} decisions, -{Bb} bb each."});
	}
	Out.push_back({"empty", "Play me two hundred hands and we'll talk. There's nothing graded to look at yet, and I don't review vibes."});
	Out.push_back({"short", "Play me two hundred hands and we'll talk. You've got {Count} graded decisions since the last review, and the numbers don't mean anything until there are two hundred."});
	Out.push_back({"homework.better", "Last month's homework, {Spot}: {Bb} bb a decision then, {Bb} now. That's the work paying."});
	Out.push_back({"homework.same", "Last month's homework, {Spot}: {Bb} bb a decision then, {Bb} now. Still there. Keep at it."});
	Out.push_back({"homework.thin", "Last month's homework, {Spot}: not enough hands in it to tell yet."});
	Out.push_back({"hand", "The one that cost you most: A-T off, 22.0 big blinds, facing a 9.0-blind shove on the bubble: you chose call; fold was 0.6 bb better."});
	return Out;
}

void WriteLines(const Notes& N, std::string& Out)
{
	for (int I = 0; I < SpotCount; ++I)
	{
		const Tally& A = N.All[static_cast<size_t>(I)];
		const Tally& S = N.SinceReview[static_cast<size_t>(I)];
		if (A.Decisions > 0 || S.Decisions > 0)
		{
			Out += "coach\ttally\t" + std::to_string(I) + "\t" + std::to_string(A.Decisions) + "\t" + std::to_string(A.Blunders) + "\t" + std::to_string(A.LossMilliBb) + "\t" +
				std::to_string(S.Decisions) + "\t" + std::to_string(S.LossMilliBb) + "\t" + std::to_string(S.Blunders) + "\n";
		}
	}
	const bool Notes1 = N.LastReview != 0.0 || N.TiltFactor != 1.0 || N.TiltSince != 0.0 || N.ReadsUntil != 0.0 || N.Sessions != 0 || N.Homework != -1 || N.Reviews != 0;
	if (Notes1)
	{
		Out += "coach\tnotes\t" + Fixed(N.LastReview, 2) + "\t" + Fixed(N.TiltFactor, 4) + "\t" + Fixed(N.TiltSince, 2) + "\t" + Fixed(N.ReadsUntil, 2) + "\t" +
			std::to_string(N.Sessions) + "\t" + std::to_string(N.Homework) + "\t" + std::to_string(N.HomeworkMilliBb) + "\t" + std::to_string(N.Reviews) + "\n";
	}
	for (int I = 0; I < SpotCount; ++I)
	{
		const Worst& W = N.Worsts[static_cast<size_t>(I)];
		if (W.LossMilliBb > 0 && !W.Hand.empty())
		{
			Out += "coach\tworst\t" + std::to_string(I) + "\t" + std::to_string(W.LossMilliBb) + "\t" + Esc(W.Hand) + "\n";
		}
	}
}

bool ReadLine(const std::vector<std::string>& P, Notes& N)
{
	if (P.size() < 2 || P[0] != "coach")
	{
		return false;
	}
	if (P[1] == "tally" && P.size() >= 8)
	{
		const int I = std::atoi(P[2].c_str());
		if (I >= 0 && I < SpotCount)
		{
			Tally& A = N.All[static_cast<size_t>(I)];
			Tally& S = N.SinceReview[static_cast<size_t>(I)];
			A.Decisions = std::atoi(P[3].c_str());
			A.Blunders = std::atoi(P[4].c_str());
			A.LossMilliBb = std::atoll(P[5].c_str());
			S.Decisions = std::atoi(P[6].c_str());
			S.LossMilliBb = std::atoll(P[7].c_str());
			S.Blunders = P.size() > 8 ? std::atoi(P[8].c_str()) : 0;
		}
	}
	else if (P[1] == "notes" && P.size() >= 7)
	{
		N.LastReview = std::strtod(P[2].c_str(), nullptr);
		N.TiltFactor = std::strtod(P[3].c_str(), nullptr);
		N.TiltSince = std::strtod(P[4].c_str(), nullptr);
		N.ReadsUntil = std::strtod(P[5].c_str(), nullptr);
		N.Sessions = std::atoi(P[6].c_str());
		N.Homework = P.size() > 7 ? std::atoi(P[7].c_str()) : -1;
		N.HomeworkMilliBb = P.size() > 8 ? std::atoll(P[8].c_str()) : 0;
		N.Reviews = P.size() > 9 ? std::atoi(P[9].c_str()) : 0;
	}
	else if (P[1] == "worst" && P.size() >= 5)
	{
		const int I = std::atoi(P[2].c_str());
		if (I >= 0 && I < SpotCount)
		{
			N.Worsts[static_cast<size_t>(I)].LossMilliBb = std::atoll(P[3].c_str());
			N.Worsts[static_cast<size_t>(I)].Hand = Unesc(P[4]);
		}
	}
	return true;
}
} // namespace coach
} // namespace ss
