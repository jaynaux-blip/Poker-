// Northside's shops, wardrobe, fitness and pawn (the realism update's spec, §7.8's shops_test; core-shops).
//
// The contract first (every declaration of the six headers and the Session's Northside block links and runs), then
// §7.8's list: the catalogs; hours across midnight and the week; tax and tips; the ledger (every bankroll change is the
// ledger's); the DECLINED path; services run through Session::Update; 60 days of fitness; one membership renewal from
// the desk or the laptop; the barber's length rule; Saturday's yellow tags and the weekly racks; the closet; eviction;
// Ruby's coffee (bottomless, never sold twice), real meals and "sit a while"; Big Lou's check; Outfit 6 "None"; barefoot;
// the pawn (values, offers and loans, redemption in term and in grace, forfeits, renewals at what the ticket owes, gear
// effects, storage, the auction's lots and what Augie calls them, the cup, nothing round-tripping for a profit); every
// new save line round-tripping; the migration fixtures; the 14; and the 30-day pacing sim against rent.
#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/Weather.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"
#include "ShortStack/UI/Canvas.h"
#include "ShortStack/UI/RiverLine.h"

#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace shops_test
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

bool Near(double A, double B, double Eps = 1e-9)
{
	return std::fabs(A - B) < Eps;
}

struct Hooks : ss::SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Inbox;
	ss::SaveData Last;
	int Saves = 0;
	std::string Went;
	bool CanGo = false;
	void Text(const std::string& From, const std::string& Body) override { Inbox.emplace_back(From, Body); }
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
	}
	bool GoOut(const std::string& Id, ss::Chips /*Cents*/) override
	{
		Went = Id;
		return CanGo;
	}
	bool Got(const char* From, const char* Part) const
	{
		for (const auto& M : Inbox)
		{
			if (M.first == From && M.second.find(Part) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}
};

/** Rough text widths: enough for the pages to lay out (the goldens live in ui_test). */
struct Measure : ss::ui::TextMeasurer
{
	float Width(const std::string& Text, ss::ui::Font /*Face*/, float SizePx) const override { return static_cast<float>(Text.size()) * SizePx * 0.55f; }
	float Ascent(ss::ui::Font /*Face*/, float SizePx) const override { return SizePx * 0.8f; }
};

bool Drew(const ss::ui::DrawList& L, const std::string& Needle)
{
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.Text.find(Needle) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

std::vector<std::string> Tabs(const std::string& Line)
{
	std::vector<std::string> Out;
	std::string Cur;
	for (const char C : Line)
	{
		if (C == '\t')
		{
			Out.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur += C;
		}
	}
	Out.push_back(Cur);
	return Out;
}

ss::hero::Character Someone()
{
	ss::hero::Character C;
	C.FirstName = "Jesse";
	C.LastName = "Cole";
	C.Story = ss::hero::Background::Kitchen;
	C.Appearance.Outfit = 1; // the bomber
	C.Appearance.Hat = 2;
	C.Appearance.Glasses = 1;
	return C;
}

// ------------------------------------------------------------------ the clock

constexpr double Day = 1440.0;

/** World minutes at Day D (day 0 is Monday, October 5), Hour o'clock. */
double When(int D, double Hour)
{
	return static_cast<double>(D) * Day + Hour * 60.0;
}

/** A fresh session (before its first update) put at World: money in the bank, the rent paid well ahead, fed, rested. */
void Place(ss::Session& S, double World, ss::Chips Bank)
{
	S.LobbyMinutes = World - Day * static_cast<double>(ss::net::NightOneDay);
	S.BankrollCents = Bank;
	S.Life.RentStage = ss::life::Rent::Paid;
	S.Life.RentDueCents = ss::life::MonthlyRentCents;
	S.Life.RentsPaid = 1;
	S.Life.RentDeadline = World + 200.0 * Day;
	S.Life.Energy = 80.0;
	S.Life.Hunger = 20.0;
	S.Life.Thirst = 20.0;
}

/** The clock on to World, awake (the calendar catches up). */
void Jump(ss::Session& S, double& Now, double World)
{
	S.LobbyMinutes += World - S.WorldMinutes();
	Now += 0.05;
	S.Update(Now);
}

/** Plays the skip in progress out. */
void Finish(ss::Session& S, double& Now)
{
	for (int K = 0; K < 4000 && S.TimeSkip.Active; ++K)
	{
		Now += 0.05;
		S.Update(Now);
	}
}

ss::Session::ShopOrder Order(const std::string& Venue, const std::vector<std::string>& Offers, int Tip = 0, const std::vector<std::string>& Garments = {})
{
	ss::Session::ShopOrder O;
	O.Venue = Venue;
	O.Offers = Offers;
	O.Garments = Garments;
	O.TipPercent = Tip;
	return O;
}

/** What the ledger's lines add up to. */
ss::Chips LedgerSum(const ss::life::State& L)
{
	ss::Chips Sum = 0;
	for (const ss::life::LedgerEntry& E : L.Ledger)
	{
		Sum += E.Amount;
	}
	return Sum;
}

const ss::shop::Lot* FindLot(const std::vector<ss::shop::Lot>& Lots, const std::string& Id)
{
	for (const ss::shop::Lot& L : Lots)
	{
		if (L.Id == Id)
		{
			return &L;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------ the contract (Phase A)

/** Every declaration of the six headers and the Session's block, called once: they link, and nothing throws a fit. */
void Contracts()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-contracts");
	const double W = S.WorldMinutes();

	// Shop.h
	shop::Venue V;
	V.Id = "barber";
	V.Open.push_back(shop::Window());
	(void)shop::Venues();
	(void)shop::FindVenue("barber");
	(void)shop::IsOpen(V, W);
	(void)shop::IsStaffed(V, W);
	Expect(shop::NextOpen(V, W) >= W, "shop: NextOpen is now or later");
	(void)shop::ClosesAt(V, W);
	(void)shop::HoursLine(V, W);
	(void)shop::HoursSign(V);
	for (const shop::Venue& Each : shop::Venues())
	{
		Expect(shop::FindVenue(Each.Id) == &Each, "shop: every venue is found by its id");
		for (const shop::Offer& O : shop::Offers(Each.Id))
		{
			Expect(shop::FindOffer(O.Id) != nullptr && O.Venue == Each.Id, "shop: every offer is found, at its venue");
		}
	}
	Expect(shop::FindVenue("no-such-venue") == nullptr && shop::FindOffer("no/such-offer") == nullptr, "shop: unknown ids find nothing");
	(void)shop::GearValue(gear::Catalog().front());
	world::Award A;
	A.Event = "Main Event";
	(void)shop::AwardValue(A);
	(void)shop::KeepsakeOf(hero::Background::Kitchen);
	shop::Ticket T;
	T.Lot = "gear:monitor-24";
	(void)shop::RedeemCents(T, W);
	(void)shop::RenewCents(T);
	(void)shop::UsedStock(0, 7, ~0ull);
	shop::PawnBook B;
	shop::Consign(B, {"monitor-24"}, W, 7);
	std::string Lines;
	shop::WriteLines(S.Life, Lines);
	life::State Scratch;
	(void)shop::ReadLine(Tabs("visit\tstore\t1\t100.00\t299\t\t0\t0"), Scratch);
	SaveData D;
	shop::AfterParse(D);
	Expect(shop::PawnTermDays == 30.0 && shop::PawnGraceDays == 7.0 && shop::PawnShelfDays == 21.0, "shop: the pawn's terms");
	Expect(B.NextTicket >= 412, "shop: pawn tickets start at 412");

	// Wardrobe.h
	(void)wardrobe::Catalog();
	(void)wardrobe::Find("outer.bomber.charcoal");
	wardrobe::Closet C = wardrobe::SeedFrom(S.Person.Appearance);
	hero::Look L = S.Person.Appearance;
	wardrobe::ApplyTo(C, L);
	(void)wardrobe::StyleOf(C);
	(void)wardrobe::ThriftStock(0, 7, ~0ull);
	for (const wardrobe::Garment& G : wardrobe::Catalog())
	{
		Expect(wardrobe::Find(G.Id) == &G && wardrobe::ThriftPrice(G, W) >= 0, "wardrobe: every garment is found and priced");
	}
	Expect(wardrobe::SlotCount == 6, "wardrobe: six slots");

	// Body.h: an unfit non-member feels nothing.
	const body::Effects Fx = body::EffectsOf(body::Fitness(), false);
	Expect(Fx.DrainScale == 1.0 && Fx.RestBonus == 0.0 && Fx.ExhaustedAt == 20.0 && Fx.Calm == 0.0, "body: no fitness, no effect");
	(void)body::TableOf(body::Fitness(), W);
	body::Fitness F;
	(void)body::Gain(F, 3.0, 60.0, W);
	body::Decay(F, W);
	(void)body::HairRank(3);
	(void)body::BeardRank(2);
	Expect(body::HairAllowed(body::Grooming(), L, W) >= 0 && body::BeardAllowed(body::Grooming(), L, W) >= 0, "body: the barber can cut something");
	const double Sh = body::Sharp(body::Grooming(), W);
	Expect(Sh >= 0.0 && Sh <= 1.0, "body: Sharp is 0..1");

	// Town.h, Talk.h, Weather.h (people-core's; linked here).
	(void)town::People();
	(void)town::FindPerson("lonnie");
	(void)town::Who(W, S);
	town::Placement P;
	(void)town::Where("lonnie", W, S, P);
	(void)town::OnShift("store", W);
	(void)town::CrowdLevel(W, 0.0);
	(void)town::Signal(W);
	(void)town::BusTimes("market-east", W, 3);
	(void)town::BarberQueue(W);
	(void)town::Renown(S);
	std::string TownLines;
	town::WriteLines(S.Life.Town, S.Life.Said, net::DayOf(W), TownLines);
	(void)town::ReadLine(Tabs("said\tsal-rumour\t3"), Scratch.Town, Scratch.Said);
	if (const std::shared_ptr<const town::TalkContext> Ctx = town::ContextFor(S, "lonnie", "barber"))
	{
		(void)town::Open(*Ctx);
		(void)town::Answer(*Ctx, "bye");
		(void)town::KeeperLine(*Ctx, "greet");
		(void)town::Bark(*Ctx, "bump");
	}
	(void)weather::At(W, S.WorldSeed());
	Expect(std::string(weather::Word(weather::Sky::Rain)) == "Rain", "weather: the words");

	// The Session's block: nothing bogus moves money or the clock.
	const Chips Bank = S.BankrollCents;
	const size_t Ledger = S.Life.Ledger.size();
	Session::ShopOrder O;
	O.Venue = "no-such-venue";
	O.Offers.push_back("no/such-offer");
	(void)S.ShopOpenFor("barber");
	Expect(!S.ShopBlocked("no/such-offer").empty(), "session: an unknown offer can't be had");
	(void)S.ShopTotal(O);
	Expect(!S.ShopPay(O).empty() && !S.ShopService(O).empty() && !S.TimeSkip.Active, "session: an unknown order is refused");
	Expect(!S.WaitAt("no-such-venue", W + 60.0).empty() && !S.TimeSkip.Active, "session: no waiting anywhere unknown");
	Expect(!S.Wear("no.such.garment").empty() && !S.Donate("no.such.garment").empty(), "session: nobody wears or gives away what they don't own");
	(void)S.TakeOff(static_cast<int>(wardrobe::Slot::Hat));
	Expect(!S.TakeOff(99).empty(), "session: no such slot");
	Expect(!S.BoardBus("no-such-event").empty(), "session: the 14 needs an entry");
	Expect(S.Fitness() == S.Life.Fit.Level, "session: Fitness reads the life");
	Expect(S.Sharp() >= 0.0 && S.Sharp() <= 1.0, "session: Sharp is 0..1");
	(void)S.PawnLots();
	(void)S.PawnShelf();
	Expect(!S.PawnSell("no:such-lot").empty() && !S.PawnLoan("no:such-lot").empty() && !S.PawnBuy("no:such-lot").empty(), "session: Augie won't touch what isn't there");
	Expect(!S.PawnRedeem(1).empty() && !S.PawnRenew(1).empty(), "session: no such ticket");
	town::Talk Said;
	Said.Name = "Lonnie";
	S.Heard("lonnie", Said);
	(void)S.Lend("nobody", 0);
	S.Tipped("nobody", 0);
	Expect(S.BankrollCents == Bank && S.Life.Ledger.size() == Ledger, "session: the refusals cost nothing");
	Expect(S.WorldSeed() != 0, "session: a world seed");
	const kast::Inputs In = S.StreamInputs();
	Expect(In.Gear.Follow == S.GearFx().Follow && In.Gear.Tips == S.GearFx().Tips, "session: no cut, no outfit: the stream reads the gear alone");
}

/** The career's seed survives a save and load, and every line of the save reads back as it was written. */
void Saves()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-saves");
	S.NewCareer(Someone());
	S.Person.Appearance.Outfit = hero::OutfitNone;
	S.Life.Record(S.WorldMinutes(), "Fifth St. Barbers", -2800, 9);
	S.Life.Record(S.WorldMinutes(), "Northside Pawn & Loan (sold: 27\" monitor)", 6300, 9);
	S.Save();
	const std::string Text = H.Last.Serialize();
	SaveData D;
	Expect(SaveData::Parse(Text, D), "saves: parses");
	Expect(D.Serialize() == Text, "saves: a save reads back to the same text");
	Expect(D.Person.Appearance.Outfit == hero::OutfitNone, "saves: Outfit 6 \"None\" round-trips");
	Expect(D.Life.Ledger.size() >= 2 && D.Life.Ledger[0].Kind == 9 && D.Life.Ledger[1].Kind == 9, "saves: ledger kind 9 round-trips");
	Hooks H2;
	Session Back(H2, "shops-saves", &D);
	Expect(Back.WorldSeed() == S.WorldSeed(), "saves: the world seed is the career's");
	Hooks H3;
	Session Other(H3, "another-career");
	Expect(Other.WorldSeed() != S.WorldSeed(), "saves: another career, another seed");
}

/** Northside's own save keys (§7.3): what a save from before the shops never had. */
bool NorthsideLine(const std::string& Line)
{
	const std::string Key = Tabs(Line).front();
	return Key == "closet" || Key == "fit" || Key == "groom" || Key == "visit" || Key == "pawn" || Key == "meal" || Key == "town" || Key == "said";
}

/** A save's text without Northside's lines: the same career as a build from before the shops saved it. */
std::string WithoutNorthside(const std::string& Text)
{
	std::istringstream In(Text);
	std::string Old;
	std::string Line;
	while (std::getline(In, Line))
	{
		if (!NorthsideLine(Line))
		{
			Old += Line + "\n";
		}
	}
	return Old;
}

/**
 * The migration runs once: what a save from before the shops loads into is written, and reads back the same. And a save
 * this build writes reads back to the same text, whatever was never touched: a career that never met the creator (an
 * unseeded closet), or a new one that joined the gym on GearDrop before its first workout (fitness at its defaults must
 * not read as "no fit line" and earn the migration's 60).
 */
void Migration()
{
	using namespace ss;
	{
		Hooks H;
		Session S(H, "shops-old-save");
		S.NewCareer(Someone());
		S.BankrollCents = 100000;
		Expect(S.Buy("gym").empty(), "migration: a gym member");
		store::Basket B;
		B.Add("oodle-cup");
		Expect(S.Checkout(B).empty(), "migration: a store regular");
		S.Save();
		const std::string Old = WithoutNorthside(H.Last.Serialize());
		SaveData D;
		Expect(SaveData::Parse(Old, D), "migration: an old save loads");
		const std::string First = D.Serialize();
		bool Kept = true;
		std::istringstream OldIn(Old);
		std::string Line;
		while (std::getline(OldIn, Line))
		{
			Kept = Kept && First.find(Line + "\n") != std::string::npos;
		}
		Expect(Kept, "migration: every line of an old save is kept");
		SaveData Again;
		Expect(SaveData::Parse(First, Again) && Again.Serialize() == First, "migration: it runs once (the migrated save reads back the same)");
		Expect(Again.Gear.count("gym") > 0 && D.Life.Fit.Level == Again.Life.Fit.Level, "migration: an old member's fitness is settled once");
		// What it seeded: the creator's closet, 60 for the member, the store's visits from the ledger, a cut a week back.
		const double Now = Day * static_cast<double>(net::NightOneDay) + D.ClockMinutes;
		Expect(D.Life.Closet.Seeded && D.Life.Closet.Owned.count("outer.bomber.charcoal") > 0 && D.Life.Closet.Worn[1] == "outer.bomber.charcoal" &&
				   D.Life.Closet.Worn[4] == "hat.ballcap" && D.Life.Closet.Worn[5] == "eyes.round",
			"migration: the closet is the creator's look");
		Expect(D.Life.Fit.Level == 60.0 && Near(D.Life.Fit.LastCounted, Now, 0.02) && Near(D.Life.Fit.LastDecay, Now, 0.02) && D.Life.Fit.Workouts == 0,
			"migration: an existing member lands at 60, the fading three days off");
		body::Fitness Faded = D.Life.Fit;
		body::Decay(Faded, Now + 3.0 * Day - 1.0);
		const double Held = Faded.Level;
		body::Decay(Faded, Now + 5.0 * Day);
		Expect(Held == 60.0 && Near(Faded.Level, 58.0, 1e-6) && body::Gain(D.Life.Fit, 3.0, 60.0, Now + 1.0) > 0.0,
			"migration: it holds three days, then fades a point a day; the first workout counts at once");
		const auto Store = D.Life.Visits.find("store");
		Expect(Store != D.Life.Visits.end() && Store->second.Count == 1 && Store->second.SpentCents == B.Total() && Store->second.Usual == "oodle-cup",
			"migration: the store's visits come from the ledger, the usual from what was bought");
		Expect(Near(D.Life.Groom.CutAt, Now - 7.0 * Day, 0.02) && Near(D.Life.Groom.ShaveAt, Now - 7.0 * Day, 0.02), "migration: a cut a week back");
		Expect(D.Life.Groom.Cuts == 0 && body::Sharp(D.Life.Groom, Now) == 0.0, "migration: that cut wasn't Lonnie's: the stream sees no fresh cut");
		Expect(D.Life.Pawn.Tickets.empty() && D.Life.Pawn.Shelf.empty() && D.Life.Pawn.Away.empty() && D.Life.Pawn.NextTicket == 412, "migration: an empty pawn book");
		Hooks H2;
		Session Back(H2, "shops-old-save", &D);
		Expect(FindLot(Back.PawnLots(), "keepsake:kitchen") != nullptr, "migration: the keepsake is in the hero's pocket");
	}
	{
		Hooks H;
		Session S(H, "shops-unseeded");
		S.Save();
		SaveData D;
		Expect(SaveData::Parse(H.Last.Serialize(), D) && D.Serialize() == H.Last.Serialize(), "migration: a career that never met the creator round-trips");
		Expect(!D.Life.Closet.Seeded && D.Life.Closet.Owned.empty(), "migration: and has no closet to seed");
	}
	{
		Hooks H;
		Session S(H, "shops-new-member");
		S.NewCareer(Someone());
		S.BankrollCents = 100000;
		Expect(S.Buy("gym").empty(), "migration: a new member");
		const double Level = S.Fitness();
		S.Save();
		SaveData D;
		Expect(SaveData::Parse(H.Last.Serialize(), D) && D.Serialize() == H.Last.Serialize(), "migration: a new member's save round-trips");
		Hooks H2;
		Session Back(H2, "shops-new-member", &D);
		Expect(Back.Fitness() == Level, "migration: a new member's fitness isn't the old members' 60");
	}
}

/** The Lucky Penny's counter keeps the player's visits (the clerk reads them), not just the ledger. */
void StoreVisits()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-store");
	S.BankrollCents = 10000;
	store::Basket B;
	B.Add("cascade", 2);
	B.Add("oodle-cup");
	const Chips Total = B.Total();
	Expect(S.Checkout(B).empty(), "store: checkout");
	const auto It = S.Life.Visits.find("store");
	Expect(It != S.Life.Visits.end() && It->second.Count == 1 && It->second.SpentCents == Total && Near(It->second.LastAt, S.WorldMinutes()), "store: the visit is kept");
	Expect(It->second.Usual == "cascade", "store: the usual is what's bought most");
	Expect(!S.ClerkSays(store::Basket()).empty(), "store: the clerk still talks");
	Expect(S.Checkout(B).empty() && S.Life.Visits["store"].Count == 2 && S.Life.Visits["store"].SpentCents == 2 * Total, "store: and the next");
	// 02:07: Benny's shift, and he knows the face a little better (once a day).
	Expect(S.Life.Town.count("benny") > 0 && S.Life.Town["benny"].Familiarity == 1 && S.Life.Town["benny"].Visits == 1, "store: the night clerk knows the face, once a day");
	Expect(S.Life.Town["benny"].LookSeen == town::LookHash(S.Person.Appearance, S.Life.Closet), "store: and how the player looked");
}

/** §5.5: Ray on the register by day in his own voice, Benny by night; both notice a fresh cut and a new look. */
void ClerkRoster()
{
	using namespace ss;
	// The voices, straight from the context.
	store::ClerkContext Ray;
	Ray.Clerk = "ray";
	Ray.World = When(8, 11.0);
	Ray.Hunger = 30.0;
	Ray.Energy = 60.0;
	Expect(store::ClerkLine(Ray, store::Basket()) == "Morning. Benny leave the coffee on? Course he didn't.", "ray: his own morning");
	Ray.World = When(8, 19.0);
	Expect(store::ClerkLine(Ray, store::Basket()).find("Lotto") != std::string::npos, "ray: his evening");
	Ray.OnSchedule = true;
	Expect(store::ClerkLine(Ray, store::Basket()).find("my night cashier") != std::string::npos, "ray: the owner knows who works his nights (whoever the hero is)");
	Ray.NewLook = true;
	Expect(store::ClerkLine(Ray, store::Basket()).find("New jacket?") != std::string::npos, "ray: a new look comes before the schedule");
	Ray.FreshCut = true;
	Expect(store::ClerkLine(Ray, store::Basket()).find("Lonnie") != std::string::npos, "ray: and a fresh cut before that");
	Ray.Hunger = 80.0;
	Expect(store::ClerkLine(Ray, store::Basket()) == "Roller grill's on. Fresh, not Benny-fresh.", "ray: a hungry face first");
	store::Basket Two;
	Two.Add("volt-rush", 2);
	Expect(store::ClerkLine(Ray, Two).find("I sell 'em, I don't drink 'em") != std::string::npos, "ray: two energy drinks");
	store::ClerkContext Benny = Ray;
	Benny.Clerk = "benny";
	Benny.Hunger = 30.0;
	Expect(store::ClerkLine(Benny, store::Basket()).find("Fresh cut? Lonnie's.") == 0, "benny: notices the cut too, in his words");
	Benny.FreshCut = false;
	Expect(store::ClerkLine(Benny, store::Basket()).find("new look") != std::string::npos, "benny: and the new look");
	Benny.Clerk.clear();
	Benny.NewLook = false;
	Benny.World = When(8, 2.0);
	Expect(store::ClerkLine(Benny, store::Basket()).find("Don't tell Ray") != std::string::npos, "benny: no clerk named is Benny, as before");

	// Through the session: the roster picks the voice, and the look is noticed once, until the next checkout.
	Hooks H;
	Session S(H, "shops-roster");
	S.NewCareer(Someone());
	Place(S, When(8, 10.5), 10000);
	double Now = 0.0;
	S.Update(Now);
	auto Steady = [&S]() {
		S.Life.Hunger = 30.0;
		S.Life.Thirst = 30.0;
		S.Life.Energy = 60.0;
	};
	Steady();
	store::Basket Water;
	Water.Add("cascade");
	Expect(S.Checkout(Water).empty() && S.Life.Town.count("ray") > 0 && S.Life.Town.count("benny") == 0, "roster: half past ten is Ray's");
	Jump(S, Now, When(8, 11.75));
	Steady();
	Expect(S.ClerkSays(store::Basket()) == "Morning. Benny leave the coffee on? Course he didn't.", "roster: Ray talks like Ray");
	Expect(S.Wear("top.tee.white").empty() && S.ClerkSays(store::Basket()).find("New jacket?") != std::string::npos, "roster: a change of clothes is noticed");
	Expect(S.Checkout(Water).empty(), "roster: rung up");
	Jump(S, Now, When(8, 12.5));
	Steady();
	Expect(S.ClerkSays(store::Basket()) == "Back again? What'd you forget?", "roster: and only until he's rung it up");
	Jump(S, Now, When(8, 23.0));
	Steady();
	Expect(S.ClerkSays(store::Basket()) == "Evening. Lotto's up to forty million if you're feeling lucky.", "roster: eleven at night is Benny's, who never saw the old look");
	// A fresh cut is one of Lonnie's (Groom.Cuts, body::Sharp's rule), however recent a cut on record reads: the one a
	// career starts with, from before the city, is never remarked on at the counter.
	S.Life.Groom.CutAt = S.WorldMinutes() - Day;
	Expect(S.Life.Groom.Cuts == 0 && S.ClerkSays(store::Basket()).find("Lonnie") == std::string::npos, "roster: a cut that isn't Lonnie's isn't a fresh cut");
	S.Life.Groom.Cuts = 1;
	Expect(S.ClerkSays(store::Basket()).find("Fresh cut? Lonnie's.") == 0, "roster: one of Lonnie's from yesterday is");
}

/** A real meal (>= 40 hunger relief) quiets Mom's hungry text for a day; without one, she texts. */
void RealMeal()
{
	using namespace ss;
	for (int Ate = 0; Ate < 2; ++Ate)
	{
		Hooks H;
		Session S(H, "shops-meal");
		const double W = S.WorldMinutes();
		if (Ate)
		{
			S.Life.Hunger = 60.0;
			S.Life.Pantry["egg-salad"] = 1;
			Expect(S.Consume("egg-salad").empty() && Near(S.Life.RealMealAt, W), "meal: a Deli Wedge is a real meal");
		}
		else
		{
			S.Life.Pantry["hilltop-chips"] = 1;
			Expect(S.Consume("hilltop-chips").empty() && S.Life.RealMealAt < 0.0, "meal: chips aren't");
		}
		S.Life.Hunger = 74.0;
		S.Life.Thirst = 0.0;
		S.ResumeCalendarFrom(W - 60.0, false);
		S.Update(0.0);
		const bool Mom = H.Got("Mom", "eat");
		Expect(Ate ? !Mom : Mom, Ate ? "meal: Mom doesn't ask the day of a real meal" : "meal: Mom asks when there wasn't one");
	}
}

/** Ruby's coffee: the awake drain is 3 an hour while it lasts, not 4. */
void Coffee()
{
	using namespace ss;
	for (int Cup = 0; Cup < 2; ++Cup)
	{
		Hooks H;
		Session S(H, "shops-coffee");
		const double W = S.WorldMinutes();
		S.Life.Energy = 80.0;
		S.Life.Hunger = 0.0;
		S.Life.Thirst = 0.0;
		S.Life.CoffeeUntil = Cup ? W + 600.0 : 0.0;
		S.ResumeCalendarFrom(W - 60.0, false);
		S.Update(0.0);
		Expect(Near(S.Life.Energy, Cup ? 77.0 : 76.0), Cup ? "coffee: an awake hour costs 3 with coffee" : "coffee: an awake hour costs 4");
	}
}

/** A "shop:" skip isn't a night's sleep: on Dee's couch, a meal's energy still comes whole (Rest 1.0). */
void ShopSkip()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-skip");
	const double W = S.WorldMinutes();
	S.Life.Energy = 50.0;
	S.Life.Hunger = 0.0;
	S.Life.Thirst = 0.0;
	S.TimeSkip = Session::Skip();
	S.TimeSkip.Active = true;
	S.TimeSkip.From = W;
	S.TimeSkip.To = W + 30.0;
	S.TimeSkip.RealStart = 0.0;
	S.TimeSkip.RealSeconds = 0.5;
	S.TimeSkip.Rest = life::CouchRest;
	S.TimeSkip.Couch = true;
	S.TimeSkip.Label = "At Ruby's";
	S.TimeSkip.Result.ActivityId = "shop:diner:diner/short-stack";
	S.TimeSkip.Result.Start = W;
	S.TimeSkip.Result.End = W + 30.0;
	S.TimeSkip.Result.Energy = 6.0;
	double Now = 0.0;
	for (int K = 0; K < 40 && S.TimeSkip.Active; ++K)
	{
		Now += 0.1;
		S.Update(Now);
	}
	Expect(!S.TimeSkip.Active && S.HasOutcome, "skip: it ends with an outcome");
	Expect(Near(S.Life.Energy, 56.0) && Near(S.LastOutcome.Energy, 6.0), "skip: the meal's energy comes whole on the couch");
	Expect(Near(S.WorldMinutes(), W + 30.0), "skip: the clock moved by its minutes");
}

/** Ledger kind 9 is Northside in the Bank: its own name, and its own rows in and out. */
void LedgerNine()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-bank");
	const double W = S.WorldMinutes();
	S.Life.Record(W - 30.0, "Fifth St. Barbers", -2800, 9);
	S.Life.Record(W - 10.0, "Northside Pawn & Loan (sold: 27\" monitor)", 6300, 9);
	ui::RiverLine RL(S);
	Measure M;
	double Now = 0.0;
	RL.OpenApp(ui::RiverLine::App::Bank, Now);
	ui::DrawList Last;
	for (int K = 0; K < 40; ++K)
	{
		Now += 1.0 / 30.0;
		S.Update(Now);
		ui::DrawList L;
		ui::Canvas C(L, M, ui::RiverLine::Width, ui::RiverLine::Height, 1.0f);
		RL.Draw(C, Now);
		if (K == 39)
		{
			Last = L;
		}
	}
	Expect(Drew(Last, "Sales & refunds") && Drew(Last, "Looks, fitness & food out"), "bank: Northside's rows in and out");
	Expect(Drew(Last, "Northside"), "bank: kind 9 is named Northside in the history");
	Expect(Drew(Last, "$63.00") && Drew(Last, "$28.00"), "bank: the amounts");
	Expect(!Drew(Last, "Prizes"), "bank: kind 9 isn't a prize any more");
}

/** hero::Look's seventh Outfit, "None": appended, so older looks read unchanged. */
void OutfitNone()
{
	using namespace ss;
	Expect(hero::OutfitNone == 6 && hero::OptionCount(hero::Slot::Outfit) == 7, "hero: seven outfits");
	Expect(hero::OptionName(hero::Slot::Outfit, hero::OutfitNone) == "None" && hero::OptionName(hero::Slot::Outfit, 5) == "Track Jacket", "hero: the names");
	hero::Look L;
	L.Set(hero::Slot::Outfit, hero::OutfitNone);
	Expect(L.Outfit == hero::OutfitNone, "hero: None can be set");
	hero::Character C = Someone();
	C.Appearance.Outfit = hero::OutfitNone;
	hero::Character Back;
	std::istringstream In(C.Serialize());
	std::string Line;
	while (std::getline(In, Line))
	{
		Back.Read(Tabs(Line));
	}
	Expect(Back.Appearance == C.Appearance, "hero: None round-trips");
	hero::Character Old;
	Expect(Old.Read(Tabs("hero\tlook\tbody\t0\toutfit\t3\toutfitcolor\t2")) && Old.Appearance.Outfit == 3 && Old.Appearance.OutfitColor == 2 && Old.Appearance.Hair == hero::Look().Hair,
		"hero: an old look line reads unchanged");
	Rng R("shops-outfits");
	bool Jackets = true;
	for (int K = 0; K < 300; ++K)
	{
		Jackets = Jackets && hero::RandomLook(R, 30, K % 2).Outfit < hero::OutfitNone;
	}
	Expect(Jackets, "hero: a random look still wears a jacket");
}

// ------------------------------------------------------------------ the catalogs

void Catalogs()
{
	using namespace ss;
	std::set<std::string> Ids;
	for (const shop::Venue& V : shop::Venues())
	{
		Expect(Ids.insert(V.Id).second, "venues: ids are unique");
		Expect(!V.Name.empty() && !V.Address.empty() && !V.Open.empty() && V.Color != 0, "venues: every one has a name, an address, hours and a color");
		Expect(!shop::HoursSign(V).empty(), "venues: every one has a door decal");
	}
	for (const char* Id : {"store", "barber", "thrift", "gym", "diner", "pawn", "washfold"})
	{
		const shop::Venue* V = shop::FindVenue(Id);
		Expect(V && V->Enterable, "venues: §5.0's shops are enterable");
		bool Kept = false;
		for (double W = When(7, 0.0); V && W < When(14, 0.0); W += 30.0)
		{
			Kept = Kept || (shop::IsStaffed(*V, W) && !shop::KeeperOf(Id, W).empty());
		}
		Expect(Kept, "venues: every shop has a keeper while staffed");
	}
	Expect(shop::FindVenue("barber")->Name == "Fifth St. Barbers" && shop::FindVenue("thrift")->Name == "Second Chance Thrift" && shop::FindVenue("gym")->Name == "IronHouse Gym" &&
			   shop::FindVenue("diner")->Name == "Ruby's" && shop::FindVenue("pawn")->Name == "Northside Pawn & Loan",
		"venues: the names on the signs");
	Expect(shop::FindVenue("thrift")->Address == "220 Market St." && shop::FindVenue("pawn")->Address == "232 Market St." && shop::FindVenue("gym")->Address == "246 Market St." &&
			   shop::FindVenue("diner")->Address == "201 Market St." && shop::FindVenue("barber")->Address == "1818 Fifth St.",
		"venues: the addresses");
	Expect(shop::KeeperOf("store", When(8, 12.0)) == "ray" && shop::KeeperOf("store", When(8, 23.0)) == "benny", "keepers: Ray by day, Benny by night");
	Expect(shop::KeeperOf("diner", When(8, 7.0)) == "ruby" && shop::KeeperOf("diner", When(8, 20.0)) == "marlene", "keepers: Ruby by day, Marlene by night");
	Expect(shop::KeeperOf("gym", When(8, 13.0)).empty() && shop::KeeperOf("gym", When(8, 17.0)) == "rosa", "keepers: Rosa's split shift");
	Expect(shop::KeeperOf("barber", When(7, 12.0)).empty() && shop::KeeperOf("barber", When(8, 12.0)) == "lonnie", "keepers: Lonnie's off Mondays");
	Expect(shop::KeeperOf("pawn", When(8, 12.0)) == "augie" && shop::KeeperOf("thrift", When(8, 12.0)) == "bea" && shop::KeeperOf("kims", When(8, 12.0)).empty(),
		"keepers: Augie, Bea, and nobody for a display shop");

	// The offers.
	std::set<std::string> OfferIds;
	std::map<std::string, int> PerTab;
	for (const shop::Venue& V : shop::Venues())
	{
		for (const shop::Offer& O : shop::Offers(V.Id))
		{
			Expect(OfferIds.insert(O.Id).second, "offers: ids are unique");
			Expect(O.Venue == V.Id && shop::FindVenue(O.Venue) != nullptr, "offers: every offer's venue exists");
			Expect(!O.Name.empty() && !O.Blurb.empty() && !O.Tab.empty() && O.PriceCents >= 0, "offers: a name, a blurb, a tab, a price");
			const bool Goods = O.Type == shop::OfferKind::Meal || O.Type == shop::OfferKind::Drink || O.Type == shop::OfferKind::Shake;
			Expect(O.Taxed == Goods, "offers: goods and meals are taxed, services and passes aren't");
			if (O.LookSlot >= 0)
			{
				const int Count = hero::OptionCount(static_cast<hero::Slot>(O.LookSlot));
				Expect(O.LookValue >= -1 && O.LookValue < Count, "offers: every look a service sets is a real option");
			}
			++PerTab[V.Id + "/" + O.Tab];
		}
	}
	Expect(PerTab["barber/CUT"] == 13 && PerTab["barber/BEARD"] == 6 && PerTab["barber/COLOR"] == 8 && PerTab["barber/BROWS"] == 3, "offers: the Style Book's tabs");
	Expect(PerTab["gym/WORKOUTS"] == 4 && PerTab["gym/DESK"] == 3 && PerTab["gym/COOLER"] == 2, "offers: the gym's");
	Expect(PerTab["diner/MENU"] == 7 && PerTab["diner/DRINKS"] == 3, "offers: Ruby's menu");
	auto Offer = [](const char* Id) { return *ss::shop::FindOffer(Id); };
	Expect(Offer("barber/crew").PriceCents == 2200 && Offer("barber/crew").Minutes == 30 && Offer("barber/braids").PriceCents == 6500 && Offer("barber/braids").Minutes == 120 &&
			   Offer("barber/lineup").LookValue == -1 && Offer("barber/color-grey").PriceCents == 4500 && Offer("barber/brows-thin").PriceCents == 800,
		"offers: the barber's prices and minutes");
	Expect(Offer("gym/treadmill").Minutes == 30 && Offer("gym/treadmill").Energy == -10.0 && Offer("gym/treadmill").FitnessBase == 3.0 && Offer("gym/weights").Minutes == 45 &&
			   Offer("gym/bag").Minutes == 20 && Offer("gym/trainer").PriceCents == 4000 && Offer("gym/trainer").Minutes == 60 && Offer("gym/daypass").PriceCents == 1000 &&
			   Offer("gym/shake").PriceCents == 499,
		"offers: the gym's workouts and desk");
	Expect(Offer("gym/membership").PriceCents == gear::Find("gym")->PriceCents, "offers: the desk's membership is GearDrop's");
	Expect(Offer("gym/cascade").PriceCents == store::Find("cascade")->PriceCents && Offer("gym/volt-rush").PriceCents == store::Find("volt-rush")->PriceCents,
		"offers: the cooler at the Lucky Penny's prices");
	Expect(Offer("diner/short-stack").PriceCents == 699 && Offer("diner/steak-eggs").PriceCents == 1499 && Offer("diner/coffee").PriceCents == 225 && Offer("diner/patty-melt").Thirst == -10.0,
		"offers: Ruby's prices");
	const gear::Item& GymItem = *gear::Find("gym");
	Expect(GymItem.Calm == 0.10 && GymItem.Drain == 0.0 && GymItem.Blurb.find("246 Market St") != std::string::npos, "gear: the membership is the sauna now; fitness carries the rest");

	// The garments.
	std::set<std::string> GarmentIds;
	for (const wardrobe::Garment& G : wardrobe::Catalog())
	{
		Expect(GarmentIds.insert(G.Id).second, "garments: ids are unique");
		Expect(wardrobe::Find(G.Id) == &G && wardrobe::SlotOf(G.Kind) == G.Where, "garments: found, and worn where their cut goes");
		Expect(!G.Name.empty() && G.PriceCents > 0 && G.Style > 0.0 && G.Style <= 1.0 && wardrobe::TagOf(G) >= 0 && wardrobe::TagOf(G) <= 4, "garments: a name, a price, a style, a tag");
		Expect(wardrobe::Tone(G) != 0, "garments: a color");
		wardrobe::Closet C;
		C.Seeded = true;
		C.Owned.insert(G.Id);
		C.Worn[static_cast<size_t>(G.Where)] = G.Id;
		hero::Look L;
		wardrobe::ApplyTo(C, L);
		if (G.Where == wardrobe::Slot::Outer)
		{
			Expect(L.Outfit >= 0 && L.Outfit < hero::OutfitNone && L.Outfit == static_cast<int>(G.Kind) - 1 && L.OutfitColor == G.Color, "garments: every jacket is a Look Outfit");
		}
		if (G.Where == wardrobe::Slot::Hat)
		{
			Expect(L.Hat >= 1 && L.Hat <= 4, "garments: every hat is a Look Hat");
		}
		if (G.Where == wardrobe::Slot::Eyes)
		{
			Expect(L.Glasses >= 1 && L.Glasses <= 4, "garments: every frame is a Look Glasses");
		}
		if (G.Condition > 0)
		{
			const wardrobe::Garment* Base = wardrobe::Find(G.Id.substr(0, G.Id.find('~')));
			Expect(Base && Base->Kind == G.Kind && Base->Color == G.Color && Base->Style > G.Style && Base->PriceCents == G.PriceCents, "garments: a worn piece is the same garment, less sharp");
		}
		if (G.Tier == 0 && !G.Rare && G.Condition == 0)
		{
			const Chips P = G.PriceCents;
			bool InRange = true;
			switch (G.Kind)
			{
			case wardrobe::Cut::Tee: InRange = P >= 300 && P <= 600; break;
			case wardrobe::Cut::Jeans: InRange = P >= 1000 && P <= 1400; break;
			case wardrobe::Cut::Hoodie:
			case wardrobe::Cut::Flannel: InRange = P == 800; break;
			case wardrobe::Cut::Chinos: InRange = P == 900; break;
			case wardrobe::Cut::Track: InRange = P == 1200; break;
			case wardrobe::Cut::WorkPants: InRange = P == 1100; break;
			case wardrobe::Cut::Denim: InRange = P == 1600; break;
			case wardrobe::Cut::Shorts: InRange = P == 400; break;
			case wardrobe::Cut::Bomber: InRange = P == 1800; break;
			case wardrobe::Cut::Sneakers: InRange = P == 1200; break;
			case wardrobe::Cut::Leather: InRange = P >= 3800 && P <= 4500; break;
			default:
				if (G.Where == wardrobe::Slot::Hat)
				{
					InRange = P >= 300 && P <= 600;
				}
				else if (G.Where == wardrobe::Slot::Eyes)
				{
					InRange = P >= 500 && P <= 800;
				}
				break;
			}
			Expect(InRange, "garments: the thrift's prices are §5.2's");
		}
	}
	Expect(wardrobe::Find("outer.leather.rider")->PriceCents == 4500 && wardrobe::Find("top.graphic.sundaymajor")->PriceCents == 900 && wardrobe::Find("outer.track.embercrest")->PriceCents == 2200 &&
			   wardrobe::Find("outer.leather.rider")->Rare,
		"garments: the rare finds");
	Expect(shop::Curios().size() == 4 && wardrobe::Find("eyes.aviators")->Tier == 1 && wardrobe::Find("hat.trilby")->Tier == 1 && shop::FindCurio("aviators")->PriceCents == 1800 &&
			   shop::FindCurio("trilby")->PriceCents == 1400 && shop::FindCurio("luckyseven")->Bit == 1 && shop::FindCurio("watch")->Bit == 2,
		"curios: the case");
	for (const shop::Curio& C : shop::Curios())
	{
		Expect(C.Garment.empty() || (wardrobe::Find(C.Garment) && wardrobe::Find(C.Garment)->PriceCents == C.PriceCents), "curios: the wearable ones are the wardrobe's, at the case's price");
	}
}

// ------------------------------------------------------------------ hours

void Hours()
{
	using namespace ss;
	const std::string D = "\xE2\x80\x93";
	const std::string Dot = " \xC2\xB7 ";
	const shop::Venue& Barber = *shop::FindVenue("barber");
	// Day 7 is a Monday, 8 a Tuesday, 11 a Friday, 13 a Sunday.
	Expect(!shop::IsOpen(Barber, When(7, 15.0)) && shop::HoursLine(Barber, When(7, 15.0)) == "Opens Tue 10 AM", "hours: the barber's shut Mondays, and says when he opens");
	Expect(shop::HoursLine(Barber, When(8, 9.0)) == "Opens 10 AM", "hours: later today");
	Expect(shop::IsOpen(Barber, When(8, 15.0)) && shop::HoursLine(Barber, When(8, 15.0)) == "Open until 10 PM", "hours: open until ten");
	Expect(shop::IsOpen(Barber, When(11, 23.5)) && shop::HoursLine(Barber, When(11, 23.5)) == "Open until midnight" && shop::ClosesAt(Barber, When(11, 23.5)) == When(12, 0.0),
		"hours: Fridays to midnight");
	Expect(!shop::IsOpen(Barber, When(13, 0.5)) && shop::HoursLine(Barber, When(13, 0.5)) == "Opens 10 AM", "hours: shut after Saturday's midnight");
	Expect(shop::ClosesAt(Barber, When(7, 15.0)) == When(7, 15.0) && shop::NextOpen(Barber, When(7, 15.0)) == When(8, 10.0), "hours: shut: ClosesAt is now, NextOpen Tuesday");
	Expect(shop::HoursSign(Barber) == "TUE" + D + "SUN 10" + D + "10" + Dot + "FRI" + D + "SAT TIL MIDNIGHT" + Dot + "CLOSED MON", "hours: the barber's decal (§4.9)");

	const shop::Venue& Gym = *shop::FindVenue("gym");
	Expect(shop::HoursSign(Gym) == "MEMBERS ANY HOUR" + Dot + "STAFFED DAILY 6 AM" + D + "12 PM & 4" + D + "10 PM", "hours: the gym's decal");
	Expect(shop::IsOpen(Gym, When(8, 7.0)) && !shop::IsOpen(Gym, When(8, 13.0)) && shop::IsStaffed(Gym, When(8, 17.0)) && !shop::IsStaffed(Gym, When(8, 2.0)), "hours: the gym's staffed shifts");
	Expect(shop::HoursLine(Gym, When(8, 13.0)) == "Opens 4 PM" && shop::HoursLine(Gym, When(8, 7.0)) == "Open until noon", "hours: the gym's lines");

	const shop::Venue& Thrift = *shop::FindVenue("thrift");
	Expect(shop::HoursSign(Thrift) == "MON" + D + "SAT 10" + D + "9" + Dot + "SUN 12" + D + "6", "hours: the thrift's decal");
	Expect(shop::HoursLine(Thrift, When(13, 11.0)) == "Opens noon" && shop::IsOpen(Thrift, When(13, 13.0)) && !shop::IsOpen(Thrift, When(13, 18.5)), "hours: the thrift's Sunday");

	const shop::Venue& Pawn = *shop::FindVenue("pawn");
	Expect(shop::HoursSign(Pawn) == "MON" + D + "SAT 9" + D + "8" + Dot + "FRI" + D + "SAT TIL 9" + Dot + "SUN 11" + D + "5", "hours: the pawn's decal");
	Expect(shop::IsOpen(Pawn, When(11, 20.5)) && !shop::IsOpen(Pawn, When(10, 20.5)) && shop::IsOpen(Pawn, When(13, 16.5)) && !shop::IsOpen(Pawn, When(13, 10.5)), "hours: the pawn's week");

	const shop::Venue& Diner = *shop::FindVenue("diner");
	Expect(shop::HoursSign(Diner) == "OPEN 24 HOURS" && shop::HoursLine(Diner, When(8, 3.0)) == "Open 24 hours" && shop::ClosesAt(Diner, When(8, 3.0)) == When(8, 3.0) + 7.0 * Day,
		"hours: Ruby's never closes");
	const shop::Venue& Dragon = *shop::FindVenue("golden-dragon");
	Expect(shop::IsOpen(Dragon, When(8, 1.0)) && shop::HoursLine(Dragon, When(8, 1.0)) == "Open until 2 AM" && !shop::IsOpen(Dragon, When(8, 3.0)), "hours: across midnight");
	Expect(shop::HoursSign(Dragon) == "DAILY 11 AM" + D + "2 AM", "hours: a decal across midnight");
	Expect(shop::HoursSign(*shop::FindVenue("liquor")) == "DAILY 10 AM" + D + "MIDNIGHT", "hours: to midnight");
	Expect(shop::HoursSign(*shop::FindVenue("dental")) == "MON" + D + "FRI 9" + D + "5" + Dot + "CLOSED SAT" + D + "SUN", "hours: weekdays");
	Expect(shop::HoursSign(*shop::FindVenue("library")) == "MON" + D + "SAT 10" + D + "8" + Dot + "FRI" + D + "SAT TIL 6" + Dot + "CLOSED SUN", "hours: the library");

	// Every venue, every 20 minutes for two weeks: NextOpen opens it, ClosesAt shuts it (or it never shuts).
	bool Consistent = true;
	for (const shop::Venue& V : shop::Venues())
	{
		for (double W = When(7, 0.0); W < When(21, 0.0); W += 20.0)
		{
			const double Opens = shop::NextOpen(V, W);
			Consistent = Consistent && Opens >= W && shop::IsOpen(V, Opens) && (shop::IsOpen(V, W) == (Opens == W));
			if (shop::IsOpen(V, W))
			{
				const double Shuts = shop::ClosesAt(V, W);
				Consistent = Consistent && Shuts > W && (Shuts >= W + 7.0 * Day || !shop::IsOpen(V, Shuts));
			}
		}
	}
	Expect(Consistent, "hours: NextOpen and ClosesAt agree with IsOpen for every venue");
}

// ------------------------------------------------------------------ tax, tips, the receipt

void TaxAndTips()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-tax");
	Place(S, When(8, 15.0), 100000);
	// Cut and beard: $22 + $18 - $5, untaxed; 20% on top.
	Expect(S.ShopTotal(Order("barber", {"barber/crew", "barber/beard-short"}, 20)) == 4200, "tax: a cut and a beard, $5 off, 20% tip, no tax");
	Expect(S.ShopTotal(Order("barber", {"barber/crew"}, 0)) == 2200, "tax: no tip, no tax");
	// A patty melt: $10.99 + 7.25% tax ($0.80) + 18% of $10.99 ($1.98).
	Expect(S.ShopTotal(Order("diner", {"diner/patty-melt"}, 18)) == 1099 + 80 + 198, "tax: Ruby's, taxed and tipped on the food");
	Expect(S.ShopTotal(Order("gym", {"gym/shake"}, 20)) == 499 + store::Tax(499), "tax: the shake's taxed; no tip at the desk");
	Expect(S.ShopTotal(Order("gym", {"gym/daypass"})) == 1000 && S.ShopTotal(Order("gym", {"gym/membership"})) == gear::Find("gym")->PriceCents, "tax: passes and the membership aren't");
	const wardrobe::Garment& Bomber = *wardrobe::Find("outer.bomber.navy~worn");
	Expect(S.ShopTotal(Order("thrift", {}, 0, {Bomber.Id})) == wardrobe::ThriftPrice(Bomber, S.WorldMinutes()) + store::Tax(wardrobe::ThriftPrice(Bomber, S.WorldMinutes())) &&
			   wardrobe::ThriftPrice(Bomber, S.WorldMinutes()) == 1530,
		"tax: a worn bomber, 15% off, taxed");
	Expect(S.ShopTotal(Order("barber", {"barber/crew", "barber/buzz"})) == 0, "tax: a bad order has no total");
}

/** A charge over the bankroll is declined: nothing moves, nothing starts. */
void DeclinedPath()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-declined");
	Place(S, When(8, 17.0), 500);
	S.Person.Appearance.Hair = 2;
	const size_t Lines = S.Life.Ledger.size();
	Expect(S.ShopService(Order("barber", {"barber/crew"}, 20)) == "Card declined." && !S.TimeSkip.Active && S.BankrollCents == 500 && S.Life.Ledger.size() == Lines,
		"declined: a cut over the bankroll");
	Expect(S.ShopPay(Order("gym", {"gym/membership"})) == "Card declined." && !S.Owns("gym"), "declined: the membership");
	Expect(S.Life.Visits.count("barber") == 0, "declined: no visit counted");
}

// ------------------------------------------------------------------ services through the clock

void Services()
{
	using namespace ss;
	{
		// The barber: a crew cut on a buzz three weeks grown, through Session::Update.
		Hooks H;
		Session S(H, "shops-barber");
		const double Start = When(8, 15.0);
		Place(S, Start, 100000);
		S.Person.Appearance.Hair = 1;
		S.Life.Groom.CutAt = Start - 22.0 * Day;
		double Now = 0.0;
		S.Update(Now);
		const double W = S.WorldMinutes();
		const double HungerWas = S.Life.Hunger;
		Expect(S.ShopService(Order("barber", {"barber/crew"}, 20)).empty() && S.TimeSkip.Active && S.TimeSkip.Result.ActivityId == "shop:barber:barber/crew" && S.TimeSkip.Rest == 1.0 &&
				   S.TimeSkip.RealSeconds == 8.0,
			"barber: the cut starts as a skip");
		Expect(S.BankrollCents == 100000 - 2640 && S.Life.Ledger.front().Label == "Fifth St. Barbers \xC2\xB7 Crew Cut" && S.Life.Ledger.front().Kind == 9, "barber: paid, tip and all, as ledger 9");
		Finish(S, Now);
		Expect(!S.TimeSkip.Active && S.HasOutcome && Near(S.WorldMinutes(), W + 30.0, 0.02), "barber: the clock moved by its 30 minutes");
		Expect(S.Person.Appearance.Hair == 2 && Near(S.Life.Groom.CutAt, W + 30.0, 0.02) && S.Life.Groom.Cuts == 1 && S.Sharp() == 1.0, "barber: the look changed, and it's sharp");
		Expect(S.Life.Hunger > HungerWas && S.LastOutcome.Title == "Crew Cut" && S.LastOutcome.Body.find("Lonnie's chair") != std::string::npos, "barber: the needs moved; the card says so");
		Expect(S.Life.Visits["barber"].Count == 1 && S.Life.Visits["barber"].Usual == "barber/crew" && S.Life.Visits["barber"].Tips == 1, "barber: the visit, the usual, the tip");
		const kast::Inputs In = S.StreamInputs();
		Expect(In.Gear.Follow > S.GearFx().Follow, "barber: a fresh cut draws follows on stream");
		// An hour of color: the receipt and the card say what it was ("Auburn" alone is only a name under the COLOR tab).
		Expect(S.ShopService(Order("barber", {"barber/color-auburn"})).empty() && S.Life.Ledger.front().Label == "Fifth St. Barbers \xC2\xB7 Auburn color", "barber: the color's receipt");
		Finish(S, Now);
		Expect(S.Person.Appearance.HairColor == 3 && S.LastOutcome.Title == "Auburn color" && S.LastOutcome.Body.rfind("An hour in Lonnie's chair.", 0) == 0,
			"barber: an hour of color, and the card starts with a capital");
	}
	{
		// The gym: a member on the treadmill at 2 AM (the fob), then the bag the next afternoon.
		Hooks H;
		Session S(H, "shops-gym");
		Place(S, When(8, 2.0), 100000);
		double Now = 0.0;
		S.Update(Now);
		Expect(S.Buy("gym").empty(), "gym: a member");
		const double EnergyWas = S.Life.Energy;
		const double ThirstWas = S.Life.Thirst;
		Expect(S.ShopOpenFor("gym") && S.ShopService(Order("gym", {"gym/treadmill"})).empty(), "gym: a member works out at 2 AM");
		Finish(S, Now);
		Expect(Near(S.Fitness(), 3.0) && S.Life.Fit.Workouts == 1 && Near(S.Life.Energy, EnergyWas - 10.0, 0.01) && S.Life.Thirst >= ThirstWas + 12.0, "gym: fitness 3, energy -10, thirsty");
		Expect(S.GearFx().Calm > gear::Sum(S.Gear).Calm, "gym: fitness calms tilt");
		Expect(S.ShopService(Order("gym", {"gym/weights"})).empty(), "gym: another, right away");
		Finish(S, Now);
		Expect(Near(S.Fitness(), 3.0) && S.LastOutcome.Body.find("16 hours") != std::string::npos, "gym: a second inside 16 hours counts for nothing");
		Jump(S, Now, When(8, 19.0));
		S.Life.Energy = 80.0;
		S.Life.Hunger = 20.0;
		S.Life.Thirst = 20.0;
		Expect(S.ShopService(Order("gym", {"gym/bag"})).empty(), "gym: the heavy bag");
		Finish(S, Now);
		Expect(S.Fitness() > 3.0 && body::TableOf(S.Life.Fit, S.WorldMinutes()).TiltGainScale == 0.85 && body::TableOf(S.Life.Fit, S.WorldMinutes() + 9.0 * 60.0).TiltGainScale == 1.0,
			"gym: the bag clears the head for eight hours");
	}
	{
		// Ruby's: a patty melt, its relief, its energy, a real meal; Mom waits a day.
		Hooks H;
		Session S(H, "shops-diner");
		Place(S, When(8, 12.0), 100000);
		S.Life.Hunger = 60.0;
		S.Life.Thirst = 20.0;
		S.Life.Energy = 40.0;
		double Now = 0.0;
		S.Update(Now);
		const double W = S.WorldMinutes();
		Expect(S.ShopService(Order("diner", {"diner/patty-melt", "diner/coffee"}, 18)).empty() && S.TimeSkip.RealSeconds == 4.0, "diner: lunch");
		Finish(S, Now);
		Expect(Near(S.WorldMinutes(), W + 35.0, 0.02) && Near(S.Life.Energy, 40.0 + 8.0 + 8.0, 0.01), "diner: 35 minutes; the plate's and the cup's energy");
		Expect(S.Life.Hunger < 10.0 && Near(S.Life.RealMealAt, W + 35.0, 0.02) && S.Life.CoffeeUntil > S.WorldMinutes(), "diner: fed, a real meal, coffee in the system");
		Expect(S.LastOutcome.Body.find("A real meal") != std::string::npos, "diner: the card");
	}
}

// ------------------------------------------------------------------ fitness

void FitnessDays()
{
	using namespace ss;
	// The formulas.
	body::Fitness F;
	F.Level = 60.0;
	const body::Effects E = body::EffectsOf(F, true);
	Expect(Near(E.DrainScale, 0.91) && Near(E.RestBonus, 0.06) && E.ExhaustedAt == 15.0 && Near(E.Calm, 0.12), "fitness: at 60");
	F.Level = 100.0;
	F.Sessions = 3;
	const body::TableEffects T = body::TableOf(F, 0.0);
	Expect(Near(T.RestBpm, 60.0) && Near(T.SettleScale, 1.5) && Near(T.FocusRefill, 1.0 / 11.0) && T.SteadyBpm == 20.0 && T.FocusCostScale == 0.75, "fitness: at 100, with Box Breathing");
	body::Fitness Fresh;
	Expect(Near(body::Gain(Fresh, 3.0, 60.0, 0.0), 3.0) && Near(body::Gain(Fresh, 3.0, 20.0, 0.0), 1.5), "fitness: a gain, halved when spent");
	Fresh.LastCounted = 0.0;
	Expect(body::Gain(Fresh, 3.0, 60.0, 15.0 * 60.0) > 0.0, "fitness: the first workout counts whenever it comes");
	Fresh.Workouts = 1;
	Expect(body::Gain(Fresh, 3.0, 60.0, 15.0 * 60.0) == 0.0 && body::Gain(Fresh, 3.0, 60.0, 16.0 * 60.0) > 0.0, "fitness: then one counted workout every 16 hours");

	// Sixty days: three workouts a week for thirty, then nothing for thirty.
	body::Fitness G;
	double Peak = 0.0;
	for (int D = 0; D < 60; ++D)
	{
		const double W = When(D, 17.0);
		body::Decay(G, W);
		const int Wd = D % 7;
		if (D < 30 && (Wd == 0 || Wd == 2 || Wd == 4))
		{
			const double Gain = body::Gain(G, 4.0, 60.0, W);
			G.Level += Gain;
			G.LastCounted = W;
			G.LastDecay = W;
		}
		if (D == 29)
		{
			Peak = G.Level;
		}
	}
	body::Decay(G, When(60, 17.0));
	Expect(Peak > 40.0 && Peak < 60.0, "fitness: a month of three a week builds a real level");
	// The last workout was on day 28 (a Monday): fading from day 31, a point a day.
	Expect(Near(G.Level, std::max(0.0, Peak - (60.0 - 31.0)), 0.01), "fitness: then fades a point a day after three idle days");

	// Through the session: the effects follow the level, in the drain and in sleep.
	Hooks H;
	Session S(H, "shops-fit");
	Place(S, When(8, 9.0), 100000);
	double Now = 0.0;
	S.Update(Now);
	const double Drain = S.GearFx().Drain;
	const double Rest = S.RestFactor();
	S.Life.Fit.Level = 80.0;
	S.Life.Fit.LastCounted = S.WorldMinutes();
	Jump(S, Now, S.WorldMinutes() + 1.0);
	Expect(S.GearFx().Drain > Drain && S.RestFactor() > Rest, "fitness: slows the awake drain and deepens sleep");
	Expect(S.MaxTables() >= 2, "fitness: the tables still open");
}

/** One subscription, renewed every thirty days, whether bought at IronHouse's desk or on GearDrop. */
void Membership()
{
	using namespace ss;
	for (int Desk = 0; Desk < 2; ++Desk)
	{
		Hooks H;
		Session S(H, Desk ? "shops-desk" : "shops-laptop");
		Place(S, When(8, 7.0), 20000);
		double Now = 0.0;
		S.Update(Now);
		const double W = S.WorldMinutes();
		Expect((Desk ? S.ShopPay(Order("gym", {"gym/membership"})) : S.Buy("gym")).empty() && S.Owns("gym") && Near(S.Gear["gym"], W + 30.0 * Day, 0.02),
			Desk ? "membership: bought at the desk" : "membership: bought on the laptop");
		if (Desk)
		{
			Expect(S.Life.Ledger.front().Kind == 9 && S.Life.Ledger.front().Label == "IronHouse Gym \xC2\xB7 Membership", "membership: the desk's receipt");
			Expect(S.ShopBlocked("gym/membership") == "You're already a member.", "membership: once");
		}
		const Chips Bank = S.BankrollCents;
		Jump(S, Now, W + 30.0 * Day + 60.0);
		int Renewals = 0;
		for (const life::LedgerEntry& E : S.Life.Ledger)
		{
			Renewals += E.Label == "GearDrop: Gym membership (renewal)" ? 1 : 0;
		}
		Expect(Renewals == 1 && S.BankrollCents == Bank - gear::Find("gym")->PriceCents && S.Owns("gym"), "membership: one renewal at thirty days");
	}
}

/** The barber can't cut longer than it's grown: a rank every 21 days (beards every 5), always shorter. */
void BarberLength()
{
	using namespace ss;
	hero::Look L;
	L.Hair = 1;       // a buzz
	L.FacialHair = 0; // clean
	body::Grooming G;
	G.CutAt = 1000.0;
	G.ShaveAt = 1000.0;
	G.Cuts = 1;
	Expect(body::HairAllowed(G, L, 1000.0) == 1 && body::HairAllowed(G, L, 1000.0 + 21.0 * Day) == 2 && body::HairAllowed(G, L, 1000.0 + 42.0 * Day) == 3 &&
			   body::HairAllowed(G, L, 1000.0 + 400.0 * Day) == 6,
		"barber: a rank of length every three weeks, to 6");
	Expect(body::BeardAllowed(G, L, 1000.0 + 4.0 * Day) == 0 && body::BeardAllowed(G, L, 1000.0 + 5.0 * Day) == 1 && body::BeardAllowed(G, L, 1000.0 + 20.0 * Day) == 4,
		"barber: a rank of beard every five days, to 4");
	Expect(body::HairAllowed(body::Grooming(), L, 1.0e6) == 1 && body::Sharp(body::Grooming(), 1.0e6) == 0.0, "barber: never in the chair: no growth on record yet");
	Expect(Near(body::Sharp(G, 1000.0 + 60.0), 1.0) && Near(body::Sharp(G, 1000.0 + 7.0 * Day), 2.0 / 3.0) && body::Sharp(G, 1000.0 + 21.0 * Day) == 0.0, "barber: sharp, fading over 21 days");
	body::Grooming Before = G;
	Before.Cuts = 0;
	Expect(body::Sharp(Before, 1000.0 + 60.0) == 0.0 && body::HairAllowed(Before, L, 1000.0 + 21.0 * Day) == 2, "barber: the cut a career starts with grows, but isn't Lonnie's: not sharp");

	{
		// A new career: the hair was cut a week before night one, so it's grown out a rank two weeks in, chair or no chair.
		Hooks Hn;
		Session N(Hn, "shops-length-new");
		hero::Character Who = Someone();
		Who.Appearance.Hair = 1;
		Who.Appearance.FacialHair = 0;
		N.NewCareer(Who);
		const double Start = N.WorldMinutes();
		Expect(Near(N.Life.Groom.CutAt, Start - 7.0 * Day, 0.02) && Near(N.Life.Groom.ShaveAt, Start - 7.0 * Day, 0.02) && N.Life.Groom.Cuts == 0 && N.Sharp() == 0.0,
			"new career: cut a week before night one, not sharp");
		Expect(body::HairAllowed(N.Life.Groom, N.Person.Appearance, Start) == 1 && body::HairAllowed(N.Life.Groom, N.Person.Appearance, Start + 14.0 * Day) == 2 &&
				   body::BeardAllowed(N.Life.Groom, N.Person.Appearance, Start) == 1,
			"new career: the hair and the beard grow from the start");
		std::string Saved;
		shop::WriteLines(N.Life, Saved);
		Expect(Saved.find("groom\t" + Fixed(Start - 7.0 * Day, 2) + "\t") != std::string::npos, "new career: the week-old cut is saved");
	}

	Hooks H;
	Session S(H, "shops-length");
	const double W = When(8, 12.0);
	Place(S, W, 100000);
	S.Person.Appearance.Hair = 1;
	S.Person.Appearance.FacialHair = 0;
	S.Life.Groom.CutAt = W - 2.0 * Day;
	S.Life.Groom.ShaveAt = W - 2.0 * Day;
	Expect(S.ShopBlocked("barber/crew") == "Give it three weeks." && S.ShopBlocked("barber/shaved").empty() && S.ShopBlocked("barber/lineup").empty() && S.ShopBlocked("barber/buzz").empty(),
		"barber: shorter, or the same, but not longer");
	Expect(S.ShopBlocked("barber/stubble") == "Give it a few days." && S.ShopBlocked("barber/shave").empty(), "barber: a beard has to grow too");
	S.Life.Groom.CutAt = W - 85.0 * Day;
	Expect(S.ShopBlocked("barber/braids").empty() && S.ShopBlocked("barber/shoulder") == "Give it three weeks.", "barber: braids want an afro's length");
	Expect(S.ShopBlocked("barber/color-grey").empty(), "barber: color any time");
	S.Person.Appearance.Brows = 2;
	Expect(S.ShopBlocked("barber/brows-natural").empty() && S.ShopBlocked("barber/brows-thin").empty(), "barber: brows thinner");
	S.Person.Appearance.Brows = 0;
	Expect(S.ShopBlocked("barber/brows-natural") == "Can't put brows back on." && S.ShopBlocked("barber/brows-thin") == "That's how they are already.", "barber: but not back");
	Expect(S.ShopTotal(Order("barber", {"barber/crew", "barber/buzz"})) == 0 && S.ShopService(Order("barber", {"barber/crew", "barber/buzz"})) == "One cut at a time.",
		"barber: one cut to a sitting");
	Place(S, When(8, 21.9), 100000);
	Expect(S.ShopBlocked("barber/lineup") == "Not enough time before closing.", "barber: nothing starts that ends after closing");
	Place(S, When(7, 12.0), 100000);
	Expect(S.ShopBlocked("barber/lineup").rfind("Closed", 0) == 0, "barber: Mondays");
}

// ------------------------------------------------------------------ the thrift

void Thrift()
{
	using namespace ss;
	// Yellow tags: half off on Saturdays (day 12 is a Saturday, 11 a Friday).
	const wardrobe::Garment* Yellow = nullptr;
	const wardrobe::Garment* White = nullptr;
	for (const wardrobe::Garment& G : wardrobe::Catalog())
	{
		if (G.Tier == 0 && !G.Rare && G.Condition == 0)
		{
			Yellow = !Yellow && wardrobe::TagOf(G) == 1 ? &G : Yellow;
			White = !White && wardrobe::TagOf(G) == 0 ? &G : White;
		}
	}
	Expect(Yellow && White, "thrift: tags of every color");
	if (Yellow && White)
	{
		Expect(wardrobe::ThriftPrice(*Yellow, When(11, 12.0)) == Yellow->PriceCents && wardrobe::ThriftPrice(*Yellow, When(12, 12.0)) == (Yellow->PriceCents + 1) / 2,
			"thrift: yellow tags are half off Saturday, not Friday");
		Expect(wardrobe::ThriftPrice(*White, When(12, 12.0)) == White->PriceCents, "thrift: the other tags aren't");
	}
	Expect(wardrobe::ThriftPrice(*wardrobe::Find("outer.hoodie.navy~rough"), When(11, 12.0)) == 560 || wardrobe::TagOf(*wardrobe::Find("outer.hoodie.navy~rough")) == 1,
		"thrift: rough is 30% off");
	Expect(wardrobe::ThriftPrice(*wardrobe::Find("outer.leather.rider"), When(12, 12.0)) == 4500, "thrift: a rare find is never marked down");
	int Yellows = 0;
	int Pieces = 0;
	for (const wardrobe::Garment& G : wardrobe::Catalog())
	{
		if (G.Tier == 0 && !G.Rare)
		{
			++Pieces;
			Yellows += wardrobe::TagOf(G) == 1 ? 1 : 0;
		}
	}
	Expect(Yellows * 8 > Pieces && Yellows * 3 < Pieces, "thrift: about a fifth of the racks are yellow");

	// The racks: restocked Thursdays, 24-30 pieces (and a rare find in some weeks), deterministic, only what renders.
	Expect(wardrobe::ThriftWeek(When(9, 23.9)) + 1 == wardrobe::ThriftWeek(When(10, 0.0)) && wardrobe::ThriftWeek(When(10, 0.0)) == wardrobe::ThriftWeek(When(16, 23.9)),
		"thrift: the week turns on Thursday");
	const uint64_t Seed = 12345;
	int Rares = 0;
	bool Sizes = true;
	bool Same = true;
	bool OneEach = true;
	for (int Week = 0; Week < 8; ++Week)
	{
		const std::vector<std::string> Stock = wardrobe::ThriftStock(Week, Seed, ~0ull);
		Same = Same && Stock == wardrobe::ThriftStock(Week, Seed, ~0ull);
		int Rare = 0;
		std::set<std::string> Bases;
		for (const std::string& Id : Stock)
		{
			const wardrobe::Garment* G = wardrobe::Find(Id);
			Rare += G && G->Rare ? 1 : 0;
			OneEach = OneEach && G && G->Tier == 0 && Bases.insert(Id.substr(0, Id.find('~'))).second;
		}
		Rares += Rare;
		Sizes = Sizes && static_cast<int>(Stock.size()) - Rare >= 24 && static_cast<int>(Stock.size()) - Rare <= 30 && Rare <= 1;
	}
	Expect(Sizes && Same && OneEach, "thrift: 24-30 pieces a week, one of a garment, the same for the same week and seed");
	Expect(Rares >= 2 && Rares <= 4, "thrift: one or two rare finds in each four weeks");
	Expect(wardrobe::ThriftStock(0, Seed, ~0ull) != wardrobe::ThriftStock(1, Seed, ~0ull) && wardrobe::ThriftStock(0, Seed, ~0ull) != wardrobe::ThriftStock(0, Seed + 1, ~0ull),
		"thrift: new racks every week, and every career's its own");
	// Only kinds the host can show: P0's tee, hoodie, bomber, jeans and chinos.
	uint64_t Kinds = 0;
	for (const wardrobe::Cut K : {wardrobe::Cut::Tee, wardrobe::Cut::Hoodie, wardrobe::Cut::Bomber, wardrobe::Cut::Jeans, wardrobe::Cut::Chinos})
	{
		Kinds |= 1ull << static_cast<int>(K);
	}
	bool Shown = true;
	for (int Week = 0; Week < 4; ++Week)
	{
		const std::vector<std::string> Stock = wardrobe::ThriftStock(Week, Seed, Kinds);
		Shown = Shown && Stock.size() >= 24;
		for (const std::string& Id : Stock)
		{
			Shown = Shown && ((Kinds >> static_cast<int>(wardrobe::Find(Id)->Kind)) & 1ull) != 0;
		}
	}
	Expect(Shown, "thrift: only the kinds the 3D can show go out");
}

// ------------------------------------------------------------------ the closet

void Closet()
{
	using namespace ss;
	// Any look the creator makes is a closet, and collapses back to itself.
	bool Same = true;
	for (int Outfit = 0; Outfit <= hero::OutfitNone; ++Outfit)
	{
		for (int Color = 0; Color < 8; ++Color)
		{
			for (int Hat = 0; Hat <= 4; ++Hat)
			{
				for (int Glasses = 0; Glasses <= 4; ++Glasses)
				{
					hero::Look L;
					L.Outfit = Outfit;
					L.OutfitColor = Color;
					L.Hat = Hat;
					L.Glasses = Glasses;
					const wardrobe::Closet C = wardrobe::SeedFrom(L);
					hero::Look Back = L;
					Back.Outfit = 3;
					Back.OutfitColor = Outfit == hero::OutfitNone ? Color : 5;
					Back.Hat = 1;
					Back.Glasses = 2;
					wardrobe::ApplyTo(C, Back);
					Same = Same && Back == L && C.Seeded && C.Owned.count("top.tee.white") > 0 && C.Worn[2] == "bottom.jeans.mid" && C.Worn[3] == "shoes.sneakers.white";
				}
			}
		}
	}
	Expect(Same, "closet: every created look seeds a closet that collapses back to it");
	const wardrobe::Closet Plain = wardrobe::SeedFrom(hero::Look());
	Expect(wardrobe::StyleOf(Plain) > 0.3 && wardrobe::StyleOf(Plain) < 0.6 && wardrobe::StyleOf(wardrobe::Closet()) == 0.0, "closet: style, and none without a closet");
	// One meaning for an empty Shoes slot: barefoot (the closet's word), worth nothing. Any pair beats none, by exactly the
	// shoes' share, with a jacket on or without; kicking them off never dresses an outfit up.
	{
		bool Shod = true;
		int Pairs = 0;
		for (const bool Jacket : {true, false})
		{
			wardrobe::Closet Bare = wardrobe::SeedFrom(hero::Look());
			if (!Jacket)
			{
				Bare.Worn[static_cast<size_t>(wardrobe::Slot::Outer)].clear();
			}
			Bare.Worn[static_cast<size_t>(wardrobe::Slot::Shoes)].clear();
			const double Barefoot = wardrobe::StyleOf(Bare);
			const double Weight = Jacket ? 1.0 : 0.65;
			for (const wardrobe::Garment& G : wardrobe::Catalog())
			{
				if (G.Where != wardrobe::Slot::Shoes)
				{
					continue;
				}
				wardrobe::Closet On = Bare;
				On.Owned.insert(G.Id);
				On.Worn[static_cast<size_t>(wardrobe::Slot::Shoes)] = G.Id;
				const double With = wardrobe::StyleOf(On);
				Shod = Shod && With > Barefoot && Near(With - Barefoot, 0.20 * G.Style / Weight, 1e-12);
				++Pairs;
			}
		}
		Expect(Shod && Pairs >= 10, "closet: barefoot adds nothing to Style; every pair of shoes beats it by its own share");
	}

	Hooks H;
	Session S(H, "shops-closet");
	Place(S, When(8, 12.0), 100000);
	S.NewCareer(Someone());
	Place(S, When(8, 12.0), 100000);
	double Now = 0.0;
	S.Update(Now);
	const double Style = wardrobe::StyleOf(S.Life.Closet);
	// A leather jacket at the thrift, worn out.
	const std::string Leather = "outer.leather.black";
	Expect(S.ShopPay(Order("thrift", {}, 0, {Leather, "bottom.chinos.khaki"})).empty() && S.Life.Closet.Owned.count(Leather) > 0, "closet: bought at the thrift");
	Expect(S.Life.Ledger.front().Label == "Second Chance Thrift \xC2\xB7 Black leather jacket & Khaki chinos" && S.Life.Ledger.front().Kind == 9, "closet: the receipt");
	Expect(S.ShopPay(Order("thrift", {}, 0, {Leather})) == "You already own that.", "closet: not twice");
	Expect(S.Wear(Leather).empty() && S.Person.Appearance.Outfit == 4 && S.Person.Appearance.OutfitColor == 5 && wardrobe::StyleOf(S.Life.Closet) > Style, "closet: worn out, sharper");
	Expect(S.TakeOff(static_cast<int>(wardrobe::Slot::Outer)).empty() && S.Person.Appearance.Outfit == hero::OutfitNone, "closet: off, and no jacket");
	Expect(S.TakeOff(static_cast<int>(wardrobe::Slot::Outer)) == "Nothing to take off.", "closet: nothing more");
	Expect(S.Wear("top.tee.white").empty() && S.Person.Appearance.OutfitColor == 4, "closet: a white tee on the portrait");
	const Chips Bank = S.BankrollCents;
	Expect(S.Donate("top.tee.white").empty() && S.Life.Closet.Owned.count("top.tee.white") == 0 && S.Life.Closet.Worn[0].empty() && S.BankrollCents == Bank,
		"closet: given away, for nothing");
	Expect(S.Life.Ledger.front().Amount == 0 && S.Life.Ledger.front().Label.find("donation") != std::string::npos, "closet: Bea's receipt for your taxes");
	Expect(S.Life.Town.count("bea") > 0 && (S.Life.Town["bea"].Flags & town::Donor) != 0 && S.Life.Town["bea"].Familiarity >= 2, "closet: Bea remembers");
	Place(S, When(13, 20.0), 100000);
	Expect(S.Donate("bottom.chinos.khaki").rfind("Closed", 0) == 0, "closet: the bin's inside, and the doors are locked");
}

// ------------------------------------------------------------------ eviction, waits, the desk

void Eviction()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-evicted");
	S.NewCareer(Someone());
	Place(S, When(8, 17.0), 100000);
	S.Gear["gym"] = When(8, 17.0) + 20.0 * Day;
	S.Life.RentStage = life::Rent::Evicted;
	S.Life.EvictedAt = When(8, 0.0);
	S.Life.Energy = 50.0;
	S.Life.Hunger = 60.0;
	double Now = 0.0;
	S.Update(Now);
	Expect(S.Evicted() && S.Life.Closet.Seeded && S.Wear("top.tee.white").empty(), "evicted: the closet travels to Dee's couch");
	Expect(S.ShopService(Order("gym", {"gym/treadmill"})).empty() && S.TimeSkip.Couch, "evicted: the gym still works");
	Finish(S, Now);
	Expect(S.Fitness() > 0.0 && Near(S.Life.Energy, 40.0, 0.01), "evicted: and counts");
	Expect(S.ShopService(Order("diner", {"diner/two-eggs"}, 18)).empty(), "evicted: Ruby's");
	Finish(S, Now);
	Expect(Near(S.Life.Energy, 40.0 + 5.0 * S.Life.Perks.MealBoost, 0.01), "evicted: a meal's energy comes whole on the couch");
}

void Waits()
{
	using namespace ss;
	{
		// Coffee, then three hours in a booth: the drain is 3 an hour, and the refills come every half hour.
		Hooks H;
		Session S(H, "shops-sit");
		Place(S, When(9, 1.0), 100000);
		S.Life.Energy = 60.0;
		S.Life.Hunger = 10.0;
		S.Life.Thirst = 30.0;
		double Now = 0.0;
		S.Update(Now);
		const double Ordered = S.WorldMinutes();
		Expect(S.ShopService(Order("diner", {"diner/coffee"}, 18)).empty(), "sit: a coffee");
		Finish(S, Now);
		const double E0 = S.Life.Energy;
		const double W = S.WorldMinutes();
		Expect(Near(E0, 68.0, 0.01) && Near(W, Ordered + 15.0, 0.02), "sit: the first cup's +8");
		// Bottomless is the Session's rule, not only the page's: a cup going isn't rung up again, alone or beside a plate,
		// whoever calls (Marlene's words at 1 AM), and its first cup's energy can't be bought twice.
		{
			const Chips Bank = S.BankrollCents;
			const size_t Lines = S.Life.Ledger.size();
			const std::string Cup = S.ShopBlocked("diner/coffee");
			Expect(Cup == "It's bottomless, hon. Sit a while and I'll keep it coming." && S.ShopService(Order("diner", {"diner/coffee"}, 18)) == Cup &&
					   S.ShopService(Order("diner", {"diner/pie", "diner/coffee"}, 18)) == Cup && S.ShopPay(Order("diner", {"diner/coffee"})) != std::string() &&
					   S.BankrollCents == Bank && S.Life.Ledger.size() == Lines && !S.TimeSkip.Active && Near(S.Life.Energy, E0),
				"coffee: a cup going isn't sold again, by any call");
			Expect(S.ShopBlocked("diner/pie").empty(), "coffee: the pie still is");
		}
		Expect(S.WaitAt("diner", W + 180.0).empty() && S.TimeSkip.Active && S.TimeSkip.Result.ActivityId.empty() && S.TimeSkip.RealSeconds == 4.5, "sit: a while");
		Finish(S, Now);
		// Refills at 30, 60, 90, 120, 150 and 180 minutes after the order: the second cup +4, the third +2, then only the
		// drain held at 3 an hour (§5.4's +8, +4, +2).
		Expect(Near(S.WorldMinutes(), W + 180.0, 0.02) && Near(S.Life.Energy, E0 - 9.0 + 6.0, 0.05), "sit: three hours at 3 an hour, and two refills' worth");
		Expect(S.LastOutcome.Body.find("topped up the coffee 5 times") != std::string::npos || S.LastOutcome.Body.find("topped up the coffee 6 times") != std::string::npos,
			"sit: the card counts the refills");
		Expect(S.LastOutcome.Body.rfind("Three hours in a booth at Ruby's", 0) == 0, "sit: the card spells the hours");
		// Eight more hours on the same cup: the drain stays at 3 an hour and nothing more comes of the pot.
		const double E1 = S.Life.Energy;
		Expect(S.WaitAt("diner", S.WorldMinutes() + 480.0).empty(), "sit: all night");
		Finish(S, Now);
		Expect(Near(S.Life.Energy, E1 - 24.0, 0.05) && S.Life.Thirst > 30.0, "sit: eight hours at the counter cost 24, and the thirst climbs as awake");
		// The pot's done a couple of hours after the hero gets up: then a cup is a new cup, rung up again.
		Expect(!S.ShopBlocked("diner/coffee").empty(), "coffee: still going when the hero stands up");
		Jump(S, Now, S.Life.CoffeeUntil + 1.0);
		Expect(S.ShopBlocked("diner/coffee").empty(), "coffee: gone cold, a new cup can be had");
	}
	{
		Hooks H;
		Session S(H, "shops-sit-dry");
		Place(S, When(9, 1.0), 100000);
		S.Life.Energy = 60.0;
		S.Life.Hunger = 10.0;
		S.Life.Thirst = 10.0;
		double Now = 0.0;
		S.Update(Now);
		Expect(S.WaitAt("diner", S.WorldMinutes() + 120.0).empty(), "sit: no coffee");
		Finish(S, Now);
		Expect(Near(S.Life.Energy, 52.0, 0.05), "sit: then it's 4 an hour");
		Expect(S.LastOutcome.Body.rfind("Two hours in a booth at Ruby's", 0) == 0 && S.LastOutcome.Body.find("Nobody hurried you.") != std::string::npos, "sit: no coffee, no refills");
		Expect(S.WaitAt("diner", S.WorldMinutes() + 90.0).empty(), "sit: an hour and a half");
		Finish(S, Now);
		Expect(S.LastOutcome.Body.rfind("90 minutes in a booth", 0) == 0, "sit: the card says the time sat, not the nearest hour");
		Expect(S.WaitAt("diner", S.WorldMinutes() + 9.0 * 60.0) == "Eight hours, tops." && S.WaitAt("diner", S.WorldMinutes()) == "Nothing to wait for.", "sit: one to eight hours");
		Expect(S.WaitAt("kims", S.WorldMinutes() + 60.0) == "Nowhere to sit here.", "sit: not in a display window");
		Expect(S.WaitAt("barber", S.WorldMinutes() + 30.0).rfind("Closed", 0) == 0, "sit: the barber's bench is inside a shut shop");
		Expect(S.WaitAt("shelter", S.WorldMinutes() + 20.0).empty() && S.TimeSkip.Label == "Waiting for the 14", "sit: the shelter, any hour");
		Finish(S, Now);
	}
}

void GymDesk()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-desk-pass");
	Place(S, When(8, 7.0), 100000);
	double Now = 0.0;
	S.Update(Now);
	Expect(S.ShopBlocked("gym/treadmill") == "Day pass or membership first.", "desk: no pass, no machines");
	Expect(S.ShopService(Order("gym", {"gym/daypass", "gym/treadmill"})).empty(), "desk: a pass and a run together");
	Finish(S, Now);
	Expect(S.ShopBlocked("gym/treadmill").empty() && S.ShopBlocked("gym/daypass") == "You've got today's pass.", "desk: the pass lasts the day");
	Expect(S.ShopPay(Order("gym", {"gym/shake"})).empty() && S.LastEaten.Line.find("IronHouse shake") != std::string::npos && S.Life.Pantry.count("gym/shake") == 0,
		"desk: the shake, drunk at the desk");
	Expect(S.ShopPay(Order("gym", {"gym/cascade"})).empty() && S.Life.Pantry["cascade"] == 1, "desk: the cooler's water into the bag");
	Jump(S, Now, When(9, 7.0));
	Expect(S.ShopBlocked("gym/treadmill") == "Day pass or membership first.", "desk: tomorrow's another pass");
	Jump(S, Now, When(9, 13.0));
	Expect(!S.ShopOpenFor("gym") && S.ShopBlocked("gym/daypass").rfind("Closed", 0) == 0, "desk: between Rosa's shifts it's members only");
	S.BankrollCents += 5000;
	S.Life.Record(S.WorldMinutes(), "test", 5000, 7);
	Expect(S.Buy("gym").empty() && S.ShopOpenFor("gym") && S.ShopBlocked("gym/shake").rfind("Nobody at the desk till 4:00 PM", 0) == 0, "desk: a member's fob, but nobody to make a shake");
	Expect(S.ShopBlocked("gym/trainer").rfind("Nobody at the desk", 0) == 0, "desk: Rosa trains while she's in");

	// Joining on the way to the floor: one order, the membership and an hour with Rosa, both charged and both given.
	Hooks Hj;
	Session J(Hj, "shops-desk-join");
	Place(J, When(8, 17.0), 100000);
	double Then = 0.0;
	J.Update(Then);
	const Chips Bank = J.BankrollCents;
	const size_t Lines = J.Life.Ledger.size();
	Expect(J.ShopService(Order("gym", {"gym/membership", "gym/daypass"})) == "Membership covers today. Skip the pass." && J.BankrollCents == Bank && J.Life.Ledger.size() == Lines,
		"desk: a day pass with the membership is refused, and costs nothing");
	Expect(J.ShopPay(Order("gym", {"gym/daypass", "gym/membership"})) == "Membership covers today. Skip the pass." && J.BankrollCents == Bank && J.Life.Ledger.size() == Lines &&
			   !J.Owns("gym") && J.Life.Visits.count("gym/daypass") == 0,
		"desk: never both on one receipt, whichever way it's paid (the desk's page says the same)");
	Expect(J.ShopService(Order("gym", {"gym/membership", "gym/trainer"})).empty() && J.Owns("gym") && J.BankrollCents == Bank - gear::Find("gym")->PriceCents - 4000 &&
			   Near(J.Gear["gym"], J.WorldMinutes() + 30.0 * Day, 0.02),
		"desk: joined and trained in one order: charged for both, and a member for thirty days");
	Finish(J, Then);
	Expect(J.Life.Fit.Sessions == 1 && J.LastOutcome.Body.rfind("An hour at IronHouse.", 0) == 0, "desk: the session counted, and the card starts with a capital");
	// Rosa's hour ends inside her shift, fob or no fob; the machines are the fob's.
	Jump(J, Then, When(9, 21.25));
	J.Life.Energy = 80.0;
	Expect(J.ShopBlocked("gym/trainer") == "Not enough time before the desk closes." && J.ShopBlocked("gym/treadmill").empty(), "desk: no hour with Rosa at a quarter past nine");
	// The order's workouts together have to fit in what's left.
	J.Life.Energy = 20.0;
	Expect(J.ShopBlocked("gym/weights").empty() && J.ShopService(Order("gym", {"gym/weights", "gym/treadmill"})) == "You don't have it in you. Eat or sleep first." && !J.TimeSkip.Active,
		"desk: two workouts that take more than's left are refused together");
}

// ------------------------------------------------------------------ the ledger, the monkey way

void LedgerInvariant()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-monkey");
	S.NewCareer(Someone());
	Place(S, When(8, 9.0), 50000);
	S.Gear["tower-mid"] = 0.0;
	S.Gear["monitor-24"] = 0.0;
	S.Gear["chair"] = 0.0;
	S.Life.LiveBestPlace = 1;
	double Now = 0.0;
	S.Update(Now);
	Rng R("shops-monkey-ops");
	const std::vector<std::string> Offers = {"barber/lineup", "barber/shave", "barber/color-auburn", "gym/daypass", "gym/treadmill", "gym/bag", "gym/shake", "gym/cascade",
		"diner/pie", "diner/coffee", "diner/two-eggs"};
	bool Ledgered = true;
	int Done = 0;
	for (int K = 0; K < 160; ++K)
	{
		// A clean page each step: whatever the step moves, the ledger's new lines must say.
		const Chips Before = S.BankrollCents;
		S.Life.Ledger.clear();
		std::string Why = "skip";
		switch (R.Int(9))
		{
		case 0:
		case 1:
		{
			const std::string& Id = Offers[static_cast<size_t>(R.Int(static_cast<int>(Offers.size())))];
			Why = S.ShopService(Order(shop::FindOffer(Id)->Venue, {Id}, R.Chance(0.5) ? 20 : 0));
			break;
		}
		case 2:
		{
			const std::vector<std::string> Stock = wardrobe::ThriftStock(wardrobe::ThriftWeek(S.WorldMinutes()), S.WorldSeed(), ~0ull);
			Why = S.ShopPay(Order("thrift", {}, 0, {Stock[static_cast<size_t>(R.Int(static_cast<int>(Stock.size())))]}));
			break;
		}
		case 3:
		{
			const std::vector<shop::Lot> Lots = S.PawnLots();
			if (!Lots.empty())
			{
				const shop::Lot& L = Lots[static_cast<size_t>(R.Int(static_cast<int>(Lots.size())))];
				Why = R.Chance(0.5) ? S.PawnLoan(L.Id) : S.PawnSell(L.Id);
			}
			break;
		}
		case 4:
			if (!S.Life.Pawn.Tickets.empty())
			{
				Why = R.Chance(0.7) ? S.PawnRedeem(S.Life.Pawn.Tickets.front().Number) : S.PawnRenew(S.Life.Pawn.Tickets.front().Number);
			}
			break;
		case 5:
		{
			const std::vector<shop::Lot> Shelf = S.PawnShelf();
			Why = S.PawnBuy(Shelf[static_cast<size_t>(R.Int(static_cast<int>(Shelf.size())))].Id);
			break;
		}
		case 6:
			if (!S.Life.Closet.Owned.empty())
			{
				auto It = S.Life.Closet.Owned.begin();
				std::advance(It, R.Int(static_cast<int>(S.Life.Closet.Owned.size())));
				Why = R.Chance(0.5) ? S.Wear(*It) : S.Donate(*It);
			}
			break;
		case 7: Why = S.WaitAt("diner", S.WorldMinutes() + 30.0 + 30.0 * R.Int(4)); break;
		default:
			Jump(S, Now, S.WorldMinutes() + 60.0 * R.Range(0.5, 9.0));
			S.Life.Energy = std::max(S.Life.Energy, 60.0);
			S.Life.Hunger = std::min(S.Life.Hunger, 40.0);
			S.Life.Thirst = std::min(S.Life.Thirst, 40.0);
			break;
		}
		Finish(S, Now);
		Done += Why.empty() ? 1 : 0;
		Ledgered = Ledgered && S.BankrollCents - Before == LedgerSum(S.Life);
		if (S.BankrollCents < 5000)
		{
			S.BankrollCents += 20000;
			S.Life.Record(S.WorldMinutes(), "top-up", 20000, 7);
		}
	}
	Expect(Ledgered, "ledger: every bankroll change is the ledger's");
	Expect(Done > 40, "ledger: and plenty of them went through");
}

// ------------------------------------------------------------------ the pawn

void PawnValues()
{
	using namespace ss;
	// Gear: V is 55% of GearDrop's price to the dollar; he pays half of V and lends 40% of it, down to the dollar.
	bool Gear = true;
	int Taken = 0;
	for (const gear::Item& I : gear::Catalog())
	{
		if (!shop::Takes(I))
		{
			continue;
		}
		++Taken;
		const Chips V = shop::GearValue(I);
		const Chips Want = static_cast<Chips>(std::llround(static_cast<double>(I.PriceCents) * 0.55 / 100.0)) * 100;
		Gear = Gear && V == Want && V % 100 == 0 && shop::SellOf(V) == V / 200 * 100 && shop::LoanOf(V) == V * 4 / 1000 * 100 && shop::SellOf(V) * 2 <= V;
	}
	Expect(Gear && Taken == 16, "pawn: every piece of gear he takes, at 55/50/40, whole dollars");
	for (const char* Id : {"ram-32", "overlay-pack", "mattress", "curtains", "plant", "led-kit", "gym", "fiber", "meal-kit", "modbot"})
	{
		Expect(!shop::Takes(*gear::Find(Id)), "pawn: not what he can't hold, not furniture, not subscriptions");
	}
	const gear::Item& Desktop = *gear::Find("tower-mid");
	Expect(shop::GearValue(Desktop) == 49400 && shop::SellOf(49400) == 24700 && shop::LoanOf(49400) == 19700, "pawn: the 8-core desktop: $494, $247, $197 (§5.8)");
	shop::Ticket T;
	T.LoanCents = 19700;
	T.TakenAt = 0.0;
	T.DueAt = 30.0 * Day;
	Expect(shop::RedeemCents(T, 29.0 * Day) == 23640 && shop::RedeemCents(T, 33.0 * Day) == 24625 && shop::RedeemCents(T, 37.0 * Day) == 24625 && shop::RedeemCents(T, 38.0 * Day) == 0,
		"pawn: $236.40 inside 30 days, +25% in the grace, nothing after");
	Expect(shop::RenewCents(T) == 3940 && shop::TicketNumber(417) == "0417", "pawn: the renewal's 20%; the ticket number");
	Expect(shop::RenewCents(T, 29.0 * Day) == 3940 && shop::RenewCents(T, 33.0 * Day) == 4925 && shop::RenewCents(T, 38.0 * Day) == 0,
		"pawn: a renewal costs what the ticket owes that day: 20% in term, 25% in the grace, nothing once it's his");
	Expect(!shop::Renewable(T, 0.0) && !shop::Renewable(T, 0.5 * Day) && shop::Renewable(T, Day) && shop::Renewable(T, 33.0 * Day) && !shop::Renewable(T, 38.0 * Day),
		"pawn: nothing to renew the day it's written; renewable from the next, through the grace; not once forfeited");
	// No profit in the order of things: at any hour of the term and the grace, renewing is the interest a redemption would
	// add that day, so renewing and then redeeming (any time in the new term) never costs less than redeeming at once.
	{
		bool Fair = true;
		int Points = 0;
		for (double W = 0.0; W <= 40.0 * Day; W += 60.0)
		{
			const Chips Redeem = shop::RedeemCents(T, W);
			const Chips Renew = shop::RenewCents(T, W);
			Fair = Fair && shop::Renewable(T, W) == (Redeem > 0 && W + 30.0 * Day - T.DueAt >= Day);
			if (Redeem <= 0)
			{
				Fair = Fair && Renew == 0;
				continue;
			}
			++Points;
			Fair = Fair && Renew == Redeem - T.LoanCents && Renew >= shop::RenewCents(T);
			shop::Ticket After = T;
			After.DueAt = W + 30.0 * Day;
			for (const double Later : {W + 1.0, W + 15.0 * Day, W + 30.0 * Day})
			{
				Fair = Fair && Renew + shop::RedeemCents(After, Later) >= Redeem;
			}
		}
		Expect(Fair && Points == 37 * 24 + 1, "pawn: renewing is never the cheap way out, in term or in the grace");
	}
	// The mid rig sells for about $620 and pawns for about $500 (§5.8).
	Chips Sold = 0;
	Chips Lent = 0;
	for (const char* Id : {"tower-mid", "monitor-24", "monitor-27", "webcam-1080", "mic-xlr", "key-lights", "chair"})
	{
		const Chips V = shop::GearValue(*gear::Find(Id));
		Sold += shop::SellOf(V);
		Lent += shop::LoanOf(V);
	}
	Expect(Sold == 61700 && Lent == 49200, "pawn: a mid rig: $617 sold, $492 lent");
	Expect(shop::LoanOf(shop::GearValue(*gear::Find("monitor-27"))) == 6300, "pawn: a ticket on the 27-inch is $63");

	// Bracelets and rings.
	world::Award A;
	A.Prize = 5000000; // $50,000
	Expect(shop::AwardValue(A) == 140000, "pawn: a bracelet: $900 and 1% of its prize");
	A.Prize = 200000000;
	Expect(shop::AwardValue(A) == 600000, "pawn: at most $6,000");
	A.Main = true;
	Expect(shop::AwardValue(A) == 900000, "pawn: $9,000 for a Main Event's");
	A.Ring = true;
	A.Main = false;
	A.Prize = 10000000; // $100,000
	Expect(shop::AwardValue(A) == 85000, "pawn: a ring: $350 and 0.5%");
	A.Prize = 200000000;
	Expect(shop::AwardValue(A) == 250000, "pawn: at most $2,500");
	// The keepsakes.
	const std::pair<hero::Background, Chips> Keeps[] = {{hero::Background::Kitchen, 18000}, {hero::Background::DealersKid, 30000}, {hero::Background::Dropout, 22000},
		{hero::Background::Bouncer, 24000}, {hero::Background::Hustler, 35000}, {hero::Background::Newcomer, 16000}};
	for (const auto& K : Keeps)
	{
		const shop::Lot L = shop::KeepsakeOf(K.first);
		Expect(L.ValueCents == K.second && L.Final && L.Kind == shop::LotKind::Keepsake && !L.Name.empty() && L.Id == std::string("keepsake:") + hero::InfoOf(K.first).Key &&
				   L.SellCents == shop::SellOf(K.second) && L.LoanCents == shop::LoanOf(K.second),
			"pawn: every keepsake (§5.8's table)");
	}
}

/** A session at the pawn's counter (Tuesday noon), with a rig, the cup, a bracelet, Dee's chip and the keepsake. */
void PawnSetup(ss::Session& S, Hooks& /*H*/, double& Now)
{
	S.NewCareer(Someone());
	Place(S, When(8, 12.0), 50000);
	S.Gear["tower-mid"] = 0.0;
	S.Gear["monitor-24"] = 0.0;
	S.Gear["monitor-27"] = 0.0;
	S.Gear["chair"] = 0.0;
	S.Gear["ram-32"] = 0.0;
	S.Gear["mattress"] = 0.0;
	S.Life.LiveBestPlace = 1;
	S.Life.BackRoomNetCents = 4000;
	S.Living().GrantAward(-1, false, false);
	S.SetLedsOn(false); // the gear above, counted (as a purchase would have it)
	S.Update(Now);
}

void PawnCounter()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-pawn-counter");
	double Now = 0.0;
	PawnSetup(S, H, Now);
	const std::vector<shop::Lot> Lots = S.PawnLots();
	Expect(FindLot(Lots, "gear:tower-mid") && FindLot(Lots, "gear:chair") && !FindLot(Lots, "gear:ram-32") && !FindLot(Lots, "gear:mattress"), "pawn: he takes the rig and the chair, not the RAM or the bed");
	Expect(FindLot(Lots, "memento:cup") && FindLot(Lots, "memento:cup")->Final && FindLot(Lots, "keepsake:kitchen") && FindLot(Lots, "keepsake:kitchen")->ValueCents == 18000,
		"pawn: the cup and the keepsake, both final");
	const shop::Lot* Chip = FindLot(Lots, "memento:deechip");
	Expect(Chip && Chip->Refused.find("She'd have my license") != std::string::npos && S.PawnSell("memento:deechip") == Chip->Refused, "pawn: Dee's chip refused");
	const shop::Lot* Bracelet = nullptr;
	for (const shop::Lot& L : Lots)
	{
		Bracelet = L.Kind == shop::LotKind::Award ? &L : Bracelet;
	}
	Expect(Bracelet && Bracelet->Final && Bracelet->ValueCents >= 90000 && Bracelet->Name.find("bracelet") != std::string::npos, "pawn: the bracelet from the trophy case");
	Expect(!S.Living().HeroAwards().empty(), "pawn: (the trophy case has one)");
	if (!Bracelet)
	{
		return;
	}

	// Pawning the desktop: the cash, the ticket, the desk and the stream lose it at once.
	const int PcTier = S.GearFx().PcTier;
	const int Tables = S.GearFx().Tables;
	const Chips Bank = S.BankrollCents;
	Expect(PcTier == 2 && S.PawnLoan("gear:tower-mid").empty() && S.BankrollCents == Bank + 19700 && !S.Owns("tower-mid") && S.GearFx().PcTier < PcTier, "pawn: the desktop's on a ticket");
	Expect(S.Life.Ledger.front().Label == "Northside Pawn & Loan (loan #0412)" && S.Life.Ledger.front().Kind == 9 && S.Life.Pawn.Tickets.size() == 1 && S.Life.Pawn.NextTicket == 413,
		"pawn: ticket #0412");
	Expect(S.Life.Town.count("augie") > 0 && (S.Life.Town["augie"].Flags & town::Owed) != 0, "pawn: Augie's owed");
	Expect(S.PawnSell("gear:monitor-24").empty() && S.GearFx().Tables == Tables - 1 && S.BankrollCents == Bank + 19700 + shop::SellOf(shop::GearValue(*gear::Find("monitor-24"))),
		"pawn: selling the monitor takes a table at once");
	Expect(FindLot(S.PawnShelf(), "gear:monitor-24") && FindLot(S.PawnShelf(), "gear:monitor-24")->ValueCents == 8200, "pawn: and it's on his shelf at V");
	// The bracelet, pawned: Dee hears.
	Expect(S.PawnLoan(Bracelet->Id).empty() && S.PawnHolds(Bracelet->Id) && H.Got("Dee", "pawned the bracelet"), "pawn: the bracelet pawned; Dee hears");

	// Redeem inside the term (+20%), through the ledger: it comes home, and the desk with it.
	shop::Age(S.Life.Pawn, 29.0);
	Jump(S, Now, S.WorldMinutes() + 1.0);
	const Chips Before = S.BankrollCents;
	Expect(S.PawnRedeem(412).empty() && S.BankrollCents == Before - 23640 && S.Owns("tower-mid") && S.GearFx().PcTier == PcTier, "pawn: redeemed at day 29 for $236.40");
	Expect(S.Life.Ledger.front().Label == "Northside Pawn & Loan (redeemed #0412)", "pawn: the redemption's line");
	// The bracelet's in its grace by day 33: +25%.
	shop::Age(S.Life.Pawn, 4.0);
	Jump(S, Now, S.WorldMinutes() + 1.0);
	const shop::Ticket Br = S.Life.Pawn.Tickets.front();
	const Chips Grace = Br.LoanCents * 125 / 100;
	const Chips Was = S.BankrollCents;
	Expect(S.PawnRedeem(Br.Number).empty() && S.BankrollCents == Was - Grace && !S.PawnHolds(Bracelet->Id), "pawn: redeemed in the grace at +25%");
	Expect(S.Life.Pawn.Tickets.empty() && (S.Life.Town["augie"].Flags & town::Owed) == 0, "pawn: owed nothing");
}

void PawnForfeit()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-pawn-forfeit");
	double Now = 0.0;
	PawnSetup(S, H, Now);
	std::string Award;
	for (const shop::Lot& L : S.PawnLots())
	{
		Award = L.Kind == shop::LotKind::Award ? L.Id : Award;
	}
	const size_t Titles = S.Living().HeroAwards().size();
	Expect(S.PawnLoan("gear:monitor-27").empty() && S.PawnLoan("memento:cup").empty() && S.PawnHolds("memento:cup"), "forfeit: two tickets");
	Expect(!Award.empty() && S.PawnLoan(Award).empty() && S.PawnHolds(Award) && S.Life.Pawn.Tickets.size() == 3, "forfeit: and the bracelet, a third");
	// The reminders: three days before it's due, and on the grace's first day.
	shop::Age(S.Life.Pawn, 26.9);
	Jump(S, Now, S.WorldMinutes() + 0.2 * Day);
	Expect(H.Got("Northside Pawn & Loan", "#0412 (27\" monitor) is due") && H.Got("Northside Pawn & Loan", "$75.60 redeems it"), "forfeit: the due-date reminder");
	Jump(S, Now, S.WorldMinutes() + 3.0 * Day);
	Expect(H.Got("Northside Pawn & Loan", "#0412 is in grace until") && H.Got("Northside Pawn & Loan", "After that it's ours."), "forfeit: the grace reminder");
	// Day 38: forfeited. The monitor on his shelf, the cup in his case, for 21 days.
	const double Taken = S.Life.Pawn.Tickets.front().TakenAt;
	Jump(S, Now, Taken + 38.0 * Day);
	Expect(S.Life.Pawn.Tickets.empty(), "forfeit: all three gone at day 38");
	const std::vector<shop::Lot> Shelf = S.PawnShelf();
	Expect(FindLot(Shelf, "gear:monitor-27") && FindLot(Shelf, "gear:monitor-27")->ValueCents == shop::GearValue(*gear::Find("monitor-27")) && FindLot(Shelf, "memento:cup") &&
			   FindLot(Shelf, "memento:cup")->ValueCents == 6000,
		"forfeit: on his shelf at V");
	const shop::Lot* InCase = FindLot(Shelf, Award);
	Expect(InCase && InCase->Final && InCase->ValueCents >= 90000 && S.PawnHolds(Award) && !FindLot(S.PawnLots(), Award), "forfeit: the bracelet in his case at V, out of the trophy case");
	Expect(S.PawnHolds("memento:cup") && !FindLot(S.PawnLots(), "memento:cup"), "forfeit: the cup isn't on the sill");
	// GearDrop's upgrade path holds at his shelf: the 27-inch goes beside a 24.
	Expect(S.PawnSell("gear:monitor-24").empty() && FindLot(S.PawnShelf(), "gear:monitor-27")->Refused == "Needs the 24\" monitor first." &&
			   S.PawnBuy("gear:monitor-27") == "Needs the 24\" monitor first." && S.PawnBuy("gear:monitor-24").empty() && FindLot(S.PawnShelf(), "gear:monitor-27")->Refused.empty(),
		"forfeit: the 27-inch needs the 24 beside it");
	// Buying the monitor back restores it; the cup's sold on 21 days later and gone for good.
	const Chips Bank = S.BankrollCents;
	const Chips V = shop::GearValue(*gear::Find("monitor-27"));
	Expect(S.PawnBuy("gear:monitor-27").empty() && S.Owns("monitor-27") && S.BankrollCents == Bank - V - store::Tax(V), "forfeit: bought back, taxed");
	Expect(S.Life.Ledger.front().Label == "Northside Pawn & Loan (bought: 27\" monitor)" && !S.PawnBuy("gear:monitor-27").empty(), "forfeit: its line; not twice");
	Jump(S, Now, Taken + 37.0 * Day + 21.0 * Day + 1.0);
	Expect(!FindLot(S.PawnShelf(), "memento:cup") && S.Life.Pawn.Away.count("memento:cup") > 0 && S.PawnHolds("memento:cup"), "forfeit: then away for good");
	Expect(!FindLot(S.PawnShelf(), Award) && S.Life.Pawn.Away.count(Award) > 0 && S.PawnHolds(Award) && S.Living().HeroAwards().size() == Titles,
		"forfeit: the bracelet sold on 21 days later, its title still in the records");
}

void PawnRenewStoreAuction()
{
	using namespace ss;
	{
		Hooks H;
		Session S(H, "shops-pawn-renew");
		double Now = 0.0;
		PawnSetup(S, H, Now);
		Expect(S.PawnLoan("gear:chair").empty(), "renew: a ticket");
		const Chips Fresh = S.BankrollCents;
		Expect(S.PawnRenew(S.Life.Pawn.Tickets.front().Number).rfind("Nothing to renew yet.", 0) == 0 && S.BankrollCents == Fresh, "renew: not a ticket written today");
		shop::Age(S.Life.Pawn, 20.0);
		Jump(S, Now, S.WorldMinutes() + 1.0);
		const shop::Ticket T = S.Life.Pawn.Tickets.front();
		const Chips Bank = S.BankrollCents;
		Expect(S.PawnRenew(T.Number).empty() && S.BankrollCents == Bank - T.LoanCents / 5 && S.Life.Pawn.Tickets.front().Renewals == 1 &&
				   Near(S.Life.Pawn.Tickets.front().DueAt, S.WorldMinutes() + 30.0 * Day, 0.02),
			"renew: 20% buys another thirty days");
		Expect(S.PawnRenew(T.Number).rfind("Nothing to renew yet.", 0) == 0 && S.BankrollCents == Bank - T.LoanCents / 5 && S.Life.Pawn.Tickets.front().Renewals == 1,
			"renew: a second press the same day pays nothing");
		// Three days into the grace: a renewal costs the grace's 25%, as redeeming would; the ledger says so.
		shop::Age(S.Life.Pawn, 33.0);
		Jump(S, Now, S.WorldMinutes() + 1.0);
		const shop::Ticket G = S.Life.Pawn.Tickets.front();
		const double W = S.WorldMinutes();
		Expect(W > G.DueAt && shop::RedeemCents(G, W) == G.LoanCents * 125 / 100 && shop::RenewCents(G, W) == G.LoanCents / 4 && shop::Renewable(G, W),
			"renew: in the grace the ticket owes 25%");
		const Chips Was = S.BankrollCents;
		Expect(S.PawnRenew(G.Number).empty() && S.BankrollCents == Was - G.LoanCents / 4 && S.Life.Ledger.front().Amount == -(G.LoanCents / 4) &&
				   S.Life.Ledger.front().Label == "Northside Pawn & Loan (renewed #" + shop::TicketNumber(G.Number) + ")" && S.Life.Pawn.Tickets.front().Renewals == 2 &&
				   Near(S.Life.Pawn.Tickets.front().DueAt, W + 30.0 * Day, 0.02),
			"renew: in the grace, at the grace's 25%, and thirty days from today");
		// Short of the 25%: declined, nothing moves.
		S.Life.Pawn.Tickets.front().DueAt = W - 2.0 * Day;
		const Chips Short = S.Life.Pawn.Tickets.front().LoanCents / 4 - 100;
		S.Life.Record(W, "test", Short - S.BankrollCents, 7);
		S.BankrollCents = Short;
		const size_t Lines = S.Life.Ledger.size();
		Expect(S.PawnRenew(G.Number) == "Card declined." && S.BankrollCents == Short && S.Life.Ledger.size() == Lines && Near(S.Life.Pawn.Tickets.front().DueAt, W - 2.0 * Day),
			"renew: a grace renewal the card can't cover is declined");
	}
	{
		// Evicted with the unit paid: the gear's in storage; selling it is an hour's round trip first.
		Hooks H;
		Session S(H, "shops-pawn-stored");
		double Now = 0.0;
		PawnSetup(S, H, Now);
		S.Life.RentStage = life::Rent::Evicted;
		S.Life.EvictedAt = S.WorldMinutes() - Day;
		S.Life.StorageDue = S.WorldMinutes() + 10.0 * Day;
		const shop::Lot* Desk = FindLot(S.PawnLots(), "gear:tower-mid");
		Expect(Desk && Desk->Stored, "stored: in Ninth St. Storage");
		const double W = S.WorldMinutes();
		const Chips Bank = S.BankrollCents;
		Expect(S.PawnSell("gear:tower-mid").empty() && S.TimeSkip.Active && S.TimeSkip.Label == "To Ninth St. Storage and back" && S.BankrollCents == Bank, "stored: an hour there and back first");
		Finish(S, Now);
		Expect(Near(S.WorldMinutes(), W + 60.0, 0.02) && !S.Owns("tower-mid") && S.BankrollCents == Bank + 24700 && S.LastOutcome.Title == "Sold", "stored: then sold");
		// On his shelf: the desktop just sold, and the week's used gear. What the hero owns is marked; the rest needs a room.
		bool Marked = FindLot(S.PawnShelf(), "gear:tower-mid") != nullptr;
		for (const shop::Lot& L : S.PawnShelf())
		{
			if (L.Kind == shop::LotKind::Gear)
			{
				Marked = Marked && L.Refused == (S.Owns(L.Id.substr(5)) ? "Owned." : "Needs a room.");
			}
		}
		Expect(Marked && S.PawnBuy("gear:tower-mid") == "Needs a room.", "stored: nothing that needs a room while on Dee's couch");
		S.Life.StorageDue = S.WorldMinutes() - 1.0;
		Expect(!FindLot(S.PawnLots(), "gear:monitor-24"), "stored: an unpaid unit's gear can't be fetched");
	}
	{
		// The storage auction: 1-3 of the lost pieces turn up on his shelf two days later, for 21.
		Hooks H;
		Session S(H, "shops-pawn-auction");
		double Now = 0.0;
		PawnSetup(S, H, Now);
		S.Life.RentStage = life::Rent::Evicted;
		S.Life.EvictedAt = S.WorldMinutes() - 31.0 * Day;
		S.Life.StorageDue = S.WorldMinutes() + 60.0;
		S.Life.Record(S.WorldMinutes(), "test", -S.BankrollCents + 1000, 7);
		S.BankrollCents = 1000;
		Jump(S, Now, S.WorldMinutes() + 120.0);
		Expect(S.Life.Auctions == 1 && !S.Owns("tower-mid"), "auction: Unit 114 sold");
		int Lots = 0;
		bool Timed = true;
		for (const shop::ShelfLot& L : S.Life.Pawn.Shelf)
		{
			Lots += L.Auction ? 1 : 0;
			Timed = Timed && (!L.Auction || (Near(L.Until - L.From, 21.0 * Day, 0.01) && L.From > S.WorldMinutes() + 1.9 * Day - 120.0));
			Timed = Timed && (!L.Auction || shop::Takes(*gear::Find(L.Lot.substr(5))));
		}
		Expect(Lots >= 1 && Lots <= 3 && Timed, "auction: 1-3 of its pieces consigned, 2 days later for 21");
		Expect(H.Got("Ninth St. Storage", "sold at auction"), "auction: the notice");
		// Back in the apartment, a lot bought back is home.
		Jump(S, Now, S.WorldMinutes() + 2.5 * Day);
		S.BankrollCents += 400000;
		S.Life.Record(S.WorldMinutes(), "test", 400000, 7);
		Expect(S.MoveBackIn().empty(), "auction: moved back in");
		std::string Lot;
		for (const shop::Lot& L : S.PawnShelf())
		{
			const auto Own = std::find_if(S.Life.Pawn.Shelf.begin(), S.Life.Pawn.Shelf.end(), [&L](const shop::ShelfLot& Sh) { return Sh.Lot == L.Id && Sh.Auction; });
			Lot = Own != S.Life.Pawn.Shelf.end() && L.Refused.empty() ? L.Id : Lot;
		}
		Jump(S, Now, When(net::DayOf(S.WorldMinutes()) + 1, 12.0));
		Expect(!Lot.empty() && S.PawnBuy(Lot).empty() && S.Owns(Lot.substr(5)), "auction: a lot bought back is home");
	}
	{
		// An award sold: final, gone for good, the title kept.
		Hooks H;
		Session S(H, "shops-pawn-award");
		double Now = 0.0;
		PawnSetup(S, H, Now);
		std::string Award;
		for (const shop::Lot& L : S.PawnLots())
		{
			Award = L.Kind == shop::LotKind::Award ? L.Id : Award;
		}
		const size_t Titles = S.Living().HeroAwards().size();
		Expect(!Award.empty() && S.PawnSell(Award).empty() && S.PawnHolds(Award) && S.Life.Pawn.Away.count(Award) > 0 && S.Living().HeroAwards().size() == Titles,
			"award: sold, gone for good, the title still in the records");
		Expect(H.Got("Dee", "sold the bracelet"), "award: Dee hears");
		Expect(S.PawnSell("memento:cup").empty() && S.PawnHolds("memento:cup") && !FindLot(S.PawnShelf(), "memento:cup"), "award: the cup sold isn't for sale again");
	}
}

/**
 * Big Lou's Friday night at Ruby's (§6.3): for a player who's cashed live in the last week he picks up the check, once a
 * night. The food and its tax are his (a line for nothing in the Bank, "Ruby's (Big Lou got it)"); the tip is still the
 * player's, on its own line; the night is marked so the next plate is the player's again; and it all saves and reloads.
 */
void BigLou()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-big-lou");
	S.NewCareer(Someone());
	// The first Friday night he's in his booth (some weeks he's away, or up at the ridge).
	double T = -1.0;
	for (int D = 4; D < 120 && T < 0.0; D += 7)
	{
		const double W = When(D, 23.5);
		Place(S, W, 100000);
		S.Living().AdvanceTo(W);
		town::Placement P;
		T = town::Where("big-lou", W, S, P) && P.Venue == "diner" && P.What == town::Doing::Sit ? W : -1.0;
	}
	Expect(T > 0.0, "lou: a Friday night with Big Lou in his booth at Ruby's");
	if (T < 0.0)
	{
		return;
	}
	double Now = 0.0;
	S.Update(Now);
	const ss::Session::ShopOrder Supper = Order("diner", {"diner/patty-melt"}, 18);
	const Chips Full = 1099 + 80 + 198;
	Expect(S.ShopTotal(Supper) == Full && town::PaysYourCheck(S, S.WorldMinutes()).empty(), "lou: no cash lately: the check is the player's");
	life::LiveEntry E;
	E.Id = "embercrest-nightly@" + std::to_string(net::DayOf(T) - 2);
	E.State = life::LiveEntry::Finished;
	E.PrizeCents = 31000;
	E.FinishedAt = T - 2.0 * Day;
	S.Life.LiveEntries.push_back(E);
	const double W = S.WorldMinutes();
	const std::string Key = town::CheckKey(S, W);
	Expect(town::PaysYourCheck(S, W) == "big-lou" && !Key.empty() && S.ShopTotal(Supper) == 198, "lou: he's got it: the total is the 18% tip on $10.99, alone");
	Expect(S.ShopTotal(Order("diner", {"diner/patty-melt"})) == 0, "lou: and nothing at all without a tip");

	// Short even of the tip: declined, and his night isn't spent.
	const Chips Bank = S.BankrollCents;
	S.Life.Record(W, "test", 100 - Bank, 7);
	S.BankrollCents = 100;
	size_t Lines = S.Life.Ledger.size();
	Expect(S.ShopService(Supper) == "Card declined." && S.BankrollCents == 100 && S.Life.Ledger.size() == Lines && S.Life.Said.count(Key) == 0 && !S.TimeSkip.Active,
		"lou: a tip the card can't cover is declined, and he'll still get it");
	S.Life.Record(W, "test", Bank - 100, 7);
	S.BankrollCents = Bank;

	// Paid: the tip alone through the card; his line for nothing; the tip's own; the night marked.
	Lines = S.Life.Ledger.size();
	const Chips Before = S.BankrollCents;
	Expect(S.ShopService(Supper).empty() && S.TimeSkip.Active && S.BankrollCents == Before - 198, "lou: charged the tip, nothing else");
	Expect(S.Life.Ledger.size() == Lines + 2 && S.Life.Ledger[0].Label == "Ruby's (Big Lou got it)" && S.Life.Ledger[0].Amount == 0 && S.Life.Ledger[0].Kind == 9 &&
			   S.Life.Ledger[1].Label == "Ruby's \xC2\xB7 Tip" && S.Life.Ledger[1].Amount == -198 && S.Life.Ledger[1].Kind == 9,
		"lou: the Bank shows his line for nothing and the tip on its own");
	Expect(S.Life.Said.count(Key) == 1 && S.Life.Said[Key] == net::DayOf(W) && town::PaysYourCheck(S, W).empty(), "lou: the night is marked: one check a night");
	Expect(S.Life.Visits["diner"].Tips == 1 && S.Life.Visits["diner"].SpentCents == 198, "lou: the visit counts the tip as what was spent");
	Finish(S, Now);
	Expect(S.LastOutcome.Title == "Patty melt & fries" && S.Life.RealMealAt > 0.0, "lou: supper all the same");

	// The next plate the same night is the player's, at full price.
	const Chips Then = S.BankrollCents;
	Expect(S.ShopTotal(Supper) == Full && S.ShopService(Order("diner", {"diner/pie"}, 0)).empty() && S.BankrollCents == Then - 450 - store::Tax(450) &&
			   S.Life.Ledger.front().Label == "Ruby's \xC2\xB7 Slice of pie",
		"lou: he covers one check a night; the pie after is the player's");
	Finish(S, Now);

	// Past midnight it's the same night (the key is the day his visit began): the check is still the player's.
	const double Friday = std::floor(T / Day) * Day;
	if (S.WorldMinutes() < Friday + Day + 20.0)
	{
		Jump(S, Now, Friday + Day + 20.0);
	}
	town::Placement Late;
	Expect(town::Where("big-lou", S.WorldMinutes(), S, Late) && Late.Venue == "diner" && Late.What == town::Doing::Sit && town::CheckKey(S, S.WorldMinutes()) == Key &&
			   town::PaysYourCheck(S, S.WorldMinutes()).empty() && S.ShopTotal(Supper) == Full,
		"lou: past midnight, still in his booth, it's the same night: no second check");

	// Another night, no tip: a line for nothing, no tip line, nothing charged.
	S.Life.Said.erase(Key);
	Lines = S.Life.Ledger.size();
	const Chips Was = S.BankrollCents;
	Expect(town::PaysYourCheck(S, S.WorldMinutes()) == "big-lou", "lou: a night not marked is his to pay");
	Expect(S.ShopService(Order("diner", {"diner/chicken-soup"}, 0)).empty() && S.BankrollCents == Was && S.Life.Ledger.size() == Lines + 1 &&
			   S.Life.Ledger.front().Label == "Ruby's (Big Lou got it)" && S.Life.Ledger.front().Amount == 0,
		"lou: no tip, nothing charged: his line alone");
	Finish(S, Now);

	// The ledger tells every cent of it, and the night he paid survives a save.
	S.Save();
	const std::string Text = H.Last.Serialize();
	SaveData D;
	Expect(SaveData::Parse(Text, D) && D.Serialize() == Text && D.Life.Said.count(Key) == S.Life.Said.count(Key), "lou: the save reads back the same");

	// A later Friday he's in his booth is another night: a key of its own, and that check is his again (for a player
	// who's cashed that week), however the clock got there.
	bool Again = false;
	for (int Week = 1; Week <= 12 && !Again; ++Week)
	{
		const double Next = Friday + 7.0 * Day * Week + 23.5 * 60.0;
		S.LobbyMinutes += Next - S.WorldMinutes();
		S.Living().AdvanceTo(Next);
		town::Placement P;
		if (!town::Where("big-lou", Next, S, P) || P.Venue != "diner" || P.What != town::Doing::Sit)
		{
			continue;
		}
		Expect(town::PaysYourCheck(S, Next).empty(), "lou: a cash more than a week old doesn't buy supper");
		life::LiveEntry Later = E;
		Later.Id = "embercrest-nightly@" + std::to_string(net::DayOf(Next) - 1);
		Later.FinishedAt = Next - Day;
		S.Life.LiveEntries.push_back(Later);
		const std::string NextKey = town::CheckKey(S, Next);
		Expect(!NextKey.empty() && NextKey != Key && S.Life.Said.count(NextKey) == 0 && town::PaysYourCheck(S, Next) == "big-lou",
			"lou: next time he's in on a Friday it's a new night, and his check again");
		Again = true;
	}
	Expect(Again, "lou: another Friday with Big Lou in his booth");
}

/** Augie names what Unit 114 really brought him (the pawn book's auction lots on his shelf), not a stock pair. */
void PawnAuctionWords()
{
	using namespace ss;
	const double W = When(20, 12.0);
	auto Lot = [](const char* Id, bool Auction, double From) {
		shop::ShelfLot Sh;
		Sh.Lot = std::string("gear:") + Id;
		Sh.PriceCents = shop::GearValue(*gear::Find(Id));
		Sh.From = From;
		Sh.Until = From + shop::PawnShelfDays * Day;
		Sh.Auction = Auction;
		return Sh;
	};
	shop::PawnBook B;
	Expect(shop::AuctionWords(B, W) == "a few pieces out of it", "lot 114: nothing of the unit's on his shelf: a few pieces");
	B.Shelf = {Lot("chair", true, W - Day), Lot("monitor-27", true, W - Day)};
	Expect(shop::AuctionWords(B, W) == "a chair and a 27-inch", "lot 114: the spec's pair when that's what came in, the dearer first");
	B.Shelf = {Lot("ring-light", true, W - Day), Lot("chair", true, W - Day), Lot("tower-mid", true, W - Day), Lot("monitor-24", false, W - Day)};
	Expect(shop::AuctionWords(B, W) == "a desktop, a chair and a ring light", "lot 114: three pieces, the dearest first, and not the hero's own sold monitor");
	B.Shelf = {Lot("mic-xlr", true, W + Day), Lot("headphones", true, W - 30.0 * Day)};
	Expect(shop::AuctionWords(B, W) == "a few pieces out of it", "lot 114: not before they reach his shelf, nor after they've gone");

	// Through the talk and the pawn screen's fallback words: the line he says names the lots.
	Hooks H;
	Session S(H, "shops-lot114");
	S.NewCareer(Someone());
	Place(S, W, 50000);
	auto Says = [&S]() { return town::KeeperLine(*town::ContextFor(S, "augie", "pawn"), "appraise:lot114"); };
	Expect(Says() == "Guy bought a unit on Ninth. Brought me a few pieces out of it. One-fourteen. Mean anything to you?", "lot 114: his line with nothing of the unit's in");
	S.Life.Pawn.Shelf = {Lot("tower-mid", true, W - Day), Lot("webcam-1080", true, W - Day)};
	Expect(Says() == "Guy bought a unit on Ninth. Brought me a desktop and a good webcam. One-fourteen. Mean anything to you?", "lot 114: his line names what came in");
	Expect(Says().find("27-inch") == std::string::npos, "lot 114: no 27-inch he never saw");
}

/** Nothing round-trips for a profit: whatever he sells, at least twice what he'd pay for it. */
void NoProfit()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-no-profit");
	double Now = 0.0;
	PawnSetup(S, H, Now);
	for (const wardrobe::Garment& G : wardrobe::Catalog())
	{
		if (G.Rare)
		{
			S.Life.Closet.Owned.insert(G.Id);
		}
	}
	S.Life.Unlocks.insert("curio:watch");
	bool Fair = true;
	int Lots = 0;
	for (const shop::Lot& L : S.PawnLots())
	{
		if (!L.Refused.empty())
		{
			continue;
		}
		++Lots;
		Fair = Fair && L.ValueCents >= 2 * L.SellCents && L.LoanCents <= L.SellCents && L.ValueCents % 100 == 0;
	}
	for (const shop::Lot& L : S.PawnShelf())
	{
		Fair = Fair && L.ValueCents >= 2 * L.SellCents;
	}
	// Every week's used gear, at V.
	for (int Week = 0; Week < 20; ++Week)
	{
		const std::vector<shop::ShelfLot> Used = shop::UsedStock(Week, 99, ~0ull);
		Fair = Fair && Used.size() >= 4 && Used.size() <= 8;
		for (const shop::ShelfLot& U : Used)
		{
			const gear::Item& I = *gear::Find(U.Lot.substr(5));
			Fair = Fair && shop::Takes(I) && U.PriceCents == shop::GearValue(I) && U.PriceCents >= 2 * shop::SellOf(U.PriceCents) && Near(U.Until - U.From, 7.0 * Day);
		}
	}
	Expect(Fair && Lots >= 10, "no profit: for every lot, the shelf price is at least twice what he pays");
	Expect(FindLot(S.PawnLots(), "garment:outer.leather.rider")->ValueCents == 1800 && FindLot(S.PawnLots(), "curio:watch")->ValueCents == 4500, "no profit: a rare jacket at 40%, the watch");
	uint64_t OnlyMonitors = 0;
	for (size_t I = 0; I < gear::Catalog().size(); ++I)
	{
		OnlyMonitors |= gear::Catalog()[I].Id.rfind("monitor", 0) == 0 ? 1ull << I : 0ull;
	}
	bool Shows = true;
	for (const shop::ShelfLot& U : shop::UsedStock(3, 99, OnlyMonitors))
	{
		Shows = Shows && U.Lot.rfind("gear:monitor", 0) == 0;
	}
	auto Ids = [](const std::vector<shop::ShelfLot>& Used) {
		std::vector<std::string> Out;
		for (const shop::ShelfLot& U : Used)
		{
			Out.push_back(U.Lot + "@" + std::to_string(U.PriceCents));
		}
		return Out;
	};
	Expect(Shows && Ids(shop::UsedStock(3, 99, ~0ull)) == Ids(shop::UsedStock(3, 99, ~0ull)) && Ids(shop::UsedStock(3, 99, ~0ull)) != Ids(shop::UsedStock(4, 99, ~0ull)),
		"used gear: only what his shelf can stand up; the same for the same week, new on Monday");
	Expect(shop::UsedWeek(When(13, 23.9)) + 1 == shop::UsedWeek(When(14, 0.0)), "used gear: restocked Mondays");
	shop::PawnBook B;
	shop::Consign(B, {"ram-32", "mattress", "plant"}, 1000.0, 7);
	Expect(B.Shelf.empty(), "used gear: an auction of things he won't take consigns nothing");
}

// ------------------------------------------------------------------ save lines and migration

void SaveLines()
{
	using namespace ss;
	life::State L;
	L.Closet = wardrobe::SeedFrom(hero::Look());
	L.Closet.Owned.insert("outer.leather.rider");
	L.Closet.Worn[1] = "outer.leather.rider";
	L.Fit.Level = 47.123456;
	L.Fit.LastCounted = 12345.67;
	L.Fit.Workouts = 19;
	L.Fit.Sessions = 3;
	L.Fit.ClearHeadUntil = 12400.5;
	L.Fit.LastDecay = 12350.25;
	L.Groom.CutAt = 9000.5;
	L.Groom.ShaveAt = -1.0e9;
	L.Groom.Cuts = 4;
	L.Visits["barber"] = {5, 9000.5, 14400, "barber/crew", 4, 1};
	L.Visits["diner"] = {2, 9100.0, 2600, "diner/pie\twith a tab", 1, 1};
	shop::Ticket T;
	T.Number = 417;
	T.Lot = "award:812:Event #12: $1,500 No-Limit";
	T.LoanCents = 56000;
	T.TakenAt = 9000.25;
	T.DueAt = 9000.25 + 30.0 * Day;
	T.Renewals = 1;
	L.Pawn.Tickets.push_back(T);
	L.Pawn.Shelf.push_back({"gear:monitor-27", 15900, 9100.0, 9100.0 + 21.0 * Day, false});
	L.Pawn.Shelf.push_back({"gear:chair", 18100, 9200.0, 9200.0 + 21.0 * Day, true});
	L.Pawn.Away.insert("memento:cup");
	L.Pawn.NextTicket = 418;
	L.CoffeeUntil = 9300.75;
	L.RealMealAt = 9250.5;
	std::string Text;
	shop::WriteLines(L, Text);
	life::State Back;
	std::istringstream In(Text);
	std::string Line;
	bool All = true;
	while (std::getline(In, Line))
	{
		All = All && shop::ReadLine(Tabs(Line), Back);
	}
	std::string Again;
	shop::WriteLines(Back, Again);
	Expect(All && Again == Text, "lines: every closet, fit, groom, visit, pawn and meal line round-trips");
	Expect(Back.Closet.Worn[1] == "outer.leather.rider" && Back.Visits["diner"].Usual == "diner/pie\twith a tab" && Back.Pawn.Tickets.size() == 1 &&
			   Back.Pawn.Tickets[0].Lot == T.Lot && Back.Pawn.Shelf[1].Auction && Back.Pawn.Away.count("memento:cup") > 0 && Back.Pawn.NextTicket == 418 && Near(Back.CoffeeUntil, 9300.75) &&
			   Back.Fit.Sessions == 3,
		"lines: and say what they said");
	Expect(Text.find("pawn\tticket\t417\t") != std::string::npos && Text.find("meal\t9300.75\t9250.50") != std::string::npos && Text.find("closet\tseeded") != std::string::npos,
		"lines: the keys of §7.3");
	Expect(!shop::ReadLine(Tabs("ticket\tsatellite\t1"), Back) && !shop::ReadLine(Tabs("town\tlonnie\t1\t1\t3\t0\t0"), Back), "lines: not another's keys");

	// Through a whole save.
	Hooks H;
	Session S(H, "shops-lines");
	S.NewCareer(Someone());
	S.Life.Fit = L.Fit;
	S.Life.Groom = L.Groom;
	S.Life.Visits = L.Visits;
	S.Life.Pawn = L.Pawn;
	S.Life.CoffeeUntil = L.CoffeeUntil;
	S.Life.RealMealAt = L.RealMealAt;
	S.Save();
	SaveData D;
	Expect(SaveData::Parse(H.Last.Serialize(), D) && D.Serialize() == H.Last.Serialize(), "lines: a whole save round-trips byte for byte");
}

void MigrationFixtures()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-fixtures");
	S.NewCareer(Someone());
	S.Save();
	const std::string Modern = H.Last.Serialize();
	const std::string Old = WithoutNorthside(Modern);
	{
		// A v1 save (the casino was the Riverside): it migrates too.
		std::string V1 = Old;
		V1.replace(0, V1.find('\n'), "shortstack.nightone.v1");
		SaveData D;
		// (Night one is day 1, so a cut a week back is before the clock's day 0: on record, though negative.)
		Expect(SaveData::Parse(V1, D) && D.Life.Closet.Seeded && D.Life.Closet.Worn[1] == "outer.bomber.charcoal" &&
				   Near(D.Life.Groom.CutAt, Day * static_cast<double>(net::NightOneDay) + D.ClockMinutes - 7.0 * Day, 0.02),
			"fixtures: a v1 save migrates");
	}
	{
		// A seeded closet that's empty stays empty.
		SaveData D;
		Expect(SaveData::Parse(Old + "closet\tseeded\n", D) && D.Life.Closet.Seeded && D.Life.Closet.Owned.empty(), "fixtures: a seeded empty closet stays empty");
		SaveData Again;
		Expect(SaveData::Parse(D.Serialize(), Again) && Again.Serialize() == D.Serialize() && Again.Life.Closet.Owned.empty(), "fixtures: and round-trips");
	}
	{
		// Garments this build doesn't know are dropped, and a slot wearing one is bare.
		SaveData D;
		Expect(SaveData::Parse(Old + "closet\tseeded\ncloset\town\touter.cape.velvet\ncloset\town\ttop.tee.white\ncloset\twear\t1\touter.cape.velvet\ncloset\twear\t0\ttop.tee.white\n", D),
			"fixtures: unknown garments parse");
		Expect(D.Life.Closet.Owned.size() == 1 && D.Life.Closet.Worn[1].empty() && D.Life.Closet.Worn[0] == "top.tee.white", "fixtures: unknown garment ids are dropped");
	}
	{
		// An old member lands at 60 (Migration's fixture); an old non-member at 0.
		SaveData D;
		Expect(SaveData::Parse(Old, D) && D.Life.Fit.Level == 0.0 && D.Life.Fit.LastCounted < -1.0e8, "fixtures: no membership, no fitness");
	}
	{
		// A said line with no day (people-core's line): the save still loads.
		SaveData D;
		Expect(SaveData::Parse(Old + "said\tgarage-rumour\n", D), "fixtures: a said line with no day reads");
	}
	{
		// Evicted with the unit already auctioned before this update: no lots on Augie's shelf.
		std::string Text = Old;
		const size_t At = Text.find("life\tevicted\t");
		Text.replace(At, Text.find('\n', At) - At, "life\tevicted\t1\t5000.00\t0\t0.00\t1");
		const size_t Rent = Text.find("life\trent\t");
		Text.replace(Rent, Text.find('\n', Rent) - Rent, "life\trent\t3\t260000\t7200.00\t0");
		SaveData D;
		Expect(SaveData::Parse(Text, D) && D.Life.RentStage == life::Rent::Evicted && D.Life.Auctions == 1 && D.Life.Pawn.Shelf.empty() && D.Life.Pawn.Tickets.empty(),
			"fixtures: an evicted career already auctioned has no lots");
	}
}

// ------------------------------------------------------------------ the 14

void Bus()
{
	using namespace ss;
	Hooks H;
	H.CanGo = true;
	Session S(H, "shops-bus");
	// The first event the desk takes registrations for on day 8.
	const std::vector<live::Occurrence> Os = live::Occurrences(8);
	Expect(!Os.empty(), "bus: the Embercrest runs on day 8");
	if (Os.empty())
	{
		return;
	}
	const live::Occurrence O = Os.front();
	Place(S, O.DeskOpens - 120.0, 100000);
	double Now = 0.0;
	S.Update(Now);
	Expect(S.BoardBus("") == "The 14 \xC2\xB7 Embercrest. Register for an event first (Burner).", "bus: no entry, no ride");
	life::LiveEntry E;
	E.Id = O.Id;
	E.Name = O.T->Name;
	E.State = life::LiveEntry::Registered;
	E.PaidCents = O.T->BuyInCents;
	E.RegisteredAt = S.WorldMinutes();
	E.Entrants = O.Field;
	S.Life.LiveEntries.push_back(E);
	Expect(S.BoardBus("").rfind("The desk opens at", 0) == 0 && H.Went.empty(), "bus: too early for the desk");
	Jump(S, Now, O.DeskOpens + 10.0);
	const Chips Bank = S.BankrollCents;
	Expect(S.BoardBus("").empty() && H.Went == O.Id && S.BankrollCents == Bank - live::BusFareCents, "bus: boards for the registered event, the fare paid");
}

// ------------------------------------------------------------------ the month

/**
 * §5.7's pacing sim: a month of upkeep (a cut every three weeks, the gym three times a week, Ruby's twice a week, one
 * thrift outfit) on ShiftLink's wages from the $2.37 start, the first month's rent behind them: the next month's rent
 * is collected when it comes due, and nothing is ever declined. The night cashier's shift every night, and the Wash &
 * Fold's attendant on Sundays: the "2 to 4 more shifts a month" the upkeep costs.
 */
void PacingSim()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-pacing");
	// A background with no money in it (the line cook's shifts pay 20% more; the newcomer brings $40).
	hero::Character Who = Someone();
	Who.Story = hero::Background::Bouncer;
	S.NewCareer(Who);
	const double Start = S.WorldMinutes(); // 2:07 AM, Tuesday, October 6
	S.Life.RentStage = life::Rent::Paid;
	S.Life.RentsPaid = 1;
	S.Life.RentDueCents = life::MonthlyRentCents;
	S.Life.RentDeadline = (std::floor(Start / Day) + 31.0) * Day; // thirty days on, at midnight
	S.BankrollCents = 237;
	S.Life.Ledger.clear();
	double Now = 0.0;
	S.Update(Now);
	int Declines = 0;
	int Workouts = 0;
	int Rubys = 0;
	int Cuts = 0;
	int Outfits = 0;
	int Shifts = 0;
	Chips Upkeep = 0;
	Chips Food = 0;
	auto Spend = [&](const std::string& Why, Chips Before, Chips& Into) {
		if (Why == "Card declined.")
		{
			++Declines;
		}
		if (Why.empty())
		{
			Into += Before - S.BankrollCents;
		}
		return Why.empty();
	};
	// A player who knows what's in the bank: the upkeep when it's covered, with five dollars to spare for food.
	auto Covered = [&](const Session::ShopOrder& O) { return S.BankrollCents >= S.ShopTotal(O) + 500; };
	auto Feed = [&]() {
		// The Lucky Penny: noodle cups while hungry, the tap and a bottle of water while thirsty.
		for (int K = 0; K < 4 && S.Life.Hunger > 35.0; ++K)
		{
			store::Basket B;
			B.Add("oodle-cup");
			const Chips Before = S.BankrollCents;
			if (S.Checkout(B).empty() && S.Consume("oodle-cup").empty())
			{
				Food += Before - S.BankrollCents;
			}
			else
			{
				break;
			}
		}
		(void)S.DrinkTapWater();
		if (S.Life.Thirst > 55.0)
		{
			store::Basket B;
			B.Add("cascade");
			const Chips Before = S.BankrollCents;
			if (S.Checkout(B).empty() && S.Consume("cascade").empty())
			{
				Food += Before - S.BankrollCents;
			}
		}
	};
	auto Work = [&](const char* Job) {
		if (S.StartActivity(Job).empty())
		{
			Finish(S, Now);
			++Shifts;
		}
	};
	// Night one: straight to bed (the first shift is tomorrow night).
	Jump(S, Now, When(1, 3.0));
	Expect(S.StartActivity("sleep").empty(), "pacing: to bed on night one");
	Finish(S, Now);
	int Cut = 0;
	const int ThriftDay = 14;
	for (int D = 1; D <= 30; ++D)
	{
		const int Today = net::DayOf(Start) + D - 1;
		const int Wd = net::Weekday(Today);
		Jump(S, Now, When(Today, 11.0));
		Feed();
		if (Wd == 6)
		{
			Work("washfold"); // Sunday: the Wash & Fold, eleven to five
		}
		Jump(S, Now, std::max(S.WorldMinutes(), When(Today, 12.0)));
		const Session::ShopOrder Rubys2 = Order("diner", {"diner/two-eggs", "diner/coffee"}, 18);
		if ((Wd == 1 || Wd == 5) && Covered(Rubys2))
		{
			// Ruby's: two eggs and a coffee, 18% ($12.20).
			const Chips Before = S.BankrollCents;
			if (Spend(S.ShopService(Rubys2), Before, Upkeep))
			{
				Finish(S, Now);
				++Rubys;
			}
		}
		const Session::ShopOrder Trim = Order("barber", {"barber/crop"}, 20);
		if (Cut < 2 && D >= 7 + Cut * 21 && Covered(Trim) && S.ShopOpenFor("barber"))
		{
			// Lonnie, every three weeks: the same cut, kept up, and 20% ($28.80).
			const Chips Before = S.BankrollCents;
			if (Spend(S.ShopService(Trim), Before, Upkeep))
			{
				Finish(S, Now);
				++Cuts;
				++Cut;
			}
		}
		if (D == ThriftDay)
		{
			// One outfit at the thrift: this week's first jacket and first trousers.
			std::string Jacket;
			std::string Pants;
			for (const std::string& Id : wardrobe::ThriftStock(wardrobe::ThriftWeek(S.WorldMinutes()), S.WorldSeed(), ~0ull))
			{
				const wardrobe::Garment& G = *wardrobe::Find(Id);
				Jacket = Jacket.empty() && G.Where == wardrobe::Slot::Outer && !G.Rare ? Id : Jacket;
				Pants = Pants.empty() && G.Where == wardrobe::Slot::Bottom ? Id : Pants;
			}
			const Chips Before = S.BankrollCents;
			if (Covered(Order("thrift", {}, 0, {Jacket, Pants})) && Spend(S.ShopPay(Order("thrift", {}, 0, {Jacket, Pants})), Before, Upkeep))
			{
				++Outfits;
				(void)S.Wear(Jacket);
				(void)S.Wear(Pants);
			}
		}
		Jump(S, Now, std::max(S.WorldMinutes(), When(Today, 17.0)));
		if (Wd == 0 || Wd == 2 || Wd == 4)
		{
			// IronHouse: the membership at the desk the first time, then the machines.
			if (!S.Owns("gym") && Covered(Order("gym", {"gym/membership"})))
			{
				const Chips Before = S.BankrollCents;
				(void)Spend(S.ShopPay(Order("gym", {"gym/membership"})), Before, Upkeep);
			}
			static const char* const Machines[] = {"gym/treadmill", "gym/weights", "gym/bag"};
			if (S.Owns("gym") && S.ShopService(Order("gym", {Machines[Workouts % 3]})).empty())
			{
				Finish(S, Now);
				++Workouts;
			}
		}
		Jump(S, Now, std::max(S.WorldMinutes(), When(Today, 18.5)));
		Feed();
		Jump(S, Now, When(Today, 21.0));
		Work("quikstop");
		Expect(S.StartActivity("sleep").empty(), "pacing: to bed after the shift");
		Finish(S, Now);
	}
	// The deadline: midnight, thirty days on. The landlord comes up the stairs.
	Jump(S, Now, S.Life.RentDeadline + 30.0);
	const bool Collected = S.Life.RentsPaid == 2 && S.Life.RentStage == life::Rent::Paid && !S.Evicted();
	std::printf("pacing: %d shifts, %d workouts, %d at Ruby's, %d cuts, %d outfit; upkeep %s, food %s; after rent %s in the bank\n", Shifts, Workouts, Rubys, Cuts, Outfits, Money(Upkeep).c_str(),
		Money(Food).c_str(), Money(S.BankrollCents).c_str());
	Expect(Workouts >= 11 && Rubys >= 8 && Cuts == 2 && Outfits == 1, "pacing: the month's upkeep, all of it");
	Expect(Declines == 0, "pacing: nothing declined");
	Expect(Collected, "pacing: rent stays payable on ShiftLink's shifts with the upkeep");
	Expect(Upkeep >= 15000 && Upkeep <= 25000, "pacing: about what §5.7 says the upkeep costs");
}
} // namespace shops_test

namespace shops_test
{
/** The section running, named if it crashes (stdout is unbuffered, so every failure before it is on the page). */
const char* Section = "";

void Crashed(int /*Signal*/)
{
	std::printf("shops_test: crashed in %s\n", Section);
	std::fflush(stdout);
	std::_Exit(3);
}

/** shops_test [Section...]: only those sections (all of them with none named). */
std::set<std::string> Only;

void Run(const char* Name, void (*Fn)())
{
	if (!Only.empty() && Only.count(Name) == 0)
	{
		return;
	}
	Section = Name;
	Fn();
}
} // namespace shops_test

namespace shops_test
{
/**
 * The sponsors at the shops (next_segment_spec §3.6, core-sponsor's additions): the receipt's credit line; Fifth St.
 * Barbers' cut rung up at $0 "on the house" once a fortnight; IronHouse paying the membership (its renewal skipped); the
 * pawn refusing a brand's gear; the closet's patches round-tripping as sponsor\tpatch lines.
 */
void Sponsors()
{
	// The Lucky Penny's credit pays first, and the receipt says so.
	{
		Hooks H;
		ss::Session S(H, "shops-sponsor-credit");
		Place(S, When(3, 11.0), 50000);
		double Now = 0.0;
		Jump(S, Now, When(3, 11.0));
		S.DebugSponsor("sign luckypenny");
		S.Life.Sponsors.StoreCreditCents = 200;
		ss::store::Basket B;
		B.Add("volt-rush");
		const ss::Chips Bank = S.BankrollCents;
		Expect(S.Checkout(B).empty(), "sponsors: checkout with credit");
		Expect(S.BankrollCents == Bank - (B.Total() - 200) && S.Life.Sponsors.StoreCreditCents == 0, "sponsors: the credit first, the card the rest");
		Expect(S.Life.Ledger.front().Label.find("($2.00 store credit)") != std::string::npos && S.Life.Ledger.front().Amount == -(B.Total() - 200), "sponsors: the receipt's credit line");
	}
	// A cut on the house every fourteen days; the tip is still the player's.
	{
		Hooks H;
		ss::Session S(H, "shops-sponsor-cut");
		Place(S, When(3, 11.0), 50000);
		double Now = 0.0;
		Jump(S, Now, When(3, 11.0));
		S.DebugSponsor("sign barber");
		const ss::Chips Bank = S.BankrollCents;
		Expect(S.ShopTotal(Order("barber", {"barber/lineup"}, 20)) == 240, "sponsors: a covered line-up costs only the tip");
		Expect(S.ShopService(Order("barber", {"barber/lineup"}, 20)).empty(), "sponsors: the line-up, covered");
		Finish(S, Now);
		Expect(S.BankrollCents == Bank - 240, "sponsors: $0 for the cut, the tip paid");
		bool House = false;
		for (const ss::life::LedgerEntry& E : S.Life.Ledger)
		{
			House = House || (E.Label.find("(cut on the house)") != std::string::npos && E.Amount == -240);
		}
		Expect(House, "sponsors: rung up \"on the house\"");
		Expect(S.ShopTotal(Order("barber", {"barber/lineup"})) == 1200, "sponsors: the next one in the fortnight is paid");
		Jump(S, Now, When(18, 11.0));
		Expect(S.ShopTotal(Order("barber", {"barber/lineup"})) == 0, "sponsors: a new fortnight, a new cut on the house");
	}
	// IronHouse pays the membership: its renewal is skipped while the deal runs.
	{
		Hooks H;
		ss::Session S(H, "shops-sponsor-gym");
		Place(S, When(3, 11.0), 50000);
		double Now = 0.0;
		Jump(S, Now, When(3, 11.0));
		S.DebugSponsor("sign ironhouse");
		Expect(S.Owns("gym"), "sponsors: the membership comes with the deal");
		const ss::Chips Bank = S.BankrollCents;
		Jump(S, Now, When(34, 12.0));
		bool Billed = false;
		for (const ss::life::LedgerEntry& E : S.Life.Ledger)
		{
			Billed = Billed || (E.Kind == 5 && E.Amount == -3499);
		}
		Expect(!Billed && S.Owns("gym") && S.BankrollCents >= Bank - 1000, "sponsors: the gym renewal skipped");
	}
	// The closet's patches round-trip.
	{
		Hooks H;
		ss::Session S(H, "shops-sponsor-patch");
		Place(S, When(3, 11.0), 50000);
		double Now = 0.0;
		Jump(S, Now, When(3, 11.0));
		S.DebugSponsor("sign luckypenny");
		S.DebugSponsor("sign barber");
		Expect(S.SewPatch(2, "luckypenny").empty() && S.SewPatch(1, "barber").empty(), "sponsors: patches sewn");
		S.Save();
		const std::string Text = H.Last.Serialize();
		Expect(Text.find("sponsor\tpatch\t1\tbarber\n") != std::string::npos && Text.find("sponsor\tpatch\t2\tluckypenny\n") != std::string::npos, "sponsors: sponsor\\tpatch lines");
		ss::SaveData D;
		Expect(ss::SaveData::Parse(Text, D) && D.Life.Closet.Patches[1] == "barber" && D.Life.Closet.Patches[2] == "luckypenny" && D.Life.Closet.Patches[0].empty() &&
				   D.Serialize() == Text,
			"sponsors: the patches round-trip");
	}
}
} // namespace shops_test

int main(int Argc, char** Argv)
{
	using namespace shops_test;
	for (int I = 1; I < Argc; ++I)
	{
		Only.insert(Argv[I]);
	}
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	std::signal(SIGSEGV, Crashed);
	std::signal(SIGABRT, Crashed);
	Run("Contracts", Contracts);
	Run("Saves", Saves);
	Run("Migration", Migration);
	Run("StoreVisits", StoreVisits);
	Run("ClerkRoster", ClerkRoster);
	Run("RealMeal", RealMeal);
	Run("Coffee", Coffee);
	Run("ShopSkip", ShopSkip);
	Run("LedgerNine", LedgerNine);
	Run("OutfitNone", OutfitNone);
	Run("Catalogs", Catalogs);
	Run("Hours", Hours);
	Run("TaxAndTips", TaxAndTips);
	Run("DeclinedPath", DeclinedPath);
	Run("Services", Services);
	Run("FitnessDays", FitnessDays);
	Run("Membership", Membership);
	Run("BarberLength", BarberLength);
	Run("Thrift", Thrift);
	Run("Closet", Closet);
	Run("Eviction", Eviction);
	Run("Waits", Waits);
	Run("GymDesk", GymDesk);
	Run("LedgerInvariant", LedgerInvariant);
	Run("PawnValues", PawnValues);
	Run("PawnCounter", PawnCounter);
	Run("PawnForfeit", PawnForfeit);
	Run("PawnRenewStoreAuction", PawnRenewStoreAuction);
	Run("PawnAuctionWords", PawnAuctionWords);
	Run("BigLou", BigLou);
	Run("NoProfit", NoProfit);
	Run("SaveLines", SaveLines);
	Run("MigrationFixtures", MigrationFixtures);
	Run("Bus", Bus);
	Run("PacingSim", PacingSim);
	Run("Sponsors", Sponsors);
	std::printf("shops_test: %d checks, %d failed\n", Checks, Failures);
	return Failures == 0 ? 0 : 1;
}
