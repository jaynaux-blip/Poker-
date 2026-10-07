// Northside Pawn & Loan (the realism update's spec, §5.8 and §7.4): SELL at the counter (what the hero could bring
// Augie, the loupe's beat, then his two offers, and the second ask on what can't be bought back), the pink carbon pawn
// ticket, TICKETS at the cage (in term, in grace in red; REDEEM, and RENEW), and BUY at the cases and the electronics
// (GEAR, CURIOS, and YOURS: the hero's forfeits and the storage auction's lots), with NOT IN STOCK and "Needs a room".
#include "ShopFrame.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Talk.h"
#include "ShortStack/Game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace ss
{
namespace ui
{
namespace shopframe
{
namespace
{
using namespace frontend_detail;

constexpr int SellTab = 0;
constexpr int TicketsTab = 1;
constexpr int GearTab = 2;
constexpr int CuriosTab = 3;
constexpr int YoursTab = 4;
constexpr double AppraiseSeconds = 1.5;

std::string Capital(std::string S)
{
	if (!S.empty() && S[0] >= 'a' && S[0] <= 'z')
	{
		S[0] = static_cast<char>(S[0] - 32);
	}
	return S;
}

std::string TicketNo(int N)
{
	return "#" + shop::TicketNumber(N);
}

const char* KindWord(shop::LotKind K)
{
	switch (K)
	{
	case shop::LotKind::Gear: return "Gear";
	case shop::LotKind::Memento: return "Memento";
	case shop::LotKind::Award: return "Your title's piece";
	case shop::LotKind::Keepsake: return "Keepsake";
	case shop::LotKind::Garment: return "A rare find";
	case shop::LotKind::Curio: return "Curio";
	}
	return "";
}

class PawnPage final : public Page
{
public:
	explicit PawnPage(Session& InSession) : Page(InSession, "pawn") {}

protected:
	std::vector<std::string> TabNames() const override { return {"SELL", "TICKETS", "GEAR", "CURIOS", "YOURS"}; }

	void Opened(double Now, const std::string& PageId) override
	{
		Reset();
		if (PageId == "pawn-cage" || PageId == "TICKETS")
		{
			Tab = TicketsTab;
		}
		else if (PageId == "pawn-cases" || PageId == "CURIOS")
		{
			Tab = CuriosTab;
		}
		else if (PageId == "pawn-electronics" || PageId == "GEAR" || PageId == "BUY")
		{
			Tab = GearTab;
		}
		else if (PageId == "YOURS")
		{
			Tab = YoursTab;
		}
		Shot();
		if (PageId == "pawn-guitars")
		{
			Say("guitars", Now);
		}
		else if (HasAuctionLot() && (Tab == YoursTab || Tab == GearTab))
		{
			Say("appraise:lot114", Now);
		}
	}

	void TabChanged(double Now) override
	{
		Reset();
		Shot();
		if (Tab == YoursTab && HasAuctionLot())
		{
			Say("appraise:lot114", Now);
		}
	}

	bool TabsHeld() const override { return !Appraising.empty() || Asking; }

	bool StepBack(double Now) override
	{
		if (Asking)
		{
			Asking = false;
			return true;
		}
		if (!Appraising.empty())
		{
			Appraising.clear();
			SayText("Suit yourself.", Now);
			return true;
		}
		return false;
	}

	bool PageKey(const std::string& Name, double Now) override
	{
		if (Asking)
		{
			if (Name == "Left" || Name == "Right" || Name == "Up" || Name == "Down")
			{
				AskSell = !AskSell;
			}
			else if (Name == "Enter" || Name == "Space" || Name == "Tab" || Name == "Reset")
			{
				Answer(Now);
			}
			else if (Name == "Backspace" || Name == "Delete" || Name == "PutBack")
			{
				Asking = false;
			}
			return true;
		}
		if (!Appraising.empty())
		{
			if (!Appraised(Now))
			{
				return true; // he's still turning it under the loupe
			}
			if (Name == "Left" || Name == "Right" || Name == "Up" || Name == "Down")
			{
				TakeLoan = !TakeLoan;
			}
			else if (Name == "Enter" || Name == "Space" || Name == "Tab" || Name == "Reset")
			{
				Take(Now);
			}
			else if (Name == "Backspace" || Name == "Delete" || Name == "PutBack")
			{
				StepBack(Now);
			}
			return true;
		}
		const int N = Count();
		if (GridKey(Name, N, Cols, Now))
		{
			return true;
		}
		if ((Name == "Enter" || Name == "Space") && Sel < N)
		{
			Activate(Sel, Now);
			return true;
		}
		if ((Name == "Alt" || Name == "F" || Name == "RT") && Tab == TicketsTab && Sel < N)
		{
			Renew(Now);
			return true;
		}
		return false;
	}

	std::vector<Hint> Hints() const override
	{
		if (Asking)
		{
			return {{"LEFT", "DPAD", "SELL OR KEEP"}, {"ENTER", "A", "DECIDE"}};
		}
		if (!Appraising.empty())
		{
			return {{"LEFT", "DPAD", "SELL OR PAWN"}, {"ENTER", "A", "TAKE IT"}};
		}
		if (Tab == SellTab)
		{
			return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "ON THE GLASS"}};
		}
		if (Tab == TicketsTab)
		{
			// No tickets, nothing to redeem: only the way out. RENEW only while it would buy something (shop::Renewable).
			const std::vector<shop::Ticket>& Tk = S.Life.Pawn.Tickets;
			if (Tk.empty())
			{
				return {};
			}
			if (Sel < static_cast<int>(Tk.size()) && !shop::Renewable(Tk[static_cast<size_t>(Sel)], S.WorldMinutes()))
			{
				return {{"UP", "DPAD", "CHOOSE"}, {"TAB", "Y", "REDEEM"}};
			}
			return {{"UP", "DPAD", "CHOOSE"}, {"TAB", "Y", "REDEEM"}, {"F", "RT", "RENEW"}};
		}
		return {{"UP", "DPAD", "CHOOSE"}, {"ENTER", "A", "PICK"}, {"TAB", "Y", "BUY"}};
	}

	void Body(Canvas& C, const Rect& R, double Now) override
	{
		const double World = S.WorldMinutes();
		Cols = R.W >= 1000.0f ? 3 : 2;
		std::string Top;
		switch (Tab)
		{
		case SellTab: Top = "What you could set on the glass. He pays half what it'd sell for, or lends four-tenths against it for thirty days."; break;
		case TicketsTab:
			Top = "Your tickets. Inside thirty days the loan plus twenty percent redeems one, or the twenty alone renews it thirty more. In the week of grace after, it's twenty-five percent either way. Then it's his.";
			break;
		case GearTab: Top = "Used electronics, restocked Mondays. ALL SALES FINAL."; break;
		case CuriosTab: Top = "The curio case. Everything in it works, mostly."; break;
		default: Top = "Yours, once: what you sold him or let go, and what turned up from Ninth St. Storage."; break;
		}
		// The house's terms, wrapped where the column's narrow (4:3); the grid starts under them.
		const float Under = Paragraph(C, Top, R.X, R.Y + 10.0f, R.W, 14.0f, 500, MenuMuted, 19.0f, 2) - R.Y + 24.0f;
		const Rect G{R.X, R.Y + Under, R.W, R.H - Under};
		if (Tab == TicketsTab)
		{
			const std::vector<shop::Ticket>& Tk = S.Life.Pawn.Tickets;
			if (Tk.empty())
			{
				C.Text("No open tickets. Nothing of yours in the cage.", R.X, R.Y + 80.0f, Ts(18.0f, 500, MenuDim));
				return;
			}
			Grid(
				C, G, static_cast<int>(Tk.size()), Cols, 170.0f, [&](int I, const Rect& Cell, bool Focus, bool Over) { TicketCard(C, Tk[static_cast<size_t>(I)], Cell, Focus, Over, Now); },
				[&](int I) { Activate(I, Now); }, Now);
			return;
		}
		const std::vector<shop::Lot> L = Lots();
		if (L.empty())
		{
			C.Text(Tab == SellTab ? "Nothing he'd take. Not on you, not in storage." : "Nothing on this shelf today.", R.X, R.Y + 80.0f, Ts(18.0f, 500, MenuDim));
			return;
		}
		Grid(
			C, G, static_cast<int>(L.size()), Cols, 214.0f, [&](int I, const Rect& Cell, bool Focus, bool Over) { LotCard(C, L[static_cast<size_t>(I)], Cell, Focus, Over, Now); },
			[&](int I) { Activate(I, Now); }, Now);
		(void)World;
	}

	void Tick(double Now) override
	{
		LastNow = Now;
		Restock();
		// After the loupe's beat, his two offers.
		if (!Appraising.empty() && Appraised(Now) && !Offered)
		{
			Offer(Now);
		}
		if (Appraising.empty())
		{
			Offered = false;
		}
	}

	Receipt Bill(double Now) override
	{
		const double World = S.WorldMinutes();
		Receipt R;
		R.Paper = PaperKind::Thermal;
		R.Title = "NORTHSIDE PAWN & LOAN";
		R.Head = {"232 MARKET ST. \xC2\xB7 555-0187", Upper(Stamped(World))};
		// A ticket just written or the one at the cage window: the pink carbon.
		const shop::Ticket* Tk = nullptr;
		const bool JustRedeemed = Tab == TicketsTab && !RedeemedLot.empty() && StampWas == StampKind::Redeemed && Now - StampAt < 4.0;
		if (Tab == TicketsTab && !JustRedeemed && Sel < static_cast<int>(S.Life.Pawn.Tickets.size()))
		{
			Tk = &S.Life.Pawn.Tickets[static_cast<size_t>(Sel)];
		}
		if (Tab == TicketsTab && !Tk)
		{
			// Nothing at the window, or the ticket just handed back: the redemption's receipt.
			R.Empty = "No open tickets.";
			if (!RedeemedLot.empty())
			{
				R.Title = "REDEMPTION";
				R.Number = TicketNo(RedeemedNo);
				R.Lines.push_back({RedeemedLot, Upper(shopart::LotName(S, RedeemedLot)), "", ""});
				R.Total = {"total", "PAID", "", Money(RedeemedCents)};
			}
			R.Foot = {"TICKET RETURNED \xC2\xB7 THANK YOU", "CARD BALANCE " + Money(S.BankrollCents)};
			return R;
		}
		if (Tab == SellTab && Written > 0)
		{
			for (const shop::Ticket& T : S.Life.Pawn.Tickets)
			{
				Tk = T.Number == Written ? &T : Tk;
			}
		}
		if (Tk)
		{
			R.Paper = PaperKind::Ticket;
			R.Title = "PAWN TICKET";
			R.Number = TicketNo(Tk->Number);
			R.Head = {"NORTHSIDE PAWN & LOAN \xC2\xB7 232 MARKET ST.", "TAKEN " + Upper(net::DateLabel(net::DayOf(Tk->TakenAt))) + " \xC2\xB7 REDEEM BY " + Upper(net::DateLabel(net::DayOf(Tk->DueAt)))};
			R.Lines.push_back({Tk->Lot, Upper(shopart::LotName(S, Tk->Lot)), "", ""});
			R.Lines.push_back({"", "LOAN", "", Money(Tk->LoanCents)});
			R.Lines.push_back({"", "TO REDEEM BY " + Upper(net::DateLabel(net::DayOf(Tk->DueAt))), "", Money(Tk->LoanCents + shop::RenewCents(*Tk))});
			const Chips Today = shop::RedeemCents(*Tk, World);
			const bool Grace = World > Tk->DueAt;
			R.Sums.push_back({"", Grace ? "IN GRACE: +25%" : "IN TERM: +20%", "", Today > 0 ? Money(Today - Tk->LoanCents) : std::string("--")});
			R.Total = {"total", Today > 0 ? "TODAY" : "FORFEITED", "", Today > 0 ? Money(Today) : std::string("--")};
			R.Total.Red = Grace || Today <= 0;
			R.Foot = {"7-DAY GRACE AFTER " + Upper(net::DateLabel(net::DayOf(Tk->DueAt))) + ", THEN IT'S OURS", "TICKET REQUIRED TO REDEEM \xC2\xB7 NO ID NO LOAN"};
			return R;
		}
		if (Tab == SellTab)
		{
			R.Empty = "Set it on the glass.";
			const shop::Lot* L = Find(Appraising.empty() ? SoldId : Appraising);
			if (!SoldId.empty() && Appraising.empty())
			{
				R.Lines.push_back({SoldId, Upper(SoldName), "", ""});
				R.Total = {"total", "PAID TO YOU", "", Money(SoldCents)};
				R.Foot = {"BILL OF SALE \xC2\xB7 ALL SALES FINAL", "CARD BALANCE " + Money(S.BankrollCents)};
				return R;
			}
			if (L)
			{
				R.Lines.push_back({L->Id, Upper(L->Name), "", ""});
				if (L->Stored)
				{
					R.Lines.push_back({"", "IN STORAGE \xC2\xB7 NINTH ST.", "", "+1 HR"});
				}
				if (!Appraised(Now))
				{
					R.Lines.push_back({"", "UNDER THE LOUPE\xE2\x80\xA6", "", ""});
				}
				else
				{
					shop::Ticket Hypo;
					Hypo.LoanCents = L->LoanCents;
					Hypo.TakenAt = World;
					Hypo.DueAt = World + shop::PawnTermDays * 1440.0;
					Line Sell{"sell", "SELL OUTRIGHT", "", Money(L->SellCents)};
					Line Pawn{"pawn", "PAWN \xC2\xB7 30 DAYS", "", Money(L->LoanCents)};
					Sell.Bold = !TakeLoan;
					Pawn.Bold = TakeLoan;
					R.Lines.push_back(Sell);
					R.Lines.push_back(Pawn);
					R.Sums.push_back({"", "REDEEMS FOR", "", Money(shop::RedeemCents(Hypo, World))});
					if (Asking)
					{
						Line Warn{"", "THIS ONE DOESN'T COME BACK.", "", ""};
						Warn.Red = true;
						R.Sums.push_back(Warn);
					}
				}
			}
			R.Foot = {"NO ID NO LOAN \xC2\xB7 30-DAY LOANS", "CARD BALANCE " + Money(S.BankrollCents)};
			return R;
		}
		// Buying: the lot on the receipt, taxed.
		R.Empty = "Pick something off the shelf.";
		const std::string Id = BoughtId.empty() ? Picked : BoughtId;
		const std::vector<shop::Lot> L = Lots();
		const shop::Lot* Lt = nullptr;
		for (const shop::Lot& X : L)
		{
			Lt = X.Id == Id ? &X : Lt;
		}
		const Chips Price_ = Lt ? Lt->ValueCents : BoughtCents;
		const std::string Name = Lt ? Lt->Name : BoughtName;
		if (!Id.empty() && !Name.empty())
		{
			Line Ln{Id, Upper(Name), "", Money(Price_)};
			Ln.Removable = BoughtId.empty();
			R.Lines.push_back(Ln);
			if (IsAuction(Id))
			{
				R.Lines.push_back({"", "LOT 114 \xC2\xB7 NINTH ST.", "", ""});
			}
			R.Sums.push_back({"subtotal", "SUBTOTAL", "", Money(Price_)});
			R.Sums.push_back({"tax", "TAX 7.25%", "", Money(store::Tax(Price_))});
			R.Total = {"total", "TOTAL", "", Money(Price_ + store::Tax(Price_))};
		}
		R.Foot = {"ALL SALES FINAL", "CARD BALANCE " + Money(S.BankrollCents)};
		return R;
	}

	void RemoveLine(const std::string& Id, double Now) override
	{
		if (Id == Picked)
		{
			Picked.clear();
			Touch(Id, Now);
		}
	}

	void Side(Canvas& C, const Rect& R, double Now) override
	{
		float Y = DrawBill(C, Bill(Now), R.X, R.Y, Now);
		if (!Appraising.empty() && Appraised(Now))
		{
			// His two offers, or the second ask, as two buttons; the arrows pick, Enter takes.
			const shop::Lot* L = Find(Appraising);
			if (L)
			{
				const bool Ask = Asking;
				const std::string A = Ask ? "SELL IT" : "SELL " + Price(L->SellCents);
				const std::string B = Ask ? "KEEP IT" : "PAWN " + Price(L->LoanCents);
				const bool First = Ask ? AskSell : !TakeLoan;
				const float Bw = (ReceiptW - 12.0f) * 0.5f;
				const Rect Ra{R.X, Y + 30.0f, Bw, 66.0f};
				const Rect Rb{R.X + Bw + 12.0f, Y + 30.0f, Bw, 66.0f};
				for (int K = 0; K < 2; ++K)
				{
					const Rect& Rr = K == 0 ? Ra : Rb;
					const bool On = (K == 0) == First;
					const bool Over = Hover(Rr);
					C.FillRect(Rr, Paint(On ? Mix(Mint, Hex(0xffffff), Over ? 0.15f : 0.0f) : Fade(MenuInk, Over ? 0.12f : 0.06f)));
					if (On)
					{
						C.GlowRoundRect(Rr, 1.0f, Fade(Mint, 0.3f), 18.0f);
					}
					TrackedText(C, K == 0 ? A : B, Rr.X + Rr.W * 0.5f, Rr.Y + 41.0f, 17.0f, 900, On ? Hex(0x05140c) : MenuInk, 2.4f, Align::Center);
					if (Released(Rr))
					{
						if (Ask)
						{
							AskSell = K == 0;
							Answer(Now);
						}
						else
						{
							TakeLoan = K == 1;
							Take(Now);
						}
						return;
					}
				}
				return;
			}
		}
		const float By = DrawButton(C, Main(Now), R.X, Y + 30.0f, Now);
		if (Tab == TicketsTab && IsOpen())
		{
			RenewButton(C, R.X, By + 12.0f, Now);
		}
	}

	Button Main(double Now) override
	{
		Button B;
		const double World = S.WorldMinutes();
		if (Tab == SellTab)
		{
			const std::vector<shop::Lot> L = Lots();
			B.Label = !Appraising.empty() ? "UNDER THE LOUPE" : "SET IT ON THE GLASS";
			B.Enabled = Appraising.empty() && Sel < static_cast<int>(L.size()) && L[static_cast<size_t>(Sel)].Refused.empty();
			(void)Now;
			return B;
		}
		if (Tab == TicketsTab)
		{
			const std::vector<shop::Ticket>& Tk = S.Life.Pawn.Tickets;
			if (Sel >= static_cast<int>(Tk.size()))
			{
				B.Label = "REDEEM";
				return B;
			}
			const Chips Due = shop::RedeemCents(Tk[static_cast<size_t>(Sel)], World);
			if (Due <= 0)
			{
				B.Label = "FORFEITED";
				return B;
			}
			B.Enabled = true;
			B.Warn = Due > S.BankrollCents;
			B.Label = B.Warn ? "CARD WON'T COVER IT" : "REDEEM " + Money(Due);
			return B;
		}
		if (Picked.empty())
		{
			B.Label = "BUY";
			return B;
		}
		const std::string Why = BuyBlocked(Picked);
		if (!Why.empty())
		{
			B.Label = Why;
			return B;
		}
		const shop::Lot* L = FindShelf(Picked);
		const Chips Due = L ? L->ValueCents + store::Tax(L->ValueCents) : 0;
		B.Enabled = L != nullptr;
		B.Warn = Due > S.BankrollCents;
		B.Label = B.Warn ? "CARD WON'T COVER IT" : "BUY " + Money(Due);
		return B;
	}

	void DoMain(double Now) override
	{
		if (Tab == SellTab)
		{
			if (!Appraising.empty())
			{
				if (Appraised(Now))
				{
					Take(Now);
				}
				return;
			}
			if (Sel < Count())
			{
				Activate(Sel, Now);
			}
			return;
		}
		if (Tab == TicketsTab)
		{
			Redeem(Now);
			return;
		}
		Buy(Now);
	}

private:
	void Reset()
	{
		Appraising.clear();
		Asking = false;
		AskSell = false;
		TakeLoan = false;
		Picked.clear();
		BoughtId.clear();
		SoldId.clear();
		Written = 0;
		StampWas = StampKind::None;
	}

	void Shot()
	{
		if (Tab == SellTab)
		{
			Camera("pawn-counter");
		}
		else if (Tab == TicketsTab)
		{
			Camera("pawn-cage");
		}
		else if (Tab == CuriosTab)
		{
			Camera("pawn-case");
		}
	}

	bool IsAuction(const std::string& Id) const
	{
		for (const shop::ShelfLot& L : S.Life.Pawn.Shelf)
		{
			if (L.Lot == Id && L.Auction)
			{
				return true;
			}
		}
		return false;
	}

	bool IsTheirs(const std::string& Id) const
	{
		for (const shop::ShelfLot& L : S.Life.Pawn.Shelf)
		{
			if (L.Lot == Id)
			{
				return true;
			}
		}
		return false;
	}

	bool HasAuctionLot() const
	{
		for (const shop::ShelfLot& L : S.Life.Pawn.Shelf)
		{
			const double World = S.WorldMinutes();
			if (L.Auction && World >= L.From && World < L.Until)
			{
				return true;
			}
		}
		return false;
	}

	/** The lots on this tab: what the hero could sell, or the shelf's, sorted by tab. */
	std::vector<shop::Lot> Lots() const
	{
		if (Tab == SellTab)
		{
			return LotsNow;
		}
		std::vector<shop::Lot> Out;
		for (const shop::Lot& L : ShelfNow)
		{
			const bool Theirs = IsTheirs(L.Id);
			if ((Tab == YoursTab && Theirs) || (Tab == CuriosTab && !Theirs && L.Kind == shop::LotKind::Curio) || (Tab == GearTab && !Theirs && L.Kind != shop::LotKind::Curio))
			{
				Out.push_back(L);
			}
		}
		return Out;
	}

	/** What the hero could bring Augie, and his shelf, read once a frame (and again after a sale, a loan or a buy). */
	void Restock()
	{
		LotsNow = S.PawnLots();
		ShelfNow = S.PawnShelf();
	}

	int Count() const { return Tab == TicketsTab ? static_cast<int>(S.Life.Pawn.Tickets.size()) : static_cast<int>(Lots().size()); }

	const shop::Lot* Find(const std::string& Id) const
	{
		for (const shop::Lot& L : LotsNow)
		{
			if (L.Id == Id)
			{
				return &L;
			}
		}
		return nullptr;
	}

	const shop::Lot* FindShelf(const std::string& Id) const
	{
		for (const shop::Lot& L : ShelfNow)
		{
			if (L.Id == Id)
			{
				return &L;
			}
		}
		return nullptr;
	}

	bool Appraised(double Now) const { return !Appraising.empty() && Now - AppraiseAt >= AppraiseSeconds; }

	/** Why a shelf lot can't be bought ("" when it can): NOT IN STOCK (the 3D can't stand it up), or the shelf's own reason
	 *  money aside (Session::PawnShelf: OWNED, NEEDS A ROOM, YOU HAVE BETTER). */
	std::string BuyBlocked(const std::string& Id) const
	{
		if (!shopart::LotShowable(Id, Caps.GearKinds, Caps.GarmentKinds, Caps.Curios))
		{
			return "NOT IN STOCK";
		}
		for (const shop::Lot& L : ShelfNow)
		{
			if (L.Id == Id && !L.Refused.empty())
			{
				std::string Why = Upper(L.Refused);
				while (!Why.empty() && Why.back() == '.')
				{
					Why.pop_back();
				}
				return Why;
			}
		}
		return std::string();
	}

	void Activate(int I, double Now)
	{
		LastNow = Now;
		if (Tab == TicketsTab)
		{
			Sel = I;
			StampWas = StampKind::None;
			return;
		}
		const std::vector<shop::Lot> L = Lots();
		if (I < 0 || I >= static_cast<int>(L.size()))
		{
			return;
		}
		const shop::Lot& Lt = L[static_cast<size_t>(I)];
		if (Tab == SellTab)
		{
			if (!Lt.Refused.empty())
			{
				SayText(Lt.Refused, Now, true);
				return;
			}
			// On the glass: he turns it under the loupe, then makes his two offers.
			Appraising = Lt.Id;
			AppraiseAt = Now;
			TakeLoan = false;
			SoldId.clear();
			Written = 0;
			StampWas = StampKind::None;
			Camera("pawn-counter");
			const char* Kind = Lt.Kind == shop::LotKind::Award ? "appraise:award" : Lt.Kind == shop::LotKind::Memento ? "appraise:cup" : Lt.Kind == shop::LotKind::Keepsake ? "appraise:keepsake" : Lt.Kind == shop::LotKind::Garment ? "appraise:garment" : "appraise:gear";
			Say(Kind, Now);
			return;
		}
		// The shelf: one lot on the receipt.
		const std::string Why = BuyBlocked(Lt.Id);
		if (Picked == Lt.Id)
		{
			Picked.clear();
			Touch(Lt.Id, Now);
			return;
		}
		BoughtId.clear();
		StampWas = StampKind::None;
		Picked = Lt.Id;
		Touch(Lt.Id, Now);
		if (Why == "NEEDS A ROOM")
		{
			SayText("Where you gonna plug it in, the couch?", Now);
		}
		else if (IsAuction(Lt.Id))
		{
			Say("appraise:lot114", Now);
		}
	}

	/** After the loupe: his offers, in his words. */
	void Offer(double Now)
	{
		const shop::Lot* L = Find(Appraising);
		if (!L)
		{
			return;
		}
		const std::string Said = KeeperSays("offer:" + std::to_string(L->SellCents) + ":" + std::to_string(L->LoanCents));
		SayText(!Said.empty() && Said.find("to own") != std::string::npos ? Said
																		  : Capital(town::SpokenDollars(L->SellCents / 100)) + " to own it. " + Capital(town::SpokenDollars(L->LoanCents / 100)) +
																			  " to hold it, thirty days. The ticket's your receipt. Don't lose it.",
			Now);
		Offered = true;
	}

	void Take(double Now)
	{
		const shop::Lot* L = Find(Appraising);
		if (!L)
		{
			Appraising.clear();
			return;
		}
		if (!TakeLoan && L->Final && !Asking)
		{
			// What can't be bought again anywhere: once more, on the receipt.
			Asking = true;
			AskSell = false;
			return;
		}
		const shop::Lot Lot = *L;
		if (TakeLoan)
		{
			const int Before = S.Life.Pawn.NextTicket;
			const std::string Why = S.PawnLoan(Lot.Id);
			if (!Why.empty())
			{
				SayText(Why, Now, true);
				return;
			}
			Written = 0;
			for (const shop::Ticket& T : S.Life.Pawn.Tickets)
			{
				Written = T.Lot == Lot.Id && T.Number >= Before ? T.Number : Written;
			}
			if (Written == 0 && !S.Life.Pawn.Tickets.empty())
			{
				Written = S.Life.Pawn.Tickets.back().Number;
			}
			Appraising.clear();
			Restock();
			Stamp(StampKind::Punched, Now);
			Say("loan", Now);
			Started(Now);
			return;
		}
		const std::string Why = S.PawnSell(Lot.Id);
		Asking = false;
		if (!Why.empty())
		{
			SayText(Why, Now, true);
			return;
		}
		SoldId = Lot.Id;
		SoldName = Lot.Name;
		SoldCents = Lot.SellCents;
		Appraising.clear();
		Restock();
		Stamp(StampKind::PaidOut, Now);
		Say("sell", Now);
		Started(Now);
	}

	void Answer(double Now)
	{
		if (AskSell)
		{
			Take(Now);
			return;
		}
		Asking = false;
		Appraising.clear();
		SayText("Smart. Some things you keep.", Now);
	}

	void Redeem(double Now)
	{
		const std::vector<shop::Ticket>& Tk = S.Life.Pawn.Tickets;
		if (Sel >= static_cast<int>(Tk.size()))
		{
			return;
		}
		const shop::Ticket T = Tk[static_cast<size_t>(Sel)];
		const Chips Due = shop::RedeemCents(T, S.WorldMinutes());
		if (Due <= 0)
		{
			Say("forfeit", Now, true);
			return;
		}
		const std::string Why = S.PawnRedeem(T.Number);
		if (!Why.empty())
		{
			if (Due > S.BankrollCents)
			{
				Stamp(StampKind::Declined, Now);
				Say("declined", Now, true);
			}
			else
			{
				SayText(Why, Now, true);
			}
			return;
		}
		Restock();
		RedeemedLot = T.Lot;
		RedeemedNo = T.Number;
		RedeemedCents = Due;
		Stamp(StampKind::Redeemed, Now);
		Say("redeem", Now);
		if (shopart::LotKindOf(T.Lot) == shop::LotKind::Garment)
		{
			Redress();
		}
		Sel = std::max(0, std::min(Sel, static_cast<int>(S.Life.Pawn.Tickets.size()) - 1));
	}

	void Renew(double Now)
	{
		const std::vector<shop::Ticket>& Tk = S.Life.Pawn.Tickets;
		if (Sel >= static_cast<int>(Tk.size()))
		{
			return;
		}
		const shop::Ticket T = Tk[static_cast<size_t>(Sel)];
		const double World = S.WorldMinutes();
		const Chips Due = shop::RenewCents(T, World); // the 25% in the grace, as the Session charges it
		const bool Buys = shop::Renewable(T, World);
		const std::string Why = S.PawnRenew(T.Number);
		if (!Why.empty())
		{
			if (Buys && Due > S.BankrollCents)
			{
				Stamp(StampKind::Declined, Now);
				Say("declined", Now, true);
			}
			else
			{
				SayText(Why, Now, true);
			}
			return;
		}
		Stamp(StampKind::Paid, Now);
		Say("renew", Now);
	}

	/**
	 * RENEW for the mouse (the keys' F and the pad's RT): under REDEEM while a ticket in term or grace is at the window, at
	 * what the ticket owes today (the 20%, or the 25% in the grace). Greyed, with the date it's good till, while a renewal
	 * would buy nothing (shop::Renewable: written or renewed today).
	 */
	void RenewButton(Canvas& C, float X, float Y, double Now)
	{
		const std::vector<shop::Ticket>& Tk = S.Life.Pawn.Tickets;
		const bool JustRedeemed = !RedeemedLot.empty() && StampWas == StampKind::Redeemed && Now - StampAt < 4.0;
		const double World = S.WorldMinutes();
		if (JustRedeemed || Sel < 0 || Sel >= static_cast<int>(Tk.size()) || shop::RedeemCents(Tk[static_cast<size_t>(Sel)], World) <= 0)
		{
			return;
		}
		const shop::Ticket& T = Tk[static_cast<size_t>(Sel)];
		const Rect Rr{X, Y, ReceiptW, 46.0f};
		if (!shop::Renewable(T, World))
		{
			C.FillRect(Rr, Paint(Fade(MenuInk, 0.04f)));
			TrackedText(C, "RENEW \xC2\xB7 GOOD TILL " + Upper(net::DateLabel(net::DayOf(T.DueAt))), Rr.X + Rr.W * 0.5f, Rr.Y + 29.0f, 14.0f, 800, MenuMuted, 2.4f, Align::Center);
			return;
		}
		const Chips Fee = shop::RenewCents(T, World);
		const bool Over = Hover(Rr);
		C.FillRect(Rr, Paint(Fade(Fee > S.BankrollCents ? MenuWarn : MenuInk, Over ? 0.16f : 0.07f)));
		TrackedText(C, "RENEW 30 DAYS \xC2\xB7 " + Money(Fee), Rr.X + (Rr.W - 70.0f) * 0.5f, Rr.Y + 29.0f, 14.0f, 800, MenuInk, 2.4f, Align::Center);
		Glyph(C, Gamepad ? "RT" : "F", Rr.X + Rr.W - 60.0f, Rr.Y + 31.0f, 0.8f);
		if (Released(Rr))
		{
			Renew(Now);
		}
	}

	void Buy(double Now)
	{
		if (Picked.empty())
		{
			Say("browse", Now);
			return;
		}
		const std::string Blocked = BuyBlocked(Picked);
		if (!Blocked.empty())
		{
			// His reason, in his words: the shelf's own (the upgrade path, a better one at home) rather than "not for sale"
			// for something that is.
			const shop::Lot* Held = FindShelf(Picked);
			const std::string Said = Blocked == "NEEDS A ROOM"	   ? std::string("Where you gonna plug it in, the couch?")
				: Blocked == "OWNED"						   ? std::string("You've got one. Nobody needs two.")
				: Blocked == "YOU HAVE BETTER"				   ? std::string("You've got better than that at home. Keep your money.")
				: Blocked == "NOT IN STOCK" || !Held || Held->Refused.empty() ? std::string("That one's not for sale.")
															   : Held->Refused;
			SayText(Said, Now, true);
			return;
		}
		const shop::Lot* L = FindShelf(Picked);
		if (!L)
		{
			Picked.clear();
			return;
		}
		const shop::Lot Lot = *L;
		const Chips Due = Lot.ValueCents + store::Tax(Lot.ValueCents);
		const bool WasTheirs = IsTheirs(Lot.Id); // read before the sale takes it off his books: the hero's own coming home
		const std::string Why = S.PawnBuy(Lot.Id);
		if (!Why.empty())
		{
			if (Due > S.BankrollCents)
			{
				Stamp(StampKind::Declined, Now);
				Say("declined", Now, true);
			}
			else
			{
				SayText(Why, Now, true);
			}
			return;
		}
		BoughtId = Lot.Id;
		BoughtName = Lot.Name;
		BoughtCents = Lot.ValueCents;
		Picked.clear();
		Restock();
		Stamp(StampKind::Paid, Now);
		Say(WasTheirs ? "redeem" : "pay", Now);
		Sel = 0;
	}

	void LotCard(Canvas& C, const shop::Lot& L, const Rect& R, bool Focus, bool Over, double Now)
	{
		LastNow = Now;
		const bool Selling = Tab == SellTab;
		const std::string Blocked = Selling ? (L.Refused.empty() ? std::string() : std::string("HE WON'T TAKE IT")) : BuyBlocked(L.Id);
		const bool OnGlass = Selling && Appraising == L.Id;
		const bool Chosen = !Selling && Picked == L.Id;
		CardBack(C, R, Focus || OnGlass, Over, !Blocked.empty() && !Chosen, Now);
		const Rect Pic{R.X + 12.0f, R.Y + 12.0f, R.W - 24.0f, R.H - 92.0f};
		const float A0 = C.GetAlpha();
		if (!Blocked.empty())
		{
			C.SetAlpha(A0 * 0.5f);
		}
		shopart::LotPicture(C, S, L, Pic, Now);
		C.SetAlpha(A0);
		if (OnGlass && !Appraised(Now))
		{
			// The loupe going over it.
			const float T = static_cast<float>((Now - AppraiseAt) / AppraiseSeconds);
			const float Lx = Pic.X + Pic.W * (0.3f + 0.4f * std::sin(T * 6.0f) * 0.5f + 0.2f);
			const float Ly = Pic.Y + Pic.H * (0.45f + 0.15f * std::cos(T * 5.0f));
			C.FillCircle(Lx, Ly, 24.0f, Paint(Rgba(220, 235, 255, 0.18f)));
			C.StrokeEllipse(Lx, Ly, 24.0f, 24.0f, Hex(0x1c1d20), 5.0f);
			C.StrokePolyline({{Lx + 17.0f, Ly + 17.0f}, {Lx + 34.0f, Ly + 34.0f}}, false, Hex(0x1c1d20), 6.0f, true);
		}
		const float Ty = R.Y + R.H - 54.0f;
		const std::string Pr = Selling ? (OnGlass && Appraised(Now) ? Price(L.SellCents) + " / " + Price(L.LoanCents) : std::string()) : Price(L.ValueCents);
		const float Pw = Pr.empty() ? 0.0f : C.Measure(Pr, 18.0f, 900);
		C.Text(L.Name, R.X + 14.0f, Ty, Ts(17.0f, 800, Blocked.empty() ? MenuInk : MenuMuted, Align::Left, Baseline::Alphabetic, false, R.W - 40.0f - Pw));
		if (!Pr.empty())
		{
			C.Text(Pr, R.X + R.W - 14.0f, Ty, Ts(18.0f, 900, Accent("pawn"), Align::Right));
		}
		std::string Sub = KindWord(L.Kind);
		if (L.Stored)
		{
			Sub = "In storage \xC2\xB7 Ninth St.";
		}
		else if (!Selling && IsAuction(L.Id))
		{
			Sub = "LOT 114 \xC2\xB7 NINTH ST.";
		}
		else if (!Selling && Tab == YoursTab)
		{
			Sub = "Was yours";
		}
		C.Text(Sub, R.X + 14.0f, Ty + 20.0f, Ts(12.0f, 600, L.Stored || IsAuction(L.Id) ? MenuGold : MenuMuted, Align::Left, Baseline::Alphabetic, false, R.W - 28.0f));
		float Bx = R.X + 12.0f;
		if (Chosen)
		{
			Bx += Badge(C, "ON THE RECEIPT", Bx, R.Y + 12.0f, Mint, true) + 6.0f;
		}
		if (OnGlass)
		{
			Bx += Badge(C, Appraised(Now) ? "HIS OFFER" : "ON THE GLASS", Bx, R.Y + 12.0f, Accent("pawn"), true) + 6.0f;
		}
		if (L.Final && Selling)
		{
			Bx += Badge(C, "ONE OF A KIND", Bx, R.Y + 12.0f, MenuGold) + 6.0f;
		}
		if (!Blocked.empty())
		{
			Badge(C, Blocked, R.X + 12.0f, Pic.Y + Pic.H - 24.0f, Blocked == "OWNED" ? MenuInk : MenuWarn);
		}
	}

	void TicketCard(Canvas& C, const shop::Ticket& T, const Rect& R, bool Focus, bool Over, double Now)
	{
		const double World = S.WorldMinutes();
		const Chips Today = shop::RedeemCents(T, World);
		const bool Grace = World > T.DueAt && Today > 0;
		const bool Gone = Today <= 0;
		CardBack(C, R, Focus, Over, Gone, Now);
		// The ticket's stub: pink, the number, the punch.
		const Rect Stub{R.X + 12.0f, R.Y + 12.0f, 96.0f, R.H - 24.0f};
		C.FillRoundRect(Stub, 4.0f, Paint::Linear({Stub.X, Stub.Y}, {Stub.X, Stub.Y + Stub.H}, Hex(0xf5c8d3), Hex(0xe9b3c1)));
		C.Text(TicketNo(T.Number), Stub.X + Stub.W * 0.5f, Stub.Y + 34.0f, Ts(18.0f, 800, Hex(0xb01e3c), Align::Center, Baseline::Alphabetic, true));
		C.FillCircle(Stub.X + Stub.W * 0.5f, Stub.Y + Stub.H - 26.0f, 9.0f, Paint(Hex(0x0b0e14)));
		const float X = Stub.X + Stub.W + 18.0f;
		const float W = R.X + R.W - 16.0f - X;
		C.Text(shopart::LotName(S, T.Lot), X, R.Y + 40.0f, Ts(18.0f, 800, MenuInk, Align::Left, Baseline::Alphabetic, false, W));
		C.Text("Loan " + Money(T.LoanCents) + " \xC2\xB7 taken " + net::DateLabel(net::DayOf(T.TakenAt)), X, R.Y + 64.0f, Ts(13.0f, 600, MenuMuted, Align::Left, Baseline::Alphabetic, false, W));
		const std::string State = Gone ? "FORFEITED" : Grace ? "IN GRACE UNTIL " + Upper(net::WeekdayName(net::DayOf(T.DueAt + shop::PawnGraceDays * 1440.0))) : "DUE " + Upper(net::WeekdayName(net::DayOf(T.DueAt))) + " " + Upper(net::DateLabel(net::DayOf(T.DueAt)));
		Badge(C, State, X, R.Y + 82.0f, Gone || Grace ? MenuWarn : Mint, Grace);
		if (!Gone)
		{
			C.Text(Money(Today), R.X + R.W - 16.0f, R.Y + R.H - 22.0f, Ts(22.0f, 900, Grace ? MenuWarn : Accent("pawn"), Align::Right));
			C.Text("redeems it today", X, R.Y + R.H - 24.0f, Ts(13.0f, 500, MenuMuted));
		}
	}

	std::string Appraising; // the lot on the glass
	double AppraiseAt = -10.0;
	bool Offered = false;
	bool TakeLoan = false; // PAWN chosen over SELL
	bool Asking = false;   // the second ask on a Final lot
	bool AskSell = false;
	int Written = 0;       // the ticket just written
	std::string SoldId;
	std::string SoldName;
	Chips SoldCents = 0;
	std::string Picked; // the shelf lot on the receipt
	std::string BoughtId;
	std::string BoughtName;
	Chips BoughtCents = 0;
	std::vector<shop::Lot> LotsNow;
	std::vector<shop::Lot> ShelfNow;
	std::string RedeemedLot; // the ticket last handed back (its receipt)
	int RedeemedNo = 0;
	Chips RedeemedCents = 0;
	int Cols = 3;
	double LastNow = 0.0;
};
} // namespace

std::unique_ptr<ShopScreen> MakePawn(Session& S)
{
	return std::make_unique<PawnPage>(S);
}
} // namespace shopframe
} // namespace ui
} // namespace ss
