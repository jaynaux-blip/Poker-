// night_one_session: core-stake's additions (next_segment_spec §7.6, §8.3), compiled into session_test and called
// from main() in session_main.cpp (the shared kit is SessionSeg.h): Lou's stake through GoToLive shows the exact three
// lines; a cancelled event returns Lou's money; online buy-ins route to the roll; ResumeLive keeps the funding; the
// calendar's step texts and mails what it should once; the debug commands; the save keeps it all. Update 2 (core-deals,
// G1): Mrs. Park's gate in a fresh world, where the online rank is unknown.
#include "SessionSeg.h"

#include "ShortStack/Game/Coach.h"
#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Stake.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

namespace session_test
{
namespace stake_seg
{
namespace life = ss::life;
namespace live = ss::live;
namespace stake = ss::stake;
const double Day = 1440.0;

struct OutHooks : Hooks
{
	std::string Went;
	bool GoOut(const std::string& Id, ss::Chips) override
	{
		Went = Id;
		return true;
	}
};

const life::LedgerEntry* Line(const life::State& L, const std::string& Part)
{
	for (const life::LedgerEntry& X : L.Ledger)
	{
		if (X.Label.find(Part) != std::string::npos)
		{
			return &X;
		}
	}
	return nullptr;
}

/** A session at Wednesday's Nightly desk (world day 9, 5:30 PM), with money and energy. */
void AtTheDesk(ss::Session& S, double& Now, ss::Chips Bank)
{
	S.CurrentScreen = ss::Screen::Lobby;
	Now = Wait(S, 0.0, 0.2);
	S.LobbyMinutes = 9.0 * Day - Day + 17.0 * 60.0 + 30.0;
	Now = Wait(S, Now, 0.2);
	S.Life.Energy = 60.0;
	S.BankrollCents = Bank;
}

void LousStake()
{
	OutHooks H;
	ss::Session S(H, "stake-lou");
	double Now = 0.0;
	AtTheDesk(S, Now, 30000);
	const live::Occurrence N = live::FindOccurrence("embercrest-nightly@9");
	// Lou's offer (as he'd make it), accepted on the Backing page.
	const std::string Made = S.DebugStake("offer lou");
	Expect(Made.rfind("Offer lou:" + N.Id, 0) == 0 && H.Got(0, "Big Lou", "I'm putting you in"), "stake: Lou's offer arrives by text");
	const size_t Texts = H.Inbox.size();
	std::string Id;
	for (const stake::Offer& O : S.Life.Stakes.Offers)
	{
		Id = O.Backer == "Big Lou" ? O.Id : Id;
	}
	Expect(S.AcceptStake(Id).empty() && H.Inbox.size() == Texts + 1, "stake: accepted, and Lou says so");
	const std::vector<std::pair<life::Funding, std::string>> Opts = S.FundingOptions(N.Id);
	Expect(Opts.size() == 2 && Opts[0].first.From == life::Paid::Backer && Opts[0].second.empty() && live::FundingLine(Opts[0].first, N.T->Short) == "Big Lou is putting you in the Nightly.",
		"stake: Big Lou is putting you in the Nightly");
	Expect(S.LifeContext().LiveFunding && S.LifeContext().LiveFunding(N.Id).From == life::Paid::Backer, "stake: the Burner's row sees who pays");
	const size_t Lines = S.Life.Ledger.size();
	Expect(S.GoToLive(N.Id).empty() && H.Went == N.Id, "stake: GoToLive on Lou's money");
	// The exact three lines (§9.1 step 3), then the bus.
	const std::vector<life::LedgerEntry>& L = S.Life.Ledger;
	Expect(L.size() == Lines + 4 && L[3].Label == "Big Lou: stake, Embercrest Nightly" && L[3].Amount == 12000 && L[3].Kind == life::LedgerBacking &&
			   L[2].Label == "Embercrest Nightly $120: buy-in" && L[2].Amount == -10500 && L[1].Label == "Embercrest Nightly $120: entry fee" && L[1].Amount == -1500 &&
			   L[0].Amount == -live::BusFareCents,
		"stake: Big Lou: stake +$120.00, the buy-in -$105.00, the fee -$15.00 (and the bus)");
	Expect(S.BankrollCents == 30000 - live::BusFareCents, "stake: the player paid only the bus");
	const life::LiveEntry* E = live::EntryFor(S.Life, N.Id);
	Expect(E && E->Fund.From == life::Paid::Backer && E->Fund.Who == "Big Lou" && E->Fund.Permille == 1000, "stake: the entry knows who paid");
	// The world hears it once (World::HeroMoney): Lou's $120 out of his roll, and he remembers putting the player in.
	{
		const int Lou = S.Living().Find("Big Lou");
		const ss::Chips Roll = Lou >= 0 ? S.Living().Get(Lou)->Bankroll : 0;
		Now = Wait(S, Now, 0.1);
		const ss::world::Bond* Bd = Lou >= 0 ? S.Living().BondWith(Lou) : nullptr;
		const bool Remembers = Bd && std::any_of(Bd->Memories.begin(), Bd->Memories.end(), [](const ss::world::Memory& M) { return M.Kind == ss::world::MemoryKind::Staked; });
		Expect(Lou >= 0 && Remembers && S.Living().Get(Lou)->Bankroll == Roll - 12000, "stake: the world remembers Lou putting the player in, and his roll paid for it");
	}
	// The save keeps the funding; a resumed night still knows it.
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Back), "stake: the save parses");
	const life::LiveEntry* Kept = live::EntryFor(Back.Life, N.Id);
	Expect(Kept && Kept->Fund.From == life::Paid::Backer && Kept->Fund.Ref == E->Fund.Ref && Back.Life.Stakes.Deals.size() == 1 &&
			   Back.Life.Stakes.Deals[0].State == stake::DealPlaying,
		"stake: the save keeps the funding and the deal");
	// The cage: 13th of 88 for $236. Lou's $178 and the player's $58.
	ss::Chips Bank = S.BankrollCents;
	live::Settle(Bank, S.Life, N.Id, 13, 88, 23600, N.Start + 300.0);
	Expect(Bank == S.BankrollCents + 5800 && Line(S.Life, "Big Lou: his share") && Line(S.Life, "Big Lou: his share")->Amount == -17800, "stake: Lou's $178, the player's $58");
	S.BankrollCents = Bank;
	// The calendar: Lou's text after the cage, once.
	const size_t Before = H.Inbox.size();
	S.StakeStep(N.Start + 300.0, N.Start + 330.0);
	S.StakeStep(N.Start + 330.0, N.Start + 360.0);
	Expect(H.Inbox.size() == Before + 1 && H.Got(Before, "Big Lou", "$58.00"), "stake: Lou texts the result once");
}

void CancelledStake()
{
	OutHooks H;
	ss::Session S(H, "stake-cancel");
	double Now = 0.0;
	AtTheDesk(S, Now, 30000);
	const live::Occurrence N = live::FindOccurrence("embercrest-nightly@9");
	S.DebugStake("offer lou");
	for (const stake::Offer& O : std::vector<stake::Offer>(S.Life.Stakes.Offers))
	{
		S.AcceptStake(O.Id);
	}
	S.GoToLive(N.Id);
	const ss::Chips Bank = S.BankrollCents;
	ss::Chips B = Bank;
	Expect(live::Refund(B, S.Life, N.Id, N.Start, "not enough players"), "stake: the event is called off");
	const life::LedgerEntry* Back = Line(S.Life, "Big Lou: stake back");
	Expect(Back && Back->Amount == -12000 && B == Bank, "stake: Lou's $120 goes back to him; the player's money is as it was");
	Expect(S.Life.Stakes.Deals[0].State == stake::DealRefunded && stake::FundFor(S.Life.Stakes, N.Id, 12000, true, N.Start).From == life::Paid::Own,
		"stake: a refunded stake isn't used (Lou can offer again)");
}

void OnlineRoll()
{
	Hooks H;
	ss::Session S(H, "stake-roll");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Wait(S, 0.0, 0.2);
	S.BankrollCents = 500;
	const std::string Granted = S.DebugStake("grant lanternrow");
	Expect(Granted.rfind("Granted", 0) == 0 && stake::ActivePackage(S.Life.Stakes) && stake::StakedRoll(S.Life.Stakes) == 220000, "stake: ss.Stake.Grant lanternrow: a $2,200 roll");
	bool Welcome = false;
	for (const life::Letter& L : S.Life.Inbox)
	{
		Welcome = Welcome || (L.Source == "stake" && L.Subject == "Your roll is live");
	}
	Expect(Welcome, "stake: the welcome letter in the Mail app's inbox");
	// The next $22-or-less event: the roll pays, the bankroll doesn't.
	const double World = S.WorldMinutes();
	ss::LobbyEvent Pick;
	bool Found = false;
	for (const ss::net::EventInstance& I : ss::net::Shared().Window(World - 120.0, World + 60.0))
	{
		const ss::LobbyEvent L = ss::net::Shared().Listing(I, nullptr, S.Unlocks());
		if (L.Joinable && L.Spec.Entrants <= 400 && L.BuyInCents > 0 && L.BuyInCents <= 1100)
		{
			Pick = L;
			Found = true;
			break;
		}
	}
	if (!Found)
	{
		Expect(false, "stake: an online event under the cap to play");
		return;
	}
	const std::vector<std::pair<life::Funding, std::string>> Opts = S.FundingOptions(Pick.Spec.Id);
	Expect(!Opts.empty() && Opts[0].first.From == life::Paid::Package && Opts[0].second.empty(), "stake: the online event's options start with the roll");
	S.BankrollCents = 100000; // (CanAfford reads the bankroll: see for_other_tracks)
	const ss::Chips Bank = S.BankrollCents;
	const ss::Chips Roll = stake::StakedRoll(S.Life.Stakes);
	S.RegisterEvent(Pick);
	Expect(S.BankrollCents == Bank && stake::StakedRoll(S.Life.Stakes) == Roll - Pick.BuyInCents && S.Life.Stakes.Funds.size() == 1,
		"stake: online buy-ins route to the roll, not the bankroll");
	S.CurrentPace = ss::Pace::Sprint;
	int Decisions = 0;
	ss::Rng Choice("stake-roll-play");
	Now = PlayOut(S, Choice, Now, Decisions);
	Expect(S.BankrollCents == Bank, "stake: the prize, if any, goes to the roll too");
	Expect(S.Life.Stakes.Funds[0].Settled, "stake: the bullet settled once");
}

void Calendar()
{
	// Mrs. Park's piece paid at the due time, keeping the rent reserve; Lantern Row's chop day mails the chop and Fred's review.
	Hooks H;
	ss::Session S(H, "stake-calendar");
	S.CurrentScreen = ss::Screen::Lobby;
	Wait(S, 0.0, 0.2);
	const double World = S.WorldMinutes();
	stake::Owed O;
	O.Id = "owe:cal";
	O.Who = "Mrs. Park";
	O.Label = "10% of your Sunday, 4th";
	O.Cents = 11900;
	O.Since = World;
	O.Due = World + 72.0 * 60.0;
	O.EntryId = "embercrest@6";
	S.Life.Stakes.Owes.push_back(O);
	S.BankrollCents = 200000;
	S.Life.RentStage = life::Rent::Paid;
	const size_t Texts = H.Inbox.size();
	S.DebugStake("advance 80");
	Expect(S.Life.Stakes.Owes.empty() || S.Life.Stakes.Owes[0].State == stake::OwedPaid, "stake: auto-paid at the due time");
	Expect(H.Got(Texts, "Mrs. Park", "Received"), "stake: Mrs. Park thanks you");
	const std::string Described = S.DebugStake("describe");
	Expect(Described.find("bankroll") == 0, "stake: ss.Stake.Describe");
	S.DebugStake("grant lanternrow");
	S.DebugStake("advance 750");
	bool Chop = false;
	bool Review = false;
	for (const life::Letter& L : S.Life.Inbox)
	{
		Chop = Chop || (L.Source == "stake" && (L.Subject == "No chop this month" || L.Subject.rfind("Chop:", 0) == 0));
		Review = Review || (L.From == "FoldEquityFred" && L.Body.find("Play me two hundred hands") == 0);
	}
	Expect(Chop && Review, "stake: chop day mails the period's end and Fred's review (with nothing graded, it says so)");
	// The marks for the second screen.
	Expect(!S.Marks(S.WorldMinutes(), S.WorldMinutes() + 40.0 * Day).empty(), "stake: the chop day is on the calendar");
	// The whole thing survives a save.
	ss::SaveData Back;
	Expect(ss::SaveData::Parse(H.Last.Serialize(), Back) && Back.Serialize() == H.Last.Serialize() && Back.Life.Stakes.Deals.size() == S.Life.Stakes.Deals.size(),
		"stake: the session's save round-trips the book");
}

void Seams()
{
	// The Session.h block: refusals in the game's voice; the funding options for an unknown id are the player's own.
	Hooks H;
	ss::Session S(H, "stake-seams");
	Expect(S.FundingOptions("no-such-event").size() == 1 && life::IsOwnMoney(S.FundingOptions("no-such-event")[0].first), "stake: an unknown event is the player's own money");
	Expect(S.AcceptStake("nope") == "That offer's gone." && S.PayOwed("nope") == "You don't owe anyone that." && S.RequestChop("nope") == "There's no deal to chop." &&
			   S.WalkAway("nope") == "There's no deal to walk away from." && !S.BookCoach("fred").empty(),
		"stake: the actions refuse what isn't there");
	Expect(S.ListAction("embercrest-nightly@9", 300, 1050) == "Buyers read results, and yours don't sell at a markup yet. Par to the people who know you, maybe." &&
			   S.Life.Stakes.Listings.empty(),
		"stake: a fresh career can't sell 30% at 1.05, and says why");
	Expect(S.ListAction("embercrest-nightly@9", 30, 1000) == "Pieces at the Embercrest go in fives: 5%, 10%, 15%.", "stake: live pieces in fives");
}

/** Mrs. Park's share of 10% of an event listed at par through the session's own view of the player; the listing is taken down again. */
int ParkShare(ss::Session& S, const std::string& EventId, std::string& Said)
{
	Said = S.ListAction(EventId, 100, 1000);
	int Pct = 0;
	for (const stake::Listing& Ls : S.Life.Stakes.Listings)
	{
		for (const std::tuple<double, std::string, int>& Pl : Ls.EventId == EventId ? Ls.Plan : std::vector<std::tuple<double, std::string, int>>())
		{
			Pct += std::get<1>(Pl) == "Mrs. Park" ? std::get<2>(Pl) : 0;
		}
	}
	S.CancelListing(EventId);
	return Pct;
}

void ParkReadsTheSample()
{
	// Update 2 bug 6 (SPEC §3.6), end to end. On a fresh world's first night nobody has 200 events on record, so
	// World::RoiRank is unknown (-1): Mrs. Park's gate reads the player's own sample instead (200 paid online events in
	// profit) rather than refusing as if they sat at the bottom. An established world ranks the player as it always has.
	ss::hero::Character Who;
	const live::Occurrence N = live::FindOccurrence("embercrest-nightly@1");
	for (const ss::world::Start Start : {ss::world::Start::Blank, ss::world::Start::Established})
	{
		const bool Fresh = Start == ss::world::Start::Blank;
		const std::string Tag = std::string("stake: Mrs. Park, ") + (Fresh ? "a fresh world" : "an established world");
		Hooks H;
		ss::Session S(H, Fresh ? "stake-park-fresh" : "stake-park-established");
		S.NewCareer(Who, Start);
		S.CurrentScreen = ss::Screen::Lobby;
		Wait(S, 0.0, 0.2);
		ss::world::World& W = S.Living();
		const int Park = W.Find("Mrs. Park");
		for (int K = 0; K < 4; ++K)
		{
			W.Lent(Park, true, S.WorldMinutes()); // she trusts the player (0.4): a friend buys at par, whatever the price model
		}
		const ss::world::Bond* Bd = W.BondWith(Park);
		Expect(N.Valid() && Park >= 0 && Bd && Bd->Label() == "Trusts you", (Tag + ": she trusts the player").c_str());
		// No record yet: no live events, no online sample, and nobody else in the view who'd buy.
		std::string Said;
		Expect(ParkShare(S, N.Id, Said) == 0 && Said == "Nobody's buying a piece of you yet. Backers read results. Go get some." && S.Life.Stakes.Listings.empty(),
			(Tag + ": with no record she passes, and the listing says why").c_str());
		// Two samples (World::GrantHeroResults is seeded by the player's event count alone, so both are the same in either
		// world): 240 paid online events at a loss, then more, ten at a time, until the whole sample turns a profit.
		for (int Step = 0; Step < 2; ++Step)
		{
			if (Step == 0)
			{
				W.GrantHeroResults(240);
			}
			while (Step == 1 && W.HeroStats().Roi() <= 0.0 && W.HeroStats().Events < 2000)
			{
				W.GrantHeroResults(10);
			}
			const ss::rep::Public Rec = S.Record();
			const double Rank = W.RoiRank(Rec.OnlineRoi);
			const int Share = ParkShare(S, N.Id, Said);
			const bool Profit = Rec.OnlineRoi > 0.0;
			std::printf("  stake: Mrs. Park, %s: %d paid online events at %s%%, rank %s, her share %d permille\n", Fresh ? "fresh" : "established", Rec.OnlineEvents,
				ss::Fixed(100.0 * Rec.OnlineRoi, 1).c_str(), ss::Fixed(Rank, 3).c_str(), Share);
			Expect(Rec.OnlineEvents >= 200 && Profit == (Step == 1), (Tag + (Step == 0 ? ": 200+ paid online events at a loss" : ": then in profit")).c_str());
			if (Fresh)
			{
				// Nobody else has 200 events on night one: the rank is unknown, and she goes by the sample alone.
				Expect(Rank == -1.0, (Tag + ": too few regulars on record to rank the player (-1)").c_str());
				Expect(Profit ? Said.empty() && Share == 100 : Share == 0 && S.Life.Stakes.Listings.empty(),
					(Tag + (Profit ? ": the rank is unknown, so she reads the sample, a profit, and takes her 10%" : ": the rank is unknown and the sample's a loss: no piece"))
						.c_str());
			}
			else
			{
				// 1,600 regulars to rank against: the rank decides, as it always has (both samples sit in the bottom half here).
				Expect(Rank >= 0.0 && Rank < 0.5 && Share == 0, (Tag + ": a bottom-half rank, so no piece, profit or not").c_str());
			}
		}
	}
	// Under 200 paid online events there's nothing to rank yet: the session passes 0, never the rank a hot start would
	// earn. An established world, ten events at a time until the small sample's ROI sits in the regulars' top half.
	{
		Hooks H;
		ss::Session S(H, "stake-park-small");
		S.NewCareer(Who);
		S.CurrentScreen = ss::Screen::Lobby;
		Wait(S, 0.0, 0.2);
		ss::world::World& W = S.Living();
		const int Park = W.Find("Mrs. Park");
		for (int K = 0; K < 4; ++K)
		{
			W.Lent(Park, true, S.WorldMinutes());
		}
		ss::rep::Public Rec = S.Record();
		while (W.HeroStats().Events < 190 && !(Rec.OnlineEvents > 0 && W.RoiRank(Rec.OnlineRoi) >= 0.5))
		{
			W.GrantHeroResults(10);
			Rec = S.Record();
		}
		const double Rank = W.RoiRank(Rec.OnlineRoi);
		std::string Said;
		const int Share = ParkShare(S, N.Id, Said);
		std::printf("  stake: Mrs. Park, established: %d paid online events at %s%%, rank %s, her share %d permille\n", Rec.OnlineEvents,
			ss::Fixed(100.0 * Rec.OnlineRoi, 1).c_str(), ss::Fixed(Rank, 3).c_str(), Share);
		Expect(Rec.OnlineEvents > 0 && Rec.OnlineEvents < 200 && Rank >= 0.5 && Share == 0,
			"stake: Mrs. Park, an established world: a top-half ROI on fewer than 200 paid online events buys no piece");
	}
}
} // namespace stake_seg

void SegStake()
{
	stake_seg::LousStake();
	stake_seg::CancelledStake();
	stake_seg::OnlineRoll();
	stake_seg::Calendar();
	stake_seg::Seams();
	stake_seg::ParkReadsTheSample();
}
} // namespace session_test
