// Update 3 (SPEC §4.4-§4.8, §7.3) with the creator update's revisions (SPEC_CREATORS §3): Rerun's recommender and its money,
// core-videoalgo's file. Kappa and the breakout chance, the compression of Update 3's levers, the advertisers' season, the
// CPM to RPM identity at each day's ad load, the score, the verdict and the check-ins at world midnights (two-phase, keyed
// on Fate and the day), Momentum from realized performance (H, MomC and the meter's display average), the perfkick, the
// honest labels, the breakout and the narrated viral roll, the Binge wave and Late Bloomer, the Duel's switch on watch time,
// Next-up and Cut links, burnout from Update 3's extra editing, repackaging, Live Drop (NoteStream) and the Kast Heat Rerun
// earns. Closed form forever (V1): every decision appends a component that starts at its own midnight.
//
// A channel runs this once its ChannelX is on (VideoAlgoPriv.h); legacy videos (no VideoX) never get a component (I7).
#include "ShortStack/Game/VideoAlgo.h"

#include "VideoAlgoPriv.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace video
{
namespace valgo
{
// ================================================================== length and ads (§4.5.5)

/** Breakdown, Recap, Vlog: the rows of §4.5.5's tables (a Cut has no length option and no ads of its own). */
int LongRow(Format F)
{
	return F == Format::Recap ? 1 : F == Format::Vlog ? 2 : 0;
}

double LengthMinFor(Format F, LengthOpt L)
{
	if (F == Format::Short)
	{
		return 0.75;
	}
	const double Minutes[3][3] = {{6.0, 8.0, 12.0}, {10.0, 14.0, 20.0}, {12.0, 18.0, 25.0}};
	return Minutes[LongRow(F)][std::clamp(static_cast<int>(L), 0, 2)];
}

double EditMinutesFor(Format F, LengthOpt L)
{
	if (F == Format::Short)
	{
		return 20.0;
	}
	const double Minutes[3][3] = {{75.0, 90.0, 120.0}, {120.0, 150.0, 190.0}, {150.0, 180.0, 220.0}};
	return Minutes[LongRow(F)][std::clamp(static_cast<int>(L), 0, 2)];
}

double EditEnergyFor(Format F, LengthOpt L)
{
	if (F == Format::Short)
	{
		return 2.0;
	}
	const double Energy[3][3] = {{7.0, 8.0, 10.0}, {10.0, 12.0, 15.0}, {12.0, 15.0, 18.0}};
	return Energy[LongRow(F)][std::clamp(static_cast<int>(L), 0, 2)];
}

double LengthRet(Format F, LengthOpt L)
{
	if (F == Format::Short || L == LengthOpt::Standard)
	{
		return 1.0;
	}
	const double Tight[3] = {1.12, 1.08, 1.10};
	const double Extended[3] = {0.86, 0.88, 0.88};
	return L == LengthOpt::Tight ? Tight[LongRow(F)] : Extended[LongRow(F)];
}

/** The load changes this video's ads at all (a Tight Breakdown has no mid-rolls at any load). */
bool LoadBites(Format F, LengthOpt L, AdLoad A)
{
	return F != Format::Short && A != AdLoad::Standard && algo::AdsPerPlayback(F, L, A) != algo::AdsPerPlayback(F, L, AdLoad::Standard);
}

double AdRet(Format F, LengthOpt L, AdLoad A)
{
	if (!LoadBites(F, L, A))
	{
		return 1.0;
	}
	const double Heavy[3] = {0.94, 0.93, 0.92};
	return A == AdLoad::Light ? 1.03 : Heavy[LongRow(F)];
}

double AdDislikes(Format F, LengthOpt L, AdLoad A)
{
	return !LoadBites(F, L, A) ? 1.0 : A == AdLoad::Light ? 0.90 : 1.35;
}

double AdScore(Format F, LengthOpt L, AdLoad A)
{
	return !LoadBites(F, L, A) ? 0.0 : A == AdLoad::Light ? 0.03 : -0.10;
}

double AdConv(Format F, LengthOpt L, AdLoad A)
{
	return LoadBites(F, L, A) && A == AdLoad::Heavy ? 0.90 : 1.0;
}

AdLoad LoadOn(const VideoX& X, int Day)
{
	return X.AdsChangedDay >= 0 && Day >= X.AdsChangedDay ? X.Ads : X.AdsAtUpload;
}

/** r / r_U2 at the stored load, as far as the saved fields tell it (burnout isn't saved: the switch's RPM ratio leaves it out). */
double RetentionRatioAt(const Video& V, AdLoad A)
{
	const double Over = static_cast<double>(OverOf(V)), Under = static_cast<double>(UnderOf(V));
	return LengthRet(V.F, V.X.Len) * AdRet(V.F, V.X.Len, A) * (1.0 - 0.10 * Over) * (1.0 + 0.02 * Under);
}

double RpmOn(const Video& V, int Day)
{
	if (!V.X.On || V.F == Format::Short)
	{
		return V.Rpm;
	}
	const AdLoad Now = LoadOn(V.X, Day), Was = V.X.AdsAtUpload;
	if (Now == Was)
	{
		return V.Rpm;
	}
	const double Rho = RetentionRatioAt(V, Was);
	const double Ratio = AdRet(V.F, V.X.Len, Now) / AdRet(V.F, V.X.Len, Was);
	const double Pays = (1.0 + 0.6 * (Rho * Ratio - 1.0)) / (1.0 + 0.6 * (Rho - 1.0));
	return V.Rpm * algo::AdsPerPlayback(V.F, V.X.Len, Now) / algo::AdsPerPlayback(V.F, V.X.Len, Was) * Pays;
}

double RetentionOn(const Video& V, int Day)
{
	if (!V.X.On)
	{
		return V.Retention;
	}
	const AdLoad Now = LoadOn(V.X, Day), Was = V.X.AdsAtUpload;
	return Now == Was ? V.Retention : V.Retention * AdRet(V.F, V.X.Len, Now) / AdRet(V.F, V.X.Len, Was);
}

double ConvOn(const Video& V, int Day)
{
	if (!V.X.On)
	{
		return V.Conv;
	}
	const AdLoad Now = LoadOn(V.X, Day), Was = V.X.AdsAtUpload;
	return Now == Was ? V.Conv : V.Conv * AdConv(V.F, V.X.Len, Now) / AdConv(V.F, V.X.Len, Was);
}

void ApplyLoad(Video& V, int Day)
{
	VideoX& X = V.X;
	if (!X.On || X.AdsChangedDay < 0 || Day < X.AdsChangedDay || X.Ads == X.AdsAtUpload)
	{
		return;
	}
	const AdLoad Was = X.AdsAtUpload;
	const double Rpm = RpmOn(V, Day), Ret = RetentionOn(V, Day), Conv = ConvOn(V, Day);
	V.Rpm = Rpm;
	V.Retention = Ret;
	V.Conv = Conv;
	X.S0 += AdScore(V.F, X.Len, X.Ads) - AdScore(V.F, X.Len, Was);
	X.AdsAtUpload = X.Ads;
}

// ================================================================== engagement (§5.1)

/** Fnv1a("<Fate>:rates:<N>") without building the string (every day step reads each kept video's rates). */
uint32_t RatesHash(uint32_t Fate, int N)
{
	uint32_t H = 2166136261u;
	auto Feed = [&H](unsigned char Ch) {
		H ^= Ch;
		H *= 16777619u;
	};
	auto Digits = [&Feed](unsigned long long X) {
		char Buf[24];
		int Len = 0;
		do
		{
			Buf[Len++] = static_cast<char>('0' + static_cast<int>(X % 10u));
			X /= 10u;
		} while (X > 0u);
		while (Len > 0)
		{
			Feed(static_cast<unsigned char>(Buf[--Len]));
		}
	};
	Digits(Fate);
	for (const char Ch : {':', 'r', 'a', 't', 'e', 's', ':'})
	{
		Feed(static_cast<unsigned char>(Ch));
	}
	Digits(static_cast<unsigned long long>(N));
	return H;
}

double FateNormal(uint32_t Fate, int I)
{
	// Box-Muller on a pair of hashed uniforms: draw I is the pair's cosine (even) or sine (odd) leg, clamped to +-3.
	const int Pair = I / 2;
	const double U1 = (static_cast<double>(RatesHash(Fate, 2 * Pair)) + 0.5) / 4294967296.0;
	const double U2 = (static_cast<double>(RatesHash(Fate, 2 * Pair + 1)) + 0.5) / 4294967296.0;
	const double R = std::sqrt(-2.0 * std::log(U1));
	const double Angle = 2.0 * 3.14159265358979323846 * U2;
	return std::clamp(I % 2 == 0 ? R * std::cos(Angle) : R * std::sin(Angle), -3.0, 3.0);
}

uint32_t FateOf(const Video& V)
{
	return V.X.On ? V.X.Fate : Fnv1a(V.Id);
}

double KindLike(Kind K)
{
	switch (K)
	{
	case Kind::Win: return 1.30;
	case Kind::FinalTable: return 1.25;
	case Kind::Story: return 1.25;
	case Kind::HeroCall: return 1.20;
	case Kind::Bluff: return 1.15;
	case Kind::Cooler: return 0.95;
	case Kind::BluffCaught: return 0.90;
	default: return 1.0;
	}
}

double KindDislike(Kind K)
{
	switch (K)
	{
	case Kind::Rival: return 2.0;
	case Kind::BluffCaught: return 1.5;
	case Kind::HeroFold: return 1.3;
	default: return 1.0;
	}
}

double KindComment(Kind K)
{
	switch (K)
	{
	case Kind::Rival: return 1.8;
	case Kind::HeroFold: return 1.6;
	case Kind::BadBeat: return 1.4;
	default: return 1.0;
	}
}

double KindShare(Kind K)
{
	switch (K)
	{
	case Kind::BadBeat: return 1.6;
	case Kind::Monster: return 1.5;
	case Kind::Win: return 1.4;
	case Kind::Bluff: return 1.3;
	default: return 1.0;
	}
}

double RetentionRef(Format F)
{
	// Story's title retention at a mid rig (0.85 + 0.3 x 0.5 = 1.0): the neutral video's AvgPctViewed.
	return FormatRetention(F) * StyleRet(TitleStyle::Story);
}

/** FateNormal(Fate, 0 .. 4), each Box-Muller pair drawn once (the same numbers, a third of the hashing: DayStep's hot loop). */
std::array<double, 5> FateNormals5(uint32_t Fate)
{
	std::array<double, 5> N{};
	for (int Pair = 0; Pair < 3; ++Pair)
	{
		const double U1 = (static_cast<double>(RatesHash(Fate, 2 * Pair)) + 0.5) / 4294967296.0;
		const double U2 = (static_cast<double>(RatesHash(Fate, 2 * Pair + 1)) + 0.5) / 4294967296.0;
		const double R = std::sqrt(-2.0 * std::log(U1));
		const double Angle = 2.0 * 3.14159265358979323846 * U2;
		N[static_cast<size_t>(2 * Pair)] = std::clamp(R * std::cos(Angle), -3.0, 3.0);
		if (2 * Pair + 1 < 5)
		{
			N[static_cast<size_t>(2 * Pair + 1)] = std::clamp(R * std::sin(Angle), -3.0, 3.0);
		}
	}
	return N;
}

Rates RatesOf(const Video& V, AdLoad Load, double Retention, int Over)
{
	const uint32_t Fate = FateOf(V);
	const std::array<double, 5> Fn = FateNormals5(Fate);
	const bool Cut = V.F == Format::Short;
	const double O = static_cast<double>(Over);
	const double Sigma = V.X.On ? V.X.Sigma : 0.3;
	const LengthOpt Len = V.X.On ? V.X.Len : LengthOpt::Standard;
	Rates R;
	const double StyleL = V.T == TitleStyle::Teaching ? 1.25 : V.T == TitleStyle::Hype ? 0.90 : 1.0;
	R.Like = 0.035 * KindLike(V.K) * StyleL * std::sqrt(std::max(0.0, Retention) / RetentionRef(V.F)) * (1.0 + 0.3 * Sigma) * std::max(0.0, 1.0 - 0.12 * O) *
		(Cut ? 0.8 : 1.0) * (1.0 + 0.15 * Fn[0]);
	const double StyleD = V.T == TitleStyle::Hype ? 1.3 : V.T == TitleStyle::Teaching ? 0.7 : 1.0;
	R.DislikePerLike = 0.03 * KindDislike(V.K) * StyleD * (1.0 + 0.9 * O) * AdDislikes(V.F, Len, Load) * (1.0 + 0.3 * Fn[1]);
	const double StyleC = V.T == TitleStyle::Question ? 2.5 : V.T == TitleStyle::Plain ? 0.8 : 1.0;
	const bool Reply = V.X.On && (V.X.Launch & (1u << static_cast<int>(Launch::Reply))) != 0;
	R.Comment = 0.004 * KindComment(V.K) * StyleC * (1.0 + 0.1 * O) * (Cut ? 0.25 : 1.0) * (1.0 + 0.25 * Fn[2]) * (Reply ? 1.25 : 1.0);
	R.Share = 0.0025 * KindShare(V.K) * (V.T == TitleStyle::Teaching ? 0.8 : 1.0) * (Cut ? 1.5 : 1.0) * (1.0 + 0.3 * Fn[3]);
	R.Save = 0.002 * (V.T == TitleStyle::Teaching ? 4.0 : 1.0) * (1.0 + 0.3 * Fn[4]);
	return R;
}

bool RecipeUpload(const Video& V)
{
	return V.X.On && V.Fr == Frame::Recipe;
}

bool StrongLanguage(const Video& V)
{
	if (!RecipeUpload(V) || V.T != TitleStyle::Hype)
	{
		return false;
	}
	for (int K = 0; K < std::clamp<int>(V.X.Duels, 1, 3); ++K)
	{
		if (V.X.Spec[static_cast<size_t>(K)].Expr == VaExprTilted)
		{
			return true;
		}
	}
	return false;
}

bool AdultOnly(const Video& V)
{
	return V.X.On && V.X.C == Coin::Limited && !StrongLanguage(V);
}

int PromiseOf(const Video& V)
{
	if (!RecipeUpload(V))
	{
		return 0;
	}
	const VideoX& X = V.X;
	int Thumb = 0;
	if (X.Duels >= 2 && X.DuelWin < 0)
	{
		int Sum = 0;
		for (int K = 0; K < X.Duels && K < 3; ++K)
		{
			Sum += ThumbPromise(X.Spec[static_cast<size_t>(K)]);
		}
		Thumb = static_cast<int>(std::lround(static_cast<double>(Sum) / static_cast<double>(std::min<int>(3, X.Duels))));
	}
	else
	{
		Thumb = ThumbPromise(X.Spec[static_cast<size_t>(std::clamp<int>(X.DuelWin, 0, 2))]);
	}
	return std::max(static_cast<int>(X.Promise), Thumb);
}

int OverOf(const Video& V)
{
	return RecipeUpload(V) ? std::max(0, PromiseOf(V) - static_cast<int>(V.X.Payoff)) : 0;
}

int UnderOf(const Video& V)
{
	return RecipeUpload(V) ? std::max(0, static_cast<int>(V.X.Payoff) - PromiseOf(V)) : 0;
}

double QOf(const Video& V, const Rates& R, double Retention, int Over)
{
	// (A reply round's +0.04 is a launch action's, added at the decisions: ScoreNow. Its comments x1.25 are in R.)
	const double Q = 0.45 * std::log(std::max(1e-6, Retention) / RetentionRef(V.F)) + 0.25 * (R.LikeRatio() - 0.965) / 0.02 + 0.15 * std::log(R.Comment / 0.004) +
		0.15 * std::log(R.Share / 0.0025) - 0.25 * static_cast<double>(Over);
	return std::clamp(Q, -1.5, 1.5);
}

double LostShare(double Q, int Over, bool Heavy)
{
	const double Lambda = std::clamp(0.06 + 0.04 * static_cast<double>(Over) + (Heavy ? 0.03 : 0.0) - (Q > 0.5 ? 0.02 : 0.0), 0.03, 0.25);
	return Lambda / (1.0 - Lambda);
}

// ================================================================== traffic and impressions (§5.4)

std::array<double, 10> MixOf(const Video& V, uint8_t Mix)
{
	// Home, Next up, Search, Subscriptions, Notifications, Cuts feed, Kast, From Cuts, External, Playlists and channel.
	std::array<double, 10> M{};
	switch (Mix)
	{
	case 0:
	{
		const double Ks = V.X.On ? V.X.KastShare : 0.0;
		const double Sig = V.X.On ? V.X.Sigma : 0.3;
		const double Rest = std::max(0.0, 0.93 - Ks);
		M[V.F == Format::Short ? 5 : 0] = Rest * (1.0 - Sig);
		M[3] = Rest * Sig * 0.6;
		M[4] = Rest * Sig * 0.4;
		M[6] = Ks;
		M[8] = 0.04;
		M[9] = 0.03;
		break;
	}
	case 1: M[0] = 0.60; M[1] = 0.40; break;
	case 2: M[1] = 0.70; M[2] = 0.30; break;
	case 3:
	case 11: M[2] = 0.70; M[1] = 0.30; break;
	case 4: M[1] = 0.85; M[0] = 0.15; break;
	case 6: M[2] = 0.50; M[0] = 0.50; break;
	case 7: M[7] = 1.0; break;
	case 8: M[9] = 1.0; break;
	case 9: M[6] = 1.0; break;
	case 10: M[9] = 0.50; M[1] = 0.50; break;
	default: M[0] = 0.50; M[1] = 0.50; break; // a rescale: about the curve it rescales
	}
	if (AdultOnly(V))
	{
		// Site promotion is 18+ (§4.5.4): Home shows it to adults only (x0.6) and the Cuts feed not at all; the views it still
		// gets come from the other sources (the upload's P already took the cut), so the shares keep their sum.
		double Was = 0.0, Now = 0.0;
		for (const double X : M)
		{
			Was += X;
		}
		M[0] *= VaAdultHome;
		M[5] = 0.0;
		for (const double X : M)
		{
			Now += X;
		}
		for (double& X : M)
		{
			X = Now > 0.0 ? X * Was / Now : X;
		}
	}
	return M;
}

uint8_t MixFor(CompKind K)
{
	switch (K)
	{
	case CompKind::Verdict: return 1;
	case CompKind::Check7:
	case CompKind::Check14: return 2;
	case CompKind::Check30:
	case CompKind::Check90: return 3;
	case CompKind::Breakout: return 4;
	case CompKind::Trend: return 6;
	case CompKind::FromCut: return 7;
	case CompKind::FromNextUp: return 8;
	case CompKind::Kast: return 9;
	case CompKind::Binge: return 10;
	case CompKind::LateBloom: return 11;
	default: return 5; // a rescale: the Duel's switch, a repackage
	}
}

bool KindA(const Video& V, CompKind K, double& A)
{
	switch (K)
	{
	case CompKind::Verdict:
	case CompKind::Check7:
	case CompKind::Check14:
	case CompKind::Check30:
	case CompKind::Check90:
	case CompKind::FromNextUp: A = ShapeU3(V.C).A; return true;
	case CompKind::Breakout: A = VaBreakoutA; return true;
	case CompKind::Trend: A = VaTrendA; return true;
	case CompKind::FromCut: A = VaFromCutA; return true;
	case CompKind::Kast: A = VaDropA; return true;
	case CompKind::Binge: A = VaBingeA; return true;
	case CompKind::LateBloom: A = VaBloomA; return true;
	default: return false;
	}
}

double SourceRel(int Source)
{
	const double Rel[10] = {1.0, 1.2, 2.4, 2.0, 3.0, 1.0, 0.0, 0.0, 0.0, 2.0};
	return Source >= 0 && Source < 10 ? Rel[Source] : 0.0;
}

double LegacyCtr(const Video& V)
{
	const double Row = FormatRetention(V.F) * StyleRet(V.T);
	const double Gear = std::clamp((V.Retention / std::max(1e-6, Row) - 0.85) / 0.3, 0.0, 1.0);
	return std::clamp(0.05 * FrameFit(V.Fr, KindGroup(V.K)) * StyleCtr(V.T) * (0.7 + 0.5 * Gear), 0.02, 0.14);
}

double RolledCtr(const Video& V)
{
	return V.X.On ? V.X.Ctr * V.X.CtrNoise : LegacyCtr(V);
}

double FirstWaveNorm(const Video& V)
{
	const std::array<double, 10> M = MixOf(V, 0);
	double Views = 0.0, Weighted = 0.0;
	for (int S = 0; S < 10; ++S)
	{
		if (SourceRel(S) > 0.0)
		{
			Views += M[static_cast<size_t>(S)];
			Weighted += M[static_cast<size_t>(S)] / SourceRel(S);
		}
	}
	return Views > 0.0 ? Weighted / Views : 1.0;
}

double ImprPerView(const Video& V, uint8_t Mix, float CtrF)
{
	const double Ctr = std::max(1e-6, RolledCtr(V));
	const std::array<double, 10> M = MixOf(V, Mix);
	double Out = 0.0;
	if (Mix == 0)
	{
		for (int S = 0; S < 10; ++S)
		{
			Out += SourceRel(S) > 0.0 ? M[static_cast<size_t>(S)] : 0.0;
		}
		return Out / Ctr; // the first wave's blend is exactly the rolled CTR
	}
	const double F = std::max(1e-6, static_cast<double>(CtrF));
	for (int S = 0; S < 10; ++S)
	{
		Out += SourceRel(S) > 0.0 ? M[static_cast<size_t>(S)] / (Ctr * SourceRel(S) * F) : 0.0;
	}
	return Out;
}

double ImprGain(const Video& V, double T0, double T1)
{
	if (!V.X.On)
	{
		return std::max(0.0, Views(V, T1) - Views(V, T0)) * ImprPerView(V, 0, 1.0f);
	}
	const VaShape Sh = ShapeU3(V.C);
	double Out = (CurveFrac(Sh.A, Sh.Tau1, Sh.Tau2, (T1 - V.At) / VaDay) - CurveFrac(Sh.A, Sh.Tau1, Sh.Tau2, (T0 - V.At) / VaDay)) * V.X.P0 * ImprPerView(V, 0, 1.0f);
	for (const Comp& K : V.X.Comps)
	{
		const double Part = (CurveFrac(K.A, Sh.Tau1, Sh.Tau2, (T1 - K.T) / VaDay) - CurveFrac(K.A, Sh.Tau1, Sh.Tau2, (T0 - K.T) / VaDay)) * K.P;
		Out += Part * ImprPerView(V, K.SrcMix, K.CtrF);
	}
	return std::max(0.0, Out);
}

// ================================================================== the audience's hours (§5.5) and timing (§4.4)

double HourHeat(int Weekday, int Hour)
{
	// Kast's poker hour curve (Kast.cpp's TimeOfDay), blended over the viewers' time zones around the player's.
	const double Zones[6][2] = {{-3.0, 0.10}, {-2.0, 0.15}, {-1.0, 0.20}, {0.0, 0.35}, {1.0, 0.12}, {2.0, 0.08}};
	double Sum = 0.0;
	for (const auto& Z : Zones)
	{
		const int H = ((Hour + static_cast<int>(Z[0])) % 24 + 24) % 24;
		const double F = H >= 18 ? 1.25 : H < 3 ? 1.05 : H < 8 ? 0.55 : H < 12 ? 0.7 : 0.9;
		Sum += Z[1] * F;
	}
	return Sum * ((Weekday % 7 + 7) % 7 >= 4 ? 1.1 : 1.0);
}

double TimeFitAt(const Channel& C, double At)
{
	const int Weekday = DayOf(At) % 7;
	const double Hour = std::fmod(At, VaDay) / 60.0;
	double Before = algo::PeakHour(C, Weekday) - Hour; // hours until the peak hour starts
	Before = Before > 12.0 ? Before - 24.0 : Before <= -12.0 ? Before + 24.0 : Before;
	if (Before > -1.0 && Before <= 2.0)
	{
		return 1.08;
	}
	return Before > 2.0 && Before <= 4.0 ? 1.04 : 1.0;
}

double TimingV(double TimeFit)
{
	return TimeFit >= 1.08 ? 0.08 : TimeFit >= 1.04 ? 0.04 : 0.0;
}

// ================================================================== the score at a decision, the kick, Momentum

int VerdictDayOf(const Video& V)
{
	return static_cast<int>(std::ceil((V.At + 2.0 * VaDay) / VaDay - 1e-9));
}

double ShareOf(Class C, int CheckDay)
{
	if (C == Class::Strategy)
	{
		return CheckDay == 0 ? 0.50 : CheckDay == 7 ? 0.20 : 0.10;
	}
	return CheckDay == 0 ? 0.60 : CheckDay == 7 ? 0.25 : CheckDay == 14 ? 0.15 : 0.0;
}

double ScoreNow(const Video& V, const Channel& C, int Day, int Stage)
{
	const VideoX& X = V.X;
	double S = X.S0;
	if (Stage >= 1)
	{
		S += (X.Launch & (1u << static_cast<int>(Launch::Kast))) != 0 ? VaDropV : 0.0;
		S += (X.Launch & (1u << static_cast<int>(Launch::Teaser))) != 0 ? VaTeaserV : 0.0;
		S += (X.Launch & (1u << static_cast<int>(Launch::Reply))) != 0 ? VaReplyQ : 0.0;
	}
	if (Stage >= 2 && X.Series >= 0)
	{
		// A newer episode in its series just dropped (in the week before this decision): +0.10.
		for (const Video& O : C.Videos)
		{
			if (&O != &V && O.X.On && O.X.Series == X.Series && O.At > V.At && O.At > (static_cast<double>(Day) - 7.0) * VaDay && O.At <= static_cast<double>(Day) * VaDay)
			{
				S += 0.10;
				break;
			}
		}
	}
	return std::clamp(S, -2.0, 2.0);
}

double Perf(const VideoX& X)
{
	return X.On ? std::clamp(static_cast<double>(X.LuckLn), VaPerfLo, VaPerfHi) : 0.0;
}

double PerfKick(const Video& V)
{
	return std::clamp(std::exp(0.5 * Perf(V.X)), VaKickLo, VaKickHi) / (Travels(V.K) ? VaKickNormTravel : VaKickNormStay);
}

double MomCOf(double H)
{
	return VaRound(std::clamp(std::exp(0.5 * H), VaMomLo, VaMomHi) / VaMomNorm, 1e6); // (the channel's line keeps it to 1e-6)
}

/** The decisions a video still has after Day (each may append a kick), and its breakout's slot while one can still fire. */
size_t ReservedAfter(const Video& V, int Day)
{
	const int D0 = DayOf(V.At);
	const int Vd = VerdictDayOf(V);
	size_t N = Vd > Day ? 1 : 0;
	const int Checks[4] = {7, 14, 30, 90};
	for (const int Ck : Checks)
	{
		if ((Ck <= 14 || V.C == Class::Strategy) && D0 + Ck > Day)
		{
			++N;
		}
	}
	const bool Broke = std::any_of(V.X.Comps.begin(), V.X.Comps.end(), [](const Comp& K) { return K.Kind == CompKind::Breakout; });
	if (!Broke && (Vd > Day || D0 + 14 > Day))
	{
		++N;
	}
	return N;
}

bool CompFits(const Video& V, int Day)
{
	return V.X.On && V.X.Comps.size() + ReservedAfter(V, Day) < VaMaxComps;
}

/** A component as the save keeps it (its views whole from 100 up, else to the cent; A to 1e-4, the CTR factor to 1e-3; a Kast
 *  part's average whole). */
void AddComp(VideoX& X, Comp K)
{
	K.P = VaViews(K.P);
	K.A = VaRound(K.A, 1e4);
	K.CtrF = static_cast<float>(K.Kind == CompKind::Kast ? std::round(static_cast<double>(K.CtrF)) : VaRound(static_cast<double>(K.CtrF), 1e3));
	X.Comps.push_back(K);
}

double TitleTraitCtr(uint16_t Traits)
{
	double F = 1.0;
	F *= (Traits & 1u) ? 1.06 : 1.0;
	F *= (Traits & 2u) ? 1.04 : 1.0;
	F *= (Traits & 4u) ? 1.03 : 1.0;
	F *= (Traits & 8u) ? 1.03 : 1.0;
	F *= (Traits & 16u) ? 1.03 : 1.0;
	F *= (Traits & 32u) ? 0.95 : 1.0;
	F *= (Traits & 64u) ? 1.05 : 1.0;
	F *= (Traits & 128u) ? 0.97 : 1.0;
	F *= (Traits & 256u) ? 1.06 : 1.0;
	const int Norm = static_cast<int>((Traits >> 9) & 127u);
	return F * (Norm == 0 ? 1.0 : 1.0 + static_cast<double>(Norm - 64) / 500.0); // (core-videocontent_to_core-videoalgo_1: the 1/500 step)
}

double TitleTraitRet(uint16_t Traits)
{
	return (Traits & 128u) ? 1.04 : 1.0;
}

// ================================================================== an idea's facts and the trends (§4.5.4, §4.6.10)

std::vector<std::string> FactValues(const std::string& Facts, const std::string& Key)
{
	std::vector<std::string> Out;
	size_t At = 0;
	while (At < Facts.size())
	{
		size_t End = At;
		while (End < Facts.size() && Facts[End] != ';' && Facts[End] != ' ' && Facts[End] != '\t' && Facts[End] != '\n')
		{
			++End;
		}
		const size_t Eq = Facts.find('=', At);
		if (Eq != std::string::npos && Eq < End && Facts.compare(At, Eq - At, Key) == 0 && Eq - At == Key.size())
		{
			Out.push_back(Facts.substr(Eq + 1, End - Eq - 1));
		}
		At = End + 1;
	}
	return Out;
}

double AdCategoryFit(const std::string& Category)
{
	const std::pair<const char*, double> Fits[] = {{"gear", 1.25}, {"teach", 1.20}, {"results", 1.15}, {"story", 1.00}, {"life", 0.95}, {"loud", 0.90}, {"news", 0.90},
		{"rival", 0.85}};
	for (const auto& F : Fits)
	{
		if (Category == F.first)
		{
			return F.second;
		}
	}
	return 0.0;
}

bool TrendOfKey(const std::string& Key, VaTrend& Out)
{
	const size_t Colon = Key.find(':');
	if (Colon == std::string::npos || Colon + 1 >= Key.size())
	{
		return false;
	}
	const std::string Kind = Key.substr(0, Colon), Arg = Key.substr(Colon + 1);
	Out = VaTrend();
	Out.Key = Key;
	if (Kind == "series")
	{
		// The series' days (the network's calendar): the strength by its guarantee, 1.12 for a small one to 1.20 at $100M.
		if (const net::SeriesInfo* S = net::Shared().FindSeries(Arg))
		{
			Out.Start = static_cast<double>(S->FirstDay) * VaDay;
			Out.End = static_cast<double>(S->LastDay + 1) * VaDay;
			Out.Strength = 1.12 + 0.08 * std::min(1.0, static_cast<double>(S->GtdCents) / 1e10);
			return true;
		}
		return false;
	}
	char* Stop = nullptr;
	const double N = std::strtod(Arg.c_str(), &Stop);
	if (Stop == Arg.c_str() || *Stop != '\0')
	{
		return false;
	}
	const int Year = static_cast<int>(N);
	auto Days = [&Out](int D0, int D1, double Strength) {
		Out.Start = static_cast<double>(D0) * VaDay;
		Out.End = static_cast<double>(D1) * VaDay;
		Out.Strength = Strength;
		return true;
	};
	auto From = [&Out](double Start, double LengthDays, double Strength) {
		Out.Start = Start;
		Out.End = Start + LengthDays * VaDay;
		Out.Strength = Strength;
		return true;
	};
	// The world's log (the key is the event's minute): the week's list takes an event in at the first Monday it is a day old
	// by (§4.6.10, I2), and its window runs from that Monday, so a 3-day trend is hot Monday to Wednesday wherever in the week
	// before it happened. (From the event itself, a final table at Monday 00:15 or a launch on a Monday was over before any
	// list could show it.)
	const double TrListWeek = 7.0 * VaDay;
	const double TrListed = (std::floor((N + VaDay) / TrListWeek) + 1.0) * TrListWeek;
	if (Kind == "sunday")
	{
		return From(N * VaDay + 12.0 * 60.0, 1.0, 1.08);
	}
	if (Kind == "champ")
	{
		return From(TrListed, 3.0, 1.15);
	}
	if (Kind == "final")
	{
		return From(TrListed, 3.0, 1.10);
	}
	if (Kind == "site")
	{
		return From(TrListed, 7.0, 1.25);
	}
	if (Kind == "overlay")
	{
		return From(TrListed, 7.0, 1.10);
	}
	if (Kind == "poy")
	{
		return Days(world::DayOn(Year, 11, 15), world::DayOn(Year + 1, 1, 1), 1.10);
	}
	if (Kind == "hof")
	{
		return Days(world::DayOn(Year, 7, 10), world::DayOn(Year, 7, 21), 1.15);
	}
	if (Kind == "holiday")
	{
		return Days(world::DayOn(Year, 12, 20), world::DayOn(Year + 1, 1, 1), 1.10);
	}
	if (Kind == "newyear")
	{
		return Days(world::DayOn(Year, 12, 26), world::DayOn(Year + 1, 1, 16), 1.15);
	}
	return false;
}

double TrendHeat(const VaTrend& T, double At)
{
	if (At < T.Start || At >= T.End || T.End <= T.Start)
	{
		return 0.0;
	}
	const double Mid = 0.5 * (T.Start + T.End);
	return std::max(0.0, 1.0 - std::fabs(At - Mid) / (0.5 * (T.End - T.Start)));
}

std::vector<VaTrend> TrendsFor(const Candidate& Lead, double At)
{
	std::vector<VaTrend> Out;
	auto Add = [&Out, At](const std::string& Key) {
		VaTrend T;
		if (TrendOfKey(Key, T) && TrendHeat(T, At) > 0.0 &&
			std::none_of(Out.begin(), Out.end(), [&Key](const VaTrend& X) { return X.Key == Key; }))
		{
			Out.push_back(T);
		}
	};
	// A Sunday major's own candidate, from Sunday noon (it can't be older than the window's start, less the half day).
	const int D = DayOf(At);
	const int Sunday = D % 7 == 6 ? D : D % 7 == 0 ? D - 1 : -1;
	if (Sunday >= 0 && Lead.Event.find("Sunday") != std::string::npos && Lead.At >= static_cast<double>(Sunday) * VaDay)
	{
		Add("sunday:" + std::to_string(Sunday));
	}
	// A series event's candidate while its series runs (a moment from inside its days: an older series' moment can't ride the
	// next series' week).
	if (Lead.Series)
	{
		if (const net::SeriesInfo* S = net::Shared().CurrentSeries(At))
		{
			if (D >= S->FirstDay && D <= S->LastDay && DayOf(Lead.At) >= S->FirstDay)
			{
				Add("series:" + S->Id);
			}
		}
	}
	for (const std::string& Key : FactValues(Lead.IdeaFacts, "trend"))
	{
		Add(Key);
	}
	std::stable_sort(Out.begin(), Out.end(), [At](const VaTrend& A, const VaTrend& B) {
		return (A.Strength - 1.0) * TrendHeat(A, At) > (B.Strength - 1.0) * TrendHeat(B, At);
	});
	return Out;
}

int TrendCoolKey(const std::string& Key)
{
	return static_cast<int>(Fnv1a(Key) & 0x7fffffffu);
}

// ================================================================== Rerun 2.0's copy (the bell, the log's words)

std::string Quoted(const std::string& Title)
{
	return "\xE2\x80\x9C" + Title + "\xE2\x80\x9D";
}

const char* VerdictName(int Label)
{
	switch (Label)
	{
	case 0: return "Not pushed";
	case 1: return "Held";
	case 2: return "Pushed";
	case 3: return "Pushed more";
	case 4: return "Pushed hard";
	default: return "";
	}
}

std::string BellVerdict(const std::string& Title, int Label)
{
	switch (Label)
	{
	case 0: return Quoted(Title) + " didn't hold viewers. Rerun stopped testing it.";
	case 1: return "Rerun is holding " + Quoted(Title) + " about where your uploads usually land.";
	case 3: return "Rerun is pushing " + Quoted(Title) + " further: it's beating most of your recent uploads.";
	case 4: return "Rerun is pushing " + Quoted(Title) + " hard. People keep clicking and staying.";
	default: return "Rerun is pushing " + Quoted(Title) + " to more viewers.";
	}
}

std::string BellKick(const std::string& Title)
{
	return "Rerun is showing " + Quoted(Title) + " to more people.";
}

std::string BellBreakout(const std::string& Title)
{
	return Quoted(Title) + " is showing up next to other videos.";
}

std::string BellViral(const std::string& Title)
{
	return Quoted(Title) + " is breaking out: it's showing up next to other videos.";
}

std::string BellBinge(int Videos, double Views)
{
	// (C3 fix L4: the views its parts really added on the wave's first day, rung at the midnight that ends it. A tiny channel's
	// wave that added under one whole view that day keeps the old words: no "0 views" bell.)
	const long long Whole = std::isfinite(Views) ? std::llround(std::max(0.0, Views)) : 0;
	if (Whole < 1)
	{
		return Videos == 1 ? "New viewers are bingeing your channel: an older video is getting views again."
						   : "New viewers are bingeing your channel: " + std::string(Videos == 2 ? "two" : "three") + " older videos are getting views again.";
	}
	const std::string Picked = " picked up " + Grouped(static_cast<int64_t>(Whole)) + (Whole == 1 ? " view." : " views.");
	return Videos == 1 ? "New viewers are bingeing your channel: an older video" + Picked
					   : "New viewers are bingeing your channel: " + std::to_string(Videos) + " older videos" + Picked;
}

std::string BellBloom(const std::string& Title, double Ratio)
{
	// (C3 fix L4: its first day awake against its usual day, the 7 before the check-in that woke it; whole times, 2 and up.)
	const long long Times = std::isfinite(Ratio) && Ratio < 1e6 ? std::llround(Ratio) : 0;
	return Times >= 2 ? Quoted(Title) + " woke up: " + std::to_string(Times) + "\xC3\x97 its usual daily views."
					  : Quoted(Title) + " woke up: search viewers are finding it again.";
}

std::string BellDuel(int Winner)
{
	const char Letter[2] = {static_cast<char>('A' + std::clamp(Winner, 0, 2)), '\0'};
	return std::string("Thumbnail test finished: ") + Letter + " won on watch time.";
}

std::string BellStreak(int Weeks)
{
	return "Upload streak: " + std::to_string(Weeks) + (Weeks == 1 ? " week." : " weeks.");
}

std::string BellEarly(const std::string& Title)
{
	return Quoted(Title) + " is doing better than most of your recent uploads.";
}

std::string BellTrend(const std::string& Label)
{
	return "Searches for " + Quoted(Label) + " are up.";
}

std::string BellBurnout(double Hours)
{
	const long long H = std::llround(Hours);
	return "You've put " + std::to_string(H) + (H == 1 ? " extra hour" : " extra hours") + " into this week's videos. It shows.";
}

std::string BellRainCheck(int Weeks)
{
	return "Upload streak: " + std::to_string(Weeks) + " weeks. You earned a Rain Check: a missed week won't break it.";
}

std::string LaunchCarry(double MomCAtUpload)
{
	return "Riding your breakout: this upload starts with +" + std::to_string(std::llround(100.0 * (MomCAtUpload - 1.0))) + "% reach";
}

std::string VerdictDespite(int Label)
{
	return Label == 0 ? "Not pushed despite your momentum" : Label == 1 ? "Held despite your momentum" : std::string();
}

std::string LiveDropLine(int Watching)
{
	return "Live Drop: your stream gave it a push (" + Grouped(Watching) + " watching)";
}

std::string LiveDropHint()
{
	return "Go live within 6 hours to give it a push";
}

std::string StreakLine(int Weeks, int RainChecks)
{
	std::string Out = "Upload streak: " + std::to_string(Weeks) + (Weeks == 1 ? " week" : " weeks");
	if (RainChecks > 0)
	{
		Out += " \xC2\xB7 " + std::to_string(RainChecks) + (RainChecks == 1 ? " Rain Check" : " Rain Checks");
	}
	return Out;
}

std::string WouldEarnLine(Chips Cents)
{
	return "As a Partner, this video would have earned about " + Money(Cents) + " so far (not paid)";
}

const char* TagName(int Tag)
{
	return Tag == 1 ? "Above typical" : Tag == 2 ? "Breakout" : Tag == 3 ? "Outlier" : "";
}

std::string LimitedSubject()
{
	return "Limited ads on one of your videos.";
}

std::string LimitedBody(const std::string& Title)
{
	return Quoted(Title) + " shows a real-money site's promotions, so most advertisers sit it out: it runs limited ads and Rerun shows it to adults "
		"only, off the Cuts feed. Videos about the poker itself keep full ads.";
}


VaSplit SplitCovering(const Channel& C, Chips Target)
{
	// The newest days of the ring until their cents reach Target; past the ring's first day, the month rows (the ring's first
	// month less its days in the ring, then whole older months), the last one's part scaled to what is left (a balance older
	// than the 90-day ring is a channel under $100 for three months: its oldest month is attributed in proportion).
	VaSplit S;
	S.Has = VaHasSplit;
	auto Take = [&S](const DayRow& R, double Share) {
		S.Cents += static_cast<Chips>(std::llround(static_cast<double>(R.Cents) * Share));
		S.CutCents += static_cast<Chips>(std::llround(static_cast<double>(VaRowCut(R)) * Share));
		S.Views += static_cast<double>(R.Views) * Share;
		S.CutViews += VaRowCutViews(R) * Share;
		S.AdImpr += VaRowImpr(R) * Share;
		S.Monetized += VaRowMonetized(R) * Share;
		S.Held += static_cast<Chips>(std::llround(static_cast<double>(VaRowHeld(R)) * Share));
		S.HeldCut += static_cast<Chips>(std::llround(static_cast<double>(VaRowHeldCut(R)) * Share));
	};
	const ChannelX& X = C.X;
	for (size_t I = X.Days.size(); I-- > 0 && S.Cents < Target;)
	{
		if (X.Days[I].Cents <= 0 && VaRowHeld(X.Days[I]) <= 0 && VaRowImpr(X.Days[I]) <= 0.0 && VaRowCutViews(X.Days[I]) <= 0.0)
		{
			continue; // (a day no ad paid: before Partner; its views aren't the statement's. A day the top-earner rate held
					  // whole carries its split, so it stays the month's: its views in the RPM, its impressions in the CPM.)
		}
		Take(X.Days[I], 1.0);
		S.FromDay = X.Days[I].Key;
		S.ToDay = S.ToDay < 0 ? X.Days[I].Key : S.ToDay;
	}
	if (S.Cents < Target && !X.Days.empty())
	{
		int Y = 0, Mo = 0, Dm = 0;
		world::CivilDate(X.Days.front().Key, Y, Mo, Dm);
		const int First = Y * 12 + (Mo - 1);
		for (size_t I = X.Months.size(); I-- > 0 && S.Cents < Target;)
		{
			const DayRow& M = X.Months[I];
			if (M.Key > First)
			{
				continue;
			}
			DayRow Part = M;
			if (M.Key == First)
			{
				// The month's days before the ring: the month's row less its days in the ring.
				for (const DayRow& D : X.Days)
				{
					world::CivilDate(D.Key, Y, Mo, Dm);
					if (Y * 12 + (Mo - 1) == First)
					{
						Part.Cents -= D.Cents;
						Part.Views -= D.Views;
						VaSetRowSplit(Part, VaRowCut(Part) - VaRowCut(D), static_cast<float>(VaRowCutViews(Part) - VaRowCutViews(D)), static_cast<float>(VaRowImpr(Part) - VaRowImpr(D)),
							static_cast<float>(VaRowMonetized(Part) - VaRowMonetized(D)));
						VaSetRowHeld(Part, VaRowHeld(Part) - VaRowHeld(D), VaRowHeldCut(Part) - VaRowHeldCut(D));
					}
				}
			}
			if (Part.Cents <= 0)
			{
				continue;
			}
			const Chips Left = Target - S.Cents;
			Take(Part, Part.Cents > Left ? static_cast<double>(Left) / static_cast<double>(Part.Cents) : 1.0);
		}
	}
	S.Cents = std::min(S.Cents, std::max<Chips>(Target, 0));
	S.CutCents = std::clamp<Chips>(S.CutCents, 0, S.Cents);
	return S;
}

VaSplit BalanceSplit(const Channel& C)
{
	return SplitCovering(C, C.BalanceCents);
}

// ================================================================== the top-earner rate (the user's soft ceiling, 2026-10-27)

Chips TopRateFrom()
{
	return 6 * life::MonthlyRentCents;
}

Chips TopRateTop()
{
	return 10 * life::MonthlyRentCents;
}

int PayMonthOf(int Day)
{
	int Y = 0, Mo = 0, Dm = 0;
	world::CivilDate(Day, Y, Mo, Dm);
	return Y * 12 + (Mo - 1) + (Dm >= VaPayDay ? 1 : 0);
}

int PayMonthFirst(int Day)
{
	// The 21st on or before Day: this month's, or the month before's (its length from its last day's date).
	int Y = 0, Mo = 0, Dm = 0;
	world::CivilDate(Day, Y, Mo, Dm);
	if (Dm >= VaPayDay)
	{
		return Day - (Dm - VaPayDay);
	}
	const int LastOfBefore = Day - Dm;
	world::CivilDate(LastOfBefore, Y, Mo, Dm);
	return LastOfBefore - (Dm - VaPayDay);
}

double TopRateCurve(double EarnedCents)
{
	const double T = static_cast<double>(TopRateFrom()), C = static_cast<double>(TopRateTop() - TopRateFrom());
	return EarnedCents <= T ? EarnedCents : T + C * (1.0 - std::exp(-(EarnedCents - T) / C));
}

Chips TopRatePay(Chips PaidSoFar, Chips Earned)
{
	const Chips T = TopRateFrom(), Top = TopRateTop();
	if (Earned <= 0 || PaidSoFar + Earned <= T)
	{
		return Earned; // (every day at or under T: today's cents exactly)
	}
	// The part that still fits under T pays in full; the rest closes the gap to T + C by 1 - e^(-rest / C), the closed form's
	// own step from wherever the month stands (so the month's days in any grouping land on the same curve).
	const Chips Straight = std::max<Chips>(0, T - PaidSoFar);
	const Chips From = PaidSoFar + Straight;
	const double Gap = static_cast<double>(Top - From);
	if (Gap <= 0.0)
	{
		return Straight;
	}
	const double After = static_cast<double>(Top) - Gap * std::exp(-static_cast<double>(Earned - Straight) / static_cast<double>(Top - T));
	return std::clamp<Chips>(Straight + (static_cast<Chips>(std::llround(After)) - From), Straight, Earned);
}

double TopRateShare(Chips PaidSoFar)
{
	const Chips T = TopRateFrom(), Top = TopRateTop();
	return PaidSoFar <= T ? 1.0 : std::max(0.0, static_cast<double>(Top - PaidSoFar) / static_cast<double>(Top - T));
}

Chips PaidThisMonth(const Channel& C, int Day)
{
	if (!C.X.On)
	{
		return 0;
	}
	const int First = PayMonthFirst(Day);
	Chips Sum = 0;
	for (size_t I = C.X.Days.size(); I-- > 0;)
	{
		const DayRow& R = C.X.Days[I];
		if (R.Key >= Day)
		{
			continue;
		}
		if (R.Key < First)
		{
			break; // (the ring runs in day order: the month's first day is behind us)
		}
		Sum += R.Cents;
	}
	return Sum;
}

double DayPaidShare(const Channel& C, int Day)
{
	if (!C.X.On)
	{
		return 1.0;
	}
	const Chips Before = PaidThisMonth(C, Day);
	const auto It = std::find_if(C.X.Days.begin(), C.X.Days.end(), [Day](const DayRow& R) { return R.Key == Day; });
	if (It == C.X.Days.end())
	{
		return TopRateShare(Before); // (not stepped yet: the share its first view pays at, StatsOf's "today")
	}
	const Chips T = TopRateFrom(), Paid = It->Cents;
	if (Before + Paid <= T)
	{
		return 1.0; // (a month at or under T so far: every cent the day's ads earned)
	}
	if constexpr (VaHasHeld)
	{
		const Chips Earned = Paid + VaRowHeld(*It);
		return Earned > 0 ? static_cast<double>(Paid) / static_cast<double>(Earned) : TopRateShare(Before);
	}
	else
	{
		// (CpmReadBack's reading: the part under T paid in full, the rest closed the gap G0 to After, so its ads were C ln(G0 /
		// After); a day that closed the gap to under half a cent reads as if it stopped at half a cent.)
		const Chips Straight = std::max<Chips>(0, T - Before);
		const double G0 = static_cast<double>(TopRateTop() - (Before + Straight));
		if (G0 <= 0.0)
		{
			return 0.0;
		}
		const double After = std::max(0.5, G0 - static_cast<double>(Paid - Straight));
		const double Earned = static_cast<double>(Straight) + static_cast<double>(TopRateTop() - T) * std::log(G0 / After);
		return Earned > 0.0 ? std::min(1.0, static_cast<double>(Paid) / Earned) : 1.0;
	}
}

std::string TopRateDollars(Chips Cents)
{
	return Cents % 100 == 0 ? "$" + Grouped(Cents / 100) : Money(Cents);
}

std::string TopRateLine()
{
	return "Top-earner rate: above " + TopRateDollars(TopRateFrom()) + " this month pays at a reduced share.";
}

std::string TopRateHelp()
{
	return "Rerun pays its biggest channels at a reduced share. A payout month (the 21st to the 20th) pays in full up to " + TopRateDollars(TopRateFrom()) +
		"; past that, each dollar your ads earn pays a little less than the one before, and no month pays more than " + TopRateDollars(TopRateTop()) +
		". Views, subscribers and analytics still count every view.";
}

/**
 * Before DayRow keeps the held cents, the rings keep only what was paid, so each of the month's days is read back through the
 * curve: a day the month stayed under T paid all its ads; a day past T paid Straight (what still fit under T) plus the gap's
 * closing, G0 (1 - e^(-rest / C)), so its ads were Straight + C ln(G0 / gap after). A day whose gap after is under $10 can't
 * be read back to the cent (the curve is flat there) and sits out; the rest give the advertisers' price on their impressions.
 */
double CpmReadBack(const Channel& C, const VaSplit& S, int PayMonth)
{
	const double Top = static_cast<double>(TopRateTop()), Room = static_cast<double>(TopRateTop() - TopRateFrom());
	Chips Paid = 0;
	double Gross = 0.0, Impr = 0.0;
	for (const DayRow& R : C.X.Days)
	{
		if (PayMonthOf(R.Key) != PayMonth || R.Cents <= 0)
		{
			continue;
		}
		const Chips Before = Paid;
		Paid += R.Cents;
		double Ads = static_cast<double>(R.Cents);
		if (Paid > TopRateFrom())
		{
			const Chips Straight = std::max<Chips>(0, TopRateFrom() - Before);
			const double From = static_cast<double>(Before + Straight), Above = static_cast<double>(R.Cents - Straight);
			const double G0 = Top - From, After = G0 - Above;
			if (G0 <= 0.0 || After < 1000.0)
			{
				continue;
			}
			Ads = static_cast<double>(Straight) + Room * std::log(G0 / After);
		}
		Gross += static_cast<double>(R.Cents - VaRowCut(R)) * Ads / static_cast<double>(R.Cents);
		Impr += VaRowImpr(R);
	}
	return Impr >= 100.0 && Gross > 0.0 ? Gross / 100.0 / VaCreatorShare / Impr * 1000.0 : S.Cpm();
}

double StatementCpm(const Channel& C, const VaSplit& S, int PayMonth)
{
	// With DayRow's held cents the split's CPM is the advertisers' already; a span at or under T held nothing.
	if constexpr (VaHasHeld)
	{
		(void)C;
		(void)PayMonth;
		return S.Cpm();
	}
	else
	{
		return S.Cents <= TopRateFrom() ? S.Cpm() : CpmReadBack(C, S, PayMonth);
	}
}

std::vector<std::string> AlgoInventory()
{
	std::vector<std::string> Out;
	const std::string T = "kings, 82% to win, and then the nine of hearts";
	for (int L = 0; L <= 4; ++L)
	{
		Out.push_back("line\t" + BellVerdict(T, L));
	}
	Out.push_back("line\t" + BellKick(T));
	Out.push_back("line\t" + BellBreakout(T));
	Out.push_back("line\t" + BellViral(T));
	Out.push_back("line\t" + BellBinge(1, 412.0));
	Out.push_back("line\t" + BellBinge(1, 1.0));
	Out.push_back("line\t" + BellBinge(2, 860.0));
	Out.push_back("line\t" + BellBinge(3, 1240.0));
	for (int N = 1; N <= 3; ++N)
	{
		Out.push_back("line\t" + BellBinge(N, 0.3)); // (a wave under one whole view that day: the old words)
	}
	Out.push_back("line\t" + BellBloom("ICM at the bubble", 3.0));
	Out.push_back("line\t" + BellBloom("ICM at the bubble", 1.2));
	Out.push_back("line\t" + TopRateLine());
	Out.push_back("body\t" + TopRateHelp());
	for (int W = 0; W < 3; ++W)
	{
		Out.push_back("line\t" + BellDuel(W));
	}
	Out.push_back("line\t" + BellStreak(1));
	Out.push_back("line\t" + BellStreak(6));
	Out.push_back("line\t" + BellEarly(T));
	for (const char* Label : {"Sunday majors", "Player of the Year race", "Hall of Fame class", "Holiday grind", "New Year challenges", "Is Ignite soft?"})
	{
		Out.push_back("line\t" + BellTrend(Label));
	}
	Out.push_back("line\t" + BellBurnout(6.2));
	Out.push_back("line\t" + BellBurnout(1.0));
	Out.push_back("line\t" + BellRainCheck(4));
	Out.push_back("frag\t" + LaunchCarry(1.38));
	Out.push_back("frag\t" + VerdictDespite(0));
	Out.push_back("frag\t" + VerdictDespite(1));
	Out.push_back("frag\t" + LiveDropLine(212));
	Out.push_back("frag\t" + LiveDropHint());
	Out.push_back("frag\t" + StreakLine(6, 1));
	Out.push_back("frag\t" + StreakLine(1, 0));
	Out.push_back("frag\t" + StreakLine(8, 2));
	Out.push_back("frag\t" + WouldEarnLine(410));
	for (int Tg = 1; Tg <= 3; ++Tg)
	{
		Out.push_back(std::string("label\t") + TagName(Tg));
	}
	Out.push_back("subject\t" + LimitedSubject());
	Out.push_back("body\t" + LimitedBody("Is the Arcade soft? My first week"));
	return Out;
}

// ================================================================== a midnight's decision for one video

struct Decision
{
	size_t Index = 0;
	int Stage = 0;  // 1 the verdict, 2 a check-in
	int Check = 0;  // the check-in's day (7, 14, 30, 90); 0 at the verdict
	bool Merged = false; // Strategy's day 30 taking day 90's share (a rank of 30 or more at day 30)
	double S = 0.0;
};

/** The kept videos newer than Index that count for a rank (Grant's synthetic video aside). */
int RankOf(const Channel& C, size_t Index, double AtOrBefore)
{
	int N = 0;
	for (size_t J = Index + 1; J < C.Videos.size(); ++J)
	{
		N += !IsGrant(C.Videos[J]) && C.Videos[J].At <= AtOrBefore ? 1 : 0;
	}
	return N;
}

double MedianOf(std::vector<double> X)
{
	if (X.empty())
	{
		return 0.0;
	}
	std::sort(X.begin(), X.end());
	return X[X.size() / 2];
}

/** The median P of the (up to) ten kept uploads before Index; 0 until five are there. */
double TypicalBefore(const Channel& C, size_t Index)
{
	std::vector<double> Ps;
	for (size_t J = Index; J-- > 0 && Ps.size() < static_cast<size_t>(VaBingeTypical);)
	{
		if (!IsGrant(C.Videos[J]))
		{
			Ps.push_back(C.Videos[J].P);
		}
	}
	return Ps.size() >= static_cast<size_t>(VaBingeHistory) ? MedianOf(Ps) : 0.0;
}

/** The Binge wave (SPEC_CREATORS §3.3): 4% of the hit's excess to each of the three newest kept Update 3 uploads before it.
 *  Returns how many really picked up a part (the bell's count: a skipped slot is no video "getting views again"). */
int BingeWave(Channel& C, size_t Index, double Excess, int Day)
{
	int Lit = 0, Fitted = 0;
	for (size_t J = Index; J-- > 0 && Lit < VaBingeTargets;)
	{
		Video& T = C.Videos[J];
		if (!T.X.On)
		{
			continue; // legacy videos are skipped (V6)
		}
		++Lit; // the slot counts whether or not it fits: a skipped slot moves nothing
		if (CompFits(T, Day))
		{
			Comp K;
			K.T = static_cast<double>(Day) * VaDay;
			K.P = VaBingeShare * Excess;
			K.A = VaBingeA;
			K.Kind = CompKind::Binge;
			K.SrcMix = 10;
			K.CtrF = 1.0f;
			AddComp(T.X, K);
			++Fitted;
		}
	}
	return Fitted;
}

Comp KickComp(const Video& V, double T, double P, CompKind Kind, uint8_t Mix, double S)
{
	Comp K;
	K.T = T;
	K.P = P;
	K.A = ShapeU3(V.C).A;
	K.Kind = Kind;
	K.SrcMix = Mix;
	K.CtrF = static_cast<float>(0.6 * std::exp(-0.8 * std::max(0.0, S)));
	return K;
}

void Decide(Channel& C, life::State& L, const Decision& D, int Day)
{
	(void)L;
	Video& V = C.Videos[D.Index];
	VideoX& X = V.X;
	const double D1 = static_cast<double>(Day) * VaDay;
	// Rng(Fate:kick:<D1>): the chance, N, the breakout's roll and size, always in that order; Late Bloomer's draw after them.
	Rng R(std::to_string(X.Fate) + ":kick:" + std::to_string(Day));
	const double UChance = R.Next();
	const double UN = R.Next();
	const double UBreak = R.Next();
	const double USize = R.Next();
	const double N = 0.5 + UN;
	std::string Line;
	int LinePriority = -1;
	auto Say = [&Line, &LinePriority](const std::string& What, int Priority) {
		if (Priority > LinePriority)
		{
			Line = What;
			LinePriority = Priority;
		}
	};
	const bool Broke = std::any_of(X.Comps.begin(), X.Comps.end(), [](const Comp& K) { return K.Kind == CompKind::Breakout; });
	bool BrokeNow = false;
	double BreakSize = 0.0;
	const double Kick = PerfKick(V);
	if (D.Stage == 1)
	{
		const double Size = X.P0 * algo::KappaRegular(D.S) * ShareOf(V.C, 0) * N * Kick;
		AddComp(X, KickComp(V, D1, Size, CompKind::Verdict, 1, D.S));
		// The label and Momentum read the video's own standing (S0, the upload's score as repackages and load switches left it)
		// against S0_base: the launch actions are bonus-only, in this video's kicks and nowhere after (SPEC_CREATORS §3.3's sx).
		X.Verdict = algo::VerdictLabel(Perf(X), X.S0, X.S0Base);
		Say(BellVerdict(V.Title, X.Verdict), 1);
	}
	else
	{
		const double Share = ShareOf(V.C, D.Check) + (D.Merged ? ShareOf(V.C, 90) : 0.0);
		if (UChance < VaCheckChance)
		{
			const CompKind Kind = D.Check == 7 ? CompKind::Check7 : D.Check == 14 ? CompKind::Check14 : D.Check == 30 ? CompKind::Check30 : CompKind::Check90;
			const uint8_t Mix = D.Check >= 30 ? 3 : 2;
			const double Size = X.P0 * algo::KappaRegular(D.S) * Share / VaCheckChance * N * Kick;
			AddComp(X, KickComp(V, D1, Size, Kind, Mix, D.S));
			if (Size >= 0.3 * X.P0)
			{
				Say(BellKick(V.Title), 1);
			}
		}
	}
	// The breakout comes out of the kicks (verdict, day 7, day 14), once a video.
	if (!Broke && (D.Stage == 1 || D.Check == 7 || D.Check == 14))
	{
		const double Sum = ShareOf(V.C, 0) + ShareOf(V.C, 7) + ShareOf(V.C, 14);
		const double Chance = algo::BreakoutChance(D.S) * ShareOf(V.C, D.Stage == 1 ? 0 : D.Check) / Sum;
		if (UBreak < Chance)
		{
			BreakSize = X.P0 * (3.0 + 5.0 * USize);
			Comp K;
			K.T = D1;
			K.P = BreakSize;
			K.A = VaBreakoutA;
			K.Kind = CompKind::Breakout;
			K.SrcMix = 4;
			K.CtrF = static_cast<float>(0.6 * std::exp(-0.8 * std::max(0.0, D.S)));
			AddComp(X, K);
			BrokeNow = true;
			Say(BellBreakout(V.Title), 3);
		}
	}
	// Late Bloomer (SPEC_CREATORS §3.3): a Strategy lesson's day-30 or day-90 check-in, one more draw on the kick fork.
	if (V.C == Class::Strategy && (D.Check == 30 || D.Check == 90))
	{
		const double UBloom = R.Next();
		if (UBloom < VaBloomChance && CompFits(V, Day))
		{
			Comp K;
			K.T = D1;
			K.P = VaBloomSize * X.P0;
			K.A = VaBloomA;
			K.Kind = CompKind::LateBloom;
			K.SrcMix = 11;
			K.CtrF = 1.0f;
			AddComp(X, K);
			// (Its bell rings at the next midnight, with its first day against its usual one: MidnightU3, C3 fix L4.)
		}
	}
	if (D.Stage == 1)
	{
		// The Duel: the contender with the largest share of watch time (CTR x its own retention) takes over.
		if (X.Duels >= 2 && X.DuelWin < 0)
		{
			int Best = 0;
			double BestShare = -1.0;
			for (int K = 0; K < X.Duels && K < 3; ++K)
			{
				const int Over = std::max(0, std::max(static_cast<int>(X.Promise), static_cast<int>(ThumbPromise(X.Spec[static_cast<size_t>(K)]))) - static_cast<int>(X.Payoff));
				const int Under = std::max(0, static_cast<int>(X.Payoff) - std::max(static_cast<int>(X.Promise), static_cast<int>(ThumbPromise(X.Spec[static_cast<size_t>(K)]))));
				const double Share = static_cast<double>(X.DuelCtr[static_cast<size_t>(K)]) * (1.0 - 0.10 * Over) * (1.0 + 0.02 * Under);
				if (Share > BestShare)
				{
					BestShare = Share;
					Best = K;
				}
			}
			const double NewCtr = static_cast<double>(X.DuelCtr[static_cast<size_t>(Best)]);
			if (CompFits(V, Day) && X.Ctr > 0.0)
			{
				Comp K = algo::RepackComp(V, D1, NewCtr);
				K.Kind = CompKind::Duel;
				AddComp(X, K);
				X.Ctr = VaRound(NewCtr, 1e7);
			}
			X.DuelWin = static_cast<int8_t>(Best);
			Say(BellDuel(Best), 0);
		}
		// A link: a Cut sends viewers to the video it was made from; a Next-up card sends 2% of what's still to come.
		if (!X.NextUp.empty())
		{
			for (Video& T : C.Videos)
			{
				if (&T == &V || T.Id != X.NextUp || !T.X.On)
				{
					continue;
				}
				const CompKind Kind = V.F == Format::Short ? CompKind::FromCut : CompKind::FromNextUp;
				const bool Has = std::any_of(T.X.Comps.begin(), T.X.Comps.end(), [Kind](const Comp& K) { return K.Kind == Kind; });
				if (!Has && CompFits(T, Day))
				{
					Comp K;
					K.T = D1;
					if (Kind == CompKind::FromCut)
					{
						const int Over = OverOf(V);
						const double Q = QOf(V, RatesOf(V, X.AdsAtUpload, V.Retention, Over), V.Retention, Over);
						K.P = 0.03 * (1.0 + Q) * V.P;
						K.A = VaFromCutA;
						K.SrcMix = 7;
					}
					else
					{
						K.P = 0.02 * RemainingU3(V, D1);
						K.A = ShapeU3(T.C).A;
						K.SrcMix = 8;
					}
					K.Kind = Kind;
					K.CtrF = 1.0f;
					if (K.P > 0.0)
					{
						AddComp(T.X, K);
					}
				}
				break;
			}
		}
	}
	// The Binge wave: a hit (3x the channel's typical P) lights up the three uploads before it; a breakout too, once.
	if (D.Stage == 1 || BrokeNow)
	{
		const double Typical = TypicalBefore(C, D.Index);
		const bool HitAtUpload = Typical > 0.0 && V.P >= VaBingeAt * Typical;
		double Excess = 0.0;
		if (D.Stage == 1 && HitAtUpload)
		{
			Excess = V.P - Typical;
		}
		else if (BrokeNow && Typical > 0.0 && !HitAtUpload && V.P + BreakSize >= VaBingeAt * Typical)
		{
			Excess = V.P + BreakSize - Typical;
		}
		if (Excess > 0.0)
		{
			// (The wave's bell rings at the next midnight with the views its parts really added that day: MidnightU3, C3 fix L4.)
			BingeWave(C, D.Index, Excess, Day);
		}
	}
	// Momentum (SPEC_CREATORS §3.3): H reads how the video really did (its own luck) plus the compressed score part; a
	// breakout or a narrated viral roll adds 0.5. (The Binge wave pushed components onto older videos, never onto V.)
	ChannelX& Cx = C.X;
	if (D.Stage == 1)
	{
		const double Sx = X.S0 - X.S0Base;
		const double Part = Sx / (1.0 + std::fabs(Sx) / VaLmaxVideo); // ln Compress(e^Sx, 0.30)
		Cx.MomH = VaRound(0.6 * Cx.MomH + VaMomWeight * (Perf(X) + Part) + ((X.Viral || BrokeNow) ? VaMomBreakout : 0.0), 1e6);
		Cx.Momentum = MomCOf(Cx.MomH);
		Cx.MomShown = VaRound(0.5 * Cx.MomShown + 0.5 * Cx.Momentum, 1e6);
		for (size_t K = 0; K + 1 < Cx.Settled.size(); ++K)
		{
			Cx.Settled[K] = Cx.Settled[K + 1];
		}
		Cx.Settled.back() = static_cast<float>(Perf(X));
		Cx.SettledN = std::min(static_cast<int>(Cx.Settled.size()), Cx.SettledN + 1);
		if (X.Viral)
		{
			Say(BellViral(V.Title), 4);
		}
	}
	else if (BrokeNow && !X.Viral)
	{
		// (A narrated viral video already took its +0.5 at the verdict: one breakout story a video, as the model has it.)
		Cx.MomH = VaRound(Cx.MomH + VaMomBreakout, 1e6);
		Cx.Momentum = MomCOf(Cx.MomH);
	}
	if (!Line.empty())
	{
		PushBell(C, D1, Line);
	}
}
} // namespace valgo

namespace algo
{
double Kappa(double S)
{
	return std::clamp(valgo::VaKappaNeutral * std::exp(S), valgo::VaKappaLo, valgo::VaKappaHi);
}

double BreakoutChance(double S)
{
	return std::min(0.03, 0.004 * std::exp(1.2 * S));
}

double KappaRegular(double S)
{
	return std::max(0.0, Kappa(S) - valgo::VaBreakoutMeanSize * BreakoutChance(S));
}

double Compress(double M, double Lmax)
{
	if (M <= 0.0 || Lmax <= 0.0)
	{
		return M <= 0.0 ? 0.0 : 1.0;
	}
	const double L = std::log(M);
	return std::exp(L / (1.0 + std::fabs(L) / Lmax));
}

namespace algo_season
{
/** The advertisers' calendar (§4.5.3), before each civil year's mean is divided out. */
double SeasonRaw(int Month, int DayOfMonth)
{
	switch (Month)
	{
	case 1: return 0.70;
	case 2: return 0.80;
	case 3: return 0.90;
	case 4: return 0.92;
	case 5: return 0.95;
	case 6: return 0.95;
	case 7: return 0.92;
	case 8: return 0.98;
	case 9: return 1.03;
	case 10: return 1.10;
	case 11: return DayOfMonth >= 23 ? 1.50 : 1.35;
	case 12: return DayOfMonth >= 26 ? 1.02 : 1.37;
	default: return 1.0;
	}
}

double SeasonMean(int Year)
{
	const bool Leap = (Year % 4 == 0 && Year % 100 != 0) || Year % 400 == 0;
	const int Days[12] = {31, Leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	double Sum = 0.0;
	int Count = 0;
	for (int Mo = 1; Mo <= 12; ++Mo)
	{
		for (int Dm = 1; Dm <= Days[Mo - 1]; ++Dm)
		{
			Sum += SeasonRaw(Mo, Dm);
			++Count;
		}
	}
	return Sum / static_cast<double>(Count);
}
} // namespace algo_season

double Season(int WorldDay)
{
	int Y = 0, Mo = 0, Dm = 0;
	world::CivilDate(WorldDay, Y, Mo, Dm);
	return algo_season::SeasonRaw(Mo, Dm) / algo_season::SeasonMean(Y);
}

double AdsPerPlayback(Format F, LengthOpt L, AdLoad A)
{
	if (F == Format::Short)
	{
		return valgo::VaCutAdsPerPlayback;
	}
	const double Table[3][3][3] = {
		{{1.00, 1.00, 1.00}, {1.00, 1.20, 1.55}, {1.00, 1.36, 1.90}}, // Breakdown: Tight 6:00 (no mid-rolls), Standard 8:00, Extended 12:00
		{{1.00, 1.30, 1.75}, {1.00, 1.68, 2.40}, {1.00, 1.84, 2.75}}, // Recap: 10:00, 14:00, 20:00
		{{1.00, 1.36, 1.90}, {1.00, 1.68, 2.40}, {1.00, 1.90, 2.85}}, // Vlog: 12:00, 18:00, 25:00
	};
	return Table[valgo::LongRow(F)][std::clamp(static_cast<int>(L), 0, 2)][std::clamp(static_cast<int>(A), 0, 2)];
}

Money MoneyOf(const Video& V, int WorldDay)
{
	// The identity on one day at that day's load: CPM x MonetizedRate x AdsPerPlayback x Share is the RPM actually paid
	// (RetentionPays is inside the CPM: advertisers pay more for the videos people watch).
	Money M;
	const bool Cut = V.F == Format::Short;
	const AdLoad Load = V.X.On ? valgo::LoadOn(V.X, WorldDay) : AdLoad::Standard;
	const LengthOpt Len = V.X.On ? V.X.Len : LengthOpt::Standard;
	M.Share = Cut ? valgo::VaCutShare : valgo::VaCreatorShare;
	M.MonetizedRate = Cut ? 1.0 : (V.X.On && V.X.C == Coin::Limited) ? valgo::VaMonetizedLimited : valgo::VaMonetized;
	M.AdsPerPlayback = AdsPerPlayback(V.F, Len, Load);
	M.Rpm = valgo::RpmOn(V, WorldDay) * Season(WorldDay);
	M.Cpm = M.Rpm / (M.MonetizedRate * M.AdsPerPlayback * M.Share);
	M.RetentionPays = 1.0;
	return M;
}

Score ScoreAt(const Video& V, const Channel& C, double World, int Stage)
{
	Score Out;
	const int Over = valgo::OverOf(V);
	const valgo::Rates R = valgo::RatesOf(V, V.X.On ? valgo::LoadOn(V.X, valgo::DayOf(World)) : AdLoad::Standard, V.Retention, Over);
	Out.Ctr = V.X.Ctr;
	Out.Q = valgo::QOf(V, R, V.Retention, Over);
	if (Stage <= 0)
	{
		// The upload's score from its packaging (the trend term at the drop aside: it lives in the saved S0).
		Out.V = valgo::TimingV(valgo::TimeFitAt(C, V.At)) + valgo::AdScore(V.F, V.X.Len, V.X.AdsAtUpload);
		int Episodes = 0;
		for (const Video& O : C.Videos)
		{
			Episodes += &O != &V && O.X.On && V.X.Series >= 0 && O.X.Series == V.X.Series && O.At < V.At ? 1 : 0;
		}
		Out.Se = Episodes > 0 ? 0.05 : 0.0;
		Out.S = std::clamp(0.9 * std::log(std::max(1e-6, V.X.Ctr) / 0.0475) + Out.Q + Out.V + Out.Tr + Out.Se, -2.0, 2.0);
		return Out;
	}
	Out.S = valgo::ScoreNow(V, C, valgo::DayOf(World), Stage);
	Out.V = Out.S - V.X.S0;
	return Out;
}

double ScoreBase(const Video& V, const Channel& /*C*/)
{
	return V.X.On ? V.X.S0Base : 0.0;
}

int8_t VerdictLabel(double Perf, double S, double S0Base)
{
	const double Rp = Perf + 0.5 * (S - S0Base);
	return static_cast<int8_t>(Rp < -0.8 ? 0 : Rp < -0.1 ? 1 : Rp < 0.6 ? 2 : Rp < 1.2 ? 3 : 4);
}

void DecideDay(Channel& C, life::State& L, int Day, const std::vector<Trend>* /*Week*/)
{
	// Two phases (§4.6.3): every decision of this midnight is scored from state as of it, then the components are appended in
	// upload order. Eligibility goes by upload rank among the newest 60, so folded and unfolded runs decide the same (I4).
	using namespace valgo;
	if (!C.X.On)
	{
		return;
	}
	const double D1 = static_cast<double>(Day) * VaDay;
	std::vector<Decision> Todo;
	// The newest 60 uploads by rank (newest first, Grant's synthetic video aside), walked oldest first.
	std::vector<size_t> Eligible;
	for (size_t I = C.Videos.size(); I-- > 0 && Eligible.size() < 60;)
	{
		if (C.Videos[I].At < D1 && !IsGrant(C.Videos[I]))
		{
			Eligible.push_back(I);
		}
	}
	std::reverse(Eligible.begin(), Eligible.end());
	for (const size_t I : Eligible)
	{
		const Video& V = C.Videos[I];
		if (!V.X.On)
		{
			continue;
		}
		Decision D;
		D.Index = I;
		const int Vd = VerdictDayOf(V);
		const int D0 = DayOf(V.At);
		if (Day == Vd)
		{
			D.Stage = 1;
		}
		else if (Day > Vd)
		{
			const int Checks[4] = {7, 14, 30, 90};
			for (const int Ck : Checks)
			{
				if ((Ck <= 14 || V.C == Class::Strategy) && D0 + Ck == Day)
				{
					D.Stage = 2;
					D.Check = Ck;
				}
			}
			if (D.Stage == 2 && V.C == Class::Strategy && (D.Check == 30 || D.Check == 90))
			{
				// A Strategy video whose upload rank at its day-30 midnight is 30 or more takes its day-90 share then.
				const bool Late = RankOf(C, I, static_cast<double>(D0 + 30) * VaDay) >= 30;
				if (D.Check == 30)
				{
					D.Merged = Late;
				}
				else if (Late)
				{
					D.Stage = 0;
				}
			}
			if (D.Stage == 2)
			{
				// A repackage since the last decision skips this check-in ("Rerun needs a day to learn a new look").
				const int Prev = D.Check == 7 ? Vd : D0 + (D.Check == 14 ? 7 : D.Check == 30 ? 14 : 30);
				if (V.X.LastRepackDay >= Prev)
				{
					D.Stage = 0;
				}
			}
		}
		if (D.Stage > 0)
		{
			Todo.push_back(D);
		}
	}
	for (Decision& D : Todo)
	{
		D.S = ScoreNow(C.Videos[D.Index], C, Day, D.Stage);
	}
	for (const Decision& D : Todo)
	{
		Decide(C, L, D, Day);
	}
}

double PeakHour(const Channel& /*C*/, int Weekday)
{
	int Best = 0;
	for (int H = 1; H < 24; ++H)
	{
		Best = valgo::HourHeat(Weekday, H) > valgo::HourHeat(Weekday, Best) ? H : Best;
	}
	return static_cast<double>(Best);
}

Comp RepackComp(const Video& V, double T0, double NewCtr)
{
	// The exact rescale (§4.6.7): every component's unrealized part at T0, times c'/c - 1, as one component.
	const valgo::VaShape Sh = valgo::ShapeU3(V.C);
	double Fast = 0.0, Slow = 0.0;
	auto Add = [&](double T, double P, double A) {
		if (T > T0)
		{
			return; // (a component that starts at T0 itself, the verdict's kick under a Duel's switch, is all still to come)
		}
		const double Age = (T0 - T) / valgo::VaDay;
		Fast += P * A * std::exp(-Age / Sh.Tau1);
		Slow += P * (1.0 - A) * std::exp(-Age / Sh.Tau2);
	};
	Add(V.At, V.X.P0, Sh.A);
	for (const Comp& K : V.X.Comps)
	{
		Add(K.T, K.P, K.A);
	}
	Comp Out;
	Out.T = T0;
	Out.Kind = CompKind::Repackage;
	Out.SrcMix = 5;
	const double Ratio = V.X.Ctr > 0.0 ? NewCtr / V.X.Ctr - 1.0 : 0.0;
	Out.P = Ratio * (Fast + Slow);
	Out.A = Fast + Slow > 0.0 ? Fast / (Fast + Slow) : Sh.A;
	Out.CtrF = static_cast<float>(V.X.Ctr > 0.0 ? NewCtr / V.X.Ctr : 1.0);
	return Out;
}

double Momentum(const Channel& C, double /*World*/)
{
	return C.X.On ? valgo::MomCOf(C.X.MomH) : 1.0;
}

double ExtraHours7(const Channel& C, double World)
{
	// Update 3's extra editing finished in the last 7 days (§4.6.6), dated by each video's At: a Duel's extra contenders
	// (15 minutes each), the launch actions (a Cut 20, a teaser 10, a reply round 20), Extended length over Standard, and a
	// clean edit's 15 (a strong-language upload that kept full ads had one: StrongLanguage).
	double Minutes = 0.0;
	for (const Video& V : C.Videos)
	{
		if (!V.X.On || V.At <= World - 7.0 * valgo::VaDay || V.At > World)
		{
			continue;
		}
		Minutes += 15.0 * static_cast<double>(std::max(0, V.X.Duels - 1));
		Minutes += (V.X.Launch & (1u << static_cast<int>(Launch::Cut))) != 0 ? 20.0 : 0.0;
		Minutes += (V.X.Launch & (1u << static_cast<int>(Launch::Teaser))) != 0 ? 10.0 : 0.0;
		Minutes += (V.X.Launch & (1u << static_cast<int>(Launch::Reply))) != 0 ? 20.0 : 0.0;
		Minutes += V.X.Len == LengthOpt::Extended ? valgo::EditMinutesFor(V.F, LengthOpt::Extended) - valgo::EditMinutesFor(V.F, LengthOpt::Standard) : 0.0;
		Minutes += valgo::StrongLanguage(V) && V.X.C == Coin::Full ? valgo::VaCleanEditMinutes : 0.0;
	}
	return Minutes / 60.0;
}

void NoteStream(Channel& C, double Start, double End, int /*PeakViewers*/, double AvgOwn, double Norm)
{
	// Live Drop (SPEC_CREATORS §3.4): a stream of 30+ world minutes that starts within 6 world hours after a publish gives
	// that video V +0.10 at its verdict and a Kast component of 1.2 x min(AvgOwn, 1.5 x Norm) x min(2, hours live), A 0.9,
	// once a video. AvgOwn leaves out raiders and the Broadcast; the cap stops a final-table night pumping it.
	using namespace valgo;
	if (!C.X.On || End - Start < VaDropMinutes)
	{
		return;
	}
	const uint8_t Bit = static_cast<uint8_t>(1u << static_cast<int>(Launch::Kast));
	for (Video& V : C.Videos)
	{
		if (!V.X.On || (V.X.Launch & Bit) != 0 || Start < V.At || Start > V.At + VaDropWindow)
		{
			continue;
		}
		V.X.Launch = static_cast<uint8_t>(V.X.Launch | Bit);
		const double Hours = std::min(VaDropHours, (End - Start) / 60.0);
		const double Own = std::min(std::max(0.0, AvgOwn), VaDropCap * std::max(1.0, Norm));
		const double Push = VaDropX * Own * Hours;
		if (Push > 0.0 && CompFits(V, DayOf(End)))
		{
			Comp K;
			K.T = End;
			K.P = Push;
			K.A = VaDropA;
			K.Kind = CompKind::Kast;
			K.SrcMix = 9;
			K.CtrF = static_cast<float>(Own); // (a Kast part carries no impressions: its CtrF keeps the stream's own average)
			AddComp(V.X, K);
		}
	}
}

std::vector<std::pair<double, double>> KastHeatSince(const Channel& C, double Since)
{
	// SPEC_CREATORS §3.3: "Pushed hard" +6, a breakout or a narrated viral roll +10, each with its own world minute, oldest
	// first. Kast decays each from its time and caps them (+12 a world week, only after a week with a stream) at Start.
	std::vector<std::pair<double, double>> Out;
	if (!C.X.On)
	{
		return Out;
	}
	for (const Video& V : C.Videos)
	{
		if (!V.X.On)
		{
			continue;
		}
		// One story, one bump (the model's): the narrated viral roll and a breakout are both "a breakout", +10 once a video;
		// "Pushed hard" is +6 at its verdict unless that same midnight is the breakout's.
		const double Verdict = static_cast<double>(valgo::VerdictDayOf(V)) * valgo::VaDay;
		const bool BreakAtVerdict = std::any_of(V.X.Comps.begin(), V.X.Comps.end(), [Verdict](const Comp& K) { return K.Kind == CompKind::Breakout && K.T == Verdict; });
		if (V.X.Verdict >= 0 && Verdict > Since && Verdict <= C.SteppedTo)
		{
			if (V.X.Viral || BreakAtVerdict)
			{
				Out.push_back({Verdict, valgo::VaHeatBreak});
			}
			else if (V.X.Verdict == 4)
			{
				Out.push_back({Verdict, valgo::VaHeatHard});
			}
		}
		for (const Comp& K : V.X.Comps)
		{
			if (K.Kind == CompKind::Breakout && !V.X.Viral && K.T != Verdict && K.T > Since && K.T <= C.SteppedTo)
			{
				Out.push_back({K.T, valgo::VaHeatBreak});
			}
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const std::pair<double, double>& A, const std::pair<double, double>& B) { return A.first < B.first; });
	return Out;
}
} // namespace algo
} // namespace video
} // namespace ss
