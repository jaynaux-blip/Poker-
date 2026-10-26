// The save fixtures of next_segment_spec §7.5, and the transcript that proves Phase A's seams changed nothing.
//
// Builds representative careers through the session, the way the game does (the host's side of a live night done as
// session_test's EmbercrestRail does it: settle on the save, note the night, load it again):
//   - pre_segment_fresh: a new career, saved;
//   - pre_segment_mid_nightly: the middle of a month, rent paid, a shift, the corner store, a Penny Drop order, three
//     nights at the Embercrest (one played out by the engine, one won, one called off and refunded) and tonight's
//     Nightly registered, the player in the room and the night checkpointed (no voices, no room);
//   - pre_segment_team_riverline: a streamer with Kast's own deals: an active riverline deal (accepted through
//     AcceptDeal), a tunnelrat offer, a declined sitwell (through DeclineDeal);
//   - pre_segment_evicted: locked out, on Dee's couch, the gear in storage;
//   - pre_segment_v1: the mid-month save as a v1 ("riverside") save; and world_v1.txt, the mid-month save's world.
// It uses only what the session had at c09bc31, so the same file builds against that tree and this one.
//
// Update 2's day-0 fixtures (update2 SPEC §6.5, §7.2), the first committed saves holding the last segment's lines. Built
// the same way, with only what the session had at bda07f3 (so the file builds against bda07f3's tree and Phase A's):
//   - pre_update2_fresh: a new career, saved;
//   - pre_update2_staked_mid_nightly: a shift, the store, rent paid, a night at the Embercrest played out, then tonight's
//     Nightly on Big Lou's money (livefund), the player in the room forty hands in, checkpointed with the field's voices,
//     badges and leaders and the room director's state (liveroom);
//   - pre_update2_sponsored_streamer: a streamer with a sponsor's book (a contract signed, an offer open, Mail), a patch
//     sewn, Kast's clips and stream log, and the coach's notes from a hyper played on stream;
//   - pre_update2_evicted: locked out, on Dee's couch, the gear in storage;
//   - pre_update2_veteran: 300 and more results online, live days, titles on the wall and badges;
//   - world_v2.txt: the staked save's world (version 2).
//
// Update 2's gate 1 fixtures (update2 SPEC §6.5, §7.5 C1; for PIE §8.1), built with gate 1's code (only on a tree that
// has autoplay: the block below is compiled when Game/LiveAuto.h is there, so this file still builds against bda07f3's):
//   - update2_blank_world: a fresh-start (Blank) career four days in, WITH its world lines (an imported save without them
//     would build an Established world): hypers the plan skipped (one whole, one with the bubble taken by hand), one
//     played by hand, a shift, Thursday's Nightly skipped by the plan through live::SkipSlice and settled;
//   - update2_auto_mid_nightly: tonight's Nightly (Wednesday October 14) on the player's own money, the plan (GTO)
//     skipping to the bubble and the night checkpointed mid-skip the way the card room saves it every 5 hands (R5): the
//     engine checkpoint, the room director's state and the host text with its hands, met, auto, autoseen and autoread
//     lines (FAutoHost's, BackRoomAutoRules::AutoHostText's format). Without its world lines, like pre_update2_*.
//
// Update 2's gate 2 fixture (update2 SPEC §6.5, §7.5 C2; for PIE §8.2), built with gate 2's code (compiled when the tree has
// gate 1's autoplay and Game/Video.h):
//   - update2_creator: an Established career that streams its hypers on Kast and cuts a Rerun video from its own hands
//     every evening, from Night One until the week after Rerun's first payout: the Wash & Fold when the roll needs it, a
//     hyper every afternoon on stream (sprinted one day, skipped by the plan the next, the last one played by hand), the
//     stream kit, two Friday Nightlies at the Embercrest skipped by the plan with their hands noted the card room's way
//     (video::NoteLiveHand, NoteLiveRecap). Studio's picks rotate the four formats, the five title styles and the five
//     frames; a Teaching Breakdown of a strategy hand is evergreen. So the channel holds the 60 newest videos whole, the
//     older ones folded into its tails, evergreen stubs, the partner gate crossed (the review, the welcome), a payout paid
//     (its key and its bank line), a balance accruing, and open candidates: the plan's (AUTO), the player's own from the
//     last hyper, the Embercrest's, and a hand at Dee's table on the last night (storytime). Without its world lines, like
//     pre_update2_*. Its numbers follow Rerun's model, so a change there (the Shorts' FormatSubs, say) means writing it again.
//     (Gate 3's world moves who plays the Embercrest's nights: on a gate 3 tree write-g2 writes another creator, Ignite's
//     letter in Mail from day 151 but its last Nightly leaving no candidate open, which Gate2Fixtures needs. The committed
//     file stays C2's, update 2 C3; a gate 2 save in a gate 3 build is MigrationAll2's case.)
//
// Update 2's gate 3 fixture (update2 SPEC §6.5, §7.5 C3; for PIE §8.3), built with gate 3's code (compiled when the tree has
// gate 2's and the rival sites' money rules):
//   - update2_multisite: an Established career that grinds RiverLine's Hyper Sprints five an afternoon from Night One (two
//     rents paid from the bank, the Rewards rake counting), signs Team RiverLine under the "plays only on RiverLine" clause
//     (the offer made as ss.Sponsor.Offer makes it), and once Act I is over takes Ignite's invitation as its gate sends it
//     (Mail's Accept) and RunItTwice's as ss.Site.Invite sends it (its own door, 300 events at an $11 average, is months
//     away): $50 by card on each (the fees, "We noticed", Ignite's pending bonus, RunItTwice's tickets), the Supper Stack on
//     a RunItTwice ticket and the Spark Jackpot on Ignite's wallet the same evening (one strike under the clause, its cure
//     date in Mail), and the next morning $40 out of Ignite: saved that afternoon with the withdrawal in transit. Without
//     its world lines, like pre_update2_*.
//
// The creator update's Kast fixture (SPEC_CREATORS §8.2, §9.1; for PIE §9.4 and C-K's 165 KB check), built with the creator
// update's code (compiled when the tree has Game/KastGrowth.h) and written only by a build whose Kast runs the new model (a
// new career's first stream sets life::U3Kast; until core-kastglue's KastAfterStart does, write-g3k writes nothing):
//   - update3_streamer: an Established career that streams five evenings a week on its posted schedule (Monday to Friday,
//     7 PM), three world hours a night around an evening hyper it sprints, from Night One until its community reaches the
//     member cap (1,500) or two years have passed, the Wash & Fold when the roll needs it. Without its world lines, like
//     pre_update2_*.
// And kast-worst, the synthetic worst save Kast's first budget (LineBytesKast <= 180 KB) is measured on: a save two years
// in with every Kast list at its bound (1,500 members, 60 log rows, 300 subs, 36 goodwill names, 16 clips, 60 days, 6 mods)
// and every field at its widest; it prints today's Kast lines, what this build writes once U3Kast is set, and the new lines
// projected in §8.1's formats while their writers are stubs. Nothing is written.
//
//   fixture_tool write <dir>   writes the fixtures (the saves without their world lines, which go in world_v1.txt;
//                              <name>.expected.ssave is what loading and saving the v1 save gives)
//   fixture_tool write2 <dir>  writes update 2's day-0 fixtures (the saves without their world lines; world_v2.txt)
//   fixture_tool write-g1 <dir>  writes update 2's gate 1 fixtures (deterministic: two writes are byte for byte the same)
//   fixture_tool write-g2 <dir> [show]  writes update 2's gate 2 fixture (deterministic too; "show" prints the channel as it
//                              grows, every ten days, and the save's video lines)
//   fixture_tool write-g3 <dir> [show]  writes update 2's gate 3 fixture (deterministic too; "show" prints the career's days
//                              and the site book as it goes)
//   fixture_tool write-g3k <dir> [show]  writes the creator update's Kast fixture, on a build with the new model only
//                              ("show" prints the channel every ten days)
//   fixture_tool g3k-dry <days>  builds update3_streamer's career for that many days on whatever Kast this build has and
//                              prints the channel every ten days and its Kast lines' bytes; writes nothing (the lead's check
//                              of the career before the model lands)
//   fixture_tool kast-worst    measures the synthetic worst save's Kast lines (above); writes nothing
//   fixture_tool transcript    prints every save as the careers are built and as each fixture is loaded and played on
//                              (texts, ledgers, the whole save but the world, whose size and hash are printed), the
//                              c09bc31 careers first, then update 2's; a build whose transcript matches the code the
//                              fixtures came from behaves the same
// Writes and reads in binary: the saves are LF, whatever the platform.
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Room.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/Store.h"

#if __has_include("ShortStack/Game/LiveAuto.h")
#define SS_FIXTURE_GATE1 1
#include "ShortStack/AI/Reads.h"
#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/LiveAuto.h"
#include "ShortStack/Game/World.h"
#else
#define SS_FIXTURE_GATE1 0
#endif

#if SS_FIXTURE_GATE1 && __has_include("ShortStack/Game/Video.h")
#define SS_FIXTURE_GATE2 1
#include "ShortStack/Cards.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Recall.h"
#include "ShortStack/Game/TableTalk.h"
#include "ShortStack/Game/Video.h"
#include "ShortStack/Hand.h"
#include "ShortStack/Rng.h"
#else
#define SS_FIXTURE_GATE2 0
#endif

// Gate 3's rival sites (their money rules came with B3's private SitesDetail.h; Sites.h's declarations date from Phase A).
#if SS_FIXTURE_GATE2 && __has_include("../../Source/ShortStackCore/Private/ShortStack/Game/SitesDetail.h")
#define SS_FIXTURE_GATE3 1
#include "ShortStack/Game/Sites.h"
#else
#define SS_FIXTURE_GATE3 0
#endif

// The creator update's Kast (SPEC_CREATORS §8.2, §9.1; its declarations date from Phase A-K's KastGrowth.h).
#if SS_FIXTURE_GATE3 && __has_include("ShortStack/Game/KastGrowth.h")
#define SS_FIXTURE_G3K 1
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/KastGrowth.h"
#else
#define SS_FIXTURE_G3K 0
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace fixture_tool
{
namespace life = ss::life;
namespace live = ss::live;
const double Day = 1440.0;
bool Transcript = false;

struct Hooks : ss::SessionHooks
{
	ss::SaveData Last;
	int Saves = 0;
	void Text(const std::string& From, const std::string& Body) override
	{
		if (Transcript)
		{
			std::printf("  [phone] %s: %s\n", From.c_str(), Body.c_str());
		}
	}
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
	bool GoOut(const std::string&, ss::Chips) override { return true; }
};

/** The save's text without the world's lines (they go in world_v1.txt), and the world's. */
std::string WithoutWorld(const std::string& Text, std::string* World = nullptr)
{
	std::string Out;
	size_t Start = 0;
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		const std::string Line = Text.substr(Start, End - Start);
		if (Line.rfind("world\t", 0) == 0)
		{
			if (World)
			{
				*World += Line;
			}
		}
		else
		{
			Out += Line;
		}
		Start = End;
	}
	return Out;
}

uint32_t Hash(const std::string& S)
{
	uint32_t H = 2166136261u;
	for (const char Ch : S)
	{
		H = (H ^ static_cast<unsigned char>(Ch)) * 16777619u;
	}
	return H;
}

void Dump(const char* Tag, const ss::SaveData& D)
{
	if (!Transcript)
	{
		return;
	}
	std::string World;
	const std::string Text = WithoutWorld(D.Serialize(), &World);
	std::printf("==== %s: %zu bytes, the world %zu bytes (%08x)\n%s", Tag, Text.size(), World.size(), Hash(World), Text.c_str());
}

ss::hero::Character Someone()
{
	ss::hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Story = ss::hero::Background::DealersKid;
	C.Created = true;
	return C;
}

double Wait(ss::Session& S, double Now, double Seconds)
{
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}

/** The clock to World (the calendar runs over the gap), then a moment. */
double At(ss::Session& S, double Now, double World)
{
	S.LobbyMinutes += World - S.WorldMinutes();
	return Wait(S, Now, 0.1);
}

/** Walks the clock on a minute at a time to World. */
double Walk(ss::Session& S, double Now, double World)
{
	while (S.WorldMinutes() < World)
	{
		S.LobbyMinutes = std::min(S.LobbyMinutes + 1.0, World - 1440.0);
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}

void Fed(ss::Session& S)
{
	S.Life.Hunger = 10.0;
	S.Life.Thirst = 10.0;
	S.Life.Energy = 90.0;
}

/** The save the host has while the player is out (the session's last save, with the life as it stands). */
ss::SaveData HostSave(const Hooks& H, const ss::Session& S)
{
	ss::SaveData D = H.Last;
	D.Life = S.Life;
	D.BankrollCents = S.BankrollCents;
	if (D.WorldText.empty() && D.WorldSnapshot)
	{
		D.WorldSnapshot->Write(D.WorldText);
	}
	return D;
}

/** The roster as the engine's reserved players. */
std::vector<ss::ReservedPlayer> Known(const life::LiveEntry& E)
{
	std::vector<ss::ReservedPlayer> Out;
	for (const std::pair<std::string, int>& R : E.Roster)
	{
		ss::ReservedPlayer P;
		P.Name = R.first;
		P.Type = static_cast<ss::Archetype>(R.second);
		Out.push_back(P);
	}
	return Out;
}

/** What the player's table saw, for the world: the faces they sat with. */
std::vector<ss::LiveSeen> Seen(const life::LiveEntry& E)
{
	std::vector<ss::LiveSeen> Out;
	for (size_t K = 0; K < E.Faces.size() && K < 4; ++K)
	{
		ss::LiveSeen M;
		M.Tag = 'M';
		M.Name = E.Faces[K].Name;
		Out.push_back(M);
	}
	if (!E.Faces.empty())
	{
		ss::LiveSeen B;
		B.Tag = 'B';
		B.Name = E.Faces[0].Name;
		B.Value = -24000;
		Out.push_back(B);
	}
	return Out;
}

ss::SaveData Fresh()
{
	Hooks H;
	ss::Session S(H, "fixture-fresh");
	S.NewCareer(Someone());
	S.Save();
	Dump("fresh", H.Last);
	return H.Last;
}

ss::SaveData MidNightly()
{
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "fixture-nightly"));
	S->NewCareer(Someone());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	// Wednesday, October 7: a shift at the laundromat, the corner store, an order from Penny Drop.
	Now = At(*S, Now, 2.0 * Day + 7.0 * 60.0);
	Fed(*S);
	S->BankrollCents += 20000;
	if (!S->StartActivity("washfold").empty())
	{
		std::printf("fixture: the shift didn't start\n");
	}
	Now = Wait(*S, Now, 6.0);
	ss::store::Basket Snack;
	Snack.Add("roller-dog");
	Snack.Add("volt-rush");
	S->Checkout(Snack);
	ss::store::Basket Order;
	Order.Add("volt-rush", 4);
	S->PlaceOrder(Order);
	Now = Wait(*S, Now, 2.0);
	Dump("mid-month: a shift, the store, an order", H.Last);
	// The rent, paid before Friday.
	S->BankrollCents += 200000;
	S->PayRent();
	// Three nights at the Embercrest: Thursday's Nightly played out by the engine, Saturday's won, Sunday's Warm-Up called
	// off. The host's side: settle on the save, note the night, load the save again (as the room does).
	struct Night
	{
		const char* Id;
		int Mode; // 0 played out, 1 won, 2 refunded
	};
	const Night Nights[] = {{"embercrest-nightly@3", 0}, {"embercrest-nightly@5", 1}, {"embercrest-warmup@6", 2}};
	for (const Night& N : Nights)
	{
		const live::Occurrence O = live::FindOccurrence(N.Id);
		Now = At(*S, Now, O.DeskOpens + 40.0);
		Fed(*S);
		const std::string Why = S->GoToLive(O.Id);
		if (!Why.empty())
		{
			std::printf("fixture: %s: %s\n", N.Id, Why.c_str());
			continue;
		}
		ss::SaveData D = HostSave(H, *S);
		life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
		E->ArrivedAt = O.Start - 15.0;
		double End = O.Start + 180.0;
		if (N.Mode == 2)
		{
			live::Refund(D.BankrollCents, D.Life, O.Id, O.Start + 2.0, "not enough players");
			live::PayFare(D.BankrollCents, D.Life, O.Id, true, O.Start + 5.0);
			End = O.Start + 5.0;
		}
		else
		{
			bool Restored = false;
			std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known(*E), false, Restored);
			int Place = 0;
			ss::Chips Prize = 0;
			if (N.Mode == 1)
			{
				Place = 1;
				Prize = T->PrizeFor(1);
			}
			else
			{
				ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
				for (int Guard = 0; Guard < 6000 && !T->Hero().Busted && !T->bFinished; ++Guard)
				{
					for (const ss::TEvent& Ev : T->SimulateTick(&Auto))
					{
						if (Ev.Type == ss::TEventType::Bust && Ev.IsHero)
						{
							Place = Ev.Place;
							Prize = Ev.PrizeCents;
						}
					}
				}
				if (Place <= 0)
				{
					Place = T->Hero().Place > 0 ? T->Hero().Place : T->HeroRank();
					Prize = T->PrizeFor(Place);
				}
				End = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
			}
			const std::vector<ss::LiveSeen> Table = Seen(*E);
			live::Settle(D.BankrollCents, D.Life, O.Id, Place, T->Spec.Entrants, Prize, End);
			live::PayFare(D.BankrollCents, D.Life, O.Id, true, End + 5.0);
			D.NoteLive(End, O.Id, Place, Prize, T->Spec.Entrants, Table);
		}
		D.ClockMinutes = End + 30.0 - Day;
		Dump(N.Id, D);
		S.reset();
		S.reset(new ss::Session(H, "fixture-nightly", &D));
		S->CurrentScreen = ss::Screen::Lobby;
		S->ResumeCalendarFrom(End + 30.0);
		Now = Wait(*S, Now, 0.2);
	}
	// Tonight, Wednesday October 14: the Nightly, the player in the room and forty hands in when the game closed.
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
	Now = At(*S, Now, O.DeskOpens + 60.0);
	Fed(*S);
	const std::string Why = S->GoToLive(O.Id);
	if (!Why.empty())
	{
		std::printf("fixture: tonight: %s\n", Why.c_str());
	}
	ss::SaveData D = HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	if (E)
	{
		E->ArrivedAt = O.Start - 20.0;
		bool Restored = false;
		std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known(*E), false, Restored);
		ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
		for (int Hand = 0; Hand < 40 && !T->Hero().Busted && !T->bFinished; ++Hand)
		{
			T->SimulateTick(&Auto);
		}
		E->Checkpoint = T->Checkpoint();
		E->CheckpointAt = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
		E->CheckpointHost = "hands=40\tmet=npc:Mei,npc:Big Lou\nbreak=0";
		D.ClockMinutes = E->CheckpointAt - Day;
	}
	Dump("tonight, checkpointed", D);
	return D;
}

ss::SaveData TeamRiverLine()
{
	Hooks H;
	ss::Session S(H, "fixture-kast");
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	Now = At(S, Now, 2.0 * Day + 20.0 * 60.0);
	Fed(S);
	S.BankrollCents += 100000;
	S.Buy("ram-32");
	S.GoLive();
	Now = Wait(S, Now, 20.0);
	S.EndStream();
	// A streamer RiverLine signed: the offer accepted, another on the table, one turned down.
	S.Channel.Followers = std::max(S.Channel.Followers, 6200);
	S.Channel.Offers.insert("riverline");
	S.Channel.Offers.insert("tunnelrat");
	S.Channel.Offers.insert("sitwell");
	const std::string Signed = S.AcceptDeal("riverline");
	if (!Signed.empty())
	{
		std::printf("fixture: riverline: %s\n", Signed.c_str());
	}
	S.DeclineDeal("sitwell");
	Now = Wait(S, Now, 0.5);
	Now = At(S, Now, S.WorldMinutes() + 3.0 * Day);
	S.Save();
	Dump("team riverline", H.Last);
	return H.Last;
}

ss::SaveData Evicted()
{
	Hooks H;
	ss::Session S(H, "fixture-evicted");
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	S.BankrollCents = 100000;
	S.Buy("monitor-24");
	S.Buy("mattress");
	S.BankrollCents = 0;
	Now = At(S, Now, 5.0 * Day + 1.0);
	Now = At(S, Now, 8.0 * Day + 1.0);
	S.BankrollCents = 5000;
	Now = At(S, Now, 8.0 * Day + 10.0 * 60.0 + 1.0);
	S.Save();
	Dump("evicted", H.Last);
	return H.Last;
}

/** A save from before the casino was the Embercrest: the v1 header and "riverside" for "embercrest" (not in the names). */
std::string AsV1(const std::string& V2)
{
	static const std::pair<std::string, std::string> Names[] = {{"embercrest", "riverside"}, {"Embercrest", "Riverside"}, {"EMBERCREST", "RIVERSIDE"}};
	std::string Out;
	size_t Start = 0;
	bool First = true;
	while (Start < V2.size())
	{
		size_t End = V2.find('\n', Start);
		End = End == std::string::npos ? V2.size() : End + 1;
		std::string Line = V2.substr(Start, End - Start);
		if (First)
		{
			Line = "shortstack.nightone.v1\n";
			First = false;
		}
		else if (Line.rfind("name\t", 0) != 0 && Line.rfind("hero\tname\t", 0) != 0)
		{
			for (const auto& Pair : Names)
			{
				for (size_t P = Line.find(Pair.first); P != std::string::npos; P = Line.find(Pair.first, P + Pair.second.size()))
				{
					Line.replace(P, Pair.first.size(), Pair.second);
				}
			}
		}
		Out += Line;
		Start = End;
	}
	return Out;
}

// ---------------------------------------------------------------- update 2's day-0 fixtures (write2)

/** One live night the host plays: settle on the save, note the night, load the save again (MidNightly's loop, one night). */
double HostNight(std::unique_ptr<ss::Session>& S, Hooks& H, double Now, const char* Seed, const char* Id, bool Won)
{
	const live::Occurrence O = live::FindOccurrence(Id);
	Now = At(*S, Now, O.DeskOpens + 40.0);
	Fed(*S);
	const std::string Why = S->GoToLive(O.Id);
	if (!Why.empty())
	{
		std::printf("fixture: %s: %s\n", Id, Why.c_str());
		return Now;
	}
	ss::SaveData D = HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	E->ArrivedAt = O.Start - 15.0;
	bool Restored = false;
	std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known(*E), false, Restored);
	int Place = 0;
	ss::Chips Prize = 0;
	double End = O.Start + 180.0;
	if (Won)
	{
		Place = 1;
		Prize = T->PrizeFor(1);
	}
	else
	{
		ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
		for (int Guard = 0; Guard < 6000 && !T->Hero().Busted && !T->bFinished; ++Guard)
		{
			for (const ss::TEvent& Ev : T->SimulateTick(&Auto))
			{
				if (Ev.Type == ss::TEventType::Bust && Ev.IsHero)
				{
					Place = Ev.Place;
					Prize = Ev.PrizeCents;
				}
			}
		}
		if (Place <= 0)
		{
			Place = T->Hero().Place > 0 ? T->Hero().Place : T->HeroRank();
			Prize = T->PrizeFor(Place);
		}
		End = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
	}
	const std::vector<ss::LiveSeen> Table = Seen(*E);
	live::Settle(D.BankrollCents, D.Life, O.Id, Place, T->Spec.Entrants, Prize, End);
	live::PayFare(D.BankrollCents, D.Life, O.Id, true, End + 5.0);
	D.NoteLive(End, O.Id, Place, Prize, T->Spec.Entrants, Table);
	D.ClockMinutes = End + 30.0 - Day;
	Dump(Id, D);
	S.reset();
	S.reset(new ss::Session(H, Seed, &D));
	S->CurrentScreen = ss::Screen::Lobby;
	S->ResumeCalendarFrom(End + 30.0);
	return Wait(*S, Now, 0.2);
}

/** The hero's turn, if it's open: the grader's best option (DeepRuns' hero). */
void ActBest(ss::Session& S, double Now)
{
	if (!S.HasPrompt || Now - S.Prompt.OpenedAt <= 0.3)
	{
		return;
	}
	const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
	size_t Best = 0;
	for (size_t K = 1; K < Opts.size(); ++K)
	{
		Best = Opts[K].Ev > Opts[Best].Ev ? K : Best;
	}
	S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
}

/** A network event from its start to the result and back to the lobby: sprinted, the player taking the hands Sprint
 *  hands back (the first, the bubble, the final table) with the grader's best option and sprinting on after each. False
 *  when it didn't open. */
bool Sprinted(ss::Session& S, double& Now, const ss::net::EventInstance& I)
{
	const ss::net::Network& Net = ss::net::Shared();
	if (S.WorldMinutes() < I.Start)
	{
		Now = At(S, Now, I.Start);
	}
	Fed(S);
	S.RegisterEvent(Net.Listing(I));
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		S.CurrentPace = ss::Pace::Sprint;
		Now += 1.0 / 30.0;
		S.Update(Now);
		ActBest(S, Now);
	}
	S.LeaveResults();
	return true;
}

ss::SaveData Fresh2()
{
	Hooks H;
	ss::Session S(H, "fixture2-fresh");
	S.NewCareer(Someone());
	S.Save();
	Dump("update2: fresh", H.Last);
	return H.Last;
}

ss::SaveData StakedMidNightly()
{
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "fixture2-staked"));
	S->NewCareer(Someone());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	// Wednesday, October 7: a shift at the laundromat, the corner store, an order from Penny Drop; the rent paid.
	Now = At(*S, Now, 2.0 * Day + 7.0 * 60.0);
	Fed(*S);
	S->BankrollCents += 20000;
	if (!S->StartActivity("washfold").empty())
	{
		std::printf("fixture: the shift didn't start\n");
	}
	Now = Wait(*S, Now, 6.0);
	ss::store::Basket Snack;
	Snack.Add("roller-dog");
	Snack.Add("volt-rush");
	S->Checkout(Snack);
	ss::store::Basket Order;
	Order.Add("volt-rush", 4);
	S->PlaceOrder(Order);
	Now = Wait(*S, Now, 2.0);
	S->BankrollCents += 200000;
	S->PayRent();
	Dump("update2: staked: a shift, the store, an order, the rent", H.Last);
	// Thursday's Nightly, played out by the engine on the player's own money.
	Now = HostNight(S, H, Now, "fixture2-staked", "embercrest-nightly@3", false);
	// Tonight, Wednesday October 14: Big Lou's offer at the desk, taken; the player in the room on his money.
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
	Now = At(*S, Now, O.DeskOpens + 30.0);
	Fed(*S);
	const std::string Made = S->DebugStake("offer lou");
	std::string Lou;
	for (const ss::stake::Offer& Of : S->Life.Stakes.Offers)
	{
		Lou = Of.Backer == "Big Lou" ? Of.Id : Lou;
	}
	const std::string Took = Lou.empty() ? std::string("no offer: ") + Made : S->AcceptStake(Lou);
	if (!Took.empty())
	{
		std::printf("fixture: Lou's stake: %s\n", Took.c_str());
	}
	const std::string Why = S->GoToLive(O.Id);
	if (!Why.empty())
	{
		std::printf("fixture: tonight: %s\n", Why.c_str());
	}
	ss::SaveData D = HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	if (E)
	{
		// Forty hands in when the game closed: the engine's checkpoint, the director's state and the host's own text.
		E->ArrivedAt = O.Start - 20.0;
		bool Restored = false;
		std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known(*E), false, Restored);
		ss::room::Director Room(*E, O, live::SeedFor(*E, D.HeroName) + ":room", D.Person.FirstName);
		ss::Profile Auto = ss::MakeProfile(ss::Archetype::Tag, T->R);
		for (int Hand = 0; Hand < 40 && !T->Hero().Busted && !T->bFinished; ++Hand)
		{
			const std::vector<ss::TEvent> Events = T->SimulateTick(&Auto);
			const double World = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
			Room.Events(*T, Events, World);
			Room.Rounds(*T, T->Rounds());
			Room.Clock(*T, World, 0.0);
			Room.Take();
		}
		E->Checkpoint = T->Checkpoint();
		E->CheckpointAt = O.Start + T->ClockMinutes() - static_cast<double>(O.T->StartMinute);
		E->CheckpointHost = "hands=40\tmet=npc:Mei,npc:Big Lou\nbreak=0";
		E->Room = Room.Write();
		D.ClockMinutes = E->CheckpointAt - Day;
	}
	Dump("update2: staked, tonight, checkpointed", D);
	return D;
}

ss::SaveData SponsoredStreamer()
{
	Hooks H;
	ss::Session S(H, "fixture2-streamer");
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	Now = At(S, Now, 2.0 * Day + 20.0 * 60.0);
	Fed(S);
	S.BankrollCents += 150000;
	S.Buy("ram-32");
	S.Buy("webcam-720");
	S.Channel.Followers = std::max(S.Channel.Followers, 2400);
	// A sponsor's book: Volt Rush signed (its letters in Mail), the patch sewn on the chest, Halo's offer still open.
	const std::string Signed = S.DebugSponsor("sign voltrush");
	const std::string Sewn = S.DebugSponsor("patch cap voltrush");
	const std::string Offered = S.DebugSponsor("offer halo");
	if (Transcript)
	{
		std::printf("  [sponsor] %s / %s / %s\n", Signed.c_str(), Sewn.c_str(), Offered.c_str());
	}
	// On air for hypers played by hand (the grader's best move each time) until chat has clipped one, three at most:
	// the coach's notes, Kast's log and clips.
	S.GoLive();
	Now = Wait(S, Now, 2.0);
	const ss::net::Network& Net = ss::net::Shared();
	for (int Hyper = 0; Hyper < 3 && S.Channel.Clips.empty(); ++Hyper)
	{
		ss::net::EventInstance I;
		if (!Net.Next("hyper-sprint", S.WorldMinutes() + 1.0, I))
		{
			break;
		}
		Now = At(S, Now, I.Start);
		Fed(S);
		S.RegisterEvent(Net.Listing(I));
		for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 6000000; ++Frames)
		{
			Now += 1.0 / 30.0;
			S.Update(Now);
			ActBest(S, Now);
		}
		S.LeaveResults();
	}
	if (S.Channel.Clips.empty())
	{
		// Nobody in chat clipped a hand: the regular who clips every cash cut the min-cash.
		ss::kast::Clip C;
		C.Title = "three big blinds to a min-cash. we take those";
		C.By = "dee_spincycle";
		C.At = S.WorldMinutes() - 60.0;
		C.Reach = 48.0;
		C.Kind = static_cast<int>(ss::kast::Moment::Cashed);
		S.Channel.Clips.insert(S.Channel.Clips.begin(), C);
	}
	Now = Wait(S, Now, 1.0);
	S.EndStream();
	Now = Wait(S, Now, 0.5);
	// The next afternoon, fed.
	Now = At(S, Now, S.WorldMinutes() + 12.0 * 60.0);
	Fed(S);
	Now = Wait(S, Now, 0.2);
	S.Save();
	Dump("update2: sponsored streamer", H.Last);
	return H.Last;
}

ss::SaveData Evicted2()
{
	Hooks H;
	ss::Session S(H, "fixture2-evicted");
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	S.BankrollCents = 100000;
	S.Buy("monitor-24");
	S.Buy("mattress");
	S.BankrollCents = 0;
	Now = At(S, Now, 5.0 * Day + 1.0);
	Now = At(S, Now, 8.0 * Day + 1.0);
	S.BankrollCents = 5000;
	Now = At(S, Now, 8.0 * Day + 10.0 * 60.0 + 1.0);
	S.Save();
	Dump("update2: evicted", H.Last);
	return H.Last;
}

ss::SaveData Veteran()
{
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "fixture2-veteran"));
	S->NewCareer(Someone());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	S->BankrollCents += 2000000;
	// A night at the Embercrest won and one played out: the wall, the live record, the live days.
	Now = HostNight(S, H, Now, "fixture2-veteran", "embercrest-nightly@1", true);
	Now = HostNight(S, H, Now, "fixture2-veteran", "embercrest-nightly@2", false);
	// Three hundred and ten hypers, back to back, sprinted. Sprint's batch is wall-clock (10 ms an update): a whole stretch
	// to its stop in one update keeps the frames, and so the session's own draws, the same on any machine.
	S->SprintBudgetMs = 1.0e9;
	const ss::net::Network& Net = ss::net::Shared();
	int Played = 0;
	for (int Guard = 0; Played < 310 && Guard < 400; ++Guard)
	{
		ss::net::EventInstance I;
		if (!Net.Next("hyper-sprint", S->WorldMinutes() + 1.0, I))
		{
			break;
		}
		Played += Sprinted(*S, Now, I) ? 1 : 0;
	}
	// The badges the calendar would take months to give: the club's top tier, a ring, Verified.
	const std::string Club = S->DebugBadge("grant club 3");
	const std::string Ring = S->DebugBadge("grant ring 1");
	const std::string Verified = S->DebugBadge("grant verified");
	if (Transcript)
	{
		std::printf("  [veteran] %d hypers; %s / %s / %s\n", Played, Club.c_str(), Ring.c_str(), Verified.c_str());
	}
	Now = Wait(*S, Now, 0.5);
	S->Save();
	Dump("update2: veteran", H.Last);
	return H.Last;
}

#if SS_FIXTURE_GATE1
// ---------------------------------------------------------------- update 2's gate 1 fixtures (write-g1)

namespace autoplay = ss::autoplay;
namespace style = ss::style;

autoplay::TableAuto PlanOf(style::Style St, autoplay::Mode M, autoplay::Target To, uint32_t Stops)
{
	autoplay::TableAuto A;
	A.S = St;
	A.M = M;
	A.Speed = 1;
	A.To = To;
	A.Stops = Stops;
	return A;
}

/** The learned read book as the card room seats a night with it (BackRoomAutoRules::AutoLearnedOf): every "Name/Tell" read
 *  seen twice or more, counted by name. */
style::Learned LearnedOf(const std::map<std::string, int>& Reads)
{
	style::Learned Out;
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

/** The plan's lines of the checkpoint host text, as FAutoHost writes them (BackRoomAutoRules::AutoHostText): the auto line
 *  (autoplay::RunText), an autoseen line for each player observed tonight, an autoread line for each name in the book. */
std::string PlanHostText(const autoplay::TableAuto& A, const autoplay::Run& R, const style::Learned& Book)
{
	std::string Out = autoplay::RunText(A, R) + "\n";
	for (const auto& Kv : R.Seen)
	{
		Out += "autoseen\t" + Kv.first + "\t" + ss::reads::Write(Kv.second) + "\n";
	}
	for (const auto& Kv : Book.Tells)
	{
		if (Kv.second > 0)
		{
			Out += "autoread\t" + Kv.first + "\t" + std::to_string(Kv.second) + "\n";
		}
	}
	return Out;
}

/**
 * A night the plan skips, on the card room host's side (FAutoHost::TickSkip and its sink): each hand the skip played goes
 * into the run (autoplay::NoteSkippedHand) and the brain is read afresh for the next one, the player's tablemates are kept
 * for the world, and the room director hears every tick. HostText is what LiveHostText writes at a checkpoint.
 */
struct PlanNight : ss::live::SkipSink
{
	ss::Tournament* T = nullptr;
	ss::room::Director* Room = nullptr;
	double DayStart = 0.0; // world minutes at the tournament clock's zero
	autoplay::TableAuto A;
	autoplay::Run R;
	style::Learned Book;
	const ss::reads::Table* Notes = nullptr;
	double TiltGain = 1.0;
	style::HeroBrain Brain;
	std::vector<std::string> Met;
	int Hands = 0;
	int Checkpoints = 0;
	int LastCheckpoint = -1;
	// Gate 2: the night's hands noted for Rerun the card room's way (FAutoHost::OnSkippedHand), into the host's save.
	life::State* Film = nullptr;
	std::string FilmEntry, FilmEvent;

	void Think() { Brain = autoplay::BrainFor(A, R, TiltGain, 1, Notes, &Book); }
	void HandDone(const ss::Hand& H, int HeroSeat) override
	{
		autoplay::NoteSkippedHand(R, H, HeroSeat, TiltGain);
		Think();
		++Hands;
		for (size_t K = 0; K < H.Seats.size(); ++K)
		{
			const std::string& Id = H.Seats[K].Id;
			if (static_cast<int>(K) != HeroSeat && !Id.empty() && std::find(Met.begin(), Met.end(), Id) == Met.end())
			{
				Met.push_back(Id);
			}
		}
#if SS_FIXTURE_GATE2
		if (Film && T)
		{
			FilmHand(H, HeroSeat);
		}
#endif
	}
#if SS_FIXTURE_GATE2
	/** The hand as the table card's HANDS page records it (BackRoomAutoRules::AutoSkippedRecord: the player "You", the others
	 *  by the room's caption name, the hands turned up once each), then video::NoteLiveHand with the night's context. */
	void FilmHand(const ss::Hand& H, int HeroSeat)
	{
		std::vector<std::string> Names;
		std::vector<int> Shown;
		const int Mine = HeroSeat >= 0 && HeroSeat < static_cast<int>(H.Seats.size()) ? H.Seats[static_cast<size_t>(HeroSeat)].Seat : -1;
		for (const ss::HandSeat& S : H.Seats)
		{
			if (S.Seat >= static_cast<int>(Names.size()))
			{
				Names.resize(static_cast<size_t>(S.Seat) + 1);
			}
			const int Idx = T->PlayerIndex(S.Id);
			const std::string Name = Idx >= 0 ? T->Players[static_cast<size_t>(Idx)].Name : std::string();
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
		const ss::recall::Record Rec = ss::recall::Of(H, Mine, Names, Shown, T->Tick, T->LevelIndex + 1);
		ss::video::LiveContext Ctx;
		Ctx.Source = 1;
		Ctx.EntryId = FilmEntry;
		Ctx.Event = FilmEvent;
		Ctx.BuyIn = T->Spec.BuyInCents;
		Ctx.Field = T->Spec.Entrants;
		Ctx.Remaining = T->Remaining;
		Ctx.Bubble = T->HandForHand();
		Ctx.Final = T->Tables.size() <= 1;
		Ctx.Auto = true;
		Ctx.Style = static_cast<int>(A.S);
		Ctx.World = DayStart + T->ClockMinutes();
		ss::video::NoteLiveHand(*Film, Rec, nullptr, Ctx);
	}
#endif
	void Events(const std::vector<ss::TEvent>& E) override
	{
		if (Room && T)
		{
			const double World = DayStart + T->ClockMinutes();
			Room->Events(*T, E, World);
			Room->Rounds(*T, T->Rounds());
			Room->Clock(*T, World, 0.0);
			Room->Take();
		}
	}
	void Checkpoint(int Tick) override
	{
		++Checkpoints;
		LastCheckpoint = Tick;
	}
	std::string HostText() const
	{
		std::string Out = "hands\t" + std::to_string(Hands) + "\n";
		for (const std::string& Id : Met)
		{
			Out += "met\t" + Id + "\n";
		}
		return Out + PlanHostText(A, R, Book);
	}
};

/** The plan takes the seat as the night begins (FAutoHost::StartRun): Sharpness read once from Form and the player's
 *  condition (fed and rested here), held for the run; the plan's tilt from the player's (calm); the read book seated. */
void TakeSeat(PlanNight& N, ss::SaveData& D, double World)
{
	const double Form = autoplay::FormAt(D.Life.Auto, World);
	N.R.Sharp = autoplay::SharpnessOf(Form, 0.0, false, false);
	N.R.SharpSetAt = World;
	N.R.Tilt = 0.0;
	N.TiltGain = std::min(2.5, std::max(0.3, D.Life.Perks.TiltGain));
	N.Book = LearnedOf(D.Life.Reads);
	N.Notes = &D.Life.Auto.Notes;
	D.Life.Update2LinesRead |= life::U2AutoPref;
	N.Think();
}

/**
 * A Nightly the plan skips to the result through live::SkipSlice, settled the host's way (the cage, the bus home,
 * FAutoHost::OnSettle's played line and notes, the night noted for the world) and the save loaded again.
 */
double PlanNightToResult(std::unique_ptr<ss::Session>& S, Hooks& H, double Now, const char* Seed, const char* Id, style::Style St, bool Film = false)
{
	const live::Occurrence O = live::FindOccurrence(Id);
	Now = At(*S, Now, O.DeskOpens + 40.0);
	Fed(*S);
	const std::string Why = S->GoToLive(O.Id);
	if (!Why.empty())
	{
		std::printf("fixture: %s: %s\n", Id, Why.c_str());
		return Now;
	}
	ss::SaveData D = HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	E->ArrivedAt = O.Start - 15.0;
	bool Restored = false;
	std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known(*E), false, Restored);
	PlanNight N;
	N.T = T.get();
	N.DayStart = O.Start - static_cast<double>(O.T->StartMinute);
	N.A = PlanOf(St, autoplay::Mode::Skip, autoplay::Target::Result, 0);
	TakeSeat(N, D, O.Start);
#if SS_FIXTURE_GATE2
	if (Film)
	{
		// The skipped hands' ids as the card room keeps them apart from the table's own ("lv:<entry>:skip:<n>", BackRoomAuto.cpp).
		N.Film = &D.Life;
		N.FilmEntry = O.Id + ":skip";
		N.FilmEvent = O.T->Name;
	}
#else
	(void)Film;
#endif
	for (int Guard = 0; Guard < 20000 && !T->Hero().Busted && !T->bFinished; ++Guard)
	{
		if (ss::live::SkipSlice(*T, N.Brain, N.A, 0.0, N).Why != ss::live::SkipWhy::Budget)
		{
			break;
		}
	}
	const int Place = T->Hero().Place > 0 ? T->Hero().Place : T->HeroRank();
	const ss::Chips Prize = T->PrizeFor(Place);
	const double End = N.DayStart + T->ClockMinutes();
	const std::vector<ss::LiveSeen> Table = Seen(*E);
	live::Settle(D.BankrollCents, D.Life, O.Id, Place, T->Spec.Entrants, Prize, End);
	live::PayFare(D.BankrollCents, D.Life, O.Id, true, End + 5.0);
#if SS_FIXTURE_GATE2
	if (Film)
	{
		// The night's recap, when it was one (FAutoHost::OnSettle's NoteLiveRecap).
		ss::video::LiveContext Ctx;
		Ctx.Source = 1;
		Ctx.EntryId = O.Id;
		Ctx.Event = O.T->Name;
		Ctx.BuyIn = T->Spec.BuyInCents;
		Ctx.Field = T->Spec.Entrants;
		Ctx.Remaining = T->Remaining;
		Ctx.Final = T->Tables.size() <= 1;
		Ctx.Auto = N.R.AiHands > 0;
		Ctx.Style = Ctx.Auto ? static_cast<int>(N.A.S) : 0;
		Ctx.World = End;
		ss::video::NoteLiveRecap(D.Life, Ctx, Place, Prize);
	}
#endif
	if (N.R.AiHands > 0 || N.R.AiDecisions > 0)
	{
		autoplay::Record(D.Life.Auto, O.Id, 1, 0, N.A, N.R);
		D.Life.Update2LinesRead |= life::U2AutoPlayed | life::U2AutoPref;
	}
	ss::reads::Table Named;
	for (const auto& It : N.R.Seen)
	{
		if (It.first.rfind("npc:", 0) == 0)
		{
			Named[autoplay::NoteKey(T.get(), It.first)] = It.second;
		}
	}
	autoplay::FoldNotes(D.Life.Auto, Named, End);
	if (!D.Life.Auto.Notes.empty())
	{
		D.Life.Update2LinesRead |= life::U2Notes;
	}
	D.NoteLive(End, O.Id, Place, Prize, T->Spec.Entrants, Table);
	D.ClockMinutes = End + 30.0 - Day;
	if (Transcript)
	{
		std::printf("  [plan] %s: %s skipped %d hands (%d decisions), %s of %d for %s\n", Id, style::InfoOf(St).Name, N.R.AiHands, N.R.AiDecisions, ss::Ordinal(Place).c_str(),
			T->Spec.Entrants, ss::Money(Prize).c_str());
	}
	Dump(Id, D);
	S.reset();
	S.reset(new ss::Session(H, Seed, &D));
	S->CurrentScreen = ss::Screen::Lobby;
	S->ResumeCalendarFrom(End + 30.0);
	return Wait(*S, Now, 0.2);
}

/**
 * A network event from its start to the result with the plan in the seat, skipped (SkipAhead's path) with the stops
 * given. A decision a stop rule hands over is answered with the grader's best option. The first time a stop leaves the
 * plan watching at 1x, the player turns it off and plays Mine hands the same way, then skips on to the result (and again
 * after any later stop). False when it didn't open.
 */
bool PlanHyper(ss::Session& S, double& Now, const ss::net::EventInstance& I, style::Style St, uint32_t Stops, int Mine)
{
	const ss::net::Network& Net = ss::net::Shared();
	if (S.WorldMinutes() < I.Start)
	{
		Now = At(S, Now, I.Start);
	}
	Fed(S);
	S.RegisterEvent(Net.Listing(I));
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	// Whole stretches in one update: the same frames on any machine.
	S.SprintBudgetMs = 1.0e9;
	S.SetAutoplay(0, PlanOf(St, autoplay::Mode::Skip, autoplay::Target::Result, Stops));
	int MineFrom = -1;
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.TableCount() == 0 || S.Finishing())
		{
			continue;
		}
		ActBest(S, Now);
		if (S.HasPrompt)
		{
			continue;
		}
		const autoplay::Panel P = S.AutoPanel(0);
		if (MineFrom < 0 && P.On && !P.Skipping)
		{
			// The first stop: the player takes the next hands.
			MineFrom = S.HandsPlayed;
			autoplay::TableAuto Off = S.Autoplay(0);
			Off.M = autoplay::Mode::Off;
			S.SetAutoplay(0, Off);
		}
		else if (MineFrom >= 0 && ((!P.On && S.HandsPlayed >= MineFrom + Mine) || (P.On && !P.Skipping)))
		{
			S.SkipAhead(0, autoplay::Target::Result);
		}
	}
	S.LeaveResults();
	return true;
}

/** A network event played by hand from its start to the result: every decision the grader's best option (DeepRuns' hero). */
bool ByHand(ss::Session& S, double& Now, const ss::net::EventInstance& I)
{
	const ss::net::Network& Net = ss::net::Shared();
	if (S.WorldMinutes() < I.Start)
	{
		Now = At(S, Now, I.Start);
	}
	Fed(S);
	S.RegisterEvent(Net.Listing(I));
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	// The pace the last table left (a skip leaves it at Sprint) carries over: this one is played, at Smart.
	S.CurrentPace = ss::Pace::Smart;
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 6000000; ++Frames)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		ActBest(S, Now);
	}
	S.LeaveResults();
	return true;
}

/** update2_blank_world: a fresh-start career four days in (Friday, October 9), its world lines kept. */
ss::SaveData BlankWorld()
{
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "fixture-g1-blank"));
	S->NewCareer(Someone(), ss::world::Start::Blank);
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	S->BankrollCents += 100000;
	const ss::net::Network& Net = ss::net::Shared();
	ss::net::EventInstance I;
	// Night One: the first hyper after 2:07 AM, skipped whole by the plan (GTO): nothing of the player's to grade.
	if (Net.Next("hyper-sprint", S->WorldMinutes() + 1.0, I))
	{
		PlanHyper(*S, Now, I, style::Style::Gto, 0, 0);
	}
	// Tuesday: a shift at the Wash & Fold, then a hyper played by hand (the grader's best, every decision): Form moves.
	Now = At(*S, Now, 1.0 * Day + 7.0 * 60.0);
	Fed(*S);
	if (!S->StartActivity("washfold").empty())
	{
		std::printf("fixture: the shift didn't start\n");
	}
	Now = Wait(*S, Now, 6.0);
	if (Net.Next("hyper-sprint", 1.0 * Day + 14.0 * 60.0, I))
	{
		ByHand(*S, Now, I);
	}
	// Wednesday: a hyper the plan (TAG) skips with the premium-hands, all-in and bubble stops on: those decisions come to the
	// player (a night can't end without an all-in, so one always does), and at the first stop the player takes four hands,
	// then skips on to the result. Its played line has both.
	if (Net.Next("hyper-sprint", 2.0 * Day + 14.0 * 60.0, I))
	{
		PlanHyper(*S, Now, I, style::Style::Tag, autoplay::StopBubble | autoplay::StopPremium | autoplay::StopMyAllIn, 4);
	}
	Dump("update2 g1: blank world, three hypers", H.Last);
	// Thursday's Nightly at the Embercrest, skipped by the plan (TAG) to the result.
	Now = PlanNightToResult(S, H, Now, "fixture-g1-blank", "embercrest-nightly@3", style::Style::Tag);
	// Friday afternoon, fed: saved.
	Now = At(*S, Now, 4.0 * Day + 15.0 * 60.0);
	Fed(*S);
	Now = Wait(*S, Now, 0.2);
	S->Save();
	Dump("update2 g1: blank world", H.Last);
	return H.Last;
}

/** update2_auto_mid_nightly: tonight's Nightly, the plan (GTO) skipping to the bubble, checkpointed mid-skip. */
ss::SaveData AutoMidNightly()
{
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "fixture-g1-auto"));
	S->NewCareer(Someone());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	// Wednesday, October 7: a shift, the rent paid, then a hyper the plan (TAG) skipped to the result.
	Now = At(*S, Now, 2.0 * Day + 7.0 * 60.0);
	Fed(*S);
	S->BankrollCents += 200000;
	if (!S->StartActivity("washfold").empty())
	{
		std::printf("fixture: the shift didn't start\n");
	}
	Now = Wait(*S, Now, 6.0);
	S->PayRent();
	ss::net::EventInstance I;
	if (ss::net::Shared().Next("hyper-sprint", 2.0 * Day + 14.0 * 60.0, I))
	{
		PlanHyper(*S, Now, I, style::Style::Tag, 0, 0);
	}
	// A player who has been grinding hands (Form 88, the dev command's way), with tells learned at the tables.
	const std::string Form = S->DebugAuto("form 88");
	S->Life.Reads["Mei/ChipGlance"] = 3;
	S->Life.Reads["Big Lou/Swallow"] = 2;
	S->Life.Reads["Sal/StareDown"] = 1;
	if (Transcript)
	{
		std::printf("  [plan] %s\n", Form.c_str());
	}
	// Tonight, Wednesday October 14: the Nightly on the player's own money; the plan takes the seat and skips to the bubble.
	const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
	Now = At(*S, Now, O.DeskOpens + 30.0);
	Fed(*S);
	const std::string Why = S->GoToLive(O.Id);
	if (!Why.empty())
	{
		std::printf("fixture: tonight: %s\n", Why.c_str());
	}
	ss::SaveData D = HostSave(H, *S);
	life::LiveEntry* E = live::EntryFor(D.Life, O.Id);
	if (E)
	{
		E->ArrivedAt = O.Start - 20.0;
		bool Restored = false;
		std::unique_ptr<ss::Tournament> T = live::ResumeField(O, *E, E->Entrants, D.HeroName, Known(*E), false, Restored);
		ss::room::Director Room(*E, O, live::SeedFor(*E, D.HeroName) + ":room", D.Person.FirstName);
		PlanNight N;
		N.T = T.get();
		N.Room = &Room;
		N.DayStart = O.Start - static_cast<double>(O.T->StartMinute);
		N.A = PlanOf(style::Style::Gto, autoplay::Mode::Skip, autoplay::Target::Bubble, autoplay::StopFinalTable | autoplay::StopBubble);
		TakeSeat(N, D, O.Start);
		// The game closed just after the checkpoint at hand 25 (the skip saves the night every 5 hands, R5).
		ss::live::SkipWhy Last = ss::live::SkipWhy::Budget;
		for (int Guard = 0; Guard < 2000 && N.LastCheckpoint < 25 && !T->Hero().Busted && !T->bFinished; ++Guard)
		{
			Last = ss::live::SkipSlice(*T, N.Brain, N.A, 0.0, N).Why;
			if (Last != ss::live::SkipWhy::Budget)
			{
				break;
			}
		}
		if (Last != ss::live::SkipWhy::Budget || T->Hero().Busted)
		{
			std::printf("fixture: tonight's skip ended before its checkpoint (why %d, hand %d)\n", static_cast<int>(Last), T->Tick);
		}
		E->Checkpoint = T->Checkpoint();
		E->CheckpointAt = N.DayStart + T->ClockMinutes();
		E->CheckpointHost = N.HostText();
		E->Room = Room.Write();
		D.ClockMinutes = E->CheckpointAt - Day;
		if (Transcript)
		{
			std::printf("  [plan] tonight: %s, Sharpness %.2f, tilt %.3f, %d hands (%d decisions) by the plan, %d checkpoints, hand %d, %d left\n", style::InfoOf(N.A.S).Name,
				N.R.Sharp, N.R.Tilt, N.R.AiHands, N.R.AiDecisions, N.Checkpoints, T->Tick, T->Remaining);
		}
	}
	Dump("update2 g1: auto mid nightly, checkpointed", D);
	return D;
}
#endif

#if SS_FIXTURE_GATE2
// ---------------------------------------------------------------- update 2's gate 2 fixture (write-g2)

namespace video = ss::video;

/** The clock to World if it's still ahead (At would wind it back). */
double CreatorUntil(ss::Session& S, double Now, double World)
{
	return World > S.WorldMinutes() ? At(S, Now, World) : Now;
}

/** A Teaching Breakdown of one of these is a strategy video, evergreen (§4.5). */
bool CreatorStrategy(video::Kind K)
{
	return K == video::Kind::Bluff || K == video::Kind::HeroCall || K == video::Kind::HeroFold || K == video::Kind::Bubble;
}

bool CreatorEventKind(video::Kind K)
{
	return K == video::Kind::DeepRun || K == video::Kind::FinalTable || K == video::Kind::Win;
}

/** The night a candidate was cut on (Video.cpp's NightKey: the small hours count with the evening before). */
int CreatorNight(double World)
{
	return static_cast<int>(std::floor((World - 10.0 * 60.0) / Day));
}

/** The event a candidate came from (Video.cpp's EventKey: the id less its hand, and a live skip's ":skip"). */
std::string CreatorEvent(const std::string& Id)
{
	const size_t Cut = Id.rfind(':');
	std::string Key = Cut == std::string::npos ? Id : Id.substr(0, Cut);
	if (Key.size() >= 5 && Key.compare(Key.size() - 5, 5, ":skip") == 0)
	{
		Key.resize(Key.size() - 5);
	}
	return Key;
}

/**
 * Studio's pick for an evening, in turn, from the open candidates (best first): a Teaching Breakdown of a strategy hand
 * (evergreen), a Hype Short, a Question Breakdown with the Number frame, a Vlog of the best night (three to eight of its
 * hands), a Recap of an event's result with its hands (two to six; else a Plain Breakdown with the Trophy frame); whatever
 * the turn can't make, a Story Breakdown.
 */
std::vector<video::Choice> CreatorPicks(const std::vector<video::Candidate>& Open, int Turn)
{
	std::vector<video::Choice> Out;
	if (Open.empty())
	{
		return Out;
	}
	auto One = [](const video::Candidate& C, video::Format F, video::TitleStyle T, video::Frame Fr) {
		video::Choice Ch;
		Ch.Candidates = {C.Id};
		Ch.F = F;
		Ch.T = T;
		Ch.Fr = Fr;
		return Ch;
	};
	const video::Candidate& Best = Open.front();
	switch (Turn % 5)
	{
	case 0:
	{
		const auto It = std::find_if(Open.begin(), Open.end(), [](const video::Candidate& C) { return CreatorStrategy(C.K); });
		Out.push_back(One(It != Open.end() ? *It : Best, video::Format::Breakdown, video::TitleStyle::Teaching, video::Frame::Cards));
		break;
	}
	case 1:
		Out.push_back(One(Best, video::Format::Short, video::TitleStyle::Hype, video::Frame::Reaction));
		break;
	case 2:
		Out.push_back(One(Best, video::Format::Breakdown, video::TitleStyle::Question, video::Frame::Number));
		break;
	case 3:
	{
		video::Choice Night;
		Night.F = video::Format::Vlog;
		Night.T = video::TitleStyle::Story;
		Night.Fr = video::Frame::Versus;
		const auto Lead = std::find_if(Open.begin(), Open.end(), [](const video::Candidate& C) { return C.K != video::Kind::Story; });
		for (const video::Candidate& X : Open)
		{
			if (Lead != Open.end() && Night.Candidates.size() < 8 && X.K != video::Kind::Story && CreatorNight(X.At) == CreatorNight(Lead->At))
			{
				Night.Candidates.push_back(X.Id);
			}
		}
		Out.push_back(Night);
		break;
	}
	default:
	{
		for (const video::Candidate& E : Open)
		{
			if (!CreatorEventKind(E.K))
			{
				continue;
			}
			video::Choice Recap;
			Recap.F = video::Format::Recap;
			Recap.T = video::TitleStyle::Plain;
			Recap.Fr = video::Frame::Trophy;
			Recap.Candidates = {E.Id};
			for (const video::Candidate& X : Open)
			{
				if (Recap.Candidates.size() < 7 && !CreatorEventKind(X.K) && X.K != video::Kind::Story && CreatorEvent(X.Id) == CreatorEvent(E.Id))
				{
					Recap.Candidates.push_back(X.Id);
				}
			}
			Out.push_back(Recap);
			break;
		}
		Out.push_back(One(Best, video::Format::Breakdown, video::TitleStyle::Plain, video::Frame::Trophy));
		break;
	}
	}
	Out.push_back(One(Best, video::Format::Breakdown, video::TitleStyle::Story, video::Frame::Reaction));
	return Out;
}

/** The evening's upload through Studio's PUBLISH: the edit's time and energy pass, and FinishSkip lands it. */
bool CreatorUpload(ss::Session& S, double& Now, int Turn, std::array<int, 4>& ByFormat)
{
	for (const video::Choice& Ch : CreatorPicks(S.VideoCandidates(), Turn))
	{
		if (!video::CanUpload(Ch, S.Life.Video.Open).empty())
		{
			continue;
		}
		Fed(S);
		if (!S.Publish(Ch).empty())
		{
			return false;
		}
		for (int Guard = 0; S.TimeSkip.Active && Guard < 200; ++Guard)
		{
			Now = Wait(S, Now, 0.5);
		}
		++ByFormat[static_cast<size_t>(Ch.F)];
		return true;
	}
	return false;
}

/** A network event sprinted from its start to the result (today's Sprint; the hands it hands back played with the grader's
 *  best), whole stretches an update so the stream sees the same frames on any machine. False when it didn't open. */
bool CreatorSprint(ss::Session& S, double& Now, const ss::net::EventInstance& I)
{
	Now = CreatorUntil(S, Now, I.Start);
	Fed(S);
	S.RegisterEvent(ss::net::Shared().Listing(I));
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	S.SprintBudgetMs = 1.0e9;
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		S.CurrentPace = ss::Pace::Sprint;
		Now += 1.0 / 30.0;
		S.Update(Now);
		ActBest(S, Now);
	}
	S.LeaveResults();
	return true;
}

/**
 * A hand at Dee's table on the night the host played (FAutoHost::OnHandEnded's video::NoteLiveHand, Source 2): a cooler,
 * the player's eights against Sal's ace-king, all in on the flop at $1/$2 with $200 behind. Sal and Twitch are in the
 * record as the table had them; the candidate is storytime, and its titles name nobody.
 */
void CreatorDeeHand(life::State& L, double World)
{
	std::vector<ss::Card> Deck = ss::ParseCards("8hAh7c8dKh2d3h8sAcKd4h2c5h9s");
	for (int C = 0; C < 52; ++C)
	{
		if (std::find(Deck.begin(), Deck.end(), C) == Deck.end())
		{
			Deck.push_back(C);
		}
	}
	ss::HandConfig Cfg;
	Cfg.Players = {{0, ss::HeroId, 20000}, {1, "Sal", 20000}, {2, "Twitch", 20000}};
	Cfg.ButtonSeat = 2;
	Cfg.SmallBlind = 100;
	Cfg.BigBlind = 200;
	Cfg.Deck = Deck;
	ss::Rng R("fixture-g2-dee-hand");
	ss::Hand H(Cfg, R);
	// Twitch folds the button; the player raises from the small blind and Sal calls; the player bets the flop, Sal moves in.
	for (const ss::PlayerAction& A : {ss::PlayerAction::Fold(), ss::PlayerAction::RaiseTo(600), ss::PlayerAction::Call(), ss::PlayerAction::RaiseTo(800),
			 ss::PlayerAction::RaiseTo(19400), ss::PlayerAction::Call()})
	{
		if (!H.bComplete)
		{
			H.Act(A);
		}
	}
	std::vector<int> Shown;
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
	const ss::recall::Record Rec = ss::recall::Of(H, 0, {"You", "Sal", "Twitch"}, Shown, 14, 1);
	ss::video::LiveContext Ctx;
	Ctx.Source = 2;
	Ctx.World = World;
	ss::video::NoteLiveHand(L, Rec, nullptr, Ctx);
	if (!H.bComplete || Transcript)
	{
		std::printf("  [dee] the cooler: %s, pot %lld, the player's net %lld\n", H.bComplete ? "dealt out" : "NOT COMPLETE", static_cast<long long>(Rec.Pot),
			static_cast<long long>(Rec.HeroNet));
	}
}

/** One line on the channel (the "show" mode's progress). */
void CreatorLine(const ss::Session& S, int K)
{
	const video::Channel& C = S.Life.Video;
	const double World = S.WorldMinutes();
	std::printf("  [creator] day %3d %s: %6.0f subscribers, %8.0f views, %2zu videos whole, %2zu stubs, %2zu open; partner %s; balance %s, paid %s; Kast %d; bank %s\n", K,
		ss::net::DateLabel(ss::net::DayOf(World)).c_str(), video::SubsAt(C, World), video::TotalViews(C, World), C.Videos.size(), C.Stubs.size(), C.Open.size(),
		C.PartnerAt >= 0.0 ? ss::net::DateLabel(ss::net::DayOf(C.PartnerAt)).c_str() : C.ReviewAt >= 0.0 ? "review" : "no", ss::Money(C.BalanceCents).c_str(),
		ss::Money(C.PaidCents).c_str(), S.Channel.Followers, ss::Money(S.BankrollCents).c_str());
}

/**
 * update2_creator (see the top): the creator's days until Rerun's first payout has landed and the next Friday (at most
 * MaxDays), then a night at Dee's, saved the next afternoon. The last Friday's hyper is played by hand on stream (the
 * watch path: a candidate of the player's own with the stream's face and chat), and its Nightly filmed.
 */
ss::SaveData Creator(int MaxDays)
{
	const char* Seed = "fixture-g2-creator";
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, Seed));
	S->NewCareer(Someone());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	// Night One: a little saved up from the last job (the first rent and a mid rig: 32 GB of RAM to stream, a webcam, a USB
	// mic, a ring light). After that the Wash & Fold keeps the roll over the rent.
	S->BankrollCents += 160000;
	for (const char* Id : {"ram-32", "webcam-720", "mic-usb", "ring-light"})
	{
		const std::string Why = S->Buy(Id);
		if (!Why.empty())
		{
			std::printf("fixture: the creator's %s: %s\n", Id, Why.c_str());
		}
	}
	S->SprintBudgetMs = 1.0e9;
	const ss::net::Network& Net = ss::net::Shared();
	const int FirstDay = ss::net::DayOf(S->WorldMinutes());
	// Two Nightlies at the Embercrest, skipped by the plan (TAG) with the hands noted: a Friday about two months in (its
	// hands make videos), and the last Friday (its hands still open).
	const int EarlyFriday = FirstDay + 60 - ((ss::net::Weekday(FirstDay + 60) + 7 - 4) % 7);
	int PaidDay = -1;
	int Turn = 0;
	std::array<int, 4> ByFormat{};
	bool Last = false;
	int Days = 0;
	for (int K = 0; K < MaxDays && !Last; ++K)
	{
		const int DayNo = FirstDay + K;
		Days = K + 1;
		PaidDay = PaidDay < 0 && S->Life.Video.PaidCents > 0 ? DayNo : PaidDay;
		Last = PaidDay >= 0 && DayNo >= PaidDay + 3 && ss::net::Weekday(DayNo) == 4;
		// 7 AM: a shift when the roll's under the rent and a cushion.
		Now = CreatorUntil(*S, Now, static_cast<double>(DayNo) * Day + 7.0 * 60.0);
		Fed(*S);
		if (S->BankrollCents < ss::life::MonthlyRentCents + 40000 && S->StartActivity("washfold").empty())
		{
			for (int Guard = 0; S->TimeSkip.Active && Guard < 200; ++Guard)
			{
				Now = Wait(*S, Now, 0.5);
			}
		}
		// 2 PM: a hyper on stream; the plan (TAG) skips it whole one day, Sprint plays it the next.
		Now = CreatorUntil(*S, Now, static_cast<double>(DayNo) * Day + 14.0 * 60.0);
		Fed(*S);
		ss::net::EventInstance I;
		if (S->BankrollCents >= 500 && Net.Next("hyper-sprint", S->WorldMinutes() + 1.0, I))
		{
			const bool OnAir = S->GoLive().empty();
			if (Last)
			{
				ByHand(*S, Now, I);
			}
			else if (K % 2 == 0)
			{
				PlanHyper(*S, Now, I, style::Style::Tag, 0, 0);
			}
			else
			{
				CreatorSprint(*S, Now, I);
			}
			if (OnAir)
			{
				Now = Wait(*S, Now, 0.5);
				S->EndStream();
			}
		}
		if (DayNo == EarlyFriday || Last)
		{
			ss::live::Occurrence O;
			for (const ss::live::Occurrence& X : ss::live::Occurrences(DayNo))
			{
				O = std::string(X.T->Key) == "nightly" ? X : O;
			}
			if (O.Valid() && S->BankrollCents >= O.T->BuyInCents + 30000)
			{
				Now = PlanNightToResult(S, H, Now, Seed, O.Id.c_str(), style::Style::Tag, true);
			}
		}
		// 9 PM (or home from the Embercrest): the evening's video, from day 3.
		if (K >= 3)
		{
			Now = CreatorUntil(*S, Now, static_cast<double>(DayNo) * Day + 21.0 * 60.0);
			Turn += CreatorUpload(*S, Now, Turn, ByFormat) ? 1 : 0;
		}
		if (Transcript && K % 10 == 9)
		{
			CreatorLine(*S, K + 1);
		}
	}
	// The last night: Dee's game (the next one open), played out by the host: the cooler noted for Rerun, the night's net
	// on the save and the world told, the save loaded again.
	const life::Activity* G = life::Find("dee-game");
	const double Doors = G ? life::NextOpen(*G, S->WorldMinutes()) : -1.0;
	if (Doors > 0.0)
	{
		Now = CreatorUntil(*S, Now, Doors + 30.0);
		Fed(*S);
		const std::string Why = S->GoToGame("dee-game", 20000);
		if (!Why.empty())
		{
			std::printf("fixture: Dee's game: %s\n", Why.c_str());
		}
		ss::SaveData D = HostSave(H, *S);
		const double End = Doors + 4.0 * 60.0;
		CreatorDeeHand(D.Life, Doors + 95.0);
		const ss::Chips NightNet = 18800;
		D.BankrollCents += NightNet;
		D.Life.Record(End, "Dee's game", NightNet, 0);
		D.Life.BackRoomNights += 1;
		D.Life.BackRoomNetCents += NightNet;
		D.NoteBackRoom(End, {"Sal", "Big Lou", "Twitch", "Mei"}, NightNet);
		D.ClockMinutes = End + 4.0 - ss::net::MinutesPerDay * ss::net::NightOneDay;
		S.reset();
		S.reset(new ss::Session(H, Seed, &D));
		S->CurrentScreen = ss::Screen::Lobby;
		S->ResumeCalendarFrom(End + 4.0);
		Now = Wait(*S, Now, 0.2);
	}
	// The next afternoon, fed: saved.
	Now = CreatorUntil(*S, Now, static_cast<double>(ss::net::DayOf(S->WorldMinutes()) + 1) * Day + 15.0 * 60.0);
	Fed(*S);
	Now = Wait(*S, Now, 0.2);
	S->Save();
	if (Transcript)
	{
		CreatorLine(*S, Days + 1);
		std::printf("  [creator] %d uploads: %d Shorts, %d Breakdowns, %d Vlogs, %d Recaps\n", Turn, ByFormat[0], ByFormat[1], ByFormat[2], ByFormat[3]);
	}
	return H.Last;
}
#endif

#if SS_FIXTURE_GATE3
// ---------------------------------------------------------------- update 2's gate 3 fixture (write-g3)

/** A site's event (RiverLine's or a rival's) sprinted from its start to the result, the hands Sprint hands back played with
 *  the grader's best (whole stretches an update: the same frames on any machine). False when it didn't open. */
bool MultisiteSprint(ss::Session& S, double& Now, const ss::net::Network& Net, const ss::net::EventInstance& I)
{
	if (S.WorldMinutes() < I.Start)
	{
		Now = At(S, Now, I.Start);
	}
	Fed(S);
	S.RegisterEvent(Net.Listing(I, nullptr, S.Unlocks()));
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	S.SprintBudgetMs = 1.0e9;
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		S.CurrentPace = ss::Pace::Sprint;
		Now += 1.0 / 30.0;
		S.Update(Now);
		ActBest(S, Now);
	}
	S.LeaveResults();
	return true;
}

/** A daily's next start from From to To on a site's network, its launch week's one-off included ("ignite.spark-jackpot" or
 *  "ignite.spark-jackpot-launch4": a launch week runs the guaranteed dailies as one-offs of their own). */
bool MultisiteNext(const ss::net::Network& Net, const std::string& Base, double From, double To, ss::net::EventInstance& Out)
{
	bool Found = false;
	for (const ss::net::EventInstance& I : Net.Window(From, To))
	{
		const std::string Template = I.Id.substr(0, I.Id.find('@'));
		if (I.Start >= From && (Template == Base || Template.rfind(Base + "-launch", 0) == 0) && (!Found || I.Start < Out.Start))
		{
			Out = I;
			Found = true;
		}
	}
	return Found;
}

/** The site's invitation as Mail holds it, unanswered (State 0 or 3), or null. */
const ss::site::Invitation* MultisiteOpenInvite(const ss::Session& S, ss::net::SiteId Site)
{
	const ss::site::Invitation* Out = nullptr;
	for (const ss::site::Invitation& I : S.Life.Sites.Invites)
	{
		Out = I.Site == Site && (I.State == 0 || I.State == 3) ? &I : Out;
	}
	return Out;
}

/** One line on the career (the "show" mode's progress). */
void MultisiteLine(const ss::Session& S, int K, const char* What)
{
	std::string Book;
	for (const ss::site::Account& A : S.Life.Sites.Accounts)
	{
		Book += std::string(" ") + ss::net::KeyOf(A.Site) + " " + ss::Money(A.Cents);
	}
	std::printf("  [multisite] day %3d %s: %s; bank %s, rents paid %d, %zu results, Rewards rake %s;%s\n", K, ss::net::DateLabel(ss::net::DayOf(S.WorldMinutes())).c_str(), What,
		ss::Money(S.BankrollCents).c_str(), S.Life.RentsPaid, S.History.size(), ss::Money(S.Life.Sites.RiverLine.Rake).c_str(), Book.c_str());
}

/** update2_multisite (see the top): the grind to Ignite's invitation, the sites joined, an evening on both, a withdrawal. */
ss::SaveData Multisite()
{
	namespace net = ss::net;
	const char* Seed = "fixture-g3-multisite";
	Hooks H;
	ss::Session S(H, Seed);
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	// Night One: the last job's savings (both rents and a cushion; the grind's buy-ins come out of the same bank).
	S.BankrollCents += 260000;
	const net::Network& Home = net::Shared();
	const int FirstDay = net::DayOf(S.WorldMinutes());
	int JoinedDay = -1;
	for (int K = 0; K < 70; ++K)
	{
		const int DayNo = FirstDay + K;
		// 2 PM: five Hyper Sprints at RiverLine, back to back (the rake counts toward RiverLine Rewards).
		Now = CreatorUntil(S, Now, static_cast<double>(DayNo) * Day + 14.0 * 60.0);
		for (int N = 0; N < 5 && JoinedDay < 0; ++N)
		{
			net::EventInstance I;
			if (Home.Next("hyper-sprint", S.WorldMinutes() + 1.0, I))
			{
				MultisiteSprint(S, Now, Home, I);
			}
		}
		// The second Monday: Team RiverLine's offer, signed from Mail; signed from now on, it plays only on RiverLine.
		if (K == 6)
		{
			const std::string Offered = S.DebugSponsor("offer riverline");
			const std::string Id = ss::sponsor::OpenOfferFor(S.Life.Sponsors, "riverline", S.WorldMinutes());
			const std::string Why = Id.empty() ? Offered : S.SignDeal(Id);
			if (!Why.empty() || Transcript)
			{
				std::printf("  [multisite] Team RiverLine: %s\n", Why.empty() ? "signed" : Why.c_str());
			}
		}
		// Act I over (two rents paid) and 150 paid events: Ignite's letter comes on its own at a midnight. Accepted from Mail
		// that morning with $50 by card; RunItTwice's invitation as the console sends it, accepted, $50 by card (its tickets).
		if (JoinedDay < 0 && MultisiteOpenInvite(S, net::SiteId::Ignite))
		{
			Now = CreatorUntil(S, Now, static_cast<double>(DayNo) * Day + 15.0 * 60.0);
			std::string Why = S.AnswerInvite(net::SiteId::Ignite, true);
			Why += S.Deposit(net::SiteId::Ignite, 5000, "card");
			Why += S.DebugSite("invite runittwice").find("invitation is in Mail") != std::string::npos ? std::string() : std::string("no RunItTwice letter. ");
			Why += S.AnswerInvite(net::SiteId::RunItTwice, true);
			Why += S.Deposit(net::SiteId::RunItTwice, 5000, "card");
			if (!Why.empty())
			{
				std::printf("  [multisite] joining: %s\n", Why.c_str());
			}
			JoinedDay = K;
			// That evening: the Supper Stack on a RunItTwice ticket (the strike under the clause, the first site play of the
			// day), then the Spark Jackpot on Ignite's wallet (no second strike the same day).
			const net::Network& Rit = net::Site(net::SiteId::RunItTwice);
			const net::Network& Ign = net::Site(net::SiteId::Ignite);
			const double Evening = static_cast<double>(DayNo) * Day + 17.0 * 60.0;
			net::EventInstance I;
			if (!MultisiteNext(Rit, "runittwice.supper-stack", Evening, Evening + 6.0 * 60.0, I) || !MultisiteSprint(S, Now, Rit, I))
			{
				std::printf("  [multisite] the Supper Stack didn't open\n");
			}
			if (!MultisiteNext(Ign, "ignite.spark-jackpot", Evening, Evening + 6.0 * 60.0, I) || !MultisiteSprint(S, Now, Ign, I))
			{
				std::printf("  [multisite] the Spark Jackpot didn't open\n");
			}
			if (Transcript)
			{
				MultisiteLine(S, K, "joined Ignite and RunItTwice, an evening on both");
			}
			continue;
		}
		// The next morning: $40 out of Ignite to the bank (three to five days), and saved that afternoon with it in transit.
		if (JoinedDay >= 0 && K == JoinedDay + 1)
		{
			Now = CreatorUntil(S, Now, static_cast<double>(DayNo) * Day + 10.0 * 60.0);
			const std::string Why = S.Withdraw(net::SiteId::Ignite, 4000, "bank");
			if (!Why.empty())
			{
				std::printf("  [multisite] the withdrawal: %s\n", Why.c_str());
			}
			Now = CreatorUntil(S, Now, static_cast<double>(DayNo) * Day + 15.0 * 60.0);
			Fed(S);
			Now = Wait(S, Now, 0.2);
			S.Save();
			if (Transcript)
			{
				MultisiteLine(S, K, "saved");
			}
			return H.Last;
		}
		if (Transcript && K % 5 == 4)
		{
			MultisiteLine(S, K, "the grind");
		}
	}
	std::printf("fixture: Ignite's invitation never came in 70 days\n");
	S.Save();
	return H.Last;
}
#endif

#if SS_FIXTURE_G3K
namespace kast = ss::kast;

/** The bytes a save's kast\t lines take, all of them (session_test's journeys::LineBytesKast), or those of one sub-tag. */
size_t KastBytes(const std::string& Text, const char* Tag = "kast\t")
{
	size_t Bytes = 0;
	size_t Start = 0;
	const size_t Len = std::strlen(Tag);
	while (Start < Text.size())
	{
		size_t End = Text.find('\n', Start);
		End = End == std::string::npos ? Text.size() : End + 1;
		Bytes += Text.compare(Start, Len, Tag) == 0 ? End - Start : 0;
		Start = End;
	}
	return Bytes;
}

/** The stream kit and a little saved up (the creator's Night One): 32 GB of RAM to stream, a webcam, a USB mic, a ring light. */
void StreamKit(ss::Session& S)
{
	S.BankrollCents += 160000;
	for (const char* Id : {"ram-32", "webcam-720", "mic-usb", "ring-light"})
	{
		const std::string Why = S.Buy(Id);
		if (!Why.empty())
		{
			std::printf("fixture: the streamer's %s: %s\n", Id, Why.c_str());
		}
	}
}

/** Whether this build's Kast runs the creator update's model: a new career's first stream sets life::U3Kast (Session::
 *  KastAfterStart, core-kastglue's). Phase A-K's stubs leave it unset, so write-g3k writes nothing yet. */
bool KastModelBuilt()
{
	Hooks H;
	ss::Session S(H, "fixture-g3k-probe");
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	StreamKit(S);
	const bool OnAir = S.GoLive().empty();
	Now = Wait(S, Now, 0.2);
	const bool Model = (S.Life.Update3LinesRead & life::U3Kast) != 0;
	if (OnAir)
	{
		S.EndStream();
	}
	(void)Now;
	return OnAir && Model;
}

/** One line on the streamer's channel (the "show" mode's progress), its Kast lines measured on a save taken now. */
void StreamerLine(ss::Session& S, const Hooks& H, int K)
{
	const kast::Channel& C = S.Channel;
	S.Save();
	const std::string Text = H.Last.Serialize();
	std::printf("  [streamer] day %3d %s: %5d followers, %4zu members (%d regulars), %3d subs, %4d streams, average %.1f, %s paid; Kast lines %zu bytes; bank %s\n", K,
		ss::net::DateLabel(ss::net::DayOf(S.WorldMinutes())).c_str(), C.Followers, C.Members.size(), C.Regulars(), C.ActiveSubs(S.WorldMinutes()), C.Streams,
		C.AvgViewers(), ss::Money(C.PaidCents).c_str(), KastBytes(Text), ss::Money(S.BankrollCents).c_str());
}

/**
 * update3_streamer (see the top): five evenings a week on the posted schedule, three world hours a night around the evening's
 * hyper (sprinted), until the community reaches the member cap or MaxDays have passed; then saved the next afternoon.
 */
ss::SaveData Streamer3K(int MaxDays)
{
	Hooks H;
	std::unique_ptr<ss::Session> S(new ss::Session(H, "fixture-g3k-streamer"));
	S->NewCareer(Someone());
	S->CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(*S, 0.0, 0.2);
	StreamKit(*S);
	S->StreamSchedule(0x1f, 19 * 60);
	S->SprintBudgetMs = 1.0e9;
	const ss::net::Network& Net = ss::net::Shared();
	const int FirstDay = ss::net::DayOf(S->WorldMinutes());
	int K = 0;
	for (; K < MaxDays && static_cast<int>(S->Channel.Members.size()) < kast::MaxMembers; ++K)
	{
		const int DayNo = FirstDay + K;
		// 7 AM: a shift when the roll's under the rent and a cushion.
		Now = CreatorUntil(*S, Now, static_cast<double>(DayNo) * Day + 7.0 * 60.0);
		Fed(*S);
		if (S->BankrollCents < life::MonthlyRentCents + 40000 && S->StartActivity("washfold").empty())
		{
			for (int Guard = 0; S->TimeSkip.Active && Guard < 200; ++Guard)
			{
				Now = Wait(*S, Now, 0.5);
			}
		}
		// Monday to Friday, 7 PM: live for three world hours, the evening's hyper sprinted when there's one in the first two.
		if (ss::net::Weekday(DayNo) < 5)
		{
			const double Air = static_cast<double>(DayNo) * Day + 19.0 * 60.0;
			Now = CreatorUntil(*S, Now, Air);
			Fed(*S);
			if (S->GoLive().empty())
			{
				ss::net::EventInstance I;
				if (S->BankrollCents >= 500 && Net.Next("hyper-sprint", S->WorldMinutes() + 1.0, I) && I.Start < Air + 120.0)
				{
					CreatorSprint(*S, Now, I);
				}
				Now = CreatorUntil(*S, Now, Air + 180.0);
				Now = Wait(*S, Now, 0.5);
				S->EndStream();
			}
		}
		if (Transcript && K % 10 == 9)
		{
			StreamerLine(*S, H, K + 1);
		}
	}
	// Saved the next afternoon.
	Now = CreatorUntil(*S, Now, static_cast<double>(FirstDay + K) * Day + 15.0 * 60.0);
	Fed(*S);
	Now = Wait(*S, Now, 0.2);
	if (Transcript)
	{
		StreamerLine(*S, H, K);
	}
	S->Save();
	return H.Last;
}

/** A handle of exactly Len characters, unique by N (the community's names run 5-17). */
std::string WorstName(const char* Stem, int N, size_t Len)
{
	std::string Name = std::string(Stem) + "_" + std::to_string(N);
	Name.insert(std::strlen(Stem), Len > Name.size() ? Len - Name.size() : 0, 'x');
	return Name.substr(0, Len);
}

/** kast\tmo in §8.1's field order, at the view's values: the projection's line while growth::WriteLines is a stub (the real
 *  format is core-kastmath's; this one spends the digits a full line would). */
std::string ProjectedMo(const kast::growth::SaveView& V)
{
	std::string Out = "kast\tmo\t" + std::to_string(V.Version);
	for (const double X : {V.Heat, V.HeatAt, V.Norm})
	{
		Out += "\t" + ss::Fixed(X, 2);
	}
	Out += "\t" + ss::Fixed(V.Bias, 4) + "\t" + ss::Fixed(V.RunNorm, 4) + "\t" + std::to_string(V.BestAvg) + "\t" + ss::Fixed(V.PingAt, 2);
	for (const int X : {V.LastStreamDay, V.ShowWeek, V.ShowDays, V.GainWeek})
	{
		Out += "\t" + std::to_string(X);
	}
	Out += "\t" + ss::Fixed(V.GainUsed, 2) + "\t" + std::to_string(V.FtBase) + "\t" + std::to_string(V.FtDays) + "\t" + std::to_string(V.LiveDay);
	for (const int M : V.LiveMin)
	{
		Out += "\t" + std::to_string(M);
	}
	Out += "\t" + std::to_string(V.GrantDay) + "\t" + std::to_string(V.GrantMask) + "\t" + (V.WelcomeRaidDone ? "1" : "0") + "\t" + std::to_string(V.CotnDay) + "\t" +
		std::to_string(V.PendingCents) + "\t" + ss::Fixed(V.LiveSince, 2) + "\t" + ss::Fixed(V.GainSoFar, 2);
	for (const double X : {V.CarryFollow, V.CarryWord, V.CarrySub, V.CarryTip, V.CarryCheer, V.CarryRaid, V.CarryGift})
	{
		Out += "\t" + ss::Fixed(X, 4);
	}
	std::string Heat;
	for (const std::pair<double, double>& B : V.RerunHeat)
	{
		Heat += (Heat.empty() ? "" : ";") + ss::Fixed(B.first, 2) + "," + ss::Fixed(B.second, 1);
	}
	// Firsts last (the contract review's addition to §8.1: the one-time stings' bits).
	return Out + "\t" + Heat + "\t" + std::to_string(V.Firsts) + "\n";
}

/**
 * kast-worst (see the top): the synthetic worst save. A new career's save moved two years on (7-digit world minutes), its
 * channel filled to every bound: 1,500 members with 17-character handles, 60 log rows under the longest title, 300 subs
 * (lapsed ones' renewals negative), 16 clips with their candidates, 60 days live, 36 goodwill names, 6 mods. SPEC_CREATORS
 * §8.2's worst case gives the numbers the widths REVIEW_ENGINEERING's kast_bytes.py gave them (a member's streams and
 * messages three digits, what they gave four, a log row's average three); Widest gives every number the widest it can
 * plausibly reach two years in (every member a superfan of 9,999 streams). Prints the Kast lines as today writes them, what
 * this build adds once U3Kast is set (only the flags' second digit while the writers are Phase A-K's stubs), and the creator
 * update's new lines projected in §8.1's formats; true when the whole is within §8.2's 180 KB.
 */
bool KastWorst(bool Widest)
{
	Hooks H;
	ss::Session S(H, "fixture-kast-worst");
	S.NewCareer(Someone());
	S.CurrentScreen = ss::Screen::Lobby;
	Wait(S, 0.0, 0.2);
	S.Save();
	ss::SaveData D = H.Last;
	const int Today = 760;
	const double World = static_cast<double>(Today) * Day + 23.0 * 60.0 + 59.75;
	D.ClockMinutes = World - ss::net::MinutesPerDay * static_cast<double>(ss::net::NightOneDay);
	const double Long = World - 0.25; // a world minute with all seven digits and both decimals
	kast::Channel& K = D.Channel;
	K = kast::Channel();
	K.Title = 0;
	std::string LongestTitle;
	for (const kast::TitleSpec& T : kast::Titles())
	{
		LongestTitle = T.Text.size() > LongestTitle.size() ? T.Text : LongestTitle;
	}
	K.Followers = 999999;
	K.FollowFrac = 0.9999;
	K.MinutesLive = 999999.99;
	K.ViewerMinutes = 99999999.9;
	K.Peak = 99999;
	K.Streams = 9999;
	K.Affiliate = true;
	K.Partner = true;
	K.Milestone = 99;
	K.LastOffline = Long;
	K.GiftedSubs = 9999;
	K.RaidsIn = 9999;
	K.EarnedSubs = K.EarnedBits = K.EarnedTips = K.EarnedAds = K.EarnedSponsors = 99999999;
	K.UnpaidCents = 9999999;
	K.PaidCents = 99999999;
	for (int N = 0; N < 300; ++N)
	{
		kast::Subscriber Sb;
		Sb.Name = WorstName("subscriber", N, 17);
		Sb.Since = Long;
		Sb.Renews = -Long;
		Sb.Months = 24;
		Sb.Gift = true;
		K.Subs.push_back(Sb);
	}
	for (int N = 0; N < kast::Stream::MaxMods; ++N)
	{
		kast::Moderator M;
		M.Name = WorstName("moderator", N, 17);
		M.Since = Long;
		M.Actions = 9999;
		M.Online = 0.999;
		K.Mods.push_back(M);
	}
	for (int N = 0; N < 16; ++N)
	{
		kast::Clip Cl;
		Cl.At = Long;
		Cl.Views = 9999999.9;
		Cl.Reach = 9999999.9;
		Cl.Kind = static_cast<int>(kast::Moment::Count) - 1;
		Cl.By = WorstName("clipper", N, 17);
		Cl.Title = LongestTitle;
		Cl.Candidate = "kast:" + std::to_string(static_cast<long long>(Long)) + ":" + std::to_string(N) + ":finaltable";
		Cl.Lane = true; // C-K's L11: the Clip Lane flag, kast\tclip's 10th field (+2 B a clip)
		K.Clips.push_back(Cl);
	}
	for (int N = 0; N < 60; ++N)
	{
		kast::StreamLog Lg;
		Lg.Start = Long;
		Lg.Minutes = 1439.99;
		Lg.Avg = Widest ? 9999 : 999;
		Lg.Peak = Widest ? 99999 : 9999;
		Lg.Follows = Widest ? 9999 : 999;
		Lg.Subs = Widest ? 999 : 99;
		Lg.Cents = Widest ? 9999999 : 999999;
		Lg.Title = LongestTitle;
		Lg.Returning = Widest ? 1500 : 999;
		Lg.OnSchedule = true;
		K.Log.push_back(Lg);
	}
	for (int N = 0; N < kast::MaxMembers; ++N)
	{
		kast::Member M;
		M.Name = WorstName("member", N, 17);
		M.Loyalty = 0.9999;
		M.Affinity = 0.9999;
		M.FirstSeen = Long;
		M.LastSeen = Long;
		M.Streams = Widest ? 9999 : 999;
		M.WatchMinutes = 99999.9;
		M.Messages = Widest ? 99999 : 999;
		M.Follower = true;
		M.Friend = N < 2;
		M.Given = Widest ? 999999 : 9999;
		K.Members.push_back(M);
	}
	K.ScheduleDays = 0x7f;
	K.ScheduleStart = 1439;
	K.Xp = 9999999.9;
	K.Streak = 999;
	K.Stage = 9;
	K.Week = Today / 7;
	K.WeekStreams = 99;
	K.WeekMinutes = 9999.9;
	K.WeekAnswers = 999;
	K.WeekRegulars = 999;
	K.WeekRewarded = 255;
	K.ProcessedDay = Today;
	K.RaidsOut = 9999;
	for (int N = 0; N < 60; ++N)
	{
		K.DaysLive.push_back(Today - N);
	}
	for (int N = 0; N < 36; ++N)
	{
		K.Goodwill[WorstName("smallchannel", N, 17)] = 99;
	}
	// Today's Kast lines at the cap (U3Kast unset: what every save writes now).
	D.Life.Update3LinesRead &= ~static_cast<uint32_t>(life::U3Kast);
	const std::string Before = D.Serialize();
	// The creator update's state at its widest, and U3Kast set: what this build writes then.
	K.Heat = 100.0;
	K.HeatAt = Long;
	K.Norm = 9999.99;
	K.Bias = -1.2345;
	K.RunNorm = 99.9999;
	K.BestAvg = 99999;
	K.PingAt = Long;
	K.LastStreamDay = Today;
	K.ShowWeek = Today / 7;
	K.ShowDays = 127;
	K.GainWeek = Today / 7;
	K.GainUsed = 35.0;
	K.FtBase = Today;
	K.FtDays = 127;
	K.LiveDay = Today;
	K.LiveMin.fill(1440);
	K.GrantDay = Today;
	K.GrantMask = 511;
	K.WelcomeRaidDone = true;
	K.CotnDay = Today;
	K.PendingCents = 9999999;
	K.LiveSince = Long;
	K.GainSoFar = -99.99;
	K.CarryFollow = K.CarryWord = K.CarrySub = K.CarryTip = K.CarryCheer = K.CarryRaid = K.CarryGift = 0.9999;
	for (int N = 0; N < 4; ++N)
	{
		K.RerunHeat.push_back({Long, 12.0});
	}
	for (kast::Member& M : K.Members)
	{
		M.Bonded = true;
		M.Noticed = true;
		M.GaveDay = Today;
		M.Hw = 99.9;
		M.HwDay = Today;
	}
	for (kast::StreamLog& Lg : K.Log)
	{
		Lg.AvgOwn = 9999;
		Lg.PeakAt = 1439.5;
		Lg.PeakMoment = static_cast<int>(kast::Moment::Count) - 1;
		Lg.HeatAfter = 100.0;
	}
	for (const auto& G : K.Goodwill)
	{
		K.RaidedDay[G.first] = Today;
	}
	D.Life.Update3LinesRead |= life::U3Kast;
	const std::string After = D.Serialize();
	// §8.1's new lines at these values, while their writers are stubs: kast\tmo whole; per member the flags' second digit
	// (Bonded 4, Noticed 8: "13" for "1") and "\t<GaveDay>\t<Hw tenths>\t<HwDay>"; per log row "\t<AvgOwn>\t<PeakAt>\t
	// <PeakMoment>\t<HeatAfter>"; per goodwill name "\t<LastRaidDay>".
	const std::string DayText = std::to_string(Today);
	const size_t MoBytes = ProjectedMo(kast::SaveViewOf(K)).size();
	const size_t MemBytes = 1 + (1 + DayText.size()) + (1 + std::to_string(static_cast<int>(std::lround(99.9 * 10.0))).size()) + (1 + DayText.size());
	const size_t LogBytes = (1 + std::string("9999").size()) + (1 + ss::Fixed(1439.5, 1).size()) + (1 + std::to_string(static_cast<int>(kast::Moment::Count) - 1).size()) +
		(1 + ss::Fixed(100.0, 1).size());
	const size_t GoodBytes = 1 + DayText.size();
	const size_t Projected = MoBytes + K.Members.size() * MemBytes + K.Log.size() * LogBytes + K.Goodwill.size() * GoodBytes;
	const size_t Today0 = KastBytes(Before);
	const size_t Built = KastBytes(After);
	const size_t Budget = 180 * 1024;
	std::printf("kast-worst (%s): the synthetic worst save, day %d (world minute %s): %zu members, %zu log rows, %zu subs, %zu clips, %zu days, %zu goodwill, %zu mods\n",
		Widest ? "every number at its widest" : "SPEC_CREATORS 8.2's worst case, kast_bytes.py's widths", Today, ss::Fixed(World, 2).c_str(), K.Members.size(), K.Log.size(),
		K.Subs.size(), K.Clips.size(), K.DaysLive.size(), K.Goodwill.size(), K.Mods.size());
	std::printf("  today's Kast lines: %zu bytes (%.1f KB): mem %zu, log %zu, sub %zu, clip %zu, goodwill %zu, day %zu, mod %zu, ch+earn+growth %zu\n", Today0, Today0 / 1024.0,
		KastBytes(Before, "kast\tmem\t"), KastBytes(Before, "kast\tlog\t"), KastBytes(Before, "kast\tsub\t"), KastBytes(Before, "kast\tclip\t"), KastBytes(Before, "kast\tgoodwill\t"),
		KastBytes(Before, "kast\tday\t"), KastBytes(Before, "kast\tmod\t"),
		KastBytes(Before, "kast\tch\t") + KastBytes(Before, "kast\tearn\t") + KastBytes(Before, "kast\tgrowth\t"));
	std::printf("  this build with U3Kast set: %zu bytes (+%zu: the flags' second digit%s)\n", Built, Built - Today0,
		Built - Today0 > K.Members.size() ? " and the writers' lines" : "; the writers are stubs");
	std::printf("  projected new lines (SPEC_CREATORS §8.1): kast\\tmo %zu, member tails %zu x %zu = %zu, log tails %zu x %zu = %zu, goodwill tails %zu x %zu = %zu: %zu bytes\n",
		MoBytes, K.Members.size(), MemBytes, K.Members.size() * MemBytes, K.Log.size(), LogBytes, K.Log.size() * LogBytes, K.Goodwill.size(), GoodBytes,
		K.Goodwill.size() * GoodBytes, Projected);
	// Once the writers are real (core-kastmath's growth::WriteLines and the tails write more than the flags' digit), the build's
	// own bytes decide; the projection stays printed beside them.
	const bool Real = Built - Today0 > K.Members.size();
	const size_t Total = Real ? Built : std::max(Built, Today0 + Projected);
	const size_t MemberBytes = Real ? KastBytes(After, "kast\tmem\t") : KastBytes(Before, "kast\tmem\t") + K.Members.size() * MemBytes;
	std::printf("  LineBytesKast at the worst case (%s): %zu bytes (%.1f KB) against the 180 KB budget (%zu): %s; the members alone %.1f KB with their tails\n",
		Real ? "the writers' own bytes" : "projected", Total, Total / 1024.0, Budget, Total <= Budget ? "within it" : "OVER IT", MemberBytes / 1024.0);
	return Total <= Budget;
}
#endif

bool WriteFile(const std::string& Path, const std::string& Text)
{
	FILE* F = std::fopen(Path.c_str(), "wb");
	if (!F)
	{
		std::printf("can't write %s\n", Path.c_str());
		return false;
	}
	std::fwrite(Text.data(), 1, Text.size(), F);
	std::fclose(F);
	std::printf("wrote %s (%zu bytes)\n", Path.c_str(), Text.size());
	return true;
}

/** Loads a save into a session and plays two days on, the calendar running (what each hook site does on the way). */
void PlayOn(const char* Tag, const ss::SaveData& Saved)
{
	Hooks H;
	ss::SaveData D;
	if (!ss::SaveData::Parse(Saved.Serialize(), D))
	{
		std::printf("%s: doesn't parse\n", Tag);
		return;
	}
	ss::Session S(H, std::string("fixture-on-") + Tag, &D);
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	S.Save();
	Dump((std::string(Tag) + ": loaded").c_str(), H.Last);
	Now = Walk(S, Now, S.WorldMinutes() + 2.0 * Day);
	S.Save();
	Dump((std::string(Tag) + ": two days on").c_str(), H.Last);
	(void)Now;
}
} // namespace fixture_tool

int main(int Argc, char** Argv)
{
	using namespace fixture_tool;
	const std::string Mode = Argc > 1 ? Argv[1] : "";
	if (Mode != "write" && Mode != "write2" && Mode != "write-g1" && Mode != "write-g2" && Mode != "write-g3" && Mode != "write-g3k" && Mode != "g3k-dry" && Mode != "kast-worst" &&
		Mode != "transcript")
	{
		std::printf("usage: fixture_tool write <dir> | fixture_tool write2 <dir> | fixture_tool write-g1 <dir> [show] | fixture_tool write-g2 <dir> [show] | fixture_tool write-g3 <dir> "
					"[show] | fixture_tool write-g3k <dir> [show] | fixture_tool g3k-dry <days> | fixture_tool kast-worst | fixture_tool transcript\n");
		return 2;
	}
	Transcript = Mode == "transcript";
	if (Mode == "write-g3k" || Mode == "g3k-dry" || Mode == "kast-worst")
	{
#if SS_FIXTURE_G3K
		if (Mode == "kast-worst")
		{
			// §8.2's worst case decides; the widest case is printed beside it.
			const bool Within = KastWorst(false);
			KastWorst(true);
			return Within ? 0 : 1;
		}
		if (Mode == "g3k-dry")
		{
			// The career on whatever Kast this build has, printed every ten days; nothing written.
			Transcript = true;
			const ss::SaveData D = Streamer3K(Argc > 2 ? std::max(1, std::atoi(Argv[2])) : 30);
			std::printf("g3k-dry: %zu bytes of Kast lines in the save (%zu members); nothing written\n", KastBytes(D.Serialize()), D.Channel.Members.size());
			return 0;
		}
		if (Argc < 3)
		{
			std::printf("usage: fixture_tool write-g3k <dir> [show]\n");
			return 2;
		}
		if (!KastModelBuilt())
		{
			std::printf("fixture_tool write-g3k: this build's Kast doesn't run the creator update's model yet (a first stream sets no U3Kast): nothing written\n");
			return 2;
		}
		// "show" prints the channel every ten days as the career is built.
		Transcript = Argc > 3 && std::string(Argv[3]) == "show";
		const std::string Dir = std::string(Argv[2]) + "/";
		return WriteFile(Dir + "update3_streamer.ssave", WithoutWorld(Streamer3K(730).Serialize())) ? 0 : 1;
#else
		std::printf("fixture_tool %s: this tree has no creator update (Game/KastGrowth.h)\n", Mode.c_str());
		return 2;
#endif
	}
	if (Mode == "write-g3")
	{
#if SS_FIXTURE_GATE3
		if (Argc < 3)
		{
			std::printf("usage: fixture_tool write-g3 <dir> [show]\n");
			return 2;
		}
		// "show" prints the career every five days and the site book as it goes, for the lead's PIE notes.
		Transcript = Argc > 3 && std::string(Argv[3]) == "show";
		const std::string Dir = std::string(Argv[2]) + "/";
		return WriteFile(Dir + "update2_multisite.ssave", WithoutWorld(Multisite().Serialize())) ? 0 : 1;
#else
		std::printf("fixture_tool write-g3: this tree has no rival sites (update 2 gate 3)\n");
		return 2;
#endif
	}
	if (Mode == "write-g2")
	{
#if SS_FIXTURE_GATE2
		if (Argc < 3)
		{
			std::printf("usage: fixture_tool write-g2 <dir> [show]\n");
			return 2;
		}
		// "show" prints the channel every ten days as the career is built, for the lead's PIE notes.
		Transcript = Argc > 3 && std::string(Argv[3]) == "show";
		const std::string Dir = std::string(Argv[2]) + "/";
		return WriteFile(Dir + "update2_creator.ssave", WithoutWorld(Creator(420).Serialize())) ? 0 : 1;
#else
		std::printf("fixture_tool write-g2: this tree has no Rerun (update 2 gate 2)\n");
		return 2;
#endif
	}
	if (Mode == "write-g1")
	{
#if SS_FIXTURE_GATE1
		if (Argc < 3)
		{
			std::printf("usage: fixture_tool write-g1 <dir> [show]\n");
			return 2;
		}
		// "show" prints the careers as they're built (the transcript's dump), for the lead's PIE notes.
		Transcript = Argc > 3 && std::string(Argv[3]) == "show";
		const std::string Dir = std::string(Argv[2]) + "/";
		bool Ok = WriteFile(Dir + "update2_blank_world.ssave", BlankWorld().Serialize());
		Ok = WriteFile(Dir + "update2_auto_mid_nightly.ssave", WithoutWorld(AutoMidNightly().Serialize())) && Ok;
		return Ok ? 0 : 1;
#else
		std::printf("fixture_tool write-g1: this tree has no autoplay (update 2 gate 1)\n");
		return 2;
#endif
	}
	if (Mode == "write2")
	{
		if (Argc < 3)
		{
			std::printf("usage: fixture_tool write2 <dir>\n");
			return 2;
		}
		const std::string Dir = std::string(Argv[2]) + "/";
		std::string World2;
		const std::string Staked = WithoutWorld(StakedMidNightly().Serialize(), &World2);
		bool Ok = WriteFile(Dir + "pre_update2_fresh.ssave", WithoutWorld(Fresh2().Serialize()));
		Ok = WriteFile(Dir + "pre_update2_staked_mid_nightly.ssave", Staked) && Ok;
		Ok = WriteFile(Dir + "pre_update2_sponsored_streamer.ssave", WithoutWorld(SponsoredStreamer().Serialize())) && Ok;
		Ok = WriteFile(Dir + "pre_update2_evicted.ssave", WithoutWorld(Evicted2().Serialize())) && Ok;
		Ok = WriteFile(Dir + "pre_update2_veteran.ssave", WithoutWorld(Veteran().Serialize())) && Ok;
		Ok = WriteFile(Dir + "world_v2.txt", World2) && Ok;
		return Ok ? 0 : 1;
	}
	const ss::SaveData FreshSave = Fresh();
	const ss::SaveData Mid = MidNightly();
	const ss::SaveData Kast = TeamRiverLine();
	const ss::SaveData Locked = Evicted();
	std::string World;
	const std::string MidText = WithoutWorld(Mid.Serialize(), &World);
	const std::string V1 = AsV1(MidText);
	ss::SaveData FromV1;
	const std::string V1Expected = ss::SaveData::Parse(V1, FromV1) ? FromV1.Serialize() : std::string("(no parse)\n");
	if (Transcript)
	{
		std::printf("==== v1 read and written again:\n%s", V1Expected.c_str());
		PlayOn("fresh", FreshSave);
		PlayOn("mid_nightly", Mid);
		PlayOn("team_riverline", Kast);
		PlayOn("evicted", Locked);
		// Tonight's night settled by the host (busted 30th), the night noted, the save loaded again.
		ss::SaveData D = Mid;
		const live::Occurrence O = live::FindOccurrence("embercrest-nightly@9");
		if (const life::LiveEntry* E = live::EntryFor(D.Life, O.Id))
		{
			const double End = E->CheckpointAt + 60.0;
			const int Field = E->Entrants;
			live::Settle(D.BankrollCents, D.Life, O.Id, 30, Field, 0, End);
			live::PayFare(D.BankrollCents, D.Life, O.Id, true, End + 5.0);
			D.NoteLive(End, O.Id, 30, 0, Field, {});
			D.ClockMinutes = End + 30.0 - Day;
			PlayOn("mid_nightly settled", D);
		}
		// Update 2's day-0 careers (write2), built and played on the same way.
		const ss::SaveData Fresh2Save = Fresh2();
		const ss::SaveData Staked = StakedMidNightly();
		const ss::SaveData Streamer = SponsoredStreamer();
		const ss::SaveData Locked2 = Evicted2();
		const ss::SaveData Vet = Veteran();
		PlayOn("update2_fresh", Fresh2Save);
		PlayOn("update2_staked_mid_nightly", Staked);
		PlayOn("update2_sponsored_streamer", Streamer);
		PlayOn("update2_evicted", Locked2);
		PlayOn("update2_veteran", Vet);
		return 0;
	}
	if (Argc < 3)
	{
		std::printf("usage: fixture_tool write <dir>\n");
		return 2;
	}
	const std::string Dir = std::string(Argv[2]) + "/";
	bool Ok = WriteFile(Dir + "pre_segment_fresh.ssave", WithoutWorld(FreshSave.Serialize()));
	Ok = WriteFile(Dir + "pre_segment_mid_nightly.ssave", MidText) && Ok;
	Ok = WriteFile(Dir + "pre_segment_team_riverline.ssave", WithoutWorld(Kast.Serialize())) && Ok;
	Ok = WriteFile(Dir + "pre_segment_evicted.ssave", WithoutWorld(Locked.Serialize())) && Ok;
	Ok = WriteFile(Dir + "pre_segment_v1.ssave", V1) && Ok;
	Ok = WriteFile(Dir + "pre_segment_v1.expected.ssave", V1Expected) && Ok;
	Ok = WriteFile(Dir + "world_v1.txt", World) && Ok;
	return Ok ? 0 : 1;
}
