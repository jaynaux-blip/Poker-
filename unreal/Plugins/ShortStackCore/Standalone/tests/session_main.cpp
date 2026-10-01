// Plays whole tournaments through the Night One session (the layer the Unreal
// client drives), with a scripted hero, and checks the flow end to end.
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"

#include <cstdio>
#include <map>

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
	Expect(S.LastResults.Hands > 20, "sprint simulated many hands");
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

int main()
{
	session_test::Formatting();
	session_test::NetworkChecks();
	session_test::ScheduledTournament();
	session_test::FullTournament();
	session_test::SprintTournament();
	session_test::DeepRuns();
	if (session_test::Failures == 0)
	{
		std::printf("session tests: all passed\n");
	}
	return session_test::Failures == 0 ? 0 : 1;
}
