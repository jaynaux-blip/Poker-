#pragma once
// The shared frame of Northside's shop screens (the realism update's spec, §7.4): the header (the venue's mark, name,
// address, hours and OPEN or CLOSED), the keeper's speech bubble (town::KeeperLine), the Q/E tabs, cards in a grid
// with their badges, the receipt (thermal paper, the barber's slip, Ruby's guest check, the pawn's pink ticket, a
// donation receipt) with its PAID, DECLINED and punch stamps, the tip row, the main button and the key-glyph footer,
// and OpenGrace. Each page (BarberScreen.cpp, ThriftScreen.cpp, GymScreen.cpp, DinerScreen.cpp, PawnScreen.cpp,
// WardrobeScreen.cpp) is a shopframe::Page that fills in its tabs, its body, its bill and its main action.

#include "FrontEndShared.h"

#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/UI/ShopArt.h"
#include "ShortStack/UI/ShopScreen.h"
#include "ShortStack/UI/StoreCounter.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace shopframe
{
// ------------------------------------------------------------------ the look
constexpr float Margin = 96.0f;
constexpr float ReceiptW = 440.0f;
constexpr float TabY = 226.0f;
constexpr float BodyTop = 262.0f;
constexpr float FooterY = 1080.0f - 62.0f;
/** How long after opening a screen ignores everything but Escape (seconds), as StoreCounter::OpenGrace. */
constexpr double OpenGrace = 0.3;

inline const Color Paper = Hex(0xf3efe6);
inline const Color PaperInk = Hex(0x2b2723);
inline const Color Mint = Hex(0x3ecf8e);
inline const Color CardTop = Rgba(30, 33, 44, 0.96f);
inline const Color CardLow = Rgba(12, 14, 20, 0.95f);

inline float Cf(int V)
{
	return static_cast<float>(V);
}

inline Color Fade(Color C, float A)
{
	C.A *= A;
	return C;
}

inline Color Shade(const Color& C, float K)
{
	return K >= 0.0f ? Mix(C, Hex(0xffffff), K) : Mix(C, Hex(0x000000), -K);
}

/** "$12.00": dollars and cents, as the receipts print them. */
std::string Money2(Chips Cents);
/** "$12" for whole dollars, "$4.99" otherwise: the cards' and the boards' prices. */
std::string Price(Chips Cents);
std::string Upper(std::string S);
/** "45 MIN", "1 HR", "1 HR 30 MIN". */
std::string MinutesText(int Minutes);
/** "Tue Oct 6  7:42 PM". */
std::string Stamped(double World);
/** A stable hash of a string (FNV-1a), for picks that vary by day and visit but not by frame. */
uint64_t Hash(const std::string& S);

/** The venue's accent (its table color, or the screen's own when the table has none). */
Color Accent(const std::string& Venue);
/** The venue's mark beside its name: the barber's pole, a hanger, a dumbbell, a coffee cup, three gold balls. */
void VenueMark(Canvas& C, const std::string& Venue, float X, float Y, float R, double Time);

/** Hunger (a fork), thirst (a drop) and energy (a bolt), as the counter draws them. */
void NeedIcon(Canvas& C, int Which, float X, float Y, float S, const Color& Col);
/** Pills for what something does to the needs (relief: positive eases a need); returns the width used. */
float NeedPills(Canvas& C, double Hunger, double Thirst, double Energy, float X, float Y, float MaxW);

/** A small rounded badge with Text (OWNED, WORN, PORTRAIT ONLY, NOT IN STOCK); returns its width. */
float Badge(Canvas& C, const std::string& Text, float X, float Y, const Color& Col, bool Solid = false);
float BadgeWidth(Canvas& C, const std::string& Text);
/** A reason as a badge ("Give it three weeks." reads GIVE IT THREE WEEKS), or Fallback when it won't fit MaxW. */
std::string BadgeText(Canvas& C, const std::string& Why, float MaxW, const char* Fallback);

// ------------------------------------------------------------------ the receipt
enum class PaperKind : int
{
	Thermal,    // the register's roll (the thrift, the gym's desk, the pawn's counter)
	Slip,       // the barber's ticket: a pole stripe and a number
	GuestCheck, // Ruby's green guest check, written in pen
	Ticket,     // the pawn's pink carbon ticket
	Donation,   // St. Agnes Outreach's letterhead
};

struct Line
{
	std::string Id;    // for the print-in and the put-back minus ("" for neither)
	std::string Left;
	std::string Mid;   // a column between (the barber's minutes)
	std::string Right;
	bool Bold = false;
	bool Dim = false;
	bool Red = false;
	bool Removable = false; // the mouse's minus beside it
};

struct Receipt
{
	PaperKind Paper = PaperKind::Thermal;
	std::string Title;
	std::vector<std::string> Head;
	std::vector<Line> Lines;
	std::string Empty; // printed when there are no lines
	std::vector<Line> Sums;
	Line Total;
	std::vector<std::string> Foot;
	std::string Number; // the slip's or the ticket's number, top right
};

enum class StampKind : int
{
	None,
	Paid,
	Declined,
	Punched,  // the pawn ticket: a punch where PAID would be
	Redeemed,
	Thanks,   // a donation
	PaidOut,  // the pawn paid the player
};

struct Button
{
	std::string Label;
	bool Enabled = false;
	bool Warn = false; // short of money: drawn red, still pressable (the DECLINED path)
};

/** A thing to choose from: what a card in the grid shows besides its picture. */
struct Hint
{
	std::string Glyph; // keyboard glyph ("ENTER")
	std::string Pad;   // the pad's ("A")
	std::string Label;
};

enum class Backdrop : int
{
	Full,  // the counter's: the room dimmed behind everything
	Split, // the left and right columns dimmed, the middle clear for the hero in the mirror
	Low,   // only a band along the bottom (the fitting booth: the three-way mirror is the show)
};

// ------------------------------------------------------------------ the page
class Page : public ShopScreen
{
public:
	Page(Session& InSession, std::string InVenue);
	void Open(double Now, const std::string& PageId) final;
	void Close(double Now) final;
	bool IsOpen() const final { return Shown; }
	bool TakeLeave() final;
	void Key(const std::string& Name, double Now) final;
	void Draw(Canvas& C, double Now) final;
	void SetCaps(const HostCaps& InCaps) final;
	std::vector<HostRequest> TakeRequests() final;

protected:
	// ---- what each page fills in
	virtual std::vector<std::string> TabNames() const = 0;
	/** Opening on PageId (a use id or a tab's name); Tab is already 0. */
	virtual void Opened(double Now, const std::string& PageId) = 0;
	virtual void TabChanged(double /*Now*/) {}
	/** A key the frame didn't take (Escape, the tabs, the tip row are the frame's); true when it did something. */
	virtual bool PageKey(const std::string& Name, double Now) = 0;
	/** Escape inside a step (the tip row, the fitting booth, the pawn's second ask): true when it stepped back. */
	virtual bool StepBack(double /*Now*/) { return false; }
	/** A step that holds the tab (the tip row, a modal ask). */
	virtual bool TabsHeld() const { return Tipping; }
	virtual void Body(Canvas& C, const Rect& R, double Now) = 0;
	/** The right column: by default the bill, the tip row while tipping, and the main button. */
	virtual void Side(Canvas& C, const Rect& R, double Now);
	virtual Receipt Bill(double Now) = 0;
	virtual Button Main(double Now) = 0;
	virtual void DoMain(double Now) = 0;
	/** The mouse's minus on a removable receipt line. */
	virtual void RemoveLine(const std::string& /*Id*/, double /*Now*/) {}
	virtual std::vector<Hint> Hints() const;
	virtual Backdrop Backing() const { return Backdrop::Full; }
	/** How much of the left column the body uses (Split: the rest stays clear for the mirror). */
	virtual float BodyWidth(float LeftW) const;
	/** The name in the header (the venue's). */
	virtual std::string Title() const;
	/** The line under the name (the address and the hours by default). */
	virtual std::string Under() const;
	/** The bubble's label when nobody's at the desk (the gym between shifts); "" draws no bubble. */
	virtual std::string KeeperAway() const;
	/** What a tip is a share of: the bill before tax. */
	virtual Chips TipBase() { return 0; }
	Chips TipAmount(int Percent);
	virtual void CapsChanged() {}
	/** The page is closing, however it closes. */
	virtual void Closing(double /*Now*/) {}
	/** The keeper's moment as the page opens. */
	virtual std::string Greeting() const { return "greet"; }
	/** Each frame and before each key, while open (what runs on a clock: the pawn's loupe). */
	virtual void Tick(double /*Now*/) {}

	// ---- the keeper
	struct Keeper
	{
		std::string Id;   // "lonnie"; "" when nobody's there
		std::string Name; // "Lonnie"
		std::string Role; // "Barber"
	};
	Keeper Who() const;
	/** What the keeper says at Moment (town::KeeperLine, or their own words when the town has none). */
	std::string KeeperSays(const std::string& Moment) const;
	void Say(const std::string& Moment, double Now, bool Bad = false);
	void SayText(const std::string& Words, double Now, bool Bad = false);

	// ---- requests
	void Ask(const HostRequest& R) { Requests.push_back(R); }
	void Camera(const std::string& Shot);
	void Preview(const hero::Look& L, const wardrobe::Closet& Closet);
	void EndPreview();
	void Redress();
	/** After a Session call: a skip began, so ask for the service and close (true when it did). */
	bool Started(double Now);
	void WalkAway(double Now);

	// ---- the tip row
	std::vector<int> TipChoices; // percent; empty: no tips here
	int TipIndex = 0;
	int TipDefault = 0; // where the row starts each visit (§5.0: the barber's 20%, Ruby's 18%), whatever was tipped last
	bool Tipping = false;
	int TipPercent() const { return TipChoices.empty() ? 0 : TipChoices[static_cast<size_t>(TipIndex)]; }

	// ---- the stamp and the receipt's lines
	void Stamp(StampKind K, double Now);
	void Touch(const std::string& LineId, double Now) { LineAt[LineId] = Now; }
	bool Declining(double Now) const { return StampWas == StampKind::Declined && Now - StampAt < 4.0; }

	// ---- drawing helpers
	bool Released(const Rect& R) const;
	bool Hover(const Rect& R) const { return frontend_detail::Inside(R, Ptr.X, Ptr.Y); }
	/** The receipt at (X, Y), ReceiptW wide; its bottom. */
	float DrawBill(Canvas& C, const Receipt& R, float X, float Y, double Now);
	/** The main button at Y in the right column; its bottom. */
	float DrawButton(Canvas& C, const Button& B, float X, float Y, double Now);
	/** The tip row at Y; its bottom. */
	float DrawTips(Canvas& C, float X, float Y, double Now);
	/**
	 * A grid of N cards (Cols across, CardH tall) in R, scrolled to keep Sel in view, the wheel scrolling it too. Paint
	 * draws card I at its rect. A click picks it: Sel moves there and Picked (Enter's work) runs.
	 */
	void Grid(Canvas& C, const Rect& R, int N, int Cols, float CardH, const std::function<void(int, const Rect&, bool, bool)>& Paint,
		const std::function<void(int)>& Picked, double Now);
	/** Arrows in a grid of N, Cols across: true when it moved. */
	bool GridKey(const std::string& Name, int N, int Cols, double Now);
	/** The card behind a grid cell (the counter's look), lit when it's the focus; Dim for what can't be had. */
	void CardBack(Canvas& C, const Rect& R, bool Focus, bool Over, bool Dim, double Now);
	/** A wrapped paragraph at (X, Y) Width wide, at most MaxLines (the last one ends in an ellipsis); its bottom. */
	float Paragraph(Canvas& C, const std::string& Text, float X, float Y, float Width, float Size, int Weight, const Color& Col, float LineH, int MaxLines);

	Session& S;
	std::string Venue;
	HostCaps Caps;
	std::string OpenedOn;
	int Tab = 0;
	double TabAt = -10.0;
	int Sel = 0;
	double SelAt = 0.0;
	double OpenedAt = -10.0;
	int FirstRow = 0;
	bool Previewing = false;
	bool Seated = false; // the hero sat down to this page (the barber's chair, a stool): walking away stands them up
	std::string Note;
	double NoteAt = -10.0;
	bool NoteBad = false;
	StampKind StampWas = StampKind::None;
	double StampAt = -10.0;
	std::map<std::string, double> LineAt;

private:
	void DrawHeader(Canvas& C, float LeftW, double Now);
	void DrawTabs(Canvas& C, float LeftW, double Now);
	void DrawFooter(Canvas& C, double Now);
	void SwitchTab(int To, double Now);

	bool Shown = false;
	bool Left = false;
	float PressX = -1.0f;
	float PressY = -1.0f;
	float GridWheel = 0.0f;
	bool ClosingSaid = false;
	std::vector<HostRequest> Requests;
};

// ------------------------------------------------------------------ the pages (one .cpp each)
std::unique_ptr<ShopScreen> MakeBarber(Session& S);
std::unique_ptr<ShopScreen> MakeThrift(Session& S);
std::unique_ptr<ShopScreen> MakeGym(Session& S);
std::unique_ptr<ShopScreen> MakeDiner(Session& S);
std::unique_ptr<ShopScreen> MakePawn(Session& S);
std::unique_ptr<ShopScreen> MakeWardrobe(Session& S);

/** The hero's look with the closet collapsed onto it (wardrobe::ApplyTo): what the portrait and a preview show. */
hero::Look Dressed(const hero::Look& L, const wardrobe::Closet& Closet);
/** The order's line for the Session (a venue and offer ids, a tip). */
Session::ShopOrder Order(const std::string& Venue, const std::vector<std::string>& Offers, int TipPercent = 0);
} // namespace shopframe
} // namespace ui
} // namespace ss
