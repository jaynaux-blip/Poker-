// Northside's shops, wardrobe, fitness and pawn (the realism update's spec, §7.8's shops_test; core-shops).
//
// Phase A: the contract. Every declaration in Shop.h, Wardrobe.h, Body.h and the Session's Northside block links and
// runs (Town.h, Talk.h and Weather.h too, until people-core's town_test covers them), and the seam's hooks in
// Session.cpp behave: the store's visits, a real meal quieting Mom, Ruby's coffee on the awake drain, a "shop:" skip's
// energy while evicted, ledger kind 9 in the Bank, PawnHolds, Outfit 6 "None", saves that still round-trip, and a
// migration that runs once (an old save's lines kept, a new career's defaults never mistaken for an old save).
// Everything checked here holds with Phase B's bodies too; Phase B adds the rest of §7.8's list.
#include "ShortStack/Game/Body.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Life.h"
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

#include <cmath>
#include <cstdio>
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

bool Near(double A, double B)
{
	return std::fabs(A - B) < 1e-9;
}

struct Hooks : ss::SessionHooks
{
	std::vector<std::pair<std::string, std::string>> Inbox;
	ss::SaveData Last;
	int Saves = 0;
	void Text(const std::string& From, const std::string& Body) override { Inbox.emplace_back(From, Body); }
	void Save(const ss::SaveData& D) override
	{
		++Saves;
		Last = D;
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

	// Body.h: an unfit non-member feels nothing (Phase B's fitness at 0 changes nothing either).
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

	// Town.h, Talk.h, Weather.h (people-core's; linked here until town_test has them).
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
		// The same career as a save from before Northside: its lines dropped.
		std::istringstream In(H.Last.Serialize());
		std::string Old;
		std::string Line;
		while (std::getline(In, Line))
		{
			if (!NorthsideLine(Line))
			{
				Old += Line + "\n";
			}
		}
		SaveData D;
		Expect(SaveData::Parse(Old, D), "migration: an old save loads");
		const std::string First = D.Serialize();
		bool Kept = true;
		std::istringstream OldIn(Old);
		while (std::getline(OldIn, Line))
		{
			Kept = Kept && First.find(Line + "\n") != std::string::npos;
		}
		Expect(Kept, "migration: every line of an old save is kept");
		SaveData Again;
		Expect(SaveData::Parse(First, Again) && Again.Serialize() == First, "migration: it runs once (the migrated save reads back the same)");
		Expect(Again.Gear.count("gym") > 0 && D.Life.Fit.Level == Again.Life.Fit.Level, "migration: an old member's fitness is settled once");
	}
	{
		Hooks H;
		Session S(H, "shops-unseeded");
		S.Save();
		SaveData D;
		Expect(SaveData::Parse(H.Last.Serialize(), D) && D.Serialize() == H.Last.Serialize(), "migration: a career that never met the creator round-trips");
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
	Expect(!S.ClerkSays(store::Basket()).empty(), "store: the clerk still talks");
	Expect(S.Checkout(B).empty() && S.Life.Visits["store"].Count == 2 && S.Life.Visits["store"].SpentCents == 2 * Total, "store: and the next");
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

/** Northside Pawn & Loan holds what's on a ticket and what's gone for good. */
void Pawn()
{
	using namespace ss;
	Hooks H;
	Session S(H, "shops-pawn");
	Expect(!S.PawnHolds("memento:cup"), "pawn: a fresh book holds nothing");
	Expect(S.Life.Pawn.NextTicket == 412 && S.Life.Pawn.Tickets.empty() && S.Life.Pawn.Shelf.empty(), "pawn: an empty book");
	shop::Ticket T;
	T.Number = 412;
	T.Lot = "memento:cup";
	S.Life.Pawn.Tickets.push_back(T);
	Expect(S.PawnHolds("memento:cup") && !S.PawnHolds("gear:monitor-24"), "pawn: the cup's in the cage");
	S.Life.Pawn.Tickets.clear();
	// Forfeited: in his case for 21 days, where it can be bought back.
	shop::ShelfLot Case;
	Case.Lot = "memento:cup";
	Case.PriceCents = 6000;
	Case.From = S.WorldMinutes();
	Case.Until = Case.From + shop::PawnShelfDays * net::MinutesPerDay;
	S.Life.Pawn.Shelf.push_back(Case);
	Expect(S.PawnHolds("memento:cup"), "pawn: a forfeited cup is in his case, not on the sill");
	S.Life.Pawn.Shelf.clear();
	Expect(!S.PawnHolds("memento:cup"), "pawn: bought back, it's home");
	S.Life.Pawn.Away.insert("memento:cup");
	Expect(S.PawnHolds("memento:cup"), "pawn: the cup's gone");
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
} // namespace shops_test

int main()
{
	using namespace shops_test;
	Contracts();
	Saves();
	Migration();
	StoreVisits();
	RealMeal();
	Coffee();
	ShopSkip();
	LedgerNine();
	Pawn();
	OutfitNone();
	std::printf("shops_test: %d checks, %d failed\n", Checks, Failures);
	return Failures == 0 ? 0 : 1;
}
