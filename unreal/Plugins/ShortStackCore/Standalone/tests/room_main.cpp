// card_room (next_segment_spec §7.6, suite 13): core-room's tests.
//   RoomNightly      a seeded 96-entrant Nightly whose full cue log matches Tests/room_nightly.txt (room_test --write
//                    regenerates it); the PA's order and numbers; honesty (every "all in and a call, table N" has a
//                    TableRound{N, Called} that tick; every "seat open, N" has a bust at N; every named card was tabled;
//                    every claimed result is in the speaker's Story; no talk from folded players; no needle at a bust).
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
//                    on the PA for the champion.
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

void Expect(bool Condition, const std::string& What)
{
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What.c_str());
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
		for (const room::Cue& C : D.Take())
		{
			N.Log.push_back(CueLine(Tick, C));
			N.Cues.push_back({Tick, C});
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

// ------------------------------------------------------------------ RoomShape: the night as it's heard

/**
 * The order and spacing a listener hears: the break's five minutes; a seated player told of a move by their dealer;
 * one question to the dealer in six hands; the table reacting to the bubble after the floor names it; nobody at the
 * rail saying two things at once; no table shouting its own number the hand the player busts there; no line twice
 * running from one pool; "Hold!" never after the river; the champion called once the rail's ooh has landed.
 */
void RoomShape(const Night& N)
{
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
	bool OneVoice = true;
	bool OwnNumber = true;
	bool NoHoldLate = true;
	bool ChampionAfter = true;
	std::map<std::string, std::string> LastOfPool;
	std::vector<std::string> Repeats;
	std::map<std::pair<int, std::string>, std::vector<double>> RailAt;
	// What each backer at the rail has said tonight, by key (stake.rail, stake.break), in order.
	std::map<std::pair<std::string, std::string>, std::vector<std::string>> RailSaid;
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
			AsksApart = AsksApart && Tick - LastAsk >= 6;
			LastAsk = Tick;
		}
		if (Starts(C.Key, "field.bubble"))
		{
			double PaEnds = -1.0;
			for (size_t J = 0; J < N.Cues.size(); ++J)
			{
				if (N.Cues[J].first == Tick && Starts(N.Cues[J].second.Key, "pa.bubble"))
				{
					PaEnds = std::max(PaEnds, N.Cues[J].second.Delay);
				}
			}
			BubbleAfter = BubbleAfter && PaEnds >= 0.0 && C.Delay > PaEnds;
		}
		if (C.What == room::Kind::Line && C.Where == room::From::Rail)
		{
			for (const double At : RailAt[{Tick, C.Who}])
			{
				OneVoice = OneVoice && std::fabs(At - C.Delay) >= 1.0;
			}
			RailAt[{Tick, C.Who}].push_back(C.Delay);
		}
		if (C.Where == room::From::Rail && (C.Key == "stake.rail" || C.Key == "stake.break"))
		{
			RailSaid[{C.Who, C.Key}].push_back(C.Text);
		}
		if (Starts(C.Key, "dealer.table.") && F != N.Facts.end())
		{
			OwnNumber = OwnNumber && C.TableId != F->second.HeroTable;
		}
		NoHoldLate = NoHoldLate && !Starts(C.Key, "field.rail.allin");
		if (Starts(C.Key, "pa.champion"))
		{
			for (size_t J = 0; J < N.Cues.size(); ++J)
			{
				const room::Cue& V = N.Cues[J].second;
				if (N.Cues[J].first == Tick && V.What == room::Kind::Vocal && V.Noise == ss::audio::Vocal::Ooh)
				{
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
	Expect(OneVoice, "room shape: nobody at the rail says two things at once");
	Expect(OwnNumber, "room shape: no table call shouts the player's own table");
	Expect(NoHoldLate, "room shape: \"Hold!\" only while the cards turn (AllInCalled), never at the hand's end");
	Expect(ChampionAfter, "room shape: the floor calls the champion once the rail's ooh has landed");
	for (const std::string& R : Repeats)
	{
		std::printf("room shape: said twice running: %s\n", R.c_str());
	}
	Expect(Repeats.empty(), "room shape: no line twice running from one pool");
	// A backer's lines (stake::RailLine's pools, three a moment for what comes up more than once a night): nothing twice
	// running, and the double-ups and bad beats all different.
	bool RailFresh = true;
	int RailLines = 0;
	for (const auto& Said : RailSaid)
	{
		RailLines += static_cast<int>(Said.second.size());
		for (size_t K = 1; K < Said.second.size(); ++K)
		{
			RailFresh = RailFresh && Said.second[K] != Said.second[K - 1];
		}
		if (Said.first.second == "stake.rail")
		{
			std::set<std::string> Distinct(Said.second.begin(), Said.second.end());
			RailFresh = RailFresh && Distinct.size() == Said.second.size();
		}
	}
	Expect(RailLines >= 6 && RailFresh, "room shape: a backer at the rail never says a line twice running, nor a rail line twice in a night (" + std::to_string(RailLines) + " lines)");
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

void RoomNightly(bool WriteGolden)
{
	// A Tuesday Nightly, 96 runners. The player wears the Lucky Penny's patch; Mrs. Park holds 15% of them from the rail.
	const Setup S = MakeNight("embercrest-nightly@15", 96, "luckypenny", ":20");
	const Night N = Play(S, 9, 1.0f);
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
	std::printf("room nightly: %d hands, %d cues, the player %s, champion %s, liveroom at most %d bytes\n", N.Ticks, static_cast<int>(N.Cues.size()),
		N.HeroPlace == 1 ? "won it" : ("finished " + talk::Ordinal(N.HeroPlace)).c_str(), N.Winner.c_str(), static_cast<int>(N.MaxRoomText));
	Expect(N.Ticks > 60 && N.Cues.size() > 150, "room nightly: a whole night with plenty said");
	Expect(N.MaxRoomText <= 16 * 1024, "room nightly: the director's liveroom state stays under 16 KB (§10)");
	const double Plain = N.PlainMs / std::max(1, N.PlainTicks);
	std::printf("room nightly: the director takes %.4f ms a hand end (budget 0.05), %.4f with the night's %d equity runs averaged in, %.3f ms at most\n", Plain,
		N.DirectorMs / std::max(1, N.Ticks), static_cast<int>(N.EquityTables.size()), N.WorstMs);
	Expect(Plain < 0.15, "room nightly: the director stays cheap (a hand end well under 0.15 ms; the budget is 0.05)");

	// The PA's order: each milestone once, in the night's order.
	const std::vector<std::string> Order = {"pa.desk.open", "pa.ten.minutes", "pa.start", "pa.last.call", "pa.reg.closed", "pa.away", "pa.h4h", "pa.bubble",
		"pa.two.tables", "pa.final", "pa.headsup", "pa.champion"};
	std::map<std::string, int> FirstAt;
	std::map<std::string, int> Times;
	for (size_t I = 0; I < N.Cues.size(); ++I)
	{
		const room::Cue& C = N.Cues[I].second;
		if (C.What != room::Kind::Pa || C.Part != 0)
		{
			continue; // an announcement once, by its first part
		}
		// The template's pool ("pa.bubble.seat.0" is "pa.bubble.seat"); the bubble and the champion each have their variants.
		std::string Pool = C.Key.substr(0, C.Key.rfind('.'));
		if (Pool == "pa.bubble.seat")
		{
			Pool = "pa.bubble";
		}
		if (Pool == "pa.champion.seat" || Pool == "pa.champion.dee.hero")
		{
			Pool = "pa.champion";
		}
		for (const std::string& M : Order)
		{
			if (Pool == M)
			{
				if (!FirstAt.count(M))
				{
					FirstAt[M] = static_cast<int>(I);
				}
				++Times[M];
			}
		}
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

	// Honesty.
	std::set<std::string> TabledTonight;
	std::map<std::string, int> LastNeedle;
	int Needles = 0;
	int Stories = 0;
	int Beats = 0;
	int Calls = 0;
	int Noticed = 0;
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
			Expect(Prev2 == LastNeedle.end() || Tick - Prev2->second >= 4, "room honesty: one needle per needler per 4 hands (" + C.Speaker + ")");
			LastNeedle[C.Who] = Tick;
		}
		if (Starts(C.Key, "field.small.") || Starts(C.Key, "field.ask."))
		{
			Expect(F == N.Facts.end() || !F->second.HandForHandAtHand, "room honesty: hand for hand, reactions only (" + C.Key + ")");
		}
		if (Starts(C.Topic, "noticed:"))
		{
			++Noticed;
		}
	}
	Expect(Noticed <= 1, "room noticed: the patch is noticed at most once a night");
	std::printf("room honesty: %d all-in calls elsewhere, %d stories, %d bad beats, %d needles, %d noticed: all checked\n", Calls, Stories, Beats, Needles, Noticed);
	RoomShape(N);
	RoomCaptionFit({&N});

	// RoomEquityScope: equity only at the player's table, and only once an all-in was called and tabled.
	bool Scoped = true;
	for (const int Table : N.EquityTables)
	{
		Scoped = Scoped && N.HeroTables.count(Table) > 0;
	}
	Expect(Scoped, "room equity scope: no equity for a table other than the player's (or the feature table)");
	Expect(static_cast<int>(N.EquityTables.size()) == N.EquityHands, "room equity scope: once per called all-in at the player's table (" + std::to_string(N.EquityTables.size()) + " of " +
		std::to_string(N.EquityHands) + ")");

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
	const std::set<std::string> Milestones = {"pa.start", "pa.last.call", "pa.reg.closed", "pa.away", "pa.h4h", "pa.bubble", "pa.two.tables", "pa.final", "pa.intro",
		"pa.headsup", "pa.champion", "pa.desk.open", "pa.ten.minutes"};
	auto MilestoneOf = [&Milestones](const std::string& Key) -> std::string {
		for (const std::string& M : Milestones)
		{
			if (Key.rfind(M + ".", 0) == 0 && !(M == "pa.h4h" && (Key.rfind("pa.h4h.call", 0) == 0 || Key.rfind("pa.h4h.done", 0) == 0)))
			{
				return M;
			}
		}
		return std::string();
	};
	for (const int At : {Last / 3, Last / 2, 4 * Last / 5, Last - 1})
	{
		Saved Bare = N.Saves.at(At);
		Bare.Room.clear();
		const Night Rest = Play(S, 9, 1.0f, At, &Bare);
		std::set<std::string> Before;
		for (const auto& TC : N.Cues)
		{
			if (TC.first <= At && TC.second.What == room::Kind::Pa && !MilestoneOf(TC.second.Key).empty())
			{
				Before.insert(MilestoneOf(TC.second.Key));
			}
		}
		bool Repeats = false;
		for (const auto& TC : Rest.Cues)
		{
			const std::string M = TC.second.What == room::Kind::Pa ? MilestoneOf(TC.second.Key) : std::string();
			if (!M.empty() && Before.count(M))
			{
				Repeats = true;
				std::printf("room checkpoints: an empty-state resume after hand %d repeated %s\n", At, TC.second.Key.c_str());
			}
		}
		Expect(!Repeats, "room checkpoints: an empty-state resume after hand " + std::to_string(At) + " repeats no milestone");
	}
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

// ------------------------------------------------------------------ RoomTalkCopy

void RoomTalkCopy()
{
	const std::map<std::string, std::string> Extra = {
		{"Average", "41,000"}, {"Players", "114"}, {"Pool", "$15,390"}, {"MinCash", "$270"}, {"Sb", "12,500"}, {"Bb", "25,000"}, {"Ante", "25,000"}, {"Minutes", "15"},
		{"Stack", "30,000"}, {"Denoms", "purple five-hundreds and the yellow thousands"}, {"Away", "three"}, {"From", "14"}, {"SeatWord", "four"},
		{"Honor", "two-time Embercrest champion"}, {"A", "Jesse Cole"}, {"AChips", "1,036,000"}, {"B", "Maria G."}, {"BChips", "1,012,000"}, {"First", "$4,160"},
		{"Mine", "Ace-king suited"}, {"MineLower", "ace-king suited"}, {"Theirs", "Queen-jack suited"}, {"TheirsLower", "queen-jack suited"}, {"Street", "river"},
		{"Bubble", "Maria G."}, {"PlaceWord", "Seventeenth"}, {"PlaceLower", "seventeenth"}, {"EventSpoken", "Midnight Turbo"}, {"When", "Wednesday"},
		{"WhenCap", "Yesterday"}, {"Entries", "114"}, {"Prize", "$1,280"}, {"LevelsCap", "Two"}, {"Spoken", "Eighteen thousand four hundred."},
		{"Rough", "Two hundred-something"}, {"RoughLower", "two hundred-something"}, {"Event", "Saturday Big Stack"}, {"Short", "Midnight Turbo"},
		{"Name", "Danny Q."}, {"TableWord", "eighteen"}};
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
} // namespace room_test

namespace room_field
{
/** RoomField (the lead's, Phase A): tests/room_field.cpp. The failures it found. */
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
	room_test::RoomNightly(Write);
	room_test::RoomFinalTable();
	room_test::Failures += room_field::Run();
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Started).count();
	std::printf("room tests: %.0f ms\n", Ms);
	if (room_test::Failures == 0)
	{
		std::printf("room tests: all passed\n");
	}
	return room_test::Failures == 0 ? 0 : 1;
}
