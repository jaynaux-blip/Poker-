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
//
// Update 3 (SPEC §7.1, §8.2): split without a change. This file keeps the shell, the header, the bell, Channel and the
// helpers' bodies; Studio is RiverLineVideoStudio.cpp; the look, the copy, VideoUi and RrFrame are RiverLineVideoShared.h.
//
// Rerun 2.0 (C3, ui-rerun; update 3 SPEC §5.2, §6.6.5-§6.6.6, SPEC_CREATORS §5.3 R1-R2): the header's four pages HOME ·
// STUDIO · CHANNEL · ANALYTICS (indices 3, 0, 1, 2), the public subscriber glance and the bell with Rerun 2.0's lines (the
// verdicts, breakouts, the Binge wave and Late Bloomer in their accents); the Channel as the public page (VIDEOS · CUTS ·
// SERIES · ABOUT, five columns of 300 x 169 tiles, views and ages only: never dislikes, revenue or a balance; the gate's two
// bars before Partner; the Outlier shelf; the upload streak); the thumbnails (a recipe through ui-thumbs' compositor in
// Rerun's own cache pool, at most two painted uncached a frame, a skeleton until then; an Update 2 frame as Update 2 drew
// it) and the caches every page reads the core through.
#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "../Game/VideoAlgoPriv.h"
#include "RiverLineShared.h"
#include "RiverLineVideoShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/Avatars.h"
#include "ShortStack/UI/Portrait.h"
#include "ShortStack/UI/ThumbArt.h"
#include "ShortStack/UI/VideoArt.h"

#include <algorithm>
#include <array>
#include <cmath>
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
// Rerun's look, its copy, the side state (VideoUi) and RrFrame: RiverLineVideoShared.h (update 3's split, SPEC §8.2).

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
	case video::Format::Count: break;
	}
	return 8.0;
}

const char* RrFormatSub(video::Format F)
{
	switch (F)
	{
	case video::Format::Short: return "45 s \xC2\xB7 one moment";
	case video::Format::Vlog: return "one night";
	case video::Format::Recap: return "one run";
	case video::Format::Breakdown: break;
	case video::Format::Count: break;
	}
	return "one hand";
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

/**
 * The pick Studio's preview and PUBLISH use: the moments, the format, the style and (Rerun 2.0) the picked title template,
 * the recipes (the selected one, or the Duel's two or three), the length, the ad load, the clean edit, the Next-up card and
 * the series. With no title options or recipes from the library it is an Update 2 choice set (its own title and frame).
 */
video::Choice RrChoice(const VideoUi& U)
{
	video::Choice Ch;
	Ch.Candidates = U.Pick;
	Ch.F = U.F;
	Ch.T = U.T;
	Ch.Fr = U.Fr;
	Ch.Shuffle = U.TitleShuffle;
	if (!U.Titles.empty())
	{
		Ch.TitleTpl = U.Titles[static_cast<size_t>(std::clamp(U.TitlePick, 0, static_cast<int>(U.Titles.size()) - 1))].Key;
	}
	if (U.F != video::Format::Short && !U.Thumbs.empty())
	{
		if (U.DuelOn && U.Duel.size() >= 2)
		{
			for (const int K : U.Duel)
			{
				if (K >= 0 && K < static_cast<int>(U.Thumbs.size()) && Ch.Thumbs.size() < 3)
				{
					Ch.Thumbs.push_back(U.Thumbs[static_cast<size_t>(K)]);
				}
			}
		}
		else
		{
			Ch.Thumbs.push_back(U.Thumbs[static_cast<size_t>(std::clamp(U.ThumbPick, 0, static_cast<int>(U.Thumbs.size()) - 1))]);
		}
	}
	Ch.Len = U.F == video::Format::Short ? video::LengthOpt::Standard : U.Len;
	Ch.Ads = U.F == video::Format::Short ? video::AdLoad::Standard : U.Ads;
	Ch.CleanEdit = U.CleanEdit;
	Ch.NextUp = U.NextUp;
	Ch.Series = U.Series;
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

/** A row clicked: the pick it makes for the format (one for a Cut or a Breakdown; a toggle in a night's Vlog or a Recap). */
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

// Rerun 2.0's words.

const char* RrFormatName(video::Format F)
{
	switch (F)
	{
	case video::Format::Short: return "Cut"; // the enum is unchanged (§1.10: a Cut, not a Short)
	case video::Format::Vlog: return "Vlog";
	case video::Format::Recap: return "Recap";
	case video::Format::Breakdown: break;
	case video::Format::Count: break;
	}
	return "Breakdown";
}

const char* RrStyleName(video::TitleStyle T)
{
	switch (T)
	{
	case video::TitleStyle::Hype: return "Loud"; // TitleStyle::Hype, shown "Loud" everywhere (§6.2)
	case video::TitleStyle::Teaching: return "Teaching";
	case video::TitleStyle::Question: return "Question";
	case video::TitleStyle::Plain: return "Plain";
	case video::TitleStyle::Story: break;
	case video::TitleStyle::Count: break;
	}
	return "Story";
}

std::string RrClock(double Minutes)
{
	const long long S = std::llround(std::max(0.0, Minutes) * 60.0);
	if (S >= 3600)
	{
		return std::to_string(S / 3600) + ":" + rlnet_detail::NetPad2((S / 60) % 60) + ":" + rlnet_detail::NetPad2(S % 60);
	}
	return std::to_string(S / 60) + ":" + rlnet_detail::NetPad2(S % 60);
}

std::string RrCount(double V)
{
	return Grouped(static_cast<int64_t>(std::floor(std::max(0.0, V))));
}

std::string RrPlural(double V, const char* One, const char* Many)
{
	return RrCount(V) + " " + (std::floor(std::max(0.0, V)) == 1.0 ? One : Many);
}

std::string RrShort(double V)
{
	const long long N = static_cast<long long>(std::floor(std::max(0.0, V)));
	if (N >= 1000000)
	{
		return Fixed(static_cast<double>(N) / 1000000.0, N >= 10000000 ? 0 : 1) + "M";
	}
	if (N >= 10000)
	{
		return std::to_string(N / 1000) + "K";
	}
	if (N >= 1000)
	{
		return Fixed(std::floor(static_cast<double>(N) / 100.0) / 10.0, 1) + "K";
	}
	return std::to_string(N);
}

std::string RrPct(double Fraction, int Digits)
{
	return Fixed(100.0 * Fraction, Digits) + "%";
}

std::string RrCents(double Cents)
{
	return Money(static_cast<Chips>(std::floor(Cents + 1e-6)));
}

std::string RrSubsPublic(double Subs)
{
	const long long N = static_cast<long long>(std::floor(std::max(0.0, Subs)));
	return (N >= 1000 ? RrShort(static_cast<double>(N)) : std::to_string(N)) + (N == 1 ? " subscriber" : " subscribers");
}

double RrLikeRatio(double Likes, double Dislikes)
{
	const double Up = std::floor(std::max(0.0, Likes)), Down = std::floor(std::max(0.0, Dislikes));
	return Up + Down > 0.0 ? Up / (Up + Down) : 1.0;
}

void RrSubsShown(double Net, double Lost, double& NetShown, double& LostShown, double& GainedShown)
{
	NetShown = Net >= 0.0 ? std::floor(Net) : -std::floor(-Net);
	LostShown = std::floor(std::max(0.0, Lost));
	GainedShown = std::max(0.0, NetShown + LostShown);
}

const char* RrVerdictName(int Label)
{
	return video::valgo::VerdictName(Label);
}

Color RrVerdictColor(int Label)
{
	switch (Label)
	{
	case 0: return RrBad;
	case 1: return RrWarn;
	case 2: return RrCream;
	default: return RrGood;
	}
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
		Ts(R.H < 40.0f ? 14.0f : 18.0f, 900, Enabled ? Hex(videoart::Ink) : Rgba(160, 160, 160, 0.95f), Align::Center, Baseline::Middle, false, B.W - 20.0f));
	return St.Clicked;
}

/** A link in Rerun's text (Show all, Delete): true when clicked. */
bool RrLink(RrFrame& F, const std::string& Id, const std::string& Label, float X, float Y, float Size, const Color& Col, Align A)
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

/**
 * A video as Rerun shows it in public: the thumbnail (cached; the player's own look), its length, title, views and age.
 * Earnings (Update 2's Channel) only where the caller is the player's own page; the Channel never passes them now (§6.6.6).
 */
void RrTile(RrFrame& F, const Rect& R, const video::Video& V, bool Earnings, double EarnedCents)
{
	const Rect Th{R.X, R.Y, R.W, std::round(R.W * 9.0f / 16.0f)};
	F.C.FillRoundRect(Th, 8.0f, RrGround);
	RrThumb(F, Th, V, RrGridLod);
	F.C.StrokeRoundRect(Th, 8.0f, RrLine, 1.0f);
	RrLengthBadge(F, Th, V);
	const std::vector<std::string> Lines = rlnet_detail::NetWrap(F.C, V.Title, R.W, 14.0f, 700);
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

void RrCard(RrFrame& F, const Rect& R, const std::string& Title, const std::string& Right)
{
	F.C.FillRoundRect(R, 10.0f, RrPanel);
	F.C.StrokeRoundRect({R.X + 0.5f, R.Y + 0.5f, R.W - 1.0f, R.H - 1.0f}, 9.5f, RrLine, 1.0f);
	if (!Title.empty())
	{
		F.UI.Text(Title, R.X + 18.0f, R.Y + 26.0f, Ts(11.5f, 800, RrMuted, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
	}
	if (!Right.empty())
	{
		F.UI.Text(Right, R.X + R.W - 18.0f, R.Y + 26.0f, Ts(11.5f, 600, RrDim, Align::Right, Baseline::Alphabetic, false, R.W * 0.55f));
	}
}

float RrTag(Canvas& Cv, const std::string& Label, float X, float Y, const Color& Col, bool Solid, float Size)
{
	const float W = Cv.Measure(Label, Size, 900) + 16.0f;
	const float H = Size + 8.0f;
	Cv.FillRoundRect({X, Y, W, H}, H / 2.0f, Solid ? Col : rlnet_detail::NetA(Col, 0.18f));
	Cv.Text(Label, X + W / 2.0f, Y + H / 2.0f + 0.5f, Ts(Size, 900, Solid ? Hex(videoart::Ink) : Col, Align::Center, Baseline::Middle));
	return W;
}

void RrBars5(Canvas& Cv, float X, float Y, int N, const Color& Col, float W, float H, float Gap, int Ghost)
{
	for (int B = 0; B < 5; ++B)
	{
		const Rect R{X + rlnet_detail::Nf(B) * (W + Gap), Y, W, H};
		if (B < N)
		{
			Cv.FillRoundRect(R, 2.0f, Col);
		}
		else if (B < Ghost)
		{
			Cv.FillRoundRect(R, 2.0f, rlnet_detail::NetA(Col, 0.45f));
			Cv.StrokeRoundRect({R.X - 1.5f, R.Y - 1.5f, R.W + 3.0f, R.H + 3.0f}, 3.0f, rlnet_detail::NetA(RrCream, 0.9f), 1.2f);
		}
		else
		{
			Cv.FillRoundRect(R, 2.0f, RrPanel2);
		}
	}
}

bool RrChip(RrFrame& F, const std::string& Id, const Rect& R, const std::string& Label, bool On, float Size)
{
	const Ui::ClickState St = F.UI.Clickable(Id, R, F.Live);
	if (On)
	{
		F.C.FillRoundRect(R, R.H / 2.0f, RrPanel2);
		F.C.StrokeRoundRect(R, R.H / 2.0f, RrGold, 1.2f);
	}
	else if (St.Hover)
	{
		F.C.FillRoundRect(R, R.H / 2.0f, rlnet_detail::NetA(RrPanel2, 0.6f));
	}
	F.UI.Text(Label, R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, Ts(Size, On ? 800 : 600, On ? RrCream : St.Hover ? Mix(RrMuted, RrCream, 0.5f) : RrMuted, Align::Center, Baseline::Middle, false, R.W - 8.0f));
	return St.Clicked;
}

const chart::Theme& RrTheme()
{
	static const chart::Theme T = chart::Rerun();
	return T;
}

Color RrBellAccent(const std::string& Line)
{
	if (Line.find("breaking out") != std::string::npos || Line.find("next to other videos") != std::string::npos || Line.find("bingeing") != std::string::npos ||
		Line.find("pushing") != std::string::npos)
	{
		return RrGold;
	}
	if (Line.find("woke up") != std::string::npos || Line.find("Rain Check") != std::string::npos || Line.find("Upload streak") != std::string::npos)
	{
		return RrGood;
	}
	if (Line.find("stopped testing") != std::string::npos || Line.find("extra hour") != std::string::npos)
	{
		return RrWarn;
	}
	return RrCream;
}

bool RrIsGrant(const video::Video& V)
{
	return video::valgo::IsGrant(V);
}

const video::Video* RrLaunchVideo(const Session& S, double World)
{
	const video::Video* Best = nullptr;
	for (const video::Video& V : S.Life.Video.Videos)
	{
		if (V.X.On && !RrIsGrant(V) && World >= V.At && World < V.At + 48.0 * 60.0 && (!Best || V.At > Best->At))
		{
			Best = &V;
		}
	}
	return Best;
}

double RrCarryAt(const video::Channel& Ch, double UpAt)
{
	const video::ChannelX& X = Ch.X;
	if (!X.On)
	{
		return 1.0;
	}
	const bool Back = X.LastUploadDay >= 0 && video::valgo::DayOf(UpAt) - X.LastUploadDay >= video::valgo::VaDormantDays;
	return Back ? video::valgo::MomCOf(X.MomH + video::valgo::VaMomWelcome) : X.Momentum;
}

const video::Video* RrVideo(const Session& S, const std::string& Id)
{
	for (const video::Video& V : S.Life.Video.Videos)
	{
		if (V.Id == Id)
		{
			return &V;
		}
	}
	return nullptr;
}

void RrOpenVideo(RrFrame& F, const std::string& Id)
{
	F.U.Page = RrPageAnalytics;
	F.U.PageAt = F.Now;
	F.U.Sub = 1;
	F.U.One = Id;
	F.U.OneTab = 0;
	F.U.BellOpen = false;
}

// ------------------------------------------------------------------ thumbnails

namespace
{
uint64_t RrMix(uint64_t H, uint64_t V)
{
	H ^= V + 0x9E3779B97F4A7C15ull + (H << 6) + (H >> 2);
	return H;
}

uint64_t RrFloatKey(float V)
{
	return static_cast<uint64_t>(std::llround(static_cast<double>(V) * 8.0));
}

uint64_t RrLookHash(const hero::Character& C)
{
	uint64_t H = 0x52455255ull; // "RERU"
	for (int K = 0; K < hero::SlotCount; ++K)
	{
		H = RrMix(H, static_cast<uint64_t>(static_cast<int64_t>(C.Appearance.Get(static_cast<hero::Slot>(K))) + 1000));
	}
	return RrMix(H, static_cast<uint64_t>(C.Age));
}
} // namespace

namespace
{
/** A thumbnail not painted yet: the panel and two bars where the words go (V12's skeleton tile). */
void RrSkeleton(Canvas& Cv, const Rect& R)
{
	Cv.FillRect(R, Mix(RrPanel, RrPanel2, 0.5f));
	Cv.FillRoundRect({R.X + R.W * 0.08f, R.Y + R.H * 0.62f, R.W * 0.46f, std::max(3.0f, R.H * 0.08f)}, 2.0f, RrPanel2);
	Cv.FillRoundRect({R.X + R.W * 0.08f, R.Y + R.H * 0.76f, R.W * 0.30f, std::max(3.0f, R.H * 0.08f)}, 2.0f, RrPanel2);
}

} // namespace

void RrPooledThumb(RrFrame& F, const Rect& R, uint64_t Key, const std::function<void(Canvas&)>& Paint)
{
	// The frame's allowance is ui-thumbs' (thumbart::Budget): an Update 2 frame waits its turn as a recipe does.
	const bool Have = F.C.IsCached(Key);
	if (!Have && F.U.Paint.Painted >= F.U.Paint.Max)
	{
		++F.U.Paint.Waiting;
		RrSkeleton(F.C, R);
		return;
	}
	if (!Have)
	{
		++F.U.Paint.Painted;
	}
	F.C.PushClip(R);
	F.C.Cached(Key, R.X, R.Y, Paint, Canvas::CachePool::Rerun);
	F.C.PopClip();
}

void RrFlag(Canvas& Cv, float X, float Y, const Color& Col)
{
	Cv.FillRect({X, Y, 1.6f, 13.0f}, Col);
	Cv.FillPolygon({{X + 1.6f, Y}, {X + 10.0f, Y + 3.0f}, {X + 1.6f, Y + 6.0f}}, Col);
}

void RrSpecThumb(RrFrame& F, const Rect& R, const video::ThumbSpec& Sp, const video::Thumb& Facts)
{
	// ui-thumbs' contract (ThumbArt.h): its cache key, its pool, its LOD below 211 units, its skeleton.
	thumbart::Draw(F.C, R, Sp, thumbart::LookFor(Sp, F.S.Life.Video, F.S.Person), Facts, F.U.Paint);
}

void RrWorldThumb(RrFrame& F, const Rect& R, const video::WorldVideo& W, bool Lod)
{
	thumbart::Draw(F.C, R, W.Spec, hero::CharacterFor(W.Creator), video::Thumb(), F.U.Paint, Lod || thumbart::LodFor(R.W));
}

void RrLegacyThumb(RrFrame& F, const Rect& R, const video::Thumb& T)
{
	uint64_t Key = 0x6c6567616379ull; // "legacy"
	Key = RrMix(Key, static_cast<uint64_t>(T.Fr));
	Key = RrMix(Key, static_cast<uint64_t>(T.Mood + 100));
	Key = RrMix(Key, Fnv1a(T.Cards));
	Key = RrMix(Key, Fnv1a(T.BigText));
	Key = RrMix(Key, T.Accent);
	Key = RrMix(Key, Fnv1a(T.Opponent));
	Key = RrMix(Key, static_cast<uint64_t>(T.Site + 100));
	Key = RrMix(Key, F.U.LookKey);
	Key = RrMix(Key, RrFloatKey(R.W));
	Key = RrMix(Key, RrFloatKey(R.H));
	const float W = R.W, H = R.H;
	const hero::Character* Who = &F.S.Person;
	RrPooledThumb(F, R, Key, [&T, Who, W, H](Canvas& Cv) { videoart::Thumbnail(Cv, {0.0f, 0.0f, W, H}, T, Who); });
}

void RrThumb(RrFrame& F, const Rect& R, const video::Video& V, bool Lod)
{
	if (V.X.On)
	{
		const video::ThumbSpec& Sp = V.X.Spec[static_cast<size_t>(std::clamp<int>(V.X.DuelWin, 0, 2))];
		if (Sp.Arch >= 0)
		{
			if (Lod)
			{
				// VideoThumb's own call with the page's LOD (ThumbArt.cpp: the shown recipe, its frozen Look, the video's facts).
				thumbart::Draw(F.C, R, Sp, thumbart::LookFor(Sp, F.S.Life.Video, F.S.Person), V.Th, F.U.Paint, true);
				return;
			}
			thumbart::VideoThumb(F.C, R, V, F.S.Life.Video, F.S.Person, F.U.Paint);
			return;
		}
	}
	// An Update 2 frame: the same drawing (videoart::Thumbnail), replayed from Rerun's pool after its first paint (a page of
	// 60 rows can't redraw 60 of them a frame).
	RrLegacyThumb(F, R, V.Th);
}

void RrLengthBadge(RrFrame& F, const Rect& Th, const video::Video& V)
{
	const std::string Length = RrClock(V.LengthMin);
	const float Size = Th.W < 160.0f ? 10.0f : 11.5f;
	const float Lw = F.UI.Measure(Length, Size, 800, true) + 12.0f;
	const float Bh = Size + 8.5f;
	F.C.FillRoundRect({Th.X + Th.W - Lw - 6.0f, Th.Y + Th.H - Bh - 6.0f, Lw, Bh}, 4.0f, Hex(0x000000, 0.78f));
	F.UI.Text(Length, Th.X + Th.W - 12.0f, Th.Y + Th.H - 6.0f - Bh / 2.0f + 0.5f, Ts(Size, 800, RrCream, Align::Right, Baseline::Middle, true));
	if (V.F == video::Format::Short)
	{
		const float Cw = F.UI.Measure("CUT", Size - 1.0f, 900) + 12.0f;
		F.C.FillRoundRect({Th.X + 6.0f, Th.Y + Th.H - Bh - 6.0f, Cw, Bh}, 4.0f, RrGold);
		F.UI.Text("CUT", Th.X + 6.0f + Cw / 2.0f, Th.Y + Th.H - 6.0f - Bh / 2.0f + 0.5f, Ts(Size - 1.0f, 900, Hex(videoart::Ink), Align::Center, Baseline::Middle));
	}
}

// ------------------------------------------------------------------ the core's numbers, cached (§5.10)

const video::VideoStats& RrStats(RrFrame& F, const video::Video& V)
{
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	RrStatsEntry& E = U.Stats[V.Id];
	const double Minute = std::floor(F.World);
	const bool Moved = E.Stepped != Ch.SteppedTo || E.Comps != V.X.Comps.size() || E.Ads != static_cast<int>(V.X.Ads) || E.Launch != static_cast<int>(V.X.Launch) ||
		E.Repacks != V.X.Repacks;
	// A page of rows refreshes a few a frame (the budget); a video never read yet, or its parts changed, reads at once.
	if (E.Minute < 0.0 || (E.Minute != Minute && (Moved || U.StatsBudget > 0)))
	{
		if (E.Minute >= 0.0 && !Moved)
		{
			--U.StatsBudget;
		}
		E.S = video::StatsOf(Ch, V, F.World);
		E.Minute = Minute;
		E.Stepped = Ch.SteppedTo;
		E.Comps = V.X.Comps.size();
		E.Ads = static_cast<int>(V.X.Ads);
		E.Launch = static_cast<int>(V.X.Launch);
		E.Repacks = V.X.Repacks;
	}
	return E.S;
}

const video::Range& RrRange(RrFrame& F, int Days)
{
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const double Minute = std::floor(F.World);
	if (U.RangeDays != Days || U.RangeMinute != Minute || U.RangeStepped != Ch.SteppedTo || U.RangeVideos != Ch.Videos.size())
	{
		U.Range = video::StatsRange(Ch, F.World, Days);
		U.Before = video::StatsRange(Ch, F.World - static_cast<double>(Days) * net::MinutesPerDay, Days);
		U.RangeDays = Days;
		U.RangeMinute = Minute;
		U.RangeStepped = Ch.SteppedTo;
		U.RangeVideos = Ch.Videos.size();
	}
	return U.Range;
}

const video::RealtimeView& RrRealtime(RrFrame& F)
{
	VideoUi& U = F.U;
	const double Minute = std::floor(F.World);
	if (U.RtMinute != Minute || U.RtVideos != F.S.Life.Video.Videos.size())
	{
		U.Rt = video::Realtime(F.S.Life.Video, F.World);
		U.RtMinute = Minute;
		U.RtVideos = F.S.Life.Video.Videos.size();
	}
	return U.Rt;
}

void RrDaily(RrFrame& F)
{
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const double Day = std::floor(F.World / net::MinutesPerDay);
	if (U.DayKey == Day && U.DayStepped == Ch.SteppedTo && U.DayVideos == Ch.Videos.size())
	{
		return;
	}
	U.DayKey = Day;
	U.DayStepped = Ch.SteppedTo;
	U.DayVideos = Ch.Videos.size();
	U.Health = video::HealthOf(Ch, F.World);
	U.Audience = video::AudienceOf(Ch, F.World);
	U.Trends = video::Trends(F.S.Life, F.S.Living(), F.World);
	U.Market = video::AdMarket(Ch, F.World);
	U.WorldVids = video::WorldVideos(F.S.Living(), F.World, 24);
}

const std::vector<video::IdeaCard>& RrIdeas(RrFrame& F)
{
	VideoUi& U = F.U;
	const double Bucket = std::floor(F.World / 10.0);
	if (U.IdeasMinute != Bucket || U.IdeasOpen != F.S.Life.Video.Open.size())
	{
		U.Ideas = F.S.VideoIdeas();
		U.IdeasMinute = Bucket;
		U.IdeasOpen = F.S.Life.Video.Open.size();
	}
	return U.Ideas;
}

// ------------------------------------------------------------------ the header and the bell

void RrHeader(RrFrame& F)
{
	using namespace rlnet_detail;
	F.C.FillRect({0.0f, 0.0f, NetW, VideoTop}, RrBar);
	F.C.FillRect({0.0f, VideoTop - 1.0f, NetW, 1.0f}, RrLine);
	videoart::Logo(F.C, {22.0f, 12.0f, 36.0f, 36.0f});
	F.UI.Text("Rerun", 68.0f, 40.0f, Ts(26.0f, 900, RrCream));
	// HOME · STUDIO · CHANNEL · ANALYTICS (§5.2): shown in this order, the pages keep their indices (Mail opens 1).
	const char* Tabs[4] = {"HOME", "STUDIO", "CHANNEL", "ANALYTICS"};
	const int Pages[4] = {RrPageHome, RrPageStudio, RrPageChannel, RrPageAnalytics};
	float X = 172.0f;
	for (int K = 0; K < 4; ++K)
	{
		const float W = F.UI.Measure(Tabs[K], 15.0f, 800) + 28.0f;
		const Rect R{X, 10.0f, W, 40.0f};
		const Ui::ClickState St = F.UI.Clickable(std::string("rrtab") + Tabs[K], R, F.Live);
		if (St.Clicked)
		{
			F.U.Page = Pages[K];
			F.U.PageAt = F.Now;
			F.U.Confirm.clear();
			if (Pages[K] == RrPageAnalytics)
			{
				F.U.One.clear(); // the tab goes back to the page, out of one video's
			}
		}
		const bool On = F.U.Page == Pages[K];
		F.UI.Text(Tabs[K], R.X + 14.0f, R.Y + 26.0f, Ts(15.0f, 800, On ? RrCream : St.Hover ? Mix(RrMuted, RrCream, 0.5f) : RrMuted));
		if (On)
		{
			F.C.FillRoundRect({R.X + 10.0f, VideoTop - 4.0f, W - 20.0f, 3.0f}, 1.5f, RrGold);
		}
		X += W + 6.0f;
	}
	// Right: the bell, and the channel at a glance (public rounding, §5.1; exact numbers are Analytics').
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
	const video::Channel& Ch = F.S.Life.Video;
	const double Subs = std::floor(video::SubsAt(Ch, F.World));
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	F.UI.Text(RrSubsPublic(Subs), Bell.X - 16.0f, 30.0f, Ts(13.5f, 700, RrCream, Align::Right));
	F.UI.Text((Partner ? std::string("Partner \xC2\xB7 ") : std::string()) + F.S.HeroName, Bell.X - 16.0f, 47.0f, Ts(11.5f, 600, RrMuted, Align::Right, Baseline::Alphabetic, false, 360.0f));
}

/** The bell's list over the page: the milestones and Rerun 2.0's lines, newest first; a click anywhere else closes it. */
void RrBellPanel(RrFrame& F)
{
	using namespace rlnet_detail;
	const std::vector<std::pair<double, std::string>>& Bell = F.S.Life.Video.Bell;
	const int Rows = std::min<int>(static_cast<int>(Bell.size()), 12);
	const float W = 460.0f;
	const float H = Rows == 0 ? 110.0f : 60.0f + Nf(Rows) * 62.0f;
	const Rect R{NetW - 24.0f - W, VideoTop - 2.0f, W, H};
	if (F.Live && F.UI.Ptr.Pressed && !RrInside(R, F.UI.Ptr.X, F.UI.Ptr.Y) && !RrInside({NetW - 64.0f, 10.0f, 40.0f, 40.0f}, F.UI.Ptr.X, F.UI.Ptr.Y))
	{
		F.U.BellOpen = false;
		return;
	}
	F.C.GlowRoundRect(R, 14.0f, Hex(0x000000, 0.5f), 24.0f);
	F.C.FillRoundRect(R, 14.0f, Hex(0x23262e));
	F.C.StrokeRoundRect(R, 14.0f, RrLine, 1.0f);
	F.UI.Text("Notifications", R.X + 20.0f, R.Y + 34.0f, Ts(16.0f, 900, RrCream));
	if (!F.U.BellFresh.empty())
	{
		F.UI.Text(std::to_string(F.U.BellFresh.size()) + " new", R.X + W - 20.0f, R.Y + 34.0f, Ts(12.0f, 800, RrGold, Align::Right));
	}
	if (Rows == 0)
	{
		NetParagraph(F.C, RrBellEmpty, R.X + 20.0f, R.Y + 66.0f, W - 40.0f, 14.0f, 500, RrMuted, 20.0f, 2);
		return;
	}
	float Y = R.Y + 52.0f;
	for (int I = 0; I < Rows; ++I)
	{
		const auto& B = Bell[Bell.size() - 1 - static_cast<size_t>(I)];
		const bool Fresh = F.U.BellFresh.count(VideoBellKey(B)) > 0;
		const Rect Row{R.X + 10.0f, Y, W - 20.0f, 56.0f};
		F.C.FillRoundRect(Row, 8.0f, RrPanel);
		F.C.FillRoundRect({Row.X, Row.Y, 3.0f, Row.H}, 1.5f, RrBellAccent(B.second));
		if (Fresh)
		{
			F.C.FillCircle(Row.X + Row.W - 12.0f, Row.Y + 12.0f, 4.0f, RrGold);
		}
		NetParagraph(F.C, B.second, Row.X + 14.0f, Row.Y + 20.0f, Row.W - 40.0f, 13.0f, Fresh ? 700 : 600, RrCream, 17.0f, 2);
		F.UI.Text(RrAge(F.World - B.first), Row.X + Row.W - 12.0f, Row.Y + Row.H - 6.0f, Ts(10.5f, 700, RrDim, Align::Right));
		Y += 62.0f;
	}
}

// ------------------------------------------------------------------ STUDIO: RiverLineVideoStudio.cpp (update 3's split)

// ------------------------------------------------------------------ CHANNEL (§6.6.6: the public page)

/** The weekdays the last 10 uploads went up on most ("TUE · THU · SAT"), or "" with fewer than 3 uploads. */
std::string RrUsualDays(const video::Channel& Ch)
{
	std::vector<const video::Video*> List;
	for (const video::Video& V : Ch.Videos)
	{
		if (!RrIsGrant(V))
		{
			List.push_back(&V);
		}
	}
	std::stable_sort(List.begin(), List.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	if (List.size() < 3)
	{
		return std::string();
	}
	std::array<int, 7> Count{};
	for (size_t I = 0; I < List.size() && I < 10; ++I)
	{
		++Count[static_cast<size_t>(((net::DayOf(List[I]->At) % 7) + 7) % 7)];
	}
	std::vector<int> Days;
	for (int K = 0; K < 7; ++K)
	{
		if (Count[static_cast<size_t>(K)] > 0)
		{
			Days.push_back(K);
		}
	}
	std::stable_sort(Days.begin(), Days.end(), [&Count](int A, int B) { return Count[static_cast<size_t>(A)] > Count[static_cast<size_t>(B)]; });
	Days.resize(std::min<size_t>(Days.size(), 3));
	std::sort(Days.begin(), Days.end());
	std::string Out;
	for (const int D : Days)
	{
		// Day 0 is a Monday (net::WeekdayName's calendar): the short name in capitals.
		Out += (Out.empty() ? std::string() : std::string(" \xC2\xB7 ")) + rlnet_detail::NetUpper(net::WeekdayName(D));
	}
	return Out;
}

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
	// The player's own portrait (the person behind the channel, never a glyph), in a marigold ring.
	const Rect Photo{Ban.X + 28.0f, Ban.Y + 22.0f, 92.0f, 92.0f};
	F.C.FillRoundRect(Photo, 18.0f, Hex(0x2a2015));
	F.C.PushClip(Photo);
	{
		PortraitLight Light;
		Light.Rim = RrGold;
		const hero::Character Who = F.S.Person;
		F.C.Cached(RrMix(F.U.LookKey, 0x62616e6e6572ull), Photo.X + Photo.W / 2.0f, Photo.Y + 40.0f, [&Who, &Light](Canvas& Cv) { DrawPortrait(Cv, Who, 0.0f, 0.0f, 0.30f, 0.0, Light); });
	}
	F.C.PopClip();
	F.C.StrokeRoundRect(Photo, 18.0f, RrGold, 2.0f);
	const float Nx = Ban.X + 142.0f;
	const float NameW = F.UI.Text(F.S.HeroName, Nx, Ban.Y + 52.0f, Ts(30.0f, 900, RrCream, Align::Left, Baseline::Alphabetic, false, 560.0f));
	if (V.Partner)
	{
		// The Partner mark: a marigold seal with a check, and the word.
		const float Mx = Nx + std::min(NameW, 560.0f) + 30.0f;
		F.C.FillCircle(Mx, Ban.Y + 41.0f, 12.0f, RrGold);
		NetCheck(F.C, Mx, Ban.Y + 41.0f, 13.0f, Hex(videoart::Ink));
		NetPill(F.C, "PARTNER", Mx + 20.0f, Ban.Y + 31.0f, RrGold, true, 11.0f);
	}
	// Public numbers only (§1.12): subscribers rounded, the videos, the views.
	const size_t Count = Ch.Videos.size() + Ch.Stubs.size();
	std::string Line = RrSubsPublic(V.Subs);
	if (Count > 0)
	{
		Line += " \xC2\xB7 " + std::to_string(Count) + (Ch.Videos.size() >= 60 ? "+" : "") + (Count == 1 ? " video" : " videos");
	}
	Line += " \xC2\xB7 " + RrViews(video::TotalViews(Ch, F.World));
	F.UI.Text(Line, Nx, Ban.Y + 82.0f, Ts(15.0f, 600, Mix(RrCream, RrMuted, 0.3f), Align::Left, Baseline::Alphabetic, false, 760.0f));
	const std::string Days = RrUsualDays(Ch);
	F.UI.Text(Days.empty() ? std::string("Poker \xC2\xB7 highlights from your own tables") : "NEW VIDEOS MOST " + Days, Nx, Ban.Y + 106.0f, Ts(12.5f, 800, Days.empty() ? RrMuted : RrGold));
	if (Ch.X.On && Ch.X.StreakW > 0)
	{
		// R2: the upload streak with its Rain Checks (the core's words).
		F.UI.Text(video::valgo::StreakLine(Ch.X.StreakW, Ch.X.RainChecks), Nx, Ban.Y + 126.0f, Ts(12.5f, 700, RrCream));
	}
	// Right: where the Partner Program stands (no balance in public: Analytics › Revenue keeps the money).
	const float Mx = Ban.X + Ban.W - 330.0f;
	NetSpaced(F.C, "PARTNER PROGRAM", Mx, Ban.Y + 36.0f, 10.0f, 900, RrMuted, 1.4f);
	if (V.Partner)
	{
		F.UI.Text("since " + net::DateLabel(net::DayOf(Ch.PartnerAt)), Mx, Ban.Y + 74.0f, Ts(24.0f, 900, RrGold));
		F.UI.Text("Earnings and payouts are in Analytics.", Mx, Ban.Y + 98.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 310.0f));
	}
	else
	{
		const bool Review = Ch.ReviewAt >= 0.0;
		F.UI.Text(Review ? "Under review" : "Not yet", Mx, Ban.Y + 74.0f, Ts(26.0f, 900, Review ? RrGold : RrCream));
		F.UI.Text(Review ? RrAdsAfterReview : RrAdsAtGate, Mx, Ban.Y + 98.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 310.0f));
	}
}

/** Before Partner (§6.6.6): the gate's two bars (1,000 subscribers and 4,000 long-form watch hours in 365 days). */
void RrGateCard(RrFrame& F, const video::ChannelView& V, const Rect& R)
{
	using namespace rlnet_detail;
	const video::Channel& Ch = F.S.Life.Video;
	F.C.FillRoundRect(R, 16.0f, RrPanel);
	F.UI.Text("Partner Program", R.X + 20.0f, R.Y + 32.0f, Ts(18.0f, 900, RrCream));
	F.UI.Text("last 365 days", R.X + R.W - 20.0f, R.Y + 32.0f, Ts(12.5f, 600, RrMuted, Align::Right));
	struct Bar
	{
		const char* Label;
		double Have, Need;
	};
	const Bar Bars[2] = {{"Subscribers", std::floor(V.Subs), 1000.0}, {"Watch hours", std::floor(V.WatchHours365), 4000.0}};
	for (int K = 0; K < 2; ++K)
	{
		const float Y = R.Y + 66.0f + Nf(K) * 48.0f;
		const bool Full = Bars[K].Have >= Bars[K].Need;
		F.UI.Text(Bars[K].Label, R.X + 20.0f, Y, Ts(15.0f, 700, RrCream));
		F.UI.Text(Grouped(static_cast<int64_t>(Bars[K].Have)) + " / " + Grouped(static_cast<int64_t>(Bars[K].Need)), R.X + R.W - 20.0f, Y, Ts(15.0f, 800, Full ? RrGold : RrCream, Align::Right));
		const Rect Track{R.X + 20.0f, Y + 10.0f, R.W - 40.0f, 10.0f};
		F.C.FillRoundRect(Track, 5.0f, RrPanel2);
		const float Frac = Nf(std::clamp(Bars[K].Have / Bars[K].Need, 0.0, 1.0));
		if (Frac > 0.0f)
		{
			F.C.FillRoundRect({Track.X, Track.Y, std::max(10.0f, Track.W * Frac), Track.H}, 5.0f, RrGold);
		}
	}
	if (Ch.ReviewAt >= 0.0)
	{
		const float Pw = NetPill(F.C, "UNDER REVIEW", R.X + 20.0f, R.Y + 156.0f, RrGold, true, 10.5f);
		F.UI.Text(RrUnderReview, R.X + 30.0f + Pw, R.Y + 170.0f, Ts(13.0f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 50.0f - Pw));
	}
	else
	{
		F.UI.Text(RrGateNext, R.X + 20.0f, R.Y + 166.0f, Ts(13.0f, 700, RrCream));
		F.UI.Text(RrHoursNote, R.X + 20.0f, R.Y + 186.0f, Ts(12.0f, 500, RrMuted, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
	}
}

/** R1: the Outlier shelf, the videos that did five times the channel's typical or more (Breakout, Outlier). */
void RrShelf(RrFrame& F, const Rect& R, const std::vector<const video::Video*>& Outliers)
{
	using namespace rlnet_detail;
	RrCard(F, R, "THE OUTLIER SHELF", "five times your typical video or more");
	if (Outliers.empty())
	{
		NetParagraph(F.C, "No outliers yet. A video that reaches five times your typical lands on this shelf.", R.X + 18.0f, R.Y + 70.0f, R.W - 36.0f, 14.0f, 600, RrMuted, 20.0f, 3);
		return;
	}
	const float Tw = 211.0f;
	for (size_t I = 0; I < Outliers.size(); ++I)
	{
		const float X = R.X + 18.0f + Nf(I) * (Tw + 16.0f);
		if (X + Tw > R.X + R.W - 10.0f)
		{
			break;
		}
		const video::Video& V = *Outliers[I];
		const Rect Th{X, R.Y + 40.0f, Tw, std::round(Tw * 9.0f / 16.0f)};
		const Ui::ClickState St = F.UI.Clickable("rrshelf:" + V.Id, {Th.X, Th.Y, Th.W, Th.H + 24.0f}, F.Live);
		F.C.FillRoundRect(Th, 6.0f, RrGround);
		RrThumb(F, Th, V);
		F.C.StrokeRoundRect(Th, 6.0f, St.Hover ? RrGold : RrLine, St.Hover ? 1.5f : 1.0f);
		const video::VideoStats& Sv = RrStats(F, V);
		// Bottom left: the thumbnail's caption sits on top and its length badge bottom right.
		RrTag(F.C, NetUpper(video::valgo::TagName(Sv.Tag)), Th.X + 6.0f, Th.Y + Th.H - 23.0f, Sv.Tag >= 3 ? RrCream : RrGold, true, 9.0f);
		F.UI.Text(V.Title, Th.X, Th.Y + Th.H + 18.0f, Ts(12.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Tw));
		F.UI.Text(RrViews(video::Views(V, F.World)), Th.X, Th.Y + Th.H + 34.0f, Ts(11.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Tw));
		if (St.Clicked)
		{
			RrOpenVideo(F, V.Id);
		}
	}
}

void RrVideos(RrFrame& F, const video::ChannelView& V, float Top)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	// The tabs and the sort (§6.6.6): VIDEOS · CUTS · SERIES · ABOUT; Latest, Popular, Oldest.
	const char* Tabs[4] = {"VIDEOS", "CUTS", "SERIES", "ABOUT"};
	float Tx = 20.0f;
	for (int K = 0; K < 4; ++K)
	{
		const float W = F.UI.Measure(Tabs[K], 13.5f, 800) + 32.0f;
		if (RrChip(F, std::string("rrchtab") + Tabs[K], {Tx, Top, W, 32.0f}, Tabs[K], U.ChanTab == K))
		{
			U.ChanTab = K;
			U.GridGoal = U.GridScroll = 0.0f;
		}
		Tx += W + 6.0f;
	}
	if (U.ChanTab != 3)
	{
		const char* Sorts[3] = {"Latest", "Popular", "Oldest"};
		float Sx = NetW - 20.0f;
		for (int K = 2; K >= 0; --K)
		{
			const float W = F.UI.Measure(Sorts[K], 12.5f, 800) + 26.0f;
			Sx -= W;
			if (RrChip(F, std::string("rrchsort") + Sorts[K], {Sx, Top + 2.0f, W, 28.0f}, Sorts[K], U.ChanSort == K, 12.5f))
			{
				U.ChanSort = K;
				U.GridGoal = U.GridScroll = 0.0f;
			}
			Sx -= 4.0f;
		}
	}
	const Rect Area{20.0f, Top + 46.0f, NetW - 40.0f, VideoH - 14.0f - (Top + 46.0f)};
	if (U.ChanTab == 3)
	{
		// ABOUT: what the channel is, in public words.
		const Rect Ab{Area.X, Area.Y, 760.0f, 220.0f};
		RrCard(F, Ab, "ABOUT");
		NetParagraph(F.C, "Poker from " + F.S.HeroName + "'s own tables: the hands, the runs and the nights, cut from what really happened.", Ab.X + 18.0f, Ab.Y + 60.0f, Ab.W - 36.0f, 15.0f, 600, RrCream, 22.0f, 3);
		F.UI.Text(RrSubsPublic(V.Subs) + " \xC2\xB7 " + RrViews(video::TotalViews(Ch, F.World)), Ab.X + 18.0f, Ab.Y + 140.0f, Ts(13.5f, 700, RrMuted));
		if (Ch.X.On && Ch.X.StreakW > 0)
		{
			F.UI.Text(video::valgo::StreakLine(Ch.X.StreakW, Ch.X.RainChecks), Ab.X + 18.0f, Ab.Y + 166.0f, Ts(13.5f, 700, RrCream));
		}
		return;
	}
	std::vector<const video::Video*> List;
	for (const video::Video& X : Ch.Videos)
	{
		const bool Cut = X.F == video::Format::Short;
		if ((U.ChanTab == 0 && !Cut) || (U.ChanTab == 1 && Cut) || (U.ChanTab == 2 && X.X.On && X.X.Series >= 0))
		{
			List.push_back(&X);
		}
	}
	const double W0 = F.World;
	std::stable_sort(List.begin(), List.end(), [&U, W0](const video::Video* A, const video::Video* B) {
		if (U.ChanSort == 1)
		{
			return video::Views(*A, W0) > video::Views(*B, W0);
		}
		return U.ChanSort == 2 ? A->At < B->At : A->At > B->At;
	});
	if (List.empty())
	{
		const float Cx = Area.X + Area.W / 2.0f;
		const bool Nothing = Ch.Videos.empty();
		F.UI.Text(Nothing ? RrChannelEmpty : U.ChanTab == 1 ? "No Cuts yet." : U.ChanTab == 2 ? "No series yet." : "No long videos yet.", Cx, Area.Y + 100.0f, Ts(24.0f, 900, RrCream, Align::Center));
		F.UI.Text(RrChannelEmptyMore, Cx, Area.Y + 132.0f, Ts(15.0f, 500, RrMuted, Align::Center));
		if (Nothing && !Ch.Open.empty() && RrButton(F, "rrtostudio", {Cx - 120.0f, Area.Y + 160.0f, 240.0f, 48.0f}, "Open Studio", true))
		{
			U.Page = RrPageStudio;
			U.PageAt = F.Now;
		}
		return;
	}
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
		const float Tx0 = Area.X + Nf(static_cast<int>(I) % RrCols) * (TileW + RrTileGap);
		const float Ty = Area.Y + Nf(static_cast<int>(I) / RrCols) * RrTilePitch - U.GridScroll;
		if (Ty + RrTilePitch < Area.Y || Ty > Area.Y + Area.H)
		{
			continue;
		}
		const Rect R{Tx0, Ty, TileW, RrTilePitch - 10.0f};
		RrTile(F, R, X, false, 0.0);
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
}

/** The page's note (a delete, a refusal): a toast near the bottom for six seconds. */
void RrToast(RrFrame& F)
{
	VideoUi& U = F.U;
	if (U.Note.empty() || F.Now - U.NoteAt >= 6.0)
	{
		return;
	}
	const float Tw = std::min(rlnet_detail::NetW - 80.0f, F.UI.Measure(U.Note, 14.0f, 700) + 32.0f);
	const Rect T{rlnet_detail::NetW / 2.0f - Tw / 2.0f, VideoH - 64.0f, Tw, 40.0f};
	F.C.FillRoundRect(T, 12.0f, RrGround);
	F.C.StrokeRoundRect(T, 12.0f, U.NoteBad ? RrWarn : RrGold, 1.5f);
	F.UI.Text(U.Note, T.X + T.W / 2.0f, T.Y + 21.0f, Ts(14.0f, 700, RrCream, Align::Center, Baseline::Middle, false, Tw - 24.0f));
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
	// The Outlier shelf (R1): Breakout and Outlier tags, the newest first.
	std::vector<const video::Video*> Outliers;
	for (const video::Video& X : Ch.Videos)
	{
		if (!RrIsGrant(X) && RrStats(F, X).Tag >= 2)
		{
			Outliers.push_back(&X);
		}
	}
	std::stable_sort(Outliers.begin(), Outliers.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	float Top = 226.0f;
	if (!V.Partner)
	{
		RrGateCard(F, V, {20.0f, 226.0f, 600.0f, 204.0f});
		RrShelf(F, {636.0f, 226.0f, rlnet_detail::NetW - 20.0f - 636.0f, 204.0f}, Outliers);
		Top = 446.0f;
	}
	else if (!Outliers.empty())
	{
		RrShelf(F, {20.0f, 226.0f, rlnet_detail::NetW - 40.0f, 204.0f}, Outliers);
		Top = 446.0f;
	}
	RrVideos(F, V, Top);
	RrToast(F);
}
} // namespace rlvideo_detail

// ------------------------------------------------------------------ the entry points (RiverLineShared.h's callers)

namespace rlvideo_detail
{
void VideoOpen(RiverLine& RL, int Page, double Now)
{
	VideoUi& U = VideoUiOf(&RL);
	U.Page = std::clamp(Page, 0, RrPageCount - 1);
	U.PageAt = Now;
	U.BellOpen = false;
	RL.OpenApp(RiverLine::App::Video, Now);
}

int VideoPageNamed(const std::string& Name)
{
	return Name == "studio" ? RrPageStudio : Name == "channel" ? RrPageChannel : Name == "analytics" ? RrPageAnalytics : Name == "home" ? RrPageHome : -1;
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
	U.ListTab = 0;
	U.OpenTab = 0;         // the visit opens on MOMENTS, whatever ideas wait (L9's rule is for a plain visit)
	U.LaunchHidden = true; // the editor, not a running launch card: the player came to cut
	U.ListGoal = U.ListScroll = 0.0f;
	VideoOpen(RL, RrPageStudio, Now);
}

/**
 * Kast's end card (K15, G3's "Make a Cut and link it"; ui-kast's button calls it by a patch): Studio on the clip's moments as
 * a Cut, its Next-up card linking the newest kept long video, so the Cut sends its viewers there (§4.5.6's From Cuts).
 */
void VideoOpenCut(RiverLine& RL, double From, double To, double Now)
{
	VideoOpenTonight(RL, From, To, Now);
	VideoUi& U = VideoUiOf(&RL);
	U.F = video::Format::Short;
	U.LinkNewest = true;
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
	// §6.6.5: a new trending idea lights it too (the ideas as Studio last read them: the taskbar never builds them itself).
	bool Trend = false;
	for (const video::IdeaCard& I : U.Ideas)
	{
		Trend = Trend || (I.Trending && I.Ready && U.SeenIdeas.count(I.Id) == 0);
	}
	return Fresh >= 3 || VideoUnseenBell(U, S) > 0 || Trend;
}

void ForgetVideo(const RiverLine* RL)
{
	VideoUiStates().erase(RL);
}

std::vector<std::string> VideoUiInventory()
{
	std::vector<std::string> Out = {RrStudioEmpty, RrStudioEmptyMore, RrChannelEmpty, RrChannelEmptyMore, RrPickFirst, RrAutoNote, RrTonightGone, RrVlogFull, RrRecapFull,
		RrDeleteAgain, RrHoursNote, RrGateNext, RrUnderReview, RrPartnerHead, RrPartnerSince, RrAdsAtGate, RrAdsAfterReview, RrPaysOut, RrBellEmpty, "It goes up at 3:40 AM.", RrIdeasEmpty,
		RrSeriesEmpty, RrCutNoThumb, RrDuelNeedsNew, RrDuelHelp, RrEarlyHint, RrAdsBeforePartner, RrNotPaid, RrNoTrends, RrHelpCard, RrSeasonLine, RrHomeEmpty, RrSearchNote,
		RrChangeOnce, RrAgeNote, "Earnings and payouts are in Analytics.", "No outliers yet. A video that reaches five times your typical lands on this shelf.",
		"Poker from grinder_3c's own tables: the hands, the runs and the nights, cut from what really happened.", "No Cuts yet.", "No series yet.", "No long videos yet."};
	for (const std::vector<std::string>& More : {RrStudioInventory(true), RrStatsInventory(true), RrHomeInventory(true)})
	{
		Out.insert(Out.end(), More.begin(), More.end());
	}
	return Out;
}

std::vector<std::string> VideoUiFragments()
{
	std::vector<std::string> Out = {"HOME", "STUDIO", "CHANNEL", "ANALYTICS", "Notifications", "2 new", "4.2K subscribers", "Partner \xC2\xB7 grinder_3c", "PARTNER PROGRAM",
		"Under review", "Not yet", "since Oct 3", "NEW VIDEOS MOST TUE \xC2\xB7 THU \xC2\xB7 SAT", "Poker \xC2\xB7 highlights from your own tables", "THE OUTLIER SHELF",
		"five times your typical video or more", "VIDEOS", "CUTS", "SERIES", "ABOUT", "Latest", "Popular", "Oldest", "Partner Program", "last 365 days", "Subscribers",
		"Watch hours", "UNDER REVIEW", "Cut", "Breakdown", "Vlog", "Recap", "Story", "Loud", "Teaching", "Question", "Plain", "45 s \xC2\xB7 one moment", "one hand",
		"one night", "one run", RrDislikesPrivate, RrLegacyRow, "BREAKOUT", "OUTLIER", "ABOVE TYPICAL", "Not pushed", "Held", "Pushed", "Pushed more", "Pushed hard"};
	for (const std::vector<std::string>& More : {RrStudioInventory(false), RrStatsInventory(false), RrHomeInventory(false)})
	{
		Out.insert(Out.end(), More.begin(), More.end());
	}
	return Out;
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
		U.StudioShown = false; // a new visit (L9: Studio picks its list's tab again)
	}
	U.LastDrawn = Now;
	// A frame's budgets: two uncached thumbnails painted, a few cached rows refreshed (§10, V12); the look they're drawn with.
	U.Paint = thumbart::Budget();
	U.StatsBudget = 6;
	U.LookKey = RrLookHash(S.Person);
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
	switch (U.Page)
	{
	case RrPageChannel: RrChannel(F); break;
	case RrPageAnalytics: RrAnalytics(F); break;
	case RrPageHome: RrHome(F); break;
	case RrPageSheet: RrSheet(F); break;
	default: RrStudio(F); break;
	}
	U.StudioShown = U.Page == RrPageStudio && U.StudioShown; // another page ends the visit (RrStudio sets it while it draws)
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
