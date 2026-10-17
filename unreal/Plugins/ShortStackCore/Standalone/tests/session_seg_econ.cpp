// night_one_session: core-econ's additions (update 2 SPEC §7.3). Compiled into session_test and called from main() in
// session_main.cpp (the kit is SessionSeg.h). core-econ's file; its helpers live in session_test::econ_seg.
//
// G1 (P0):
//   EconPace          Kast's strangers and follows by the watch speed (§1.5), on a stream of its own: 1x and 2x give the
//                     same stream to the byte; 4x a tenth fewer strangers and the follow chance of 1x; 8x and a skip
//                     today's Sprinting (strangers x0.8, follows x0.6), never both at once.
//   EconPaceSession   the session's own inputs (autoplay off, watching at 1x/2x/4x/8x, skipping) reach the stream with
//                     those factors.
//   EconFreshTalk     a fresh world's first month on the block (§3.3 F9, §3.5): the board up at the Embercrest blank and
//                     the first of each of the room's events still to come only while they are; the first winners up
//                     the hill, the first of each event, two wins, the room's points leader, the Season board's leader
//                     (from the ones who watch it) and the first Night Shift board, each naming the real result; the
//                     books in the first week, true of every background; nothing after day 30. Every line linted (U1's
//                     words, no brand, a line the card holds).
//   EconFreshYou      the same when the player is the one winning: the first name on the board, the first Noon
//                     Deepstack, two wins, the room's points and the Season board.
//   EconEstablished   an established world's block says none of it, at the same hours.
// G2 and G3 (VideoBuzz, the moment log, rep::Public's video and site fields) land with their gates.
#include "CopyLint.h"
#include "SessionSeg.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace session_test
{
namespace econ_seg
{
namespace kast = ss::kast;
namespace world = ss::world;

double EconSecondsSince(const std::chrono::steady_clock::time_point& T0)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - T0).count();
}

std::string EconLower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return S;
}

bool EconHas(const std::string& Line, const std::string& Part)
{
	return EconLower(Line).find(EconLower(Part)) != std::string::npos;
}

// ------------------------------------------------------------------ Kast by the watch speed (§1.5)

/** A stream's inputs at a table with a mid rig, at nine on a Thursday night. */
kast::Inputs EconBaseInputs()
{
	ss::gear::Owned Own;
	for (const char* Id : {"tower-mid", "fiber", "webcam-1080", "mic-usb", "ring-light", "overlay-pack", "monitor-24"})
	{
		Own[Id] = 0.0;
	}
	kast::Inputs In;
	In.Hero = "grinder_3c";
	In.Gear = ss::gear::Sum(Own);
	In.AtTable = true;
	In.Tables = 1;
	In.EventName = "Night Owl";
	In.World = 3.0 * ss::net::MinutesPerDay + 21.0 * 60.0;
	In.Real = 0.0;
	return In;
}

struct EconStreamRun
{
	double First = 0.0;     // strangers watching after the first tick
	double Start = 0.0;     // strangers watching before it
	kast::Summary Sum;
	int Followers = 0;
	int Members = 0;
	std::vector<float> Graph;
	std::string Chat;       // the last chat lines, as written
};

/**
 * One stream of Minutes world minutes from In (its speed and Sprinting as given), ticked every Step world minutes (a real
 * second each), with the same all-in moments at the same ticks whatever the speed. Grown: a channel a few months in
 * (level 20, an eight-week streak), so a night brings enough strangers to count follows by.
 */
EconStreamRun EconStream(const std::string& Seed, kast::Inputs In, double Minutes, double Step = 0.25 / 6.0, bool Grown = false)
{
	EconStreamRun Out;
	kast::Channel Ch;
	if (Grown)
	{
		Ch.Xp = kast::Channel::LevelXp(20);
	}
	kast::Stream St(Seed);
	ss::Rng Ev(Seed + ":moments");
	St.Start(Ch, In);
	if (Grown)
	{
		Ch.Streak = 8;
	}
	Out.Start = St.Strangers;
	const double End = In.World + Minutes - 1e-9;
	const int Every = std::max(1, static_cast<int>(std::lround(0.75 / Step)));
	int Tick = 0;
	while (In.World < End)
	{
		In.World += Step;
		In.Real += 1.0;
		St.Tick(Ch, In, Step, 1.0);
		if (++Tick == 1)
		{
			Out.First = St.Strangers;
		}
		if (Tick % Every == 0 && Ev.Chance(0.05))
		{
			St.OnMoment(Ch, In, kast::Moment::AllIn);
			St.OnMoment(Ch, In, Ev.Chance(0.6) ? kast::Moment::WonAllIn : kast::Moment::LostAllIn);
		}
		St.Notices.clear();
	}
	for (const kast::ChatMsg& M : St.Chat)
	{
		Out.Chat += M.Who + ": " + M.Text + "\n";
	}
	Out.Graph = St.Graph;
	Out.Sum = St.Stop(Ch, In);
	Out.Followers = Ch.Followers;
	Out.Members = static_cast<int>(Ch.Members.size());
	return Out;
}

kast::Inputs EconAt(kast::Inputs In, int Speed, bool Sprinting)
{
	In.WatchSpeed = Speed;
	In.Sprinting = Sprinting;
	return In;
}

/** The strangers watching after a fresh stream's first tick on In (its clock set to the base inputs'). */
double EconFirstStrangers(kast::Inputs In)
{
	const kast::Inputs Base = EconBaseInputs();
	In.World = Base.World;
	In.Real = Base.Real;
	return EconStream("econ-first", In, 0.25 / 6.0).First;
}

bool EconNear(double A, double B, double Tolerance = 1e-9)
{
	return std::fabs(A - B) <= Tolerance * std::max(1.0, std::fabs(B));
}

void EconPace()
{
	const kast::Inputs Base = EconBaseInputs();
	// The first tick: the same stream up to the strangers, so their ratio is the factor itself.
	const EconStreamRun One = EconStream("econ-pace", EconAt(Base, 1, false), 0.25 / 6.0);
	const EconStreamRun Four = EconStream("econ-pace", EconAt(Base, 4, false), 0.25 / 6.0);
	const EconStreamRun Eight = EconStream("econ-pace", EconAt(Base, 8, true), 0.25 / 6.0);
	const EconStreamRun EightOnly = EconStream("econ-pace", EconAt(Base, 8, false), 0.25 / 6.0);
	const EconStreamRun Skip = EconStream("econ-pace", EconAt(Base, 1, true), 0.25 / 6.0);
	std::printf("  econ: first-tick strangers 1x %.6f, 4x %.6f, 8x %.6f, 8x (speed only) %.6f, skip %.6f\n", One.First, Four.First, Eight.First, EightOnly.First, Skip.First);
	Expect(One.Start == 0.0 && One.First > 0.0, "econ pace: a stream starts with no strangers and the directory brings some on the first tick");
	Expect(EconNear(Four.First, 0.9 * One.First), "econ pace: at 4x the directory sends nine strangers in ten");
	Expect(EconNear(Eight.First, 0.8 * One.First) && EconNear(Skip.First, 0.8 * One.First),
		"econ pace: 8x and a skip are today's Sprinting, eight in ten (and 8x isn't thinned twice)");
	Expect(EconNear(EightOnly.First, Eight.First), "econ pace: 8x reads as Sprinting even when only the speed says so");

	// 1x and 2x: the same stream, tick for tick (viewers, chat, follows, money).
	bool Same = true;
	for (int Seed = 0; Seed < 2; ++Seed)
	{
		const EconStreamRun A = EconStream("econ-same-" + std::to_string(Seed), EconAt(Base, 1, false), 150.0);
		const EconStreamRun B = EconStream("econ-same-" + std::to_string(Seed), EconAt(Base, 2, false), 150.0);
		Same = Same && A.Sum.NewFaces == B.Sum.NewFaces && A.Sum.Follows == B.Sum.Follows && A.Sum.Avg == B.Sum.Avg && A.Sum.Peak == B.Sum.Peak &&
			A.Sum.Subs == B.Sum.Subs && A.Sum.Total() == B.Sum.Total() && A.Sum.Returning == B.Sum.Returning && A.Followers == B.Followers &&
			A.Members == B.Members && A.Graph == B.Graph && A.Chat == B.Chat && A.Sum.Xp == B.Sum.Xp && A.Sum.NewFaces > 0;
	}
	Expect(Same, "econ pace: at 2x the stream is the 1x stream to the byte (viewers, chat, follows, money)");

	// Whole nights on a grown channel over several seeds: 4x thins the strangers and keeps the follow chance; 8x and a
	// skip thin both as Sprint did.
	// The fifth: 8x when only the speed says so (no Sprinting flag) thins the follows exactly as 8x with it.
	double Faces[5] = {0, 0, 0, 0, 0};
	double Follows[5] = {0, 0, 0, 0, 0};
	const int Speeds[5] = {1, 4, 8, 1, 8};
	const bool Sprints[5] = {false, false, true, true, false};
	for (int Seed = 0; Seed < 16; ++Seed)
	{
		for (int K = 0; K < 5; ++K)
		{
			const EconStreamRun R = EconStream("econ-pace-" + std::to_string(Seed), EconAt(Base, Speeds[K], Sprints[K]), 480.0, 0.5, true);
			Faces[K] += R.Sum.NewFaces;
			Follows[K] += R.Sum.Follows;
		}
	}
	const double Face4 = Faces[1] / Faces[0];
	const double Face8 = Faces[2] / Faces[0];
	const double FaceSkip = Faces[3] / Faces[0];
	const double Per1 = Follows[0] / std::max(1.0, Faces[0]);
	const double Per4 = Follows[1] / std::max(1.0, Faces[1]);
	const double Per8 = Follows[2] / std::max(1.0, Faces[2]);
	const double PerSkip = Follows[3] / std::max(1.0, Faces[3]);
	std::printf("  econ: sixteen 8-hour streams: new faces 1x %.0f, 4x %.3f, 8x %.3f, skip %.3f of 1x; follows %.0f/%.0f/%.0f/%.0f, per new face 1x %.4f, 4x %.4f, 8x %.4f, skip %.4f\n",
		Faces[0], Face4, Face8, FaceSkip, Follows[0], Follows[1], Follows[2], Follows[3], Per1, Per4, Per8, PerSkip);
	Expect(Face4 > 0.84 && Face4 < 0.95, "econ pace: over a night at 4x about a tenth fewer strangers drop in");
	Expect(Face8 > 0.72 && Face8 < 0.86 && FaceSkip == Face8, "econ pace: at 8x and on a skip about a fifth fewer, the same as each other");
	Expect(Follows[0] >= 80.0 && Per4 > 0.88 * Per1 && Per4 < 1.12 * Per1, "econ pace: at 4x the ones who stay follow as often as at 1x");
	Expect(Per8 > 0.35 * Per1 && Per8 < 0.75 * Per1 && PerSkip == Per8, "econ pace: at 8x and on a skip they follow about 40% less often (Sprinting)");
	Expect(Faces[4] == Faces[2] && Follows[4] == Follows[2], "econ pace: 8x on the speed alone is the Sprinting stream, follows and all");
}

ss::autoplay::TableAuto EconPlan(ss::autoplay::Mode M, int Speed)
{
	ss::autoplay::TableAuto A;
	A.S = ss::style::Style::Gto;
	A.M = M;
	A.Speed = Speed;
	A.To = ss::autoplay::Target::Result;
	A.Stops = 0;
	return A;
}

void EconPaceSession()
{
	// The session's own inputs: autoplay off, watching at each speed, then skipping; the stream reads each with its factor.
	Hooks H;
	ss::Session S(H, "econ-pace-session");
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	S.Register(1);
	Expect(Rig && S.StreamInputs().Gear.CanStream() && S.T, "econ pace: a rig that streams, at a table");
	const double Off = EconFirstStrangers(S.StreamInputs());
	std::map<int, double> Watched;
	for (const int Speed : {1, 2, 4, 8})
	{
		S.SetAutoplay(0, EconPlan(ss::autoplay::Mode::Watch, Speed));
		Watched[Speed] = EconFirstStrangers(S.StreamInputs()) / Off;
	}
	S.SprintBudgetMs = 0.0;
	S.SetAutoplay(0, EconPlan(ss::autoplay::Mode::Skip, 1));
	S.SkipAhead(0, ss::autoplay::Target::Result);
	double Now = 0.0;
	for (int F = 0; F < 20000 && !S.Sprinting && !S.Finishing(); ++F)
	{
		Now += 0.1;
		S.Update(Now);
	}
	const double Skipped = S.Sprinting ? EconFirstStrangers(S.StreamInputs()) / Off : -1.0;
	std::printf("  econ: the session's inputs, strangers against autoplay off: 1x %.3f, 2x %.3f, 4x %.3f, 8x %.3f, skipping %.3f\n", Watched[1], Watched[2], Watched[4], Watched[8],
		Skipped);
	Expect(Off > 0.0 && EconNear(Watched[1], 1.0) && EconNear(Watched[2], 1.0), "econ pace: watching at 1x and 2x, the stream's strangers are as with autoplay off");
	Expect(EconNear(Watched[4], 0.9) && EconNear(Watched[8], 0.8), "econ pace: watching at 4x x0.9, at 8x x0.8 (the session's inputs)");
	Expect(EconNear(Skipped, 0.8), "econ pace: a skip x0.8, as Sprint's");
}

// ------------------------------------------------------------------ a fresh world's first month on the block (§3.3 F9)

/** What someone on the block said: the key it spent, the line, who said it, and when. */
struct EconSaid
{
	std::string Key;
	std::string Line;
	std::string Speaker;
	double At = 0.0;
};

/** Moves a session's clock (and its world) forward to World, as the town tests do. */
void EconClock(ss::Session& S, double World)
{
	S.LobbyMinutes = World - ss::net::MinutesPerDay * ss::net::NightOneDay;
	S.Living().AdvanceTo(World);
}

/**
 * Everything Speaker has to tell now ("What's new?" until they've nothing new), heard. Keep: the session remembers it
 * (the block doesn't repeat it); otherwise what was said is forgotten after, so the next look sees the same news.
 */
std::vector<EconSaid> EconNews(ss::Session& S, const std::string& Speaker, bool Keep)
{
	const std::map<std::string, int> Said = S.Life.Said;
	std::vector<EconSaid> Out;
	for (int K = 0; K < 24; ++K)
	{
		const ss::town::Talk T = ss::town::Answer(*ss::town::ContextFor(S, Speaker, ""), "news");
		if (T.SaidKey.empty())
		{
			break;
		}
		Out.push_back({T.SaidKey, T.Line, Speaker, S.WorldMinutes()});
		S.Heard(Speaker, T);
	}
	if (!Keep)
	{
		S.Life.Said = Said;
	}
	return Out;
}

const EconSaid* EconFind(const std::vector<EconSaid>& All, const std::string& KeyPrefix)
{
	for (const EconSaid& E : All)
	{
		if (E.Key.rfind(KeyPrefix, 0) == 0)
		{
			return &E;
		}
	}
	return nullptr;
}

/** The first place of one of the room's events, as the world has it: its name (the player's for -1), or "". */
std::string EconWinner(const ss::Session& S, const std::string& EventId, int* Player = nullptr)
{
	const ss::net::EventResult* R = S.Living().ResultOf(EventId);
	if (!R || R->FinalTable.empty())
	{
		return "";
	}
	const ss::net::Placing& P = R->FinalTable.front();
	if (Player)
	{
		*Player = P.Player;
	}
	const world::Npc* N = S.Living().Get(P.Player);
	return P.Player == -1 ? S.HeroName : N ? N->Name : P.Name;
}

/**
 * Whether Line names Name: as it is, or as the room says it (talk::CaptionName's rule: a whole name or one the room knows
 * by it as it is, else the first name in a handle).
 */
bool EconNames(const std::string& Line, const std::string& Name)
{
	if (Line.find(Name) != std::string::npos)
	{
		return true;
	}
	const std::string First = ss::talk::KnownByName(Name) || Name.find(' ') != std::string::npos ? std::string() : ss::talk::FirstNameIn(Name);
	return !First.empty() && Line.find(First) != std::string::npos;
}

/** What's wrong with a line the block says (U1's words, a brand, the card's 140 characters, the lint), or "". */
std::string EconLint(const std::string& Line)
{
	const std::string Why = copylint::Check(Line, 140, {});
	if (!Why.empty())
	{
		return Why;
	}
	const std::string Low = " " + EconLower(Line) + " ";
	for (const char* Word : {"ai", "bot", "bots", "autoplay"})
	{
		for (size_t At = Low.find(Word); At != std::string::npos; At = Low.find(Word, At + 1))
		{
			const auto Letter = [](char Ch) { return (Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9'); };
			if (!Letter(Low[At - 1]) && !Letter(Low[At + std::string(Word).size()]))
			{
				return std::string("says \"") + Word + "\" (U1)";
			}
		}
	}
	for (const char* Real : {"pokerstars", "ggpoker", "partypoker", "888", "wsop", "world series", "wpt", "youtube", "tiktok", "vimeo", "skrill", "neteller", "paypal", "venmo",
			 "winamax", "unibet", "coinpoker", "betonline", "bovada", "ignition", "natural8", "red bull"})
	{
		if (Low.find(Real) != std::string::npos)
		{
			return std::string("a real brand: ") + Real;
		}
	}
	return "";
}

/** The board's top row's name now (the player's for -1), or "". */
std::string EconBoardTop(const ss::Session& S, ss::net::Board B)
{
	const ss::net::HeroStats Hero = ss::net::StatsFrom(S.HeroName, S.History, S.WorldMinutes());
	const std::vector<ss::net::BoardRow> Rows = ss::net::Shared().Leaderboard(B, S.WorldMinutes(), Hero, 1, nullptr);
	if (Rows.empty() || Rows.front().Value <= 0.0)
	{
		return "";
	}
	const std::vector<ss::net::Player>& Players = ss::net::Shared().Players();
	const int P = Rows.front().Player;
	return P == -1 ? S.HeroName : P >= 0 && P < static_cast<int>(Players.size()) ? Players[static_cast<size_t>(P)].Name : std::string();
}

/** The room's points leader this month (the player's name for -1), or "". */
std::string EconRoomLeader(const ss::Session& S)
{
	const std::vector<std::pair<int, double>> L = S.Living().RoomLeaders("embercrest", world::MonthKeyOf(ss::net::DayOf(S.WorldMinutes())), 1);
	if (L.empty() || L.front().second <= 0.0)
	{
		return "";
	}
	const world::Npc* N = S.Living().Get(L.front().first);
	return L.front().first == -1 ? S.HeroName : N ? N->Name : std::string();
}

/** The block's people who follow the game, where they are when asked. */
const char* const EconSpeakers[] = {"sal", "mei", "big-lou", "twitch", "mrs-park", "lonnie", "ray", "benny", "marlene", "augie"};

ss::hero::Character EconWho()
{
	ss::hero::Character Who;
	Who.FirstName = "Jesse";
	Who.LastName = "Cole";
	Who.Created = true;
	return Who;
}

/** A month of the block's talk, from Night One: what everyone says, kept, at two hours a day (and Night One's first). */
struct EconMonth
{
	std::vector<EconSaid> Said;
	int Problems = 0;
};

/**
 * The checks every fresh line must pass when it's said: linted, and each claim true of the world right then (the
 * winners named are the results' first places, the boards' leaders are on top, the counts are the counts).
 */
void EconCheckLine(ss::Session& S, const EconSaid& E, EconMonth& M)
{
	auto Bad = [&](const std::string& Why) {
		++M.Problems;
		std::printf("  econ FRESH LINE (%s at %.0f, %s): \"%s\": %s\n", E.Speaker.c_str(), E.At, E.Key.c_str(), E.Line.c_str(), Why.c_str());
	};
	const std::string Lint = EconLint(E.Line);
	if (!Lint.empty())
	{
		Bad(Lint);
	}
	const world::World& W = S.Living();
	const int Fresh = ss::net::DayOf(E.At) - ss::net::DayOf(W.StartedAt());
	const bool InBooks = W.Began() == world::Start::Blank && Fresh >= 0 && Fresh < 30;
	const bool FreshLine = E.Key.rfind("fresh:", 0) == 0 || EconHas(E.Line, " books") || EconHas(E.Line, "first name");
	if (FreshLine && !InBooks)
	{
		Bad("a fresh world's first-month line outside one");
	}
	if (E.Key.rfind("rumour:winner:", 0) == 0)
	{
		const std::string Id = E.Key.substr(14);
		const std::string Who = EconWinner(S, Id);
		if (Who.empty() || (!EconNames(E.Line, Who) && !EconHas(E.Line, "you")))
		{
			Bad("names someone other than the winner (" + Who + ")");
		}
		const ss::live::Occurrence O = ss::live::FindOccurrence(Id);
		if (O.Valid() && E.Line.find(O.T->Short) == std::string::npos && !EconHas(E.Line, "first name on the embercrest"))
		{
			Bad(std::string("doesn't name the event (") + O.T->Short + ")");
		}
	}
	if (E.Key.rfind("fresh:wall-blank:", 0) == 0)
	{
		int Wins = W.HeroRoom().Wins;
		for (const world::Npc& N : W.People())
		{
			Wins += N.Room.Wins;
		}
		if (Wins > 0)
		{
			Bad("the board isn't blank");
		}
	}
	if (E.Key.rfind("fresh:first-nightly:", 0) == 0)
	{
		for (int D = ss::net::DayOf(W.StartedAt()) - 1; D <= ss::net::DayOf(E.At); ++D)
		{
			const std::string Id = "embercrest-nightly@" + std::to_string(D);
			if (!EconWinner(S, Id).empty())
			{
				Bad("a Nightly's been won (" + Id + ")");
			}
		}
	}
	if (E.Key.rfind("fresh:twice:", 0) == 0)
	{
		const std::string Rest = E.Key.substr(12);
		const size_t Colon = Rest.find(':');
		const std::string Who = Rest.substr(0, Colon);
		const int Count = std::atoi(Rest.substr(Colon + 1).c_str());
		const int Has = Who == "you" ? W.HeroRoom().Wins : W.Get(std::atoi(Who.c_str())) ? W.Get(std::atoi(Who.c_str()))->Room.Wins : -1;
		const world::Npc* N = Who == "you" ? nullptr : W.Get(std::atoi(Who.c_str()));
		if (Has != Count || Count < 2 || (N && !EconNames(E.Line, N->Name)))
		{
			Bad("the wins counted aren't theirs (" + std::to_string(Has) + ")");
		}
	}
	if (E.Key.rfind("fresh:no-repeat:", 0) == 0)
	{
		int Total = W.HeroRoom().Wins;
		int Most = W.HeroRoom().Wins;
		for (const world::Npc& N : W.People())
		{
			Total += N.Room.Wins;
			Most = std::max(Most, N.Room.Wins);
		}
		if (Most > 1 || Total < 3)
		{
			Bad("somebody has won twice, or too few have won to say it");
		}
	}
	if (E.Key.rfind("fresh:leader:", 0) == 0)
	{
		const std::string Who = EconRoomLeader(S);
		if (Who.empty() || (Who == S.HeroName ? !EconHas(E.Line, "you're top") : !EconNames(E.Line, Who)))
		{
			Bad("isn't the room's points leader (" + Who + ")");
		}
	}
	if (E.Key.rfind("fresh:season:", 0) == 0)
	{
		const std::string Who = EconBoardTop(S, ss::net::Board::Season);
		if (Who.empty() || (Who == S.HeroName ? !EconHas(E.Line, "you're on top") && !EconHas(E.Line, "you're top") : E.Line.find(Who) == std::string::npos))
		{
			Bad("isn't the Season board's leader (" + Who + ")");
		}
	}
	if (E.Key.rfind("rumour:nightshift:", 0) == 0 && InBooks && !EconHas(E.Line, "you're on top"))
	{
		const std::string Who = EconBoardTop(S, ss::net::Board::NightShift);
		const bool FirstShift = ss::life::NightShiftStart(E.At) <= W.StartedAt();
		if (EconHas(E.Line, "first night shift board") != FirstShift)
		{
			Bad("calls the shift the first in the books when it isn't, or the other way round");
		}
		if (EconHas(E.Line, "again") || EconHas(E.Line, "I was winning"))
		{
			const ss::net::HeroStats Hero = ss::net::StatsFrom(S.HeroName, S.History, E.At);
			const std::vector<ss::net::BoardRow> Last = ss::net::Shared().Leaderboard(ss::net::Board::NightShift, ss::life::NightShiftStart(E.At) - 1.0, ss::net::StatsFrom(S.HeroName, S.History, ss::life::NightShiftStart(E.At) - 1.0), 1, nullptr);
			const std::vector<ss::net::BoardRow> Now = ss::net::Shared().Leaderboard(ss::net::Board::NightShift, E.At, Hero, 1, nullptr);
			if (FirstShift || Last.empty() || Now.empty() || Last.front().Player != Now.front().Player || EconHas(E.Line, "I was winning"))
			{
				Bad("says \"again\" (or claims a lead) the board can't show");
			}
		}
		if (!Who.empty() && Who != S.HeroName && E.Line.find(Who) == std::string::npos)
		{
			Bad("isn't the Night Shift's leader (" + Who + ")");
		}
	}
	if (E.Key.rfind("fresh:books:", 0) == 0 && Fresh >= 7)
	{
		Bad("the books' first-week line after the first week");
	}
	if (E.Key.rfind("fresh:first-next:", 0) == 0 || E.Key.rfind("fresh:first-nightly:", 0) == 0)
	{
		// The first of its kind still to come: this one isn't decided, and none of its kind has been won in the books.
		const std::string Id = E.Key.substr(E.Key.find(':', 6) + 1);
		const ss::live::Occurrence O = ss::live::FindOccurrence(Id);
		bool HeroWon = false;
		for (const ss::life::LiveEntry& L : S.Life.LiveEntries)
		{
			HeroWon = HeroWon || (L.Id == Id && L.State == ss::life::LiveEntry::Finished && L.Place == 1);
		}
		if (!O.Valid() || !EconWinner(S, Id).empty() || HeroWon)
		{
			Bad("the event is already decided (" + Id + ")");
		}
		else
		{
			for (int D = ss::net::DayOf(W.StartedAt()) - 1; D <= ss::net::DayOf(E.At); ++D)
			{
				for (const ss::live::Occurrence& P : ss::live::Occurrences(D))
				{
					if (P.Id != Id && std::string(P.T->Key) == O.T->Key && P.Start + P.T->Hours * 60.0 > W.StartedAt() && !EconWinner(S, P.Id).empty())
					{
						Bad("a " + std::string(O.T->Short) + " has already been won in the books (" + P.Id + ")");
					}
				}
			}
			if (E.Line.find(O.T->Short) == std::string::npos)
			{
				Bad(std::string("doesn't name the event (") + O.T->Short + ")");
			}
		}
	}
	if (E.Key.rfind("fresh:season:", 0) == 0 && E.Speaker != "mei" && E.Speaker != "twitch" && E.Speaker != "benny")
	{
		Bad("reads out RiverLine's Season board without being one of the block's who watch it");
	}
	if (FreshLine && (EconHas(E.Line, "got to town") || EconHas(E.Line, "got here") || EconHas(E.Line, "new in town")))
	{
		Bad("tells the player they just arrived (a dealer's kid grew up in the Embercrest's break room)");
	}
	if (E.Key.rfind("fresh:hill:", 0) == 0 && W.HeroRoom().Events > 0)
	{
		Bad("says the player hasn't been up the hill when they have");
	}
}

/** Talks to the block at the hours a month's checks look at, keeping what's said, and checks every line as it's said. */
EconMonth EconWalkMonth(ss::Session& S, int Days, const std::vector<std::pair<int, double>>& Extra = {})
{
	EconMonth M;
	const double Opened = S.Living().StartedAt();
	const int First = ss::net::DayOf(Opened);
	std::vector<double> Times = {Opened + 3.0, First * 1440.0 + 3.5 * 60.0};
	for (int D = First; D < First + Days; ++D)
	{
		for (const double H : {10.0, 15.0, 22.5})
		{
			Times.push_back(D * 1440.0 + H * 60.0);
		}
	}
	for (const std::pair<int, double>& X : Extra)
	{
		Times.push_back(X.first * 1440.0 + X.second * 60.0);
	}
	std::sort(Times.begin(), Times.end());
	for (const double T : Times)
	{
		if (T <= S.WorldMinutes())
		{
			continue;
		}
		EconClock(S, T);
		for (const char* Sp : EconSpeakers)
		{
			for (const EconSaid& E : EconNews(S, Sp, true))
			{
				EconCheckLine(S, E, M);
				M.Said.push_back(E);
			}
		}
	}
	return M;
}

void EconFreshTalk()
{
	const auto T0 = std::chrono::steady_clock::now();
	Hooks H;
	ss::Session S(H, "econ-fresh");
	S.NewCareer(EconWho(), world::Start::Blank);
	const world::World& W = S.Living();
	const double Opened = W.StartedAt();
	Expect(W.Began() == world::Start::Blank && S.WorldMinutes() == Opened, "econ fresh: a new career in a fresh start opens the books on Night One");

	// Night One, minutes in: the board up the hill is blank and the first Nightly in the books is being dealt (Monday's,
	// which ends after the books opened). Nobody's on a board, so nobody's named.
	EconClock(S, Opened + 2.0);
	const std::vector<EconSaid> Early = EconNews(S, "sal", false);
	const EconSaid* Blank = EconFind(Early, "fresh:wall-blank:");
	const EconSaid* Nightly = EconFind(Early, "fresh:first-nightly:embercrest-nightly@0");
	const EconSaid* Books = EconFind(Early, "fresh:books:sal");
	for (const EconSaid& E : Early)
	{
		std::printf("  econ: Night One, Sal: [%s] \"%s\"\n", E.Key.c_str(), E.Line.c_str());
	}
	Expect(Blank && Blank->Line == "Board up at the Embercrest's empty. First name on it, people remember.", "econ fresh: Sal on the blank board up the hill");
	Expect(Nightly && EconHas(Nightly->Line, "first Nightly in the books is on right now"), "econ fresh: the first Nightly in the books, being dealt now");
	Expect(Books && Books->Line == "New books. Everybody's even. Lasts about a week, then the good ones pull away.", "econ fresh: Sal on the new books");
	Expect(!EconFind(Early, "fresh:season:") && !EconFind(Early, "rumour:winner:"), "econ fresh: on Night One nobody's won anything to name");

	// The month, from the start: every line said is checked as it's said (EconCheckLine).
	EconMonth M = EconWalkMonth(S, 31);
	std::map<std::string, int> Topics;
	std::set<std::string> Lines;
	for (const EconSaid& E : M.Said)
	{
		std::string Topic = E.Key.substr(0, E.Key.find(':', E.Key.find(':') + 1));
		if (E.Key.rfind("rumour:winner:", 0) == 0)
		{
			Topic = EconHas(E.Line, "first name") ? "winner:first-wall" : EconHas(E.Line, " books") ? "winner:first" : "winner";
		}
		if (E.Key.rfind("rumour:nightshift:", 0) == 0)
		{
			Topic = EconHas(E.Line, "first night shift board") ? "nightshift:first" : "nightshift";
		}
		++Topics[Topic];
		if (E.Key.rfind("fresh:", 0) == 0 || Topic.rfind("winner:", 0) == 0 || Topic == "nightshift:first")
		{
			Lines.insert(E.Line);
		}
	}
	std::printf("  econ: a fresh world's month on the block: %zu lines said, %zu distinct fresh lines; by topic:", M.Said.size(), Lines.size());
	for (const auto& T : Topics)
	{
		std::printf(" %s %d", T.first.c_str(), T.second);
	}
	std::printf("\n");
	for (const std::string& L : Lines)
	{
		std::printf("    \"%s\"\n", L.c_str());
	}
	Expect(M.Problems == 0, "econ fresh: every line of the month is linted and true when it's said");
	Expect(Topics["winner:first-wall"] == 1, "econ fresh: one first name up the hill, named for the event that put it there");
	Expect(Topics["winner:first"] >= 4, "econ fresh: the firsts of the room's other events in the books (Noon, Turbo, Big Stack, Warm-Up, Sunday)");
	Expect(Topics["fresh:twice"] >= 1 && Topics["fresh:no-repeat"] >= 1 && Topics["fresh:leader"] >= 20 && Topics["fresh:season"] >= 20,
		"econ fresh: nobody twice yet, then two wins, the room's points leader and the Season board's, through the month");
	Expect(Topics["fresh:books"] >= 5 && Topics["fresh:hill"] >= 3 && Topics["nightshift:first"] >= 1 && Topics["fresh:wall-blank"] == 1 && Topics["fresh:first-nightly"] == 1,
		"econ fresh: the books in the first week, the hill once a week, the first Night Shift board; the blank board and the first Nightly once, on Night One");
	Expect(Topics["fresh:first-next"] >= 4 && Topics["fresh:first-next"] <= 8 && Topics["fresh:no-repeat"] <= 5,
		"econ fresh: who takes the first of the room's other events, through the first week; nobody twice yet at most once a week");
	int After = 0;
	int FreshAfter = 0;
	for (const EconSaid& E : M.Said)
	{
		const int Day = ss::net::DayOf(E.At) - ss::net::DayOf(Opened);
		After += Day >= 30 ? 1 : 0;
		FreshAfter += Day >= 30 && (E.Key.rfind("fresh:", 0) == 0 || EconHas(E.Line, " books") || EconHas(E.Line, "first name")) ? 1 : 0;
	}
	Expect(After > 0 && FreshAfter == 0, "econ fresh: after the first thirty days the block talks as it does in any world");
	std::printf("  econ: fresh talk %.1f s\n", EconSecondsSince(T0));
}

void EconFreshYou()
{
	// The player winning up the hill in a fresh world: Monday's Nightly (being dealt when the books open) and Tuesday's
	// Noon Deepstack, told into the world before they're decided.
	Hooks H;
	ss::Session S(H, "econ-fresh-you");
	S.NewCareer(EconWho(), world::Start::Blank);
	world::World& W = S.Living();
	const double Opened = W.StartedAt();
	EconClock(S, Opened + 2.0);
	for (const char* Id : {"embercrest-nightly@0", "embercrest-noon@1"})
	{
		const ss::live::Occurrence O = ss::live::FindOccurrence(Id);
		W.EnsurePlanned(O.Day);
		W.HeroEntered(Id, O.T->Name);
		W.HeroFinished(Id, 1, 250000, world::World::TableReport());
		// The card room settles the player's entry at once; the world tells the result at the schedule's end. Until then
		// nobody on the block asks who takes the first one (EconCheckLine).
		ss::life::LiveEntry L;
		L.Id = Id;
		L.Name = O.T->Name;
		L.State = ss::life::LiveEntry::Finished;
		L.Place = 1;
		L.PrizeCents = 250000;
		L.FinishedAt = Opened + 2.0;
		S.Life.LiveEntries.push_back(L);
	}
	// A big online win on the Season board.
	ss::HistoryEntry Big;
	Big.Name = "Night Owl Turbo";
	Big.Place = 1;
	Big.Entrants = 1800;
	Big.Prize = 450000;
	Big.BuyInCents = 2200;
	S.History.push_back(Big);
	EconMonth M = EconWalkMonth(S, 3);
	std::map<std::string, std::string> Got;
	for (const EconSaid& E : M.Said)
	{
		if (E.Key.rfind("rumour:winner:embercrest-nightly@0", 0) == 0 || E.Key.rfind("rumour:winner:embercrest-noon@1", 0) == 0 || E.Key.rfind("fresh:twice:you", 0) == 0 ||
			E.Key.rfind("fresh:leader:", 0) == 0 || E.Key.rfind("fresh:season:", 0) == 0)
		{
			const std::string K = E.Key.substr(0, E.Key.rfind(':'));
			if (!Got.count(K))
			{
				Got[K] = E.Line;
				std::printf("  econ: the player's month: [%s] \"%s\"\n", E.Key.c_str(), E.Line.c_str());
			}
		}
	}
	Expect(M.Problems == 0, "econ fresh (the player): every line linted and true");
	Expect(EconWinner(S, "embercrest-nightly@0") == S.HeroName, "econ fresh (the player): the player's wins are in the world's results");
	Expect(EconFind(M.Said, "rumour:winner:embercrest-nightly@0") &&
			   EconFind(M.Said, "rumour:winner:embercrest-nightly@0")->Line == "First name on the Embercrest's board, and it's yours. Don't let it go to your head.",
		"econ fresh (the player): the first name on the board is the player's");
	Expect(EconFind(M.Said, "rumour:winner:embercrest-noon@1") &&
			   EconFind(M.Said, "rumour:winner:embercrest-noon@1")->Line == "You took the first Noon Deepstack in the books? They'll be telling that one a while.",
		"econ fresh (the player): the first Noon Deepstack in the books is the player's");
	Expect(EconFind(M.Said, "fresh:twice:you:2") && EconFind(M.Said, "fresh:twice:you:2")->Line == "Two wins up the hill already, and the books aren't a month old. Easy now.",
		"econ fresh (the player): two wins up the hill");
	Expect(Got.count("fresh:leader") && EconHas(Got["fresh:leader"], "you're top of the points"), "econ fresh (the player): the room's points leader is the player");
	Expect(Got.count("fresh:season") && EconHas(Got["fresh:season"], "you're") && EconHas(Got["fresh:season"], "top of the season board"),
		"econ fresh (the player): on top of the Season board");
	Expect(!EconFind(M.Said, "fresh:hill:"), "econ fresh (the player): nobody tells a player who's won up the hill to go up there");
}

void EconEstablished()
{
	// The same hours in an established world: none of it, and the winners' rumours as they always were.
	Hooks H;
	ss::Session S(H, "econ-fresh");
	S.NewCareer(EconWho());
	EconMonth M = EconWalkMonth(S, 8);
	int Fresh = 0;
	int Winners = 0;
	for (const EconSaid& E : M.Said)
	{
		Fresh += E.Key.rfind("fresh:", 0) == 0 || EconHas(E.Line, " books") || EconHas(E.Line, "first name") ? 1 : 0;
		Winners += E.Key.rfind("rumour:winner:", 0) == 0 ? 1 : 0;
	}
	std::printf("  econ: an established world's first week on the block: %zu lines, %d winners' rumours, %d fresh\n", M.Said.size(), Winners, Fresh);
	Expect(S.WorldStart() == world::Start::Established && Fresh == 0 && Winners > 0 && M.Problems == 0, "econ established: the block says none of a fresh world's talk");
}
} // namespace econ_seg

void SegEcon()
{
	const auto T0 = std::chrono::steady_clock::now();
	econ_seg::EconPace();
	econ_seg::EconPaceSession();
	econ_seg::EconFreshTalk();
	econ_seg::EconFreshYou();
	econ_seg::EconEstablished();
	std::printf("  econ: core-econ's checks %.1f s\n", econ_seg::EconSecondsSince(T0));
}
} // namespace session_test
