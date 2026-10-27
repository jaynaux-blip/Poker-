#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Video.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
namespace world
{
class World;
}

/**
 * Update 3 (SPEC §6, §7.3): Rerun 2.0's library. About 2,000 title templates behind a stable picker that never offers one of
 * the last 20 titles' templates, thumbnail recipes over 36 archetypes (captions only from facts), 175 video ideas from the
 * player's own career (locked until their facts exist), 20 comment archetypes and the world's creators. Declared by the
 * lead in Phase A; bodies by core-videocontent (VideoTitles.cpp, VideoThumbs.cpp, VideoIdeas.cpp, VideoComments.cpp,
 * VideoWorld.cpp; G3).
 */
namespace video
{
/** One of Studio's three title options: the template's key, the filled text, its promise and hidden traits. */
struct TitleOption
{
	std::string Key, Text;
	uint8_t Promise = 0;
	uint16_t Traits = 0;
};
/** An idea on the IDEAS board: its facts, its formats, whether it's ready, and why not. */
struct IdeaCard
{
	int Id = 0;
	std::string Name, Fact, Hint;
	uint8_t Group = 0;
	std::vector<Format> Formats;
	int EditMin = 0, Energy = 0;
	bool Ready = false, Trending = false, Limited = false, Strike = false;
	int CooldownDays = 0, Have = 0, Need = 0;
};
/** What the ideas read (built by the Session from History, the ledger, vsnap, grades, moments, Kast, life, live entries, the world log, site play). */
struct IdeaCtx;
/** A repackage: a new title template, a new recipe, or both. */
struct Repack
{
	std::string TitleKey;
	ThumbSpec Spec;
};
/** Count options for a choice (the Shuffle-th deal); skips templates in the last 20 titles. */
SHORTSTACKCORE_API std::vector<TitleOption> TitleOptions(const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Shuffle, int Count);
/** Count recipes in hash order, at least 4 archetypes a deal. */
SHORTSTACKCORE_API std::vector<ThumbSpec> ThumbOptions(const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Shuffle, int Count);
/** How well an archetype fits the moment (§4.6.2). */
SHORTSTACKCORE_API double ArchetypeFit(int Arch, int KindGroup, const Candidate& Lead);
/** An archetype's Update 2 frame (§4.4). */
SHORTSTACKCORE_API Frame FrameOf(int Arch);
/** Pinned by rerun_content --write. */
SHORTSTACKCORE_API double TraitMean(int KindGroup);
/** The recipe's traits over TraitMean. */
SHORTSTACKCORE_API double ThumbTraits(const ThumbSpec& S, const Candidate& Lead, const Channel& C);
SHORTSTACKCORE_API uint8_t ThumbPromise(const ThumbSpec& S);
/** The recipe's visual signature (no caption text): rerun_content counts distinct looks with it. */
SHORTSTACKCORE_API uint64_t ThumbSignature(const ThumbSpec& S);
SHORTSTACKCORE_API std::vector<IdeaCard> Ideas(const IdeaCtx& X, const Channel& C, double World);
SHORTSTACKCORE_API bool MakeIdeaCandidate(const IdeaCtx& X, int IdeaId, double World, Candidate& Out);
/** A generated comment (procedural and read-only in P0). */
struct Comment
{
	std::string Author, Text;
	int Likes = 0;
	double At = 0;
	uint8_t Archetype = 0;
};
SHORTSTACKCORE_API std::vector<Comment> CommentsOf(const Video& V, double World, int From, int Count);
/** A video by one of the world's creators (Home, Next up, Research). */
struct WorldVideo
{
	std::string Creator, Title;
	ThumbSpec Spec;
	double At = 0, Views = 0;
};
SHORTSTACKCORE_API std::vector<WorldVideo> WorldVideos(const world::World& W, double World, int Count);
/** Every sentence the library says, for the copy lint and the trademark screen. */
SHORTSTACKCORE_API std::vector<std::string> InventoryU3();

// ------------------------------------------------------------------ core-videocontent's additions (C3; append-only)
// What the compositor (ui-thumbs), Studio and the Channel pages (ui-rerun), the Session and core-videoalgo read beyond the
// Phase A declarations above. Nothing above changed.

/**
 * The thumbnail vocabulary (§6.1): every ThumbSpec field's values, numbered for ui-thumbs' compositor.
 *   Arch     §6.1's table row less one (0 Gut Punch .. 35 Mistake; POV, 33, is P1 and never dealt)
 *   Layout   the 18 landscape layouts below (each archetype fixes 1-3)
 *   Mirror   bit 0 the layout mirrored, bit 1 the wide zoom (else tight)
 *   Bg, Pal, Subj, CapStyle, Sticker (-1 none), Corner: the enums below
 *   Expr     Portrait.h's ExpressionOf order (0 Neutral .. 11 Exhausted); Pose PortraitPose's (0 None .. 4 Fist pump)
 *   PropA, PropB (-1 none): the props below
 *   LookRef  ChannelX::Looks' Ref the player's face was frozen with at upload (-1: the current look, Studio's preview)
 *   Variant  the compositor's small choices (prop placement, a burst's turn), 0-999
 *   Caption, Caption2  the caption's two lines (<= 14 characters each; Caption2 may be empty), filled only from facts (U7)
 * The compositor's facts ride at the end of Caption2, behind FactSep, as FactSep-separated "key=value" (ThumbFact reads
 * them; the caption's own second line is ThumbCaption2): c the cards in Thumb::Cards' convention ("AsAd/7c7h/Kd8s2c9h7h": the
 * player's, a hand turned up, the board), k the board card the archetype marks (0-4: the river circled, the card that
 * turned it), o the other face's name (hero::CharacterFor; never in storytime), n the big number when it isn't a caption
 * line, f the "from" of Before -> After ("$11", "STEEL"), t its "to", p a progress 0-100 (Day Counter), g the graph ("u" or
 * "d" and up to 8 digits 0-9, oldest first), s the site (net::SiteId), e the event's name (its crest), m a multiplier ("25"),
 * h a chat line (Stream Clip: up to two h entries), r a list's rows (Tier List, Top Five: up to five, '|'-separated).
 */
namespace thumb
{
enum ArchId : int16_t
{
	ArGutPunch,
	ArCircledRiver,
	ArTheRead,
	ArNervesOfSteel,
	ArCaught,
	ArTheFold,
	ArThePile,
	ArBigNumber,
	ArBeforeAfter,
	ArTrophyShot,
	ArVsSplit,
	ArTheBubble,
	ArHeadhunter,
	ArEnvelope,
	ArTheGraph,
	ArDayCounter,
	ArNightShift,
	ArVenue,
	ArRoadTrip,
	ArRentLetter,
	ArClassroom,
	ArQuiz,
	ArTierList,
	ArTopFive,
	ArDeskSetup,
	ArCollabDuo, // P1 (collabs): never dealt in P0
	ArReactionBox,
	ArStreamClip,
	ArMilestone,
	ArTextSlab,
	ArTheGhost,
	ArPayday,
	ArMonsterHand,
	ArPov, // P1 (vertical): never dealt in P0
	ArSiteCard,
	ArMistake,
	ArCount,
};
enum LayoutId : int16_t
{
	LyFaceWords,   // face left, words right
	LyCardsFace,   // cards left, face right
	LyFaceFan,     // face centre, the fan below
	LyRiver,       // the board, the river circled
	LySplitVs,     // a diagonal split, two faces
	LyNumberFace,  // a big number and a face
	LyBeforeAfter, // two panels and an arrow
	LyTableTop,    // a top-down table
	LyTrophy,      // the trophy hero shot
	LyStreamClip,  // a stream frame with the facecam
	LyGraph,       // a line graph
	LyTextSlab,    // a slab of text
	LyCollage,     // a collage of panels
	LyTierList,    // tier rows (or five numbered cells)
	LyDesk,        // the desk
	LyLetter,      // the letter (a notice, a bill, a statement)
	LyVenue,       // the venue's marquee
	LyQuestion,    // the question: option cards
	LyCount,
};
enum BgId : int16_t
{
	BgGradient,
	BgBurst,
	BgSpeedLines,
	BgFeltBokeh,
	BgMoneyRain,
	BgDiagonal,
	BgSpotlight,
	BgScanlines,
	BgCount,
};
enum PalId : int16_t
{
	PlHeat,
	PlFelt,
	PlMoney,
	PlIce,
	PlNeon, // clear of Kast's violet and lime
	PlGoldOnBlack,
	PlSunrise,
	PlNight,
	PlMono, // mono and one accent
	PlBlood,
	PlSite, // the hand's site's colours (the s fact)
	PlCount,
};
enum SubjId : int16_t
{
	SjYou,
	SjOpponent, // procedural from the o fact; storytime: a silhouette (never named)
	SjCreator,
	SjWorldPlayer,
	SjGhost, // gh0stfold's ghost
	SjNobody, // cards only
	SjObject, // a trophy, an envelope, a letter, the desk
	SjCount,
};
enum ExprId : int16_t
{
	ExNeutral,
	ExSmile,
	ExGrin,
	ExLaugh,
	ExShocked,
	ExScream,
	ExDevastated,
	ExTilted,
	ExSmug,
	ExSkeptical,
	ExThinking,
	ExExhausted,
	ExCount,
};
enum PoseId : int16_t
{
	PsNone,
	PsHandsOnHead,
	PsFacepalm,
	PsPoint,
	PsFistPump,
	PsCount, // P0's (PortraitPose has the P1 four after these)
};
enum PropId : int16_t
{
	PrHoleCards,   // your two cards (c)
	PrShownCards,  // the hand turned up (c)
	PrBoard,       // the board (c)
	PrCardBacks,   // two backs with "??"
	PrChipPile,
	PrCashStack,
	PrTrophy,
	PrBracelet,
	PrRing,
	PrEventCrest,  // eventart's crest (e)
	PrSiteMark,    // siteart's mark (s)
	PrVenueMark,   // the Embercrest's crest and marquee
	PrSponsorMark,
	PrBadge,
	PrGear,        // the desk's gear (streamart::Product)
	PrFlag,        // NetFlag
	PrEviction,    // the eviction notice
	PrPowerBill,
	PrStickyNote,
	PrNeonSign,    // props:: the neon sign
	PrPawnTicket,
	PrBus,
	PrLaptopLobby, // a laptop with a lobby (four tiny tables)
	PrPhoneChat,   // a phone with chat (h)
	PrWhiteboard,  // a whiteboard with a 13 x 13 range grid
	PrFeltTop,     // a top-down felt
	PrCardFan,     // five cards fanned with vector ranks (c)
	PrMysteryEnvelope,
	PrStatement,   // the payout statement
	PrProgressBar, // p
	PrGraphLine,   // g
	PrCrosshair,
	PrOptionCards, // CALL / FOLD
	PrTierRows,    // r
	PrNumberedCells, // r, a crown on #1
	PrConfetti,
	PrShield,      // a League division's shield (f, t)
	PrFuseReel,    // the reel at the multiple (m)
	PrCount,
};
enum CapStyleId : int16_t
{
	CsOutlined, // the 8-way fake outline and a drop shadow
	CsBoxed,    // a boxed plate
	CsTwoTone,
	CsMarker,   // a marker bar
	CsStamp,
	CsNumberBurst,
	CsLowerThird,
	CsNone,
	CsCount,
};
/** Stickers and annotations (vectors, never emoji). "Fire" from §6.1's list is a lightning bolt here (C9: no fire glyph). */
enum StickerId : int16_t
{
	StRedCircle,
	StCurvedArrow,
	StRedX,
	StQuestionMarks, // "??"
	StPercentChip,
	StAllInStamp,
	StSkull,
	StBolt,
	StMoneyBag,
	StCryingFace,
	StShockedFace,
	StSiren,
	StCrown,
	StGhost,
	StCount,
};
enum CornerId : int16_t
{
	CnInitials,
	CnLogo,
	CnColorBar,
	CnNone,
	CnCount,
};
/** Before the compositor's facts in Caption2 (ASCII unit separator: never in copy). */
constexpr char FactSep = '\x1f';
} // namespace thumb
/** A compositor's fact from S's Caption2 ("" when absent); every value of a repeated key (h); the second line alone. */
SHORTSTACKCORE_API std::string ThumbFact(const ThumbSpec& S, const char* Key);
SHORTSTACKCORE_API std::vector<std::string> ThumbFacts(const ThumbSpec& S, const char* Key);
SHORTSTACKCORE_API std::string ThumbCaption2(const ThumbSpec& S);
/** An archetype's name ("Circled River") and the read under its option in Studio ("the river, circled: a board that tells the story"). */
SHORTSTACKCORE_API const char* ArchName(int Arch);
SHORTSTACKCORE_API std::string ArchRead(int Arch);
/** VT1's contact sheet (ss.Video.Sheet): Count recipes for a kind group (0 beats .. 5 stories, 6 the ideas) from the library's
 *  fixture moments, no (archetype, palette) pair twice. */
SHORTSTACKCORE_API std::vector<ThumbSpec> ContactSheet(int Group, int Count);

/**
 * What the ideas read (§6.3): the Session builds it (SessionVideo3.cpp) from what it already keeps; nothing in it is saved.
 * Every list is newest first. A test builds one by hand.
 */
enum IdeaFlag : uint32_t
{
	IfLive = 1,        // the Embercrest
	IfSeries = 2,      // a series event (RCOP, Micro Madness, a site's series)
	IfBounty = 4,      // a PKO
	IfMystery = 8,     // mystery bounties
	IfHyper = 16,
	IfOmaha = 32,
	IfSatellite = 64,
	IfSeatWon = 128,   // a satellite's seat won
	IfFreeroll = 256,
	IfMajor = 512,     // a Sunday major (Sunday Blaze, Sunday 100K, the Embercrest Sunday) or a Main Event
	IfOverlay = 1024,  // the guarantee wasn't met
	IfMidnight = 2048, // the Embercrest's Midnight Turbo
	IfFlipRun = 4096,  // Ignite's Flip & Run
	IfSpark = 8192,    // The Spark (RiverLine's)
	IfTurbo = 16384,
	IfFinal = 32768,   // reached the final table
};
struct IdeaResult
{
	double At = -1;        // the event's start (world minutes)
	std::string Name, Id;  // its name and scheduled id ("mm-26@1530")
	int Site = 0;          // net::SiteId
	int Place = 0, Field = 0;
	Chips BuyIn = 0, Prize = 0; // a seat won counts at its value
	uint32_t Flags = 0;    // IdeaFlag
	int Hands = 0;         // hands the player played in it (0: unknown)
};
/** A graded decision of the player's own (the coach's grade; never autoplay's, U4). */
struct IdeaGrade
{
	double At = 0;
	int Grade = 0;         // 0 best .. 3 mistake (coach::Grade's order)
	std::string Event, Hand, Street; // the event, the player's cards ("AhQh"), the street of the decision
	double StackBb = 0, PotBb = 0;
	bool River = false, Shove = false, Bubble = false, Fold = false;
	Chips CostCents = 0;   // what a mistake cost in chips (0: unknown)
	double CostBb = 0;
};
/** A world event at least a day old (U5), with its people's names. */
struct IdeaWorldEvent
{
	double At = 0;
	int Kind = 0;          // world::EventKind
	std::string Who, Other, What;
	Chips Amount = 0;
	int Value = 0, Site = 0;
};
/** A Kast clip (the Clip Lane's and the channel's). */
struct IdeaClip
{
	std::string Title, Candidate;
	double At = 0, Views = 0;
	int Moment = 0;        // kast::Moment
};
/** A Kast stream's log line (the newest 60). */
struct IdeaStream
{
	double Start = 0, Minutes = 0;
	int Avg = 0, Peak = 0, Follows = 0, Raids = 0, Predictions = 0;
};
struct IdeaCtx
{
	double World = 0;
	std::string Hero; // the screen name
	const Channel* Chan = nullptr; // the channel the ideas read (MOMENTS, the kept videos, the series); MakeIdeaCandidate's
	// Poker.
	std::vector<IdeaResult> Results; // History's (online, dated by the scheduled id) and the live entries
	std::vector<IdeaGrade> Grades;
	int Events = 0, Cashes = 0, FinalTables = 0, Wins = 0, Freerolls = 0; // the Tracker's lifetime
	Chips BuyIns = 0, Prizes = 0, BestPrize = 0, BestPotBb = 0;           // BestPotBb: the career's biggest pot before tonight, in BB
	int StudyWeek = 0;     // coaching sessions in the last 7 days
	int DeesNights = 0;    // nights at Dee's
	int LiveEvents = 0;
	double FirstLiveAt = -1;
	// Money and life.
	Chips Bankroll = 0, RentDue = 0;
	double RentDeadline = 0;
	int RentStage = 0, RentsPaid = 0, Evictions = 0;
	bool Evicted = false;
	double LastRentAt = -1; // the last rent paid
	int Shifts = 0, NightShifts14 = 0, WashFold = 0; // ShiftLink shifts: all, nights in 14 days, Wash & Fold ever
	double LastShiftAt = -1.0e9;
	std::vector<std::string> Stories; // the story keys told (rent-paid, back:N, bracelet-first, ...)
	std::vector<std::pair<std::string, Chips>> Gear; // owned gear: name, price
	Chips GearBought7 = 0;  // gear bought in the last 7 days
	std::string NewestGear;
	int Webcam = 0, WebcamWas = 0; // the webcam tier now, and before the last purchase
	int Home = 0;           // homes lived in (the apartment, Dee's couch)
	// Kast.
	int KastFollowers = 0, KastStreams = 0, RaidsIn = 0;
	std::vector<IdeaClip> Clips;
	std::vector<IdeaStream> Streams;
	// Rerun (the channel itself is passed beside the context).
	int Comments30 = 0;     // comments on the kept videos in the last 30 days
	double SubsWeekAgo = 0;
	// The world, at least a day old.
	std::vector<IdeaWorldEvent> WorldLog;
	std::string SeriesNow, SeriesNext; // a series running now; the next one
	double SeriesNextAt = -1;
	int SeasonRank = 0;     // RiverLine's Season board (0: unranked)
	// The rival sites.
	std::vector<int> Joined; // net::SiteId of each site the player has an account on
	std::vector<std::pair<int, double>> FirstSession; // site, the first session's start
	bool WelcomePending = false; // Ignite's welcome bonus still clearing
	int RewardsTier = 0;    // RunItTwice's Second Run or Ignite's Rewards tier reached (0 none)
	int FuseBest = 0;       // the best FUSE multiple (site\tfuse)
	Chips FuseBestPrize = 0, FuseBestBuyIn = 0;
	int Flame = 0;          // Ignite's Flame, days
	bool BeefPaid = false;  // knocked out your Beef
	std::string Beef;
	int BeefKo = 0;         // the times your Beef busted you
	std::string Headliner;  // a Headliner you scalped (latest)
	bool SpecialGuest = false;
	std::string Division, DivisionWas; // a League promotion
	bool DeadlineDay = false;
	std::string CalledOut;  // a Callout won, against
	bool TeamScalps = false;
	int ClubTier = 0;       // 2 High Table, 3 Inner Circle
	std::string TeamSigned; // the site team that signed the player
};
/** A series (§6.3 P: the SERIES tab): its record and its totals. */
struct SeriesRow
{
	int Id = 0, Idea = 0, Site = 0, State = 0; // State: 0 running, 1 the goal reached, 2 bust, 3 ended
	std::string Name;
	int Episodes = 0;
	double Views = 0, StartedAt = 0;
	Chips StartCents = 0, GoalCents = 0, NowCents = 0;
	std::vector<std::string> Videos; // its episodes, oldest first
};
SHORTSTACKCORE_API std::vector<SeriesRow> SeriesOf(const Channel& C, const IdeaCtx& X, double World);
/** The idea's advertiser category (the Ad market card's rows: 0 Setup and gear .. 7 Rival drama; -1: the title style's). */
SHORTSTACKCORE_API int IdeaAdCategory(int IdeaId);
/** An idea's name by id ("" when none), and its group's letter (A .. P). */
SHORTSTACKCORE_API std::string IdeaName(int IdeaId);
/**
 * The new hand kinds a record shows on its own (§6.3, the ideas' unlock column): quads or better made (Monster), both sets
 * at showdown (SetOverSet), a hand under 5% after the flop that won or lost to it (RunnerRunner). Classify takes the
 * weightiest; the kinds that need the event's context (DoubleKo, LadderFold, MinCash, HeadsUp, BiggestPot, LastHand,
 * Envelope) are the Session's (VideoNoteHand) and the card room's (NoteLiveHand).
 */
SHORTSTACKCORE_API std::vector<Kind> RecordKinds(const recall::Record& Rec, bool Showdown);
/** Studio's packaging read (§6.6.2): facts and comparisons, never a future number (W9). */
SHORTSTACKCORE_API std::vector<std::string> PackagingRead(const Choice& Ch, const std::vector<Candidate>& Open, const Channel& C, double World, bool AdsOn);
/** The suitability coin a choice will get (Coin), and Studio's reason for it ("" for full ads). */
SHORTSTACKCORE_API Coin CoinOf(const Choice& Ch, const std::vector<Candidate>& Open);
SHORTSTACKCORE_API std::string CoinReason(const Choice& Ch, const std::vector<Candidate>& Open);
/** The edit's world minutes and energy for an Update 3 choice: the length option, a Duel's extra contenders (15 minutes and
 *  1 energy each), Clean edit's 15 minutes (Session::Publish's TimeSkip; Studio's "Edit 1 h 45 m (+Duel) · 9 energy"). */
SHORTSTACKCORE_API double EditMinutesOf(const Choice& Ch);
SHORTSTACKCORE_API double EditEnergyOf(const Choice& Ch);
/**
 * Repackage's options for a published video (§4.6.7: Studio's REPACKAGE card): its moment has left MOMENTS, so they read
 * what the video itself keeps (its kind and idea, its cards, the named opponent, its old title's numbers). Fewer titles
 * than at the first publish come back (a template that needs the event's name doesn't). Same picker, same recipes.
 */
SHORTSTACKCORE_API std::vector<TitleOption> RepackTitles(const Video& V, const Channel& C, int Shuffle, int Count);
SHORTSTACKCORE_API std::vector<ThumbSpec> RepackThumbs(const Video& V, const Channel& C, int Shuffle, int Count);

// ------------------------------------------------------------------ the C3 fix round's additions (c3f-content; append-only)

/**
 * Studio's CAPTION, FACE and PALETTE pickers (§6.6.2): what one dealt recipe may become inside its archetype's rules on this
 * moment. The values are the generator's own (the archetype's palettes, faces, caption styles and its caption rows filled from
 * this moment's facts, after the same "never"s), so an edited recipe is one ThumbOptions could have dealt: it draws, prices and
 * saves like any other (the edit is the recipe's own fields; nothing else is kept). Exprs is empty when the archetype shows no
 * face of yours; a stream moment keeps the face the stream showed.
 */
struct ThumbChoices
{
	std::vector<int16_t> Pals, Exprs, CapStyles;
	std::vector<std::pair<std::string, std::string>> Captions; // both lines, facts only; the recipe's own among them
};
enum ThumbField : int
{
	TfPalette,  // Value: a thumb::PalId from ThumbChoices::Pals
	TfFace,     // Value: a thumb::ExprId from ThumbChoices::Exprs
	TfCapStyle, // Value: a thumb::CapStyleId from ThumbChoices::CapStyles
	TfCaption,  // Value: an index into ThumbChoices::Captions
};
/** The choices for recipe S of choice Ch (Cands and C as ThumbOptions takes them: the channel's MOMENTS and the channel). */
SHORTSTACKCORE_API ThumbChoices ThumbChoicesOf(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C);
/** S with one field changed (ThumbField), the compositor's facts kept (the site's colours' fact follows the palette); S
 *  unchanged when the value isn't one of S's choices. */
SHORTSTACKCORE_API ThumbSpec ThumbEdit(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C, int Field, int Value);
/** Whether S is a recipe the generator could deal for its archetype on this choice, edits included (Publish asks it of every
 *  contender). */
SHORTSTACKCORE_API bool ThumbWithin(const ThumbSpec& S, const Choice& Ch, const std::vector<Candidate>& Cands, const Channel& C);

/**
 * Research's "What poker viewers search" (§5.8): phrases viewers type, in the poker world's terms (evergreen strategy, the
 * formats the player plays, the stories viewers look for, and this week's trends from the world's calendar and log), each with
 * this week's relative interest and its arrow against last week. Interest and arrow are facts of the world, not of the player:
 * the week's trends (their mean heat over the week, read at its Monday like Trends), the calendar's seasons and a draw keyed
 * on the phrase and the week (Fnv1a), so a skip or a reload never changes them inside a week. The player's state picks
 * nothing but which rows are theirs (YourFormat), how many of their kept videos are on each (Mine), and the idea to make next.
 */
struct SearchRow
{
	std::string Key, Phrase;
	int Interest = 0;      // this week, 1-100 (the week's most searched phrase is 100)
	int Arrow = 0;         // against last week: +1 rising, 0 steady, -1 falling
	int Source = 0;        // 0 strategy, 1 this week's trend, 2 a format, 3 a story viewers look for
	bool YourFormat = false; // a format the player played in the last 60 days
	int Mine = 0;          // the player's kept videos on it
	int Idea = 0;          // the idea it suggests (IdeaCard::Id), 0 none
	std::string IdeaName, IdeaHint; // the idea's name; when it's locked, what to do (its card's hint)
	bool IdeaReady = false;
};
struct Trend; // VideoStats.h
/** The table: Count rows, the most searched first (at least two of the player's formats when they have them). ThisWeek and
 *  LastWeek are WeekTrends' lists; Ideas the IDEAS board's cards (Ideas()); World the minute it's read (its week). */
SHORTSTACKCORE_API std::vector<SearchRow> Searches(const IdeaCtx& X, const Channel& C, const std::vector<IdeaCard>& Ideas, const std::vector<Trend>& ThisWeek,
	const std::vector<Trend>& LastWeek, double World, int Count);
} // namespace video
class Session;
namespace video
{
/** The trends of the week that holds World (Monday 00:00 to Sunday), each with its mean heat over the week's 14 half-days:
 *  every one listed at that Monday (Trends' rule: the world's events before it), so the list is the week's, whole. */
SHORTSTACKCORE_API std::vector<Trend> WeekTrends(const Session& S, double World);
/** The UI's call: Searches over the session's career, this week's and last week's trends, at the session's world minute.
 *  Ideas: the IDEAS board's cards (Session::VideoIdeas, as the page already holds them). Cache it per world day. */
SHORTSTACKCORE_API std::vector<SearchRow> SearchesOf(const Session& S, const std::vector<IdeaCard>& Ideas, int Count);
} // namespace video
} // namespace ss
