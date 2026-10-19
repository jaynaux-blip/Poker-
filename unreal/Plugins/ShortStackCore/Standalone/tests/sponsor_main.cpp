// sponsor_deals (next_segment_spec §7.6, suite 12): core-sponsor's tests.
// Catalog integrity; tiers per road and the conduct limits; offer cadence; exclusivity and slots; an idempotent Step;
// measurement; strikes and cures; conduct; the counter table; renewals; live money; the round trip and the Kast
// migration fixtures; Kast's view (Channel::Deals never written); the exploits.
//
// Goldens and fixtures are read from SS_TESTS_DIR (unreal/Plugins/ShortStackCore/Tests, set by CMakeLists.txt).
#include "CopyLint.h"
#include "ShortStack/Game/Kast.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Sponsor.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/UI/SponsorArt.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace sponsor_test
{
int Failures = 0;
int Checks = 0;

void Expect(bool Condition, const char* What)
{
	++Checks;
	if (!Condition)
	{
		++Failures;
		std::printf("FAILED: %s\n", What);
	}
}

namespace life = ss::life;
namespace live = ss::live;
namespace sponsor = ss::sponsor;
using ss::Chips;

constexpr double Day = 1440.0;

std::string ReadFile(const std::string& Path)
{
	std::string Out;
	FILE* F = std::fopen(Path.c_str(), "rb");
	if (!F)
	{
		return Out;
	}
	char Buf[65536];
	size_t N = 0;
	while ((N = std::fread(Buf, 1, sizeof(Buf), F)) > 0)
	{
		Out.append(Buf, N);
	}
	std::fclose(F);
	Out.erase(std::remove(Out.begin(), Out.end(), '\r'), Out.end());
	return Out;
}

std::string Lower(std::string S)
{
	for (char& Ch : S)
	{
		Ch = Ch >= 'A' && Ch <= 'Z' ? static_cast<char>(Ch - 'A' + 'a') : Ch;
	}
	return S;
}

/** A record that reaches tier 2 by the stream road (500 followers, 20 a night), the conduct clean. */
ss::rep::Public Streamer(int Followers = 600, double Avg = 25.0)
{
	ss::rep::Public P;
	P.Followers = Followers;
	P.Avg30 = Avg;
	return P;
}

/** The world as Step sees it: a book, a life, a bankroll, a channel and the record. */
struct World
{
	life::State L;
	Chips Bank = 100000;
	ss::kast::Channel Ch;
	ss::rep::Public P; // nobody yet: the brands that look find nothing, so a test gets only the deals it makes
	std::vector<std::pair<std::string, std::string>> Texts;

	void Step(double From, double To)
	{
		sponsor::StepContext Ctx;
		Ctx.Texts = &Texts;
		sponsor::Step(L.Sponsors, L, Bank, Ch, P, From, To, "test-seed", Ctx);
	}
	sponsor::Contract* Of(const std::string& Brand)
	{
		for (sponsor::Contract& C : L.Sponsors.Contracts)
		{
			if (C.Brand == Brand && C.St != sponsor::State::Ended)
			{
				return &C;
			}
		}
		return nullptr;
	}
	/** An offer from Brand now (forced: the gate isn't what's tested), signed. Its id. */
	std::string Signed(const std::string& Brand, double At)
	{
		const std::string Id = sponsor::MakeOffer(L.Sponsors, L, P, Brand, At, "test-seed", true);
		const std::string Why = sponsor::Sign(L.Sponsors, L, Bank, Id, At);
		if (!Why.empty())
		{
			std::printf("  (signing %s: %s)\n", Brand.c_str(), Why.c_str());
		}
		return Id;
	}
	/** A finished live entry at the Embercrest at At, the player wearing Brand's patch ("" for none). */
	void Played(double At, const std::string& Brand, int Place = 40, Chips Prize = 0)
	{
		life::LiveEntry E;
		E.Id = "embercrest-nightly@" + std::to_string(ss::net::DayOf(At));
		// One a day at most here; a second on the same day gets the Midnight Turbo's id.
		for (const life::LiveEntry& X : L.LiveEntries)
		{
			if (X.Id == E.Id)
			{
				E.Id = "embercrest-turbo@" + std::to_string(ss::net::DayOf(At));
			}
		}
		E.Name = "Embercrest Nightly $120";
		E.State = life::LiveEntry::Finished;
		E.PaidCents = 12000;
		E.FeeCents = 1500;
		E.RegisteredAt = At - 300.0;
		E.FinishedAt = At;
		E.Place = Place;
		E.PrizeCents = Prize;
		E.Roster = {{"Mei", 0}, {"Sal", 1}};
		if (!Brand.empty())
		{
			E.Patches.push_back({"grinder_3c", Brand});
		}
		L.LiveEntries.push_back(E);
	}
	/** A letter whose body has Part in it. */
	bool Mailed(const std::string& Part) const
	{
		for (const life::Letter& M : L.Inbox)
		{
			if (M.Body.find(Part) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}
	int Letters(int Kind) const
	{
		int N = 0;
		for (const life::Letter& M : L.Inbox)
		{
			N += M.Kind == Kind ? 1 : 0;
		}
		return N;
	}
	Chips Ledger(const std::string& Part) const
	{
		Chips Sum = 0;
		for (const life::LedgerEntry& E : L.Ledger)
		{
			Sum += E.Kind == life::LedgerSponsors && E.Label.find(Part) != std::string::npos ? E.Amount : 0;
		}
		return Sum;
	}
};

// ------------------------------------------------------------------ 1. the catalog

void Catalog()
{
	const std::vector<sponsor::Brand>& All = sponsor::Brands();
	Expect(All.size() == 13, "catalog: thirteen brands");
	std::set<std::string> Ids;
	std::set<std::string> Reps;
	for (const sponsor::Brand& B : All)
	{
		Expect(Ids.insert(B.Id).second, "catalog: ids are unique");
		Expect(!B.Name.empty() && !B.Short.empty() && !B.Rep.empty(), "catalog: every brand has a name, a short name and a rep");
		Expect(sponsor::FindBrand(B.Id) == &B && sponsor::FindBrandByName(B.Name) == &B, "catalog: found by id and by name");
		Expect(B.Tier >= 1 && B.Tier <= 5, "catalog: tiers one to five");
		if (B.Keeper.empty())
		{
			Expect(B.Pitch.size() == 2, "catalog: a brand that mails has two wordings of its offer");
			Expect(Reps.insert(B.Rep).second, "catalog: reps are unique");
		}
		else
		{
			Expect(ss::town::FindPerson(B.Keeper) != nullptr, "catalog: a local brand's keeper works on the block");
		}
		Expect(!B.Offer.Wants.empty(), "catalog: every deal shows somewhere (a patch spot or the overlay)");
	}
	for (const char* Id : {"overclock", "tunnelrat", "stacked", "riverline", "sitwell"})
	{
		const sponsor::Brand* B = sponsor::FindBrand(Id);
		Expect(B && B->Legacy, "catalog: Kast's five ids are kept, and marked legacy");
	}
	Expect(sponsor::FindBrand("turnstone") && !sponsor::FindBrand("lantern") && !sponsor::FindBrand("lanternpoker"), "catalog: the site is Turnstone Poker, not Lantern");
	Expect(sponsor::FindBrandByName("RiverLine") == sponsor::FindBrand("riverline") && sponsor::FindBrandByName("Team RiverLine") == sponsor::FindBrand("riverline"),
		"catalog: the world's 'RiverLine' and 'Team RiverLine' are the site's deal");
	// Kast's view: the five, in their old order, with their old names and rates (the world's streamers draw from these).
	const std::vector<ss::kast::Sponsor>& K = ss::kast::Sponsors();
	const char* Names[] = {"Overclock", "TunnelRat VPN", "Stacked Apparel", "RiverLine", "Sitwell"};
	const Chips Hours[] = {300, 600, 1000, 2500, 3500};
	const Chips Reads[] = {1000, 2500, 4000, 6000, 9000};
	bool Same = K.size() == 5;
	for (size_t I = 0; Same && I < 5; ++I)
	{
		Same = K[I].Brand == Names[I] && K[I].PerHourCents == Hours[I] && K[I].ReadCents == Reads[I];
	}
	Expect(Same, "catalog: kast::Sponsors is the legacy five, in order, at their old rates");
	Expect(ss::kast::FindSponsor("halo") && ss::kast::FindSponsor("halo")->PerHourCents == 200 && !ss::kast::FindSponsor("voltrush"), "catalog: Kast finds any brand that pays on stream");
	// The trademark screen: none of the catalog's words is a real poker site, drink, VPN, chair, camera or training site.
	const char* Real[] = {"pokerstars", "ggpoker", "partypoker", "888", "wsop", "world series", "wpt", "run it once", "upswing", "red bull", "redbull", "monster",
		"rockstar", "celsius", "g fuel", "gfuel", "bang energy", "prime hydration", "ghost energy", "nordvpn", "expressvpn", "surfshark", "secretlab", "dxracer",
		"logitech", "elgato", "razer", "nike", "adidas", "bodog", "ignition", "americas cardroom", "natural8", "bovada", "pokerbros", "global poker", "acr poker",
		// Update 2 (update 2 SPEC §1.8): video platforms, payment wallets and more real poker sites, for Rerun and the rival sites.
		"youtube", "twitch", "kick", "tiktok", "rumble", "vimeo", "dailymotion", "skrill", "neteller", "paypal", "venmo", "ecopayz", "muchbetter", "888poker", "acr ",
		"betonline", "wpt global", "coinpoker", "winamax", "unibet"};
	bool Clean = true;
	for (const sponsor::Brand& B : All)
	{
		const std::string Words = Lower(B.Id + " " + B.Name + " " + B.Short + " " + B.Kast + " " + B.Rep + " " + B.Product);
		for (const char* R : Real)
		{
			if (Words.find(R) != std::string::npos)
			{
				Clean = false;
				std::printf("  real brand '%s' in %s\n", R, B.Id.c_str());
			}
		}
	}
	Expect(Clean, "catalog: the trademark screen passes");
	// The mail reps don't share a name with anyone the player meets.
	bool Apart = true;
	for (const sponsor::Brand& B : All)
	{
		if (!B.Keeper.empty())
		{
			continue;
		}
		for (const ss::town::Person& P : ss::town::People())
		{
			Apart = Apart && Lower(P.Name) != Lower(B.Rep);
		}
		for (const live::RoomLocal& R : live::RoomLocals())
		{
			Apart = Apart && Lower(R.Name) != Lower(B.Rep);
		}
		for (const live::CastMember& C : live::EmbercrestCast())
		{
			Apart = Apart && Lower(C.Name) != Lower(B.Rep);
		}
	}
	Expect(Apart, "catalog: the reps' names collide with no one on the block, in the room or at Dee's");
	// Every line the book can write passes the copy lint.
	std::map<std::string, std::string> Vars = sponsor::InventorySamples();
	int Problems = copylint::Run("sponsor letters", sponsor::Inventory(0), copylint::LetterBody, Vars);
	Problems += copylint::Run("sponsor subjects", sponsor::Inventory(1), copylint::MailSubject, Vars);
	Problems += copylint::Run("sponsor lines", sponsor::Inventory(2), copylint::Pa, Vars);
	Expect(Problems == 0, "catalog: every letter, subject and line passes the copy lint");
	Expect(sponsor::Inventory(0).size() >= 60 && sponsor::Inventory(1).size() >= 25, "catalog: the inventory has the letters");
	// A fresh world's Team RiverLine starts empty and signs on results (update 2 SPEC §3.3 F3), and the book can't see the
	// world: no letter counts the team's roster (U5: no copy claims what didn't happen in this world).
	bool NoHeadcount = true;
	for (const std::pair<std::string, std::string>& L : sponsor::Inventory(0))
	{
		const std::string Body = Lower(L.second);
		for (const char* Count : {"ten players", "ten pros", "team riverline is ten", "ten-player"})
		{
			if (Body.find(Count) != std::string::npos)
			{
				NoHeadcount = false;
				std::printf("  '%s' in %s\n", Count, L.first.c_str());
			}
		}
	}
	Expect(NoHeadcount, "catalog: no letter counts Team RiverLine's roster (a fresh world's team starts empty)");
}

// ------------------------------------------------------------------ 2. standing

void Standing()
{
	ss::rep::Public P;
	Expect(sponsor::Tier(P, "voltrush") == 0, "standing: nobody calls a nobody");
	P.Followers = 150;
	Expect(sponsor::Tier(P, "voltrush") == 1, "standing: 150 followers is known on the block");
	P.Followers = 500;
	P.Avg30 = 19.0;
	Expect(sponsor::Tier(P, "voltrush") == 1, "standing: 500 followers without 20 a night is still tier 1");
	P.Avg30 = 20.0;
	Expect(sponsor::Tier(P, "voltrush") == 2, "standing: 500 and 20 a night is a face in the room");
	P.Partner = true;
	Expect(sponsor::Tier(P, "voltrush") == 3, "standing: Partner is a name in the city");
	P.Followers = 5000;
	P.Avg30 = 150.0;
	Expect(sponsor::Tier(P, "voltrush") == 4, "standing: 5,000 and 150 is on the circuit");
	P.Followers = 25000;
	P.Avg30 = 600.0;
	Expect(sponsor::Tier(P, "voltrush") == 5, "standing: 25,000 and 600 is a personality");
	ss::rep::Public Live;
	Live.LiveEvents = 3;
	Expect(sponsor::Tier(Live, "voltrush") == 1, "standing: three Embercrest events");
	Live.Embercrest60 = 8;
	Live.Cashes60 = 1;
	Expect(sponsor::Tier(Live, "voltrush") == 1, "standing: eight events and one cash isn't tier 2");
	Live.Cashes60 = 2;
	Expect(sponsor::Tier(Live, "voltrush") == 2, "standing: eight and two cashes in sixty days");
	ss::rep::Public Ft;
	Ft.BestEmbercrestPlace = 6;
	Expect(sponsor::Tier(Ft, "voltrush") == 2, "standing: an Embercrest final table");
	Ft.BestEmbercrestPlace = 7;
	Expect(sponsor::Tier(Ft, "voltrush") == 0, "standing: seventh isn't a final table");
	Ft.Titles = 1;
	Expect(sponsor::Tier(Ft, "voltrush") == 3, "standing: a title");
	ss::rep::Public Cash;
	Cash.Cashes90 = 5;
	Expect(sponsor::Tier(Cash, "voltrush") == 3, "standing: five cashes in ninety days");
	ss::rep::Public Big = Streamer(30000, 700.0);
	Big.Titles = 3;
	Big.PickedUp90 = true;
	Expect(sponsor::Tier(Big, "voltrush") == 1 && sponsor::Tier(Big, "luckypenny") == 1, "standing: a pickup holds every brand to tier 1");
	Big.PickedUp90 = false;
	Big.Restricted180 = true;
	Expect(sponsor::Tier(Big, "riverline") == 0 && sponsor::Tier(Big, "rangewise") == 0 && sponsor::Tier(Big, "voltrush") == 5, "standing: a restriction shuts out sites and training");
	Expect(std::string(sponsor::StandingWords(2)) == "A face in the room", "standing: Career shows the words");
	// Dee's road and the block road (that shop only).
	life::State L;
	sponsor::Book B;
	ss::rep::Public None;
	L.BackRoomNetCents = 19999;
	Expect(sponsor::TierOf(None, B, L, "barber", 100.0 * Day) == 0, "standing: Dee's game up $199.99 isn't enough");
	L.BackRoomNetCents = 20000;
	Expect(sponsor::TierOf(None, B, L, "barber", 100.0 * Day) == 1, "standing: Dee's game up $200 is known on the block");
	L.BackRoomNetCents = 0;
	for (int D = 0; D < 12; ++D)
	{
		for (int K = 0; K < 3; ++K)
		{
			sponsor::NoteVisit(B, "store", (88.0 + D) * Day + 600.0 + K);
		}
	}
	Expect(sponsor::VisitDays(B, "store", 100.0 * Day) == 12, "standing: the block road counts days, not purchases");
	Expect(sponsor::TierOf(None, B, L, "luckypenny", 100.0 * Day) == 1 && sponsor::TierOf(None, B, L, "barber", 100.0 * Day) == 0,
		"standing: twelve days at the store make the store's standing, nobody else's");
	Expect(sponsor::VisitDays(B, "store", 160.0 * Day) == 0, "standing: the block road forgets after sixty days");
}

// ------------------------------------------------------------------ 3. offers on the weekly look

void Offers()
{
	World W;
	W.P = Streamer(400, 20.0); // Overclock's gate (300 and 15), short of tier 2's stream road
	const double Start = 20.0 * Day;
	W.Step(Start, Start + 21.0 * Day);
	int Overclock = 0;
	double First = 0.0;
	for (const sponsor::Contract& C : W.L.Sponsors.Contracts)
	{
		if (C.Brand == "overclock")
		{
			++Overclock;
			First = First == 0.0 ? C.OfferedAt : First;
		}
	}
	// Seven days to answer, then it lapses; three weeks before it looks again.
	Expect(Overclock == 1, "offers: Overclock offers once, and doesn't again for three weeks after it lapses");
	const int Weekday = ss::net::Weekday(ss::net::DayOf(First));
	const double Minute = First - std::floor(First / Day) * Day;
	Expect(Weekday < 5 && Minute >= 9.0 * 60.0 && Minute < 18.0 * 60.0, "offers: mail comes on a weekday, nine to six");
	Expect(W.Letters(sponsor::LetterOffer) == 1, "offers: the offer is a letter in Mail");
	const sponsor::Contract* C = nullptr;
	for (const sponsor::Contract& X : W.L.Sponsors.Contracts)
	{
		C = X.Brand == "overclock" ? &X : C;
	}
	Expect(C && C->St == sponsor::State::Ended && C->Why == sponsor::End::Expired && std::fabs(C->ExpiresAt - C->OfferedAt - 7.0 * Day) < 1e-6, "offers: an offer stands seven days");
	Expect(W.L.Inbox.back().Body.find("Your channel's up to 400 followers") == 0, "offers: the letter says why they're calling now");
	// Stepping the same span again changes nothing (a calendar that runs twice).
	const std::string Before = [&W] {
		std::string S;
		sponsor::WriteLines(W.L, S);
		return S;
	}();
	W.Step(Start, Start + 21.0 * Day);
	std::string After;
	sponsor::WriteLines(W.L, After);
	Expect(Before == After, "offers: stepping a span twice offers nothing twice");
	// The same career stepped in small pieces gets the same offer at the same minute (only the seed decides).
	World Pieces;
	Pieces.P = W.P;
	for (double T = Start; T < Start + 21.0 * Day; T += 37.0)
	{
		Pieces.Step(T, std::min(T + 37.0, Start + 21.0 * Day));
	}
	std::string Piecewise;
	sponsor::WriteLines(Pieces.L, Piecewise);
	std::string Whole;
	sponsor::WriteLines(W.L, Whole);
	const auto Deals = [](const std::string& S) {
		std::string Out;
		size_t I = 0;
		while (I < S.size())
		{
			const size_t E = S.find('\n', I);
			const std::string Line = S.substr(I, E - I);
			Out += Line.rfind("sponsor\tdeal\t", 0) == 0 ? Line + "\n" : "";
			I = E == std::string::npos ? S.size() : E + 1;
		}
		return Out;
	};
	Expect(!Deals(Whole).empty() && Deals(Piecewise) == Deals(Whole) && Pieces.L.Inbox.size() == W.L.Inbox.size() && Pieces.L.Inbox.back().At == W.L.Inbox.back().At,
		"offers: the same offers at the same minutes, stepped whole or in pieces");
	// A local brand makes its offer in person: no letter until the keeper's told it.
	World Local;
	Local.P = ss::rep::Public();
	for (int D = 0; D < 12; ++D)
	{
		sponsor::NoteVisit(Local.L.Sponsors, "store", (8.0 + D) * Day + 700.0);
	}
	Local.Step(19.0 * Day, 26.0 * Day);
	const std::string Penny = sponsor::OpenOfferFor(Local.L.Sponsors, "luckypenny", 26.0 * Day);
	Expect(!Penny.empty() && Local.L.Inbox.empty(), "offers: the Lucky Penny's offer waits for Ray, no letter yet");
	sponsor::HeardOffer(Local.L.Sponsors, Local.L, Penny, 26.0 * Day);
	sponsor::HeardOffer(Local.L.Sponsors, Local.L, Penny, 26.0 * Day + 5.0);
	Expect(Local.Letters(sponsor::LetterHeard) == 1 && Local.L.Inbox.back().From == "Lucky Penny #212", "offers: heard in person, it's put in writing once");
	// Turnstone needs venues that aren't playable yet: it never looks on its own.
	World Star;
	Star.P = Streamer(30000, 700.0);
	Star.P.Partner = true;
	Star.Step(20.0 * Day, 50.0 * Day);
	Expect(std::none_of(Star.L.Sponsors.Contracts.begin(), Star.L.Sponsors.Contracts.end(), [](const sponsor::Contract& C) { return C.Brand == "turnstone"; }) &&
			   std::any_of(Star.L.Sponsors.Contracts.begin(), Star.L.Sponsors.Contracts.end(), [](const sponsor::Contract& C) { return C.Brand == "riverline"; }),
		"offers: the site at tier 4 offers, Turnstone doesn't (P2)");
}

// ------------------------------------------------------------------ 4. exclusivity, the overlay, the patch spots

void Exclusivity()
{
	World W;
	const double T0 = 30.0 * Day;
	const std::string Volt = W.Signed("voltrush", T0);
	const std::string Over = sponsor::MakeOffer(W.L.Sponsors, W.L, W.P, "overclock", T0 + 10.0, "test-seed", true);
	const std::string Why = sponsor::CanSign(W.L.Sponsors, W.L, Over, T0 + 20.0);
	Expect(Why.find("Volt Rush has your energy-drink category until") == 0, "exclusivity: signing a second energy drink is refused with the reason");
	Expect(!sponsor::Conflicts(W.L.Sponsors, W.L, Over, T0 + 20.0).empty(), "exclusivity: the conflict shows over Sign");
	// Two offers in the category on the table: signing one passes on the other.
	World Two;
	const std::string A = sponsor::MakeOffer(Two.L.Sponsors, Two.L, Two.P, "voltrush", T0, "s", true);
	const std::string B = sponsor::MakeOffer(Two.L.Sponsors, Two.L, Two.P, "overclock", T0, "s", true);
	bool Warned = false;
	for (const std::string& L : sponsor::Conflicts(Two.L.Sponsors, Two.L, A, T0))
	{
		Warned = Warned || L.find("Only one can be signed") == 0;
	}
	Expect(Warned, "exclusivity: Mail warns only one can be signed");
	Expect(sponsor::Sign(Two.L.Sponsors, Two.L, Two.Bank, A, T0).empty(), "exclusivity: Volt Rush signs");
	const sponsor::Contract* Passed = sponsor::Find(Two.L.Sponsors, B);
	Expect(Passed && Passed->St == sponsor::State::Ended && Passed->Why == sponsor::End::Declined && Two.L.Sponsors.Cooldown["overclock"] >= T0 + 59.0 * Day,
		"exclusivity: Overclock's offer turns passed, and it won't look for sixty days");
	// The overlay's two slots.
	World Slots;
	Slots.Signed("halo", T0);
	Slots.Signed("tunnelrat", T0);
	const std::string Third = sponsor::MakeOffer(Slots.L.Sponsors, Slots.L, Slots.P, "sitwell", T0, "s", true);
	Expect(sponsor::CanSign(Slots.L.Sponsors, Slots.L, Third, T0) == "Both overlay slots are spoken for.", "exclusivity: two overlay slots");
	// One patch per spot: the barber's sleeve and the gym's sleeve.
	World Sleeves;
	Sleeves.Signed("barber", T0);
	const std::string Gym = sponsor::MakeOffer(Sleeves.L.Sponsors, Sleeves.L, Sleeves.P, "ironhouse", T0, "s", true);
	Expect(sponsor::CanSign(Sleeves.L.Sponsors, Sleeves.L, Gym, T0).find("No room for IronHouse's patch") == 0, "exclusivity: one patch per spot");
	// The penny goes on the cap when the chest is taken; then the cap is Volt Rush's and there's no spot left.
	World Spots;
	Spots.Signed("embercrest", T0);
	Expect(!Spots.Signed("luckypenny", T0).empty() && Spots.Of("luckypenny") && Spots.Of("luckypenny")->St == sponsor::State::Active, "exclusivity: the penny fits on the cap");
	const std::string Cap = sponsor::MakeOffer(Spots.L.Sponsors, Spots.L, Spots.P, "voltrush", T0, "s", true);
	Expect(!sponsor::CanSign(Spots.L.Sponsors, Spots.L, Cap, T0).empty(), "exclusivity: with chest and cap spoken for, Volt Rush can't sign");
	(void)Volt;
}

// ------------------------------------------------------------------ 5. reviews: paid in arrears, once

void Reviews()
{
	World W;
	const double T0 = 40.0 * Day + 600.0;
	const std::string Id = W.Signed("voltrush", T0);
	Expect(W.Ledger("Volt Rush") == 0 && W.Bank == 100000, "reviews: nothing is paid on signing (stipends come in arrears)");
	Expect(W.Letters(sponsor::LetterWelcome) == 1, "reviews: a welcome letter");
	// Three live events in the cap each month.
	for (int M = 0; M < 3; ++M)
	{
		for (int K = 0; K < 3; ++K)
		{
			W.Played(T0 + (static_cast<double>(M) * 30.0 + 3.0 + 7.0 * K) * Day, "voltrush");
		}
	}
	W.Step(T0, T0 + 29.0 * Day);
	Expect(W.Ledger("Volt Rush: monthly") == 0, "reviews: no stipend before the review");
	W.Step(T0 + 29.0 * Day, T0 + 31.0 * Day);
	Expect(W.Ledger("Volt Rush: monthly") == 15000 && W.Bank == 115000, "reviews: the first review pays $150");
	W.Step(T0, T0 + 31.0 * Day);
	W.Step(T0 + 30.0 * Day, T0 + 31.0 * Day);
	Expect(W.Ledger("Volt Rush: monthly") == 15000, "reviews: stepping the span again pays once");
	// A save in the middle of the next period, then on.
	std::string Saved;
	sponsor::WriteLines(W.L, Saved);
	World Back;
	Back.L = W.L;
	Back.L.Sponsors = sponsor::Book();
	Back.Bank = W.Bank;
	size_t I = 0;
	while (I < Saved.size())
	{
		const size_t E = Saved.find('\n', I);
		std::vector<std::string> F(1);
		for (size_t K = I; K < E; ++K)
		{
			if (Saved[K] == '\t')
			{
				F.emplace_back();
			}
			else
			{
				F.back().push_back(Saved[K]);
			}
		}
		sponsor::ReadLine(F, Back.L);
		I = E + 1;
	}
	std::string Again;
	sponsor::WriteLines(Back.L, Again);
	Expect(Again == Saved, "reviews: the book writes back as it was read");
	Back.Step(T0 + 31.0 * Day, T0 + 95.0 * Day);
	Expect(Back.Ledger("Volt Rush: monthly") == 45000 && Back.Bank == 145000, "reviews: three periods pay three times, across a save");
	const sponsor::Contract* C = sponsor::Find(Back.L.Sponsors, Id);
	Expect(C && C->St == sponsor::State::Ended && C->Why == sponsor::End::Lapsed && C->Kept, "reviews: a clean term ends lapsed, the gear the player's");
	Expect(Back.Letters(sponsor::LetterRenewal) == 1, "reviews: fourteen days out, the brand mails a renewal");
	Expect(Back.Texts.size() >= 8, "reviews: a case at the door every Monday");
	Expect(Back.L.Pantry["volt-rush"] >= 12 * 12, "reviews: twelve cans a week");
}

// ------------------------------------------------------------------ 6. measuring the asks

void Measuring()
{
	World W;
	const double T0 = 50.0 * Day + 60.0;
	W.Signed("overclock", T0);
	sponsor::Contract* C = W.Of("overclock");
	// Streams that started in the period count; one before it doesn't; the stream in progress does.
	ss::kast::StreamLog S1;
	S1.Start = T0 + Day;
	S1.Minutes = 600.0;
	ss::kast::StreamLog S0;
	S0.Start = T0 - Day;
	S0.Minutes = 600.0;
	W.Ch.Log = {S1, S0};
	sponsor::Tally T = sponsor::Measure(*C, W.L, W.Ch, T0 + 3.0 * Day);
	Expect(std::fabs(T.LiveMinutes - 600.0) < 1e-6, "measure: streams started in the period count, earlier ones don't");
	T = sponsor::Measure(*C, W.L, W.Ch, T0 + 3.0 * Day, T0 + 3.0 * Day - 90.0);
	Expect(std::fabs(T.LiveMinutes - 690.0) < 1e-6, "measure: the stream in progress counts");
	// Reads through Kast's view: one a stream.
	std::vector<ss::kast::Deal> View = sponsor::StreamView(W.L.Sponsors, T0 + Day);
	Expect(View.size() == 1 && View[0].Id == "overclock" && View[0].ReadCents == 1000 && View[0].PerHourCents == 300 && View[0].Overlay, "measure: Kast sees the deal with its rates");
	View[0].ReadAt = T0 + Day + 30.0;
	View[0].EarnedCents += 1000;
	sponsor::AbsorbStream(W.L.Sponsors, View);
	sponsor::AbsorbStream(W.L.Sponsors, View);
	C = W.Of("overclock");
	Expect(C->Now.Reads == 1 && C->PaidCents == 1000, "measure: a read counts once, however often it's handed back");
	// Teaching streams by title and two hours.
	World R;
	R.Signed("rangewise", T0);
	ss::kast::StreamLog Lesson;
	Lesson.Start = T0 + Day;
	Lesson.Minutes = 125.0;
	Lesson.Title = ss::kast::Titles()[3].Text;
	ss::kast::StreamLog Short = Lesson;
	Short.Minutes = 110.0;
	ss::kast::StreamLog Other = Lesson;
	Other.Title = ss::kast::Titles()[0].Text;
	R.Ch.Log = {Lesson, Short, Other};
	Expect(sponsor::Measure(*R.Of("rangewise"), R.L, R.Ch, T0 + 2.0 * Day).Teaching == 1, "measure: a teaching stream is titled Teaching and runs two hours");
	// Live events in the patch: only the player's own rows, only finished in the period.
	World P;
	P.Signed("luckypenny", T0);
	P.Played(T0 + Day, "luckypenny");
	P.Played(T0 + 2.0 * Day, "");
	P.Played(T0 - Day, "luckypenny");
	life::LiveEntry Theirs;
	Theirs.Id = "embercrest-noon@" + std::to_string(ss::net::DayOf(T0 + 3.0 * Day));
	Theirs.State = life::LiveEntry::Finished;
	Theirs.FinishedAt = T0 + 3.0 * Day;
	Theirs.Roster = {{"Mei", 0}};
	Theirs.Patches = {{"Mei", "luckypenny"}};
	P.L.LiveEntries.push_back(Theirs);
	T = sponsor::Measure(*P.Of("luckypenny"), P.L, P.Ch, T0 + 4.0 * Day);
	Expect(T.LiveEvents == 1 && T.Embercrest == 3, "measure: live events in the patch are the player's own, in the period");
	// Workouts from the gym's count; appearances only inside the window.
	World G;
	G.L.Fit.Workouts = 12;
	G.Signed("ironhouse", T0);
	G.L.Fit.Workouts = 15;
	Expect(sponsor::Measure(*G.Of("ironhouse"), G.L, G.Ch, T0 + Day).Workouts == 3, "measure: workouts since the period began");
	const double Night = std::floor(T0 / Day) * Day + Day + 23.0 * 60.0;
	const double Noon = std::floor(T0 / Day) * Day + Day + 12.0 * 60.0;
	Expect(sponsor::Appear(P.L.Sponsors, "store", Night) != "" && sponsor::Appear(P.L.Sponsors, "barber", Noon) != "", "measure: no appearance outside the window or the shop");
	Expect(sponsor::Appear(P.L.Sponsors, "store", Noon).empty() && P.Of("luckypenny")->Now.Appearances == 1, "measure: the photo with Ray, ten to ten");
	Expect(sponsor::Appear(P.L.Sponsors, "store", Noon + 60.0) == "You've done that one this month.", "measure: one photo a month");
	// Team Embercrest's meet: a Sunday from a quarter to six through the first level (cards at seven), so the player off the
	// bus (seated at ten to seven) can still make it from the chair; not after.
	for (const double At : {19.0 * 60.0 + 5.0, 19.0 * 60.0 + 25.0})
	{
		World Em;
		Em.Signed("embercrest", T0);
		int Sunday = ss::net::DayOf(T0) + 1;
		while (ss::net::Weekday(Sunday) != 6)
		{
			++Sunday;
		}
		const double Base = static_cast<double>(Sunday) * Day;
		const std::vector<sponsor::Window> Ws = sponsor::Appearances(Em.L.Sponsors, Base, Base + Day);
		const bool Listed = Ws.size() == 1 && Ws[0].Venue == "embercrest" && std::fabs(Ws[0].From - (Base + 17.0 * 60.0 + 45.0)) < 1e-6 && std::fabs(Ws[0].To - (Base + 19.0 * 60.0 + 20.0)) < 1e-6;
		const std::string Why = sponsor::Appear(Em.L.Sponsors, "embercrest", Base + At);
		if (At < 19.0 * 60.0 + 20.0)
		{
			Expect(Listed && Why.empty() && Em.Of("embercrest")->Now.Appearances == 1, "measure: Meet the regulars from the chair in the Sunday's first level");
		}
		else
		{
			Expect(Listed && !Why.empty() && Em.Of("embercrest")->Now.Appearances == 0, "measure: no meet once the first level is over");
		}
	}
}

// ------------------------------------------------------------------ 7. strikes and cures

void Strikes()
{
	World W;
	const double T0 = 60.0 * Day + 600.0;
	const std::string Id = W.Signed("voltrush", T0);
	W.L.Closet.Patches[2] = "voltrush";
	// Month one: one live event of three. A strike: half paid, half held, two weeks to cure.
	W.Played(T0 + 5.0 * Day, "voltrush");
	W.Step(T0, T0 + 30.5 * Day);
	const sponsor::Contract* C = sponsor::Find(W.L.Sponsors, Id);
	Expect(C && C->Strikes == 1 && C->HeldCents == 7500 && W.Ledger("Volt Rush: monthly") == 7500, "strikes: a month short pays half and holds half");
	Expect(C && std::fabs(C->CureUntil - (T0 + 30.0 * Day + 14.0 * Day)) < 1e-6 && C->Short.LiveEvents == 2, "strikes: two weeks to make up the two events");
	Expect(W.Letters(sponsor::LetterStrike) == 1 && W.Mailed("1 of 3 live events in the patch"), "strikes: the letter names what's missing");
	// The cure: two events in the patch inside two weeks.
	W.Played(T0 + 33.0 * Day, "voltrush");
	W.Played(T0 + 35.0 * Day, "voltrush");
	W.Step(T0 + 30.5 * Day, T0 + 36.0 * Day);
	C = sponsor::Find(W.L.Sponsors, Id);
	Expect(C && C->CureUntil == 0.0 && C->HeldCents == 0 && W.Ledger("Volt Rush: held money") == 7500 && W.Letters(sponsor::LetterCured) == 1, "strikes: cured, the held half is paid");
	// Month two short again: the second strike in a term ends it, and the patch comes off.
	W.Step(T0 + 36.0 * Day, T0 + 61.0 * Day);
	C = sponsor::Find(W.L.Sponsors, Id);
	Expect(C && C->St == sponsor::State::Ended && C->Why == sponsor::End::ForCause && !C->Kept, "strikes: a second strike ends it for cause");
	Expect(W.L.Closet.Patches[2].empty() && W.L.Sponsors.Cooldown["voltrush"] >= T0 + 60.0 * Day + 119.0 * Day, "strikes: the patch comes off; 120 days before they look again");
	// A cure not made: ended at the deadline.
	World U;
	const std::string Id2 = U.Signed("luckypenny", T0);
	U.Step(T0, T0 + 30.5 * Day);
	const sponsor::Contract* K = sponsor::Find(U.L.Sponsors, Id2);
	Expect(K && K->Strikes == 1 && K->CureUntil > 0.0, "strikes: the Lucky Penny strikes a month with no photo and no events");
	U.Step(T0 + 30.5 * Day, T0 + 43.0 * Day);
	// Halved comps: the penny's weekly credit comes at half in the month after a strike (four full weeks, then two halves).
	Expect(U.L.Sponsors.StoreCreditCents == 4 * 2500 + 2 * 1250, "strikes: the credit after a strike comes at half");
	U.Step(T0 + 43.0 * Day, T0 + 45.0 * Day);
	K = sponsor::Find(U.L.Sponsors, Id2);
	Expect(K && K->St == sponsor::State::Ended && K->Why == sponsor::End::ForCause, "strikes: no cure in two weeks ends it");
	Expect(U.L.Sponsors.StoreCreditCents == 0, "strikes: ended for cause, the unused credit lapses");
	// A cure made brings the held comps back: the next week's credit is whole again.
	World Cu;
	const std::string Id3 = Cu.Signed("luckypenny", T0);
	Cu.Step(T0, T0 + 30.5 * Day);
	Cu.Played(T0 + 31.0 * Day, "luckypenny");
	Cu.Played(T0 + 32.0 * Day, "luckypenny");
	const double Noon = std::floor((T0 + 31.0 * Day) / Day) * Day + 12.0 * 60.0;
	Expect(sponsor::Appear(Cu.L.Sponsors, "store", Noon).empty(), "strikes: the photo inside the cure");
	Cu.Step(T0 + 30.5 * Day, T0 + 33.0 * Day);
	const sponsor::Contract* Kc = sponsor::Find(Cu.L.Sponsors, Id3);
	Expect(Kc && Kc->CureUntil == 0.0 && !Kc->CompsHalved && Cu.Letters(sponsor::LetterCured) == 1, "strikes: cured, the comps are whole again");
	Cu.Step(T0 + 33.0 * Day, T0 + 36.0 * Day);
	Expect(Cu.L.Sponsors.StoreCreditCents == 5 * 2500, "strikes: the week after the cure, the full credit");
}

// ------------------------------------------------------------------ 8. the morals clause

void Conduct()
{
	World W;
	const double T0 = 70.0 * Day + 600.0;
	W.Signed("luckypenny", T0);
	W.Signed("voltrush", T0);
	W.L.Closet.Patches[0] = "luckypenny";
	W.L.Closet.Patches[2] = "voltrush";
	sponsor::Conduct(W.L.Sponsors, W.L, W.Bank, sponsor::Breach::PickedUp, T0 + 2.0 * Day);
	Expect(W.Of("luckypenny") && W.Of("luckypenny")->St == sponsor::State::Active && W.L.Sponsors.PennyPass, "conduct: the owner lets one pickup slide");
	Expect(!W.Of("voltrush") && W.L.Closet.Patches[2].empty() && W.L.Closet.Patches[0] == "luckypenny", "conduct: Volt Rush ends that day and its patch comes off");
	Expect(W.L.Sponsors.Cooldown["*"] >= T0 + 2.0 * Day + 179.0 * Day, "conduct: brands stay away 180 days");
	sponsor::Conduct(W.L.Sponsors, W.L, W.Bank, sponsor::Breach::PickedUp, T0 + 9.0 * Day);
	Expect(!W.Of("luckypenny") && W.L.Closet.Patches[0].empty(), "conduct: a second pickup ends the penny too");
	// A restriction ends the site and training deals only.
	World R;
	R.Signed("riverline", T0);
	R.Signed("voltrush", T0);
	sponsor::Conduct(R.L.Sponsors, R.L, R.Bank, sponsor::Breach::AccountRestricted, T0 + Day);
	Expect(!R.Of("riverline") && R.Of("voltrush"), "conduct: a restriction ends Team RiverLine, not the energy drink");
	Expect(R.L.Sponsors.Cooldown["cat:" + std::to_string(static_cast<int>(sponsor::Category::Site))] >= T0 + 364.0 * Day, "conduct: no site offers for a year");
	// A clawback stops at the rent reserve and writes off the rest.
	World C;
	const std::string Off = sponsor::MakeOffer(C.L.Sponsors, C.L, C.P, "stacked", T0, "s", true);
	sponsor::Find(C.L.Sponsors, Off)->T.SigningCents = 60000;
	sponsor::Find(C.L.Sponsors, Off)->T.ConductClause = true;
	C.Bank = 0;
	sponsor::Sign(C.L.Sponsors, C.L, C.Bank, Off, T0);
	Expect(C.Bank == 60000 && C.Ledger("Stacked: signing") == 60000, "conduct: a signing bonus is paid on signing");
	C.L.RentStage = life::Rent::Due;
	C.L.RentDueCents = 107500;
	C.L.RentDeadline = T0 + 3.0 * Day;
	C.Bank = 120000;
	sponsor::Conduct(C.L.Sponsors, C.L, C.Bank, sponsor::Breach::PickedUp, T0 + Day);
	Expect(C.Bank == 107500 && C.Ledger("signing money back") == -12500, "conduct: the clawback stops at the rent due this week");
	Expect(C.L.Inbox.back().Body.find("Consider the rest a lesson.") != std::string::npos, "conduct: the rest is written off in the letter");
	Expect(C.Bank >= 0, "conduct: never a negative balance");
}

// ------------------------------------------------------------------ 9. counters (§3.7: rules, no randomness)

void Counters()
{
	using L = sponsor::Lever;
	const double Levels[] = {0.0, 0.15, 0.3, 0.35, 0.55, 0.8};
	// Money, Lighter, Shorter, OneMoreEntry, NoExclusivity at each leverage: 2 met, 1 halfway, 0 pass.
	const int Want[6][5] = {{0, 0, 0, 0, 0}, {1, 1, 2, 0, 0}, {1, 2, 2, 0, 0}, {2, 2, 2, 0, 0}, {2, 2, 2, 2, 0}, {2, 2, 2, 2, 2}};
	bool Table = true;
	for (int I = 0; I < 6; ++I)
	{
		for (int K = 0; K < 5; ++K)
		{
			const int Got = sponsor::CounterOutcome(static_cast<L>(K), Levels[I]);
			if (Got != Want[I][K])
			{
				Table = false;
				std::printf("  counter lever %d at %.2f: %d, wanted %d\n", K, Levels[I], Got, Want[I][K]);
			}
		}
	}
	Expect(Table, "counters: the table at leverage 0, 0.15, 0.3, 0.35, 0.55, 0.8");
	Expect(std::string(ss::rep::Mood(0.05)) == "Sounds open to it" && std::string(ss::rep::Mood(-0.1)) == "That's a stretch" && std::string(ss::rep::Mood(-0.3)) == "Don't push it",
		"counters: the mood lines");
	// Volt Rush at leverage 0.15 to 0.35 meets more money halfway: $150 becomes $170, the next business morning.
	World W;
	W.P = Streamer(600, 25.0); // tier 2: 0 past the gate; a rival offer adds 0.15
	const double T0 = 81.0 * Day + 15.0 * 60.0; // a Friday afternoon
	const std::string Id = sponsor::MakeOffer(W.L.Sponsors, W.L, W.P, "voltrush", T0, "s", true);
	sponsor::MakeOffer(W.L.Sponsors, W.L, W.P, "overclock", T0, "s", true);
	const double Lev = sponsor::Leverage(W.L.Sponsors, W.P, "voltrush");
	Expect(std::fabs(Lev - 0.15) < 1e-9, "counters: a rival's offer in the category is +0.15");
	Expect(sponsor::Counter(W.L.Sponsors, W.P, Id, L::Money, T0).empty(), "counters: the counter is sent");
	Expect(sponsor::Counter(W.L.Sponsors, W.P, Id, L::Lighter, T0 + 1.0) == "Waiting on their answer.", "counters: one counter per offer");
	const sponsor::Contract* C = sponsor::Find(W.L.Sponsors, Id);
	const int ReplyDay = ss::net::DayOf(C->ReplyAt);
	Expect(ss::net::Weekday(ReplyDay) == 0 && C->ReplyAt - ReplyDay * Day >= 9.0 * 60.0 && C->ReplyAt - ReplyDay * Day < 10.5 * 60.0,
		"counters: Friday's counter is answered Monday morning");
	W.Step(T0, T0 + 4.0 * Day);
	C = sponsor::Find(W.L.Sponsors, Id);
	Expect(C && C->St == sponsor::State::Offered && C->T.MonthlyCents == 17000 && W.Mailed("Can do $170 a month. That's the ceiling on my end."),
		"counters: halfway is +12%, rounded to the five dollars");
	// No leverage: they pass, and wait sixty days.
	World N;
	N.P = Streamer(600, 25.0);
	const std::string Gone = sponsor::MakeOffer(N.L.Sponsors, N.L, N.P, "voltrush", T0, "s", true);
	sponsor::Counter(N.L.Sponsors, N.P, Gone, L::NoExclusivity, T0);
	N.Step(T0, T0 + 4.0 * Day);
	C = sponsor::Find(N.L.Sponsors, Gone);
	Expect(C && C->St == sponsor::State::Ended && C->Why == sponsor::End::Withdrawn && N.L.Sponsors.Cooldown["voltrush"] > T0 + 59.0 * Day, "counters: past their room, they pass for sixty days");
	Expect(sponsor::LeverBlocked(*C, L::OneMoreEntry) == "They don't cover entries.", "counters: a lever that means nothing on this deal is blocked");
	// A clean previous term is +0.1; a strike is -0.2.
	World H;
	H.Signed("voltrush", T0);
	sponsor::Contract* Old = H.Of("voltrush");
	Old->St = sponsor::State::Ended;
	Old->Why = sponsor::End::Lapsed;
	Expect(std::fabs(sponsor::Leverage(H.L.Sponsors, H.P, "voltrush") - 0.1) < 1e-9, "counters: a clean term before is +0.1");
	H.L.Sponsors.Strikes.push_back(T0);
	Expect(std::fabs(sponsor::Leverage(H.L.Sponsors, H.P, "voltrush") + 0.1) < 1e-9, "counters: a strike is -0.2");
	H.L.Sponsors.SteppedTo = T0 + 200.0 * Day;
	Expect(std::fabs(sponsor::Leverage(H.L.Sponsors, H.P, "voltrush") - 0.1) < 1e-9, "counters: a strike more than 180 days old no longer counts");
}

// ------------------------------------------------------------------ 10. renewals

void Renewals()
{
	const double T0 = 90.0 * Day + 600.0;
	// Grown three tiers since signing, every ask met: (0.85 + 0.3) x 1.3 = 1.495, held to 1.4.
	World W;
	W.P = Streamer(600, 25.0);
	const std::string Id = W.Signed("voltrush", T0);
	for (int M = 0; M < 3; ++M)
	{
		for (int K = 0; K < 3; ++K)
		{
			W.Played(T0 + (static_cast<double>(M) * 30.0 + 2.0 + 5.0 * K) * Day, "voltrush");
		}
	}
	W.P = Streamer(30000, 700.0);
	W.Step(T0, T0 + 77.0 * Day);
	const sponsor::Contract* R = nullptr;
	for (const sponsor::Contract& C : W.L.Sponsors.Contracts)
	{
		R = C.Renews == Id ? &C : R;
	}
	Expect(R && R->T.MonthlyCents == 21000, "renewal: rates rise at most 40%");
	// Barely met (60%): (0.85 + 0.18) = 1.03.
	World Lo;
	Lo.P = Streamer(600, 25.0);
	const std::string Id2 = Lo.Signed("overclock", T0);
	sponsor::Contract* C2 = Lo.Of("overclock");
	C2->Period = 2;     // two reviews done,
	C2->MetSum = 1.2;   // 60% of the asks met over them
	C2->T.Days = 90;
	C2->Until = C2->Since + 90.0 * Day;
	Lo.Step(T0, T0 + 77.0 * Day);
	const sponsor::Contract* R2 = nullptr;
	for (const sponsor::Contract& C : Lo.L.Sponsors.Contracts)
	{
		R2 = C.Renews == Id2 ? &C : R2;
	}
	Expect(R2 && R2->T.PerLiveHourCents >= 300 && R2->T.PerLiveHourCents <= 420 && R2->T.MonthlyCents >= 10000, "renewal: Overclock renews with the sleeve and $100 a month");
	Expect(R2 && std::find(R2->T.Wants.begin(), R2->T.Wants.end(), sponsor::Spot::Sleeve) != R2->T.Wants.end() && R2->T.Days == 90, "renewal: ninety days, the sleeve too");
	// The right to match: a rival's richer offer in the category, and the incumbent matches it (inside its cap).
	World M;
	M.P = Streamer(600, 25.0);
	const std::string Id3 = M.Signed("voltrush", T0);
	for (int K = 0; K < 9; ++K)
	{
		M.Played(T0 + (2.0 + 9.0 * K) * Day, "voltrush");
	}
	M.Step(T0, T0 + 70.0 * Day);
	const std::string Rival = sponsor::MakeOffer(M.L.Sponsors, M.L, M.P, "overclock", T0 + 75.0 * Day, "s", true);
	sponsor::Find(M.L.Sponsors, Rival)->T.MonthlyCents = 20000;
	M.Step(T0 + 70.0 * Day, T0 + 77.0 * Day);
	const sponsor::Contract* R3 = nullptr;
	for (const sponsor::Contract& C : M.L.Sponsors.Contracts)
	{
		R3 = C.Renews == Id3 ? &C : R3;
	}
	Expect(R3 && R3->T.MonthlyCents == 20000 && M.Mailed("we matched it"), "renewal: the incumbent matches a rival");
	// Team Embercrest after a clean term: three covered entries.
	World E;
	const std::string Id4 = E.Signed("embercrest", T0);
	sponsor::Contract* K4 = E.Of("embercrest");
	K4->Period = 2;
	K4->MetSum = 2.0;
	K4->T.Asks = {}; // the last month met too: what's tested is the renewal, not the review
	E.Step(T0, T0 + 77.0 * Day);
	const sponsor::Contract* R4 = nullptr;
	for (const sponsor::Contract& C : E.L.Sponsors.Contracts)
	{
		R4 = C.Renews == Id4 ? &C : R4;
	}
	Expect(R4 && R4->T.CoveredEntries == 3, "renewal: three covered entries after a clean term");
	// Signed ahead, the new term starts where the old ends and the patch stays on.
	E.L.Closet.Patches[0] = "embercrest";
	Expect(R4 && sponsor::Sign(E.L.Sponsors, E.L, E.Bank, R4->Id, T0 + 78.0 * Day).empty(), "renewal: signed");
	const sponsor::Contract* Next = sponsor::Find(E.L.Sponsors, R4 ? R4->Id : std::string());
	Expect(Next && std::fabs(Next->Since - (T0 + 90.0 * Day)) < 1e-6, "renewal: the new term starts at the old one's end");
	E.Step(T0 + 77.0 * Day, T0 + 95.0 * Day);
	Expect(E.L.Closet.Patches[0] == "embercrest" && sponsor::Active(E.L.Sponsors, "embercrest", T0 + 95.0 * Day), "renewal: the crest stays on across the renewal");
	// Team RiverLine renewed ahead: the new term's first Daily Main tickets come when it starts, not never.
	World Rl;
	const std::string Id5 = Rl.Signed("riverline", T0);
	Expect(Rl.L.TicketsFor("daily-main") == 2, "renewal: Team RiverLine's tickets come at signing");
	sponsor::Contract* K5 = Rl.Of("riverline");
	K5->T.Asks = {};
	const double End5 = K5->Until;
	Rl.Step(T0, End5 - 10.0 * Day);
	std::string R5;
	for (const sponsor::Contract& C : Rl.L.Sponsors.Contracts)
	{
		R5 = C.Renews == Id5 ? C.Id : R5;
	}
	Expect(!R5.empty() && sponsor::Sign(Rl.L.Sponsors, Rl.L, Rl.Bank, R5, End5 - 10.0 * Day).empty(), "renewal: Team RiverLine signs on");
	const int Before = Rl.L.TicketsFor("daily-main");
	Rl.Step(End5 - 10.0 * Day, End5 - Day);
	Expect(Rl.L.TicketsFor("daily-main") == Before, "renewal: no tickets before the new term starts");
	Rl.Step(End5 - Day, End5 + Day);
	Expect(Rl.L.TicketsFor("daily-main") == Before + 2 && sponsor::Active(Rl.L.Sponsors, "riverline", End5 + Day), "renewal: the new term's first tickets come when it starts");
}

// ------------------------------------------------------------------ 11. live money (§1.2)

void LiveMoney()
{
	const double T0 = 100.0 * Day + 8.0 * 60.0; // a Monday? any weekday: the Noon Deepstack runs Monday to Friday
	int D = static_cast<int>(T0 / Day);
	while (ss::net::Weekday(D) > 4)
	{
		++D;
	}
	const double Morning = D * Day + 9.0 * 60.0 + 30.0;
	World W;
	W.Bank = 10000;
	W.Signed("luckypenny", Morning - Day);
	const live::Occurrence Noon = live::FindOccurrence("embercrest-noon@" + std::to_string(D));
	Expect(Noon.Valid(), "live: the Noon Deepstack runs today");
	const sponsor::Contract* Cov = sponsor::Covering(W.L.Sponsors, Noon, Morning);
	Expect(Cov && Cov->Brand == "luckypenny", "live: the Lucky Penny covers a Noon Deepstack");
	const live::Occurrence Nightly = live::FindOccurrence("embercrest-nightly@" + std::to_string(D));
	Expect(!sponsor::Covering(W.L.Sponsors, Nightly, Morning), "live: not the Nightly");
	const life::Funding F = sponsor::CoverFunding(*Cov, Noon);
	Expect(F.From == life::Paid::Sponsor && F.Who == "Lucky Penny" && F.Cents == 8000 && !F.Fares, "live: the funding names the brand");
	const size_t Lines = W.L.Ledger.size();
	const std::string Why = live::Register(W.Bank, W.L, Noon, Morning, 70, {}, F);
	Expect(Why.empty() && W.Bank == 10000, "live: a covered entry costs nothing");
	Expect(W.L.Ledger.size() == Lines + 3 && W.L.Ledger[2].Amount == 8000 && W.L.Ledger[2].Label == "Lucky Penny: covered the Noon Deepstack" && W.L.Ledger[1].Amount == -7000 &&
			   W.L.Ledger[0].Amount == -1000,
		"live: +$80 (Lucky Penny), -$70, -$10");
	Expect(W.Of("luckypenny")->Now.CoveredUsed == 1 && !sponsor::Covering(W.L.Sponsors, live::FindOccurrence("embercrest-noon@" + std::to_string(D + 1)), Morning),
		"live: one covered a month, used");
	// A cover handed to the desk after it's gone (a screen drawn before the month's cover was used, or one the book never
	// had) is refused with the reason, and registering on it moves nothing.
	{
		World U;
		U.Bank = 10000;
		U.Signed("luckypenny", Morning - Day);
		const life::Funding Fu = sponsor::CoverFunding(*U.Of("luckypenny"), Noon);
		U.Of("luckypenny")->Now.CoveredUsed = 1;
		life::Funding Made = Fu;
		Made.Ref = "c:nobody";
		Expect(live::CanRegister(U.Bank, U.L, Noon, Morning, Fu) == "Lucky Penny's covered entry is used this month." &&
				   live::CanRegister(U.Bank, U.L, Noon, Morning, Made) == "That cover's off. It's your own money tonight." && !live::Register(U.Bank, U.L, Noon, Morning, 70, {}, Fu).empty() &&
				   U.Bank == 10000 && U.L.LiveEntries.empty() && U.Of("luckypenny")->Now.CoveredUsed == 1,
			"live: a cover that's gone is refused at the desk, with why");
	}
	// Registering twice moves nothing twice.
	live::Register(W.Bank, W.L, Noon, Morning + 1.0, 70, {}, F);
	sponsor::OnRegister(W.L.Sponsors, W.L, W.Bank, *live::EntryFor(W.L, Noon.Id), Morning + 2.0);
	Expect(W.Bank == 10000 && W.L.Ledger.size() == Lines + 3, "live: the cover is paid once");
	// Refunded: the cover goes back and doesn't count as used.
	live::Refund(W.Bank, W.L, Noon.Id, Morning + 30.0, "called off");
	Expect(W.Bank == 10000 && W.Ledger("cover returned") == -8000 && W.Of("luckypenny")->Now.CoveredUsed == 0, "live: a refund returns the cover, not counted as used");
	live::Refund(W.Bank, W.L, Noon.Id, Morning + 31.0, "called off");
	sponsor::OnRefund(W.L.Sponsors, W.L, W.Bank, *live::EntryFor(W.L, Noon.Id), Morning + 32.0);
	Expect(W.Bank == 10000 && W.Ledger("cover returned") == -8000, "live: returned once");
	// The cut: 0% and 30% of the profit on a covered entry.
	for (const int Cut : {0, 30})
	{
		World K;
		K.Bank = 0;
		K.Signed("embercrest", Morning - Day);
		const live::Occurrence N = live::FindOccurrence("embercrest-nightly@" + std::to_string(D));
		// The cut is the contract's (a counter's lever): the cover's funding carries it, and the desk takes nothing else.
		K.Of("embercrest")->T.PrizeCutPct = Cut;
		const life::Funding Fk = sponsor::CoverFunding(*K.Of("embercrest"), N);
		live::Register(K.Bank, K.L, N, N.Start - 60.0, 80, {}, Fk);
		Expect(K.Bank == 0 && Fk.Fares, "live: Team Embercrest covers the Nightly and the bus");
		live::PayFare(K.Bank, K.L, N.Id, false, N.Start - 30.0);
		Expect(K.Bank == 0, "live: the covered bus nets to nothing");
		live::Settle(K.Bank, K.L, N.Id, 5, 80, 52000, N.Start + 300.0);
		const Chips Expected = 52000 - (52000 - 12000) * Cut / 100;
		Expect(K.Bank == Expected, Cut == 0 ? "live: no cut, the prize is the player's" : "live: a 30% cut of the profit");
	}
	// Bonuses only when the patch was worn.
	for (const bool Worn : {false, true})
	{
		World B;
		B.Bank = 0;
		B.Signed("voltrush", Morning - Day);
		B.L.Closet.Patches[2] = "voltrush";
		B.L.Closet.Worn[static_cast<size_t>(ss::wardrobe::Slot::Hat)] = Worn ? "hat.ballcap" : "";
		B.L.Closet.Owned.insert("hat.ballcap");
		B.Bank = 12000 + 580;
		live::Register(B.Bank, B.L, Nightly, Nightly.Start - 60.0, 80, {});
		life::LiveEntry* E = live::EntryFor(B.L, Nightly.Id);
		sponsor::NoteWorn(*E, "grinder_3c", B.L.Closet, B.L.Sponsors, Nightly.Start - 60.0);
		const Chips Before = B.Bank;
		live::Settle(B.Bank, B.L, Nightly.Id, 4, 80, 30000, Nightly.Start + 400.0);
		Expect(B.Bank - Before == 30000 + (Worn ? 5000 : 0), Worn ? "live: a final table in the cap pays Volt Rush's $50" : "live: no cap, no bonus");
		live::Settle(B.Bank, B.L, Nightly.Id, 4, 80, 30000, Nightly.Start + 401.0);
		Expect(B.Bank - Before == 30000 + (Worn ? 5000 : 0), "live: the bonus is paid once");
	}
	// The options the desk shows: the cover and why not.
	World O;
	O.Signed("embercrest", Morning - Day);
	const live::Occurrence Sunday = live::FindOccurrence("embercrest@" + std::to_string(D + (6 - ss::net::Weekday(D))));
	const std::vector<std::pair<life::Funding, std::string>> Opts = sponsor::CoverOptions(O.L.Sponsors, Sunday, Morning);
	Expect(Opts.size() == 1 && Opts[0].second == "Team Embercrest doesn't cover the Sunday.", "live: the Sunday isn't covered, and the desk says why");
}

// ------------------------------------------------------------------ 12. the save and the Kast migration (§3.8)

void Migration()
{
	const std::string Dir = std::string(SS_TESTS_DIR) + "/fixtures/";
	const std::string Text = ReadFile(Dir + "pre_segment_team_riverline.ssave");
	ss::SaveData D;
	const bool Parsed = ss::SaveData::Parse(Text, D);
	Expect(Parsed, "migration: the Team RiverLine save parses");
	const double World = ss::net::MinutesPerDay * ss::net::NightOneDay + D.ClockMinutes;
	const sponsor::Contract* Team = sponsor::Running(D.Life.Sponsors, "riverline", World);
	Expect(Team && Team->Legacy && std::fabs(Team->Until - 47280.33) < 0.01 && std::fabs(Team->Since - 4080.33) < 0.01, "migration: Team RiverLine is an active legacy contract, the same days left");
	Expect(Team && Team->T.PerLiveHourCents == 2500 && Team->T.PerReadCents == 6000 && !Team->T.ConductClause, "migration: on its old terms, hourly and reads, nothing asked");
	const std::string Offer = sponsor::OpenOfferFor(D.Life.Sponsors, "tunnelrat", World);
	Expect(!Offer.empty() && sponsor::Find(D.Life.Sponsors, Offer)->ExpiresAt == World + 7.0 * Day, "migration: TunnelRat's offer stands a week from load");
	bool Letter = false;
	for (const life::Letter& M : D.Life.Inbox)
	{
		Letter = Letter || (M.Thread == Offer && M.Body == "Picking this back up. The terms have changed since we last wrote; they're below.");
	}
	Expect(Letter, "migration: the offer is a letter in Mail");
	Expect(std::any_of(D.Life.Inbox.begin(), D.Life.Inbox.end(), [Team](const life::Letter& M) { return Team && M.Thread == Team->Id && M.Subject == "Team RiverLine: on the books."; }),
		"migration: the running deal shows in Mail too");
	Expect(D.Life.Sponsors.Cooldown.count("sitwell") == 1 && D.Life.Sponsors.Cooldown["sitwell"] == World + 60.0 * Day, "migration: Sitwell's no is sixty days, not never");
	Expect(D.Channel.Deals.empty() && D.Channel.Offers.empty() && D.Channel.Declined.empty(), "migration: Kast's own lists are cleared");
	const std::string Once = D.Serialize();
	Expect(Once.find("kast\tdeal") == std::string::npos && Once.find("kast\toffer") == std::string::npos && Once.find("sponsor\tbook") != std::string::npos,
		"migration: from here on only sponsor lines are written");
	ss::SaveData Again;
	Expect(ss::SaveData::Parse(Once, Again) && Again.Serialize() == Once, "migration: the migrated save round-trips byte for byte");
	Expect(Again.Life.Inbox.size() == D.Life.Inbox.size() && sponsor::Running(Again.Life.Sponsors, "riverline", World), "migration: and migrates once");
	// Saves from before sponsors write back as they were: no sponsor line.
	for (const char* Name : {"pre_segment_fresh.ssave", "pre_segment_mid_nightly.ssave", "pre_segment_evicted.ssave"})
	{
		ss::SaveData F;
		const std::string Saved = ReadFile(Dir + Name);
		Expect(ss::SaveData::Parse(Saved, F) && F.Serialize().find("sponsor\t") == std::string::npos, "migration: a save with no deals gets no sponsor lines");
	}
}

// ------------------------------------------------------------------ 13. Kast works on the view; Channel::Deals is never written

void KastView()
{
	World W;
	const double T0 = 110.0 * Day + 20.0 * 60.0;
	W.Signed("overclock", T0 - Day);
	W.Signed("halo", T0 - Day);
	// The trap: a deal left in Kast's own list (as an old save would have it, unmigrated). Kast must neither pay it nor
	// touch it: it works only on the book's view.
	ss::kast::Deal Sentinel;
	Sentinel.Id = "tunnelrat";
	Sentinel.Since = T0 - 2.0 * Day;
	Sentinel.Until = T0 + 20.0 * Day;
	Sentinel.PerHourCents = 600;
	Sentinel.ReadCents = 2500;
	W.Ch.Deals = {Sentinel};
	ss::kast::Stream St("kast-view");
	ss::kast::Inputs In;
	In.World = T0;
	In.Real = 0.0;
	In.Hero = "grinder_3c";
	In.Gear = ss::gear::Sum({{"ram-32", 0.0}});
	In.Deals = sponsor::StreamView(W.L.Sponsors, T0);
	St.Start(W.Ch, In);
	Expect(St.Live, "kast: the stream starts");
	for (int K = 1; K <= 60; ++K)
	{
		In.World = T0 + static_cast<double>(K);
		In.Real = static_cast<double>(K);
		In.Deals = sponsor::StreamView(W.L.Sponsors, In.World);
		St.Tick(W.Ch, In, 1.0, 1.0);
		sponsor::AbsorbStream(W.L.Sponsors, St.Deals);
		St.Deals.clear();
	}
	In.Deals = sponsor::StreamView(W.L.Sponsors, In.World);
	const Chips Read = St.SponsorRead(W.Ch, In, "overclock");
	sponsor::AbsorbStream(W.L.Sponsors, St.Deals);
	St.Deals.clear();
	In.Deals = sponsor::StreamView(W.L.Sponsors, In.World);
	const Chips Twice = St.SponsorRead(W.Ch, In, "overclock");
	const Chips Halo = St.SponsorRead(W.Ch, In, "halo");
	const Chips Old = St.SponsorRead(W.Ch, In, "tunnelrat");
	const sponsor::Contract* O = W.Of("overclock");
	const sponsor::Contract* H = W.Of("halo");
	Expect(Read == 1000 && Twice == 0 && Halo == 0, "kast: a read pays once a stream, and Halo doesn't read");
	Expect(O && O->PaidCents >= 1000 + 250 && O->Now.Reads == 1 && H && H->PaidCents >= 150, "kast: an hour live and a read, paid through the channel, kept on the contracts");
	Expect(W.Ch.Deals.size() == 1 && W.Ch.Deals[0].EarnedCents == 0 && W.Ch.Deals[0].ReadAt == Sentinel.ReadAt && W.Ch.Deals[0].Until == Sentinel.Until && W.Ch.Offers.empty() &&
			   Old == 0,
		"kast: Channel::Deals is never written, nor paid from");
	Expect(W.Ch.UnpaidCents >= 1000 + 250 + 150, "kast: Kast's pay is the channel's balance, as before");
}

// ------------------------------------------------------------------ 14. the exploits

void Exploits()
{
	const double T0 = 120.0 * Day + 600.0;
	// Sign, collect, quit: notice at once, nothing until the review, and nothing if the month's asks weren't met.
	World Q;
	const std::string Id = Q.Signed("voltrush", T0);
	Expect(sponsor::GiveNotice(Q.L.Sponsors, Q.L, Id, T0 + 1.0).empty() && Q.Bank == 100000, "exploit: notice at once pays nothing now");
	Q.Step(T0, T0 + 40.0 * Day);
	const sponsor::Contract* C = sponsor::Find(Q.L.Sponsors, Id);
	Expect(C && C->St == sponsor::State::Ended && C->Why == sponsor::End::Walked && Q.Bank == 100000 && Q.Ledger("Volt Rush") == 0, "exploit: a month not worked isn't paid");
	Expect(Q.L.Sponsors.Cooldown["voltrush"] >= T0 + 30.0 * Day + 179.0 * Day && Q.L.Sponsors.Cooldown["cat:" + std::to_string(static_cast<int>(sponsor::Category::Energy))] > T0 + 50.0 * Day,
		"exploit: notice keeps that brand away 180 days, the category 30");
	// Notice after a month worked: that month paid once.
	World P;
	const std::string Id2 = P.Signed("voltrush", T0);
	for (int K = 0; K < 3; ++K)
	{
		P.Played(T0 + (2.0 + 5.0 * K) * Day, "voltrush");
	}
	P.Step(T0, T0 + 10.0 * Day);
	sponsor::GiveNotice(P.L.Sponsors, P.L, Id2, T0 + 10.0 * Day);
	P.Step(T0 + 10.0 * Day, T0 + 70.0 * Day);
	P.Step(T0, T0 + 70.0 * Day);
	Expect(P.Ledger("Volt Rush: monthly") == 15000, "exploit: notice after a month worked pays that month, once");
	// Covered entries don't roll over: an unused month leaves two, not four.
	World E;
	E.Signed("embercrest", T0);
	E.Step(T0, T0 + 31.0 * Day);
	const sponsor::Contract* K = E.Of("embercrest");
	Expect(K && K->Now.CoveredUsed == 0 && K->T.CoveredEntries == 2, "exploit: covered entries lapse at the review");
	// The store credit is its own pot: it never touches the bankroll, and spends only at the counter.
	World S;
	S.Signed("luckypenny", T0);
	S.Step(T0, T0 + 15.0 * Day);
	Expect(S.L.Sponsors.StoreCreditCents == 5000 && S.Bank == 100000, "exploit: two weeks of credit, not a cent in the bankroll");
	Expect(sponsor::SpendCredit(S.L.Sponsors, 7000) == 2000 && S.L.Sponsors.StoreCreditCents == 0, "exploit: the credit pays first, the card the rest");
	// Sponsor gear is the brand's while the deal runs.
	World G;
	G.Signed("sitwell", T0);
	G.Of("sitwell")->Property.push_back("gear:chair");
	Expect(sponsor::IsProperty(G.L.Sponsors, "chair", T0 + Day) && sponsor::IsProperty(G.L.Sponsors, "gear:chair", T0 + Day), "exploit: the chair is Sitwell's during the term");
	G.Of("sitwell")->Kept = true;
	Expect(!sponsor::IsProperty(G.L.Sponsors, "chair", T0 + Day), "exploit: and the player's after a clean end");
	// Sign for the gear and walk: a thirty-day deal never worked keeps no chair, and notice on a month not worked keeps no
	// camera (a term finished clean keeps it: session_seg_sponsor's Property).
	World Chair;
	const std::string Sit = Chair.Signed("sitwell", T0);
	sponsor::Find(Chair.L.Sponsors, Sit)->Property.push_back("gear:chair");
	Chair.Step(T0, T0 + 31.0 * Day);
	const sponsor::Contract* Sc = sponsor::Find(Chair.L.Sponsors, Sit);
	Expect(Sc && Sc->St == sponsor::State::Ended && Sc->Why == sponsor::End::NotRenewed && !Sc->Kept && Chair.Mailed("We'll take back what we sent you."),
		"exploit: a deal never worked keeps no chair");
	World Cam;
	const std::string Hal = Cam.Signed("halo", T0);
	sponsor::Find(Cam.L.Sponsors, Hal)->Property.push_back("gear:webcam-720");
	sponsor::GiveNotice(Cam.L.Sponsors, Cam.L, Hal, T0 + 1.0);
	Cam.Step(T0, T0 + 31.0 * Day);
	const sponsor::Contract* Hc = sponsor::Find(Cam.L.Sponsors, Hal);
	Expect(Hc && Hc->Why == sponsor::End::Walked && !Hc->Kept && Cam.Mailed("We'll take back what we sent you."), "exploit: notice on a month not worked sends the camera back");
	// Gates can't be farmed in a day.
	World F;
	for (int K2 = 0; K2 < 30; ++K2)
	{
		sponsor::NoteVisit(F.L.Sponsors, "store", T0 + static_cast<double>(K2));
	}
	F.P = ss::rep::Public();
	Expect(!sponsor::Eligible(F.L.Sponsors, F.L, F.P, "luckypenny", T0 + Day).empty(), "exploit: thirty purchases in a day aren't twelve days");
	// Patches only for running deals, only on spots they take.
	ss::wardrobe::Closet Cl;
	Expect(!sponsor::Sew(Cl, F.L.Sponsors, 0, "luckypenny", T0).empty(), "exploit: no patch without a deal");
	World Sw;
	Sw.Signed("barber", T0);
	Expect(sponsor::Sew(Cl, Sw.L.Sponsors, 0, "barber", T0) == "Fifth St. Barbers' patch goes on the sleeve.", "exploit: the barber's patch only on the sleeve");
	Expect(sponsor::Sew(Cl, Sw.L.Sponsors, 1, "barber", T0).empty() && Cl.Patches[1] == "barber", "exploit: on the sleeve it goes");
}

// ------------------------------------------------------------------ 15. what the rest of the game reads

void Readers()
{
	Expect(sponsor::NpcWeekly("riverline") == 86500, "readers: Team RiverLine's pros earn $865 a week, not $1,500");
	Expect(sponsor::NpcWeekly("luckypenny") == 0, "readers: in-kind deals are no income");
	World W;
	const double T0 = 130.0 * Day + 600.0;
	W.Signed("luckypenny", T0);
	W.Signed("barber", T0);
	W.Signed("embercrest", T0);
	const std::vector<sponsor::Window> Win = sponsor::Appearances(W.L.Sponsors, T0, T0 + 7.0 * Day);
	int Penny = 0;
	int Chair = 0;
	int Meet = 0;
	bool Hours = true;
	for (const sponsor::Window& X : Win)
	{
		const double Start = X.From - std::floor(X.From / Day) * Day;
		const int Wd = ss::net::Weekday(ss::net::DayOf(X.From));
		if (X.Venue == "store")
		{
			++Penny;
			Hours = Hours && Start == 600.0 && X.To - X.From == 720.0;
		}
		if (X.Venue == "barber")
		{
			++Chair;
			Hours = Hours && Wd == 5 && Start == 780.0;
		}
		if (X.Venue == "embercrest")
		{
			++Meet;
			Hours = Hours && Wd == 6 && Start == 17.0 * 60.0 + 45.0;
		}
	}
	Expect(Penny >= 6 && Chair == 1 && Meet == 1 && Hours, "readers: the photo ten to ten, the chair Saturday one to five, the meet Sunday before cards");
	const std::vector<life::Mark> Marks = sponsor::Marks(W.L.Sponsors, T0, T0 + 100.0 * Day);
	int Reviews = 0;
	for (const life::Mark& M : Marks)
	{
		Reviews += M.Kind == sponsor::MarkReview ? 1 : 0;
	}
	Expect(Reviews == 3 + 2 + 3, "readers: the calendar shows each review");
	const std::vector<std::pair<std::string, uint32_t>> Badges = sponsor::Badges(W.L.Sponsors, T0 + Day);
	Expect(Badges.size() == 3 && Badges.front().first == "embercrest", "readers: the badges, the highest tier first");
	// The patch on the cap shows only on a cap.
	ss::wardrobe::Closet Cl;
	Cl.Patches[2] = "luckypenny";
	Expect(!ss::wardrobe::PatchFits(Cl, 2), "readers: no cap, no cap patch");
	Cl.Owned.insert("hat.beanie");
	Cl.Worn[static_cast<size_t>(ss::wardrobe::Slot::Hat)] = "hat.beanie";
	Expect(ss::wardrobe::PatchFits(Cl, 2) && ss::wardrobe::PatchFits(Cl, 0), "readers: a beanie takes it");
	Expect(sponsor::Fits(W.L.Sponsors, Cl, false, T0 + Day) == "Fifth St. Barbers on the sleeve at live events", "readers: what's missing, by name");
	Cl.Patches[1] = "barber";
	Expect(sponsor::Fits(W.L.Sponsors, Cl, false, T0 + Day).empty() && sponsor::Fits(W.L.Sponsors, Cl, true, T0 + Day) == "Team Embercrest on the chest in the room",
		"readers: the crest only in the room");
	life::LiveEntry E;
	E.Roster = {{"Mei", 0}};
	sponsor::NoteWorn(E, "grinder_3c", Cl, W.L.Sponsors, T0 + Day);
	Expect(E.Patches.size() == 2 && sponsor::WearingLine(Cl, W.L.Sponsors, T0 + Day) == "Wearing Fifth St. Barbers (sleeve) \xC2\xB7 Lucky Penny (cap)", "readers: what the player wore in, and the toast");
	// The card: what you get, what they ask, the fine print.
	const sponsor::Card K = sponsor::CardOf(*W.Of("embercrest"), W.L, W.Ch, T0 + Day);
	Expect(!K.Get.empty() && K.Asks.size() == 2 && !K.Fine.empty() && K.Deadline.find("Period ends") == 0, "readers: the contract card");
	Expect(std::find(K.Get.begin(), K.Get.end(), std::string("2 covered entries of $120 or less a month")) != K.Get.end(),
		"readers: the card says the covered entries in plain words");
}

// ------------------------------------------------------------------ 16. art

void Art()
{
	struct Flat : ss::ui::TextMeasurer
	{
		float Width(const std::string& T, ss::ui::Font, float Px) const override { return static_cast<float>(T.size()) * Px * 0.55f; }
		float Ascent(ss::ui::Font, float Px) const override { return Px * 0.8f; }
	} M;
	std::set<size_t> Sizes;
	bool Drew = true;
	for (const sponsor::Brand& B : sponsor::Brands())
	{
		ss::ui::DrawList L;
		ss::ui::Canvas C(L, M, 512.0f, 340.0f, 1.0f);
		ss::ui::sponsorart::Patch(C, B.Id);
		Drew = Drew && L.Vertices.size() > 100;
		Sizes.insert(L.Vertices.size() * 1000 + L.Cmds.size());
		ss::ui::DrawList Small;
		ss::ui::Canvas Cs(Small, M, 64.0f, 64.0f, 1.0f);
		ss::ui::sponsorart::Mark(Cs, B.Id, {26.0f, 26.0f, 12.0f, 12.0f});
		Drew = Drew && !Small.Vertices.empty();
	}
	Expect(Drew, "art: every brand's patch and its mark at 12 px draw");
	Expect(Sizes.size() >= 12, "art: the patches differ");
	ss::ui::DrawList Banner;
	ss::ui::Canvas Cb(Banner, M, 1024.0f, 1536.0f, 1.0f);
	ss::ui::sponsorart::AmbassadorBanner(Cb, "Jesse Cole", "Team Embercrest since Oct 30");
	ss::ui::DrawList Photo;
	ss::ui::Canvas Cp(Photo, M, 512.0f, 512.0f, 1.0f);
	ss::ui::sponsorart::Polaroid(Cp, ss::hero::Look(), "PENNY'S PICK");
	Expect(Banner.Vertices.size() > 100 && Photo.Vertices.size() > 500, "art: the ambassador banner and the Polaroid");
}

// ------------------------------------------------------------------ 17. update 2: the creator road (SPEC §4.7 item 5)

/** A record with a Rerun channel and nothing else: Subs subscribers, Views in the last 30 days, Partner or not. */
ss::rep::Public Creator(int Subs, int Views = 0, bool Partner = false)
{
	ss::rep::Public P;
	P.VideoSubs = Subs;
	P.Views30 = Views;
	P.VideoPartner = Partner;
	return P;
}

/** The offer letter Brand's look put in Mail (its body), "" for none. */
std::string OfferLetter(const life::State& L, const std::string& Brand)
{
	for (const life::Letter& M : L.Inbox)
	{
		if (M.Kind == sponsor::LetterOffer && M.Thread.rfind(Brand + "@", 0) == 0)
		{
			return M.Body;
		}
	}
	return std::string();
}

/** Whether a word stands alone in Text (no letter on either side), case-insensitive: "ai" in "said" doesn't count. */
bool SaysWord(const std::string& Text, const std::string& Word)
{
	const std::string Low = Lower(Text);
	for (size_t At = Low.find(Word); At != std::string::npos; At = Low.find(Word, At + 1))
	{
		const bool Before = At > 0 && std::isalpha(static_cast<unsigned char>(Low[At - 1]));
		const bool After = At + Word.size() < Low.size() && std::isalpha(static_cast<unsigned char>(Low[At + Word.size()]));
		if (!Before && !After)
		{
			return true;
		}
	}
	return false;
}

void CreatorRoad()
{
	const auto T = [](const ss::rep::Public& P) { return sponsor::Tier(P, "voltrush"); };
	// Each tier at its edges, the channel alone (no Kast, no Embercrest).
	Expect(T(Creator(999, 5000000, false)) == 0, "creator road: 999 subscribers is nobody yet, whatever the views");
	Expect(T(Creator(1000)) == 1, "creator road: 1,000 subscribers is known on the block");
	Expect(T(Creator(4999, 5000000)) == 1, "creator road: 4,999 subscribers is still tier 1, whatever the views");
	Expect(T(Creator(5000, 19999)) == 1, "creator road: 5,000 without 20,000 views in 30 days is still tier 1");
	Expect(T(Creator(5000, 20000)) == 2, "creator road: 5,000 and 20,000 views in 30 days is a face in the room");
	Expect(T(Creator(9999, 20000, true)) == 2, "creator road: Partner with 9,999 is still tier 2");
	Expect(T(Creator(9999, 0, true)) == 1, "creator road: Partner with 9,999 and a quiet month is tier 1");
	Expect(T(Creator(10000, 0, true)) == 3, "creator road: Rerun Partner with 10,000 is a name in the city");
	Expect(T(Creator(10000, 0, false)) == 1 && T(Creator(10000, 20000, false)) == 2, "creator road: 10,000 without Partner is tier 1 or 2, by the month's views");
	Expect(T(Creator(49999, 5000000, true)) == 3, "creator road: 49,999 is still tier 3, whatever the views");
	Expect(T(Creator(50000, 149999, true)) == 3, "creator road: 50,000 short of 150,000 views is still tier 3");
	Expect(T(Creator(50000, 150000, false)) == 4 && T(Creator(50000, 150000, true)) == 4, "creator road: 50,000 and 150,000 views in 30 days is on the circuit");
	Expect(T(Creator(249999, 5000000, true)) == 4, "creator road: 249,999 is still tier 4");
	Expect(T(Creator(250000)) == 5 && T(Creator(250000, 0, true)) == 5, "creator road: 250,000 subscribers is a poker personality, whatever the month");
	// More of anything never lowers the road.
	bool Monotone = true;
	const int Subs[] = {0, 999, 1000, 4999, 5000, 9999, 10000, 49999, 50000, 249999, 250000, 2000000};
	const int Views[] = {0, 19999, 20000, 149999, 150000, 50000000};
	for (int S = 0; S < 12; ++S)
	{
		for (int V = 0; V < 6; ++V)
		{
			for (int Pa = 0; Pa < 2; ++Pa)
			{
				const int Here = T(Creator(Subs[S], Views[V], Pa == 1));
				Monotone = Monotone && (S == 11 || T(Creator(Subs[S + 1], Views[V], Pa == 1)) >= Here) && (V == 5 || T(Creator(Subs[S], Views[V + 1], Pa == 1)) >= Here) &&
					(Pa == 1 || T(Creator(Subs[S], Views[V], true)) >= Here) && Here >= 0 && Here <= 5;
			}
		}
	}
	Expect(Monotone, "creator road: more subscribers, more views or Partner never lowers it");
	// Standing takes the best of the roads.
	ss::rep::Public Mixed = Streamer(600, 25.0); // the stream road's 2
	Mixed.VideoSubs = 1000;
	Expect(T(Mixed) == 2, "creator road: the best road counts (the stream's 2 over Rerun's 1)");
	Mixed.VideoSubs = 10000;
	Mixed.VideoPartner = true;
	Expect(T(Mixed) == 3, "creator road: the best road counts (Rerun's 3 over the stream's 2)");
	ss::rep::Public Regular;
	Regular.Titles = 1; // the live road's 3
	Regular.VideoSubs = 5000;
	Regular.Views30 = 20000;
	Expect(T(Regular) == 3, "creator road: the live road's title over Rerun's 2");
	Regular.VideoSubs = 50000;
	Regular.Views30 = 150000;
	Expect(T(Regular) == 4, "creator road: Rerun's 4 over the live road's 3");
	// The conduct limits hold on it.
	ss::rep::Public Famous = Creator(300000, 2000000, true);
	Famous.PickedUp90 = true;
	Expect(sponsor::Tier(Famous, "voltrush") == 1 && sponsor::Tier(Famous, "riverline") == 1, "creator road: a pickup holds every brand to tier 1, subscribers or not");
	Famous.PickedUp90 = false;
	Famous.Restricted180 = true;
	Expect(sponsor::Tier(Famous, "riverline") == 0 && sponsor::Tier(Famous, "rangewise") == 0 && sponsor::Tier(Famous, "voltrush") == 5,
		"creator road: a restriction shuts out sites and training, subscribers or not");
	// TierOf (Career's words, the renewals) takes it too, with Dee's road and the block road under it.
	{
		life::State L;
		sponsor::Book B;
		L.BackRoomNetCents = 20000;
		Expect(sponsor::TierOf(Creator(5000, 20000), B, L, "barber", 100.0 * Day) == 2 && sponsor::TierOf(Creator(0), B, L, "barber", 100.0 * Day) == 1,
			"creator road: TierOf takes the best of every road");
		Expect(std::string(sponsor::StandingWords(sponsor::TierOf(Creator(50000, 150000), B, L, std::string(), 100.0 * Day))) == "On the circuit",
			"creator road: Career's words for it (never the thresholds)");
	}
	// It opens standing, not Kast's own gates: brands that pay by the live hour and the read still want a stream.
	{
		life::State L;
		sponsor::Book B;
		const ss::rep::Public Big = Creator(300000, 2000000, true);
		Expect(!sponsor::Eligible(B, L, Big, "overclock", 100.0 * Day).empty() && !sponsor::Eligible(B, L, Big, "tunnelrat", 100.0 * Day).empty() &&
				   !sponsor::Eligible(B, L, Big, "sitwell", 100.0 * Day).empty() && !sponsor::Eligible(B, L, Big, "stacked", 100.0 * Day).empty(),
			"creator road: the overlay and apparel brands still want Kast's followers (or a title)");
		Expect(sponsor::Eligible(B, L, Big, "riverline", 100.0 * Day).empty(), "creator road: Team RiverLine opens on standing alone (on the circuit)");
		Expect(!sponsor::Eligible(B, L, Creator(10000, 0, true), "riverline", 100.0 * Day).empty(), "creator road: a name in the city isn't enough for Team RiverLine");
	}
	// Leverage and the raise read standing: a channel that grew since signing can ask.
	{
		World W;
		W.P = Creator(5000, 20000);
		const std::string Id = W.Signed("voltrush", 40.0 * Day);
		const sponsor::Contract* C = sponsor::Find(W.L.Sponsors, Id);
		Expect(C && C->TierAtSigning == 2, "creator road: the standing at signing is the channel's");
		Expect(sponsor::Leverage(W.L.Sponsors, Creator(250000), "voltrush") > sponsor::Leverage(W.L.Sponsors, Creator(5000, 20000), "voltrush"),
			"creator road: a bigger channel has more leverage in a counter");
		Expect(sponsor::AskForRaise(W.L.Sponsors, Creator(5000, 20000), Id, 41.0 * Day) == "Your numbers haven't moved enough to ask.", "creator road: no raise on the same channel");
		Expect(sponsor::AskForRaise(W.L.Sponsors, Creator(50000, 150000), Id, 41.0 * Day).empty(), "creator road: the channel grew two tiers since signing, so the raise is asked");
	}
	// Offers on the weekly look, and why the letter says they're calling.
	{
		World W;
		W.P = Creator(50000, 150000);
		W.Step(20.0 * Day, 41.0 * Day);
		const std::string Letter = OfferLetter(W.L, "riverline");
		Expect(Letter.rfind("Your Rerun videos pulled 150,000 views in the last month, and 50,000 people subscribe. ", 0) == 0,
			"creator road: Team RiverLine calls on a channel on the circuit, and says it's the channel");
		Expect(std::none_of(W.L.Sponsors.Contracts.begin(), W.L.Sponsors.Contracts.end(),
				   [](const sponsor::Contract& C) { return C.Brand != "riverline"; }),
			"creator road: nobody else calls (their gates want Kast, cans, cuts or the room)");
		World Partner;
		Partner.P = Creator(10000, 0, true);
		Partner.Step(20.0 * Day, 41.0 * Day);
		Expect(Partner.L.Sponsors.Contracts.empty(), "creator road: Rerun Partner with 10,000 opens no deal on its own (no gate of theirs is tier 3)");
	}
	{
		// Volt Rush: a face in the room and six cans; the letter's reason is the cans, true either way.
		World W;
		W.P = Creator(5000, 20000);
		W.L.Bought["volt-rush"] = 6;
		W.Step(20.0 * Day, 41.0 * Day);
		Expect(OfferLetter(W.L, "voltrush").rfind("You've bought 6 of our cans at the Lucky Penny. ", 0) == 0, "creator road: Volt Rush calls at Rerun's tier 2 with six cans bought");
		World Short;
		Short.P = Creator(5000, 19999);
		Short.L.Bought["volt-rush"] = 6;
		Short.Step(20.0 * Day, 41.0 * Day);
		Expect(OfferLetter(Short.L, "voltrush").empty(), "creator road: not a view short of tier 2");
	}
	{
		// Halo: a face in the room with a webcam, and the face is Rerun's, so the letter doesn't count Kast followers.
		World W;
		W.P = Creator(5000, 20000);
		ss::gear::Owned Desk;
		Desk["webcam-720"] = 0.0;
		sponsor::StepContext Ctx;
		Ctx.Gear = &Desk;
		sponsor::Step(W.L.Sponsors, W.L, W.Bank, W.Ch, W.P, 20.0 * Day, 41.0 * Day, "test-seed", Ctx);
		const std::string Letter = OfferLetter(W.L, "halo");
		Expect(Letter.rfind("Your Rerun videos pulled 20,000 views in the last month, and 5,000 people subscribe. ", 0) == 0,
			"creator road: Halo calls a creator with a webcam, and says it's the channel (not '0 followers')");
		World None;
		None.P = Creator(5000, 20000);
		sponsor::Step(None.L.Sponsors, None.L, None.Bank, None.Ch, None.P, 20.0 * Day, 41.0 * Day, "test-seed");
		Expect(OfferLetter(None.L, "halo").empty(), "creator road: no webcam, no Halo");
	}
	{
		// The barber makes his offer in person: known on the block by Rerun and five cuts.
		World W;
		W.P = Creator(1000);
		W.L.Groom.Cuts = 5;
		W.Step(20.0 * Day, 27.0 * Day); // a week: the shop looks once (an offer stands seven days)
		Expect(!sponsor::OpenOfferFor(W.L.Sponsors, "barber", 27.0 * Day).empty() && W.L.Inbox.empty(), "creator road: Lonnie's offer waits at the shop, known on the block by Rerun");
		World Nobody;
		Nobody.L.Groom.Cuts = 5;
		Nobody.Step(20.0 * Day, 27.0 * Day);
		Expect(Nobody.L.Sponsors.Contracts.empty(), "creator road: five cuts and nobody knows you: no offer");
	}
	{
		// A forced offer (ss.Sponsor.Offer) says the channel's own numbers when Rerun leads, the room's line when it doesn't.
		World W;
		W.P = Creator(250000);
		sponsor::MakeOffer(W.L.Sponsors, W.L, W.P, "turnstone", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(W.L, "turnstone").rfind("Your Rerun channel's up to 250,000 subscribers, and they keep watching. ", 0) == 0,
			"creator road: a forced offer to a big channel says the subscribers");
		World P2;
		P2.P = Creator(10000, 0, true);
		sponsor::MakeOffer(P2.L.Sponsors, P2.L, P2.P, "riverline", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(P2.L, "riverline").rfind("Rerun made you a Partner, and 10,000 people subscribe. ", 0) == 0, "creator road: and Partner when that's the tier");
		World Tie;
		Tie.P = Streamer(30000, 700.0); // the stream road's 5
		Tie.P.VideoSubs = 250000;       // Rerun's 5: a tie, so the stream's own line stands
		sponsor::MakeOffer(Tie.L.Sponsors, Tie.L, Tie.P, "turnstone", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(Tie.L, "turnstone").rfind("People in the room know your name now. ", 0) == 0, "creator road: a tie with another road keeps that road's line");
		World Nobody;
		sponsor::MakeOffer(Nobody.L.Sponsors, Nobody.L, Nobody.P, "turnstone", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(Nobody.L, "turnstone").rfind("People in the room know your name now. ", 0) == 0, "creator road: no channel, the old line");
		// Rerun leads only over both roads: the live road's title above Rerun's 2 keeps the room's line.
		World Live;
		Live.P = Creator(5000, 20000);
		Live.P.Titles = 1; // the live road's 3
		sponsor::MakeOffer(Live.L.Sponsors, Live.L, Live.P, "turnstone", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(Live.L, "turnstone").rfind("People in the room know your name now. ", 0) == 0, "creator road: the live road above Rerun keeps the room's line");
		// Tier 1 stands on subscribers alone, so its letter says them, never a month's views it needn't have.
		World Small;
		Small.P = Creator(1000, 300);
		sponsor::MakeOffer(Small.L.Sponsors, Small.L, Small.P, "turnstone", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(Small.L, "turnstone").rfind("Your Rerun channel's up to 1,000 subscribers, and they keep watching. ", 0) == 0,
			"creator road: tier 1 says the subscribers, not the month's views");
	}
	{
		// A brand's own gate met says that gate, even with Rerun bigger: Team RiverLine on Kast's Partner and its numbers,
		// Rangewise on a title. Rerun's words are only for a door Rerun opened.
		World Kast;
		Kast.P = Creator(50000, 150000); // Rerun's 4
		Kast.P.Followers = 3000;         // the stream road's 3, and Team RiverLine's stream gate
		Kast.P.Avg30 = 80.0;
		Kast.P.Partner = true;
		sponsor::MakeOffer(Kast.L.Sponsors, Kast.L, Kast.P, "riverline", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(Kast.L, "riverline").rfind("Kast made you a Partner. ", 0) == 0, "creator road: Team RiverLine's stream gate met, its letter says Kast's Partner");
		World Champ;
		Champ.P = Creator(250000); // Rerun's 5
		Champ.P.Titles = 1;
		Champ.P.LiveCashes = 5;
		Champ.Played(25.0 * Day, "", 1);
		sponsor::MakeOffer(Champ.L.Sponsors, Champ.L, Champ.P, "rangewise", 30.0 * Day, "test-seed", true);
		Expect(OfferLetter(Champ.L, "rangewise").rfind("You won the Embercrest Nightly $120, and people noticed. ", 0) == 0,
			"creator road: Rangewise's own gate (a title and five cashes) says the title");
	}
	// The words: every offer body with each Rerun reason in it, at a big channel's numbers, still passes the lint, and nothing
	// the book can write says a plan played (U1) or names a real platform.
	{
		std::map<std::string, std::string> Vars = sponsor::InventorySamples();
		Vars["Subs"] = "1,250,000";
		Vars["Views"] = "12,500,000";
		std::vector<std::pair<std::string, std::string>> Filled;
		int Reasons = 0;
		for (const std::pair<std::string, std::string>& Line : sponsor::Inventory(2))
		{
			if (Line.first.rfind("why:rerun", 0) != 0)
			{
				continue;
			}
			++Reasons;
			for (const std::pair<std::string, std::string>& Body : sponsor::Inventory(0))
			{
				if (Body.second.find("{Why}") != std::string::npos)
				{
					std::map<std::string, std::string> One = Vars;
					One["Why"] = copylint::Fill(Line.second, Vars);
					Filled.push_back({Body.first + "+" + Line.first, copylint::Fill(Body.second, One)});
				}
			}
		}
		Expect(Reasons == 3, "creator road: three Rerun reasons in the copy (subscribers, views, Partner)");
		Expect(copylint::Run("sponsor letters with Rerun's reasons", Filled, copylint::LetterBody, Vars) == 0 && Filled.size() >= 30,
			"creator road: every offer letter with a Rerun reason passes the copy lint at a big channel's numbers");
		std::string Said;
		for (int Kind = 0; Kind < 3; ++Kind)
		{
			for (const std::pair<std::string, std::string>& Line : sponsor::Inventory(Kind))
			{
				for (const char* Word : {"ai", "bot", "bots", "autoplay", "autoplayed", "autopilot", "youtube", "twitch", "tiktok", "kick", "rumble", "vimeo"})
				{
					Said = Said.empty() && SaysWord(Line.second, Word) ? Line.first + " says '" + Word + "'" : Said;
				}
			}
		}
		Expect(Said.empty(), ("creator road: no sponsor line says a plan played or names a real platform (U1, the trademark screen)" + (Said.empty() ? std::string() : ": " + Said)).c_str());
	}
}

// ------------------------------------------------------------------ 18. update 2: old books, no video (SPEC §4.7, U10)

/** FNV-1a, 64 bits: a digest of what a book wrote. */
uint64_t Digest(const std::string& S, uint64_t H = 1469598103934665603ull)
{
	for (const char Ch : S)
	{
		H = (H ^ static_cast<uint8_t>(Ch)) * 1099511628211ull;
	}
	return H;
}

/** One old save's book and mail stepped Days days from the save's minute with record P: everything the book wrote. */
std::string SteppedBook(const ss::SaveData& D, const ss::rep::Public& P, double Days)
{
	life::State L = D.Life;
	Chips Bank = D.BankrollCents;
	ss::kast::Channel Ch = D.Channel;
	std::vector<std::pair<std::string, std::string>> Texts;
	sponsor::StepContext Ctx;
	Ctx.Gear = &D.Gear;
	Ctx.Texts = &Texts;
	const double From = ss::net::MinutesPerDay * ss::net::NightOneDay + D.ClockMinutes;
	sponsor::Step(L.Sponsors, L, Bank, Ch, P, From, From + Days * Day, "old-books", Ctx);
	std::string Out;
	sponsor::WriteLines(L, Out);
	for (const life::Letter& M : L.Inbox)
	{
		Out += "mail|" + std::to_string(static_cast<long long>(std::llround(M.At * 100.0))) + "|" + M.Thread + "|" + M.Source + "|" + M.From + "|" + M.Subject + "|" + M.Body + "|" +
			std::to_string(M.Kind) + "\n";
	}
	for (const life::LedgerEntry& E : L.Ledger)
	{
		Out += "ledger|" + std::to_string(E.Kind) + "|" + E.Label + "|" + std::to_string(E.Amount) + "\n";
	}
	for (const std::pair<std::string, std::string>& T : Texts)
	{
		Out += "text|" + T.first + "|" + T.second + "\n";
	}
	return Out + "bank|" + std::to_string(Bank) + "\n";
}

/**
 * The creator road is a road with nothing on it in an old save: no channel, so rep::Public's video fields are 0, and every
 * old book behaves as it did. Five fixtures' books stepped 120 days under five records (nobody, a streamer, a Partner star,
 * a live regular, a restricted Partner), and once more with the video fields just short of the road's first tier (999
 * subscribers, 19,999 views in 30 days, even Partner): the same letters, the same money, the same lines. The digest is
 * pinned from fc70b71's build, before the creator road (sponsor_test printed it there).
 */
void OldBooks()
{
	const std::string Dir = std::string(SS_TESTS_DIR) + "/fixtures/";
	std::vector<ss::rep::Public> Records(5);
	Records[1] = Streamer(600, 25.0);
	Records[2] = Streamer(30000, 700.0);
	Records[2].Partner = true;
	Records[2].Titles = 1;
	Records[3].LiveEvents = 10;
	Records[3].Embercrest60 = 9;
	Records[3].Cashes60 = 3;
	Records[3].Cashes90 = 5;
	Records[3].BestEmbercrestPlace = 4;
	Records[3].Titles = 1;
	Records[4] = Streamer(3000, 80.0);
	Records[4].Partner = true;
	Records[4].Restricted180 = true;
	uint64_t All = Digest("");
	bool Same = true;
	int Books = 0;
	std::set<uint64_t> Distinct;
	size_t Letters = 0;
	for (const char* Name : {"pre_update2_sponsored_streamer.ssave", "pre_update2_veteran.ssave", "pre_update2_staked_mid_nightly.ssave", "pre_segment_team_riverline.ssave",
			 "pre_update2_fresh.ssave"})
	{
		ss::SaveData D;
		if (!ss::SaveData::Parse(ReadFile(Dir + Name), D))
		{
			Expect(false, "old books: the fixture parses");
			continue;
		}
		Expect(D.Life.Video.Videos.empty() && D.Life.Video.Subs == 0.0, "old books: an old save has no channel");
		uint64_t One = Digest("");
		for (const ss::rep::Public& P : Records)
		{
			const std::string Book = SteppedBook(D, P, 120.0);
			ss::rep::Public Short = P;
			Short.VideoSubs = 999;
			Short.Views30 = 19999;
			Short.VideoPartner = true;
			Same = Same && SteppedBook(D, Short, 120.0) == Book;
			One = Digest(Book, One);
			Distinct.insert(Digest(Book));
			for (size_t At = Book.find("\nmail|"); At != std::string::npos; At = Book.find("\nmail|", At + 1))
			{
				++Letters;
			}
			++Books;
		}
		std::printf("  old books: %s %016llx\n", Name, static_cast<unsigned long long>(One));
		All = Digest(std::to_string(One), All);
	}
	std::printf("  old books: %d books stepped 120 days (%zu different, %zu letters), digest %016llx\n", Books, Distinct.size(), Letters, static_cast<unsigned long long>(All));
	Expect(Books == 25 && Distinct.size() >= 15 && Letters >= 100, "old books: five fixtures under five records, and the records lead the books different ways");
	Expect(Same, "old books: a channel short of the creator road's first tier changes nothing");
	Expect(All == 0x4d1d7fe1ca2fae27ull, "old books: every old book behaves as it did before the creator road (the digest from fc70b71)");
}
} // namespace sponsor_test

int main()
{
	using namespace sponsor_test;
	Failures += copylint::SelfCheck();
	Catalog();
	Standing();
	Offers();
	Exclusivity();
	Reviews();
	Measuring();
	Strikes();
	Conduct();
	Counters();
	Renewals();
	LiveMoney();
	Migration();
	KastView();
	Exploits();
	Readers();
	Art();
	CreatorRoad();
	OldBooks();
	std::printf("sponsor tests: %d checks, %d failed\n", Checks, Failures);
	if (Failures == 0)
	{
		std::printf("sponsor tests: all passed\n");
	}
	return Failures == 0 ? 0 : 1;
}
