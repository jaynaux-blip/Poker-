// Update 3 (SPEC §5, §7.3): Rerun 2.0's analytics, core-videoalgo's file: every number the Analytics pages draw, the
// model's own (V4), derived from the saved fields, the components and the rings; an Update 2 video's from Fate =
// Fnv1a(Id), marked Estimated (V6). Nothing here is saved and nothing draws from an Rng: every read at a minute is the same
// after any reload. The UI reads these and computes no number of its own.
#include "ShortStack/Game/VideoStats.h"

#include "VideoAlgoPriv.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoContent.h"
#include "ShortStack/Game/World.h"

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
namespace video_stats_detail
{
constexpr double StDay = 1440.0;

const char* WeekdayShort(int Day)
{
	const char* const Names[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	return Names[((Day % 7) + 7) % 7];
}

/** "Tue 3 AM" for a world minute. */
std::string WhenText(double World)
{
	const int D = valgo::DayOf(World);
	const int Hour = static_cast<int>(std::floor(std::fmod(World, StDay) / 60.0));
	const int H12 = Hour % 12 == 0 ? 12 : Hour % 12;
	return std::string(WeekdayShort(D)) + " " + std::to_string(H12) + (Hour < 12 ? " AM" : " PM");
}

/** "3:41" for minutes. */
std::string Duration(double Minutes)
{
	const int Seconds = static_cast<int>(std::lround(std::max(0.0, Minutes) * 60.0));
	char Buf[24];
	std::snprintf(Buf, sizeof(Buf), "%d:%02d", Seconds / 60, Seconds % 60);
	return Buf;
}

std::string Percent(double X, int Digits)
{
	return Fixed(100.0 * X, Digits) + "%";
}

std::string Quoted(const std::string& Title)
{
	return "\xE2\x80\x9C" + Title + "\xE2\x80\x9D";
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

/** One part of a video's curve: its start, its views to come, its A and its mix. */
struct Part
{
	double T = 0, P = 0, A = 0;
	uint8_t Mix = 0;
	float CtrF = 1.0f;
	CompKind Kind = CompKind::Verdict;
	bool First = false;
};

std::vector<Part> PartsOf(const Video& V)
{
	std::vector<Part> Out;
	const valgo::VaShape S = valgo::ShapeU3(V.C);
	Part F;
	F.T = V.At;
	F.P = V.X.On ? V.X.P0 : V.P;
	F.A = V.X.On ? S.A : V.A;
	F.First = true;
	Out.push_back(F);
	for (const Comp& K : V.X.Comps)
	{
		Part P;
		P.T = K.T;
		P.P = K.P;
		P.A = K.A;
		P.Mix = K.SrcMix;
		P.CtrF = K.CtrF;
		P.Kind = K.Kind;
		Out.push_back(P);
	}
	return Out;
}

double PartViews(const Video& V, const Part& P, double World)
{
	const valgo::VaShape S = valgo::ShapeU3(V.C);
	const double Tau1 = V.X.On ? S.Tau1 : V.Tau1, Tau2 = V.X.On ? S.Tau2 : V.Tau2;
	return P.P * valgo::CurveFrac(P.A, Tau1, Tau2, (World - P.T) / StDay);
}

/** The evergreen term's views to World (Strategy, Update 2's rule). */
double EvergreenViews(const Video& V, double World)
{
	return V.E * std::clamp((World - V.At) / StDay, 0.0, 365.0);
}

/** A source's CTR for a part (the rolled CTR, its source's rel, the part's factor; the first wave normalized to blend exactly). */
double SourceCtr(const Video& V, const Part& P, int Source)
{
	const double Rel = valgo::SourceRel(Source);
	if (Rel <= 0.0)
	{
		return 0.0;
	}
	const double Ctr = std::max(1e-6, valgo::RolledCtr(V));
	return P.First ? Ctr * Rel * valgo::FirstWaveNorm(V) : Ctr * Rel * std::max(1e-6, static_cast<double>(P.CtrF));
}

/** The 61-sample retention curve (§5.4 Engagement): a start at 100%, the hook at 0:30, a body whose decay is solved so the
 *  trapezoid mean is exactly r after the moments' bumps and the mid-rolls' dips, and an end drop over the last 3%. */
std::array<float, 61> CurveOf(const Video& V, double R, int Over, double Q, AdLoad Load, std::vector<std::pair<float, std::string>>* Moments)
{
	std::array<float, 61> Out{};
	const double Length = std::max(0.5, V.LengthMin);
	const double Hook = std::clamp(0.78 + (Q > 0.0 ? 0.04 : 0.0) - 0.08 * static_cast<double>(Over) - (Load == AdLoad::Heavy ? 0.02 : 0.0), 0.30, 0.98);
	const double Xh = std::clamp(0.5 / Length, 0.02, 0.6);
	// The moments: where the hand turned, from the format (a Breakdown's all in and river, a Recap's three, a Vlog's hands).
	std::vector<std::pair<double, std::string>> Marks;
	switch (V.F)
	{
	case Format::Recap: Marks = {{0.30, "the first big hand"}, {0.60, "the turning point"}, {0.86, "the finish"}}; break;
	case Format::Vlog: Marks = {{0.25, "the first hand"}, {0.50, "the second hand"}, {0.75, "the third hand"}}; break;
	default: Marks = {{0.48, "the all in"}, {0.74, "the river"}}; break;
	}
	const uint32_t Fate = V.X.On ? V.X.Fate : Fnv1a(V.Id);
	std::vector<double> Heights;
	for (size_t I = 0; I < Marks.size(); ++I)
	{
		Heights.push_back(0.06 + 0.04 * (valgo::FateNormal(Fate, 10 + static_cast<int>(I)) + 3.0) / 3.0); // 0.06-0.14
	}
	// The mid-rolls: one at 4:00 then about every 6 minutes at Standard, every 3 at Heavy, none at Light (and none under 8:00).
	std::vector<double> Rolls;
	const LengthOpt Len = V.X.On ? V.X.Len : LengthOpt::Standard;
	if (V.F != Format::Short && algo::AdsPerPlayback(V.F, Len, Load) > 1.0)
	{
		const double Every = Load == AdLoad::Heavy ? 3.0 : 6.0;
		for (double M = Load == AdLoad::Heavy ? 3.0 : 4.0; M < Length - 0.5; M += Every)
		{
			Rolls.push_back(M / Length);
		}
	}
	const double Dip = Load == AdLoad::Heavy ? 0.06 : 0.03;
	auto Build = [&](double K) {
		std::array<float, 61> C{};
		for (int I = 0; I <= 60; ++I)
		{
			const double X = static_cast<double>(I) / 60.0;
			double Y = 0.0;
			if (X <= Xh)
			{
				Y = 1.0 + (Hook - 1.0) * X / Xh;
			}
			else
			{
				Y = Hook * std::exp(-K * (X - Xh));
				for (size_t B = 0; B < Marks.size(); ++B)
				{
					const double D = (X - Marks[B].first) / 0.035;
					Y += Heights[B] * Hook * std::exp(-0.5 * D * D);
				}
				for (const double Roll : Rolls)
				{
					Y *= X >= Roll ? 1.0 - Dip : 1.0;
				}
				if (X > 0.97)
				{
					Y *= 1.0 - 0.4 * (X - 0.97) / 0.03;
				}
			}
			C[static_cast<size_t>(I)] = static_cast<float>(std::clamp(Y, 0.0, 1.0));
		}
		return C;
	};
	auto Mean = [](const std::array<float, 61>& C) {
		double S = 0.0;
		for (int I = 0; I < 60; ++I)
		{
			S += 0.5 * (static_cast<double>(C[static_cast<size_t>(I)]) + static_cast<double>(C[static_cast<size_t>(I + 1)]));
		}
		return S / 60.0;
	};
	double Lo = 0.0, Hi = 30.0;
	if (Mean(Build(Lo)) <= R)
	{
		Out = Build(Lo);
	}
	else
	{
		for (int It = 0; It < 60; ++It)
		{
			const double Mid = 0.5 * (Lo + Hi);
			(Mean(Build(Mid)) > R ? Lo : Hi) = Mid;
		}
		Out = Build(0.5 * (Lo + Hi));
	}
	if (Moments)
	{
		for (size_t B = 0; B < Marks.size(); ++B)
		{
			Moments->push_back({static_cast<float>(Marks[B].first), Marks[B].second});
		}
	}
	return Out;
}

/** The newest kept uploads (up to N) before or at World, newest first, of a format (Count for any). */
std::vector<const Video*> Recent(const Channel& C, double World, size_t N, Format F, bool AnyFormat)
{
	std::vector<const Video*> Out;
	for (auto It = C.Videos.rbegin(); It != C.Videos.rend() && Out.size() < N; ++It)
	{
		if (!valgo::IsGrant(*It) && It->At <= World && (AnyFormat || It->F == F))
		{
			Out.push_back(&*It);
		}
	}
	return Out;
}

double TotalOf(const Video& V)
{
	double T = (V.X.On ? V.X.P0 : V.P) + V.E * 365.0;
	for (const Comp& K : V.X.Comps)
	{
		T += K.P;
	}
	return T;
}

// ---------------------------------------------------------------- the copy (InventoryAlgo lists every line)

std::string LogFirstWave()
{
	return "your subscribers got it";
}

std::string LogHomeTest(int Verdict)
{
	return Verdict > 0 ? "tested on Home: people clicked more than usual" : Verdict < 0 ? "tested on Home: fewer people clicked than usual" : "tested on Home: about as many clicked as usual";
}

std::string LogPart(CompKind K, int Label, const std::string& Extra)
{
	switch (K)
	{
	case CompKind::Verdict:
		return Label <= 0 ? "Rerun stopped testing it" : Label == 1 ? "held: shown about as widely as your usual uploads" : Label >= 4 ? "pushed hard: shown to many more people on Home"
			: Label == 3 ? "pushed more: shown to more people on Home" : "pushed: shown to more people on Home";
	case CompKind::Check7: return "day 7: in Next up beside other poker videos";
	case CompKind::Check14: return "day 14: in Next up beside other poker videos";
	case CompKind::Check30: return "day 30: search viewers found it";
	case CompKind::Check90: return "day 90: search viewers still find it";
	case CompKind::Breakout: return "it started showing up next to other videos";
	case CompKind::Duel: return Extra + " won the thumbnail test on watch time";
	case CompKind::Repackage: return "title or thumbnail changed";
	case CompKind::Trend: return "a trending search found it";
	case CompKind::FromCut: return "your Cut sent viewers here";
	case CompKind::Kast: return Extra.empty() ? "Live Drop: your stream gave it a push" : "Live Drop: your stream gave it a push (" + Extra + " watching)";
	case CompKind::FromNextUp: return "viewers came from your Next-up card";
	case CompKind::Binge: return "new viewers bingeing your channel found it";
	case CompKind::LateBloom: return "it woke up: search viewers found it again";
	default: return "Rerun showed it to more people";
	}
}

std::string InsightClicksUp(double Ctr, double Typical)
{
	return "Viewers clicked your thumbnail more than usual (" + Percent(Ctr, 1) + " vs your typical " + Percent(Typical, 1) + ").";
}

std::string InsightClicksDown()
{
	return "Fewer viewers clicked than usual. A face or a number usually helps a bad beat.";
}

std::string InsightHonesty(double DislikeShare)
{
	return "The title promised more than the hand delivered. Viewers who clicked left early, and " + Percent(DislikeShare, 0) + " of ratings were dislikes.";
}

std::string InsightHoldUp(double Avd, double Typical)
{
	return "Viewers watched longer than your typical (" + Duration(Avd) + " vs " + Duration(Typical) + ").";
}

std::string InsightHoldDown()
{
	return "Most viewers left before the all in. A tighter cut gets there faster.";
}

std::string InsightAds(double Lost)
{
	return "The mid-roll ads lost " + Percent(Lost, 0) + " of viewers.";
}

std::string InsightVelocity()
{
	return "Your subscribers came within hours: posting before your 9 PM peak helped.";
}

std::string InsightLaunch()
{
	return "Your Kast stream right after posting brought the first viewers fast.";
}

std::string InsightPushed()
{
	return "Rerun started suggesting this video next to other poker videos.";
}

std::string InsightStopped()
{
	return "Rerun stopped testing this video after two days.";
}

std::string InsightCut(long long Viewers)
{
	return "Your Cut sent " + Grouped(Viewers) + " viewers to this one.";
}

std::string InsightNextUp(long long Viewers)
{
	return "Your Next-up card sent " + Grouped(Viewers) + " viewers to this one.";
}

std::string InsightSearch()
{
	return "Still growing: search viewers find it every day.";
}

std::string InsightBloom()
{
	return "Search viewers found it again a month after it went up.";
}

std::string InsightBinge()
{
	return "New viewers found it after your hit and kept watching.";
}

std::string InsightTrend()
{
	return "This video rode a trending topic: search viewers found it while it was hot.";
}

std::string InsightBurnout(double Hours)
{
	const long long H = std::llround(Hours);
	return "You put " + std::to_string(H) + (H == 1 ? " extra hour" : " extra hours") + " into that week's videos. It showed: viewers left sooner.";
}

std::string InsightRepeat(int Days)
{
	return Days <= 0 ? std::string("Your audience saw this idea earlier the same day.")
					 : "Your audience saw this idea " + std::to_string(Days) + (Days == 1 ? " day" : " days") + " earlier.";
}

std::string InsightSeasonUp(int Month)
{
	return Month == 12 ? "December: advertisers paid more for every view." : "November: advertisers paid more for every view.";
}

std::string InsightSeasonDown()
{
	return "January: the cheapest month for ads.";
}

std::string LegacyNote()
{
	return "Published before Rerun's new analytics.";
}
} // namespace video_stats_detail

// ================================================================== one video (§5.4)

VideoStats StatsOf(const Channel& C, const Video& V, double World)
{
	using namespace video_stats_detail;
	VideoStats S;
	S.Estimated = !V.X.On;
	const int Today = valgo::DayOf(World);
	const AdLoad Load = V.X.On ? valgo::LoadOn(V.X, Today) : AdLoad::Standard;
	const double R = valgo::RetentionOn(V, Today);
	const int Over = valgo::OverOf(V);
	const valgo::Rates Rt = valgo::RatesOf(V, Load, R, Over);
	const double Q = valgo::QOf(V, Rt, R, Over);
	const std::vector<Part> Parts = PartsOf(V);
	// Views, impressions and the sources, part by part.
	double ImprBearing = 0.0;
	for (const Part& P : Parts)
	{
		const double Seen = PartViews(V, P, World);
		const std::array<double, 10> Mix = valgo::MixOf(V, P.Mix);
		for (int Src = 0; Src < 10; ++Src)
		{
			const double Vs = Seen * Mix[static_cast<size_t>(Src)];
			S.Src.Views[static_cast<size_t>(Src)] += Vs;
			const double Ctr = SourceCtr(V, P, Src);
			if (Ctr > 0.0)
			{
				S.Src.Impressions[static_cast<size_t>(Src)] += Vs / Ctr;
				ImprBearing += Vs;
			}
		}
	}
	const double Ever = EvergreenViews(V, World);
	S.Src.Views[2] += Ever; // the evergreen search term
	S.Src.Impressions[2] += Ever / std::max(1e-6, valgo::RolledCtr(V) * valgo::SourceRel(2));
	ImprBearing += Ever;
	S.Views = Views(V, World);
	for (const double I : S.Src.Impressions)
	{
		S.Impressions += I;
	}
	S.Ctr48 = valgo::RolledCtr(V);
	S.CtrLife = S.Impressions > 0.0 ? ImprBearing / S.Impressions : S.Ctr48;
	S.AvgPct = R;
	S.AvdMin = V.LengthMin * R;
	S.WatchHours = S.Views * V.LengthMin * R / 60.0;
	const double Sigma = V.X.On ? V.X.Sigma : 0.3;
	S.Returning = std::clamp(0.5 * Sigma, 0.05, 0.60);
	S.Unique = S.Views / (1.0 + 0.6 * S.Returning);
	S.Likes = Rt.Like * S.Views;
	S.Dislikes = Rt.Dislike() * S.Views;
	S.Comments = Rt.Comment * S.Views;
	S.Shares = Rt.Share * S.Views;
	S.Saves = Rt.Save * S.Views;
	S.SubsNet = valgo::ConvOn(V, Today) * S.Views;
	S.SubsLost = S.SubsNet * valgo::LostShare(Q, Over, Load == AdLoad::Heavy);
	// Money (§4.5): the identity at today's load and season; the video's own double cents once it pays.
	const algo::Money M = algo::MoneyOf(V, Today);
	S.Rpm = M.Rpm;
	S.Cpm = M.Cpm;
	S.PlaybackCpm = M.Cpm * M.AdsPerPlayback;
	S.Monetized = S.Views * M.MonetizedRate;
	S.AdImpressions = S.Monetized * M.AdsPerPlayback;
	const bool Partner = C.PartnerAt >= 0.0 && C.PartnerAt <= World;
	if (V.X.On)
	{
		S.EarnedCents = static_cast<Chips>(std::llround(V.X.Earned));
	}
	else if (Partner)
	{
		const double Since = std::max(V.At, C.PartnerAt);
		S.EarnedCents = static_cast<Chips>(std::llround(std::max(0.0, Views(V, World) - Views(V, Since)) * V.Rpm / 1000.0 * 100.0));
	}
	if (!Partner)
	{
		// "As a Partner, this video would have earned about $4.10 so far (not paid)": the pipeline over the views so far, a day
		// at a time at each day's season (V9: views that already happened).
		double Cents = 0.0;
		for (int D = valgo::DayOf(V.At); static_cast<double>(D) * StDay < World; ++D)
		{
			const double A = std::max(V.At, static_cast<double>(D) * StDay), B = std::min(World, static_cast<double>(D + 1) * StDay);
			if (B > A)
			{
				Cents += std::max(0.0, Views(V, B) - Views(V, A)) * algo::MoneyOf(V, D).Rpm / 1000.0 * 100.0;
			}
		}
		S.WouldEarnCents = static_cast<Chips>(std::llround(Cents));
	}
	S.Coin = V.X.On ? static_cast<int>(V.X.C) : 0;
	S.Verdict = V.X.On ? V.X.Verdict : -1;
	// Retention: this video's curve and the typical one (the median of the last 10 of its format).
	S.Retention = CurveOf(V, R, Over, Q, Load, &S.Moments);
	{
		std::vector<std::array<float, 61>> Curves;
		for (const Video* O : Recent(C, V.At - 1.0, 10, V.F, false))
		{
			const int Oo = valgo::OverOf(*O);
			const double Or = valgo::RetentionOn(*O, Today);
			const AdLoad Ol = O->X.On ? valgo::LoadOn(O->X, Today) : AdLoad::Standard;
			Curves.push_back(CurveOf(*O, Or, Oo, valgo::QOf(*O, valgo::RatesOf(*O, Ol, Or, Oo), Or, Oo), Ol, nullptr));
		}
		for (size_t I = 0; I < 61; ++I)
		{
			std::vector<double> At;
			for (const auto& Cv : Curves)
			{
				At.push_back(static_cast<double>(Cv[I]));
			}
			S.Typical[I] = At.empty() ? S.Retention[I] : static_cast<float>(MedianOf(At));
		}
	}
	// The Duel: the realized first-48-hour CTRs and each one's share of watch time.
	if (V.X.On && V.X.Duels >= 2)
	{
		double Sum = 0.0;
		std::array<double, 3> Share{};
		for (size_t K = 0; K < 3 && static_cast<int>(K) < V.X.Duels; ++K)
		{
			// Watch time: each contender's clicks times how long its own promise keeps them (the verdict's rule).
			const int Promise = std::max(static_cast<int>(V.X.Promise), static_cast<int>(ThumbPromise(V.X.Spec[K])));
			const int Ov = std::max(0, Promise - static_cast<int>(V.X.Payoff)), Un = std::max(0, static_cast<int>(V.X.Payoff) - Promise);
			S.DuelCtr[K] = static_cast<float>(static_cast<double>(V.X.DuelCtr[K]) * V.X.CtrNoise);
			Share[K] = static_cast<double>(V.X.DuelCtr[K]) * (1.0 - 0.10 * Ov) * (1.0 + 0.02 * Un);
			Sum += Share[K];
		}
		for (size_t K = 0; K < 3; ++K)
		{
			S.DuelShare[K] = static_cast<float>(Sum > 0.0 ? Share[K] / Sum : 0.0);
		}
	}
	S.DuelWin = V.X.On ? V.X.DuelWin : -1;
	// The creator update's facts (§3): tags against the typical upload, the carry, Live Drop, the narrated roll, the waves.
	{
		std::vector<double> Ps;
		for (const Video* O : Recent(C, V.At - 1.0, 10, V.F, true))
		{
			Ps.push_back(O->P);
		}
		const double Typical = Ps.size() >= 5 ? MedianOf(Ps) : 0.0;
		const double Total = TotalOf(V);
		S.Tag = Typical <= 0.0 ? 0 : Total >= 10.0 * Typical ? 3 : Total >= 5.0 * Typical ? 2 : Total >= 2.0 * Typical ? 1 : 0;
	}
	S.MomCAtUpload = V.X.On && V.X.SettledS > 0.0f ? static_cast<double>(V.X.SettledS) : 1.0;
	S.LiveDrop = V.X.On && (V.X.Launch & (1u << static_cast<int>(Launch::Kast))) != 0;
	S.Viral = V.X.On && V.X.Viral;
	for (const Comp& K : V.X.Comps)
	{
		S.LiveDropAvg = K.Kind == CompKind::Kast ? static_cast<int>(std::lround(static_cast<double>(K.CtrF))) : S.LiveDropAvg;
		S.LateBloom = S.LateBloom || K.Kind == CompKind::LateBloom;
		S.Binged += K.Kind == CompKind::Binge ? 1 : 0;
	}
	// "What Rerun did with it" (§5.4.1): the component list, told humbly; a legacy video says it predates the log.
	std::vector<double> Ctrs, Avds;
	for (const Video* O : Recent(C, V.At - 1.0, 10, V.F, false))
	{
		Ctrs.push_back(valgo::RolledCtr(*O));
		Avds.push_back(O->LengthMin * valgo::RetentionOn(*O, Today));
	}
	const double TypicalCtr = MedianOf(Ctrs), TypicalAvd = MedianOf(Avds);
	if (!V.X.On)
	{
		S.Log.push_back({V.At, LegacyNote()});
	}
	else
	{
		S.Log.push_back({V.At, WhenText(V.At) + " \xC2\xB7 " + LogFirstWave()});
		if (World >= V.At + 2.0 * StDay)
		{
			const int Test = TypicalCtr <= 0.0 ? 0 : S.Ctr48 > 1.1 * TypicalCtr ? 1 : S.Ctr48 < TypicalCtr / 1.1 ? -1 : 0;
			S.Log.push_back({V.At + 2.0 * StDay, std::string(WeekdayShort(valgo::DayOf(V.At))) + "-" + WeekdayShort(valgo::DayOf(V.At) + 1) + " \xC2\xB7 " + LogHomeTest(Test)});
		}
		for (const Comp& K : V.X.Comps)
		{
			if (K.T > World)
			{
				continue;
			}
			const char Winner[2] = {static_cast<char>('A' + std::clamp<int>(V.X.DuelWin, 0, 2)), '\0'};
			const std::string Extra = K.Kind == CompKind::Duel ? std::string(Winner) : K.Kind == CompKind::Kast && K.CtrF >= 1.0f ? Grouped(std::lround(static_cast<double>(K.CtrF))) : "";
			S.Log.push_back({K.T, WhenText(K.T) + " \xC2\xB7 " + LogPart(K.Kind, V.X.Verdict, Extra)});
		}
		std::stable_sort(S.Log.begin(), S.Log.end(), [](const std::pair<double, std::string>& A, const std::pair<double, std::string>& B) { return A.first < B.first; });
	}
	// Insights (§5.4.2): up to three lines, the largest deviations against the typical upload, a +-10% dead band.
	std::vector<std::pair<double, std::string>> Cands;
	if (TypicalCtr > 0.0 && World >= V.At + 2.0 * StDay)
	{
		const double Dev = std::log(S.Ctr48 / TypicalCtr);
		if (std::fabs(Dev) > std::log(1.1))
		{
			Cands.push_back({std::fabs(Dev), Dev > 0.0 ? InsightClicksUp(S.Ctr48, TypicalCtr) : InsightClicksDown()});
		}
	}
	if (Over > 0)
	{
		Cands.push_back({0.25 * Over + 0.2, InsightHonesty(Rt.Dislike() / std::max(1e-9, Rt.Like + Rt.Dislike()))});
	}
	if (TypicalAvd > 0.0)
	{
		const double Dev = std::log(S.AvdMin / TypicalAvd);
		if (std::fabs(Dev) > std::log(1.1))
		{
			Cands.push_back({std::fabs(Dev), Dev > 0.0 ? InsightHoldUp(S.AvdMin, TypicalAvd) : InsightHoldDown()});
		}
	}
	if (Load == AdLoad::Heavy && valgo::AdRet(V.F, V.X.Len, Load) < 1.0)
	{
		Cands.push_back({0.12, InsightAds(1.0 - valgo::AdRet(V.F, V.X.Len, Load))});
	}
	if (V.X.On && valgo::TimeFitAt(C, V.At) > 1.0)
	{
		Cands.push_back({0.11, InsightVelocity()});
	}
	if (S.LiveDrop)
	{
		Cands.push_back({0.13, InsightLaunch()});
	}
	if (V.X.Verdict >= 3)
	{
		Cands.push_back({0.10 * V.X.Verdict, InsightPushed()});
	}
	else if (V.X.Verdict == 0)
	{
		Cands.push_back({0.30, InsightStopped()});
	}
	for (const Comp& K : V.X.Comps)
	{
		const double From = std::max(0.0, K.P * valgo::CurveFrac(K.A, valgo::ShapeU3(V.C).Tau1, valgo::ShapeU3(V.C).Tau2, (World - K.T) / StDay));
		if (K.Kind == CompKind::FromCut && From >= 1.0)
		{
			Cands.push_back({0.2, InsightCut(std::llround(From))});
		}
		if (K.Kind == CompKind::FromNextUp && From >= 1.0)
		{
			Cands.push_back({0.2, InsightNextUp(std::llround(From))});
		}
	}
	if (S.LateBloom)
	{
		Cands.push_back({0.5, InsightBloom()});
	}
	if (S.Binged > 0)
	{
		Cands.push_back({0.15, InsightBinge()});
	}
	if (V.E > 0.0 && World < V.At + 365.0 * StDay)
	{
		Cands.push_back({0.105, InsightSearch()});
	}
	if (V.X.On && std::any_of(V.X.Comps.begin(), V.X.Comps.end(), [World](const Comp& K) { return K.Kind == CompKind::Trend && K.T <= World; }))
	{
		Cands.push_back({0.14, InsightTrend()});
	}
	if (V.X.On)
	{
		// Burnout (§4.6.6): Update 3's extra editing in the week up to its upload (itself included), as the upload read it: its
		// own launch actions came after the upload and never entered its retention, so they're left out.
		const uint8_t OwnLaunch = V.X.Launch;
		const double OwnLaunchMin = ((OwnLaunch & (1u << static_cast<int>(Launch::Cut))) != 0 ? 20.0 : 0.0) + ((OwnLaunch & (1u << static_cast<int>(Launch::Teaser))) != 0 ? 10.0 : 0.0) +
			((OwnLaunch & (1u << static_cast<int>(Launch::Reply))) != 0 ? 20.0 : 0.0);
		const double Hours = algo::ExtraHours7(C, V.At) - OwnLaunchMin / 60.0;
		const double Cost = 1.0 - std::clamp(1.0 - valgo::VaBurnSlope * std::max(0.0, Hours - valgo::VaBurnFree), valgo::VaBurnFloor, 1.0);
		if (Cost > 0.0)
		{
			Cands.push_back({0.3 + Cost, InsightBurnout(Hours)});
		}
		// Repetition: the same idea up the week before.
		for (const Video& O : C.Videos)
		{
			if (&O != &V && O.X.On && V.X.Idea >= 0 && O.X.Idea == V.X.Idea && O.At < V.At && O.At > V.At - 7.0 * StDay)
			{
				Cands.push_back({0.25, InsightRepeat(valgo::DayOf(V.At) - valgo::DayOf(O.At))});
				break;
			}
		}
	}
	{
		int Y = 0, Mo = 0, Dm = 0;
		world::CivilDate(Today, Y, Mo, Dm);
		if (Partner && (Mo == 11 || Mo == 12))
		{
			Cands.push_back({0.101, InsightSeasonUp(Mo)});
		}
		if (Partner && Mo == 1)
		{
			Cands.push_back({0.101, InsightSeasonDown()});
		}
	}
	std::stable_sort(Cands.begin(), Cands.end(), [](const std::pair<double, std::string>& A, const std::pair<double, std::string>& B) { return A.first > B.first; });
	for (size_t I = 0; I < Cands.size() && S.Insights.size() < 3; ++I)
	{
		S.Insights.push_back(Cands[I].second);
	}
	return S;
}

// ================================================================== the channel over a window (§5.3)

Range StatsRange(const Channel& C, double World, int Days)
{
	using namespace video_stats_detail;
	Range Out;
	Out.Days = std::max(1, Days);
	const int Today = valgo::DayOf(World);
	auto Push = [&Out](double Vw, double Watch, double Gained, double Lost, double Likes, double Dislikes, double Impr, double Cents, bool Est) {
		Out.Views.push_back(Vw);
		Out.WatchH.push_back(Watch);
		Out.SubsGained.push_back(Gained);
		Out.SubsLost.push_back(Lost);
		Out.Likes.push_back(Likes);
		Out.Dislikes.push_back(Dislikes);
		Out.Impressions.push_back(Impr);
		Out.Cents.push_back(Cents);
		Out.Estimated.push_back(Est);
	};
	for (int D = Today - Out.Days + 1; D <= Today; ++D)
	{
		if (D == Today || static_cast<double>(D + 1) * StDay > C.SteppedTo)
		{
			// The day not stepped yet: the closed form over the kept videos (the tails omitted: a little under, never over).
			const double From = static_cast<double>(D) * StDay, To = std::min(World, static_cast<double>(D + 1) * StDay);
			double Seen = 0, Watch = 0, Subs = 0, Lost = 0, Likes = 0, Dis = 0, Impr = 0, Cents = 0;
			const bool Paying = C.PartnerAt >= 0.0 && From >= C.PartnerAt - 1e-6;
			for (const Video& V : C.Videos)
			{
				const double Gain = std::max(0.0, Views(V, To) - Views(V, From));
				const double R = valgo::RetentionOn(V, D);
				const int Over = valgo::OverOf(V);
				const AdLoad L = V.X.On ? valgo::LoadOn(V.X, D) : AdLoad::Standard;
				const valgo::Rates Rt = valgo::RatesOf(V, L, R, Over);
				Seen += Gain;
				Watch += Gain * V.LengthMin * R / 60.0;
				const double Net = valgo::ConvOn(V, D) * Gain;
				const double Ls = Net * valgo::LostShare(valgo::QOf(V, Rt, R, Over), Over, L == AdLoad::Heavy);
				Subs += Net + Ls;
				Lost += Ls;
				Likes += Rt.Like * Gain;
				Dis += Rt.Dislike() * Gain;
				Impr += valgo::ImprGain(V, From, To);
				Cents += Paying ? valgo::RpmOn(V, D) / 1000.0 * Gain * 100.0 * (C.X.On ? algo::Season(D) : 1.0) : 0.0;
			}
			Push(Seen, Watch, Subs, Lost, Likes, Dis, Impr, Cents, !C.X.On);
			continue;
		}
		const auto It = std::find_if(C.X.Days.begin(), C.X.Days.end(), [D](const DayRow& R) { return R.Key == D; });
		if (It != C.X.Days.end())
		{
			Push(It->Views, It->WatchH, It->Gained, It->Lost, It->Likes, It->Dislikes, It->Impr, static_cast<double>(It->Cents), false);
			continue;
		}
		// Older than the day ring: the month's row spread over its days; before the rings began, nothing ("est.").
		int Y = 0, Mo = 0, Dm = 0;
		world::CivilDate(D, Y, Mo, Dm);
		const int Key = Y * 12 + (Mo - 1);
		const auto Mt = std::find_if(C.X.Months.begin(), C.X.Months.end(), [Key](const DayRow& R) { return R.Key == Key; });
		if (Mt != C.X.Months.end())
		{
			const int Len = Mo == 2 ? ((Y % 4 == 0 && Y % 100 != 0) || Y % 400 == 0 ? 29 : 28) : (Mo == 4 || Mo == 6 || Mo == 9 || Mo == 11) ? 30 : 31;
			const double F = 1.0 / static_cast<double>(Len);
			Push(Mt->Views * F, Mt->WatchH * F, Mt->Gained * F, Mt->Lost * F, Mt->Likes * F, Mt->Dislikes * F, Mt->Impr * F, static_cast<double>(Mt->Cents) * F, true);
		}
		else
		{
			Push(0, 0, 0, 0, 0, 0, 0, 0, true);
		}
	}
	return Out;
}

// ================================================================== Audience (§5.5: display only, W5)

AudienceView AudienceOf(const Channel& C, double World)
{
	using namespace video_stats_detail;
	AudienceView A;
	for (int D = 0; D < 7; ++D)
	{
		for (int H = 0; H < 24; ++H)
		{
			A.Heat[static_cast<size_t>(D)][static_cast<size_t>(H)] = static_cast<float>(valgo::HourHeat(D, H));
		}
	}
	const std::vector<const Video*> Last = Recent(C, World, 10, Format::Count, true);
	double Sigma = 0, Series = 0, Teaching = 0, Cuts = 0;
	for (const Video* V : Last)
	{
		Sigma += V->X.On ? V->X.Sigma : 0.3;
		Series += V->X.On && V->X.Series >= 0 ? 1.0 : 0.0;
		Teaching += V->T == TitleStyle::Teaching ? 1.0 : 0.0;
		Cuts += V->F == Format::Short ? 1.0 : 0.0;
	}
	const double N = std::max(1.0, static_cast<double>(Last.size()));
	A.Returning = std::clamp(0.5 * Sigma / N + 0.1 * Series / N, 0.05, 0.60);
	A.BellShare = 0.10 + 0.05 * std::min(1.0, static_cast<double>(C.Videos.size()) / 100.0);
	A.SubsWatchShare = std::clamp(Sigma / N, 0.0, 1.0);
	// The country split: a fixed table nudged by the content mix (Teaching-heavy channels lean to the English-speaking
	// countries' search; deterministic, display only).
	const char* const Codes[8] = {"US", "GB", "CA", "DE", "AU", "SE", "BR", "NL"};
	const double Base[8] = {0.38, 0.14, 0.10, 0.08, 0.07, 0.05, 0.05, 0.04};
	const uint32_t H = Fnv1a("rerun:geo:" + std::to_string(static_cast<int>(Teaching)) + ":" + std::to_string(static_cast<int>(Cuts)));
	double Sum = 0.0;
	std::array<double, 8> Geo{};
	for (size_t I = 0; I < 8; ++I)
	{
		Geo[I] = Base[I] * (0.9 + 0.2 * static_cast<double>((H >> (I * 4)) & 15u) / 15.0);
		Sum += Geo[I];
	}
	for (size_t I = 0; I < 8; ++I)
	{
		A.Geo.push_back({Codes[I], Geo[I] / Sum * 0.91});
	}
	const double Shift = 0.03 * Teaching / N;
	A.Ages = {0.21 - Shift, 0.42 - Shift, 0.22 + Shift, 0.10 + Shift, 0.05};
	const double Phone = 0.54 + 0.15 * Cuts / N;
	A.Devices = {Phone, 0.31 - (Phone - 0.54) * 0.6, 0.09 - (Phone - 0.54) * 0.25, 0.06 - (Phone - 0.54) * 0.15};
	return A;
}

// ================================================================== Realtime (§5.7)

RealtimeView Realtime(const Channel& C, double World)
{
	using namespace video_stats_detail;
	RealtimeView Rt;
	const double Hour = 60.0;
	std::vector<std::pair<double, std::string>> Top;
	for (const Video& V : C.Videos)
	{
		const double Last48 = std::max(0.0, Views(V, World) - Views(V, World - 48.0 * Hour));
		if (!valgo::IsGrant(V))
		{
			Top.push_back({Last48, V.Title});
		}
		for (int H = 0; H < 48; ++H)
		{
			const double B = World - static_cast<double>(47 - H) * Hour;
			Rt.Hours[static_cast<size_t>(H)] += std::max(0.0, Views(V, B) - Views(V, B - Hour));
		}
		// Shown on Home right now: a Home-tagged part in its fast phase (age under 2 x tau1).
		const valgo::VaShape Sh = valgo::ShapeU3(V.C);
		for (const Part& P : PartsOf(V))
		{
			const std::array<double, 10> Mix = valgo::MixOf(V, P.Mix);
			if (V.X.On && Mix[0] > 0.0 && World >= P.T && World - P.T < 2.0 * Sh.Tau1 * StDay && P.P > 0.0)
			{
				if (std::find(Rt.OnHomeNow.begin(), Rt.OnHomeNow.end(), V.Title) == Rt.OnHomeNow.end())
				{
					Rt.OnHomeNow.push_back(V.Title);
				}
			}
		}
	}
	// The minutes: the last hour's views split with a zero-sum jitter (each hour's total unchanged), keyed by the hour.
	const double LastHour = Rt.Hours[47];
	const int HourKey = static_cast<int>(std::floor(World / Hour));
	double Jitter[60];
	double Mean = 0.0;
	for (int M = 0; M < 60; ++M)
	{
		Jitter[M] = static_cast<double>(Fnv1a("rerun:rt:" + std::to_string(HourKey) + ":" + std::to_string(M)) % 1000u) / 1000.0;
		Mean += Jitter[M] / 60.0;
	}
	for (int M = 0; M < 60; ++M)
	{
		Rt.Minutes[static_cast<size_t>(M)] = std::max(0.0, LastHour / 60.0 * (1.0 + 0.6 * (Jitter[M] - Mean)));
	}
	double Scale = 0.0;
	for (const double X : Rt.Minutes)
	{
		Scale += X;
	}
	for (double& X : Rt.Minutes)
	{
		X = Scale > 0.0 ? X * LastHour / Scale : 0.0;
	}
	Rt.SubsNow = SubsAt(C, World);
	// The breakouts' notes (SPEC_CREATORS §3.2): two days from the breakout (a narrated viral roll's from its verdict midnight).
	std::vector<std::pair<double, std::string>> Notes;
	for (const Video& V : C.Videos)
	{
		if (!V.X.On)
		{
			continue;
		}
		const double Verdict = static_cast<double>(valgo::VerdictDayOf(V)) * StDay;
		if (V.X.Viral && V.X.Verdict >= 0 && World >= Verdict && World < Verdict + 2.0 * StDay)
		{
			Notes.push_back({Verdict, valgo::BellViral(V.Title)});
			continue;
		}
		for (const Comp& K : V.X.Comps)
		{
			if (K.Kind == CompKind::Breakout && World >= K.T && World < K.T + 2.0 * StDay)
			{
				Notes.push_back({K.T, valgo::BellBreakout(V.Title)});
			}
		}
	}
	std::stable_sort(Notes.begin(), Notes.end(), [](const std::pair<double, std::string>& A, const std::pair<double, std::string>& B) { return A.first > B.first; });
	for (const auto& N : Notes)
	{
		Rt.Notes.push_back(N.second);
	}
	std::stable_sort(Top.begin(), Top.end(), [](const std::pair<double, std::string>& A, const std::pair<double, std::string>& B) { return A.first > B.first; });
	for (size_t I = 0; I < Top.size() && I < 3; ++I)
	{
		Rt.Top.push_back({Top[I].second, Top[I].first});
	}
	return Rt;
}

// ================================================================== Channel health (§5.9)

Health HealthOf(const Channel& C, double World)
{
	using namespace video_stats_detail;
	Health H;
	// Momentum: the meter's display average, 0.75 -> 0 bars, 1.0 -> 3, 1.5+ -> 5 (SPEC_CREATORS §3.2).
	const double D = C.X.On ? C.X.MomShown : 1.0;
	const double Bars = D <= 1.0 ? 3.0 * (D - 0.75) / 0.25 : 3.0 + 2.0 * (D - 1.0) / 0.5;
	H.Momentum = std::clamp(static_cast<int>(std::lround(Bars)), 0, 5);
	const std::vector<const Video*> Last = Recent(C, World, 10, Format::Count, true);
	std::vector<double> Ctr, Hold, Sat;
	const int Today = valgo::DayOf(World);
	for (const Video* V : Last)
	{
		Ctr.push_back(valgo::RolledCtr(*V));
		const double R = valgo::RetentionOn(*V, Today);
		Hold.push_back(R / valgo::RetentionRef(V->F));
		const int Over = valgo::OverOf(*V);
		Sat.push_back(valgo::QOf(*V, valgo::RatesOf(*V, V->X.On ? valgo::LoadOn(V->X, Today) : AdLoad::Standard, R, Over), R, Over));
	}
	auto Steps = [](double Ratio) { return std::clamp(3 + static_cast<int>(std::lround(std::log(std::max(1e-6, Ratio)) / std::log(1.1))), 0, 5); };
	H.Packaging = Last.empty() ? 3 : Steps(MedianOf(Ctr) / 0.0475);
	H.Holding = Last.empty() ? 3 : Steps(MedianOf(Hold));
	H.Satisfaction = Last.empty() ? 3 : std::clamp(3 + static_cast<int>(std::lround(MedianOf(Sat) / 0.25)), 0, 5);
	// Consistency: the upload streak (8 weeks in a row reads 5 bars).
	H.Consistency = std::clamp(static_cast<int>(std::lround(5.0 * static_cast<double>(std::min(8, C.X.StreakW)) / 8.0)), 0, 5);
	// Energy: Update 3's extra editing this week against 4 hours (5 bars at 2 hours or less, none at 8 or more).
	const double Extra = algo::ExtraHours7(C, World);
	H.Energy = std::clamp(static_cast<int>(std::lround(5.0 * (8.0 - Extra) / 6.0)), 0, 5);
	return H;
}

std::string HealthWords(int Meter, int Bars)
{
	const bool High = Bars >= 4, Low = Bars <= 1;
	switch (Meter)
	{
	case 0: return High ? "Your channel lately: on a roll." : Low ? "Your channel lately: cooling off." : "Your channel lately: steady.";
	case 1: return High ? "Packaging: people click." : Low ? "Packaging: people scrolled past." : "Packaging: about average.";
	case 2: return High ? "Holding: they stay." : Low ? "Holding: they leave early." : "Holding: about average.";
	case 3: return High ? "Satisfaction: liked." : Low ? "Satisfaction: disliked." : "Satisfaction: mixed.";
	case 4: return High ? "Consistency: regular." : Low ? "Consistency: quiet." : "Consistency: patchy.";
	default: return High ? "Energy: fresh." : Low ? "Energy: burnt out." : "Energy: busy.";
	}
}

// ================================================================== the trends (§4.6.10)

std::vector<Trend> Trends(const life::State& L, const world::World& W, double World)
{
	return valgo::TrendsNow(L, &W, World);
}

std::vector<Trend> valgo::TrendsNow(const life::State& L, const world::World* Wp, double World)
{
	// Research's list (§4.6.10): every trend hot at World, each under the self-describing key an idea's facts name
	// (VideoAlgoPriv.h's TrendOfKey: the upload prices it from the key), the hottest first. Mine: the player's matching
	// uploads this window (TrendCool's count); the world's creators' (Others) are core-videocontent's to fill. Without a
	// world (tests, the card room), the calendar's and the network's only.
	using namespace video_stats_detail;
	std::vector<Trend> Out;
	const int Today = valgo::DayOf(World);
	auto Add = [&Out, &L, World](const std::string& Key, const std::string& Label, bool Limited) {
		valgo::VaTrend V;
		if (!valgo::TrendOfKey(Key, V) || valgo::TrendHeat(V, World) <= 0.0 ||
			std::any_of(Out.begin(), Out.end(), [&Key](const Trend& X) { return X.Key == Key; }))
		{
			return;
		}
		Trend T;
		T.Key = Key;
		T.Label = Label;
		T.Heat = valgo::TrendHeat(V, World);
		T.Strength = V.Strength;
		T.Limited = Limited;
		T.Strike = Limited;
		const int CoolKey = valgo::TrendCoolKey(Key);
		for (const Cool& Cl : L.Video.X.TrendCool)
		{
			T.Mine = Cl.Key == CoolKey ? Cl.Count : T.Mine;
		}
		Out.push_back(T);
	};
	// The weekly majors: Sunday noon to Monday noon.
	{
		const int Sunday = Today % 7 == 6 ? Today : Today % 7 == 0 ? Today - 1 : -1;
		if (Sunday >= 0)
		{
			Add("sunday:" + std::to_string(Sunday), "Sunday majors", false);
		}
	}
	// The series running (the network's calendar).
	if (const net::SeriesInfo* S = net::Shared().CurrentSeries(World))
	{
		if (Today >= S->FirstDay && Today <= S->LastDay)
		{
			Add("series:" + S->Id, S->Name + " week", false);
		}
	}
	// The calendar's own: the Player of the Year race, the Hall of Fame class, the holiday grind, New Year challenges.
	{
		int Y = 0, Mo = 0, Dm = 0;
		world::CivilDate(Today, Y, Mo, Dm);
		Add("poy:" + std::to_string(Y), "Player of the Year race", false);
		Add("hof:" + std::to_string(Y), "Hall of Fame class", false);
		Add("holiday:" + std::to_string(Y), "Holiday grind", false);
		Add("newyear:" + std::to_string(Mo == 12 ? Y : Y - 1), "New Year challenges", false);
	}
	// The world's log, dated before this week's Monday (at least a day old): a champion, a major's final table, a rival site
	// opening, a rival's overlay.
	const double Monday = static_cast<double>(Today - Today % 7) * StDay;
	// (The log grows at its end in time order, give or take a day's results: read back from the end, stop two weeks before.)
	const std::vector<world::WorldEvent> NoEvents;
	const std::vector<world::WorldEvent>& Log = Wp ? Wp->Events() : NoEvents;
	for (auto It = Log.rbegin(); It != Log.rend() && It->At >= Monday - 16.0 * StDay; ++It)
	{
		const world::WorldEvent& E = *It;
		if (E.At >= Monday - StDay || E.At < Monday - 14.0 * StDay || E.What.empty())
		{
			continue;
		}
		const std::string At = std::to_string(static_cast<long long>(E.At));
		const world::Npc* P = Wp->Get(E.Npc);
		const std::string Who = P ? P->Name : std::string();
		if (E.Kind == world::EventKind::Champion && !Who.empty())
		{
			Add("champ:" + At, Who + " wins the " + E.What, false);
		}
		else if (E.Kind == world::EventKind::FinalTable && !Who.empty())
		{
			Add("final:" + At, "The " + E.What + " final table", false);
		}
		else if (E.Kind == world::EventKind::SiteLaunched)
		{
			// (The log keeps the site's key, "runittwice": the trend says its name, as the world's news does.)
			const net::SiteId Launched = net::SiteOf(E.What + ".");
			if (Launched != net::SiteId::RiverLine && net::Def(Launched).Id == Launched)
			{
				Add("site:" + At, "Is " + net::Def(Launched).Name + " soft?", true);
			}
		}
		else if (E.Kind == world::EventKind::SiteOverlay)
		{
			Add("overlay:" + At, E.What + " overlay", true);
		}
	}
	std::stable_sort(Out.begin(), Out.end(), [](const Trend& A, const Trend& B) { return (A.Strength - 1.0) * A.Heat > (B.Strength - 1.0) * B.Heat; });
	return Out;
}

// ================================================================== the Ad market (§4.5.8)

namespace video_stats_detail
{
/** The advertiser categories (§4.5.4's AdvertiserFit), in the card's order. */
struct AdCategory
{
	const char* Name;
	double Fit;
};
const AdCategory* AdCategories(size_t& N)
{
	static const AdCategory List[] = {{"Setup and gear", 1.25}, {"Teaching", 1.20}, {"Results and bankroll", 1.15}, {"Story", 1.00}, {"Life vlogs", 0.95},
		{"Loud highlights", 0.90}, {"News and reactions", 0.90}, {"Rival drama", 0.85}};
	N = sizeof(List) / sizeof(List[0]);
	return List;
}

/** A video's category: its title style, a rival's hand as rival drama (an idea's group is core-videocontent's to tell). */
int CategoryOf(const Video& V)
{
	if (V.K == Kind::Rival)
	{
		return 7;
	}
	return V.T == TitleStyle::Teaching ? 1 : V.T == TitleStyle::Hype ? 5 : 3;
}
} // namespace video_stats_detail

std::vector<AdMarketRow> AdMarket(const Channel& C, double World)
{
	using namespace video_stats_detail;
	size_t N = 0;
	const AdCategory* List = AdCategories(N);
	std::vector<AdMarketRow> Out;
	const double Season = algo::Season(valgo::DayOf(World));
	const std::vector<const Video*> Last = Recent(C, World, 10, Format::Count, true);
	for (size_t I = 0; I < N; ++I)
	{
		AdMarketRow R;
		R.Category = List[I].Name;
		R.Fit = List[I].Fit;
		R.Cpm = valgo::VaBaseCpm * List[I].Fit * Season;
		for (const Video* V : Last)
		{
			R.Mix += CategoryOf(*V) == static_cast<int>(I) ? 1.0 / static_cast<double>(Last.size()) : 0.0;
		}
		Out.push_back(R);
	}
	return Out;
}

// ================================================================== the copy

std::vector<std::string> InventoryAlgo()
{
	using namespace video_stats_detail;
	std::vector<std::string> Out = valgo::AlgoInventory();
	Out.push_back("line\t" + LegacyNote());
	Out.push_back("line\t" + InsightClicksUp(0.064, 0.046));
	Out.push_back("line\t" + InsightClicksDown());
	Out.push_back("line\t" + InsightHonesty(0.11));
	Out.push_back("line\t" + InsightHoldUp(4.0 + 2.0 / 60.0, 3.0 + 41.0 / 60.0));
	Out.push_back("line\t" + InsightHoldDown());
	Out.push_back("line\t" + InsightAds(0.06));
	Out.push_back("line\t" + InsightVelocity());
	Out.push_back("line\t" + InsightLaunch());
	Out.push_back("line\t" + InsightPushed());
	Out.push_back("line\t" + InsightStopped());
	Out.push_back("line\t" + InsightCut(312));
	Out.push_back("line\t" + InsightNextUp(84));
	Out.push_back("line\t" + InsightSearch());
	Out.push_back("line\t" + InsightBloom());
	Out.push_back("line\t" + InsightBinge());
	Out.push_back("line\t" + InsightTrend());
	Out.push_back("line\t" + InsightBurnout(6.2));
	Out.push_back("line\t" + InsightBurnout(4.6));
	Out.push_back("line\t" + InsightRepeat(3));
	Out.push_back("line\t" + InsightRepeat(1));
	Out.push_back("line\t" + InsightRepeat(0));
	Out.push_back("line\t" + InsightSeasonUp(12));
	Out.push_back("line\t" + InsightSeasonUp(11));
	Out.push_back("line\t" + InsightSeasonDown());
	for (int M = 0; M < 6; ++M)
	{
		for (const int B : {0, 3, 5})
		{
			Out.push_back("line\t" + HealthWords(M, B));
		}
	}
	// The log's phrases (they follow "Tue 3 AM · ", so they're fragments: "frag" lines, no closing stop).
	Out.push_back("frag\t" + LogFirstWave());
	for (const int T : {-1, 0, 1})
	{
		Out.push_back("frag\t" + LogHomeTest(T));
	}
	for (int K = 0; K < static_cast<int>(CompKind::Count); ++K)
	{
		for (int Label = 0; Label <= 4; ++Label)
		{
			const std::string P = LogPart(static_cast<CompKind>(K), Label, K == static_cast<int>(CompKind::Duel) ? "B" : K == static_cast<int>(CompKind::Kast) ? "212" : "");
			if (std::find(Out.begin(), Out.end(), "frag\t" + P) == Out.end())
			{
				Out.push_back("frag\t" + P);
			}
		}
	}
	Out.push_back("frag\t" + LogPart(CompKind::Kast, 0, ""));
	size_t N = 0;
	const AdCategory* List = AdCategories(N);
	for (size_t I = 0; I < N; ++I)
	{
		Out.push_back(std::string("label\t") + List[I].Name);
	}
	return Out;
}
} // namespace video
} // namespace ss
