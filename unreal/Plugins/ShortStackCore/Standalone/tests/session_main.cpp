// Plays whole tournaments through the Night One session (the layer the Unreal
// client drives), with a scripted hero, and checks the flow end to end.
#include "ShortStack/Game/Format.h"
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
	session_test::FullTournament();
	session_test::SprintTournament();
	session_test::DeepRuns();
	if (session_test::Failures == 0)
	{
		std::printf("session tests: all passed\n");
	}
	return session_test::Failures == 0 ? 0 : 1;
}
