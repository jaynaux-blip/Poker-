#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"

#include <algorithm>
#include <iterator>
#include <map>

// The Session's side of the people on the block (the realism update's spec, §7.2, §6.4): what was said and heard,
// Twitch's twenty, the tips. people-core; in its own translation unit so Session.cpp stays closed after Phase A.
//
// Heard applies a Talk as it's shown: the one-time key it spent (Life.Said, by the day: rumours told once, a remark made
// once; anything told more than 90 days ago is forgotten first, as the save forgets it), the familiarity it earned and the
// look the speaker saw (Life.Town, the block's people by id), and, for the living world's people, the meeting itself
// (world::MemoryKind::Street, once a day, kept as one memory for a run of them). The street plays in-process with its own
// Session, so the meeting goes straight into the world (saved with it) rather than through SaveData::WorldNotes: Dee's
// table reads the same bond. Two keys do more than mark: a rumour of the Garage or the Basement puts the game on the
// phone's calendar (Life.Unlocks "rumour:garage", "rumour:basement") with a text, and "repaid:twitch" is Twitch handing
// back the twenty (ledger kind 7, as transfers are). That one is an act, not a line to remember: it isn't kept, so the
// next twenty he borrows comes back the same way.

namespace ss
{
namespace session_town_detail
{
constexpr Chips TwitchAsks = 2000;
constexpr const char LoanMark[] = "loan:twitch:won:"; // + what he had won in his career when he borrowed it
constexpr size_t LoanMarkSize = sizeof(LoanMark) - 1;

bool Down(const world::Npc* N)
{
	return N && (N->St == world::Status::Broke || N->Mood == world::Momentum::Downswing || N->Mood == world::Momentum::MajorDownswing);
}

/**
 * Forgets what was said more than 90 days ago (town::SaidFresh), as the save does: a stale loan to Twitch goes with its
 * mark, and his bond stops saying he owes it.
 */
void Forget(life::State& L, int Today)
{
	for (auto It = L.Said.begin(); It != L.Said.end();)
	{
		It = town::SaidFresh(It->second, Today) ? std::next(It) : L.Said.erase(It);
	}
	const auto Twitch = L.Town.find("twitch");
	if (Twitch != L.Town.end() && L.Said.count("loan:twitch") == 0)
	{
		Twitch->second.Flags &= ~town::Lent;
	}
}

/** Where on the block a meeting happened, for the world's memory ("Ruby's"). */
std::string PlaceOf(const std::string& Venue)
{
	if (Venue == "diner")
	{
		return "Ruby's";
	}
	if (Venue == "washfold")
	{
		return "the Wash & Fold";
	}
	if (Venue == "market-stop")
	{
		return "the 14's stop";
	}
	if (const shop::Venue* V = shop::FindVenue(Venue))
	{
		return V->Name;
	}
	return "Fifth St.";
}
} // namespace session_town_detail

void Session::Heard(const std::string& Speaker, const town::Talk& Told) // (not "T": Session::T is the tournament)
{
	using namespace session_town_detail;
	const town::Person* P = town::FindPerson(Speaker);
	const bool Local = Speaker.rfind("npc:", 0) == 0;
	if ((!P && !Local) || (Told.Line.empty() && Told.SaidKey.empty()))
	{
		return;
	}
	const double World = WorldMinutes();
	const int Today = net::DayOf(World);
	Forget(Life, Today);
	const std::string Name = P ? P->Name : Speaker.substr(4);
	const int Npc = ((P && P->InWorld) || Local) && LivingWorld.Ready() ? LivingWorld.Find(Name) : -1;

	// The one-time line it spent.
	const std::string& Key = Told.SaidKey;
	if (Key == "repaid:twitch")
	{
		// The twenty back from his next cash: paid once per loan (the loan's own key goes with it), and not kept.
		if (Life.Said.count("loan:twitch") > 0)
		{
			BankrollCents += TwitchAsks;
			Life.Record(World, "Twitch paid you back", TwitchAsks, 7);
			Life.Said.erase("loan:twitch");
			for (auto It = Life.Said.lower_bound(LoanMark); It != Life.Said.end() && It->first.compare(0, LoanMarkSize, LoanMark) == 0;)
			{
				It = Life.Said.erase(It);
			}
			Life.Town["twitch"].Flags &= ~town::Lent;
			if (Npc >= 0)
			{
				LivingWorld.Lent(Npc, true, World);
			}
		}
		Life.Said.erase(Key);
	}
	else if (!Key.empty())
	{
		Life.Said[Key] = Today;
		if (Key == "rumour:garage" && Life.Unlocks.insert(Key).second)
		{
			StoryText(Key, "Calendar", "Added: The Garage \xC2\xB7 Thursdays, 10 PM \xC2\xB7 across the river \xC2\xB7 $2/$5 no-limit. Rick plays. (Heard it from " + Name + ".)");
		}
		if (Key == "rumour:basement" && Life.Unlocks.insert(Key).second)
		{
			StoryText(Key, "Calendar", "Added: The Basement \xC2\xB7 Saturdays, 11 PM \xC2\xB7 downtown \xC2\xB7 $10/$20 no-limit. (Heard it from " + Name + ".)");
		}
	}

	// The block's bond: familiarity, the day, how the player looked.
	if (P)
	{
		town::Bond& B = Life.Town[Speaker];
		B.Familiarity = std::min(100, B.Familiarity + std::max(0, Told.Familiarity));
		if (Told.Familiarity > 0 && B.LastDay != Today)
		{
			++B.Visits;
			B.LastDay = Today;
		}
		B.LookSeen = town::LookHash(Person.Appearance, Life.Closet);
		if (Speaker == "augie")
		{
			B.Flags = Life.Pawn.Tickets.empty() ? (B.Flags & ~town::Owed) : (B.Flags | town::Owed);
		}
	}

	// The world's memory of meeting them on the block.
	if (Npc >= 0 && Told.Familiarity > 0)
	{
		town::Placement Here;
		const std::string Where = town::Where(Speaker, World, *this, Here) ? PlaceOf(Here.Venue) : std::string("Fifth St.");
		LivingWorld.Remember(Npc, world::MemoryKind::Street, Where, 0, World);
	}
}

std::string Session::Lend(const std::string& Speaker, Chips Cents)
{
	using namespace session_town_detail;
	if (Speaker != "twitch")
	{
		return "Nobody asked.";
	}
	Forget(Life, net::DayOf(WorldMinutes()));
	if (Life.Said.count("loan:twitch") > 0)
	{
		return "He still owes you the last twenty.";
	}
	const int Npc = LivingWorld.Ready() ? LivingWorld.Find("Twitch") : -1;
	if (!Down(LivingWorld.Get(Npc)))
	{
		return "He's not asking. He's flush this week.";
	}
	if (Cents != TwitchAsks)
	{
		return "He asked for twenty.";
	}
	if (TimeSkip.Active)
	{
		return "Not now.";
	}
	if (BankrollCents < Cents)
	{
		return "You don't have twenty to lend.";
	}
	const double World = WorldMinutes();
	BankrollCents -= Cents;
	Life.Record(World, "Lent Twitch $20", -Cents, 7);
	Life.Said["loan:twitch"] = net::DayOf(World);
	// What he'd won in his career when he borrowed it: his next cash is anything past it (town::ContextFor reads it).
	Life.Said[std::string(LoanMark) + std::to_string(LivingWorld.Get(Npc)->CareerWon())] = net::DayOf(World);
	Life.Town["twitch"].Flags |= town::Lent;
	LivingWorld.Lent(Npc, false, World);
	return "";
}

void Session::Tipped(const std::string& Speaker, int Percent)
{
	if (!town::FindPerson(Speaker))
	{
		return;
	}
	const int Today = net::DayOf(WorldMinutes());
	town::Bond& B = Life.Town[Speaker];
	if (Percent <= 0)
	{
		B.Flags = (B.Flags | town::Stiffed) & ~town::Tipper; // remembered (Marlene: "Mm-hm. Come again.")
	}
	else if (Percent >= 15)
	{
		B.Flags = (B.Flags | town::Tipper) & ~town::Stiffed;
	}
	B.Familiarity = std::min(100, B.Familiarity + 1);
	if (B.LastDay != Today)
	{
		++B.Visits;
		B.LastDay = Today;
	}
	B.LookSeen = town::LookHash(Person.Appearance, Life.Closet);
}
} // namespace ss
