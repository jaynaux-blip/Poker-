// Rerun (update 2 SPEC §4.1, §4.5, §4.8, §4.9): the video site on the laptop, its ninth app. ui-video's file.
//
// STUDIO: the candidates on the left, best first (Session::VideoCandidates), each with its kind's glyph, what happened,
// when, the AUTO, site and storytime chips and the days it has left; the editor in the middle (the format, the title
// style with the real title, the thumbnail frame drawn by videoart::Thumbnail at 640 x 360, the edit's time and energy,
// PUBLISH through Session::Publish, greyed with the honest reason first); on the right, a line on what each choice does
// (video::Advice), never a number, and the video's tile as the Channel will show it. CHANNEL: the banner, the partner
// gate's two bars, the last 28 days (views, subscribers gained and, once in, revenue: video::View) and the videos, each
// taken down with a second press. The bell keeps the channel's milestones (Channel::Bell, written by core-video's Step).
//
// Side state lives beside the class, by laptop, as Mail's does (RiverLine.h is frozen): the page, the pick, the scroll,
// the bell, what was new since the last visit (the taskbar's dot, read through VideoDot), the delete's confirm.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <array>
#include <cmath>
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

std::map<const RiverLine*, VideoUi>& VideoUiStates()
{
	static std::map<const RiverLine*, VideoUi> ByLaptop;
	return ByLaptop;
}

VideoUi& VideoUiOf(const RiverLine* RL)
{
	return VideoUiStates()[RL];
}

std::string VideoBellKey(const std::pair<double, std::string>& B)
{
	return std::to_string(static_cast<long long>(std::llround(B.first * 100.0))) + "|" + B.second;
}

/**
 * The first look on this laptop: what's older than half a day counts as seen. The last visit isn't saved (the channel's
 * struct froze with Life.h), so after a load the last twelve hours count as new: a night at the card room still lights
 * the dot when the player gets home.
 */
void VideoPrime(VideoUi& U, const Session& S)
{
	if (U.Primed)
	{
		return;
	}
	U.Primed = true;
	const double World = S.WorldMinutes();
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		if (C.At < World - 12.0 * 60.0)
		{
			U.SeenCandidates.insert(C.Id);
		}
	}
	for (const auto& B : S.Life.Video.Bell)
	{
		if (B.first < World - 12.0 * 60.0)
		{
			U.SeenBell.insert(VideoBellKey(B));
		}
	}
}

int VideoUnseenBell(const VideoUi& U, const Session& S)
{
	int N = 0;
	for (const auto& B : S.Life.Video.Bell)
	{
		N += U.SeenBell.count(VideoBellKey(B)) == 0 ? 1 : 0;
	}
	return N;
}

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

bool RrInside(const Rect& R, float X, float Y)
{
	return X >= R.X && X <= R.X + R.W && Y >= R.Y && Y <= R.Y + R.H;
}

// ------------------------------------------------------------------ words

/** The night a moment belongs to (a night runs to 10 AM): Video.cpp's rule for a Vlog's moments. */
int RrNight(double At)
{
	return static_cast<int>(std::floor((At - 600.0) / net::MinutesPerDay + 1e-9));
}

/**
 * "rl:mm-26@1530:214" -> "rl:mm-26@1530": the event a moment came from (a Recap's are one event's). A live hand the plan
 * played through a skip is "lv:<entry>:skip:<n>": the same entry's (Video.cpp's EventKey reads it the same way).
 */
std::string RrEventKey(const std::string& Id)
{
	const size_t Cut = Id.rfind(':');
	std::string Key = Cut == std::string::npos ? Id : Id.substr(0, Cut);
	const size_t Skip = Key.size() >= 5 ? Key.size() - 5 : std::string::npos;
	if (Skip != std::string::npos && Key.compare(Skip, 5, ":skip") == 0)
	{
		Key.resize(Skip);
	}
	return Key;
}

bool RrEventKind(video::Kind K)
{
	return K == video::Kind::DeepRun || K == video::Kind::FinalTable || K == video::Kind::Win;
}

const video::Candidate* RrFind(const std::vector<video::Candidate>& Open, const std::string& Id)
{
	for (const video::Candidate& C : Open)
	{
		if (C.Id == Id)
		{
			return &C;
		}
	}
	return nullptr;
}

/** "Tue Oct 13 · 3:40 AM". */
std::string RrWhen(double At)
{
	const int Day = net::DayOf(At);
	return std::string(net::WeekdayName(Day)) + " " + net::DateLabel(Day) + " \xC2\xB7 " + net::TimeLabel(At);
}

std::string RrDaysLeft(double Expires, double World)
{
	const double Days = (Expires - World) / net::MinutesPerDay;
	if (Days >= 2.0)
	{
		return std::to_string(static_cast<int>(std::floor(Days))) + " days left";
	}
	return Days >= 1.0 ? std::string("1 day left") : std::string("last day");
}

/** "1 h 30 m", "20 m", "3 h". */
std::string RrEditTime(double Minutes)
{
	const long long M = std::llround(Minutes);
	const long long H = M / 60, Rest = M % 60;
	if (H == 0)
	{
		return std::to_string(Rest) + " m";
	}
	return std::to_string(H) + " h" + (Rest > 0 ? " " + std::to_string(Rest) + " m" : std::string());
}

/** A format's length on Rerun (§4.5's table), for the tile's corner. */
double RrLengthMin(video::Format F)
{
	switch (F)
	{
	case video::Format::Short: return 0.75;
	case video::Format::Vlog: return 18.0;
	case video::Format::Recap: return 14.0;
	case video::Format::Breakdown: break;
	}
	return 8.0;
}

const char* RrFormatSub(video::Format F)
{
	switch (F)
	{
	case video::Format::Short: return "45 s \xC2\xB7 one moment";
	case video::Format::Vlog: return "18 min \xC2\xB7 one night";
	case video::Format::Recap: return "14 min \xC2\xB7 one run";
	case video::Format::Breakdown: break;
	}
	return "8 min \xC2\xB7 one hand";
}

/** "1 view", "312 views", "4,210 views", "12K views", "1.2M views". */
std::string RrViews(double Views)
{
	const long long N = static_cast<long long>(std::floor(std::max(0.0, Views)));
	if (N >= 1000000)
	{
		return Fixed(static_cast<double>(N) / 1000000.0, N >= 10000000 ? 0 : 1) + "M views";
	}
	if (N >= 10000)
	{
		return std::to_string(N / 1000) + "K views";
	}
	return Grouped(N) + (N == 1 ? " view" : " views");
}

std::string RrAge(double Minutes)
{
	const double Days = Minutes / net::MinutesPerDay;
	if (Minutes < 60.0)
	{
		return "just now";
	}
	if (Days < 1.0)
	{
		const int H = static_cast<int>(Minutes / 60.0);
		return std::to_string(H) + (H == 1 ? " hour ago" : " hours ago");
	}
	if (Days < 30.0)
	{
		const int D = static_cast<int>(Days);
		return std::to_string(D) + (D == 1 ? " day ago" : " days ago");
	}
	if (Days < 365.0)
	{
		const int M = static_cast<int>(Days / 30.0);
		return std::to_string(M) + (M == 1 ? " month ago" : " months ago");
	}
	const int Y = static_cast<int>(Days / 365.0);
	return std::to_string(Y) + (Y == 1 ? " year ago" : " years ago");
}

/**
 * What happened, in a line: the event (returned) and the rest (Tail: the pot and who, or the run's place and prize), or the
 * life beat a story tells (no Tail). The row shortens the event's name before the rest: a series event's long name
 * ("RLCOOP #23: $109 Headhunter High Roller") would otherwise push the pot off the row.
 */
std::string RrFact(const video::Candidate& C, const std::vector<video::Candidate>& Open, std::string& Tail)
{
	Tail.clear();
	if (C.K == video::Kind::Story)
	{
		video::Choice One;
		One.Candidates = {C.Id};
		One.F = video::Format::Short;
		One.T = video::TitleStyle::Plain;
		const std::string Line = video::Title(One, Open, 0);
		return Line.empty() ? std::string("A story from your own life") : Line;
	}
	// Dee's game is told as storytime (§4.4): nobody named, and no "Dee's" either.
	const std::string Out = C.Storytime ? std::string("A back-room game") : C.Event.empty() ? rlsites_detail::SitesShortName(static_cast<net::SiteId>(std::clamp(C.Site, 0, static_cast<int>(net::SiteId::Count) - 1))) : C.Event;
	if (RrEventKind(C.K))
	{
		if (C.PlaceSoFar > 0)
		{
			Tail += " \xC2\xB7 " + Ordinal(C.PlaceSoFar) + (C.Field > 0 ? " of " + Grouped(C.Field) : std::string());
		}
		if (C.Rec.Pot > 0)
		{
			Tail += " \xC2\xB7 " + Money(C.Rec.Pot);
		}
		return Out;
	}
	if (C.Rec.Blinds.Bb > 0 && C.Rec.Pot > 0)
	{
		Tail += " \xC2\xB7 " + Grouped(std::llround(static_cast<double>(C.Rec.Pot) / static_cast<double>(C.Rec.Blinds.Bb))) + " BB";
	}
	if (!C.Storytime && !C.Opponent.empty())
	{
		Tail += " \xC2\xB7 vs " + C.Opponent;
	}
	return Out;
}

/** Text cut to MaxW with an ellipsis, a whole UTF-8 character at a time (Canvas::Text's own rule, for a part of a line). */
std::string RrShorten(const Canvas& Cv, std::string Text, float Size, int Weight, float MaxW)
{
	if (Cv.Measure(Text, Size, Weight) <= MaxW)
	{
		return Text;
	}
	const std::string Ellipsis = "\xE2\x80\xA6";
	while (!Text.empty() && Cv.Measure(Text + Ellipsis, Size, Weight) > MaxW)
	{
		while (!Text.empty() && (static_cast<unsigned char>(Text.back()) & 0xC0) == 0x80)
		{
			Text.pop_back();
		}
		if (!Text.empty())
		{
			Text.pop_back();
		}
	}
	while (!Text.empty() && Text.back() == ' ')
	{
		Text.pop_back();
	}
	return Text + Ellipsis;
}

/** The site or room a moment came from, for its chip ("" for none): the poker site online, LIVE at the Embercrest. */
std::string RrWhereChip(const video::Candidate& C)
{
	if (C.Source == 0 || C.Source == 3)
	{
		const int Site = std::clamp(C.Site, 0, static_cast<int>(net::SiteId::Count) - 1);
		return rlnet_detail::NetUpper(net::Def(static_cast<net::SiteId>(Site)).Name);
	}
	return C.Source == 1 ? std::string("LIVE") : std::string();
}

/** Why a moment can't join the pick in a Vlog or a Recap ("" when it can, or the format takes one). */
std::string RrJoinHint(const VideoUi& U, const std::vector<video::Candidate>& Open, const video::Candidate& C)
{
	if (U.F != video::Format::Vlog && U.F != video::Format::Recap)
	{
		return std::string();
	}
	if (C.K == video::Kind::Story)
	{
		return "stands alone";
	}
	const video::Candidate* First = nullptr;
	for (const std::string& Id : U.Pick)
	{
		const video::Candidate* X = RrFind(Open, Id);
		if (X && X->Id != C.Id && X->K != video::Kind::Story)
		{
			First = X;
			break;
		}
	}
	if (!First)
	{
		return std::string();
	}
	if (U.F == video::Format::Vlog)
	{
		return RrNight(C.At) == RrNight(First->At) ? std::string() : std::string("another night");
	}
	return RrEventKey(C.Id) == RrEventKey(First->Id) ? std::string() : std::string("another event");
}

/** The pick Studio's preview and PUBLISH use. */
video::Choice RrChoice(const VideoUi& U)
{
	video::Choice Ch;
	Ch.Candidates = U.Pick;
	Ch.F = U.F;
	Ch.T = U.T;
	Ch.Fr = U.Fr;
	return Ch;
}

/**
 * Why PUBLISH would refuse, the choice's own problem first (it's what the player is building), then the session's
 * (Session::Publish's checks and words: busy, tables open, mid-stream, below 10 energy).
 */
std::vector<std::string> RrPublishWhy(const Session& S, const video::Choice& Ch)
{
	std::vector<std::string> Out;
	const std::string Pick = video::CanUpload(Ch, S.Life.Video.Open);
	if (!Pick.empty())
	{
		Out.push_back(Pick);
	}
	int Reason = 0;
	if (S.TimeSkip.Active)
	{
		Reason = 1;
	}
	else if (S.T || S.TableCount() > 0)
	{
		Reason = 2;
	}
	else if (S.Stream.Live)
	{
		Reason = 3;
	}
	else if (S.Life.Energy < 10.0)
	{
		Reason = 4;
	}
	if (Reason > 0)
	{
		Out.push_back(video::PublishWhy(Reason));
	}
	return Out;
}

/** A moment from the stream that ran From..To: the online hands and runs it went out over (not a life story). */
bool VideoFromStream(const video::Candidate& C, double From, double To)
{
	return C.At >= From && C.At <= To && (C.Source == 0 || C.Source == 3);
}

/** The candidates the list shows: every open one, or tonight's stream only (Kast's Cut highlights). */
std::vector<video::Candidate> RrListed(const Session& S, const VideoUi& U)
{
	std::vector<video::Candidate> All = S.VideoCandidates();
	if (!U.Tonight)
	{
		return All;
	}
	std::vector<video::Candidate> Out;
	for (const video::Candidate& C : All)
	{
		if (VideoFromStream(C, U.TonightFrom, U.TonightTo))
		{
			Out.push_back(C);
		}
	}
	return Out;
}

/** A row clicked: the pick it makes for the format (one for a Short or a Breakdown; a toggle in a night's Vlog or a Recap). */
void RrToggle(VideoUi& U, const std::vector<video::Candidate>& Open, const video::Candidate& C, double Now)
{
	U.Touched = true;
	const auto It = std::find(U.Pick.begin(), U.Pick.end(), C.Id);
	if (U.F == video::Format::Short || U.F == video::Format::Breakdown)
	{
		U.Pick = {C.Id};
		return;
	}
	if (It != U.Pick.end())
	{
		U.Pick.erase(It);
		return;
	}
	if (!RrJoinHint(U, Open, C).empty())
	{
		U.Pick = {C.Id}; // another night or event starts over from this one (a story: CanUpload says why it can't)
		return;
	}
	const size_t Max = U.F == video::Format::Vlog ? 8 : 7;
	if (U.Pick.size() >= Max)
	{
		U.Note = U.F == video::Format::Vlog ? RrVlogFull : RrRecapFull;
		U.NoteBad = true;
		U.NoteAt = Now;
		return;
	}
	U.Pick.push_back(C.Id);
}

/** A new format: the pick keeps what it can (a Recap brings in its run's result when it's on the list). */
void RrSetFormat(VideoUi& U, const std::vector<video::Candidate>& Open, video::Format F)
{
	U.F = F;
	if (F == video::Format::Short || F == video::Format::Breakdown)
	{
		if (U.Pick.size() > 1)
		{
			U.Pick.resize(1);
		}
		return;
	}
	if (U.Pick.empty())
	{
		return;
	}
	const video::Candidate* First = RrFind(Open, U.Pick.front());
	if (!First)
	{
		return;
	}
	if (F == video::Format::Recap)
	{
		bool HasRun = false;
		for (const std::string& Id : U.Pick)
		{
			const video::Candidate* X = RrFind(Open, Id);
			HasRun = HasRun || (X && RrEventKind(X->K));
		}
		if (!HasRun)
		{
			for (const video::Candidate& X : Open)
			{
				if (RrEventKind(X.K) && RrEventKey(X.Id) == RrEventKey(First->Id))
				{
					U.Pick.insert(U.Pick.begin(), X.Id);
					break;
				}
			}
		}
		const std::string Key = RrEventKey(First->Id);
		U.Pick.erase(std::remove_if(U.Pick.begin(), U.Pick.end(), [&](const std::string& Id) { return RrEventKey(Id) != Key; }), U.Pick.end());
		return;
	}
	const int Night = RrNight(First->At);
	U.Pick.erase(std::remove_if(U.Pick.begin(), U.Pick.end(),
					 [&](const std::string& Id) {
						 const video::Candidate* X = RrFind(Open, Id);
						 return !X || RrNight(X->At) != Night;
					 }),
		U.Pick.end());
}

// ------------------------------------------------------------------ pieces

/** A segmented choice (a format, a title style, a frame): marigold when picked. */
bool RrSegment(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, const std::string& Sub, bool Picked)
{
	using namespace rlnet_detail;
	const Ui::ClickState St = F.UI.Clickable(Id, R, F.Live);
	const Color Fill = Picked ? Mix(RrPanel2, RrGold, 0.16f) : St.Hover ? RrPanel2 : RrPanel;
	F.C.FillRoundRect(R, 10.0f, Fill);
	F.C.StrokeRoundRect(R, 10.0f, Picked ? RrGold : St.Hover ? NetA(RrCream, 0.3f) : RrLine, Picked ? 2.0f : 1.0f);
	const float Cx = R.X + R.W / 2.0f;
	const Color Ink = Picked ? RrCream : NetA(RrCream, 0.82f);
	if (Sub.empty())
	{
		F.UI.Text(Label, Cx, R.Y + R.H / 2.0f + 1.0f, Ts(15.0f, 800, Ink, Align::Center, Baseline::Middle, false, R.W - 16.0f));
	}
	else
	{
		F.UI.Text(Label, Cx, R.Y + R.H / 2.0f - 2.0f, Ts(16.0f, 800, Ink, Align::Center, Baseline::Alphabetic, false, R.W - 16.0f));
		F.UI.Text(Sub, Cx, R.Y + R.H / 2.0f + 16.0f, Ts(12.0f, 600, Picked ? RrGold : RrMuted, Align::Center, Baseline::Alphabetic, false, R.W - 16.0f));
	}
	return St.Clicked;
}

/** Rerun's main button: marigold on ink, or grey when it can't go. */
bool RrButton(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, bool Enabled)
{
	using namespace rlnet_detail;
	const Ui::ClickState St = F.UI.Clickable(Id, R, Enabled && F.Live);
	const Rect B{R.X, R.Y + (St.Down ? 2.0f : 0.0f), R.W, R.H};
	if (Enabled)
	{
		if (St.Hover)
		{
			F.C.GlowRoundRect(B, 12.0f, NetA(RrGold, 0.35f), 14.0f);
		}
		F.C.FillRoundRect(B, 12.0f, Paint::Linear({0.0f, B.Y}, {0.0f, B.Y + B.H}, St.Hover ? Mix(RrGold, Hex(0xffffff), 0.12f) : RrGold, Mix(RrGold, Hex(0x000000), 0.14f)));
	}
	else
	{
		F.C.FillRoundRect(B, 12.0f, Rgba(128, 128, 128, 0.16f));
		F.C.StrokeRoundRect(B, 12.0f, Rgba(128, 128, 128, 0.3f), 1.0f);
	}
	F.UI.Text(Label, B.X + B.W / 2.0f, B.Y + B.H / 2.0f + 1.0f,
		Ts(18.0f, 900, Enabled ? Hex(videoart::Ink) : Rgba(160, 160, 160, 0.95f), Align::Center, Baseline::Middle, false, B.W - 20.0f));
	return St.Clicked;
}

/** A link in Rerun's text (Show all, Delete): true when clicked. */
bool RrLink(RrFrame& F, const std::string& Id, const std::string& Label, float X, float Y, float Size, const Color& Col, Align A = Align::Left)
{
	const float W = F.UI.Measure(Label, Size, 800);
	const float X0 = A == Align::Right ? X - W : A == Align::Center ? X - W / 2.0f : X;
	const Rect R{X0 - 4.0f, Y - Size - 2.0f, W + 8.0f, Size + 10.0f};
	const Ui::ClickState St = F.UI.Clickable(Id, R, F.Live);
	F.UI.Text(Label, X0, Y, Ts(Size, 800, St.Hover ? RrCream : Col));
	if (St.Hover)
	{
		F.C.FillRect({X0, Y + 3.0f, W, 1.0f}, RrCream);
	}
	return St.Clicked;
}

void RrBellIcon(Canvas& Cv, float Cx, float Cy, float S, const Color& Col)
{
	Cv.FillCircle(Cx, Cy - S * 0.44f, S * 0.08f, Col);
	Cv.FillRoundRect({Cx - S * 0.3f, Cy - S * 0.38f, S * 0.6f, S * 0.56f}, S * 0.28f, Col);
	Cv.FillPolygon({{Cx - S * 0.3f, Cy + S * 0.02f}, {Cx + S * 0.3f, Cy + S * 0.02f}, {Cx + S * 0.44f, Cy + S * 0.28f}, {Cx - S * 0.44f, Cy + S * 0.28f}}, Col);
	Cv.FillCircle(Cx, Cy + S * 0.38f, S * 0.1f, Col);
}

/** A video as Rerun shows it: the thumbnail (the player's own look on a live frame), its length, title, views and age. */
void RrTile(RrFrame& F, const Rect& R, const video::Video& V, bool Earnings, double EarnedCents)
{
	using namespace rlnet_detail;
	const Rect Th{R.X, R.Y, R.W, std::round(R.W * 9.0f / 16.0f)};
	F.C.FillRoundRect(Th, 8.0f, RrGround);
	F.C.PushClip(Th);
	videoart::Thumbnail(F.C, Th, V.Th, &F.S.Person);
	F.C.PopClip();
	F.C.StrokeRoundRect(Th, 8.0f, RrLine, 1.0f);
	const long long Seconds = std::llround(V.LengthMin * 60.0);
	const std::string Length = std::to_string(Seconds / 60) + ":" + NetPad2(Seconds % 60);
	const float Lw = F.UI.Measure(Length, 11.5f, 800, true) + 12.0f;
	F.C.FillRoundRect({Th.X + Th.W - Lw - 6.0f, Th.Y + Th.H - 26.0f, Lw, 20.0f}, 4.0f, Hex(0x000000, 0.78f));
	F.UI.Text(Length, Th.X + Th.W - 12.0f, Th.Y + Th.H - 15.5f, Ts(11.5f, 800, RrCream, Align::Right, Baseline::Middle, true));
	const std::vector<std::string> Lines = NetWrap(F.C, V.Title, R.W, 14.0f, 700);
	float Y = Th.Y + Th.H + 20.0f;
	for (size_t I = 0; I < Lines.size() && I < 2; ++I)
	{
		std::string L = Lines[I];
		if (I == 1 && Lines.size() > 2)
		{
			L += " " + Lines[2];
		}
		F.UI.Text(L, R.X, Y, Ts(14.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, R.W));
		Y += 18.0f;
	}
	const float MetaY = Th.Y + Th.H + 20.0f + 2.0f * 18.0f + 2.0f;
	F.UI.Text(RrViews(video::Views(V, F.World)) + " \xC2\xB7 " + RrAge(F.World - V.At), R.X, MetaY, Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, R.W));
	if (Earnings)
	{
		F.UI.Text(Money(static_cast<Chips>(std::floor(EarnedCents))) + " earned", R.X, MetaY + 18.0f, Ts(12.0f, 700, RrCash, Align::Left, Baseline::Alphabetic, false, R.W));
	}
}

/** What one video has earned since the channel became a Partner (its RPM over the views since; §4.6: none before). */
double RrEarnedCents(const video::Video& V, double PartnerAt, double World)
{
	if (PartnerAt < 0.0 || World <= PartnerAt)
	{
		return 0.0;
	}
	const double Since = std::max(V.At, PartnerAt);
	return std::max(0.0, video::Views(V, World) - video::Views(V, Since)) * V.Rpm / 1000.0 * 100.0;
}

// ------------------------------------------------------------------ the header and the bell

void RrHeader(RrFrame& F)
{
	using namespace rlnet_detail;
	F.C.FillRect({0.0f, 0.0f, NetW, VideoTop}, RrBar);
	F.C.FillRect({0.0f, VideoTop - 1.0f, NetW, 1.0f}, RrLine);
	videoart::Logo(F.C, {22.0f, 12.0f, 36.0f, 36.0f});
	F.UI.Text("Rerun", 68.0f, 40.0f, Ts(26.0f, 900, RrCream));
	const char* Tabs[2] = {"Studio", "Channel"};
	float X = 172.0f;
	for (int K = 0; K < 2; ++K)
	{
		const float W = F.UI.Measure(Tabs[K], 16.0f, 800) + 28.0f;
		const Rect R{X, 10.0f, W, 40.0f};
		const Ui::ClickState St = F.UI.Clickable(std::string("rrtab") + Tabs[K], R, F.Live);
		if (St.Clicked)
		{
			F.U.Page = K;
			F.U.PageAt = F.Now;
			F.U.Confirm.clear();
		}
		const bool On = F.U.Page == K;
		F.UI.Text(Tabs[K], R.X + 14.0f, R.Y + 26.0f, Ts(16.0f, 800, On ? RrCream : St.Hover ? Mix(RrMuted, RrCream, 0.5f) : RrMuted));
		if (On)
		{
			F.C.FillRoundRect({R.X + 10.0f, VideoTop - 4.0f, W - 20.0f, 3.0f}, 1.5f, RrGold);
		}
		X += W + 6.0f;
	}
	// Right: the bell, and the channel at a glance.
	const Rect Bell{NetW - 64.0f, 10.0f, 40.0f, 40.0f};
	const Ui::ClickState St = F.UI.Clickable("rrbell", Bell, F.Live);
	if (St.Hover || F.U.BellOpen)
	{
		F.C.FillRoundRect(Bell, 10.0f, Rgba(255, 255, 255, F.U.BellOpen ? 0.1f : 0.06f));
	}
	RrBellIcon(F.C, Bell.X + 20.0f, Bell.Y + 20.0f, 24.0f, F.U.BellOpen ? RrGold : RrCream);
	const int Unseen = VideoUnseenBell(F.U, F.S);
	if (Unseen > 0)
	{
		const std::string N = Unseen > 9 ? std::string("9+") : std::to_string(Unseen);
		const float Bw = std::max(18.0f, F.UI.Measure(N, 11.0f, 900) + 10.0f);
		F.C.FillRoundRect({Bell.X + 24.0f, Bell.Y + 2.0f, Bw, 18.0f}, 9.0f, RrGold);
		F.UI.Text(N, Bell.X + 24.0f + Bw / 2.0f, Bell.Y + 11.5f, Ts(11.0f, 900, Hex(videoart::Ink), Align::Center, Baseline::Middle));
	}
	if (St.Clicked)
	{
		F.U.BellOpen = !F.U.BellOpen;
		F.U.BellFresh.clear();
		if (F.U.BellOpen)
		{
			for (const auto& B : F.S.Life.Video.Bell)
			{
				const std::string Key = VideoBellKey(B);
				if (F.U.SeenBell.insert(Key).second)
				{
					F.U.BellFresh.insert(Key);
				}
			}
		}
	}
	const double Subs = std::floor(video::SubsAt(F.S.Life.Video, F.World));
	const std::string Glance = Grouped(static_cast<int64_t>(Subs)) + (Subs == 1.0 ? " subscriber" : " subscribers");
	F.UI.Text(Glance, Bell.X - 16.0f, 37.0f, Ts(15.0f, 700, RrCream, Align::Right));
}

/** The bell's list over the page: the milestones, newest first; a click anywhere else closes it. */
void RrBellPanel(RrFrame& F)
{
	using namespace rlnet_detail;
	const std::vector<std::pair<double, std::string>>& Bell = F.S.Life.Video.Bell;
	const int Rows = std::min<int>(static_cast<int>(Bell.size()), 12);
	const float W = 460.0f;
	const float H = Rows == 0 ? 110.0f : 60.0f + Nf(Rows) * 58.0f;
	const Rect R{NetW - 24.0f - W, VideoTop - 2.0f, W, H};
	if (F.Live && F.UI.Ptr.Pressed && !RrInside(R, F.UI.Ptr.X, F.UI.Ptr.Y) && !RrInside({NetW - 64.0f, 10.0f, 40.0f, 40.0f}, F.UI.Ptr.X, F.UI.Ptr.Y))
	{
		F.U.BellOpen = false;
		return;
	}
	F.C.GlowRoundRect(R, 14.0f, Hex(0x000000, 0.5f), 24.0f);
	F.C.FillRoundRect(R, 14.0f, RrPanel);
	F.C.StrokeRoundRect(R, 14.0f, RrLine, 1.0f);
	F.UI.Text("Milestones", R.X + 20.0f, R.Y + 34.0f, Ts(16.0f, 900, RrCream));
	if (Rows == 0)
	{
		NetParagraph(F.C, RrBellEmpty, R.X + 20.0f, R.Y + 66.0f, W - 40.0f, 14.0f, 500, RrMuted, 20.0f, 2);
		return;
	}
	float Y = R.Y + 56.0f;
	for (int I = 0; I < Rows; ++I)
	{
		const auto& B = Bell[Bell.size() - 1 - static_cast<size_t>(I)];
		const bool Fresh = F.U.BellFresh.count(VideoBellKey(B)) > 0;
		if (I > 0)
		{
			F.C.FillRect({R.X + 20.0f, Y - 4.0f, W - 40.0f, 1.0f}, RrLine);
		}
		if (Fresh)
		{
			F.C.FillCircle(R.X + 24.0f, Y + 15.0f, 4.0f, RrGold);
		}
		NetParagraph(F.C, B.second, R.X + 38.0f, Y + 20.0f, W - 58.0f, 14.0f, Fresh ? 700 : 500, RrCream, 18.0f, 2);
		F.UI.Text(RrAge(F.World - B.first), R.X + 38.0f, Y + 48.0f, Ts(12.0f, 600, RrMuted));
		Y += 58.0f;
	}
}

// ------------------------------------------------------------------ STUDIO

void RrCandidateRow(RrFrame& F, const video::Candidate& Cd, const std::vector<video::Candidate>& Open, const Rect& R, bool Clickable)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const auto At = std::find(U.Pick.begin(), U.Pick.end(), Cd.Id);
	const bool Picked = At != U.Pick.end();
	const std::string Hint = Picked ? std::string() : RrJoinHint(U, Open, Cd);
	const Ui::ClickState St = F.UI.Clickable("rrcand:" + Cd.Id, R, F.Live && Clickable);
	if (St.Clicked)
	{
		RrToggle(U, Open, Cd, F.Now);
	}
	const float A0 = F.C.GetAlpha();
	if (!Hint.empty())
	{
		F.C.SetAlpha(A0 * 0.5f);
	}
	F.C.FillRoundRect(R, 12.0f, Picked ? Mix(RrPanel, RrGold, 0.12f) : St.Hover ? RrPanel2 : RrPanel);
	F.C.StrokeRoundRect(R, 12.0f, Picked ? RrGold : RrLine, Picked ? 2.0f : 1.0f);
	const Rect Glyph{R.X + 12.0f, R.Y + 17.0f, 52.0f, 52.0f};
	F.C.FillRoundRect(Glyph, 10.0f, RrGround);
	videoart::KindGlyph(F.C, Glyph.X + 26.0f, Glyph.Y + 26.0f, 30.0f, Cd.K);
	if (Picked)
	{
		// The order it goes in, for a night's Vlog or a Recap; a check for a single moment.
		F.C.FillCircle(Glyph.X + Glyph.W - 2.0f, Glyph.Y + 2.0f, 10.0f, RrGold);
		if (U.F == video::Format::Vlog || U.F == video::Format::Recap)
		{
			F.UI.Text(std::to_string(At - U.Pick.begin() + 1), Glyph.X + Glyph.W - 2.0f, Glyph.Y + 2.5f, Ts(11.0f, 900, Hex(videoart::Ink), Align::Center, Baseline::Middle));
		}
		else
		{
			NetCheck(F.C, Glyph.X + Glyph.W - 2.0f, Glyph.Y + 2.0f, 11.0f, Hex(videoart::Ink));
		}
	}
	const float X0 = R.X + 78.0f;
	const float Right = R.X + R.W - 14.0f;
	// Line 1: the kind, the chips, the days left.
	float X = X0 + F.UI.Text(video::KindLabel(Cd.K), X0, R.Y + 28.0f, Ts(13.0f, 900, RrGold)) + 10.0f;
	const std::string Left = Hint.empty() ? RrDaysLeft(Cd.Expires, F.World) : Hint;
	const float LeftW = F.UI.Text(Left, Right, R.Y + 28.0f, Ts(12.0f, 700, !Hint.empty() ? RrMuted : Cd.Expires - F.World < 2.0 * net::MinutesPerDay ? RrWarn : RrMuted, Align::Right));
	std::vector<std::pair<std::string, Color>> Tags;
	if (Cd.Auto)
	{
		Tags.push_back({"AUTO", RrSky}); // to the player only: the video never says it (U1)
	}
	if (Cd.Storytime)
	{
		Tags.push_back({"STORYTIME", RrCash});
	}
	const std::string Where = RrWhereChip(Cd);
	if (!Where.empty())
	{
		Tags.push_back({Where, RrMuted});
	}
	for (const auto& Ch : Tags)
	{
		const float W = F.UI.Measure(Ch.first, 9.5f, 700) + 9.5f * 1.3f;
		if (X + W > Right - LeftW - 8.0f)
		{
			break;
		}
		X += NetPill(F.C, Ch.first, X, R.Y + 15.0f, Ch.second, false, 9.5f) + 5.0f;
	}
	// Line 2: what happened (a long event name gives way to the pot and who). Line 3: when.
	std::string Tail;
	std::string Head = RrFact(Cd, Open, Tail);
	if (!Tail.empty())
	{
		Head = RrShorten(F.C, Head, 15.0f, 700, std::max(80.0f, Right - X0 - F.UI.Measure(Tail, 15.0f, 700)));
	}
	F.UI.Text(Head + Tail, X0, R.Y + 51.0f, Ts(15.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Right - X0));
	F.UI.Text(RrWhen(Cd.At), X0, R.Y + 71.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Right - X0));
	F.C.SetAlpha(A0);
}

void RrCandidates(RrFrame& F, const std::vector<video::Candidate>& Listed, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	NetSpaced(F.C, "CANDIDATES", RrListX + 2.0f, 100.0f, 10.0f, 900, RrMuted, 1.4f);
	const float Right = RrListX + RrListW;
	if (U.Tonight)
	{
		const std::string Chip = "TONIGHT'S STREAM";
		const float Cw = F.UI.Measure(Chip, 10.0f, 700) + 13.0f;
		const float ShowW = F.UI.Measure("Show all", 12.0f, 800);
		NetPill(F.C, Chip, Right - ShowW - 14.0f - Cw, 86.0f, RrGold, false, 10.0f);
		if (RrLink(F, "rrshowall", "Show all", Right, 100.0f, 12.0f, RrGold, Align::Right))
		{
			U.Tonight = false;
			U.ListGoal = U.ListScroll = 0.0f;
		}
	}
	else
	{
		F.UI.Text(std::to_string(Listed.size()) + " open", Right, 100.0f, Ts(12.0f, 700, RrMuted, Align::Right));
	}
	const Rect Area{RrListX, RrListTop, RrListW, VideoH - 20.0f - RrListTop};
	if (Listed.empty())
	{
		F.UI.Text(RrTonightGone, Area.X + Area.W / 2.0f, Area.Y + 60.0f, Ts(15.0f, 700, RrMuted, Align::Center, Baseline::Alphabetic, false, Area.W - 20.0f));
		return;
	}
	const float Content = Nf(Listed.size()) * (RrRowH + RrRowGap) - RrRowGap;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.ListGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * (RrRowH + RrRowGap) * 1.5f;
	}
	U.ListGoal = std::clamp(U.ListGoal, 0.0f, MaxScroll);
	U.ListScroll += (U.ListGoal - U.ListScroll) * NetFollow(F.Dt, 18.0);
	U.ListScroll = std::clamp(U.ListScroll, 0.0f, MaxScroll);
	if (std::fabs(U.ListGoal - U.ListScroll) < 0.5f)
	{
		U.ListScroll = U.ListGoal;
	}
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < Listed.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * (RrRowH + RrRowGap) - U.ListScroll;
		if (Y + RrRowH < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		RrCandidateRow(F, Listed[I], Open, {Area.X, Y, Area.W, RrRowH}, PointerIn);
	}
	F.C.PopClip();
	if (MaxScroll > 0.0f)
	{
		// A thin thumb on the right edge, so the player knows there's more.
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * (U.ListScroll / MaxScroll);
		F.C.FillRoundRect({Area.X + Area.W + 6.0f, Ty, 4.0f, Th}, 2.0f, NetA(RrCream, 0.25f));
	}
}

void RrEditor(RrFrame& F, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const float X = RrEdX, W = RrEdW;
	// FORMAT.
	NetSpaced(F.C, "FORMAT", X + 2.0f, 100.0f, 10.0f, 900, RrMuted, 1.4f);
	for (int K = 0; K < 4; ++K)
	{
		const video::Format Fm = static_cast<video::Format>(K);
		const float Bw = (W - 30.0f) / 4.0f;
		if (RrSegment(F, std::string("rrfmt") + video::FormatName(Fm), {X + Nf(K) * (Bw + 10.0f), 112.0f, Bw, 56.0f}, video::FormatName(Fm), RrFormatSub(Fm), U.F == Fm))
		{
			RrSetFormat(U, Open, Fm);
		}
	}
	// TITLE: the style, and the real title it makes from the moment's own facts.
	NetSpaced(F.C, "TITLE", X + 2.0f, 194.0f, 10.0f, 900, RrMuted, 1.4f);
	for (int K = 0; K < 5; ++K)
	{
		const video::TitleStyle Ts_ = static_cast<video::TitleStyle>(K);
		const float Bw = (W - 32.0f) / 5.0f;
		if (RrSegment(F, std::string("rrtitle") + video::TitleStyleName(Ts_), {X + Nf(K) * (Bw + 8.0f), 206.0f, Bw, 36.0f}, video::TitleStyleName(Ts_), std::string(), U.T == Ts_))
		{
			U.T = Ts_;
		}
	}
	const video::Choice Ch = RrChoice(U);
	const std::string Title = U.Pick.empty() ? std::string() : video::Title(Ch, Open, 0);
	const Rect Box{X, 252.0f, W, 68.0f};
	F.C.FillRoundRect(Box, 12.0f, RrPanel);
	F.C.StrokeRoundRect(Box, 12.0f, RrLine, 1.0f);
	if (Title.empty())
	{
		F.UI.Text(RrPickFirst, Box.X + 20.0f, Box.Y + 41.0f, Ts(16.0f, 600, RrMuted));
	}
	else
	{
		const std::string Quoted = "\xE2\x80\x9C" + Title + "\xE2\x80\x9D";
		const std::vector<std::string> Lines = NetWrap(F.C, Quoted, Box.W - 40.0f, 19.0f, 800);
		if (Lines.size() <= 1)
		{
			F.UI.Text(Quoted, Box.X + 20.0f, Box.Y + 42.0f, Ts(19.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, Box.W - 40.0f));
		}
		else
		{
			NetParagraph(F.C, Quoted, Box.X + 20.0f, Box.Y + 29.0f, Box.W - 40.0f, 17.0f, 800, RrCream, 22.0f, 2);
		}
	}
	// THUMBNAIL: the frame, drawn for real at 640 x 360.
	NetSpaced(F.C, "THUMBNAIL", X + 2.0f, 342.0f, 10.0f, 900, RrMuted, 1.4f);
	for (int K = 0; K < 5; ++K)
	{
		const video::Frame Fm = static_cast<video::Frame>(K);
		const float Bw = (W - 32.0f) / 5.0f;
		if (RrSegment(F, std::string("rrframe") + video::FrameName(Fm), {X + Nf(K) * (Bw + 8.0f), 354.0f, Bw, 34.0f}, video::FrameName(Fm), std::string(), U.Fr == Fm))
		{
			U.Fr = Fm;
		}
	}
	const Rect Th{X + (W - RrThumbW) / 2.0f, 400.0f, RrThumbW, RrThumbH};
	F.C.FillRoundRect({Th.X - 4.0f, Th.Y - 4.0f, Th.W + 8.0f, Th.H + 8.0f}, 12.0f, RrPanel);
	if (U.Pick.empty())
	{
		F.C.FillRect(Th, RrGround);
		videoart::Logo(F.C, {Th.X + Th.W / 2.0f - 40.0f, Th.Y + Th.H / 2.0f - 40.0f, 80.0f, 80.0f});
	}
	else
	{
		F.C.PushClip(Th);
		videoart::Thumbnail(F.C, Th, video::ThumbFor(Ch, Open), &F.S.Person);
		F.C.PopClip();
	}
	F.C.StrokeRoundRect({Th.X - 4.0f, Th.Y - 4.0f, Th.W + 8.0f, Th.H + 8.0f}, 12.0f, RrLine, 1.0f);
	// The edit: like a shift, the clock moves and the energy goes (§4.5); then PUBLISH.
	const double Minutes = video::EditMinutes(U.F);
	const std::string Edit = "Edit: " + RrEditTime(Minutes) + " \xC2\xB7 " + std::to_string(std::lround(video::EditEnergy(U.F))) + " energy";
	F.UI.Text(Edit, X, 800.0f, Ts(18.0f, 800, RrCream));
	F.UI.Text("It goes up at " + net::TimeLabel(F.World + Minutes) + ".", X, 824.0f, Ts(13.0f, 600, RrMuted));
	const std::vector<std::string> Why = RrPublishWhy(F.S, Ch);
	const Rect Btn{X + W - 220.0f, 776.0f, 220.0f, 56.0f};
	// Update 2 (§5.7): a video of an online table (any site's) is a strike under a deal that plays only on another one. PUBLISH
	// says so first, and the strike lands only on the second press.
	std::string Strike;
	std::string PickKey;
	for (const std::string& Id : U.Pick)
	{
		PickKey += Id + "|";
		const video::Candidate* Cand = RrFind(Open, Id);
		if (Strike.empty() && Cand && (Cand->Source == 0 || Cand->Source == 3) && Cand->Site >= 0 && Cand->Site < static_cast<int>(net::SiteId::Count))
		{
			Strike = rlsites_detail::SitesClauseWarning(F.S, static_cast<net::SiteId>(Cand->Site), false, true);
		}
	}
	const bool Armed = !Strike.empty() && U.StrikeArmed == PickKey;
	if (!Strike.empty() && Why.empty())
	{
		// Under the button: above it the line sat on the thumbnail's frame (it ends at 764).
		F.UI.Text(Armed ? std::string(rlsites_detail::SitesPublishArmed) : Strike, X + W, 856.0f, Ts(14.0f, 800, RrWarn, Align::Right, Baseline::Alphabetic, false, W));
	}
	const bool Pressed = RrButton(F, "rrpublish", Btn, Armed ? std::string("PUBLISH ANYWAY") : std::string("PUBLISH"), Why.empty());
	if (Pressed && !Strike.empty() && !Armed)
	{
		U.StrikeArmed = PickKey; // the first press only warns
	}
	else if (Pressed)
	{
		U.StrikeArmed.clear();
		const std::string Refused = F.S.Publish(Ch);
		if (Refused.empty())
		{
			// The edit runs as a time skip; the video's on the Channel the moment it's done.
			U.Pick.clear();
			U.Touched = false;
			U.Page = 1;
			U.PageAt = F.Now;
			U.GridGoal = U.GridScroll = 0.0f;
			U.Note.clear();
		}
		else
		{
			U.Note = Refused;
			U.NoteBad = true;
			U.NoteAt = F.Now;
		}
	}
	float Ry = 860.0f;
	for (const std::string& Line : Why)
	{
		Ry = NetParagraph(F.C, Line, X, Ry, W, 14.0f, 600, RrWarn, 19.0f, 2) + 4.0f;
	}
	if (!U.Note.empty() && F.Now - U.NoteAt < 6.0)
	{
		const float A0 = F.C.GetAlpha();
		F.C.SetAlpha(A0 * Nf(std::min(1.0, (6.0 - (F.Now - U.NoteAt)) / 0.6)));
		NetParagraph(F.C, U.Note, X, std::max(Ry + 2.0f, 900.0f), W, 14.0f, 700, U.NoteBad ? RrWarn : RrGold, 19.0f, 2);
		F.C.SetAlpha(A0);
	}
}

void RrSide(RrFrame& F, const std::vector<video::Candidate>& Open)
{
	using namespace rlnet_detail;
	const VideoUi& U = F.U;
	const float X = RrSideX, W = RrSideW;
	NetSpaced(F.C, "WHAT IT DOES", X + 2.0f, 100.0f, 10.0f, 900, RrMuted, 1.4f);
	const video::Choice Ch = RrChoice(U);
	// Ads come up only once they run (a Partner, past the review): before that they're no reason to pick a format.
	const video::Channel& Rerun = F.S.Life.Video;
	std::vector<std::string> Lines = video::Advice(Ch, Open, Rerun.PartnerAt >= 0.0 && Rerun.PartnerAt <= F.World);
	bool Auto = false;
	for (const std::string& Id : U.Pick)
	{
		const video::Candidate* C = RrFind(Open, Id);
		Auto = Auto || (C && C->Auto);
	}
	if (Auto)
	{
		Lines.push_back(RrAutoNote);
	}
	float Y = 130.0f;
	for (const std::string& L : Lines)
	{
		F.C.FillCircle(X + 6.0f, Y - 5.0f, 3.5f, RrGold);
		Y = NetParagraph(F.C, L, X + 20.0f, Y, W - 20.0f, 15.0f, 500, RrCream, 21.0f, 4) + 12.0f;
	}
	if (U.Pick.empty())
	{
		return;
	}
	// The tile as it'll sit on the Channel (§4.5: the preview shows it there).
	const float Ty = std::max(Y + 28.0f, 330.0f);
	NetSpaced(F.C, "ON YOUR CHANNEL", X + 2.0f, Ty, 10.0f, 900, RrMuted, 1.4f);
	video::Video V;
	V.Title = video::Title(Ch, Open, 0);
	V.Th = video::ThumbFor(Ch, Open);
	V.F = U.F;
	V.LengthMin = RrLengthMin(U.F);
	V.At = F.World;
	RrTile(F, {X, Ty + 14.0f, 300.0f, 240.0f}, V, false, 0.0);
}

void RrStudio(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const std::vector<video::Candidate>& Open = F.S.Life.Video.Open;
	if (Open.empty())
	{
		// The empty state (§4.8).
		const float Cx = NetW / 2.0f;
		videoart::Logo(F.C, {Cx - 44.0f, 300.0f, 88.0f, 88.0f});
		F.UI.Text(RrStudioEmpty, Cx, 448.0f, Ts(26.0f, 900, RrCream, Align::Center));
		F.UI.Text(RrStudioEmptyMore, Cx, 482.0f, Ts(16.0f, 500, RrMuted, Align::Center, Baseline::Alphabetic, false, 900.0f));
		return;
	}
	const std::vector<video::Candidate> Listed = RrListed(F.S, U);
	// The pick keeps only moments still open (posted from the console, gone stale); emptied that way, or never picked by
	// hand, it takes the best one listed.
	const size_t Had = U.Pick.size();
	U.Pick.erase(std::remove_if(U.Pick.begin(), U.Pick.end(), [&Open](const std::string& Id) { return RrFind(Open, Id) == nullptr; }), U.Pick.end());
	if (Had > 0 && U.Pick.empty())
	{
		U.Touched = false;
	}
	if (U.Pick.empty() && !U.Touched && !Listed.empty())
	{
		U.Pick = {Listed.front().Id};
		RrSetFormat(U, Open, U.F);
	}
	RrCandidates(F, Listed, Open);
	RrEditor(F, Open);
	RrSide(F, Open);
}

// ------------------------------------------------------------------ CHANNEL

void RrBanner(RrFrame& F, const video::ChannelView& V)
{
	using namespace rlnet_detail;
	const video::Channel& Ch = F.S.Life.Video;
	const Rect Ban{20.0f, 74.0f, NetW - 40.0f, 136.0f};
	F.C.FillRoundRect(Ban, 18.0f, Paint::Linear({Ban.X, Ban.Y}, {Ban.X + Ban.W, Ban.Y + Ban.H}, Hex(0x3d2b10), Hex(0x1b1c21)));
	F.C.PushClip(Ban);
	for (int K = 0; K < 4; ++K)
	{
		F.C.FillCircle(Ban.X + Ban.W - 420.0f - Nf(K) * 200.0f, Ban.Y + 30.0f + Nf(K % 2) * 80.0f, 64.0f + Nf(K) * 10.0f, NetA(RrGold, 0.06f));
	}
	F.C.PopClip();
	AvatarSpec Av = AvatarFor(F.S.HeroName);
	Av.Frame = AvatarFrame::Neon;
	Av.Rim = videoart::Marigold;
	DrawAvatar(F.C, Ban.X + 76.0f, Ban.Y + Ban.H / 2.0f, 46.0f, Av);
	const float Nx = Ban.X + 142.0f;
	const float NameW = F.UI.Text(F.S.HeroName, Nx, Ban.Y + 56.0f, Ts(30.0f, 900, RrCream, Align::Left, Baseline::Alphabetic, false, 560.0f));
	if (V.Partner)
	{
		// The Partner mark: a marigold seal with a check, and the word.
		const float Mx = Nx + std::min(NameW, 560.0f) + 30.0f;
		F.C.FillCircle(Mx, Ban.Y + 45.0f, 12.0f, RrGold);
		NetCheck(F.C, Mx, Ban.Y + 45.0f, 13.0f, Hex(videoart::Ink));
		NetPill(F.C, "PARTNER", Mx + 20.0f, Ban.Y + 35.0f, RrGold, true, 11.0f);
	}
	const long long Subs = static_cast<long long>(std::floor(V.Subs));
	const size_t Count = Ch.Videos.size() + Ch.Stubs.size();
	std::string Stats = Grouped(Subs) + (Subs == 1 ? " subscriber" : " subscribers") + " \xC2\xB7 " + RrViews(video::TotalViews(Ch, F.World));
	if (Count > 0)
	{
		Stats += " \xC2\xB7 " + std::to_string(Count) + (Ch.Videos.size() >= 60 ? "+" : "") + (Count == 1 ? " video" : " videos");
	}
	F.UI.Text(Stats, Nx, Ban.Y + 88.0f, Ts(15.0f, 600, Mix(RrCream, RrMuted, 0.3f), Align::Left, Baseline::Alphabetic, false, 760.0f));
	F.UI.Text("Poker \xC2\xB7 highlights from your own tables", Nx, Ban.Y + 112.0f, Ts(13.0f, 600, RrMuted));
	// Right: the money once in, else where the program stands.
	const float Mx = Ban.X + Ban.W - 330.0f;
	if (V.Partner)
	{
		NetSpaced(F.C, "BALANCE", Mx, Ban.Y + 36.0f, 10.0f, 900, RrMuted, 1.4f);
		F.UI.Text(Money(V.Balance), Mx, Ban.Y + 76.0f, Ts(32.0f, 900, RrCash, Align::Left, Baseline::Alphabetic, true));
		F.UI.Text("paid out " + Money(Ch.PaidCents) + " so far", Mx, Ban.Y + 98.0f, Ts(12.5f, 600, RrMuted));
		F.UI.Text(RrPaysOut, Mx, Ban.Y + 118.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 310.0f));
	}
	else
	{
		NetSpaced(F.C, "PARTNER PROGRAM", Mx, Ban.Y + 36.0f, 10.0f, 900, RrMuted, 1.4f);
		const bool Review = Ch.ReviewAt >= 0.0;
		F.UI.Text(Review ? "Under review" : "Not yet", Mx, Ban.Y + 74.0f, Ts(26.0f, 900, Review ? RrGold : RrCream));
		F.UI.Text(Review ? RrAdsAfterReview : RrAdsAtGate, Mx, Ban.Y + 98.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 310.0f));
	}
}

void RrGateCard(RrFrame& F, const video::ChannelView& V)
{
	using namespace rlnet_detail;
	const video::Channel& Ch = F.S.Life.Video;
	const Rect R{20.0f, 226.0f, 600.0f, 256.0f};
	F.C.FillRoundRect(R, 16.0f, RrPanel);
	F.UI.Text("Partner Program", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, RrCream));
	if (V.Partner)
	{
		F.UI.Text("since " + net::DateLabel(net::DayOf(Ch.PartnerAt)), R.X + R.W - 20.0f, R.Y + 34.0f, Ts(12.5f, 700, RrGold, Align::Right));
		F.C.FillCircle(R.X + 44.0f, R.Y + 84.0f, 20.0f, RrGold);
		NetCheck(F.C, R.X + 44.0f, R.Y + 84.0f, 22.0f, Hex(videoart::Ink));
		F.UI.Text(RrPartnerHead, R.X + 78.0f, R.Y + 91.0f, Ts(20.0f, 900, RrCream));
		float Y = NetParagraph(F.C, RrPartnerSince, R.X + 20.0f, R.Y + 136.0f, R.W - 40.0f, 14.5f, 500, RrMuted, 20.0f, 2);
		double Cents28 = 0.0;
		for (const double C : V.CentsByDay)
		{
			Cents28 += C;
		}
		F.UI.Text("Last 28 days", R.X + 20.0f, Y + 22.0f, Ts(14.0f, 700, RrCream));
		F.UI.Text(Money(static_cast<Chips>(std::floor(Cents28))), R.X + R.W - 20.0f, Y + 22.0f, Ts(18.0f, 900, RrCash, Align::Right, Baseline::Alphabetic, true));
		F.UI.Text("Balance", R.X + 20.0f, Y + 50.0f, Ts(14.0f, 700, RrCream));
		F.UI.Text(Money(V.Balance), R.X + R.W - 20.0f, Y + 50.0f, Ts(18.0f, 900, RrCash, Align::Right, Baseline::Alphabetic, true));
		return;
	}
	F.UI.Text("last 365 days", R.X + R.W - 20.0f, R.Y + 34.0f, Ts(12.5f, 600, RrMuted, Align::Right));
	// The two bars of the real gate (§4.6): 1,000 subscribers and 4,000 long-form watch hours.
	struct Bar
	{
		const char* Label;
		double Have, Need;
	};
	const Bar Bars[2] = {{"Subscribers", std::floor(V.Subs), 1000.0}, {"Watch hours", std::floor(V.WatchHours365), 4000.0}};
	for (int K = 0; K < 2; ++K)
	{
		const float Y = R.Y + 74.0f + Nf(K) * 56.0f;
		const bool Full = Bars[K].Have >= Bars[K].Need;
		F.UI.Text(Bars[K].Label, R.X + 20.0f, Y, Ts(15.0f, 700, RrCream));
		F.UI.Text(Grouped(static_cast<int64_t>(Bars[K].Have)) + " / " + Grouped(static_cast<int64_t>(Bars[K].Need)), R.X + R.W - 20.0f, Y,
			Ts(15.0f, 800, Full ? RrGold : RrCream, Align::Right));
		const Rect Track{R.X + 20.0f, Y + 12.0f, R.W - 40.0f, 10.0f};
		F.C.FillRoundRect(Track, 5.0f, RrPanel2);
		const float Frac = Nf(std::clamp(Bars[K].Have / Bars[K].Need, 0.0, 1.0));
		if (Frac > 0.0f)
		{
			F.C.FillRoundRect({Track.X, Track.Y, std::max(10.0f, Track.W * Frac), Track.H}, 5.0f, RrGold);
		}
	}
	NetParagraph(F.C, RrHoursNote, R.X + 20.0f, R.Y + 196.0f, R.W - 40.0f, 13.0f, 500, RrMuted, 18.0f, 2);
	if (Ch.ReviewAt >= 0.0)
	{
		const float Pw = NetPill(F.C, "UNDER REVIEW", R.X + 20.0f, R.Y + 220.0f, RrGold, true, 10.5f);
		F.UI.Text(RrUnderReview, R.X + 30.0f + Pw, R.Y + 234.0f, Ts(13.0f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 50.0f - Pw));
	}
	else
	{
		F.UI.Text(RrGateNext, R.X + 20.0f, R.Y + 238.0f, Ts(13.0f, 700, RrCream));
	}
}

/** The last 28 days, as three small charts on one day axis: views, subscribers gained, and revenue once in (§4.8). */
void RrChart(RrFrame& F, const video::ChannelView& V)
{
	using namespace rlnet_detail;
	const Rect R{640.0f, 226.0f, NetW - 20.0f - 640.0f, 256.0f};
	F.C.FillRoundRect(R, 16.0f, RrPanel);
	F.UI.Text("Last 28 days", R.X + 20.0f, R.Y + 34.0f, Ts(18.0f, 900, RrCream));
	const int Today = net::DayOf(F.World);
	F.UI.Text(net::DateLabel(Today - 27) + " to " + net::DateLabel(Today), R.X + R.W - 20.0f, R.Y + 34.0f, Ts(12.5f, 600, RrMuted, Align::Right));
	const float Px = R.X + 190.0f;
	const float Pw = R.W - 210.0f;
	const float Slot = Pw / 28.0f;
	struct Series
	{
		const char* Label;
		const std::array<double, 28>* Days;
		std::string Total;
		Color Col;
		float Top, H;
		bool On;
	};
	double Views = 0.0, Subs = 0.0, Cents = 0.0;
	for (int I = 0; I < 28; ++I)
	{
		Views += V.ViewsByDay[static_cast<size_t>(I)];
		Subs += V.SubsByDay[static_cast<size_t>(I)];
		Cents += V.CentsByDay[static_cast<size_t>(I)];
	}
	const Series Rows[3] = {
		{"VIEWS", &V.ViewsByDay, Grouped(static_cast<int64_t>(std::floor(Views))), RrGold, R.Y + 52.0f, 78.0f, true},
		{"SUBSCRIBERS", &V.SubsByDay, "+" + Grouped(static_cast<int64_t>(std::floor(Subs))), RrSky, R.Y + 144.0f, 42.0f, true},
		{"REVENUE", &V.CentsByDay, Money(static_cast<Chips>(std::floor(Cents))), RrCash, R.Y + 200.0f, 42.0f, V.Partner},
	};
	// The day under the pointer, across all three.
	int Hot = -1;
	if (F.Live && F.UI.Ptr.Active && F.UI.Ptr.X >= Px && F.UI.Ptr.X < Px + Pw && F.UI.Ptr.Y >= R.Y + 48.0f && F.UI.Ptr.Y <= R.Y + R.H - 8.0f)
	{
		Hot = std::clamp(static_cast<int>((F.UI.Ptr.X - Px) / Slot), 0, 27);
		F.C.FillRect({Px + Nf(Hot) * Slot, R.Y + 48.0f, Slot, R.H - 56.0f}, Rgba(255, 255, 255, 0.05f));
	}
	for (const Series& S : Rows)
	{
		NetSpaced(F.C, S.Label, R.X + 20.0f, S.Top + 12.0f, 9.5f, 900, RrMuted, 1.2f);
		if (!S.On)
		{
			F.UI.Text("\xE2\x80\x94", R.X + 20.0f, S.Top + 36.0f, Ts(18.0f, 900, RrDim));
			F.UI.Text(F.S.Life.Video.ReviewAt >= 0.0 ? RrAdsAfterReview : RrAdsAtGate, Px, S.Top + S.H / 2.0f + 5.0f, Ts(13.0f, 600, RrMuted));
			continue;
		}
		F.UI.Text(S.Total, R.X + 20.0f, S.Top + (S.H > 60.0f ? 44.0f : 36.0f), Ts(S.H > 60.0f ? 24.0f : 18.0f, 900, S.Col, Align::Left));
		double Max = 0.0;
		for (const double D : *S.Days)
		{
			Max = std::max(Max, D);
		}
		const float Base = S.Top + S.H;
		F.C.FillRect({Px, Base, Pw, 1.0f}, RrLine);
		for (int I = 0; I < 28; ++I)
		{
			const double D = (*S.Days)[static_cast<size_t>(I)];
			if (D <= 0.0 || Max <= 0.0)
			{
				continue;
			}
			const float H = std::max(1.5f, (S.H - 4.0f) * Nf(D / Max));
			// Today is a partial day: drawn fainter.
			F.C.FillRoundRect({Px + Nf(I) * Slot + Slot * 0.18f, Base - H, Slot * 0.64f, H}, 2.0f, I == 27 ? NetA(S.Col, 0.55f) : I == Hot ? Mix(S.Col, Hex(0xffffff), 0.3f) : S.Col);
		}
	}
	if (Hot >= 0)
	{
		const size_t I = static_cast<size_t>(Hot);
		std::string Tip = net::DateLabel(Today - 27 + Hot) + " \xC2\xB7 " + RrViews(V.ViewsByDay[I]) + " \xC2\xB7 +" + Grouped(static_cast<int64_t>(std::floor(V.SubsByDay[I]))) + " subscribers";
		if (V.Partner)
		{
			Tip += " \xC2\xB7 " + Money(static_cast<Chips>(std::floor(V.CentsByDay[I])));
		}
		const float Tw = F.UI.Measure(Tip, 12.5f, 700) + 20.0f;
		const float Tx = std::clamp(Px + Nf(Hot) * Slot + Slot / 2.0f - Tw / 2.0f, R.X + 8.0f, R.X + R.W - 8.0f - Tw);
		F.C.FillRoundRect({Tx, R.Y + 40.0f, Tw, 26.0f}, 8.0f, RrGround);
		F.C.StrokeRoundRect({Tx, R.Y + 40.0f, Tw, 26.0f}, 8.0f, RrLine, 1.0f);
		F.UI.Text(Tip, Tx + 10.0f, R.Y + 57.5f, Ts(12.5f, 700, RrCream));
	}
}

void RrVideos(RrFrame& F, const video::ChannelView& V)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	NetSpaced(F.C, "YOUR VIDEOS", 22.0f, 516.0f, 10.0f, 900, RrMuted, 1.4f);
	const Rect Area{20.0f, RrGridTop, NetW - 40.0f, VideoH - 14.0f - RrGridTop};
	std::vector<const video::Video*> List;
	for (const video::Video& X : Ch.Videos)
	{
		List.push_back(&X);
	}
	std::stable_sort(List.begin(), List.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	if (List.empty())
	{
		const float Cx = Area.X + Area.W / 2.0f;
		F.UI.Text(RrChannelEmpty, Cx, Area.Y + 140.0f, Ts(24.0f, 900, RrCream, Align::Center));
		F.UI.Text(RrChannelEmptyMore, Cx, Area.Y + 172.0f, Ts(15.0f, 500, RrMuted, Align::Center));
		if (!Ch.Open.empty() && RrButton(F, "rrtostudio", {Cx - 120.0f, Area.Y + 200.0f, 240.0f, 48.0f}, "Open Studio", true))
		{
			U.Page = 0;
			U.PageAt = F.Now;
		}
		return;
	}
	F.UI.Text("newest first", NetW - 22.0f, 516.0f, Ts(12.0f, 600, RrMuted, Align::Right));
	const float TileW = (Area.W - Nf(RrCols - 1) * RrTileGap) / Nf(RrCols);
	const int Rows = (static_cast<int>(List.size()) + RrCols - 1) / RrCols;
	const float Content = Nf(Rows) * RrTilePitch;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.GridGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * RrTilePitch * 0.75f;
	}
	U.GridGoal = std::clamp(U.GridGoal, 0.0f, MaxScroll);
	U.GridScroll += (U.GridGoal - U.GridScroll) * NetFollow(F.Dt, 18.0);
	U.GridScroll = std::clamp(U.GridScroll, 0.0f, MaxScroll);
	if (std::fabs(U.GridGoal - U.GridScroll) < 0.5f)
	{
		U.GridScroll = U.GridGoal;
	}
	if (!U.Confirm.empty() && F.Now - U.ConfirmAt > 5.0)
	{
		U.Confirm.clear();
	}
	const bool PointerIn = F.UI.Hover(Area);
	std::string Delete;
	F.C.PushClip(Area);
	for (size_t I = 0; I < List.size(); ++I)
	{
		const video::Video& X = *List[I];
		const float Tx = Area.X + Nf(static_cast<int>(I) % RrCols) * (TileW + RrTileGap);
		const float Ty = Area.Y + Nf(static_cast<int>(I) / RrCols) * RrTilePitch - U.GridScroll;
		if (Ty + RrTilePitch < Area.Y || Ty > Area.Y + Area.H)
		{
			continue;
		}
		const Rect R{Tx, Ty, TileW, RrTilePitch - 10.0f};
		RrTile(F, R, X, V.Partner, RrEarnedCents(X, Ch.PartnerAt, F.World));
		// Taking one down: a second press within five seconds (§4.5: its views stop; its subscribers stay).
		const bool Asked = U.Confirm == X.Id;
		const bool Hover = PointerIn && F.UI.Hover(R);
		if (Hover || Asked)
		{
			const std::string Label = Asked ? "Sure?" : "Delete";
			const float Bw = F.UI.Measure(Label, 12.0f, 800) + 18.0f;
			const Rect Btn{R.X + R.W - Bw - 6.0f, R.Y + 6.0f, Bw, 24.0f};
			const Ui::ClickState St = F.UI.Clickable("rrdel:" + X.Id, Btn, F.Live && PointerIn);
			F.C.FillRoundRect(Btn, 6.0f, Asked ? RrWarn : St.Hover ? Hex(0x000000, 0.85f) : Hex(0x000000, 0.7f));
			F.UI.Text(Label, Btn.X + Btn.W / 2.0f, Btn.Y + Btn.H / 2.0f + 0.5f, Ts(12.0f, 800, Asked ? Hex(videoart::Ink) : RrCream, Align::Center, Baseline::Middle));
			if (St.Clicked)
			{
				if (Asked)
				{
					Delete = X.Id;
				}
				else
				{
					U.Confirm = X.Id;
					U.ConfirmAt = F.Now;
				}
			}
			if (Asked)
			{
				const float Th = std::round(R.W * 9.0f / 16.0f);
				F.C.FillRoundRect({R.X + 6.0f, R.Y + Th - 32.0f, R.W - 12.0f, 26.0f}, 6.0f, Hex(0x000000, 0.8f));
				F.UI.Text(RrDeleteAgain, R.X + R.W / 2.0f, R.Y + Th - 18.5f, Ts(12.0f, 700, RrWarn, Align::Center, Baseline::Middle, false, R.W - 20.0f));
			}
		}
	}
	F.C.PopClip();
	if (!Delete.empty())
	{
		const std::string Why = F.S.DeleteVideo(Delete);
		U.Confirm.clear();
		U.Note = Why.empty() ? video::Deleted() : Why;
		U.NoteBad = !Why.empty();
		U.NoteAt = F.Now;
	}
	if (MaxScroll > 0.0f)
	{
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * (U.GridScroll / MaxScroll);
		F.C.FillRoundRect({Area.X + Area.W + 8.0f, Ty, 4.0f, Th}, 2.0f, NetA(RrCream, 0.25f));
	}
	if (!U.Note.empty() && F.Now - U.NoteAt < 6.0)
	{
		const float Tw = F.UI.Measure(U.Note, 14.0f, 700) + 32.0f;
		const Rect T{NetW / 2.0f - Tw / 2.0f, VideoH - 64.0f, Tw, 40.0f};
		F.C.FillRoundRect(T, 12.0f, RrGround);
		F.C.StrokeRoundRect(T, 12.0f, U.NoteBad ? RrWarn : RrGold, 1.5f);
		F.UI.Text(U.Note, T.X + T.W / 2.0f, T.Y + 21.0f, Ts(14.0f, 700, RrCream, Align::Center, Baseline::Middle));
	}
}

void RrChannel(RrFrame& F)
{
	// The page's numbers (video::View: 28 days over every video, about 3,400 exp() calls) are read once a world minute, or
	// at once when the channel changes (an upload, a delete, a step, a grant).
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const double Minute = std::floor(F.World);
	if (!U.ViewOk || U.ViewMinute != Minute || U.ViewVideos != Ch.Videos.size() || U.ViewStepped != Ch.SteppedTo || U.ViewSubs != Ch.Subs || U.ViewPartner != Ch.PartnerAt ||
		U.ViewHours != Ch.GrantedHours || U.ViewBalance != Ch.BalanceCents)
	{
		U.View = F.S.VideoView();
		U.ViewOk = true;
		U.ViewMinute = Minute;
		U.ViewVideos = Ch.Videos.size();
		U.ViewStepped = Ch.SteppedTo;
		U.ViewSubs = Ch.Subs;
		U.ViewPartner = Ch.PartnerAt;
		U.ViewHours = Ch.GrantedHours;
		U.ViewBalance = Ch.BalanceCents;
	}
	const video::ChannelView V = U.View;
	RrBanner(F, V);
	RrGateCard(F, V);
	RrChart(F, V);
	RrVideos(F, V);
}
} // namespace rlvideo_detail

// ------------------------------------------------------------------ the entry points (RiverLineShared.h's callers)

namespace rlvideo_detail
{
void VideoOpen(RiverLine& RL, int Page, double Now)
{
	VideoUi& U = VideoUiOf(&RL);
	U.Page = std::clamp(Page, 0, 1);
	U.PageAt = Now;
	U.BellOpen = false;
	RL.OpenApp(RiverLine::App::Video, Now);
}

void VideoOpenTonight(RiverLine& RL, double From, double To, double Now)
{
	VideoUi& U = VideoUiOf(&RL);
	U.Tonight = true;
	U.KeepTonight = true;
	U.TonightFrom = From;
	U.TonightTo = To;
	U.Pick.clear();
	U.Touched = false;
	U.ListGoal = U.ListScroll = 0.0f;
	VideoOpen(RL, 0, Now);
}

int VideoCutCount(const Session& S, double From, double To)
{
	int N = 0;
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		N += VideoFromStream(C, From, To) ? 1 : 0;
	}
	return N;
}

bool VideoDot(const RiverLine* RL, const Session& S)
{
	VideoUi& U = VideoUiOf(RL);
	VideoPrime(U, S);
	int Fresh = 0;
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		Fresh += U.SeenCandidates.count(C.Id) == 0 ? 1 : 0;
	}
	return Fresh >= 3 || VideoUnseenBell(U, S) > 0;
}

void ForgetVideo(const RiverLine* RL)
{
	VideoUiStates().erase(RL);
}

std::vector<std::string> VideoUiInventory()
{
	return {RrStudioEmpty, RrStudioEmptyMore, RrChannelEmpty, RrChannelEmptyMore, RrPickFirst, RrAutoNote, RrTonightGone, RrVlogFull, RrRecapFull, RrDeleteAgain,
		RrHoursNote, RrGateNext, RrUnderReview, RrPartnerHead, RrPartnerSince, RrAdsAtGate, RrAdsAfterReview, RrPaysOut, RrBellEmpty, "It goes up at 3:40 AM."};
}
} // namespace rlvideo_detail

void RiverLine::VideoApp(double Now)
{
	using namespace rlnet_detail;
	using namespace rlvideo_detail;
	VideoUi& U = VideoUiOf(this);
	VideoPrime(U, S);
	// The app just came up: a filter from Kast's Cut highlights is kept once; otherwise the list shows everything.
	if (Now - U.LastDrawn > 0.5)
	{
		if (!U.KeepTonight)
		{
			U.Tonight = false;
		}
		U.KeepTonight = false;
		U.BellOpen = false;
	}
	U.LastDrawn = Now;
	RrFrame F{*this, *C, UI, S, U, Now, World, Dt, !Previewing};
	C->FillRect({0.0f, 0.0f, NetW, VideoH}, RrGround);
	RrHeader(F);
	// While the bell's list is up, the page under it takes no clicks.
	const Pointer Real = UI.Ptr;
	if (U.BellOpen)
	{
		UI.Ptr.Pressed = false;
		UI.Ptr.Released = false;
		UI.Ptr.Wheel = 0.0f;
	}
	if (U.Page == 0)
	{
		RrStudio(F);
	}
	else
	{
		RrChannel(F);
	}
	UI.Ptr = Real;
	if (U.BellOpen)
	{
		RrBellPanel(F);
	}
	// A visit: every candidate on the list now has been seen (the taskbar's dot counts from here).
	for (const video::Candidate& Cand : S.Life.Video.Open)
	{
		U.SeenCandidates.insert(Cand.Id);
	}
}
} // namespace ui
} // namespace ss
