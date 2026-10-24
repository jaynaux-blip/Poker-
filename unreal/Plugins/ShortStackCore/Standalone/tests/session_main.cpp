// Plays whole tournaments through the Night One session (the layer the Unreal
// client drives), with a scripted hero, and checks the flow end to end.
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"
#include "SessionSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <vector>

namespace session_test
{
int Failures = 0;

void Expect(bool Condition, const char* What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

// Hooks (the suite's SessionHooks) lives in SessionSeg.h, which the segment's per-track files share.

/** Runs until the results screen; the hero calls, raises or folds at random. */
double PlayOut(ss::Session& S, ss::Rng& Choice, double Now, int& Decisions)
{
	const double Dt = 1.0 / 30.0;
	int Frames = 0;
	while (S.CurrentScreen == ss::Screen::Table && ++Frames < 4000000)
	{
		Now += Dt;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.5)
		{
			++Decisions;
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

void FullTournament()
{
	Hooks H;
	ss::Session S(H, "session-test");
	S.OnBoot();
	Expect(H.Texts == 1, "intro text on boot");
	S.CurrentScreen = ss::Screen::Lobby;
	const ss::Chips Before = S.BankrollCents;
	S.Register(1); // $0.25 Hyper Sprint, 180 players
	Expect(S.CurrentScreen == ss::Screen::Table, "registering opens the table");
	Expect(S.BankrollCents == Before - 25, "buy-in deducted");
	Expect(S.CurHand != nullptr, "first hand dealt");
	ss::Rng Choice("hero-choices");
	int Decisions = 0;
	const double End = PlayOut(S, Choice, 0.0, Decisions);
	Expect(S.CurrentScreen == ss::Screen::Results, "tournament reaches the results screen");
	Expect(S.HasResults, "results recorded");
	const ss::Results& R = S.LastResults;
	std::printf("  hyper: finished %s of %d after %d hands, %d decisions graded (accuracy %.1f), %.0f s game time, prize %s\n",
		ss::Ordinal(R.Place).c_str(), R.Entrants, R.Hands, static_cast<int>(R.Grades.size()), R.AccuracyPct, End, ss::Money(R.PrizeCents).c_str());
	Expect(R.Place >= 1 && R.Place <= 180, "valid finishing place");
	Expect(static_cast<int>(R.Grades.size()) <= Decisions, "every graded decision was a hero decision");
	Expect(S.BankrollCents == Before - 25 + R.PrizeCents, "bankroll settles with the prize");
	Expect(!S.History.empty() && S.History[0].Place == R.Place, "history updated");
	Expect(H.Saves > 0 && H.Last.BankrollCents == S.BankrollCents, "session saved");
	Expect(H.Sounds[ss::SoundId::Deal] > 0 && H.Sounds[ss::SoundId::ChipStack] > 0, "hand sounds fired");
	int Dealer = 0;
	for (const ss::ChatLine& L : S.Chat)
	{
		Dealer += L.Kind == ss::ChatKind::Dealer ? 1 : 0;
	}
	Expect(Dealer > 10, "dealer chat");

	// Save round trip.
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed), "save parses");
	Expect(Parsed.BankrollCents == H.Last.BankrollCents && Parsed.HeroName == H.Last.HeroName && Parsed.History.size() == H.Last.History.size() && Parsed.TextsSeen == H.Last.TextsSeen, "save round trip");
	ss::Session Reloaded(H, "reload", &Parsed);
	Expect(Reloaded.BankrollCents == S.BankrollCents && Reloaded.History.size() == S.History.size(), "session restores from save");

	S.LeaveResults();
	Expect(S.CurrentScreen == ss::Screen::Lobby && !S.T, "back to lobby");
}

void SprintTournament()
{
	Hooks H;
	ss::Session S(H, "sprint-test");
	S.CurrentScreen = ss::Screen::Lobby;
	S.Register(2); // Midnight Freeroll, 1,000 players
	S.CurrentPace = ss::Pace::Sprint;
	ss::Rng Choice("sprint-choices");
	int Decisions = 0;
	int Sprinted = 0; // decisions put to the hero while the sprint was still on: none, ever
	double Now = 0.0;
	// PlayOut's loop, counting the prompts by pace: a sprint plays by itself until it stops (approaching the bubble, the
	// final table); a deep run hands the seat back there, and those decisions are the hero's (the 2026-10-20 limp fix
	// sent this freeroll's hero past the bubble).
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.5)
		{
			++Decisions;
			Sprinted += S.CurrentPace == ss::Pace::Sprint ? 1 : 0;
			const double Roll = Choice.Next();
			S.HeroAct(Roll < 0.15 && S.Prompt.CanRaise ? ss::PlayerAction::RaiseTo(static_cast<double>(S.Prompt.RaiseTo))
					  : Roll < 0.75 || S.Prompt.CanCheck ? (S.Prompt.CanCheck ? ss::PlayerAction::Check() : ss::PlayerAction::Call())
														  : ss::PlayerAction::Fold());
		}
	}
	Expect(S.CurrentScreen == ss::Screen::Results, "sprint tournament reaches results");
	std::printf("  freeroll with sprint: finished %s of %d after %d hands (%d hero decisions, %d while sprinting; stopped: %s), cans %d\n",
		ss::Ordinal(S.LastResults.Place).c_str(), S.LastResults.Entrants, S.LastResults.Hands, Decisions, Sprinted,
		S.SprintStopReason.empty() ? "no" : S.SprintStopReason.c_str(), H.Cans);
	Expect(S.LastResults.Hands > 5 && Sprinted == 0 && (Decisions == 0 || !S.SprintStopReason.empty()), "sprint plays the hands by itself until it stops");
}

/** A hero that always takes the grader's best option: goes deep, so the bubble and final table get exercised. */
void DeepRuns()
{
	int Cashes = 0;
	int Wins = 0;
	for (int Seed = 0; Seed < 6; ++Seed)
	{
		Hooks H;
		ss::Session S(H, "deep-" + std::to_string(Seed));
		S.CurrentScreen = ss::Screen::Lobby;
		S.BankrollCents = 10000;
		S.Register(1);
		double Now = 0.0;
		int Frames = 0;
		bool SawBubble = false;
		bool SawFinal = false;
		while (S.CurrentScreen == ss::Screen::Table && ++Frames < 6000000)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
			if (S.CurrentBanner.Active)
			{
				SawBubble = SawBubble || S.CurrentBanner.Title == "THE BUBBLE" || S.CurrentBanner.Title == "IN THE MONEY";
				SawFinal = SawFinal || S.CurrentBanner.Title == "FINAL TABLE";
			}
			if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
			{
				const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
				size_t Best = 0;
				for (size_t I = 1; I < Opts.size(); ++I)
				{
					Best = Opts[I].Ev > Opts[Best].Ev ? I : Best;
				}
				S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
			}
		}
		Expect(S.CurrentScreen == ss::Screen::Results, "deep run reaches results");
		const ss::Results& R = S.LastResults;
		Cashes += R.PrizeCents > 0 ? 1 : 0;
		Wins += R.Won ? 1 : 0;
		std::printf("  deep %d: %s of %d, %d hands, accuracy %.1f, prize %s, bubble %s, final %s, texts %d, cans %d\n", Seed, ss::Ordinal(R.Place).c_str(), R.Entrants, R.Hands,
			R.AccuracyPct, ss::Money(R.PrizeCents).c_str(), SawBubble ? "yes" : "no", SawFinal ? "yes" : "no", H.Texts, H.Cans);
	}
	std::printf("  deep runs: %d cashes, %d wins of 6\n", Cashes, Wins);
}

/** The simulated network: tonight's schedule at 2:07 AM, fees, locks, boards, and the player's results in it. */
void NetworkChecks()
{
	namespace net = ss::net;
	const net::Network Net;
	const net::Network Again;
	Expect(Net.Players().size() == 1600 && Net.Players()[0].Name == Again.Players()[0].Name && Net.Players()[Net.RivalIndex()].Name == ss::RivalName, "network players are fixed");
	const double Now = net::MinutesPerDay + 127.0;
	auto Find = [&](const std::string& Id, net::EventInstance& Out) { return Net.FindInstance(Id, Out); };
	net::EventInstance Owl;
	net::EventInstance Crawler;
	net::EventInstance Free;
	Expect(Find("night-owl@1560", Owl) && Net.Live(Owl, Now).St == net::Status::LateReg, "the 2:00 Night Owl is in late registration at 2:07");
	Expect(Find("mm-26@1530", Crawler) && Net.Live(Crawler, Now).St == net::Status::LateReg, "MM #26 is in late registration at 2:07");
	Expect(Find("freeroll@1545", Free) && Net.Live(Free, Now).St == net::Status::LateReg, "the freeroll is in late registration at 2:07");
	const net::LiveState Lc = Net.Live(Crawler, Now);
	Expect(Lc.Entries > 0 && Lc.Entries <= Crawler.Entries && Lc.LateRegLeft > 60.0 && Lc.LateRegLeft < 64.0, "late registration counts down to 3:10");
	int Events = 0;
	bool PoolsOk = true;
	for (const net::EventInstance& E : Net.Window(Now - 12.0 * 60.0, Now + 24.0 * 60.0))
	{
		const net::EventTemplate& T = Net.TemplateOf(E);
		PoolsOk = PoolsOk && E.Entries > 0 && E.Duration > 0.0 && E.LateReg <= E.Duration && (T.BuyInCents == 0 || E.Pool >= T.GtdCents);
		++Events;
	}
	std::printf("  network: %d events in the 36-hour window\n", Events);
	Expect(Events > 200 && PoolsOk, "the schedule is full and every pool covers its guarantee");
	net::EventInstance Main;
	Expect(Net.Next("mm-main", Now, Main) && net::DayOf(Main.Start) == 6 && Net.Live(Main, Now).St == net::Status::Registering, "the MM Main Event is open on Sunday");
	Expect(net::FeeOf(110) == 10 && net::FeeOf(220) == 20 && net::FeeOf(1080) == 80 && net::FeeOf(21500) == 1500 && net::FeeOf(525000) == 25000 && net::FeeOf(10900) == 900, "fees split like a lobby");
	std::string Lock;
	net::EventInstance Pko;
	Expect(Find("hh-110@1500", Pko) && !Net.Joinable(Pko, &Lock) && !Lock.empty(), "bounty events are locked with a reason");
	const ss::LobbyEvent L = Net.Listing(Crawler);
	Expect(L.Joinable && L.Spec.Id == Crawler.Id && L.Spec.BuyInCents == 220 && L.Spec.FeeCents == 20 && L.Spec.StartClock == 90.0, "a story event's listing keeps its spec");
	net::EventInstance Grind;
	Expect(Find("grind-330@1530", Grind), "the $3.30 Daily Grind is scheduled");
	const ss::LobbyEvent G = Net.Listing(Grind);
	Expect(G.Joinable && G.Spec.Entrants == Grind.Entries && G.Spec.FeeCents == 30 && G.Spec.Population == "micro" && G.Spec.Levels.empty() == (G.Spec.StartingStack <= 10000), "a scheduled freezeout gets a playable spec");
	// Boards.
	net::HeroStats Nobody;
	Nobody.Name = "grinder_3c";
	net::BoardRow Mine;
	const std::vector<net::BoardRow> Night = Net.Leaderboard(net::Board::NightShift, Now, Nobody, 25, &Mine);
	bool Sorted = Night.size() == 25;
	ss::Chips Prizes = 0;
	for (size_t I = 0; I < Night.size(); ++I)
	{
		Sorted = Sorted && Night[I].Rank == static_cast<int>(I) + 1 && (I == 0 || Night[I].Value <= Night[I - 1].Value);
		Prizes += Night[I].Prize;
	}
	Expect(Sorted && Prizes == 100000 && Mine.Rank == 0, "the Night Shift board ranks and pays $1,000 to the top 20");
	// The player's win goes into that final table and the boards.
	net::Network Mutable;
	ss::HistoryEntry Won;
	Won.Name = "MM #26: Night Crawler";
	Won.Place = 1;
	Won.Entrants = 1184;
	Won.Prize = 45000;
	Won.BuyInCents = 220;
	Won.EventId = Crawler.Id;
	Mutable.SetHero("grinder_3c", {Won});
	const net::EventResult& Res = Mutable.Result(Crawler);
	Expect(!Res.FinalTable.empty() && Res.FinalTable[0].Player == -1 && Res.FinalTable[0].Prize == 45000, "the player's win is in the final table");
	const net::HeroStats Hero = net::StatsFrom("grinder_3c", {Won});
	Expect(Hero.Wins == 1 && Hero.SeriesTitles == 1 && Hero.NightPoints > 0.0 && Hero.SeriesPoints == Hero.SeasonPoints, "stats from history");
	Mutable.Leaderboard(net::Board::NightShift, Now + 120.0, Hero, 25, &Mine);
	Expect(Mine.Rank >= 1, "a series title puts the player on the Night Shift board");
	std::printf("  network: after a $450 MM #26 win the player is #%d on the Night Shift board (%s)\n", Mine.Rank, ss::Money(Mine.Prize).c_str());
	const std::vector<net::NewsItem> News = Mutable.News(Crawler.Start + Crawler.Duration + 30.0, Hero, 10);
	bool HeroNews = false;
	for (const net::NewsItem& It : News)
	{
		HeroNews = HeroNews || (It.Kind == net::NewsKind::Hero && It.Title.find("grinder_3c wins") == 0);
	}
	Expect(HeroNews, "the player's title makes the news");
}

/** Lobby clock, registering for a scheduled event, and the result carried back into the network. */
void ScheduledTournament()
{
	Hooks H;
	ss::Session S(H, "scheduled");
	S.CurrentScreen = ss::Screen::Lobby;
	for (int I = 0; I <= 120 * 30; ++I)
	{
		S.Update(static_cast<double>(I) / 30.0);
	}
	Expect(S.LobbyMinutes > 128.9 && S.LobbyMinutes < 129.1 && S.ClockMinutes() == S.LobbyMinutes, "the lobby clock runs in real time");
	ss::net::EventInstance Hyper;
	const ss::net::Network& Net = ss::net::Shared();
	Expect(Net.FindInstance("hyper-sprint@1590", Hyper), "the 2:30 hyper sprint is scheduled");
	S.RegisterEvent(Net.Listing(Hyper));
	Expect(S.CurrentScreen == ss::Screen::Table && S.T && S.T->Spec.Id == Hyper.Id && S.T->Spec.StartClock == 150.0, "registering early waits for the start");
	S.CurrentPace = ss::Pace::Sprint;
	ss::Rng Choice("scheduled-choices");
	int Decisions = 0;
	PlayOut(S, Choice, 200.0, Decisions);
	Expect(S.CurrentScreen == ss::Screen::Results && !S.History.empty() && S.History[0].EventId == Hyper.Id && S.History[0].BuyInCents == 25, "the result keeps its event");
	const double End = S.T->ClockMinutes();
	S.LeaveResults();
	Expect(S.LobbyMinutes == End, "the lobby picks up the clock where the tournament ended");
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed) && Parsed.History[0].EventId == Hyper.Id && Parsed.History[0].BuyInCents == 25 && Parsed.ClockMinutes > 150.0, "event ids and the clock survive a save");
	ss::SaveData Old;
	Expect(ss::SaveData::Parse("shortstack.nightone.v1\nbankroll\t500\nresult\tNight Owl Turbo\t12\t1000\t300\t80.000\n", Old) && Old.History.size() == 1 && Old.History[0].BuyInCents == -1 && Old.ClockMinutes == 127.0,
		"older saves still load");
	std::printf("  scheduled: %s in the 2:30 hyper sprint, back in the lobby at %s\n", ss::Ordinal(S.History[0].Place).c_str(), ss::ClockString(S.LobbyMinutes).c_str());
}

/** Runs the session for some real seconds (the lobby clock, time skips and the calendar move). */
double Wait(ss::Session& S, double Now, double Seconds)
{
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}

/** Dee's game: the nights it runs, what it takes to sit, the trip out, and what a night leaves in the save. */
void DeeGame()
{
	struct OutHooks : Hooks
	{
		std::string Went;
		ss::Chips BuyIn = 0;
		int FromDee = 0;
		void Text(const std::string& From, const std::string& Body) override
		{
			Hooks::Text(From, Body);
			FromDee += From == "Dee" ? 1 : 0;
		}
		bool GoOut(const std::string& Id, ss::Chips Cents) override
		{
			Went = Id;
			BuyIn = Cents;
			return true;
		}
	};
	namespace life = ss::life;
	OutHooks H;
	ss::Session S(H, "dee");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	const life::Activity* G = life::Find("dee-game");
	Expect(G != nullptr && G->Type == life::Kind::Game, "Dee's game is in the catalog");
	if (!G)
	{
		return;
	}
	Expect(!life::InWindow(*G, S.WorldMinutes()), "2:07 AM Tuesday is Monday night: no game");
	Expect(life::InWindow(*G, 1440.0 + 22.0 * 60.0), "Tuesday 10 PM, the game is on");
	Expect(life::InWindow(*G, 2.0 * 1440.0 + 2.0 * 60.0), "2 AM Wednesday is still Tuesday's game");
	Expect(!life::InWindow(*G, 2.0 * 1440.0 + 22.0 * 60.0), "no game on Wednesday night");
	Expect(life::InWindow(*G, 3.0 * 1440.0 + 21.0 * 60.0) && life::InWindow(*G, 5.0 * 1440.0 + 23.0 * 60.0), "Thursday and Saturday nights");
	Expect(life::NextOpen(*G, S.WorldMinutes()) == 1440.0 + 21.0 * 60.0, "the next game is Tuesday at nine");
	Expect(life::NextOpen(*G, 2.0 * 1440.0 + 12.0 * 60.0) == 3.0 * 1440.0 + 21.0 * 60.0, "from Wednesday, the next game is Thursday");
	Expect(S.GoToGame("dee-game", 4000).find("$40") != std::string::npos, "$2.37 can't sit");
	S.BankrollCents = 30000;
	S.Life.Energy = 60.0;
	Expect(S.GoToGame("dee-game", 4000) == "Tonight 9:00 PM", "closed until nine");
	// Dee's heads-up at 8:30 on a game night.
	S.LobbyMinutes = 20.0 * 60.0 + 29.0;
	Now = Wait(S, Now, 0.2);
	Expect(H.FromDee == 0, "no text before 8:30");
	S.LobbyMinutes = 20.0 * 60.0 + 31.0;
	Now = Wait(S, Now, 0.2);
	Expect(H.FromDee == 1, "Dee texts half an hour before the doors");
	S.LobbyMinutes = 22.0 * 60.0;
	Now = Wait(S, Now, 0.2);
	S.Life.Energy = 60.0;
	Expect(S.GoToGame("dee-game", 25000) == "Pick a buy-in you can cover.", "the buy-in is $40 to $200");
	Expect(S.StartActivity("dee-game") == "Pick a buy-in first.", "the game isn't a time skip");
	const int SavesBefore = H.Saves;
	Expect(S.GoToGame("dee-game", 10000).empty() && H.Went == "dee-game" && H.BuyIn == 10000 && H.Saves > SavesBefore, "sitting with $100 saves and heads out");
	S.Life.Energy = 5.0;
	Expect(S.GoToGame("dee-game", 4000).find("Too tired") == 0, "too tired to sit");
	// A night as the Back Room writes it into the save.
	ss::SaveData D;
	D.BankrollCents = 23400;
	D.Life.BackRoomNights = 1;
	D.Life.BackRoomNetCents = -1200;
	D.Life.Reads["Sal/ChipGlance"] = 2;
	D.Life.Reads["Twitch/Swallow"] = 1;
	D.Life.Record(1440.0 + 23.0 * 60.0, "Dee's game", -1200, 0);
	ss::SaveData P;
	Expect(ss::SaveData::Parse(D.Serialize(), P) && P.Life.BackRoomNights == 1 && P.Life.BackRoomNetCents == -1200 && P.Life.Reads.size() == 2 && P.Life.Reads["Sal/ChipGlance"] == 2 &&
			   P.Life.Ledger.size() == 1 && P.Life.Ledger[0].Label == "Dee's game",
		"a night at Dee's survives a save");
	std::printf("  dee: %d text(s), sat with %s\n", H.FromDee, ss::Money(H.BuyIn).c_str());
}

/** The Embercrest's card room: the schedule, registering once, the world's field, breaks, settling once, saves. */
void Embercrest()
{
	namespace life = ss::life;
	namespace live = ss::live;
	const life::Activity* Rv = life::Find("embercrest");
	Expect(Rv != nullptr && Rv->Type == life::Kind::Live, "the Embercrest is in the catalog");
	if (!Rv)
	{
		return;
	}

	// The schedule, eight weeks of it.
	{
		bool Counts = true;
		bool Times = true;
		bool Fields = true;
		bool Unique = true;
		std::set<std::string> Ids;
		for (int Day = 0; Day < 56; ++Day)
		{
			const std::vector<live::Occurrence> Os = live::Occurrences(Day);
			const int Wd = Day % 7;
			Counts = Counts && static_cast<int>(Os.size()) == (Wd == 4 || Wd == 5 ? 3 : 2);
			for (const live::Occurrence& O : Os)
			{
				Unique = Unique && Ids.insert(O.Id).second;
				Fields = Fields && O.Field >= 60 && O.Field <= 120 && O.Field >= O.T->FieldLo && O.Field <= O.T->FieldHi;
				Times = Times && O.Start == Day * 1440.0 + O.T->StartMinute && live::FindOccurrence(O.Id).Id == O.Id && O.LateRegEnds > O.Start;
				if (std::string(O.T->Key) == "sunday")
				{
					Times = Times && O.Id == "embercrest@" + std::to_string(Day);
				}
			}
		}
		Expect(Counts, "two events a day Monday to Thursday and Sunday, three on Friday and Saturday");
		Expect(Unique && Times, "every occurrence has its own id and its published start; the Sunday keeps embercrest@<day>");
		Expect(Fields, "planned fields of 60 to 120");
	}

	struct OutHooks : Hooks
	{
		std::string Went;
		ss::Chips BuyIn = 0;
		bool GoOut(const std::string& Id, ss::Chips Cents) override
		{
			Went = Id;
			BuyIn = Cents;
			return true;
		}
	};
	OutHooks H;
	ss::Session S(H, "embercrest");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	// Wednesday, October 14 (world day 9), 5:30 PM: the Nightly's desk is open.
	const double Wednesday = 9.0 * 1440.0;
	S.LobbyMinutes = Wednesday - 1440.0 + 17.0 * 60.0 + 30.0;
	Now = Wait(S, Now, 0.2);
	S.Life.Energy = 60.0;
	const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@9");
	Expect(Nightly.Valid() && Nightly.T->BuyInCents == 12000, "Wednesday's Nightly, $120");
	S.BankrollCents = 12000;
	Expect(S.GoToLive(Nightly.Id).find("bus") != std::string::npos && S.Life.LiveEntries.empty(), "the buy-in without the bus can't enter");
	S.BankrollCents = 50000;
	const size_t Lines0 = S.Life.Ledger.size();
	Expect(S.GoToLive(Nightly.Id).empty() && H.Went == Nightly.Id && H.BuyIn == 12000, "registering heads out to the Nightly");
	Expect(S.BankrollCents == 50000 - 12000 - live::BusFareCents && S.Life.Ledger.size() == Lines0 + 3, "the buy-in, the fee and the bus: three lines, charged once");
	const life::LiveEntry* E = live::EntryFor(S.Life, Nightly.Id);
	Expect(E && E->State == life::LiveEntry::Registered && E->Entrants >= 61 && E->Entrants <= 101, "an entry with the world's field, the player included");
	if (!E)
	{
		return;
	}
	// The faces in the field: the room's regulars by name (and anyone who remembers the player), each in the roster.
	{
		bool InRoster = true;
		bool Labelled = true;
		int Locals = 0;
		std::string Who;
		for (const life::LiveFace& F : E->Faces)
		{
			bool Found = false;
			for (const std::pair<std::string, int>& R : E->Roster)
			{
				Found = Found || R.first == F.Name;
			}
			InRoster = InRoster && Found;
			Labelled = Labelled && !F.Label.empty();
			Locals += live::FindRoomLocal(F.Name) ? 1 : 0;
			Who += (Who.empty() ? "" : ", ") + F.Name + " (" + F.Label + ")";
		}
		Expect(E->Faces.size() >= 3 && InRoster && Labelled && Locals >= 1, "the Nightly's regulars are in its field, by name");
		std::printf("  embercrest: faces in the Nightly: %s\n", Who.c_str());
	}
	H.Went.clear();
	Expect(S.GoToLive(Nightly.Id).empty() && H.Went == Nightly.Id && S.BankrollCents == 50000 - 12000 - live::BusFareCents && S.Life.LiveEntries.size() == 1 &&
			   S.Life.Ledger.size() == Lines0 + 3,
		"registering again charges nothing and goes back");
	Expect(!S.GoToLive("embercrest-turbo@11").empty() && S.Life.LiveEntries.size() == 1, "one live entry at a time");

	// The field: the world's people by name among the room's regulars, and the same draw every time it's opened.
	std::vector<ss::ReservedPlayer> Known;
	for (const std::pair<std::string, int>& R : E->Roster)
	{
		ss::ReservedPlayer P;
		P.Name = R.first;
		P.Type = static_cast<ss::Archetype>(R.second);
		Known.push_back(P);
	}
	std::unique_ptr<ss::Tournament> T = live::MakeField(Nightly, E->Entrants, "grinder_3c", live::SeedFor(*E, "grinder_3c"), Known);
	std::unique_ptr<ss::Tournament> Twin = live::MakeField(Nightly, E->Entrants, "grinder_3c", live::SeedFor(*E, "grinder_3c"), Known);
	int Named = 0;
	bool SameDraw = T->Players.size() == Twin->Players.size();
	for (size_t I = 0; I < T->Players.size(); ++I)
	{
		for (const std::pair<std::string, int>& R : E->Roster)
		{
			Named += T->Players[I].Name == R.first ? 1 : 0;
		}
		SameDraw = SameDraw && I < Twin->Players.size() && T->Players[I].TableId == Twin->Players[I].TableId && T->Players[I].Seat == Twin->Players[I].Seat;
	}
	Expect(T->Spec.Entrants == E->Entrants && static_cast<int>(T->Players.size()) == E->Entrants && Named == static_cast<int>(E->Roster.size()) && T->TableSize == 6,
		"the field is the world's people and the room's regulars, 6-max");
	Expect(SameDraw, "the same entry draws the same seats");

	// A whole night: only balancing moves anyone, breaks fall between levels, the chips are all still there.
	bool Balanced = true;
	bool Conserved = true;
	bool BreaksOk = true;
	int Breaks = 0;
	int Moves = 0;
	int Ticks = 0;
	const ss::Chips Total = static_cast<ss::Chips>(T->Spec.Entrants) * T->Spec.StartingStack;
	ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
	while (!T->bFinished && Ticks < 6000)
	{
		int Lo = 1 << 30;
		int Hi = 0;
		for (const auto& It : T->Tables)
		{
			int N = 0;
			for (int Idx : It.second.Seats)
			{
				N += Idx >= 0 ? 1 : 0;
			}
			Lo = std::min(Lo, N);
			Hi = std::max(Hi, N);
		}
		std::vector<ss::TEvent> Events;
		std::unique_ptr<ss::Hand> Hand = T->StartTick(Events);
		bool Broke = false;
		int MovedNow = 0;
		for (const ss::TEvent& Ev : Events)
		{
			Broke = Broke || Ev.Type == ss::TEventType::TableBroken;
			MovedNow += Ev.Type == ss::TEventType::Moved ? 1 : 0;
		}
		Moves += MovedNow;
		Balanced = Balanced && (MovedNow == 0 || Broke || Hi - Lo >= 2);
		for (const ss::TEvent& Ev : T->FinishTick(Hand.get(), &Auto))
		{
			if (Ev.Type == ss::TEventType::Break)
			{
				++Breaks;
				BreaksOk = BreaksOk && (Ev.LevelNumber - 1) % Nightly.T->BreakEvery == 0;
			}
		}
		ss::Chips Sum = 0;
		for (const ss::TPlayer& P : T->Players)
		{
			Sum += P.Stack;
		}
		Conserved = Conserved && Sum == Total;
		++Ticks;
	}
	const double Clock = T->Spec.StartClock + T->ElapsedSeconds() / 60.0 + static_cast<double>(Breaks) * Nightly.T->BreakMinutes;
	Expect(T->bFinished, "the night plays out to a winner");
	Expect(Balanced && Moves > 0, "nobody is moved but by breaking or evening out tables");
	Expect(BreaksOk && Breaks >= 1 && Breaks == T->BreaksTaken && std::fabs(T->ClockMinutes() - Clock) < 1e-6, "breaks fall between levels and go on the clock");
	Expect(Conserved, "every chip is still on the tables");
	std::printf("  embercrest: %s, %d entrants, %d hands, %d moves, %d breaks, done at %s\n", Nightly.T->Name, T->Spec.Entrants, Ticks, Moves, Breaks,
		ss::net::TimeLabel(T->ClockMinutes()).c_str());

	// What the screens and the room ask of the schedule: what's still open, the last call, how long a deep run takes.
	{
		const live::Occurrence Noon10 = live::FindOccurrence("embercrest-noon@10");
		Expect(live::NextOpen(Nightly.Start - 120.0).Id == Nightly.Id && live::NextOpen(Nightly.Start + 30.0, 0.0).Id == Nightly.Id &&
				   live::NextOpen(Nightly.Start + 30.0, 0.0, Nightly.Id).Id == Noon10.Id && live::NextOpen(Nightly.LateRegEnds - 5.0).Id == Noon10.Id,
			"the next event still open: from home (the bus included), from the room, and past the one the player is in");
		Expect(live::LateRegLeft(Nightly, Nightly.Start) == 3.0 * Nightly.T->LevelMinutes && live::LastCall(Nightly, Nightly.LateRegEnds - 10.0) &&
				   !live::LastCall(Nightly, Nightly.LateRegEnds - 30.0) && !live::LastCall(Nightly, Nightly.LateRegEnds + 1.0),
			"late registration's last call is its last fifteen minutes");
		Expect(live::DayWord(Nightly, Wednesday + 600.0) == "tonight" && live::DayWord(Nightly, Wednesday - 600.0) == "tomorrow night" &&
				   live::DayWord(Nightly, Wednesday + 1440.0 + 600.0) == "last night" && live::DayWord(Noon10, Wednesday + 600.0) == "tomorrow" &&
				   live::DayWord(Nightly, Wednesday - 3.0 * 1440.0) == "on Wednesday",
			"when an event is, said the way people say it");
		const double Took = T->ClockMinutes() - T->Spec.StartClock;
		Expect(std::fabs(live::DeepRunMinutes(Nightly, E->Entrants) - Took) <= 0.15 * Took, "a deep run's length is about what the engine's night takes");
		// Every event of the week, played to its last hand.
		std::set<std::string> Keys;
		std::string Worst;
		double WorstOff = 0.0;
		for (int Day = 140; Day < 147; ++Day)
		{
			for (const live::Occurrence& O : live::Occurrences(Day))
			{
				if (!Keys.insert(O.T->Key).second)
				{
					continue;
				}
				std::unique_ptr<ss::Tournament> Deep = live::MakeField(O, O.Field + 1, "grinder_3c", O.Id + ":deep", {});
				for (int K = 0; K < 6000 && !Deep->bFinished; ++K)
				{
					std::vector<ss::TEvent> Ev;
					std::unique_ptr<ss::Hand> Hand = Deep->StartTick(Ev);
					Deep->FinishTick(Hand.get(), &Auto);
				}
				const double Played = Deep->ClockMinutes() - Deep->Spec.StartClock;
				const double Off = std::fabs(live::DeepRunMinutes(O, O.Field + 1) - Played) / Played;
				if (Off > WorstOff)
				{
					WorstOff = Off;
					Worst = std::string(O.T->Name) + ": said " + ss::Fixed(live::DeepRunMinutes(O, O.Field + 1) / 60.0, 1) + "h, played " + ss::Fixed(Played / 60.0, 1) + "h";
				}
			}
		}
		std::printf("  embercrest: deep runs as the card says them, worst %s\n", Worst.c_str());
		Expect(Keys.size() == live::Schedule().size() && WorstOff <= 0.2, "every event's deep run is about what the engine takes");
	}

	// Settling: once, however often it's asked for, and the save keeps it.
	{
		const ss::Chips Before = S.BankrollCents;
		const double At = Wednesday + 23.0 * 60.0;
		Expect(live::Settle(S.BankrollCents, S.Life, Nightly.Id, 7, E->Entrants, 41000, At) && !live::Settle(S.BankrollCents, S.Life, Nightly.Id, 7, E->Entrants, 41000, At) &&
				   S.BankrollCents == Before + 41000 && S.Life.LiveEvents == 1 && S.Life.LiveCashes == 1 && S.Life.LiveBestPlace == 7,
			"a result settles once");
		Expect(!live::Refund(S.BankrollCents, S.Life, Nightly.Id, At, "late") && S.BankrollCents == Before + 41000, "a settled entry can't be refunded");
		Expect(live::PayFare(S.BankrollCents, S.Life, Nightly.Id, true, At) && !live::PayFare(S.BankrollCents, S.Life, Nightly.Id, true, At) &&
				   S.BankrollCents == Before + 41000 - live::BusFareCents,
			"the bus home is paid once");
		ss::SaveData D;
		D.BankrollCents = S.BankrollCents;
		D.Life = S.Life;
		ss::SaveData P;
		const life::LiveEntry* Kept = ss::SaveData::Parse(D.Serialize(), P) ? live::EntryFor(P.Life, Nightly.Id) : nullptr;
		Expect(Kept && Kept->State == life::LiveEntry::Finished && Kept->Place == 7 && Kept->PrizeCents == 41000 && Kept->Roster.size() == E->Roster.size() && Kept->FareHome &&
				   Kept->Faces.size() == E->Faces.size() && (Kept->Faces.empty() || Kept->Faces.front().Label == E->Faces.front().Label) &&
				   P.Life.LiveEvents == 1,
			"entries survive a save");
		// A save from when the casino had its old name (a v1 save): the same entry, under its old id and name, loads as
		// the Embercrest's; the player's own name is theirs and stays as it was.
		ss::SaveData LegacySave = D;
		LegacySave.HeroName = "RiversideRat";
		std::string Old = LegacySave.Serialize();
		Expect(Old.rfind("shortstack.nightone.v2\n", 0) == 0, "saves are written as v2");
		Old.replace(0, std::string("shortstack.nightone.v2").size(), "shortstack.nightone.v1");
		for (const auto& [To, From] : {std::pair<std::string, std::string>{"embercrest", "riverside"}, {"Embercrest", "Riverside"}})
		{
			for (size_t Pos = Old.find(To); Pos != std::string::npos; Pos = Old.find(To, Pos + From.size()))
			{
				Old.replace(Pos, To.size(), From);
			}
		}
		ss::SaveData R;
		const life::LiveEntry* Renamed = Old.find("riverside") != std::string::npos && ss::SaveData::Parse(Old, R) ? live::EntryFor(R.Life, Nightly.Id) : nullptr;
		Expect(Renamed && Renamed->State == life::LiveEntry::Finished && Renamed->Name == Kept->Name && Renamed->Name.find("Embercrest") != std::string::npos,
			"a save from before the rename loads as the Embercrest's");
		Expect(R.HeroName == "RiversideRat", "the rename leaves the player's own name alone");
		ss::SaveData Again;
		Expect(ss::SaveData::Parse(R.Serialize(), Again) && Again.HeroName == "RiversideRat", "a v2 save isn't migrated again");
		// A night checkpointed before the rename (drawn under its old id) still resumes from its checkpoint.
		{
			live::Occurrence Legacy = Nightly;
			Legacy.Id = "riverside" + Nightly.Id.substr(std::string("embercrest").size());
			life::LiveEntry LegacyEntry = *Kept;
			LegacyEntry.Id = Legacy.Id;
			std::unique_ptr<ss::Tournament> Drawn = live::MakeField(Legacy, std::max(Kept->Entrants, 2), "Hero", live::SeedFor(LegacyEntry, "Hero"), {});
			life::LiveEntry Resumed = *Kept;
			Resumed.Checkpoint = Drawn->Checkpoint();
			bool bFits = false;
			std::unique_ptr<ss::Tournament> Back = live::ResumeField(Nightly, Resumed, std::max(Kept->Entrants, 2), "Hero", {}, true, bFits);
			Expect(bFits && Back && Back->Spec.Id == Legacy.Id, "a night checkpointed before the rename resumes");
			bool bFresh = false;
			life::LiveEntry Fresh = *Kept;
			Fresh.Checkpoint = live::MakeField(Nightly, std::max(Kept->Entrants, 2), "Hero", live::SeedFor(Fresh, "Hero"), {})->Checkpoint();
			std::unique_ptr<ss::Tournament> Same = live::ResumeField(Nightly, Fresh, std::max(Kept->Entrants, 2), "Hero", {}, true, bFresh);
			Expect(bFresh && Same && Same->Spec.Id == Nightly.Id, "a night checkpointed after the rename resumes under its own id");
		}
	}

	// A thin noon game: below the minimum it's cancelled, and everything comes back once.
	{
		ss::Chips B = 50000;
		life::State L;
		const live::Occurrence Noon = live::FindOccurrence("embercrest-noon@10");
		Expect(Noon.Valid() && live::Register(B, L, Noon, 10.0 * 1440.0 + 11.0 * 60.0, 12, {}).empty() && B == 50000 - 8000, "registered for Thursday's noon game");
		Expect(live::BelowMinimum(Noon, 12) && !live::BelowMinimum(Noon, 60), "twelve is below the room's minimum");
		Expect(live::Refund(B, L, Noon.Id, 10.0 * 1440.0 + 12.0 * 60.0, "not enough players") && !live::Refund(B, L, Noon.Id, 10.0 * 1440.0 + 12.0 * 60.0, "") && B == 50000 &&
				   L.Ledger.size() == 4 && L.LiveEvents == 0,
			"a cancelled event refunds once");
		// The ledger reversed: the buy-in and the fee come back as the two lines they went out as.
		Expect(L.Ledger[0].Amount == Noon.T->FeeCents && L.Ledger[1].Amount == Noon.T->BuyInCents - Noon.T->FeeCents && L.Ledger[2].Amount == -Noon.T->FeeCents &&
				   L.Ledger[3].Amount == -(Noon.T->BuyInCents - Noon.T->FeeCents) && L.Ledger[0].Label.find(": refunded") != std::string::npos &&
				   L.Ledger[1].Label.find(": refunded") != std::string::npos && L.Ledger[1].Label.find("not enough players") != std::string::npos,
			"a refund reverses the entry and the fee, line for line");
		Expect(!live::CanRegister(B, L, Noon, 10.0 * 1440.0 + 11.5 * 60.0).empty(), "a cancelled event can't be entered again");
	}

	// The game closed mid-tournament: the next load settles the entry by the rule for leaving, once.
	{
		ss::SaveData D;
		D.BankrollCents = 50000;
		const live::Occurrence Sunday = live::FindOccurrence("embercrest@13");
		Expect(live::Register(D.BankrollCents, D.Life, Sunday, 13.0 * 1440.0 + 18.0 * 60.0, 95, {}).empty(), "registered for the Sunday");
		D.ClockMinutes = 14.0 * 1440.0 + 12.0 * 60.0 - 1440.0; // Monday noon
		Hooks H2;
		ss::Session S2(H2, "abandoned", &D);
		const life::LiveEntry* A = live::EntryFor(S2.Life, Sunday.Id);
		Expect(A && A->State == life::LiveEntry::Finished && A->Place >= 1 && A->Place <= 95 && S2.Life.LiveEvents == 1, "an entry left mid-tournament is blinded out and settled on the next load");
		if (A)
		{
			std::printf("  embercrest: left the Sunday, blinded out in %s of 95\n", ss::Ordinal(A->Place).c_str());
		}
	}

	// Late registration: the field plays on without the player, whose stack waits untouched.
	{
		std::unique_ptr<ss::Tournament> Late = live::MakeField(live::FindOccurrence("embercrest@13"), 90, "grinder_3c", "late", {});
		Late->HeroAway = true;
		for (int K = 0; K < 12; ++K)
		{
			Late->SimulateTick();
		}
		Late->HeroAway = false;
		std::vector<ss::TEvent> Ev;
		std::unique_ptr<ss::Hand> First = Late->StartTick(Ev);
		Expect(Late->Hero().Stack == Late->Spec.StartingStack && Late->Hero().Hands == 0 && Late->LevelIndex >= 1 && First != nullptr,
			"a late registration sits down with a full stack, levels in");
		Late->FinishTick(First.get(), &Auto);
	}
	// Every night, every field size: while the player is in and the night isn't over, there's a hand for them.
	{
		bool Dealt = true;
		std::string Where;
		int Nights = 0;
		for (int Day = 140; Day < 147; ++Day)
		{
			for (const live::Occurrence& O : live::Occurrences(Day))
			{
				std::unique_ptr<ss::Tournament> N = live::MakeField(O, O.Field + 1, "grinder_3c", O.Id + ":dealt", {});
				ss::Profile Me = ss::MakeProfile(ss::Archetype::Lag, N->R);
				for (int K = 0; K < 6000 && !N->bFinished && !N->Hero().Busted; ++K)
				{
					std::vector<ss::TEvent> Ev;
					std::unique_ptr<ss::Hand> Hand = N->StartTick(Ev);
					if (!Hand && !N->Hero().Busted && !N->bFinished && Dealt)
					{
						Dealt = false;
						int Seated = 0;
						for (int Idx : N->Tables[N->Hero().TableId].Seats)
						{
							Seated += Idx >= 0 ? 1 : 0;
						}
						Where = O.Id + " tick " + std::to_string(N->Tick) + ", table " + std::to_string(N->Hero().TableId) + " with " + std::to_string(Seated) + ", " +
							std::to_string(N->Tables.size()) + " tables, " + std::to_string(N->Remaining) + " left";
					}
					N->FinishTick(Hand.get(), &Me);
				}
				++Nights;
			}
		}
		if (!Where.empty())
		{
			std::printf("  no hand for the player at %s\n", Where.c_str());
		}
		Expect(Dealt, "a player still in always has a hand dealt");
		std::printf("  embercrest: %d nights, the player dealt in every hand they were in\n", Nights);
	}
	// Walking away: dealt in, checking and folding, the blinds take the stack.
	{
		std::unique_ptr<ss::Tournament> Gone = live::MakeField(live::FindOccurrence("embercrest-nightly@16"), 70, "grinder_3c", "gone", {});
		Gone->HeroSitsOut = true;
		int Ticks2 = 0;
		while (!Gone->Hero().Busted && !Gone->bFinished && Ticks2 < 6000)
		{
			Gone->SimulateTick();
			++Ticks2;
		}
		Expect(Gone->Hero().Busted && Gone->Hero().Place > 1 && Gone->Hero().VpipHands == 0, "a player who walks away is blinded off, never putting a chip in voluntarily");
		std::printf("  embercrest: walked away, blinded off in %s after %d hands\n", ss::Ordinal(Gone->Hero().Place).c_str(), Gone->Hero().Hands);
	}
}

/** One line per event, to compare two runs of a night. */
std::string EventLine(const ss::TEvent& E)
{
	return std::to_string(static_cast<int>(E.Type)) + "|" + E.Id + "|" + E.Name + "|" + std::to_string(E.Place) + "|" + std::to_string(E.PrizeCents) + "|" + std::to_string(E.TableId) +
		"|" + std::to_string(E.From) + ">" + std::to_string(E.To) + "|" + std::to_string(E.LevelNumber) + "|" + E.EliminatedBy;
}

/** The Embercrest's checkpoints: a night closed after any hand carries on exactly as if it never was. */
void EmbercrestCheckpoints()
{
	namespace life = ss::life;
	namespace live = ss::live;
	const live::Occurrence Sunday = live::FindOccurrence("embercrest@13");
	std::vector<ss::ReservedPlayer> Known(3);
	Known[0].Name = "Mei";
	Known[0].Type = ss::Archetype::Crusher;
	Known[1].Name = "Big Lou";
	Known[1].Type = ss::Archetype::Maniac;
	Known[2].Name = "Mrs. Park";
	Known[2].Type = ss::Archetype::Nit;
	auto Field = [&](const std::string& Seed) { return live::MakeField(Sunday, 104, "grinder_3c", Seed, Known); };
	// The player's own hands, played the same way in every run (their choices don't come from the room's draw).
	ss::Rng AutoRng("checkpoint-auto");
	const ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, AutoRng);
	auto Step = [&](ss::Tournament& T, std::vector<std::string>& Log) {
		std::vector<ss::TEvent> Ev;
		std::unique_ptr<ss::Hand> Hand = T.StartTick(Ev);
		for (const ss::TEvent& E : T.FinishTick(Hand.get(), &Auto))
		{
			Ev.push_back(E);
		}
		for (const ss::TEvent& E : Ev)
		{
			Log.push_back(std::to_string(T.Tick) + ":" + EventLine(E));
		}
	};

	// The night straight through.
	std::unique_ptr<ss::Tournament> Whole = Field("checkpoints");
	std::vector<std::string> WholeLog;
	std::vector<std::string> Marks; // the checkpoint after every hand
	Marks.push_back(Whole->Checkpoint());
	while (!Whole->bFinished && Whole->Tick < 6000)
	{
		Step(*Whole, WholeLog);
		Marks.push_back(Whole->Checkpoint());
	}
	Expect(Whole->bFinished && Marks.size() > 100, "the Sunday plays to a winner");

	// Closed and reopened after hand 0, 1, 7, a third of the way, the bubble's neighbourhood, the last hand: every one
	// carries on to the same night, event for event, chip for chip.
	bool Exact = true;
	bool RoundTrip = true;
	std::string Where;
	const size_t N = Marks.size();
	for (const size_t At : {static_cast<size_t>(0), static_cast<size_t>(1), static_cast<size_t>(7), N / 3, N / 2, (N * 4) / 5, N - 2, N - 1})
	{
		std::unique_ptr<ss::Tournament> Again = Field("checkpoints");
		const bool Ok = Again->Restore(Marks[At]);
		RoundTrip = RoundTrip && Ok && Again->Checkpoint() == Marks[At];
		std::vector<std::string> Log;
		while (Ok && !Again->bFinished && Again->Tick < 6000)
		{
			Step(*Again, Log);
		}
		// The events after the checkpoint, against the same stretch of the whole night.
		std::vector<std::string> Tail;
		for (const std::string& L : WholeLog)
		{
			if (std::atoi(L.c_str()) > static_cast<int>(At))
			{
				Tail.push_back(L);
			}
		}
		const bool Same = Ok && Log == Tail && Again->Checkpoint() == Whole->Checkpoint();
		if (!Same && Where.empty())
		{
			Where = "after hand " + std::to_string(At);
		}
		Exact = Exact && Same;
	}
	Expect(RoundTrip, "a checkpoint reads back to itself");
	if (!Where.empty())
	{
		std::printf("  checkpoint diverged %s\n", Where.c_str());
	}
	Expect(Exact, "a night reopened after any hand plays on exactly as it would have");

	// Closed again and again: a checkpoint every forty hands, each read into a fresh field.
	{
		std::unique_ptr<ss::Tournament> Hop = Field("checkpoints");
		std::vector<std::string> Log;
		int Reopened = 0;
		while (!Hop->bFinished && Hop->Tick < 6000)
		{
			Step(*Hop, Log);
			if (Hop->Tick % 40 == 0)
			{
				const std::string Ck = Hop->Checkpoint();
				Hop = Field("checkpoints");
				Reopened += Hop->Restore(Ck) ? 1 : 0;
			}
		}
		Expect(Reopened >= 4 && Log == WholeLog && Hop->Checkpoint() == Whole->Checkpoint(), "a night closed every forty hands is still the same night");
		std::printf("  embercrest: %zu hands, reopened %d times, every hand and every chip where it was\n", Marks.size() - 1, Reopened);
	}

	// Only its own field takes it, and a bad one changes nothing.
	{
		std::unique_ptr<ss::Tournament> Other = Field("someone else's night");
		const std::string Before = Other->Checkpoint();
		const std::string Mid = Marks[N / 2];
		Expect(!Other->Restore(Mid) && Other->Checkpoint() == Before, "another field's checkpoint is refused");
		std::unique_ptr<ss::Tournament> Mine = Field("checkpoints");
		const std::string Fresh = Mine->Checkpoint();
		Expect(!Mine->Restore(Mid.substr(0, Mid.size() / 2)) && !Mine->Restore("") && !Mine->Restore("ck1 0 0 0") && Mine->Checkpoint() == Fresh, "a broken checkpoint is refused");
		// Between ticks only.
		std::vector<ss::TEvent> Ev;
		std::unique_ptr<ss::Hand> Hand = Mine->StartTick(Ev);
		Mine->BeginFinish(Hand.get(), &Auto);
		Expect(Mine->Checkpoint().empty() && !Mine->Restore(Mid), "no checkpoint in the middle of a round");
		while (!Mine->FinishSome(4))
		{
		}
		Mine->EndFinish();
	}

	// The entry keeps it in the save, and settling clears it.
	{
		ss::SaveData D;
		D.BankrollCents = 50000;
		Expect(live::Register(D.BankrollCents, D.Life, Sunday, 13.0 * 1440.0 + 18.0 * 60.0, 104, {}).empty(), "registered for the Sunday");
		life::LiveEntry* E = live::EntryFor(D.Life, Sunday.Id);
		E->FareThere = true;
		E->ArrivedAt = 13.0 * 1440.0 + 18.0 * 60.0 + 40.0;
		E->Checkpoint = Marks[N / 3];
		E->CheckpointAt = 13.0 * 1440.0 + 21.0 * 60.0 + 12.5;
		E->CheckpointHost = "hands=88\tmet=npc:Mei,npc:Big Lou\nbreak=0";
		ss::SaveData P;
		const life::LiveEntry* K = ss::SaveData::Parse(D.Serialize(), P) ? live::EntryFor(P.Life, Sunday.Id) : nullptr;
		Expect(K && K->Checkpoint == E->Checkpoint && K->CheckpointHost == E->CheckpointHost && std::fabs(K->CheckpointAt - E->CheckpointAt) < 1e-3 &&
				   std::fabs(K->ArrivedAt - E->ArrivedAt) < 1e-2,
			"a checkpoint survives a save");
		ss::Chips B = D.BankrollCents;
		life::State L = D.Life;
		Expect(live::Settle(B, L, Sunday.Id, 30, 104, 0, E->CheckpointAt) && live::EntryFor(L, Sunday.Id)->Checkpoint.empty(), "settling clears the checkpoint");
	}

	// The game closed during the night: the next load goes back to the seat; nothing was dealt while it was closed.
	{
		struct BackHooks : Hooks
		{
			std::string Went;
			bool GoOut(const std::string& Id, ss::Chips) override
			{
				Went = Id;
				return true;
			}
		};
		ss::SaveData D;
		D.BankrollCents = 50000;
		D.HeroName = "grinder_3c";
		const double Reg = 13.0 * 1440.0 + 18.0 * 60.0;
		Expect(live::Register(D.BankrollCents, D.Life, Sunday, Reg, 104, {}).empty(), "registered for the Sunday");
		live::PayFare(D.BankrollCents, D.Life, Sunday.Id, false, Reg);
		life::LiveEntry* E = live::EntryFor(D.Life, Sunday.Id);
		std::unique_ptr<ss::Tournament> Night = live::MakeField(Sunday, E->Entrants, "grinder_3c", live::SeedFor(*E, "grinder_3c"), {});
		// Forty hands in, folding most of them: still in, a stack to come back to.
		ss::Rng Careful("careful");
		const ss::Profile Nit = ss::MakeProfile(ss::Archetype::Nit, Careful);
		for (int K = 0; K < 40; ++K)
		{
			std::vector<ss::TEvent> Ev;
			std::unique_ptr<ss::Hand> Hand = Night->StartTick(Ev);
			Night->FinishTick(Hand.get(), &Nit);
		}
		Expect(!Night->Hero().Busted && Night->Hero().Stack > 0, "still in after forty hands");
		E->ArrivedAt = Reg + 20.0;
		E->Checkpoint = Night->Checkpoint();
		E->CheckpointAt = Sunday.Start + Night->ClockMinutes() - static_cast<double>(Sunday.T->StartMinute);
		D.ClockMinutes = E->CheckpointAt - 1440.0;
		const ss::Chips Bank = D.BankrollCents;
		BackHooks H;
		ss::Session S(H, "reopened", &D);
		const life::LiveEntry* Still = live::EntryFor(S.Life, Sunday.Id);
		Expect(Still && Still->State == life::LiveEntry::Registered && Still->Checkpoint == E->Checkpoint && S.BankrollCents == Bank, "reopened mid-night: the entry is untouched");
		Expect(S.LiveInProgress() == Sunday.Id && S.ResumeLive().empty() && H.Went == Sunday.Id && S.BankrollCents == Bank, "the next load goes straight back to the seat, no bus");

		// Never went back (a day later): the seat is dealt in from where the player left it, and blinded off.
		std::unique_ptr<ss::Tournament> Gone = live::MakeField(Sunday, E->Entrants, "grinder_3c", live::SeedFor(*E, "grinder_3c"), {});
		Gone->Restore(E->Checkpoint);
		Gone->HeroSitsOut = true;
		const int StartHands = Gone->Hero().Hands;
		while (!Gone->Hero().Busted && !Gone->bFinished)
		{
			Gone->SimulateTick();
		}
		D.ClockMinutes = Sunday.Start + 24.0 * 60.0 - 1440.0;
		BackHooks H2;
		ss::Session S2(H2, "never went back", &D);
		const life::LiveEntry* A = live::EntryFor(S2.Life, Sunday.Id);
		Expect(A && A->State == life::LiveEntry::Finished && A->Place == Gone->Hero().Place && A->Checkpoint.empty() && S2.LiveInProgress().empty(),
			"a night never gone back to is blinded off from the checkpoint, and settled once");
		std::printf("  embercrest: left at hand %d with %lld chips, blinded off in %s after %d more hands\n", Night->Tick, static_cast<long long>(Night->Hero().Stack),
			ss::Ordinal(Gone->Hero().Place).c_str(), Gone->Hero().Hands - StartHands);
	}
}

/** Railing a live tournament after busting: the places the player watched are the world's result. */
void EmbercrestRail()
{
	namespace life = ss::life;
	namespace live = ss::live;
	struct OutHooks : Hooks
	{
		bool GoOut(const std::string&, ss::Chips) override { return true; }
	};
	OutHooks H;
	ss::Session S(H, "railing");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	S.LobbyMinutes = 9.0 * 1440.0 - 1440.0 + 17.0 * 60.0 + 30.0;
	Now = Wait(S, Now, 0.2);
	S.Life.Energy = 60.0;
	S.BankrollCents = 50000;
	const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@9");
	Expect(S.GoToLive(Nightly.Id).empty(), "registered for Wednesday's Nightly");
	const life::LiveEntry* E = live::EntryFor(S.Life, Nightly.Id);
	if (!E || E->Faces.size() < 2)
	{
		Expect(false, "the Nightly has faces to watch");
		return;
	}
	const std::string Winner = E->Faces[0].Name;
	const std::string Second = E->Faces[1].Name;
	// The player went out 40th, stayed on the rail and watched the last two.
	ss::SaveData D = H.Last;
	D.Life = S.Life;
	const double End = Nightly.Start + Nightly.T->Hours * 60.0;
	live::Settle(D.BankrollCents, D.Life, Nightly.Id, 40, E->Entrants, 0, Nightly.Start + 200.0);
	D.NoteLive(Nightly.Start + 200.0, Nightly.Id, 40, 0, E->Entrants, {});
	ss::LiveSeen A;
	A.Tag = 'P';
	A.Name = Winner;
	A.Value = 1;
	ss::LiveSeen B = A;
	B.Name = Second;
	B.Value = 2;
	D.NoteLivePlaces(Nightly.Start + 200.0, Nightly.Id, {A, B});
	D.ClockMinutes = End + 60.0 - 1440.0;
	Expect(!D.WorldText.empty() || D.WorldSnapshot != nullptr, "the save carries the world");
	if (D.WorldText.empty() && D.WorldSnapshot)
	{
		D.WorldSnapshot->Write(D.WorldText);
	}
	OutHooks H2;
	ss::Session S2(H2, "railed", &D);
	S2.Living().AdvanceTo(End + 180.0);
	const ss::net::EventResult* Res = S2.Living().ResultOf(Nightly.Id);
	int First = -9;
	int Runner = -9;
	if (Res)
	{
		for (const ss::net::Placing& P : Res->FinalTable)
		{
			First = P.Place == 1 ? P.Player : First;
			Runner = P.Place == 2 ? P.Player : Runner;
		}
	}
	Expect(Res && First == S2.Living().Find(Winner) && Runner == S2.Living().Find(Second), "the world's result is the final the player watched from the rail");
	std::printf("  embercrest: from the rail, %s beat %s heads-up; the world's result agrees\n", Winner.c_str(), Second.c_str());
}

/** Bond texts and the rest of the phone: who wrote what, and what the world said about the sender's next night when it came. */
struct PhoneHooks : Hooks
{
	ss::Session* S = nullptr;
	std::vector<std::string> Nexts;
	bool GoOut(const std::string&, ss::Chips) override { return true; }
	void Text(const std::string& From, const std::string& Body) override
	{
		Hooks::Text(From, Body);
		const int Npc = S ? S->Living().Find(From) : -1;
		Nexts.push_back(Npc >= 0 ? S->Living().NextEmbercrest(Npc, S->WorldMinutes()) : std::string());
	}
};

/** Walks the session's clock on a minute at a time (the phone's moments are crossed as they come). */
double Walk(ss::Session& S, double Now, double ToWorld)
{
	while (S.WorldMinutes() < ToWorld)
	{
		S.LobbyMinutes = std::min(S.LobbyMinutes + 1.0, ToWorld - 1440.0);
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = static_cast<char>(std::tolower(static_cast<unsigned char>(Ch)));
	}
	return S;
}

/** The minimum field (R6): below it an event is called off, every entry refunded once, and the world records no winner. */
void EmbercrestMinimum()
{
	namespace life = ss::life;
	namespace live = ss::live;
	// A save with a world in it, Night One.
	Hooks H0;
	ss::Session S0(H0, "minimum");
	S0.Save();
	const ss::SaveData Fresh = H0.Last;

	// Registered for Wednesday's noon game with twelve in the field, and never got there (the game closed on the bus).
	{
		const live::Occurrence Noon = live::FindOccurrence("embercrest-noon@2");
		ss::SaveData D = Fresh;
		D.BankrollCents = 50000;
		Expect(live::Register(D.BankrollCents, D.Life, Noon, Noon.Start - 90.0, 12, {}).empty() && live::PayFare(D.BankrollCents, D.Life, Noon.Id, false, Noon.Start - 90.0),
			"registered for a thin noon game, the bus paid");
		D.ClockMinutes = Noon.Start - 60.0 - 1440.0;
		PhoneHooks H;
		ss::Session S(H, "minimum", &D);
		H.S = &S;
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Walk(S, 0.0, Noon.Start - 1.0);
		const life::LiveEntry* E = live::EntryFor(S.Life, Noon.Id);
		Expect(E && E->State == life::LiveEntry::Registered && S.BankrollCents == 50000 - Noon.T->BuyInCents - live::BusFareCents, "nothing's called off before cards");
		const size_t Inbox = H.Inbox.size();
		Now = Walk(S, Now, Noon.Start + 5.0);
		E = live::EntryFor(S.Life, Noon.Id);
		const ss::world::Cancellation* Called = S.Living().CancellationOf(Noon.Id);
		Expect(E && E->State == life::LiveEntry::Refunded && S.BankrollCents == 50000 - live::BusFareCents && S.Life.LiveEvents == 0,
			"at cards-up a short field is called off: the buy-in and fee come back, nothing is played");
		Expect(H.Got(Inbox, "Embercrest", "was cancelled: 12 players"), "the desk texts the player who wasn't there");
		Expect(Called && Called->Field == 12 && S.Living().Registered(Noon.Id).empty() && S.Living().PlannedEntries(Noon.Id) == 0, "the world records the cancellation");
		S.Living().AdvanceTo(Noon.Start + 2.0 * 1440.0);
		Expect(S.Living().ResultOf(Noon.Id) == nullptr && S.Living().CancellationOf(Noon.Id) != nullptr, "and never a winner");
		// Reloaded: nothing is refunded or said twice.
		const ss::Chips Bank = S.BankrollCents;
		const size_t Lines = S.Life.Ledger.size();
		PhoneHooks H2;
		ss::Session Again(H2, "minimum", &H.Last);
		H2.S = &Again;
		Again.CurrentScreen = ss::Screen::Lobby;
		Walk(Again, 0.0, Again.WorldMinutes() + 30.0);
		Expect(Again.BankrollCents == Bank && Again.Life.Ledger.size() == Lines && !H2.Got(0, "Embercrest", "cancelled") && Again.Living().CancellationOf(Noon.Id) != nullptr,
			"a reload refunds nothing twice and says nothing twice");
	}

	// The room called it off with the player there (the desk refunded, the bus home): the world agrees nobody played it.
	{
		PhoneHooks H;
		ss::Session S(H, "minimum-room", &Fresh);
		H.S = &S;
		S.CurrentScreen = ss::Screen::Lobby;
		const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@2");
		double Now = Walk(S, 0.0, Nightly.Start - 100.0);
		S.Life.Energy = 60.0;
		S.BankrollCents = 50000;
		// A slow night: ten people coming (the debug hook thins the world's plan).
		S.Living().SetPlannedEntries(Nightly.Id, 10);
		Expect(S.GoToLive(Nightly.Id).empty(), "registered for a thin Nightly");
		const life::LiveEntry* E = live::EntryFor(S.Life, Nightly.Id);
		Expect(E && live::BelowMinimum(Nightly, E->Entrants), "the entry has the thin field");
		// Waiting at home past the start isn't the room's call to make for it: the player's room makes it.
		ss::SaveData D = H.Last;
		life::LiveEntry* Mine = live::EntryFor(D.Life, Nightly.Id);
		Mine->ArrivedAt = Nightly.Start - 10.0;
		live::Refund(D.BankrollCents, D.Life, Nightly.Id, Nightly.Start + 2.0, "not enough players");
		live::PayFare(D.BankrollCents, D.Life, Nightly.Id, true, Nightly.Start + 2.0);
		D.ClockMinutes = Nightly.Start + 27.0 - 1440.0;
		PhoneHooks H2;
		ss::Session Home(H2, "minimum-room", &D);
		H2.S = &Home;
		Home.CurrentScreen = ss::Screen::Lobby;
		Now = Walk(Home, Now, Home.WorldMinutes() + 10.0);
		Expect(Home.Living().CancellationOf(Nightly.Id) != nullptr && !H2.Got(0, "Embercrest", "cancelled"), "the room's refund is the world's cancellation (the floor already said so)");
		Home.Living().AdvanceTo(Nightly.Start + 2.0 * 1440.0 + 60.0);
		Expect(Home.Living().ResultOf(Nightly.Id) == nullptr, "a night the room called off has no winner");
		Expect(Home.BankrollCents == 50000 - 2 * live::BusFareCents, "the player is out only the bus");
	}

	// The world calls off a thin night nobody it follows could make up, and the desk won't take an entry for it.
	{
		PhoneHooks H;
		ss::Session S(H, "minimum-world", &Fresh);
		H.S = &S;
		S.CurrentScreen = ss::Screen::Lobby;
		const live::Occurrence Noon = live::FindOccurrence("embercrest-noon@2");
		const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@2");
		double Now = Walk(S, 0.0, Nightly.Start - 300.0);
		S.Living().SetPlannedEntries(Nightly.Id, 9);
		Now = Walk(S, Now, Nightly.Start + 5.0);
		const ss::world::Cancellation* Called = S.Living().CancellationOf(Nightly.Id);
		Expect(Called && Called->Field == 9 && Called->At == Nightly.Start, "a thin night is called off at its start");
		S.Life.Energy = 60.0;
		S.BankrollCents = 50000;
		Expect(S.GoToLive(Nightly.Id) == "That one was cancelled." && live::EntryFor(S.Life, Nightly.Id) == nullptr, "the desk won't register anyone for it");
		S.Living().AdvanceTo(Nightly.Start + 12.0 * 60.0);
		Expect(S.Living().ResultOf(Nightly.Id) == nullptr && S.Living().ResultOf(Noon.Id) != nullptr, "the thin night has no winner; the noon game that ran does");
		(void)Now;
	}

	// Never went back to a short night the room was about to call: the next load refunds it (nothing is played out).
	{
		const live::Occurrence Sunday = live::FindOccurrence("embercrest@6");
		ss::SaveData D = Fresh;
		D.BankrollCents = 50000;
		Expect(live::Register(D.BankrollCents, D.Life, Sunday, Sunday.Start - 60.0, 14, {}).empty(), "registered for a thin Sunday");
		live::PayFare(D.BankrollCents, D.Life, Sunday.Id, false, Sunday.Start - 60.0);
		live::EntryFor(D.Life, Sunday.Id)->ArrivedAt = Sunday.Start - 30.0;
		D.ClockMinutes = Sunday.Start + 1440.0 - 1440.0;
		PhoneHooks H;
		ss::Session S(H, "minimum-gone", &D);
		H.S = &S;
		const life::LiveEntry* E = live::EntryFor(S.Life, Sunday.Id);
		Expect(E && E->State == life::LiveEntry::Refunded && S.Life.LiveEvents == 0 && S.LiveInProgress().empty() && S.BankrollCents == 50000 - live::BusFareCents,
			"a short night never gone back to is refunded on the next load, never played out");
		S.CurrentScreen = ss::Screen::Lobby;
		Walk(S, 0.0, S.WorldMinutes() + 5.0);
		Expect(H.Got(0, "Embercrest", "was cancelled: 14 players") && S.Living().CancellationOf(Sunday.Id) != nullptr, "and the desk says why");
	}

	// Written in at the desk for a full Nightly and off the list again before the seats (the room's other refund): no
	// cancellation. The night runs without the player, nobody texts that it was called off, and it has its winner.
	{
		const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@2");
		ss::SaveData D = Fresh;
		D.BankrollCents = 50000;
		const double Desk = Nightly.Start - 120.0;
		Expect(live::Register(D.BankrollCents, D.Life, Nightly, Desk, Nightly.Field + 1, {}).empty(), "written in at the desk for a full Nightly");
		live::EntryFor(D.Life, Nightly.Id)->ArrivedAt = Desk;
		Expect(live::Refund(D.BankrollCents, D.Life, Nightly.Id, Desk + 20.0, "off the list before sitting down") && D.BankrollCents == 50000, "and off the list before the seats");
		D.ClockMinutes = Nightly.Start + 60.0 - 1440.0;
		PhoneHooks H;
		ss::Session S(H, "minimum-unlisted", &D);
		H.S = &S;
		S.CurrentScreen = ss::Screen::Lobby;
		Walk(S, 0.0, S.WorldMinutes() + 10.0);
		Expect(S.Living().CancellationOf(Nightly.Id) == nullptr && !H.Got(0, "Embercrest", "cancelled"), "leaving the list isn't a cancellation, and nobody says it was");
		S.Living().AdvanceTo(Nightly.Start + Nightly.T->Hours * 60.0 + 120.0);
		Expect(S.Living().ResultOf(Nightly.Id) != nullptr, "the night ran without the player and has its winner");
	}
	std::printf("  embercrest: short fields called off, refunded once, no winners\n");
}

/** After a night at the Embercrest, people the player sat with text: about what passed between them, a couple at most, never strangers. */
void EmbercrestAftermath()
{
	namespace life = ss::life;
	namespace live = ss::live;
	PhoneHooks H;
	ss::Session S(H, "aftermath");
	H.S = &S;
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	const double Wednesday = 9.0 * 1440.0;
	S.LobbyMinutes = Wednesday - 1440.0 + 17.0 * 60.0 + 30.0;
	Now = Wait(S, Now, 0.2);
	S.Life.Energy = 60.0;
	S.BankrollCents = 50000;
	const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@9");
	Expect(S.GoToLive(Nightly.Id).empty(), "registered for Wednesday's Nightly");
	const life::LiveEntry* E = live::EntryFor(S.Life, Nightly.Id);
	if (!E || E->Roster.size() < 5)
	{
		Expect(false, "the Nightly has people the world follows");
		return;
	}
	// Five of them at the player's table: one sent the player home, one the player sent home, one took a big pot off them,
	// one the player knew from Dee's, and one the player had never met.
	ss::world::World& W = S.Living();
	const std::string Buster = E->Roster[0].first;
	const std::string Busted = E->Roster[1].first;
	const std::string Stranger = E->Roster[2].first;
	const std::string Friend = E->Roster[3].first;
	const std::string Potted = E->Roster[4].first;
	W.Remember(W.Find(Friend), ss::world::MemoryKind::BackRoom, "Dee's game", 0, Wednesday - 4.0 * 1440.0);
	W.Remember(W.Find(Friend), ss::world::MemoryKind::BackRoom, "Dee's game", 0, Wednesday - 2.0 * 1440.0);
	S.Save();
	ss::SaveData D = H.Last;
	const double Out = Nightly.Start + 170.0;
	live::Settle(D.BankrollCents, D.Life, Nightly.Id, 30, E->Entrants, 0, Out);
	auto Saw = [](char Tag, const std::string& Name, long long Value) {
		ss::LiveSeen L;
		L.Tag = Tag;
		L.Name = Name;
		L.Value = Value;
		return L;
	};
	D.NoteLive(Out, Nightly.Id, 30, 0, E->Entrants,
		{Saw('S', Buster, 0), Saw('M', Buster, 0), Saw('K', Buster, 0), Saw('P', Busted, 44), Saw('M', Busted, 0), Saw('H', Busted, 0), Saw('S', Stranger, 0),
			Saw('M', Stranger, 0), Saw('S', Friend, 0), Saw('M', Friend, 0), Saw('S', Potted, 0), Saw('M', Potted, 0), Saw('B', Potted, 9400)});
	live::PayFare(D.BankrollCents, D.Life, Nightly.Id, true, Out);
	D.ClockMinutes = Out + 25.0 - 1440.0;
	{
		ss::SaveData P;
		const life::LiveEntry* Kept = ss::SaveData::Parse(D.Serialize(), P) ? live::EntryFor(P.Life, Nightly.Id) : nullptr;
		bool Table = Kept && Kept->Table.size() == 5;
		for (const life::TableMate& M : Kept ? Kept->Table : std::vector<life::TableMate>())
		{
			Table = Table && (M.Seen & life::TableMate::SatWith) != 0;
			Table = Table && (M.Name != Buster || (M.Seen & life::TableMate::KnockedYouOut) != 0) && (M.Name != Busted || (M.Seen & life::TableMate::YouKnockedOut) != 0);
			Table = Table && (M.Name != Potted || ((M.Seen & life::TableMate::PotWon) != 0 && M.Pot == 9400));
		}
		Expect(Table, "the entry keeps who sat with the player and what passed between them, through a save");
	}

	// The next two days, a minute at a time. Who texts, and when.
	struct Run
	{
		std::vector<std::pair<std::string, std::string>> Texts; // the bond texts, in order
		std::vector<std::string> Nexts;
		std::vector<double> At;
		ss::SaveData Last;
	};
	auto Play = [&](const std::string& Seed, bool JoinTomorrow) {
		Run R;
		PhoneHooks Ph;
		ss::Session Home(Ph, Seed, &D);
		Ph.S = &Home;
		Home.CurrentScreen = ss::Screen::Lobby;
		if (JoinTomorrow)
		{
			// The one who sent the player home is in Thursday's Nightly too.
			Home.Living().Join("embercrest-nightly@10", Home.Living().Find(Buster));
		}
		double T = 0.0;
		const double Until = Wednesday + 3.0 * 1440.0;
		size_t Seen = 0;
		while (Home.WorldMinutes() < Until)
		{
			T = Walk(Home, T, std::min(Until, Home.WorldMinutes() + 60.0));
			for (; Seen < Ph.Inbox.size(); ++Seen)
			{
				const std::string& From = Ph.Inbox[Seen].first;
				if (From == Buster || From == Busted || From == Stranger || From == Friend || From == Potted)
				{
					R.Texts.push_back(Ph.Inbox[Seen]);
					R.Nexts.push_back(Ph.Nexts[Seen]);
					R.At.push_back(Home.WorldMinutes());
				}
			}
		}
		R.Last = Ph.Last;
		return R;
	};
	const Run A = Play("aftermath-home", true);
	bool Tokens = true;
	bool Mentions = true;
	for (size_t K = 0; K < A.Texts.size(); ++K)
	{
		Tokens = Tokens && !A.Texts[K].second.empty() && A.Texts[K].second.find('{') == std::string::npos;
		if (!A.Nexts[K].empty())
		{
			const live::Occurrence Next = live::FindOccurrence(A.Nexts[K]);
			const std::string Key = Next.T->Key;
			const std::string Word = Key == "bigstack" ? "big stack" : Key == "warmup" ? "warm-up" : Key;
			Mentions = Mentions && Lower(A.Texts[K].second).find(Word) != std::string::npos;
		}
	}
	Expect(A.Texts.size() == 2, "a night out brings two texts, no more");
	// Who sent the player home has the most to say; then who the player sent home (or someone who went on to win it).
	Expect(A.Texts.size() == 2 && A.Texts[0].first == Buster && (A.Texts[1].first == Busted || A.Texts[1].first == Potted), "from the ones with the most to say");
	Expect(A.At.size() == 2 && A.At[0] >= Wednesday + 1440.0 && A.At[1] < Wednesday + 3.0 * 1440.0 && A.At[1] > A.At[0], "over the next day or two");
	Expect(Tokens, "every text is written out");
	Expect(!A.Nexts.empty() && A.Nexts[0] == "embercrest-nightly@10" && Lower(A.Texts[0].second).find("nightly") != std::string::npos &&
			   Lower(A.Texts[0].second).find("tonight") != std::string::npos,
		"someone with a night planned at the room says so");
	Expect(Mentions, "whoever has a night planned mentions it");
	for (size_t K = 0; K < A.Texts.size(); ++K)
	{
		std::printf("  embercrest: %s texted: %s\n", A.Texts[K].first.c_str(), A.Texts[K].second.c_str());
	}
	// The same save says the same things.
	const Run B = Play("aftermath-again", true);
	Expect(B.Texts == A.Texts, "the texts come from the save: the same night, the same words");
	// The same night without the one planned: the same people, and the plans they really have.
	const Run C = Play("aftermath-quiet", false);
	Expect(C.Texts.size() == 2 && C.Texts[0].first == Buster && (C.Nexts[0] == A.Nexts[0]) == (C.Texts[0].second == A.Texts[0].second),
		"the next night is mentioned only as the world has it planned");
	// A reload after the texts says nothing twice.
	{
		PhoneHooks Ph;
		ss::Session Later(Ph, "aftermath-later", &A.Last);
		Ph.S = &Later;
		Later.CurrentScreen = ss::Screen::Lobby;
		Walk(Later, 0.0, Later.WorldMinutes() + 1440.0);
		bool Again = false;
		for (const auto& T : Ph.Inbox)
		{
			Again = Again || T.first == Buster || T.first == Busted || T.first == Stranger || T.first == Friend || T.first == Potted;
		}
		Expect(!Again, "a reload says nothing twice");
	}
	// Home at ten on Thursday night from a day out that crossed both of Thursday's moments: what comes in then is said as
	// of then. Thursday's Nightly is under way, so nobody asks the player to it; Friday's they can.
	{
		ss::SaveData Late = D;
		Late.ClockMinutes = Wednesday + 22.0 * 60.0;
		PhoneHooks Ph;
		ss::Session Home(Ph, "aftermath-late", &Late);
		Ph.S = &Home;
		Home.CurrentScreen = ss::Screen::Lobby;
		ss::world::World& Lw = Home.Living();
		Lw.Join("embercrest-nightly@10", Lw.Find(Buster));
		Lw.Join("embercrest-nightly@11", Lw.Find(Buster));
		Home.ResumeCalendarFrom(Wednesday + 1440.0 + 11.0 * 60.0);
		Wait(Home, 0.0, 0.1);
		std::vector<std::string> Who;
		bool Stale = false;
		bool Tomorrow = false;
		for (size_t K = 0; K < Ph.Inbox.size(); ++K)
		{
			const std::string& From = Ph.Inbox[K].first;
			if (From != Buster && From != Busted && From != Stranger && From != Friend && From != Potted)
			{
				continue;
			}
			Who.push_back(From);
			const std::string Said = Lower(Ph.Inbox[K].second);
			Stale = Stale || Said.find("tonight") != std::string::npos;
			Tomorrow = Tomorrow || (From == Buster && Said.find("tomorrow") != std::string::npos);
			std::printf("  embercrest: home late, %s texted: %s\n", From.c_str(), Ph.Inbox[K].second.c_str());
		}
		Expect(Who.size() == 2 && Who[0] == Buster, "a day away still brings the night's two texts");
		Expect(!Stale && Tomorrow, "texts that come in late are said as of when they come: no invitation to a night already under way");
	}
	(void)Now;
}

/** Shifts, hustles, sleep, rent, Night Shift prizes, unlocks, bounties, satellites and tickets. */
void LifeChecks()
{
	namespace net = ss::net;
	Hooks H;
	ss::Session S(H, "life");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.5);
	const ss::Chips Start = S.BankrollCents;
	Expect(S.StartActivity("washfold").find("Next start") == 0, "the laundromat shift opens in the morning");
	Expect(S.StartActivity("quikstop").empty() && S.TimeSkip.Active, "a night shift at the Lucky Penny starts at 2:07 AM");
	Expect(!S.CanAfford(ss::Lobby()[0]), "no registering while at work");
	Now = Wait(S, Now, 6.0);
	Expect(!S.TimeSkip.Active && S.HasOutcome && S.Life.Shifts == 1, "the shift finishes");
	Expect(S.BankrollCents - Start >= 4350 && S.BankrollCents - Start <= 4950, "six hours at $7.25 plus tips");
	Expect(S.LobbyMinutes > 486.0 && S.LobbyMinutes < 488.0 && S.Life.Energy < 20.0, "it is 8:07 AM and the player is exhausted");
	Expect(S.StartActivity("pallet") == "Too tired. Sleep first.", "too tired for the warehouse");
	Expect(S.StartActivity("marcus-drop").find("Next start") == 0, "Marcus works at night");
	Expect(S.StartActivity("sam-ghost").find("Sam only hires") == 0, "Sam wants a player who cashes");
	Expect(S.StartActivity("sleep").empty(), "sleep");
	Now = Wait(S, Now, 6.0);
	Expect(S.Life.Energy > 99.0 && S.Daylight() > 0.99, "slept until the afternoon, fully rested");
	std::printf("  life: shift paid %s, slept until %s, energy %.0f\n", ss::Money(S.Life.EarnedJobs).c_str(), ss::ClockString(S.LobbyMinutes).c_str(), S.Life.Energy);
	S.LobbyMinutes = 20.0 * 60.0 + 30.0;
	Now = Wait(S, Now, 0.2);
	int Delivered = 0;
	int Picked = 0;
	for (int K = 0; K < 6 && S.Life.DebtCents == 0; ++K)
	{
		S.LobbyMinutes = 1440.0 + 21.0 * 60.0 + static_cast<double>(K) * 1440.0;
		Now = Wait(S, Now, 0.1);
		S.Life.Energy = 100.0; // (jumping the clock a day ahead drained it)
		const ss::Chips Before = S.BankrollCents;
		const std::string Why = S.StartActivity("marcus-drop");
		Expect(Why.empty(), "Marcus has a drop-off at 9 PM");
		Now = Wait(S, Now, 4.0);
		if (S.LastOutcome.Bad)
		{
			++Picked;
			Expect(S.Life.DebtCents == 20000 && S.BankrollCents <= Before && S.StartActivity("marcus-drop").find("Marcus wants") == 0, "picked up: a fine, and a debt before more work");
		}
		else
		{
			++Delivered;
			Expect(S.BankrollCents - Before >= 12000 && S.BankrollCents - Before <= 20000 && S.Life.Heat > 0.0, "a delivery pays and draws heat");
		}
	}
	std::printf("  life: Marcus: %d delivered, %d picked up, heat %.0f, debt %s\n", Delivered, Picked, S.Life.Heat, ss::Money(S.Life.DebtCents).c_str());
	if (S.Life.DebtCents > 0)
	{
		S.BankrollCents += 50000;
		Expect(S.PayDebt() && S.Life.DebtCents == 0, "paying Marcus back");
	}

	// Rent: collected at the deadline when the money is there.
	{
		Hooks Hr;
		ss::Session Rich(Hr, "rent-paid");
		Rich.CurrentScreen = ss::Screen::Lobby;
		Rich.BankrollCents = 130000;
		double T0 = Wait(Rich, 0.0, 0.2);
		Rich.LobbyMinutes = 4.0 * 1440.0 - 0.02; // Friday, a second before midnight
		T0 = Wait(Rich, T0, 2.0);
		Expect(Rich.Life.RentStage == ss::life::Rent::Paid && Rich.BankrollCents == 7500 && Rich.Life.RentDeadline == 27.0 * 1440.0, "the landlord collects the rent at midnight Friday");
		Hooks Hp;
		ss::Session Poor(Hp, "rent-missed");
		Poor.CurrentScreen = ss::Screen::Lobby;
		T0 = Wait(Poor, 0.0, 0.2);
		Poor.LobbyMinutes = 4.0 * 1440.0 + 1.0;
		T0 = Wait(Poor, T0, 0.2);
		Expect(Poor.Life.RentStage == ss::life::Rent::FinalNotice && Poor.Life.RentDueCents == 137500, "a missed rent becomes a final notice with a late fee");
		Poor.LobbyMinutes = 7.0 * 1440.0 + 1.0;
		T0 = Wait(Poor, T0, 0.2);
		Expect(Poor.Life.RentStage == ss::life::Rent::Evicted, "and then the locks change");
		Expect(Poor.Life.StorageDue == 0.0 && Poor.Life.RentDueCents == 137500 + ss::life::MonthlyRentCents, "nothing to put in storage; the back rent and a month to get back in");
	}

	// The Night Shift pays the top 20 at 6 AM.
	{
		Hooks Hn;
		ss::Session Night(Hn, "night-shift");
		Night.CurrentScreen = ss::Screen::Lobby;
		ss::HistoryEntry Won;
		Won.Name = "MM #26: Night Crawler";
		Won.Place = 1;
		Won.Entrants = 1184;
		Won.Prize = 45000;
		Won.BuyInCents = 220;
		Won.EventId = "mm-26@1530";
		ss::HistoryEntry Owl;
		Owl.Name = "$1.10 Night Owl Turbo";
		Owl.Place = 2;
		Owl.Entrants = 1000;
		Owl.Prize = 12000;
		Owl.BuyInCents = 110;
		Owl.EventId = "night-owl@1560";
		Night.History = {Owl, Won};
		Night.LobbyMinutes = 5.0 * 60.0 + 59.0;
		double T0 = Wait(Night, 0.0, 0.2);
		const ss::Chips Before = Night.BankrollCents;
		T0 = Wait(Night, T0, 70.0);
		Expect(Night.BankrollCents - Before >= 1650 && Night.Life.NightsPaid.count(1) == 1 && !Night.Life.Ledger.empty() && Night.Life.Ledger.front().Kind == 4, "a Night Shift top-20 finish pays at 6 AM");
		std::printf("  life: Night Shift prize %s at %s\n", ss::Money(Night.BankrollCents - Before).c_str(), ss::ClockString(Night.LobbyMinutes).c_str());
		Expect(net::StatsFrom("grinder_3c", Night.History, Night.WorldMinutes() + 13.0 * 60.0).NightPoints == 0.0, "the next night starts from zero");
	}

	// Unlocks: a cash opens bounties, a final table satellites, a title six-max.
	{
		Hooks Hu;
		ss::Session U(Hu, "unlocks");
		U.CurrentScreen = ss::Screen::Lobby;
		const net::Network& Net = net::Shared();
		net::EventInstance Pko;
		net::EventInstance Step1;
		net::EventInstance Step2;
		Expect(Net.FindInstance("hh-110@1500", Pko) && Net.FindInstance("step1@1530", Step1) && Net.FindInstance("step2@1620", Step2), "bounty and satellite events are scheduled");
		Expect(!Net.Listing(Pko, nullptr, U.Unlocks()).Joinable, "bounties start locked");
		U.Life.Unlocks = {"bounty", "satellite"};
		const ss::LobbyEvent L = Net.Listing(Pko, nullptr, U.Unlocks());
		Expect(L.Joinable && L.Spec.BountyCents == 50 && L.Spec.GuaranteeCents == 50000 && !L.Spec.MysteryBounty, "a $1.10 PKO puts 50 cents on every head");
		const ss::LobbyEvent Sat = Net.Listing(Step1, nullptr, U.Unlocks());
		Expect(Sat.Joinable && Sat.Spec.SeatValueCents == 1100 && Sat.Spec.SeatTicket == "step2", "Step 1 pays Step 2 tickets");
		U.BankrollCents = 0;
		U.Life.Tickets["step2"] = 1;
		const ss::LobbyEvent Sat2 = Net.Listing(Step2, nullptr, U.Unlocks());
		Expect(U.CanAfford(Sat2), "a ticket pays the buy-in");
		U.LobbyMinutes = 2.0 * 60.0 + 50.0;
		U.RegisterEvent(Sat2);
		Expect(U.T && U.BankrollCents == 0 && U.Life.TicketsFor("step2") == 0 && U.SeatsInPlay() == 10, "registered with the ticket: 60 players, 10 seats");
		// Play Step 2 out with a best-EV hero.
		double T0 = 0.0;
		int Frames = 0;
		while (U.CurrentScreen == ss::Screen::Table && ++Frames < 6000000)
		{
			T0 += 1.0 / 30.0;
			U.Update(T0);
			if (U.HasPrompt && T0 - U.Prompt.OpenedAt > 0.3)
			{
				const std::vector<ss::OptionEV>& Opts = U.Prompt.Analysis.Options;
				size_t Best = 0;
				for (size_t I = 1; I < Opts.size(); ++I)
				{
					Best = Opts[I].Ev > Opts[Best].Ev ? I : Best;
				}
				U.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
			}
		}
		const ss::Results& R = U.LastResults;
		Expect(U.CurrentScreen == ss::Screen::Results, "the satellite reaches results");
		Expect(R.SeatWon.empty() ? R.Place > 10 : (R.SeatWon == "step3" && U.Life.TicketsFor("step3") == 1 && R.Place <= 10), "seat winners get a Step 3 ticket, everyone else busts outside the seats");
		std::printf("  life: Step 2 with a ticket: %s of %d, %s\n", ss::Ordinal(R.Place).c_str(), R.Entrants, R.SeatWon.empty() ? "no seat" : "won a Step 3 ticket");
	}

	// Saves keep the life.
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed) && Parsed.Life.Shifts == H.Last.Life.Shifts && Parsed.Life.Ledger.size() == H.Last.Life.Ledger.size() &&
			   Parsed.Life.Runs == H.Last.Life.Runs && std::abs(Parsed.Life.Energy - H.Last.Life.Energy) < 0.01 && Parsed.Life.Ledger.front().Label == H.Last.Life.Ledger.front().Label,
		"life survives a save");
}

/** A progressive knockout played out: bounties change hands and the player's are paid. */
void BountyTournament()
{
	Hooks H;
	ss::Session S(H, "pko");
	S.CurrentScreen = ss::Screen::Lobby;
	S.Life.Unlocks.insert("bounty");
	S.BankrollCents = 10000;
	const ss::net::Network& Net = ss::net::Shared();
	ss::net::EventInstance Pko;
	Expect(Net.FindInstance("hh-110@1500", Pko), "the 1:00 AM Headhunter PKO is scheduled");
	const ss::LobbyEvent L = Net.Listing(Pko, nullptr, S.Unlocks());
	S.RegisterEvent(L);
	Expect(S.T && S.T->PrizePoolCents == std::max<ss::Chips>(50000, 50 * static_cast<ss::Chips>(L.Spec.Entrants)) && S.Bounties.size() == static_cast<size_t>(L.Spec.Entrants), "half the pool goes on heads");
	S.CurrentPace = ss::Pace::Sprint;
	ss::Rng Choice("pko-choices");
	int Decisions = 0;
	PlayOut(S, Choice, 0.0, Decisions);
	ss::Chips OnHeads = 0;
	for (const auto& B : S.Bounties)
	{
		OnHeads += B.second;
	}
	const ss::Results& R = S.LastResults;
	Expect(S.CurrentScreen == ss::Screen::Results && OnHeads <= 50 * static_cast<ss::Chips>(L.Spec.Entrants) && OnHeads > 0, "bounties move to the players who knock others out");
	Expect(R.BountyCents >= 0 && (R.Knockouts > 0 || R.BountyCents == 0), "the player's bounties are counted");
	std::printf("  pko: %s of %d, %d knockouts, %s in bounties, prize %s\n", ss::Ordinal(R.Place).c_str(), R.Entrants, R.Knockouts, ss::Money(R.BountyCents).c_str(), ss::Money(R.PrizeCents).c_str());
}

/** Several tournaments at once: all of them play on, the one in front changes, finished ones close, the last brings the results. */
void MultiTable()
{
	Hooks H;
	ss::Session S(H, "multi");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 20000 + 14900 + 28900;
	Expect(S.MaxTables() == 2, "the laptop's screen fits two tables");
	Expect(S.CanBuy("monitor-27") != "" && S.Buy("monitor-24").empty() && S.MaxTables() == 3 && S.Buy("monitor-27").empty(), "GearDrop: each monitor adds a table (the big one needs the first)");
	Expect(S.BankrollCents == 20000 && S.Life.Ledger.size() == 2 && S.Life.Ledger.front().Kind == 5, "monitors paid from the bankroll, in the ledger");
	const ss::Chips Start = S.BankrollCents;
	const ss::net::Network& Net = ss::net::Shared();
	const double World = S.WorldMinutes();
	std::vector<ss::LobbyEvent> Picks;
	for (const ss::net::EventInstance& E : Net.Window(World - 180.0, World + 90.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		if (L.Joinable && L.Spec.Entrants <= 600 && L.BuyInCents <= 1100 && Picks.size() < 5)
		{
			Picks.push_back(L);
		}
	}
	Expect(Picks.size() == 5, "five small events to pick from");
	if (Picks.size() < 5)
	{
		return;
	}
	ss::Session Tired(H, "tired");
	Tired.Life.Energy = 10.0;
	Expect(S.MaxTables() == ss::Session::TableLimit && Tired.MaxTables() == 2, "four tables, two when exhausted");

	S.RegisterEvent(Picks[0]);
	Expect(S.TableCount() == 1 && S.FocusedTable() == 0 && S.CurrentScreen == ss::Screen::Table, "the first table opens in front");
	double Now = Wait(S, 0.0, 30.0);
	const int Before = S.HandsPlayed;
	S.ShowLobby();
	Now = Wait(S, Now, 40.0);
	Expect(S.CurrentScreen == ss::Screen::Lobby && S.T && S.HandsPlayed > Before, "the table plays on while the player is in the lobby");
	S.RegisterEvent(Picks[0]);
	Expect(S.TableCount() == 1, "one seat per event");
	for (int I = 1; I < 4; ++I)
	{
		S.RegisterEvent(Picks[static_cast<size_t>(I)]);
	}
	Expect(S.TableCount() == 4 && S.FocusedTable() == 3 && S.CurrentScreen == ss::Screen::Table && S.T->Spec.Id == Picks[3].Spec.Id, "each new table opens in front");
	S.RegisterEvent(Picks[4]);
	Expect(S.TableCount() == 4 && !S.IsPlaying(Picks[4].Spec.Id), "four is the limit");
	ss::Chips Paid = 0;
	for (int I = 0; I < 4; ++I)
	{
		Paid += Picks[static_cast<size_t>(I)].BuyInCents;
		Expect(S.IsPlaying(Picks[static_cast<size_t>(I)].Spec.Id) && S.Glance(I).Name == Picks[static_cast<size_t>(I)].Spec.Name, "every table is open under its own name");
	}
	Expect(S.BankrollCents == Start - Paid, "four buy-ins");
	const double OpenedAt = S.ClockMinutes();
	for (int I = 0; I < 4; ++I)
	{
		S.WithTable(I, [&]() { Expect(S.T->Spec.StartClock >= Picks[static_cast<size_t>(I)].Spec.StartClock - 0.001, "tables keep their start"); });
	}

	// Play everything: act wherever it's the player's turn, through a tile's buttons (HeroActAt) half the time.
	ss::Rng Choice("multi-choices");
	int Decisions = 0;
	int Background = 0;
	int AutoSwitches = 0;
	int MostTables = 0;
	int Frames = 0;
	bool Sped = false;
	while (S.CurrentScreen == ss::Screen::Table && ++Frames < 6000000)
	{
		const int Front = S.FocusedTable();
		const int Count = S.TableCount();
		Now += 1.0 / 30.0;
		S.Update(Now);
		AutoSwitches += S.TableCount() == Count && S.FocusedTable() != Front ? 1 : 0;
		MostTables = std::max(MostTables, S.TableCount());
		if (!Sped && Now > 400.0)
		{
			// Enough of watching every hand: let the tables sprint to their milestones.
			Sped = true;
			for (int I = 0; I < S.TableCount(); ++I)
			{
				S.WithTable(I, [&]() { S.CurrentPace = ss::Pace::Sprint; });
			}
		}
		for (int I = 0; I < S.TableCount(); ++I)
		{
			const ss::TableGlance G = S.Glance(I);
			if (!G.YourTurn || Now - G.TurnOpenedAt < 0.4)
			{
				continue;
			}
			++Decisions;
			Background += I != S.FocusedTable() ? 1 : 0;
			const double Roll = Choice.Next();
			const ss::PlayerAction A = Roll < 0.3 ? ss::PlayerAction::Fold() : Roll < 0.4 ? ss::PlayerAction::RaiseTo(1e12) : ss::PlayerAction::Call();
			if (Choice.Chance(0.5))
			{
				S.HeroActAt(I, A);
			}
			else
			{
				S.FocusTable(I);
				S.HeroAct(A);
			}
			break;
		}
	}
	Expect(S.CurrentScreen == ss::Screen::Results, "the last table brings the results screen");
	Expect(S.Finished.size() == 3 && S.History.size() == 4, "three tables closed early, four results recorded");
	Expect(S.LastResults.SessionEvents == 4 && S.LastResults.SessionNetCents == S.BankrollCents - Start, "the results sum up the sitting");
	Expect(Decisions > 20 && Background > 0, "decisions came from tables in front and behind");
	Expect(AutoSwitches > 0, "a waiting table comes forward on its own");
	Expect(H.Sounds[ss::SoundId::Turn] > 0, "turns chime");
	Expect(S.ClockMinutes() >= OpenedAt, "the clock only moves forward");
	ss::Chips Prizes = 0;
	for (const ss::FinishedTable& F : S.Finished)
	{
		Prizes += F.Result.PrizeCents;
	}
	S.LeaveResults();
	Expect(S.TableCount() == 0 && !S.T && S.FocusedTable() == -1, "leaving the results closes the sitting");
	S.RegisterEvent(Picks[4]);
	Expect(S.TableCount() == 1 && S.FocusedTable() == 0, "a new sitting starts from one table");
	std::printf("  multi: 4 tables, %d decisions (%d away from the front), %d auto-switches, %s in early prizes, net %s\n", Decisions, Background, AutoSwitches, ss::Money(Prizes).c_str(),
		ss::Money(S.LastResults.SessionNetCents).c_str());
}

void Formatting()
{
	Expect(ss::Money(237) == "$2.37", "money small");
	Expect(ss::Money(122500) == "$1,225.00", "money grouped");
	Expect(ss::Money(-100) == "-$1.00", "money negative");
	Expect(ss::ChipsText(12345) == "12,345", "chips grouped");
	Expect(ss::ChipsText(1250000) == "1.25M", "chips millions");
	Expect(ss::ChipsText(12300000) == "12.3M", "chips tens of millions");
	Expect(ss::Ordinal(1) == "1st" && ss::Ordinal(2) == "2nd" && ss::Ordinal(3) == "3rd" && ss::Ordinal(4) == "4th", "ordinals 1-4");
	Expect(ss::Ordinal(11) == "11th" && ss::Ordinal(12) == "12th" && ss::Ordinal(13) == "13th", "ordinals teens");
	Expect(ss::Ordinal(21) == "21st" && ss::Ordinal(112) == "112th" && ss::Ordinal(1000) == "1,000th", "ordinals large");
	Expect(ss::ClockString(127) == "2:07 AM" && ss::ClockString(0) == "12:00 AM" && ss::ClockString(13 * 60 + 5) == "1:05 PM", "clock");
}
/** The character creator's person: validation, the save, and the background perks where the game honors them. */
void CharacterChecks()
{
	namespace hero = ss::hero;
	// Every background has its words, and the perks differ from neutral.
	for (int I = 0; I < hero::BackgroundCount; ++I)
	{
		const hero::BackgroundInfo& B = hero::InfoOf(static_cast<hero::Background>(I));
		Expect(B.Id == static_cast<hero::Background>(I) && hero::FindBackground(B.Key) == &B && std::string(B.Story).size() > 80 && std::string(B.PerkText).size() > 20, "backgrounds are complete");
	}
	Expect(hero::Countries().size() == 28 && hero::FindCountry("JP") && !hero::FindCountry("XX"), "28 countries with a home city each");
	Expect(hero::OptionName(hero::Slot::Height, 185) == "6'1\" \xC2\xB7 185 cm" && hero::OptionName(hero::Slot::Hair, 0) == "Shaved", "option names");

	// Validation.
	hero::Character C;
	Expect(C.Problem().empty() && !C.Created, "the default character is valid but not created");
	C.FirstName = "";
	Expect(!C.Problem().empty(), "a first name is required");
	C.FirstName = "Ana\tMaria";
	Expect(!C.Problem().empty(), "names can't carry tabs into the save");
	C.FirstName = "Jos\xC3\xA9";
	C.LastName = "O'Neill-Ruiz";
	Expect(C.Problem().empty(), "accents, apostrophes and hyphens are fine");
	C.Age = 17;
	Expect(!C.Problem().empty(), "18 or older");
	C.Age = 30;
	C.Country = "ZZ";
	Expect(!C.Problem().empty(), "a known country");

	// Looks clamp and wrap.
	hero::Look L;
	L.Set(hero::Slot::Hair, -1, true);
	L.Set(hero::Slot::Height, 400);
	L.Set(hero::Slot::Skin, 99);
	Expect(L.Hair == hero::OptionCount(hero::Slot::Hair) - 1 && L.Height == hero::MaxHeight && L.Skin == hero::OptionCount(hero::Slot::Skin) - 1, "look values wrap and clamp");

	// Random people are valid, varied and in range.
	ss::Rng R("hero");
	std::set<std::string> Names;
	bool AllValid = true;
	for (int I = 0; I < 300; ++I)
	{
		const hero::Character Rc = hero::Random(R);
		AllValid = AllValid && Rc.Problem().empty() && Rc.Created && Rc.Appearance.Height >= hero::MinHeight && Rc.Appearance.Height <= hero::MaxHeight;
		for (int K = 0; K < hero::SlotCount; ++K)
		{
			const hero::Slot Sl = static_cast<hero::Slot>(K);
			AllValid = AllValid && (Sl == hero::Slot::Height || (Rc.Appearance.Get(Sl) >= 0 && Rc.Appearance.Get(Sl) < hero::OptionCount(Sl)));
		}
		Names.insert(Rc.FullName());
	}
	Expect(AllValid && Names.size() > 150, "random characters are valid and varied");

	// The save: a new career for a line cook, saved, parsed and loaded.
	Hooks H;
	ss::Session S(H, "character");
	Expect(!S.Person.Created && S.Life.Perks.JobPay == 1.0, "a session with no character has neutral perks");
	hero::Character Cook = hero::Random(R);
	Cook.FirstName = "Rosa";
	Cook.LastName = "Delgado";
	Cook.Age = 31;
	Cook.Country = "MX";
	Cook.Story = hero::Background::Kitchen;
	Cook.Appearance.Hat = 2;
	S.NewCareer(Cook);
	Expect(S.Person.Created && S.Person.FullName() == "Rosa Delgado" && S.Life.Perks.JobPay > 1.0 && S.BankrollCents == 237, "a new career takes the character");
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed), "the save with a character parses");
	Expect(Parsed.Person.FullName() == "Rosa Delgado" && Parsed.Person.Age == 31 && Parsed.Person.Country == "MX" && Parsed.Person.Story == hero::Background::Kitchen &&
			Parsed.Person.Appearance == Cook.Appearance && Parsed.Person.Created,
		"the character survives the save");
	ss::Session Reloaded(H, "character", &Parsed);
	Expect(Reloaded.Person.Created && Reloaded.Life.Perks.JobPay > 1.0, "the perk comes back with the save");

	// The line cook's shift pays 20% more than the same shift for nobody in particular.
	const ss::life::Activity* Shift = ss::life::Find("quikstop");
	ss::life::State Plain;
	ss::Rng R1("shift");
	ss::Rng R2("shift");
	const ss::life::Outcome Base = ss::life::Resolve(*Shift, Plain, 0.0, R1, 1000);
	const ss::life::Outcome Cooked = ss::life::Resolve(*Shift, Reloaded.Life, 0.0, R2, 1000);
	Expect(Base.Money > 0 && std::llabs(Cooked.Money - static_cast<ss::Chips>(std::llround(static_cast<double>(Base.Money) * 1.2))) <= 1, "a line cook's shift pays 20% more");

	// The others.
	auto Career = [&](hero::Background B) {
		Hooks Hb;
		ss::Session Sb(Hb, "bg");
		hero::Character Who;
		Who.Story = B;
		Sb.NewCareer(Who);
		return std::make_pair(Sb.BankrollCents, Sb.Life);
	};
	const auto Fresh = Career(hero::Background::Newcomer);
	Expect(Fresh.first == 237 + 4000 && !Fresh.second.Ledger.empty() && Fresh.second.Ledger.front().Kind == 7, "a fresh start brings $40 of savings");
	Expect(Career(hero::Background::Dropout).second.Unlocks.count("bounty") == 1 && Career(hero::Background::Kitchen).second.Unlocks.empty(), "a stats dropout starts with bounty events open");
	const auto Hustle = Career(hero::Background::Hustler);
	const ss::life::Activity* Run = ss::life::Find("marcus-run");
	Expect(ss::life::RiskOf(*Run, Hustle.second) < ss::life::RiskOf(*Run, Plain) - 0.04 && Hustle.second.Perks.HeatCool > 1.0, "a corner hustler runs safer and cools faster");
	Expect(Career(hero::Background::Bouncer).second.Perks.TiltGain < 1.0 && Career(hero::Background::DealersKid).second.Perks.ReadWeight == 2, "a bouncer tilts less, a dealer's kid reads faster");

	// A save from before the creator: no hero lines, a neutral person.
	ss::SaveData Old;
	Expect(ss::SaveData::Parse("shortstack.nightone.v1\nbankroll\t500\nname\told_timer\n", Old) && !Old.Person.Created && Old.Person.FirstName == "Jesse", "older saves load without a character");
	// The casino's rename (a v1 save) leaves the character's own names alone, a sixteen-letter one included.
	ss::SaveData Named;
	Expect(ss::SaveData::Parse("shortstack.nightone.v1\nbankroll\t500\nname\tRiversideRat\nhero\tname\tRiverside-Smithy\tRiverside\nhero\tcreated\t1\nledger\t10.00\t0\t-500\tRiverside Sunday: buy-in\n",
			   Named) &&
			   Named.Person.FirstName == "Riverside-Smithy" && Named.Person.LastName == "Riverside" && Named.HeroName == "RiversideRat" && !Named.Life.Ledger.empty() &&
			   Named.Life.Ledger.front().Label == "Embercrest Sunday: buy-in",
		"a character named Riverside keeps the name through the rename");
	Expect(hero::Bio(Cook).find("Guadalajara, Mexico") != std::string::npos && hero::Bio(Cook).find("Rosa Delgado, 31") == 0, "the bio reads from the character");
	hero::Character Newcomer;
	Newcomer.Story = hero::Background::Newcomer;
	Expect(hero::Bio(Newcomer).find("$42.37 on RiverLine ($40 of it saved from home).") != std::string::npos, "the fresh start's bio counts the savings in the bankroll");
	hero::Look Greying;
	Greying.HairColor = 1;
	const uint32_t Grey = hero::HairToneAt(Greying, 57);
	Expect(hero::HairToneAt(Greying, 30) == hero::HairTone(1) && std::abs(static_cast<int>((Grey >> 16) & 0xffu) - static_cast<int>(Grey & 0xffu)) < 14,
		"dark brown greys to grey, not beige");
	bool HatsApart = true;
	for (int J = 0; J < 8; ++J)
	{
		for (int Hc = 0; Hc < 8; ++Hc)
		{
			hero::Look K;
			K.OutfitColor = J;
			K.HairColor = Hc;
			const uint32_t Hat = hero::HatTone(K, 57);
			const uint32_t Hr = hero::HairToneAt(K, 57);
			int Sum = 0;
			for (int Sh = 0; Sh <= 16; Sh += 8)
			{
				Sum += std::abs(static_cast<int>((Hat >> Sh) & 0xffu) - static_cast<int>((Hr >> Sh) & 0xffu));
			}
			HatsApart = HatsApart && Sum >= 80 && Hat != hero::OutfitTone(J);
		}
	}
	Expect(HatsApart, "a hat never vanishes into the hair or matches the jacket");
	// A new career resets the person too.
	Reloaded.ResetSave();
	Expect(!Reloaded.Person.Created && Reloaded.Life.Perks.JobPay == 1.0, "starting over clears the character");
}

/** Hunger and thirst over the hours, the corner store's counter, and the bag. */
void StoreChecks()
{
	namespace store = ss::store;
	Expect(store::Catalog().size() >= 12 && store::Find("volt-rush") && !store::Find("nope"), "the Lucky Penny is stocked");
	for (int Sh = 0; Sh < store::ShelfCount; ++Sh)
	{
		int Count = 0;
		for (const store::Item& I : store::Catalog())
		{
			Count += static_cast<int>(I.Where) == Sh ? 1 : 0;
		}
		Expect(Count >= 2 && Count <= 6, "every shelf has a few things (and fits the counter's grid)");
	}
	store::Basket B;
	B.Add("volt-rush", 2);
	B.Add("roller-dog");
	B.Add("volt-rush");
	B.Add("nope");
	Expect(B.Count() == 4 && B.Lines.size() == 2 && B.Subtotal() == 3 * 299 + 199 && B.Total() == B.Subtotal() + store::Tax(B.Subtotal()), "a basket adds up, with tax");
	B.Remove("volt-rush");
	Expect(B.Count() == 3, "putting one back");
	Expect(store::Tax(1000) == 73, "7.25% tax");
	Expect(store::Tax(200) == 15 && store::Tax(3000) == 218 && store::Tax(0) == 0 && store::Tax(1) == 0 && store::Tax(7) == 1, "half a cent rounds up, every time (in whole cents, not a double's 7.25%)");
	store::ClerkContext Walk;
	Walk.World = 130.0;
	Walk.Hunger = 30.0;
	Walk.Energy = 60.0;
	Expect(!store::ClerkLine(Walk, store::Basket()).empty() && store::ClerkLine(Walk, B).find("Two of those") == 0, "Benny has something to say");
	Walk.OnSchedule = true;
	Expect(store::ClerkLine(Walk, store::Basket()).find("on the schedule") != std::string::npos, "Benny knows who's on the schedule this week");
	Walk.Hunger = 80.0;
	Expect(store::ClerkLine(Walk, store::Basket()).find("roller dog") != std::string::npos, "but a hungry face comes first");

	// Drunk or eaten by the shelf it sits on (a cola's sugar doesn't make it a meal); what one does, perks and all.
	Expect(store::Find("fizz-cola")->Drink() && store::Find("hot-cocoa")->Drink() && store::Find("sunny-peach")->Drink() && store::Find("hilltop-chips")->Food() &&
			   !store::Find("roller-dog")->Drink(),
		"a cola, a cocoa and an iced tea are drinks; chips are food");
	{
		const store::Relief Volt = store::ReliefOf(*store::Find("volt-rush"), 1.25);
		const store::Relief Cola = store::ReliefOf(*store::Find("fizz-cola"), 1.25);
		const store::Relief Dog = store::ReliefOf(*store::Find("roller-dog"), 1.25);
		Expect(std::fabs(Volt.Energy - 18.0 * 1.25) < 1e-9 && std::fabs(Cola.Energy - 6.0 * 1.25) < 1e-9 && std::fabs(Cola.Hunger - 4.0) < 1e-9 && std::fabs(Dog.Hunger - 30.0 * 1.25) < 1e-9 &&
				   std::fabs(Dog.Thirst + 2.0) < 1e-9 && std::fabs(Dog.Energy - 3.0 * 1.25) < 1e-9,
			"a line cook gets more energy out of food and drink, and more out of a meal (not out of a cola)");
		Expect(store::Called(*store::Find("hilltop-chips")) == "the Hilltop chips" && store::Called(*store::Find("cascade")) == "the Cascade", "the shelf's names in a sentence");
	}

	// Needs climb with the hours; past 70 they cost energy.
	Hooks H;
	ss::Session S(H, "store");
	S.CurrentScreen = ss::Screen::Lobby;
	S.Life.Hunger = 40.0;
	S.Life.Thirst = 40.0;
	S.Life.Energy = 80.0;
	S.WorldSkip(0);
	double Now = 0.0;
	S.Update(Now);
	const double Hunger0 = S.Life.Hunger;
	const double Energy0 = S.Life.Energy;
	S.LobbyMinutes += 120.0;
	S.Update(Now += 0.5);
	Expect(S.Life.Hunger > Hunger0 + 5.0 && S.Life.Thirst > 40.0 + 7.0 && S.Life.Energy < Energy0, "two hours make you hungrier, thirstier and more tired");
	S.Life.Hunger = 95.0;
	S.Life.Thirst = 95.0;
	ss::life::State Calm = S.Life;
	Calm.Hunger = 20.0;
	Calm.Thirst = 20.0;
	Expect(ss::life::NeedsDrain(S.Life) > 4.0 && ss::life::NeedsDrain(Calm) == 0.0, "starving and parched drain energy; fed and watered don't");
	Expect(std::string(ss::life::HungerWord(95.0)) == "Starving" && std::string(ss::life::ThirstWord(10.0)) == "Hydrated", "needs have words");

	// The counter: pay, then eat and drink from the bag.
	S.BankrollCents = 1000;
	store::Basket Snack;
	Snack.Add("roller-dog");
	Snack.Add("cascade");
	const ss::Chips Total = Snack.Total();
	Expect(S.Checkout(Snack).empty() && S.BankrollCents == 1000 - Total && S.Life.Pantry["roller-dog"] == 1 && S.Life.Pantry["cascade"] == 1, "checkout pays and bags it");
	Expect(!S.Life.Ledger.empty() && S.Life.Ledger.front().Kind == 8 && S.Life.Ledger.front().Amount == -Total, "the bank shows the corner store");
	Expect(S.Consume("roller-dog").empty() && S.Life.Hunger < 95.0 - 25.0 && S.Life.Pantry.count("roller-dog") == 0, "a hot dog takes the edge off");
	// What it did, said the way it happened (past tense, what each meter actually moved; a real minus sign).
	Expect(S.LastEaten.ItemId == "roller-dog" && S.LastEaten.Line.find("Ate the Roller Dog. Hunger \xE2\x88\x92" "30") == 0 && S.LastEaten.Line.find("thirst +2") != std::string::npos,
		"the toast says what eating it did");
	Expect(S.Consume("cascade").empty() && S.Life.Thirst < 95.0 - 40.0, "water helps");
	Expect(S.LastEaten.Line.find("Drank the Cascade. Thirst \xE2\x88\x92" "45") == 0 && S.LastEaten.Thirst < -44.0, "and drinking it");
	Expect(!S.Consume("cascade").empty(), "can't drink what you don't have");
	store::Basket Big;
	Big.Add("egg-salad", 9);
	S.BankrollCents = 500;
	Expect(S.Checkout(Big) == "Card declined." && S.BankrollCents == 500 && S.Life.Pantry.empty(), "a declined card takes nothing");
	Expect(!S.Checkout(store::Basket()).empty(), "an empty basket doesn't check out");

	// What the eat key reaches for: the drink when thirst is worse, the food when hunger is.
	store::Basket Both;
	Both.Add("roller-dog");
	Both.Add("cascade");
	S.BankrollCents = 2000;
	S.Checkout(Both);
	S.Life.Hunger = 30.0;
	S.Life.Thirst = 80.0;
	Expect(S.BagPick() == "cascade", "thirsty: reach for the water");
	S.Life.Hunger = 80.0;
	S.Life.Thirst = 30.0;
	Expect(S.BagPick() == "roller-dog", "hungry: reach for the hot dog");
	S.Consume("roller-dog");
	S.Consume("cascade");
	Expect(S.BagPick().empty(), "nothing to reach for in an empty bag");

	// The eat key: never the best thing in the bag by accident, never what makes the worse need worse, and it says why not.
	{
		Hooks Hb;
		ss::Session Sb(Hb, "bag");
		Sb.Life.Pantry.clear();
		Expect(Sb.EatFromBag() == "Nothing in it. The Lucky Penny's on the corner.", "an empty bag says so");
		Sb.Life.Pantry["egg-salad"] = 1;
		Sb.Life.Hunger = 5.0;
		Sb.Life.Thirst = 3.0;
		Sb.Life.Energy = 70.0;
		Expect(Sb.BagPick().empty() && Sb.EatFromBag() == "You're not hungry or thirsty." && Sb.Life.Pantry["egg-salad"] == 1, "fed and watered, a tap of the key doesn't eat the sandwich");
		Sb.Life.Energy = 30.0;
		Expect(Sb.BagPick().empty() && Sb.EatFromBag() == "Not hungry, not thirsty. Just tired, and nothing in here fixes that." && Sb.Life.Pantry["egg-salad"] == 1,
			"tired, but a sandwich isn't worth eating for five energy (and the key says so)");
		Sb.Life.Pantry.clear();
		Sb.Life.Pantry["hilltop-chips"] = 1;
		Sb.Life.Hunger = 20.0;
		Sb.Life.Thirst = 90.0;
		Sb.Life.Energy = 70.0;
		Expect(Sb.BagPick().empty() && Sb.EatFromBag() == "Nothing in here for that thirst." && Sb.Life.Pantry["hilltop-chips"] == 1, "parched: salt and vinegar isn't the answer");
		Sb.Life.Hunger = 50.0;
		Sb.Life.Thirst = 49.5;
		Sb.Life.Energy = 90.0;
		Expect(Sb.EatFromBag() == "The Hilltop chips would only make you thirstier." && Sb.Life.Pantry["hilltop-chips"] == 1, "nearly as thirsty as hungry: the chips would make it worse");
		Sb.Life.Pantry.erase("hilltop-chips");
		Sb.Life.Pantry["egg-salad"] = 1;
		Sb.Life.Hunger = 21.0;
		Sb.Life.Thirst = 10.0;
		Expect(Sb.EatFromBag() == "Not hungry enough for the Deli Wedge yet." && Sb.Life.Pantry["egg-salad"] == 1, "a big sandwich at the first rumble would mostly go to waste");
		Sb.Life.Hunger = 60.0;
		Sb.Life.Thirst = 30.0;
		Sb.Life.Energy = 50.0;
		Expect(Sb.EatFromBag().empty() && Sb.LastEaten.ItemId == "egg-salad" && Sb.LastEaten.Line == "Ate the Deli Wedge. Hunger \xE2\x88\x92" "42, energy +5." && Sb.Life.Pantry.count("egg-salad") == 0,
			"hungry: the sandwich, and the line says what it did");
	}

	// The bag and the needs survive a save.
	store::Basket Keep;
	Keep.Add("volt-rush", 2);
	S.BankrollCents = 2000;
	S.Checkout(Keep);
	S.Life.Hunger = 61.5;
	S.Save();
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed) && Parsed.Life.Pantry["volt-rush"] == 2 && std::fabs(Parsed.Life.Hunger - 61.5) < 0.01, "the bag and the needs are saved");

	// A line cook makes more of a meal.
	auto Fed = [&](ss::hero::Background Story) {
		Hooks Hc;
		ss::Session Sc(Hc, "meal");
		ss::hero::Character Who;
		Who.Story = Story;
		Sc.NewCareer(Who);
		Sc.Life.Hunger = 90.0;
		Sc.BankrollCents = 1000;
		store::Basket Dog;
		Dog.Add("bean-burrito");
		Sc.Checkout(Dog);
		Sc.Consume("bean-burrito");
		return Sc.Life.Hunger;
	};
	Expect(Fed(ss::hero::Background::Kitchen) < Fed(ss::hero::Background::Newcomer) - 5.0, "a line cook's meals go further");
	// And the perk card's other half: food and drink both restore more energy.
	auto Buzz = [&](ss::hero::Background Story) {
		Hooks Hc;
		ss::Session Sc(Hc, "buzz");
		ss::hero::Character Who;
		Who.Story = Story;
		Sc.NewCareer(Who);
		Sc.Life.Energy = 30.0;
		Sc.Life.Pantry["volt-rush"] = 1;
		Sc.Consume("volt-rush");
		return Sc.Life.Energy;
	};
	Expect(std::fabs(Buzz(ss::hero::Background::Kitchen) - (30.0 + 18.0 * 1.25)) < 1e-9 && std::fabs(Buzz(ss::hero::Background::Newcomer) - 48.0) < 1e-9, "a line cook gets more out of a Volt Rush too");

	// A renamed item left in an old bag doesn't sit there unseen.
	{
		ss::SaveData Renamed;
		Expect(ss::SaveData::Parse("shortstack.nightone.v2\npantry\tcascade\t2\npantry\tgone-snack\t3\nbought\tgone-snack\t5\nbought\tcascade\t4\n", Renamed) && Renamed.Life.Pantry.size() == 1 &&
				   Renamed.Life.Pantry["cascade"] == 2 && Renamed.Life.Bought.size() == 1 && Renamed.Life.Bought["cascade"] == 4,
			"only what the shelves still sell loads into the bag");
	}

	// Benny: the schedule is this week's (the Lucky Penny's own shifts), a declined card, back within the hour, the usual.
	{
		Hooks Hk;
		ss::Session Sk(Hk, "benny");
		Sk.Life.Hunger = 30.0;
		Sk.Life.Energy = 60.0;
		const double World = Sk.WorldMinutes();
		Sk.Life.Record(World - 8.0 * ss::net::MinutesPerDay, "Lucky Penny #212 shift", 4500, 1);
		Sk.Life.Record(World - 2.0 * ss::net::MinutesPerDay, "Wash & Fold shift", 4800, 1);
		Expect(Sk.ClerkSays(store::Basket()).find("schedule") == std::string::npos, "a shift last week, or at the laundromat, isn't this week's schedule");
		Sk.Life.Record(World - 1.0 * ss::net::MinutesPerDay, "Lucky Penny #212 shift", 4500, 1);
		Expect(Sk.ClerkSays(store::Basket()).find("on the schedule") != std::string::npos, "a shift behind the counter yesterday is");
		Sk.Life.Ledger.clear();
		store::Basket Pricey;
		Pricey.Add("egg-salad", 9);
		Sk.BankrollCents = 100;
		Expect(Sk.Checkout(Pricey) == "Card declined." && Sk.ClerkSays(store::Basket()).find("Card's having a night") == 0, "Benny saw the card say no");
		Sk.BankrollCents = 10000;
		store::Basket Noodles;
		Noodles.Add("oodle-cup", 3);
		Expect(Sk.Checkout(Noodles).empty() && Sk.ClerkSays(store::Basket()) == "Back already? Forget something?" && Sk.Life.Bought["oodle-cup"] == 3, "paid up: back within the hour");
		Sk.LobbyMinutes = 6.0 * 60.0 + 10.0; // an even hour, four hours on
		Expect(Sk.ClerkSays(store::Basket()).find("The usual? Oodle Cup.") == 0, "three of the same makes it the usual");
		Sk.LobbyMinutes = 7.0 * 60.0 + 10.0;
		Expect(Sk.ClerkSays(store::Basket()) == "Morning. Coffee's on the left.", "and the rest of the time he just says hello");
		Sk.Save();
		ss::SaveData Kept;
		Expect(ss::SaveData::Parse(Hk.Last.Serialize(), Kept) && Kept.Life.Bought["oodle-cup"] == 3 && Kept.Serialize() == Hk.Last.Serialize(), "what the player buys is in the save");
	}
}

/** Penny Drop: the store's delivery app on the laptop, the order arriving with the clock, and the kitchen tap. */
void DeliveryChecks()
{
	namespace store = ss::store;
	for (const store::Item& I : store::Catalog())
	{
		const ss::Chips App = store::AppPrice(I);
		Expect(App >= I.PriceCents * 115 / 100 && App % 10 == 9 && App <= I.PriceCents * 115 / 100 + 10, "app prices run 15% over the shelf, ending in 9");
	}
	store::Basket Lunch;
	Lunch.Add("bean-burrito");
	Lunch.Add("fizz-cola", 2);
	const ss::Chips Goods = Lunch.AppSubtotal() + store::DeliveryFeeCents;
	Expect(store::DeliveryTotal(Lunch) == Goods + store::Tax(Goods) && store::DeliveryTotal(Lunch) > Lunch.Total() + store::DeliveryFeeCents, "delivery costs the app's prices, the fee and tax: more than walking");

	Hooks H;
	ss::Session S(H, "drop");
	S.CurrentScreen = ss::Screen::Lobby;
	S.WorldSkip(0);
	double Now = 0.0;
	S.Update(Now);
	S.BankrollCents = 300;
	Expect(S.PlaceOrder(store::Basket()) == "Your cart is empty.", "an empty cart doesn't order");
	store::Basket Tiny;
	Tiny.Add("oodle-cup");
	Expect(S.PlaceOrder(Tiny).find("Delivery starts at") == 0, "a delivery minimum");
	Expect(S.PlaceOrder(Lunch) == "Card declined." && S.BankrollCents == 300 && S.Life.Deliveries.empty(), "a declined card orders nothing");
	S.BankrollCents = 5000;
	const double Ordered = S.WorldMinutes();
	const ss::Chips Cost = store::DeliveryTotal(Lunch);
	Expect(S.PlaceOrder(Lunch).empty() && S.BankrollCents == 5000 - Cost, "an order is paid up front");
	Expect(S.Life.Deliveries.size() == 1 && S.Life.Deliveries[0].ArriveAt >= Ordered + 25.0 && S.Life.Deliveries[0].ArriveAt <= Ordered + 53.0, "it's 25 to 45 minutes out (a little more in the dead hours)");
	Expect(!S.Life.Ledger.empty() && S.Life.Ledger.front().Kind == 8 && S.Life.Ledger.front().Amount == -Cost && S.Life.Ledger.front().Label.find("Penny Drop") == 0, "the bank shows the order");
	Expect(S.Life.Pantry.empty(), "nothing's in the bag until it arrives");
	S.Save();
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed) && Parsed.Life.Deliveries.size() == 1 && Parsed.Life.Deliveries[0].Lines.size() == 2 &&
			   std::fabs(Parsed.Life.Deliveries[0].ArriveAt - S.Life.Deliveries[0].ArriveAt) < 0.01 && Parsed.Life.Orders == 1,
		"an order on the way is saved");
	const int Texts = H.Texts;
	S.LobbyMinutes += 60.0;
	S.Update(Now += 0.5);
	Expect(S.Life.Deliveries.empty() && S.Life.Pantry["bean-burrito"] == 1 && S.Life.Pantry["fizz-cola"] == 2 && H.Texts > Texts, "the order arrives at the door, into the bag, with a text");
	Expect(H.Got(0, "Penny Drop", "Left at your door: Big Bean, 2 Fizz Cola."), "the text says what came");
	{
		// The game closed right after: the save has it in the bag and off the road, so the next load can't bring it twice.
		bool Keyed = false;
		for (const std::string& Key : H.Last.TextsSeen)
		{
			Keyed = Keyed || Key.rfind("drop:", 0) == 0;
		}
		ss::SaveData Kept;
		Expect(ss::SaveData::Parse(H.Last.Serialize(), Kept) && Kept.Life.Deliveries.empty() && Kept.Life.Pantry["fizz-cola"] == 2 && !Keyed, "the save after it arrives is whole");
		Hooks H2;
		ss::Session Again(H2, "drop-again", &Kept);
		Again.CurrentScreen = ss::Screen::Lobby;
		Wait(Again, 0.0, 0.3);
		Expect(Again.Life.Pantry["fizz-cola"] == 2 && Again.Life.Pantry["bean-burrito"] == 1 && !H2.Got(0, "Penny Drop", "Left at your door"), "loaded again, it isn't delivered twice");
	}
	Expect(S.Consume("bean-burrito").empty(), "and you can eat it at home");

	// Three on the way at most.
	S.BankrollCents = 100000;
	Expect(S.PlaceOrder(Lunch).empty() && S.PlaceOrder(Lunch).empty() && S.PlaceOrder(Lunch).empty(), "three orders at once");
	Expect(!S.PlaceOrder(Lunch).empty() && S.Life.Deliveries.size() == 3, "not a fourth");

	// The kitchen tap: free, a little at a time, and only so far.
	S.Life.Thirst = 70.0;
	Expect(S.DrinkTapWater().empty() && std::fabs(S.Life.Thirst - 55.0) < 0.01, "a glass of tap water");
	Expect(!S.DrinkTapWater().empty() && S.TapWait() > 0.0, "not another one straight away");
	S.LobbyMinutes += 50.0;
	S.Update(Now += 0.5);
	Expect(S.TapWait() == 0.0 && S.DrinkTapWater().empty(), "the tap's there again later");
	S.Life.Thirst = 45.0;
	S.Life.TapAt = -1.0e9;
	Expect(S.DrinkTapWater().empty() && std::fabs(S.Life.Thirst - ss::Session::TapFloor) < 0.01, "the tap takes the edge off, no further");
	S.Life.TapAt = -1.0e9;
	Expect(S.DrinkTapWater() == "You're not thirsty.", "not when you're not thirsty");
}

/**
 * Penny Drop's edges: two orders landing together are never saved half-delivered, and an order that lands after the
 * locks change goes to Dee's, its text after the landlord's.
 */
void DeliveryRules()
{
	namespace store = ss::store;
	const double Day = ss::net::MinutesPerDay;
	struct DropHooks : Hooks
	{
		bool Both = false;
		void Save(const ss::SaveData& D) override
		{
			Hooks::Save(D);
			const auto Have = D.Life.Pantry.find("cascade");
			Both = Both || (Have != D.Life.Pantry.end() && Have->second > 0 && !D.Life.Deliveries.empty());
		}
	};
	DropHooks H;
	ss::Session S(H, "drop-rules");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	auto At = [&](double World) {
		S.LobbyMinutes += World - S.WorldMinutes();
		Now = Wait(S, Now, 0.1);
	};
	S.BankrollCents = 100000;
	store::Basket Water;
	Water.Add("cascade", 4);
	Expect(S.DeliveryAddress() == "1812 Fifth St, Apt 3B" && S.PlaceOrder(Water).empty() && S.PlaceOrder(Water).empty(), "two orders to the apartment");
	S.LobbyMinutes += 60.0;
	Now = Wait(S, Now, 0.2);
	Expect(S.Life.Deliveries.empty() && S.Life.Pantry["cascade"] == 8 && !H.Both, "two orders land together, and no save has one in the bag and on the road at once");
	Expect(H.Got(0, "Penny Drop", "Left at your door: 4 Cascade."), "left at the door");

	// Ten minutes before the locks change, an order: it lands at Dee's, after the landlord's text.
	At(5.0 * Day + 1.0);
	Expect(S.Life.RentStage == ss::life::Rent::FinalNotice, "the rent's late");
	At(8.0 * Day - 10.0);
	S.Life.Pantry.clear(); // (drunk; the hooks watch the bag for this order's water)
	Expect(!S.Evicted() && S.PlaceOrder(Water).empty(), "an order at ten to midnight on the last day");
	const size_t Mark = H.Inbox.size();
	At(8.0 * Day + 60.0);
	size_t Locks = std::string::npos;
	size_t Drop = std::string::npos;
	for (size_t I = Mark; I < H.Inbox.size(); ++I)
	{
		Locks = H.Inbox[I].first == "Landlord" && H.Inbox[I].second.rfind("Locks changed", 0) == 0 ? I : Locks;
		Drop = H.Inbox[I].first == "Penny Drop" ? I : Drop;
	}
	Expect(S.Evicted() && Locks != std::string::npos && Drop != std::string::npos && Drop > Locks && H.Inbox[Drop].second.rfind("Handed to Dee at the door: 4 Cascade.", 0) == 0,
		"locked out: the order goes to Dee's, and its text comes after the landlord's");
	Expect(S.Life.Pantry["cascade"] == 4 && S.DeliveryAddress() == "Dee's place" && !H.Both, "it's in the bag all the same");
}

/**
 * Pacing: a week of a sensible player who eats and drinks when they're getting hungry or thirsty, walks to the
 * Lucky Penny when the bag's empty, and sleeps eight hours a night. The needs should take a couple of meals and
 * drinks a day (a few dollars at the shelf), never get bad enough to drain energy, and a store run a day covers it.
 */
void NeedsPacing()
{
	namespace store = ss::store;
	Hooks H;
	ss::Session S(H, "pacing");
	ss::hero::Character Who;
	S.NewCareer(Who);
	Expect(S.Life.Pantry["oodle-cup"] == 2, "two noodle cups in the cupboard on night one");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 1000000;
	double Now = Wait(S, 0.0, 0.2);
	ss::Chips Spent = 0;
	int Trips = 0;
	int Taps = 0;
	int Eaten = 0;
	double Worst = 0.0;
	auto Tend = [&] {
		Worst = std::max({Worst, S.Life.Hunger, S.Life.Thirst});
		if (S.Life.Thirst >= 55.0 && S.DrinkTapWater().empty())
		{
			++Taps;
		}
		const bool NeedFood = S.Life.Hunger >= 55.0;
		const bool NeedDrink = S.Life.Thirst >= 60.0;
		auto Has = [&](const char* Id) { return S.Life.Pantry.count(Id) > 0 && S.Life.Pantry[Id] > 0; };
		if ((NeedFood && !Has("oodle-cup") && !Has("bean-burrito")) || (NeedDrink && !Has("cascade")))
		{
			// The bag's empty: one walk to the store for the day's food and drink.
			store::Basket Run;
			Run.Add("bean-burrito", 2);
			Run.Add("cascade", 2);
			const ss::Chips Before = S.BankrollCents;
			Expect(S.Checkout(Run).empty(), "a store run");
			Spent += Before - S.BankrollCents;
			++Trips;
		}
		if (NeedFood)
		{
			Eaten += S.Consume(Has("oodle-cup") ? "oodle-cup" : "bean-burrito").empty() ? 1 : 0;
		}
		if (NeedDrink)
		{
			Eaten += S.Consume("cascade").empty() ? 1 : 0;
		}
	};
	const double Start = S.WorldMinutes();
	for (int Day = 0; Day < 7; ++Day)
	{
		for (int Hour = 0; Hour < 16; ++Hour)
		{
			S.LobbyMinutes += 60.0;
			Now = Wait(S, Now, 0.05);
			S.Life.Energy = 80.0; // (the grind's own drain isn't what this measures)
			Tend();
		}
		Expect(S.StartActivity("sleep").empty(), "bed");
		Now = Wait(S, Now, 6.0);
		Tend();
	}
	const double Days = (S.WorldMinutes() - Start) / ss::net::MinutesPerDay;
	std::printf("  needs: %.1f days, %d store runs, %s spent (%s a day), %d meals and drinks, %d glasses of tap water, worst need %.0f\n", Days, Trips, ss::Money(Spent).c_str(),
		ss::Money(static_cast<ss::Chips>(static_cast<double>(Spent) / Days)).c_str(), Eaten, Taps, Worst);
	Expect(Days > 6.5 && Days < 8.0, "a week went by");
	Expect(Worst < 70.0, "a sensible player never gets hungry or thirsty enough to lose energy");
	Expect(Trips >= 3 && Trips <= 10, "about a store run a day, or less");
	Expect(Spent >= 1000 && Spent <= 5000, "food runs a few dollars a day at the shelf");
}

/**
 * Rent, month after month: collected on time, the week-out notice and the reminders, paid late without the next
 * month moving, the final notice, the locks, Dee's couch, the storage unit (renewed, then auctioned), and moving
 * back in, with and without the gear.
 */
void RentChecks()
{
	using ss::life::Rent;
	const double Day = ss::net::MinutesPerDay;
	auto At = [](ss::Session& S, double& Now, double World) {
		S.LobbyMinutes += World - S.WorldMinutes();
		Now = Wait(S, Now, 0.1);
	};
	auto Fed = [](ss::Session& S) {
		S.Life.Hunger = 0.0;
		S.Life.Thirst = 0.0;
	};

	// On time, every month.
	{
		Hooks H;
		ss::Session S(H, "rent-months");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 130000;
		At(S, Now, 5.0 * Day - 0.02);
		Now = Wait(S, Now, 2.0);
		Expect(S.Life.RentStage == Rent::Paid && S.BankrollCents == 7500 && S.Life.RentDeadline == 27.0 * Day && S.Life.Ledger.front().Label == "Rent (collected)",
			"the landlord collects the first month at midnight Friday; the next is due October 31");
		At(S, Now, 20.0 * Day - 60.0);
		Expect(S.Life.RentStage == Rent::Paid, "paid up until a week out");
		size_t Mark = H.Inbox.size();
		At(S, Now, 20.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::Due && H.Got(Mark, "Landlord", "Reminder: rent is $1,075.00, due by midnight Saturday, Oct 31"), "a week out the next month is due, and the landlord says when");
		Mark = H.Inbox.size();
		At(S, Now, 27.0 * Day - 39.0 * 60.0 + 1.0);
		Expect(H.Got(Mark, "Landlord", "due tomorrow at midnight"), "a reminder the morning before");
		At(S, Now, 27.0 * Day - 6.0 * 60.0 + 1.0);
		Expect(H.Got(Mark, "Landlord", "Midnight tonight. $1,075"), "and the evening it's due");

		// Missed: the final notice. Paid in the grace days, the next month is still due on its own day.
		S.BankrollCents = 0;
		Mark = H.Inbox.size();
		At(S, Now, 27.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::FinalNotice && S.Life.RentDueCents == ss::life::MonthlyRentCents + ss::life::LateFeeCents && S.Life.RentDeadline == 30.0 * Day,
			"missed: a final notice, the late fee, three days");
		Expect(H.Got(Mark, "Landlord", "until Tuesday at midnight"), "the final notice names the day");
		Mark = H.Inbox.size();
		At(S, Now, 30.0 * Day - 39.0 * 60.0 + 1.0);
		Expect(H.Got(Mark, "Landlord", "the locks change"), "one last warning the morning before");
		S.BankrollCents = 300000;
		Expect(S.PayRent() && S.Life.RentStage == Rent::Paid && S.BankrollCents == 300000 - 122500 && S.Life.RentDeadline == 57.0 * Day && S.Life.Ledger.front().Label == "Rent + late fee",
			"paid late: the next month doesn't move back (no drift)");
		At(S, Now, 57.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::Paid && S.Life.RentsPaid == 3 && S.Life.RentDeadline == 87.0 * Day && S.BankrollCents == 300000 - 122500 - 107500,
			"and thirty days on it's collected on time");
		// Paying ahead.
		S.BankrollCents = 200000;
		Expect(S.PayRent() && S.Life.RentDeadline == 117.0 * Day && S.Life.Ledger.front().Label == "Rent (paid ahead)", "a month paid ahead");
		Expect(S.MoveBackIn() == "You still have the apartment.", "nothing to move back into");
	}

	// Evicted with a rig, then the couch, the storage unit, and the auction.
	{
		Hooks H;
		ss::Session S(H, "rent-evicted");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 100000;
		Expect(S.Buy("ram-32").empty() && S.Buy("monitor-24").empty() && S.Buy("mattress").empty(), "a rig and a better bed");
		const int HomeTables = S.GearFx().Tables;
		Expect(HomeTables == 3 && S.GearFx().CanStream() && S.RestFactor() > 1.0, "at home: three tables, a stream, a good night's sleep");
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::FinalNotice && S.Life.RentDueCents == 137500 && S.Life.RentDeadline == 8.0 * Day, "late");
		At(S, Now, 8.0 * Day - 10.0);
		Fed(S);
		Expect(S.GoLive().empty() && S.Streaming(), "live as the clock runs out");
		size_t Mark = H.Inbox.size();
		At(S, Now, 8.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::Evicted && S.Life.RentDueCents == 137500 + ss::life::MonthlyRentCents && S.Life.Evictions == 1 && S.Life.EvictedAt == 8.0 * Day,
			"the locks change: what's owed, plus a month up front to get back in");
		Expect(H.Got(Mark, "Landlord", "You owe $1,375") && H.Got(Mark, "Landlord", "Ninth Street Storage") && H.Got(Mark, "Dee", "couch"), "the landlord says what it takes; Dee offers the couch");
		Expect(!S.Streaming() && !S.GearFx().CanStream() && S.GearFx().Tables == 2 && S.GearFx().Rest == 0.0 && S.Owns("monitor-24") && S.Life.StorageDue == 38.0 * Day,
			"the gear's in storage (still yours): the laptop, two tables, no stream");
		Expect(S.GoLive() == "No apartment, no internet." && !S.PayRent(), "no stream, and no rent on an apartment you're locked out of");
		Expect(S.CanBuy("webcam-720").find("Nowhere") == 0 && S.CanBuy("gym").find("Nowhere") == std::string::npos, "nowhere to put new gear (subscriptions still sell)");
		Expect(S.CanBuy("fiber").find("No apartment") == 0, "but not an internet line for an apartment you're locked out of");
		Expect(S.MaxTables() == 2, "the laptop still plays two tables");
		// The block talks: Benny's heard, the first time in since the locks changed (and doesn't say it every time after).
		S.Life.Hunger = 30.0;
		S.Life.Energy = 60.0;
		Expect(S.ClerkSays(ss::store::Basket()).find("on her couch") != std::string::npos, "Benny's heard about the couch");
		S.BankrollCents = 1000;
		ss::store::Basket Candy;
		Candy.Add("choco-stack");
		Expect(S.Checkout(Candy).empty() && S.ClerkSays(ss::store::Basket()).find("on her couch") == std::string::npos, "and says so once");
		S.BankrollCents = 0;

		// Dee's couch: worse sleep, and a share of the groceries.
		S.Life.Energy = 0.0;
		Fed(S);
		Expect(S.StartActivity("sleep").empty(), "lights out on the couch");
		Now = Wait(S, Now, 6.0);
		Expect(std::fabs(S.LastOutcome.Energy - 90.0 * ss::life::CouchRest) < 0.01 && S.LastOutcome.Body.find("couch") != std::string::npos, "a night on the couch gives back less");
		S.BankrollCents = 5000;
		At(S, Now, 8.0 * Day + 10.0 * 60.0 + 1.0);
		Expect(S.BankrollCents == 4000 && S.Life.CouchCents == 1000 && S.Life.Ledger.front().Label == "Groceries at Dee's", "ten dollars a day toward Dee's groceries");
		S.BankrollCents = 500;
		At(S, Now, 9.0 * Day + 10.0 * 60.0 + 1.0);
		Expect(S.BankrollCents == 500 && S.Life.CouchCents == 1000, "Dee covers it when you're broke");
		S.Save();
		ss::SaveData Parsed;
		Expect(ss::SaveData::Parse(H.Last.Serialize(), Parsed) && Parsed.Life.RentStage == Rent::Evicted && Parsed.Life.RentDueCents == S.Life.RentDueCents && Parsed.Life.Evictions == 1 &&
				   std::fabs(Parsed.Life.EvictedAt - 8.0 * Day) < 0.01 && std::fabs(Parsed.Life.StorageDue - 38.0 * Day) < 0.01 && Parsed.Life.CouchCents == 1000,
			"the eviction is saved");

		// The storage unit: a warning, the card on file, then nothing on the card.
		Mark = H.Inbox.size();
		At(S, Now, 36.0 * Day + 1.0);
		Expect(H.Got(Mark, "Ninth St. Storage", "renews in 2 days"), "the storage unit warns two days out");
		At(S, Now, 38.0 * Day - 60.0);
		S.BankrollCents = ss::life::StorageCents;
		At(S, Now, 38.0 * Day + 1.0);
		Expect(S.BankrollCents == 0 && S.Life.StorageDue == 68.0 * Day && S.Owns("monitor-24") && S.Life.Ledger.front().Label == "Ninth St. Storage (month)", "the card on file renews the unit");
		Mark = H.Inbox.size();
		At(S, Now, 68.0 * Day + 1.0);
		Expect(!S.Owns("monitor-24") && !S.Owns("ram-32") && !S.Owns("mattress") && S.Life.StorageDue == 0.0 && S.Life.Auctions == 1 && H.Got(Mark, "Ninth St. Storage", "auction"),
			"unpaid, the unit is sold at auction and the gear is gone");

		// Back in: the back rent and a month, a key, and the next month from tonight.
		Expect(S.MoveBackIn().find("Short $2,450") == 0, "moving back in takes the back rent and a month");
		S.BankrollCents = 300000;
		const double World = S.WorldMinutes();
		Expect(S.MoveBackIn().empty() && S.Life.RentStage == Rent::Paid && S.BankrollCents == 300000 - 245000 && S.Life.RentDueCents == ss::life::MonthlyRentCents,
			"paid in full: a key");
		Expect(S.Life.RentDeadline == (std::floor(World / Day) + 31.0) * Day && S.Life.Ledger.front().Label == "Back rent + a month up front", "paid through thirty days from tonight");
		Expect(S.GearFx().Tables == 2 && !S.GearFx().CanStream() && S.RestFactor() == 1.0, "home, but the auction took the rig and the mattress");
	}

	// Evicted and back within the month: the gear comes out of storage; the rent goes on from the new date.
	{
		Hooks H;
		ss::Session S(H, "rent-back");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 50000;
		Expect(S.Buy("ram-32").empty() && S.Buy("monitor-24").empty(), "a rig");
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day + 1.0);
		At(S, Now, 8.0 * Day + 1.0);
		Expect(S.Evicted() && S.GearFx().Tables == 2, "evicted");
		At(S, Now, 12.0 * Day);
		S.BankrollCents = 245000;
		Expect(S.MoveBackIn().empty() && S.GearFx().Tables == 3 && S.GearFx().CanStream() && S.Life.StorageDue == 0.0 && S.Life.CouchCents == 0, "home: the gear comes back out of storage");
		Fed(S);
		Expect(S.GoLive().empty() && S.Streaming(), "and the stream's back");
		S.EndStream();
		const double Next = S.Life.RentDeadline;
		Expect(Next == 43.0 * Day, "due thirty days from tonight");
		At(S, Now, Next - ss::life::RentNoticeDays * Day + 1.0);
		Expect(S.Life.RentStage == Rent::Due, "and due again a week before");
	}

	// The world-skip tool goes around the calendar: the bills move with it instead of being left behind the clock.
	{
		Hooks H;
		ss::Session S(H, "rent-skip");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.WorldSkip(40);
		Now = Wait(S, Now, 0.5);
		Expect(S.Life.RentStage == Rent::Due && S.Life.RentDeadline > S.WorldMinutes() && S.Life.RentDeadline == 65.0 * Day, "a skipped month's deadline moves past the skip");
		At(S, Now, 65.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::FinalNotice, "and still comes round");
		// The first month paid long after October 31: the next one is thirty days from its own date, not behind the clock.
		S.BankrollCents = 200000;
		Expect(S.PayRent() && S.Life.RentDeadline == 95.0 * Day && S.Life.RentDeadline > S.WorldMinutes(), "a late first month doesn't set the next deadline in the past");
	}

	// A host hands the clock back rounded to the minute: a deadline right on the hand-off still comes (rent and storage).
	{
		Hooks H;
		ss::Session S(H, "rent-handoff");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day - 0.33); // twenty seconds to midnight, out on the street
		Expect(S.Life.RentStage == Rent::Due, "not yet");
		S.Save();
		ss::SaveData D;
		Expect(ss::SaveData::Parse(H.Last.Serialize(), D), "the street's save");
		Hooks H2;
		ss::Session Home(H2, "rent-handoff-home", &D);
		Home.CurrentScreen = ss::Screen::Lobby;
		Home.ResumeCalendarFrom(std::round(S.WorldMinutes())); // "From=%.0f": midnight itself
		Home.LobbyMinutes += 2.0;
		double T2 = Wait(Home, 0.0, 0.1);
		Expect(Home.Life.RentStage == Rent::FinalNotice && Home.Life.RentDeadline == 8.0 * Day, "the landlord still comes at midnight");
		Home.Gear["monitor-24"] = 0.0;
		Home.LobbyMinutes += 8.0 * Day - 0.2 - Home.WorldMinutes();
		T2 = Wait(Home, T2, 0.1);
		Home.ResumeCalendarFrom(8.0 * Day);
		Home.LobbyMinutes += 1.0;
		T2 = Wait(Home, T2, 0.1);
		Expect(Home.Evicted() && Home.Life.StorageDue == 38.0 * Day, "and the locks change at the grace deadline");
		Home.LobbyMinutes += 38.0 * Day - 0.2 - Home.WorldMinutes();
		T2 = Wait(Home, T2, 0.1);
		Home.BankrollCents = ss::life::StorageCents;
		Home.ResumeCalendarFrom(38.0 * Day);
		Home.LobbyMinutes += 1.0;
		T2 = Wait(Home, T2, 0.1);
		Expect(Home.Life.StorageDue == 68.0 * Day && Home.BankrollCents == 0 && Home.Owns("monitor-24"), "the storage renewal on the hand-off is charged, not skipped");
	}

	// The LED kit goes into storage with the rest: the room goes dark, its controls do nothing, and it lights up again back home.
	{
		Hooks H;
		ss::Session S(H, "rent-leds");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 100000;
		Expect(S.Buy(ss::gear::LedKitId).empty() && S.RoomGlow(1.0).On, "the kit, lit");
		S.SetLedPreset(3);
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day + 1.0);
		At(S, Now, 8.0 * Day + 1.0);
		Expect(S.Evicted() && !S.RoomGlow(1.0).On && !S.GearFx().Leds && S.Life.StorageDue == 38.0 * Day, "evicted: the LED kit is in storage, and the room is dark");
		S.SetLedPreset(5);
		S.SetLedsOn(false);
		Expect(S.Leds.Preset == 3 && S.Leds.On && !S.RoomGlow(1.0).On, "its controls do nothing from Dee's couch");
		S.BankrollCents = 245000;
		Expect(S.MoveBackIn().empty() && S.RoomGlow(1.0).On && S.RoomGlow(1.0).Preset == 3 && S.GearFx().Leds, "home: the lights come back on as they were");
	}

	// Subscriptions while evicted: the gym goes where the player goes (billed, and still working); the internet stays
	// with the apartment (suspended, not billed) and comes back with the key.
	{
		Hooks H;
		ss::Session S(H, "rent-subs");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 100000;
		Expect(S.Buy("gym").empty() && S.Buy("fiber").empty() && S.Buy("monitor-24").empty(), "a gym card, fiber and a monitor");
		const double Calm = S.GearFx().Calm;
		const double FiberDue = S.Gear["fiber"];
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day + 1.0);
		size_t Mark = H.Inbox.size();
		At(S, Now, 8.0 * Day + 1.0);
		const ss::Chips Gym = ss::gear::Find("gym")->PriceCents;
		Expect(S.Evicted() && Calm > 0.0 && S.GearFx().Calm == Calm && !S.GearFx().Fiber && S.GearFx().MonthlyCents == Gym && S.GearFx().Tables == 2, "evicted: the gym still works and is all that's billed");
		Expect(H.Got(Mark, "Northline", "suspended"), "the internet company says the line is on hold");
		At(S, Now, 31.0 * Day);
		S.BankrollCents = 10000;
		At(S, Now, 31.0 * Day + 3.0 * 60.0);
		Expect(S.BankrollCents == 10000 - Gym && S.Owns("fiber") && S.Gear["fiber"] == FiberDue + 30.0 * Day && S.Life.Ledger.front().Label == "GearDrop: Gym membership (renewal)",
			"the gym renews; the suspended line doesn't bill");
		S.BankrollCents = 245000;
		Mark = H.Inbox.size();
		Expect(S.MoveBackIn().empty() && S.GearFx().Fiber && H.Got(Mark, "Northline", "restored"), "back home, the line comes back on");
	}

	// A night out across a deadline, home at three: the bills land in the order they came. The landlord at midnight
	// comes before the internet at one (a renewal after midnight doesn't spend the rent), and a renewal after the locks
	// change isn't billed for the apartment.
	{
		Hooks H;
		ss::Session S(H, "rent-bill-order");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		const ss::Chips Fiber = ss::gear::Find("fiber")->PriceCents;
		auto NightOut = [&](double Leave) {
			At(S, Now, Leave);
			const double Left = S.WorldMinutes();
			S.LobbyMinutes += 8.0 * 60.0;
			S.ResumeCalendarFrom(Left);
			Now = Wait(S, Now, 0.1);
		};
		S.BankrollCents = 100000;
		Expect(S.Buy("fiber").empty(), "fiber");
		S.Gear["fiber"] = 5.0 * Day + 60.0; // renews at 1 AM, an hour after the first month comes due
		S.BankrollCents = 122500 + Fiber - 1;
		NightOut(5.0 * Day - 5.0 * 60.0);
		Expect(S.Life.RentStage == Rent::Paid && S.BankrollCents == Fiber - 1 && !S.Owns("fiber"), "the landlord at midnight first; the internet at one finds the rent gone");
		S.BankrollCents = 0;
		At(S, Now, 27.0 * Day + 1.0);
		Expect(S.Life.RentStage == Rent::FinalNotice, "late on the second month");
		S.BankrollCents = 100000;
		Expect(S.Buy("fiber").empty(), "fiber again");
		S.Gear["fiber"] = 30.0 * Day + 60.0;
		S.BankrollCents = Fiber + 500;
		NightOut(30.0 * Day - 5.0 * 60.0);
		Expect(S.Evicted() && S.BankrollCents == Fiber + 500 && S.Owns("fiber") && S.Gear["fiber"] == 60.0 * Day + 60.0,
			"locked out at midnight: the line is suspended before its bill at one");
	}

	// Dee would rather go without than see the unit auctioned: no chip-in that would leave the renewal short.
	{
		Hooks H;
		ss::Session S(H, "rent-couch-unit");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 50000;
		Expect(S.Buy("monitor-24").empty(), "a monitor");
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day + 1.0);
		At(S, Now, 8.0 * Day + 1.0);
		At(S, Now, 36.0 * Day + 1.0);
		S.BankrollCents = 7500;
		const ss::Chips Couch = S.Life.CouchCents;
		const size_t Mark = H.Inbox.size();
		At(S, Now, 38.0 * Day + 1.0);
		Expect(S.Owns("monitor-24") && S.Life.StorageDue == 68.0 * Day && S.BankrollCents == 500 && S.Life.CouchCents == Couch + ss::life::CouchChipInCents && H.Got(Mark, "Dee", "keep your ten"),
			"$75 two days out: one chip-in, the second skipped, and the unit renews");
		// Paying the unit ahead, from the Bank: a month at most.
		Expect(S.PayStorage().find("Short $55") == 0, "short for the next month");
		S.BankrollCents = 10000;
		Expect(S.PayStorage().empty() && S.Life.StorageDue == 98.0 * Day && S.BankrollCents == 4000 && S.Life.Ledger.front().Label == "Ninth St. Storage (paid ahead)", "a month paid ahead");
		Expect(S.PayStorage().find("Paid through") == 0 && S.BankrollCents == 4000, "and no further");
	}

	// The locks change in the middle of a night's sleep at home: the night is the bed's, and the morning says what happened.
	{
		Hooks H;
		ss::Session S(H, "rent-asleep");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.BankrollCents = 100000;
		Expect(S.Buy("mattress").empty(), "a good mattress");
		S.BankrollCents = 0;
		At(S, Now, 5.0 * Day + 1.0);
		At(S, Now, 8.0 * Day - 2.0 * 60.0);
		S.Life.Energy = 0.0;
		const double Rest = S.RestFactor();
		Expect(Rest > 1.0 && S.StartActivity("sleep").empty(), "to bed at home, the night the locks change");
		Now = Wait(S, Now, 6.0);
		Expect(S.Evicted() && std::fabs(S.LastOutcome.Energy - 90.0 * Rest) < 0.01 && S.LastOutcome.Title == "Locked out", "a night in the bed, then the landlord's knock");
	}

	// A save evicted before the locks had consequences: the key costs a month on top, and the gear is in a unit with a clock.
	{
		ss::SaveData Old;
		Expect(ss::SaveData::Parse("shortstack.nightone.v1\nbankroll\t500\nname\tcouch_surfer\nclock\t10000.00\nlife\trent\t3\t137500\t11520.00\t0\ngear\tmonitor-24\t0.00\n", Old) &&
				   Old.Life.RentStage == Rent::Evicted && Old.Life.RentDueCents == 137500 + ss::life::MonthlyRentCents && Old.Life.Evictions == 1 && Old.Life.EvictedAt == 11520.0 &&
				   Old.Life.StorageDue == 11520.0 + ss::life::StorageDays * Day,
			"an old eviction loads with what a key costs now, and the unit's clock");
		ss::SaveData Bare;
		Expect(ss::SaveData::Parse("shortstack.nightone.v1\nbankroll\t500\nclock\t10000.00\nlife\trent\t3\t137500\t11520.00\t0\n", Bare) && Bare.Life.StorageDue == 0.0 &&
				   Bare.Life.RentDueCents == 137500 + ss::life::MonthlyRentCents,
			"nothing stored, no unit");
		Hooks H;
		ss::Session S(H, "rent-old-evicted", &Old);
		Expect(S.Evicted() && S.MoveBackIn().find("Short $2,445") == 0, "moving back in costs the back rent and a month");
	}
}

/**
 * Coming home from a night the host played out (Dee's game, the Embercrest, the street): its hours aren't charged to
 * the energy twice, the needs climb (and wear the player down as they climb), and invitations to what's already over
 * aren't sent. Plus the trip out that can't go, and an empty stomach at the table.
 */
void NightOutChecks()
{
	const double Day = ss::net::MinutesPerDay;
	// Eight hours out: the host kept the energy (and saved what it left); home, only the needs catch up.
	{
		Hooks H;
		ss::Session S(H, "night-out");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 50.0;
		double Left = S.WorldMinutes();
		S.LobbyMinutes += 8.0 * 60.0;
		S.ResumeCalendarFrom(Left);
		Now = Wait(S, Now, 0.2);
		Expect(S.Life.Energy > 49.9 && S.Life.Hunger > 43.0 && S.Life.Thirst > 53.0, "a night out isn't charged to the energy twice; the needs still climb");
		// Out from 66/66 for nine hours: past 70 the needs drag on the energy as they climb, not from where they started.
		S.Life.Hunger = 66.0;
		S.Life.Thirst = 66.0;
		S.Life.Energy = 80.0;
		Left = S.WorldMinutes();
		S.LobbyMinutes += 9.0 * 60.0;
		S.ResumeCalendarFrom(Left);
		Now = Wait(S, Now, 0.2);
		Expect(S.Life.Energy < 70.0 && S.Life.Energy > 50.0 && S.Life.Hunger > 92.0 && S.Life.Thirst == 100.0, "hungry and thirsty through the night wears the player down");
		// At the desk, the hours are the desk's.
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 80.0;
		S.LobbyMinutes += 2.0 * 60.0;
		Now = Wait(S, Now, 0.2);
		Expect(S.Life.Energy < 80.0 - 7.0, "awake at home, the hours cost energy as always");
	}

	// Tuesday: out at seven, home at three. Dee's heads-up for a game that's over isn't sent; nor is Sunday's.
	{
		Hooks H;
		ss::Session S(H, "night-out-texts");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.LobbyMinutes = 19.0 * 60.0;
		Now = Wait(S, Now, 0.1);
		size_t Mark = H.Inbox.size();
		double Left = S.WorldMinutes();
		S.LobbyMinutes += 8.0 * 60.0;
		S.ResumeCalendarFrom(Left);
		Now = Wait(S, Now, 0.2);
		Expect(!H.Got(Mark, "Dee", "back room of the laundromat"), "no heads-up for a game night that's over");
		S.LobbyMinutes = 6.0 * Day + 15.0 * 60.0 - Day * ss::net::NightOneDay; // Sunday, 3 PM
		Now = Wait(S, Now, 0.1);
		Mark = H.Inbox.size();
		Left = S.WorldMinutes();
		S.LobbyMinutes += 8.0 * 60.0;
		S.ResumeCalendarFrom(Left);
		Now = Wait(S, Now, 0.2);
		Expect(!H.Got(Mark, "Dee", "sunday $150"), "nor an invitation to the Sunday at eleven at night");
		// The same afternoon at the desk: it comes at four.
		S.LobbyMinutes = 13.0 * Day + 15.0 * 60.0 + 59.0 - Day * ss::net::NightOneDay;
		Now = Wait(S, Now, 0.1);
		Mark = H.Inbox.size();
		S.LobbyMinutes += 2.0;
		Now = Wait(S, Now, 0.1);
		Expect(H.Got(Mark, "Dee", "sunday $150"), "Sunday at four, at the desk, Dee says so");
	}

	// The host can't go right now (already on its way out): nothing paid, nothing registered.
	{
		Hooks H; // GoOut says no
		ss::Session S(H, "embercrest-stuck");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Wait(S, 0.0, 0.2);
		S.LobbyMinutes = 9.0 * Day + 17.0 * 60.0 + 30.0 - Day * ss::net::NightOneDay; // Wednesday, the Nightly's desk is open
		Now = Wait(S, Now, 0.1);
		S.Life.Energy = 60.0;
		S.BankrollCents = 50000;
		const size_t Lines = S.Life.Ledger.size();
		Expect(S.GoToLive("embercrest-nightly@9") == "Can't get there right now." && S.BankrollCents == 50000 && S.Life.LiveEntries.empty() && S.Life.Ledger.size() == Lines,
			"a trip that can't happen costs nothing");
	}

	// Starving at the table: a shorter time bank.
	{
		Hooks H;
		ss::Session S(H, "starving");
		S.CurrentScreen = ss::Screen::Lobby;
		S.Life.Energy = 60.0;
		S.Life.Hunger = 90.0;
		S.Register(1);
		Expect(S.T != nullptr && std::fabs(S.TimeBank - 22.5) < 1e-9, "starving, the time bank is three quarters");
	}
}

} // namespace session_test

namespace session_test
{
/** GearDrop and Kast: the laptop can't stream, the first PC upgrade can; a stream through a whole tournament; the payout;
 * a shift ends the stream; subscriptions renew and lapse; everything survives a save. */
void Streaming()
{
	Hooks H;
	ss::Session S(H, "stream");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	Expect(!S.GoLive().empty() && !S.Streaming(), "the laptop can't stream");
	Expect(S.Buy(ss::gear::FirstPcUpgrade().Id).empty() && S.GearFx().CanStream() && H.GearEvents == 1, "the first PC upgrade makes the rig stream-capable");
	for (const char* Id : {"webcam-1080", "mic-usb", "ring-light", "overlay-pack"})
	{
		Expect(S.Buy(Id).empty(), "the stream kit");
	}
	const double Q = S.GearFx().Quality;
	Expect(Q > 0.3 && S.GearFx().Resolution == 720, "the kit adds production value; the old line caps the stream at 720p");
	Expect(S.Buy("fiber").empty() && S.GearFx().Resolution == 720, "fiber alone can't lift what the RAM kit can encode");
	Expect(S.Buy("tower-mid").empty() && S.GearFx().Resolution == 1080 && S.GearFx().Quality > Q, "a desktop and fiber: 1080p60");
	Expect(S.GoLive().empty() && S.Streaming() && H.OnAirs == 1 && S.Channel.Streams == 1, "live on Kast");

	// A tournament on stream, played out at the sprint's pace.
	const ss::net::Network& Net = ss::net::Shared();
	ss::LobbyEvent Pick;
	bool Found = false;
	for (const ss::net::EventInstance& E : Net.Window(S.WorldMinutes() - 120.0, S.WorldMinutes() + 60.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		if (!Found && L.Joinable && L.Spec.Entrants <= 400 && L.BuyInCents <= 1100)
		{
			Pick = L;
			Found = true;
		}
	}
	Expect(Found, "an event to stream");
	if (!Found)
	{
		return;
	}
	S.RegisterEvent(Pick);
	Expect(S.Stream.Pred.Active, "registering on stream opens a prediction");
	ss::Rng Choice("stream-choice");
	int Decisions = 0;
	double Now = PlayOut(S, Choice, 0.0, Decisions);
	const ss::kast::Stream& St = S.Stream;
	Expect(St.Pred.Resolved, "the prediction resolves when the tournament ends");
	Expect(S.Channel.FindMember("dee_spincycle") && S.Channel.FindMember("dee_spincycle")->Friend && S.Channel.FindMember("mei_ng"), "Dee and Mei are there on night one");
	Expect(St.Peak <= 12, "a first stream draws a handful of people, not a crowd");
	// Who talks is who came (StreamCountsChatters, below): Dee, Mei and the few strangers a first stream draws. Before the
	// 2026-10-21 fix this asked for more than five names, which a first stream peaking at three viewers only reached by the
	// bug (lines from strangers nobody counted).
	Expect(St.Chat.size() > 20 && St.Chatters() >= 2, "chat talks through the tournament");
	Expect(S.Channel.Followers > 0 && St.Peak > 0 && !St.Graph.empty(), "viewers come, some follow");
	std::printf("  stream: %.0f min on stream, %d followers, peak %d, %zu chat lines, %d chatters, %s earned, hype %.0f\n", St.Uptime(S.WorldMinutes()), S.Channel.Followers, St.Peak, St.Chat.size(),
		St.Chatters(), ss::Money(St.Tonight.Total()).c_str(), St.Hype);
	// A shift ends the stream and pays the balance out.
	const ss::Chips Bank = S.BankrollCents;
	const ss::Chips Owed = S.Channel.UnpaidCents;
	S.LeaveResults();
	Expect(S.StartActivity("nap").empty() && !S.Streaming() && S.StreamCard && H.OnAirs == 1, "a nap ends the stream (the summary card comes up)");
	Expect(S.BankrollCents == Bank + Owed && S.Channel.UnpaidCents == 0 && (Owed == 0 || S.Life.Ledger.front().Kind == 6), "the stream's balance goes to the bank, in the ledger");
	Expect(S.Channel.Log.size() == 1 && S.Channel.MinutesLive > 0.0, "the stream is logged");
	Now = Wait(S, Now, 8.0);

	// The save keeps the gear and the channel.
	ss::SaveData Back;
	Expect(H.Saves > 0 && ss::SaveData::Parse(H.Last.Serialize(), Back) && Back.Serialize() == H.Last.Serialize(), "the save round-trips");
	Expect(Back.Gear == S.Gear && Back.Channel.Followers == S.Channel.Followers && Back.Channel.Log.size() == S.Channel.Log.size() && Back.Channel.Members.size() == S.Channel.Members.size() &&
		Back.Channel.Regulars() == S.Channel.Regulars() && Back.Channel.ScheduleDays == S.Channel.ScheduleDays && Back.Channel.DaysLive == S.Channel.DaysLive, "gear and channel (and the community) in the save");
	ss::Session Loaded(H, "stream-loaded", &Back);
	Expect(Loaded.GearFx().CanStream() && Loaded.GearFx().Resolution == 1080 && Loaded.Channel.Streams == 1, "a loaded game streams at 1080p");

	// A posted schedule, and a stream that ends with a raid.
	S.StreamSchedule(0x7f | 0x80, 24 * 60 + 90);
	Expect(S.Channel.ScheduleDays == 0x7f && S.Channel.ScheduleStart == 90, "the schedule keeps seven days and a minute of the day");
	S.StreamSchedule(0x7f, static_cast<int>(std::fmod(S.WorldMinutes(), ss::net::MinutesPerDay)));
	Expect(S.Channel.OnSchedule(S.WorldMinutes()), "a stream now would be on schedule");
	S.StreamCard = false;
	Expect(S.GoLive().empty() && S.Streaming() && S.Stream.Tonight.OnSchedule, "the second stream is on schedule");
	for (int K = 0; K < 60 && S.Stream.Uptime(S.WorldMinutes()) < 3.0; ++K)
	{
		Now = Wait(S, Now, 10.0);
	}
	const std::vector<ss::kast::SmallChannel> Small = ss::kast::Network(S.WorldMinutes());
	const std::string Target = Small.empty() ? std::string("chipleader_carla") : Small.front().Name;
	S.EndStream(Target);
	Expect(!S.Streaming() && S.Stream.Last.RaidedOut == Target && S.Channel.Goodwill[Target] == 1 && S.Channel.RaidsOut == 1, "ending with a raid sends the viewers on (and the channel remembers)");
	Expect(S.Channel.Log.size() == 2 && S.Channel.Log.front().OnSchedule && S.Channel.Streams == 2, "both streams logged");

	// Subscriptions: one renews (in the ledger), one lapses when the bank can't cover it.
	ss::SaveData Monthly = Back;
	Monthly.BankrollCents = 10000;
	Monthly.Gear["gym"] = Monthly.ClockMinutes + ss::net::MinutesPerDay * static_cast<double>(ss::net::NightOneDay) + 1.0;
	ss::Session Gym(H, "stream-gym", &Monthly);
	Gym.CurrentScreen = ss::Screen::Lobby;
	Wait(Gym, 0.0, 90.0);
	const ss::gear::Item& GymItem = *ss::gear::Find("gym");
	Expect(Gym.Owns("gym") && Gym.BankrollCents == 10000 - GymItem.PriceCents && Gym.Life.Ledger.front().Kind == 5, "a subscription renews from the bankroll");
	Monthly.BankrollCents = 100;
	ss::Session Broke(H, "stream-broke", &Monthly);
	Broke.CurrentScreen = ss::Screen::Lobby;
	Wait(Broke, 0.0, 90.0);
	Expect(!Broke.Owns("gym") && Broke.BankrollCents == 100 && Broke.GearFx().Calm == Loaded.GearFx().Calm, "a subscription the bank can't cover lapses");
	Expect(S.Cancel("fiber") && !S.Owns("fiber") && S.GearFx().Resolution == 720, "cancelling fiber drops the stream back to 720p");
}

/**
 * A chatter is a viewer (the 2026-10-21 report: "When playing MTTs while live streaming, kast shows 0 Viewers even when there
 * is chatters"). A new channel streams online tournaments the way the frame loop plays them (one table, two tables, a sprint):
 * whenever a viewer's line lands in chat, the count Kast shows (the pill, the studio, the VIEWERS tile: Stream::Viewers,
 * rounded) is at least one, and it is never below the number of different people who spoke in the last ten seconds. Spam
 * bots aren't viewers (no count includes them); trolls are.
 */
namespace kast_count
{
struct Watch
{
	int Frames = 0;
	int Lines = 0;   // viewers' lines that landed in chat
	int Silent = 0;  // ... while the count showed nobody
	int Crowded = 0; // frames where more people had spoken in the last 10 s than the count showed
	size_t Worst = 0; // the most speakers over the count
	int Counted = 0; // frames with somebody counted
	int MaxShown = 0;
	std::string FirstSilent; // the first line that landed while the count showed nobody ("who: text")
};

bool ViewerLine(const ss::kast::ChatMsg& M)
{
	using ss::kast::LineKind;
	return !M.Who.empty() && !M.Spam && (M.Kind == LineKind::Chat || M.Kind == LineKind::Question || M.Kind == LineKind::Tip || M.Kind == LineKind::Sub || M.Kind == LineKind::Cheer);
}

/** One frame: the lines that landed since Prev against the count shown now. */
void Observe(const ss::Session& S, double Prev, double Now, Watch& W)
{
	const ss::kast::Stream& St = S.Stream;
	if (!St.Live)
	{
		return;
	}
	const long long Shown = static_cast<long long>(std::round(St.Viewers)); // KastCount's rounding
	std::set<std::string> Spoke;
	for (const ss::kast::ChatMsg& M : St.Chat)
	{
		if (!ViewerLine(M) || M.At > Now)
		{
			continue;
		}
		if (M.At > Prev)
		{
			++W.Lines;
			W.Silent += Shown < 1 ? 1 : 0;
			if (Shown < 1 && W.FirstSilent.empty())
			{
				W.FirstSilent = M.Who + ": " + M.Text;
			}
		}
		if (M.At > Now - 10.0)
		{
			Spoke.insert(M.Who);
		}
	}
	++W.Frames;
	W.Counted += Shown >= 1 ? 1 : 0;
	W.MaxShown = std::max(W.MaxShown, static_cast<int>(Shown));
	if (static_cast<long long>(Spoke.size()) > Shown)
	{
		++W.Crowded;
		W.Worst = std::max(W.Worst, Spoke.size() - static_cast<size_t>(std::max(0LL, Shown)));
	}
}

/** The first joinable listing around now that passes Ok and isn't already open. */
bool PickListing(const ss::Session& S, ss::LobbyEvent& Out, const std::function<bool(const ss::LobbyEvent&)>& Ok)
{
	const ss::net::Network& Net = ss::net::Shared();
	for (const ss::net::EventInstance& E : Net.Window(S.WorldMinutes() - 120.0, S.WorldMinutes() + 60.0))
	{
		const ss::LobbyEvent L = Net.Listing(E, nullptr, S.Unlocks());
		if (L.Joinable && L.BuyInCents <= 1100 && !S.IsPlaying(L.Spec.Id) && Ok(L))
		{
			Out = L;
			return true;
		}
	}
	return false;
}

/**
 * Tables 1 or 2: a channel on the first PC upgrade (nothing else) goes live in the lobby, registers and plays for up to
 * Seconds of real time at 30 frames a second, the hero taking Think seconds over each decision; Sprint sprints the table.
 * Community 0 is a new channel's first stream (Dee and Mei); otherwise a channel thirty streams in, with that many members
 * (casual to superfan) and Followers in all (the rest lurk).
 */
Watch OnStream(const char* Seed, int Tables, bool Sprint, double Think, double Seconds, int Community = 0, int Followers = 0)
{
	Hooks H;
	ss::Session S(H, Seed);
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	Watch W;
	for (int K = 0; K < Community; ++K)
	{
		ss::kast::Member M;
		M.Name = "kastfan_" + std::to_string(K);
		M.Loyalty = 0.1 + 0.8 * static_cast<double>(K % 10) / 9.0;
		M.Affinity = 0.9;
		M.Follower = true;
		M.Streams = 10;
		M.LastSeen = S.WorldMinutes() - 1440.0;
		S.Channel.Members.push_back(M);
	}
	if (Community > 0)
	{
		S.Channel.Followers = std::max(Community, Followers);
		S.Channel.Streams = 30;
		S.Channel.Milestone = 10000000; // no milestone alerts for a channel that got here off screen
	}
	Expect(S.Buy(ss::gear::FirstPcUpgrade().Id).empty() && S.GoLive().empty() && S.Streaming(), "kast count: the channel goes live");
	ss::LobbyEvent First;
	// The listing's speed reads "Hyper · 3 min", "Turbo · 5 min": one table plays a full-speed MTT, two put a hyper beside it.
	auto Hyper = [](const ss::LobbyEvent& L) { return L.Speed.rfind("Hyper", 0) == 0; };
	const bool Found = PickListing(S, First, [&](const ss::LobbyEvent& L) { return Tables == 2 ? Hyper(L) : !Hyper(L); });
	const std::string Why = Found ? S.RegisterEvent(First) : std::string("no listing");
	std::printf("  kast count (%s): %s, %d entrants, %s%s\n", Seed, First.Spec.Name.c_str(), First.Spec.Entrants, First.Speed.c_str(), Why.empty() ? "" : (": " + Why).c_str());
	Expect(Why.empty() && S.T, "kast count: an online tournament on stream");
	if (Tables == 2)
	{
		ss::LobbyEvent Second;
		const std::string Why2 = PickListing(S, Second, [&](const ss::LobbyEvent& L) { return !Hyper(L); }) ? S.RegisterEvent(Second) : std::string("no listing");
		std::printf("  kast count (%s): and %s, %d entrants, %s%s\n", Seed, Second.Spec.Name.c_str(), Second.Spec.Entrants, Second.Speed.c_str(), Why2.empty() ? "" : (": " + Why2).c_str());
		Expect(Why2.empty() && S.TableCount() == 2, "kast count: and a second table");
	}
	if (Sprint)
	{
		S.CurrentPace = ss::Pace::Sprint;
		S.SprintBudgetMs = 0.0; // one simulated hand a frame (the skip loop's do-while), so the run doesn't hang on the CPU's speed
	}
	ss::Rng Choice(std::string(Seed) + "-hero");
	double Now = 0.0;
	const double Dt = 1.0 / 30.0;
	double ResultsAt = -1.0; // the stream goes on over the results screen (the prediction resolves there): 15 s of it
	for (int Frame = 0; S.Streaming() && S.TableCount() > 0 && Frame < static_cast<int>(Seconds * 30.0) && (ResultsAt < 0.0 || Now - ResultsAt < 15.0); ++Frame)
	{
		const double Prev = Now;
		Now += Dt;
		S.Update(Now);
		if (S.CurrentScreen == ss::Screen::Results && ResultsAt < 0.0)
		{
			ResultsAt = Now;
		}
		for (int K = 0; K < S.TableCount(); ++K)
		{
			const ss::TableGlance G = S.Glance(K);
			if (G.YourTurn && Now - G.TurnOpenedAt > Think)
			{
				const double Roll = Choice.Next();
				S.HeroActAt(K, Roll < 0.6 ? ss::PlayerAction::Call() : ss::PlayerAction::Fold());
			}
		}
		Observe(S, Prev, Now, W);
	}
	std::printf("  kast count (%s, %d table%s%s): %d frames, %d viewer lines (%d while the count showed 0), %d frames with more speakers in 10 s than counted (worst +%zu), "
				"counted somebody in %d, peak shown %d, world %.1f min%s%s\n",
		Seed, Tables, Tables == 1 ? "" : "s", Sprint ? ", sprint" : "", W.Frames, W.Lines, W.Silent, W.Crowded, W.Worst, W.Counted, W.MaxShown, S.Stream.Uptime(S.WorldMinutes()),
		W.FirstSilent.empty() ? "" : "; first line at 0: ", W.FirstSilent.c_str());
	return W;
}

/**
 * A timed-out stranger is still watching but can't type (BUG_REVIEW.md: with the strangers who talk as few as the count
 * shows, the troll the mods had just timed out was the stranger who reacted to the next hand). A channel whose 400
 * followers lurk, with two mods online, streams three nights at a table with a moment every half minute or so; Kast is
 * driven directly (no session). Nobody speaks in the ten minutes after their timeout notice.
 */
void TimedOutStayQuiet()
{
	Hooks H;
	ss::Session Rig(H, "kast-count-mods");
	Rig.BankrollCents = 300000;
	Expect(Rig.Buy(ss::gear::FirstPcUpgrade().Id).empty() && Rig.GearFx().CanStream(), "kast count: the first PC upgrade streams");
	ss::kast::Channel Ch;
	Ch.Followers = 400; // no tracked community: the followers lurk, strangers drop in
	Ch.Streams = 30;
	Ch.Milestone = 10000000;
	for (int K = 0; K < 2; ++K)
	{
		ss::kast::Moderator M;
		M.Name = "kastmod_" + std::to_string(K);
		M.Online = 0.95;
		Ch.Mods.push_back(M);
	}
	ss::kast::Stream St("kast-count-mods");
	ss::Rng Pick("kast-count-mods-moments");
	const ss::kast::Moment Kinds[] = {ss::kast::Moment::AllIn, ss::kast::Moment::BigPot, ss::kast::Moment::LevelUp, ss::kast::Moment::Knockout, ss::kast::Moment::Blunder, ss::kast::Moment::BestPlay};
	int Timeouts = 0;
	int Lines = 0;
	int Spoke = 0;
	std::string FirstSpoke;
	for (int Night = 0; Night < 3; ++Night)
	{
		ss::kast::Inputs In;
		In.World = 1440.0 * (10.0 + Night) + 20.0 * 60.0;
		In.Real = 100000.0 * (Night + 1);
		In.AtTable = true;
		In.Tables = 1;
		In.Hero = "kast_count_hero";
		In.EventName = "Night Owl Turbo";
		In.Gear = Rig.GearFx();
		St.Start(Ch, In);
		std::map<std::string, double> OutAt; // name -> the real second of their timeout notice
		std::set<int> Landed;
		double NextMoment = In.Real + 30.0;
		for (const double End = In.World + 150.0; In.World < End;)
		{
			In.World += 0.01; // three world minutes a real one, as at a table
			In.Real += 0.2;
			St.Tick(Ch, In, 0.01, 0.2);
			if (In.Real >= NextMoment)
			{
				St.OnMoment(Ch, In, Kinds[Pick.Int(6)], "somebody");
				NextMoment = In.Real + 20.0 + 40.0 * Pick.Next();
			}
			for (const ss::kast::ChatMsg& M : St.Chat)
			{
				if (M.At > In.Real || !Landed.insert(M.Id).second)
				{
					continue;
				}
				const size_t Cut = M.Kind == ss::kast::LineKind::System ? std::min(M.Text.find(" has been timed out"), M.Text.find(" was timed out by ")) : std::string::npos;
				if (Cut != std::string::npos)
				{
					OutAt[M.Text.substr(0, Cut)] = M.At;
					++Timeouts;
				}
				else if (ViewerLine(M))
				{
					++Lines;
					const auto It = OutAt.find(M.Who);
					if (It != OutAt.end() && M.At > It->second && M.At - It->second < 600.0)
					{
						++Spoke;
						FirstSpoke = FirstSpoke.empty() ? M.Who + ": " + M.Text : FirstSpoke;
					}
				}
			}
		}
		St.Stop(Ch, In);
	}
	std::printf("  kast count (timeouts): %d timeouts, %d viewer lines, %d from someone timed out%s%s\n", Timeouts, Lines, Spoke, FirstSpoke.empty() ? "" : "; first: ", FirstSpoke.c_str());
	Expect(Timeouts >= 3 && Lines > 300 && Spoke == 0, "kast count: a timed-out stranger doesn't talk until the timeout runs out");
}
} // namespace kast_count

void StreamCountsChatters()
{
	const kast_count::Watch One = kast_count::OnStream("kast-count-one", 1, false, 1.0, 300.0);
	const kast_count::Watch Slow = kast_count::OnStream("kast-count-slow", 1, false, 4.0, 300.0);
	const kast_count::Watch Two = kast_count::OnStream("kast-count-two", 2, false, 1.5, 300.0);
	const kast_count::Watch Fast = kast_count::OnStream("kast-count-sprint", 1, true, 0.5, 120.0);
	// A small channel (a few casual members, the count around a handful) and a bigger one (lurkers, a busy chat).
	const kast_count::Watch Small = kast_count::OnStream("kast-count-small", 1, false, 1.0, 300.0, 8, 40);
	const kast_count::Watch Big = kast_count::OnStream("kast-count-big", 2, false, 1.0, 300.0, 120, 3000);
	Expect(One.Lines > 0 && Two.Lines > 0 && One.Counted > 0 && Two.Counted > 0 && Small.Lines > 20 && Big.Lines > 100 && Big.MaxShown >= 10,
		"kast count: chat talks and somebody is counted (the check isn't empty)");
	Expect(One.Silent == 0 && Slow.Silent == 0 && Two.Silent == 0 && Fast.Silent == 0 && Small.Silent == 0 && Big.Silent == 0,
		"kast count: no viewer's line lands in chat while the count shows nobody (a chatter is a viewer)");
	Expect(One.Crowded == 0 && Slow.Crowded == 0 && Two.Crowded == 0 && Fast.Crowded == 0 && Small.Crowded == 0 && Big.Crowded == 0, "kast count: never more people talking (last 10 s) than the count shows");

	// An empty room, the moment the stream starts: every kind of moment, the prediction resolving. Nobody is there to say
	// anything (a raid that turns up is counted before its chat lands).
	Hooks H;
	ss::Session S(H, "kast-count-empty");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	Expect(S.Buy(ss::gear::FirstPcUpgrade().Id).empty() && S.GoLive().empty() && S.Stream.Viewers < 0.5 && S.Stream.Present == 0, "kast count: live to an empty room");
	const ss::kast::Inputs In = S.StreamInputs();
	for (int M = 0; M < static_cast<int>(ss::kast::Moment::Count); ++M)
	{
		S.Stream.OnMoment(S.Channel, In, static_cast<ss::kast::Moment>(M), "somebody");
	}
	S.Stream.Resolve(S.Channel, In, true);
	kast_count::Watch Empty;
	double Now = S.Now;
	for (int Frame = 0; Frame < 30 * 5; ++Frame)
	{
		const double Prev = Frame == 0 ? -1e9 : Now; // the first frame takes in every line already landed
		Now += 1.0 / 30.0;
		S.Update(Now);
		kast_count::Observe(S, Prev, Now, Empty);
	}
	std::printf("  kast count (empty room): %d viewer lines (%d while the count showed 0), %d crowded frames, peak shown %d, %d chatters%s%s\n", Empty.Lines, Empty.Silent, Empty.Crowded, Empty.MaxShown,
		S.Stream.Chatters(), Empty.FirstSilent.empty() ? "" : "; first line at 0: ", Empty.FirstSilent.c_str());
	Expect(Empty.Silent == 0 && Empty.Crowded == 0 && (Empty.Lines == 0 || Empty.MaxShown >= 1), "kast count: an empty room reacts to nothing (no moment, no prediction line)");
	kast_count::TimedOutStayQuiet();
}
/** The LED room kit: bought, each colour's perk, the power switch, the save, and the room flashing with the stream. */
void LedChecks()
{
	Hooks H;
	ss::Session S(H, "leds");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 100000;
	Expect(!S.RoomGlow(1.0).On && !S.GearFx().Leds, "no kit, no lights");
	S.SetLedPreset(4);
	Expect(!S.RoomGlow(1.0).On && !S.GearFx().Leds, "picking a colour without the kit lights nothing");
	const ss::gear::Effects Base = ss::gear::Sum(S.Gear);
	Expect(S.Buy(ss::gear::LedKitId).empty() && H.GearEvents == 1 && S.BankrollCents == 100000 - ss::gear::Find(ss::gear::LedKitId)->PriceCents, "GearDrop sells the kit ($59)");
	Expect(S.RoomGlow(1.0).On && S.RoomGlow(1.0).Preset == 4 && S.RoomGlow(1.0).Rgb == 0xffb21e && S.RoomGlow(1.0).Level == 1.0, "it lights up in the colour picked");
	struct Perk
	{
		double Calm, Drain, Follow, Hype, Tips, Stay;
	};
	const Perk Perks[7] = {{0.05, 0, 0, 0, 0, 0}, {0, 0.05, 0, 0, 0, 0}, {0, 0, 0.05, 0, 0, 0}, {0, 0, 0, 0.1, 0, 0}, {0, 0, 0, 0, 0.1, 0}, {0, 0, 0, 0, 0, 0.1}, {0, 0, 0.03, 0.05, 0, 0}};
	auto Near = [](double A, double B) { return std::fabs(A - B) < 1e-9; };
	for (int K = 0; K < ss::gear::LedPresetCount; ++K)
	{
		S.SetLedPreset(K);
		const ss::gear::Effects& Fx = S.GearFx();
		const Perk& P = Perks[K];
		Expect(Fx.Leds && Fx.LedPreset == K && Near(Fx.Quality, Base.Quality + 0.05), "every colour lights the stream (+5% production value)");
		Expect(Near(Fx.Calm, Base.Calm + P.Calm) && Near(Fx.Drain, Base.Drain + P.Drain) && Near(Fx.Follow, Base.Follow + P.Follow) && Near(Fx.Hype, P.Hype) && Near(Fx.Tips, P.Tips) && Near(Fx.Stay, P.Stay),
			"each colour has its own perk, and only that one");
	}
	Expect(S.RoomGlow(0.0).Rgb == 0x19e68c && S.RoomGlow(8.0).Rgb == 0x2fb4ff && S.RoomGlow(4.0).Rgb != S.RoomGlow(0.0).Rgb && S.RoomGlow(32.0).Rgb == 0x19e68c, "Aurora drifts green, blue, violet, pink and around again");
	S.SetLedsOn(false);
	Expect(!S.RoomGlow(1.0).On && !S.GearFx().Leds && Near(S.GearFx().Quality, Base.Quality) && S.GearFx().Hype == 0.0, "off: dark room, no perk");
	S.SetLedsOn(true);
	// The save keeps the colour and the switches; an old save without them gets the defaults.
	S.SetLedPreset(5);
	S.SetLedSync(false);
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Back) && Back.Leds == S.Leds && Back.Leds.Preset == 5 && !Back.Leds.Sync && Back.Serialize() == H.Last.Serialize(), "the LEDs are in the save");
	ss::Session Loaded(H, "leds-loaded", &Back);
	Expect(Loaded.RoomGlow(1.0).On && Loaded.RoomGlow(1.0).Rgb == 0xff3fb4 && Loaded.GearFx().Stay > 0.09, "a loaded game comes back with After Hours on");
	std::string Old = H.Last.Serialize();
	Old.erase(Old.find("leds\t"), Old.find('\n', Old.find("leds\t")) - Old.find("leds\t") + 1);
	ss::SaveData Older;
	Expect(ss::SaveData::Parse(Old, Older) && Older.Leds == ss::gear::LedState(), "an old save gets the default lights (Felt Green, on, synced)");
	// Synced to the stream: alerts flash the room, a won all-in sweeps it gold, a bad beat dips it; unsynced it holds.
	S.SetLedSync(true);
	S.SetLedPreset(1);
	Expect(S.Buy("ram-32").empty() && S.GoLive().empty() && S.Streaming(), "live");
	const double T = 500.0;
	ss::kast::Alert A;
	A.Kind = ss::kast::AlertKind::Sub;
	A.Who = "dee_spincycle";
	A.At = T;
	S.Stream.Alerts.insert(S.Stream.Alerts.begin(), A);
	S.Stream.LastMomentReal = -100.0;
	S.Stream.Hype = 0.0;
	const ss::gear::Glow Flash = S.RoomGlow(T + 0.05);
	Expect(Flash.Rgb != 0x2fb4ff && Flash.Level > 1.5, "a sub flashes the room (lime)");
	Expect(S.RoomGlow(T + 2.0).Rgb == 0x2fb4ff && S.RoomGlow(T + 2.0).Level == 1.0, "then it settles back to Ice Blue");
	S.Stream.Alerts.clear();
	S.Stream.LastMoment = ss::kast::Moment::BadBeat;
	S.Stream.LastMomentReal = T;
	Expect(S.RoomGlow(T + 0.1).Level < 0.8, "a bad beat dims the room");
	S.Stream.LastMoment = ss::kast::Moment::WonAllIn;
	const ss::gear::Glow Win = S.RoomGlow(T + 0.1);
	Expect(((Win.Rgb >> 16) & 0xff) > 0xc0 && Win.Level > 1.2, "a won all-in sweeps it gold");
	S.Stream.Hype = 100.0;
	S.Stream.LastMomentReal = -100.0;
	double Lo = 9.0;
	double Hi = 0.0;
	for (int K = 0; K < 40; ++K)
	{
		const double L = S.RoomGlow(T + static_cast<double>(K) * 0.07).Level;
		Lo = std::min(Lo, L);
		Hi = std::max(Hi, L);
	}
	Expect(Hi - Lo > 0.15, "the room breathes with the hype");
	S.SetLedSync(false);
	S.Stream.LastMomentReal = T;
	Expect(S.RoomGlow(T + 0.1).Rgb == 0x2fb4ff && S.RoomGlow(T + 0.1).Level == 1.0, "unsynced, the room holds its colour");
	S.EndStream();
	S.ResetSave();
	Expect(S.Leds == ss::gear::LedState() && !S.RoomGlow(1.0).On, "a new game starts with no kit and default settings");
}

/** One channel's first months, stream by stream: the grind is slow, and consistency pays. */
struct Grind
{
	int Streams = 0;
	int FirstPeak = 0;
	int Followers10 = 0;
	int Followers = 0;
	int Regulars10 = 0;
	int Regulars = 0;
	double ReturningEarly = 0.0; // average over streams 1-10
	double ReturningLate = 0.0;  // the last ten
	int AffiliateAt = 0;
	bool RulesKept = true; // Affiliate only once the 30-day rules are met
	double LoyaltyBefore = 0.0;
	double LoyaltyAfter = 0.0; // after three weeks away
	int RegularsAfter = 0;
};

Grind RunGrind(bool Engaged, int Total)
{
	Grind G;
	ss::gear::Owned Own;
	for (const char* Id : {"tower-mid", "fiber", "webcam-1080", "mic-usb", "ring-light", "overlay-pack", "monitor-24"})
	{
		Own[Id] = 0.0;
	}
	ss::kast::Channel Ch;
	const bool Schedule = Engaged;
	const double AnswerRate = Engaged ? 0.6 : 0.1;
	if (Schedule)
	{
		Ch.ScheduleDays = 1 | 4 | 16 | 64;
		Ch.ScheduleStart = 21 * 60;
	}
	ss::kast::Stream St("grind");
	ss::Rng Ev("grind-events");
	ss::kast::Inputs In;
	In.Hero = "grinder_3c";
	In.Gear = ss::gear::Sum(Own);
	In.AtTable = true;
	In.Tables = 2;
	In.EventName = "Night Owl";
	int Day = 0;
	for (; G.Streams < Total; ++Day)
	{
		In.World = static_cast<double>(Day) * ss::net::MinutesPerDay + 21.0 * 60.0 + (Schedule ? 0.0 : 37.0 * static_cast<double>(Day % 5));
		ss::kast::Offline(Ch, In.World, Ev);
		if (!(((1 | 4 | 16 | 64) >> (Day % 7)) & 1))
		{
			continue;
		}
		const bool WasAffiliate = Ch.Affiliate;
		St.Start(Ch, In);
		const double End = In.World + 180.0;
		int Hand = 0;
		double NextThank = In.Real + 600.0;
		while (In.World < End)
		{
			for (int K = 0; K < 6; ++K)
			{
				In.World += 0.25 / 6.0;
				In.Real += 1.0;
				St.Tick(Ch, In, 0.25 / 6.0, 1.0);
			}
			for (const ss::kast::ChatMsg& M : St.Chat)
			{
				if (M.Kind == ss::kast::LineKind::Question && !M.Answered && In.Real - M.At > 4.0 && In.Real - M.At < 6.0 && Ev.Chance(AnswerRate))
				{
					St.Answer(Ch, In, M.Id);
					break;
				}
			}
			if (Engaged && In.Real > NextThank)
			{
				St.Thank(Ch, In);
				NextThank = In.Real + 900.0;
			}
			if (++Hand % 3 == 0 && Ev.Chance(0.05))
			{
				St.OnMoment(Ch, In, ss::kast::Moment::AllIn);
				St.OnMoment(Ch, In, Ev.Chance(0.6) ? ss::kast::Moment::WonAllIn : ss::kast::Moment::LostAllIn);
			}
			St.Notices.clear();
		}
		const std::vector<ss::kast::SmallChannel> Net = ss::kast::Network(In.World);
		const ss::kast::Summary Sm = St.Stop(Ch, In, Engaged && !Net.empty() && G.Streams % 2 == 0 ? Net.front().Name : std::string());
		++G.Streams;
		if (Ch.Affiliate && !WasAffiliate)
		{
			double Minutes = 0.0;
			int Days = 0;
			double Avg = 0.0;
			Ch.Window30(In.World, Minutes, Days, Avg);
			G.AffiliateAt = G.Streams;
			G.RulesKept = Ch.Followers >= ss::kast::AffiliateFollowers && Minutes >= ss::kast::AffiliateMinutes && Days >= ss::kast::AffiliateDays && Avg >= ss::kast::AffiliateAvg;
		}
		G.FirstPeak = G.Streams == 1 ? Sm.Peak : G.FirstPeak;
		G.ReturningEarly += G.Streams <= 10 ? static_cast<double>(Sm.Returning) / 10.0 : 0.0;
		G.ReturningLate += G.Streams > Total - 10 ? static_cast<double>(Sm.Returning) / 10.0 : 0.0;
		if (G.Streams == 10)
		{
			G.Followers10 = Ch.Followers;
			G.Regulars10 = Ch.Regulars();
		}
	}
	G.Followers = Ch.Followers;
	G.Regulars = Ch.Regulars();
	auto Mean = [&Ch]() {
		double Sum = 0.0;
		for (const ss::kast::Member& M : Ch.Members)
		{
			Sum += M.Friend ? 0.0 : M.Loyalty;
		}
		return Sum / static_cast<double>(std::max<size_t>(1, Ch.Members.size()));
	};
	G.LoyaltyBefore = Mean();
	for (int K = 0; K < 21; ++K)
	{
		ss::kast::Offline(Ch, static_cast<double>(Day + K) * ss::net::MinutesPerDay + 12.0 * 60.0, Ev);
	}
	G.LoyaltyAfter = Mean();
	G.RegularsAfter = Ch.Regulars();
	return G;
}

void StreamGrind()
{
	const Grind G = RunGrind(true, 50);
	std::printf("  grind (a mid rig, 4 nights a week, 3 hours, on schedule, talks to chat): first peak %d, followers %d -> %d, regulars %d -> %d, returning %.1f -> %.1f, affiliate at stream %d\n", G.FirstPeak, G.Followers10,
		G.Followers, G.Regulars10, G.Regulars, G.ReturningEarly, G.ReturningLate, G.AffiliateAt);
	Expect(G.FirstPeak >= 1 && G.FirstPeak <= 6, "the first stream: a handful of viewers");
	Expect(G.Followers10 < 40, "ten streams in, followers are still in the tens");
	Expect(G.Followers > G.Followers10 * 4 && G.Followers < 600, "fifty streams in: real growth, not a flood");
	Expect(G.Regulars > G.Regulars10 && G.Regulars >= 8, "regulars build up stream by stream");
	Expect(G.ReturningLate > G.ReturningEarly * 1.5, "more people come back as the community grows");
	Expect(G.AffiliateAt > 10 && G.RulesKept, "Affiliate takes weeks, and comes only with the 30-day rules met");
	Expect(G.LoyaltyAfter < G.LoyaltyBefore * 0.9 && G.RegularsAfter < G.Regulars, "three weeks away and the community drifts");
	const Grind Lazy = RunGrind(false, 50);
	std::printf("  grind (no schedule, rarely talks to chat, never raids): followers %d, regulars %d, returning %.1f\n", Lazy.Followers, Lazy.Regulars, Lazy.ReturningLate);
	Expect(Lazy.Regulars < G.Regulars && Lazy.ReturningLate < G.ReturningLate, "a schedule and a streamer who talks to chat build a bigger community");
}
/** The living world inside a session: who sits at the player's tables, what they remember, and the save. */
void LivingWorld()
{
	namespace net = ss::net;
	Hooks H;
	ss::Session S(H, "living-world");
	S.OnBoot();
	S.CurrentScreen = ss::Screen::Lobby;
	const ss::world::World& W = S.Living();
	const net::Network& Net = net::Shared();
	Expect(W.Ready() && Net.Attached() == &W && Net.Players().size() == W.People().size() && W.People().size() >= 1618, "a new career starts a living world the network shows");
	Expect(Net.Players()[static_cast<size_t>(Net.RivalIndex())].Name == ss::RivalName && Net.FindPlayer("Mei") >= 0, "the rival and the cast are on the network");
	// Tonight's schedule: an event the player can join with the world's regulars registered.
	const double Now = S.WorldMinutes();
	net::EventInstance Pick;
	size_t Best = 0;
	for (const net::EventInstance& E : Net.Window(Now, Now + 180.0))
	{
		std::string Lock;
		if (Net.Joinable(E, &Lock, S.Unlocks()) && Net.TemplateOf(E).BuyInCents <= S.BankrollCents && Net.TemplateOf(E).BuyInCents > 0 && E.Entries <= 1000)
		{
			const size_t N = S.Living().Registered(E.Id).size();
			if (N > Best)
			{
				Best = N;
				Pick = E;
			}
		}
	}
	std::printf("  world: %zu regulars registered for %s\n", Best, Pick.Id.c_str());
	Expect(Best >= 3, "tonight's events have the world's regulars in them");
	S.RegisterEvent(Net.Listing(Pick));
	Expect(S.T != nullptr && S.FieldNpc.size() >= Best, "the regulars take seats at the player's tournament");
	bool Named = true;
	for (const ss::TPlayer& P : S.T->Players)
	{
		const auto It = S.FieldNpc.find(P.Id);
		Named = Named && (It == S.FieldNpc.end() || W.Get(It->second)->Name == P.Name);
	}
	Expect(Named, "they play under their own names");
	S.CurrentPace = ss::Pace::Sprint;
	ss::Rng Choice("world-choices");
	int Decisions = 0;
	double T = PlayOut(S, Choice, 10.0, Decisions);
	Expect(S.CurrentScreen == ss::Screen::Results, "the tournament finishes");
	const int Place = S.History[0].Place;
	Expect(!S.Living().Bonds().empty(), "the people at the player's tables remember them");
	S.LeaveResults();
	// The world plays the event out around the player's own finish.
	S.WorldSkip(1);
	const net::EventResult* Res = S.Living().ResultOf(Pick.Id);
	bool HeroThere = Place > 9;
	if (Res)
	{
		for (const net::Placing& P : Res->FinalTable)
		{
			HeroThere = HeroThere || (P.Player == -1 && P.Place == Place);
		}
	}
	Expect(Res != nullptr && HeroThere, "the world's result of the event has the player's finish in it");
	std::printf("  world: finished %s; %zu people now know the player\n", ss::Ordinal(Place).c_str(), S.Living().Bonds().size());
	// Days go by: the news is about people.
	S.WorldSkip(3);
	int People = 0;
	for (const net::NewsItem& It : Net.News(S.WorldMinutes(), net::StatsFrom(S.HeroName, S.History, S.WorldMinutes()), 40))
	{
		People += It.Player >= 0 ? 1 : 0;
	}
	Expect(People > 0, "the news follows the world's people");
	Expect(S.WorldPreview(14).find("World ") == 0 && S.Living().Clock() <= S.WorldMinutes() + 1.0, "a preview plays a copy forward and leaves the world alone");
	// The save carries the world, and a night at Dee's goes into it.
	ss::SaveData Saved = H.Last;
	Expect(!Saved.WorldText.empty(), "the world is in the save");
	Saved.NoteBackRoom(S.WorldMinutes() + 60.0, {"Sal", "Big Lou", "Twitch", "Mei"}, 4200);
	ss::SaveData Parsed;
	Expect(ss::SaveData::Parse(Saved.Serialize(), Parsed) && Parsed.WorldText == Saved.WorldText && Parsed.WorldNotes.size() == 1, "the world and its notes survive a save");
	Parsed.ClockMinutes += 120.0;
	{
		Hooks H2;
		ss::Session Back(H2, "living-world-2", &Parsed);
		const ss::world::Bond* Sal = Back.Living().BondWith(Back.Living().Find("Sal"));
		Expect(Back.Living().People().size() == W.People().size() && Back.Living().Bonds().size() > S.Living().Bonds().size() - 1, "a loaded session carries on the same world");
		Expect(Sal && !Sal->Memories.empty() && Sal->Memories.back().Kind == ss::world::MemoryKind::BackRoom, "Sal remembers the night at Dee's");
		Back.Save();
		Expect(H2.Last.WorldNotes.empty(), "a note is read once");
	}
	Expect(Net.Attached() == &S.Living(), "the network goes back to the session still running");
	(void)T;
}

} // namespace session_test

// ------------------------------------------------------------------ update 2's carried checks (update 2 SPEC §7.2, day 5)

namespace session_test
{
namespace seams
{
std::string Fixture(const char* Name); // session_seams.cpp
void RoundTrip(const char* Name, const std::string& Saved, const std::string& Expected); // session_seams.cpp
void SaveFixtures();                  // session_seams.cpp (SegLead's)
void Gate1Fixtures();                 // session_seams.cpp (also SegLead's)
void Gate2Fixtures();                 // session_seams.cpp (also SegLead's)
void Gate3Fixtures();                 // session_seams.cpp (also SegLead's; update 2 C3)
int ScreenLetters(const char* Suite, const std::vector<ss::life::Letter>& Letters); // session_seams.cpp (C3's copy pass)
void StreamLinks();                   // session_seams.cpp (also SegLead's)
void RivalSites();                    // session_seams.cpp (also SegLead's; update 2 B3)
} // namespace seams

namespace pacing
{
namespace life = ss::life;
namespace live = ss::live;
const double Day = 1440.0;

/** A career that goes out and keeps quiet: the phone counts, the bus always comes. */
struct PaceHooks : Hooks
{
	int Phone = 0;
	void Text(const std::string&, const std::string&) override { ++Phone; }
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

/** The career's ledger as it grows (Life keeps the last sixty lines): every line once, as it's written. */
struct LedgerTally
{
	life::LedgerEntry Newest;
	bool Any = false;
	std::vector<life::LedgerEntry> All;
	ss::Chips Credit = 0;    // the store credit the sponsors' perks granted (no ledger line: read off the book's balance)
	ss::Chips CreditNow = 0; // the balance last seen (only the corner store spends it, which the careers never visit)
	void Take(const life::State& L)
	{
		Credit += std::max<ss::Chips>(0, L.Sponsors.StoreCreditCents - CreditNow);
		CreditNow = L.Sponsors.StoreCreditCents;
		size_t Fresh = 0;
		while (Fresh < L.Ledger.size())
		{
			const life::LedgerEntry& E = L.Ledger[Fresh];
			if (Any && E.At == Newest.At && E.Amount == Newest.Amount && E.Kind == Newest.Kind && E.Label == Newest.Label)
			{
				break;
			}
			++Fresh;
		}
		for (size_t K = Fresh; K-- > 0;)
		{
			All.push_back(L.Ledger[K]);
		}
		if (!L.Ledger.empty())
		{
			Newest = L.Ledger.front();
			Any = true;
		}
	}
};

/** One event a career played (G1's paired comparison reads them): its result, how long it took and who played it. */
struct EventRec
{
	std::string Id;
	int Place = 0;
	int Field = 0;
	ss::Chips Prize = 0;
	int Hands = 0;
	double At = 0.0;      // world minutes: its start
	ss::Chips BuyIn = 0;  // with the fee
	double Minutes = 0.0; // world minutes from its start to the result
	bool Live = false;
	bool Plan = false; // played by the plan (G1's career: TAG, skipped, at the default Sharpness)
};

/** One of EconomyPacing's careers and what it measured. Kind: 0 live-only, 1 streaming, 2 mixed. */
struct Career
{
	std::string Seed;
	int Kind = 2;
	bool Deals = true;
	int Days = 180;
	bool Auto = false; // G1's autoplay career: every other event (online and live) skipped by TAG at Sharpness 0.80
	std::vector<EventRec> Events;
	LedgerTally Ledger;
	std::vector<double> LouAt;     // when Big Lou's stakes were taken
	int Lantern = 0;               // Lantern Row's package offers seen
	bool LanternOffTier = false;   // one whose terms aren't its tier's (staking_backing proves the tiers positive-EV)
	bool RollPaidRent = false;     // a package's roll moved across a rent deadline
	int RentChecks = 0;
	std::vector<int> TierByDay;    // SponsorTier() each day
	std::array<ss::Chips, 4> Bank{}; // the bankroll on days 30, 60, 90 and the last
	std::array<ss::Chips, 3> DealsTo{};  // check 6: what the deals put in by days 30, 60 and 90 (DealValue)
	std::array<ss::Chips, 3> IncomeTo{}; // check 6: the income by then (Income: check 1's kinds)
	int Online = 0, Live = 0, Streams = 0, Signed = 0;
	// G2's creator career (update 2 SPEC §1.11): the streaming career's schedule with the stream kit once it's back in (not
	// §1.11's mid rig: CreatorPacing), and a Rerun upload three times a week from day 7 (Tuesday, Thursday and Saturday: the
	// career starts on Night One's Tuesday), cut from its own hands.
	bool Creator = false;
	double Start = 0.0;                 // world minutes when the career began
	int Uploads = 0, UploadsMissed = 0; // what went up; upload days with nothing to cut
	std::array<int, 4> ByFormat{};      // uploads by format (Short, Breakdown, Vlog, Recap)
	std::array<double, 4> Subs{};       // Rerun's subscribers on days 30, 60, 90 and the last
	std::array<double, 4> Views{};      // and its views to date
	double PartnerAt = -1.0;            // world minutes
	int Kast = 0;                       // Kast followers at the end
	double Rig = 0.0;                   // the rig's quality at the end (gear::Effects::Quality)
	// G3's multi-site career (update 2 SPEC §1.11): Ignite and RunItTwice joined from their own invitations, money moving
	// between the bank and the sites (CheckCalendar's SiteStep), and what it measured (SitesChecks).
	bool Sites = false;
	std::vector<std::pair<int, double>> Invited;   // a site's invitation, when it came
	std::vector<std::pair<int, int>> InvitedAt;    // and the career's tracked events and last-100 average buy-in (cents) then
	int Deposits = 0, Withdrawals = 0;
	ss::Chips DepositCents = 0, WithdrawCents = 0; // what went to the sites and what was asked back
	int RentDeadlines = 0;                         // deadlines crossed while the career held an account
	int SiteRentBreaks = 0;                        // a deadline where site money moved toward the rent, or the rent was taken on site money
	std::string SiteRentFirst;                     // the first one, for a failure's reader
	std::set<long long> ShortDeadlines;            // deadlines the bank alone couldn't cover but the bank and the sites could (at a sample)
	int WarnSamples = 0, WarnMissed = 0;           // the hourly samples in those last 72 hours, and where the rent card stayed quiet
	double WarnEarliest = 0.0;                     // hours before its deadline the warning first showed (the most of any)
	std::array<int, 3> BySite{};                   // events by site
	std::array<ss::Chips, 3> BuyInsBySite{};       // their buy-ins
	int Phase2At = -1;                             // the career's day it moved up to $11 events
	ss::Chips SitesEnd = 0;                        // on the sites and in transit at the end
	int JoinedSites = 0;                           // rivals joined with money on them
	int EventsEnd = 0;                             // the record at the end: tracked events,
	ss::Chips Abi100End = 0;                       // and the last hundred's average buy-in
};

/** Income as check 1 reads it: poker, the job, hustles, prizes, streaming and the sponsors' lines. */
bool Income(const life::LedgerEntry& E)
{
	return E.Amount > 0 && (E.Kind == 0 || E.Kind == 1 || E.Kind == 2 || E.Kind == 4 || E.Kind == 6 || E.Kind == life::LedgerSponsors);
}

/**
 * Check 6's measure of the deals so far: what they put in, cash and in-kind at face. The sponsors' lines (signing,
 * stipends, bonuses, the covered entries and the covered bus fares, less a cut, a cover returned or signing money paid
 * back) and the store credit their perks granted; backing's lines (a stake, pieces sold, a chop) net of what the backers
 * took back. Each kind counts at zero when it took more than it gave.
 */
ss::Chips DealValue(const LedgerTally& T)
{
	ss::Chips Sponsors = 0;
	ss::Chips Backing = 0;
	for (const life::LedgerEntry& E : T.All)
	{
		Sponsors += E.Kind == life::LedgerSponsors ? E.Amount : 0;
		Backing += E.Kind == life::LedgerBacking ? E.Amount : 0;
	}
	return std::max<ss::Chips>(0, Sponsors) + T.Credit + std::max<ss::Chips>(0, Backing);
}

/** When Act I ended (update 2 U11): the second rent paid, Night One's and the first $1,075 month (the ledger's second rent
 *  line, collected, paid ahead or paid back on moving in); 1e18 if it hasn't. */
double ActIEndOf(const LedgerTally& T)
{
	int Paid = 0;
	for (const life::LedgerEntry& E : T.All)
	{
		if (E.Kind == 3 && E.Amount < 0 && (E.Label.rfind("Rent", 0) == 0 || E.Label.rfind("Back rent", 0) == 0) && ++Paid == 2)
		{
			return E.At;
		}
	}
	return 1.0e18;
}

ss::Chips RollSum(const ss::Session& S)
{
	ss::Chips Sum = 0;
	for (const ss::stake::Deal& D : S.Life.Stakes.Deals)
	{
		Sum += D.K == ss::stake::Kind::Package ? D.RollNow : 0;
	}
	return Sum;
}

/** Fed and rested enough to play (the careers don't model the meals: EconomyPacing is about the deals and the rent). */
void Fed(ss::Session& S)
{
	S.Life.Hunger = 10.0;
	S.Life.Thirst = 10.0;
	S.Life.Energy = 90.0;
}

/** The money on the rival sites: every wallet, and what's on its way to the bank (net of its fee, as the rent card counts it;
 *  Gross: the whole amount, as the wallet paid it out). */
ss::Chips SitesMoney(const ss::Session& S, bool Gross = false)
{
	ss::Chips Sum = 0;
	for (const ss::site::Account& A : S.Life.Sites.Accounts)
	{
		Sum += A.Cents;
	}
	for (const ss::site::Transfer& X : S.Life.Sites.Transfers)
	{
		Sum += X.State == 0 && X.Out ? (Gross ? X.Cents : X.Cents - X.FeeCents) : 0;
	}
	return Sum;
}

/**
 * G3's career: one look at the rent card in a deadline's last 72 hours (§5.5, §1.11). Whenever the bank alone is short of
 * the rent and the bank and the sites together aren't, the card's site line ("site money can't pay rent") is showing.
 */
void SitesSample(const ss::Session& S, Career& C)
{
	const life::State& L = S.Life;
	const double World = S.WorldMinutes();
	if (!C.Sites || L.RentStage == life::Rent::Evicted || L.RentStage == life::Rent::Paid || World < L.RentDeadline - 72.0 * 60.0 || World >= L.RentDeadline)
	{
		return;
	}
	const ss::Chips Sites = SitesMoney(S);
	if (Sites <= 0 || S.BankrollCents >= L.RentDueCents || S.BankrollCents + Sites < L.RentDueCents)
	{
		return;
	}
	C.ShortDeadlines.insert(std::llround(L.RentDeadline));
	++C.WarnSamples;
	std::string Line;
	const bool Shown = ss::site::RentWarning(L.Sites, S.BankrollCents, L.RentDueCents, L.RentDeadline, World, Line) && !Line.empty();
	C.WarnMissed += Shown ? 0 : 1;
	C.WarnEarliest = Shown ? std::max(C.WarnEarliest, (L.RentDeadline - World) / 60.0) : C.WarnEarliest;
}

/** G3's career: each site's invitation as it comes, and the record it met (Act I's end is the ledger's: ActIEndOf). */
void SitesObserve(ss::Session& S, Career& C)
{
	if (!C.Sites)
	{
		return;
	}
	for (const ss::site::Invitation& I : S.Life.Sites.Invites)
	{
		const int Site = static_cast<int>(I.Site);
		if (std::none_of(C.Invited.begin(), C.Invited.end(), [Site](const std::pair<int, double>& X) { return X.first == Site; }))
		{
			const ss::rep::Public P = S.Record();
			C.Invited.emplace_back(Site, I.At);
			C.InvitedAt.emplace_back(P.OnlineEvents, static_cast<int>(P.Abi100));
		}
	}
}

/** The clock to World (the calendar runs over the gap), checking that no package roll moves at a rent deadline (and, on G3's
 *  career, looking at the rent card each hour of a deadline's last 72, and that no site money moves toward the rent). */
double Hop(ss::Session& S, Career& C, double Now, double World)
{
	if (World <= S.WorldMinutes())
	{
		return Now;
	}
	const double Deadline = S.Life.RentDeadline;
	if (C.Sites && !S.Evicted())
	{
		// The rent card in the deadline's last 72 hours, from the first of them (the earliest it shows) on the hour.
		for (double At = std::max(Deadline - 72.0 * 60.0, std::ceil(S.WorldMinutes() / 60.0) * 60.0); At < std::min(World, Deadline - 1.0); At += 60.0)
		{
			if (At > S.WorldMinutes())
			{
				S.LobbyMinutes += At - S.WorldMinutes();
				Now = Wait(S, Now, 0.05);
				SitesObserve(S, C);
			}
			SitesSample(S, C);
		}
	}
	if (Deadline > S.WorldMinutes() + 1.0 && Deadline < World - 1.0)
	{
		S.LobbyMinutes += Deadline - 1.0 - S.WorldMinutes();
		Now = Wait(S, Now, 0.05);
		SitesSample(S, C);
		C.Ledger.Take(S.Life);
		const ss::Chips Before = RollSum(S);
		// G3: what the sites hold as the landlord comes (site money never pays rent, U11).
		const std::vector<ss::site::Account> Wallets = S.Life.Sites.Accounts;
		const std::vector<ss::site::Transfer> Transit = S.Life.Sites.Transfers;
		const ss::Chips Bank = S.BankrollCents;
		const size_t Lines = C.Ledger.All.size();
		S.LobbyMinutes += Deadline + 1.0 - S.WorldMinutes();
		Now = Wait(S, Now, 0.05);
		C.RollPaidRent = C.RollPaidRent || RollSum(S) != Before;
		++C.RentChecks;
		C.Ledger.Take(S.Life);
		if (C.Sites && (!Wallets.empty() || !Transit.empty()))
		{
			++C.RentDeadlines;
			std::string Broke;
			// Each wallet moved only by its own lines, none of them the rent's.
			for (const ss::site::Account& A : S.Life.Sites.Accounts)
			{
				const auto Was = std::find_if(Wallets.begin(), Wallets.end(), [&A](const ss::site::Account& X) { return X.Site == A.Site; });
				ss::Chips Moved = 0;
				for (size_t K = Was == Wallets.end() ? 0 : Was->Lines.size(); K < A.Lines.size(); ++K)
				{
					Moved += A.Lines[K].Cents;
					Broke += A.Lines[K].Label.rfind("Rent", 0) == 0 || A.Lines[K].Label.find(" rent") != std::string::npos ? "a rent line on a wallet; " : "";
				}
				const bool Kept = Was != Wallets.end() && Was->Lines.size() >= 40;
				Broke += !Kept && Was != Wallets.end() && A.Cents - Was->Cents != Moved ? "a wallet moved without its lines; " : "";
			}
			// What was on its way still is, or landed (as the bank's own line, below), never before its minute.
			for (const ss::site::Transfer& X : Transit)
			{
				const auto Now2 = std::find_if(S.Life.Sites.Transfers.begin(), S.Life.Sites.Transfers.end(), [&X](const ss::site::Transfer& Y) { return Y.Id == X.Id; });
				if (X.State != 0 || Now2 == S.Life.Sites.Transfers.end() || Now2->State == 0)
				{
					continue;
				}
				Broke += X.ArrivesAt > Deadline + 1.0 ? "a transfer landed early for the rent; " : "";
			}
			// The bank moved only by its own lines (so nothing but the bank, and what landed in it, paid the rent).
			ss::Chips Logged = 0;
			for (size_t K = Lines; K < C.Ledger.All.size(); ++K)
			{
				Logged += C.Ledger.All[K].Amount;
			}
			Broke += S.BankrollCents - Bank != Logged ? "the bank moved without its ledger; " : "";
			Broke += S.BankrollCents < 0 ? "a negative bank; " : "";
			if (!Broke.empty())
			{
				++C.SiteRentBreaks;
				C.SiteRentFirst = C.SiteRentFirst.empty() ? ss::net::DateLabel(ss::net::DayOf(Deadline)) + ": " + Broke : C.SiteRentFirst;
			}
		}
		SitesObserve(S, C);
	}
	S.LobbyMinutes += World - S.WorldMinutes();
	Now = Wait(S, Now, 0.05);
	C.Ledger.Take(S.Life);
	SitesObserve(S, C);
	SitesSample(S, C);
	return Now;
}

/** The deals on the table taken (backing and sponsors on): every stake offered, every sponsor's offer signed. */
void TakeDeals(ss::Session& S, Career& C)
{
	const double World = S.WorldMinutes();
	for (const ss::stake::Offer& O : std::vector<ss::stake::Offer>(S.Life.Stakes.Offers))
	{
		if (O.State != ss::stake::OfferStanding || O.Expires < World)
		{
			continue;
		}
		if (O.By == ss::stake::Outfit::LanternRow)
		{
			++C.Lantern;
			const ss::stake::Terms Tier = ss::stake::LanternRowTerms(O.Tier);
			C.LanternOffTier = C.LanternOffTier || O.Tier < 1 || O.T.CutPermille != Tier.CutPermille || O.T.Roll != Tier.Roll || O.T.CapOnline != Tier.CapOnline;
		}
		if (C.Deals && S.AcceptStake(O.Id).empty() && O.Backer == "Big Lou")
		{
			C.LouAt.push_back(World);
		}
	}
	if (!C.Deals)
	{
		return;
	}
	for (const ss::sponsor::Brand& B : ss::sponsor::Brands())
	{
		const std::string Id = ss::sponsor::OpenOfferFor(S.Life.Sponsors, B.Id, World);
		if (!Id.empty())
		{
			ss::sponsor::HeardOffer(S.Life.Sponsors, S.Life, Id, World);
			C.Signed += S.SignDeal(Id).empty() ? 1 : 0;
		}
	}
	C.Ledger.Take(S.Life);
}

/** A hyper from its start to the result: sprinted, the hands Sprint hands back played with the grader's best option. */
double Online(ss::Session& S, Career& C, double Now)
{
	ss::net::EventInstance I;
	if (S.BankrollCents < 500 || !ss::net::Shared().Next("hyper-sprint", S.WorldMinutes() + 1.0, I))
	{
		return Now;
	}
	Now = Hop(S, C, Now, I.Start);
	Fed(S);
	S.RegisterEvent(ss::net::Shared().Listing(I));
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return Now;
	}
	++C.Online;
	S.SprintBudgetMs = 1.0e9; // a stretch to its stop in one update: the same frames on any machine
	// G1's career: every other event the plan (TAG) skips to the result, at the default Sharpness (pinned: the hands the
	// careful twin's Sprint hands back, played with the grader's best, would lift Form above 80).
	const bool Plan = C.Auto && C.Online % 2 == 0;
	if (Plan)
	{
		ss::autoplay::TableAuto A;
		A.S = ss::style::Style::Tag;
		A.M = ss::autoplay::Mode::Skip;
		A.To = ss::autoplay::Target::Result;
		A.Stops = 0;
		S.SetAutoplay(0, A);
		S.DebugAuto("sharp 0.80");
	}
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		if (!Plan)
		{
			S.CurrentPace = ss::Pace::Sprint;
		}
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
			size_t Best = 0;
			for (size_t K = 1; K < Opts.size(); ++K)
			{
				Best = Opts[K].Ev > Opts[Best].Ev ? K : Best;
			}
			S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
		}
	}
	EventRec Rec;
	Rec.Id = I.Id;
	Rec.Place = S.LastResults.Place;
	Rec.Field = S.LastResults.Entrants;
	Rec.Prize = S.LastResults.PrizeCents;
	Rec.Hands = S.LastResults.Hands;
	Rec.At = I.Start;
	Rec.BuyIn = ss::net::Shared().Listing(I).BuyInCents;
	Rec.Plan = Plan;
	S.LeaveResults();
	Rec.Minutes = S.WorldMinutes() - I.Start;
	C.Events.push_back(Rec);
	C.Ledger.Take(S.Life);
	return Now;
}

/** A daily's next start on a site's network from From, within a day: the template itself or, in a launch week, its one-off
 *  ("ignite.spark-jackpot-launch4": a launch week runs the guaranteed dailies as one-offs of their own). */
bool NextOn(const ss::net::Network& Net, const std::string& Base, double From, ss::net::EventInstance& Out)
{
	bool Found = false;
	for (const ss::net::EventInstance& I : Net.Window(From, From + Day))
	{
		const std::string Template = I.Id.substr(0, I.Id.find('@'));
		if (I.Start >= From && (Template == Base || Template.rfind(Base + "-launch", 0) == 0) && (!Found || I.Start < Out.Start))
		{
			Out = I;
			Found = true;
		}
	}
	return Found;
}

/**
 * G3's career: a site's event (RiverLine's or a rival's) from its start to the result, sprinted, the hands Sprint hands back
 * played with the grader's best; its money from the pot its site says (the bank or the wallet). Not if it can't be afforded,
 * or if its next start is after Latest (world minutes).
 */
double SiteEvent(ss::Session& S, Career& C, double Now, ss::net::SiteId Site, const std::string& Base, double Latest)
{
	const ss::net::Network& Net = ss::net::Site(Site);
	ss::net::EventInstance I;
	if (!NextOn(Net, Base, S.WorldMinutes() + 1.0, I) || I.Start > Latest || !S.CanAfford(Net.Listing(I, nullptr, S.Unlocks())))
	{
		return Now;
	}
	Now = Hop(S, C, Now, I.Start);
	Fed(S);
	const ss::LobbyEvent L = Net.Listing(I, nullptr, S.Unlocks());
	if (!S.CanAfford(L))
	{
		return Now;
	}
	S.RegisterEvent(L);
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return Now;
	}
	S.SprintBudgetMs = 1.0e9;
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		S.CurrentPace = ss::Pace::Sprint;
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
			size_t Best = 0;
			for (size_t K = 1; K < Opts.size(); ++K)
			{
				Best = Opts[K].Ev > Opts[Best].Ev ? K : Best;
			}
			S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
		}
	}
	EventRec Rec;
	Rec.Id = I.Id;
	Rec.Place = S.LastResults.Place;
	Rec.Field = S.LastResults.Entrants;
	Rec.Prize = S.LastResults.PrizeCents;
	Rec.Hands = S.LastResults.Hands;
	Rec.At = I.Start;
	Rec.BuyIn = L.BuyInCents;
	S.LeaveResults();
	Rec.Minutes = S.WorldMinutes() - I.Start;
	C.Events.push_back(Rec);
	++C.Online;
	++C.BySite[static_cast<size_t>(Site)];
	C.BuyInsBySite[static_cast<size_t>(Site)] += L.BuyInCents;
	C.Ledger.Take(S.Life);
	SitesObserve(S, C);
	return Now;
}

/**
 * G3's day online (update 2 SPEC §1.11's multi-site career): a grinder who joins each rival the morning its invitation comes
 * (Mail's Accept), puts $50 on it by card once the bank holds the next rent and a cushion (and tops it up the same way),
 * and withdraws what piles up there (a Sunday sweep too), and everything on the sites when a rent is near and the bank is
 * short. Its afternoons:
 *   - until it moves up, RiverLine's $0.25 Hyper Sprints, four (two on a Saturday), one swapped for each rival joined
 *     (Ignite's $1.10 Flip & Run, RunItTwice's $3.30 Rush);
 *   - it moves up once Act I is over and 200 events are on the record, with the rent and $100 more in the bank; from then
 *     on $11 and up only (RunItTwice's door, an $11 average over the last hundred): RiverLine's $11 Step 2 at 3 PM, Ignite's
 *     $11 Coral Classic at 4 PM, RunItTwice's $11 Midday Classic at 1 PM, and on weekdays the 9 PM Step 2, each while the
 *     bank can spare it (on a day it can't, none).
 */
double SitesDay(ss::Session& S, Career& C, double Now, int DayNo, int K)
{
	namespace site = ss::site;
	namespace net = ss::net;
	// The morning's Mail: an invitation is accepted the day it comes.
	for (const site::Invitation& I : std::vector<site::Invitation>(S.Life.Sites.Invites))
	{
		if (I.State == 0 || I.State == 3)
		{
			S.AnswerInvite(I.Site, true);
		}
	}
	SitesObserve(S, C);
	auto Reserve = [&S]() {
		return (S.Evicted() || S.Life.RentStage != life::Rent::Paid ? S.Life.RentDueCents : life::MonthlyRentCents) + 20000;
	};
	auto Wallet = [&S](net::SiteId Site) {
		const ss::Chips* W = site::WalletOf(S.Life.Sites, Site);
		return W ? *W : -1;
	};
	auto Fund = [&](net::SiteId Site, ss::Chips Need) {
		const ss::Chips W = Wallet(Site);
		if (W >= 0 && W < Need && S.BankrollCents >= Reserve() + 5300 && S.Deposit(Site, 5000, "card").empty())
		{
			++C.Deposits;
			C.DepositCents += 5000;
			C.Ledger.Take(S.Life);
		}
		return Wallet(Site) >= Need;
	};
	const bool Saturday = net::Weekday(DayNo) == 5;
	const double Evening = static_cast<double>(DayNo) * Day + 18.0 * 60.0; // the afternoon's events start by 6 PM
	// Moved up once Act I is over and 200 events are on the record, with the rent and $100 more in the bank; from then on $11
	// and up only, and on a day the bank can't spare it, none (so the last hundred's average climbs to RunItTwice's door).
	C.Phase2At = C.Phase2At < 0 && S.Life.RentsPaid >= 2 && C.Online >= 200 && S.BankrollCents >= Reserve() + 10000 ? K : C.Phase2At;
	const bool Up = C.Phase2At >= 0;
	const bool Spare = S.BankrollCents >= Reserve() + 1100;
	Now = Hop(S, C, Now, static_cast<double>(DayNo) * Day + 12.5 * 60.0);
	Fed(S);
	if (Up)
	{
		if (Fund(net::SiteId::RunItTwice, 1100))
		{
			Now = SiteEvent(S, C, Now, net::SiteId::RunItTwice, "runittwice.midday-classic", Evening);
		}
		if (Spare)
		{
			Now = SiteEvent(S, C, Now, net::SiteId::RiverLine, "step2", static_cast<double>(DayNo) * Day + 15.0 * 60.0 + 1.0);
		}
		if (Fund(net::SiteId::Ignite, 1100))
		{
			Now = SiteEvent(S, C, Now, net::SiteId::Ignite, "ignite.coral-classic", Evening);
		}
		if (!Saturday && S.BankrollCents >= Reserve() + 1100)
		{
			Now = Hop(S, C, Now, static_cast<double>(DayNo) * Day + 20.5 * 60.0);
			Now = SiteEvent(S, C, Now, net::SiteId::RiverLine, "step2", static_cast<double>(DayNo) * Day + 21.0 * 60.0 + 1.0);
		}
	}
	else
	{
		int Hypers = Saturday ? 2 : 4;
		Now = Hop(S, C, Now, static_cast<double>(DayNo) * Day + 14.0 * 60.0);
		if (Fund(net::SiteId::Ignite, 110))
		{
			Now = SiteEvent(S, C, Now, net::SiteId::Ignite, "ignite.flip-go", Evening);
			--Hypers;
		}
		if (Fund(net::SiteId::RunItTwice, 330))
		{
			Now = SiteEvent(S, C, Now, net::SiteId::RunItTwice, "runittwice.the-rush", Evening);
			--Hypers;
		}
		for (int H = 0; H < Hypers && S.BankrollCents >= 500; ++H)
		{
			Now = SiteEvent(S, C, Now, net::SiteId::RiverLine, "hyper-sprint", Evening);
		}
	}
	// Money back to the bank: what piles up on a site (over $150, all but $50; on a Sunday, over $60, all but $40), and everything
	// there when a rent is near and the bank is short of it (five days out: the slowest withdrawal takes five).
	const bool Sunday = net::Weekday(DayNo) == 6;
	const bool RentNear = !S.Evicted() && S.Life.RentStage != life::Rent::Paid && S.Life.RentDeadline - S.WorldMinutes() < 5.0 * Day &&
		S.BankrollCents < S.Life.RentDueCents + 10000;
	for (const site::Account& A : std::vector<site::Account>(S.Life.Sites.Accounts))
	{
		const ss::Chips Out = RentNear ? A.Cents : A.Cents >= 15000 ? A.Cents - 5000 : Sunday && A.Cents >= 6000 ? A.Cents - 4000 : 0;
		if (Out >= 2000 && S.Withdraw(A.Site, Out, "bank").empty())
		{
			++C.Withdrawals;
			C.WithdrawCents += Out;
		}
	}
	C.Ledger.Take(S.Life);
	return Now;
}

/** A Nightly at the Embercrest, the host's side (EmbercrestRail's): settle on the save, note the night, load it again. */
double LiveNight(std::unique_ptr<ss::Session>& S, PaceHooks& H, Career& C, double Now, int DayNo)
{
	live::Occurrence O;
	for (const live::Occurrence& X : live::Occurrences(DayNo))
	{
		O = std::string(X.T->Key) == "nightly" ? X : O;
	}
	if (!O.Valid() || S->WorldMinutes() > O.DeskOpens + 60.0)
	{
		return Now;
	}
	Now = Hop(*S, C, Now, O.DeskOpens + 30.0);
	Fed(*S);
	TakeDeals(*S, C);
	const std::vector<std::pair<life::Funding, std::string>> Options = S->FundingOptions(O.Id);
	const bool Backed = !Options.empty() && Options[0].second.empty() && Options[0].first.From != life::Paid::Own;
	if (!Backed && S->BankrollCents < 40000)
	{
		return Now; // the rent comes first
	}
	if (!S->GoToLive(O.Id).empty())
	{
		return Now;
	}
	++C.Live;
	ss::SaveData D = H.Last;
	D.Life = S->Life;
	D.BankrollCents = S->BankrollCents;
	if (D.WorldText.empty() && D.WorldSnapshot)
	{
		D.WorldSnapshot->Write(D.WorldText);
	}
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	if (!E)
	{
		return Now;
	}
	E->ArrivedAt = O.Start - 15.0;
	std::vector<ss::ReservedPlayer> Known;
	for (const std::pair<std::string, int>& R : E->Roster)
	{
		ss::ReservedPlayer P;
		P.Name = R.first;
		P.Type = static_cast<ss::Archetype>(R.second);
		Known.push_back(P);
	}
	bool Restored = false;
	std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known, false, Restored);
	int Place = 0;
	ss::Chips Prize = 0;
	// G1's career: every other night the plan (TAG, Sharpness 0.80) skips it through live::SkipSlice, the card room's way.
	const bool Plan = C.Auto && C.Live % 2 == 0;
	ss::autoplay::TableAuto PlanA;
	ss::autoplay::Run PlanR;
	if (Plan)
	{
		PlanA.S = ss::style::Style::Tag;
		PlanA.M = ss::autoplay::Mode::Skip;
		PlanA.To = ss::autoplay::Target::Result;
		PlanA.Stops = 0;
		PlanR.Sharp = 0.80;
		PlanR.SharpSetAt = O.Start;
		struct Sink : ss::live::SkipSink
		{
			ss::autoplay::Run* R = nullptr;
			ss::style::HeroBrain* B = nullptr;
			const ss::autoplay::TableAuto* A = nullptr;
			const ss::reads::Table* Notes = nullptr;
			double Gain = 1.0;
			void HandDone(const ss::Hand& H, int HeroSeat) override
			{
				ss::autoplay::NoteSkippedHand(*R, H, HeroSeat, Gain);
				*B = ss::autoplay::BrainFor(*A, *R, Gain, 1, Notes, nullptr);
			}
			void Events(const std::vector<ss::TEvent>&) override {}
			void Checkpoint(int) override {}
		};
		const double Gain = std::min(2.5, std::max(0.3, D.Life.Perks.TiltGain));
		ss::style::HeroBrain Brain = ss::autoplay::BrainFor(PlanA, PlanR, Gain, 1, &D.Life.Auto.Notes, nullptr);
		Sink K;
		K.Gain = Gain;
		K.R = &PlanR;
		K.B = &Brain;
		K.A = &PlanA;
		K.Notes = &D.Life.Auto.Notes;
		for (int Guard = 0; Guard < 6000 && !T->Hero().Busted && !T->bFinished; ++Guard)
		{
			if (ss::live::SkipSlice(*T, Brain, PlanA, 1.0e9, K).Why != ss::live::SkipWhy::Budget)
			{
				break;
			}
		}
	}
	else
	{
		const ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
		for (int Guard = 0; Guard < 6000 && !T->Hero().Busted && !T->bFinished; ++Guard)
		{
			for (const ss::TEvent& Ev : T->SimulateTick(&Auto))
			{
				if (Ev.Type == ss::TEventType::Bust && Ev.IsHero)
				{
					Place = Ev.Place;
					Prize = Ev.PrizeCents;
				}
			}
		}
	}
	if (Place <= 0)
	{
		Place = T->Hero().Place > 0 ? T->Hero().Place : T->HeroRank();
		Prize = T->PrizeFor(Place);
	}
	const double End = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
	live::Settle(D.BankrollCents, D.Life, O.Id, Place, T->Spec.Entrants, Prize, End);
	live::PayFare(D.BankrollCents, D.Life, O.Id, true, End + 5.0);
	if (Plan && (PlanR.AiHands > 0 || PlanR.AiDecisions > 0))
	{
		ss::autoplay::Record(D.Life.Auto, O.Id, 1, 0, PlanA, PlanR);
		D.Life.Update2LinesRead |= ss::life::U2AutoPlayed | ss::life::U2AutoPref;
	}
	D.NoteLive(End, O.Id, Place, Prize, T->Spec.Entrants, {});
	{
		EventRec Rec;
		Rec.Id = O.Id;
		Rec.Place = Place;
		Rec.Field = T->Spec.Entrants;
		Rec.Prize = Prize;
		Rec.Hands = T->Tick;
		Rec.At = O.Start;
		Rec.BuyIn = O.T->BuyInCents;
		Rec.Minutes = End - O.Start;
		Rec.Live = true;
		Rec.Plan = Plan;
		C.Events.push_back(Rec);
	}
	D.ClockMinutes = End + 30.0 - Day;
	S.reset();
	S.reset(new ss::Session(H, C.Seed, &D));
	S->CurrentScreen = ss::Screen::Lobby;
	S->ResumeCalendarFrom(End + 30.0);
	Now = Wait(*S, Now, 0.1);
	C.Ledger.Take(S->Life);
	return Now;
}

/** A shift at the Wash & Fold (6 AM to 4 PM starts), played out: the job that keeps a broke grinder in the game. */
double Shift(ss::Session& S, Career& C, double Now)
{
	if (!S.StartActivity("washfold").empty())
	{
		return Now;
	}
	for (int Guard = 0; S.TimeSkip.Active && Guard < 200; ++Guard)
	{
		Now = Wait(S, Now, 0.5);
	}
	C.Ledger.Take(S.Life);
	return Now;
}

/** G2's creator: the stream kit (720p), a piece at a time as the roll allows once back in from the lockout (the stream's RAM
 *  first, then a webcam, a USB mic and a ring light), never into the rent. */
void CreatorRig(ss::Session& S, Career& C)
{
	for (const char* Id : {"ram-32", "webcam-720", "mic-usb", "ring-light"})
	{
		if (S.Owns(Id))
		{
			continue;
		}
		const ss::gear::Item* I = ss::gear::Find(Id);
		if (I && S.BankrollCents >= I->PriceCents + 30000)
		{
			S.Buy(Id);
			C.Ledger.Take(S.Life);
		}
		return;
	}
}

/** The night a candidate belongs to (Video.cpp's: the world day it was cut, the small hours counted with the evening before). */
int CreatorNight(double At)
{
	return static_cast<int>(std::floor((At - 10.0 * 60.0) / Day));
}

/**
 * G2's creator: one upload at 9 PM, picked as a player picks in Studio: Short, Breakdown and Vlog in turn (the Python
 * model's rotation), the best candidate first (a Vlog is the best night's best three to eight; a Breakdown when that night
 * hasn't three), Story titles, the Reaction frame. Through Publish: the edit's time and energy pass and FinishSkip lands it.
 */
double CreatorUpload(ss::Session& S, Career& C, double Now, int DayNo)
{
	namespace video = ss::video;
	Now = Hop(S, C, Now, static_cast<double>(DayNo) * Day + 21.0 * 60.0);
	Fed(S);
	const std::vector<video::Candidate> Open = S.VideoCandidates();
	if (Open.empty())
	{
		++C.UploadsMissed;
		return Now;
	}
	video::Choice Ch;
	Ch.F = static_cast<video::Format>(C.Uploads % 3);
	Ch.T = video::TitleStyle::Story;
	Ch.Fr = video::Frame::Reaction;
	Ch.Candidates = {Open.front().Id};
	if (Ch.F == video::Format::Vlog)
	{
		Ch.F = video::Format::Breakdown;
		const auto Lead = std::find_if(Open.begin(), Open.end(), [](const video::Candidate& X) { return X.K != video::Kind::Story; });
		if (Lead != Open.end())
		{
			video::Choice Night = Ch;
			Night.F = video::Format::Vlog;
			Night.Candidates.clear();
			for (const video::Candidate& X : Open)
			{
				if (Night.Candidates.size() < 8 && X.K != video::Kind::Story && CreatorNight(X.At) == CreatorNight(Lead->At))
				{
					Night.Candidates.push_back(X.Id);
				}
			}
			Ch = video::CanUpload(Night, S.Life.Video.Open).empty() ? Night : Ch;
		}
	}
	if (!S.Publish(Ch).empty())
	{
		++C.UploadsMissed;
		return Now;
	}
	for (int Guard = 0; S.TimeSkip.Active && Guard < 200; ++Guard)
	{
		Now = Wait(S, Now, 0.5);
	}
	++C.Uploads;
	++C.ByFormat[static_cast<size_t>(Ch.F)];
	C.Ledger.Take(S.Life);
	return Now;
}

/**
 * A career of Days days, from Night One's $2.37: a shift each morning until there's a cushion over the rent, a hyper online
 * every other afternoon (streamed, for the streamer, once the rig can), the Saturday Nightly on its own money or a backer's
 * (live-only, mixed), and every deal on the table taken when Deals is on. (The suite's budget sets the pace: about 25 s a
 * career.)
 */
void Run(Career& C)
{
	PaceHooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, C.Seed));
	ss::hero::Character Who;
	Who.FirstName = "Jesse";
	Who.LastName = "Cole";
	Who.Story = ss::hero::Background::DealersKid;
	Who.Created = true;
	S->NewCareer(Who);
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.1);
	C.Ledger.Take(S->Life);
	const double Start = S->WorldMinutes();
	C.Start = Start;
	const int FirstDay = ss::net::DayOf(Start);
	for (int K = 0; K < C.Days; ++K)
	{
		const int DayNo = FirstDay + K;
		Now = Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 7.0 * 60.0);
		Fed(*S);
		// Locked out (Night One's $1,225 by Friday is out of reach for most): the storage unit kept up, and back in as soon
		// as the back rent and a month up front are there.
		if (S->Evicted() && S->Life.StorageDue > 0.0 && S->Life.StorageDue < S->WorldMinutes() + 2.0 * Day)
		{
			S->PayStorage();
		}
		if (S->Evicted() && S->BankrollCents >= S->Life.RentDueCents + 20000)
		{
			S->MoveBackIn();
		}
		C.Ledger.Take(S->Life);
		TakeDeals(*S, C);
		if (C.Creator && !S->Evicted())
		{
			CreatorRig(*S, C);
		}
		if (S->BankrollCents < (S->Evicted() ? S->Life.RentDueCents : ss::life::MonthlyRentCents) + 40000)
		{
			Now = Shift(*S, C, Now);
		}
		// G3's multi-site career: its own day online (SitesDay), the rivals in it once they've asked.
		if (C.Sites)
		{
			Now = SitesDay(*S, C, Now, DayNo, K);
		}
		else
		{
			Now = Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 14.0 * 60.0);
			Fed(*S);
		}
		if (!C.Sites && C.Kind != 0 && K % 2 == 0)
		{
			if (C.Kind == 1 && !S->Owns("ram-32") && S->BankrollCents >= 30000)
			{
				S->Buy("ram-32");
				C.Ledger.Take(S->Life);
			}
			const bool OnAir = C.Kind == 1 && S->GoLive().empty();
			C.Streams += OnAir ? 1 : 0;
			Now = Online(*S, C, Now);
			if (OnAir)
			{
				Now = Wait(*S, Now, 0.5);
				S->EndStream();
			}
		}
		if (C.Kind != 1 && ss::net::Weekday(DayNo) == 5)
		{
			Now = LiveNight(S, H, C, Now, DayNo);
		}
		// G2's creator: Tuesday, Thursday and Saturday (the career's days 0, 2 and 4 of each week), from day 7, after the day's play.
		if (C.Creator && K >= 7 && (K % 7 == 0 || K % 7 == 2 || K % 7 == 4))
		{
			Now = CreatorUpload(*S, C, Now, DayNo);
		}
		C.TierByDay.push_back(S->SponsorTier());
		const int Done = K + 1;
		if (Done == 30 || Done == 60 || Done == 90 || Done == C.Days)
		{
			const size_t At = Done == 30 ? 0 : Done == 60 ? 1 : Done == 90 ? 2 : 3;
			C.Bank[At] = S->BankrollCents;
			C.Subs[At] = ss::video::SubsAt(S->Life.Video, S->WorldMinutes());
			C.Views[At] = ss::video::TotalViews(S->Life.Video, S->WorldMinutes());
		}
		if (Done == 30 || Done == 60 || Done == 90)
		{
			C.Ledger.Take(S->Life);
			ss::Chips Earned = 0;
			for (const life::LedgerEntry& E : C.Ledger.All)
			{
				Earned += Income(E) ? E.Amount : 0;
			}
			C.DealsTo[static_cast<size_t>(Done / 30 - 1)] = DealValue(C.Ledger);
			C.IncomeTo[static_cast<size_t>(Done / 30 - 1)] = Earned;
		}
	}
	if (C.Creator)
	{
		C.Ledger.Take(S->Life);
	}
	C.PartnerAt = S->Life.Video.PartnerAt;
	C.Kast = S->Channel.Followers;
	C.Rig = S->GearFx().Quality;
	if (C.Sites)
	{
		C.Ledger.Take(S->Life);
		C.SitesEnd = SitesMoney(*S, true);
		for (const ss::site::Account& A : S->Life.Sites.Accounts)
		{
			C.JoinedSites += A.Deposited > 0 ? 1 : 0;
		}
		const ss::rep::Public P = S->Record();
		C.EventsEnd = P.OnlineEvents;
		C.Abi100End = P.Abi100;
	}
}

/** The last segment's six checks on one career (the deals on; the sixth as restated in C1, against the career's income). */
void Checks(const Career& On)
{
	const char* Name = On.Creator ? "creator" : On.Sites ? "multi-site" : On.Kind == 0 ? "live-only" : On.Kind == 1 ? "streaming" : "mixed";
	const double Start = On.Ledger.All.empty() ? 0.0 : On.Ledger.All.front().At;
	// 1. Sponsor cash under 20% of income for the first 60 days.
	ss::Chips Sponsor60 = 0;
	ss::Chips Income60 = 0;
	for (const life::LedgerEntry& E : On.Ledger.All)
	{
		if (E.At < Start + 60.0 * Day && Income(E))
		{
			Income60 += E.Amount;
			Sponsor60 += E.Kind == life::LedgerSponsors ? E.Amount : 0;
		}
	}
	// 2. No month's rent covered by sponsorship alone (in-kind at face: every sponsor line in) before tier 3.
	ss::Chips WorstMonth = 0;
	for (size_t D = 0; D < On.TierByDay.size(); ++D)
	{
		if (On.TierByDay[D] >= 3)
		{
			break;
		}
		const double To = Start + static_cast<double>(D + 1) * Day;
		ss::Chips Month = 0;
		for (const life::LedgerEntry& E : On.Ledger.All)
		{
			Month += E.Kind == life::LedgerSponsors && E.Amount > 0 && E.At > To - 30.0 * Day && E.At <= To ? E.Amount : 0;
		}
		WorstMonth = std::max(WorstMonth, Month);
	}
	// 5. Big Lou: at most LouMonthCap stakes in any 30 days (each worth $40 to $70 to the player: staking_backing).
	int LouMax = 0;
	for (size_t I = 0; I < On.LouAt.size(); ++I)
	{
		int In = 0;
		for (size_t J = I; J < On.LouAt.size() && On.LouAt[J] < On.LouAt[I] + 30.0 * Day; ++J)
		{
			++In;
		}
		LouMax = std::max(LouMax, In);
	}
	std::printf("  pacing %s (%d days): %d online, %d live, %d streams, %d deals signed; sponsor cash $%.2f of $%.2f income in 60 days; the worst month's "
				"sponsorship before tier 3 $%.2f; Lou %zu stakes (at most %d in 30 days); Lantern Row %d offers; %d rent deadlines checked; bankroll day 30 $%.2f, "
				"day 60 $%.2f, day 90 $%.2f, last $%.2f\n",
		Name, On.Days, On.Online, On.Live, On.Streams, On.Signed, static_cast<double>(Sponsor60) / 100.0, static_cast<double>(Income60) / 100.0,
		static_cast<double>(WorstMonth) / 100.0, On.LouAt.size(), LouMax, On.Lantern, On.RentChecks, static_cast<double>(On.Bank[0]) / 100.0,
		static_cast<double>(On.Bank[1]) / 100.0, static_cast<double>(On.Bank[2]) / 100.0, static_cast<double>(On.Bank[3]) / 100.0);
	auto Share = [&](size_t K) { return On.IncomeTo[K] > 0 ? 100.0 * static_cast<double>(On.DealsTo[K]) / static_cast<double>(On.IncomeTo[K]) : 0.0; };
	std::printf("  pacing %s: the deals' value (cash, and in-kind at face) $%.2f of $%.2f income by day 30 (%.1f%%), $%.2f of $%.2f by day 60 (%.1f%%), $%.2f of $%.2f "
				"by day 90 (%.1f%%)\n",
		Name, static_cast<double>(On.DealsTo[0]) / 100.0, static_cast<double>(On.IncomeTo[0]) / 100.0, Share(0), static_cast<double>(On.DealsTo[1]) / 100.0,
		static_cast<double>(On.IncomeTo[1]) / 100.0, Share(1), static_cast<double>(On.DealsTo[2]) / 100.0, static_cast<double>(On.IncomeTo[2]) / 100.0, Share(2));
	const std::string Tag = std::string("pacing (") + Name + "): ";
	Expect(Income60 <= 0 || Sponsor60 * 5 < Income60, (Tag + "sponsor cash stays under 20% of income for the first 60 days").c_str());
	Expect(WorstMonth < ss::life::MonthlyRentCents, (Tag + "no month's rent is covered by sponsorship alone before tier 3").c_str());
	// 3. The staked roll never pays rent (enforced in core): no package roll moves at a rent deadline.
	Expect(!On.RollPaidRent, (Tag + "the staked roll never pays rent").c_str());
	// 4. Lantern Row's offers are its tiers' terms, which staking_backing's Monte Carlo proves positive for the backer.
	Expect(!On.LanternOffTier, (Tag + "Lantern Row's offers are never negative-EV for the backer (each a tier's terms)").c_str());
	Expect(LouMax <= ss::stake::LouMonthCap, (Tag + "Lou's stakes move at most $300 of EV in any 30 days (at most four)").c_str());
	// 6. On days 30, 60 and 90, the deals' value (cash, and in-kind at face: covered entries, fares, credit) is at most 15%
	// of the career's income to date: deals help; they don't carry a career. (Restated in C1, update2/C1_REPORT.md §3: the
	// last segment's "median bankroll with the deals on within +15% of the same seeds' with them off" came down to one
	// career's deal on a three-career median, and no sponsor that covers even one $120 Nightly fits +15% of the live-only
	// career's $736 roll on day 90. That comparison is printed in "long", for information.)
	for (size_t K = 0; K < 3; ++K)
	{
		Expect(On.DealsTo[K] * 100 <= std::max<ss::Chips>(On.IncomeTo[K], 0) * 15,
			(Tag + "the deals' value (cash, and in-kind at face) stays at most 15% of the career's income to date (day " + std::to_string(30 * (K + 1)) + ")").c_str());
	}
	// U11 (update 2, from G3): RiverLine Rewards stays under 3% of the rent in Act I (until the second rent is paid).
	const double Act1 = ActIEndOf(On.Ledger);
	ss::Chips Rewards1 = 0;
	ss::Chips RewardsAll = 0;
	for (const life::LedgerEntry& E : On.Ledger.All)
	{
		const bool Rewards = E.Kind == 4 && E.Label.rfind("RiverLine Rewards:", 0) == 0;
		Rewards1 = std::max(Rewards1, Rewards && E.At < Act1 ? E.Amount : 0);
		RewardsAll += Rewards ? E.Amount : 0;
	}
	std::printf("  pacing %s: Act I %s; RiverLine Rewards paid %s in all, at most %s a month in Act I (under 3%% of the rent: %s)\n", Name,
		Act1 < 1.0e17 ? ("over on day " + std::to_string(static_cast<int>((Act1 - Start) / Day))).c_str() : "not over", ss::Money(RewardsAll).c_str(), ss::Money(Rewards1).c_str(),
		ss::Money(life::MonthlyRentCents * 3 / 100).c_str());
	Expect(Rewards1 * 100 < life::MonthlyRentCents * 3, (Tag + "RiverLine Rewards stays under 3% of the rent in Act I (U11)").c_str());
}

/** A finish as a percentile of the field (100 the winner, 0 last; GAME_DESIGN's "average finish", 50 the middle). */
double Percentile(const EventRec& E)
{
	return E.Field > 1 ? 100.0 * static_cast<double>(E.Field - E.Place) / static_cast<double>(E.Field - 1) : 50.0;
}

/** The mean of V and its standard error. */
std::pair<double, double> MeanSe(const std::vector<double>& V)
{
	if (V.empty())
	{
		return {0.0, 0.0};
	}
	double Sum = 0.0;
	for (const double X : V)
	{
		Sum += X;
	}
	const double Mean = Sum / static_cast<double>(V.size());
	double Sq = 0.0;
	for (const double X : V)
	{
		Sq += (X - Mean) * (X - Mean);
	}
	const double Sd = V.size() > 1 ? std::sqrt(Sq / static_cast<double>(V.size() - 1)) : 0.0;
	return {Mean, Sd / std::sqrt(static_cast<double>(V.size()))};
}

/**
 * G1's career (update 2 SPEC §1.11, U3): the autoplay career against its twin, the same seed and schedule played today's
 * way (Sprint, the hands it hands back played with the grader's best), over the autoplay career's days.
 *   - Autoplay buys time, not hands: events per world hour (the career) and hands per world hour at the online tables at
 *     most 1.05x the twin's.
 *   - It doesn't out-earn the careful career: on the events the plan played, paired with the twin's same events, neither
 *     the finish percentile nor the ROI is significantly higher (the paired mean difference must not exceed 2 SE; a
 *     money ratio over one career is decided by a few deep runs, so the honest claim is the paired one, and the strength
 *     harness's intervals hold the edge itself). The prize totals and the bankrolls are printed beside it.
 */
void AutoChecks(const Career& Auto, const Career& Twin)
{
	const double Begin = Auto.Events.empty() ? 0.0 : std::min(Auto.Events.front().At, Twin.Events.empty() ? Auto.Events.front().At : Twin.Events.front().At);
	const double Limit = static_cast<double>(ss::net::DayOf(Begin) + Auto.Days) * Day;
	auto Rates = [&](const Career& C, int& Events, double& HandsPerHour) {
		Events = 0;
		int Hands = 0;
		double Minutes = 0.0;
		for (const EventRec& E : C.Events)
		{
			if (E.At >= Limit)
			{
				continue;
			}
			++Events;
			if (!E.Live)
			{
				Hands += E.Hands;
				Minutes += E.Minutes;
			}
		}
		HandsPerHour = Minutes > 0.0 ? 60.0 * static_cast<double>(Hands) / Minutes : 0.0;
	};
	int EventsA = 0;
	int EventsT = 0;
	double HandsA = 0.0;
	double HandsT = 0.0;
	Rates(Auto, EventsA, HandsA);
	Rates(Twin, EventsT, HandsT);
	const double Hours = static_cast<double>(Auto.Days) * 24.0;
	// The paired comparison on the events the plan played.
	std::vector<double> Finish;
	std::vector<double> Roi;
	ss::Chips PrizeA = 0;
	ss::Chips PrizeT = 0;
	int Plan = 0;
	int PlanLive = 0;
	for (const EventRec& A : Auto.Events)
	{
		if (!A.Plan)
		{
			continue;
		}
		++Plan;
		PlanLive += A.Live ? 1 : 0;
		for (const EventRec& T : Twin.Events)
		{
			if (T.Id == A.Id && T.BuyIn > 0)
			{
				Finish.push_back(Percentile(A) - Percentile(T));
				Roi.push_back(static_cast<double>(A.Prize - T.Prize) / static_cast<double>(T.BuyIn));
				PrizeA += A.Prize;
				PrizeT += T.Prize;
				break;
			}
		}
	}
	const std::pair<double, double> F = MeanSe(Finish);
	const std::pair<double, double> R = MeanSe(Roi);
	std::printf("  pacing autoplay (%d days, every other event TAG skipped at Sharpness 0.80): %d events (%d by the plan, %d of them live), the twin %d; events per world hour "
				"%.4f vs %.4f; hands per world hour online %.1f vs %.1f\n",
		Auto.Days, EventsA, Plan, PlanLive, EventsT, static_cast<double>(EventsA) / Hours, static_cast<double>(EventsT) / Hours, HandsA, HandsT);
	std::printf("  pacing autoplay: %zu paired events: finish percentile %+.2f (SE %.2f), ROI %+.3f buy-ins (SE %.3f); their prizes $%.2f by the plan, $%.2f by the twin (ratio %.2f); "
				"bankroll on day 30 $%.2f vs $%.2f, day 60 $%.2f vs $%.2f, last $%.2f\n",
		Finish.size(), F.first, F.second, R.first, R.second, static_cast<double>(PrizeA) / 100.0, static_cast<double>(PrizeT) / 100.0,
		PrizeT > 0 ? static_cast<double>(PrizeA) / static_cast<double>(PrizeT) : 0.0, static_cast<double>(Auto.Bank[0]) / 100.0, static_cast<double>(Twin.Bank[0]) / 100.0,
		static_cast<double>(Auto.Bank[1]) / 100.0, static_cast<double>(Twin.Bank[1]) / 100.0, static_cast<double>(Auto.Bank[Auto.Days <= 60 ? 1 : 3]) / 100.0);
	Expect(EventsT > 0 && static_cast<double>(EventsA) <= 1.05 * static_cast<double>(EventsT), "pacing (autoplay): events per world hour at most 1.05x the same seeds sprinted with Autopilot");
	Expect(HandsT > 0.0 && HandsA <= 1.05 * HandsT, "pacing (autoplay): hands per world hour at the online tables at most 1.05x Sprint's (autoplay buys time, not hands)");
	Expect(Finish.size() >= 5, "pacing (autoplay): the plan's events pair with the twin's");
	Expect(F.first <= 2.0 * F.second && R.first <= 2.0 * R.second, "pacing (autoplay): the plan at the default Sharpness doesn't out-finish or out-earn the careful career on the same events (U3)");
}

/**
 * G2's creator career (update 2 SPEC §1.11, U11): Rerun pays nothing before its partner gate, so
 *   - no month's rent before day 60 is covered by Rerun (each rent paid or collected in the first 60 days, against what
 *     Rerun paid into the bank in the 30 days before it, and all Rerun paid in those days under one month's rent: a career
 *     locked out by Night One's rent pays none till later; RiverLine Rewards, the check's other half, is G3's and pays
 *     nothing yet), and
 *   - Rerun's first payout (its first ledger line, kind 13) lands on day 90 or later.
 * VideoGrind (session_seg_video.cpp) holds the longer floors and ceilings in closed form; this is the same channel grown
 * from a career's own hands, through Studio's Publish and the calendar (CheckCalendar's VideoStep). Its numbers print.
 */
void CreatorChecks(const Career& C)
{
	double FirstPay = -1.0; // days from the career's start
	ss::Chips Paid = 0;
	ss::Chips Paid60 = 0; // what Rerun paid into the bank in the first 60 days, any rent or none
	int Lines = 0;
	for (const life::LedgerEntry& E : C.Ledger.All)
	{
		if (E.Kind == life::LedgerVideo && E.Amount > 0)
		{
			FirstPay = FirstPay < 0.0 ? (E.At - C.Start) / Day : FirstPay;
			Paid += E.Amount;
			Paid60 += E.At < C.Start + 60.0 * Day ? E.Amount : 0;
			++Lines;
		}
	}
	int Rents = 0;
	ss::Chips WorstCover = 0;
	bool Covered = false;
	for (const life::LedgerEntry& R : C.Ledger.All)
	{
		const bool Rent = R.Kind == 3 && R.Amount < 0 && (R.Label.rfind("Rent", 0) == 0 || R.Label.rfind("Back rent", 0) == 0);
		if (!Rent || R.At >= C.Start + 60.0 * Day)
		{
			continue;
		}
		++Rents;
		ss::Chips Rerun = 0;
		for (const life::LedgerEntry& E : C.Ledger.All)
		{
			Rerun += E.Kind == life::LedgerVideo && E.Amount > 0 && E.At > R.At - 30.0 * Day && E.At <= R.At ? E.Amount : 0;
		}
		WorstCover = std::max(WorstCover, Rerun);
		Covered = Covered || Rerun >= -R.Amount;
	}
	std::printf("  pacing creator (%d days, the rig at quality %.2f by the end): %d uploads (%d Shorts, %d Breakdowns, %d Vlogs, %d Recaps), %d upload days with nothing to cut; "
				"Rerun subscribers %.0f / %.0f / %.0f / %.0f on days 30 / 60 / 90 / %d, views %.0f / %.0f / %.0f / %.0f; Partner %s; %d payouts, $%.2f, the first %s; "
				"%d Kast followers\n",
		C.Days, C.Rig, C.Uploads, C.ByFormat[0], C.ByFormat[1], C.ByFormat[2], C.ByFormat[3], C.UploadsMissed, C.Subs[0], C.Subs[1], C.Subs[2], C.Subs[3], C.Days, C.Views[0],
		C.Views[1], C.Views[2], C.Views[3], C.PartnerAt >= 0.0 ? ("day " + std::to_string(static_cast<int>((C.PartnerAt - C.Start) / Day))).c_str() : "not in these days", Lines,
		static_cast<double>(Paid) / 100.0, FirstPay >= 0.0 ? ("on day " + std::to_string(static_cast<int>(FirstPay))).c_str() : "never", C.Kast);
	std::printf("  pacing creator: %d rents paid in the first 60 days, the most Rerun paid in the 30 days before one $%.2f; Rerun paid $%.2f in those 60 days\n", Rents,
		static_cast<double>(WorstCover) / 100.0, static_cast<double>(Paid60) / 100.0);
	Expect(C.Uploads >= C.Days / 4, "pacing (creator): the creator uploads about three times a week from day 7, cut from the career's own hands");
	Expect(!Covered && Paid60 < ss::life::MonthlyRentCents, "pacing (creator): no month's rent before day 60 is covered by Rerun");
	Expect(FirstPay < 0.0 || FirstPay >= 90.0, "pacing (creator): Rerun's first payout lands no earlier than day 90");
}

/**
 * G3's multi-site career (update 2 SPEC §1.11, §5.4, §5.5, U11):
 *   - site money never pays rent: at every rent deadline the career crossed with an account, each wallet moved only by its own
 *     lines (none of them the rent's), nothing in transit landed before its minute, and the bank moved only by its ledger;
 *   - whenever the bank alone couldn't cover a rent deadline but the bank and the sites could, the rent card's site line was
 *     showing: on every hour of the deadline's last 72 (it shows from 72 hours out, the earliest look);
 *   - no site invites before Act I is over (the second rent paid), and each rival's letter came at its own gate;
 *   - both rivals joined from their letters, with money moving both ways (transfers on).
 * Rewards under 3% of the rent in Act I is every career's (Checks).
 */
void SitesChecks(const Career& C)
{
	const double Act1 = ActIEndOf(C.Ledger);
	auto DayOf = [&C](double At) { return static_cast<int>((At - C.Start) / Day); };
	std::string Letters;
	bool Early = false;
	bool Ignite = false, Rit = false;
	bool Gates = true;
	for (size_t I = 0; I < C.Invited.size(); ++I)
	{
		const ss::net::SiteId Site = static_cast<ss::net::SiteId>(C.Invited[I].first);
		const ss::net::SiteDef& D = ss::net::Def(Site);
		const int Events = C.InvitedAt[I].first;
		const ss::Chips Abi = C.InvitedAt[I].second;
		Letters += (Letters.empty() ? "" : "; ") + D.Name + "'s on day " + std::to_string(DayOf(C.Invited[I].second)) + " (" + std::to_string(Events) + " tracked events, the last hundred's average " +
			ss::Money(Abi) + ")";
		Early = Early || C.Invited[I].second < Act1;
		Ignite = Ignite || Site == ss::net::SiteId::Ignite;
		Rit = Rit || Site == ss::net::SiteId::RunItTwice;
		// (The record is read the moment the letter is seen, a step at most after the gate passed.)
		Gates = Gates && Events >= D.Gate.MinEvents && (D.Gate.MinAbi100 == 0 || Abi >= D.Gate.MinAbi100);
	}
	const int Joined = C.JoinedSites;
	ss::Chips Fees = 0;
	for (const life::LedgerEntry& E : C.Ledger.All)
	{
		Fees += E.Kind == life::LedgerSites ? -E.Amount : 0;
	}
	std::printf("  pacing multi-site (%d days): %d online events (RiverLine %d, Ignite %d, RunItTwice %d; buy-ins %s, %s, %s), $11 and up from day %d; Act I over on day %d\n", C.Days,
		C.Online, C.BySite[0], C.BySite[1], C.BySite[2], ss::Money(C.BuyInsBySite[0]).c_str(), ss::Money(C.BuyInsBySite[1]).c_str(), ss::Money(C.BuyInsBySite[2]).c_str(), C.Phase2At,
		Act1 < 1.0e17 ? DayOf(Act1) : -1);
	std::printf("  pacing multi-site: invitations: %s\n", Letters.empty() ? "none" : Letters.c_str());
	std::printf("  pacing multi-site: %d deposits (%s), %d withdrawals (%s asked back), %s in card fees; %s on the sites at the end; %d rent deadlines with an account, "
				"%zu of them short in the bank with the sites enough (%d hourly looks, %d without the warning, the earliest it showed %.0f h out); %d with site money moving toward the "
				"rent%s\n",
		C.Deposits, ss::Money(C.DepositCents).c_str(), C.Withdrawals, ss::Money(C.WithdrawCents).c_str(), ss::Money(Fees).c_str(), ss::Money(C.SitesEnd).c_str(), C.RentDeadlines,
		C.ShortDeadlines.size(), C.WarnSamples, C.WarnMissed, C.WarnEarliest, C.SiteRentBreaks, C.SiteRentFirst.empty() ? "" : (" (" + C.SiteRentFirst + ")").c_str());
	Expect(C.SiteRentBreaks == 0, "pacing (multi-site): site money never pays rent (wallets move by their own lines, transfers land on their minute, the bank by its ledger)");
	Expect(C.WarnMissed == 0, "pacing (multi-site): whenever the bank alone can't cover a rent deadline but the bank and the sites could, the rent card's warning shows, from 72 hours out");
	Expect(!Early && Act1 < 1.0e17, "pacing (multi-site): no site invites before Act I is over (U11)");
	// (RunItTwice's door, an $11 average over the last hundred, takes this grinder past day 180: printed, not asserted. The
	// journey and the monkey walk through it.)
	std::printf("  pacing multi-site: at the end, %d tracked events, the last hundred's average %s; RunItTwice %s\n", C.EventsEnd, ss::Money(C.Abi100End).c_str(),
		Rit ? "joined from its letter" : "hasn't asked yet");
	Expect(Ignite && Gates && Joined == static_cast<int>(C.Invited.size()),
		"pacing (multi-site): each rival invites at its own gate (Ignite once Act I is over), and the career joins each the morning it asks");
	Expect(C.Deposits >= 2 && C.Withdrawals >= 1, "pacing (multi-site): money moves both ways (deposits by card, withdrawals to the bank)");
}

/** The median of the careers' bankrolls on day 30 (K 0), 60 (1) or 90 (2). */
ss::Chips Median(const std::vector<Career>& Runs, size_t K)
{
	std::vector<ss::Chips> V;
	for (const Career& C : Runs)
	{
		V.push_back(C.Bank[K]);
	}
	std::sort(V.begin(), V.end());
	return V.empty() ? 0 : V[V.size() / 2];
}
} // namespace pacing

/**
 * G2's career (update 2 SPEC §1.11, §7.5 C2): the creator, on the streaming career's seed and schedule with a Rerun upload
 * three times a week from day 7, the last segment's six checks and the creator's own (pacing::CreatorChecks).
 * In "long" only (180 days, about the time of a streaming career): the default run's budget has no room for it, and its
 * day-90 check needs the days.
 * Not §1.11's mid rig (the model's gear quality 0.5; StreamGrind's desktop, fiber and 1080p webcam): this seed's career is
 * locked out by Night One's rent from day 7 to day 70 (it pays no rent before then), and on Dee's couch the rig sits in
 * storage (the laptop alone, RefreshGear); back in, it buys the stream kit by day 75 (720p, quality 0.29). Its uploads are
 * the weaker for it. Rerun still pays nothing in 180 days with the partner gate at 5 subscribers and 5 hours, and $74 in all
 * with the $100 minimum at $1 too (which fails the day-90 check: review-c2-lead, 2026-10-19).
 */
void CreatorPacing(bool Long)
{
	if (!Long)
	{
		return;
	}
	const auto T0 = std::chrono::steady_clock::now();
	pacing::Career C;
	C.Seed = "pacing-1";
	C.Kind = 1;
	C.Creator = true;
	C.Days = 180;
	pacing::Run(C);
	pacing::Checks(C);
	pacing::CreatorChecks(C);
	std::printf("  pacing creator: %.1f s\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

/**
 * G3's career (update 2 SPEC §1.11, §7.5 C3): the multi-site grinder, 180 days from Night One's $2.37 on the mixed career's
 * life (a shift when the money's short, the Saturday Nightly, the deals taken), online every afternoon (pacing::SitesDay):
 * RiverLine's hypers to 200 tracked events, then $11 and up while the bank holds the rent; Ignite and RunItTwice joined the
 * morning each one's own letter comes, $50 by card on each, what piles up withdrawn, and everything there when a rent is near
 * and the bank short. The last segment's six checks, U11's Rewards, and the gate's own three (pacing::SitesChecks). In "long"
 * only: about the time of the creator's career.
 */
void SitesPacing(bool Long)
{
	if (!Long)
	{
		return;
	}
	const auto T0 = std::chrono::steady_clock::now();
	pacing::Career C;
	C.Seed = "pacing-sites";
	C.Kind = 2;
	C.Sites = true;
	C.Days = 180;
	pacing::Run(C);
	pacing::Checks(C);
	pacing::SitesChecks(C);
	std::printf("  pacing multi-site: bankroll day 30 %s, day 60 %s, day 90 %s, day 180 %s; %.1f s\n", ss::Money(C.Bank[0]).c_str(), ss::Money(C.Bank[1]).c_str(), ss::Money(C.Bank[2]).c_str(),
		ss::Money(C.Bank[3]).c_str(), std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count());
}

/**
 * EconomyPacing (update 2 SPEC §1.11, carried from next_segment_spec §1.11): seeded 180-day careers (live-only, streaming,
 * mixed) with every feature on. The default run plays the mixed career's first 90 days (every check still reads: check 1
 * 60 days, check 6 days 30, 60 and 90, G1's twin its 60; C1 moved the rest behind "long" for the suite's 120 s budget);
 * "session_test long" plays all three whole, each beside its own seed with backing and sponsors off for the bankroll
 * comparison the sixth check used to be (printed, not asserted: C1). The checks are the last segment's six; each gate
 * adds a career of its own (G1 autoplay, G2 creator, G3 multi-site).
 */
void EconomyPacing(bool Long)
{
	const int Kinds[3] = {2, 0, 1};
	std::vector<pacing::Career> Ons;
	std::vector<pacing::Career> Offs;
	for (int K = 0; K < (Long ? 3 : 1); ++K)
	{
		pacing::Career On;
		On.Seed = "pacing-" + std::to_string(Kinds[K]);
		On.Kind = Kinds[K];
		On.Days = Long ? 180 : 90;
		pacing::Run(On);
		pacing::Checks(On);
		Ons.push_back(On);
		if (Long)
		{
			pacing::Career Off;
			Off.Seed = On.Seed;
			Off.Kind = On.Kind;
			Off.Deals = false;
			Off.Days = 90; // the comparison reads days 30, 60 and 90
			pacing::Run(Off);
			Offs.push_back(Off);
		}
	}
	// The sixth check as the last segment had it: on days 30, 60 and 90, the median bankroll with backing and sponsors on
	// within +15% of the same seeds' with them off. Information since C1 (check 6 is now the deals against income, in
	// Checks): on three careers the median is one career's, and one covered Nightly is more than 15% of a thin roll. In
	// "long" only: the deals-off twins cost a 90-day career each (about 16 s) and assert nothing; three seeds a kind would
	// add about 200 s to "long", so it stays on the three careers' own seeds.
	for (size_t I = 0; I < Offs.size(); ++I)
	{
		const char* Name = Ons[I].Kind == 0 ? "live-only" : Ons[I].Kind == 1 ? "streaming" : "mixed";
		std::printf("  pacing %s (information, not a check): the bankroll with the deals on/off, day 30 $%.2f/$%.2f, day 60 $%.2f/$%.2f, day 90 $%.2f/$%.2f\n", Name,
			static_cast<double>(Ons[I].Bank[0]) / 100.0, static_cast<double>(Offs[I].Bank[0]) / 100.0, static_cast<double>(Ons[I].Bank[1]) / 100.0,
			static_cast<double>(Offs[I].Bank[1]) / 100.0, static_cast<double>(Ons[I].Bank[2]) / 100.0, static_cast<double>(Offs[I].Bank[2]) / 100.0);
	}
	for (size_t K = 0; K < 3 && !Offs.empty(); ++K)
	{
		const ss::Chips A = pacing::Median(Ons, K);
		const ss::Chips B = pacing::Median(Offs, K);
		std::printf("  pacing (information, not a check): day %zu, the median bankroll $%.2f with the deals on, $%.2f off (%+.0f%%; the old bound +15%%)\n", 30 * (K + 1),
			static_cast<double>(A) / 100.0, static_cast<double>(B) / 100.0, B > 0 ? 100.0 * static_cast<double>(A - B) / static_cast<double>(B) : 0.0);
	}
	// G1's career (update 2 SPEC §1.11): the autoplay career on the mixed career's seed and schedule, the mixed career its twin.
	// 60 days by default; the whole 180 in "long".
	{
		pacing::Career Auto;
		Auto.Seed = Ons[0].Seed;
		Auto.Kind = Ons[0].Kind;
		Auto.Auto = true;
		Auto.Days = Long ? 180 : 60;
		pacing::Run(Auto);
		pacing::AutoChecks(Auto, Ons[0]);
	}
	CreatorPacing(Long);
	SitesPacing(Long);
}

/**
 * MigrationAll (update 2 SPEC §6.5, §7.2 day 5) over the day-0 fixtures (fixture_tool write2, from bda07f3): every one
 * parses, writes back byte for byte (§6.4: no update 2 line is written before its Update2LinesRead bit is set, and loading
 * sets none, so an untouched old save comes back as it was, at every gate), loads a second time the same, loads into a
 * session, and keeps its facts: the staked entry's funding and checkpoint, the sponsor book, Mail and the clip, the
 * veteran's results and badges, the eviction. The world's own text reads and writes back too. From C2 the gate's own
 * fixture, update2_creator (fixture_tool write-g2), goes through the same chain (its facts: seams::Gate2Fixtures).
 */
void MigrationAll()
{
	namespace life = ss::life;
	const char* Names[] = {"pre_update2_fresh.ssave", "pre_update2_staked_mid_nightly.ssave", "pre_update2_sponsored_streamer.ssave", "pre_update2_evicted.ssave",
		"pre_update2_veteran.ssave", "update2_creator.ssave"};
	for (const char* Name : Names)
	{
		const std::string Saved = seams::Fixture(Name);
		ss::SaveData D;
		const bool Parsed = Saved.rfind("shortstack.nightone.v2\n", 0) == 0 && ss::SaveData::Parse(Saved, D);
		Expect(Parsed, (std::string("migration: ") + Name + " is there and parses").c_str());
		const std::string Again = D.Serialize();
		const bool Exact = Again == Saved;
		Expect(Exact, (std::string("migration: ") + Name + " writes back byte for byte (an untouched old save gains no update 2 line)").c_str());
		ss::SaveData Second;
		Expect(ss::SaveData::Parse(Again, Second) && Second.Serialize() == Again, (std::string("migration: ") + Name + " loads a second time the same").c_str());
		Hooks H;
		ss::Session S(H, std::string("migration-") + Name, &D);
		S.Save();
		Expect(H.Saves > 0 && H.Last.BankrollCents == D.BankrollCents && H.Last.History.size() == D.History.size(), (std::string("migration: ") + Name + " loads into a session").c_str());
		std::printf("  migration: %s: %s (%zu bytes)\n", Name, Exact ? "written back byte for byte" : "NOT written back byte for byte", Again.size());
	}
	// The facts each one keeps.
	{
		ss::SaveData D;
		ss::SaveData::Parse(seams::Fixture("pre_update2_fresh.ssave"), D);
		Expect(D.BankrollCents == 237 + ss::hero::PerksOf(D.Person).StartCents && D.Life.RentDueCents == 122500 && D.History.empty(), "migration: the fresh career");
	}
	{
		ss::SaveData D;
		ss::SaveData::Parse(seams::Fixture("pre_update2_staked_mid_nightly.ssave"), D);
		const life::LiveEntry* E = ss::live::EntryFor(D.Life, "embercrest-nightly@9");
		Expect(E && E->State == life::LiveEntry::Registered && E->Fund.From == life::Paid::Backer && E->Fund.Who == "Big Lou" && E->Fund.Permille == 1000 && E->Fund.Cents == 12000 &&
				   E->Checkpoint.rfind("ck1 ", 0) == 0 && !E->Voices.empty() && !E->Room.empty() && !E->Badges.empty() && !E->Leaders.empty(),
			"migration: tonight's Nightly on Big Lou's money, checkpointed, with its voices, room, badges and leaders");
		Hooks H;
		ss::Session S(H, "migration-staked", &D);
		const life::LiveEntry* Kept = ss::live::EntryFor(S.Life, "embercrest-nightly@9");
		Expect(S.LiveInProgress() == "embercrest-nightly@9" && Kept && Kept->Fund.Who == "Big Lou" && Kept->Checkpoint == E->Checkpoint && S.Life.Stakes.Deals.size() == 1,
			"migration: the session goes back to the night on Lou's stake");
	}
	{
		ss::SaveData D;
		ss::SaveData::Parse(seams::Fixture("pre_update2_sponsored_streamer.ssave"), D);
		Hooks H;
		ss::Session S(H, "migration-streamer", &D);
		const double World = S.WorldMinutes();
		Expect(ss::sponsor::Active(S.Life.Sponsors, "voltrush", World) && !ss::sponsor::OpenOfferFor(S.Life.Sponsors, "halo", World).empty() && S.Life.Inbox.size() == 3 &&
				   S.Life.Closet.Patches[2] == "voltrush",
			"migration: Volt Rush signed and on the cap, Halo's offer open, three letters");
		Expect(!S.Channel.Clips.empty() && !S.Channel.Log.empty() && S.Channel.Followers >= 2400 && D.History.size() == 3, "migration: Kast's clip and log, the hypers played on stream");
		int Graded = 0;
		for (const ss::coach::Tally& T : S.Life.Study.All)
		{
			Graded += T.Decisions;
		}
		Expect(Graded > 0, "migration: the coach's notes");
	}
	{
		ss::SaveData D;
		ss::SaveData::Parse(seams::Fixture("pre_update2_evicted.ssave"), D);
		Expect(D.Life.RentStage == life::Rent::Evicted && D.Life.StorageDue > 0.0 && D.Life.Evictions == 1, "migration: still evicted, the gear in storage");
	}
	{
		ss::SaveData D;
		ss::SaveData::Parse(seams::Fixture("pre_update2_veteran.ssave"), D);
		Hooks H;
		ss::Session S(H, "migration-veteran", &D);
		const ss::badge::Badge* Club = S.HeroBadges().Find(ss::badge::Kind::Club);
		const ss::badge::Badge* Verified = S.HeroBadges().Find(ss::badge::Kind::VerifiedPro);
		Expect(D.History.size() == 100 && D.Life.LiveWins >= 1 && !D.Life.LiveDays.empty() && D.Life.VerifiedHeld && Club && Verified,
			"migration: the veteran's hundred results, a title on the wall, the live days, the club card and Verified");
	}
	// The world's own lines (version 2): read and written back, a second read the same. At version 2 byte for byte; from
	// version 3 (update 2 G1, §6.5) the same text but the head, which moves to the new version and gains Start (0,
	// Established) after HeroCardPrev: nothing else in an old world changes.
	{
		const std::string Text = seams::Fixture("world_v2.txt");
		auto ReadWorld = [](const std::string& T, ss::world::World& W) {
			size_t Start = 0;
			while (Start < T.size())
			{
				size_t End = T.find('\n', Start);
				End = End == std::string::npos ? T.size() : End;
				std::vector<std::string> Fields(1);
				for (size_t K = Start; K < End; ++K)
				{
					if (T[K] == '\t')
					{
						Fields.emplace_back();
					}
					else
					{
						Fields.back().push_back(T[K]);
					}
				}
				W.Read(Fields);
				Start = End + 1;
			}
			W.Finish();
		};
		std::string Expected = Text;
		const size_t HeadEnd = Text.find('\n');
		const std::string V2 = "world\thead\t2";
		if (ss::world::Version >= 3 && HeadEnd != std::string::npos && Text.rfind(V2 + "\t", 0) == 0)
		{
			Expected = "world\thead\t" + std::to_string(ss::world::Version) + Text.substr(V2.size(), HeadEnd - V2.size()) + "\t0" + Text.substr(HeadEnd);
		}
		ss::world::World W;
		ReadWorld(Text, W);
		std::string Again;
		W.Write(Again);
		ss::world::World Second;
		ReadWorld(Again, Second);
		std::string Third;
		Second.Write(Third);
		// From version 4 (update 2 G3, §6.4, §6.5) an old world also gains the sites: each npc line one field more (Sites,
		// appended), and new "world\tsite..." lines (the launch days, the sparse site maps). Everything else is version 3's,
		// line for line (the head may append its own new fields).
		auto SameButSites = [](const std::string& Want, const std::string& Got) {
			auto Lines = [](const std::string& T) {
				std::vector<std::string> Out;
				for (size_t Start = 0; Start < T.size();)
				{
					const size_t End = std::min(T.find('\n', Start), T.size());
					Out.push_back(T.substr(Start, End - Start));
					Start = End + 1;
				}
				return Out;
			};
			const std::vector<std::string> A = Lines(Want);
			size_t I = 0;
			for (const std::string& L : Lines(Got))
			{
				if (L.rfind("world\tsite", 0) == 0)
				{
					continue;
				}
				if (I >= A.size())
				{
					return false;
				}
				const std::string& E = A[I++];
				const bool Grows = E.rfind("world\tnpc\t", 0) == 0 || E.rfind("world\thead\t", 0) == 0;
				const bool OneMore = L.size() > E.size() + 1 && L.compare(0, E.size(), E) == 0 && L[E.size()] == '\t' && L.find('\t', E.size() + 1) == std::string::npos;
				if (!(L == E || (Grows && (OneMore || (E.rfind("world\thead\t", 0) == 0 && L.compare(0, E.size(), E) == 0)))))
				{
					return false;
				}
			}
			return I == A.size();
		};
		const bool Kept = ss::world::Version >= 4 ? SameButSites(Expected, Again) : Again == Expected;
		Expect(W.Ready() && Text.rfind(V2 + "\t", 0) == 0 && Kept && Third == Again && W.Began() == ss::world::Start::Established &&
				   Second.Began() == ss::world::Start::Established,
			"migration: world_v2.txt reads and writes back as it was (from version 3, but the head's version and Start; from version 4, the sites), an "
			"established world");
	}
}

namespace migration2
{
namespace life = ss::life;
namespace net = ss::net;
namespace site = ss::site;

double Since(const std::chrono::steady_clock::time_point& T0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
}

/** The save a session writes now, its world's text in it (a session may hand the host a snapshot to write instead). */
std::string SavedNow(ss::Session& S, Hooks& H)
{
	S.Save();
	ss::SaveData D = H.Last;
	if (D.WorldText.empty() && D.WorldSnapshot)
	{
		D.WorldSnapshot->Write(D.WorldText);
	}
	return D.Serialize();
}

/** The world head's version in a save's text (0: no world lines). */
int WorldVersionIn(const std::string& Text)
{
	const std::string Head = "world\thead\t";
	size_t At = Text.rfind(Head, 0) == 0 ? 0 : Text.find("\n" + Head);
	if (At == std::string::npos)
	{
		return 0;
	}
	At += Text[At] == '\n' ? 1 : 0;
	return std::atoi(Text.c_str() + At + Head.size());
}

/** Where two texts first differ, for whoever reads a failure. */
std::string FirstDifference(const std::string& A, const std::string& B)
{
	size_t Line = 1;
	size_t I = 0;
	while (I < A.size() && I < B.size() && A[I] == B[I])
	{
		Line += A[I] == '\n' ? 1 : 0;
		++I;
	}
	auto LineAt = [I](const std::string& T) {
		const size_t From = I == 0 ? std::string::npos : T.rfind('\n', I - 1);
		const size_t Start = From == std::string::npos ? 0 : From + 1;
		const size_t End = std::min(T.find('\n', Start), T.size());
		return T.substr(Start, std::min<size_t>(160, End - Start));
	};
	return "line " + std::to_string(Line) + ": \"" + LineAt(A) + "\" against \"" + LineAt(B) + "\"";
}

/** A save's world taken back to version 3's lines (update 2 G1's): the head's version, no Sites at the end of each npc line,
 *  no "world\tsite..." lines. Everything else as it is. */
std::string AsWorldV3(const std::string& Text)
{
	std::string Out;
	size_t Start = 0;
	while (Start < Text.size())
	{
		const size_t End = std::min(Text.find('\n', Start), Text.size());
		std::string Line = Text.substr(Start, End - Start);
		Start = End + 1;
		if (Line.rfind("world\tsite", 0) == 0)
		{
			continue;
		}
		if (Line.rfind("world\thead\t4\t", 0) == 0)
		{
			Line = "world\thead\t3\t" + Line.substr(std::string("world\thead\t4\t").size());
		}
		else if (Line.rfind("world\tnpc\t", 0) == 0)
		{
			Line = Line.substr(0, Line.rfind('\t'));
		}
		Out += Line + "\n";
	}
	return Out;
}

/** The career's Team RiverLine contract (the newest signed), or null. */
const ss::sponsor::Contract* TeamRiverLineDeal(const ss::Session& S)
{
	const ss::sponsor::Contract* Found = nullptr;
	for (const ss::sponsor::Contract& C : S.Life.Sponsors.Contracts)
	{
		Found = C.Brand == "riverline" && C.St != ss::sponsor::State::Offered ? &C : Found;
	}
	return Found;
}

/** A wallet's balance is its lines' sum (true while the site keeps every line: under 40). */
bool WalletAddsUp(const site::Account& A)
{
	ss::Chips Sum = 0;
	for (const site::Line& L : A.Lines)
	{
		Sum += L.Cents;
	}
	return A.Lines.size() >= 40 || Sum == A.Cents;
}

/**
 * The facts a fixture keeps once it's in a session (MigrationAll2): its money, its record, its book, and whatever made it
 * that fixture. An old save stays an Established, RiverLine-only career (U10): no account, no site money, no "Poker & sites"
 * fee, no clause on a deal signed before the clause existed.
 */
void Facts(const std::string& Name, ss::Session& S, const ss::SaveData& D)
{
	const std::string Tag = "migration (all): " + Name + ": ";
	const bool Multi = Name == "update2_multisite.ssave";
	const bool Blank = Name == "update2_blank_world.ssave";
	ss::Chips Fees = 0;
	for (const life::LedgerEntry& E : S.Life.Ledger)
	{
		Fees += E.Kind == life::LedgerSites ? E.Amount : 0;
	}
	Expect(S.BankrollCents == D.BankrollCents && S.History.size() == D.History.size() && S.Life.RentsPaid == D.Life.RentsPaid && S.Life.Inbox.size() == D.Life.Inbox.size(),
		(Tag + "the bank, the results, the rents paid and Mail as saved").c_str());
	Expect(S.WorldStart() == (Blank ? ss::world::Start::Blank : ss::world::Start::Established), (Tag + (Blank ? "a Blank world" : "an Established world")).c_str());
	if (!Multi)
	{
		const ss::sponsor::Contract* Team = TeamRiverLineDeal(S);
		Expect(S.Life.Sites.Accounts.empty() && S.Life.Sites.Transfers.empty() && Fees == 0 && (!Team || Team->T.Only.empty()),
			(Tag + "RiverLine only: no account, nothing in transit, no site fees, no clause on a deal from before it").c_str());
	}
	if (Name == "pre_segment_fresh.ssave" || Name == "pre_update2_fresh.ssave")
	{
		Expect(S.BankrollCents == 237 + ss::hero::PerksOf(S.Person).StartCents && S.Life.RentDueCents == 122500 && S.History.empty(), (Tag + "Night One's $2.37 and its rent").c_str());
	}
	else if (Name == "pre_segment_mid_nightly.ssave" || Name == "update2_auto_mid_nightly.ssave" || Name == "pre_update2_staked_mid_nightly.ssave")
	{
		const life::LiveEntry* E = ss::live::EntryFor(S.Life, "embercrest-nightly@9");
		const bool Plan = Name != "update2_auto_mid_nightly.ssave" || (E && E->CheckpointHost.find("auto\t") != std::string::npos);
		const bool Lou = Name != "pre_update2_staked_mid_nightly.ssave" || (E && E->Fund.Who == "Big Lou");
		Expect(S.LiveInProgress() == "embercrest-nightly@9" && E && E->Checkpoint.rfind("ck1 ", 0) == 0 && Plan && Lou,
			(Tag + "tonight's Nightly to go back to, checkpointed (the plan's host text, Lou's money)").c_str());
	}
	else if (Name == "pre_segment_team_riverline.ssave")
	{
		Expect(S.TeamRiverLine(), (Tag + "Team RiverLine kept, on its old terms").c_str());
	}
	else if (Name == "pre_segment_evicted.ssave" || Name == "pre_update2_evicted.ssave")
	{
		Expect(S.Evicted() && S.Life.StorageDue > 0.0, (Tag + "still locked out, the gear in storage").c_str());
	}
	else if (Name == "pre_segment_v1.ssave")
	{
		bool Ours = !S.Life.LiveEntries.empty();
		for (const life::LiveEntry& E : S.Life.LiveEntries)
		{
			Ours = Ours && E.Id.rfind("embercrest", 0) == 0;
		}
		Expect(Ours, (Tag + "the v1 save's nights are the Embercrest's").c_str());
	}
	else if (Name == "pre_update2_sponsored_streamer.ssave")
	{
		Expect(ss::sponsor::Active(S.Life.Sponsors, "voltrush", S.WorldMinutes()) && S.Channel.Followers >= 2400, (Tag + "Volt Rush signed, the Kast channel").c_str());
	}
	else if (Name == "pre_update2_veteran.ssave")
	{
		Expect(S.History.size() == 100 && S.HeroBadges().Find(ss::badge::Kind::VerifiedPro), (Tag + "the veteran's hundred results and Verified").c_str());
	}
	else if (Blank)
	{
		int Team = 0;
		for (const ss::world::Npc& N : S.Living().People())
		{
			Team += N.Pro || N.Sponsor.rfind("Team ", 0) == 0 ? 1 : 0; // (Team RiverLine's pros, and a site team's: "Team RunItTwice")
		}
		Expect(Team == 0 && S.DebugWorld("").find("World: Fresh start, version 4") == 0, (Tag + "a fresh world at version 4, no team signed yet").c_str());
	}
	else if (Name == "update2_creator.ssave")
	{
		Expect(S.Life.Video.Videos.size() == 60 && S.Life.Video.PartnerAt >= 0.0 && S.Life.Video.BalanceCents > 0, (Tag + "Rerun's channel: 60 videos, Partner, a balance").c_str());
		// (Gate 2 wrote it, without its world lines: the world built on load has no tracker of the player's events, so no site's
		// gate is met here; Update2Journey and EconomyPacing's multi-site career walk Ignite's letter in through the calendar.)
		Expect(S.Life.Sites.Invites.empty(), (Tag + "no site's letter yet (no site lines in a gate 2 save)").c_str());
	}
	else if (Multi)
	{
		const site::Account* Ign = site::AccountOf(S.Life.Sites, net::SiteId::Ignite);
		const site::Account* Rit = site::AccountOf(S.Life.Sites, net::SiteId::RunItTwice);
		int Pending = 0;
		for (const site::Transfer& X : S.Life.Sites.Transfers)
		{
			Pending += X.State == 0 && X.Out && X.ArrivesAt > S.WorldMinutes() ? 1 : 0;
		}
		const ss::sponsor::Contract* Team = TeamRiverLineDeal(S);
		bool Rivals = false;
		for (const ss::HistoryEntry& E : S.History)
		{
			Rivals = Rivals || net::SiteOf(E.EventId) != net::SiteId::RiverLine;
		}
		Expect(Ign && Rit && Ign->Deposited > 0 && Rit->Deposited > 0 && WalletAddsUp(*Ign) && WalletAddsUp(*Rit) && Pending == 1 && Fees < 0,
			(Tag + "accounts on Ignite and RunItTwice, each wallet its lines' sum, a withdrawal in transit, the card fees in the bank's ledger").c_str());
		Expect(Team && Team->St == ss::sponsor::State::Active && Team->T.Only == "riverline" && Team->Strikes == 1 && Rivals &&
				   S.Life.TicketsFor("runittwice.sunday-grind") == 1 && S.Life.TicketsFor("runittwice.supper-stack") == 1,
			(Tag + "Team RiverLine under the clause with one strike, the rival results, RunItTwice's tickets left").c_str());
	}
}
} // namespace migration2

/**
 * MigrationAll2 (update 2 SPEC §6.5, §6.7, §7.5 C3): MigrationAll's chain (the day-0 fixtures, world_v2.txt) and then every
 * committed save fixture, the last segment's, update 2's day-0 and every gate's (update2_blank_world, update2_auto_mid_nightly,
 * update2_creator, update2_multisite):
 *   - it parses and writes back (byte for byte; the v1 save as what loading it gives; the Kast deals' save but for the
 *     segment's own lines, which its migration writes), and a second parse writes the same;
 *   - it loads into a session whose world is version 4 (a save without world lines builds one; update2_blank_world's
 *     world migrates), and that session's save reads and writes back byte for byte;
 *   - that save loads into a second session that saves it again identically (every migration ran once, on the first load);
 *   - and it keeps its facts (migration2::Facts): money, results, rents, Mail, the world it began in, and what makes it
 *     that fixture; an old save stays a RiverLine-only career.
 */
void MigrationAll2()
{
	const auto T0 = std::chrono::steady_clock::now();
	MigrationAll();
	struct Fx
	{
		const char* Name;
		const char* Expected;
	};
	const Fx All[] = {{"pre_segment_fresh.ssave", nullptr}, {"pre_segment_mid_nightly.ssave", nullptr}, {"pre_segment_team_riverline.ssave", nullptr},
		{"pre_segment_evicted.ssave", nullptr}, {"pre_segment_v1.ssave", "pre_segment_v1.expected.ssave"}, {"pre_update2_fresh.ssave", nullptr},
		{"pre_update2_staked_mid_nightly.ssave", nullptr}, {"pre_update2_sponsored_streamer.ssave", nullptr}, {"pre_update2_evicted.ssave", nullptr},
		{"pre_update2_veteran.ssave", nullptr}, {"update2_blank_world.ssave", nullptr}, {"update2_auto_mid_nightly.ssave", nullptr}, {"update2_creator.ssave", nullptr},
		{"update2_multisite.ssave", nullptr}};
	for (const Fx& F : All)
	{
		const auto F0 = std::chrono::steady_clock::now();
		const std::string Saved = seams::Fixture(F.Name);
		Expect(Saved.rfind(F.Expected ? "shortstack.nightone.v1\n" : "shortstack.nightone.v2\n", 0) == 0, (std::string("migration (all): ") + F.Name + " is there").c_str());
		seams::RoundTrip(F.Name, Saved, F.Expected ? seams::Fixture(F.Expected) : Saved);
		ss::SaveData D;
		if (!ss::SaveData::Parse(Saved, D))
		{
			continue;
		}
		const int From = migration2::WorldVersionIn(Saved);
		const std::string Seed = std::string("migration2-") + F.Name;
		std::string First;
		{
			Hooks H;
			ss::Session S(H, Seed, &D);
			First = migration2::SavedNow(S, H);
			migration2::Facts(F.Name, S, D);
		}
		ss::SaveData Back;
		const bool Reads = ss::SaveData::Parse(First, Back) && Back.Serialize() == First;
		std::string Second;
		{
			Hooks H;
			ss::Session S(H, Seed, &Back);
			Second = migration2::SavedNow(S, H);
		}
		const int To = migration2::WorldVersionIn(First);
		std::printf("  migration (all): %s: world %s -> version %d; its session's save %zu bytes, %s (%.2f s)\n", F.Name, From > 0 ? ("version " + std::to_string(From)).c_str() : "none (built)",
			To, First.size(), Second == First ? "the same from a second load" : ("a second load differs at " + migration2::FirstDifference(First, Second)).c_str(),
			migration2::Since(F0));
		Expect(To == ss::world::Version && To == 4, (std::string("migration (all): ") + F.Name + " loads into a world at version 4").c_str());
		Expect(Reads, (std::string("migration (all): ") + F.Name + ": the session's save reads and writes back byte for byte").c_str());
		Expect(Second == First, (std::string("migration (all): ") + F.Name + " loads a second time identically (every migration ran once)").c_str());
	}
	// A fresh-start save as gate 1 wrote its world (version 3): update2_blank_world's world taken back to version 3's lines
	// (written by gate 3 now, it's version 4). It migrates to version 4 once, on the first load, and stays a Blank world.
	{
		const std::string Saved = seams::Fixture("update2_blank_world.ssave");
		const std::string Old = migration2::WorldVersionIn(Saved) == 4 ? migration2::AsWorldV3(Saved) : Saved;
		ss::SaveData D;
		const bool Parsed = migration2::WorldVersionIn(Old) == 3 && ss::SaveData::Parse(Old, D);
		std::string First, Second;
		int Members = 0; // RunItTwice's accounts after the migration (its lobby has been open since Night One: §5.9)
		if (Parsed)
		{
			Hooks H;
			ss::Session S(H, "migration2-blank-v3", &D);
			First = migration2::SavedNow(S, H);
			migration2::Facts("update2_blank_world.ssave", S, D);
			for (const ss::world::Npc& N : S.Living().People())
			{
				Members += (N.Sites & (1u << static_cast<unsigned>(ss::net::SiteId::RunItTwice))) != 0 ? 1 : 0;
			}
		}
		ss::SaveData Back;
		if (Parsed && ss::SaveData::Parse(First, Back))
		{
			Hooks H;
			ss::Session S(H, "migration2-blank-v3", &Back);
			Second = migration2::SavedNow(S, H);
		}
		std::printf("  migration (all): update2_blank_world.ssave with its world at version 3: -> version %d, %d RunItTwice accounts, %s\n", migration2::WorldVersionIn(First),
			Members, Second == First ? "the same from a second load" : ("a second load differs at " + migration2::FirstDifference(First, Second)).c_str());
		Expect(Parsed && migration2::WorldVersionIn(First) == 4 && !First.empty() && Second == First,
			"migration (all): a fresh-start save with a version 3 world migrates to version 4 once, and loads a second time identically");
		// (The step itself, not only the head's number: the version 3 world gets the rivals' memberships as of its day, U10.)
		Expect(Members > 0, "migration (all): the version 3 world's migration seeds the rivals' memberships (RunItTwice's accounts)");
	}
	std::printf("  migration (all): %.1f s\n", migration2::Since(T0));
}
} // namespace session_test

// ------------------------------------------------------------------ update 2's C1 journeys (update 2 SPEC §7.5 C1, §6.7)

namespace session_test
{
namespace journeys
{
namespace autoplay = ss::autoplay;
namespace life = ss::life;
namespace live = ss::live;
namespace style = ss::style;
const double Day = 1440.0;

double SecondsSince(const std::chrono::steady_clock::time_point& T0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
}

autoplay::TableAuto PlanOf(style::Style S, autoplay::Mode M, int Speed, autoplay::Target To, uint32_t Stops)
{
	autoplay::TableAuto A;
	A.S = S;
	A.M = M;
	A.Speed = Speed;
	A.To = To;
	A.Stops = Stops;
	return A;
}

ss::hero::Character Jesse()
{
	ss::hero::Character Who;
	Who.FirstName = "Jesse";
	Who.LastName = "Cole";
	Who.Story = ss::hero::Background::DealersKid;
	Who.Created = true;
	return Who;
}

/** The open table playing EventId (tables close as they finish, so an index moves), or -1. */
int TableOf(ss::Session& S, const std::string& EventId)
{
	for (int I = 0; I < S.TableCount(); ++I)
	{
		if (S.Glance(I).EventId == EventId)
		{
			return I;
		}
	}
	return -1;
}

/** The player's turn at table I, if it's open: the grader's best option (DeepRuns' hero), through the tile's buttons. */
bool BestAt(ss::Session& S, int I, double Now)
{
	const ss::TableGlance G = S.Glance(I);
	if (!G.YourTurn || Now - G.TurnOpenedAt < 0.3)
	{
		return false;
	}
	ss::PlayerAction A = ss::PlayerAction::Fold();
	S.WithTable(I, [&]() {
		const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
		size_t Best = 0;
		for (size_t K = 1; K < Opts.size(); ++K)
		{
			Best = Opts[K].Ev > Opts[Best].Ev ? K : Best;
		}
		A = Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action;
	});
	S.HeroActAt(I, A);
	return true;
}

/** Every decision the coach has tallied (Life.Study): only the player's own decisions ever reach it (U4). */
int Coached(const ss::Session& S)
{
	int N = 0;
	for (const ss::coach::Tally& T : S.Life.Study.All)
	{
		N += T.Decisions;
	}
	return N;
}

/** The learned read book as the card room seats a night with it (BackRoomAutoRules::AutoLearnedOf). */
style::Learned LearnedOf(const std::map<std::string, int>& Reads)
{
	style::Learned Out;
	for (const auto& Kv : Reads)
	{
		const size_t Slash = Kv.first.find('/');
		if (Kv.second >= 2 && Slash != std::string::npos && Slash > 0)
		{
			++Out.Tells[Kv.first.substr(0, Slash)];
		}
	}
	return Out;
}

/** The plan's run as text: the auto line and an autoseen line a player (what the host text keeps of it). */
std::string RunState(const autoplay::TableAuto& A, const autoplay::Run& R)
{
	std::string Out = autoplay::RunText(A, R);
	for (const auto& Kv : R.Seen)
	{
		Out += "\nautoseen\t" + Kv.first + "\t" + ss::reads::Write(Kv.second);
	}
	return Out;
}

/**
 * A night the plan skips through live::SkipSlice, on the card room host's side (FAutoHost::TickSkip's sink, as the
 * fixture_tool's): each hand into the run (NoteSkippedHand) and the brain read afresh, the tablemates kept, every
 * checkpoint's state (the engine's and the host text's) noted. HostText is what LiveHostText writes.
 */
struct PlanNight : ss::live::SkipSink
{
	ss::Tournament* T = nullptr;
	autoplay::TableAuto A;
	autoplay::Run R;
	style::Learned Book;
	const ss::reads::Table* Notes = nullptr;
	double TiltGain = 1.0;
	style::HeroBrain Brain;
	std::vector<std::string> Met;
	int Hands = 0;
	int LastCheckpoint = -1;
	std::vector<int> CheckpointTicks;

	void Think() { Brain = autoplay::BrainFor(A, R, TiltGain, 1, Notes, &Book); }
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		autoplay::NoteSkippedHand(R, H, HeroSeat, TiltGain);
		Think();
		++Hands;
		for (size_t K = 0; K < H.Seats.size(); ++K)
		{
			const std::string& Id = H.Seats[K].Id;
			if (static_cast<int>(K) != HeroSeat && !Id.empty() && std::find(Met.begin(), Met.end(), Id) == Met.end())
			{
				Met.push_back(Id);
			}
		}
	}
	void Events(const std::vector<ss::TEvent>&) override {}
	void Checkpoint(int Tick) override
	{
		LastCheckpoint = Tick;
		CheckpointTicks.push_back(Tick);
	}
	std::string HostText() const
	{
		std::string Out = "hands\t" + std::to_string(Hands) + "\n";
		for (const std::string& Id : Met)
		{
			Out += "met\t" + Id + "\n";
		}
		Out += autoplay::RunText(A, R) + "\n";
		for (const auto& Kv : R.Seen)
		{
			Out += "autoseen\t" + Kv.first + "\t" + ss::reads::Write(Kv.second) + "\n";
		}
		for (const auto& Kv : Book.Tells)
		{
			Out += "autoread\t" + Kv.first + "\t" + std::to_string(Kv.second) + "\n";
		}
		return Out;
	}
	/** Skips to the plan's target or a stop (SkipSlice's reason), or, with UntilCheckpoint, to the first checkpoint at
	 *  or past that hand. */
	ss::live::SkipWhy Skip(int UntilCheckpoint = -1)
	{
		ss::live::SkipWhy Why = ss::live::SkipWhy::Budget;
		for (int Guard = 0; Guard < 20000 && !T->Hero().Busted && !T->bFinished; ++Guard)
		{
			Why = ss::live::SkipSlice(*T, Brain, A, 0.0, *this).Why;
			if (Why != ss::live::SkipWhy::Budget || (UntilCheckpoint >= 0 && LastCheckpoint >= UntilCheckpoint))
			{
				break;
			}
		}
		return Why;
	}
};

/** The plan takes the seat as the night begins (FAutoHost::StartRun): Sharpness read once from Form and condition. */
void TakeSeat(PlanNight& N, life::State& L, double World)
{
	const double Form = autoplay::FormAt(L.Auto, World);
	N.R.Sharp = autoplay::SharpnessOf(Form, 0.0, false, false);
	N.R.SharpSetAt = World;
	N.R.Tilt = 0.0;
	N.TiltGain = std::min(2.5, std::max(0.3, L.Perks.TiltGain));
	N.Book = LearnedOf(L.Reads);
	N.Notes = &L.Auto.Notes;
	L.Update2LinesRead |= life::U2AutoPref;
	N.Think();
}

/** The checkpoint host text read back the way FAutoHost::ReadHostLine does: the plan and its run, the stats, the book. */
bool ReadPlanLines(const std::string& Host, autoplay::TableAuto& A, autoplay::Run& R, style::Learned& Book, int& Hands)
{
	bool Ok = false;
	size_t At = 0;
	while (At < Host.size())
	{
		size_t End = Host.find('\n', At);
		End = End == std::string::npos ? Host.size() : End;
		const std::string Line = Host.substr(At, End - At);
		At = End + 1;
		if (Line.rfind("auto\t", 0) == 0)
		{
			Ok = autoplay::ReadRun(Line, A, R);
		}
		else if (Line.rfind("autoseen\t", 0) == 0)
		{
			const size_t Tab = Line.find('\t', 9);
			ss::reads::Stats St;
			if (Tab == std::string::npos || !ss::reads::Parse(Line.substr(Tab + 1), St))
			{
				return false;
			}
			R.Seen[Line.substr(9, Tab - 9)] = St;
		}
		else if (Line.rfind("autoread\t", 0) == 0)
		{
			const size_t Tab = Line.find('\t', 9);
			Book.Tells[Line.substr(9, Tab - 9)] = Tab == std::string::npos ? 0 : std::atoi(Line.c_str() + Tab + 1);
		}
		else if (Line.rfind("hands\t", 0) == 0)
		{
			Hands = std::atoi(Line.c_str() + 6);
		}
	}
	return Ok;
}

/** The roster as the engine's reserved players. */
std::vector<ss::ReservedPlayer> KnownOf(const life::LiveEntry& E)
{
	std::vector<ss::ReservedPlayer> Out;
	for (const std::pair<std::string, int>& R : E.Roster)
	{
		ss::ReservedPlayer P;
		P.Name = R.first;
		P.Type = static_cast<ss::Archetype>(R.second);
		Out.push_back(P);
	}
	return Out;
}

/** The save the host has while the player is out (the session's last save, the life and bankroll as they stand). */
ss::SaveData HostSave(const Hooks& H, const ss::Session& S)
{
	ss::SaveData D = H.Last;
	D.Life = S.Life;
	D.BankrollCents = S.BankrollCents;
	if (D.WorldText.empty() && D.WorldSnapshot)
	{
		D.WorldSnapshot->Write(D.WorldText);
	}
	return D;
}

/**
 * The career's books as SegmentJourney audits them after every step (the last segment's monkey invariants, carried:
 * next_segment_spec §7.6): the bankroll moves only through the ledger (money the test hands the career is counted in
 * Injected), it never goes negative, no backing or sponsor payment is written twice, at most one brand holds an exclusive
 * category, the stream's overlay holds at most two deals, every patch sewn is a running contract's, sponsor property never
 * sells at the pawn, and an unanswered offer is never dropped from Mail.
 */
struct Books
{
	ss::Chips Bank = 0;
	ss::Chips Injected = 0;
	std::vector<life::LedgerEntry> Seen; // the ledger as it stood at the last audit
	std::vector<life::LedgerEntry> All;  // every line once, oldest first
	int Steps = 0;
	int MostLines = 0;
	std::map<std::string, int> Broken; // invariant -> times broken
	std::string First;

	void Break(const std::string& What, const std::string& Where)
	{
		if (Broken[What]++ == 0 && First.empty())
		{
			First = What + " (" + Where + ")";
		}
	}
	static bool Same(const life::LedgerEntry& A, const life::LedgerEntry& B) { return A.At == B.At && A.Label == B.Label && A.Amount == B.Amount && A.Kind == B.Kind; }
	void Start(const ss::Session& S)
	{
		Bank = S.BankrollCents;
		Seen = S.Life.Ledger;
		for (size_t K = Seen.size(); K-- > 0;)
		{
			All.push_back(Seen[K]);
		}
	}
	void Give(ss::Session& S, ss::Chips Cents)
	{
		S.BankrollCents += Cents;
		Injected += Cents;
	}
	void Audit(ss::Session& S, const std::string& Where)
	{
		++Steps;
		// New lines: the shortest prefix after which the ledger goes on as it stood (it keeps the newest 60).
		const std::vector<life::LedgerEntry>& Cur = S.Life.Ledger;
		size_t Fresh = Cur.size();
		if (Cur.size() < 60 && Cur.size() >= Seen.size())
		{
			Fresh = Cur.size() - Seen.size();
		}
		for (size_t N = 0; Cur.size() >= 60 && N < Cur.size() && !Seen.empty(); ++N)
		{
			bool Match = true;
			for (size_t I = 0; N + I < Cur.size() && I < Seen.size() && Match; ++I)
			{
				Match = Same(Cur[N + I], Seen[I]);
			}
			if (Match)
			{
				Fresh = N;
				break;
			}
		}
		ss::Chips Logged = 0;
		for (size_t K = Fresh; K-- > 0;)
		{
			const life::LedgerEntry& E = Cur[K];
			Logged += E.Amount;
			if (E.Kind == life::LedgerBacking || E.Kind == life::LedgerSponsors)
			{
				for (const life::LedgerEntry& Old : All)
				{
					if (Same(Old, E))
					{
						Break("a backing or sponsor payment written twice: " + E.Label, Where);
					}
				}
			}
			All.push_back(E);
		}
		MostLines = std::max(MostLines, static_cast<int>(Fresh));
		if (S.BankrollCents - Bank != Logged + Injected)
		{
			Break("the bankroll moved without the ledger (" + ss::Money(S.BankrollCents - Bank) + " vs " + ss::Money(Logged + Injected) + ")", Where);
		}
		if (S.BankrollCents < 0)
		{
			Break("a negative bankroll", Where);
		}
		Bank = S.BankrollCents;
		Injected = 0;
		Seen = Cur;
		// The sponsor book.
		const double World = S.WorldMinutes();
		const ss::sponsor::Book& B = S.Life.Sponsors;
		std::map<int, int> ByCategory;
		for (const ss::sponsor::Brand& Br : ss::sponsor::Brands())
		{
			if (ss::sponsor::ExclusiveCategory(Br.Cat) && ss::sponsor::Active(B, Br.Id, World))
			{
				++ByCategory[static_cast<int>(Br.Cat)];
			}
		}
		for (const auto& It : ByCategory)
		{
			if (It.second > 1)
			{
				Break("two brands active in one exclusive category", Where);
			}
		}
		const std::vector<ss::kast::Deal> OnStream = ss::sponsor::StreamView(B, World);
		if (std::count_if(OnStream.begin(), OnStream.end(), [](const ss::kast::Deal& D) { return D.Overlay; }) > ss::kast::MaxDeals)
		{
			Break("more than two deals on the stream's overlay", Where);
		}
		for (const std::string& Patch : S.Life.Closet.Patches)
		{
			if (!Patch.empty() && !ss::sponsor::Running(B, Patch, World))
			{
				Break("a patch sewn for a brand with no running contract: " + Patch, Where);
			}
		}
		for (const auto& Owned : S.Gear)
		{
			if (ss::sponsor::IsProperty(B, Owned.first, World) && S.PawnSell("gear:" + Owned.first).empty())
			{
				Break("sponsor property sold at the pawn: " + Owned.first, Where);
			}
		}
		for (const ss::sponsor::Contract& C : B.Contracts)
		{
			if (C.St == ss::sponsor::State::Offered && C.ExpiresAt > World)
			{
				const bool Mailed = std::any_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [&C](const life::Letter& L) { return L.Thread == C.Id; });
				if (!Mailed)
				{
					Break("an open offer dropped from Mail: " + C.Id, Where);
				}
			}
		}
	}
};

/** The bytes a save's lines of the given tags take (the segment's ≤ 80 KB, update 2's non-world ≤ 60 KB). */
size_t LineBytes(const std::string& Text, std::initializer_list<const char*> Tags)
{
	size_t Bytes = 0;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		for (const char* Tag : Tags)
		{
			if (Text.compare(Start, std::strlen(Tag), Tag) == 0)
			{
				Bytes += End - Start;
				break;
			}
		}
		Start = End;
	}
	return Bytes;
}

// Update 3 (update 3 SPEC §0.6 #10b, §1.8, §7.4): Update 3's lines live inside site\t and video\t (new sub-tags, and fields
// appended to video\tvid after its title and to video\tcand after its recall record), so Update 2's budget counts Update 2's
// part of those lines only (LineBytesU2) and Update 3's 27 KB counts the rest (LineBytesU3).

/** Update 3's sub-tags inside site\t and video\t (§7.4). */
bool U3SubTag(const std::string& Text, size_t Start)
{
	static const char* const Tags[] = {"site\tspark\t", "site\tflame\t", "site\tdare\t", "site\tbox\t", "site\tband\t", "site\tlevel\t", "site\tlook\t", "site\tfuse\t",
		"site\tbeef\t", "site\tstick\t", "site\tstar\t", "site\tleague\t", "site\tlhist\t", "site\tcallout\t", "site\th2h\t", "site\tscalp\t", "site\tluck\t", "site\tcred\t",
		"site\tgov\t", "site\thold\t", "video\tvcomp\t", "video\tvday\t", "video\tvmonth\t", "video\tvtop\t", "video\tvlife\t", "video\tvtailx\t", "video\tvmom\t",
		"video\tvlook\t", "video\tvlist\t", "video\tvchal\t", "video\tvidea\t", "video\tvtrend\t", "video\tvsnap\t", "video\tvstubd\t"};
	for (const char* Tag : Tags)
	{
		if (Text.compare(Start, std::strlen(Tag), Tag) == 0)
		{
			return true;
		}
	}
	return false;
}

/** Of the line at [Start, End): the bytes Update 2's fields take (a vid's first 29 fields, a cand's first 27; any other line whole). */
size_t U2PartOf(const std::string& Text, size_t Start, size_t End)
{
	const int Keep = Text.compare(Start, 10, "video\tvid\t") == 0 ? 29 : Text.compare(Start, 11, "video\tcand\t") == 0 ? 27 : 0;
	if (Keep == 0)
	{
		return End - Start;
	}
	int Fields = 1;
	for (size_t I = Start; I < End; ++I)
	{
		if (Text[I] == '\t' && ++Fields > Keep)
		{
			return I - Start + 1; // up to Update 2's last field, and the line's end
		}
	}
	return End - Start;
}

/** The bytes Update 2's lines take (autoplay\t, video\t and site\t without Update 3's sub-tags and appended fields). */
size_t LineBytesU2(const std::string& Text)
{
	size_t Bytes = 0;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		const bool Ours = Text.compare(Start, 9, "autoplay\t") == 0 || Text.compare(Start, 6, "video\t") == 0 || Text.compare(Start, 5, "site\t") == 0;
		if (Ours && !U3SubTag(Text, Start))
		{
			Bytes += U2PartOf(Text, Start, End);
		}
		Start = End;
	}
	return Bytes;
}

/** The bytes Update 3's lines take (its sub-tags, and the fields appended to vid and cand): its own budget, world lines aside. */
size_t LineBytesU3(const std::string& Text)
{
	size_t Bytes = 0;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		if (U3SubTag(Text, Start))
		{
			Bytes += End - Start;
		}
		else if (Text.compare(Start, 6, "video\t") == 0)
		{
			Bytes += End - Start - U2PartOf(Text, Start, End);
		}
		Start = End;
	}
	return Bytes;
}

/**
 * SegmentJourney's deals: every stake offered is taken (EconomyPacing's TakeDeals), but of the sponsors only the
 * journey's own (Lucky Penny, Volt Rush) are signed; any other brand's offer is declined, so a natural offer can't take
 * the spot a scripted patch needs.
 */
void SegDeals(ss::Session& S, pacing::Career& C)
{
	const double World = S.WorldMinutes();
	for (const ss::stake::Offer& O : std::vector<ss::stake::Offer>(S.Life.Stakes.Offers))
	{
		if (O.State == ss::stake::OfferStanding && O.Expires >= World && S.AcceptStake(O.Id).empty() && O.Backer == "Big Lou")
		{
			C.LouAt.push_back(World);
		}
	}
	for (const ss::sponsor::Brand& B : ss::sponsor::Brands())
	{
		const std::string Id = ss::sponsor::OpenOfferFor(S.Life.Sponsors, B.Id, World);
		if (Id.empty())
		{
			continue;
		}
		if (B.Id == "luckypenny" || B.Id == "voltrush")
		{
			ss::sponsor::HeardOffer(S.Life.Sponsors, S.Life, Id, World);
			C.Signed += S.SignDeal(Id).empty() ? 1 : 0;
		}
		else if (B.Id != "overclock")
		{
			S.DeclineDeal(Id);
		}
	}
	C.Ledger.Take(S.Life);
}

/**
 * A night at the Embercrest on the host's side (EconomyPacing's LiveNight, any event key): the save taken as it stands,
 * the desk (whoever pays: the player, a backer, a sponsor's cover, pieces sold), the engine plays the night (a TAG), the
 * cage settles it on the save, and the save loads again. Fills Funded with who paid; false when it didn't happen. With
 * Finish, the night ends in that place five hours in (the fixtures' "won" night: a deep run the test wants paid out).
 */
bool SegNight(std::unique_ptr<ss::Session>& S, pacing::PaceHooks& H, pacing::Career& C, double& Now, int DayNo, const char* Key, life::Funding* Funded = nullptr,
	int* PlaceOut = nullptr, int Finish = 0)
{
	live::Occurrence O;
	for (const live::Occurrence& X : live::Occurrences(DayNo))
	{
		O = std::string(X.T->Key) == Key ? X : O;
	}
	if (!O.Valid() || S->WorldMinutes() > O.Start - 45.0)
	{
		return false; // (the bus wouldn't make the first hand)
	}
	Now = pacing::Hop(*S, C, Now, std::max(S->WorldMinutes(), O.DeskOpens + 30.0));
	pacing::Fed(*S);
	SegDeals(*S, C);
	const std::string Refused = S->GoToLive(O.Id);
	if (!Refused.empty())
	{
		std::printf("    (the desk at %s: %s)\n", O.Id.c_str(), Refused.c_str());
		return false;
	}
	++C.Live;
	S->Save();
	ss::SaveData D = HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	if (!E)
	{
		return false;
	}
	if (Funded)
	{
		*Funded = E->Fund;
	}
	E->ArrivedAt = O.Start - 15.0;
	bool Restored = false;
	std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, KnownOf(*E), false, Restored);
	const ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
	for (int Guard = 0; Finish <= 0 && Guard < 6000 && !T->Hero().Busted && !T->bFinished; ++Guard)
	{
		T->SimulateTick(&Auto);
	}
	const int Place = Finish > 0 ? Finish : T->Hero().Place > 0 ? T->Hero().Place : T->HeroRank();
	const ss::Chips Prize = T->PrizeFor(Place);
	const double End = Finish > 0 ? O.Start + 300.0 : O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
	live::Settle(D.BankrollCents, D.Life, O.Id, Place, T->Spec.Entrants, Prize, End);
	live::PayFare(D.BankrollCents, D.Life, O.Id, true, End + 5.0);
	D.NoteLive(End, O.Id, Place, Prize, T->Spec.Entrants, {});
	D.ClockMinutes = End + 30.0 - Day;
	if (PlaceOut)
	{
		*PlaceOut = Place;
	}
	S.reset();
	S.reset(new ss::Session(H, C.Seed, &D));
	S->CurrentScreen = ss::Screen::Lobby;
	S->ResumeCalendarFrom(End + 30.0);
	Now = Wait(*S, Now, 0.1);
	C.Ledger.Take(S->Life);
	return true;
}

/** Whole-word, case-blind: does Text say Word? (U1: nothing in the world says autoplay played.) */
bool Says(const std::string& Text, const std::string& Word)
{
	std::string Low;
	for (const char Ch : Text)
	{
		Low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(Ch))));
	}
	for (size_t P = Low.find(Word); P != std::string::npos; P = Low.find(Word, P + 1))
	{
		const bool Left = P == 0 || !std::isalnum(static_cast<unsigned char>(Low[P - 1]));
		const bool Right = P + Word.size() >= Low.size() || !std::isalnum(static_cast<unsigned char>(Low[P + Word.size()]));
		if (Left && Right)
		{
			return true;
		}
	}
	return false;
}
} // namespace journeys

/**
 * G1Journey (update 2 SPEC §7.5 C1, §6.7): one career through gate 1's user-visible guarantees, end to end in core.
 *   1. A fresh start (§3.1): no team, nobody on Season or Earnings at 2:07 AM; the panel's default (Autopilot, Form 70).
 *   2. Online autoplay on three tables (§2.6.1, §8.1 step 3): Push/Fold skipped to the bubble stops there and the plan holds
 *      the seat at 1x; with "My all-in decisions" on, the skip hands a decision over mid-hand (AdoptHand); GTO watched at
 *      2x is taken over mid-think (the prompt opens, nothing acts; graded, but Form doesn't move), then the resume window;
 *      TAG skipped whole leaves a played line with no decision of the player's (Career's AUTO tag and the "—"). The coach
 *      tallies exactly the player's own decisions (U4); ss.Auto.Parity on another table: equal hashes.
 *   3. A save and a reload: the Blank world, Form, the played lines and the notes come back.
 *   4. The Embercrest (§2.12, §8.1 step 4): the Nightly the plan skips through live::SkipSlice, checkpointed every 5 hands
 *      (R5); the game "killed" after a checkpoint and loaded from the save carries on from it exactly as the night that
 *      never stopped (the engine checkpoint, the tilt, Sharpness, the counts and the stats); the skip stops at the bubble
 *      between hands; the night settled, its played line on the record.
 *   5. The world hears the plan's events as the player's (§2.9), and never that a plan played them (U1); the fresh world's
 *      boards fill.
 */
void G1Journey()
{
	namespace autoplay = ss::autoplay;
	namespace style = ss::style;
	namespace life = ss::life;
	namespace live = ss::live;
	using journeys::Day;
	const auto T0 = std::chrono::steady_clock::now();
	pacing::PaceHooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "g1-journey"));
	S->NewCareer(journeys::Jesse(), ss::world::Start::Blank);
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.1);
	pacing::Fed(*S);
	// 1. Night One in a fresh world.
	{
		ss::net::HeroStats Hero;
		Hero.Name = S->HeroName;
		const std::vector<ss::net::BoardRow> Season = ss::net::Shared().Leaderboard(ss::net::Board::Season, S->WorldMinutes(), Hero, 60, nullptr);
		const std::vector<ss::net::BoardRow> Earnings = ss::net::Shared().Leaderboard(ss::net::Board::Earnings, S->WorldMinutes(), Hero, 60, nullptr);
		int Team = 0;
		for (const ss::world::Npc& N : S->Living().People())
		{
			Team += N.Pro ? 1 : 0;
		}
		Expect(S->WorldStart() == ss::world::Start::Blank && Team == 0 && Season.empty() && Earnings.empty() &&
				   S->DebugWorld("").find("World: Fresh start, version " + std::to_string(ss::world::Version)) == 0,
			"g1 journey: night one in a fresh world: no team, nobody on Season or Earnings at 2:07 AM");
		const autoplay::Panel P = S->AutoPanel(0);
		Expect(!P.On && P.Now.S == style::Style::Autopilot && P.Form == autoplay::FormStart && std::fabs(P.Sharp - 0.80) < 1e-9 && P.Styles.size() == static_cast<size_t>(style::Style::Count),
			"g1 journey: the panel's default is Autopilot, Form 70, Sharpness 0.80");
	}
	// 2. Three tables online (the monitors for them), each with its own plan.
	S->BankrollCents += 14900 + 28900 + 50000;
	Expect(S->Buy("monitor-24").empty() && S->Buy("monitor-27").empty() && S->MaxTables() == ss::Session::TableLimit, "g1 journey: the screens for four tables");
	// The events: small fields (the bubble comes in reasonable time), no bounties, open now and not played yet.
	std::vector<std::string> Used;
	auto PickEvents = [&](size_t Count) {
		std::vector<ss::LobbyEvent> Out;
		const ss::net::Network& Net = ss::net::Shared();
		const double World = S->WorldMinutes();
		for (const ss::net::EventInstance& E : Net.Window(World - 180.0, World + 90.0))
		{
			const ss::LobbyEvent L = Net.Listing(E, nullptr, S->Unlocks());
			if (L.Joinable && L.Spec.Entrants >= 90 && L.Spec.Entrants <= 400 && L.Spec.BountyCents == 0 && L.BuyInCents <= 1100 && S->CanAfford(L) && !S->IsPlaying(L.Spec.Id) &&
				std::find(Used.begin(), Used.end(), L.Spec.Id) == Used.end() && Out.size() < Count)
			{
				Out.push_back(L);
			}
		}
		return Out;
	};
	std::vector<ss::LobbyEvent> Picks = PickEvents(3);
	Expect(Picks.size() == 3, "g1 journey: three small events open on night one");
	if (Picks.size() < 3)
	{
		return;
	}
	std::string IdA = Picks[0].Spec.Id;       // Nit, skipped to the bubble (another event if it busts before)
	const std::string IdB = Picks[1].Spec.Id; // GTO, watched at 2x
	const std::string IdC = Picks[2].Spec.Id; // Push/Fold, skipped whole
	const autoplay::TableAuto PlanA = journeys::PlanOf(style::Style::Nit, autoplay::Mode::Skip, 1, autoplay::Target::Bubble, autoplay::StopFinalTable | autoplay::StopBubble);
	for (const ss::LobbyEvent& L : Picks)
	{
		Used.push_back(L.Spec.Id);
		S->RegisterEvent(L);
	}
	Expect(S->TableCount() == 3, "g1 journey: three tables open");
	const int CoachedBefore = journeys::Coached(*S);
	S->SprintBudgetMs = 1.0e9; // a skip runs to its stop in one update: the same frames on any machine
	Expect(S->SetAutoplay(journeys::TableOf(*S, IdA), PlanA).empty() &&
			   S->SetAutoplay(journeys::TableOf(*S, IdB), journeys::PlanOf(style::Style::Gto, autoplay::Mode::Watch, 2, autoplay::Target::Result, autoplay::StopFinalTable | autoplay::StopBubble)).empty() &&
			   S->SetAutoplay(journeys::TableOf(*S, IdC), journeys::PlanOf(style::Style::PushFold, autoplay::Mode::Skip, 1, autoplay::Target::Result, 0)).empty(),
		"g1 journey: the Nit skips to the bubble, GTO watches at 2x, Push/Fold skips to the result");
	{
		const autoplay::Panel PB = S->AutoPanel(journeys::TableOf(*S, IdB));
		Expect(PB.On && PB.Status == "AUTO \xC2\xB7 GTO 2x" && std::fabs(PB.Sharp - 0.80) < 1e-9, "g1 journey: GTO's tile line, its Sharpness held at the default");
	}
	int Mine = 0;              // the player's own decisions (each one HeroAct: graded, tallied)
	int Retries = 0;           // events the Nit busted in before the bubble
	bool BubbleStop = false;   // A's skip stopped at the bubble, the plan holding the seat at 1x
	bool AllInOn = false;      // "My all-in decisions" turned on for A, and the skip on to the result
	int HandOffs = 0;          // decisions A's skip handed over mid-hand
	bool HandOffMidHand = false;
	bool TookOver = false;     // K during GTO's think at B
	bool QuietTake = false;    // the take-over acted nothing: the prompt opened on the same history
	bool TakeGraded = false;   // the decision in that hand: graded, a week's decision, Form unmoved
	bool Window = false;       // the next hand at B opened in the resume window
	bool BSkipped = false;
	double FormAtTake = 0.0;
	int WeekAtTake = 0;
	size_t HistoryAtTake = 0;
	int MineAtTake = 0;
	auto AnotherA = [&]() {
		// The Nit busted before the bubble: the test of the bubble's stop goes on at another event.
		const std::vector<ss::LobbyEvent> More = PickEvents(1);
		if (More.empty())
		{
			return;
		}
		++Retries;
		IdA = More[0].Spec.Id;
		Used.push_back(IdA);
		S->RegisterEvent(More[0]);
		S->SetAutoplay(journeys::TableOf(*S, IdA), PlanA);
	};
	for (int F = 0; F < 4000000; ++F)
	{
		if (S->CurrentScreen == ss::Screen::Results || S->TableCount() == 0)
		{
			if (BubbleStop || Retries >= 6)
			{
				break;
			}
			S->LeaveResults();
			AnotherA();
			if (S->TableCount() == 0)
			{
				break;
			}
		}
		Now += 1.0 / 30.0;
		S->Update(Now);
		int A = journeys::TableOf(*S, IdA);
		const int B = journeys::TableOf(*S, IdB);
		if (A < 0 && !BubbleStop && Retries < 6 && S->CurrentScreen == ss::Screen::Table)
		{
			AnotherA();
			A = journeys::TableOf(*S, IdA);
		}
		// A: the bubble stop, then "My all-in decisions" and on to the result; every hand-off answered.
		if (A >= 0 && !S->Glance(A).Busted)
		{
			const autoplay::Panel PA = S->AutoPanel(A);
			if (!BubbleStop && PA.On && !PA.Skipping && !PA.TakenOver)
			{
				BubbleStop = PA.Now.M == autoplay::Mode::Watch && PA.Now.Speed == 1 &&
					(PA.StopReason == autoplay::Text(autoplay::Words::AtBubble) || PA.StopReason == autoplay::Text(autoplay::Words::AtHandForHand));
				S->WithTable(A, [&]() { BubbleStop = BubbleStop && S->T->Remaining <= S->T->PaidPlaces() + 15; });
				autoplay::TableAuto Next = S->Autoplay(A);
				Next.Stops |= autoplay::StopMyAllIn;
				S->SetAutoplay(A, Next);
				AllInOn = S->SkipAhead(A, autoplay::Target::Result).empty();
			}
			else if (AllInOn && S->Glance(A).YourTurn)
			{
				if (PA.TakenOver && PA.StopReason == autoplay::Text(autoplay::Words::HandedOver))
				{
					S->WithTable(A, [&]() { HandOffMidHand = HandOffMidHand || (S->CurHand && !S->CurHand->History.empty()); });
				}
				if (journeys::BestAt(*S, A, Now))
				{
					++Mine;
					++HandOffs;
				}
			}
			else if (AllInOn && PA.On && !PA.Skipping && !S->Glance(A).YourTurn)
			{
				S->SkipAhead(A, autoplay::Target::Result);
			}
		}
		// B: K during the plan's think, the prompt, the answer, the resume window, then skipped to the result.
		if (B >= 0 && !S->Glance(B).Busted)
		{
			const autoplay::Panel PB = S->AutoPanel(B);
			int Tick = 0;
			S->WithTable(B, [&]() { Tick = S->T ? S->T->Tick : 0; });
			if (!TookOver && PB.Thinking && Tick >= 4)
			{
				FormAtTake = S->Life.Auto.Pref.Form;
				WeekAtTake = S->Life.Auto.Pref.WeekDecisions;
				MineAtTake = Mine;
				S->WithTable(B, [&]() { HistoryAtTake = S->CurHand ? S->CurHand->History.size() : 0; });
				TookOver = S->TakeOver(B).empty() && S->AutoPanel(B).TakenOver && !S->AutoPanel(B).Thinking;
			}
			else if (TookOver && !TakeGraded && S->Glance(B).YourTurn)
			{
				S->WithTable(B, [&]() { QuietTake = QuietTake || (S->CurHand && S->CurHand->History.size() == HistoryAtTake); });
				if (journeys::BestAt(*S, B, Now))
				{
					++Mine;
					TakeGraded = S->Life.Auto.Pref.Form == FormAtTake && S->Life.Auto.Pref.WeekDecisions == WeekAtTake + 1 && Mine == MineAtTake + 1;
				}
			}
			else if (TakeGraded && !Window && PB.ResumeWindow)
			{
				Window = PB.On && !PB.TakenOver && PB.ResumeLeft > 0.0;
			}
			else if ((Window || (TakeGraded && Tick > 40)) && !BSkipped && PB.On && !PB.TakenOver && !S->Glance(B).YourTurn)
			{
				BSkipped = S->SetAutoplay(B, journeys::PlanOf(style::Style::Gto, autoplay::Mode::Skip, 1, autoplay::Target::Result, 0)).empty();
			}
			else if (TookOver && S->Glance(B).YourTurn && journeys::BestAt(*S, B, Now))
			{
				++Mine;
			}
		}
		// Anything else that's the player's (a hand taken over runs to its end), answered.
		for (int I = 0; I < S->TableCount(); ++I)
		{
			if (I != A && I != B && journeys::BestAt(*S, I, Now))
			{
				++Mine;
			}
		}
	}
	Expect(S->CurrentScreen == ss::Screen::Results, "g1 journey: the events played to their results");
	Expect(BubbleStop, ("g1 journey: the Nit's skip stopped at the bubble, the plan holding the seat at 1x (after " + std::to_string(Retries) + " events it busted in first)").c_str());
	Expect(TookOver && QuietTake && TakeGraded, "g1 journey: K during GTO's think took the wheel: the prompt opened with nothing acted; the decision graded, Form unmoved");
	Expect(Window, "g1 journey: the next hand opened in the resume window (AUTO RESUMES)");
	std::printf("  g1 journey: online: bubble stop %s (%d retries), %d hand-offs (%s mid-hand), take-over %s, %d decisions of the player's, %.1f s\n", BubbleStop ? "yes" : "no", Retries,
		HandOffs, HandOffMidHand ? "one" : "none", TookOver ? "yes" : "no", Mine, journeys::SecondsSince(T0));
	Expect(HandOffs > 0, "g1 journey: with \"My all-in decisions\" on, the skip handed decisions over (AdoptHand)");
	// The record: three history rows; the played lines; Career's tags; only the player's own decisions coached.
	const autoplay::Played* PA = autoplay::PlayedFor(S->Life.Auto, IdA);
	const autoplay::Played* PB = autoplay::PlayedFor(S->Life.Auto, IdB);
	const autoplay::Played* PC = autoplay::PlayedFor(S->Life.Auto, IdC);
	Expect(PA && PB && PC && S->History.size() >= 3, "g1 journey: a played line for each event the plan played");
	if (PA && PB && PC)
	{
		bool CDash = false;
		for (const ss::HistoryEntry& E : S->History)
		{
			CDash = CDash || (E.EventId == IdC && E.AccuracyPct == 0.0);
		}
		const std::string TagC = autoplay::CareerTag(*PC);
		Expect(PC->S == style::Style::PushFold && PC->OwnDecisions == 0 && PC->AiDecisions > 0 && CDash && TagC.rfind("AUTO \xC2\xB7 ", 0) == 0 && TagC.size() > 5 &&
				   TagC.substr(TagC.size() - 5) == " 100%",
			("g1 journey: the event the player never touched: " + TagC + ", the accuracy \xE2\x80\x94").c_str());
		Expect(PB->S == style::Style::Gto && PB->OwnDecisions >= 1 && PB->AiDecisions > 0 &&
				   autoplay::PlayedLine(*PB).rfind("Played by your plan: " + std::to_string(PB->AiDecisions) + " of " + std::to_string(PB->AiDecisions + PB->OwnDecisions) + " decisions (GTO)", 0) == 0,
			("g1 journey: the results line for the event taken over (" + autoplay::PlayedLine(*PB) + ")").c_str());
		Expect(PA->S == style::Style::Nit && PA->AiDecisions > 0 && PA->OwnDecisions == HandOffs, "g1 journey: the Nit's line counts the hand-offs as the player's");
		Expect(journeys::Coached(*S) - CoachedBefore == Mine && PA->OwnDecisions + PB->OwnDecisions + PC->OwnDecisions == Mine,
			"g1 journey: the coach tallied exactly the player's own decisions, none of the plan's (U4)");
	}
	S->LeaveResults();
	// ss.Auto.Parity on a new table (§8.1 step 3), then skipped to its result.
	{
		const std::vector<ss::LobbyEvent> More = PickEvents(1);
		if (!More.empty())
		{
			S->RegisterEvent(More[0]);
		}
		const int I = More.empty() ? -1 : journeys::TableOf(*S, More[0].Spec.Id);
		Expect(I == 0 && S->SetAutoplay(0, journeys::PlanOf(style::Style::Gto, autoplay::Mode::Watch, 2, autoplay::Target::Result, autoplay::StopFinalTable | autoplay::StopBubble)).empty(),
			"g1 journey: a new table, GTO watching");
		if (I != 0)
		{
			return;
		}
		for (int F = 0; F < 30; ++F)
		{
			Now += 1.0 / 30.0;
			S->Update(Now);
		}
		const std::string Parity = S->DebugAuto("parity 30");
		std::printf("  g1 journey: %s\n", Parity.c_str());
		Expect(Parity.find(": equal.") != std::string::npos, "g1 journey: ss.Auto.Parity 30 on a fresh world's table: equal hashes");
		S->SkipAhead(0, autoplay::Target::Result);
		for (int F = 0; F < 300000 && S->CurrentScreen == ss::Screen::Table; ++F)
		{
			Now += 1.0 / 30.0;
			S->Update(Now);
			if (S->TableCount() > 0 && !S->Glance(0).Busted && S->AutoPanel(0).On && !S->AutoPanel(0).Skipping && !S->Glance(0).YourTurn)
			{
				S->SkipAhead(0, autoplay::Target::Result);
			}
			if (S->TableCount() > 0 && journeys::BestAt(*S, 0, Now))
			{
				++Mine;
			}
		}
		S->LeaveResults();
	}
	// 3. Saved and loaded: the fresh world, Form, the played lines and the notes.
	{
		pacing::Fed(*S);
		S->Save();
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(H.Last.Serialize(), D);
		pacing::PaceHooks H2;
		ss::Session Back(H2, "g1-journey", &D);
		Expect(Parsed && Back.WorldStart() == ss::world::Start::Blank && Back.Living().Began() == ss::world::Start::Blank && std::fabs(Back.Life.Auto.Pref.Form - S->Life.Auto.Pref.Form) < 1e-9 &&
				   Back.Life.Auto.Results.size() == S->Life.Auto.Results.size() && Back.Life.Auto.Notes.size() == S->Life.Auto.Notes.size() && D.Serialize() == H.Last.Serialize(),
			"g1 journey: saved and loaded: the fresh world, Form, the played lines and the notes");
	}
	// 4. The Embercrest: the next Nightly, the plan (GTO) skipping to the bubble.
	live::Occurrence O;
	for (int DayNo = ss::net::DayOf(S->WorldMinutes()); DayNo < ss::net::DayOf(S->WorldMinutes()) + 8 && !O.Valid(); ++DayNo)
	{
		for (const live::Occurrence& X : live::Occurrences(DayNo))
		{
			if (!O.Valid() && std::string(X.T->Key) == "nightly" && X.DeskOpens > S->WorldMinutes() + 30.0)
			{
				O = X;
			}
		}
	}
	S->LobbyMinutes += O.DeskOpens + 30.0 - S->WorldMinutes();
	Now = Wait(*S, Now, 0.1);
	pacing::Fed(*S);
	S->BankrollCents += 20000;
	Expect(O.Valid() && S->GoToLive(O.Id).empty(), "g1 journey: registered for the Nightly");
	ss::SaveData D = journeys::HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	if (!E)
	{
		return;
	}
	E->ArrivedAt = O.Start - 15.0;
	const double DayStart = O.Start - static_cast<double>(O.T->StartMinute);
	const autoplay::TableAuto LivePlan = journeys::PlanOf(style::Style::Gto, autoplay::Mode::Skip, 1, autoplay::Target::Bubble, autoplay::StopFinalTable | autoplay::StopBubble);
	// The night that never stops: to the bubble.
	bool Restored = false;
	std::unique_ptr<ss::Tournament> T1 = live::ResumeField(O, *E, E->Entrants, D.HeroName, journeys::KnownOf(*E), false, Restored);
	journeys::PlanNight Whole;
	Whole.T = T1.get();
	Whole.A = LivePlan;
	life::State L1 = D.Life;
	journeys::TakeSeat(Whole, L1, O.Start);
	const ss::live::SkipWhy WholeWhy = Whole.Skip();
	// The same night, the game killed just after its checkpoint at hand 15, the night's save written by the host as the room does.
	std::unique_ptr<ss::Tournament> T2 = live::ResumeField(O, *E, E->Entrants, D.HeroName, journeys::KnownOf(*E), false, Restored);
	journeys::PlanNight First;
	First.T = T2.get();
	First.A = LivePlan;
	life::State L2 = D.Life;
	journeys::TakeSeat(First, L2, O.Start);
	First.Skip(15);
	ss::SaveData Killed = D;
	Killed.Life = L2;
	life::LiveEntry* KE = live::EntryFor(Killed.Life, O.Id);
	KE->Checkpoint = T2->Checkpoint();
	KE->CheckpointAt = DayStart + T2->ClockMinutes();
	KE->CheckpointHost = First.HostText();
	Killed.ClockMinutes = KE->CheckpointAt - Day;
	const std::string AtKill = journeys::RunState(First.A, First.R);
	// Relaunched: the save loaded into a new session, the night's checkpoint and host text read back.
	ss::SaveData Relaunch;
	const bool ReParsed = ss::SaveData::Parse(Killed.Serialize(), Relaunch);
	pacing::PaceHooks H3;
	S.reset(new ss::Session(H3, "g1-journey", &Relaunch));
	S->CurrentScreen = ss::Screen::Lobby;
	S->Save(); // (the host holds the career's save: what it says now)
	const life::LiveEntry* RE = live::EntryFor(S->Life, O.Id);
	Expect(ReParsed && S->LiveInProgress() == O.Id && RE && RE->Checkpoint == KE->Checkpoint && RE->CheckpointHost == KE->CheckpointHost,
		"g1 journey: relaunched, the game goes back to the Nightly at its last checkpoint");
	journeys::PlanNight Second;
	std::unique_ptr<ss::Tournament> T3 = RE ? live::ResumeField(O, *RE, RE->Entrants, S->HeroName, journeys::KnownOf(*RE), true, Restored) : nullptr;
	int HostHands = -1;
	const bool Read = RE && journeys::ReadPlanLines(RE->CheckpointHost, Second.A, Second.R, Second.Book, HostHands);
	Expect(T3 && Restored && T3->Checkpoint() == KE->Checkpoint && Read && journeys::RunState(Second.A, Second.R) == AtKill && Second.Book.Tells == First.Book.Tells &&
			   HostHands == First.Hands && autoplay::WasPlayingLine(Second.A.S) == "Your plan was playing: GTO. [K] carry on.",
		"g1 journey: the checkpoint restores the night, and the host text the plan: the same tilt, Sharpness, counts and stats");
	if (!T3)
	{
		return;
	}
	Second.T = T3.get();
	Second.Hands = HostHands;
	Second.TiltGain = First.TiltGain;
	Second.Notes = &S->Life.Auto.Notes;
	Second.Think();
	const ss::live::SkipWhy SecondWhy = Second.Skip();
	const bool BetweenHands = !T3->FinishPending();
	Expect(SecondWhy == WholeWhy && T3->Checkpoint() == T1->Checkpoint() && journeys::RunState(Second.A, Second.R) == journeys::RunState(Whole.A, Whole.R),
		"g1 journey: the night carries on from its checkpoint exactly as the night that never stopped (ResumeEqualsUninterrupted, through the save)");
	bool Checkpoints5 = !Whole.CheckpointTicks.empty();
	for (size_t K = 0; K + 1 < Whole.CheckpointTicks.size(); ++K)
	{
		Checkpoints5 = Checkpoints5 && Whole.CheckpointTicks[K] % 5 == 0;
	}
	const bool AtBubble = !T3->Hero().Busted && (SecondWhy == ss::live::SkipWhy::Bubble || SecondWhy == ss::live::SkipWhy::HandForHand) && T3->Remaining <= T3->PaidPlaces() + 15;
	Expect(Checkpoints5 && BetweenHands && (AtBubble || T3->Hero().Busted), "g1 journey: checkpoints every 5 hands; the skip stops at the bubble, between hands (or the night ends first)");
	std::printf("  g1 journey: the Nightly: %d entrants, skip stopped (why %d) at hand %d with %d left (paid %d), %zu checkpoints, the plan %d hands / %d decisions, tilt %.3f, Sharpness %.2f\n",
		T3->Spec.Entrants, static_cast<int>(SecondWhy), T3->Tick, T3->Remaining, T3->PaidPlaces(), Whole.CheckpointTicks.size(), Second.R.AiHands, Second.R.AiDecisions, Second.R.Tilt,
		Second.R.Sharp);
	// "Skip to the result" chosen again: past the final table's stop, to the end of the night; settled on the save.
	Second.A.To = autoplay::Target::Result;
	for (int Guard = 0; Guard < 10 && !T3->Hero().Busted && !T3->bFinished; ++Guard)
	{
		Second.Skip();
	}
	ss::SaveData Night = journeys::HostSave(H3, *S);
	const int Place = T3->Hero().Place > 0 ? T3->Hero().Place : T3->HeroRank();
	const ss::Chips Prize = T3->PrizeFor(Place);
	const double End = DayStart + T3->ClockMinutes();
	live::Settle(Night.BankrollCents, Night.Life, O.Id, Place, T3->Spec.Entrants, Prize, End);
	live::PayFare(Night.BankrollCents, Night.Life, O.Id, true, End + 5.0);
	autoplay::Record(Night.Life.Auto, O.Id, 1, 0, Second.A, Second.R);
	Night.Life.Update2LinesRead |= life::U2AutoPlayed | life::U2AutoPref;
	Night.NoteLive(End, O.Id, Place, Prize, T3->Spec.Entrants, {});
	Night.ClockMinutes = End + 30.0 - Day;
	pacing::PaceHooks H4;
	S.reset(new ss::Session(H4, "g1-journey", &Night));
	S->CurrentScreen = ss::Screen::Lobby;
	S->ResumeCalendarFrom(End + 30.0);
	Now = Wait(*S, Now, 0.1);
	const autoplay::Played* PL = autoplay::PlayedFor(S->Life.Auto, O.Id);
	const life::LiveEntry* Done = live::EntryFor(S->Life, O.Id);
	Expect(PL && PL->Venue == 1 && PL->S == style::Style::Gto && PL->AiHands == Second.R.AiHands && PL->OwnDecisions == 0 && Done && Done->State == life::LiveEntry::Finished &&
			   Done->Place == Place && S->LiveInProgress().empty(),
		"g1 journey: the Nightly settled: its played line (the Embercrest, GTO, every hand the plan's) and the result on the record");
	// 5. The world: the plan's events are the player's; nobody says a plan played them; the boards fill.
	S->LobbyMinutes += (static_cast<double>(ss::net::DayOf(S->WorldMinutes()) + 1) * Day + 9.0 * 60.0) - S->WorldMinutes();
	Now = Wait(*S, Now, 0.1);
	{
		const ss::world::Tracker& Page = S->Living().HeroStats();
		ss::net::HeroStats Hero;
		Hero.Name = S->HeroName;
		const std::vector<ss::net::BoardRow> Season = ss::net::Shared().Leaderboard(ss::net::Board::Season, S->WorldMinutes(), Hero, 60, nullptr);
		bool Positive = !Season.empty();
		for (const ss::net::BoardRow& R : Season)
		{
			Positive = Positive && R.Value > 0.0;
		}
		std::string Said;
		int Items = 0;
		for (const ss::net::NewsItem& N : ss::net::Shared().News(S->WorldMinutes(), Hero, 60))
		{
			++Items;
			for (const char* Word : {"ai", "bot", "bots", "autoplay", "autoplayed", "autopilot"})
			{
				Said = Said.empty() && (journeys::Says(N.Title, Word) || journeys::Says(N.Body, Word)) ? N.Title : Said;
			}
		}
		const int Results = static_cast<int>(S->History.size()) + S->Life.LiveEvents;
		std::printf("  g1 journey: the world: %d events on the player's page (%d results online and live), %zu Season rows, %d news items\n", Page.Events, Results, Season.size(), Items);
		Expect(Page.Events == Results && S->Life.LiveEvents == 1, "g1 journey: every event the plan played reached the world as the player's (SPEC 2.9)");
		Expect(Positive, "g1 journey: the fresh world's Season board has filled, every row above zero");
		Expect(Items > 0 && Said.empty(), ("g1 journey: the news never says a plan played (U1)" + (Said.empty() ? std::string() : ": " + Said)).c_str());
	}
	std::printf("  g1 journey: %.1f s\n", journeys::SecondsSince(T0));
}

/**
 * SegmentJourney (carried from the last segment, next_segment_spec §7.6; update 2 SPEC debt item 9): one career of Days
 * days (120 in "long", the whole journey; 60 by default) through every feature of the last segment, with the books
 * audited after every step (journeys::Books). In order:
 *   - Night One: Big Lou's stake on Tuesday's Nightly: his money pays the desk (the three lines); a deep run (third),
 *     and the cage pays him his share, once;
 *   - Thursday: Lantern Row's package: the online buy-ins in its range come off the roll, and the roll never pays rent;
 *   - Friday: pieces of Saturday's Nightly listed at par (Night One's deep run is a result to buy on); Sunday: Dee's
 *     ten percent of the Sunday $150 at par, paid at once, and Mei's swap on it;
 *   - the next Monday: Lucky Penny signed (a patch sewn, Ray's appearance at the store, the Noon Deepstack covered);
 *     Wednesday: a cap from the thrift, Volt Rush signed and its patch on the cap, Overclock refused for the category;
 *   - week three: six Nightlies in six nights, and the club card that comes of them (Regular, toasted once);
 *   - Volt Rush's stipend at its review and never before; notice after it, the term ended at the next review (long);
 *   - every Saturday a Nightly, a hyper every other afternoon (every fourth after two weeks), a shift when the money's
 *     short; the save round-trips each week;
 *   - at the end the segment's lines take at most 80 KB of the save, update 2's non-world lines at most 60 KB.
 */
void SegmentJourney(bool Long)
{
	namespace life = ss::life;
	namespace live = ss::live;
	namespace stake = ss::stake;
	namespace sponsor = ss::sponsor;
	using journeys::Day;
	const auto T0 = std::chrono::steady_clock::now();
	pacing::Career C;
	C.Seed = "segment-journey";
	C.Kind = 2;
	C.Days = Long ? 120 : 60;
	pacing::PaceHooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, C.Seed));
	S->NewCareer(journeys::Jesse());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.1);
	C.Ledger.Take(S->Life);
	journeys::Books B;
	B.Start(*S);
	const int FirstDay = ss::net::DayOf(S->WorldMinutes());
	bool LouPaid = false;       // Lou's money paid the desk: his stake line, the buy-in and the fee
	bool LouShared = false;     // the cage paid him his share (or the night paid nothing)
	bool RollPays = false;      // an online buy-in in the package's range came off the roll
	std::string Pieces;         // what listing pieces of Saturday's Nightly said
	bool Swapped = false;       // Mei's swap on the Sunday: accepted, played
	bool Covered = false;       // Lucky Penny covered the Noon Deepstack
	bool PatchSewn = false;
	bool OverclockRefused = false;
	int WeekNights = 0;         // Nightlies in week three
	bool ClubToast = false;     // the club card toasted once (Unseen)
	int ClubTier = 0;
	std::string VoltId;
	int VoltStipends = 0;
	bool StipendEarly = false;
	int Appearances = 0;
	bool NoticeGiven = false;
	bool CleanEnd = false;
	int RoundTrips = 0;
	bool RoundTripsSame = true;
	double NightSeconds = 0.0; // (where the time goes: the host's nights reload the career, world and all)
	double SaveSeconds = 0.0;
	auto Step = [&](const std::string& Where) { B.Audit(*S, Where); };
	auto Night = [&](int DayNo, const char* Key, life::Funding* F) {
		const auto N0 = std::chrono::steady_clock::now();
		const bool Ok = journeys::SegNight(S, H, C, Now, DayNo, Key, F);
		NightSeconds += journeys::SecondsSince(N0);
		return Ok;
	};
	auto ContractOf = [&](const std::string& Brand) -> const sponsor::Contract* {
		const sponsor::Contract* Found = nullptr;
		for (const sponsor::Contract& K : S->Life.Sponsors.Contracts)
		{
			Found = K.Brand == Brand && K.St != sponsor::State::Offered ? &K : Found;
		}
		return Found;
	};
	for (int K = 0; K < C.Days; ++K)
	{
		const int DayNo = FirstDay + K;
		const std::string When = "day " + std::to_string(K);
		Now = pacing::Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 7.0 * 60.0);
		pacing::Fed(*S);
		if (S->Evicted() && S->Life.StorageDue > 0.0 && S->Life.StorageDue < S->WorldMinutes() + 2.0 * Day)
		{
			S->PayStorage();
		}
		if (S->Evicted() && S->BankrollCents >= S->Life.RentDueCents + 20000)
		{
			S->MoveBackIn();
		}
		Step(When + " morning");
		// The money to play the journey's nights (counted, not earned: Injected).
		if (K == 0 || (K >= 13 && K <= 18 && S->BankrollCents < 60000) || S->BankrollCents < 20000)
		{
			B.Give(*S, K == 0 ? 150000 : 60000);
			Step(When + " a cushion");
		}
		// The scripted offers: what the books would take months to bring, made as their makers make them.
		if (K == 0)
		{
			S->DebugStake("offer lou");
		}
		else if (K == 2)
		{
			S->DebugStake("grant lanternrow");
		}
		else if (K == 5)
		{
			S->DebugStake("offer mei");
		}
		else if (K == 6)
		{
			S->DebugSponsor("offer luckypenny");
		}
		else if (K == 8)
		{
			// Volt Rush wants its patch on a cap: a ball cap from the thrift, worn.
			Now = pacing::Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 11.0 * 60.0);
			ss::Session::ShopOrder Cap;
			Cap.Venue = "thrift";
			Cap.Garments = {"hat.ballcap"};
			const std::string Bought = S->ShopPay(Cap);
			const std::string Worn = Bought.empty() ? S->Wear("hat.ballcap") : Bought;
			if (!Worn.empty())
			{
				std::printf("    (the cap: %s)\n", Worn.c_str());
			}
			Step(When + " a cap");
			S->DebugSponsor("offer voltrush");
		}
		journeys::SegDeals(*S, C);
		Step(When + " the deals");
		if (K == 3)
		{
			// Pieces of Saturday's Nightly, at par.
			for (const live::Occurrence& X : live::Occurrences(DayNo + 1))
			{
				Pieces = std::string(X.T->Key) == "nightly" ? S->ListAction(X.Id, 100, 1000) + "|" + X.Id : Pieces;
			}
			Step(When + " pieces listed");
		}
		if (K == 6 || K == 8)
		{
			// The patch on a spot the contract takes (its asks count only the events it's worn at).
			const std::string Brand = K == 6 ? "luckypenny" : "voltrush";
			bool Sewn = false;
			for (int Spot = 0; Spot < sponsor::PatchSpots && !Sewn; ++Spot)
			{
				const std::vector<std::string> Can = sponsor::PatchesFor(S->Life.Sponsors, Spot, S->WorldMinutes());
				if (std::find(Can.begin(), Can.end(), Brand) != Can.end() && S->Life.Closet.Patches[static_cast<size_t>(Spot)].empty())
				{
					Sewn = S->SewPatch(Spot, Brand).empty() && S->Life.Closet.Patches[static_cast<size_t>(Spot)] == Brand;
				}
			}
			PatchSewn = (K == 6 || PatchSewn) && Sewn;
			if (K == 8)
			{
				const sponsor::Contract* V = ContractOf("voltrush");
				VoltId = V && V->St != sponsor::State::Ended ? V->Id : std::string();
				if (VoltId.empty())
				{
					const std::string Open = sponsor::OpenOfferFor(S->Life.Sponsors, "voltrush", S->WorldMinutes());
					std::printf("    (Volt Rush not signed: %s)\n", Open.empty() ? "no offer open" : S->SignDeal(Open).c_str());
				}
				// A second energy drink while Volt Rush holds the category: refused, and said why.
				S->DebugSponsor("offer overclock");
				const std::string Offer = sponsor::OpenOfferFor(S->Life.Sponsors, "overclock", S->WorldMinutes());
				OverclockRefused = !VoltId.empty() && !Offer.empty() && !S->SignDeal(Offer).empty() && !sponsor::Active(S->Life.Sponsors, "overclock", S->WorldMinutes()) &&
					!sponsor::Conflicts(S->Life.Sponsors, S->Life, Offer, S->WorldMinutes()).empty();
				S->DeclineDeal(Offer);
			}
			Step(When + " a patch");
		}
		// An appearance a contract asks for, when its window falls before the afternoon (the photo with Ray at the store).
		for (const sponsor::Window& W : sponsor::Appearances(S->Life.Sponsors, S->WorldMinutes(), static_cast<double>(DayNo) * Day + 11.0 * 60.0))
		{
			if (W.Done || W.To <= S->WorldMinutes() + 25.0)
			{
				continue;
			}
			Now = pacing::Hop(*S, C, Now, std::max(S->WorldMinutes(), W.From + 1.0));
			if (S->Appear(W.Venue).empty())
			{
				for (int Guard = 0; S->TimeSkip.Active && Guard < 200; ++Guard)
				{
					Now = Wait(*S, Now, 0.5);
				}
				++Appearances;
			}
			Step(When + " an appearance at " + W.Venue);
			break;
		}
		// Monday noon: the Noon Deepstack on Lucky Penny's cover.
		if (K == 6)
		{
			life::Funding F;
			Covered = Night(DayNo, "noon", &F) && F.From == life::Paid::Sponsor && F.Who == "Lucky Penny";
			Step(When + " the Noon Deepstack");
		}
		// The afternoon: a hyper every other day for two weeks, then every fourth (the roll pays once it's there), a shift
		// when money's short.
		if (K != 6 && S->BankrollCents < ss::life::MonthlyRentCents + 40000)
		{
			Now = pacing::Shift(*S, C, Now);
			Step(When + " a shift");
		}
		Now = pacing::Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 14.0 * 60.0);
		pacing::Fed(*S);
		if (K % 2 == 0 && (K < 14 || K % 4 == 2))
		{
			const ss::Chips BankBefore = S->BankrollCents;
			const ss::Chips RollBefore = pacing::RollSum(*S);
			const int Played = C.Online;
			Now = pacing::Online(*S, C, Now);
			if (C.Online > Played && RollBefore > 0)
			{
				// The buy-in came off the roll: the bankroll didn't pay it (the bank moves only by what the deal's lines say).
				const pacing::EventRec& E = C.Events.back();
				RollPays = RollPays || (pacing::RollSum(*S) != RollBefore && S->BankrollCents - BankBefore != -E.BuyIn);
			}
			Step(When + " a hyper");
		}
		// The evening's Embercrest: Lou's Nightly, Saturday's, Mei's Sunday, the covered Noon, week three's six.
		const int Weekday = ss::net::Weekday(DayNo);
		if (K == 0)
		{
			life::Funding F;
			const bool Played = journeys::SegNight(S, H, C, Now, DayNo, "nightly", &F, nullptr, 3); // (a deep run: third)
			bool Stake = false;
			int Shares = 0;
			for (const life::LedgerEntry& E : S->Life.Ledger)
			{
				Stake = Stake || (E.Label == "Big Lou: stake, Embercrest Nightly" && E.Amount == 12000 && E.Kind == life::LedgerBacking);
				Shares += E.Label.rfind("Big Lou: his share", 0) == 0 && E.Amount < 0 ? 1 : 0;
			}
			const life::LiveEntry* LE = nullptr;
			for (const life::LiveEntry& X : S->Life.LiveEntries)
			{
				LE = X.Id.find("nightly@" + std::to_string(DayNo)) != std::string::npos ? &X : LE;
			}
			LouPaid = Played && F.From == life::Paid::Backer && F.Who == "Big Lou" && Stake;
			LouShared = LE && LE->Place == 3 && LE->PrizeCents > 0 && Shares == 1;
			Step(When + " Lou's Nightly");
		}
		else if (K == 5)
		{
			Swapped = Night(DayNo, "sunday", nullptr);
			Step(When + " Mei's Sunday");
		}
		else if (K >= 13 && K <= 18)
		{
			WeekNights += Night(DayNo, "nightly", nullptr) ? 1 : 0;
			Step(When + " week three's Nightly");
		}
		else if (Weekday == 5)
		{
			Night(DayNo, "nightly", nullptr);
			Step(When + " Saturday's Nightly");
		}
		// The club card, toasted once.
		if (K >= 18 && !ClubToast)
		{
			for (const ss::badge::Badge& Bd : S->Unseen())
			{
				ClubToast = ClubToast || Bd.K == ss::badge::Kind::Club;
			}
			const ss::badge::Badge* Club = S->HeroBadges().Find(ss::badge::Kind::Club);
			ClubTier = Club ? Club->Tier : 0;
		}
		// Volt Rush: its stipend paid at each review, never at signing; notice after the second review.
		if (!VoltId.empty())
		{
			int Lines = 0;
			for (const life::LedgerEntry& E : B.All)
			{
				Lines += E.Kind == life::LedgerSponsors && E.Label.rfind("Volt Rush: monthly", 0) == 0 ? 1 : 0;
			}
			VoltStipends = Lines;
			const sponsor::Contract* V = sponsor::Find(S->Life.Sponsors, VoltId);
			// Paid at a review, never at signing: never more stipends than reviews done.
			StipendEarly = StipendEarly || (V && Lines > V->Period);
			if (V && !NoticeGiven && V->Period >= 1 && V->St == sponsor::State::Active)
			{
				// Notice after the first review: the term's next review is its last, paid if its asks were met.
				NoticeGiven = S->GiveNotice(VoltId).empty() && sponsor::Find(S->Life.Sponsors, VoltId)->St == sponsor::State::Notice;
				Step(When + " notice to Volt Rush");
			}
			if (V && NoticeGiven && V->St == sponsor::State::Ended)
			{
				CleanEnd = V->Why == sponsor::End::Walked;
			}
		}
		// A week's save, read back: the same bytes.
		if (K % 7 == 6)
		{
			const auto S0 = std::chrono::steady_clock::now();
			S->Save();
			ss::SaveData Back;
			const std::string Text = H.Last.Serialize();
			RoundTripsSame = RoundTripsSame && ss::SaveData::Parse(Text, Back) && Back.Serialize() == Text;
			++RoundTrips;
			SaveSeconds += journeys::SecondsSince(S0);
		}
	}
	S->Save();
	const std::string Text = H.Last.Serialize();
	const size_t SegmentBytes = journeys::LineBytes(Text, {"livefund\t", "livevoice\t", "liveroom\t", "livebadge\t", "liveleader\t", "livepatch\t", "mail\t", "stake\t", "coach\t",
															   "sponsor\t", "badge\t", "life\tshift\t", "life\tliveday\t"});
	// Update 3 (Phase A, a listed rewrite): Update 2's lines without Update 3's sub-tags and appended fields (LineBytesU2).
	const size_t Update2Bytes = journeys::LineBytesU2(Text);
	int Packages = 0;
	for (const stake::Deal& D : S->Life.Stakes.Deals)
	{
		Packages += D.K == stake::Kind::Package ? 1 : 0;
	}
	std::printf("  segment journey (%d days): %d online, %d live (%d in week three), %d audits, at most %d ledger lines a step; Lou %s/%s; roll pays %s; pieces: %s; swap %s; "
				"covered %s; patch %s; Overclock refused %s; club tier %d (toast %s); Volt Rush stipends %d, notice %s, clean end %s; %d appearances; %d round trips; segment lines %zu bytes, "
				"update 2's %zu; %.1f s (%.1f in the nights, %.1f in the weekly saves)\n",
		C.Days, C.Online, C.Live, WeekNights, B.Steps, B.MostLines, LouPaid ? "paid" : "no", LouShared ? "shared" : "no", RollPays ? "yes" : "no", Pieces.c_str(), Swapped ? "yes" : "no",
		Covered ? "yes" : "no", PatchSewn ? "yes" : "no", OverclockRefused ? "yes" : "no", ClubTier, ClubToast ? "yes" : "no", VoltStipends, NoticeGiven ? "yes" : "no",
		CleanEnd ? "yes" : "no", Appearances, RoundTrips, SegmentBytes, Update2Bytes, journeys::SecondsSince(T0), NightSeconds, SaveSeconds);
	bool PiecesSold = false;
	bool MeiSettled = false;
	for (const life::LedgerEntry& E : B.All)
	{
		PiecesSold = PiecesSold || (E.Kind == life::LedgerBacking && E.Label.rfind("Pieces sold: ", 0) == 0 && E.Amount > 0);
		MeiSettled = MeiSettled || (E.Kind == life::LedgerBacking && E.Label.rfind("Mei: ", 0) == 0 && E.Label.find("swap") != std::string::npos);
	}
	const int FailuresBefore = Failures;
	Expect(B.Broken.empty(), ("segment journey: the books hold after every step (" + (B.First.empty() ? std::string("none broken") : B.First) + ")").c_str());
	Expect(PiecesSold && Pieces.rfind("|embercrest-nightly@", 0) == 0, ("segment journey: pieces listed at par and sold pay the player at once (" + Pieces + ")").c_str());
	Expect(LouPaid && LouShared, "segment journey: Big Lou's money paid the desk, and the cage paid him his share");
	Expect(Packages >= 1 && RollPays && !C.RollPaidRent, "segment journey: the package's roll pays the online buy-ins in its range, and never the rent");
	for (const stake::Swap& Sw : S->Life.Stakes.Swaps)
	{
		MeiSettled = MeiSettled || (Sw.Partner == "Mei" && Sw.State == stake::SwapSettled);
	}
	Expect(Swapped && MeiSettled, "segment journey: Mei's swap: the Sunday played, the swap settled");
	Expect(Covered && PatchSewn, "segment journey: Lucky Penny covers the Noon Deepstack; a patch sewn for a running contract");
	Expect(OverclockRefused, "segment journey: a second energy drink is refused while Volt Rush holds the category, and says why");
	Expect(WeekNights == 6 && ClubTier >= 1 && ClubToast, "segment journey: six Nightlies in a week: the club card, toasted once");
	Expect(VoltStipends >= 1 && !StipendEarly && NoticeGiven, "segment journey: Volt Rush's stipend at its reviews, never at signing; notice given after the first");
	Expect(!Long || (CleanEnd && VoltStipends >= 2), "segment journey: the notice ends the term at its next review, cleanly, that period paid");
	Expect(RoundTrips >= 8 && RoundTripsSame, "segment journey: each week's save reads back the same bytes");
	Expect(SegmentBytes <= 80 * 1024 && Update2Bytes <= 60 * 1024, "segment journey: the segment's lines at most 80 KB of the save, update 2's non-world lines at most 60 KB");
	if (Failures > FailuresBefore)
	{
		// What the career did, for whoever reads the failure.
		for (const auto& It : B.Broken)
		{
			std::printf("    broken %d times: %s\n", It.second, It.first.c_str());
		}
		for (const sponsor::Contract& K : S->Life.Sponsors.Contracts)
		{
			std::printf("    contract %s: state %d (%s), period %d of %d, strikes %d, paid %s\n", K.Id.c_str(), static_cast<int>(K.St), sponsor::Pill(K, S->WorldMinutes()), K.Period,
				sponsor::Periods(K), K.Strikes, ss::Money(K.PaidCents).c_str());
		}
		for (const life::LedgerEntry& E : B.All)
		{
			if (E.Kind == life::LedgerSponsors || E.Kind == life::LedgerBacking)
			{
				std::printf("    %s %s %s\n", ss::net::DateLabel(ss::net::DayOf(E.At)).c_str(), ss::Money(E.Amount).c_str(), E.Label.c_str());
			}
		}
	}
}
} // namespace session_test

// ------------------------------------------------------------------ update 2's C3 journey (update 2 SPEC §7.5 C3, §6.7)

namespace session_test
{
namespace journeys
{
/**
 * The rival sites' books (update 2 SPEC §1.2's conservation; Update2Journey audits them after every step, beside Books):
 *   - each wallet moves only by its own lines;
 *   - deposits out of the bank (its "Deposit: <site>" lines) equal the wallets' deposit lines, and the bank's card-fee lines
 *     (kind 14) are the card's rate on each, to the cent;
 *   - withdrawals asked for equal what's in transit plus what landed plus the fees taken, to the cent;
 *   - a transfer lands once (one bank line each), and no month's RiverLine Rewards or Rerun payout is paid twice;
 *   - an invitation not yet answered is always in Mail.
 */
struct SiteBooks
{
	struct Wallet
	{
		ss::Chips Cents = 0;
		std::vector<ss::site::Line> Lines;
	};
	std::map<int, Wallet> Wallets;
	std::map<std::string, int> Xfers; // transfer id -> its state as last seen
	size_t BankSeen = 0;              // Books::All's lines read so far
	ss::Chips DepositOut = 0, DepositIn = 0, Fees = 0, FeesDue = 0, Asked = 0, Landed = 0, LandedFees = 0;
	int Arrivals = 0, LandedXfers = 0;
	std::set<std::string> Months; // the monthly payouts' lines
	std::map<std::string, int> Broken;
	std::string First;

	void Break(const std::string& What, const std::string& Where)
	{
		if (Broken[What]++ == 0 && First.empty())
		{
			First = What + " (" + Where + ")";
		}
	}
	static bool Same(const ss::site::Line& A, const ss::site::Line& B) { return A.At == B.At && A.Kind == B.Kind && A.Cents == B.Cents && A.Label == B.Label; }
	void Audit(const ss::Session& S, const Books& B, const std::string& Where)
	{
		namespace site = ss::site;
		namespace net = ss::net;
		for (; BankSeen < B.All.size(); ++BankSeen)
		{
			const life::LedgerEntry& E = B.All[BankSeen];
			if (E.Kind == 7 && E.Amount < 0 && E.Label.rfind("Deposit: ", 0) == 0)
			{
				DepositOut -= E.Amount;
				for (const net::SiteDef& D : net::Sites())
				{
					const net::CashMethod* M = D.Id != net::SiteId::RiverLine && E.Label == "Deposit: " + D.Name ? site::MethodFor(D.Id, "card", false) : nullptr;
					FeesDue += M ? static_cast<ss::Chips>(std::llround(static_cast<double>(-E.Amount) * static_cast<double>(M->FeePermille) / 1000.0)) : 0;
				}
			}
			if (E.Kind == 7 && E.Amount > 0 && E.Label.rfind("Withdrawal: ", 0) == 0)
			{
				Landed += E.Amount;
				++Arrivals;
			}
			Fees -= E.Kind == life::LedgerSites ? E.Amount : 0;
			const bool Monthly = (E.Kind == 4 && E.Label.rfind("RiverLine Rewards:", 0) == 0) || (E.Kind == life::LedgerVideo && E.Label.rfind("Rerun payout:", 0) == 0);
			if (Monthly && !Months.insert(E.Label).second)
			{
				Break("a month paid twice: " + E.Label, Where);
			}
		}
		for (const site::Account& A : S.Life.Sites.Accounts)
		{
			Wallet& W = Wallets[static_cast<int>(A.Site)];
			// The lines since the last look (a site keeps its last 40, the newest at the back).
			size_t From = 0;
			for (size_t K = A.Lines.size(); !W.Lines.empty() && K-- > 0;)
			{
				if (Same(A.Lines[K], W.Lines.back()))
				{
					From = K + 1;
					break;
				}
			}
			ss::Chips Moved = 0;
			for (size_t K = From; K < A.Lines.size(); ++K)
			{
				const site::Line& L = A.Lines[K];
				Moved += L.Cents;
				DepositIn += L.Kind == 7 && L.Cents > 0 ? L.Cents : 0;
				Asked += L.Kind == 7 && L.Cents < 0 ? -L.Cents : 0;
			}
			if (A.Cents - W.Cents != Moved)
			{
				Break("a wallet moved without its lines (" + std::string(net::KeyOf(A.Site)) + " " + ss::Money(A.Cents - W.Cents) + " vs " + ss::Money(Moved) + ")", Where);
			}
			W.Cents = A.Cents;
			W.Lines = A.Lines;
		}
		ss::Chips Transit = 0;
		for (const site::Transfer& X : S.Life.Sites.Transfers)
		{
			const auto It = Xfers.find(X.Id);
			if (X.State == 1 && (It == Xfers.end() || It->second == 0))
			{
				++LandedXfers;
				LandedFees += X.FeeCents;
			}
			Transit += X.State == 0 && X.Out ? X.Cents : 0;
			Xfers[X.Id] = X.State;
		}
		if (DepositOut != DepositIn)
		{
			Break("deposits out of the bank aren't the wallets' deposits (" + ss::Money(DepositOut) + " vs " + ss::Money(DepositIn) + ")", Where);
		}
		if (Fees != FeesDue)
		{
			Break("the card fees aren't the card's rate (" + ss::Money(Fees) + " vs " + ss::Money(FeesDue) + ")", Where);
		}
		if (Asked != Transit + Landed + LandedFees)
		{
			Break("withdrawals asked for aren't transit + landed + fees (" + ss::Money(Asked) + " vs " + ss::Money(Transit) + " + " + ss::Money(Landed) + " + " + ss::Money(LandedFees) + ")",
				Where);
		}
		if (Arrivals != LandedXfers)
		{
			Break("a transfer landed twice, or without its bank line (" + std::to_string(Arrivals) + " lines, " + std::to_string(LandedXfers) + " landed)", Where);
		}
		for (const site::Invitation& I : S.Life.Sites.Invites)
		{
			const bool Open = I.State == 0 || I.State == 3;
			const std::string Thread = net::KeyOf(I.Site);
			if (Open && std::none_of(S.Life.Inbox.begin(), S.Life.Inbox.end(),
							[&Thread](const life::Letter& L) { return L.Source == "site" && L.Thread == Thread && L.Kind == ss::site::LetterInvite; }))
			{
				Break("an unanswered invitation dropped from Mail: " + Thread, Where);
			}
		}
	}
};

/**
 * Update2Journey's event: a site's next start of a template (no later than Latest), played by the player at Sprint (the hands
 * Sprint hands back, the grader's best) or skipped to the result by a plan of the given style (the decisions a stop hands
 * back, the grader's best). Notes U4 on the way: an event's graded decisions are exactly the player's own, and a plan's played
 * line carries its site. False when it didn't open.
 */
bool JourneyEvent(ss::Session& S, pacing::Career& C, double& Now, ss::net::SiteId Site, const std::string& Base, double Latest, int Plan, int& Ungraded, int& OffSite)
{
	const ss::net::Network& Net = ss::net::Site(Site);
	ss::net::EventInstance I;
	if (!pacing::NextOn(Net, Base, S.WorldMinutes() + 1.0, I) || I.Start > Latest || !S.CanAfford(Net.Listing(I, nullptr, S.Unlocks())))
	{
		return false;
	}
	Now = pacing::Hop(S, C, Now, I.Start);
	pacing::Fed(S);
	const ss::LobbyEvent L = Net.Listing(I, nullptr, S.Unlocks());
	if (!S.CanAfford(L))
	{
		return false;
	}
	// Update 3 (W3, a deliberate rewrite at Phase A): under a "plays only on" clause the register button's second press is
	// the token core asks for (SitesTwoPress). The first press alone is refused, with nothing changed; the second registers
	// and strikes, as the journey's RegisterStruck expects.
	const bool Clause = ss::sponsor::WouldStrike(S.Life.Sponsors, static_cast<int>(Site), false, false);
	if (Clause)
	{
		const ss::Chips* Wallet = ss::site::WalletOf(S.Life.Sites, Site);
		const ss::Chips Before = Wallet ? *Wallet : -1, Bank = S.BankrollCents;
		const int Tables = S.TableCount();
		const std::string Why = S.RegisterEvent(L);
		Wallet = ss::site::WalletOf(S.Life.Sites, Site);
		Expect(!Why.empty() && S.TableCount() == Tables && (Wallet ? *Wallet : -1) == Before && S.BankrollCents == Bank && !S.IsPlaying(L.Spec.Id),
			("journey: under the clause, a registration without the second press is refused and changes nothing: " + Why).c_str());
	}
	S.RegisterEvent(L, ss::StrikeConfirm{Clause});
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	S.SprintBudgetMs = 1.0e9;
	if (Plan >= 0)
	{
		S.SetAutoplay(0, PlanOf(static_cast<ss::style::Style>(Plan), ss::autoplay::Mode::Skip, 1, ss::autoplay::Target::Result, 0));
	}
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		if (Plan < 0)
		{
			S.CurrentPace = ss::Pace::Sprint;
		}
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
			size_t Best = 0;
			for (size_t K = 1; K < Opts.size(); ++K)
			{
				Best = Opts[K].Ev > Opts[Best].Ev ? K : Best;
			}
			S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
		}
	}
	const ss::autoplay::Played* P = ss::autoplay::PlayedFor(S.Life.Auto, I.Id);
	if (S.CurrentScreen == ss::Screen::Results)
	{
		const size_t Graded = S.LastResults.Grades.size();
		Ungraded += P ? (Graded != static_cast<size_t>(P->OwnDecisions) ? 1 : 0) : 0;
		OffSite += Plan >= 0 && (!P || P->Site != static_cast<int>(Site) || P->AiHands <= 0) ? 1 : 0;
	}
	pacing::EventRec Rec;
	Rec.Id = I.Id;
	Rec.Place = S.LastResults.Place;
	Rec.Field = S.LastResults.Entrants;
	Rec.Prize = S.LastResults.PrizeCents;
	Rec.At = I.Start;
	Rec.BuyIn = L.BuyInCents;
	Rec.Plan = Plan >= 0;
	S.LeaveResults();
	C.Events.push_back(Rec);
	++C.Online;
	++C.BySite[static_cast<size_t>(Site)];
	C.BuyInsBySite[static_cast<size_t>(Site)] += L.BuyInCents;
	C.Ledger.Take(S.Life);
	pacing::SitesObserve(S, C);
	return true;
}

/** Update2Journey's upload at 9 PM: the best open candidate Studio would let through without a strike (while a clause runs,
 *  only RiverLine's hands, the Embercrest's and Dee's), as a Breakdown in Story style with the Reaction frame. */
bool JourneyUpload(ss::Session& S, pacing::Career& C, double& Now)
{
	for (const ss::video::Candidate& X : S.VideoCandidates())
	{
		// Any online hand on a rival's table: the player's (Source 0) or the plan's (3), as Studio's warning reads them.
		if ((X.Source == 0 || X.Source == 3) && X.Site != 0 && ss::sponsor::WouldStrike(S.Life.Sponsors, X.Site, false, true))
		{
			continue;
		}
		ss::video::Choice Ch;
		Ch.Candidates = {X.Id};
		Ch.F = C.Uploads % 3 == 1 ? ss::video::Format::Short : ss::video::Format::Breakdown;
		Ch.T = ss::video::TitleStyle::Story;
		Ch.Fr = ss::video::Frame::Reaction;
		pacing::Fed(S);
		if (!ss::video::CanUpload(Ch, S.Life.Video.Open).empty() || !S.Publish(Ch).empty())
		{
			continue;
		}
		for (int Guard = 0; S.TimeSkip.Active && Guard < 200; ++Guard)
		{
			Now = Wait(S, Now, 0.5);
		}
		++C.Uploads;
		C.Ledger.Take(S.Life);
		return true;
	}
	return false;
}
} // namespace journeys

/**
 * Update2Journey (update 2 SPEC §6.7, §7.5 C3): one 150-day career through all four of the update's features, end to end in
 * core, the clock always moving through the session (CheckCalendar's SiteStep and VideoStep, the rent, the world):
 *   1. A fresh start (Blank): the books open empty; by the end, Team RiverLine and Team RunItTwice have signed on results.
 *   2. Autoplay: every other event skipped to the result by a plan (TAG, LAG, GTO and Exploitative in turn), on every site;
 *      no plan decision is ever graded (U4), and a plan's played line carries its site.
 *   3. Rerun: an upload three times a week from day 7, cut from the career's own hands (never a strike: while the clause
 *      runs, only RiverLine's); on day 100 the console's grant (150 days are too short for the partner gate), then the review,
 *      the welcome and a payout through VideoStep: one kind-13 bank line with its statement.
 *   4. Ignite: its letter at its own gate (Act I over, 150 paid events, open since Nov 2), Mail's Accept, $50 by card (its fee,
 *      "We noticed"); then, four days before a rent, the bank short of it and Ignite's money asked back: the rent card warns
 *      from 72 hours out, the money lands once, net of its fee, and site money never pays the rent.
 *   5. RunItTwice: its letter at its own gate (300 tracked events, the last hundred at $11 or more: the career moves up to $11
 *      events at 180), Mail's Accept, $50 by card and its tickets, with their letter.
 *   6. The clause: Team RiverLine signed the day after (plays only on RiverLine); the register button warns first
 *      (WouldStrike), then a RunItTwice table is a strike with its cure date in Mail; another day, Studio's PUBLISH of a
 *      RunItTwice hand warns first, then it's the second strike, and the deal ends.
 *   7. After every step the books hold (Books: the bank only through its ledger, the journey's cushions counted; no payment
 *      twice; no open offer dropped) and the sites' (SiteBooks: wallets, deposits and fees, withdrawals to the cent, a transfer
 *      landed once, no month paid twice, no unanswered invitation dropped); each week's save reads back the same; the
 *      update's non-world lines stay under 60 KB.
 * In "long" only (about two minutes).
 */
void Update2Journey()
{
	namespace autoplay = ss::autoplay;
	namespace life = ss::life;
	namespace net = ss::net;
	namespace site = ss::site;
	namespace sponsor = ss::sponsor;
	namespace style = ss::style;
	using journeys::Day;
	const auto T0 = std::chrono::steady_clock::now();
	pacing::Career C;
	C.Seed = "update2-journey";
	C.Kind = 2;
	C.Days = 150;
	C.Sites = true; // pacing::Hop's looks at the rent card, and the site money at every deadline
	pacing::PaceHooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, C.Seed));
	S->NewCareer(journeys::Jesse(), ss::world::Start::Blank);
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.1);
	C.Start = S->WorldMinutes();
	C.Ledger.Take(S->Life);
	journeys::Books B;
	B.Start(*S);
	journeys::SiteBooks M;
	std::vector<life::Letter> AllMail; // every letter the journey saw, once (the copy pass at the end)
	std::set<std::string> MailSeen;
	auto Step = [&](const std::string& Where) {
		B.Audit(*S, Where);
		M.Audit(*S, B, Where);
		pacing::SitesObserve(*S, C);
		for (const life::Letter& L : S->Life.Inbox)
		{
			if (MailSeen.insert(ss::Fixed(L.At, 2) + "|" + L.Thread + "|" + L.Subject).second)
			{
				AllMail.push_back(L);
			}
		}
	};
	const bool BlankAtStart = S->WorldStart() == ss::world::Start::Blank;
	const int FirstDay = net::DayOf(S->WorldMinutes());
	const style::Style Plans[4] = {style::Style::Tag, style::Style::Lag, style::Style::Gto, style::Style::Exploit};
	int Turn = 0;
	int Ungraded = 0, OffSite = 0, ByPlan = 0;
	int Uploads = 0;
	bool Granted = false;
	// Ignite's rent: the deadline the script sets up, and what it saw.
	double ScriptRent = -1.0;
	bool ScriptAsked = false;
	ss::Chips ScriptOut = 0;
	std::string ScriptLine;
	bool ScriptFigures = false;   // the card's line names what's on its way net of its fee, and what sits on the wallet
	bool ScriptLateRight = false; // late exactly when the money hadn't landed by the deadline
	bool ScriptLate = false;
	// RunItTwice and the clause.
	int RitJoinedDay = -1;
	bool TicketsLetter = false;
	std::string ClauseId;
	bool RegisterWarned = false, RegisterStruck = false, CureDated = false;
	bool PublishWarned = false, PublishStruck = false, DealEnded = false;
	std::string RitCandidate;
	int RoundTrips = 0;
	bool RoundTripsSame = true;
	for (int K = 0; K < C.Days; ++K)
	{
		const int DayNo = FirstDay + K;
		const std::string When = "day " + std::to_string(K);

		Now = pacing::Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 7.0 * 60.0);
		pacing::Fed(*S);
		if (S->Evicted() && S->Life.StorageDue > 0.0 && S->Life.StorageDue < S->WorldMinutes() + 2.0 * Day)
		{
			S->PayStorage();
		}
		if (S->Evicted() && S->BankrollCents >= S->Life.RentDueCents + 20000)
		{
			S->MoveBackIn();
		}
		Step(When + " morning");
		// The journey's money (counted, not earned: Books' Injected): the rent due and the day's buy-ins (the scripted short rent
		// below takes the bank down itself; a late rent is paid in its grace days from this).
		const ss::Chips Need = (S->Life.RentStage == life::Rent::Paid ? life::MonthlyRentCents : S->Life.RentDueCents) + 30000;
		if (S->BankrollCents < Need)
		{
			B.Give(*S, Need + 30000 - S->BankrollCents);
			Step(When + " a cushion");
		}
		// Mail: an invitation is accepted the morning it comes; $50 by card on the site.
		for (const site::Invitation& I : std::vector<site::Invitation>(S->Life.Sites.Invites))
		{
			if (I.State != 0 && I.State != 3)
			{
				continue;
			}
			pacing::SitesObserve(*S, C);
			const std::string Why = S->AnswerInvite(I.Site, true);
			const std::string Paid = Why.empty() ? S->Deposit(I.Site, 5000, "card") : Why;
			if (!Paid.empty())
			{
				std::printf("    (joining %s: %s)\n", net::Def(I.Site).Name.c_str(), Paid.c_str());
			}
			RitJoinedDay = I.Site == net::SiteId::RunItTwice ? K : RitJoinedDay;
			Step(When + " joined " + net::Def(I.Site).Name);
		}
		if (RitJoinedDay >= 0 && !TicketsLetter)
		{
			for (const life::Letter& L : S->Life.Inbox)
			{
				TicketsLetter = TicketsLetter || (L.Source == "site" && L.Thread == "runittwice" && L.Subject == "Your tickets are in");
			}
		}
		// The clause: Team RiverLine the day after RunItTwice's account opens (signed from now on, it plays only on RiverLine).
		if (RitJoinedDay >= 0 && K == RitJoinedDay + 1)
		{
			S->DebugSponsor("offer riverline");
			const std::string Id = sponsor::OpenOfferFor(S->Life.Sponsors, "riverline", S->WorldMinutes());
			const std::string Why = Id.empty() ? std::string("no offer") : S->SignDeal(Id);
			ClauseId = Why.empty() ? Id : std::string();
			if (!Why.empty())
			{
				std::printf("    (Team RiverLine: %s)\n", Why.c_str());
			}
			Step(When + " Team RiverLine");
		}
		const sponsor::Contract* Clause = ClauseId.empty() ? nullptr : sponsor::Find(S->Life.Sponsors, ClauseId);
		const bool Only = Clause && Clause->St == sponsor::State::Active && Clause->T.Only == "riverline";
		// Ignite's rent: four days before the next deadline after Ignite's account opened, the bank down to $25 short of the rent
		// (spent elsewhere: the journey's hand) and all but $15 on Ignite asked back (it lands in three to five days; the $15
		// stays on the wallet, where the rent can't touch it).
		const site::Account* Ign = site::AccountOf(S->Life.Sites, net::SiteId::Ignite);
		if (ScriptRent < 0.0 && Ign && Ign->Deposited > 0 && K >= 7 && !S->Evicted() && S->Life.RentStage != life::Rent::Paid &&
			S->Life.RentDeadline - S->WorldMinutes() < 4.0 * Day + 6.0 * 60.0 && S->Life.RentDeadline - S->WorldMinutes() > 3.0 * Day + 6.0 * 60.0)
		{
			ScriptRent = S->Life.RentDeadline;
			if (Ign->Cents < 7000)
			{
				S->Deposit(net::SiteId::Ignite, 5000, "card");
			}
			B.Give(*S, S->Life.RentDueCents - 2500 - S->BankrollCents);
			ScriptOut = site::AccountOf(S->Life.Sites, net::SiteId::Ignite)->Cents - 1500;
			ScriptAsked = S->Withdraw(net::SiteId::Ignite, ScriptOut, "bank").empty();
			Step(When + " Ignite's money asked back, the rent near");
			// The rent card from here to the deadline (pacing::Hop looks on every hour of its last 72).
			std::string Line;
			const double At72 = ScriptRent - 72.0 * 60.0;
			Now = pacing::Hop(*S, C, Now, At72);
			if (ss::site::RentWarning(S->Life.Sites, S->BankrollCents, S->Life.RentDueCents, S->Life.RentDeadline, S->WorldMinutes(), Line))
			{
				ScriptLine = Line;
			}
			// Its figures (§5.5: "$197 is on its way", the $200 asked less its fee): the transfer's net, never its gross, and the
			// wallet's balance.
			const site::Account* Left = site::AccountOf(S->Life.Sites, net::SiteId::Ignite);
			for (const site::Transfer& X : S->Life.Sites.Transfers)
			{
				ScriptFigures = ScriptFigures || (X.State == 0 && X.Out && X.Site == net::SiteId::Ignite && X.FeeCents > 0 && Left && Left->Cents > 0 &&
													 ScriptLine.find(ss::Money(X.Cents - X.FeeCents)) != std::string::npos &&
													 ScriptLine.find(ss::Money(X.Cents)) == std::string::npos && ScriptLine.find(ss::Money(Left->Cents)) != std::string::npos);
			}
			Step(When + " 72 hours before the rent");
			// Past the deadline (pacing::Hop checks the sites' money across it): the money landed in time and the bank paid the
			// rent with it, or it hadn't and the rent is late, whatever sat on the sites.
			bool Landed = false;
			Now = pacing::Hop(*S, C, Now, ScriptRent + 60.0);
			for (const site::Transfer& X : S->Life.Sites.Transfers)
			{
				Landed = Landed || (X.Site == net::SiteId::Ignite && X.State == 1 && X.ArrivesAt <= ScriptRent);
			}
			ScriptLate = S->Life.RentStage == life::Rent::FinalNotice;
			ScriptLateRight = ScriptLate != Landed;
			Step(When + " the rent's deadline");
			continue;
		}
		// The afternoon online: 180 tracked events of hypers (four a day, six until Ignite's 150), then $11 and up (RunItTwice's
		// door); under the clause, RiverLine only. Every other event the plan's.
		Now = pacing::Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 12.5 * 60.0);
		pacing::Fed(*S);
		const double Evening = static_cast<double>(DayNo) * Day + 18.0 * 60.0;
		auto Play = [&](net::SiteId Site, const char* Base, double Latest) {
			const int Plan = Turn % 2 == 0 ? static_cast<int>(Plans[(Turn / 2) % 4]) : -1;
			const bool Played = journeys::JourneyEvent(*S, C, Now, Site, Base, Latest, Plan, Ungraded, OffSite);
			Turn += Played ? 1 : 0;
			ByPlan += Played && Plan >= 0 ? 1 : 0;
			Step(When + " " + Base);
			return Played;
		};
		auto Wallet = [&](net::SiteId Site) {
			const ss::Chips* W = site::WalletOf(S->Life.Sites, Site);
			return W ? *W : -1;
		};
		auto TopUp = [&](net::SiteId Site, ss::Chips Need) {
			if (Wallet(Site) >= 0 && Wallet(Site) < Need)
			{
				B.Give(*S, 5200);
				S->Deposit(Site, 5000, "card");
				Step(When + " a top-up");
			}
			return !Only && Wallet(Site) >= Need;
		};
		if (RitJoinedDay >= 0 && K == RitJoinedDay + 2)
		{
			// The register button under the clause: it warns first; pressed again, the table opens and it's a strike.
			const int Before = Clause ? Clause->Strikes : -1;
			RegisterWarned = Only && sponsor::WouldStrike(S->Life.Sponsors, static_cast<int>(net::SiteId::RunItTwice), false, false);
			if (Wallet(net::SiteId::RunItTwice) < 1100)
			{
				B.Give(*S, 5200);
				S->Deposit(net::SiteId::RunItTwice, 5000, "card");
				Step(When + " a top-up");
			}
			Play(net::SiteId::RunItTwice, "runittwice.midday-classic", Evening);
			Clause = sponsor::Find(S->Life.Sponsors, ClauseId);
			RegisterStruck = Clause && Clause->Strikes == Before + 1;
			for (const life::Letter& L : S->Life.Inbox)
			{
				CureDated = CureDated || (L.Source == "sponsor" && L.Thread == ClauseId && L.Body.find("Stay on RiverLine through") != std::string::npos);
			}
		}
		else if (RitJoinedDay >= 0 && K == RitJoinedDay + 3)
		{
			// Studio's PUBLISH on a RunItTwice hand from yesterday's table (still open: the newest thirty are kept): it warns
			// first; published, it's the second strike, and the deal ends.
			for (const ss::video::Candidate& X : S->VideoCandidates())
			{
				RitCandidate = RitCandidate.empty() && X.Site == static_cast<int>(net::SiteId::RunItTwice) && (X.Source == 0 || X.Source == 3) ? X.Id : RitCandidate;
			}
			if (RitCandidate.empty())
			{
				std::printf("    (no RunItTwice hand open in Studio)\n");
			}
			PublishWarned = Only && sponsor::WouldStrike(S->Life.Sponsors, static_cast<int>(net::SiteId::RunItTwice), false, true);
			ss::video::Choice Ch;
			Ch.Candidates = {RitCandidate};
			Ch.F = ss::video::Format::Breakdown;
			Ch.T = ss::video::TitleStyle::Story;
			Ch.Fr = ss::video::Frame::Reaction;
			const int Before = Clause ? Clause->Strikes : -1;
			const std::string Why = S->Publish(Ch);
			if (!Why.empty())
			{
				std::printf("    (the RunItTwice hand's PUBLISH: %s)\n", Why.c_str());
			}
			for (int Guard = 0; S->TimeSkip.Active && Guard < 200; ++Guard)
			{
				Now = Wait(*S, Now, 0.5);
			}
			Clause = sponsor::Find(S->Life.Sponsors, ClauseId);
			PublishStruck = Why.empty() && Clause && (Clause->Strikes == Before + 1 || Clause->St == sponsor::State::Ended);
			DealEnded = Clause && Clause->St == sponsor::State::Ended;
			Uploads += Why.empty() ? 1 : 0;
			Step(When + " a RunItTwice hand published");
		}
		else if (C.Online < 180)
		{
			const int Hypers = site::AccountOf(S->Life.Sites, net::SiteId::Ignite) ? 4 : 6;
			for (int N = 0; N < Hypers; ++N)
			{
				Play(net::SiteId::RiverLine, "hyper-sprint", Evening);
			}
			if (TopUp(net::SiteId::Ignite, 110))
			{
				Play(net::SiteId::Ignite, "ignite.flip-go", Evening);
			}
		}
		else if (RitJoinedDay < 0)
		{
			// $11 and up: the 3 PM Step 2, Ignite's 4 PM Coral Classic, the 9 PM Step 2.
			Play(net::SiteId::RiverLine, "step2", static_cast<double>(DayNo) * Day + 15.0 * 60.0 + 1.0);
			if (TopUp(net::SiteId::Ignite, 1100))
			{
				Play(net::SiteId::Ignite, "ignite.coral-classic", Evening);
			}
			Now = pacing::Hop(*S, C, Now, static_cast<double>(DayNo) * Day + 20.5 * 60.0);
			Play(net::SiteId::RiverLine, "step2", static_cast<double>(DayNo) * Day + 21.0 * 60.0 + 1.0);
		}
		else
		{
			// After RunItTwice: two hypers a day, and while the clause doesn't hold (the deal ended), a rival's too.
			Play(net::SiteId::RiverLine, "hyper-sprint", Evening);
			Play(net::SiteId::RiverLine, "hyper-sprint", Evening);
			if (TopUp(net::SiteId::Ignite, 110))
			{
				Play(net::SiteId::Ignite, "ignite.flip-go", Evening);
			}
		}
		// Rerun: Tuesday, Thursday and Saturday from day 7, at 9 PM; the grant on day 100.
		if (K >= 7 && (K % 7 == 0 || K % 7 == 2 || K % 7 == 4))
		{
			Now = pacing::Hop(*S, C, Now, std::max(S->WorldMinutes(), static_cast<double>(DayNo) * Day + 21.5 * 60.0));
			Uploads += journeys::JourneyUpload(*S, C, Now) ? 1 : 0;
			Step(When + " an upload");
		}
		if (K == 100 && !Granted)
		{
			Granted = S->DebugVideo("grant 1000 4000 60000").rfind("Granted.", 0) == 0;
			Step(When + " the grant");
		}
		// A week's save, read back: the same bytes.
		if (K % 7 == 6)
		{
			S->Save();
			ss::SaveData Back;
			const std::string Text = H.Last.Serialize();
			RoundTripsSame = RoundTripsSame && ss::SaveData::Parse(Text, Back) && Back.Serialize() == Text;
			++RoundTrips;
		}
	}
	S->Save();
	const std::string Text = H.Last.Serialize();
	// Update 3 (Phase A, a listed rewrite): Update 2's lines without Update 3's sub-tags and appended fields (LineBytesU2).
	const size_t Update2Bytes = journeys::LineBytesU2(Text);
	// What the journey saw.
	const double Act1 = pacing::ActIEndOf(C.Ledger);
	int RiverLineTeam = 0, RitTeam = 0;
	for (const ss::world::Npc& N : S->Living().People())
	{
		RiverLineTeam += N.Pro ? 1 : 0;
		RitTeam += N.Sponsor == "Team " + net::Def(net::SiteId::RunItTwice).Name ? 1 : 0;
	}
	const life::LedgerEntry* Payout = nullptr;
	for (const life::LedgerEntry& E : B.All)
	{
		Payout = !Payout && E.Kind == life::LedgerVideo && E.Label.rfind("Rerun payout:", 0) == 0 ? &E : Payout;
	}
	bool Statement = false;
	for (const life::Letter& L : S->Life.Inbox)
	{
		Statement = Statement || (L.Source == "video" && Payout && L.At >= Payout->At - 1.0 && L.At <= Payout->At + Day);
	}
	std::string Letters;
	bool Gates = !C.Invited.empty();
	bool BeforeActI = false;
	double IgniteAt = -1.0, RitAt = -1.0;
	for (size_t I = 0; I < C.Invited.size(); ++I)
	{
		const net::SiteId Site = static_cast<net::SiteId>(C.Invited[I].first);
		const net::SiteDef& D = net::Def(Site);
		Letters += (Letters.empty() ? "" : "; ") + D.Name + "'s on day " + std::to_string(static_cast<int>((C.Invited[I].second - C.Start) / Day)) + " (" +
			std::to_string(C.InvitedAt[I].first) + " events, the last hundred's average " + ss::Money(C.InvitedAt[I].second) + ")";
		Gates = Gates && C.InvitedAt[I].first >= D.Gate.MinEvents && C.InvitedAt[I].second >= D.Gate.MinAbi100;
		BeforeActI = BeforeActI || C.Invited[I].second < Act1;
		IgniteAt = Site == net::SiteId::Ignite ? C.Invited[I].second : IgniteAt;
		RitAt = Site == net::SiteId::RunItTwice ? C.Invited[I].second : RitAt;
	}
	const double IgniteOpens = static_cast<double>(net::Def(net::SiteId::Ignite).LaunchDay) * Day;
	std::printf("  update2 journey (%d days, a fresh world): %d events online (RiverLine %d, Ignite %d, RunItTwice %d), %d of them the plan's; %d uploads (Partner %s, the first "
				"payout %s); Act I over on day %d; invitations: %s\n",
		C.Days, C.Online, C.BySite[0], C.BySite[1], C.BySite[2], ByPlan, Uploads, S->Life.Video.PartnerAt >= 0.0 ? "yes" : "no",
		Payout ? (ss::Money(Payout->Amount) + " on day " + std::to_string(static_cast<int>((Payout->At - C.Start) / Day))).c_str() : "none",
		Act1 < 1.0e17 ? static_cast<int>((Act1 - C.Start) / Day) : -1, Letters.c_str());
	std::printf("  update2 journey: Ignite's rent: %s asked back four days out, the rent card at 72 hours: \"%s\"; the rent %s; %d rent deadlines with an account, %d hourly looks "
				"short in the bank with the sites enough, %d without the warning\n",
		ss::Money(ScriptOut).c_str(), ScriptLine.c_str(), ScriptLate ? "late (paid in its grace days: the money landed after it)" : "collected from the bank", C.RentDeadlines,
		C.WarnSamples, C.WarnMissed);
	std::printf("  update2 journey: the clause: register warned %s, struck %s (cure dated %s); publish warned %s, struck %s, the deal %s; tickets' letter %s; teams at the end: "
				"RiverLine %d, RunItTwice %d; the deposits %s, fees %s, asked back %s, landed %s; %d audits; %d round trips; update 2's lines %zu bytes (autoplay %zu, video %zu, "
				"sites %zu); %.1f s\n",
		RegisterWarned ? "yes" : "no", RegisterStruck ? "yes" : "no", CureDated ? "yes" : "no", PublishWarned ? "yes" : "no", PublishStruck ? "yes" : "no", DealEnded ? "ended" : "running",
		TicketsLetter ? "yes" : "no", RiverLineTeam, RitTeam, ss::Money(M.DepositOut).c_str(), ss::Money(M.Fees).c_str(), ss::Money(M.Asked).c_str(), ss::Money(M.Landed).c_str(), B.Steps,
		RoundTrips, Update2Bytes, journeys::LineBytes(Text, {"autoplay\t"}), journeys::LineBytes(Text, {"video\t"}), journeys::LineBytes(Text, {"site\t"}), journeys::SecondsSince(T0));
	Expect(BlankAtStart && S->WorldStart() == ss::world::Start::Blank && RiverLineTeam > 0 && RitTeam > 0,
		"update2 journey: a fresh world, whose teams (Team RiverLine, Team RunItTwice) sign on results");
	Expect(ByPlan >= 40 && Ungraded == 0 && OffSite == 0, "update2 journey: the plan plays every other event on every site, never graded, its played lines carrying their site (U4)");
	Expect(Uploads >= 40 && Granted && S->Life.Video.PartnerAt >= 0.0 && Payout && Payout->Amount > 0 && Statement,
		"update2 journey: Rerun: uploads from the career's own hands; after the grant, the partner gate, and a payout through the calendar with its statement");
	Expect(Gates && !BeforeActI && IgniteAt >= IgniteOpens && RitAt > 0.0, "update2 journey: Ignite and RunItTwice invite at their own gates, after Act I (and Ignite after its launch)");
	Expect(ScriptAsked && !ScriptLine.empty() && ScriptFigures && C.WarnMissed == 0 && C.ShortDeadlines.count(std::llround(ScriptRent)) == 1 && C.WarnEarliest >= 71.9 &&
			   ScriptLateRight && C.SiteRentBreaks == 0,
		"update2 journey: Ignite's money asked back before a rent the bank can't cover: the rent card warns from 72 hours out (what's on its way net of its fee, what "
		"sits on the site), and site money never pays the rent");
	Expect(TicketsLetter, "update2 journey: RunItTwice's tickets come with their letter (it carries money: never swallowed by the weekly rule)");
	Expect(RegisterWarned && RegisterStruck && CureDated && PublishWarned && PublishStruck && DealEnded,
		"update2 journey: the clause warns before the register button and Studio's PUBLISH, strikes on each (a cure date in Mail), and two strikes end the deal");
	Expect(B.Broken.empty() && M.Broken.empty(), ("update2 journey: the books hold after every step (" + (B.First.empty() ? M.First.empty() ? std::string("none broken") : M.First : B.First) + ")").c_str());
	Expect(RoundTrips >= 20 && RoundTripsSame && Update2Bytes <= 65 * 1024,
		"update2 journey: each week's save reads back the same; the update's non-world lines within the heavy save's 65 KB (SPEC 9)");
	// The copy pass on every letter the career got (the sites', RiverLine's, the clause's, Rerun's, the sponsors' and backers').
	const int Copy = seams::ScreenLetters("update2 journey: Mail", AllMail);
	Failures += Copy;
	std::printf("  update2 journey: the copy pass over %zu letters: %d problems\n", AllMail.size(), Copy);
	for (const auto& It : B.Broken)
	{
		std::printf("    broken %d times: %s\n", It.second, It.first.c_str());
	}
	for (const auto& It : M.Broken)
	{
		std::printf("    broken %d times: %s\n", It.second, It.first.c_str());
	}
}
} // namespace session_test

// ------------------------------------------------------------------ update 3's migration (update 3 SPEC §7.5)

namespace session_test
{
namespace migration3
{
/** Every committed fixture's name, sorted: Tests/fixtures' *.ssave and world_*.txt, by glob (a new fixture joins without a list). */
std::vector<std::string> Committed()
{
	namespace fs = std::filesystem;
	std::vector<std::string> Out;
	std::error_code Ec;
	for (fs::directory_iterator It(fs::path(SS_TESTS_DIR) / "fixtures", Ec), End; !Ec && It != End; It.increment(Ec))
	{
		const std::string Name = It->path().filename().string();
		const std::string Ext = It->path().extension().string();
		if (It->is_regular_file() && (Ext == ".ssave" || (Ext == ".txt" && Name.rfind("world_", 0) == 0)))
		{
			Out.push_back(Name);
		}
	}
	std::sort(Out.begin(), Out.end());
	return Out;
}

/** The bytes of Update 3's lines in a save's text: its site\t and video\t sub-tags and the fields appended to vid and cand
 *  (journeys::LineBytesU3), and the League's world line. Zero in any save Update 3 never touched (S10). */
size_t U3Bytes(const std::string& Text)
{
	return journeys::LineBytesU3(Text) + journeys::LineBytes(Text, {"world\tleague\t"});
}

/** A world's own text read a line at a time (World::Read takes a line's fields), then finished. */
void ReadWorldText(const std::string& Text, ss::world::World& W)
{
	size_t Start = 0;
	while (Start < Text.size())
	{
		const size_t End = std::min(Text.find('\n', Start), Text.size());
		std::vector<std::string> Fields(1);
		for (size_t K = Start; K < End; ++K)
		{
			if (Text[K] == '\t')
			{
				Fields.emplace_back();
			}
			else
			{
				Fields.back().push_back(Text[K]);
			}
		}
		W.Read(Fields);
		Start = End + 1;
	}
	W.Finish();
}
} // namespace migration3

/**
 * MigrationAll3 (update 3 SPEC §7.5; behind "session_test u3", which every C pass runs). Phase A's stub: every committed
 * fixture, found by glob and not by a list (Phase 0's 17 frozen snapshots: 6 pre_segment_* with v1's .expected, 5
 * pre_update2_*, 4 update2_*, world_v1.txt and world_v2.txt):
 *   - a save parses and writes back byte for byte (the v1 save as its .expected), and its text parses a second time the same;
 *   - it loads into a session whose save reads and writes back byte for byte, and that loads a second time identically;
 *   - a world reads, migrates once to version 4 and from then on writes back byte for byte;
 *   - and none of it carries an Update 3 line (a site\t play sub-tag, a video\t Update 3 line or appended field, world\tleague):
 *     Update 3 writes nothing into a save it never touched (S10).
 * Then the no-write case's core half: update2_multisite (the clause's save) stepped 40 days through the calendar, so across
 * a 1st, writes no Update 3 line and sets no Update3LinesRead bit. Opening every rival page through ss.Laptop.Open and the
 * Kast preview of each join it at C2, with the rivals' frames; update3_siteplay's facts at C2 and update3_creator's at C3.
 */
void MigrationAll3()
{
	const auto T0 = std::chrono::steady_clock::now();
	const std::vector<std::string> All = migration3::Committed();
	static const char* const Frozen[] = {"pre_segment_evicted.ssave", "pre_segment_fresh.ssave", "pre_segment_mid_nightly.ssave", "pre_segment_team_riverline.ssave",
		"pre_segment_v1.expected.ssave", "pre_segment_v1.ssave", "pre_update2_evicted.ssave", "pre_update2_fresh.ssave", "pre_update2_sponsored_streamer.ssave",
		"pre_update2_staked_mid_nightly.ssave", "pre_update2_veteran.ssave", "update2_auto_mid_nightly.ssave", "update2_blank_world.ssave", "update2_creator.ssave",
		"update2_multisite.ssave", "world_v1.txt", "world_v2.txt"};
	int Known = 0;
	for (const char* Name : Frozen)
	{
		Known += std::find(All.begin(), All.end(), std::string(Name)) != All.end() ? 1 : 0;
	}
	std::printf("  migration (update 3): %zu committed fixtures by glob, %d of Phase 0's 17 among them\n", All.size(), Known);
	Expect(Known == 17, "migration (update 3): the glob finds every committed fixture (Phase 0's 17 frozen snapshots)");
	int Exact = 0;
	for (const std::string& Name : All)
	{
		const auto F0 = std::chrono::steady_clock::now();
		const std::string Tag = "migration (update 3): " + Name;
		const std::string Saved = seams::Fixture(Name.c_str());
		if (Name.rfind("world_", 0) == 0)
		{
			ss::world::World W;
			migration3::ReadWorldText(Saved, W);
			std::string Again;
			W.Write(Again);
			ss::world::World Second;
			migration3::ReadWorldText(Again, Second);
			std::string Third;
			Second.Write(Third);
			const bool Kept = W.Ready() && !Saved.empty() && Again.rfind("world\thead\t" + std::to_string(ss::world::Version) + "\t", 0) == 0 && ss::world::Version == 4 && Third == Again;
			Exact += Kept ? 1 : 0;
			std::printf("  migration (update 3): %s: version %d -> %d, then written back %s; Update 3 bytes %zu (%.2f s)\n", Name.c_str(), migration2::WorldVersionIn(Saved),
				migration2::WorldVersionIn(Again), Third == Again ? "byte for byte" : ("differently at " + migration2::FirstDifference(Again, Third)).c_str(),
				migration3::U3Bytes(Again), migration2::Since(F0));
			Expect(Kept, (Tag + " reads, migrates once to version 4 and then writes back byte for byte").c_str());
			Expect(migration3::U3Bytes(Again) == 0, (Tag + ": no League line in a world Update 3 never touched").c_str());
			continue;
		}
		// A save that has an .expected beside it (the v1 "riverside" save) writes that back; every other one itself. The
		// written-back check is SaveFixtures' (seams::RoundTrip): byte for byte, or for a save from before the last segment
		// byte for byte but for that segment's own lines, which its migration writes; then a second parse writes the same.
		const std::string ExpectedName = Name.substr(0, Name.size() - std::string(".ssave").size()) + ".expected.ssave";
		const bool HasExpected = std::find(All.begin(), All.end(), ExpectedName) != All.end();
		const std::string Expected = HasExpected ? seams::Fixture(ExpectedName.c_str()) : Saved;
		seams::RoundTrip(Name.c_str(), Saved, Expected);
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(Saved, D);
		const std::string Again = Parsed ? D.Serialize() : std::string();
		if (!Parsed)
		{
			continue;
		}
		const std::string Seed = "migration3-" + Name;
		std::string First;
		{
			Hooks H;
			ss::Session S(H, Seed, &D);
			First = migration2::SavedNow(S, H);
		}
		ss::SaveData Back;
		const bool Reads = ss::SaveData::Parse(First, Back) && Back.Serialize() == First;
		std::string Second;
		{
			Hooks H;
			ss::Session S(H, Seed, &Back);
			Second = migration2::SavedNow(S, H);
		}
		const size_t U3 = migration3::U3Bytes(Again) + migration3::U3Bytes(First) + migration3::U3Bytes(Second);
		Exact += Again == Expected ? 1 : 0;
		std::printf("  migration (update 3): %s: written back %s; its session's save %zu bytes, %s; Update 3 bytes %zu (%.2f s)\n", Name.c_str(),
			Again == Expected ? "byte for byte" : "with the last segment's own lines (as SaveFixtures allows)", First.size(),
			Second == First ? "the same from a second load" : ("a second load differs at " + migration2::FirstDifference(First, Second)).c_str(), U3, migration2::Since(F0));
		Expect(Reads && Second == First, (Tag + ": its session's save reads and writes back byte for byte, and loads a second time identically").c_str());
		Expect(U3 == 0, (Tag + ": no Update 3 line written back or by its session (S10)").c_str());
	}
	// The no-write case's core half: the clause's save, 40 days of the calendar (ss.Stake.Advance's path: every step
	// CheckCalendar runs, SiteStep and SitePlayStep, VideoStep and the world's), so across a 1st.
	{
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(seams::Fixture("update2_multisite.ssave"), D);
		Hooks H;
		ss::Session S(H, "migration3-no-write", &D);
		const bool Clause = S.TeamRiverLine();
		const double From = S.WorldMinutes();
		const std::string Stepped = S.DebugStake("advance 960");
		const std::string After = migration2::SavedNow(S, H);
		const size_t U3 = migration3::U3Bytes(After);
		std::printf("  migration (update 3): update2_multisite stepped %.0f days under the clause: %s; Update 3 bytes %zu, Update3LinesRead %u\n",
			(S.WorldMinutes() - From) / ss::net::MinutesPerDay, Stepped.c_str(), U3, static_cast<unsigned>(S.Life.Update3LinesRead));
		Expect(Parsed && Clause && Stepped.rfind("Advanced", 0) == 0 && S.WorldMinutes() - From >= 40.0 * ss::net::MinutesPerDay - 1.0 && U3 == 0 &&
				   S.Life.Update3LinesRead == 0,
			"migration (update 3): the no-write case: update2_multisite's 40 days across a 1st write no Update 3 line and set no Update 3 bit");
	}
	std::printf("  migration (update 3): %d of %zu fixtures written back byte for byte (a world after its one migration; the rest but for the last segment's "
				"own lines), none with an Update 3 line, %.1f s\n",
		Exact, All.size(), migration2::Since(T0));
}
} // namespace session_test

int main(int Argc, char** Argv)
{
	// "session_test long": the long careers (EconomyPacing's three whole, with their deals-off twins, G2's creator career and G3's
	// multi-site career; SegmentJourney's whole 120 days; C3's Update2Journey; update 2 SPEC §6.7). "session_test c1 [long]":
	// only the lead's C1 checks (the gate's fixtures, the journeys, the pacing). "session_test c2" and "session_test c3": only
	// the lead's C2 or C3 checks (below). "session_test u3": update 3's (below).
	bool Long = false;
	bool C1 = false;
	bool C2 = false;
	bool G3 = false;
	bool C3 = false;
	bool U3 = false;
	for (int K = 1; K < Argc; ++K)
	{
		if (std::string(Argv[K]) == "kastcount")
		{
			// "kastcount": only StreamCountsChatters (also in the default run, after Streaming).
			session_test::StreamCountsChatters();
			std::printf("%s\n", session_test::Failures == 0 ? "session tests (kastcount): all passed" : "session tests (kastcount): FAILED");
			return session_test::Failures == 0 ? 0 : 1;
		}
		Long = Long || std::string(Argv[K]) == "long";
		C1 = C1 || std::string(Argv[K]) == "c1";
		C2 = C2 || std::string(Argv[K]) == "c2";
		G3 = G3 || std::string(Argv[K]) == "g3";
		C3 = C3 || std::string(Argv[K]) == "c3";
		U3 = U3 || std::string(Argv[K]) == "u3";
	}
	if (U3)
	{
		// "u3": update 3's lead checks (update 3 SPEC §7.5, §7.7), run by every C pass and kept out of the default run (no new
		// check there): MigrationAll3, Phase A's stub over every committed fixture; U3SitesJourney joins it at C2 and
		// U3CreatorJourney at C3.
		session_test::MigrationAll3();
		std::printf("%s\n", session_test::Failures == 0 ? "session tests (u3): all passed" : "session tests (u3): FAILED");
		return session_test::Failures == 0 ? 0 : 1;
	}
	if (C3)
	{
		// "c3 [fixtures] [migration] [pacing] [journey]": only the lead's C3 checks (update 2 SPEC §7.5 C3): the save fixtures with
		// the gate's own, MigrationAll2, EconomyPacing's multi-site career whole, Update2Journey; all four when none is named.
		std::set<std::string> Parts;
		for (int K = 1; K < Argc; ++K)
		{
			const std::string A = Argv[K];
			if (A == "fixtures" || A == "migration" || A == "pacing" || A == "journey")
			{
				Parts.insert(A);
			}
		}
		const bool All = Parts.empty();
		if (All || Parts.count("fixtures"))
		{
			session_test::seams::SaveFixtures();
			session_test::seams::Gate3Fixtures();
		}
		if (All || Parts.count("migration"))
		{
			session_test::MigrationAll2();
		}
		if (All || Parts.count("pacing"))
		{
			session_test::SitesPacing(true);
		}
		if (All || Parts.count("journey"))
		{
			session_test::Update2Journey();
		}
		std::printf("%s\n", session_test::Failures == 0 ? "session tests (c3): all passed" : "session tests (c3): FAILED");
		return session_test::Failures == 0 ? 0 : 1;
	}
	if (G3)
	{
		// "g3": only the lead's seams for the rival sites (update 2 B3; also in the default run, in SegLead), then every
		// committed save (MigrationAll2), which the sites' lines and the world's version 4 must keep.
		session_test::seams::RivalSites();
		session_test::MigrationAll2();
		std::printf("%s\n", session_test::Failures == 0 ? "session tests (g3): all passed" : "session tests (g3): FAILED");
		return session_test::Failures == 0 ? 0 : 1;
	}
	if (C2)
	{
		// "c2 [fixtures] [streams] [creator]": only the lead's C2 checks (the gate's fixture with SaveFixtures and MigrationAll,
		// the stream's links, EconomyPacing's creator career whole); all three when none is named.
		std::set<std::string> Parts;
		for (int K = 1; K < Argc; ++K)
		{
			const std::string A = Argv[K];
			if (A == "fixtures" || A == "streams" || A == "creator")
			{
				Parts.insert(A);
			}
		}
		const bool All = Parts.empty();
		if (All || Parts.count("fixtures"))
		{
			session_test::seams::Gate2Fixtures();
			session_test::MigrationAll();
		}
		if (All || Parts.count("streams"))
		{
			session_test::seams::StreamLinks();
		}
		if (All || Parts.count("creator"))
		{
			session_test::CreatorPacing(true);
		}
		std::printf("%s\n", session_test::Failures == 0 ? "session tests (c2): all passed" : "session tests (c2): FAILED");
		return session_test::Failures == 0 ? 0 : 1;
	}
	if (C1)
	{
		// "c1 [fixtures] [g1] [pacing] [segment] [long]": those parts only (all four when none is named).
		std::set<std::string> Parts;
		for (int K = 1; K < Argc; ++K)
		{
			const std::string A = Argv[K];
			if (A == "fixtures" || A == "g1" || A == "pacing" || A == "segment")
			{
				Parts.insert(A);
			}
		}
		const bool All = Parts.empty();
		if (All || Parts.count("fixtures"))
		{
			session_test::seams::Gate1Fixtures();
		}
		if (All || Parts.count("g1"))
		{
			session_test::G1Journey();
		}
		if (All || Parts.count("pacing"))
		{
			session_test::EconomyPacing(Long);
		}
		if (All || Parts.count("segment"))
		{
			session_test::SegmentJourney(Long);
		}
		std::printf("%s\n", session_test::Failures == 0 ? "session tests (c1): all passed" : "session tests (c1): FAILED");
		return session_test::Failures == 0 ? 0 : 1;
	}
	session_test::Formatting();
	session_test::NetworkChecks();
	session_test::ScheduledTournament();
	session_test::LifeChecks();
	session_test::DeeGame();
	session_test::Embercrest();
	session_test::EmbercrestCheckpoints();
	session_test::EmbercrestRail();
	session_test::EmbercrestMinimum();
	session_test::EmbercrestAftermath();
	session_test::BountyTournament();
	session_test::MultiTable();
	session_test::FullTournament();
	session_test::SprintTournament();
	session_test::DeepRuns();
	session_test::Streaming();
	session_test::StreamCountsChatters();
	session_test::StreamGrind();
	session_test::LedChecks();
	session_test::LivingWorld();
	session_test::CharacterChecks();
	session_test::StoreChecks();
	session_test::DeliveryChecks();
	session_test::DeliveryRules();
	session_test::NeedsPacing();
	session_test::RentChecks();
	session_test::NightOutChecks();
	// The next segment: the lead's Phase A checks (tests/session_seams.cpp), then the per-track additions
	// (next_segment_spec §7.6; tests/session_seg_<track>.cpp).
	session_test::SegLead();
	// Update 2: the lead's carried checks (update 2 SPEC §7.2, day 5), before the tracks'. From C3, MigrationAll2: MigrationAll
	// and every committed save into a version 4 world (EconomyPacing's multi-site career is in "long").
	session_test::EconomyPacing(Long);
	session_test::MigrationAll2();
	// Update 2 C1 (the lead): the journeys of gate 1 and the carried segment; C3's Update2Journey in "long".
	session_test::G1Journey();
	session_test::SegmentJourney(Long);
	if (Long)
	{
		session_test::Update2Journey();
	}
	session_test::SegStake();
	session_test::SegSponsor();
	session_test::SegTableUi();
	// Update 2 (update 2 SPEC §6.7): the per-track additions (tests/session_seg_<track>.cpp).
	session_test::SegAuto();
	session_test::SegVideo();
	session_test::SegSites();
	session_test::SegEcon();
	if (session_test::Failures == 0)
	{
		std::printf("session tests: all passed\n");
	}
	return session_test::Failures == 0 ? 0 : 1;
}
