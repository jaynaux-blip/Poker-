// ue-auto's rules for the plan in the hero's seat at the card room (update 2 SPEC §2.8, §2.12), in plain C++ over the core's
// types and nothing of Unreal's, so they can be run headless beside the core's tests: the read the room takes on the plan's
// bets (the engine's HeroReadHook, the same on the watch and the skip paths), the checkpoint host text's auto, autoseen and
// autoread lines, what a hand played through live::SkipSlice leaves behind (the tablemates, the HANDS page's record, a big
// pot remembered), Dee's per-hand clock, the live skip card's line and ss.Auto.Parity's hash. FAutoHost (BackRoomAuto.cpp)
// and the table's side (BackRoomTableAuto.cpp) call them. ue-auto's file; every name here is prefixed Auto.
#pragma once

#include "ShortStack/AI/Bot.h"
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Equity.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace BackRoomAutoRules
{
// ------------------------------------------------------------------ the read on the plan (§2.8)

/**
 * What the room reads off the plan's bets tonight (FAutoHost sets it once a night): how each player at the hero's tables
 * reads shaking hands (their persona's HeroRead, by tournament id; above 0 they read it right, below 0 they take nerves for
 * a bluff; strangers are 0 and aren't listed) and the player's heart as the night began (the table's resting rate and how
 * much of the pressure reaches it).
 */
struct AutoReadModel
{
	std::map<std::string, double> HeroRead;
	double RestBpm = 66.0;
	double Sensitivity = 1.0;
};

/** One read the room took on the plan (ss.Auto.Describe lists tonight's). */
struct AutoReadNote
{
	std::string Reader;
	int Tick = 0;
	double Shake = 0.0;
	bool Bluffing = false;
	bool Called = false; // the reader called where they'd have folded (else folded where they'd have called)
};

/** style::Mix's salt for the read's roll ("READ"). */
constexpr uint32_t AutoReadSalt = 0x52454144u;

/** The player's index in a hand's seats: the tournament's hero id, or "You" at Dee's game (the cast's names are the ids there). */
inline int AutoHeroIndex(const ss::Hand& H)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (H.Seats[I].Id == ss::HeroId || H.Seats[I].Id == "You")
		{
			return static_cast<int>(I);
		}
	}
	return -1;
}

/**
 * How shaken the player's hands looked pushing their last bet or raise of this street, 0..1, the way the table's heart has it
 * (ABackRoomTable::UpdateComposure): the pot against the stack they started the hand with, all in, a bluff, the plan's own
 * tilt and the moment (the final table, the bubble, hand for hand), through the night's resting rate and sensitivity. Bluff:
 * the table's own rule (FeltEquity: equity against random hands for everyone still in, under 0.42), drawn from a stream keyed
 * by the hand's state, never the tournament's R, so watching and skipping see the same. -1 when the player isn't the
 * street's last aggressor (nothing to read).
 */
inline double AutoPlanShake(const AutoReadModel& M, double Tilt, const ss::Tournament* T, const ss::Hand& H, int HeroIdx, bool& OutBluff)
{
	OutBluff = false;
	if (HeroIdx < 0 || HeroIdx >= static_cast<int>(H.Seats.size()))
	{
		return -1.0;
	}
	const ss::HandSeat& Me = H.Seats[static_cast<size_t>(HeroIdx)];
	if (Me.Folded || Me.Hole.size() < 2)
	{
		return -1.0;
	}
	// The street's last bet or raise, and what's gone in since (the pot as it stood when the player pushed theirs).
	int Last = -1;
	for (size_t I = 0; I < H.History.size(); ++I)
	{
		const ss::ActionRecord& A = H.History[I];
		if (A.OnStreet == H.CurrentStreet && (A.Type == ss::ActionType::Bet || A.Type == ss::ActionType::Raise))
		{
			Last = static_cast<int>(I);
		}
	}
	if (Last < 0 || H.History[static_cast<size_t>(Last)].Seat != Me.Seat)
	{
		return -1.0;
	}
	ss::Chips Since = 0;
	for (size_t I = static_cast<size_t>(Last) + 1; I < H.History.size(); ++I)
	{
		Since += H.History[I].Added;
	}
	const double PotAtBet = static_cast<double>(std::max<ss::Chips>(0, H.Pot() - Since));
	int InHand = 0;
	for (const ss::HandSeat& S : H.Seats)
	{
		InHand += S.Folded ? 0 : 1;
	}
	const int Others = std::max(1, InHand - 1);
	const std::vector<ss::RangeBand> Ranges(static_cast<size_t>(Others), ss::RangeBand{0.0, 1.0});
	char Key[96];
	std::snprintf(Key, sizeof(Key), "autoread:%016llx:%d:%d", static_cast<unsigned long long>(T ? T->SeedHash : 0ull), T ? T->Tick : 0, Last);
	ss::Rng R(Key);
	const double Eq = std::min(1.0, std::max(0.0, ss::EquityVsRanges(Me.Hole, H.Board, Ranges, 300, R) * (0.6 + 0.4 * static_cast<double>(Others))));
	OutBluff = Eq < 0.42;
	const double Extra = !T ? 0.0 : T->HandForHand() ? 14.0 : T->Tables.size() <= 1 ? 10.0 : (T->Remaining <= T->PaidPlaces() + 6 ? 6.0 : 0.0);
	const double Stack = static_cast<double>(std::max<ss::Chips>(1, Me.StartStack));
	double Pressure = 6.0 + 30.0 * std::min(PotAtBet / Stack, 1.5) / 1.5;
	Pressure += Me.AllIn ? 28.0 : 0.0;
	Pressure += OutBluff ? 18.0 : 0.0;
	Pressure += 20.0 * std::min(1.0, std::max(0.0, Tilt));
	Pressure += Extra;
	const double Rest = std::min(80.0, std::max(48.0, M.RestBpm));
	const double Bpm = std::min(168.0, std::max(Rest - 10.0, Rest + M.Sensitivity * Pressure));
	return std::min(1.0, std::max(0.0, (Bpm - 95.0) / 55.0));
}

/**
 * The room's read on the plan (Tournament::HeroReadHook's body, §2.8): ABackRoomTable::ReadHero with every random part made a
 * function of the hand. A player at the hero's table facing the plan's bet or raise of this street, who reads hands
 * (|HeroRead| at least 0.05), sees the shake (AutoPlanShake) with chance shake x |read| x 1.4 (at most 0.9), rolled with
 * style::Mix(seed, tick, seat, action, "READ"); the sharp ones read a bluff for what it is, the rest take nerves for one: a
 * fold becomes a call (with more than 12% equity) or a call a fold (under 60%). True when it changed D (Out says how).
 */
inline bool AutoApplyRead(const AutoReadModel& M, double Tilt, const ss::Tournament& T, const ss::Hand& H, int Seat, ss::BotDecision& D, AutoReadNote* Out)
{
	if (Seat < 0 || Seat >= static_cast<int>(H.Seats.size()))
	{
		return false;
	}
	const ss::HandSeat& Reader = H.Seats[static_cast<size_t>(Seat)];
	const auto It = M.HeroRead.find(Reader.Id);
	const double Read = It == M.HeroRead.end() ? 0.0 : It->second;
	if (std::fabs(Read) < 0.05 || H.CurrentBet - Reader.StreetBet <= 0)
	{
		return false;
	}
	const bool Fold = D.Action.Type == ss::PlayerAction::Kind::Fold;
	const bool Call = D.Action.Type == ss::PlayerAction::Kind::Call;
	if (!Fold && !Call)
	{
		return false;
	}
	bool Bluffing = false;
	const double Shake = AutoPlanShake(M, Tilt, &T, H, AutoHeroIndex(H), Bluffing);
	if (Shake < 0.08)
	{
		return false;
	}
	const double Roll = ss::style::Mix(T.SeedHash, T.Tick, Seat, static_cast<int>(H.History.size()), AutoReadSalt);
	if (Roll > std::min(0.9, std::max(0.0, Shake * std::fabs(Read) * 1.4)))
	{
		return false;
	}
	const bool ReadsWeak = Read > 0.0 ? Bluffing : true;
	bool Changed = false;
	if (ReadsWeak && Fold && D.Equity > 0.12)
	{
		D.Action = ss::PlayerAction::Call();
		Changed = true;
	}
	else if (!ReadsWeak && Call && D.Equity < 0.6)
	{
		D.Action = ss::PlayerAction::Fold();
		Changed = true;
	}
	if (Changed && Out)
	{
		const int Idx = T.PlayerIndex(Reader.Id);
		Out->Reader = Idx >= 0 ? T.Players[static_cast<size_t>(Idx)].Name : Reader.Id;
		Out->Tick = T.Tick;
		Out->Shake = Shake;
		Out->Bluffing = Bluffing;
		Out->Called = D.Action.Type == ss::PlayerAction::Kind::Call;
	}
	return Changed;
}

// ------------------------------------------------------------------ the read book's snapshot

/** The learned read book at seating (§2.4.3): Life.Reads' "Name/Tell" keys with 2 or more sightings, counted by name. */
inline ss::style::Learned AutoLearnedOf(const std::map<std::string, int>& Reads)
{
	ss::style::Learned Out;
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

// ------------------------------------------------------------------ the checkpoint host text (§2.12, §6.4)

/**
 * The plan's lines of the checkpoint host text: auto (autoplay::RunText: the plan and its run) when bWithRun, autoseen (a
 * player's 15 observed counts, reads::Write) for everyone observed tonight, autoread (a name and its learned tells) for the
 * read book the night seated with. Each ends in a newline.
 */
inline std::string AutoHostText(const ss::autoplay::TableAuto& A, const ss::autoplay::Run& R, const ss::style::Learned& Book, bool bWithRun)
{
	std::string Out;
	if (bWithRun)
	{
		Out += ss::autoplay::RunText(A, R) + "\n";
	}
	for (const auto& Kv : R.Seen)
	{
		if (!Kv.first.empty() && Kv.first.find('\t') == std::string::npos && Kv.first.find('\n') == std::string::npos)
		{
			Out += "autoseen\t" + Kv.first + "\t" + ss::reads::Write(Kv.second) + "\n";
		}
	}
	for (const auto& Kv : Book.Tells)
	{
		if (Kv.second > 0 && !Kv.first.empty() && Kv.first.find('\t') == std::string::npos && Kv.first.find('\n') == std::string::npos)
		{
			Out += "autoread\t" + Kv.first + "\t" + std::to_string(Kv.second) + "\n";
		}
	}
	return Out;
}

/** What one host-text line was, read back (AutoReadHostLine). */
enum class AutoHostLine : int
{
	None,
	Run,
	Seen,
	Read,
};

/** One host-text line split on tabs: the plan's own (read into A and R, R.Seen or Book) or None. */
inline AutoHostLine AutoReadHostLine(const std::vector<std::string>& F, ss::autoplay::TableAuto& A, ss::autoplay::Run& R, ss::style::Learned& Book)
{
	if (F.empty())
	{
		return AutoHostLine::None;
	}
	auto Join = [&F](size_t From) {
		std::string Text;
		for (size_t I = From; I < F.size(); ++I)
		{
			Text += (I > From ? "\t" : "") + F[I];
		}
		return Text;
	};
	if (F[0] == "auto" && F.size() >= 12)
	{
		ss::autoplay::TableAuto NewPlan;
		ss::autoplay::Run NewRun = R;
		if (ss::autoplay::ReadRun(Join(0), NewPlan, NewRun))
		{
			A = NewPlan;
			R = NewRun;
			return AutoHostLine::Run;
		}
		return AutoHostLine::None;
	}
	if (F[0] == "autoseen" && F.size() >= 17 && !F[1].empty())
	{
		ss::reads::Stats S;
		if (ss::reads::Parse(Join(2), S))
		{
			R.Seen[F[1]] = S;
			return AutoHostLine::Seen;
		}
		return AutoHostLine::None;
	}
	if (F[0] == "autoread" && F.size() >= 3 && !F[1].empty())
	{
		const int N = std::atoi(F[2].c_str());
		if (N > 0)
		{
			Book.Tells[F[1]] = N;
		}
		return AutoHostLine::Read;
	}
	return AutoHostLine::None;
}

// ------------------------------------------------------------------ a hand played through the live skip

/** Everyone the player sat with in a hand (their tournament ids), for the world's LiveMet. */
inline std::vector<std::string> AutoTablemates(const ss::Hand& H, int HeroIdx)
{
	std::vector<std::string> Out;
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		if (static_cast<int>(I) != HeroIdx && !H.Seats[I].Id.empty())
		{
			Out.push_back(H.Seats[I].Id);
		}
	}
	return Out;
}

/**
 * A hand the live skip played as the table card's HANDS page records it (the table's own EndHand rule): the player "You",
 * everyone else by the name the room says (talk::CaptionName: never an engine id), the hands turned up at the all-in and the
 * showdown once each. Number: the hand's number on the page.
 */
inline ss::recall::Record AutoSkippedRecord(const ss::Tournament& T, const ss::Hand& H, int HeroIdx, int Number)
{
	std::vector<std::string> Names;
	std::vector<int> Shown;
	const int Mine = HeroIdx >= 0 && HeroIdx < static_cast<int>(H.Seats.size()) ? H.Seats[static_cast<size_t>(HeroIdx)].Seat : -1;
	for (const ss::HandSeat& S : H.Seats)
	{
		if (S.Seat >= static_cast<int>(Names.size()))
		{
			Names.resize(static_cast<size_t>(S.Seat) + 1);
		}
		const int Idx = T.PlayerIndex(S.Id);
		const std::string Name = Idx >= 0 ? T.Players[static_cast<size_t>(Idx)].Name : std::string();
		Names[static_cast<size_t>(S.Seat)] = S.Seat == Mine ? std::string("You") : ss::talk::CaptionName(S.Id, Name, S.Seat);
	}
	for (const ss::HandEvent& E : H.Events)
	{
		if (E.Type == ss::EventType::Showdown)
		{
			for (const ss::ShowdownHand& SH : E.Hands)
			{
				Shown.push_back(SH.Seat);
			}
		}
		else if (E.Type == ss::EventType::Reveal)
		{
			Shown.insert(Shown.end(), E.Seats.begin(), E.Seats.end());
		}
	}
	std::sort(Shown.begin(), Shown.end());
	Shown.erase(std::unique(Shown.begin(), Shown.end()), Shown.end());
	return ss::recall::Of(H, Mine, Names, Shown, Number, T.LevelIndex + 1);
}

/**
 * Who was across a pot the player showed down (the table's FBackRoomShowdown rule: the one who lost the most to them, or won
 * the most from them), with the player's net and the pot. False when the player folded, nobody showed, or nothing changed.
 */
inline bool AutoShowdownAcross(const ss::Hand& H, int HeroIdx, std::string& OutId, ss::Chips& OutNet, ss::Chips& OutPot)
{
	bool Showdown = false;
	for (const ss::HandEvent& E : H.Events)
	{
		Showdown = Showdown || E.Type == ss::EventType::Showdown;
	}
	if (!Showdown || HeroIdx < 0 || HeroIdx >= static_cast<int>(H.Seats.size()))
	{
		return false;
	}
	const ss::HandSeat& Me = H.Seats[static_cast<size_t>(HeroIdx)];
	const ss::Chips Net = Me.Stack - Me.StartStack;
	if (Me.Folded || Net == 0)
	{
		return false;
	}
	ss::Chips Most = 0;
	OutId.clear();
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		const ss::HandSeat& S = H.Seats[I];
		const ss::Chips Theirs = static_cast<int>(I) != HeroIdx && !S.Folded ? S.Stack - S.StartStack : 0;
		if (Theirs * Net < 0 && std::llabs(static_cast<long long>(Theirs)) > Most)
		{
			Most = static_cast<ss::Chips>(std::llabs(static_cast<long long>(Theirs)));
			OutId = S.Id;
		}
	}
	OutNet = Net;
	OutPot = 0;
	for (const ss::PotResult& P : H.PotResults)
	{
		OutPot += P.Amount;
	}
	return !OutId.empty();
}

// ------------------------------------------------------------------ Dee's clock (§1.1, §2.6.3)

/**
 * The world minutes Dee's night moves this frame: while the plan holds the seat (bPerHand) only the hands it finished
 * (Pending, a fixed number of minutes each, added at each hand's end), never real time, so watching it at 4x never packs
 * more hands into a night; otherwise today's real-time minutes, with anything still pending. Pending is spent.
 */
inline double AutoDeeClock(bool bPerHand, double RealMinutes, double& Pending)
{
	const double Out = (bPerHand ? 0.0 : RealMinutes) + std::max(0.0, Pending);
	Pending = 0.0;
	return Out;
}

// ------------------------------------------------------------------ the skip card and the parity check

/** The live skip card's line under its header: "Level 9 · 31 left of 88 · you 42 BB · 16 to the bubble". */
inline std::string AutoSkipCardLine(const ss::Tournament& T, const ss::autoplay::TableAuto& A)
{
	const ss::Chips Bb = std::max<ss::Chips>(1, T.CurrentLevel().Bb);
	const long long YouBb = static_cast<long long>(std::llround(static_cast<double>(T.Hero().Stack) / static_cast<double>(Bb)));
	std::string Out = "Level " + std::to_string(T.LevelIndex + 1) + " \xC2\xB7 " + std::to_string(T.Remaining) + " left of " + std::to_string(T.Spec.Entrants) + " \xC2\xB7 you " +
		std::to_string(YouBb) + " BB";
	ss::autoplay::TableAuto Shown = A;
	Shown.M = ss::autoplay::Mode::Skip;
	std::string Progress = ss::autoplay::TileStatus(Shown, &T);
	const std::string Head = "SKIP \xC2\xB7 ";
	if (Progress.compare(0, Head.size(), Head) == 0)
	{
		Progress = Progress.substr(Head.size());
	}
	if (!Progress.empty() && A.To != ss::autoplay::Target::Result)
	{
		Out += " \xC2\xB7 " + Progress;
	}
	return Out;
}

/** FNV-1a over text (ss.Auto.Parity's hash, as the session's). */
inline uint64_t AutoHash(const std::string& Text)
{
	uint64_t H = 14695981039346656037ull;
	for (const char Ch : Text)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 1099511628211ull;
	}
	return H;
}

inline std::string AutoHex(uint64_t V)
{
	char Buf[24];
	std::snprintf(Buf, sizeof(Buf), "%016llx", static_cast<unsigned long long>(V));
	return Buf;
}

/** What ss.Auto.Parity hashes: the engine checkpoint, then the run as the host text carries it (Plan fixed by the caller). */
inline std::string AutoParityState(const ss::Tournament& T, const ss::autoplay::TableAuto& A, const ss::autoplay::Run& R)
{
	std::string Out = T.Checkpoint() + "\n" + ss::autoplay::RunText(A, R);
	for (const auto& Kv : R.Seen)
	{
		Out += "\nautoseen\t" + Kv.first + "\t" + ss::reads::Write(Kv.second);
	}
	return Out;
}
} // namespace BackRoomAutoRules
