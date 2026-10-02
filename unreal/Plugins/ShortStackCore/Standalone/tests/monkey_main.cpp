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
		{1250, 410}, {1382, 410}, {1514, 410}, {60, 981}, {170, 981}, {268, 981}, {360, 981}, {450, 981}, {800, 552}, {800, 600}, {800, 650}, {800, 700}};
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
	// GearDrop and Kast: the taskbar, the store's tabs, cards and order card, the studio's controls, chat, the channel page.
	const std::vector<Spot> Stream = {{470, 981}, {565, 981}, {60, 110}, {140, 110}, {235, 110}, {340, 110}, {460, 110}, {210, 436}, {480, 436}, {747, 436}, {1016, 436},
		{210, 790}, {480, 790}, {747, 790}, {1016, 790}, {887, 654}, {1080, 654}, {1340, 735}, {1513, 660}, {197, 30}, {283, 30}, {374, 30}, {130, 686}, {524, 547},
		{661, 686}, {697, 686}, {782, 698}, {880, 698}, {978, 698}, {110, 746}, {1085, 100}, {1160, 100}, {1230, 100}, {1530, 272}, {1530, 453}, {1530, 664},
		{1510, 917}, {1390, 213}, {898, 411}, {1028, 411}, {1040, 703}, {1240, 32}, {1335, 410}, {1520, 920}, {490, 30}, {130, 715}, {70, 763}, {202, 763}, {400, 763},
		{539, 763}, {741, 763}, {1010, 748}, {1252, 402}, {1333, 402}, {1414, 402}, {1508, 402}, {900, 286}, {900, 460}, {900, 634}, {1244, 223}, {846, 700}, {1201, 769}};
	H.insert(H.end(), Stream.begin(), Stream.end());
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
		// The rich seeds have the screens for four tables; the others start on the laptop's two.
		if (Seed % 3 == 2)
		{
			S.BankrollCents += 14900 + 28900;
			S.Buy("monitor-24");
			S.Buy("monitor-27");
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
			// Streaming: shop now and then (the PC upgrade first), go live and off again, work the studio.
			if (R.Chance(0.002))
			{
				// Only what the bankroll can spare (a quarter of it), so there's still money for buy-ins.
				const std::vector<ss::gear::Item>& Cat = ss::gear::Catalog();
				const ss::gear::Item& It = !S.GearFx().CanStream() && R.Chance(0.6) ? ss::gear::FirstPcUpgrade() : Cat[static_cast<size_t>(R.Int(static_cast<int>(Cat.size())))];
				if (It.PriceCents * 4 <= S.BankrollCents)
				{
					Reached["bought"] += S.Buy(It.Id).empty() ? 1 : 0;
				}
			}
			if (S.CurrentScreen != ss::Screen::Boot && R.Chance(0.0006))
			{
				const double Pick = R.Next();
				RL.OpenApp(Pick < 0.5 ? ss::ui::RiverLine::App::RiverLine : Pick < 0.75 ? ss::ui::RiverLine::App::Kast : ss::ui::RiverLine::App::GearDrop, Now);
			}
			if (!S.GearFx().CanStream() && S.Streaming())
			{
				Fail("streaming without the PC upgrade", Seed, F);
			}
			if (!S.Streaming() && R.Chance(0.002))
			{
				const std::string Why = S.GoLive();
				if (Why.empty() != S.GearFx().CanStream() && Why != "You're busy." && Why != "No apartment, no internet.")
				{
					Fail("going live doesn't follow the PC upgrade", Seed, F);
				}
				Reached["live"] += Why.empty() ? 1 : 0;
			}
			if (S.Streaming())
			{
				++Reached["streaming"];
				const ss::kast::Stream& St = S.Stream;
				if (!(St.Viewers >= 0.0 && St.Viewers < 1e7) || St.Health < 0.0 || St.Health > 1.0 || St.Hype < 0.0 || St.Hype > 100.0 || S.Channel.UnpaidCents < 0)
				{
					Fail("the stream's numbers left their ranges", Seed, F);
				}
				if (R.Chance(0.003))
				{
					S.StreamAd(R.Chance(0.5) ? 60 : 180);
				}
				if (R.Chance(0.004))
				{
					S.StreamThank();
				}
				if (!St.Chat.empty() && R.Chance(0.004))
				{
					const ss::kast::ChatMsg& Msg = St.Chat[static_cast<size_t>(R.Int(static_cast<int>(St.Chat.size())))];
					const int Id = Msg.Id;
					if (R.Chance(0.5))
					{
						S.StreamAnswer(Id);
					}
					else
					{
						S.StreamTimeout(Id);
					}
				}
				if (R.Chance(0.001))
				{
					const std::vector<std::string> Cand = St.ModCandidates(S.Channel, S.WorldMinutes());
					if (!Cand.empty())
					{
						S.StreamPromote(Cand.front());
					}
				}
				if (R.Chance(0.0005))
				{
					// Half the time, end it the way communities like: a raid on a small channel.
					const std::vector<ss::kast::SmallChannel> Net = ss::kast::Network(S.WorldMinutes());
					const bool Raid = !Net.empty() && R.Chance(0.5);
					const std::string Target = Raid ? Net[static_cast<size_t>(R.Int(static_cast<int>(Net.size())))].Name : std::string();
					S.EndStream(Target);
					++Reached[Raid ? "raided" : "ended"];
					if (Raid && (S.Stream.Last.RaidedOut != Target || S.Channel.Goodwill.count(Target) == 0))
					{
						Fail("a raid out didn't land", Seed, F);
					}
				}
			}
			// The LED kit: a colour, the power switch, sync.
			if (S.Owns(ss::gear::LedKitId) && R.Chance(0.0008))
			{
				const double Pick = R.Next();
				if (Pick < 0.6)
				{
					S.SetLedPreset(R.Int(ss::gear::LedPresetCount));
				}
				else if (Pick < 0.8)
				{
					S.SetLedsOn(!S.Leds.On);
				}
				else
				{
					S.SetLedSync(!S.Leds.Sync);
				}
				++Reached["leds"];
			}
			{
				const ss::gear::Glow G = S.RoomGlow(Now);
				const bool Should = S.Owns(ss::gear::LedKitId) && S.Leds.On;
				if (G.On != Should || G.On != S.GearFx().Leds || (G.On && (G.Level < 0.3 || G.Level > 2.0 || G.Preset != S.Leds.Preset)) || S.Leds.Preset < 0 || S.Leds.Preset >= ss::gear::LedPresetCount)
				{
					Fail("the room's LEDs don't match the kit and its settings", Seed, F);
				}
			}
			if (R.Chance(0.0003))
			{
				const int Days = R.Int(128);
				const int Start = R.Int(48) * 30;
				S.StreamSchedule(Days, Start);
				++Reached["schedule"];
			}
			{
				// The community stays in its ranges.
				const ss::kast::Channel& Ch = S.Channel;
				bool Bad = Ch.Members.size() > static_cast<size_t>(ss::kast::MaxMembers) || Ch.ScheduleDays < 0 || Ch.ScheduleDays > 127 || Ch.ScheduleStart < 0 || Ch.ScheduleStart >= 1440 ||
					Ch.Stage < 0 || Ch.Stage >= static_cast<int>(ss::kast::Stages().size()) || Ch.Xp < 0.0 || Ch.Followers < 0 || Ch.Superfans() > Ch.Regulars();
				for (const ss::kast::Member& Mb : Ch.Members)
				{
					Bad = Bad || !(Mb.Loyalty >= 0.0 && Mb.Loyalty <= 1.0) || !(Mb.Affinity >= 0.0 && Mb.Affinity <= 1.0) || Mb.Streams < 0;
				}
				if (Bad)
				{
					Fail("the community left its ranges", Seed, F);
				}
			}
			if (R.Chance(0.0005))
			{
				S.CashOut();
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
	if (Seeds >= 3 && Frames >= 15000 && (Reached["live"] == 0 || Reached["streaming"] == 0 || Reached["bought"] == 0))
	{
		++Failures;
		std::printf("FAIL: the monkey never bought gear and went live\n");
	}
	if (Seeds >= 3 && Frames >= 15000 && (Reached["table+4"] == 0 || Reached["results"] == 0 || Reached["registered"] == 0))
	{
		std::printf("FAIL: the monkey didn't reach four tables and a results screen\n");
		++Failures;
	}
	std::printf("%s: %d failures\n", Failures == 0 ? "monkey passed" : "MONKEY FAILED", Failures);
	return Failures == 0 ? 0 : 1;
}
