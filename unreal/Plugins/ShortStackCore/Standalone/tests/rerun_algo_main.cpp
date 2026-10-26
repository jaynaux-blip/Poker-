// rerun_algo (update 3 SPEC §4.8, §7.7, suite 19; SPEC_CREATORS §3, §9.3): Rerun 2.0's model and money. The invariants
// I1-I9 (I6 a >= check over the grid of Update 2's choice sets, with the per-kind perfkick normalisers), UploadU2 against
// Update 2's Upload, the CPM identity, the season's yearly mean, Update 2's RPMs to the cent, the analytics' consistency,
// the ad-load switch, burnout from extra editing only, the verdicts and check-ins, the Duel, Next-up and Cut links, the
// creator update's Momentum, perfkick, honest labels, breakouts, the upload streak, Fresh on Rerun, the Binge wave, Late
// Bloomer, Live Drop and the Kast Heat Rerun earns, the save lines and their budgets, and spike S-C's archetypes against
// their Update 2 twins (§3.6's bands). Default run <= 30 s; "sc [seeds]" prints S-C's full table (24 seeds); "--write"
// writes Tests/video_vectors_u3.txt. core-videoalgo's file.
#include "CopyLint.h"
#include "Trademarks.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/VideoAlgoPriv.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Game/VideoStats.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifndef SS_TESTS_DIR
#define SS_TESTS_DIR "."
#endif

// The fold test's seam (Video.cpp defines it; video_main.cpp declares it the same way): the same step with every video kept.
namespace ss
{
namespace video
{
namespace video_test_seam
{
void StepUnfolded(Channel& C, life::State& L, Chips& Bankroll, kast::Channel& K, double From, double To);
} // namespace video_test_seam
} // namespace video
} // namespace ss

namespace rerun_algo_test
{
namespace video = ss::video;
namespace algo = ss::video::algo;
namespace valgo = ss::video::valgo;
namespace life = ss::life;
constexpr double Day = 1440.0;

int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const std::string& What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
	}
}

std::string Num(double X, int Digits = 6)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.*g", Digits, X);
	return Buf;
}

double Quantile(std::vector<double> X, double Q)
{
	if (X.empty())
	{
		return 0.0;
	}
	std::sort(X.begin(), X.end());
	return X[std::min(X.size() - 1, static_cast<size_t>(static_cast<double>(X.size()) * Q))];
}

double Median(const std::vector<double>& X)
{
	return Quantile(X, 0.5);
}

/** A candidate for the creator careers: a cooler at $5.50 (VideoGrind's), with the kind and stakes the archetype asks. */
video::Candidate Moment(const std::string& Id, double World, video::Kind K = video::Kind::Cooler, ss::Chips BuyIn = 550, int Viewers = 0)
{
	video::Candidate C;
	C.Id = Id;
	C.At = World;
	C.K = K;
	C.Event = "Night Owl Turbo";
	C.BuyIn = BuyIn;
	C.Rec.Blinds.Bb = 200;
	C.Rec.Pot = 12800;
	C.Rec.HeroNet = -6400;
	C.Rec.Mine = ss::ParseCards("KhKs");
	C.Stream.Viewers = Viewers;
	C.Expires = World + 21.0 * Day;
	return C;
}

std::string SaveText(const life::State& L)
{
	std::string Out;
	video::WriteLines(L, Out);
	return Out;
}

std::vector<std::vector<std::string>> SplitLines(const std::string& Text)
{
	std::vector<std::vector<std::string>> Out;
	std::istringstream In(Text);
	std::string Line;
	while (std::getline(In, Line))
	{
		std::vector<std::string> P(1);
		for (const char Ch : Line)
		{
			if (Ch == '\t')
			{
				P.emplace_back();
			}
			else
			{
				P.back().push_back(Ch);
			}
		}
		Out.push_back(P);
	}
	return Out;
}

life::State Reload(const std::string& Text)
{
	life::State L;
	for (const std::vector<std::string>& P : SplitLines(Text))
	{
		video::ReadLine(P, L);
	}
	return L;
}

// ================================================================== the archetypes (spike S-C, SPEC_CREATORS §3.5-§3.6)

/** A lever that ramps from V0 at day D0 to V1 at day D1 (the probe's ramp). */
struct Ramp
{
	double D0 = 0, D1 = 0, V0 = 0, V1 = 0;
	double At(int D) const
	{
		const double X = static_cast<double>(D);
		return X <= D0 ? V0 : X >= D1 ? V1 : V0 + (V1 - V0) * (X - D0) / (D1 - D0);
	}
};

struct Arch
{
	std::string Name;
	std::vector<int> Weekdays;
	bool U3 = true;      // false: the Update 2 twin (the channel's X stays off: Update 2's choices, Update 2's rules)
	double Hour = 3.0;   // the moment's hour; the upload goes up half an hour later (VideoGrind's 3:30 AM)
	video::Kind K = video::Kind::Cooler;
	video::Frame Fr = video::Frame::Versus;
	video::TitleStyle T = video::TitleStyle::Story;
	Ramp Gear{0, 0, 0.5, 0.5};
	Ramp BuyIn{0, 0, 550, 550};
	Ramp Kast{0, 0, 0, 0};
	int StreamViewers = 0;
	double LiveAvg = 0, LiveNorm = 0, LiveMinutes = 0; // Live Drop: a stream 10 minutes after each upload
	uint8_t Launch = 0;                                  // launch actions taken in each video's test (bits)
	std::vector<video::Format> Mix{video::Format::Short, video::Format::Breakdown, video::Format::Vlog};
	video::AdLoad Ads = video::AdLoad::Standard;
	bool Switcher = false; // Heavy ads on every long video once its last check-in has passed (day 15), as Session::SetAdLoad does
	std::string Seeds;     // the archetype whose seeds it shares ("" its own): paired comparisons between archetypes
};

struct VideoRec
{
	int Day = 0;
	std::string Id;
	double P = 0, Total = -1, MomC = 1, Shown = 1;
	int Label = -1;
	bool Viral = false;
};

struct Run
{
	int Partner = 0;
	std::map<int, double> Subs;
	std::vector<double> Cents;    // per day
	std::vector<double> Deseason; // per day, divided by the day's season
	std::vector<VideoRec> Vids;
	int Breakouts = 0, Binge = 0, Bloom = 0, Comps = 0, MaxComps = 0;
	double DropShare = 0;        // Live Drop's component and V against P0, summed (streamers)
	int Drops = 0;
};

const std::vector<int> Weekly{5}, ThreeAWeek{0, 2, 4}, Daily{0, 1, 2, 3, 4, 5, 6};

double TotalOf(const video::Video& V)
{
	if (!V.X.On)
	{
		return V.P + V.E * 365.0;
	}
	double T = V.X.P0 + V.E * 365.0;
	for (const video::Comp& K : V.X.Comps)
	{
		T += K.P;
	}
	return T;
}

Run Simulate(const Arch& A, int Seed, int Days)
{
	video::Channel C;
	life::State L;
	L.RentsPaid = 2; // past Act I by construction (W11 would hold every review otherwise)
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	Run R;
	R.Cents.assign(static_cast<size_t>(Days) + 2, 0.0);
	R.Deseason.assign(static_cast<size_t>(Days) + 2, 0.0);
	// (An archetype and its twin share their seeds, so Update 2's four draws are the same uploads' on both: paired ratios.)
	const std::string Base = !A.Seeds.empty() ? A.Seeds : A.Name.size() > 5 && A.Name.compare(A.Name.size() - 5, 5, " twin") == 0 ? A.Name.substr(0, A.Name.size() - 5) : A.Name;
	const std::string SeedText = "sc:" + Base + ":" + std::to_string(Seed);
	if (A.U3)
	{
		C.X.On = true; // a Rerun 2.0 channel from its first upload (a new career's)
	}
	int Uploads = 0;
	ss::Chips Earned = 0;
	std::map<std::string, size_t> Index;
	for (int D = 0; D <= Days; ++D)
	{
		const bool OnDay = std::find(A.Weekdays.begin(), A.Weekdays.end(), D % 7) != A.Weekdays.end();
		if (D >= 7 && OnDay)
		{
			const double At = static_cast<double>(D) * Day + A.Hour * 60.0;
			video::Choice Ch;
			Ch.F = A.Mix[static_cast<size_t>(Uploads) % A.Mix.size()];
			Ch.T = A.T;
			Ch.Fr = A.Fr;
			Ch.Ads = Ch.F == video::Format::Short ? video::AdLoad::Standard : A.Ads;
			for (int I = 0; I < (Ch.F == video::Format::Vlog ? 3 : 1); ++I)
			{
				const std::string Id = "rl:sc@" + std::to_string(D) + ":" + std::to_string(I);
				video::NoteHand(C, Moment(Id, At, A.K, static_cast<ss::Chips>(std::llround(A.BuyIn.At(D))), A.StreamViewers));
				Ch.Candidates.push_back(Id);
			}
			video::UploadFacts F;
			F.GearQuality = A.Gear.At(D);
			F.KastFollowers = static_cast<int>(A.Kast.At(D));
			F.RentsPaid = 2;
			const std::string Id = video::Upload(C, Ch, At + 30.0, SeedText, F);
			++Uploads;
			if (!Id.empty())
			{
				video::Video& V = C.Videos.back();
				V.X.Launch = static_cast<uint8_t>(V.X.Launch | (A.U3 ? A.Launch : 0));
				VideoRec Rec;
				Rec.Day = D;
				Rec.Id = Id;
				Rec.P = V.P;
				Rec.MomC = C.X.On ? C.X.Momentum : 1.0;
				Rec.Shown = C.X.On ? C.X.MomShown : 1.0;
				Rec.Viral = V.X.Viral;
				Index[Id] = R.Vids.size();
				R.Vids.push_back(Rec);
				if (A.U3 && A.LiveMinutes > 0.0)
				{
					const size_t Before = V.X.Comps.size();
					algo::NoteStream(C, At + 40.0, At + 40.0 + A.LiveMinutes, static_cast<int>(A.LiveAvg * 1.5), A.LiveAvg, A.LiveNorm);
					const video::Video& W = C.Videos.back();
					R.DropShare += W.X.Comps.size() > Before ? W.X.Comps.back().P / std::max(1.0, W.X.P0) : 0.0;
					++R.Drops;
				}
			}
		}
		if (A.U3 && A.Switcher)
		{
			// Session::SetAdLoad's protocol: Ads now, effective from the next midnight (AdsChangedDay), on a kept long video.
			for (video::Video& V : C.Videos)
			{
				if (V.X.On && V.F != video::Format::Short && V.X.Ads == video::AdLoad::Standard && V.X.AdsChangedDay < 0 && static_cast<double>(D) * Day >= V.At + 15.0 * Day)
				{
					V.X.Ads = video::AdLoad::Heavy;
					V.X.AdsChangedDay = D + 1;
				}
			}
		}
		video::Step(C, L, Bank, K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		const ss::Chips Now = C.BalanceCents + C.PaidCents;
		R.Cents[static_cast<size_t>(D)] = static_cast<double>(Now - Earned);
		R.Deseason[static_cast<size_t>(D)] = static_cast<double>(Now - Earned) / (A.U3 ? algo::Season(D) : 1.0);
		Earned = Now;
		R.Partner = R.Partner == 0 && C.PartnerAt >= 0.0 ? static_cast<int>(C.PartnerAt / Day) : R.Partner;
		if (D + 1 == 30 || D + 1 == 90 || D + 1 == 180 || D + 1 == 365 || D + 1 == 540 || D + 1 == 730)
		{
			R.Subs[D + 1] = C.Subs;
		}
		// Each video's eventual total and its label, read 20 days after it went up (every News and Cut decision is made).
		for (const video::Video& V : C.Videos)
		{
			const auto It = Index.find(V.Id);
			if (It == Index.end() || R.Vids[It->second].Total >= 0.0 || static_cast<double>(D + 1) * Day < V.At + 20.0 * Day)
			{
				continue;
			}
			VideoRec& Rec = R.Vids[It->second];
			Rec.Total = TotalOf(V);
			Rec.Label = V.X.Verdict;
			for (const video::Comp& Cp : V.X.Comps)
			{
				R.Breakouts += Cp.Kind == video::CompKind::Breakout ? 1 : 0;
				R.Binge += Cp.Kind == video::CompKind::Binge ? 1 : 0;
				R.Bloom += Cp.Kind == video::CompKind::LateBloom ? 1 : 0;
			}
			R.Comps += static_cast<int>(V.X.Comps.size());
			R.MaxComps = std::max(R.MaxComps, static_cast<int>(V.X.Comps.size()));
		}
	}
	return R;
}

double MonthOf(const std::vector<double>& Cents, int To)
{
	double Sum = 0.0;
	for (int D = std::max(0, To - 30); D < To && D < static_cast<int>(Cents.size()); ++D)
	{
		Sum += Cents[static_cast<size_t>(D)];
	}
	return Sum / 100.0;
}

/** §5.3 R1's meter: the display average 0.75 -> 0 bars, 1.0 -> 3, 1.5+ -> 5. */
int Bars(double Mc)
{
	const double X = Mc <= 1.0 ? 3.0 * (Mc - 0.75) / 0.25 : 3.0 + 2.0 * (Mc - 1.0) / 0.5;
	return std::clamp(static_cast<int>(std::lround(X)), 0, 5);
}

/** The archetype's medians over its seeds, as S-C's table and §3.6's bands read them. */
struct Summary
{
	std::string Name;
	int N = 0;
	double Partner = 0, PartnerP10 = 0;
	std::map<int, double> Subs;
	double M180 = 0, M365 = 0, M365P90 = 0, M540 = 0, M730 = 0; // deseasonalized
	double Raw365 = 0;
	double After3 = 0, Lag1 = 0, MomP10 = 1, MomP90 = 1;
	std::array<double, 5> Labels{};
	double Moves2 = 0, ZeroBars = 0;
	double Breakout = 0, BingePerUpload = 0, MaxComps = 0, DropShare = 0;
};

Summary Summarize(const Arch& A, int Seeds, int Days)
{
	Summary S;
	S.Name = A.Name;
	S.N = Seeds;
	std::vector<double> Gate, M180, M365, M540, M730, Raw365, After, Xs, Ys, Mom;
	std::map<int, std::vector<double>> Subs;
	std::array<double, 5> Labels{};
	double Labelled = 0, Moves = 0, Zero = 0, Steps = 0, Shown = 0, Uploads = 0, Breaks = 0, Binge = 0, Drop = 0, Drops = 0;
	for (int Seed = 0; Seed < Seeds; ++Seed)
	{
		const Run R = Simulate(A, Seed, Days);
		Gate.push_back(R.Partner > 0 ? R.Partner : 9999);
		for (const auto& Kv : R.Subs)
		{
			Subs[Kv.first].push_back(Kv.second);
		}
		M180.push_back(MonthOf(R.Deseason, 180));
		M365.push_back(MonthOf(R.Deseason, 365));
		M540.push_back(MonthOf(R.Deseason, 540));
		M730.push_back(MonthOf(R.Deseason, 730));
		Raw365.push_back(MonthOf(R.Cents, 365));
		std::vector<double> V;
		int Prev = -1;
		for (const VideoRec& Rec : R.Vids)
		{
			if (Rec.Total >= 0.0)
			{
				V.push_back(Rec.Total);
			}
			if (Rec.Label >= 0 && Rec.Label < 5)
			{
				Labels[static_cast<size_t>(Rec.Label)] += 1.0;
				Labelled += 1.0;
			}
			Mom.push_back(Rec.MomC);
			const int B = Bars(Rec.Shown);
			Zero += B == 0 ? 1.0 : 0.0;
			Shown += 1.0;
			if (Prev >= 0)
			{
				Moves += std::abs(B - Prev) >= 2 ? 1.0 : 0.0;
				Steps += 1.0;
			}
			Prev = B;
		}
		for (size_t I = 10; I + 3 < V.size(); ++I)
		{
			std::vector<double> Last(V.begin() + static_cast<std::ptrdiff_t>(I - 10), V.begin() + static_cast<std::ptrdiff_t>(I));
			const double Med = Median(Last);
			Xs.push_back(std::log(V[I] / Med));
			Ys.push_back(std::log(V[I + 1] / Med));
			if (V[I] / Med >= 3.0)
			{
				After.push_back(Median({V[I + 1], V[I + 2], V[I + 3]}) / Med);
			}
		}
		Uploads += static_cast<double>(R.Vids.size());
		Breaks += R.Breakouts;
		Binge += R.Binge;
		Drop += R.DropShare;
		Drops += R.Drops;
		S.MaxComps = std::max(S.MaxComps, static_cast<double>(R.MaxComps));
	}
	S.Partner = Median(Gate);
	S.PartnerP10 = Quantile(Gate, 0.1);
	for (const auto& Kv : Subs)
	{
		S.Subs[Kv.first] = Median(Kv.second);
	}
	S.M180 = Median(M180);
	S.M365 = Median(M365);
	S.M365P90 = Quantile(M365, 0.9);
	S.M540 = Median(M540);
	S.M730 = Median(M730);
	S.Raw365 = Median(Raw365);
	S.After3 = Median(After);
	if (Xs.size() > 2)
	{
		double Mx = 0, My = 0;
		for (size_t I = 0; I < Xs.size(); ++I)
		{
			Mx += Xs[I];
			My += Ys[I];
		}
		Mx /= static_cast<double>(Xs.size());
		My /= static_cast<double>(Ys.size());
		double Cov = 0, Vx = 0, Vy = 0;
		for (size_t I = 0; I < Xs.size(); ++I)
		{
			Cov += (Xs[I] - Mx) * (Ys[I] - My);
			Vx += (Xs[I] - Mx) * (Xs[I] - Mx);
			Vy += (Ys[I] - My) * (Ys[I] - My);
		}
		S.Lag1 = Cov / std::sqrt(std::max(1e-12, Vx * Vy));
	}
	S.MomP10 = Quantile(Mom, 0.1);
	S.MomP90 = Quantile(Mom, 0.9);
	for (size_t I = 0; I < 5; ++I)
	{
		S.Labels[I] = Labelled > 0.0 ? 100.0 * Labels[I] / Labelled : 0.0;
	}
	S.Moves2 = Steps > 0.0 ? 100.0 * Moves / Steps : 0.0;
	S.ZeroBars = Shown > 0.0 ? 100.0 * Zero / Shown : 0.0;
	S.Breakout = Uploads > 0.0 ? 100.0 * Breaks / Uploads : 0.0;
	S.BingePerUpload = Uploads > 0.0 ? Binge / Uploads : 0.0;
	S.DropShare = Drops > 0.0 ? Drop / Drops : 0.0;
	return S;
}

std::string Line(const Summary& S)
{
	char Buf[1024];
	std::snprintf(Buf, sizeof(Buf),
		"%-26s n %2d | Partner day %5.0f (p10 %4.0f) | subs d30 %6.0f d90 %6.0f d180 %7.0f d365 %7.0f d730 %8.0f | $/mo (deseas.) d180 %5.0f d365 %5.0f (p90 %5.0f) "
		"d540 %5.0f d730 %5.0f | raw d365 %5.0f || next 3 after a 3x hit x%.2f; lag-1 %+.2f; MomC p10 %.2f p90 %.2f; labels %.0f/%.0f/%.0f/%.0f/%.0f%%; meter 2+ bar "
		"moves %.1f%%, at 0 bars %.1f%%; breakouts %.2f%%; binge comps/upload %.2f; max comps %.0f; Live Drop %.3f x P0",
		S.Name.c_str(), S.N, S.Partner, S.PartnerP10, S.Subs.count(30) ? S.Subs.at(30) : 0.0, S.Subs.count(90) ? S.Subs.at(90) : 0.0, S.Subs.count(180) ? S.Subs.at(180) : 0.0,
		S.Subs.count(365) ? S.Subs.at(365) : 0.0, S.Subs.count(730) ? S.Subs.at(730) : 0.0, S.M180, S.M365, S.M365P90, S.M540, S.M730, S.Raw365, S.After3, S.Lag1, S.MomP10,
		S.MomP90, S.Labels[0], S.Labels[1], S.Labels[2], S.Labels[3], S.Labels[4], S.Moves2, S.ZeroBars, S.Breakout, S.BingePerUpload, S.MaxComps, S.DropShare);
	return Buf;
}

/** An archetype against its Update 2 twin on the same seeds: the medians of the paired ratios (S-C's low-noise read). */
struct Paired
{
	double Subs90 = 0, Subs365 = 0, Subs730 = 0, Money365 = 0, First10 = 0, Gate = 0;
};

Arch TwinOf(Arch A)
{
	A.U3 = false;
	A.Name += " twin";
	return A;
}

/** A against B on the same seeds (B: A's twin, or another archetype sharing A's seeds): the medians of the per-seed ratios. */
Paired PairedBetween(const Arch& A, const Arch& B, int Seeds, int Days)
{
	std::vector<double> S90, S365, S730, M365, F10, Gate;
	for (int Seed = 0; Seed < Seeds; ++Seed)
	{
		const Run U = Simulate(A, Seed, Days);
		const Run T = Simulate(B, Seed, Days);
		auto At = [](const Run& R, int D) { return R.Subs.count(D) ? R.Subs.at(D) : 0.0; };
		S90.push_back(At(U, 90) / std::max(1.0, At(T, 90)));
		S365.push_back(At(U, 365) / std::max(1.0, At(T, 365)));
		S730.push_back(At(U, 730) / std::max(1.0, At(T, 730)));
		const double Mt = MonthOf(T.Deseason, 365);
		if (Mt > 0.0)
		{
			M365.push_back(MonthOf(U.Deseason, 365) / Mt);
		}
		double Su = 0, St = 0;
		for (size_t I = 0; I < 10 && I < U.Vids.size() && I < T.Vids.size(); ++I)
		{
			Su += U.Vids[I].Total;
			St += T.Vids[I].Total;
		}
		F10.push_back(St > 0.0 ? Su / St : 0.0);
		Gate.push_back(static_cast<double>(U.Partner > 0 ? U.Partner : 9999) / static_cast<double>(T.Partner > 0 ? T.Partner : 9999));
	}
	Paired P;
	P.Subs90 = Median(S90);
	P.Subs365 = Median(S365);
	P.Subs730 = Median(S730);
	P.Money365 = Median(M365);
	P.First10 = Median(F10);
	P.Gate = Median(Gate);
	return P;
}

Paired PairedRatios(const Arch& A, int Seeds, int Days)
{
	return PairedBetween(A, TwinOf(A), Seeds, Days);
}

Arch Neutral(const std::string& Name, const std::vector<int>& Days, bool U3)
{
	Arch A;
	A.Name = Name + (U3 ? "" : " twin");
	A.Weekdays = Days;
	A.U3 = U3;
	return A;
}

/** Spike S-C: every archetype C++ can express today and its Update 2 twin, against the model's twins (SPEC_CREATORS §3.5). */
std::vector<Arch> ScArchetypes()
{
	std::vector<Arch> Out;
	for (const bool U3 : {false, true})
	{
		Out.push_back(Neutral("casual 1/wk neutral", Weekly, U3));
		Out.push_back(Neutral("3/wk neutral", ThreeAWeek, U3));
		Out.push_back(Neutral("daily neutral", Daily, U3));
		Arch S = Neutral("3/wk streamer 20k", ThreeAWeek, U3);
		S.Kast = {0, 0, 20000, 20000};
		S.StreamViewers = 100;
		S.LiveAvg = 180;
		S.LiveNorm = 180;
		S.LiveMinutes = 180;
		Out.push_back(S);
		// Update 2's levers worked well (the skilled creator's Update 2 half: a fitting frame, the rig, the stakes, the pick, Kast).
		Arch K = Neutral("3/wk skilled (U2 levers)", ThreeAWeek, U3);
		K.Fr = video::Frame::Reaction;
		K.K = video::Kind::BadBeat;
		K.Gear = {0, 240, 0.3, 0.75};
		K.BuyIn = {60, 240, 550, 1100};
		K.Kast = {0, 365, 0, 600};
		Out.push_back(K);
		// The neutral creator who posts before the peak and works the launch actions (Update 3's levers it can reach today).
		Arch T = Neutral("3/wk timed + launch", ThreeAWeek, U3);
		T.Hour = 19.0;
		T.Launch = static_cast<uint8_t>((1u << static_cast<int>(video::Launch::Teaser)) | (1u << static_cast<int>(video::Launch::Reply)));
		Out.push_back(T);
		Arch M = Neutral("3/wk ad-maxer", ThreeAWeek, U3);
		M.Ads = video::AdLoad::Heavy;
		M.Seeds = "3/wk neutral";
		Out.push_back(M);
		Arch W = Neutral("3/wk switcher", ThreeAWeek, U3);
		W.Switcher = true;
		W.Seeds = "3/wk neutral";
		Out.push_back(W);
	}
	return Out;
}

// ================================================================== the pure parts

/** A Rerun 2.0 channel ready to upload on: X on, past Fresh on Rerun's twelve, no streak, H at 0 (MomC 1 / 1.049). */
video::Channel PlainU3Channel(double World)
{
	video::Channel C;
	C.SteppedTo = std::floor(World / Day) * Day;
	C.HoursDay0 = std::max(0, static_cast<int>(World / Day) - 365);
	C.X.On = true;
	C.X.UploadsEver = 12;
	C.X.Since = static_cast<int>(World / Day);
	C.X.StreakWeek = static_cast<int>(World / Day) / 7;
	C.X.Momentum = valgo::MomCOf(0.0);
	return C;
}

void Pure()
{
	Expect(std::fabs(algo::Kappa(0.0) - 0.35) < 1e-12 && algo::Kappa(-9.0) == 0.07 && algo::Kappa(9.0) == 0.875, "pure: kappa(0) 0.35, clamped to [0.07, 0.875]");
	Expect(algo::BreakoutChance(9.0) == 0.03 && std::fabs(algo::BreakoutChance(0.0) - 0.004) < 1e-15, "pure: the breakout's chance, 0.004 at S 0, at most 3%");
	bool Regular = true;
	for (double S = -2.0; S <= 2.0; S += 0.05)
	{
		Regular = Regular && algo::KappaRegular(S) >= 0.0 && std::fabs(algo::KappaRegular(S) + 5.5 * algo::BreakoutChance(S) - algo::Kappa(S)) < 1e-12;
	}
	Expect(Regular, "pure: the regular kicks and the breakout's mean add up to kappa (E[sum of kicks] = kappa x P0)");
	bool Bounded = std::fabs(algo::Compress(1.0, 0.3) - 1.0) < 1e-15;
	double Prev = 0.0;
	for (double M = 0.05; M < 50.0; M *= 1.1)
	{
		const double X = algo::Compress(M, 0.30);
		Bounded = Bounded && X > std::exp(-0.30) && X < std::exp(0.30) && X >= Prev;
		Prev = X;
	}
	Expect(Bounded, "pure: Compress(M, 0.30) is monotone, 1 at 1, inside (0.74, 1.35) (I9)");
	// The season: each civil year's mean exactly 1 (2027-2045, the leap years included).
	double Worst = 0.0;
	int Years = 0;
	for (int Y = 2027; Y <= 2045; ++Y)
	{
		double Sum = 0.0;
		int N = 0;
		for (int D = 0; D < 20 * 366; ++D)
		{
			int Yy = 0, Mo = 0, Dm = 0;
			ss::world::CivilDate(D, Yy, Mo, Dm);
			if (Yy == Y)
			{
				Sum += algo::Season(D);
				++N;
			}
		}
		Worst = std::max(Worst, std::fabs(Sum / static_cast<double>(N) - 1.0));
		++Years;
	}
	Expect(Years == 19 && Worst < 1e-9, "pure: each civil year's season averages exactly 1 (worst " + Num(Worst) + ")");
	int Y = 0, Mo = 0, Dm = 0;
	ss::world::CivilDate(0, Y, Mo, Dm);
	Expect(Y == 2026 && Mo == 10 && algo::Season(0) > 1.0 && algo::Season(60) > algo::Season(0) && algo::Season(100) < 0.75, "pure: October pays well, late November better, January least");
	// Ads per playback (§4.5.5): Update 2's cells at Standard, monotone in length and load, no mid-roll under 8 minutes.
	using LO = video::LengthOpt;
	using AL = video::AdLoad;
	Expect(algo::AdsPerPlayback(video::Format::Breakdown, LO::Standard, AL::Standard) == 1.20 && algo::AdsPerPlayback(video::Format::Vlog, LO::Standard, AL::Standard) == 1.68 &&
			   algo::AdsPerPlayback(video::Format::Recap, LO::Standard, AL::Standard) == 1.68 && algo::AdsPerPlayback(video::Format::Breakdown, LO::Tight, AL::Heavy) == 1.0,
		"pure: ads per playback keep Update 2's 1.20 and 1.68, and a 6-minute Breakdown has no mid-roll");
	bool Monotone = true;
	for (const video::Format F : {video::Format::Breakdown, video::Format::Recap, video::Format::Vlog})
	{
		for (int L = 0; L < 3; ++L)
		{
			for (int A = 0; A < 3; ++A)
			{
				const double X = algo::AdsPerPlayback(F, static_cast<LO>(L), static_cast<AL>(A));
				Monotone = Monotone && (L == 0 || X >= algo::AdsPerPlayback(F, static_cast<LO>(L - 1), static_cast<AL>(A))) &&
					(A == 0 || X >= algo::AdsPerPlayback(F, static_cast<LO>(L), static_cast<AL>(A - 1)));
			}
		}
	}
	Expect(Monotone, "pure: ads per playback rise with length and load");
	// The labels' bands (SPEC_CREATORS §3.2).
	Expect(algo::VerdictLabel(-0.81, 0, 0) == 0 && algo::VerdictLabel(-0.8, 0, 0) == 1 && algo::VerdictLabel(-0.1, 0, 0) == 2 && algo::VerdictLabel(0.6, 0, 0) == 3 &&
			   algo::VerdictLabel(1.2, 0, 0) == 4 && algo::VerdictLabel(0.0, 0.4, 0.0) == 2 && algo::VerdictLabel(0.0, 1.2, 0.0) == 3,
		"pure: the verdict reads rp = perf + 0.5 (S - S0_base) against -0.8, -0.1, 0.6 and 1.2");
	// A video's rates come from Fnv1a("<Fate>:rates:<i>") Box-Muller pairs, clamped to +-3 (§5.1; nothing saved, nothing drawn).
	{
		bool Same = true;
		for (const uint32_t Fate : {0u, 7u, 123456789u, 4000000000u})
		{
			for (int I = 0; I < 12; ++I)
			{
				const std::string Key = std::to_string(Fate) + ":rates:";
				const double U1 = (static_cast<double>(ss::Fnv1a(Key + std::to_string(I / 2 * 2))) + 0.5) / 4294967296.0;
				const double U2 = (static_cast<double>(ss::Fnv1a(Key + std::to_string(I / 2 * 2 + 1))) + 0.5) / 4294967296.0;
				const double R = std::sqrt(-2.0 * std::log(U1)), A = 2.0 * 3.14159265358979323846 * U2;
				Same = Same && valgo::FateNormal(Fate, I) == std::clamp(I % 2 == 0 ? R * std::cos(A) : R * std::sin(A), -3.0, 3.0);
			}
		}
		Expect(Same, "pure: the rates' normals are Fnv1a(\"<Fate>:rates:<i>\") Box-Muller pairs, clamped to 3");
	}
	// The perfkick's normalisers (SPEC_CREATORS §3.2): mean 1 in views for each kind, over Update 2's own luck.
	for (const bool Travel : {false, true})
	{
		ss::Rng R(Travel ? "perfkick:travel" : "perfkick:stay");
		double Lv = 0.0, LvPk = 0.0;
		for (int I = 0; I < 400000; ++I)
		{
			const double U1 = std::max(1e-12, R.Next()), U2 = R.Next();
			const double G = std::sqrt(-2.0 * std::log(U1)) * std::cos(2.0 * 3.14159265358979323846 * U2);
			const double Roll = R.Next(), Size = R.Range(20.0, 50.0);
			const double V = Roll < (Travel ? 0.01 : 0.005) ? Size : 1.0;
			const double LuckV = std::exp(0.9 * G) * V;
			const double Pk = std::clamp(std::exp(0.5 * std::clamp(std::log(LuckV), -1.2, 2.0)), 0.4, 3.0) / (Travel ? valgo::VaKickNormTravel : valgo::VaKickNormStay);
			Lv += LuckV;
			LvPk += LuckV * Pk;
		}
		Expect(std::fabs(LvPk / Lv - 1.0) < 0.01, std::string("pure: the perfkick averages 1 in views for ") + (Travel ? "bad beats, wins and bluffs" : "the kinds that don't travel") +
			" (x" + Num(LvPk / Lv, 4) + ")");
	}
	// MomC's normaliser: a neutral channel's H wanders around 0 and its reach multiplier averages about 1.
	{
		ss::Rng R("momc:neutral");
		double H = 0.0, Sum = 0.0;
		const int N = 200000;
		for (int I = 0; I < N; ++I)
		{
			const double U1 = std::max(1e-12, R.Next()), U2 = R.Next();
			const double G = std::sqrt(-2.0 * std::log(U1)) * std::cos(2.0 * 3.14159265358979323846 * U2);
			const bool Viral = R.Next() < 0.0075;
			const double Perf = std::clamp(0.9 * G + (Viral ? std::log(R.Range(20.0, 50.0)) : 0.0), -1.2, 2.0);
			Sum += valgo::MomCOf(H);
			H = 0.6 * H + 0.4 * Perf + (Viral ? 0.5 : 0.0);
		}
		Expect(std::fabs(Sum / N - 1.0) < 0.03, "pure: MomC averages about 1 for a neutral channel (" + Num(Sum / N, 4) + ")");
	}
}

/** One upload of a choice on a fresh copy of a channel: the video, or an empty one. */
video::Video UploadOn(video::Channel& C, const video::Choice& Ch, double World, const std::string& Seed, const video::UploadFacts& F)
{
	const std::string Id = video::Upload(C, Ch, World, Seed, F);
	return Id.empty() ? video::Video() : C.Videos.back();
}

void Identity()
{
	// I6, exactly: every Update 2 choice set on a Rerun 2.0 channel is Update 2's upload (P_U2) times MomC, the streak and the
	// CTR noise, with M3 = 1, S0 = S0_base, and Update 2's retention, conversion and RPM to the digit.
	int Sets = 0, Exact = 0, Same = 0, Neutral = 0, Rpms = 0;
	const double World = 40.0 * Day + 4.0 * 60.0;
	const video::Kind Kinds[5] = {video::Kind::BadBeat, video::Kind::Cooler, video::Kind::Bluff, video::Kind::HeroFold, video::Kind::BigPot};
	for (int Fr = 0; Fr < 5; ++Fr)
	{
		for (int T = 0; T < 5; ++T)
		{
			for (int Ki = 0; Ki < 5; ++Ki)
			{
				for (const double Gear : {0.0, 0.5, 1.0})
				{
					for (const int Kast : {0, 2000, 20000})
					{
						for (const video::Format Fm : {video::Format::Short, video::Format::Breakdown})
						{
							if ((Fr + T + Ki + Kast / 1000 + static_cast<int>(Gear * 2.0)) % 3 != 0)
							{
								continue; // a third of the grid: every frame, style, kind, rig, audience and format still met
							}
							video::Channel C = PlainU3Channel(World);
							video::Channel Legacy;
							const video::Candidate M = Moment("rl:id@40:" + std::to_string(Sets), World - 60.0, Kinds[Ki], 1100, 30);
							video::NoteHand(C, M);
							video::NoteHand(Legacy, M);
							video::Choice Ch;
							Ch.Candidates = {M.Id};
							Ch.F = Fm;
							Ch.T = static_cast<video::TitleStyle>(T);
							Ch.Fr = static_cast<video::Frame>(Fr);
							video::UploadFacts F;
							F.GearQuality = Gear;
							F.KastFollowers = Kast;
							const std::string Seed = "identity:" + std::to_string(Sets);
							const double PU2 = video::UploadU2(C, Ch, World, Seed, F);
							const video::Video L2 = UploadOn(Legacy, Ch, World, Seed, F);
							const video::Video U = UploadOn(C, Ch, World, Seed, F);
							++Sets;
							const double Want = std::round(PU2 * valgo::MomCOf(0.0) * 1.0 * U.X.CtrNoise * 100.0) / 100.0;
							Exact += U.X.On && std::fabs(U.P - Want) < 1e-9 && PU2 == L2.P ? 1 : 0;
							const double E = U.C == video::Class::Strategy ? std::round(U.P * 0.0015 * 1e4) / 1e4 : 0.0;
							const bool SameNow = U.Retention == L2.Retention && U.Conv == L2.Conv && U.E == E && U.C == L2.C && U.LengthMin == L2.LengthMin;
							Same += SameNow ? 1 : 0;
							// Ctr_U2, Update 2's own click-through for the choices (an online hand: no portrait; no haircut).
							const double CtrU2 = std::clamp(0.05 * valgo::FrameFit(Ch.Fr, valgo::KindGroup(Kinds[Ki])) * valgo::StyleCtr(Ch.T) * (0.7 + 0.5 * Gear), 0.02, 0.14);
							const bool NeutralNow = U.X.S0 == U.X.S0Base && U.X.Ctr == valgo::VaRound(CtrU2, 1e7);
							Neutral += NeutralNow ? 1 : 0;
							if ((!SameNow || !NeutralNow) && Sets - std::min(Same, Neutral) < 4)
							{
								std::printf("  identity: frame %d style %d kind %d gear %.1f kast %d format %d: same %d neutral %d: r %.6g / %.6g, c %.8g / %.8g, E %.6g / %.6g, class %d / %d, "
											"length %.4g / %.4g, ctr %.9g / %.9g\n",
									Fr, T, Ki, Gear, Kast, static_cast<int>(Fm), SameNow ? 1 : 0, NeutralNow ? 1 : 0, U.Retention, L2.Retention, U.Conv, L2.Conv, U.E, L2.E,
									static_cast<int>(U.C), static_cast<int>(L2.C), U.LengthMin, L2.LengthMin, U.X.Ctr, CtrU2);
							}
							Rpms += U.Rpm == L2.Rpm ? 1 : 0;
						}
					}
				}
			}
		}
	}
	Expect(Sets > 300 && Exact == Sets, "identity: every Update 2 choice set's P is P_U2 x MomC x Streak x CtrNoise, to the cent (" + std::to_string(Exact) + " of " + std::to_string(Sets) + ")");
	Expect(Same == Sets, "identity: and Update 2's retention, conversion, evergreen term, class and length (" + std::to_string(Same) + " of " + std::to_string(Sets) + ")");
	Expect(Neutral == Sets, "identity: and S0 = S0_base, Ctr = Ctr_U2: M3 is exactly 1 (" + std::to_string(Neutral) + " of " + std::to_string(Sets) + ")");
	Expect(Rpms == Sets, "identity: and Update 2's RPM to the digit (" + std::to_string(Rpms) + " of " + std::to_string(Sets) + ")");
	// The timing window: a 9 PM drop and a 4 AM drop of the same choices differ by exactly Compress(TimeFit x (1 + kappa(S0 +
	// 0.08)) / (1 + kappa(S0)), 0.30); the 4 AM one is Update 2's (bonus-only).
	{
		const double Night = 41.0 * Day + 20.0 * 60.0, Early = 41.0 * Day + 4.0 * 60.0;
		video::Channel A = PlainU3Channel(Night), B = PlainU3Channel(Early);
		const video::Candidate M = Moment("rl:time@41:1", Early - 60.0);
		video::NoteHand(A, M);
		video::NoteHand(B, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		video::UploadFacts F;
		F.GearQuality = 0.5;
		const video::Video Va = UploadOn(A, Ch, Night, "timing", F);
		const video::Video Vb = UploadOn(B, Ch, Early, "timing", F);
		const double Fit = valgo::TimeFitAt(A, Va.At);
		const double M3 = Fit * (1.0 + algo::Kappa(Va.X.S0)) / (1.0 + algo::Kappa(Va.X.S0Base));
		const double PU2a = Va.P / (algo::Compress(M3, 0.30) * valgo::MomCOf(0.0) * Va.X.CtrNoise);
		Expect(Fit == 1.08 && valgo::TimeFitAt(B, Vb.At) == 1.0 && std::fabs(Va.X.S0 - Va.X.S0Base - 0.08) < 1e-12 && Vb.X.S0 == Vb.X.S0Base && PU2a > 0.0,
			"identity: a drop before the 9 PM peak fits x1.08 and scores +0.08; 4 AM is Update 2's exactly");
	}
	// I6 in expectation: P0 + every kick + the breakout pays back P (the perfkick's normaliser), over 3,000 uploads.
	{
		double P = 0.0, Got = 0.0;
		for (int I = 0; I < 3000; ++I)
		{
			const double At = 50.0 * Day + 4.0 * 60.0;
			video::Channel C = PlainU3Channel(At);
			const video::Candidate M = Moment("rl:exp@50:1", At - 60.0, I % 2 == 0 ? video::Kind::BadBeat : video::Kind::Cooler);
			video::NoteHand(C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			video::UploadFacts F;
			F.GearQuality = 0.5;
			UploadOn(C, Ch, At, "expect:" + std::to_string(I), F);
			life::State L;
			L.RentsPaid = 2;
			ss::kast::Channel K;
			ss::Chips Bank = 0;
			video::Step(C, L, Bank, K, At, At + 20.0 * Day);
			const video::Video& V = C.Videos.back();
			P += V.P;
			Got += TotalOf(V);
		}
		Expect(std::fabs(Got / P - 1.0) < 0.02, "identity: P0 and its kicks pay back P in expectation (x" + Num(Got / P, 4) + " over 3,000 uploads)");
	}
}

// ================================================================== a Rerun 2.0 channel's careers for the invariants

struct Career
{
	video::Channel C;
	life::State L;
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	int Uploads = 0;
};

/** One day of a creator: on its weekdays a Breakdown, a Short or a Vlog (every third a Teaching bluff, the evergreen class), up at
 *  Hour; then the day's step. Duel: three recipes on every fifth upload (the content stubs score them neutrally). */
void CareerDay(Career& Cr, int D, const std::string& Seed, const std::vector<int>& Weekdays, bool Unfolded, double Hour = 3.0, bool Duels = false)
{
	const double At = static_cast<double>(D) * Day + Hour * 60.0;
	if (std::find(Weekdays.begin(), Weekdays.end(), D % 7) != Weekdays.end())
	{
		video::Choice Ch;
		Ch.F = static_cast<video::Format>(Cr.Uploads % 3);
		const bool Teach = Ch.F == video::Format::Breakdown && (Cr.Uploads / 3) % 3 == 0;
		Ch.T = Teach ? video::TitleStyle::Teaching : video::TitleStyle::Story;
		Ch.Fr = video::Frame::Versus;
		if (Duels && Cr.Uploads % 5 == 4 && Ch.F != video::Format::Short)
		{
			for (int K = 0; K < 3; ++K)
			{
				video::ThumbSpec S;
				S.Arch = static_cast<int16_t>(K);
				S.Layout = static_cast<int16_t>(K + 1);
				Ch.Thumbs.push_back(S);
			}
		}
		for (int I = 0; I < (Ch.F == video::Format::Vlog ? 3 : 1); ++I)
		{
			const std::string Id = "rl:car@" + std::to_string(D) + ":" + std::to_string(I);
			video::NoteHand(Cr.C, Moment(Id, At, Teach ? video::Kind::Bluff : video::Kind::Cooler));
			Ch.Candidates.push_back(Id);
		}
		video::UploadFacts F;
		F.GearQuality = 0.5;
		video::Upload(Cr.C, Ch, At + 30.0, Seed, F);
		++Cr.Uploads;
	}
	const double From = static_cast<double>(D) * Day;
	if (Unfolded)
	{
		ss::video::video_test_seam::StepUnfolded(Cr.C, Cr.L, Cr.Bank, Cr.K, From, From + Day);
	}
	else
	{
		video::Step(Cr.C, Cr.L, Cr.Bank, Cr.K, From, From + Day);
	}
}

Career NewCareer()
{
	Career Cr;
	Cr.L.RentsPaid = 2;
	Cr.L.Update2LinesRead |= life::U2Video;
	Cr.L.Update3LinesRead |= life::U3Video;
	Cr.C.X.On = true;
	return Cr;
}

std::string CareerText(const Career& Cr)
{
	life::State L = Cr.L;
	L.Video = Cr.C;
	return SaveText(L);
}

void Invariants()
{
	// I1: views never go down and are 0 before the upload, for a career's videos, a repackage down and a Duel's switch.
	{
		Career Cr = NewCareer();
		for (int D = 0; D < 40; ++D)
		{
			CareerDay(Cr, D, "i1", ThreeAWeek, false, 3.0, true);
		}
		video::Video Down = Cr.C.Videos.front();
		const video::Comp Rp = algo::RepackComp(Down, Down.At + 5.0 * Day, Down.X.Ctr * 0.5);
		Down.X.Comps.push_back(Rp);
		std::vector<video::Video> All = Cr.C.Videos;
		All.push_back(Down);
		bool Monotone = true, Before = true;
		for (const video::Video& V : All)
		{
			double Prev = 0.0;
			Before = Before && video::Views(V, V.At - 1.0) == 0.0;
			for (int I = 0; I <= 10000; ++I)
			{
				const double T = V.At + static_cast<double>(I) * 6.0;
				const double X = video::Views(V, T);
				Monotone = Monotone && X >= Prev - 1e-9;
				Prev = X;
			}
		}
		Expect(Monotone && Before && Rp.P < 0.0, "I1: views never go down at any of 10,000 minutes (a repackage down and the Duels included), 0 before the upload");
	}
	// I2: any split of a span steps the same days: 1 x 30, 30 x 1 and a random split give the same bytes.
	{
		Career A = NewCareer();
		for (int D = 0; D < 60; ++D)
		{
			CareerDay(A, D, "i2", Daily, false);
		}
		Career B = A, Cc = A;
		const double From = 60.0 * Day;
		video::Step(A.C, A.L, A.Bank, A.K, From, From + 30.0 * Day);
		for (int D = 0; D < 30; ++D)
		{
			video::Step(B.C, B.L, B.Bank, B.K, From + D * Day, From + (D + 1) * Day);
		}
		ss::Rng R("i2:split");
		for (double T = From; T < From + 30.0 * Day;)
		{
			const double Next = std::min(From + 30.0 * Day, T + R.Range(1.0, 4.0 * Day));
			video::Step(Cc.C, Cc.L, Cc.Bank, Cc.K, T, Next);
			T = Next;
		}
		Expect(CareerText(A) == CareerText(B) && CareerText(A) == CareerText(Cc) && A.Bank == B.Bank && B.Bank == Cc.Bank,
			"I2: a 30-day skip, 30 one-day steps and a random split write the same bytes");
	}
	// I3, I4: folded against unfolded over three years of a daily uploader (components, the season, the evergreen buckets).
	{
		Career F = NewCareer(), U = NewCareer();
		for (int D = 0; D < 3 * 365; ++D)
		{
			CareerDay(F, D, "i3", Daily, false);
			CareerDay(U, D, "i3", Daily, true);
		}
		const double End = 3.0 * 365.0 * Day;
		const double Vf = video::TotalViews(F.C, End), Vu = video::TotalViews(U.C, End);
		const ss::Chips Mf = F.C.BalanceCents + F.C.PaidCents, Mu = U.C.BalanceCents + U.C.PaidCents;
		Expect(F.C.Videos.size() <= 60 && U.C.Videos.size() > 900 && !F.C.X.StubDays.empty(), "I3: the folded run keeps 60 whole and buckets its evergreen stubs");
		Expect(std::fabs(F.C.Subs - U.C.Subs) / std::max(1.0, U.C.Subs) < 1e-6 && std::fabs(Vf - Vu) / std::max(1.0, Vu) < 1e-6,
			"I3: folding is exact: subscribers " + Num(F.C.Subs, 10) + " against " + Num(U.C.Subs, 10) + ", views " + Num(Vf, 10) + " against " + Num(Vu, 10));
		Expect(std::llabs(Mf - Mu) <= std::max<long long>(1, Mu / 100000000), "I3: and the money to the cent (" + std::to_string(Mf) + " against " + std::to_string(Mu) + ")");
		// I4: the same decisions either way, read on the videos both runs still keep whole.
		bool SameComps = true;
		int Compared = 0;
		for (const video::Video& V : F.C.Videos)
		{
			for (const video::Video& W : U.C.Videos)
			{
				if (V.Id == W.Id)
				{
					++Compared;
					SameComps = SameComps && V.X.Comps.size() == W.X.Comps.size() && V.X.Verdict == W.X.Verdict;
					for (size_t K = 0; SameComps && K < V.X.Comps.size(); ++K)
					{
						SameComps = V.X.Comps[K].P == W.X.Comps[K].P && V.X.Comps[K].T == W.X.Comps[K].T && V.X.Comps[K].Kind == W.X.Comps[K].Kind;
					}
				}
			}
		}
		Expect(SameComps && Compared >= 55 && F.C.X.MomH == U.C.X.MomH && F.C.X.StreakW == U.C.X.StreakW, "I4: folded and unfolded runs decide the same (" + std::to_string(Compared) + " videos)");
	}
	// I5: a read in the past never sees a later decision; I7: an old video gets nothing; I8: a reload never rerolls.
	{
		Career Cr = NewCareer();
		video::Video Old;
		Old.Id = "v-old";
		Old.At = 2.0 * Day;
		Old.P = 900.0;
		Old.A = 0.75;
		Old.Tau1 = 1.5;
		Old.Tau2 = 12.0;
		Old.Conv = 0.006;
		Old.Retention = 0.45;
		Old.LengthMin = 8.0;
		Old.Rpm = 2.5;
		Old.Title = "An old one";
		Cr.C.Videos.push_back(Old);
		Cr.C.SteppedTo = 3.0 * Day;
		for (int D = 3; D < 30; ++D)
		{
			CareerDay(Cr, D, "i5", ThreeAWeek, false);
		}
		const video::Video& First = Cr.C.Videos[1];
		const double Read = First.At + 6.5 * Day;
		const double Before = video::Views(First, Read);
		Career Later = Cr;
		for (int D = 30; D < 60; ++D)
		{
			CareerDay(Later, D, "i5", ThreeAWeek, false);
		}
		const auto Find = [](const video::Channel& C, const std::string& Id) {
			for (const video::Video& V : C.Videos)
			{
				if (V.Id == Id)
				{
					return V;
				}
			}
			return video::Video();
		};
		Expect(video::Views(Find(Later.C, First.Id), Read) == Before, "I5: a read at a past minute is unchanged by every decision after it");
		const video::Video OldNow = Find(Later.C, "v-old");
		Expect(!OldNow.X.On && OldNow.X.Comps.empty() && OldNow.P == Old.P && OldNow.Rpm == Old.Rpm, "I7: an Update 2 video on a Rerun 2.0 channel never gets a component or a field");
		// I8: save at a mid-day minute, reload, step on: the same bytes as never having saved.
		const std::string Mid = CareerText(Later);
		Career Back = Later;
		Back.C = Reload(Mid).Video;
		for (int D = 60; D < 75; ++D)
		{
			CareerDay(Later, D, "i5", ThreeAWeek, false);
			CareerDay(Back, D, "i5", ThreeAWeek, false);
		}
		Expect(Reload(Mid).Video.X.On && CareerText(Back) == CareerText(Later), "I8: a reload never rerolls (save, reload, step: the same bytes)");
	}
	// I9: the bounds over a long daily career with Duels.
	{
		Career Cr = NewCareer();
		size_t MaxComps = 0;
		double MinMc3 = 9.0, MaxMc3 = 0.0;
		int Uploads = 0, Breakouts = 0;
		for (int D = 0; D < 400; ++D)
		{
			CareerDay(Cr, D, "i9", Daily, false, 3.0, true);
			for (const video::Video& V : Cr.C.Videos)
			{
				MaxComps = std::max(MaxComps, V.X.Comps.size());
			}
			if (!Cr.C.Videos.empty() && valgo::DayOf(Cr.C.Videos.back().At) == D)
			{
				const video::Video& V = Cr.C.Videos.back();
				const double M3 = algo::Compress(V.X.Ctr / std::max(1e-9, V.X.Ctr) * (1.0 + algo::Kappa(V.X.S0)) / (1.0 + algo::Kappa(V.X.S0Base)), 0.30);
				MinMc3 = std::min(MinMc3, M3);
				MaxMc3 = std::max(MaxMc3, M3);
				++Uploads;
			}
			if (D >= 20)
			{
				for (const video::Video& V : Cr.C.Videos)
				{
					if (valgo::DayOf(V.At) == D - 20)
					{
						for (const video::Comp& K : V.X.Comps)
						{
							Breakouts += K.Kind == video::CompKind::Breakout ? 1 : 0;
						}
					}
				}
			}
		}
		Expect(MaxComps <= valgo::VaMaxComps && MinMc3 > 0.74 && MaxMc3 < 1.35 && Breakouts <= Uploads * 3 / 100 + 2,
			"I9: at most 9 components beside the first wave, Mc3 inside (0.74, 1.35), breakouts at most 3% (" + std::to_string(MaxComps) + ", " + std::to_string(Breakouts) + " of " +
				std::to_string(Uploads) + ")");
	}
}

// ================================================================== the levers and the decisions

video::Video* Newest(video::Channel& C)
{
	return C.Videos.empty() ? nullptr : &C.Videos.back();
}

void Decisions()
{
	life::State L;
	L.RentsPaid = 2;
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	// The verdict at the first midnight 48 hours on, the check-ins on days 7 and 14 (chance 0.6), a Strategy lesson's 30 and 90.
	{
		const double At = 10.0 * Day + 4.0 * 60.0;
		int Verdicts = 0, Checkins = 0, Strategy30 = 0, Strategy90 = 0, Lessons = 0, Bloom = 0, Late = 0;
		for (int I = 0; I < 400; ++I)
		{
			video::Channel C = PlainU3Channel(At);
			video::Candidate M = Moment("rl:dec@10:1", At - 60.0, I % 2 == 0 ? video::Kind::Bluff : video::Kind::Cooler);
			video::NoteHand(C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			Ch.T = I % 2 == 0 ? video::TitleStyle::Teaching : video::TitleStyle::Story;
			video::UploadFacts F;
			F.GearQuality = 0.5;
			UploadOn(C, Ch, At, "dec:" + std::to_string(I), F);
			video::Step(C, L, Bank, K, At, At + 100.0 * Day);
			const video::Video& V = C.Videos.back();
			for (const video::Comp& Cp : V.X.Comps)
			{
				const int Dd = valgo::DayOf(Cp.T) - valgo::DayOf(V.At);
				Verdicts += Cp.Kind == video::CompKind::Verdict && valgo::DayOf(Cp.T) == valgo::VerdictDayOf(V) ? 1 : 0;
				Checkins += (Cp.Kind == video::CompKind::Check7 && Dd == 7) || (Cp.Kind == video::CompKind::Check14 && Dd == 14) ? 1 : 0;
				Strategy30 += Cp.Kind == video::CompKind::Check30 ? 1 : 0;
				Strategy90 += Cp.Kind == video::CompKind::Check90 ? 1 : 0;
				Bloom += Cp.Kind == video::CompKind::LateBloom ? 1 : 0;
				Late += Cp.Kind == video::CompKind::LateBloom && V.C != video::Class::Strategy ? 1 : 0;
			}
			Lessons += V.C == video::Class::Strategy ? 1 : 0;
		}
		Expect(Verdicts == 400 && valgo::VerdictDayOf(video::Video()) == 2, "decisions: every video's verdict at the first midnight 48 hours after it went up");
		Expect(Checkins > 400 && Checkins < 560, "decisions: the day 7 and 14 check-ins come 60% of the time (" + std::to_string(Checkins) + " of 800)");
		Expect(Lessons == 200 && Strategy30 > 80 && Strategy90 > 80 && Strategy30 < 160, "decisions: a Teaching lesson is Strategy, checked on days 30 and 90 too");
		// (review-core-videoalgo: a floor too, so a Late Bloomer that never fires fails here: 400 draws at 4% expect 16.)
		Expect(Late == 0 && Bloom >= 4 && Bloom <= 40, "decisions: Late Bloomer only wakes a Strategy lesson, about 4% of its day-30 and day-90 check-ins (" + std::to_string(Bloom) + ")");
	}
	// A daily Strategy uploader: a lesson 30 or more uploads back at its day-30 midnight takes its day-90 share then.
	{
		Career Cr = NewCareer();
		int Merged = 0, Ninety = 0;
		for (int D = 0; D < 130; ++D)
		{
			const double At = static_cast<double>(D) * Day + 4.0 * 60.0;
			video::Candidate M = Moment("rl:lesson@" + std::to_string(D) + ":1", At - 60.0, video::Kind::Bluff);
			video::NoteHand(Cr.C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			Ch.T = video::TitleStyle::Teaching;
			video::UploadFacts F;
			F.GearQuality = 0.5;
			video::Upload(Cr.C, Ch, At, "merge", F);
			video::Step(Cr.C, Cr.L, Cr.Bank, Cr.K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		}
		for (const video::Video& V : Cr.C.Videos)
		{
			for (const video::Comp& Cp : V.X.Comps)
			{
				Merged += Cp.Kind == video::CompKind::Check30 ? 1 : 0;
				Ninety += Cp.Kind == video::CompKind::Check90 ? 1 : 0;
			}
		}
		Expect(Merged > 20 && Ninety == 0, "decisions: a daily lesson uploader's videos take their day-90 share at day 30 (" + std::to_string(Merged) + " merged, " + std::to_string(Ninety) +
			" at day 90)");
	}
	// The Duel: decided at the verdict on watch time (CTR x the contender's own retention), from saved rolls; its CTR takes over.
	{
		const double At = 12.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		const video::Candidate M = Moment("rl:duel@12:1", At - 60.0, video::Kind::BadBeat);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		for (int I = 0; I < 3; ++I)
		{
			video::ThumbSpec S;
			S.Arch = static_cast<int16_t>(I);
			Ch.Thumbs.push_back(S);
		}
		video::UploadFacts F;
		F.GearQuality = 0.5;
		UploadOn(C, Ch, At, "duel", F);
		video::Video& V = C.Videos.back();
		const std::array<float, 3> Ctrs = V.X.DuelCtr;
		const double Mean = (static_cast<double>(Ctrs[0]) + static_cast<double>(Ctrs[1]) + static_cast<double>(Ctrs[2])) / 3.0;
		const int Best = static_cast<int>(std::max_element(Ctrs.begin(), Ctrs.end()) - Ctrs.begin());
		Expect(V.X.Duels == 3 && V.Fr == video::Frame::Recipe && std::fabs(V.X.Ctr - Mean) < 1e-6 && V.X.DuelWin < 0, "duel: three contenders, served at their mean CTR for 48 hours");
		video::Step(C, L, Bank, K, At, At + 4.0 * Day);
		const video::Video& W = C.Videos.back();
		const bool Switched = std::any_of(W.X.Comps.begin(), W.X.Comps.end(), [](const video::Comp& Cp) { return Cp.Kind == video::CompKind::Duel; });
		Expect(W.X.DuelWin == Best && Switched && W.X.Ctr == valgo::VaRound(static_cast<double>(Ctrs[static_cast<size_t>(Best)]), 1e7),
			"duel: the verdict picks the most watch time and its CTR takes over");
	}
	// A Next-up card: the linking video's verdict sends 2% of what it still has to come; a Cut's link 3% (1 + Q) of its views.
	{
		const double At = 14.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		video::Candidate A = Moment("rl:nu@14:1", At - 60.0), B = Moment("rl:nu@14:2", At - 50.0), S = Moment("rl:nu@14:3", At - 40.0);
		video::NoteHand(C, A);
		video::NoteHand(C, B);
		video::NoteHand(C, S);
		video::UploadFacts F;
		F.GearQuality = 0.5;
		video::Choice Ca;
		Ca.Candidates = {A.Id};
		Ca.F = video::Format::Breakdown;
		const std::string Target = video::Upload(C, Ca, At, "nu", F);
		video::Choice Cb = Ca;
		Cb.Candidates = {B.Id};
		Cb.NextUp = Target;
		video::Upload(C, Cb, At + 60.0, "nu", F);
		video::Choice Cs = Ca;
		Cs.Candidates = {S.Id};
		Cs.F = video::Format::Short;
		Cs.NextUp = Target;
		video::Upload(C, Cs, At + 120.0, "nu", F);
		video::Step(C, L, Bank, K, At, At + 4.0 * Day);
		int FromNext = 0, FromCut = 0;
		for (const video::Comp& Cp : C.Videos.front().X.Comps)
		{
			FromNext += Cp.Kind == video::CompKind::FromNextUp ? 1 : 0;
			FromCut += Cp.Kind == video::CompKind::FromCut ? 1 : 0;
		}
		Expect(FromNext == 1 && FromCut == 1, "links: a Next-up card and a Cut each send viewers to the video they link, once");
	}
	// The review waits for the second rent on a Rerun 2.0 channel (W11); an Update 2 channel's doesn't.
	{
		for (const int Rents : {1, 2})
		{
			video::Channel C = PlainU3Channel(20.0 * Day);
			C.Subs = 1500.0;
			C.GrantedHours = 5000.0;
			video::Channel Legacy = C;
			Legacy.X = video::ChannelX();
			life::State Ls;
			Ls.RentsPaid = Rents;
			video::Step(C, Ls, Bank, K, 20.0 * Day, 22.0 * Day);
			video::Step(Legacy, Ls, Bank, K, 20.0 * Day, 22.0 * Day);
			Expect((C.ReviewAt >= 0.0) == (Rents >= 2) && Legacy.ReviewAt >= 0.0,
				std::string("review: a Rerun 2.0 channel's review ") + (Rents >= 2 ? "starts after the second rent" : "waits for the second rent") + "; Update 2's doesn't wait");
		}
	}
}

void Levers()
{
	life::State L;
	L.RentsPaid = 2;
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	// The ad-load switch (§4.5.5): Heavy from the next midnight; that day on, the RPM, retention, conversion, dislikes and S move.
	{
		const double At = 30.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		C.PartnerAt = 0.0;
		const video::Candidate M = Moment("rl:ads@30:1", At - 60.0);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Vlog;
		Ch.Candidates = {M.Id, "rl:ads@30:2", "rl:ads@30:3"};
		video::NoteHand(C, Moment("rl:ads@30:2", At - 50.0));
		video::NoteHand(C, Moment("rl:ads@30:3", At - 40.0));
		video::UploadFacts F;
		F.GearQuality = 0.5;
		UploadOn(C, Ch, At, "ads", F);
		video::Step(C, L, Bank, K, At, At + 20.0 * Day);
		video::Video& V = C.Videos.back();
		const double Rpm = V.Rpm, Ret = V.Retention, Conv = V.Conv, S0 = V.X.S0;
		const int Today = valgo::DayOf(At + 20.0 * Day);
		V.X.Ads = video::AdLoad::Heavy;
		V.X.AdsChangedDay = Today + 1;
		const double Want = 2.40 / 1.68 * (1.0 + 0.6 * (0.92 - 1.0));
		Expect(valgo::RpmOn(V, Today) == Rpm && std::fabs(valgo::RpmOn(V, Today + 1) / Rpm - Want) < 1e-9,
			"ads: a switch to Heavy pays from the next midnight: +43% ads per playback on an 18-minute Vlog, less its retention's pay (x" + Num(valgo::RpmOn(V, Today + 1) / Rpm, 4) + ")");
		video::Step(C, L, Bank, K, At + 20.0 * Day, At + 22.0 * Day);
		const video::Video& W = C.Videos.back();
		Expect(W.X.AdsAtUpload == video::AdLoad::Heavy && std::fabs(W.Retention / Ret - 0.92) < 1e-9 && std::fabs(W.Conv / Conv - 0.90) < 1e-9 && std::fabs(W.X.S0 - S0 + 0.10) < 1e-12,
			"ads: from that day the stored rates are Heavy's: retention x0.92, conversion x0.90, the score -0.10");
		const valgo::Rates Std = valgo::RatesOf(W, video::AdLoad::Standard, W.Retention, 0), Hvy = valgo::RatesOf(W, video::AdLoad::Heavy, W.Retention, 0);
		Expect(std::fabs(Hvy.DislikePerLike / Std.DislikePerLike - 1.35) < 1e-9, "ads: Heavy's dislikes per like x1.35");
		// The identity on the day: CPM x monetized x ads per playback x 55% is the RPM paid, for every format, length, load, coin.
		double Worst = 0.0;
		for (int Fm = 0; Fm < 4; ++Fm)
		{
			for (int Ln = 0; Ln < 3; ++Ln)
			{
				for (int Ad = 0; Ad < 3; ++Ad)
				{
					for (int Co = 0; Co < 2; ++Co)
					{
						video::Video X = W;
						X.F = static_cast<video::Format>(Fm);
						X.X.Len = static_cast<video::LengthOpt>(Ln);
						X.X.Ads = X.X.AdsAtUpload = static_cast<video::AdLoad>(Ad);
						X.X.C = static_cast<video::Coin>(Co);
						const algo::Money Mo = algo::MoneyOf(X, Today);
						Worst = std::max(Worst, std::fabs(Mo.Cpm * Mo.MonetizedRate * Mo.AdsPerPlayback * Mo.Share - Mo.Rpm));
					}
				}
			}
		}
		Expect(Worst < 1e-9, "money: CPM x monetized x ads per playback x share is the RPM to 1e-9 for every format, length, load and coin");
	}
	// Burnout counts Update 3's extra editing only: three-contender Duels every day burn, Update 2's own edits never do.
	{
		Career Cr = NewCareer();
		for (int D = 0; D < 20; ++D)
		{
			CareerDay(Cr, D, "burn", Daily, false, 3.0, true);
		}
		const double Extra = algo::ExtraHours7(Cr.C, 20.0 * Day);
		Career Plain = NewCareer();
		for (int D = 0; D < 20; ++D)
		{
			CareerDay(Plain, D, "burn", Daily, false);
		}
		Expect(Extra > 0.5 && algo::ExtraHours7(Plain.C, 20.0 * Day) == 0.0, "burnout: a Duel's extra contenders count as extra editing; a daily Update 2 rotation has none");
	}
	// Live Drop (SPEC_CREATORS §3.4): a stream of 30+ minutes within 6 hours after the publish, once a video, sized by its own average.
	{
		const double At = 16.0 * Day + 20.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		const video::Candidate M = Moment("rl:drop@16:1", At - 60.0);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		video::UploadFacts F;
		F.GearQuality = 0.5;
		UploadOn(C, Ch, At, "drop", F);
		algo::NoteStream(C, At + 20.0, At + 40.0, 50, 40.0, 40.0); // 20 minutes: too short
		const size_t Before = C.Videos.back().X.Comps.size();
		algo::NoteStream(C, At + 60.0, At + 60.0 + 180.0, 300, 300.0, 100.0); // 3 hours, a 300 average against a 100 norm
		const video::Video& V = C.Videos.back();
		const double Push = V.X.Comps.empty() ? 0.0 : V.X.Comps.back().P;
		algo::NoteStream(C, At + 300.0, At + 400.0, 300, 300.0, 100.0); // a second stream: once a video
		Expect(Before == 0 && V.X.Comps.size() == 1 && std::fabs(Push - 1.2 * 150.0 * 2.0) < 1e-9 && C.Videos.back().X.Comps.size() == 1 &&
				   (V.X.Launch & (1u << static_cast<int>(video::Launch::Kast))) != 0,
			"live drop: 1.2 x min(AvgOwn, 1.5 x Norm) x min(2, hours) once a video, nothing for a stream under 30 minutes");
		video::Channel Late = PlainU3Channel(At);
		video::NoteHand(Late, M);
		UploadOn(Late, Ch, At, "drop", F);
		algo::NoteStream(Late, At + 7.0 * 60.0, At + 10.0 * 60.0, 300, 300.0, 100.0);
		Expect(Late.Videos.back().X.Comps.empty() && Late.Videos.back().X.Launch == 0, "live drop: a stream 7 hours after the publish is no Live Drop");
		const double Sv = valgo::ScoreNow(C.Videos.back(), C, valgo::VerdictDayOf(C.Videos.back()), 1);
		Expect(std::fabs(Sv - C.Videos.back().X.S0 - 0.10) < 1e-12, "live drop: V +0.10 at the verdict");
	}
	// The upload streak: weeks in a row with an upload (at most 8), a Rain Check every 4 (at most 2), a missed week halving it.
	{
		Career Cr = NewCareer();
		int Peak = 0, PeakRain = 0, AfterGap = -1, RainBells = 0;
		for (int D = 0; D < 120; ++D)
		{
			const bool Gap = D >= 84 && D < 105; // three weeks off
			const size_t Bells = Cr.C.Bell.size();
			const double Last = Cr.C.Bell.empty() ? -1.0 : Cr.C.Bell.back().first;
			CareerDay(Cr, D, "streak", Gap ? std::vector<int>{} : Weekly, false);
			for (const auto& B : Cr.C.Bell)
			{
				RainBells += (B.first > Last || Bells == 0) && B.second.find("Rain Check") != std::string::npos ? 1 : 0;
			}
			Peak = std::max(Peak, Cr.C.X.StreakW);
			PeakRain = std::max(PeakRain, Cr.C.X.RainChecks);
			AfterGap = D == 105 ? Cr.C.X.StreakW : AfterGap;
		}
		Expect(Peak == 8 && PeakRain == 2 && AfterGap == 4 && Cr.C.X.StreakW == 6,
			"streak: 8 weeks in a row at most, 2 Rain Checks covering two missed weeks, the third halving it, then counting up again (" + std::to_string(AfterGap) + ", " +
				std::to_string(Cr.C.X.StreakW) + ")");
		Expect(RainBells == 2, "streak: the bell says each Rain Check earned (weeks 4 and 8), once (" + std::to_string(RainBells) + ")");
	}
	// Burnout's warning (§4.6.6): past 4 extra hours in a week the bell says so with the hours, at most once a week.
	{
		const double At = 50.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		std::vector<std::pair<double, std::string>> Warnings;
		for (int I = 0; I < 11; ++I)
		{
			const double When = At + static_cast<double>(I) * Day;
			std::vector<std::string> Ids;
			for (int J = 0; J < 3; ++J)
			{
				const video::Candidate M = Moment("rl:burn@" + std::to_string(50 + I) + ":" + std::to_string(J), When - 60.0 + J);
				video::NoteHand(C, M);
				Ids.push_back(M.Id);
			}
			video::Choice Ch;
			Ch.Candidates = Ids;
			Ch.F = video::Format::Vlog;
			Ch.Len = video::LengthOpt::Extended; // 40 minutes over Standard
			Ch.Thumbs = std::vector<video::ThumbSpec>(3, video::ThumbSpec());
			for (int Tk = 0; Tk < 3; ++Tk)
			{
				Ch.Thumbs[static_cast<size_t>(Tk)].Arch = static_cast<int16_t>(Tk + 1); // a three-way Duel: 30 more minutes
			}
			video::UploadFacts F;
			F.GearQuality = 0.5;
			video::Upload(C, Ch, When, "burn", F);
			for (const auto& B : C.Bell)
			{
				if (B.second.rfind("You've put ", 0) == 0 && std::none_of(Warnings.begin(), Warnings.end(), [&B](const std::pair<double, std::string>& W) { return W.first == B.first; }))
				{
					Warnings.push_back(B);
				}
			}
		}
		// 70 minutes a day: the fourth upload is the first past 4 hours (4.7, "5 extra hours"); the next warning waits a week, for
		// the eleventh (seven uploads in its 7 days: 8.2 hours).
		Expect(Warnings.size() == 2 && Warnings[0].first == C.Videos[3].At && Warnings[0].second == valgo::BellBurnout(4.0 * 70.0 / 60.0) &&
				   Warnings[1].first == C.Videos[10].At && Warnings[1].second == valgo::BellBurnout(7.0 * 70.0 / 60.0),
			"burnout: the bell warns past 4 extra hours with the week's hours, at most once a week (" + std::to_string(Warnings.size()) + ")");
	}
	// (review) Burnout's Insight says what the upload read: a video's own launch actions come after it went up, so a teaser, a
	// reply round and a Cut on the third of three 70-minute uploads (3.5 extra hours at its upload, 4.3 with them) bring no
	// "You put ..." Insight; the fourth, 5.5 hours when it went up, has one with that week's hours.
	{
		const double At = 50.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		auto Up = [&C, At](int I) {
			const double When = At + static_cast<double>(I) * Day;
			std::vector<std::string> Ids;
			for (int J = 0; J < 3; ++J)
			{
				const video::Candidate M = Moment("rl:burni@" + std::to_string(50 + I) + ":" + std::to_string(J), When - 60.0 + J);
				video::NoteHand(C, M);
				Ids.push_back(M.Id);
			}
			video::Choice Ch;
			Ch.Candidates = Ids;
			Ch.F = video::Format::Vlog;
			Ch.Len = video::LengthOpt::Extended;
			Ch.Thumbs = std::vector<video::ThumbSpec>(3, video::ThumbSpec());
			for (int Tk = 0; Tk < 3; ++Tk)
			{
				Ch.Thumbs[static_cast<size_t>(Tk)].Arch = static_cast<int16_t>(Tk + 1);
			}
			video::UploadFacts F;
			F.GearQuality = 0.5;
			video::Upload(C, Ch, When, "burni", F);
		};
		auto Said = [&C](size_t I) {
			const video::VideoStats S = video::StatsOf(C, C.Videos[I], C.Videos[I].At + 60.0);
			for (const std::string& L : S.Insights)
			{
				if (L.rfind("You put ", 0) == 0)
				{
					return L;
				}
			}
			return std::string();
		};
		Up(0);
		Up(1);
		Up(2);
		C.Videos[2].X.Launch = static_cast<uint8_t>((1u << static_cast<int>(video::Launch::Cut)) | (1u << static_cast<int>(video::Launch::Teaser)) |
			(1u << static_cast<int>(video::Launch::Reply)));
		const std::string Third = Said(2);
		Up(3);
		const std::string Fourth = Said(3);
		Expect(Third.empty() && Fourth == "You put 6 extra hours into that week's videos. It showed: viewers left sooner.",
			"burnout: the Insight counts what the upload read, not the video's own later launch actions ('" + Third + "', '" + Fourth + "')");
	}
	// Fresh on Rerun: a new channel's first uploads find viewers; the count never falls (a deleted video can't re-open it).
	{
		const double At = 8.0 * Day + 4.0 * 60.0;
		video::Channel New = PlainU3Channel(At);
		New.X.UploadsEver = 0;
		video::Channel Old = PlainU3Channel(At);
		const video::Candidate M = Moment("rl:fresh@8:1", At - 60.0);
		video::NoteHand(New, M);
		video::NoteHand(Old, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		video::UploadFacts F;
		F.GearQuality = 0.5;
		const video::Video A = UploadOn(New, Ch, At, "fresh", F);
		const video::Video B = UploadOn(Old, Ch, At, "fresh", F);
		New.Videos.clear();
		Expect(std::fabs(A.P / B.P - 3000.0 / 1200.0) < 0.01 && New.X.UploadsEver == 1, "fresh: the first upload reaches 2.5x (Base's 1,200 + 1,800), and the count stays after a delete");
	}
	// Suitability (§4.5.4): an idea that shows a real-money site's promotions runs Limited ads (CPM x0.40, 30% monetized), and
	// the first such coin brings one letter, ever.
	{
		const double At = 22.0 * Day + 4.0 * 60.0;
		life::State Ls;
		Ls.RentsPaid = 2;
		video::Channel C = PlainU3Channel(At);
		for (int I = 0; I < 2; ++I)
		{
			video::Candidate M = Moment("idea:5@22:" + std::to_string(I), At - 60.0 + I);
			M.Source = 5;
			M.K = video::Kind::Idea;
			M.Idea = 5;
			M.IdeaFacts = "site=ignite\tltd=1";
			video::NoteHand(C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			video::UploadFacts F;
			F.GearQuality = 0.5;
			video::Upload(C, Ch, At + I * 3.0 * Day, "limited", F);
			video::Step(C, Ls, Bank, K, At + I * 3.0 * Day, At + (I + 1) * 3.0 * Day);
		}
		int Letters = 0;
		for (const life::Letter& M : Ls.Inbox)
		{
			Letters += M.Kind == valgo::VaLetterLimited ? 1 : 0;
		}
		const algo::Money Mo = algo::MoneyOf(C.Videos.front(), 23);
		Expect(C.Videos.front().X.C == video::Coin::Limited && Mo.MonetizedRate == 0.30 && Letters == 1 && C.Paid.count("notice:limited") == 1,
			"suitability: a site's promotion runs Limited ads, and its first coin brings one letter");
	}
	// The Binge wave never touches a legacy video; KastHeatSince lists Rerun's bumps oldest first.
	{
		const double At = 40.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		for (int I = 0; I < 6; ++I)
		{
			video::Video Old;
			Old.Id = "legacy" + std::to_string(I);
			Old.At = At - static_cast<double>(6 - I) * Day;
			Old.P = 100.0;
			Old.A = 0.75;
			Old.Tau1 = 1.5;
			Old.Tau2 = 12.0;
			Old.Title = "old";
			C.Videos.push_back(Old);
		}
		const video::Candidate M = Moment("rl:hit@40:1", At - 60.0);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		video::UploadFacts F;
		F.GearQuality = 0.5;
		F.KastFollowers = 20000; // a hit against the old videos' 100 views
		UploadOn(C, Ch, At, "binge", F);
		video::Step(C, L, Bank, K, At, At + 4.0 * Day);
		bool Untouched = true;
		for (const video::Video& V : C.Videos)
		{
			Untouched = Untouched && (V.X.On || V.X.Comps.empty());
		}
		Expect(Untouched && C.Videos.back().P > 300.0, "binge: a hit's wave skips the legacy videos before it (V6)");
		C.Videos.back().X.Verdict = 4;
		const auto Bumps = algo::KastHeatSince(C, 0.0);
		Expect(!Bumps.empty() && Bumps.front().second == (C.Videos.back().X.Viral ? 10.0 : 6.0) && algo::KastHeatSince(C, C.SteppedTo).empty(),
			"kast heat: a Pushed hard verdict is +6 at its midnight (a viral +10), and nothing after the last go-live");
	}
}

// ================================================================== suitability, payoff, the idea's facts and the trends (wave 2)

/** One upload on a fresh Rerun 2.0 channel (past Fresh's twelve) of a single candidate, with the open moments Extra beside it. */
video::Video OneUpload(const video::Candidate& M, const video::Choice& Base, double At, const std::string& Seed, const std::vector<video::Candidate>& Extra = {},
	video::Channel* Out = nullptr)
{
	video::Channel C = PlainU3Channel(At);
	for (const video::Candidate& X : Extra)
	{
		video::NoteHand(C, X);
	}
	video::NoteHand(C, M);
	video::Choice Ch = Base;
	Ch.Candidates = {M.Id};
	video::UploadFacts F;
	F.GearQuality = 0.5;
	const video::Video V = UploadOn(C, Ch, At, Seed, F);
	if (Out)
	{
		*Out = C;
	}
	return V;
}

video::Candidate IdeaMoment(const std::string& Id, double World, const std::string& Facts)
{
	video::Candidate M = Moment(Id, World);
	M.Source = 5;
	M.K = video::Kind::Idea;
	M.Idea = 7;
	M.IdeaFacts = Facts;
	return M;
}

void SuitabilityAndTrends()
{
	const double At = 30.0 * Day + 4.0 * 60.0;
	video::Choice Bd;
	Bd.F = video::Format::Breakdown;
	// Payoff (§4.6.5): a pot in the top 10% of the player's own (the open moments, 10 or more) pays off at 3; fewer, the drama.
	{
		std::vector<video::Candidate> Others;
		for (int I = 0; I < 12; ++I)
		{
			video::Candidate X = Moment("rl:pots@30:" + std::to_string(I), At - 300.0 + I);
			X.Rec.Pot = 4000 + 400 * I; // 20-44 BB against the lead's 64
			Others.push_back(X);
		}
		video::Choice Ch = Bd;
		Ch.Thumbs = {video::ThumbSpec()};
		Ch.Thumbs[0].Arch = 3;
		const video::Candidate Lead = Moment("rl:pots@30:lead", At - 60.0, video::Kind::BigPot); // drama 0.55: payoff 1 on its own
		const video::Video Big = OneUpload(Lead, Ch, At, "payoff", Others);
		const video::Video Alone = OneUpload(Lead, Ch, At, "payoff", std::vector<video::Candidate>(Others.begin(), Others.begin() + 5));
		video::Candidate Small = Lead;
		Small.Rec.Pot = 3000;
		const video::Video Low = OneUpload(Small, Ch, At, "payoff", Others);
		Expect(Big.X.Payoff == 3 && Alone.X.Payoff < 3 && Low.X.Payoff < 3,
			"payoff: the biggest of 13 pots pays off at 3; with 6 moments to compare, or a small pot, the drama decides (" + std::to_string(Big.X.Payoff) + ", " +
				std::to_string(Alone.X.Payoff) + ", " + std::to_string(Low.X.Payoff) + ")");
		// (review) The edge of the top 10%: of 20 pots, one bigger is inside it, two bigger are not.
		std::vector<video::Candidate> Nineteen;
		for (int I = 0; I < 19; ++I)
		{
			video::Candidate X = Moment("rl:pots@30:t" + std::to_string(I), At - 400.0 + I);
			X.Rec.Pot = 4000 + 400 * I; // 20-56 BB against the lead's 64
			Nineteen.push_back(X);
		}
		Nineteen[18].Rec.Pot = 20000; // 100 BB: one bigger
		const video::Video Second = OneUpload(Lead, Ch, At, "payoff", Nineteen);
		Nineteen[17].Rec.Pot = 16000; // 80 BB: two bigger
		const video::Video Third = OneUpload(Lead, Ch, At, "payoff", Nineteen);
		Expect(Second.X.Payoff == 3 && Third.X.Payoff < 3,
			"payoff: of 20 pots the second biggest is in the top 10% (3), the third isn't (" + std::to_string(Second.X.Payoff) + ", " + std::to_string(Third.X.Payoff) + ")");
	}
	// Strong language (§4.5.4): a tilted face with a Loud title runs Limited ads unless the edit is clean, whose 15 minutes
	// count toward burnout from the saved fields alone (no flag).
	{
		video::Choice Ch = Bd;
		Ch.T = video::TitleStyle::Hype;
		Ch.Thumbs = {video::ThumbSpec()};
		Ch.Thumbs[0].Arch = 1;
		Ch.Thumbs[0].Expr = static_cast<int16_t>(valgo::VaExprTilted);
		const video::Candidate M = Moment("rl:tilt@30:1", At - 60.0);
		video::Channel Raw, Clean;
		const video::Video Swearing = OneUpload(M, Ch, At, "tilt", {}, &Raw);
		Ch.CleanEdit = true;
		const video::Video Edited = OneUpload(M, Ch, At, "tilt", {}, &Clean);
		video::Choice Calm = Ch;
		Calm.Thumbs[0].Expr = 1;
		const video::Video Smiling = OneUpload(M, Calm, At, "tilt");
		Expect(Swearing.X.C == video::Coin::Limited && Edited.X.C == video::Coin::Full && Smiling.X.C == video::Coin::Full && valgo::StrongLanguage(Edited) &&
				   !valgo::AdultOnly(Swearing),
			"suitability: strong language runs Limited ads (not 18+); a clean edit keeps full ads; a calm face needs none");
		Expect(std::fabs(algo::ExtraHours7(Clean, At + 60.0) - 0.25) < 1e-12 && algo::ExtraHours7(Raw, At + 60.0) == 0.0,
			"suitability: a clean edit's 15 minutes count as Update 3's extra editing, read back from the coin and the face");
	}
	// Site promotion is 18+ (§4.5.4): Home x0.6 and no Cuts feed, outside the compression; the sources say so.
	{
		const video::Candidate Promo = IdeaMoment("idea:7@30:1", At - 60.0, "site=ignite;ltd=1");
		const video::Candidate Plain = IdeaMoment("idea:7@30:1", At - 60.0, "site=ignite");
		const video::Video A = OneUpload(Promo, Bd, At, "adult"), B = OneUpload(Plain, Bd, At, "adult");
		const double Feed = std::max(0.0, 0.93 - B.X.KastShare) * (1.0 - B.X.Sigma);
		video::Choice Cut = Bd;
		Cut.F = video::Format::Short;
		const video::Video Ac = OneUpload(Promo, Cut, At, "adult"), Bc = OneUpload(Plain, Cut, At, "adult");
		const std::array<double, 10> Mix = valgo::MixOf(A, 0), MixB = valgo::MixOf(B, 0), MixC = valgo::MixOf(Ac, 0);
		double Sum = 0.0, SumB = 0.0;
		for (int S = 0; S < 10; ++S)
		{
			Sum += Mix[static_cast<size_t>(S)];
			SumB += MixB[static_cast<size_t>(S)];
		}
		Expect(A.X.C == video::Coin::Limited && valgo::AdultOnly(A) && std::fabs(A.P / B.P - (1.0 - 0.4 * Feed)) < 0.002 && std::fabs(Ac.P / Bc.P - (1.0 - Feed)) < 0.004,
			"suitability: site promotion is 18+: Home x0.6 for a long video (x" + Num(A.P / B.P, 4) + "), no Cuts feed for a Cut (x" + Num(Ac.P / Bc.P, 4) + ")");
		Expect(std::fabs(Sum - SumB) < 1e-12 && Mix[0] / Sum < MixB[0] / SumB && MixC[5] == 0.0, "suitability: an 18+ video's sources show less Home and no Cuts feed, summing as before");
	}
	// The idea's advertiser category (§4.5.4): Setup and gear 1.25 against a Story title's 1.00, Rival drama 0.85.
	{
		const video::Video Gear = OneUpload(IdeaMoment("idea:7@30:2", At - 60.0, "ad=gear"), Bd, At, "fit");
		const video::Video Rival = OneUpload(IdeaMoment("idea:7@30:2", At - 60.0, "ad=rival"), Bd, At, "fit");
		const video::Video None = OneUpload(IdeaMoment("idea:7@30:2", At - 60.0, "ad=nonsense"), Bd, At, "fit");
		Expect(std::fabs(Gear.Rpm / None.Rpm - 1.25) < 1e-3 && std::fabs(Rival.Rpm / None.Rpm - 0.85) < 1e-3 && None.Rpm == 2.5,
			"money: an idea's advertiser category sets its CPM (gear x1.25, rival drama x0.85; an unknown one keeps the title's)");
		Expect(valgo::FactValues("a=1;trend=champ:5;trend=poy:2026 b=2\tc=3", "trend") == std::vector<std::string>{"champ:5", "poy:2026"} &&
				   valgo::FactValues("xtrend=1;trendy=2", "trend").empty(),
			"facts: key=value tokens split on ';', spaces and tabs, the key matched whole");
	}
	// An idea's Topic (§6.3): its weight in place of the kind's 1.0, x0.5 inside its cooldown; the upload records the use, and
	// the repeat's Insight says how many days earlier the audience saw it.
	{
		const video::Video Plain = OneUpload(IdeaMoment("idea:7@30:6", At - 60.0, ""), Bd, At, "topic");
		video::Channel C;
		const video::Video Heavy = OneUpload(IdeaMoment("idea:7@30:6", At - 60.0, "w=1.6"), Bd, At, "topic", {}, &C);
		video::UploadFacts F;
		F.GearQuality = 0.5;
		auto Again = [&](const std::string& Facts, double When, const std::string& Seed) {
			const video::Candidate M = IdeaMoment("idea:7@" + Seed, When - 30.0, Facts);
			video::NoteHand(C, M);
			video::Choice Ch = Bd;
			Ch.Candidates = {M.Id};
			return UploadOn(C, Ch, When, Seed, F);
		};
		// (The repeat's twin: the same channel, seed and minute with a cooldown of 0 days.)
		video::Channel Twin = C;
		const video::Video Repeat = Again("w=1.6", At + 3.0 * Day, "topic");
		std::swap(C, Twin);
		const video::Video RepeatTwin = Again("w=1.6;cool=0", At + 3.0 * Day, "topic");
		std::swap(C, Twin);
		const video::Video Later = Again("w=1.6;cool=2", At + 6.0 * Day, "later");
		const video::VideoStats Rs = video::StatsOf(C, C.Videos[1], At + 6.0 * Day);
		const int Cooled = C.X.IdeaCool.empty() ? 0 : C.X.IdeaCool.front().Count;
		Expect(std::fabs(Heavy.P / Plain.P - 1.6) < 0.005 && std::fabs(Repeat.P / RepeatTwin.P - 0.5) < 0.005 && Later.P > 0.0 && Cooled == 3 &&
				   std::find(Rs.Insights.begin(), Rs.Insights.end(), "Your audience saw this idea 3 days earlier.") != Rs.Insights.end(),
			"ideas: the idea's weight (x" + Num(Heavy.P / Plain.P, 4) + "), x0.5 inside its cooldown, three uses recorded, the repeat's Insight");
		// (review) The cooldown's edge: 7 days after (the default's length) it reaches in full, 6 days after at half.
		auto RepeatAfter = [&](int Days) {
			video::Channel Once;
			OneUpload(IdeaMoment("idea:7@30:7", At - 60.0, "w=1.6"), Bd, At, "edge", {}, &Once);
			video::Channel Twice = Once;
			const double When = At + static_cast<double>(Days) * Day;
			std::swap(C, Once);
			const video::Video A = Again("w=1.6", When, "edge" + std::to_string(Days));
			std::swap(C, Once);
			std::swap(C, Twice);
			const video::Video B = Again("w=1.6;cool=0", When, "edge" + std::to_string(Days));
			std::swap(C, Twice);
			return A.P / B.P;
		};
		const double Seven = RepeatAfter(7), Six = RepeatAfter(6);
		Expect(std::fabs(Seven - 1.0) < 1e-9 && std::fabs(Six - 0.5) < 0.005,
			"ideas: the cooldown lasts 7 days, not 8 (x" + Num(Seven, 4) + " a week later, x" + Num(Six, 4) + " six days later)");
	}
	// Trends (§4.6.10): self-describing keys, a triangular heat, the upload's fit and score, the component at the next
	// midnight, the third matching upload's nothing, "few" x1.10, a series week, trend tourists.
	{
		valgo::VaTrend T;
		// (The world log's keys: the window runs from the first Monday the event is a day old by, the week's list's.)
		const bool Champ = valgo::TrendOfKey("champ:43200", T) && T.Start == 35.0 * Day && T.End == 38.0 * Day && T.Strength == 1.15;
		const bool Edge = valgo::TrendOfKey("final:8639", T) && T.Start == 7.0 * Day && valgo::TrendOfKey("final:8640", T) && T.Start == 14.0 * Day && T.Strength == 1.10;
		const bool Sunday = valgo::TrendOfKey("sunday:6", T) && T.Start == 6.0 * Day + 720.0 && T.End == 7.0 * Day + 720.0 && T.Strength == 1.08;
		const bool Poy = valgo::TrendOfKey("poy:2026", T) && T.Start == static_cast<double>(ss::world::DayOn(2026, 11, 15)) * Day &&
			T.End == static_cast<double>(ss::world::DayOn(2027, 1, 1)) * Day && T.Strength == 1.10;
		const bool NewYear = valgo::TrendOfKey("newyear:2026", T) && T.End == static_cast<double>(ss::world::DayOn(2027, 1, 16)) * Day;
		const bool Bad = !valgo::TrendOfKey("champ:", T) && !valgo::TrendOfKey("nope:5", T) && !valgo::TrendOfKey("champ:5x", T);
		valgo::TrendOfKey("site:1000", T); // a launch on day 0: the list takes it in on day 7
		Expect(Champ && Edge && Sunday && Poy && NewYear && Bad && T.Start == 7.0 * Day && valgo::TrendHeat(T, 7.0 * Day + 3.5 * Day) == 1.0 &&
				   valgo::TrendHeat(T, 7.0 * Day - 1.0) == 0.0 && std::fabs(valgo::TrendHeat(T, 7.0 * Day + 1.75 * Day) - 0.5) < 1e-12 && T.Strength == 1.25,
			"trends: keys carry their windows (the log's from the Monday the list takes them in) and strengths; heat is 0 at the ends and 1 in the middle");
		// A champion crowned on Saturday, day 26: the list takes it in on Monday, day 28; its window peaks on Tuesday at noon.
		const std::string Key = "champ:" + std::to_string(static_cast<long long>(26.0 * Day + 600.0));
		const double Mid = 29.5 * Day;
		video::Channel C;
		const video::Video Hot = OneUpload(IdeaMoment("idea:7@29:3", Mid - 60.0, "trend=" + Key), Bd, Mid, "trend", {}, &C);
		const video::Video Cold = OneUpload(IdeaMoment("idea:7@29:3", Mid - 60.0, ""), Bd, Mid, "trend");
		const video::Comp* Tc = Hot.X.Comps.empty() ? nullptr : &Hot.X.Comps.front();
		Expect(std::fabs(Hot.X.S0 - Cold.X.S0 - 0.4) < 2e-5 && Tc && Tc->Kind == video::CompKind::Trend && Tc->T == 30.0 * Day && Tc->A == 0.5 && Tc->SrcMix == 6 &&
				   std::fabs(Tc->P - 0.08 * 0.15 / 0.25 * Hot.X.P0) < 0.01 && Hot.P > Cold.P && Cold.X.Comps.empty(),
			"trends: a reaction at its champion's peak: S +0.4, more reach, and a trend part (0.08 x 0.15 / 0.25 x P0) from the next midnight");
		// The same trend's second and third uploads on the channel: the second still rides it, the third gets nothing.
		video::UploadFacts F;
		F.GearQuality = 0.5;
		double Fits[2] = {0.0, 0.0};
		for (int I = 0; I < 2; ++I)
		{
			const video::Candidate M = IdeaMoment("idea:7@29:" + std::to_string(4 + I), Mid - 30.0 + I, "trend=" + Key);
			video::NoteHand(C, M);
			video::Choice Ch = Bd;
			Ch.Candidates = {M.Id};
			const video::Video V = UploadOn(C, Ch, Mid + 1.0 + I, "trend" + std::to_string(I), F);
			Fits[I] = V.X.Comps.empty() || V.X.Comps.front().Kind != video::CompKind::Trend ? 0.0 : V.X.Comps.front().P;
		}
		Expect(Fits[0] > 0.0 && Fits[1] == 0.0, "trends: a trend's third matching upload gets no trend factor");
		const video::Video Few = OneUpload(IdeaMoment("idea:7@29:3", Mid - 60.0, "trend=" + Key + ";few=1"), Bd, Mid, "trend");
		Expect(!Few.X.Comps.empty() && std::fabs(Few.X.Comps.front().P / Few.X.P0 - 0.08 * (1.15 * 1.10 - 1.0) / 0.25) < 1e-3,
			"trends: a topic few creators have covered rides it x1.10");
		// (review) Trend tourists (§4.6.10): a trend video off the channel's niche (a reaction on a channel of coolers) converts
		// its trend viewers at x0.70, x(1 - 0.3 x 0.15 / 1.15) overall at the peak; on the niche (a channel of ideas) in full.
		{
			auto Niche = [&](video::Kind K, const std::string& Facts) {
				video::Channel Ch0 = PlainU3Channel(Mid - 8.0 * Day);
				for (int I = 0; I < 6; ++I)
				{
					const double When = Mid - 8.0 * Day + static_cast<double>(I) * Day;
					video::Candidate M = K == video::Kind::Idea ? IdeaMoment("idea:3@n" + std::to_string(I), When - 30.0, "")
																: Moment("rl:niche@" + std::to_string(I), When - 30.0, K);
					M.Idea = K == video::Kind::Idea ? 3 : M.Idea;
					video::NoteHand(Ch0, M);
					video::Choice Ch = Bd;
					Ch.Candidates = {M.Id};
					UploadOn(Ch0, Ch, When, "niche" + std::to_string(I), F);
				}
				const video::Candidate M = IdeaMoment("idea:7@29:8", Mid - 60.0, Facts);
				video::NoteHand(Ch0, M);
				video::Choice Ch = Bd;
				Ch.Candidates = {M.Id};
				return UploadOn(Ch0, Ch, Mid, "tourist", F).Conv;
			};
			const double Off = Niche(video::Kind::Cooler, "trend=" + Key) / Niche(video::Kind::Cooler, "");
			const double On = Niche(video::Kind::Idea, "trend=" + Key) / Niche(video::Kind::Idea, "");
			Expect(std::fabs(Off - (1.0 - 0.3 * 0.15 / 1.15)) < 1e-4 && std::fabs(On - 1.0) < 1e-4,
				"trends: off the channel's niche the trend's viewers convert at x0.70 (x" + Num(Off, 5) + " overall), on it in full (x" + Num(On, 5) + ")");
		}
		// (review) TrendCool keeps at most 12; past it, the trend that closes first goes: a dozen short trends in a week don't push
		// out a long one's count, so its third matching upload still gets nothing (the Player of the Year race, Nov 15 - Dec 31).
		{
			const int Monday = ss::world::DayOn(2026, 11, 23); // a Monday inside the race
			const double When = static_cast<double>(Monday + 1) * Day + 600.0;
			video::Channel Cp = PlainU3Channel(When);
			int Seq = 0;
			auto Ride = [&](const std::string& Facts) {
				video::Candidate M = IdeaMoment("idea:" + std::to_string(20 + Seq) + "@trim", When - 60.0 + Seq, Facts);
				M.Idea = 20 + Seq; // a different idea each upload (no cooldown in the way)
				video::NoteHand(Cp, M);
				video::Choice Ch = Bd;
				Ch.Candidates = {M.Id};
				const video::Video V = UploadOn(Cp, Ch, When + static_cast<double>(Seq), "trim" + std::to_string(Seq), F);
				++Seq;
				return std::any_of(V.X.Comps.begin(), V.X.Comps.end(), [](const video::Comp& K) { return K.Kind == video::CompKind::Trend; });
			};
			const bool First = Ride("trend=poy:2026"), Second = Ride("trend=poy:2026");
			bool Shorts = true;
			for (int I = 0; I < 12; ++I)
			{
				// Twelve champions crowned the Saturday before: each its own 3-day trend this Monday.
				Shorts = Ride("trend=champ:" + std::to_string(static_cast<long long>(static_cast<double>(Monday - 2) * Day + 60.0 * I))) && Shorts;
			}
			const bool Third = Ride("trend=poy:2026");
			Expect(First && Second && Shorts && !Third && Cp.X.TrendCool.size() <= 12,
				"trends: the race's third matching upload gets nothing even after a dozen other trends (TrendCool " + std::to_string(Cp.X.TrendCool.size()) + ")");
		}
		// A series event's candidate in its series' days.
		const ss::net::SeriesInfo* S = nullptr;
		for (const ss::net::SeriesInfo& X : ss::net::Shared().Series())
		{
			S = S == nullptr && X.LastDay > X.FirstDay ? &X : S;
		}
		bool SeriesOk = S != nullptr;
		if (S)
		{
			const double Ms = (0.5 * static_cast<double>(S->FirstDay + S->LastDay + 1)) * Day;
			const ss::net::SeriesInfo* Running = ss::net::Shared().CurrentSeries(Ms);
			video::Candidate In = Moment("rl:series@1", Ms - 60.0);
			In.Series = true;
			const std::vector<valgo::VaTrend> Ts = valgo::TrendsFor(In, Ms);
			video::Candidate Out = In;
			Out.Series = false;
			// (A day after it when no series runs: nothing to ride.)
			double Quiet = -1.0;
			for (int D = S->LastDay + 1; D < S->LastDay + 120 && Quiet < 0.0; ++D)
			{
				const ss::net::SeriesInfo* X = ss::net::Shared().CurrentSeries(static_cast<double>(D) * Day + 600.0);
				Quiet = X == nullptr || D < X->FirstDay || D > X->LastDay ? static_cast<double>(D) * Day + 600.0 : -1.0;
			}
			SeriesOk = Running && !Ts.empty() && Ts.front().Key == "series:" + Running->Id && Ts.front().Strength >= 1.12 && Ts.front().Strength <= 1.20 &&
				valgo::TrendsFor(Out, Ms).empty() && Quiet > 0.0 && valgo::TrendsFor(In, Quiet).empty();
			// (review) An older series' moment (from before this one's first day) doesn't ride this series' week.
			video::Candidate Older = In;
			Older.At = Running ? static_cast<double>(Running->FirstDay - 2) * Day + 600.0 : Older.At;
			SeriesOk = SeriesOk && valgo::TrendsFor(Older, Ms).empty();
		}
		Expect(SeriesOk, "trends: a series event's moment rides its own series' week (1.12-1.20), nothing outside it, not the next series'");
		// Research's list without the world's log: the calendar's, hottest first, with the player's matching uploads.
		ss::world::World W;
		life::State Ls;
		Ls.Video = C;
		const double Dec = static_cast<double>(ss::world::DayOn(2026, 12, 28)) * Day + 600.0;
		const std::vector<video::Trend> List = video::Trends(Ls, W, Dec);
		bool Keys = List.size() >= 3;
		std::string Shown;
		for (size_t I = 0; I < List.size(); ++I)
		{
			Keys = Keys && (I + 1 >= List.size() || (List[I].Strength - 1.0) * List[I].Heat >= (List[I + 1].Strength - 1.0) * List[I + 1].Heat);
			Shown += (I > 0 ? ", " : "") + List[I].Key + " " + Num(List[I].Heat, 2);
		}
		std::printf("  trends on Dec 28, 2026: %s\n", Shown.c_str());
		Expect(Keys && std::any_of(List.begin(), List.end(), [](const video::Trend& X) { return X.Key == "poy:2026"; }) &&
				   std::any_of(List.begin(), List.end(), [](const video::Trend& X) { return X.Key == "newyear:2026"; }),
			"trends: Research lists the season's trends under the keys an idea names, hottest first");
		// The bell (§6.6.5): a trend that opened since the last midnight, one a day.
		const int Nov15 = ss::world::DayOn(2026, 11, 15);
		video::Channel Quiet = PlainU3Channel(static_cast<double>(Nov15 - 1) * Day);
		life::State Lb;
		Lb.RentsPaid = 2;
		ss::kast::Channel Kb;
		ss::Chips Bb = 0;
		video::Step(Quiet, Lb, Bb, Kb, static_cast<double>(Nov15 - 1) * Day, static_cast<double>(Nov15 + 9) * Day);
		int PoyBells = 0, Sundays = 0, PerDay = 0;
		std::map<double, int> Days;
		for (const auto& B : Quiet.Bell)
		{
			PoyBells += B.second == valgo::BellTrend("Player of the Year race") ? 1 : 0;
			Sundays += B.second == valgo::BellTrend("Sunday majors") ? 1 : 0;
			PerDay = std::max(PerDay, ++Days[B.first]);
		}
		// (Nov 15, 2026 is a Sunday: Monday's bell is the Sunday majors', the race's comes the day after.)
		const int Monday = Nov15 + 1;
		Expect(PoyBells == 1 && Sundays == 2 && PerDay == 1 &&
				   std::any_of(Quiet.Bell.begin(), Quiet.Bell.end(),
					   [Monday](const std::pair<double, std::string>& B) { return B.first == static_cast<double>(Monday) * Day && B.second == valgo::BellTrend("Sunday majors"); }) &&
				   std::any_of(Quiet.Bell.begin(), Quiet.Bell.end(),
					   [Monday](const std::pair<double, std::string>& B) {
						   return B.first == static_cast<double>(Monday + 1) * Day && B.second == valgo::BellTrend("Player of the Year race");
					   }),
			"trends: a new trend rings the bell once, one a day (the Sunday majors on Monday, the Player of the Year race the day after)");
	}
	// Trends from a world's log (§4.6.10): a fresh world lists only the calendar's and the network's; an established one adds
	// its log's champions, final tables and site news, each dated before the week's Monday by a day or more, hot at the minute.
	{
		const auto T0 = std::chrono::steady_clock::now();
		auto Calendar = [](const std::string& Key) {
			for (const char* P : {"sunday:", "series:", "poy:", "hof:", "holiday:", "newyear:"})
			{
				if (Key.rfind(P, 0) == 0)
				{
					return true;
				}
			}
			return false;
		};
		life::State Lw;
		ss::world::World Fresh;
		Fresh.Create(11u, 0.0, ss::world::Start::Blank);
		bool OnlyCalendar = true;
		for (const video::Trend& T : video::Trends(Lw, Fresh, 1.0 * Day + 600.0))
		{
			OnlyCalendar = OnlyCalendar && Calendar(T.Key);
		}
		ss::world::World Old;
		Old.Create(11u, 0.0);
		Old.AdvanceTo(29.0 * Day);
		int FromLog = 0;
		bool Dated = true, Same = true;
		std::vector<std::string> Labels;
		for (int D = 14; D <= 29; ++D)
		{
			const double When = static_cast<double>(D) * Day + 600.0;
			const std::vector<video::Trend> List = video::Trends(Lw, Old, When), Again = video::Trends(Lw, Old, When);
			Same = Same && List.size() == Again.size();
			const double Monday = static_cast<double>(D - D % 7) * Day;
			for (const video::Trend& T : List)
			{
				if (Calendar(T.Key))
				{
					continue;
				}
				++FromLog;
				if (std::find(Labels.begin(), Labels.end(), T.Label) == Labels.end())
				{
					Labels.push_back(T.Label);
				}
				// (The key carries the event's minute; the window starts on the Monday whose list took it in: this one.)
				valgo::VaTrend V;
				const double Event = valgo::TrendOfKey(T.Key, V) ? std::strtod(T.Key.c_str() + T.Key.find(':') + 1, nullptr) : 1e18;
				Dated = Dated && Event < Monday - Day && Event >= Monday - 8.0 * Day && V.Start == Monday && valgo::TrendHeat(V, When) > 0.0;
			}
		}
		std::string Shown;
		for (size_t I = 0; I < Labels.size() && I < 4; ++I)
		{
			Shown += (I > 0 ? "; " : "") + Labels[I];
		}
		std::printf("  trends: a fresh world and an established one's days 14-29: %d trends from the log (%.1f s): %s\n", FromLog,
			std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count(), Shown.c_str());
		// Every label the bell would say passes the lint and the screen.
		std::vector<std::pair<std::string, std::string>> Bells;
		int Screened = 0;
		for (const std::string& L : Labels)
		{
			Bells.push_back({"line", valgo::BellTrend(L)});
			Screened += trademarks::Find(L, trademarks::kSourceScan) || trademarks::Find(L, trademarks::kCopyScan) ? 1 : 0;
		}
		Expect(copylint::Run("rerun_algo trends", Bells, copylint::Pa) == 0 && Screened == 0, "trends: the world's trend labels read as clean bell lines");
		Expect(OnlyCalendar && FromLog > 0 && Dated && Same, "trends: a fresh world lists the calendar's only; the log's trends are dated before their Monday, hot at the minute");
		// (review) Every champion, major final table, rival launch and overlay of a 60-day world is listed in the week after it,
		// at its window's peak, a launch under its site's name (Ignite opens on day 28, a Monday: "Is Ignite soft?"). Each
		// midnight's read of the log is timed too (the 30-day bench below runs past this world's log).
		Old.AdvanceTo(60.0 * Day);
		int LogEvents = 0, Missed = 0;
		bool Ignite = false, Named = true;
		double ReadMs = 0.0;
		for (const ss::world::WorldEvent& E : Old.Events())
		{
			const ss::world::EventKind K = E.Kind;
			const char* Prefix = K == ss::world::EventKind::Champion ? "champ:" : K == ss::world::EventKind::FinalTable ? "final:"
				: K == ss::world::EventKind::SiteLaunched																? "site:"
				: K == ss::world::EventKind::SiteOverlay																? "overlay:"
																														: nullptr;
			const ss::world::Npc* Who = Old.Get(E.Npc);
			const bool Person = K == ss::world::EventKind::Champion || K == ss::world::EventKind::FinalTable;
			if (!Prefix || E.What.empty() || E.At > 52.0 * Day || (Person && (!Who || Who->Name.empty())) ||
				(K == ss::world::EventKind::SiteLaunched && ss::net::SiteOf(E.What + ".") == ss::net::SiteId::RiverLine))
			{
				continue;
			}
			++LogEvents;
			const std::string Key = Prefix + std::to_string(static_cast<long long>(E.At));
			valgo::VaTrend V;
			valgo::TrendOfKey(Key, V);
			const auto R0 = std::chrono::steady_clock::now();
			const std::vector<video::Trend> List = video::Trends(Lw, Old, 0.5 * (V.Start + V.End));
			ReadMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - R0).count();
			const auto It = std::find_if(List.begin(), List.end(), [&Key](const video::Trend& X) { return X.Key == Key; });
			Missed += It == List.end() ? 1 : 0;
			if (It != List.end() && K == ss::world::EventKind::SiteLaunched)
			{
				Ignite = Ignite || It->Label == "Is Ignite soft?";
				Named = Named && It->Label == "Is " + ss::net::Def(ss::net::SiteOf(E.What + ".")).Name + " soft?";
			}
		}
		std::printf("  trends: %d champions, final tables, launches and overlays in a 60-day world's log, %d never listed; a read %.3f ms\n", LogEvents, Missed,
			LogEvents > 0 ? ReadMs / LogEvents : 0.0);
		Expect(LogEvents > 10 && Missed == 0 && Ignite && Named && ReadMs / std::max(1, LogEvents) < 0.05,
			"trends: every champion, final table, launch and overlay is listed the week after it, a launch under its site's name (" + std::to_string(Missed) + " of " +
				std::to_string(LogEvents) + " missed)");
		// The cost (§4.7: a 30-day skip within 3 ms with the sites' steps): a three-a-week creator's 60 kept videos, the world's
		// trends read at each midnight.
		Career Cr = NewCareer();
		for (int D = 0; D < 200; ++D)
		{
			CareerDay(Cr, D, "bench", ThreeAWeek, false, 3.0, true);
		}
		double Best = 1e9, BestNull = 1e9;
		for (int Rep = 0; Rep < 10; ++Rep)
		{
			video::Channel C = Cr.C;
			life::State Lb = Cr.L;
			ss::Chips Bk = 0;
			const auto B0 = std::chrono::steady_clock::now();
			video::Step(C, Lb, Bk, Cr.K, 200.0 * Day, 230.0 * Day, Rep % 2 == 0 ? &Old : nullptr);
			(Rep % 2 == 0 ? Best : BestNull) = std::min(Rep % 2 == 0 ? Best : BestNull, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - B0).count());
		}
		std::printf("  bench: a 30-day Rerun 2.0 step over %zu videos: %.3f ms with the world's trends, %.3f ms without (best of 5)\n", Cr.C.Videos.size(), Best, BestNull);
		Expect(Best < 30.0, "bench: a 30-day Rerun 2.0 step stays in milliseconds with the world's trends (" + Num(Best, 3) + " ms)");
	}
	// A repackage skips the next check-in ("Rerun needs a day to learn a new look"): never a day-7 part after a day-5 change.
	{
		int Twins = 0, Repacked = 0;
		for (int S = 0; S < 20; ++S)
		{
			video::Channel A;
			OneUpload(Moment("rl:learn@30:" + std::to_string(S), At - 60.0), Bd, At, "learn" + std::to_string(S), {}, &A);
			video::Channel B = A;
			life::State La;
			ss::kast::Channel Ka;
			ss::Chips Ba = 0;
			video::Step(A, La, Ba, Ka, At, At + 5.0 * Day);
			video::Step(B, La, Ba, Ka, At, At + 5.0 * Day);
			B.Videos.back().X.LastRepackDay = valgo::DayOf(At + 5.0 * Day);
			video::Step(A, La, Ba, Ka, At + 5.0 * Day, At + 9.0 * Day);
			video::Step(B, La, Ba, Ka, At + 5.0 * Day, At + 9.0 * Day);
			auto Has7 = [](const video::Video& V) { return std::any_of(V.X.Comps.begin(), V.X.Comps.end(), [](const video::Comp& K) { return K.Kind == video::CompKind::Check7; }); };
			Twins += Has7(A.Videos.back()) ? 1 : 0;
			Repacked += Has7(B.Videos.back()) ? 1 : 0;
		}
		Expect(Repacked == 0 && Twins >= 6, "decisions: a repackage skips the next check-in (" + std::to_string(Twins) + " of 20 unchanged videos took a day-7 part, none repackaged)");
	}
	// Realtime's note (SPEC_CREATORS §3.2): a narrated viral roll "is breaking out" for two days from its verdict midnight.
	{
		video::Channel C;
		OneUpload(Moment("rl:viral@30:1", At - 60.0), Bd, At, "viral", {}, &C);
		C.Videos.back().X.Viral = true;
		life::State Lv;
		ss::kast::Channel Kv;
		ss::Chips Bv = 0;
		const double Verdict = static_cast<double>(valgo::VerdictDayOf(C.Videos.back())) * Day;
		video::Step(C, Lv, Bv, Kv, At, Verdict + Day);
		const std::string Title = C.Videos.back().Title;
		const video::RealtimeView Now = video::Realtime(C, Verdict + 60.0), Before = video::Realtime(C, Verdict - 60.0), After = video::Realtime(C, Verdict + 2.0 * Day + 60.0);
		Expect(!Now.Notes.empty() && Now.Notes.front() == valgo::BellViral(Title) && Before.Notes.empty() && After.Notes.empty(),
			"realtime: a narrated viral roll's note reads from its verdict midnight for two days");
	}
}

// ================================================================== the carry and the waves, exactly (review-core-videoalgo)

/** The kept video with this id (a copy; an empty one when it's gone). */
video::Video FindVideo(const video::Channel& C, const std::string& Id)
{
	for (const video::Video& V : C.Videos)
	{
		if (V.Id == Id)
		{
			return V;
		}
	}
	return video::Video();
}

/** Binge components a video got at one minute, and their views. */
int BingeAt(const video::Video& V, double T, double& P)
{
	int N = 0;
	for (const video::Comp& K : V.X.Comps)
	{
		if (K.Kind == video::CompKind::Binge && K.T == T)
		{
			++N;
			P = K.P;
		}
	}
	return N;
}

void CarryAndWaves()
{
	life::State L;
	L.RentsPaid = 2;
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	video::UploadFacts F;
	F.GearQuality = 0.5;
	// Dormancy and welcome back (SPEC_CREATORS §3.3): after 21 idle days H drifts 5% a day toward 0; the first upload after 21
	// or more dark days adds 0.2, once.
	{
		const int Last = 40;
		const double At = static_cast<double>(Last) * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		C.X.MomH = 1.0;
		C.X.Momentum = valgo::MomCOf(1.0);
		C.X.LastUploadDay = Last;
		video::Step(C, L, Bank, K, At, At + 30.0 * Day); // the midnights Last + 1 .. Last + 30
		double H = 1.0;
		for (int Next = Last + 1; Next <= Last + 30; ++Next)
		{
			H = Next - Last > 21 ? valgo::VaRound(H * 0.95, 1e6) : H;
		}
		Expect(C.X.MomH == H && C.X.Momentum == valgo::MomCOf(H) && H < 1.0, "dormancy: after 21 idle days H drifts 5% a day toward 0 (" + Num(C.X.MomH, 7) + ")");
		const double Back = At + 30.0 * Day;
		const video::Candidate M = Moment("rl:back@70:1", Back - 60.0);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		const video::Video V = UploadOn(C, Ch, Back, "back", F);
		const double Hb = valgo::VaRound(H + 0.2, 1e6);
		const bool First = C.X.WelcomeBack && C.X.MomH == Hb && C.X.Momentum == valgo::MomCOf(Hb) && V.X.SettledS == static_cast<float>(valgo::VaRound(valgo::MomCOf(Hb), 1e4));
		const video::Candidate M2 = Moment("rl:back@71:1", Back + Day - 60.0);
		video::NoteHand(C, M2);
		Ch.Candidates = {M2.Id};
		UploadOn(C, Ch, Back + Day, "back", F);
		Expect(First && !C.X.WelcomeBack && C.X.MomH == Hb, "welcome back: the first upload after 21 dark days adds 0.2 to H (its reach reads it), the next one nothing");
	}
	// Momentum from realized performance and the honest label (§3.3): at the verdict H <- 0.6 H + 0.4 (perf + ln Compress(e^(S0 -
	// S0_base), 0.30)) and the meter's average D <- 0.5 D + 0.5 MomC; the label reads perf + 0.5 (S0 - S0_base), so the launch
	// actions (+0.19 here, enough to cross 0.6 if the label read them) push the kicks and never the label.
	{
		const double At = 44.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At);
		C.X.MomH = 0.5;
		const video::Candidate M = Moment("rl:mom@44:1", At - 60.0);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		UploadOn(C, Ch, At, "mom", F);
		video::Video& V = C.Videos.back();
		V.X.LuckLn = 0.55f;
		V.X.Viral = false;
		V.X.Launch = static_cast<uint8_t>((1u << static_cast<int>(video::Launch::Kast)) | (1u << static_cast<int>(video::Launch::Teaser)) | (1u << static_cast<int>(video::Launch::Reply)));
		const double S0 = V.X.S0, Base = V.X.S0Base, Shown = C.X.MomShown;
		video::Step(C, L, Bank, K, At, At + 3.0 * Day);
		const video::Video& W = C.Videos.back();
		const bool Broke = std::any_of(W.X.Comps.begin(), W.X.Comps.end(), [](const video::Comp& Cp) { return Cp.Kind == video::CompKind::Breakout; });
		const double Sx = S0 - Base, Perf = static_cast<double>(0.55f);
		const double Want = valgo::VaRound(0.6 * 0.5 + 0.4 * (Perf + Sx / (1.0 + std::fabs(Sx) / 0.30)), 1e6);
		Expect(!Broke && C.X.MomH == Want && C.X.Momentum == valgo::MomCOf(Want) && C.X.MomShown == valgo::VaRound(0.5 * Shown + 0.5 * valgo::MomCOf(Want), 1e6) &&
				   C.X.SettledN == 1 && C.X.Settled.back() == 0.55f,
			"momentum: the verdict moves H by the video's own luck and its standing score, and the meter's average halfway to MomC (H " + Num(C.X.MomH, 7) + ")");
		Expect(S0 == Base && W.X.Verdict == algo::VerdictLabel(Perf, S0, Base) && W.X.Verdict == 2,
			"labels: perf 0.55 with every launch action reads Pushed: the launch actions never move the label (" + std::to_string(static_cast<int>(W.X.Verdict)) + ")");
	}
	// The Binge wave (§3.3): a hit at 3x the typical P or more sends 4% of its excess to each of the three newest kept Update 3
	// uploads before it, at its verdict midnight. A legacy video is skipped and takes no slot (V6); a full video's slot moves
	// nothing (the 10-slot rule); the bell counts only the videos that picked up a part.
	{
		const double At0 = 50.0 * Day + 4.0 * 60.0;
		video::Channel C = PlainU3Channel(At0);
		std::vector<std::string> Ids;
		for (int I = 0; I < 6; ++I)
		{
			const double At = At0 + static_cast<double>(I) * Day;
			if (I == 4)
			{
				video::Video Old;
				Old.Id = "legacy-bw";
				Old.At = At - 12.0 * 60.0;
				Old.P = 100.0;
				Old.A = 0.75;
				Old.Tau1 = 1.5;
				Old.Tau2 = 12.0;
				Old.Title = "old";
				C.Videos.push_back(Old);
			}
			const video::Candidate M = Moment("rl:bw@" + std::to_string(50 + I) + ":1", At - 60.0);
			video::NoteHand(C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			Ids.push_back(video::Upload(C, Ch, At, "bw", F));
		}
		// Every decision of the six made (day 14 and the breakout's window past); then each P reads 100, the typical.
		video::Step(C, L, Bank, K, At0, At0 + 22.0 * Day);
		for (video::Video& V : C.Videos)
		{
			V.P = 100.0;
			if (V.Id == Ids[4])
			{
				while (V.X.Comps.size() < valgo::VaMaxComps)
				{
					video::Comp Cp;
					Cp.T = V.At + 3.0 * Day;
					Cp.P = 1.0;
					Cp.A = 0.5;
					Cp.Kind = video::CompKind::Repackage;
					V.X.Comps.push_back(Cp); // full: its slot is skipped
				}
			}
		}
		const double Ah = At0 + 22.0 * Day;
		const video::Candidate M = Moment("rl:bw@72:1", Ah - 60.0);
		video::NoteHand(C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = video::Format::Breakdown;
		const std::string Hit = video::Upload(C, Ch, Ah, "bw", F);
		C.Videos.back().P = 1000.0;
		const double Vt = static_cast<double>(valgo::VerdictDayOf(C.Videos.back())) * Day;
		video::Step(C, L, Bank, K, Ah, Ah + 3.0 * Day);
		int Got[6] = {0, 0, 0, 0, 0, 0};
		double P6 = 0.0, P4 = 0.0, Unused = 0.0;
		Got[5] = BingeAt(FindVideo(C, Ids[5]), Vt, P6);
		Got[4] = BingeAt(FindVideo(C, Ids[4]), Vt, Unused);
		Got[3] = BingeAt(FindVideo(C, Ids[3]), Vt, P4);
		for (int I = 0; I < 3; ++I)
		{
			Got[I] = BingeAt(FindVideo(C, Ids[static_cast<size_t>(I)]), Vt, Unused);
		}
		const bool Legacy = FindVideo(C, "legacy-bw").X.Comps.empty();
		bool Bell = false;
		for (const auto& B : C.Bell)
		{
			Bell = Bell || (B.first == Vt && B.second == valgo::BellBinge(2));
		}
		Expect(!Hit.empty() && Got[5] == 1 && Got[4] == 0 && Got[3] == 1 && Got[2] + Got[1] + Got[0] == 0 && P6 == 36.0 && P4 == 36.0 && Legacy,
			"binge: 4% of the excess (36 of 900) to the newest two that fit, the legacy video skipped, the full one's slot spent on nothing");
		Expect(Bell, "binge: the bell counts the videos that really picked up a part (two)");
	}
	// Kast Heat from Rerun (§3.3), one story one bump: a viral video whose verdict broke out too is +10 once; "Pushed hard" with a
	// breakout the same midnight is +10, not 16; a breakout on day 7 after a "Pushed hard" verdict is a second story; nothing at or
	// before Since.
	{
		video::Channel C = PlainU3Channel(100.0 * Day);
		auto Make = [&C](const std::string& Id, int Day0, bool Viral, int Label, int BreakDay) {
			video::Video V;
			V.Id = Id;
			V.At = static_cast<double>(Day0) * Day + 4.0 * 60.0;
			V.Title = Id;
			V.X.On = true;
			V.X.Viral = Viral;
			V.X.Verdict = static_cast<int8_t>(Label);
			if (BreakDay >= 0)
			{
				video::Comp Cp;
				Cp.T = static_cast<double>(BreakDay) * Day;
				Cp.P = 500.0;
				Cp.A = 0.3;
				Cp.Kind = video::CompKind::Breakout;
				V.X.Comps.push_back(Cp);
			}
			C.Videos.push_back(V);
		};
		Make("early", 60, false, 4, 66); // its verdict (day 63) and breakout (66) before Since
		Make("hard-then-break", 70, false, 4, 77);
		Make("viral-broke", 80, true, 4, 83);
		Make("hard-broke", 85, false, 4, 88);
		const auto Bumps = algo::KastHeatSince(C, 72.0 * Day);
		const std::vector<std::pair<double, double>> Want = {{73.0 * Day, 6.0}, {77.0 * Day, 10.0}, {83.0 * Day, 10.0}, {88.0 * Day, 10.0}};
		Expect(Bumps == Want, "kast heat: one bump a story (a viral breakout +10 once, Pushed hard with its breakout +10), a later breakout its own, nothing before Since (" +
								  std::to_string(Bumps.size()) + " bumps)");
	}
}

// ================================================================== the save and its budgets

void Saves()
{
	// vid's block carries LuckLn and the viral bit; vmom the creator update's Momentum, the streak and Fresh's count.
	{
		Career Cr = NewCareer();
		for (int D = 0; D < 120; ++D)
		{
			CareerDay(Cr, D, "save", ThreeAWeek, false, 3.0, true);
		}
		const std::string Text = CareerText(Cr);
		const life::State Back = Reload(Text);
		bool Luck = !Back.Video.Videos.empty();
		for (size_t I = 0; I < Back.Video.Videos.size() && I < Cr.C.Videos.size(); ++I)
		{
			Luck = Luck && Back.Video.Videos[I].X.LuckLn == Cr.C.Videos[I].X.LuckLn && Back.Video.Videos[I].X.Viral == Cr.C.Videos[I].X.Viral;
		}
		const video::ChannelX& X = Back.Video.X;
		Expect(Luck && X.MomH == Cr.C.X.MomH && X.MomShown == Cr.C.X.MomShown && X.StreakW == Cr.C.X.StreakW && X.UploadsEver == Cr.C.X.UploadsEver && X.RainChecks == Cr.C.X.RainChecks,
			"save: LuckLn, the viral bit, H, the display average, the streak and Fresh's count round-trip");
		Expect(SaveText(Back) == Text, "save: Rerun 2.0's state writes back byte for byte");
		// The packed components (vid's last field) and the rings (a line a run of days) read back field for field.
		bool Comps = Back.Video.Videos.size() == Cr.C.Videos.size();
		size_t Count = 0, Fixed = 0;
		for (size_t I = 0; Comps && I < Cr.C.Videos.size(); ++I)
		{
			const video::Video &A = Cr.C.Videos[I], &B = Back.Video.Videos[I];
			Comps = Comps && A.X.Comps.size() == B.X.Comps.size() && A.X.S0Base == B.X.S0Base;
			for (size_t K = 0; Comps && K < A.X.Comps.size(); ++K)
			{
				const video::Comp &P = A.X.Comps[K], &Q = B.X.Comps[K];
				Comps = P.T == Q.T && P.P == Q.P && P.A == Q.A && P.Kind == Q.Kind && P.SrcMix == Q.SrcMix && P.CtrF == Q.CtrF && P.SrcMix == valgo::MixFor(P.Kind);
				double Own = 0.0;
				Fixed += valgo::KindA(A, P.Kind, Own) && Own == P.A ? 1 : 0;
				++Count;
			}
		}
		bool Rings = X.Days.size() == Cr.C.X.Days.size() && X.Months.size() == Cr.C.X.Months.size();
		for (size_t I = 0; Rings && I < X.Days.size(); ++I)
		{
			Rings = X.Days[I].Key == Cr.C.X.Days[I].Key && X.Days[I].Views == Cr.C.X.Days[I].Views && X.Days[I].Cents == Cr.C.X.Days[I].Cents &&
				X.Days[I].Comments == Cr.C.X.Days[I].Comments;
		}
		size_t DayLines = 0;
		for (const std::vector<std::string>& P : SplitLines(Text))
		{
			DayLines += P.size() > 1 && P[1] == "vday" ? 1 : 0;
		}
		Expect(Comps && Count > 20 && Fixed * 2 > Count && Rings && DayLines == 1 && Text.find("\tvcomp\t") == std::string::npos,
			"save: every component reads back field for field from vid's packed field (" + std::to_string(Count) + ", " + std::to_string(Fixed) +
				" with their kind's own A), the 90-day ring from one line");
		// A vcomp line (the format before the packing) still reads.
		std::string Old = Text;
		const size_t Vid = Old.find("video\tvid\t");
		const size_t End = Old.find('\n', Vid);
		const size_t Tab = Old.rfind('\t', End);
		const std::string Packed = Old.substr(Tab + 1, End - Tab - 1);
		std::string Line = Old.substr(Vid, End - Vid);
		const std::string Id = Line.substr(10, Line.find('\t', 10) - 10);
		Old.erase(Tab, End - Tab); // the 64-field block
		std::string Lines;
		const life::State First = Reload(Text);
		for (const video::Comp& Cp : First.Video.Videos.front().X.Comps)
		{
			char Buf[160];
			std::snprintf(Buf, sizeof(Buf), "video\tvcomp\t%s\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.9g\n", Id.c_str(), Cp.T, Cp.P, Cp.A, static_cast<int>(Cp.Kind), static_cast<int>(Cp.SrcMix),
				static_cast<double>(Cp.CtrF));
			Lines += Buf;
		}
		Old += Lines;
		const life::State Legacy = Reload(Old);
		bool Same = !Packed.empty() && Legacy.Video.Videos.front().X.Comps.size() == First.Video.Videos.front().X.Comps.size();
		for (size_t K = 0; Same && K < First.Video.Videos.front().X.Comps.size(); ++K)
		{
			Same = Legacy.Video.Videos.front().X.Comps[K].P == First.Video.Videos.front().X.Comps[K].P;
		}
		Expect(Same && SaveText(Legacy) == Text, "save: a 64-field vid block with vcomp lines reads as the packed one");
	}
	// The budgets (§7.4): the heavy creator <= 22 KB of Update 3 lines; the true worst case (a daily Strategy uploader for two
	// years, every slot of every kept video filled) <= 80 KB.
	auto U3Bytes = [](const std::string& Text) {
		size_t Bytes = 0;
		std::map<std::string, size_t> ByTag;
		for (const std::vector<std::string>& P : SplitLines(Text))
		{
			std::string Line;
			for (size_t I = 0; I < P.size(); ++I)
			{
				Line += (I > 0 ? "\t" : "") + P[I];
			}
			if (P.size() > 1 && P[1].size() > 1 && P[1][0] == 'v' && P[1] != "vid")
			{
				Bytes += Line.size() + 1;
				ByTag[P[1]] += Line.size() + 1;
			}
			else if (P.size() > 29 && P[1] == "vid" && P[29] == "3")
			{
				std::string Tail;
				for (size_t I = 29; I < P.size(); ++I)
				{
					Tail += "\t" + P[I];
				}
				Bytes += Tail.size();
				ByTag["vid+"] += Tail.size();
			}
		}
		std::string Parts;
		for (const auto& Kv : ByTag)
		{
			Parts += " " + Kv.first + " " + std::to_string(Kv.second);
		}
		std::printf("   (by line:%s)\n", Parts.c_str());
		return Bytes;
	};
	{
		// update3_creator's shape (§7.4): three uploads a week for 420 days, every third a Teaching Breakdown, 5 repackages, 3 Duels,
		// 2 challenges (core-videocontent's lines, here by hand).
		Career Cr = NewCareer();
		int Uploads = 0;
		for (int D = 0; D < 420; ++D)
		{
			const double At = static_cast<double>(D) * Day + 3.0 * 60.0;
			if (std::find(ThreeAWeek.begin(), ThreeAWeek.end(), D % 7) != ThreeAWeek.end())
			{
				video::Choice Ch;
				const int Slot = Uploads % 3;
				Ch.F = Slot == 0 ? video::Format::Breakdown : Slot == 1 ? video::Format::Short : video::Format::Vlog;
				Ch.T = Slot == 0 ? video::TitleStyle::Teaching : video::TitleStyle::Story;
				if (Uploads == 150 || Uploads == 160 || Uploads == 170)
				{
					for (int K = 0; K < 3; ++K)
					{
						video::ThumbSpec S;
						S.Arch = static_cast<int16_t>(K + 2);
						S.Layout = static_cast<int16_t>(K);
						S.Caption = K == 0 ? "82%" : "ONE CARD";
						Ch.Thumbs.push_back(S);
					}
				}
				for (int I = 0; I < (Ch.F == video::Format::Vlog ? 3 : 1); ++I)
				{
					const std::string Id = "rl:heavy@" + std::to_string(D) + ":" + std::to_string(I);
					video::NoteHand(Cr.C, Moment(Id, At, Slot == 0 ? video::Kind::Bluff : video::Kind::Cooler));
					Ch.Candidates.push_back(Id);
				}
				video::UploadFacts F;
				F.GearQuality = 0.5;
				video::Upload(Cr.C, Ch, At + 30.0, "heavy", F);
				++Uploads;
			}
			// Five repackages, as Session::Repackage makes them: the rescale component at the minute, the next check-in skipped.
			for (video::Video& V : Cr.C.Videos)
			{
				const int Age = valgo::DayOf(static_cast<double>(D) * Day) - valgo::DayOf(V.At);
				const bool Pick = V.X.On && V.X.Repacks == 0 && Age == 5 && (Uploads == 141 || Uploads == 146 || Uploads == 156 || Uploads == 166 || Uploads == 176);
				if (Pick)
				{
					const video::Comp Rp = algo::RepackComp(V, static_cast<double>(D) * Day + 12.0 * 60.0, V.X.Ctr * 1.1);
					V.X.Comps.push_back(Rp);
					V.X.Ctr *= 1.1;
					V.X.Repacks = 1;
					V.X.LastRepackDay = D;
				}
			}
			video::Step(Cr.C, Cr.L, Cr.Bank, Cr.K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		}
		Cr.C.X.Challenges.push_back({1, 2, 1, 2000, 100000, 35.0 * Day, 1, 3});
		Cr.C.X.Challenges.push_back({2, 1, 0, 50000, 250000, 300.0 * Day, 0, 7});
		const size_t Bytes = U3Bytes(CareerText(Cr));
		std::printf("  save: the heavy creator (update3_creator's shape): %zu bytes of Update 3 lines\n", Bytes);
		// §7.4's 22 KB (I9). The model makes about 3 components a video (the Binge wave, a lesson's day-30 and day-90 check-ins,
		// Late Bloomer): they ride on vid packed (no vcomp lines), the rings are a line a run of days, s0Base is empty when it is s0.
		Expect(Bytes <= 22 * 1024, "save: the heavy creator's Update 3 lines within 22 KB (" + std::to_string(Bytes) + ")");
	}
	{
		Career Cr = NewCareer();
		for (int D = 0; D < 730; ++D)
		{
			const double At = static_cast<double>(D) * Day + (3.0 + static_cast<double>(D % 7)) * 60.0;
			video::Candidate M = Moment("rl:worst@" + std::to_string(D) + ":1", At - 30.0, video::Kind::Bluff);
			video::NoteHand(Cr.C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			Ch.T = video::TitleStyle::Teaching;
			Ch.NextUp = Cr.C.Videos.empty() ? std::string() : Cr.C.Videos.back().Id;
			video::UploadFacts F;
			F.GearQuality = 0.5;
			video::Upload(Cr.C, Ch, At, "worst", F);
			video::Step(Cr.C, Cr.L, Cr.Bank, Cr.K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		}
		// Fill every slot of every kept video (the true worst case), then measure.
		for (video::Video& V : Cr.C.Videos)
		{
			while (V.X.Comps.size() < valgo::VaMaxComps)
			{
				video::Comp Cp;
				Cp.T = V.At + 3.123456789 * Day;
				Cp.P = 1234.56789012345;
				Cp.A = 0.456789012345;
				Cp.Kind = video::CompKind::Repackage;
				Cp.CtrF = 0.8765432f;
				V.X.Comps.push_back(Cp);
			}
		}
		const size_t Bytes = U3Bytes(CareerText(Cr));
		std::printf("  save: the worst case (a daily Strategy uploader for two years, every slot filled): %zu bytes of Update 3 lines\n", Bytes);
		Expect(Bytes <= 80 * 1024, "save: the worst case within 80 KB (" + std::to_string(Bytes) + ")");
	}
}

// ================================================================== the analytics' consistency (§5)

void Analytics()
{
	Career Cr = NewCareer();
	Cr.C.PartnerAt = 0.0;
	for (int D = 0; D < 130; ++D)
	{
		CareerDay(Cr, D, "stats", ThreeAWeek, false, 19.0, true);
	}
	const double World = 130.0 * Day + 13.0 * 60.0;
	int Ok = 0, Videos = 0, Shares = 0, Curves = 0;
	double Earned = 0.0;
	for (const video::Video& V : Cr.C.Videos)
	{
		const video::VideoStats S = video::StatsOf(Cr.C, V, World);
		++Videos;
		double SrcViews = 0.0, Bearing = 0.0;
		for (size_t I = 0; I < 10; ++I)
		{
			SrcViews += S.Src.Views[I];
			Bearing += valgo::SourceRel(static_cast<int>(I)) > 0.0 ? S.Src.Views[I] : 0.0;
		}
		Ok += S.Likes <= S.Views && S.Dislikes <= 0.5 * S.Likes && S.Comments <= S.Views && S.SubsLost >= 0.0 && std::fabs(S.Impressions * S.CtrLife - Bearing) <= 1.0 ? 1 : 0;
		Shares += std::fabs(SrcViews - S.Views) <= 0.5 ? 1 : 0;
		double Mean = 0.0;
		for (int I = 0; I < 60; ++I)
		{
			Mean += 0.5 * (static_cast<double>(S.Retention[static_cast<size_t>(I)]) + static_cast<double>(S.Retention[static_cast<size_t>(I + 1)])) / 60.0;
		}
		Curves += std::fabs(Mean - S.AvgPct) < 2e-3 && S.Retention[0] == 1.0f ? 1 : 0;
		Earned += V.X.Earned;
	}
	Expect(Videos > 0 && Ok == Videos, "analytics: likes under views, dislikes under half the likes, impressions x CTR = the impression-bearing views (" + std::to_string(Ok) + " of " +
		std::to_string(Videos) + ")");
	Expect(Shares == Videos, "analytics: the traffic sources sum to the views");
	Expect(Curves == Videos, "analytics: each retention curve starts at 100% and averages the video's retention (" + std::to_string(Curves) + " of " + std::to_string(Videos) + ")");
	// The rings against the closed form: the day ring's views sum to the channel's views over the same days (+-0.5 a day).
	const video::Range R = video::StatsRange(Cr.C, World, 28);
	double Ring = 0.0, Closed = 0.0;
	for (int I = 0; I < 27; ++I)
	{
		Ring += R.Views[static_cast<size_t>(I)];
	}
	const double From = (std::floor(World / Day) - 27.0) * Day, To = std::floor(World / Day) * Day;
	// (The closed form over the same 27 whole days: TotalViews' difference, folded tails included.)
	Closed = video::TotalViews(Cr.C, To) - video::TotalViews(Cr.C, From);
	Expect(R.Days == 28 && R.Views.size() == 28 && std::fabs(Ring - Closed) <= 0.5 * 27.0 + 1e-6 * Closed,
		"analytics: the day ring's views equal the closed form's over 27 days (" + Num(Ring, 9) + " against " + Num(Closed, 9) + ")");
	// The per-video double cents add up to the money (no folded video yet on this channel: every cent is a kept video's).
	const double Money = static_cast<double>(Cr.C.BalanceCents + Cr.C.PaidCents);
	Expect(Cr.C.Videos.size() < 60 && std::fabs(Earned - Money) <= 0.5 * 130.0, "analytics: the videos' own cents sum to the channel's money (" + Num(Earned, 9) + " against " +
		Num(Money, 9) + ")");
	const video::Health H = video::HealthOf(Cr.C, World);
	Expect(H.Momentum >= 0 && H.Momentum <= 5 && H.Energy >= 0 && H.Energy <= 5 && H.Consistency == 5, "analytics: Channel health's meters in 0-5 bars, a three-a-week streak at 5");
	const video::RealtimeView Rt = video::Realtime(Cr.C, World);
	double Minutes = 0.0;
	for (const double X : Rt.Minutes)
	{
		Minutes += X;
	}
	Expect(std::fabs(Minutes - Rt.Hours[47]) < 1e-6 * std::max(1.0, Rt.Hours[47]) && Rt.Top.size() <= 3, "analytics: the minutes' jitter never changes the last hour's total");
	const video::AudienceView A = video::AudienceOf(Cr.C, World);
	double Ages = 0.0, Devices = 0.0;
	for (const double X : A.Ages)
	{
		Ages += X;
	}
	for (const double X : A.Devices)
	{
		Devices += X;
	}
	Expect(std::fabs(Ages - 1.0) < 1e-9 && std::fabs(Devices - 1.0) < 1e-9 && algo::PeakHour(Cr.C, 6) == 21.0, "analytics: the audience's ages and devices sum to 1; the peak is 9 PM");
	const std::vector<video::AdMarketRow> Market = video::AdMarket(Cr.C, 2.0 * Day);
	double Mix = 0.0;
	for (const video::AdMarketRow& Row : Market)
	{
		Mix += Row.Mix;
	}
	Expect(Market.size() == 8 && std::fabs(Market[0].Cpm - valgo::VaBaseCpm * 1.25 * algo::Season(2)) < 1e-12 && Market[0].Cpm > 10.4 && std::fabs(Mix - 1.0) < 1e-9,
		"analytics: the Ad market card's CPMs by category and season (setup and gear $" + Num(Market.empty() ? 0.0 : Market[0].Cpm, 4) + " in October), the mix summing to 1");
	// The copy: every line linted, no real name, nothing in-world says AI or autoplay.
	std::vector<std::pair<std::string, std::string>> Lines, Letters;
	int Banned = 0;
	for (const std::string& L : video::InventoryAlgo())
	{
		const size_t Tab = L.find('\t');
		const std::string Kind = L.substr(0, Tab), Text = L.substr(Tab + 1);
		if (Kind == "line" || Kind == "subject")
		{
			Lines.push_back({Kind, Text});
		}
		if (Kind == "body")
		{
			Letters.push_back({Kind, Text});
		}
		const std::string Low = trademarks::Lower(Text);
		Banned += trademarks::Find(Text, trademarks::kSourceScan) || trademarks::Find(Text, trademarks::kCopyScan) || trademarks::HasWord(Low, "ai") || trademarks::HasWord(Low, "bot") ||
				trademarks::HasWord(Low, "bots") || Low.find("autoplay") != std::string::npos || Low.find("autopilot") != std::string::npos
			? 1
			: 0;
	}
	const int Problems = copylint::Run("rerun_algo", Lines, copylint::Pa) + copylint::Run("rerun_algo", Letters, copylint::LetterBody);
	Expect(Lines.size() > 40 && Problems == 0 && Banned == 0, "copy: Rerun 2.0's core lines pass the lint, the trademark screen and U1 (" + std::to_string(Lines.size()) + " lines)");
}

// ================================================================== the goldens (Tests/video_vectors_u3.txt)

/**
 * video_vectors.txt's five uploads (a bad beat at 81%, five formats and styles) through Rerun 2.0's uploader with the levers
 * that need nothing of core-videocontent's library (Fresh, the 9 PM window, Heavy after the last check-in, a teaser and a
 * reply, a Live Drop), stepped 120 days; then a three-a-week creator on Rerun 2.0 for 400 days. The recipe, template and
 * Duel lines join at C3, once core-videocontent's library lands (update 3 SPEC §7.7). Written LF by "--write", read CR-stripped.
 */
std::string GoldensU3()
{
	std::string Out;
	for (int S = 1; S <= 5; ++S)
	{
		Career Cr = NewCareer();
		Cr.C.PartnerAt = 0.0;
		const double At = 12.0 * Day + 20.0 * 60.0 + 40.0;
		video::Candidate M = Moment("rl:golden@12:1", At - 60.0, video::Kind::BadBeat);
		M.Stream.Viewers = 4;
		M.EquityAtAllIn = 0.81;
		M.Rec.Mine = ss::ParseCards("AsAd");
		M.Rec.Shown = {{"Mike_R", ss::ParseCards("7c7h")}};
		M.Rec.Board = ss::ParseCards("Kd8s2c9h7d");
		M.Opponent = "Mike_R";
		video::NoteHand(Cr.C, M);
		video::Choice Ch;
		Ch.Candidates = {M.Id};
		Ch.F = static_cast<video::Format>(S % 4) == video::Format::Vlog || static_cast<video::Format>(S % 4) == video::Format::Recap ? video::Format::Breakdown
																															  : static_cast<video::Format>(S % 4);
		Ch.T = static_cast<video::TitleStyle>(S % 5);
		Ch.Fr = video::Frame::Reaction;
		Ch.Len = static_cast<video::LengthOpt>(S % 3);
		video::UploadFacts F;
		F.GearQuality = 0.5;
		F.KastFollowers = 4;
		const std::string Id = video::Upload(Cr.C, Ch, At, "golden3:" + std::to_string(S), F);
		video::Video& V = Cr.C.Videos.back();
		if (S == 2)
		{
			V.X.Launch = static_cast<uint8_t>((1u << static_cast<int>(video::Launch::Teaser)) | (1u << static_cast<int>(video::Launch::Reply)));
		}
		if (S == 3)
		{
			algo::NoteStream(Cr.C, At + 30.0, At + 30.0 + 150.0, 40, 28.0, 20.0);
		}
		Out += "upload3\t" + std::to_string(S) + "\t" + Id + "\tP=" + Num(V.P, 9) + "\tP0=" + Num(V.X.P0, 9) + "\tctr=" + Num(V.X.Ctr, 9) + "\tnoise=" + Num(V.X.CtrNoise, 9) +
			"\tS0=" + Num(V.X.S0, 9) + "\tbase=" + Num(V.X.S0Base, 9) + "\tluck=" + Num(static_cast<double>(V.X.LuckLn), 9) + "\tr=" + Num(V.Retention, 9) + "\tc=" + Num(V.Conv, 9) +
			"\trpm=" + Num(V.Rpm, 9) + "\tlen=" + Num(V.LengthMin, 9) + "\ttitle=" + V.Title + "\n";
		for (int D = 12; D < 132; ++D)
		{
			if (S == 4 && D == 28)
			{
				video::Video& W = Cr.C.Videos.back();
				W.X.Ads = video::AdLoad::Heavy;
				W.X.AdsChangedDay = D + 1;
			}
			video::Step(Cr.C, Cr.L, Cr.Bank, Cr.K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		}
		const video::Video& W = Cr.C.Videos.back();
		Out += "decided3\t" + std::to_string(S) + "\tverdict=" + std::to_string(static_cast<int>(W.X.Verdict)) + "\tH=" + Num(Cr.C.X.MomH, 9) + "\tshown=" + Num(Cr.C.X.MomShown, 9) +
			"\tearned=" + Num(W.X.Earned, 9) + "\tads=" + std::to_string(static_cast<int>(W.X.Ads)) + "\n";
		for (const video::Comp& K : W.X.Comps)
		{
			Out += "comp3\t" + std::to_string(S) + "\tT=" + Num(K.T, 9) + "\tP=" + Num(K.P, 9) + "\tA=" + Num(K.A, 9) + "\tkind=" + std::to_string(static_cast<int>(K.Kind)) + "\n";
		}
		for (const double T : {1.0, 7.0, 30.0, 119.0})
		{
			Out += "views3\t" + std::to_string(S) + "\t" + Num(T) + "\t" + Num(video::Views(W, W.At + T * Day), 9) + "\n";
		}
	}
	Career Cr = NewCareer();
	for (int D = 0; D <= 400; ++D)
	{
		CareerDay(Cr, D, "golden3:creator", ThreeAWeek, false, 3.0);
		if (D == 30 || D == 90 || D == 180 || D == 365 || D == 400)
		{
			const double W = static_cast<double>(D + 1) * Day;
			Out += "creator3\t" + std::to_string(D) + "\tsubs=" + Num(Cr.C.Subs, 9) + "\tviews=" + Num(video::TotalViews(Cr.C, W), 9) + "\tpartner=" + Num(Cr.C.PartnerAt, 9) +
				"\tbalance=" + std::to_string(Cr.C.BalanceCents) + "\tpaid=" + std::to_string(Cr.C.PaidCents) + "\tH=" + Num(Cr.C.X.MomH, 9) + "\tstreak=" +
				std::to_string(Cr.C.X.StreakW) + "\n";
		}
	}
	return Out;
}

void Goldens(bool Write)
{
	const std::string Path = std::string(SS_TESTS_DIR) + "/video_vectors_u3.txt";
	const std::string Now = GoldensU3();
	if (Write)
	{
		std::FILE* F = std::fopen(Path.c_str(), "wb");
		Expect(F != nullptr, "goldens: video_vectors_u3.txt can be written");
		if (F)
		{
			std::fwrite(Now.data(), 1, Now.size(), F);
			std::fclose(F);
			std::printf("  wrote %s (%zu bytes)\n", Path.c_str(), Now.size());
		}
		return;
	}
	std::ifstream In(Path, std::ios::binary);
	std::stringstream Was;
	Was << In.rdbuf();
	std::string Old = Was.str();
	Old.erase(std::remove(Old.begin(), Old.end(), '\r'), Old.end());
	if (Old != Now && !Old.empty())
	{
		auto Lines = [](const std::string& S) {
			std::vector<std::string> V;
			std::istringstream In(S);
			std::string L;
			while (std::getline(In, L))
			{
				V.push_back(L);
			}
			return V;
		};
		const std::vector<std::string> A = Lines(Old), B = Lines(Now);
		int Shown = 0;
		for (size_t I = 0; I < std::max(A.size(), B.size()) && Shown < 5; ++I)
		{
			const std::string La = I < A.size() ? A[I] : std::string(), Lb = I < B.size() ? B[I] : std::string();
			if (La != Lb)
			{
				std::printf("  golden: was \"%s\"\n          now \"%s\"\n", La.c_str(), Lb.c_str());
				++Shown;
			}
		}
	}
	Expect(!Old.empty() && Old == Now, "goldens: Rerun 2.0 matches Tests/video_vectors_u3.txt (\"rerun_algo_test --write\" rewrites it, the diff reviewed)");
}

// ================================================================== VideoGrind on Rerun 2.0 (update 3 SPEC §4.9)

/** VideoGrind's plans (session_seg_video.cpp), the same uploads on a Rerun 2.0 channel past Act I (RentsPaid 2), the money
 *  deseasonalized: Update 2's floors and ceilings hold (night_one_session keeps the Update 2 twins: its time budget). */
struct Grind
{
	int Gate = 0, FirstPay = 0, Tier4 = 0;
	double Subs30 = 0, Subs90 = 0, Subs365 = 0, Subs730 = 0, Subs5y = 0;
	double Ads365 = 0, Ads540 = 0, Ads5y = 0;
};

enum class Plan
{
	Rotation,
	Breakdowns,
	ShortsTriple,
	ShortAndBreakdowns,
};

Grind GrindU3(const std::string& Seed, Plan P, const std::vector<int>& Weekdays, int Days)
{
	video::Channel C;
	C.X.On = true;
	life::State L;
	L.RentsPaid = 2;
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	Grind G;
	std::vector<double> Earned(static_cast<size_t>(Days) + 2, 0.0);
	ss::Chips Was = 0;
	int Uploads = 0;
	for (int D = 0; D <= Days; ++D)
	{
		const bool OnDay = std::find(Weekdays.begin(), Weekdays.end(), D % 7) != Weekdays.end();
		std::vector<std::pair<double, video::Format>> Today;
		if (D >= 7)
		{
			switch (P)
			{
			case Plan::Rotation:
				if (OnDay)
				{
					Today.push_back({3.0, static_cast<video::Format>(Uploads % 3)});
				}
				break;
			case Plan::Breakdowns:
				if (OnDay)
				{
					Today.push_back({3.0, video::Format::Breakdown});
				}
				break;
			case Plan::ShortsTriple: Today = {{3.0, video::Format::Short}, {11.0, video::Format::Short}, {19.0, video::Format::Short}}; break;
			case Plan::ShortAndBreakdowns:
				Today.push_back({3.0, video::Format::Short});
				if (OnDay)
				{
					Today.push_back({15.0, video::Format::Breakdown});
				}
				break;
			}
		}
		for (size_t Slot = 0; Slot < Today.size(); ++Slot)
		{
			const double At = static_cast<double>(D) * Day + Today[Slot].first * 60.0;
			video::Choice Ch;
			Ch.F = Today[Slot].second;
			Ch.T = video::TitleStyle::Story;
			Ch.Fr = video::Frame::Versus;
			for (int I = 0; I < (Ch.F == video::Format::Vlog ? 3 : 1); ++I)
			{
				const std::string Id = "rl:grind@" + std::to_string(D) + ":" + std::to_string(static_cast<int>(Slot) * 10 + I);
				video::NoteHand(C, Moment(Id, At));
				Ch.Candidates.push_back(Id);
			}
			video::UploadFacts F;
			F.GearQuality = 0.5;
			video::Upload(C, Ch, At + 30.0, Seed, F);
			++Uploads;
		}
		const ss::Chips Before = Bank;
		video::Step(C, L, Bank, K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		G.FirstPay = G.FirstPay == 0 && Bank > Before ? D + 1 : G.FirstPay;
		G.Gate = G.Gate == 0 && C.PartnerAt >= 0.0 ? static_cast<int>(C.PartnerAt / Day) : G.Gate;
		G.Tier4 = G.Tier4 == 0 && C.Subs >= 50000.0 ? D + 1 : G.Tier4;
		const ss::Chips Now = C.BalanceCents + C.PaidCents;
		Earned[static_cast<size_t>(D + 1)] = Earned[static_cast<size_t>(D)] + static_cast<double>(Now - Was) / algo::Season(D);
		Was = Now;
		G.Subs30 = D + 1 == 30 ? C.Subs : G.Subs30;
		G.Subs90 = D + 1 == 90 ? C.Subs : G.Subs90;
		G.Subs365 = D + 1 == 365 ? C.Subs : G.Subs365;
		G.Subs730 = D + 1 == 730 ? C.Subs : G.Subs730;
		G.Subs5y = C.Subs;
	}
	auto Month = [&Earned](int To) { return To < 30 || To >= static_cast<int>(Earned.size()) ? 0.0 : (Earned[static_cast<size_t>(To)] - Earned[static_cast<size_t>(To - 30)]) / 100.0; };
	G.Ads365 = Month(365);
	G.Ads540 = Month(540);
	G.Ads5y = Month(Days + 1);
	return G;
}

void GrindBands()
{
	const auto T0 = std::chrono::steady_clock::now();
	std::vector<double> Gate3, Pay3, S30, S90, A365, A5y, GateD, AD365, AD540, AD5y;
	for (int Seed = 0; Seed < 5; ++Seed)
	{
		const Grind Three = GrindU3("grind:3:" + std::to_string(Seed), Plan::Rotation, ThreeAWeek, 1825);
		const Grind Seven = GrindU3("grind:7:" + std::to_string(Seed), Plan::Rotation, Daily, 1825);
		Gate3.push_back(Three.Gate > 0 ? Three.Gate : 9999);
		Pay3.push_back(Three.FirstPay > 0 ? Three.FirstPay : 9999);
		S30.push_back(Three.Subs30);
		S90.push_back(Three.Subs90);
		A365.push_back(Three.Ads365);
		A5y.push_back(Three.Ads5y);
		GateD.push_back(Seven.Gate > 0 ? Seven.Gate : 9999);
		AD365.push_back(Seven.Ads365);
		AD540.push_back(Seven.Ads540);
		AD5y.push_back(Seven.Ads5y);
	}
	std::printf("  grind (Rerun 2.0, medians of 5): three a week: subscribers %.0f on day 30, %.0f on day 90; Partner on day %.0f; first payout day %.0f; $%.0f a month at "
				"day 365, $%.0f in year 5 (deseasonalized)\n",
		Median(S30), Median(S90), Median(Gate3), Median(Pay3), Median(A365), Median(A5y));
	std::printf("  grind: daily: Partner on day %.0f; $%.0f a month at day 365, $%.0f at day 540, $%.0f in year 5\n", Median(GateD), Median(AD365), Median(AD540), Median(AD5y));
	Expect(Median(S30) >= 15.0 && Median(S30) <= 60.0 && Median(S90) >= 140.0 && Median(S90) <= 400.0, "grind: three a week, 15-60 subscribers at day 30 and 140-400 at day 90");
	Expect(Median(Gate3) >= 130.0 && Median(Gate3) <= 245.0 && Median(Pay3) <= 300.0, "grind: three a week, Partner between days 130 and 245, paid by day 300 (SPEC_CREATORS §3.6)");
	Expect(Median(A365) >= 110.0 && Median(A365) <= 420.0 && Median(A5y) <= 1075.0, "grind: three a week, $110-420 a month at day 365, at most a rent in year 5");
	Expect(Median(GateD) >= 90.0 && Median(GateD) <= 160.0 && Median(AD365) <= 1100.0 && Median(AD540) >= 540.0 && Median(AD5y) <= 2150.0,
		"grind: daily, Partner by days 90-160, at most $1,100 at day 365, $540 at day 540, two rents in year 5");
	std::vector<double> LongS365, ShortsS365, ShortsAds, ShortsTier4, MixedS365, MixedTier4;
	for (int Seed = 0; Seed < 5; ++Seed)
	{
		const Grind Long = GrindU3("grind:long:" + std::to_string(Seed), Plan::Breakdowns, ThreeAWeek, 730);
		const Grind Shorts = GrindU3("grind:shorts:" + std::to_string(Seed), Plan::ShortsTriple, {}, 730);
		const Grind Mixed = GrindU3("grind:mixed:" + std::to_string(Seed), Plan::ShortAndBreakdowns, ThreeAWeek, 730);
		LongS365.push_back(Long.Subs365);
		ShortsS365.push_back(Shorts.Subs365);
		ShortsAds.push_back(Shorts.Ads365);
		ShortsTier4.push_back(Shorts.Tier4 > 0 ? Shorts.Tier4 : 9999);
		MixedS365.push_back(Mixed.Subs365);
		MixedTier4.push_back(Mixed.Tier4 > 0 ? Mixed.Tier4 : 9999);
	}
	const double LongMedian = std::max(1.0, Median(LongS365));
	std::printf("  grind: three Breakdowns a week %.0f subscribers at day 365; three Cuts a day %.0f (%.2fx), $%.0f a month; a Cut a day + three Breakdowns %.0f (%.2fx); "
				"50,000 on days %.0f and %.0f (%.1f s)\n",
		LongMedian, Median(ShortsS365), Median(ShortsS365) / LongMedian, Median(ShortsAds), Median(MixedS365), Median(MixedS365) / LongMedian, Median(ShortsTier4), Median(MixedTier4),
		std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
	// (VideoGrind's v1.2 ceilings, with the tier-4 day restated for Rerun 2.0's faster channels: every plan grows about 1.4x,
	// Cuts no faster than the rest, so no Cuts-heavy channel at tier 4 inside its first year; Update 2's twins in
	// night_one_session keep day 400.)
	Expect(Median(ShortsAds) < 150.0 && Median(MixedS365) <= 2.5 * LongMedian && Median(ShortsS365) <= 4.5 * LongMedian && Median(ShortsTier4) >= 365.0 &&
			   Median(MixedTier4) >= 365.0,
		"grind: Cuts never outgrow every other plan (VideoGrind's v1.2 ceilings on Rerun 2.0; tier 4 not inside the first year)");
}

// ================================================================== C2 career by career (SPEC_CREATORS §3.6's first row)

/**
 * Every Update 2 choice set on the I6 grid, as a three-a-week career against its Update 2 twin on the same seeds: subscribers
 * at d90 and d365 and money at d365 at least 0.97x the twin's (ruling C2: nobody's channel gets slower; the streak, Fresh
 * and the Binge wave raise them on purpose). The grid spans the frames, the title styles (Teaching's evergreen lesson
 * included), the rig, the stakes, the best moment and the Kast audience (I6's axes), one archetype a corner.
 */
void GridCareers(int Seeds)
{
	const auto T0 = std::chrono::steady_clock::now();
	struct Cell
	{
		const char* Name;
		video::Frame Fr;
		video::TitleStyle T;
		video::Kind K;
		double Gear;
		double BuyIn;
		double Kast;
	};
	const Cell Cells[] = {
		{"grid: Reaction, Loud, a bad beat", video::Frame::Reaction, video::TitleStyle::Hype, video::Kind::BadBeat, 0.5, 550, 0},
		{"grid: Cards, Teaching, a bluff (lessons)", video::Frame::Cards, video::TitleStyle::Teaching, video::Kind::Bluff, 0.5, 550, 0},
		{"grid: Number, Story, a big pot, no rig", video::Frame::Number, video::TitleStyle::Story, video::Kind::BigPot, 0.0, 550, 0},
		{"grid: Trophy, Question, a win, full rig", video::Frame::Trophy, video::TitleStyle::Question, video::Kind::Win, 1.0, 550, 0},
		{"grid: Versus, Plain, a cooler, $55 stakes", video::Frame::Versus, video::TitleStyle::Plain, video::Kind::Cooler, 0.5, 5500, 0},
		{"grid: Versus, Story, 2,000 Kast followers", video::Frame::Versus, video::TitleStyle::Story, video::Kind::Cooler, 0.5, 550, 2000},
		{"grid: Reaction, Story, 20,000 Kast followers", video::Frame::Reaction, video::TitleStyle::Story, video::Kind::HeroCall, 0.5, 2200, 20000},
	};
	double Worst = 99.0;
	std::string WorstName;
	for (const Cell& G : Cells)
	{
		Arch A = Neutral(G.Name, ThreeAWeek, true);
		A.Fr = G.Fr;
		A.T = G.T;
		A.K = G.K;
		A.Gear = {0, 0, G.Gear, G.Gear};
		A.BuyIn = {0, 0, G.BuyIn, G.BuyIn};
		A.Kast = {0, 0, G.Kast, G.Kast};
		if (G.T == video::TitleStyle::Teaching)
		{
			A.Mix = {video::Format::Breakdown}; // Teaching Breakdowns of a decision: the evergreen class
		}
		const Paired P = PairedRatios(A, Seeds, 400);
		const double Low = std::min({P.Subs90, P.Subs365, P.Money365 > 0.0 ? P.Money365 : 9.0});
		std::printf("  %-46s subs d90 x%.3f, d365 x%.3f; money d365 x%.3f\n", G.Name, P.Subs90, P.Subs365, P.Money365);
		if (Low < Worst)
		{
			Worst = Low;
			WorstName = G.Name;
		}
	}
	std::printf("  grid: %d seeds in %.1f s\n", Seeds, std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
	Expect(Worst >= 0.97, "bands: every Update 2 choice set on the I6 grid at least 0.97x its twin at d90 and d365 (C2; lowest x" + Num(Worst, 3) + ", " + WorstName + ")");
}

// ================================================================== the PIE criterion's levers that need no library (§4.9)

/**
 * §4.9's PIE criterion: at a 2,000-5,000 subscriber channel, the median views of a well-packaged upload are at least +25%
 * over a neutral one of the same kind. Its levers that need no library are measured here (published before the peak, an
 * honest title, a launch action: a teaser); the archetype that fits the moment is core-videocontent's (ArchetypeFit's stub is
 * neutral), so the band is asserted at C3 with the library. Printed, and held to "more views, never fewer".
 */
void PieCriterion()
{
	std::vector<double> Neutral, Packaged;
	for (int S = 0; S < 401; ++S) // (the luck is each upload's own: 401 a side keeps the medians' noise near 5%)
	{
		for (const bool Well : {false, true})
		{
			const double DayAt = 60.0 * Day;
			// (Before the peak: the 9 PM hour curve's window opens 2 hours before; the neutral drop is Update 2's 4 AM.)
			const double At = DayAt + (Well ? 19.5 : 4.0) * 60.0;
			video::Channel C = PlainU3Channel(At);
			C.Subs = 3000.0;
			const video::Candidate M = Moment("rl:pie@60:" + std::to_string(S), At - 60.0, video::Kind::BadBeat);
			video::NoteHand(C, M);
			video::Choice Ch;
			Ch.Candidates = {M.Id};
			Ch.F = video::Format::Breakdown;
			Ch.T = video::TitleStyle::Story; // honest: the Story style's promise 1 against a bad beat's payoff
			video::UploadFacts F;
			F.GearQuality = 0.5;
			UploadOn(C, Ch, At, "pie:" + std::to_string(S), F);
			if (Well)
			{
				C.Videos.back().X.Launch = static_cast<uint8_t>(1u << static_cast<int>(video::Launch::Teaser));
			}
			life::State L;
			ss::kast::Channel K;
			ss::Chips Bank = 0;
			video::Step(C, L, Bank, K, At, At + 20.0 * Day);
			// (Each upload's own luck divided out, ln(Luck x Viral) and the CTR noise: the two sides draw apart, being different uploads.)
			const video::Video& W = C.Videos.back();
			(Well ? Packaged : Neutral).push_back(video::Views(W, At + 20.0 * Day) / (std::exp(static_cast<double>(W.X.LuckLn)) * W.X.CtrNoise));
		}
	}
	const double Lift = Median(Packaged) / std::max(1.0, Median(Neutral)) - 1.0;
	std::printf("  pie: a 3,000-subscriber channel's bad beat, 401 uploads each: published before the peak with a teaser, %+.1f%% median views at day 20, each "
				"upload's luck divided out "
				"(the archetype's fit, core-videocontent's, brings the rest of +25%%)\n",
		100.0 * Lift);
	Expect(Lift > 0.0, "pie: the levers that need no library add views (" + Num(100.0 * Lift, 3) + "%)");
}

// ================================================================== spike S-C's bands (SPEC_CREATORS §3.6)

void Bands(int Seeds)
{
	const auto T0 = std::chrono::steady_clock::now();
	const Arch Three = Neutral("3/wk neutral", ThreeAWeek, true);
	const Summary S = Summarize(Three, Seeds, 400);
	const Paired P = PairedRatios(Three, Seeds, 400);
	std::printf("  bands: %s\n", Line(S).c_str());
	std::printf("  bands: paired with its twin: subs d90 x%.3f, d365 x%.3f; money d365 x%.3f; Partner day x%.3f\n", P.Subs90, P.Subs365, P.Money365, P.Gate);
	Expect(S.Partner >= 130.0 && S.Partner <= 245.0, "bands: 3/wk neutral reaches Partner between days 130 and 245 (" + Num(S.Partner) + ")");
	Expect(P.Subs90 >= 0.97 && P.Subs365 >= 1.0 && P.Subs365 <= 1.7 && P.Money365 >= 1.0 && P.Money365 <= 1.6,
		"bands: 3/wk neutral against its twin: subscribers x1.0-1.7 at d365, money x1.0-1.6, never slower at d90 (C2)");
	Expect(S.MomP10 <= 0.80 && S.MomP90 >= 1.25, "bands: the Momentum multiplier moves (p10 " + Num(S.MomP10, 3) + ", p90 " + Num(S.MomP90, 3) + ")");
	Expect(S.Labels[0] >= 12.0 && S.Labels[0] <= 28.0 && S.Labels[3] + S.Labels[4] >= 18.0 && S.Labels[3] + S.Labels[4] <= 32.0,
		"bands: the labels tell the truth: Not pushed 12-28%, Pushed more and hard 18-32%");
	Expect(S.Moves2 <= 10.0, "bands: the displayed meter moves 2+ bars on at most 10% of uploads (" + Num(S.Moves2, 3) + "%)");
	// (§3.6: the next three >= x1.35 and lag-1 >= +0.20, checked at 24 seeds; the default run's 8 seeds read them a little wide.)
	Expect(S.After3 >= (Seeds >= 24 ? 1.35 : 1.30) && S.Lag1 >= (Seeds >= 24 ? 0.20 : 0.17), "bands: a hit carries (the next three x" + Num(S.After3, 3) + ", lag-1 " + Num(S.Lag1, 3) + ")");
	Expect(S.MaxComps <= static_cast<double>(valgo::VaMaxComps), "bands: Binge and Late Bloomer stay inside the slots");
	Arch Streamer = Neutral("3/wk streamer 20k", ThreeAWeek, true);
	Streamer.Kast = {0, 0, 20000, 20000};
	Streamer.StreamViewers = 100;
	Streamer.LiveAvg = 180;
	Streamer.LiveNorm = 180;
	Streamer.LiveMinutes = 180;
	const Paired Ps = PairedRatios(Streamer, std::max(4, Seeds / 2), 400);
	const Summary Ss = Summarize(Streamer, 2, 120);
	Expect(Ps.Subs90 >= 0.9 && Ps.Subs365 >= 0.9 && Ss.DropShare >= 0.05, "bands: the 20,000-follower streamer at least 0.9x its twin, Live Drop worth 5% of P0 or more (" +
		Num(Ss.DropShare, 3) + ")");
	// The traps and the trick (§4.9's rows), each against the neutral creator on its own seeds: Heavy from day one costs
	// subscribers and pays no more; Heavy after the last check-in is a small trick, never a chore (S-C at 48 seeds: x0.95, x1.02).
	Arch AdMax = Neutral("3/wk ad-maxer", ThreeAWeek, true);
	AdMax.Ads = video::AdLoad::Heavy;
	AdMax.Seeds = Three.Name;
	Arch Switch = Neutral("3/wk switcher", ThreeAWeek, true);
	Switch.Switcher = true;
	Switch.Seeds = Three.Name;
	const Paired Pm = PairedBetween(AdMax, Three, Seeds, 400), Pw = PairedBetween(Switch, Three, Seeds, 400);
	const double Slack = Seeds >= 24 ? 0.0 : 0.05; // the default run's 8 seeds
	Expect(Pm.Money365 <= 1.0 + Slack && Pm.Subs365 < 0.9 && Pm.Gate > 1.0, "bands: the ad-maxer costs subscribers and the gate and pays no more than neutral (money x" +
		Num(Pm.Money365, 3) + ", subscribers x" + Num(Pm.Subs365, 3) + ")");
	Expect(Pw.Money365 >= 0.98 && Pw.Money365 <= 1.10, "bands: the switcher's trick pays at most 1.10x (x" + Num(Pw.Money365, 3) + ")");
	std::printf("  bands: %d seeds in %.1f s\n", Seeds, std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace rerun_algo_test

int main(int Argc, char** Argv)
{
	using namespace rerun_algo_test;
	const auto T0 = std::chrono::steady_clock::now();
	const std::string Mode = Argc > 1 ? Argv[1] : "";
	if (Mode.empty() || Mode == "long")
	{
		const auto Lap = [&T0](const char* What) {
			std::printf("  %s done at %.1f s\n", What, std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
			std::fflush(stdout);
		};
		Pure();
		Lap("pure");
		Identity();
		Lap("identity");
		Invariants();
		Lap("invariants");
		Decisions();
		Lap("decisions");
		Levers();
		Lap("levers");
		SuitabilityAndTrends();
		Lap("suitability and trends");
		CarryAndWaves();
		Lap("carry and waves");
		Saves();
		Lap("saves");
		Analytics();
		Lap("analytics");
		Bands(Mode == "long" ? 24 : 8);
		Lap("bands");
		GridCareers(Mode == "long" ? 24 : 8);
		Lap("grid");
		PieCriterion();
		Lap("pie");
		GrindBands();
		Lap("grind");
		Goldens(false);
		Lap("goldens");
	}
	if (Mode == "--write")
	{
		Goldens(true);
	}
	if (Mode == "grid")
	{
		GridCareers(Argc > 2 ? std::atoi(Argv[2]) : 24);
	}
	if (Mode == "sc")
	{
		const int Seeds = Argc > 2 ? std::atoi(Argv[2]) : 24;
		for (const Arch& A : ScArchetypes())
		{
			const int N = A.Weekdays.size() == 7 ? std::max(2, Seeds / 2) : Seeds;
			std::printf("%s\n", Line(Summarize(A, N, 730)).c_str());
			if (A.U3)
			{
				const Paired P = PairedRatios(A, N, 730);
				std::printf("%-26s paired with its twin: the first 10 uploads' views x%.3f; subs d90 x%.3f, d365 x%.3f, d730 x%.3f; money d365 x%.3f; Partner day x%.3f\n",
					A.Name.c_str(), P.First10, P.Subs90, P.Subs365, P.Subs730, P.Money365, P.Gate);
			}
			std::fflush(stdout);
		}
	}
	if (Mode == "dump")
	{
		// A three-a-week creator's save after 200 days, a few lines of each Update 3 kind (for review).
		Career Cr = NewCareer();
		for (int D = 0; D < 200; ++D)
		{
			CareerDay(Cr, D, "dump", ThreeAWeek, false, 3.0, true);
		}
		std::map<std::string, int> Shown;
		for (const std::vector<std::string>& P : SplitLines(CareerText(Cr)))
		{
			const std::string Tag = P.size() > 1 ? P[1] : "";
			if (Shown[Tag]++ < 3)
			{
				std::string Line;
				for (size_t I = 0; I < P.size(); ++I)
				{
					Line += (I > 0 ? " | " : "") + P[I];
				}
				std::printf("%s\n", Line.c_str());
			}
		}
	}
	if (Mode == "scx")
	{
		// Spike S-C's experiments: one lever at a time on the 3/wk neutral creator.
		const int Seeds = Argc > 2 ? std::atoi(Argv[2]) : 24;
		const Arch Base = Neutral("3/wk neutral", ThreeAWeek, true);
		Arch AdMax = Neutral("3/wk ad-maxer", ThreeAWeek, true);
		AdMax.Ads = video::AdLoad::Heavy;
		AdMax.Seeds = Base.Name;
		Arch Switch = Neutral("3/wk switcher", ThreeAWeek, true);
		Switch.Switcher = true;
		Switch.Seeds = Base.Name;
		for (const Arch& A : {AdMax, Switch})
		{
			const Paired P = PairedBetween(A, Base, Seeds, 730);
			std::printf("%-26s against 3/wk neutral on its seeds: the first 10 uploads' views x%.3f; subs d90 x%.3f, d365 x%.3f, d730 x%.3f; money d365 x%.3f; Partner day x%.3f\n",
				A.Name.c_str(), P.First10, P.Subs90, P.Subs365, P.Subs730, P.Money365, P.Gate);
			std::fflush(stdout);
		}
	}
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("rerun algo tests: %d checks, %d failed in %.1fs\n", Checks, Failures, Seconds);
	return Failures == 0 ? 0 : 1;
}
