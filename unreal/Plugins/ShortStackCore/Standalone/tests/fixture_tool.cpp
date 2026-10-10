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
//   fixture_tool write <dir>   writes the fixtures (the saves without their world lines, which go in world_v1.txt;
//                              <name>.expected.ssave is what loading and saving the v1 save gives)
//   fixture_tool transcript    prints every save as the careers are built and as each fixture is loaded and played on
//                              (texts, ledgers, the whole save but the world, whose size and hash are printed); a build
//                              whose transcript matches c09bc31's behaves the same
// Writes and reads in binary: the saves are LF, whatever the platform.
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Store.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
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
	if (Mode != "write" && Mode != "transcript")
	{
		std::printf("usage: fixture_tool write <dir> | fixture_tool transcript\n");
		return 2;
	}
	Transcript = Mode == "transcript";
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
