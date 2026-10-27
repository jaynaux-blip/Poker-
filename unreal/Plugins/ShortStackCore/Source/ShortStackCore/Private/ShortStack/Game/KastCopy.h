// The creator update (SPEC_CREATORS §5.3, §5.5): every new Kast string, core-kastmath's header (private to the core: Kast.cpp,
// KastPlay.cpp, SessionKast.cpp and UI/RiverLineKastGrowth.cpp include it, the UI as "../Game/KastCopy.h"; the tests by their
// relative path, "../../Source/ShortStackCore/Private/ShortStack/Game/KastCopy.h"). The bodies are Game/KastCopy.cpp's.
//
// The rules (SPEC v1.2 §1.10, SPEC_CREATORS §5.5): the game's voice; every digit a fact of the moment it describes (each
// accessor takes the facts and fills them; nothing is rounded into a different number); no real names or brands (tests/
// Trademarks.h, both tiers); nothing in the world says AI, bot(s), autoplay or autopilot, and "heads-down" is the only pace
// word; Heat is a thermometer (no flame, fire, spark, blaze or burn word); Heat's tier words belong to the meter (chat never
// uses them). Every template is in Entries() (and kast::Inventory(), the same texts), screened by kast_growth's SegPure.
//
// Kinds: Chat lines are chat's own voice (lower case, no closing stop needed), System lines are Kast's sentences in chat,
// Toast is the toast line and alerts, Label a UI label (no closing stop), Letter a letter's subject or body. Chat pools return
// every variant: the caller picks one with the stream's own R (display only, ChatStep) or a keyed draw, and posts it from a
// person who is present (R1).
#pragma once

#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"

#include <array>
#include <string>
#include <vector>

namespace ss
{
namespace kast
{
namespace copy
{
enum class Kind : int
{
	Chat,
	System,
	Toast,
	Label,
	Letter,
};
/** One template: its key ("k3.rail.reached"), its kind, its text with {Vars}. */
struct Entry
{
	std::string Key;
	Kind Type = Kind::Label;
	std::string Text;
};
/** Every template of the creator update's Kast copy, in order (the lint's view; kast::Inventory() is the texts). */
std::vector<Entry> Entries();

// ------------------------------------------------------------------ numbers in copy

/** "$5", "$134", "$5.20", "$1,184.60": whole dollars without cents, otherwise to the cent (never rounded to another number). */
std::string Dollars(Chips Cents);
/** "1,150". */
std::string Number(int N);

// ------------------------------------------------------------------ K1 the Room

/** "12 watching · 5 chatting" (StreamSide's strip, Studio's chat header). */
std::string WatchingChatting(int Watching, int Chatting);
/** "12 watching". */
std::string Watching(int N);
/** "5 chatting". */
std::string Chatting(int N);
/** "2 moderators are watching chat." ("1 moderator is watching chat."). */
std::string ModsWatching(int N);
/** The Mods tab: "2 of 3 here". */
std::string ModsHere(int Here, int Of);
/** StreamSide, under the count, while heads-down and Engage is under 0.15: "heads-down tonight: chat is quieter". */
std::string HeadsDown();
/** The end card: "38 people chatted tonight" ("1 person chatted tonight"). */
std::string ChattedTonight(int N);
/** Studio's preview at a true 0: "nobody here right now". */
std::string NobodyHere();

// ------------------------------------------------------------------ K2 First Faces, First Follower, Doors open, Set the scene

/** The go-live line (Doors open): "You're live! Your ping reached 412 of 640 followers." (one follower, none: their own). */
std::string GoLive(int Reached, int Of);
/** The night after 10+ new followers: "You're live! Your ping reached 214 of 290 · 31 followed you last night." */
std::string GoLiveAfterglow(int Reached, int Of, int FollowedLastNight);
/** A go-live inside 4 world hours of the last ping (nobody new is pinged): "You're live again. Kast pings your followers once
 *  every 4 hours." */
std::string GoLiveNoPing();
/** Studio's toast: "Your ping reached 412 of 640 followers (64%)". */
std::string PingToast(int Reached, int Of);
/** Dee's first line on stream 1, at world minute 0.25: "LIVE!! the laundromat is watching". */
std::string DeeHi();
/** Mei's, just after: "mei is here". */
std::string MeiHi();
/** Dee, present, on the first stream within 48 world hours of a live night that cashed: "how was the Embercrest??". */
std::string DeeEmbercrest();
/** First Follower (stream 1): "nightrail_88 followed: your first follower who isn't family" (with KastShine). */
std::string FirstFollower(const std::string& Who);
/** The alert's line under the name: "your first follower who isn't family". */
std::string FirstFollowerSub();
/** Chat's reaction to it (chat, from someone present): "FIRST FOLLOWER kastPog". */
const std::vector<std::string>& FirstFollowerChat();
/** R6's empty-room system lines, at most one every 3 real minutes, every number the model's. "Your stream is #41 of 49 live
 *  in Poker." */
std::string EmptyRank(int Rank, int Of);
/** "41 people saw your stream in the directory in the last 30 minutes; 2 came in." (impressions A / DirCtr over the window). */
std::string EmptySeen(int Saw, int Came);
/** The same fact as a compact label (Studio's tile): "41 saw you in the directory". */
std::string SawYou(int Saw);
/** "New on Kast: you're on the Fresh shelf tonight." (streams 1-12). */
std::string EmptyFresh();
/** "Rising in Poker: Kast shows small channels to new viewers." (while the usual is under 15). */
std::string EmptyRising();
/** Set the scene's button (streams 1-20, a true 0, attended, once a stream): "Set the scene". */
std::string SetTheSceneButton();
/** What it does, as a system line: "You talk to the camera about tonight's tournament." */
std::string SetTheScene();

// ------------------------------------------------------------------ K3 the Ping, Run Alerts, Rail Call

/** The Community page: "Ping reach last stream 64%". */
std::string PingReach(int Pct);
/** The Community page's heading: "THE PING". */
std::string PingTitle();
/** The three-part bar's parts (Summary::PingParts' order): "this week", "this month", "lapsed". */
std::string PingPart(int Part);
/** The Run Alert's system line: "RUN ALERT sent to 256 followers". */
std::string RunAlertSent(int N);
/** StreamSide's counter as they arrive: "12 back so far". */
std::string RunAlertBack(int N);
/** StreamSide's strip, both: "RUN ALERT · 12 back so far". */
std::string RunAlertStrip(int Back);
/** A returner's line (chat, from the member who came back), by the stage that pulled them: "back for the FT!". */
const std::vector<std::string>& RunAlertReturn(growth::RunStage S);
/** The action slot's button: "RAIL CALL". */
std::string RailCallButton();
/** After the press: "Your rail call reached 37 followers." */
std::string RailCallReached(int N);
/** A Rail Call arrival's line (chat, from the follower who came): "rail call got me". */
const std::vector<std::string>& RailCallArrive();
/** Dee's Rail Call arrival: "omw!!". */
std::string DeeOmw();
/** Mei's: "mei on the rail". */
std::string MeiRail();
/** Every follower is already here: "Your whole rail is here". */
std::string WholeRailHere();

// ------------------------------------------------------------------ K4 Heat, Front Row, Stamina

/** The meter's tier words (growth::Tier): "Cold", "Warming up", "Running hot", "On a heater", "Sun run". */
std::string TierName(int Tier);
/** The meter's label: "HEAT". */
std::string HeatTitle();
/** The ghost fill while live: "tonight +18 so far" ("tonight -4 so far"). Gain is rounded to the whole number shown. */
std::string GhostTonight(double Gain);
/** Tile 4's sub line before the night has settled (the first 15 world minutes, and until the forecast has a fair read):
 *  "tonight: settling in". */
std::string GhostSettling();
/** The 24-inch Studio: "12 watching · Running hot". */
std::string WatchingTier(int Watching, int Tier);
/** The Community page: "Heat 41 · lifts the directory, turnout and your ping". */
std::string HeatEffects(int Heat);
/** The Community page: "Cold means you went dark 7+ days: while you stream every week it never reads Cold." */
std::string ColdMeans();
/** Why lines (the top two, each a fact). A deep run's news: by stage and how long ago. "Last night's final table (1,150
 *  runners)", "Tonight's final table (980 runners)", "Tuesday's win (5,000 runners)". DaysAgo 0 tonight, 1 last night, 2-6 the
 *  weekday's name (DayName, net::WeekdayName(Day, true)). */
std::string WhyRun(growth::RunStage S, int Runners, int DaysAgo, const std::string& DayName);
/** "38% over your forecast". */
std::string WhyOverForecast(int Pct);
/** "21% under your forecast". */
std::string WhyUnderForecast(int Pct);
/** "Showing up: 4 nights this week" ("1 night"). */
std::string WhyShowingUp(int Nights);
/** "Cooling: 9 days dark" ("1 day dark"). */
std::string WhyCooling(int DaysDark);
/** "Your clip hit the top of the Kast feed". */
std::string WhyClipTop();
/** Rerun's bumps: "Your video kings, 82%… got pushed hard" (a breakout: "…broke out"); the title shortened to 16 characters. */
std::string WhyVideo(const std::string& Title, bool Breakout);
/** The week's cap reached: "Hot week: Heat can't climb any faster this week". */
std::string WhyHotWeek();
/** The record peak's bump: "Your biggest room yet (41 at the peak)". */
std::string WhyRecordPeak(int Peak);
/** A raid of 5+ that was 20% of the room: "kat_v's raid of 14". */
std::string WhyRaid(const std::string& From, int Size);
/** A follower milestone's bump: "250 followers". */
std::string WhyMilestone(int Followers);
/** Nights near the forecast with nothing else to say: "Steady: nights near your forecast". */
std::string WhySteady();
/** A new channel's first streams: "New channel: Kast is getting to know you". */
std::string WhyNew();
/** A deep run without its field's size (off air: the channel keeps the final-table days, FtDays): "Last night's final table",
 *  "Tonight's final table", "Tuesday's final table" (DaysAgo and DayName as WhyRun's). The C-K PIE fix (F4). */
std::string WhyRunPlain(growth::RunStage S, int DaysAgo, const std::string& DayName);
/** A night under 60 world minutes that moved no Heat says why (the end card, C-K PIE F4): no deep run's news on it, "Under an
 *  hour: only a deep run counts" (beside the card's Heat move); ... */
std::string WhyShort();
/** ...its run's news already counted on an earlier stream of the same tournament: "This run's Heat already counted"; ... */
std::string WhyShortCounted();
/** ...its run no deeper than the channel's usual night (RunNorm): "Runs this deep are your usual now"; ... */
std::string WhyShortUsual();
/** ...no armed stage on stream while the deepest run at the stop is in the money (its stages came before the go-live, as at a
 *  final table that had formed): "Stages before going live don't count" (review-pie-room); ... */
std::string WhyShortBefore();
/** ...and Heat at the soft ceiling (§2.4: a night adds nothing at 90): "Heat is at its ceiling" (the week's cap: WhyHotWeek). */
std::string WhyCeiling();
/** Heat's why lines off air, the top two ("" where there's none), each a fact the channel holds, in this order: a final table on
 *  stream in the last 6 world days (WhyRunPlain, from FtDays: the C-K PIE fix, F4), "Cooling: N days
 *  dark" from 2 world days dark, "Hot week: ..." once this world week's gains reach §2.4's cap (35), "Showing up: N nights this
 *  week", and "New channel: ..." for a channel in its first 3 streams with nothing else to say. Studio's Heat tile off air and
 *  ss.Kast.Describe read the same lines. LastStreamDay is the channel's own (Ch.LastStreamDay), or for an old save before its
 *  first stream under this build the day its go-live will seed (SessionKast.cpp's KastHeatAtGoLive, the C-K gate's L8). */
std::array<std::string, 2> WhyOffline(const Channel& Ch, int LastStreamDay, double World);
/** The tier-up burst at the commit into Running hot or higher (chat; never a tier word): "chat is cooking". */
const std::vector<std::string>& TierUpChat();
/** Front Row (On a heater, 30% of streams, 20 world minutes): "Kast put you in Front Row in Poker." */
std::string FrontRow();
/** Stamina's chip and Community row (from 24 world hours live in 7 days): "Stamina: 38 hours live this week. Chat can tell." */
std::string StaminaLine(int Hours);
/** Its heading: "STAMINA". */
std::string StaminaTitle();
/** Its note: "Shown from 24 hours live in 7 days. A day off brings it back." */
std::string StaminaNote();

// ------------------------------------------------------------------ K5 the Sweat, Event Mode

/** The EVENT bar across the preview (the last two tables of an armed run): "FINAL TABLE · $11 Big Stack · 7 left of 980"
 *  ("LAST TWO TABLES · ..." above the final table). Event is the run's display name (Inputs::DeepEvent). */
std::string EventBar(bool FinalTable, const std::string& Event, int Left, int Of);
/** The directory card's chip: "FINAL TABLE". */
std::string FinalTableChip();
/** The directory card's line: "FINAL TABLE · $3.30 Headhunter PKO · 6 left". */
std::string DirectoryEvent(const std::string& Event, int Left);
/** Chat sweating the spot (Event Mode): "ICM spot here", "fold fold fold", "THIS IS IT". */
const std::vector<std::string>& SweatChat();
/** The Browse page's shelf: "Sweating now". */
std::string SweatingNowShelf();
/** Its line: "streams at a deep run, live". */
std::string SweatingNowSub();

// ------------------------------------------------------------------ K6 the Broadcast, big names

/** The site's own raid at a final table with P >= 1, in its voice (net::SiteId): "RiverLine Live sent 41 viewers: one of our
 *  final tablists is streaming." · "Ignite's Bandstand sent 41 viewers." · "RunItTwice's Newsdesk sent 41 viewers." */
std::string Broadcast(int Site, int Viewers);
/** The "Sweating now" shelf's tease: "RiverLine Live features final tables from $5". */
std::string BroadcastTease(int Site);
/** A Broadcast raider's line (chat): "RiverLine Live brought me, gl". */
const std::vector<std::string>& BroadcastChat(int Site);
/** A raid's alert: "VikingVolta raided with 312". */
std::string RaidedWith(const std::string& From, int Size);

// ------------------------------------------------------------------ K7 Fresh on Kast, Rising in Poker, Poker Rank

/** Your Browse card's chips: "NEW ON KAST" (streams 1-12), "RISING IN POKER" (the usual under 15). */
std::string NewOnKastChip();
std::string RisingChip();
/** The Browse shelves: "Fresh on Kast" / "new channels: Kast shows them to new viewers"; "Rising in Poker" / "small channels:
 *  Kast shows them to new viewers". */
std::string FreshShelf();
std::string FreshShelfSub();
std::string RisingShelf();
std::string RisingShelfSub();
/** The header chip while live: "#23 of 49 in Poker". */
std::string RankHeader(int Rank, int Of);
/** Browse: "You're #8 in Poker right now". */
std::string RankNow(int Rank);
/** The end card: "You climbed from #31 to #6 in Poker tonight" (no climb: "Best tonight: #6 in Poker"). */
std::string RankClimb(int From, int Best);

// ------------------------------------------------------------------ K8 Where They Came From

/** The VIEWERS tile's short word for a source: "returning", "directory", "fresh", "rising", "railbirds", "raid", "broadcast",
 *  "word of mouth", "video", "clip", "lurking" ("5 returning · 4 directory · 2 raid · 1 video"). */
std::string SourceTag(View::Source S);
/** The end card's phrase for a source: "the ping", "the directory", "Fresh on Kast", "Rising in Poker", "railbirds", "raids",
 *  "the Broadcast", "word of mouth", "your video", "your clip", "lurkers" ("Came because of the ping 23 · the directory 9"). */
std::string SourceCause(View::Source S);
/** "5 returning" (the tile's pairs). */
std::string SourceCount(View::Source S, int N);
/** The end card's headings: "Came because of", "Followed:". */
std::string CameBecauseOf();
std::string FollowedFrom();
/** A guest from Rerun says so (chat): "came from the kings video". */
std::string CameFromVideo(const std::string& Title);
/** A guest from a clip (chat): "came from your clip". */
std::string CameFromClip();

// ------------------------------------------------------------------ K9 the Forecast

/** Under Go live: "Tonight: likely 8-14" (Lo and Hi rounded to the whole viewers shown). */
std::string ForecastBand(double Lo, double Hi);
/** Its second line: "Tuesday · on schedule · Running hot". Schedule: 0 none posted, 1 on schedule, 2 off schedule. */
std::string ForecastContext(const std::string& DayName, int Schedule, int Tier);
/** The small label: "forecast 41-63". */
std::string ForecastShort(double Lo, double Hi);
/** The end card: "above your forecast" / "inside your forecast" / "under your forecast" (Avg against the band's Lo and Hi). */
std::string ForecastVerdict(double Avg, const growth::Band& Fc);

// ------------------------------------------------------------------ K10 raids

/** The action slot's button for the raid's first 2 world minutes: "WELCOME THE RAID". */
std::string WelcomeTheRaidButton();
/** The welcome raid (the first stream at or after stream 3 that reaches world minute 40): "pocketqueens_pk raided with 8:
 *  welcome to Kast!" */
std::string WelcomeRaid(const std::string& From, int Size);
/** A welcomed raider's thanks (chat). */
const std::vector<std::string>& WelcomedChat();
/** The streamer's own line when they press it (Streamer line): "Welcome in, raiders! Grab a seat on the rail." */
std::string WelcomeRaidStreamer();
/** The Community page: "3 channels owe you a raid" ("1 channel owes you a raid"). */
std::string RaidsOwed(int N);
/** The Raids tile: "2 raids owed" ("1 raid owed"). */
std::string RaidsOwedShort(int N);
/** "You raided pocket_aces_pk 3 days ago (no new token until Friday)". DaysAgo 0: "today", 1: "yesterday"; NextDay the weekday
 *  a new token comes (the 7-day rule). */
std::string RaidedAgo(const std::string& Who, int DaysAgo, const std::string& NextDay);
/** The token rule: "A raid only earns a token after an hour live, and once a week per channel. They come back when they come
 *  back." */
std::string TokenRule();
/** The end card's retention: "kat_v raided with 14 · still here at 15 min: 6 · followed: 2". */
std::string RaidRetention(const std::string& From, int Size, int At15, int Followed);
/** The same for the site's Broadcast (Summary::RaidHeld::Broadcast; From is the site's voice, as K6's alert):
 *  "RiverLine Live sent 41 · still here at 15 min: 18 · followed: 3". */
std::string BroadcastRetention(const std::string& From, int Size, int At15, int Followed);

// ------------------------------------------------------------------ K11 Thank, Answer, Nod

/** The chatter menu's new item: "Nod". */
std::string NodButton();
/** "2 Nods left" ("1 Nod left", "No Nods left"). */
std::string NodsLeft(int N);
/** A Nodded chatter's reply (chat): "he noticed me kastLove", "got noticed!!". */
const std::vector<std::string>& NodReply();
/** Thank chat's label after two in ten world minutes: "(chat heard you)". */
std::string ThankHeard();
/** An answered guest who follows (chat): "thanks for answering! followed". */
const std::vector<std::string>& AnswerFollowed();

// ------------------------------------------------------------------ K12 Hold or Crack

/** StreamSide's strip: "HOLD OR CRACK?". */
std::string HoldOrCrackTitle();
/** Under 10 callers, counts: "2 HOLDS · 1 CRACKS". */
std::string HoldOrCrackCounts(int Holds, int Cracks);
/** From 10 callers, shares: "HOLDS 71% · CRACKS 29%". */
std::string HoldOrCrackShares(int Holds, int Cracks);
/** The marker: "your equity 64%". */
std::string YourEquity(double Equity);
/** The two buttons: "Call it: HOLDS", "Call it: CRACKS". */
std::string CallIt(bool Holds);
/** The streamer's own call in chat (Streamer line): "HOLDS. calling it" / "CRACKS. calling it". */
std::string StreamerCall(bool Holds);
/** Chat's calls while it's open (chat, from present chatters): HOLDS or CRACKS. */
const std::vector<std::string>& ChatCalls(bool Holds);
/** The reveal: "71% of chat called it." (the share who were right; system line); a reveal most called wrong: "the rail is in
 *  shambles" (chat). */
std::string RevealRight(int Pct);
const std::vector<std::string>& RevealWrong();
/** The reveal's system line with the points: "HOLDS! 71% of chat called it: 340 points." (from 10 callers), "CRACKS! 3 of 4
 *  called it: 120 points." (under 10), "HOLDS! Nobody in chat called it." (none right). */
std::string RevealLine(bool Held, int Right, int Of, int Points);
/** The callers who were right (chat): "called it!"; the ones who weren't: "pain". */
const std::vector<std::string>& CalledItChat();
const std::vector<std::string>& MissedCallChat();
/** The end card: "kat_v called 4 of 5". */
std::string BestCaller(const std::string& Who, int Calls, int Of);

// ------------------------------------------------------------------ K15 Clip Lane, Clip of the Night

/** The end card's clip slot: "Clip of the night: “river nine” · in the Kast feed" (unlisted: "· on your channel"). */
std::string ClipOfTheNight(const std::string& Title, bool InFeed);
/** Its buttons: "Clip it", "Make it a Cut". */
std::string ClipItButton();
std::string MakeItACutButton();
/** A top-of-feed clip's toast: "Your clip is at the top of the Kast feed". */
std::string ClipTopOfFeed();

// ------------------------------------------------------------------ K16 Welcome back, the streak

/** The first stream after 5+ dark days: "Welcome back: 14 familiar faces checked in." ("1 familiar face"). */
std::string WelcomeBack(int N);
/** The Community page's streak row: "Streak 6 weeks" ("Streak 1 week"). */
std::string StreakRow(int Weeks);

// ------------------------------------------------------------------ K17 the end card, stings, the letter

/** Peak first: "Peak 41 at the final table, 9:48 PM" (Moment the one that brought the peak, PeakRaid: "Peak 30 on a raid, 9:48
 *  PM", or -1: "Peak 12 at 9:48 PM"). Clock is ClockString's "9:48 PM". */
std::string PeakLine(int Peak, int MomentAtPeak, const std::string& Clock);
/** "+46 · best night ever" (Best) or "+46". */
std::string NewFollowersLine(int N, bool Best);
/** The roll call: members present for the whole run. "kat_v, nitbox and 3 more sweated the whole run" (one: "kat_v sweated
 *  the whole run"; two: "kat_v and nitbox ..."); "" when there are none. */
std::string RollCall(const std::vector<std::string>& Names);
/** Heat's move: "Warming up → Running hot · +18" (the same tier: "Running hot · +6"; a fall: "Running hot · -4"). */
std::string HeatMove(int FromTier, int ToTier, double Gain);
/** Tonight's money apart from renewals: "Tonight: tips $24 · cheers $11" (no cheers before Affiliate: "Tonight: tips $24"). */
std::string TipsTonight(Chips Tips, Chips Cheers, bool Affiliate);
/** "While you were away: renewals $38". */
std::string RenewalsAway(Chips Cents);
/** Before Affiliate, a tip by name instead of bars of cents: "Dee tipped $5". */
std::string Tipped(const std::string& Who, Chips Cents);
/** Before the second rent: "pending: $86, paid after your next rent". */
std::string PendingLine(Chips Cents);
/** Without the sum: "pending: paid after your next rent". */
std::string PendingNote();
/** The next goal under its bar: "88 to 5,000 followers" ("1 to 25 followers"). */
std::string NextGoal(int ToGo, int Goal);
/** The first payout after the second rent (the toast; the ledger line stays "Kast payout"): "Your first Kast payout: $134". */
std::string FirstPayout(Chips Cents);
/** C13's ledger labels (LedgerSponsors, the amount in its own column): "TunnelRat: stream read", "TunnelRat: 6 hours on
 *  stream" ("1 hour"). */
std::string SponsorReadLabel(const std::string& Brand);
std::string SponsorHoursLabel(const std::string& Brand, int Hours);
/** Studio's read button once today's read was paid: "TunnelRat: read done today". */
std::string SponsorReadDone(const std::string& Brand);
/** KastAfterStop's LedgerSponsors labels (core-kastglue): "TunnelRat: stream read" ("TunnelRat: 2 stream reads" when a stream
 *  across midnight paid two) and "TunnelRat: live hours". */
std::string LedgerRead(const std::string& Brand, int Reads);
std::string LedgerHours(const std::string& Brand);
/** The milestone stings' notice bodies (Notice::Milestone, KastShine; core-kastglue's at Stop): "25 followers!", "Your first
 *  tip from someone new!", "Your first raid!", "You're a Kast Affiliate!", "You're a Kast Partner!". */
std::string StingFollowers(int Step);
std::string StingStrangerTip();
std::string StingFirstRaid();
std::string StingAffiliate();
std::string StingPartner();
/** The end card's "Done". */
std::string DoneButton();
/** The milestone stings (KastShine): "Milestone: 5 followers!" (the existing line's form). */
std::string MilestoneFollowers(int N);
/** The first stranger's tip (growth::FirstStrangerTip): "First tip from someone new: lucia.mm sent $2!" */
std::string FirstStrangerTip(const std::string& Who, Chips Cents);
/** The first raid in: "Your first raid: freeroll_fritz brought 3!" */
std::string FirstRaidIn(const std::string& From, int Size);
/** Whether a line is FirstStrangerTip's (Summary::Firsts holds the firsts the Room posted live): the template up to its first
 *  fact. */
bool IsFirstStrangerTip(const std::string& Line);
/** Whether a line is FirstRaidIn's. */
bool IsFirstRaidIn(const std::string& Line);
/** The one-time Kast letter (C5) at an old channel's first stream under this build: its sender, subject and body. */
std::string LetterFrom();
std::string LetterSubject();
std::string LetterBody();

// ------------------------------------------------------------------ K20 Kast money (the Community page)

/** "Sponsor pay is its own ledger line, never part of the Kast payout." */
std::string SponsorApart();
/** "Partner's average counts your own viewers: raids and the Broadcast don't count." */
std::string PartnerAverage();
/** C8's bar: "25 followers · 240 minutes · 4 days live · an average of 3, over 30 days". */
std::string AffiliateBar(int Followers, int Minutes, int Days, int Average);
/** "Dee and Mei subscribed the night it landed." */
std::string AffiliateFriends();
/** The followers tile before Affiliate: "2 to go: Affiliate at 25". */
std::string AffiliateToGo(int ToGo, int At);
} // namespace copy
} // namespace kast
} // namespace ss
