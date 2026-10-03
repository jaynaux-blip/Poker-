// Plays whole tournaments through the Night One session (the layer the Unreal
// client drives), with a scripted hero, and checks the flow end to end.
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"

#include <cmath>
#include <algorithm>
#include <cstdio>
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

struct Hooks : ss::SessionHooks
{
	std::map<ss::SoundId, int> Sounds;
	int Texts = 0;
	int Cans = 0;
	int Beats = 0;
	int Saves = 0;
	ss::SaveData Last;
	void Sound(ss::SoundId Id, double) override { ++Sounds[Id]; }
	void Text(const std::string& From, const std::string& Body) override
	{
		++Texts;
		std::printf("  [phone] %s: %s\n", From.c_str(), Body.c_str());
	}
	void Heartbeat(bool On) override { Beats += On ? 1 : 0; }
	void AddCan() override { ++Cans; }
	int OnAirs = 0;
	int GearEvents = 0;
	void OnAir(bool On) override { OnAirs += On ? 1 : 0; }
	void GearChanged(const std::string&, bool) override { ++GearEvents; }
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
};

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
	int Stops = 0;
	double Now = 0.0;
	while (S.CurrentScreen == ss::Screen::Table && Stops < 10)
	{
		Now = PlayOut(S, Choice, Now, Decisions);
		break;
	}
	(void)Stops;
	Expect(S.CurrentScreen == ss::Screen::Results, "sprint tournament reaches results");
	std::printf("  freeroll with sprint: finished %s of %d after %d hands (%d hero decisions), cans %d\n",
		ss::Ordinal(S.LastResults.Place).c_str(), S.LastResults.Entrants, S.LastResults.Hands, Decisions, H.Cans);
	Expect(S.LastResults.Hands > 5 && Decisions == 0, "sprint plays the hands by itself");
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

/** The Riverside's card room: the schedule, registering once, the world's field, breaks, settling once, saves. */
void Riverside()
{
	namespace life = ss::life;
	namespace live = ss::live;
	const life::Activity* Rv = life::Find("riverside");
	Expect(Rv != nullptr && Rv->Type == life::Kind::Live, "the Riverside is in the catalog");
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
					Times = Times && O.Id == "riverside@" + std::to_string(Day);
				}
			}
		}
		Expect(Counts, "two events a day Monday to Thursday and Sunday, three on Friday and Saturday");
		Expect(Unique && Times, "every occurrence has its own id and its published start; the Sunday keeps riverside@<day>");
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
	ss::Session S(H, "riverside");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	// Wednesday, October 14 (world day 9), 5:30 PM: the Nightly's desk is open.
	const double Wednesday = 9.0 * 1440.0;
	S.LobbyMinutes = Wednesday - 1440.0 + 17.0 * 60.0 + 30.0;
	Now = Wait(S, Now, 0.2);
	S.Life.Energy = 60.0;
	const live::Occurrence Nightly = live::FindOccurrence("riverside-nightly@9");
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
	H.Went.clear();
	Expect(S.GoToLive(Nightly.Id).empty() && H.Went == Nightly.Id && S.BankrollCents == 50000 - 12000 - live::BusFareCents && S.Life.LiveEntries.size() == 1 &&
			   S.Life.Ledger.size() == Lines0 + 3,
		"registering again charges nothing and goes back");
	Expect(!S.GoToLive("riverside-turbo@11").empty() && S.Life.LiveEntries.size() == 1, "one live entry at a time");

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
	std::printf("  riverside: %s, %d entrants, %d hands, %d moves, %d breaks, done at %s\n", Nightly.T->Name, T->Spec.Entrants, Ticks, Moves, Breaks,
		ss::net::TimeLabel(T->ClockMinutes()).c_str());

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
				   P.Life.LiveEvents == 1,
			"entries survive a save");
	}

	// A thin noon game: below the minimum it's cancelled, and everything comes back once.
	{
		ss::Chips B = 50000;
		life::State L;
		const live::Occurrence Noon = live::FindOccurrence("riverside-noon@10");
		Expect(Noon.Valid() && live::Register(B, L, Noon, 10.0 * 1440.0 + 11.0 * 60.0, 12, {}).empty() && B == 50000 - 8000, "registered for Thursday's noon game");
		Expect(live::BelowMinimum(Noon, 12) && !live::BelowMinimum(Noon, 60), "twelve is below the room's minimum");
		Expect(live::Refund(B, L, Noon.Id, 10.0 * 1440.0 + 12.0 * 60.0, "not enough players") && !live::Refund(B, L, Noon.Id, 10.0 * 1440.0 + 12.0 * 60.0, "") && B == 50000 &&
				   L.Ledger.size() == 3 && L.LiveEvents == 0,
			"a cancelled event refunds once");
		Expect(!live::CanRegister(B, L, Noon, 10.0 * 1440.0 + 11.5 * 60.0).empty(), "a cancelled event can't be entered again");
	}

	// The game closed mid-tournament: the next load settles the entry by the rule for leaving, once.
	{
		ss::SaveData D;
		D.BankrollCents = 50000;
		const live::Occurrence Sunday = live::FindOccurrence("riverside@13");
		Expect(live::Register(D.BankrollCents, D.Life, Sunday, 13.0 * 1440.0 + 18.0 * 60.0, 95, {}).empty(), "registered for the Sunday");
		D.ClockMinutes = 14.0 * 1440.0 + 12.0 * 60.0 - 1440.0; // Monday noon
		Hooks H2;
		ss::Session S2(H2, "abandoned", &D);
		const life::LiveEntry* A = live::EntryFor(S2.Life, Sunday.Id);
		Expect(A && A->State == life::LiveEntry::Finished && A->Place >= 1 && A->Place <= 95 && S2.Life.LiveEvents == 1, "an entry left mid-tournament is blinded out and settled on the next load");
		if (A)
		{
			std::printf("  riverside: left the Sunday, blinded out in %s of 95\n", ss::Ordinal(A->Place).c_str());
		}
	}

	// Late registration: the field plays on without the player, whose stack waits untouched.
	{
		std::unique_ptr<ss::Tournament> Late = live::MakeField(live::FindOccurrence("riverside@13"), 90, "grinder_3c", "late", {});
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
		std::printf("  riverside: %d nights, the player dealt in every hand they were in\n", Nights);
	}
	// Walking away: dealt in, checking and folding, the blinds take the stack.
	{
		std::unique_ptr<ss::Tournament> Gone = live::MakeField(live::FindOccurrence("riverside-nightly@16"), 70, "grinder_3c", "gone", {});
		Gone->HeroSitsOut = true;
		int Ticks2 = 0;
		while (!Gone->Hero().Busted && !Gone->bFinished && Ticks2 < 6000)
		{
			Gone->SimulateTick();
			++Ticks2;
		}
		Expect(Gone->Hero().Busted && Gone->Hero().Place > 1 && Gone->Hero().VpipHands == 0, "a player who walks away is blinded off, never putting a chip in voluntarily");
		std::printf("  riverside: walked away, blinded off in %s after %d hands\n", ss::Ordinal(Gone->Hero().Place).c_str(), Gone->Hero().Hands);
	}
}

/** One line per event, to compare two runs of a night. */
std::string EventLine(const ss::TEvent& E)
{
	return std::to_string(static_cast<int>(E.Type)) + "|" + E.Id + "|" + E.Name + "|" + std::to_string(E.Place) + "|" + std::to_string(E.PrizeCents) + "|" + std::to_string(E.TableId) +
		"|" + std::to_string(E.From) + ">" + std::to_string(E.To) + "|" + std::to_string(E.LevelNumber) + "|" + E.EliminatedBy;
}

/** The Riverside's checkpoints: a night closed after any hand carries on exactly as if it never was. */
void RiversideCheckpoints()
{
	namespace life = ss::life;
	namespace live = ss::live;
	const live::Occurrence Sunday = live::FindOccurrence("riverside@13");
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
		std::printf("  riverside: %zu hands, reopened %d times, every hand and every chip where it was\n", Marks.size() - 1, Reopened);
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
		std::printf("  riverside: left at hand %d with %lld chips, blinded off in %s after %d more hands\n", Night->Tick, static_cast<long long>(Night->Hero().Stack),
			ss::Ordinal(Gone->Hero().Place).c_str(), Gone->Hero().Hands - StartHands);
	}
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
	Expect(S.StartActivity("quikstop").empty() && S.TimeSkip.Active, "a night shift at the Quik Stop starts at 2:07 AM");
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
	Expect(St.Chat.size() > 20 && St.Chatters() > 5, "chat talks through the tournament");
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

int main()
{
	session_test::Formatting();
	session_test::NetworkChecks();
	session_test::ScheduledTournament();
	session_test::LifeChecks();
	session_test::DeeGame();
	session_test::Riverside();
	session_test::RiversideCheckpoints();
	session_test::BountyTournament();
	session_test::MultiTable();
	session_test::FullTournament();
	session_test::SprintTournament();
	session_test::DeepRuns();
	session_test::Streaming();
	session_test::StreamGrind();
	session_test::LedChecks();
	session_test::LivingWorld();
	if (session_test::Failures == 0)
	{
		std::printf("session tests: all passed\n");
	}
	return session_test::Failures == 0 ? 0 : 1;
}
