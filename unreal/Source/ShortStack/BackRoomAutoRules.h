// ue-auto's rules for the plan in the hero's seat at the card room (update 2 SPEC §2.8, §2.12), in plain C++ over the core's
// types and nothing of Unreal's, so they run headless in the core's card_room suite (tests/room_seg_autorules.cpp, C1): the
// heart's terms as ABackRoomTable::UpdateComposure adds them, and the read the room takes on the plan's bets off that heart
// (the engine's HeroReadHook, the same on the watch and the skip paths; the table's heart shows it), the checkpoint host
// text's auto, autoseen and autoread lines, what a hand played through live::SkipSlice leaves behind (the tablemates, the
// HANDS page's record and its number, a big pot remembered), Dee's per-hand clock, the live skip card's line and
// ss.Auto.Parity's hash. FAutoHost (BackRoomAuto.cpp) and the table (BackRoomTable.cpp, BackRoomTableAuto.cpp) call them.
// ue-auto's file; every name here is prefixed Auto.
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
	std::string Reader;   // the name the room knows them by
	std::string ReaderId; // their tournament id
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

// ------------------------------------------------------------------ the heart the room reads (§2.8, §2.12)

/**
 * The table's line between a bet with the goods and a bluff (ABackRoomTable's FeltEquity, the body's heart, and the read on the
 * plan all use it): the felt's equity, against random hands for everyone still in, under this is a bluff.
 */
constexpr double AutoBluffEquity = 0.42;

/** A bet without the goods: the felt's equity under the line. */
inline bool AutoIsBluff(double FeltEquity)
{
	return FeltEquity < AutoBluffEquity;
}

/**
 * What presses on the player's heart, term by term as ABackRoomTable::UpdateComposure adds them up (BackRoomTable.cpp): being in
 * the hand at all, the pot against the stack the hand began with, the price to call at the decision (and whether that price
 * was everything), all in, a bluff out there, eyes on the player, tilt and the moment.
 */
struct AutoHeartTerms
{
	double PotOverStack = 0.0;   // the pot against the stack the hand began with
	double PriceOverStack = 0.0; // the price to call at the decision against the stack then (0: nothing to call)
	bool CallAllIn = false;      // that price was all the player had
	bool AllIn = false;          // all in now
	bool Bluff = false;          // the street's last bet is a bluff
	bool Watched = false;        // somebody's eyes are on the player
	double Tilt = 0.0;           // 0..1
	double Extra = 0.0;          // the moment (AutoMomentPressure)
};

/** UpdateComposure's sum for a player in the hand: 6, +30 x pot/stack (to 1.5), +22 x the price (to half the stack), +28 for an
 *  all-in call, +28 all in, +18 a bluff, +8 watched, +20 x tilt, + the moment. */
inline double AutoHeartPressure(const AutoHeartTerms& In)
{
	double P = 6.0 + 30.0 * std::min(std::max(0.0, In.PotOverStack), 1.5) / 1.5;
	P += 22.0 * std::min(1.0, std::max(0.0, In.PriceOverStack * 2.0));
	P += In.CallAllIn ? 28.0 : 0.0;
	P += In.AllIn ? 28.0 : 0.0;
	P += In.Bluff ? 18.0 : 0.0;
	P += In.Watched ? 8.0 : 0.0;
	P += 20.0 * std::min(1.0, std::max(0.0, In.Tilt));
	P += In.Extra;
	return P;
}

/** Where a pressure settles the heart (UpdateComposure's target, no slow breaths): the night's resting rate (48 to 80) plus
 *  the share that reaches it, from 10 under rest to 168. */
inline double AutoHeartBpm(const AutoReadModel& M, double Pressure)
{
	const double Rest = std::min(80.0, std::max(48.0, M.RestBpm));
	return std::min(168.0, std::max(Rest - 10.0, Rest + M.Sensitivity * Pressure));
}

/** How much hands shake at a heart rate (FBackRoomComposure::Shake): none to 95, all of it at 150. */
inline double AutoShakeAt(double Bpm)
{
	return std::min(1.0, std::max(0.0, (Bpm - 95.0) / 55.0));
}

/** The moment's pressure (LiveTick's ExtraPressure): hand for hand 14, the final table 10, six from the money 6. */
inline double AutoMomentPressure(const ss::Tournament* T)
{
	return !T ? 0.0 : T->HandForHand() ? 14.0 : T->Tables.size() <= 1 ? 10.0 : (T->Remaining <= T->PaidPlaces() + 6 ? 6.0 : 0.0);
}

/** The index in H.History of this street's last bet or raise (-1: nobody has bet this street). */
inline int AutoLastAggression(const ss::Hand& H)
{
	int Last = -1;
	for (size_t I = 0; I < H.History.size(); ++I)
	{
		const ss::ActionRecord& A = H.History[I];
		if (A.OnStreet == H.CurrentStreet && (A.Type == ss::ActionType::Bet || A.Type == ss::ActionType::Raise))
		{
			Last = static_cast<int>(I);
		}
	}
	return Last;
}

/**
 * Eyes on the player, made a function of the table (the body's +8 is an opponent's stare, which the camera rolls): somebody at
 * the player's table who reads shaking hands (in the model, folded or not) is watching for it.
 */
inline bool AutoWatched(const AutoReadModel& M, const ss::Hand& H, int HeroIdx)
{
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		const auto It = static_cast<int>(I) == HeroIdx ? M.HeroRead.end() : M.HeroRead.find(H.Seats[I].Id);
		if (It != M.HeroRead.end() && std::fabs(It->second) >= 0.05)
		{
			return true;
		}
	}
	return false;
}

/**
 * The felt's equity behind the player's bet at History[Last] (FeltEquity's rule: against random hands for everyone still in
 * when it went in, scaled by how many), from a stream keyed by the night's seed, the tick and the bet, never the tournament's R,
 * so watching and skipping see the same, and so does every reader of that bet.
 */
inline double AutoPlanEquity(const ss::Tournament* T, const ss::Hand& H, int HeroIdx, int Last)
{
	if (HeroIdx < 0 || HeroIdx >= static_cast<int>(H.Seats.size()) || H.Seats[static_cast<size_t>(HeroIdx)].Hole.size() < 2)
	{
		return 0.5;
	}
	int InHand = 0;
	for (const ss::HandSeat& S : H.Seats)
	{
		InHand += S.Folded ? 0 : 1;
	}
	for (size_t I = static_cast<size_t>(std::max(0, Last + 1)); I < H.History.size(); ++I)
	{
		InHand += H.History[I].Type == ss::ActionType::Fold ? 1 : 0; // still in when the bet went in
	}
	const int Others = std::max(1, InHand - 1);
	const std::vector<ss::RangeBand> Ranges(static_cast<size_t>(Others), ss::RangeBand{0.0, 1.0});
	char Key[96];
	std::snprintf(Key, sizeof(Key), "autoread:%016llx:%d:%d", static_cast<unsigned long long>(T ? T->SeedHash : 0ull), T ? T->Tick : 0, Last);
	ss::Rng R(Key);
	const double Eq = ss::EquityVsRanges(H.Seats[static_cast<size_t>(HeroIdx)].Hole, H.Board, Ranges, 300, R) * (0.6 + 0.4 * static_cast<double>(Others));
	return std::min(1.0, std::max(0.0, Eq));
}

/** The player's heart as the room reads it at their last bet or raise of the street (AutoPlanHeartAt). */
struct AutoPlanHeart
{
	double Shake = -1.0; // 0..1; -1: nothing to read (the player isn't the street's last aggressor)
	double Bpm = 0.0;
	double Pressure = 0.0;
	double Equity = 0.0;
	bool Bluff = false;
	AutoHeartTerms Terms;
};

/**
 * How shaken the player's hands looked pushing their last bet or raise of this street, the way the table's heart has it
 * (UpdateComposure's terms, AutoHeartPressure) at the moment the bet went in: the pot then against the stack the hand began
 * with, the price they faced deciding it, all in, a bluff (AutoIsBluff of AutoPlanEquity), a reader at the table watching,
 * the plan's own tilt and the moment, through the night's resting rate and sensitivity. Every input is fixed once the bet is
 * out, so the shake holds for every reader of it and the HUD's heart can show it. Shake -1 when the player isn't the street's
 * last aggressor (nothing to read).
 */
inline AutoPlanHeart AutoPlanHeartAt(const AutoReadModel& M, double Tilt, const ss::Tournament* T, const ss::Hand& H, int HeroIdx)
{
	AutoPlanHeart Out;
	if (HeroIdx < 0 || HeroIdx >= static_cast<int>(H.Seats.size()))
	{
		return Out;
	}
	const ss::HandSeat& Me = H.Seats[static_cast<size_t>(HeroIdx)];
	const int Last = AutoLastAggression(H);
	if (Me.Folded || Me.Hole.size() < 2 || Last < 0 || H.History[static_cast<size_t>(Last)].Seat != Me.Seat)
	{
		return Out;
	}
	// What's gone in since the bet (the pot as it stood when the player pushed theirs), and the stack they decided it with.
	ss::Chips Since = 0;
	ss::Chips Mine = 0;
	for (size_t I = static_cast<size_t>(Last); I < H.History.size(); ++I)
	{
		Since += I > static_cast<size_t>(Last) ? H.History[I].Added : 0;
		Mine += H.History[I].Seat == Me.Seat ? H.History[I].Added : 0;
	}
	const ss::ActionRecord& Bet = H.History[static_cast<size_t>(Last)];
	const double Before = static_cast<double>(std::max<ss::Chips>(1, Me.Stack + Mine));
	Out.Equity = AutoPlanEquity(T, H, HeroIdx, Last);
	Out.Bluff = AutoIsBluff(Out.Equity);
	AutoHeartTerms& In = Out.Terms;
	In.PotOverStack = static_cast<double>(std::max<ss::Chips>(0, H.Pot() - Since)) / static_cast<double>(std::max<ss::Chips>(1, Me.StartStack));
	In.PriceOverStack = static_cast<double>(std::max<ss::Chips>(0, Bet.Facing)) / Before;
	In.CallAllIn = Bet.Facing > 0 && static_cast<double>(Bet.Facing) >= Before;
	In.AllIn = Me.AllIn;
	In.Bluff = Out.Bluff;
	In.Watched = AutoWatched(M, H, HeroIdx);
	In.Tilt = Tilt;
	In.Extra = AutoMomentPressure(T);
	Out.Pressure = AutoHeartPressure(In);
	Out.Bpm = AutoHeartBpm(M, Out.Pressure);
	Out.Shake = AutoShakeAt(Out.Bpm);
	return Out;
}

/** AutoPlanHeartAt's shake and bluff alone (-1: nothing to read). */
inline double AutoPlanShake(const AutoReadModel& M, double Tilt, const ss::Tournament* T, const ss::Hand& H, int HeroIdx, bool& OutBluff)
{
	const AutoPlanHeart P = AutoPlanHeartAt(M, Tilt, T, H, HeroIdx);
	OutBluff = P.Bluff;
	return P.Shake;
}

// ------------------------------------------------------------------ the read on the plan's bet (§2.8)

/** The least shake anyone reads (ReadHero's 0.08), and the least read a player has to have to look (|HeroRead| 0.05). */
constexpr double AutoReadMinShake = 0.08;
constexpr double AutoReadMinEye = 0.05;

/** The chance a reader sees it (ReadHero's): shakier hands and sharper eyes, more often; never more than 0.9. */
inline double AutoReadChance(double Shake, double Read)
{
	return std::min(0.9, std::max(0.0, Shake * std::fabs(Read) * 1.4));
}

/** What a reader takes the shake for (ReadHero's): the sharp ones (read above 0) see a bluff for a bluff and a bet with the
 *  goods for one; the rest think nerves mean a bluff, whatever it was. True: they read the player as weak. */
inline bool AutoReadsWeak(double Read, bool Bluffing)
{
	return Read > 0.0 ? Bluffing : true;
}

/** The roll on whether the reader at Seat sees it this time: style::Mix of the night's seed, the tick, the seat and the action. */
inline double AutoReadRoll(const ss::Tournament& T, const ss::Hand& H, int Seat)
{
	return ss::style::Mix(T.SeedHash, T.Tick, Seat, static_cast<int>(H.History.size()), AutoReadSalt);
}

/**
 * The room's read on the plan (Tournament::HeroReadHook's body, §2.8): ABackRoomTable::ReadHero with every random part made a
 * function of the hand. A player at the hero's table facing the plan's bet or raise of this street, who reads hands
 * (|HeroRead| at least AutoReadMinEye), sees the shake (AutoPlanHeartAt) with AutoReadChance, rolled with AutoReadRoll; what they
 * make of it is AutoReadsWeak: read as weak, a fold becomes a call (with more than 12% equity); read as strong, a call becomes a
 * fold (under 60%). True when it changed D (Out says how).
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
	if (std::fabs(Read) < AutoReadMinEye || H.CurrentBet - Reader.StreetBet <= 0)
	{
		return false;
	}
	const bool Fold = D.Action.Type == ss::PlayerAction::Kind::Fold;
	const bool Call = D.Action.Type == ss::PlayerAction::Kind::Call;
	if (!Fold && !Call)
	{
		return false;
	}
	const AutoPlanHeart Heart = AutoPlanHeartAt(M, Tilt, &T, H, AutoHeroIndex(H));
	if (Heart.Shake < AutoReadMinShake)
	{
		return false;
	}
	if (AutoReadRoll(T, H, Seat) > AutoReadChance(Heart.Shake, Read))
	{
		return false;
	}
	const bool ReadsWeak = AutoReadsWeak(Read, Heart.Bluff);
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
		Out->ReaderId = Reader.Id;
		Out->Tick = T.Tick;
		Out->Shake = Heart.Shake;
		Out->Bluffing = Heart.Bluff;
		Out->Called = D.Action.Type == ss::PlayerAction::Kind::Call;
	}
	return Changed;
}

// ------------------------------------------------------------------ the HANDS page's numbers

/**
 * The number the table card's HANDS page gives the next hand it keeps: the one asked for (the table's own count, or for a
 * skipped hand the count after it), or one past the highest it holds when that's taken. Hands the skip played and a night come
 * back to carry the count on, so no two hands on the page share a number.
 */
inline int AutoNextHandNumber(const std::vector<ss::recall::Record>& Kept, int Wanted)
{
	int Next = Wanted;
	for (const ss::recall::Record& R : Kept)
	{
		Next = std::max(Next, R.Number + 1);
	}
	return Next;
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
