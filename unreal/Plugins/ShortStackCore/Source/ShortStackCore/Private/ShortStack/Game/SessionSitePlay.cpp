// Update 3 (SPEC §7.2): the Session's site-play methods, core-siteplay's file (the SessionStake.cpp precedent): the pages'
// view, the claims (each "" or why not, through site::play's keyed and idempotent claims, autosaved on success: the claim
// commits before the UI animates it), the post-hand hook, the calendar's step and ss.Site.Play.
#include "ShortStack/Game/Session.h"
#include "SessionHold.h"

#include "SitePlayPriv.h"
#include "SitesDetail.h"
#include "ShortStack/AI/Grading.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Fuse.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace ss
{
namespace session_siteplay_detail
{
std::vector<std::string> SitePlayWords(const std::string& Args)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char Ch : Args)
	{
		if (Ch == ' ' || Ch == '\t')
		{
			if (!Cur.empty())
			{
				Out.push_back(Cur);
				Cur.clear();
			}
		}
		else
		{
			Cur.push_back(Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch);
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return Out;
}

net::SiteId SitePlaySiteNamed(const std::string& W)
{
	return W == "runittwice" || W == "rit" ? net::SiteId::RunItTwice : W == "ignite" || W == "ig" ? net::SiteId::Ignite : net::SiteId::Count;
}

/** "7h2c": the hole cards' ranks and suits as text (the Dares read the ranks). */
std::string SitePlayHole(const std::vector<Card>& Hole)
{
	static const char Ranks[] = "23456789TJQKA";
	static const char Suits[] = "cdhs";
	std::string S;
	for (const Card C : Hole)
	{
		if (C >= 0 && C < 52)
		{
			S.push_back(Ranks[C >> 2]);
			S.push_back(Suits[C & 3]);
		}
	}
	return S;
}

/** Who won the last pot a seat was in (its seat id), the tournament's rule for a knockout (Session.cpp's stream hook). */
std::string SitePlayBustedBy(const Hand& H, const HandSeat& S)
{
	std::string By;
	for (const PotResult& P : H.PotResults)
	{
		if (std::find(P.Eligible.begin(), P.Eligible.end(), S.Seat) != P.Eligible.end() && !P.Winners.empty())
		{
			const HandSeat* W = H.SeatByNumber(P.Winners.front());
			By = W && W->Id != S.Id ? W->Id : By;
		}
	}
	return By;
}
} // namespace session_siteplay_detail

site::play::View Session::SitePlayView(net::SiteId S) const
{
	using namespace site::play;
	const double World = WorldMinutes();
	View V = ViewOf(Life.SitePlay, Life, Life.Sites, &LivingWorld, S, World);
	if (S == net::SiteId::RunItTwice)
	{
		// The Briefing (R9): yesterday's line and today's stakes, from History and the view (information only).
		const int Day = PlayDay(World);
		int Events = 0;
		Chips Net = 0;
		for (const HistoryEntry& E : History)
		{
			const size_t At = E.EventId.rfind('@');
			if (At == std::string::npos || net::SiteOf(E.EventId) != net::SiteId::RunItTwice)
			{
				continue;
			}
			if (PlayDay(std::atof(E.EventId.c_str() + At + 1)) == Day - 1)
			{
				++Events;
				Net += E.Prize - std::max<Chips>(0, E.BuyInCents);
			}
		}
		std::string Line = "BRIEFING";
		if (Events > 0)
		{
			Line += " · yesterday " + std::to_string(Events) + (Events == 1 ? " event, " : " events, ") + (Net >= 0 ? "+" : "") + Money(Net);
		}
		const RitView& Rv = V.Rt;
		if (Rv.League)
		{
			for (const LeagueRowView& Row : Rv.Table)
			{
				if (Row.Hero && Row.Move != 0)
				{
					Line += " · League " + Ordinal(Row.Rank + Row.Move) + " to " + Ordinal(Row.Rank);
				}
			}
			if (Rv.ToPromotion > 0.0)
			{
				Line += " · " + Fixed(Rv.ToPromotion, 0) + " pts to promotion";
			}
			if (!Rv.MatchdayTonight.empty())
			{
				Line += " · MATCHDAY TONIGHT: " + Rv.MatchdayTonight;
			}
		}
		if (Rv.CalloutOn && Rv.CalloutByThem && !Rv.CalloutName.empty())
		{
			Line += " · INCOMING CALLOUT: " + Rv.CalloutName;
		}
		V.Rt.Briefing = Line;
		// The open RunItTwice tables whose result would score if it came in now: paid, at the division's floor, and today's
		// first-k quota not yet filled (results count in the order they finish, the world's rule for its members).
		const site::play::League& Lg = Life.SitePlay.Rt.L;
		const int Done = Lg.TodayKey == Day ? Lg.TodayCount : 0;
		if (Rv.League && Done < std::clamp<int>(Lg.Pace, 1, 3))
		{
			for (int K = 0; K < TableCount(); ++K)
			{
				const LobbyEvent& Ev = K == Active ? Joined : Runs[static_cast<size_t>(K)]->Joined;
				if (net::SiteOf(Ev.Spec.Id) == net::SiteId::RunItTwice && PaidEntry(Life, Ev.Spec.Id) && Ev.BuyInCents >= DivisionFloor(Lg.Div))
				{
					V.Rt.Counting.push_back(Ev.Spec.Id);
				}
			}
		}
	}
	return V;
}

std::string Session::SpinWheel()
{
	const std::string Why = site::play::Spin(Life.SitePlay, Life, WorldSeed(), WorldMinutes());
	if (Why.empty())
	{
		Save(); // the claim commits before the wheel turns: a reload replays the same spin
	}
	return Why;
}

std::string Session::OpenCracker(const std::string& BoxId)
{
	const std::string Why = site::play::Open(Life.SitePlay, Life, WorldSeed(), BoxId, WorldMinutes());
	if (Why.empty())
	{
		Save();
	}
	return Why;
}

std::string Session::ClaimDare(int Slot)
{
	const std::string Why = site::play::ClaimDare(Life.SitePlay, Life, Slot, WorldMinutes());
	if (Why.empty())
	{
		Save();
	}
	return Why;
}

std::string Session::ClaimTier(int Tier)
{
	const std::string Why = site::play::ClaimTier(Life.SitePlay, Life, Tier, WorldMinutes());
	if (Why.empty())
	{
		Save();
	}
	return Why;
}

std::string Session::BuyPrize(int Item)
{
	const std::string Why = site::play::Buy(Life.SitePlay, Life, Item, WorldMinutes());
	if (Why.empty())
	{
		Save();
	}
	return Why;
}

std::string Session::Equip(int Slot, int Look)
{
	const std::string Why = site::play::Equip(Life.SitePlay, Slot, Look);
	if (Why.empty())
	{
		Life.Update3LinesRead |= life::U3SitePlay; // a Clicked claim: the book has state
		Save();
	}
	return Why;
}

std::string Session::StarEvent(const std::string& TemplateId, bool On)
{
	const std::string Why = site::play::Star(Life.SitePlay, TemplateId, On);
	if (Why.empty())
	{
		Life.Update3LinesRead |= life::U3SitePlay;
		Save();
	}
	return Why;
}

std::string Session::CallOut(int Npc)
{
	// On leave (W7, §1.5): under a clause that strikes RunItTwice, in a month with no RunItTwice day played, no Callouts in or
	// out (the pages grey CALL; core refuses it too: review, a console or a stale page called out while on leave).
	const std::string Leave = site::play::siteplay_detail::RitCallOutBlocked(Life.SitePlay, &Life.Sponsors, WorldMinutes());
	if (!Leave.empty())
	{
		return Leave;
	}
	const std::string Why = site::play::CallOut(Life.SitePlay, Npc, WorldMinutes());
	if (Why.empty())
	{
		// The member's points as the call is made: the world's recorded best days (the duel is the week's points).
		site::play::Callout& C = Life.SitePlay.Rt.C;
		DrainWorld(); // Update 4 (§3.7 R3, PA-35): drain-on-touch
		const world::LeagueBook* Lb = LivingWorld.League(net::SiteId::RunItTwice);
		double Theirs = 0.0;
		if (Lb && Lb->MonthKey == Life.SitePlay.Rt.L.Month)
		{
			for (const world::LeagueMember& M : Lb->Members)
			{
				if (M.Npc == Npc)
				{
					for (const auto& P : M.Best)
					{
						Theirs += static_cast<double>(P.second) / 10.0;
					}
				}
			}
		}
		C.NpcPts0 = static_cast<float>(Theirs);
		Life.Update3LinesRead |= life::U3SitePlay;
		Save();
	}
	return Why;
}

void Session::SitePlayHand(const Hand& H, const HandFacts& F)
{
	using namespace session_siteplay_detail;
	// After every hand the player was dealt, on both paths (HeroHandDone): only a paid rival entry's hands count (W4).
	if (!T)
	{
		return;
	}
	const std::string EventId = T->Spec.Id;
	const net::SiteId Site = net::SiteOf(EventId);
	if ((Site != net::SiteId::Ignite && Site != net::SiteId::RunItTwice) || !site::play::PaidEntry(Life, EventId))
	{
		return;
	}
	int HeroIdx = -1;
	for (size_t I = 0; I < H.Seats.size(); ++I)
	{
		HeroIdx = H.Seats[I].Id == HeroId ? static_cast<int>(I) : HeroIdx;
	}
	if (HeroIdx < 0)
	{
		return;
	}
	const HandSeat& Me = H.Seats[static_cast<size_t>(HeroIdx)];
	site::play::HandNote N;
	N.Paid = true;
	N.Own = !F.ByPlan;
	N.EventId = EventId;
	N.HandId = EventId + "#" + std::to_string(T->Hero().TableId) + "#" + std::to_string(HandsPlayed);
	N.HoleCards = SitePlayHole(Me.Hole);
	N.BuyIn = T->Spec.BuyInCents;
	N.PotBb = static_cast<double>(F.PotBefore) / static_cast<double>(std::max<Chips>(1, H.BigBlind));
	for (const PotResult& P : H.PotResults)
	{
		N.Won = N.Won || std::find(P.Winners.begin(), P.Winners.end(), Me.Seat) != P.Winners.end();
	}
	for (const HandEvent& E : H.Events)
	{
		if (E.Type == EventType::Showdown)
		{
			for (const ShowdownHand& Sh : E.Hands)
			{
				N.Showdown = N.Showdown || Sh.Seat == Me.Seat;
			}
		}
	}
	// The all-in with cards to come: its equity, its street and the real run's result (site::show's record, on both paths).
	site::show::SecondRecord Rec;
	if (site::show::SecondRecordOf(H, HeroIdx, N.HandId, Rec) && Rec.Board.HeroIn)
	{
		N.AllIn = true;
		N.Equity = Rec.Equity;
		N.Preflop = Rec.Board.Shared == 0;
		N.Lost = Rec.Board.First == 0;
		N.WonFirst = Rec.Board.First == 2;
		if (Site == net::SiteId::RunItTwice && Rec.Board.FromStub)
		{
			N.SecondVerdict = static_cast<int8_t>(Rec.Board.Second);
		}
	}
	// Who the player busted (the tournament's rule) and who busted the player, by name and by the world's id.
	for (const HandSeat& S : H.Seats)
	{
		if (S.Id == HeroId || S.Stack > 0 || S.StartStack <= 0)
		{
			continue;
		}
		if (SitePlayBustedBy(H, S) == HeroId)
		{
			const TPlayer* P = PlayerById(S.Id);
			const std::string Name = P ? P->Name : std::string();
			if (!Name.empty())
			{
				N.Knocked.push_back(Name);
				DrainWorld(); // Update 4 (§3.7 R3, PA-35): drain-on-touch (the lists below follow it)
				N.KnockedNpcs.push_back(LivingWorld.Find(Name));
			}
		}
	}
	if (Me.Stack <= 0)
	{
		const std::string By = SitePlayBustedBy(H, Me);
		if (const TPlayer* P = By.empty() ? nullptr : PlayerById(By))
		{
			N.BustedBy = P->Name;
			DrainWorld(); // Update 4 (§3.7 R3, PA-35): drain-on-touch
			N.BustedByNpc = LivingWorld.Find(P->Name);
		}
	}
	// The world's lists only when somebody went out this hand (most hands knock nobody out: the lists walk the world).
	if (!N.Knocked.empty() && Site == net::SiteId::Ignite)
	{
		for (const int Id : site::play::Headliners(&LivingWorld))
		{
			const world::Npc* Nh = LivingWorld.Get(Id);
			if (Nh && std::find(N.Knocked.begin(), N.Knocked.end(), Nh->Name) != N.Knocked.end())
			{
				N.Headliners.push_back(Nh->Name);
			}
		}
	}
	else if (!N.Knocked.empty())
	{
		N.TeamSix = site::play::TeamPros(&LivingWorld);
		for (const int Id : N.KnockedNpcs)
		{
			if (Id >= 0 && std::find(N.TeamSix.begin(), N.TeamSix.end(), Id) != N.TeamSix.end())
			{
				N.TeamPros.push_back(Id);
			}
		}
	}
	// The player's own graded decisions this hand (U4: the plan's aren't the player's): the event's grades since the last hand.
	site::play::siteplay_detail::SitePlayScratch& Scratch = site::play::siteplay_detail::SitePlayScratchOf(Life.SitePlay);
	if (Scratch.GradeEvent != EventId || Scratch.GradeSeen > Grades.size())
	{
		Scratch.GradeEvent = EventId;
		Scratch.GradeSeen = HandsPlayed <= 1 ? 0 : Grades.size();
	}
	for (size_t K = Scratch.GradeSeen; K < Grades.size(); ++K)
	{
		++N.Graded;
		N.GoodGrades += GradeScore(Grades[K].Result) >= 90.0 ? 1 : 0;
	}
	Scratch.GradeSeen = Grades.size();
	site::play::OnHandPaid(Life.SitePlay, Life, Site, N, WorldMinutes());
}

void Session::SitePlayStep(double From, double To)
{
	// Phase A's seam (the lead): site::play::Step steps only a book a paid entry or a claim already made (never seeding one,
	// S10), and a League group it draws goes to the world, which records the group's own results (§3.6 R2). A first League
	// month waits on its placement: Abi100 from History, as the invitation gates read it.
	site::play::League& Lg = Life.SitePlay.Rt.L;
	if (Lg.Month == -2 && Lg.Placement)
	{
		rep::Public P;
		rep::FromHistory(P, History);
		Lg.Div = static_cast<int8_t>(site::play::PlacementDivision(P.Abi100));
	}
	// Update 4 (SPEC v1.7 §3.7 R3, §6 PA-35): the step reads the world only through RunItTwice's loops (RitStep), so a held turn
	// finishes first only for a RunItTwice book (CheckCalendar runs every frame); the League group it draws goes to the world.
	if (site::play::siteplay_detail::SitePlayRitStarted(Life.SitePlay))
	{
		DrainWorld();
	}
	const site::play::StepOut Out = site::play::Step(Life.SitePlay, Life, Life.Sites, &LivingWorld, Life.Sponsors, WorldSeed(), From, To);
	if (Out.Roster)
	{
		DrainWorld();
		LivingWorld.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
	}
	for (const std::pair<std::string, std::string>& Tx : Out.Mail)
	{
		Hooks.Text(Tx.first, Tx.second);
	}
}

std::string Session::DebugSitePlay(const std::string& Args)
{
	using namespace session_siteplay_detail;
	using namespace site::play;
	const std::vector<std::string> W = SitePlayWords(Args);
	const std::string Verb = W.empty() ? std::string("describe") : W[0];
	const double World = WorldMinutes();
	Book& B = Life.SitePlay;
	auto Num = [&W](size_t I, long long Default) { return W.size() > I ? std::atoll(W[I].c_str()) : Default; };
	DrainWorld(); // Update 4 (§3.7 R3, PA-35): the verbs read and write the world (the League roster)
	if (Verb == "describe")
	{
		return siteplay_detail::IgniteDescribe(B, World) + siteplay_detail::RitDescribe(B, &LivingWorld, World) + DebugSitePlay("gov");
	}
	if (Verb == "gov")
	{
		std::string S;
		for (const net::SiteId Site : {net::SiteId::Ignite, net::SiteId::RunItTwice})
		{
			const View V = ViewOf(B, Life, Life.Sites, &LivingWorld, Site, World);
			const GovView& G = V.Gov;
			S += "GOV " + std::string(net::Def(Site).Name) + ": rake30 " + Money(G.Rake30) + " promo30 " + Money(G.Promo30) + " (" + Fixed(G.Share * 100.0, 1) + "%, ceiling " +
				Fixed(G.Ceiling * 100.0, 0) + "%) room " + Money(G.Room) + " week " + Money(G.WeekFace) + " of " + (G.WeekCap > 0 ? Money(G.WeekCap) : std::string("uncapped")) +
				(G.MondayBank > 0 ? ", Monday bonus " + Grouped(G.MondayBank) + " Sparks" : std::string()) + "\n";
			for (const HoldView& H : G.Holds)
			{
				S += "  hold: " + H.What + " " + Money(H.Cents) + " covered " + Money(H.Covered) + " (" + (H.SparksOwed ? Grouped(H.Sparks) + " Sparks, " : std::string()) +
					"clears after " + Money(H.RakeLeft) + " more rake)" + (H.Reset ? " reset" : "") + "\n";
			}
		}
		return S;
	}
	if (Verb == "fuse")
	{
		return fuse::DebugForce(*this, static_cast<int>(Num(1, 0)), static_cast<int>(Num(2, 2)));
	}
	if (Verb == "wheel" || Verb == "dares")
	{
		const View V = SitePlayView(net::SiteId::Ignite);
		const IgniteView& I = V.Ig;
		std::string S;
		if (Verb == "wheel")
		{
			for (int Rg = 0; Rg < 3; ++Rg)
			{
				double U = siteplay_detail::SitePlayDraw(WorldSeed(), "ignite:wheel:" + std::to_string(PlayDay(World)));
				const std::vector<WedgeView>& Ring = I.Rings[static_cast<size_t>(Rg)];
				size_t Wedge = Ring.size() - 1;
				for (size_t K = 0; K < Ring.size(); ++K)
				{
					U -= Ring[K].P;
					if (U < 0.0)
					{
						Wedge = K;
						break;
					}
				}
				S += std::string(Rg == 0 ? "GLOW" : Rg == 1 ? "SHINE" : "DAZZLE") + ": " + (Ring.empty() ? std::string() : Ring[Wedge].Label) + (I.RingsLit == Rg + 1 ? " (lit)" : "") + "\n";
			}
			S += I.Spun ? "spun today\n" : I.SpinReady ? "spin ready\n" : "no ring lit\n";
			return S;
		}
		for (const DareView& D : I.Dares)
		{
			S += std::to_string(D.Id) + " " + D.Label + ": " + std::to_string(D.Prog) + "/" + std::to_string(D.Needs) + " (needs " + std::to_string(D.Entries) + " paid entries, " +
				Grouped(D.Sparks) + " Sparks)" + (D.Done ? " DONE" : "") + (D.Claimed ? " claimed" : "") + (D.Held ? " (Sparks owed: on the Guest List)" : "") + "\n";
		}
		return S + "paid entries today: " + std::to_string(I.EntriesToday) + "\n";
	}
	if (Verb == "league")
	{
		return siteplay_detail::RitDescribe(B, &LivingWorld, World);
	}
	if (Verb == "sparks" || Verb == "flame" || Verb == "level")
	{
		const std::string Said = siteplay_detail::IgniteDebug(B, Life, Verb, Num(1, 0), World);
		Save();
		return Said;
	}
	if (Verb == "scalp" || Verb == "league-draw" || Verb == "league-close")
	{
		StepOut Out;
		const std::string Said = siteplay_detail::RitDebug(B, Life, &LivingWorld, &Life.Sponsors, WorldSeed(), Verb, static_cast<int>(Num(1, -1)), World, Out);
		if (Out.Roster)
		{
			LivingWorld.SetLeagueRoster(net::SiteId::RunItTwice, Out.MonthKey, Out.Div, Out.Npcs);
		}
		Save();
		return Said;
	}
	if (Verb == "beef")
	{
		// Dev only (PIE §9.3 #3; the C2 gate): a bust of the player on Ignite by that world member, noted as the per-hand hook
		// notes one (twice makes them the Beef). In play the list counts 24 names over the 30-day window (G2 fix M1).
		std::string Name;
		int Npc = -1;
		for (const world::Npc& P : LivingWorld.People())
		{
			std::string Lower = P.Name;
			for (char& Ch : Lower)
			{
				Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
			}
			if (Name.empty() && W.size() > 1 && Lower == W[1])
			{
				Name = P.Name;
				Npc = P.Id;
			}
		}
		if (Name.empty() || !SitePlayView(net::SiteId::Ignite).Ig.Started)
		{
			return Name.empty() ? "usage: beef <a world member's name>" : "no Ignite book yet (a paid Ignite entry starts it)";
		}
		HandNote N;
		N.Paid = true;
		N.BustedBy = Name;
		N.BustedByNpc = Npc;
		N.EventId = "dev:beef";
		N.HandId = "dev:beef:" + std::to_string(static_cast<long long>(World));
		site::play::OnHandPaid(B, Life, net::SiteId::Ignite, N, World);
		Save();
		return "a bust by " + Name + " noted";
	}
	if (Verb == "callout")
	{
		const std::string Said = CallOut(static_cast<int>(Num(1, -1)));
		return Said.empty() ? "called out " + std::to_string(Num(1, -1)) : Said;
	}
	if (Verb == "grant" || Verb == "rake")
	{
		const net::SiteId Site = W.size() > 1 ? SitePlaySiteNamed(W[1]) : net::SiteId::Count;
		const Chips Cents = static_cast<Chips>(Num(2, 0));
		if (Site == net::SiteId::Count || Cents <= 0)
		{
			return "usage: " + Verb + " <ignite|runittwice> <cents>";
		}
		const int Day = PlayDay(World);
		if (Verb == "rake")
		{
			// Dev only: paid rake into today's slot, so a hold can be watched accruing and clearing without the events.
			Life.Update3LinesRead |= life::U3SitePlay;
			siteplay_detail::SitePlayRoll(B, Site, Day);
			GovRing& G = B.Gov[static_cast<size_t>(siteplay_detail::SitePlayGovIndex(Site))];
			const size_t Slot = static_cast<size_t>(((Day % 30) + 30) % 30);
			G.Rake[Slot] = static_cast<int32_t>(std::min<int64_t>(0x7fffffff, static_cast<int64_t>(G.Rake[Slot]) + Cents * ValuePerCent));
			Save();
			return "booked " + Money(Cents) + " of paid rake on " + std::string(net::Def(Site).Name) + " (dev)";
		}
		// A counter key, the first "dev:<n>" no grant on this site has used (G2 PIE, dev: the world minute made a second grant in
		// the same minute a duplicate that paid nothing and read "held"). Grant's keyed contract is unchanged: a key that has
		// paid or queued pays nothing again, and the verb says so rather than calling it held.
		int DevSeq = 1;
		while (DevSeq < 1000000 && siteplay_detail::SitePlayDone(Life.Sites, siteplay_detail::SitePlayGrantDoneKey(Site, "dev:" + std::to_string(DevSeq))))
		{
			++DevSeq;
		}
		const std::string DevKey = "dev:" + std::to_string(DevSeq);
		if (siteplay_detail::SitePlayDone(Life.Sites, siteplay_detail::SitePlayGrantDoneKey(Site, DevKey)))
		{
			return "already granted (same key): Grant " + Money(Cents);
		}
		const bool Paid = Grant(B, Life, Site, Kind::WalletPromo, "Grant " + Money(Cents), Cents, DevKey, Day, World);
		Save();
		return Paid ? "paid " + Money(Cents) + " into the " + std::string(net::Def(Site).Name) + " wallet" : "held on the Guest List: Grant " + Money(Cents);
	}
	return "ss.Site.Play: describe, gov, fuse <tier> <m>, wheel, dares, league, sparks <n>, flame <days>, level <n>, scalp <npc>, beef <name>, league-draw, league-close, "
		   "callout <npc>, grant <site> <cents>, rake <site> <cents>";
}
} // namespace ss
