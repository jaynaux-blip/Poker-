// rerun_content (update 3 SPEC §6, §7.7, suite 20): Rerun 2.0's library, core-videocontent's suite.
//   Titles: at least 1,600 templates, every cell at its minimum (§6.2's table), each filling from a fixture candidate of its
//   kind or idea, filled <= 100 characters (a Cut's <= 50), linted (tests/CopyLint.h), U1, the trademark screen (both tiers),
//   every digit run a fact of the moment, no near-duplicates inside a cell (word-level edit distance >= 3); the stable picker
//   never offers one of the last 20 titles' templates and keeps >= 85% of its picks when 10% more templates are added; the
//   C3 rubric (a cell's median filled length 40-70, a Cut's cell exempt; at most 1 in 5 in capitals outside Loud); the
//   worked titles (DESIGN §4.9.2) reproduce from their fixtures.
//   Thumbnails: 500 generated from fixture moments give >= 300 distinct visual signatures (no caption text), every P0
//   archetype reached, none above 8%, every kind group's deals show >= 5 archetypes over 20 shuffles, >= 4 archetypes a deal,
//   at most one best-fit archetype a deal, captions only from facts, storytime with no face or name but the player's, no
//   unshown card, no consecutive (archetype, palette) repeat, FrameOf covers all 36, TraitMean pinned (each group's mean
//   trait product 1.000 +- 0.02), the worked thumbnails (DESIGN §4.9.1) reachable from their fixtures.
//   Ideas: every one of the 148 P0 ideas (§6.3's 147 and Road to the Final Table) ready on fixture state with its facts, a
//   title and an archetype; every locked idea says why; a 180-day pacing creator unlocks >= 80 distinct ideas.
//   Comments: 10,000 through the lint, U1, storytime safety, facts-only numbers and the rude cap. World creators: their
//   videos tell only fixture-world facts.
// Modes: default (<= 20 s); "titles" prints every cell with one filled example per template (the reviewable table);
// "--write" prints the pinned constants (TitleStyleNorm, TraitMean) as measured.
#include "CopyLint.h"
#include "Trademarks.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/VideoContentPriv.h"
#include "../../Source/ShortStackCore/Private/ShortStack/Game/VideoAlgoPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace rerun_content_test
{
namespace video = ss::video;
namespace vc = ss::video::vcontent;
using ss::Card;
using ss::Chips;
constexpr double Day = 1440.0;

int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const std::string& What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

std::string Lower(std::string S)
{
	for (char& C : S)
	{
		C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
	}
	return S;
}

/** No "AI", "bot" or "autoplay" in the world's words (U1), as whole words. */
bool SaysAutoplay(const std::string& Text)
{
	const std::string Low = Lower(Text);
	for (const char* W : {"ai", "bot", "bots", "autoplay", "autopilot"})
	{
		if (trademarks::HasWord(Low, W))
		{
			return true;
		}
	}
	return false;
}

const char* Brand(const std::string& Text)
{
	// A money figure is a fact, not a name: "$1,888.10" and "1,888" never read as the brand "888".
	std::string T = Text;
	for (size_t I = 0; I < T.size(); ++I)
	{
		const bool Money = T[I] == '$';
		size_t J = I + (Money ? 1 : 0);
		bool Grouped = false;
		while (J < T.size() && (std::isdigit(static_cast<unsigned char>(T[J])) || ((T[J] == ',' || T[J] == '.') && J + 1 < T.size() && std::isdigit(static_cast<unsigned char>(T[J + 1])))))
		{
			Grouped = Grouped || T[J] == ',';
			++J;
		}
		if (J > I + (Money ? 1 : 0) && (Money || Grouped))
		{
			for (size_t K = I; K < J; ++K)
			{
				T[K] = std::isdigit(static_cast<unsigned char>(T[K])) ? '0' : T[K];
			}
		}
		I = J > I ? J - 1 : I;
	}
	const char* Hit = trademarks::Find(T, trademarks::kSourceScan);
	return Hit ? Hit : trademarks::Find(T, trademarks::kCopyScan);
}

/** The digit runs of a text ("$1,190" has "1" and "190"). */
std::vector<std::string> DigitRuns(const std::string& T)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char C : T)
	{
		if (C >= '0' && C <= '9')
		{
			Cur.push_back(C);
		}
		else if (!Cur.empty())
		{
			Out.push_back(Cur);
			Cur.clear();
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

/** Every digit run of a filled text is a digit run of one of the facts it was filled from. */
bool DigitsAreFacts(const std::string& Filled, const vc::Facts& F, std::string& Bad)
{
	std::set<std::string> Known;
	for (const auto& Kv : F.V)
	{
		for (const std::string& R : DigitRuns(Kv.second))
		{
			Known.insert(R);
		}
	}
	for (const std::string& R : DigitRuns(Filled))
	{
		if (!Known.count(R))
		{
			Bad = R;
			return false;
		}
	}
	return true;
}

/** Word-level edit distance (slots are words). */
int WordDistance(const std::string& A, const std::string& B)
{
	auto Split = [](const std::string& S) {
		std::vector<std::string> W;
		std::string Cur;
		for (const char C : S)
		{
			if (C == ' ')
			{
				if (!Cur.empty())
				{
					W.push_back(Lower(Cur));
				}
				Cur.clear();
			}
			else if (std::isalnum(static_cast<unsigned char>(C)) || C == '{' || C == '}' || C == '\'' || C == '$' || C == '%')
			{
				Cur.push_back(C);
			}
		}
		if (!Cur.empty())
		{
			W.push_back(Lower(Cur));
		}
		return W;
	};
	const std::vector<std::string> X = Split(A), Y = Split(B);
	std::vector<std::vector<int>> D(X.size() + 1, std::vector<int>(Y.size() + 1, 0));
	for (size_t I = 0; I <= X.size(); ++I)
	{
		D[I][0] = static_cast<int>(I);
	}
	for (size_t J = 0; J <= Y.size(); ++J)
	{
		D[0][J] = static_cast<int>(J);
	}
	for (size_t I = 1; I <= X.size(); ++I)
	{
		for (size_t J = 1; J <= Y.size(); ++J)
		{
			D[I][J] = std::min({D[I - 1][J] + 1, D[I][J - 1] + 1, D[I - 1][J - 1] + (X[I - 1] == Y[J - 1] ? 0 : 1)});
		}
	}
	return D[X.size()][Y.size()];
}

// ================================================================== the fixtures

/** A hand as the player saw it: their cards, a hand turned up (or not), the board, an all in at Eq, the net. */
video::Candidate HandOf(const std::string& Id, video::Kind K, const char* Mine, const char* Theirs, const char* Board, Chips Net, double Eq, double At)
{
	video::Candidate C;
	C.Id = Id;
	C.At = At;
	C.K = K;
	C.Source = 0;
	C.Event = "Night Owl Turbo";
	C.BuyIn = 550;
	C.Field = 520;
	C.PlaceSoFar = 41;
	ss::recall::Record& R = C.Rec;
	R.Number = 214;
	R.Level = 9;
	R.Blinds.Sb = 100;
	R.Blinds.Bb = 200;
	R.Mine = ss::ParseCards(Mine);
	R.Board = ss::ParseCards(Board);
	if (Theirs && Theirs[0])
	{
		R.Shown.push_back({"Mike_R", ss::ParseCards(Theirs)});
	}
	C.Opponent = "Mike_R";
	R.Lines.push_back({ss::Street::Preflop, "You", "raises to", 600});
	R.Lines.push_back({ss::Street::Flop, "You", "bets", 1800});
	R.Lines.push_back({ss::Street::River, "You", "all in", 6400});
	R.Pot = 12800;
	R.HeroNet = Net;
	C.EquityAtAllIn = Eq;
	C.Drama = 0.62;
	C.Expires = At + 21.0 * Day;
	return C;
}

video::Candidate RunOf(const std::string& Id, video::Kind K, int Place, Chips Prize, double At)
{
	video::Candidate C;
	C.Id = Id;
	C.At = At;
	C.K = K;
	C.Event = "Sunday Blaze";
	C.BuyIn = 1100;
	C.Field = 1530;
	C.PlaceSoFar = Place;
	C.Rec.Pot = Prize;
	C.Rec.HeroNet = Prize - 1100;
	C.Final = Place <= 9;
	C.Drama = 0.8;
	C.Expires = At + 21.0 * Day;
	return C;
}

/** Every variant of a kind's moment a title template might need: lost with a hand shown, won, unshown, the bubble, storytime,
 *  live, the rival. */
std::vector<video::Candidate> Variants(video::Kind K, double At)
{
	using video::Kind;
	std::vector<video::Candidate> Out;
	const std::string P = "rl:night-owl@" + std::to_string(static_cast<long long>(At)) + ":";
	switch (K)
	{
	case Kind::DeepRun: Out.push_back(RunOf(P + "recap", K, 15, 4200, At)); break;
	case Kind::FinalTable: Out.push_back(RunOf(P + "recap", K, 4, 119000, At)); break;
	case Kind::Win: Out.push_back(RunOf(P + "recap", K, 1, 148600, At)); break;
	case Kind::Story:
	{
		video::Candidate S;
		S.Id = "story:rent-paid";
		S.K = Kind::Story;
		S.Event = "rent-paid";
		S.Source = 4;
		S.At = At;
		Out.push_back(S);
		break;
	}
	default:
	{
		const bool Showdown = K != Kind::Bluff && K != Kind::HeroFold;
		video::Candidate Lost = HandOf(P + "1", K, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, At);
		video::Candidate Won = HandOf(P + "2", K, "AcJd", "QhQc", "8d3cJs5hAh", 6500, 0.30, At);
		video::Candidate Quiet = HandOf(P + "3", K, "6s5s", "", "KhQd2cJs8h", 3000, -1.0, At);
		if (K == Kind::Monster)
		{
			Lost = HandOf(P + "1", K, "9s9h", "AcKd", "9c9d2hKs4c", 9000, -1.0, At);
			Won = Lost;
		}
		if (K == Kind::SetOverSet)
		{
			Lost = HandOf(P + "1", K, "8s8h", "9c9d", "9s8d2c5hKc", -6400, -1.0, At);
			Won = HandOf(P + "2", K, "9c9d", "8s8h", "9s8d2c5hKc", 6400, -1.0, At);
		}
		if (K == Kind::RunnerRunner)
		{
			Won = HandOf(P + "2", K, "7s6s", "AcAd", "AhTs2c8h9d", 6400, 0.04, At);
			Lost = HandOf(P + "1", K, "AcAd", "7s6s", "Ah9s2c8h5d", -6400, 0.96, At);
		}
		if (K == Kind::HeroFold || K == Kind::LadderFold)
		{
			Quiet = HandOf(P + "3", K, "KhKd", "", "Ks9c4h", -1100, -1.0, At);
			Quiet.Rec.Lines.back() = {ss::Street::Flop, "You", "folds", 0};
		}
		if (Showdown)
		{
			Out.push_back(Lost);
			Out.push_back(Won);
		}
		Out.push_back(Quiet);
		video::Candidate Bubble = Lost;
		Bubble.Id = P + "4";
		Bubble.Bubble = true;
		Out.push_back(Bubble);
		video::Candidate Dees = Lost;
		Dees.Id = "dee:30:" + std::to_string(500 + static_cast<int>(K));
		Dees.Source = 2;
		Dees.Storytime = true;
		Dees.Event = "a back-room game";
		Dees.BuyIn = 4000;
		Dees.Opponent.clear();
		Dees.Rec.Shown[0].first = "Seat 1";
		Out.push_back(Dees);
		video::Candidate Live = Won;
		Live.Id = "lv:embercrest-nightly@30:" + std::to_string(static_cast<int>(K));
		Live.Source = 1;
		Live.Event = "Embercrest Nightly";
		Live.BuyIn = 12000;
		Out.push_back(Live);
		video::Candidate Rival = Lost;
		Rival.Id = P + "5";
		Rival.Opponent = ss::RivalName;
		Rival.Rival = true;
		Rival.Rec.Shown[0].first = ss::RivalName;
		Out.push_back(Rival);
		video::Candidate Rival2 = Won;
		Rival2.Id = P + "6";
		Rival2.Opponent = ss::RivalName;
		Rival2.Rival = true;
		Rival2.Rec.Shown[0].first = ss::RivalName;
		Out.push_back(Rival2);
		// Update 3's context kinds carry their facts as an idea-tagged moment does.
		for (video::Candidate& X : Out)
		{
			if (K == Kind::LadderFold)
			{
				X.IdeaFacts = "Left=9";
			}
			if (K == Kind::MinCash)
			{
				X.IdeaFacts = "StackBb=4;Prize=$9.70";
				X.PlaceSoFar = 72;
			}
			if (K == Kind::Envelope)
			{
				X.IdeaFacts = "Mult=25;Bounty=$27.50";
			}
			if (K == Kind::HeadsUp || K == Kind::LastHand)
			{
				X.PlaceSoFar = K == Kind::HeadsUp ? 1 : 41;
				X.IdeaFacts = K == Kind::HeadsUp ? ((&X - &Out[0]) % 2 == 0 ? "If=X" : "") : "";
				X.PlaceSoFar = K == Kind::HeadsUp ? ((&X - &Out[0]) % 2 == 0 ? 1 : 2) : 41;
			}
			if (K == Kind::StreamMoment)
			{
				X.Stream.Viewers = 312;
				X.Stream.Face = 5;
				X.Stream.Chat = {"NO WAY", "the river!!"};
			}
		}
		break;
	}
	}
	return Out;
}

/** A Vlog's or a Recap's picks (one night, one event). */
std::vector<video::Candidate> MultiPicks(bool Recap, double At)
{
	std::vector<video::Candidate> Out;
	const std::string P = "lv:embercrest-sunday@40:";
	Out.push_back(RunOf(P + "recap", Recap ? video::Kind::FinalTable : video::Kind::DeepRun, 4, 119000, At));
	Out.back().Event = "Embercrest Nightly";
	Out.back().Source = 1;
	Out.back().Field = 88;
	for (int I = 0; I < 3; ++I)
	{
		video::Candidate H = HandOf(P + std::to_string(I), I == 0 ? video::Kind::BadBeat : video::Kind::BigPot, "AsAd", "7c7h", "Kd8s2c9h7d", I == 0 ? -6400 : 6400, 0.81, At + I);
		H.Event = "Embercrest Nightly";
		H.Source = 1;
		H.BuyIn = 12000;
		H.Field = 88;
		Out.push_back(H);
	}
	return Out;
}

/** The idea fixture: a career at day 441 (Monday, December 20, 2027, 8 PM) where every idea but three is ready (Full), and
 *  one where those three are (Hard: a downswing, rent due and short, the locks changed). */
struct Fixture
{
	video::IdeaCtx X;
	video::Channel C;
};

video::IdeaResult Res(const char* Name, const char* Id, double At, int Site, int Place, int Field, Chips BuyIn, Chips Prize, uint32_t Flags, int Hands = 0)
{
	video::IdeaResult R;
	R.Name = Name;
	R.Id = Id;
	R.At = At;
	R.Site = Site;
	R.Place = Place;
	R.Field = Field;
	R.BuyIn = BuyIn;
	R.Prize = Prize;
	R.Flags = Flags;
	R.Hands = Hands;
	return R;
}

Fixture FullFixture(bool Hard)
{
	using namespace ss::video;
	Fixture Fx;
	IdeaCtx& X = Fx.X;
	Channel& C = Fx.C;
	const double Now = 441.0 * Day + 20.0 * 60.0;
	X.World = Now;
	X.Hero = "grinder_3c";
	const double Sun = 440.0 * Day; // Sunday, December 19
	const int RL = 0, IG = 1, RIT = 2;
	// Sunday night: the Sunday Blaze, ten hypers, two Omaha, three bounty events, two series events.
	X.Results.push_back(Res("Sunday Blaze", "sb@1200", Sun + 20.0 * 60.0, RL, 4, 1530, 1100, 119000, IfMajor | IfFinal, 6));
	for (int I = 0; I < 10; ++I)
	{
		X.Results.push_back(Res("Hyper Turbo", ("ht@" + std::to_string(1260 + I)).c_str(), Sun + 21.0 * 60.0 + I, RL, 40 + I, 180, 220, I == 3 ? 900 : 0, IfHyper));
	}
	X.Results.push_back(Res("Omaha Circuit", "oc@1300", Sun + 21.5 * 60.0, RL, 12, 90, 550, 0, IfOmaha));
	X.Results.push_back(Res("Omaha Circuit Turbo", "oct@1310", Sun + 21.6 * 60.0, RL, 3, 90, 550, 2400, IfOmaha));
	X.Results.push_back(Res("Bounty Builder Night", "bb@1320", Sun + 22.0 * 60.0, RL, 20, 300, 550, 780, IfBounty));
	X.Results.push_back(Res("PKO Turbo", "pko@1330", Sun + 22.1 * 60.0, RL, 70, 300, 550, 0, IfBounty));
	X.Results.push_back(Res("PKO Deep", "pkod@1340", Sun + 22.2 * 60.0, RL, 31, 300, 550, 420, IfBounty));
	for (int I = 0; I < 8; ++I)
	{
		X.Results.push_back(Res("PKO Nightly", ("pkon@" + std::to_string(900 + I)).c_str(), 420.0 * Day + static_cast<double>(I) * Day, RL, 60 + I, 300, 550, 0, IfBounty));
	}
	X.Results.push_back(Res("Micro Madness #12", "mm-12@1350", Sun + 18.0 * 60.0, RL, 88, 900, 330, 0, IfSeries));
	X.Results.push_back(Res("Micro Madness #13", "mm-13@1360", Sun + 19.0 * 60.0, RL, 1, 900, 330, 51000, IfSeries | IfFinal, 9));
	// The week before: a satellite seat, a freeroll cash, a deep run, a bubble, a final table, an overlay on Ignite, a shot,
	// The Spark, Flip & Runs, mystery bounties and the Sunday 100K.
	const double Wk = 436.0 * Day;
	X.Results.push_back(Res("RCOP Main Mega Satellite", "sat@1000", Wk + 20.0 * 60.0, RL, 3, 60, 220, 5500, IfSatellite | IfSeatWon));
	X.Results.push_back(Res("Night Freeroll", "fr@1010", Wk + 21.0 * 60.0, RL, 6, 800, 0, 250, IfFreeroll));
	X.Results.push_back(Res("Night Owl Turbo", "no@1020", Wk + 22.0 * 60.0, RL, 9, 520, 550, 4200, IfFinal | IfTurbo, 5));
	X.Results.push_back(Res("Midnight Deepstack", "md@1030", Wk + 23.0 * 60.0, RL, 16, 100, 550, 0, 0));
	X.Results.push_back(Res("Ignite Nightly", "ignite.in@1040", Wk + 20.5 * 60.0, IG, 7, 140, 1100, 3300, IfOverlay));
	X.Results.push_back(Res("Night Owl 55", "no55@1050", Wk + 19.0 * 60.0, RL, 60, 300, 5500, 0, 0));
	X.Results.push_back(Res("The Spark", "spark@1060", Wk + 21.0 * 60.0 - 1.0 * Day, RL, 22, 400, 1100, 2900, IfSpark));
	for (int I = 0; I < 10; ++I)
	{
		X.Results.push_back(Res("Flip & Run", ("ignite.fr@" + std::to_string(1070 + I)).c_str(), Wk + 18.0 * 60.0 + I, IG, 2, 3, 220, I % 3 == 0 ? 660 : 0, IfFlipRun));
	}
	for (int I = 0; I < 5; ++I)
	{
		X.Results.push_back(Res("Mystery Hunt", ("mh@" + std::to_string(1090 + I)).c_str(), Wk - 2.0 * Day + I, RL, 50 + I, 200, 550, 0, IfMystery));
	}
	X.Results.push_back(Res("Sunday 100K", "rit.s100k@1100", Sun - 7.0 * Day + 19.0 * 60.0, RIT, 120, 900, 2200, 0, IfMajor));
	// Live: the Embercrest Sunday, a Nightly, a Midnight Turbo cash (the night ended late), and five more over the months.
	X.Results.push_back(Res("Embercrest Sunday", "lv:embercrest-sunday@440", Sun + 13.0 * 60.0, RL, 9, 88, 12000, 24000, IfLive | IfFinal));
	X.Results.push_back(Res("Midnight Turbo", "lv:midnight@439", 439.0 * Day + 24.0 * 60.0 - 10.0, RL, 3, 40, 6000, 31000, IfLive | IfMidnight));
	X.Results.push_back(Res("Embercrest Nightly", "lv:nightly@437", 437.0 * Day + 19.0 * 60.0, RL, 15, 60, 12000, 0, IfLive));
	X.Results.push_back(Res("Embercrest Bounty", "lv:bounty@300", 300.0 * Day, RL, 11, 50, 8000, 0, IfLive));
	X.Results.push_back(Res("Saturday Big Stack", "lv:bigstack@200", 200.0 * Day, RL, 7, 70, 15000, 21000, IfLive));
	// The 50 before them at the micros (E9's move up) and fifty more spread over the year.
	for (int I = 0; I < 60; ++I)
	{
		X.Results.push_back(Res(I % 2 ? "Night Owl Turbo" : "Daily Deep", ("old@" + std::to_string(I)).c_str(), 380.0 * Day - static_cast<double>(I) * Day * 3.0, I % 7 == 0 ? IG : RL,
			100 + I, 300, 330, I % 9 == 0 ? 1200 : 0, I % 4 == 0 ? IfHyper : 0));
	}
	// December's: 25 events this month so far (E1), the newest 50 at $11 (the move up).
	for (int I = 0; I < 6; ++I)
	{
		X.Results.push_back(Res("Daily $11", ("d11@" + std::to_string(I)).c_str(), 432.0 * Day + static_cast<double>(I) * Day + 20.0 * 60.0, RL, 30 + I, 400, 1100, I == 2 ? 3300 : 0, 0));
	}
	std::sort(X.Results.begin(), X.Results.end(), [](const IdeaResult& A, const IdeaResult& B) { return A.At > B.At; });
	if (Hard)
	{
		// A downswing: the last 30 days' cashes gone.
		for (IdeaResult& R : X.Results)
		{
			R.Prize = R.At > Now - 30.0 * Day ? 0 : R.Prize;
		}
	}
	X.Events = 900;
	X.Cashes = 130;
	X.FinalTables = 14;
	X.Wins = 3;
	X.BuyIns = 900 * 500;
	X.Prizes = 900 * 540;
	X.BestPrize = 119000;
	X.LiveEvents = 8;
	X.FirstLiveAt = 60.0 * Day;
	X.StudyWeek = 1;
	X.DeesNights = 4;
	// The coach's grades: a Best and a Mistake, river decisions this week, short-stack shoves.
	IdeaGrade Best;
	Best.At = Now - 2.0 * Day;
	Best.Grade = 0;
	Best.Event = "Night Owl Turbo";
	Best.Hand = "ace-queen";
	Best.Street = "turn";
	Best.PotBb = 24;
	Best.River = true;
	X.Grades.push_back(Best);
	IdeaGrade Miss = Best;
	Miss.Grade = 3;
	Miss.Street = "turn";
	Miss.CostBb = 38;
	X.Grades.push_back(Miss);
	for (int I = 0; I < 12; ++I)
	{
		IdeaGrade G;
		G.At = Now - static_cast<double>(I % 6) * Day;
		G.Grade = I % 4 == 3 ? 1 : 3;
		G.River = I < 4;
		G.Shove = true;
		G.StackBb = 12;
		G.Hand = "king-ten";
		G.Street = "preflop";
		X.Grades.push_back(G);
	}
	// Money and life.
	X.Bankroll = Hard ? 42000 : 186000;
	X.RentDue = 107500;
	X.RentDeadline = Hard ? Now + 2.0 * Day : Now + 20.0 * Day;
	X.RentStage = Hard ? 0 : 1;
	X.RentsPaid = 14;
	X.Evictions = 1;
	X.Evicted = Hard;
	X.LastRentAt = Now - 6.0 * Day;
	X.Shifts = 40;
	X.NightShifts14 = 3;
	X.WashFold = 2;
	X.LastShiftAt = Now - 10.0 * 60.0;
	X.Stories = {"rent-paid", "bracelet-first", "back:1"};
	X.Gear = {{"Lumen 4K", 40700}, {"Second monitor", 25000}, {"Desk mic", 12900}, {"Key light", 8900}, {"Ergo chair", 52300}};
	X.GearBought7 = 40700;
	X.NewestGear = "Lumen 4K";
	X.Webcam = 2;
	X.WebcamWas = 1;
	X.Home = 1;
	// Kast.
	X.KastFollowers = 820;
	X.KastStreams = 120;
	X.RaidsIn = 6;
	IdeaStream S;
	S.Start = Sun + 19.0 * 60.0;
	S.Minutes = 300.0;
	S.Avg = 140;
	S.Peak = 412;
	S.Predictions = 4;
	S.Raids = 1;
	X.Streams.push_back(S);
	X.Comments30 = 260;
	X.SubsWeekAgo = 980;
	// The world, a day old and more.
	auto World = [&X](double At, ss::world::EventKind K, const char* Who, const char* Other, const char* What, Chips Amount, int Site) {
		IdeaWorldEvent E;
		E.At = At;
		E.Kind = static_cast<int>(K);
		E.Who = Who;
		E.Other = Other;
		E.What = What;
		E.Amount = Amount;
		E.Site = Site;
		X.WorldLog.push_back(E);
	};
	World(Now - 1.5 * Day, ss::world::EventKind::Champion, "Tomas Ekdahl", "", "Grand Circuit Main", 120000000, 0);
	World(Now - 2.0 * Day, ss::world::EventKind::FinalTable, "Ana Varga", "", "Grand Circuit Main", 0, 0);
	World(Now - 3.0 * Day, ss::world::EventKind::Won, "Ana Varga", "", "Night Owl Turbo", 50000, 0);
	World(Now - 5.0 * Day, ss::world::EventKind::BoardReport, "K.Lindqvist", "", "Season", 0, 0);
	World(Now - 6.0 * Day, ss::world::EventKind::SiteOverlay, "", "", "Ignite Nightly", 21000, 1);
	World(Now - 9.0 * Day, ss::world::EventKind::Rivalry, "Ana Varga", "Mo Reyes", "", 0, 0);
	World(Now - 10.0 * Day, ss::world::EventKind::SiteLaunched, "", "", "", 0, 2);
	World(Now - 12.0 * Day, ss::world::EventKind::HallOfFame, "Lena Moss", "", "Hall of Fame", 0, 0);
	X.SeriesNow = "Micro Madness";
	X.SeriesNext = "Winter Classic";
	X.SeriesNextAt = Now + 2.0 * Day;
	X.SeasonRank = 41;
	X.Joined = {RL, IG, RIT};
	X.FirstSession = {{IG, Now - 3.0 * Day}, {RIT, Now - 40.0 * Day}};
	X.WelcomePending = true;
	X.RewardsTier = 2;
	X.FuseBest = 25;
	X.FuseBestBuyIn = 2750;
	X.FuseBestPrize = 68750;
	X.Flame = 31;
	X.BeefPaid = true;
	X.Beef = "NeonNate";
	X.BeefKo = 3;
	X.Headliner = "LilaVance";
	X.SpecialGuest = true;
	X.Division = "Gold";
	X.DivisionWas = "Steel";
	X.DeadlineDay = true;
	X.CalledOut = "K.Lindqvist";
	X.TeamScalps = true;
	X.ClubTier = 2;
	X.TeamSigned = "Team Ignite";
	// The channel: Partner a week ago, paid, 1,050 subscribers, sixty kept videos (ten Cuts), the first a year and a half ago.
	C.Subs = 1050;
	C.PartnerAt = Now - 7.0 * Day;
	C.PaidCents = 14430;
	C.HoursByDay.fill(12.0f); // 4,392 watch hours over the year (a Partner)
	C.X.On = true;
	C.X.Since = 30;
	C.X.UploadsEver = 64;
	for (int I = 0; I < 60; ++I)
	{
		Video V;
		V.Id = "v" + std::to_string(I);
		V.At = Now - static_cast<double>(60 - I) * 2.0 * Day;
		V.Title = "a video";
		V.K = I % 9 == 0 ? Kind::Rival : Kind::BadBeat;
		V.F = I % 6 == 0 ? Format::Short : Format::Breakdown;
		V.P = 400.0 + I;
		V.Rpm = V.F == Format::Short ? 0.0 : 1.8 + 0.01 * I;
		C.Videos.push_back(V);
	}
	// MOMENTS: one of every hand kind (and kings against aces, seven-deuce, a back-room hand, a stream moment, a run on stream).
	using K = Kind;
	const double M = Now - 1.0 * Day;
	auto Add = [&C](video::Candidate H) { C.Open.push_back(H); };
	Add(HandOf("rl:no@1:1", K::BadBeat, "KsKh", "Ad9d", "Kd9c2s9h9s", -11600, 0.82, M)); // kings full, then the case nine on the river
	Add(HandOf("rl:no@1:2", K::Suckout, "AcJd", "QhQc", "8d3cJs5hAh", 6500, 0.30, M + 1));
	Add(HandOf("rl:no@1:3", K::Cooler, "KdKs", "AhAc", "Kh7dAs9s4h", -10400, -1.0, M + 2));
	Add(HandOf("rl:no@1:4", K::SetOverSet, "8s8h", "9c9d", "9s8d2c5hKc", -10400, -1.0, M + 3));
	Add(HandOf("rl:no@1:5", K::Bluff, "7c2d", "", "KhQd5cJs8h", 9200, -1.0, M + 4));
	Add(HandOf("rl:no@1:6", K::BluffCaught, "6s5s", "KcTd", "KhQd2cJs8h", -7200, -1.0, M + 5));
	Add(HandOf("rl:no@1:7", K::HeroCall, "Td9d", "AsQs", "Th6c3s2hKc", 8000, -1.0, M + 6));
	video::Candidate Fold = HandOf("rl:no@1:8", K::HeroFold, "KhKd", "", "Ks9c4h", -1100, -1.0, M + 7);
	Fold.Rec.Lines.back() = {ss::Street::Flop, "You", "folds", 0};
	Add(Fold);
	{
		video::Candidate Pot = HandOf("rl:no@1:9", K::BigPot, "JhJc", "8h8d", "Jd8s3c2h9c", 11100, -1.0, M + 8);
		Pot.Rec.Pot = 25600; // 128 big blinds (DESIGN §4.9.1 #7)
		Add(Pot);
	}
	Add(HandOf("rl:no@1:10", K::Knockout, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, -1.0, M + 9));
	Add(HandOf("rl:no@1:11", K::DoubleKo, "AsAd", "7c7h", "Kd8s2c9h4d", 13000, -1.0, M + 10));
	video::Candidate Bubble = HandOf("rl:no@1:12", K::Bubble, "AsKd", "QcQh", "Ad7s2c9h4d", 6500, -1.0, M + 11);
	Bubble.Bubble = true;
	Add(Bubble);
	Add(HandOf("rl:no@1:13", K::Monster, "9s9h", "AcKd", "9c9d2hKs4c", 9000, -1.0, M + 12));
	video::Candidate Env = HandOf("rl:mh@1:14", K::Envelope, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, -1.0, M + 13);
	Env.IdeaFacts = "Mult=25;Bounty=$27.50";
	Add(Env);
	video::Candidate Rival = HandOf("rl:no@1:15", K::Rival, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, -1.0, M + 14);
	Rival.Opponent = ss::RivalName;
	Rival.Rival = true;
	Rival.Rec.Shown[0].first = ss::RivalName;
	Add(Rival);
	video::Candidate Rival2 = Rival;
	Rival2.Id = "rl:no@1:16";
	Add(Rival2);
	video::Candidate Rival3 = Rival;
	Rival3.Id = "rl:no@1:17";
	Add(Rival3);
	video::Candidate Clip = HandOf("rl:no@1:18", K::StreamMoment, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, M + 15);
	Clip.Stream.Viewers = 312;
	Clip.Stream.Face = 5;
	Clip.Stream.Chat = {"NO WAY", "the river!!"};
	Clip.Stream.ClipAt = M + 15;
	Add(Clip);
	Add(HandOf("rl:no@1:19", K::RunnerRunner, "7s6s", "AcAd", "AhTs2c8h9d", 6400, 0.04, M + 16));
	video::Candidate Ladder = Fold;
	Ladder.Id = "rl:no@1:20";
	Ladder.K = K::LadderFold;
	Ladder.IdeaFacts = "Left=9";
	Add(Ladder);
	video::Candidate Min = Bubble;
	Min.Id = "rl:no@1:21";
	Min.K = K::MinCash;
	Min.IdeaFacts = "StackBb=4";
	Add(Min);
	video::Candidate Hu = HandOf("rl:no@1:22", K::HeadsUp, "AsKd", "QcQh", "Ad7s2c9h4d", 6500, -1.0, M + 17);
	Hu.PlaceSoFar = 1;
	Hu.IdeaFacts = "If=X;Place=1st;PlaceWord=first";
	Add(Hu);
	video::Candidate Last = HandOf("rl:no@1:23", K::LastHand, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, M + 18);
	Last.Rec.Level = 1;
	Add(Last);
	video::Candidate Dees = HandOf("dee:439:7", K::BadBeat, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, M + 19);
	Dees.Source = 2;
	Dees.Storytime = true;
	Dees.Event = "a back-room game";
	Dees.Opponent.clear();
	Dees.Rec.Shown[0].first = "Seat 1";
	Add(Dees);
	video::Candidate Live = HandOf("lv:embercrest-sunday@440:3", K::BigPot, "JhJc", "8h8d", "Jd8s3c2h9c", 11100, -1.0, Sun + 14.0 * 60.0);
	Live.Source = 1;
	Live.Event = "Embercrest Sunday";
	Add(Live);
	// Four all ins in the Sunday Blaze (G5), on Sunday night.
	for (int I = 0; I < 4; ++I)
	{
		video::Candidate A = HandOf("rl:sb@1200:" + std::to_string(30 + I), K::BigPot, "AsAd", "7c7h", "Kd8s2c9h4d", 6400, -1.0, Sun + 20.5 * 60.0 + I);
		A.Event = "Sunday Blaze";
		Add(A);
	}
	// The run on stream (Road to the Final Table): the final table and its moments, all live.
	video::Candidate Run = RunOf("rl:sb@1200:recap", K::FinalTable, 4, 119000, Sun + 23.0 * 60.0);
	Run.Stream.Viewers = 290;
	Add(Run);
	(void)Hard;
	X.Chan = &Fx.C;
	return Fx;
}

/** A choice of one candidate in a format and a style. */
video::Choice One(const std::string& Id, video::Format F, video::TitleStyle T)
{
	video::Choice Ch;
	Ch.Candidates = {Id};
	Ch.F = F;
	Ch.T = T;
	return Ch;
}

// ================================================================== titles

/** Every template's cell and the fixture choices that might fill it. */
struct Fill
{
	std::string Text;
	vc::Facts F;
	bool Cut = false;
};

/** The fills of a template from every fixture variant of its cell (empty: it filled from none). */
std::vector<Fill> FillsOf(const vc::Tpl& T, const std::map<int, std::vector<std::pair<video::Choice, std::vector<video::Candidate>>>>& Pools)
{
	std::vector<Fill> Out;
	const auto It = Pools.find(T.For);
	if (It == Pools.end())
	{
		return Out;
	}
	for (const auto& Pc : It->second)
	{
		const std::vector<video::Candidate> P = video::Picked(Pc.first, Pc.second);
		if (P.empty())
		{
			continue;
		}
		video::Channel C;
		const video::Candidate& Lead = vc::LeadOf(Pc.first, P);
		vc::Facts F = vc::FactsOf(Pc.first, P, Lead, &C);
		for (int Variant = 0; Variant < 4; ++Variant)
		{
			// The conditions an idea's facts carry ("If"): try each.
			vc::Facts G = F;
			G.First = G.First || Variant == 1;
			G.EventWon = G.EventWon || Variant == 2;
			G.RentShort = G.RentShort || Variant == 3;
			if (Variant == 2)
			{
				G.Won = true;
				G.Lost = false;
			}
			if (Variant == 3)
			{
				G.Lost = true;
				G.Won = false;
			}
			const std::string Filled = vc::FillU3(T.Text, G);
			if (!Filled.empty())
			{
				Fill X;
				X.Text = vc::StyledU3(Filled, T.T);
				X.F = G;
				X.Cut = Pc.first.F == video::Format::Short;
				Out.push_back(X);
				break;
			}
		}
	}
	return Out;
}

std::map<int, std::vector<std::pair<video::Choice, std::vector<video::Candidate>>>> TitlePools(const Fixture& A, const Fixture& B)
{
	using video::Format;
	using video::Kind;
	std::map<int, std::vector<std::pair<video::Choice, std::vector<video::Candidate>>>> Pools;
	const double At = 50.0 * Day + 21.0 * 60.0;
	for (int K = 0; K < static_cast<int>(Kind::Idea); ++K)
	{
		for (const video::Candidate& V : Variants(static_cast<Kind>(K), At))
		{
			const bool Story = static_cast<Kind>(K) == Kind::Story;
			Pools[Story ? vc::ForStory + 0 : K].push_back({One(V.Id, Format::Breakdown, video::TitleStyle::Story), {V}});
			Pools[vc::ForCut].push_back({One(V.Id, Format::Short, video::TitleStyle::Story), {V}});
			Pools[vc::ForFallback].push_back({One(V.Id, Format::Breakdown, video::TitleStyle::Story), {V}});
		}
	}
	// Story keys: each one's candidate.
	for (size_t I = 0; I < vc::StoryKeys().size(); ++I)
	{
		video::Candidate S;
		S.Id = "story:" + vc::StoryKeys()[I] + (vc::StoryKeys()[I] == "back" ? ":1" : "");
		S.K = Kind::Story;
		S.Event = vc::StoryKeys()[I] == "back" ? "back:1" : vc::StoryKeys()[I];
		S.Source = 4;
		S.At = At;
		Pools[vc::ForStory + static_cast<int>(I)].push_back({One(S.Id, Format::Breakdown, video::TitleStyle::Story), {S}});
	}
	// A night (Vlog) and a run (Recap), live and online.
	for (const bool Recap : {false, true})
	{
		const std::vector<video::Candidate> P = MultiPicks(Recap, At);
		video::Choice Ch;
		Ch.F = Recap ? Format::Recap : Format::Vlog;
		for (const video::Candidate& X : P)
		{
			Ch.Candidates.push_back(X.Id);
		}
		Pools[Recap ? vc::ForRecap : vc::ForVlog].push_back({Ch, P});
		std::vector<video::Candidate> Q = P;
		for (video::Candidate& X : Q)
		{
			X.Source = 0;
			X.Id = "rl:" + X.Id.substr(3);
		}
		video::Choice Cq;
		Cq.F = Ch.F;
		for (const video::Candidate& X : Q)
		{
			Cq.Candidates.push_back(X.Id);
		}
		Pools[Recap ? vc::ForRecap : vc::ForVlog].push_back({Cq, Q});
	}
	// The ideas: each ready idea's candidate on the two fixtures (group A: its moment, tagged).
	for (const Fixture* Fx : {&A, &B})
	{
		for (const vc::IdeaDef& D : vc::IdeaTable())
		{
			video::Candidate N;
			std::string Why;
			if (D.Id / 100 == 1)
			{
				const std::string Src = vc::IdeaMoment(Fx->X, Fx->C, D.Id, Fx->X.World);
				for (const video::Candidate& M : Fx->C.Open)
				{
					if (M.Id == Src)
					{
						N = M;
						N.Idea = D.Id;
						N.IdeaFacts = (N.IdeaFacts.empty() ? std::string() : N.IdeaFacts + ";") + vc::MomentIdeaFacts(D.Id);
					}
				}
				if (N.Id.empty())
				{
					continue;
				}
			}
			else if (!vc::MakeIdeaCandidateOn(Fx->X, Fx->C, D.Id, Fx->X.World, N, Why))
			{
				continue;
			}
			const Format F = (D.Formats & 2) ? Format::Breakdown : (D.Formats & 4) ? Format::Vlog : (D.Formats & 8) ? Format::Recap : Format::Short;
			Pools[vc::ForIdea + D.Id].push_back({One(N.Id, F, video::TitleStyle::Story), {N}});
			Pools[vc::ForFallback].push_back({One(N.Id, F, video::TitleStyle::Story), {N}});
			if (D.Id / 100 == 16)
			{
				video::Choice Ch = One(N.Id, F, video::TitleStyle::Story);
				Pools[vc::ForSeries].push_back({Ch, {N}});
			}
		}
	}
	return Pools;
}

/** The cells a Cut's titles come from (its own family, the Cut-only ideas). */
bool CutCell(int For)
{
	if (For == vc::ForCut)
	{
		return true;
	}
	if (For >= vc::ForIdea)
	{
		const vc::IdeaDef* D = vc::IdeaById(For - vc::ForIdea);
		return D && D->Formats == 1;
	}
	return false;
}

void Titles(const Fixture& A, const Fixture& B, bool Print)
{
	using video::TitleStyle;
	const std::vector<vc::Tpl>& All = vc::Templates3();
	const auto Pools = TitlePools(A, B);
	Expect(All.size() >= 1600, "titles: at least 1,600 templates (" + std::to_string(All.size()) + ")");
	// Cells.
	std::map<std::pair<int, int>, std::vector<const vc::Tpl*>> Cells;
	for (const vc::Tpl& T : All)
	{
		Cells[{T.For, static_cast<int>(T.T)}].push_back(&T);
	}
	int Short = 0;
	std::string ShortList;
	auto Need = [&](int For, int Style, size_t Min) {
		const auto It = Cells.find({For, Style});
		const size_t N = It == Cells.end() ? 0 : It->second.size();
		if (N < Min)
		{
			++Short;
			ShortList += " " + vc::ForSlug(For) + "." + vc::StyleSlug(static_cast<TitleStyle>(Style)) + "=" + std::to_string(N);
		}
	};
	for (int K = 0; K < static_cast<int>(video::Kind::Idea); ++K)
	{
		if (K == static_cast<int>(video::Kind::Story))
		{
			continue;
		}
		for (int S = 0; S < 5; ++S)
		{
			Need(K, S, K < video::KindCountU2 ? 7 : 4);
		}
	}
	for (int S = 0; S < 5; ++S)
	{
		Need(vc::ForVlog, S, 5);
		Need(vc::ForRecap, S, 5);
		Need(vc::ForCut, S, 10);
		for (size_t I = 0; I < vc::StoryKeys().size(); ++I)
		{
			Need(vc::ForStory + static_cast<int>(I), S, 1);
		}
	}
	for (const vc::IdeaDef& D : vc::IdeaTable())
	{
		if (D.Id / 100 == 1 && D.Id != 105 && D.Id != 107)
		{
			continue; // group A: its kind's cells
		}
		for (int S = 0; S < 5; ++S)
		{
			if ((D.Styles >> S) & 1)
			{
				Need(vc::ForIdea + D.Id, S, 3);
			}
		}
	}
	Expect(Short == 0, "titles: every cell at its minimum (" + std::to_string(Short) + " short:" + ShortList + ")");
	// Every template fills, linted, U1, trademarks, digits are facts, the length.
	int Unfilled = 0, Linted = 0, U1 = 0, Brands = 0, Digits = 0, Long = 0;
	std::map<std::pair<int, int>, std::vector<std::string>> Filled;
	for (const vc::Tpl& T : All)
	{
		const std::vector<Fill> Fs = FillsOf(T, Pools);
		if (Fs.empty())
		{
			++Unfilled;
			std::printf("  unfilled: %s \"%s\"\n", T.Key.c_str(), T.Text.c_str());
			continue;
		}
		Filled[{T.For, static_cast<int>(T.T)}].push_back(Fs.front().Text);
		for (const Fill& X : Fs)
		{
			const std::string Why = copylint::Check(X.Text + ".", 101, {});
			if (!Why.empty())
			{
				++Linted;
				std::printf("  lint %s: \"%s\" %s\n", T.Key.c_str(), X.Text.c_str(), Why.c_str());
			}
			if (SaysAutoplay(X.Text))
			{
				++U1;
			}
			if (const char* Hit = Brand(X.Text))
			{
				++Brands;
				std::printf("  trademark '%s': \"%s\"\n", Hit, X.Text.c_str());
			}
			std::string Bad;
			if (!DigitsAreFacts(X.Text, X.F, Bad))
			{
				++Digits;
				std::printf("  digit '%s' isn't a fact: %s \"%s\"\n", Bad.c_str(), T.Key.c_str(), X.Text.c_str());
			}
			if (!X.Cut && X.Text.size() > 100u)
			{
				++Long;
				std::printf("  too long (%zu): %s \"%s\"\n", X.Text.size(), T.Key.c_str(), X.Text.c_str());
			}
		}
		if (CutCell(T.For) && std::none_of(Fs.begin(), Fs.end(), [](const Fill& X) { return X.Text.size() <= 50u; }))
		{
			++Long;
			std::printf("  a Cut template never fits 50: %s \"%s\"\n", T.Key.c_str(), Fs.front().Text.c_str());
		}
	}
	Expect(Unfilled == 0, "titles: every template fills from a fixture moment of its kind or idea (" + std::to_string(Unfilled) + " don't)");
	Expect(Linted == 0, "titles: every filled title passes the copy lint (" + std::to_string(Linted) + ")");
	Expect(U1 == 0, "titles: no title says AI, bot or autoplay");
	Expect(Brands == 0, "titles: the trademark screen passes over every filled title");
	Expect(Digits == 0, "titles: every digit run in a filled title is one of its moment's facts (" + std::to_string(Digits) + ")");
	Expect(Long == 0, "titles: filled <= 100 characters, a Cut's <= 50 (" + std::to_string(Long) + ")");
	// TitleTraits renormalized (SPEC §4.6.2): each style's templates average 1.000 with TitleStyleNorm (pinned by --write).
	{
		double Sum[5] = {}, Raw[5] = {};
		int N[5] = {};
		for (const vc::Tpl& T : All)
		{
			const std::vector<Fill> Fs = FillsOf(T, Pools);
			if (Fs.empty())
			{
				continue;
			}
			const uint16_t Bits = vc::TraitsOf(T, Fs.front().Text);
			const int S = std::clamp(static_cast<int>(T.T), 0, 4);
			Sum[S] += video::valgo::TitleTraitCtr(Bits);
			Raw[S] += video::valgo::TitleTraitCtr(static_cast<uint16_t>(Bits & 0x1FFu));
			++N[S];
		}
		// The decoder's step (core-videoalgo's TitleTraitCtr: 1.000 + (n - 64) x step); a mean past its reach clamps.
		const double Step = video::valgo::TitleTraitCtr(static_cast<uint16_t>(65u << 9)) - 1.0;
		bool Even = true, Clamped = false;
		std::string Means, Pin;
		for (int S = 0; S < 5; ++S)
		{
			const double Mean = N[S] > 0 ? Sum[S] / N[S] : 1.0, RawMean = N[S] > 0 ? Raw[S] / N[S] : 1.0;
			const double Want = 64.0 + (1.0 / RawMean - 1.0) / Step;
			const bool Reach = Want >= 0.5 && Want <= 127.5;
			Clamped = Clamped || !Reach;
			// Within 0.5% where the scale reaches; where it doesn't, the clamp's best (and pinned to this measure).
			const double Best = RawMean * (1.0 + (std::clamp(std::round(Want), 1.0, 127.0) - 64.0) * Step);
			Even = Even && (Reach ? std::fabs(Mean - 1.0) <= 0.005 : std::fabs(Mean - Best) <= 0.001);
			char Buf[64];
			std::snprintf(Buf, sizeof(Buf), " %s:%.4f", vc::StyleSlug(static_cast<TitleStyle>(S)), Mean);
			Means += Buf;
			std::snprintf(Buf, sizeof(Buf), "%s%.4f", S ? ", " : "", RawMean);
			Pin += Buf;
		}
		if (Print || !Even || Clamped)
		{
			std::printf("  TitleStyleMean (pin these in VideoTitles.cpp): {%s}; the styles' renormalized means now%s%s\n", Pin.c_str(), Means.c_str(),
				Clamped ? " (a style past the renormalizer's reach at this step: core-videocontent_to_core-videoalgo_1.diff widens it)" : "");
		}
		Expect(Even, "titles: each style's templates average a trait product of 1.000 within 0.5% (" + Means + ")");
	}
	// Near-duplicates inside a cell (word-level edit distance >= 3; Update 2's own pairs as they always were).
	int Near = 0;
	for (const auto& Kv : Cells)
	{
		for (size_t I = 0; I < Kv.second.size(); ++I)
		{
			for (size_t J = I + 1; J < Kv.second.size(); ++J)
			{
				if (Kv.second[I]->U2 && Kv.second[J]->U2)
				{
					continue;
				}
				if (WordDistance(Kv.second[I]->Text, Kv.second[J]->Text) < 3)
				{
					++Near;
					std::printf("  near-duplicate in %s: \"%s\" / \"%s\"\n", Kv.second[I]->Key.c_str(), Kv.second[I]->Text.c_str(), Kv.second[J]->Text.c_str());
				}
			}
		}
	}
	Expect(Near == 0, "titles: no two templates in a cell within two words of each other (" + std::to_string(Near) + ")");
	// The rubric: a cell's median filled length 40-70 (a Cut's cell exempt), at most 1 in 5 in capitals outside Loud.
	int Median = 0, Caps = 0;
	for (const auto& Kv : Filled)
	{
		std::vector<size_t> L;
		int Upper = 0;
		for (const std::string& T : Kv.second)
		{
			L.push_back(T.size());
			bool AllCaps = true;
			for (const char C : T)
			{
				AllCaps = AllCaps && !(C >= 'a' && C <= 'z');
			}
			Upper += AllCaps ? 1 : 0;
		}
		std::sort(L.begin(), L.end());
		const size_t Mid = L.empty() ? 0 : L[L.size() / 2];
		const bool Cut = CutCell(Kv.first.first);
		if (!Cut && (Mid < 40 || Mid > 70))
		{
			++Median;
			if (Print)
			{
				std::printf("  rubric: %s.%s median %zu\n", vc::ForSlug(Kv.first.first).c_str(), vc::StyleSlug(static_cast<TitleStyle>(Kv.first.second)), Mid);
			}
		}
		if (Kv.first.second != static_cast<int>(TitleStyle::Hype) && Upper * 5 > static_cast<int>(Kv.second.size()))
		{
			++Caps;
		}
	}
	std::printf("  rubric: %d cells with a median outside 40-70 (of %zu), %d with too many capitals\n", Median, Filled.size(), Caps);
	Expect(Caps == 0, "titles: at most 1 in 5 titles in capitals outside Loud, every cell");
	Expect(Median <= static_cast<int>(Filled.size()) / 10, "titles: the C3 rubric's median length 40-70 in 90% of the cells or more (" + std::to_string(Median) + " outside)");
	if (Print)
	{
		for (const auto& Kv : Cells)
		{
			std::printf("== %s.%s (%zu)\n", vc::ForSlug(Kv.first.first).c_str(), vc::StyleSlug(static_cast<TitleStyle>(Kv.first.second)), Kv.second.size());
			for (const vc::Tpl* T : Kv.second)
			{
				const std::vector<Fill> Fs = FillsOf(*T, Pools);
				std::printf("  %-28s p%d %s  ->  %s\n", T->Key.c_str(), T->Promise, T->Text.c_str(), Fs.empty() ? "(no fill)" : Fs.front().Text.c_str());
			}
		}
	}
}

/** The stable picker: never a template of the last 20 titles; >= 85% of picks kept when 10% more templates are added. */
void Picker(const Fixture& A)
{
	using video::Format;
	using video::TitleStyle;
	// The last 20: put every option of the first shuffle into the channel's kept videos' TitleTpl, and look again.
	video::Channel C = A.C;
	const video::Candidate& Beat = C.Open.front();
	const video::Choice Ch = One(Beat.Id, Format::Breakdown, TitleStyle::Story);
	std::set<std::string> Used;
	int Offered = 0;
	for (int S = 0; S < 6; ++S)
	{
		for (const video::TitleOption& O : video::TitleOptions(Ch, C.Open, C, S, 3))
		{
			Used.insert(O.Key);
		}
	}
	int I = 0;
	for (const std::string& K : Used)
	{
		C.Videos[C.Videos.size() - 1 - static_cast<size_t>(I++ % 20)].X.TitleTpl = K;
	}
	std::set<std::string> Recent;
	std::vector<const video::Video*> Kept;
	for (const video::Video& V : C.Videos)
	{
		Kept.push_back(&V);
	}
	std::sort(Kept.begin(), Kept.end(), [](const video::Video* X, const video::Video* Y) { return X->At > Y->At; });
	for (size_t J = 0; J < 20 && J < Kept.size(); ++J)
	{
		if (!Kept[J]->X.TitleTpl.empty())
		{
			Recent.insert(Kept[J]->X.TitleTpl);
		}
	}
	for (int S = 0; S < 40; ++S)
	{
		for (const video::TitleOption& O : video::TitleOptions(Ch, C.Open, C, S, 3))
		{
			Offered += Recent.count(O.Key) > 0 ? 1 : 0;
		}
	}
	Expect(!Recent.empty() && Offered == 0, "picker: never offers one of the last 20 titles' templates (" + std::to_string(Recent.size()) + " fresh, " + std::to_string(Offered) + " offered)");
	// The rendezvous property: the top three of a cell, with 10% of its templates held back as "not yet appended".
	int Kept3 = 0, Total3 = 0;
	const std::vector<vc::Tpl>& All = vc::Templates3();
	std::map<std::pair<int, int>, std::vector<const vc::Tpl*>> Cells;
	for (const vc::Tpl& T : All)
	{
		Cells[{T.For, static_cast<int>(T.T)}].push_back(&T);
	}
	for (const auto& Kv : Cells)
	{
		if (Kv.second.size() < 10)
		{
			continue;
		}
		for (int Key = 0; Key < 20; ++Key)
		{
			const std::string Choice = "1|fixture:" + std::to_string(Key);
			auto Top = [&](bool Before) {
				std::vector<std::pair<uint32_t, std::string>> R;
				for (size_t J = 0; J < Kv.second.size(); ++J)
				{
					// "Before": the last 10% of the cell's rows not appended yet.
					if (Before && J >= Kv.second.size() - Kv.second.size() / 10)
					{
						continue;
					}
					R.push_back({ss::Fnv1a(Choice + "|" + Kv.second[J]->Key + "|0"), Kv.second[J]->Key});
				}
				std::sort(R.rbegin(), R.rend());
				std::set<std::string> Out;
				for (size_t J = 0; J < 3 && J < R.size(); ++J)
				{
					Out.insert(R[J].second);
				}
				return Out;
			};
			const std::set<std::string> Was = Top(true), Now = Top(false);
			for (const std::string& K : Was)
			{
				Kept3 += Now.count(K) > 0 ? 1 : 0;
				++Total3;
			}
		}
	}
	const double Share = Total3 > 0 ? static_cast<double>(Kept3) / static_cast<double>(Total3) : 0.0;
	Expect(Share >= 0.85, "picker: a 10% append keeps " + std::to_string(static_cast<int>(std::lround(Share * 100.0))) + "% of the picks (>= 85%)");
	// Determinism and Count: three options, the same on the same shuffle, different on another.
	const std::vector<video::TitleOption> P0 = video::TitleOptions(Ch, A.C.Open, A.C, 0, 3), P0b = video::TitleOptions(Ch, A.C.Open, A.C, 0, 3),
										  P1 = video::TitleOptions(Ch, A.C.Open, A.C, 1, 3);
	bool Same = P0.size() == 3 && P0b.size() == 3, Moved = false;
	for (size_t J = 0; J < P0.size() && J < P0b.size(); ++J)
	{
		Same = Same && P0[J].Key == P0b[J].Key;
	}
	for (size_t J = 0; J < P0.size() && J < P1.size(); ++J)
	{
		Moved = Moved || P0[J].Key != P1[J].Key;
	}
	Expect(Same && Moved, "picker: three options, stable for a shuffle, new on the next");
	// A thin cell spans styles: a story with one row a style still gets three options.
	video::Candidate Story;
	Story.Id = "story:ring-first";
	Story.K = video::Kind::Story;
	Story.Event = "ring-first";
	Story.Source = 4;
	const std::vector<video::TitleOption> Span = video::TitleOptions(One(Story.Id, Format::Breakdown, TitleStyle::Plain), {Story}, A.C, 0, 3);
	Expect(Span.size() == 3, "picker: a thin cell's options span styles (" + std::to_string(Span.size()) + ")");
	// The renormalizer is in the traits' high bits, every option's.
	bool Norm = true;
	for (const video::TitleOption& O : P0)
	{
		Norm = Norm && (O.Traits >> 9) != 0;
	}
	Expect(Norm, "picker: every option carries its style's renormalizer");
}

/** DESIGN §4.9.2's fifty worked titles, each from its fixture (the rows the engine fills, styled as Studio shows them). */
void WorkedTitles(const Fixture& A)
{
	using video::Format;
	using video::Kind;
	using video::TitleStyle;
	struct Worked
	{
		const char* Title;
		video::Candidate Lead;
		Format F;
		TitleStyle T;
	};
	std::vector<Worked> W;
	const double At = 300.0 * Day + 21.0 * 60.0;
	video::Candidate Kings = HandOf("rl:no@9:214", Kind::BadBeat, "KsKh", "Ad9d", "Kd8s2c9h9s", -11600, 0.82, At);
	Kings.Event = "Night Owl";
	Kings.Rec.Pot = 11600;
	Kings.Rec.Blinds.Bb = 200;
	video::Candidate K58 = Kings;
	K58.Id = "rl:no@9:215";
	K58.Rec.Pot = 11600;
	W.push_back({"kings, 82% to win, and then the nine of hearts", Kings, Format::Breakdown, TitleStyle::Story});
	W.back().Lead.Rec.Board = ss::ParseCards("9c8s2h7c9h");
	W.back().Lead.Idea = 101; // the bad-beat idea's own cell (it near-duplicates Update 2's "{Eq}% to win, and then {Killer}")
	video::Candidate Ace81 = HandOf("rl:no@9:216", Kind::BadBeat, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, At);
	W.push_back({"81% AND THE RIVER DID THIS", Ace81, Format::Breakdown, TitleStyle::Hype});
	W.push_back({"Is 82% ever safe? $5.50 Night Owl, 58 big blinds", K58, Format::Breakdown, TitleStyle::Question});
	W.push_back({"What to do after a 58 big blind bad beat (from someone who just took one)", K58, Format::Breakdown, TitleStyle::Teaching});
	W.push_back({"$5.50 Night Owl, hand 214: KsKh vs Ad9d all in", Kings, Format::Short, TitleStyle::Plain});
	video::Candidate Sets = HandOf("rl:no@9:217", Kind::SetOverSet, "8s8h", "9c9d", "9s8d2c5hKc", -10400, -1.0, At);
	Sets.Rec.Pot = 10400;
	W.push_back({"SET OVER SET FOR 52 BIG BLINDS", Sets, Format::Breakdown, TitleStyle::Hype});
	W.push_back({"Set over set: why nobody folds here", Sets, Format::Breakdown, TitleStyle::Teaching});
	W.push_back({"I made quads and still had to get paid", HandOf("rl:no@9:218", Kind::Monster, "9s9h", "AcKd", "9c9d2hKs4c", 9000, -1.0, At), Format::Breakdown, TitleStyle::Story});
	W.push_back({"RUNNER-RUNNER AT 4%", HandOf("rl:no@9:219", Kind::RunnerRunner, "7s6s", "AcAd", "AhTs2c8h9d", 6400, 0.04, At), Format::Breakdown, TitleStyle::Hype});
	video::Candidate Ladder = HandOf("rl:no@9:220", Kind::LadderFold, "KhKd", "", "Ks9c4h", -1100, -1.0, At);
	Ladder.IdeaFacts = "Left=9";
	W.push_back({"I FOLDED KINGS. ON PURPOSE.", Ladder, Format::Breakdown, TitleStyle::Hype});
	W.push_back({"ICM in one hand: folding kings with 9 left", Ladder, Format::Breakdown, TitleStyle::Teaching});
	video::Candidate Deuce = HandOf("rl:no@9:221", Kind::Bluff, "7c2d", "", "KhQd5cJs8h", 9200, -1.0, At);
	Deuce.Rec.Pot = 9200;
	Deuce.Idea = 107;
	W.push_back({"I bluffed 46 big blinds with seven-deuce and my hands were shaking", Deuce, Format::Breakdown, TitleStyle::Story});
	W.push_back({"The river bluff with seven-deuce, explained street by street", Deuce, Format::Breakdown, TitleStyle::Teaching});
	video::Candidate Call = HandOf("rl:no@9:222", Kind::HeroCall, "Td9d", "AsQs", "Th6c3sKh2c", 8000, -1.0, At);
	Call.Rec.Pot = 8000;
	W.push_back({"40 big blinds on the river with second pair. Do you call?", Call, Format::Breakdown, TitleStyle::Question});
	video::Candidate Env = HandOf("rl:mh@9:223", Kind::Envelope, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, -1.0, At);
	Env.IdeaFacts = "Mult=25;Bounty=$27.50";
	W.push_back({"25X BOUNTY ENVELOPE", Env, Format::Breakdown, TitleStyle::Hype});
	video::Candidate Hu = HandOf("lv:nightly@9:224", Kind::HeadsUp, "AsKd", "QcQh", "Ad7s2c9h4d", 6500, -1.0, At);
	Hu.Event = "Nightly";
	W.push_back({"one flip for the Nightly title", Hu, Format::Breakdown, TitleStyle::Story});
	video::Candidate Deep = RunOf("rl:sb@9:recap", Kind::DeepRun, 4, 119000, At);
	Deep.Event = "Sunday Blaze";
	W.push_back({"4th of 1,530 in the Sunday Blaze: the whole run", Deep, Format::Recap, TitleStyle::Plain});
	video::Candidate Spark = RunOf("rl:spark@9:recap", Kind::Win, 1, 148600, At);
	Spark.Event = "The Spark";
	W.push_back({"HOW I WON $1,486 IN THE SPARK", Spark, Format::Breakdown, TitleStyle::Hype});
	video::Candidate Dees = HandOf("dee:9:7", Kind::BadBeat, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, At);
	Dees.Source = 2;
	Dees.Storytime = true;
	Dees.Event = "a back-room game";
	Dees.BuyIn = 4000;
	Dees.Opponent.clear();
	Dees.Rec.Shown[0].first = "Seat 1";
	W.push_back({"a back-room game, a regular, and the worst river of my life", Dees, Format::Breakdown, TitleStyle::Story});
	auto IdeaLead = [](int Id, const char* Facts) {
		video::Candidate C;
		C.Id = "idea:w:" + std::to_string(Id);
		C.K = Kind::Idea;
		C.Source = 5;
		C.Idea = Id;
		C.IdeaFacts = Facts;
		return C;
	};
	W.push_back({"my October in poker: 214 tournaments, up $412", IdeaLead(501, "Month=October;Events=214;NetSigned=up+$412;If=U"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"NOVEMBER WAS A DISASTER", IdeaLead(501, "Month=November;Events=180;NetSigned=down+$96;If=M"), Format::Breakdown, TitleStyle::Hype});
	W.push_back({"Is 9% ROI good at $5.50?", IdeaLead(501, "Month=October;Roi=9%25;Abi=$5.50"), Format::Breakdown, TitleStyle::Question});
	W.push_back({"22 buy-ins down. this is what a downswing feels like", IdeaLead(503, "Down=22;Days=30"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"rent is due Friday and I'm $214 short", IdeaLead(506, "Weekday=Friday;Short=$214;Days=3;Rent=$1,075;If=R"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"MY FIRST +$1,060 MONTH", IdeaLead(501, "Month=October;NetPlus=%2B$1,060;If=FU"), Format::Breakdown, TitleStyle::Hype});
	W.push_back({"$50 to $500 on Ignite Poker, day 4: $86", IdeaLead(1601, "Start=$50;Goal=$500;Site=Ignite+Poker;Day=4;Now=$86;Ep=2;Series=$50+to+$500+on+Ignite+Poker"),
		Format::Breakdown, TitleStyle::Plain});
	W.push_back({"I did it: $50 to $500 in 23 days", IdeaLead(1601, "Start=$50;Goal=$500;Site=Ignite+Poker;Day=23;Now=$512;Ep=9;If=X"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"The turn mistake that cost me 38 big blinds", IdeaLead(602, "Street=turn;CostBb=38;Hand=ace-queen"), Format::Breakdown, TitleStyle::Teaching});
	W.push_back({"Push or fold with 12 big blinds: the chart says push", IdeaLead(604, "StackBb=12;ChartSays=push;Events=24"), Format::Breakdown, TitleStyle::Teaching});
	W.push_back({"Quiz: 5 river spots from my week. Call or fold?", IdeaLead(608, "Count=5"), Format::Breakdown, TitleStyle::Question});
	W.push_back({"Every RiverLine tournament I play, ranked", IdeaLead(708, "Count=12"), Format::Breakdown, TitleStyle::Plain});
	W.push_back({"my 4 worst beats of 2027", IdeaLead(702, "Count=4;WorstOf=beats;Year=2027;What=bad+beats;Month=November"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"Tomas Ekdahl just won the Grand Circuit Main. Let's talk about it", IdeaLead(801, "Winner=Tomas+Ekdahl;Major=Grand+Circuit+Main"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"Is Ignite Poker as soft as everyone says?", IdeaLead(613, "Site=Ignite+Poker;Events=60"), Format::Breakdown, TitleStyle::Question});
	W.push_back({"What RunItTwice pays back: the rakeback, explained", IdeaLead(1304, "Site=RunItTwice"), Format::Breakdown, TitleStyle::Teaching});
	W.push_back({"I LIT A 25X FUSE", IdeaLead(1401, "Mult=25;BuyIn=$27.50;Prize=$687.50"), Format::Breakdown, TitleStyle::Hype});
	W.push_back({"three players, one fuse, $27.50", IdeaLead(1401, "Mult=25;BuyIn=$27.50"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"they busted me three times. tonight I got them back", IdeaLead(1402, "Beef=NeonNate;BeefTimes=three+times"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"I knocked out LilaVance on the Bandstand", IdeaLead(1403, "Headliner=LilaVance"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"promoted to Gold: my RunItTwice League month", IdeaLead(1406, "Division=Gold;DivisionWas=Steel"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"K.Lindqvist called me out. Who won the week?", IdeaLead(1408, "Callout=K.Lindqvist"), Format::Breakdown, TitleStyle::Question});
	W.push_back({"ALL SIX TEAM PROS: SCALPED", IdeaLead(1409, ""), Format::Breakdown, TitleStyle::Hype});
	W.push_back({"Poker setup tour: 2 monitors, the Lumen 4K", IdeaLead(1105, "Monitors=2+monitors;Gear=Lumen+4K;Items=5;GearCost=$1,407"), Format::Breakdown, TitleStyle::Plain});
	W.push_back({"a day in my life: a night cashier shift, then 6 tournaments", IdeaLead(1101, "Job=night+cashier;Events=6"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"1,000 subscribers. thank you (and a story)", IdeaLead(901, "Subs=1,000"), Format::Breakdown, TitleStyle::Story});
	W.push_back({"gh0stfold again. every pot we've played", IdeaLead(1201, "Opp=gh0stfold;Count=5"), Format::Breakdown, TitleStyle::Story});
	video::Candidate Cut81 = Ace81;
	Cut81.Id = "rl:no@9:230";
	W.push_back({"81% and still lost", Cut81, Format::Short, TitleStyle::Story});
	W.push_back({"would you call this?", Cut81, Format::Short, TitleStyle::Question});
	W.push_back({"Bus, buy-in, bust: an Embercrest Nightly vlog", video::Candidate(), Format::Vlog, TitleStyle::Story});
	int Missing = 0;
	for (const Worked& X : W)
	{
		std::vector<video::Candidate> Open;
		video::Choice Ch;
		Ch.F = X.F;
		Ch.T = X.T;
		if (X.F == Format::Vlog)
		{
			Open = MultiPicks(false, At);
			for (video::Candidate& C : Open)
			{
				C.Event = "Embercrest Nightly";
				Ch.Candidates.push_back(C.Id);
			}
		}
		else
		{
			Open = {X.Lead};
			Ch.Candidates = {X.Lead.Id};
		}
		video::Channel C;
		bool Found = false;
		for (int S = 0; S < 400 && !Found; ++S)
		{
			for (const video::TitleOption& O : video::TitleOptions(Ch, Open, C, S, 3))
			{
				Found = Found || O.Text == X.Title;
			}
		}
		if (!Found)
		{
			++Missing;
			std::printf("  worked title missing: \"%s\"\n", X.Title);
			for (const video::TitleOption& O : video::TitleOptions(Ch, Open, C, 0, 3))
			{
				std::printf("    offered instead: %s \"%s\"\n", O.Key.c_str(), O.Text.c_str());
			}
			const vc::Facts Fx = vc::FactsOf(Ch, Open, vc::LeadOf(Ch, Open), &C);
			std::string Line;
			for (const auto& Kv : Fx.V)
			{
				Line += " " + Kv.first + "=" + Kv.second;
			}
			std::printf("    facts:%s\n", Line.c_str());
			std::set<std::string> Every;
			for (int S = 0; S < 400; ++S)
			{
				for (const video::TitleOption& O : video::TitleOptions(Ch, Open, C, S, 3))
				{
					Every.insert(O.Key);
				}
			}
			Line.clear();
			for (const std::string& K : Every)
			{
				Line += " " + K;
			}
			std::printf("    every key offered:%s\n", Line.c_str());
			for (const vc::Tpl& T : vc::Templates3())
			{
				const std::string Fill = vc::FillU3(T.Text, Fx);
				if (Fill == X.Title || (T.Text.find("{HandNo}") != std::string::npos && T.For == vc::ForCut))
				{
					std::printf("    %s fills \"%s\" (%zu)\n", T.Key.c_str(), Fill.c_str(), Fill.size());
				}
			}
		}
	}
	Expect(Missing == 0, "titles: the " + std::to_string(W.size()) + " worked titles reproduce from their fixtures (" + std::to_string(Missing) + " missing)");
	(void)A;
}

// ================================================================== thumbnails

void Thumbnails(const Fixture& A, bool Write)
{
	using namespace ss::video::thumb;
	using video::Format;
	// FrameOf covers all 36; the read and the name of each.
	bool Frames = true;
	for (int Ar = 0; Ar < ArCount; ++Ar)
	{
		const int Fr = static_cast<int>(video::FrameOf(Ar));
		Frames = Frames && Fr >= 0 && Fr < static_cast<int>(video::Frame::Recipe) && video::ArchName(Ar)[0] != 0 && !video::ArchRead(Ar).empty();
	}
	Expect(Frames, "thumbnails: FrameOf, a name and a read for all 36 archetypes");
	// 500 thumbnails from fixture moments (every kind and idea): signatures, archetypes, shares.
	std::vector<video::Candidate> Moments;
	for (const video::Candidate& M : A.C.Open)
	{
		Moments.push_back(M);
	}
	for (const vc::IdeaDef& D : vc::IdeaTable())
	{
		video::Candidate N;
		std::string Why;
		if (D.Id / 100 != 1 && vc::MakeIdeaCandidateOn(A.X, A.C, D.Id, A.X.World, N, Why))
		{
			Moments.push_back(N);
		}
	}
	for (int K = 0; K < static_cast<int>(video::Kind::Idea); ++K)
	{
		for (const video::Candidate& V : Variants(static_cast<video::Kind>(K), 60.0 * Day + static_cast<double>(K)) /* a minute apart: unique ids */)
		{
			Moments.push_back(V);
		}
	}
	// A fair sample: the moments in a fixed shuffled order (by their ids' hash), so the 500 cover hands, runs and ideas
	// alike rather than the first few dozen in the list.
	std::stable_sort(Moments.begin(), Moments.end(), [](const video::Candidate& X, const video::Candidate& Y) { return ss::Fnv1a("sample:" + X.Id) < ss::Fnv1a("sample:" + Y.Id); });
	video::Channel C;
	C.Open = Moments;
	std::set<uint64_t> Signatures;
	std::map<int, int> ByArch;
	std::set<int> AnyOption; // every option of the deals behind the 500 (reach)
	int Made = 0, Deals = 0, FewArchs = 0, TwoBest = 0, CapFacts = 0, Storytime = 0, Unshown = 0;
	for (size_t I = 0; Made < 500 && I < 4000; ++I)
	{
		const video::Candidate& M = Moments[I % Moments.size()];
		const int Shuffle = static_cast<int>(I / Moments.size());
		video::Choice Ch = One(M.Id, Format::Breakdown, video::TitleStyle::Story);
		const std::vector<video::ThumbSpec> Deal = video::ThumbOptions(Ch, C.Open, C, Shuffle, 6);
		if (Deal.empty())
		{
			continue;
		}
		++Deals;
		std::set<int> Archs;
		int Best = 0;
		const std::vector<video::Candidate> P = {M};
		const vc::Facts F = vc::FactsOf(Ch, P, M, &C);
		double BestFit = 0.0;
		for (const int Ar : vc::AllowedArchs(M, F))
		{
			BestFit = std::max(BestFit, video::ArchetypeFit(Ar, 0, M));
		}
		for (const video::ThumbSpec& S : Deal)
		{
			Archs.insert(S.Arch);
			if (Made < 500)
			{
				AnyOption.insert(S.Arch);
			}
			// The 500: each deal's first option (hash order, so any allowed archetype), over every moment and then again.
			if (Made < 500 && &S == &Deal.front())
			{
				Signatures.insert(video::ThumbSignature(S));
				++ByArch[S.Arch];
				++Made;
			}
			// Captions only from facts: every digit run of the caption is a fact of the moment.
			std::string Bad;
			if (!DigitsAreFacts(S.Caption + " " + video::ThumbCaption2(S), F, Bad))
			{
				++CapFacts;
				std::printf("  caption digit '%s' isn't a fact: %s / %s (%s)\n", Bad.c_str(), S.Caption.c_str(), video::ThumbCaption2(S).c_str(), video::ArchName(S.Arch));
			}
			// Storytime: no face but the player's, no name.
			if (M.Storytime && (S.Subj == SjOpponent || S.Subj == SjWorldPlayer || !video::ThumbFact(S, "o").empty()))
			{
				++Storytime;
			}
			// No unshown card: the cards fact's second part only when a hand was turned up.
			const std::string Cards = video::ThumbFact(S, "c");
			if (!Cards.empty())
			{
				const size_t A1 = Cards.find('/'), A2 = Cards.find('/', A1 + 1);
				const std::string Theirs = A1 == std::string::npos || A2 == std::string::npos ? std::string() : Cards.substr(A1 + 1, A2 - A1 - 1);
				Unshown += !Theirs.empty() && M.Rec.Shown.empty() ? 1 : 0;
			}
		}
		if (static_cast<int>(Archs.size()) < std::min<int>(4, static_cast<int>(vc::AllowedArchs(M, F).size())))
		{
			std::printf("  short deal: %s kind %d idea %d: %zu archetypes of %zu allowed (%zu dealt)\n", M.Id.c_str(), static_cast<int>(M.K), M.Idea, Archs.size(), vc::AllowedArchs(M, F).size(), Deal.size());
		}
		FewArchs += static_cast<int>(Archs.size()) < std::min<int>(4, static_cast<int>(vc::AllowedArchs(M, F).size())) ? 1 : 0;
		for (const video::ThumbSpec& S : Deal)
		{
			Best += std::fabs(video::ArchetypeFit(S.Arch, 0, M) - BestFit) < 1e-9 ? 1 : 0;
		}
		std::set<int> BestArchs;
		for (const video::ThumbSpec& S : Deal)
		{
			if (std::fabs(video::ArchetypeFit(S.Arch, 0, M) - BestFit) < 1e-9)
			{
				BestArchs.insert(S.Arch);
			}
		}
		TwoBest += BestArchs.size() > 1 || (BestArchs.size() == 1 && Best > 1 && Archs.size() > 1) ? 0 : 0;
	}
	int Biggest = 0;
	for (const auto& Kv : ByArch)
	{
		Biggest = std::max(Biggest, Kv.second);
	}
	int Reached = 0;
	for (int Ar = 0; Ar < ArCount; ++Ar)
	{
		Reached += AnyOption.count(Ar) > 0 ? 1 : 0;
	}
	std::printf("  thumbnails: %d made, %zu distinct signatures, %d archetypes reached, the largest share %d\n", Made, Signatures.size(), Reached, Biggest);
	{
		std::string Shares, Missing;
		for (int Ar = 0; Ar < ArCount; ++Ar)
		{
			if (ByArch.count(Ar) > 0)
			{
				Shares += " " + std::string(video::ArchName(Ar)) + "=" + std::to_string(ByArch[Ar]);
			}
			else if (Ar != ArPov && Ar != ArCollabDuo)
			{
				Missing += " " + std::string(video::ArchName(Ar));
			}
		}
		std::printf("  thumbnail shares:%s\n  never dealt:%s\n", Shares.c_str(), Missing.c_str());
	}
	Expect(Made == 500 && Signatures.size() >= 300, "thumbnails: 500 from fixture moments give >= 300 distinct looks (" + std::to_string(Signatures.size()) + ")");
	Expect(Reached >= 34, "thumbnails: every P0 archetype reached (" + std::to_string(Reached) + " of 34; Collab Duo and POV are P1)");
	Expect(Biggest * 100 <= 8 * 500, "thumbnails: no archetype above 8% of the 500 (" + std::to_string(Biggest) + ")");
	Expect(FewArchs == 0, "thumbnails: at least four archetypes a deal where the moment allows four (" + std::to_string(FewArchs) + " short)");
	// A deal of six with at least four archetypes (SPEC §6.1): every moment and idea allows four.
	{
		int Few = 0;
		std::set<std::string> SeenIds;
		for (const video::Candidate& M : Moments)
		{
			const vc::IdeaDef* Def = M.K == video::Kind::Idea ? vc::IdeaById(M.Idea) : nullptr;
			if (!SeenIds.insert(M.Id).second || (Def && Def->Formats == 1))
			{
				continue; // a Cut-only idea gets no thumbnail (P0)
			}
			const video::Choice Ch = One(M.Id, Format::Breakdown, video::TitleStyle::Story);
			const vc::Facts F = vc::FactsOf(Ch, {M}, M, &C);
			const std::vector<int> Al = vc::AllowedArchs(M, F);
			if (Al.size() < 4)
			{
				++Few;
				std::string Names;
				for (const int Ar : Al)
				{
					Names += std::string(" ") + video::ArchName(Ar) + ",";
				}
				std::printf("  few archetypes: %s kind %d idea %d:%s\n", M.Id.c_str(), static_cast<int>(M.K), M.Idea, Names.c_str());
			}
		}
		Expect(Few == 0, "thumbnails: every moment and idea allows at least four archetypes (" + std::to_string(Few) + " fewer)");
	}
	Expect(CapFacts == 0, "thumbnails: every digit in a caption is a fact (" + std::to_string(CapFacts) + ")");
	Expect(Storytime == 0, "thumbnails: storytime draws no face and no name but the player's");
	Expect(Unshown == 0, "thumbnails: no unshown card");
	(void)TwoBest;
	// Every kind group's deals show >= 5 archetypes over 20 shuffles.
	for (int G = 0; G < 6; ++G)
	{
		std::set<int> Seen;
		for (const video::Candidate& M : Moments)
		{
			bool InGroup = false;
			switch (G)
			{
			case 0: InGroup = M.K == video::Kind::BadBeat || M.K == video::Kind::Suckout || M.K == video::Kind::Cooler || M.K == video::Kind::SetOverSet || M.K == video::Kind::RunnerRunner; break;
			case 1: InGroup = M.K == video::Kind::Bluff || M.K == video::Kind::BluffCaught || M.K == video::Kind::HeroCall || M.K == video::Kind::HeroFold || M.K == video::Kind::LadderFold; break;
			case 2: InGroup = M.K == video::Kind::BigPot || M.K == video::Kind::Knockout || M.K == video::Kind::Bubble || M.K == video::Kind::Monster || M.K == video::Kind::DoubleKo; break;
			case 3: InGroup = M.K == video::Kind::DeepRun || M.K == video::Kind::FinalTable || M.K == video::Kind::Win || M.K == video::Kind::HeadsUp; break;
			case 4: InGroup = M.K == video::Kind::Rival; break;
			default: InGroup = M.K == video::Kind::Idea || M.K == video::Kind::StreamMoment || M.K == video::Kind::Story; break;
			}
			if (!InGroup)
			{
				continue;
			}
			for (int S = 0; S < 20; ++S)
			{
				for (const video::ThumbSpec& T : video::ThumbOptions(One(M.Id, Format::Breakdown, video::TitleStyle::Story), C.Open, C, S, 6))
				{
					Seen.insert(T.Arch);
				}
			}
		}
		Expect(Seen.size() >= 5, "thumbnails: kind group " + std::to_string(G) + "'s deals show >= 5 archetypes over 20 shuffles (" + std::to_string(Seen.size()) + ")");
	}
	// No consecutive (archetype, palette) repeat: the last upload's pair never comes back in the next deal.
	{
		video::Channel Ch2 = A.C;
		video::Video& Last = Ch2.Videos.back();
		Last.X.On = true;
		Last.Fr = video::Frame::Recipe;
		const video::Candidate& Beat = Ch2.Open.front();
		const std::vector<video::ThumbSpec> D0 = video::ThumbOptions(One(Beat.Id, Format::Breakdown, video::TitleStyle::Story), Ch2.Open, Ch2, 0, 6);
		int Repeat = 0;
		if (!D0.empty())
		{
			Last.X.Spec[0] = D0.front();
			for (int S = 0; S < 20; ++S)
			{
				for (const video::ThumbSpec& T : video::ThumbOptions(One(Beat.Id, Format::Breakdown, video::TitleStyle::Story), Ch2.Open, Ch2, S, 6))
				{
					Repeat += T.Arch == D0.front().Arch && T.Pal == D0.front().Pal ? 1 : 0;
				}
			}
		}
		Expect(!D0.empty() && Repeat == 0, "thumbnails: the last upload's (archetype, palette) never comes straight back (" + std::to_string(Repeat) + ")");
	}
	// The contact sheet (VT1): 24 a group, no (archetype, palette) twice.
	for (const int G : {0, 3})
	{
		const std::vector<video::ThumbSpec> Sheet = video::ContactSheet(G, 24);
		std::set<std::pair<int, int>> Pairs;
		for (const video::ThumbSpec& S : Sheet)
		{
			Pairs.insert({S.Arch, S.Pal});
		}
		Expect(Sheet.size() >= 12 && Pairs.size() == Sheet.size(), "thumbnails: group " + std::to_string(G) + "'s contact sheet repeats no (archetype, palette) (" + std::to_string(Sheet.size()) + ")");
	}
	// TraitMean: each group's mean trait product over the generator's deals is 1 within 2% (pinned by --write).
	double Sum[6] = {}, N[6] = {};
	for (const video::Candidate& M : Moments)
	{
		for (int S = 0; S < 40; ++S)
		{
			for (const video::ThumbSpec& T : video::ThumbOptions(One(M.Id, Format::Breakdown, video::TitleStyle::Story), C.Open, C, S, 6))
			{
				const int Group = [&] {
					switch (M.K)
					{
					case video::Kind::BadBeat:
					case video::Kind::Suckout:
					case video::Kind::Cooler:
					case video::Kind::SetOverSet:
					case video::Kind::RunnerRunner: return 0;
					case video::Kind::Bluff:
					case video::Kind::BluffCaught:
					case video::Kind::HeroCall:
					case video::Kind::HeroFold:
					case video::Kind::LadderFold: return 1;
					case video::Kind::BigPot:
					case video::Kind::Knockout:
					case video::Kind::Bubble:
					case video::Kind::Monster:
					case video::Kind::DoubleKo:
					case video::Kind::MinCash:
					case video::Kind::Envelope:
					case video::Kind::BiggestPot: return 2;
					case video::Kind::DeepRun:
					case video::Kind::FinalTable:
					case video::Kind::Win:
					case video::Kind::HeadsUp: return 3;
					case video::Kind::Rival: return 4;
					default: return 5;
					}
				}();
				video::Channel Empty;
				Sum[Group] += video::ThumbTraits(T, M, Empty) * video::TraitMean(Group);
				N[Group] += 1.0;
			}
		}
	}
	std::string Means;
	bool Pinned = true;
	for (int G = 0; G < 6; ++G)
	{
		const double Mean = N[G] > 0.0 ? Sum[G] / N[G] : 1.0;
		Means += " " + std::to_string(G) + ":" + ss::Fixed(Mean, 4);
		Pinned = Pinned && std::fabs(Mean / video::TraitMean(G) - 1.0) <= 0.02;
	}
	if (Write)
	{
		std::printf("TraitMean (pin these in VideoThumbs.cpp):%s\n", Means.c_str());
	}
	Expect(Pinned, "thumbnails: TraitMean pinned within 2% of each group's mean trait product (" + Means + ")");
	// Promise and the facts block's round trip.
	video::ThumbSpec S;
	S.Arch = ArGutPunch;
	S.Expr = ExScream;
	S.Sticker = StSiren;
	S.Caption = "WHY.";
	S.Caption2 = std::string("AND LOST") + FactSep + "c=AsAd/7c7h/Kd8s2c9h7d" + FactSep + "h=one" + FactSep + "h=two";
	Expect(video::ThumbPromise(S) == 3 && video::ThumbCaption2(S) == "AND LOST" && video::ThumbFact(S, "c") == "AsAd/7c7h/Kd8s2c9h7d" && video::ThumbFacts(S, "h").size() == 2,
		"thumbnails: the promise, the second line and the compositor's facts read back");
}

/** DESIGN §4.9.1's worked thumbnails: each one's archetype allowed for its fixture, its palette in the archetype's set, and
 *  its caption among the archetype's fillable rows. */
void WorkedThumbnails(const Fixture& A)
{
	using namespace ss::video::thumb;
	using video::Format;
	struct Worked
	{
		const char* Name;
		std::string Lead;
		int Arch, Pal;
		const char* Caption;
	};
	std::vector<Worked> W = {
		{"1 gut punch", "rl:no@1:1", ArGutPunch, PlHeat, "82%"},
		{"2 circled river", "rl:no@1:1", ArCircledRiver, PlNight, "THE RIVER."},
		{"3 the fold", "rl:no@1:8", ArTheFold, PlIce, "FOLD?"},
		{"4 nerves of steel", "rl:no@1:5", ArNervesOfSteel, PlNeon, "7-2"},
		{"5 the read", "rl:no@1:7", ArTheRead, PlGoldOnBlack, "HERO CALL"},
		{"6 vs split", "rl:no@1:4", ArVsSplit, PlBlood, "SET vs SET"},
		{"7 the pile", "rl:no@1:9", ArThePile, PlMoney, "128 BB"},
		{"10 the ghost", "rl:no@1:15", ArTheGhost, PlNeon, "HIM AGAIN"},
		{"11 text slab", "dee:439:7", ArTextSlab, PlMono, "THEY HAD IT"},
		{"21 stream clip", "rl:no@1:18", ArStreamClip, PlNeon, "CHAT LOST IT"},
		{"22 envelope", "rl:mh@1:14", ArEnvelope, PlGoldOnBlack, "25X"},
	};
	int Bad = 0;
	for (const Worked& X : W)
	{
		const video::Candidate* M = nullptr;
		for (const video::Candidate& C : A.C.Open)
		{
			M = C.Id == X.Lead ? &C : M;
		}
		if (!M)
		{
			++Bad;
			std::printf("  worked thumbnail %s: no fixture\n", X.Name);
			continue;
		}
		const video::Choice Ch = One(M->Id, Format::Breakdown, video::TitleStyle::Story);
		const vc::Facts F = vc::FactsOf(Ch, {*M}, *M, &A.C);
		const std::vector<int> Allowed = vc::AllowedArchs(*M, F);
		bool Arch = std::find(Allowed.begin(), Allowed.end(), X.Arch) != Allowed.end();
		bool Pal = false, Cap = false;
		for (uint32_t H = 0; H < 400 && Arch && !(Pal && Cap); ++H)
		{
			const video::ThumbSpec S = vc::RecipeFor(X.Arch, *M, {*M}, F, ss::Fnv1a("worked" + std::to_string(H)), -1, -1);
			Pal = Pal || S.Pal == X.Pal;
			Cap = Cap || S.Caption == X.Caption;
		}
		if (!(Arch && Pal && Cap))
		{
			++Bad;
			std::printf("  worked thumbnail %s: archetype %d palette %d caption %d\n", X.Name, Arch, Pal, Cap);
		}
	}
	Expect(Bad == 0, "thumbnails: the worked thumbnails reproduce from their fixtures (" + std::to_string(Bad) + " of " + std::to_string(W.size()) + " don't)");
}

// ================================================================== ideas

void IdeasCheck(const Fixture& A, const Fixture& B)
{
	using video::Format;
	int Ready = 0, NoFacts = 0, NoTitle = 0, NoArch = 0, NoHint = 0;
	std::string NotReady;
	for (const vc::IdeaDef& D : vc::IdeaTable())
	{
		bool Done = false;
		for (const Fixture* Fx : {&A, &B})
		{
			if (Done)
			{
				break;
			}
			video::Candidate N;
			std::string Why;
			if (D.Id / 100 == 1)
			{
				const std::string Src = vc::IdeaMoment(Fx->X, Fx->C, D.Id, Fx->X.World);
				for (const video::Candidate& M : Fx->C.Open)
				{
					if (M.Id == Src)
					{
						N = M;
						N.Idea = D.Id;
					}
				}
				if (N.Id.empty())
				{
					continue;
				}
			}
			else if (!vc::MakeIdeaCandidateOn(Fx->X, Fx->C, D.Id, Fx->X.World, N, Why))
			{
				continue;
			}
			Done = true;
			++Ready;
			const bool Facts = D.Id / 100 == 1 || N.IdeaFacts.find('=') != std::string::npos;
			NoFacts += Facts ? 0 : 1;
			// A title from the idea's own rows (or its kind's), not the last word.
			const Format F = (D.Formats & 2) ? Format::Breakdown : (D.Formats & 4) ? Format::Vlog : (D.Formats & 8) ? Format::Recap : Format::Short;
			bool Title = false;
			for (int S = 0; S < 5 && !Title; ++S)
			{
				if (!((D.Styles >> S) & 1))
				{
					continue;
				}
				for (const video::TitleOption& O : video::TitleOptions(One(N.Id, F, static_cast<video::TitleStyle>(S)), {N}, Fx->C, 0, 3))
				{
					Title = Title || O.Key.rfind("fallback", 0) != 0;
				}
			}
			if (!Title)
			{
				++NoTitle;
				std::printf("  idea %s: no title\n", D.Code);
			}
			const vc::Facts Fc = vc::FactsOf(One(N.Id, F, video::TitleStyle::Story), {N}, N, &Fx->C);
			if (vc::AllowedArchs(N, Fc).empty())
			{
				++NoArch;
				std::printf("  idea %s: no archetype\n", D.Code);
			}
		}
		if (!Done)
		{
			NotReady += std::string(" ") + D.Code;
		}
	}
	Expect(vc::IdeaTable().size() == 148, "ideas: 148 P0 ideas in the table (" + std::to_string(vc::IdeaTable().size()) + ")");
	Expect(Ready == static_cast<int>(vc::IdeaTable().size()), "ideas: every idea ready on fixture state (" + std::to_string(Ready) + "; not:" + NotReady + ")");
	Expect(NoFacts == 0, "ideas: every ready idea's candidate carries its facts");
	Expect(NoTitle == 0, "ideas: every ready idea gets a title of its own (" + std::to_string(NoTitle) + ")");
	Expect(NoArch == 0, "ideas: every ready idea has an archetype (" + std::to_string(NoArch) + ")");
	// Every locked idea says why (a fresh career: nothing played).
	video::IdeaCtx Fresh;
	Fresh.World = 2.0 * Day;
	video::Channel Empty;
	Fresh.Chan = &Empty;
	int Locked = 0;
	for (const video::IdeaCard& Card : video::Ideas(Fresh, Empty, Fresh.World))
	{
		if (!Card.Ready)
		{
			++Locked;
			const std::string Why = copylint::Check(Card.Hint, copylint::Pa, {});
			if (Card.Hint.empty() || !Why.empty() || SaysAutoplay(Card.Hint))
			{
				++NoHint;
				std::printf("  locked idea %d: \"%s\" %s\n", Card.Id, Card.Hint.c_str(), Why.c_str());
			}
		}
	}
	Expect(Locked > 100 && NoHint == 0, "ideas: a fresh career's locked ideas each say what to do (" + std::to_string(Locked) + " locked)");
	// Facts-only and linted fact lines on the board.
	int BadLines = 0;
	for (const video::IdeaCard& Card : video::Ideas(A.X, A.C, A.X.World))
	{
		if (Card.Ready && (!copylint::Check(Card.Fact + ".", copylint::Pa, {}).empty() || Brand(Card.Fact)))
		{
			++BadLines;
			std::printf("  idea %d fact line: \"%s\"\n", Card.Id, Card.Fact.c_str());
		}
	}
	Expect(BadLines == 0, "ideas: every ready card's fact line is linted and clean");
}

/** A 180-day pacing creator's state (§6.3: >= 80 distinct ideas by day 180): RiverLine nights, a few live entries, a channel
 *  posting three a week, a stream a week. */
void PacingCreator()
{
	using namespace ss::video;
	IdeaCtx X;
	Channel C;
	const double Now = 180.0 * Day + 21.0 * 60.0;
	X.World = Now;
	// The last 30 nights (three RiverLine tournaments a night, a Sunday's majors, now and then an Omaha, a mystery, a series
	// day, a hyper or two, a freeroll, a satellite, The Spark; Ignite since day 120), and five live nights at the Embercrest.
	int N = 0;
	auto Play = [&](const char* Name, double At, int Site, Chips BuyIn, uint32_t Flags, int Place, Chips Prize) {
		IdeaResult R;
		R.Name = Name;
		R.Id = "p@" + std::to_string(N++);
		R.At = At;
		R.Site = Site;
		R.Field = Flags & IfLive ? 60 : 300;
		R.Place = Place;
		R.BuyIn = BuyIn;
		R.Prize = Prize;
		R.Flags = Flags;
		X.Results.push_back(R);
	};
	const int Ig = static_cast<int>(ss::net::SiteId::Ignite);
	for (int D = 0; D < 30; ++D)
	{
		const double Night = Now - static_cast<double>(D) * Day - 60.0;
		const int Today = static_cast<int>(Night / Day);
		const bool Sunday = ss::net::Weekday(Today) == 6;
		Play("Night Owl Turbo", Night, 0, 550, IfTurbo, D % 7 == 0 ? 9 : 120 + D, D % 7 == 0 ? 4200 : 0);
		Play("Daily Deep", Night + 10.0, 0, 550, 0, D % 11 == 0 ? 30 : 140 + D, D % 11 == 0 ? 1100 : 0);
		Play("PKO Turbo", Night + 20.0, 0, 550, IfBounty, 90 + D, D % 5 == 0 ? 780 : 0);
		if (Sunday)
		{
			Play("Sunday Blaze", Night - 120.0, 0, 1100, IfMajor, D < 7 ? 4 : 210, D < 7 ? 30000 : 0);
			Play("Sunday 100K", Night - 110.0, 0, 1100, IfMajor, 400, 0);
			Play("Daily $11", Night - 100.0, 0, 1100, 0, 60, 0);
		}
		if (D == 2)
		{
			Play("Omaha Circuit", Night + 30.0, 0, 550, IfOmaha, 12, 0);
			Play("Omaha Circuit Turbo", Night + 40.0, 0, 550, IfOmaha, 3, 2400);
			Play("PKO Deep", Night + 50.0, 0, 550, IfBounty, 31, 420);
		}
		if (D == 1 || D == 9 || D == 16)
		{
			Play("Mystery Hunt", Night + 30.0, 0, 550, IfMystery, 50, 0);
			for (int H = 0; H < 3; ++H)
			{
				Play("Hyper Turbo", Night + 35.0 + H, 0, 550, IfHyper, 6 + H, H == 0 ? 1100 : 0);
			}
		}
		if (D == 3)
		{
			Play("Micro Madness #4", Night - 60.0, 0, 330, IfSeries, 88, 0);
			Play("Micro Madness #5", Night - 30.0, 0, 330, IfSeries, 40, 900);
		}
		if (D == 4)
		{
			Play("Night Freeroll", Night + 30.0, 0, 0, IfFreeroll, 20, 250);
		}
		if (D == 5)
		{
			Play("RCOP Main Mega Satellite", Night + 30.0, 0, 220, IfSatellite | IfSeatWon, 3, 5500);
		}
		if (D == 6)
		{
			Play("The Spark", Night + 30.0, 0, 1100, IfSpark, 22, 2900);
		}
		if (D % 4 == 0 && D < 28)
		{
			Play("Ignite Nightly", Night + 60.0, Ig, 1100, 0, 70, 0);
		}
		if (D == 8)
		{
			Play("Midnight Deepstack", Night + 30.0, 0, 550, IfFinal, 1, 30000);
		}
	}
	const char* Live[] = {"Embercrest Nightly", "Embercrest Sunday", "Midnight Turbo", "Embercrest Nightly", "Embercrest Bounty"};
	for (int I = 0; I < 5; ++I)
	{
		Play(Live[I], Now - static_cast<double>(2 + 9 * I) * Day, 0, I == 1 ? 12000 : 4000, IfLive | (I == 2 ? IfTurbo : 0), I == 2 ? 3 : 8 + I, I == 2 ? 31000 : 0);
	}
	X.Joined = {0, Ig};
	X.Events = 420;
	X.Cashes = 60;
	X.Wins = 2;
	X.BuyIns = 420 * 500;
	X.Prizes = 420 * 520;
	X.BestPrize = 30000;
	X.LiveEvents = 5;
	X.StudyWeek = 1;
	X.Bankroll = 90000;
	X.RentDeadline = Now + 15.0 * Day;
	X.RentStage = 1;
	X.RentsPaid = 6;
	X.LastRentAt = Now - 8.0 * Day;
	X.Stories = {"rent-paid"};
	X.Shifts = 30;
	X.NightShifts14 = 3;
	X.WashFold = 1;
	X.LastShiftAt = Now - 6.0 * 60.0;
	X.Gear = {{"Desk mic", 12900}, {"Key light", 8900}, {"Second monitor", 25000}, {"Webcam", 9900}, {"Chair", 30000}};
	X.GearBought7 = 25000;
	X.NewestGear = "Second monitor";
	X.Home = 1;
	X.KastFollowers = 140;
	X.KastStreams = 26;
	IdeaStream S;
	S.Start = Now - 3.0 * Day;
	S.Minutes = 200.0;
	S.Avg = 30;
	S.Peak = 61;
	S.Predictions = 3;
	S.Raids = 1;
	X.Streams.push_back(S);
	X.Comments30 = 210;
	X.SubsWeekAgo = 90;
	IdeaGrade G;
	G.At = Now - 1.0 * Day;
	G.Grade = 3;
	G.Hand = "ace-ten";
	G.Street = "river";
	G.River = true;
	G.Shove = true;
	G.StackBb = 11;
	G.CostBb = 6;
	for (int I = 0; I < 12; ++I)
	{
		G.Grade = I % 3 == 0 ? 0 : 3;
		X.Grades.push_back(G);
	}
	X.WorldLog.push_back({Now - 2.0 * Day, static_cast<int>(ss::world::EventKind::Champion), "Ana Varga", "", "Sunday Blaze", 120000, 0});
	X.WorldLog.push_back({Now - 3.0 * Day, static_cast<int>(ss::world::EventKind::FinalTable), "Mo Reyes", "", "Sunday Blaze", 0, 0});
	X.WorldLog.push_back({Now - 4.0 * Day, static_cast<int>(ss::world::EventKind::BoardReport), "Mo Reyes", "", "Season", 0, 0});
	X.WorldLog.push_back({Now - 5.0 * Day, static_cast<int>(ss::world::EventKind::Rivalry), "Ana Varga", "Mo Reyes", "", 0, 0});
	X.SeriesNow = "Micro Madness";
	X.SeasonRank = 140;
	C.Subs = 120;
	C.X.On = true;
	C.X.Since = 4;
	C.X.UploadsEver = 70;
	for (int I = 0; I < 60; ++I)
	{
		Video V;
		V.Id = "v" + std::to_string(I);
		V.At = Now - static_cast<double>(60 - I) * 2.4 * Day;
		V.K = I % 10 == 0 ? Kind::Rival : Kind::BadBeat;
		V.F = I % 5 == 0 ? Format::Short : Format::Breakdown;
		V.P = 200.0;
		C.Videos.push_back(V);
	}
	// MOMENTS the last week of play left: a spread of hands, a stream moment, a final table on stream.
	const Kind Kinds[] = {Kind::BadBeat, Kind::Suckout, Kind::Cooler, Kind::Bluff, Kind::HeroCall, Kind::HeroFold, Kind::BigPot, Kind::Knockout, Kind::Bubble, Kind::Rival,
		Kind::StreamMoment, Kind::BluffCaught, Kind::LastHand, Kind::DoubleKo, Kind::LadderFold, Kind::MinCash};
	for (int I = 0; I < static_cast<int>(sizeof(Kinds) / sizeof(Kinds[0])); ++I)
	{
		video::Candidate M = HandOf("rl:p@" + std::to_string(I) + ":1", Kinds[I], "AsAd", "7c7h", "Kd8s2c9h7d", I % 2 ? 6400 : -6400, 0.81, Now - 1.0 * Day + I);
		if (Kinds[I] == Kind::Rival)
		{
			M.Opponent = ss::RivalName;
			M.Rival = true;
			M.Rec.Shown[0].first = ss::RivalName;
		}
		if (Kinds[I] == Kind::StreamMoment)
		{
			M.Stream.Viewers = 40;
			M.Stream.Chat = {"nice"};
		}
		if (Kinds[I] == Kind::HeroFold)
		{
			M.Rec.Mine = ss::ParseCards("KhKd");
		}
		C.Open.push_back(M);
	}
	X.Chan = &C;
	std::set<int> Ready;
	for (const IdeaCard& Card : Ideas(X, C, Now))
	{
		if (Card.Ready)
		{
			Ready.insert(Card.Id);
		}
	}
	std::printf("  a 180-day pacing creator: %zu ideas ready\n", Ready.size());
	if (Ready.size() < 80)
	{
		for (const IdeaCard& Card : Ideas(X, C, Now))
		{
			if (!Card.Ready)
			{
				std::printf("    locked %d: %s\n", Card.Id, Card.Hint.c_str());
			}
		}
	}
	Expect(Ready.size() >= 80, "ideas: a 180-day pacing creator unlocks >= 80 distinct ideas (" + std::to_string(Ready.size()) + ")");
}

// ================================================================== comments and the world's creators

/** Studio's per-frame costs (§10: a Rerun page 1.5 ms warm): three titles, six recipes, the packaging read and a page of
 *  comments, for the fixture's hand and an idea. */
void Budgets(const Fixture& A)
{
	const video::Candidate* Hand = nullptr;
	for (const video::Candidate& C : A.C.Open)
	{
		Hand = !Hand && C.K == video::Kind::BadBeat ? &C : Hand;
	}
	if (!Hand)
	{
		Expect(false, "budgets: the fixture has a bad beat");
		return;
	}
	video::Choice Ch = One(Hand->Id, video::Format::Breakdown, video::TitleStyle::Story);
	const int N = 200;
	auto Time = [&](auto&& F) {
		const auto T0 = std::chrono::steady_clock::now();
		for (int I = 0; I < N; ++I)
		{
			F(I);
		}
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / N;
	};
	size_t Sink = 0;
	const double Titles = Time([&](int I) { Sink += video::TitleOptions(Ch, A.C.Open, A.C, I, 3).size(); });
	const double Thumbs = Time([&](int I) { Sink += video::ThumbOptions(Ch, A.C.Open, A.C, I, 6).size(); });
	const double Read = Time([&](int) { Sink += video::PackagingRead(Ch, A.C.Open, A.C, A.X.World, true).size(); });
	video::Video V = A.C.Videos.back();
	V.X.On = true;
	V.X.Fate = 99;
	const double Comments = Time([&](int I) { Sink += video::CommentsOf(V, A.X.World, (I % 5) * 20, 20).size(); });
	std::printf("  budgets: titles %.3f ms, recipes %.3f ms, packaging read %.3f ms, 20 comments %.3f ms a call (%zu)\n", Titles, Thumbs, Read, Comments, Sink);
	Expect(Titles + Thumbs + Read < 1.5, "budgets: Studio's titles, recipes and read together under a page's 1.5 ms");
	Expect(Comments < 0.5, "budgets: a page of comments under 0.5 ms");
}

/** The Session's glue (SessionVideo3.cpp, SessionVideo.cpp): Studio's titles and recipes through the session, PUBLISH with
 *  a template and a recipe (the title Studio showed goes up, the edit's time), a Duel's time, Make a Cut, an idea chosen,
 *  a repackage. */
void SessionGlue()
{
	struct Hooks : ss::SessionHooks
	{
		void Save(const ss::SaveData& D) override { Last = D; }
		ss::SaveData Last;
	};
	auto Wait = [](ss::Session& S, double& Now, double Seconds) {
		for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
		}
	};
	Hooks H;
	ss::Session S(H, "rerun-content-glue");
	double Now = 0.0;
	S.CurrentScreen = ss::Screen::Lobby;
	Wait(S, Now, 0.2);
	S.Life.Update3LinesRead |= ss::life::U3Video; // a career on Rerun 2.0 from its start (NewCareer's)
	S.Life.Energy = 90.0;
	S.Life.Hunger = 5.0;
	S.Life.Thirst = 5.0;
	S.DebugVideo("fake badbeat online");
	Expect(S.Life.Video.Open.size() == 1, "glue: a moment in MOMENTS");
	if (S.Life.Video.Open.empty())
	{
		return;
	}
	video::Choice C;
	C.Candidates = {S.Life.Video.Open[0].Id};
	C.F = video::Format::Breakdown;
	C.T = video::TitleStyle::Story;
	const std::vector<video::TitleOption> Titles = S.TitleOptions(C, 0);
	const std::vector<video::ThumbSpec> Thumbs = S.ThumbOptions(C, 0);
	std::set<int> Archs;
	for (const video::ThumbSpec& T : Thumbs)
	{
		Archs.insert(T.Arch);
	}
	Expect(Titles.size() == 3 && Thumbs.size() == 6 && Archs.size() >= 4, "glue: Studio's three titles and six recipes (four archetypes or more) through the session (" + std::to_string(Titles.size()) + ", " +
		std::to_string(Thumbs.size()) + ", " + std::to_string(Archs.size()) + ")");
	if (Titles.size() < 2 || Thumbs.empty())
	{
		return;
	}
	C.TitleTpl = Titles[1].Key;
	C.Thumbs = {Thumbs[0]};
	const double W0 = S.WorldMinutes();
	Expect(S.Publish(C).empty() && S.TimeSkip.Active && S.TimeSkip.Result.Body == Titles[1].Text, "glue: PUBLISH starts the edit under the title Studio showed");
	const double Minutes = S.TimeSkip.To - S.TimeSkip.From;
	Expect(std::fabs(Minutes - video::EditMinutesOf(C)) < 0.01, "glue: the edit takes the length option's time");
	Wait(S, Now, 8.0);
	const video::Video* V = S.Life.Video.Videos.empty() ? nullptr : &S.Life.Video.Videos.back();
	Expect(V && V->X.On && V->Title == Titles[1].Text && V->X.TitleTpl == Titles[1].Key && V->X.Spec[0].Arch == Thumbs[0].Arch,
		"glue: the video goes up as Rerun 2.0's, with the title and the recipe Studio showed");
	Expect(S.WorldMinutes() - W0 >= Minutes - 1.0, "glue: the clock moved by the edit");
	if (!V)
	{
		return;
	}
	const std::string Long = V->Id;
	// Make a Cut: twenty minutes, then a Cut of the same hand goes up beside it.
	Expect(S.LaunchAction(Long, video::Launch::Cut).empty() && S.TimeSkip.Active, "glue: Make a Cut starts its twenty minutes");
	Wait(S, Now, 4.0);
	const video::Video* Cut = nullptr;
	for (const video::Video& X : S.Life.Video.Videos)
	{
		Cut = X.F == video::Format::Short && X.X.On ? &X : Cut;
	}
	Expect(Cut && !Cut->Title.empty() && Cut->Title.size() <= 50, "glue: Make a Cut puts a Cut of the hand up (" + std::string(Cut ? Cut->Title : "none") + ")");
	// A Duel: three recipes, 30 more minutes of edit.
	S.DebugVideo("fake cooler online");
	video::Choice D;
	D.Candidates = {S.Life.Video.Open.back().Id};
	D.F = video::Format::Breakdown;
	for (const video::TitleOption& O : S.TitleOptions(D, 0))
	{
		D.TitleTpl = D.TitleTpl.empty() ? O.Key : D.TitleTpl;
	}
	for (const video::ThumbSpec& T : S.ThumbOptions(D, 0))
	{
		if (D.Thumbs.size() < 3)
		{
			D.Thumbs.push_back(T);
		}
	}
	video::Choice D1 = D;
	D1.Thumbs.resize(1);
	Expect(std::fabs(video::EditMinutesOf(D) - video::EditMinutesOf(D1) - 30.0) < 0.01 && video::EditEnergyOf(D) == video::EditEnergyOf(D1) + 2.0,
		"glue: a Duel of three adds 30 minutes and 2 energy");
	Expect(S.Publish(D).empty(), "glue: a Duel publishes");
	Wait(S, Now, 8.0);
	// Repackage: a new title from what the video keeps of its moment.
	video::Video* R = nullptr;
	for (video::Video& X : S.Life.Video.Videos)
	{
		R = X.Id == Long ? &X : R;
	}
	if (R)
	{
		const std::string Was = R->Title;
		const std::vector<video::TitleOption> New = video::RepackTitles(*R, S.Life.Video, 0, 3);
		video::Repack Rp;
		Rp.TitleKey = New.empty() ? std::string() : New.front().Key;
		Expect(!New.empty() && S.Repackage(Long, Rp).empty(), "glue: a repackage takes a new title");
		for (const video::Video& X : S.Life.Video.Videos)
		{
			Expect(X.Id != Long || (X.Title != Was && !New.empty() && X.Title == New.front().Text), "glue: the repackaged title is the option's words");
		}
	}
	// An idea: the board lists them; a ready one becomes a candidate (Source 5).
	const std::vector<video::IdeaCard> Board = S.VideoIdeas();
	int Chosen = -1;
	for (const video::IdeaCard& I : Board)
	{
		Chosen = Chosen < 0 && I.Ready && I.Id / 100 != 1 ? I.Id : Chosen;
	}
	Expect(Board.size() >= 140, "glue: the session's IDEAS board (" + std::to_string(Board.size()) + " cards)");
	if (Chosen >= 0)
	{
		const std::string Why = S.ChooseIdea(Chosen);
		bool In = false;
		for (const video::Candidate& X : S.Life.Video.Open)
		{
			In = In || (X.Source == 5 && X.Idea == Chosen);
		}
		Expect(Why.empty() && In, "glue: a ready idea becomes a candidate in MOMENTS (" + std::to_string(Chosen) + (Why.empty() ? std::string() : ": " + Why) + ")");
	}
	Expect(H.Last.Serialize().find("video\tvid\t") != std::string::npos, "glue: the save keeps the videos");
}

bool PrintWorld = false; // "world": every world video, for review

/** The world's creators (§6.5): their videos from the world log and the calendar, true to the fixture world (U5). */
void WorldCreators()
{
	const auto T0 = std::chrono::steady_clock::now();
	ss::world::World W;
	W.Create(1234u, 1440.0 + 127.0);
	W.Simulate(30);
	const double World = W.Clock();
	const std::vector<video::WorldVideo> Vs = video::WorldVideos(W, World, 60);
	if (PrintWorld)
	{
		for (const video::WorldVideo& V : Vs)
		{
			std::printf("  %-18s %8.0f views  arch %2d  %s\n", V.Creator.c_str(), V.Views, static_cast<int>(V.Spec.Arch), V.Title.c_str());
		}
	}
	// What the world knows: its log's events, stakes, amounts, places and years, its people's names; the weeks of the year.
	std::set<std::string> Known;
	auto Learn = [&Known](const std::string& S) {
		for (const std::string& Run : DigitRuns(S))
		{
			Known.insert(Run);
		}
	};
	for (const ss::world::WorldEvent& E : W.Events())
	{
		Learn(E.What);
		Learn(vc::Cash(E.Amount));
		Learn(std::to_string(E.Value));
		Learn(std::to_string(ss::world::YearOf(static_cast<int>(E.At / Day))));
		if (const ss::world::Npc* N = W.Get(E.Npc))
		{
			Learn(N->Name);
		}
		if (const ss::world::Npc* N = W.Get(E.Other))
		{
			Learn(N->Name);
		}
	}
	for (int Wk = 1; Wk <= 53; ++Wk)
	{
		Known.insert(std::to_string(Wk));
	}
	std::set<std::string> Creators = {"Overlay Weekly"};
	for (const ss::kast::Streamer& S : ss::kast::Directory())
	{
		Creators.insert(S.Name);
	}
	int Bad = 0, Digits = 0, Strangers = 0, Late = 0, News = 0;
	std::set<std::string> Seen;
	for (const video::WorldVideo& V : Vs)
	{
		Seen.insert(V.Creator);
		Strangers += Creators.count(V.Creator) ? 0 : 1;
		Late += V.At > World || V.Views < 0 ? 1 : 0;
		News += V.Title.find("Overlay Weekly") == 0 || V.Spec.Arch == ss::video::thumb::ArReactionBox ? 1 : 0;
		const std::string Why = copylint::Check(V.Title + ".", 101, {});
		if (V.Title.empty() || V.Title.size() > 100 || !Why.empty() || SaysAutoplay(V.Title) || Brand(V.Title))
		{
			++Bad;
			std::printf("  world video: %s: \"%s\" %s\n", V.Creator.c_str(), V.Title.c_str(), Why.c_str());
		}
		for (const std::string& Run : DigitRuns(V.Title))
		{
			if (!Known.count(Run))
			{
				++Digits;
				std::printf("  world video digit '%s' isn't the world's: %s \"%s\"\n", Run.c_str(), V.Creator.c_str(), V.Title.c_str());
				break;
			}
		}
	}
	// A blank world has no news yet: its creators post only their own shows.
	ss::world::World Blank;
	Blank.Create(99u, 1440.0 + 127.0, ss::world::Start::Blank);
	int BlankNews = 0;
	for (const video::WorldVideo& V : video::WorldVideos(Blank, Blank.Clock() + 2.0 * Day, 60))
	{
		BlankNews += V.Title.find("Overlay Weekly") == 0 ? 1 : 0;
		for (const ss::world::WorldEvent& E : Blank.Events())
		{
			const ss::world::Npc* N = Blank.Get(E.Npc);
			BlankNews += N && V.Title.find(N->Name) != std::string::npos ? 1 : 0;
		}
	}
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	const auto C0 = std::chrono::steady_clock::now();
	for (int I = 0; I < 200; ++I)
	{
		(void)video::WorldVideos(W, World + static_cast<double>(I), 9); // Home's nine, cached for the day
	}
	const double Warm = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - C0).count() / 200.0;
	std::printf("  world creators: %zu videos from %zu channels (%d news), a world of 30 days in %.0f ms; Home's nine warm %.3f ms\n", Vs.size(), Seen.size(), News,
		Ms, Warm);
	Expect(Vs.size() >= 20 && Seen.size() >= 10, "world creators: a month's videos from most of the 13 channels (" + std::to_string(Vs.size()) + ", " +
		std::to_string(Seen.size()) + ")");
	Expect(Strangers == 0 && Late == 0, "world creators: only the directory's 12 and Overlay Weekly, nothing not yet up");
	Expect(Bad == 0, "world creators: every title linted, U1, no brand, 100 characters at most (" + std::to_string(Bad) + ")");
	Expect(Digits == 0, "world creators: every number in a title is the world's (" + std::to_string(Digits) + ")");
	Expect(BlankNews == 0, "world creators: a blank world shows only what happened (no news yet)");
	Expect(Warm < 0.2, "world creators: Home's nine from the day's cache under 0.2 ms");
}

bool PrintComments = false; // "comments": the first twelve of each kind of video, for review

void Comments(const Fixture& A)
{
	// Six kinds of video, round the fixture's 60: storytime, a live night, an online hand with a named player (heavy ads, a
	// title over its payoff), the rival, a series episode, a stream moment.
	int Made = 0, Bad = 0, Rude = 0, Names = 0, Digits = 0, Kinds = 0;
	std::set<int> Archetypes;
	for (size_t N = 0; N < A.C.Videos.size(); ++N)
	{
		video::Video V = A.C.Videos[N];
		V.X.On = true;
		V.X.Fate = ss::Fnv1a(V.Id);
		V.LengthMin = 8.5;
		V.Retention = 0.5;
		V.Th = video::Thumb();
		switch (N % 6)
		{
		case 0:
			V.K = video::Kind::BadBeat;
			V.Title = "a back-room game, a regular, and the worst river of my life";
			V.Th.Mood = 12;
			V.Th.Cards = "AsAd/7c7h/Kd8s2c9h7d";
			break;
		case 1:
			V.K = video::Kind::DeepRun;
			V.Title = "a night at the Embercrest card room: the Embercrest Nightly";
			V.Th.Mood = 11;
			V.Th.Opponent = "Mike_R";
			break;
		case 2:
			V.K = video::Kind::BadBeat;
			V.Title = "kings, 82% to win, and then the nine of hearts";
			V.Th.Cards = "KsKh/Ad9d/9c8s2h7c9h";
			V.Th.Opponent = "Mike_R";
			V.X.Ads = video::AdLoad::Heavy;
			V.X.Promise = 3;
			V.X.Payoff = 1;
			break;
		case 3:
			V.K = video::Kind::Rival;
			V.Title = "gh0stfold again. every pot we've played";
			V.Th.Opponent = ss::RivalName;
			break;
		case 4:
			V.K = video::Kind::Idea;
			V.X.Series = 2;
			V.Title = "$50 to $500 on Ignite Poker, episode 4: $86";
			break;
		default:
			V.K = video::Kind::StreamMoment;
			V.X.KastShare = 0.2;
			V.Title = "64 big blinds on one card, live on stream";
			V.Th.BigText = "64 BB";
			break;
		}
		const bool Storytime = N % 6 == 0;
		std::set<std::string> Known;
		for (const std::string& Fact : vc::CommentFactsOf(V, A.X.World))
		{
			for (const std::string& Run : DigitRuns(Fact))
			{
				Known.insert(Run);
			}
		}
		++Kinds;
		for (int Chunk = 0; Chunk < 10 && Made < 12000; ++Chunk)
		{
			const std::vector<video::Comment> Cs = video::CommentsOf(V, A.X.World, Chunk * 20, 20);
			int Picky = 0;
			for (size_t I = 0; I < Cs.size(); ++I)
			{
				const video::Comment& C = Cs[I];
				++Made;
				Archetypes.insert(C.Archetype);
				if (PrintComments && N < 6 && Chunk == 0 && I < 12)
				{
					std::printf("  [%zu] %-2d %-22s %4d  %s\n", N, C.Archetype, C.Author.c_str(), C.Likes, C.Text.c_str());
				}
				const std::string Why = copylint::Check(C.Text + (C.Text.empty() || std::string(".!?\"'").find(C.Text.back()) == std::string::npos ? "." : ""), 200, {});
				if (C.Text.empty() || !Why.empty() || SaysAutoplay(C.Text) || SaysAutoplay(C.Author) || Brand(C.Text) || Brand(C.Author) ||
					C.Text.find("http") != std::string::npos || C.Text.find("www") != std::string::npos)
				{
					++Bad;
					if (Bad < 10)
					{
						std::printf("  comment: %s: \"%s\" %s\n", C.Author.c_str(), C.Text.c_str(), Why.c_str());
					}
				}
				// Storytime names nobody; elsewhere a named player signs only "was in the hand", and nobody else's name appears.
				const bool Named = C.Author == "Mike_R" || C.Text.find("Mike_R") != std::string::npos;
				if ((Storytime && (Named || C.Author.find("gh0st") != std::string::npos || C.Text.find("gh0st") != std::string::npos)) ||
					(Named && C.Archetype != 7))
				{
					++Names;
					std::printf("  comment names someone: %s: \"%s\" (video %zu)\n", C.Author.c_str(), C.Text.c_str(), N);
				}
				// Every digit a fact (the handle has none).
				std::string Handle = C.Author == ss::RivalName || C.Author == "totally_not_gh0stfold" ? std::string() : C.Author;
				for (const std::string& Run : DigitRuns(C.Text + " " + Handle))
				{
					if (!Known.count(Run))
					{
						++Digits;
						std::printf("  comment digit '%s' isn't a fact: \"%s\" (video %zu)\n", Run.c_str(), C.Text.c_str(), N);
						break;
					}
				}
				Picky += C.Archetype == 2 || C.Archetype == 11 || C.Archetype == 13 ? 1 : 0;
				if ((I + 1) % 10 == 0)
				{
					Rude += Picky > 1 ? 1 : 0;
					Picky = 0;
				}
			}
		}
	}
	(void)Kinds;
	std::printf("  comments: %d made, %zu archetypes\n", Made, Archetypes.size());
	Expect(Made >= 10000 || Made == 0, "comments: 10,000 generated (" + std::to_string(Made) + ")");
	Expect(Made > 0 && Archetypes.size() >= 18, "comments: the archetypes say something (" + std::to_string(Archetypes.size()) + " of 20 seen)");
	Expect(Bad == 0, "comments: every comment linted, U1, no brand, no link (" + std::to_string(Bad) + ")");
	Expect(Names == 0, "comments: no name in storytime, and a named player only signs his own hand (" + std::to_string(Names) + ")");
	Expect(Digits == 0, "comments: every digit a fact (" + std::to_string(Digits) + ")");
	Expect(Rude == 0, "comments: at most one results-oriented, nit or gh0stfold comment among any 10 shown (" + std::to_string(Rude) + ")");
	// Any ten in a row, not only the aligned ones.
	{
		video::Video V = A.C.Videos.front();
		V.X.On = true;
		V.X.Fate = 7;
		V.K = video::Kind::Rival;
		V.Th.Opponent = ss::RivalName;
		V.Th.Cards = "AsAd/7c7h/Kd8s2c9h7d";
		const std::vector<video::Comment> Cs = video::CommentsOf(V, A.X.World, 0, 400);
		int Worst = 0;
		for (size_t I = 0; I + 10 <= Cs.size(); ++I)
		{
			int P = 0;
			for (size_t J = I; J < I + 10; ++J)
			{
				P += Cs[J].Archetype == 2 || Cs[J].Archetype == 11 || Cs[J].Archetype == 13 ? 1 : 0;
			}
			Worst = std::max(Worst, P);
		}
		Expect(Worst <= 1, "comments: the rude cap holds over any window of ten (" + std::to_string(Worst) + ")");
	}
}
/**
 * The review's checks (review-core-videocontent): the Session's IdeaCtx reads what the save keeps (the ledger, the coach,
 * the channel's day rows, the gear) so the ideas that need it unlock in play, not only on a hand-built fixture; a locked
 * series declares nothing; Road to the Final Table's Topic reads the peak room; the launch actions keep Publish's refusals;
 * a title repackage moves the CTR and the promise; a thumbnail never lies (a trophy for a win only, BOUNTY on a bounty,
 * PAYBACK on a debt, no read claimed on a caught bluff, CRACKED only for a hand that was ahead); comments commiserate only
 * over a pot the player lost, blame the card that turned it, and keep hand talk off a video with no hand.
 */
void ReviewSession();

void ReviewChecks(const Fixture& A)
{
	// ---- The stable picker's window is the last 20 titles exactly: a template in the 20th newest kept video stays out, one
	// only in the 21st comes back.
	{
		video::Channel C = A.C;
		const video::Choice Ch = One(C.Open.front().Id, video::Format::Breakdown, video::TitleStyle::Story);
		const std::vector<video::TitleOption> First = video::TitleOptions(Ch, C.Open, C, 0, 3);
		std::vector<video::Video*> Kept;
		for (video::Video& V : C.Videos)
		{
			Kept.push_back(&V);
		}
		std::sort(Kept.begin(), Kept.end(), [](const video::Video* X, const video::Video* Y) { return X->At > Y->At; });
		bool In20 = false, Back21 = false;
		if (!First.empty() && Kept.size() > 21)
		{
			for (video::Video* V : Kept)
			{
				V->X.TitleTpl.clear();
			}
			Kept[19]->X.TitleTpl = First.front().Key;
			In20 = true;
			for (int S = 0; S < 40; ++S)
			{
				for (const video::TitleOption& O : video::TitleOptions(Ch, C.Open, C, S, 3))
				{
					In20 = In20 && O.Key != First.front().Key;
				}
			}
			Kept[19]->X.TitleTpl.clear();
			Kept[20]->X.TitleTpl = First.front().Key;
			for (const video::TitleOption& O : video::TitleOptions(Ch, C.Open, C, 0, 3))
			{
				Back21 = Back21 || O.Key == First.front().Key;
			}
		}
		Expect(In20 && Back21, "review: the picker skips the 20th newest title's template and offers the 21st's again");
	}
	// ---- A title's {KillerCard} is the card that turned the hand (a turn beat names the turn's ace, not the river's three).
	{
		video::Candidate Turn = A.C.Open.front();
		Turn.Id = "rv:turnbeat";
		Turn.Rec.Mine = ss::ParseCards("KsKh");
		Turn.Rec.Shown = {{Turn.Opponent, ss::ParseCards("Ad9d")}};
		Turn.Rec.Board = ss::ParseCards("9c8s2hAc3d");
		Turn.Rec.HeroNet = -11600;
		const vc::Facts F = vc::FactsOf(One(Turn.Id, video::Format::Breakdown, video::TitleStyle::Story), {Turn}, Turn, &A.C);
		const std::string Kc = F.V.count("KillerCard") ? F.V.at("KillerCard") : std::string();
		Expect(Kc == "the ace of clubs", "review: the killer card is the turn's (" + Kc + ")");
	}
	// ---- The edit's cost: Clean edit's 15 minutes, the length option's edit, a Duel's contenders.
	{
		video::Choice Ch = One("x", video::Format::Breakdown, video::TitleStyle::Hype);
		Ch.TitleTpl = "badbeat.loud.1";
		Ch.Thumbs.resize(1);
		const double Base = video::EditMinutesOf(Ch), Energy = video::EditEnergyOf(Ch);
		Ch.CleanEdit = true;
		const double Clean = video::EditMinutesOf(Ch);
		Ch.CleanEdit = false;
		Ch.Len = video::LengthOpt::Extended;
		const double Long = video::EditMinutesOf(Ch);
		Expect(Base == 90.0 && Energy == 8.0 && Clean == Base + video::valgo::VaCleanEditMinutes && Long == 120.0,
			"review: the edit's minutes: Standard 90, Clean edit +15, Extended 120 (" + ss::Fixed(Base, 0) + ", " + ss::Fixed(Clean, 0) + ", " + ss::Fixed(Long, 0) + ")");
	}
	ReviewSession();
}

void ReviewSession()
{
	struct Hooks : ss::SessionHooks
	{
		void Save(const ss::SaveData& D) override { Last = D; }
		ss::SaveData Last;
	};
	auto Wait = [](ss::Session& S, double& Now, double Seconds) {
		for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
		}
	};
	// ---- The Session's IdeaCtx: the ledger, the coach, the day rows, the gear.
	{
		Hooks H;
		ss::Session S(H, "rerun-content-review-ctx");
		double Now = 0.0;
		S.CurrentScreen = ss::Screen::Lobby;
		Wait(S, Now, 0.2);
		const double World = S.WorldMinutes();
		const int Today = static_cast<int>(std::floor(World / Day));
		S.BankrollCents = 500000;
		Expect(S.PayRent(), "review: the rent is paid (the ledger's line, the rent-paid story)");
		S.Life.Record(World - 30.0, "Wash & Fold shift", 4800, 1);
		for (int I = 0; I < 3; ++I)
		{
			S.Life.Record(World - static_cast<double>(I + 1) * Day, "Lucky Penny #212 shift", 4350, 1);
		}
		S.Gear["webcam-720"] = 0.0;
		S.Gear["webcam-1080"] = 0.0;
		S.Life.Record(World - 60.0, "GearDrop: 1080p60 webcam", -11900, 5);
		S.Life.Record(World - 50.0, "GearDrop: a desk lamp", -9000, 5);
		S.Life.Study.Reviews = 1;
		S.Life.Study.LastReview = World - Day;
		S.Life.Video.Subs = 1040.0;
		for (int D = 7; D >= 1; --D)
		{
			video::DayRow R;
			R.Key = Today - D;
			R.Gained = 12.0f;
			R.Comments = 40.0f;
			S.Life.Video.X.Days.push_back(R);
		}
		std::map<int, bool> Ready;
		for (const video::IdeaCard& I : S.VideoIdeas())
		{
			Ready[I.Id] = I.Ready;
		}
		const std::pair<int, const char*> Need[] = {{505, "E5 rent paid by poker (the ledger's rent line)"}, {611, "F11 coach session (the coach's review this week)"},
			{901, "I1 subscriber milestone (the day rows' week)"}, {906, "I6 reading your comments (the day rows' 30 days)"}, {1102, "K2 night cashier (three shifts in 14 days)"},
			{1103, "K3 Wash & Fold storytime (a shift there)"}, {1106, "K6 setup upgrade ($200 of gear this week)"}, {1110, "K10 camera test (a better webcam)"}};
		for (const auto& N : Need)
		{
			Expect(Ready[N.first], std::string("review: the session's ideas read the save: ") + N.second);
		}
		// A locked series (Climb the Season board: no rank yet) declares nothing.
		const size_t Before = S.Life.Video.X.Challenges.size();
		const std::string Why = S.ChooseIdea(1610);
		Expect(!Why.empty() && S.Life.Video.X.Challenges.size() == Before, "review: a locked series idea declares no series (" + Why + ")");
	}
	// ---- Road to the Final Table's Topic reads the stream's peak (SPEC_CREATORS §3.7).
	{
		video::IdeaCtx X;
		video::Channel C;
		X.World = 200.0 * Day;
		video::Candidate Ft;
		Ft.Id = "rl:ft@1:9";
		Ft.K = video::Kind::FinalTable;
		Ft.At = X.World - 120.0;
		Ft.Event = "Sunday Blaze";
		Ft.Field = 1530;
		Ft.PlaceSoFar = 4;
		Ft.Rec.Pot = 119000;
		Ft.Stream.Viewers = 40;
		Ft.Drama = 0.8;
		C.Open = {Ft};
		video::Candidate Out;
		std::string Why;
		const bool Made = vc::MakeIdeaCandidateOn(X, C, 312, X.World, Out, Why);
		Expect(Made && Out.IdeaFacts.find("w=2.16;") != std::string::npos, "review: Road to the Final Table's Topic is 1.8 x (1 + min(0.5, 40/200)) (" + Out.IdeaFacts.substr(0, 12) + ")");
	}
	// ---- The launch actions keep Publish's refusals; a title repackage moves the CTR and the promise.
	{
		Hooks H;
		ss::Session S(H, "rerun-content-review-glue");
		double Now = 0.0;
		S.CurrentScreen = ss::Screen::Lobby;
		Wait(S, Now, 0.2);
		S.Life.Update3LinesRead |= ss::life::U3Video;
		S.Life.Energy = 90.0;
		S.Life.Hunger = 5.0;
		S.Life.Thirst = 5.0;
		S.DebugVideo("fake badbeat online");
		if (S.Life.Video.Open.empty())
		{
			Expect(false, "review: a moment to publish");
			return;
		}
		video::Choice C = One(S.Life.Video.Open[0].Id, video::Format::Breakdown, video::TitleStyle::Story);
		const std::vector<video::TitleOption> Titles = S.TitleOptions(C, 0);
		const std::vector<video::ThumbSpec> Thumbs = S.ThumbOptions(C, 0);
		if (Titles.empty() || Thumbs.empty())
		{
			Expect(false, "review: options to publish");
			return;
		}
		C.TitleTpl = Titles[0].Key;
		C.Thumbs = {Thumbs[0]};
		Expect(S.Publish(C).empty(), "review: the video publishes");
		Wait(S, Now, 8.0);
		const std::string Id = S.Life.Video.Videos.empty() ? std::string() : S.Life.Video.Videos.back().Id;
		S.Stream.Live = true;
		const std::string Live = S.LaunchAction(Id, video::Launch::Teaser);
		Expect(!Live.empty() && !S.TimeSkip.Active, "review: no launch action's TimeSkip while live on Kast (" + Live + ")");
		S.Stream.Live = false;
		S.Life.Energy = 3.0;
		const std::string Tired = S.LaunchAction(Id, video::Launch::Cut);
		Expect(!Tired.empty() && !S.TimeSkip.Active && S.Life.Energy == 3.0, "review: Make a Cut needs the editor's energy (" + Tired + ")");
		S.Life.Energy = 90.0;
		video::Video* V = nullptr;
		for (video::Video& X : S.Life.Video.Videos)
		{
			V = X.Id == Id ? &X : V;
		}
		if (!V)
		{
			return;
		}
		uint16_t OldTraits = 0;
		for (const vc::Tpl& T : vc::Templates3())
		{
			OldTraits = T.Key == V->X.TitleTpl ? vc::TraitsOf(T, V->Title) : OldTraits;
		}
		const double Ctr0 = V->X.Ctr;
		const int Promise0 = V->X.Promise;
		const video::TitleOption* Pick = nullptr;
		std::vector<video::TitleOption> Options;
		for (int Sh = 0; Sh < 8 && !Pick; ++Sh)
		{
			Options = video::RepackTitles(*V, S.Life.Video, Sh, 3);
			for (const video::TitleOption& O : Options)
			{
				if (!Pick && O.Promise == Promise0 && std::fabs(video::valgo::TitleTraitCtr(O.Traits) / video::valgo::TitleTraitCtr(OldTraits) - 1.0) > 0.005)
				{
					Pick = &O;
				}
			}
		}
		Expect(Pick != nullptr, "review: a repackage title whose traits differ from the shown one's");
		if (Pick)
		{
			video::Repack Rp;
			Rp.TitleKey = Pick->Key;
			const double Want = video::valgo::TitleTraitCtr(Pick->Traits) / video::valgo::TitleTraitCtr(OldTraits);
			const std::string Text = Pick->Text;
			const int Promise = Pick->Promise;
			Expect(S.Repackage(Id, Rp).empty(), "review: the title repackage goes through");
			for (video::Video& X : S.Life.Video.Videos)
			{
				V = X.Id == Id ? &X : V;
			}
			Expect(std::fabs(V->X.Ctr / Ctr0 - Want) < 1e-4 && V->Title == Text && V->X.Promise == Promise,
				"review: a new title moves the CTR by its traits (" + ss::Fixed(V->X.Ctr / Ctr0, 4) + " vs " + ss::Fixed(Want, 4) + ") and keeps its promise");
		}
		// The ad load changes once a week (§4.5.5), and the launch actions live inside the first 48 hours (§4.6.9).
		const bool Heavy = S.SetAdLoad(Id, video::AdLoad::Heavy).empty();
		S.DebugVideo("advance 2");
		const bool Twice = S.SetAdLoad(Id, video::AdLoad::Light).empty();
		const std::string Late = S.LaunchAction(Id, video::Launch::Reply);
		S.DebugVideo("advance 6");
		const bool Week = S.SetAdLoad(Id, video::AdLoad::Light).empty();
		Expect(Heavy && !Twice && Week, "review: the ad load changes once a week (a second change two days later refused, a week later taken)");
		Expect(!Late.empty() && !S.TimeSkip.Active, "review: a launch action after the first 48 hours is refused (" + Late + ")");
		// A Loud video repackaged to a title of another promise: the later check-ins read the new promise.
		S.DebugVideo("fake badbeat online");
		video::Choice L = One(S.Life.Video.Open.empty() ? std::string() : S.Life.Video.Open.back().Id, video::Format::Breakdown, video::TitleStyle::Hype);
		const std::vector<video::TitleOption> LoudTitles = S.TitleOptions(L, 0);
		const std::vector<video::ThumbSpec> LoudThumbs = S.ThumbOptions(L, 0);
		std::string Key2, Id2;
		int Promise2 = -1;
		if (!LoudTitles.empty() && !LoudThumbs.empty())
		{
			L.TitleTpl = LoudTitles[0].Key;
			L.Thumbs = {LoudThumbs[0]};
			if (S.Publish(L).empty())
			{
				Wait(S, Now, 8.0);
				Id2 = S.Life.Video.Videos.back().Id;
				const video::Video& V2 = S.Life.Video.Videos.back();
				for (int Sh = 0; Sh < 48 && Key2.empty(); ++Sh)
				{
					for (const video::TitleOption& O : video::RepackTitles(V2, S.Life.Video, Sh, 3))
					{
						if (Key2.empty() && O.Promise != V2.X.Promise)
						{
							Key2 = O.Key;
							Promise2 = O.Promise;
						}
					}
				}
			}
		}
		Expect(!Key2.empty(), "review: a Loud video's repackage options include another promise");
		if (!Key2.empty())
		{
			video::Repack Rp2;
			Rp2.TitleKey = Key2;
			const std::string Why2 = S.Repackage(Id2, Rp2);
			const video::Video* V2 = nullptr;
			for (const video::Video& X : S.Life.Video.Videos)
			{
				V2 = X.Id == Id2 ? &X : V2;
			}
			Expect(Why2.empty() && V2 && V2->X.Promise == Promise2 && V2->X.TitleTpl == Key2, "review: a repackaged title's promise is the video's from then on (" + Why2 + ")");
		}
		// A day on: Live Drop's window has closed (the card says so), and a video that keeps no hand has nothing to cut.
		if (!Id2.empty())
		{
			S.DebugVideo("advance 1");
			const std::string Drop = S.LaunchAction(Id2, video::Launch::Kast);
			Expect(Drop.find("6 hours") != std::string::npos && Drop.find("needed") != std::string::npos, "review: Live Drop's window closed after 6 hours (" + Drop + ")");
			video::Video* V2 = nullptr;
			for (video::Video& X : S.Life.Video.Videos)
			{
				V2 = X.Id == Id2 ? &X : V2;
			}
			if (V2)
			{
				const std::string Cards = V2->Th.Cards;
				V2->Th.Cards.clear();
				const double Energy = S.Life.Energy;
				const std::string NoHand = S.LaunchAction(Id2, video::Launch::Cut);
				Expect(!NoHand.empty() && !S.TimeSkip.Active && S.Life.Energy == Energy, "review: no Cut, no time and no energy spent on a video with no hand (" + NoHand + ")");
				V2->Th.Cards = Cards;
			}
		}
	}
	// ---- Thumbnails never lie.
	{
		auto HandC = [](const char* Id, video::Kind K, const char* Mine, const char* Theirs, const char* Board, Chips Net, double Eq) {
			video::Candidate C;
			C.Id = Id;
			C.At = 40.0 * Day;
			C.K = K;
			C.Event = "Night Owl Turbo";
			C.BuyIn = 550;
			C.Field = 104;
			C.Rec.Number = 214;
			C.Rec.Blinds.Sb = 100;
			C.Rec.Blinds.Bb = 200;
			C.Rec.Mine = ss::ParseCards(Mine);
			C.Rec.Board = ss::ParseCards(Board);
			if (Theirs[0])
			{
				C.Rec.Shown.push_back({"Mike_R", ss::ParseCards(Theirs)});
			}
			C.Opponent = "Mike_R";
			C.Rec.Lines.push_back({ss::Street::Preflop, "You", "all in", 6400});
			C.Rec.Pot = 12900;
			C.Rec.HeroNet = Net;
			C.EquityAtAllIn = Eq;
			C.Drama = 0.6;
			return C;
		};
		video::Candidate Hu = HandC("rv:hu", video::Kind::HeadsUp, "AsKd", "QcQh", "2c7d9h3s4c", -9000, 0.46);
		Hu.PlaceSoFar = 2;
		Hu.Idea = 122;
		Hu.IdeaFacts = vc::MomentIdeaFacts(122);
		const video::Candidate Ko = HandC("rv:ko", video::Kind::Knockout, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, 0.8);
		const video::Candidate Bc = HandC("rv:bc", video::Kind::BluffCaught, "6s5s", "KcTd", "KhQd2cJs8h", -7200, -1.0);
		int Trophy = 0, Bounty = 0, Payback = 0, Read = 0, Cracked = 0;
		const video::Candidate* const Leads[] = {&Hu, &Ko, &Bc};
		for (const video::Candidate* M : Leads)
		{
			video::Channel Ch;
			Ch.Open = {*M};
			for (int Sh = 0; Sh < 40; ++Sh)
			{
				for (const video::ThumbSpec& T : video::ThumbOptions(One(M->Id, video::Format::Breakdown, video::TitleStyle::Story), Ch.Open, Ch, Sh, 6))
				{
					const std::string Cap = T.Caption + " " + video::ThumbCaption2(T);
					Trophy += T.Arch == ss::video::thumb::ArTrophyShot ? 1 : 0;
					Bounty += Cap.find("BOUNTY") != std::string::npos ? 1 : 0;
					Payback += Cap.find("PAYBACK") != std::string::npos ? 1 : 0;
					Read += M == &Bc && (Cap.find("I KNEW") != std::string::npos || Cap.find("READ IT") != std::string::npos) ? 1 : 0;
					Cracked += M == &Bc && Cap.find("CRACKED") != std::string::npos ? 1 : 0;
				}
			}
		}
		Expect(Trophy == 0, "review: no trophy on a heads-up the player lost, even under its idea (" + std::to_string(Trophy) + ")");
		Expect(Bounty == 0 && Payback == 0, "review: no BOUNTY on a freezeout's knockout, no PAYBACK without a debt (" + std::to_string(Bounty) + ", " + std::to_string(Payback) + ")");
		Expect(Read == 0 && Cracked == 0, "review: a caught bluff claims no read and nothing CRACKED (" + std::to_string(Read) + ", " + std::to_string(Cracked) + ")");
	}
	// ---- Comments: sympathy only for a pot lost, the card that turned it, no hand talk without a hand.
	{
		auto Vid = [](const char* Id, video::Kind K, const char* Cards, const char* Title) {
			video::Video V;
			V.Id = Id;
			V.K = K;
			V.At = 40.0 * Day;
			V.X.On = true;
			V.X.Fate = ss::Fnv1a(Id);
			V.LengthMin = 8.0;
			V.Retention = 0.5;
			V.Title = Title;
			V.Th.Cards = Cards;
			return V;
		};
		int Sorry = 0, Blame = 0, Turned = 0, HandTalk = 0;
		const video::Video Won = Vid("rv:won", video::Kind::Monster, "9s9h/AcKd/9c9d2hKs4c", "quads on the turn, 64 big blinds");
		const video::Video Turn = Vid("rv:turn", video::Kind::BadBeat, "KsKh/Ad9d/9c8s2hAc3d", "kings, 82% to win, and then the turn");
		video::Video Tour = Vid("rv:tour", video::Kind::Idea, "", "Poker setup tour: 2 monitors, the Lumen 4K");
		Tour.X.Idea = 1105;
		for (const video::Comment& C : video::CommentsOf(Won, 45.0 * Day, 0, 400))
		{
			Sorry += C.Archetype == 0 || C.Archetype == 2 || C.Archetype == 6 ? 1 : 0;
		}
		for (const video::Comment& C : video::CommentsOf(Turn, 45.0 * Day, 0, 400))
		{
			Blame += C.Text.find("three of diamonds") != std::string::npos || C.Text.find("river card") != std::string::npos || C.Text.find("no more rivers") != std::string::npos ? 1 : 0;
			Turned += C.Text.find("ace of clubs") != std::string::npos ? 1 : 0;
		}
		for (const video::Comment& C : video::CommentsOf(Tour, 45.0 * Day, 0, 400))
		{
			HandTalk += C.Archetype == 0 || C.Archetype == 1 || C.Archetype == 2 || C.Archetype == 3 || C.Archetype == 6 || C.Archetype == 10 || C.Archetype == 11 ? 1 : 0;
		}
		Expect(Sorry == 0, "review: no sympathy, joke or results talk over a pot the player won (" + std::to_string(Sorry) + " of 400)");
		Expect(Blame == 0 && Turned > 0, "review: a turn beat's comments blame the turn's card, never the river's (" + std::to_string(Blame) + ", " + std::to_string(Turned) + ")");
		Expect(HandTalk == 0, "review: a setup tour gets no hand talk (" + std::to_string(HandTalk) + " of 400)");
		// The player who was in the hand comments once, not on every page.
		video::Video Named = Vid("rv:named", video::Kind::HeroCall, "Td9d/AsQs/Th6c3s2h8c", "ten-nine calls the river");
		Named.Th.Opponent = "Mike_R";
		int Signed = 0;
		for (const video::Comment& C : video::CommentsOf(Named, 45.0 * Day, 0, 200))
		{
			Signed += C.Author == "Mike_R" ? 1 : 0;
		}
		Expect(Signed == 1, "review: the named player signs one comment in 200 (" + std::to_string(Signed) + ")");
	}
}

// ================================================================== the C3 fix round (c3f-content): Studio's pickers (M1),
// Research's searches (M2), articles before amounts (L3), a Cut from an idea video (L8)

/** Two recipes the same in every saved field. */
bool SameSpec(const video::ThumbSpec& A, const video::ThumbSpec& B)
{
	return A.Arch == B.Arch && A.Layout == B.Layout && A.Mirror == B.Mirror && A.Bg == B.Bg && A.Pal == B.Pal && A.Subj == B.Subj && A.Expr == B.Expr && A.Pose == B.Pose &&
		A.PropA == B.PropA && A.PropB == B.PropB && A.CapStyle == B.CapStyle && A.Sticker == B.Sticker && A.Corner == B.Corner && A.LookRef == B.LookRef &&
		A.Variant == B.Variant && A.Caption == B.Caption && A.Caption2 == B.Caption2;
}

/** The first lone "a"/"an" before a number read the other way ("a $11", "an $5.50"), "" when none. */
std::string BadArticle(const std::string& T)
{
	for (size_t I = 0; I < T.size(); ++I)
	{
		if (!std::isalpha(static_cast<unsigned char>(T[I])) || (I > 0 && (std::isalnum(static_cast<unsigned char>(T[I - 1])) || T[I - 1] == '\'' || T[I - 1] == '-')))
		{
			continue;
		}
		size_t J = I;
		while (J < T.size() && std::isalpha(static_cast<unsigned char>(T[J])))
		{
			++J;
		}
		const std::string W = Lower(T.substr(I, J - I));
		if ((W == "a" || W == "an") && J + 1 < T.size() && T[J] == ' ' && (T[J + 1] == '$' || std::isdigit(static_cast<unsigned char>(T[J + 1]))))
		{
			size_t E = J + 1;
			while (E < T.size() && T[E] != ' ')
			{
				++E;
			}
			if (vc::TakesAn(T.substr(J + 1, E - J - 1)) != (W == "an"))
			{
				return T.substr(I, E - I);
			}
		}
		I = J;
	}
	return std::string();
}

void C3Pickers(const Fixture& A)
{
	using namespace video::thumb;
	// ---- Every dealt recipe is within its archetype's values; its own values are among its choices; every allowed edit
	// stays within, changes only its own field (and the compositor's facts as a dealt recipe of that palette carries them),
	// comes out the same twice; a value outside the choices changes nothing and a recipe carrying it is refused.
	int Recipes = 0, Outside = 0, Own = 0, Edits = 0, EditsOk = 0, Wrong = 0, WrongKept = 0, WrongRefused = 0, Forgeries = 0, FacelessArchs = 0;
	std::string FirstBad;
	std::set<int> Archs;
	for (const video::Candidate& M : A.C.Open)
	{
		const bool Run = M.K == video::Kind::DeepRun || M.K == video::Kind::FinalTable || M.K == video::Kind::Win;
		const video::Format F = Run ? video::Format::Recap : video::Format::Breakdown;
		for (int Shuffle = 0; Shuffle < 3; ++Shuffle)
		{
			const video::Choice Ch = One(M.Id, F, video::TitleStyle::Story);
			for (const video::ThumbSpec& S : video::ThumbOptions(Ch, A.C.Open, A.C, Shuffle, 6))
			{
				++Recipes;
				Archs.insert(S.Arch);
				if (!video::ThumbWithin(S, Ch, A.C.Open, A.C))
				{
					++Outside;
					FirstBad = FirstBad.empty() ? M.Id + " " + video::ArchName(S.Arch) : FirstBad;
					continue;
				}
				const video::ThumbChoices Ok = video::ThumbChoicesOf(S, Ch, A.C.Open, A.C);
				const auto In = [](const std::vector<int16_t>& L, int V) { return std::find(L.begin(), L.end(), static_cast<int16_t>(V)) != L.end(); };
				const bool OwnIn = In(Ok.Pals, S.Pal) && In(Ok.Exprs, S.Expr) && In(Ok.CapStyles, S.CapStyle) &&
					std::find(Ok.Captions.begin(), Ok.Captions.end(), std::make_pair(S.Caption, video::ThumbCaption2(S))) != Ok.Captions.end();
				Own += OwnIn ? 1 : 0;
				FacelessArchs += Ok.Exprs.empty() ? 1 : 0;
				auto Try = [&](int Field, int Value, bool Allowed) {
					const video::ThumbSpec E = video::ThumbEdit(S, Ch, A.C.Open, A.C, Field, Value);
					if (!Allowed)
					{
						++Wrong;
						WrongKept += SameSpec(E, S) ? 1 : 0;
						// (A recipe carrying it, forged past the pickers, is refused: checked on the first deal of each moment.)
						if (Shuffle == 0)
						{
							video::ThumbSpec Forged = S;
							(Field == video::TfPalette ? Forged.Pal : Field == video::TfFace ? Forged.Expr : Forged.CapStyle) = static_cast<int16_t>(Value);
							++Forgeries;
							WrongRefused += !video::ThumbWithin(Forged, Ch, A.C.Open, A.C) ? 1 : 0;
						}
						return;
					}
					++Edits;
					video::ThumbSpec Rest = E, Was = S;
					Rest.Pal = Was.Pal = 0;
					Rest.Expr = Was.Expr = 0;
					Rest.CapStyle = Was.CapStyle = 0;
					Rest.Caption = Was.Caption = "";
					Rest.Caption2 = Was.Caption2 = "";
					const bool Field1 = Field == video::TfPalette ? E.Pal == Value : Field == video::TfFace ? E.Expr == Value : Field == video::TfCapStyle ? E.CapStyle == Value :
						(std::make_pair(E.Caption, video::ThumbCaption2(E)) == Ok.Captions[static_cast<size_t>(Value)]);
					const bool Others = SameSpec(Rest, Was) && (Field == video::TfPalette || E.Pal == S.Pal) && (Field == video::TfFace || E.Expr == S.Expr) &&
						(Field == video::TfCapStyle || E.CapStyle == S.CapStyle) && (Field == video::TfCaption || (E.Caption == S.Caption && video::ThumbCaption2(E) == video::ThumbCaption2(S)));
					const bool Twice = SameSpec(E, video::ThumbEdit(S, Ch, A.C.Open, A.C, Field, Value));
					EditsOk += video::ThumbWithin(E, Ch, A.C.Open, A.C) && Field1 && Others && Twice ? 1 : 0;
				};
				for (int P = 0; P < PlCount; ++P)
				{
					Try(video::TfPalette, P, In(Ok.Pals, P));
				}
				for (int X = 0; X < ExCount; ++X)
				{
					Try(video::TfFace, X, In(Ok.Exprs, X));
				}
				for (int Cs = 0; Cs < CsCount; ++Cs)
				{
					Try(video::TfCapStyle, Cs, In(Ok.CapStyles, Cs));
				}
				for (size_t K = 0; K < Ok.Captions.size(); ++K)
				{
					Try(video::TfCaption, static_cast<int>(K), true);
				}
				// (A caption index past the list changes nothing.)
				Wrong += 1;
				WrongKept += SameSpec(video::ThumbEdit(S, Ch, A.C.Open, A.C, video::TfCaption, static_cast<int>(Ok.Captions.size())), S) ? 1 : 0;
			}
		}
	}
	Expect(Recipes >= 300 && Outside == 0, "pickers: every dealt recipe is within its archetype's values (" + std::to_string(Recipes) + " recipes, " + std::to_string(Outside) + " outside" +
		(FirstBad.empty() ? std::string() : ": " + FirstBad) + ")");
	Expect(Own == Recipes - Outside && FacelessArchs == 0, "pickers: each recipe's own palette, face, caption style and caption are among its choices, and every P0 archetype has a face to pick");
	Expect(Edits > 3000 && EditsOk == Edits, "pickers: every allowed edit stays within, changes its own field only and comes out the same twice (" + std::to_string(EditsOk) + " of " + std::to_string(Edits) + ")");
	Expect(Wrong > 1000 && WrongKept == Wrong && Forgeries > 1000 && WrongRefused == Forgeries, "pickers: a value outside the choices changes nothing, and a recipe carrying one is refused (" +
		std::to_string(WrongKept) + " of " + std::to_string(Wrong) + ", " + std::to_string(WrongRefused) + " of " + std::to_string(Forgeries) + ")");
	std::printf("  pickers: %d recipes over %zu archetypes, %d edits within, %d outside values refused\n", Recipes, Archs.size(), EditsOk, WrongRefused);
	// ---- A stream moment keeps the face the stream showed; storytime's recipes never let a second face in.
	{
		bool Stream = false, StreamOk = true;
		for (const video::Candidate& M : A.C.Open)
		{
			if (M.Stream.Viewers <= 0 || M.K != video::Kind::StreamMoment)
			{
				continue;
			}
			const video::Choice Ch = One(M.Id, video::Format::Breakdown, video::TitleStyle::Story);
			for (int Shuffle = 0; Shuffle < 6; ++Shuffle)
			{
				for (const video::ThumbSpec& S : video::ThumbOptions(Ch, A.C.Open, A.C, Shuffle, 6))
				{
					const video::ThumbChoices Ok = video::ThumbChoicesOf(S, Ch, A.C.Open, A.C);
					const bool Has = std::find(Ok.Exprs.begin(), Ok.Exprs.end(), static_cast<int16_t>(ExLaugh)) != Ok.Exprs.end(); // the clip's face (kast::Mood 5)
					Stream = Stream || Has;
					StreamOk = StreamOk && (!Has || Ok.Exprs.size() == 1);
				}
			}
		}
		Expect(Stream && StreamOk, "pickers: a stream moment's face is the one the stream showed (the only face offered where the archetype has it)");
	}
	// ---- Studio deals before a series is picked and PUBLISH carries it: a recipe dealt without the series stays within with it,
	// and its edits too (a run's Before -> After reads the series' start once there is one).
	{
		Fixture Fs = A;
		Fs.X.Chan = &Fs.C;
		video::Challenge Ser;
		Ser.Id = 7;
		Ser.Kind = 1601;
		Ser.StartedAt = Fs.X.World - 10.0 * Day;
		Ser.Episodes = 3;
		Ser.Start = 5000;
		Ser.Goal = 50000;
		Fs.C.X.Challenges.push_back(Ser);
		int Dealt = 0, Ok = 0, SerEdits = 0, SerEditsOk = 0;
		for (const video::Candidate& M : Fs.C.Open)
		{
			const bool Run = M.K == video::Kind::FinalTable || M.K == video::Kind::DeepRun || M.K == video::Kind::Win;
			const video::Choice Ch = One(M.Id, Run ? video::Format::Recap : video::Format::Breakdown, video::TitleStyle::Story);
			video::Choice WithSeries = Ch;
			WithSeries.Series = Ser.Id;
			for (int Shuffle = 0; Shuffle < 2; ++Shuffle)
			{
				for (const video::ThumbSpec& S : video::ThumbOptions(Ch, Fs.C.Open, Fs.C, Shuffle, 6))
				{
					++Dealt;
					Ok += video::ThumbWithin(S, WithSeries, Fs.C.Open, Fs.C) ? 1 : 0;
					for (const int16_t P : video::ThumbChoicesOf(S, WithSeries, Fs.C.Open, Fs.C).Pals)
					{
						++SerEdits;
						SerEditsOk += video::ThumbWithin(video::ThumbEdit(S, WithSeries, Fs.C.Open, Fs.C, video::TfPalette, P), WithSeries, Fs.C.Open, Fs.C) ? 1 : 0;
					}
				}
			}
		}
		Expect(Dealt > 100 && Ok == Dealt && SerEdits > 300 && SerEditsOk == SerEdits, "pickers: a recipe dealt before the series was picked, and its edits, stay within when PUBLISH names the series (" +
			std::to_string(Ok) + " of " + std::to_string(Dealt) + ", " + std::to_string(SerEditsOk) + " of " + std::to_string(SerEdits) + ")");
	}
	// ---- A choice that picks nothing, a Cut, or an archetype the moment doesn't allow: no choices, no edit, refused.
	{
		const video::Candidate& M = A.C.Open.front(); // a bad beat
		const video::Choice Ch = One(M.Id, video::Format::Breakdown, video::TitleStyle::Story);
		const std::vector<video::ThumbSpec> Deal = video::ThumbOptions(Ch, A.C.Open, A.C, 0, 6);
		video::ThumbSpec Trophy = Deal.empty() ? video::ThumbSpec() : Deal.front();
		Trophy.Arch = ArTrophyShot; // a bad beat won nothing
		const video::Choice Cut = One(M.Id, video::Format::Short, video::TitleStyle::Story);
		Expect(!Deal.empty() && !video::ThumbWithin(Trophy, Ch, A.C.Open, A.C) && !video::ThumbWithin(Deal.front(), Cut, A.C.Open, A.C) &&
				   video::ThumbChoicesOf(Deal.front(), Cut, A.C.Open, A.C).Pals.empty() && SameSpec(video::ThumbEdit(Deal.front(), Cut, A.C.Open, A.C, video::TfPalette, PlHeat), Deal.front()),
			"pickers: a trophy on a bad beat is refused; a Cut has no thumbnail to edit");
	}
	// ---- The compositor's facts are the moment's (review-c3f-content's mutant): a dealt recipe whose fact block is changed
	// (another opponent's name, other cards, a fact the archetype doesn't draw) is refused, its words and fields untouched.
	{
		int Forged = 0, Refused = 0;
		for (const video::Candidate& M : A.C.Open)
		{
			const bool Run = M.K == video::Kind::DeepRun || M.K == video::Kind::FinalTable || M.K == video::Kind::Win;
			const video::Choice Ch = One(M.Id, Run ? video::Format::Recap : video::Format::Breakdown, video::TitleStyle::Story);
			for (const video::ThumbSpec& S : video::ThumbOptions(Ch, A.C.Open, A.C, 0, 6))
			{
				const std::string Line2 = video::ThumbCaption2(S);
				for (const char* Fake : {"o=Somebody Else", "c=AsAd/KsKd/2c3c4c", "s=99"})
				{
					video::ThumbSpec F = S;
					F.Caption2 = Line2 + video::thumb::FactSep + Fake;
					++Forged;
					Refused += !video::ThumbWithin(F, Ch, A.C.Open, A.C) ? 1 : 0;
				}
			}
		}
		Expect(Forged > 300 && Refused == Forged, "pickers: a recipe whose compositor facts aren't the moment's is refused (" + std::to_string(Refused) + " of " + std::to_string(Forged) + ")");
	}
}

void C3Searches(const Fixture& A, const Fixture& B)
{
	const std::vector<video::IdeaCard> IdeasA = video::Ideas(A.X, A.C, A.X.World), IdeasB = video::Ideas(B.X, B.C, B.X.World);
	// This week's trends as WeekTrends hands them over (the week's mean heat): the Player of the Year race, a champion's news
	// (new this week); last week only the race, cooler.
	video::Trend Poy;
	Poy.Key = "poy:2027";
	Poy.Label = "Player of the Year race";
	Poy.Heat = 0.62;
	Poy.Strength = 1.10;
	video::Trend Champ;
	Champ.Key = "champ:633000";
	Champ.Label = "Tomas Ekdahl wins the Grand Circuit Main";
	Champ.Heat = 0.55;
	Champ.Strength = 1.15;
	video::Trend PoyWas = Poy;
	PoyWas.Heat = 0.15;
	const std::vector<video::Trend> Now = {Champ, Poy}, Was = {PoyWas};
	const double World = A.X.World;
	const std::vector<video::SearchRow> Rows = video::Searches(A.X, A.C, IdeasA, Now, Was, World, 12);
	const std::vector<video::SearchRow> All = video::Searches(A.X, A.C, IdeasA, Now, Was, World, 0);
	bool Shape = Rows.size() == 12 && !Rows.empty() && Rows.front().Interest == 100;
	std::set<std::string> Keys;
	int Formats = 0;
	for (size_t I = 0; I < Rows.size(); ++I)
	{
		const video::SearchRow& R = Rows[I];
		Shape = Shape && R.Interest >= 1 && R.Interest <= 100 && (I == 0 || R.Interest <= Rows[I - 1].Interest) && R.Arrow >= -1 && R.Arrow <= 1 && Keys.insert(R.Key).second &&
			!R.Phrase.empty();
		Formats += R.YourFormat ? 1 : 0;
	}
	Expect(Shape && All.size() >= 60, "searches: twelve phrases, the most searched first at 100, each 1-100 with an arrow (" + std::to_string(Rows.size()) + " of " + std::to_string(All.size()) + ")");
	Expect(Formats >= 2, "searches: at least two of the player's own formats make the table (" + std::to_string(Formats) + ")");
	for (const video::SearchRow& R : Rows)
	{
		std::printf("  search  %-42s %3d %s %s mine %2d  -> %s%s\n", R.Phrase.c_str(), R.Interest, R.Arrow > 0 ? "up  " : R.Arrow < 0 ? "down" : "--  ", R.YourFormat ? "yours" : "     ",
			R.Mine, R.IdeaName.empty() ? "(no idea)" : R.IdeaName.c_str(), R.Idea == 0 ? "" : R.IdeaReady ? " (ready)" : (" (locked: " + R.IdeaHint + ")").c_str());
	}
	// The table's words: never a brand, no AI words, lint-clean, no digit but a trend's own facts.
	{
		std::string Bad;
		for (const std::string& P : vc::SearchInventory())
		{
			const char* Hit = Brand(P);
			if (Hit || SaysAutoplay(P) || !DigitRuns(P).empty() || P.size() > 40)
			{
				Bad += " [" + P + "]";
			}
		}
		Expect(Bad.empty() && vc::SearchInventory().size() >= 55, "searches: " + std::to_string(vc::SearchInventory().size()) + " phrases, no brand, no AI words, no digits, 40 characters at most" + Bad);
	}
	// Interest and arrows are the world's: the same for a different career in the same week (only Mine, the formats and the
	// suggested idea are the player's), and the same at any minute of the week.
	{
		const std::vector<video::SearchRow> Other = video::Searches(B.X, B.C, IdeasB, Now, Was, World, 0);
		const double Monday = std::floor(World / Day) * Day - static_cast<double>((static_cast<int>(std::floor(World / Day)) % 7 + 7) % 7) * Day;
		const std::vector<video::SearchRow> Early = video::Searches(A.X, A.C, IdeasA, Now, Was, Monday + 1.0, 0);
		const std::vector<video::SearchRow> Late = video::Searches(A.X, A.C, IdeasA, Now, Was, Monday + 7.0 * Day - 1.0, 0);
		std::map<std::string, std::pair<int, int>> ByKey;
		for (const video::SearchRow& R : All)
		{
			ByKey[R.Key] = {R.Interest, R.Arrow};
		}
		bool Same = Other.size() == All.size() && Early.size() == All.size() && Late.size() == All.size();
		for (const std::vector<video::SearchRow>* L : {&Other, &Early, &Late})
		{
			for (const video::SearchRow& R : *L)
			{
				Same = Same && ByKey.count(R.Key) && ByKey.at(R.Key) == std::make_pair(R.Interest, R.Arrow);
			}
		}
		const std::vector<video::SearchRow> NextWeek = video::Searches(A.X, A.C, IdeasA, Now, Was, Monday + 7.0 * Day + 1.0, 0);
		int Moved = 0;
		for (const video::SearchRow& R : NextWeek)
		{
			Moved += ByKey.count(R.Key) && ByKey.at(R.Key).first != R.Interest ? 1 : 0;
		}
		Expect(Same && Moved > 0, "searches: interest and arrows are the world's week's (another career, Monday 00:01, Sunday 23:59 alike; the next week moves " +
			std::to_string(Moved) + " phrases)");
	}
	// The week's trends: a champion's news is a phrase of its own, rising, answered by the champion idea; the race lifts its
	// phrase; a year in review is a December search.
	{
		const video::SearchRow* News = nullptr;
		const video::SearchRow* Race = nullptr;
		for (const video::SearchRow& R : All)
		{
			News = R.Key == "trend:" + Champ.Key ? &R : News;
			Race = R.Key == "poy" ? &R : Race;
		}
		const std::vector<video::SearchRow> Quiet = video::Searches(A.X, A.C, IdeasA, {}, {}, World, 0);
		int RaceQuiet = 0;
		for (const video::SearchRow& R : Quiet)
		{
			RaceQuiet = R.Key == "poy" ? R.Interest : RaceQuiet;
		}
		Expect(News && News->Source == 1 && News->Phrase == Champ.Label && News->Arrow == 1 && News->Idea == 801 && Race && Race->Arrow == 1 && Race->Interest > RaceQuiet,
			"searches: this week's champion is a rising phrase answered by the champion idea; the race lifts its own (" + std::to_string(Race ? Race->Interest : 0) + " against " +
				std::to_string(RaceQuiet) + " without it)");
		const auto YearIn = [&](int Day0) {
			for (const video::SearchRow& R : video::Searches(A.X, A.C, IdeasA, {}, {}, static_cast<double>(Day0) * Day + 600.0, 0))
			{
				if (R.Key == "year-review")
				{
					return R.Interest;
				}
			}
			return 0;
		};
		const int Dec = YearIn(441 - 1), Jun = YearIn(441 - 190);
		Expect(Dec >= 4 * Jun && Jun > 0, "searches: a year in review is a December search (" + std::to_string(Dec) + " against " + std::to_string(Jun) + " in June)");
	}
	// Yours on it, and the idea to make: a bad beat's phrase counts the kept bad beats; the suggestion is a ready card when
	// one answers it, else a locked one with its hint; a career with no results plays no format.
	{
		int Beats = 0;
		for (const video::Video& V : A.C.Videos)
		{
			Beats += V.K == video::Kind::BadBeat ? 1 : 0;
		}
		bool MineOk = false, Ideas = true;
		for (const video::SearchRow& R : All)
		{
			MineOk = MineOk || (R.Key == "bad-beat" && R.Mine == Beats);
			const video::IdeaCard* Card = nullptr;
			for (const video::IdeaCard& I : IdeasA)
			{
				Card = I.Id == R.Idea ? &I : Card;
			}
			Ideas = Ideas && (R.Idea == 0 || (Card && Card->Ready == R.IdeaReady && R.IdeaName == Card->Name && (R.IdeaReady ? R.IdeaHint.empty() : R.IdeaHint == Card->Hint)));
		}
		Fixture Empty = A;
		Empty.X.Results.clear();
		bool NoFormats = true;
		for (const video::SearchRow& R : video::Searches(Empty.X, Empty.C, IdeasA, Now, Was, World, 0))
		{
			NoFormats = NoFormats && !R.YourFormat;
		}
		Expect(MineOk && Ideas && NoFormats, "searches: yours on it (" + std::to_string(Beats) + " bad beats), the suggested idea's card as the board has it, no format for a career with no results");
	}
	// A phrase that names the cards counts only the videos whose cards show them (review-c3f-content: the C3 fixture's 23
	// coolers read "kings vs aces: 23 of yours", none of them kings against aces): queens into aces, a fold of nines, a fold of
	// kings and quads count for none; kings against aces either way, a fold of aces and a royal flush count.
	{
		Fixture F = A;
		F.X.Chan = &F.C;
		F.C.Videos.clear();
		auto Add = [&F](video::Kind K, int Idea, const char* Cards) {
			video::Video V;
			V.Id = "named" + std::to_string(F.C.Videos.size());
			V.K = K;
			V.At = F.X.World - 5.0 * Day;
			V.X.On = true;
			V.X.Idea = Idea;
			V.Th.Cards = Cards;
			F.C.Videos.push_back(V);
		};
		auto MineOf = [&F, &IdeasA, &Now, &Was, World](const char* Key) {
			for (const video::SearchRow& R : video::Searches(F.X, F.C, IdeasA, Now, Was, World, 0))
			{
				if (R.Key == Key)
				{
					return R.Mine;
				}
			}
			return -1;
		};
		Add(video::Kind::Cooler, -1, "QdQh/AdAh/4h8s4cTh8d");
		Add(video::Kind::HeroFold, -1, "9h9s/QcKc/Ad5sQhJsJh");
		Add(video::Kind::Idea, 612, "KhKd//Ks9c4h");
		Add(video::Kind::Monster, 115, "9s9h/AcKd/9c9d2hKs4c");
		const int Kings0 = MineOf("kings-aces"), Aces0 = MineOf("fold-aces"), Royal0 = MineOf("royal");
		Add(video::Kind::Cooler, -1, "KsKd/AcAh/3h5s2s5dQs");
		Add(video::Kind::Cooler, 105, "AsAd/KcKh/3h5s2s5dQs");
		Add(video::Kind::HeroFold, -1, "AsAc/QcKc/Ad5sQhJsJh");
		Add(video::Kind::Idea, 612, "AhAd//Ks9c4h");
		Add(video::Kind::Monster, -1, "AhKh/7c7d/QhJhTh2c3d");
		const int Kings = MineOf("kings-aces"), Aces = MineOf("fold-aces"), Royal = MineOf("royal");
		Expect(Kings0 == 0 && Aces0 == 0 && Royal0 == 0 && Kings == 2 && Aces == 2 && Royal == 1,
			"searches: a phrase that names the cards counts only videos whose cards show them (kings vs aces " + std::to_string(Kings0) + " -> " + std::to_string(Kings) +
				", fold aces " + std::to_string(Aces0) + " -> " + std::to_string(Aces) + ", royal flush " + std::to_string(Royal0) + " -> " + std::to_string(Royal) + ")");
	}
	// The suggestion (review-c3f-content's mutant): the first ready idea that answers the phrase, ahead of a locked one listed
	// before it; a locked one, with its hint, only when none is ready.
	{
		auto Card = [](int Id, bool Ready) {
			video::IdeaCard C;
			C.Id = Id;
			C.Name = "idea " + std::to_string(Id);
			C.Ready = Ready;
			C.Hint = Ready ? std::string() : std::string("Do this first.");
			return C;
		};
		auto Pick = [&A, &Now, &Was, World](const std::vector<video::IdeaCard>& Board) {
			for (const video::SearchRow& R : video::Searches(A.X, A.C, Board, Now, Was, World, 0))
			{
				if (R.Key == "icm-bubble") // answered by 603, then 114, then 120
				{
					return std::make_pair(R.Idea, R.IdeaReady ? std::string("ready") : R.IdeaHint);
				}
			}
			return std::make_pair(-1, std::string());
		};
		const auto LockedFirst = Pick({Card(603, false), Card(114, true)}), BothLocked = Pick({Card(114, false), Card(603, false)}),
				   BothReady = Pick({Card(114, true), Card(603, true)});
		Expect(LockedFirst == std::make_pair(114, std::string("ready")) && BothLocked == std::make_pair(603, std::string("Do this first.")) &&
				   BothReady == std::make_pair(603, std::string("ready")),
			"searches: MAKE IT suggests the first ready idea that answers the phrase (" + std::to_string(LockedFirst.first) + "), a locked one with its hint only when none is ready (" +
				std::to_string(BothLocked.first) + ", " + std::to_string(BothReady.first) + ")");
	}
	// The budget: one table a world day (Research caches it).
	{
		const auto T0 = std::chrono::steady_clock::now();
		size_t Sink = 0;
		for (int I = 0; I < 200; ++I)
		{
			Sink += video::Searches(A.X, A.C, IdeasA, Now, Was, World + static_cast<double>(I), 12).size();
		}
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 200.0;
		Expect(Sink == 2400 && Ms < 1.0, "searches: a table in under 1 ms (" + ss::Fixed(Ms, 3) + " ms)");
		std::printf("  searches: %.3f ms a table (12 of %zu phrases)\n", Ms, All.size());
	}
}

void C3Articles(const Fixture& A, const Fixture& B)
{
	// ---- The rule: a number by its spoken first word; a word by its sound.
	{
		struct Case
		{
			const char* Next;
			bool An;
		};
		const Case Cases[] = {{"$11", true}, {"$8", true}, {"$8.80", true}, {"$18,000", true}, {"$80", true}, {"$800", true}, {"$89.50", true}, {"8%", true}, {"11x", true},
			{"18", true}, {"8th", true}, {"$1,190", false}, {"$1.2M", false}, {"$5.50", false}, {"$110", false}, {"$1,800", false}, {"12", false}, {"1st", false},
			{"Embercrest", true}, {"Inner", true}, {"October", true}, {"hour", true}, {"Midnight", false}, {"one-time", false}, {"European", false}, {"unbeaten", true},
			{"unique", false}, {"Sunday", false}};
		std::string Bad;
		for (const Case& C : Cases)
		{
			Bad += vc::TakesAn(C.Next) != C.An ? std::string(" ") + C.Next : std::string();
		}
		Expect(Bad.empty(), "articles: an $11, an 8%, an 18, an Embercrest; a $1,190, a $1.2M, a Midnight" + Bad);
	}
	// ---- The fill fits the article to the slot, keeps a capitals template in capitals, and turns a written "an" back.
	{
		vc::Facts F;
		F.V["BuyIn"] = "$11";
		F.V["Event"] = "Midnight Turbo";
		F.V["Bb"] = "18";
		F.V["Cheap"] = "$5.50";
		const std::string A1 = vc::FillU3("a {BuyIn} buy-in and a {Bb} big blind pot", F), A2 = vc::FillU3("A {BuyIn} SHOT", F), A3 = vc::FillU3("A {BuyIn} shot", F),
						  A4 = vc::FillU3("an {Event} vlog", F), A5 = vc::FillU3("a {Cheap} buy-in", F), A6 = vc::FillU3("banana {BuyIn}", F);
		Expect(A1 == "an $11 buy-in and an 18 big blind pot" && A2 == "AN $11 SHOT" && A3 == "An $11 shot" && A4 == "a Midnight Turbo vlog" && A5 == "a $5.50 buy-in" && A6 == "banana $11",
			"articles: the fill says an $11 buy-in, AN $11 SHOT, a Midnight Turbo vlog (" + A1 + " | " + A2 + " | " + A3 + " | " + A4 + " | " + A5 + ")");
	}
	// ---- Every template filled from its fixtures, and the comments: no "a $11" and no "an $5" anywhere in Rerun 2.0's words.
	{
		const auto Pools = TitlePools(A, B);
		int Filled = 0;
		std::string Bad;
		for (const vc::Tpl& T : vc::Templates3())
		{
			for (const Fill& X : FillsOf(T, Pools))
			{
				++Filled;
				const std::string Hit = BadArticle(X.Text);
				Bad += !Hit.empty() && Bad.size() < 200 ? " [" + Hit + " in " + T.Key + "]" : std::string();
			}
		}
		int Comments = 0;
		for (const char* Title : {"kings, 82% to win, and then the nine of hearts", "I was 89% and lost 18 big blinds", "92% and the river did this", "81% AND THE RIVER DID THIS"})
		{
			video::Video V;
			V.Id = std::string("art:") + Title;
			V.Title = Title;
			V.K = video::Kind::BadBeat;
			V.At = A.X.World - 3.0 * Day;
			V.P = 4000.0;
			V.Th.Cards = "KsKh/Ad9d/Kd9c2s9h9s";
			V.X.On = true;
			V.X.Fate = ss::Fnv1a(V.Id);
			for (const video::Comment& Cm : video::CommentsOf(V, A.X.World, 0, 400))
			{
				++Comments;
				const std::string Hit = BadArticle(Cm.Text);
				Bad += !Hit.empty() && Bad.size() < 200 ? " [" + Hit + " in a comment]" : std::string();
			}
		}
		Expect(Filled > 2000 && Comments > 1000 && Bad.empty(), "articles: " + std::to_string(Filled) + " filled titles and " + std::to_string(Comments) + " comments read an $11, never a $11" + Bad);
	}
	// ---- An idea card's fact line too ("An $11 shot").
	{
		std::string Bad;
		for (const Fixture* Fx : {&A, &B})
		{
			for (const video::IdeaCard& I : video::Ideas(Fx->X, Fx->C, Fx->X.World))
			{
				const std::string Hit = BadArticle(I.Fact) + BadArticle(I.Hint);
				Bad += Hit.empty() ? std::string() : " [" + Hit + "]";
			}
		}
		Expect(Bad.empty(), "articles: the IDEAS cards' fact lines and hints" + Bad);
	}
}

void C3Cuts(const Fixture& A)
{
	using video::Kind;
	// ---- What a Cut from a published video is: a hand video's kind; an idea video's hand as its idea and cards show it.
	{
		auto Long = [](Kind K, int Idea, const char* Cards) {
			video::Video V;
			V.Id = "long";
			V.K = K;
			V.X.On = true;
			V.X.Idea = Idea;
			V.Th.Cards = Cards;
			return V;
		};
		const Kind K1 = vc::CutKindOf(Long(Kind::Idea, 612, "KhKd//Ks9c4h")), K2 = vc::CutKindOf(Long(Kind::Idea, 1502, "QhQd//Ks9c4h")),
				   K3 = vc::CutKindOf(Long(Kind::Idea, 603, "AsKd/QcQh/Ad7s2c9h4d")), K4 = vc::CutKindOf(Long(Kind::Idea, 603, "JhJd//Ks9c4h")),
				   K5 = vc::CutKindOf(Long(Kind::Idea, 1506, "AsAd/7c7h/Kd8s2c9h7d")), K6 = vc::CutKindOf(Long(Kind::Idea, 1501, "9s9h/AcKd/9c9d2hKs4c")),
				   K7 = vc::CutKindOf(Long(Kind::Idea, 1501, "AcJd/QhQc/8d3cJs5hAh")), K8 = vc::CutKindOf(Long(Kind::BadBeat, -1, "AsAd/7c7h/Kd8s2c9h7d")),
				   K9 = vc::CutKindOf(Long(Kind::FinalTable, -1, "AsKd/QcQh/Ad7s2c9h4d"));
		// (A flush and a full house aren't monsters, a straight flush is: Classify's own line, quads or better.)
		const Kind K10 = vc::CutKindOf(Long(Kind::Idea, 1501, "AhKh/7c7d/Qh2h9h4c3d")), K11 = vc::CutKindOf(Long(Kind::Idea, 1501, "9s9h/AcKd/9cKs2hKh4c")),
				   K12 = vc::CutKindOf(Long(Kind::Idea, 1501, "8h9h/AcKd/ThJhQh2c3d"));
		Expect(K1 == Kind::HeroFold && K2 == Kind::HeroFold && K3 == Kind::Bubble && K4 == Kind::HeroFold && K5 == Kind::LastHand && K6 == Kind::Monster && K7 == Kind::Idea &&
				   K8 == Kind::BadBeat && K9 == Kind::Idea && K10 == Kind::Idea && K11 == Kind::Idea && K12 == Kind::Monster,
			"cuts: a fold idea's Cut is a fold, a bubble's a bubble, a level-one bust the last hand, quads or better a monster, any other hand one hand; never a big pot");
	}
	// ---- A Cut's titles never claim an all in or one card to come on a hand without one (a fold, a hand told by an idea; a
	// bubble's or a bust's own kind is an all in, and says so).
	{
		std::string Bad;
		int Offered = 0;
		for (const Kind K : {Kind::HeroFold, Kind::Idea})
		{
			video::Candidate C = A.C.Open.front();
			C.Id = "cut:test" + std::to_string(static_cast<int>(K));
			C.K = K;
			C.Idea = -1;
			C.IdeaFacts.clear();
			C.Event.clear();
			C.EquityAtAllIn = -1.0;
			C.Rec.HeroNet = 0;
			C.Rec.Shown.clear();
			C.Rec.Board = ss::ParseCards("Ks9c4h");
			C.Rec.Mine = ss::ParseCards("KhKd");
			for (int T = 0; T < 5; ++T)
			{
				video::Choice Ch = One(C.Id, video::Format::Short, static_cast<video::TitleStyle>(T));
				for (int S = 0; S < 30; ++S)
				{
					for (const video::TitleOption& O : video::TitleOptions(Ch, {C}, video::Channel(), S, 3))
					{
						++Offered;
						const std::string L = Lower(O.Text);
						if (L.find("all in") != std::string::npos || L.find("one card") != std::string::npos || L.find("whole stack") != std::string::npos ||
							L.find("big pot") != std::string::npos || L.find("biggest pot") != std::string::npos)
						{
							Bad += Bad.size() < 200 ? " [" + O.Text + "]" : std::string();
						}
					}
				}
			}
		}
		Expect(Offered > 100 && Bad.empty(), "cuts: " + std::to_string(Offered) + " Cut titles for a fold or a hand without an all in: none says all in, one card, a whole stack or a big pot" + Bad);
	}
}

/** L7's gear for the compositor (c3f-thumbs' request): a setup idea's Desk Setup recipe carries the ids of the gear the player
 *  owns (fact d, the catalog's order), never one they don't; a desk with nothing owned carries none. */
void C3Desk(const Fixture& A)
{
	const std::vector<ss::gear::Item>& Cat = ss::gear::Catalog();
	if (Cat.size() < 6)
	{
		Expect(false, "desk: the gear catalog");
		return;
	}
	Fixture F = A;
	F.X.Chan = &F.C;
	F.X.Gear = {{Cat[5].Name, Cat[5].PriceCents}, {Cat[1].Name, Cat[1].PriceCents}, {Cat[3].Name, Cat[3].PriceCents}, {"Not a real item", 100}};
	video::Candidate N;
	std::string Why;
	const bool Made = vc::MakeIdeaCandidateOn(F.X, F.C, 1105, F.X.World, N, Why);
	std::string D;
	if (Made)
	{
		const video::Choice Ch = One(N.Id, video::Format::Breakdown, video::TitleStyle::Story);
		const vc::Facts Fa = vc::FactsOf(Ch, {N}, N, &F.C);
		const video::ThumbSpec S = vc::RecipeFor(video::thumb::ArDeskSetup, N, {N}, Fa, 7u, -1, -1);
		D = video::ThumbFact(S, "d");
	}
	const std::string Want = Cat[1].Id + "|" + Cat[3].Id + "|" + Cat[5].Id;
	Fixture Bare = A;
	Bare.X.Chan = &Bare.C;
	Bare.X.Gear = {{"Not a real item", 100}, {"Nor this", 100}, {"Nor that", 100}};
	video::Candidate M;
	std::string BareD = "unset";
	if (vc::MakeIdeaCandidateOn(Bare.X, Bare.C, 1105, Bare.X.World, M, Why))
	{
		const video::Choice Ch = One(M.Id, video::Format::Breakdown, video::TitleStyle::Story);
		BareD = video::ThumbFact(vc::RecipeFor(video::thumb::ArDeskSetup, M, {M}, vc::FactsOf(Ch, {M}, M, &Bare.C), 7u, -1, -1), "d");
	}
	Expect(Made && D == Want && BareD.empty(), "desk: Desk Setup draws the gear the player owns (" + D + "), none they don't (" + BareD + ")");
}

/** H1's top-earner rate on an idea's own numbers: "Why my RPM is {Rpm}" quotes the RPM Analytics shows (StatsOf: the month's
 *  share past T); at T or under it, the same facts byte for byte. */
void C3TopRateIdea(const Fixture& A)
{
	const int Today = static_cast<int>(std::floor(A.X.World / Day));
	const int First = video::valgo::PayMonthFirst(Today);
	double Sum = 0.0, N = 0.0;
	for (const video::Video& V : A.C.Videos)
	{
		if (V.Rpm > 0.0 && V.F != video::Format::Short)
		{
			Sum += V.Rpm;
			N += 1.0;
		}
	}
	auto RpmOf = [&](Chips Paid, std::string& Facts) -> long long {
		Fixture F = A;
		F.X.Chan = &F.C;
		F.C.X.Days.clear();
		if (Paid > 0)
		{
			video::DayRow R;
			R.Key = First;
			R.Cents = Paid;
			F.C.X.Days.push_back(R);
		}
		video::Candidate Out;
		std::string Why;
		if (!vc::MakeIdeaCandidateOn(F.X, F.C, 512, F.X.World, Out, Why))
		{
			Facts = "not ready: " + Why;
			return -1;
		}
		Facts = Out.IdeaFacts;
		const size_t At = Facts.find(";Rpm=");
		if (At == std::string::npos)
		{
			return -2;
		}
		// "$2.10" (or "$2"): dollars, then the cents after the point.
		long long Dollars = 0, Cents = 0;
		int CentDigits = -1;
		bool Any = false;
		for (size_t I = At + 5; I < Facts.size() && Facts[I] != ';'; ++I)
		{
			const char Ch = Facts[I];
			if (Ch == '.')
			{
				CentDigits = 0;
			}
			else if (std::isdigit(static_cast<unsigned char>(Ch)))
			{
				Any = true;
				if (CentDigits < 0)
				{
					Dollars = Dollars * 10 + (Ch - '0');
				}
				else if (CentDigits < 2)
				{
					Cents = Cents * 10 + (Ch - '0');
					++CentDigits;
				}
			}
		}
		return Any ? Dollars * 100 + (CentDigits == 1 ? Cents * 10 : Cents) : -3;
	};
	const Chips T = video::valgo::TopRateFrom(), Top = video::valgo::TopRateTop();
	const Chips Past = T + (Top - T) / 2;
	std::string F0, F1, F2;
	const long long Full = RpmOf(0, F0), AtT = RpmOf(T, F1), Half = RpmOf(Past, F2);
	const double Share = video::valgo::TopRateShare(Past);
	const long long WantFull = N > 0.0 ? std::llround(Sum / N * 100.0) : -9, WantHalf = N > 0.0 ? std::llround(Sum / N * Share * 100.0) : -9;
	std::printf("  top rate: the RPM idea quotes %lld cents at or under T, %lld past it (share %.3f)\n", Full, Half, Share);
	Expect(First < Today && Full == WantFull && AtT == Full && F1 == F0 && Share > 0.49 && Share < 0.51 && Half == WantHalf,
		"top rate: the RPM idea quotes Analytics' RPM: " + std::to_string(Full) + " cents at or under T (same facts), " + std::to_string(Half) +
			" past it at a " + std::to_string(Share).substr(0, 4) + " share (want " + std::to_string(WantFull) + ", " + std::to_string(WantHalf) + ")" +
			(Full < 0 ? " [" + F0 + "]" : std::string()));
}

void C3Session()
{
	struct Hooks : ss::SessionHooks
	{
		void Save(const ss::SaveData& D) override { Last = D; }
		ss::SaveData Last;
	};
	auto Wait = [](ss::Session& S, double& Now, double Seconds) {
		for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
		}
	};
	Hooks H;
	ss::Session S(H, "rerun-content-c3f");
	double Now = 0.0;
	S.CurrentScreen = ss::Screen::Lobby;
	Wait(S, Now, 0.2);
	S.Life.Update3LinesRead |= ss::life::U3Video;
	S.Life.Energy = 90.0;
	S.Life.Hunger = 5.0;
	S.Life.Thirst = 5.0;
	S.DebugVideo("fake badbeat online");
	if (S.Life.Video.Open.empty())
	{
		Expect(false, "c3f session: a moment in MOMENTS");
		return;
	}
	// ---- M1 end to end: Studio's edit of a dealt recipe goes up as edited, and the save keeps it; a forged one is refused.
	video::Choice C;
	C.Candidates = {S.Life.Video.Open[0].Id};
	C.F = video::Format::Breakdown;
	C.T = video::TitleStyle::Story;
	const std::vector<video::TitleOption> Titles = S.TitleOptions(C, 0);
	const std::vector<video::ThumbSpec> Thumbs = S.ThumbOptions(C, 0);
	if (Titles.empty() || Thumbs.empty())
	{
		Expect(false, "c3f session: Studio's options");
		return;
	}
	C.TitleTpl = Titles[0].Key;
	video::ThumbSpec Pick = Thumbs[0];
	const video::ThumbChoices Ok = video::ThumbChoicesOf(Pick, C, S.Life.Video.Open, S.Life.Video);
	for (const int16_t P : Ok.Pals)
	{
		Pick = P != Thumbs[0].Pal && Pick.Pal == Thumbs[0].Pal ? video::ThumbEdit(Pick, C, S.Life.Video.Open, S.Life.Video, video::TfPalette, P) : Pick;
	}
	for (const int16_t X : Ok.Exprs)
	{
		Pick = X != Thumbs[0].Expr && Pick.Expr == Thumbs[0].Expr ? video::ThumbEdit(Pick, C, S.Life.Video.Open, S.Life.Video, video::TfFace, X) : Pick;
	}
	if (Ok.Captions.size() > 1)
	{
		const size_t K = Ok.Captions[0].first == Pick.Caption ? 1 : 0;
		Pick = video::ThumbEdit(Pick, C, S.Life.Video.Open, S.Life.Video, video::TfCaption, static_cast<int>(K));
	}
	const bool Edited = Pick.Pal != Thumbs[0].Pal && (Ok.Exprs.size() < 2 || Pick.Expr != Thumbs[0].Expr);
	video::Choice Forged = C;
	Forged.Thumbs = {Pick};
	Forged.Thumbs[0].Caption = "ROYAL"; // a word the moment never had
	const std::string Refused = S.Publish(Forged);
	Expect(Refused == vc::ThumbNotALook() && !S.TimeSkip.Active, "c3f session: PUBLISH refuses a recipe outside its archetype's values (" + Refused + ")");
	C.Thumbs = {Pick};
	Expect(Edited && S.Publish(C).empty() && S.TimeSkip.Active, "c3f session: PUBLISH takes the recipe as Studio's pickers edited it");
	Wait(S, Now, 8.0);
	const video::Video* V = S.Life.Video.Videos.empty() ? nullptr : &S.Life.Video.Videos.back();
	Expect(V && V->X.On && SameSpec(V->X.Spec[0], [&] {
		video::ThumbSpec P = Pick;
		P.LookRef = V ? V->X.Spec[0].LookRef : P.LookRef; // the face is frozen at upload
		return P;
	}()),
		"c3f session: the video goes up with the edited palette, face and caption");
	{
		ss::SaveData Back;
		const bool Parsed = ss::SaveData::Parse(H.Last.Serialize(), Back);
		const video::Video* R = nullptr;
		for (const video::Video& X : Back.Life.Video.Videos)
		{
			R = V && X.Id == V->Id ? &X : R;
		}
		Expect(Parsed && R && V && SameSpec(R->X.Spec[0], V->X.Spec[0]), "c3f session: the save keeps the edited recipe as it went up");
	}
	if (!V)
	{
		return;
	}
	// ---- L8 end to end: the long video told as an idea of a fold (F12, kings folded on the flop); its Cut is a fold.
	const std::string LongId = V->Id;
	for (video::Video& X : S.Life.Video.Videos)
	{
		if (X.Id == LongId)
		{
			X.K = video::Kind::Idea;
			X.X.Idea = 612;
			X.Th.Cards = "KhKd//Ks9c4h";
			X.Th.Opponent.clear();
		}
	}
	Expect(S.LaunchAction(LongId, video::Launch::Cut).empty() && S.TimeSkip.Active, "c3f session: Make a Cut from the fold idea's video starts");
	Wait(S, Now, 4.0);
	const video::Video* Cut = nullptr;
	for (const video::Video& X : S.Life.Video.Videos)
	{
		Cut = X.F == video::Format::Short && X.X.On ? &X : Cut;
	}
	const std::string Title = Cut ? Lower(Cut->Title) : std::string();
	Expect(Cut && Cut->K == video::Kind::HeroFold && Title.find("pot") == std::string::npos && Title.find("all in") == std::string::npos &&
			   Title.find("one card") == std::string::npos && Title.find("stack") == std::string::npos,
		"c3f session: the Cut of a fold idea goes up as a fold, titled as one (" + (Cut ? Cut->Title : std::string("none")) + ")");
	// ---- M2 through the session: the week's table, the same all week.
	{
		const std::vector<video::IdeaCard> Board = S.VideoIdeas();
		const std::vector<video::SearchRow> Rows = video::SearchesOf(S, Board, 10);
		const double W = S.WorldMinutes();
		const int Today = static_cast<int>(std::floor(W / Day));
		const double Monday = static_cast<double>(Today - Today % 7) * Day;
		const std::vector<video::Trend> T1 = video::WeekTrends(S, Monday + 1.0), T2 = video::WeekTrends(S, Monday + 7.0 * Day - 1.0);
		bool Same = T1.size() == T2.size();
		for (size_t I = 0; Same && I < T1.size(); ++I)
		{
			Same = T1[I].Key == T2[I].Key && T1[I].Heat == T2[I].Heat;
		}
		Expect(Rows.size() == 10 && Rows.front().Interest == 100 && Same, "c3f session: Research's table through the session, its week's trends the same at Monday 00:01 and Sunday 23:59 (" +
			std::to_string(Rows.size()) + " rows, " + std::to_string(T1.size()) + " trends)");
		const auto T0 = std::chrono::steady_clock::now();
		for (int I = 0; I < 20; ++I)
		{
			(void)video::SearchesOf(S, Board, 10);
		}
		std::printf("  searches through the session: %.3f ms a table (two weeks of trends)\n", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count() / 20.0);
	}
	// ---- The player's formats are History's last 60 days (review-c3f-content's mutant): a hyper 30 days ago marks "beat hyper
	// turbos" as theirs; a bounty 90 days ago doesn't mark "PKO bounty maths".
	{
		const double W = S.WorldMinutes();
		ss::HistoryEntry Hyper;
		Hyper.Name = "$1.10 Hyper Sprint";
		Hyper.Place = 40;
		Hyper.BuyInCents = 110;
		Hyper.EventId = "hyper-sprint@" + std::to_string(static_cast<long long>(W - 30.0 * Day));
		ss::HistoryEntry Pko = Hyper;
		Pko.Name = "$5.50 PKO Bounty Builder";
		Pko.BuyInCents = 550;
		Pko.EventId = "pko@" + std::to_string(static_cast<long long>(W - 90.0 * Day));
		S.History.push_back(Hyper);
		S.History.push_back(Pko);
		bool HyperYours = false, PkoYours = true;
		for (const video::SearchRow& R : video::SearchesOf(S, S.VideoIdeas(), 0))
		{
			HyperYours = R.Key == "hypers" ? R.YourFormat : HyperYours;
			PkoYours = R.Key == "pko" ? R.YourFormat : PkoYours;
		}
		S.History.resize(S.History.size() - 2);
		Expect(HyperYours && !PkoYours, "c3f session: Research marks the formats of the last 60 days as yours (a hyper 30 days ago yes, a bounty 90 days ago no)");
	}
}
} // namespace rerun_content_test

int main(int Argc, char** Argv)
{
	using namespace rerun_content_test;
	const auto T0 = std::chrono::steady_clock::now();
	const std::string Mode = Argc > 1 ? Argv[1] : "";
	const Fixture A = FullFixture(false), B = FullFixture(true);
	// (The fixtures' channel pointers: each IdeaCtx reads its own fixture's channel.)
	Fixture Ax = A, Bx = B;
	Ax.X.Chan = &Ax.C;
	Bx.X.Chan = &Bx.C;
	if (Mode == "world")
	{
		PrintWorld = true;
		WorldCreators();
		return 0;
	}
	if (Mode == "comments")
	{
		PrintComments = true;
		Comments(Ax);
		return 0;
	}
	if (Mode == "titles")
	{
		Titles(Ax, Bx, true);
		return 0;
	}
	Titles(Ax, Bx, false);
	Picker(Ax);
	WorkedTitles(Ax);
	Thumbnails(Ax, Mode == "--write");
	WorkedThumbnails(Ax);
	IdeasCheck(Ax, Bx);
	PacingCreator();
	Comments(Ax);
	WorldCreators();
	Budgets(Ax);
	SessionGlue();
	ReviewChecks(Ax);
	C3Pickers(Ax);
	C3Searches(Ax, Bx);
	C3Articles(Ax, Bx);
	C3Cuts(Ax);
	C3Desk(Ax);
	C3TopRateIdea(Ax);
	C3Session();
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("rerun content tests: %d checks, %d failed in %.1fs\n", Checks, Failures, Seconds);
	return Failures == 0 ? 0 : 1;
}
