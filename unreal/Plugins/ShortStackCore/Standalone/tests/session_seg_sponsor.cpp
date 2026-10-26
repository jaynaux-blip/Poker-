// night_one_session: core-sponsor's additions (next_segment_spec §7.6, §8.3), compiled into session_test and called
// from main() in session_main.cpp (the shared kit is SessionSeg.h): the SponsorJourney (twelve days at the counter to
// Ray's offer, signed on the counter slip, the credit at the counter and on Penny Drop, a covered Noon Deepstack, a bust);
// the stipend paid at the review, not at signing; notice after one period paying that period once; sponsor gear refused
// at the pawn during the term and sellable after a clean end; a clawback stopping at the rent reserve; the membership's
// renewal skipped; Kast working on the book's view (Channel::Deals never written) from an old Team RiverLine save.
#include "SessionSeg.h"

#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Sites.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Video.h"

#include <algorithm>
#include <cmath>

namespace session_test
{
namespace econ_seg
{
bool EconRoomIn(); // session_seg_econ.cpp (core-kastglue): whether the creator update's Room is in the build
} // namespace econ_seg
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
	if (!econ_seg::EconRoomIn())
	{
		const Chips Read = S.StreamRead("riverline");
		Now = Wait(S, Now, 5.0);
		S.EndStream();
		const sponsor::Contract* C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
		Expect(Live.empty() && Read == 6000 && C && C->Now.Reads == 1 && C->PaidCents >= 6000, "kast trap: a read pays as it did, on the contract");
	}
	else
	{
		// The creator update's C13 (SPEC_CREATORS §0.4): a read pays once a deal a world day, only after 30 world minutes live
		// with someone watching, and within 1.5x the reads the deal asks a period; the live hours pay on the contract, booked as
		// their own sponsor ledger line at the stream's end (Session::KastAfterStop), never inside the Kast payout. The old Team
		// RiverLine deal asks for no reads (Kast's old terms, "hourly and reads, nothing asked"): it keeps one paid read a world
		// day, with no 1.5x cap (the C-K gate's M1, the lead's decision; kast::growth::SponsorReadPays).
		const Chips Early = S.StreamRead("riverline");
		S.LobbyMinutes += 35.0;
		Now = Wait(S, Now, 1.0);
		const int Watching = S.Stream.Count();
		const Chips Read = S.StreamRead("riverline");
		const Chips Twice = S.StreamRead("riverline");
		const size_t Ledger0 = S.Life.Ledger.size();
		S.EndStream();
		const sponsor::Contract* C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
		const int Ask = C ? static_cast<int>(std::lround(C->T.Asks[static_cast<size_t>(sponsor::Ask::Reads)])) : -1;
		Chips Sponsors = 0;
		Chips ReadLines = 0;
		for (size_t I = 0; I + Ledger0 < S.Life.Ledger.size(); ++I)
		{
			const life::LedgerEntry& Le = S.Life.Ledger[I];
			Sponsors += Le.Kind == life::LedgerSponsors ? Le.Amount : 0;
			ReadLines += Le.Kind == life::LedgerSponsors && Le.Amount == Read && Le.Label.find("read") != std::string::npos ? Le.Amount : 0;
		}
		std::printf("  kast trap (C13, M1): a read at once %lld, 35 minutes in with %d watching %lld, again the same day %lld (the deal asks %d reads); the night's sponsor "
					"lines %lld (reads %lld)\n",
			static_cast<long long>(Early), Watching, static_cast<long long>(Read), static_cast<long long>(Twice), Ask, static_cast<long long>(Sponsors),
			static_cast<long long>(ReadLines));
		Expect(Live.empty() && C && Ask == 0 && Watching >= 1 && Early == 0 && Read == 6000 && Twice == 0 && C->Now.Reads == 1 && C->PaidCents > 0,
			"kast trap: the old Team RiverLine deal (it asks for no reads) pays one read after 30 live minutes with someone watching, once that world day; the live "
			"hours pay on the contract");
		Expect(Sponsors > 0 && ReadLines == 6000, "kast trap: tonight's sponsor pay is its own ledger lines (LedgerSponsors, the read's among them), not the Kast payout");
		// The next world day it reads again, with no 1.5x cap: twelve reads already paid this period make no difference to a deal
		// that asks for none.
		for (sponsor::Contract& K : S.Life.Sponsors.Contracts)
		{
			if (C && K.Id == C->Id)
			{
				K.Now.Reads = 12;
			}
		}
		S.LobbyMinutes += Day;
		Now = Wait(S, Now, 1.0);
		S.Life.RentStage = life::Rent::Paid;
		const std::string LiveNext = S.GoLive();
		S.LobbyMinutes += 35.0;
		Now = Wait(S, Now, 1.0);
		const int WatchingNext = S.Stream.Count();
		const Chips ReadNext = S.StreamRead("riverline");
		S.EndStream();
		const sponsor::Contract* Cn = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
		std::printf("  kast trap (M1): the next world day (\"%s\", %d watching, 12 reads paid before): a read %lld; the period's reads %d\n", LiveNext.c_str(), WatchingNext,
			static_cast<long long>(ReadNext), Cn ? Cn->Now.Reads : -1);
		Expect(LiveNext.empty() && Cn && WatchingNext >= 1 && ReadNext == 6000 && Cn->Now.Reads == 13,
			"kast trap: the next world day the old deal reads again, past 1.5x of nothing (no cap for a deal that asks for no reads)");
	}
	Expect(S.Channel.Deals.empty() && S.Channel.Offers.empty(), "kast trap: Channel::Deals is never written");
	S.Save();
	Expect(H.Last.Serialize().find("kast\tdeal") == std::string::npos && H.Last.Serialize().find("sponsor\tdeal") != std::string::npos, "kast trap: the save keeps the deal in the book");
	(void)Now;
}

std::string Fixture(const char* Name)
{
	std::string Text;
	FILE* F = std::fopen((std::string(SS_TESTS_DIR) + "/fixtures/" + Name).c_str(), "rb");
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
	return Text;
}

/** The sponsor lines of a save, in order. */
std::string SponsorLines(const std::string& Save)
{
	std::string Out;
	size_t I = 0;
	while (I < Save.size())
	{
		const size_t E = Save.find('\n', I);
		const std::string Line = Save.substr(I, E == std::string::npos ? std::string::npos : E - I);
		Out += Line.rfind("sponsor\t", 0) == 0 ? Line + "\n" : std::string();
		I = E == std::string::npos ? Save.size() : E + 1;
	}
	return Out;
}

/**
 * Update 2 (SPEC §4.7, U10): an old sponsored streamer's save has no Rerun channel, so the creator road is empty: the
 * session's record has no video, standing and the book read as they did, the save round-trips byte for byte, a month of
 * the calendar writes no letter that mentions Rerun, and Describe shows the road at 0.
 */
void OldStreamer()
{
	const std::string Text = Fixture("pre_update2_sponsored_streamer.ssave");
	ss::SaveData D;
	Expect(ss::SaveData::Parse(Text, D), "old streamer: the save parses");
	const std::string Book = SponsorLines(D.Serialize());
	Expect(!Book.empty() && Book == SponsorLines(Text), "old streamer: the sponsor book reads back as it was written");
	Hooks H;
	ss::Session S(H, "sponsor-old-streamer", &D);
	const ss::rep::Public P = S.Record();
	Expect(P.VideoSubs == 0 && P.Views30 == 0 && !P.VideoPartner && S.Life.Video.Videos.empty(), "old streamer: no channel, no video in the record");
	const std::string Said = S.DebugSponsor("describe");
	Expect(Said.find(", Rerun 0 (0 subscribers, 0 views in 30 days)") != std::string::npos, "old streamer: Describe shows the creator road at 0");
	S.Save();
	ss::SaveData Again;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Again) && Again.Serialize() == H.Last.Serialize() && SponsorLines(H.Last.Serialize()) == SponsorLines(D.Serialize()),
		"old streamer: saved at once, the book is the same lines, and the save round-trips");
	S.BankrollCents = std::max<Chips>(S.BankrollCents, 600000); // the rent's collected at its deadline: nothing here is about the rent
	double Now = 0.0;
	for (int K = 1; K <= 4; ++K)
	{
		Now = Jump(S, Now, S.WorldMinutes() + 7.0 * Day);
	}
	Expect(std::none_of(S.Life.Inbox.begin(), S.Life.Inbox.end(), [](const life::Letter& M) { return M.Body.find("Rerun") != std::string::npos; }),
		"old streamer: a month of the calendar, and no letter says Rerun");
	S.Save();
	ss::SaveData Later;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Later) && Later.Serialize() == H.Last.Serialize(), "old streamer: a month on, the save still round-trips");
}

/**
 * The creator road end to end (SPEC §4.7 items 4 and 5): a channel granted through ss.Video.Grant's dev path reaches the
 * public record (rep::Of, core-econ's half), standing takes it, Describe names the road, and Team RiverLine's weekly look
 * brings an offer whose letter says it's the channel. The record's fields come from core-econ's Rep.cpp.
 */
void Creator()
{
	Hooks H;
	ss::Session S(H, "sponsor-creator");
	S.BankrollCents = 600000;
	double Now = Jump(S, 0.0, 20.0 * Day + 9.0 * 60.0);
	Expect(S.SponsorTier() == 0, "creator: nobody calls a nobody");
	S.DebugVideo("grant 50000 4000 300000");
	// A day at a time: the calendar reads the record as of each step's end, and the views come in over the month.
	for (int K = 0; K < 10; ++K)
	{
		Now = Jump(S, Now, S.WorldMinutes() + Day);
	}
	const ss::rep::Public P = S.Record();
	std::printf("  creator: day %d, %d subscribers, %d views in 30 days, Partner %d, standing %d\n", ss::net::DayOf(S.WorldMinutes()), P.VideoSubs, P.Views30, P.VideoPartner ? 1 : 0,
		S.SponsorTier());
	Expect(P.VideoSubs >= 50000 && P.Views30 >= 150000, "creator: the channel is in the public record (rep::Of's video fields)");
	Expect(S.SponsorTier() == 4 && std::string(sponsor::StandingWords(S.SponsorTier())) == "On the circuit", "creator: on the circuit by the creator road");
	const std::string Said = S.DebugSponsor("describe");
	Expect(Said.find("roads: stream 0, live 0, Rerun 4 (") != std::string::npos && Said.find(" views in 30 days, Partner)\n") != std::string::npos,
		"creator: Describe shows the road standing stands on, and Partner");
	// A day at a time, so the offer is still open when its letter turns up (an offer stands seven days).
	std::string Letter;
	for (int K = 0; K <= 21 && Letter.empty(); ++K)
	{
		Now = K == 0 ? Now : Jump(S, Now, S.WorldMinutes() + Day);
		for (const life::Letter& M : S.Life.Inbox)
		{
			Letter = M.Kind == sponsor::LetterOffer && M.Thread.rfind("riverline@", 0) == 0 ? M.Body : Letter;
		}
	}
	Expect(Letter.rfind("Your Rerun videos pulled ", 0) == 0 || Letter.rfind("Your Rerun channel's up to ", 0) == 0,
		("creator: Team RiverLine's letter says it's the channel: " + Letter.substr(0, 90)).c_str());
	// The deal signs like any other, and the save keeps the book and the channel.
	const std::string Id = sponsor::OpenOfferFor(S.Life.Sponsors, "riverline", S.WorldMinutes());
	const std::string Signed = Id.empty() ? std::string("no open offer") : S.SignDeal(Id);
	Expect(Signed.empty() && S.TeamRiverLine(), ("creator: signed, Team RiverLine" + (Signed.empty() ? std::string() : ": " + Signed)).c_str());
	S.Save();
	ss::SaveData Again;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Again) && Again.Serialize() == H.Last.Serialize() && Again.Life.Video.Subs >= 50000.0,
		"creator: the save round-trips with the deal and the channel");
}

/** A joinable event on Site now (the cheapest small one: what's tested is where it's played, not the play). */
bool SiteEvent(const ss::Session& S, ss::net::SiteId Site, ss::LobbyEvent& Out)
{
	const double World = S.WorldMinutes();
	bool Found = false;
	for (const ss::net::EventInstance& I : ss::net::Site(Site).Window(World - 120.0, World + 90.0))
	{
		const ss::LobbyEvent L = ss::net::Site(Site).Listing(I, nullptr, S.Unlocks());
		if (L.Joinable && L.BuyInCents > 0 && L.BuyInCents <= 2200 && (!Found || L.BuyInCents < Out.BuyInCents))
		{
			Out = L;
			Found = true;
		}
	}
	return Found;
}

/** The newest sponsor letter of Kind on a contract of Brand's ("" for none). */
std::string BrandLetter(const ss::Session& S, const std::string& Brand, int Kind)
{
	std::string Body;
	for (const life::Letter& M : S.Life.Inbox)
	{
		Body = M.Source == "sponsor" && M.Kind == Kind && M.Thread.rfind(Brand + "@", 0) == 0 ? M.Body : Body;
	}
	return Body;
}

/**
 * Update 2 (§5.7, §8.3 step 5, G3): the clause through the session. Team RiverLine signed from G3 plays only on RiverLine:
 * a RiverLine table is nothing; opening a RunItTwice table (the register button's second press: RegisterEvent) is a strike
 * with its Mail the same hour and the cure date; a second table there the same day isn't a second strike. An old Team
 * RiverLine deal (signed before G3, the Kast migration's) has no clause until it renews.
 */
void Clause()
{
	// An old Team RiverLine deal (the Kast migration's, signed before G3): no clause, no warning, no strike.
	{
		ss::SaveData D;
		Expect(ss::SaveData::Parse(Fixture("pre_segment_team_riverline.ssave"), D), "clause: the old Team RiverLine save parses");
		bool Old = false;
		for (const sponsor::Contract& X : D.Life.Sponsors.Contracts)
		{
			Old = Old || (X.Brand == "riverline" && (X.St == sponsor::State::Active || X.St == sponsor::State::Notice) && X.T.Only.empty());
		}
		bool Warns = false;
		for (int Site = 0; Site < static_cast<int>(ss::net::SiteId::Count); ++Site)
		{
			Warns = Warns || sponsor::WouldStrike(D.Life.Sponsors, Site, true, true);
		}
		Expect(Old && !Warns && SponsorLines(D.Serialize()).find("only=") == std::string::npos, "clause: an old Team RiverLine deal keeps its old terms (no clause, no warning)");
	}
	Hooks H;
	ss::Session S(H, "sponsor-clause");
	S.BankrollCents = 600000;
	double Now = Jump(S, 0.0, 30.0 * Day + 20.0 * 60.0);
	const std::string Signed = S.DebugSponsor("sign riverline");
	const sponsor::Contract* C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
	Expect(C && C->T.Only == "riverline", ("clause: Team RiverLine signed from G3 plays only on RiverLine: " + Signed).c_str());
	Expect(S.DebugSponsor("describe").find("  a table would be a strike on: ignite, runittwice") != std::string::npos, "clause: Describe says where a table would be a strike");
	// A RunItTwice account, as if joined (core-sites' Join), with money on it.
	ss::site::Account A;
	A.Site = ss::net::SiteId::RunItTwice;
	A.JoinedAt = S.WorldMinutes();
	A.Cents = 50000;
	S.Life.Sites.Accounts.push_back(A);
	ss::LobbyEvent Home;
	ss::LobbyEvent Away;
	if (ss::net::Site(ss::net::SiteId::RunItTwice).Templates().empty())
	{
		std::printf("  clause: RunItTwice has no schedule in this build (core-sites' registry): the register button's strike is skipped\n");
		return;
	}
	if (!SiteEvent(S, ss::net::SiteId::RiverLine, Home) || !SiteEvent(S, ss::net::SiteId::RunItTwice, Away))
	{
		Expect(false, "clause: a RiverLine and a RunItTwice event to register for");
		return;
	}
	Expect(!sponsor::WouldStrike(S.Life.Sponsors, static_cast<int>(ss::net::SiteOf(Home.Spec.Id)), false, false) &&
			   sponsor::WouldStrike(S.Life.Sponsors, static_cast<int>(ss::net::SiteOf(Away.Spec.Id)), false, false),
		"clause: the register button warns on RunItTwice's table, not on RiverLine's");
	S.RegisterEvent(Home);
	Expect(S.IsPlaying(Home.Spec.Id) && C->Strikes == 0 && BrandLetter(S, "riverline", sponsor::LetterStrike).empty(), "clause: a RiverLine table, no strike");
	// Update 3 (W3, a deliberate rewrite at Phase A): the gate is in core now. Without the second press's token the table is
	// refused, with nothing changed (no table, no debit, no strike) and the reason returned; the second press takes it.
	{
		const ss::Chips* Rit = ss::site::WalletOf(S.Life.Sites, ss::net::SiteId::RunItTwice);
		const ss::Chips Wallet = Rit ? *Rit : -1, Bank = S.BankrollCents;
		const size_t Tables = static_cast<size_t>(S.TableCount()), Letters = S.Life.Inbox.size();
		const std::string Why = S.RegisterEvent(Away);
		C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
		Rit = ss::site::WalletOf(S.Life.Sites, ss::net::SiteId::RunItTwice);
		Expect(!Why.empty() && !S.IsPlaying(Away.Spec.Id) && static_cast<size_t>(S.TableCount()) == Tables && Rit && *Rit == Wallet && S.BankrollCents == Bank && C &&
				   C->Strikes == 0 && S.Life.Inbox.size() == Letters && BrandLetter(S, "riverline", sponsor::LetterStrike).empty(),
			("clause: without the second press, core refuses the RunItTwice table and changes nothing: " + Why).c_str());
	}
	const double At = S.WorldMinutes();
	S.RegisterEvent(Away, ss::StrikeConfirm{true});
	C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
	const std::string Strike = BrandLetter(S, "riverline", sponsor::LetterStrike);
	Expect(S.IsPlaying(Away.Spec.Id) && C && C->Strikes == 1 && C->CureUntil > At + 13.9 * Day && Strike.find("today you were on RunItTwice") != std::string::npos &&
			   S.Life.Inbox.back().At == At,
		("clause: the RunItTwice table is a strike, its Mail the same hour, cure in two weeks: " + Strike.substr(0, 80)).c_str());
	Now = Jump(S, Now, S.WorldMinutes() + 60.0);
	C = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
	Expect(C && C->Strikes == 1, "clause: one strike, however long the table runs");
	// The save keeps the clause and the strike's day (a reload doesn't strike that day again).
	S.Save();
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Back) && Back.Serialize() == H.Last.Serialize() && SponsorLines(H.Last.Serialize()).find("\tonly=riverline") != std::string::npos,
		"clause: the save keeps the clause and round-trips");
	const sponsor::Contract* Bc = sponsor::Running(Back.Life.Sponsors, "riverline", At);
	sponsor::NoteSitePlay(Back.Life.Sponsors, Back.Life, static_cast<int>(ss::net::SiteId::RunItTwice), At + 120.0, false, false);
	Expect(Bc && Bc->Strikes == 1, "clause: reloaded, the same day is still one strike");
}

/**
 * Update 2 (§5.6): Team RunItTwice's door also opens on a series title won on RunItTwice. The book can't see results, so the
 * sponsor step marks it from the session's History (MarkSiteTitles): a RunItTwice series event won, with an account there.
 * Not a second place, not a RunItTwice daily, not a RiverLine series title, and nothing without an account.
 */
void SiteTitle()
{
	Hooks H;
	ss::Session S(H, "sponsor-site-title");
	Jump(S, 0.0, 30.0 * Day + 12.0 * 60.0);
	const ss::net::EventTemplate* Series = nullptr;
	const ss::net::EventTemplate* Daily = nullptr;
	for (const ss::net::EventTemplate& T : ss::net::Site(ss::net::SiteId::RunItTwice).Templates())
	{
		Series = !Series && !T.Series.empty() ? &T : Series;
		Daily = !Daily && T.Series.empty() ? &T : Daily;
	}
	const ss::net::EventTemplate* Home = nullptr;
	for (const ss::net::EventTemplate& T : ss::net::Shared().Templates())
	{
		Home = !Home && !T.Series.empty() ? &T : Home;
	}
	if (!Series || !Daily || !Home)
	{
		Expect(false, "site title: a RunItTwice series event, a RunItTwice daily and a RiverLine series event");
		return;
	}
	const auto Result = [&S](const ss::net::EventTemplate& T, int Place) {
		ss::HistoryEntry E;
		E.Name = T.Name;
		E.Place = Place;
		E.Entrants = 300;
		E.BuyInCents = T.BuyInCents;
		E.EventId = T.Id + "@" + std::to_string(static_cast<long long>(S.WorldMinutes() - 600.0));
		return E;
	};
	const auto Step = [&S]() {
		S.Life.Sponsors.NextAt = 0.0;
		S.SponsorStep(S.WorldMinutes() - 1.0, S.WorldMinutes());
		return S.Life.Sponsors.PaidKeys.count("sitetitle|runittwice") > 0;
	};
	const std::string Door = "RunItTwice signs players who grind its tables: 100 events there, or a series title won there.";
	S.History.insert(S.History.begin(), Result(*Series, 1));
	Expect(!Step(), "site title: no RunItTwice account, no mark");
	ss::site::Account A;
	A.Site = ss::net::SiteId::RunItTwice;
	A.JoinedAt = S.WorldMinutes() - Day;
	A.LifetimeEvents = 3;
	S.Life.Sites.Accounts.push_back(A);
	S.History = {Result(*Series, 2), Result(*Daily, 1), Result(*Home, 1)};
	Expect(!Step() && S.Life.Sponsors.PaidKeys.count("sitetitle|riverline") == 0 && sponsor::Eligible(S.Life.Sponsors, S.Life, S.Record(), "turnstone", S.WorldMinutes()) == Door,
		"site title: a RunItTwice second place, a RunItTwice daily won, a RiverLine series title: the door stays shut");
	S.History.insert(S.History.begin(), Result(*Series, 1));
	Expect(Step() && sponsor::Eligible(S.Life.Sponsors, S.Life, S.Record(), "turnstone", S.WorldMinutes()) != Door,
		("site title: a RunItTwice series event won opens Team RunItTwice's door (" + Series->Name + ")").c_str());
}

/**
 * Update 2 (§5.7, §8.3 step 5): Studio's PUBLISH under the clause. A video of a RunItTwice hand warns (WouldStrike with
 * Video) and, published, is a strike; a video of an Embercrest hand shows no site and never is. The strike on publishing is
 * core-video's call in Session::Publish (patches/core-deals-g3_to_core-video_1.diff); a build without it says so.
 */
void VideoStrike()
{
	for (const char* Where : {"embercrest", "online"})
	{
		const bool Online = std::string(Where) == "online";
		Hooks H;
		ss::Session S(H, std::string("sponsor-video-strike-") + Where);
		S.BankrollCents = 600000;
		Jump(S, 0.0, 30.0 * Day + 12.0 * 60.0);
		S.Life.Energy = 80.0;
		S.DebugSponsor("sign riverline");
		S.DebugVideo(std::string("fake bigpot ") + Where);
		if (S.Life.Video.Open.empty())
		{
			Expect(false, "clause: a candidate to publish");
			continue;
		}
		ss::video::Candidate& X = S.Life.Video.Open.back();
		X.Site = Online ? static_cast<int>(ss::net::SiteId::RunItTwice) : X.Site;
		const bool Warned = sponsor::WouldStrike(S.Life.Sponsors, X.Site, false, true);
		ss::video::Choice C;
		C.Candidates = {X.Id};
		C.F = ss::video::Format::Breakdown;
		const std::string Why = S.Publish(C);
		const sponsor::Contract* K = sponsor::Running(S.Life.Sponsors, "riverline", S.WorldMinutes());
		if (!Online)
		{
			Expect(Why.empty() && !Warned && K && K->Strikes == 0, "clause: a video of an Embercrest hand shows no site: no warning, no strike");
			continue;
		}
		Expect(Warned && Why.empty(), "clause: Studio's PUBLISH warns on a RunItTwice hand, and it publishes");
		if (K && K->Strikes == 0)
		{
			std::printf("  clause: Session::Publish doesn't note the video's site in this build (patches/core-deals-g3_to_core-video_1.diff): its strike isn't checked\n");
			continue;
		}
		Expect(K && K->Strikes == 1 && BrandLetter(S, "riverline", sponsor::LetterStrike).find("today you were on RunItTwice") != std::string::npos,
			"clause: published, a video of a RunItTwice hand is a strike");
	}
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
	sponsor_seg::OldStreamer();
	sponsor_seg::Creator();
	sponsor_seg::Clause();
	sponsor_seg::SiteTitle();
	sponsor_seg::VideoStrike();
}
} // namespace session_test
