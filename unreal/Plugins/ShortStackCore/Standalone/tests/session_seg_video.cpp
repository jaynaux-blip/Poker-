// night_one_session: core-video's additions (update 2 SPEC §6.7): capture on the watch path draws nothing (a hyper played
// with every hand captured twice tells the same chat and finishes the same), capture on the skip path, the event's recap,
// a life story (the rent-paid one only when poker paid it: C2's S4), VideoGrind (§4.6: two creators in closed form, their
// floors and ceilings; C2's S2: the long-form creator and two Shorts-heavy ones), payouts through CheckCalendar
// (the review, Partner, the 21st), the edit through FinishSkip (its time, its energy, the result card, Dee's one text),
// and a save that only writes the channel once it has something. Compiled into session_test and called from main() in
// session_main.cpp (the kit is SessionSeg.h); helpers in session_test::video_seg.
#include "SessionSeg.h"

#include "ShortStack/Cards.h"
#include "ShortStack/Evaluator.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/VideoAlgo.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>

namespace session_test
{
namespace video_seg
{
namespace video = ss::video;
namespace life = ss::life;
const double Day = 1440.0;

void Lobby(ss::Session& S, double& Now)
{
	S.CurrentScreen = ss::Screen::Lobby;
	Now = Wait(S, Now, 0.2);
}

/** Plays the table to the results the way PlayOut does; Twice: every finished hand is captured a second time. */
double PlayTable(ss::Session& S, ss::Rng& Choice, double Now, bool Twice, int& Extra)
{
	const double Dt = 1.0 / 30.0;
	int Frames = 0;
	int Captured = -1;
	while (S.CurrentScreen == ss::Screen::Table && ++Frames < 4000000)
	{
		Now += Dt;
		S.Update(Now);
		if (Twice && S.CurHand && S.CurHand->bComplete && S.HandsPlayed != Captured)
		{
			Captured = S.HandsPlayed;
			ss::Session::HandFacts F;
			for (const ss::SeatVis& V : S.Seats)
			{
				F.EquityAtAllIn = V.IsHero && V.HasEquity ? V.Equity : F.EquityAtAllIn;
			}
			S.VideoNoteHand(*S.CurHand, F);
			++Extra;
		}
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.5)
		{
			const double Roll = Choice.Next();
			if (Roll < 0.15 && S.Prompt.CanRaise)
			{
				S.HeroAct(ss::PlayerAction::RaiseTo(static_cast<double>(S.Prompt.RaiseTo)));
			}
			else if (Roll < 0.75 || S.Prompt.CanCheck)
			{
				S.HeroAct(S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call());
			}
			else
			{
				S.HeroAct(ss::PlayerAction::Fold());
			}
		}
	}
	return Now;
}

void CaptureDrawsNothing()
{
	Hooks Ha, Hb;
	ss::Session A(Ha, "video-capture"), B(Hb, "video-capture");
	double Na = 0.0, Nb = 0.0;
	Lobby(A, Na);
	Lobby(B, Nb);
	A.BankrollCents = B.BankrollCents = 10000;
	A.Register(1); // $0.25 Hyper Sprint, 180 players
	B.Register(1);
	ss::Rng Ca("video-capture-choices"), Cb("video-capture-choices");
	int Unused = 0, Extra = 0;
	PlayTable(A, Ca, Na, false, Unused);
	PlayTable(B, Cb, Nb, true, Extra);
	bool SameChat = A.Chat.size() == B.Chat.size();
	for (size_t I = 0; SameChat && I < A.Chat.size(); ++I)
	{
		SameChat = A.Chat[I].Who == B.Chat[I].Who && A.Chat[I].Text == B.Chat[I].Text && A.Chat[I].Kind == B.Chat[I].Kind;
	}
	Expect(A.CurrentScreen == ss::Screen::Results && B.CurrentScreen == ss::Screen::Results, "video capture: both hypers reach the results");
	Expect(SameChat && A.LastResults.Place == B.LastResults.Place && A.LastResults.Hands == B.LastResults.Hands && A.BankrollCents == B.BankrollCents,
		"video capture: every hand captured twice tells the same chat and finishes the same (capture draws nothing)");
	// What the watch path noted: real hands, as the player saw them.
	bool Formed = true;
	for (const video::Candidate& C : A.Life.Video.Open)
	{
		bool You = C.K == video::Kind::DeepRun || C.K == video::Kind::FinalTable || C.K == video::Kind::Win;
		for (const ss::recall::Line& L : C.Rec.Lines)
		{
			You = You || L.Who == "You";
		}
		Formed = Formed && You && C.Source == 0 && C.Id.rfind("rl:", 0) == 0 && C.Drama > 0.0 && C.Expires > C.At;
	}
	std::printf("  video capture: %zu candidates from a hyper (%d extra captures), finished %s\n", A.Life.Video.Open.size(), Extra, ss::Ordinal(A.LastResults.Place).c_str());
	Expect(Formed, "video capture: every candidate is a real hand with the player in it");
	Expect(A.Life.Video.Open.empty() || (A.Life.Update2LinesRead & life::U2Video) != 0, "video capture: the channel's bit is set once it has something to keep");
	// The event's recap: a win at this table is a candidate keyed to the event.
	ss::Results R;
	R.Place = 1;
	R.Won = true;
	R.Entrants = 180;
	R.PrizeCents = 1215;
	R.BuyInCents = 25;
	Hooks Hc;
	ss::Session C(Hc, "video-recap");
	double Nc = 0.0;
	Lobby(C, Nc);
	C.Register(1);
	C.VideoRecap(R);
	const std::vector<video::Candidate> Open = C.VideoCandidates();
	Expect(Open.size() == 1 && Open[0].K == video::Kind::Win && Open[0].Rec.Pot == 1215 && Open[0].Id == "rl:" + C.T->Spec.Id + ":recap" && Open[0].PlaceSoFar == 1,
		"video recap: a win is a candidate, keyed to the event, its prize in the record");
}

/** Aces all in preflop against sevens, the player on the button, a river from the list ("7d": the sevens get there). */
std::unique_ptr<ss::Hand> AcesAllIn(const char* River)
{
	// Dealt from the button's left: the villain, the player, the villain, the player; then burn, flop, burn, turn, burn, river.
	std::vector<ss::Card> Deck = ss::ParseCards(std::string("7cAs7hAd2hKd8s2c3h9h4h") + River);
	for (int C = 0; C < 52; ++C)
	{
		if (std::find(Deck.begin(), Deck.end(), C) == Deck.end())
		{
			Deck.push_back(C);
		}
	}
	ss::HandConfig Cfg;
	Cfg.Players = {{0, ss::HeroId, 6400}, {1, "villain", 6400}};
	Cfg.ButtonSeat = 0;
	Cfg.SmallBlind = 100;
	Cfg.BigBlind = 200;
	Cfg.Deck = Deck;
	ss::Rng R("video-aces");
	auto Hd = std::make_unique<ss::Hand>(Cfg, R);
	for (const ss::PlayerAction& A : {ss::PlayerAction::RaiseTo(600), ss::PlayerAction::RaiseTo(6400), ss::PlayerAction::Call()})
	{
		Hd->Act(A);
	}
	return Hd;
}

void BubbleHands()
{
	// The bubble is hand for hand: an all-in once the money's reached (the burst a hand ago) is no bubble hand.
	Hooks H;
	ss::Session S(H, "video-bubble");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 10000;
	S.Register(1);
	Expect(S.T != nullptr, "video bubble: a table is open");
	if (!S.T)
	{
		return;
	}
	const auto Won = AcesAllIn("5d");
	ss::Session::HandFacts F;
	S.T->Remaining = S.T->PaidPlaces(); // in the money: the bubble burst on an earlier hand
	S.VideoNoteHand(*Won, F);
	const size_t After = S.Life.Video.Open.size();
	const bool Past = After == 0 || (S.Life.Video.Open.back().K != video::Kind::Bubble && !S.Life.Video.Open.back().Bubble);
	S.T->Remaining = S.T->PaidPlaces() + 1; // hand for hand
	S.T->Tick += 1;
	S.VideoNoteHand(*Won, F);
	const bool At = S.Life.Video.Open.size() == After + 1 && S.Life.Video.Open.back().K == video::Kind::Bubble;
	Expect(Won->bComplete && Past && At, "video bubble: an all-in at hand for hand is a bubble hand, one in the money after the burst isn't");
}

void WatchedBadBeat()
{
	// The watch path hands VideoNoteHand the table's equity, which the run-out has updated to the river's by the award (0
	// for the loser). The candidate is still the all-in's bad beat: aces against sevens preflop, about 81%, a seven on the river.
	Hooks H;
	ss::Session S(H, "video-watched-beat");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 10000;
	S.Register(1);
	Expect(S.T != nullptr, "video watched beat: a table is open");
	if (!S.T)
	{
		return;
	}
	const auto Hd = AcesAllIn("7d");
	ss::Session::HandFacts F;
	F.Skipped = false;
	F.EquityAtAllIn = 0.0; // what the table shows at the award
	S.VideoNoteHand(*Hd, F);
	const video::Candidate* Beat = nullptr;
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		Beat = C.K == video::Kind::BadBeat ? &C : Beat;
	}
	Expect(Hd->bComplete && Beat && Beat->EquityAtAllIn > 0.75 && Beat->EquityAtAllIn < 0.86 && Beat->Source == 0,
		("video watched beat: a watched bad beat is one, at the all-in's equity (" + std::to_string(Beat ? Beat->EquityAtAllIn : -1.0) + ")").c_str());
}

void SkipCapture()
{
	Hooks H;
	ss::Session S(H, "video-skip");
	double Now = 0.0;
	Lobby(S, Now);
	S.BankrollCents = 10000;
	S.Register(1);
	S.CurrentPace = ss::Pace::Sprint;
	ss::Rng Choice("video-skip-choices");
	int Decisions = 0;
	Now = PlayOut(S, Choice, Now, Decisions);
	bool Skipped = true;
	int FromSkip = 0;
	// Every hand the plan played while skipping is the skip path's (Source 3); a hand the player was handed is the watch path's.
	for (const video::Candidate& C : S.Life.Video.Open)
	{
		const bool Event = C.K == video::Kind::DeepRun || C.K == video::Kind::FinalTable || C.K == video::Kind::Win;
		Skipped = Skipped && (Event || C.Source == 3 || (Decisions > 0 && C.Source == 0));
		FromSkip += C.Source == 3 ? 1 : 0;
	}
	std::printf("  video skip: %zu candidates from a sprinted hyper (%d by the skip path)\n", S.Life.Video.Open.size(), FromSkip);
	Expect(S.CurrentScreen == ss::Screen::Results && Skipped, "video skip: the skip path's hands are candidates of their own source");
}

// ------------------------------------------------------------------ VideoGrind (§4.6)

video::Candidate Moment(const std::string& Id, double World)
{
	// A cooler at $5.50: Topic 1.8 with the Versus frame's 1.0 (the Python model's Topic 1.5 x Thumb 1.2).
	video::Candidate C;
	C.Id = Id;
	C.At = World;
	C.K = video::Kind::Cooler;
	C.Event = "Night Owl Turbo";
	C.BuyIn = 550;
	C.Rec.Blinds.Bb = 200;
	C.Rec.Pot = 12800;
	C.Rec.HeroNet = -6400;
	C.Rec.Mine = {48, 49};
	C.Expires = World + 21.0 * Day;
	return C;
}

struct Grind
{
	int Gate = 0;       // the day Partner began (0: never)
	int FirstPay = 0;   // the day of the first payout (0: never)
	int Tier4 = 0;      // the first day at 50,000 subscribers (the creator road's tier 4; 0: never)
	double Subs30 = 0, Subs90 = 0, Subs180 = 0, Subs365 = 0, Subs730 = 0, Subs5y = 0;
	double Ads365 = 0, Ads540 = 0, Ads5y = 0; // dollars earned in the 30 days to that day, deseasonalized (each day's ÷ Season(day))
};

/**
 * What a creator posts each day from day 7 (Story titles, the Versus frame, a mid rig). §4.6's two creators rotate Short,
 * Breakdown and Vlog on their weekdays; v1.2 (S2) adds the long-form creator they're measured against and two
 * Shorts-heavy ones.
 */
enum class GrindPlan
{
	Rotation,           // on the weekdays given: Short, Breakdown and Vlog in turn, at 3:30 AM
	Breakdowns,         // on the weekdays given: a Breakdown at 3:30 AM (three a week: the long-form creator)
	ShortsTriple,       // every day: three Shorts, at 3:30 AM, 11:30 AM and 7:30 PM
	ShortAndBreakdowns, // every day a Short at 3:30 AM; on the weekdays given a Breakdown at 3:30 PM too
};

/**
 * A creator in closed form (a channel and a calendar, no tables): uploads from day 7, by the plan. Update 3 (SPEC §4.9, C3):
 * the channel runs Rerun 2.0 from its first upload (a new career's, ChannelX on) past Act I (RentsPaid 2, so W11 holds no
 * review), with Update 2's choices; its money is deseasonalized (each day's cents divided by that day's Season), so the
 * floors and ceilings read the channel and not the advertisers' calendar (the daily creator's day 540 falls in late March).
 */
Grind Creator(const std::string& Seed, GrindPlan Plan, const std::vector<int>& Weekdays, int Days)
{
	video::Channel C;
	C.X.On = true;
	life::State L;
	L.RentsPaid = 2;
	ss::kast::Channel K;
	ss::Chips Bank = 0;
	Grind G;
	std::vector<double> Earned(static_cast<size_t>(Days) + 2, 0.0);
	ss::Chips EarnedWas = 0;
	int Uploads = 0;
	for (int D = 0; D <= Days; ++D)
	{
		const bool OnDay = std::find(Weekdays.begin(), Weekdays.end(), D % 7) != Weekdays.end();
		std::vector<std::pair<double, video::Format>> Today; // (the hour the moment came, the format)
		if (D >= 7)
		{
			switch (Plan)
			{
			case GrindPlan::Rotation:
				if (OnDay)
				{
					Today.push_back({3.0, static_cast<video::Format>(Uploads % 3)});
				}
				break;
			case GrindPlan::Breakdowns:
				if (OnDay)
				{
					Today.push_back({3.0, video::Format::Breakdown});
				}
				break;
			case GrindPlan::ShortsTriple: Today = {{3.0, video::Format::Short}, {11.0, video::Format::Short}, {19.0, video::Format::Short}}; break;
			case GrindPlan::ShortAndBreakdowns:
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
			F.GearQuality = 0.5; // a mid rig
			video::Upload(C, Ch, At + 30.0, Seed, F);
			++Uploads;
		}
		const ss::Chips Was = Bank;
		video::Step(C, L, Bank, K, static_cast<double>(D) * Day, static_cast<double>(D + 1) * Day);
		G.FirstPay = G.FirstPay == 0 && Bank > Was ? D + 1 : G.FirstPay;
		G.Gate = G.Gate == 0 && C.PartnerAt >= 0.0 ? static_cast<int>(C.PartnerAt / Day) : G.Gate;
		G.Tier4 = G.Tier4 == 0 && C.Subs >= 50000.0 ? D + 1 : G.Tier4;
		const ss::Chips EarnedNow = C.BalanceCents + C.PaidCents;
		Earned[static_cast<size_t>(D + 1)] = Earned[static_cast<size_t>(D)] + static_cast<double>(EarnedNow - EarnedWas) / ss::video::algo::Season(D);
		EarnedWas = EarnedNow;
		G.Subs30 = D + 1 == 30 ? C.Subs : G.Subs30;
		G.Subs90 = D + 1 == 90 ? C.Subs : G.Subs90;
		G.Subs180 = D + 1 == 180 ? C.Subs : G.Subs180;
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

double Median(std::vector<double> X)
{
	std::sort(X.begin(), X.end());
	return X.empty() ? 0.0 : X[X.size() / 2];
}

void VideoGrind()
{
	const auto T0 = std::chrono::steady_clock::now();
	std::vector<double> Gate3, Pay3, S30, S90, A365, A5y, GateD, AD365, AD540, AD5y, SD5y;
	for (int Seed = 0; Seed < 5; ++Seed)
	{
		const Grind Three = Creator("grind:3:" + std::to_string(Seed), GrindPlan::Rotation, {0, 2, 4}, 1825);
		const Grind Daily = Creator("grind:7:" + std::to_string(Seed), GrindPlan::Rotation, {0, 1, 2, 3, 4, 5, 6}, 1825);
		Gate3.push_back(Three.Gate > 0 ? Three.Gate : 9999);
		Pay3.push_back(Three.FirstPay > 0 ? Three.FirstPay : 9999);
		S30.push_back(Three.Subs30);
		S90.push_back(Three.Subs90);
		A365.push_back(Three.Ads365);
		A5y.push_back(Three.Ads5y);
		GateD.push_back(Daily.Gate > 0 ? Daily.Gate : 9999);
		AD365.push_back(Daily.Ads365);
		AD540.push_back(Daily.Ads540);
		AD5y.push_back(Daily.Ads5y);
		SD5y.push_back(Daily.Subs5y);
	}
	const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
	std::printf("  VideoGrind (Rerun 2.0, medians of 5, %.1f s): three a week: subscribers %.0f on day 30, %.0f on day 90; Partner on day %.0f; first payout day %.0f; "
				"$%.0f a month at day 365, $%.0f in year 5 (deseasonalized)\n",
		Seconds, Median(S30), Median(S90), Median(Gate3), Median(Pay3), Median(A365), Median(A5y));
	std::printf("  VideoGrind: daily: Partner on day %.0f; $%.0f a month at day 365, $%.0f at day 540, $%.0f in year 5; %.0f subscribers in year 5\n", Median(GateD), Median(AD365),
		Median(AD540), Median(AD5y), Median(SD5y));
	Expect(Median(S30) >= 15.0 && Median(S30) <= 60.0, "VideoGrind: three a week, 15 to 60 subscribers at day 30");
	Expect(Median(S90) >= 140.0 && Median(S90) <= 400.0, "VideoGrind: three a week, 140 to 400 subscribers at day 90");
	Expect(Median(Gate3) >= 150.0 && Median(Gate3) <= 240.0, "VideoGrind: three a week, the partner gate between day 150 and 240");
	Expect(Median(Pay3) <= 300.0, "VideoGrind: three a week, the first payout by day 300");
	Expect(Median(A365) >= 110.0 && Median(A365) <= 420.0, "VideoGrind: three a week, $110 to $420 a month at day 365");
	Expect(Median(A5y) <= 1075.0, "VideoGrind: three a week, at most one rent a month in year 5");
	Expect(Median(GateD) >= 90.0 && Median(GateD) <= 160.0, "VideoGrind: daily, the partner gate between day 90 and 160");
	Expect(Median(AD365) <= 1000.0, "VideoGrind: daily, at most $1,000 a month at day 365");
	Expect(Median(AD540) >= 540.0, "VideoGrind: daily, at least half the rent a month at day 540");
	Expect(Median(AD5y) <= 2150.0 && Median(SD5y) <= 1000000.0, "VideoGrind: daily, at most two rents a month and a million subscribers in year 5");

	// v1.2 (the G2 audit's S2): Shorts must not outgrow every other plan. FormatSubs (a Short's viewers subscribe at half a
	// long video's rate) against three creators to day 730: three Breakdowns a week (the long-form creator they're measured
	// against), three Shorts a day, and a Short a day with three Breakdowns a week.
	const auto T1 = std::chrono::steady_clock::now();
	std::vector<double> LongS365, LongS730, LongGate, ShortsS90, ShortsS365, ShortsS730, ShortsAds, ShortsTier4, MixedS90, MixedS365, MixedS730, MixedGate, MixedAds, MixedTier4;
	for (int Seed = 0; Seed < 5; ++Seed)
	{
		const Grind Long = Creator("grind:long:" + std::to_string(Seed), GrindPlan::Breakdowns, {0, 2, 4}, 730);
		const Grind Shorts = Creator("grind:shorts:" + std::to_string(Seed), GrindPlan::ShortsTriple, {}, 730);
		const Grind Mixed = Creator("grind:mixed:" + std::to_string(Seed), GrindPlan::ShortAndBreakdowns, {0, 2, 4}, 730);
		LongS365.push_back(Long.Subs365);
		LongS730.push_back(Long.Subs730);
		LongGate.push_back(Long.Gate > 0 ? Long.Gate : 9999);
		ShortsS90.push_back(Shorts.Subs90);
		ShortsS365.push_back(Shorts.Subs365);
		ShortsS730.push_back(Shorts.Subs730);
		ShortsAds.push_back(Shorts.Ads365);
		ShortsTier4.push_back(Shorts.Tier4 > 0 ? Shorts.Tier4 : 9999);
		MixedS90.push_back(Mixed.Subs90);
		MixedS365.push_back(Mixed.Subs365);
		MixedS730.push_back(Mixed.Subs730);
		MixedGate.push_back(Mixed.Gate > 0 ? Mixed.Gate : 9999);
		MixedAds.push_back(Mixed.Ads365);
		MixedTier4.push_back(Mixed.Tier4 > 0 ? Mixed.Tier4 : 9999);
	}
	const double LongMedian = Median(LongS365);
	std::printf("  VideoGrind (v1.2, %.1f s): three Breakdowns a week: %.0f subscribers at day 365, %.0f at day 730; Partner on day %.0f\n",
		std::chrono::duration<double>(std::chrono::steady_clock::now() - T1).count(), LongMedian, Median(LongS730), Median(LongGate));
	auto Tier4Day = [](const std::vector<double>& X) { return Median(X) >= 9999.0 ? std::string("not by day 730") : "day " + std::to_string(static_cast<int>(Median(X))); };
	std::printf("  VideoGrind: three Shorts a day: %.0f / %.0f / %.0f subscribers at days 90 / 365 / 730 (%.2fx the long-form creator at day 365); 50,000 on %s; "
				"$%.0f a month at day 365\n",
		Median(ShortsS90), Median(ShortsS365), Median(ShortsS730), Median(ShortsS365) / std::max(1.0, LongMedian), Tier4Day(ShortsTier4).c_str(), Median(ShortsAds));
	std::printf("  VideoGrind: a Short a day + three Breakdowns a week: %.0f / %.0f / %.0f subscribers at days 90 / 365 / 730 (%.2fx); 50,000 on %s; Partner on day %.0f; "
				"$%.0f a month at day 365\n",
		Median(MixedS90), Median(MixedS365), Median(MixedS730), Median(MixedS365) / std::max(1.0, LongMedian), Tier4Day(MixedTier4).c_str(), Median(MixedGate),
		Median(MixedAds));
	Expect(Median(ShortsAds) < 150.0, "VideoGrind: three Shorts a day earn under $150 a month at day 365 (Shorts' RPM is 0.03)");
	Expect(Median(MixedS365) <= 2.5 * LongMedian, "VideoGrind: a Short a day + three Breakdowns a week, at most 2.5x the long-form creator's subscribers at day 365");
	Expect(Median(ShortsS365) < 50000.0 && Median(MixedS365) < 50000.0, "VideoGrind: neither Shorts-heavy creator reaches 50,000 subscribers (tier 4) in the first year");
	// The lead's call (C2): FormatSubs stays at 0.5, the lowest that keeps §4.6's three-a-week floors (0.35 failed them: the
	// sweep in STATUS_c2-video.txt), and the Shorts-only grinder's ceilings are restated to what that buys: at most 4.5x the
	// long-form creator at day 365 (3.7x measured; 7.7x before the factor), and no Shorts-heavy creator at tier 4 before day
	// 400 (days 464 and 619 measured; 2.5 and 730 asked, which no factor reaches with the floors held).
	Expect(Median(ShortsS365) <= 4.5 * LongMedian, "VideoGrind: three Shorts a day, at most 4.5x the long-form creator's subscribers at day 365");
	// Update 3 (C3, SPEC_CREATORS §3.6's one re-pin): on Rerun 2.0 every plan grows about 1.4x (the streak, Fresh, Momentum),
	// Cuts no faster than the rest (3.0x the long-form creator against 3.7x), and the three-Cuts-a-day creator's tier 4 moves
	// from day 464 to about day 397: the ceiling is restated from day 400 to "not inside the first year".
	Expect(Median(ShortsTier4) >= 365.0 && Median(MixedTier4) >= 365.0, "VideoGrind: no Shorts-heavy creator reaches 50,000 subscribers (tier 4) inside its first year");
}

// ------------------------------------------------------------------ the session's calendar, the edit, the save

void PayoutsThroughTheCalendar()
{
	Hooks H;
	ss::Session S(H, "video-pay");
	double Now = 0.0;
	Lobby(S, Now);
	const ss::Chips Bank0 = S.BankrollCents;
	const std::string Granted = S.DebugVideo("grant 1000 4000 60000");
	Expect(Granted.rfind("Granted.", 0) == 0, "video pay: ss.Video.Grant adds subscribers, hours and a month of views");
	const double Start = S.WorldMinutes();
	std::vector<ss::life::Mark> Marks;
	bool SawMark = false;
	for (int Hour = 0; Hour < 60 * 24; ++Hour)
	{
		S.LobbyMinutes += 60.0;
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (!SawMark)
		{
			for (const ss::life::Mark& M : S.Marks(S.WorldMinutes(), S.WorldMinutes() + 40.0 * Day))
			{
				SawMark = SawMark || (M.Kind == video::MarkPayout && M.Label == "Rerun payout");
			}
		}
	}
	int Review = 0, Welcome = 0, Statements = 0;
	for (const ss::life::Letter& M : S.Life.Inbox)
	{
		Review += M.Source == "video" && M.Kind == video::LetterReview ? 1 : 0;
		Welcome += M.Source == "video" && M.Kind == video::LetterWelcome ? 1 : 0;
		Statements += M.Source == "video" && M.Kind == video::LetterStatement ? 1 : 0;
	}
	const ss::life::LedgerEntry* Paid = nullptr;
	int Lines = 0;
	for (const ss::life::LedgerEntry& E : S.Life.Ledger)
	{
		if (E.Kind == ss::life::LedgerVideo)
		{
			Paid = &E;
			++Lines;
		}
	}
	std::printf("  video pay: partner day %.0f, %d statements, paid %s on %s (%s)\n", (S.Life.Video.PartnerAt - Start) / Day, Statements,
		Paid ? ss::Money(Paid->Amount).c_str() : "nothing", Paid ? ss::net::DateLabel(ss::net::DayOf(Paid->At)).c_str() : "-", Paid ? Paid->Label.c_str() : "-");
	Expect(Review == 1 && Welcome == 1, "video pay: the review letter, then the Partner welcome, through CheckCalendar");
	Expect(Lines == 1 && Paid && Paid->Amount >= 10000 && Paid->Label.rfind("Rerun payout: ", 0) == 0, "video pay: one bank line, kind 13, \"Rerun payout: <month>\", $100 or more");
	int Y = 0, Mo = 0, Dm = 0;
	if (Paid)
	{
		ss::world::CivilDate(ss::net::DayOf(Paid->At), Y, Mo, Dm);
	}
	Expect(Dm == 21 && Statements >= 2, "video pay: paid on the 21st, with a statement (and the month before it rolled)");
	Expect(SawMark, "video pay: the wall calendar marks the payout day");
	Expect(S.Channel.FollowFrac > 0.0 || S.Channel.Followers > 0, "video pay: the views feed Kast's follows");
	(void)Bank0;
}

void EditThroughFinishSkip()
{
	Hooks H;
	ss::Session S(H, "video-edit");
	double Now = 0.0;
	Lobby(S, Now);
	S.Life.Energy = 80.0;
	S.Life.Hunger = 10.0;
	S.Life.Thirst = 10.0;
	Expect(S.DebugVideo("fake badbeat online").rfind("Faked ", 0) == 0 && S.Life.Video.Open.size() == 1, "video edit: ss.Video.Fake makes a candidate");
	video::Choice C;
	C.Candidates = {S.Life.Video.Open[0].Id};
	C.F = video::Format::Breakdown;
	C.T = video::TitleStyle::Story;
	C.Fr = video::Frame::Reaction;
	const std::string Preview = video::Title(C, S.Life.Video.Open, 0);
	// Too tired, then rested.
	S.Life.Energy = 6.0;
	Expect(!S.Publish(C).empty() && !S.TimeSkip.Active, "video edit: too tired to edit below 10 energy");
	S.Life.Energy = 80.0;
	const double W0 = S.WorldMinutes();
	const double E0 = S.Life.Energy;
	Expect(S.Publish(C).empty() && S.TimeSkip.Active && S.TimeSkip.Result.ActivityId.rfind("video:", 0) == 0, "video edit: PUBLISH starts the edit's time skip");
	Expect(!S.Publish(C).empty(), "video edit: one edit at a time");
	Now = Wait(S, Now, 6.0);
	Expect(!S.TimeSkip.Active && S.Life.Video.Videos.size() == 1 && S.Life.Video.Open.empty(), "video edit: the edit lands as a video, its moment used");
	Expect(std::fabs(S.WorldMinutes() - W0 - 90.0) < 2.0, "video edit: a Breakdown takes an hour and a half");
	Expect(std::fabs(S.Life.Energy - (E0 - 8.0)) < 1.0, "video edit: and 8 energy");
	Expect(S.HasOutcome && S.LastOutcome.Title == "Up on Rerun" && S.LastOutcome.Body.find(Preview) != std::string::npos && S.Life.Video.Videos[0].Title == Preview,
		"video edit: the result card names the video, and its title is Studio's preview");
	Expect(H.Last.Serialize().find("video\tvid\t") != std::string::npos, "video edit: the save keeps the video");
	// Dee's game as storytime: Dee texts once.
	const size_t Since = H.Inbox.size();
	S.DebugVideo("fake bluff dees");
	video::Choice D;
	D.Candidates = {S.Life.Video.Open.back().Id};
	D.F = video::Format::Short;
	D.T = video::TitleStyle::Story;
	Expect(S.Publish(D).empty(), "video edit: a storytime Short");
	Now = Wait(S, Now, 6.0);
	Expect(H.Got(Since, "Dee", "No names"), "video edit: Dee's one text about the storytime video");
	// Tables open: no editing.
	S.BankrollCents = 10000;
	S.Register(1);
	S.DebugVideo("fake bigpot online");
	video::Choice T;
	T.Candidates = {S.Life.Video.Open.back().Id};
	Expect(!S.Publish(T).empty() && !S.TimeSkip.Active, "video edit: not with tables open");
	// Deleting stops the views and keeps the subscribers.
	const double Subs = S.Life.Video.Subs;
	Expect(S.DeleteVideo(S.Life.Video.Videos[0].Id).empty() && S.Life.Video.Videos.size() == 1 && S.Life.Video.Subs == Subs, "video edit: deleting a video keeps its subscribers");
}

void StoriesAndSaves()
{
	// A save from before Rerun writes no video line, however long the calendar runs; a new career writes the channel.
	{
		Hooks H;
		ss::Session S(H, "video-old");
		double Now = 0.0;
		Lobby(S, Now);
		Now = Wait(S, Now, 2.0);
		S.Save();
		Expect(H.Last.Serialize().find("video\t") == std::string::npos, "video save: an untouched career writes no video line");
		Hooks Hn;
		ss::Session N(Hn, "video-new");
		N.NewCareer(N.Person);
		N.Save();
		const std::string Text = Hn.Last.Serialize();
		ss::SaveData Back;
		auto VideoLines = [](const std::string& All) {
			std::string Out;
			for (size_t At = All.find("video\t"); At != std::string::npos; At = All.find("\nvideo\t", At + 1))
			{
				const size_t From = All[At] == '\n' ? At + 1 : At;
				Out += All.substr(From, All.find('\n', From) - From) + "\n";
			}
			return Out;
		};
		Expect(Text.find("video\tch\t") != std::string::npos && ss::SaveData::Parse(Text, Back) && VideoLines(Back.Serialize()) == VideoLines(Text),
			"video save: a new career writes the channel, and it round-trips");
	}
	// The rent-paid story says poker paid it (S4, G2 audit): no story when it didn't. The rent paid from other money (a
	// bankroll with no poker in it, poker money from more than 30 days before, or a buy-in refunded) is told without a
	// candidate, once.
	for (const int Case : {0, 1, 2})
	{
		Hooks H;
		ss::Session S(H, Case == 0 ? "video-story-job" : Case == 1 ? "video-story-old" : "video-story-refund");
		double Now = 0.0;
		Lobby(S, Now);
		S.DebugVideo("fake bigpot online");
		S.BankrollCents = 500000;
		if (Case == 1)
		{
			S.Life.Record(S.WorldMinutes() - 31.0 * Day, "Prize: Night Owl Turbo", 200000, 0); // a month and a day before
		}
		if (Case == 2)
		{
			// A called-off event's buy-in back (Live.cpp's refund line, kind 0): the player's own money, not poker's winnings.
			S.Life.Record(S.WorldMinutes() - 3.0 * Day, "Embercrest Sunday: refunded buy-in (the room called it off)", 150000, 0);
		}
		S.Life.Record(S.WorldMinutes() - 2.0 * Day, "Prize: Hyper Sprint", 4000, 0); // poker, but nowhere near the rent
		Expect(S.PayRent(), "video story: the rent is paid from the bankroll");
		Now = Wait(S, Now, 1.0);
		const bool None = std::none_of(S.Life.Video.Open.begin(), S.Life.Video.Open.end(), [](const video::Candidate& C) { return C.Id == "story:rent-paid"; });
		Expect(None && S.Life.Video.Paid.count("story:rent-paid") == 1,
			Case == 0 ? "video story: rent poker didn't cover is no rent-paid story (told once, with no candidate)"
			: Case == 1 ? "video story: poker money from more than 30 days before the rent doesn't count toward it"
						: "video story: a refunded buy-in isn't poker money toward the rent");
		Now = Wait(S, Now, 1.0);
		Expect(std::none_of(S.Life.Video.Open.begin(), S.Life.Video.Open.end(), [](const video::Candidate& C) { return C.Id == "story:rent-paid"; }),
			"video story: and it doesn't come back");
	}
	// A life story: the first rent paid with poker money, once the channel has something.
	{
		Hooks H;
		ss::Session S(H, "video-story");
		double Now = 0.0;
		Lobby(S, Now);
		S.DebugVideo("fake bigpot online");
		S.BankrollCents = 500000;
		S.Life.Record(S.WorldMinutes() - 6.0 * Day, "Prize: Night Owl Turbo", 90000, 0); // poker's month: $900 and $400,
		S.Life.Record(S.WorldMinutes() - 2.0 * Day, "Night Shift: 3rd", 40000, 4);       // $1,300 for the first rent's $1,225
		Expect(S.PayRent(), "video story: the rent is paid");
		Now = Wait(S, Now, 1.0);
		const std::vector<video::Candidate> Open = S.VideoCandidates();
		const auto It = std::find_if(Open.begin(), Open.end(), [](const video::Candidate& C) { return C.Id == "story:rent-paid"; });
		Expect(It != Open.end() && It->K == video::Kind::Story && It->Source == 4, "video story: the first rent paid is a story candidate");
		if (It != Open.end())
		{
			video::Choice C;
			C.Candidates = {It->Id};
			C.F = video::Format::Short;
			Expect(video::Title(C, Open, 0) == "I paid rent with poker money", "video story: its Story title tells it");
		}
		Now = Wait(S, Now, 1.0);
		Expect(std::count_if(S.Life.Video.Open.begin(), S.Life.Video.Open.end(), [](const video::Candidate& C) { return C.Id == "story:rent-paid"; }) == 1,
			"video story: told once");
		const std::string Describe = S.DebugVideo("describe");
		Expect(Describe.find("Gate: subscribers") != std::string::npos && S.DebugVideo("candidates").find("story:rent-paid") != std::string::npos,
			"video debug: ss.Video.Describe and .Candidates read the channel");
	}
}

void FakesDealTrue()
{
	// C2's verify: ss.Video.Fake's scripted hands (PIE §8.2 step 2, VC1) deal what their records say. A hand turned up on a
	// full board wins or loses as the cards play it: the cooler's kings make a set under the aces' set (it lost), the
	// suckout's ace-jack catches two pair past the queens (it won); the frames draw these cards next to the titles.
	int Checked = 0, Wrong = 0;
	for (const char* Kind : {"badbeat", "suckout", "cooler", "bubble", "knockout", "rival"})
	{
		for (const char* Source : {"online", "dees"})
		{
			Hooks H;
			ss::Session S(H, std::string("video-fake-true-") + Kind + Source);
			double Now = 0.0;
			Lobby(S, Now);
			S.DebugVideo(std::string("fake ") + Kind + " " + Source);
			if (S.Life.Video.Open.empty())
			{
				continue;
			}
			const ss::recall::Record& R = S.Life.Video.Open.back().Rec;
			if (R.Shown.empty() || R.Board.size() != 5 || R.Mine.size() != 2)
			{
				continue;
			}
			const int Mine = ss::EvaluateHand(R.Mine, R.Board);
			const int Theirs = ss::EvaluateHand(R.Shown.front().second, R.Board);
			const bool Ok = (R.HeroNet > 0 && Mine > Theirs) || (R.HeroNet < 0 && Mine < Theirs);
			++Checked;
			Wrong += Ok ? 0 : 1;
			if (!Ok)
			{
				std::printf("  video fake %s %s: the player's %s against %s, yet the record's net is %lld\n", Kind, Source, ss::Describe(Mine).c_str(), ss::Describe(Theirs).c_str(),
					static_cast<long long>(R.HeroNet));
			}
		}
	}
	Expect(Checked >= 10 && Wrong == 0, "video fake: every scripted showdown wins or loses as its cards play");
}
} // namespace video_seg

void SegVideo()
{
	const auto T0 = std::chrono::steady_clock::now();
	video_seg::CaptureDrawsNothing();
	video_seg::WatchedBadBeat();
	video_seg::BubbleHands();
	video_seg::SkipCapture();
	video_seg::VideoGrind();
	video_seg::PayoutsThroughTheCalendar();
	video_seg::EditThroughFinishSkip();
	video_seg::StoriesAndSaves();
	video_seg::FakesDealTrue();
	std::printf("  video: core-video's session checks in %.1f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}
} // namespace session_test
