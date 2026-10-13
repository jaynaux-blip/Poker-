#include "ShortStack/Game/Sites.h"

#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Rep.h"

// The poker sites' data (update 2 §5.3) and the money rules' entry points (§5.5-§5.7). Phase A, day 1 (the lead): the
// registry holds RiverLine alone, as it is today (its rake and traffic are today's numbers), and the money rules are stubs
// that move nothing and write no line, so every save writes back as it was read. The routing (Site, SiteOf, ForEvent,
// AttachAll, SetHeroAll) lives in Network.cpp. core-sites owns this file from here (Flopjoy's and Turnstone's SiteDefs,
// and SiteMoney.cpp beside it); dormant until G3.

namespace ss
{
namespace net
{
namespace sites_detail
{
SiteDef SitesRiverLine()
{
	SiteDef D;
	D.Id = SiteId::RiverLine;
	D.Key = "riverline";
	D.Name = "RiverLine";
	D.Tagline = "Tournaments around the clock.";
	D.Rep = "RiverLine Player Relations";
	D.RepTitle = "Player Relations";
	D.TeamName = "Team RiverLine";
	// pal:: in order (Ui.h): Bg, Bg2, Panel, Panel2, Line, Ink, Muted, Dim, Accent, Accent2, Gold, Red, Orange, Green, Felt, FeltDark.
	D.Palette = {0x0a111c, 0x0e1726, 0x111c2e, 0x16233a, 0x223352, 0xe6edf7, 0x7b8aa3, 0x4d5b73, 0x27d3c3, 0x3b82f6, 0xf2c14e, 0xef4d5a, 0xf28a3a, 0x3ecf6e, 0x0f6a4e,
		0x083a2b};
	D.Rake.Kind = 0;
	D.Linked = true; // the bank's site: no wallet, no fees, no delays (U8)
	return D;
}
} // namespace sites_detail

Chips RakeRule::Fee(Chips BuyIn) const
{
	// Kind 0: RiverLine's (FeeOf). The rivals' rules are core-sites'.
	return Kind == 0 ? FeeOf(BuyIn) : static_cast<Chips>(static_cast<double>(BuyIn) * Flat);
}

double RakeRule::PrizeShare(const EventTemplate& T) const
{
	// Kind 0: RiverLine's (Network.cpp's prize share: 0.915 of a paid event's buy-in, after its fee).
	return T.BuyInCents == 0 ? 0.0 : 0.915;
}

const std::vector<SiteDef>& Sites()
{
	static const std::vector<SiteDef> All = {sites_detail::SitesRiverLine()};
	return All;
}

const SiteDef& Def(SiteId S)
{
	for (const SiteDef& D : Sites())
	{
		if (D.Id == S)
		{
			return D;
		}
	}
	return Sites().front();
}
} // namespace net

namespace site
{
std::string Join(Book& B, net::SiteId S, double World)
{
	(void)B;
	(void)S;
	(void)World;
	return "Only RiverLine's open to you.";
}

void AddLine(Book& B, net::SiteId S, double At, int Kind, Chips Cents, const std::string& Label)
{
	(void)B;
	(void)S;
	(void)At;
	(void)Kind;
	(void)Cents;
	(void)Label;
}

std::string Deposit(Book& B, life::State& L, Chips& Bankroll, net::SiteId S, Chips Cents, const std::string& Method, double World)
{
	(void)B;
	(void)L;
	(void)Bankroll;
	(void)S;
	(void)Cents;
	(void)Method;
	(void)World;
	return "Only RiverLine's open to you.";
}

std::string Withdraw(Book& B, net::SiteId S, Chips Cents, const std::string& Method, double World)
{
	(void)B;
	(void)S;
	(void)Cents;
	(void)Method;
	(void)World;
	return "Only RiverLine's open to you.";
}

std::string Reverse(Book& B, const std::string& TransferId, double World)
{
	(void)B;
	(void)TransferId;
	(void)World;
	return "Only RiverLine's open to you.";
}

Chips* WalletOf(Book& B, net::SiteId S)
{
	// RiverLine has no wallet: its money is the bank's. Phase A: no rival account exists.
	(void)B;
	(void)S;
	return nullptr;
}

void OnBuyIn(Book& B, net::SiteId S, Chips BuyIn, Chips Fee, const std::string& Label, double World)
{
	// Phase A's stub: the rake into RiverLine Rewards or a rival's VIP month is core-sites' (G3).
	(void)B;
	(void)S;
	(void)BuyIn;
	(void)Fee;
	(void)Label;
	(void)World;
}

void OnPrize(Book& B, net::SiteId S, Chips Cents, const std::string& Label, double World, const Season& Add)
{
	(void)B;
	(void)S;
	(void)Cents;
	(void)Label;
	(void)World;
	(void)Add;
}

void Step(Book& B, life::State& L, Chips& Bankroll, const rep::Public& P, int SponsorTier, double From, double To, std::vector<std::pair<std::string, std::string>>& Mail)
{
	// Phase A's stub: nothing to step (a no-op until G3).
	(void)B;
	(void)L;
	(void)Bankroll;
	(void)P;
	(void)SponsorTier;
	(void)From;
	(void)To;
	(void)Mail;
}

bool RentWarning(const Book& B, Chips Bankroll, Chips RentDue, double Deadline, double World, std::string& Line)
{
	(void)B;
	(void)Bankroll;
	(void)RentDue;
	(void)Deadline;
	(void)World;
	(void)Line;
	return false;
}

std::vector<life::Mark> Marks(const Book& B, double From, double To)
{
	(void)B;
	(void)From;
	(void)To;
	return {};
}

std::vector<std::string> Inventory()
{
	return {};
}

void WriteLines(const life::State& L, std::string& Out)
{
	// Phase A's stub: no site line yet, so every save writes back as it was read.
	(void)L;
	(void)Out;
}

bool ReadLine(const std::vector<std::string>& P, life::State& L)
{
	(void)P;
	(void)L;
	return false;
}

void AfterParse(SaveData& D)
{
	(void)D;
}
} // namespace site
} // namespace ss
