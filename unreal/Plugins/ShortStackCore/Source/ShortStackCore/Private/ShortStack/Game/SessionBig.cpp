// Update 4, the session's side of the Big Field (SPEC v1.7 §4.2-§4.15): the unlock and the reveal, the plans and the stops, the
// beats, CHIP COUNTS, the broadcast kit's view and the honorifics, the play-on and its delivery, late registration, re-entry, YOUR
// RUN's record and ss.Big.*. B-session's file from G3.
//
// Phase A (§6 PA-39, PA-42; the lead): stubs that keep today's behaviour. Re-entry refuses, there are no play-ons to drain, the two
// views are empty and off, and ss.Big.* answers that it isn't built yet. Step A-2 (the lead; LEAD_CALLS Q-5): PlayOnStep and
// RunNote do nothing (nothing calls them before G3), and PayEntry is RegisterEvent's payment block, extracted verbatim.
#include "ShortStack/Game/BigField.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/SitePlay.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Stake.h"

#include <string>

namespace ss
{
std::string Session::Reenter(const std::string& /*EventId*/)
{
	return "Not yet.";
}

void Session::DrainPlayOns()
{
	// Phase A: PlayOns is always empty (nothing moves a busted run there before G3).
}

const bigfield::CountsView& Session::CountsView() const
{
	static const bigfield::CountsView BigNoCounts;
	return BigNoCounts;
}

const bigfield::Broadcast& Session::BroadcastView() const
{
	static const bigfield::Broadcast BigNoBroadcast;
	return BigNoBroadcast;
}

std::string Session::DebugBig(const std::string& /*Args*/)
{
	return "Not built yet: the Big Field comes with Update 4's G3.";
}

void Session::PlayOnStep()
{
	// Phase A: PlayOns is always empty, and Update doesn't call this before G3 (B-session, §4.10.1 item 3).
}

void Session::RunNote()
{
	// Phase A: nothing is collected, and nothing calls this before G3 (B-session, §4.15.1).
}

void Session::PayEntry(const LobbyEvent& Ev, Chips RakeOverride, int /*Bullet*/)
{
	// §6 PA-39: RegisterEvent's payment block, verbatim (bullet 1 writes the same ledger lines in the same order). G3 (B-session,
	// §4.13.1 item 3) gives bullet 2 on its own "Re-entry: {name}" lines and its reentry:<instance>#<bullet> key.
	if (TicketsFor(Ev) > 0)
	{
		const std::string Tid = net::ForEvent(Ev.Spec.Id).TicketOf(Ev.Spec.Id.substr(0, Ev.Spec.Id.find('@')));
		if (--Life.Tickets[Tid] <= 0)
		{
			Life.Tickets.erase(Tid);
		}
		// The entry's line goes in its site's ledger (update 2 §1.2): RiverLine's is the bank's; a rival's ticket never
		// touches the bank, so its line is the site's own.
		const net::SiteId TicketSite = net::SiteOf(Ev.Spec.Id);
		if (TicketSite == net::SiteId::RiverLine)
		{
			Life.Record(WorldMinutes(), "Ticket: " + Ev.Spec.Name, 0, 0);
		}
		else
		{
			site::AddLine(Life.Sites, TicketSite, WorldMinutes(), 0, 0, "Ticket: " + Ev.Spec.Name);
		}
	}
	else
	{
		// Who pays (next_segment_spec §1.2): a package's roll (its own ledger takes the buy-in), otherwise the player, from
		// the event's site's pot (update 2 §1.2: RiverLine's is the bankroll and the bank's line; a rival's is its wallet,
		// whose line site::OnBuyIn writes). OnBuyIn also takes the rake into RiverLine Rewards or the site's VIP month.
		const life::Funding Fund = stake::FundFor(Life.Stakes, Ev.Spec.Id, Ev.BuyInCents, false, WorldMinutes());
		const net::SiteId Site = net::SiteOf(Ev.Spec.Id);
		if (Fund.From != life::Paid::Package)
		{
			stake::Pot Pot = PotFor(Site);
			if (!Pot.Cents)
			{
				Pot = stake::Pot{&BankrollCents, 0};
			}
			*Pot.Cents -= Ev.BuyInCents;
			if (Ev.BuyInCents > 0 && Pot.Site == 0)
			{
				Life.Record(WorldMinutes(), "Buy-in: " + Ev.Spec.Name, -Ev.BuyInCents, 0);
			}
		}
		// Update 3 (§2.6 F7): the fee the rake hooks see is the override when there is one (FUSE's real fee, never its spec's).
		const Chips RakeFee = RakeOverride >= 0 ? RakeOverride : Ev.Spec.FeeCents;
		site::OnBuyIn(Life.Sites, Site, Ev.BuyInCents, RakeFee, "Buy-in: " + Ev.Spec.Name, WorldMinutes());
		// Update 3 (W4, §7.2; core-siteplay's body at G2): a rival entry paid from its wallet moves the site's loops, with the
		// same fee. Not a ticket (the branch above), a freeroll or a package's roll (it pays RiverLine's only).
		if (Site != net::SiteId::RiverLine && Fund.From != life::Paid::Package && Ev.BuyInCents > 0)
		{
			const net::EventTemplate* EntryTmpl = net::ForEvent(Ev.Spec.Id).FindTemplate(Ev.Spec.Id.substr(0, Ev.Spec.Id.find('@')));
			const int EntryFormat = static_cast<int>(EntryTmpl ? EntryTmpl->Fmt : net::Format::Freezeout);
			site::play::OnPaidEntry(Life.SitePlay, Life, Site, RakeFee, Ev.BuyInCents, EntryFormat, Ev.Spec.Id, WorldMinutes());
		}
		StakeEntered(Ev.Spec.Id, Ev.BuyInCents, Fund);
	}
}
} // namespace ss
