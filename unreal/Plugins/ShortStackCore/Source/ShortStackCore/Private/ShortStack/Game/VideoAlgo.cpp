// Update 3 (SPEC §4.4-§4.8, §7.3): Rerun's recommender and its money, core-videoalgo's file. Kappa, the compression of
// Update 3's levers, the season, the CPM to RPM identity, the score, the verdict and the check-ins at world midnights,
// Momentum, burnout from extra editing, repackaging, Kast as a launch action. Phase A, day 1: neutral stubs (every lever
// at 1, nothing decided).
#include "ShortStack/Game/VideoAlgo.h"

#include "ShortStack/Game/Life.h"

#include <vector>

namespace ss
{
namespace video
{
namespace algo
{
double Kappa(double /*S*/)
{
	return 0.0;
}

double BreakoutChance(double /*S*/)
{
	return 0.0;
}

double KappaRegular(double /*S*/)
{
	return 0.0;
}

double Compress(double M, double /*Lmax*/)
{
	return M;
}

double Season(int /*WorldDay*/)
{
	return 1.0;
}

Money MoneyOf(const Video& /*V*/, int /*WorldDay*/)
{
	return Money();
}

double AdsPerPlayback(Format /*F*/, LengthOpt /*L*/, AdLoad /*A*/)
{
	return 0.0;
}

Score ScoreAt(const Video& /*V*/, const Channel& /*C*/, double /*World*/, int /*Stage*/)
{
	return Score();
}

double ScoreBase(const Video& /*V*/, const Channel& /*C*/)
{
	return 0.0;
}

int8_t VerdictLabel(double /*Kick*/, double /*P0*/, Class /*K*/)
{
	return -1;
}

void DecideDay(Channel& /*C*/, life::State& /*L*/, int /*Day*/, const std::vector<Trend>* /*Week*/)
{
}

double PeakHour(const Channel& /*C*/, int /*Weekday*/)
{
	return 21.0;
}

Comp RepackComp(const Video& /*V*/, double /*T0*/, double /*NewCtr*/)
{
	return Comp();
}

double Momentum(const Channel& /*C*/, double /*World*/)
{
	return 1.0;
}

double ExtraHours7(const Channel& /*C*/, double /*World*/)
{
	return 0.0;
}

void NoteStream(Channel& /*C*/, double /*Start*/, double /*End*/, int /*PeakViewers*/)
{
}
} // namespace algo
} // namespace video
} // namespace ss
