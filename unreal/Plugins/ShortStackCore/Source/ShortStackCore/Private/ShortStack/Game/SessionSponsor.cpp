#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"

// The session's side of sponsors (next_segment_spec §3, §7.2): the Session.h block's sponsor calls, the calendar's
// SponsorStep, Kast's offers (AcceptDeal forwards to SignDeal; DeclineDeal) and the hook sites' seams (SponsorStream,
// SponsorStreamDone, SponsorCovers). core-sponsor.
//
// Phase A, days 2-3: stubs that keep today's behavior. Kast's own deals are still the deals: SignDeal and DeclineDeal
// are Kast's accept and decline as they were (Session.cpp:4886-4921 at c09bc31), TeamRiverLine reads the Kast deal
// (Session.h:642), the stream keeps working on Channel.Deals (SponsorStream and SponsorStreamDone do nothing), nothing
// is covered, and the book's own actions are refused in the game's voice. core-sponsor moves all of it onto the book.

namespace ss
{
std::string Session::SignDeal(const std::string& Id)
{
	const kast::Sponsor* S = kast::FindSponsor(Id);
	if (!S || !Channel.Offers.count(Id))
	{
		return "No offer.";
	}
	const double World = WorldMinutes();
	int Active2 = 0;
	for (const kast::Deal& D : Channel.Deals)
	{
		Active2 += D.Until > World ? 1 : 0;
	}
	if (Active2 >= kast::MaxDeals)
	{
		return "Both sponsor slots are taken.";
	}
	kast::Deal D;
	D.Id = Id;
	D.Since = World;
	D.Until = World + static_cast<double>(S->Days) * net::MinutesPerDay;
	Channel.Deals.push_back(D);
	Channel.Offers.erase(Id);
	Hooks.Text(S->Brand, "Welcome aboard! The deal runs " + std::to_string(S->Days) + " days. We pay " + Money(S->PerHourCents) + " for every hour you're live" + (S->ReadCents > 0 ? ", plus " + Money(S->ReadCents) + " a read." : "."));
	Sound(SoundId::Cash, 0.8);
	Save();
	return "";
}

void Session::DeclineDeal(const std::string& Id)
{
	if (Channel.Offers.erase(Id))
	{
		Channel.Declined.insert(Id);
		Save();
	}
}

std::string Session::CounterDeal(const std::string& /*ContractId*/, int /*Lever*/)
{
	return "There's no deal on the table.";
}

std::string Session::GiveNotice(const std::string& /*ContractId*/)
{
	return "There's no deal to end.";
}

std::string Session::AskForRaise(const std::string& /*ContractId*/)
{
	return "There's no deal to talk about.";
}

std::string Session::SewPatch(int /*Spot*/, const std::string& /*BrandId*/)
{
	return "Nobody's given you a patch to wear.";
}

std::string Session::Appear(const std::string& Venue)
{
	return sponsor::Appear(Life.Sponsors, Venue, WorldMinutes());
}

int Session::SponsorTier() const
{
	return 0;
}

bool Session::TeamRiverLine() const
{
	return Channel.ActiveDeal("riverline", WorldMinutes()) != nullptr;
}

void Session::SponsorStep(double /*From*/, double /*To*/)
{
}

std::string Session::DebugSponsor(const std::string& /*Args*/)
{
	return "ss.Sponsor: no contracts yet; Kast's deals are still Kast's.";
}

void Session::SponsorStream(kast::Inputs& /*In*/) const
{
}

void Session::SponsorStreamDone()
{
}

bool Session::SponsorCovers(const std::string& /*GearId*/) const
{
	return false;
}
} // namespace ss
