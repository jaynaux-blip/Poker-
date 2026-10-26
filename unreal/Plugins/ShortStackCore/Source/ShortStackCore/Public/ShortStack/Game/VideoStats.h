#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Video.h"

#include <array>
#include <string>
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

/**
 * Update 3 (SPEC §5, §7.3): Rerun 2.0's analytics, the data contract from core to the UI. Every number is the model's own
 * (V4): impressions, click-through, views, unique and returning viewers, average view duration, retention with its key
 * moments, likes and dislikes, comments, shares, saves, subscribers gained and lost, traffic sources, revenue, RPM, CPM.
 * An Update 2 video's numbers derive from its saved fields (Fate = Fnv1a(Id)) and carry Estimated. Declared by the lead in
 * Phase A; bodies by core-videoalgo (VideoStats.cpp, G3). ui-rerun reads it and never computes a number.
 */
namespace video
{
/** Views and impressions by source, in Src order: Home, NextUp, Search, Subs, Notif, Cuts, Kast, FromCuts, External, Channel. */
struct SourceSplit
{
	std::array<double, 10> Views{}, Impressions{};
};
/** One video's numbers at a world minute (StatsOf). */
struct VideoStats
{
	double Views = 0, Impressions = 0, Ctr48 = 0, CtrLife = 0, AvdMin = 0, AvgPct = 0, WatchHours = 0, Unique = 0, Returning = 0, Likes = 0, Dislikes = 0, Comments = 0,
		   Shares = 0, Saves = 0, SubsNet = 0, SubsLost = 0;
	Chips EarnedCents = 0;
	double Rpm = 0, Cpm = 0, PlaybackCpm = 0, Monetized = 0, AdImpressions = 0;
	int Coin = 0, Verdict = -1;
	SourceSplit Src;
	std::array<float, 61> Retention{}, Typical{};
	std::vector<std::pair<float, std::string>> Moments;
	std::vector<std::pair<double, std::string>> Log; // "What Rerun did with it"
	std::vector<std::string> Insights;
	std::array<float, 3> DuelCtr{}, DuelShare{};
	bool Estimated = false;
	// core-videoalgo, appended (SPEC_CREATORS §3, §5.3 R1-R4; announced in STATUS_core-videoalgo.txt):
	Chips WouldEarnCents = 0; // before Partner: what the views so far would have earned at the day's season (not paid, W9)
	int Tag = 0;              // against the channel's typical upload: 1 Above typical (2x), 2 Breakout (5x), 3 Outlier (10x)
	double MomCAtUpload = 1;  // the reach Momentum gave it at upload (the launch card: "Riding your breakout" while > 1.1)
	int LiveDropAvg = 0;      // Live Drop: the stream's own average its push was sized by ("212 watching"); 0 none
	bool LiveDrop = false;    // a stream within 6 hours of the publish (V +0.10 at the verdict), with or without a push
	bool Viral = false;       // a narrated viral roll ("is breaking out")
	bool LateBloom = false;   // Late Bloomer woke it up
	int Binged = 0;           // Binge waves it took part in
	int DuelWin = -1;         // the Duel's winner (0 A, 1 B, 2 C), -1 none yet
};
/** The channel over a window, a value a day (the rings, and the closed form for today). */
struct Range
{
	int Days = 0;
	std::vector<double> Views, WatchH, SubsGained, SubsLost, Likes, Dislikes, Impressions, Cents;
	std::vector<bool> Estimated;
};
/** Audience (display only: none of it enters money, W5). */
struct AudienceView
{
	std::array<std::array<float, 24>, 7> Heat{};
	double Returning = 0, BellShare = 0, SubsWatchShare = 0;
	std::vector<std::pair<std::string, double>> Geo;
	std::array<double, 5> Ages{};
	std::array<double, 4> Devices{};
};
/** Realtime: the last 48 hours and the last 60 minutes. */
struct RealtimeView
{
	std::array<double, 48> Hours{};
	std::array<double, 60> Minutes{};
	double SubsNow = 0;
	std::vector<std::pair<std::string, double>> Top;
	std::vector<std::string> OnHomeNow;
	// core-videoalgo, appended (SPEC_CREATORS §3.2, R1): a breakout's note for two days from it (a narrated viral roll from
	// its verdict midnight: "... is breaking out"), the newest first.
	std::vector<std::string> Notes;
};
/** Channel health, 0-5 bars each. */
struct Health
{
	int Momentum = 0, Packaging = 0, Holding = 0, Satisfaction = 0, Consistency = 0, Energy = 0;
};
/** A trend on the Research page (§4.6.10). */
struct Trend
{
	std::string Key, Label;
	double Heat = 0, Strength = 1;
	int Mine = 0, Others = 0;
	bool Limited = false, Strike = false;
};
SHORTSTACKCORE_API VideoStats StatsOf(const Channel& C, const Video& V, double World);
SHORTSTACKCORE_API Range StatsRange(const Channel& C, double World, int Days);
SHORTSTACKCORE_API AudienceView AudienceOf(const Channel& C, double World);
SHORTSTACKCORE_API RealtimeView Realtime(const Channel& C, double World);
SHORTSTACKCORE_API Health HealthOf(const Channel& C, double World);
SHORTSTACKCORE_API std::vector<Trend> Trends(const life::State& L, const world::World& W, double World);
/** core-videoalgo, appended: every line Rerun 2.0's core says (the bell, "What Rerun did with it", Insights, the health
 *  meters' words, the trends), "<kind>\t<text>" as video::Inventory()'s, for the copy lint and the trademark screen. */
SHORTSTACKCORE_API std::vector<std::string> InventoryAlgo();
/** core-videoalgo, appended: Channel health's words for a meter's bars (§5.9: "on a roll · steady · cooling off"). */
SHORTSTACKCORE_API std::string HealthWords(int Meter, int Bars);
/** core-videoalgo, appended (§4.5.8, the Ad market card): this world day's CPM by advertiser category (BaseCPM x the
 *  category's fit x the season), and the channel's category mix over its last 10 uploads (shares that sum to 1). */
struct AdMarketRow
{
	std::string Category;
	double Cpm = 0, Fit = 1, Mix = 0;
};
SHORTSTACKCORE_API std::vector<AdMarketRow> AdMarket(const Channel& C, double World);
} // namespace video
} // namespace ss
