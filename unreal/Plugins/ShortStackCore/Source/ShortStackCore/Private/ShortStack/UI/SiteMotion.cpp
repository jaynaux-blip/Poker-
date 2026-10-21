// Update 3 (SPEC §1.11, §7.3): the motion kit, the lead's file. Closed forms of Now - At (real seconds): the same clock
// always gives the same frame, a settled clock (the Kast preview's) gives the still, and nothing here decides an outcome.
// Nothing drawn sets the canvas's alpha: Confetti's pieces carry their own, which the parent's multiplies.
#include "ShortStack/UI/SiteMotion.h"
#include "../StrictFloat.h"

#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ss
{
namespace ui
{
namespace sitemotion_detail
{
/** A well-mixed 32-bit hash (a piece's path, a shake's direction): the same Id and salt always give the same value. */
uint32_t MotionHash(uint32_t Id, uint32_t Salt)
{
	uint32_t H = Id * 0x9E3779B1u ^ (Salt + 0x7F4A7C15u + (Id << 6) + (Id >> 2));
	H ^= H >> 16;
	H *= 0x7FEB352Du;
	H ^= H >> 15;
	H *= 0x846CA68Bu;
	H ^= H >> 16;
	return H;
}

/** MotionHash as a number in [0, 1). */
double MotionUnit(uint32_t Id, uint32_t Salt)
{
	return static_cast<double>(MotionHash(Id, Salt) >> 8) / 16777216.0;
}

/** Confetti's run: pieces fall for this long after At, and fade over the last part of it. */
constexpr double MotionConfettiSeconds = 2.6;
constexpr double MotionConfettiFade = 0.6;
/** The spring's natural frequency (rad/s) and damping: about 10% overshoot, settled in half a second. */
constexpr double MotionSpringOmega = 14.0;
constexpr double MotionSpringZeta = 0.6;
/** Past this the spring is exactly 1 (a still never wobbles). */
constexpr double MotionSpringDone = 1.5;
} // namespace sitemotion_detail

namespace motion
{
double CountUp(double From, double To, double Now, double At, double Seconds)
{
	if (Seconds <= 0.0 || Now - At >= Seconds)
	{
		return To;
	}
	if (Now <= At)
	{
		return From;
	}
	const double T = (Now - At) / Seconds;
	const double U = 1.0 - T;
	return From + (To - From) * (1.0 - U * U * U);
}

int Stage(double Now, double At, const std::vector<double>& Marks)
{
	int Passed = 0;
	for (const double M : Marks)
	{
		Passed += Now - At >= M ? 1 : 0;
	}
	return Passed;
}

double Spring(double Now, double At)
{
	const double T = Now - At;
	if (T <= 0.0)
	{
		return 0.0;
	}
	if (T >= sitemotion_detail::MotionSpringDone)
	{
		return 1.0;
	}
	const double W = sitemotion_detail::MotionSpringOmega;
	const double Z = sitemotion_detail::MotionSpringZeta;
	const double Wd = W * std::sqrt(1.0 - Z * Z);
	return 1.0 - std::exp(-Z * W * T) * (std::cos(Wd * T) + Z * W / Wd * std::sin(Wd * T));
}

Vec2 Shake(double Now, double At, uint32_t Id)
{
	// About 8 units at first, 18 swings a second, gone in half a second; its direction is the Id's.
	const double T = Now - At;
	if (T <= 0.0 || T >= 0.5)
	{
		return Vec2();
	}
	const double Angle = sitemotion_detail::MotionUnit(Id, 1u) * 2.0 * 3.14159265358979;
	const double Amp = 8.0 * std::exp(-T / 0.12) * std::sin(2.0 * 3.14159265358979 * 18.0 * T);
	return {static_cast<float>(Amp * std::cos(Angle)), static_cast<float>(Amp * std::sin(Angle))};
}

void Confetti(Canvas& C, uint32_t Id, double At, double Now, int N, Rect R)
{
	using namespace sitemotion_detail;
	const double T = Now - At;
	if (T <= 0.0 || T >= MotionConfettiSeconds || N <= 0 || R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	// The site's own colours (the client's SkinScope holds them while its frame draws).
	const Color Tints[6] = {pal::Accent, pal::Gold, pal::Accent2, pal::Red, pal::Green, pal::Orange};
	const double Fade = std::min(1.0, (MotionConfettiSeconds - T) / MotionConfettiFade);
	const int Pieces = std::min(N, 200);
	for (int I = 0; I < Pieces; ++I)
	{
		const uint32_t K = MotionHash(Id, static_cast<uint32_t>(I) * 8u);
		// Each piece's path: thrown up from the rect's lower middle, falling back under gravity, swaying as it turns.
		const double X0 = R.X + R.W * (0.2 + 0.6 * MotionUnit(K, 1u));
		const double Vx = (MotionUnit(K, 2u) - 0.5) * R.W * 0.9;
		const double Vy = -(0.55 + 0.45 * MotionUnit(K, 3u)) * R.H * 1.3;
		const double G = R.H * 1.9;
		const double X = X0 + Vx * T + std::sin(T * (3.0 + 4.0 * MotionUnit(K, 4u)) + MotionUnit(K, 5u) * 6.28) * 10.0;
		const double Y = R.Y + R.H * 0.85 + Vy * T + 0.5 * G * T * T;
		if (Y < R.Y - 12.0 || Y > R.Y + R.H + 12.0)
		{
			continue;
		}
		const double Spin = MotionUnit(K, 6u) * 6.28 + T * (4.0 + 8.0 * MotionUnit(K, 7u));
		const float Hw = 4.0f;
		const float Hh = static_cast<float>(2.5 * std::fabs(std::cos(T * 7.0 + MotionUnit(K, 8u) * 6.28)) + 0.6);
		const float Cs = static_cast<float>(std::cos(Spin));
		const float Sn = static_cast<float>(std::sin(Spin));
		const float Px = static_cast<float>(X);
		const float Py = static_cast<float>(Y);
		Color Tint = Tints[MotionHash(K, 9u) % 6u];
		Tint.A *= static_cast<float>(Fade);
		C.FillPolygon({{Px - Hw * Cs + Hh * Sn, Py - Hw * Sn - Hh * Cs}, {Px + Hw * Cs + Hh * Sn, Py + Hw * Sn - Hh * Cs}, {Px + Hw * Cs - Hh * Sn, Py + Hw * Sn + Hh * Cs},
						  {Px - Hw * Cs - Hh * Sn, Py - Hw * Sn + Hh * Cs}},
			Tint);
	}
}

int Flap(double Now, double At, int Chars)
{
	// A row's first character settles 0.25 s after At, each next one 50 ms later.
	const double T = Now - At;
	if (Chars <= 0 || T < 0.25)
	{
		return 0;
	}
	return std::min(Chars, static_cast<int>(std::floor((T - 0.25) / 0.05)) + 1);
}

double Chase(double Now, int I, int N, double Bpm)
{
	// Every third bulb lit, the pattern stepping one bulb a beat; a newly lit bulb comes up over the first fifth of its beat.
	if (N <= 0 || Bpm <= 0.0)
	{
		return 1.0;
	}
	const double Beats = Now * Bpm / 60.0;
	const long long Step = static_cast<long long>(std::floor(Beats));
	const double Into = Beats - static_cast<double>(Step);
	const long long D = ((static_cast<long long>(I) - Step) % N + N) % N;
	if (D % 3 != 0)
	{
		return 0.25;
	}
	return 0.25 + 0.75 * std::min(1.0, Into / 0.2);
}

double Beat(double Now, double Bpm)
{
	if (Bpm <= 0.0)
	{
		return 0.0;
	}
	const double Beats = Now * Bpm / 60.0;
	return Beats - std::floor(Beats);
}
} // namespace motion
} // namespace ui
} // namespace ss
