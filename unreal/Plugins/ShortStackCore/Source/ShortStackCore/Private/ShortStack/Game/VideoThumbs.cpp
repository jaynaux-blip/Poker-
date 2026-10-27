// Update 3 (SPEC §6.1, §4.6.2, §7.3): Rerun 2.0's thumbnail recipes, core-videocontent's file. 36 archetypes (35 dealt in P0:
// POV is vertical and Collab Duo is a collab, both P1) over the layouts, backgrounds, palettes, subjects, expressions, poses,
// props, caption styles, stickers and corner marks of VideoContent.h's thumb vocabulary, dealt six at a time in hash order
// (never sorted by fit), with a fit that depends on the moment (ArchetypeFit), each archetype's Update 2 frame (FrameOf), the
// hidden traits normalized once per kind group by a pinned constant (TraitMean), the promise a recipe makes (ThumbPromise)
// and its visual signature (ThumbSignature: the caption text left out).
//
// Captions come only from facts (U7): an archetype's caption rows are filled from the same facts a title reads, and a row
// whose fact the moment lacks is never dealt. Storytime draws nobody but the player and names nobody: no opponent face, no
// name in a caption. The compositor's own facts (the cards, the marked card, the other face, a number, a progress, the
// graph, the site, a chat line) ride after the caption's second line (thumb::FactSep).
#include "VideoContentPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Chat.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"

#include <algorithm>
#include <array>
#include <cmath>
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
int GroupOf(Kind K); // Video.cpp
bool EventKind(Kind K);
int MadeLevel(const std::vector<Card>& Mine, const std::vector<Card>& Board);
} // namespace video_detail

namespace vcontent
{
namespace
{
/** Bits of a palette set. */
constexpr uint32_t VtPal(std::initializer_list<int> P)
{
	uint32_t M = 0;
	for (const int X : P)
	{
		M |= 1u << X;
	}
	return M;
}

/** One caption row: the condition (as a title's: "#L "), the two lines, the caption style it prefers (-1: the archetype's). */
struct VtCap
{
	const char* Line1;
	const char* Line2;
};

/** An archetype's rules: its frame, layouts, the allowed backgrounds, palettes, expressions, poses, props, caption styles,
 *  stickers, the subject, its caption rows, its read, and its base promise. */
struct VtArch
{
	const char* Name;
	const char* Read;
	Frame Fr;
	std::vector<int> Layouts, Bgs;
	uint32_t Pals;
	std::vector<int> Exprs, Poses, PropsA, PropsB, Caps, Stickers;
	int Subj;
	std::vector<VtCap> Captions;
	int Promise;
};

const std::vector<VtArch>& VtTable()
{
	using namespace thumb;
	static const std::vector<VtArch> All = {
		{"Gut Punch", "your face, the beat in words: for a hand you lost when you were ahead", Frame::Reaction, {LyFaceWords, LyCardsFace}, {BgBurst, BgSpotlight, BgGradient},
			VtPal({PlHeat, PlBlood, PlNight, PlMono}), {ExDevastated, ExShocked, ExScream}, {PsHandsOnHead, PsNone}, {PrHoleCards}, {PrShownCards, -1}, {CsTwoTone, CsOutlined, CsStamp},
			{StRedX, StCryingFace, -1}, SjYou,
			{{"#L {Eq}%", "AND LOST"}, {"#L {MineShort}", "{Cracked}"}, {"#L THE {KillStreet}", ""}, {"#L {Bb} BB", "GONE"}, {"#L WHY.", ""}}, 1},
		{"Circled River", "the river, circled: a board that tells the story", Frame::Cards, {LyRiver}, {BgSpotlight, BgGradient, BgFeltBokeh}, VtPal({PlNight, PlFelt, PlHeat, PlIce}),
			{ExShocked, ExDevastated, ExLaugh}, {PsNone}, {PrBoard}, {PrHoleCards, PrShownCards}, {CsBoxed, CsOutlined, CsLowerThird}, {StRedCircle, StCurvedArrow}, SjYou,
			{{"THE {TurnStreet}.", ""}, {"THE {TurnStreet}", "DID THIS"}, {"ONE CARD.", ""}, {"#L WHY THE", "{TurnStreet}"}}, 1},
		{"The Read", "your face and two backs: a call or a fold that came down to a read", Frame::Cards, {LyCardsFace, LyFaceFan}, {BgFeltBokeh, BgGradient, BgSpotlight},
			VtPal({PlGoldOnBlack, PlFelt, PlNight, PlIce}), {ExSkeptical, ExThinking}, {PsNone, PsPoint}, {PrHoleCards}, {PrCardBacks}, {CsOutlined, CsBoxed, CsTwoTone},
			{StQuestionMarks, -1}, SjYou, {{"{HeroCall}", ""}, {"{IKnew}", ""}, {"{ReadIt}", ""}, {"{Bb} BB", "{CallQ}"}}, 1},
		{"Nerves of Steel", "a calm face over the cards nobody saw: for a bluff", Frame::Reaction, {LyFaceFan}, {BgSpeedLines, BgBurst, BgGradient},
			VtPal({PlNeon, PlNight, PlGoldOnBlack, PlHeat}), {ExSmug, ExGrin}, {PsNone, PsPoint}, {PrHoleCards}, {PrChipPile, -1}, {CsMarker, CsOutlined, CsStamp},
			{StCrown, StAllInStamp, -1}, SjYou, {{"#0 NOTHING", ""}, {"{MineDash}", ""}, {"{Bb} BB", "BLUFF"}, {"NO PAIR", ""}}, 1},
		{"Caught", "a facepalm and a CALLED stamp: for a bluff that didn't work", Frame::Reaction, {LyFaceWords, LyFaceFan}, {BgGradient, BgSpotlight, BgDiagonal},
			VtPal({PlBlood, PlMono, PlHeat, PlNight}), {ExExhausted, ExDevastated, ExShocked}, {PsFacepalm}, {PrHoleCards}, {PrShownCards, -1}, {CsStamp, CsOutlined},
			{StRedX, StSkull, -1}, SjYou, {{"CALLED", ""}, {"CAUGHT", ""}, {"{Bb} BB", "GONE"}, {"BUSTED", ""}}, 1},
		{"The Fold", "a premium hand with a red X: for a big fold", Frame::Cards, {LyQuestion, LyCardsFace}, {BgGradient, BgSpotlight, BgDiagonal}, VtPal({PlIce, PlNight, PlMono, PlFelt}),
			{ExThinking, ExSkeptical}, {PsNone, PsPoint}, {PrHoleCards}, {PrOptionCards, -1}, {CsTwoTone, CsBoxed, CsOutlined}, {StRedX, StQuestionMarks}, SjYou,
			{{"FOLD?", ""}, {"{MineShort}?", ""}, {"{Left} LEFT", "FOLD?"}, {"I FOLDED", "{MineShort}"}}, 1},
		{"The Pile", "a top-down felt and the pile: for a big pot", Frame::Number, {LyTableTop}, {BgBurst, BgFeltBokeh, BgGradient}, VtPal({PlMoney, PlFelt, PlGoldOnBlack, PlSunrise}),
			{ExGrin, ExLaugh, ExSmile}, {PsFistPump, PsNone}, {PrFeltTop}, {PrChipPile}, {CsNumberBurst, CsOutlined, CsBoxed}, {StCurvedArrow, StMoneyBag, -1}, SjYou,
			{{"{Bb} BB", ""}, {"SHIP IT", ""}, {"{Bb} BIG", "BLINDS"}}, 1},
		{"Big Number", "the number, huge: for a cash, a win or a big pot", Frame::Number, {LyNumberFace}, {BgBurst, BgGradient, BgMoneyRain}, VtPal({PlGoldOnBlack, PlMoney, PlSunrise, PlHeat}),
			{ExGrin, ExLaugh, ExScream}, {PsFistPump, PsNone}, {PrCashStack, -1}, {PrTrophy, -1}, {CsNumberBurst, CsOutlined}, {StMoneyBag, -1}, SjYou,
			{{"{Prize}", ""}, {"{Bb} BB", ""}, {"{PlaceU} OF", "{Field}"}, {"{Prize}", "{PlaceU}"}, {"{Events}", "EVENTS"}, {"{Days}", "DAYS"}, {"{Views}", "VIEWS"}, {"{Subs}", "SUBS"}, {"{Comments}", "COMMENTS"}, {"{Shifts}", "SHIFTS"},
			{"{GearCost}", ""}, {"{Overlay}", "OVERLAY"}, {"{CountHands}", "HANDS"}}, 1},
		{"Before -> After", "two panels and an arrow: where it started and where it got to", Frame::Number, {LyBeforeAfter}, {BgGradient, BgDiagonal, BgBurst},
			VtPal({PlSunrise, PlGoldOnBlack, PlMoney, PlIce}), {ExGrin, ExLaugh, ExSmile}, {PsFistPump}, {PrCashStack, PrShield}, {-1}, {CsLowerThird, CsStamp, CsOutlined}, {-1, StCurvedArrow},
			SjYou, {{"{PlaceU} OF {Field}", ""}, {"{PlaceU} OF", "{Field}"}, {"PROMOTED", ""}, {"DAY {Day}", ""}, {"{Prize}", ""}}, 1},
		{"Trophy Shot", "the trophy and its rays: for a win", Frame::Trophy, {LyTrophy}, {BgBurst, BgSpotlight}, VtPal({PlGoldOnBlack, PlSunrise, PlMoney}), {ExLaugh, ExGrin, ExSmile},
			{PsFistPump, PsNone}, {PrTrophy, PrBracelet, PrRing}, {PrEventCrest, -1}, {CsOutlined, CsNumberBurst, CsStamp}, {StCrown, -1}, SjYou,
			{{"{IWon}", ""}, {"{Champ}", ""}, {"{Prize}", "{IWon}"}, {"{WinnerWord}", ""}, {"{Honour}", ""}}, 1},
		{"VS Split", "a diagonal split, two faces: for a rival or a cooler", Frame::Versus, {LySplitVs}, {BgDiagonal, BgSpeedLines, BgBurst}, VtPal({PlBlood, PlHeat, PlNeon, PlIce}),
			{ExShocked, ExTilted, ExSmug}, {PsNone, PsPoint}, {PrHoleCards, -1}, {PrShownCards, -1}, {CsStamp, CsOutlined, CsTwoTone}, {StBolt, -1}, SjOpponent,
			{{"{VsWord}", ""}, {"{MineShort} VS", "{TheirsShort}"}, {"{Rivals}", ""}, {"{Bb} BB", "{CoolerWord}"}}, 1},
		{"The Bubble", "a teetering stack and one place to go: for a bubble", Frame::Reaction, {LyFaceWords, LyNumberFace}, {BgSpotlight, BgGradient, BgSpeedLines},
			VtPal({PlIce, PlHeat, PlNight, PlBlood}), {ExShocked, ExThinking, ExExhausted}, {PsHandsOnHead, PsNone}, {PrChipPile}, {-1, PrProgressBar}, {CsTwoTone, CsOutlined, CsBoxed},
			{StSiren, -1}, SjYou, {{"ONE AWAY", ""}, {"BUBBLE", ""}, {"{Left} LEFT", ""}, {"{StackBb} BB", "LEFT"}}, 1},
		{"Headhunter", "a crosshair over the bust: for a knockout", Frame::Versus, {LySplitVs, LyFaceWords}, {BgSpeedLines, BgBurst, BgGradient}, VtPal({PlBlood, PlHeat, PlNeon, PlGoldOnBlack}),
			{ExSmug, ExGrin}, {PsPoint, PsFistPump}, {PrCrosshair}, {PrCashStack, -1}, {CsStamp, CsOutlined, CsNumberBurst}, {StSkull, -1}, SjOpponent,
			{{"KNOCKOUT", ""}, {"{BountyWord}", ""}, {"{Bounty}", ""}, {"{Payback}", ""}, {"GOT 'EM", ""}}, 1},
		{"Envelope", "a torn envelope and the multiple: for a mystery bounty", Frame::Number, {LyNumberFace, LyCollage}, {BgMoneyRain, BgBurst}, VtPal({PlGoldOnBlack, PlMoney, PlSunrise}),
			{ExScream, ExShocked, ExLaugh}, {PsHandsOnHead, PsFistPump}, {PrMysteryEnvelope}, {PrCashStack, -1}, {CsNumberBurst, CsOutlined}, {StMoneyBag, -1}, SjYou,
			{{"{Mult}X", ""}, {"{Bounty}", ""}, {"{Mult}X", "BOUNTY"}}, 2},
		{"The Graph", "the bankroll line and your face: for a month or a swing", Frame::Number, {LyGraph}, {BgGradient, BgSpotlight}, VtPal({PlMoney, PlBlood, PlIce, PlNight}),
			{ExSmile, ExExhausted, ExDevastated, ExGrin}, {PsPoint, PsFacepalm}, {PrGraphLine}, {-1}, {CsNumberBurst, CsMarker, CsOutlined}, {-1, StCurvedArrow}, SjYou,
			{{"#U {NetPlus}", ""}, {"#M -{Down} BUY-INS", ""}, {"#U +{Up} BUY-INS", ""}, {"{MonthU}", "{NetPlus}"}, {"#M {MonthU}", ""}, {"{Down} BUY-INS", "DOWN"}, {"{Up} BUY-INS", "UP"}, {"{AbiWas} TO", "{Abi}"}, {"{Events}", "EVENTS"}, {"{PlaceU} OF", "{Field}"}}, 1},
		{"Day Counter", "the day, huge, and a bar from start to goal: for a series", Frame::Number, {LyNumberFace, LyGraph}, {BgSpeedLines, BgGradient}, VtPal({PlSite, PlSunrise, PlIce, PlNeon}),
			{ExGrin, ExSmile, ExThinking}, {PsFistPump, PsPoint}, {PrProgressBar}, {PrSiteMark, -1}, {CsOutlined, CsNumberBurst}, {-1}, SjYou,
			{{"DAY {Day}", ""}, {"EPISODE {Ep}", ""}, {"DAY {Day}", "{Now}"}}, 0},
		{"Night Shift", "the laptop's glow at the hour: for a long session", Frame::Reaction, {LyDesk, LyFaceWords}, {BgScanlines, BgGradient}, VtPal({PlNight, PlIce, PlNeon}),
			{ExExhausted, ExThinking}, {PsNone}, {PrLaptopLobby}, {-1, PrPhoneChat}, {CsLowerThird, CsMarker}, {-1}, SjYou,
			{{"{Events} EVENTS", ""}, {"ALL NIGHT", ""}, {"{Time}", ""}, {"{NetSignedU}", ""}}, 0},
		{"Venue", "the Embercrest's marquee and your portrait: for a live night", Frame::Reaction, {LyVenue}, {BgSpotlight, BgGradient}, VtPal({PlGoldOnBlack, PlNight, PlSunrise}),
			{ExSmile, ExGrin, ExThinking}, {PsNone}, {PrVenueMark}, {-1, PrEventCrest}, {CsLowerThird, CsOutlined}, {-1}, SjYou,
			{{"LIVE", ""}, {"EMBERCREST", ""}, {"{PlaceU} OF", "{Field}"}, {"#F FIRST", "NIGHT"}}, 0},
		{"Road Trip", "the bus and the city lights: for the trip there and back", Frame::Reaction, {LyCollage, LyFaceWords}, {BgGradient, BgSpotlight}, VtPal({PlNight, PlSunrise, PlIce}),
			{ExExhausted, ExNeutral, ExSmile}, {PsNone}, {PrBus}, {-1}, {CsLowerThird, CsMarker}, {-1}, SjYou, {{"BUS HOME", ""}, {"{Time}", ""}, {"THE BUS", ""}}, 0},
		{"Rent Letter", "the notice or the bill, stamped: for rent stories", Frame::Number, {LyLetter}, {BgSpotlight, BgGradient}, VtPal({PlMoney, PlMono, PlBlood, PlIce}),
			{ExLaugh, ExExhausted, ExShocked}, {PsNone, PsHandsOnHead}, {PrEviction, PrPowerBill}, {-1}, {CsBoxed, CsStamp}, {-1}, SjYou,
			{{"RENT PAID.", ""}, {"EVICTED.", ""}, {"#R {Short}", "SHORT"}, {"{Rent}", ""}, {"BACK HOME.", ""}}, 1},
		{"Classroom", "a whiteboard and a range grid: for a lesson", Frame::Reaction, {LyFaceWords, LyCollage}, {BgGradient, BgSpotlight}, VtPal({PlNight, PlIce, PlFelt, PlMono}),
			{ExThinking, ExSmile}, {PsPoint}, {PrWhiteboard}, {-1, PrHoleCards}, {CsBoxed, CsMarker}, {-1}, SjYou,
			{{"{Lesson}", ""}, {"LESSON", ""}, {"{FoldShort}", "FOLD?"}}, 0},
		{"Quiz", "two option cards and a question: for a quiz", Frame::Cards, {LyQuestion}, {BgDiagonal, BgGradient}, VtPal({PlIce, PlNight, PlFelt, PlSunrise}), {ExThinking, ExSkeptical},
			{PsNone, PsPoint}, {PrOptionCards}, {PrHoleCards, -1}, {CsTwoTone, CsBoxed}, {StQuestionMarks, -1}, SjYou, {{"CALL?", ""}, {"FOLD?", ""}, {"{Bb} BB", "CALL?"}, {"QUIZ", ""}}, 1},
		{"Tier List", "rows S to C: for a ranking", Frame::Number, {LyTierList}, {BgGradient, BgDiagonal}, VtPal({PlNight, PlNeon, PlSunrise, PlIce}), {ExSmug, ExThinking},
			{PsPoint}, {PrTierRows}, {-1}, {CsBoxed, CsOutlined}, {-1}, SjYou, {{"RANKED", ""}, {"TIER LIST", ""}, {"S TIER?", ""}}, 0},
		{"Top Five", "numbered cells and a crown: for a compilation", Frame::Trophy, {LyTierList, LyCollage}, {BgBurst, BgGradient}, VtPal({PlHeat, PlGoldOnBlack, PlNeon, PlSunrise}),
			{ExGrin, ExShocked}, {PsPoint, PsFistPump}, {PrNumberedCells}, {-1}, {CsOutlined, CsNumberBurst}, {StCrown, -1}, SjYou, {{"TOP {Count}", ""}, {"BEST OF", "{MonthU}"}, {"{Count}", "HANDS"}, {"{RunWord}", ""}, {"{PlaceU} OF", "{Field}"}}, 1},
		{"Desk Setup", "the desk with the gear you own: for a setup tour", Frame::Reaction, {LyDesk}, {BgGradient, BgSpotlight}, VtPal({PlNight, PlMono, PlIce, PlNeon}), {ExSmile, ExGrin},
			{PsPoint, PsNone}, {PrGear}, {PrLaptopLobby, -1}, {CsOutlined, CsBoxed}, {-1}, SjYou, {{"MY {GearCost}", "SETUP"}, {"MY SETUP", ""}, {"NEW GEAR", ""}, {"{GearCost}", ""}}, 0},
		{"Collab Duo", "two faces and a plus: for a collab", Frame::Versus, {LySplitVs}, {BgGradient}, VtPal({PlSunrise}), {ExGrin}, {PsNone}, {-1}, {-1}, {CsOutlined}, {-1}, SjCreator, {}, 0},
		{"Reaction Box", "your face in a box beside the news: for a reaction", Frame::Reaction, {LyCollage, LyFaceWords}, {BgBurst, BgGradient}, VtPal({PlGoldOnBlack, PlNeon, PlHeat, PlNight}),
			{ExShocked, ExLaugh, ExThinking}, {PsHandsOnHead, PsPoint}, {PrEventCrest}, {PrTrophy, -1}, {CsNumberBurst, CsOutlined}, {StShockedFace, -1}, SjWorldPlayer,
			{{"{Prize}", ""}, {"{News}", ""}, {"WOW", ""}, {"{LeaderU}", ""}}, 1},
		{"Stream Clip", "the stream frame, the facecam and chat: for a stream moment", Frame::Reaction, {LyStreamClip}, {BgScanlines, BgGradient}, VtPal({PlNeon, PlNight, PlHeat}),
			{ExLaugh, ExShocked, ExScream}, {PsHandsOnHead, PsFistPump, PsNone}, {PrPhoneChat}, {PrLaptopLobby, -1}, {CsStamp, CsOutlined}, {-1, StSiren}, SjYou,
			{{"CHAT LOST IT", ""}, {"{Watching}", "WATCHING"}, {"LIVE", ""}}, 1},
		{"Milestone", "a number in confetti: for a milestone", Frame::Trophy, {LyNumberFace}, {BgBurst, BgGradient}, VtPal({PlSunrise, PlGoldOnBlack, PlNeon}), {ExLaugh, ExSmile},
			{PsFistPump}, {PrConfetti}, {-1}, {CsNumberBurst, CsOutlined}, {-1}, SjYou, {{"{Subs}", ""}, {"THANK YOU", ""}, {"{Subs}", "THANK YOU"}}, 0},
		{"Text Slab", "big words on a colour block, a small face: for a story", Frame::Reaction, {LyTextSlab}, {BgGradient}, VtPal({PlMono, PlNight, PlSunrise, PlBlood}),
			{ExNeutral, ExSkeptical, ExExhausted}, {PsNone}, {PrStickyNote, -1}, {-1}, {CsLowerThird, CsMarker, CsBoxed}, {-1}, SjYou,
			{{"#Y THEY HAD IT", ""}, {"#Y STORYTIME", ""}, {"#Y NO NAMES", ""}, {"{Slab}", ""}, {"{Slab1}", "{Slab2}"}}, 0},
		{"The Ghost", "gh0stfold's ghost over your shoulder: for the rival saga", Frame::Versus, {LyFaceWords, LySplitVs}, {BgSpotlight, BgScanlines}, VtPal({PlNeon, PlNight, PlBlood}),
			{ExTilted, ExShocked, ExSmug}, {PsNone}, {-1}, {-1}, {CsStamp, CsOutlined}, {StGhost, -1}, SjGhost, {{"HIM AGAIN", ""}, {"{OppU}", ""}, {"#W FINALLY", ""}}, 1},
		{"Payday", "the statement and a cash stack: for creator money", Frame::Number, {LyLetter, LyNumberFace}, {BgMoneyRain, BgGradient}, VtPal({PlMoney, PlGoldOnBlack, PlSunrise}),
			{ExGrin, ExSmile}, {PsFistPump, PsPoint}, {PrStatement}, {PrCashStack}, {CsNumberBurst, CsBoxed}, {StMoneyBag, -1}, SjYou, {{"{Paid}", ""}, {"PAYDAY", ""}, {"{TierU}", ""}}, 0},
		{"Monster Hand", "five cards fanned with a glow: for a monster", Frame::Cards, {LyFaceFan, LyCardsFace}, {BgBurst, BgSpotlight}, VtPal({PlGoldOnBlack, PlNeon, PlHeat}),
			{ExScream, ExLaugh, ExShocked}, {PsHandsOnHead, PsFistPump}, {PrCardFan}, {-1}, {CsOutlined, CsNumberBurst}, {-1, StBolt}, SjYou, {{"{MadeShort}", ""}, {"{MadeShort}", "!!"}}, 2},
		{"POV", "first person over the felt (vertical)", Frame::Cards, {LyCardsFace}, {BgFeltBokeh}, VtPal({PlFelt}), {ExNeutral}, {PsNone}, {-1}, {-1}, {CsOutlined}, {-1}, SjNobody, {}, 0},
		{"Site Card", "the site's strip and colours: for a night on a site", Frame::Reaction, {LyFaceWords, LyNumberFace}, {BgBurst, BgGradient}, VtPal({PlSite, PlNeon, PlHeat}),
			{ExScream, ExGrin, ExSmile}, {PsFistPump, PsNone}, {PrSiteMark}, {PrFuseReel, -1}, {CsNumberBurst, CsOutlined}, {-1}, SjYou,
			{{"x{Mult}", ""}, {"FIRST NIGHT", ""}, {"{SiteU}", ""}, {"SPECIAL", "GUEST"}}, 1},
		{"Mistake", "a red X over your card and a facepalm: for a mistake you made", Frame::Cards, {LyCardsFace, LyFaceWords}, {BgGradient, BgDiagonal}, VtPal({PlBlood, PlMono, PlIce}),
			{ExExhausted, ExDevastated}, {PsFacepalm}, {PrHoleCards}, {-1}, {CsMarker, CsStamp}, {StRedX}, SjYou, {{"MY MISTAKE", ""}, {"{CostBb} BB", "LOST"}, {"WRONG", ""}}, 1},
	};
	return All;
}

/** The Update 2 kind group's emotion: the expressions a face should wear on it (ThumbTraits' match). */
bool VtEmotionMatch(int Group, int Expr, const Candidate& Lead)
{
	using namespace thumb;
	switch (Group)
	{
	case 0: return Lead.Rec.HeroNet >= 0 ? (Expr == ExLaugh || Expr == ExShocked || Expr == ExScream) : (Expr == ExDevastated || Expr == ExShocked || Expr == ExScream || Expr == ExExhausted);
	case 1: return Expr == ExSkeptical || Expr == ExThinking || Expr == ExSmug || Expr == ExExhausted;
	case 2: return Expr == ExGrin || Expr == ExLaugh || Expr == ExSmile || Expr == ExScream || Expr == ExSmug;
	case 3: return Expr == ExLaugh || Expr == ExGrin || Expr == ExSmile || Expr == ExScream;
	case 4: return Expr == ExTilted || Expr == ExSmug || Expr == ExShocked;
	default: break;
	}
	return Expr != ExNeutral;
}

/** The kast::Mood a stream moment's face wore, as a thumbnail expression (stream and thumbnail agree). */
int VtMoodExpr(int Mood)
{
	using namespace thumb;
	static const int Map[6] = {ExNeutral, ExSmile, ExGrin, ExShocked, ExTilted, ExLaugh};
	return Map[std::clamp(Mood, 0, 5)];
}

/** The street (4 turn, 5 river) whose card turned the hand, from the player's view; 0 when none. */
int VtTurnStreet(const Candidate& L, int& BoardIndex)
{
	BoardIndex = -1;
	const std::vector<Card>* Theirs = nullptr;
	for (const auto& S : L.Rec.Shown)
	{
		Theirs = S.second.size() >= 2 && (!Theirs || S.first == L.Opponent) ? &S.second : Theirs;
	}
	if (!Theirs || L.Rec.Mine.size() < 2 || L.Rec.Board.size() < 5)
	{
		return 0;
	}
	const bool HeroWon = L.Rec.HeroNet > 0;
	for (int N = 5; N >= 3; --N)
	{
		const std::vector<Card> Part(L.Rec.Board.begin(), L.Rec.Board.begin() + N);
		const int Me = EvaluateHand(L.Rec.Mine, Part), Them = EvaluateHand(*Theirs, Part);
		if (!(HeroWon ? Me > Them : Them > Me))
		{
			if (N == 5)
			{
				return 0;
			}
			BoardIndex = N;
			return N + 1;
		}
	}
	return 0;
}

std::string VtCards(const std::vector<Card>& C)
{
	std::string S;
	for (const Card X : C)
	{
		S += CardToString(X);
	}
	return S;
}

/** The caption facts an archetype's rows read: the title's facts and a few of its own, in capitals where a caption wants them. */
Facts VtCaptionFacts(const Facts& F, const Candidate& L)
{
	Facts C = F;
	auto Set = [&C](const char* K, const std::string& V) {
		if (!V.empty())
		{
			C.V[K] = V;
		}
	};
	auto Get = [&F](const char* K) { return F.V.count(K) ? F.V.at(K) : std::string(); };
	Set("PlaceU", UpperU3(Get("Place")));
	Set("MonthU", UpperU3(Get("Month")));
	Set("NetSignedU", UpperU3(Get("NetSigned")));
	Set("SiteU", UpperU3(Get("Site")));
	Set("TierU", UpperU3(Get("Tier")));
	Set("LeaderU", UpperU3(Get("Leader").size() <= 14 ? Get("Leader") : std::string()));
	Set("OppU", L.Storytime ? std::string() : UpperU3(Get("Opp")));
	if (L.Rec.Mine.size() >= 2)
	{
		const int A = std::max(RankOf(L.Rec.Mine[0]), RankOf(L.Rec.Mine[1])), B = std::min(RankOf(L.Rec.Mine[0]), RankOf(L.Rec.Mine[1]));
		Set("MineShort", std::string(1, RankChars[A]) + RankChars[B]);
	}
	for (const auto& S : L.Rec.Shown)
	{
		if (S.second.size() >= 2 && !L.Storytime)
		{
			const int A = std::max(RankOf(S.second[0]), RankOf(S.second[1])), B = std::min(RankOf(S.second[0]), RankOf(S.second[1]));
			Set("TheirsShort", std::string(1, RankChars[A]) + RankChars[B]);
			break;
		}
	}
	int Idx = -1;
	const int Street = VtTurnStreet(L, Idx);
	Set("TurnStreet", Street == 5 ? "RIVER" : Street == 4 ? "TURN" : std::string());
	const std::string Made = Get("MadeHand");
	Set("MadeShort", Made.find("royal") != std::string::npos ? "ROYAL" : Made.find("straight") != std::string::npos ? "STRAIGHT FLUSH" : Made.rfind("quad", 0) == 0 ? "QUADS" : std::string());
	if (C.V.count("MadeShort") && C.V.at("MadeShort").size() > 14)
	{
		C.V.erase("MadeShort");
	}
	// A lesson's name on the whiteboard (Classroom), the fold's hand ("KK / FOLD?"), the news stamp (Reaction Box).
	const int Idea = L.Idea, G = Idea / 100;
	const char* Lesson = (Idea == 603 || Idea == 610 || L.K == Kind::LadderFold || L.K == Kind::Bubble || L.K == Kind::MinCash || L.K == Kind::FinalTable) ? "ICM" :
		(Idea == 604 || Idea == 609) ? "PUSH/FOLD" : Idea == 605 ? "PKO MATHS" : (Idea == 607 || L.K == Kind::Envelope) ? "ENVELOPES" :
		(Idea == 606 || Idea == 205) ? "SATELLITES" : (Idea == 614 || Idea == 713) ? "PREFLOP" : (Idea == 1605 || Idea == 208) ? "OMAHA" :
		Idea == 611 ? "COACHED" : (Idea == 508 || Idea == 503 || Idea == 509) ? "BANKROLL" : (Idea == 1302 || Idea == 613 || Idea == 710) ? "THE FIELDS" : Idea == 805 ? "FIRST LOOK" : (Idea == 802 || Idea == 804 || Idea == 809) ? "HOW THEY PLAY" : "";
	Set("Lesson", Lesson);
	if ((L.K == Kind::HeroFold || L.K == Kind::LadderFold || Idea == 612 || Idea == 603) && C.V.count("MineShort"))
	{
		Set("FoldShort", C.V.at("MineShort"));
	}
	Set("News", G == 8 ? "NEWS" : "");
	// "I WON" only on the player's own win; a count of hands as Big Number's number where the idea counts hands.
	const bool Mine = L.PlaceSoFar == 1 || Idea == 303 || Idea == 309 || Idea == 205 || Idea == 606 || Idea == 1603 || Idea == 1406 || Idea == 1408 ||
		Idea == 1411 || Idea == 1403;
	Set("IWon", Mine && G != 8 ? "I WON" : "");
	// The trophy's words: the player's own title, or a news story's champion (H1, the winner holds it); the Hall of Fame and
	// the Player of the Year race name their honour, never a win.
	Set("Champ", (Mine && G != 8) || Idea == 801 ? "CHAMPION" : "");
	Set("WinnerWord", (Mine && G != 8) || Idea == 801 ? "WINNER" : "");
	Set("Honour", Idea == 804 ? "HALL OF FAME" : Idea == 803 ? "THE POY RACE" : "");
	Set("HeroCall", L.K == Kind::HeroCall || Idea == 109 ? "HERO CALL" : "");
	// Words a caption says only when they're true: CRACKED for a hand that was ahead and lost (not a caught bluff), a read
	// that was right ("I KNEW"), CALL? on a decision to call or fold, BOUNTY on a bounty, PAYBACK on a debt paid back.
	{
		int TurnIdx = -1;
		const bool Ahead = L.Rec.HeroNet < 0 && L.K != Kind::BluffCaught && L.K != Kind::Bluff && (VtTurnStreet(L, TurnIdx) >= 4 || L.EquityAtAllIn >= 0.5);
		Set("Cracked", Ahead ? "CRACKED" : "");
		// (A caught bluff was the wrong read, and the player made the bet there, so nobody asks "call?" of it.)
		const bool Wrong = L.K == Kind::BluffCaught || (L.K == Kind::Bluff && L.Rec.HeroNet <= 0);
		Set("IKnew", Wrong ? "" : "I KNEW");
		Set("ReadIt", Wrong ? "" : "READ IT");
		Set("CallQ", Wrong || L.K == Kind::Bluff ? "" : "CALL?");
		std::string Ev = L.Event;
		for (char& Ch : Ev)
		{
			Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
		}
		const bool BountyEvent = Ev.find("bounty") != std::string::npos || Ev.find("pko") != std::string::npos || Ev.find("mystery") != std::string::npos ||
			Ev.find("headhunter") != std::string::npos;
		Set("BountyWord", BountyEvent || L.K == Kind::Envelope || C.V.count("Bounty") || Idea == 209 || Idea == 605 || Idea == 607 || Idea == 1403 || Idea == 1409 ? "BOUNTY" : "");
		Set("Payback", Idea == 1402 || ((L.Rival || L.K == Kind::Rival) && L.Rec.HeroNet > 0) ? "PAYBACK" : "");
	}
	if (Idea == 701 || Idea == 702 || Idea == 705 || Idea == 712 || Idea == 608 || Idea == 609 || Idea == 1201 || Idea == 1202)
	{
		Set("CountHands", Get("Count"));
	}
	// VS Split's words by what the two sides are; Top Five's "THE RUN" for a run.
	const bool Cool = L.K == Kind::Cooler || Idea == 105;
	const bool Rivals = L.Rival || L.K == Kind::Rival || G == 12 || Idea == 809 || Idea == 1402 || Idea == 1408;
	Set("VsWord", L.K == Kind::SetOverSet ? "SET vs SET" : Cool ? "COOLER" : Idea == 404 ? "LIVE vs ONLINE" : Idea == 1302 ? "SITE vs SITE" :
		Idea == 1306 ? "TWO MAJORS" : L.K == Kind::HeadsUp ? "HEADS-UP" : "");
	Set("Rivals", Rivals && !L.Storytime ? "RIVALS" : "");
	Set("CoolerWord", Cool || L.K == Kind::SetOverSet ? "COOLER" : "");
	Set("RunWord", video_detail::EventKind(L.K) || (G == 3 && Idea != 306) ? "THE RUN" : "");
	// The slab's words: an idea's name, a life story's line, over two lines of 14 at most (none when they won't fit).
	std::string Words;
	if (L.K == Kind::Story)
	{
		static const std::map<std::string, std::string> Lines = {{"rent-paid", "RENT PAID"}, {"seat-rcop-main", "I'M IN THE MAIN"},
			{"bracelet-first", "MY FIRST BRACELET"}, {"ring-first", "MY FIRST RING"}, {"mm-title", "CHAMPION"}, {"series-title", "CHAMPION"},
			{"back", "BACK HOME"}};
		const auto It = Lines.find(L.Event);
		Words = It != Lines.end() ? It->second : std::string();
	}
	else if (Idea >= 0 && G != 1)
	{
		if (const IdeaDef* D = IdeaById(Idea))
		{
			Words = UpperU3(D->Name);
		}
	}
	if (!Words.empty() && !L.Storytime)
	{
		if (Words.size() <= 14)
		{
			Set("Slab", Words);
		}
		else
		{
			std::string L1, L2, W;
			bool First = true, Fits = true;
			for (size_t I = 0; I <= Words.size(); ++I)
			{
				if (I < Words.size() && Words[I] != ' ')
				{
					W.push_back(Words[I]);
					continue;
				}
				if (W.empty())
				{
					continue;
				}
				if (First && L1.size() + (L1.empty() ? 0 : 1) + W.size() <= 14)
				{
					L1 += (L1.empty() ? "" : " ") + W;
				}
				else
				{
					First = false;
					L2 += (L2.empty() ? "" : " ") + W;
					Fits = Fits && L2.size() <= 14;
				}
				W.clear();
			}
			if (Fits && !L1.empty() && !L2.empty())
			{
				Set("Slab1", L1);
				Set("Slab2", L2);
			}
		}
	}
	return C;
}

/** Fills a caption row (both lines), or false when a fact is missing or a line runs past 14 characters. */
bool VtFill(const VtCap& Cap, const Facts& F, std::string& L1, std::string& L2)
{
	L1 = FillU3(Cap.Line1, F);
	L2 = Cap.Line2[0] ? FillU3(Cap.Line2, F) : std::string();
	if (L1.empty() || (Cap.Line2[0] && L2.empty()))
	{
		return false;
	}
	L1 = UpperU3(L1);
	L2 = UpperU3(L2);
	// (Captions keep their own case where a row writes one: "SET vs SET", "x25".)
	if (std::string(Cap.Line1).find("vs") != std::string::npos || std::string(Cap.Line1).find("{VsWord}") != std::string::npos || std::string(Cap.Line1).find("x{") == 0)
	{
		L1 = FillU3(Cap.Line1, F);
	}
	return L1.size() <= 14 && L2.size() <= 14;
}

/**
 * §6.1's hard "never"s, which no idea's own looks and no kind's general looks get past: the trophy only for what was won,
 * Gut Punch never on a won hand, the river circled only on a board with a river, no second face in storytime.
 */
bool VtNever(int Arch, const Candidate& L)
{
	using namespace thumb;
	const int Idea = L.Idea;
	switch (Arch)
	{
	case ArTrophyShot:
	{
		// (The same wins as the caption's "I WON": a first place, a title idea, a seat or a promotion won; a title story.)
		const bool Won = L.PlaceSoFar == 1 || Idea == 303 || Idea == 309 || Idea == 205 || Idea == 606 || Idea == 1603 || Idea == 1406 || Idea == 1408 || Idea == 1411 ||
			Idea == 1403 || (L.K == Kind::Story && L.Event != "rent-paid" && L.Event != "back");
		// (A news story's trophy is its own people's: the champion, the Hall of Fame class, the Season's race; the winner's
		// face holds it, never the player's: RecipeFor.)
		return !Won && Idea != 801 && Idea != 803 && Idea != 804;
	}
	case ArGutPunch: return L.Rec.HeroNet > 0;
	case ArCircledRiver: return L.Rec.Board.size() < 5;
	case ArVsSplit:
	case ArHeadhunter:
	case ArTheGhost:
	case ArVenue: return L.Storytime;
	default: break;
	}
	return false;
}

/** Whether the lead has the facts and the moment an archetype needs, and none of its "never"s. */
bool VtAllowed(int Arch, const Candidate& L, const Facts& F)
{
	using namespace thumb;
	if (VtNever(Arch, L))
	{
		return false;
	}
	const auto Has = [&F](const char* K) { return F.V.count(K) > 0; };
	const bool Hand = !video_detail::EventKind(L.K) && L.Rec.Mine.size() >= 2; // an idea built on a moment (group O) keeps its hand
	const bool Board5 = Hand && L.Rec.Board.size() >= 5;
	const bool Shown = Hand && !L.Rec.Shown.empty();
	const int Idea = L.Idea, G = Idea / 100;
	const bool Won = L.Rec.HeroNet > 0;
	// An idea's own looks (IdeaDef::Arch, three to an idea) are always open to it.
	if (const IdeaDef* D = Idea >= 0 ? IdeaById(Idea) : nullptr)
	{
		if (std::find(std::begin(D->Arch), std::end(D->Arch), Arch) != std::end(D->Arch))
		{
			return true;
		}
	}
	switch (Arch)
	{
	case ArGutPunch: // a hand lost when ahead (a caught bluff, a hero call or a fold has its own look)
		return Hand && !Won && L.Rec.HeroNet < 0 && L.K != Kind::BluffCaught && L.K != Kind::HeroCall && L.K != Kind::HeroFold && L.K != Kind::LadderFold && L.K != Kind::Bluff;
	case ArCircledRiver: // the river that decided it (a beat's turn too); POV and the squeeze circle the river itself
	{
		int Idx = -1;
		const int St = Board5 ? VtTurnStreet(L, Idx) : 0;
		const bool Decides = video_detail::GroupOf(L.K) == 0 || L.K == Kind::LastHand || L.K == Kind::HeadsUp || L.K == Kind::Bubble || L.K == Kind::Knockout || L.K == Kind::BiggestPot;
		return Board5 && ((Decides && (St == 5 || (St == 4 && video_detail::GroupOf(L.K) == 0))) || L.Idea == 1501 || L.Idea == 1507);
	}
	case ArTheRead: return Hand && (L.K == Kind::HeroCall || L.K == Kind::HeroFold || L.K == Kind::LadderFold || L.K == Kind::Bluff || L.K == Kind::BluffCaught || Idea == 601);
	case ArNervesOfSteel: return Hand && (L.K == Kind::Bluff) && video_detail::MadeLevel(L.Rec.Mine, L.Rec.Board) <= 1;
	case ArCaught: return Hand && L.K == Kind::BluffCaught;
	case ArTheFold: return Hand && (L.K == Kind::HeroFold || L.K == Kind::LadderFold || Idea == 612 || Idea == 603);
	case ArThePile: // the chips coming your way: a pot that was about the chips
		return Hand && Won && Has("Bb") && (L.K == Kind::BigPot || L.K == Kind::BiggestPot || L.K == Kind::Knockout || L.K == Kind::DoubleKo || L.K == Kind::Suckout || L.K == Kind::Monster || L.K == Kind::Envelope);
	case ArBigNumber: // a cash (an idea's own number is in its looks), a big pot
		return (Has("Prize") && L.K != Kind::Idea) || (Hand && Won && Has("Bb") && (L.K == Kind::BigPot || L.K == Kind::BiggestPot));
	case ArBeforeAfter: return (Has("Prize") && Has("BuyIn")) || Idea == 1406 || (G == 16 && Has("Start") && Has("Now"));
	case ArTrophyShot: return L.PlaceSoFar == 1 && (L.K == Kind::Win || L.K == Kind::HeadsUp || Idea == 303 || Idea == 309 || Idea == 1411);
	case ArVsSplit: return !L.Storytime && ((Hand && Shown && (L.K == Kind::Cooler || L.K == Kind::SetOverSet || L.K == Kind::Rival || L.K == Kind::HeadsUp)) || Idea == 809 || Idea == 1408 || Idea == 1402 || Idea == 404 || Idea == 1302 || Idea == 1306 || G == 12);
	case ArTheBubble: return L.K == Kind::Bubble || L.K == Kind::MinCash || L.Bubble || Idea == 306 || Idea == 1407 || Idea == 603;
	case ArHeadhunter: return !L.Storytime && (L.K == Kind::Knockout || L.K == Kind::DoubleKo || Idea == 1402 || Idea == 1403 || Idea == 1409 || Idea == 209 || Idea == 605);
	case ArEnvelope: return L.K == Kind::Envelope || Idea == 607;
	case ArTheGraph: return L.K == Kind::DeepRun || L.K == Kind::FinalTable || L.K == Kind::Win || Idea == 501 || Idea == 503 || Idea == 504 || Idea == 508 || Idea == 509 || Idea == 511 || Idea == 201 || Idea == 904 || Idea == 404;
	case ArDayCounter: return G == 16 || Idea == 204 || Idea == 806 || Idea == 1404 || Idea == 506;
	case ArNightShift: return ((L.K == Kind::DeepRun || L.K == Kind::FinalTable) && L.Source != 1) || (G == 2 && Idea != 205) || Idea == 1101 || Idea == 1102 || Idea == 406;
	case ArVenue: return !L.Storytime && (L.Source == 1 || (G == 4 && Idea != 405 && Idea != 404) || Idea == 709 || Idea == 1602 || Idea == 1507);
	case ArRoadTrip: return Idea == 407 || Idea == 401 || L.Source == 1;
	case ArRentLetter: return Idea == 505 || Idea == 506 || Idea == 1107 || Idea == 1108;
	case ArClassroom: return L.K == Kind::FinalTable || L.K == Kind::DeepRun || G == 6 || Idea == 304 || Idea == 404 || Idea == 508 || Idea == 1605 || Idea == 120 || L.K == Kind::LadderFold;
	case ArQuiz: return Idea == 608 || Idea == 609 || Idea == 109 || Idea == 110 || Idea == 1502 || Idea == 1503 || (Hand && (L.K == Kind::HeroCall || L.K == Kind::HeroFold));
	case ArTierList: return Idea == 708 || Idea == 709 || Idea == 710 || Idea == 711 || Idea == 713 || Idea == 614 || Idea == 803 || Idea == 808;
	case ArTopFive: return L.K == Kind::FinalTable || L.K == Kind::Win || L.K == Kind::DeepRun || Idea == 701 || Idea == 714 || Idea == 702 || Idea == 705 || Idea == 715 || Idea == 712 || Idea == 1409 || Idea == 304 || Idea == 511;
	case ArDeskSetup: return Idea == 1105 || Idea == 1106 || Idea == 711 || Idea == 1110 || Idea == 1104;
	case ArReactionBox: return G == 8 || Idea == 810;
	case ArStreamClip: return L.Stream.Viewers > 0 || L.K == Kind::StreamMoment || Idea == 907 || Idea == 908 || Idea == 312 || Idea == 1504 || Idea == 1505;
	case ArMilestone: return Idea == 901 || Idea == 903 || Idea == 904 || Idea == 715 || Idea == 810 || Idea == 902;
	case ArTextSlab: return L.Storytime || Idea == 405 || Idea == 1103 || Idea == 902 || Idea == 906 || Idea == 1104 || Idea == 1101 || L.K == Kind::Story;
	case ArTheGhost: return !L.Storytime && ((L.Rival && L.Opponent == RivalName) || G == 12);
	case ArPayday: return Idea == 512 || Idea == 1410 || Idea == 903 || Idea == 1304;
	case ArMonsterHand: return Hand && (L.K == Kind::Monster || Has("MadeHand"));
	case ArSiteCard: // a site's own night: its events, its bounties, its ideas (not every hand played there)
		return (L.Site != 0 && L.Site != static_cast<int>(net::SiteId::Halcyon) && !L.Storytime && (!Hand || L.K == Kind::Knockout || L.K == Kind::DoubleKo || L.K == Kind::Envelope)) || G == 13 || Idea == 1401 || Idea == 1404 || Idea == 1405 || Idea == 1410 || Idea == 805 || Idea == 807 || Idea == 310 || Idea == 613 || Idea == 710 || Idea == 1606;
	case ArMistake: return (Hand && (L.K == Kind::LastHand || L.K == Kind::BluffCaught)) || Idea == 602 || Idea == 712 || Idea == 1506;
	default: break;
	}
	return false; // Collab Duo and POV: P1
}

/** The general looks of a moment's kind, in order: what tops a deal up to four archetypes when its specific ones are fewer. */
std::vector<int> VtFallbacks(const Candidate& L)
{
	using namespace thumb;
	const bool Hand = !video_detail::EventKind(L.K) && L.Rec.Mine.size() >= 2;
	const bool Won = L.Rec.HeroNet > 0;
	const int Idea = L.Idea, G = Idea / 100;
	std::vector<int> Out;
	if (Hand)
	{
		// The hand's own look first (the pot or the beat), then the general ones in an order that turns
		// with its kind (no one of them carries every hand's deal).
		// (The river that turned it comes after the first general look: a board is a hand's look, but not every hand's.)
		static const int General[4] = {ArQuiz, ArTheRead, ArClassroom, ArBigNumber};
		const int Turn = static_cast<int>(L.K) % 4;
		int Idx = -1;
		const bool River = VtTurnStreet(L, Idx) == 5 && video_detail::GroupOf(L.K) != 1;
		Out = {Won ? ArThePile : ArGutPunch, L.Storytime ? ArTextSlab : -1, General[Turn], River ? ArCircledRiver : -1, General[(Turn + 1) % 4],
			General[(Turn + 2) % 4], General[(Turn + 3) % 4]};
	}
	else if (video_detail::EventKind(L.K))
	{
		Out = {ArBigNumber, ArBeforeAfter, ArTopFive, ArTheGraph, ArClassroom, L.Source == 1 ? ArVenue : ArNightShift};
	}
	else
	{
		if (const IdeaDef* D = Idea >= 0 ? IdeaById(Idea) : nullptr)
		{
			for (const int A : D->Arch)
			{
				Out.push_back(A);
			}
		}
		if (L.K == Kind::Story)
		{
			// The rent stories: the letter and the payday or the thank-you; the titles: the trophy and the thank-you.
			const bool Rent = L.Event == "rent-paid" || L.Event == "back";
			Out.push_back(Rent ? ArRentLetter : ArTrophyShot);
			Out.push_back(L.Event == "rent-paid" ? ArPayday : ArMilestone);
		}
		Out.push_back(G == 2 || G == 5 ? ArTheGraph : G == 6 ? ArClassroom : G == 7 ? ArTopFive : G == 16 ? ArDayCounter : G == 9 ? ArMilestone : -1);
		// The three general looks, in an order that turns with the idea (no one of them carries every idea's deal).
		static const int General[3] = {ArTextSlab, ArBigNumber, ArReactionBox};
		const int Turn = Idea >= 0 ? Idea % 3 : 0;
		for (int I = 0; I < 3; ++I)
		{
			Out.push_back(General[(Turn + I) % 3]);
		}
	}
	return Out;
}

/** How well an archetype fits this moment (§4.6.2): its best fit only at its moment, a neutral 1.00, a poor 0.70-0.80. */

double VtFit(int Arch, int Group, const Candidate& L)
{
	using namespace thumb;
	const int Idea = L.Idea, G = Idea / 100;
	const bool Lost = L.Rec.HeroNet < 0;
	switch (Arch)
	{
	case ArGutPunch: return Group == 0 ? (Lost && L.EquityAtAllIn >= 0.75 ? 1.30 : Lost ? 1.12 : 0.75) : L.K == Kind::LastHand ? 1.20 : 0.90;
	case ArCircledRiver:
	{
		int I = -1;
		const int Street = VtTurnStreet(L, I);
		return Group == 0 || L.K == Kind::RunnerRunner ? (Street == 5 ? 1.35 : 0.95) : Street == 5 ? 1.10 : 0.85;
	}
	case ArTheRead: return L.K == Kind::HeroCall || L.K == Kind::HeroFold ? 1.30 : Group == 1 ? 1.05 : 0.85;
	case ArNervesOfSteel: return L.K == Kind::Bluff ? 1.35 : 0.80;
	case ArCaught: return L.K == Kind::BluffCaught ? 1.25 : 0.80;
	case ArTheFold: return L.K == Kind::HeroFold || L.K == Kind::LadderFold || Idea == 612 ? 1.35 : Idea == 603 ? 1.15 : 0.85;
	case ArThePile: return L.K == Kind::BigPot || L.K == Kind::BiggestPot ? 1.25 : Group == 2 ? 1.05 : 0.90;
	case ArBigNumber: return Group == 3 || L.K == Kind::BigPot || L.K == Kind::BiggestPot || Idea == 301 || Idea == 305 || Idea == 303 ? 1.30 : 1.00;
	case ArBeforeAfter: return Group == 3 || Idea == 1406 || G == 16 || Idea == 205 || Idea == 206 ? 1.30 : 0.95;
	case ArTrophyShot: return L.PlaceSoFar == 1 ? 1.40 : 0.75;
	case ArVsSplit: return L.K == Kind::Rival || Idea == 809 || Idea == 1408 ? 1.40 : L.K == Kind::Cooler || L.K == Kind::SetOverSet ? 1.30 : 1.00;
	case ArTheBubble: return L.K == Kind::Bubble || L.K == Kind::MinCash || Idea == 306 || Idea == 1407 ? 1.35 : 1.00;
	case ArHeadhunter: return L.K == Kind::Knockout || L.K == Kind::DoubleKo || Idea == 1402 || Idea == 1403 || Idea == 1409 ? 1.30 : 1.00;
	case ArEnvelope: return L.K == Kind::Envelope ? 1.40 : 1.05;
	case ArTheGraph: return Idea == 501 || Idea == 503 || Idea == 504 ? 1.30 : 1.05;
	case ArDayCounter: return G == 16 || Idea == 204 ? 1.35 : 1.00;
	case ArNightShift: return G == 2 || Idea == 1102 ? 1.25 : 1.00;
	case ArVenue: return L.Source == 1 || G == 4 ? 1.30 : 0.95;
	case ArRoadTrip: return Idea == 407 ? 1.20 : 0.95;
	case ArRentLetter: return Idea == 505 || Idea == 506 || Idea == 1107 || Idea == 1108 ? 1.35 : 0.90;
	case ArClassroom: return G == 6 ? 1.30 : (L.K == Kind::LadderFold || L.K == Kind::Bubble || L.K == Kind::MinCash || L.K == Kind::FinalTable || Idea == 304 || Idea == 404 || Idea == 508 || Idea == 1605) ? 1.05 : 0.90;
	case ArQuiz: return Idea == 608 || Idea == 1502 || Idea == 1503 ? 1.30 : L.K == Kind::HeroCall || L.K == Kind::HeroFold || Idea == 609 ? 1.10 : 0.90;
	case ArTierList: return Idea == 708 || Idea == 709 || Idea == 710 || Idea == 711 || Idea == 713 ? 1.35 : 1.00;
	case ArTopFive: return Idea == 701 || Idea == 702 || Idea == 705 || Idea == 715 ? 1.30 : 1.00;
	case ArDeskSetup: return Idea == 1105 || Idea == 1106 || Idea == 711 ? 1.30 : 1.00;
	case ArReactionBox: return G == 8 ? 1.30 : Idea == 810 ? 1.10 : 0.90;
	case ArStreamClip: return L.K == Kind::StreamMoment || Idea == 118 || Idea == 312 || Idea == 1504 || Idea == 1505 ? 1.25 : L.Stream.Viewers > 0 ? 1.10 : 0.95;
	case ArMilestone: return Idea == 901 || Idea == 903 || Idea == 904 ? 1.30 : 1.00;
	case ArTextSlab: return L.Storytime || Idea == 405 || Idea == 1103 ? 1.20 : L.K == Kind::Story || Idea == 902 || Idea == 906 ? 1.05 : 0.95;
	case ArTheGhost: return (L.Rival && L.Opponent == RivalName) || G == 12 ? 1.40 : 0.80;
	case ArPayday: return Idea == 512 || Idea == 1410 ? 1.30 : 1.00;
	case ArMonsterHand: return L.K == Kind::Monster ? 1.40 : 1.10;
	case ArSiteCard: return G == 13 || G == 14 || Idea == 805 || Idea == 807 ? 1.25 : 1.00;
	case ArMistake: return Idea == 602 || L.K == Kind::LastHand ? 1.35 : 1.00;
	default: break;
	}
	return 1.0;
}

/** The archetype's palette the hash lands on, never the last upload's own pair (unless the set has nothing else). */
int VtPick(const std::vector<int>& Set, uint32_t H)
{
	return Set.empty() ? -1 : Set[H % Set.size()];
}

uint32_t VtMix(uint32_t H, uint32_t Salt)
{
	H ^= Salt * 0x9e3779b9u;
	H ^= H >> 16;
	H *= 0x85ebca6bu;
	H ^= H >> 13;
	H *= 0xc2b2ae35u;
	H ^= H >> 16;
	return H;
}

std::vector<int> VtPals(uint32_t Mask)
{
	std::vector<int> Out;
	for (int P = 0; P < thumb::PlCount; ++P)
	{
		if ((Mask >> P) & 1)
		{
			Out.push_back(P);
		}
	}
	return Out;
}

/** The archetype's fillable caption rows on this lead. */
std::vector<std::pair<std::string, std::string>> VtCaptions(const VtArch& A, const Facts& F)
{
	std::vector<std::pair<std::string, std::string>> Out;
	for (const VtCap& Cap : A.Captions)
	{
		std::string L1, L2;
		if (VtFill(Cap, F, L1, L2))
		{
			Out.push_back({L1, L2});
		}
	}
	return Out;
}

/** The last upload's shown (archetype, palette), -1 when it had no recipe. */
void VtLastPair(const Channel& C, int& Arch, int& Pal)
{
	Arch = -1;
	Pal = -1;
	const Video* Last = nullptr;
	for (const Video& V : C.Videos)
	{
		Last = !Last || V.At > Last->At ? &V : Last;
	}
	if (Last && Last->X.On && Last->Fr == Frame::Recipe)
	{
		const ThumbSpec& S = Last->X.Spec[static_cast<size_t>(std::clamp<int>(Last->X.DuelWin, 0, 2))];
		Arch = S.Arch;
		Pal = S.Pal;
	}
}
} // namespace

std::string FactBlock(const std::vector<std::pair<const char*, std::string>>& KV)
{
	std::string Out;
	for (const auto& Kv : KV)
	{
		if (!Kv.second.empty())
		{
			Out += thumb::FactSep;
			Out += std::string(Kv.first) + "=" + Kv.second;
		}
	}
	return Out;
}

std::vector<int> AllowedArchs(const Candidate& Lead, const Facts& F)
{
	using namespace thumb;
	std::vector<int> Out;
	const Facts Cf = VtCaptionFacts(F, Lead);
	const auto Drawable = [&Cf](int A) { return A != ArPov && A != ArCollabDuo && !VtCaptions(VtTable()[static_cast<size_t>(A)], Cf).empty(); };
	for (int A = 0; A < ArCount; ++A)
	{
		if (VtAllowed(A, Lead, F) && Drawable(A))
		{
			Out.push_back(A);
		}
	}
	// A deal shows at least four archetypes (§6.1): a moment the specific looks don't cover draws its kind's general ones, in
	// order, each still within its captions' facts (they fit it less: VtFit).
	for (const int A : VtFallbacks(Lead))
	{
		if (Out.size() >= 4)
		{
			break;
		}
		if (A >= 0 && std::find(Out.begin(), Out.end(), A) == Out.end() && !VtNever(A, Lead) && Drawable(A))
		{
			Out.push_back(A);
		}
	}
	std::sort(Out.begin(), Out.end());
	return Out;
}

ThumbSpec RecipeFor(int Arch, const Candidate& Lead, const std::vector<Candidate>& Picks, const Facts& F, uint32_t H, int LastArch, int LastPal)
{
	using namespace thumb;
	(void)Picks;
	const VtArch& A = VtTable()[static_cast<size_t>(std::clamp(Arch, 0, static_cast<int>(ArCount) - 1))];
	const Facts Cf = VtCaptionFacts(F, Lead);
	ThumbSpec S;
	S.Arch = static_cast<int16_t>(Arch);
	S.Layout = static_cast<int16_t>(VtPick(A.Layouts, VtMix(H, 1)));
	S.Mirror = static_cast<int16_t>(VtMix(H, 2) % 4);
	S.Bg = static_cast<int16_t>(VtPick(A.Bgs, VtMix(H, 3)));
	std::vector<int> Pals = VtPals(A.Pals);
	// Never the last upload's (archetype, palette) pair (§6.1): another palette when the archetype is the same.
	if (Arch == LastArch && Pals.size() > 1)
	{
		Pals.erase(std::remove(Pals.begin(), Pals.end(), LastPal), Pals.end());
	}
	// A site's colours only when the moment has a site that isn't RiverLine (the s fact).
	if (Lead.Site == 0 && Pals.size() > 1)
	{
		Pals.erase(std::remove(Pals.begin(), Pals.end(), static_cast<int>(PlSite)), Pals.end());
	}
	S.Pal = static_cast<int16_t>(VtPick(Pals, VtMix(H, 4)));
	S.Subj = static_cast<int16_t>(Lead.Storytime && A.Subj == SjOpponent ? SjYou : A.Subj);
	if (Arch == ArTrophyShot && Lead.Idea / 100 == 8)
	{
		S.Subj = SjOpponent; // a news story's trophy: its champion holds it (the o fact), never the player
	}
	std::vector<int> Exprs = A.Exprs;
	if (Lead.Stream.Viewers > 0 && std::find(Exprs.begin(), Exprs.end(), VtMoodExpr(Lead.Stream.Face)) != Exprs.end())
	{
		Exprs = {VtMoodExpr(Lead.Stream.Face)}; // the face the stream showed
	}
	S.Expr = static_cast<int16_t>(VtPick(Exprs, VtMix(H, 5)));
	S.Pose = static_cast<int16_t>(VtPick(A.Poses, VtMix(H, 6)));
	S.PropA = static_cast<int16_t>(VtPick(A.PropsA, VtMix(H, 7)));
	S.PropB = static_cast<int16_t>(VtPick(A.PropsB, VtMix(H, 8)));
	S.CapStyle = static_cast<int16_t>(VtPick(A.Caps, VtMix(H, 9)));
	S.Sticker = static_cast<int16_t>(VtPick(A.Stickers, VtMix(H, 10)));
	S.Corner = static_cast<int16_t>(VtMix(H, 11) % CnCount);
	S.LookRef = -1;
	S.Variant = VtMix(H, 12) % 1000;
	const std::vector<std::pair<std::string, std::string>> Caps = VtCaptions(A, Cf);
	if (!Caps.empty())
	{
		const auto& Cap = Caps[VtMix(H, 13) % Caps.size()];
		S.Caption = Cap.first;
		S.Caption2 = Cap.second;
	}
	S.Caption2 += VtFactBlock(Arch, Lead, F, S.Pal, S.Subj);
	return S;
}

namespace
{
/** What an archetype allows on a lead beyond its fixed lists (Studio's pickers, §6.6.2): its palettes (a site's colours only
 *  with a site), its subject, its faces (a stream's own face), its caption rows filled from the lead's facts. RecipeFor deals
 *  from exactly these (its "not the last upload's pair" rule only narrows a deal, never the player's own pick). */
struct VtAllow
{
	std::vector<int> Pals, Exprs;
	int Subj = 0;
	std::vector<std::pair<std::string, std::string>> Caps;
};

VtAllow VtAllowedOn(int Arch, const Candidate& Lead, const Facts& F)
{
	using namespace thumb;
	const VtArch& A = VtTable()[static_cast<size_t>(std::clamp(Arch, 0, static_cast<int>(ArCount) - 1))];
	VtAllow Out;
	Out.Pals = VtPals(A.Pals);
	if (Lead.Site == 0 && Out.Pals.size() > 1)
	{
		Out.Pals.erase(std::remove(Out.Pals.begin(), Out.Pals.end(), static_cast<int>(PlSite)), Out.Pals.end());
	}
	Out.Subj = Lead.Storytime && A.Subj == SjOpponent ? SjYou : A.Subj;
	if (Arch == ArTrophyShot && Lead.Idea / 100 == 8)
	{
		Out.Subj = SjOpponent;
	}
	Out.Exprs = A.Exprs;
	if (Lead.Stream.Viewers > 0 && std::find(Out.Exprs.begin(), Out.Exprs.end(), VtMoodExpr(Lead.Stream.Face)) != Out.Exprs.end())
	{
		Out.Exprs = {VtMoodExpr(Lead.Stream.Face)};
	}
	Out.Caps = VtCaptions(A, VtCaptionFacts(F, Lead));
	return Out;
}

template <typename T>
bool VtIn(const std::vector<T>& List, int V)
{
	return std::find(List.begin(), List.end(), static_cast<T>(V)) != List.end();
}

/** The choice's lead and facts as ThumbOptions reads them; false when it picks nothing (or is a Cut). */
bool VtLeadOf(const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, std::vector<Candidate>& P, Candidate& Lead, Facts& F)
{
	P = Picked(Ch, Cands);
	if (P.empty() || Ch.F == Format::Short)
	{
		return false;
	}
	Lead = LeadOf(Ch, P);
	F = FactsOf(Ch, P, Lead, &C);
	return true;
}

/** The lead and facts recipe S was dealt on: the choice's own or, when the choice names a series S was dealt without (Studio
 *  deals before a series is picked; PUBLISH carries it), the choice without the series. False when it picks nothing. */
bool VtBasisOf(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, std::vector<Candidate>& P, Candidate& Lead, Facts& F)
{
	if (!VtLeadOf(Ch, Cands, C, P, Lead, F))
	{
		return false;
	}
	if (Ch.Series >= 0 && !ThumbWithinOn(S, Lead, F))
	{
		Choice Bare = Ch;
		Bare.Series = -1;
		std::vector<Candidate> P2;
		Candidate L2;
		Facts F2;
		if (VtLeadOf(Bare, Cands, C, P2, L2, F2) && ThumbWithinOn(S, L2, F2))
		{
			P = P2;
			Lead = L2;
			F = F2;
		}
	}
	return true;
}
} // namespace

std::string VtFactBlock(int Arch, const Candidate& Lead, const Facts& F, int Pal, int Subj)
{
	using namespace thumb;
	const VtArch& A = VtTable()[static_cast<size_t>(std::clamp(Arch, 0, static_cast<int>(ArCount) - 1))];
	// The compositor's facts: only what this archetype draws, and only what the moment has.
	std::vector<std::pair<const char*, std::string>> KV;
	const bool Hand = !video_detail::EventKind(Lead.K) && Lead.Rec.Mine.size() >= 2;
	if (Hand)
	{
		std::string Theirs;
		for (const auto& Sh : Lead.Rec.Shown)
		{
			Theirs = Sh.second.size() >= 2 && (Theirs.empty() || Sh.first == Lead.Opponent) ? VtCards(Sh.second) : Theirs;
		}
		const bool NeedsCards = A.Fr == Frame::Cards || Arch == ArGutPunch || Arch == ArVsSplit || Arch == ArNervesOfSteel || Arch == ArCaught || Arch == ArClassroom || Arch == ArQuiz;
		if (NeedsCards)
		{
			KV.push_back({"c", VtCards(Lead.Rec.Mine) + "/" + Theirs + "/" + VtCards(Lead.Rec.Board)});
		}
		int Idx = -1;
		if (Arch == ArCircledRiver && VtTurnStreet(Lead, Idx) >= 4)
		{
			KV.push_back({"k", std::to_string(Idx)});
		}
	}
	const auto Get = [&F](const char* K) { return F.V.count(K) ? F.V.at(K) : std::string(); };
	if ((Subj == SjOpponent || Subj == SjWorldPlayer) && !Lead.Storytime)
	{
		const std::string Other = !Lead.Opponent.empty() ? Lead.Opponent : !Get("Winner").empty() ? Get("Winner") : !Get("Npc").empty() ? Get("Npc") : !Get("Beef").empty() ? Get("Beef") :
			Arch == ArTrophyShot ? Get("Leader") : std::string();
		KV.push_back({"o", Other});
	}
	if (Arch == ArBeforeAfter)
	{
		KV.push_back({"f", Get("DivisionWas").empty() ? (Get("Start").empty() ? Get("BuyIn") : Get("Start")) : UpperU3(Get("DivisionWas"))});
		KV.push_back({"t", Get("Division").empty() ? (Get("Now").empty() ? Get("Prize") : Get("Now")) : UpperU3(Get("Division"))});
	}
	if (Arch == ArDayCounter && !Get("Start").empty())
	{
		// The bar from start to goal, on a log scale of the roll (a challenge's) or the count (a hundred of something).
		const std::string Now = Get("Now"), Goal = Get("Goal");
		const auto Num = [](std::string T) {
			T.erase(std::remove_if(T.begin(), T.end(), [](char Ch) { return !(Ch >= '0' && Ch <= '9') && Ch != '.'; }), T.end());
			return T.empty() ? 0.0 : std::atof(T.c_str());
		};
		const double Sv = Num(Get("Start")), Nv = Num(Now), Gv = Num(Goal);
		if (Sv > 0.0 && Gv > Sv && Nv > 0.0)
		{
			KV.push_back({"p", std::to_string(std::clamp(static_cast<int>(std::lround(100.0 * std::log(Nv / Sv) / std::log(Gv / Sv))), 0, 100))});
		}
	}
	if (Arch == ArTheGraph)
	{
		KV.push_back({"g", F.Lost ? "d" : "u"});
	}
	if (Lead.Site != 0 && (Pal == PlSite || Arch == ArSiteCard || Arch == ArDayCounter))
	{
		KV.push_back({"s", std::to_string(Lead.Site)});
	}
	if (Arch == ArTrophyShot || Arch == ArVenue || Arch == ArReactionBox)
	{
		KV.push_back({"e", Get("Major").empty() ? Lead.Event : Get("Major")});
	}
	if (Arch == ArEnvelope || Arch == ArSiteCard)
	{
		KV.push_back({"m", Get("Mult")});
	}
	if (Arch == ArDeskSetup)
	{
		// The desk's gear: the ids the player owns, from the setup idea's own facts ("desk", VideoIdeas.cpp); none, the bare desk.
		size_t At = 0;
		while (At < Lead.IdeaFacts.size())
		{
			const size_t End = std::min(Lead.IdeaFacts.find(';', At), Lead.IdeaFacts.size());
			if (Lead.IdeaFacts.compare(At, 5, "desk=") == 0)
			{
				KV.push_back({"d", FactUnesc(Lead.IdeaFacts.substr(At + 5, End - At - 5))});
			}
			At = End + 1;
		}
	}
	if (Arch == ArStreamClip)
	{
		for (size_t I = 0; I < Lead.Stream.Chat.size() && I < 2; ++I)
		{
			std::string Line = Lead.Stream.Chat[I];
			Line.erase(std::remove(Line.begin(), Line.end(), thumb::FactSep), Line.end());
			KV.push_back({"h", Line});
		}
	}
	return FactBlock(KV);
}
} // namespace vcontent

// ================================================================== the public face

std::vector<ThumbSpec> ThumbOptions(const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Shuffle, int Count)
{
	using namespace vcontent;
	std::vector<ThumbSpec> Out;
	const std::vector<Candidate> P = Picked(Ch, Cands);
	if (P.empty() || Count <= 0 || Ch.F == Format::Short)
	{
		return Out; // a Cut gets no thumbnail options in P0 (§5.4)
	}
	const Candidate& Lead = LeadOf(Ch, P);
	const Facts F = FactsOf(Ch, P, Lead, &C);
	const std::vector<int> Allowed = AllowedArchs(Lead, F);
	if (Allowed.empty())
	{
		return Out;
	}
	const int Group = video_detail::GroupOf(Lead.K);
	int Best = Allowed.front();
	for (const int A : Allowed)
	{
		Best = ArchetypeFit(A, Group, Lead) > ArchetypeFit(Best, Group, Lead) ? A : Best;
	}
	std::string Key = std::to_string(static_cast<int>(Ch.F));
	for (const std::string& Id : Ch.Candidates)
	{
		Key += "|" + Id;
	}
	// The deal: the allowed archetypes in hash order (never by fit), the best one at most once; past the distinct archetypes,
	// a second recipe of the others (another palette, face and caption).
	std::vector<std::pair<uint32_t, int>> Order;
	for (const int A : Allowed)
	{
		Order.push_back({VtMix(Fnv1a(Key + ":thumb:" + std::to_string(Shuffle) + ":a" + std::to_string(A)), 0), A}); // mixed: FNV-1a's last bytes barely move it
	}
	std::sort(Order.begin(), Order.end(), [](const std::pair<uint32_t, int>& X, const std::pair<uint32_t, int>& Y) { return X.first != Y.first ? X.first > Y.first : X.second < Y.second; });
	std::vector<int> Slots;
	for (const auto& O : Order)
	{
		if (static_cast<int>(Slots.size()) < Count)
		{
			Slots.push_back(O.second);
		}
	}
	// One second recipe of each of the others at most (another palette, face and caption), never the best fit's.
	const int Fill = Count; // a deal of six (§6.1): at least four archetypes, the rest second recipes of the others
	for (size_t Round = 0; static_cast<int>(Slots.size()) < Fill && Round < 1; ++Round)
	{
		for (const auto& O : Order)
		{
			if (O.second != Best && static_cast<int>(Slots.size()) < Fill)
			{
				Slots.push_back(O.second);
			}
		}
		if (Order.size() == 1)
		{
			break;
		}
	}
	int LastArch = -1, LastPal = -1;
	VtLastPair(C, LastArch, LastPal);
	std::set<std::pair<int, int>> Pairs;
	for (size_t I = 0; I < Slots.size(); ++I)
	{
		const uint32_t H = VtMix(Fnv1a(Key + ":thumb:" + std::to_string(Shuffle) + ":" + std::to_string(I)), 0);
		ThumbSpec S = RecipeFor(Slots[I], Lead, P, F, H, LastArch, LastPal);
		// A second recipe of the same archetype in one deal takes another palette.
		for (int Try = 1; Try < 8 && !Pairs.insert({S.Arch, S.Pal}).second; ++Try)
		{
			S = RecipeFor(Slots[I], Lead, P, F, VtMix(H, 100 + static_cast<uint32_t>(Try)), LastArch, LastPal);
		}
		Out.push_back(S);
	}
	return Out;
}

double ArchetypeFit(int Arch, int KindGroup, const Candidate& Lead)
{
	if (Arch < 0 || Arch >= thumb::ArCount)
	{
		return 1.0;
	}
	return std::clamp(vcontent::VtFit(Arch, KindGroup, Lead), 0.70, 1.40);
}

Frame FrameOf(int Arch)
{
	if (Arch < 0 || Arch >= thumb::ArCount)
	{
		return Frame::Reaction;
	}
	return vcontent::VtTable()[static_cast<size_t>(Arch)].Fr;
}

double TraitMean(int KindGroup)
{
	// The generator's expected trait product per kind group (beats .. stories, the ideas in 5), measured by rerun_content
	// --write over the library's fixture moments and every deal of 40 shuffles; pinned so Shuffle never changes a CTR.
	static const double Mean[6] = {0.9879, 1.0318, 1.0103, 1.0114, 0.9814, 1.0654};
	return Mean[std::clamp(KindGroup, 0, 5)];
}

double ThumbTraits(const ThumbSpec& S, const Candidate& Lead, const Channel& C)
{
	using namespace thumb;
	if (S.Arch < 0)
	{
		return 1.0;
	}
	const int Group = video_detail::GroupOf(Lead.K);
	double T = 1.0;
	// A face whose expression matches the moment's emotion +8%, a mismatched one -4% (no face: neither).
	if (S.Subj == SjYou || S.Subj == SjOpponent || S.Subj == SjWorldPlayer || S.Subj == SjCreator)
	{
		T *= vcontent::VtEmotionMatch(Group, S.Expr, Lead) ? 1.08 : 0.96;
	}
	// A caption of three words or fewer +5%; five or more -8%.
	const std::string Text = S.Caption + " " + ThumbCaption2(S);
	int Words = 0;
	bool In = false;
	for (const char Ch : Text)
	{
		const bool W = Ch != ' ';
		Words += W && !In ? 1 : 0;
		In = W;
	}
	T *= Words <= 3 ? 1.05 : Words >= 5 ? 0.92 : 1.0;
	// A big number on a money fact +6%.
	const bool Money = Text.find('$') != std::string::npos || Text.find(" BB") != std::string::npos || Text.find('%') != std::string::npos;
	if (Money && (S.CapStyle == CsNumberBurst || S.Arch == ArBigNumber || S.Arch == ArThePile))
	{
		T *= 1.06;
	}
	// Contrast against your last three thumbnails +4% (a palette none of them used).
	std::vector<const Video*> Recent;
	for (const Video& V : C.Videos)
	{
		if (V.X.On && V.Fr == Frame::Recipe)
		{
			Recent.push_back(&V);
		}
	}
	std::sort(Recent.begin(), Recent.end(), [](const Video* A, const Video* B) { return A->At > B->At; });
	bool Fresh = Recent.size() >= 3;
	for (size_t I = 0; I < Recent.size() && I < 3; ++I)
	{
		Fresh = Fresh && Recent[I]->X.Spec[static_cast<size_t>(std::clamp<int>(Recent[I]->X.DuelWin, 0, 2))].Pal != S.Pal;
	}
	T *= Fresh ? 1.04 : 1.0;
	// Clutter: more than four elements -10%.
	const int Elements = (S.Subj != SjNobody ? 1 : 0) + (S.PropA >= 0 ? 1 : 0) + (S.PropB >= 0 ? 1 : 0) + (S.Sticker >= 0 ? 1 : 0) + (!S.Caption.empty() ? 1 : 0) +
		(!ThumbCaption2(S).empty() ? 1 : 0) + (S.Corner != CnNone ? 1 : 0);
	T *= Elements > 4 ? 0.90 : 1.0;
	// Legible cards on a read +3%.
	if (Group == 1 && (S.PropA == PrHoleCards || S.PropB == PrHoleCards || S.PropA == PrCardFan))
	{
		T *= 1.03;
	}
	return T / TraitMean(Group);
}

uint8_t ThumbPromise(const ThumbSpec& S)
{
	using namespace thumb;
	if (S.Arch < 0 || S.Arch >= ArCount)
	{
		return 0;
	}
	int P = vcontent::VtTable()[static_cast<size_t>(S.Arch)].Promise;
	P += S.Expr == ExScream ? 1 : 0;
	P += S.Sticker == StSiren || S.Sticker == StSkull || S.Sticker == StBolt || S.Sticker == StShockedFace || S.Sticker == StCryingFace ? 1 : 0;
	const std::string Up = vcontent::UpperU3(S.Caption + " " + ThumbCaption2(S));
	for (const char* W : {"!!", "WHY", "INSANE", "WORST", "BIGGEST", "NEVER", "CRACKED"})
	{
		if (Up.find(W) != std::string::npos)
		{
			++P;
			break;
		}
	}
	return static_cast<uint8_t>(std::clamp(P, 0, 3));
}

uint64_t ThumbSignature(const ThumbSpec& S)
{
	if (S.Arch < 0)
	{
		return 0;
	}
	uint64_t H = 1469598103934665603ull;
	for (const int V : {static_cast<int>(S.Arch), static_cast<int>(S.Layout), static_cast<int>(S.Mirror), static_cast<int>(S.Bg), static_cast<int>(S.Pal), static_cast<int>(S.Expr),
			 static_cast<int>(S.Pose), static_cast<int>(S.CapStyle), static_cast<int>(S.Sticker)})
	{
		H ^= static_cast<uint64_t>(static_cast<uint32_t>(V + 1));
		H *= 1099511628211ull;
	}
	return H;
}

std::string ThumbFact(const ThumbSpec& S, const char* Key)
{
	const std::vector<std::string> All = ThumbFacts(S, Key);
	return All.empty() ? std::string() : All.front();
}

std::vector<std::string> ThumbFacts(const ThumbSpec& S, const char* Key)
{
	std::vector<std::string> Out;
	const std::string K = std::string(Key) + "=";
	size_t At = S.Caption2.find(thumb::FactSep);
	while (At != std::string::npos)
	{
		const size_t Next = S.Caption2.find(thumb::FactSep, At + 1);
		const std::string Part = S.Caption2.substr(At + 1, Next == std::string::npos ? std::string::npos : Next - At - 1);
		if (Part.compare(0, K.size(), K) == 0)
		{
			Out.push_back(Part.substr(K.size()));
		}
		At = Next;
	}
	return Out;
}

std::string ThumbCaption2(const ThumbSpec& S)
{
	const size_t At = S.Caption2.find(thumb::FactSep);
	return At == std::string::npos ? S.Caption2 : S.Caption2.substr(0, At);
}

const char* ArchName(int Arch)
{
	return Arch >= 0 && Arch < thumb::ArCount ? vcontent::VtTable()[static_cast<size_t>(Arch)].Name : "";
}

std::string ArchRead(int Arch)
{
	return Arch >= 0 && Arch < thumb::ArCount ? std::string(vcontent::VtTable()[static_cast<size_t>(Arch)].Read) : std::string();
}

std::vector<ThumbSpec> ContactSheet(int Group, int Count)
{
	using namespace vcontent;
	std::vector<ThumbSpec> Out;
	std::vector<Candidate> Moments;
	for (const Candidate& M : FixtureMoments())
	{
		const bool Idea = M.K == Kind::Idea;
		if (Group == 6 ? Idea : (!Idea && video_detail::GroupOf(M.K) == Group))
		{
			Moments.push_back(M);
		}
	}
	if (Moments.empty())
	{
		return Out;
	}
	std::set<std::pair<int, int>> Seen;
	Channel C;
	C.Open = Moments;
	for (int Shuffle = 0; static_cast<int>(Out.size()) < Count && Shuffle < 200; ++Shuffle)
	{
		const Candidate& M = Moments[static_cast<size_t>(Shuffle) % Moments.size()];
		Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = Format::Breakdown;
		for (const ThumbSpec& S : ThumbOptions(Ch, C.Open, C, Shuffle, 6))
		{
			if (static_cast<int>(Out.size()) < Count && Seen.insert({S.Arch, S.Pal}).second)
			{
				Out.push_back(S);
			}
		}
	}
	return Out;
}

ThumbChoices ThumbChoicesOf(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C)
{
	using namespace vcontent;
	ThumbChoices Out;
	std::vector<Candidate> P;
	Candidate Lead;
	Facts F;
	if (S.Arch < 0 || S.Arch >= thumb::ArCount || !VtBasisOf(S, Ch, Cands, C, P, Lead, F))
	{
		return Out;
	}
	const VtArch& A = VtTable()[static_cast<size_t>(S.Arch)];
	const VtAllow Al = VtAllowedOn(S.Arch, Lead, F);
	for (const int V : Al.Pals)
	{
		Out.Pals.push_back(static_cast<int16_t>(V));
	}
	// (Every P0 archetype shows a face of yours; one that didn't, nobody or an object, would have none to pick.)
	if (Al.Subj != thumb::SjNobody && Al.Subj != thumb::SjObject)
	{
		for (const int V : Al.Exprs)
		{
			Out.Exprs.push_back(static_cast<int16_t>(V));
		}
	}
	for (const int V : A.Caps)
	{
		Out.CapStyles.push_back(static_cast<int16_t>(V));
	}
	Out.Captions = Al.Caps;
	return Out;
}

ThumbSpec ThumbEdit(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Field, int Value)
{
	using namespace vcontent;
	std::vector<Candidate> P;
	Candidate Lead;
	Facts F;
	if (S.Arch < 0 || S.Arch >= thumb::ArCount || !VtBasisOf(S, Ch, Cands, C, P, Lead, F))
	{
		return S;
	}
	const ThumbChoices Ok = ThumbChoicesOf(S, Ch, Cands, C); // (on the same basis: VtBasisOf of the same recipe)
	ThumbSpec Out = S;
	std::string Line2 = ThumbCaption2(S);
	switch (Field)
	{
	case TfPalette:
		if (!VtIn(Ok.Pals, Value))
		{
			return S;
		}
		Out.Pal = static_cast<int16_t>(Value);
		break;
	case TfFace:
		if (!VtIn(Ok.Exprs, Value))
		{
			return S;
		}
		Out.Expr = static_cast<int16_t>(Value);
		break;
	case TfCapStyle:
		if (!VtIn(Ok.CapStyles, Value))
		{
			return S;
		}
		Out.CapStyle = static_cast<int16_t>(Value);
		break;
	case TfCaption:
		if (Value < 0 || Value >= static_cast<int>(Ok.Captions.size()))
		{
			return S;
		}
		Out.Caption = Ok.Captions[static_cast<size_t>(Value)].first;
		Line2 = Ok.Captions[static_cast<size_t>(Value)].second;
		break;
	default: return S;
	}
	// The compositor's facts as a dealt recipe of this archetype and palette carries them.
	Out.Caption2 = Line2 + VtFactBlock(Out.Arch, Lead, F, Out.Pal, Out.Subj);
	return Out;
}

bool ThumbWithin(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C)
{
	using namespace vcontent;
	using namespace thumb;
	std::vector<Candidate> P;
	Candidate Lead;
	Facts F;
	// (Studio deals the recipes before a series is picked for the upload: a recipe within the moment's values without the
	// series' episode facts is as good as one with them: VtBasisOf.)
	return S.Arch >= 0 && S.Arch < ArCount && VtBasisOf(S, Ch, Cands, C, P, Lead, F) && ThumbWithinOn(S, Lead, F);
}

namespace vcontent
{
bool ThumbWithinOn(const ThumbSpec& S, const Candidate& Lead, const Facts& F)
{
	using namespace thumb;
	if (S.Arch < 0 || S.Arch >= ArCount)
	{
		return false;
	}
	const std::vector<int> Archs = AllowedArchs(Lead, F);
	if (std::find(Archs.begin(), Archs.end(), static_cast<int>(S.Arch)) == Archs.end())
	{
		return false;
	}
	const VtArch& A = VtTable()[static_cast<size_t>(S.Arch)];
	const VtAllow Al = VtAllowedOn(S.Arch, Lead, F);
	const bool Fields = VtIn(A.Layouts, S.Layout) && S.Mirror >= 0 && S.Mirror < 4 && VtIn(A.Bgs, S.Bg) && VtIn(Al.Pals, S.Pal) && S.Subj == Al.Subj &&
		VtIn(Al.Exprs, S.Expr) && VtIn(A.Poses, S.Pose) && VtIn(A.PropsA, S.PropA) && VtIn(A.PropsB, S.PropB) && VtIn(A.Caps, S.CapStyle) &&
		VtIn(A.Stickers, S.Sticker) && S.Corner >= 0 && S.Corner < CnCount && S.Variant < 1000;
	if (!Fields)
	{
		return false;
	}
	const std::pair<std::string, std::string> Cap{S.Caption, ThumbCaption2(S)};
	if (std::find(Al.Caps.begin(), Al.Caps.end(), Cap) == Al.Caps.end())
	{
		return false;
	}
	return S.Caption2 == Cap.second + VtFactBlock(S.Arch, Lead, F, S.Pal, S.Subj);
}

std::vector<Candidate> FixtureMoments()
{
	// One moment of every kind the library draws (a real hand's shape, real arithmetic), and an idea candidate per idea group:
	// the contact sheet's and the suite's (rerun_content keeps its own, richer fixtures; these are the sheet's).
	static const std::vector<Candidate> All = [] {
		std::vector<Candidate> Out;
		auto Hand = [&Out](const char* Id, Kind K, const char* Mine, const char* Theirs, const char* Board, Chips Net, double Eq, int Source) {
			Candidate C;
			C.Id = Id;
			C.At = 40.0 * 1440.0 + static_cast<double>(Out.size());
			C.K = K;
			C.Source = Source;
			C.Event = Source == 1 ? "Embercrest Nightly" : "Night Owl Turbo";
			C.BuyIn = Source == 1 ? 12000 : 550;
			C.Field = 104;
			C.Rec.Number = 214;
			C.Rec.Blinds.Sb = 100;
			C.Rec.Blinds.Bb = 200;
			C.Rec.Mine = ParseCards(Mine);
			C.Rec.Board = ParseCards(Board);
			if (Theirs[0])
			{
				C.Rec.Shown.push_back({"Mike_R", ParseCards(Theirs)});
			}
			C.Opponent = "Mike_R";
			C.Rec.Lines.push_back({Street::Preflop, "You", "all in", 6400});
			C.Rec.Pot = 12900;
			C.Rec.HeroNet = Net;
			C.EquityAtAllIn = Eq;
			C.Drama = 0.6;
			Out.push_back(C);
		};
		Hand("fx:beat", Kind::BadBeat, "AsAd", "7c7h", "Kd8s2c9h7d", -6400, 0.81, 0);
		Hand("fx:suck", Kind::Suckout, "AcJd", "QhQc", "8d3cJs5hAh", 6500, 0.30, 0);
		Hand("fx:cool", Kind::Cooler, "KdKs", "AhAc", "Kh7dAs9s4h", -6400, -1.0, 0);
		Hand("fx:bluff", Kind::Bluff, "6s5s", "", "KhQd2cJs8h", 3000, -1.0, 0);
		Hand("fx:caught", Kind::BluffCaught, "6s5s", "KcTd", "KhQd2cJs8h", -7200, -1.0, 0);
		Hand("fx:call", Kind::HeroCall, "Td9d", "AsQs", "Th6c3s2h8c", 4400, -1.0, 0);
		Hand("fx:fold", Kind::HeroFold, "QhQd", "", "Ks9c4h", -1100, -1.0, 0);
		Hand("fx:pot", Kind::BigPot, "JhJc", "8h8d", "Jd8s3c2h9c", 11100, -1.0, 0);
		Hand("fx:ko", Kind::Knockout, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, -1.0, 1);
		Hand("fx:bubble", Kind::Bubble, "AsKd", "QcQh", "Ad7s2c9h4d", 6500, -1.0, 0);
		Hand("fx:rival", Kind::Rival, "AsAd", "7c7h", "Kd8s2c9h4d", 6500, -1.0, 0);
		Out.back().Opponent = RivalName;
		Out.back().Rival = true;
		Out.back().Rec.Shown[0].first = RivalName;
		Hand("fx:quads", Kind::Monster, "9s9h", "AcKd", "9c9d2hKs4c", 9000, -1.0, 0);
		Hand("fx:sets", Kind::SetOverSet, "8s8h", "9c9d", "9s8d2c5hKc", -6400, -1.0, 0);
		auto Run = [&Out](const char* Id, Kind K, int Place, Chips Prize) {
			Candidate C;
			C.Id = Id;
			C.At = 41.0 * 1440.0 + static_cast<double>(Out.size());
			C.K = K;
			C.Event = "Sunday Blaze";
			C.BuyIn = 1100;
			C.Field = 1530;
			C.PlaceSoFar = Place;
			C.Rec.Pot = Prize;
			C.Rec.HeroNet = Prize - 1100;
			C.Final = Place <= 9;
			C.Drama = 0.8;
			Out.push_back(C);
		};
		Run("fx:deep", Kind::DeepRun, 15, 4200);
		Run("fx:ft", Kind::FinalTable, 4, 119000);
		Run("fx:win", Kind::Win, 1, 148600);
		Candidate Story;
		Story.Id = "story:rent-paid";
		Story.K = Kind::Story;
		Story.Event = "rent-paid";
		Story.Source = 4;
		Story.At = 42.0 * 1440.0;
		Out.push_back(Story);
		Candidate Clip = Out.front();
		Clip.Id = "fx:clip";
		Clip.K = Kind::StreamMoment;
		Clip.Stream.Viewers = 312;
		Clip.Stream.Face = 5;
		Clip.Stream.Chat = {"NO WAY", "the river!!"};
		Out.push_back(Clip);
		// Ideas: a few of each family the sheet shows (their facts in the form MakeIdeaCandidate writes them).
		auto Idea = [&Out](int Id, const char* Facts) {
			Candidate C;
			C.Id = "idea:fx:" + std::to_string(Id);
			C.At = 43.0 * 1440.0 + static_cast<double>(Out.size());
			C.K = Kind::Idea;
			C.Source = 5;
			C.Idea = Id;
			C.IdeaFacts = Facts;
			C.Drama = 0.5;
			Out.push_back(C);
		};
		Idea(501, "w=1.30;cool=25;ad=results;fmt=12;Month=October;Events=214;Cashes=31;NetSigned=up+$412;Net=$412;NetPlus=+$412;Roi=9%25;Abi=$5.50;If=U");
		Idea(503, "w=1.40;cool=10;ad=results;fmt=12;Down=22;Days=30;Events=96;NetSigned=down+$121;Abi=$5.50");
		Idea(602, "w=1.50;cool=7;ad=teach;fmt=1;cls=S;Hand=ace-queen;Street=turn;CostBb=38;Event=Night+Owl+Turbo");
		Idea(608, "w=1.30;cool=7;ad=teach;fmt=1;cls=S;Count=5");
		Idea(1105, "w=1.00;cool=30;ad=gear;fmt=12;Items=6;GearCost=$1,407;Gear=Lumen+4K");
		Idea(801, "w=1.20;cool=3;ad=news;fmt=1;Winner=Tomas+Ekdahl;Major=Grand+Circuit+Main;Prize=$1.2M");
		Idea(1601, "w=1.30;cool=0;ad=results;fmt=12;Start=$50;Goal=$500;Now=$214;Day=19;Ep=4;Site=Ignite+Poker;Series=$50+to+$500+on+Ignite+Poker");
		Idea(901, "w=1.40;cool=0;ad=story;fmt=01;Subs=1,000");
		Idea(505, "w=1.50;cool=0;ad=results;fmt=12;Rent=$1,075;Month=November");
		Idea(1406, "w=1.40;cool=25;ad=results;fmt=12;Division=Gold;DivisionWas=Steel");
		Out.back().Site = 2;
		return Out;
	}();
	return All;
}
} // namespace vcontent
} // namespace video
} // namespace ss
