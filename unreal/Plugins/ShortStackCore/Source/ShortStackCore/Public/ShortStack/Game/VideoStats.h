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
} // namespace video
} // namespace ss
