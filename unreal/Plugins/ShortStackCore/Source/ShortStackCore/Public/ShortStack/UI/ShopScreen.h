#pragma once
// Northside's shop screens (the realism update's spec, §7.4): one interface for the barber's Style Book, the thrift's
// racks, the gym's desk and machines, Ruby's menu, the pawn's counter, cage and cases, and the closet. Everything draws
// into a Canvas 1080 logical units tall, as StoreCounter does, and nothing here knows Unreal: the host feeds a screen
// keys, the pointer and HostCaps, draws it, and drains TakeRequests for whatever it needs in 3D.

#include "ShortStack/Game/Hero.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/UI/Ui.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ss
{
class Session;

namespace ui
{
/**
 * What the game can show in 3D (§5.0: the 3D only sells what it can show). A choice the host can't show reads PORTRAIT
 * ONLY (NOT IN STOCK at the pawn) and can't be bought. HairStyles, FacialHair and Brows are indexed by hero::Look's
 * value for that slot (Hair, FacialHair, Brows); an index past the end counts as missing.
 */
struct HostCaps
{
	std::vector<bool> HairStyles;
	std::vector<bool> FacialHair;
	std::vector<bool> Brows;
	uint64_t GarmentKinds = 0; // a bit per wardrobe::Cut that has a mesh
	uint64_t GearKinds = 0;    // a bit per gear::Catalog index the pawn's shelf can stand up
	uint32_t Curios = 0;       // the pawn's curios that render: 1 the Lucky Seven chip, 2 the wristwatch (aviators and trilby go by GarmentKinds)
	bool Preview3D = false;    // the hero can be dressed for a preview in 3D; without it the pages preview on the portrait
};

/**
 * Something a screen asks the game to do in 3D. The host drains them with TakeRequests every frame (§7.5.3):
 *  - PreviewLook: dress the hero in Look and Closet for a moment (the barber's draft look, the thrift's try-on).
 *  - EndPreview: back to the hero's own look.
 *  - Camera: blend to the registry shot Name ("barber-mirror", "thrift-fitting", "pawn-counter", "pawn-case",
 *    "pawn-cage") and hold it while the screen is open.
 *  - BeginService: a service has started. The page calls Session::ShopService itself, as StoreCounter calls Checkout
 *    (the order, the tip and the DECLINED stamp are the page's), and asks for this only once the charge went through
 *    and the TimeSkip began, so the host must not call ShopService again. Name is the service's offer ids as the
 *    skip's activity id lists them ("barber/crew,barber/beard-short"); the host plays the venue's cinematic for them
 *    (AShopStage::Cinematic) under the skip.
 *  - Redress: the hero now wears Closet for real (the closet's Wear and Take off, the thrift's "Wear it out").
 *  - Sit, Stand: the hero's seated mode. Name is the seat's use or station id ("barber-chair1"), or empty for where
 *    the hero is.
 *  - Talk: the talk card's choice, Name its id.
 *  - Leave: the talk card was walked away from.
 */
struct HostRequest
{
	enum Kind : int
	{
		PreviewLook,
		EndPreview,
		Camera,
		BeginService,
		Redress,
		Sit,
		Stand,
		Talk,
		Leave
	} K = Camera;
	hero::Look Look;         // PreviewLook
	wardrobe::Closet Closet; // PreviewLook, Redress
	std::string Name;        // Camera: a shot id; BeginService: the offer ids; Sit: a seat id; Talk: a choice id
};

/**
 * One shop's screen, drawn full screen over the 3D shop into a canvas 1080 logical units tall (the width follows the
 * viewport). Keys use StoreCounter's names: "Up", "Down", "Left", "Right", "Enter" (or "Space"), "Q" and "E" (or
 * "TabPrev" and "TabNext") for tabs, "Tab" (or "Reset": the pad's Y) to pay or confirm, "Backspace" (or "Delete", or
 * "PutBack": the pad's X) to take one out, and "Escape" (or "P") to walk away. For a moment after opening only Escape
 * gets through (StoreCounter's OpenGrace, 0.3 s), since the key that opened it may still be held down.
 *
 * Now is the host's clock in real seconds (StoreCounter's RealTime), not world minutes; the world's time comes from the
 * Session. The host polls TakeLeave every frame, after Draw as well as after Key: a click on the footer's WALK AWAY
 * walks away inside Draw, and a host that only checks after keys would keep the input on a screen that has closed.
 */
class ShopScreen
{
public:
	static constexpr float Height = 1080.0f;
	virtual ~ShopScreen() = default;
	/**
	 * Opens the screen. Page: the rack, tab or station that opened it, a registry use id or a tab's name. An empty or
	 * unknown page opens the venue's first page. The pawn opens on the page of its use: "pawn-counter" SELL,
	 * "pawn-cage" TICKETS, "pawn-cases" BUY on CURIOS and "pawn-electronics" BUY on GEAR. The others: "barber-chair1"
	 * (or "CUT", "BEARD", "COLOR", "BROWS"); "thrift-rack-tops", "thrift-rack-outer", "thrift-rack-pants",
	 * "thrift-shoes", "thrift-hats", "thrift-booth", "thrift-register" and "thrift-donate"; "gym-desk", or a machine's use
	 * ("gym-treadmill1".."gym-treadmill5", "gym-bag", "gym-weights", "gym-cooler"); a diner seat ("diner-stool1",
	 * "diner-booth1", ...); and for the closet, where it was opened from.
	 */
	virtual void Open(double Now, const std::string& Page) = 0; // Page: the rack, tab or station that opened it
	/** The host ends the screen (the shop closing, a skip starting elsewhere). TakeLeave doesn't report it. */
	virtual void Close(double Now) = 0;
	virtual bool IsOpen() const = 0;
	virtual bool TakeLeave() = 0;          // the player walked away (the host hands control back)
	virtual void Key(const std::string& Name, double Now) = 0; // StoreCounter's key names
	virtual void Draw(Canvas& C, double Now) = 0;
	/** What the host can show in 3D; call it before Open, and again whenever it changes. */
	virtual void SetCaps(const HostCaps& /*Caps*/) {}
	/** What the screen asked for since the last call, oldest first (and then forgets them). */
	virtual std::vector<HostRequest> TakeRequests() = 0;
	Pointer Ptr;
	bool Gamepad = false;
};

/**
 * The screen for a venue: "barber", "thrift", "gym", "diner", "pawn", or "wardrobe" (the closet, opened from the
 * apartment, Dee's room, the thrift's booth and its "Wear it out"). nullptr for any other id. The session must outlive
 * the screen.
 */
SHORTSTACKCORE_API std::unique_ptr<ShopScreen> MakeShopScreen(const std::string& Venue, Session& S); // "barber", "thrift", "gym", "diner", "pawn", "wardrobe"
} // namespace ui
} // namespace ss
