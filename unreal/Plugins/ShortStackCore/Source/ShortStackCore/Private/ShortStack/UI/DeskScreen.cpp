#include "ShortStack/UI/TableHud.h"
#include "../StrictFloat.h"
#include "FrontEndShared.h"
#include "RiverLineShared.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Stake.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// The desk and the cage (next_segment_spec §6.6; declared in TableHud.h). core-tableui.
//
// One modal over the room, fed by data (the card room has a SaveData, not a Session) and handing back requests the host
// carries out through the desk's existing paths (DeskSignup, FinishSignup, LiveUnregister, LiveSettle). The desk: the
// clerk and the player's club card on the left; the open events on the right as honest cards (the fee split, the stack,
// the levels, late registration, the minimum, the field); who pays, with what doesn't apply greyed and said why; REGISTER,
// which prints a seat card for ten seconds; a refund for an entry that hasn't sat. The cage: the receipt (place, field,
// prize, time), the entry's own ledger lines (a backer's share, a sponsor's cut or bonus), the W-2G note when the
// winnings clear $5,000 over the buy-in (paperwork: nothing is withheld), and COLLECT.

namespace ss
{
namespace ui
{
namespace desk_detail
{
using namespace frontend_detail;

const Color Ash = Hex(rlnet_detail::EcAsh);
const Color Ice = Hex(rlnet_detail::EcIce);
const Color Midnight = Hex(rlnet_detail::EcMidnight);
const Color Navy = Hex(rlnet_detail::EcNavy);
const Color Slate = Hex(rlnet_detail::EcSlate);
const Color Hot = Hex(rlnet_detail::EcEmberHot);
const Color Gold = Hex(rlnet_detail::EcGold);
const Color Green = Hex(0x3ecf6e);
const float Tall = 1080.0f;

// The desk's own sentences (the clerk's lines when the host has none, the empty desk, the cage's tax note): also in
// TableUiCopy, for the copy lint.
const char* const ClerkDesk = "Card room's open. What are we playing tonight?";
const char* const ClerkCage = "Let me count it out for you.";
const char* const NothingOpen = "Nothing's open right now. The schedule's on the board by the door.";
const char* const W2gNote = "W-2G on file: winnings over $5,000 past the buy-in get reported. Nothing's withheld with your taxpayer number on the form.";

std::vector<std::pair<std::string, std::string>> DeskCopy()
{
	return {{"desk.clerk", ClerkDesk}, {"cage.clerk", ClerkCage}, {"desk.empty", NothingOpen}, {"cage.w2g", W2gNote}};
}

// Action codes for the pointer's targets.
constexpr int SelectEvent = 1000;
constexpr int SelectPay = 2000;
/** WHO PAYS rows drawn at once (the list scrolls with the choice past them). */
constexpr int PayShown = 4;
constexpr int DoRegister = 1;
constexpr int DoUnregister = 2;
constexpr int DoCollect = 3;
constexpr int DoLeave = 4;
constexpr int DoAppear = 5;

Color Fa(Color C, float A)
{
	C.A *= std::clamp(A, 0.0f, 1.0f);
	return C;
}

std::string Up(std::string S)
{
	for (char& Ch : S)
	{
		if (Ch >= 'a' && Ch <= 'z')
		{
			Ch = static_cast<char>(Ch - 'a' + 'A');
		}
	}
	return S;
}

std::string Dollars(Chips Cents)
{
	return Cents % 100 == 0 ? "$" + Grouped(Cents / 100) : Money(Cents);
}

/** Who pays, in the desk's words: the option's name and the line under it. */
std::pair<std::string, std::string> PayWords(const life::Funding& F, const live::Occurrence& O, Chips Bankroll)
{
	const std::string Short = O.Valid() ? O.T->Short : std::string("event");
	switch (F.From)
	{
	case life::Paid::Own:
		if (F.Permille > 0)
		{
			return {"YOUR MONEY, PIECES SOLD", live::FundingLine(F, Short)};
		}
		return {"YOUR MONEY", (O.Valid() ? Dollars(O.T->BuyInCents) : std::string("The buy-in")) + " from your " + Money(Bankroll) + "."};
	case life::Paid::Backer:
		return {F.Who.empty() ? std::string("A BACKER PUTS YOU IN") : Up(F.Who) == "BIG LOU" ? std::string("LOU'LL PUT YOU IN") : Up(F.Who) + " PUTS YOU IN", live::FundingLine(F, Short)};
	case life::Paid::Package: return {"FROM THE PACKAGE'S ROLL", live::FundingLine(F, Short)};
	case life::Paid::Sponsor: return {"COVERED BY " + (F.Who.empty() ? std::string("YOUR SPONSOR") : Up(F.Who)), live::FundingLine(F, Short)};
	case life::Paid::Ticket: return {"A TICKET", live::FundingLine(F, Short)};
	}
	return {"YOUR MONEY", std::string()};
}

/**
 * The option the desk starts on. What the player has already agreed to comes first: a stake they accepted for this
 * event (stake::Accepted: registering on anything else would call it off), then a package that can pay (the deal says
 * every event in its range plays on its roll). Otherwise the player's own money (pieces sold on it included), even when
 * it can't pay, so REGISTER says why. A standing offer or a cover is somebody's offer on their terms: at the desk the
 * player moves to it themselves, it's never preselected (GoToLive, with no choice to show, takes the first that pays and
 * shows it on the Burner's row beforehand). With none of these, the first that can pay.
 */
int DefaultPay(const DeskEvent& E)
{
	int Roll = -1;
	int Own = -1;
	int First = -1;
	for (size_t K = 0; K < E.Pay.size(); ++K)
	{
		const life::Funding& F = E.Pay[K].first;
		const bool Can = E.Pay[K].second.empty();
		if (Can && stake::Accepted(F))
		{
			return static_cast<int>(K); // a stake the player already shook on (GoToLive takes it too): never dropped by default
		}
		Roll = Roll < 0 && Can && F.From == life::Paid::Package ? static_cast<int>(K) : Roll;
		Own = Own < 0 && F.From == life::Paid::Own ? static_cast<int>(K) : Own;
		First = First < 0 && Can ? static_cast<int>(K) : First;
	}
	return Roll >= 0 ? Roll : Own >= 0 ? Own : First >= 0 ? First : 0;
}

bool Registered(const DeskInfo& I, const std::string& Id)
{
	return I.Entry && I.Entry->Id == Id && I.Entry->State == life::LiveEntry::Registered;
}

/** The appearance the desk offers: the first of the Embercrest's own (Meet the regulars) whose window is open at the desk's
 *  clock and that isn't done yet. Another sponsor's (the Lucky Penny's photo at Ray's counter) is made where it's due. */
const sponsor::Window* OpenDuty(const DeskInfo& I)
{
	for (const sponsor::Window& Wd : I.Duties)
	{
		if (Wd.Venue == "embercrest" && I.World >= Wd.From && I.World <= Wd.To && !Wd.Done)
		{
			return &Wd;
		}
	}
	return nullptr;
}

/** Registered and not a hand played (no checkpoint yet): the room still gives the money back. */
bool Refundable(const DeskInfo& I, const std::string& Id)
{
	return Registered(I, Id) && I.Entry->Checkpoint.empty();
}
} // namespace desk_detail

void DeskScreen::Open(double Now, const std::string& Window)
{
	Which = Window;
	OpenedAt = Now;
	Pending.clear();
	Sel = 0;
	Pay = -1;
	Focus = 0;
	ActedAt = -10.0;
	Said.clear();
	SaidAt = -10.0;
	Hits.clear();
	Last = DeskInfo();
	EntryId.clear();
	EntryOpen = false;
	EntryFresh = false;
}

void DeskScreen::Act(int Code, double Now)
{
	using namespace desk_detail;
	if (Which.empty() || Now - OpenedAt < 0.25)
	{
		return; // the press that opened it
	}
	const bool Desk = Which == "desk";
	if (Code >= SelectPay)
	{
		Pay = Code - SelectPay;
		Focus = 1;
		return;
	}
	if (Code >= SelectEvent)
	{
		if (Sel != Code - SelectEvent)
		{
			Sel = Code - SelectEvent;
			Pay = -1;
		}
		Focus = 0;
		return;
	}
	DeskRequest R;
	switch (Code)
	{
	case DoRegister:
	{
		if (!Desk || Sel < 0 || Sel >= static_cast<int>(Last.Open.size()))
		{
			return;
		}
		const DeskEvent& E = Last.Open[static_cast<size_t>(Sel)];
		const int Choice = Pay >= 0 ? Pay : DefaultPay(E);
		if (EntryOpen && EntryId == E.O.Id)
		{
			return;
		}
		if (Choice < static_cast<int>(E.Pay.size()) && !E.Pay[static_cast<size_t>(Choice)].second.empty())
		{
			Said = E.Pay[static_cast<size_t>(Choice)].second;
			SaidAt = Now;
			return;
		}
		R.K = DeskRequest::Register;
		R.Id = E.O.Id;
		R.PayOption = Choice;
		R.Fund = Choice < static_cast<int>(E.Pay.size()) ? E.Pay[static_cast<size_t>(Choice)].first : life::Funding();
		ActedAt = Now;
		break;
	}
	case DoUnregister:
		// Only before a hand is dealt to the player: once they've played, the money's in the game.
		if (EntryId.empty() || !EntryOpen || !EntryFresh)
		{
			return;
		}
		R.K = DeskRequest::Unregister;
		R.Id = EntryId;
		break;
	case DoCollect:
		if (Desk || EntryId.empty() || ActedAt > OpenedAt)
		{
			return;
		}
		R.K = DeskRequest::Collect;
		R.Id = EntryId;
		ActedAt = Now;
		break;
	case DoAppear:
	{
		// The appearance on the card: the first whose window is open now and isn't done.
		const sponsor::Window* Open = OpenDuty(Last);
		if (!Open)
		{
			return;
		}
		R.K = DeskRequest::Appear;
		R.Id = Open->Venue;
		break;
	}
	default:
		R.K = DeskRequest::Leave;
		break;
	}
	Pending.push_back(R);
	if (R.K == DeskRequest::Leave)
	{
		Which.clear(); // stepped away: the host hands control back when it takes the request
	}
}

void DeskScreen::Key(const std::string& Name, double Now)
{
	using namespace desk_detail;
	if (Which.empty())
	{
		return;
	}
	const bool Desk = Which == "desk";
	const int Events = static_cast<int>(Last.Open.size());
	if (Name == "Escape" || Name == "Back" || Name == "B")
	{
		Act(DoLeave, Now);
		return;
	}
	const bool Confirm = Name == "Enter" || Name == "Space" || Name == "A";
	if (!Desk)
	{
		if (Confirm)
		{
			Act(DoCollect, Now);
		}
		return;
	}
	if ((Name == "Up" || Name == "Down") && Events > 0)
	{
		Sel = std::clamp(Sel + (Name == "Up" ? -1 : 1), 0, Events - 1);
		Pay = -1;
		Focus = 0;
	}
	else if ((Name == "Left" || Name == "Right" || Name == "Q" || Name == "E" || Name == "TabPrev" || Name == "TabNext") && Sel >= 0 && Sel < Events)
	{
		const DeskEvent& E = Last.Open[static_cast<size_t>(Sel)];
		const int N = static_cast<int>(E.Pay.size());
		if (N > 0)
		{
			const int From = Pay >= 0 ? Pay : DefaultPay(E);
			const bool Back = Name == "Left" || Name == "Q" || Name == "TabPrev";
			Pay = std::clamp(From + (Back ? -1 : 1), 0, N - 1);
			Focus = 1;
		}
	}
	else if (Confirm)
	{
		Act(DoRegister, Now);
	}
	else if (Name == "R" || Name == "Reset" || Name == "Y")
	{
		Act(DoUnregister, Now); // the glyph on the refund: R, or Y on the pad
	}
	else if (Name == "PutBack" || Name == "F" || Name == "X")
	{
		Act(DoAppear, Now); // the glyph on Meet the regulars: F, or X on the pad
	}
}

std::vector<DeskRequest> DeskScreen::TakeRequests()
{
	std::vector<DeskRequest> Out;
	Out.swap(Pending);
	return Out;
}

void DeskScreen::Draw(Canvas& C, const DeskInfo& I, double Now)
{
	using namespace desk_detail;
	Last = I;
	Last.Entry = nullptr; // only for this frame: the keys between frames act on EntryId
	EntryId = I.Entry ? I.Entry->Id : std::string();
	EntryOpen = I.Entry && I.Entry->State == life::LiveEntry::Registered;
	EntryFresh = EntryOpen && Refundable(I, I.Entry->Id);
	Hits.clear();
	if (Which.empty())
	{
		return;
	}
	const float W = C.Width();
	const bool Desk = Which == "desk";
	const float In = Ease((Now - OpenedAt) / 0.35);
	C.FillRect({0.0f, 0.0f, W, Tall}, Paint(Rgba(3, 5, 10, 0.62f * In)));
	const float Pw = std::min(1500.0f, W - 116.0f);
	const float Px = W * 0.5f - Pw * 0.5f;
	const float Py = 120.0f + (1.0f - In) * 30.0f;
	const float Ph = 820.0f;
	C.FillRoundRect({Px, Py, Pw, Ph}, 18.0f, Paint::Linear({Px, Py}, {Px, Py + Ph}, Fa(Navy, 0.97f * In), Fa(Midnight, 0.97f * In)));
	C.StrokeRoundRect({Px + 0.5f, Py + 0.5f, Pw - 1.0f, Ph - 1.0f}, 18.0f, Fa(Ash, 0.14f * In), 1.0f);
	rlnet_detail::NetEmbercrest(C, Px + 46.0f, Py + 46.0f, 40.0f, Navy);
	TrackedText(C, Desk ? "THE DESK" : "THE CAGE", Px + 80.0f, Py + 54.0f, 22.0f, 900, Fa(Ash, In), 4.0f);
	TrackedText(C, "THE EMBERCREST \xC2\xB7 POKER ROOM", Px + Pw - 30.0f, Py + 52.0f, 13.0f, 800, Fa(Ice, In), 2.4f, Align::Right);
	C.FillRect({Px + 30.0f, Py + 76.0f, Pw - 60.0f, 2.0f}, Paint(Fa(Hot, 0.8f * In)));
	// Pointer: press and release on the same target acts.
	auto Hit = [&](const Rect& R, int Code) { Hits.push_back({R, Code}); };
	const float Lw = std::min(470.0f, Pw * 0.36f);
	const float Lx = Px + 30.0f;
	const float Top = Py + 100.0f;
	// The left: the clerk's line, then the club card (printing on an upgrade).
	{
		const std::string Line = !Said.empty() && Now - SaidAt < 4.0 ? Said
								 : !I.ClerkLine.empty()                ? I.ClerkLine
								 : Desk                                ? std::string(ClerkDesk)
																	   : std::string(ClerkCage);
		const std::vector<std::string> Rows = WrapText(C, Line, Lw - 40.0f, 19.0f, 500);
		const float Bh = 50.0f + 28.0f * static_cast<float>(std::min<size_t>(Rows.size(), 4));
		C.FillRoundRect({Lx, Top, Lw, Bh}, 12.0f, Paint(Fa(Slate, 0.7f * In)));
		TrackedText(C, Desk ? "THE DESK" : "THE CASHIER", Lx + 20.0f, Top + 28.0f, 13.0f, 800, Fa(Gold, In), 2.4f);
		float Ry = Top + 30.0f;
		for (size_t K = 0; K < Rows.size() && K < 4; ++K)
		{
			Ry += 28.0f;
			C.Text(Rows[K], Lx + 20.0f, Ry, Ts(19.0f, 500, Fa(Ash, In), Align::Left, Baseline::Alphabetic, false, Lw - 40.0f));
		}
		float Cy = Top + Bh + 24.0f;
		if (!I.Card.Name.empty())
		{
			const float Cw = Lw - 60.0f;
			const float Ch = Cw / 1.586f;
			// Printing: it slides up out of the slot.
			const float Print = I.CardPrinting ? Ease((Now - OpenedAt - 0.4) / 1.4) : 1.0f;
			C.FillRoundRect({Lx + 20.0f, Cy + Ch - 6.0f, Cw + 20.0f, 10.0f}, 5.0f, Paint(Fa(Hex(0x000000), 0.55f * In)));
			C.PushClip({Lx, Cy - 10.0f, Lw, Ch + 12.0f});
			C.Save();
			C.Translate(0.0f, (1.0f - Print) * Ch);
			badgeart::ClubCard(C, {Lx + 30.0f, Cy, Cw, Ch}, I.Card, Now);
			C.Restore();
			C.PopClip();
			Cy += Ch + 24.0f;
		}
		// Meet the regulars, during its window.
		if (const sponsor::Window* Open = OpenDuty(I))
		{
			const sponsor::Window& Wd = *Open;
			const Rect B{Lx, Cy, Lw, 56.0f};
			C.FillRoundRect(B, 12.0f, Paint(Fa(Gold, 0.16f * In)));
			C.StrokeRoundRect({B.X + 0.5f, B.Y + 0.5f, B.W - 1.0f, B.H - 1.0f}, 12.0f, Fa(Gold, 0.6f * In), 1.0f);
			const float Gw = Glyph(C, Gamepad ? "X" : "F", Lx + 16.0f, Cy + 37.0f, In);
			C.Text(Wd.Label.empty() ? std::string("Meet the regulars (20 min)") : Wd.Label, Lx + 30.0f + Gw, Cy + 34.0f, Ts(18.0f, 700, Fa(Ash, In), Align::Left, Baseline::Alphabetic, false, Lw - 50.0f - Gw));
			Hit(B, DoAppear);
			Cy += 70.0f;
		}
	}
	const float Rx = Lx + Lw + 30.0f;
	const float Rw = Px + Pw - 30.0f - Rx;
	if (Desk)
	{
		// The events, as the room's honest cards.
		TrackedText(C, "TONIGHT AT THE DESK", Rx, Top + 14.0f, 13.0f, 800, Fa(Ice, In), 2.4f);
		float Ey = Top + 30.0f;
		if (I.Open.empty())
		{
			const std::string Why = I.Why.empty() ? std::string(NothingOpen) : I.Why;
			for (const std::string& L : WrapText(C, Why, Rw, 19.0f, 400))
			{
				Ey += 30.0f;
				C.Text(L, Rx, Ey, Ts(19.0f, 400, Fa(Ash, 0.8f * In)));
			}
		}
		Sel = std::clamp(Sel, 0, std::max(0, static_cast<int>(I.Open.size()) - 1));
		// Two cards at a time (the room rarely has more open at once): the window follows the selection.
		const size_t FirstShown = static_cast<size_t>(std::clamp(Sel - 1, 0, std::max(0, static_cast<int>(I.Open.size()) - 2)));
		for (size_t K = FirstShown; K < I.Open.size() && K < FirstShown + 2; ++K)
		{
			const DeskEvent& E = I.Open[K];
			if (!E.O.Valid())
			{
				continue;
			}
			const live::EventTemplate& T = *E.O.T;
			const bool On = static_cast<int>(K) == Sel;
			const bool Mine = Registered(I, E.O.Id);
			const Rect Card{Rx, Ey, Rw, 112.0f};
			C.FillRoundRect(Card, 12.0f, Paint(On ? Fa(Slate, 0.95f * In) : Fa(Midnight, 0.7f * In)));
			C.FillRoundRect({Card.X, Card.Y, 5.0f, Card.H}, 2.5f, Paint(Fa(rlnet_detail::LiveColor(T), In)));
			if (On)
			{
				C.StrokeRoundRect({Card.X + 1.0f, Card.Y + 1.0f, Card.W - 2.0f, Card.H - 2.0f}, 12.0f, Fa(Hot, In), 2.0f);
			}
			C.Text(T.Name, Card.X + 22.0f, Card.Y + 34.0f, Ts(22.0f, 800, Fa(Ash, In), Align::Left, Baseline::Alphabetic, false, Rw * 0.6f));
			const std::string When = Up(live::DayWord(E.O, I.World)) + " \xC2\xB7 " + net::TimeLabel(E.O.Start);
			C.Text(When, Card.X + Rw - 20.0f, Card.Y + 33.0f, Ts(17.0f, 700, Fa(Gold, In), Align::Right));
			const std::string Split = Dollars(T.BuyInCents - T.FeeCents) + " + " + Dollars(T.FeeCents) + " \xC2\xB7 " + Grouped(T.StartingStack) + " CHIPS \xC2\xB7 " +
				std::to_string(static_cast<int>(T.LevelMinutes)) + "-MIN LEVELS \xC2\xB7 BREAK EVERY " + std::to_string(T.BreakEvery);
			C.Text(Split, Card.X + 22.0f, Card.Y + 64.0f, Ts(15.0f, 600, Fa(Ash, 0.9f * In), Align::Left, Baseline::Alphabetic, false, Rw - 44.0f));
			const int Field = K < I.Fields.size() ? I.Fields[K] : 0;
			const std::string Reg = live::LateRegLeft(E.O, I.World) > 0.0 ? "LATE REG TO " + net::TimeLabel(E.O.LateRegEnds) : std::string("REGISTRATION CLOSED");
			const std::string Crowd = Field > 0 ? std::to_string(Field) + " IN SO FAR" : "USUALLY " + std::to_string(T.FieldLo) + " TO " + std::to_string(T.FieldHi);
			const std::string Facts = Reg + " \xC2\xB7 " + Crowd + " \xC2\xB7 RUNS WITH " + std::to_string(T.MinEntrants) + " OR MORE";
			C.Text(Facts, Card.X + 22.0f, Card.Y + 92.0f, Ts(13.0f, 700, Fa(Ice, In), Align::Left, Baseline::Alphabetic, false, Rw - 44.0f - (Mine ? 150.0f : 0.0f)));
			if (Mine)
			{
				TrackedText(C, "YOU'RE IN", Card.X + Rw - 20.0f, Card.Y + 92.0f, 13.0f, 900, Fa(Green, In), 2.4f, Align::Right);
			}
			Hit(Card, SelectEvent + static_cast<int>(K));
			Ey += Card.H + 12.0f;
		}
		// Who pays, for the chosen event.
		if (Sel < static_cast<int>(I.Open.size()) && I.Open[static_cast<size_t>(Sel)].O.Valid())
		{
			const DeskEvent& E = I.Open[static_cast<size_t>(Sel)];
			const int Choice = Pay >= 0 ? std::min(Pay, static_cast<int>(E.Pay.size()) - 1) : DefaultPay(E);
			Ey += 10.0f;
			TrackedText(C, "WHO PAYS", Rx, Ey + 14.0f, 13.0f, 800, Fa(Ice, In), 2.4f);
			if (E.Pay.size() > 1)
			{
				const float Gw = GlyphWidth(C, Gamepad ? "LB" : "LEFT");
				Glyph(C, Gamepad ? "LB" : "LEFT", Rx + Rw - Gw * 2.0f - 8.0f, Ey + 22.0f, 0.8f * In);
				Glyph(C, Gamepad ? "RB" : "RIGHT", Rx + Rw - Gw, Ey + 22.0f, 0.8f * In);
			}
			Ey += 34.0f;
			// One row an option: the name, then the line (or why it can't pay, in ember). Four at a time: a backer, each
			// sponsor's cover, a package and the player's own money can run past four, and the window follows the choice
			// (LB/RB step through them all), so the chosen row is always the one drawn lit.
			const float TitleW = std::min(330.0f, Rw * 0.42f);
			const int PayRows = static_cast<int>(E.Pay.size());
			const int FirstRow = std::clamp(Choice - 2, 0, std::max(0, PayRows - PayShown));
			for (int K = FirstRow; K < PayRows && K < FirstRow + PayShown; ++K)
			{
				const std::pair<life::Funding, std::string>& Op = E.Pay[static_cast<size_t>(K)];
				const bool Can = Op.second.empty();
				const bool On = K == Choice;
				const Rect B{Rx, Ey, Rw, 48.0f};
				C.FillRoundRect(B, 10.0f, Paint(On ? Fa(Hot, (Can ? 0.22f : 0.1f) * In) : Fa(Midnight, 0.7f * In)));
				C.StrokeRoundRect({B.X + 0.5f, B.Y + 0.5f, B.W - 1.0f, B.H - 1.0f}, 10.0f, On ? Fa(Hot, In) : Fa(Ash, 0.14f * In), On ? 2.0f : 1.0f);
				C.FillCircle(B.X + 22.0f, B.Y + 24.0f, 6.0f, Paint(On ? Fa(Hot, In) : Fa(Ash, 0.25f * In)));
				const std::pair<std::string, std::string> Words = PayWords(Op.first, E.O, I.Bankroll);
				C.Text(Words.first, B.X + 40.0f, B.Y + 30.0f, Ts(15.0f, 800, Fa(Can ? Ash : Ice, (Can ? 1.0f : 0.6f) * In), Align::Left, Baseline::Alphabetic, false, TitleW - 40.0f));
				const std::string Under = Can ? Words.second : Op.second;
				C.Text(Under, B.X + TitleW + 10.0f, B.Y + 30.0f, Ts(14.0f, 500, Fa(Can ? Ice : Hot, 0.95f * In), Align::Left, Baseline::Alphabetic, false, Rw - TitleW - 26.0f));
				Hit(B, SelectPay + K);
				Ey += 56.0f;
			}
			Ey += 14.0f;
			// REGISTER, or the refund once in.
			const bool Mine = Registered(I, E.O.Id);
			const bool CanPay = Choice < static_cast<int>(E.Pay.size()) ? E.Pay[static_cast<size_t>(Choice)].second.empty() : E.Pay.empty();
			const Rect Go{Rx, Ey, 300.0f, 58.0f};
			if (!Mine)
			{
				C.FillRoundRect(Go, 12.0f, Paint(CanPay ? Fa(Hot, In) : Fa(Slate, In)));
				const float Gw = Glyph(C, Gamepad ? "A" : "ENTER", Go.X + 16.0f, Go.Y + 39.0f, In);
				TrackedText(C, "REGISTER", Go.X + 30.0f + Gw, Go.Y + 36.0f, 18.0f, 900, CanPay ? Hex(0x14070a) : Fa(Ice, In), 3.0f);
				Hit(Go, DoRegister);
			}
			else if (Refundable(I, E.O.Id))
			{
				C.FillRoundRect(Go, 12.0f, Paint(Fa(Midnight, 0.85f * In)));
				C.StrokeRoundRect({Go.X + 0.5f, Go.Y + 0.5f, Go.W - 1.0f, Go.H - 1.0f}, 12.0f, Fa(Ash, 0.3f * In), 1.0f);
				const float Gw = Glyph(C, Gamepad ? "Y" : "R", Go.X + 16.0f, Go.Y + 39.0f, In);
				C.Text("Leave and get refunded", Go.X + 30.0f + Gw, Go.Y + 36.0f, Ts(17.0f, 700, Fa(Ash, In), Align::Left, Baseline::Alphabetic, false, Go.W - 46.0f - Gw));
				Hit(Go, DoUnregister);
			}
		}
		// The seat card prints at the bottom left for ten seconds after registering.
		if (I.Entry && I.Entry->State == life::LiveEntry::Registered && Now - ActedAt >= 0.0 && Now - ActedAt < 10.0 && !I.Seat.empty())
		{
			const double Age = Now - ActedAt;
			const float A = std::min(Ease(Age / 0.5), Ease((10.0 - Age) / 0.6));
			const std::string Seat = Up(I.Entry->Name) + " \xC2\xB7 " + I.Seat;
			const float Sw = std::min(Lw, TrackedWidth(C, Seat, 13.0f, 800, 1.6f) + 40.0f);
			const float Sy = Py + Ph - 82.0f + (1.0f - A) * 16.0f;
			C.FillRoundRect({Lx, Sy, Sw, 52.0f}, 6.0f, Paint(Fa(Hex(0xf4efe4), A)));
			C.FillRect({Lx, Sy, Sw, 4.0f}, Paint(Fa(Hot, A)));
			if (TrackedWidth(C, Seat, 13.0f, 800, 1.6f) + 40.0f <= Lw)
			{
				TrackedText(C, Seat, Lx + 20.0f, Sy + 33.0f, 13.0f, 800, Fa(Hex(0x1a1410), A), 1.6f);
			}
			else
			{
				C.Text(Seat, Lx + 20.0f, Sy + 33.0f, Ts(13.0f, 800, Fa(Hex(0x1a1410), A), Align::Left, Baseline::Alphabetic, false, Sw - 40.0f));
			}
		}
	}
	else
	{
		// The cage: the receipt, the entry's own lines, COLLECT.
		const float Rw2 = std::min(Rw, 560.0f);
		const Chips PaidIn = I.Entry ? I.Entry->PaidCents : 0;
		const std::string Tax = W2gNote;
		const size_t TaxRows = I.PrizeCents - PaidIn > 500000 ? WrapText(C, Tax, Rw2 - 60.0f, 13.0f, 500).size() : 0;
		// The paper is as long as what's printed on it.
		size_t Lines = 0;
		for (const life::LedgerEntry& L : I.Receipt)
		{
			Lines += L.Kind != 4 ? 1u : 0u; // (the prize is its own row)
		}
		const float Rh = 236.0f + 30.0f * static_cast<float>(Lines) + (TaxRows > 0 ? 30.0f + 19.0f * static_cast<float>(TaxRows) : 0.0f) + 30.0f;
		const float Ry = Top + 10.0f;
		C.FillRoundRect({Rx, Ry, Rw2, Rh}, 6.0f, Paint(Fa(Hex(0xf4efe4), In)));
		const Color Ink = Hex(0x1a1410);
		const Color Faint = Hex(0x6b6156);
		float Y = Ry + 44.0f;
		TrackedText(C, "THE EMBERCREST \xC2\xB7 CAGE", Rx + Rw2 * 0.5f, Y, 15.0f, 900, Fa(Ink, In), 3.0f, Align::Center);
		Y += 34.0f;
		const std::string Event = I.Entry ? I.Entry->Name : std::string();
		C.Text(Event, Rx + Rw2 * 0.5f, Y, Ts(17.0f, 700, Fa(Ink, In), Align::Center, Baseline::Alphabetic, false, Rw2 - 40.0f));
		Y += 30.0f;
		const std::string Place = I.Place > 0 ? Ordinal(I.Place) + " of " + std::to_string(I.Entrants) : std::string();
		const std::string When = I.FinishedAt > 0.0 ? net::TimeLabel(I.FinishedAt) : std::string();
		C.Text(Place + (When.empty() ? "" : " \xC2\xB7 " + When), Rx + Rw2 * 0.5f, Y, Ts(15.0f, 500, Fa(Faint, In), Align::Center));
		Y += 22.0f;
		C.FillRect({Rx + 24.0f, Y, Rw2 - 48.0f, 1.0f}, Paint(Fa(Faint, 0.6f * In)));
		Y += 34.0f;
		C.Text("PRIZE", Rx + 30.0f, Y, Ts(15.0f, 800, Fa(Ink, In)));
		C.Text(Money(I.PrizeCents), Rx + Rw2 - 30.0f, Y, Ts(18.0f, 800, Fa(Ink, In), Align::Right, Baseline::Alphabetic, true));
		Chips Net = I.PrizeCents;
		for (const life::LedgerEntry& L : I.Receipt)
		{
			if (L.Kind == 4)
			{
				continue; // a prize line: the PRIZE row says it, and YOURS counts it once
			}
			Y += 30.0f;
			C.Text(L.Label, Rx + 30.0f, Y, Ts(15.0f, 500, Fa(Ink, In), Align::Left, Baseline::Alphabetic, false, Rw2 - 200.0f));
			C.Text(Money(L.Amount), Rx + Rw2 - 30.0f, Y, Ts(16.0f, 700, Fa(L.Amount < 0 ? Hex(0x9e2f0c) : Ink, In), Align::Right, Baseline::Alphabetic, true));
			Net += L.Amount;
		}
		Y += 20.0f;
		C.FillRect({Rx + 24.0f, Y, Rw2 - 48.0f, 1.0f}, Paint(Fa(Faint, 0.6f * In)));
		Y += 32.0f;
		C.Text("YOURS", Rx + 30.0f, Y, Ts(16.0f, 900, Fa(Ink, In)));
		C.Text(Money(Net), Rx + Rw2 - 30.0f, Y, Ts(20.0f, 900, Fa(Ink, In), Align::Right, Baseline::Alphabetic, true));
		// Paperwork over $5,000 of winnings past the buy-in: reported, nothing withheld.
		if (TaxRows > 0)
		{
			Y += 40.0f;
			for (const std::string& L : WrapText(C, Tax, Rw2 - 60.0f, 13.0f, 500))
			{
				C.Text(L, Rx + 30.0f, Y, Ts(13.0f, 500, Fa(Faint, In)));
				Y += 19.0f;
			}
		}
		const Rect Go{Rx, Ry + Rh + 24.0f, 340.0f, 58.0f};
		const bool Done = ActedAt > OpenedAt; // counted out: COLLECT doesn't come back while the cage is open (nor does the key)
		C.FillRoundRect(Go, 12.0f, Paint(Done ? Fa(Slate, In) : Fa(Hot, In)));
		const float Gw = Glyph(C, Gamepad ? "A" : "ENTER", Go.X + 16.0f, Go.Y + 39.0f, In);
		TrackedText(C, Done ? "COUNTED OUT" : "COLLECT " + Dollars(Net), Go.X + 30.0f + Gw, Go.Y + 36.0f, 18.0f, 900, Done ? Fa(Ice, In) : Hex(0x14070a), 2.4f);
		Hit(Go, DoCollect);
	}
	// The footer: what the keys do here.
	const float Fy = Py + Ph - 26.0f;
	float Fx = Px + Pw - 30.0f;
	auto Hint = [&](const char* Key, const char* PadKey, const char* Label) {
		const float Tw = TrackedWidth(C, Label, 13.0f, 700, 2.4f);
		const float Gw = GlyphWidth(C, Gamepad ? PadKey : Key);
		Fx -= Tw;
		TrackedText(C, Label, Fx, Fy - 2.0f, 13.0f, 700, Fa(Ice, 0.9f * In), 2.4f);
		Fx -= Gw + 10.0f;
		Glyph(C, Gamepad ? PadKey : Key, Fx, Fy, 0.9f * In);
		Fx -= 34.0f;
	};
	Hint("ESC", "B", "STEP AWAY");
	if (Desk)
	{
		Hint("ENTER", "A", "REGISTER");
		Hint("RIGHT", "RB", "WHO PAYS");
		Hint("DOWN", "DPAD", "EVENT");
	}
	else
	{
		Hint("ENTER", "A", "COLLECT");
	}
	// The pointer.
	if (Ptr.Pressed)
	{
		PressX = Ptr.X;
		PressY = Ptr.Y;
	}
	if (Ptr.Released)
	{
		for (const std::pair<Rect, int>& H : Hits)
		{
			if (Inside(H.first, Ptr.X, Ptr.Y) && Inside(H.first, PressX, PressY))
			{
				Act(H.second, Now);
				break;
			}
		}
	}
	Ptr.EndFrame();
}
} // namespace ui
} // namespace ss
