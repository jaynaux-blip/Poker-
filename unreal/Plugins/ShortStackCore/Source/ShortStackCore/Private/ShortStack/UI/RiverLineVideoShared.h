// Rerun's shared look, side state and helpers (update 3 SPEC §6.6, §7.1, §8.2: the split of RiverLineVideo.cpp, a no-change
// move). RiverLineVideo.cpp keeps the app's shell, the header, the bell and Channel; RiverLineVideoStudio.cpp has Studio (and
// Rerun 2.0's launch card next); RiverLineVideoStats.cpp (Analytics) and RiverLineVideoHome.cpp (Home, Next up) share
// these. The look (marigold on ink), the copy, VideoUi (one laptop's Rerun, kept beside the class by laptop) and RrFrame are
// defined here; the helpers' bodies stay in RiverLineVideo.cpp. ui-rerun owns this header after Phase A.
#pragma once

#include "ShortStack/UI/RiverLine.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Video.h"
#include "ShortStack/UI/VideoArt.h"

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

// Studio's three columns.
const float RrListX = 20.0f;
const float RrListW = 420.0f;
const float RrListTop = 118.0f;
const float RrRowH = 86.0f;
const float RrRowGap = 8.0f;
const float RrEdX = 460.0f;
const float RrEdW = 700.0f;
const float RrSideX = 1180.0f;
const float RrSideW = 400.0f;
// The thumbnail at its real size (§4.5, §8.6 VC2).
const float RrThumbW = 640.0f;
const float RrThumbH = 360.0f;
// The Channel's grid.
const int RrCols = 7;
const float RrGridTop = 536.0f;
const float RrTileGap = 14.0f;
const float RrTilePitch = 214.0f;

// The copy these screens add to core's (VideoCopy.cpp's titles, advice, letters and bell). Sentences are in
// VideoUiInventory (the copy lint).
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
constexpr const char* RrHoursNote = "Long videos count toward watch hours. Shorts bring subscribers, not hours.";
constexpr const char* RrGateNext = "Fill both bars and Rerun takes a look.";
constexpr const char* RrUnderReview = "Rerun's looking your channel over. Word comes by Mail in about a week.";
constexpr const char* RrPartnerHead = "You're a Rerun Partner.";
constexpr const char* RrPartnerSince = "Ads run on every video from that day on. Views from before it never earn.";
constexpr const char* RrAdsAtGate = "Ads start at the gate.";
constexpr const char* RrAdsAfterReview = "Ads start when the review's done."; // both bars full: the gate's passed, the week's review runs
constexpr const char* RrPaysOut = "Pays out on the 21st once it reaches $100.";
constexpr const char* RrBellEmpty = "Nothing yet. Milestones land here as your videos grow.";

/** One laptop's Rerun. */
struct VideoUi
{
	int Page = 0; // 0 Studio, 1 Channel
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

// ------------------------------------------------------------------ pieces (RiverLineVideo.cpp)

bool RrSegment(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, const std::string& Sub, bool Picked);
bool RrButton(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, bool Enabled);
bool RrLink(RrFrame& F, const std::string& Id, const std::string& Label, float X, float Y, float Size, const Color& Col, Align A = Align::Left);
void RrBellIcon(Canvas& Cv, float Cx, float Cy, float S, const Color& Col);
void RrTile(RrFrame& F, const Rect& R, const video::Video& V, bool Earnings, double EarnedCents);
double RrEarnedCents(const video::Video& V, double PartnerAt, double World);

// ------------------------------------------------------------------ the pages

/** Studio (RiverLineVideoStudio.cpp): the candidates, the editor, the side column. */
void RrStudio(RrFrame& F);
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss
