#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Recall.h"

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
struct SaveData;
namespace life
{
struct State;
struct Mark;
}
namespace kast
{
struct Channel;
}
namespace room
{
struct HandNote;
}
namespace world
{
class World;
}

/**
 * Rerun (update 2 §4): the fictional video site where the player posts highlights cut from their own hands and streams.
 * Candidates exist only for hands that happened and carry only what the player saw (U7, N3); each video rolls its numbers
 * once at upload and then earns views in closed form over world minutes (a burst, then a long tail); ad money starts at
 * the partner gate (1,000 subscribers and 4,000 watch hours in 365 days) and pays monthly into the bank (ledger kind 13).
 * Saved as video\t... lines once Update2LinesRead's U2Video bit is set. Declared by the lead in Phase A; bodies by
 * core-video (Game/Video.cpp, VideoCopy.cpp, SessionVideo.cpp). Dormant until G2.
 */
namespace video
{
/** Saved: append-only. */
enum class Kind : int
{
	BadBeat,
	Suckout,
	Cooler,
	Bluff,
	BluffCaught,
	HeroCall,
	HeroFold,
	BigPot,
	Knockout,
	Bubble,
	DeepRun,
	FinalTable,
	Win,
	Rival,
	Story,
	StreamMoment,
	// Update 3 (SPEC §6.3, §7.3): the new hand kinds and the idea videos (core-videocontent's Classify rules and copy, G3).
	Monster,
	SetOverSet,
	RunnerRunner,
	DoubleKo,
	LadderFold,
	MinCash,
	Envelope,
	HeadsUp,
	BiggestPot,
	LastHand,
	Idea,
	Count,
};
/**
 * Update 2's kinds (BadBeat .. StreamMoment): the loops over what Update 2 ships (its copy, its glyphs, its tests) stop here,
 * so the kinds Update 3 appends change nothing until their copy lands (G3).
 */
constexpr int KindCountU2 = static_cast<int>(Kind::StreamMoment) + 1;
/** Saved: append-only. Short is shown as "Cut" from G3 (the enum is unchanged). */
enum class Format : int
{
	Short,
	Breakdown,
	Vlog,
	Recap,
	Count,
};
/** Saved: append-only. Hype is shown as "Loud" from G3 (the enum is unchanged). */
enum class TitleStyle : int
{
	Story,
	Hype,
	Teaching,
	Question,
	Plain,
	Count,
};
/** The thumbnail's frame. Saved: append-only. */
enum class Frame : int
{
	Reaction,
	Cards,
	Number,
	Versus,
	Trophy,
	Recipe, // Update 3: drawn from the video's ThumbSpec (ui-thumbs' compositor)
	Count,
};
/** How a video's tail behaves. Saved: append-only. */
enum class Class : int
{
	News,
	Strategy,
	Short,
	Count,
};

// ------------------------------------------------------------------ Update 3 (SPEC §7.3; declared by the lead in Phase A)
// Rerun 2.0's data: the levers (ad load, length, suitability, launch actions), the thumbnail recipe, a video's curve
// components and its Update 3 block, the channel's rings. Every saved enum is append-only. Written (video\t... sub-tags and
// vid's appended fields, §7.4) only for Update 3 videos and once Update3LinesRead's U3Video bit is set, so an old save
// writes back byte for byte; bodies by core-videoalgo and core-videocontent (G3).

/** The ads on a video (§4.5.5). Saved: append-only. */
enum class AdLoad : uint8_t
{
	Light,
	Standard,
	Heavy,
	Count,
};
/** The edit's length (§4.5.5). Saved: append-only. */
enum class LengthOpt : uint8_t
{
	Tight,
	Standard,
	Extended,
	Count,
};
/** Suitability (§4.5.4): full ads, limited, none (18+). Saved: append-only. */
enum class Coin : uint8_t
{
	Full,
	Limited,
	NoAds,
	Count,
};
/** The launch actions (§4.6.9): bits in VideoX::Launch. Saved: append-only. */
enum class Launch : uint8_t
{
	Cut,
	Kast,
	Teaser,
	Reply,
	Count,
};
/** What a curve component is (§4.4). Saved: append-only. */
enum class CompKind : uint8_t
{
	Verdict,
	Check7,
	Check14,
	Check30,
	Check90,
	Breakout,
	Duel,
	Repackage,
	Trend,
	FromCut,
	Kast,
	FromNextUp,
	// The creator update (SPEC_CREATORS §3.3, §8.1): the Binge wave and Late Bloomer, bonus-only (core-videoalgo, C3).
	Binge,
	LateBloom,
	Count,
};
/** A thumbnail recipe (§6.1): the archetype, layout and the rest, drawn by ui-thumbs' compositor; captions only from facts. */
struct ThumbSpec
{
	int16_t Arch = -1, Layout = -1, Mirror = 0, Bg = 0, Pal = 0, Subj = 0, Expr = 0, Pose = 0, PropA = -1, PropB = -1, CapStyle = 0, Sticker = -1, Corner = 0, LookRef = -1;
	uint32_t Variant = 0;
	std::string Caption, Caption2;
};
/** One curve component (V1): P [A (1 - e^(-(t - T)/tau1)) + (1 - A)(1 - e^(-(t - T)/tau2))] for t > T, the class's taus. */
struct Comp
{
	double T = 0, P = 0, A = 0;
	CompKind Kind = CompKind::Verdict;
	uint8_t SrcMix = 0;
	float CtrF = 1;
};
/** An Update 3 video's block (On), after the title in its vid line. Video::P keeps the full P. */
struct VideoX
{
	bool On = false;
	uint32_t Fate = 0;
	double Ctr = 0, Sigma = 0, KastShare = 0, S0 = 0, S0Base = 0, P0 = 0;
	double CtrNoise = 1, Earned = 0; // P0: the first wave's; Earned in double cents
	int Idea = -1, Series = -1;
	std::string TitleTpl;
	int Repacks = 0, LastRepackDay = -1;
	uint8_t Promise = 0, Payoff = 0;
	Coin C = Coin::Full;
	AdLoad Ads = AdLoad::Standard, AdsAtUpload = AdLoad::Standard;
	LengthOpt Len = LengthOpt::Standard;
	int AdsChangedDay = -1;
	uint8_t Launch = 0;
	std::array<float, 3> DuelCtr{};
	int8_t Duels = 1, DuelWin = -1, Verdict = -1;
	float SettledS = 0;
	std::array<ThumbSpec, 3> Spec{}; // [0] shown; [1], [2] the Duel's contenders
	std::vector<Comp> Comps;         // <= 9; the first wave is P0
	std::string NextUp;              // the Next-up card's target
	// The creator update (SPEC_CREATORS §3.3, §8.1), appended: the upload's own luck, set at PUBLISH from its existing draws
	// (ln(Luck x Viral)) so Momentum reads realized performance and a reload can't change it; vid's Update 3 block carries
	// them once core-videoalgo lands them (U3VideoFields 63 -> 64). Phase A-K: never set, never written.
	float LuckLn = 0;
	bool Viral = false; // the upload's Update 2 viral roll hit (narrated at the verdict as a breakout)
};
/** A day's (or a month's) row of the channel's rings. */
struct DayRow
{
	int Key = 0;
	float Views = 0, WatchH = 0, Gained = 0, Lost = 0, Likes = 0, Dislikes = 0, Impr = 0, Comments = 0;
	Chips Cents = 0;
	// core-videoalgo (C3: §4.5.7's itemized statement, §5.6's Revenue), appended: the Cuts feed's share of Cents and of Views,
	// and the watch page's ad impressions and monetized playbacks behind the rest (filled on the days ads pay; vday/vmonth/vlife
	// carry them).
	Chips CutCents = 0;
	float CutViews = 0, AdImpr = 0, Monetized = 0;
	// c3f-algo (C3 fix H1, the top-earner rate: the user's soft ceiling), appended: what the rate held back on the row's days (the
	// ads' earnings less Cents) and the Cuts feed's part of it; 0 under the rate (a "*" run in vday/vmonth, two after vlife's split).
	Chips HeldCents = 0, HeldCutCents = 0;
};
/** The folded tails' engagement: likes, dislikes, comments, impressions, per time constant. */
struct TailX
{
	double Total[4] = {0, 0, 0, 0};
	double Decay[2][4] = {};
};
/** A video kept in the lifetime top 20 after it left the newest 60. */
struct TopRow
{
	std::string Id, Title;
	double Views = 0;
	Chips Cents = 0;
	double At = 0;
};
/** A face frozen for a thumbnail (the Look it was drawn with). */
struct FrozenLook
{
	int Ref = 0;
	std::array<int16_t, 14> Look{};
};
struct Playlist
{
	std::string Id, Name;
	std::vector<std::string> Videos;
};
/** A challenge series (a bankroll run on a site, say). */
struct Challenge
{
	int Id = 0, Kind = 0, Site = 0;
	Chips Start = 0, Goal = 0;
	double StartedAt = 0;
	int State = 0, Episodes = 0;
};
/** A cooldown: an idea or a trend used, and when. */
struct Cool
{
	int Key = 0, LastDay = -1, Count = 0;
};
/** A month of the player's poker, for the monthly ideas' facts. */
struct Snap
{
	int Month = 0, Events = 0, Cashes = 0;
	Chips Net = 0, Rake = 0, Abi = 0;
	int RoiMilli = 0;
	float Hours = 0;
};
/** An upload day's evergreen stubs, summed (vstubd): F is the same sum x the minute offset / 1440. */
struct StubDay
{
	int Day = 0;
	double E = 0, EConv = 0, ERpm = 0, EF = 0, EConvF = 0, ERpmF = 0;
};
/** The channel's Update 3 state (On): the rings, the lifetime row, Momentum, looks, lists, cooldowns, snapshots, stubs. */
struct ChannelX
{
	bool On = false;
	std::vector<DayRow> Days;   // <= 90
	std::vector<DayRow> Months; // <= 24
	std::vector<TopRow> Top;    // <= 20
	DayRow Life;
	std::array<TailX, 3> TailsX{};
	double Momentum = 1.0;
	std::array<float, 6> Settled{};
	int SettledN = 0, LastUploadDay = -1;
	bool WelcomeBack = false;
	std::vector<FrozenLook> Looks;   // <= 8
	std::vector<Playlist> Lists;     // <= 8
	std::vector<Challenge> Challenges; // <= 4
	std::vector<Cool> IdeaCool;      // <= 40
	std::vector<Cool> TrendCool;     // <= 12
	std::vector<Snap> Snaps;         // <= 24
	std::vector<StubDay> StubDays;   // <= 366
	int TrendWeek = -1;
	int Since = -1;
	// The creator update (SPEC_CREATORS §3.3, §8.1), appended: Momentum from performance (Momentum above becomes MomC), the
	// meter's display average, the upload streak and Fresh on Rerun's monotonic count. vmom carries them once core-videoalgo
	// redefines it (C3). Phase A-K: never set, never written.
	double MomH = 0;     // H: 0.6 H + 0.4 (perf + the compressed score part) at each verdict
	double MomShown = 1; // the meter's display average: 0.5 D + 0.5 MomC at each verdict
	int StreakW = 0;     // weeks in a row with an upload (at most 8)
	int StreakWeek = -1; // the streak's last counted week (Monday 00:00)
	int RainChecks = 0;  // at most 2 held
	int UploadsEver = 0; // Fresh on Rerun's n: never falls (DeleteVideo can't re-open the lane)
};
/** What the stream had going when a candidate came from it (Kast's moment log). */
struct StreamBit
{
	int Viewers = 0;
	double HypeAfter = 0;
	int Face = 0;
	std::vector<std::string> Chat;
	double ClipAt = -1; // the Kast clip it links to, by its time (clips have no id)
};
/** A highlight that could be a video: a real hand or night, as the player saw it. */
struct Candidate
{
	std::string Id;
	double At = 0;
	int Source = 0, Site = 0; // Source: 0 online, 1 the Embercrest, 2 Dee's (storytime), 3 a skip
	Kind K = Kind::BigPot;
	recall::Record Rec;
	double EquityAtAllIn = -1;
	std::string Opponent;
	int OpponentNpc = -1;
	bool Rival = false;
	std::string Event;
	Chips BuyIn = 0;
	int Field = 0, PlaceSoFar = 0;
	bool Bubble = false, Final = false, Series = false;
	bool Auto = false;
	int Style = 0;
	bool Storytime = false;
	StreamBit Stream;
	double Drama = 0, Expires = 0;
	// Update 3 (§7.3; Source 5 = an idea video, §6.3): appended to the cand line for Idea candidates only.
	int Idea = -1;
	std::string IdeaFacts; // escaped key=value facts
};
/** What the player picked in Studio. */
struct Choice
{
	std::vector<std::string> Candidates;
	Format F = Format::Breakdown;
	TitleStyle T = TitleStyle::Story;
	Frame Fr = Frame::Reaction;
	// Update 3 (§7.3): the template picked, the shuffle, the recipes (1-3: a Duel's contenders), the levers. An empty
	// TitleTpl and no Thumbs is an Update 2 choice set (Update 2's own title picker and frame).
	std::string TitleTpl;
	int Shuffle = 0;
	std::vector<ThumbSpec> Thumbs; // 1-3
	AdLoad Ads = AdLoad::Standard;
	LengthOpt Len = LengthOpt::Standard;
	bool CleanEdit = false;
	std::string NextUp;
	int Series = -1;
};
struct Thumb
{
	Frame Fr = Frame::Reaction;
	int Mood = 0;
	std::string Cards; // one field, "AsKd7h"
	std::string BigText;
	uint32_t Accent = 0;
	std::string Opponent;
	int Site = 0;
};
/** A published video: its numbers are rolled once at upload (P, a, the two time constants, E, conv, retention, rpm). */
struct Video
{
	std::string Id, Title;
	double At = 0;
	Kind K = Kind::BigPot;
	Format F = Format::Breakdown;
	TitleStyle T = TitleStyle::Story;
	Frame Fr = Frame::Reaction;
	Class C = Class::News;
	Thumb Th;
	double P = 0, A = 0, Tau1 = 0, Tau2 = 0, E = 0, Conv = 0, Retention = 0, LengthMin = 0, Rpm = 0;
	std::string Sponsor;
	bool Auto = false;
	int Site = 0;
	VideoX X; // Update 3 (§7.3): read and written only when X.On
};
/** The folded tails of videos past the newest 60, per time constant; weights: views, subscribers, ad RPM (rebased each step). */
struct Tail
{
	double Total[3] = {0, 0, 0};
	double Decay[2][3] = {{0, 0, 0}, {0, 0, 0}};
};
/** An evergreen video's linear term, under 365 days old, after it left the newest 60. */
struct Stub
{
	std::string Id;
	double At = 0, E = 0, Conv = 0, Rpm = 0;
};
/** The player's channel (life::State::Video). */
struct Channel
{
	double Subs = 0;
	double PartnerAt = -1, ReviewAt = -1;
	Chips BalanceCents = 0, PaidCents = 0;
	std::vector<Candidate> Open; // at most 30
	std::vector<Video> Videos;   // at most 60
	std::array<Tail, 3> Tails{};
	std::vector<Stub> Stubs;
	std::array<float, 366> HoursByDay{};
	int HoursDay0 = 0; // until Partner
	std::set<std::string> Paid;
	double SteppedTo = 0;
	int Uploads14 = 0, Uploads7 = 0;
	std::vector<std::pair<double, std::string>> Bell; // the in-app bell, the last 20: when (world minutes), the line (video\tbell)
	double GrantedHours = 0; // dev only
	ChannelX X; // Update 3 (§7.3): read and written only when X.On
};
/** What the Channel page draws. */
struct ChannelView
{
	double Subs = 0, WatchHours365 = 0, Views28 = 0;
	bool Partner = false;
	Chips Balance = 0;
	std::array<double, 28> ViewsByDay{}, SubsByDay{}, CentsByDay{}; // the Channel page's chart
};
/** A live hand's context (the card room has no Session). */
struct LiveContext
{
	int Source = 1;
	std::string EntryId, Event;
	Chips BuyIn = 0;
	int Field = 0, Remaining = 0;
	bool Bubble = false, Final = false;
	bool Auto = false;
	int Style = 0;
	double World = 0;
};
/** What the session reads at upload. */
struct UploadFacts
{
	double GearQuality = 0;
	int Sharp = 0;
	int StreamViewers = 0;
	int KastFollowers = 0;
	double Energy = 0;
	// Update 3 (§7.3): the review waits for two rents paid (W11); burnout counts Update 3's extra editing only (§4.6.6).
	int RentsPaid = 0;
	double ExtraHours7 = 0;
};
SHORTSTACKCORE_API Kind Classify(const recall::Record& Rec, double EquityAtAllIn, bool Bubble, bool Showdown, double AvgStackBb, bool& Is);
SHORTSTACKCORE_API void NoteHand(Channel& C, const Candidate& Cand);
SHORTSTACKCORE_API void NoteLiveHand(life::State& L, const recall::Record& Rec, const room::HandNote* Note, const LiveContext& Ctx);
SHORTSTACKCORE_API void NoteStory(Channel& C, const std::string& Key, double World);
SHORTSTACKCORE_API std::string Title(const Choice& Ch, const std::vector<Candidate>& Cands, uint32_t Seed);
/** Publishes (the edit's time is the session's TimeSkip): the new video's id, or "" with nothing changed. */
SHORTSTACKCORE_API std::string Upload(Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F);
SHORTSTACKCORE_API double Views(const Video& V, double World);
/**
 * Views, subscribers, the gate, the review and the monthly payout from From to To (idempotent over any span). W: the
 * session's world (Update 3's trends read its log, §4.6.10); null reads the calendar's trends only.
 */
SHORTSTACKCORE_API void Step(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To, const world::World* W = nullptr);
SHORTSTACKCORE_API ChannelView View(const Channel& C, double World);
SHORTSTACKCORE_API std::vector<life::Mark> Marks(const Channel& C, double From, double To);
/** Dev (ss.Video.Grant): subscribers, watch hours and a synthetic video that gains Views over the next 30 days. */
SHORTSTACKCORE_API void Grant(Channel& C, double Subs, double Hours, double Views, double World);
SHORTSTACKCORE_API std::vector<std::string> Inventory();
SHORTSTACKCORE_API void WriteLines(const life::State& L, std::string& Out);
SHORTSTACKCORE_API bool ReadLine(const std::vector<std::string>& P, life::State& L);
SHORTSTACKCORE_API void AfterParse(SaveData& D);

// ------------------------------------------------------------------ core-video's additions (G2; append-only, §6.1)
// What Studio, the Channel page, the card room's host and Kast read beyond the day-1 contract. No struct above changed.
//
// Conventions the bodies keep (for ui-video, ue-auto and core-econ):
//   - A candidate's Rec names the player "You" (the live recall's own convention, BackRoomTable.cpp), so the online
//     records match the live ones. Storytime records (Dee's) name nobody else: "Seat 1", "Seat 2" by first appearance.
//   - Candidate::Source: 0 online, 1 the Embercrest, 2 Dee's (storytime), 3 an online hand the plan played while
//     skipping, 4 a life story (NoteStory), 5 an idea video (Update 3, §6.3). Rival: gh0stfold, or a name the room knows
//     (Rep Overall >= 35).
//   - DeepRun, FinalTable and Win candidates are the event, not a hand: Rec.Pot is the prize in cents, Rec.HeroNet the
//     prize less the buy-in, Rec.Mine and Rec.Lines empty.
//   - Thumb::Mood: a kast::Mood (0..5) for the facecam; 10 + mood for the portrait (live, where nobody films: §4.4).
//     Thumb::Cards: the player's cards, then '/', the opponent's if they were shown, then '/', the board ("AsAd/7c7h/Kd8s2c").

/** Letters Rerun mails (life::Letter::Kind, Source "video"). */
enum LetterKind : int
{
	LetterReview = 1,    // "Your channel's under review."
	LetterWelcome = 2,   // the Partner Program welcome
	LetterStatement = 3, // the monthly statement (paid, or rolled over under $100)
	LetterLimited = 4,   // the first Limited ads coin's notice, ever (core-videoalgo, C3: valgo::VaLetterLimited)
};
/** Marks (life::Mark::Kind) Rerun adds to the wall calendar and the second screen. */
enum MarkKind : int
{
	MarkPayout = 40, // "Rerun payout Apr 21"
	MarkReview = 41, // the review's answer
};
/** Studio's PUBLISH: "" when the choice can go up, else why not (a line for the button's tooltip). */
SHORTSTACKCORE_API std::string CanUpload(const Choice& Ch, const std::vector<Candidate>& Open);
/** The thumbnail the choice will get (Studio's preview; Upload stores the same). */
SHORTSTACKCORE_API Thumb ThumbFor(const Choice& Ch, const std::vector<Candidate>& Open);
/** The candidates a choice names, in the choice's order (the lead first for a Recap). */
SHORTSTACKCORE_API std::vector<Candidate> Picked(const Choice& Ch, const std::vector<Candidate>& Open);
/** The edit: world minutes and energy (§4.5's table). */
SHORTSTACKCORE_API double EditMinutes(Format F);
SHORTSTACKCORE_API double EditEnergy(Format F);
/** The kind's topic weight (§4.4), and its label in Studio ("BAD BEAT"). */
SHORTSTACKCORE_API double KindWeight(Kind K);
SHORTSTACKCORE_API const char* KindLabel(Kind K);
SHORTSTACKCORE_API const char* FormatName(Format F);
SHORTSTACKCORE_API const char* TitleStyleName(TitleStyle T);
SHORTSTACKCORE_API const char* FrameName(Frame F);
/** Studio's right-hand column: a line on what each choice does, never a promised number. */
SHORTSTACKCORE_API std::vector<std::string> Advice(const Choice& Ch, const std::vector<Candidate>& Open);
/** The channel at World in closed form (the saved numbers are as of the last day stepped). */
SHORTSTACKCORE_API double SubsAt(const Channel& C, double World);
SHORTSTACKCORE_API double TotalViews(const Channel& C, double World);
/** Views in the 48 hours to World: kast::Inputs::VideoBuzz (§4.7). */
SHORTSTACKCORE_API double Buzz(const Channel& C, double World);
/** The card room's settle (FAutoHost::OnSettle): a DeepRun, FinalTable or Win candidate for the night, when it was one. */
SHORTSTACKCORE_API void NoteLiveRecap(life::State& L, const LiveContext& Ctx, int Place, Chips PrizeCents);

// ------------------------------------------------------------------ C2's additions (gate 2's integration; append-only)

/**
 * Studio's right-hand column with the channel's state: a line about ads only once they run (AdsOn: a Partner, past the
 * review). The two-argument Advice above is this one with AdsOn false.
 */
SHORTSTACKCORE_API std::vector<std::string> Advice(const Choice& Ch, const std::vector<Candidate>& Open, bool AdsOn);
/** Session::Publish's refusals, word for word (Studio greys PUBLISH with them): 1 busy, 2 tables open, 3 mid-stream, 4 too tired. */
SHORTSTACKCORE_API std::string PublishWhy(int Reason);
/** The line after a video is taken down (Session::DeleteVideo's success; the Channel page's note). */
SHORTSTACKCORE_API std::string Deleted();

// ------------------------------------------------------------------ Update 3 (SPEC §7.3; the lead, Phase A)

/**
 * Update 2's impressions x click-through for a choice set, verbatim (§4.4: P_U2, quantised; Upload's own computation,
 * extracted at Phase A). video_main's five vectors upload through it, so video_vectors.txt's upload lines stay pinned.
 */
SHORTSTACKCORE_API double UploadU2(const Channel& C, const Choice& Ch, double World, const std::string& Seed, const UploadFacts& F);
/** The Update 3 lines (vid's appended fields aside): called at the end of WriteLines; pure const; nothing while U3Video is unset. */
SHORTSTACKCORE_API void WriteLinesU3(const life::State& L, std::string& Out);
} // namespace video
} // namespace ss
