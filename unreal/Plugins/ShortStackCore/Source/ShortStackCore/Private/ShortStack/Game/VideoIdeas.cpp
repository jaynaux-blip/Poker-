// Update 3 (SPEC §6.3, §7.3; SPEC_CREATORS §3.7): Rerun 2.0's video ideas, core-videocontent's file. 148 idea types from the
// player's own career (§6.3's 147 P0 and the creator update's Road to the Final Table), each true to its facts and locked,
// saying what to do, until the facts exist. The 28 P1 ideas stay out (the P1 list stays P1): nothing here promises them.
//
// An idea reads only IdeaCtx (what the Session already keeps) and the channel. Choosing a ready idea makes a candidate
// (MakeIdeaCandidate, Source 5) whose IdeaFacts carry what core-videoalgo reads (w, cool, ad, ltd, trend, fmt, cls: lower
// case) and what a title may say (the slots' own names: Month=October;Events=214, escaped by FactEsc) and nothing else (U7).
// Group A reads its hand from MOMENTS: its candidate is that moment, tagged with the idea (the hand's own record and kind).
//
// Also here: the new hand kinds a record shows by itself (RecordKinds: Monster, SetOverSet, RunnerRunner; Classify's), the
// idea -> advertiser category table (IdeaAdCategory, for AdMarket) and the series totals (SeriesOf).
#include "VideoContentPriv.h"
#include "VideoAlgoPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ss
{
namespace video
{
namespace video_detail
{
int OwnCategory(const std::vector<Card>& Hole, const std::vector<Card>& Board); // Video.cpp
double PotBbOf(const Candidate& C);
bool EventKind(Kind K);
} // namespace video_detail
namespace video_copy
{
std::string HandWords(const std::vector<Card>& H); // VideoCopy.cpp
} // namespace video_copy

namespace vcontent
{
namespace
{
// Formats (bits by Format), styles (bits by TitleStyle), the advertiser rows (IdeaAdCategory's), suitability.
constexpr uint8_t ViCut = 1, ViBkd = 2, ViVlog = 4, ViRecap = 8;
constexpr uint8_t ViStory = 1, ViLoud = 2, ViTeach = 4, ViQuest = 8, ViPlain = 16;
constexpr int ViGear = 0, ViTeachAd = 1, ViResults = 2, ViStoryAd = 3, ViLife = 4, ViLoudAd = 5, ViNews = 6, ViRival = 7;
constexpr uint8_t ViLtd = 1, ViFlag = 2;

IdeaDef ViDef(int Id, const char* Code, const char* Name, double Topic, uint8_t Formats, uint8_t Styles, bool Strategy, int Ad, uint8_t Suit, int Cool,
	std::initializer_list<int> Arch, Kind K = Kind::Idea)
{
	IdeaDef D;
	D.Id = Id;
	D.Code = Code;
	D.Name = Name;
	D.Topic = Topic;
	D.Formats = Formats;
	D.Styles = Styles;
	D.Strategy = Strategy;
	D.Ad = Ad;
	D.Limited = (Suit & ViLtd) != 0;
	D.Adult = D.Limited;
	D.Flag = (Suit & ViFlag) != 0;
	D.Cool = Cool;
	int N = 0;
	for (const int A : Arch)
	{
		if (N < 3)
		{
			D.Arch[N++] = A;
		}
	}
	D.HandKind = K;
	return D;
}
} // namespace

const std::vector<IdeaDef>& IdeaTable()
{
	using namespace thumb;
	static const std::vector<IdeaDef> All = {
		// ---- A. single hands, from MOMENTS (Cut, Breakdown; the hand kind's own rows)
		ViDef(101, "A1", "Bad beat", 2.0, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArGutPunch, ArCircledRiver}, Kind::BadBeat),
		ViDef(102, "A2", "Suckout", 1.4, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArCircledRiver, ArGutPunch}, Kind::Suckout),
		ViDef(103, "A3", "Cooler", 1.8, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArVsSplit, ArGutPunch}, Kind::Cooler),
		ViDef(104, "A4", "Set over set", 1.9, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArVsSplit, ArGutPunch}, Kind::SetOverSet),
		ViDef(105, "A5", "Kings against aces", 2.0, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArVsSplit, ArGutPunch}, Kind::Cooler),
		ViDef(106, "A6", "Bluff", 1.9, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArNervesOfSteel, ArTheRead}, Kind::Bluff),
		ViDef(107, "A7", "The seven-deuce bluff", 2.1, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArNervesOfSteel}, Kind::Bluff),
		ViDef(108, "A8", "Bluff caught", 1.5, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArCaught}, Kind::BluffCaught),
		ViDef(109, "A9", "Hero call", 1.8, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArTheRead, ArQuiz}, Kind::HeroCall),
		ViDef(110, "A10", "Hero fold", 1.6, ViCut | ViBkd, ViStory | ViTeach | ViQuest, false, -1, 0, 0, {ArTheFold, ArQuiz}, Kind::HeroFold),
		ViDef(111, "A11", "Big pot", 1.2, ViCut | ViBkd, ViStory | ViLoud | ViPlain, false, -1, 0, 0, {ArThePile, ArBigNumber}, Kind::BigPot),
		ViDef(112, "A12", "Knockout", 1.0, ViCut | ViBkd, ViStory | ViLoud | ViPlain, false, -1, 0, 0, {ArHeadhunter}, Kind::Knockout),
		ViDef(113, "A13", "Double knockout", 1.4, ViCut | ViBkd, ViStory | ViLoud | ViPlain, false, -1, 0, 0, {ArHeadhunter, ArBigNumber}, Kind::DoubleKo),
		ViDef(114, "A14", "Bubble", 1.6, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArTheBubble}, Kind::Bubble),
		ViDef(115, "A15", "Monster hand", 1.7, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArMonsterHand}, Kind::Monster),
		ViDef(116, "A16", "Mystery envelope", 1.6, ViCut | ViBkd, ViStory | ViLoud | ViPlain, false, -1, 0, 0, {ArEnvelope}, Kind::Envelope),
		ViDef(117, "A17", "Rival pot", 1.8, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, ViRival, 0, 0, {ArTheGhost, ArVsSplit}, Kind::Rival),
		ViDef(118, "A18", "Stream moment", 1.3, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArStreamClip}, Kind::StreamMoment),
		ViDef(119, "A19", "Runner-runner", 1.6, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArCircledRiver, ArGutPunch}, Kind::RunnerRunner),
		ViDef(120, "A20", "Ladder fold", 1.6, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArTheFold, ArClassroom}, Kind::LadderFold),
		ViDef(121, "A21", "Min-cash scramble", 1.3, ViCut | ViBkd, ViStory | ViLoud | ViTeach, false, -1, 0, 0, {ArTheBubble}, Kind::MinCash),
		ViDef(122, "A22", "Heads-up for the title", 1.8, ViCut | ViBkd, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArVsSplit, ArTrophyShot}, Kind::HeadsUp),
		ViDef(124, "A24", "The last hand", 1.0, ViCut, ViStory | ViLoud | ViQuest, false, -1, 0, 0, {ArGutPunch, ArMistake}, Kind::LastHand),
		// ---- B. nights and sessions (Vlog, Breakdown)
		ViDef(201, "B1", "Session recap", 1.2, ViVlog | ViBkd, ViStory | ViQuest | ViPlain, false, ViResults, 0, 3, {ArNightShift, ArTheGraph}),
		ViDef(202, "B2", "Sunday grind", 1.4, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 6, {ArNightShift, ArTheGraph}),
		ViDef(204, "B4", "Series day", 1.5, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 3, {ArNightShift, ArTheGraph}),
		ViDef(205, "B5", "Satellite step", 1.5, ViVlog | ViBkd, ViStory | ViLoud | ViTeach, false, ViResults, 0, 7, {ArBeforeAfter, ArTrophyShot, ArBigNumber}),
		ViDef(206, "B6", "Freeroll grind", 1.0, ViVlog | ViBkd, ViStory | ViTeach | ViPlain, false, ViResults, 0, 7, {ArBeforeAfter, ArNightShift}),
		ViDef(207, "B7", "Hyper marathon", 1.1, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 5, {ArNightShift, ArTheGraph}),
		ViDef(208, "B8", "Omaha night", 1.1, ViVlog | ViBkd, ViStory | ViTeach | ViQuest, false, ViResults, 0, 5, {ArNightShift, ArClassroom, ArTheGraph}),
		ViDef(209, "B9", "Bounty night", 1.2, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 5, {ArHeadhunter, ArNightShift}),
		// ---- C. runs and titles (Recap, Breakdown, Vlog)
		ViDef(301, "C1", "Deep run", 1.3, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViPlain, false, ViResults, 0, 3, {ArBigNumber, ArBeforeAfter, ArTopFive}),
		ViDef(302, "C2", "Final table", 1.7, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViTeach, false, ViResults, 0, 3, {ArTopFive, ArBeforeAfter, ArClassroom}),
		ViDef(303, "C3", "Win", 2.2, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViPlain, false, ViResults, 0, 3, {ArTrophyShot, ArBigNumber, ArBeforeAfter}),
		ViDef(304, "C4", "Every hand of my final table", 1.8, ViRecap | ViBkd, ViStory | ViTeach | ViPlain, false, ViTeachAd, 0, 7, {ArTopFive, ArClassroom}),
		ViDef(305, "C5", "Biggest score ever", 2.0, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 0, {ArBigNumber, ArBeforeAfter, ArTheGraph}),
		ViDef(306, "C6", "Bubble boy", 1.4, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 7, {ArTheBubble, ArTextSlab, ArBigNumber}),
		ViDef(307, "C7", "Back to back", 1.5, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViPlain, false, ViResults, 0, 7, {ArTopFive, ArBeforeAfter}),
		ViDef(309, "C9", "Series title, bracelet, ring", 1.8, ViRecap | ViBkd | ViVlog, ViStory | ViLoud | ViTeach, false, ViResults, 0, 0, {ArTrophyShot, ArBigNumber, ArBeforeAfter}),
		ViDef(310, "C10", "Overlay catch", 1.3, ViRecap | ViBkd | ViVlog, ViStory | ViTeach | ViQuest, false, ViResults, ViFlag, 7, {ArBigNumber, ArSiteCard, ArBeforeAfter}),
		ViDef(312, "C12", "Road to the Final Table", 1.8, ViRecap | ViVlog, ViStory | ViLoud | ViPlain, false, ViResults, 0, 0, {ArStreamClip, ArBigNumber, ArTopFive}),
		// ---- D. live (the Embercrest told vlog-style; Dee's as storytime)
		ViDef(401, "D1", "Embercrest night", 1.3, ViVlog | ViBkd, ViStory | ViQuest | ViPlain, false, ViLife, 0, 3, {ArVenue, ArRoadTrip}),
		ViDef(402, "D2", "Embercrest Sunday", 1.5, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViLife, 0, 6, {ArVenue, ArBeforeAfter, ArRoadTrip}),
		ViDef(404, "D4", "Live vs online", 1.2, ViBkd | ViVlog, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 30, {ArVsSplit, ArTheGraph, ArClassroom}),
		ViDef(405, "D5", "Back-room storytime", 1.5, ViVlog | ViBkd, ViStory | ViLoud | ViQuest, false, ViStoryAd, 0, 7, {ArTextSlab, ArGutPunch, ArQuiz}),
		ViDef(406, "D6", "Midnight Turbo run", 1.2, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViLife, 0, 3, {ArVenue, ArNightShift, ArRoadTrip}),
		ViDef(407, "D7", "The bus home", 0.9, ViCut, ViStory | ViQuest, false, ViLife, 0, 3, {ArRoadTrip, ArVenue, ArNightShift}),
		// ---- E. bankroll and money
		ViDef(501, "E1", "Bankroll update", 1.3, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 25, {ArTheGraph, ArTextSlab, ArNightShift}),
		ViDef(503, "E3", "Downswing diary", 1.4, ViBkd | ViVlog, ViStory | ViTeach | ViQuest, false, ViResults, 0, 10, {ArTheGraph, ArClassroom, ArTextSlab}),
		ViDef(504, "E4", "A winning stretch", 1.2, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 10, {ArTheGraph, ArBeforeAfter}),
		ViDef(505, "E5", "Rent paid by poker", 1.5, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 0, {ArRentLetter, ArPayday, ArTextSlab}),
		ViDef(506, "E6", "Rent watch", 1.4, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 0, {ArRentLetter, ArTextSlab, ArNightShift}),
		ViDef(508, "E8", "ROI after N tournaments", 1.3, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 0, {ArTheGraph, ArClassroom}),
		ViDef(509, "E9", "Moving up", 1.3, ViBkd | ViVlog, ViStory | ViTeach | ViQuest, false, ViResults, 0, 30, {ArTheGraph, ArClassroom, ArTextSlab}),
		ViDef(510, "E10", "Taking a shot", 1.4, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViResults, 0, 7, {ArBeforeAfter, ArTextSlab, ArReactionBox}),
		ViDef(511, "E11", "My first year in poker", 1.6, ViBkd | ViVlog, ViStory | ViTeach | ViPlain, false, ViResults, 0, 0, {ArTheGraph, ArTopFive}),
		ViDef(512, "E12", "What Rerun pays me", 1.4, ViBkd | ViVlog, ViStory | ViTeach | ViQuest, false, ViResults, 0, 0, {ArPayday, ArBigNumber, ArTextSlab}),
		// ---- F. strategy and study (Teaching-styled: Strategy)
		ViDef(601, "F1", "A spot I'm proud of", 1.4, ViBkd, ViStory | ViTeach | ViQuest, true, ViTeachAd, 0, 7, {ArClassroom, ArTheRead, ArQuiz}),
		ViDef(602, "F2", "A mistake I made", 1.5, ViBkd, ViStory | ViTeach | ViQuest, true, ViTeachAd, 0, 7, {ArMistake, ArClassroom, ArQuiz}),
		ViDef(603, "F3", "Bubble ICM", 1.5, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 7, {ArClassroom, ArTheBubble, ArTheFold}),
		ViDef(604, "F4", "Push/fold chart", 1.3, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 14, {ArClassroom, ArQuiz}),
		ViDef(605, "F5", "PKO bounty maths", 1.3, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 14, {ArClassroom, ArHeadhunter}),
		ViDef(606, "F6", "Satellite strategy", 1.4, ViBkd, ViTeach | ViQuest | ViStory, true, ViTeachAd, 0, 14, {ArClassroom, ArBeforeAfter, ArTrophyShot}),
		ViDef(607, "F7", "Mystery bounty strategy", 1.2, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 14, {ArClassroom, ArEnvelope, ArQuiz}),
		ViDef(608, "F8", "Quiz: call or fold", 1.3, ViBkd, ViQuest | ViTeach | ViPlain, true, ViTeachAd, 0, 7, {ArQuiz, ArClassroom, ArTheRead}),
		ViDef(609, "F9", "Short-stack play", 1.2, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 14, {ArClassroom, ArQuiz}),
		ViDef(610, "F10", "Final table ICM", 1.5, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 14, {ArClassroom, ArTopFive}),
		ViDef(611, "F11", "Coach session", 1.4, ViBkd, ViStory | ViTeach | ViQuest, true, ViTeachAd, 0, 7, {ArClassroom, ArTextSlab, ArQuiz}),
		ViDef(612, "F12", "Folding aces or kings", 1.7, ViBkd, ViStory | ViLoud | ViTeach, true, ViTeachAd, 0, 7, {ArTheFold, ArMistake, ArQuiz}),
		ViDef(613, "F13", "How the fields play on a site", 1.2, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, ViFlag, 30, {ArSiteCard, ArClassroom, ArTheGraph}),
		ViDef(614, "F14", "Starting hands, the basics", 1.0, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 60, {ArClassroom, ArTierList, ArQuiz}),
		// ---- G. lists and compilations
		ViDef(701, "G1", "Top hands of the week", 1.4, ViBkd | ViVlog | ViCut, ViStory | ViLoud | ViPlain, false, -1, 0, 6, {ArTopFive, ArTextSlab}),
		ViDef(702, "G2", "The month's best", 1.45, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, -1, 0, 25, {ArTopFive, ArTierList, ArTextSlab}),
		ViDef(705, "G5", "Every all in of the Sunday", 1.3, ViBkd | ViVlog, ViStory | ViQuest | ViPlain, false, -1, 0, 6, {ArTopFive, ArTextSlab}),
		ViDef(708, "G8", "RiverLine tournaments ranked", 1.2, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 60, {ArTierList, ArClassroom, ArTextSlab}),
		ViDef(709, "G9", "The Embercrest schedule ranked", 1.2, ViBkd, ViStory | ViTeach | ViPlain, true, ViTeachAd, 0, 60, {ArTierList, ArVenue, ArTextSlab}),
		ViDef(710, "G10", "Every site I play, ranked", 1.3, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, ViFlag, 60, {ArTierList, ArSiteCard, ArClassroom}),
		ViDef(711, "G11", "Gear tier list", 1.1, ViBkd | ViVlog, ViStory | ViQuest | ViPlain, false, ViGear, 0, 60, {ArTierList, ArDeskSetup, ArTextSlab}),
		ViDef(712, "G12", "Hands that taught me the most", 1.3, ViBkd, ViStory | ViTeach, true, ViTeachAd, 0, 0, {ArTopFive, ArMistake, ArClassroom}),
		ViDef(713, "G13", "Starting hands tier list", 1.1, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, 0, 60, {ArTierList, ArClassroom, ArQuiz}),
		ViDef(714, "G14", "Rivers in a minute", 1.1, ViCut, ViStory | ViLoud | ViPlain, false, -1, 0, 7, {ArCircledRiver, ArTopFive, ArBigNumber}),
		ViDef(715, "G15", "My most-watched Cuts", 1.1, ViBkd | ViCut, ViStory | ViLoud | ViPlain, false, -1, 0, 30, {ArTopFive, ArBigNumber, ArTextSlab}),
		// ---- H. reactions and news (world facts at least a day old)
		ViDef(801, "H1", "A champion", 1.2, ViBkd, ViStory | ViLoud | ViQuest, false, ViNews, 0, 3, {ArReactionBox, ArTrophyShot}),
		ViDef(802, "H2", "A major's final table", 1.3, ViBkd, ViStory | ViQuest | ViPlain, false, ViNews, 0, 3, {ArReactionBox, ArTierList, ArClassroom}),
		ViDef(803, "H3", "The Player of the Year race", 1.2, ViBkd, ViStory | ViQuest | ViPlain, false, ViNews, 0, 14, {ArReactionBox, ArTierList, ArTrophyShot}),
		ViDef(804, "H4", "Hall of Fame class", 1.1, ViBkd, ViStory | ViQuest | ViPlain, false, ViNews, 0, 30, {ArReactionBox, ArTrophyShot, ArClassroom}),
		ViDef(805, "H5", "A new site, first look", 1.5, ViBkd | ViVlog, ViStory | ViQuest | ViPlain, false, ViNews, ViLtd | ViFlag, 14, {ArSiteCard, ArReactionBox, ArClassroom}),
		ViDef(806, "H6", "Series preview", 1.2, ViBkd, ViTeach | ViQuest | ViPlain, false, ViNews, 0, 14, {ArClassroom, ArTierList, ArReactionBox}),
		ViDef(807, "H7", "Overlay alert", 1.1, ViBkd | ViCut, ViLoud | ViQuest | ViPlain, false, ViNews, ViLtd | ViFlag, 7, {ArSiteCard, ArReactionBox, ArTextSlab}),
		ViDef(808, "H8", "Leaderboard week", 1.0, ViBkd, ViStory | ViQuest | ViPlain, false, ViNews, 0, 7, {ArTierList, ArReactionBox, ArTopFive}),
		ViDef(809, "H9", "Rivalry breakdown", 1.1, ViBkd, ViStory | ViTeach | ViQuest, false, ViNews, 0, 14, {ArVsSplit, ArReactionBox, ArClassroom}),
		ViDef(810, "H10", "Reacting to my first video", 1.4, ViBkd, ViStory | ViLoud | ViQuest, false, ViStoryAd, 0, 0, {ArReactionBox, ArMilestone, ArTextSlab}),
		// ---- I. community and milestones
		ViDef(901, "I1", "Subscriber milestone", 1.4, ViBkd | ViCut, ViStory | ViLoud | ViPlain, false, ViStoryAd, 0, 0, {ArMilestone, ArTextSlab, ArReactionBox}),
		ViDef(902, "I2", "Q&A from chat", 1.1, ViBkd, ViStory | ViQuest | ViPlain, false, ViStoryAd, 0, 14, {ArTextSlab, ArMilestone, ArReactionBox}),
		ViDef(903, "I3", "Partner reveal", 1.5, ViBkd | ViVlog, ViStory | ViLoud | ViQuest, false, ViStoryAd, 0, 0, {ArMilestone, ArPayday}),
		ViDef(904, "I4", "One year on Rerun", 1.4, ViBkd | ViVlog, ViStory | ViTeach | ViPlain, false, ViStoryAd, 0, 0, {ArMilestone, ArTheGraph}),
		ViDef(906, "I6", "Reading your comments", 1.0, ViBkd, ViStory | ViLoud | ViPlain, false, ViStoryAd, 0, 30, {ArTextSlab, ArReactionBox, ArBigNumber}),
		ViDef(907, "I7", "Chat picks my tournaments", 1.1, ViVlog | ViBkd, ViStory | ViLoud | ViQuest, false, ViStoryAd, 0, 14, {ArStreamClip, ArNightShift, ArTextSlab}),
		ViDef(908, "I8", "Raid recap", 1.0, ViCut, ViStory | ViLoud, false, ViStoryAd, 0, 7, {ArStreamClip, ArReactionBox, ArTextSlab}),
		// ---- K. life (never Marcus's errands or Sam's account; nothing names Dee)
		ViDef(1101, "K1", "A day in my life", 1.0, ViVlog | ViBkd, ViStory | ViQuest | ViPlain, false, ViLife, 0, 14, {ArNightShift, ArTextSlab, ArDeskSetup}),
		ViDef(1102, "K2", "Night shifts, then the grind", 1.1, ViVlog | ViBkd, ViStory | ViQuest | ViPlain, false, ViLife, 0, 14, {ArNightShift, ArTextSlab, ArTheGraph}),
		ViDef(1103, "K3", "Wash & Fold storytime", 0.9, ViVlog | ViBkd | ViCut, ViStory | ViQuest, false, ViLife, 0, 30, {ArTextSlab, ArNightShift, ArPayday}),
		ViDef(1104, "K4", "Apartment tour", 0.9, ViVlog | ViBkd, ViStory | ViPlain, false, ViLife, 0, 0, {ArDeskSetup, ArTextSlab, ArRentLetter}),
		ViDef(1105, "K5", "Setup tour", 1.0, ViVlog | ViBkd, ViStory | ViQuest | ViPlain, false, ViGear, 0, 30, {ArDeskSetup, ArTextSlab}),
		ViDef(1106, "K6", "Setup upgrade", 1.0, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViGear, 0, 7, {ArDeskSetup, ArBeforeAfter, ArTextSlab}),
		ViDef(1107, "K7", "Back after eviction", 1.5, ViVlog | ViBkd, ViStory | ViLoud | ViQuest, false, ViLife, 0, 0, {ArRentLetter, ArBeforeAfter, ArTextSlab}),
		ViDef(1108, "K8", "Evicted", 1.5, ViVlog | ViBkd, ViStory | ViLoud | ViQuest, false, ViLife, 0, 0, {ArRentLetter, ArTextSlab, ArReactionBox}),
		ViDef(1110, "K10", "Camera test", 0.8, ViCut, ViStory | ViPlain, false, ViGear, 0, 30, {ArDeskSetup, ArTextSlab, ArReactionBox}),
		// ---- L. the rival saga
		ViDef(1201, "L1", "Every pot against gh0stfold", 1.6, ViBkd | ViVlog, ViStory | ViLoud | ViPlain, false, ViRival, 0, 14, {ArTheGhost, ArVsSplit, ArTopFive}),
		ViDef(1202, "L2", "Who is gh0stfold?", 1.4, ViBkd, ViStory | ViQuest, false, ViRival, 0, 30, {ArTheGhost, ArVsSplit, ArTextSlab}),
		ViDef(1203, "L3", "Finally beat gh0stfold", 1.7, ViBkd | ViCut, ViStory | ViLoud, false, ViRival, 0, 7, {ArTheGhost, ArVsSplit}),
		// ---- M. sites (a strike under a clause when the site isn't the deal's)
		ViDef(1301, "M1", "First night on a site", 1.4, ViVlog | ViBkd, ViStory | ViQuest | ViPlain, false, ViResults, ViFlag, 0, {ArSiteCard, ArTextSlab, ArNightShift}),
		ViDef(1302, "M2", "Site against site", 1.3, ViBkd, ViTeach | ViQuest | ViPlain, true, ViTeachAd, ViFlag, 30, {ArVsSplit, ArSiteCard, ArClassroom}),
		ViDef(1303, "M3", "Flip & Run week", 1.2, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, ViFlag, 7, {ArSiteCard, ArTheGraph, ArNightShift}),
		ViDef(1304, "M4", "Rakeback explained", 1.1, ViBkd, ViTeach | ViQuest, true, ViTeachAd, ViLtd | ViFlag, 30, {ArPayday, ArSiteCard, ArClassroom}),
		ViDef(1305, "M5", "Welcome bonus walkthrough", 1.0, ViBkd, ViTeach | ViPlain, false, ViTeachAd, ViLtd | ViFlag, 30, {ArSiteCard, ArClassroom, ArPayday}),
		ViDef(1306, "M6", "Two Sunday majors", 1.4, ViBkd | ViVlog, ViStory | ViQuest | ViPlain, false, ViResults, ViFlag, 7, {ArVsSplit, ArSiteCard}),
		ViDef(1307, "M7", "Overlay hunting", 1.3, ViBkd, ViStory | ViTeach | ViQuest, true, ViTeachAd, ViFlag, 14, {ArSiteCard, ArClassroom}),
		ViDef(1308, "M8", "The Spark night", 1.2, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 6, {ArSiteCard, ArNightShift}),
		// ---- N. the rival sites' moments (each a strike under a RiverLine clause)
		ViDef(1401, "N1", "A FUSE at a big multiple", 1.8, ViBkd | ViCut, ViStory | ViLoud, false, ViResults, ViLtd | ViFlag, 7, {ArSiteCard, ArBigNumber, ArEnvelope}),
		ViDef(1402, "N2", "Payback", 1.6, ViBkd | ViCut, ViStory | ViLoud, false, ViRival, ViFlag, 7, {ArHeadhunter, ArVsSplit, ArSiteCard}),
		ViDef(1403, "N3", "I scalped a Headliner", 1.5, ViBkd | ViCut, ViStory | ViLoud, false, ViResults, ViFlag, 7, {ArHeadhunter, ArSiteCard, ArTrophyShot}),
		ViDef(1404, "N4", "Thirty days on Ignite", 1.1, ViBkd | ViVlog, ViStory | ViPlain, false, ViResults, ViLtd | ViFlag, 30, {ArSiteCard, ArDayCounter, ArTheGraph}),
		ViDef(1405, "N5", "Special guest", 1.3, ViBkd | ViCut, ViStory | ViLoud, false, ViResults, ViLtd | ViFlag, 7, {ArSiteCard, ArMilestone, ArReactionBox}),
		ViDef(1406, "N6", "Promoted in the League", 1.4, ViBkd | ViVlog, ViStory | ViLoud | ViPlain, false, ViResults, ViFlag, 25, {ArBeforeAfter, ArTrophyShot, ArSiteCard}),
		ViDef(1407, "N7", "Deadline Day", 1.5, ViBkd | ViVlog, ViStory | ViLoud, false, ViResults, ViFlag, 25, {ArTheBubble, ArDayCounter, ArSiteCard}),
		ViDef(1408, "N8", "Callout won", 1.2, ViBkd, ViStory | ViQuest, false, ViRival, ViFlag, 7, {ArVsSplit, ArTrophyShot, ArSiteCard}),
		ViDef(1409, "N9", "All six Team pros", 1.6, ViBkd | ViVlog, ViStory | ViLoud, false, ViResults, ViFlag, 0, {ArHeadhunter, ArTopFive, ArTrophyShot}),
		ViDef(1410, "N10", "The Club card re-mints", 1.0, ViBkd, ViStory | ViPlain, false, ViResults, ViLtd | ViFlag, 25, {ArPayday, ArSiteCard, ArTextSlab}),
		ViDef(1411, "N11", "Team patch day", 1.5, ViBkd | ViVlog, ViStory | ViLoud, false, ViResults, ViFlag, 0, {ArTrophyShot, ArSiteCard, ArBeforeAfter}),
		// ---- O. Cut-native
		ViDef(1501, "O1", "POV: the river", 1.0, ViCut, ViStory | ViPlain, false, -1, 0, 3, {ArCircledRiver}),
		ViDef(1502, "O2", "Rate this fold", 1.1, ViCut, ViStory | ViQuest, false, -1, 0, 3, {ArTheFold, ArQuiz}),
		ViDef(1503, "O3", "Guess my hand", 1.1, ViCut, ViLoud | ViQuest, false, -1, 0, 3, {ArTheRead, ArQuiz}),
		ViDef(1504, "O4", "One-word reaction", 1.0, ViCut, ViStory | ViLoud, false, -1, 0, 3, {ArStreamClip}),
		ViDef(1505, "O5", "Chat saw it first", 1.0, ViCut, ViStory | ViLoud, false, -1, 0, 3, {ArStreamClip}),
		ViDef(1506, "O6", "Out in level one", 0.9, ViCut, ViStory | ViLoud, false, -1, 0, 3, {ArGutPunch, ArMistake}),
		ViDef(1507, "O7", "The squeeze", 1.0, ViCut, ViStory | ViQuest, false, -1, 0, 3, {ArCircledRiver, ArVenue}),
		ViDef(1508, "O8", "The reveal", 1.5, ViCut, ViLoud | ViQuest, false, -1, 0, 3, {ArMonsterHand}),
		// ---- P. series (the SERIES tab; each episode a candidate of its own)
		ViDef(1601, "P1", "Bankroll challenge", 1.3, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 0, {ArDayCounter, ArBeforeAfter, ArTheGraph}),
		ViDef(1602, "P2", "Road to the Embercrest Sunday", 1.2, ViVlog | ViBkd, ViStory | ViPlain, false, ViLife, 0, 0, {ArDayCounter, ArVenue, ArRoadTrip}),
		ViDef(1603, "P3", "The satellite ladder", 1.2, ViVlog | ViBkd, ViStory | ViPlain, false, ViResults, 0, 0, {ArDayCounter, ArBeforeAfter, ArTrophyShot}),
		ViDef(1604, "P4", "A hundred Night Owls", 1.1, ViVlog | ViBkd, ViStory | ViPlain, false, ViResults, 0, 0, {ArDayCounter, ArNightShift, ArTheGraph}),
		ViDef(1605, "P5", "Learning Omaha", 1.1, ViVlog | ViBkd, ViStory | ViTeach, false, ViTeachAd, 0, 0, {ArDayCounter, ArClassroom, ArNightShift}),
		ViDef(1606, "P6", "Every site for a week", 1.2, ViVlog | ViBkd, ViStory | ViPlain, false, ViResults, ViFlag, 0, {ArDayCounter, ArSiteCard, ArTextSlab}),
		ViDef(1607, "P7", "A League season diary", 1.2, ViVlog | ViBkd, ViStory | ViPlain, false, ViResults, ViFlag, 0, {ArDayCounter, ArBeforeAfter, ArTextSlab}),
		ViDef(1608, "P8", "A hundred tournaments in a week", 1.2, ViVlog | ViBkd, ViStory | ViLoud | ViPlain, false, ViResults, 0, 0, {ArDayCounter, ArNightShift}),
		ViDef(1609, "P9", "A hypers-only week", 1.1, ViVlog | ViBkd, ViStory | ViPlain, false, ViResults, 0, 0, {ArDayCounter, ArNightShift, ArTheGraph}),
		ViDef(1610, "P10", "Climb the Season board", 1.1, ViVlog | ViBkd, ViStory | ViPlain, false, ViResults, 0, 0, {ArDayCounter, ArTierList, ArTheGraph}),
	};
	return All;
}

const IdeaDef* IdeaById(int Id)
{
	static const std::map<int, size_t> Index = [] {
		std::map<int, size_t> Out;
		const std::vector<IdeaDef>& All = IdeaTable();
		for (size_t I = 0; I < All.size(); ++I)
		{
			Out[All[I].Id] = I;
		}
		return Out;
	}();
	const auto It = Index.find(Id);
	return It == Index.end() ? nullptr : &IdeaTable()[It->second];
}

// ================================================================== reading the context

namespace
{
constexpr double ViDay = 1440.0;

std::string ViSiteName(int Site)
{
	return net::Def(static_cast<net::SiteId>(std::clamp(Site, 0, static_cast<int>(net::SiteId::Count) - 1))).Name;
}

std::string ViSigned(Chips Net)
{
	return Net > 0 ? "up " + Cash(Net) : Net < 0 ? "down " + Cash(-Net) : std::string("even");
}

std::string ViPlus(Chips Net)
{
	return Net >= 0 ? "+" + Cash(Net) : "-" + Cash(-Net);
}

std::string ViPct(double X)
{
	return std::to_string(static_cast<int>(std::lround(X * 100.0))) + "%";
}

int ViDayOf(double World)
{
	return static_cast<int>(std::floor(World / ViDay));
}

/** The night a result belongs to (a night runs to 10 AM, as the Vlog's). */
int ViNight(double At)
{
	return ViDayOf(At - 600.0);
}

struct ViSum
{
	int Events = 0, Cashes = 0, Wins = 0;
	Chips BuyIns = 0, Prizes = 0;
	Chips Net() const { return Prizes - BuyIns; }
};

ViSum ViSumOf(const std::vector<const IdeaResult*>& R)
{
	ViSum S;
	for (const IdeaResult* X : R)
	{
		++S.Events;
		S.Cashes += X->Prize > 0 ? 1 : 0;
		S.Wins += X->Place == 1 ? 1 : 0;
		S.BuyIns += X->BuyIn;
		S.Prizes += X->Prize;
	}
	return S;
}

std::vector<const IdeaResult*> ViWhere(const IdeaCtx& X, const std::function<bool(const IdeaResult&)>& Keep)
{
	std::vector<const IdeaResult*> Out;
	for (const IdeaResult& R : X.Results)
	{
		if (R.At >= 0.0 && Keep(R))
		{
			Out.push_back(&R);
		}
	}
	return Out;
}

/** The newest night with results (online, the last 36 hours), and its results. */
std::vector<const IdeaResult*> ViLastNight(const IdeaCtx& X, uint32_t Need = 0)
{
	int Night = -1;
	for (const IdeaResult& R : X.Results)
	{
		if (R.At >= 0.0 && R.At > X.World - 36.0 * 60.0 && (R.Flags & IfLive) == 0)
		{
			Night = std::max(Night, ViNight(R.At));
		}
	}
	return ViWhere(X, [&](const IdeaResult& R) { return Night >= 0 && ViNight(R.At) == Night && (R.Flags & IfLive) == 0 && (Need == 0 || (R.Flags & Need) != 0); });
}

Chips ViAbi(const IdeaCtx& X)
{
	return X.Events > 0 ? X.BuyIns / X.Events : 0;
}

/** A locked card's hint with its progress, as a sentence: "Play 20 hypers. So far: 4 of 20." (a need of one says only the hint). */
std::string ViProgress(const std::string& Hint, int Have, int Need)
{
	std::string Out = Hint;
	if (!Out.empty() && std::string(".!?").find(Out.back()) == std::string::npos)
	{
		Out += ".";
	}
	if (Need > 1)
	{
		Out += " So far: " + std::to_string(std::clamp(Have, 0, Need)) + " of " + std::to_string(Need) + ".";
	}
	return Out;
}

/** The candidate being built for a ready idea: the display facts and the moment's own fields. */
/** The gear on the player's desk as Desk Setup draws it (thumb fact d): the ids of what they own, in the catalog's order,
 *  '|'-separated ("monitor-24|mic-usb|plant"); "" when nothing. Rides in a setup idea's facts as "desk" (lower case: no
 *  title says it). */
std::string ViDeskIds(const IdeaCtx& X)
{
	std::string Out;
	for (const gear::Item& I : gear::Catalog())
	{
		if (std::any_of(X.Gear.begin(), X.Gear.end(), [&I](const std::pair<std::string, Chips>& G) { return G.first == I.Name; }))
		{
			Out += (Out.empty() ? "" : "|") + I.Id;
		}
	}
	return Out;
}

struct ViEval
{
	bool Ready = false;
	int Have = 0, Need = 0;
	std::string Fact, Hint, Src;
	std::vector<std::pair<std::string, std::string>> Facts;
	std::string Trend;
	bool Trending = false, Strike = false, Storytime = false, Rival = false, Series = false;
	std::string Event, Opponent;
	Chips BuyIn = 0, Prize = 0;
	int Field = 0, Place = 0, Site = 0, Viewers = 0;
	double At = -1;
	int Format = -1; // the format the board suggests (-1: the idea's first)
	std::string If;  // the title conditions it meets ("F" a first, "X" won, "U" up, "M" down, "R" rent short)
	void Set(const char* K, const std::string& V)
	{
		if (!V.empty())
		{
			Facts.push_back({K, V});
		}
	}
	void FromResult(const IdeaResult& R)
	{
		Event = R.Name;
		BuyIn = R.BuyIn;
		Field = R.Field;
		Place = R.Place;
		Prize = R.Prize;
		Site = R.Site;
		At = R.At;
		Set("Event", R.Name);
		if (R.BuyIn > 0)
		{
			Set("BuyIn", Cash(R.BuyIn));
		}
		if (R.Field > 1)
		{
			Set("Field", Grouped(R.Field));
		}
		if (R.Place > 0)
		{
			Set("Place", Ordinal(R.Place));
			static const char* const Words[] = {"", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth"};
			Set("PlaceWord", R.Place <= 10 ? std::string(Words[R.Place]) : Ordinal(R.Place));
		}
		if (R.Prize > 0)
		{
			Set("Prize", Cash(R.Prize));
		}
		Set("Weekday", net::WeekdayName(ViNight(R.At), true));
		Set("Site", ViSiteName(R.Site));
	}
};

/** The best open moment (MOMENTS) that passes Keep: the most dramatic, the newest of a tie. */
const Candidate* ViMoment(const Channel& C, const std::function<bool(const Candidate&)>& Keep)
{
	const Candidate* Best = nullptr;
	for (const Candidate& X : C.Open)
	{
		if (X.Source != 5 && Keep(X) && (!Best || X.Drama > Best->Drama || (X.Drama == Best->Drama && X.At > Best->At)))
		{
			Best = &X;
		}
	}
	return Best;
}

bool ViPair(const std::vector<Card>& H, int Rank)
{
	return H.size() >= 2 && RankOf(H[0]) == Rank && RankOf(H[1]) == Rank;
}

/** A world event of a kind in the log (a day old at least, the newest first), or null. */
const IdeaWorldEvent* ViWorld(const IdeaCtx& X, world::EventKind K, double Within)
{
	for (const IdeaWorldEvent& E : X.WorldLog)
	{
		if (E.Kind == static_cast<int>(K) && E.At <= X.World - ViDay && E.At >= X.World - Within)
		{
			return &E;
		}
	}
	return nullptr;
}

int ViKeptCount(const Channel& C, const std::function<bool(const Video&)>& Keep)
{
	return static_cast<int>(std::count_if(C.Videos.begin(), C.Videos.end(), Keep));
}

int ViUploads(const Channel& C)
{
	return std::max(C.X.UploadsEver, static_cast<int>(C.Videos.size()));
}

/** The channel's first upload as far as it keeps one: the kept videos, the lifetime top 20, the day Rerun 2.0 began. */
double ViFirstUpload(const Channel& C)
{
	double First = -1.0;
	auto Take = [&First](double At) { First = First < 0.0 ? At : std::min(First, At); };
	for (const Video& V : C.Videos)
	{
		Take(V.At);
	}
	for (const TopRow& R : C.X.Top)
	{
		Take(R.At);
	}
	if (C.X.Since >= 0)
	{
		Take(static_cast<double>(C.X.Since) * ViDay);
	}
	return First;
}

bool ViJoined(const IdeaCtx& X, int Site)
{
	return std::find(X.Joined.begin(), X.Joined.end(), Site) != X.Joined.end();
}

/** The next subscriber milestone below Subs (100, 1K, 10K, 100K), or 0. */
int ViMilestone(double Subs)
{
	int M = 0;
	for (const int Step : {100, 1000, 10000, 100000})
	{
		M = Subs >= Step ? Step : M;
	}
	return M;
}

/** One idea's state from the context: ready with its facts, or locked with what to do. */
void ViEvalIdea(const IdeaDef& D, const IdeaCtx& X, const Channel& C, double World, ViEval& E)
{
	const int Today = ViDayOf(World);
	int Y = 0, Mo = 0, Dm = 0;
	world::CivilDate(Today, Y, Mo, Dm);
	static const char* const Months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	auto MomentIdea = [&](const char* Hint, const std::function<bool(const Candidate&)>& Keep) {
		if (const Candidate* M = ViMoment(C, Keep))
		{
			E.Ready = true;
			E.Src = M->Id;
			E.At = M->At;
			E.Fact = std::string("In MOMENTS: ") + KindLabel(M->K) + (M->Event.empty() ? std::string() : " from the " + M->Event);
			return;
		}
		E.Hint = ViProgress(Hint, 0, 1);
	};
	auto Need = [&](int Have, int N, const std::string& Hint) {
		E.Have = Have;
		E.Need = N;
		if (Have >= N)
		{
			E.Ready = true;
			return true;
		}
		E.Hint = ViProgress(Hint, Have, N);
		return false;
	};
	switch (D.Id)
	{
	// ---- A. from MOMENTS
	case 101: MomentIdea("Get it all in ahead and lose: an all in at 60% or better that doesn't hold.", [](const Candidate& M) { return M.K == Kind::BadBeat; }); break;
	case 102: MomentIdea("Win an all in from behind: 40% or less, and the deck comes through.", [](const Candidate& M) { return M.K == Kind::Suckout; }); break;
	case 103: MomentIdea("A cooler: two hands of two pair or better, 40 big blinds or more, one way.", [](const Candidate& M) { return M.K == Kind::Cooler; }); break;
	case 104: MomentIdea("Both players turn over a set at the showdown.", [](const Candidate& M) { return M.K == Kind::SetOverSet; }); break;
	case 105:
		MomentIdea("Kings against aces at a showdown, either way round.", [](const Candidate& M) {
			for (const auto& S : M.Rec.Shown)
			{
				if ((ViPair(M.Rec.Mine, 11) && ViPair(S.second, 12)) || (ViPair(M.Rec.Mine, 12) && ViPair(S.second, 11)))
				{
					return true;
				}
			}
			return false;
		});
		break;
	case 106: MomentIdea("Win a pot of 15 big blinds or more without a showdown, with next to nothing.", [](const Candidate& M) { return M.K == Kind::Bluff; }); break;
	case 107:
		MomentIdea("Win a pot with seven-deuce and no showdown.", [](const Candidate& M) {
			if (M.K != Kind::Bluff || M.Rec.Mine.size() < 2)
			{
				return false;
			}
			const int A = std::max(RankOf(M.Rec.Mine[0]), RankOf(M.Rec.Mine[1])), B = std::min(RankOf(M.Rec.Mine[0]), RankOf(M.Rec.Mine[1]));
			return A == 5 && B == 0;
		});
		break;
	case 108: MomentIdea("A bluff that gets called and shown down.", [](const Candidate& M) { return M.K == Kind::BluffCaught; }); break;
	case 109: MomentIdea("Call a river bet with one pair or worse, and win.", [](const Candidate& M) { return M.K == Kind::HeroCall; }); break;
	case 110: MomentIdea("Fold a top-10% hand to a big bet.", [](const Candidate& M) { return M.K == Kind::HeroFold; }); break;
	case 111: MomentIdea("Win a pot of 30 big blinds or more.", [](const Candidate& M) { return M.K == Kind::BigPot || M.K == Kind::BiggestPot; }); break;
	case 112: MomentIdea("Knock someone out.", [](const Candidate& M) { return M.K == Kind::Knockout; }); break;
	case 113: MomentIdea("Knock out two players in one hand.", [](const Candidate& M) { return M.K == Kind::DoubleKo; }); break;
	case 114: MomentIdea("An all in at hand for hand on a money bubble.", [](const Candidate& M) { return M.K == Kind::Bubble; }); break;
	case 115: MomentIdea("Make quads or better.", [](const Candidate& M) { return M.K == Kind::Monster; }); break;
	case 116: MomentIdea("Open a mystery bounty envelope.", [](const Candidate& M) { return M.K == Kind::Envelope; }); break;
	case 117: MomentIdea("A pot of 20 big blinds or more against gh0stfold or a name people know.", [](const Candidate& M) { return M.K == Kind::Rival; }); break;
	case 118:
		MomentIdea("Make a Kast clip of a hand while you're live.", [](const Candidate& M) { return M.K == Kind::StreamMoment || M.Stream.ClipAt >= 0.0; });
		break;
	case 119: MomentIdea("Win (or lose) with two perfect cards in a row: under 5% after the flop.", [](const Candidate& M) { return M.K == Kind::RunnerRunner; }); break;
	case 120: MomentIdea("Fold a strong hand two places or fewer from a pay jump.", [](const Candidate& M) { return M.K == Kind::LadderFold; }); break;
	case 121: MomentIdea("Make the money with 5 big blinds or fewer when the bubble bursts.", [](const Candidate& M) { return M.K == Kind::MinCash; }); break;
	case 122: MomentIdea("Play the last all in of a tournament you finish first or second in.", [](const Candidate& M) { return M.K == Kind::HeadsUp; }); break;
	case 124: MomentIdea("The hand you bust a tournament on shows up here.", [](const Candidate& M) { return M.K == Kind::LastHand; }); break;
	// ---- B. nights and sessions
	case 201:
	{
		const std::vector<const IdeaResult*> N = ViLastNight(X);
		if (Need(static_cast<int>(N.size()), 4, "Play 4 or more online tournaments in one night"))
		{
			const ViSum S = ViSumOf(N);
			E.Set("Events", std::to_string(S.Events));
			E.Set("Cashes", std::to_string(S.Cashes));
			E.Set("NetSigned", ViSigned(S.Net()));
			E.Set("Weekday", net::WeekdayName(ViNight(N.front()->At), true));
			E.If = S.Net() >= 0 ? "U" : "M";
			E.Fact = std::to_string(S.Events) + " tournaments last night, " + ViSigned(S.Net());
			E.Src = "n" + std::to_string(ViNight(N.front()->At));
		}
		break;
	}
	case 202:
	{
		// 3+ events on a Sunday, a major among them (the newest Sunday in the last 8 days).
		std::vector<const IdeaResult*> Sun;
		int Day = -1;
		for (const IdeaResult& R : X.Results)
		{
			const int Nd = ViNight(R.At);
			if (R.At >= 0.0 && R.At > World - 8.0 * ViDay && net::Weekday(Nd) == 6 && (R.Flags & IfLive) == 0)
			{
				Day = std::max(Day, Nd);
			}
		}
		std::string Major;
		for (const IdeaResult& R : X.Results)
		{
			if (Day >= 0 && R.At >= 0.0 && ViNight(R.At) == Day && (R.Flags & IfLive) == 0)
			{
				Sun.push_back(&R);
				Major = (R.Flags & IfMajor) != 0 && Major.empty() ? R.Name : Major;
			}
		}
		if (Need(static_cast<int>(Sun.size()), 3, "Play 3 or more tournaments on a Sunday, a Sunday major among them") && !Major.empty())
		{
			const ViSum S = ViSumOf(Sun);
			E.Set("Events", std::to_string(S.Events));
			E.Set("Major", Major);
			E.Set("NetSigned", ViSigned(S.Net()));
			E.Fact = "Sunday: " + std::to_string(S.Events) + " tournaments and the " + Major;
			E.Src = "sun" + std::to_string(Day);
		}
		else if (E.Ready)
		{
			E.Ready = false;
			E.Hint = "Play a Sunday major on your Sunday (the Sunday Blaze, the Sunday 100K or the Embercrest Sunday).";
		}
		break;
	}
	case 204:
	{
		std::map<int, std::vector<const IdeaResult*>> ByDay;
		for (const IdeaResult& R : X.Results)
		{
			if (R.At >= 0.0 && R.At > World - 3.0 * ViDay && (R.Flags & IfSeries) != 0)
			{
				ByDay[ViNight(R.At)].push_back(&R);
			}
		}
		int Best = 0;
		for (const auto& Kv : ByDay)
		{
			Best = std::max(Best, static_cast<int>(Kv.second.size()));
		}
		if (Need(Best, 2, "Play 2 or more series events in one day") )
		{
			for (auto It = ByDay.rbegin(); It != ByDay.rend(); ++It)
			{
				if (It->second.size() >= 2)
				{
					const ViSum S = ViSumOf(It->second);
					E.Set("Series", X.SeriesNow.empty() ? std::string("series") : X.SeriesNow);
					E.Set("Events", std::to_string(S.Events));
					E.Set("NetSigned", ViSigned(S.Net()));
					E.Set("Weekday", net::WeekdayName(It->first, true));
					E.Series = true;
					E.Fact = std::to_string(S.Events) + " series events in a day, " + ViSigned(S.Net());
					E.Src = "sd" + std::to_string(It->first);
					break;
				}
			}
			if (X.SeriesNow.empty())
			{
				E.Facts.erase(std::remove_if(E.Facts.begin(), E.Facts.end(), [](const std::pair<std::string, std::string>& F) { return F.first == "Series"; }), E.Facts.end());
			}
		}
		break;
	}
	case 205:
	case 606:
	{
		const std::vector<const IdeaResult*> Sat = ViWhere(X, [&](const IdeaResult& R) { return (R.Flags & IfSeatWon) != 0 && R.At > World - 30.0 * ViDay; });
		if (Need(static_cast<int>(Sat.size()), 1, "Win a satellite seat"))
		{
			const IdeaResult& R = *Sat.front();
			E.FromResult(R);
			// "RCOP Main Mega Satellite" -> "RCOP Main Event"; "$330 Championship Satellite" -> "Championship".
			std::string Seat = R.Name;
			for (const char* Tail : {" Mega Satellite", " Satellite", " Mega"})
			{
				const size_t Sp = Seat.find(Tail);
				Seat = Sp != std::string::npos ? Seat.substr(0, Sp) : Seat;
			}
			if (!Seat.empty() && Seat[0] == '$' && Seat.find(' ') != std::string::npos)
			{
				Seat = Seat.substr(Seat.find(' ') + 1);
			}
			if (Seat.size() >= 5 && Seat.compare(Seat.size() - 5, 5, " Main") == 0)
			{
				Seat += " Event";
			}
			E.Set("Seat", Seat);
			E.Set("SeatValue", Cash(R.Prize));
			E.Fact = "A seat won in the " + R.Name;
			E.Src = R.Id;
		}
		break;
	}
	case 206:
	{
		const std::vector<const IdeaResult*> Fr = ViWhere(X, [&](const IdeaResult& R) { return (R.Flags & IfFreeroll) != 0 && R.Prize > 0 && R.At > World - 14.0 * ViDay; });
		if (Need(static_cast<int>(Fr.size()), 1, "Cash a freeroll"))
		{
			E.FromResult(*Fr.front());
			E.Fact = "A freeroll cash: " + Ordinal(Fr.front()->Place) + " of " + Grouped(Fr.front()->Field);
			E.Src = Fr.front()->Id;
		}
		break;
	}
	case 207:
	case 208:
	case 209:
	{
		const uint32_t Flag = D.Id == 207 ? IfHyper : D.Id == 208 ? IfOmaha : IfBounty;
		const int N = D.Id == 207 ? 10 : D.Id == 208 ? 2 : 3;
		const std::vector<const IdeaResult*> Night = ViLastNight(X, Flag);
		const char* What = D.Id == 207 ? "Play 10 or more hypers in one night" : D.Id == 208 ? "Play 2 or more Omaha events in one night" : "Play 3 or more bounty events in one night";
		if (Need(static_cast<int>(Night.size()), N, What))
		{
			const ViSum S = ViSumOf(Night);
			E.Set("Events", std::to_string(S.Events));
			E.Set("Cashes", std::to_string(S.Cashes));
			E.Set("NetSigned", ViSigned(S.Net()));
			E.Set("Weekday", net::WeekdayName(ViNight(Night.front()->At), true));
			if (D.Id == 209 && S.Prizes > 0)
			{
				E.Set("Bounties", Cash(S.Prizes));
			}
			E.Fact = std::to_string(S.Events) + (D.Id == 207 ? " hypers" : D.Id == 208 ? " Omaha events" : " bounty events") + " last night, " + ViSigned(S.Net());
			E.Src = "n" + std::to_string(ViNight(Night.front()->At));
		}
		break;
	}
	// ---- C. runs and titles
	case 301:
	case 302:
	case 303:
	case 305:
	{
		const auto Keep = [&](const IdeaResult& R) {
			if (R.At < World - 14.0 * ViDay || R.Prize <= 0)
			{
				return false;
			}
			if (D.Id == 301)
			{
				return R.Prize >= 3 * std::max<Chips>(1, R.BuyIn);
			}
			if (D.Id == 302)
			{
				return (R.Flags & IfFinal) != 0 || (R.Place > 0 && R.Place <= 9 && R.Field > 9);
			}
			if (D.Id == 303)
			{
				return R.Place == 1;
			}
			return R.Prize >= X.BestPrize && X.BestPrize > 0;
		};
		const std::vector<const IdeaResult*> R = ViWhere(X, Keep);
		const char* Hint = D.Id == 301 ? "Cash for three times your buy-in or more" : D.Id == 302 ? "Make a final table" : D.Id == 303 ? "Win a tournament" : "Beat your biggest cash";
		if (Need(static_cast<int>(R.size()), 1, Hint))
		{
			E.FromResult(*R.front());
			E.Src = R.front()->Id;
			const int Wins = X.Wins;
			E.If = D.Id == 303 && Wins <= 1 ? "FX" : R.front()->Place == 1 ? "X" : "";
			E.Fact = Ordinal(R.front()->Place) + " of " + Grouped(R.front()->Field) + " in the " + R.front()->Name + " for " + Cash(R.front()->Prize);
		}
		break;
	}
	case 304:
	{
		const std::vector<const IdeaResult*> R =
			ViWhere(X, [&](const IdeaResult& Rr) { return Rr.At > World - 14.0 * ViDay && ((Rr.Flags & IfFinal) != 0 || (Rr.Place > 0 && Rr.Place <= 9 && Rr.Field > 9)) && Rr.Hands >= 4; });
		if (Need(static_cast<int>(R.size()), 1, "Play 4 or more hands at a final table"))
		{
			E.FromResult(*R.front());
			E.Set("Hands", std::to_string(R.front()->Hands));
			E.Src = R.front()->Id;
			E.Fact = std::to_string(R.front()->Hands) + " hands at the " + R.front()->Name + " final table";
		}
		break;
	}
	case 306:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) {
			// One place short of the money: a field paid at about one in seven, the bubble place just outside it.
			const int Paid = std::max(1, static_cast<int>(std::lround(static_cast<double>(Rr.Field) * 0.15)));
			return Rr.At > World - 14.0 * ViDay && Rr.Prize <= 0 && Rr.Place == Paid + 1 && Rr.Field >= 10;
		});
		if (Need(static_cast<int>(R.size()), 1, "Bust one place from the money"))
		{
			E.FromResult(*R.front());
			E.Src = R.front()->Id;
			E.Fact = Ordinal(R.front()->Place) + " of " + Grouped(R.front()->Field) + ": the bubble";
		}
		break;
	}
	case 307:
	{
		const std::vector<const IdeaResult*> R =
			ViWhere(X, [&](const IdeaResult& Rr) { return Rr.At > World - 7.0 * ViDay && ((Rr.Flags & IfFinal) != 0 || (Rr.Place > 0 && Rr.Place <= 9 && Rr.Field > 9)); });
		if (Need(static_cast<int>(R.size()), 2, "Make 2 final tables in 7 days"))
		{
			E.FromResult(*R[1]);
			E.Set("Event2", R[0]->Name);
			E.Set("Days", std::to_string(std::max(1, ViDayOf(R[0]->At) - ViDayOf(R[1]->At))));
			E.Src = R[0]->Id;
			E.Fact = "Final tables in the " + R[1]->Name + " and the " + R[0]->Name;
		}
		break;
	}
	case 309:
	{
		std::string Award;
		for (const std::string& K : X.Stories)
		{
			Award = K == "bracelet-first" ? "bracelet" : K == "ring-first" ? "ring" : (K == "mm-title" || K == "series-title") && Award.empty() ? "series title" : Award;
		}
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return Rr.Place == 1 && (Rr.Flags & IfSeries) != 0; });
		if (Need(!Award.empty() && !R.empty() ? 1 : 0, 1, "Win a series event, a bracelet or a ring"))
		{
			E.FromResult(*R.front());
			E.Set("Award", Award);
			E.If = "X";
			E.Src = R.front()->Id;
			E.Fact = std::string(TakesAn(Award) ? "An " : "A ") + Award + ": the " + R.front()->Name;
		}
		break;
	}
	case 310:
	case 1307:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) {
			return Rr.At > World - 30.0 * ViDay && Rr.Prize > 0 && (Rr.Flags & IfOverlay) != 0 && (D.Id == 1307 || Rr.Site != static_cast<int>(net::SiteId::RiverLine));
		});
		if (Need(static_cast<int>(R.size()), 1, D.Id == 310 ? "Cash in a rival site's event that overlaid" : "Cash in an event that overlaid"))
		{
			E.FromResult(*R.front());
			E.Src = R.front()->Id;
			E.Strike = R.front()->Site != static_cast<int>(net::SiteId::RiverLine);
			E.Fact = "An overlay cash in the " + R.front()->Name + " on " + ViSiteName(R.front()->Site);
		}
		break;
	}
	case 312:
	{
		// Road to the Final Table (SPEC_CREATORS §3.7): a stream with a final table or better in a field of 100+, or a cash on
		// stream; the night's moments in MOMENTS and the stream's real peak.
		const Candidate* Run = ViMoment(C, [](const Candidate& M) {
			return (M.K == Kind::FinalTable || M.K == Kind::Win || M.K == Kind::DeepRun) && M.Stream.Viewers > 0 && (M.Field >= 100 || M.K == Kind::DeepRun);
		});
		if (!Run)
		{
			E.Hint = "Stream a deep run: a final table in a field of 100 or more, or a cash, live on Kast.";
			break;
		}
		const std::string Key = Run->Id.substr(0, Run->Id.rfind(':'));
		int Moments = 0;
		for (const Candidate& M : C.Open)
		{
			Moments += M.Id.rfind(Key + ":", 0) == 0 && M.Source != 5 ? 1 : 0;
		}
		int Peak = Run->Stream.Viewers;
		for (const IdeaStream& S : X.Streams)
		{
			Peak = Run->At >= S.Start && Run->At <= S.Start + S.Minutes + 30.0 ? std::max(Peak, S.Peak) : Peak;
		}
		E.Ready = true;
		E.Event = Run->Event;
		E.BuyIn = Run->BuyIn;
		E.Field = Run->Field;
		E.Place = Run->PlaceSoFar;
		E.Prize = Run->Rec.Pot;
		E.Site = Run->Site;
		E.At = Run->At;
		E.Viewers = Peak;
		E.Set("Event", Run->Event);
		E.Set("Field", Grouped(Run->Field));
		E.Set("Place", Ordinal(Run->PlaceSoFar));
		static const char* const Words[] = {"", "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth"};
		E.Set("PlaceWord", Run->PlaceSoFar >= 1 && Run->PlaceSoFar <= 10 ? std::string(Words[Run->PlaceSoFar]) : Ordinal(Run->PlaceSoFar));
		if (Run->Rec.Pot > 0)
		{
			E.Set("Prize", Cash(Run->Rec.Pot));
		}
		E.Set("Moments", std::to_string(std::max(1, Moments)));
		E.Set("Peak", Grouped(Peak));
		E.Set("Stage", Run->K == Kind::DeepRun ? "the money" : "the final table");
		E.Src = Key;
		E.Format = static_cast<int>(Format::Recap);
		E.Fact = "Your " + Run->Event + " run: " + std::to_string(std::max(1, Moments)) + (Moments == 1 ? " moment" : " moments");
		break;
	}
	// ---- D. live
	case 401:
	case 402:
	case 406:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) {
			if ((Rr.Flags & IfLive) == 0 || Rr.At < World - 7.0 * ViDay)
			{
				return false;
			}
			return D.Id == 401 || (D.Id == 402 && net::Weekday(ViDayOf(Rr.At)) == 6) || (D.Id == 406 && (Rr.Flags & IfMidnight) != 0 && Rr.Prize > 0);
		});
		const char* Hint = D.Id == 401 ? "Play a tournament at the Embercrest" : D.Id == 402 ? "Play the Embercrest's Sunday event" : "Cash the Midnight Turbo at the Embercrest";
		if (Need(static_cast<int>(R.size()), 1, Hint))
		{
			E.FromResult(*R.front());
			E.Src = R.front()->Id;
			E.If = X.LiveEvents <= 1 && D.Id == 401 ? "F" : "";
			E.Fact = "The " + R.front()->Name + ", " + net::WeekdayName(ViDayOf(R.front()->At), true);
		}
		break;
	}
	case 404:
	{
		const int Online = X.Events - X.LiveEvents;
		if (Need(std::min(X.LiveEvents * 20, Online), 100, "Play 5 live events and 100 online"))
		{
			E.Set("LiveEvents", std::to_string(X.LiveEvents));
			E.Set("Events", Grouped(std::max(0, Online)));
			E.Fact = std::to_string(X.LiveEvents) + " live events, " + Grouped(Online) + " online";
			E.Src = "y" + std::to_string(X.LiveEvents);
		}
		break;
	}
	case 405:
		MomentIdea("A hand at the back-room game (no names, no faces).", [](const Candidate& M) { return M.Storytime; });
		E.Storytime = E.Ready;
		if (E.Ready)
		{
			E.Fact = "A back-room hand in MOMENTS";
		}
		break;
	case 407:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return (Rr.Flags & IfLive) != 0 && Rr.At > World - 3.0 * ViDay; });
		// The night ended after 2 AM: the last live entry's end isn't kept, so a late start (the Midnight Turbo) stands for it.
		const IdeaResult* Late = nullptr;
		for (const IdeaResult* Rr : R)
		{
			Late = (Rr->Flags & IfMidnight) != 0 ? Rr : Late;
		}
		if (Need(Late ? 1 : 0, 1, "Play a live night that ends after 2 AM"))
		{
			E.FromResult(*Late);
			E.Set("Time", net::TimeLabel(std::fmod(Late->At, ViDay) + 150.0));
			E.Set("NetSigned", ViSigned(Late->Prize - Late->BuyIn));
			E.Src = Late->Id;
			E.Fact = "The bus home after the " + Late->Name;
		}
		break;
	}
	// ---- E. bankroll and money
	case 501:
	{
		const int Start = world::MonthStart(Today);
		const std::vector<const IdeaResult*> M = ViWhere(X, [&](const IdeaResult& R) { return ViDayOf(R.At) >= Start; });
		if (Need(static_cast<int>(M.size()), 20, "Play 20 events this month"))
		{
			const ViSum S = ViSumOf(M);
			E.Set("Month", Months[Mo - 1]);
			E.Set("Events", std::to_string(S.Events));
			E.Set("Cashes", std::to_string(S.Cashes));
			E.Set("NetSigned", ViSigned(S.Net()));
			E.Set("Net", Cash(std::abs(S.Net())));
			E.Set("NetPlus", ViPlus(S.Net()));
			E.Set("Roi", S.BuyIns > 0 ? ViPct(static_cast<double>(S.Net()) / static_cast<double>(S.BuyIns)) : std::string());
			E.Set("Abi", S.Events > 0 ? Cash(S.BuyIns / S.Events) : std::string());
			bool First = S.Net() >= 100000;
			for (const Snap& Sn : C.X.Snaps)
			{
				First = First && Sn.Net < 100000;
			}
			E.If = std::string(S.Net() >= 0 ? "U" : "M") + (First ? "F" : "");
			E.Fact = std::string(Months[Mo - 1]) + " so far: " + std::to_string(S.Events) + " tournaments, " + ViSigned(S.Net());
			E.Src = "m" + std::to_string(world::MonthKeyOf(Today));
		}
		break;
	}
	case 503:
	case 504:
	{
		const int Days = D.Id == 503 ? 30 : 14;
		const ViSum S = ViSumOf(ViWhere(X, [&](const IdeaResult& R) { return R.At > World - static_cast<double>(Days) * ViDay; }));
		const Chips Abi = std::max<Chips>(1, S.Events > 0 ? S.BuyIns / S.Events : ViAbi(X));
		const int Swing = static_cast<int>(std::lround(static_cast<double>(D.Id == 503 ? -S.Net() : S.Net()) / static_cast<double>(Abi)));
		const int NeedN = D.Id == 503 ? 10 : 20;
		if (Need(S.Events >= 10 ? Swing : 0, NeedN, D.Id == 503 ? "Ten average buy-ins down over 30 days (hope it doesn't happen)" : "Twenty average buy-ins up over 14 days"))
		{
			E.Set(D.Id == 503 ? "Down" : "Up", std::to_string(Swing));
			E.Set("Days", std::to_string(Days));
			E.Set("Events", std::to_string(S.Events));
			E.Set("NetSigned", ViSigned(S.Net()));
			E.Set("Abi", Cash(Abi));
			E.Fact = std::to_string(Swing) + " buy-ins " + (D.Id == 503 ? "down" : "up") + " in " + std::to_string(Days) + " days";
			E.Src = "s" + std::to_string(Today);
		}
		break;
	}
	case 505:
	{
		const bool Told = std::find(X.Stories.begin(), X.Stories.end(), "rent-paid") != X.Stories.end();
		if (Need(Told && X.LastRentAt > World - 14.0 * ViDay ? 1 : 0, 1, "Pay a month's rent with what poker brought in"))
		{
			int Ry = 0, Rm = 0, Rd = 0;
			world::CivilDate(ViDayOf(X.LastRentAt), Ry, Rm, Rd);
			E.Set("Rent", Cash(1075 * 100));
			E.Set("Month", Months[std::clamp(Rm, 1, 12) - 1]);
			E.Fact = "Poker paid " + std::string(Months[std::clamp(Rm, 1, 12) - 1]) + "'s rent";
			E.Src = "r" + std::to_string(ViDayOf(X.LastRentAt));
		}
		break;
	}
	case 506:
	{
		const double Left = (X.RentDeadline - World) / ViDay;
		const Chips Short = X.RentDue - X.Bankroll;
		if (Need(X.RentStage != 1 && Left > 0.0 && Left <= 3.0 && Short > 0 ? 1 : 0, 1, "Rent due in 3 days or fewer, and the bank short of it"))
		{
			E.Set("Rent", Cash(X.RentDue));
			E.Set("Short", Cash(Short));
			E.Set("Days", std::to_string(std::max(1, static_cast<int>(std::ceil(Left)))));
			E.Set("Weekday", net::WeekdayName(ViDayOf(X.RentDeadline - 1.0), true));
			E.If = "R";
			E.Fact = "Rent's due " + std::string(net::WeekdayName(ViDayOf(X.RentDeadline - 1.0), true)) + ": " + Cash(Short) + " short";
			E.Src = "rw" + std::to_string(ViDayOf(X.RentDeadline));
		}
		break;
	}
	case 508:
	{
		int Mark = 0;
		for (const int M : {100, 500, 1000, 5000})
		{
			Mark = X.Events >= M ? M : Mark;
		}
		if (Need(X.Events, Mark > 0 ? Mark : 100, "Play 100 tournaments"))
		{
			E.Set("Events", Grouped(Mark));
			E.Set("Roi", X.BuyIns > 0 ? ViPct(static_cast<double>(X.Prizes - X.BuyIns) / static_cast<double>(X.BuyIns)) : std::string());
			E.Set("Itm", X.Events > 0 ? ViPct(static_cast<double>(X.Cashes) / static_cast<double>(X.Events)) : std::string());
			E.Set("Abi", Cash(ViAbi(X)));
			E.Set("NetSigned", ViSigned(X.Prizes - X.BuyIns));
			E.Fact = Grouped(X.Events) + " tournaments played";
			E.Src = "roi" + std::to_string(Mark);
		}
		break;
	}
	case 509:
	{
		// The last 50 against the 50 before them: an average buy-in that crossed a band (micro, low, mid).
		std::vector<Chips> B;
		for (const IdeaResult& R : X.Results)
		{
			if ((R.Flags & IfFreeroll) == 0)
			{
				B.push_back(R.BuyIn);
			}
		}
		Chips Now = 0, Was = 0;
		const size_t Half = std::min<size_t>(50, B.size() / 2);
		for (size_t I = 0; I < Half; ++I)
		{
			Now += B[I];
			Was += B[Half + I];
		}
		Now = Half > 0 ? Now / static_cast<Chips>(Half) : 0;
		Was = Half > 0 ? Was / static_cast<Chips>(Half) : 0;
		const bool Crossed = Half >= 20 && net::TierOf(Now) > net::TierOf(Was) && Was > 0;
		if (Need(Crossed ? 1 : 0, 1, "Move up: your average buy-in over your last tournaments moves into the next stakes"))
		{
			E.Set("Abi", Cash(Now));
			E.Set("AbiWas", Cash(Was));
			E.Set("Events", std::to_string(static_cast<int>(Half) * 2));
			E.Fact = "Your average buy-in: " + Cash(Was) + " to " + Cash(Now);
			E.Src = "up" + std::to_string(static_cast<int>(net::TierOf(Now)));
		}
		break;
	}
	case 510:
	{
		const Chips Abi = ViAbi(X);
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return Rr.At > World - 7.0 * ViDay && Abi > 0 && Rr.BuyIn >= 5 * Abi; });
		if (Need(static_cast<int>(R.size()), 1, "Play an event at five times your average buy-in"))
		{
			E.FromResult(*R.front());
			E.Set("Abi", Cash(Abi));
			E.Src = R.front()->Id;
			E.Fact = std::string(TakesAn(Cash(R.front()->BuyIn)) ? "An " : "A ") + Cash(R.front()->BuyIn) + " shot (you average " + Cash(Abi) + ")"; // "An $11 shot"
		}
		break;
	}
	case 511:
	{
		if (Need(Today >= net::NightOneDay + 365 ? 1 : 0, 1, "A year from your first night"))
		{
			E.Set("Events", Grouped(X.Events));
			E.Set("Cashes", Grouped(X.Cashes));
			E.Set("Wins", std::to_string(X.Wins));
			E.Set("NetSigned", ViSigned(X.Prizes - X.BuyIns));
			if (X.BestPrize > 0)
			{
				E.Set("Best", Cash(X.BestPrize));
			}
			E.Fact = "A year in: " + Grouped(X.Events) + " tournaments";
			E.Src = "y1";
		}
		break;
	}
	case 512:
	{
		// A Rerun payout in the ledger's months, every 90 days.
		const bool Paid = C.PaidCents > 0;
		if (Need(Paid ? 1 : 0, 1, "Get paid by Rerun (Partner, and a $100 balance on the 21st)"))
		{
			E.Set("Paid", Cash(C.PaidCents));
			E.Set("Month", Months[Mo - 1]);
			double Views = 0.0;
			for (const Video& V : C.Videos)
			{
				Views += V.P;
			}
			E.Set("Views", Grouped(static_cast<int64_t>(std::llround(Views))));
			double Rpm = 0.0, Rn = 0.0;
			for (const Video& V : C.Videos)
			{
				if (V.Rpm > 0.0 && V.F != Format::Short)
				{
					Rpm += V.Rpm;
					Rn += 1.0;
				}
			}
			if (Rn > 0.0)
			{
				// The RPM Analytics shows (StatsOf): past the top-earner rate's T this pay month, today's views pay the month's share.
				const double Share = valgo::TopRateShare(valgo::PaidThisMonth(C, Today));
				E.Set("Rpm", Cash(static_cast<Chips>(std::llround(Rpm / Rn * (Share < 1.0 ? Share : 1.0) * 100.0))));
			}
			E.Fact = "Rerun has paid you " + Cash(C.PaidCents);
			E.Src = "pay" + std::to_string(static_cast<long long>(C.PaidCents));
		}
		break;
	}
	// ---- F. strategy and study
	case 601:
	case 602:
	{
		const int Want = D.Id == 601 ? 0 : 3;
		const IdeaGrade* G = nullptr;
		for (const IdeaGrade& Gr : X.Grades)
		{
			if (Gr.Grade == Want && Gr.At > World - 14.0 * ViDay && !Gr.Hand.empty())
			{
				G = &Gr;
				break;
			}
		}
		if (Need(G ? 1 : 0, 1, D.Id == 601 ? "Make a decision the coach grades Best" : "Make a decision the coach grades a Mistake (it happens)"))
		{
			E.Set("Hand", G->Hand);
			E.Set("Street", G->Street);
			E.Set("Event", G->Event);
			if (G->PotBb >= 1.0)
			{
				E.Set("Bb", Grouped(static_cast<int64_t>(std::llround(G->PotBb))));
			}
			if (G->CostBb >= 1.0)
			{
				E.Set("CostBb", Grouped(static_cast<int64_t>(std::llround(G->CostBb))));
			}
			E.Fact = (D.Id == 601 ? "A Best decision: " : "A Mistake: ") + G->Hand + " on the " + G->Street;
			E.Src = "g" + std::to_string(static_cast<long long>(std::llround(G->At)));
		}
		break;
	}
	case 603:
	case 612:
		MomentIdea(D.Id == 603 ? "A bubble hand or a big fold in MOMENTS." : "Fold aces or kings to a big bet.", [&](const Candidate& M) {
			if (D.Id == 603)
			{
				return M.K == Kind::Bubble || M.K == Kind::HeroFold || M.K == Kind::LadderFold;
			}
			return (M.K == Kind::HeroFold || M.K == Kind::LadderFold) && (ViPair(M.Rec.Mine, 12) || ViPair(M.Rec.Mine, 11));
		});
		break;
	case 604:
	case 605:
	case 607:
	{
		const uint32_t Flag = D.Id == 604 ? IfHyper : D.Id == 605 ? IfBounty : IfMystery;
		const int N = D.Id == 604 ? 20 : D.Id == 605 ? 10 : 5;
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return (Rr.Flags & Flag) != 0; });
		const char* What = D.Id == 604 ? "Play 20 hypers" : D.Id == 605 ? "Play 10 bounty events" : "Play 5 mystery bounty events";
		if (Need(static_cast<int>(R.size()), N, What))
		{
			E.Set("Events", std::to_string(R.size()));
			Chips Bounties = 0;
			for (const IdeaResult* Rr : R)
			{
				Bounties += Rr->Prize;
			}
			if (D.Id == 605 && Bounties > 0)
			{
				E.Set("Bounties", Cash(Bounties));
			}
			if (D.Id == 604)
			{
				for (const IdeaGrade& G : X.Grades)
				{
					if (G.Shove && G.StackBb >= 1.0 && G.StackBb <= 15.0 && G.Grade != 2)
					{
						// A shove graded Best or Good is what the chart says; a shove graded a Mistake was a fold.
						E.Set("StackBb", std::to_string(static_cast<int>(std::lround(G.StackBb))));
						E.Set("ChartSays", G.Grade <= 1 ? "push" : "fold");
						break;
					}
				}
			}
			E.Fact = std::to_string(R.size()) + (D.Id == 604 ? " hypers played" : D.Id == 605 ? " bounty events played" : " mystery bounty events played");
			E.Src = "f" + std::to_string(D.Id) + "-" + std::to_string(R.size() / 10);
		}
		break;
	}
	case 608:
	case 609:
	{
		int N = 0;
		for (const IdeaGrade& G : X.Grades)
		{
			N += (D.Id == 608 ? G.River && G.At > World - 7.0 * ViDay : G.Shove && G.StackBb <= 15.0) ? 1 : 0;
		}
		if (Need(N, D.Id == 608 ? 3 : 10, D.Id == 608 ? "Make 3 river decisions the coach grades this week" : "Make 10 graded shoves with 15 big blinds or fewer"))
		{
			E.Set("Count", std::to_string(D.Id == 608 ? std::min(N, 5) : N));
			E.Fact = std::to_string(N) + (D.Id == 608 ? " graded river decisions this week" : " graded short-stack shoves");
			E.Src = "q" + std::to_string(Today / 7);
		}
		break;
	}
	case 610:
	{
		const std::vector<const IdeaResult*> R =
			ViWhere(X, [&](const IdeaResult& Rr) { return Rr.At > World - 30.0 * ViDay && ((Rr.Flags & IfFinal) != 0 || (Rr.Place > 0 && Rr.Place <= 9 && Rr.Field > 9)); });
		if (Need(static_cast<int>(R.size()), 1, "Make a final table"))
		{
			E.FromResult(*R.front());
			E.Src = R.front()->Id;
			E.Fact = "Your final table in the " + R.front()->Name;
		}
		break;
	}
	case 611:
		if (Need(X.StudyWeek, 1, "Book a coaching session this week"))
		{
			E.Set("Count", std::to_string(X.StudyWeek));
			E.Fact = std::to_string(X.StudyWeek) + (X.StudyWeek == 1 ? " coaching session this week" : " coaching sessions this week");
			E.Src = "c" + std::to_string(Today / 7);
		}
		break;
	case 613:
	{
		std::map<int, std::vector<const IdeaResult*>> BySite;
		for (const IdeaResult& R : X.Results)
		{
			if ((R.Flags & IfLive) == 0)
			{
				BySite[R.Site].push_back(&R);
			}
		}
		int Site = -1;
		size_t Most = 0;
		for (const auto& Kv : BySite)
		{
			if (Kv.second.size() > Most)
			{
				Most = Kv.second.size();
				Site = Kv.first;
			}
		}
		if (Need(static_cast<int>(Most), 50, "Play 50 events on one site"))
		{
			const ViSum S = ViSumOf(BySite[Site]);
			E.Site = Site;
			E.Set("Site", ViSiteName(Site));
			E.Set("Events", std::to_string(S.Events));
			E.Set("Roi", S.BuyIns > 0 ? ViPct(static_cast<double>(S.Net()) / static_cast<double>(S.BuyIns)) : std::string());
			E.Strike = Site != static_cast<int>(net::SiteId::RiverLine);
			E.Fact = std::to_string(S.Events) + " events on " + ViSiteName(Site);
			E.Src = "site" + std::to_string(Site);
		}
		break;
	}
	case 614:
	case 713:
		if (Need(ViUploads(C), 30, "Post 30 videos"))
		{
			E.Fact = "A lesson anyone can use";
			E.Src = "basics";
		}
		break;
	// ---- G. lists and compilations
	case 701:
	{
		int N = 0;
		for (const Candidate& M : C.Open)
		{
			N += M.Source != 5 && M.At > World - 7.0 * ViDay ? 1 : 0;
		}
		if (Need(N, 5, "Have 5 moments from the last 7 days in MOMENTS"))
		{
			E.Set("Count", std::to_string(std::min(N, 5)));
			E.Fact = std::to_string(N) + " moments this week";
			E.Src = "w" + std::to_string(Today / 7);
		}
		break;
	}
	case 702:
	{
		int Bluffs = 0, Beats = 0, Calls = 0;
		for (const Candidate& M : C.Open)
		{
			Bluffs += M.K == Kind::Bluff ? 1 : 0;
			Beats += M.K == Kind::BadBeat || M.K == Kind::Cooler || M.K == Kind::SetOverSet ? 1 : 0;
			Calls += M.K == Kind::HeroCall ? 1 : 0;
		}
		const char* What = Beats >= 4 ? "bad beats" : Bluffs >= 4 ? "bluffs" : Calls >= 3 ? "hero calls" : nullptr;
		if (Need(What ? 1 : 0, 1, "Four bluffs, four beats or three hero calls in MOMENTS"))
		{
			E.Set("What", What);
			E.Set("Count", std::to_string(Beats >= 4 ? Beats : Bluffs >= 4 ? Bluffs : Calls));
			E.Set("Month", Months[Mo - 1]);
			E.Set("Year", std::to_string(Y));
			if (Beats >= 4)
			{
				E.Set("WorstOf", "beats");
			}
			E.Fact = std::string("The month's ") + What;
			E.Src = std::string("best-") + What;
		}
		break;
	}
	case 705:
	{
		std::map<std::string, int> AllIns;
		std::string Ev;
		for (const Candidate& M : C.Open)
		{
			bool AllIn = false;
			for (const recall::Line& L : M.Rec.Lines)
			{
				AllIn = AllIn || (L.Who == "You" && L.What == "all in");
			}
			if (AllIn && net::Weekday(ViNight(M.At)) == 6)
			{
				const std::string Key = M.Id.substr(0, M.Id.rfind(':'));
				if (++AllIns[Key] >= 4)
				{
					Ev = M.Event;
				}
			}
		}
		int Best = 0;
		for (const auto& Kv : AllIns)
		{
			Best = std::max(Best, Kv.second);
		}
		if (Need(Best, 4, "Four all ins in one Sunday event"))
		{
			E.Set("Count", std::to_string(Best));
			E.Set("Event", Ev);
			E.Fact = std::to_string(Best) + " all ins in the " + Ev;
			E.Src = "ai" + std::to_string(Today / 7);
		}
		break;
	}
	case 708:
	case 709:
	{
		std::set<std::string> Names;
		for (const IdeaResult& R : X.Results)
		{
			if (D.Id == 708 ? (R.Site == static_cast<int>(net::SiteId::RiverLine) && (R.Flags & IfLive) == 0) : (R.Flags & IfLive) != 0)
			{
				Names.insert(R.Name);
			}
		}
		const int N = static_cast<int>(Names.size());
		if (Need(N, D.Id == 708 ? 10 : 4, D.Id == 708 ? "Play 10 different RiverLine events" : "Play 4 different Embercrest events"))
		{
			E.Set("Count", std::to_string(N));
			E.Fact = std::to_string(N) + (D.Id == 708 ? " RiverLine events played" : " Embercrest events played");
			E.Src = "rank" + std::to_string(N);
		}
		break;
	}
	case 710:
	{
		std::map<int, int> BySite;
		for (const IdeaResult& R : X.Results)
		{
			BySite[R.Site] += (R.Flags & IfLive) == 0 ? 1 : 0;
		}
		int Sites = 0, Events = 0;
		for (const auto& Kv : BySite)
		{
			Sites += Kv.second > 0 ? 1 : 0;
			Events += Kv.second;
		}
		if (Need(Sites >= 2 ? Events : 0, 20, "Play 20 events on 2 sites or more"))
		{
			E.Set("Sites", std::to_string(Sites));
			E.Set("Events", std::to_string(Events));
			E.Strike = true;
			E.Fact = std::to_string(Sites) + " sites, " + std::to_string(Events) + " events";
			E.Src = "sites" + std::to_string(Sites);
		}
		break;
	}
	case 711:
	{
		Chips Cost = 0;
		for (const auto& G : X.Gear)
		{
			Cost += G.second;
		}
		if (Need(static_cast<int>(X.Gear.size()), 5, "Own 5 pieces of gear"))
		{
			E.Set("Items", std::to_string(X.Gear.size()));
			E.Set("GearCost", Cash(Cost));
			E.Set("desk", ViDeskIds(X));
			E.Fact = std::to_string(X.Gear.size()) + " pieces of gear, " + Cash(Cost);
			E.Src = "gear" + std::to_string(X.Gear.size());
		}
		break;
	}
	case 712:
	{
		int Mistakes = 0;
		for (const IdeaGrade& G : X.Grades)
		{
			Mistakes += G.Grade == 3 ? 1 : 0;
		}
		const bool YearEnd = Mo == 12 && Dm >= 15;
		if (Need(YearEnd ? Mistakes : 0, 10, "At the year's end, with 10 graded Mistakes to learn from"))
		{
			E.Set("Count", std::to_string(std::min(Mistakes, 10)));
			E.Set("Year", std::to_string(Y));
			E.Fact = std::to_string(Mistakes) + " graded Mistakes this year";
			E.Src = "y" + std::to_string(Y);
		}
		break;
	}
	case 714:
	{
		int N = 0;
		for (const Candidate& M : C.Open)
		{
			N += M.Source != 5 && M.At > World - 7.0 * ViDay && M.Rec.Board.size() >= 5 ? 1 : 0;
		}
		if (Need(N, 10, "Have 10 hands that saw a river in MOMENTS this week"))
		{
			E.Set("Count", std::to_string(N));
			E.Fact = std::to_string(N) + " rivers this week";
			E.Src = "rv" + std::to_string(Today / 7);
		}
		break;
	}
	case 715:
	{
		double Top = 0.0;
		const int Cuts = ViKeptCount(C, [&](const Video& V) {
			if (V.F == Format::Short)
			{
				Top = std::max(Top, V.P);
				return true;
			}
			return false;
		});
		if (Need(Cuts, 10, "Post 10 Cuts"))
		{
			E.Set("Count", std::to_string(std::min(Cuts, 10)));
			E.Set("Views", Grouped(static_cast<int64_t>(std::llround(Top))));
			E.Fact = std::to_string(Cuts) + " Cuts posted";
			E.Src = "cuts" + std::to_string(Cuts / 10);
		}
		break;
	}
	// ---- H. reactions and news (the world's, a day old)
	case 801:
	case 802:
	{
		const IdeaWorldEvent* W = ViWorld(X, D.Id == 801 ? world::EventKind::Champion : world::EventKind::FinalTable, 4.0 * ViDay);
		if (Need(W ? 1 : 0, 1, D.Id == 801 ? "A major gets its champion (Research shows the week's news)" : "A major's final table plays out"))
		{
			E.Set(D.Id == 801 ? "Winner" : "Npc", W->Who);
			E.Set("Major", W->What);
			if (W->Amount > 0)
			{
				E.Set("Prize", Cash(W->Amount));
			}
			E.Trend = (D.Id == 801 ? "champ:" : "final:") + std::to_string(static_cast<long long>(std::llround(W->At)));
			E.Trending = true;
			E.Fact = W->Who + (D.Id == 801 ? " won the " : " made the final table of the ") + W->What;
			E.Src = "w" + std::to_string(static_cast<long long>(std::llround(W->At)));
		}
		break;
	}
	case 803:
	{
		const bool Season = (Mo == 11 && Dm >= 15) || Mo == 12;
		const IdeaWorldEvent* Lead = nullptr;
		for (const IdeaWorldEvent& W : X.WorldLog)
		{
			Lead = !Lead && W.Kind == static_cast<int>(world::EventKind::Won) ? &W : Lead;
		}
		if (Need(Season && Lead ? 1 : 0, 1, "The Player of the Year race runs from November 15"))
		{
			E.Set("Leader", Lead->Who);
			E.Set("Year", std::to_string(Y));
			E.Trend = "poy:" + std::to_string(Y);
			E.Trending = true;
			E.Fact = "The " + std::to_string(Y) + " Player of the Year race";
			E.Src = "poy" + std::to_string(Y);
		}
		break;
	}
	case 804:
	{
		const IdeaWorldEvent* W = ViWorld(X, world::EventKind::HallOfFame, 30.0 * ViDay);
		if (Need(W ? 1 : 0, 1, "The Hall of Fame names its class (July 15)"))
		{
			E.Set("Npc", W->Who);
			E.Set("Year", std::to_string(Y));
			E.Trend = "hof:" + std::to_string(Y);
			E.Trending = true;
			E.Fact = W->Who + " is in the Hall of Fame";
			E.Src = "hof" + std::to_string(Y);
		}
		break;
	}
	case 805:
	case 807:
	{
		const IdeaWorldEvent* W = ViWorld(X, D.Id == 805 ? world::EventKind::SiteLaunched : world::EventKind::SiteOverlay, (D.Id == 805 ? 14.0 : 7.0) * ViDay);
		if (Need(W ? 1 : 0, 1, D.Id == 805 ? "A rival site opens its lobby" : "A rival site's guarantee isn't met this week"))
		{
			E.Site = W->Site;
			E.Set("Site", ViSiteName(W->Site));
			if (D.Id == 807)
			{
				E.Set("Event", W->What);
				if (W->Amount > 0)
				{
					E.Set("Overlay", Cash(W->Amount));
				}
			}
			E.Trend = (D.Id == 805 ? "site:" : "overlay:") + std::to_string(static_cast<long long>(std::llround(W->At)));
			E.Trending = true;
			E.Strike = true;
			E.Fact = D.Id == 805 ? ViSiteName(W->Site) + " is open" : "An overlay on " + ViSiteName(W->Site);
			E.Src = "w" + std::to_string(static_cast<long long>(std::llround(W->At)));
		}
		break;
	}
	case 806:
		if (Need(!X.SeriesNext.empty() && X.SeriesNextAt >= World && X.SeriesNextAt <= World + 3.0 * ViDay ? 1 : 0, 1, "A series starts within 3 days"))
		{
			E.Set("Series", X.SeriesNext);
			E.Fact = X.SeriesNext + " starts " + net::WeekdayName(ViDayOf(X.SeriesNextAt), true);
			E.Src = "sp" + std::to_string(ViDayOf(X.SeriesNextAt));
		}
		break;
	case 808:
	{
		const IdeaWorldEvent* W = ViWorld(X, world::EventKind::BoardReport, 8.0 * ViDay);
		if (Need(W ? 1 : 0, 1, "The Season board's weekly report"))
		{
			E.Set("Leader", W->Who);
			E.Set("Count", "10"); // the video's rundown: the board's top ten
			E.Fact = W->Who + " leads the Season board";
			E.Src = "lb" + std::to_string(Today / 7);
		}
		break;
	}
	case 809:
	{
		const IdeaWorldEvent* W = ViWorld(X, world::EventKind::Rivalry, 14.0 * ViDay);
		if (Need(W && !W->Other.empty() ? 1 : 0, 1, "Two players keep meeting at final tables"))
		{
			E.Set("Npc", W->Who);
			E.Set("Other", W->Other);
			E.Fact = W->Who + " and " + W->Other;
			E.Src = "rv" + std::to_string(static_cast<long long>(std::llround(W->At)));
		}
		break;
	}
	case 810:
	{
		const bool Done = std::any_of(C.X.IdeaCool.begin(), C.X.IdeaCool.end(), [](const Cool& Cl) { return Cl.Key == 810 && Cl.Count > 0; });
		if (Need(Done ? 0 : ViUploads(C), 50, "Post 50 videos"))
		{
			E.Set("Videos", std::to_string(ViUploads(C)));
			const Video* First = nullptr;
			for (const Video& V : C.Videos)
			{
				First = !First || V.At < First->At ? &V : First;
			}
			if (First)
			{
				E.Set("Views", Grouped(static_cast<int64_t>(std::llround(First->P))));
			}
			E.Fact = std::to_string(ViUploads(C)) + " videos posted";
			E.Src = "first";
		}
		break;
	}
	// ---- I. community and milestones
	case 901:
	{
		const int M = ViMilestone(C.Subs);
		if (Need(M > 0 && X.SubsWeekAgo < M ? 1 : 0, 1, "Reach 100 subscribers (or the next milestone) this week"))
		{
			E.Set("Subs", Grouped(M));
			E.Fact = Grouped(M) + " subscribers";
			E.Src = "sub" + std::to_string(M);
		}
		break;
	}
	case 902:
		if (Need(static_cast<int>(std::floor(C.Subs)), 1000, "Reach 1,000 subscribers"))
		{
			E.Set("Subs", Grouped(static_cast<int64_t>(std::floor(C.Subs / 100.0) * 100.0)));
			E.Fact = "Questions from chat";
			E.Src = "qa" + std::to_string(Today / 14);
		}
		break;
	case 903:
		if (Need(C.PartnerAt >= 0.0 && C.PartnerAt > World - 14.0 * ViDay ? 1 : 0, 1, "Join the Partner Program (in the last 14 days)"))
		{
			E.Set("Subs", Grouped(static_cast<int64_t>(std::floor(C.Subs))));
			double Hours = 0.0;
			for (const float H : C.HoursByDay)
			{
				Hours += H;
			}
			if (Hours >= 1.0)
			{
				E.Set("Hours", Grouped(static_cast<int64_t>(std::floor(Hours))));
			}
			E.Fact = "You're a Rerun Partner";
			E.Src = "partner";
		}
		break;
	case 904:
	{
		const double First = ViFirstUpload(C);
		if (Need(First >= 0.0 && First <= World - 365.0 * ViDay ? 1 : 0, 1, "A year since your first upload"))
		{
			double Views = 0.0;
			for (const Video& V : C.Videos)
			{
				Views += V.P;
			}
			E.Set("Videos", std::to_string(ViUploads(C)));
			E.Set("Views", Grouped(static_cast<int64_t>(std::llround(Views))));
			E.Set("Subs", Grouped(static_cast<int64_t>(std::floor(C.Subs))));
			E.Fact = "A year on Rerun";
			E.Src = "y1";
		}
		break;
	}
	case 906:
		if (Need(X.Comments30, 200, "200 comments on your videos in 30 days"))
		{
			E.Set("Comments", Grouped(X.Comments30));
			E.Fact = Grouped(X.Comments30) + " comments this month";
			E.Src = "cm" + std::to_string(Today / 30);
		}
		break;
	case 907:
	{
		const IdeaStream* S = nullptr;
		for (const IdeaStream& St : X.Streams)
		{
			S = !S && St.Predictions >= 3 ? &St : S;
		}
		if (Need(S ? 1 : 0, 1, "Stream a night with 3 resolved predictions"))
		{
			E.Set("Count", std::to_string(S->Predictions));
			E.Fact = "Chat called " + std::to_string(S->Predictions) + " predictions";
			E.Src = "pr" + std::to_string(static_cast<long long>(std::llround(S->Start)));
		}
		break;
	}
	case 908:
	{
		const IdeaStream* S = nullptr;
		for (const IdeaStream& St : X.Streams)
		{
			S = !S && St.Raids > 0 ? &St : S;
		}
		if (Need(S ? 1 : 0, 1, "Get raided on Kast"))
		{
			E.Fact = "A raid on your stream";
			E.Src = "raid" + std::to_string(static_cast<long long>(std::llround(S->Start)));
		}
		break;
	}
	// ---- K. life
	case 1101:
	{
		const bool Shift = X.LastShiftAt > World - ViDay;
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return Rr.At > World - ViDay; });
		if (Need(Shift && !R.empty() ? 1 : 0, 1, "Work a shift and play a session the same day"))
		{
			E.Set("Job", "night cashier");
			E.Set("Events", std::to_string(R.size()));
			E.Fact = "A shift and " + std::to_string(R.size()) + " tournaments today";
			E.Src = "d" + std::to_string(Today);
		}
		break;
	}
	case 1102:
		if (Need(X.NightShifts14, 3, "Work 3 night shifts in 14 days"))
		{
			E.Set("Shifts", std::to_string(X.NightShifts14));
			E.Set("Job", "night cashier");
			E.Fact = std::to_string(X.NightShifts14) + " night shifts in two weeks";
			E.Src = "ns" + std::to_string(Today / 14);
		}
		break;
	case 1103:
		if (Need(X.WashFold, 1, "Work a shift at the Wash & Fold"))
		{
			E.Fact = "A Wash & Fold shift";
			E.Src = "wf" + std::to_string(X.WashFold);
		}
		break;
	case 1104:
	{
		const bool Done = std::any_of(C.X.IdeaCool.begin(), C.X.IdeaCool.end(), [&](const Cool& Cl) { return Cl.Key == 1104 && Cl.Count >= X.Home; });
		if (Need(!X.Evicted && !Done ? 1 : 0, 1, X.Evicted ? std::string("Get your apartment back first") : std::string("Once for each place you live")))
		{
			E.Set("desk", ViDeskIds(X));
			E.Fact = "Your apartment";
			E.Src = "home" + std::to_string(X.Home);
		}
		break;
	}
	case 1105:
	case 1106:
	{
		Chips Cost = 0;
		for (const auto& G : X.Gear)
		{
			Cost += G.second;
		}
		const bool Upgrade = D.Id == 1106;
		const bool Ok = Upgrade ? X.GearBought7 >= 20000 : X.Gear.size() >= 3;
		if (Need(Ok ? 1 : 0, 1, Upgrade ? "Buy $200 or more of gear in 7 days" : "Own 3 pieces of gear"))
		{
			E.Set("Items", std::to_string(X.Gear.size()));
			E.Set("GearCost", Cash(Upgrade ? X.GearBought7 : Cost));
			E.Set("Gear", X.NewestGear.empty() && !X.Gear.empty() ? X.Gear.front().first : X.NewestGear);
			E.Set("desk", ViDeskIds(X));
			int Monitors = 0;
			for (const auto& G : X.Gear)
			{
				Monitors += LowerU3(G.first).find("monitor") != std::string::npos ? 1 : 0;
			}
			if (!Upgrade && Monitors > 0)
			{
				E.Set("Monitors", std::to_string(Monitors + 1) + " monitors");
			}
			E.Fact = Upgrade ? "New gear: " + X.NewestGear : std::to_string(X.Gear.size()) + " pieces of gear";
			E.Src = std::string(Upgrade ? "up" : "tour") + std::to_string(static_cast<long long>(Cost / 10000));
		}
		break;
	}
	case 1107:
	{
		const bool Back = std::any_of(X.Stories.begin(), X.Stories.end(), [](const std::string& K) { return K.rfind("back:", 0) == 0; });
		if (Need(Back && !X.Evicted ? 1 : 0, 1, "Get your apartment back after an eviction"))
		{
			E.Fact = "Home again";
			E.Src = "back" + std::to_string(X.Evictions);
		}
		break;
	}
	case 1108:
		if (Need(X.Evicted ? 1 : 0, 1, "(Only if the locks change: let's hope not)"))
		{
			E.Fact = "Evicted";
			E.Src = "ev" + std::to_string(X.Evictions);
		}
		break;
	case 1110:
		if (Need(X.Webcam > X.WebcamWas ? 1 : 0, 1, "Get a better webcam"))
		{
			E.Set("Gear", X.NewestGear);
			E.Set("desk", ViDeskIds(X));
			E.Fact = "A new camera";
			E.Src = "cam" + std::to_string(X.Webcam);
		}
		break;
	// ---- L. the rival saga
	case 1201:
	case 1202:
	case 1203:
	{
		int Pots = 0;
		const Candidate* Won = nullptr;
		for (const Candidate& M : C.Open)
		{
			if (M.Rival && M.Opponent == RivalName)
			{
				++Pots;
				Won = M.Rec.HeroNet > 0 && video_detail::PotBbOf(M) >= 30.0 && (!Won || M.At > Won->At) ? &M : Won;
			}
		}
		Pots += ViKeptCount(C, [](const Video& V) { return V.K == Kind::Rival; });
		if (D.Id == 1203)
		{
			if (Need(Won ? 1 : 0, 1, "Win a pot of 30 big blinds or more against gh0stfold"))
			{
				E.Src = Won->Id;
				E.At = Won->At;
				E.Set("Bb", Grouped(static_cast<int64_t>(std::llround(video_detail::PotBbOf(*Won)))));
				E.Set("MineWords", video_copy::HandWords(Won->Rec.Mine));
			}
		}
		else if (Need(Pots, D.Id == 1201 ? 3 : 5, D.Id == 1201 ? "Play 3 pots against gh0stfold" : "Play 5 pots against gh0stfold"))
		{
			E.Set("Count", std::to_string(Pots));
			E.Src = "g" + std::to_string(Pots);
		}
		if (E.Ready)
		{
			E.Set("Opp", RivalName);
			E.Opponent = RivalName;
			E.Rival = true;
			E.Fact = std::to_string(Pots) + " pots against " + RivalName;
		}
		break;
	}
	// ---- M. sites
	case 1301:
	{
		const std::pair<int, double>* First = nullptr;
		for (const auto& S : X.FirstSession)
		{
			First = S.first != static_cast<int>(net::SiteId::RiverLine) && S.second > World - 7.0 * ViDay && (!First || S.second > First->second) ? &S : First;
		}
		if (Need(First ? 1 : 0, 1, "Play your first session on Ignite or RunItTwice"))
		{
			const ViSum Sm = ViSumOf(ViWhere(X, [&](const IdeaResult& R) { return R.Site == First->first && R.At >= First->second && R.At < First->second + ViDay; }));
			E.Site = First->first;
			E.Set("Site", ViSiteName(First->first));
			E.Set("Events", std::to_string(std::max(1, Sm.Events)));
			E.Set("NetSigned", ViSigned(Sm.Net()));
			E.Strike = true;
			E.Fact = "Your first night on " + ViSiteName(First->first);
			E.Src = "first" + std::to_string(First->first);
		}
		break;
	}
	case 1302:
	{
		std::vector<int> Rivals;
		for (const int S : X.Joined)
		{
			if (S != static_cast<int>(net::SiteId::RiverLine))
			{
				Rivals.push_back(S);
			}
		}
		if (Need(Rivals.empty() ? 1 : 2, 2, "Join a second site"))
		{
			E.Site = Rivals.front();
			E.Set("Site", ViSiteName(static_cast<int>(net::SiteId::RiverLine)));
			E.Set("Site2", ViSiteName(Rivals.front()));
			E.Strike = true;
			E.Fact = "RiverLine and " + ViSiteName(Rivals.front());
			E.Src = "vs" + std::to_string(Rivals.front());
		}
		break;
	}
	case 1303:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return (Rr.Flags & IfFlipRun) != 0 && Rr.At > World - 7.0 * ViDay; });
		if (Need(static_cast<int>(R.size()), 10, "Play 10 Flip & Runs in a week"))
		{
			const ViSum S = ViSumOf(R);
			E.Site = static_cast<int>(net::SiteId::Ignite);
			E.Set("Events", std::to_string(S.Events));
			E.Set("NetSigned", ViSigned(S.Net()));
			E.Strike = true;
			E.Fact = std::to_string(S.Events) + " Flip & Runs this week";
			E.Src = "fr" + std::to_string(Today / 7);
		}
		break;
	}
	case 1304:
	case 1305:
	{
		const bool Ok = D.Id == 1304 ? X.RewardsTier > 0 : X.WelcomePending;
		const int Site = D.Id == 1304 ? static_cast<int>(net::SiteId::RunItTwice) : static_cast<int>(net::SiteId::Ignite);
		if (Need(Ok ? 1 : 0, 1, D.Id == 1304 ? "Reach a rewards tier on a rival site" : "Have Ignite's welcome bonus clearing"))
		{
			E.Site = Site;
			E.Set("Site", ViSiteName(Site));
			E.Set("Tier", D.Id == 1304 ? std::string("Second Run") : std::string());
			E.Strike = true;
			E.Fact = D.Id == 1304 ? "What " + ViSiteName(Site) + " gives back" : ViSiteName(Site) + "'s welcome bonus";
			E.Src = "ltd" + std::to_string(D.Id);
		}
		break;
	}
	case 1306:
	{
		std::string Blaze, Hundred;
		for (const IdeaResult& R : X.Results)
		{
			Blaze = Blaze.empty() && R.Name.find("Sunday Blaze") != std::string::npos ? R.Name : Blaze;
			Hundred = Hundred.empty() && R.Name.find("Sunday 100K") != std::string::npos ? R.Name : Hundred;
		}
		if (Need((Blaze.empty() ? 0 : 1) + (Hundred.empty() ? 0 : 1), 2, "Play the Sunday Blaze and the Sunday 100K"))
		{
			E.Set("Event", Blaze);
			E.Set("Event2", Hundred);
			E.Strike = true;
			E.Fact = Blaze + " and " + Hundred;
			E.Src = "majors";
		}
		break;
	}
	case 1308:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [&](const IdeaResult& Rr) { return (Rr.Flags & IfSpark) != 0 && Rr.At > World - 7.0 * ViDay; });
		if (Need(static_cast<int>(R.size()), 1, "Play The Spark"))
		{
			E.FromResult(*R.front());
			E.If = R.front()->Place == 1 ? "X" : "";
			E.Src = R.front()->Id;
			E.Fact = "Your night in " + R.front()->Name;
		}
		break;
	}
	// ---- N. the rival sites' moments (site::play's facts)
	case 1401:
		if (Need(X.FuseBest >= 25 ? 1 : 0, 1, "Light a FUSE at x25 or more"))
		{
			E.Site = static_cast<int>(net::SiteId::Ignite);
			E.Set("Mult", std::to_string(X.FuseBest));
			if (X.FuseBestBuyIn > 0)
			{
				E.Set("BuyIn", Cash(X.FuseBestBuyIn));
			}
			if (X.FuseBestPrize > 0)
			{
				E.Set("Prize", Cash(X.FuseBestPrize));
			}
			E.Strike = true;
			E.Fact = "A FUSE at x" + std::to_string(X.FuseBest);
			E.Src = "fuse" + std::to_string(X.FuseBest);
		}
		break;
	case 1402:
	case 1403:
	case 1404:
	case 1405:
	case 1406:
	case 1407:
	case 1408:
	case 1409:
	case 1410:
	case 1411:
	{
		bool Ok = false;
		std::string Hint;
		switch (D.Id)
		{
		case 1402:
		{
			Ok = X.BeefPaid && !X.Beef.empty();
			Hint = "Knock out your Beef on Ignite";
			E.Set("Beef", X.Beef);
			static const char* const Times[] = {"", "once", "twice", "three times", "four times", "five times", "six times", "seven times", "eight times", "nine times", "ten times"};
			if (X.BeefKo >= 1 && X.BeefKo <= 10)
			{
				E.Set("BeefTimes", Times[X.BeefKo]);
			}
			break;
		}
		case 1403: Ok = !X.Headliner.empty(); Hint = "Knock out a Headliner on Ignite"; E.Set("Headliner", X.Headliner); break;
		case 1404: Ok = X.Flame >= 30; Hint = "Keep Ignite's Flame for 30 days"; E.Set("Days", std::to_string(X.Flame)); break;
		case 1405: Ok = X.SpecialGuest; Hint = "Get your name on Ignite's Bandstand"; break;
		case 1406:
			Ok = !X.Division.empty() && !X.DivisionWas.empty();
			Hint = "Win a promotion in the RunItTwice League";
			E.Set("Division", X.Division);
			E.Set("DivisionWas", X.DivisionWas);
			break;
		case 1407: Ok = X.DeadlineDay; Hint = "A League month that comes down to its last day"; break;
		case 1408: Ok = !X.CalledOut.empty(); Hint = "Win a Callout on RunItTwice"; E.Set("Callout", X.CalledOut); break;
		case 1409: Ok = X.TeamScalps; Hint = "Knock out all six of Team RunItTwice"; break;
		case 1410: Ok = X.ClubTier >= 2; Hint = "A High Table or Inner Circle month at the Club"; E.Set("Tier", X.ClubTier >= 3 ? "Inner Circle" : "High Table"); break;
		default: Ok = !X.TeamSigned.empty(); Hint = "Sign with a site's team"; E.Set("Team", X.TeamSigned); break;
		}
		if (Need(Ok ? 1 : 0, 1, Hint))
		{
			E.Site = D.Id <= 1405 ? static_cast<int>(net::SiteId::Ignite) : static_cast<int>(net::SiteId::RunItTwice);
			E.Strike = true;
			E.Fact = D.Name;
			E.Src = std::string("n") + std::to_string(D.Id);
		}
		else
		{
			E.Facts.clear();
		}
		break;
	}
	// ---- O. Cut-native
	case 1501:
		MomentIdea("A hand that saw a river.", [](const Candidate& M) { return M.Rec.Board.size() >= 5 && !M.Storytime; });
		break;
	case 1502: MomentIdea("A big fold in MOMENTS.", [](const Candidate& M) { return M.K == Kind::HeroFold || M.K == Kind::LadderFold; }); break;
	case 1503: MomentIdea("A hand you played to a showdown.", [](const Candidate& M) { return !M.Rec.Shown.empty() && !M.Storytime; }); break;
	case 1504: MomentIdea("A moment caught on stream (your face on Kast).", [](const Candidate& M) { return M.Stream.Viewers > 0; }); break;
	case 1505: MomentIdea("A moment on stream with chat talking.", [](const Candidate& M) { return !M.Stream.Chat.empty(); }); break;
	case 1506: MomentIdea("Bust a tournament in its first level.", [](const Candidate& M) { return M.K == Kind::LastHand && M.Rec.Level <= 1; }); break;
	case 1507: MomentIdea("A river at the Embercrest.", [](const Candidate& M) { return M.Source == 1 && M.Rec.Board.size() >= 5; }); break;
	case 1508: MomentIdea("Make a monster hand.", [](const Candidate& M) { return M.K == Kind::Monster; }); break;
	// ---- P. series: always declarable (an episode needs the series' own facts, below)
	default:
	{
		bool Ok = true;
		std::string Hint;
		switch (D.Id)
		{
		case 1602: Ok = net::Weekday(Today) != 6; Hint = "Declare it before a Sunday"; break;
		case 1603: Ok = !ViWhere(X, [](const IdeaResult& R) { return (R.Flags & IfSeatWon) != 0; }).empty(); Hint = "Win a satellite seat (a first step)"; break;
		case 1605: Ok = !ViWhere(X, [](const IdeaResult& R) { return (R.Flags & IfOmaha) != 0; }).empty(); Hint = "Play an Omaha event"; break;
		case 1606: Ok = X.Joined.size() >= 3; Hint = "Have accounts on 3 sites"; break;
		case 1607: Ok = ViJoined(X, static_cast<int>(net::SiteId::RunItTwice)); Hint = "Join RunItTwice (its League)"; break;
		case 1610: Ok = X.SeasonRank > 0; Hint = "Get a rank on the Season board"; break;
		default: break;
		}
		if (Need(Ok ? 1 : 0, 1, Hint.empty() ? std::string("Declare it") : Hint))
		{
			E.Fact = D.Name;
			E.Series = true;
			E.Src = "series";
		}
		break;
	}
	}
}

/** The series record behind a group P idea (its challenge and its playlist), or null. */
const Challenge* ViSeries(const Channel& C, int Idea)
{
	for (const Challenge& S : C.X.Challenges)
	{
		if (S.Kind == Idea && S.State == 0)
		{
			return &S;
		}
	}
	return nullptr;
}

std::string ViSeriesName(const Channel& C, const Challenge& S)
{
	for (const Playlist& P : C.X.Lists)
	{
		if (P.Id == "s" + std::to_string(S.Id))
		{
			return P.Name;
		}
	}
	return std::string();
}

/** A series' tracked roll: the start plus every result on its site since it began (P1 only; others report their count). */
Chips ViSeriesRoll(const Challenge& S, const IdeaCtx& X)
{
	Chips Roll = S.Start;
	for (const IdeaResult& R : X.Results)
	{
		if (R.At >= S.StartedAt && R.Site == S.Site && (R.Flags & IfLive) == 0)
		{
			Roll += R.Prize - R.BuyIn;
		}
	}
	return Roll;
}

/** The facts of a series' next episode (group P), into E. */
void ViEpisode(const IdeaDef& D, const IdeaCtx& X, const Channel& C, double World, ViEval& E)
{
	const Challenge* S = ViSeries(C, D.Id);
	const int Ep = S ? S->Episodes + 1 : 1;
	const int Day = S ? std::max(1, static_cast<int>(std::floor((World - S->StartedAt) / ViDay)) + 1) : 1;
	E.Set("Ep", std::to_string(Ep));
	E.Set("Day", std::to_string(Day));
	std::string Name = S ? ViSeriesName(C, *S) : std::string();
	const int Site = D.Id == 1601 ? (S ? S->Site : (X.Joined.empty() ? 0 : X.Joined.back())) : 0;
	switch (D.Id)
	{
	case 1601:
	{
		const Chips Start = S ? S->Start : std::max<Chips>(5000, std::min<Chips>(X.Bankroll / 4, 50000) / 1000 * 1000);
		const Chips Goal = S ? S->Goal : Start * 10;
		const Chips Now = S ? ViSeriesRoll(*S, X) : Start;
		E.Set("Start", Cash(Start));
		E.Set("Goal", Cash(Goal));
		E.Set("Now", Cash(Now));
		E.Set("Site", ViSiteName(Site));
		E.Site = Site;
		E.Strike = Site != static_cast<int>(net::SiteId::RiverLine);
		Name = Name.empty() ? Cash(Start) + " to " + Cash(Goal) + " on " + ViSiteName(Site) : Name;
		E.If = Now >= Goal ? "X" : "";
		break;
	}
	case 1604:
	case 1608:
	{
		int Count = 0;
		for (const IdeaResult& R : X.Results)
		{
			Count += S && R.At >= S->StartedAt && (D.Id == 1608 || R.Name.find("Night Owl") != std::string::npos) ? 1 : 0;
		}
		E.Set("Goal", "100");
		E.Set("Count", std::to_string(Count));
		E.Set("Left", std::to_string(std::max(0, 100 - Count)));
		if (D.Id == 1608)
		{
			E.Set("Days", "7");
		}
		Name = Name.empty() ? (D.Id == 1604 ? std::string("100 Night Owls") : std::string("100 tournaments in 7 days")) : Name;
		break;
	}
	case 1607:
		E.Set("Division", X.Division.empty() ? std::string("Steel") : X.Division);
		break;
	case 1610:
		if (X.SeasonRank > 0)
		{
			E.Set("Rank", Ordinal(X.SeasonRank));
		}
		break;
	case 1603:
	{
		const std::vector<const IdeaResult*> R = ViWhere(X, [](const IdeaResult& Rr) { return (Rr.Flags & IfSeatWon) != 0; });
		if (!R.empty())
		{
			E.Set("Event", R.front()->Name);
		}
		break;
	}
	default: break;
	}
	E.Set("Series", Name.empty() ? std::string(D.Name) : Name);
	E.Series = true;
}
} // namespace

// ================================================================== the public face

namespace
{
std::string ViCoreFacts(const IdeaDef& D, const ViEval& E)
{
	// What core-videoalgo reads (VideoAlgoPriv.h): the Topic, the cooldown, the advertiser category, Limited, a trend, the
	// formats a single idea may go up as, a Strategy lesson.
	static const char* const AdKeys[] = {"gear", "teach", "results", "story", "life", "loud", "news", "rival"};
	// Road to the Final Table's Topic reads the real peak room (SPEC_CREATORS §3.7: x(1 + min(0.5, V/200)), inside the band).
	const double Topic = D.Id == 312 && E.Viewers > 0 ? std::min(2.4, D.Topic * (1.0 + std::min(0.5, static_cast<double>(E.Viewers) / 200.0))) : D.Topic;
	std::string Out = "w=" + Fixed(Topic, 2) + ";cool=" + std::to_string(D.Cool);
	if (D.Ad >= 0 && D.Ad < 8)
	{
		Out += std::string(";ad=") + AdKeys[D.Ad];
	}
	if (D.Limited)
	{
		Out += ";ltd=1";
	}
	if (!E.Trend.empty())
	{
		Out += ";trend=" + E.Trend;
	}
	std::string Fmt;
	for (int F = 0; F < static_cast<int>(Format::Count); ++F)
	{
		Fmt += ((D.Formats >> F) & 1) ? std::to_string(F) : std::string();
	}
	Out += ";fmt=" + Fmt;
	if (D.Strategy)
	{
		Out += ";cls=S";
	}
	return Out;
}

/** The idea's board card and its candidate's facts. */
IdeaCard ViCard(const IdeaDef& D, const IdeaCtx& X, const Channel& C, double World, ViEval& E)
{
	ViEvalIdea(D, X, C, World, E);
	if (E.Ready && D.Id / 100 == 16)
	{
		ViEpisode(D, X, C, World, E);
	}
	IdeaCard Card;
	Card.Id = D.Id;
	Card.Name = D.Name;
	Card.Group = static_cast<uint8_t>(D.Id / 100);
	for (int F = 0; F < static_cast<int>(Format::Count); ++F)
	{
		if ((D.Formats >> F) & 1)
		{
			Card.Formats.push_back(static_cast<Format>(F));
		}
	}
	if (E.Format >= 0)
	{
		auto It = std::find(Card.Formats.begin(), Card.Formats.end(), static_cast<Format>(E.Format));
		if (It != Card.Formats.end())
		{
			std::rotate(Card.Formats.begin(), It, It + 1);
		}
	}
	const Format First = Card.Formats.empty() ? Format::Breakdown : Card.Formats.front();
	Card.EditMin = static_cast<int>(std::lround(EditMinutes(First)));
	Card.Energy = static_cast<int>(std::lround(EditEnergy(First)));
	Card.Ready = E.Ready;
	Card.Fact = E.Ready ? E.Fact : std::string();
	Card.Hint = E.Ready ? std::string() : (E.Hint.empty() ? std::string("Not yet.") : E.Hint);
	if (!Card.Hint.empty() && Card.Hint.back() != '.' && Card.Hint.back() != ')' && Card.Hint.back() != '?')
	{
		Card.Hint += ".";
	}
	Card.Have = E.Have;
	Card.Need = E.Need;
	Card.Trending = E.Trending;
	Card.Limited = D.Limited;
	Card.Strike = E.Strike || (D.Flag && E.Site != static_cast<int>(net::SiteId::RiverLine) && E.Site != 0);
	// The soft cooldown: a repeat inside it reaches half as far (core-videoalgo's IdeaCool); the card says how long is left.
	const int Today = ViDayOf(World);
	for (const Cool& Cl : C.X.IdeaCool)
	{
		if (Cl.Key == D.Id && Cl.LastDay >= 0 && D.Cool > 0)
		{
			Card.CooldownDays = std::max(0, D.Cool - (Today - Cl.LastDay));
		}
	}
	return Card;
}
} // namespace
} // namespace vcontent

std::vector<IdeaCard> Ideas(const IdeaCtx& X, const Channel& C, double World)
{
	using namespace vcontent;
	std::vector<IdeaCard> Ready, Trend, Locked;
	for (const IdeaDef& D : IdeaTable())
	{
		ViEval E;
		IdeaCard Card = ViCard(D, X, C, World, E);
		(Card.Ready ? (Card.Trending ? Trend : Ready) : Locked).push_back(Card);
	}
	// Trend matches on top, then ready ones, locked last (each in the table's order).
	std::vector<IdeaCard> Out = Trend;
	Out.insert(Out.end(), Ready.begin(), Ready.end());
	Out.insert(Out.end(), Locked.begin(), Locked.end());
	return Out;
}

bool MakeIdeaCandidate(const IdeaCtx& X, int IdeaId, double World, Candidate& Out)
{
	// The channel is the context's (IdeaCtx::Chan; an empty one when unset). Group A's ideas are their moments: the Session
	// tags the moment itself (SessionVideo3.cpp's ChooseIdea), so this makes candidates for the other groups only.
	static const Channel None;
	std::string Why;
	return vcontent::MakeIdeaCandidateOn(X, X.Chan ? *X.Chan : None, IdeaId, World, Out, Why);
}

namespace vcontent
{
/** The idea's candidate on a channel (MakeIdeaCandidate with the channel): false when the idea isn't ready. */
bool MakeIdeaCandidateOn(const IdeaCtx& X, const Channel& C, int IdeaId, double World, Candidate& Out, std::string& Why)
{
	const IdeaDef* D = IdeaById(IdeaId);
	if (!D || D->Id / 100 == 1)
	{
		Why = D ? "That one's a moment: pick it in MOMENTS." : "That idea isn't on the board.";
		return false;
	}
	ViEval E;
	const IdeaCard Card = ViCard(*D, X, C, World, E);
	if (!Card.Ready)
	{
		Why = Card.Hint;
		return false;
	}
	std::string Facts = ViCoreFacts(*D, E);
	for (const auto& Kv : E.Facts)
	{
		Facts += ";" + Kv.first + "=" + FactEsc(Kv.second);
	}
	if (!E.If.empty())
	{
		Facts += ";If=" + E.If;
	}
	Candidate N;
	N.Id = "idea:" + LowerU3(D->Code) + ":" + (E.Src.empty() ? std::to_string(static_cast<int>(std::floor(World / ViDay))) : E.Src);
	N.At = World;
	N.Source = 5;
	N.Site = E.Site;
	N.K = Kind::Idea;
	N.Idea = D->Id;
	N.IdeaFacts = Facts;
	N.Event = E.Event;
	N.BuyIn = E.BuyIn;
	N.Field = E.Field;
	N.PlaceSoFar = E.Place;
	N.Rec.Pot = E.Prize;
	N.Rec.HeroNet = E.Prize - E.BuyIn;
	N.Opponent = E.Opponent;
	N.Rival = E.Rival;
	N.Series = E.Series;
	N.Storytime = E.Storytime;
	N.Stream.Viewers = E.Viewers;
	// An idea cut from a moment (a fold, a river, a back-room hand, a stream's face): the moment's record and the stream
	// around it come along, so its title and thumbnail tell that hand (and only what the player saw of it).
	for (const Candidate& M : C.Open)
	{
		if (M.Id == E.Src && M.Source != 5)
		{
			N.Rec = M.Rec;
			N.EquityAtAllIn = M.EquityAtAllIn;
			N.Opponent = M.Storytime ? std::string() : M.Opponent;
			N.Rival = M.Rival && !M.Storytime;
			N.Event = N.Event.empty() ? M.Event : N.Event;
			N.BuyIn = N.BuyIn > 0 ? N.BuyIn : M.BuyIn;
			N.Field = N.Field > 0 ? N.Field : M.Field;
			N.Site = M.Site;
			N.Storytime = M.Storytime;
			N.Bubble = M.Bubble;
			N.Stream = M.Stream;
		}
	}
	// Drama (Studio's sort and the payoff's read): the idea's Topic as a hand's, a stream's room on top (Video.cpp's DramaOf).
	N.Drama = std::clamp(0.35 * D->Topic + (E.Viewers > 0 ? 0.15 : 0.0) + (E.Series ? 0.10 : 0.0), 0.0, 1.0);
	N.Drama = std::round(N.Drama * 10000.0) / 10000.0;
	N.Expires = World + (D->Id / 100 == 8 ? 7.0 : 14.0) * ViDay;
	Out = N;
	return true;
}

/** The idea facts a group A moment is tagged with (the moment's own record carries the rest). */
std::string MomentIdeaFacts(int IdeaId)
{
	const IdeaDef* D = IdeaById(IdeaId);
	if (!D)
	{
		return std::string();
	}
	ViEval E;
	return ViCoreFacts(*D, E);
}

/** The group A idea's moment on a channel (its id), "" when none. */
std::string IdeaMoment(const IdeaCtx& X, const Channel& C, int IdeaId, double World)
{
	const IdeaDef* D = IdeaById(IdeaId);
	if (!D || D->Id / 100 != 1)
	{
		return std::string();
	}
	ViEval E;
	ViEvalIdea(*D, X, C, World, E);
	return E.Ready ? E.Src : std::string();
}

/** Declares (or continues) a group P series on the channel: its challenge and its playlist (the SERIES tab). */
int DeclareSeries(Channel& C, const IdeaCtx& X, int IdeaId, double World)
{
	const IdeaDef* D = IdeaById(IdeaId);
	if (!D || D->Id / 100 != 16)
	{
		return -1;
	}
	if (const Challenge* S = ViSeries(C, IdeaId))
	{
		return S->Id;
	}
	ViEval E;
	ViEpisode(*D, X, C, World, E);
	Challenge S;
	for (const Challenge& Old : C.X.Challenges)
	{
		S.Id = std::max(S.Id, Old.Id);
	}
	++S.Id;
	S.Kind = IdeaId;
	S.Site = E.Site;
	for (const auto& Kv : E.Facts)
	{
		if (Kv.first == "Start")
		{
			S.Start = std::max<Chips>(0, X.Bankroll > 0 ? std::min<Chips>(std::max<Chips>(5000, std::min<Chips>(X.Bankroll / 4, 50000) / 1000 * 1000), X.Bankroll) : 5000);
		}
	}
	S.Goal = IdeaId == 1601 ? S.Start * 10 : IdeaId == 1604 || IdeaId == 1608 ? 100 : 0;
	S.StartedAt = World;
	S.State = 0;
	S.Episodes = 0;
	// At most 4 records: a finished one makes room first, else the oldest.
	if (C.X.Challenges.size() >= 4)
	{
		auto Drop = std::find_if(C.X.Challenges.begin(), C.X.Challenges.end(), [](const Challenge& Ch) { return Ch.State != 0; });
		Drop = Drop == C.X.Challenges.end() ? C.X.Challenges.begin() : Drop;
		const std::string ListId = "s" + std::to_string(Drop->Id);
		C.X.Lists.erase(std::remove_if(C.X.Lists.begin(), C.X.Lists.end(), [&](const Playlist& P) { return P.Id == ListId; }), C.X.Lists.end());
		C.X.Challenges.erase(Drop);
	}
	C.X.Challenges.push_back(S);
	Playlist P;
	P.Id = "s" + std::to_string(S.Id);
	std::string Name = D->Name;
	for (const auto& Kv : E.Facts)
	{
		Name = Kv.first == "Series" ? Kv.second : Name;
	}
	if (IdeaId == 1601)
	{
		Name = Cash(S.Start) + " to " + Cash(S.Goal) + " on " + ViSiteName(S.Site);
	}
	P.Name = Name;
	if (C.X.Lists.size() >= 8)
	{
		C.X.Lists.erase(C.X.Lists.begin());
	}
	C.X.Lists.push_back(P);
	return S.Id;
}

/** After an episode goes up: the series counts it and keeps its video; a P1 that reached its goal (or busted) ends. */
void NoteEpisode(Channel& C, const IdeaCtx& X, int SeriesId, const std::string& VideoId)
{
	for (Challenge& S : C.X.Challenges)
	{
		if (S.Id != SeriesId)
		{
			continue;
		}
		++S.Episodes;
		if (S.Kind == 1601)
		{
			const Chips Roll = ViSeriesRoll(S, X);
			S.State = Roll >= S.Goal ? 1 : Roll <= 0 ? 2 : 0;
		}
		else if (S.Kind == 1605 && S.Episodes >= 10)
		{
			S.State = 1;
		}
		for (Playlist& P : C.X.Lists)
		{
			if (P.Id == "s" + std::to_string(S.Id) && std::find(P.Videos.begin(), P.Videos.end(), VideoId) == P.Videos.end())
			{
				P.Videos.push_back(VideoId);
			}
		}
	}
}
} // namespace vcontent

std::vector<SeriesRow> SeriesOf(const Channel& C, const IdeaCtx& X, double World)
{
	using namespace vcontent;
	std::vector<SeriesRow> Out;
	for (const Challenge& S : C.X.Challenges)
	{
		SeriesRow R;
		R.Id = S.Id;
		R.Idea = S.Kind;
		R.Site = S.Site;
		R.State = S.State;
		R.Name = ViSeriesName(C, S);
		R.Episodes = S.Episodes;
		R.StartedAt = S.StartedAt;
		R.StartCents = S.Start;
		R.GoalCents = S.Goal;
		R.NowCents = S.Kind == 1601 ? ViSeriesRoll(S, X) : 0;
		for (const Playlist& P : C.X.Lists)
		{
			if (P.Id == "s" + std::to_string(S.Id))
			{
				R.Videos = P.Videos;
			}
		}
		for (const std::string& Id : R.Videos)
		{
			for (const Video& V : C.Videos)
			{
				R.Views += V.Id == Id ? Views(V, World) : 0.0;
			}
		}
		Out.push_back(R);
	}
	return Out;
}

int IdeaAdCategory(int IdeaId)
{
	const vcontent::IdeaDef* D = vcontent::IdeaById(IdeaId);
	return D ? D->Ad : -1;
}

std::string IdeaName(int IdeaId)
{
	const vcontent::IdeaDef* D = vcontent::IdeaById(IdeaId);
	return D ? std::string(D->Name) : std::string();
}

// ================================================================== the kinds a record shows by itself (Classify's)

std::vector<Kind> RecordKinds(const recall::Record& Rec, bool Showdown)
{
	std::vector<Kind> Out;
	if (Rec.Mine.size() < 2 || Rec.Board.size() < 3)
	{
		return Out;
	}
	bool HeroFolded = false;
	for (const recall::Line& L : Rec.Lines)
	{
		HeroFolded = HeroFolded || (L.Who == "You" && L.What == "folds");
	}
	if (HeroFolded)
	{
		return Out;
	}
	// Monster: quads or better of the player's own, in a pot they won or at a showdown.
	const int Cat = video_detail::OwnCategory(Rec.Mine, Rec.Board);
	if (Cat >= static_cast<int>(Category::Quads) && (Rec.HeroNet > 0 || Showdown))
	{
		Out.push_back(Kind::Monster);
	}
	if (!Showdown)
	{
		return Out;
	}
	// Set over set: both players made a set from a pocket pair on this board.
	const auto SetOf = [&Rec](const std::vector<Card>& H) {
		if (H.size() < 2 || RankOf(H[0]) != RankOf(H[1]))
		{
			return false;
		}
		int OnBoard = 0;
		for (const Card C : Rec.Board)
		{
			OnBoard += RankOf(C) == RankOf(H[0]) ? 1 : 0;
		}
		return OnBoard == 1;
	};
	for (const auto& S : Rec.Shown)
	{
		if (SetOf(Rec.Mine) && SetOf(S.second) && std::fabs(static_cast<double>(Rec.HeroNet)) * 4.0 >= static_cast<double>(Rec.Pot))
		{
			Out.push_back(Kind::SetOverSet);
			break;
		}
	}
	// Runner-runner: under 5% after the flop and won, or over 95% and lost, against the hand turned up.
	if (Rec.Board.size() >= 5)
	{
		for (const auto& S : Rec.Shown)
		{
			if (S.second.size() < 2)
			{
				continue;
			}
			bool Used[52] = {};
			for (const Card C : Rec.Mine)
			{
				Used[C] = true;
			}
			for (const Card C : S.second)
			{
				Used[C] = true;
			}
			for (size_t I = 0; I < 3; ++I)
			{
				Used[Rec.Board[I]] = true;
			}
			double Win = 0.0, N = 0.0;
			std::vector<Card> Full(Rec.Board.begin(), Rec.Board.begin() + 3);
			Full.resize(5);
			for (int A = 0; A < 52; ++A)
			{
				for (int B = A + 1; !Used[A] && B < 52; ++B)
				{
					if (Used[B])
					{
						continue;
					}
					Full[3] = static_cast<Card>(A);
					Full[4] = static_cast<Card>(B);
					const int Me = EvaluateHand(Rec.Mine, Full), Them = EvaluateHand(S.second, Full);
					Win += Me > Them ? 1.0 : Me == Them ? 0.5 : 0.0;
					N += 1.0;
				}
			}
			const double Eq = N > 0.0 ? Win / N : 0.5;
			if ((Eq < 0.05 && Rec.HeroNet > 0) || (Eq > 0.95 && Rec.HeroNet < 0))
			{
				Out.push_back(Kind::RunnerRunner);
			}
			break;
		}
	}
	return Out;
}

// ================================================================== Make a Cut from a published video (§4.6.9)

namespace vcontent
{
Kind CutKindOf(const Video& Long)
{
	// A hand video's Cut is that hand's kind. An idea video (or a run's) keeps no moment's kind, only its idea and its cards:
	// the Cut says what those show and never more (C3 gate L8: a fold told as an idea went up as a "Big pot" Cut).
	if (Long.K != Kind::Idea && !video_detail::EventKind(Long.K))
	{
		return Long.K;
	}
	std::vector<Card> Mine, Theirs, Board;
	const std::string& Cards = Long.Th.Cards;
	const size_t A1 = Cards.find('/'), A2 = A1 == std::string::npos ? std::string::npos : Cards.find('/', A1 + 1);
	if (A1 != std::string::npos && A2 != std::string::npos)
	{
		Mine = ParseCards(Cards.substr(0, A1));
		Theirs = ParseCards(Cards.substr(A1 + 1, A2 - A1 - 1));
		Board = ParseCards(Cards.substr(A2 + 1));
	}
	const int Idea = Long.K == Kind::Idea ? Long.X.Idea : -1;
	if (const IdeaDef* D = Idea / 100 == 1 ? IdeaById(Idea) : nullptr)
	{
		return D->HandKind; // a group A idea is its moment's kind
	}
	switch (Idea)
	{
	case 612:  // F12, folding aces or kings: a fold of aces or kings (its moment is a hero fold or a ladder fold)
	case 1502: // O2, rate this fold: a big fold
		return Kind::HeroFold;
	case 603: // F3, bubble ICM: a bubble all in shows the other hand; a big fold shows none
		return Theirs.size() >= 2 ? Kind::Bubble : Kind::HeroFold;
	case 1506: return Kind::LastHand; // O6, out in level one: the hand that busted
	case 1508: return Kind::Monster;  // O8, the reveal: a monster hand
	default: break;
	}
	// The rest (a river, a showdown, a stream's face, a back-room hand, a run's hand): quads or better made is a monster; any
	// other is a hand the Cut tells as one hand (Kind::Idea with no idea: a Cut's own titles, no pot and no all in promised).
	if (Mine.size() >= 2 && Board.size() >= 3 && video_detail::OwnCategory(Mine, Board) >= static_cast<int>(Category::Quads))
	{
		return Kind::Monster;
	}
	return Kind::Idea;
}
} // namespace vcontent

// ================================================================== Research's "What poker viewers search" (§5.8)

namespace vcontent
{
namespace
{
/**
 * A phrase poker viewers search: its key, its words (never a brand), how much it's searched in an ordinary week (0-100),
 * what it is (0 strategy, 2 a format, 3 a story), the format it's about (IdeaFlag), its season (the month it peaks, 0.0 = the
 * 1st of January, and how far it swings either way), the calendar trend that lifts it (a Trend key's prefix), the ideas that
 * answer it (the first ready one is suggested) and the hand kinds of the player's videos that are on it (-1 none).
 */
struct ViSearch
{
	const char* Key;
	const char* Phrase;
	int Base;
	int Source;
	uint32_t Format;
	double Peak, Swing;
	const char* Lift;
	int Ideas[3];
	int Kinds[2];
};

const std::vector<ViSearch>& ViSearches()
{
	constexpr int No = -1;
	const int Beat = static_cast<int>(Kind::BadBeat), Cool = static_cast<int>(Kind::Cooler), Bluff = static_cast<int>(Kind::Bluff);
	const int Call = static_cast<int>(Kind::HeroCall), Fold = static_cast<int>(Kind::HeroFold), Pot = static_cast<int>(Kind::BigPot);
	const int Bub = static_cast<int>(Kind::Bubble), Ft = static_cast<int>(Kind::FinalTable), Win = static_cast<int>(Kind::Win);
	const int Clip = static_cast<int>(Kind::StreamMoment), Monster = static_cast<int>(Kind::Monster), Sets = static_cast<int>(Kind::SetOverSet);
	const int Hu = static_cast<int>(Kind::HeadsUp), Env = static_cast<int>(Kind::Envelope), Biggest = static_cast<int>(Kind::BiggestPot);
	const int Ladder = static_cast<int>(Kind::LadderFold);
	static const std::vector<ViSearch> All = {
		// ---- strategy: what people type when they want to play better (a lesson's year of search tail)
		{"icm-bubble", "ICM at the bubble", 74, 0, 0, 0.0, 0.0, "", {603, 114, 120}, {Bub, Ladder}},
		{"ft-icm", "final table ICM", 52, 0, 0, 0.0, 0.0, "", {610, 302, 304}, {Ft, No}},
		{"push-fold", "push or fold chart", 60, 0, 0, 0.0, 0.0, "", {604, 609, No}, {No, No}},
		{"play-ak", "how to play AK", 68, 0, 0, 0.0, 0.0, "", {614, 713, No}, {No, No}},
		{"play-kings", "how to play pocket kings", 57, 0, 0, 0.0, 0.0, "", {612, 105, No}, {No, No}},
		{"fold-aces", "can you fold aces", 41, 0, 0, 0.0, 0.0, "", {612, 110, No}, {Fold, No}},
		{"how-bluff", "how to bluff", 70, 0, 0, 0.0, 0.0, "", {106, 107, No}, {Bluff, No}},
		{"river-call", "when to call a river bet", 44, 0, 0, 0.0, 0.0, "", {109, 608, No}, {Call, No}},
		{"starting-hands", "starting hands chart", 63, 0, 0, 0.3, 0.15, "", {614, 713, No}, {No, No}},
		{"beginners", "tournament poker for beginners", 66, 0, 0, 0.3, 0.20, "", {614, 404, No}, {No, No}},
		{"pot-odds", "pot odds explained", 50, 0, 0, 0.3, 0.10, "", {608, 614, No}, {No, No}},
		{"short-stack", "short stack strategy", 46, 0, 0, 0.0, 0.0, "", {609, 604, No}, {No, No}},
		{"bankroll", "bankroll management", 58, 0, 0, 0.3, 0.15, "", {508, 501, 509}, {No, No}},
		{"mtt", "MTT strategy", 48, 0, 0, 0.0, 0.0, "", {301, 304, 302}, {No, No}},
		{"heads-up", "heads-up strategy", 34, 0, 0, 0.0, 0.0, "", {122, No, No}, {Hu, No}},
		{"reads", "how to read opponents", 43, 0, 0, 0.0, 0.0, "", {601, 109, No}, {No, No}},
		{"tells", "live poker tells", 47, 0, 0, 6.5, 0.10, "", {404, 401, No}, {No, No}},
		{"live-online", "live vs online poker", 40, 0, 0, 0.0, 0.0, "", {404, No, No}, {No, No}},
		{"move-up", "when to move up stakes", 37, 0, 0, 0.0, 0.0, "", {509, 510, No}, {No, No}},
		{"tilt", "how to stop tilting", 39, 0, 0, 0.0, 0.0, "", {503, 602, No}, {No, No}},
		{"mistakes", "common tournament mistakes", 45, 0, 0, 0.0, 0.0, "", {602, 712, No}, {No, No}},
		{"quiz", "poker quiz", 30, 0, 0, 0.0, 0.0, "", {608, 1502, 1503}, {No, No}},
		{"rakeback", "rakeback explained", 27, 0, 0, 0.0, 0.0, "", {1304, No, No}, {No, No}},
		{"overlays", "overlay tournaments", 25, 0, 0, 0.0, 0.0, "", {1307, 310, No}, {No, No}},
		{"best-sites", "best sites for tournaments", 50, 0, 0, 0.0, 0.0, "", {710, 1302, 613}, {No, No}},
		{"coaching", "poker coaching", 33, 0, 0, 0.3, 0.10, "", {611, No, No}, {No, No}},
		{"deep-run", "how to make a deep run", 38, 0, 0, 0.0, 0.0, "", {301, 304, No}, {No, No}},
		// ---- formats: what a player of that format looks up (the player's own are marked)
		{"hypers", "beat hyper turbos", 49, 2, IfHyper, 0.0, 0.0, "", {207, 1609, 604}, {No, No}},
		{"pko", "PKO bounty maths", 45, 2, IfBounty, 0.0, 0.0, "", {605, 209, No}, {No, No}},
		{"mystery", "mystery bounty strategy", 37, 2, IfMystery, 0.0, 0.0, "", {607, 116, No}, {Env, No}},
		{"satellites", "how to win a satellite", 40, 2, IfSatellite, 0.0, 0.0, "", {606, 205, 1603}, {No, No}},
		{"omaha", "Omaha for beginners", 35, 2, IfOmaha, 0.3, 0.10, "", {1605, 208, No}, {No, No}},
		{"turbos", "turbo tournament strategy", 39, 2, IfTurbo, 0.0, 0.0, "", {207, 406, No}, {No, No}},
		{"freerolls", "freeroll strategy", 36, 2, IfFreeroll, 0.0, 0.0, "", {206, No, No}, {No, No}},
		{"series", "how to grind a series", 28, 2, IfSeries, 0.0, 0.0, "", {204, 806, No}, {No, No}},
		{"first-live", "first live tournament", 33, 2, IfLive, 6.5, 0.20, "", {401, 402, No}, {No, No}},
		{"sunday-majors", "Sunday majors", 42, 2, IfMajor, 0.0, 0.0, "", {202, 1306, 402}, {No, No}},
		// ---- stories: what people watch poker for
		{"bad-beat", "worst bad beat ever", 80, 3, 0, 0.0, 0.0, "", {101, 102, No}, {Beat, No}},
		{"kings-aces", "kings vs aces", 61, 3, 0, 0.0, 0.0, "", {105, 103, No}, {Cool, No}},
		{"set-set", "set over set", 33, 3, 0, 0.0, 0.0, "", {104, No, No}, {Sets, No}},
		{"royal", "royal flush", 66, 3, 0, 0.0, 0.0, "", {115, 1508, No}, {Monster, No}},
		{"big-bluff", "biggest bluff ever", 55, 3, 0, 0.0, 0.0, "", {106, 107, No}, {Bluff, No}},
		{"seven-deuce", "seven deuce bluff", 30, 3, 0, 0.0, 0.0, "", {107, No, No}, {No, No}},
		{"biggest-pot", "biggest pot ever", 43, 3, 0, 0.0, 0.0, "", {111, No, No}, {Pot, Biggest}},
		{"challenge", "bankroll challenge", 52, 3, 0, 0.2, 0.35, "", {1601, 501, No}, {No, No}},
		{"day-life", "day in the life of a poker player", 49, 3, 0, 0.0, 0.0, "", {1101, 1102, No}, {No, No}},
		{"living", "can you make a living playing poker", 51, 3, 0, 0.3, 0.10, "", {505, 512, 501}, {No, No}},
		{"downswing", "poker downswing", 29, 3, 0, 0.0, 0.0, "", {503, No, No}, {No, No}},
		{"setup", "poker stream setup", 28, 3, 0, 0.0, 0.0, "", {1105, 1106, No}, {No, No}},
		{"vlog", "poker vlog", 44, 3, 0, 6.5, 0.15, "", {201, 401, 202}, {No, No}},
		{"tournament-win", "winning a poker tournament", 46, 3, 0, 0.0, 0.0, "", {303, 302, No}, {Win, Ft}},
		{"bubble-boy", "bubble boy", 26, 3, 0, 0.0, 0.0, "", {306, No, No}, {No, No}},
		{"best-hands", "best poker hands this week", 50, 3, 0, 0.0, 0.0, "", {701, 702, No}, {No, No}},
		{"tier-list", "poker tier list", 31, 3, 0, 0.0, 0.0, "", {713, 708, 711}, {No, No}},
		{"rent", "paying rent with poker", 34, 3, 0, 0.0, 0.0, "", {505, 506, No}, {No, No}},
		{"stream-highlights", "poker stream highlights", 42, 3, 0, 0.0, 0.0, "", {118, 1504, No}, {Clip, No}},
		// ---- the calendar's: quiet most of the year, lifted by their season or their trend
		{"year-review", "poker year in review", 22, 3, 0, 11.9, 0.80, "", {511, 904, No}, {No, No}},
		{"poy", "player of the year race", 14, 3, 0, 0.0, 0.0, "poy", {803, No, No}, {No, No}},
		{"hof", "Hall of Fame class", 12, 3, 0, 0.0, 0.0, "hof", {804, No, No}, {No, No}},
		{"new-year", "new year poker goals", 18, 3, 0, 0.0, 0.50, "newyear", {1601, 501, No}, {No, No}},
		{"holiday", "holiday poker grind", 16, 3, 0, 11.7, 0.40, "holiday", {202, 201, No}, {No, No}},
	};
	return All;
}

/** The ideas that answer one of this week's trends (by its key's prefix), and whether the trend gets a phrase of its own (the
 *  calendar's lift the phrases above instead; a Sunday's majors are every week's). */
std::vector<int> ViTrendIdeas(const std::string& Key)
{
	const std::string P = Key.substr(0, Key.find(':'));
	return P == "series" ? std::vector<int>{806, 204} : P == "champ" ? std::vector<int>{801} : P == "final" ? std::vector<int>{802} :
		P == "site" ? std::vector<int>{805, 1301} : P == "overlay" ? std::vector<int>{807, 1307} : std::vector<int>();
}

/** A draw keyed on the phrase and the week (its Monday): this week's wobble around the phrase's level, 0.90-1.10. */
double ViWobble(const std::string& Key, int Monday)
{
	const uint32_t H = Fnv1a("search:" + Key + ":" + std::to_string(Monday));
	return 0.90 + 0.20 * static_cast<double>((H >> 8) % 10000u) / 10000.0;
}

/** The season's factor on a world day: 1 + Swing cos(2 pi (month position - Peak) / 12). */
double ViSeason(const ViSearch& S, int Day)
{
	if (S.Swing == 0.0)
	{
		return 1.0;
	}
	int Y = 0, Mo = 1, Dm = 1;
	world::CivilDate(Day, Y, Mo, Dm);
	const double Pos = static_cast<double>(Mo - 1) + static_cast<double>(Dm - 1) / 31.0;
	return 1.0 + S.Swing * std::cos(2.0 * 3.14159265358979323846 * (Pos - S.Peak) / 12.0);
}

/** A calendar trend's lift on the phrases it names: 1 + 12 (strength - 1) x the week's mean heat. */
double ViLift(const std::vector<Trend>& Ts, const char* Prefix)
{
	double L = 1.0;
	if (!Prefix || !*Prefix)
	{
		return L;
	}
	const std::string P = std::string(Prefix) + ":";
	for (const Trend& T : Ts)
	{
		L += T.Key.compare(0, P.size(), P) == 0 ? 12.0 * (T.Strength - 1.0) * T.Heat : 0.0;
	}
	return L;
}

/** Whether a phrase names the cards ("kings vs aces", "can you fold aces", "royal flush"): a video is on it only when its
 *  own cards show them (a cooler isn't kings against aces, a fold of nines isn't a fold of aces, quads aren't a royal). */
bool ViNamesCards(const std::string& Key)
{
	return Key == "kings-aces" || Key == "fold-aces" || Key == "royal";
}

/** Whether a kept video's cards ("mine/theirs/board") show what a phrase that names cards says. */
bool ViCardsShow(const std::string& Key, const Video& V)
{
	const std::string& Cards = V.Th.Cards;
	const size_t A1 = Cards.find('/'), A2 = A1 == std::string::npos ? std::string::npos : Cards.find('/', A1 + 1);
	if (A1 == std::string::npos || A2 == std::string::npos)
	{
		return false;
	}
	const std::vector<Card> Mine = ParseCards(Cards.substr(0, A1)), Theirs = ParseCards(Cards.substr(A1 + 1, A2 - A1 - 1)), Board = ParseCards(Cards.substr(A2 + 1));
	constexpr int Ten = 8, King = 11, Ace = 12;
	if (Mine.size() != 2)
	{
		return false;
	}
	if (Key == "kings-aces")
	{
		return Theirs.size() == 2 && ((ViPair(Mine, King) && ViPair(Theirs, Ace)) || (ViPair(Mine, Ace) && ViPair(Theirs, King)));
	}
	if (Key == "fold-aces")
	{
		return ViPair(Mine, Ace) && (V.K == Kind::HeroFold || V.K == Kind::LadderFold || V.X.Idea == 612 || V.X.Idea == 1502);
	}
	// A royal flush: ten to ace of one suit between the player's cards and the board, one of the player's among them.
	for (int Suit = 0; Suit < 4; ++Suit)
	{
		int Have = 0;
		bool Own = false;
		for (int Rank = Ten; Rank <= Ace; ++Rank)
		{
			const auto Is = [Rank, Suit](Card C) { return RankOf(C) == Rank && SuitOf(C) == Suit; };
			const bool InMine = std::any_of(Mine.begin(), Mine.end(), Is);
			Have += InMine || std::any_of(Board.begin(), Board.end(), Is) ? 1 : 0;
			Own = Own || InMine;
		}
		if (Have == 5 && Own)
		{
			return true;
		}
	}
	return false;
}
} // namespace

std::vector<std::string> SearchInventory()
{
	std::vector<std::string> Out;
	for (const ViSearch& S : ViSearches())
	{
		Out.push_back(S.Phrase);
	}
	return Out;
}
} // namespace vcontent

std::vector<SearchRow> Searches(const IdeaCtx& X, const Channel& C, const std::vector<IdeaCard>& Ideas, const std::vector<Trend>& ThisWeek, const std::vector<Trend>& LastWeek,
	double World, int Count)
{
	using namespace vcontent;
	const int Today = static_cast<int>(std::floor(World / ViDay));
	const int Monday = Today - ((Today % 7) + 7) % 7;
	// The formats the player played in the last 60 days (their rows are marked, and two of them always make the table).
	uint32_t Played = 0;
	for (const IdeaResult& R : X.Results)
	{
		Played |= R.At >= World - 60.0 * ViDay && R.At <= World ? R.Flags : 0u;
	}
	struct Raw
	{
		SearchRow R;
		double Now = 0, Before = 0;
		std::vector<int> Answers, Kinds;
		int TrendMine = 0;
	};
	std::vector<Raw> Rows;
	for (const ViSearch& S : ViSearches())
	{
		Raw W;
		W.R.Key = S.Key;
		W.R.Phrase = S.Phrase;
		W.R.Source = S.Source;
		W.R.YourFormat = S.Format != 0 && (Played & S.Format) != 0;
		// This week's at its Thursday (the season), against last week's at its own.
		W.Now = static_cast<double>(S.Base) * ViSeason(S, Monday + 3) * ViLift(ThisWeek, S.Lift) * ViWobble(S.Key, Monday);
		W.Before = static_cast<double>(S.Base) * ViSeason(S, Monday - 4) * ViLift(LastWeek, S.Lift) * ViWobble(S.Key, Monday - 7);
		for (const int I : S.Ideas)
		{
			if (I > 0)
			{
				W.Answers.push_back(I);
			}
		}
		for (const int K : S.Kinds)
		{
			if (K >= 0)
			{
				W.Kinds.push_back(K);
			}
		}
		Rows.push_back(W);
	}
	// This week's news and series: a phrase each (the trend's own words), searched as hard as the trend runs this week.
	for (const Trend& T : ThisWeek)
	{
		const std::vector<int> Answers = ViTrendIdeas(T.Key);
		if (Answers.empty())
		{
			continue;
		}
		Raw W;
		W.R.Key = "trend:" + T.Key;
		W.R.Phrase = T.Label;
		W.R.Source = 1;
		W.Now = (25.0 + 300.0 * (T.Strength - 1.0) * T.Heat) * ViWobble(T.Key, Monday);
		double Was = 0.0;
		for (const Trend& L : LastWeek)
		{
			Was = L.Key == T.Key ? L.Heat * (L.Strength - 1.0) : Was;
		}
		W.Before = (25.0 + 300.0 * Was) * ViWobble(T.Key, Monday - 7);
		W.Answers = Answers;
		W.TrendMine = T.Mine;
		Rows.push_back(W);
	}
	double Max = 0.0;
	for (const Raw& W : Rows)
	{
		Max = std::max(Max, W.Now);
	}
	for (Raw& W : Rows)
	{
		W.R.Interest = Max > 0.0 ? std::clamp(static_cast<int>(std::lround(100.0 * W.Now / Max)), 1, 100) : 1;
		W.R.Arrow = W.Now >= 1.10 * W.Before ? 1 : W.Now <= 0.90 * W.Before ? -1 : 0;
		// Yours on it: an idea video that answers it, a hand of its kind (or, for a trend, the uploads that matched it); a phrase
		// that names the cards counts only the videos whose cards show them ("23 of yours" is a fact or it isn't said).
		int Mine = 0;
		const bool Named = ViNamesCards(W.R.Key);
		for (const Video& V : C.Videos)
		{
			const bool ByIdea = V.X.On && V.X.Idea >= 0 && std::find(W.Answers.begin(), W.Answers.end(), V.X.Idea) != W.Answers.end();
			const bool ByKind = std::find(W.Kinds.begin(), W.Kinds.end(), static_cast<int>(V.K)) != W.Kinds.end();
			Mine += (Named ? ViCardsShow(W.R.Key, V) : (ByIdea || ByKind)) ? 1 : 0;
		}
		W.R.Mine = std::max(Mine, W.TrendMine);
		// The idea to make next: the first ready one that answers it, else the first on the board (its hint says what to do).
		const IdeaCard* Pick = nullptr;
		for (const int Id : W.Answers)
		{
			for (const IdeaCard& Card : Ideas)
			{
				if (Card.Id == Id && (Card.Ready ? !Pick || !Pick->Ready : !Pick))
				{
					Pick = &Card;
				}
			}
		}
		if (Pick)
		{
			W.R.Idea = Pick->Id;
			W.R.IdeaName = Pick->Name;
			W.R.IdeaReady = Pick->Ready;
			W.R.IdeaHint = Pick->Ready ? std::string() : Pick->Hint;
		}
	}
	std::stable_sort(Rows.begin(), Rows.end(), [](const Raw& A, const Raw& B) { return A.Now != B.Now ? A.Now > B.Now : A.R.Key < B.R.Key; });
	const size_t N = Count > 0 ? std::min(Rows.size(), static_cast<size_t>(Count)) : Rows.size();
	std::vector<Raw> Shown(Rows.begin(), Rows.begin() + static_cast<std::ptrdiff_t>(N));
	// At least two of the player's own formats (the most searched of them), in place of the least searched phrases that aren't
	// theirs or this week's news.
	int Theirs = 0;
	for (const Raw& W : Shown)
	{
		Theirs += W.R.YourFormat ? 1 : 0;
	}
	for (size_t I = N; I < Rows.size() && Theirs < 2 && N >= 4; ++I)
	{
		if (!Rows[I].R.YourFormat)
		{
			continue;
		}
		for (size_t J = Shown.size(); J-- > 0;)
		{
			if (!Shown[J].R.YourFormat && Shown[J].R.Source != 1)
			{
				Shown[J] = Rows[I];
				++Theirs;
				break;
			}
		}
	}
	std::stable_sort(Shown.begin(), Shown.end(), [](const Raw& A, const Raw& B) { return A.Now != B.Now ? A.Now > B.Now : A.R.Key < B.R.Key; });
	std::vector<SearchRow> Out;
	for (const Raw& W : Shown)
	{
		Out.push_back(W.R);
	}
	return Out;
}
} // namespace video
} // namespace ss
