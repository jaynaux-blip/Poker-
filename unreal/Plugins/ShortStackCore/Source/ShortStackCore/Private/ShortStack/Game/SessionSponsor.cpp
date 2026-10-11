#include "ShortStack/Game/Session.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Sponsor.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

// The session's side of sponsors (next_segment_spec §3, §7.2): the Session.h block's sponsor calls, the calendar's
// SponsorStep, Kast's offers (AcceptDeal forwards to SignDeal; DeclineDeal) and the hook sites' seams (SponsorStream,
// SponsorStreamDone, SponsorCovers). core-sponsor.
//
// The book is life::State::Sponsors (sponsor::Book); everything here is a thin wrapper that adds what only the session
// has: the gear a brand sends (GearDrop's owned list), the phone and the Mail chime, the public record, the save. Kast
// works on the book's view: StreamInputs fills Inputs::Deals (SponsorStream) and every Kast call hands back what it did
// (SponsorStreamDone): Channel::Deals is never written.

namespace ss
{
namespace session_sponsor_detail
{
constexpr double DayMinutes = 1440.0;

/** Halo's camera: the next one up from what's on the desk ("" when the best is there already). */
std::string NextCamera(const gear::Owned& Gear)
{
	for (const char* Id : {"webcam-720", "webcam-1080", "mirrorless"})
	{
		if (Gear.count(Id) == 0)
		{
			return Id;
		}
	}
	return std::string();
}

std::vector<std::string> Words(const std::string& Args)
{
	std::vector<std::string> Out;
	std::istringstream In(Args);
	std::string W;
	while (In >> W)
	{
		Out.push_back(W);
	}
	return Out;
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = static_cast<char>(std::tolower(static_cast<unsigned char>(Ch)));
	}
	return S;
}

const char* StateName(sponsor::State S)
{
	switch (S)
	{
	case sponsor::State::Offered: return "offered";
	case sponsor::State::Countered: return "countered";
	case sponsor::State::Active: return "active";
	case sponsor::State::Notice: return "notice";
	case sponsor::State::Ended: return "ended";
	}
	return "?";
}

/** A contract id from what the player or Kast's old UI passed: the id itself, or a brand's open offer. */
std::string Resolve(const sponsor::Book& B, const std::string& Id, double World)
{
	if (sponsor::Find(B, Id))
	{
		return Id;
	}
	const std::string Open = sponsor::OpenOfferFor(B, Id, World);
	if (!Open.empty())
	{
		return Open;
	}
	const sponsor::Contract* C = sponsor::Running(B, Id, World);
	return C ? C->Id : Id;
}
} // namespace session_sponsor_detail

std::string Session::SignDeal(const std::string& Id)
{
	using namespace session_sponsor_detail;
	const double World = WorldMinutes();
	const std::string ContractId = Resolve(Life.Sponsors, Id, World);
	const std::string Why = sponsor::Sign(Life.Sponsors, Life, BankrollCents, ContractId, World);
	if (!Why.empty())
	{
		return Why;
	}
	// What the brand sends: the membership paid from today; Halo's next camera up and Sitwell's chair, the brand's
	// property while the deal runs (the pawn won't take them, §3.6).
	if (sponsor::Contract* C = sponsor::Find(Life.Sponsors, ContractId))
	{
		bool Changed = false;
		for (const sponsor::Perk& P : C->T.Perks)
		{
			if (P.Kind != sponsor::Gift::Gear)
			{
				continue;
			}
			if (P.Id == "gym")
			{
				if (!Owns("gym") && gear::Find("gym"))
				{
					Gear["gym"] = World + 30.0 * DayMinutes;
					Hooks.GearChanged("gym", true);
					Changed = true;
				}
				continue;
			}
			const std::string Item = P.Id == "camera" ? NextCamera(Gear) : P.Id;
			const gear::Item* I = Item.empty() ? nullptr : gear::Find(Item);
			if (!I || Owns(Item))
			{
				continue;
			}
			Gear[Item] = I->Monthly ? World + 30.0 * DayMinutes : 0.0;
			C->Property.push_back("gear:" + Item);
			Hooks.GearChanged(Item, true);
			Changed = true;
		}
		if (Changed)
		{
			RefreshGear();
		}
	}
	Life.Sponsors.NextAt = 0.0;
	Sound(SoundId::Cash, 0.8);
	Save();
	return "";
}

void Session::DeclineDeal(const std::string& Id)
{
	const double World = WorldMinutes();
	const std::string ContractId = session_sponsor_detail::Resolve(Life.Sponsors, Id, World);
	const sponsor::Contract* C = sponsor::Find(Life.Sponsors, ContractId);
	if (!C || (C->St != sponsor::State::Offered && C->St != sponsor::State::Countered))
	{
		return;
	}
	sponsor::Decline(Life.Sponsors, ContractId, World);
	Life.Sponsors.NextAt = 0.0;
	Save();
}

std::string Session::CounterDeal(const std::string& ContractId, int Lever)
{
	if (Lever < 0 || Lever > static_cast<int>(sponsor::Lever::NoExclusivity))
	{
		return "That's not something they'd move on.";
	}
	const double World = WorldMinutes();
	const std::string Why = sponsor::Counter(Life.Sponsors, Record(), session_sponsor_detail::Resolve(Life.Sponsors, ContractId, World), static_cast<sponsor::Lever>(Lever), World);
	if (Why.empty())
	{
		Sound(SoundId::Click, 0.6);
		Save();
	}
	return Why;
}

std::string Session::GiveNotice(const std::string& ContractId)
{
	const double World = WorldMinutes();
	const std::string Why = sponsor::GiveNotice(Life.Sponsors, Life, session_sponsor_detail::Resolve(Life.Sponsors, ContractId, World), World);
	if (Why.empty())
	{
		Sound(SoundId::Mail, 0.6);
		Save();
	}
	return Why;
}

std::string Session::AskForRaise(const std::string& ContractId)
{
	const double World = WorldMinutes();
	const std::string Why = sponsor::AskForRaise(Life.Sponsors, Record(), session_sponsor_detail::Resolve(Life.Sponsors, ContractId, World), World);
	if (Why.empty())
	{
		Sound(SoundId::Click, 0.6);
		Save();
	}
	return Why;
}

std::string Session::SewPatch(int Spot, const std::string& BrandId)
{
	const std::string Why = sponsor::Sew(Life.Closet, Life.Sponsors, Spot, BrandId, WorldMinutes());
	if (Why.empty())
	{
		Sound(SoundId::Click, 0.5);
		Save();
	}
	return Why;
}

std::string Session::Appear(const std::string& Venue)
{
	// An appearance is time spent at the keeper (the photo with Ray: 20 minutes; Lonnie's window chair: 30), inside its
	// window: a skip at the venue, counted when it ends (FinishSkip: sponsor::Appear at "shop:<venue>:appear"). The desk's
	// meet-the-regulars is the card room's (its desk screen calls sponsor::Appear on the save).
	if (TimeSkip.Active)
	{
		return "You're busy.";
	}
	if (T)
	{
		return "Finish your tournament first.";
	}
	const double World = WorldMinutes();
	const double Minutes = Venue == "barber" ? 30.0 : 20.0;
	bool Open = false;
	bool Done = false;
	std::string Label;
	for (const sponsor::Window& W : sponsor::Appearances(Life.Sponsors, World, World + 1.0))
	{
		if (W.Venue == Venue && World >= W.From && World + Minutes <= W.To)
		{
			Open = Open || !W.Done;
			Done = Done || W.Done;
			Label = W.Label;
		}
	}
	if (!Open)
	{
		return Done ? "You've done that one this month." : "Nobody's expecting you.";
	}
	if (Venue != "store" && Venue != "barber")
	{
		return "That one's at the desk.";
	}
	TimeSkip = Skip();
	TimeSkip.Active = true;
	TimeSkip.From = World;
	TimeSkip.To = World + Minutes;
	TimeSkip.RealStart = Now;
	TimeSkip.RealSeconds = 2.5;
	TimeSkip.Rest = 1.0;
	TimeSkip.Couch = Evicted();
	TimeSkip.Label = Label;
	TimeSkip.Result.ActivityId = "shop:" + Venue + ":appear";
	TimeSkip.Result.Start = World;
	TimeSkip.Result.End = World + Minutes;
	HasOutcome = false;
	Sound(SoundId::Click, 0.6);
	return "";
}

int Session::SponsorTier() const
{
	return sponsor::TierOf(Record(), Life.Sponsors, Life, std::string(), WorldMinutes());
}

bool Session::TeamRiverLine() const
{
	return sponsor::Active(Life.Sponsors, "riverline", WorldMinutes());
}

void Session::SponsorStep(double From, double To)
{
	sponsor::Book& B = Life.Sponsors;
	if (To < B.NextAt)
	{
		return; // nothing due before then (a look, a review, a reply, a perk); every change outside the step resets it
	}
	std::vector<std::pair<std::string, std::string>> Texts;
	sponsor::StepContext Ctx;
	Ctx.Gear = &Gear;
	Ctx.StreamStart = Stream.Live ? Stream.StartWorld : -1.0;
	Ctx.Texts = &Texts;
	const size_t Letters = Life.Inbox.size();
	const double Newest = Life.Inbox.empty() ? -1.0e18 : Life.Inbox.back().At;
	const std::string NewestThread = Life.Inbox.empty() ? std::string() : Life.Inbox.back().Thread + "|" + Life.Inbox.back().Subject;
	sponsor::Step(B, Life, BankrollCents, Channel, Record(), From, To, SeedBase, Ctx);
	for (const std::pair<std::string, std::string>& T2 : Texts)
	{
		Hooks.Text(T2.first, T2.second);
	}
	const bool NewMail = Life.Inbox.size() != Letters ||
		(!Life.Inbox.empty() && (Life.Inbox.back().At != Newest || Life.Inbox.back().Thread + "|" + Life.Inbox.back().Subject != NewestThread));
	if (NewMail)
	{
		Sound(SoundId::Mail, 0.6);
	}
	// Gear a brand sent goes back when a deal ends for cause or conduct (it's the player's after a clean end).
	bool Returned = false;
	for (sponsor::Contract& C : B.Contracts)
	{
		if (C.St != sponsor::State::Ended || C.Kept || C.Property.empty())
		{
			continue;
		}
		for (const std::string& P : C.Property)
		{
			if (P.rfind("gear:", 0) == 0 && Gear.erase(P.substr(5)) > 0)
			{
				Hooks.GearChanged(P.substr(5), false);
				Returned = true;
			}
		}
		C.Property.clear();
	}
	if (Returned)
	{
		RefreshGear();
	}
}

std::string Session::DebugSponsor(const std::string& Args)
{
	using namespace session_sponsor_detail;
	const std::vector<std::string> W = Words(Args);
	const std::string Cmd = W.empty() ? std::string("describe") : Lower(W[0]);
	const double World = WorldMinutes();
	sponsor::Book& B = Life.Sponsors;
	B.NextAt = 0.0;
	if (Cmd == "standing" && W.size() >= 2)
	{
		// A standing to test with (not saved), and the block road at the store, so Ray has something to say.
		B.DebugTier = std::clamp(std::atoi(W[1].c_str()), 0, 5);
		const int Today = net::DayOf(World);
		std::vector<int>& Days = B.Days["store"];
		for (int D = Today - 13; D <= Today; ++D)
		{
			if (std::find(Days.begin(), Days.end(), D) == Days.end())
			{
				Days.push_back(D);
			}
		}
		std::sort(Days.begin(), Days.end());
		SponsorStep(World, World);
		return "Standing " + std::to_string(B.DebugTier) + ": " + sponsor::StandingWords(B.DebugTier) + ".";
	}
	if ((Cmd == "offer" || Cmd == "sign") && W.size() >= 2)
	{
		const std::string Brand = Lower(W[1]);
		const sponsor::Brand* Br = sponsor::FindBrand(Brand);
		if (!Br)
		{
			return "No brand '" + Brand + "'.";
		}
		std::string Id = sponsor::OpenOfferFor(B, Brand, World);
		if (Id.empty())
		{
			Id = sponsor::MakeOffer(B, Life, Record(), Brand, World, SeedBase, true);
		}
		if (Id.empty())
		{
			return Br->Short + ": " + sponsor::Eligible(B, Life, Record(), Brand, World, &Gear);
		}
		sponsor::HeardOffer(B, Life, Id, World);
		if (Cmd == "offer")
		{
			Save();
			return Br->Short + " offered: " + Id + ".";
		}
		const std::string Why = SignDeal(Id);
		return Why.empty() ? Br->Short + " signed: " + Id + "." : Why;
	}
	if (Cmd == "period")
	{
		// The running contracts' current period ends now: its review, its stipend, its strike (a test's fast-forward).
		for (sponsor::Contract& C : B.Contracts)
		{
			if (!sponsor::Running(B, C.Brand, World) || sponsor::Running(B, C.Brand, World) != &C)
			{
				continue;
			}
			const double Ends = sponsor::PeriodEnd(C, C.Period + 1);
			const double Shift = std::max(0.0, Ends - World);
			C.Since -= Shift;
			C.Until -= Shift;
			C.CureUntil = C.CureUntil > 0.0 ? C.CureUntil - Shift : 0.0;
		}
		SponsorStep(World - 0.01, World);
		Save();
		return "Reviewed.";
	}
	if (Cmd == "breach" && W.size() >= 2)
	{
		Conduct(Lower(W[1]) == "restricted" ? sponsor::Breach::AccountRestricted : sponsor::Breach::PickedUp, World);
		SponsorStep(World, World);
		Save();
		return "Breach: " + Lower(W[1]) + ".";
	}
	if (Cmd == "patch" && W.size() >= 2)
	{
		const std::string Spot = Lower(W[1]);
		const int I = Spot == "chest" ? 0 : Spot == "sleeve" ? 1 : Spot == "cap" ? 2 : std::atoi(Spot.c_str());
		const std::string Why = SewPatch(I, W.size() >= 3 ? Lower(W[2]) : std::string());
		return Why.empty() ? "Sewn." : Why;
	}
	if (Cmd == "appear" && W.size() >= 2)
	{
		const std::string Why = sponsor::Appear(B, Lower(W[1]), World);
		return Why.empty() ? "Counted." : Why;
	}
	// Describe.
	const rep::Public P = Record();
	std::string Out = "Standing " + std::to_string(SponsorTier()) + " (" + sponsor::StandingWords(SponsorTier()) + ") \xC2\xB7 credit " + Money(B.StoreCreditCents) + "\n";
	for (const sponsor::Contract& C : B.Contracts)
	{
		Out += "  " + C.Id + " " + StateName(C.St) + " " + sponsor::Pill(C, World) + " period " + std::to_string(C.Period) + "/" + std::to_string(sponsor::Periods(C)) + " strikes " +
			std::to_string(C.Strikes) + " paid " + Money(C.PaidCents) + "\n";
	}
	for (const sponsor::Brand& Br : sponsor::Brands())
	{
		const std::string Why = sponsor::Eligible(B, Life, P, Br.Id, World, &Gear);
		Out += "  " + Br.Id + ": " + (Why.empty() ? std::string("would offer") : Why) + "\n";
	}
	for (int I = 0; I < sponsor::PatchSpots; ++I)
	{
		Out += std::string("  patch ") + sponsor::SpotWord(static_cast<sponsor::Spot>(I)) + ": " + (Life.Closet.Patches[static_cast<size_t>(I)].empty() ? "-" : Life.Closet.Patches[static_cast<size_t>(I)]) + "\n";
	}
	return Out;
}

void Session::SponsorStream(kast::Inputs& In) const
{
	In.Deals = sponsor::StreamView(Life.Sponsors, In.World);
}

void Session::SponsorStreamDone()
{
	sponsor::AbsorbStream(Life.Sponsors, Stream.Deals);
	Stream.Deals.clear();
}

bool Session::SponsorCovers(const std::string& GearId) const
{
	return sponsor::CoversGear(Life.Sponsors, GearId, WorldMinutes());
}
} // namespace ss
