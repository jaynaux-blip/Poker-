// Update 3 (SPEC §5, §7.1): Rerun 2.0's Analytics, ui-rerun's file: Overview, Content, Audience, Revenue, Realtime,
// Research and Channel health, every number from VideoStats.h's contract. Shares the look and side state through
// RiverLineVideoShared.h.
//
// C3 (ui-rerun; update 3 SPEC §5.2-§5.10, DESIGN §4.7; SPEC_CREATORS §5.3 R1-R4): the sub-tabs OVERVIEW · Content · Audience
// · Revenue · Realtime · Research and the range picker (7 d · 28 d · 90 d · 365 d · Lifetime); every chart is ChartArt's.
//   OVERVIEW: four tiles (views, watch hours, subscribers "+412 (31 lost)", revenue) against the window before; the selected
//     tile's chart over the typical range with the uploads and the verdicts marked; Realtime; the top content (likes and
//     dislikes with the ratio, private); the latest upload against the typical one; how the money adds up (CPM to RPM) with
//     the season; Channel health with the moving Momentum meter and the upload streak.
//   CONTENT: the newest 60 in full rows (sortable; a Cut's "shown in feed" and "stayed to watch"; Update 2 videos "est."),
//     the Above typical / Breakout / Outlier tags, Late Bloomer and the Binge wave; the folded row and the all-time top 20.
//     One video: the list on the left and four tabs, Overview (the verdict on the five-step scale, the KPI row, views a day
//     against the typical, "What Rerun did with it", Insights, Channel health and the next upload's carry), Reach (the
//     funnel, the traffic donut with each source's CTR, views and impressions a day, the Duel), Engagement (the retention
//     curve against the typical with its moments, likes against dislikes, comments) and Revenue (RPM, CPM, the ad types by
//     position, the coin, the ad load with its weekly change, RPM a day).
//   AUDIENCE: returning and new, when viewers are on (the heat map, the best window against the hour you post), ages
//     (18+), devices, the bell's share, geographies, the channels your audience watches.
//   REVENUE: before Partner the gate's bars, the Ad market card and "would have earned"; after it the tiles, revenue a day
//     with the season behind, the ad types, payouts, the ad settings with MANAGE, the top earners.
//   REALTIME: 48 hours by the hour, the last 60 minutes, subscribers now, the breakouts' notes, what's on Home right now.
//   RESEARCH: this week's trends with their heat, the Limited coin and the flag; the Ad market before Partner; content gaps
//     linked to IDEAS; your niche; the help card.
// Dislikes appear only here and in Studio, always beside "dislikes are only visible to you".
#include "RiverLineVideoShared.h"
#include "../StrictFloat.h"
#include "../Game/VideoAlgoPriv.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/ChartArt.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace rlvideo_detail
{
// ------------------------------------------------------------------ Analytics' own copy

constexpr const char* RrVerdictHow = "the label reads how the video really did";
constexpr const char* RrBeforeVerdict = "Rerun is still testing it. The verdict comes at midnight two days after it went up.";
constexpr const char* RrNoSubs = "Nothing in this window yet.";
constexpr const char* RrManageHelp = "A video's last check-in is its last judging: after it, more ads cost only its new viewers' patience.";
constexpr const char* RrGapsNote = "Ideas that match this week's trends, ready to cut:";
constexpr const char* RrMarketNote = "What advertisers pay this week for a thousand ad views, by what the video is about.";
// C3 gate (update 3 §4.6.7): the one-video page's repackage card.
constexpr const char* RrRepackHelp = "A new title or thumbnail changes who clicks from now on. Rerun skips its next check-in while it learns the new look.";
constexpr const char* RrRepackDone = "Changed. Rerun skips its next check-in while it learns the new look.";

std::vector<std::string> RrStatsInventory(bool Sentences)
{
	if (Sentences)
	{
		return {RrBeforeVerdict, RrNoSubs, RrManageHelp, RrMarketNote, RrRepackHelp, RrRepackDone, "Best: Sun 8-11 PM. You post at 4 AM.",
			"Your niche: 7 of your last 10 are beats and coolers.", "Earnings and payouts are in Analytics.", "This video was published before ad settings.",
			"Comments arrive with the views.", "Ads on this video: Light, from the next midnight.", "This video's first 48 hours: CTR 3.1%, your typical 5.2%.",
			// C3 fix round (H1): the top-earner rate's line under HOW YOU EARNED (its other words are the core's).
			"The next views pay 74% of the full rate."};
	}
	return {RrVerdictHow, RrGapsNote, "More than the 28 days before", "Less than the 28 days before", "About the same as the 28 days before", "Since your first video",
		"October ads pay more", "January is quiet for ads", "About an average month for ads", "shaded: your typical range", "dashed: your typical",
		"dashed: your typical video at the same age", "14% of subscribers get every alert", "Changes at the next midnight", "Next change from Oct 13", "SEE ALL CONTENT \xE2\x80\xBA",
		"\xE2\x80\xB9 All content", "matching videos get about 14% more viewers", "2 of yours", "judged", "check-in on day 14", "Kept videos, since each went up",
		"Next payout Nov 21, if $100+", "the views so far, at each day's ad prices", "your share after Rerun", "ads per playback", "of long views showed an ad",
		"Watch-page ads", "Cuts feed", "pre-roll 61%", "mid-roll 22%", "post-roll 7%", "skippable 84% \xC2\xB7 6-second 16%", "OVERVIEW", "Content", "Audience", "Revenue",
		"Realtime", "Research", "7 d", "28 d", "90 d", "365 d", "Lifetime", "VIEWS", "WATCH TIME (HOURS)", "SUBSCRIBERS", "ESTIMATED REVENUE", "(31 lost)", "443 gained \xC2\xB7 31 lost",
		"VIEWS \xC2\xB7 LAST 28 DAYS", "LIFETIME", "REALTIME", "updating live", "views \xC2\xB7 last 48 hours", "48 h ago", "now", "views \xC2\xB7 last 60 minutes \xC2\xB7 +6 subscribers",
		"SHOWN ON HOME", "TOP CONTENT \xC2\xB7 28 DAYS", "CTR 48H", "LIKES", "REVENUE", "LATEST", "2 days \xC2\xB7 2nd of your last 10 at 2 days \xC2\xB7 8:00", "AVD 3:41 (46%)",
		"TRAFFIC SOURCES", "Home 38%", "HOW THE MONEY ADDS UP", "BEFORE PARTNER", "CPM", "RPM", "long videos", "$2.67 counting every view", "Balance $118.40",
		"CHANNEL HEALTH", "Momentum", "heading up", "easing", "holding", "Packaging", "Holding", "Satisfaction", "Consistency", "Energy", "VIDEO", "WATCH H", "SUBS",
		"IMPRESSIONS", "AVD", "LIKES \xC2\xB7 DISLIKES", "COIN", "TREND", "shown in feed", "stayed to watch", "est.", "LATE BLOOMER", "BINGE WAVE", "OLDER VIDEOS",
		"ALL-TIME TOP 20", "3 still found by search", "YOUR VIDEOS", "newest first", "THE VERDICT", "THE VERDICT \xC2\xB7 THU MIDNIGHT", "WHAT RERUN DID WITH IT", "INSIGHTS",
		"VIEWS A DAY SINCE IT WENT UP", "day 1", "YOUR NEXT UPLOAD", "Studio", "OPEN STUDIO", "IDEAS", "Overview", "Reach", "Engagement", "RECOMMENDED ON HOME",
		"published Tue 9:40 PM, 9 days ago", "CTR 48 H", "lifetime 5.1%", "typical 2,300", "46% viewed", "98.2% liked", "RPM $2.85", "the packaging's",
		"colder viewers later", "CTR LIFETIME", "1,240 unique", "HOW VIEWERS GOT THERE", "Impressions", "Views", "Past 0:30", "Past half", "Liked", "Subscribed",
		"Home", "Next up", "Search", "Subscriptions", "Notifications", "Kast", "From Cuts", "External", "Playlists and channel", "VIEWS AND IMPRESSIONS A DAY",
		"dashed: your typical video's views", "won on watch time", "A won", "4.1% CTR \xC2\xB7 47% of watch time", "AVERAGE VIEW", "VIEWED", "COMMENTS \xC2\xB7 SHARES",
		"12 saves", "AUDIENCE RETENTION", "This video", "Dip: 0:00-0:30, 31% left", "Moments that held them: (1) 3:50 the all in", "LIKES AND DISLIKES",
		"typical 97.5% \xC2\xB7 dislikes are only visible to you", "443 subscribed \xC2\xB7 31 unsubscribed after watching", "COMMENTS", "since it went up",
		"per 1,000 views today", "per 1,000 ad views", "PLAYBACK CPM", "per 1,000 plays with an ad", "MONETIZED PLAYS", "1,412 ad views", "AD TYPES",
		"CPM $8.03 \xC2\xB7 1.29 ads per playback \xC2\xB7 skippable 84% \xC2\xB7 6-second 16%", "SUITABILITY AND ADS", "Limited ads", "No ads", "RPM A DAY",
		"RETURNING AND NEW VIEWERS", "Returning 31%", "New 69%", "Unique viewers 44,900 \xC2\xB7 views per viewer 1.19 \xC2\xB7 34% of watch time from subscribers",
		"WHEN YOUR VIEWERS ARE ON", "your time", "AGE", "18-24", "25-34", "35-44", "45-54", "55+", "DEVICES", "Phone", "Computer", "TV", "Tablet", "TOP GEOGRAPHIES",
		"CHANNELS YOUR AUDIENCE WATCHES", "followers", "THE AD MARKET THIS WEEK", "your last 10 uploads", "THE PARTNER PROGRAM", "WHAT YOUR VIDEOS WOULD HAVE EARNED",
		"THE SEASON", "REVENUE A DAY", "HOW YOU EARNED", "PAYOUTS", "Paid out $151.06 so far", "SEP PAID", "SEP ROLLED OVER","AD SETTINGS ON YOUR VIDEOS", "Standard 41 \xC2\xB7 Heavy 6 \xC2\xB7 Light 2 \xC2\xB7 Limited 1",
		"MANAGE", "DONE", "TOP EARNERS", "MANAGE ADS", "per 1,000 views, every view", "50% of long views", "VIEWS \xC2\xB7 LAST 48 HOURS", "by the hour", "LAST 60 MINUTES",
		"60 min ago", "subscribers now \xC2\xB7 +6 in the last hour", "TOP VIDEOS RIGHT NOW", "the last 48 hours", "SHOWN ON HOME RIGHT NOW", "TRENDING IN POKER THIS WEEK",
		"CONTENT GAPS", "your chance", "YOUR NICHE", "HOW RERUN DECIDES", "As a Partner, your videos would have earned about $4.10 so far (not paid)",
		"CHANGE THE PACKAGING", "2 changes left", "1 change left", "TITLE", "THUMBNAIL", "SHUFFLE",
		// C3 fix round: the top-earner rate (H1) and Research's search table (M2).
		"TOP-EARNER RATE", "top-earner share", "Held back so far this month: $1,234.50", "?", "WHAT POKER VIEWERS SEARCH", "this week, against its top search", "SEARCH",
		"INTEREST", "WEEK", "YOURS", "none yet", "3 of yours", "MAKE IT \xE2\x80\xBA", "TRENDING"};
}

const char* const RrMonthNames[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char* const RrMonthShort[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char* const RrSourceNames[10] = {"Home", "Next up", "Search", "Subscriptions", "Notifications", "Cuts feed", "Kast", "From Cuts", "External", "Playlists and channel"};
const char* const RrGroupNames[6] = {"beats and coolers", "reads and bluffs", "pots and knockouts", "runs and wins", "the rival", "stories"};

/** The window's days: 7, 28, 90, 365, or the channel's life (its oldest kept or folded video, at most two years). */
int RrWindowDays(RrFrame& F)
{
	const int Fixed4[4] = {7, 28, 90, 365};
	if (F.U.RangeIx < 4)
	{
		return Fixed4[std::clamp(F.U.RangeIx, 0, 3)];
	}
	const video::Channel& Ch = F.S.Life.Video;
	double First = F.World;
	for (const video::Video& V : Ch.Videos)
	{
		First = std::min(First, V.At);
	}
	for (const video::TopRow& T : Ch.X.Top)
	{
		First = std::min(First, T.At);
	}
	for (const video::Stub& S : Ch.Stubs)
	{
		First = std::min(First, S.At);
	}
	return std::clamp(net::DayOf(F.World) - net::DayOf(First) + 1, 7, 730);
}

double RrSum(const std::vector<double>& V)
{
	double S = 0.0;
	for (const double X : V)
	{
		S += X;
	}
	return S;
}

double RrMid(std::vector<double> V, double Q)
{
	if (V.empty())
	{
		return 0.0;
	}
	std::sort(V.begin(), V.end());
	return V[static_cast<size_t>(std::clamp(Q, 0.0, 1.0) * static_cast<double>(V.size() - 1) + 0.5)];
}

/** The kept uploads newest first (no ss.Video.Grant video). */
std::vector<const video::Video*> RrUploads(const Session& S)
{
	std::vector<const video::Video*> Out;
	for (const video::Video& V : S.Life.Video.Videos)
	{
		if (!RrIsGrant(V))
		{
			Out.push_back(&V);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const video::Video* A, const video::Video* B) { return A->At > B->At; });
	return Out;
}

/** Where V stands among the 9 uploads before it at the same age (1 the best), and how many it was ranked with. */
void RrRankAtAge(RrFrame& F, const video::Video& V, int& Rank, int& Of, double& Typical)
{
	VideoUi& U = F.U;
	const double Minute = std::floor(F.World);
	if (U.DailyId == V.Id && U.DailyMinute == Minute && U.OneOf > 0)
	{
		Rank = U.OneRank;
		Of = U.OneOf;
		Typical = U.OneTypical;
		return;
	}
	const double Age = std::max(0.0, F.World - V.At);
	const double Mine = video::Views(V, F.World);
	std::vector<double> Seen;
	for (const video::Video* O : RrUploads(F.S))
	{
		if (O != &V && O->At < V.At && Seen.size() < 9)
		{
			Seen.push_back(video::Views(*O, O->At + Age));
		}
	}
	Rank = 1;
	for (const double X : Seen)
	{
		Rank += X > Mine ? 1 : 0;
	}
	Of = static_cast<int>(Seen.size()) + 1;
	Typical = RrMid(Seen, 0.5);
	if (U.DailyId == V.Id && U.DailyMinute == Minute)
	{
		U.OneRank = Rank;
		U.OneOf = Of;
		U.OneTypical = Typical;
	}
}

/** "1st", "2nd" ... */
std::string RrNth(int N)
{
	return Ordinal(N);
}

/** A row of KPI tiles across R (chart::Kpi in Rerun's look). */
struct RrTileSpec
{
	std::string Label, Value, Sub, Note, Delta;
	int Sign = 0;
	Color Col{0, 0, 0, 0};
	std::vector<double> Spark;
	Color SparkCol{0, 0, 0, 0};
	bool Selected = false;
};

void RrTiles(RrFrame& F, const Rect& R, const std::vector<RrTileSpec>& Tiles, float ValueSize, const std::string& IdPrefix = std::string(), int* Clicked = nullptr,
	const std::vector<float>& Weights = std::vector<float>())
{
	using rlnet_detail::Nf;
	const float Gap = 12.0f;
	// Equal widths, or by weight (a tile whose words are longer gets more of the row).
	float Total = 0.0f;
	for (size_t K = 0; K < Tiles.size(); ++K)
	{
		Total += K < Weights.size() ? Weights[K] : 1.0f;
	}
	const float Unit = (R.W - Gap * Nf(Tiles.size() - 1)) / std::max(1.0f, Total);
	float X = R.X;
	for (size_t K = 0; K < Tiles.size(); ++K)
	{
		const RrTileSpec& T = Tiles[K];
		const float W = Unit * (K < Weights.size() ? Weights[K] : 1.0f);
		const Rect Tr{X, R.Y, W, R.H};
		X += W + Gap;
		chart::KpiCard Kc;
		Kc.Label = T.Label;
		Kc.Value = T.Value;
		Kc.Sub = T.Sub;
		Kc.Note = T.Note;
		Kc.Delta = T.Delta;
		Kc.DeltaSign = T.Sign;
		Kc.ValueCol = T.Col;
		Kc.Spark = T.Spark;
		Kc.SparkCol = T.SparkCol;
		Kc.Selected = T.Selected;
		chart::KpiOpts Ko;
		Ko.ValueSize = ValueSize;
		Ko.NoteSize = ValueSize >= 30.0f ? 12.5f : 11.0f;
		chart::Kpi(F.C, Tr, Kc, RrTheme(), Ko);
		if (!IdPrefix.empty() && Clicked && F.UI.Clickable(IdPrefix + std::to_string(K), Tr, F.Live).Clicked)
		{
			*Clicked = static_cast<int>(K);
		}
	}
}

/** The change against the window before: "18%" with its sign, or nothing when there's nothing to compare. */
void RrDelta(double Now, double Was, RrTileSpec& T)
{
	if (Was <= 0.0 || Now < 0.0)
	{
		return;
	}
	const double D = (Now - Was) / Was;
	T.Delta = Fixed(std::fabs(100.0 * D), 0) + "%";
	T.Sign = D > 0.005 ? 1 : D < -0.005 ? -1 : 0;
}

std::string RrVersus(double Now, double Was, int Days, bool Lifetime)
{
	if (Lifetime)
	{
		return "Since your first video";
	}
	const std::string Window = std::to_string(Days) + " days before";
	if (Was <= 0.0)
	{
		return Now > 0.0 ? "More than the " + Window : std::string(RrNoSubs);
	}
	return Now > 1.1 * Was ? "More than the " + Window : Now < Was / 1.1 ? "Less than the " + Window : std::string("About the same as before");
}

/** The season in words for the tile (§4.5.3): this month's ads against the year's average. */
std::string RrSeasonWords(double World)
{
	int Y = 0, Mo = 0, D = 0;
	world::CivilDate(net::DayOf(World), Y, Mo, D);
	const double S = video::algo::Season(net::DayOf(World));
	const std::string Month = RrMonthNames[std::clamp(Mo, 1, 12) - 1];
	return S > 1.05 ? Month + " ads pay more" : S < 0.95 ? Month + " is quiet for ads" : std::string("About an average month for ads");
}

/** This pay month and the top-earner rate (H1, the user's soft ceiling; core-videoalgo's numbers): whether the month has
 *  passed it, what the next views pay against the full rate, what it held back. */
video::valgo::TopRateView RrTopRate(RrFrame& F)
{
	return video::valgo::TopRateOf(F.S.Life.Video, F.World);
}

// ------------------------------------------------------------------ Channel health (§5.9; SPEC_CREATORS §3.2 R1, R2)

void RrHealthCard(RrFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	RrDaily(F);
	const video::Health& H = F.U.Health;
	const video::ChannelX& X = F.S.Life.Video.X;
	RrCard(F, R, "CHANNEL HEALTH");
	if (R.H < 200.0f)
	{
		// Compact (Overview's money card): the six meters in two columns, the streak's cells under them.
		const char* All[6] = {"Momentum", "Packaging", "Holding", "Satisfaction", "Consistency", "Energy"};
		const int Lit[6] = {H.Momentum, H.Packaging, H.Holding, H.Satisfaction, H.Consistency, H.Energy};
		std::string Over;
		for (int K = 0; K < 6; ++K)
		{
			const float Cx = R.X + 18.0f + Nf(K % 2) * ((R.W - 30.0f) / 2.0f);
			const float Cy = R.Y + 50.0f + Nf(K / 2) * 22.0f;
			F.UI.Text(All[K], Cx, Cy, Ts(11.5f, K == 0 ? 800 : 600, K == 0 ? RrGold : RrCream));
			RrBars5(F.C, Cx + 92.0f, Cy - 10.0f, Lit[K], RrGold);
			if (F.Live && F.UI.Hover({Cx, Cy - 14.0f, (R.W - 30.0f) / 2.0f, 20.0f}))
			{
				Over = video::HealthWords(K, Lit[K]);
			}
		}
		if (X.On && R.H >= 130.0f)
		{
			F.UI.Text(video::valgo::StreakLine(X.StreakW, X.RainChecks), R.X + 18.0f, R.Y + R.H - 12.0f, Ts(11.5f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
		}
		if (!Over.empty())
		{
			chart::Tooltip(F.C, {F.UI.Ptr.X, F.UI.Ptr.Y}, Over, {}, RrTheme(), R);
		}
		return;
	}
	// Momentum, moving without jumping (the display average): lit to its bars, the ghost the raw MomC points to (R1).
	const Rect Mo{R.X + 12.0f, R.Y + 38.0f, R.W - 24.0f, 56.0f};
	F.C.FillRoundRect(Mo, 8.0f, NetA(RrGold, 0.10f));
	F.UI.Text("Momentum", Mo.X + 12.0f, Mo.Y + 24.0f, Ts(14.0f, 800, RrCream));
	const double Raw = X.On ? X.Momentum : 1.0;
	const int Ghost = std::clamp(static_cast<int>(std::lround(Raw <= 1.0 ? 3.0 * (Raw - 0.75) / 0.25 : 3.0 + 2.0 * (Raw - 1.0) / 0.5)), 0, 5);
	RrBars5(F.C, Mo.X + 106.0f, Mo.Y + 12.0f, H.Momentum, RrGold, 14.0f, 15.0f, 4.0f, Ghost > H.Momentum ? Ghost : -1);
	const std::string Words = video::HealthWords(0, H.Momentum);
	const size_t Colon = Words.find(": ");
	std::string Short = Colon == std::string::npos ? Words : Words.substr(Colon + 2);
	if (!Short.empty() && Short.back() == '.')
	{
		Short.pop_back();
	}
	F.UI.Text(Short, Mo.X + Mo.W - 12.0f, Mo.Y + 26.0f, Ts(13.0f, 900, RrGold, Align::Right));
	F.UI.Text(Ghost > H.Momentum ? std::string("heading up") : Ghost < H.Momentum ? std::string("easing") : std::string("holding"), Mo.X + 12.0f, Mo.Y + 46.0f, Ts(11.0f, 600, RrMuted));
	// The other five, in two columns; hover gives the words, never a number.
	const char* Names[5] = {"Packaging", "Holding", "Satisfaction", "Consistency", "Energy"};
	const int Bars[5] = {H.Packaging, H.Holding, H.Satisfaction, H.Consistency, H.Energy};
	std::string Hovered;
	for (int K = 0; K < 5; ++K)
	{
		const float Cx = R.X + 24.0f + Nf(K % 2) * ((R.W - 36.0f) / 2.0f);
		const float Cy = Mo.Y + Mo.H + 28.0f + Nf(K / 2) * 24.0f;
		F.UI.Text(Names[K], Cx, Cy, Ts(12.0f, 600, RrCream));
		RrBars5(F.C, Cx + 92.0f, Cy - 10.0f, Bars[K], RrGold);
		if (F.Live && F.UI.Hover({Cx, Cy - 14.0f, (R.W - 36.0f) / 2.0f, 20.0f}))
		{
			Hovered = video::HealthWords(K + 1, Bars[K]);
		}
	}
	// R2: the upload streak with its Rain Checks, eight weeks as eight cells.
	const float Sy = R.Y + R.H - 18.0f;
	if (X.On)
	{
		F.UI.Text(video::valgo::StreakLine(X.StreakW, X.RainChecks), R.X + 24.0f, Sy, Ts(12.5f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 160.0f));
		for (int K = 0; K < 8; ++K)
		{
			F.C.FillRoundRect({R.X + R.W - 24.0f - 8.0f * 13.0f + Nf(K) * 13.0f, Sy - 11.0f, 10.0f, 12.0f}, 2.0f, K < X.StreakW ? RrGold : RrPanel2);
		}
	}
	if (!Hovered.empty())
	{
		chart::Tooltip(F.C, {F.UI.Ptr.X, F.UI.Ptr.Y}, Hovered, {}, RrTheme(), R);
	}
}

// ------------------------------------------------------------------ the sub-tabs and the range

void RrSubTabs(RrFrame& F, bool Ranges)
{
	VideoUi& U = F.U;
	const char* Tabs[6] = {"OVERVIEW", "Content", "Audience", "Revenue", "Realtime", "Research"};
	float X = 20.0f;
	for (int K = 0; K < 6; ++K)
	{
		const float W = F.UI.Measure(Tabs[K], 13.5f, 800) + 30.0f;
		if (RrChip(F, std::string("rrsub") + Tabs[K], {X, 74.0f, W, 32.0f}, Tabs[K], U.Sub == K))
		{
			U.Sub = K;
			U.One.clear();
			U.Manage = false;
		}
		X += W + 6.0f;
	}
	if (!Ranges)
	{
		return;
	}
	const char* Rs[5] = {"7 d", "28 d", "90 d", "365 d", "Lifetime"};
	float Rx = rlnet_detail::NetW - 20.0f;
	for (int K = 4; K >= 0; --K)
	{
		const float W = F.UI.Measure(Rs[K], 12.5f, 800) + 26.0f;
		Rx -= W;
		const Rect R{Rx, 76.0f, W, 28.0f};
		const bool On = U.RangeIx == K;
		const Ui::ClickState St = F.UI.Clickable(std::string("rrrange") + Rs[K], R, F.Live);
		F.C.FillRoundRect(R, 4.0f, On ? RrGold : St.Hover ? RrPanel2 : RrPanel);
		if (!On)
		{
			F.C.StrokeRoundRect(R, 4.0f, RrLine, 1.0f);
		}
		F.UI.Text(Rs[K], R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, Ts(12.5f, 800, On ? Hex(videoart::Ink) : RrMuted, Align::Center, Baseline::Middle));
		if (St.Clicked)
		{
			U.RangeIx = K;
		}
		Rx -= 4.0f;
	}
	const int Days = RrWindowDays(F);
	const int Today = net::DayOf(F.World);
	F.UI.Text(net::DateLabel(Today - Days + 1) + " \xE2\x80\x93 " + net::DateLabel(Today), Rx - 10.0f, 95.0f, Ts(12.5f, 600, RrMuted, Align::Right));
}

// ------------------------------------------------------------------ OVERVIEW (§5.3)

std::vector<double> RrNet(const video::Range& R)
{
	std::vector<double> Out;
	for (size_t K = 0; K < R.SubsGained.size(); ++K)
	{
		Out.push_back(R.SubsGained[K] - (K < R.SubsLost.size() ? R.SubsLost[K] : 0.0));
	}
	return Out;
}

void RrOverview(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const chart::Theme& T = RrTheme();
	const int Days = RrWindowDays(F);
	const bool Life = U.RangeIx == 4;
	const video::Range& Rg = RrRange(F, Days);
	const video::Range& Was = U.Before;
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	// The four tiles: the window's totals against the window before (words at +-10%), each with its days as a sparkline.
	const std::vector<double> Net = RrNet(Rg);
	std::vector<RrTileSpec> Tiles(4);
	Tiles[0].Label = "VIEWS";
	Tiles[0].Value = RrCount(RrSum(Rg.Views));
	Tiles[0].Spark = Rg.Views;
	Tiles[0].Note = RrVersus(RrSum(Rg.Views), RrSum(Was.Views), Days, Life);
	Tiles[1].Label = "WATCH TIME (HOURS)";
	Tiles[1].Value = RrCount(RrSum(Rg.WatchH));
	Tiles[1].Spark = Rg.WatchH;
	Tiles[1].SparkCol = RrSky;
	Tiles[1].Note = RrVersus(RrSum(Rg.WatchH), RrSum(Was.WatchH), Days, Life);
	const double Gained = RrSum(Rg.SubsGained), Lost = RrSum(Rg.SubsLost);
	double NetShown = 0.0, LostShown = 0.0, GainedShown = 0.0;
	RrSubsShown(Gained - Lost, Lost, NetShown, LostShown, GainedShown);
	Tiles[2].Label = "SUBSCRIBERS";
	Tiles[2].Value = (NetShown >= 0.0 ? "+" : "-") + RrCount(std::fabs(NetShown));
	Tiles[2].Sub = "(" + RrCount(LostShown) + " lost)";
	Tiles[2].Spark = Net;
	Tiles[2].SparkCol = RrGood;
	Tiles[2].Note = RrCount(GainedShown) + " gained \xC2\xB7 " + RrCount(LostShown) + " lost";
	Tiles[3].Label = "ESTIMATED REVENUE";
	Tiles[3].Value = RrCents(RrSum(Rg.Cents));
	Tiles[3].Spark = Rg.Cents;
	Tiles[3].SparkCol = RrCash;
	Tiles[3].Note = Partner ? RrSeasonWords(F.World) : std::string(RrAdsAtGate);
	if (!Life)
	{
		RrDelta(RrSum(Rg.Views), RrSum(Was.Views), Tiles[0]);
		RrDelta(RrSum(Rg.WatchH), RrSum(Was.WatchH), Tiles[1]);
		RrDelta(Gained - Lost, RrSum(RrNet(Was)), Tiles[2]);
		RrDelta(RrSum(Rg.Cents), RrSum(Was.Cents), Tiles[3]);
	}
	Tiles[static_cast<size_t>(std::clamp(U.KpiSel, 0, 3))].Selected = true;
	int Clicked = -1;
	RrTiles(F, {20.0f, 120.0f, NetW - 40.0f, 110.0f}, Tiles, 34.0f, "rrkpi", &Clicked);
	if (Clicked >= 0)
	{
		U.KpiSel = Clicked;
	}
	// The main chart: the selected tile's days over the typical range (the window before's middle half), the uploads pinned
	// and the verdicts numbered.
	const int Sel = std::clamp(U.KpiSel, 0, 3);
	const char* Names[4] = {"VIEWS", "WATCH TIME", "SUBSCRIBERS", "REVENUE"};
	const std::vector<double>& Ys = Sel == 0 ? Rg.Views : Sel == 1 ? Rg.WatchH : Sel == 2 ? Net : Rg.Cents;
	const std::vector<double> Prev = Sel == 0 ? Was.Views : Sel == 1 ? Was.WatchH : Sel == 2 ? RrNet(Was) : Was.Cents;
	const Rect Mc{20.0f, 246.0f, 1120.0f, 320.0f};
	RrCard(F, Mc, std::string(Names[Sel]) + " \xC2\xB7 " + (Life ? std::string("LIFETIME") : "LAST " + std::to_string(Days) + " DAYS"), Life ? std::string() : std::string("shaded: your typical range"));
	const int First = net::DayOf(F.World) - Days + 1;
	chart::Series Sv;
	Sv.Y = Ys;
	Sv.Width = 3.0f;
	Sv.Area = 0.35f;
	Sv.AreaFade = 0.06f;
	Sv.Name = Names[Sel];
	chart::Band Typ;
	if (!Life && !Prev.empty())
	{
		const double Lo = RrMid(Prev, 0.25), Hi = RrMid(Prev, 0.75);
		Typ.Lo.assign(Ys.size(), Lo);
		Typ.Hi.assign(Ys.size(), Hi);
	}
	std::vector<chart::Mark> Marks;
	std::vector<std::string> Legend;
	for (const video::Video* V : RrUploads(F.S))
	{
		const int D = net::DayOf(V->At) - First;
		if (D >= 0 && D < Days)
		{
			Marks.push_back({D, "", true, Color{0, 0, 0, 0}});
		}
		if (V->X.On && V->X.Verdict >= 3 && Legend.size() < 3)
		{
			const int Vd = video::valgo::VerdictDayOf(*V) - First;
			if (Vd >= 0 && Vd < Days)
			{
				Legend.push_back("(" + std::to_string(Legend.size() + 1) + ") \xE2\x80\x9C" + RrShorten(F.C, V->Title, 11.5f, 600, 220.0f) + "\xE2\x80\x9D: " + RrVerdictName(V->X.Verdict));
				Marks.push_back({Vd, std::to_string(Legend.size()), false, Color{0, 0, 0, 0}});
			}
		}
	}
	chart::LineOpts O;
	O.Y = Sel == 3 ? chart::Units::Money : chart::Units::Count;
	O.X.FirstDay = First;
	O.X.Count = Days <= 7 ? 7 : 9;
	O.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	O.Empty = RrNoSubs;
	const Rect Plot{Mc.X + 10.0f, Mc.Y + 40.0f, Mc.W - 20.0f, Mc.H - 70.0f};
	const chart::LineOut Lo = chart::LineChart(F.C, Plot, {Sv}, O, T, Typ.Lo.empty() ? nullptr : &Typ, &Marks);
	std::string LegendLine;
	for (const std::string& L : Legend)
	{
		LegendLine += (LegendLine.empty() ? std::string() : std::string("   ")) + L;
	}
	F.UI.Text(LegendLine, Mc.X + 18.0f, Mc.Y + Mc.H - 12.0f, Ts(11.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Mc.W - 36.0f));
	if (Lo.Hover >= 0 && Lo.Hover < static_cast<int>(Ys.size()))
	{
		std::vector<chart::TipRow> Rows = {{Names[Sel], Sel == 3 ? RrCents(Ys[static_cast<size_t>(Lo.Hover)]) : RrCount(Ys[static_cast<size_t>(Lo.Hover)]), RrGold, Color{0, 0, 0, 0}}};
		if (!Typ.Lo.empty())
		{
			Rows.push_back({"Typical", (Sel == 3 ? RrCents(Typ.Lo.front()) : RrCount(Typ.Lo.front())) + " to " + (Sel == 3 ? RrCents(Typ.Hi.front()) : RrCount(Typ.Hi.front())), NetA(RrCream, 0.3f), Color{0, 0, 0, 0}});
		}
		chart::Tooltip(F.C, Lo.HoverAt, chart::AxisDate(First + Lo.Hover), Rows, T, Mc);
	}
	// Realtime (§5.7's card).
	{
		const video::RealtimeView& Rt = RrRealtime(F);
		const Rect Rc{1156.0f, 246.0f, 424.0f, 320.0f};
		RrCard(F, Rc, "REALTIME", "updating live");
		F.C.FillCircle(Rc.X + Rc.W - 108.0f, Rc.Y + 22.0f, 4.0f, RrGood);
		double H48 = 0.0, M60 = 0.0;
		for (const double X : Rt.Hours)
		{
			H48 += X;
		}
		for (const double X : Rt.Minutes)
		{
			M60 += X;
		}
		F.UI.Text(RrCount(H48), Rc.X + 18.0f, Rc.Y + 62.0f, Ts(30.0f, 800, RrCream));
		F.UI.Text("views \xC2\xB7 last 48 hours", Rc.X + 18.0f + F.UI.Measure(RrCount(H48), 30.0f, 800) + 10.0f, Rc.Y + 60.0f, Ts(12.5f, 600, RrMuted));
		chart::BarSet Hb;
		Hb.V.assign(Rt.Hours.begin(), Rt.Hours.end());
		Hb.Col = Hex(0x8a6a3a);
		Hb.Each.assign(48, Color{0, 0, 0, 0});
		for (size_t K = 40; K < 48; ++K)
		{
			Hb.Each[K] = RrGold;
		}
		chart::BarOpts Bo;
		Bo.YLabels = false;
		Bo.Grid = false;
		Bo.Gap = 0.3f;
		Bo.X.Labels.assign(48, "");
		Bo.X.Labels[0] = "48 h ago";
		Bo.X.Labels[47] = "now";
		chart::Bars(F.C, {Rc.X + 10.0f, Rc.Y + 74.0f, Rc.W - 20.0f, 96.0f}, {Hb}, Bo, T);
		const double SubsHour = video::SubsAt(Ch, F.World) - video::SubsAt(Ch, F.World - 60.0);
		F.UI.Text(RrCount(M60), Rc.X + 18.0f, Rc.Y + 196.0f, Ts(22.0f, 800, RrCream));
		F.UI.Text("views \xC2\xB7 last 60 minutes \xC2\xB7 +" + RrCount(SubsHour) + " subscribers", Rc.X + 24.0f + F.UI.Measure(RrCount(M60), 22.0f, 800), Rc.Y + 195.0f,
			Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Rc.W - 80.0f));
		chart::BarSet Mb;
		Mb.V.assign(Rt.Minutes.begin(), Rt.Minutes.end());
		Mb.Col = RrSky;
		chart::BarOpts Bm;
		Bm.YLabels = false;
		Bm.XLabels = false;
		Bm.Grid = false;
		chart::Bars(F.C, {Rc.X + 10.0f, Rc.Y + 204.0f, Rc.W - 20.0f, 40.0f}, {Mb}, Bm, T);
		F.C.FillRect({Rc.X + 18.0f, Rc.Y + 252.0f, Rc.W - 36.0f, 1.0f}, RrLine);
		for (size_t K = 0; K < Rt.Top.size() && K < 3; ++K)
		{
			const float Yy = Rc.Y + 272.0f + Nf(K) * 18.0f;
			const bool Home = std::find(Rt.OnHomeNow.begin(), Rt.OnHomeNow.end(), Rt.Top[K].first) != Rt.OnHomeNow.end();
			F.UI.Text(Rt.Top[K].first, Rc.X + 18.0f, Yy, Ts(12.0f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, Home ? 224.0f : 330.0f));
			if (Home)
			{
				RrTag(F.C, "SHOWN ON HOME", Rc.X + 250.0f, Yy - 12.0f, RrGold, true, 8.5f);
			}
			F.UI.Text(RrCount(Rt.Top[K].second), Rc.X + Rc.W - 18.0f, Yy, Ts(12.0f, 700, RrMuted, Align::Right));
		}
	}
	// Top content (the window's most-watched; likes against dislikes, private).
	{
		const Rect Tc{20.0f, 582.0f, 700.0f, 368.0f};
		RrCard(F, Tc, "TOP CONTENT \xC2\xB7 " + (Life ? std::string("LIFETIME") : std::to_string(Days) + " DAYS"));
		const char* Cols[5] = {"VIEWS", "AVD", "CTR 48H", "LIKES", "REVENUE"};
		const float Cx[5] = {438.0f, 494.0f, 560.0f, 622.0f, 682.0f};
		for (int K = 0; K < 5; ++K)
		{
			F.UI.Text(Cols[K], Tc.X + Cx[K], Tc.Y + 26.0f, Ts(10.5f, 800, RrDim, Align::Right));
		}
		std::vector<std::pair<double, const video::Video*>> Ranked;
		const double From = F.World - static_cast<double>(Days) * net::MinutesPerDay;
		for (const video::Video* V : RrUploads(F.S))
		{
			Ranked.push_back({video::Views(*V, F.World) - video::Views(*V, std::max(V->At, From)), V});
		}
		std::stable_sort(Ranked.begin(), Ranked.end(), [](const std::pair<double, const video::Video*>& A, const std::pair<double, const video::Video*>& B) { return A.first > B.first; });
		// A top earner's revenues (review-c3f-ui): when one is too wide for its column, the column drops its cents, so no value
		// runs into LIKES (a typical channel's table is as it was).
		bool Whole = false;
		for (size_t K = 0; Partner && K < Ranked.size() && K < 5; ++K)
		{
			Whole = Whole || F.UI.Measure(RrCents(static_cast<double>(RrStats(F, *Ranked[K].second).EarnedCents)), 13.0f, 700) > Cx[4] - Cx[3] - 8.0f;
		}
		for (size_t K = 0; K < Ranked.size() && K < 5; ++K)
		{
			const video::Video& V = *Ranked[K].second;
			const video::VideoStats& St = RrStats(F, V);
			const float Yy = Tc.Y + 44.0f + Nf(K) * 58.0f;
			const Rect Row{Tc.X + 8.0f, Yy - 4.0f, Tc.W - 16.0f, 56.0f};
			const Ui::ClickState Cs = F.UI.Clickable("rrtop:" + V.Id, Row, F.Live);
			if (K % 2 == 0 || Cs.Hover)
			{
				F.C.FillRoundRect(Row, 6.0f, Rgba(255, 255, 255, Cs.Hover ? 0.05f : 0.02f));
			}
			F.UI.Text(std::to_string(K + 1), Tc.X + 24.0f, Yy + 30.0f, Ts(14.0f, 800, RrDim, Align::Center));
			const Rect Th{Tc.X + 40.0f, Yy, 88.0f, 49.5f};
			F.C.FillRoundRect(Th, 4.0f, RrGround);
			RrThumb(F, Th, V);
			const double Ratio = RrLikeRatio(St.Likes, St.Dislikes);
			const std::string Rev = !Partner ? std::string("\xE2\x80\x94") : Whole ? "$" + Grouped(std::max<Chips>(0, St.EarnedCents) / 100) : RrCents(static_cast<double>(St.EarnedCents));
			const std::string Vals[5] = {RrCount(Ranked[K].first), RrClock(St.AvdMin), V.F == video::Format::Short ? std::string("\xE2\x80\x94") : RrPct(St.Ctr48), RrPct(Ratio), Rev};
			// (A view count in the millions narrows the title beside it.)
			const float TitleW = std::min(240.0f, Cx[0] - F.UI.Measure(Vals[0], 13.0f, 700) - 152.0f);
			F.UI.Text(V.Title, Tc.X + 140.0f, Yy + 22.0f, Ts(13.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, TitleW));
			F.UI.Text(std::string(RrFormatName(V.F)) + " " + RrClock(V.LengthMin) + (St.Estimated ? std::string(" \xC2\xB7 est.") : std::string()), Tc.X + 140.0f, Yy + 41.0f, Ts(11.5f, 600, RrMuted));
			for (int C = 0; C < 5; ++C)
			{
				F.UI.Text(Vals[C], Tc.X + Cx[C], Yy + 31.0f, Ts(13.0f, 700, C == 3 ? (Ratio >= 0.96 ? RrGood : RrWarn) : RrCream, Align::Right));
			}
			if (Cs.Clicked)
			{
				RrOpenVideo(F, V.Id);
			}
		}
		RrDaily(F);
		F.UI.Text(video::HealthWords(0, U.Health.Momentum), Tc.X + 18.0f, Tc.Y + Tc.H - 16.0f, Ts(12.5f, 800, RrGold));
		if (RrLink(F, "rrseeall", "SEE ALL CONTENT \xE2\x80\xBA", Tc.X + Tc.W - 18.0f, Tc.Y + Tc.H - 16.0f, 12.0f, RrMuted, Align::Right))
		{
			U.Sub = 1;
			U.One.clear();
		}
	}
	// The latest upload against the typical one: its label, its retention against the typical curve, likes and dislikes, traffic.
	{
		const Rect Lc{736.0f, 582.0f, 404.0f, 368.0f};
		const std::vector<const video::Video*> Ups = RrUploads(F.S);
		if (Ups.empty())
		{
			RrCard(F, Lc, "LATEST");
			F.UI.Text(RrChannelEmpty, Lc.X + 18.0f, Lc.Y + 64.0f, Ts(14.0f, 600, RrMuted));
		}
		else
		{
			const video::Video& V = *Ups.front();
			const video::VideoStats& St = RrStats(F, V);
			RrCard(F, Lc, "LATEST \xC2\xB7 " + RrShorten(F.C, NetUpper(V.Title), 11.5f, 800, 220.0f));
			if (St.Verdict >= 0)
			{
				const float Tw = F.C.Measure(NetUpper(RrVerdictName(St.Verdict)), 9.5f, 900) + 16.0f;
				RrTag(F.C, NetUpper(RrVerdictName(St.Verdict)), Lc.X + Lc.W - 18.0f - Tw, Lc.Y + 12.0f, RrVerdictColor(St.Verdict), false, 9.5f);
			}
			int Rank = 0, Of = 0;
			double Typical = 0.0;
			RrRankAtAge(F, V, Rank, Of, Typical);
			const int Age = static_cast<int>(std::floor((F.World - V.At) / 60.0));
			const std::string AgeWords = Age >= 48 ? std::to_string(Age / 24) + " days" : std::to_string(Age) + " hours";
			F.UI.Text(AgeWords + " \xC2\xB7 " + RrNth(Rank) + " of your last " + std::to_string(Of) + " at " + AgeWords + " \xC2\xB7 " + RrClock(V.LengthMin), Lc.X + 18.0f, Lc.Y + 48.0f,
				Ts(11.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Lc.W - 36.0f));
			const Rect G{Lc.X + 18.0f, Lc.Y + 62.0f, Lc.W - 36.0f, 108.0f};
			F.C.FillRoundRect(G, 4.0f, Hex(0x000000, 0.18f));
			chart::Series Rs;
			Rs.Y.assign(St.Retention.begin(), St.Retention.end());
			Rs.Col = RrSky;
			Rs.Area = 0.12f;
			Rs.Width = 2.5f;
			chart::Series Ty;
			Ty.Y.assign(St.Typical.begin(), St.Typical.end());
			Ty.Col = NetA(RrMuted, 0.8f);
			Ty.Dash = 4.0f;
			Ty.Gap = 4.0f;
			Ty.Width = 1.5f;
			chart::LineOpts Ro;
			Ro.Y = chart::Units::Percent;
			Ro.YMin = 0.0;
			Ro.YMax = 1.0;
			Ro.YLabels = false;
			Ro.XLabels = false;
			Ro.Grid = false;
			chart::LineChart(F.C, G, {Ty, Rs}, Ro, T);
			F.UI.Text("AVD " + RrClock(St.AvdMin) + " (" + RrPct(St.AvgPct, 0) + ")", Lc.X + 18.0f, Lc.Y + 188.0f, Ts(11.0f, 600, RrMuted));
			F.UI.Text("dashed: your typical", Lc.X + Lc.W - 18.0f, Lc.Y + 188.0f, Ts(10.5f, 600, RrDim, Align::Right));
			const float Ly = Lc.Y + 206.0f;
			chart::DrawIcon(F.C, chart::Icon::ThumbUp, Lc.X + 28.0f, Ly + 1.0f, 16.0f, RrGood);
			F.UI.Text(RrCount(St.Likes), Lc.X + 42.0f, Ly + 6.0f, Ts(15.0f, 800, RrCream));
			chart::DrawIcon(F.C, chart::Icon::ThumbDown, Lc.X + 118.0f, Ly + 3.0f, 16.0f, RrBad);
			F.UI.Text(RrCount(St.Dislikes), Lc.X + 132.0f, Ly + 6.0f, Ts(15.0f, 800, RrCream));
			const double Ratio = RrLikeRatio(St.Likes, St.Dislikes);
			F.UI.Text(RrPct(Ratio), Lc.X + Lc.W - 18.0f, Ly + 6.0f, Ts(15.0f, 800, Ratio >= 0.96 ? RrGood : RrWarn, Align::Right));
			chart::RatioBar(F.C, {Lc.X + 18.0f, Ly + 14.0f, Lc.W - 36.0f, 10.0f}, std::floor(St.Likes), std::floor(St.Dislikes), T, -1.0, RrGood, RrBad);
			F.UI.Text(std::string(RrDislikesPrivate) + " \xC2\xB7 " + RrPlural(St.Comments, "comment", "comments") + " \xC2\xB7 " + RrPlural(St.Shares, "share", "shares"), Lc.X + 18.0f, Ly + 42.0f,
				Ts(10.5f, 600, RrDim, Align::Left, Baseline::Alphabetic, false, Lc.W - 36.0f));
			// Traffic sources as one stacked bar and the six biggest named.
			const float Sy = Ly + 66.0f;
			F.UI.Text("TRAFFIC SOURCES", Lc.X + 18.0f, Sy, Ts(10.5f, 800, RrMuted));
			double Total = 0.0;
			for (const double X : St.Src.Views)
			{
				Total += X;
			}
			std::vector<std::pair<double, int>> Order;
			for (int K = 0; K < 10; ++K)
			{
				Order.push_back({St.Src.Views[static_cast<size_t>(K)], K});
			}
			std::stable_sort(Order.begin(), Order.end(), [](const std::pair<double, int>& A, const std::pair<double, int>& B) { return A.first > B.first; });
			float Bx = Lc.X + 18.0f;
			const float Bw = Lc.W - 36.0f;
			for (const auto& P : Order)
			{
				if (Total <= 0.0 || P.first <= 0.0)
				{
					continue;
				}
				const float W = Bw * Nf(P.first / Total);
				F.C.FillRect({Bx, Sy + 8.0f, std::max(0.0f, W - 1.5f), 12.0f}, T.Series[static_cast<size_t>(P.second % 6)]);
				Bx += W;
			}
			for (size_t K = 0; K < Order.size() && K < 6 && Total > 0.0; ++K)
			{
				if (Order[K].first <= 0.0)
				{
					break;
				}
				const float Xx = Lc.X + 18.0f + Nf(K % 3) * 126.0f, Yy = Sy + 38.0f + Nf(K / 3) * 17.0f;
				F.C.FillRoundRect({Xx, Yy - 8.0f, 8.0f, 8.0f}, 2.0f, T.Series[static_cast<size_t>(Order[K].second % 6)]);
				F.UI.Text(std::string(RrSourceNames[Order[K].second]) + " " + RrPct(Order[K].first / Total, 0), Xx + 13.0f, Yy, Ts(11.0f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, 110.0f));
			}
			if (F.UI.Clickable("rrlatest", Lc, F.Live).Clicked)
			{
				RrOpenVideo(F, V.Id);
			}
		}
	}
	// How the money adds up (CPM to RPM, view-weighted over the window's long videos) with the season, and Channel health.
	{
		const Rect Vc{1156.0f, 582.0f, 424.0f, 368.0f};
		RrCard(F, Vc, Partner ? "HOW THE MONEY ADDS UP" : "BEFORE PARTNER");
		const int Today = net::DayOf(F.World);
		double W = 0.0, Cpm = 0.0, App = 0.0, Mon = 0.0, Share = 0.0, Rpm = 0.0;
		Chips Would = 0;
		const double From = F.World - static_cast<double>(Days) * net::MinutesPerDay;
		for (const video::Video* V : RrUploads(F.S))
		{
			if (!Partner)
			{
				Would += RrStats(F, *V).WouldEarnCents;
			}
			if (V->F == video::Format::Short)
			{
				continue;
			}
			const double Wt = video::Views(*V, F.World) - video::Views(*V, std::max(V->At, From));
			const video::algo::Money M = video::algo::MoneyOf(*V, Today);
			W += Wt;
			Cpm += Wt * M.Cpm;
			App += Wt * M.AdsPerPlayback;
			Mon += Wt * M.MonetizedRate;
			Share += Wt * M.Share;
			Rpm += Wt * M.Rpm;
		}
		const float Yy = Vc.Y + 56.0f;
		// The top-earner rate (H1): past it this pay month, the share and the RPM are what the next view pays now (the
		// advertisers' CPM is theirs, unchanged).
		const video::valgo::TopRateView Top = RrTopRate(F);
		if (Partner && Top.Applies)
		{
			Share *= Top.Share;
			Rpm *= Top.Share;
		}
		if (Partner && W > 0.0)
		{
			F.UI.Text("CPM", Vc.X + 18.0f, Yy, Ts(11.0f, 800, RrMuted));
			F.UI.Text(Money(static_cast<Chips>(std::llround(Cpm / W * 100.0))), Vc.X + 18.0f, Yy + 30.0f, Ts(26.0f, 800, RrCream));
			const std::string Steps[3][2] = {{"\xC3\x97 " + Fixed(App / W, 2), "ads a playback"}, {"\xC3\x97 " + RrPct(Mon / W, 0), "showed an ad"},
				{"\xC3\x97 " + RrPct(Share / W, 0), Top.Applies ? "top-earner share" : "your share"}}; // your share at the top-earner rate
			for (int K = 0; K < 3; ++K)
			{
				F.UI.Text(Steps[K][0], Vc.X + 120.0f + Nf(K) * 98.0f, Yy + 28.0f, Ts(16.0f, 800, RrGold));
				F.UI.Text(Steps[K][1], Vc.X + 120.0f + Nf(K) * 98.0f, Yy + 44.0f, Ts(9.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 94.0f));
			}
			F.C.FillRect({Vc.X + 18.0f, Yy + 58.0f, Vc.W - 36.0f, 1.0f}, RrLine);
			F.UI.Text("RPM", Vc.X + 18.0f, Yy + 88.0f, Ts(11.0f, 800, RrMuted));
			F.UI.Text(Money(static_cast<Chips>(std::llround(Rpm / W * 100.0))), Vc.X + 62.0f, Yy + 90.0f, Ts(24.0f, 800, RrCash));
			F.UI.Text("long videos", Vc.X + 160.0f, Yy + 82.0f, Ts(11.0f, 600, RrMuted));
			const double AllViews = RrSum(Rg.Views);
			if (AllViews > 0.0)
			{
				F.UI.Text(Money(static_cast<Chips>(std::llround(RrSum(Rg.Cents) / AllViews * 1000.0))) + " counting every view", Vc.X + 160.0f, Yy + 96.0f, Ts(11.0f, 600, RrMuted));
			}
		}
		else if (!Partner)
		{
			NetParagraph(F.C, RrAdsBeforePartner, Vc.X + 18.0f, Yy, Vc.W - 36.0f, 13.0f, 600, RrCream, 18.0f, 2);
			if (Would > 0)
			{
				NetParagraph(F.C, "As a Partner, your videos would have earned about " + Money(Would) + " so far (not paid)", Vc.X + 18.0f, Yy + 40.0f, Vc.W - 36.0f, 12.0f, 700, RrCash, 16.0f, 2);
			}
			F.UI.Text(RrNotPaid, Vc.X + 18.0f, Yy + 84.0f, Ts(11.5f, 600, RrMuted));
		}
		// The season: the months around this one, this one lit (§4.5.3, the advertisers' calendar).
		const float Sy = Vc.Y + 176.0f;
		int Y = 0, Mo = 0, D = 0;
		world::CivilDate(Today, Y, Mo, D);
		for (int K = -1; K <= 4; ++K)
		{
			int Mm = Mo + K, Yr = Y;
			while (Mm < 1)
			{
				Mm += 12;
				--Yr;
			}
			while (Mm > 12)
			{
				Mm -= 12;
				++Yr;
			}
			// The middle of that month, from today: the season's own value on that day.
			const int Mid = Today + static_cast<int>(std::lround(30.4 * K)) - (D - 15);
			const float H = Nf(video::algo::Season(Mid)) * 22.0f;
			const float Bx = Vc.X + 18.0f + Nf(K + 1) * 22.0f;
			F.C.FillRoundRect({Bx, Sy + 30.0f - H, 16.0f, H}, 2.0f, K == 0 ? RrGold : Hex(0x5a4a30));
			F.UI.Text(std::string(1, RrMonthShort[Mm - 1][0]), Bx + 8.0f, Sy + 44.0f, Ts(10.0f, 700, K == 0 ? RrCream : RrDim, Align::Center));
		}
		NetParagraph(F.C, std::string("\xE2\x80\x9C") + RrSeasonLine + "\xE2\x80\x9D", Vc.X + 160.0f, Sy + 12.0f, Vc.W - 178.0f, 11.0f, 600, RrMuted, 15.0f, 2);
		if (Partner)
		{
			// Above Channel health's card (its top is Vc.Y + 230): the season's two lines end at Sy + 27.
			F.UI.Text("Balance " + Money(Ch.BalanceCents), Vc.X + 160.0f, Sy + 46.0f, Ts(11.0f, 700, RrCream));
		}
		RrHealthCard(F, {Vc.X + 8.0f, Vc.Y + 230.0f, Vc.W - 16.0f, 130.0f});
	}
}

// ------------------------------------------------------------------ CONTENT: the table (§5.4)

struct RrCol
{
	const char* Name;
	float X; // the column's right edge
};
const RrCol RrTableCols[10] = {{"VIEWS", 700.0f}, {"WATCH H", 790.0f}, {"SUBS", 860.0f}, {"IMPRESSIONS", 975.0f}, {"CTR 48H", 1060.0f}, {"AVD", 1130.0f},
	{"LIKES \xC2\xB7 DISLIKES", 1300.0f}, {"REVENUE", 1405.0f}, {"COIN", 1460.0f}, {"TREND", 1540.0f}};

double RrSortValue(RrFrame& F, const video::Video& V, int Col)
{
	const video::VideoStats& St = RrStats(F, V);
	switch (Col)
	{
	case 0: return St.Views;
	case 1: return St.WatchHours;
	case 2: return St.SubsNet;
	case 3: return St.Impressions;
	case 4: return St.Ctr48;
	case 5: return St.AvdMin;
	case 6: return RrLikeRatio(St.Likes, St.Dislikes);
	case 7: return static_cast<double>(St.EarnedCents);
	case 8: return static_cast<double>(St.Coin);
	default: return video::Views(V, F.World) - video::Views(V, F.World - net::MinutesPerDay);
	}
}

void RrContentTable(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	std::vector<const video::Video*> List = RrUploads(F.S);
	F.UI.Text(std::to_string(List.size()) + (List.size() == 1 ? " video" : " videos") + " \xC2\xB7 the newest 60 in full", 20.0f, 140.0f, Ts(12.5f, 600, RrMuted));
	// The header (click to sort; again to turn it round).
	const float Hy = 166.0f;
	F.C.FillRect({20.0f, Hy + 10.0f, NetW - 40.0f, 1.0f}, RrLine);
	F.UI.Text("VIDEO", 140.0f, Hy, Ts(10.5f, 800, RrDim));
	for (int K = 0; K < 10; ++K)
	{
		const float W = F.UI.Measure(RrTableCols[K].Name, 10.5f, 800);
		const Rect R{RrTableCols[K].X - W - 14.0f, Hy - 14.0f, W + 16.0f, 22.0f};
		const Ui::ClickState St = F.UI.Clickable(std::string("rrsort") + RrTableCols[K].Name, R, F.Live);
		const bool On = U.SortCol == K;
		F.UI.Text(RrTableCols[K].Name, RrTableCols[K].X, Hy, Ts(10.5f, 800, On ? RrGold : St.Hover ? RrCream : RrDim, Align::Right));
		if (On)
		{
			NetTriangle(F.C, RrTableCols[K].X - W - 8.0f, Hy - 4.0f, 6.0f, U.SortUp, RrGold);
		}
		if (St.Clicked)
		{
			U.SortUp = On ? !U.SortUp : false;
			U.SortCol = K;
		}
	}
	if (U.SortCol >= 0)
	{
		std::vector<std::pair<double, const video::Video*>> Keyed;
		for (const video::Video* V : List)
		{
			Keyed.push_back({RrSortValue(F, *V, U.SortCol), V});
		}
		const bool Up = U.SortUp;
		std::stable_sort(Keyed.begin(), Keyed.end(), [Up](const std::pair<double, const video::Video*>& A, const std::pair<double, const video::Video*>& B) {
			return Up ? A.first < B.first : A.first > B.first;
		});
		for (size_t K = 0; K < Keyed.size(); ++K)
		{
			List[K] = Keyed[K].second;
		}
	}
	const Rect Area{20.0f, 182.0f, NetW - 40.0f, VideoH - 14.0f - 182.0f};
	const float RowH = 64.0f;
	const bool Folded = !Ch.Stubs.empty() || !Ch.X.Top.empty();
	const float Content = Nf(List.size()) * RowH + (Folded ? 70.0f + Nf(std::min<size_t>(Ch.X.Top.size(), 20)) * 26.0f + 40.0f : 0.0f);
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.TableGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * RowH * 2.0f;
	}
	U.TableGoal = std::clamp(U.TableGoal, 0.0f, MaxScroll);
	U.TableScroll += (U.TableGoal - U.TableScroll) * NetFollow(F.Dt, 18.0);
	U.TableScroll = std::clamp(U.TableScroll, 0.0f, MaxScroll);
	if (std::fabs(U.TableGoal - U.TableScroll) < 0.5f)
	{
		U.TableScroll = U.TableGoal;
	}
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < List.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * RowH - U.TableScroll;
		if (Y + RowH < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		const video::Video& V = *List[I];
		const video::VideoStats& St = RrStats(F, V);
		const Rect Row{Area.X, Y + 2.0f, Area.W, RowH - 4.0f};
		const Ui::ClickState Cs = F.UI.Clickable("rrrow:" + V.Id, Row, F.Live && PointerIn);
		F.C.FillRoundRect(Row, 6.0f, Cs.Hover ? RrPanel2 : I % 2 == 0 ? RrPanel : Mix(RrPanel, RrGround, 0.5f));
		const Rect Th{Row.X + 10.0f, Row.Y + 3.0f, 96.0f, 54.0f};
		F.C.FillRoundRect(Th, 4.0f, RrGround);
		RrThumb(F, Th, V);
		RrLengthBadge(F, Th, V);
		// The title, its tags (R1, R2, R4) and when it went up.
		const float Tx = 136.0f;
		F.UI.Text(V.Title, Tx, Row.Y + 24.0f, Ts(13.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 460.0f));
		std::string Meta = RrWhen(V.At);
		F.UI.Text(Meta, Tx, Row.Y + 46.0f, Ts(11.5f, 600, RrMuted));
		float Gx = Tx + F.UI.Measure(Meta, 11.5f, 600) + 10.0f;
		if (St.Tag > 0)
		{
			Gx += RrTag(F.C, NetUpper(video::valgo::TagName(St.Tag)), Gx, Row.Y + 35.0f, St.Tag == 1 ? RrSky : St.Tag == 2 ? RrGold : RrCream, St.Tag >= 2, 8.5f) + 5.0f;
		}
		if (St.LateBloom)
		{
			Gx += RrTag(F.C, "LATE BLOOMER", Gx, Row.Y + 35.0f, RrGood, false, 8.5f) + 5.0f;
		}
		if (St.Binged > 0)
		{
			Gx += RrTag(F.C, "BINGE WAVE", Gx, Row.Y + 35.0f, RrGold, false, 8.5f) + 5.0f;
		}
		const bool Cut = V.F == video::Format::Short;
		const double Ratio = RrLikeRatio(St.Likes, St.Dislikes);
		const std::string Est = St.Estimated ? " est." : "";
		const std::string Vals[8] = {RrCount(St.Views), Cut ? std::string("\xE2\x80\x94") : Fixed(St.WatchHours, St.WatchHours < 10.0 ? 1 : 0), "+" + RrCount(St.SubsNet),
			RrShort(St.Impressions) + Est, RrPct(St.Ctr48) + Est, RrClock(St.AvdMin), RrCount(St.Likes) + " \xC2\xB7 " + RrCount(St.Dislikes) + "  " + RrPct(Ratio),
			Partner ? RrCents(static_cast<double>(St.EarnedCents)) : std::string("\xE2\x80\x94")};
		for (int K = 0; K < 8; ++K)
		{
			F.UI.Text(Vals[K], RrTableCols[K].X, Row.Y + 30.0f, Ts(13.0f, 700, K == 6 ? (Ratio >= 0.96 ? RrCream : RrWarn) : RrCream, Align::Right));
		}
		if (Cut)
		{
			// A Cut has no thumbnail and no impressions in a feed: the columns relabelled (§5.4).
			F.UI.Text("shown in feed", RrTableCols[3].X, Row.Y + 46.0f, Ts(10.0f, 600, RrMuted, Align::Right));
			F.UI.Text("stayed to watch", RrTableCols[4].X, Row.Y + 46.0f, Ts(10.0f, 600, RrMuted, Align::Right));
		}
		// The coin: full ads (green), limited (amber), feed ads for a Cut.
		const Color Coin = St.Coin == 1 ? Hex(0xf2a93b) : St.Coin == 2 ? RrBad : RrGood;
		F.C.FillCircle(RrTableCols[8].X - 10.0f, Row.Y + 25.0f, 7.0f, Cut ? RrMuted : Coin);
		// The trend: the last day against the day before.
		const double Last = video::Views(V, F.World) - video::Views(V, F.World - net::MinutesPerDay);
		const double Prior = video::Views(V, F.World - net::MinutesPerDay) - video::Views(V, F.World - 2.0 * net::MinutesPerDay);
		if (Last > 0.5 || Prior > 0.5)
		{
			NetTriangle(F.C, RrTableCols[9].X - 12.0f, Row.Y + 24.0f, 9.0f, Last >= Prior, Last >= Prior ? RrGood : RrWarn);
		}
		if (Cs.Clicked)
		{
			U.One = V.Id;
			U.OneTab = 0;
		}
	}
	if (Folded)
	{
		// Older videos: one folded row, then the all-time top 20 (video\tvtop).
		float Y = Area.Y + Nf(List.size()) * RowH - U.TableScroll + 10.0f;
		double Kept = 0.0;
		for (const video::Video& V : Ch.Videos)
		{
			Kept += video::Views(V, F.World);
		}
		F.C.FillRoundRect({Area.X, Y, Area.W, 50.0f}, 6.0f, RrPanel);
		F.UI.Text("OLDER VIDEOS", Area.X + 18.0f, Y + 30.0f, Ts(12.0f, 900, RrMuted));
		F.UI.Text(RrViews(std::max(0.0, video::TotalViews(Ch, F.World) - Kept)) + " \xC2\xB7 " + std::to_string(Ch.Stubs.size()) + " still found by search", Area.X + 150.0f, Y + 30.0f,
			Ts(13.0f, 700, RrCream));
		Y += 70.0f;
		if (!Ch.X.Top.empty())
		{
			F.UI.Text("ALL-TIME TOP 20", Area.X + 18.0f, Y, Ts(12.0f, 900, RrGold));
			Y += 26.0f;
			for (size_t K = 0; K < Ch.X.Top.size() && K < 20; ++K)
			{
				const video::TopRow& T = Ch.X.Top[K];
				F.UI.Text(std::to_string(K + 1) + ". " + T.Title, Area.X + 18.0f, Y, Ts(12.5f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, 900.0f));
				F.UI.Text(RrViews(T.Views) + (Partner ? " \xC2\xB7 " + Money(T.Cents) : std::string()), Area.X + Area.W - 18.0f, Y, Ts(12.5f, 700, RrMuted, Align::Right));
				Y += 26.0f;
			}
		}
	}
	F.C.PopClip();
	if (List.empty())
	{
		F.UI.Text(RrChannelEmpty, NetW / 2.0f, 320.0f, Ts(22.0f, 900, RrCream, Align::Center));
		F.UI.Text(RrChannelEmptyMore, NetW / 2.0f, 350.0f, Ts(14.0f, 500, RrMuted, Align::Center));
	}
	if (MaxScroll > 0.0f)
	{
		const float Th = std::max(40.0f, Area.H * Area.H / Content);
		const float Ty = Area.Y + (Area.H - Th) * (U.TableScroll / MaxScroll);
		F.C.FillRoundRect({Area.X + Area.W + 8.0f, Ty, 4.0f, Th}, 2.0f, NetA(RrCream, 0.25f));
	}
}

// ------------------------------------------------------------------ CONTENT: one video (§5.4, the creator mockup)

/** The list of your videos on the left (20, 120, 360, 830). */
void RrOneList(RrFrame& F, const std::vector<const video::Video*>& List)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const Rect R{20.0f, 120.0f, 360.0f, 830.0f};
	RrCard(F, R, "YOUR VIDEOS", "newest first");
	const Rect Area{R.X, R.Y + 38.0f, R.W, R.H - 44.0f};
	const float RowH = 98.0f;
	const float Content = Nf(List.size()) * RowH;
	const float MaxScroll = std::max(0.0f, Content - Area.H);
	if (F.Live && F.UI.Hover(Area) && F.UI.Ptr.Wheel != 0.0f)
	{
		U.OneGoal += (F.UI.Ptr.Wheel > 0.0f ? 1.0f : -1.0f) * RowH;
	}
	U.OneGoal = std::clamp(U.OneGoal, 0.0f, MaxScroll);
	U.OneScroll += (U.OneGoal - U.OneScroll) * NetFollow(F.Dt, 18.0);
	U.OneScroll = std::clamp(U.OneScroll, 0.0f, MaxScroll);
	if (std::fabs(U.OneGoal - U.OneScroll) < 0.5f)
	{
		U.OneScroll = U.OneGoal;
	}
	const bool PointerIn = F.UI.Hover(Area);
	F.C.PushClip(Area);
	for (size_t I = 0; I < List.size(); ++I)
	{
		const float Y = Area.Y + Nf(I) * RowH - U.OneScroll;
		if (Y + RowH < Area.Y || Y > Area.Y + Area.H)
		{
			continue;
		}
		const video::Video& V = *List[I];
		const video::VideoStats& St = RrStats(F, V);
		const bool Sel = V.Id == U.One;
		const Rect Row{R.X + 8.0f, Y, R.W - 16.0f, RowH - 6.0f};
		const Ui::ClickState Cs = F.UI.Clickable("rrone:" + V.Id, Row, F.Live && PointerIn);
		if (Sel)
		{
			F.C.FillRoundRect(Row, 8.0f, RrPanel2);
			F.C.StrokeRoundRect(Row, 8.0f, RrGold, 1.4f);
		}
		else if (St.Binged > 0)
		{
			F.C.FillRoundRect(Row, 8.0f, NetA(RrGold, 0.08f));
			F.C.FillRoundRect({Row.X, Row.Y, 3.0f, Row.H}, 1.5f, RrGold);
		}
		else if (Cs.Hover)
		{
			F.C.FillRoundRect(Row, 8.0f, NetA(RrPanel2, 0.6f));
		}
		const Rect Th{Row.X + 10.0f, Row.Y + 8.0f, 112.0f, 63.0f};
		F.C.FillRoundRect(Th, 4.0f, RrGround);
		RrThumb(F, Th, V);
		RrLengthBadge(F, Th, V);
		NetParagraph(F.C, V.Title, Row.X + 134.0f, Row.Y + 22.0f, Row.W - 144.0f, 13.0f, 700, RrCream, 16.0f, 2);
		F.UI.Text(std::string(RrFormatName(V.F)) + " " + RrClock(V.LengthMin) + " \xC2\xB7 " + RrAge(F.World - V.At), Row.X + 134.0f, Row.Y + 58.0f, Ts(11.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Row.W - 144.0f));
		F.UI.Text(RrViews(St.Views), Row.X + 134.0f, Row.Y + 76.0f, Ts(11.5f, 700, RrCream));
		if (St.Tag > 0 || St.LateBloom)
		{
			const std::string Tag = St.LateBloom ? std::string("LATE BLOOMER") : NetUpper(video::valgo::TagName(St.Tag));
			const float Tw = F.C.Measure(Tag, 8.5f, 900) + 16.0f;
			RrTag(F.C, Tag, Row.X + Row.W - 8.0f - Tw, Row.Y + 64.0f, St.LateBloom ? RrGood : St.Tag == 1 ? RrSky : RrGold, !St.LateBloom && St.Tag >= 2, 8.5f);
		}
		if (Cs.Clicked)
		{
			U.One = V.Id;
		}
	}
	F.C.PopClip();
}

/** The five-step scale with the label lit (R1: the honest verdict), the bell's sentence, a breakout's note. */
void RrVerdictBlock(RrFrame& F, const video::Video& V, const video::VideoStats& St, const Rect& R)
{
	using namespace rlnet_detail;
	const int Day = video::valgo::VerdictDayOf(V);
	const std::string When = V.X.On && St.Verdict >= 0 ? "THE VERDICT \xC2\xB7 " + NetUpper(net::WeekdayName(Day)) + " MIDNIGHT" : std::string("THE VERDICT");
	RrCard(F, R, When, RrVerdictHow);
	const float Lw = (R.W - 36.0f - 4.0f * 6.0f) / 5.0f;
	for (int K = 0; K < 5; ++K)
	{
		const bool On = K == St.Verdict;
		const Rect B{R.X + 18.0f + Nf(K) * (Lw + 6.0f), R.Y + 38.0f, Lw, 26.0f};
		F.C.FillRoundRect(B, 13.0f, On ? RrVerdictColor(K) : RrPanel2);
		F.UI.Text(RrVerdictName(K), B.X + B.W / 2.0f, B.Y + 18.0f, Ts(12.5f, On ? 900 : 600, On ? Hex(videoart::Ink) : RrMuted, Align::Center));
	}
	std::string Line, Second;
	if (!V.X.On)
	{
		Line = RrLegacyRow;
	}
	else if (St.Verdict < 0)
	{
		Line = RrBeforeVerdict;
	}
	else
	{
		Line = video::valgo::BellVerdict(V.Title, St.Verdict);
		if (St.MomCAtUpload > 1.1 && !video::valgo::VerdictDespite(St.Verdict).empty())
		{
			Second = video::valgo::VerdictDespite(St.Verdict);
		}
		if (St.Viral)
		{
			Second = video::valgo::BellViral(V.Title);
		}
		else
		{
			for (const video::Comp& K : V.X.Comps)
			{
				Second = K.Kind == video::CompKind::Breakout && K.T <= F.World ? video::valgo::BellBreakout(V.Title) : Second;
			}
		}
	}
	F.UI.Text(Line, R.X + 18.0f, R.Y + 88.0f, Ts(13.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
	if (!Second.empty())
	{
		F.UI.Text(Second, R.X + 18.0f, R.Y + 106.0f, Ts(12.5f, 800, RrGold, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
	}
}

/** "What Rerun did with it" (§5.4.1): each line's when and what, the kicks in marigold. */
void RrLogCard(RrFrame& F, const video::VideoStats& St, const Rect& R)
{
	RrCard(F, R, "WHAT RERUN DID WITH IT");
	float Y = R.Y + 48.0f;
	const std::string Dot = " \xC2\xB7 ";
	for (const auto& L : St.Log)
	{
		if (Y > R.Y + R.H - 10.0f)
		{
			break;
		}
		const size_t At = L.second.find(Dot);
		if (At == std::string::npos)
		{
			Y = rlnet_detail::NetParagraph(F.C, L.second, R.X + 18.0f, Y, R.W - 36.0f, 11.5f, 600, RrCream, 15.0f, 2) + 6.0f;
			continue;
		}
		const std::string When = L.second.substr(0, At), What = L.second.substr(At + Dot.size());
		const bool Kick = What.find("push") != std::string::npos || What.find("breakout") != std::string::npos || What.find("next to other") != std::string::npos;
		F.UI.Text(When, R.X + 18.0f, Y, Ts(11.5f, 800, Kick ? RrGold : RrMuted, Align::Left, Baseline::Alphabetic, false, 96.0f));
		Y = rlnet_detail::NetParagraph(F.C, What, R.X + 118.0f, Y, R.W - 136.0f, 11.5f, 600, RrCream, 15.0f, 2) + 5.0f;
	}
}

void RrInsightsCard(RrFrame& F, const video::VideoStats& St, const Rect& R)
{
	RrCard(F, R, "INSIGHTS");
	float Y = R.Y + 50.0f;
	if (St.Insights.empty())
	{
		F.UI.Text(St.Estimated ? RrLegacyRow : RrBeforeVerdict, R.X + 18.0f, Y, Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
	}
	for (const std::string& L : St.Insights)
	{
		if (Y > R.Y + R.H - 10.0f)
		{
			break;
		}
		F.C.FillCircle(R.X + 22.0f, Y - 4.0f, 3.5f, RrGood);
		Y = rlnet_detail::NetParagraph(F.C, L, R.X + 32.0f, Y, R.W - 50.0f, 12.0f, 600, RrCream, 16.0f, 3) + 10.0f;
	}
}

/** One video's views a day since it went up (up to 28 days), against the typical upload's at the same day (dashed). */
void RrDailyChart(RrFrame& F, const video::Video& V, const Rect& R, bool Impressions)
{
	const int Days = std::clamp(static_cast<int>(std::ceil((F.World - V.At) / net::MinutesPerDay)), 1, 28);
	VideoUi& U = F.U;
	const double Minute = std::floor(F.World);
	if (U.DailyId != V.Id || U.DailyMinute != Minute || U.DailyMine.size() != static_cast<size_t>(Days))
	{
		// The video's own closed form a day at a time, and the 9 uploads before it at the same days (their median), once a
		// world minute.
		U.DailyId = V.Id;
		U.DailyMinute = Minute;
		U.OneOf = 0; // the rank at its age reads again (RrRankAtAge)
		U.DailyMine.clear();
		U.DailyTyp.clear();
		U.DailyImpr.clear();
		std::vector<const video::Video*> Before;
		for (const video::Video* O : RrUploads(F.S))
		{
			if (O != &V && O->At < V.At && Before.size() < 9)
			{
				Before.push_back(O);
			}
		}
		for (int D = 0; D < Days; ++D)
		{
			const double A = V.At + static_cast<double>(D) * net::MinutesPerDay, B = std::min(F.World, A + net::MinutesPerDay);
			U.DailyMine.push_back(std::max(0.0, video::Views(V, B) - video::Views(V, A)));
			U.DailyImpr.push_back(std::max(0.0, video::valgo::ImprGain(V, A, B)));
			std::vector<double> At;
			for (const video::Video* O : Before)
			{
				const double Oa = O->At + static_cast<double>(D) * net::MinutesPerDay;
				At.push_back(std::max(0.0, video::Views(*O, Oa + net::MinutesPerDay) - video::Views(*O, Oa)));
			}
			U.DailyTyp.push_back(RrMid(At, 0.5));
		}
	}
	const std::vector<double>& Mine = U.DailyMine;
	const std::vector<double>& Typ = U.DailyTyp;
	const std::vector<double>& Impr = U.DailyImpr;
	chart::Series Sv;
	Sv.Y = Mine;
	Sv.Width = 3.0f;
	Sv.Area = 0.35f;
	Sv.AreaFade = 0.06f;
	Sv.Name = "Views";
	Sv.Col = RrGold; // (a series without its own colour takes the theme's by its place: this one is second)
	Sv.Dots = Days <= 10;
	chart::Series Ts2;
	Ts2.Y = Typ;
	Ts2.Col = rlnet_detail::NetA(RrMuted, 0.8f);
	Ts2.Dash = 5.0f;
	Ts2.Gap = 4.0f;
	Ts2.Width = 1.6f;
	Ts2.Name = "Typical";
	const std::vector<chart::Series> Lines = {Ts2, Sv};
	Rect Vr = R;
	if (Impressions)
	{
		// Impressions run ten to forty times the views: their own chart above the views', on the same days.
		chart::Series Is;
		Is.Y = Impr;
		Is.Col = RrSky;
		Is.Width = 2.0f;
		Is.Area = 0.2f;
		Is.AreaFade = 0.06f;
		Is.Name = "Impressions";
		chart::LineOpts Io;
		Io.XLabels = false;
		Io.YTicks = 2;
		const Rect Ir{R.X, R.Y, R.W, R.H * 0.38f};
		chart::LineChart(F.C, Ir, {Is}, Io, RrTheme());
		F.UI.Text("impressions", R.X + R.W - 8.0f, R.Y + 12.0f, Ts(10.5f, 700, RrSky, Align::Right));
		Vr = {R.X, R.Y + R.H * 0.42f, R.W, R.H * 0.58f};
		F.UI.Text("views", R.X + R.W - 8.0f, Vr.Y + 12.0f, Ts(10.5f, 700, RrGold, Align::Right));
	}
	// The marks: the verdict, a breakout, Live Drop's push, the check-ins.
	std::vector<chart::Mark> Marks;
	if (V.X.On)
	{
		const int Vd = video::valgo::VerdictDayOf(V) - net::DayOf(V.At);
		if (V.X.Verdict >= 0 && Vd < Days)
		{
			Marks.push_back({Vd, "V", false, Color{0, 0, 0, 0}});
		}
		for (const video::Comp& K : V.X.Comps)
		{
			const int D = static_cast<int>(std::floor((K.T - V.At) / net::MinutesPerDay));
			if (D >= 0 && D < Days && K.T <= F.World && (K.Kind == video::CompKind::Breakout || K.Kind == video::CompKind::Kast || K.Kind == video::CompKind::Check7))
			{
				Marks.push_back({D, K.Kind == video::CompKind::Breakout ? "B" : K.Kind == video::CompKind::Kast ? "K" : "7", false, Color{0, 0, 0, 0}});
			}
		}
	}
	chart::LineOpts O;
	std::vector<std::string> Labels;
	for (int D = 0; D < Days; ++D)
	{
		Labels.push_back(D == 0 ? std::string("day 1") : (D + 1) % 7 == 0 ? "day " + std::to_string(D + 1) : std::string());
	}
	O.X.Labels = Labels;
	O.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	const chart::LineOut Lo = chart::LineChart(F.C, Vr, Lines, O, RrTheme(), nullptr, &Marks);
	if (Lo.Hover >= 0 && Lo.Hover < Days)
	{
		std::vector<chart::TipRow> Rows = {{"Views", RrCount(Mine[static_cast<size_t>(Lo.Hover)]), RrGold, Color{0, 0, 0, 0}},
			{"Typical", RrCount(Typ[static_cast<size_t>(Lo.Hover)]), rlnet_detail::NetA(RrMuted, 0.8f), Color{0, 0, 0, 0}}};
		if (Impressions)
		{
			Rows.push_back({"Impressions", RrCount(Impr[static_cast<size_t>(Lo.Hover)]), RrSky, Color{0, 0, 0, 0}});
		}
		chart::Tooltip(F.C, Lo.HoverAt, "day " + std::to_string(Lo.Hover + 1), Rows, RrTheme(), R);
	}
}

/** A kept Rerun 2.0 video the player can still repackage (§4.6.7: up to 28 days old, twice at most). */
bool RrCanRepack(const RrFrame& F, const video::Video& V)
{
	return V.X.On && F.World <= V.At + 28.0 * 1440.0 && V.X.Repacks < 2;
}

/**
 * C3 gate (update 3 §4.6.7, §6.6.1's REPACKAGE): CHANGE THE PACKAGING on one video's Overview: the first 48 hours' CTR against
 * the channel's typical (its last ten long Rerun 2.0 uploads' realized first-48-hour CTRs), the library's three titles and six
 * thumbnails for this video (video::RepackTitles / RepackThumbs, SHUFFLE deals again; a Cut has titles only), and a press
 * repackages through Session::Repackage, whose refusal (three days apart, twice, 28 days) or a done line shows under it.
 */
void RrRepackCard(RrFrame& F, const video::Video& V, const video::VideoStats& St, const Rect& R)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const bool Cut = V.F == video::Format::Short;
	const std::string Key = V.Id + "#" + std::to_string(V.X.Repacks) + "#" + std::to_string(U.RepackShuffle) + "#" + V.X.TitleTpl + "#" +
		std::to_string(V.X.Spec[0].Arch) + "." + std::to_string(V.X.Spec[0].Variant);
	if (Key != U.RepackKey)
	{
		U.RepackKey = Key;
		U.RepackT = video::RepackTitles(V, Ch, U.RepackShuffle, 3);
		U.RepackS = Cut ? std::vector<video::ThumbSpec>() : video::RepackThumbs(V, Ch, U.RepackShuffle, 6);
	}
	RrCard(F, R, "CHANGE THE PACKAGING", V.X.Repacks == 0 ? "2 changes left" : "1 change left");
	float Y = R.Y + 58.0f;
	// The packaging read against the video's real numbers: its first 48 hours' CTR and the channel's typical.
	std::vector<double> Typ;
	for (auto It = Ch.Videos.rbegin(); It != Ch.Videos.rend() && Typ.size() < 10; ++It)
	{
		if (It->Id != V.Id && It->X.On && It->F != video::Format::Short)
		{
			Typ.push_back(It->X.Ctr * It->X.CtrNoise);
		}
	}
	std::sort(Typ.begin(), Typ.end());
	if (!Cut)
	{
		const std::string Read = "This video's first 48 hours: CTR " + RrPct(St.Ctr48) + (Typ.empty() ? std::string(".") : ", your typical " + RrPct(Typ[Typ.size() / 2]) + ".");
		F.UI.Text(Read, R.X + 18.0f, Y, Ts(12.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, R.W - 36.0f));
		Y += 22.0f;
	}
	auto Press = [&](const video::Repack& Rp) {
		const std::string Why = F.S.Repackage(V.Id, Rp);
		U.RepackSaid = Why.empty() ? std::string(RrRepackDone) : Why;
		U.RepackKey.clear();
	};
	F.UI.Text("TITLE", R.X + 18.0f, Y + 4.0f, Ts(10.5f, 800, RrMuted));
	if (RrButton(F, "rrrepshuf", {R.X + R.W - 104.0f, Y - 12.0f, 86.0f, 24.0f}, "SHUFFLE", true))
	{
		++U.RepackShuffle;
	}
	Y += 12.0f;
	for (size_t K = 0; K < U.RepackT.size() && K < 3; ++K)
	{
		const Rect Tr{R.X + 12.0f, Y, R.W - 24.0f, 28.0f};
		const Ui::ClickState Cs = F.UI.Clickable("rrreptitle" + std::to_string(K), Tr, F.Live);
		F.C.FillRoundRect(Tr, 6.0f, Cs.Hover ? Mix(RrPanel2, RrGold, 0.08f) : RrPanel2);
		F.C.StrokeRoundRect(Tr, 6.0f, RrLine, 1.0f);
		F.UI.Text("\xE2\x80\x9C" + U.RepackT[K].Text + "\xE2\x80\x9D", Tr.X + 10.0f, Tr.Y + 19.0f, Ts(12.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Tr.W - 20.0f));
		if (Cs.Clicked)
		{
			video::Repack Rp;
			Rp.TitleKey = U.RepackT[K].Key;
			Press(Rp);
		}
		Y += 32.0f;
	}
	if (!U.RepackS.empty())
	{
		F.UI.Text("THUMBNAIL", R.X + 18.0f, Y + 14.0f, Ts(10.5f, 800, RrMuted));
		Y += 22.0f;
		const float Tw = 104.0f, Th = 58.5f, X0 = R.X + (R.W - 3.0f * Tw - 16.0f) * 0.5f; // the Duel card's 104 x 58.5
		for (size_t K = 0; K < U.RepackS.size() && K < 6; ++K)
		{
			const Rect O{X0 + static_cast<float>(K % 3) * (Tw + 8.0f), Y + static_cast<float>(K / 3) * (Th + 8.0f), Tw, Th};
			const Ui::ClickState Cs = F.UI.Clickable("rrrepthumb" + std::to_string(K), O, F.Live);
			F.C.FillRoundRect(O, 6.0f, RrGround);
			RrSpecThumb(F, O, U.RepackS[K], V.Th);
			F.C.StrokeRoundRect(O, 6.0f, Cs.Hover ? RrGold : RrLine, Cs.Hover ? 2.0f : 1.0f);
			if (Cs.Clicked)
			{
				video::Repack Rp;
				Rp.Spec = U.RepackS[K];
				Press(Rp);
			}
		}
		Y += 2.0f * (Th + 8.0f);
	}
	NetParagraph(F.C, U.RepackSaid.empty() ? std::string(RrRepackHelp) : U.RepackSaid, R.X + 18.0f, Y + 16.0f, R.W - 36.0f, 11.5f, 600, U.RepackSaid.empty() ? RrMuted : RrGold,
		15.0f, 2);
}

void RrOneOverview(RrFrame& F, const video::Video& V, const video::VideoStats& St)
{
	using namespace rlnet_detail;
	const video::Channel& Ch = F.S.Life.Video;
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	RrVerdictBlock(F, V, St, {396.0f, 296.0f, 764.0f, 112.0f});
	int Rank = 0, Of = 0;
	double Typical = 0.0;
	RrRankAtAge(F, V, Rank, Of, Typical);
	const bool Cut = V.F == video::Format::Short;
	const double Ratio = RrLikeRatio(St.Likes, St.Dislikes);
	std::vector<RrTileSpec> Tiles(6);
	Tiles[0] = {Cut ? "SHOWN IN FEED" : "IMPRESSIONS", RrShort(St.Impressions), "", St.Estimated ? "est." : "reported", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[1] = {Cut ? "STAYED TO WATCH" : "CTR 48 H", RrPct(St.Ctr48), "", St.Estimated ? "est." : "lifetime " + RrPct(St.CtrLife), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[2] = {"VIEWS", RrCount(St.Views), "", Of > 1 ? "typical " + RrCount(Typical) : std::string(), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[3] = {"AVD", RrClock(St.AvdMin), "", RrPct(St.AvgPct, 0) + " viewed", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[4] = {"LIKES \xC2\xB7 DISLIKES", RrCount(St.Likes) + " \xC2\xB7 " + RrCount(St.Dislikes), "", RrPct(Ratio) + " liked", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	// Before Partner the tile reads what it would have earned, not paid (§5.4): the row gives it the room.
	Tiles[5] = {Partner ? "REVENUE" : "WOULD HAVE EARNED", RrCents(static_cast<double>(Partner ? St.EarnedCents : St.WouldEarnCents)), "",
		Partner ? "RPM " + Money(static_cast<Chips>(std::llround(St.Rpm * 100.0))) : std::string("not paid"), "", 0, RrCash, {}, Color{0, 0, 0, 0}, false};
	RrTiles(F, {396.0f, 418.0f, 764.0f, 78.0f}, Tiles, 20.0f, std::string(), nullptr, {0.9f, 0.85f, 0.9f, 0.85f, 1.1f, 1.4f});
	const Rect Cr{396.0f, 506.0f, 764.0f, 236.0f};
	RrCard(F, Cr, "VIEWS A DAY SINCE IT WENT UP", "dashed: your typical video at the same age");
	RrDailyChart(F, V, {Cr.X + 10.0f, Cr.Y + 40.0f, Cr.W - 20.0f, Cr.H - 50.0f}, false);
	RrLogCard(F, St, {396.0f, 752.0f, 400.0f, 198.0f});
	RrInsightsCard(F, St, {806.0f, 752.0f, 354.0f, 198.0f});
	// The right: Channel health (the moving meter, the streak) and your next upload (the carry, Live Drop's hint, an idea).
	RrHealthCard(F, {1176.0f, 120.0f, 404.0f, 240.0f});
	// C3 gate: a video still inside its 28 days (and its two changes) takes the column's top for CHANGE THE PACKAGING.
	const bool Repack = RrCanRepack(F, V);
	if (Repack)
	{
		RrRepackCard(F, V, St, {1176.0f, 372.0f, 404.0f, 380.0f});
	}
	const Rect Nx = Repack ? Rect{1176.0f, 764.0f, 404.0f, 186.0f} : Rect{1176.0f, 372.0f, 404.0f, 578.0f};
	RrCard(F, Nx, "YOUR NEXT UPLOAD", "Studio");
	float Y = Nx.Y + 40.0f;
	const double NextCarry = RrCarryAt(Ch, F.World);
	if (NextCarry > 1.1)
	{
		F.C.FillRoundRect({Nx.X + 12.0f, Y, Nx.W - 24.0f, 52.0f}, 8.0f, RrGold);
		NetParagraph(F.C, video::valgo::LaunchCarry(NextCarry), Nx.X + 26.0f, Y + 21.0f, Nx.W - 52.0f, 14.0f, 900, Hex(videoart::Ink), 18.0f, 2);
		Y += 64.0f;
	}
	F.UI.Text(video::valgo::LiveDropHint(), Nx.X + 18.0f, Y + 16.0f, Ts(12.5f, 700, RrCream));
	if (RrButton(F, "rronestudio", {Nx.X + 18.0f, Y + 32.0f, 176.0f, 44.0f}, "OPEN STUDIO", true))
	{
		F.U.Page = RrPageStudio;
		F.U.PageAt = F.Now;
		F.U.LaunchHidden = true;
	}
	Y += 96.0f;
	// R5's card and the library's ready ideas (Road to the Final Table among them when a deep run on stream made one).
	const std::vector<video::IdeaCard>& Ideas = RrIdeas(F);
	int Shown = 0;
	for (const video::IdeaCard& I : Ideas)
	{
		if (!I.Ready || Shown >= 3 || Y + 92.0f > Nx.Y + Nx.H - 10.0f)
		{
			continue;
		}
		if (Shown == 0)
		{
			F.UI.Text("IDEAS", Nx.X + 18.0f, Y + 6.0f, Ts(10.5f, 800, RrMuted));
			Y += 16.0f;
		}
		const Rect Ir{Nx.X + 12.0f, Y, Nx.W - 24.0f, 84.0f};
		const Ui::ClickState Cs = F.UI.Clickable("rroneidea" + std::to_string(I.Id), Ir, F.Live);
		F.C.FillRoundRect(Ir, 10.0f, Cs.Hover ? Mix(RrPanel2, RrGold, 0.06f) : RrPanel2);
		F.C.StrokeRoundRect(Ir, 10.0f, RrLine, 1.0f);
		F.UI.Text(I.Name, Ir.X + 14.0f, Ir.Y + 26.0f, Ts(14.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, Ir.W - 90.0f));
		F.UI.Text(I.Fact, Ir.X + 14.0f, Ir.Y + 46.0f, Ts(12.5f, 800, RrGold, Align::Left, Baseline::Alphabetic, false, Ir.W - 28.0f));
		F.UI.Text(I.Hint.empty() ? std::string("Studio cuts it from your career.") : I.Hint, Ir.X + 14.0f, Ir.Y + 66.0f, Ts(11.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Ir.W - 28.0f));
		if (I.Trending)
		{
			RrTag(F.C, "TRENDING", Ir.X + Ir.W - 84.0f, Ir.Y + 10.0f, RrGold, true, 9.0f);
		}
		if (Cs.Clicked)
		{
			F.U.Page = RrPageStudio;
			F.U.ListTab = 1;
			F.U.OpenTab = 1; // the visit opens on IDEAS (L9's rule would pick it only while the idea is new)
			F.U.LaunchHidden = true;
		}
		Y += 92.0f;
		++Shown;
	}
}

void RrOneReach(RrFrame& F, const video::Video& V, const video::VideoStats& St)
{
	using namespace rlnet_detail;
	const bool Cut = V.F == video::Format::Short;
	const chart::Theme& T = RrTheme();
	std::vector<RrTileSpec> Tiles(5);
	const std::string Est = St.Estimated ? "est." : std::string();
	Tiles[0] = {Cut ? "SHOWN IN FEED" : "IMPRESSIONS", RrShort(St.Impressions), "", St.Estimated ? Est : std::string("reported"), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[1] = {Cut ? "STAYED TO WATCH" : "CTR 48 H", RrPct(St.Ctr48), "", St.Estimated ? Est : std::string("the packaging's"), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	// "colder viewers later" only when it fell (Next up and search click more than Home: a pushed video's lifetime CTR can rise).
	Tiles[2] = {"CTR LIFETIME", RrPct(St.CtrLife), "", St.Estimated ? Est : St.CtrLife < St.Ctr48 ? std::string("colder viewers later") : std::string("since it went up"), "", 0,
		Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[3] = {"VIEWS", RrCount(St.Views), "", RrCount(St.Unique) + " unique", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[4] = {"AVD", RrClock(St.AvdMin), "", RrPct(St.AvgPct, 0) + " viewed", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	RrTiles(F, {396.0f, 296.0f, 764.0f, 78.0f}, Tiles, 20.0f);
	// The funnel: impressions, views, past the hook (0:30), past half, liked, subscribed.
	const Rect Fr{396.0f, 386.0f, 376.0f, 250.0f};
	RrCard(F, Fr, "HOW VIEWERS GOT THERE");
	const size_t Hook = static_cast<size_t>(std::clamp(static_cast<int>(std::lround(0.5 / std::max(0.5, V.LengthMin) * 60.0)), 1, 60));
	const double PastHook = St.Views * static_cast<double>(St.Retention[Hook]);
	const double PastHalf = St.Views * static_cast<double>(St.Retention[30]);
	std::vector<chart::Stage> Stages = {{Cut ? "Shown in feed" : "Impressions", St.Impressions, RrCount(St.Impressions)}, {"Views", St.Views, RrCount(St.Views)},
		{"Past " + RrClock(std::min(0.5, V.LengthMin)), PastHook, RrCount(PastHook)}, {"Past half", PastHalf, RrCount(PastHalf)}, {"Liked", St.Likes, RrCount(St.Likes)},
		{"Subscribed", St.SubsNet, RrCount(St.SubsNet)}};
	chart::Funnel(F.C, {Fr.X + 18.0f, Fr.Y + 40.0f, Fr.W - 36.0f, Fr.H - 52.0f}, Stages, T, Color{0, 0, 0, 0}, true);
	// Traffic sources: the donut, each source's share and its own CTR where it has impressions.
	const Rect Dr{784.0f, 386.0f, 376.0f, 250.0f};
	RrCard(F, Dr, "TRAFFIC SOURCES");
	double Total = 0.0;
	for (const double X : St.Src.Views)
	{
		Total += X;
	}
	std::vector<chart::Slice> Slices;
	std::vector<std::pair<double, int>> Order;
	for (int K = 0; K < 10; ++K)
	{
		Order.push_back({St.Src.Views[static_cast<size_t>(K)], K});
	}
	std::stable_sort(Order.begin(), Order.end(), [](const std::pair<double, int>& A, const std::pair<double, int>& B) { return A.first > B.first; });
	for (const auto& P : Order)
	{
		if (P.first > 0.0)
		{
			Slices.push_back({P.first, T.Series[static_cast<size_t>(P.second % 6)], RrSourceNames[P.second]});
		}
	}
	const Vec2 Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	const int Hot = chart::Donut(F.C, Dr.X + 92.0f, Dr.Y + 140.0f, 70.0f, 18.0f, Slices, T, Total > 0.0 && !Order.empty() ? RrPct(Order.front().first / Total, 0) : std::string(),
		Total > 0.0 && !Order.empty() ? RrSourceNames[Order.front().second] : std::string(), Mouse);
	for (size_t K = 0; K < Order.size() && K < 7; ++K)
	{
		if (Order[K].first <= 0.0 || Total <= 0.0)
		{
			break;
		}
		const int Src = Order[K].second;
		const float Yy = Dr.Y + 64.0f + Nf(K) * 24.0f;
		F.C.FillRoundRect({Dr.X + 184.0f, Yy - 9.0f, 9.0f, 9.0f}, 2.0f, T.Series[static_cast<size_t>(Src % 6)]);
		F.UI.Text(RrSourceNames[Src], Dr.X + 198.0f, Yy, Ts(11.5f, static_cast<int>(K) == Hot ? 800 : 600, RrCream, Align::Left, Baseline::Alphabetic, false, 90.0f));
		F.UI.Text(RrPct(Order[K].first / Total, 0), Dr.X + Dr.W - 66.0f, Yy, Ts(11.5f, 700, RrCream, Align::Right));
		const double Impr = St.Src.Impressions[static_cast<size_t>(Src)];
		F.UI.Text(Impr > 0.0 ? RrPct(Order[K].first / Impr) : std::string("\xE2\x80\x94"), Dr.X + Dr.W - 16.0f, Yy, Ts(11.5f, 600, RrMuted, Align::Right));
	}
	// Views and impressions a day.
	const Rect Cr{396.0f, 648.0f, 764.0f, 302.0f};
	RrCard(F, Cr, "VIEWS AND IMPRESSIONS A DAY", "dashed: your typical video's views");
	RrDailyChart(F, V, {Cr.X + 10.0f, Cr.Y + 40.0f, Cr.W - 20.0f, Cr.H - 50.0f}, !Cut && !St.Estimated);
	// The right: the Duel, the log, the Insights.
	float Ry = 120.0f;
	if (V.X.On && V.X.Duels >= 2)
	{
		const Rect Du{1176.0f, Ry, 404.0f, 250.0f};
		RrCard(F, Du, "THUMBNAIL DUEL", St.DuelWin >= 0 ? "won on watch time" : "so far");
		for (int K = 0; K < V.X.Duels && K < 3; ++K)
		{
			const float Yy = Du.Y + 40.0f + Nf(K) * 68.0f;
			const Rect O{Du.X + 18.0f, Yy, 104.0f, 58.5f};
			F.C.FillRoundRect(O, 6.0f, RrGround);
			RrSpecThumb(F, O, V.X.Spec[static_cast<size_t>(K)], V.Th);
			const bool Won = St.DuelWin == K;
			F.C.StrokeRoundRect(O, 6.0f, Won ? RrGold : RrLine, Won ? 2.5f : 1.0f);
			const char Letter[2] = {static_cast<char>('A' + K), '\0'};
			F.UI.Text(std::string(Letter) + (Won ? " won" : ""), O.X + O.W + 14.0f, Yy + 22.0f, Ts(14.0f, 900, Won ? RrGold : RrCream));
			F.UI.Text(RrPct(St.DuelCtr[static_cast<size_t>(K)]) + " CTR \xC2\xB7 " + RrPct(St.DuelShare[static_cast<size_t>(K)], 0) + " of watch time", O.X + O.W + 14.0f, Yy + 42.0f,
				Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Du.W - O.W - 50.0f));
		}
		Ry += 262.0f;
	}
	RrLogCard(F, St, {1176.0f, Ry, 404.0f, 520.0f - (Ry - 120.0f) * 0.5f});
	const float Iy = Ry + 520.0f - (Ry - 120.0f) * 0.5f + 12.0f;
	RrInsightsCard(F, St, {1176.0f, Iy, 404.0f, 950.0f - Iy});
}

void RrOneEngagement(RrFrame& F, const video::Video& V, const video::VideoStats& St)
{
	using namespace rlnet_detail;
	const chart::Theme& T = RrTheme();
	// The typical: AVD and the share viewed from the typical curve's mean (the core's 61 samples).
	double TypMean = 0.0;
	for (int K = 0; K < 60; ++K)
	{
		TypMean += 0.5 * (static_cast<double>(St.Typical[static_cast<size_t>(K)]) + static_cast<double>(St.Typical[static_cast<size_t>(K + 1)])) / 60.0;
	}
	const double Ratio = RrLikeRatio(St.Likes, St.Dislikes);
	std::vector<RrTileSpec> Tiles(5);
	Tiles[0] = {"AVERAGE VIEW", RrClock(St.AvdMin), "", "typical " + RrClock(V.LengthMin * TypMean), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[1] = {"VIEWED", RrPct(St.AvgPct, 0), "", "typical " + RrPct(TypMean, 0), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[2] = {"LIKES \xC2\xB7 DISLIKES", RrCount(St.Likes) + " \xC2\xB7 " + RrCount(St.Dislikes), "", RrPct(Ratio) + " liked", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[3] = {"COMMENTS \xC2\xB7 SHARES", RrCount(St.Comments) + " \xC2\xB7 " + RrCount(St.Shares), "", RrCount(St.Saves) + " saves", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	double SubsNetShown = 0.0, SubsLostShown = 0.0, SubsGainedShown = 0.0;
	RrSubsShown(St.SubsNet, St.SubsLost, SubsNetShown, SubsLostShown, SubsGainedShown);
	Tiles[4] = {"SUBSCRIBERS", (SubsNetShown >= 0.0 ? "+" : "-") + RrCount(std::fabs(SubsNetShown)), "", RrCount(SubsGainedShown) + " gained \xC2\xB7 " + RrCount(SubsLostShown) + " lost", "", 0,
		Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	RrTiles(F, {396.0f, 296.0f, 764.0f, 78.0f}, Tiles, 20.0f);
	// The retention curve: this video's against the typical (dashed), its moments named, the biggest dip.
	const Rect Cr{396.0f, 386.0f, 764.0f, 360.0f};
	RrCard(F, Cr, "AUDIENCE RETENTION", "dashed: your typical");
	chart::Series Rs;
	Rs.Y.assign(St.Retention.begin(), St.Retention.end());
	Rs.Col = RrSky;
	Rs.Area = 0.12f;
	Rs.Width = 2.5f;
	Rs.Name = "This video";
	chart::Series Ty;
	Ty.Y.assign(St.Typical.begin(), St.Typical.end());
	Ty.Col = NetA(RrMuted, 0.8f);
	Ty.Dash = 4.0f;
	Ty.Gap = 4.0f;
	Ty.Width = 1.5f;
	Ty.Name = "Typical";
	chart::LineOpts O;
	O.Y = chart::Units::Percent;
	O.YMin = 0.0;
	O.YMax = 1.0;
	O.X.Labels.assign(61, "");
	for (int K = 0; K <= 60; K += 15)
	{
		O.X.Labels[static_cast<size_t>(K)] = RrClock(V.LengthMin * static_cast<double>(K) / 60.0);
	}
	O.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	std::vector<chart::Mark> Marks;
	std::string Moments;
	for (size_t K = 0; K < St.Moments.size(); ++K)
	{
		const int At = std::clamp(static_cast<int>(std::lround(St.Moments[K].first * 60.0f)), 0, 60);
		Marks.push_back({At, std::to_string(K + 1), false, Color{0, 0, 0, 0}});
		Moments += (Moments.empty() ? std::string() : std::string("   ")) + "(" + std::to_string(K + 1) + ") " + RrClock(V.LengthMin * St.Moments[K].first) + " " + St.Moments[K].second;
	}
	const Rect Plot{Cr.X + 10.0f, Cr.Y + 40.0f, Cr.W - 20.0f, Cr.H - 78.0f};
	const chart::LineOut Lo = chart::LineChart(F.C, Plot, {Ty, Rs}, O, T, nullptr, &Marks);
	// The biggest dip is the intro's, the start to the hook at 0:30 (the curve's own drop): named with what's left.
	const size_t Hook = static_cast<size_t>(std::clamp(static_cast<int>(std::lround(0.5 / std::max(0.5, V.LengthMin) * 60.0)), 1, 60));
	F.UI.Text("Dip: 0:00-" + RrClock(std::min(0.5, V.LengthMin)) + " intro, " + RrPct(St.Retention[Hook], 0) + " left", Cr.X + 18.0f, Cr.Y + Cr.H - 30.0f, Ts(11.5f, 700, RrWarn));
	F.UI.Text("Moments that held them: " + Moments, Cr.X + 18.0f, Cr.Y + Cr.H - 12.0f, Ts(11.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Cr.W - 36.0f));
	if (Lo.Hover >= 0 && Lo.Hover <= 60)
	{
		chart::Tooltip(F.C, Lo.HoverAt, RrClock(V.LengthMin * static_cast<double>(Lo.Hover) / 60.0),
			{{"This video", RrPct(St.Retention[static_cast<size_t>(Lo.Hover)], 0), RrSky, Color{0, 0, 0, 0}}, {"Typical", RrPct(St.Typical[static_cast<size_t>(Lo.Hover)], 0), NetA(RrMuted, 0.8f), Color{0, 0, 0, 0}}},
			T, Cr);
	}
	// Likes against dislikes, with the typical ratio (the last uploads' own rates) and the privacy note.
	const Rect Lk{396.0f, 758.0f, 764.0f, 192.0f};
	RrCard(F, Lk, "LIKES AND DISLIKES", RrDislikesPrivate);
	std::vector<double> Ratios;
	for (const video::Video* O2 : RrUploads(F.S))
	{
		if (O2 != &V && Ratios.size() < 10)
		{
			const video::VideoStats& Os = RrStats(F, *O2);
			if (std::floor(Os.Likes) + std::floor(Os.Dislikes) > 0.0)
			{
				Ratios.push_back(RrLikeRatio(Os.Likes, Os.Dislikes));
			}
		}
	}
	const double Typical = Ratios.empty() ? -1.0 : RrMid(Ratios, 0.5);
	chart::DrawIcon(F.C, chart::Icon::ThumbUp, Lk.X + 30.0f, Lk.Y + 66.0f, 22.0f, RrGood);
	F.UI.Text(RrCount(St.Likes), Lk.X + 48.0f, Lk.Y + 74.0f, Ts(20.0f, 800, RrCream));
	chart::DrawIcon(F.C, chart::Icon::ThumbDown, Lk.X + 160.0f, Lk.Y + 68.0f, 22.0f, RrBad);
	F.UI.Text(RrCount(St.Dislikes), Lk.X + 178.0f, Lk.Y + 74.0f, Ts(20.0f, 800, RrCream));
	F.UI.Text(RrPct(Ratio) + " liked", Lk.X + Lk.W - 18.0f, Lk.Y + 74.0f, Ts(18.0f, 800, Ratio >= 0.96 ? RrGood : RrWarn, Align::Right));
	chart::RatioBar(F.C, {Lk.X + 18.0f, Lk.Y + 92.0f, Lk.W - 36.0f, 14.0f}, std::floor(St.Likes), std::floor(St.Dislikes), T, Typical, RrGood, RrBad);
	F.UI.Text(Typical >= 0.0 ? "typical " + RrPct(Typical) + " \xC2\xB7 " + RrDislikesPrivate : std::string(RrDislikesPrivate), Lk.X + 18.0f, Lk.Y + 130.0f, Ts(11.5f, 600, RrDim));
	F.UI.Text(RrCount(SubsGainedShown) + " subscribed \xC2\xB7 " + RrCount(SubsLostShown) + " unsubscribed after watching", Lk.X + 18.0f, Lk.Y + 156.0f, Ts(12.0f, 600, RrMuted));
	// The comments (generated and read-only): the most liked of what's arrived.
	const Rect Cm{1176.0f, 120.0f, 404.0f, 830.0f};
	RrCard(F, Cm, "COMMENTS", RrCount(St.Comments));
	const int Count = static_cast<int>(std::floor(St.Comments));
	std::vector<video::Comment> All = Count > 0 ? video::CommentsOf(V, F.World, 0, std::min(Count, 20)) : std::vector<video::Comment>();
	std::stable_sort(All.begin(), All.end(), [](const video::Comment& A, const video::Comment& B) { return A.Likes > B.Likes; });
	float Y = Cm.Y + 52.0f;
	if (All.empty())
	{
		F.UI.Text("Comments arrive with the views.", Cm.X + 18.0f, Y, Ts(12.5f, 600, RrMuted));
	}
	for (const video::Comment& C : All)
	{
		if (Y > Cm.Y + Cm.H - 40.0f)
		{
			break;
		}
		F.UI.Text(C.Author, Cm.X + 18.0f, Y, Ts(12.0f, 800, RrGold, Align::Left, Baseline::Alphabetic, false, 240.0f));
		F.UI.Text(RrCount(C.Likes), Cm.X + Cm.W - 18.0f, Y, Ts(11.5f, 700, RrMuted, Align::Right));
		Y = NetParagraph(F.C, C.Text, Cm.X + 18.0f, Y + 18.0f, Cm.W - 36.0f, 12.5f, 600, RrCream, 17.0f, 3) + 14.0f;
	}
}

std::vector<double> RrRpmADay(const video::Channel& Ch, const video::Video& V, int Today, int Days, double World)
{
	// Each day at what its views paid: past the top-earner rate the day's share of the full rate (review-c3f-algo's F2, its
	// valgo::DayPaidShare; today's is the share StatsOf's RPM tile reads); a month under it, and before Partner, the full rate.
	const bool Pays = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= World && Ch.X.On;
	std::vector<double> Rpm;
	for (int D = Today - Days + 1; D <= Today; ++D)
	{
		Rpm.push_back(video::algo::MoneyOf(V, D).Rpm * (Pays ? video::valgo::DayPaidShare(Ch, D) : 1.0) * 100.0);
	}
	return Rpm;
}

void RrOneRevenue(RrFrame& F, const video::Video& V, const video::VideoStats& St)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	const bool Cut = V.F == video::Format::Short;
	const int Today = net::DayOf(F.World);
	std::vector<RrTileSpec> Tiles(5);
	Tiles[0] = {Partner ? "EST. REVENUE" : "WOULD HAVE EARNED", RrCents(static_cast<double>(Partner ? St.EarnedCents : St.WouldEarnCents)), "", Partner ? std::string("since it went up") : std::string("not paid"),
		"", 0, RrCash, {}, Color{0, 0, 0, 0}, false};
	Tiles[1] = {"RPM", Money(static_cast<Chips>(std::llround(St.Rpm * 100.0))), "", "per 1,000 views today", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[2] = {"CPM", Money(static_cast<Chips>(std::llround(St.Cpm * 100.0))), "", "per 1,000 ad views", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[3] = {"PLAYBACK CPM", Money(static_cast<Chips>(std::llround(St.PlaybackCpm * 100.0))), "", "per 1,000 plays with an ad", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[4] = {"MONETIZED PLAYS", RrCount(St.Monetized), "", RrCount(St.AdImpressions) + " ad views", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	RrTiles(F, {396.0f, 296.0f, 764.0f, 78.0f}, Tiles, 20.0f);
	if (!Partner && St.WouldEarnCents > 0)
	{
		F.UI.Text(video::valgo::WouldEarnLine(St.WouldEarnCents), 396.0f, 400.0f, Ts(13.0f, 800, RrCash, Align::Left, Baseline::Alphabetic, false, 764.0f));
	}
	// The ads by position (§5.6): mid-rolls from the video's own ads per playback; the rest pre- and post-roll 9 : 1.
	const Rect Ad{396.0f, 418.0f, 764.0f, 150.0f};
	RrCard(F, Ad, "AD TYPES", Cut ? std::string("Cuts share the feed's ads") : std::string());
	if (!Cut)
	{
		const video::AdLoad Load = V.X.On ? video::valgo::LoadOn(V.X, Today) : video::AdLoad::Standard;
		const double App = video::algo::AdsPerPlayback(V.F, V.X.On ? V.X.Len : video::LengthOpt::Standard, Load);
		const double Mid = App > 0.0 ? (App - 1.0) / App : 0.0;
		const double Pre = (1.0 - Mid) * 0.9, Post = (1.0 - Mid) * 0.1;
		const double Parts[3] = {Pre, Mid, Post};
		const char* Names[3] = {"pre-roll", "mid-roll", "post-roll"};
		const Color Cols[3] = {RrGold, RrSky, RrMuted};
		float Bx = Ad.X + 18.0f;
		const float Bw = Ad.W - 36.0f;
		for (int K = 0; K < 3; ++K)
		{
			const float W = Bw * Nf(Parts[K]);
			if (W > 0.5f)
			{
				F.C.FillRect({Bx, Ad.Y + 48.0f, std::max(0.0f, W - 2.0f), 16.0f}, Cols[K]);
			}
			F.C.FillRoundRect({Ad.X + 18.0f + Nf(K) * 160.0f, Ad.Y + 82.0f, 9.0f, 9.0f}, 2.0f, Cols[K]);
			F.UI.Text(std::string(Names[K]) + " " + RrPct(Parts[K], 0), Ad.X + 32.0f + Nf(K) * 160.0f, Ad.Y + 91.0f, Ts(12.0f, 700, RrCream));
			Bx += W;
		}
		F.UI.Text("CPM " + Money(static_cast<Chips>(std::llround(St.Cpm * 100.0))) + " \xC2\xB7 " + Fixed(App, 2) + " ads per playback \xC2\xB7 skippable 84% \xC2\xB7 6-second 16%", Ad.X + 18.0f,
			Ad.Y + 124.0f, Ts(12.0f, 600, RrMuted));
	}
	// The coin and the ad load, with its weekly change (§4.5.5: effective the next midnight, refused on Update 2 videos).
	const Rect Ld{396.0f, 580.0f, 764.0f, 110.0f};
	RrCard(F, Ld, "SUITABILITY AND ADS");
	const Color CoinCol = St.Coin == 1 ? Hex(0xf2a93b) : St.Coin == 2 ? RrBad : RrGood;
	F.C.FillCircle(Ld.X + 26.0f, Ld.Y + 58.0f, 7.0f, CoinCol);
	F.UI.Text(Cut ? std::string("Feed ads") : St.Coin == 1 ? std::string("Limited ads") : St.Coin == 2 ? std::string("No ads") : std::string("Full ads"), Ld.X + 40.0f, Ld.Y + 63.0f, Ts(14.0f, 800, RrCream));
	if (!Cut && V.X.On)
	{
		const char* Loads[3] = {"Light", "Standard", "Heavy"};
		for (int K = 0; K < 3; ++K)
		{
			const Rect B{Ld.X + Ld.W - 18.0f - Nf(3 - K) * 98.0f + 6.0f, Ld.Y + 42.0f, 92.0f, 30.0f};
			if (RrSegment(F, std::string("rroneads") + Loads[K], B, Loads[K], std::string(), V.X.Ads == static_cast<video::AdLoad>(K)) && V.X.Ads != static_cast<video::AdLoad>(K))
			{
				const std::string Why = F.S.SetAdLoad(V.Id, static_cast<video::AdLoad>(K));
				U.Note = Why.empty() ? std::string("Ads on this video: ") + Loads[K] + ", from the next midnight." : Why;
				U.NoteBad = !Why.empty();
				U.NoteAt = F.Now;
			}
		}
		const bool Pending = V.X.Ads != V.X.AdsAtUpload;
		const int Next = V.X.AdsChangedDay >= 0 ? V.X.AdsChangedDay + 6 : -1;
		F.UI.Text(Pending ? std::string("Changes at the next midnight") : Next > Today ? "Next change from " + net::DateLabel(Next) : std::string(RrChangeOnce), Ld.X + 18.0f, Ld.Y + 94.0f,
			Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Ld.W - 36.0f));
	}
	else if (!V.X.On)
	{
		F.UI.Text("This video was published before ad settings.", Ld.X + 18.0f, Ld.Y + 94.0f, Ts(12.0f, 600, RrMuted));
	}
	// RPM a day since it went up (the season and any switch move it; the top-earner rate's share, RrRpmADay).
	const Rect Hr{396.0f, 702.0f, 764.0f, 248.0f};
	RrCard(F, Hr, "RPM A DAY");
	const int From = net::DayOf(V.At);
	const int Days = std::clamp(Today - From + 1, 1, 90);
	const std::vector<double> Rpm = RrRpmADay(Ch, V, Today, Days, F.World);
	chart::Series Sv;
	Sv.Y = Rpm;
	Sv.Col = RrCash;
	Sv.Width = 2.5f;
	chart::LineOpts O;
	O.Y = chart::Units::Money;
	O.Zero = false;
	O.X.FirstDay = Today - Days + 1;
	O.X.Count = 6;
	chart::LineChart(F.C, {Hr.X + 10.0f, Hr.Y + 40.0f, Hr.W - 20.0f, Hr.H - 50.0f}, {Sv}, O, RrTheme());
	RrInsightsCard(F, St, {1176.0f, 120.0f, 404.0f, 300.0f});
}

void RrOneVideo(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const std::vector<const video::Video*> List = RrUploads(F.S);
	const video::Video* Vp = RrVideo(F.S, U.One);
	if (!Vp)
	{
		U.One.clear();
		return;
	}
	const video::Video& V = *Vp;
	if (RrLink(F, "rrallcontent", "\xE2\x80\xB9 All content", 1160.0f, 95.0f, 13.0f, RrMuted, Align::Right))
	{
		U.One.clear();
		return;
	}
	RrOneList(F, List);
	const video::VideoStats& St = RrStats(F, V);
	// The header: the thumbnail, the title, what it is and when, its tags, the four tabs.
	const Rect Th{396.0f, 124.0f, 256.0f, 144.0f};
	F.C.FillRoundRect(Th, 8.0f, RrGround);
	RrThumb(F, Th, V);
	F.C.StrokeRoundRect(Th, 8.0f, RrLine, 1.0f);
	RrLengthBadge(F, Th, V);
	const float Tx = Th.X + Th.W + 20.0f;
	NetParagraph(F.C, V.Title, Tx, 150.0f, 1160.0f - Tx, 22.0f, 800, RrCream, 27.0f, 2);
	const std::string Ads = V.F == video::Format::Short ? std::string("feed ads") : V.X.On ? std::string(V.X.Ads == video::AdLoad::Light ? "Light" : V.X.Ads == video::AdLoad::Heavy ? "Heavy" : "Standard") + " ads" : std::string("Standard ads");
	F.UI.Text(std::string(RrFormatName(V.F)) + " " + RrClock(V.LengthMin) + " \xC2\xB7 " + Ads + " \xC2\xB7 published " + net::WeekdayName(net::DayOf(V.At)) + " " + net::TimeLabel(V.At) + ", " + RrAge(F.World - V.At),
		Tx, 212.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, 1160.0f - Tx));
	float Gx = Tx;
	if (St.Tag > 0)
	{
		Gx += RrTag(F.C, NetUpper(video::valgo::TagName(St.Tag)), Gx, 222.0f, St.Tag == 1 ? RrSky : St.Tag == 2 ? RrGold : RrCream, St.Tag >= 2, 10.5f) + 8.0f;
	}
	if (St.LateBloom)
	{
		Gx += RrTag(F.C, "LATE BLOOMER", Gx, 222.0f, RrGood, false, 10.5f) + 8.0f;
	}
	if (!RrIsGrant(V) && RrHomeTested(V, F.World))
	{
		Gx += RrTag(F.C, "RECOMMENDED ON HOME", Gx, 222.0f, RrCream, false, 10.5f) + 8.0f;
	}
	const char* Tabs[4] = {"Overview", "Reach", "Engagement", "Revenue"};
	float Xx = Tx;
	for (int K = 0; K < 4; ++K)
	{
		const float W = F.UI.Measure(Tabs[K], 13.5f, 800) + 26.0f;
		if (RrChip(F, std::string("rronetab") + Tabs[K], {Xx, 250.0f, W, 28.0f}, Tabs[K], U.OneTab == K))
		{
			U.OneTab = K;
		}
		Xx += W + 6.0f;
	}
	switch (U.OneTab)
	{
	case 1: RrOneReach(F, V, St); break;
	case 2: RrOneEngagement(F, V, St); break;
	case 3: RrOneRevenue(F, V, St); break;
	default: RrOneOverview(F, V, St); break;
	}
}

// ------------------------------------------------------------------ AUDIENCE (§5.5: display only)

void RrAudiencePage(RrFrame& F)
{
	using namespace rlnet_detail;
	RrDaily(F);
	const video::AudienceView& A = F.U.Audience;
	const chart::Theme& T = RrTheme();
	const int Days = RrWindowDays(F);
	const video::Range& Rg = RrRange(F, Days);
	const double Views = RrSum(Rg.Views);
	// Returning and new viewers.
	const Rect Rn{20.0f, 120.0f, 760.0f, 110.0f};
	RrCard(F, Rn, "RETURNING AND NEW VIEWERS");
	F.UI.Text("Returning " + RrPct(A.Returning, 0), Rn.X + 18.0f, Rn.Y + 62.0f, Ts(22.0f, 800, RrCream));
	F.UI.Text("New " + RrPct(1.0 - A.Returning, 0), Rn.X + 230.0f, Rn.Y + 62.0f, Ts(22.0f, 800, RrSky));
	const double Unique = Views / (1.0 + 0.6 * A.Returning);
	F.UI.Text("Unique viewers " + RrCount(Unique) + " \xC2\xB7 views per viewer " + Fixed(1.0 + 0.6 * A.Returning, 2) + " \xC2\xB7 " + RrPct(A.SubsWatchShare, 0) + " of watch time from subscribers",
		Rn.X + 18.0f, Rn.Y + 92.0f, Ts(12.0f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Rn.W - 36.0f));
	// When your viewers are on: the heat map, the best window, the hours your last uploads went up ringed.
	const Rect Hm{796.0f, 120.0f, 784.0f, 330.0f};
	RrCard(F, Hm, "WHEN YOUR VIEWERS ARE ON", "your time");
	std::vector<std::vector<double>> Week;
	int BestD = 0, BestH = 0;
	double Best = -1.0;
	for (int D = 0; D < 7; ++D)
	{
		std::vector<double> Row;
		for (int H = 0; H < 24; ++H)
		{
			const double V = static_cast<double>(A.Heat[static_cast<size_t>(D)][static_cast<size_t>(H)]);
			Row.push_back(V);
			const double Win = V + static_cast<double>(A.Heat[static_cast<size_t>(D)][static_cast<size_t>((H + 1) % 24)]) + static_cast<double>(A.Heat[static_cast<size_t>(D)][static_cast<size_t>((H + 2) % 24)]);
			if (Win > Best)
			{
				Best = Win;
				BestD = D;
				BestH = H;
			}
		}
		Week.push_back(Row);
	}
	chart::HeatOpts Ho;
	Ho.Rows = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	for (int H = 0; H < 24; ++H)
	{
		Ho.Cols.push_back(ClockString(H * 60.0));
	}
	Ho.ColEvery = 4;
	std::vector<double> Hours;
	for (const video::Video* V : RrUploads(F.S))
	{
		if (Ho.Ring.size() >= 10)
		{
			break;
		}
		const int D = ((net::DayOf(V->At) % 7) + 7) % 7;
		const int H = static_cast<int>(std::floor(std::fmod(V->At, net::MinutesPerDay) / 60.0));
		Ho.Ring.push_back({D, H});
		Hours.push_back(static_cast<double>(H));
	}
	Ho.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	chart::Heatmap(F.C, {Hm.X + 14.0f, Hm.Y + 40.0f, Hm.W - 28.0f, Hm.H - 74.0f}, Week, Ho, T);
	auto HourWord = [](int H) {
		const int H12 = H % 12 == 0 ? 12 : H % 12;
		return std::to_string(H12) + (H < 12 ? " AM" : " PM");
	};
	std::string Caption = "Best: " + std::string(net::WeekdayName(BestD)) + " " + std::to_string((BestH % 12 == 0) ? 12 : BestH % 12) + "-" + HourWord((BestH + 3) % 24) + ".";
	if (!Hours.empty())
	{
		Caption += " You post at " + HourWord(static_cast<int>(RrMid(Hours, 0.5))) + ".";
	}
	F.UI.Text(Caption, Hm.X + 18.0f, Hm.Y + Hm.H - 12.0f, Ts(12.5f, 700, RrCream));
	// Ages (18+ only) and devices.
	const Rect Ag{20.0f, 246.0f, 370.0f, 186.0f};
	RrCard(F, Ag, "AGE", RrAgeNote);
	const char* Ages[5] = {"18-24", "25-34", "35-44", "45-54", "55+"};
	for (int K = 0; K < 5; ++K)
	{
		const float Y = Ag.Y + 52.0f + Nf(K) * 26.0f;
		F.UI.Text(Ages[K], Ag.X + 18.0f, Y, Ts(12.0f, 700, RrCream));
		F.C.FillRoundRect({Ag.X + 74.0f, Y - 10.0f, 220.0f, 10.0f}, 3.0f, RrPanel2);
		F.C.FillRoundRect({Ag.X + 74.0f, Y - 10.0f, std::max(4.0f, 220.0f * Nf(A.Ages[static_cast<size_t>(K)] / 0.5)), 10.0f}, 3.0f, RrGold);
		F.UI.Text(RrPct(A.Ages[static_cast<size_t>(K)], 0), Ag.X + Ag.W - 18.0f, Y, Ts(12.0f, 700, RrCream, Align::Right));
	}
	const Rect Dv{406.0f, 246.0f, 374.0f, 186.0f};
	RrCard(F, Dv, "DEVICES");
	const char* Devices[4] = {"Phone", "Computer", "TV", "Tablet"};
	for (int K = 0; K < 4; ++K)
	{
		const float Y = Dv.Y + 52.0f + Nf(K) * 30.0f;
		F.UI.Text(Devices[K], Dv.X + 18.0f, Y, Ts(12.0f, 700, RrCream));
		F.C.FillRoundRect({Dv.X + 100.0f, Y - 10.0f, 200.0f, 10.0f}, 3.0f, RrPanel2);
		F.C.FillRoundRect({Dv.X + 100.0f, Y - 10.0f, std::max(4.0f, 200.0f * Nf(A.Devices[static_cast<size_t>(K)])), 10.0f}, 3.0f, RrSky);
		F.UI.Text(RrPct(A.Devices[static_cast<size_t>(K)], 0), Dv.X + Dv.W - 18.0f, Y, Ts(12.0f, 700, RrCream, Align::Right));
	}
	// The bell.
	const Rect Bl{20.0f, 448.0f, 760.0f, 60.0f};
	RrCard(F, Bl, std::string());
	RrBellIcon(F.C, Bl.X + 30.0f, Bl.Y + 30.0f, 22.0f, RrGold);
	F.UI.Text(RrPct(A.BellShare, 0) + " of subscribers get every alert", Bl.X + 54.0f, Bl.Y + 36.0f, Ts(14.0f, 800, RrCream));
	// Top geographies.
	const Rect Ge{796.0f, 466.0f, 784.0f, 200.0f};
	RrCard(F, Ge, "TOP GEOGRAPHIES");
	for (size_t K = 0; K < A.Geo.size() && K < 8; ++K)
	{
		const float X = Ge.X + 18.0f + Nf(K % 2) * 380.0f, Y = Ge.Y + 58.0f + Nf(K / 2) * 34.0f;
		NetFlag(F.C, A.Geo[K].first, X, Y - 13.0f, 24.0f, 16.0f);
		const hero::CountryInfo* Ci = hero::FindCountry(A.Geo[K].first);
		F.UI.Text(Ci ? std::string(Ci->Name) : A.Geo[K].first, X + 34.0f, Y, Ts(13.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 240.0f));
		F.UI.Text(RrPct(A.Geo[K].second), X + 350.0f, Y, Ts(13.0f, 700, RrMuted, Align::Right));
	}
	// Channels your audience watches: the Kast directory's poker creators.
	const Rect Cw{20.0f, 682.0f, 1560.0f, 120.0f};
	RrCard(F, Cw, "CHANNELS YOUR AUDIENCE WATCHES");
	std::vector<const kast::Streamer*> Names;
	for (const kast::Streamer& K : kast::Directory())
	{
		Names.push_back(&K);
	}
	const std::string Me = F.S.HeroName;
	std::stable_sort(Names.begin(), Names.end(), [&Me](const kast::Streamer* A2, const kast::Streamer* B) { return Fnv1a(Me + "|" + A2->Name) < Fnv1a(Me + "|" + B->Name); });
	for (size_t K = 0; K < Names.size() && K < 8; ++K)
	{
		const float X = Cw.X + 40.0f + Nf(K) * 190.0f;
		NetAvatar(F.C, X, Cw.Y + 70.0f, 20.0f, Names[K]->Name, Hex(Names[K]->Color));
		F.UI.Text(Names[K]->Name, X + 28.0f, Cw.Y + 66.0f, Ts(12.5f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, 150.0f));
		F.UI.Text(RrShort(Names[K]->Followers) + " followers", X + 28.0f, Cw.Y + 84.0f, Ts(11.0f, 600, RrMuted));
	}
	const Rect Sb{20.0f, 524.0f, 760.0f, 142.0f};
	RrCard(F, Sb, "SUBSCRIBERS", std::to_string(Days) + " days");
	const double Gained = RrSum(Rg.SubsGained), Lost = RrSum(Rg.SubsLost);
	double NetShown = 0.0, LostShown = 0.0, GainedShown = 0.0;
	RrSubsShown(Gained - Lost, Lost, NetShown, LostShown, GainedShown);
	F.UI.Text((NetShown >= 0.0 ? "+" : "-") + RrCount(std::fabs(NetShown)), Sb.X + 18.0f, Sb.Y + 70.0f, Ts(28.0f, 800, RrCream));
	F.UI.Text(RrCount(GainedShown) + " gained \xC2\xB7 " + RrCount(LostShown) + " lost", Sb.X + 18.0f, Sb.Y + 96.0f, Ts(12.5f, 600, RrMuted));
	chart::SparkOpts So;
	chart::Sparkline(F.C, {Sb.X + 300.0f, Sb.Y + 44.0f, Sb.W - 320.0f, 80.0f}, RrNet(Rg), RrGood, So, T);
}

// ------------------------------------------------------------------ REVENUE (§5.6, §4.5.8)

void RrAdMarketCard(RrFrame& F, const Rect& R, bool Compact = false)
{
	using namespace rlnet_detail;
	RrDaily(F);
	RrCard(F, R, "THE AD MARKET THIS WEEK", "your last 10 uploads");
	// Compact (Research before Partner, under the search table): the note on one line, the rows closer.
	NetParagraph(F.C, RrMarketNote, R.X + 18.0f, R.Y + 50.0f, R.W - 36.0f, 12.0f, 600, RrMuted, 16.0f, Compact ? 1 : 2);
	const float Pitch = Compact ? 22.0f : 24.0f;
	float Y = R.Y + (Compact ? 78.0f : 90.0f);
	double Top = 0.0;
	for (const video::AdMarketRow& M : F.U.Market)
	{
		Top = std::max(Top, M.Cpm);
	}
	for (const video::AdMarketRow& M : F.U.Market)
	{
		if (Y > R.Y + R.H - 12.0f)
		{
			break;
		}
		F.UI.Text(M.Category, R.X + 18.0f, Y, Ts(12.5f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 170.0f));
		F.C.FillRoundRect({R.X + 196.0f, Y - 10.0f, R.W - 330.0f, 10.0f}, 3.0f, RrPanel2);
		if (Top > 0.0)
		{
			F.C.FillRoundRect({R.X + 196.0f, Y - 10.0f, std::max(4.0f, (R.W - 330.0f) * Nf(M.Cpm / Top)), 10.0f}, 3.0f, RrGold);
		}
		F.UI.Text(Money(static_cast<Chips>(std::llround(M.Cpm * 100.0))), R.X + R.W - 76.0f, Y, Ts(12.5f, 800, RrCash, Align::Right));
		F.UI.Text(M.Mix > 0.0 ? RrPct(M.Mix, 0) : std::string("\xE2\x80\x94"), R.X + R.W - 18.0f, Y, Ts(12.0f, 700, M.Mix > 0.0 ? RrCream : RrDim, Align::Right));
		Y += Pitch;
	}
}

// ------------------------------------------------------------------ the "?" cards (C3 fix round: H1, Research's help)

/** A "?" beside a line (§4.5.1's words, the top-earner rate): a press opens its small card over the page; another press, or a
 *  click off it, closes it. */
void RrHelpDot(RrFrame& F, const std::string& Id, float Cx, float Cy, const std::string& Title, const std::string& Text, float Below = 0.0f)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const Rect R{Cx - 11.0f, Cy - 11.0f, 22.0f, 22.0f};
	const Ui::ClickState St = F.UI.Clickable("rrhelp:" + Id, R, F.Live);
	const bool Open = U.HelpOpen == Id;
	F.C.FillCircle(Cx, Cy, 9.0f, Open ? RrGold : St.Hover ? RrPanel2 : RrGround);
	F.C.StrokeEllipse(Cx, Cy, 9.0f, 9.0f, Open ? RrGold : NetA(RrCream, St.Hover ? 0.9f : 0.55f), 1.3f);
	F.UI.Text("?", Cx, Cy + 0.5f, Ts(12.0f, 900, Open ? Hex(videoart::Ink) : RrCream, Align::Center, Baseline::Middle));
	if (St.Clicked)
	{
		U.HelpOpen = Open ? std::string() : Id;
	}
	if (U.HelpOpen == Id)
	{
		U.HelpSeen = true;
		U.HelpAt = R;
		U.HelpBelow = std::max(Below, R.Y + R.H);
		U.HelpTitle = Title;
		U.HelpText = Text;
	}
}

/** The open "?" card, over the page (drawn after it). A page that no longer draws its "?" closes it; a click off the card and
 *  its "?" closes it too. */
void RrHelpPop(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	if (U.HelpOpen.empty() || !U.HelpSeen)
	{
		U.HelpOpen.clear();
		U.HelpBox = Rect{};
		return;
	}
	U.HelpSeen = false;
	const float W = 400.0f;
	const size_t N = std::min<size_t>(12, std::max<size_t>(1, NetWrap(F.C, U.HelpText, W - 36.0f, 13.0f, 600).size()));
	const float H = 58.0f + 19.0f * static_cast<float>(N) + 6.0f;
	const float X = std::clamp(U.HelpAt.X + U.HelpAt.W - W + 16.0f, 20.0f, NetW - 20.0f - W);
	float Y = U.HelpBelow + 8.0f;
	if (Y + H > VideoH - 12.0f)
	{
		Y = U.HelpAt.Y - 8.0f - H;
	}
	const Rect R{X, Y, W, H};
	U.HelpBox = R;
	F.C.GlowRoundRect(R, 12.0f, Hex(0x000000, 0.55f), 22.0f);
	F.C.FillRoundRect(R, 12.0f, RrPanel2);
	F.C.StrokeRoundRect(R, 12.0f, NetA(RrGold, 0.75f), 1.2f);
	F.UI.Text(U.HelpTitle, X + 18.0f, Y + 28.0f, Ts(11.5f, 800, RrGold, Align::Left, Baseline::Alphabetic, false, W - 36.0f));
	NetParagraph(F.C, U.HelpText, X + 18.0f, Y + 52.0f, W - 36.0f, 13.0f, 600, RrCream, 19.0f, 12);
	if (F.Live && F.UI.Ptr.Pressed && !RrInside(R, F.UI.Ptr.X, F.UI.Ptr.Y) && !RrInside(U.HelpAt, F.UI.Ptr.X, F.UI.Ptr.Y))
	{
		U.HelpOpen.clear();
	}
}

void RrRevenuePage(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	const video::Channel& Ch = F.S.Life.Video;
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	const int Days = RrWindowDays(F);
	const video::Range& Rg = RrRange(F, Days);
	const chart::Theme& T = RrTheme();
	const int Today = net::DayOf(F.World);
	if (!Partner)
	{
		// Before Partner: the gate's two bars, the Ad market card and what the videos would have earned (not paid).
		const Rect Gc{20.0f, 120.0f, 760.0f, 250.0f};
		RrCard(F, Gc, "THE PARTNER PROGRAM", "last 365 days");
		const video::ChannelView V = F.S.VideoView();
		const double Have[2] = {std::floor(V.Subs), std::floor(V.WatchHours365)};
		const double Need[2] = {1000.0, 4000.0};
		const char* Names[2] = {"Subscribers", "Watch hours"};
		for (int K = 0; K < 2; ++K)
		{
			const float Y = Gc.Y + 72.0f + Nf(K) * 54.0f;
			F.UI.Text(Names[K], Gc.X + 18.0f, Y, Ts(15.0f, 700, RrCream));
			F.UI.Text(Grouped(static_cast<int64_t>(Have[K])) + " / " + Grouped(static_cast<int64_t>(Need[K])), Gc.X + Gc.W - 18.0f, Y, Ts(15.0f, 800, Have[K] >= Need[K] ? RrGold : RrCream, Align::Right));
			const Rect Track{Gc.X + 18.0f, Y + 10.0f, Gc.W - 36.0f, 10.0f};
			F.C.FillRoundRect(Track, 5.0f, RrPanel2);
			const float Frac = Nf(std::clamp(Have[K] / Need[K], 0.0, 1.0));
			if (Frac > 0.0f)
			{
				F.C.FillRoundRect({Track.X, Track.Y, std::max(10.0f, Track.W * Frac), Track.H}, 5.0f, RrGold);
			}
		}
		F.UI.Text(Ch.ReviewAt >= 0.0 ? RrUnderReview : RrAdsBeforePartner, Gc.X + 18.0f, Gc.Y + 200.0f, Ts(13.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Gc.W - 36.0f));
		F.UI.Text(RrNotPaid, Gc.X + 18.0f, Gc.Y + 224.0f, Ts(12.0f, 600, RrMuted));
		RrAdMarketCard(F, {796.0f, 120.0f, 784.0f, 300.0f});
		const Rect Wc{20.0f, 386.0f, 760.0f, 564.0f};
		RrCard(F, Wc, "WHAT YOUR VIDEOS WOULD HAVE EARNED", "not paid");
		Chips Total = 0;
		std::vector<std::pair<Chips, const video::Video*>> Rows;
		for (const video::Video* V2 : RrUploads(F.S))
		{
			const Chips C = RrStats(F, *V2).WouldEarnCents;
			Total += C;
			Rows.push_back({C, V2});
		}
		std::stable_sort(Rows.begin(), Rows.end(), [](const std::pair<Chips, const video::Video*>& A, const std::pair<Chips, const video::Video*>& B) { return A.first > B.first; });
		F.UI.Text(Money(Total), Wc.X + 18.0f, Wc.Y + 74.0f, Ts(32.0f, 800, RrCash));
		F.UI.Text("the views so far, at each day's ad prices", Wc.X + 18.0f, Wc.Y + 98.0f, Ts(12.0f, 600, RrMuted));
		for (size_t K = 0; K < Rows.size() && K < 12; ++K)
		{
			const float Y = Wc.Y + 140.0f + Nf(K) * 32.0f;
			F.UI.Text(Rows[K].second->Title, Wc.X + 18.0f, Y, Ts(13.0f, 600, RrCream, Align::Left, Baseline::Alphabetic, false, Wc.W - 160.0f));
			F.UI.Text(Money(Rows[K].first), Wc.X + Wc.W - 18.0f, Y, Ts(13.0f, 800, RrCash, Align::Right));
		}
		// The advertisers' calendar (§4.5.3): the twelve months from this one, each month's middle day's season, this month lit.
		const Rect Sc{796.0f, 436.0f, 784.0f, 514.0f};
		RrCard(F, Sc, "THE SEASON", "a year of ad prices, the average 1");
		NetParagraph(F.C, RrSeasonLine, Sc.X + 18.0f, Sc.Y + 56.0f, Sc.W - 36.0f, 14.0f, 600, RrCream, 20.0f, 2);
		F.UI.Text(RrSeasonWords(F.World), Sc.X + 18.0f, Sc.Y + 86.0f, Ts(13.0f, 800, RrGold));
		int Yr = 0, Mo = 0, Dm = 0;
		world::CivilDate(Today, Yr, Mo, Dm);
		chart::BarSet Sb;
		Sb.Col = Hex(0x5a4a30);
		std::vector<std::string> Months;
		for (int K = 0; K < 12; ++K)
		{
			const int Mid = Today + static_cast<int>(std::lround(30.4 * K)) - (Dm - 15);
			Sb.V.push_back(video::algo::Season(Mid));
			Sb.Each.push_back(K == 0 ? RrGold : Color{0, 0, 0, 0});
			Months.push_back(RrMonthShort[(Mo - 1 + K) % 12]);
		}
		chart::BarOpts Bo;
		Bo.Y = chart::Units::Plain;
		Bo.X.Labels = Months;
		Bo.X.Count = 12;
		Bo.YTicks = 3;
		chart::Bars(F.C, {Sc.X + 10.0f, Sc.Y + 110.0f, Sc.W - 20.0f, Sc.H - 124.0f}, {Sb}, Bo, RrTheme());
		return;
	}
	// While MANAGE's list is up the page under it takes no clicks (the bell's rule).
	const Pointer Real = F.UI.Ptr;
	const bool WasOpen = U.Manage;
	if (U.Manage)
	{
		F.UI.Ptr.Pressed = false;
		F.UI.Ptr.Released = false;
		F.UI.Ptr.Wheel = 0.0f;
	}
	// The tiles: the window's revenue, RPM, playback CPM and monetized plays (each video at its own rates, by its views).
	double W = 0.0, Cpm = 0.0, App = 0.0, Mon = 0.0;
	std::array<double, 3> LoadCount{};
	int Limited = 0;
	const double From = F.World - static_cast<double>(Days) * net::MinutesPerDay;
	Chips Watch = 0, Cuts = 0;
	std::vector<std::pair<Chips, const video::Video*>> Earners;
	for (const video::Video* V : RrUploads(F.S))
	{
		const video::VideoStats& St = RrStats(F, *V);
		(V->F == video::Format::Short ? Cuts : Watch) += St.EarnedCents;
		Earners.push_back({St.EarnedCents, V});
		if (V->F == video::Format::Short)
		{
			continue;
		}
		const double Wt = video::Views(*V, F.World) - video::Views(*V, std::max(V->At, From));
		const video::algo::Money M = video::algo::MoneyOf(*V, Today);
		W += Wt;
		Cpm += Wt * M.Cpm;
		App += Wt * M.AdsPerPlayback;
		Mon += Wt * M.MonetizedRate;
		if (V->X.On)
		{
			LoadCount[static_cast<size_t>(std::clamp(static_cast<int>(V->X.Ads), 0, 2))] += 1.0;
			Limited += V->X.C == video::Coin::Limited ? 1 : 0;
		}
	}
	const double Views = RrSum(Rg.Views), Cents = RrSum(Rg.Cents);
	std::vector<RrTileSpec> Tiles(4);
	Tiles[0] = {"EST. REVENUE", RrCents(Cents), "", RrSeasonWords(F.World), "", 0, RrCash, Rg.Cents, RrCash, false};
	RrDelta(Cents, RrSum(U.Before.Cents), Tiles[0]);
	Tiles[1] = {"RPM", Views > 0.0 ? Money(static_cast<Chips>(std::llround(Cents / Views * 1000.0))) : std::string("$0.00"), "", "per 1,000 views, every view", "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[2] = {"PLAYBACK CPM", W > 0.0 ? Money(static_cast<Chips>(std::llround(Cpm / W * App / W * 100.0))) : std::string("$0.00"), "", W > 0.0 ? "CPM " + Money(static_cast<Chips>(std::llround(Cpm / W * 100.0))) + " \xC2\xB7 " + Fixed(App / W, 2) + " ads per playback" : std::string(),
		"", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	Tiles[3] = {"MONETIZED PLAYS", RrCount(Mon), "", W > 0.0 ? RrPct(Mon / W, 0) + " of long views" : std::string(), "", 0, Color{0, 0, 0, 0}, {}, Color{0, 0, 0, 0}, false};
	RrTiles(F, {20.0f, 120.0f, NetW - 40.0f, 110.0f}, Tiles, 34.0f);
	// Revenue a day, the season shaded behind.
	const Rect Rc{20.0f, 246.0f, 1120.0f, 320.0f};
	RrCard(F, Rc, "REVENUE A DAY", RrSeasonLine);
	const Rect Plot{Rc.X + 10.0f, Rc.Y + 40.0f, Rc.W - 20.0f, Rc.H - 50.0f};
	{
		// The season behind: each day's value as a faint column, its height against the year's average of 1.
		const float Sw = Plot.W / Nf(std::max<size_t>(1, Rg.Cents.size()));
		for (size_t K = 0; K < Rg.Cents.size(); ++K)
		{
			const double S = video::algo::Season(Today - static_cast<int>(Rg.Cents.size()) + 1 + static_cast<int>(K));
			const float H = Nf(std::clamp(S / 1.6, 0.0, 1.0)) * (Plot.H - 30.0f);
			F.C.FillRect({Plot.X + Nf(K) * Sw, Plot.Y + Plot.H - 20.0f - H, Sw + 0.5f, H}, NetA(RrGold, 0.05f));
		}
	}
	chart::BarSet Cb;
	Cb.V = Rg.Cents;
	Cb.Col = RrCash;
	chart::BarOpts Bo;
	Bo.Y = chart::Units::Money;
	Bo.X.FirstDay = Today - static_cast<int>(Rg.Cents.size()) + 1;
	Bo.X.Count = 8;
	Bo.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	const chart::BarOut Out = chart::Bars(F.C, Plot, {Cb}, Bo, T);
	if (Out.Hover >= 0 && Out.Hover < static_cast<int>(Rg.Cents.size()))
	{
		chart::Tooltip(F.C, {Out.HoverSlot.X + Out.HoverSlot.W / 2.0f, Out.HoverSlot.Y}, chart::AxisDate(Bo.X.FirstDay + Out.Hover),
			{{"Revenue", RrCents(Rg.Cents[static_cast<size_t>(Out.Hover)]), RrCash, Color{0, 0, 0, 0}}}, T, Rc);
	}
	// How you earned: watch-page ads and the Cuts feed (kept videos, each since it went up).
	const Rect Hw{1156.0f, 246.0f, 424.0f, 320.0f};
	RrCard(F, Hw, "HOW YOU EARNED", "Kept videos, since each went up");
	F.UI.Text("Watch-page ads", Hw.X + 18.0f, Hw.Y + 72.0f, Ts(14.0f, 700, RrCream));
	F.UI.Text(Money(Watch), Hw.X + Hw.W - 18.0f, Hw.Y + 72.0f, Ts(16.0f, 800, RrCash, Align::Right));
	F.UI.Text("Cuts feed", Hw.X + 18.0f, Hw.Y + 104.0f, Ts(14.0f, 700, RrCream));
	F.UI.Text(Money(Cuts), Hw.X + Hw.W - 18.0f, Hw.Y + 104.0f, Ts(16.0f, 800, RrCash, Align::Right));
	chart::RatioBar(F.C, {Hw.X + 18.0f, Hw.Y + 122.0f, Hw.W - 36.0f, 12.0f}, static_cast<double>(Watch), static_cast<double>(Cuts), T, -1.0, RrCash, RrSky);
	// The top-earner rate (the user's soft ceiling, C3_AUDIT H1): when this pay month has passed it, its line in Rerun's words,
	// what the next views pay against the full rate (or what it held back), and its "?" card. Every amount is the core's.
	const video::valgo::TopRateView Top = RrTopRate(F);
	if (Top.Applies)
	{
		// A tinted block under the split: the core's line (it names the rate) beside its "?", then what it means right now.
		const std::string Line = video::valgo::TopRateLine();
		const float Lines = Nf(std::min<size_t>(3, std::max<size_t>(1, NetWrap(F.C, Line, Hw.W - 80.0f, 13.0f, 700).size())));
		const Rect Box{Hw.X + 12.0f, Hw.Y + 146.0f, Hw.W - 24.0f, 25.0f + 18.0f * (Lines - 1.0f) + 36.0f};
		F.C.FillRoundRect(Box, 8.0f, NetA(RrCash, 0.10f));
		F.C.FillRect({Box.X, Box.Y + 8.0f, 3.0f, Box.H - 16.0f}, RrCash);
		RrHelpDot(F, "toprate", Box.X + Box.W - 20.0f, Box.Y + 20.0f, "TOP-EARNER RATE", video::valgo::TopRateHelp(), Box.Y + Box.H);
		const float After = NetParagraph(F.C, Line, Box.X + 16.0f, Box.Y + 25.0f, Box.W - 56.0f, 13.0f, 700, RrCream, 18.0f, 3);
		const std::string Now = Top.Held > 0 ? "Held back so far this month: " + Money(Top.Held)
											 : "The next views pay " + RrPct(Top.Share, 0) + " of the full rate.";
		F.UI.Text(Now, Box.X + 16.0f, std::min(After + 2.0f, Box.Y + Box.H - 12.0f), Ts(12.0f, 600, RrCash, Align::Left, Baseline::Alphabetic, false, Box.W - 32.0f));
		NetParagraph(F.C, RrSeasonLine, Hw.X + 18.0f, std::max(Box.Y + Box.H + 26.0f, Hw.Y + 262.0f), Hw.W - 36.0f, 11.5f, 600, RrMuted, 15.0f, 2);
	}
	else
	{
		NetParagraph(F.C, RrSeasonLine, Hw.X + 18.0f, Hw.Y + 176.0f, Hw.W - 36.0f, 12.5f, 600, RrMuted, 18.0f, 3);
	}
	// Ad types: by position (the window's long videos at their own loads) and by format (a fixed display split).
	const Rect At{20.0f, 582.0f, 560.0f, 200.0f};
	RrCard(F, At, "AD TYPES");
	if (W > 0.0)
	{
		const double Ap = App / W;
		const double Mid = Ap > 0.0 ? (Ap - 1.0) / Ap : 0.0;
		const double Parts[3] = {(1.0 - Mid) * 0.9, Mid, (1.0 - Mid) * 0.1};
		const char* Names[3] = {"pre-roll", "mid-roll", "post-roll"};
		const Color Cols[3] = {RrGold, RrSky, RrMuted};
		float Bx = At.X + 18.0f;
		for (int K = 0; K < 3; ++K)
		{
			const float Bw = (At.W - 36.0f) * Nf(Parts[K]);
			F.C.FillRect({Bx, At.Y + 50.0f, std::max(0.0f, Bw - 2.0f), 14.0f}, Cols[K]);
			Bx += Bw;
			F.C.FillRoundRect({At.X + 18.0f + Nf(K) * 170.0f, At.Y + 84.0f, 9.0f, 9.0f}, 2.0f, Cols[K]);
			F.UI.Text(std::string(Names[K]) + " " + RrPct(Parts[K], 0), At.X + 32.0f + Nf(K) * 170.0f, At.Y + 93.0f, Ts(12.0f, 700, RrCream));
		}
		F.UI.Text("skippable 84% \xC2\xB7 6-second 16%", At.X + 18.0f, At.Y + 130.0f, Ts(12.0f, 600, RrMuted));
		F.UI.Text("CPM " + Money(static_cast<Chips>(std::llround(Cpm / W * 100.0))) + " \xC2\xB7 " + Fixed(Ap, 2) + " ads per playback", At.X + 18.0f, At.Y + 156.0f, Ts(12.5f, 800, RrCream));
	}
	// Payouts: the balance, the next payout (the 21st, $100 or more), the months paid.
	const Rect Po{596.0f, 582.0f, 984.0f, 200.0f};
	RrCard(F, Po, "PAYOUTS");
	int Y = 0, Mo = 0, D = 0;
	world::CivilDate(Today, Y, Mo, D);
	const int NextMo = D < 21 ? Mo : Mo % 12 + 1;
	F.UI.Text("Balance " + Money(Ch.BalanceCents), Po.X + 18.0f, Po.Y + 64.0f, Ts(22.0f, 800, RrCash));
	F.UI.Text(std::string("Next payout ") + RrMonthShort[NextMo - 1] + " 21, if $100+", Po.X + 18.0f, Po.Y + 90.0f, Ts(12.5f, 600, RrMuted));
	F.UI.Text("Paid out " + Money(Ch.PaidCents) + " so far", Po.X + 18.0f, Po.Y + 114.0f, Ts(12.5f, 700, RrCream));
	float Px = Po.X + 18.0f;
	for (auto It = Ch.Paid.rbegin(); It != Ch.Paid.rend() && Px < Po.X + Po.W - 120.0f; ++It)
	{
		// The keys are "pay:YYYY-MM": a payday, whether it paid or rolled over under $100. C3 gate (review-core-videoalgo's
		// finding): PAID only where the bank has that month's payout line (ledger kind 13), else ROLLED OVER.
		const std::string& Key = *It;
		const int Yy = Key.size() >= 11 ? std::atoi(Key.substr(4, 4).c_str()) : 0;
		const int Mm = Key.size() >= 11 ? std::atoi(Key.substr(9, 2).c_str()) : 0;
		if (Key.rfind("pay:", 0) != 0 || Mm < 1 || Mm > 12)
		{
			continue;
		}
		bool Paid = false;
		for (const life::LedgerEntry& E : F.S.Life.Ledger)
		{
			int Ey = 0, Em = 0, Ed = 0;
			world::CivilDate(net::DayOf(E.At), Ey, Em, Ed);
			Paid = Paid || (E.Kind == life::LedgerVideo && E.Amount > 0 && Ey == Yy && Em == Mm);
		}
		Px += RrTag(F.C, std::string(RrMonthShort[Mm - 1]) + (Paid ? " PAID" : " ROLLED OVER"), Px, Po.Y + 140.0f, Paid ? RrGood : RrMuted, false, 9.5f) + 6.0f;
	}
	// Ad settings on your videos, with MANAGE (every kept Update 3 long video: its load and whether it's been judged).
	const Rect As{20.0f, 798.0f, 1560.0f, 60.0f};
	RrCard(F, As, std::string());
	F.UI.Text("AD SETTINGS ON YOUR VIDEOS", As.X + 18.0f, As.Y + 36.0f, Ts(11.5f, 800, RrMuted));
	F.UI.Text("Standard " + std::to_string(static_cast<int>(LoadCount[1])) + " \xC2\xB7 Heavy " + std::to_string(static_cast<int>(LoadCount[2])) + " \xC2\xB7 Light " +
			std::to_string(static_cast<int>(LoadCount[0])) + " \xC2\xB7 Limited " + std::to_string(Limited),
		As.X + 250.0f, As.Y + 36.0f, Ts(13.5f, 800, RrCream));
	if (RrButton(F, "rrmanage", {As.X + As.W - 150.0f, As.Y + 12.0f, 132.0f, 36.0f}, U.Manage ? "DONE" : "MANAGE", true))
	{
		U.Manage = !U.Manage;
	}
	// Top earners (kept videos).
	const Rect Te{20.0f, 874.0f, 1560.0f, 76.0f};
	RrCard(F, Te, "TOP EARNERS");
	std::stable_sort(Earners.begin(), Earners.end(), [](const std::pair<Chips, const video::Video*>& A, const std::pair<Chips, const video::Video*>& B) { return A.first > B.first; });
	for (size_t K = 0; K < Earners.size() && K < 5; ++K)
	{
		const float X = Te.X + 18.0f + Nf(K) * 308.0f;
		F.UI.Text(Earners[K].second->Title, X, Te.Y + 50.0f, Ts(12.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 210.0f));
		F.UI.Text(Money(Earners[K].first), X + 290.0f, Te.Y + 50.0f, Ts(12.0f, 800, RrCash, Align::Right));
	}
	F.UI.Ptr = Real;
	if (U.Manage)
	{
		// Over the page (a scrim; a click outside it closes the list and clicks nothing under it).
		const Rect Mg{220.0f, 140.0f, 1160.0f, 640.0f};
		F.C.FillRect({0.0f, VideoTop, NetW, VideoH - VideoTop}, Hex(0x000000, 0.45f));
		if (WasOpen && F.Live && F.UI.Ptr.Pressed && !RrInside(Mg, F.UI.Ptr.X, F.UI.Ptr.Y))
		{
			U.Manage = false;
			return;
		}
		F.C.GlowRoundRect(Mg, 14.0f, Hex(0x000000, 0.5f), 24.0f);
		RrCard(F, Mg, "MANAGE ADS", "once a week, from the next midnight");
		NetParagraph(F.C, RrManageHelp, Mg.X + 18.0f, Mg.Y + 52.0f, Mg.W - 36.0f, 12.0f, 600, RrMuted, 16.0f, 2);
		float Ry = Mg.Y + 96.0f;
		for (const video::Video* V : RrUploads(F.S))
		{
			if (!V->X.On || V->F == video::Format::Short || Ry > Mg.Y + Mg.H - 30.0f)
			{
				continue;
			}
			F.UI.Text(V->Title, Mg.X + 18.0f, Ry, Ts(13.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 520.0f));
			const int Last = net::DayOf(V->At) + (V->C == video::Class::Strategy ? 90 : 14);
			F.UI.Text(Today > Last ? std::string("judged") : "check-in on day " + std::to_string(V->C == video::Class::Strategy ? 90 : 14), Mg.X + 560.0f, Ry, Ts(12.0f, 600, Today > Last ? RrGood : RrMuted));
			const char* Loads[3] = {"Light", "Standard", "Heavy"};
			for (int K = 0; K < 3; ++K)
			{
				const Rect B{Mg.X + Mg.W - 18.0f - Nf(3 - K) * 92.0f + 6.0f, Ry - 18.0f, 86.0f, 26.0f};
				if (RrSegment(F, "rrmg:" + V->Id + Loads[K], B, Loads[K], std::string(), V->X.Ads == static_cast<video::AdLoad>(K)) && V->X.Ads != static_cast<video::AdLoad>(K))
				{
					const std::string Why = F.S.SetAdLoad(V->Id, static_cast<video::AdLoad>(K));
					U.Note = Why.empty() ? std::string("Ads on this video: ") + Loads[K] + ", from the next midnight." : Why;
					U.NoteBad = !Why.empty();
					U.NoteAt = F.Now;
				}
			}
			Ry += 36.0f;
		}
	}
}

// ------------------------------------------------------------------ REALTIME (§5.7)

void RrRealtimePage(RrFrame& F)
{
	using namespace rlnet_detail;
	const video::RealtimeView& Rt = RrRealtime(F);
	const video::Channel& Ch = F.S.Life.Video;
	const chart::Theme& T = RrTheme();
	double H48 = 0.0, M60 = 0.0;
	for (const double X : Rt.Hours)
	{
		H48 += X;
	}
	for (const double X : Rt.Minutes)
	{
		M60 += X;
	}
	const Rect Hc{20.0f, 120.0f, 1100.0f, 380.0f};
	RrCard(F, Hc, "VIEWS \xC2\xB7 LAST 48 HOURS", "by the hour");
	F.UI.Text(RrCount(H48), Hc.X + 18.0f, Hc.Y + 70.0f, Ts(34.0f, 800, RrCream));
	chart::BarSet Hb;
	Hb.V.assign(Rt.Hours.begin(), Rt.Hours.end());
	Hb.Col = Hex(0x8a6a3a);
	Hb.Each.assign(48, Color{0, 0, 0, 0});
	for (size_t K = 40; K < 48; ++K)
	{
		Hb.Each[K] = RrGold;
	}
	chart::BarOpts Bo;
	Bo.Gap = 0.25f;
	Bo.X.Labels.assign(48, "");
	for (int K = 0; K < 48; K += 6)
	{
		Bo.X.Labels[static_cast<size_t>(K)] = net::TimeLabel(F.World - static_cast<double>(47 - K) * 60.0);
	}
	Bo.X.Labels[47] = "now";
	Bo.Mouse = F.Live && F.UI.Ptr.Active ? Vec2{F.UI.Ptr.X, F.UI.Ptr.Y} : Vec2{-1.0f, -1.0f};
	const chart::BarOut Out = chart::Bars(F.C, {Hc.X + 10.0f, Hc.Y + 88.0f, Hc.W - 20.0f, Hc.H - 98.0f}, {Hb}, Bo, T);
	if (Out.Hover >= 0 && Out.Hover < 48)
	{
		chart::Tooltip(F.C, {Out.HoverSlot.X + Out.HoverSlot.W / 2.0f, Out.HoverSlot.Y}, net::TimeLabel(F.World - static_cast<double>(47 - Out.Hover) * 60.0),
			{{"Views", RrCount(Rt.Hours[static_cast<size_t>(Out.Hover)]), RrGold, Color{0, 0, 0, 0}}}, T, Hc);
	}
	const Rect Mc{1136.0f, 120.0f, 444.0f, 380.0f};
	RrCard(F, Mc, "LAST 60 MINUTES", "updating live");
	F.UI.Text(RrCount(M60), Mc.X + 18.0f, Mc.Y + 70.0f, Ts(34.0f, 800, RrCream));
	chart::BarSet Mb;
	Mb.V.assign(Rt.Minutes.begin(), Rt.Minutes.end());
	Mb.Col = RrSky;
	chart::BarOpts Bm;
	Bm.YLabels = false;
	Bm.Grid = false;
	Bm.X.Labels.assign(60, "");
	Bm.X.Labels[0] = "60 min ago";
	Bm.X.Labels[59] = "now";
	chart::Bars(F.C, {Mc.X + 10.0f, Mc.Y + 88.0f, Mc.W - 20.0f, 180.0f}, {Mb}, Bm, T);
	const double SubsHour = video::SubsAt(Ch, F.World) - video::SubsAt(Ch, F.World - 60.0);
	F.UI.Text(RrCount(Rt.SubsNow), Mc.X + 18.0f, Mc.Y + 320.0f, Ts(26.0f, 800, RrCream));
	F.UI.Text("subscribers now \xC2\xB7 +" + RrCount(SubsHour) + " in the last hour", Mc.X + 18.0f, Mc.Y + 346.0f, Ts(12.5f, 600, RrMuted));
	// What's breaking out (the core's notes), and the top videos right now.
	const Rect Tc{20.0f, 516.0f, 1560.0f, 434.0f};
	RrCard(F, Tc, "TOP VIDEOS RIGHT NOW", "the last 48 hours");
	float Y = Tc.Y + 54.0f;
	for (const std::string& N : Rt.Notes)
	{
		F.C.FillRoundRect({Tc.X + 12.0f, Y - 20.0f, Tc.W - 24.0f, 30.0f}, 8.0f, NetA(RrGold, 0.12f));
		F.UI.Text(N, Tc.X + 26.0f, Y, Ts(13.5f, 800, RrGold, Align::Left, Baseline::Alphabetic, false, Tc.W - 52.0f));
		Y += 38.0f;
	}
	for (size_t K = 0; K < Rt.Top.size(); ++K)
	{
		const bool Home = std::find(Rt.OnHomeNow.begin(), Rt.OnHomeNow.end(), Rt.Top[K].first) != Rt.OnHomeNow.end();
		F.UI.Text(std::to_string(K + 1), Tc.X + 30.0f, Y + 6.0f, Ts(18.0f, 800, RrDim, Align::Center));
		F.UI.Text(Rt.Top[K].first, Tc.X + 56.0f, Y + 6.0f, Ts(15.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, 900.0f));
		if (Home)
		{
			RrTag(F.C, "SHOWN ON HOME RIGHT NOW", Tc.X + 980.0f, Y - 8.0f, RrGold, true, 10.0f);
		}
		F.UI.Text(RrViews(Rt.Top[K].second), Tc.X + Tc.W - 18.0f, Y + 6.0f, Ts(14.0f, 700, RrMuted, Align::Right));
		Y += 44.0f;
	}
}

// ------------------------------------------------------------------ RESEARCH (§5.8)

/** Research's search table (§5.8; video::SearchesOf, the C3 fix round's M2), once a world day or when the channel or the
 *  ideas move. */
const std::vector<video::SearchRow>& RrSearches(RrFrame& F)
{
	VideoUi& U = F.U;
	const std::vector<video::IdeaCard>& Ideas = RrIdeas(F);
	int Ready = 0;
	for (const video::IdeaCard& I : Ideas)
	{
		Ready += I.Ready ? 1 : 0;
	}
	const std::string Key = std::to_string(net::DayOf(F.World)) + "|" + std::to_string(F.S.Life.Video.Videos.size()) + "|" + std::to_string(Ideas.size()) + "|" + std::to_string(Ready) +
		"|" + std::to_string(static_cast<int64_t>(std::floor(F.S.Life.Video.SteppedTo)));
	if (Key != U.SearchKey)
	{
		U.SearchKey = Key;
		U.Searches = video::SearchesOf(F.S, Ideas, 10);
	}
	return U.Searches;
}

/**
 * WHAT POKER VIEWERS SEARCH (§5.8): this week's phrases, the most searched first: each with its interest against the week's
 * top phrase (a bar, never an inflated count), its arrow against last week, the player's videos on it, and MAKE IT: a ready
 * idea goes straight to Studio with it picked; a locked one opens IDEAS and says what to do.
 */
void RrSearchCard(RrFrame& F, const Rect& R)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	RrCard(F, R, "WHAT POKER VIEWERS SEARCH", "this week, against its top search");
	const std::vector<video::SearchRow> Rows = RrSearches(F); // a copy: MAKE IT can move the ideas under it
	const float Xi = R.X + 330.0f, Wi = 132.0f, Xa = R.X + 492.0f, Xm = R.X + 540.0f;
	const float Hy = R.Y + 56.0f;
	F.UI.Text("SEARCH", R.X + 18.0f, Hy, Ts(10.5f, 800, RrDim));
	F.UI.Text("INTEREST", Xi, Hy, Ts(10.5f, 800, RrDim));
	F.UI.Text("WEEK", Xa, Hy, Ts(10.5f, 800, RrDim, Align::Center));
	F.UI.Text("YOURS", Xm, Hy, Ts(10.5f, 800, RrDim));
	if (Rows.empty())
	{
		F.UI.Text(RrNoTrends, R.X + 18.0f, Hy + 34.0f, Ts(13.5f, 600, RrMuted));
		return;
	}
	float Y = Hy + 32.0f;
	for (const video::SearchRow& S : Rows)
	{
		if (Y > R.Y + R.H - 14.0f)
		{
			break;
		}
		// The phrase (a gold dot: a format you play), a TRENDING tag on this week's trend.
		float Px = R.X + 18.0f;
		if (S.YourFormat)
		{
			F.C.FillCircle(Px + 3.0f, Y - 5.0f, 3.5f, RrGold);
			Px += 12.0f;
		}
		const float Pw = F.UI.Text(S.Phrase, Px, Y, Ts(14.0f, 700, RrCream, Align::Left, Baseline::Alphabetic, false, Xi - Px - (S.Source == 1 ? 92.0f : 16.0f)));
		if (S.Source == 1)
		{
			RrTag(F.C, "TRENDING", Px + Pw + 8.0f, Y - 14.0f, RrGold, false, 9.0f);
		}
		// Interest: a bar against the week's top phrase (100).
		const float Frac = Nf(std::clamp(S.Interest, 0, 100)) / 100.0f;
		F.C.FillRoundRect({Xi, Y - 10.0f, Wi, 8.0f}, 4.0f, RrPanel2);
		F.C.FillRoundRect({Xi, Y - 10.0f, std::max(6.0f, Wi * Frac), 8.0f}, 4.0f, RrGold);
		// The week: rising, falling, or steady.
		if (S.Arrow > 0)
		{
			NetTriangle(F.C, Xa, Y - 6.0f, 6.5f, true, RrGood);
		}
		else if (S.Arrow < 0)
		{
			NetTriangle(F.C, Xa, Y - 6.0f, 6.5f, false, RrBad);
		}
		else
		{
			F.C.FillRect({Xa - 5.0f, Y - 6.5f, 10.0f, 2.0f}, RrDim);
		}
		F.UI.Text(S.Mine > 0 ? std::to_string(S.Mine) + " of yours" : std::string("none yet"), Xm, Y, Ts(12.5f, S.Mine > 0 ? 800 : 600, S.Mine > 0 ? RrGold : RrDim));
		// MAKE IT: the idea it suggests.
		if (S.Idea > 0)
		{
			const std::string Id = "rrsearch:" + S.Key;
			if (RrLink(F, Id, "MAKE IT \xE2\x80\xBA", R.X + R.W - 18.0f, Y, 12.0f, S.IdeaReady ? RrGold : RrMuted, Align::Right))
			{
				const video::IdeaCard* Card = nullptr;
				for (const video::IdeaCard& I : RrIdeas(F))
				{
					Card = I.Id == S.Idea ? &I : Card;
				}
				const video::IdeaCard Chosen = Card ? *Card : video::IdeaCard();
				U.Page = RrPageStudio;
				U.ListTab = 1;
				U.OpenTab = 1;
				U.LaunchHidden = true;
				if (Card && S.IdeaReady && Chosen.Ready)
				{
					RrChooseIdea(F, Chosen);
				}
				else
				{
					// Locked: IDEAS has its card; the toast says what to do first.
					U.Note = !S.IdeaHint.empty() ? S.IdeaHint : S.IdeaName;
					U.NoteBad = false;
					U.NoteAt = F.Now;
				}
			}
		}
		F.C.FillRect({R.X + 18.0f, Y + 12.0f, R.W - 36.0f, 1.0f}, RrLine);
		Y += 34.0f;
	}
}

void RrResearchPage(RrFrame& F)
{
	using namespace rlnet_detail;
	VideoUi& U = F.U;
	RrDaily(F);
	const video::Channel& Ch = F.S.Life.Video;
	const bool Partner = Ch.PartnerAt >= 0.0 && Ch.PartnerAt <= F.World;
	// Trending in poker this week (the left half; the search table beside it, C3 fix round M2): each on two lines, its name
	// with yours, the coin and the flag, then its heat as five bars and the lift a match gets.
	const Rect Tr{20.0f, 120.0f, 760.0f, 440.0f};
	RrCard(F, Tr, "TRENDING IN POKER THIS WEEK");
	if (!Partner)
	{
		// The help card's words behind a "?" (before Partner its card makes room for the Ad market).
		RrHelpDot(F, "research", Tr.X + Tr.W - 28.0f, Tr.Y + 22.0f, "HOW RERUN DECIDES", RrHelpCard);
	}
	float Y = Tr.Y + 60.0f;
	if (U.Trends.empty())
	{
		F.UI.Text(RrNoTrends, Tr.X + 18.0f, Y, Ts(14.0f, 600, RrMuted));
	}
	for (const video::Trend& T : U.Trends)
	{
		if (Y + 22.0f > Tr.Y + Tr.H - 8.0f)
		{
			break;
		}
		float Rx = Tr.X + Tr.W - 18.0f;
		if (T.Mine > 0)
		{
			const std::string Mine = std::to_string(T.Mine) + " of yours";
			F.UI.Text(Mine, Rx, Y, Ts(12.5f, 800, RrGold, Align::Right));
			Rx -= F.UI.Measure(Mine, 12.5f, 800) + 12.0f;
		}
		if (T.Strike)
		{
			Rx -= 12.0f;
			RrFlag(F.C, Rx, Y - 12.0f, RrWarn);
			Rx -= 8.0f;
		}
		if (T.Limited)
		{
			F.C.FillCircle(Rx - 7.0f, Y - 5.0f, 6.0f, Hex(0xf2a93b));
			F.UI.Text("LTD", Rx - 18.0f, Y, Ts(11.0f, 900, Hex(0xf2a93b), Align::Right));
			Rx -= 46.0f;
		}
		F.UI.Text(T.Label, Tr.X + 18.0f, Y, Ts(15.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, Rx - Tr.X - 30.0f));
		RrBars5(F.C, Tr.X + 18.0f, Y + 9.0f, std::clamp(static_cast<int>(std::lround(T.Heat * 5.0)), 1, 5), RrGold);
		const int Lift = static_cast<int>(std::lround((T.Strength - 1.0) * 100.0));
		F.UI.Text("matching videos get about " + std::to_string(Lift) + "% more viewers", Tr.X + 86.0f, Y + 19.0f, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Tr.W - 104.0f));
		F.C.FillRect({Tr.X + 18.0f, Y + 30.0f, Tr.W - 36.0f, 1.0f}, RrLine);
		Y += 46.0f;
	}
	RrSearchCard(F, {796.0f, 120.0f, 784.0f, 440.0f});
	// Content gaps: the ready ideas that match a trend, straight to IDEAS.
	const Rect Gp{20.0f, 576.0f, 760.0f, 374.0f};
	RrCard(F, Gp, "CONTENT GAPS", "your chance");
	F.UI.Text(RrGapsNote, Gp.X + 18.0f, Gp.Y + 54.0f, Ts(12.5f, 600, RrMuted));
	float Gy = Gp.Y + 86.0f;
	int Shown = 0;
	for (const video::IdeaCard& I : RrIdeas(F))
	{
		if (!I.Ready || !I.Trending || Gy > Gp.Y + Gp.H - 30.0f)
		{
			continue;
		}
		const Rect Row{Gp.X + 12.0f, Gy - 20.0f, Gp.W - 24.0f, 34.0f};
		const Ui::ClickState Cs = F.UI.Clickable("rrgap" + std::to_string(I.Id), Row, F.Live);
		if (Cs.Hover)
		{
			F.C.FillRoundRect(Row, 6.0f, RrPanel2);
		}
		F.UI.Text(I.Name, Gp.X + 24.0f, Gy, Ts(13.5f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, 300.0f));
		F.UI.Text(I.Fact, Gp.X + 340.0f, Gy, Ts(12.5f, 700, RrGold, Align::Left, Baseline::Alphabetic, false, 360.0f));
		if (Cs.Clicked)
		{
			U.Page = RrPageStudio;
			U.ListTab = 1;
			U.OpenTab = 1;
			U.LaunchHidden = true;
		}
		Gy += 38.0f;
		++Shown;
	}
	if (Shown == 0)
	{
		F.UI.Text(RrIdeasEmpty, Gp.X + 18.0f, Gy, Ts(12.5f, 600, RrMuted, Align::Left, Baseline::Alphabetic, false, Gp.W - 36.0f));
	}
	// Your niche: what your last 10 uploads were about, by kind group; the help card (before Partner, the Ad market in its
	// place and the help behind the trends' "?").
	const Rect Ni{796.0f, 576.0f, 784.0f, Partner ? 120.0f : 96.0f};
	RrCard(F, Ni, "YOUR NICHE");
	std::array<int, 6> Groups{};
	int N = 0;
	for (const video::Video* V : RrUploads(F.S))
	{
		if (N >= 10)
		{
			break;
		}
		++Groups[static_cast<size_t>(std::clamp(video::valgo::KindGroup(V->K), 0, 5))];
		++N;
	}
	int Top = 0;
	for (int K = 1; K < 6; ++K)
	{
		Top = Groups[static_cast<size_t>(K)] > Groups[static_cast<size_t>(Top)] ? K : Top;
	}
	F.UI.Text(N == 0 ? std::string(RrChannelEmpty) : "Your niche: " + std::to_string(Groups[static_cast<size_t>(Top)]) + " of your last " + std::to_string(N) + " are " + RrGroupNames[Top] + ".", Ni.X + 18.0f,
		Ni.Y + (Partner ? 66.0f : 60.0f), Ts(15.0f, 800, RrCream, Align::Left, Baseline::Alphabetic, false, Ni.W - 36.0f));
	if (!Partner)
	{
		RrAdMarketCard(F, {796.0f, 688.0f, 784.0f, 262.0f}, true);
		return;
	}
	const Rect Hp{796.0f, 712.0f, 784.0f, 238.0f};
	RrCard(F, Hp, "HOW RERUN DECIDES");
	NetParagraph(F.C, RrHelpCard, Hp.X + 18.0f, Hp.Y + 56.0f, Hp.W - 60.0f, 13.5f, 600, RrCream, 20.0f, 8);
}

// ------------------------------------------------------------------ the page

void RrAnalytics(RrFrame& F)
{
	VideoUi& U = F.U;
	U.Sub = std::clamp(U.Sub, 0, 5);
	const bool OneVideo = U.Sub == 1 && !U.One.empty() && RrVideo(F.S, U.One) != nullptr;
	{
		// MANAGE's list over Revenue takes the clicks: the sub-tabs under its scrim take none.
		const Pointer Real = F.UI.Ptr;
		if (U.Sub == 3 && U.Manage)
		{
			F.UI.Ptr.Pressed = false;
			F.UI.Ptr.Released = false;
		}
		RrSubTabs(F, !OneVideo && (U.Sub == 0 || U.Sub == 2 || U.Sub == 3));
		F.UI.Ptr = Real;
	}
	// An open "?" card sits over the page: the page takes no click inside it (the card's last place).
	const Pointer Real = F.UI.Ptr;
	if (!U.HelpOpen.empty() && RrInside(U.HelpBox, F.UI.Ptr.X, F.UI.Ptr.Y))
	{
		F.UI.Ptr.Pressed = false;
		F.UI.Ptr.Released = false;
	}
	switch (U.Sub)
	{
	case 1:
		if (OneVideo)
		{
			RrOneVideo(F);
		}
		else
		{
			U.One.clear();
			RrContentTable(F);
		}
		break;
	case 2: RrAudiencePage(F); break;
	case 3: RrRevenuePage(F); break;
	case 4: RrRealtimePage(F); break;
	case 5: RrResearchPage(F); break;
	default: RrOverview(F); break;
	}
	F.UI.Ptr = Real;
	RrHelpPop(F);
	if (!U.Note.empty() && F.Now - U.NoteAt < 6.0)
	{
		const float Tw = std::min(rlnet_detail::NetW - 80.0f, F.UI.Measure(U.Note, 14.0f, 700) + 32.0f);
		const Rect Tr{rlnet_detail::NetW / 2.0f - Tw / 2.0f, VideoH - 64.0f, Tw, 40.0f};
		F.C.FillRoundRect(Tr, 12.0f, RrGround);
		F.C.StrokeRoundRect(Tr, 12.0f, U.NoteBad ? RrWarn : RrGold, 1.5f);
		F.UI.Text(U.Note, Tr.X + Tr.W / 2.0f, Tr.Y + 21.0f, Ts(14.0f, 700, RrCream, Align::Center, Baseline::Middle, false, Tw - 24.0f));
	}
}
} // namespace rlvideo_detail
} // namespace ui
} // namespace ss
