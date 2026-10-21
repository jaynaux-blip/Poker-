// card_room (next_segment_spec §7.6, suite 13): core-room's tests.
//   RoomNightly      seeded 96-entrant Nightlies taken in turn from NightlySalts until the floors are met (twelve at
//                    most), the first's full cue log matching Tests/room_nightly.txt (room_test --write regenerates it);
//                    on each, the PA's order and numbers (the bubble named at one away, and only there); honesty (every
//                    "all in and a call, table N" has a TableRound{N, Called} that tick; every "seat open, N" has a bust
//                    at N; every named card was tabled; every claimed result is in the speaker's Story; no talk from
//                    folded players; no needle at a bust); with floors under every count those checks run over, the
//                    comparisons counted as pairs, on the nights together (a regenerated night can't empty one
//                    silently, and no one night has to have it all); the director's cost over the nights' hands.
//   RoomHushAtHandForHand    the hand that takes the room to hand for hand is reactions only, chatter due or not.
//   RoomHushAtLoneFinalTable at a lone final table hand for hand (the mood FinalTable): no small talk, reactions still.
//   RoomAwayThenHandForHand  the floor says how far from the money, then hand for hand: one announcement when the
//                    field jumps there; on the bubble at one away, walked down or jumped, before the burst; never at two;
//                    an empty-state resume after it doesn't say it again; the routine PA waits after the joined call;
//                    the bubble's applause after its call; at a lone table hand for hand without the dealers' decks,
//                    and without "All tables complete"; one place paid is "1 place is paid".
//   RoomRailOneVoice a backer's two lines in one hand's end a second and more apart; RoomShape's check by Take.
//   RoomRates        lines an hour per Sociability band at 0.5x, 1x and 1.5x.
//   RoomAsk          ss.Live.Cue Ask: the dealer answers the question after it (the average, the break).
//   RoomNeedleUp     no needle at a player up from their seat; a seated one gets them.
//   RoomCheckpoints  resumes after hands 0, 1, 7, N/3, N/2, 4N/5 and N-1 say the same rest of the night; a resume with
//                    no kept state repeats no milestone; Write and Read round-trip.
//   RoomPace         the same night at Live, Brisk and Fast pace says the same lines in the same hands.
//   RoomEquityScope  equity runs only for the player's table (and the feature table).
//   RoomTalkCopy     the copy lint over every inventory; the Back Room's eight word for word.
//   RoomVoices       VoicesFor, VoiceOf, and livevoice lines round-trip.
//   RoomFinalTable   a Sunday: introductions seat by seat with the counts on the plates and one honorific each; Dee
//                    on the PA for the champion; the player's backer at the rail through the whole night they win.
//   RoomRailRounds   a backer's three double-up lines said through twice, across a checkpoint: dealt fresh each time,
//                    never one twice running (RailListen).
//   RoomField        the lead's (tests/room_field.cpp): DescribeField against both old copies of the field code.
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "TestFontMetrics.h"
#include "ShortStack/AI/Profiles.h"
#include "ShortStack/Audio/Voice.h"
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#include "ShortStack/Tournament.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/Captions.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace room_test
{
namespace room = ss::room;
namespace talk = ss::talk;
namespace live = ss::live;
namespace life = ss::life;
namespace world = ss::world;

int Failures = 0;
std::string NightOn; // the night a check is running on (RoomNightly's salts), named with a failure

void Expect(bool Condition, const std::string& What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s%s\n", What.c_str(), NightOn.empty() ? "" : (" [salt \"" + NightOn + "\"]").c_str());
	}
}

std::string ReadFile(const std::string& Path)
{
	std::string Out;
	if (FILE* F = std::fopen(Path.c_str(), "rb"))
	{
		char Buf[4096];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Out.append(Buf, N);
		}
		std::fclose(F);
	}
	std::string Clean;
	for (const char Ch : Out)
	{
		if (Ch != '\r')
		{
			Clean += Ch;
		}
	}
	return Clean;
}

bool WriteFile(const std::string& Path, const std::string& Text)
{
	FILE* F = std::fopen(Path.c_str(), "wb");
	if (!F)
	{
		return false;
	}
	std::fwrite(Text.data(), 1, Text.size(), F);
	std::fclose(F);
	return true;
}

std::vector<std::string> SplitLines(const std::string& S)
{
	std::vector<std::string> Out;
	size_t A = 0;
	while (A < S.size())
	{
		size_t B = S.find('\n', A);
		if (B == std::string::npos)
		{
			B = S.size();
		}
		Out.push_back(S.substr(A, B - A));
		A = B + 1;
	}
	return Out;
}

bool Starts(const std::string& S, const std::string& Head)
{
	return S.rfind(Head, 0) == 0;
}

bool Contains(const std::string& S, const std::string& Part)
{
	return S.find(Part) != std::string::npos;
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'A' && Ch <= 'Z')
		{
			Ch = static_cast<char>(Ch - 'A' + 'a');
		}
	}
	return S;
}

// ------------------------------------------------------------------ the night's people

life::LiveVoice MakeVoice(const std::string& Name, ss::Archetype Style, float Social, float Aggro, world::Identity Is, int Age)
{
	life::LiveVoice V;
	V.Name = Name;
	V.Style = static_cast<int>(Style);
	V.Social = Social;
	V.Aggro = Aggro;
	V.Grit = 0.5f;
	V.Is = static_cast<int>(Is);
	V.Age = Age;
	return V;
}

std::string StoryRecord(int Day, const std::string& Event, int Place, int Entries, ss::Chips Prize, bool Online = false)
{
	talk::Result R;
	R.Day = Day;
	R.Event = Event;
	R.Place = Place;
	R.Entries = Entries;
	R.Prize = Prize;
	R.Online = Online;
	return talk::StoryText({R});
}

/** One live night as the host would have it: the occurrence, the entry (roster, voices, faces, patches), the seed. */
struct Setup
{
	live::Occurrence O;
	life::LiveEntry E;
	std::string Seed;
	std::string Hero = "Jesse";
	std::string HeroFull = "Jesse Cole";
	std::vector<ss::ReservedPlayer> Known;
	std::vector<room::RailBacker> Rail;
	ss::Profile HeroProfile;
	int Entrants = 96;
	/** The host calls AllInCalled mid-hand (the Sunday's does; the Nightly's golden is the night without it). */
	bool Lean = false;
	/** A fresh world's books (update 2 §3.3 F9): the host's Director::Fresh. Blank false: an established world's night. */
	room::Books Books;
};

Setup MakeNight(const std::string& OccurrenceId, int Entrants, const std::string& HeroPatch, const std::string& Salt)
{
	Setup S;
	S.O = live::FindOccurrence(OccurrenceId);
	S.Entrants = Entrants;
	life::LiveEntry& E = S.E;
	E.Id = S.O.Id;
	E.Name = S.O.T->Name;
	E.Entrants = Entrants;
	E.RegisteredAt = S.O.DeskOpens + 25.0;
	const int Day = S.O.Day;
	using ss::Archetype;
	using world::Identity;
	// The cast, the room's own and a few known by their handles: who they are, what they can claim, who they know.
	std::vector<life::LiveVoice> Voices;
	life::LiveVoice Mei = MakeVoice("Mei", Archetype::Reg, 0.15f, 0.55f, Identity::DisciplinedPro, 34);
	Mei.Story = StoryRecord(Day - 2, "Embercrest Nightly $120", 2, 88, 214000);
	Mei.Ties = {{"Sonny", static_cast<int>(world::TieKind::Friend)}};
	Voices.push_back(Mei);
	life::LiveVoice Rick = MakeVoice("Rick", Archetype::Lag, 0.75f, 0.75f, Identity::WealthyAmateur, 47);
	Rick.Mood = static_cast<int>(world::Momentum::Heater);
	Rick.Rivalry = 0.45f;
	Rick.Familiarity = 0.4f;
	Voices.push_back(Rick);
	Voices.push_back(MakeVoice("Dre", Archetype::Station, 0.9f, 0.4f, Identity::Recreational, 31));
	Voices.push_back(MakeVoice("Sal", Archetype::Nit, 0.35f, 0.2f, Identity::LiveRegular, 71));
	Voices.push_back(MakeVoice("Twitch", Archetype::Maniac, 0.5f, 0.85f, Identity::Recreational, 26));
	life::LiveVoice Ghost = MakeVoice("gh0stfold", Archetype::Crusher, 0.3f, 0.7f, Identity::OnlineGrinder, 24);
	Ghost.Rivalry = 0.6f;
	Voices.push_back(Ghost);
	static const char* Locals[] = {"Boots", "T.J.", "Maria G.", "Omar", "Lin", "Duke", "Bev", "Sonny", "J.P.", "Rosie", "Hector", "Gloria", "Vince",
		"Tammy", "Earl", "Danny Q.", "Priya", "Walt", "Connie", "Nina", "Big Sam", "Kevin", "Lorraine", "Teddy"};
	static const Archetype Styles[] = {Archetype::Fish, Archetype::Station, Archetype::Tag, Archetype::Lag, Archetype::Reg, Archetype::Nit};
	int K = 0;
	for (const char* Name : Locals)
	{
		const live::RoomLocal* L = live::FindRoomLocal(Name);
		const Archetype Style = Styles[K % 6];
		life::LiveVoice V = MakeVoice(Name, Style, 0.2f + 0.07f * static_cast<float>(K % 11), 0.25f + 0.06f * static_cast<float>(K % 9),
			K % 4 == 0 ? Identity::LiveRegular : K % 7 == 3 ? Identity::OnlineGrinder : Identity::Recreational, L ? L->Age : 40);
		if (K % 3 == 0)
		{
			V.Story = StoryRecord(Day - 1 - K % 5, K % 2 == 0 ? "Noon Deepstack $80" : "Embercrest Nightly $120", 4 + K % 9, 60 + K * 2, 16000 + K * 2500);
		}
		if (K % 5 == 2)
		{
			V.Story += std::string(V.Story.empty() ? "" : "|") + StoryRecord(Day - 3, "Saturday Big Stack $200", 16, 106, 0);
		}
		V.Mood = K % 6 == 1 ? static_cast<int>(world::Momentum::Downswing) : K % 6 == 4 ? static_cast<int>(world::Momentum::Heater) : 0;
		V.Staked = K % 8 == 5;
		V.Familiarity = K % 4 == 1 ? 0.3f : 0.0f;
		V.Respect = K % 5 == 0 ? 0.35f : 0.0f;
		Voices.push_back(V);
		++K;
	}
	Voices[static_cast<size_t>(6 + 1)].Ties = {{"Boots", static_cast<int>(world::TieKind::TravelPartner)}}; // T.J. and Boots ride up together
	Voices[static_cast<size_t>(6)].Ties = {{"T.J.", static_cast<int>(world::TieKind::TravelPartner)}};
	Voices[static_cast<size_t>(6 + 7)].Ties = {{"Mei", static_cast<int>(world::TieKind::Friend)}}; // Sonny
	Voices[static_cast<size_t>(6 + 5)].Ties = {{"Vince", static_cast<int>(world::TieKind::Rival)}}; // Duke and Vince
	Voices[static_cast<size_t>(6 + 12)].Ties = {{"Duke", static_cast<int>(world::TieKind::Rival)}};
	for (const char* Handle : {"olgapetrov", "mike_1987", "VelvetRiver", "DmitriVolkov"})
	{
		life::LiveVoice V = MakeVoice(Handle, Archetype::Reg, 0.45f, 0.55f, Identity::OnlineGrinder, 29);
		if (std::string(Handle) == "mike_1987")
		{
			V.Story = StoryRecord(Day - 1, "RiverLine Daily $22", 7, 410, 61000, true);
		}
		Voices.push_back(V);
	}
	for (life::LiveVoice& V : Voices)
	{
		E.Roster.push_back({V.Name, V.Style});
		S.Known.push_back({V.Name, static_cast<Archetype>(V.Style)});
	}
	E.Voices = Voices;
	E.Faces = {{"Rick", "Rival", "Look who's back. Try not to cry this time."},
		{"Boots", "Knows you", "Hey, it's the kid from the bus. Good luck tonight."},
		{"Sonny", "Knows you", "Evening. Save some chips for the rest of us."},
		{"Maria G.", "Embercrest regular", ""}};
	if (!HeroPatch.empty())
	{
		E.Patches.push_back({S.Hero, HeroPatch});
	}
	E.Patches.push_back({"Omar", "voltrush"});
	S.Seed = live::SeedFor(E, S.Hero) + Salt;
	ss::Rng R("hero-profile");
	S.HeroProfile = ss::MakeProfile(Archetype::Crusher, R);
	S.Rail = {{"Mrs. Park", 150}};
	return S;
}

// ------------------------------------------------------------------ the host's note of a hand (BackRoomTable.cpp's NoteHand)

room::HandNote NoteOf(const ss::Tournament& T, const ss::Hand& H, int Tick)
{
	room::HandNote N;
	N.Hand = Tick;
	N.Pot = 0;
	N.Board = H.Board;
	auto NameOf = [&T](const ss::HandSeat& S) -> std::string {
		if (S.Id == ss::HeroId)
		{
			return ss::HeroId;
		}
		const int I = T.PlayerIndex(S.Id);
		return I >= 0 ? T.Players[static_cast<size_t>(I)].Name : S.Id;
	};
	const ss::HandSeat* Mine = nullptr;
	for (const ss::HandSeat& S : H.Seats)
	{
		(S.Folded ? N.Folded : N.InHand).push_back(NameOf(S));
		if (S.StartStack > 0 && S.Stack <= 0)
		{
			N.Busted.push_back(NameOf(S));
		}
		if (S.Id == ss::HeroId)
		{
			Mine = &S;
		}
	}
	N.HeroStack = Mine ? Mine->Stack : 0;
	if (H.LastAggressor >= 0 && H.LastAggressor < static_cast<int>(H.Seats.size()))
	{
		N.Aggressor = NameOf(H.Seats[static_cast<size_t>(H.LastAggressor)]);
	}
	for (const ss::ActionRecord& A : H.History)
	{
		if (A.AllIn && !N.AllIn)
		{
			N.AllIn = true;
			const size_t Seen = A.OnStreet == ss::Street::Preflop ? 0 : A.OnStreet == ss::Street::Flop ? 3 : A.OnStreet == ss::Street::Turn ? 4 : 5;
			N.BoardAtAllIn.assign(H.Board.begin(), H.Board.begin() + static_cast<std::ptrdiff_t>(std::min(Seen, H.Board.size())));
		}
		if (Mine && A.Seat == Mine->Seat && A.Type == ss::ActionType::Fold && A.Facing > 0 && (A.OnStreet != ss::Street::Preflop || A.Facing + A.To > H.BigBlind))
		{
			N.HeroFoldedToRaise = true;
		}
	}
	bool Showdown = false;
	for (const ss::HandEvent& Ev : H.Events)
	{
		if (Ev.Type == ss::EventType::Showdown)
		{
			Showdown = true;
			for (const ss::ShowdownHand& SH : Ev.Hands)
			{
				const ss::HandSeat* HS = H.SeatByNumber(SH.Seat);
				N.Shown.push_back({HS ? NameOf(*HS) : std::to_string(SH.Seat), SH.Hole});
			}
		}
	}
	N.Called = N.AllIn && Showdown && N.Shown.size() >= 2;
	for (const ss::PotResult& R : H.PotResults)
	{
		N.Pot += R.Amount;
		for (const int W : R.Winners)
		{
			const ss::HandSeat* HS = H.SeatByNumber(W);
			const std::string Who = HS ? NameOf(*HS) : std::to_string(W);
			if (std::find(N.Winners.begin(), N.Winners.end(), Who) == N.Winners.end())
			{
				N.Winners.push_back(Who);
			}
		}
	}
	// The player's own tells, scripted: a long think now and then, and a bluff shown once in a while.
	N.HeroThink = Tick % 23 == 7 ? 31.0 : 4.0;
	N.HeroShowedBluff = Tick % 41 == 13 && !Showdown && std::find(N.Winners.begin(), N.Winners.end(), std::string(ss::HeroId)) != N.Winners.end();
	return N;
}

// ------------------------------------------------------------------ playing a night

struct TickFacts
{
	std::vector<ss::TableRound> Rounds;
	std::vector<ss::TEvent> Events;
	bool HadNote = false;
	room::HandNote Note;
	int HeroTable = -1;
	bool HandForHandAtHand = false;
	double Average = 0.0;
	int Remaining = 0;
	int Paid = 0;
	ss::Level Blinds;
	std::map<int, ss::Chips> FinalSeats; // seat -> stack, when this tick made the final table
	std::vector<std::pair<std::string, ss::Chips>> Stacks; // everyone alive (name, stack)
	double World = 0.0; // world minutes at the hand's end (the director's TickWorld)
};

struct Saved
{
	std::string Checkpoint;
	std::string Room;
	double World = 0.0;
};

struct Night
{
	std::vector<std::string> Log;          // the cue log, one line a cue (and the mood as it turns)
	std::vector<std::pair<int, room::Cue>> Cues;
	// The Take each cue came in (parallel to Cues). A cue's Delay counts from its own Take: the start of a hand (the
	// balance, the final table) is one, its end another, so two cues' delays only compare within one Take, never by tick.
	std::vector<int> TakeOf;
	int Takes = 0;
	std::map<int, TickFacts> Facts;
	std::map<int, Saved> Saves;
	int Ticks = 0;
	int HeroPlace = 0;
	std::vector<int> EquityTables;
	std::set<int> HeroTables;
	int EquityHands = 0;
	double DirectorMs = 0.0; // the director's own time, every call of the night
	double WorstMs = 0.0;   // its slowest hand end
	double PlainMs = 0.0;   // its time on hands with no equity to run
	int PlainTicks = 0;
	std::string Winner;
	size_t MaxRoomText = 0;
	std::map<std::string, std::string> NameOfId; // tournament id -> the name a hand note uses
	int Leans = 0;                               // AllInCalled calls (Setup::Lean)
};

template <typename F>
double Timed(F&& Fn)
{
	const auto A = std::chrono::steady_clock::now();
	Fn();
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - A).count();
}

double WorldAt(const Setup& S, const ss::Tournament& T)
{
	return S.O.Start + (T.ClockMinutes() - T.Spec.StartClock);
}

const char* KindName(room::Kind K)
{
	switch (K)
	{
	case room::Kind::Line: return "LINE";
	case room::Kind::Call: return "CALL";
	case room::Kind::Pa: return "PA";
	case room::Kind::Vocal: return "VOCAL";
	case room::Kind::Phone: return "PHONE";
	}
	return "?";
}

const char* FromName(room::From F)
{
	switch (F)
	{
	case room::From::HeroTable: return "here";
	case room::From::Table: return "table";
	case room::From::FeatureTable: return "feature";
	case room::From::Rail: return "rail";
	case room::From::Bar: return "bar";
	case room::From::Desk: return "desk";
	case room::From::Pa: return "pa";
	}
	return "?";
}

std::string CueLine(int Tick, const room::Cue& C)
{
	char Head[96];
	std::snprintf(Head, sizeof(Head), "%d\t%s\t%s\tt%d\ts%d\t", Tick, KindName(C.What), FromName(C.Where), C.TableId, C.Seat);
	char Tail[64];
	std::snprintf(Tail, sizeof(Tail), "\tp%d\td%.1f\t%s%s%s", C.Priority, C.Delay, C.ToHero ? "to-you " : "", C.Chime ? "chime " : "", C.Effort >= 0.85f ? "shout" : "");
	return std::string(Head) + C.Speaker + "\t" + C.Key + "\t" + (C.What == room::Kind::Vocal ? C.Sound : C.Text) + Tail;
}

/**
 * Plays S's night from the start (StartAt < 0) or from a saved point. ClockCalls: the host's Clock calls a hand (its
 * pace: 9 Live, 4 Brisk, 1 Fast). The cue log is every cue in Take's order, then the mood when it turns.
 */
Night Play(const Setup& S, int ClockCalls, float TalkScale, int StartAt = -1, const Saved* From = nullptr)
{
	Night N;
	std::unique_ptr<ss::Tournament> T = live::MakeField(S.O, S.Entrants, S.Hero, S.Seed, S.Known);
	room::Director D(S.E, S.O, S.Seed + ":room", S.Hero);
	for (const ss::TPlayer& P : T->Players)
	{
		N.NameOfId[P.Id] = P.IsHero ? std::string(ss::HeroId) : P.Name;
	}
	D.HeroFullName = S.HeroFull;
	D.TalkScale = TalkScale;
	D.SetRail(S.Rail);
	D.Fresh = S.Books;
	room::Mood Last = D.Now();
	std::vector<room::Mood> Turned; // the mood as it turned during the tick, after every call
	auto Watch = [&]() {
		if (D.Now() != Last)
		{
			Last = D.Now();
			Turned.push_back(Last);
		}
	};
	auto Drain = [&](int Tick) {
		const int Take = N.Takes++;
		for (const room::Cue& C : D.Take())
		{
			N.Log.push_back(CueLine(Tick, C));
			N.Cues.push_back({Tick, C});
			N.TakeOf.push_back(Take);
		}
		for (const room::Mood M : Turned)
		{
			N.Log.push_back(std::to_string(Tick) + "\tMOOD\t" + room::MoodName(M));
		}
		Turned.clear();
	};
	if (From)
	{
		Expect(T->Restore(From->Checkpoint), "room checkpoints: the tournament restores");
		D.Resume(*T, From->Room, From->World);
		Last = D.Now();
	}
	else
	{
		// Before cards: in the room early, the desk opening and the ten-minute call; then cards in the air.
		for (const double At : {S.O.DeskOpens + 5.0, S.O.Start - 60.0, S.O.Start - 9.0, S.O.Start})
		{
			D.Clock(*T, At, 0.0);
			Watch();
		}
		Drain(0);
		N.Saves[0] = {T->Checkpoint(), D.Write(), S.O.Start};
	}
	(void)StartAt;
	for (int Guard = 0; Guard < 4000 && !T->bFinished; ++Guard)
	{
		TickFacts F;
		F.HeroTable = T->Hero().Busted ? -1 : T->Hero().TableId;
		std::vector<ss::TEvent> Start;
		std::unique_ptr<ss::Hand> H = T->StartTick(Start);
		const size_t EquityBefore = D.EquityTables().size();
		double TickMs = Timed([&] { D.Events(*T, Start, WorldAt(S, *T)); });
		Watch();
		// The host plays what the start of the hand says as the hand starts (the final table's call before its first hand).
		Drain(T->Tick + 1);
		const bool FinalNow = std::any_of(Start.begin(), Start.end(), [](const ss::TEvent& E) { return E.Type == ss::TEventType::FinalTable; });
		if (FinalNow && !T->Tables.empty())
		{
			for (const int Idx : T->Tables.begin()->second.Seats)
			{
				if (Idx >= 0)
				{
					F.FinalSeats[T->Players[static_cast<size_t>(Idx)].Seat] = T->Players[static_cast<size_t>(Idx)].Stack;
				}
			}
		}
		const std::vector<ss::TEvent> Fin = T->FinishTick(H.get(), &S.HeroProfile);
		F.Rounds = T->Rounds();
		F.Events = Start;
		F.Events.insert(F.Events.end(), Fin.begin(), Fin.end());
		F.Average = T->AverageStack();
		F.Remaining = T->Remaining;
		F.Paid = T->PaidPlaces();
		F.Blinds = T->CurrentLevel();
		F.HandForHandAtHand = T->HandForHand();
		for (const ss::TPlayer& P : T->Players)
		{
			if (!P.Busted)
			{
				F.Stacks.push_back({P.Name, P.Stack});
			}
		}
		if (H)
		{
			F.HadNote = true;
			F.Note = NoteOf(*T, *H, T->Tick);
			F.World = WorldAt(S, *T);
			N.HeroTables.insert(F.HeroTable);
			const room::HandNote& Nt = F.Note;
			N.EquityHands += Nt.AllIn && Nt.Called && Nt.Board.size() == 5 && Nt.BoardAtAllIn.size() < 5 && Nt.Shown.size() >= 2 ? 1 : 0;
			if (S.Lean && Nt.AllIn && Nt.Called && Nt.Shown.size() >= 2)
			{
				// Mid-hand, as the host would: the action closed on an all-in and a call, the board still to come.
				const bool HeroIn = std::any_of(Nt.Shown.begin(), Nt.Shown.end(), [](const auto& Sh) { return Sh.first == ss::HeroId; });
				TickMs += Timed([&] { D.AllInCalled(*T, 0, HeroIn); });
				Watch();
				Drain(T->Tick);
				++N.Leans;
			}
			TickMs += Timed([&] { D.Hand(*T, F.Note); });
			Watch();
		}
		TickMs += Timed([&] { D.Rounds(*T, T->Rounds()); });
		Watch();
		TickMs += Timed([&] { D.Events(*T, Fin, WorldAt(S, *T)); });
		Watch();
		for (int K = 0; K < ClockCalls; ++K)
		{
			D.Clock(*T, WorldAt(S, *T) + 2.5 * K / std::max(1, ClockCalls), 0.0);
			Watch();
		}
		const bool Break = std::any_of(Fin.begin(), Fin.end(), [](const ss::TEvent& E) { return E.Type == ss::TEventType::Break; });
		if (Break)
		{
			// The host's break: 24 real seconds whatever the pace.
			for (int Left = 24; Left >= 0; Left -= 2)
			{
				D.Clock(*T, WorldAt(S, *T), static_cast<double>(Left));
				Watch();
			}
		}
		for (const ss::TEvent& E : Fin)
		{
			if (E.Type == ss::TEventType::Bust && E.IsHero)
			{
				N.HeroPlace = E.Place;
			}
			if (E.Type == ss::TEventType::Finished)
			{
				N.Winner = E.Name;
				if (E.Id == ss::HeroId)
				{
					N.HeroPlace = 1;
				}
			}
		}
		Drain(T->Tick);
		N.DirectorMs += TickMs;
		N.WorstMs = std::max(N.WorstMs, TickMs);
		if (D.EquityTables().size() == EquityBefore)
		{
			N.PlainMs += TickMs;
			++N.PlainTicks;
		}
		N.Facts[T->Tick] = F;
		const std::string Kept = D.Write();
		N.MaxRoomText = std::max(N.MaxRoomText, Kept.size());
		N.Saves[T->Tick] = {T->Checkpoint(), Kept, WorldAt(S, *T) + 2.5 * (ClockCalls - 1) / std::max(1, ClockCalls)};
		N.Ticks = T->Tick;
	}
	// After the cheer: the room winds down.
	for (int K = 0; K < 4; ++K)
	{
		D.Clock(*T, WorldAt(S, *T) + 2.5 + 1.0 * K, 0.0);
		Watch();
	}
	Drain(T->Tick);
	N.EquityTables = D.EquityTables();
	return N;
}

/** A whole announcement: the PA cue at I (its first part) and the parts that follow it, joined. */
std::string Whole(const Night& N, size_t I)
{
	const room::Cue& First = N.Cues[I].second;
	std::string Text = First.Text;
	int Want = 1;
	for (size_t J = I + 1; J < N.Cues.size() && N.Cues[J].first == N.Cues[I].first; ++J)
	{
		const room::Cue& C = N.Cues[J].second;
		if (C.What == room::Kind::Pa && C.Key == First.Key && C.Part == Want)
		{
			Text += " " + C.Text;
			++Want;
		}
	}
	return Text;
}

/**
 * The floor calls a PA cue carries, in the order they're said: its key's templates' pools ("pa.h4h.0" is pa.h4h; one
 * announcement of several, Director::PaTogether's "pa.away.0+pa.h4h.0", is pa.away then pa.h4h).
 */
std::vector<std::string> CallsOf(const std::string& Key)
{
	std::vector<std::string> Out;
	size_t A = 0;
	while (A <= Key.size())
	{
		size_t B = Key.find('+', A);
		if (B == std::string::npos)
		{
			B = Key.size();
		}
		const std::string One = Key.substr(A, B - A);
		Out.push_back(One.substr(0, One.rfind('.')));
		A = B + 1;
	}
	return Out;
}

/** A PA cue that carries the floor's call Pool (alone or in one announcement with others). */
bool Carries(const room::Cue& C, const std::string& Pool)
{
	if (C.What != room::Kind::Pa)
	{
		return false;
	}
	const std::vector<std::string> Calls = CallsOf(C.Key);
	return std::find(Calls.begin(), Calls.end(), Pool) != Calls.end();
}

std::vector<std::string> After(const Night& N, int Tick)
{
	std::vector<std::string> Out;
	for (const std::string& L : N.Log)
	{
		if (std::atoi(L.c_str()) > Tick)
		{
			Out.push_back(L);
		}
	}
	return Out;
}

std::string Field(const std::string& Line, int Which)
{
	size_t A = 0;
	for (int I = 0; I < Which; ++I)
	{
		A = Line.find('\t', A);
		if (A == std::string::npos)
		{
			return std::string();
		}
		++A;
	}
	const size_t B = Line.find('\t', A);
	return Line.substr(A, B == std::string::npos ? std::string::npos : B - A);
}

// ------------------------------------------------------------------ RailListen: a backer at the rail

/** What the backers at the rail said in a night (stake.rail, stake.break: stake::RailLine's lines), and the first slip. */
struct RailHeard
{
	int Lines = 0;
	std::map<int, int> Pools; // a pool (the talk::Moment it's for) -> its lines said
	int Fresh = 0;            // pools said through and dealt fresh
	std::string Problem;
};

/**
 * A backer's lines (stake::RailLine's pools: three a moment for what comes up more than once a night, two for a
 * break): never the same line twice running, and nothing from a pool again until that pool's been said through (the
 * director deals it fresh then, never with the line said last first). Short of that, no rail line twice in a night: the
 * double-ups all different. Each line's pool is the moment it's one of, from stake::RailLine itself: the first in
 * Moments whose lines have it. (A bad beat, a suckout and a cooler are one set of lines in RailLine, "rail.beat", so
 * they're one pool here: a line from it is the same line to a listener whichever of the three it was said for. Should
 * RailLine give a suckout or a cooler lines of its own, they're their own pools here with no change.)
 */
RailHeard RailListen(const std::string& EntryId, const std::vector<std::pair<int, room::Cue>>& Cues)
{
	RailHeard H;
	static const ss::stake::Book NoBook;
	static const talk::Moment Moments[] = {talk::Moment::Bubble, talk::Moment::InTheMoney, talk::Moment::DoubleUp, talk::Moment::FinalTable, talk::Moment::Lose,
		talk::Moment::Win, talk::Moment::BadBeat, talk::Moment::Suckout, talk::Moment::Cooler, talk::Moment::Rail, talk::Moment::Break};
	std::map<std::pair<std::string, std::string>, std::string> LastOfKey; // (backer, key) -> the line said last
	std::map<std::pair<std::string, int>, std::vector<std::string>> Round; // (backer, pool) -> said since the pool was dealt
	std::map<std::pair<std::string, int>, std::string> LastOfPool;         // (backer, pool) -> the line said last
	for (const auto& TC : Cues)
	{
		const room::Cue& C = TC.second;
		if (C.Where != room::From::Rail || (C.Key != "stake.rail" && C.Key != "stake.break"))
		{
			continue;
		}
		++H.Lines;
		const auto Problem = [&H, &C, &TC](const std::string& Why) {
			if (H.Problem.empty())
			{
				H.Problem = ": " + Why + " (hand " + std::to_string(TC.first) + ", " + C.Who + ": \"" + C.Text + "\")";
			}
		};
		std::string& Last = LastOfKey[{C.Who, C.Key}];
		if (Last == C.Text)
		{
			Problem("said twice running");
		}
		Last = C.Text;
		// Its pool: the moment whose lines it's one of (a break's are on the phone, stake.break; the rest stake.rail).
		int Pool = -1;
		size_t Size = 0;
		for (const talk::Moment M : Moments)
		{
			std::set<std::string> Lines;
			for (uint32_t Salt = 0; Salt < 16u; ++Salt)
			{
				Lines.insert(ss::stake::RailLine(NoBook, EntryId, C.Who, static_cast<int>(M), Salt));
			}
			if (Lines.count(C.Text) && (M == talk::Moment::Break) == (C.Key == "stake.break"))
			{
				Pool = static_cast<int>(M);
				Size = Lines.size();
				break;
			}
		}
		if (Pool < 0)
		{
			Problem("not one of the backer's lines");
			continue;
		}
		++H.Pools[Pool];
		std::vector<std::string>& Said = Round[{C.Who, Pool}];
		if (Said.size() >= Size)
		{
			Said.clear(); // said through: dealt fresh
			++H.Fresh;
		}
		if (std::find(Said.begin(), Said.end(), C.Text) != Said.end() || LastOfPool[{C.Who, Pool}] == C.Text)
		{
			Problem("said again before its pool was said through");
		}
		Said.push_back(C.Text);
		LastOfPool[{C.Who, Pool}] = C.Text;
	}
	return H;
}

RailHeard RailListen(const Setup& S, const Night& N)
{
	return RailListen(S.E.Id, N.Cues);
}

// ------------------------------------------------------------------ RoomShape: the night as it's heard

/**
 * Nobody at the rail says two things at once: a rail voice's lines in one Take start at least a second apart. The
 * director puts each line after the one before on the Take's talk cursor, the smallest gap the line before's speech (0.4
 * s and its characters at 15 a second) and 0.3 s (a shout or a cheer from the rail; a backer's line waits 0.9 s), so any
 * rail line of five characters and more clears the second. Within one Take only: Delays count from their own Take,
 * and the start of a hand (the final table's call) and its end (a level's rail read) are two Takes a hand apart that
 * share the hand's number in the log. Together: how many pairs it compared.
 */
struct RailVoices
{
	int Together = 0;
	bool OneVoice = true;
	std::string Problem;
};

RailVoices RailVoicesOf(const Night& N)
{
	RailVoices R;
	std::map<std::pair<int, std::string>, std::vector<size_t>> Said; // (Take, voice) -> their lines in it
	for (size_t I = 0; I < N.Cues.size(); ++I)
	{
		const room::Cue& C = N.Cues[I].second;
		if (C.What != room::Kind::Line || C.Where != room::From::Rail)
		{
			continue;
		}
		std::vector<size_t>& Before = Said[{N.TakeOf[I], C.Who}];
		for (const size_t J : Before)
		{
			const room::Cue& P = N.Cues[J].second;
			++R.Together;
			if (std::fabs(P.Delay - C.Delay) < 1.0)
			{
				R.OneVoice = false;
				if (R.Problem.empty())
				{
					char Buf[64];
					std::snprintf(Buf, sizeof(Buf), " (%.2f s and %.2f s)", P.Delay, C.Delay);
					R.Problem = ": hand " + std::to_string(N.Cues[I].first) + ", " + C.Who + ": " + P.Key + " \"" + P.Text + "\" and " + C.Key + " \"" + C.Text + "\"" + Buf;
				}
			}
		}
		Before.push_back(I);
	}
	return R;
}

/**
 * The order and spacing a listener hears: the break's five minutes; a seated player told of a move by their dealer;
 * one question to the dealer in six hands; the table reacting to the bubble after the floor names it; nobody at the
 * rail saying two things at once; no table shouting its own number the hand the player busts there; no line twice
 * running from one pool; "Hold!" never after the river; the champion called once the rail's ooh has landed; a backer
 * at the rail never repeating a line (RailListen). What each check ran over comes back, for the night's floors.
 */
struct ShapeHeard
{
	int HeroMoves = 0;    // the player's moves, each their dealer's to tell
	int Asks = 0;         // the dealer's answers to the table (the average, the count, the break)
	int AskPairs = 0;     // answers after another that night: what "once in six hands" compares
	int Bubbles = 0;      // the table's reactions to the bubble, each after the floor's call
	int RailTogether = 0; // a rail voice's lines heard in one Take with another of theirs (RailVoicesOf)
	int TableCalls = 0;   // the dealers' calls from other tables
	int ChampionOohs = 0; // the rail's oohs in the hand the champion is called
	RailHeard Rail;
};

ShapeHeard RoomShape(const Setup& S, const Night& N)
{
	ShapeHeard Heard;
	int Breaks = 0;
	int Fives = 0;
	int HeroMoves = 0;
	int DealerMoves = 0;
	int FloorMoves = 0;
	for (const auto& F : N.Facts)
	{
		const bool Final = std::any_of(F.second.Events.begin(), F.second.Events.end(), [](const ss::TEvent& E) { return E.Type == ss::TEventType::FinalTable; });
		for (const ss::TEvent& E : F.second.Events)
		{
			Breaks += E.Type == ss::TEventType::Break ? 1 : 0;
			HeroMoves += E.Type == ss::TEventType::Moved && E.IsHero && !Final ? 1 : 0;
		}
	}
	int LastAsk = -100;
	bool AsksApart = true;
	bool BubbleAfter = true;
	bool OwnNumber = true;
	bool NoHoldLate = true;
	bool ChampionAfter = true;
	std::map<std::string, std::string> LastOfPool;
	std::vector<std::string> Repeats;
	for (size_t I = 0; I < N.Cues.size(); ++I)
	{
		const int Tick = N.Cues[I].first;
		const room::Cue& C = N.Cues[I].second;
		const auto F = N.Facts.find(Tick);
		Fives += Starts(C.Key, "pa.five.minutes") ? 1 : 0;
		DealerMoves += Starts(C.Key, "dealer.move") && C.Where == room::From::HeroTable ? 1 : 0;
		FloorMoves += Starts(C.Key, "pa.moved") ? 1 : 0;
		if (Starts(C.Key, "dealer.average") || Starts(C.Key, "dealer.left") || Starts(C.Key, "dealer.break.next") || Starts(C.Key, "dealer.break.levels"))
		{
			++Heard.Asks;
			Heard.AskPairs += LastAsk > -100 ? 1 : 0;
			AsksApart = AsksApart && Tick - LastAsk >= 6;
			LastAsk = Tick;
		}
		if (Starts(C.Key, "field.bubble"))
		{
			++Heard.Bubbles;
			double PaEnds = -1.0;
			for (size_t J = 0; J < N.Cues.size(); ++J)
			{
				if (N.TakeOf[J] == N.TakeOf[I] && Starts(N.Cues[J].second.Key, "pa.bubble"))
				{
					PaEnds = std::max(PaEnds, N.Cues[J].second.Delay);
				}
			}
			BubbleAfter = BubbleAfter && PaEnds >= 0.0 && C.Delay > PaEnds;
		}
		if (Starts(C.Key, "dealer.table.") && F != N.Facts.end())
		{
			++Heard.TableCalls;
			OwnNumber = OwnNumber && C.TableId != F->second.HeroTable;
		}
		NoHoldLate = NoHoldLate && !Starts(C.Key, "field.rail.allin");
		if (Starts(C.Key, "pa.champion"))
		{
			for (size_t J = 0; J < N.Cues.size(); ++J)
			{
				const room::Cue& V = N.Cues[J].second;
				if (N.TakeOf[J] == N.TakeOf[I] && V.What == room::Kind::Vocal && V.Noise == ss::audio::Vocal::Ooh)
				{
					++Heard.ChampionOohs;
					ChampionAfter = ChampionAfter && C.Delay > V.Delay + 0.5;
				}
			}
		}
		// One pool's lines at the player's table: never the same one twice running.
		if (C.What == room::Kind::Line && Starts(C.Key, "field."))
		{
			const std::string Pool = C.Key.substr(0, C.Key.rfind('.'));
			if (LastOfPool[Pool] == C.Key)
			{
				Repeats.push_back(std::to_string(Tick) + " " + C.Key);
			}
			LastOfPool[Pool] = C.Key;
		}
	}
	Expect(Breaks > 0 && Fives == Breaks, "room shape: five minutes left is called on every break (" + std::to_string(Fives) + " of " + std::to_string(Breaks) + ")");
	Expect(DealerMoves == HeroMoves && FloorMoves == 0,
		"room shape: a seated player's move is their dealer's to tell, not the PA's (" + std::to_string(DealerMoves) + " of " + std::to_string(HeroMoves) + ")");
	Expect(AsksApart, "room shape: the table asks the dealer at most once in six hands");
	Expect(BubbleAfter, "room shape: the table reacts to the bubble after the floor names it");
	const RailVoices Voices = RailVoicesOf(N);
	Heard.RailTogether = Voices.Together;
	Expect(Voices.OneVoice, "room shape: nobody at the rail says two things at once" + Voices.Problem);
	Expect(OwnNumber, "room shape: no table call shouts the player's own table");
	Expect(NoHoldLate, "room shape: \"Hold!\" only while the cards turn (AllInCalled), never at the hand's end");
	Expect(ChampionAfter, "room shape: the floor calls the champion once the rail's ooh has landed");
	for (const std::string& R : Repeats)
	{
		std::printf("room shape: said twice running: %s\n", R.c_str());
	}
	Expect(Repeats.empty(), "room shape: no line twice running from one pool");
	// A backer's lines (stake::RailLine's pools, three a moment for what comes up more than once a night): nothing twice
	// running, and the double-ups and bad beats all different. (As many as this night's run gave the rail to say: the
	// player's luck decides it; RoomNightly's floor wants six and more, and RoomFinalTable's Sunday, the player winning
	// it, a whole night of them.)
	Heard.HeroMoves = HeroMoves;
	Heard.Rail = RailListen(S, N);
	Expect(Heard.Rail.Problem.empty(), "room shape: a backer at the rail never says a line twice running, nor a rail line twice in a night till its pool's said through (" +
		std::to_string(Heard.Rail.Lines) + " lines)" + Heard.Rail.Problem);
	return Heard;
}

// ------------------------------------------------------------------ RoomCaptionFit

/** Text widths from the Chromium metrics table (Roboto, the family Unreal's UI font uses), as ui_test measures. */
struct Roboto : ss::ui::TextMeasurer
{
	static const float* Ascii(ss::ui::Font F)
	{
		switch (F)
		{
		case ss::ui::Font::Bold: return test_font::BoldAscii;
		case ss::ui::Font::Mono: return test_font::MonoAscii;
		case ss::ui::Font::Black: return test_font::BlackAscii;
		case ss::ui::Font::Light: return test_font::LightAscii;
		default: return test_font::RegularAscii;
		}
	}
	float Width(const std::string& Text, ss::ui::Font Face, float SizePx) const override
	{
		float Sum = 0.0f;
		for (size_t I = 0; I < Text.size();)
		{
			const unsigned char C0 = static_cast<unsigned char>(Text[I]);
			const size_t Len = C0 >= 0xF0 ? 4 : C0 >= 0xE0 ? 3 : C0 >= 0xC0 ? 2 : 1;
			Sum += C0 >= 32 && C0 < 127 ? Ascii(Face)[C0 - 32] : 66.0f;
			I += Len;
		}
		return Sum * SizePx / 100.0f;
	}
	float Ascent(ss::ui::Font Face, float SizePx) const override
	{
		(void)Face;
		return test_font::RegularAscent * SizePx / 100.0f;
	}
};

/**
 * Every caption a night makes fits its box: two rows at most, never cut short with an ellipsis, at caption sizes 100,
 * 125 and 150% at 16:9 (1920 logical) and 2550x1220 (2257 logical). The PA comes in caption-sized parts for this.
 */
void RoomCaptionFit(const std::vector<const Night*>& Nights)
{
	const Roboto Metrics;
	std::set<std::string> Seen;
	int Checked = 0;
	std::vector<std::string> Cut;
	for (const Night* N : Nights)
	{
		for (const auto& TC : N->Cues)
		{
			const room::Cue& C = TC.second;
			if (C.What == room::Kind::Phone)
			{
				continue; // the toast lane, not a caption
			}
			ss::ui::Caption Cp;
			Cp.Speaker = C.Speaker;
			Cp.Line = C.What == room::Kind::Vocal ? C.Sound : C.Text;
			Cp.Style = C.What == room::Kind::Pa ? 1 : C.What == room::Kind::Vocal ? 2 : C.ToHero ? 3 : 0;
			Cp.Side = C.Where == room::From::HeroTable || C.Where == room::From::Pa ? 0 : 1;
			Cp.Room = C.Where == room::From::Table || C.Where == room::From::Rail || C.Where == room::From::FeatureTable;
			if (Cp.Line.empty() || !Seen.insert(std::to_string(Cp.Style) + Cp.Speaker + "|" + Cp.Line).second)
			{
				continue;
			}
			++Checked;
			for (const float W : {1920.0f, 2257.0f})
			{
				for (const int Size : {100, 125, 150})
				{
					ss::ui::CaptionStyle Look;
					Look.Mode = 2;
					Look.Size = Size;
					ss::ui::DrawList L;
					ss::ui::Canvas Cv(L, Metrics, W, 1080.0f, 1.0f);
					ss::ui::DrawCaptions(Cv, {Cp}, Look, 904.0f, std::min(980.0f, W - 840.0f), 0.5);
					for (const ss::ui::DrawCmd& Cmd : L.Cmds)
					{
						if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Contains(Cmd.Text.Text, "\xE2\x80\xA6"))
						{
							Cut.push_back(std::to_string(Size) + "% at " + std::to_string(static_cast<int>(W)) + ": " + Cp.Line);
						}
					}
				}
			}
		}
	}
	for (size_t I = 0; I < Cut.size() && I < 8; ++I)
	{
		std::printf("room captions: cut short at %s\n", Cut[I].c_str());
	}
	Expect(Checked > 100 && Cut.empty(), "room captions: every line the room says fits its caption box in two rows at 100, 125 and 150% (" + std::to_string(Cut.size()) +
		" cut of " + std::to_string(Checked) + ")");
}

// ------------------------------------------------------------------ RoomNightly

/**
 * What one night's checks ran over. RoomNightly's floors are on its nights' sum, so no one night has to have it all. A
 * check that compares one thing with another (the dealer's answers six hands apart, a needler's needles four apart) is
 * counted by the pairs it compared, not the things: two answers on two nights compare nothing.
 */
struct NightlyHeard
{
	int Averages = 0, Counts = 0, Busts = 0;
	int Calls = 0, SeatOpens = 0, Stories = 0, Beats = 0, Reactions = 0, Needles = 0, NeedlePairs = 0, HandForHandHands = 0, HandForHandLines = 0;
	int HeroMoves = 0, Asks = 0, AskPairs = 0, Bubbles = 0, TableCalls = 0, ChampionOohs = 0, RailLines = 0, RailTogether = 0, EquityHands = 0;
	int OneAway = 0; // nights the field was one from the money at a hand's end: the floor's bubble call checked
	double PlainMs = 0.0; // the director's time on hands with no equity to run, and how many (its cost, over the nights)
	int PlainTicks = 0;

	NightlyHeard& operator+=(const NightlyHeard& O)
	{
		Averages += O.Averages;
		Counts += O.Counts;
		Busts += O.Busts;
		Calls += O.Calls;
		SeatOpens += O.SeatOpens;
		Stories += O.Stories;
		Beats += O.Beats;
		Reactions += O.Reactions;
		Needles += O.Needles;
		NeedlePairs += O.NeedlePairs;
		HandForHandHands += O.HandForHandHands;
		HandForHandLines += O.HandForHandLines;
		HeroMoves += O.HeroMoves;
		Asks += O.Asks;
		AskPairs += O.AskPairs;
		Bubbles += O.Bubbles;
		TableCalls += O.TableCalls;
		ChampionOohs += O.ChampionOohs;
		RailLines += O.RailLines;
		RailTogether += O.RailTogether;
		EquityHands += O.EquityHands;
		OneAway += O.OneAway;
		PlainMs += O.PlainMs;
		PlainTicks += O.PlainTicks;
		return *this;
	}
};

/** A floor under one of NightlyHeard's counts, on the nights together: the check it counts runs over something. */
struct NightlyFloor
{
	const char* Name; // the count's name in the report (and a sweep's COUNTS line)
	int NightlyHeard::*Count;
	int Need;
	const char* Why; // what the floor makes sure of
};

const std::vector<NightlyFloor>& NightlyFloors()
{
	static const std::vector<NightlyFloor> All = {
		{"averages", &NightlyHeard::Averages, 1, "the table asks the dealer the average, so the answer is checked against the engine"},
		{"counts", &NightlyHeard::Counts, 1, "the table asks the dealer the count, so the answer is checked against the engine"},
		{"busts", &NightlyHeard::Busts, 1, "busts are called on the PA, so they're checked against the engine's"},
		{"calls", &NightlyHeard::Calls, 1, "all-in calls are called from other tables, so each is checked against the engine"},
		{"seats", &NightlyHeard::SeatOpens, 1, "open seats are called from other tables, so each is checked against the engine"},
		{"beats", &NightlyHeard::Beats, 1, "a bad beat is told, so the hands it names are checked against what was tabled"},
		{"stories", &NightlyHeard::Stories, 1, "someone claims a result, so it's checked against their Story"},
		{"reactions", &NightlyHeard::Reactions, 1, "the player's table reacts, so each reaction is checked against the hand"},
		{"needles", &NightlyHeard::Needles, 1, "the player is needled, so each needle is checked against the hand"},
		{"needlepairs", &NightlyHeard::NeedlePairs, 1, "a needler needles the player again in a night, so \"one needle per needler per 4 hands\" compares something"},
		{"h4hhands", &NightlyHeard::HandForHandHands, 1, "the player is seated at hand for hand"},
		{"h4hlines", &NightlyHeard::HandForHandLines, 1, "the player's table talks at hand for hand, so it's checked for reactions only"},
		{"moves", &NightlyHeard::HeroMoves, 1, "the player moves, so who tells them is checked"},
		{"askpairs", &NightlyHeard::AskPairs, 1, "the table asks the dealer again in a night, so \"at most once in six hands\" compares something"},
		{"bubbles", &NightlyHeard::Bubbles, 1, "the table reacts to the bubble, so it's checked against the floor's call"},
		{"tablecalls", &NightlyHeard::TableCalls, 1, "other tables call, so no call shouts the player's own table"},
		{"oohs", &NightlyHeard::ChampionOohs, 1, "the rail oohs in the champion's hand, so the floor's call is checked after it"},
		{"together", &NightlyHeard::RailTogether, 1, "a rail voice says two things in one go, so \"nobody at the rail says two things at once\" compares something"},
		{"raillines", &NightlyHeard::RailLines, 6, "the backer at the rail has six lines and more to say, all checked"},
		{"equity", &NightlyHeard::EquityHands, 1, "an all-in is called and tabled at the player's table, so the equity runs are checked"},
		{"oneaway", &NightlyHeard::OneAway, 1, "the field is one from the money, so the floor's bubble call is checked"},
	};
	return All;
}

/** Every count a night's checks ran over, by its floor's name ("averages=1 counts=0 ..."; the asks too, unfloored). */
std::string NightlyCounts(const NightlyHeard& H)
{
	std::string Out = "asks=" + std::to_string(H.Asks);
	for (const NightlyFloor& F : NightlyFloors())
	{
		Out += std::string(" ") + F.Name + "=" + std::to_string(H.*F.Count);
	}
	return Out;
}

/** The floors H's counts don't meet yet, as "name have/need" ("askpairs 0/1"); empty when every one is met. */
std::vector<std::string> NightlyShort(const NightlyHeard& H)
{
	std::vector<std::string> Out;
	for (const NightlyFloor& F : NightlyFloors())
	{
		if (H.*F.Count < F.Need)
		{
			Out.push_back(std::string(F.Name) + " " + std::to_string(H.*F.Count) + "/" + std::to_string(F.Need));
		}
	}
	return Out;
}

/**
 * Every check RoomNightly runs on a night but its golden and its floors: the PA's order and numbers, honesty, the shape
 * (RoomShape), equity scope, pace (the same night at Brisk and Fast) and checkpoints. What each check ran over comes
 * back for the floors.
 */
NightlyHeard NightlyChecks(const Setup& S, const Night& N)
{
	NightlyHeard Heard;
	std::printf("room nightly: %d hands, %d cues, the player %s, champion %s, liveroom at most %d bytes\n", N.Ticks, static_cast<int>(N.Cues.size()),
		N.HeroPlace == 1 ? "won it" : ("finished " + talk::Ordinal(N.HeroPlace)).c_str(), N.Winner.c_str(), static_cast<int>(N.MaxRoomText));
	Expect(N.Ticks > 60 && N.Cues.size() > 150, "room nightly: a whole night with plenty said");
	Expect(N.MaxRoomText <= 16 * 1024, "room nightly: the director's liveroom state stays under 16 KB (§10)");
	// The director's cost: printed a night, checked by RoomNightly over the nights' hands together (one night's hands
	// under a load spike aren't the director's cost).
	const double Plain = N.PlainMs / std::max(1, N.PlainTicks);
	std::printf("room nightly: the director takes %.4f ms a hand end (budget 0.05), %.4f with the night's %d equity runs averaged in, %.3f ms at most\n", Plain,
		N.DirectorMs / std::max(1, N.Ticks), static_cast<int>(N.EquityTables.size()), N.WorstMs);
	Heard.PlainMs = N.PlainMs;
	Heard.PlainTicks = N.PlainTicks;

	// The PA's order: each milestone once, in the night's order. One announcement of several calls (Director::PaTogether)
	// says each in turn, so each counts, in its place in the announcement.
	const std::vector<std::string> Order = {"pa.desk.open", "pa.ten.minutes", "pa.start", "pa.last.call", "pa.reg.closed", "pa.away", "pa.h4h", "pa.bubble",
		"pa.two.tables", "pa.final", "pa.headsup", "pa.champion"};
	std::map<std::string, int> FirstAt;
	std::map<std::string, int> Times;
	std::vector<int> OnBubbleTicks; // the hands the floor called the bubble (one away) on
	for (size_t I = 0; I < N.Cues.size(); ++I)
	{
		const room::Cue& C = N.Cues[I].second;
		if (C.What != room::Kind::Pa || C.Part != 0)
		{
			continue; // an announcement once, by its first part
		}
		const std::vector<std::string> Calls = CallsOf(C.Key);
		for (size_t K = 0; K < Calls.size(); ++K)
		{
			// The template's pool ("pa.bubble.seat.0" is "pa.bubble.seat"); the bubble, hand for hand (at a lone table) and the
			// champion each have their variants, and the away call's on-the-bubble form (a jump to one away) names the bubble.
			std::string Pool = Calls[K];
			if (Pool == "pa.bubble.seat")
			{
				Pool = "pa.bubble";
			}
			if (Pool == "pa.h4h.lone")
			{
				Pool = "pa.h4h";
			}
			if (Pool == "pa.champion.seat" || Pool == "pa.champion.dee.hero")
			{
				Pool = "pa.champion";
			}
			if (Pool == "pa.onbubble" || Pool == "pa.away.bubble")
			{
				OnBubbleTicks.push_back(N.Cues[I].first);
				if (!FirstAt.count("pa.onbubble"))
				{
					FirstAt["pa.onbubble"] = static_cast<int>(I * 8 + K);
				}
			}
			Pool = Pool == "pa.away.bubble" ? std::string("pa.away") : Pool;
			for (const std::string& M : Order)
			{
				if (Pool == M)
				{
					if (!FirstAt.count(M))
					{
						FirstAt[M] = static_cast<int>(I * 8 + K); // the announcement, then the call's place in it
					}
					++Times[M];
				}
			}
		}
	}
	// On the bubble: named once, at the first hand the field ends one from the money (walked down to it, or jumped there
	// and called with hand for hand), before the burst; and "on the bubble" never anywhere else (not at two away).
	int OneAwayAt = -1;
	for (const auto& F : N.Facts)
	{
		if (F.second.Remaining - F.second.Paid == 1)
		{
			OneAwayAt = F.first;
			break;
		}
	}
	Heard.OneAway = OneAwayAt >= 0 ? 1 : 0;
	const bool BubbleNamed = OneAwayAt < 0 ? OnBubbleTicks.empty()
										   : OnBubbleTicks == std::vector<int>{OneAwayAt} && (!FirstAt.count("pa.bubble") || FirstAt["pa.onbubble"] < FirstAt["pa.bubble"]);
	Expect(BubbleNamed, "room nightly: the floor names the bubble once, the hand the field is one from the money (" +
							(OneAwayAt < 0 ? std::string("never one away") : "one away at hand " + std::to_string(OneAwayAt)) + ", called " +
							std::to_string(OnBubbleTicks.size()) + (OnBubbleTicks.empty() ? std::string(" times") : " times, first at hand " + std::to_string(OnBubbleTicks[0])) + ")");
	for (size_t I = 0; I < N.Cues.size(); ++I)
	{
		const room::Cue& C = N.Cues[I].second;
		if (C.What != room::Kind::Pa || C.Part != 0 || !Contains(Lower(Whole(N, I)), "on the bubble"))
		{
			continue;
		}
		const auto F = N.Facts.find(N.Cues[I].first);
		Expect(F != N.Facts.end() && F->second.Remaining - F->second.Paid == 1, "room nightly: \"on the bubble\" only one from the money: " + Whole(N, I));
	}
	int Prev = -1;
	std::string PrevName;
	for (const std::string& M : Order)
	{
		Expect(Times[M] == 1, "room nightly: the floor says " + M + " exactly once (" + std::to_string(Times[M]) + ")");
		if (FirstAt.count(M))
		{
			Expect(FirstAt[M] > Prev, "room nightly: " + M + " comes after " + PrevName);
			Prev = FirstAt[M];
			PrevName = M;
		}
	}
	// Numbers from the engine.
	int Levels = 0;
	int Averages = 0;
	int Counts = 0;
	int Busts = 0;
	for (size_t I = 0; I < N.Cues.size(); ++I)
	{
		const auto& TC = N.Cues[I];
		const room::Cue& C = TC.second;
		const auto F = N.Facts.find(TC.first);
		if (C.What == room::Kind::Pa && C.Part != 0)
		{
			continue; // read whole, from its first part
		}
		const std::string Said = C.What == room::Kind::Pa ? Whole(N, I) : C.Text;
		if (Starts(C.Key, "pa.start.") && !Starts(C.Key, "pa.start.late") && !Starts(C.Key, "pa.start.closed"))
		{
			Expect(Contains(Said, "96 players"), "room nightly: the start names the field");
		}
		if (F == N.Facts.end())
		{
			continue;
		}
		if (Starts(C.Key, "pa.level"))
		{
			++Levels;
			Expect(Contains(Said, "now " + talk::Count(F->second.Blinds.Sb) + " and " + talk::Count(F->second.Blinds.Bb)), "room nightly: a level call says the engine's blinds: " + Said);
		}
		if (Starts(C.Key, "pa.reg.closed"))
		{
			Expect(Contains(Said, "top " + std::to_string(F->second.Paid) + " are paid"), "room nightly: registration closes with the engine's paid places");
		}
		if (Starts(C.Key, "dealer.average"))
		{
			++Averages;
			const long long Rounded = static_cast<long long>(std::llround(F->second.Average / 1000.0)) * 1000;
			Expect(C.Text == "Average is about " + talk::Count(Rounded) + ".", "room nightly: the dealer's average is the clock card's, rounded: " + C.Text);
		}
		if (Starts(C.Key, "dealer.left."))
		{
			++Counts;
			Expect(Starts(C.Text, std::to_string(F->second.Remaining) + " left. "), "room nightly: the dealer's count is the engine's: " + C.Text);
		}
		if (Starts(C.Key, "pa.bust"))
		{
			++Busts;
			bool Found = false;
			for (const ss::TEvent& E : F->second.Events)
			{
				Found = Found || (E.Type == ss::TEventType::Bust && Contains(Said, "Table " + std::to_string(E.TableId) + ",") && Contains(Said, talk::Ordinal(E.Place) + " place"));
			}
			Expect(Found, "room nightly: a bust on the PA is a bust the engine made that hand: " + Said);
		}
		if (Starts(C.Key, "pa.headsup"))
		{
			int Matched = 0;
			for (const auto& P : F->second.Stacks)
			{
				Matched += Contains(Said, talk::Count(P.second)) ? 1 : 0;
			}
			Expect(Matched == 2, "room nightly: heads-up says both stacks: " + Said);
		}
	}
	Expect(Levels >= 5, "room nightly: the levels are called");
	std::printf("room nightly: %d level calls, %d averages asked, %d counts asked, %d busts on the PA\n", Levels, Averages, Counts, Busts);
	Heard.Averages = Averages;
	Heard.Counts = Counts;
	Heard.Busts = Busts;

	// Honesty.
	std::set<std::string> TabledTonight;
	std::map<std::string, int> LastNeedle;
	int Needles = 0;
	int Stories = 0;
	int Beats = 0;
	int Calls = 0;
	int SeatOpens = 0;
	int Reactions = 0;
	int Noticed = 0;
	int HandForHandLines = 0; // lines from the player's table in the hands at hand for hand
	int HandForHandHands = 0; // hands at hand for hand with the player seated
	for (const auto& F : N.Facts)
	{
		HandForHandHands += F.second.HandForHandAtHand && F.second.HeroTable >= 0 ? 1 : 0;
	}
	std::map<std::string, const life::LiveVoice*> VoiceByName;
	for (const life::LiveVoice& V : S.E.Voices)
	{
		VoiceByName[V.Name] = &V;
	}
	int LastTick = -1;
	for (const auto& TC : N.Cues)
	{
		const int Tick = TC.first;
		const room::Cue& C = TC.second;
		const auto F = N.Facts.find(Tick);
		if (Tick != LastTick && F != N.Facts.end() && F->second.HadNote)
		{
			for (const auto& Sh : F->second.Note.Shown)
			{
				TabledTonight.insert(talk::SpokenHand(Sh.second));
			}
		}
		LastTick = Tick;
		if (Starts(C.Key, "dealer.table.called"))
		{
			++Calls;
			bool Ok = F != N.Facts.end();
			bool Found = false;
			for (const ss::TableRound& R : Ok ? F->second.Rounds : std::vector<ss::TableRound>())
			{
				Found = Found || (R.TableId == C.TableId && R.Called);
			}
			Expect(Found, "room honesty: \"" + C.Text + "\" has a TableRound{" + std::to_string(C.TableId) + ", Called} that tick");
		}
		if (Starts(C.Key, "dealer.table.seatopen"))
		{
			++SeatOpens;
			bool Found = false;
			for (const ss::TEvent& E : F != N.Facts.end() ? F->second.Events : std::vector<ss::TEvent>())
			{
				Found = Found || (E.Type == ss::TEventType::Bust && E.TableId == C.TableId);
			}
			Expect(Found, "room honesty: \"" + C.Text + "\" has a bust at table " + std::to_string(C.TableId) + " that tick");
		}
		if (Starts(C.Key, "field.badbeat") || Starts(C.Key, "field.break.beat"))
		{
			++Beats;
			int Named = 0;
			for (const std::string& Hand : TabledTonight)
			{
				Named += Contains(Lower(C.Text), Hand) ? 1 : 0;
			}
			Expect(Named >= 1, "room honesty: every named hand was tabled tonight: " + C.Text);
		}
		if (Starts(C.Topic, "story:"))
		{
			++Stories;
			const std::string Rest = C.Topic.substr(6);
			const std::string Who = Rest.substr(0, Rest.find(':'));
			const std::string Tail = Rest.substr(Rest.find(':') + 1);
			const int Day = std::atoi(Tail.c_str());
			const std::string Event = Tail.substr(Tail.find(':') + 1);
			bool Found = false;
			const auto V = VoiceByName.find(Who);
			if (V != VoiceByName.end())
			{
				for (const talk::Result& R : talk::StoryOf(V->second->Story))
				{
					Found = Found || (R.Day == Day && R.Event == Event && Contains(Lower(C.Text), talk::OrdinalWord(R.Place)));
				}
			}
			Expect(Found, "room honesty: a claimed result is in the speaker's Story: " + C.Topic + " / " + C.Text);
		}
		// Reactions come only from people who were in the hand.
		const bool Reaction = Starts(C.Key, "field.nicehand") || Starts(C.Key, "field.suckout") || Starts(C.Key, "field.badbeat") || Starts(C.Key, "field.cooler") ||
			Starts(C.Key, "field.win") || Starts(C.Key, "field.lose") || Contains(C.Key, ".win.") || Contains(C.Key, ".lose.");
		if (Reaction && C.What == room::Kind::Line && C.Where == room::From::HeroTable && F != N.Facts.end() && F->second.HadNote)
		{
			++Reactions;
			const room::HandNote& Nt = F->second.Note;
			bool In = false;
			for (const std::string& Name : Nt.InHand)
			{
				In = In || Name == (N.NameOfId.count(C.Who) ? N.NameOfId.at(C.Who) : C.Who);
			}
			Expect(In, "room honesty: no talk from folded players: " + C.Speaker + " (" + C.Key + ")");
		}
		if (Starts(C.Key, "field.needle"))
		{
			++Needles;
			const bool HeroOut = F != N.Facts.end() && std::any_of(F->second.Events.begin(), F->second.Events.end(), [](const ss::TEvent& E) {
				return E.Type == ss::TEventType::Bust && E.IsHero;
			});
			Expect(!HeroOut, "room honesty: no needle at a bust");
			const auto Prev2 = LastNeedle.find(C.Who);
			Heard.NeedlePairs += Prev2 != LastNeedle.end() ? 1 : 0;
			Expect(Prev2 == LastNeedle.end() || Tick - Prev2->second >= 4, "room honesty: one needle per needler per 4 hands (" + C.Speaker + ")");
			LastNeedle[C.Who] = Tick;
		}
		if (Starts(C.Key, "field.small.") || Starts(C.Key, "field.ask."))
		{
			Expect(F == N.Facts.end() || !F->second.HandForHandAtHand, "room honesty: hand for hand, reactions only (" + C.Key + ")");
		}
		HandForHandLines += C.What == room::Kind::Line && C.Where == room::From::HeroTable && F != N.Facts.end() && F->second.HandForHandAtHand ? 1 : 0;
		if (Starts(C.Topic, "noticed:"))
		{
			++Noticed;
		}
	}
	Expect(Noticed <= 1, "room noticed: the patch is noticed at most once a night");
	std::printf("room honesty: %d all-in calls elsewhere, %d seats open elsewhere, %d stories, %d bad beats, %d reactions, %d needles, %d noticed: all checked\n", Calls,
		SeatOpens, Stories, Beats, Reactions, Needles, Noticed);
	std::printf("room honesty: %d hands at hand for hand with the player seated, %d lines from their table in them: reactions only\n", HandForHandHands, HandForHandLines);
	Heard.Calls = Calls;
	Heard.SeatOpens = SeatOpens;
	Heard.Stories = Stories;
	Heard.Beats = Beats;
	Heard.Reactions = Reactions;
	Heard.Needles = Needles;
	Heard.HandForHandHands = HandForHandHands;
	Heard.HandForHandLines = HandForHandLines;
	const ShapeHeard Shape = RoomShape(S, N);
	std::printf("room shape: %d moves told by the dealer, %d answers to the table, %d reactions to the bubble, %d calls from other tables, %d oohs as the champion's "
				"called; %d rail lines (%d pools said through), %d heard together\n",
		Shape.HeroMoves, Shape.Asks, Shape.Bubbles, Shape.TableCalls, Shape.ChampionOohs, Shape.Rail.Lines, Shape.Rail.Fresh, Shape.RailTogether);
	Heard.HeroMoves = Shape.HeroMoves;
	Heard.Asks = Shape.Asks;
	Heard.AskPairs = Shape.AskPairs;
	Heard.Bubbles = Shape.Bubbles;
	Heard.TableCalls = Shape.TableCalls;
	Heard.ChampionOohs = Shape.ChampionOohs;
	Heard.RailLines = Shape.Rail.Lines;
	Heard.RailTogether = Shape.RailTogether;

	// RoomEquityScope: equity only at the player's table, and only once an all-in was called and tabled.
	bool Scoped = true;
	for (const int Table : N.EquityTables)
	{
		Scoped = Scoped && N.HeroTables.count(Table) > 0;
	}
	Expect(Scoped, "room equity scope: no equity for a table other than the player's (or the feature table)");
	Expect(static_cast<int>(N.EquityTables.size()) == N.EquityHands, "room equity scope: once per called all-in at the player's table (" + std::to_string(N.EquityTables.size()) + " of " +
		std::to_string(N.EquityHands) + ")");
	Heard.EquityHands = N.EquityHands;

	// RoomPace: Brisk and Fast say the same lines in the same hands.
	const Night Brisk = Play(S, 4, 1.0f);
	const Night Fast = Play(S, 1, 1.0f);
	for (const Night* Other : {&Brisk, &Fast})
	{
		size_t D = 0;
		while (D < N.Log.size() && D < Other->Log.size() && N.Log[D] == Other->Log[D])
		{
			++D;
		}
		if (Other->Log != N.Log)
		{
			std::printf("room pace: line %d differs\n  live: %s\n  now:  %s\n", static_cast<int>(D + 1), D < N.Log.size() ? N.Log[D].c_str() : "(end)",
				D < Other->Log.size() ? Other->Log[D].c_str() : "(end)");
		}
	}
	Expect(Brisk.Log == N.Log, "room pace: the same night at Brisk pace gives the same cue log");
	Expect(Fast.Log == N.Log, "room pace: the same night at Fast pace gives the same cue log");

	// RoomCheckpoints: resume after hands 0, 1, 7, N/3, N/2, 4N/5, N-1 and say the same rest of the night.
	const int Last = N.Ticks;
	for (const int At : {0, 1, 7, Last / 3, Last / 2, 4 * Last / 5, Last - 1})
	{
		const auto Kept = N.Saves.find(At);
		if (Kept == N.Saves.end())
		{
			Expect(false, "room checkpoints: a save after hand " + std::to_string(At));
			continue;
		}
		const Night Rest = Play(S, 9, 1.0f, At, &Kept->second);
		const std::vector<std::string> Want = After(N, At);
		size_t D = 0;
		while (D < Want.size() && D < Rest.Log.size() && Want[D] == Rest.Log[D])
		{
			++D;
		}
		if (Rest.Log != Want)
		{
			std::printf("room checkpoints: after hand %d, line %d differs\n  then: %s\n  now:  %s\n", At, static_cast<int>(D + 1), D < Want.size() ? Want[D].c_str() : "(end)",
				D < Rest.Log.size() ? Rest.Log[D].c_str() : "(end)");
		}
		Expect(Rest.Log == Want, "room checkpoints: resumed after hand " + std::to_string(At) + ", the rest of the night is the same");
		// Write and Read round-trip.
		room::Director Again(S.E, S.O, S.Seed + ":room", S.Hero);
		Expect(Again.Read(Kept->second.Room) && Again.Write() == Kept->second.Room, "room checkpoints: liveroom reads back and writes the same after hand " + std::to_string(At));
	}
	// A resume with nothing kept (a checkpoint from before the room kept its own) repeats no milestone.
	const std::set<std::string> Milestones = {"pa.start", "pa.last.call", "pa.reg.closed", "pa.away", "pa.onbubble", "pa.h4h", "pa.bubble", "pa.two.tables", "pa.final",
		"pa.intro", "pa.headsup", "pa.champion", "pa.desk.open", "pa.ten.minutes"};
	// The milestones a PA cue says: each template its key names (one announcement can say several; the away call's
	// on-the-bubble form says the bubble too).
	auto MilestonesOf = [&Milestones](const room::Cue& C) {
		std::vector<std::string> Out;
		size_t A = 0;
		while (C.What == room::Kind::Pa && A <= C.Key.size())
		{
			const size_t B = C.Key.find('+', A) == std::string::npos ? C.Key.size() : C.Key.find('+', A);
			const std::string Key = C.Key.substr(A, B - A);
			A = B + 1;
			for (const std::string& M : Milestones)
			{
				if (Key.rfind(M + ".", 0) == 0 && !(M == "pa.h4h" && (Key.rfind("pa.h4h.call", 0) == 0 || Key.rfind("pa.h4h.done", 0) == 0)))
				{
					Out.push_back(M);
				}
			}
			if (Key.rfind("pa.away.bubble.", 0) == 0)
			{
				Out.push_back("pa.onbubble");
			}
		}
		return Out;
	};
	for (const int At : {Last / 3, Last / 2, 4 * Last / 5, Last - 1})
	{
		Saved Bare = N.Saves.at(At);
		Bare.Room.clear();
		const Night Rest = Play(S, 9, 1.0f, At, &Bare);
		std::set<std::string> Before;
		for (const auto& TC : N.Cues)
		{
			for (const std::string& M : TC.first <= At ? MilestonesOf(TC.second) : std::vector<std::string>())
			{
				Before.insert(M);
			}
		}
		bool Repeats = false;
		for (const auto& TC : Rest.Cues)
		{
			for (const std::string& M : MilestonesOf(TC.second))
			{
				if (Before.count(M))
				{
					Repeats = true;
					std::printf("room checkpoints: an empty-state resume after hand %d repeated %s (%s)\n", At, M.c_str(), TC.second.Key.c_str());
				}
			}
		}
		Expect(!Repeats, "room checkpoints: an empty-state resume after hand " + std::to_string(At) + " repeats no milestone");
	}
	return Heard;
}

/**
 * The Nightly's salts (each a night's draw and cards), in the order RoomNightly takes them: the golden night first, then
 * nights that pass every per-night check, as many as it takes to meet the floors (NightlyCap at most). From round 4's
 * sweep of 1,847 salts (":20" to ":40", ":20a" to ":20z", ":41" to ":1840"; 1,399 pass every check NightlyChecks makes):
 * ":1782" (the golden: the player second, heads-up for the title), ":379" and ":1278" as before, then nine taken one at a
 * time for the floor the list had the fewest nights behind, so each floor has nine nights or more of the twelve (the
 * rail's ooh in the champion's hand is on 35 of the 1,399, a rail voice's two lines in one go on 64). Today ":1782" and
 * ":379" meet every floor. A change that moves every night (a bot's) meets them within twelve fresh nights about one
 * time in eleven when every night still passes; when the twelve fall short the failure names the floors short, and a
 * sweep's passing nights go here.
 */
const char* const NightlySalts[] = {":1782", ":379", ":1278", ":953", ":361", ":406", ":1641", ":57", ":758", ":824", ":1255", ":1521"};
constexpr size_t NightlyCap = 12;

void RoomNightly(bool WriteGolden)
{
	// A Tuesday Nightly, 96 runners. The player wears the Lucky Penny's patch; Mrs. Park holds 15% of them from the rail.
	// The nights come from NightlySalts in turn until the floors are met on them together, twelve at most; the golden is
	// the first's. Every night run passes every check NightlyChecks makes (the floors are the nights' together), so a bot or
	// director change that moves the nights needs no edit here unless the twelve no longer meet the floors (then a sweep
	// gives new passing nights: the failure says which floors are short).
	std::vector<std::unique_ptr<Night>> Nights;
	std::vector<std::string> Used;
	NightlyHeard All;
	for (const char* Salt : NightlySalts)
	{
		if (Used.size() >= NightlyCap || (!Used.empty() && NightlyShort(All).empty()))
		{
			break;
		}
		const Setup S = MakeNight("embercrest-nightly@15", 96, "luckypenny", Salt);
		Nights.push_back(std::make_unique<Night>(Play(S, 9, 1.0f)));
		Used.push_back(Salt);
		const Night& N = *Nights.back();
		NightOn = Salt;
		std::printf("room nightly: salt \"%s\"\n", Salt);
		if (Nights.size() == 1)
		{
			// The golden: the first night's whole cue log.
			std::string Text = "# room_nightly: the cue log of a seeded Embercrest Nightly (" + S.O.Id + ", 96 runners, seed \"" + S.Seed + ":room\").\n";
			Text += "# tick\tkind\twhere\ttable\tseat\tspeaker\tkey\ttext\tpriority\tdelay\tflags (room_test --write regenerates it)\n";
			for (const std::string& L : N.Log)
			{
				Text += L + "\n";
			}
			const std::string Path = std::string(SS_TESTS_DIR) + "/room_nightly.txt";
			if (WriteGolden)
			{
				Expect(WriteFile(Path, Text), "room nightly: the golden is written to " + Path);
				std::printf("room nightly: wrote %d lines to %s\n", static_cast<int>(N.Log.size()), Path.c_str());
			}
			else
			{
				const std::string Want = ReadFile(Path);
				const std::vector<std::string> A = SplitLines(Want);
				const std::vector<std::string> B = SplitLines(Text);
				size_t Diff = 0;
				while (Diff < A.size() && Diff < B.size() && A[Diff] == B[Diff])
				{
					++Diff;
				}
				const bool Same = A.size() == B.size() && Diff == A.size();
				if (!Same)
				{
					std::printf("room nightly: line %d differs\n  golden: %s\n  now:    %s\n", static_cast<int>(Diff + 1), Diff < A.size() ? A[Diff].c_str() : "(end)",
						Diff < B.size() ? B[Diff].c_str() : "(end)");
				}
				Expect(Same, "room nightly: the cue log matches Tests/room_nightly.txt (room_test --write regenerates it)");
			}
		}
		const NightlyHeard One = NightlyChecks(S, N);
		std::printf("room nightly: counts on \"%s\": %s\n", Salt, NightlyCounts(One).c_str());
		All += One;
		NightOn.clear();
	}
	std::vector<const Night*> Heard;
	for (const std::unique_ptr<Night>& N : Nights)
	{
		Heard.push_back(N.get());
	}
	RoomCaptionFit(Heard);
	std::string Which;
	for (const std::string& Salt : Used)
	{
		Which += (Which.empty() ? "" : ", ") + Salt;
	}
	const std::vector<std::string> Short = NightlyShort(All);
	std::printf("room nightly: %d of the list's %d nights %s: %s\n", static_cast<int>(Used.size()),
		static_cast<int>(std::min(NightlyCap, sizeof(NightlySalts) / sizeof(NightlySalts[0]))), Short.empty() ? "met the floors" : "run, the floors still short",
		Which.c_str());
	std::string Missing;
	for (const std::string& M : Short)
	{
		Missing += (Missing.empty() ? "" : ", ") + M;
	}
	Expect(Short.empty(), "room nightly: the floors are met within " + std::to_string(Used.size()) + " nights of NightlySalts (short: " + Missing +
							  "); the nights moved: add passing nights from a sweep to NightlySalts");
	// Floors, on the nights together: with nothing to run over, each check above passes on nothing.
	const std::string Over = " over the " + std::to_string(Nights.size()) + " nights";
	for (const NightlyFloor& F : NightlyFloors())
	{
		Expect(All.*F.Count >= F.Need, std::string("room nightly floor: ") + F.Why + " (" + F.Name + " " + std::to_string(All.*F.Count) + ", wants " + std::to_string(F.Need) + Over + ")");
	}
	// The director's cost, on every hand with no equity to run over the nights (one night under a load spike can't fail it).
	const double Plain = All.PlainMs / std::max(1, All.PlainTicks);
	std::printf("room nightly: the director takes %.4f ms a hand end over the %d nights' %d plain hands (budget 0.05)\n", Plain, static_cast<int>(Nights.size()), All.PlainTicks);
	Expect(Plain < 0.15, "room nightly: the director stays cheap (a hand end well under 0.15 ms over the nights' plain hands; the budget is 0.05)");
	std::printf("room nightly: over the %d nights, %s\n", static_cast<int>(Nights.size()), NightlyCounts(All).c_str());
}

// ------------------------------------------------------------------ RoomRates

void RoomRates()
{
	// One table of six, nothing but folded hands: what's said is the small talk, at each band's rate.
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@15");
	ss::TournamentSpec Spec = live::SpecFor(O, 6);
	Spec.BreakEvery = 0;
	const float Socials[5] = {0.12f, 0.37f, 0.62f, 0.87f, 0.95f};
	const char* Names[5] = {"Quiet One", "Steady Hand", "Chatty Cathy", "Loud Larry", "Motor Mouth"};
	std::vector<ss::ReservedPlayer> Known;
	life::LiveEntry E;
	E.Id = O.Id;
	for (int I = 0; I < 5; ++I)
	{
		life::LiveVoice V = MakeVoice(Names[I], ss::Archetype::Fish, Socials[I], 0.3f, world::Identity::Recreational, 40);
		E.Voices.push_back(V);
		E.Roster.push_back({Names[I], static_cast<int>(ss::Archetype::Fish)});
		Known.push_back({Names[I], ss::Archetype::Fish});
	}
	for (const float Scale : {0.5f, 1.0f, 1.5f})
	{
		ss::Tournament T(Spec, "Jesse", "rates", Known);
		T.LevelIndex = 6;
		room::Director D(E, O, "rates:room", "Jesse");
		D.TalkScale = Scale;
		D.Clock(T, O.Start, 0.0);
		D.Take();
		std::map<std::string, int> Lines;
		const int Hands = 800; // ten hours at Live pace
		for (int H = 1; H <= Hands; ++H)
		{
			T.Tick = H;
			room::HandNote N;
			N.Hand = H;
			for (const ss::TPlayer& P : T.Players)
			{
				N.Folded.push_back(P.IsHero ? std::string(ss::HeroId) : P.Name);
			}
			D.Hand(T, N);
			for (const room::Cue& C : D.Take())
			{
				if (C.What == room::Kind::Line && (Starts(C.Key, "field.small") || Starts(C.Key, "field.ask")))
				{
					++Lines[C.Who];
				}
			}
		}
		for (int I = 0; I < 5; ++I)
		{
			const double PerHour = static_cast<double>(Lines[std::string("npc:") + Names[I]]) / (Hands / 80.0);
			const double S = Socials[I];
			const double Lo = S < 0.25 ? 1.0 : S < 0.5 ? 3.0 : S < 0.75 ? 6.0 : 10.0;
			const double Hi = S < 0.25 ? 2.0 : S < 0.5 ? 5.0 : S < 0.75 ? 9.0 : 14.0;
			const bool In = PerHour >= Lo * Scale * 0.8 - 0.3 && PerHour <= Hi * Scale * 1.2 + 0.3;
			char Buf[200];
			std::snprintf(Buf, sizeof(Buf), "room rates: Sociability %.2f at %.1fx says %.1f lines an hour (band %.0f to %.0f, x%.1f)", S, Scale, PerHour, Lo, Hi, Scale);
			Expect(In, Buf);
			if (!In || I == 0 || I == 3)
			{
				std::printf("%s\n", Buf);
			}
		}
	}
}

// ------------------------------------------------------------------ RoomNoticed

void RoomNoticed()
{
	// The player wears the Lucky Penny's patch at a table with Boots, who knows them: he notices, once, in the store's
	// own words. At a table of strangers nobody does.
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@15");
	const ss::TournamentSpec Spec = live::SpecFor(O, 6);
	const char* Names[5] = {"Boots", "T.J.", "Lin", "Omar", "Duke"};
	for (const bool Knows : {true, false})
	{
		std::vector<ss::ReservedPlayer> Known;
		life::LiveEntry E;
		E.Id = O.Id;
		for (int I = 0; I < 5; ++I)
		{
			life::LiveVoice V = MakeVoice(Names[I], ss::Archetype::Fish, I == 0 ? 0.9f : 0.3f, 0.3f, world::Identity::Recreational, 50);
			V.Familiarity = I == 0 && Knows ? 0.3f : 0.0f;
			E.Voices.push_back(V);
			E.Roster.push_back({Names[I], static_cast<int>(ss::Archetype::Fish)});
			Known.push_back({Names[I], ss::Archetype::Fish});
		}
		E.Patches = {{"Jesse", "luckypenny"}};
		ss::Tournament T(Spec, "Jesse", "noticed", Known);
		T.LevelIndex = 6;
		room::Director D(E, O, "noticed:room", "Jesse");
		D.Clock(T, O.Start, 0.0);
		D.Take();
		if (Knows)
		{
			// ss.Live.Cue: a moment on demand, past the caps but not the honesty gates, and nothing the night keeps changes.
			const std::string Kept = D.Write();
			const bool Small = D.Prompt(T, talk::Moment::Small, -1);
			const bool Beat = D.Prompt(T, talk::Moment::BadBeat, -1);
			const std::vector<room::Cue> Said = D.Take();
			Expect(Small && !Beat && Said.size() == 1 && !Said[0].Text.empty() && D.Write() == Kept, "room prompt: ss.Live.Cue says a moment now, never a dishonest one, and keeps nothing");
		}
		int Noticed = 0;
		bool Right = true;
		for (int H = 1; H <= 300; ++H)
		{
			T.Tick = H;
			room::HandNote N;
			N.Hand = H;
			D.Hand(T, N);
			for (const room::Cue& C : D.Take())
			{
				if (Starts(C.Topic, "noticed:"))
				{
					++Noticed;
					Right = Right && C.Who == "npc:Boots" && C.ToHero && C.Topic == "noticed:luckypenny" &&
						(C.Text == "The corner store sponsors you?" || C.Text == "The Lucky Penny? You get free gum or what?");
				}
			}
		}
		if (Knows)
		{
			Expect(Noticed == 1 && Right, "room noticed: someone who knows the player notices the patch once, in the brand's own words (" + std::to_string(Noticed) + ")");
		}
		else
		{
			Expect(Noticed == 0, "room noticed: strangers don't remark on the patch");
		}
	}
}

// ------------------------------------------------------------------ RoomAsk and RoomNeedleUp

/** One table of six, the player and five of the room's regulars (Aggro: how hard they needle). */
struct SmallTable
{
	live::Occurrence O;
	ss::TournamentSpec Spec;
	life::LiveEntry E;
	std::vector<ss::ReservedPlayer> Known;
};

SmallTable MakeSmallTable(int BreakEvery, float Aggro)
{
	SmallTable S;
	S.O = live::FindOccurrence("embercrest-nightly@15");
	S.Spec = live::SpecFor(S.O, 6);
	S.Spec.BreakEvery = BreakEvery;
	S.E.Id = S.O.Id;
	for (const char* Name : {"Boots", "T.J.", "Lin", "Omar", "Duke"})
	{
		S.E.Voices.push_back(MakeVoice(Name, ss::Archetype::Lag, 0.5f, Aggro, world::Identity::Recreational, 50));
		S.E.Roster.push_back({Name, static_cast<int>(ss::Archetype::Lag)});
		S.Known.push_back({Name, ss::Archetype::Lag});
	}
	return S;
}

/**
 * ss.Live.Cue Ask (acceptance F1): someone at the player's table asks the dealer, and the dealer answers after them, as the
 * table's own questions are answered (the average; the break when it's close), and nothing the night keeps changes.
 */
void RoomAsk()
{
	struct Case
	{
		int BreakEvery;
		int Level;
		const char* Asked;
		const char* Answer;
	};
	for (const Case& K : {Case{0, 2, "field.ask.average", "dealer.average"}, Case{4, 3, "field.ask.break", "dealer.break.next"}})
	{
		const SmallTable S = MakeSmallTable(K.BreakEvery, 0.3f);
		ss::Tournament T(S.Spec, "Jesse", "ask", S.Known);
		T.LevelIndex = K.Level;
		T.Tick = 12;
		room::Director D(S.E, S.O, "ask:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		const std::string Kept = D.Write();
		const bool Asked = D.Prompt(T, talk::Moment::Ask, -1);
		const std::vector<room::Cue> Said = D.Take();
		const std::string Average = "Average is about " + talk::Count(static_cast<long long>(std::lround(T.AverageStack() / 1000.0)) * 1000) + ".";
		const bool Right = Asked && Said.size() == 2 && Said[0].What == room::Kind::Line && Starts(Said[0].Key, K.Asked) && Said[1].What == room::Kind::Call &&
			Starts(Said[1].Key, std::string(K.Answer) + ".") && Said[1].Where == room::From::HeroTable && Said[1].TableId == T.Hero().TableId && Said[1].Delay > Said[0].Delay + 0.5 &&
			Said[1].Text == (K.BreakEvery == 0 ? Average : std::string("Break after this level.")) && D.Write() == Kept;
		Expect(Right, std::string("room ask: ss.Live.Cue Ask gets the dealer's answer after the question (") + K.Answer + ": " +
						  (Said.size() > 1 ? Said[0].Text + " / " + Said[1].Text : std::to_string(Said.size()) + " cues") + ")");
	}
}

/**
 * The player folds to the same raiser hand after hand (acceptance F16): seated, the raiser needles them now and then; up
 * from their seat, where the dealer mucks for them, nobody does.
 */
void RoomNeedleUp()
{
	for (const bool Up : {false, true})
	{
		const SmallTable S = MakeSmallTable(0, 0.95f);
		ss::Tournament T(S.Spec, "Jesse", "needle", S.Known);
		room::Director D(S.E, S.O, "needle:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		D.HeroUp = Up;
		std::string Raiser;
		for (const ss::TPlayer& P : T.Players)
		{
			Raiser = Raiser.empty() && !P.IsHero && P.TableId == T.Hero().TableId ? P.Name : Raiser;
		}
		int Needles = 0;
		for (int H = 1; H <= 80; ++H)
		{
			T.Tick = H;
			room::HandNote N;
			N.Hand = H;
			N.Aggressor = Raiser;
			N.HeroFoldedToRaise = true;
			for (const ss::TPlayer& P : T.Players)
			{
				(P.IsHero ? N.Folded : N.InHand).push_back(P.IsHero ? std::string(ss::HeroId) : P.Name);
			}
			D.Hand(T, N);
			for (const room::Cue& C : D.Take())
			{
				Needles += C.ToHero && Starts(C.Key, "field.needle") ? 1 : 0;
			}
		}
		Expect(Up ? Needles == 0 : Needles > 0, std::string("room needles: ") + (Up ? "none at a player up from their seat" : "a raiser needles the seated player") + " (" +
													 std::to_string(Needles) + ")");
	}
}

// ------------------------------------------------------------------ RoomMoves and RoomHeroHonor

std::vector<room::Cue> Keyed(const std::vector<room::Cue>& Cues, const std::string& Head)
{
	std::vector<room::Cue> Out;
	for (const room::Cue& C : Cues)
	{
		if (Starts(C.Key, Head))
		{
			Out.push_back(C);
		}
	}
	return Out;
}

/** A table breaking: seated, the player hears it from their dealer (Dee on a Sunday); up from the table, from the floor;
 *  at the final table from neither (the floor's call takes everyone over). Someone moved in is greeted or seated. */
void RoomMoves()
{
	for (const char* Id : {"embercrest-nightly@15", "embercrest@20"})
	{
		const live::Occurrence O = live::FindOccurrence(Id);
		const bool Sunday = std::string(Id) == "embercrest@20";
		ss::TournamentSpec Spec = live::SpecFor(O, 24);
		std::vector<ss::ReservedPlayer> Known;
		life::LiveEntry E;
		E.Id = O.Id;
		static const char* Names[] = {"Boots", "T.J.", "Lin", "Omar", "Duke", "Bev", "Sonny", "Rosie", "Vince", "Tammy", "Earl", "Walt"};
		for (const char* Name : Names)
		{
			E.Voices.push_back(MakeVoice(Name, ss::Archetype::Fish, 0.5f, 0.3f, world::Identity::Recreational, 50));
			E.Roster.push_back({Name, static_cast<int>(ss::Archetype::Fish)});
			Known.push_back({Name, ss::Archetype::Fish});
		}
		E.Faces = {{"Boots", "Knows you", "Hey, it's the kid from the bus."}};
		ss::Tournament T(Spec, "Jesse", "moves", Known);
		Expect(T.Tables.size() >= 2, "room moves: a field over more than one table");
		room::Director D(E, O, "moves:room", "Jesse");
		D.Clock(T, O.Start, 0.0);
		D.Take();
		T.Tick = 3;
		const int Mine = T.Hero().TableId;
		int Other = -1;
		for (const auto& Tb : T.Tables)
		{
			Other = Tb.first != Mine && Other < 0 ? Tb.first : Other;
		}
		ss::TEvent Mv;
		Mv.Type = ss::TEventType::Moved;
		Mv.IsHero = true;
		Mv.Id = ss::HeroId;
		Mv.From = Other;
		Mv.To = Mine;
		D.Events(T, {Mv}, O.Start + 8.0);
		const std::vector<room::Cue> Seated = D.Take();
		const std::vector<room::Cue> Told = Keyed(Seated, "dealer.move");
		const std::string Want = std::string(Sunday ? "You're moving, kid. Table " : "You're moving. Table ") + std::to_string(Mine) + ", seat " +
			std::to_string(live::SeatLabel(T.Hero().Seat)) + ". Grab your chips.";
		Expect(Told.size() == 1 && Told[0].Where == room::From::HeroTable && Told[0].Text == Want && Told[0].Speaker == (Sunday ? "Dee" : "Dealer") &&
				Keyed(Seated, "pa.moved").empty(),
			std::string("room moves: seated, the dealer tells the player (") + (Told.empty() ? "nothing" : Told[0].Text) + ")");
		T.Tick = 4;
		D.HeroUp = true;
		D.Events(T, {Mv}, O.Start + 10.0);
		const std::vector<room::Cue> Up = D.Take();
		Expect(Keyed(Up, "pa.moved").size() == 1 && Keyed(Up, "dealer.move").empty(), "room moves: up from the table, the floor calls it");
		D.HeroUp = false;
		T.Tick = 5;
		ss::TEvent Ft;
		Ft.Type = ss::TEventType::FinalTable;
		D.Events(T, {Mv, Ft}, O.Start + 12.0);
		const std::vector<room::Cue> Final = D.Take();
		Expect(Keyed(Final, "pa.moved").empty() && Keyed(Final, "dealer.move").empty() && !Keyed(Final, "pa.final.").empty(),
			"room moves: at the final table the floor's call is the only one");
		// Someone moved in: a face says their own hello to the player.
		T.Tick = 6;
		ss::TEvent In;
		In.Type = ss::TEventType::Moved;
		In.Id = T.Players[static_cast<size_t>(T.PlayerIndex("npc:Boots"))].Id;
		In.From = Other;
		In.To = Mine;
		D.Events(T, {In}, O.Start + 14.0);
		const std::vector<room::Cue> Hello = D.Take();
		Expect(Keyed(Hello, "face.greeting").size() == 1 && Keyed(Hello, "face.greeting")[0].Text == "Hey, it's the kid from the bus." && Keyed(Hello, "face.greeting")[0].ToHero,
			"room moves: a face moved in to the player's table says hello");
	}
}

/**
 * The order a listener hears: the table answers the bubble after the floor names it; the table where the player just
 * busted doesn't shout its own all-in at them; a line from a pool isn't said twice running (Remember deals it fresh).
 */
void RoomOrder()
{
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@15");
	ss::TournamentSpec Spec = live::SpecFor(O, 24);
	std::vector<ss::ReservedPlayer> Known;
	life::LiveEntry E;
	E.Id = O.Id;
	static const char* Names[] = {"Boots", "T.J.", "Lin", "Omar", "Duke", "Bev", "Sonny", "Rosie", "Vince", "Tammy", "Earl", "Walt", "Gloria", "Hector",
		"Connie", "Nina", "Teddy", "Carla", "Moe", "Fitz", "Deb", "Ozzie", "Gus"};
	for (const char* Name : Names)
	{
		E.Voices.push_back(MakeVoice(Name, ss::Archetype::Fish, 0.9f, 0.3f, world::Identity::Recreational, 50));
		E.Roster.push_back({Name, static_cast<int>(ss::Archetype::Fish)});
		Known.push_back({Name, ss::Archetype::Fish});
	}
	ss::Tournament T(Spec, "Jesse", "order", Known);
	room::Director D(E, O, "order:room", "Jesse");
	D.Clock(T, O.Start, 0.0);
	D.Take();
	// The bubble, at another table: the floor names them, then the sociable at the player's table answer.
	T.Tick = 50;
	std::string Bubble;
	for (const ss::TPlayer& P : T.Players)
	{
		Bubble = Bubble.empty() && !P.IsHero && P.TableId != T.Hero().TableId ? P.Name : Bubble;
	}
	ss::TEvent B;
	B.Type = ss::TEventType::Bubble;
	B.Name = Bubble;
	D.Events(T, {B}, O.Start + 150.0);
	double FloorDone = -1.0;
	std::vector<double> Answers;
	for (const room::Cue& C : D.Take())
	{
		FloorDone = Starts(C.Key, "pa.bubble") ? std::max(FloorDone, C.Delay) : FloorDone;
		if (Starts(C.Key, "field.bubble"))
		{
			Answers.push_back(C.Delay);
		}
	}
	bool After = !Answers.empty() && FloorDone >= 0.0;
	for (const double At : Answers)
	{
		After = After && At > FloorDone;
	}
	Expect(After, "room order: the table answers the bubble after the floor names it (" + std::to_string(Answers.size()) + " answers)");
	// The player busts at their table on an all-in and a call: that table's round is theirs, not another table's.
	T.Tick = 51;
	T.Remaining = T.PaidPlaces() + 5;
	const int Mine = T.Hero().TableId;
	T.Players[static_cast<size_t>(T.PlayerIndex(ss::HeroId))].Busted = true;
	ss::TableRound R;
	R.TableId = Mine;
	R.AllIn = true;
	R.Called = true;
	R.Showdown = true;
	R.Out = {ss::HeroId};
	D.Rounds(T, {R});
	const std::vector<room::Cue> Calls = D.Take();
	Expect(Keyed(Calls, "dealer.table.").empty(), "room order: the table the player busted at doesn't shout its own all-in");
	// One pool's lines never twice running, all of it used before any repeats.
	std::set<std::string> Told;
	std::vector<std::string> Said;
	for (uint32_t Salt = 0; Salt < 60; ++Salt)
	{
		talk::Context C;
		C.Self = &E.Voices[0];
		C.Salt = Salt * 2654435761u;
		C.Told = &Told;
		const talk::Said S = talk::Line(talk::Moment::Win, C);
		talk::Remember(Told, S);
		Said.push_back(S.Key);
	}
	bool Fresh = std::set<std::string>(Said.begin(), Said.begin() + 6).size() == 6;
	for (size_t I = 1; I < Said.size(); ++I)
	{
		Fresh = Fresh && Said[I] != Said[I - 1];
	}
	Expect(Fresh, "room order: a pool's six lines all come before any repeats, and never one twice running");
}

/** The player's own badge row (badge::HeroFieldPack, under the save's name, not in the roster) gives their honorific. */
void RoomHeroHonor()
{
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@15");
	const ss::TournamentSpec Spec = live::SpecFor(O, 6);
	std::vector<ss::ReservedPlayer> Known;
	life::LiveEntry E;
	E.Id = O.Id;
	for (const char* Name : {"Boots", "T.J.", "Lin", "Omar", "Duke"})
	{
		E.Voices.push_back(MakeVoice(Name, ss::Archetype::Fish, 0.4f, 0.3f, world::Identity::Recreational, 50));
		E.Roster.push_back({Name, static_cast<int>(ss::Archetype::Fish)});
		Known.push_back({Name, ss::Archetype::Fish});
	}
	ss::badge::Set Wall;
	ss::badge::Badge B;
	B.K = ss::badge::Kind::OnTheWall;
	B.Count = 2;
	Wall.Held.push_back(B);
	const std::string Honor = ss::badge::Honorific(Wall, ss::badge::Room);
	E.Badges = {{"grinder_3c", ss::badge::Pack(Wall)}};
	ss::Tournament T(Spec, "Jesse", "honor", Known);
	room::Director D(E, O, "honor:room", "Jesse");
	D.HeroFullName = "Jesse Cole";
	D.Clock(T, O.Start, 0.0);
	D.Take();
	T.Tick = 40;
	ss::TEvent Ft;
	Ft.Type = ss::TEventType::FinalTable;
	D.Events(T, {Ft}, O.Start + 120.0);
	std::string Said;
	for (const room::Cue& C : D.Take())
	{
		Said += Starts(C.Key, "pa.final.honor") ? C.Text : std::string();
	}
	Expect(!Honor.empty() && T.Tables.size() == 1 && Contains(Said, Honor) && Contains(Said, "Jesse Cole"),
		"room final table: the player's own badges give their honorific (" + Said + ")");
}

// ------------------------------------------------------------------ the money's edge: hand for hand, the floor, the rail

/** Twenty-four runners over three tables, a few places paid; the room's chattiest regulars and the player. */
struct NearMoney
{
	live::Occurrence O;
	ss::TournamentSpec Spec;
	life::LiveEntry E;
	std::vector<ss::ReservedPlayer> Known;
};

NearMoney MakeNearMoney()
{
	NearMoney S;
	S.O = live::FindOccurrence("embercrest-nightly@15");
	S.Spec = live::SpecFor(S.O, 24);
	S.Spec.BreakEvery = 0;
	S.E.Id = S.O.Id;
	static const char* Names[] = {"Boots", "T.J.", "Lin", "Omar", "Duke", "Bev", "Sonny", "Rosie", "Vince", "Tammy", "Earl", "Walt", "Gloria", "Hector",
		"Connie", "Nina", "Teddy", "Carla", "Moe", "Fitz", "Deb", "Ozzie", "Gus"};
	for (const char* Name : Names)
	{
		S.E.Voices.push_back(MakeVoice(Name, ss::Archetype::Fish, 0.9f, 0.3f, world::Identity::Recreational, 50));
		S.E.Roster.push_back({Name, static_cast<int>(ss::Archetype::Fish)});
		S.Known.push_back({Name, ss::Archetype::Fish});
	}
	return S;
}

/** A hand at the player's table that everyone folded. */
room::HandNote FoldedHand(const ss::Tournament& T, int Tick)
{
	room::HandNote N;
	N.Hand = Tick;
	for (const ss::TPlayer& P : T.Players)
	{
		if (!P.Busted && P.TableId == T.Hero().TableId)
		{
			N.Folded.push_back(P.IsHero ? std::string(ss::HeroId) : P.Name);
		}
	}
	return N;
}

/** Small talk or a question to the dealer (Chatter's): never at hand for hand. */
bool IsChatter(const room::Cue& C)
{
	return C.What == room::Kind::Line && (Starts(C.Key, "field.small.") || Starts(C.Key, "field.ask."));
}

/**
 * Hand for hand is reactions only from the hand that takes the room there (round 3: salt ":20i" hand 109, the floor's
 * "We're now hand for hand." at 5.7 s and then small talk at 6.4 s). The hand's busts are in the engine before the
 * director's Hand runs and its mood turns after, so Chatter reads the engine's T.HandForHand() as well as the mood. Two
 * directors with the same night behind them, four from the money: the first plays on till the table has something to say
 * (chatter due); the second plays the same hands, and that one is the hand whose busts took the field to two from the
 * money (the mood still the one before): the floor's call, and no small talk and no question to the dealer.
 */
void RoomHushAtHandForHand()
{
	const NearMoney S = MakeNearMoney();
	ss::Tournament T(S.Spec, "Jesse", "hush", S.Known);
	const int Paid = T.PaidPlaces();
	auto Fresh = [&S, &T]() {
		auto D = std::make_unique<room::Director>(S.E, S.O, "hush:room", "Jesse");
		D->TalkScale = 2.0f;
		T.Tick = 0;
		D->Clock(T, S.O.Start, 0.0);
		D->Take();
		return D;
	};
	auto PlayOn = [&S, &T, Paid](room::Director& D, int H) {
		T.Tick = H;
		T.Remaining = Paid + 4;
		D.Hand(T, FoldedHand(T, H));
		D.Rounds(T, {});
		D.Events(T, {}, S.O.Start + 0.75 * H);
		return D.Take();
	};
	std::unique_ptr<room::Director> First = Fresh();
	int Due = -1;
	std::string Said;
	for (int H = 1; H <= 120 && Due < 0; ++H)
	{
		for (const room::Cue& C : PlayOn(*First, H))
		{
			if (IsChatter(C) && Due < 0)
			{
				Due = H;
				Said = C.Key + " \"" + C.Text + "\"";
			}
		}
	}
	Expect(Due > 0, "room hush: four from the money, the table talks (" + (Due > 0 ? "hand " + std::to_string(Due) + ": " + Said : std::string("never")) + ")");
	if (Due <= 0)
	{
		return;
	}
	std::unique_ptr<room::Director> Second = Fresh();
	for (int H = 1; H < Due; ++H)
	{
		PlayOn(*Second, H);
	}
	T.Tick = Due;
	T.Remaining = Paid + 2; // the hand's busts, at the other tables: two from the money, hand for hand
	const bool Turning = T.HandForHand() && Second->Now() == room::Mood::MoneyClose;
	Second->Hand(T, FoldedHand(T, Due));
	Second->Rounds(T, {});
	ss::TEvent H4H;
	H4H.Type = ss::TEventType::HandForHand;
	Second->Events(T, {H4H}, S.O.Start + 0.75 * Due);
	const std::vector<room::Cue> Heard = Second->Take();
	std::vector<std::string> Chatter;
	for (const room::Cue& C : Heard)
	{
		if (IsChatter(C))
		{
			char Buf[32];
			std::snprintf(Buf, sizeof(Buf), " at %.1f s", C.Delay);
			Chatter.push_back(C.Key + " \"" + C.Text + "\"" + Buf);
		}
	}
	const int FloorParts = static_cast<int>(std::count_if(Heard.begin(), Heard.end(), [](const room::Cue& C) { return Carries(C, "pa.h4h"); }));
	std::printf("room hush: four from the money the table talks at hand %d (%s); that hand taking the room to hand for hand: %d floor parts, %d lines from the table, %s\n", Due,
		Said.c_str(), FloorParts, static_cast<int>(Keyed(Heard, "field.").size()), Chatter.empty() ? "no chatter" : Chatter[0].c_str());
	Expect(Turning && FloorParts > 0 && Chatter.empty() && Second->Now() == room::Mood::HandForHand,
		"room hush: the hand that takes the room to hand for hand gets the floor's call and reactions only, though the table had something to say (hand " +
			std::to_string(Due) + (Chatter.empty() ? std::string(": nothing said") : ": " + Chatter[0]) + ")");
}

/**
 * Hand for hand at a lone final table (round 4, the lead's ruling): MoodWith gives FinalTable before HandForHand, so on
 * every bubble hand there the mood is FinalTable while the engine says hand for hand, and the rule is the same as at any
 * hand for hand ("hand for hand: reactions only", the honesty check's F.HandForHandAtHand): Chatter's T.HandForHand()
 * hushes the small talk and the questions to the dealer. Two directors play the same six-handed final table: three
 * from the money (not hand for hand) the table talks; two from the money, the same hands say no small talk and ask the
 * dealer nothing, and a player at the table who doubles up still gets their reaction.
 */
void RoomHushAtLoneFinalTable()
{
	NearMoney S = MakeNearMoney();
	S.Spec = live::SpecFor(S.O, live::EmbercrestTableSize); // one table: the final table from the first hand
	S.Spec.BreakEvery = 0;
	S.Known.resize(live::EmbercrestTableSize - 1); // the player and the room's chattiest
	S.E.Roster.resize(S.Known.size());
	S.E.Voices.resize(S.Known.size());
	ss::Tournament T(S.Spec, "Jesse", "lonefinal", S.Known);
	const int Paid = T.PaidPlaces();
	auto Fresh = [&S, &T]() {
		auto D = std::make_unique<room::Director>(S.E, S.O, "lonefinal:room", "Jesse");
		D->TalkScale = 2.0f;
		T.Tick = 0;
		D->Clock(T, S.O.Start, 0.0);
		D->Take();
		return D;
	};
	// One hand at the table, everyone folding, the field Away from the money (the engine's hand-for-hand event with the
	// first hand at it, as the engine sends it).
	auto PlayOn = [&S, &T, Paid](room::Director& D, int H, int Away, bool FirstAtIt) {
		T.Tick = H;
		T.Remaining = Paid + Away;
		D.Hand(T, FoldedHand(T, H));
		D.Rounds(T, {});
		std::vector<ss::TEvent> Ev;
		if (FirstAtIt)
		{
			ss::TEvent H4H;
			H4H.Type = ss::TEventType::HandForHand;
			Ev.push_back(H4H);
		}
		D.Events(T, Ev, S.O.Start + 0.75 * H);
		return D.Take();
	};
	T.Remaining = Paid + 3;
	const bool Lone = T.Tables.size() == 1 && T.Remaining > 2 && !T.HandForHand();
	T.Remaining = Paid + 2;
	const bool AtIt = T.HandForHand() && T.Remaining > 2;
	Expect(Lone && AtIt, "room hush (final table): one table, " + std::to_string(Paid) + " paid: hand for hand at two from the money, not at three (" +
							 std::to_string(T.Tables.size()) + " tables)");
	if (!Lone || !AtIt)
	{
		return;
	}
	std::unique_ptr<room::Director> First = Fresh();
	int Due = -1;
	std::string Said;
	for (int H = 1; H <= 120 && Due < 0; ++H)
	{
		for (const room::Cue& C : PlayOn(*First, H, 3, false))
		{
			if (IsChatter(C) && Due < 0)
			{
				Due = H;
				Said = C.Key + " \"" + C.Text + "\"";
			}
		}
	}
	Expect(Due > 0 && First->Now() == room::Mood::FinalTable,
		"room hush (final table): three from the money, the final table talks (" + (Due > 0 ? "hand " + std::to_string(Due) + ": " + Said : std::string("never")) + ")");
	if (Due <= 0)
	{
		return;
	}
	std::unique_ptr<room::Director> Second = Fresh();
	std::vector<std::string> Chatter;
	bool Final = true;
	std::string Mood = "FinalTable"; // the mood the lone table had at hand for hand (the first other one)
	const int Last = Due + 30;
	for (int H = 1; H <= Last; ++H)
	{
		for (const room::Cue& C : PlayOn(*Second, H, 2, H == 1))
		{
			if (IsChatter(C))
			{
				Chatter.push_back("hand " + std::to_string(H) + ": " + C.Key + " \"" + C.Text + "\"");
			}
		}
		Final = Final && Second->Now() == room::Mood::FinalTable && T.HandForHand() && T.Tables.size() == 1;
		Mood = Final ? Mood : Mood != "FinalTable" ? Mood : room::MoodName(Second->Now());
	}
	// Reactions still: a player at the table doubles up (against their stack the hand before), and the table says so.
	std::vector<size_t> Seated;
	for (size_t I = 0; I < T.Players.size(); ++I)
	{
		if (!T.Players[I].IsHero && !T.Players[I].Busted && T.Players[I].TableId == T.Hero().TableId)
		{
			Seated.push_back(I);
		}
	}
	int Reactions = 0;
	std::string Reacted;
	for (size_t K = 0; K < Seated.size() && Reactions == 0; ++K)
	{
		const int H = Last + 1 + static_cast<int>(K);
		ss::TPlayer& P = T.Players[Seated[K]];
		T.Tick = H;
		T.Remaining = Paid + 2;
		P.Stack *= 2;
		room::HandNote N = FoldedHand(T, H);
		N.Folded.erase(std::remove(N.Folded.begin(), N.Folded.end(), P.Name), N.Folded.end());
		N.InHand.push_back(P.Name);
		N.Winners = {P.Name};
		Second->Hand(T, N);
		Second->Rounds(T, {});
		Second->Events(T, {}, S.O.Start + 0.75 * H);
		for (const room::Cue& C : Second->Take())
		{
			if (IsChatter(C))
			{
				Chatter.push_back("hand " + std::to_string(H) + ": " + C.Key + " \"" + C.Text + "\"");
			}
			else if (C.What == room::Kind::Line && C.Where == room::From::HeroTable && Starts(C.Key, "field.") && C.Who == P.Id)
			{
				++Reactions;
				Reacted = P.Name + ": " + C.Key + " \"" + C.Text + "\"";
			}
		}
		Final = Final && Second->Now() == room::Mood::FinalTable && T.HandForHand();
		Mood = Final ? Mood : Mood != "FinalTable" ? Mood : room::MoodName(Second->Now());
	}
	std::printf("room hush (final table): %d paid, the table talks three from the money at hand %d (%s); two from the money, hand for hand at the lone table "
				"(mood %s): %d hands, %s; a double-up's reaction: %s\n",
		Paid, Due, Said.c_str(), Mood.c_str(), Last, Chatter.empty() ? "no chatter" : Chatter[0].c_str(), Reacted.empty() ? "none" : Reacted.c_str());
	Expect(Final && Chatter.empty() && Reactions >= 1,
		"room hush (final table): hand for hand at a lone final table (the mood FinalTable) is reactions only: no small talk and no question to the dealer, a "
		"double-up still answered (" +
			(Chatter.empty() ? std::string("nothing said") : Chatter[0]) + "; " + std::to_string(Reactions) + " reactions; the mood " + Mood + ")");
}

/**
 * The floor near the money, as a floor says it (rounds 3 and 4): how far, then hand for hand, then the bubble. Hand for
 * hand starts two from the money (three with seven tables and more); "on the bubble" is one from it. Walked down, each
 * is its own call: three away, hand for hand at two, on the bubble at one, then the burst. When the field jumps (round 3:
 * on 323 of 1,847 nights it dropped to two from the money in one hand and the floor said "on the bubble, hand for hand"
 * before "two away"), what's due comes in one announcement with one chime and one "Players,": how far, on the bubble at
 * one away, then hand for hand. A jump straight to one away is said once, as a floor would (round 5): the away call's
 * on-the-bubble form ("Players, we are on the bubble. N places are paid, and a min-cash is $X.") and the rule, never "one
 * away from the money" and then "on the bubble". Every path through one away names the bubble before "And that's the
 * bubble!"; none says it at two. On each, a resume with nothing kept after the hand that named the bubble (a checkpoint
 * from before the room kept its own) works out from the night that it was said, and another hand one away doesn't say it
 * again (Resume's inference). Then three of round 5's: the routine PA waits after the joined announcement as after Pa's;
 * the applause answers "And that's the bubble!" after it when the floor has spoken first that hand; and at a lone table
 * hand for hand is the rule without the dealers' decks (the three tables here keep it). Then two of round 6's: one place
 * paid (a lone table) is "1 place is paid" in both forms of the away call; and a lone table at hand for hand never hears
 * "All tables complete. Dealers, next hand." (the field's several tables do, after an all-in and on every third hand).
 */
void RoomAwayThenHandForHand()
{
	struct Case
	{
		const char* Name;
		std::vector<int> Path;                       // from the money at each hand's end (the engine's events with it)
		std::vector<std::vector<std::string>> Calls; // the floor's announcements each hand: each one's calls, "+" between
	};
	const std::vector<Case> Cases = {
		{"walked down", {4, 3, 2, 1, 1, 0}, {{}, {"pa.away"}, {"pa.h4h"}, {"pa.onbubble"}, {}, {"pa.bubble"}}},
		{"four away to two in one hand", {4, 2, 1, 0}, {{}, {"pa.away+pa.h4h"}, {"pa.onbubble"}, {"pa.bubble"}}},
		{"four away to one in one hand", {4, 1, 0}, {{}, {"pa.away.bubble+pa.h4h"}, {"pa.bubble"}}},
		{"three away, then one in one hand", {4, 3, 1, 0}, {{}, {"pa.away"}, {"pa.onbubble+pa.h4h"}, {"pa.bubble"}}},
		{"two away into the money in one hand", {4, 3, 2, 0}, {{}, {"pa.away"}, {"pa.h4h"}, {"pa.bubble"}}},
	};
	const std::set<std::string> Near = {"pa.away", "pa.away.bubble", "pa.onbubble", "pa.h4h", "pa.h4h.lone", "pa.bubble"};
	static const std::string Hold = "Dealers, please hold your decks until every table has finished the hand.";
	for (const Case& K : Cases)
	{
		const NearMoney S = MakeNearMoney();
		ss::Tournament T(S.Spec, "Jesse", "away", S.Known);
		const int Paid = T.PaidPlaces();
		std::string Bubble;
		for (const ss::TPlayer& P : T.Players)
		{
			Bubble = Bubble.empty() && !P.IsHero && P.TableId != T.Hero().TableId ? P.Name : Bubble;
		}
		room::Director D(S.E, S.O, "away:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		bool Fired = false;
		bool Right = true;
		bool Resumed = false;
		std::string Heard;
		std::string Problem;
		for (size_t I = 0; I < K.Path.size(); ++I)
		{
			const int Away = K.Path[I];
			T.Tick = 30 + static_cast<int>(I);
			T.Remaining = Paid + Away;
			std::vector<ss::TEvent> Ev;
			if (!Fired && T.HandForHand())
			{
				Fired = true;
				ss::TEvent H4H;
				H4H.Type = ss::TEventType::HandForHand;
				Ev.push_back(H4H);
			}
			if (Away <= 0)
			{
				ss::TEvent B;
				B.Type = ss::TEventType::Bubble;
				B.Name = Bubble;
				Ev.push_back(B);
			}
			D.Events(T, Ev, S.O.Start + 100.0 + static_cast<double>(I));
			const std::vector<room::Cue> Cues = D.Take();
			// The floor's announcements near the money, each whole: its calls, its text, its chimes.
			std::vector<std::string> Calls;
			double FloorH4H = -1.0;
			double DealerH4H = -1.0;
			for (size_t J = 0; J < Cues.size(); ++J)
			{
				const room::Cue& C = Cues[J];
				DealerH4H = Starts(C.Key, "dealer.h4h.") ? C.Delay : DealerH4H;
				if (C.What != room::Kind::Pa || C.Part != 0)
				{
					continue;
				}
				std::string Pools;
				std::vector<std::string> List;
				bool Ours = false;
				for (std::string Pool : CallsOf(C.Key))
				{
					Pool = Pool == "pa.bubble.seat" ? std::string("pa.bubble") : Pool;
					Ours = Ours || Near.count(Pool) > 0;
					Pools += (Pools.empty() ? "" : "+") + Pool;
					List.push_back(Pool);
				}
				if (!Ours)
				{
					continue;
				}
				const auto Says = [&List](const char* Pool) { return std::find(List.begin(), List.end(), Pool) != List.end(); };
				std::string Text = C.Text;
				int Chimes = C.Chime ? 1 : 0;
				int Want = 1;
				for (size_t L = J + 1; L < Cues.size(); ++L)
				{
					if (Cues[L].What == room::Kind::Pa && Cues[L].Key == C.Key && Cues[L].Part == Want)
					{
						Text += " " + Cues[L].Text;
						Chimes += Cues[L].Chime ? 1 : 0;
						FloorH4H = Carries(C, "pa.h4h") ? std::max(FloorH4H, Cues[L].Delay) : FloorH4H;
						++Want;
					}
				}
				FloorH4H = Carries(C, "pa.h4h") ? std::max(FloorH4H, C.Delay) : FloorH4H;
				Calls.push_back(Pools);
				Heard += std::string(Heard.empty() ? "" : " / ") + "[" + std::to_string(Away) + " away] " + Text;
				const auto Fail = [&Problem, &Text](const std::string& Why) {
					Problem = Problem.empty() ? Why + ": " + Text : Problem;
				};
				size_t Players = 0;
				for (size_t At = Text.find("Players,"); At != std::string::npos; At = Text.find("Players,", At + 1))
				{
					++Players;
				}
				const bool Opens = Pools != "pa.bubble";
				if (Chimes != 1 || !C.Chime || (Opens && (Players != 1 || !Starts(Text, "Players, "))))
				{
					Fail("one chime and one \"Players,\" an announcement");
				}
				if (Says("pa.away") && !Contains(Text, "we are " + talk::NumberWords(Away) + " away from the money"))
				{
					Fail("how far, as far as it is");
				}
				if (Says("pa.away.bubble") && !(Starts(Text, "Players, we are on the bubble. ") && Contains(Text, " places are paid, and a min-cash is ")))
				{
					Fail("a jump to one away: on the bubble, then what's paid");
				}
				if (Contains(Text, "one away from the money"))
				{
					Fail("one away is on the bubble, said once");
				}
				if ((Says("pa.onbubble") || Says("pa.away.bubble")) != Contains(Lower(Text), "on the bubble") || Says("pa.h4h") != Contains(Text, "hand for hand") ||
					Says("pa.h4h") != Contains(Text, Hold))
				{
					Fail("each call's words");
				}
				if (Contains(Lower(Text), "bubble") && Away != 1 && Away > 0)
				{
					Fail("the bubble named only one from the money");
				}
			}
			// Resume's inference: after the hand that named the bubble, a director resumed with nothing kept works out from the
			// night that it was said, and another hand one from the money calls nothing near the money again.
			if (Away == 1 && !Resumed)
			{
				Resumed = true;
				room::Director R(S.E, S.O, "away:room", "Jesse");
				const double World = S.O.Start + 100.0 + static_cast<double>(I);
				R.Resume(T, "", World);
				R.Take();
				T.Tick += 1; // the path's next hand sets its own
				R.Events(T, {}, World + 0.5);
				T.Tick -= 1;
				std::string Again;
				for (const room::Cue& C : R.Take())
				{
					for (const std::string& Pool : C.What == room::Kind::Pa ? CallsOf(C.Key) : std::vector<std::string>())
					{
						Again = Again.empty() && Near.count(Pool) > 0 ? C.Key + " \"" + C.Text + "\"" : Again;
					}
				}
				Problem = Problem.empty() && !Again.empty() ? "resumed with nothing kept, another hand one away called " + Again : Problem;
				Heard += " / [resumed, one away] " + (Again.empty() ? std::string("nothing said again") : Again);
			}
			if (Calls != K.Calls[I])
			{
				std::string Got;
				for (const std::string& C : Calls)
				{
					Got += (Got.empty() ? "" : ", ") + C;
				}
				Problem = Problem.empty() ? "hand " + std::to_string(I + 1) + " (" + std::to_string(Away) + " away) called {" + Got + "}" : Problem;
			}
			if (FloorH4H >= 0.0 && DealerH4H <= FloorH4H)
			{
				Problem = Problem.empty() ? std::string("the dealer's hand for hand after the floor's") : Problem;
			}
		}
		Right = Right && Problem.empty();
		std::printf("room away: %s: %s\n", K.Name, Heard.c_str());
		Expect(Right, std::string("room away: ") + K.Name + ": how far, the bubble at one away, hand for hand, in one announcement when they come together (" +
						  (Problem.empty() ? Heard : Problem) + ")");
	}

	// One Take's floor announcements, each whole: its calls, its parts' text, and the Delays of its first and last parts.
	struct Announcement
	{
		std::vector<std::string> Calls;
		std::string Text;
		double First = 0.0;
		double Last = 0.0;
	};
	const auto Announced = [](const std::vector<room::Cue>& Cues) {
		std::vector<Announcement> Out;
		for (size_t J = 0; J < Cues.size(); ++J)
		{
			if (Cues[J].What != room::Kind::Pa || Cues[J].Part != 0)
			{
				continue;
			}
			Announcement A;
			A.Calls = CallsOf(Cues[J].Key);
			A.Text = Cues[J].Text;
			A.First = A.Last = Cues[J].Delay;
			int Want = 1;
			for (size_t L = J + 1; L < Cues.size(); ++L)
			{
				if (Cues[L].What == room::Kind::Pa && Cues[L].Key == Cues[J].Key && Cues[L].Part == Want)
				{
					A.Text += " " + Cues[L].Text;
					A.Last = Cues[L].Delay;
					++Want;
				}
			}
			Out.push_back(A);
		}
		return Out;
	};
	const auto With = [](const std::vector<Announcement>& All, const std::string& Pool) -> const Announcement* {
		for (const Announcement& A : All)
		{
			if (std::find(A.Calls.begin(), A.Calls.end(), Pool) != A.Calls.end())
			{
				return &A;
			}
		}
		return nullptr;
	};
	ss::TEvent H4H;
	H4H.Type = ss::TEventType::HandForHand;

	// The routine PA waits after the joined announcement as after Pa's (PaTogether's PaFree): the next hand's start (the
	// engine's Balance) comes before that hand's tick turns, so at the tick of the call. A table breaking then isn't called
	// over it (the routine PA's thirty seconds); a hand on, the same call is.
	{
		const NearMoney S = MakeNearMoney();
		ss::Tournament T(S.Spec, "Jesse", "away", S.Known);
		const int Paid = T.PaidPlaces();
		room::Director D(S.E, S.O, "away:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		T.Tick = 30;
		T.Remaining = Paid + 4;
		D.Events(T, {}, S.O.Start + 100.0);
		D.Take();
		T.Tick = 31;
		T.Remaining = Paid + 2; // four away to two in one hand: how far and hand for hand, joined
		D.Events(T, {H4H}, S.O.Start + 101.0);
		const std::vector<Announcement> Joined = Announced(D.Take());
		std::vector<int> Others;
		for (const auto& It : T.Tables)
		{
			if (It.first != T.Hero().TableId)
			{
				Others.push_back(It.first);
			}
		}
		ss::TEvent Broken;
		Broken.Type = ss::TEventType::TableBroken;
		Broken.TableId = Others.empty() ? -1 : Others.front();
		D.Events(T, {Broken}, S.O.Start + 101.5); // the next hand's start: the same tick
		const std::vector<Announcement> Then = Announced(D.Take());
		T.Tick = 32;
		Broken.TableId = Others.empty() ? -1 : Others.back();
		D.Events(T, {Broken}, S.O.Start + 102.0); // a hand on
		const std::vector<Announcement> Later = Announced(D.Take());
		const bool Together = Joined.size() == 1 && Joined[0].Calls == std::vector<std::string>{"pa.away", "pa.h4h"};
		const Announcement* Over = With(Then, "pa.table.breaking");
		const Announcement* On = With(Later, "pa.table.breaking");
		std::printf("room away: the routine PA after the joined announcement: %s / the next hand's start: %s / a hand on: %s\n",
			Joined.empty() ? "(nothing)" : Joined[0].Text.c_str(), Over ? Over->Text.c_str() : "(waits)", On ? On->Text.c_str() : "(nothing)");
		Expect(Together && !Over && On, "room away: the routine PA waits after the joined announcement (a table breaking at the next hand's start, the same tick, "
										"isn't called over it; a hand on, it is)");
	}

	// The applause answers "And that's the bubble!", after it, when the floor has spoken first that hand: the burst's hand
	// with a paid bust at another table called before the bubble's call (the engine's order: the hand's busts, then the
	// bubble). A fixed 1.4 s put it under the bust's call, before the bubble was said.
	{
		const NearMoney S = MakeNearMoney();
		ss::Tournament T(S.Spec, "Jesse", "away", S.Known);
		const int Paid = T.PaidPlaces();
		std::vector<const ss::TPlayer*> Elsewhere;
		for (const ss::TPlayer& P : T.Players)
		{
			if (!P.IsHero && P.TableId != T.Hero().TableId)
			{
				Elsewhere.push_back(&P);
			}
		}
		room::Director D(S.E, S.O, "away:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		T.Tick = 30;
		T.Remaining = Paid + 3;
		D.Events(T, {}, S.O.Start + 100.0);
		D.Take();
		T.Tick = 31;
		T.Remaining = Paid + 1;
		D.Events(T, {H4H}, S.O.Start + 101.0);
		D.Take();
		T.Tick = 32;
		T.Remaining = Paid - 1; // two out in one hand: the bubble, and one in the money
		std::vector<ss::TEvent> Ev;
		for (int K = 0; K < 2 && Elsewhere.size() >= 2; ++K)
		{
			const ss::TPlayer& P = *Elsewhere[static_cast<size_t>(K)];
			ss::TEvent B;
			B.Type = ss::TEventType::Bust;
			B.Id = P.Id;
			B.Name = P.Name;
			B.TableId = P.TableId;
			B.Place = K == 0 ? Paid + 1 : Paid;
			B.PrizeCents = T.PrizeFor(B.Place);
			Ev.push_back(B);
		}
		ss::TEvent Burst;
		Burst.Type = ss::TEventType::Bubble;
		Burst.Name = Elsewhere.empty() ? std::string() : Elsewhere[0]->Name;
		Ev.push_back(Burst);
		D.Events(T, Ev, S.O.Start + 102.0);
		const std::vector<room::Cue> Cues = D.Take();
		const std::vector<Announcement> Heard = Announced(Cues);
		const Announcement* Paidout = With(Heard, "pa.bust.paid");
		const Announcement* Bubble = With(Heard, "pa.bubble") ? With(Heard, "pa.bubble") : With(Heard, "pa.bubble.seat");
		double Applause = -1.0;
		for (const room::Cue& C : Cues)
		{
			Applause = C.What == room::Kind::Vocal && C.Where == room::From::Pa && C.Noise == ss::audio::Vocal::Applause ? C.Delay : Applause;
		}
		std::printf("room away: the burst after a paid bust's call: %s (%.1f s) / %s (%.1f to %.1f s) / applause at %.1f s\n", Paidout ? Paidout->Text.c_str() : "(no bust call)",
			Paidout ? Paidout->First : -1.0, Bubble ? Bubble->Text.c_str() : "(no bubble)", Bubble ? Bubble->First : -1.0, Bubble ? Bubble->Last : -1.0, Applause);
		Expect(Paidout && Bubble && Paidout->First < Bubble->First && Applause > Bubble->Last,
			"room away: the applause follows \"And that's the bubble!\" when the floor called a paid bust first that hand (applause at " + std::to_string(Applause) +
				" s, the bubble's call from " + std::to_string(Bubble ? Bubble->First : -1.0) + " s)");
	}

	// Hand for hand at a lone table: the rule without "Dealers, please hold your decks until every table has finished the
	// hand." (there's no other table); the three tables above keep it ("each call's words"). Walked down from three away,
	// so the hand-for-hand hand's call is the rule alone.
	{
		NearMoney S = MakeNearMoney();
		S.Spec = live::SpecFor(S.O, live::EmbercrestTableSize); // one table
		S.Spec.BreakEvery = 0;
		S.Known.resize(live::EmbercrestTableSize - 1);
		S.E.Roster.resize(S.Known.size());
		S.E.Voices.resize(S.Known.size());
		ss::Tournament T(S.Spec, "Jesse", "lonefinal", S.Known);
		const int Paid = T.PaidPlaces();
		room::Director D(S.E, S.O, "lonefinal:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		T.Tick = 30;
		T.Remaining = Paid + 3;
		D.Events(T, {}, S.O.Start + 100.0);
		D.Take();
		T.Tick = 31;
		T.Remaining = Paid + 2;
		const bool Lone = T.Tables.size() == 1 && T.HandForHand() && T.Remaining > 2;
		D.Events(T, {H4H}, S.O.Start + 101.0);
		const std::vector<Announcement> Heard = Announced(D.Take());
		const Announcement* Rule = With(Heard, "pa.h4h.lone");
		std::printf("room away: hand for hand at a lone table (%d paid): %s\n", Paid, Rule ? Rule->Text.c_str() : "(no lone table's call)");
		Expect(Lone && Rule && Contains(Rule->Text, "hand for hand") && !Contains(Rule->Text, "Dealers") && !With(Heard, "pa.h4h"),
			"room away: hand for hand at a lone table is the rule without the dealers' decks (" + std::string(Rule ? Rule->Text : "no lone table's call") + ")");
	}

	// A lone six-handed table (Embercrest's table size) pays one place.
	const auto LoneTable = [] {
		NearMoney S = MakeNearMoney();
		S.Spec = live::SpecFor(S.O, live::EmbercrestTableSize);
		S.Spec.BreakEvery = 0;
		S.Known.resize(live::EmbercrestTableSize - 1);
		S.E.Roster.resize(S.Known.size());
		S.E.Voices.resize(S.Known.size());
		return S;
	};

	// One place paid is "1 place is paid", not "1 places are paid", in both forms of the away call: walked down to three
	// away (pa.away), and jumped from four away to one (pa.away.bubble, with the rule); several tables' four places are
	// still "4 places are paid".
	for (const bool OneTable : {true, false})
	{
		for (const int To : {3, 1})
		{
			const NearMoney S = OneTable ? LoneTable() : MakeNearMoney();
			ss::Tournament T(S.Spec, "Jesse", OneTable ? "lonefinal" : "away", S.Known);
			const int Paid = T.PaidPlaces();
			room::Director D(S.E, S.O, OneTable ? "lonefinal:room" : "away:room", "Jesse");
			D.Clock(T, S.O.Start, 0.0);
			D.Take();
			T.Tick = 30;
			T.Remaining = Paid + 4;
			D.Events(T, {}, S.O.Start + 100.0);
			D.Take();
			T.Tick = 31;
			T.Remaining = Paid + To;
			std::vector<ss::TEvent> Ev;
			if (T.HandForHand())
			{
				Ev.push_back(H4H);
			}
			D.Events(T, Ev, S.O.Start + 101.0);
			const std::vector<Announcement> Heard = Announced(D.Take());
			const std::string Pool = To == 1 ? "pa.away.bubble" : "pa.away";
			const Announcement* Call = With(Heard, Pool);
			const std::string Want = std::to_string(Paid) + (Paid == 1 ? " place is paid, and a min-cash is " : " places are paid, and a min-cash is ");
			const bool Shape = OneTable ? T.Tables.size() == 1 && Paid == 1 : T.Tables.size() > 1 && Paid > 1;
			std::printf("room away: %s, %d paid, %s: %s\n", OneTable ? "a lone table" : "several tables", Paid, Pool.c_str(), Call ? Call->Text.c_str() : "(no call)");
			Expect(Shape && Call && Contains(Call->Text, Want) && !(Paid == 1 && Contains(Call->Text, "places are paid")),
				"room away: " + Pool + " says \"" + Want + "\" (" + std::to_string(Paid) + " paid, " + std::to_string(T.Tables.size()) + " tables: " +
					(Call ? Call->Text : std::string("no call")) + ")");
		}
	}

	// "All tables complete. Dealers, next hand." (pa.h4h.done) only where there are other tables to wait for: at hand for
	// hand several tables hear it after the hand with an all-in (another table's) and on the third hand; a lone table's
	// floor never says it, after its own all-in or over three hands and more.
	for (const bool OneTable : {true, false})
	{
		const NearMoney S = OneTable ? LoneTable() : MakeNearMoney();
		ss::Tournament T(S.Spec, "Jesse", OneTable ? "lonefinal" : "away", S.Known);
		const int Paid = T.PaidPlaces();
		const int Mine = T.Hero().TableId;
		int AllInAt = Mine;
		for (const auto& It : T.Tables)
		{
			AllInAt = !OneTable && It.first != Mine ? It.first : AllInAt;
		}
		room::Director D(S.E, S.O, OneTable ? "lonefinal:room" : "away:room", "Jesse");
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		T.Tick = 30;
		T.Remaining = Paid + 3;
		D.Events(T, {}, S.O.Start + 100.0);
		D.Take();
		T.Tick = 31;
		T.Remaining = Paid + 2;
		D.Events(T, {H4H}, S.O.Start + 101.0);
		const bool AtIt = T.HandForHand() && (T.Tables.size() == 1) == OneTable && !Announced(D.Take()).empty();
		std::vector<int> Done; // the hands at it that said it
		std::string Said;
		const int Hands = 4; // the first with the all-in, then three without
		for (int H = 1; H <= Hands; ++H)
		{
			T.Tick = 31 + H;
			std::vector<ss::TableRound> R;
			for (const auto& It : T.Tables)
			{
				ss::TableRound One;
				One.TableId = It.first;
				One.AllIn = H == 1 && It.first == AllInAt;
				R.push_back(One);
			}
			D.Rounds(T, R);
			D.Events(T, {}, S.O.Start + 101.0 + static_cast<double>(H));
			for (const room::Cue& C : D.Take())
			{
				if (Carries(C, "pa.h4h.done"))
				{
					Done.push_back(H);
					Said = C.Text;
				}
			}
		}
		std::string Which;
		for (const int H : Done)
		{
			Which += (Which.empty() ? "" : ", ") + std::to_string(H);
		}
		const std::string Quoted = Said.empty() ? std::string() : " \"" + Said + "\"";
		std::printf("room away: hand for hand, %s, %d hands (an all-in on the first): \"All tables complete\" after hands {%s}%s\n",
			OneTable ? "a lone table" : "several tables", Hands, Which.c_str(), Quoted.c_str());
		Expect(AtIt && (OneTable ? Done.empty() : Done == std::vector<int>{1, 3}),
			std::string("room away: hand for hand ") + (OneTable ? "at a lone table never calls \"All tables complete\"" : "at several tables calls \"All tables complete\" after the all-in and the third hand") +
				" (hands {" + Which + "})");
	}
}

/**
 * Nobody at the rail says two things at once (round 3: salt ":1777" hand 152, Mrs. Park's final-table line at 15.5 s and
 * her read of the player's stack at 16.5 s by the log). In one hand's end, one Take: the player doubles up (her double-up
 * line, stake.rail) and the level goes up (her read of the stack, field.rail.read): both said, the read starting a second
 * and more after the line before it, here with the level's PA and nothing from the dealer between them (on the levels
 * where the dealer says "Blinds are up." the read waits for the floor too). The ":1777" pair was two Takes: the final
 * table's call starts a hand, the level comes at its end, a hand apart in the room though the log gives both the hand's
 * number; RailVoicesOf (RoomShape's check) compares Delays within a Take, where they count from the same moment, and
 * still catches one voice twice inside a second.
 */
void RoomRailOneVoice()
{
	const SmallTable S = MakeSmallTable(0, 0.3f);
	// One hand's end at the given level: the cues of its Take.
	auto Heard = [&S](int Level) {
		ss::Tournament T(S.Spec, "Jesse", "onevoice", S.Known);
		const size_t Hero = static_cast<size_t>(T.PlayerIndex(ss::HeroId));
		room::Director D(S.E, S.O, "onevoice:room", "Jesse");
		D.SetRail({{"Mrs. Park", 150}});
		D.Clock(T, S.O.Start, 0.0);
		D.Take();
		auto Won = [&T](int H) {
			room::HandNote N;
			N.Hand = H;
			for (const ss::TPlayer& P : T.Players)
			{
				(P.IsHero ? N.InHand : N.Folded).push_back(P.IsHero ? std::string(ss::HeroId) : P.Name);
			}
			N.Winners = {ss::HeroId};
			return N;
		};
		T.Tick = 1;
		D.Hand(T, Won(1)); // the stack she measures the double-up from
		D.Take();
		T.Tick = 2;
		T.Players[Hero].Stack *= 2;
		D.Hand(T, Won(2));
		D.Rounds(T, {});
		ss::TEvent Up;
		Up.Type = ss::TEventType::Level;
		Up.LevelNumber = Level;
		Up.Blinds = T.Levels[std::min<size_t>(static_cast<size_t>(Level - 1), T.Levels.size() - 1)];
		D.Events(T, {Up}, S.O.Start + 20.0);
		return D.Take();
	};
	int Tight = 0;
	for (int Level = 3; Level <= 10; ++Level)
	{
		const std::vector<room::Cue> Said = Heard(Level);
		const room::Cue* Line = nullptr;
		const room::Cue* Read = nullptr;
		Night One;
		for (const room::Cue& C : Said)
		{
			One.Cues.push_back({2, C});
			One.TakeOf.push_back(0);
			if (C.What == room::Kind::Line && C.Where == room::From::Rail && C.Who == "Mrs. Park")
			{
				Line = C.Key == "stake.rail" && !Line ? &C : Line;
				Read = Starts(C.Key, "field.rail.read") && !Read ? &C : Read;
			}
		}
		const bool Dealer = !Keyed(Said, "dealer.blinds.").empty();
		Tight += Line && Read && !Dealer ? 1 : 0;
		char Buf[200];
		std::snprintf(Buf, sizeof(Buf), " (level %d%s: her double-up line at %.2f s, her read at %.2f s)", Level, Dealer ? ", the dealer between" : "",
			Line ? Line->Delay : -1.0, Read ? Read->Delay : -1.0);
		if (Level == 3 || (!Dealer && Tight == 1))
		{
			std::printf("room rail one voice: one hand's end%s\n", Buf);
		}
		Expect(Line && Read && Read->Delay - Line->Delay >= 1.0,
			std::string("room rail one voice: a backer's double-up line and her read of the stack in one hand's end, a second and more apart") + Buf);
		const RailVoices V = RailVoicesOf(One);
		Expect(V.Together == 1 && V.OneVoice, "room rail one voice: RoomShape's check compares the two and passes them" + V.Problem);
		// The ":1777" shape: the same two lines, a hand's start (one Take) and its end (another), under a second apart by
		// their Delays. Two Takes: nothing to compare. One Take: one voice twice at once, caught.
		if (Level == 3 && Line && Read)
		{
			for (const bool Split : {true, false})
			{
				Night Two;
				room::Cue A = *Line;
				room::Cue B = *Read;
				A.Delay = 15.5;
				B.Delay = 16.4;
				Two.Cues = {{152, A}, {152, B}};
				Two.TakeOf = {0, Split ? 1 : 0};
				const RailVoices W = RailVoicesOf(Two);
				Expect(Split ? W.Together == 0 && W.OneVoice : W.Together == 1 && !W.OneVoice,
					std::string("room rail one voice: ") + (Split ? "a hand's start and its end are two Takes, not compared" : "one Take, one voice twice inside a second: caught"));
			}
		}
	}
	Expect(Tight >= 1, "room rail one voice: a level with nothing from the dealer between her two lines (" + std::to_string(Tight) + " of 8)");
}

// ------------------------------------------------------------------ RoomTalkCopy

void RoomTalkCopy()
{
	const std::map<std::string, std::string> Extra = {
		{"Average", "41,000"}, {"Players", "114"}, {"Pool", "$15,390"}, {"MinCash", "$270"}, {"Sb", "12,500"}, {"Bb", "25,000"}, {"Ante", "25,000"}, {"Minutes", "15"},
		{"Stack", "30,000"}, {"Denoms", "purple five-hundreds and the yellow thousands"}, {"Away", "three"}, {"PlacesAre", "places are"}, {"From", "14"}, {"SeatWord", "four"},
		{"Honor", "two-time Embercrest champion"}, {"A", "Jesse Cole"}, {"AChips", "1,036,000"}, {"B", "Maria G."}, {"BChips", "1,012,000"}, {"First", "$4,160"},
		{"Mine", "Ace-king suited"}, {"MineLower", "ace-king suited"}, {"Theirs", "Queen-jack suited"}, {"TheirsLower", "queen-jack suited"}, {"Street", "river"},
		{"Bubble", "Maria G."}, {"PlaceWord", "Seventeenth"}, {"PlaceLower", "seventeenth"}, {"EventSpoken", "Midnight Turbo"}, {"When", "Wednesday"},
		{"WhenCap", "Yesterday"}, {"Entries", "114"}, {"Prize", "$1,280"}, {"LevelsCap", "Two"}, {"Spoken", "Eighteen thousand four hundred."},
		{"Rough", "Two hundred-something"}, {"RoughLower", "two hundred-something"}, {"Event", "Saturday Big Stack"}, {"Short", "Midnight Turbo"},
		{"Name", "Danny Q."}, {"TableWord", "eighteen"},
		// A fresh start's first month (update 2 §3.3 F9), at their longest: the Sunday's name, a weekday night, a long first name.
		{"FreshEvent", "Embercrest Sunday"}, {"FreshWhen", "Wednesday night"}, {"FreshThen", "Wednesday night"}, {"FreshThenCap", "Wednesday night"},
		{"FreshWho", "Alessandro"}};
	std::vector<std::pair<std::string, std::string>> Talk;
	std::vector<std::pair<std::string, std::string>> Pa;
	for (const auto& Kv : talk::Inventory())
	{
		(Starts(Kv.first, "pa.") ? Pa : Talk).push_back(Kv);
	}
	const int Problems = copylint::Run("talk", Talk, copylint::TableTalk, Extra) + copylint::Run("floor", Pa, copylint::Pa, Extra);
	Expect(Problems == 0, "room copy: every line of the talk, the dealers and the floor passes the copy lint");
	Expect(Talk.size() > 250 && Pa.size() >= 40, "room copy: the inventories are there (" + std::to_string(Talk.size()) + " talk, " + std::to_string(Pa.size()) + " PA)");
	// At least six variants for the small talk's own pools, the needles and the bad beats (§4.5).
	std::map<std::string, int> Pools;
	for (const auto& Kv : talk::Inventory())
	{
		Pools[Kv.first.substr(0, Kv.first.rfind('.'))]++;
	}
	for (const char* Pool : {"field.small.trip.open", "field.small.food.open", "field.small.room.open", "field.small.night.open", "field.small.week", "field.small.heater",
			 "field.small.down", "field.small.online", "field.small.makeup", "field.needle", "field.needle.rival", "field.needle.bluff", "field.badbeat", "field.goodbye",
			 "field.suckout", "field.nicehand", "field.doubleup", "field.bubble"})
	{
		Expect(Pools[Pool] >= 6, std::string("room copy: at least six variants in ") + Pool);
	}
	// A fresh start's first month: every opener has a second way to say it, every answer pool four.
	for (const char* Pool : {"field.fresh.tonight", "field.fresh.next", "field.fresh.next.nightly", "field.fresh.wall", "field.fresh.wallfirst", "field.fresh.wallfirst.self",
			 "field.fresh.wallfirst.you", "field.fresh.first", "field.fresh.first.self", "field.fresh.first.you", "field.fresh.twice", "field.fresh.twice.self",
			 "field.fresh.twice.you", "field.fresh.books"})
	{
		Expect(Pools[Pool] >= 2, std::string("room copy: at least two variants in ") + Pool);
	}
	for (const char* Pool : {"field.fresh.reply", "field.fresh.first.reply", "field.fresh.self.reply"})
	{
		Expect(Pools[Pool] >= 4, std::string("room copy: at least four answers in ") + Pool);
	}
	// The Back Room's eight, word for word (BackRoomPlayerTells.cpp's FLines and BackRoomLive.cpp's Goodbye at c09bc31).
	struct Frozen
	{
		const char* Name;
		std::vector<std::string> Chat, Win, Lose, Noticed;
		std::string Goodbye;
	};
	const std::vector<Frozen> Eight = {
		{"Sal", {"Forty years I been playing this game.", "In my day we played for matchsticks.", "Dee, this coffee's older than me.", "Patience. That's the whole game, kid."},
			{"Thank you kindly.", "Slow and steady."}, {"Eh. Cards are cards.", "Shoulda stayed home tonight."}, {"Something on my face, kid?", "Take a picture, it lasts longer."},
			"That's me. Good luck, kid."},
		{"Big Lou", {"Who's ordering wings?", "Lou don't fold. Lou never folds!", "That dryer's louder than my ex.", "Deal 'em, Dee, I'm feeling it tonight!"},
			{"That's what I'm talking about!", "Ship it! Come to papa."}, {"Unbelievable. Un-be-lievable.", "Every time. Every single time!"},
			{"You lookin' at me? You lookin' at my chips?", "Eyes on your own cards, kid."}, "Lou's out! Lou is OUT! Unbelievable."},
		{"Twitch", {"Can we go faster? Come on.", "Bet you I hit this one.", "Anybody got a lighter?", "Come on come on come on."}, {"Yeah! Yeah! Pay me!", "Too easy, man."},
			{"This deck is rigged.", "Are you kidding me right now?"}, {"What? What're you looking at?", "Quit staring, man. Seriously."}, "Whatever, man. This deck hates me."},
		{"Mei", {"Mm.", "Your deal, Dee.", "Interesting."}, {"Thank you.", "Nice try."}, {"Good call.", "Well played."}, {"See something you like?", "Careful. I look back."},
			"Good game, everyone."},
		{"Mrs. Park", {"My grandson plays on the computer. He is not good.", "Fold, fold, fold. Then I win.", "Thirty years I play this tournament.", "Dealer, slower please. My eyes."},
			{"Thank you, dear.", "Patience."}, {"Hm.", "Next time."}, {"You want to marry me? Stop looking.", "Eyes on your cards, young man."},
			"Thirty years. Still nothing. Goodnight, everybody."},
		{"Rick", {"I sell more cars by noon than this whole table wins tonight.", "Raise it up, let's play some poker!", "Who's buying drinks? I'm buying drinks.",
					 "Dee, you're my good luck charm. Don't go anywhere."},
			{"That's how it's done!", "Stack 'em up, sweetheart."}, {"Rigged. Un-be-lievable.", "Whatever. I'm rebuying next week."}, {"Like what you see, champ?", "Take a picture."},
			"Unbelievable! I'm going to the bar."},
		{"Dre", {"Y'all see the game today? Brutal.", "I'm just here for the vibes, man.", "Dee! My favorite dealer in the building!", "If I bust, somebody tell my wife I won."},
			{"Let's gooo!", "Chip up, chip up."}, {"All good, all good.", "That's poker, baby."}, {"You reading me? Ain't nothing to read.", "My face is an open book, man. A boring one."},
			"That's poker, baby. Good luck, y'all."},
		{"gh0stfold", {"Same rake, worse coffee.", "Live is so slow.", "You play online?"}, {"gg.", "ty."}, {"nh.", "variance."},
			{"Staring won't help. I've seen your stats.", "Look at the board, not me."}, "gg. see you online."},
	};
	for (const Frozen& F : Eight)
	{
		const std::string Who = F.Name;
		Expect(talk::IsCast(Who), "room cast: " + Who + " is one of the eight");
		Expect(talk::CastLines(Who, talk::Moment::Small) == F.Chat, "room cast: " + Who + "'s chat, word for word");
		Expect(talk::CastLines(Who, talk::Moment::Win) == F.Win, "room cast: " + Who + "'s wins, word for word");
		Expect(talk::CastLines(Who, talk::Moment::Lose) == F.Lose, "room cast: " + Who + "'s losses, word for word");
		Expect(talk::CastLines(Who, talk::Moment::Noticed) == F.Noticed, "room cast: " + Who + "'s noticing you, word for word");
		Expect(talk::CastLines(Who, talk::Moment::Goodbye) == std::vector<std::string>{F.Goodbye}, "room cast: " + Who + "'s goodbye, word for word");
		for (uint32_t Salt = 0; Salt < 8; ++Salt)
		{
			const talk::Said S = talk::Cast(Who, talk::Moment::Small, Salt);
			Expect(S.Text == F.Chat[Salt % F.Chat.size()], "room cast: Cast picks " + Who + "'s line by its salt");
		}
	}
	Expect(talk::CastLines("Boots", talk::Moment::Small).empty() && !talk::IsCast("Boots"), "room cast: nobody else has the eight's lines");
	// At the Embercrest, the eight don't say what belongs to Dee's game: the dryer downstairs, and Dee unless she deals.
	life::LiveVoice Lou = MakeVoice("Big Lou", ss::Archetype::Station, 0.8f, 0.45f, world::Identity::Recreational, 54);
	int Dryer = 0;
	int Dee = 0;
	for (uint32_t Salt = 0; Salt < 64; ++Salt)
	{
		talk::Context C;
		C.Self = &Lou;
		C.Salt = Salt;
		C.Vars["Venue"] = "embercrest";
		const talk::Said S = talk::Line(talk::Moment::Small, C);
		Dryer += Contains(S.Text, "dryer") ? 1 : 0;
		Dee += Contains(S.Text, "Dee") ? 1 : 0;
	}
	Expect(Dryer == 0 && Dee == 0, "room cast: at the Embercrest, Big Lou's lines about the dryer and Dee stay at Dee's game");

	// Spoken hands and caption names.
	Expect(talk::SpokenHand({ss::MakeCard(12, 0), ss::MakeCard(12, 3)}) == "aces", "room words: aces");
	Expect(talk::SpokenHand({ss::MakeCard(2, 1), ss::MakeCard(5, 2)}) == "seven-four", "room words: seven-four");
	Expect(talk::SpokenHand({ss::MakeCard(11, 2), ss::MakeCard(12, 2)}) == "ace-king suited", "room words: ace-king suited");
	Expect(talk::CaptionName("npc:Boots", "Boots", 2) == "Boots" && talk::CaptionName("npc:Maria G.", "Maria G.", 2) == "Maria G.", "room names: the room's people by name");
	Expect(talk::CaptionName("p3", "mike_1987", 2) == "Mike", "room names: mike_1987 is Mike");
	Expect(talk::CaptionName("p4", "DmitriVolkov", 0) == "Dmitri", "room names: DmitriVolkov is Dmitri");
	Expect(talk::CaptionName("p5", "olgapetrov", 0) == "Olga", "room names: olgapetrov is Olga");
	Expect(talk::CaptionName("p6", "VelvetRiver", 2) == "Seat 3", "room names: a handle with no name in it is a seat");
	Expect(talk::CaptionName("p7", "benitez42", 4) == "Seat 5" && talk::CaptionName("p8", "ivanov7", 1) == "Seat 2", "room names: a surname isn't a first name");
	Expect(talk::CaptionName("p9", "MIKExShark", 0) == "Mike" && talk::CaptionName("p10", "lucas.m", 0) == "Lucas", "room names: the name in the shapes handles take");
	Expect(talk::CaptionName(ss::HeroId, "Jesse", 3) == "Jesse", "room names: the player by their first name");
	Expect(talk::Floor("intro.seat", {{"SeatWord", "three"}, {"Chips", "119,378"}, {"Name", "Maria G."}}, 0).Text == "In seat three, with 119,378, Maria G." &&
			talk::Floor("champion", {{"Short", "Sunday"}, {"Name", "J.P."}}, 0).Text == "Ladies and gentlemen, your Embercrest Sunday champion... J.P.!",
		"room words: a name's own period isn't doubled, and the floor's ellipsis stays");
	// An announcement in caption-sized parts: whole sentences, never split at an initial, and nothing lost.
	const std::string Cancel = talk::Floor("cancelled", {{"Players", "6"}, {"Short", "Saturday Big Stack"}}, 0).Text;
	const std::vector<std::string> Parts = room::Announcement(Cancel);
	const std::vector<std::string> Initial = room::Announcement("And that's the bubble! Danny Q. Rivera, thank you for playing. Congratulations, everyone remaining is in the money.");
	Expect(Parts.size() == 2 && Parts[0] + " " + Parts[1] == Cancel && Parts[0].size() <= 96 && Parts[1].size() <= 96 && Initial.size() == 2 &&
			Initial[0] == "And that's the bubble! Danny Q. Rivera, thank you for playing.",
		"room words: a long announcement comes in whole sentences that fit a caption (" + std::to_string(Parts.size()) + " parts)");
	Expect(talk::NumberWords(74) == "seventy-four" && talk::NumberWords(160) == "one hundred sixty" && talk::OrdinalWord(9) == "ninth" && talk::OrdinalWord(21) == "twenty-first" &&
			talk::Ordinal(13) == "13th" && talk::Count(1036000) == "1,036,000" && talk::Dollars(16000) == "$160",
		"room words: numbers the way people say them");
	// A line that can't be filled honestly isn't said.
	life::LiveVoice Plain = MakeVoice("Boots", ss::Archetype::Fish, 0.6f, 0.3f, world::Identity::Recreational, 61);
	talk::Context Ask;
	Ask.Self = &Plain;
	Ask.Vars["Topic"] = "heater";
	Expect(talk::Line(talk::Moment::Small, Ask).Text.empty(), "room talk: no heater talk from someone who isn't on one");
	Ask.Vars["Topic"] = "makeup";
	Expect(talk::Line(talk::Moment::Small, Ask).Text.empty(), "room talk: no makeup talk from someone nobody backs");
	Ask.Vars["Topic"] = "week";
	Ask.Vars["Record"] = "0";
	Expect(talk::Line(talk::Moment::Small, Ask).Text.empty(), "room talk: no result claimed without one in their Story");
	Plain.Story = StoryRecord(20, "Noon Deepstack $80", 9, 74, 16000);
	Ask.Vars["Day"] = "23";
	const talk::Said Week = talk::Line(talk::Moment::Small, Ask);
	Expect(Contains(Week.Text, "inth") && Week.Topic == "story:Boots:20:Noon Deepstack $80", "room talk: a result from the Story, spent by its topic: " + Week.Text);
	Expect(talk::Line(talk::Moment::Needle, talk::Context{&Plain, nullptr, {}, 0, nullptr}).Text.empty(), "room talk: no needle from someone easygoing");
	talk::Context Beat;
	Beat.Self = &Plain;
	Expect(talk::Line(talk::Moment::BadBeat, Beat).Text.empty(), "room talk: no bad beat without the tabled cards");
}

// ------------------------------------------------------------------ RoomVoices

void RoomVoices()
{
	life::LiveEntry E;
	E.Id = "embercrest-nightly@15";
	E.Roster = {{"Mei", static_cast<int>(ss::Archetype::Reg)}, {"Boots", static_cast<int>(ss::Archetype::Fish)}, {"olgapetrov", static_cast<int>(ss::Archetype::Nit)}};
	E.Faces = {{"Boots", "Knows you well", "Back again?"}, {"Mei", "Respects you", ""}};
	const std::vector<life::LiveVoice> V = room::VoicesFor(E);
	Expect(V.size() == 3, "room voices: one for each of the roster");
	if (V.size() == 3)
	{
		Expect(V[0].Name == "Mei" && V[0].Social == 0.15f && V[0].Respect >= 0.5f && V[0].Is == static_cast<int>(world::Identity::DisciplinedPro), "room voices: Mei is the cast's Mei");
		Expect(V[1].Is == static_cast<int>(world::Identity::LiveRegular) && V[1].Age == 61 && V[1].Familiarity >= 0.5f, "room voices: Boots is one of the room's own, and knows you well");
		Expect(V[2].Social >= 0.15f && V[2].Social <= 0.35f && V[2].Story.empty(), "room voices: a nit's sociability, and no story (nobody claims a result)");
		Expect(room::VoicesFor(E)[2].Social == V[2].Social, "room voices: the same every time");
	}
	// VoiceOf: from a world, saved as livevoice and read back exactly.
	world::World W;
	W.Create(0x5eed0003u, ss::net::MinutesPerDay + 127.0);
	W.HeroName = "grinder_3c";
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@4");
	W.AdvanceTo(O.DeskOpens + 30.0);
	life::State L;
	const live::FieldNotes Notes = live::DescribeField(W, O, "grinder_3c", L, O.DeskOpens + 30.0);
	Expect(!Notes.Voices.empty() && Notes.Voices.size() == Notes.Roster.size(), "room voices: DescribeField gives a voice for each of the world's people in the field");
	bool Quantized = true;
	for (const life::LiveVoice& X : Notes.Voices)
	{
		for (const float F : {X.Social, X.Aggro, X.Grit, X.Rivalry, X.Familiarity, X.Respect})
		{
			Quantized = Quantized && static_cast<float>(std::round(static_cast<double>(F) * 1000.0) / 1000.0) == F;
		}
	}
	Expect(Quantized, "room voices: every number a voice keeps is what the save writes (three decimals)");
	life::LiveEntry Entry;
	Entry.Id = O.Id;
	live::Apply(Notes, Entry);
	std::string Lines;
	live::WriteEntryLines(Entry, Lines);
	life::State Back;
	life::LiveEntry Empty;
	Empty.Id = O.Id;
	Back.LiveEntries.push_back(Empty);
	for (const std::string& Line : SplitLines(Lines))
	{
		std::vector<std::string> F;
		size_t A = 0;
		while (true)
		{
			const size_t B = Line.find('\t', A);
			F.push_back(Line.substr(A, B == std::string::npos ? std::string::npos : B - A));
			if (B == std::string::npos)
			{
				break;
			}
			A = B + 1;
		}
		live::ReadEntryLine(F, Back);
	}
	bool Same = Back.LiveEntries[0].Voices.size() == Entry.Voices.size();
	for (size_t I = 0; Same && I < Entry.Voices.size(); ++I)
	{
		const life::LiveVoice& A = Entry.Voices[I];
		const life::LiveVoice& B = Back.LiveEntries[0].Voices[I];
		Same = A.Name == B.Name && A.Style == B.Style && A.Social == B.Social && A.Aggro == B.Aggro && A.Grit == B.Grit && A.Mood == B.Mood && A.Is == B.Is && A.Age == B.Age &&
			A.Pro == B.Pro && A.Staked == B.Staked && A.Rivalry == B.Rivalry && A.Familiarity == B.Familiarity && A.Respect == B.Respect && A.Story == B.Story && A.Ties == B.Ties;
	}
	Expect(Same, "room voices: livevoice lines read back exactly what was written");
	int Storied = 0;
	for (const life::LiveVoice& X : Notes.Voices)
	{
		for (const talk::Result& R : talk::StoryOf(X.Story))
		{
			++Storied;
			const int Npc = W.Find(X.Name);
			const world::Npc* P = W.Get(Npc);
			bool Real = false;
			for (const world::Finish& F : P ? P->Recent : std::vector<world::Finish>())
			{
				Real = Real || (F.Day == R.Day && F.Event == R.Event && F.Place == R.Place && F.Prize == R.Prize);
			}
			Expect(Real, "room voices: a story is one of the world's own records of them (" + X.Name + ")");
		}
		for (const auto& Tie : X.Ties)
		{
			Expect(Tie.second != static_cast<int>(world::TieKind::Backer) && Tie.second != static_cast<int>(world::TieKind::Backed), "room voices: nobody's backing is in their ties");
		}
	}
	std::printf("room voices: %d voices from the world, %d results they can claim\n", static_cast<int>(Notes.Voices.size()), Storied);
}

// ------------------------------------------------------------------ RoomFinalTable (a Sunday)

void RoomFinalTable()
{
	// The Sunday, 36 runners; the player is on Team Embercrest. Dee deals the player's table and calls the champion. The
	// host tells the director mid-hand when an all-in is called (AllInCalled).
	Setup S = MakeNight("embercrest@20", 36, "embercrest", ":sunday23");
	S.Lean = true;
	const Night N = Play(S, 2, 1.0f);
	RoomCaptionFit({&N});
	// The rail leans in while the cards turn: one ooh a hand, and the player's name ("Hold, Jesse! Hold!") then, never
	// after the river.
	std::map<int, int> Oohs;
	int Holds = 0;
	bool HoldsEarly = true;
	for (const auto& TC : N.Cues)
	{
		const room::Cue& C = TC.second;
		if (C.What == room::Kind::Vocal && C.Noise == ss::audio::Vocal::Ooh && C.Where == room::From::Rail)
		{
			++Oohs[TC.first];
		}
		if (Starts(C.Key, "field.rail.allin"))
		{
			++Holds;
			HoldsEarly = HoldsEarly && C.Delay < 1.0 && C.Who == "Mrs. Park" && Contains(C.Text, S.Hero);
		}
	}
	bool OneOoh = true;
	for (const auto& O : Oohs)
	{
		OneOoh = OneOoh && O.second == 1;
	}
	Expect(N.Leans > 0 && OneOoh, "room lean: the rail leans in once a hand, mid-hand when the host says so (" + std::to_string(N.Leans) + " leans)");
	Expect(Holds > 0 && HoldsEarly, "room lean: the backer at the rail calls the player's name while the cards turn (" + std::to_string(Holds) + ")");
	int Opens = 0;
	int Seats = 0;
	bool CountsMatch = true;
	bool HeroHonor = true;
	bool HeroAtFinal = false;
	std::string DeeChampion;
	std::string ChampionLine;
	int DeeCalls = 0;
	int OtherCalls = 0;
	for (const auto& TC : N.Cues)
	{
		const room::Cue& C = TC.second;
		if (Starts(C.Key, "pa.intro.open"))
		{
			++Opens;
		}
		if (Starts(C.Key, "pa.intro") || Starts(C.Key, "pa.champion") || Starts(C.Key, "pa.final"))
		{
			std::printf("  sunday: [%s] %s\n", C.Speaker.c_str(), C.Text.c_str());
		}
		if (Starts(C.Key, "pa.intro.seat"))
		{
			++Seats;
			const auto F = N.Facts.find(TC.first);
			bool Found = false;
			if (F != N.Facts.end())
			{
				for (const auto& Seat : F->second.FinalSeats)
				{
					Found = Found || (Contains(C.Text, "seat " + talk::NumberWords(Seat.first + 1) + ",") && Contains(C.Text, talk::Count(Seat.second)));
				}
			}
			CountsMatch = CountsMatch && Found;
			if (Contains(C.Text, S.HeroFull))
			{
				HeroAtFinal = true;
				HeroHonor = Contains(C.Text, "Team Embercrest's own " + S.HeroFull);
			}
		}
		if (Starts(C.Key, "pa.champion"))
		{
			DeeChampion = C.Speaker;
			ChampionLine = C.Text;
		}
		if (C.What == room::Kind::Call && C.Where == room::From::HeroTable)
		{
			(C.Speaker == "Dee" ? DeeCalls : OtherCalls) += 1;
		}
	}
	int FinalSize = 0;
	for (const auto& F : N.Facts)
	{
		FinalSize = std::max(FinalSize, static_cast<int>(F.second.FinalSeats.size()));
	}
	Expect(Opens == 1, "room final table: the Sunday's introductions open once");
	Expect(Seats == FinalSize && Seats >= 2, "room final table: one introduction a seat (" + std::to_string(Seats) + " of " + std::to_string(FinalSize) + ")");
	Expect(CountsMatch, "room final table: each introduction's count is that seat's stack, as the plates show it");
	Expect(HeroAtFinal && HeroHonor, "room final table: the player is introduced as Team Embercrest's own");
	Expect(DeeChampion == "Dee", "room final table: on a Sunday it's Dee on the PA for the champion");
	Expect(Contains(ChampionLine, "Told you, kid."), "room final table: and when it's the player she deals to, she says so: " + ChampionLine);
	Expect(DeeCalls > 0 && OtherCalls == 0, "room final table: Dee deals the player's table on a Sunday");
	std::printf("room final table: %d introductions (the player %s), champion called by %s\n", Seats, HeroAtFinal ? "among them" : "railing", DeeChampion.c_str());
	// A whole night at the rail: the player wins this Sunday (Dee's "Told you, kid." above), so Mrs. Park has every break,
	// the bubble, the money, the final table and the win to speak to, and the double-ups between. RoomShape's rail check
	// over all of it.
	const RailHeard Rail = RailListen(S, N);
	std::printf("room rail: the Sunday the player wins: %d lines from the backer at the rail\n", Rail.Lines);
	Expect(Rail.Lines >= 6 && Rail.Problem.empty(), "room rail: through a whole night a backer at the rail never says a line twice running, nor a rail line twice in a night till its pool's said through (" +
		std::to_string(Rail.Lines) + " lines)" + Rail.Problem);

	// The Nightly isn't a flagship: the final-table call, and at most one honorific sentence.
	const Setup Nightly = MakeNight("embercrest-nightly@16", 40, "luckypenny", ":ft");
	const Night M = Play(Nightly, 1, 1.0f);
	int Intros = 0;
	int Honors = 0;
	for (const auto& TC : M.Cues)
	{
		Intros += Starts(TC.second.Key, "pa.intro") ? 1 : 0;
		Honors += Starts(TC.second.Key, "pa.final.honor") ? 1 : 0;
	}
	Expect(Intros == 0 && Honors <= 1, "room final table: a Nightly gets no introductions and at most one honorific sentence");
}

// ------------------------------------------------------------------ RoomRailRounds

/**
 * Mrs. Park at the rail and the player doubling up hand after hand, seven times (more than a night gives, so her three
 * double-up lines are said through twice): each round dealt fresh, never one line twice running and never the last of a
 * round first in the next (RailListen). Taken up from a checkpoint partway through the second round, the director still
 * knows what she's said tonight (its liveroom keeps it), so the round goes on rather than starting over.
 */
void RoomRailRounds()
{
	const SmallTable S = MakeSmallTable(0, 0.3f);
	ss::Tournament T(S.Spec, "Jesse", "rail", S.Known);
	const size_t Hero = static_cast<size_t>(T.PlayerIndex(ss::HeroId));
	const std::vector<room::RailBacker> Backers = {{"Mrs. Park", 150}};
	auto Fresh = [&S, &Backers]() {
		auto D = std::make_unique<room::Director>(S.E, S.O, "rail:room", "Jesse");
		D->SetRail(Backers);
		return D;
	};
	std::unique_ptr<room::Director> D = Fresh();
	D->Clock(T, S.O.Start, 0.0);
	D->Take();
	const int Ups = 7;
	const int Resumed = 6; // taken up before this hand: four double-ups said, one into the second round
	std::vector<std::pair<int, room::Cue>> Heard;
	for (int H = 1; H <= Ups + 1; ++H)
	{
		if (H == Resumed)
		{
			const std::string Kept = D->Write();
			D = Fresh();
			D->Resume(T, Kept, S.O.Start + 0.75 * H);
		}
		T.Tick = H;
		if (H > 1)
		{
			T.Players[Hero].Stack *= 2; // the hand before the first sets the stack she measures from
		}
		room::HandNote N;
		N.Hand = H;
		for (const ss::TPlayer& P : T.Players)
		{
			(P.IsHero ? N.InHand : N.Folded).push_back(P.IsHero ? std::string(ss::HeroId) : P.Name);
		}
		N.Winners = {ss::HeroId};
		D->Hand(T, N);
		for (const room::Cue& C : D->Take())
		{
			Heard.push_back({H, C});
		}
	}
	const RailHeard Rail = RailListen(S.E.Id, Heard);
	const int Said = Rail.Pools.count(static_cast<int>(talk::Moment::DoubleUp)) ? Rail.Pools.at(static_cast<int>(talk::Moment::DoubleUp)) : 0;
	std::string Lines;
	for (const auto& TC : Heard)
	{
		if (TC.second.Where == room::From::Rail && TC.second.Key == "stake.rail")
		{
			Lines += (Lines.empty() ? "" : " / ") + TC.second.Text;
		}
	}
	std::printf("room rail rounds: %d double-ups, %d lines from her pool, %d rounds dealt fresh: %s\n", Ups, Said, Rail.Fresh, Lines.c_str());
	Expect(Said == Ups && Rail.Lines == Ups && Rail.Fresh == 2 && Rail.Problem.empty(),
		"room rail rounds: a backer's three double-up lines, said through twice across a checkpoint, each round dealt fresh and none twice running (" +
			std::to_string(Said) + " of " + std::to_string(Ups) + ", " + std::to_string(Rail.Fresh) + " fresh)" + Rail.Problem);
}

// ------------------------------------------------------------------ RoomFresh (update 2 §3.3 F9: a fresh world's books)

/** Night One, 2:07 AM on Tuesday (day 1): when a fresh world's books open. */
const double FreshOpened = 1440.0 + 127.0;

double FreshEnd(const live::Occurrence& O)
{
	return O.Start + O.T->Hours * 60.0;
}

/** The world's text read as the card room's host reads it (every line, no Finish). */
void FreshRead(world::World& W, const std::string& Text)
{
	for (const std::string& L : SplitLines(Text))
	{
		std::vector<std::string> F;
		size_t A = 0;
		while (true)
		{
			const size_t B = L.find('\t', A);
			F.push_back(L.substr(A, B == std::string::npos ? std::string::npos : B - A));
			if (B == std::string::npos)
			{
				break;
			}
			A = B + 1;
		}
		W.Read(F);
	}
}

bool SameBooks(const room::Books& A, const room::Books& B)
{
	bool Same = A.Blank == B.Blank && A.Opened == B.Opened && A.Known == B.Known && A.Cancelled == B.Cancelled && A.HeroWins == B.HeroWins &&
		A.Most == B.Most && A.MostWins == B.MostWins && A.Firsts.size() == B.Firsts.size();
	for (size_t I = 0; Same && I < A.Firsts.size(); ++I)
	{
		Same = A.Firsts[I].Id == B.Firsts[I].Id && A.Firsts[I].Winner == B.Firsts[I].Winner && A.Firsts[I].Hero == B.Firsts[I].Hero;
	}
	return Same;
}

/** BooksOf against a living fresh world: the first Nightly decided by three on Night One, the save's text, the player's own. */
void RoomFreshBooksOf()
{
	world::World W;
	W.Create(0x5eedf5e5u, FreshOpened, world::Start::Blank);
	W.HeroName = "Jesse";
	const live::Occurrence Monday = live::FindOccurrence("embercrest-nightly@0");
	const live::Occurrence Noon = live::FindOccurrence("embercrest-noon@1");
	Expect(Monday.Valid() && Noon.Valid() && FreshEnd(Monday) > FreshOpened && FreshEnd(Monday) < Noon.Start,
		"room fresh books: Monday's Nightly is dealt across the books' opening and ends before Tuesday's Noon Deepstack");
	W.AdvanceTo(Noon.Start - 30.0);
	const room::Books B = room::BooksOf(W, {});
	Expect(B.Blank && B.Opened == FreshOpened && B.Known == W.Clock(), "room fresh books: a fresh world's books, opened on Night One, read at the world's clock");
	std::string Winner;
	if (const ss::net::EventResult* R = W.ResultOf(Monday.Id))
	{
		for (const ss::net::Placing& P : R->FinalTable)
		{
			if (P.Place == 1)
			{
				const world::Npc* N = P.Player >= 0 ? W.Get(P.Player) : nullptr;
				Winner = N ? N->Name : P.Name;
			}
		}
	}
	std::printf("room fresh books: the first Nightly in the books (%s, over %s) went to %s; Tuesday 11:30 knows %d first(s), %d called off\n", Monday.Id.c_str(),
		ss::net::TimeLabel(FreshEnd(Monday)).c_str(), Winner.empty() ? "(nobody the world shows)" : Winner.c_str(), static_cast<int>(B.Firsts.size()),
		static_cast<int>(B.Cancelled.size()));
	Expect(!Winner.empty() && B.Firsts.size() == 1 && B.Firsts.front().Id == Monday.Id && B.Firsts.front().Winner == Winner && !B.Firsts.front().Hero,
		"room fresh books: the first Nightly is decided at 3 AM on Night One, the books' only first by Tuesday noon, and its winner is the world's");
	// The save's text as the card room's host reads it (BooksOfText: the lines the books need), against the whole world.
	std::string Text;
	W.Write(Text);
	room::Books Read;
	const double Ms = Timed([&] { Read = room::BooksOfText(Text, {}, Noon.Start); });
	world::World Copy;
	room::Books Whole;
	const double WholeMs = Timed([&] {
		FreshRead(Copy, Text);
		Whole = room::BooksOf(Copy, {});
	});
	std::printf("room fresh books: the host reads the books from %d KB of world in %.1f ms (the whole world: %.1f ms)\n", static_cast<int>(Text.size() / 1024), Ms, WholeMs);
	Expect(SameBooks(B, Read) && SameBooks(B, Whole), "room fresh books: the save's world, read back, has the same books (the host's lines and the whole world)");
	// An established world's head (Start 0), a world never made, a fresh one past its first month: no books.
	{
		std::string Old;
		for (const std::string& L : SplitLines(Text))
		{
			Old += (Starts(L, "world\thead\t") ? L.substr(0, L.rfind('\t')) + "\t0" : L) + "\n";
		}
		world::World Est;
		FreshRead(Est, Old);
		world::World None;
		double EstMs = 0.0;
		bool EstBlank = true;
		EstMs = Timed([&] { EstBlank = room::BooksOfText(Old, {}, Noon.Start).Blank; });
		Expect(Est.Ready() && !room::BooksOf(Est, {}).Blank && !EstBlank && !room::BooksOf(None, {}).Blank && !room::BooksOfText(std::string(), {}, Noon.Start).Blank,
			"room fresh books: an established world (and no world) has none");
		Expect(!room::BooksOfText(Text, {}, FreshOpened + room::FreshBooksDays * 1440.0).Blank && room::BooksOfText(Text, {}, FreshOpened + 29.5 * 1440.0).Blank,
			"room fresh books: the host reads none past the books' first thirty days");
		std::printf("room fresh books: an established world's text costs the host %.3f ms\n", EstMs);
	}
	// The player's own night, settled in the room before the world hears of it, is the first Noon Deepstack; one they're
	// still playing holds its family.
	life::LiveEntry Mine;
	Mine.Id = Noon.Id;
	Mine.State = life::LiveEntry::Finished;
	Mine.Place = 1;
	const room::Books Theirs = room::BooksOf(W, {Mine});
	Mine.State = life::LiveEntry::Registered;
	const room::Books Playing = room::BooksOf(W, {Mine});
	Expect(Theirs.Firsts.size() == 2 && Theirs.Firsts[1].Id == Noon.Id && Theirs.Firsts[1].Hero && Playing.Firsts.size() == 1,
		"room fresh books: the player's settled win is their first; a night still being played decides nothing");
	// Tuesday night: the Noon Deepstack is over, the world has its winner.
	W.AdvanceTo(FreshEnd(Noon) + 60.0);
	const room::Books Night = room::BooksOf(W, {});
	Expect(Night.Firsts.size() == 2 && Night.Firsts[1].Id == Noon.Id && (Night.Firsts[1].Hero || !Night.Firsts[1].Winner.empty()),
		"room fresh books: by Tuesday night the first Noon Deepstack is in, with its winner");
	// A week on (results are kept a few days, the news longer): the firsts are still the firsts.
	W.AdvanceTo(FreshOpened + 8.0 * 1440.0);
	std::string Later;
	W.Write(Later);
	const room::Books Old = room::BooksOfText(Later, {}, W.Clock());
	int Named = 0;
	for (const room::Books::First& F : Old.Firsts)
	{
		Named += F.Hero || !F.Winner.empty() ? 1 : 0;
	}
	std::printf("room fresh books: a week on, %d firsts decided, %d with the winner the books still show; the most wins up the hill %d (%s)\n",
		static_cast<int>(Old.Firsts.size()), Named, Old.MostWins, Old.Most.c_str());
	Expect(Old.Firsts.size() == 6 && Old.Firsts.front().Id == Monday.Id && Old.Firsts.front().Winner == Winner,
		"room fresh books: a week on, every one of the room's six events has its first, the Nightly's still the oldest, named from the world's news of it");
}

/** What a night's first-month lines said, checked against the books they were said from. */
struct FreshHeard
{
	int Lines = 0;
	int Answers = 0;
	int ToYou = 0;
	std::map<std::string, int> Topics;
	std::vector<std::string> Said;
	std::vector<std::pair<std::string, double>> When; // what each spent, and when (world minutes)
	std::string Problem;
};

FreshHeard FreshListen(const Setup& S, const Night& N)
{
	FreshHeard H;
	const room::Books& B = S.Books;
	std::map<std::string, std::string> Templates;
	for (const auto& Kv : talk::Inventory())
	{
		Templates[Kv.first] = Kv.second;
	}
	const auto Cancelled = [&B](const std::string& Id) { return std::find(B.Cancelled.begin(), B.Cancelled.end(), Id) != B.Cancelled.end(); };
	const auto Decided = [&](const live::Occurrence& O, double At) {
		const bool First = std::any_of(B.Firsts.begin(), B.Firsts.end(), [&O](const room::Books::First& F) { return F.Id == O.Id; });
		return O.Id != S.O.Id && (First || (FreshEnd(O) <= At && !Cancelled(O.Id)));
	};
	const auto Room = [&](double At) {
		std::vector<live::Occurrence> Out;
		for (int D = ss::net::DayOf(B.Opened) - 1; D <= ss::net::DayOf(At) + 6; ++D)
		{
			for (const live::Occurrence& O : live::Occurrences(D))
			{
				if (FreshEnd(O) > B.Opened)
				{
					Out.push_back(O);
				}
			}
		}
		return Out;
	};
	const auto Won = [&](const std::string& Key, double At) {
		for (const live::Occurrence& O : Room(At))
		{
			if (O.T->Key == Key && Decided(O, At))
			{
				return true;
			}
		}
		return false;
	};
	for (const auto& TC : N.Cues)
	{
		const room::Cue& C = TC.second;
		if (C.What != room::Kind::Line || !Starts(C.Key, "field.fresh."))
		{
			continue;
		}
		const auto Fact = N.Facts.find(TC.first);
		const double At = Fact != N.Facts.end() ? Fact->second.World : S.O.Start;
		const int Day = ss::net::DayOf(At);
		const std::string What = "\"" + C.Speaker + ": " + C.Text + "\" (" + C.Key + ", day " + std::to_string(Day) + " " + ss::net::TimeLabel(At) + ")";
		const auto Problem = [&H, &What](const std::string& Why) {
			if (H.Problem.empty())
			{
				H.Problem = Why + ": " + What;
			}
		};
		if (Contains(C.Key, ".reply."))
		{
			++H.Answers;
			continue;
		}
		++H.Lines;
		H.ToYou += C.ToHero ? 1 : 0;
		H.Said.push_back(What);
		H.When.push_back({C.Topic, At});
		if (++H.Topics[C.Topic] > 1)
		{
			Problem("said twice in a night");
		}
		const std::string Name = N.NameOfId.count(C.Who) ? N.NameOfId.at(C.Who) : C.Who;
		const life::LiveVoice* V = nullptr;
		for (const life::LiveVoice& Each : S.E.Voices)
		{
			V = Each.Name == Name ? &Each : V;
		}
		if (!talk::KnownByName(Name) && !(V && V->Is == static_cast<int>(world::Identity::LiveRegular)))
		{
			Problem("not one of the room's own");
		}
		if (!B.Blank || Day - ss::net::DayOf(B.Opened) >= room::FreshBooksDays)
		{
			Problem("past the books' first month");
		}
		const std::string Template = Templates[C.Key];
		const std::string Key = Starts(C.Topic, "fresh:") ? C.Topic.substr(6) : std::string();
		const bool You = Contains(C.Key, ".you.");
		const bool Self = Contains(C.Key, ".self.");
		if (You != C.ToHero)
		{
			Problem("a line about the player not said to them");
		}
		if (Key == "tonight")
		{
			if (Won(S.O.T->Key, At))
			{
				Problem("tonight's isn't the first");
			}
		}
		else if (Starts(Key, "next:"))
		{
			const std::string Family = Key.substr(5);
			if (Family == S.O.T->Key || Won(Family, At))
			{
				Problem("that first is decided");
			}
			live::Occurrence Next;
			for (const live::Occurrence& O : Room(At))
			{
				Next = !Next.Valid() && O.T->Key == Family && FreshEnd(O) > At ? O : Next;
			}
			const bool Evening = Next.Valid() && Next.T->StartMinute >= 17 * 60;
			const std::string When = !Next.Valid() ? std::string("?") : Next.Start <= At ? std::string("on right now") : Next.Day == Day ? std::string(Evening ? "tonight" : "later today")
				: Next.Day == Day + 1 ? std::string(Evening ? "tomorrow night" : "tomorrow") : std::string(ss::net::WeekdayName(Next.Day, true)) + (Evening ? " night" : "");
			if (!Contains(C.Text, When + "."))
			{
				Problem("the wrong day for it (" + When + ")");
			}
		}
		else if (Key == "wall")
		{
			for (const live::Occurrence& O : Room(At))
			{
				if (Decided(O, At))
				{
					Problem("the board isn't blank (" + O.Id + ")");
				}
			}
		}
		else if (Key == "wallfirst" || Starts(Key, "first:"))
		{
			const room::Books::First* F = nullptr;
			for (const room::Books::First& Each : B.Firsts)
			{
				const live::Occurrence O = live::FindOccurrence(Each.Id);
				F = !F && (Key == "wallfirst" || Key == "first:" + std::string(O.T->Key)) ? &Each : F;
			}
			if (!F)
			{
				Problem("no such first in the books");
				continue;
			}
			const live::Occurrence Of = live::FindOccurrence(F->Id);
			for (const live::Occurrence& O : Room(At))
			{
				if (O.Id != Of.Id && FreshEnd(O) <= FreshEnd(Of) && (Key == "wallfirst" || O.T->Key == Of.T->Key) && !Cancelled(O.Id))
				{
					Problem("something ended before that first (" + O.Id + ")");
				}
			}
			const int Ago = Day - Of.Day;
			const bool Evening = Of.T->StartMinute >= 17 * 60;
			const std::string Then = Ago == 1 ? std::string(Evening ? "last night" : "yesterday") : Ago < 7 ? std::string(ss::net::WeekdayName(Of.Day, true)) + (Evening ? " night" : "")
																					   : std::string("opening week");
			if (F->Hero != You || (Self && Name != F->Winner) || (!You && !Self && !Contains(C.Text, F->Winner)))
			{
				Problem("whose first it was");
			}
			if (Contains(Template, "{FreshThen") && !Contains(Lower(C.Text), Lower(Then)))
			{
				Problem("the wrong day for it (" + Then + ")");
			}
		}
		else if (Key == "twice" || Key == "twice:you")
		{
			if ((Key == "twice" ? B.MostWins : B.HeroWins) < 2 || (Key == "twice" && !Self && !Contains(C.Text, B.Most)) || (Self && Name != B.Most))
			{
				Problem("more than once, or whose");
			}
		}
		else if (Key == "books")
		{
			if (Day - ss::net::DayOf(B.Opened) >= 7)
			{
				Problem("past the books' first week");
			}
		}
		else
		{
			Problem("a topic nobody offers");
		}
	}
	return H;
}

void FreshReport(const char* Name, const FreshHeard& H)
{
	std::printf("room fresh: %s: %d first-month lines (%d to you), %d answers\n", Name, H.Lines, H.ToYou, H.Answers);
	for (size_t I = 0; I < H.Said.size() && I < 4; ++I)
	{
		std::printf("  %s\n", H.Said[I].c_str());
	}
	if (!H.Problem.empty())
	{
		std::printf("  PROBLEM %s\n", H.Problem.c_str());
	}
}

/**
 * The director on a fresh world's books (update 2 §3.3 F9): the regulars talk about the room's firsts only while they're
 * true, an established world's room is unchanged, and past the books' first month a fresh one's is too.
 */
void RoomFresh()
{
	// The talk itself: only from the director's books, once a night, and the answers claim nothing.
	{
		talk::Context C;
		const life::LiveVoice Boots = MakeVoice("Boots", ss::Archetype::Fish, 0.6f, 0.3f, world::Identity::LiveRegular, 55);
		C.Self = &Boots;
		C.Vars = {{"Topic", "fresh.next"}, {"FreshEvent", "Big Stack"}, {"FreshWhen", "Saturday"}};
		const talk::Said Bare = talk::Line(talk::Moment::Small, C);
		C.Vars["Books"] = "fresh";
		const talk::Said Keyless = talk::Line(talk::Moment::Small, C);
		C.Vars["FreshKey"] = "next:bigstack";
		const talk::Said Said = talk::Line(talk::Moment::Small, C);
		std::set<std::string> Told;
		talk::Remember(Told, Said);
		C.Told = &Told;
		const talk::Said Again = talk::Line(talk::Moment::Small, C);
		Expect(Bare.Text.empty() && Keyless.Text.empty() && Contains(Said.Text, "Big Stack") && Contains(Said.Text, "Saturday.") && Said.Topic == "fresh:next:bigstack" &&
				   Again.Text.empty(),
			"room fresh: a first-month line only with the director's books, once a night (" + Said.Text + ")");
		const auto Answer = [&Boots](const std::string& Topic) {
			talk::Context A;
			A.Self = &Boots;
			A.Vars = {{"Topic", Topic}};
			return talk::Line(talk::Moment::Answer, A);
		};
		Expect(Answer("fresh.first.you").Text.empty() && Starts(Answer("fresh.first").Key, "field.fresh.first.reply.") &&
				   Starts(Answer("fresh.wallfirst.self").Key, "field.fresh.self.reply.") && Starts(Answer("fresh.next.nightly").Key, "field.fresh.reply."),
			"room fresh: the answers (none to a line said to the player)");
	}
	const double Tue = 1440.0;
	const double Wed = 2.0 * 1440.0;
	// Tuesday's Noon Deepstack, the first in the books; the first Nightly went to Maria G. at three on Night One.
	// (Only the room's own talk about the books, and only at the player's table: the draw, not the cards, seats them
	// there from the first hand, so the talk doesn't wait on the player lasting to a table move. Three draws of it, each
	// held to all of this; the first is the night the rest of this test goes on from.)
	Setup A;
	Night NA;
	for (const char* Salt : {":freshb", ":fresho", ":fresha2"})
	{
		Setup Each = MakeNight("embercrest-noon@1", 72, "", Salt);
		Each.Books.Blank = true;
		Each.Books.Opened = FreshOpened;
		Each.Books.Known = Tue + 11.5 * 60.0;
		Each.Books.Firsts = {{"embercrest-nightly@0", "Maria G.", false}};
		Each.Books.Most = "Maria G.";
		Each.Books.MostWins = 1;
		{
			const std::unique_ptr<ss::Tournament> Draw = live::MakeField(Each.O, Each.Entrants, Each.Hero, Each.Seed, Each.Known);
			int Own = 0;
			std::string Names;
			for (const ss::TPlayer& P : Draw->Players)
			{
				bool Regular = talk::KnownByName(P.Name);
				for (const life::LiveVoice& V : Each.E.Voices)
				{
					Regular = Regular || (V.Name == P.Name && V.Is == static_cast<int>(world::Identity::LiveRegular));
				}
				if (Regular && !P.IsHero && P.TableId == Draw->Hero().TableId)
				{
					++Own;
					Names += (Names.empty() ? "" : ", ") + P.Name;
				}
			}
			std::printf("room fresh: the first Noon Deepstack's draw (%s) seats %s with the player\n", Salt, Names.c_str());
			Expect(Own >= 3, std::string("room fresh: the first Noon Deepstack's draw seats the room's own at the player's table from the first hand (") + Salt + ": " +
				std::to_string(Own) + ")");
		}
		const Night NEach = Play(Each, 4, 1.5f);
		const FreshHeard HEach = FreshListen(Each, NEach);
		FreshReport("the first Noon Deepstack in the books", HEach);
		Expect(HEach.Problem.empty() && HEach.Lines >= 3 &&
				   (HEach.Topics.count("fresh:tonight") || HEach.Topics.count("fresh:next:bigstack") || HEach.Topics.count("fresh:next:turbo")),
			std::string("room fresh: at the first Noon Deepstack in the books the regulars talk about the firsts, each only while it's true (") + Salt + ")");
		if (A.Seed.empty())
		{
			A = Each;
			NA = NEach;
		}
	}
	// The same night in an established world (the books' facts ignored without Blank) says exactly what it always did.
	{
		Setup Est = A;
		Est.Books.Blank = false;
		Setup Plain = A;
		Plain.Books = room::Books();
		const Night NPlain = Play(Plain, 4, 1.5f);
		Expect(Play(Est, 4, 1.5f).Log == NPlain.Log, "room fresh: an established world's room is unchanged (the books' facts ignored)");
		std::printf("room fresh: the same night without the books: %.4f ms a hand end (with them: %.4f)\n", NPlain.PlainMs / std::max(1, NPlain.PlainTicks),
			NA.PlainMs / std::max(1, NA.PlainTicks));
	}
	// Tuesday's Nightly, the world read at six: the Noon Deepstack's first is still being played until nine, then decided
	// (whoever won it: the room can't say), and the question stops.
	// (Three nights of it, at the chattiest table setting, so the question comes up before nine and the talk goes on after.)
	const double Nine = FreshEnd(live::FindOccurrence("embercrest-noon@1"));
	int AskedBefore = 0;
	int AskedAfter = 0;
	int TalkAfter = 0;
	std::string BProblem;
	for (const char* Salt : {":fresh2", ":fresh2b", ":fresh2c"})
	{
		Setup B = MakeNight("embercrest-nightly@1", 96, "", Salt);
		B.Books = A.Books;
		B.Books.Known = Tue + 18.0 * 60.0;
		const Night NB = Play(B, 4, 2.0f);
		const FreshHeard HB = FreshListen(B, NB);
		FreshReport("Tuesday's Nightly, the first Noon Deepstack over at nine", HB);
		for (const std::pair<std::string, double>& W : HB.When)
		{
			AskedBefore += W.first == "fresh:next:noon" && W.second < Nine ? 1 : 0;
			AskedAfter += W.first == "fresh:next:noon" && W.second >= Nine ? 1 : 0;
			TalkAfter += W.second >= Nine ? 1 : 0;
		}
		const bool Wrong = !HB.Problem.empty() || HB.Topics.count("fresh:tonight") || HB.Topics.count("fresh:next:nightly");
		BProblem = BProblem.empty() && Wrong ? HB.Problem + " (or the second Nightly called the first)" : BProblem;
	}
	// And a night taken up after nine with nothing said yet (an established night's checkpoint, resumed on the fresh books):
	// every other topic can still come up, the Noon Deepstack's first never.
	int ResumedLines = 0;
	{
		Setup Plain = MakeNight("embercrest-nightly@1", 96, "", ":fresh2b");
		const Night NP = Play(Plain, 4, 2.0f);
		int From = -1;
		for (const auto& F : NP.Facts)
		{
			From = From < 0 && F.second.HadNote && F.second.HeroTable >= 0 && F.second.World >= Nine + 5.0 && NP.Saves.count(F.first) ? F.first : From;
		}
		Setup B = Plain;
		B.Books = A.Books;
		B.Books.Known = Tue + 18.0 * 60.0;
		if (From >= 0)
		{
			const FreshHeard HR = FreshListen(B, Play(B, 4, 2.0f, From, &NP.Saves.at(From)));
			FreshReport("Tuesday's Nightly, taken up after nine", HR);
			ResumedLines = HR.Lines;
			for (const std::pair<std::string, double>& W : HR.When)
			{
				AskedAfter += W.first == "fresh:next:noon" ? 1 : 0;
				TalkAfter += W.second >= Nine ? 1 : 0;
			}
			BProblem = BProblem.empty() ? HR.Problem : BProblem;
		}
		std::printf("room fresh: Tuesday's Nightly taken up at hand %d (%s)\n", From, From >= 0 ? ss::net::TimeLabel(NP.Facts.at(From).World).c_str() : "never");
	}
	std::printf("room fresh: Tuesday's Nightlies: the first Noon Deepstack asked about %d time(s) before %s, %d after; %d first-month line(s) after it\n", AskedBefore,
		ss::net::TimeLabel(Nine).c_str(), AskedAfter, TalkAfter);
	Expect(BProblem.empty() && AskedBefore >= 1 && AskedAfter == 0 && TalkAfter >= 1 && ResumedLines >= 1,
		"room fresh: at the second Nightly in the books nobody calls it the first, and the Noon Deepstack's first is asked about only until nine" + BProblem);
	// Monday's Nightly called off: on Tuesday nothing in the books has a winner, the board is blank, and the first Nightly is
	// tonight.
	// (Three nights of it, so the blank board and the first Nightly both come up.)
	int Blank = 0;
	int FirstNightly = 0;
	std::string EProblem;
	for (const char* Salt : {":fresh3", ":fresh3b", ":fresh3c"})
	{
		Setup E = MakeNight("embercrest-noon@1", 72, "", Salt);
		E.Books.Blank = true;
		E.Books.Opened = FreshOpened;
		E.Books.Known = Tue + 11.5 * 60.0;
		E.Books.Cancelled = {"embercrest-nightly@0"};
		const FreshHeard HE = FreshListen(E, Play(E, 4, 1.5f));
		FreshReport("Monday's Nightly called off", HE);
		Blank += static_cast<int>(HE.Topics.count("fresh:wall"));
		FirstNightly += static_cast<int>(HE.Topics.count("fresh:next:nightly"));
		EProblem = EProblem.empty() && (!HE.Problem.empty() || HE.Topics.count("fresh:wallfirst")) ? HE.Problem + " (or a first name claimed)" : EProblem;
	}
	Expect(EProblem.empty() && Blank >= 1 && FirstNightly >= 1, "room fresh: with nothing won yet the board is blank and the first Nightly is still to come (" +
		std::to_string(Blank) + " blank, " + std::to_string(FirstNightly) + " first Nightly)" + EProblem);
	// Wednesday's Noon Deepstack: the player took the first Nightly and has two wins; Boots took the first Noon Deepstack.
	Setup F = MakeNight("embercrest-noon@2", 72, "", ":fresh4");
	F.Books.Blank = true;
	F.Books.Opened = FreshOpened;
	F.Books.Known = Wed + 11.0 * 60.0;
	F.Books.Firsts = {{"embercrest-nightly@0", "", true}, {"embercrest-noon@1", "Boots", false}};
	F.Books.HeroWins = 2;
	F.Books.Most = "Boots";
	F.Books.MostWins = 1;
	const Night NF = Play(F, 4, 1.5f);
	const FreshHeard HF = FreshListen(F, NF);
	FreshReport("the player's first, and Boots's", HF);
	Expect(HF.Problem.empty() && HF.ToYou >= 1 && !HF.Topics.count("fresh:tonight"),
		"room fresh: the player's own first is said to them, Boots's by name, and nobody calls Wednesday's the first");
	// Past the books' first month a fresh world's room is an established one's.
	Setup C = MakeNight("embercrest-nightly@36", 60, "", ":fresh5");
	C.Books = A.Books;
	C.Books.Known = 36.0 * 1440.0 + 17.0 * 60.0;
	const Night NC = Play(C, 4, 1.0f);
	Setup CPlain = C;
	CPlain.Books = room::Books();
	Expect(FreshListen(C, NC).Lines == 0 && NC.Log == Play(CPlain, 4, 1.0f).Log, "room fresh: past the books' first month the room says what an established one would");
	// What the books cost the director (a fresh night's hands against the golden's budget of 0.05 ms).
	const double FreshPlain = NA.PlainMs / std::max(1, NA.PlainTicks);
	std::printf("room fresh: the director takes %.4f ms a hand end on fresh books (budget 0.05), %.4f with the night's equity runs averaged in\n", FreshPlain,
		NA.DirectorMs / std::max(1, NA.Ticks));
	Expect(FreshPlain < 0.25, "room fresh: the books cost the director next to nothing a hand");
}
} // namespace room_test

namespace room_field
{
/** RoomField (the lead's, Phase A): tests/room_field.cpp. The failures it found. */
int Run();
}
namespace room_seg_auto
{
/** Update 2 (update 2 SPEC §6.7): core-autoplay's LiveSkipSliceEqualsTicks and RoomPace under the plan, tests/room_seg_auto.cpp. */
int Run();
}

int main(int argc, char** argv)
{
	const bool Write = argc > 1 && std::strcmp(argv[1], "--write") == 0;
	const auto Started = std::chrono::steady_clock::now();
	room_test::Failures += copylint::SelfCheck();
	room_test::RoomTalkCopy();
	room_test::RoomVoices();
	room_test::RoomRates();
	room_test::RoomNoticed();
	room_test::RoomAsk();
	room_test::RoomNeedleUp();
	room_test::RoomMoves();
	room_test::RoomOrder();
	room_test::RoomHeroHonor();
	room_test::RoomHushAtHandForHand();
	room_test::RoomHushAtLoneFinalTable();
	room_test::RoomAwayThenHandForHand();
	room_test::RoomRailOneVoice();
	room_test::RoomNightly(Write);
	room_test::RoomFinalTable();
	room_test::RoomRailRounds();
	room_test::RoomFreshBooksOf();
	room_test::RoomFresh();
	room_test::Failures += room_field::Run();
	room_test::Failures += room_seg_auto::Run();
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Started).count();
	std::printf("room tests: %.0f ms\n", Ms);
	if (room_test::Failures == 0)
	{
		std::printf("room tests: all passed\n");
	}
	return room_test::Failures == 0 ? 0 : 1;
}
