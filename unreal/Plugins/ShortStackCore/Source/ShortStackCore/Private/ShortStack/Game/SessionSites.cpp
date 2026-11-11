#include "ShortStack/Game/Session.h"
#include "SessionHold.h"

#include "SitesDetail.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/SiteShow.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

// The rival sites' side of the session (update 2 §5, §6.2): invitations, the Cashier, restrictions by site, the pots the
// money lands in, the event's settle by site, the calendar's step and ss.Site.*. core-sites.
//
// The money itself moves where it always has: RegisterEvent takes a buy-in out of the event's pot (PotFor: the bankroll for
// RiverLine, a rival's wallet) and site::OnBuyIn writes the wallet's line and counts the rake (RiverLine's into Rewards);
// ShowResults puts the prize in the same pot, and SiteSettle writes the wallet's lines and the player's record there.

namespace ss
{
namespace session_sites_detail
{
std::vector<std::string> SitesWords(const std::string& Args)
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

/** A site by its key or its name, any case ("ignite", "runittwice", "riverline"); Count for none. */
net::SiteId SitesNamed(const std::string& Word)
{
	for (const net::SiteDef& D : net::Sites())
	{
		std::string Name = D.Name;
		for (char& Ch : Name)
		{
			Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
		}
		if (Word == D.Key || Word == Name)
		{
			return D.Id;
		}
	}
	return net::SiteId::Count;
}

/** "$212.40" from dollars typed at the console. */
Chips SitesDollars(const std::string& Word)
{
	return static_cast<Chips>(std::llround(std::atof(Word.c_str()) * 100.0));
}

std::string SitesWhen(double World)
{
	return std::string(net::WeekdayName(net::DayOf(World))) + " " + net::DateLabel(net::DayOf(World)) + " " + net::TimeLabel(World);
}
} // namespace session_sites_detail

namespace site
{
// SiteMoney.cpp's (core-sites-g2, G2); declared in SitesDetail.h by the patch core-sites-g2_to_lead_1.diff.
int IgniteLevelOf(const life::State& L);

site::play::SettleFacts SettleFactsOf(const ss::Tournament& T, const ss::Results& Res)
{
	// Update 3 G2 (SPEC §7.2's Results row, §2.6 F8, §3.6 R2-R4; core-sites-g2): a finished rival event as the play book hears
	// it. The knockouts and the bust by name (Beef, Payback, the Headliners' scalps, H2H, Team Scalps), the season's points
	// (the League's raw points; none for a FUSE, which scores no Month, Season or League points), the final table on the rule
	// the account's record keeps (1st to 9th, as Season::FinalTables), the template (a launch-week one-off as the daily it stands
	// in for, so The Spark is The Spark in its launch week too), its featured and series flags, and the engine's format.
	site::play::SettleFacts F;
	F.Site = net::SiteOf(T.Spec.Id);
	F.EventId = T.Spec.Id;
	const std::string Played = T.Spec.Id.substr(0, T.Spec.Id.find('@'));
	const std::string Daily = net::netsites_detail::NetSitesLaunchBase(Played);
	F.Template = Daily.empty() ? Played : Daily;
	F.Fuse = fuse::IsFuse(T.Spec.Id);
	const net::EventTemplate* Tmpl = F.Fuse || Played.empty() ? nullptr : net::ForEvent(T.Spec.Id).FindTemplate(Played);
	F.Place = Res.Place;
	F.Entrants = Res.Entrants;
	F.Prize = Res.PrizeCents;
	F.BuyIn = Res.BuyInCents;
	F.Points = Res.BuyInCents > 0 && !F.Fuse ? net::Points(Res.Place, Res.Entrants, Res.BuyInCents) : 0.0;
	F.Final = Res.Place >= 1 && Res.Place <= 9 && !F.Fuse;
	F.Win = Res.Place == 1;
	F.Format = static_cast<int>(Tmpl ? Tmpl->Fmt : net::Format::Freezeout); // as OnPaidEntry's; a FUSE says so in Fuse
	F.Featured = Tmpl && Tmpl->Featured;
	F.Series = Tmpl && !Tmpl->Series.empty();
	F.FuseM = F.Fuse && Res.BuyInCents > 0 ? static_cast<int>(T.Spec.GuaranteeCents / Res.BuyInCents) : 0;
	if (T.Players.empty())
	{
		return F;
	}
	const TPlayer& Hero = T.Hero();
	for (const TPlayer& P : T.Players)
	{
		if (P.Id == Hero.Id)
		{
			continue;
		}
		if (P.Busted && P.KnockedOutBy == Hero.Id)
		{
			F.Knocked.push_back(P.Name);
		}
		F.BustedBy = !Hero.KnockedOutBy.empty() && P.Id == Hero.KnockedOutBy ? P.Name : F.BustedBy;
		F.FuseWinner = F.Fuse && P.Place == 1 ? P.Name : F.FuseWinner;
	}
	F.FuseWinner = F.Fuse && Hero.Place == 1 ? Hero.Name : F.FuseWinner;
	return F;
}
} // namespace site

void Session::SiteSettle(const Results& Res)
{
	// RiverLine's prize is the bank's (ShowResults paid it and wrote the bank's line). A rival's landed in its wallet: its
	// lines (the bounties and the prize, one each) and the player's record there (the season, lifetime won and played).
	if (!T)
	{
		return;
	}
	const net::SiteId Site = net::SiteOf(T->Spec.Id);
	if (Site == net::SiteId::RiverLine || !site::AccountOf(Life.Sites, Site))
	{
		return;
	}
	const double World = WorldMinutes();
	site::Season Add;
	Add.Year = world::YearOf(net::DayOf(World));
	if (Res.BountyCents > 0)
	{
		site::OnPrize(Life.Sites, Site, Res.BountyCents, "Bounties: " + Res.EventName, World, Add);
	}
	// Update 3 (§2.6 F7; dormant until G2): a FUSE scores no Month or Season points and no final table; it counts in the
	// record's events and money, and in Ignite's own loops (below).
	const bool Fuse = fuse::IsFuse(T->Spec.Id);
	Add.Points = Res.BuyInCents > 0 && !Fuse ? net::Points(Res.Place, Res.Entrants, Res.BuyInCents) : 0.0;
	Add.Won = Res.PrizeCents + Res.BountyCents;
	Add.Wins = Res.Place == 1 ? 1 : 0;
	Add.FinalTables = Res.Place >= 1 && Res.Place <= 9 && !Fuse ? 1 : 0;
	Add.Events = 1;
	// Update 3 G2 (§1.3; core-fuse's patch core-fuse_to_core-sites-g2_1.diff, taken with the multiplier read here so this file
	// builds on HEAD alone): a FUSE's prize line names its multiplier and place, "FUSE x25: 1st" (the pool is exactly m x the
	// buy-in, so m is the guarantee over it, as fuse::MultiplierOf reads it); every other event's, "Prize: " and its name.
	const int FuseM = Fuse && T->Spec.BuyInCents > 0 ? static_cast<int>(T->Spec.GuaranteeCents / T->Spec.BuyInCents) : 0;
	const std::string PrizeLabel = Fuse ? "FUSE x" + std::to_string(FuseM) + ": " + Ordinal(Res.Place) : "Prize: " + Res.EventName;
	site::OnPrize(Life.Sites, Site, Res.PrizeCents, PrizeLabel, World, Add);
	Life.Update2LinesRead |= life::U2Sites;
	// Update 3 G2 (core-sites-g2): every rival result as the site play book hears it, a FUSE's with its multiplier, its winner
	// and no points. What the book does with it is core-siteplay's OnResult (a result moves only what a paid entry started:
	// W4, S10).
	site::play::OnResult(Life.SitePlay, Life, site::SettleFactsOf(*T, Res), World);
}

std::string Session::AnswerInvite(net::SiteId S, bool Accept)
{
	const std::vector<std::pair<std::string, std::string>> Name = {{"Site", net::Def(S).Name}};
	if (!site::IsRival(S))
	{
		return site::CopyText("err:site");
	}
	if (site::AccountOf(Life.Sites, S))
	{
		return site::FillCopy(site::CopyText("err:joined"), Name);
	}
	site::Invitation* Open = nullptr;
	for (site::Invitation& I : Life.Sites.Invites)
	{
		Open = I.Site == S && (I.State == 0 || I.State == 3) ? &I : Open;
	}
	if (!Open)
	{
		return site::FillCopy(site::CopyText("err:invite"), Name);
	}
	const double World = WorldMinutes();
	if (Accept)
	{
		const std::string Why = site::Join(Life.Sites, S, World);
		if (!Why.empty())
		{
			return Why;
		}
		Open->State = 1;
	}
	else
	{
		// Declined: the site writes once more, 30 days from now (SiteStep), with the same offer.
		Open->State = 2;
		Open->At = World;
	}
	// The invitation's letters are answered, so Mail's trim may let them go.
	for (life::Letter& M : Life.Inbox)
	{
		M.Read = M.Read || (M.Source == "site" && M.Thread == net::KeyOf(S) && M.Kind == site::LetterInvite);
	}
	Life.Update2LinesRead |= life::U2Sites;
	if (Accept)
	{
		SiteStep(World, World); // the welcome letter, at once
	}
	Save();
	return std::string();
}

std::string Session::Deposit(net::SiteId S, Chips Cents, const std::string& Method)
{
	const std::string Why = site::Deposit(Life.Sites, Life, BankrollCents, S, Cents, Method, WorldMinutes());
	if (Why.empty())
	{
		Life.Update2LinesRead |= life::U2Sites;
		Save();
	}
	return Why;
}

std::string Session::Withdraw(net::SiteId S, Chips Cents, const std::string& Method)
{
	const std::string Why = site::Withdraw(Life.Sites, S, Cents, Method, WorldMinutes());
	if (Why.empty())
	{
		Life.Update2LinesRead |= life::U2Sites;
		Save();
	}
	return Why;
}

std::string Session::ReverseWithdrawal(const std::string& TransferId)
{
	return site::Reverse(Life.Sites, TransferId, WorldMinutes());
}

bool Session::Restricted(net::SiteId S) const
{
	// A RiverLine restriction is RiverLine's alone (§1.9 #12): the rivals don't know or care.
	return S == net::SiteId::RiverLine && Restricted();
}

stake::Pot Session::PotFor(net::SiteId S)
{
	// RiverLine's money is the bank's (U8); a rival's is its wallet (null with no account there: nothing can be paid from it).
	stake::Pot P;
	P.Cents = S == net::SiteId::RiverLine ? &BankrollCents : site::WalletOf(Life.Sites, S);
	P.Site = S == net::SiteId::RiverLine ? 0 : static_cast<int>(S);
	return P;
}

void Session::SiteStep(double From, double To)
{
	// CheckCalendar's step after Rerun's (§1.6): withdrawals landing, the 1st's rakeback (RiverLine Rewards, the Second Run
	// Club), the welcome bonus's end, and once a day the invitation gates. The record (rep::Of, then History's last-hundred
	// numbers) is built only on a day that checks the gates.
	rep::Public P;
	if (net::DayOf(From) != net::DayOf(To))
	{
		DrainWorld(); // Update 4 (§3.7 R3, PA-35): the gates read the world's record of the player
		P = Record();
		rep::FromHistory(P, History);
	}
	std::vector<std::pair<std::string, std::string>> Texts;
	site::Step(Life.Sites, Life, BankrollCents, P, 0, From, To, Texts);
	for (const std::pair<std::string, std::string>& Tx : Texts)
	{
		Hooks.Text(Tx.first, Tx.second);
	}
}

std::string Session::DebugSite(const std::string& Args)
{
	using namespace session_sites_detail;
	const std::vector<std::string> W = SitesWords(Args);
	const std::string Verb = W.empty() ? std::string("describe") : W[0];
	const double World = WorldMinutes();
	const net::SiteId Named = W.size() > 1 ? SitesNamed(W[1]) : net::SiteId::Count;
	DrainWorld(); // Update 4 (§3.7 R3, PA-35): the verbs read the world, some write it
	const std::string Usage = "ss.Site.<Describe|Invite <site>|Accept <site>|Decline <site>|Launch <site>|Deposit <site> <dollars> [card]|Withdraw <site> <dollars> [bank]|Advance <hours>>";
	// Update 3 (§7.6): ss.Site.Play's verbs, the rest of the line as typed (core-siteplay's DebugSitePlay).
	if (Verb == "play")
	{
		const size_t Word = Args.find_first_not_of(" \t");
		const size_t Rest = Word == std::string::npos ? std::string::npos : Args.find_first_not_of(" \t", Args.find_first_of(" \t", Word));
		return DebugSitePlay(Rest == std::string::npos ? std::string() : Args.substr(Rest));
	}
	if (Verb == "describe")
	{
		const site::Book& B = Life.Sites;
		std::string Out = "Sites: RiverLine, linked to the bank (" + Money(BankrollCents) + ").";
		for (const site::Account& A : B.Accounts)
		{
			const net::SiteDef& D = net::Def(A.Site);
			Out += "\n" + D.Name + ": joined " + net::DateLabel(net::DayOf(A.JoinedAt)) + ", balance " + Money(A.Cents) + ", deposited " + Money(A.Deposited) + ", withdrawn " +
				Money(A.Withdrawn) + ", rake " + Money(A.RakeMonth) + " this month (" + Money(A.RakeLife) + " in all), won " + Money(A.LifetimeWon) + " over " +
				std::to_string(A.LifetimeEvents) + " events.";
			if (A.BonusPending > 0 || A.BonusReleased > 0)
			{
				Out += " Welcome bonus: " + Money(A.BonusReleased) + " released, " + Money(A.BonusPending) + " pending until " + net::DateLabel(net::DayOf(A.BonusExpires));
				// Update 3 (§2.6 F11): the Fuel Gauge's meter, as the pages read it (site::show::Welcome).
				const site::show::WelcomeView V = site::show::Welcome(A, World);
				Out += V.NextIn > 0 ? "; the next " + Money(std::min<Chips>(500, V.Pending)) + " in " + Money(V.NextIn) + " of rake." : std::string(".");
			}
			if (A.Site == net::SiteId::RunItTwice)
			{
				const int Tier = site::TierFor(site::SecondRunTiers(), A.RakeMonth);
				Out += std::string(" Second Run Club: ") + site::SecondRunTiers()[static_cast<size_t>(Tier)].Name + " this month.";
			}
		}
		for (const site::Transfer& X : B.Transfers)
		{
			if (X.State == 0)
			{
				Out += "\nIn transit: " + Money(X.Cents - X.FeeCents) + " from " + net::Def(X.Site).Name + ", arriving " + SitesWhen(X.ArrivesAt) + " (" + X.Id + ").";
			}
		}
		const int Tier = site::TierFor(site::RewardsTiers(), B.RiverLine.Rake);
		Out += "\nRiverLine Rewards: " + Money(B.RiverLine.Rake) + " of rake in " + (B.RiverLine.MonthKey.empty() ? std::string("this month") : site::MonthName(B.RiverLine.MonthKey)) +
			" (" + site::RewardsTiers()[static_cast<size_t>(Tier)].Name + "), last paid tier " + site::RewardsTiers()[static_cast<size_t>(std::max(0, std::min(4, B.RiverLine.Tier)))].Name +
			(B.RiverLine.BoostUntil > World ? ", one tier up until " + net::DateLabel(net::DayOf(B.RiverLine.BoostUntil)) : std::string()) + ".";
		rep::Public P = Record();
		rep::FromHistory(P, History);
		for (const net::SiteDef& D : net::Sites())
		{
			if (!site::IsRival(D.Id))
			{
				continue;
			}
			std::string State = "not yet";
			for (const site::Invitation& I : B.Invites)
			{
				State = I.Site != D.Id ? State
					: I.State == 1		? "accepted " + net::DateLabel(net::DayOf(I.At))
					: I.State == 2		? "declined " + net::DateLabel(net::DayOf(I.At))
										: "sent " + net::DateLabel(net::DayOf(I.At)) + ", unanswered";
			}
			// When it opens in this world (World::SiteLaunched; its own day before the world says, C3).
			const double Opens = LivingWorld.SiteLaunched(D.Id) >= 0.0 ? LivingWorld.SiteLaunched(D.Id) : static_cast<double>(D.LaunchDay) * net::MinutesPerDay;
			const int Level = site::IgniteLevelOf(Life);
			Out += "\n" + D.Name + "'s gate (" + (site::GateMet(D.Id, P, Life.RentsPaid, World, nullptr, Level) ? "met" : "not met") + "; invitation " + State + "): " +
				(Opens <= World ? "open since " : "opens ") + net::DateLabel(net::DayOf(Opens)) + ", rents paid " + std::to_string(Life.RentsPaid) + "/" +
				std::to_string(D.Gate.MinRentsPaid) + ", paid online events " +
				std::to_string(std::max(P.OnlineEvents, P.Paid100)) + "/" + std::to_string(D.Gate.MinEvents);
			if (D.Gate.MinAbi100 > 0)
			{
				Out += ", last-100 average " + Money(P.Abi100) + "/" + Money(D.Gate.MinAbi100);
			}
			if (D.Gate.SeriesTitle || D.Gate.KastPartner)
			{
				Out += "; or " + std::to_string(P.OnlineSeriesTitles) + " online series titles (1), or Kast Partner (" + (P.Partner ? "yes" : "no") + ")";
			}
			// Update 3 G2 (§11 Q4): the easier doors, when the site has them.
			if (D.Gate.MinEvents2 > 0)
			{
				Out += "; or " + std::to_string(std::max(P.OnlineEvents, P.Paid100)) + "/" + std::to_string(D.Gate.MinEvents2) + " at a last-100 average of " + Money(P.Abi100) + "/" +
					Money(D.Gate.MinAbi100_2);
			}
			if (D.Gate.IgniteLevel > 0)
			{
				Out += "; or Ignite Level " + std::to_string(Level) + "/" + std::to_string(D.Gate.IgniteLevel);
			}
			Out += ".";
		}
		return Out;
	}
	if (Verb == "invite")
	{
		if (!site::IsRival(Named))
		{
			return "ss.Site.Invite <ignite|runittwice>";
		}
		if (site::AccountOf(Life.Sites, Named))
		{
			return site::FillCopy(site::CopyText("err:joined"), {{"Site", net::Def(Named).Name}});
		}
		// The letter gives the reason the record meets (its gate's), or, forced past every gate, none it can't back up (-1:
		// why:hello).
		rep::Public P = Record();
		rep::FromHistory(P, History);
		int Why = -1;
		Why = site::GateMet(Named, P, Life.RentsPaid, World, &Why, site::IgniteLevelOf(Life)) ? Why : -1;
		site::Invite(Life.Sites, Life, Named, World, Why, false);
		Save();
		return net::Def(Named).Name + "'s invitation is in Mail.";
	}
	if (Verb == "accept" || Verb == "decline")
	{
		// ss.Site.Accept / ss.Site.Decline <site>: Mail's buttons on a site's invitation.
		if (!site::IsRival(Named))
		{
			return "ss.Site." + std::string(Verb == "accept" ? "Accept" : "Decline") + " <ignite|runittwice>";
		}
		const std::string Why = AnswerInvite(Named, Verb == "accept");
		return Why.empty() ? (Verb == "accept" ? "Accepted. " : "Declined. ") + DebugSite("describe") : Why;
	}
	if (Verb == "launch")
	{
		// The world keeps when each rival opened (core-world's World::SiteLaunched): the world's debug command moves it.
		if (!site::IsRival(Named))
		{
			return "ss.Site.Launch <ignite|runittwice>";
		}
		return DebugWorld("sitelaunch " + std::string(net::KeyOf(Named)));
	}
	if (Verb == "deposit" || Verb == "withdraw")
	{
		if (W.size() < 3 || Named == net::SiteId::Count)
		{
			return Usage;
		}
		const std::string Method = W.size() > 3 ? W[3] : std::string();
		const Chips Cents = SitesDollars(W[2]);
		const std::string Why = Verb == "deposit" ? Deposit(Named, Cents, Method) : Withdraw(Named, Cents, Method);
		return Why.empty() ? (Verb == "deposit" ? "Deposited " : "Withdrawal requested: ") + Money(Cents) + ". " + DebugSite("describe") : Why;
	}
	if (Verb == "advance")
	{
		// The sites' calendar runs Hours ahead while the clock stays where it is (a test's fast-forward), so whatever that brings
		// (a withdrawal landing, the 1st's rakeback, a letter) happens now and is dated now: a bank line, a wallet line or a
		// letter dated after the clock would sit in the future of every screen (C3).
		const double Hours = W.size() > 1 ? std::max(0.0, std::atof(W[1].c_str())) : 24.0;
		SiteStep(World, World + Hours * 60.0);
		for (life::LedgerEntry& E : Life.Ledger)
		{
			E.At = std::min(E.At, World);
		}
		for (life::Letter& M : Life.Inbox)
		{
			M.At = std::min(M.At, World);
		}
		for (site::Account& A : Life.Sites.Accounts)
		{
			for (site::Line& L : A.Lines)
			{
				L.At = std::min(L.At, World);
			}
		}
		for (site::Transfer& X : Life.Sites.Transfers)
		{
			X.ArrivesAt = X.State != 0 ? std::min(X.ArrivesAt, World) : X.ArrivesAt;
		}
		Save();
		return "The sites' calendar moved on " + Fixed(Hours, 0) + " hours. " + DebugSite("describe");
	}
	return Usage;
}
} // namespace ss
