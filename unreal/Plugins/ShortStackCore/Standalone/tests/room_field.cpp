// card_room's RoomField (next_segment_spec §1.8, §7.6, §8.2): live::DescribeField gives the notes both of the old
// copies of the field code gave. The lead's (Phase A), compiled into room_test and called from room_main.cpp.
//
// The two old paths are kept here word for word as they were at c09bc31 (the session's registration, Session.cpp:
// 3517-3607 with its StyleOf at :916-934; the desk's worker, BackRoomLive.cpp:193-300 with its PlayStyle at :125-142,
// ported from FString to std::string), each run on its own copy of one world, at the moment of registration, for every
// event of a week and a half. The three must agree on the planned field, the roster (names and how they play), the
// faces (name, label, line) and the champions' board.
//
// One deliberate difference (the segment's ruling, next_segment_spec §1.9 #22 and rule C4): the old paths took
// "Embercrest regular" from the world's habit list (EmbercrestRegulars); DescribeField takes it from the Players Club
// tier, 1 or better from the room record. So the faces are compared with the old paths' faces under that ruling
// (RuledFaces): whoever knows the player keeps their face word for word, and the rest of the field shows as a regular
// exactly when they hold a club card.
#include "ShortStack/Game/Badges.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace room_field
{
namespace life = ss::life;
namespace live = ss::live;
namespace world = ss::world;
namespace net = ss::net;
using ss::Archetype;

struct Notes
{
	int Planned = 0;
	std::vector<std::pair<std::string, int>> Roster;
	std::vector<life::LiveFace> Faces;
	std::vector<std::string> Board;
};

// ---- Session.cpp at c09bc31 (session_detail::StyleOf, and GoToLive's field, faces and board)

Archetype OldStyleOf(const world::Npc& N)
{
	const float Sk = N.Overall();
	const float Aggro = N.TraitOf(world::Trait::Aggro);
	const float Risk = N.TraitOf(world::Trait::Risk);
	if (Sk >= 0.72f)
	{
		return Aggro > 0.6f ? Archetype::Crusher : Archetype::Reg;
	}
	if (Sk >= 0.58f)
	{
		return Aggro > 0.62f ? Archetype::Lag : Aggro < 0.35f ? Archetype::Tag : Archetype::Reg;
	}
	if (Sk >= 0.45f)
	{
		return Aggro > 0.8f && Risk > 0.7f ? Archetype::Maniac : Aggro < 0.3f ? Archetype::Nit : Archetype::Tag;
	}
	return Aggro > 0.8f ? Archetype::Maniac : Aggro < 0.3f ? Archetype::Station : N.TraitOf(world::Trait::Patience) > 0.7f ? Archetype::Nit : Archetype::Fish;
}

/** GoToLive's: the roster, then (after live::Register) the world told, then the faces and the board. */
Notes OldSession(world::World& LivingWorld, const live::Occurrence& O, const std::string& HeroName, double World)
{
	Notes Out;
	std::vector<std::pair<std::string, int>> Roster;
	int Planned = O.Field;
	if (LivingWorld.Ready())
	{
		Planned = std::max(1, LivingWorld.PlannedEntries(O.Id));
		for (int Npc : LivingWorld.Registered(O.Id))
		{
			const world::Npc* N = LivingWorld.Get(Npc);
			if (!N || N->Name == HeroName)
			{
				continue;
			}
			int Type = static_cast<int>(OldStyleOf(*N));
			for (const live::CastMember& Cm : live::EmbercrestCast())
			{
				if (N->Name == Cm.Name)
				{
					Type = static_cast<int>(Cm.Type);
				}
			}
			Roster.push_back({N->Name, Type});
		}
	}
	Out.Planned = Planned;
	Out.Roster = Roster;
	// (live::Register: the money and the entry.)
	LivingWorld.HeroEntered(O.Id, O.T->Name);
	if (LivingWorld.Ready())
	{
		const std::vector<int> Regulars = LivingWorld.EmbercrestRegulars();
		for (int Npc : LivingWorld.Registered(O.Id))
		{
			const world::Npc* N = LivingWorld.Get(Npc);
			if (!N || N->Name == HeroName)
			{
				continue;
			}
			const world::Bond* Bd = LivingWorld.BondWith(Npc);
			const bool Regular = std::find(Regulars.begin(), Regulars.end(), Npc) != Regulars.end();
			const std::string Known = Bd ? Bd->Label() : std::string();
			if (!Regular && Known.empty() && !(Bd && !Bd->Memories.empty()))
			{
				continue;
			}
			life::LiveFace F;
			F.Name = N->Name;
			F.Label = !Known.empty() ? Known : Bd && !Bd->Memories.empty() ? std::string("Has met you") : std::string("Embercrest regular");
			F.Line = LivingWorld.SpokenGreeting(Npc, live::FaceHash(O.Id + N->Name));
			Out.Faces.push_back(F);
		}
	}
	static const char* Days[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
	for (int Day = O.Day; Day >= O.Day - 10 && Out.Board.size() < 8; --Day)
	{
		const std::vector<live::Occurrence> Os = live::Occurrences(Day);
		for (auto It = Os.rbegin(); It != Os.rend() && Out.Board.size() < 8; ++It)
		{
			const net::EventResult* Res = It->Start < World ? LivingWorld.ResultOf(It->Id) : nullptr;
			if (!Res)
			{
				continue;
			}
			for (const net::Placing& Pl : Res->FinalTable)
			{
				if (Pl.Place != 1)
				{
					continue;
				}
				const world::Npc* N = Pl.Player >= 0 ? LivingWorld.Get(Pl.Player) : nullptr;
				const std::string Who = Pl.Player == -1 ? HeroName : N ? N->Name : Pl.Name;
				std::string Short = It->T->Short;
				for (char& Ch : Short)
				{
					Ch = static_cast<char>(std::toupper(static_cast<unsigned char>(Ch)));
				}
				Out.Board.push_back(std::string(Days[((Day % 7) + 7) % 7]) + " " + Short + "\t" + Who + "\t" + std::to_string(Pl.Prize));
			}
		}
	}
	return Out;
}

// ---- BackRoomLive.cpp at c09bc31 (LiveDetail::PlayStyle, and FLiveDeskSignup::Run once the world is read)

Archetype DeskPlayStyle(const world::Npc& N)
{
	const float Sk = N.Overall();
	const float Aggro = N.TraitOf(world::Trait::Aggro);
	const float Risk = N.TraitOf(world::Trait::Risk);
	if (Sk >= 0.72f)
	{
		return Aggro > 0.6f ? Archetype::Crusher : Archetype::Reg;
	}
	if (Sk >= 0.58f)
	{
		return Aggro > 0.62f ? Archetype::Lag : Aggro < 0.35f ? Archetype::Tag : Archetype::Reg;
	}
	if (Sk >= 0.45f)
	{
		return Aggro > 0.8f && Risk > 0.7f ? Archetype::Maniac : Aggro < 0.3f ? Archetype::Nit : Archetype::Tag;
	}
	return Aggro > 0.8f ? Archetype::Maniac : Aggro < 0.3f ? Archetype::Station : N.TraitOf(world::Trait::Patience) > 0.7f ? Archetype::Nit : Archetype::Fish;
}

/** The desk's worker: the roster and the faces in one pass, the board, then the world told. Without a world, the room's crowd. */
Notes OldDesk(world::World& W, const live::Occurrence& O, const std::string& HeroName, double At)
{
	Notes Out;
	Out.Planned = O.Field;
	if (W.Ready() && O.Valid())
	{
		Out.Planned = std::max(1, W.PlannedEntries(O.Id));
		const std::vector<int> Regulars = W.EmbercrestRegulars();
		for (int Npc : W.Registered(O.Id))
		{
			const world::Npc* N = W.Get(Npc);
			if (!N || N->Name == HeroName)
			{
				continue;
			}
			int Type = static_cast<int>(DeskPlayStyle(*N));
			for (const live::CastMember& Cm : live::EmbercrestCast())
			{
				if (N->Name == Cm.Name)
				{
					Type = static_cast<int>(Cm.Type);
				}
			}
			Out.Roster.push_back({N->Name, Type});
			const world::Bond* Bd = W.BondWith(Npc);
			const bool bRegular = std::find(Regulars.begin(), Regulars.end(), Npc) != Regulars.end();
			const std::string Known = Bd ? Bd->Label() : std::string();
			if (!bRegular && Known.empty() && !(Bd && !Bd->Memories.empty()))
			{
				continue;
			}
			life::LiveFace F;
			F.Name = N->Name;
			F.Label = !Known.empty() ? Known : Bd && !Bd->Memories.empty() ? std::string("Has met you") : std::string("Embercrest regular");
			F.Line = W.SpokenGreeting(Npc, live::FaceHash(O.Id + N->Name));
			Out.Faces.push_back(F);
		}
		static const char* Days[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
		for (int Day = O.Day; Day >= O.Day - 10 && Out.Board.size() < 8; --Day)
		{
			const std::vector<live::Occurrence> Os = live::Occurrences(Day);
			for (auto It = Os.rbegin(); It != Os.rend() && Out.Board.size() < 8; ++It)
			{
				const net::EventResult* Res = It->Start < At ? W.ResultOf(It->Id) : nullptr;
				if (!Res)
				{
					continue;
				}
				for (const net::Placing& Pl : Res->FinalTable)
				{
					if (Pl.Place != 1)
					{
						continue;
					}
					const world::Npc* N = Pl.Player >= 0 ? W.Get(Pl.Player) : nullptr;
					const std::string Who = Pl.Player == -1 ? HeroName : N ? N->Name : Pl.Name;
					// FString(It->T->Short).ToUpper(): the shorts are ASCII.
					std::string Short = It->T->Short;
					for (char& Ch : Short)
					{
						Ch = static_cast<char>(std::toupper(static_cast<unsigned char>(Ch)));
					}
					Out.Board.push_back(std::string(Days[((Day % 7) + 7) % 7]) + " " + Short + "\t" + Who + "\t" + std::to_string(Pl.Prize));
				}
			}
		}
		W.HeroEntered(O.Id, O.T->Name);
	}
	return Out;
}

bool SameFaces(const std::vector<life::LiveFace>& A, const std::vector<life::LiveFace>& B)
{
	if (A.size() != B.size())
	{
		return false;
	}
	for (size_t I = 0; I < A.size(); ++I)
	{
		if (A[I].Name != B[I].Name || A[I].Label != B[I].Label || A[I].Line != B[I].Line)
		{
			return false;
		}
	}
	return true;
}

bool Same(const Notes& A, const live::FieldNotes& B)
{
	return A.Planned == B.Planned && A.Roster == B.Roster && SameFaces(A.Faces, B.Faces) && A.Board == B.Board;
}

/** Everything but the faces: the planned field, the roster and the board. */
bool SameField(const Notes& A, const live::FieldNotes& B)
{
	return A.Planned == B.Planned && A.Roster == B.Roster && A.Board == B.Board;
}

/** A Players Club member on Day (tier 1 or better, the room record's upgrade-at-once, drop-on-the-1st rule). */
bool Member(const world::Npc& N, int Day)
{
	return N.Room.Venue == "embercrest" && ss::badge::ClubTier(N.Room.VisitsIn(Day), N.Room.VisitsAtMonthStart(Day)) >= 1;
}

/**
 * An old path's faces under §1.9 #22, read from W as it stood at registration: in the field's order, everyone who knows
 * the player keeps the face the old path gave them (label and line), and the rest show as "Embercrest regular", with the
 * same spoken greeting, exactly when they hold a club card. Dropped and Added count the faces the ruling took from the
 * habit list's regulars and gave to club members off it.
 */
std::vector<life::LiveFace> RuledFaces(world::World W, const live::Occurrence& O, const std::string& HeroName, const std::vector<life::LiveFace>& Old, double At, int& Dropped,
	int& Added)
{
	const int Day = net::DayOf(At);
	const std::vector<int> Habit = W.EmbercrestRegulars();
	std::vector<life::LiveFace> Out;
	for (int Npc : W.Registered(O.Id))
	{
		const world::Npc* N = W.Get(Npc);
		if (!N || N->Name == HeroName)
		{
			continue;
		}
		const auto Was = std::find_if(Old.begin(), Old.end(), [N](const life::LiveFace& F) { return F.Name == N->Name; });
		if (Was != Old.end() && Was->Label != "Embercrest regular")
		{
			Out.push_back(*Was);
			continue;
		}
		const bool OnHabit = std::find(Habit.begin(), Habit.end(), Npc) != Habit.end();
		if (Member(*N, Day))
		{
			Added += OnHabit ? 0 : 1;
			life::LiveFace F;
			F.Name = N->Name;
			F.Label = "Embercrest regular";
			F.Line = W.SpokenGreeting(Npc, live::FaceHash(O.Id + N->Name));
			Out.push_back(F);
		}
		else
		{
			Dropped += Was != Old.end() ? 1 : 0;
		}
	}
	return Out;
}

bool Same(const Notes& A, const Notes& B)
{
	return A.Planned == B.Planned && A.Roster == B.Roster && SameFaces(A.Faces, B.Faces) && A.Board == B.Board;
}

int Run()
{
	int Failures = 0;
	auto Expect = [&Failures](bool Condition, const std::string& What) {
		if (!Condition)
		{
			++Failures;
			std::printf("FAILED: %s\n", What.c_str());
		}
	};
	const std::string Hero = "grinder_3c";
	const double Day = net::MinutesPerDay;
	int Events = 0;
	int Faces = 0;
	int Known = 0;
	int Rows = 0;
	int Named = 0;
	int Dropped = 0; // habit-list regulars without a club card: no longer faces
	int Added = 0;   // club members off the habit list: now faces
	int HeroRows = 0;
	for (const uint32_t Seed : {0x5eed0001u, 0x5eed0002u})
	{
		world::World W;
		W.Create(Seed, Day + 127.0);
		W.HeroName = Hero;
		// Some history with the player, so the faces aren't only the room's regulars: a night at Dee's game, and a
		// Nightly where the player sat with people, beat one and lost a big pot to another.
		W.AdvanceTo(2.0 * Day + 21.0 * 60.0);
		W.BackRoomNight(2.0 * Day + 21.0 * 60.0, {"Sal", "Big Lou", "Twitch", "Mei"}, 4200);
		const live::Occurrence First = live::FindOccurrence("embercrest-nightly@3");
		W.AdvanceTo(First.DeskOpens + 30.0);
		const std::vector<int> In = W.Registered(First.Id);
		W.HeroEntered(First.Id, First.T->Name);
		world::World::TableReport Report;
		for (size_t K = 0; K < In.size() && K < 12; ++K)
		{
			Report.Met.push_back(In[K]);
		}
		if (In.size() > 3)
		{
			Report.HeroKnockedOut.push_back(In[1]);
			Report.BigPots.push_back({In[2], -40000});
		}
		W.AdvanceTo(First.Start + 4.0 * 60.0);
		W.HeroFinished(First.Id, 9, 0, Report);
		// Every event of the next ten days, at the desk half an hour after it opens.
		for (int D = 4; D <= 13; ++D)
		{
			for (const live::Occurrence& O : live::Occurrences(D))
			{
				const double At = std::min(O.DeskOpens + 30.0, O.LateRegEnds - 30.0);
				if (At < W.Clock())
				{
					continue;
				}
				W.AdvanceTo(At);
				world::World ForSession = W;
				world::World ForDesk = W;
				world::World ForCore = W;
				const Notes Session = OldSession(ForSession, O, Hero, At);
				const Notes Desk = OldDesk(ForDesk, O, Hero, At);
				// The player's save: six Embercrest nights in the last month, a club card (Regular), so they have a badge row.
				life::State Player;
				for (int Back = 6; Back >= 1; --Back)
				{
					life::LiveDay Ld;
					Ld.Day = net::DayOf(At) - 4 * Back;
					Ld.Events = 1;
					Ld.Embercrest = 1;
					Player.LiveDays.push_back(Ld);
				}
				const live::FieldNotes Core = live::DescribeField(ForCore, O, Hero, Player, At);
				ForCore.HeroEntered(O.Id, O.T->Name);
				int D0 = 0;
				int A0 = 0;
				const std::vector<life::LiveFace> SessionRuled = RuledFaces(W, O, Hero, Session.Faces, At, Dropped, Added);
				const std::vector<life::LiveFace> DeskRuled = RuledFaces(W, O, Hero, Desk.Faces, At, D0, A0);
				Expect(Same(Session, Desk), "room field: the session's and the desk's old paths agree for " + O.Id);
				Expect(SameField(Session, Core) && SameFaces(SessionRuled, Core.Faces),
					"room field: DescribeField gives the session's notes for " + O.Id + " (its regulars from the club tier, §1.9 #22)");
				Expect(SameField(Desk, Core) && SameFaces(DeskRuled, Core.Faces), "room field: DescribeField gives the desk's notes for " + O.Id + " (its regulars from the club tier)");
				// The player's own badge row: last, under their name (not in the roster), their club card from the save; their
				// row on the month's leaders, if they're on it, shows the same.
				const std::string Mine = ss::badge::HeroFieldPack(W, Player, At);
				const ss::badge::Set MineSet = ss::badge::Unpack(Mine);
				const ss::badge::Badge* Club = MineSet.Find(ss::badge::Kind::Club);
				bool LeadersMine = true;
				for (const life::RoomLeader& L : Core.Leaders)
				{
					LeadersMine = LeadersMine && (L.Name != Hero || L.Badge == Mine);
				}
				Expect(!Core.Badges.empty() && Core.Badges.back().first == Hero && Core.Badges.back().second == Mine && Club && Club->Tier == 1 &&
						   std::none_of(Core.Roster.begin(), Core.Roster.end(), [&Hero](const std::pair<std::string, int>& R) { return R.first == Hero; }) && LeadersMine,
					"room field: the player's own badge row (their club card from the save) under their name, and on the leaders, for " + O.Id);
				HeroRows += std::any_of(Core.Leaders.begin(), Core.Leaders.end(), [&Hero](const life::RoomLeader& L) { return L.Name == Hero; }) ? 1 : 0;
				// And Apply puts them on an entry as the session did, with the features' notes exactly as DescribeField gave
				// them (voices, badges, patches, the month's leaders).
				life::LiveEntry E;
				live::Apply(Core, E);
				bool Copied = E.Voices.size() == Core.Voices.size() && E.Badges == Core.Badges && E.Patches == Core.Patches && E.Leaders.size() == Core.Leaders.size();
				for (size_t K = 0; Copied && K < E.Voices.size(); ++K)
				{
					Copied = E.Voices[K].Name == Core.Voices[K].Name && E.Voices[K].Style == Core.Voices[K].Style;
				}
				for (size_t K = 0; Copied && K < E.Leaders.size(); ++K)
				{
					Copied = E.Leaders[K].Rank == Core.Leaders[K].Rank && E.Leaders[K].Name == Core.Leaders[K].Name && E.Leaders[K].Badge == Core.Leaders[K].Badge;
				}
				Expect(SameFaces(E.Faces, SessionRuled) && E.Board == Session.Board && Copied, "room field: Apply writes the faces, the board and the features' notes for " + O.Id);
				++Events;
				Faces += static_cast<int>(Core.Faces.size());
				Rows += static_cast<int>(Core.Board.size());
				Named += static_cast<int>(Core.Roster.size());
				for (const life::LiveFace& F : Core.Faces)
				{
					Known += F.Label != "Embercrest regular" ? 1 : 0;
				}
			}
		}
		// A world not yet made: the room's own crowd, nobody named, no faces, no board.
		{
			world::World Empty;
			const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
			world::World EmptyDesk = Empty;
			const Notes Session = OldSession(Empty, O, Hero, O.DeskOpens + 30.0);
			const Notes Desk = OldDesk(EmptyDesk, O, Hero, O.DeskOpens + 30.0);
			world::World EmptyCore;
			const live::FieldNotes Core = live::DescribeField(EmptyCore, O, Hero, life::State(), O.DeskOpens + 30.0);
			Expect(Same(Session, Desk) && Same(Session, Core) && Core.Planned == O.Field && Core.Roster.empty() && Core.Faces.empty() && Core.Board.empty(),
				"room field: without a world, the room's own crowd from all three");
		}
	}
	Expect(Events >= 40 && Faces > 0 && Known > 0 && Rows > 0 && Named > 0, "room field: the check saw fields, faces (some who know the player) and boards");
	// The ruling must have mattered somewhere, or the face check couldn't tell the club tier from the habit list.
	Expect(Dropped + Added > 0, "room field: the club tier and the habit list disagree on someone, so the regulars' check means something");
	std::printf("room field: %d events, %d named entrants, %d faces (%d who know the player), %d board rows, %d leader rows for the player; by the club tier %d regulars added, %d dropped: "
				"the session's, the desk's and DescribeField's notes %s\n",
		Events, Named, Faces, Known, Rows, HeroRows, Added, Dropped, Failures == 0 ? "agree" : "DIFFER");
	return Failures;
}
} // namespace room_field
