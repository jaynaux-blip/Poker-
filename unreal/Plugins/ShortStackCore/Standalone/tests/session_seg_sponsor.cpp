// night_one_session: core-sponsor's additions (next_segment_spec §7.6, §8.3), compiled into session_test and called
// from main() in session_main.cpp (the shared kit is SessionSeg.h): the SponsorJourney (twelve days at the counter to
// Ray's offer, signed on the counter slip, the credit at the counter and on Penny Drop, a covered Noon Deepstack, a bust);
// the stipend paid at the review, not at signing; notice after one period paying that period once; sponsor gear refused
// at the pawn during the term and sellable after a clean end; a clawback stopping at the rent reserve; the membership's
// renewal skipped; Kast working on the book's view (Channel::Deals never written) from an old Team RiverLine save.
#include "SessionSeg.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Talk.h"

#include <algorithm>
#include <cmath>

namespace session_test
{
namespace sponsor_seg
{
namespace life = ss::life;
namespace live = ss::live;
namespace sponsor = ss::sponsor;
using ss::Chips;

constexpr double Day = 1440.0;

/** The player can go out (the Embercrest's bus), and the phone rings. */
struct OutHooks : Hooks
{
	bool GoOut(const std::string&, Chips) override { return true; }
};

/** Moves the session's clock to ToWorld in one step (the calendar runs the span), then a frame. */
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

Chips Kind11(const ss::Session& S, const std::string& Part)
{
	Chips Sum = 0;
	for (const life::LedgerEntry& E : S.Life.Ledger)
	{
		Sum += E.Kind == life::LedgerSponsors && E.Label.find(Part) != std::string::npos ? E.Amount : 0;
	}
	return Sum;
}

bool Lined(const ss::Session& S, const std::string& Part)
{
	return std::any_of(S.Life.Ledger.begin(), S.Life.Ledger.end(), [&Part](const life::LedgerEntry& E) { return E.Label.find(Part) != std::string::npos; });
}

/** A finished Embercrest night in the player's history, the player wearing Brand's patch. */
void Played(ss::Session& S, double At, const std::string& Brand)
{
	life::LiveEntry E;
	E.Id = "embercrest-turbo@" + std::to_string(ss::net::DayOf(At));
	E.Name = "Midnight Turbo $60";
	E.State = life::LiveEntry::Finished;
	E.PaidCents = 6000;
	E.FeeCents = 800;
	E.RegisteredAt = At - 200.0;
	E.FinishedAt = At;
	E.Place = 30;
	E.Roster = {{"Mei", 0}};
	E.Patches = {{S.HeroName, Brand}};
	S.Life.LiveEntries.push_back(E);
}

/** Twelve days at the Lucky Penny's counter to Ray's offer, signed across the counter, and what it's worth. */
void Journey()
{
	OutHooks H;
	ss::Session S(H, "sponsor-journey");
	S.BankrollCents = 600000; // the rent's collected at its deadline: nothing here is about the rent
	S.Life.Energy = 100.0;
	double Now = 0.0;
	ss::store::Basket Can;
	Can.Add("volt-rush");
	// Twelve purchases on one day are one day.
	{
		Hooks H1;
		ss::Session One(H1, "sponsor-one-day");
		One.BankrollCents = 600000;
		double N1 = Jump(One, 0.0, 2.0 * Day + 11.0 * 60.0);
		for (int K = 0; K < 12; ++K)
		{
			One.Checkout(Can);
		}
		N1 = Jump(One, N1, 9.0 * Day + 11.0 * 60.0);
		Expect(sponsor::VisitDays(One.Life.Sponsors, "store", One.WorldMinutes()) == 1 && sponsor::OpenOfferFor(One.Life.Sponsors, "luckypenny", One.WorldMinutes()).empty(),
			"journey: twelve purchases on one day don't make a regular");
	}
	for (int D = 1; D <= 12; ++D)
	{
		Now = Jump(S, Now, static_cast<double>(D) * Day + 11.0 * 60.0);
		Expect(S.Checkout(Can).empty(), "journey: a can at the counter");
	}
	Expect(sponsor::VisitDays(S.Life.Sponsors, "store", S.WorldMinutes()) == 12, "journey: twelve different days at the counter");
	std::string Offer;
	for (int D = 13; D <= 20 && Offer.empty(); ++D)
	{
		Now = Jump(S, Now, static_cast<double>(D) * Day + 11.0 * 60.0);
		Offer = sponsor::OpenOfferFor(S.Life.Sponsors, "luckypenny", S.WorldMinutes());
	}
	Expect(!Offer.empty(), "journey: the owner's offer is on the table within the week");
	Expect(std::none_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [&Offer](const life::Letter& M) { return M.Thread == Offer; }), "journey: no letter yet: Ray says it first");
	// Ray says it at the counter, once; the talk card has Hear him out.
	ss::town::Talk T = ss::town::Open(*ss::town::ContextFor(S, "ray", "store"));
	Expect(T.Topic == "sponsor:luckypenny" && T.Line.find("corkboard") != std::string::npos, "journey: Ray's pitch, first thing");
	Expect(std::any_of(T.Choices.begin(), T.Choices.end(), [](const ss::town::Choice& C) { return C.Id == "sponsor" && C.Label == "Hear him out"; }), "journey: Hear him out");
	S.Heard("ray", T);
	ss::town::Talk Again = ss::town::Open(*ss::town::ContextFor(S, "ray", "store"));
	Expect(Again.Topic != "sponsor:luckypenny", "journey: the pitch is said once");
	ss::town::Talk Terms = ss::town::Answer(*ss::town::ContextFor(S, "ray", "store"), "sponsor");
	Expect(Terms.Line.find("Twenty-five a week") == 0 && Terms.Choices.size() == 2 && Terms.Choices[0].Id == "sponsor-sign", "journey: the terms across the counter");
	S.Heard("ray", Terms);
	Expect(std::any_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [&Offer](const life::Letter& M) { return M.Thread == Offer && M.Kind == sponsor::LetterHeard; }),
		"journey: the deal is in writing in Mail");
	const ss::Chips Before = S.BankrollCents;
	ss::town::Talk Sign = ss::town::Answer(*ss::town::ContextFor(S, "ray", "store"), "sponsor-sign");
	Expect(Sign.Line.find("Say penny") != std::string::npos, "journey: the Polaroid");
	S.Heard("ray", Sign);
	const sponsor::Contract* C = sponsor::Find(S.Life.Sponsors, Offer);
	Expect(C && C->St == sponsor::State::Active && C->Now.Appearances == 1, "journey: signed on the counter slip, the month's photo taken");
	Expect(S.BankrollCents == Before && S.Life.Sponsors.StoreCreditCents == 0, "journey: nothing paid up front");
	// A week on: $25 of credit, spent first at the counter and on Penny Drop.
	const double Signed = S.WorldMinutes();
	Now = Jump(S, Now, Signed + 7.0 * Day + 60.0);
	Expect(S.Life.Sponsors.StoreCreditCents == 2500, "journey: a week's credit");
	const Chips Bank = S.BankrollCents;
	Expect(S.Checkout(Can).empty() && S.BankrollCents == Bank && S.Life.Sponsors.StoreCreditCents < 2500, "journey: the counter takes the credit first");
	Expect(S.Life.Ledger.front().Label.find("store credit") != std::string::npos && S.Life.Ledger.front().Amount == 0, "journey: the receipt says it");
	ss::store::Basket Drop;
	Drop.Add("volt-rush", 8);
	const Chips Credit = S.Life.Sponsors.StoreCreditCents;
	const Chips Bank2 = S.BankrollCents;
	Expect(S.PlaceOrder(Drop).empty() && S.Life.Sponsors.StoreCreditCents == 0 && Bank2 - S.BankrollCents == ss::store::DeliveryTotal(Drop) - Credit, "journey: Penny Drop takes the rest of it");
	// The covered Noon Deepstack: the desk's options list the cover; registering is -$70, -$10, +$80.
	int D = ss::net::DayOf(S.WorldMinutes()) + 1;
	while (ss::net::Weekday(D) > 4)
	{
		++D;
	}
	const live::Occurrence Noon = live::FindOccurrence("embercrest-noon@" + std::to_string(D));
	Now = Jump(S, Now, static_cast<double>(D) * Day + 10.0 * 60.0);
	bool Listed = false;
	for (const std::pair<life::Funding, std::string>& O : S.FundingOptions(Noon.Id))
	{
		Listed = Listed || (O.first.From == life::Paid::Sponsor && O.second.empty());
	}
	Expect(Listed, "journey: the desk offers the Lucky Penny's cover");
	S.Life.Energy = 100.0;
	const Chips Bank3 = S.BankrollCents;
	const std::string Went = S.GoToLive(Noon.Id);
	const life::LiveEntry* E = live::EntryFor(S.Life, Noon.Id);
	Expect(Went.empty() && E && E->Fund.From == life::Paid::Sponsor, "journey: the Noon Deepstack, covered");
	Expect(Bank3 - S.BankrollCents == live::BusFareCents && Lined(S, "covered the Noon Deepstack"), "journey: covered: the bus is all it costs");
	Expect(E && std::any_of(E->Patches.begin(), E->Patches.end(), [](const std::pair<std::string, std::string>& P) { return P.second == "luckypenny"; }) == false,
		"journey: no patch sewn, none noted");
	// A forced bust: nothing more moves.
	const Chips Bank4 = S.BankrollCents;
	live::Settle(S.BankrollCents, S.Life, Noon.Id, 55, 70, 0, Noon.Start + 200.0);
	Expect(S.BankrollCents == Bank4, "journey: a bust moves no sponsor money");
}

/** Volt Rush: no money at signing, the month paid at its review; notice after one month pays the next once, if worked. */
void Stipend()
{
	Hooks H;
	ss::Session S(H, "sponsor-stipend");
	S.BankrollCents = 800000;
	double Now = Jump(S, 0.0, 3.0 * Day + 14.0 * 60.0);
	const std::string Said = S.DebugSponsor("sign voltrush");
	const sponsor::Contract* C = sponsor::Running(S.Life.Sponsors, "voltrush", S.WorldMinutes());
	Expect(C != nullptr && Said.find("signed") != std::string::npos, "stipend: signed");
	if (!C)
	{
		return;
	}
	const std::string Id = C->Id;
	const double T0 = C->Since;
	Expect(Kind11(S, "Volt Rush") == 0, "stipend: nothing at signing");
	for (int K = 0; K < 3; ++K)
	{
		Played(S, T0 + (2.0 + 6.0 * K) * Day, "voltrush");
	}
	Now = Jump(S, Now, T0 + 29.0 * Day);
	Expect(Kind11(S, "Volt Rush: monthly") == 0, "stipend: not before the review");
	Now = Jump(S, Now, T0 + 30.5 * Day);
	Expect(Kind11(S, "Volt Rush: monthly") == 15000, "stipend: $150 at the first review");
	Expect(S.GiveNotice(Id).empty(), "stipend: notice given after a month");
	for (int K = 0; K < 3; ++K)
	{
		Played(S, T0 + (32.0 + 6.0 * K) * Day, "voltrush");
	}
	Now = Jump(S, Now, T0 + 61.0 * Day);
	Now = Jump(S, Now, T0 + 75.0 * Day);
	const sponsor::Contract* K2 = sponsor::Find(S.Life.Sponsors, Id);
	Expect(Kind11(S, "Volt Rush: monthly") == 30000 && K2 && K2->St == sponsor::State::Ended && K2->Why == sponsor::End::Walked, "stipend: the notice month paid once, and the deal ends");
	Expect(H.Sounds[ss::SoundId::Mail] > 0, "stipend: Mail chimes");
}

/** Halo's camera is Halo's during the term: the pawn won't take it; after a clean end it's the player's. */
void Property()
{
	Hooks H;
	ss::Session S(H, "sponsor-pawn");
	S.BankrollCents = 300000;
	double Now = Jump(S, 0.0, 2.0 * Day + 12.0 * 60.0);
	S.DebugSponsor("sign halo");
	Expect(S.Owns("webcam-720"), "pawn: Halo sends the camera");
	bool Refused = false;
	for (const ss::shop::Lot& L : S.PawnLots())
	{
		Refused = Refused || (L.Id == "gear:webcam-720" && L.Refused == "That's got a brand's sticker on it. I don't buy loaners.");
	}
	Expect(Refused, "pawn: refused during the term");
	Expect(S.PawnSell("gear:webcam-720") == "That's got a brand's sticker on it. I don't buy loaners." &&
			   S.PawnLoan("gear:webcam-720") == "That's got a brand's sticker on it. I don't buy loaners.",
		"pawn: neither sold nor pawned");
	// Two clean months (fifteen live hours each), and it's the player's.
	const sponsor::Contract* C = sponsor::Running(S.Life.Sponsors, "halo", S.WorldMinutes());
	const double T0 = C ? C->Since : S.WorldMinutes();
	for (int M = 0; M < 2; ++M)
	{
		ss::kast::StreamLog L;
		L.Start = T0 + (static_cast<double>(M) * 30.0 + 3.0) * Day;
		L.Minutes = 16.0 * 60.0;
		S.Channel.Log.insert(S.Channel.Log.begin(), L);
	}
	Now = Jump(S, Now, T0 + 61.0 * Day);
	bool Free = false;
	for (const ss::shop::Lot& L : S.PawnLots())
	{
		Free = Free || (L.Id == "gear:webcam-720" && L.Refused.empty());
	}
	Expect(Free && S.Owns("webcam-720"), "pawn: after a clean end the camera's the player's to sell");
	// Sign for the camera and give notice at once: the month isn't worked, so the camera goes back with the deal.
	Hooks H2;
	ss::Session Q(H2, "sponsor-walk");
	Q.BankrollCents = 300000;
	double Then = Jump(Q, 0.0, 2.0 * Day + 12.0 * 60.0);
	Q.DebugSponsor("sign halo");
	const sponsor::Contract* W = sponsor::Running(Q.Life.Sponsors, "halo", Q.WorldMinutes());
	Expect(W && Q.Owns("webcam-720") && Q.GiveNotice(W->Id).empty(), "pawn: signed for the camera, notice at once");
	Then = Jump(Q, Then, Q.WorldMinutes() + 31.0 * Day);
	Expect(!Q.Owns("webcam-720") && !sponsor::Active(Q.Life.Sponsors, "halo", Q.WorldMinutes()), "pawn: a month not worked, the camera goes back");
}

/** A clawback takes what's above the rent due this week, and writes off the rest. */
void Clawback()
{
	Hooks H;
	ss::Session S(H, "sponsor-clawback");
	S.BankrollCents = 100000;
	double Now = Jump(S, 0.0, 12.0 * Day + 15.0 * 60.0);
	S.DebugSponsor("offer stacked");
	const std::string Id = sponsor::OpenOfferFor(S.Life.Sponsors, "stacked", S.WorldMinutes());
	sponsor::Contract* Offer = sponsor::Find(S.Life.Sponsors, Id);
	Expect(Offer != nullptr, "clawback: an offer");
	if (!Offer)
	{
		return;
	}
	Offer->T.SigningCents = 50000;
	Offer->T.ConductClause = true;
	Expect(S.SignDeal(Id).empty() && Kind11(S, "Stacked: signing") == 50000, "clawback: the signing money paid on signing");
	S.Life.RentStage = life::Rent::Due;
	S.Life.RentDueCents = 107500;
	S.Life.RentDeadline = S.WorldMinutes() + 4.0 * Day;
	S.BankrollCents = 115000;
	S.DebugSponsor("breach pickedup");
	Expect(S.BankrollCents == 107500, "clawback: stops at the rent due this week");
	Expect(std::any_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [](const life::Letter& M) { return M.Body.find("Consider the rest a lesson.") != std::string::npos; }), "clawback: the rest written off");
	(void)Now;
}

/** IronHouse pays the membership: its renewal is skipped while the deal runs. */
void Membership()
{
	Hooks H;
	ss::Session S(H, "sponsor-gym");
	S.BankrollCents = 400000;
	double Now = Jump(S, 0.0, 2.0 * Day + 12.0 * 60.0);
	S.DebugSponsor("sign ironhouse");
	Expect(S.Owns("gym"), "gym: the membership comes with the deal");
	Now = Jump(S, Now, S.WorldMinutes() + 35.0 * Day);
	const bool Charged = std::any_of(S.Life.Ledger.begin(), S.Life.Ledger.end(), [](const life::LedgerEntry& E) { return E.Kind == 5 && E.Label.find("Gym") != std::string::npos; });
	Expect(!Charged && S.Owns("gym"), "gym: the renewal skipped, the membership kept");
}

/** An old Team RiverLine save: the book's view on stream, a read paid, Channel::Deals never written. */
void KastTrap()
{
	ss::SaveData D;
	FILE* F = std::fopen((std::string(SS_TESTS_DIR) + "/fixtures/pre_segment_team_riverline.ssave").c_str(), "rb");
	std::string Text;
	if (F)
	{
		char Buf[65536];
		size_t N = 0;
		while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
		{
			Text.append(Buf, N);
		}
		std::fclose(F);
	}
	Text.erase(std::remove(Text.begin(), Text.end(), '\r'), Text.end());
	Expect(ss::SaveData::Parse(Text, D), "kast trap: the old save parses");
	Hooks H;
	ss::Session S(H, "sponsor-kast", &D);
	Expect(S.TeamRiverLine(), "kast trap: still Team RiverLine");
	Expect(!sponsor::OpenOfferFor(S.Life.Sponsors, "tunnelrat", S.WorldMinutes()).empty() && S.Life.Sponsors.Cooldown.count("sitwell") == 1 && S.Channel.Offers.empty() &&
			   S.Channel.Declined.empty() && S.Channel.Deals.empty(),
		"kast trap: TunnelRat's offer in the book, Sitwell's no a 60-day cooldown, Kast's lists cleared (the lead's fixture check, patched)");
	S.Life.RentStage = life::Rent::Paid; // the stream needs the apartment
	double Now = 0.0;
	const std::string Live = S.GoLive();
	Now = Wait(S, Now, 30.0);
	const Chips Read = S.StreamRead("riverline");
	Now = Wait(S, Now, 5.0);
	S.EndStream();
	const sponsor::Contract* C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
	Expect(Live.empty() && Read == 6000 && C && C->Now.Reads == 1 && C->PaidCents >= 6000, "kast trap: a read pays as it did, on the contract");
	Expect(S.Channel.Deals.empty() && S.Channel.Offers.empty(), "kast trap: Channel::Deals is never written");
	S.Save();
	Expect(H.Last.Serialize().find("kast\tdeal") == std::string::npos && H.Last.Serialize().find("sponsor\tdeal") != std::string::npos, "kast trap: the save keeps the deal in the book");
	(void)Now;
}
} // namespace sponsor_seg

void SegSponsor()
{
	sponsor_seg::Journey();
	sponsor_seg::Stipend();
	sponsor_seg::Property();
	sponsor_seg::Clawback();
	sponsor_seg::Membership();
	sponsor_seg::KastTrap();
}
} // namespace session_test
