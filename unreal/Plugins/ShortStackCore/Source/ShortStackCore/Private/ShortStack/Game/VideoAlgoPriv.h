// Rerun 2.0's private seam (core-videoalgo's): what Video.cpp, VideoAlgo.cpp and VideoStats.cpp share beyond the public
// headers. Nothing outside those three files includes it except, by a patch, core-videocontent's Session files (the edit's
// length options, EditMinutesFor). Every name lives in ss::video::valgo; nobody pulls it in with a using-directive.
//
// The update 3 SPEC §4-§5 model with SPEC_CREATORS §3's revisions, gated per channel: a channel runs Rerun 2.0 once its
// ChannelX is on (the first Update 3 upload turns it on; a choice with a recipe or a title template is an Update 3 upload on
// any channel). An untouched channel and every legacy video (no VideoX) read, write and step exactly as Update 2 (V6, I7).
//
// What the saved Update 3 fields mean as this file's bodies use them (Video.h's comments are the lead's; these are the
// readings core-videocontent's Session code and ui-rerun rely on):
//   VideoX::Ctr          the packaging CTR the video is served at (a Duel's contenders' mean for 48 hours, the winner's after)
//   VideoX::S0           the score's standing part: the upload's, moved by each repackage and each ad-load switch
//   VideoX::AdsAtUpload  the load the stored rates (Video::Rpm, Retention, Conv) and S0 are at: the upload's until a switch
//                        takes effect; Ads, when different, is a switch waiting for its first day, AdsChangedDay (a switch
//                        made on day d sets Ads and AdsChangedDay = d + 1; DayStep's ApplyLoad moves the rates at its start)
//   VideoX::SettledS     the MomC the upload got (the launch card's carry, true at upload)
//   VideoX::Spec         the recipes: [0] A, [1] B, [2] C; the shown one is Spec[max(0, DuelWin)]
//   VideoX::Promise      the title's promise (the thumbnail's comes from ThumbPromise; PromiseOf takes the larger)
//   Comp::CtrF           the part's CTR against the rolled CTR (impressions); a Kast (Live Drop) part keeps the stream's
//                        own average it was sized by
#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoStats.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ss
{
namespace life
{
struct State;
}
namespace world
{
class World;
}
namespace video
{
namespace valgo
{
constexpr double VaDay = 1440.0;

// ---------------------------------------------------------------- the constants (SPEC_CREATORS §3.3-§3.4; update 3 SPEC §4.4-§4.6)
constexpr double VaKappaNeutral = 0.35;
constexpr double VaKappaLo = 0.07, VaKappaHi = 0.875;
constexpr double VaBreakoutMeanSize = 5.5; // U(3, 8)'s mean
constexpr double VaCheckChance = 0.6;
// (C3, SPEC_CREATORS §3.6's knob order on the library's archetypes, 24 seeds (STATUS_core-videoalgo.txt, ssb/c3r-core-videoalgo/
// knobs): at the spec's values the expert reads 1.91x / 1.91x its twin's subscribers and money, just past 1.9. The Momentum
// weight's 0.3 breaks the meter's band (MomC p10 0.80, p90 1.21) and stays 0.4; the streak step 2.0% -> 1.5% (VaStreakStep) and
// MomC's clamp 0.75-1.8 -> 0.8-1.6 (the normaliser re-pinned to 1.05, a neutral channel's mean as before) bring it to about
// 1.85x / 1.89x; the Fresh lane's 1,200 moved nothing and stays 1,800.)
constexpr double VaMomWeight = 0.4, VaMomLo = 0.8, VaMomHi = 1.6, VaMomNorm = 1.05;
constexpr double VaMomBreakout = 0.5, VaMomWelcome = 0.2, VaMomDormant = 0.95;
constexpr int VaDormantDays = 21;
constexpr double VaPerfLo = -1.2, VaPerfHi = 2.0;
constexpr double VaKickLo = 0.4, VaKickHi = 3.0;
constexpr double VaKickNormStay = 1.764;   // kinds that don't travel (the viral roll at 0.5%)
constexpr double VaKickNormTravel = 1.890; // bad beats, wins and bluffs (1%)
constexpr double VaStreakStep = 0.015; // (C3's knob: 2.0% -> 1.5%, to +12% at 8 weeks)
constexpr int VaStreakMax = 8, VaRainEvery = 4, VaRainMax = 2;
constexpr double VaFresh = 1800.0;
constexpr int VaFreshUploads = 12;
constexpr double VaBingeAt = 3.0, VaBingeShare = 0.04, VaBingeA = 0.5;
constexpr int VaBingeTargets = 3, VaBingeHistory = 5, VaBingeTypical = 10;
constexpr double VaBloomChance = 0.04, VaBloomSize = 1.0, VaBloomA = 0.5;
constexpr double VaDropMinutes = 30.0, VaDropWindow = 360.0, VaDropX = 1.2, VaDropCap = 1.5, VaDropHours = 2.0, VaDropA = 0.9;
constexpr double VaDropV = 0.10, VaTeaserV = 0.05, VaReplyQ = 0.04;
constexpr double VaHeatHard = 6.0, VaHeatBreak = 10.0;
constexpr double VaBurnFree = 4.0, VaBurnSlope = 0.05, VaBurnFloor = 0.80;
constexpr double VaLmaxVideo = 0.30;
constexpr size_t VaMaxComps = 9; // Comps beside the first wave: <= 10 a video (I9)
constexpr double VaBreakoutA = 0.30, VaFromCutA = 0.90, VaTrendA = 0.50; // the components' fixed A (§4.4's table)
constexpr double VaCreatorShare = 0.55, VaMonetized = 0.50, VaMonetizedLimited = 0.30, VaLimitedCpm = 0.40;
/** Suitability (§4.5.4): site promotion is 18+ (Home reach x0.6, no Cuts feed); strong language (a tilted face with a Loud
 *  title) runs Limited ads unless the edit is clean (+15 minutes of Update 3 editing). VaExprTilted is SPEC §6.1's twelve
 *  expressions in Portrait.h's ExpressionOf order (0 Neutral .. 11 Exhausted). */
constexpr double VaAdultHome = 0.6, VaCleanEditMinutes = 15.0;
constexpr int VaExprTilted = 7;
/** Payoff (§4.6.5): a pot in the top 10% of the player's own (the open moments', at least 10 of them) pays off at 3. */
constexpr int VaPotHistoryMin = 10;
/** BaseCPM: chosen so a neutral Breakdown reproduces Update 2's RPM exactly (2.5 / (0.50 x 1.20 x 0.55); shown "$7.58"). */
constexpr double VaBaseCpm = 2.5 / (VaMonetized * 1.20 * VaCreatorShare);
/** The Cuts pool (§4.5.6): an ad about every 5 Cuts, the creators' 45%. A Cut's RPM stays Update 2's (2.5 x 0.03). */
constexpr double VaCutAdsPerPlayback = 0.2, VaCutShare = 0.45;

/** X rounded to 1 / Scale (what a saved Update 3 number keeps: a component's views to the cent, its A to 1e-4). */
inline double VaRound(double X, double Scale)
{
	return std::round(X * Scale) / Scale;
}

/** A curve's views as the save keeps them (P0, a component's P): whole views from 100 up, to the cent below (C3's compaction). */
inline double VaViews(double P)
{
	return std::fabs(P) >= 100.0 ? std::round(P) : VaRound(P, 100.0);
}

// ---------------------------------------------------------------- shared helpers (bodies in Video.cpp unless said)
struct VaShape
{
	double A, Tau1, Tau2;
};
VaShape ShapeU3(Class C);
/** A(1 - e^(-d/Tau1)) + (1 - A)(1 - e^(-d/Tau2)) for d > 0 (days), else 0. */
double CurveFrac(double A, double Tau1, double Tau2, double Days);
/** An Update 3 video's views at World: the first wave (P0 at At, the class's A), its components and the evergreen term. */
double ViewsU3(const Video& V, double World);
/** The views a video has still to come after World from its first wave and components (the Next-up card's base). */
double RemainingU3(const Video& V, double World);
/** An Update 3 video over [T0, T1]: ViewsU3 at both ends and ImprGain between them, bit for bit, each part's curve read once. */
void SpanU3(const Video& V, double T0, double T1, double& Was, double& Now, double& Impr);
int DayOf(double World);
/** A video's Update 2 kind group (beats, reads, pots, runs, the rival, stories), its Topic weight's "travels" flag. */
int KindGroup(Kind K);
bool Travels(Kind K);
/** Update 2's tables (Video.cpp's rows): the format's base retention, RPM row and FormatSubs; the style's CTR, retention,
 *  subscribers and ad fit. */
double FormatRetention(Format F);
double FormatRpmRow(Format F);
double StyleCtr(TitleStyle T);
double StyleRet(TitleStyle T);
double StyleAdFit(TitleStyle T);
/** Update 2's thumbnail table (ThumbFit[frame][group]). */
double FrameFit(Frame F, int Group);
/** ss.Video.Grant's synthetic video (no upload of the player's: never ranked, never typical). */
bool IsGrant(const Video& V);
/** What a moment can back up, 0-3 (§4.6.5; Upload's VideoX::Payoff): Studio's honest title or recipe promises no more. */
uint8_t PayoffFor(const Candidate& Lead, const std::vector<Candidate>& Open);
/** Sameness (§4.6.2): x0.90 for an archetype in 3 of the last 5 recipe uploads, x0.82 in 4 or 5 (Upload's own factor). */
double NoveltyFor(const Channel& C, int Arch);
/** The in-app bell, the last 20 (Video.cpp's Bell). */
void PushBell(Channel& C, double At, const std::string& Line);
/**
 * c3p-stats (C3-F1, update 3 SPEC §7.5 item 3): what the folded part of the channel gained between T0 and T1, Video.cpp's
 * GainsBetween less the kept videos: the tails (read back only from the oldest kept upload: a little under before it, never
 * over), Update 2's evergreen stubs and the day-bucketed ones. Out += views, net subscribers, ad dollars (Paying), and the
 * Short class's tail's ad dollars (Paying) and views (the Cuts feed's part).
 */
void FoldedGains(const Channel& C, double T0, double T1, bool Paying, double Out[5]);
/** The audience's hour curve (§5.5, VideoAlgo.cpp): Kast's poker hours over the viewers' time zones; PeakHour is its maximum. */
double HourHeat(int Weekday, int Hour);

// ---------------------------------------------------------------- length and ads (§4.5.5)
/** The edit's length in minutes for a format and its length option (a Cut is 45 seconds, fixed). */
double LengthMinFor(Format F, LengthOpt L);
/** The edit's world minutes and energy for the format and the length option (Standard: Update 2's EditMinutes/EditEnergy). */
double EditMinutesFor(Format F, LengthOpt L);
double EditEnergyFor(Format F, LengthOpt L);
double LengthRet(Format F, LengthOpt L);
/** The ad load's retention, dislike, score and conversion terms: 1 (or 0) wherever the load doesn't change the ads. */
double AdRet(Format F, LengthOpt L, AdLoad A);
double AdDislikes(Format F, LengthOpt L, AdLoad A);
double AdScore(Format F, LengthOpt L, AdLoad A);
double AdConv(Format F, LengthOpt L, AdLoad A);
/** Strong language (§4.5.4): a recipe upload with a Loud title and a tilted face on any of its thumbnails. Limited ads, or
 *  with Clean edit full ads and 15 more minutes toward burnout; ExtraHours7 derives the clean edit (no saved flag). */
bool StrongLanguage(const Video& V);
/** Site promotion's 18+ (a Limited coin without strong language; a site promotion that also has strong language reads as
 *  strong language here: the upload's own reach used the exact reason). */
bool AdultOnly(const Video& V);
/** The load in effect on world day Day: Ads once AdsChangedDay has come (a switch waits for the next midnight). */
AdLoad LoadOn(const VideoX& X, int Day);
/** The video's rates on a day at that day's load (the stored Rpm, Retention and Conv are at X.AdsAtUpload's load). */
double RpmOn(const Video& V, int Day);
double RetentionOn(const Video& V, int Day);
double ConvOn(const Video& V, int Day);
/** A pending switch takes effect at the start of AdsChangedDay: the stored rates and S0 move to the new load (DayStep). */
void ApplyLoad(Video& V, int Day);

// ---------------------------------------------------------------- engagement (§5.1): rates per view from Fate:rates
struct Rates
{
	double Like = 0, DislikePerLike = 0, Comment = 0, Share = 0, Save = 0;
	double Dislike() const { return Like * DislikePerLike; }
	double LikeRatio() const { return 1.0 / (1.0 + DislikePerLike); }
};
/** The N(0,1) draws behind a video's rates: Box-Muller pairs from Fnv1a("<Fate>:rates:<i>"), clamped to +-3. */
double FateNormal(uint32_t Fate, int I);
/** A video's rates at a load and retention (the over-promise level and the reply round folded in). */
Rates RatesOf(const Video& V, AdLoad Load, double Retention, int Over);
/** The format's neutral retention (Story, gear 0.5): Q's reference. */
double RetentionRef(Format F);
/** max(0, Promise - Payoff) on a recipe upload; 0 on a legacy frame (Update 2's choice sets have no promise gap). */
int OverOf(const Video& V);
int UnderOf(const Video& V);
/** The thumbnail's promise in effect: the Duel's mean before the verdict, the winner's after, else the shown recipe's. */
int PromiseOf(const Video& V);
/** Q from the rates (§4.6.2), clipped to [-1.5, 1.5]. */
double QOf(const Video& V, const Rates& R, double Retention, int Over);
/** Subscribers lost per subscriber net (§5.1's display split: lambda / (1 - lambda)). */
double LostShare(double Q, int Over, bool Heavy);
/** A legacy video's click-through, estimated from its frame, its style and the rig its retention implies (§4.8: "est."). */
double LegacyCtr(const Video& V);
/** The rolled CTR the first wave saw (an Update 3 video's Ctr x CtrNoise; a legacy one's estimate). */
double RolledCtr(const Video& V);

// ---------------------------------------------------------------- traffic and impressions (§5.4: Reach)
/** The ten sources, in SourceSplit's order: Home, Next up, Search, Subscriptions, Notifications, the Cuts feed, Kast, From
 *  Cuts, External, Playlists and channel. A component's mix (Comp::SrcMix): 0 the first wave, 1 the verdict, 2 a check-in,
 *  3 a Strategy late check-in, 4 a breakout, 5 a rescale (a repackage, the Duel), 6 a trend, 7 From Cuts, 8 a Next-up card,
 *  9 Kast (Live Drop), 10 the Binge wave, 11 Late Bloomer. */
std::array<double, 10> MixOf(const Video& V, uint8_t Mix);
/** The source mix each component kind is appended with (one mix a kind: the save derives it). */
uint8_t MixFor(CompKind K);
/** The A a component of kind K on V is appended with when its kind fixes one (the class's for the kicks and a Next-up card's,
 *  §4.4's constants for the rest); false for the rescales (a Duel's switch, a repackage), whose A is their own. */
bool KindA(const Video& V, CompKind K, double& A);
/** Each source's click-through relative to the rolled CTR (Kast, From Cuts and External carry no impressions: 0). */
double SourceRel(int Source);
/** The first wave's normalizer, so its blend is exactly the rolled CTR. */
double FirstWaveNorm(const Video& V);
/** Impressions per view of a component (the rolled CTR is VideoX::Ctr x CtrNoise; later parts read Ctr x rel x CtrF). */
double ImprPerView(const Video& V, uint8_t Mix, float CtrF);
/** An Update 3 video's impressions gained between T0 and T1 (each part at its own mix); a legacy one's estimated. */
double ImprGain(const Video& V, double T0, double T1);

// ---------------------------------------------------------------- the score, the kick, Momentum
double TimeFitAt(const Channel& C, double At);
double TimingV(double TimeFit);
/** The score at a decision (1 the verdict, 2+ the check-ins), from the standing part X.S0 (the upload's, revised by each
 *  repackage and load switch) plus the launch actions and the series term. */
double ScoreNow(const Video& V, const Channel& C, int Day, int Stage);
double Perf(const VideoX& X);
double PerfKick(const Video& V);
double MomCOf(double H);
/** Kick shares by class: the verdict, days 7, 14 (Strategy 30 and 90 too). */
double ShareOf(Class C, int CheckDay);
int VerdictDayOf(const Video& V);
bool CompFits(const Video& V, int Day);

// ---------------------------------------------------------------- the copy Rerun 2.0's core says (VideoAlgo.cpp)
std::string BellVerdict(const std::string& Title, int Label);
std::string BellKick(const std::string& Title);
std::string BellBreakout(const std::string& Title);
std::string BellViral(const std::string& Title);
/** C3 fix L4: rung at the midnight after the wave began, with what its parts really added that day ("New viewers are bingeing
 *  your channel: 3 older videos picked up 1,240 views."). */
std::string BellBinge(int Videos, double Views);
/** C3 fix L4: rung at the midnight after the check-in that woke it, with its day against its usual day (the 7 days before):
 *  "... woke up: 3x its usual daily views." (U+00D7); under 2x, the old words without a number. */
std::string BellBloom(const std::string& Title, double Ratio);
std::string BellDuel(int Winner);
std::string BellStreak(int Weeks);
std::string BellEarly(const std::string& Title);
/** A trend that opened since the last midnight (§6.6.5: one a day), by Research's label. */
std::string BellTrend(const std::string& Label);
/** Burnout's warning (§4.6.6): Update 3's extra editing this week past 4 hours, the hours whole. */
std::string BellBurnout(double Hours);
/** A Rain Check earned (every 4 streak weeks, at most 2 held). */
std::string BellRainCheck(int Weeks);
// The launch card's and Studio's lines (SPEC_CREATORS §5.3 R1-R3, update 3 SPEC §4.5.8; ui-rerun draws them, the core lints
// them): fragments without a closing stop, every digit a fact.
/** "Riding your breakout: this upload starts with +38% reach" (MomC at upload - 1, whole percent; only while it's over 1.1). */
std::string LaunchCarry(double MomCAtUpload);
/** A label under the carry: "Held despite your momentum" (Not pushed and Held; "" for the others). */
std::string VerdictDespite(int Label);
/** "Live Drop: your stream gave it a push (212 watching)" (VideoStats::LiveDropAvg), and Studio's hint after a publish. */
std::string LiveDropLine(int Watching);
std::string LiveDropHint();
/** "Upload streak: 6 weeks · 1 Rain Check" (ChannelX::StreakW, RainChecks). */
std::string StreakLine(int Weeks, int RainChecks);
/** "As a Partner, this video would have earned about $4.10 so far (not paid)" (VideoStats::WouldEarnCents). */
std::string WouldEarnLine(Chips Cents);
/** VideoStats::Tag's name: 1 Above typical, 2 Breakout, 3 Outlier ("" for 0). */
const char* TagName(int Tag);
/** The first Limited ads coin's letter (life::Letter::Kind: Video.h's LetterLimited, appended at the C3 gate). */
constexpr int VaLetterLimited = LetterLimited;
static_assert(VaLetterLimited == 4, "the Limited ads letter's kind is saved: it stays 4");
std::string LimitedSubject();
std::string LimitedBody(const std::string& Title);
const char* VerdictName(int Label);
std::vector<std::string> AlgoInventory();

// ---------------------------------------------------------------- an idea's facts and the trends (§4.5.4, §4.6.10)
/**
 * What core-videoalgo reads of Candidate::IdeaFacts (core-videocontent writes them: "<key>=<value>" tokens separated by ';',
 * a value never holding ';', '=' or whitespace):
 *   ltd=1          the idea promotes a real-money site: Limited ads and 18+ (Home x0.6, no Cuts feed)
 *   ad=<category>  its advertiser category (§4.5.4): gear 1.25, teach 1.20, results 1.15, story 1.00, life 0.95, loud 0.90,
 *                  news 0.90, rival 0.85 (in place of the title style's AdFit)
 *   trend=<key>    a trend it matches (one token a trend; keys below), e.g. trend=champ:812345 for a reaction to a champion
 *   few=1          its topic has fewer than 3 matching videos among the world's creators (the trend factor x1.10)
 *   w=<weight>     its Topic weight (§6.3's column, 0.9-2.4; the kind Idea's own is 1.0)
 *   cool=<days>    its cooldown: a repeat inside it reaches x0.5 (7 days when unsaid). Upload records each idea video's use in
 *                  ChannelX::IdeaCool (key the idea, the day, a count), which the IDEAS board reads
 *   cls=S          a Strategy lesson (§6.3's S): the video is the evergreen class in any long format (C3)
 *   fmt=<digits>   the formats the idea may go up as (Studio's; CanUpload lets a lone idea up as a Vlog or a Recap it lists)
 * An idea video's advertiser category on the Ad market card is IdeaAdCategory(VideoX::Idea) (core-videocontent's table).
 */
constexpr int VaIdeaCoolDays = 7;
std::vector<std::string> FactValues(const std::string& Facts, const std::string& Key);
/** The advertiser fit of an idea's "ad=" category; 0 for an unknown one (the title style's AdFit stands). */
double AdCategoryFit(const std::string& Category);
/**
 * A trend window, from its self-describing key (so an upload prices it from the key alone, no world needed):
 *   sunday:<day>      Sunday noon -> Monday noon, 1.08         series:<id>        the series' days (net::Shared()), 1.12-1.20
 *   champ:<minute>    a champion, 3 days, 1.15                 final:<minute>     a major's final table, 3 days, 1.10
 *   poy:<year>        Nov 15 -> Dec 31, 1.10                   hof:<year>         Jul 10 -> 20, 1.15
 *   site:<minute>     a rival site's launch, 7 days, 1.25      overlay:<minute>   a rival's overlay, 7 days, 1.10
 *   holiday:<year>    Dec 20 -> 31, 1.10                       newyear:<year>     Dec 26 -> Jan 15 (the December's year), 1.15
 * champ, final, site and overlay are the world log's: <minute> is the event's, and the window runs from the first Monday the
 * event is a day old by (the Monday Research's weekly list takes it in), so a 3-day trend is that week's Monday to Wednesday.
 */
struct VaTrend
{
	std::string Key;
	double Start = 0, End = 0, Strength = 1; // world minutes; End exclusive
};
bool TrendOfKey(const std::string& Key, VaTrend& Out);
/** heat(t): a triangular pulse over the window, 0 at its ends and 1 at its middle. */
double TrendHeat(const VaTrend& T, double At);
/** The trends Lead matches at At: a Sunday major's candidate in its window, a series event's in its series' days, and every
 *  trend its idea's facts name (the world's own reach an upload only that way: Upload has no world). */
std::vector<VaTrend> TrendsFor(const Candidate& Lead, double At);
/** A trend key's TrendCool key (ChannelX::TrendCool counts each trend's matching uploads: the third gets no factor). */
int TrendCoolKey(const std::string& Key);
/** Research's list at World (VideoStats.cpp; video::Trends is it with a world): the calendar's and the network's trends, and
 *  with a world, its log's (dated before the week's Monday, a day old at least), the hottest first. */
std::vector<Trend> TrendsNow(const life::State& L, const world::World* W, double World);
constexpr double VaTrendTr = 0.4, VaTrendComp = 0.08, VaTrendCompAt = 0.25, VaTrendFew = 1.10, VaTrendTourists = 0.70;

// ---------------------------------------------------------------- the money's split (§4.5.7's itemized statement, §5.6's Revenue)
/**
 * DayRow's split, appended by the lead's Video.h patch (core-videoalgo_to_lead_2.diff): CutCents and CutViews, the Cuts feed's
 * share of a row's Cents and Views; AdImpr and Monetized, the watch page's ad impressions and monetized playbacks behind the
 * rest. DayStep fills them on the days ads pay (Partner); until the patch lands DayRow has no such fields, these read 0 and
 * write nothing, and the statement stays Update 2's. (VaRowSplit<DayRow> is true from the patch on; the trait can go once it has.)
 */
template <class R, class = void>
struct VaRowSplit : std::false_type
{
};
template <class R>
struct VaRowSplit<R, std::void_t<decltype(std::declval<R&>().CutCents), decltype(std::declval<R&>().CutViews), decltype(std::declval<R&>().AdImpr),
						 decltype(std::declval<R&>().Monetized)>> : std::true_type
{
};
constexpr bool VaHasSplit = VaRowSplit<DayRow>::value;
template <class R>
Chips VaRowCut(const R& Row)
{
	if constexpr (VaRowSplit<R>::value)
	{
		return Row.CutCents;
	}
	else
	{
		(void)Row;
		return 0;
	}
}
template <class R>
double VaRowCutViews(const R& Row)
{
	if constexpr (VaRowSplit<R>::value)
	{
		return static_cast<double>(Row.CutViews);
	}
	else
	{
		(void)Row;
		return 0.0;
	}
}
template <class R>
double VaRowImpr(const R& Row)
{
	if constexpr (VaRowSplit<R>::value)
	{
		return static_cast<double>(Row.AdImpr);
	}
	else
	{
		(void)Row;
		return 0.0;
	}
}
template <class R>
double VaRowMonetized(const R& Row)
{
	if constexpr (VaRowSplit<R>::value)
	{
		return static_cast<double>(Row.Monetized);
	}
	else
	{
		(void)Row;
		return 0.0;
	}
}
/** Sets a row's split (nothing before the patch): AddToRow's tenths-below-100 rounding for the counts. */
template <class R>
void VaSetRowSplit(R& Row, Chips Cut, float CutViews, float Impr, float Monetized)
{
	if constexpr (VaRowSplit<R>::value)
	{
		Row.CutCents = Cut;
		Row.CutViews = CutViews;
		Row.AdImpr = Impr;
		Row.Monetized = Monetized;
	}
	else
	{
		(void)Row;
		(void)Cut;
		(void)CutViews;
		(void)Impr;
		(void)Monetized;
	}
}
/**
 * The top-earner rate's held-back cents on a row (c3f-algo_to_lead_1.diff appends DayRow::HeldCents and HeldCutCents: what the
 * rate held back from the row's days, the ads' earnings less Cents, and the Cuts feed's part of it). The money never needs them
 * (TopRatePay works from the paid cents alone); the statement's CPM (the advertisers' price, before any share) and
 * TopRateView::Held do. Until the patch lands they read 0 and write nothing, like VaRowSplit before its patch.
 */
template <class R, class = void>
struct VaRowHeldT : std::false_type
{
};
template <class R>
struct VaRowHeldT<R, std::void_t<decltype(std::declval<R&>().HeldCents), decltype(std::declval<R&>().HeldCutCents)>> : std::true_type
{
};
constexpr bool VaHasHeld = VaRowHeldT<DayRow>::value;
template <class R>
Chips VaRowHeld(const R& Row)
{
	if constexpr (VaRowHeldT<R>::value)
	{
		return Row.HeldCents;
	}
	else
	{
		(void)Row;
		return 0;
	}
}
template <class R>
Chips VaRowHeldCut(const R& Row)
{
	if constexpr (VaRowHeldT<R>::value)
	{
		return Row.HeldCutCents;
	}
	else
	{
		(void)Row;
		return 0;
	}
}
template <class R>
void VaSetRowHeld(R& Row, Chips Held, Chips HeldCut)
{
	if constexpr (VaRowHeldT<R>::value)
	{
		Row.HeldCents = Held;
		Row.HeldCutCents = HeldCut;
	}
	else
	{
		(void)Row;
		(void)Held;
		(void)HeldCut;
	}
}
/**
 * A span's ad money, split: the Cuts feed's cents and views and the watch page's, with the watch page's ad impressions and
 * monetized playbacks, so the span's CPM (gross / impressions) and playback CPM (gross / monetized playbacks) are the
 * identity's own: impressions x CPM / 1,000 x 55% + the Cuts feed = the cents paid (§5.6). Views: every view of the span.
 * Has: the rings carried the split for the span (the Video.h patch). Held, HeldCut: what the top-earner rate held back on
 * the span's days (the watch page's part is in the CPMs: advertisers paid it; Rerun kept it).
 */
struct VaSplit
{
	Chips Cents = 0, CutCents = 0;
	double Views = 0, CutViews = 0, AdImpr = 0, Monetized = 0;
	int FromDay = -1, ToDay = -1;
	bool Has = false;
	Chips Held = 0, HeldCut = 0;
	Chips WatchCents() const { return Cents - CutCents; }
	/** The watch page's ads before the top-earner rate (what the CPMs are of). */
	Chips WatchGross() const { return WatchCents() + std::max<Chips>(0, Held - HeldCut); }
	double WatchViews() const { return std::max(0.0, Views - CutViews); }
	/** Dollars per 1,000 views: every view (Rerun's RPM), the watch page's, the Cuts feed's; per 1,000 ad impressions before
	 *  Rerun's share; per 1,000 monetized playbacks. */
	double Rpm() const { return Views > 0.0 ? static_cast<double>(Cents) / 100.0 / Views * 1000.0 : 0.0; }
	double WatchRpm() const { return WatchViews() > 0.0 ? static_cast<double>(WatchCents()) / 100.0 / WatchViews() * 1000.0 : 0.0; }
	double CutRpm() const { return CutViews > 0.0 ? static_cast<double>(CutCents) / 100.0 / CutViews * 1000.0 : 0.0; }
	double Cpm() const { return AdImpr > 0.0 ? static_cast<double>(WatchGross()) / 100.0 / VaCreatorShare / AdImpr * 1000.0 : 0.0; }
	double PlaybackCpm() const { return Monetized > 0.0 ? static_cast<double>(WatchGross()) / 100.0 / VaCreatorShare / Monetized * 1000.0 : 0.0; }
};
/** The newest days of the rings whose cents reach Target (past the 90-day ring, the month rows; the oldest part in proportion). */
VaSplit SplitCovering(const Channel& C, Chips Target);
/** The balance's split between steps: the days since it was last paid out (SplitCovering(C, C.BalanceCents)). */
VaSplit BalanceSplit(const Channel& C);
/** (The statement's words are core-videocontent's: vcontent::StatementU3 with WatchCents, WatchRpm, Cpm and CutCents.) */

// ---------------------------------------------------------------- the top-earner rate (the user's soft ceiling, 2026-10-27; C3_AUDIT H1)
/**
 * Rerun's pay for a pay month (the days one payout covers: the 21st to the 20th, the month named by the 21st that pays it) is
 * what the ads earned, E, up to T = 6 rents (life::MonthlyRentCents), then T + C (1 - e^(-(E - T) / C)) with C = 4 rents: past
 * T it grows more and more slowly toward T + C. Views, subscribers, bells, impressions and the audience keep every view; only
 * the money flattens, and only a Rerun 2.0 channel's (an Update 2 channel's ads stay Update 2's, as the season does).
 * DayStep applies it as each day accrues, from the month's paid cents so far P (the day rings): above T the gap to T + C
 * shrinks by e^(-r / C) for r cents more, the curve's own step, so N one-day steps are the month's curve (to the day's cent
 * rounding), a skip of N days is N steps, and a reload changes nothing (no new saved state: the rings hold P).
 */
constexpr int VaPayDay = 21; // Video.cpp's PayDay (static_assert there): the payout's day of the month
Chips TopRateFrom(); // T: 6 rents
Chips TopRateTop();  // T + C: 10 rents, what no month pays past
/** The pay month a world day's ads belong to: Y * 12 + (Mo - 1) of the 21st whose payout covers it (the 21st on: the next). */
int PayMonthOf(int Day);
/** The first world day of Day's pay month (the 21st on or before it). */
int PayMonthFirst(int Day);
/** The cents Earned more cents of ads pay in a month that has paid PaidSoFar: Earned itself while the month stays at or under
 *  T (byte for byte today's), the curve's step past it; never more than Earned, never past T + C. */
Chips TopRatePay(Chips PaidSoFar, Chips Earned);
/** The share of full pay the next cent of ads pays at: 1 at or under T, (T + C - P) / C past it. */
double TopRateShare(Chips PaidSoFar);
/** What Day's pay month has paid on the days before Day (the day rings; 0 for a channel off Rerun 2.0). */
Chips PaidThisMonth(const Channel& C, int Day);
/** What Day's ads paid against what they earned, the same for every video that day (DayStep's share): 1 at or under T; a
 *  stepped day's from its row (its held cents once DayRow keeps them, else the curve read back from its paid cents); a day not
 *  stepped yet, the share its first view pays at (StatsOf's "today"). For a video's RPM a day (review-c3f-algo). */
double DayPaidShare(const Channel& C, int Day);
/** The curve closed: what a month whose ads earned EarnedCents pays, in one go (where the stepped days land, to their cents). */
double TopRateCurve(double EarnedCents);
/** One pay month so far (through the last midnight: the rings, what its statement will itemize), for the Revenue page. */
struct TopRateView
{
	int Month = -1;       // PayMonthOf the day
	Chips Paid = 0;       // the month's cents through the last midnight
	Chips Held = -1;      // what the rate held back of them; -1 unknown (DayRow has no HeldCents yet: c3f-algo_to_lead_1.diff)
	double Share = 1.0;   // TopRateShare(Paid): what today's views pay against the full rate (StatsOf's Rpm uses the same)
	bool Applies = false; // Paid passed T
};
/** (VideoStats.cpp.) */
TopRateView TopRateOf(const Channel& C, double World);
/** "Top-earner rate: above $6,450 this month pays at a reduced share." (the statement's line and the Revenue page's) */
std::string TopRateLine();
/** The "?" card's words, every amount from the game's rent. */
std::string TopRateHelp();
/** The statement's CPM for a span that may hold top-earner days: the advertisers' price, never the rate's share (the held cents
 *  when DayRow keeps them; before that, the month's days the rate didn't touch). */
double StatementCpm(const Channel& C, const VaSplit& S, int PayMonth);

// ---------------------------------------------------------------- the title template's hidden traits (§4.6.2)
/**
 * TitleOption::Traits, as core-videocontent sets them (bit -> CTR factor): 1 a curiosity gap (+6%), 2 a specific number
 * (+4%), 4 first person (+3%), 8 a question (+3%), 16 40-70 characters (+3%), 32 over 85 characters (-5%), 64 a named
 * creator or rival (+5%), 128 a search phrase (-3% CTR, +4% retention), 256 "episode N" (+6%). Bits 9-15 are
 * core-videocontent's style renormalizer, an unsigned fixed-point factor (1.000 + (n - 64) / 500 for n in 0..127, 0.874 to
 * 1.126; 0 is 1.000): each style's templates average 1.000 with it (§4.6.2).
 */
double TitleTraitCtr(uint16_t Traits);
double TitleTraitRet(uint16_t Traits);
} // namespace valgo
} // namespace video
} // namespace ss
