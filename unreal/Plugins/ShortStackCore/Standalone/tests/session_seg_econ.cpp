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
//
// G2 (P0, §4.7, §4.4, §1.11):
//   EconBuzz          Rerun's buzz in the directory: the views of the last 48 hours raise the strangers by
//                     1 + 0.3 x min(1, buzz / 5,000), so x1 at none, x1.15 at 2,500 and x1.3 at 5,000 and at 50,000, on
//                     top of the watch speed's factor; over whole nights the new faces follow.
//   EconBuzzSession   the session hands the stream video::Buzz (none without a video; a day-old upload and a viral run).
//   EconMomentLog     tonight's moment log on the stream (§4.4): one note a moment (its kind, the clock, the viewers, the
//                     hype it left, the face, up to three of chat's own lines, the clip's time when somebody clipped it),
//                     the newest 60, empty again at the next stream; chat's lines say nothing of U1's words or a brand;
//                     a moment nobody clipped carries no clip time even in a clipped moment's minute, and a small room's
//                     clipped moment keeps chat's lines, not Kast's "clipped it" notice.
//   EconMomentLink    the session links a hand's note and its clip to the hand's candidate (SessionVideo's capture);
//                     publishing it freezes the clip, and Kast's offline days add no clip views or follows for it.
//   EconClipFollows   the same on Kast alone: an uploaded clip stops where it was, the others go on (no double count).
//   EconRecord        rep::Public's Rerun fields from the life record alone: VideoSubs (the closed-form count), Views30
//                     (the last 30 days only: exact while every folded video is older than the month, tails and stubs
//                     counted; for a heavy uploader whose tails folded young, never more than the month's real views),
//                     VideoPartner (from the welcome on, not the review); RenownOf's follower term
//                     log10(1 + F + subs / 2), the old term to the bit without subscribers; an old save reads none of it;
//                     the street's recognition follows, naming the videos once subscribers / 2 outweigh followers.
// G3 (rep::Public's site fields) lands with its gate.
//
// C3 (c3-fixes, the gate audit's should-fix 5):
//   EconSeasonPastHundred  the Season leader the block names is the board's, the player's row read as its YOU row is
//                     (site::HeroOn): past a hundred results the last hundred aren't the season. EconBoardTop reads it too.
#include "CopyLint.h"
#include "SessionSeg.h"

#include "../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"

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
namespace seams
{
std::string Fixture(const char* Name); // session_seams.cpp (the lead's): a committed fixture's text
} // namespace seams

namespace econ_seg
{
namespace kast = ss::kast;
namespace world = ss::world;
namespace video = ss::video;

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

/** What's wrong with an in-world line's words (U1's: "AI", "bot", "autoplay"; a real brand), or "". */
std::string EconWorldWords(const std::string& Line)
{
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

/** What's wrong with a line the block says (U1's words, a brand, the card's 140 characters, the lint), or "". */
std::string EconLint(const std::string& Line)
{
	const std::string Why = copylint::Check(Line, 140, {});
	return Why.empty() ? EconWorldWords(Line) : Why;
}

/** The board's top row's name now (the player's for -1), or "": the player's row as the board's YOU row reads it (site::HeroOn, C3). */
std::string EconBoardTop(const ss::Session& S, ss::net::Board B)
{
	const ss::net::HeroStats Hero = ss::site::HeroOn(ss::net::SiteId::RiverLine, S.HeroName, S.History, S.Life.Sites, &S.Living(), S.WorldMinutes());
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
		// (The line's own words: a leader's handle can hold one, "TensFullAgain".)
		std::string Said = E.Line;
		if (!Who.empty() && Said.find(Who) != std::string::npos)
		{
			Said.erase(Said.find(Who), Who.size());
		}
		if (EconHas(Said, "again") || EconHas(Said, "I was winning"))
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

/**
 * (C3, the audit's should-fix 5.) The Season board's leader the block names is the board's. Past a hundred results the board's
 * YOU row reads the world's season (site::HeroOn), not the last hundred: a player whose big score has left History is still on
 * top of the board, and Mei says so, rather than naming whoever's second.
 */
void EconSeasonPastHundred()
{
	Hooks H;
	ss::Session S(H, "econ-season-hundred");
	S.NewCareer(EconWho(), world::Start::Blank);
	world::World& W = S.Living();
	const double Opened = W.StartedAt();
	EconClock(S, Opened + 2.0);
	// The night's biggest score on RiverLine, the player's: the world counts its Season points (HeroPoints).
	const ss::net::Network& N = ss::net::Shared();
	std::string BigId;
	std::string BigName;
	double Best = 0.0;
	for (const ss::net::EventInstance& E : N.Window(Opened, Opened + 600.0))
	{
		const ss::net::EventTemplate& T = N.TemplateOf(E);
		const double P = T.JoinIndex < 0 && T.BuyInCents > 0 && T.BuyInCents <= 2200 ? ss::net::Points(1, E.Entries, T.BuyInCents) : 0.0;
		if (P > Best)
		{
			Best = P;
			BigId = E.Id;
			BigName = T.Name;
		}
	}
	W.HeroEntered(BigId, BigName);
	W.HeroFinished(BigId, 1, 250000, world::World::TableReport());
	// A hundred results since, out of the money: History holds those, and none of them scores.
	for (int K = 0; K < 100; ++K)
	{
		ss::HistoryEntry E;
		E.Name = "Night Owl Turbo";
		E.Place = 900;
		E.Entrants = 1000;
		E.Prize = 0;
		E.BuyInCents = 110;
		S.History.push_back(E);
	}
	const ss::net::HeroStats Last100 = ss::net::StatsFrom(S.HeroName, S.History, S.WorldMinutes());
	const std::string Top = EconBoardTop(S, ss::net::Board::Season);
	EconMonth M = EconWalkMonth(S, 2);
	std::string Season;
	for (const EconSaid& E : M.Said)
	{
		Season = Season.empty() && E.Key.rfind("fresh:season:", 0) == 0 ? E.Line : Season;
	}
	std::printf("  econ: the Season board past a hundred results: %s's %.0f points (the world's %.0f) against the last hundred's %.0f, top \"%s\"; \"%s\"\n", BigName.c_str(), Best,
		W.HeroSeasonPoints(), Last100.SeasonPoints, Top.c_str(), Season.c_str());
	Expect(Best > 0.0 && W.HeroSeasonPoints() > 0.0 && Last100.SeasonPoints == 0.0 && Top == S.HeroName,
		"econ season: the player tops the Season board on the world's points, with a hundred results since that score none");
	Expect(M.Problems == 0 && EconHas(Season, "you're") && EconHas(Season, "top of the season board"),
		"econ season: the block names the player as the Season board's leader, as the board does (site::HeroOn, not the last hundred)");
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
// ------------------------------------------------------------------ G2: Rerun's buzz on Kast (§4.7)

constexpr double EconDay = 1440.0;

kast::Inputs EconBuzzAt(kast::Inputs In, double Buzz)
{
	In.VideoBuzz = Buzz;
	return In;
}

/** §4.7's factor, as the spec writes it. */
double EconBuzzFactor(double Buzz)
{
	return 1.0 + 0.3 * std::min(1.0, std::max(0.0, Buzz) / 5000.0);
}

void EconBuzz()
{
	const kast::Inputs Base = EconBaseInputs();
	const double Tick = 0.25 / 6.0;
	// The first tick: the same stream up to the strangers, so their ratio is the factor itself.
	const double None = EconStream("econ-buzz", EconBuzzAt(Base, 0.0), Tick).First;
	const double Half = EconStream("econ-buzz", EconBuzzAt(Base, 2500.0), Tick).First;
	const double Full = EconStream("econ-buzz", EconBuzzAt(Base, 5000.0), Tick).First;
	const double Way = EconStream("econ-buzz", EconBuzzAt(Base, 50000.0), Tick).First;
	const double Nonsense = EconStream("econ-buzz", EconBuzzAt(Base, -40.0), Tick).First;
	const double Four = EconStream("econ-buzz", EconAt(EconBuzzAt(Base, 5000.0), 4, false), Tick).First;
	const double Skip = EconStream("econ-buzz", EconAt(EconBuzzAt(Base, 2500.0), 1, true), Tick).First;
	std::printf("  econ buzz: first-tick strangers at 0 / 2,500 / 5,000 / 50,000 views: %.6f / %.6f (x%.4f) / %.6f (x%.4f) / %.6f (x%.4f)\n", None, Half, Half / None, Full,
		Full / None, Way, Way / None);
	Expect(None > 0.0 && EconNear(Half, 1.15 * None) && EconNear(Full, 1.3 * None) && EconNear(Way, 1.3 * None),
		"econ buzz: the directory's strangers x1.15 at 2,500 views in 48 hours, x1.3 at 5,000, and no more past it (50,000)");
	Expect(Nonsense == None, "econ buzz: no views (or a count below none) is the stream without buzz, to the bit");
	Expect(EconNear(Four, 0.9 * 1.3 * None) && EconNear(Skip, 0.8 * 1.15 * None), "econ buzz: on top of the watch speed's factor (4x x0.9, a skip x0.8)");

	// Whole nights on a grown channel: about three in ten more strangers drop in, and more of them follow.
	double Faces[2] = {0, 0};
	double Follows[2] = {0, 0};
	for (int Seed = 0; Seed < 8; ++Seed)
	{
		for (int K = 0; K < 2; ++K)
		{
			const EconStreamRun R = EconStream("econ-buzz-" + std::to_string(Seed), EconBuzzAt(Base, K == 0 ? 0.0 : 5000.0), 480.0, 0.5, true);
			Faces[K] += R.Sum.NewFaces;
			Follows[K] += R.Sum.Follows;
		}
	}
	const double Ratio = Faces[1] / std::max(1.0, Faces[0]);
	std::printf("  econ buzz: eight 8-hour streams: new faces %.0f without buzz, %.0f at 5,000 views (x%.3f); follows %.0f / %.0f\n", Faces[0], Faces[1], Ratio, Follows[0], Follows[1]);
	Expect(Faces[0] > 0.0 && Ratio > 1.15 && Ratio < 1.5, "econ buzz: over whole nights at full buzz about three in ten more strangers drop in");
	Expect(Follows[1] > Follows[0], "econ buzz: and more of them follow");
}

/** A cooler at $5.50 (session_seg_video's shape): a candidate a channel can publish. */
video::Candidate EconCooler(const std::string& Id, double World)
{
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
	C.Expires = World + 21.0 * EconDay;
	return C;
}

/** Publishes one new candidate as F at World, straight on the channel (the edit's time is the session's, not the point here). */
std::string EconUpload(video::Channel& C, const std::string& Id, double World, video::Format F, int KastFollowers = 0,
	video::TitleStyle T = video::TitleStyle::Story, video::Kind K = video::Kind::Cooler)
{
	video::Candidate Cand = EconCooler(Id, World);
	Cand.K = K;
	video::NoteHand(C, Cand);
	video::Choice Ch;
	Ch.Candidates = {Id};
	Ch.F = F;
	Ch.T = T;
	Ch.Fr = video::Frame::Versus;
	video::UploadFacts Facts;
	Facts.GearQuality = 0.5; // a mid rig
	Facts.KastFollowers = KastFollowers;
	return video::Upload(C, Ch, World, "econ-g2", Facts);
}

/** A session with a rig that streams, at a table. */
bool EconAtTable(ss::Session& S)
{
	S.CurrentScreen = ss::Screen::Lobby;
	S.BankrollCents = 300000;
	const bool Rig = S.Buy(ss::gear::FirstPcUpgrade().Id).empty();
	S.Register(1);
	return Rig && S.T && S.StreamInputs().Gear.CanStream();
}

void EconBuzzSession()
{
	Hooks H;
	ss::Session S(H, "econ-buzz-session");
	const bool Ready = EconAtTable(S);
	const double World = S.WorldMinutes();
	const double Off = EconFirstStrangers(S.StreamInputs());
	const bool Quiet = S.StreamInputs().VideoBuzz == 0.0;
	// A Breakdown up a day and a half ago: its first days' views are tonight's buzz.
	const std::string Id = EconUpload(S.Life.Video, "rl:econ-buzz:1", World - 1.5 * EconDay, video::Format::Breakdown, 400);
	const double Buzz = video::Buzz(S.Life.Video, World);
	const double Handed = S.StreamInputs().VideoBuzz;
	const double Up = EconFirstStrangers(S.StreamInputs()) / Off;
	// A run that went viral (the dev grant's video, its views over a month): far past 5,000 in 48 hours.
	video::Grant(S.Life.Video, 0.0, 0.0, 600000.0, World - 1.0 * EconDay);
	const double Viral = video::Buzz(S.Life.Video, World);
	const double Capped = EconFirstStrangers(S.StreamInputs()) / Off;
	std::printf("  econ buzz session: no channel x1 (buzz %.0f); a day-old Breakdown: %.1f views in 48 hours, strangers x%.4f; viral: %.0f views, x%.4f\n",
		Quiet ? 0.0 : -1.0, Buzz, Up, Viral, Capped);
	Expect(Ready && Quiet && Off > 0.0, "econ buzz session: no channel, no buzz");
	Expect(!Id.empty() && Buzz > 0.0 && Buzz < 5000.0 && Handed == Buzz && EconNear(Up, EconBuzzFactor(Buzz)),
		"econ buzz session: the session hands the stream Rerun's last 48 hours (video::Buzz), and the strangers rise by its factor");
	Expect(Viral >= 5000.0 && S.StreamInputs().VideoBuzz == Viral && EconNear(Capped, 1.3), "econ buzz session: a viral week tops out at x1.3");
}

// ------------------------------------------------------------------ G2: tonight's moment log (§4.4)

void EconMomentLog()
{
	kast::Inputs In = EconBaseInputs();
	kast::Channel Ch;
	Ch.Xp = kast::Channel::LevelXp(20);
	kast::Stream St("econ-moments");
	St.Start(Ch, In);
	const kast::Moment Kinds[] = {kast::Moment::AllIn, kast::Moment::BadBeat, kast::Moment::WonAllIn, kast::Moment::Knockout, kast::Moment::FinalTable, kast::Moment::Win,
		kast::Moment::BigPot};
	bool Each = true;
	bool Clipped = true;
	bool Lines = true;
	int Clips = 0;
	int Problems = 0;
	std::vector<double> Times;
	for (int K = 0; K < 70; ++K)
	{
		In.World += 1.5;
		In.Real += 9.0;
		St.Viewers = 40.0; // a room big enough to clip things (the stream's own ticks would take a while to get there)
		const kast::Moment M = Kinds[K % 7];
		const double HypeBefore = St.Hype;
		const int ClipsBefore = St.Tonight.Clips;
		const size_t LogBefore = St.Moments.size();
		St.OnMoment(Ch, In, M, M == kast::Moment::Knockout ? "villain" : "");
		Times.push_back(In.World);
		const bool Logged = !St.Moments.empty() && (St.Moments.size() == LogBefore + 1 || (LogBefore == 60 && St.Moments.size() == 60));
		const kast::MomentNote N = Logged ? St.Moments.back() : kast::MomentNote();
		Each = Each && Logged && N.Kind == M && N.World == In.World && N.Real == In.Real && N.Viewers == 40 && N.HypeAfter >= HypeBefore - 1e-9 && N.HypeAfter <= St.Hype + 1e-9 &&
			N.Face == St.Face && N.Candidate.empty();
		const bool Made = St.Tonight.Clips > ClipsBefore;
		Clips += Made ? 1 : 0;
		Clipped = Clipped &&
			(Made ? N.ClipAt == In.World && !Ch.Clips.empty() && Ch.Clips.front().At == In.World && Ch.Clips.front().Kind == static_cast<int>(M) : N.ClipAt == -1.0);
		// Chat's own reaction: one to three lines, each one chat wrote just now.
		bool Said = !N.Chat.empty() && N.Chat.size() <= 3;
		for (const std::string& Line : N.Chat)
		{
			bool Found = false;
			for (const kast::ChatMsg& Msg : St.Chat)
			{
				Found = Found || (Msg.Kind == kast::LineKind::Chat && Msg.Text == Line && Msg.At >= In.Real && !Msg.Toxic);
			}
			Said = Said && Found;
			const std::string Why = EconWorldWords(Line);
			if (!Why.empty())
			{
				++Problems;
				std::printf("  econ moments: \"%s\": %s\n", Line.c_str(), Why.c_str());
			}
		}
		Lines = Lines && Said;
		if (K < 3)
		{
			std::printf("  econ moments: %s at %.1f: %d viewers, hype %.1f, face %d, clip %s, chat \"%s\"\n", kast::MomentName(M), N.World, N.Viewers, N.HypeAfter,
				static_cast<int>(N.Face), N.ClipAt >= 0.0 ? "yes" : "no", N.Chat.empty() ? "" : N.Chat.front().c_str());
		}
	}
	const bool Kept = St.Moments.size() == 60 && St.Moments.front().World == Times[10] && St.Moments.back().World == Times.back();
	St.Stop(Ch, In);
	const bool Stays = St.Moments.size() == 60;
	In.World += 600.0;
	In.Real += 60.0;
	St.Start(Ch, In);
	const bool Fresh = St.Live && St.Moments.empty();
	std::printf("  econ moments: 70 moments, %d clipped; the log keeps %d through the night\n", Clips, Kept ? 60 : -1);
	Expect(Each, "econ moments: each moment is one note: its kind, the world clock and the real one, the viewers, the hype it left, the face, no candidate yet");
	Expect(Clips > 0 && Clipped, "econ moments: a clipped moment carries the clip's time (its At, as the clip has it); the others carry none");
	Expect(Lines, "econ moments: one to three of chat's own lines about it");
	Expect(Kept && Stays && Fresh, "econ moments: the newest 60 are kept through the night and after it; the next stream starts with none");
	Expect(Problems == 0, "econ moments: chat's lines say none of U1's words and no real brand");

	// Two moments of one hand share its minute (the tick clock stands still through a hand: the all-in won, then the
	// knockout). A clip of the first isn't the second's: the second carries a clip time only if somebody clipped it.
	{
		kast::Inputs Same = EconBaseInputs();
		kast::Channel Ch2;
		Ch2.Xp = kast::Channel::LevelXp(20);
		kast::Stream St2("econ-moments-same");
		St2.Start(Ch2, Same);
		Same.World += 30.0;
		int Pairs = 0;
		bool Own = true;
		for (int K = 0; K < 400 && Pairs < 3; ++K)
		{
			Same.Real += 9.0;
			St2.Viewers = 40.0;
			const int Before = St2.Tonight.Clips;
			St2.OnMoment(Ch2, Same, kast::Moment::WonAllIn);
			if (St2.Tonight.Clips == Before)
			{
				continue;
			}
			const int AfterFirst = St2.Tonight.Clips;
			St2.OnMoment(Ch2, Same, kast::Moment::Knockout, "villain");
			if (St2.Tonight.Clips == AfterFirst)
			{
				++Pairs;
				Own = Own && St2.Moments.back().Kind == kast::Moment::Knockout && St2.Moments.back().ClipAt == -1.0 && St2.Moments[St2.Moments.size() - 2].ClipAt == Same.World;
			}
		}
		std::printf("  econ moments: %d hands with the all-in clipped and the knockout not, in one minute\n", Pairs);
		Expect(Pairs > 0 && Own, "econ moments: in one hand's minute, a moment nobody clipped carries no clip time (the all-in's clip is the all-in's)");
	}

	// A small room (under two viewers: two lines of chat a moment): when somebody clips it, Kast's own "clipped it" notice
	// follows the two lines, and the note keeps chat's lines only.
	{
		kast::Inputs Small = EconBaseInputs();
		kast::Channel Ch3;
		Ch3.Xp = kast::Channel::LevelXp(20);
		kast::Stream St3("econ-moments-small");
		St3.Start(Ch3, Small);
		int SmallClips = 0;
		bool ChatOnly = true;
		for (int K = 0; K < 2000 && SmallClips < 3; ++K)
		{
			Small.World += 1.5;
			Small.Real += 9.0;
			St3.Viewers = 1.6;
			const int Before = St3.Tonight.Clips;
			St3.OnMoment(Ch3, Small, kast::Moment::BadBeat);
			if (St3.Tonight.Clips == Before)
			{
				continue;
			}
			++SmallClips;
			const kast::MomentNote& N = St3.Moments.back();
			ChatOnly = ChatOnly && N.ClipAt == Small.World && !N.Chat.empty() && N.Chat.size() <= 2;
			for (const std::string& Line : N.Chat)
			{
				ChatOnly = ChatOnly && !EconHas(Line, "clipped it");
			}
		}
		std::printf("  econ moments: a room of two: %d clipped moments\n", SmallClips);
		Expect(SmallClips > 0 && ChatOnly, "econ moments: a small room's clipped moment keeps chat's own lines, not Kast's \"clipped it\" notice");
	}
}

/** Aces all in preflop against sevens, the player on the button, then River (session_seg_video's hand). */
std::unique_ptr<ss::Hand> EconAcesAllIn(const char* River)
{
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
	ss::Rng R("econ-aces");
	auto Hd = std::make_unique<ss::Hand>(Cfg, R);
	for (const ss::PlayerAction& A : {ss::PlayerAction::RaiseTo(600), ss::PlayerAction::RaiseTo(6400), ss::PlayerAction::Call()})
	{
		Hd->Act(A);
	}
	return Hd;
}

/** Kast's clip follows from a day off (0.002 a view, Kast.cpp's Offline): what the unlinked clips gain from their views to World. */
double EconClipGain(const kast::Channel& Ch, double World)
{
	double Gain = 0.0;
	for (const kast::Clip& C : Ch.Clips)
	{
		if (C.Candidate.empty())
		{
			Gain += std::max(0.0, C.Reach * (1.0 - std::exp(-(World - C.At) / (1.5 * EconDay))) - C.Views);
		}
	}
	return Gain;
}

void EconMomentLink()
{
	Hooks H;
	ss::Session S(H, "econ-moment-link");
	const bool Ready = EconAtTable(S);
	const std::string Live = Ready ? S.GoLive() : std::string("no table");
	Expect(Ready && Live.empty() && S.Stream.Live, "econ link: live on Kast at a table");
	if (!Ready || !S.Stream.Live || !S.T)
	{
		return;
	}
	const auto Beat = EconAcesAllIn("7d");
	video::Candidate Got;
	kast::MomentNote Note;
	bool Each = true;
	int Tries = 0;
	for (; Tries < 30 && Got.Id.empty(); ++Tries)
	{
		// A new hand (the tick clock moves a hand on), its award's moment (what Session::StreamMoment does), then its
		// capture at the same tick (HeroHandDone's VideoNoteHand, at the end of AfterAward).
		S.T->Tick += 1;
		S.Stream.Viewers = 40.0;
		S.Stream.OnMoment(S.Channel, S.StreamInputs(), kast::Moment::BadBeat);
		ss::Session::HandFacts F;
		S.VideoNoteHand(*Beat, F);
		const kast::MomentNote N = S.Stream.Moments.empty() ? kast::MomentNote() : S.Stream.Moments.back();
		const video::Candidate* C = nullptr;
		for (const video::Candidate& X : S.Life.Video.Open)
		{
			C = !N.Candidate.empty() && X.Id == N.Candidate ? &X : C;
		}
		Each = Each && C && C->K == video::Kind::BadBeat && C->Stream.Chat == N.Chat && C->Stream.ClipAt == N.ClipAt && C->Stream.Viewers == 40;
		if (C && N.ClipAt >= 0.0)
		{
			Got = *C;
			Note = N;
		}
	}
	auto ClipOf = [&S](double At) -> const kast::Clip* {
		const kast::Clip* Out = nullptr;
		for (const kast::Clip& K : S.Channel.Clips)
		{
			Out = K.At == At ? &K : Out;
		}
		return Out;
	};
	const kast::Clip* Clip = ClipOf(Note.ClipAt);
	std::printf("  econ link: %d hands captured live; the clipped one: %s, clip at %.2f (\"%s\"), %zu chat lines\n", Tries, Got.Id.c_str(), Got.Stream.ClipAt,
		Clip ? Clip->Title.c_str() : "", Got.Stream.Chat.size());
	Expect(Each, "econ link: every hand's moment goes to that hand's candidate, with its chat lines, its viewers and its clip time");
	Expect(!Got.Id.empty() && Note.Candidate == Got.Id && Got.Stream.ClipAt == Note.ClipAt && Clip && Clip->Candidate.empty() && !Got.Stream.Chat.empty(),
		"econ link: a clipped bad beat's candidate carries the Kast clip's time (clips have no id: they link by At)");
	if (!Clip)
	{
		return;
	}
	// Half a day of the clip being watched on Kast, then published (the edit's settle, FinishSkip's "video:" activity):
	// the clip is the video's now.
	const double T0 = S.WorldMinutes();
	ss::Rng R("econ-link-offline");
	kast::Offline(S.Channel, T0 + 0.5 * EconDay, R);
	const std::string Up = S.SettleEdit("video:1:0:0:" + Got.Id);
	Clip = ClipOf(Note.ClipAt);
	const double Views = Clip ? Clip->Views : -1.0;
	Expect(Up.empty() && Clip && Clip->Candidate == Got.Id && Views > 0.0, "econ link: publishing it marks the Kast clip (watched a while) as the video's");
	// Kast's days off: the clip's views stand where they were and it brings no follows; any other clip goes on as before.
	double Expected = 0.0;
	const double Before = static_cast<double>(S.Channel.Followers) + S.Channel.FollowFrac;
	for (const double Days : {1.0, 2.0, 3.5})
	{
		Expected += 0.002 * EconClipGain(S.Channel, T0 + Days * EconDay);
		kast::Offline(S.Channel, T0 + Days * EconDay, R);
	}
	Clip = ClipOf(Note.ClipAt);
	const double After = static_cast<double>(S.Channel.Followers) + S.Channel.FollowFrac;
	std::printf("  econ link: three days on, the uploaded clip at %.2f views (%.2f at the upload); clip follows %.6f (the other clips' %.6f)\n", Clip ? Clip->Views : -1.0,
		Views, After - Before, Expected);
	Expect(Clip && Clip->Views == Views && std::fabs((After - Before) - Expected) < 1e-9, "econ link: an uploaded clip adds no clip views and no clip follows (no double count)");
}

void EconClipFollows()
{
	// Two clips of one moment with the same reach; five hours on, the second goes up on Rerun (SettleEdit's freeze: its
	// reach is its views, its Candidate set). Over the next four days, a day off at a time, only the first gains, and only
	// the first adds follows (a frozen clip read again would lose views and win them back, follows and all).
	const double At = 10.0 * EconDay + 22.0 * 60.0;
	auto Night = [At](bool Upload, double& Frac, double& Second) {
		kast::Channel Ch;
		for (int K = 0; K < 2; ++K)
		{
			kast::Clip C;
			C.Title = K == 0 ? "the cruelest river" : "rivered again";
			C.By = "railbird_9";
			C.At = At;
			C.Reach = 140.0;
			C.Kind = static_cast<int>(kast::Moment::BadBeat);
			Ch.Clips.push_back(C);
		}
		ss::Rng R("econ-clip-follows");
		kast::Offline(Ch, At + 300.0, R);
		if (Upload)
		{
			Ch.Clips[1].Reach = Ch.Clips[1].Views;
			Ch.Clips[1].Candidate = "rl:econ-clip:12";
		}
		const double Was = Ch.FollowFrac + static_cast<double>(Ch.Followers);
		const double Held = Ch.Clips[1].Views;
		double Low = Held;
		for (const double Days : {1.0, 2.0, 3.0, 4.0})
		{
			kast::Offline(Ch, At + Days * EconDay, R);
			Low = std::min(Low, Ch.Clips[1].Views);
		}
		Frac = Ch.FollowFrac + static_cast<double>(Ch.Followers) - Was;
		Second = Ch.Clips[1].Views - Held + (Low - Held);
		return Ch.Clips[0].Views;
	};
	double FracBoth = 0.0, FracOne = 0.0, GainBoth = 0.0, GainUp = 0.0;
	const double First = Night(false, FracBoth, GainBoth);
	const double FirstUp = Night(true, FracOne, GainUp);
	std::printf("  econ clips: four days on: the first clip at %.1f views either way; the second gained %.1f (on Kast) / %.1f (uploaded); clip follows %.4f / %.4f\n", First,
		GainBoth, GainUp, FracBoth, FracOne);
	Expect(First == FirstUp && GainBoth > 0.0 && GainUp == 0.0, "econ clips: the uploaded clip stops where it was; the other goes on as before");
	Expect(FracOne > 0.0 && EconNear(FracBoth, 2.0 * FracOne, 1e-6), "econ clips: and only the clip still on Kast brings follows (half the follows of two)");
}

// ------------------------------------------------------------------ G2: the public record's Rerun fields (§4.7)

/** RenownOf before update 2 (Town.cpp's formula as it was), for the to-the-bit comparison. */
double EconOldRenown(const ss::rep::Public& P)
{
	const double Followers = std::min(1.0, std::log10(1.0 + static_cast<double>(std::max(0, P.Followers))) / 5.0);
	const double Fans = std::min(1.0, static_cast<double>(P.Superfans) / 40.0);
	const double Cashes = std::min(1.0, static_cast<double>(P.LiveCashes) / 15.0);
	const double Wins = std::min(1.0, static_cast<double>(P.Titles) / 4.0);
	const double Awards = std::min(1.0, static_cast<double>(P.Bracelets + P.Rings) / 2.0);
	return std::min(1.0, 0.35 * Followers + 0.15 * Fans + 0.15 * Cashes + 0.15 * Wins + 0.2 * Awards);
}

void EconRecord()
{
	namespace rep = ss::rep;
	namespace life = ss::life;
	// The formula: without subscribers the old one to the bit; a subscriber counts half a follower.
	bool Old = true;
	bool Half = true;
	bool Rises = true;
	for (const int F : {0, 1, 7, 64, 2400, 31000, 250000, 4000000})
	{
		for (const int Fans : {0, 12, 40})
		{
			rep::Public P;
			P.Followers = F;
			P.Superfans = Fans;
			P.LiveCashes = 6;
			P.Titles = 1;
			P.Bracelets = F % 2;
			Old = Old && rep::RenownOf(P) == EconOldRenown(P);
			rep::Public V = P;
			V.VideoSubs = 2 * (F + 13);
			rep::Public Same = P;
			Same.Followers = F + F + 13;
			Half = Half && rep::RenownOf(V) == rep::RenownOf(Same);
			Rises = Rises && (F >= 99999 ? rep::RenownOf(V) == rep::RenownOf(P) : rep::RenownOf(V) > rep::RenownOf(P));
		}
	}
	Expect(Old, "econ record: with no subscribers RenownOf is the old formula to the bit");
	Expect(Half && Rises, "econ record: the follower term is log10(1 + followers + subscribers / 2): renown rises with the channel until the term tops out");

	// No channel (an old save's life record): nothing, and the old renown.
	life::State Blank;
	kast::Channel NoKast;
	const rep::Public E = rep::Of(Blank, NoKast, nullptr, 40.0 * EconDay);
	Expect(E.VideoSubs == 0 && E.Views30 == 0 && !E.VideoPartner && E.Renown == EconOldRenown(E), "econ record: no channel: no subscribers, no views, no Partner, the old renown");
	bool Fixtures = true;
	for (const char* Name : {"pre_update2_sponsored_streamer.ssave", "pre_update2_veteran.ssave"})
	{
		ss::SaveData D;
		const bool Parsed = ss::SaveData::Parse(seams::Fixture(Name), D);
		Hooks H;
		ss::Session S(H, std::string("econ-record-") + Name, &D);
		const rep::Public R = S.Record();
		Fixtures = Fixtures && Parsed && R.VideoSubs == 0 && R.Views30 == 0 && !R.VideoPartner && R.Renown == EconOldRenown(R);
		std::printf("  econ record: %s: %d followers, renown %.6f (the old formula's %.6f)\n", Name, R.Followers, R.Renown, EconOldRenown(R));
	}
	Expect(Fixtures, "econ record: an old save (the sponsored streamer, the veteran) reads no channel and renders the old renown");

	// One Breakdown up on day 10, the days stepped as the calendar steps them: the record reads the channel at the hour.
	life::State L;
	kast::Channel K;
	ss::Chips Bank = 0;
	const std::string Id = EconUpload(L.Video, "rl:econ-record:1", 10.0 * EconDay + 180.0, video::Format::Breakdown, 20000);
	bool Fits = true;
	bool Young = false;
	bool Aged = false;
	for (int D = 10; D <= 60; ++D)
	{
		video::Step(L.Video, L, Bank, K, static_cast<double>(D) * EconDay, static_cast<double>(D + 1) * EconDay);
		if (D + 1 != 12 && D + 1 != 25 && D + 1 != 41 && D + 1 != 55)
		{
			continue;
		}
		const double W = static_cast<double>(D + 1) * EconDay + 600.0;
		const rep::Public P = rep::Of(L, K, nullptr, W);
		const video::Video& V = L.Video.Videos.front();
		const double All = video::Views(V, W);
		const double Window = All - video::Views(V, W - 30.0 * EconDay);
		Fits = Fits && P.VideoSubs == static_cast<int>(std::floor(video::SubsAt(L.Video, W))) && std::abs(P.Views30 - static_cast<int>(std::floor(Window))) <= 1;
		Young = Young || (D + 1 == 12 && std::abs(P.Views30 - static_cast<int>(std::floor(All))) <= 1 && P.Views30 > 0);
		Aged = Aged || (D + 1 == 55 && Window < 0.5 * All && P.Views30 < static_cast<int>(All) && P.Views30 > 0);
		std::printf("  econ record: day %d: %d subscribers (%.3f), views %.1f to date, Views30 %d (the window's %.1f)\n", D + 1, P.VideoSubs, video::SubsAt(L.Video, W), All,
			P.Views30, Window);
	}
	Expect(!Id.empty() && Fits, "econ record: VideoSubs is the channel's closed-form count; Views30 the views of the 30 days to the hour");
	Expect(Young && Aged, "econ record: two days in Views30 is every view; 45 days in it's only the last month's");

	// The gate (a dev grant past both bars): the review first, the welcome a week on. VideoPartner from the welcome.
	video::Grant(L.Video, 1000.0, 4000.0, 0.0, 61.0 * EconDay);
	bool InReview = false;
	bool ReviewSaysNo = true;
	bool PartnerSaysYes = false;
	double PartnerDay = -1.0;
	for (int D = 61; D <= 75; ++D)
	{
		video::Step(L.Video, L, Bank, K, static_cast<double>(D) * EconDay, static_cast<double>(D + 1) * EconDay);
		const double W = static_cast<double>(D + 1) * EconDay + 60.0;
		const rep::Public P = rep::Of(L, K, nullptr, W);
		if (L.Video.ReviewAt >= 0.0 && L.Video.PartnerAt < 0.0)
		{
			InReview = true;
			ReviewSaysNo = ReviewSaysNo && !P.VideoPartner;
		}
		if (L.Video.PartnerAt >= 0.0 && PartnerDay < 0.0)
		{
			PartnerDay = L.Video.PartnerAt / EconDay;
			PartnerSaysYes = P.VideoPartner && !rep::Of(L, K, nullptr, L.Video.PartnerAt - 1.0).VideoPartner;
		}
	}
	std::printf("  econ record: the gate's review from day %.0f, Partner from day %.0f\n", L.Video.ReviewAt / EconDay, PartnerDay);
	Expect(InReview && ReviewSaysNo && PartnerSaysYes, "econ record: VideoPartner reads false through the review and true from the welcome on");

	// The month's views as the same videos kept whole would have them (a video's curve doesn't change when it folds).
	auto Month = [](const std::vector<video::Video>& Vs, double At) {
		double Sum = 0.0;
		for (const video::Video& V : Vs)
		{
			Sum += video::Views(V, At) - video::Views(V, At - 30.0 * EconDay);
		}
		return Sum;
	};

	// A short-spammer: four Shorts a day for 50 days. Past 60 the oldest fold into the tail two weeks old, and a tail read
	// back a month runs its closed form before those videos went up (fifteen 48-hour windows of it, unbounded: below), so
	// the tails count only from the oldest kept upload on. Views30 is views that happened: no more than the month's real
	// views (well under the views to date), more than the kept videos' own.
	life::State L2;
	kast::Channel K2;
	ss::Chips Bank2 = 0;
	std::vector<video::Video> Whole2;
	for (int D = 0; D < 50; ++D)
	{
		for (int I = 0; I < 4; ++I)
		{
			EconUpload(L2.Video, "rl:econ-spam:" + std::to_string(D * 4 + I), static_cast<double>(100 + D) * EconDay + 60.0 * (I + 1), video::Format::Short);
			Whole2.push_back(L2.Video.Videos.back());
		}
		video::Step(L2.Video, L2, Bank2, K2, static_cast<double>(100 + D) * EconDay, static_cast<double>(101 + D) * EconDay);
	}
	const double W2 = 150.0 * EconDay + 60.0;
	double Raw = 0.0;
	for (int Wnd = 0; Wnd < 15; ++Wnd)
	{
		Raw += video::Buzz(L2.Video, W2 - 2.0 * static_cast<double>(Wnd) * EconDay);
	}
	const double ToDate = video::TotalViews(L2.Video, W2);
	const double Real2 = Month(Whole2, W2);
	const double Kept2 = Month(L2.Video.Videos, W2);
	const rep::Public P2 = rep::Of(L2, K2, nullptr, W2);
	std::printf("  econ record: four Shorts a day for 50 days: %zu kept whole, views to date %.1f, the month's real views %.1f, Views30 %d, the kept videos' "
				"own %.1f (fifteen 48-hour windows unbounded: %.4g)\n",
		L2.Video.Videos.size(), ToDate, Real2, P2.Views30, Kept2, Raw);
	Expect(L2.Video.Videos.size() == 60 && Real2 < 0.9 * ToDate && P2.Views30 <= static_cast<int>(std::floor(Real2)) && static_cast<double>(P2.Views30) > Kept2 + 1.0,
		"econ record: a heavy uploader's Views30 is views that happened: no more than the month's real views, more than the kept videos' own");

	// A daily strategy channel (Teaching Breakdowns of bluffs) 100 days on: 40 videos folded, all older than the month, with
	// their evergreen stubs. There the tails and the stubs are exact, and Views30 is the month's real views.
	life::State L4;
	kast::Channel K4;
	ss::Chips Bank4 = 0;
	std::vector<video::Video> Whole4;
	for (int D = 7; D < 107; ++D)
	{
		EconUpload(L4.Video, "rl:econ-daily:" + std::to_string(D), static_cast<double>(D) * EconDay + 600.0, video::Format::Breakdown, 200, video::TitleStyle::Teaching,
			video::Kind::Bluff);
		Whole4.push_back(L4.Video.Videos.back());
		video::Step(L4.Video, L4, Bank4, K4, static_cast<double>(D) * EconDay, static_cast<double>(D + 1) * EconDay);
	}
	const double W4 = 107.0 * EconDay + 600.0;
	const double Real4 = Month(Whole4, W4);
	const double Kept4 = Month(L4.Video.Videos, W4);
	const rep::Public P4 = rep::Of(L4, K4, nullptr, W4);
	std::printf("  econ record: a daily strategy channel at day 107: %zu kept, %zu stubs, the month's real views %.1f (the kept videos' own %.1f), Views30 %d\n",
		L4.Video.Videos.size(), L4.Video.Stubs.size(), Real4, Kept4, P4.Views30);
	Expect(L4.Video.Videos.size() == 60 && !L4.Video.Stubs.empty() && Real4 > Kept4 + 2.0 && std::abs(P4.Views30 - static_cast<int>(std::floor(Real4))) <= 1,
		"econ record: with every folded video older than the month, Views30 is the month's real views, the tails and the stubs counted");

	// Before the day's step: the record reads the channel at the hour, not as of the last step. Four Breakdowns up this
	// morning have brought subscribers by tonight that the saved count (stepped to midnight) hasn't counted yet.
	life::State L3;
	for (int I = 0; I < 4; ++I)
	{
		EconUpload(L3.Video, "rl:econ-today:" + std::to_string(I), 200.0 * EconDay + 60.0 * (I + 7), video::Format::Breakdown, 20000);
	}
	const double W3 = 200.0 * EconDay + 22.0 * 60.0;
	const rep::Public P3 = rep::Of(L3, K2, nullptr, W3);
	std::printf("  econ record: four Breakdowns this morning: %d subscribers by 10 PM (%.3f; the count saved at midnight %.3f), Views30 %d\n", P3.VideoSubs,
		video::SubsAt(L3.Video, W3), L3.Video.Subs, P3.Views30);
	Expect(L3.Video.Videos.size() == 4 && P3.VideoSubs == static_cast<int>(std::floor(video::SubsAt(L3.Video, W3))) && P3.VideoSubs >= 1 + static_cast<int>(L3.Video.Subs),
		"econ record: the record counts today's subscribers in closed form, before the calendar steps the day");

	// The card room's build (SaveData alone, no world) reads the same fields as the session's.
	{
		Hooks H;
		ss::Session S(H, "econ-record-room");
		S.Life.Video = L.Video;
		const double W = S.WorldMinutes();
		const rep::Public A = S.Record();
		const rep::Public B = rep::Of(S.Life, S.Channel, nullptr, W);
		Expect(A.VideoSubs == B.VideoSubs && A.Views30 == B.Views30 && A.VideoPartner == B.VideoPartner && A.VideoSubs >= 1000,
			"econ record: the card room's record (no world) reads the channel the same");
	}

	// The street: 15 cashes and four titles on the wall aren't a known face; 20,000 subscribers on top are, and the block
	// says so (Talk's Famous is renown 0.55), naming the videos when they're most of it.
	{
		Hooks H;
		ss::Session S(H, "econ-record-street");
		S.Life.LiveCashes = 15;
		for (int T = 0; T < 4; ++T)
		{
			life::LiveEntry Won;
			Won.Id = "embercrest-nightly@" + std::to_string(2 + T);
			Won.Name = "Embercrest Nightly $120";
			Won.State = life::LiveEntry::Finished;
			Won.Place = 1;
			S.Life.LiveEntries.push_back(Won);
		}
		S.Life.Hunger = 0.0;
		S.Life.Thirst = 0.0;
		S.Life.Energy = 100.0;
		const double Plain = ss::town::Renown(S);
		const ss::town::Talk Before = ss::town::Open(*ss::town::ContextFor(S, "sal", ""));
		video::Grant(S.Life.Video, 20000.0, 0.0, 0.0, S.WorldMinutes());
		const double Known = ss::town::Renown(S);
		const rep::Public R = S.Record();
		const ss::town::Talk Sal = ss::town::Open(*ss::town::ContextFor(S, "sal", ""));
		const ss::town::Talk Lonnie = ss::town::Open(*ss::town::ContextFor(S, "lonnie", "barber"));
		// The stream's own fame: the same renown from followers says the stream's line.
		S.Life.Video = video::Channel();
		S.Channel.Followers = 10000;
		const ss::town::Talk Streamer = ss::town::Open(*ss::town::ContextFor(S, "sal", ""));
		std::printf("  econ record: the street: renown %.3f -> %.3f with %d subscribers; Sal: [%s] \"%s\"; Lonnie: \"%s\"; a streamer hears \"%s\"\n", Plain, Known, R.VideoSubs,
			Sal.Topic.c_str(), Sal.Line.c_str(), Lonnie.Line.c_str(), Streamer.Line.c_str());
		Expect(R.VideoSubs == 20000 && Plain < 0.55 && Known >= 0.55, "econ record: 20,000 subscribers make a known face of a player with 15 cashes and four titles");
		Expect(Before.Topic != "look:famous" && Sal.Topic == "look:famous" && EconHas(Sal.Line, "videos") && EconHas(Lonnie.Line, "videos") && !EconHas(Lonnie.Line, "Kast"),
			"econ record: the block knows the face, from the videos");
		Expect(Streamer.Topic == "look:famous" && EconHas(Streamer.Line, "stream"), "econ record: a face known from the stream still hears the stream's line");
		// Both: renown weighs a subscriber at half a follower, and the line names the bigger half. 15,000 subscribers beside
		// 10,000 followers (7,500 against 10,000) are the stream's face still; 25,000 (12,500) are the videos'.
		video::Grant(S.Life.Video, 15000.0, 0.0, 0.0, S.WorldMinutes());
		const ss::town::Talk Mostly = ss::town::Open(*ss::town::ContextFor(S, "sal", ""));
		video::Grant(S.Life.Video, 10000.0, 0.0, 0.0, S.WorldMinutes());
		const ss::town::Talk Overtaken = ss::town::Open(*ss::town::ContextFor(S, "sal", ""));
		std::printf("  econ record: 10,000 followers and 15,000 subscribers: \"%s\"; and 25,000: \"%s\"\n", Mostly.Line.c_str(), Overtaken.Line.c_str());
		Expect(Mostly.Topic == "look:famous" && EconHas(Mostly.Line, "stream") && Overtaken.Topic == "look:famous" && EconHas(Overtaken.Line, "videos"),
			"econ record: the line names the videos once their subscribers, at half a follower each, outweigh the stream's followers");
		for (const ss::town::Talk* T : {&Sal, &Lonnie})
		{
			const std::string Why = EconLint(T->Line);
			Expect(Why.empty(), ("econ record: the videos' line passes the lint (" + Why + ")").c_str());
		}
	}
}
} // namespace econ_seg

void SegEcon()
{
	const auto T0 = std::chrono::steady_clock::now();
	econ_seg::EconPace();
	econ_seg::EconPaceSession();
	econ_seg::EconFreshTalk();
	econ_seg::EconFreshYou();
	econ_seg::EconSeasonPastHundred();
	econ_seg::EconEstablished();
	// G2: Rerun on Kast and in the public record.
	econ_seg::EconBuzz();
	econ_seg::EconBuzzSession();
	econ_seg::EconMomentLog();
	econ_seg::EconMomentLink();
	econ_seg::EconClipFollows();
	econ_seg::EconRecord();
	std::printf("  econ: core-econ's checks %.1f s\n", econ_seg::EconSecondsSince(T0));
}
} // namespace session_test
