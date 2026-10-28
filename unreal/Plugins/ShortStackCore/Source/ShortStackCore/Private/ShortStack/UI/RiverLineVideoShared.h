// Rerun's shared look, side state and helpers (update 3 SPEC §6.6, §7.1, §8.2: the split of RiverLineVideo.cpp, a no-change
// move). RiverLineVideo.cpp keeps the app's shell, the header, the bell and Channel; RiverLineVideoStudio.cpp has Studio (and
// Rerun 2.0's launch card next); RiverLineVideoStats.cpp (Analytics) and RiverLineVideoHome.cpp (Home, Next up) share
// these. The look (marigold on ink), the copy, VideoUi (one laptop's Rerun, kept beside the class by laptop) and RrFrame are
// defined here; the helpers' bodies stay in RiverLineVideo.cpp. ui-rerun owns this header after Phase A.
//
// Rerun 2.0 (C3, ui-rerun): four pages (0 Studio, 1 Channel, 2 Analytics, 3 Home: the indices callers use, Mail opens 1);
// every number on them is the core's (VideoStats.h's contract, read through the caches below: a video's numbers by its id,
// its stepped day, its parts and the world minute; the channel's by the world minute or day), and every line of copy the
// core's (VideoAlgoPriv.h's launch and Studio lines, the bell, Insights, the log) or this file's (VideoUiInventory, linted).
// Thumbnails: a recipe through ui-thumbs' compositor, cached in Rerun's own pool and painted at most two uncached a frame
// (a skeleton until its turn, known through Canvas::IsCached); an Update 2 video's frame as Update 2 drew it.
#pragma once

#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/UI/ChartArt.h"
#include "ShortStack/UI/ThumbArt.h"
#include "ShortStack/UI/VideoArt.h"

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlvideo_detail
{
const float VideoH = 962.0f; // above the taskbar
const float VideoTop = 60.0f;

// Rerun's look (§1.8): marigold on ink, cream text; clear of Kast's violet and lime, RiverLine's teal and a platform's red.
const Color RrGround = Hex(videoart::Ink);
const Color RrBar = Hex(0x0f1114);
const Color RrPanel = Hex(0x1f2229);
const Color RrPanel2 = Hex(0x2a2e37);
const Color RrLine = Hex(0x363b47);
const Color RrCream = Hex(videoart::Cream);
const Color RrMuted = Hex(0xb0a794);
const Color RrDim = Hex(0x767064);
const Color RrGold = Hex(videoart::Marigold);
const Color RrWarn = Hex(0xf39a6b);
const Color RrSky = Hex(0x8ccdfa);
const Color RrCash = Hex(0xffd479);
// Rerun 2.0 (the mockups' GOOD and BAD): a gain, a like; a loss, a dislike (always beside a word or a vector mark).
const Color RrGood = Hex(0x7fd99a);
const Color RrBad = Hex(0xf06a6a);

// The pages (VideoOpen's indices; Mail opens 1, RunItTwice's Desk and Kast's Cut highlights 0).
constexpr int RrPageStudio = 0;
constexpr int RrPageChannel = 1;
constexpr int RrPageAnalytics = 2;
constexpr int RrPageHome = 3;
constexpr int RrPageCount = 4;
constexpr int RrPageSheet = 9; // ss.Video.Sheet's contact sheet (VT1: a testing page, no tab)

// Studio's three columns (update 3 SPEC §6.6.2: the list (20, 72, 360, 878), the editor (396, 72, 764, 878), the side
// (1176, 72, 404, 878)).
const float RrListX = 20.0f;
const float RrListW = 360.0f;
const float RrListTop = 120.0f;
const float RrRowH = 86.0f;
const float RrRowGap = 8.0f;
const float RrEdX = 396.0f;
const float RrEdW = 764.0f;
const float RrSideX = 1176.0f;
const float RrSideW = 404.0f;
// The thumbnail preview (a 16:9 box beside the six options) and the six at 120 x 68 (§6.6.2).
const float RrThumbW = 504.0f;
const float RrThumbH = 283.5f;
const float RrOptW = 120.0f;
const float RrOptH = 67.5f;
// The Channel's grid (§6.6.6: 5 columns of 300 x 169 tiles).
const int RrCols = 5;
const float RrTileW = 300.0f;
const float RrTileGap = 15.0f;
const float RrTilePitch = 252.0f;

// The copy these screens add to core's (VideoCopy.cpp's titles, advice, letters and bell; VideoAlgoPriv.h's launch and
// Studio lines). Sentences are in VideoUiInventory (the copy lint).
constexpr const char* RrStudioEmpty = "Play some hands. The good ones show up here.";
constexpr const char* RrStudioEmptyMore = "Bad beats, bluffs, big pots and deep runs from your own tables wait here for three weeks.";
constexpr const char* RrChannelEmpty = "No videos yet.";
constexpr const char* RrChannelEmptyMore = "Studio turns your best hands into videos.";
constexpr const char* RrPickFirst = "Pick a moment on the left.";
constexpr const char* RrAutoNote = "Your plan played this one. The video never says so.";
constexpr const char* RrTonightGone = "Nothing left from tonight's stream.";
constexpr const char* RrVlogFull = "A Vlog holds eight moments at most.";
constexpr const char* RrRecapFull = "A Recap holds the run and six of its hands at most.";
constexpr const char* RrDeleteAgain = "Press again to take it down.";
constexpr const char* RrHoursNote = "Long videos count toward watch hours. Cuts bring subscribers, not hours.";
constexpr const char* RrGateNext = "Fill both bars and Rerun takes a look.";
constexpr const char* RrUnderReview = "Rerun's looking your channel over. Word comes by Mail in about a week.";
constexpr const char* RrPartnerHead = "You're a Rerun Partner.";
constexpr const char* RrPartnerSince = "Ads run on every video from that day on. Views from before it never earn.";
constexpr const char* RrAdsAtGate = "Ads start at the gate.";
constexpr const char* RrAdsAfterReview = "Ads start when the review's done."; // both bars full: the gate's passed, the week's review runs
constexpr const char* RrPaysOut = "Pays out on the 21st once it reaches $100.";
constexpr const char* RrBellEmpty = "Nothing yet. Milestones land here as your videos grow.";
// Rerun 2.0 (C3).
constexpr const char* RrIdeasEmpty = "Ideas come from your own career. Play, stream and live a little: they show up here.";
constexpr const char* RrSeriesEmpty = "No series yet. A challenge from IDEAS starts one, and each episode links to the last.";
constexpr const char* RrCutNoThumb = "A Cut plays in the feed: it has no thumbnail.";
constexpr const char* RrDuelNeedsNew = "The thumbnail test needs two of the new thumbnails.";
constexpr const char* RrDuelHelp = "Pick up to three. Rerun shows each to some viewers for two days, then keeps the one people watched longest.";
constexpr const char* RrEarlyHint = "Viewers who arrive early count.";
constexpr const char* RrDislikesPrivate = "dislikes are only visible to you";
constexpr const char* RrAdsBeforePartner = "Ads start when you join the Partner Program.";
constexpr const char* RrNotPaid = "Views before the gate are never paid.";
constexpr const char* RrLegacyRow = "Published before Rerun's new analytics";
constexpr const char* RrNoTrends = "Nothing's trending in poker this week yet.";
constexpr const char* RrHelpCard = "Rerun shows a new video to your subscribers first, then to a small group of poker viewers on Home. If they click and keep watching, it decides at the next midnight after two days how much further to push it, and it checks again a week later. Good videos also turn up next to other videos and in search for months. Nobody can promise a number, but people who click and stay are the best sign.";
constexpr const char* RrSeasonLine = "Advertisers spend more in November and December. January is quiet.";
constexpr const char* RrHomeEmpty = "Nothing on Home for you right now. A video shows up here while Rerun is testing it on Home.";
constexpr const char* RrSearchNote = "Search viewers find a lesson for months.";
constexpr const char* RrChangeOnce = "You can change a video's ads once a week. The change starts at the next midnight.";
constexpr const char* RrAgeNote = "Rerun shows poker to adults.";

/** A video's numbers kept between frames (StatsOf), keyed by what moves them. */
struct RrStatsEntry
{
	video::VideoStats S;
	double Stepped = -1.0, Minute = -1.0;
	size_t Comps = 0;
	int Ads = -1, Launch = -1, Repacks = -1;
};

/** One laptop's Rerun. */
struct VideoUi
{
	int Page = 0; // 0 Studio, 1 Channel, 2 Analytics, 3 Home
	double PageAt = -100.0;
	double LastDrawn = -100.0;
	// Studio: the choice being built.
	std::vector<std::string> Pick;
	bool Touched = false; // the player has picked by hand (an emptied pick stays empty)
	video::Format F = video::Format::Breakdown;
	video::TitleStyle T = video::TitleStyle::Story;
	video::Frame Fr = video::Frame::Reaction;
	// Kast's Cut highlights: the list shows tonight's stream only, until cleared.
	bool Tonight = false;
	bool KeepTonight = false; // set by VideoOpenTonight: the next first frame keeps the filter
	double TonightFrom = 0.0, TonightTo = 0.0;
	float ListScroll = 0.0f, ListGoal = 0.0f;
	std::string Note;
	bool NoteBad = false;
	double NoteAt = -100.0;
	// Channel.
	float GridScroll = 0.0f, GridGoal = 0.0f;
	video::ChannelView View; // the Channel page's numbers, as of ViewMinute (RrChannel)
	bool ViewOk = false;
	double ViewMinute = 0.0, ViewStepped = 0.0, ViewSubs = 0.0, ViewPartner = 0.0, ViewHours = 0.0;
	size_t ViewVideos = 0;
	Chips ViewBalance = 0;
	std::string Confirm; // the video whose Delete was pressed once
	std::string StrikeArmed; // update 2 §5.7: the pick whose PUBLISH has warned of a clause strike once
	double ConfirmAt = -100.0;
	// The bell.
	bool BellOpen = false;
	std::set<std::string> BellFresh; // unseen when the bell opened (they keep their dot while it's open)
	// What was new since the last visit (the taskbar's dot).
	bool Primed = false;
	std::set<std::string> SeenCandidates;
	std::set<std::string> SeenBell;
	std::set<int> SeenIdeas; // trending ideas seen (§6.6.5: a new trending idea lights the dot)
	// C3 fix round (L9, update 3 §4.1): Studio opens on IDEAS when a new or trending idea waits, else on MOMENTS. A visit
	// starts when Studio draws after another page or after the app was away; a caller that means a tab says so (OpenTab).
	bool StudioShown = false;  // Studio drew last frame (the same visit)
	int OpenTab = -1;          // the tab the next visit opens on (Research's links, Kast's Cut highlights); -1: §4.1's rule
	std::set<int> ReadySeen;   // ready ideas the IDEAS list has shown: the session's saved mark (C3 PIE fix F2), read each visit
	std::set<int> NewIdeas;    // this visit's new ready ideas (their NEW chip)

	// ---- Rerun 2.0 (C3)
	// Studio: the list's tab, the packaging and the settings (UI state only; the Choice carries them to PUBLISH).
	int ListTab = 0; // 0 MOMENTS, 1 IDEAS, 2 SERIES
	video::LengthOpt Len = video::LengthOpt::Standard;
	video::AdLoad Ads = video::AdLoad::Standard;
	int TitleShuffle = 0, TitlePick = 0;
	int ThumbShuffle = 0, ThumbPick = 0;
	bool DuelOn = false;
	std::vector<int> Duel; // the options in the test (2-3), in pick order
	bool CleanEdit = false;
	std::string NextUp; // a kept video's id ("" none)
	bool LinkNewest = false; // Kast's "Make a Cut and link it": the next Studio frame links the newest long video
	int Series = -1;
	bool LaunchHidden = false; // the editor shown while a launch card runs
	std::string LaunchSeen;    // the launch the card last showed (a new publish shows its card)
	std::string OptKey;        // the choice the options below are for
	float OptTop = 0.0f;       // where the six options' grid starts this frame (below a day's verdict line when it shows)
	std::vector<video::TitleOption> Titles;
	std::vector<video::ThumbSpec> Thumbs;
	std::string ReadKey;            // the choice (and the world hour) the packaging read below is for
	std::vector<std::string> Read;  // video::PackagingRead's lines (it deals the titles again: not every frame)
	// C3 fix round (M1, §6.6.2): the CAPTION, FACE and PALETTE pickers edit the selected recipe in place (Thumbs[ThumbPick])
	// within its archetype's values (video::ThumbChoicesOf / ThumbEdit). The six are dealt again only when their own deal
	// moves (the pick, the format, the thumbnails' shuffle), so a title's style or shuffle keeps the edits.
	std::string ThumbKey;           // the deal Thumbs holds
	std::string PicksKey;           // the recipe the allowed values below are for
	video::ThumbChoices Picks;
	int CapFrom = 0;                // the caption row's first chip when the row can't hold them all
	std::vector<video::IdeaCard> Ideas;
	double IdeasMinute = -1.0;
	size_t IdeasOpen = 0;
	// The launch card's comparison with the channel's typical upload at the same age (the last 10 uploads' own closed
	// forms), refreshed once a world minute.
	std::string LaunchId;
	double LaunchMinute = -1.0;
	std::vector<double> LaunchHours, LaunchTypical; // views an hour since the drop, and the typical upload's
	double TypViews = 0.0, TypCtr = 0.0, TypAvd = 0.0;
	int LaunchRank = 0, LaunchOf = 0;
	// One video's page: its days against the typical upload's and its rank at its age, once a world minute.
	std::string DailyId;
	double DailyMinute = -1.0;
	std::vector<double> DailyMine, DailyTyp, DailyImpr;
	int OneRank = 0, OneOf = 0;
	double OneTypical = 0.0;
	// Analytics: the sub-page, the window, the selected tile, one video's page, the table's order and scroll.
	int Sub = 0;      // 0 Overview, 1 Content, 2 Audience, 3 Revenue, 4 Realtime, 5 Research
	int RangeIx = 1;  // 0 7 d, 1 28 d, 2 90 d, 3 365 d, 4 Lifetime
	int KpiSel = 0;   // the Overview tile the chart follows
	std::string One;  // the video whose page is open ("" the table)
	int OneTab = 0;   // 0 Overview, 1 Reach, 2 Engagement, 3 Revenue
	int SortCol = -1; // -1 newest first
	bool SortUp = false;
	// C3 gate (update 3 §4.6.7): one video's CHANGE THE PACKAGING card: its options (dealt again when the video, its repackages
	// or the shuffle move) and the last answer Session::Repackage gave.
	std::string RepackKey, RepackSaid;
	std::vector<video::TitleOption> RepackT;
	std::vector<video::ThumbSpec> RepackS;
	int RepackShuffle = 0;
	bool Manage = false; // Revenue's MANAGE list
	float TableScroll = 0.0f, TableGoal = 0.0f;
	float OneScroll = 0.0f, OneGoal = 0.0f;
	// Channel: the tab and the sort; Home: the chip.
	int ChanTab = 0;  // 0 VIDEOS, 1 CUTS, 2 SERIES, 3 ABOUT
	int ChanSort = 0; // 0 Latest, 1 Popular, 2 Oldest
	int HomeChip = 0;
	float HomeScroll = 0.0f, HomeGoal = 0.0f;
	// ss.Video.Sheet's contact sheet (VT1): the recipes the library dealt for a kind group.
	std::vector<video::ThumbSpec> Sheet;
	std::string SheetGroup;
	// The caches (§5.10): a video's numbers by its id; the channel's by the world minute or day and its change keys.
	std::map<std::string, RrStatsEntry> Stats;
	int StatsBudget = 0; // refreshes left this frame (a table refreshes a few rows a frame)
	video::Range Range;
	int RangeDays = -1;
	double RangeMinute = -1.0, RangeStepped = -1.0;
	size_t RangeVideos = 0;
	video::Range Before; // the window before it (the tiles' deltas)
	video::RealtimeView Rt;
	double RtMinute = -1.0;
	size_t RtVideos = 0;
	video::Health Health;
	video::AudienceView Audience;
	std::vector<video::Trend> Trends;
	std::vector<video::AdMarketRow> Market;
	std::vector<video::WorldVideo> WorldVids;
	double DayKey = -1.0, DayStepped = -1.0;
	size_t DayVideos = 0;
	// C3 fix round (M2, §5.8): Research's "What poker viewers search" (video::SearchesOf), once a world day or when the
	// channel or the ideas move.
	std::vector<video::SearchRow> Searches;
	std::string SearchKey;
	// C3 fix round: Analytics' "?" cards (H1's top-earner rate, Research's "how Rerun decides"): the open one, where its "?"
	// sits and its words (drawn over the page last; the page takes no click inside it).
	std::string HelpOpen;
	bool HelpSeen = false; // its "?" drew this frame (a page that moved on closes it)
	Rect HelpAt{}, HelpBox{};
	float HelpBelow = 0.0f; // the card opens below this (the bottom of the block its "?" explains, so the block stays readable)
	std::string HelpTitle, HelpText;
	// Thumbnails: the frame's paint allowance (ui-thumbs' thumbart::Budget: at most 2 painted uncached, the rest skeletons
	// waiting their turn) and the player's look as a key (the banner's cached portrait).
	thumbart::Budget Paint;
	uint64_t LookKey = 0;
};

/** Everything one frame of the app needs (the class's members are private; the drawing below is free functions). */
struct RrFrame
{
	RiverLine& RL;
	Canvas& C;
	Ui& UI;
	Session& S;
	VideoUi& U;
	double Now;
	double World;
	double Dt;
	bool Live; // input on (not drawing into the stream preview)
};

// ------------------------------------------------------------------ the side state (RiverLineVideo.cpp)

std::map<const RiverLine*, VideoUi>& VideoUiStates();
VideoUi& VideoUiOf(const RiverLine* RL);
std::string VideoBellKey(const std::pair<double, std::string>& B);
void VideoPrime(VideoUi& U, const Session& S);
int VideoUnseenBell(const VideoUi& U, const Session& S);
bool RrInside(const Rect& R, float X, float Y);

// ------------------------------------------------------------------ words (RiverLineVideo.cpp)

int RrNight(double At);
std::string RrEventKey(const std::string& Id);
bool RrEventKind(video::Kind K);
const video::Candidate* RrFind(const std::vector<video::Candidate>& Open, const std::string& Id);
std::string RrWhen(double At);
std::string RrDaysLeft(double Expires, double World);
std::string RrEditTime(double Minutes);
double RrLengthMin(video::Format F);
const char* RrFormatSub(video::Format F);
std::string RrViews(double Views);
std::string RrAge(double Minutes);
std::string RrFact(const video::Candidate& C, const std::vector<video::Candidate>& Open, std::string& Tail);
std::string RrShorten(const Canvas& Cv, std::string Text, float Size, int Weight, float MaxW);
std::string RrWhereChip(const video::Candidate& C);
std::string RrJoinHint(const VideoUi& U, const std::vector<video::Candidate>& Open, const video::Candidate& C);
video::Choice RrChoice(const VideoUi& U);
std::vector<std::string> RrPublishWhy(const Session& S, const video::Choice& Ch);
bool VideoFromStream(const video::Candidate& C, double From, double To);
std::vector<video::Candidate> RrListed(const Session& S, const VideoUi& U);
void RrToggle(VideoUi& U, const std::vector<video::Candidate>& Open, const video::Candidate& C, double Now);
void RrSetFormat(VideoUi& U, const std::vector<video::Candidate>& Open, video::Format F);
// Rerun 2.0's words: the renames (Short shows as "Cut", Hype as "Loud"), counts, shares, durations.
const char* RrFormatName(video::Format F);
const char* RrStyleName(video::TitleStyle T);
/** "8:00", "0:45", "1:02:03". */
std::string RrClock(double Minutes);
/** "61,408" (floored). */
std::string RrCount(double V);
/** "1 comment", "24 comments" (the count floored, as RrCount writes it). */
std::string RrPlural(double V, const char* One, const char* Many);
/** Public rounding (§5.1): "312", "4.2K", "61K", "1.4M". */
std::string RrShort(double V);
/** "6.3%" (Digits after the point). */
std::string RrPct(double Fraction, int Digits = 1);
std::string RrCents(double Cents);
/** "4.2K subscribers" (public), "1 subscriber". */
std::string RrSubsPublic(double Subs);
/** The like ratio as the counts beside it read (each floored, as RrCount writes them; 1 with none): "12 · 0" is 100%. */
double RrLikeRatio(double Likes, double Dislikes);
/** Subscribers as a tile shows them, net, lost and gained floored so the three add up ("+79", "84 gained · 5 lost"). */
void RrSubsShown(double Net, double Lost, double& NetShown, double& LostShown, double& GainedShown);
/** The verdict's label (Not pushed .. Pushed hard) and its colour on the five-step scale. */
const char* RrVerdictName(int Label);
Color RrVerdictColor(int Label);

// ------------------------------------------------------------------ pieces (RiverLineVideo.cpp)

bool RrSegment(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, const std::string& Sub, bool Picked);
bool RrButton(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, bool Enabled);
bool RrLink(RrFrame& F, const std::string& Id, const std::string& Label, float X, float Y, float Size, const Color& Col, Align A = Align::Left);
void RrBellIcon(Canvas& Cv, float Cx, float Cy, float S, const Color& Col);
void RrTile(RrFrame& F, const Rect& R, const video::Video& V, bool Earnings, double EarnedCents);
double RrEarnedCents(const video::Video& V, double PartnerAt, double World);
/** A card: the panel, its spaced-caps title and a dim note on the right (the mockups' card_box). */
void RrCard(RrFrame& F, const Rect& R, const std::string& Title, const std::string& Right = std::string());
/** A tag pill (BREAKOUT solid, ABOVE TYPICAL tinted): its width. */
float RrTag(Canvas& Cv, const std::string& Label, float X, float Y, const Color& Col, bool Solid, float Size = 10.0f);
/** Five bars, N lit (Ghost > N: the bars a move is heading to, outlined: the meter moving without jumping). */
void RrBars5(Canvas& Cv, float X, float Y, int N, const Color& Col, float W = 9.0f, float H = 11.0f, float Gap = 3.0f, int Ghost = -1);
/** A pill-shaped tab (Analytics' sub-tabs, a video's four, the list's segments): true when clicked. */
bool RrChip(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, bool On, float Size = 13.5f);
/** The chart kit's Rerun look (cached). */
const chart::Theme& RrTheme();
/** The bell's line colour by what it says (a breakout or a binge in marigold, a late bloomer in green, the rest cream). */
Color RrBellAccent(const std::string& Line);
/** The newest Update 3 upload still inside its 48-hour test (the launch card's video), or null. */
const video::Video* RrLaunchVideo(const Session& S, double World);
/** A kept video by id, or null. */
const video::Video* RrVideo(const Session& S, const std::string& Id);
/** R1's carry: the Momentum an upload going up at UpAt starts with (Upload's own rule: the first upload after 21 or more
 *  dark days adds welcome back's +0.2 to H first); 1 on an Update 2 channel. */
double RrCarryAt(const video::Channel& Ch, double UpAt);
/** Whether Rerun is testing V on Home right now (RiverLineVideoHome.cpp): a Home-tagged part in its fast phase, the core's
 *  rule for Realtime's OnHomeNow, read by the video itself (OnHomeNow lists titles, and two uploads can share one). */
bool RrHomeTested(const video::Video& V, double World);
/** Analytics › Content › one video. */
void RrOpenVideo(RrFrame& F, const std::string& Id);
/** Kast's end card's "Make it a Cut" (K15, G3's "Make a Cut and link it"): Studio on the moments From..To as a Cut, its
 *  Next-up card the newest kept long video (RiverLineKast.cpp calls it by ui-rerun's patch to ui-kast). */
void VideoOpenCut(RiverLine& RL, double From, double To, double Now);
/** ss.Video.Sheet's page (DebugOpen "video sheet <group> <n>", by ui-rerun's patch to the lead's RiverLine.cpp): n of the
 *  library's recipes for a kind group (beats, reads, pots, runs, rival, stories, ideas) on one sheet; false for no group. */
bool VideoOpenSheet(RiverLine& RL, const std::string& Group, int N, double Now);
/** A synthetic ss.Video.Grant video (never ranked, never on Home): its id. */
bool RrIsGrant(const video::Video& V);

// ------------------------------------------------------------------ thumbnails (RiverLineVideo.cpp)

/** A recipe into R (Studio's options, a Duel's contenders) through ui-thumbs' thumbart::Draw: replayed when cached; else
 *  painted while the frame's budget allows (2 a frame), else a skeleton. Facts: the video's or the choice's Thumb. */
void RrSpecThumb(RrFrame& F, const Rect& R, const video::ThumbSpec& Sp, const video::Thumb& Facts);
/** A world creator's video (Home): its recipe with the creator's own face; Lod: S-T's fail-over (flat faces). */
void RrWorldThumb(RrFrame& F, const Rect& R, const video::WorldVideo& W, bool Lod = false);
/** A video's thumbnail into R: its shown recipe (thumbart::VideoThumb), or an Update 2 frame as Update 2 drew it. Lod: a
 *  recipe drawn with S-T's fail-over (thumbart::Draw's Lod overload; an Update 2 frame has none). */
void RrThumb(RrFrame& F, const Rect& R, const video::Video& V, bool Lod = false);
/**
 * Home's and the Channel's grids draw S-T's fail-over (ui-thumbs' D1): 12 full-face tiles are about 44.6k (Home) and 42.5k
 * (Channel) thumbnail vertices, over a laptop page's 35k (§10); flat faces bring a full page to about 31k. Studio, the launch
 * card, Analytics and the Outlier shelf keep the faces. False puts them back on the grids (the lead's call after PIE).
 */
constexpr bool RrGridLod = true;
/** An Update 2 frame for a choice (Studio's preview before the new thumbnails), cached as RrThumb caches one. */
void RrLegacyThumb(RrFrame& F, const Rect& R, const video::Thumb& T);
/** Paint into R through Rerun's pool under Key (drawn around the origin at R's size), or the skeleton when this frame has
 *  painted two uncached thumbnails already. */
void RrPooledThumb(RrFrame& F, const Rect& R, uint64_t Key, const std::function<void(Canvas&)>& Paint);
/** The strike flag (a vector mark: a rival site's promotion under a RiverLine clause). */
void RrFlag(Canvas& Cv, float X, float Y, const Color& Col);
/** The length's badge in a thumbnail's corner ("8:00"), and "CUT" on a Cut. */
void RrLengthBadge(RrFrame& F, const Rect& Th, const video::Video& V);

// ------------------------------------------------------------------ the core's numbers, cached (RiverLineVideo.cpp)

const video::VideoStats& RrStats(RrFrame& F, const video::Video& V);
/** The channel over the last Days days (and the Days before them, for the deltas): by the world minute. */
const video::Range& RrRange(RrFrame& F, int Days);
const video::RealtimeView& RrRealtime(RrFrame& F);
/** Health, Audience, the trends, the Ad market and the world's videos: once a world day (or when the channel changes). */
void RrDaily(RrFrame& F);
const std::vector<video::IdeaCard>& RrIdeas(RrFrame& F);

// ------------------------------------------------------------------ the pages

/** Studio (RiverLineVideoStudio.cpp): the list, the editor, the side column; the launch card for 48 hours after a drop. */
void RrStudio(RrFrame& F);
/** A ready idea chosen (an IDEAS card, Research's "make it"): its candidate made (Session::ChooseIdea) and picked, in its first
 *  format; false with the session's reason in the toast. */
bool RrChooseIdea(RrFrame& F, const video::IdeaCard& I);
/** Analytics (RiverLineVideoStats.cpp): six pages and one video's four tabs. */
void RrAnalytics(RrFrame& F);
/** Channel health (RiverLineVideoStats.cpp): the six meters, the moving Momentum and the upload streak. */
void RrHealthCard(RrFrame& F, const Rect& R);
/** One video's RPM a day in cents, the Days days to Today (its Revenue tab's chart): what each day's views paid per 1,000, past
 *  the top-earner rate at the day's share of the full rate (RiverLineVideoStats.cpp). */
std::vector<double> RrRpmADay(const video::Channel& Ch, const video::Video& V, int Today, int Days, double World);
/** C3 PIE fix F1 (RiverLineVideoStats.cpp; V6): a window's estimated days, the core's Range::Estimated where the estimate carries
 *  anything (a day before the channel's first video is a plain zero), the quiet days between the first and the last of them
 *  included; and a line split on them (the measured days solid, first; the estimated ones dashed over a lighter fill). */
std::vector<bool> RrEstDays(const video::Range& R);
std::vector<chart::Series> RrEstSplit(const chart::Series& S, const std::vector<bool>& Est);
/** Home (RiverLineVideoHome.cpp): the world's videos and yours while Rerun tests them there, RECOMMENDED. */
void RrHome(RrFrame& F);
/** ss.Video.Sheet's contact sheet (RiverLineVideoHome.cpp). */
void RrSheet(RrFrame& F);
/** The copy the pages add, each file's own: its sentences (Sentences, joined by VideoUiInventory for the copy lint) or its
 *  labels and fragments (joined by VideoUiFragments: screened for real names and U1's words, never sentence-linted). */
std::vector<std::string> RrStudioInventory(bool Sentences);
std::vector<std::string> RrStatsInventory(bool Sentences);
std::vector<std::string> RrHomeInventory(bool Sentences);
/** Every label and fragment Rerun's pages draw beyond the core's (the trademark and U1 screens read it with the sentences). */
std::vector<std::string> VideoUiFragments();
/** The key the library's options were dealt for (the pick, the format, the style, the two shuffles): Studio deals again
 *  when it moves. */
std::string RrOptionKey(const VideoUi& U);
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss
