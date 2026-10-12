// riverline_ui: core-stake's additions (next_segment_spec §7.6, §8.3), compiled into ui_test and called from main() in
// ui_main.cpp (the shared kit is UiSeg.h):
// the Backing page goldens (backing_empty, backing_deals, backing_sell, backing_offer, backing_review) drawn through the
// page's own entry point (the nav tab that opens it is ui-laptop's), a click through the offer's Accept and the sell
// panel's List, and the Embercrest page with a staked event (embercrest_card_staked).
#include "UiSeg.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Stake.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace backing_detail
{
// RiverLineBacking.cpp: the page RiverLine::BackingPage draws, the "Sell action" link's preselect, and whether the panel
// takes a live event.
void DrawPage(Ui& UI, Canvas& C, Session& S, double Now, bool Interactive);
void Preselect(const Ui& UI, const std::string& EventId);
bool LiveSellable(const Session& S, const std::string& OccurrenceId, double World);
} // namespace backing_detail
} // namespace ui
} // namespace ss

namespace ui_test
{
namespace stake_seg
{
namespace stake = ss::stake;
namespace live = ss::live;
using ss::ui::Canvas;
using ss::ui::DrawList;
using ss::ui::Ui;
const double Day = 1440.0;

/** One frame of the page into L (Ptr: where the pointer is this frame). */
void Frame(Ui& UI, ss::Session& S, DrawList& L, double Now, const ss::ui::Pointer& Ptr = ss::ui::Pointer())
{
	TableMeasurer M;
	L = DrawList();
	Canvas C(L, M, ss::ui::RiverLine::Width, ss::ui::RiverLine::Height, 1.0f);
	UI.Ptr = Ptr;
	UI.Begin(C, Now);
	ss::ui::backing_detail::DrawPage(UI, C, S, Now, true);
	UI.End();
}

/** A click at (X, Y): pressed on one frame, released on the next. */
void Click(Ui& UI, ss::Session& S, DrawList& L, double& Now, float X, float Y)
{
	ss::ui::Pointer P;
	P.X = X;
	P.Y = Y;
	P.Active = true;
	P.Down = true;
	P.Pressed = true;
	Now += 1.0 / 30.0;
	Frame(UI, S, L, Now, P);
	P.Down = false;
	P.Pressed = false;
	P.Released = true;
	Now += 1.0 / 30.0;
	Frame(UI, S, L, Now, P);
}

void Page()
{
	QuietHooks H;
	ss::Session S(H, "ui-backing");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Step(S, 0.0, 0.2);
	// Wednesday afternoon, the week's events ahead.
	S.LobbyMinutes = 9.0 * Day - Day + 14.0 * 60.0;
	Now = Step(S, Now, 0.2);
	S.BankrollCents = 139950;
	Ui UI;
	DrawList L;

	// Empty: nobody's backing you yet, and the sell panel says why nobody's buying.
	Frame(UI, S, L, Now);
	Expect(Drew(L, "Nobody's backing you yet.") && Drew(L, "Backers read results. Go get some.") && Drew(L, "No offers on the table."), "backing: the empty state");
	Expect(Drew(L, "pays the rent") && Drew(L, "no package") && Drew(L, "square with everyone"), "backing: the strip across the top");
	Expect(Drew(L, "Collected when you register. If you don't play it, nobody pays.") && Drew(L, "Selling "), "backing: the sell panel");
	WriteList("backing_empty", L);

	// An offer: Lou's stake for tonight's Nightly, with the counter's mood line; accepting it by clicking.
	S.DebugStake("offer lou");
	Frame(UI, S, L, Now);
	Expect(Drew(L, "Big Lou") && Drew(L, "100% of the Embercrest Nightly") && Drew(L, "Sounds open to it") && Drew(L, "Accept") && Drew(L, "Counter"), "backing: Lou's offer card with the mood line");
	WriteList("backing_offer", L);
	const float Ay = TextAt(L, "Accept");
	Click(UI, S, L, Now, 1076.0f + 20.0f + 65.0f, Ay);
	Expect(S.Life.Stakes.Offers.empty() && S.Life.Stakes.Deals.size() == 1, "backing: Accept takes Lou's stake");
	Frame(UI, S, L, Now);
	Expect(Drew(L, "Big Lou is putting you in the Nightly."), "backing: the stake on the deals column");

	// Deals: a Lantern Row package with makeup, something owed, the confirm card for walking away.
	S.DebugStake("grant lanternrow");
	stake::Deal* Pkg = nullptr;
	for (stake::Deal& D : S.Life.Stakes.Deals)
	{
		Pkg = D.K == stake::Kind::Package ? &D : Pkg;
	}
	if (!Pkg)
	{
		Expect(false, "backing: the package");
		return;
	}
	Pkg->RollNow -= 68000;
	stake::Owed O;
	O.Id = "owe:ui";
	O.Who = "Mrs. Park";
	O.Label = "10% of your Sunday, 4th";
	O.Cents = 11900;
	O.Since = S.WorldMinutes();
	O.Due = S.WorldMinutes() + 3.0 * Day;
	O.EntryId = "embercrest@6";
	S.Life.Stakes.Owes.push_back(O);
	Frame(UI, S, L, Now);
	Expect(Drew(L, "-$680 makeup") && Drew(L, "Lantern Row Staking") && Drew(L, "10% of your Sunday, 4th") && Drew(L, "Mrs. Park") && Drew(L, "Walk away"),
		"backing: the package card with its makeup, and what's owed");
	WriteList("backing_deals", L);
	const float Wy = TextAt(L, "Walk away");
	Click(UI, S, L, Now, 24.0f + 500.0f - 80.0f, Wy);
	Frame(UI, S, L, Now);
	Expect(Drew(L, "Walk away from $680.00 of makeup?") || Drew(L, "Walk away from $680 of makeup?"), "backing: the confirm card names the makeup");
	Expect(Drew(L, "You won't owe Lantern Row Staking a dime. Everyone who stakes will hear about it, and Lantern Row Staking won't deal with you for a year.") ||
			   Drew(L, "You won't owe Lantern Row Staking a dime."),
		"backing: and what walking away means");
	const float Sy = TextAt(L, "Stay");
	Click(UI, S, L, Now, 550.0f + 500.0f - 140.0f, Sy);
	Frame(UI, S, L, Now);
	Expect(!Drew(L, "Walk away from") && stake::ActivePackage(S.Life.Stakes) != nullptr, "backing: Stay keeps the deal");

	// Fred's review card.
	const float Ry = TextAt(L, "Fred's review");
	Click(UI, S, L, Now, 24.0f + 160.0f + 70.0f, Ry);
	Frame(UI, S, L, Now);
	Expect(Drew(L, "Monthly leak review") || Drew(L, "Monthly leak review, with each chop"), "backing: Fred's review opens");
	WriteList("backing_review", L);
	const float Cy = TextAt(L, "Close");
	Click(UI, S, L, Now, 450.0f + 700.0f - 100.0f, Cy);

	// Selling: Saturday's events listed with a record that sells. A friend at par, the projected fill after listing.
	S.Life.LiveEvents = 31;
	S.Life.LiveCashes = 6;
	S.Life.LiveWonCents = 31 * 15000 * 144 / 100;
	S.Life.LiveSpentCents = 31 * 15000;
	S.Life.SegmentLinesRead |= ss::life::SegLiveWins | ss::life::SegLiveRecord;
	Frame(UI, S, L, Now);
	Expect(Drew(L, "31 live events at +44%") && Drew(L, "Selling") && Drew(L, "List "), "backing: the sell panel's numbers");
	WriteList("backing_sell", L);
	const float Ly = TextAt(L, "List ");
	Click(UI, S, L, Now, 540.0f + 20.0f + 110.0f, Ly);
	Frame(UI, S, L, Now);
	Expect(!S.Life.Stakes.Listings.empty() || Drew(L, "Nobody"), "backing: List lists (or says why nobody's buying)");
	std::printf("  backing: %zu draw commands on the sell panel\n", L.Cmds.size());
}

void EmbercrestCard()
{
	// The Embercrest page with Lou's stake on an event (the card's funding line is ui-laptop's; the page draws).
	QuietHooks H;
	ss::Session S(H, "ui-backing-card");
	ss::ui::RiverLine RL(S);
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Step(S, 0.0, 0.2);
	S.LobbyMinutes = 9.0 * Day - Day + 14.0 * 60.0;
	Now = Step(S, Now, 0.2);
	S.BankrollCents = 50000;
	S.DebugStake("offer lou");
	for (const stake::Offer& O : std::vector<stake::Offer>(S.Life.Stakes.Offers))
	{
		S.AcceptStake(O.Id);
	}
	RL.OpenPage(ss::ui::RiverLine::Page::Embercrest, Now);
	Emit("embercrest_card_staked", RL, Now + 1.0);
	const std::vector<std::pair<ss::life::Funding, std::string>> Opts = S.FundingOptions("embercrest-nightly@9");
	Expect(!Opts.empty() && ss::live::FundingLine(Opts[0].first, "Nightly") == "Big Lou is putting you in the Nightly.", "backing: the event card's funding line reads Lou");
}

/** The sell panel's event labels (the row of picks under SELL ACTION). */
std::vector<std::string> PickLabels(const DrawList& L)
{
	std::vector<std::string> Out;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		if (Cmd.Type == ss::ui::DrawCmd::Kind::Text && Cmd.Text.BaselineY > 224.0f && Cmd.Text.BaselineY < 224.0f + 2.0f * 46.0f && Cmd.Text.X >= 540.0f && Cmd.Text.X <= 1060.0f)
		{
			Out.push_back(Cmd.Text.Text);
		}
	}
	return Out;
}

void PicksCovered()
{
	// The sell panel offers only events that are the player's own to sell: one a sponsor's cover pays for (the way GoToLive
	// would register it) is left out, whether or not another deal that covers other events comes first in the book.
	auto PicksWith = [](bool PennyFirst) {
		QuietHooks H;
		ss::Session S(H, "ui-backing-picks");
		S.CurrentScreen = ss::Screen::Lobby;
		double Now = Step(S, 0.0, 0.2);
		S.LobbyMinutes = 9.0 * Day - Day + 14.0 * 60.0;
		Now = Step(S, Now, 0.2);
		const double World = S.WorldMinutes();
		ss::sponsor::Contract Penny;
		Penny.Id = "c:luckypenny:picks";
		Penny.Brand = "luckypenny";
		Penny.St = ss::sponsor::State::Active;
		Penny.Since = World - Day;
		Penny.Until = World + 60.0 * Day;
		Penny.T.CoveredEntries = 1;
		Penny.T.CoveredCapCents = 8000;
		Penny.T.CoveredEvents = {"noon"};
		ss::sponsor::Contract Team = Penny;
		Team.Id = "c:embercrest:picks";
		Team.Brand = "embercrest";
		Team.T.CoveredEntries = 2;
		Team.T.CoveredCapCents = 12000;
		Team.T.CoveredEvents = {"noon", "warmup", "nightly", "turbo"};
		Team.T.CoversFares = true;
		if (PennyFirst)
		{
			S.Life.Sponsors.Contracts.push_back(Penny);
		}
		S.Life.Sponsors.Contracts.push_back(Team);
		Ui UI;
		DrawList L;
		Frame(UI, S, L, Now);
		return PickLabels(L);
	};
	const std::vector<std::string> Alone = PicksWith(false);
	const std::vector<std::string> Both = PicksWith(true);
	const bool NoNightly = std::none_of(Both.begin(), Both.end(), [](const std::string& X) { return X.rfind("Nightly", 0) == 0; });
	Expect(!Alone.empty() && Both == Alone && NoNightly, "backing: a covered event isn't for sale, whichever deal the book lists first");
}

void PicksKeepPreselected()
{
	// The Embercrest card's "Sell action" on an event past the panel's first six (later in the week): the panel opens on
	// it, in the sixth place, and the card's link and the panel agree on what can be sold.
	QuietHooks H;
	ss::Session S(H, "ui-backing-far");
	S.CurrentScreen = ss::Screen::Lobby;
	double Now = Step(S, 0.0, 0.2);
	S.LobbyMinutes = 9.0 * Day - Day + 14.0 * 60.0;
	Now = Step(S, Now, 0.2);
	const double World = S.WorldMinutes();
	const std::vector<live::Occurrence> Week = live::Reachable(World, 6.0 * 24.0);
	const live::Occurrence* Far = Week.size() > 7 ? &Week[7] : nullptr;
	Expect(Far != nullptr, "backing: the bus can make more than seven events this week");
	if (!Far)
	{
		return;
	}
	const std::string Label = std::string(Far->T->Short) + " \xC2\xB7 " + (Far->Day == ss::net::DayOf(World) ? std::string("Today") : std::string(ss::net::WeekdayName(Far->Day)));
	Ui UI;
	DrawList L;
	ss::ui::backing_detail::Preselect(UI, Far->Id);
	Frame(UI, S, L, Now);
	const std::vector<std::string> Picks = PickLabels(L);
	Expect(Picks.size() == 6 && Picks.back() == Label && ss::ui::backing_detail::LiveSellable(S, Far->Id, World),
		("backing: a preselected event later in the week joins the picks, last, and the panel opens on it (" + Label + ")").c_str());
	// The first six stay as they were in front of it.
	Ui Plain;
	DrawList P;
	Frame(Plain, S, P, Now);
	const std::vector<std::string> First = PickLabels(P);
	Expect(First.size() == 6 && std::equal(First.begin(), First.begin() + 5, Picks.begin()) && First.back() != Label, "backing: without it, the week's first six");
	// An event the bus can't make this week isn't for sale, from the card or the panel.
	Expect(!ss::ui::backing_detail::LiveSellable(S, "embercrest-nowhere@1", World), "backing: an event that isn't on this week's bus isn't for sale");
}
} // namespace stake_seg

void SegStake()
{
	stake_seg::Page();
	stake_seg::EmbercrestCard();
	stake_seg::PicksCovered();
	stake_seg::PicksKeepPreselected();
}
} // namespace ui_test
