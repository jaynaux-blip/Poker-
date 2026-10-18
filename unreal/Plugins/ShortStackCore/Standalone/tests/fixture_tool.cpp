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
//   fixture_tool write <dir>   writes the fixtures (the saves without their world lines, which go in world_v1.txt;
//                              <name>.expected.ssave is what loading and saving the v1 save gives)
//   fixture_tool write2 <dir>  writes update 2's day-0 fixtures (the saves without their world lines; world_v2.txt)
//   fixture_tool write-g1 <dir>  writes update 2's gate 1 fixtures (deterministic: two writes are byte for byte the same)
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

#include <algorithm>
#include <cstdio>
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
	}
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
double PlanNightToResult(std::unique_ptr<ss::Session>& S, Hooks& H, double Now, const char* Seed, const char* Id, style::Style St)
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
	if (Mode != "write" && Mode != "write2" && Mode != "write-g1" && Mode != "transcript")
	{
		std::printf("usage: fixture_tool write <dir> | fixture_tool write2 <dir> | fixture_tool write-g1 <dir> [show] | fixture_tool transcript\n");
		return 2;
	}
	Transcript = Mode == "transcript";
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
