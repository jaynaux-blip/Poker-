#include "ShortStack/UI/RiverLine.h"
#include "../StrictFloat.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Rep.h"
#include "ShortStack/Game/Stake.h"

#include <algorithm>
#include <cmath>
#include <map>

// RiverLine's Backing page (next_segment_spec §2.12): stakes, pieces, swaps and packages. core-stake.
//
// A strip across the top (your roll, the staked roll, makeup with its sign, what you owe and are owed, the next chop),
// then three columns: the deals (and what's owed), selling action and swaps, the offers. Walking away opens a confirm
// card; a Lantern Row deal opens Fred's latest review. Everything the page does goes through the Session (its calls
// say why not in a line, shown under the panel that asked). The page is drawn by backing_detail::DrawPage, which
// RiverLine::BackingPage calls with its own UI: the page's own state (the event picked, the share, the markup, the
// confirm card) is kept per UI, so the tests can draw it directly.

namespace ss
{
namespace ui
{
namespace backing_detail
{
using namespace rlnet_detail;
namespace stk = ss::stake;

const std::string Dot = " \xC2\xB7 ";
constexpr double Day = 1440.0;

struct PageState
{
	std::string Event;         // the event picked to sell ("" until one is)
	int Percent = 0;           // the share to list, in percent
	int Markup = 0;            // permille (0: the ceiling, set when an event is picked)
	std::string Confirm;       // the deal the walk-away card is up for
	bool Review = false;       // Fred's review card
	std::string Note;          // the last answer
	bool NoteBad = false;
	double NoteAt = -100.0;
	std::map<std::string, int> Lever; // the lever each offer's counter will pull
};

PageState& StateFor(const Ui& UI)
{
	static std::map<const Ui*, PageState> States;
	return States[&UI];
}

void Say(PageState& St, const std::string& Line, bool Bad, double Now)
{
	St.Note = Line;
	St.NoteBad = Bad;
	St.NoteAt = Now;
}

void PanelAt(Ui& UI, Canvas& C, const Rect& R, float Radius, const Color& Edge)
{
	C.GlowRoundRect({R.X, R.Y + 6.0f, R.W, R.H}, Radius, Rgba(0, 0, 0, 0.35f), 18.0f);
	UI.RRect(R, Radius, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H}, Hex(0x122036), Hex(0x0d1729)), Edge);
	C.FillRect({R.X + Radius, R.Y + 1.0f, R.W - Radius * 2.0f, 1.0f}, Rgba(255, 255, 255, 0.05f));
}

float SectionAt(Canvas& C, const std::string& Title, float X, float Y, const Color& Col)
{
	C.FillRoundRect({X, Y - 11.0f, 3.0f, 13.0f}, 1.5f, Col);
	return NetSpaced(C, Title, X + 11.0f, Y, 12.0f, 800, pal::Muted, 1.6f) + 11.0f;
}

/** The backer's colour: the Embercrest's ember for the room's people, Lantern Row's lantern, Sam's gold. */
Color BackerColor(const std::string& Who)
{
	if (Who == "Lantern Row Staking")
	{
		return Hex(0xf2a93b);
	}
	if (Who == "Sam")
	{
		return pal::Gold;
	}
	if (Who == "Dee")
	{
		return Hex(0xc084fc);
	}
	return Hex(EcEmberHot);
}

std::string WhenText(double At, double World)
{
	const int D = net::DayOf(At);
	const int Today = net::DayOf(World);
	const std::string DayWord = D == Today ? "today" : D == Today + 1 ? "tomorrow" : D < Today + 7 ? std::string(net::WeekdayName(D, true)) : net::DateLabel(D);
	return DayWord + " " + net::TimeLabel(At);
}

/** "2d 4h" to go, or how long ago it was due. */
std::string DueText(double Due, double World)
{
	return Due >= World ? "due in " + net::Countdown(Due - World) : "due " + net::Countdown(World - Due) + " ago";
}

std::string MarkupText(int Permille)
{
	const int H = (Permille + 5) / 10;
	const std::string F = std::to_string(H % 100);
	return std::to_string(H / 100) + "." + (F.size() < 2 ? "0" + F : F);
}

/** A bar with its label above: Frac of the way, in Col (a sign and a number always beside it: never colour alone). */
void Bar(Ui& UI, Canvas& C, const Rect& R, float Frac, const Color& Col)
{
	UI.RRect(R, R.H / 2.0f, Rgba(255, 255, 255, 0.07f));
	const float W = std::max(R.H, R.W * std::min(1.0f, std::max(0.0f, Frac)));
	if (Frac > 0.0f)
	{
		UI.RRect({R.X, R.Y, W, R.H}, R.H / 2.0f, Paint::Linear({R.X, 0.0f}, {R.X + R.W, 0.0f}, NetA(Col, 0.7f), Col));
	}
	(void)C;
}

// ------------------------------------------------------------------ the strip

void Strip(Ui& UI, Canvas& C, const Session& S, double World)
{
	const stk::Book& B = S.Life.Stakes;
	const Chips Makeup = stk::MakeupOf(B);
	const Chips Owe = stk::YouOwe(B);
	const Chips Owed = stk::OwedToYou(B);
	const stk::Deal* P = stk::ActivePackage(B);
	struct Tile
	{
		std::string Big;
		std::string Label;
		std::string Sub;
		Color Col;
	};
	const Tile Tiles[6] = {
		{Money(S.BankrollCents), "YOUR ROLL", "pays the rent", pal::Ink},
		{P ? NetMoney(P->RollNow) : std::string("\xE2\x80\x94"), "STAKED ROLL", P ? "theirs, never rent" : "no package", P ? Hex(0xf2a93b) : pal::Dim},
		{Makeup > 0 ? "-" + NetMoney(Makeup) + " makeup" : std::string("none"), "MAKEUP", Makeup > 0 ? "not a debt" : (P ? "the roll's up" : "no package"), Makeup > 0 ? pal::Red : pal::Muted},
		{Owe > 0 ? Money(Owe) : std::string("$0"), "YOU OWE", Owe > 0 ? "auto-paid when it's due" : "square with everyone", Owe > 0 ? pal::Orange : pal::Muted},
		{Owed > 0 ? Money(Owed) : std::string("$0"), "OWED TO YOU", Owed > 0 ? "they'll pay" : "nobody", Owed > 0 ? pal::Green : pal::Muted},
		{P ? net::DateLabel(net::DayOf(P->NextChop)) : std::string("\xE2\x80\x94"), "NEXT CHOP", P ? "in " + net::Countdown(std::max(0.0, P->NextChop - World)) : "no package", P ? pal::Ink : pal::Dim},
	};
	const float Tw = (1552.0f - 5.0f * 12.0f) / 6.0f;
	for (int I = 0; I < 6; ++I)
	{
		const Rect R{24.0f + Nf(I) * (Tw + 12.0f), 84.0f, Tw, 88.0f};
		PanelAt(UI, C, R, 12.0f, pal::Line);
		NetSpaced(C, Tiles[I].Label, R.X + 16.0f, R.Y + 24.0f, 9.5f, 800, pal::Muted, 1.3f);
		UI.Text(Tiles[I].Big, R.X + 16.0f, R.Y + 56.0f, Ts(I == 2 && Makeup > 0 ? 19.0f : 23.0f, 900, Tiles[I].Col, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		UI.Text(Tiles[I].Sub, R.X + 16.0f, R.Y + 76.0f, Ts(11.5f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
	}
}

// ------------------------------------------------------------------ deals and what's owed

float PackageCard(Ui& UI, Canvas& C, Session& S, PageState& St, const stk::Deal& D, float Y, double World, double Now, bool Interactive)
{
	const Rect R{24.0f, Y, 500.0f, 236.0f};
	const std::string Who = stk::BackerName(D);
	const Color Col = BackerColor(Who);
	PanelAt(UI, C, R, 14.0f, NetA(Col, 0.4f));
	NetAvatar(C, R.X + 34.0f, R.Y + 36.0f, 20.0f, D.By == stk::Outfit::LanternRow ? std::string("FoldEquityFred") : Who, Col);
	UI.Text(Who, R.X + 64.0f, R.Y + 32.0f, Ts(17.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, 280.0f));
	const std::string Sub = D.By == stk::Outfit::LanternRow ? "FoldEquityFred's stable" + Dot + "tier " + std::to_string(std::max(1, D.Tier))
		: D.By == stk::Outfit::Sam ? std::string("on the Burner") : std::string("a package from a regular");
	UI.Text(Sub, R.X + 64.0f, R.Y + 50.0f, Ts(12.0f, 600, NetA(Col, 0.9f)));
	NetPill(C, D.State == stk::DealOpen ? "RUNNING" : D.State == stk::DealWalked ? "WALKED" : "ENDED", R.X + R.W - 96.0f, R.Y + 18.0f, D.State == stk::DealOpen ? pal::Green : pal::Muted);
	NetParagraph(C, stk::DealLine(D), R.X + 20.0f, R.Y + 76.0f, R.W - 40.0f, 12.5f, 600, Hex(0xc3cedf), 16.0f, 2);
	// Makeup against the review's line (150 buy-ins at the cap), and the period's volume.
	const Chips Makeup = std::max<Chips>(0, D.Funded - D.RollNow);
	const Chips Line = 150 * std::max<Chips>(1, D.T.CapOnline);
	const std::string MakeupText = Makeup > 0 ? "MAKEUP -" + NetMoney(Makeup) : "MAKEUP NONE" + Dot + "UP " + NetMoney(D.RollNow - D.Funded);
	NetSpaced(C, MakeupText, R.X + 20.0f, R.Y + 122.0f, 9.5f, 800, Makeup > 0 ? pal::Red : pal::Green, 1.2f);
	Bar(UI, C, {R.X + 20.0f, R.Y + 130.0f, 220.0f, 7.0f}, Nf(static_cast<double>(Makeup) / static_cast<double>(Line)), pal::Red);
	if (D.T.Volume > 0)
	{
		NetSpaced(C, "VOLUME " + std::to_string(D.PeriodEvents) + " / " + std::to_string(D.T.Volume), R.X + 260.0f, R.Y + 122.0f, 9.5f, 800, pal::Muted, 1.2f);
		Bar(UI, C, {R.X + 260.0f, R.Y + 130.0f, 220.0f, 7.0f}, Nf(static_cast<double>(D.PeriodEvents) / static_cast<double>(D.T.Volume)), pal::Accent);
	}
	else if (D.Debt > 0)
	{
		NetSpaced(C, "DEBT " + NetMoney(D.Debt), R.X + 260.0f, R.Y + 122.0f, 9.5f, 800, pal::Red, 1.2f);
	}
	// The roll's last three lines.
	float Ly = R.Y + 156.0f;
	for (size_t K = 0; K < 3 && K < D.Lines.size(); ++K)
	{
		const stk::Line& Ln = D.Lines[D.Lines.size() - 1 - K];
		UI.Text(net::DateLabel(net::DayOf(Ln.At)), R.X + 20.0f, Ly, Ts(11.5f, 600, pal::Dim));
		UI.Text(Ln.Label, R.X + 76.0f, Ly, Ts(11.5f, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, 300.0f));
		UI.Text((Ln.Amount >= 0 ? "+" : "-") + NetMoney(Ln.Amount >= 0 ? Ln.Amount : -Ln.Amount), R.X + R.W - 20.0f, Ly, Ts(11.5f, 700, Ln.Amount >= 0 ? pal::Green : pal::Muted, Align::Right, Baseline::Alphabetic, true));
		Ly += 16.0f;
	}
	if (D.State == stk::DealOpen)
	{
		const float By = R.Y + R.H - 46.0f;
		ButtonOpts Chop;
		Chop.Size = 13.0f;
		Chop.Enabled = Interactive;
		if (UI.Button("bk-chop-" + D.Id, {R.X + 20.0f, By, 130.0f, 32.0f}, "Request chop", Chop))
		{
			const std::string Not = S.RequestChop(D.Id);
			Say(St, Not.empty() ? std::string("Chopped. Your cut's in your roll.") : Not, !Not.empty(), Now);
		}
		if (D.By == stk::Outfit::LanternRow)
		{
			ButtonOpts Rv;
			Rv.Kind = ButtonKind::Ghost;
			Rv.Size = 13.0f;
			Rv.Enabled = Interactive;
			if (UI.Button("bk-review-" + D.Id, {R.X + 160.0f, By, 140.0f, 32.0f}, "Fred's review", Rv))
			{
				St.Review = true;
			}
		}
		ButtonOpts Walk;
		Walk.Kind = ButtonKind::Danger;
		Walk.Size = 13.0f;
		Walk.Enabled = Interactive;
		if (UI.Button("bk-walk-" + D.Id, {R.X + R.W - 140.0f, By, 120.0f, 32.0f}, "Walk away", Walk))
		{
			St.Confirm = D.Id;
		}
	}
	(void)World;
	return R.H;
}

float StakeCard(Ui& UI, Canvas& C, const stk::Deal& D, float Y, double World)
{
	const Rect R{24.0f, Y, 500.0f, 96.0f};
	const Color Col = BackerColor(D.Backer);
	PanelAt(UI, C, R, 14.0f, NetA(Col, 0.35f));
	NetAvatar(C, R.X + 34.0f, R.Y + 38.0f, 20.0f, D.Backer, Col);
	const live::Occurrence O = live::FindOccurrence(D.EventId);
	const std::string Event = O.Valid() ? O.T->Short : std::string("event");
	UI.Text(D.Backer + (D.State == stk::DealSettled ? " put you in the " + Event + "." : " is putting you in the " + Event + "."), R.X + 64.0f, R.Y + 34.0f,
		Ts(15.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 180.0f));
	UI.Text(stk::DealLine(D), R.X + 64.0f, R.Y + 54.0f, Ts(12.0f, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, R.W - 84.0f));
	std::string State;
	Color Pill = pal::Accent;
	if (D.State == stk::DealOpen)
	{
		State = O.Valid() ? "CARDS " + NetUpper(WhenText(O.Start, World)) : std::string("WAITING");
	}
	else if (D.State == stk::DealPlaying)
	{
		State = "REGISTERED";
		Pill = pal::Green;
	}
	else
	{
		State = D.Prize > 0 ? "CASHED" + Dot + NetMoney(D.Prize - D.TheirShare) + " TO YOU" : std::string("BRICKED" + Dot + "HIS LOSS");
		Pill = D.Prize > 0 ? pal::Gold : pal::Muted;
	}
	NetPill(C, State, R.X + 64.0f, R.Y + 64.0f, Pill);
	return R.H;
}

float OwedRows(Ui& UI, Canvas& C, Session& S, PageState& St, float Y, float Bottom, double World, double Now, bool Interactive)
{
	const stk::Book& B = S.Life.Stakes;
	std::vector<stk::Owed> Open;
	for (const stk::Owed& O : B.Owes)
	{
		if (O.State == stk::OwedOpen || O.State == stk::OwedLate || O.State == stk::OwedWelched)
		{
			Open.push_back(O);
		}
	}
	if (Open.empty())
	{
		return 0.0f;
	}
	float Yy = Y;
	SectionAt(C, "WHAT'S OWED", 24.0f, Yy + 14.0f, pal::Orange);
	Yy += 26.0f;
	for (const stk::Owed& O : Open)
	{
		if (Yy + 52.0f > Bottom)
		{
			break;
		}
		const Rect R{24.0f, Yy, 500.0f, 48.0f};
		UI.RRect(R, 10.0f, Rgba(255, 255, 255, 0.03f), pal::Line);
		const bool Bad = O.State == stk::OwedLate || O.State == stk::OwedWelched;
		const std::string Who = O.ToYou ? O.Who + " owes you" : O.Who;
		UI.Text(Who, R.X + 14.0f, R.Y + 20.0f, Ts(13.5f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, 200.0f));
		UI.Text(O.Label, R.X + 14.0f, R.Y + 38.0f, Ts(11.5f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, 220.0f));
		UI.Text(Money(O.Cents), R.X + 330.0f, R.Y + 22.0f, Ts(15.0f, 900, O.ToYou ? pal::Green : Bad ? pal::Red : pal::Ink, Align::Right, Baseline::Alphabetic, true));
		UI.Text(O.State == stk::OwedWelched ? "WELCHED" : O.State == stk::OwedLate ? "LATE" : DueText(O.Due, World), R.X + 330.0f, R.Y + 39.0f,
			Ts(10.5f, 700, Bad ? pal::Red : pal::Dim, Align::Right));
		if (!O.ToYou)
		{
			ButtonOpts Pay;
			Pay.Kind = Bad ? ButtonKind::Gold : ButtonKind::Secondary;
			Pay.Size = 12.5f;
			Pay.Enabled = Interactive && S.BankrollCents >= O.Cents;
			if (UI.Button("bk-pay-" + O.Id, {R.X + R.W - 120.0f, R.Y + 9.0f, 106.0f, 30.0f}, "Pay now", Pay))
			{
				const std::string Not = S.PayOwed(O.Id);
				Say(St, Not.empty() ? "Paid " + O.Who + " " + NetMoney(O.Cents) + "." : Not, !Not.empty(), Now);
			}
		}
		Yy += 54.0f;
	}
	return Yy - Y;
}

void Deals(Ui& UI, Canvas& C, Session& S, PageState& St, double World, double Now, bool Interactive)
{
	const stk::Book& B = S.Life.Stakes;
	SectionAt(C, "YOUR DEALS", 24.0f, 210.0f, Hex(EcEmberHot));
	float Y = 224.0f;
	const float Bottom = 950.0f;
	bool Any = false;
	for (const stk::Deal& D : B.Deals)
	{
		if (D.K == stk::Kind::Package && (D.State == stk::DealOpen || D.Debt > 0) && Y + 236.0f <= Bottom)
		{
			Y += PackageCard(UI, C, S, St, D, Y, World, Now, Interactive) + 12.0f;
			Any = true;
		}
	}
	for (auto It = B.Deals.rbegin(); It != B.Deals.rend(); ++It)
	{
		const stk::Deal& D = *It;
		const bool Recent = D.State == stk::DealOpen || D.State == stk::DealPlaying || (D.State == stk::DealSettled && D.Since > World - 7.0 * Day);
		if (D.K == stk::Kind::Event && Recent && Y + 96.0f <= Bottom - 60.0f)
		{
			Y += StakeCard(UI, C, D, Y, World) + 12.0f;
			Any = true;
		}
	}
	const float Owed = OwedRows(UI, C, S, St, Y, Bottom, World, Now, Interactive);
	Any = Any || Owed > 0.0f;
	if (!Any)
	{
		// Nobody yet: the empty state, honest about what backers read.
		const Rect R{24.0f, Y, 500.0f, 190.0f};
		PanelAt(UI, C, R, 14.0f, pal::Line);
		UI.Text("Nobody's backing you yet.", R.X + R.W / 2.0f, R.Y + 62.0f, Ts(20.0f, 800, pal::Ink, Align::Center));
		UI.Text("Backers read results. Go get some.", R.X + R.W / 2.0f, R.Y + 90.0f, Ts(14.0f, 600, pal::Muted, Align::Center));
		NetParagraph(C, "Big Lou stakes people who've cashed at the Embercrest. Lantern Row reads five hundred RiverLine events and an edge. Pieces sell to whoever believes the numbers.",
			R.X + 40.0f, R.Y + 124.0f, R.W - 80.0f, 12.0f, 500, pal::Dim, 16.0f, 3);
	}
}

// ------------------------------------------------------------------ selling action and swaps

struct EventPick
{
	std::string Id;
	std::string Label;
	std::string Name;
	Chips BuyIn = 0;
	double Start = 0.0;
	bool Live = true;
};

std::vector<EventPick> Picks(const Session& S, double World)
{
	std::vector<EventPick> Out;
	for (const live::Occurrence& O : live::Reachable(World, 6.0 * 24.0))
	{
		if (Out.size() >= 6)
		{
			break;
		}
		if (live::EntryFor(S.Life, O.Id) != nullptr)
		{
			continue; // registered: its pieces sold (or didn't) at the desk
		}
		// Someone else pays for it the way GoToLive would register it now (the first option that can: a stake, a cover, a
		// package): it isn't the player's to sell. A deal listed with why it won't pay (a cover for other events, one used
		// up, an offer on hold) doesn't count, wherever the book lists it.
		const std::vector<std::pair<life::Funding, std::string>> Pay = stk::FundingOptions(S.Life, O, World);
		const auto Payer = std::find_if(Pay.begin(), Pay.end(), [](const std::pair<life::Funding, std::string>& X) { return X.second.empty(); });
		if (Payer != Pay.end() && Payer->first.From != life::Paid::Own)
		{
			continue;
		}
		EventPick P;
		P.Id = O.Id;
		P.Label = std::string(O.T->Short) + Dot + (O.Day == net::DayOf(World) ? std::string("Today") : std::string(net::WeekdayName(O.Day)));
		P.Name = O.T->Name;
		P.BuyIn = O.T->BuyInCents;
		P.Start = O.Start;
		Out.push_back(P);
	}
	return Out;
}

void Sell(Ui& UI, Canvas& C, Session& S, PageState& St, double World, double Now, bool Interactive)
{
	const stk::Book& B = S.Life.Stakes;
	SectionAt(C, "SELL ACTION", 540.0f, 210.0f, pal::Accent);
	std::vector<EventPick> Events = Picks(S, World);
	// An online event opened from its register block (Preselect): it joins the picks, sold in whole percents.
	net::EventInstance Online;
	if (!St.Event.empty() && std::none_of(Events.begin(), Events.end(), [&St](const EventPick& P) { return P.Id == St.Event; }) &&
		!live::FindOccurrence(St.Event).Valid() && net::Shared().FindInstance(St.Event, Online) && Online.Start + Online.LateReg > World)
	{
		const net::EventTemplate& T = net::Shared().TemplateOf(Online);
		EventPick P;
		P.Id = Online.Id;
		P.Label = "RiverLine" + Dot + (net::DayOf(Online.Start) == net::DayOf(World) ? std::string("Today") : std::string(net::WeekdayName(net::DayOf(Online.Start))));
		P.Name = T.Name;
		P.BuyIn = T.BuyInCents;
		P.Start = Online.Start;
		P.Live = false;
		Events.insert(Events.begin(), P);
		if (Events.size() > 6)
		{
			Events.pop_back();
		}
	}
	if (St.Event.empty() || std::none_of(Events.begin(), Events.end(), [&St](const EventPick& P) { return P.Id == St.Event; }))
	{
		St.Event = Events.empty() ? std::string() : Events.front().Id;
	}
	// The events the bus can make this week.
	const float Cw = (520.0f - 2.0f * 10.0f) / 3.0f;
	for (size_t I = 0; I < Events.size(); ++I)
	{
		const Rect R{540.0f + Nf(I % 3) * (Cw + 10.0f), 224.0f + Nf(I / 3) * 46.0f, Cw, 38.0f};
		const bool On = Events[I].Id == St.Event;
		const Ui::ClickState Cl = UI.Clickable("bk-ev-" + Events[I].Id, R, Interactive);
		if (Cl.Clicked)
		{
			St.Event = Events[I].Id;
			St.Percent = 0;
			St.Markup = 0;
		}
		UI.RRect(R, 10.0f, On ? NetA(pal::Accent, 0.16f) : Cl.Hover ? Hex(0x172a42) : Rgba(255, 255, 255, 0.03f), On ? pal::Accent : pal::Line);
		UI.Text(Events[I].Label, R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, Ts(12.5f, On ? 800 : 600, On ? pal::Ink : pal::Muted, Align::Center, Baseline::Middle, false, R.W - 12.0f));
	}
	const auto Pick = std::find_if(Events.begin(), Events.end(), [&St](const EventPick& P) { return P.Id == St.Event; });
	const Rect P{540.0f, 324.0f, 520.0f, 328.0f};
	PanelAt(UI, C, P, 14.0f, pal::Line);
	if (Pick == Events.end())
	{
		UI.Text("Nothing on the schedule the bus can make.", P.X + P.W / 2.0f, P.Y + 150.0f, Ts(14.0f, 600, pal::Muted, Align::Center));
		return;
	}
	UI.Text(Pick->Name, P.X + 20.0f, P.Y + 34.0f, Ts(19.0f, 900, pal::Ink, Align::Left, Baseline::Alphabetic, false, P.W - 40.0f));
	UI.Text("Cards " + WhenText(Pick->Start, World) + Dot + (Pick->Live ? "sold now, you register when you get there." : "sold now, paid in when you register."), P.X + 20.0f, P.Y + 54.0f,
		Ts(12.0f, 600, pal::Muted, Align::Left, Baseline::Alphabetic, false, P.W - 40.0f));
	// What buyers believe.
	const rep::Public Rec = S.Record();
	stk::View V;
	V.Rec = &Rec;
	V.World = World;
	const int Ceiling = stk::Ceiling(V, B, std::string(), Pick->Live, Pick->BuyIn);
	const int Cap = stk::MarkupCap(Pick->Live, Pick->BuyIn);
	NetSpaced(C, "BUYERS EXPECT UP TO " + MarkupText(Ceiling), P.X + 20.0f, P.Y + 86.0f, 10.5f, 800, pal::Gold, 1.4f);
	UI.Text(stk::CeilingWhy(V, Pick->Live, Pick->BuyIn), P.X + 20.0f, P.Y + 104.0f, Ts(12.0f, 600, Hex(0xc3cedf), Align::Left, Baseline::Alphabetic, false, P.W - 40.0f));
	// The share (5% steps live, 1% online), stopping at 70% less what's already listed or swapped.
	const int Step = Pick->Live ? 5 : 1;
	const int Room = std::max(0, stk::MaxSoldPermille - stk::AskedOn(B, Pick->Id) - stk::SwappedOn(B, Pick->Id)) / 10;
	const int MaxPct = Room / Step * Step;
	St.Percent = std::min(St.Percent, MaxPct);
	if (St.Percent == 0 && MaxPct > 0)
	{
		St.Percent = std::min(MaxPct, Pick->Live ? 20 : 10);
	}
	St.Markup = St.Markup <= 0 ? Ceiling : std::min(Cap, std::max(1000, St.Markup)); // a new event starts at what buyers believe
	const Chips PerPct = (Pick->BuyIn * St.Markup + 50000) / 100000;
	UI.Text("Selling " + std::to_string(St.Percent) + "%", P.X + 20.0f, P.Y + 144.0f, Ts(16.0f, 800, pal::Ink));
	UI.Text(NetMoney(PerPct * St.Percent) + " at " + MarkupText(St.Markup), P.X + P.W - 20.0f, P.Y + 144.0f, Ts(16.0f, 800, pal::Gold, Align::Right, Baseline::Alphabetic, true));
	if (MaxPct > 0)
	{
		const double Got = Interactive ? UI.Slider("bk-pct", {P.X + 30.0f, P.Y + 156.0f, P.W - 60.0f, 24.0f}, St.Percent, Step, MaxPct, Step)
									   : static_cast<double>(St.Percent);
		St.Percent = static_cast<int>(Got + 0.5);
	}
	else
	{
		UI.Text("You've sold or swapped 70% of this one. Keep the rest.", P.X + 20.0f, P.Y + 172.0f, Ts(12.5f, 600, pal::Orange));
	}
	// The markup stepper.
	ButtonOpts Small;
	Small.Size = 15.0f;
	Small.Enabled = Interactive && St.Markup > 1000;
	if (UI.Button("bk-mk-down", {P.X + 20.0f, P.Y + 196.0f, 40.0f, 32.0f}, "-", Small))
	{
		St.Markup -= 10;
	}
	Small.Enabled = Interactive && St.Markup < Cap;
	if (UI.Button("bk-mk-up", {P.X + 132.0f, P.Y + 196.0f, 40.0f, 32.0f}, "+", Small))
	{
		St.Markup += 10;
	}
	UI.Text(MarkupText(St.Markup), P.X + 96.0f, P.Y + 218.0f, Ts(17.0f, 900, St.Markup > Ceiling ? pal::Orange : pal::Ink, Align::Center, Baseline::Alphabetic, true));
	UI.Text(St.Markup > Ceiling ? "Over what they believe: friends at par, maybe." : "Markup: inside what buyers believe.", P.X + 186.0f, P.Y + 218.0f,
		Ts(12.0f, 600, St.Markup > Ceiling ? pal::Orange : pal::Muted, Align::Left, Baseline::Alphabetic, false, P.W - 206.0f));
	UI.Text("Collected when you register. If you don't play it, nobody pays.", P.X + 20.0f, P.Y + 254.0f, Ts(12.0f, 600, pal::Dim, Align::Left, Baseline::Alphabetic, false, P.W - 40.0f));
	ButtonOpts List;
	List.Kind = ButtonKind::Primary;
	List.Size = 14.0f;
	List.Enabled = Interactive && St.Percent > 0;
	if (UI.Button("bk-list", {P.X + 20.0f, P.Y + 272.0f, 220.0f, 40.0f}, "List " + std::to_string(St.Percent) + "% at " + MarkupText(St.Markup), List))
	{
		const std::string Not = S.ListAction(Pick->Id, St.Percent * 10, St.Markup);
		Say(St, Not.empty() ? stk::Projection(S.Life.Stakes, Pick->Id, World) : Not, !Not.empty(), Now);
	}
	// What's already listed on it.
	const int Asked = stk::AskedOn(B, Pick->Id);
	if (Asked > 0)
	{
		const int Filled = stk::FilledOn(B, Pick->Id, World);
		UI.Text("Listed " + std::to_string(Asked / 10) + "%" + Dot + std::to_string(Filled / 10) + "% spoken for", P.X + 256.0f, P.Y + 290.0f,
			Ts(12.5f, 700, pal::Green, Align::Left, Baseline::Alphabetic, false, 170.0f));
		ButtonOpts Cancel;
		Cancel.Kind = ButtonKind::Ghost;
		Cancel.Size = 12.0f;
		Cancel.Enabled = Interactive;
		if (UI.Button("bk-cancel", {P.X + P.W - 92.0f, P.Y + 276.0f, 72.0f, 30.0f}, "Cancel", Cancel))
		{
			const std::string Not = S.CancelListing(Pick->Id);
			Say(St, Not.empty() ? std::string("Off the table. Nobody had paid, so nothing moves.") : Not, !Not.empty(), Now);
		}
	}

	// Swaps: who's in this event, and what's swapped.
	SectionAt(C, "SWAPS", 540.0f, 684.0f, Hex(0xc084fc));
	float Y = 698.0f;
	for (const stk::Swap& Sw : B.Swaps)
	{
		if (Sw.State == stk::SwapLapsed || Y > 840.0f)
		{
			continue;
		}
		const live::Occurrence O = live::FindOccurrence(Sw.EntryId);
		const std::string Line = std::to_string(Sw.Permille / 10) + "% with " + Sw.Partner + " in the " + (O.Valid() ? O.T->Short : std::string("event"));
		const std::string State = Sw.State == stk::SwapSettled ? (Sw.Net == 0 ? std::string("a wash") : Sw.Net > 0 ? Sw.Partner + " owes you " + NetMoney(Sw.Net) : "you owe " + NetMoney(-Sw.Net))
			: Sw.State == stk::SwapLive ? std::string("you're in; waiting on results") : std::string("if you both play");
		UI.RRect({540.0f, Y, 520.0f, 36.0f}, 9.0f, Rgba(255, 255, 255, 0.03f), pal::Line);
		NetAvatar(C, 560.0f, Y + 18.0f, 12.0f, Sw.Partner, Hex(0xc084fc));
		UI.Text(Line, 580.0f, Y + 23.0f, Ts(12.5f, 700, pal::Ink, Align::Left, Baseline::Alphabetic, false, 250.0f));
		UI.Text(State, 1046.0f, Y + 23.0f, Ts(12.0f, 600, pal::Muted, Align::Right, Baseline::Alphabetic, false, 200.0f));
		Y += 42.0f;
	}
	if (Pick->Live)
	{
		std::vector<std::string> Playing;
		const world::World& W = S.Living();
		for (const int Npc : W.RegisteredNow(Pick->Id))
		{
			const world::Npc* N = W.Get(Npc);
			// Mei and Rick swap; so do regulars who respect or trust the player (the bond's label says so).
			const world::Bond* Bd = N ? W.BondWith(Npc) : nullptr;
			const std::string Label = Bd ? Bd->Label() : std::string();
			if (N && (N->Name == "Mei" || N->Name == "Rick" || Label == "Respects you" || Label == "Trusts you"))
			{
				Playing.push_back(N->Name);
			}
		}
		float X = 540.0f;
		for (const std::string& Who : Playing)
		{
			if (X > 900.0f || Y > 900.0f)
			{
				break;
			}
			const int Share = Who == "Mei" || Who == "Rick" ? 10 : 5;
			ButtonOpts Sw;
			Sw.Size = 12.0f;
			Sw.Enabled = Interactive;
			const std::string Label = "Swap " + std::to_string(Share) + "% with " + Who;
			const float W2 = UI.Measure(Label, 12.0f, 700) + 28.0f;
			if (UI.Button("bk-swap-" + Who, {X, Y, W2, 32.0f}, Label, Sw))
			{
				const std::string Not = S.ProposeSwap(Pick->Id, Who, Share * 10);
				Say(St, Not.empty() ? Who + " is in. " + std::to_string(Share) + "% each, if you both play." : Who + ": \xE2\x80\x9C" + Not + "\xE2\x80\x9D", !Not.empty(), Now);
			}
			X += W2 + 8.0f;
		}
		if (Playing.empty() && Y <= 900.0f)
		{
			UI.Text("Nobody you swap with is down for this one yet.", 540.0f, Y + 20.0f, Ts(12.0f, 600, pal::Dim));
		}
	}
}

// ------------------------------------------------------------------ offers

void Offers(Ui& UI, Canvas& C, Session& S, PageState& St, double World, double Now, bool Interactive)
{
	SectionAt(C, "OFFERS", 1076.0f, 210.0f, pal::Gold);
	const std::vector<stk::Offer> Offers = S.Life.Stakes.Offers;
	if (Offers.empty())
	{
		UI.Text("No offers on the table.", 1326.0f, 260.0f, Ts(14.0f, 600, pal::Dim, Align::Center));
		return;
	}
	float Y = 224.0f;
	for (const stk::Offer& O : Offers)
	{
		const bool Sam = O.By == stk::Outfit::Sam;
		const float H = Sam ? 300.0f : 236.0f;
		if (Y + H > 950.0f)
		{
			break;
		}
		const std::string From = stk::OfferFrom(O);
		const Color Col = BackerColor(From);
		const Rect R{1076.0f, Y, 500.0f, H};
		PanelAt(UI, C, R, 14.0f, NetA(Col, 0.45f));
		NetAvatar(C, R.X + 34.0f, R.Y + 36.0f, 20.0f, O.By == stk::Outfit::LanternRow ? std::string("FoldEquityFred") : From, Col);
		UI.Text(From, R.X + 64.0f, R.Y + 32.0f, Ts(16.0f, 800, pal::Ink, Align::Left, Baseline::Alphabetic, false, 260.0f));
		const char* What = O.K == stk::Kind::Event ? "STAKE" : O.K == stk::Kind::Pieces ? "BUYS A PIECE" : O.K == stk::Kind::Swap ? "SWAP" : "PACKAGE";
		NetSpaced(C, What, R.X + 64.0f, R.Y + 50.0f, 9.5f, 800, Col, 1.3f);
		UI.Text(O.Expires > World ? "stands " + net::Countdown(O.Expires - World) : std::string("expiring"), R.X + R.W - 20.0f, R.Y + 32.0f, Ts(11.5f, 600, pal::Dim, Align::Right));
		float Ty = R.Y + 76.0f;
		if (!O.Line.empty())
		{
			Ty = NetParagraph(C, "\xE2\x80\x9C" + O.Line + "\xE2\x80\x9D", R.X + 20.0f, Ty, R.W - 40.0f, 13.0f, 600, Hex(0xe6edf7), 17.0f, 3) + 4.0f;
		}
		Ty = NetParagraph(C, stk::TermsLine(O), R.X + 20.0f, Ty + 4.0f, R.W - 40.0f, 12.0f, 600, pal::Muted, 16.0f, 3);
		if (Sam)
		{
			NetParagraph(C, stk::Speak("Sam", "offer.terms", 0), R.X + 20.0f, Ty + 6.0f, R.W - 40.0f, 12.0f, 700, pal::Orange, 16.0f, 3);
		}
		// Counter: one lever at a time, and the mood line before it's sent (§2.10).
		const bool CanCounter = O.K == stk::Kind::Event || O.K == stk::Kind::Package || O.K == stk::Kind::Pieces;
		int& Lever = St.Lever[O.Id];
		if (O.K == stk::Kind::Pieces)
		{
			Lever = 1;
		}
		else if (O.K == stk::Kind::Event)
		{
			Lever = 0;
		}
		else if (Lever != 0 && Lever != 2)
		{
			Lever = 0;
		}
		const stk::Terms Ask = stk::CounterTerms(O, Lever);
		const std::string Mood = rep::Mood(stk::Margin(O, Ask));
		const std::string Asking = Lever == 0 ? std::to_string(Ask.CutPermille / 10) + (O.K == stk::Kind::Event ? "% of the rest to you" : "% to you")
			: Lever == 2 ? "events up to " + net::BuyIn(Ask.CapOnline) : MarkupText(Ask.MarkupPermille) + " a piece";
		const float By = R.Y + R.H - 50.0f;
		if (CanCounter)
		{
			UI.Text("Counter: " + Asking + Dot + Mood, R.X + 20.0f, By - 10.0f,
				Ts(11.5f, 700, Mood == "Sounds open to it" ? pal::Green : Mood == "That's a stretch" ? pal::Orange : pal::Red, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f));
		}
		ButtonOpts Accept;
		Accept.Kind = ButtonKind::Primary;
		Accept.Size = 13.0f;
		Accept.Enabled = Interactive;
		if (UI.Button("bk-accept-" + O.Id, {R.X + 20.0f, By, 130.0f, 34.0f}, "Accept", Accept))
		{
			const std::string Not = S.AcceptStake(O.Id);
			Say(St, Not.empty() ? "Done. " + From + " has the details." : Not, !Not.empty(), Now);
		}
		ButtonOpts Ctr;
		Ctr.Size = 13.0f;
		Ctr.Enabled = Interactive && CanCounter && O.CounteredDay != net::DayOf(World);
		if (UI.Button("bk-counter-" + O.Id, {R.X + 160.0f, By, 130.0f, 34.0f}, O.CounteredDay == net::DayOf(World) ? "Countered" : "Counter", Ctr))
		{
			const std::string Not = S.CounterStake(O.Id, Lever);
			Say(St, Not.empty() ? "Sent. " + From + "'ll answer." : Not, !Not.empty(), Now);
		}
		if (O.K == stk::Kind::Package && Interactive)
		{
			// Which lever: the cut or the cap.
			const Rect Tg{R.X + 300.0f, By + 4.0f, 70.0f, 26.0f};
			if (UI.Clickable("bk-lever-" + O.Id, Tg).Clicked)
			{
				Lever = Lever == 0 ? 2 : 0;
			}
			UI.RRect(Tg, 8.0f, Rgba(255, 255, 255, 0.04f), pal::Line);
			UI.Text(Lever == 0 ? "the cut" : "the cap", Tg.X + Tg.W / 2.0f, Tg.Y + 14.0f, Ts(11.0f, 700, pal::Muted, Align::Center, Baseline::Middle));
		}
		ButtonOpts No;
		No.Kind = ButtonKind::Ghost;
		No.Size = 13.0f;
		No.Enabled = Interactive;
		if (UI.Button("bk-decline-" + O.Id, {R.X + R.W - 110.0f, By, 90.0f, 34.0f}, "Decline", No))
		{
			const std::string Not = stk::Decline(S.Life.Stakes, O.Id, World);
			S.Save();
			Say(St, Not.empty() ? "Passed. Nobody's offended." : Not, !Not.empty(), Now);
		}
		Y += H + 12.0f;
	}
}

// ------------------------------------------------------------------ the cards over the page

void WalkCard(Ui& UI, Canvas& C, Session& S, PageState& St, double Now, bool Interactive)
{
	const stk::Deal* D = nullptr;
	for (const stk::Deal& X : S.Life.Stakes.Deals)
	{
		D = X.Id == St.Confirm ? &X : D;
	}
	if (!D || D->State != stk::DealOpen)
	{
		St.Confirm.clear();
		return;
	}
	C.FillRect({0.0f, 64.0f, RiverLine::Width, RiverLine::Height - 64.0f}, Rgba(3, 7, 14, 0.72f));
	const Rect R{550.0f, 330.0f, 500.0f, 260.0f};
	PanelAt(UI, C, R, 16.0f, NetA(pal::Red, 0.6f));
	const Chips Makeup = std::max<Chips>(0, D->Funded - D->RollNow);
	const std::string Who = stk::BackerName(*D);
	UI.Text(Makeup > 0 ? "Walk away from " + NetMoney(Makeup) + " of makeup?" : "End the package with " + Who + "?", R.X + R.W / 2.0f, R.Y + 56.0f,
		Ts(20.0f, 900, pal::Ink, Align::Center, Baseline::Alphabetic, false, R.W - 40.0f));
	const std::string Body = D->T.MakeupIsDebt && Makeup > 0 ? "With Sam, makeup is a debt: 25% of every cash and every shift until it's paid. He spelled it out when you signed."
		: Makeup > 0 && D->By == stk::Outfit::LanternRow ? "You won't owe " + Who + " a dime. Everyone who stakes will hear about it, and " + Who + " won't deal with you for a year."
		: Makeup > 0 ? "You won't owe " + Who + " a dime. Everyone who stakes will hear about it, and " + Who + " won't forget it."
					 : "The roll's up: you chop first, then it ends clean. Nothing owed either way.";
	NetParagraph(C, Body, R.X + 40.0f, R.Y + 92.0f, R.W - 80.0f, 14.0f, 600, Hex(0xc3cedf), 20.0f, 4);
	ButtonOpts Walk;
	Walk.Kind = ButtonKind::Danger;
	Walk.Size = 14.0f;
	Walk.Enabled = Interactive;
	if (UI.Button("bk-walk-yes", {R.X + 40.0f, R.Y + R.H - 70.0f, 200.0f, 42.0f}, "Walk away", Walk))
	{
		const std::string Not = S.WalkAway(D->Id);
		Say(St, Not.empty() ? std::string("It's over. ") + (Makeup > 0 ? "Word gets around." : "Clean.") : Not, !Not.empty(), Now);
		St.Confirm.clear();
		return;
	}
	ButtonOpts Stay;
	Stay.Size = 14.0f;
	Stay.Enabled = Interactive;
	if (UI.Button("bk-walk-no", {R.X + R.W - 240.0f, R.Y + R.H - 70.0f, 200.0f, 42.0f}, "Stay", Stay))
	{
		St.Confirm.clear();
	}
}

void ReviewCard(Ui& UI, Canvas& C, Session& S, PageState& St, bool Interactive)
{
	const life::Letter* Last = nullptr;
	for (const life::Letter& L : S.Life.Inbox)
	{
		Last = L.From == "FoldEquityFred" ? &L : Last;
	}
	C.FillRect({0.0f, 64.0f, RiverLine::Width, RiverLine::Height - 64.0f}, Rgba(3, 7, 14, 0.72f));
	const Rect R{450.0f, 250.0f, 700.0f, 420.0f};
	PanelAt(UI, C, R, 16.0f, NetA(Hex(0xf2a93b), 0.6f));
	NetAvatar(C, R.X + 50.0f, R.Y + 52.0f, 26.0f, "FoldEquityFred", Hex(0xf2a93b));
	UI.Text(Last ? Last->Subject : std::string("FoldEquityFred's review"), R.X + 92.0f, R.Y + 50.0f, Ts(20.0f, 900, pal::Ink, Align::Left, Baseline::Alphabetic, false, R.W - 120.0f));
	UI.Text(Last ? "Monthly leak review" + Dot + net::DateLabel(net::DayOf(Last->At)) : std::string("Monthly leak review, with each chop"), R.X + 92.0f, R.Y + 72.0f, Ts(12.5f, 600, pal::Muted));
	const std::string Body = Last ? Last->Body : std::string("The first review comes with the first chop. Play me two hundred hands and we'll talk.");
	NetParagraph(C, Body, R.X + 40.0f, R.Y + 118.0f, R.W - 80.0f, 14.5f, 500, Hex(0xe6edf7), 21.0f, 12);
	ButtonOpts Close;
	Close.Size = 14.0f;
	Close.Enabled = Interactive;
	if (UI.Button("bk-review-close", {R.X + R.W - 160.0f, R.Y + R.H - 64.0f, 120.0f, 40.0f}, "Close", Close) || !Interactive)
	{
		St.Review = Interactive ? false : St.Review;
	}
}
} // namespace backing_detail

namespace backing_detail
{
/** Opens the sell panel on an event (the Embercrest card's "Sell action" button, ui-laptop's), at the share and markup it starts at. */
void Preselect(const Ui& UI, const std::string& EventId)
{
	PageState& St = StateFor(UI);
	St.Event = EventId;
	St.Percent = 0;
	St.Markup = 0;
}

/** The page, drawn by RiverLine::BackingPage (and the tests) between UI.Begin and UI.End. Interactive: false in the stream's preview. */
void DrawPage(Ui& UI, Canvas& C, Session& S, double Now, bool Interactive)
{
	PageState& St = StateFor(UI);
	const double World = S.WorldMinutes();
	Strip(UI, C, S, World);
	// While a card is up, the page under it takes no input.
	const bool Card = !St.Confirm.empty() || St.Review;
	Deals(UI, C, S, St, World, Now, Interactive && !Card);
	Sell(UI, C, S, St, World, Now, Interactive && !Card);
	Offers(UI, C, S, St, World, Now, Interactive && !Card);
	if (!St.Note.empty() && Now - St.NoteAt < 8.0)
	{
		const float A0 = C.GetAlpha();
		C.SetAlpha(A0 * static_cast<float>(std::min(1.0, (8.0 - (Now - St.NoteAt)) / 0.6)));
		const Rect R{540.0f, 902.0f, 520.0f, 40.0f};
		UI.RRect(R, 10.0f, St.NoteBad ? NetA(pal::Red, 0.14f) : NetA(pal::Green, 0.12f), St.NoteBad ? NetA(pal::Red, 0.5f) : NetA(pal::Green, 0.45f));
		UI.Text(St.Note, R.X + R.W / 2.0f, R.Y + R.H / 2.0f + 1.0f, Ts(12.5f, 700, St.NoteBad ? Hex(0xffb3ba) : Hex(0xb8f5c9), Align::Center, Baseline::Middle, false, R.W - 24.0f));
		C.SetAlpha(A0);
	}
	if (!St.Confirm.empty())
	{
		WalkCard(UI, C, S, St, Now, Interactive);
	}
	else if (St.Review)
	{
		ReviewCard(UI, C, S, St, Interactive);
	}
}
} // namespace backing_detail

void RiverLine::BackingPage(double Now)
{
	backing_detail::DrawPage(UI, *C, S, Now, !Previewing);
}
} // namespace ui
} // namespace ss
