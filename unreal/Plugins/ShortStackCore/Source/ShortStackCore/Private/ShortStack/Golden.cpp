#include "ShortStack/Golden.h"

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Grading.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/AI/View.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Names.h"
#include "ShortStack/Structure.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>

namespace ss
{
namespace golden_detail
{
using Tokens = std::vector<std::string>;

Tokens Split(const std::string& Line)
{
	Tokens Out;
	std::string Cur;
	for (char Ch : Line)
	{
		if (Ch == ' ')
		{
			if (!Cur.empty())
			{
				Out.push_back(Cur);
				Cur.clear();
			}
		}
		else if (Ch != '\r')
		{
			Cur += Ch;
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

std::string Hex(double X)
{
	uint64_t Bits;
	std::memcpy(&Bits, &X, sizeof(Bits));
	char Buf[17];
	std::snprintf(Buf, sizeof(Buf), "%016llx", static_cast<unsigned long long>(Bits));
	return Buf;
}

std::string Word(const std::string& S)
{
	std::string Out = S;
	for (char& Ch : Out)
	{
		if (Ch == ' ')
		{
			Ch = '_';
		}
	}
	return Out;
}

/** Cursor over a record's tokens. */
struct Reader
{
	const Tokens& T;
	size_t Pos = 1;
	explicit Reader(const Tokens& InT) : T(InT) {}
	bool More() const { return Pos < T.size(); }
	const std::string& Str() { return Pos < T.size() ? T[Pos++] : T.back(); }
	long long Int() { return std::strtoll(Str().c_str(), nullptr, 10); }
	int I32() { return static_cast<int>(Int()); }
	uint32_t U32() { return static_cast<uint32_t>(std::strtoull(Str().c_str(), nullptr, 10)); }
};

class Checker
{
public:
	GoldenResult Result;
	std::map<std::string, std::pair<int, int>> PerType; // passed, failed
	std::map<std::string, int> SkippedTypes;
	int ReportedFailures = 0;
	// State carried between records.
	std::map<std::string, std::unique_ptr<Rng>> ProfileRngs;
	std::map<std::string, std::deque<std::string>> PendingGrades;
	std::map<std::string, std::deque<std::string>> PendingTicks;
	std::map<std::string, std::string> TourneyResults;

	void Check(const std::string& Type, int LineNo, bool Ok, const std::string& Detail)
	{
		std::pair<int, int>& P = PerType[Type];
		if (Ok)
		{
			++Result.Passed;
			++P.first;
			return;
		}
		++Result.Failed;
		++P.second;
		if (ReportedFailures < 25)
		{
			++ReportedFailures;
			Result.Report += "MISMATCH line " + std::to_string(LineNo) + " [" + Type + "]: " + Detail + "\n";
		}
	}

	void Skip(const std::string& Type)
	{
		++Result.Skipped;
		++SkippedTypes[Type];
	}
};

// ---------------------------------------------------------------- record handlers

void CheckRng(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int N = R.I32();
	Rng Gen(Seed);
	for (int I = 0; I < N; ++I)
	{
		const uint32_t Want = R.U32();
		const uint32_t Got = Gen.NextUint32();
		if (Want != Got)
		{
			K.Check("rng", LineNo, false, "seed " + Seed + " output " + std::to_string(I) + ": want " + std::to_string(Want) + " got " + std::to_string(Got));
			return;
		}
	}
	K.Check("rng", LineNo, true, "");
}

void CheckRngInt(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int N = R.I32();
	Rng Gen(Seed);
	for (int I = 0; I < N; ++I)
	{
		const int Want = R.I32();
		const int Got = Gen.Int(7 + I * 13);
		if (Want != Got)
		{
			K.Check("rngint", LineNo, false, "seed " + Seed + " draw " + std::to_string(I));
			return;
		}
	}
	K.Check("rngint", LineNo, true, "");
}

void CheckRngFloat(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int N = R.I32();
	Rng Gen(Seed);
	for (int I = 0; I < N; ++I)
	{
		const std::string Want = R.Str();
		const double V = I < 12 ? Gen.Next() : Gen.Gauss(1.5, 0.3);
		if (Want != Hex(V))
		{
			K.Check("rngf", LineNo, false, "seed " + Seed + " value " + std::to_string(I));
			return;
		}
	}
	K.Check("rngf", LineNo, true, "");
}

void CheckShuffle(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	Rng Gen(Seed);
	std::vector<Card> Deck = FullDeck();
	Gen.Shuffle(Deck);
	for (int I = 0; I < 52; ++I)
	{
		if (R.I32() != Deck[static_cast<size_t>(I)])
		{
			K.Check("shuffle", LineNo, false, "seed " + Seed + " position " + std::to_string(I));
			return;
		}
	}
	K.Check("shuffle", LineNo, true, "");
}

void CheckFork(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const std::string Label = R.Str();
	Rng Parent(Seed);
	Rng Child = Parent.Fork(Label);
	bool Ok = true;
	for (int I = 0; I < 3; ++I)
	{
		Ok = Ok && R.U32() == Child.NextUint32();
	}
	K.Check("fork", LineNo, Ok, "seed " + Seed);
}

void CheckDetPow(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const double Got[5] = {DetPow(0.37, 2.2), DetPow(7, 0.95), DetPow(113, 0.85), DetPow(0.61, 1), DetPow(0.5, 0)};
	bool Ok = true;
	for (double G : Got)
	{
		Ok = Ok && R.Str() == Hex(G);
	}
	K.Check("detpow", LineNo, Ok, "DetPow differs");
}

void CheckEval(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const int N = R.I32();
	Card Cards[7];
	for (int I = 0; I < N; ++I)
	{
		Cards[I] = R.I32();
	}
	const int Want = R.I32();
	const int Got = Evaluate(Cards, N);
	K.Check("eval", LineNo, Want == Got, "want " + std::to_string(Want) + " got " + std::to_string(Got));
}

void CheckDesc(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const int Score = R.I32();
	const std::string Want = R.Str();
	const std::string Got = Word(Describe(Score));
	K.Check("desc", LineNo, Want == Got, "want " + Want + " got " + Got);
}

void CheckPercentile(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	bool Ok = true;
	for (int I = 0; I < 169; ++I)
	{
		Ok = Ok && R.Str() == Hex(ClassPercentile()[I]);
	}
	K.Check("pct", LineNo, Ok, "class percentile table differs");
}

std::vector<Card> ReadCards(Reader& R, int N)
{
	std::vector<Card> Out;
	for (int I = 0; I < N; ++I)
	{
		Out.push_back(R.I32());
	}
	return Out;
}

void CheckEquityKnown(Checker& K, int LineNo, const Tokens& T, bool MonteCarlo)
{
	Reader R(T);
	std::string Seed = "unused";
	int Samples = 12000;
	if (MonteCarlo)
	{
		Seed = R.Str();
		Samples = R.I32();
	}
	const int Players = R.I32();
	std::vector<std::vector<Card>> Hands;
	for (int P = 0; P < Players; ++P)
	{
		Hands.push_back(ReadCards(R, 2));
	}
	std::vector<Card> Board;
	if (!MonteCarlo)
	{
		Board = ReadCards(R, R.I32());
	}
	Rng Gen(Seed);
	const std::vector<double> Eq = EquityKnownHands(Hands, Board, Gen, Samples);
	bool Ok = true;
	for (double V : Eq)
	{
		Ok = Ok && R.Str() == Hex(V);
	}
	K.Check(MonteCarlo ? "eqkmc" : "eqk", LineNo, Ok, "known-hand equity differs");
}

void CheckEquityRanges(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int Iterations = R.I32();
	const std::vector<Card> Hero = ReadCards(R, 2);
	const std::vector<Card> Board = ReadCards(R, R.I32());
	const int NR = R.I32();
	std::vector<RangeBand> Ranges;
	for (int I = 0; I < NR; ++I)
	{
		RangeBand B;
		uint64_t Bits = std::strtoull(R.Str().c_str(), nullptr, 16);
		std::memcpy(&B.Min, &Bits, sizeof(double));
		Bits = std::strtoull(R.Str().c_str(), nullptr, 16);
		std::memcpy(&B.Max, &Bits, sizeof(double));
		Ranges.push_back(B);
	}
	Rng Gen(Seed);
	const double E = EquityVsRanges(Hero, Board, Ranges, Iterations, Gen);
	const std::string Want = R.Str();
	K.Check("eqr", LineNo, Want == Hex(E), "want " + Want + " got " + Hex(E));
}
const char* StreetCode(Street S)
{
	switch (S)
	{
	case Street::Preflop: return "p";
	case Street::Flop: return "f";
	case Street::Turn: return "t";
	case Street::River: return "r";
	default: return "s";
	}
}

const char* ActionCode(ActionType A)
{
	switch (A)
	{
	case ActionType::Fold: return "f";
	case ActionType::Check: return "k";
	case ActionType::Call: return "c";
	case ActionType::Bet: return "b";
	default: return "r";
	}
}

template <typename T>
std::string Join(const std::vector<T>& V)
{
	std::string S;
	for (size_t I = 0; I < V.size(); ++I)
	{
		if (I)
		{
			S += ',';
		}
		S += std::to_string(V[I]);
	}
	return S;
}

/** Canonical event log, identical to eventLog() in web/scripts/gen-golden.ts. */
std::string EventLog(const std::vector<HandEvent>& Events)
{
	std::string S;
	for (const HandEvent& E : Events)
	{
		switch (E.Type)
		{
		case EventType::Ante:
			S += "A" + std::to_string(E.Seat) + "," + std::to_string(E.Amount) + ";";
			break;
		case EventType::Blind:
			S += std::string("B") + (E.IsSmallBlind ? "s" : "b") + std::to_string(E.Seat) + "," + std::to_string(E.Amount) + ";";
			break;
		case EventType::Deal:
			S += "D" + Join(E.Seats) + ";";
			break;
		case EventType::Action:
		{
			const ActionRecord& A = E.Action;
			S += std::string("X") + StreetCode(A.OnStreet) + std::to_string(A.Seat) + ActionCode(A.Type) + "," + std::to_string(A.Added) + "," + std::to_string(A.To) + "," + (A.AllIn ? "1" : "0") + "," +
				std::to_string(A.PotBefore) + "," + std::to_string(A.Facing) + ";";
			break;
		}
		case EventType::Street:
			S += std::string("S") + StreetCode(E.NewStreet) + Join(E.Cards) + ";";
			break;
		case EventType::Return:
			S += "R" + std::to_string(E.Seat) + "," + std::to_string(E.Amount) + ";";
			break;
		case EventType::Reveal:
			S += "V" + Join(E.Seats) + ";";
			break;
		case EventType::Showdown:
		{
			S += "W";
			for (size_t I = 0; I < E.Hands.size(); ++I)
			{
				if (I)
				{
					S += "|";
				}
				S += std::to_string(E.Hands[I].Seat) + ":" + Join(E.Hands[I].Hole) + ":" + std::to_string(E.Hands[I].Score);
			}
			S += ";";
			break;
		}
		case EventType::Award:
			S += "P" + std::to_string(E.PotIndex) + "/" + std::to_string(E.PotCount) + ":" + std::to_string(E.Pot.Amount) + ":" + Join(E.Pot.Eligible) + ":" + Join(E.Pot.Winners) + ":" + Join(E.Pot.Shares) + ";";
			break;
		case EventType::End:
			S += "E;";
			break;
		}
	}
	return S;
}

PlayerAction ParseAction(const std::string& Tok)
{
	if (Tok == "f") return PlayerAction::Fold();
	if (Tok == "k") return PlayerAction::Check();
	if (Tok == "c") return PlayerAction::Call();
	return PlayerAction::RaiseTo(std::strtod(Tok.c_str() + 2, nullptr));
}

std::string ActionToken(const PlayerAction& A)
{
	switch (A.Type)
	{
	case PlayerAction::Kind::Fold: return "f";
	case PlayerAction::Kind::Check: return "k";
	case PlayerAction::Kind::Call: return "c";
	default:
	{
		// Raise targets are whole numbers after legalization; print like JavaScript does.
		char Buf[40];
		std::snprintf(Buf, sizeof(Buf), "r:%.17g", A.To);
		return Buf;
	}
	}
}

void CheckHand(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int N = R.I32();
	HandConfig Cfg;
	Cfg.ButtonSeat = R.I32();
	Cfg.SmallBlind = R.Int();
	Cfg.BigBlind = R.Int();
	Cfg.Ante = R.Int();
	for (int I = 0; I < N; ++I)
	{
		SeatInput P;
		P.Seat = I;
		P.Id = "p" + std::to_string(I);
		P.Stack = R.Int();
		Cfg.Players.push_back(P);
	}
	Rng Gen(Seed);
	Hand H(Cfg, Gen);
	const int NA = R.I32();
	for (int I = 0; I < NA; ++I)
	{
		if (!H.Act(ParseAction(R.Str())))
		{
			K.Check("hand", LineNo, false, Seed + ": action " + std::to_string(I) + " rejected");
			return;
		}
	}
	R.Str(); // "|"
	std::string Detail;
	bool Ok = H.bComplete;
	for (int I = 0; I < N; ++I)
	{
		const long long Want = R.Int();
		if (Want != H.Seats[static_cast<size_t>(I)].Stack)
		{
			Ok = false;
			Detail += " stack" + std::to_string(I);
		}
	}
	const uint32_t WantHash = R.U32();
	const uint32_t GotHash = Fnv1a(EventLog(H.Events));
	if (WantHash != GotHash)
	{
		Ok = false;
		Detail += " eventlog";
	}
	K.Check("hand", LineNo, Ok, Seed + Detail);
}

double HexToDouble(const std::string& H)
{
	const uint64_t Bits = std::strtoull(H.c_str(), nullptr, 16);
	double D;
	std::memcpy(&D, &Bits, sizeof(D));
	return D;
}

void CheckProfile(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	R.Str(); // index
	Archetype A = Archetype::Tag;
	ArchetypeFromName(R.Str(), A);
	std::unique_ptr<Rng>& Gen = K.ProfileRngs[Seed];
	if (!Gen)
	{
		Gen.reset(new Rng(Seed));
	}
	const Profile P = MakeProfile(A, *Gen);
	bool Ok = R.Str() == TimingName(P.Timing);
	const double Fields[14] = {P.OpenWidth, P.LimpRate, P.ThreeBet, P.CallWidth, P.Aggression, P.Bluff, P.Stickiness, P.Cbet, P.Sizing, P.TiltProne, P.PushFold, P.IcmAware, P.ThinkBase, P.ThinkVar};
	for (double F : Fields)
	{
		Ok = Ok && R.Str() == Hex(F);
	}
	K.Check("profile", LineNo, Ok, "profile fields differ for " + std::string(ArchetypeName(A)));
}

void CheckBotHand(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const bool Fast = R.I32() == 1;
	const int N = R.I32();
	HandConfig Cfg;
	Cfg.ButtonSeat = R.I32();
	Cfg.SmallBlind = R.Int();
	Cfg.BigBlind = R.Int();
	Cfg.Ante = R.Int();
	for (int I = 0; I < N; ++I)
	{
		SeatInput P;
		P.Seat = I;
		P.Id = "p" + std::to_string(I);
		P.Stack = R.Int();
		Cfg.Players.push_back(P);
	}
	std::vector<Archetype> Archs;
	for (int I = 0; I < N; ++I)
	{
		Archetype A = Archetype::Tag;
		ArchetypeFromName(R.Str(), A);
		Archs.push_back(A);
	}
	std::vector<double> Tilts;
	std::vector<double> Pressures;
	for (int I = 0; I < N; ++I)
	{
		Tilts.push_back(HexToDouble(R.Str()));
	}
	for (int I = 0; I < N; ++I)
	{
		Pressures.push_back(HexToDouble(R.Str()));
	}
	const std::string ProfSeed = R.Str();
	R.Str(); // "|"
	const std::string Index = Seed.substr(3); // "bot<t>"

	Rng Gen(Seed);
	Rng ProfRng(ProfSeed);
	std::vector<Profile> Profiles;
	for (Archetype A : Archs)
	{
		Profiles.push_back(MakeProfile(A, ProfRng));
	}
	Hand H(Cfg, Gen);
	std::vector<std::string> Decisions;
	std::deque<std::string>& Grades = K.PendingGrades[Seed];
	int Step = 0;
	while (!H.bComplete)
	{
		const int Idx = H.ToAct;
		const int Seat = H.Seats[static_cast<size_t>(Idx)].Seat;
		const PlayerView View = MakeView(H, Idx);
		std::unique_ptr<DecisionAnalysis> Analysis;
		if (!Fast && Seat == 0)
		{
			Rng GradeRng("g" + Index + ":" + std::to_string(Step));
			Analysis.reset(new DecisionAnalysis(AnalyzeDecision(MakeView(H, Idx), GradeRng, 300)));
		}
		BotContext Ctx;
		Ctx.Prof = &Profiles[static_cast<size_t>(Seat)];
		Ctx.Tilt = Tilts[static_cast<size_t>(Seat)];
		Ctx.IcmPressure = Pressures[static_cast<size_t>(Seat)];
		Ctx.Fast = Fast;
		Ctx.R = &Gen;
		const BotDecision D = Decide(View, Ctx);
		Decisions.push_back(ActionToken(D.Action) + "/" + Hex(D.ThinkMs) + "/" + Hex(D.Equity));
		if (Analysis)
		{
			const DecisionGrade G = GradeDecision(*Analysis, D.Action);
			std::string Line = std::to_string(Step) + " " + Hex(Analysis->Equity) + " " + Hex(Analysis->PotOdds) + " " + std::to_string(Analysis->Options.size());
			for (const OptionEV& O : Analysis->Options)
			{
				Line += " " + Word(O.Label) + " " + ActionToken(O.Action) + " " + Hex(O.Ev);
			}
			Line += std::string(" ") + GradeName(G.Result) + " " + Word(G.Chosen.Label) + " " + Hex(G.Chosen.Ev) + " " + Word(G.Best.Label) + " " + Hex(G.LossBB) + " " + Word(G.Note);
			Grades.push_back(Line);
		}
		if (!H.Act(D.Action))
		{
			K.Check("bothand", LineNo, false, Seed + ": bot action rejected at step " + std::to_string(Step));
			return;
		}
		++Step;
	}
	const int ND = R.I32();
	bool Ok = ND == static_cast<int>(Decisions.size());
	std::string Detail;
	for (int I = 0; I < ND && Ok; ++I)
	{
		const std::string Want = R.Str();
		if (Want != Decisions[static_cast<size_t>(I)])
		{
			Ok = false;
			Detail = " decision " + std::to_string(I) + ": want " + Want + " got " + Decisions[static_cast<size_t>(I)];
		}
	}
	if (Ok)
	{
		R.Str(); // "|"
		for (int I = 0; I < N; ++I)
		{
			if (R.Int() != H.Seats[static_cast<size_t>(I)].Stack)
			{
				Ok = false;
				Detail += " stacks";
			}
		}
		if (R.U32() != Fnv1a(EventLog(H.Events)))
		{
			Ok = false;
			Detail += " eventlog";
		}
		if (R.U32() != Gen.NextUint32())
		{
			Ok = false;
			Detail += " rng-state";
		}
	}
	K.Check("bothand", LineNo, Ok, Seed + Detail);
}

void CheckGrade(Checker& K, int LineNo, const Tokens& T, const std::string& Line)
{
	const std::string& Seed = T[1];
	std::deque<std::string>& Pending = K.PendingGrades[Seed];
	const std::string Want = Line.substr(std::string("grade ").size() + Seed.size() + 1);
	if (Pending.empty())
	{
		K.Check("grade", LineNo, false, Seed + ": no grade computed");
		return;
	}
	const std::string Got = Pending.front();
	Pending.pop_front();
	K.Check("grade", LineNo, Got == Want, Seed + "\n    want " + Want + "\n    got  " + Got);
}

void CheckPayout(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const Chips Pool = R.Int();
	const int Entrants = R.I32();
	const Chips MinCash = R.Int();
	const int NP = R.I32();
	const std::vector<Chips> Got = PayoutTable(Pool, Entrants, MinCash);
	bool Ok = static_cast<int>(Got.size()) == NP;
	for (int I = 0; I < NP && Ok; ++I)
	{
		Ok = R.Int() == Got[static_cast<size_t>(I)];
	}
	K.Check("payout", LineNo, Ok, "payouts for " + std::to_string(Entrants) + " entrants");
}

void CheckIcm(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const int N = R.I32();
	std::vector<double> Stacks;
	for (int I = 0; I < N; ++I)
	{
		Stacks.push_back(static_cast<double>(R.Int()));
	}
	const int M = R.I32();
	std::vector<double> Prizes;
	for (int I = 0; I < M; ++I)
	{
		Prizes.push_back(static_cast<double>(R.Int()));
	}
	const std::vector<double> Got = Icm(Stacks, Prizes);
	bool Ok = true;
	for (double V : Got)
	{
		Ok = Ok && R.Str() == Hex(V);
	}
	K.Check("icm", LineNo, Ok, "ICM values differ");
}

void CheckNames(Checker& K, int LineNo, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int Count = R.I32();
	const int NR = R.I32();
	std::vector<std::string> Reserved;
	for (int I = 0; I < NR; ++I)
	{
		Reserved.push_back(R.Str());
	}
	Rng Gen(Seed);
	const std::vector<std::string> Got = UniqueNames(Count, Gen, Reserved);
	for (int I = 0; I < Count; ++I)
	{
		const std::string Want = R.Str();
		if (Want != Got[static_cast<size_t>(I)])
		{
			K.Check("names", LineNo, false, "name " + std::to_string(I) + ": want " + Want + " got " + Got[static_cast<size_t>(I)]);
			return;
		}
	}
	K.Check("names", LineNo, true, "");
}

const char* TEventName(TEventType E)
{
	switch (E)
	{
	case TEventType::Bust: return "bust";
	case TEventType::Level: return "level";
	case TEventType::Moved: return "moved";
	case TEventType::TableBroken: return "tableBroken";
	case TEventType::HandForHand: return "handForHand";
	case TEventType::Bubble: return "bubble";
	case TEventType::FinalTable: return "finalTable";
	default: return "finished";
	}
}

/** Runs a whole tournament like runTourney() in gen-golden.ts and queues its checkpoints. */
void RunTourney(Checker& K, const Tokens& T)
{
	Reader R(T);
	const std::string Seed = R.Str();
	const int MaxTicks = R.I32();
	TournamentSpec Spec;
	Spec.Id = R.Str();
	Spec.BuyInCents = R.Int();
	Spec.FeeCents = R.Int();
	Spec.GuaranteeCents = R.Int();
	Spec.Entrants = R.I32();
	Spec.StartingStack = R.Int();
	Spec.LevelMinutes = static_cast<double>(R.Int());
	Spec.SecondsPerHand = static_cast<double>(R.Int());
	Spec.Population = R.Str();
	Spec.Speed = R.Str();
	Spec.StartClock = static_cast<double>(R.Int());
	Tournament Tour(Spec, "Hero", Seed, {{"gh0stfold", Archetype::Crusher}});
	const Profile Sprint = SprintProfile();
	std::deque<std::string>& Ticks = K.PendingTicks[Seed];
	while (!Tour.bFinished && Tour.Tick < MaxTicks)
	{
		if (Tour.Tick == 12)
		{
			Tour.MoveToTable("npc:gh0stfold", Tour.Hero().TableId);
		}
		const std::vector<TEvent> Events = Tour.SimulateTick(&Sprint);
		std::string S;
		for (const TPlayer& P : Tour.Players)
		{
			S += P.Id + ":" + std::to_string(P.Stack) + ":" + std::to_string(P.TableId) + ":" + std::to_string(P.Seat) + ":" + std::to_string(P.Busted ? P.Place : 0) + ";";
		}
		for (const TEvent& E : Events)
		{
			const bool HasPlace = E.Type == TEventType::Bust;
			const bool HasId = E.Type == TEventType::Bust || E.Type == TEventType::Moved;
			const bool HasTo = E.Type == TEventType::Moved;
			const bool HasTable = E.Type == TEventType::Bust || E.Type == TEventType::TableBroken;
			S += std::string(TEventName(E.Type)) + ":" + (HasPlace ? std::to_string(E.Place) : "") + ":" + (HasId ? E.Id : "") + ":" + (HasTo ? std::to_string(E.To) : "") + ":" +
				(HasTable ? std::to_string(E.TableId) : "") + ";";
		}
		Ticks.push_back(std::to_string(Tour.Tick) + " " + std::to_string(Tour.Remaining) + " " + std::to_string(Tour.LevelIndex) + " " + std::to_string(Fnv1a(S)));
	}
	std::string Res = std::to_string(Tour.Tick) + " " + std::to_string(Tour.Remaining) + " " + (Tour.bFinished ? "1" : "0") + " " + std::to_string(Tour.PrizePoolCents) + " " +
		std::to_string(Tour.Payouts.size()) + " " + std::to_string(Tour.Players.size());
	for (const TPlayer& P : Tour.Players)
	{
		Res += " " + P.Id + "=" + std::to_string(P.Busted || P.Place == 1 ? P.Place : 0) + "/" + std::to_string(P.PrizeCents);
	}
	K.TourneyResults[Seed] = Res;
}

void CheckTick(Checker& K, int LineNo, const Tokens& T, const std::string& Line)
{
	const std::string& Seed = T[1];
	std::deque<std::string>& Pending = K.PendingTicks[Seed];
	const std::string Want = Line.substr(std::string("tick ").size() + Seed.size() + 1);
	if (Pending.empty())
	{
		K.Check("tick", LineNo, false, Seed + ": tournament ended early");
		return;
	}
	const std::string Got = Pending.front();
	Pending.pop_front();
	K.Check("tick", LineNo, Got == Want, Seed + " round state differs: want " + Want + " got " + Got);
}

void CheckTourneyResult(Checker& K, int LineNo, const Tokens& T, const std::string& Line)
{
	const std::string& Seed = T[1];
	const std::string Want = Line.substr(std::string("tresult ").size() + Seed.size() + 1);
	const std::string& Got = K.TourneyResults[Seed];
	K.Check("tresult", LineNo, Got == Want, Seed + " final standings differ");
}
} // namespace golden_detail

GoldenResult RunGoldenVectors(const std::string& FileText)
{
	using namespace golden_detail;
	Checker K;
	size_t Start = 0;
	int LineNo = 0;
	while (Start < FileText.size())
	{
		size_t End = FileText.find('\n', Start);
		if (End == std::string::npos)
		{
			End = FileText.size();
		}
		const std::string Line = FileText.substr(Start, End - Start);
		Start = End + 1;
		++LineNo;
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		const Tokens T = Split(Line);
		if (T.empty())
		{
			continue;
		}
		const std::string& Type = T[0];
		if (Type == "rng") CheckRng(K, LineNo, T);
		else if (Type == "rngint") CheckRngInt(K, LineNo, T);
		else if (Type == "rngf") CheckRngFloat(K, LineNo, T);
		else if (Type == "shuffle") CheckShuffle(K, LineNo, T);
		else if (Type == "fork") CheckFork(K, LineNo, T);
		else if (Type == "detpow") CheckDetPow(K, LineNo, T);
		else if (Type == "eval") CheckEval(K, LineNo, T);
		else if (Type == "desc") CheckDesc(K, LineNo, T);
		else if (Type == "pct") CheckPercentile(K, LineNo, T);
		else if (Type == "eqk") CheckEquityKnown(K, LineNo, T, false);
		else if (Type == "eqkmc") CheckEquityKnown(K, LineNo, T, true);
		else if (Type == "eqr") CheckEquityRanges(K, LineNo, T);
		else if (Type == "hand") CheckHand(K, LineNo, T);
		else if (Type == "profile") CheckProfile(K, LineNo, T);
		else if (Type == "bothand") CheckBotHand(K, LineNo, T);
		else if (Type == "grade") CheckGrade(K, LineNo, T, Line);
		else if (Type == "payout") CheckPayout(K, LineNo, T);
		else if (Type == "icm") CheckIcm(K, LineNo, T);
		else if (Type == "names") CheckNames(K, LineNo, T);
		else if (Type == "tourney") RunTourney(K, T);
		else if (Type == "tick") CheckTick(K, LineNo, T, Line);
		else if (Type == "tresult") CheckTourneyResult(K, LineNo, T, Line);
		else K.Skip(Type);
	}
	std::string Summary;
	for (const auto& It : K.PerType)
	{
		Summary += "  " + It.first + ": " + std::to_string(It.second.first) + " passed";
		if (It.second.second)
		{
			Summary += ", " + std::to_string(It.second.second) + " FAILED";
		}
		Summary += "\n";
	}
	for (const auto& It : K.SkippedTypes)
	{
		Summary += "  " + It.first + ": " + std::to_string(It.second) + " skipped (not ported yet)\n";
	}
	K.Result.Report = Summary + K.Result.Report;
	return K.Result;
}
} // namespace ss
