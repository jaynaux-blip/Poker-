// The creator update (SPEC_CREATORS §2, §8.1): Kast's growth model as pure functions (KastGrowth.h), core-kastmath's file.
// Every term of §2 with the constants of probes/synth/synth_kast.py (the v1.1 values), the keyed draws, Heat's closed form,
// the forecast, the payout split, the sponsor read rule and the save lines over a SaveView. Nothing here draws from an Rng,
// reads a clock or keeps state: the Room (Kast.cpp) and the chat games (KastPlay.cpp) call these with the numbers they hold.
//
// Notes for the callers (each is also in STATUS_core-kastmath.txt):
//   - Unit is splitmix64's finaliser chained over (Seed, StreamNo, Tag, A, B): consecutive slots and minutes decorrelate.
//   - Gain's weekly cap: the night's positive gain counts toward GainUsed before the soft ceiling, as synth_kast.py does:
//     GainUsed += min(Raw, max(0, 35 - WeekUsed)) when Raw > 0 (and Gain is what Heat moves by).
//   - ShowUp and the weeks: a world week is net::DayOf / 7 (day 0 is a Monday); ShowDays holds weekday bits (bit 0 Monday).
//   - HeatShown: LastStreamDay < 0 is a channel that never streamed: it reads its start (26) until its first stream, so a new
//     channel's first go-live is Warming up however long the career waited. A migrated channel's glue sets LastStreamDay.
//   - GiveHazard is a tip's hazard (TipChance spread over the visit's expected minutes, x2.5 in the 6 world minutes after a
//     big moment the member is present for, x0.75 otherwise; kastgrowth_detail::KgSpread). The Room doesn't call it: it
//     spreads tips, cheers and subs with its own per-night weight (RoomGiveWeight, S-K finding 1), so the shape is file-local.
//   - Constants: synth_kast.py's, with S-K's §6.5 step-2 knob turn (the C-K gate applied core-kastroom_to_core-kastmath_1.diff;
//     the model carries it since the gate's L12): KgStaminaK 0.03 [v1.1 0.025], KgNoveltyK 16 [20], KgBudgetAt 8 [10].
//   - SponsorReadPays: ReadAt is the deal's last *paid* read; a deal whose ReadsAsk is 0 (Kast's old terms, migrated from
//     kast\tdeal) keeps one paid read a world day with no 1.5x cap (the C-K gate's M1, the lead's decision).
//   - The member tail writes its two days as days before the member's LastSeen day (as written: Fixed(LastSeen, 2)), so the
//     widest save stays inside 180 KiB (STATUS): "\t0\t999\t0" for a member who gave and watched on the day they were last seen.
#include "ShortStack/Game/KastGrowth.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Network.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>
#include <vector>

namespace ss
{
namespace kast
{
// The file's private helpers (Unreal's unity builds paste files together: every name here is prefixed).
namespace kastgrowth_detail
{
constexpr double KgDay = 1440.0;
// The Ping and presence (§2.3).
constexpr double KgPingBase = 0.03, KgPingL = 0.75, KgPingExp = 1.15, KgPingCap = 0.95;
constexpr double KgHoney = 0.15, KgHoneyTau = 7.0;
constexpr double KgAppetite = 0.5, KgAppetiteTau = 2.0;
constexpr double KgBudgetAt = 8.0, KgBudgetTau = 10.0, KgHwTau = 7.0;
constexpr double KgSmBase = 15.0, KgSmL = 130.0, KgGBase = 30.0, KgGL = 90.0, KgGDeep = 4.0;
constexpr double KgFriendSm = 95.0, KgFriendSmEarly = 400.0, KgFriendG = 40.0;
constexpr double KgModSm = 1.5, KgModG = 0.5, KgResultsG = 1.5;
constexpr double KgSlot = 10.0;
constexpr double KgDeeAt = 0.25, KgMeiLo = 0.1, KgMeiHi = 0.5, KgFriendLo = 0.3, KgFriendHi = 2.0;
// The deepest run (§2.6).
constexpr int KgFieldMin = 100;
constexpr double KgSweatX = 4.0, KgNovK = 3.0, KgRailX = 30.0, KgRailNorm = 0.2, KgRailDepth = 0.3, KgRailCallDepth = 0.4;
constexpr double KgDeepFollow = 0.3, KgLurkDeep = 2.0, KgBroadcastAt = 1.0, KgBroadcastK = 20.0, KgBroadcastCap = 2.5;
// Heat (§2.4).
constexpr double KgHeatFloor = 8.0, KgHeatStart = 26.0, KgHeatMax = 100.0, KgHeatHalf = 5.0 * KgDay, KgHeatShownFloor = 20.0;
constexpr double KgHeatPerf = 26.0, KgHeatNews = 35.0, KgHeatLo = -9.0, KgHeatHi = 25.0, KgHeatNewsCap = 25.0;
constexpr double KgPerfLo = -0.4, KgPerfHi = 1.2, KgWeekCap = 35.0, KgSoftTop = 90.0, KgSoftSpan = 50.0;
constexpr double KgShow = 3.0;
constexpr int KgShowWeek = 4;
constexpr double KgHeatRef = 0.2, KgHeatS1 = 1.5, KgHeatA1 = 1.4, KgHeatEffFloor = 0.8;
constexpr double KgFrontAt = 55.0, KgFrontP = 0.30;
// Discovery (§2.5).
constexpr double KgLaneX = 0.8, KgLaneN = 12.0, KgRisingX = 1.0, KgRisingN = 15.0, KgPond = 600.0;
constexpr double KgNoveltyAt = 20.0, KgNoveltyK = 16.0, KgTaperAt = 240.0, KgTaperTau = 300.0, KgTaperFloor = 0.25;
constexpr double KgTideSd = 0.12, KgTideSeries = 1.3;
constexpr double KgHd0 = 0.45, KgHd1 = 1.83, KgSat = 3000.0, KgSatExp = 0.65;
constexpr double KgStaminaAt = 25.0, KgStaminaK = 0.03, KgStaminaFloor = 0.60;
// Raids, clips, the community (§2.7, §2.8).
constexpr double KgClipBase = 30.0, KgClipV = 4.0;
constexpr double KgLurkUntracked = 0.006, KgLurkPing = 0.003;
// Engagement (§2.9).
constexpr double KgEngageTau = 6.9, KgThank = 0.25, KgThankTaper = 0.6;
constexpr double KgAnsQ = 0.0197, KgAnsCap = 0.39, KgAnsTeach = 1.6;
// Money (§2.10). EngT and EngS are 1.0 at an attended stream's mean Engage (0.30): synth_kast.py's 2.3333 and 1.3333.
constexpr double KgEngCap = 0.30, KgEngT0 = 0.3, KgEngT1 = 7.0 / 3.0, KgEngS0 = 0.6, KgEngS1 = 4.0 / 3.0;
constexpr double KgDilute = 90.0, KgGiveTau = 21.0, KgTipK = 0.0184, KgCheerK = 0.020, KgSubBase = 0.0025, KgSubL2 = 0.035;
constexpr double KgRenew0 = 0.40, KgRenew1 = 0.45, KgGiftRenew0 = 0.2, KgGiftRenew1 = 0.5;
constexpr double KgReadMinutes = 30.0;
// The forecast (§2.11).
constexpr double KgBandSd = 0.85, KgBandX = 0.10;

// The normal quantile's coefficients (Acklam).
constexpr double KgNa[6] = {-3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00};
constexpr double KgNb[5] = {-5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01};
constexpr double KgNc[6] = {-7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00, -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00};
constexpr double KgNd[4] = {7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00, 3.754408661907416e+00};
constexpr double KgNormLow = 0.02425;

/** splitmix64's finaliser (Session::WorldSeed's). */
uint64_t KgMix(uint64_t Z)
{
	Z += 0x9e3779b97f4a7c15ull;
	Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ull;
	Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebull;
	return Z ^ (Z >> 31);
}

double KgClamp(double V, double Lo, double Hi)
{
	return std::min(Hi, std::max(Lo, V));
}

int KgWeekday(int Day)
{
	return ((Day % 7) + 7) % 7;
}

int KgWeekOf(int Day)
{
	return static_cast<int>(std::floor(static_cast<double>(Day) / 7.0));
}

int KgBits(uint32_t Mask)
{
	int N = 0;
	for (; Mask != 0; Mask &= Mask - 1)
	{
		++N;
	}
	return N;
}

/** The stage's own number in a table (RunStage's order). */
size_t KgStage(growth::RunStage S)
{
	return static_cast<size_t>(std::max(0, std::min(5, static_cast<int>(S))));
}

// ---- the save lines' text (the Session's own formats: Fixed decimals, integers, tabs)

std::string KgNum(double V, int Digits)
{
	return Fixed(V, Digits);
}

std::string KgInt(long long V)
{
	return std::to_string(V);
}

double KgReadNum(const std::vector<std::string>& P, size_t I, double Else)
{
	return I < P.size() && !P[I].empty() ? std::strtod(P[I].c_str(), nullptr) : Else;
}

long long KgReadInt(const std::vector<std::string>& P, size_t I, long long Else)
{
	return I < P.size() && !P[I].empty() ? std::strtoll(P[I].c_str(), nullptr, 10) : Else;
}

/** A member's LastSeen day as the save holds it (its text, Fixed(LastSeen, 2), read back): the base of the tail's two days. */
int KgSeenDay(double LastSeen)
{
	return net::DayOf(std::strtod(Fixed(LastSeen, 2).c_str(), nullptr));
}

/** RerunHeat's field: "minute,amount;minute,amount" (at most 4), "" when there are none. */
std::string KgRerunText(const std::vector<std::pair<double, double>>& Bumps)
{
	std::string Out;
	for (size_t I = 0; I < Bumps.size() && I < 4; ++I)
	{
		Out += (I == 0 ? "" : ";") + Fixed(Bumps[I].first, 2) + "," + Fixed(Bumps[I].second, 1);
	}
	return Out;
}

std::vector<std::pair<double, double>> KgRerunRead(const std::string& Text)
{
	std::vector<std::pair<double, double>> Out;
	size_t At = 0;
	while (At < Text.size() && Out.size() < 4)
	{
		const size_t End = std::min(Text.find(';', At), Text.size());
		const std::string Pair = Text.substr(At, End - At);
		const size_t Comma = Pair.find(',');
		if (Comma != std::string::npos)
		{
			Out.emplace_back(std::strtod(Pair.substr(0, Comma).c_str(), nullptr), std::strtod(Pair.substr(Comma + 1).c_str(), nullptr));
		}
		At = End + 1;
	}
	return Out;
}

/** The number of fields kast\tmo's version 1 has after "kast\tmo" (Firsts, the last, may be missing on an early writer). */
constexpr size_t KgMoFields = 39;

/** A visit chance as a hazard a present minute: the chance over the visit's expected minutes, bunched in the 6 world minutes
 *  after a big moment. GiveHazard's tip case only: the Room spreads its giving itself (Kast.cpp's RoomGiveWeight, S-K finding
 *  1), so the shape stays file-local (the C-K gate left core-kastmath_to_lead_1.diff unapplied). */
double KgSpread(double Chance, double Minutes, bool NearBigMoment)
{
	// About 40% of gifts land in the 6 world minutes after a big moment (x2.5 there, x0.75 elsewhere: a night with a big
	// moment every 40 world minutes keeps the visit's chance whole).
	constexpr double KgBunchNear = 2.5, KgBunchFar = 0.75;
	return std::max(0.0, Chance) / std::max(1.0, Minutes) * (NearBigMoment ? KgBunchNear : KgBunchFar);
}
} // namespace kastgrowth_detail

namespace growth
{
// ------------------------------------------------------------------ keyed draws

double Unit(uint64_t Seed, uint64_t StreamNo, Tag T, uint64_t A, uint64_t B)
{
	using namespace kastgrowth_detail;
	uint64_t H = KgMix(Seed);
	H = KgMix(H ^ StreamNo);
	H = KgMix(H ^ (0x6b61737400000000ull + static_cast<uint64_t>(T))); // "kast" and the tag
	H = KgMix(H ^ A);
	H = KgMix(H ^ B);
	return static_cast<double>(H >> 11) * 0x1.0p-53;
}

uint64_t NameKey(const std::string& Name)
{
	uint64_t H = 14695981039346656037ull;
	for (const char Ch : Name)
	{
		H ^= static_cast<uint64_t>(static_cast<unsigned char>(Ch));
		H *= 1099511628211ull;
	}
	return H;
}

double ExpAt(double Mean, double U)
{
	if (Mean <= 0.0)
	{
		return 0.0;
	}
	const double Below = std::min(std::max(U, 0.0), 1.0 - 0x1.0p-53);
	return -Mean * std::log(1.0 - Below);
}

int PoissonAt(double Mean, double U)
{
	if (!(Mean > 0.0))
	{
		return 0;
	}
	if (Mean >= 64.0)
	{
		// A big channel's minute: the normal approximation (continuity-corrected), never negative.
		return std::max(0, static_cast<int>(std::floor(Mean + std::sqrt(Mean) * NormalAt(U) + 0.5)));
	}
	double P = std::exp(-Mean);
	double Cdf = P;
	int K = 0;
	while (U >= Cdf && K < 1000)
	{
		++K;
		P *= Mean / static_cast<double>(K);
		Cdf += P;
		if (P <= 0.0)
		{
			break;
		}
	}
	return K;
}

double NormalAt(double U)
{
	// Acklam's rational approximation of the standard normal quantile (relative error under 1.2e-9).
	using namespace kastgrowth_detail;
	const double Q = std::min(std::max(U, 1e-12), 1.0 - 1e-12);
	if (Q < KgNormLow)
	{
		const double S = std::sqrt(-2.0 * std::log(Q));
		return (((((KgNc[0] * S + KgNc[1]) * S + KgNc[2]) * S + KgNc[3]) * S + KgNc[4]) * S + KgNc[5]) / ((((KgNd[0] * S + KgNd[1]) * S + KgNd[2]) * S + KgNd[3]) * S + 1.0);
	}
	if (Q > 1.0 - KgNormLow)
	{
		const double S = std::sqrt(-2.0 * std::log(1.0 - Q));
		return -(((((KgNc[0] * S + KgNc[1]) * S + KgNc[2]) * S + KgNc[3]) * S + KgNc[4]) * S + KgNc[5]) / ((((KgNd[0] * S + KgNd[1]) * S + KgNd[2]) * S + KgNd[3]) * S + 1.0);
	}
	const double S = Q - 0.5;
	const double R2 = S * S;
	return (((((KgNa[0] * R2 + KgNa[1]) * R2 + KgNa[2]) * R2 + KgNa[3]) * R2 + KgNa[4]) * R2 + KgNa[5]) * S /
		(((((KgNb[0] * R2 + KgNb[1]) * R2 + KgNb[2]) * R2 + KgNb[3]) * R2 + KgNb[4]) * R2 + 1.0);
}

// ------------------------------------------------------------------ the Ping and drop-in presence

double PingW(double DaysSince, double L)
{
	if (L >= SuperfanLoyalty)
	{
		return 1.0;
	}
	return DaysSince <= 7.0 ? 1.0 : DaysSince <= 21.0 ? 0.7 : DaysSince <= 60.0 ? 0.35 : 0.12;
}

double PingWHot(double W, double H)
{
	return W + (1.0 - W) * 0.5 * kastgrowth_detail::KgClamp(H, 0.0, 1.0);
}

double Honey(double DaysSinceFollow)
{
	using namespace kastgrowth_detail;
	return KgHoney * std::exp(-std::max(0.0, DaysSinceFollow) / KgHoneyTau);
}

double Appetite(double DaysSince)
{
	using namespace kastgrowth_detail;
	return 1.0 - KgAppetite * std::exp(-std::max(0.0, DaysSince) / KgAppetiteTau);
}

double Budget(double Hw)
{
	using namespace kastgrowth_detail;
	return std::exp(-std::max(0.0, Hw - KgBudgetAt) / KgBudgetTau);
}

double HwToday(double Hw, int HwDay, int Today)
{
	using namespace kastgrowth_detail;
	if (HwDay < 0 || Today <= HwDay)
	{
		return Hw;
	}
	return Hw * std::exp(-static_cast<double>(Today - HwDay) / KgHwTau);
}

double Pa(const PingTerms& T)
{
	using namespace kastgrowth_detail;
	const double Base = KgPingBase + KgPingL * std::pow(std::max(0.0, T.L), KgPingExp) + std::max(0.0, T.Honey);
	const double P = Base * T.PingWHot * T.HeatA * T.Appetite * T.Budget * T.WelcomeBack * T.Sched * T.ToD * T.Recent;
	return KgClamp(P, 0.0, KgPingCap);
}

double FriendPa(int Streams, double Sched, double ToD)
{
	using namespace kastgrowth_detail;
	if (Streams <= 1)
	{
		return 1.0; // First Faces: both friends come to the first stream
	}
	if (Streams <= 10)
	{
		return 0.9;
	}
	return KgClamp(0.6 * Sched * ToD, 0.0, KgPingCap);
}

double FriendArrive(int Streams, bool Second, double DeeAt, double U)
{
	using namespace kastgrowth_detail;
	const double Q = KgClamp(U, 0.0, 1.0);
	if (Streams <= 1)
	{
		return Second ? DeeAt + KgMeiLo + (KgMeiHi - KgMeiLo) * Q : KgDeeAt;
	}
	if (Streams <= 5)
	{
		return KgFriendLo + (KgFriendHi - KgFriendLo) * Q;
	}
	return -1.0; // not First Faces: a friend arrives like everyone else (ArriveDelay)
}

double ArriveDelay(bool OnSchedule, double U)
{
	return ExpAt(OnSchedule ? 3.0 : 6.0, U);
}

double StayMean(double L, bool Friend, int Streams, bool Mod)
{
	using namespace kastgrowth_detail;
	const double Sm = Friend ? (Streams <= 5 ? KgFriendSmEarly : KgFriendSm) : KgSmBase + KgSmL * KgClamp(L, 0.0, 1.0);
	return Mod ? Sm * KgModSm : Sm;
}

double GapMean(double L, bool Friend, bool Mod, bool Results, double DeepPull)
{
	using namespace kastgrowth_detail;
	double G = Friend ? KgFriendG : KgGBase + KgGL * (1.0 - KgClamp(L, 0.0, 1.0));
	G /= 1.0 + KgGDeep * std::max(0.0, DeepPull);
	G *= Mod ? KgModG : 1.0;
	G *= Results ? KgResultsG : 1.0;
	return G;
}

double Pst2(double P, double RunDepth)
{
	return RunDepth > 0.0 ? std::max(P, 0.5) * RunDepth * RunDepth : 0.0;
}

bool SlotNext(bool Here, double Sm, double G, double U)
{
	using namespace kastgrowth_detail;
	if (Here)
	{
		const double Leave = Sm > 0.0 ? 1.0 - std::exp(-KgSlot / Sm) : 1.0;
		return !(U < Leave);
	}
	const double Back = G > 0.0 ? 1.0 - std::exp(-KgSlot / G) : 1.0;
	return U < Back;
}

double Presence(double Tau, double Sm, double G)
{
	if (Sm <= 0.0)
	{
		return 0.0;
	}
	if (G <= 0.0)
	{
		return 1.0;
	}
	const double S = Sm / (Sm + G);
	return S + (1.0 - S) * std::exp(-std::max(0.0, Tau) * (1.0 / Sm + 1.0 / G));
}

// ------------------------------------------------------------------ the deepest run, Run Alerts and Rail Call

double Prestige(int Entrants, Chips BuyIn, double Series)
{
	using namespace kastgrowth_detail;
	const double N = static_cast<double>(std::max(1, Entrants));
	const double B = static_cast<double>(std::max<Chips>(1, BuyIn));
	return KgClamp(0.35 * std::log2(N / 100.0) + 0.25 * std::log2(B / 550.0) + Series, 0.25, 4.0);
}

double Depth(int Entrants, int Remaining)
{
	using namespace kastgrowth_detail;
	if (Entrants <= 9)
	{
		return 0.0;
	}
	const double N = static_cast<double>(Entrants);
	const double Left = static_cast<double>(std::max(1, std::min(Entrants, Remaining)));
	return KgClamp(std::log(N / Left) / std::log(N / 9.0), 0.0, 1.15);
}

bool Armed(int Entrants)
{
	return Entrants >= kastgrowth_detail::KgFieldMin;
}

bool EventMode(int Entrants, int Remaining, int TableSize)
{
	return Armed(Entrants) && TableSize > 0 && Remaining >= 1 && Remaining <= 2 * TableSize;
}

double SweatNov(int Ft7)
{
	return 1.0 / (1.0 + static_cast<double>(std::max(0, Ft7 - 1)) / kastgrowth_detail::KgNovK);
}

double Sweat(double P, double RunDepth, double Nov)
{
	using namespace kastgrowth_detail;
	return RunDepth > 0.0 ? 1.0 + KgSweatX * std::max(0.0, P) * RunDepth * RunDepth * Nov : 1.0;
}

double Railbirds(double Norm, double P, double RunDepth, double Nov)
{
	using namespace kastgrowth_detail;
	if (RunDepth <= KgRailDepth)
	{
		return 0.0;
	}
	return (KgRailX + KgRailNorm * std::max(0.0, Norm)) * std::max(0.0, P) * RunDepth * RunDepth * RunDepth * Nov;
}

double LurkDeep(double P, double RunDepth)
{
	using namespace kastgrowth_detail;
	return RunDepth > 0.0 ? 1.0 + KgLurkDeep * RunDepth * RunDepth * KgClamp(P, 0.0, 2.0) : 1.0;
}

double DeepFollow(double P, double RunDepth)
{
	using namespace kastgrowth_detail;
	return 1.0 + KgDeepFollow * KgClamp(RunDepth, 0.0, 1.0) * KgClamp(P, 0.0, 2.0);
}

int BroadcastSize(double P)
{
	using namespace kastgrowth_detail;
	if (P < KgBroadcastAt)
	{
		return 0;
	}
	return static_cast<int>(std::lround(KgBroadcastK * std::pow(std::min(P, KgBroadcastCap), 1.5)));
}

double RunValue(RunStage S)
{
	static constexpr double Values[] = {0.0, 0.15, 0.3, 1.0, 1.3, 2.0};
	return Values[kastgrowth_detail::KgStage(S)];
}

double Pull(RunStage S)
{
	static constexpr double Pulls[] = {0.0, 0.25, 0.20, 0.50, 0.40, 0.0};
	return Pulls[kastgrowth_detail::KgStage(S)];
}

double AlertBack(double Share, double L)
{
	return std::max(0.0, Share) * (0.3 + 0.7 * kastgrowth_detail::KgClamp(L, 0.0, 1.0));
}

double AlertFollower(double Share, double W, double L, double P)
{
	using namespace kastgrowth_detail;
	return std::max(0.0, Share) * std::max(W, 0.35) * (0.05 + 0.6 * KgClamp(L, 0.0, 1.0)) * KgClamp(P, 0.0, 2.0) / 2.0;
}

bool RailCallOpen(int Entrants, double RunDepth)
{
	return Armed(Entrants) && RunDepth >= kastgrowth_detail::KgRailCallDepth;
}

double RailCallChance(double W, double L, double Sched, double ToD, double AppetiteNow, double BudgetNow)
{
	using namespace kastgrowth_detail;
	const double Base = KgPingBase + KgPingL * std::pow(KgClamp(L, 0.0, 1.0), KgPingExp);
	return KgClamp(0.5 * std::max(W, 0.35) * Base * Sched * ToD * AppetiteNow * BudgetNow, 0.0, 1.0);
}

// ------------------------------------------------------------------ Heat

double HeatAt(double Heat, double At, double Now)
{
	using namespace kastgrowth_detail;
	const double Raw = KgClamp(Heat, KgHeatFloor, KgHeatMax);
	return KgHeatFloor + (Raw - KgHeatFloor) * std::pow(0.5, std::max(0.0, Now - At) / KgHeatHalf);
}

double HeatShown(double Heat, double At, double Now, int LastStreamDay)
{
	using namespace kastgrowth_detail;
	const double X = HeatAt(Heat, At, Now);
	if (LastStreamDay < 0)
	{
		return std::max(KgHeatStart, X); // never streamed: a new channel reads its start
	}
	if (net::DayOf(Now) - LastStreamDay <= 7)
	{
		// "Cold means dark": a channel that streamed in the last 7 world days never reads Cold. By world day, through the 7th
		// day after the last stream's: a weekly streamer's go-live a week on, at any hour, still reads Warming up (§9.1).
		return std::max(KgHeatShownFloor, X);
	}
	return X;
}

double HeatH(double Shown)
{
	using namespace kastgrowth_detail;
	return KgClamp((Shown - KgHeatFloor) / (KgHeatMax - KgHeatFloor), 0.0, 1.0);
}

double HeatS(double H)
{
	using namespace kastgrowth_detail;
	return std::max(KgHeatEffFloor, 1.0 + KgHeatS1 * (H - KgHeatRef));
}

double HeatA(double H)
{
	using namespace kastgrowth_detail;
	return std::max(KgHeatEffFloor, 1.0 + KgHeatA1 * (H - KgHeatRef));
}

NightGain Gain(double Lr, double Bias, double Run, double RunNorm, double WeekUsed, double HeatShownNow)
{
	using namespace kastgrowth_detail;
	NightGain G;
	G.Perf = KgClamp(Lr - Bias, KgPerfLo, KgPerfHi);
	G.News = std::max(0.0, Run - RunNorm);
	G.Raw = KgClamp(KgHeatPerf * G.Perf, KgHeatLo, KgHeatHi) + std::min(KgHeatNewsCap, KgHeatNews * G.News);
	G.Gain = G.Raw;
	if (G.Gain > 0.0)
	{
		// The week's cap first (what counts toward GainUsed), then the soft ceiling near the top.
		G.Gain = std::min(G.Gain, std::max(0.0, KgWeekCap - WeekUsed));
		G.Gain *= KgClamp((KgSoftTop - HeatShownNow) / KgSoftSpan, 0.0, 1.0);
	}
	return G;
}

double ShowUp(int ShowWeek, int ShowDays, int Today)
{
	using namespace kastgrowth_detail;
	if (ShowWeek != KgWeekOf(Today))
	{
		return KgShow; // a new world week: nothing counted yet
	}
	const uint32_t Days = static_cast<uint32_t>(ShowDays) & 0x7fu;
	if ((Days >> KgWeekday(Today)) & 1u)
	{
		return 0.0; // today counted already
	}
	return KgBits(Days) < KgShowWeek ? KgShow : 0.0;
}

int Tier(double Shown)
{
	return Shown < 20.0 ? 0 : Shown < 35.0 ? 1 : Shown < 55.0 ? 2 : Shown < 75.0 ? 3 : 4;
}

bool FrontRow(double Shown, double U)
{
	using namespace kastgrowth_detail;
	return Shown >= KgFrontAt && U < KgFrontP;
}

// ------------------------------------------------------------------ discovery, stay and follow

double Disc(int Level, int Streak)
{
	return (1.0 + 0.03 * static_cast<double>(std::max(1, Level) - 1)) * (1.0 + 0.04 * static_cast<double>(std::max(0, std::min(Streak, 8))));
}

double Lane(int Streams)
{
	using namespace kastgrowth_detail;
	return 1.0 + KgLaneX * std::max(0.0, 1.0 - static_cast<double>(std::max(1, Streams) - 1) / KgLaneN);
}

double Rising(int Streams, double Norm)
{
	using namespace kastgrowth_detail;
	return Streams > 3 ? 1.0 + KgRisingX * std::max(0.0, 1.0 - std::max(0.0, Norm) / KgRisingN) : 1.0;
}

double Pond(double V)
{
	return std::pow(1.0 + std::max(0.0, V) / kastgrowth_detail::KgPond, -0.5);
}

double Novelty(double Hours7)
{
	using namespace kastgrowth_detail;
	return std::pow(1.0 + std::max(0.0, Hours7 - KgNoveltyAt) / KgNoveltyK, -0.5);
}

double Taper(double Uptime, double RunDepth)
{
	using namespace kastgrowth_detail;
	if (Uptime <= KgTaperAt || RunDepth > KgRailDepth)
	{
		return 1.0; // the first four world hours, and never during a deep run
	}
	return std::max(KgTaperFloor, std::exp(-(Uptime - KgTaperAt) / KgTaperTau));
}

double Tide(double U, bool SeriesWindow)
{
	using namespace kastgrowth_detail;
	return std::exp(KgTideSd * NormalAt(U)) * (SeriesWindow ? KgTideSeries : 1.0);
}

double Buzz(double Views48, double Norm)
{
	return 1.0 + 0.35 * std::min(1.0, std::max(0.0, Views48) / (2000.0 + 40.0 * std::max(0.0, Norm)));
}

double HD(double E)
{
	using namespace kastgrowth_detail;
	return std::min(1.0, KgHd0 + KgHd1 * std::max(0.0, E));
}

double Sat(int Followers)
{
	using namespace kastgrowth_detail;
	return std::pow(1.0 + static_cast<double>(std::max(0, Followers)) / KgSat, -KgSatExp);
}

double Stamina(double Hours7)
{
	using namespace kastgrowth_detail;
	return KgClamp(1.0 - KgStaminaK * std::max(0.0, Hours7 - KgStaminaAt), KgStaminaFloor, 1.0);
}

// ------------------------------------------------------------------ raids, clips, the community

double RaidRate(int Tokens, double DiscNow, double H)
{
	using namespace kastgrowth_detail;
	return (0.02 + 0.03 * std::min(10.0, 0.35 * static_cast<double>(std::max(0, Tokens)))) * std::min(2.0, std::max(0.0, DiscNow)) * (1.0 + KgClamp(H, 0.0, 1.0));
}

double RaidSize(double U1, double U2)
{
	using namespace kastgrowth_detail;
	return (0.5 + 0.4 * KgClamp(U1, 0.0, 1.0)) * (2.0 + std::pow(KgClamp(U2, 0.0, 1.0), 2.2) * 55.0);
}

bool EarnsToken(int Today, int LastRaidDay, double StreamMinutes)
{
	return StreamMinutes >= 60.0 && (LastRaidDay < 0 || Today - LastRaidDay >= 7);
}

double MomentRaidChance(RunStage S)
{
	return S == RunStage::Itm ? 0.05 : S == RunStage::FinalTable ? 0.15 : S == RunStage::Win ? 0.30 : 0.0;
}

double BigNameChance(double H, double P)
{
	return 0.10 + 0.30 * kastgrowth_detail::KgClamp(H, 0.0, 1.0) + (P >= 2.0 ? 0.15 : 0.0);
}

double RaiderStay(double U1, double U2, bool Welcomed)
{
	const double Mean = U1 < 0.4 ? 20.0 : 3.0;
	return ExpAt(Mean, U2) * (Welcomed ? 1.3 : 1.0);
}

double ClipReach(double V, double N, double H, double HypeHit)
{
	using namespace kastgrowth_detail;
	return (KgClipBase + KgClipV * std::max(0.0, V)) * std::exp(N) * (1.0 + KgClamp(H, 0.0, 1.0)) * (1.0 + std::max(0.0, HypeHit) / 50.0);
}

double AffinityBump(double Affinity, double ShareOfGap)
{
	using namespace kastgrowth_detail;
	const double A = KgClamp(Affinity, 0.0, 1.0);
	return KgClamp(A + KgClamp(ShareOfGap, 0.0, 1.0) * (1.0 - A), 0.0, 1.0);
}

double LurkerMean(int Untracked, double Sched, double ToD, double SumPingWAway, double Deep)
{
	using namespace kastgrowth_detail;
	const double FromUntracked = KgLurkUntracked * static_cast<double>(std::max(0, Untracked)) * Sched * (0.4 + 0.6 * ToD / 1.25);
	return std::max(0.0, (FromUntracked + KgLurkPing * std::max(0.0, SumPingWAway)) * Deep);
}

int Lurkers(double Mean, double U)
{
	return std::max(0, static_cast<int>(std::floor(std::max(0.0, Mean) + kastgrowth_detail::KgClamp(U, 0.0, 1.0))));
}

// ------------------------------------------------------------------ engagement

double EngageFade(double E, double DWorld)
{
	return E * std::exp(-std::max(0.0, DWorld) / kastgrowth_detail::KgEngageTau);
}

double ThankGain(int K)
{
	using namespace kastgrowth_detail;
	return KgThank * std::pow(KgThankTaper, static_cast<double>(std::max(0, K)));
}

double QuestionRate(double Strangers, double Present, double Raiders, bool Teaching)
{
	using namespace kastgrowth_detail;
	const double W = std::max(0.0, Strangers) + 0.4 * std::max(0.0, Present) + 0.5 * std::max(0.0, Raiders);
	return std::min(KgAnsCap, KgAnsQ * W) * (Teaching ? KgAnsTeach : 1.0);
}

// ------------------------------------------------------------------ money

double EngT(double E)
{
	using namespace kastgrowth_detail;
	return KgEngT0 + KgEngT1 * KgClamp(E, 0.0, KgEngCap);
}

double EngS(double E)
{
	using namespace kastgrowth_detail;
	return KgEngS0 + KgEngS1 * KgClamp(E, 0.0, KgEngCap);
}

double Dil(double AvgPresent, double DaysSinceGave)
{
	using namespace kastgrowth_detail;
	return std::pow(1.0 + std::max(0.0, AvgPresent) / KgDilute, -0.5) * (1.0 - std::exp(-std::max(0.0, DaysSinceGave) / KgGiveTau));
}

double TipChance(double L, double Hype, double E, double Dilution)
{
	using namespace kastgrowth_detail;
	return KgTipK * std::pow(KgClamp(L, 0.0, 1.0), 1.5) * (1.0 + std::max(0.0, Hype) / 100.0) * EngT(E) * std::max(0.0, Dilution);
}

double CheerChance(double L, double Hype, double E, double Dilution)
{
	using namespace kastgrowth_detail;
	return KgCheerK * KgClamp(L, 0.0, 1.0) * (1.0 + std::max(0.0, Hype) / 100.0) * EngT(E) * std::max(0.0, Dilution);
}

double SubChance(double L, double Hype, double E)
{
	using namespace kastgrowth_detail;
	const double Ln = KgClamp(L, 0.0, 1.0);
	return (KgSubBase + KgSubL2 * Ln * Ln) * (1.0 + std::max(0.0, Hype) / 150.0) * EngS(E);
}

double RenewChance(double L, bool Gift)
{
	using namespace kastgrowth_detail;
	const double Ln = KgClamp(L, 0.0, 1.0);
	return Gift ? KgGiftRenew0 + KgGiftRenew1 * Ln : KgRenew0 + KgRenew1 * Ln;
}

double GiveHazard(double L, double Hype, double E, double Dilution, double Minutes, bool NearBigMoment)
{
	// The visit's tip chance spread over its expected minutes (kastgrowth_detail::KgSpread).
	return kastgrowth_detail::KgSpread(TipChance(L, Hype, E, Dilution), Minutes, NearBigMoment);
}

Payout PayoutOf(Chips Unpaid, Chips Pending, int RentsPaid)
{
	Payout Out;
	if (RentsPaid >= 2)
	{
		Out.Pay = Unpaid + std::max<Chips>(0, Pending); // the first payout after the second rent brings what was held
		Out.Pending = 0;
	}
	else
	{
		Out.Pay = Unpaid;
		Out.Pending = std::max<Chips>(0, Pending);
	}
	return Out;
}

bool SponsorReadPays(int TodayDay, double ReadAt, double LiveMinutes, int Count, int ReadsPaid, int ReadsAsk)
{
	using namespace kastgrowth_detail;
	if (Count < 1 || LiveMinutes < KgReadMinutes)
	{
		return false; // a read needs 30 world minutes live with someone watching
	}
	if (net::DayOf(ReadAt) == TodayDay)
	{
		return false; // once a deal a world day
	}
	if (ReadsAsk <= 0)
	{
		// A deal that asks for no reads (Kast's old terms: a Team RiverLine deal migrated from kast\tdeal, "hourly and reads,
		// nothing asked") keeps one paid read a world day, with no 1.5x cap: there is no ask to cap against (the C-K gate's M1).
		return true;
	}
	return 2 * (static_cast<long long>(std::max(0, ReadsPaid)) + 1) <= 3 * static_cast<long long>(ReadsAsk); // at most 1.5x the ask
}

// ------------------------------------------------------------------ the week's rings

void LiveAdd(std::array<int, 7>& Ring, int& RingDay, int Today, int Minutes)
{
	using namespace kastgrowth_detail;
	if (RingDay < 0 || Today - RingDay >= 7)
	{
		Ring.fill(0);
		RingDay = Today;
	}
	else if (Today > RingDay)
	{
		for (int D = RingDay + 1; D <= Today; ++D)
		{
			Ring[static_cast<size_t>(KgWeekday(D))] = 0;
		}
		RingDay = Today;
	}
	else if (Today <= RingDay - 7)
	{
		return; // older than the ring holds
	}
	Ring[static_cast<size_t>(KgWeekday(Today))] += std::max(0, Minutes);
}

double H7(const std::array<int, 7>& Ring, int RingDay, int Today)
{
	using namespace kastgrowth_detail;
	if (RingDay < 0)
	{
		return 0.0;
	}
	long long Minutes = 0;
	for (int D = std::max(Today - 6, RingDay - 6); D <= std::min(Today, RingDay); ++D)
	{
		Minutes += Ring[static_cast<size_t>(KgWeekday(D))];
	}
	return static_cast<double>(Minutes) / 60.0;
}

bool Granted(int GrantDay, uint32_t GrantMask, int Today, uint32_t Bit)
{
	return GrantDay == Today && (GrantMask & Bit) != 0;
}

void UseGrant(int& GrantDay, uint32_t& GrantMask, int Today, uint32_t Bit)
{
	if (GrantDay != Today)
	{
		GrantDay = Today;
		GrantMask = 0;
	}
	GrantMask |= Bit;
}

// ------------------------------------------------------------------ the forecast and the directory rank

Band Forecast(const std::vector<double>& Attend, const std::vector<double>& S, double A0, double Q, double Bias)
{
	using namespace kastgrowth_detail;
	double Here = 0.0;
	double Var = 0.0;
	for (size_t I = 0; I < Attend.size(); ++I)
	{
		const double Share = I < S.size() ? KgClamp(S[I], 0.0, 1.0) : 1.0;
		const double E = KgClamp(Attend[I], 0.0, 1.0) * std::min(1.0, 0.25 + 0.75 * Share);
		Here += E;
		Var += E * (1.0 - E);
	}
	const double Strangers = std::max(0.0, A0) / 60.0 * (4.0 + 6.0 * Q);
	const double Fc = Here + Strangers;
	const double Sd = std::sqrt(Var + Strangers);
	const double Shown = Fc * std::exp(Bias);
	Band B;
	B.Centre = Fc; // Heat's Lr reads this one (the raw forecast); the band is the shown one
	B.Lo = std::max(0.0, (Shown - KgBandSd * Sd) * std::exp(-KgBandX));
	B.Hi = (Shown + KgBandSd * Sd) * std::exp(KgBandX);
	return B;
}

Standing RankAt(double World, int Viewers)
{
	Standing S;
	S.Rank = 1;
	S.Of = 1;
	for (const Streamer& D : Directory())
	{
		const int Now = ViewersNow(D, World);
		if (Now > 0)
		{
			++S.Of;
			S.Rank += Now > Viewers ? 1 : 0;
		}
	}
	for (const SmallChannel& C : Network(World))
	{
		++S.Of;
		S.Rank += C.Viewers > Viewers ? 1 : 0;
	}
	return S;
}

// ------------------------------------------------------------------ saves

void WriteLines(std::ostream& Out, const SaveView& V)
{
	using namespace kastgrowth_detail;
	const SaveView Empty;
	const bool Holds = V.Version != Empty.Version || V.Heat != Empty.Heat || V.HeatAt != Empty.HeatAt || V.Norm != Empty.Norm || V.Bias != Empty.Bias ||
		V.RunNorm != Empty.RunNorm || V.BestAvg != Empty.BestAvg || V.PingAt != Empty.PingAt || V.LastStreamDay != Empty.LastStreamDay ||
		V.ShowWeek != Empty.ShowWeek || V.ShowDays != Empty.ShowDays || V.GainWeek != Empty.GainWeek || V.GainUsed != Empty.GainUsed || V.FtBase != Empty.FtBase ||
		V.FtDays != Empty.FtDays || V.LiveDay != Empty.LiveDay || V.LiveMin != Empty.LiveMin || V.GrantDay != Empty.GrantDay || V.GrantMask != Empty.GrantMask ||
		V.WelcomeRaidDone != Empty.WelcomeRaidDone || V.CotnDay != Empty.CotnDay || V.PendingCents != Empty.PendingCents || V.LiveSince != Empty.LiveSince ||
		V.GainSoFar != Empty.GainSoFar || V.CarryFollow != Empty.CarryFollow || V.CarryWord != Empty.CarryWord || V.CarrySub != Empty.CarrySub ||
		V.CarryTip != Empty.CarryTip || V.CarryCheer != Empty.CarryCheer || V.CarryRaid != Empty.CarryRaid || V.CarryGift != Empty.CarryGift ||
		!V.RerunHeat.empty() || V.Firsts != Empty.Firsts;
	if (!Holds)
	{
		return;
	}
	// §8.1's order with Firsts last; fixture_tool's ProjectedMo's widths.
	std::string Line = "kast\tmo\t" + KgInt(V.Version);
	Line += "\t" + KgNum(V.Heat, 2) + "\t" + KgNum(V.HeatAt, 2) + "\t" + KgNum(V.Norm, 2) + "\t" + KgNum(V.Bias, 4) + "\t" + KgNum(V.RunNorm, 4);
	Line += "\t" + KgInt(V.BestAvg) + "\t" + KgNum(V.PingAt, 2);
	Line += "\t" + KgInt(V.LastStreamDay) + "\t" + KgInt(V.ShowWeek) + "\t" + KgInt(V.ShowDays) + "\t" + KgInt(V.GainWeek) + "\t" + KgNum(V.GainUsed, 2);
	Line += "\t" + KgInt(V.FtBase) + "\t" + KgInt(V.FtDays) + "\t" + KgInt(V.LiveDay);
	for (const int M : V.LiveMin)
	{
		Line += "\t" + KgInt(M);
	}
	Line += "\t" + KgInt(V.GrantDay) + "\t" + KgInt(static_cast<long long>(V.GrantMask)) + "\t" + (V.WelcomeRaidDone ? "1" : "0") + "\t" + KgInt(V.CotnDay) + "\t" +
		KgInt(V.PendingCents) + "\t" + KgNum(V.LiveSince, 2) + "\t" + KgNum(V.GainSoFar, 2);
	for (const double C : {V.CarryFollow, V.CarryWord, V.CarrySub, V.CarryTip, V.CarryCheer, V.CarryRaid, V.CarryGift})
	{
		Line += "\t" + KgNum(C, 4);
	}
	Line += "\t" + KgRerunText(V.RerunHeat) + "\t" + KgInt(static_cast<long long>(V.Firsts)) + "\n";
	Out << Line;
}

bool ReadLine(const std::vector<std::string>& P, SaveView& V)
{
	using namespace kastgrowth_detail;
	// "At least N fields": version 1's, Firsts optional (a later version appends past them and still reads here).
	if (P.size() < 2 + KgMoFields - 1 || P[0] != "kast" || P[1] != "mo")
	{
		return false;
	}
	SaveView R;
	size_t I = 2;
	const auto MoNum = [&](double Else) { return KgReadNum(P, I++, Else); };
	const auto MoInt = [&](long long Else) { return KgReadInt(P, I++, Else); };
	R.Version = static_cast<int>(MoInt(1));
	if (R.Version < 1)
	{
		return false;
	}
	R.Heat = MoNum(R.Heat);
	R.HeatAt = MoNum(R.HeatAt);
	R.Norm = MoNum(R.Norm);
	R.Bias = MoNum(R.Bias);
	R.RunNorm = MoNum(R.RunNorm);
	R.BestAvg = static_cast<int>(MoInt(R.BestAvg));
	R.PingAt = MoNum(R.PingAt);
	R.LastStreamDay = static_cast<int>(MoInt(R.LastStreamDay));
	R.ShowWeek = static_cast<int>(MoInt(R.ShowWeek));
	R.ShowDays = static_cast<int>(MoInt(R.ShowDays));
	R.GainWeek = static_cast<int>(MoInt(R.GainWeek));
	R.GainUsed = MoNum(R.GainUsed);
	R.FtBase = static_cast<int>(MoInt(R.FtBase));
	R.FtDays = static_cast<int>(MoInt(R.FtDays));
	R.LiveDay = static_cast<int>(MoInt(R.LiveDay));
	for (int& M : R.LiveMin)
	{
		M = static_cast<int>(MoInt(0));
	}
	R.GrantDay = static_cast<int>(MoInt(R.GrantDay));
	R.GrantMask = static_cast<uint32_t>(MoInt(0));
	R.WelcomeRaidDone = MoInt(0) != 0;
	R.CotnDay = static_cast<int>(MoInt(R.CotnDay));
	R.PendingCents = static_cast<Chips>(MoInt(0));
	R.LiveSince = MoNum(R.LiveSince);
	R.GainSoFar = MoNum(R.GainSoFar);
	R.CarryFollow = MoNum(0.0);
	R.CarryWord = MoNum(0.0);
	R.CarrySub = MoNum(0.0);
	R.CarryTip = MoNum(0.0);
	R.CarryCheer = MoNum(0.0);
	R.CarryRaid = MoNum(0.0);
	R.CarryGift = MoNum(0.0);
	R.RerunHeat = KgRerunRead(I < P.size() ? P[I] : std::string());
	++I;
	R.Firsts = static_cast<uint32_t>(MoInt(0));
	V = R;
	return true;
}

std::string MemberTail(const Member& M, int Today)
{
	using namespace kastgrowth_detail;
	const bool Gave = M.GaveDay >= 0 && Today - M.GaveDay <= 90;
	// Hw is saved in tenths, so the test reads the value the save will hold: a member whose unrounded Hw decays to just over
	// 0.05 but whose tenths don't would lose its tail on the next write, and a parse-only round trip would not write back byte
	// for byte (the C-K gate found two such members in update3_streamer).
	const long Hw10 = std::lround(M.Hw * 10.0);
	const bool Watched = M.HwDay >= 0 && Hw10 > 0 && HwToday(static_cast<double>(Hw10) / 10.0, M.HwDay, Today) > 0.05;
	if (!Gave && !Watched)
	{
		return std::string();
	}
	// The two days as days before the member's LastSeen day (the line's P[6]), so the usual tail is "\t0\t<Hw>\t0".
	const int Seen = KgSeenDay(M.LastSeen);
	std::string Tail = "\t" + (Gave ? KgInt(Seen - M.GaveDay) : std::string());
	if (Watched)
	{
		Tail += "\t" + KgInt(Hw10) + "\t" + KgInt(Seen - M.HwDay);
	}
	return Tail;
}

bool ReadMemberTail(const std::vector<std::string>& P, Member& M)
{
	using namespace kastgrowth_detail;
	if (P.size() < 13 || P[0] != "kast" || P[1] != "mem")
	{
		return false;
	}
	const int Seen = net::DayOf(KgReadNum(P, 6, 0.0));
	bool Read = false;
	if (!P[12].empty())
	{
		M.GaveDay = Seen - static_cast<int>(KgReadInt(P, 12, 0));
		Read = true;
	}
	if (P.size() >= 15 && !P[13].empty() && !P[14].empty())
	{
		M.Hw = static_cast<double>(KgReadInt(P, 13, 0)) / 10.0;
		M.HwDay = Seen - static_cast<int>(KgReadInt(P, 14, 0));
		Read = true;
	}
	return Read;
}

std::string LogTail(const StreamLog& L)
{
	using namespace kastgrowth_detail;
	if (L.AvgOwn < 0 && L.PeakAt < 0.0 && L.PeakMoment < 0 && L.HeatAfter < 0.0)
	{
		return std::string(); // an old row
	}
	return "\t" + KgInt(L.AvgOwn) + "\t" + KgNum(L.PeakAt, 1) + "\t" + KgInt(L.PeakMoment) + "\t" + KgNum(L.HeatAfter, 1);
}

bool ReadLogTail(const std::vector<std::string>& P, StreamLog& L)
{
	using namespace kastgrowth_detail;
	if (P.size() < 16 || P[0] != "kast" || P[1] != "log")
	{
		return false;
	}
	L.AvgOwn = static_cast<int>(KgReadInt(P, 12, -1));
	L.PeakAt = KgReadNum(P, 13, -1.0);
	L.PeakMoment = static_cast<int>(KgReadInt(P, 14, -1));
	L.HeatAfter = KgReadNum(P, 15, -1.0);
	return true;
}

std::string GoodwillTail(int LastRaidDay)
{
	return LastRaidDay >= 0 ? "\t" + std::to_string(LastRaidDay) : std::string();
}

int ReadGoodwillTail(const std::vector<std::string>& P)
{
	using namespace kastgrowth_detail;
	if (P.size() < 5 || P[0] != "kast" || P[1] != "goodwill" || P[4].empty())
	{
		return -1;
	}
	return static_cast<int>(std::max(-1ll, KgReadInt(P, 4, -1)));
}
} // namespace growth
} // namespace kast
} // namespace ss
