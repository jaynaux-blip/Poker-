// Monkey test: drives the session and the RiverLine client with random clicks and keys across every screen, sits
// down at random events (up to four tables), and checks invariants every frame: the bankroll only moves through the
// ledger, tournament chips are conserved, the clock never runs backwards, tables never stall, saves round-trip.
// Usage: monkey_test [seeds] [frames] [first seed]
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/RiverLine.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
int Failures = 0;
std::map<std::string, int> FailCounts;

void Fail(const std::string& What, int Seed, int Frame)
{
	if (FailCounts[What]++ < 3)
	{
		std::printf("FAIL seed %d frame %d: %s\n", Seed, Frame, What.c_str());
	}
	++Failures;
}

struct Measurer : ss::ui::TextMeasurer
{
	float Width(const std::string& S, ss::ui::Font, float Size) const override { return static_cast<float>(S.size()) * Size * 0.55f; }
	float Ascent(ss::ui::Font, float Size) const override { return Size * 0.8f; }
};

struct Hooks : ss::SessionHooks
{
	ss::SaveData Last;
	bool HasSave = false;
	int Texts = 0;
	int GoOuts = 0;
	void Save(const ss::SaveData& D) override
	{
		Last = D;
		HasSave = true;
	}
	void Text(const std::string&, const std::string&) override { ++Texts; }
	bool GoOut(const std::string&, ss::Chips) override
	{
		++GoOuts;
		return false;
	}
};

struct Spot
{
	float X;
	float Y;
};

std::vector<Spot> Hotspots()
{
	std::vector<Spot> H = {{800, 550}, {270, 34}, {350, 34}, {500, 34}, {590, 34}, {690, 34}, {385, 259}, {1150, 539}, {1233, 539}, {1311, 539},
		{1300, 890}, {1200, 890}, {1450, 890}, {1330, 915}, {725, 895}, {905, 895}, {1085, 895}, {70, 892}, {166, 892}, {262, 892}, {89, 933}, {150, 837},
		{1250, 410}, {1382, 410}, {1514, 410}, {60, 981}, {170, 981}, {268, 981}, {360, 981}, {450, 981}, {800, 600}, {800, 650}, {800, 700}};
	for (int K = 0; K < 10; ++K)
	{
		H.push_back({46.0f + 82.0f * static_cast<float>(K), 330});
		H.push_back({300, 431.0f + 60.0f * static_cast<float>(K)});
	}
	for (int K = 0; K < 5; ++K)
	{
		H.push_back({673.0f + 72.0f * static_cast<float>(K), 803});
	}
	for (int K = 0; K < 8; ++K)
	{
		H.push_back({760.0f + 72.0f * static_cast<float>(K), 32}); // table tabs, +, tile toggle
	}
	for (int T = 0; T < 4; ++T)
	{
		const float Tx = 8.0f + static_cast<float>(T % 2) * 796.0f;
		const float Ty = 72.0f + static_cast<float>(T / 2) * 445.0f;
		for (int B = 0; B < 4; ++B)
		{
			H.push_back({Tx + 12.0f + 92.0f + static_cast<float>(B) * 193.0f, Ty + 407.0f});
		}
		H.push_back({Tx + 600.0f, Ty + 413.0f}); // pace chips
	}
	// App screens: a coarse grid.
	for (int Y = 120; Y < 960; Y += 70)
	{
		for (int X = 80; X < 1600; X += 160)
		{
			H.push_back({static_cast<float>(X), static_cast<float>(Y)});
		}
	}
	return H;
}

bool SameEntry(const ss::life::LedgerEntry& A, const ss::life::LedgerEntry& B)
{
	return A.At == B.At && A.Label == B.Label && A.Amount == B.Amount && A.Kind == B.Kind;
}
} // namespace

int main(int Argc, char** Argv)
{
	const int Seeds = Argc > 1 ? std::atoi(Argv[1]) : 3;
	const int Frames = Argc > 2 ? std::atoi(Argv[2]) : 15000;
	const int First = Argc > 3 ? std::atoi(Argv[3]) : 0;
	const std::vector<Spot> Spots = Hotspots();
	const char* Keys[] = {"f", "c", "x", "r", "b", "a", "ArrowUp", "ArrowDown"};
	Measurer M;
	double WorstMs = 0.0;
	std::string WorstWhere;
	long long TotalFrames = 0;
	std::map<std::string, int> Reached;
	for (int Seed = First; Seed < First + Seeds; ++Seed)
	{
		Hooks H;
		ss::Session S(H, "monkey-" + std::to_string(Seed));
		ss::ui::RiverLine RL(S);
		ss::Rng R("monkey-rng-" + std::to_string(Seed));
		if (Seed % 3 == 1)
		{
			S.BankrollCents = 5000;
		}
		if (Seed % 3 == 2)
		{
			S.BankrollCents = 60000;
			S.Life.Unlocks.insert("bounty");
			S.Life.Unlocks.insert("satellite");
			S.Life.Unlocks.insert("sixmax");
		}
		double Now = 0.0;
		ss::Chips PrevBank = S.BankrollCents;
		std::vector<ss::life::LedgerEntry> PrevLedger = S.Life.Ledger;
		double PrevWorld = S.WorldMinutes();
		bool Releasing = false;
		Spot Down{0, 0};
		std::map<const ss::Tournament*, std::pair<long long, double>> Progress; // signature, since
		for (int F = 0; F < Frames; ++F)
		{
			// Input.
			ss::ui::Pointer& P = RL.UI.Ptr;
			P.Pressed = false;
			P.Released = false;
			P.Wheel = 0.0f;
			P.Active = true;
			if (Releasing)
			{
				P.Down = false;
				P.Released = true;
				Releasing = false;
			}
			else
			{
				const double Roll = R.Next();
				if (Roll < 0.08)
				{
					const bool Hot = R.Chance(0.75);
					Down = Hot ? Spots[static_cast<size_t>(R.Int(static_cast<int>(Spots.size())))] : Spot{static_cast<float>(R.Next() * 1600.0), static_cast<float>(R.Next() * 1000.0)};
					Down.X += static_cast<float>(R.Next() * 8.0 - 4.0);
					Down.Y += static_cast<float>(R.Next() * 8.0 - 4.0);
					P.X = Down.X;
					P.Y = Down.Y;
					P.Down = true;
					P.Pressed = true;
					Releasing = true;
				}
				else if (Roll < 0.095)
				{
					RL.Key(Keys[R.Int(8)]);
				}
				else if (Roll < 0.10)
				{
					P.Wheel = static_cast<float>(R.Next() * 400.0 - 200.0);
				}
			}
			// Steering: now and then sit down in a random open event, or change a table's pace.
			// The last stretch winds the sitting down: no new tables, and the open ones sprint to their results.
			const bool Wrapping = F > Frames * 6 / 10;
			if (Wrapping && F % 300 == 0)
			{
				for (int I = 0; I < S.TableCount(); ++I)
				{
					S.WithTable(I, [&]() { S.CurrentPace = ss::Pace::Sprint; });
				}
			}
			if (!Wrapping && (S.CurrentScreen == ss::Screen::Lobby || S.CurrentScreen == ss::Screen::Table) && S.TableCount() < S.MaxTables() && R.Chance(0.003))
			{
				const ss::net::Network& Net = ss::net::Shared();
				const double W = S.WorldMinutes();
				std::vector<ss::LobbyEvent> Open;
				for (const ss::net::EventInstance& E : Net.Window(W - 240.0, W + 60.0))
				{
					const ss::LobbyEvent Lv = Net.Listing(E, nullptr, S.Unlocks());
					if (Lv.Joinable && S.CanAfford(Lv) && !S.IsPlaying(Lv.Spec.Id))
					{
						Open.push_back(Lv);
					}
				}
				if (!Open.empty())
				{
					S.RegisterEvent(Open[static_cast<size_t>(R.Int(static_cast<int>(Open.size())))]);
					++Reached["registered"];
				}
			}
			if (S.TableCount() > 0 && R.Chance(0.0008))
			{
				const int K = R.Int(S.TableCount());
				const ss::Pace Pc = static_cast<ss::Pace>(R.Int(3));
				S.WithTable(K, [&]() { S.CurrentPace = Pc; });
			}
			// A frame.
			Now += 1.0 / 30.0;
			const auto T0 = std::chrono::steady_clock::now();
			S.Update(Now);
			ss::ui::DrawList L;
			ss::ui::Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
			RL.Draw(C, Now);
			const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
			++TotalFrames;
			const char* ScreenNames[] = {"boot", "lobby", "table", "results"};
			std::string Where = std::string(ScreenNames[static_cast<int>(S.CurrentScreen)]) + (S.TableCount() > 1 ? (S.Tiled ? "/tiles" : "/multi") : "") +
				"/app" + std::to_string(static_cast<int>(RL.CurrentApp())) + "/page" + std::to_string(static_cast<int>(RL.CurrentPage()));
			++Reached[std::string(ScreenNames[static_cast<int>(S.CurrentScreen)]) + (S.TableCount() > 1 ? "+" + std::to_string(S.TableCount()) + (S.Tiled ? "tiled" : "") : "")];
			if (S.TimeSkip.Active)
			{
				++Reached["timeskip"];
			}
			if (Ms > WorstMs && F > 5)
			{
				WorstMs = Ms;
				WorstWhere = Where + " seed " + std::to_string(Seed);
			}

			// Invariants.
			if (S.BankrollCents < 0)
			{
				Fail("negative bankroll", Seed, F);
			}
			{
				// New entries: the shortest prefix after which the ledger continues with last frame's.
				const std::vector<ss::life::LedgerEntry>& Cur = S.Life.Ledger;
				size_t Fresh = Cur.size();
				if (Cur.size() < 60 && Cur.size() >= PrevLedger.size())
				{
					Fresh = Cur.size() - PrevLedger.size(); // not full yet: exact
				}
				for (size_t Nn = 0; Cur.size() >= 60 && Nn < Cur.size() && !PrevLedger.empty(); ++Nn)
				{
					bool Match = true;
					for (size_t I = 0; Nn + I < Cur.size() && I < PrevLedger.size() && Match; ++I)
					{
						Match = SameEntry(Cur[Nn + I], PrevLedger[I]);
					}
					if (Match)
					{
						Fresh = Nn;
						break;
					}
				}
				ss::Chips Logged = 0;
				for (size_t I = 0; I < Fresh; ++I)
				{
					Logged += Cur[I].Amount;
				}
				if (S.BankrollCents - PrevBank != Logged)
				{
					Fail("bankroll moved without the ledger (" + ss::Money(S.BankrollCents - PrevBank) + " vs " + ss::Money(Logged) + ")", Seed, F);
				}
				PrevBank = S.BankrollCents;
				PrevLedger = S.Life.Ledger;
			}
			const double World = S.WorldMinutes();
			if (World + 1e-6 < PrevWorld)
			{
				Fail("the clock ran backwards (" + ss::Fixed(PrevWorld - World, 3) + " min)", Seed, F);
			}
			PrevWorld = World;
			if (S.CurrentScreen == ss::Screen::Table && !S.T)
			{
				Fail("table screen without a tournament", Seed, F);
			}
			if (S.CurrentScreen == ss::Screen::Results && !S.HasResults)
			{
				Fail("results screen without results", Seed, F);
			}
			if (S.TableCount() > ss::Session::TableLimit || (S.TableCount() > 0 && (S.FocusedTable() < 0 || S.FocusedTable() >= S.TableCount())))
			{
				Fail("table count or focus out of range", Seed, F);
			}
			if ((S.TableCount() > 0) != (S.T != nullptr) && S.CurrentScreen != ss::Screen::Results)
			{
				Fail("open tables and the front tournament disagree", Seed, F);
			}
			std::set<std::string> Ids;
			{
				// Forget tournaments that closed (their addresses get reused).
				std::set<const ss::Tournament*> Open;
				for (int I = 0; I < S.TableCount(); ++I)
				{
					S.WithTable(I, [&]() { Open.insert(S.T.get()); });
				}
				for (auto It = Progress.begin(); It != Progress.end();)
				{
					It = Open.count(It->first) ? std::next(It) : Progress.erase(It);
				}
			}
			for (int I = 0; I < S.TableCount(); ++I)
			{
				S.WithTable(I, [&]() {
					if (!S.T)
					{
						Fail("an open table without a tournament", Seed, F);
						return;
					}
					if (!Ids.insert(S.T->Spec.Id).second)
					{
						Fail("two tables in one event", Seed, F);
					}
					if (S.HasPrompt && (!S.CurHand || S.CurHand->ToAct != S.HeroSeatIdx()))
					{
						Fail("a prompt when it isn't the player's turn", Seed, F);
					}
					ss::Chips Sum = 0;
					for (const ss::TPlayer* Pl : S.T->AlivePlayers())
					{
						Sum += Pl->Stack;
						if (Pl->Stack < 0)
						{
							Fail("negative stack", Seed, F);
						}
					}
					const ss::Chips Expected = static_cast<ss::Chips>(S.T->Players.size()) * S.T->Spec.StartingStack;
					if (Sum != Expected && !S.T->bFinished)
					{
						Fail("tournament chips not conserved", Seed, F);
					}
					for (const ss::SeatVis& V : S.Seats)
					{
						if (V.Present && (V.Stack < 0 || V.Bet < 0))
						{
							Fail("a seat shows negative chips", Seed, F);
						}
					}
					// Progress: a tournament that stops moving for two minutes of play is stuck.
					auto& Pr = Progress.emplace(S.T.get(), std::make_pair(-1LL, Now)).first->second;
					const long long Sig = static_cast<long long>(S.T->Tick) * 100000 + static_cast<long long>(S.HandProgress()) * 10 + (S.HasPrompt ? 1 : 0) +
						static_cast<long long>(S.Sprinting ? S.HandsPlayed : 0) * 1000000000LL;
					if (Pr.first != Sig || S.T->bFinished || S.Finishing() || S.CurrentScreen == ss::Screen::Results)
					{
						Pr = {Sig, Now};
					}
					else if (Now - Pr.second > 120.0)
					{
						std::string Hand = "none";
						if (S.CurHand)
						{
							Hand = std::string(S.CurHand->bComplete ? "complete" : "live") + " toact " + std::to_string(S.CurHand->ToAct) + " events " + std::to_string(S.CurHand->Events.size());
						}
						Fail("a table stopped moving (" + S.T->Spec.Name + ", screen " + std::to_string(static_cast<int>(S.CurrentScreen)) + ", table " + std::to_string(I) +
								", hand " + Hand + ", busted " + std::to_string(S.T->Hero().Busted) + ", left " + std::to_string(S.T->Remaining) + ", tables " +
								std::to_string(S.T->Tables.size()) + ", moving " + std::to_string(S.Moving) + ", start " + ss::Fixed(S.T->Spec.StartClock, 1) + ", clock " + ss::Fixed(S.T->ClockMinutes(), 1) + ")",
							Seed, F);
						Pr.second = Now;
					}
				});
			}
			if (F % 600 == 0 && H.HasSave)
			{
				ss::SaveData Back;
				if (!ss::SaveData::Parse(H.Last.Serialize(), Back) || Back.Serialize() != H.Last.Serialize())
				{
					Fail("a save doesn't survive a round trip", Seed, F);
				}
			}
		}
		std::printf("seed %d: %s, bankroll %s, %zu results, %d texts, rent %d paid, energy %.0f, %d tables open\n", Seed, ss::ClockString(S.ClockMinutes()).c_str(), ss::Money(S.BankrollCents).c_str(),
			S.History.size(), H.Texts, S.Life.RentsPaid, S.Life.Energy, S.TableCount());
	}
	std::printf("\nreached:");
	for (const auto& R : Reached)
	{
		std::printf(" %s=%d", R.first.c_str(), R.second);
	}
	std::printf("\nworst frame %.2f ms (%s), %lld frames\n", WorstMs, WorstWhere.c_str(), TotalFrames);
	if (Seeds >= 3 && Frames >= 15000 && (Reached["table+4"] == 0 || Reached["results"] == 0 || Reached["registered"] == 0))
	{
		std::printf("FAIL: the monkey didn't reach four tables and a results screen\n");
		++Failures;
	}
	std::printf("%s: %d failures\n", Failures == 0 ? "monkey passed" : "MONKEY FAILED", Failures);
	return Failures == 0 ? 0 : 1;
}
