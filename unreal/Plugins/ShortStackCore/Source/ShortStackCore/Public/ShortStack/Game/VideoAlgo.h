#pragma once

#include "ShortStack/Common.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoStats.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace ss
{
namespace life
{
struct State;
}

/**
 * Update 3 (SPEC §4.4-§4.8, §7.3): Rerun's recommender and its money. The score S and kappa, the compression of Update 3's
 * levers (Update 2's never move, W8), the season, the CPM to RPM identity, the verdict and the check-ins decided at world
 * midnights (two-phase), Momentum, burnout from Update 3's extra editing, repackaging and Kast as a launch action. Closed
 * form forever (V1). Declared by the lead in Phase A; bodies by core-videoalgo (VideoAlgo.cpp, G3).
 */
namespace video
{
namespace algo
{
/** clamp(0.35 e^S, 0.07, 0.875). */
SHORTSTACKCORE_API double Kappa(double S);
/** min(0.03, 0.004 e^(1.2 S)). */
SHORTSTACKCORE_API double BreakoutChance(double S);
/** max(0, Kappa - 5.5 x BreakoutChance). */
SHORTSTACKCORE_API double KappaRegular(double S);
/** exp(ln M / (1 + |ln M| / Lmax)). */
SHORTSTACKCORE_API double Compress(double M, double Lmax);
/** The advertisers' calendar: each civil year's mean exactly 1. */
SHORTSTACKCORE_API double Season(int WorldDay);
/** The money identity on one day, at that day's ad load: RPM = CPM x MonetizedRate x AdsPerPlayback x Share. */
struct Money
{
	double Cpm = 0, MonetizedRate = 0, AdsPerPlayback = 0, Share = 0.55, RetentionPays = 1, Rpm = 0;
};
SHORTSTACKCORE_API Money MoneyOf(const Video& V, int WorldDay);
SHORTSTACKCORE_API double AdsPerPlayback(Format F, LengthOpt L, AdLoad A);
/** The score's parts: click-through, satisfaction, velocity, trend, series, and S. */
struct Score
{
	double Ctr = 0, Q = 0, V = 0, Tr = 0, Se = 0, S = 0;
};
/** Stage: 0 the upload, 1 the verdict, 2 and up the check-ins. */
SHORTSTACKCORE_API Score ScoreAt(const Video& V, const Channel& C, double World, int Stage);
/** S0_base: the same upload with every Update 3 lever neutral. */
SHORTSTACKCORE_API double ScoreBase(const Video& V, const Channel& C);
/**
 * The creator update (SPEC_CREATORS §3.2): the five verdicts (Not pushed .. Pushed hard) read how the video did, from
 * rp = Perf + 0.5 (S - S0Base): < -0.8 Not pushed, < -0.1 Held, < 0.6 Pushed, < 1.2 Pushed more, else Pushed hard. Replaces
 * v1.2's VerdictLabel(Kick, P0, Class), which nothing called.
 */
SHORTSTACKCORE_API int8_t VerdictLabel(double Perf, double S, double S0Base);
/** A world midnight's decisions, two-phase (§4.6.3): every eligible video scored, then the components appended. */
SHORTSTACKCORE_API void DecideDay(Channel& C, life::State& L, int Day, const std::vector<Trend>* Week);
SHORTSTACKCORE_API double PeakHour(const Channel& C, int Weekday);
SHORTSTACKCORE_API Comp RepackComp(const Video& V, double T0, double NewCtr);
/** The channel's standing with the recommender (reads S - S0_base). */
SHORTSTACKCORE_API double Momentum(const Channel& C, double World);
/** Update 3's extra editing over the last 7 days (§4.6.6): never an upload's Update 2 edit. */
SHORTSTACKCORE_API double ExtraHours7(const Channel& C, double World);
/** A finished Kast stream (Session::EndStream): a launch action when it was within 6 hours of a publish. The creator update
 *  (SPEC_CREATORS §3.4, Live Drop): AvgOwn, the stream's average without raiders and the Broadcast, sizes the push, capped
 *  at 1.5 x Norm, the channel's usual average before tonight (kast::Summary::NormBefore: Stop has already moved
 *  kast::Channel::Norm); both 0 until the Room fills them. */
SHORTSTACKCORE_API void NoteStream(Channel& C, double Start, double End, int PeakViewers, double AvgOwn = 0.0, double Norm = 0.0);
/** The creator update (SPEC_CREATORS §3.3): the Kast Heat Rerun earned since Since ("Pushed hard" +6, a breakout or a narrated
 *  viral +10), each bump with its world minute, oldest first; Kast caps and decays them when it applies them at Start. */
SHORTSTACKCORE_API std::vector<std::pair<double, double>> KastHeatSince(const Channel& C, double Since);
} // namespace algo
} // namespace video
} // namespace ss
