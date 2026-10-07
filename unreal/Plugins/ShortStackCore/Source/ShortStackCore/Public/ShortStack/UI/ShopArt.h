#pragma once
// Northside's printed and glowing faces (the realism update's spec, §7.4): everything core draws onto the street's
// [screen] slots, and the pictures the shop screens share (garments, the pawn's lots). PropArt.cpp keeps the Lucky
// Penny's; these are the block's. The rule PropArt states holds for all of it: the window never promises what the
// register won't ring up. Every price, hour, menu, timetable and ticket comes from core data (Shop.h, Wardrobe.h,
// Town.h, the Session), so a sign changes the moment the thing it describes does.
//
// Sizes are canvas units; hosts draw each face onto a quad of the same proportions (a SlateDrawList widget or a render
// target, as the Lucky Penny's signs are drawn). Faces with a transparent background say so.

#include "ShortStack/Common.h"
#include "ShortStack/Game/Shop.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/UI/Canvas.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ss
{
class Session;

namespace ui
{
namespace shopart
{
// ------------------------------------------------------------------ sizes (canvas units)
constexpr float HoursDecalW = 400.0f; // a vinyl decal on a door's glass (white on clear: transparent background)
constexpr float HoursDecalH = 560.0f;
constexpr float OpenSignW = 600.0f; // the hanging OPEN / CLOSED card behind a door's glass
constexpr float OpenSignH = 320.0f;
constexpr float PriceBoardW = 1200.0f; // the barber's board on the wall
constexpr float PriceBoardH = 1500.0f;
constexpr float LetterBoardW = 1800.0f; // Ruby's letter board over the pass
constexpr float LetterBoardH = 900.0f;
constexpr float RackTagW = 1000.0f; // a thrift rack's sign
constexpr float RackTagH = 300.0f;
constexpr float PriceTagW = 240.0f; // a pawn shop's hanging string tag on a piece of stock
constexpr float PriceTagH = 400.0f;
constexpr float PawnTicketW = 640.0f; // the pink carbon pawn ticket
constexpr float PawnTicketH = 960.0f;
constexpr float PawnCardW = 900.0f; // the pawn's house rules and the door's buzzer sticker
constexpr float PawnCardH = 560.0f;
constexpr float WhiteboardW = 1400.0f; // IronHouse's workout of the day
constexpr float WhiteboardH = 1000.0f;
constexpr float BackSignW = 700.0f; // the gym desk's BACK AT sign
constexpr float BackSignH = 440.0f;
constexpr float SandwichW = 640.0f; // one face of an A-frame chalkboard
constexpr float SandwichH = 960.0f;
constexpr float TimetableW = 720.0f; // the 14's timetable in the shelter
constexpr float TimetableH = 1080.0f;
constexpr float ShelterAdW = 1000.0f; // the shelter's lightbox ad
constexpr float ShelterAdH = 1500.0f;
constexpr float DestSignW = 1600.0f; // the bus's amber destination sign (black background)
constexpr float DestSignH = 220.0f;
constexpr float PlateW = 600.0f; // a license plate (12 x 6 in.)
constexpr float PlateH = 300.0f;
constexpr float ClockW = 1024.0f; // the bank's clock dial
constexpr float ClockH = 1024.0f;
constexpr float MarqueeW = 2400.0f; // the Rialto's marquee letter boards
constexpr float MarqueeH = 640.0f;
constexpr float ValanceW = 2000.0f; // an awning's valance (the fabric's color is the mesh's tint: transparent background)
constexpr float ValanceH = 160.0f;

// ------------------------------------------------------------------ the shops' faces
/** A venue's hours on its door (shop::HoursSign, split on " · "): its name, the hours, the phone line. */
SHORTSTACKCORE_API void HoursDecal(Canvas& C, const std::string& VenueId);
/** The hanging card that flips with the hours: COME IN, WE'RE OPEN, or SORRY, WE'RE CLOSED with when it opens. */
SHORTSTACKCORE_API void OpenSign(Canvas& C, const std::string& VenueId, double World);
/** Fifth St. Barbers' price board: every cut, beard, color and brow at the offers' prices and minutes. */
SHORTSTACKCORE_API void PriceBoard(Canvas& C);
/** Ruby's letter board over the pass: the menu at its prices, white letters pressed into black felt. */
SHORTSTACKCORE_API void LetterBoard(Canvas& C);
/**
 * A thrift rack's sign: "tops", "outer", "pants", "shoes" or "hats" (the rack uses' suffixes), with the price range of
 * what hangs on it this week (Racks) and Saturday's yellow tags.
 */
SHORTSTACKCORE_API void RackTag(Canvas& C, const std::string& Rack, const Session& S, uint64_t GarmentKinds);
/** A string tag on a piece of the pawn's stock: the price, and LOT 114 · NINTH ST. on the storage auction's lots. */
SHORTSTACKCORE_API void PriceTag(Canvas& C, const std::string& Name, Chips PriceCents, bool Auction);
/** The pink carbon pawn ticket (the ticket number, the item, the loan, taken, REDEEM BY and today's price). */
SHORTSTACKCORE_API void PawnTicket(Canvas& C, const shop::Ticket& T, const std::string& ItemName, double World);
/** The pawn's posted cards: 0 the house rules (ALL SALES FINAL · NO ID NO LOAN · 30-DAY LOANS · TICKET REQUIRED TO
 *  REDEEM), 1 the door's PLEASE WAIT FOR THE BUZZER, 2 the window's WE BUY GOLD card with the hours. */
SHORTSTACKCORE_API void PawnCard(Canvas& C, int Which, double World);
/** IronHouse's whiteboard: the day's workout (the same all day, a new one each day) and the desk's hours. */
SHORTSTACKCORE_API void Whiteboard(Canvas& C, double World);
/** The desk's tent card while Rosa's away: BACK AT 4 (when she's next on, from the gym's staffed hours). */
SHORTSTACKCORE_API void BackSign(Canvas& C, double World);
/**
 * A sandwich board's chalk face: "diner", "thrift", "pawn" (what each really sells or buys, at its prices), "ginos" and
 * "kims" (display shops: their hours and what's in the window, no prices).
 */
SHORTSTACKCORE_API void SandwichBoard(Canvas& C, const std::string& Which, double World);
/** The 14's timetable in the shelter: the next departures from Stop (town::BusTimes) and the posted service. */
SHORTSTACKCORE_API void Timetable(Canvas& C, const std::string& Stop, double World);
/** The shelter's lightbox ad: RiverLine · the night shift never closes. */
SHORTSTACKCORE_API void ShelterAd(Canvas& C);
/** The bus's amber LED sign ("14 EMBERCREST", "14 DOWNTOWN"; black background). Side: the shorter side sign. */
SHORTSTACKCORE_API void DestinationSign(Canvas& C, const std::string& Text, bool Side = false);
/** A plate's text from a seed: three letters and four digits ("KRT 4471"), never one of another seed's in a lot. */
SHORTSTACKCORE_API std::string PlateText(uint64_t Seed);
/** A license plate with Text. */
SHORTSTACKCORE_API void Plate(Canvas& C, const std::string& Text);
/** The bank's clock dial at Minutes past midnight (world time): Roman numerals, the hands, the maker's name. */
SHORTSTACKCORE_API void BankClock(Canvas& C, double Minutes);
/** The Rialto's marquee: CLOSED FOR RENOVATION / REOPENING SPRING, or on Fridays MIDNIGHT SHOW FRI / DOORS 11:30 PM. */
SHORTSTACKCORE_API void Marquee(Canvas& C, double World);
/** An awning's valance: the shop's name printed on the fabric's edge (white on clear: the mesh tints the fabric). */
SHORTSTACKCORE_API void Valance(Canvas& C, const std::string& VenueId);

/**
 * The keyed entry point the stages use for a [screen]: "hours:<venue>", "open:<venue>", "board:barber",
 * "board:diner", "rack:<tops|outer|pants|shoes|hats>", "tag:<lot id>", "pawnticket:<n>", "pawncard:<0..2>",
 * "whiteboard", "backat", "sandwich:<which>", "timetable:<stop>", "shelterad", "dest:<text>", "destside:<text>",
 * "plate:<text>", "clock:<minutes>", "marquee", "valance:<venue>". FaceSize gives its canvas size (false for an
 * unknown key); DrawFace draws it (false when it has nothing to draw: an unknown key, a ticket no longer open).
 */
SHORTSTACKCORE_API bool FaceSize(const std::string& Key, float& W, float& H);
SHORTSTACKCORE_API bool DrawFace(Canvas& C, const std::string& Key, const Session& S, uint64_t GarmentKinds = ~0ull);

// ------------------------------------------------------------------ pictures the screens share
/** The color a garment is drawn in (wardrobe::Tone: the thrift's swatches, the portrait and the 3D's tint agree). */
SHORTSTACKCORE_API uint32_t GarmentTone(const wardrobe::Garment& G);
/** A garment as it hangs or lies (its cut, color, print and condition), centered in R. */
SHORTSTACKCORE_API void Garment(Canvas& C, const wardrobe::Garment& G, const Rect& R);
/** One of the pawn's lots (gear, an award, the cup, a keepsake, a garment, a curio), centered in R. */
SHORTSTACKCORE_API void LotPicture(Canvas& C, const Session& S, const shop::Lot& L, const Rect& R, double Time);
/** A lot's kind from its id ("gear:", "memento:", "award:", "keepsake:", "garment:", "curio:"). */
SHORTSTACKCORE_API shop::LotKind LotKindOf(const std::string& LotId);
/** What a lot is called, from its id (a ticket names only the lot): "8-core desktop", "Embercrest Sunday Cup". */
SHORTSTACKCORE_API std::string LotName(const Session& S, const std::string& LotId);
/** Whether the 3D can stand a lot up (the pawn's shelf and cases, HostCaps' bits): false reads NOT IN STOCK. */
SHORTSTACKCORE_API bool LotShowable(const std::string& LotId, uint64_t GearKinds, uint64_t GarmentKinds, uint32_t Curios);

// ------------------------------------------------------------------ the trophy case (§7.4, P1)
/**
 * What an award's piece is stamped in the trophy case (Session::PawnHolds on its "award:<day>:<event>" lot): "IN PAWN"
 * while a ticket holds it, "SOLD" once it's Augie's or gone; "" when it's on the hero's shelf. The title and its stats
 * stay in the records either way.
 */
SHORTSTACKCORE_API std::string AwardStamp(const Session& S, const world::Award& A);
/** Draws a trophy's stamp (AwardStamp's word) across a piece centered at (X, Y), Size across: the piece under it dims. */
SHORTSTACKCORE_API void DrawAwardStamp(Canvas& C, const std::string& Stamp, float X, float Y, float Size);

// ------------------------------------------------------------------ the racks and the shelf (the 3D and the screens agree)
/** This week's racks at the thrift (wardrobe::ThriftWeek's), of what the host can draw: what ThriftScreen sells and the 3D hangs. */
SHORTSTACKCORE_API std::vector<std::string> ThriftRack(const Session& S, uint64_t GarmentKinds);
/** Which rack a garment hangs on: "tops", "outer", "pants", "shoes" or "hats". */
SHORTSTACKCORE_API const char* RackOf(const wardrobe::Garment& G);
/** A yellow tag (half off on Saturdays, wardrobe::TagOf): the tag and the register agree. */
SHORTSTACKCORE_API bool YellowTag(const wardrobe::Garment& G, double World);
/** The tag's color (0xRRGGBB): wardrobe::TagOf's white, yellow, pink, green or blue. */
SHORTSTACKCORE_API uint32_t TagColor(const wardrobe::Garment& G, double World);
} // namespace shopart
} // namespace ui
} // namespace ss
