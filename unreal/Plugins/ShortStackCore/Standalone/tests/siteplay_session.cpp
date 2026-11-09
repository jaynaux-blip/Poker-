// site_play: the Session's side of the sites' loops (update 3 SPEC §7.7; core-siteplay's file): the claims through Session,
// SitePlayStep over overlapping windows and WorldSkip, the strike gate (RegisterEvent refuses without the token on any
// flagged site, returns why, changes nothing, and strikes once with it), the save's round trip, and no write into an
// untouched save (S10). Called from siteplay_main.cpp (the kit is SitePlaySeg.h).
#include "SitePlaySeg.h"

#include "ShortStack/AI/Style.h"
#include "ShortStack/Game/Autoplay.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Tournament.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace siteplay_test
{
namespace session_seg
{
namespace play = ss::site::play;
namespace net = ss::net;
using ss::Chips;

struct Hooks : ss::SessionHooks
{
	int Saves = 0;
	ss::SaveData Last;
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
};

double Jump(ss::Session& S, double Now, double ToWorld)
{
	if (ToWorld > S.WorldMinutes())
	{
		S.LobbyMinutes += ToWorld - S.WorldMinutes();
	}
	Now += 1.0 / 30.0;
	S.Update(Now);
	return Now;
}

/** An Ignite event to pay for now, a cheap one (registering or in late registration). */
bool IgniteEvent(const ss::Session& S, ss::LobbyEvent& Out)
{
	const double World = S.WorldMinutes();
	bool Found = false;
	for (const net::EventInstance& I : net::Site(net::SiteId::Ignite).Window(World - 60.0, World + 120.0))
	{
		const ss::LobbyEvent L = net::Site(net::SiteId::Ignite).Listing(I, nullptr, S.Unlocks());
		if (L.Joinable && L.BuyInCents > 0 && L.BuyInCents <= 550 && (!Found || L.BuyInCents < Out.BuyInCents))
		{
			Out = L;
			Found = true;
		}
	}
	return Found;
}

bool HasPlayLine(const std::string& Text)
{
	for (const char* Tag : {"site\tspark\t", "site\tflame\t", "site\tdare\t", "site\tgov\t", "site\thold\t", "site\tleague\t", "site\tstar\t", "site\tbox\t"})
	{
		if (Text.find(Tag) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

/** Opens Ignite's account as if the invitation was accepted, with money on it. */
void OpenIgnite(ss::Session& S)
{
	ss::site::Join(S.Life.Sites, net::SiteId::Ignite, S.WorldMinutes());
	*ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite) = 50000;
}

void Untouched()
{
	// S10: a career that never pays a rival entry writes no play line, whatever its pages draw.
	Hooks H;
	ss::Session S(H, "siteplay-untouched");
	double Now = Jump(S, 0.0, 70.0 * 1440.0 + 20.0 * 60.0);
	OpenIgnite(S);
	ss::site::Join(S.Life.Sites, net::SiteId::RunItTwice, S.WorldMinutes());
	for (const net::SiteId Site : {net::SiteId::Ignite, net::SiteId::RunItTwice})
	{
		const play::View V = S.SitePlayView(Site);
		(void)V;
	}
	(void)S.DebugSitePlay("describe");
	(void)S.DebugSitePlay("gov");
	(void)S.DebugSitePlay("dares");
	Now = Jump(S, Now, S.WorldMinutes() + 40.0 * 1440.0);
	S.Save();
	const std::string Text = H.Last.Serialize();
	Expect(!Text.empty() && !HasPlayLine(Text), "session: an untouched book writes no play line after 40 days of views and steps");
	Expect(!S.SpinWheel().empty() && !S.ClaimDare(0).empty() && !S.OpenCracker("*").empty(), "session: claims on an empty book are refused and create nothing");
	S.Save();
	Expect(!HasPlayLine(H.Last.Serialize()), "session: still nothing written after the refused claims");
}

void PaidAndClaimed()
{
	Hooks H;
	ss::Session S(H, "siteplay-paid");
	double Now = Jump(S, 0.0, 63.0 * 1440.0 + 19.0 * 60.0); // Monday, December 7, 7 PM
	OpenIgnite(S);
	S.BankrollCents = 100000;
	ss::LobbyEvent Ev;
	if (!IgniteEvent(S, Ev))
	{
		Expect(false, "session: an Ignite event to register for at 7 PM");
		return;
	}
	const std::string Why = S.RegisterEvent(Ev);
	const play::Book& B = S.Life.SitePlay;
	const Chips Fee = Ev.Spec.FeeCents;
	Expect(Why.empty() && B.Ig.S.BaseLife >= 10 * Fee && B.Ig.F.Streak == 1 && B.Ig.S.DayEntries == 1 && play::PaidEntry(S.Life, Ev.Spec.Id),
		"session: a paid Ignite entry through RegisterEvent starts the book: Sparks, the Flame, the day, the paid marker (" + Ev.Spec.Name + ")");
	const int Saves = H.Saves;
	Expect(S.SpinWheel().empty() && H.Saves > Saves, "session: SPIN takes GLOW and autosaves at the press");
	Expect(!S.SpinWheel().empty(), "session: a second spin is refused");
	const play::View V = S.SitePlayView(net::SiteId::Ignite);
	Expect(V.Ig.Started && V.Ig.Spun && V.Ig.SpunRing == 1 && V.Ig.SpunWedge >= 0 && V.Gov.Rake30 == Fee, "session: the view reads the spin and the ring");
	S.Save();
	const std::string Text = H.Last.Serialize();
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(Text, Back) && Back.Serialize() == Text && Text.find("site\tspark\tignite\t") != std::string::npos && Text.find("site\tgov\tignite\t") != std::string::npos,
		"session: the save holds the play lines and writes back byte for byte");
	Hooks H2;
	ss::Session R(H2, "siteplay-paid", &Back);
	const play::View Rv = R.SitePlayView(net::SiteId::Ignite);
	Expect(Rv.Ig.SpunWedge == V.Ig.SpunWedge && Rv.Ig.Sparks == V.Ig.Sparks && Rv.Ig.Streak == V.Ig.Streak && Rv.Ig.Dares[0].Id == V.Ig.Dares[0].Id,
		"session: a reload replays the same wedge, Sparks, Flame and Dares");
	// The Governor's dev verbs (§7.6): a grant past the cap waits; dev rake covers it at the next step.
	const std::string Held = S.DebugSitePlay("grant ignite 150");
	Expect(Held.find("held on the Guest List") != std::string::npos, "session: ss.Site.Play grant ignite 150 waits on the Guest List: " + Held);
	S.DebugSitePlay("rake ignite 400");
	Now = Jump(S, Now, S.WorldMinutes() + 1440.0);
	const play::View G = S.SitePlayView(net::SiteId::Ignite);
	bool Waiting = false;
	for (const play::HoldView& Hv : G.Gov.Holds)
	{
		Waiting = Waiting || Hv.What == "Grant $1.50";
	}
	Expect(!Waiting, "session: the dev rake covers the grant and it clears at the next step");
	Expect(S.DebugSitePlay("gov").find("GOV Ignite") != std::string::npos && S.DebugSitePlay("dares").find("paid entries today") != std::string::npos &&
			   S.DebugSitePlay("describe").find("IGNITE") != std::string::npos,
		"session: ss.Site.Play describe, gov and dares print the book");
}

void StrikeGate()
{
	// W3 (§1.5): under Team RiverLine's "plays only on" clause a rival registration is refused without the second press's
	// token, with nothing changed, and goes through with it; the Flame goes on ice.
	Hooks H;
	ss::Session S(H, "siteplay-clause");
	S.BankrollCents = 600000;
	double Now = Jump(S, 0.0, 63.0 * 1440.0 + 19.0 * 60.0);
	const std::string Signed = S.DebugSponsor("sign riverline");
	const ss::sponsor::Contract* C = ss::sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
	if (!C || C->T.Only != "riverline")
	{
		std::printf("  session: Team RiverLine's clause isn't on offer in this career (%s): the strike gate's check is skipped\n", Signed.c_str());
		return;
	}
	OpenIgnite(S);
	ss::LobbyEvent Ev;
	if (!IgniteEvent(S, Ev))
	{
		Expect(false, "session: an Ignite event to register for under the clause");
		return;
	}
	const play::View Before = S.SitePlayView(net::SiteId::Ignite);
	Expect(Before.Ig.OnIce && !Before.Ig.Started, "session: under the clause the empty book reads on ice (derived, nothing written)");
	const Chips Wallet = *ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite);
	const std::string Why = S.RegisterEvent(Ev);
	Expect(!Why.empty() && !S.IsPlaying(Ev.Spec.Id) && *ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite) == Wallet && S.Life.SitePlay.Ig.S.DayKey < 0,
		"session: without the token core refuses the Ignite table and changes nothing (no table, no debit, no play book): " + Why);
	Expect(S.RegisterEvent(Ev, ss::StrikeConfirm{true}).empty() && S.IsPlaying(Ev.Spec.Id) && S.Life.SitePlay.Ig.F.Streak == 1,
		"session: the second press registers it, and the paid entry lights the Flame");
	Now = Jump(S, Now, S.WorldMinutes() + 10.0 * 1440.0);
	const play::View After = S.SitePlayView(net::SiteId::Ignite);
	Expect(After.Ig.OnIce && S.Life.SitePlay.Ig.F.Streak == 1, "session: ten days on, the flame waits on ice");
}
void StrikeEndsDeal()
{
	// G2 PIE P5: the entry that starts the play book is also the strike that ends Team RiverLine's deal (its second strike;
	// RegisterEvent takes it after the loops' hook). The Flame is off the ice at once, not at the next site step, and the
	// week's misses count from there.
	Hooks H;
	ss::Session S(H, "siteplay-strike-ends");
	S.BankrollCents = 600000;
	double Now = Jump(S, 0.0, 63.0 * 1440.0 + 19.0 * 60.0); // Monday, December 7, 7 PM
	const std::string Signed = S.DebugSponsor("sign riverline");
	const ss::sponsor::Contract* Deal = ss::sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
	if (!Deal || Deal->T.Only != "riverline")
	{
		std::printf("  session: Team RiverLine's clause isn't on offer in this career (%s): the strike-ends-the-deal check is skipped\n", Signed.c_str());
		return;
	}
	for (ss::sponsor::Contract& C : S.Life.Sponsors.Contracts)
	{
		if (C.Id == Deal->Id)
		{
			C.Strikes = 1; // a strike already taken this term: the next one ends the deal
		}
	}
	OpenIgnite(S);
	ss::LobbyEvent Ev;
	if (!IgniteEvent(S, Ev))
	{
		Expect(false, "session: an Ignite event to register for under the clause");
		return;
	}
	const std::string Why = S.RegisterEvent(Ev, ss::StrikeConfirm{true});
	const play::View V = S.SitePlayView(net::SiteId::Ignite);
	Expect(Why.empty() && !ss::sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes()) && V.Ig.Started && V.Ig.Streak == 1 && !V.Ig.OnIce &&
			   S.Life.SitePlay.Ig.F.IceSince < 0,
		"session: the book's first entry is the strike that ends the deal, and the Flame is lit and off the ice at once (P5)");
	Now = Jump(S, Now, S.WorldMinutes() + 4.0 * 1440.0); // Friday: Tuesday and Wednesday rest, Thursday's miss puts it out
	Expect(!S.SitePlayView(net::SiteId::Ignite).Ig.OnIce && S.Life.SitePlay.Ig.F.IceSince < 0 && S.Life.SitePlay.Ig.F.Streak == 0,
		"session: with the deal over the week's misses count (no ice was written)");
}

void DevGrantKeys()
{
	// Dev (G2 PIE): ss.Site.Play grant keys each press with a counter ("dev:<n>"), so two grants in one world minute are two
	// grants (the minute's key made the second a duplicate that paid nothing and read "held on the Guest List").
	Hooks H;
	ss::Session S(H, "siteplay-devgrant");
	double Now = Jump(S, 0.0, 63.0 * 1440.0 + 19.0 * 60.0);
	OpenIgnite(S);
	const double Minute = S.WorldMinutes();
	const std::string First = S.DebugSitePlay("grant ignite 100");
	const std::string Second = S.DebugSitePlay("grant ignite 100");
	int Waiting = 0;
	for (const play::Hold& Hd : S.Life.SitePlay.Holds)
	{
		Waiting += Hd.What == "Grant $1.00" ? 1 : 0;
	}
	Expect(S.WorldMinutes() == Minute && First.find("held on the Guest List") != std::string::npos && Second.find("held on the Guest List") != std::string::npos &&
			   Waiting == 2,
		"session: two dev grants in one minute with no Room are two holds: " + First + " / " + Second);
	S.DebugSitePlay("rake ignite 1000");
	const Chips Wallet = *ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite);
	Now = Jump(S, Now, S.WorldMinutes() + 30.0);
	Expect(S.Life.SitePlay.Holds.empty() && *ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite) == Wallet + 200,
		"session: the dev rake's Room clears both at the next step ($2.00 into the wallet)");
	const Chips Before = *ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite);
	const std::string Third = S.DebugSitePlay("grant ignite 50");
	const std::string Fourth = S.DebugSitePlay("grant ignite 50");
	Expect(Third.find("paid $0.50") != std::string::npos && Fourth.find("paid $0.50") != std::string::npos &&
			   *ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite) == Before + 100 && Third.find("already granted") == std::string::npos,
		"session: with Room, two grants in one minute both pay: " + Third + " / " + Fourth);
}

void CalloutOnLeave()
{
	// W7 (§1.5, R3): on leave under a clause that strikes RunItTwice there are no Callouts in or out, and core refuses the
	// call, not only the page's greyed CALL (review-core-siteplay).
	Hooks H;
	ss::Session S(H, "siteplay-leave");
	Jump(S, 0.0, 63.0 * 1440.0 + 19.0 * 60.0);
	play::Book& B = S.Life.SitePlay;
	B.Rt.L.Month = play::PlayMonth(play::PlayDay(S.WorldMinutes()));
	B.Rt.L.Npcs.fill(-1);
	B.Rt.L.Npcs[0] = 7;
	ss::sponsor::Contract C;
	C.Id = "riverline@1";
	C.Brand = "riverline";
	C.St = ss::sponsor::State::Active;
	C.T.Only = "riverline";
	C.Since = S.WorldMinutes() - 1440.0;
	C.Until = S.WorldMinutes() + 90.0 * 1440.0;
	S.Life.Sponsors.Contracts.push_back(C);
	const std::string Why = S.CallOut(7);
	Expect(!Why.empty() && B.Rt.C.Npc < 0, "session: on leave under Team RiverLine, CallOut is refused in core: " + Why);
	// A RunItTwice day played this month under the deal (the strike taken) is no leave: the call goes through.
	B.Rt.L.Days = {{play::PlayDay(S.WorldMinutes()), 12.5f}};
	Expect(S.CallOut(7).empty() && B.Rt.C.Npc == 7, "session: under the deal with a RunItTwice day played this month, the Callout goes through");
	B.Rt.C = play::Callout();
	B.Rt.L.Days.clear();
	S.Life.Sponsors.Contracts.clear();
	Expect(S.CallOut(7).empty() && B.Rt.C.Npc == 7, "session: with no clause the same Callout goes through");
}

// ------------------------------------------------------------------ G2 fix M1's measure: how often a Beef forms in play

/** A bust of the player in a paid Ignite event: by whom (a world member or a screen name), on which play day. */
struct BeefBust
{
	std::string Name;
	bool Member = false;
	int Day = 0;
};

double BeefFrames(ss::Session& S, double Now, double Seconds)
{
	for (double T = 0.0; T < Seconds; T += 1.0 / 30.0)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
	}
	return Now;
}

double BeefTo(ss::Session& S, double Now, double World)
{
	if (World > S.WorldMinutes())
	{
		S.LobbyMinutes += World - S.WorldMinutes();
	}
	return BeefFrames(S, Now, 0.1);
}

void BeefFed(ss::Session& S)
{
	S.Life.Hunger = 10.0;
	S.Life.Thirst = 10.0;
	S.Life.Energy = 90.0;
}

/** The table in front played to its result (update3_siteplay's way: the plan skips, the player's turns take the grader's
 *  best), the player's bust logged from the tournament itself (who won the pot that busted them), then the lobby. */
double BeefTable(ss::Session& S, double Now, std::vector<BeefBust>& Log)
{
	S.SprintBudgetMs = 1.0e9;
	if (S.TableCount() > 0)
	{
		ss::autoplay::TableAuto A;
		A.S = ss::style::Style::Tag;
		A.M = ss::autoplay::Mode::Skip;
		A.Speed = 1;
		A.To = ss::autoplay::Target::Result;
		A.Stops = 0;
		S.SetAutoplay(0, A);
	}
	for (int Frames = 0; S.CurrentScreen == ss::Screen::Table && Frames < 4000000; ++Frames)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		if (S.TableCount() == 0 || S.Finishing())
		{
			continue;
		}
		if (S.HasPrompt && Now - S.Prompt.OpenedAt > 0.3)
		{
			const std::vector<ss::OptionEV>& Opts = S.Prompt.Analysis.Options;
			size_t Best = 0;
			for (size_t K = 1; K < Opts.size(); ++K)
			{
				Best = Opts[K].Ev > Opts[Best].Ev ? K : Best;
			}
			S.HeroAct(Opts.empty() ? ss::PlayerAction::Fold() : Opts[Best].Action);
		}
		if (!S.HasPrompt && !S.Glance(0).Busted && !S.AutoPanel(0).Skipping)
		{
			S.SkipAhead(0, ss::autoplay::Target::Result);
		}
	}
	if (S.T)
	{
		const ss::TPlayer& Me = S.T->Hero();
		for (const ss::TPlayer& P : S.T->Players)
		{
			if (!Me.KnockedOutBy.empty() && P.Id == Me.KnockedOutBy && !P.Name.empty())
			{
				Log.push_back({P.Name, S.Living().Find(P.Name) >= 0, play::PlayDay(S.WorldMinutes())});
			}
		}
	}
	S.LeaveResults();
	return BeefFrames(S, Now, 0.1);
}

/** update3_siteplay's evening: the site's dearest paid event at or under Cap starting in the next Ahead minutes, played out. */
bool BeefEvent(ss::Session& S, double& Now, Chips Cap, double Ahead, std::set<std::string>& Played, std::vector<BeefBust>& Log)
{
	const net::Network& Net = net::Site(net::SiteId::Ignite);
	const double World = S.WorldMinutes();
	net::EventInstance Pick;
	Chips Best = -1;
	for (const net::EventInstance& I : Net.Window(World - 60.0, World + Ahead))
	{
		const ss::LobbyEvent L = Net.Listing(I, nullptr, S.Unlocks());
		if (Played.count(I.Id) == 0 && L.Joinable && L.BuyInCents > 0 && L.BuyInCents <= Cap && L.Spec.Entrants <= 3000 && I.Start <= World + Ahead &&
			!ss::fuse::IsFuse(I.Id) && (L.BuyInCents > Best || (L.BuyInCents == Best && I.Start < Pick.Start)))
		{
			Pick = I;
			Best = L.BuyInCents;
		}
	}
	if (Best < 0)
	{
		return false;
	}
	Played.insert(Pick.Id);
	BeefFed(S);
	if (!S.RegisterEvent(Net.Listing(Pick, nullptr, S.Unlocks())).empty())
	{
		return false;
	}
	if (S.CurrentScreen != ss::Screen::Table && S.WorldMinutes() < Pick.Start)
	{
		Now = BeefTo(S, Now, Pick.Start);
	}
	for (int Guard = 0; S.CurrentScreen != ss::Screen::Table && Guard < 60; ++Guard)
	{
		Now = BeefFrames(S, Now, 0.1);
	}
	if (S.CurrentScreen != ss::Screen::Table)
	{
		return false;
	}
	Now = BeefTable(S, Now, Log);
	return true;
}

/** A Beef list rule replayed over the logged busts: Keep names at most, evicting the lowest count first (else the stalest),
 *  the 30-day window moving each day. Formed counts the days a new name becomes the Beef. */
struct BeefRule
{
	int Formed = 0, DaysWith = 0;
	std::set<std::string> Names;
};
BeefRule BeefReplay(const std::vector<BeefBust>& Log, int FirstDay, int Days, size_t Keep, bool LowestFirst, bool MembersOnly = false, bool Sticky = false)
{
	struct Row
	{
		std::string Name;
		int They = 0, Last = -1;
	};
	std::vector<Row> L;
	BeefRule Out;
	std::string Cur;
	size_t I = 0;
	for (int D = FirstDay; D < FirstDay + Days; ++D)
	{
		L.erase(std::remove_if(L.begin(), L.end(), [D](const Row& R) { return R.Last < D - 30; }), L.end());
		for (; I < Log.size() && Log[I].Day <= D; ++I)
		{
			if (MembersOnly && !Log[I].Member)
			{
				continue;
			}
			Row* Hit = nullptr;
			for (Row& R : L)
			{
				Hit = R.Name == Log[I].Name ? &R : Hit;
			}
			if (!Hit)
			{
				if (L.size() >= Keep)
				{
					auto Out1 = std::min_element(L.begin(), L.end(), [LowestFirst](const Row& A, const Row& B) {
						return LowestFirst && A.They != B.They ? A.They < B.They : A.Last < B.Last;
					});
					L.erase(Out1);
				}
				L.push_back({Log[I].Name, 0, D});
				Hit = &L.back();
			}
			Hit->They += 1;
			Hit->Last = D;
		}
		const Row* Best = nullptr;
		for (const Row& R : L)
		{
			const bool Inc = Sticky && R.Name == Cur;
			const bool BestInc = Sticky && Best && Best->Name == Cur;
			Best = R.They >= 2 && (!Best || R.They > Best->They || (R.They == Best->They && !BestInc && (Inc || R.Last > Best->Last))) ? &R : Best;
		}
		const std::string Now = Best ? Best->Name : std::string();
		Out.Formed += !Now.empty() && Now != Cur ? 1 : 0;
		Out.DaysWith += Now.empty() ? 0 : 1;
		if (!Now.empty())
		{
			Out.Names.insert(Now);
		}
		Cur = Now;
	}
	return Out;
}

void BeefCareer(int Days, int Fuses)
{
	// update3_siteplay's career (fixture_tool write-u3g2) run long: Ignite's two dearest at or under $11 every evening, Fuses
	// $1.10 FUSEs a night at 4 AM (none gated on a Beef), the claims each night. The book's own Beef is followed day by day.
	Hooks H;
	ss::Session S(H, "siteplay-beef");
	ss::hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Story = ss::hero::Background::DealersKid;
	C.Created = true;
	S.NewCareer(C);
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = BeefFrames(S, 0.0, 0.2);
	S.BankrollCents += 400000;
	(void)S.DebugSite("invite ignite");
	(void)S.AnswerInvite(net::SiteId::Ignite, true);
	(void)S.Deposit(net::SiteId::Ignite, 25000, "card");
	const int Start = play::PlayDay(S.WorldMinutes()) + 2;
	std::vector<BeefBust> Log;
	std::set<std::string> Played;
	std::string Was;
	int Formed = 0, With = 0, Events = 0, Lit = 0;
	size_t MaxList = 0;
	std::set<std::string> Beefs;
	for (int K = 0; K < Days; ++K)
	{
		const int DayNo = Start + K;
		Now = BeefTo(S, Now, static_cast<double>(DayNo) * 1440.0 + 17.0 * 60.0);
		S.BankrollCents = std::max<Chips>(S.BankrollCents, 150000);
		if (*ss::site::WalletOf(S.Life.Sites, net::SiteId::Ignite) < 8000)
		{
			S.Deposit(net::SiteId::Ignite, 10000, "card");
		}
		for (int N = 0, Tries = 0; N < 2 && Tries < 8; ++Tries)
		{
			const double Left = static_cast<double>(DayNo) * 1440.0 + 23.0 * 60.0 - S.WorldMinutes();
			const bool Sat = Left > 0.0 && BeefEvent(S, Now, 1100, Left, Played, Log);
			N += Sat ? 1 : 0;
			Events += Sat ? 1 : 0;
		}
		Now = BeefTo(S, Now, static_cast<double>(DayNo + 1) * 1440.0 + 4.0 * 60.0);
		for (int N = 0; N < Fuses; ++N)
		{
			BeefFed(S);
			if (S.LightFuse(0, false).empty() && S.CurrentScreen == ss::Screen::Table)
			{
				++Lit;
				Now = BeefTable(S, Now, Log);
			}
		}
		const play::View V = S.SitePlayView(net::SiteId::Ignite);
		for (int D = 0; D < 3; ++D)
		{
			if (V.Ig.Dares[static_cast<size_t>(D)].Done && !V.Ig.Dares[static_cast<size_t>(D)].Claimed)
			{
				(void)S.ClaimDare(D);
			}
		}
		if (V.Ig.SpinReady)
		{
			(void)S.SpinWheel();
		}
		const std::string Now0 = V.Ig.BeefName;
		Formed += !Now0.empty() && Now0 != Was ? 1 : 0;
		With += Now0.empty() ? 0 : 1;
		if (!Now0.empty())
		{
			Beefs.insert(Now0);
		}
		Was = Now0;
		MaxList = std::max(MaxList, S.Life.SitePlay.Ig.Beefs.size());
	}
	int Members = 0;
	std::map<std::string, int> ByName;
	for (const BeefBust& B : Log)
	{
		Members += B.Member ? 1 : 0;
		ByName[B.Name] += 1;
	}
	int Twice = 0;
	for (const auto& Kv : ByName)
	{
		Twice += Kv.second >= 2 ? 1 : 0;
	}
	const double Months = Days / 30.0;
	std::printf("  beef: %d days, %d Ignite events and %d FUSEs: %zu busts logged (%d by world members), %zu names, %d of them twice or more over the run\n", Days, Events,
		Lit, Log.size(), Members, ByName.size(), Twice);
	std::printf("  beef: the book: a Beef formed %d times (%.2f a month), %d days of %d with one, %zu names in all; the list peaked at %zu names\n", Formed, Formed / Months, With,
		Days, Beefs.size(), MaxList);
	for (const size_t Keep : {static_cast<size_t>(5), static_cast<size_t>(8), static_cast<size_t>(12), static_cast<size_t>(16), static_cast<size_t>(24), static_cast<size_t>(32),
			 static_cast<size_t>(48), static_cast<size_t>(1000)})
	{
		for (int Mode = 0; Mode < 5; ++Mode)
		{
			const bool Lowest = Mode >= 1;
			const bool OnlyMembers = Mode == 2 || Mode == 4;
			const bool Sticky = Mode >= 3;
			const BeefRule R = BeefReplay(Log, Start, Days + 1, Keep, Lowest, OnlyMembers, Sticky);
			std::printf("    replay: keep %4zu, evict %-13s%-9s%-7s formed %3d (%.2f a month), %3d days with a Beef, %zu names\n", Keep, Lowest ? "lowest count" : "stalest",
				OnlyMembers ? " members" : "", Sticky ? " sticky" : "", R.Formed, R.Formed / Months, R.DaysWith, R.Names.size());
		}
	}
}
} // namespace session_seg

void SegSession()
{
	session_seg::Untouched();
	session_seg::PaidAndClaimed();
	session_seg::StrikeGate();
	session_seg::StrikeEndsDeal();
	session_seg::DevGrantKeys();
	session_seg::CalloutOnLeave();
}

void SegBeef(int Days, int Fuses)
{
	session_seg::BeefCareer(Days, Fuses);
}
} // namespace siteplay_test
