// Northside's printed and glowing faces, and the pictures the shop screens share (ShopArt.h). Every price, hour, menu,
// timetable and ticket is read from core data as it's drawn.
#include "ShortStack/UI/ShopArt.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Gear.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Town.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/UI/EventArt.h"
#include "ShortStack/UI/StreamArt.h"
#include "ShortStack/UI/Ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace ss
{
namespace ui
{
namespace shopart
{
namespace
{
float Fl(int V)
{
	return static_cast<float>(V);
}

Color Alpha(Color C, float A)
{
	C.A *= A;
	return C;
}

Color Lum(const Color& C, float K)
{
	return K >= 0.0f ? Mix(C, Hex(0xffffff), K) : Mix(C, Hex(0x000000), -K);
}

float Luma(const Color& C)
{
	return 0.299f * C.R + 0.587f * C.G + 0.114f * C.B;
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

/** "$12" for whole dollars, "$4.99" otherwise. */
std::string Dollars(Chips Cents)
{
	std::string M = Money(Cents);
	if (Cents % 100 == 0 && M.size() > 3 && M.compare(M.size() - 3, 3, ".00") == 0)
	{
		M.resize(M.size() - 3);
	}
	return M;
}

uint64_t Fnv(const std::string& S)
{
	uint64_t H = 1469598103934665603ull;
	for (const char Ch : S)
	{
		H ^= static_cast<uint8_t>(Ch);
		H *= 1099511628211ull;
	}
	return H;
}

std::vector<std::string> Split(const std::string& S, const std::string& Sep)
{
	std::vector<std::string> Out;
	size_t Start = 0;
	while (true)
	{
		const size_t At = S.find(Sep, Start);
		Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
		if (At == std::string::npos)
		{
			break;
		}
		Start = At + Sep.size();
	}
	return Out;
}

/** Text that fits Width: Size, or smaller. */
float Fit(Canvas& C, const std::string& Text, float Size, int Weight, float Width)
{
	const float W = C.Measure(Text, Size, Weight);
	return W > Width && W > 0.0f ? Size * Width / W : Size;
}

void FitText(Canvas& C, const std::string& Text, float X, float Y, float Size, int Weight, const Color& Col, Align A, float Width)
{
	C.Text(Text, X, Y, Ts(Fit(C, Text, Size, Weight, Width), Weight, Col, A));
}

/** Chalk on a board: the stroke a little ragged (offset ghosts), the dust under it. */
void Chalk(Canvas& C, const std::string& Text, float X, float Y, float Size, const Color& Col, Align A, float Width, int Weight = 700)
{
	const float S = Fit(C, Text, Size, Weight, Width);
	C.Text(Text, X + S * 0.02f, Y + S * 0.025f, Ts(S, Weight, Alpha(Col, 0.18f), A));
	C.Text(Text, X - S * 0.015f, Y - S * 0.01f, Ts(S, Weight, Alpha(Col, 0.25f), A));
	C.Text(Text, X, Y, Ts(S, Weight, Alpha(Col, 0.9f), A));
}

/** A chalkboard in R: the slate, the smudges, a wooden frame. */
void Slate(Canvas& C, const Rect& R, float Frame)
{
	C.FillRect(R, Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Hex(0x6b4a2e), Hex(0x4a3220)));
	const Rect In{R.X + Frame, R.Y + Frame, R.W - Frame * 2.0f, R.H - Frame * 2.0f};
	C.FillRect(In, Paint::Linear({In.X, In.Y}, {In.X + In.W, In.Y + In.H}, Hex(0x2a302c), Hex(0x1c211e)));
	for (int K = 0; K < 6; ++K)
	{
		const float Sx = In.X + In.W * (0.15f + 0.14f * Fl(K));
		const float Sy = In.Y + In.H * (0.2f + 0.11f * Fl((K * 5) % 7));
		C.FillEllipse(Sx, Sy, In.W * 0.18f, In.H * 0.05f, Paint::Radial({Sx, Sy}, 0.0f, {Sx, Sy}, In.W * 0.18f, Rgba(255, 255, 255, 0.05f), 0.5f, Rgba(255, 255, 255, 0.02f), Rgba(255, 255, 255, 0.0f)));
	}
}

/** White plastic letters pressed into a felt board, with their little shadow. */
void Letters(Canvas& C, const std::string& Text, float X, float Y, float Size, Align A, float Width, const Color& Col = Hex(0xf4f1ea))
{
	const float S = Fit(C, Text, Size, 700, Width);
	C.Text(Text, X + S * 0.04f, Y + S * 0.05f, Ts(S, 700, Rgba(0, 0, 0, 0.55f), A));
	C.Text(Text, X, Y, Ts(S, 700, Col, A));
}

/** The 555-01xx line a venue answers on (only 555-01xx, §4.9). */
std::string PhoneOf(const std::string& Venue)
{
	if (Venue == "barber")
	{
		return "555-0118";
	}
	if (Venue == "thrift")
	{
		return "555-0126";
	}
	if (Venue == "gym")
	{
		return "555-0134";
	}
	if (Venue == "diner")
	{
		return "555-0151";
	}
	if (Venue == "pawn")
	{
		return "555-0187";
	}
	char B[16];
	std::snprintf(B, sizeof(B), "555-01%02d", 10 + static_cast<int>(Fnv(Venue) % 80));
	return B;
}

const char* const DayShort[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

std::string HourWord(int Minutes)
{
	const int M = ((Minutes % 1440) + 1440) % 1440;
	if (M == 0)
	{
		return "MIDNIGHT";
	}
	if (M == 720)
	{
		return "NOON";
	}
	const int H = M / 60;
	const int Mm = M % 60;
	const int H12 = H % 12 == 0 ? 12 : H % 12;
	char B[16];
	if (Mm == 0)
	{
		std::snprintf(B, sizeof(B), "%d %s", H12, H < 12 ? "AM" : "PM");
	}
	else
	{
		std::snprintf(B, sizeof(B), "%d:%02d %s", H12, Mm, H < 12 ? "AM" : "PM");
	}
	return B;
}

/** The hours in lines when the venue's sign has none to give: one line per run of days with the same hours. */
std::vector<std::string> HoursLines(const shop::Venue& V)
{
	std::vector<std::string> Out;
	const std::string Sign = shop::HoursSign(V);
	if (!Sign.empty())
	{
		for (const std::string& L : Split(Sign, " \xC2\xB7 "))
		{
			if (!L.empty())
			{
				Out.push_back(L);
			}
		}
		return Out;
	}
	bool AllDay = !V.Open.empty();
	for (const shop::Window& W : V.Open)
	{
		AllDay = AllDay && W.Days == 0x7f && W.Opens == 0 && W.Closes >= 1440;
	}
	if (AllDay)
	{
		return {"OPEN 24 HOURS", "7 DAYS A WEEK"};
	}
	for (const shop::Window& W : V.Open)
	{
		int First = -1;
		int Last = -1;
		for (int D = 0; D < 7; ++D)
		{
			if (W.Days & (1 << D))
			{
				First = First < 0 ? D : First;
				Last = D;
			}
		}
		if (First < 0)
		{
			continue;
		}
		const std::string Days = W.Days == 0x7f ? std::string("DAILY") : First == Last ? std::string(DayShort[First]) : std::string(DayShort[First]) + "\xE2\x80\x93" + DayShort[Last];
		Out.push_back(Days + " " + HourWord(W.Opens) + "\xE2\x80\x93" + HourWord(W.Closes));
	}
	return Out;
}

const shop::Offer* OfferLike(const std::string& Venue, const char* Needle)
{
	for (const shop::Offer& O : shop::Offers(Venue))
	{
		if (O.Id.find(Needle) != std::string::npos)
		{
			return &O;
		}
	}
	return nullptr;
}

/** The venue's name for a sign: the table's, or the spec's for the five shops. */
std::string NameOf(const std::string& Venue)
{
	if (const shop::Venue* V = shop::FindVenue(Venue))
	{
		return V->Name;
	}
	if (Venue == "barber")
	{
		return "Fifth St. Barbers";
	}
	if (Venue == "thrift")
	{
		return "Second Chance Thrift";
	}
	if (Venue == "gym")
	{
		return "IronHouse Gym";
	}
	if (Venue == "diner")
	{
		return "Ruby's";
	}
	if (Venue == "pawn")
	{
		return "Northside Pawn & Loan";
	}
	return std::string();
}

/** The next time a venue is staffed (the gym's BACK AT), on the quarter hour. */
double NextStaffed(const shop::Venue& V, double World)
{
	for (int K = 0; K <= 48 * 4 * 7; ++K)
	{
		const double At = std::floor(World / 15.0) * 15.0 + 15.0 * K;
		if (At >= World && shop::IsStaffed(V, At))
		{
			return At;
		}
	}
	return World;
}
} // namespace

// ------------------------------------------------------------------ the racks and the shelf

std::vector<std::string> ThriftRack(const Session& S, uint64_t GarmentKinds)
{
	return wardrobe::ThriftStock(wardrobe::ThriftWeek(S.WorldMinutes()), S.WorldSeed(), GarmentKinds);
}

const char* RackOf(const wardrobe::Garment& G)
{
	switch (G.Where)
	{
	case wardrobe::Slot::Outer: return "outer";
	case wardrobe::Slot::Bottom: return "pants";
	case wardrobe::Slot::Shoes: return "shoes";
	case wardrobe::Slot::Hat:
	case wardrobe::Slot::Eyes: return "hats";
	default: return "tops";
	}
}

bool YellowTag(const wardrobe::Garment& G, double /*World*/)
{
	return wardrobe::TagOf(G) == 1;
}

uint32_t TagColor(const wardrobe::Garment& G, double /*World*/)
{
	// wardrobe::TagOf: 0 white, 1 yellow, 2 pink, 3 green, 4 blue (the tag gun's stock).
	static const uint32_t Tags[5] = {0xf4f1ea, 0xffd23f, 0xf7a8c4, 0x9be39b, 0x8ec5ff};
	return Tags[std::clamp(wardrobe::TagOf(G), 0, 4)];
}

shop::LotKind LotKindOf(const std::string& LotId)
{
	const std::string Pre = LotId.substr(0, LotId.find(':'));
	if (Pre == "memento")
	{
		return shop::LotKind::Memento;
	}
	if (Pre == "award")
	{
		return shop::LotKind::Award;
	}
	if (Pre == "keepsake")
	{
		return shop::LotKind::Keepsake;
	}
	if (Pre == "garment")
	{
		return shop::LotKind::Garment;
	}
	if (Pre == "curio")
	{
		return shop::LotKind::Curio;
	}
	return shop::LotKind::Gear;
}

namespace
{
const world::Award* AwardOf(const Session& S, const std::string& LotId)
{
	// "award:<day>:<event>"
	const size_t A = LotId.find(':');
	const size_t B = A == std::string::npos ? std::string::npos : LotId.find(':', A + 1);
	if (B == std::string::npos)
	{
		return nullptr;
	}
	const int Day = std::atoi(LotId.substr(A + 1, B - A - 1).c_str());
	const std::string Event = LotId.substr(B + 1);
	for (const world::Award& Aw : S.Living().HeroAwards())
	{
		if (Aw.Day == Day && Aw.Event == Event)
		{
			return &Aw;
		}
	}
	return nullptr;
}

const char* KeepsakeName(const std::string& Key)
{
	if (Key == "kitchen")
	{
		return "Japanese-steel knife roll";
	}
	if (Key == "dealerskid")
	{
		return "Mom's dress watch";
	}
	if (Key == "dropout")
	{
		return "Steel diver's watch";
	}
	if (Key == "bouncer")
	{
		return "Embercrest service ring";
	}
	if (Key == "hustler")
	{
		return "Gold rope chain";
	}
	if (Key == "newcomer")
	{
		return "Acoustic guitar";
	}
	return "Keepsake";
}

const char* CurioName(const std::string& Key)
{
	if (Key.find("aviator") != std::string::npos)
	{
		return "Gold-rim aviators";
	}
	if (Key.find("trilby") != std::string::npos)
	{
		return "Felt trilby";
	}
	if (Key.find("chip") != std::string::npos || Key.find("seven") != std::string::npos)
	{
		return "The Lucky Seven chip";
	}
	if (Key.find("watch") != std::string::npos)
	{
		return "Steel wristwatch";
	}
	return "Curio";
}
} // namespace

std::string LotName(const Session& S, const std::string& LotId)
{
	for (const shop::Lot& L : S.PawnLots())
	{
		if (L.Id == LotId && !L.Name.empty())
		{
			return L.Name;
		}
	}
	for (const shop::Lot& L : S.PawnShelf())
	{
		if (L.Id == LotId && !L.Name.empty())
		{
			return L.Name;
		}
	}
	const std::string Rest = LotId.substr(LotId.find(':') + 1);
	switch (LotKindOf(LotId))
	{
	case shop::LotKind::Gear:
	{
		const gear::Item* I = gear::Find(Rest);
		return I ? I->Name : Rest;
	}
	case shop::LotKind::Memento: return Rest == "cup" ? "The Embercrest cup" : Rest == "deechip" ? "Dee's $100 chip" : Rest;
	case shop::LotKind::Award:
	{
		const world::Award* A = AwardOf(S, LotId);
		return A ? std::string(A->Ring ? "Ring: " : "Bracelet: ") + A->Event : std::string("A title's piece");
	}
	case shop::LotKind::Keepsake:
	{
		if (const hero::BackgroundInfo* B = hero::FindBackground(Rest))
		{
			const shop::Lot K = shop::KeepsakeOf(B->Id);
			if (!K.Name.empty())
			{
				return K.Name;
			}
		}
		return KeepsakeName(Rest);
	}
	case shop::LotKind::Garment:
	{
		const wardrobe::Garment* G = wardrobe::Find(Rest);
		return G ? G->Name : Rest;
	}
	case shop::LotKind::Curio:
	{
		const shop::Curio* Cu = shop::FindCurio(Rest);
		return Cu && !Cu->Name.empty() ? Cu->Name : CurioName(Rest);
	}
	}
	return Rest;
}

bool LotShowable(const std::string& LotId, uint64_t GearKinds, uint64_t GarmentKinds, uint32_t Curios)
{
	const std::string Rest = LotId.substr(LotId.find(':') + 1);
	auto Cut = [&](wardrobe::Cut K) { return (GarmentKinds >> static_cast<int>(K) & 1ull) != 0; };
	switch (LotKindOf(LotId))
	{
	case shop::LotKind::Gear:
	{
		const std::vector<gear::Item>& Cat = gear::Catalog();
		for (size_t I = 0; I < Cat.size() && I < 64; ++I)
		{
			if (Cat[I].Id == Rest)
			{
				return (GearKinds >> I & 1ull) != 0;
			}
		}
		return false;
	}
	case shop::LotKind::Garment:
	{
		const wardrobe::Garment* G = wardrobe::Find(Rest);
		return G && Cut(G->Kind);
	}
	case shop::LotKind::Curio:
		if (const shop::Curio* Cu = shop::FindCurio(Rest))
		{
			// The case's own record: its mesh bit on the hero, or the garment it is.
			if (Cu->Bit != 0)
			{
				return (Curios & Cu->Bit) != 0;
			}
			const wardrobe::Garment* G = wardrobe::Find(Cu->Garment);
			return G ? Cut(G->Kind) : true;
		}
		if (Rest.find("aviator") != std::string::npos)
		{
			return Cut(wardrobe::Cut::Aviators);
		}
		if (Rest.find("trilby") != std::string::npos)
		{
			return Cut(wardrobe::Cut::Trilby);
		}
		if (Rest.find("chip") != std::string::npos || Rest.find("seven") != std::string::npos)
		{
			return (Curios & 1u) != 0;
		}
		if (Rest.find("watch") != std::string::npos)
		{
			return (Curios & 2u) != 0;
		}
		return true;
	default: return true;
	}
}

std::string AwardStamp(const Session& S, const world::Award& A)
{
	const std::string Id = "award:" + std::to_string(A.Day) + ":" + A.Event;
	if (!S.PawnHolds(Id))
	{
		return std::string();
	}
	for (const shop::Ticket& T : S.Life.Pawn.Tickets)
	{
		if (T.Lot == Id)
		{
			return "IN PAWN";
		}
	}
	return "SOLD";
}

void DrawAwardStamp(Canvas& C, const std::string& Stamp, float X, float Y, float Size)
{
	if (Stamp.empty())
	{
		return;
	}
	// The piece dims under a rubber stamp, turned a little: red for SOLD, the pawn's blue for IN PAWN, inked bright
	// enough to read over a ruby or a bracelet's plate. The dimming stays inside the piece's own Size, and so does the
	// stamp (its letters smaller for a long word), so pieces side by side in a case never cover each other's.
	C.FillCircle(X, Y, Size * 0.5f, Paint(Rgba(6, 9, 16, 0.55f)));
	const Color Ink = Stamp == "SOLD" ? Hex(0xf0435a) : Hex(0x5b9cf8);
	float S = std::max(9.0f, Size * 0.16f);
	const auto Across = [&C, &Stamp](float Px) {
		float Sum = 0.0f;
		for (const char Ch : Stamp)
		{
			Sum += C.Measure(std::string(1, Ch), Px, 900);
		}
		return Sum + Px * 0.08f * static_cast<float>(Stamp.size() - 1) + Px * 1.2f;
	};
	if (Across(S) > Size * 0.94f)
	{
		S = std::max(7.0f, S * Size * 0.94f / Across(S));
	}
	const float W = Across(S);
	C.Save();
	C.Translate(X, Y);
	C.Rotate(-0.26f);
	C.FillRoundRect({-W * 0.5f, -S, W, S * 2.0f}, S * 0.3f, Paint(Rgba(6, 9, 16, 0.6f)));
	C.StrokeRoundRect({-W * 0.5f, -S, W, S * 2.0f}, S * 0.3f, Alpha(Ink, 0.9f), std::max(1.5f, S * 0.12f));
	// Letter by letter, so the word runs along the stamp's tilt (a run of text keeps its own baseline level).
	float Px = -W * 0.5f + S * 0.6f;
	for (const char Ch : Stamp)
	{
		Px += C.Text(std::string(1, Ch), Px, S * 0.36f, Ts(S, 900, Alpha(Ink, 0.95f))) + S * 0.08f;
	}
	C.Restore();
}

// ------------------------------------------------------------------ the shops' faces

void HoursDecal(Canvas& C, const std::string& VenueId)
{
	const shop::Venue* V = shop::FindVenue(VenueId);
	const std::string Name = NameOf(VenueId);
	if (Name.empty())
	{
		return;
	}
	const float W = HoursDecalW;
	const Color Vinyl = Hex(0xf4f2ec);
	// Cut vinyl on the glass: a rule, the name, the hours, the phone.
	C.FillRect({40.0f, 30.0f, W - 80.0f, 4.0f}, Paint(Vinyl));
	const std::vector<std::string> Words = Split(Up(Name), " ");
	float Y = 92.0f;
	std::string Row;
	std::vector<std::string> Rows;
	for (const std::string& Wd : Words)
	{
		const std::string Trial = Row.empty() ? Wd : Row + " " + Wd;
		if (!Row.empty() && C.Measure(Trial, 46.0f, 900) > W - 70.0f)
		{
			Rows.push_back(Row);
			Row = Wd;
		}
		else
		{
			Row = Trial;
		}
	}
	Rows.push_back(Row);
	for (const std::string& R : Rows)
	{
		FitText(C, R, W * 0.5f, Y, 46.0f, 900, Vinyl, Align::Center, W - 60.0f);
		Y += 50.0f;
	}
	Y += 6.0f;
	C.FillRect({W * 0.5f - 50.0f, Y - 22.0f, 100.0f, 2.0f}, Paint(Alpha(Vinyl, 0.8f)));
	Y += 26.0f;
	C.Text("HOURS", W * 0.5f, Y, Ts(24.0f, 800, Alpha(Vinyl, 0.85f), Align::Center));
	Y += 44.0f;
	const std::vector<std::string> Lines = V ? HoursLines(*V) : std::vector<std::string>();
	for (const std::string& L : Lines)
	{
		if (Y > HoursDecalH - 90.0f)
		{
			break;
		}
		FitText(C, L, W * 0.5f, Y, 30.0f, 700, Vinyl, Align::Center, W - 50.0f);
		Y += 40.0f;
	}
	if (V && V->MembersAnyHour)
	{
		FitText(C, "MEMBERS ANY HOUR", W * 0.5f, Y + 4.0f, 26.0f, 800, Vinyl, Align::Center, W - 50.0f);
	}
	C.Text(PhoneOf(VenueId), W * 0.5f, HoursDecalH - 40.0f, Ts(26.0f, 700, Alpha(Vinyl, 0.85f), Align::Center));
}

void OpenSign(Canvas& C, const std::string& VenueId, double World)
{
	const shop::Venue* V = shop::FindVenue(VenueId);
	const bool Open = V && (shop::IsOpen(*V, World) || V->MembersAnyHour);
	const bool Staffed = V && shop::IsStaffed(*V, World);
	const float W = OpenSignW;
	const float H = OpenSignH;
	// The card on its string from the suction cup.
	C.StrokePolyline({{W * 0.5f, 6.0f}, {W * 0.22f, 70.0f}}, false, Hex(0x2b2723), 3.0f, true);
	C.StrokePolyline({{W * 0.5f, 6.0f}, {W * 0.78f, 70.0f}}, false, Hex(0x2b2723), 3.0f, true);
	C.FillCircle(W * 0.5f, 8.0f, 9.0f, Paint(Rgba(220, 230, 240, 0.7f)));
	const Rect Card{20.0f, 66.0f, W - 40.0f, H - 86.0f};
	const Color Red = Hex(0xc8202f);
	C.FillRoundRect(Card, 18.0f, Paint(Open ? Hex(0xf7f3ea) : Red));
	C.StrokeRoundRect({Card.X + 10.0f, Card.Y + 10.0f, Card.W - 20.0f, Card.H - 20.0f}, 12.0f, Open ? Red : Hex(0xf7f3ea), 4.0f);
	if (Open && !(V && V->MembersAnyHour && !Staffed))
	{
		C.Text("COME IN, WE'RE", W * 0.5f, Card.Y + 70.0f, Ts(36.0f, 800, Hex(0x1f2a44), Align::Center));
		C.Text("OPEN", W * 0.5f, Card.Y + 170.0f, Ts(104.0f, 900, Red, Align::Center));
		return;
	}
	if (Open)
	{
		// The gym between shifts: open to the fob, nobody at the desk.
		C.Text("MEMBERS ONLY", W * 0.5f, Card.Y + 90.0f, Ts(48.0f, 900, Red, Align::Center));
		C.Text("TAP YOUR FOB", W * 0.5f, Card.Y + 146.0f, Ts(32.0f, 800, Hex(0x1f2a44), Align::Center));
		if (V)
		{
			C.Text("DESK OPENS " + Up(net::TimeLabel(NextStaffed(*V, World))), W * 0.5f, Card.Y + 196.0f, Ts(24.0f, 700, Hex(0x1f2a44), Align::Center));
		}
		return;
	}
	C.Text("SORRY, WE'RE", W * 0.5f, Card.Y + 64.0f, Ts(34.0f, 800, Hex(0xf7f3ea), Align::Center));
	C.Text("CLOSED", W * 0.5f, Card.Y + 150.0f, Ts(92.0f, 900, Hex(0xf7f3ea), Align::Center));
	if (V)
	{
		FitText(C, Up(shop::HoursLine(*V, World)), W * 0.5f, Card.Y + 200.0f, 26.0f, 700, Hex(0xf7f3ea), Align::Center, Card.W - 60.0f);
	}
}

void PriceBoard(Canvas& C)
{
	const float W = PriceBoardW;
	const float H = PriceBoardH;
	// A painted board in a dark frame: cream, the barbers' red, gold leaf on the name.
	C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0x2a1c14)));
	const Rect B{24.0f, 24.0f, W - 48.0f, H - 48.0f};
	C.FillRect(B, Paint::Linear({B.X, B.Y}, {B.X, B.Y + B.H}, Hex(0xf2ead8), Hex(0xe3d7bd)));
	C.FillRect({B.X, B.Y, B.W, 150.0f}, Paint(Hex(0x8e1b22)));
	C.Text("FIFTH ST. BARBERS", W * 0.5f, B.Y + 92.0f, Ts(66.0f, 900, Hex(0xe8c56a), Align::Center));
	C.Text("EST. 1995 \xC2\xB7 WALK-INS WELCOME", W * 0.5f, B.Y + 132.0f, Ts(26.0f, 700, Hex(0xf2ead8), Align::Center));
	const Color Ink = Hex(0x231c17);
	const Color Red = Hex(0x8e1b22);
	const char* const Tabs[] = {"CUT", "BEARD", "COLOR", "BROWS"};
	const char* const Heads[] = {"CUTS", "BEARDS & SHAVES", "COLOR", "BROWS"};
	std::vector<const shop::Offer*> Rows[4];
	for (const shop::Offer& O : shop::Offers("barber"))
	{
		for (int T = 0; T < 4; ++T)
		{
			if (O.Tab == Tabs[T])
			{
				Rows[T].push_back(&O);
			}
		}
	}
	// Everything the chair sells goes up, so the board's lettering tightens to fit between the header and the footer.
	float Needed = 0.0f;
	for (int T = 0; T < 4; ++T)
	{
		if (!Rows[T].empty())
		{
			Needed += 54.0f + (T == 2 ? 44.0f : 42.0f * Fl(static_cast<int>(Rows[T].size()))) + 26.0f;
		}
	}
	const float Room = B.H - 214.0f - 110.0f - 20.0f;
	const float K = Needed > Room ? std::max(0.7f, Room / Needed) : 1.0f;
	float Y = B.Y + 174.0f + 40.0f * K;
	for (int T = 0; T < 4; ++T)
	{
		if (Rows[T].empty())
		{
			continue;
		}
		C.Text(Heads[T], B.X + 60.0f, Y, Ts(34.0f * K, 900, Red));
		C.FillRect({B.X + 60.0f, Y + 12.0f * K, B.W - 120.0f, 3.0f}, Paint(Alpha(Red, 0.6f)));
		Y += 54.0f * K;
		// The color tab's eight colors are one price: one line for them.
		if (T == 2 && Rows[T].size() > 1)
		{
			C.Text("Any color, grey included", B.X + 60.0f, Y, Ts(30.0f * K, 600, Ink));
			C.Text(Dollars(Rows[T].front()->PriceCents), B.X + B.W - 60.0f, Y, Ts(32.0f * K, 900, Ink, Align::Right));
			Y += 44.0f * K + 26.0f * K;
			continue;
		}
		for (const shop::Offer* O : Rows[T])
		{
			const std::string P = Dollars(O->PriceCents);
			const float Pw = C.Measure(P, 32.0f * K, 900);
			const float Nw = C.Text(O->Name, B.X + 60.0f, Y, Ts(30.0f * K, 600, Ink, Align::Left, Baseline::Alphabetic, false, B.W - 200.0f - Pw));
			for (float Dx = B.X + 60.0f + Nw + 16.0f; Dx < B.X + B.W - 76.0f - Pw; Dx += 14.0f)
			{
				C.FillCircle(Dx, Y - 8.0f * K, 2.0f, Paint(Alpha(Ink, 0.35f)));
			}
			C.Text(P, B.X + B.W - 60.0f, Y, Ts(32.0f * K, 900, Ink, Align::Right));
			Y += 42.0f * K;
		}
		Y += 26.0f * K;
	}
	C.FillRect({B.X, B.Y + B.H - 110.0f, B.W, 110.0f}, Paint(Hex(0x8e1b22)));
	C.Text("CUT & BEARD TOGETHER: $5 OFF", W * 0.5f, B.Y + B.H - 60.0f, Ts(34.0f, 900, Hex(0xf2ead8), Align::Center));
	C.Text("CLOSED MONDAYS \xC2\xB7 " + PhoneOf("barber"), W * 0.5f, B.Y + B.H - 22.0f, Ts(22.0f, 700, Hex(0xe8c56a), Align::Center));
}

void LetterBoard(Canvas& C)
{
	const float W = LetterBoardW;
	const float H = LetterBoardH;
	// Black felt in grooves, an aluminum frame.
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {W, 0.0f}, Hex(0xb8bcc0), Hex(0x7d8287)));
	const Rect B{18.0f, 18.0f, W - 36.0f, H - 36.0f};
	C.FillRect(B, Paint(Hex(0x141416)));
	for (float Gy = B.Y + 12.0f; Gy < B.Y + B.H; Gy += 12.0f)
	{
		C.FillRect({B.X, Gy, B.W, 2.0f}, Paint(Rgba(0, 0, 0, 0.6f)));
		C.FillRect({B.X, Gy + 2.0f, B.W, 1.0f}, Paint(Rgba(255, 255, 255, 0.04f)));
	}
	Letters(C, "RUBY'S", W * 0.5f, B.Y + 90.0f, 80.0f, Align::Center, B.W - 80.0f, Hex(0xff5a6a));
	Letters(C, "BREAKFAST ALL DAY \xC2\xB7 OPEN 24 HOURS", W * 0.5f, B.Y + 146.0f, 34.0f, Align::Center, B.W - 80.0f);
	std::vector<const shop::Offer*> Menu;
	for (const shop::Offer& O : shop::Offers("diner"))
	{
		if (O.Type == shop::OfferKind::Meal || O.Type == shop::OfferKind::Drink)
		{
			Menu.push_back(&O);
		}
	}
	const size_t Half = (Menu.size() + 1) / 2;
	const float ColW = (B.W - 180.0f) * 0.5f;
	for (size_t I = 0; I < Menu.size(); ++I)
	{
		const int Col = I < Half ? 0 : 1;
		const float X = B.X + 60.0f + Fl(Col) * (ColW + 60.0f);
		const float Y = B.Y + 230.0f + Fl(static_cast<int>(I < Half ? I : I - Half)) * 62.0f;
		if (Y > B.Y + B.H - 30.0f)
		{
			continue;
		}
		const std::string P = Money(Menu[I]->PriceCents);
		const float Pw = C.Measure(P, 38.0f, 700);
		Letters(C, Up(Menu[I]->Name), X, Y, 38.0f, Align::Left, ColW - Pw - 40.0f);
		Letters(C, P, X + ColW, Y, 38.0f, Align::Right, Pw + 4.0f);
	}
	if (Menu.empty())
	{
		Letters(C, "COFFEE \xC2\xB7 PIE \xC2\xB7 BREAKFAST ALL DAY", W * 0.5f, B.Y + 330.0f, 48.0f, Align::Center, B.W - 80.0f);
	}
	// One letter's always a little crooked.
	C.Save();
	C.Translate(B.X + B.W - 70.0f, B.Y + 92.0f);
	C.Rotate(0.18f);
	C.Text("*", 0.0f, 0.0f, Ts(40.0f, 700, Hex(0xf4f1ea), Align::Center));
	C.Restore();
}

void RackTag(Canvas& C, const std::string& Rack, const Session& S, uint64_t GarmentKinds)
{
	const float W = RackTagW;
	const float H = RackTagH;
	const char* Name = Rack == "outer" ? "JACKETS" : Rack == "pants" ? "PANTS" : Rack == "shoes" ? "SHOES" : Rack == "hats" ? "HATS & GLASSES" : "TEES & TOPS";
	// A hand-painted sign on hooks: St. Agnes green on cream.
	C.FillRoundRect({0.0f, 0.0f, W, H}, 18.0f, Paint(Hex(0xf3ecd9)));
	C.StrokeRoundRect({10.0f, 10.0f, W - 20.0f, H - 20.0f}, 12.0f, Hex(0x2f6b3d), 6.0f);
	FitText(C, Name, W * 0.5f, 140.0f, 110.0f, 900, Hex(0x2f6b3d), Align::Center, W - 80.0f);
	// What hangs here this week: the range on its tags.
	const double World = S.WorldMinutes();
	Chips Lo = 0;
	Chips Hi = 0;
	bool Yellow = false;
	for (const std::string& Id : ThriftRack(S, GarmentKinds))
	{
		const wardrobe::Garment* G = wardrobe::Find(Id);
		if (!G || Rack != RackOf(*G))
		{
			continue;
		}
		const Chips P = wardrobe::ThriftPrice(*G, World);
		Lo = Lo == 0 ? P : std::min(Lo, P);
		Hi = std::max(Hi, P);
		Yellow = Yellow || YellowTag(*G, World);
	}
	std::string Under;
	if (Hi > 0)
	{
		Under = Lo == Hi ? Dollars(Lo) : Dollars(Lo) + " \xE2\x80\x93 " + Dollars(Hi);
		Under += Yellow ? " \xC2\xB7 YELLOW TAGS \xC2\xBD OFF SATURDAY" : " \xC2\xB7 NEW RACKS THURSDAY";
	}
	else
	{
		Under = "NEW RACKS THURSDAY";
	}
	FitText(C, Under, W * 0.5f, 226.0f, 46.0f, 800, Hex(0x3a332a), Align::Center, W - 80.0f);
}

void PriceTag(Canvas& C, const std::string& Name, Chips PriceCents, bool Auction)
{
	const float W = PriceTagW;
	const float H = PriceTagH;
	// A manila string tag: the reinforced hole, the string, the price in marker.
	const std::vector<Vec2> Shape = {{W * 0.28f, 0.0f}, {W * 0.72f, 0.0f}, {W, W * 0.28f}, {W, H}, {0.0f, H}, {0.0f, W * 0.28f}};
	C.FillPolygon(Shape, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(0xe9d9b0), Hex(0xd8c494)));
	C.FillCircle(W * 0.5f, W * 0.2f, 20.0f, Paint(Hex(0xc9b27a)));
	C.FillCircle(W * 0.5f, W * 0.2f, 10.0f, Paint(Hex(0x0b0e14)));
	C.Text(Dollars(PriceCents), W * 0.5f, H * 0.48f, Ts(Fit(C, Dollars(PriceCents), 64.0f, 900, W - 24.0f), 900, Hex(0x1b1d22), Align::Center));
	const std::vector<std::string> Words = Split(Name, " ");
	std::string Row;
	float Y = H * 0.62f;
	for (const std::string& Wd : Words)
	{
		const std::string Trial = Row.empty() ? Wd : Row + " " + Wd;
		if (!Row.empty() && C.Measure(Trial, 24.0f, 600) > W - 30.0f)
		{
			C.Text(Row, W * 0.5f, Y, Ts(24.0f, 600, Hex(0x3a332a), Align::Center));
			Y += 28.0f;
			Row = Wd;
		}
		else
		{
			Row = Trial;
		}
	}
	if (!Row.empty() && Y < H - (Auction ? 80.0f : 20.0f))
	{
		C.Text(Row, W * 0.5f, Y, Ts(Fit(C, Row, 24.0f, 600, W - 30.0f), 600, Hex(0x3a332a), Align::Center));
	}
	if (Auction)
	{
		C.Save();
		C.Translate(W * 0.5f, H - 48.0f);
		C.Rotate(-0.12f);
		C.StrokeRoundRect({-W * 0.44f, -26.0f, W * 0.88f, 48.0f}, 6.0f, Hex(0xb01e3c), 3.0f);
		C.Text("LOT 114", 0.0f, -4.0f, Ts(20.0f, 900, Hex(0xb01e3c), Align::Center));
		C.Text("NINTH ST.", 0.0f, 16.0f, Ts(16.0f, 800, Hex(0xb01e3c), Align::Center));
		C.Restore();
	}
}

void PawnTicket(Canvas& C, const shop::Ticket& T, const std::string& ItemName, double World)
{
	const float W = PawnTicketW;
	const float H = PawnTicketH;
	const Color Pink = Hex(0xf5c8d3);
	const Color Ink = Hex(0x4a1c2a);
	const Color Print = Hex(0x7a2a42);
	const Color Red = Hex(0xb01e3c);
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Pink, Hex(0xe9b3c1)));
	// The printed form: the shop, the number in red, the boxes.
	C.Text("NORTHSIDE PAWN & LOAN", W * 0.5f, 62.0f, Ts(36.0f, 900, Print, Align::Center));
	C.Text("232 MARKET ST. \xC2\xB7 " + PhoneOf("pawn") + " \xC2\xB7 LICENSED PAWNBROKER", W * 0.5f, 94.0f, Ts(17.0f, 700, Print, Align::Center));
	char No[16];
	std::snprintf(No, sizeof(No), "No. %04d", T.Number);
	C.Text("PAWN TICKET", 40.0f, 150.0f, Ts(26.0f, 900, Print));
	C.Text(No, W - 40.0f, 150.0f, Ts(32.0f, 900, Red, Align::Right, Baseline::Alphabetic, true));
	C.FillRect({30.0f, 168.0f, W - 60.0f, 3.0f}, Paint(Print));
	auto Field = [&](const char* Label, const std::string& Value, float Y, bool Big) {
		C.Text(Label, 40.0f, Y, Ts(15.0f, 800, Alpha(Print, 0.85f)));
		C.FillRect({40.0f, Y + 46.0f, W - 80.0f, 1.5f}, Paint(Alpha(Print, 0.5f)));
		C.Text(Value, 48.0f, Y + 38.0f, Ts(Fit(C, Value, Big ? 30.0f : 26.0f, 500, W - 100.0f), 500, Ink, Align::Left, Baseline::Alphabetic, true));
	};
	Field("ARTICLE PLEDGED", Up(ItemName), 210.0f, true);
	Field("AMOUNT LOANED", Money(T.LoanCents), 300.0f, false);
	Field("DATE TAKEN", Up(std::string(net::WeekdayName(net::DayOf(T.TakenAt))) + " " + net::DateLabel(net::DayOf(T.TakenAt))), 390.0f, false);
	Field("REDEEM BY", Up(std::string(net::WeekdayName(net::DayOf(T.DueAt))) + " " + net::DateLabel(net::DayOf(T.DueAt))), 480.0f, false);
	Field("TO REDEEM BY THAT DATE", Money(T.LoanCents + shop::RenewCents(T)), 570.0f, false);
	const Chips Today = shop::RedeemCents(T, World);
	C.Text("TODAY", 40.0f, 676.0f, Ts(15.0f, 800, Alpha(Print, 0.85f)));
	C.Text(Today > 0 ? Money(Today) : std::string("FORFEITED"), W - 40.0f, 676.0f, Ts(30.0f, 900, Today > 0 && World <= T.DueAt ? Ink : Red, Align::Right, Baseline::Alphabetic, true));
	// The terms in the small print, and the carbon's smudge.
	C.Text("Loan + 20% within 30 days; + 25% in the 7-day grace after.", W * 0.5f, 740.0f, Ts(16.0f, 500, Print, Align::Center));
	C.Text("Not redeemed by the grace's end, the article is forfeited.", W * 0.5f, 764.0f, Ts(16.0f, 500, Print, Align::Center));
	C.Text("TICKET REQUIRED TO REDEEM \xC2\xB7 NO ID NO LOAN", W * 0.5f, 800.0f, Ts(17.0f, 800, Print, Align::Center));
	for (float Px = 20.0f; Px < W - 20.0f; Px += 16.0f)
	{
		C.FillRect({Px, 838.0f, 8.0f, 2.0f}, Paint(Alpha(Print, 0.5f)));
	}
	C.Text("PLEDGOR'S COPY", W * 0.5f, 896.0f, Ts(20.0f, 900, Alpha(Print, 0.7f), Align::Center));
	C.FillEllipse(W * 0.7f, 300.0f, 160.0f, 40.0f, Paint::Radial({W * 0.7f, 300.0f}, 0.0f, {W * 0.7f, 300.0f}, 160.0f, Rgba(70, 30, 60, 0.08f), 0.5f, Rgba(70, 30, 60, 0.03f), Rgba(70, 30, 60, 0.0f)));
}

void PawnCard(Canvas& C, int Which, double World)
{
	const float W = PawnCardW;
	const float H = PawnCardH;
	if (Which == 1)
	{
		// The door's sticker: yellow and black, the buzzer.
		C.FillRoundRect({0.0f, 0.0f, W, H}, 24.0f, Paint(Hex(0xffd23f)));
		C.StrokeRoundRect({14.0f, 14.0f, W - 28.0f, H - 28.0f}, 16.0f, Hex(0x15161a), 8.0f);
		C.FillCircle(W * 0.5f, 130.0f, 54.0f, Paint(Hex(0x15161a)));
		C.FillCircle(W * 0.5f, 130.0f, 30.0f, Paint(Hex(0xc8202f)));
		C.Text("PLEASE WAIT", W * 0.5f, 290.0f, Ts(86.0f, 900, Hex(0x15161a), Align::Center));
		C.Text("FOR THE BUZZER", W * 0.5f, 390.0f, Ts(70.0f, 900, Hex(0x15161a), Align::Center));
		C.Text("THE DOOR WILL CLICK", W * 0.5f, 460.0f, Ts(36.0f, 700, Hex(0x15161a), Align::Center));
		return;
	}
	if (Which == 2)
	{
		// The window's card: what he buys, and the hours.
		C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0x101a3a)));
		C.StrokeRoundRect({16.0f, 16.0f, W - 32.0f, H - 32.0f}, 10.0f, Hex(0xe8c56a), 5.0f);
		C.Text("WE BUY GOLD", W * 0.5f, 130.0f, Ts(96.0f, 900, Hex(0xe8c56a), Align::Center));
		C.Text("LOANS \xC2\xB7 GUITARS \xC2\xB7 ELECTRONICS", W * 0.5f, 210.0f, Ts(44.0f, 800, Hex(0xf4f1ea), Align::Center));
		const shop::Venue* V = shop::FindVenue("pawn");
		float Y = 300.0f;
		const std::vector<std::string> Lines = V ? HoursLines(*V) : std::vector<std::string>();
		for (const std::string& L : Lines)
		{
			FitText(C, L, W * 0.5f, Y, 36.0f, 700, Hex(0xf4f1ea), Align::Center, W - 80.0f);
			Y += 46.0f;
		}
		if (V)
		{
			FitText(C, Up(shop::HoursLine(*V, World)), W * 0.5f, H - 46.0f, 30.0f, 800, Hex(0xe8c56a), Align::Center, W - 80.0f);
		}
		return;
	}
	// The house rules over the counter: white on red, in a frame.
	C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0x15161a)));
	C.FillRect({14.0f, 14.0f, W - 28.0f, H - 28.0f}, Paint(Hex(0xb31d2b)));
	const char* const Rules[] = {"ALL SALES FINAL", "NO ID NO LOAN", "30-DAY LOANS", "TICKET REQUIRED TO REDEEM"};
	for (int K = 0; K < 4; ++K)
	{
		FitText(C, Rules[K], W * 0.5f, 120.0f + Fl(K) * 110.0f, 64.0f, 900, Hex(0xf4f1ea), Align::Center, W - 80.0f);
	}
}

void Whiteboard(Canvas& C, double World)
{
	const float W = WhiteboardW;
	const float H = WhiteboardH;
	C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0x9aa0a6)));
	C.FillRect({16.0f, 16.0f, W - 32.0f, H - 52.0f}, Paint::Linear({0.0f, 0.0f}, {W, H}, Hex(0xfbfbf8), Hex(0xe9ebe8)));
	C.FillRect({16.0f, H - 36.0f, W - 32.0f, 20.0f}, Paint(Hex(0x7d8287))); // the marker tray
	const Color Black = Hex(0x1b1d22);
	const Color Blue = Hex(0x1f4fa8);
	const Color Red = Hex(0xc0262f);
	const int Day = net::DayOf(World);
	C.Text("IRONHOUSE \xC2\xB7 WORKOUT OF THE DAY", 60.0f, 96.0f, Ts(54.0f, 900, Red));
	C.Text(Up(std::string(net::WeekdayName(Day, true)) + " " + net::DateLabel(Day)), W - 60.0f, 96.0f, Ts(36.0f, 700, Black, Align::Right));
	C.FillRect({60.0f, 118.0f, W - 120.0f, 4.0f}, Paint(Alpha(Red, 0.7f)));
	// The day's workout: Rosa writes a new one every morning (the same all day).
	struct Move
	{
		const char* Text;
	};
	static const char* const Moves[] = {"ROW 500 M", "20 KETTLEBELL SWINGS", "15 PUSH-UPS", "10 BOX JUMPS", "400 M ON THE TREADMILL", "3 MIN ON THE BAG", "12 GOBLET SQUATS",
		"10 PULL-UPS (OR RINGS)", "30 SEC PLANK", "15 DUMBBELL ROWS EACH", "20 WALL BALLS", "1 MIN JUMP ROPE"};
	uint64_t H0 = Fnv("wod" + std::to_string(Day));
	const int Rounds = 3 + static_cast<int>(H0 % 3);
	C.Text(std::to_string(Rounds) + " ROUNDS", 60.0f, 200.0f, Ts(50.0f, 900, Blue));
	std::vector<int> Picked;
	float Y = 270.0f;
	for (int K = 0; K < 5; ++K)
	{
		H0 = H0 * 6364136223846793005ull + 1442695040888963407ull;
		int M = static_cast<int>((H0 >> 33) % 12);
		while (std::find(Picked.begin(), Picked.end(), M) != Picked.end())
		{
			M = (M + 1) % 12;
		}
		Picked.push_back(M);
		C.Text("\xE2\x80\x94 " + std::string(Moves[M]), 80.0f, Y, Ts(44.0f, 700, Black));
		Y += 64.0f;
	}
	C.Text("REST 90 SEC BETWEEN ROUNDS", 80.0f, Y + 16.0f, Ts(36.0f, 700, Red));
	// The desk's hours and the day pass, from the board's own numbers.
	const shop::Venue* V = shop::FindVenue("gym");
	const shop::Offer* Pass = OfferLike("gym", "day");
	std::string Desk = "MEMBERS 24/7";
	if (Pass)
	{
		Desk += " \xC2\xB7 DAY PASS " + Dollars(Pass->PriceCents);
	}
	C.Text(Desk, W - 60.0f, H - 120.0f, Ts(34.0f, 800, Blue, Align::Right));
	if (V)
	{
		const std::vector<std::string> L = HoursLines(*V);
		std::string Line;
		for (const std::string& S : L)
		{
			// The members' hours are the line above; this one is the desk's.
			if (S.compare(0, 7, "MEMBERS") != 0)
			{
				Line += (Line.empty() ? "" : " \xC2\xB7 ") + S;
			}
		}
		if (!Line.empty())
		{
			FitText(C, "DESK " + Line, W - 60.0f, H - 74.0f, 30.0f, 700, Black, Align::Right, W - 120.0f);
		}
	}
}

void BackSign(Canvas& C, double World)
{
	const float W = BackSignW;
	const float H = BackSignH;
	const shop::Venue* V = shop::FindVenue("gym");
	const double Back = V ? NextStaffed(*V, World) : World;
	const double InDay = Back - std::floor(Back / 1440.0) * 1440.0;
	const int Hr = static_cast<int>(InDay / 60.0);
	const std::string When = std::to_string(Hr % 12 == 0 ? 12 : Hr % 12) + (Hr < 12 ? " AM" : " PM");
	// A folded tent card, Rosa's marker.
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(0xfbfaf6), Hex(0xe6e2d8)));
	C.FillRect({0.0f, 0.0f, W, 14.0f}, Paint(Hex(0xf28a3a)));
	C.Text("BACK AT", W * 0.5f, 140.0f, Ts(74.0f, 900, Hex(0x1b1d22), Align::Center));
	C.Text(When, W * 0.5f, 270.0f, Ts(120.0f, 900, Hex(0xc0262f), Align::Center));
	C.Text("Fob works. Lights work. \xE2\x80\x94 Rosa", W * 0.5f, 360.0f, Ts(30.0f, 600, Hex(0x3a3d44), Align::Center));
}

void SandwichBoard(Canvas& C, const std::string& Which, double World)
{
	const float W = SandwichW;
	const float H = SandwichH;
	Slate(C, {0.0f, 0.0f, W, H}, 30.0f);
	const Color White = Hex(0xf4f1ea);
	const Color Pink = Hex(0xff9ab0);
	const Color Yellow = Hex(0xffe08a);
	const Color Blue = Hex(0x9fd3ff);
	const Color Green = Hex(0xa6e3a1);
	const float Cw = W - 100.0f;
	const float Cx = W * 0.5f;
	if (Which == "diner")
	{
		Chalk(C, "RUBY'S", Cx, 140.0f, 110.0f, Pink, Align::Center, Cw, 900);
		Chalk(C, "BREAKFAST ALL DAY", Cx, 220.0f, 44.0f, White, Align::Center, Cw);
		float Y = 330.0f;
		for (const char* Needle : {"short-stack", "patty", "pie", "coffee"})
		{
			if (const shop::Offer* O = OfferLike("diner", Needle))
			{
				Chalk(C, Up(O->Name), 70.0f, Y, 36.0f, White, Align::Left, Cw - 150.0f);
				Chalk(C, Money(O->PriceCents), W - 70.0f, Y, 38.0f, Yellow, Align::Right, 160.0f);
				Y += 74.0f;
			}
		}
		Chalk(C, "PIE: CHERRY. ALWAYS.", Cx, H - 170.0f, 40.0f, Pink, Align::Center, Cw);
		Chalk(C, "OPEN 24 HOURS", Cx, H - 90.0f, 50.0f, Blue, Align::Center, Cw, 900);
		return;
	}
	if (Which == "thrift")
	{
		Chalk(C, "SECOND", Cx, 130.0f, 84.0f, Green, Align::Center, Cw, 900);
		Chalk(C, "CHANCE", Cx, 220.0f, 84.0f, Green, Align::Center, Cw, 900);
		Chalk(C, "THRIFT", Cx, 300.0f, 60.0f, White, Align::Center, Cw);
		Chalk(C, "NEW RACKS THURSDAY", Cx, 420.0f, 44.0f, White, Align::Center, Cw);
		Chalk(C, "YELLOW TAGS", Cx, 520.0f, 52.0f, Yellow, Align::Center, Cw, 900);
		Chalk(C, "\xC2\xBD OFF SATURDAY", Cx, 584.0f, 46.0f, Yellow, Align::Center, Cw);
		Chalk(C, "DONATIONS WELCOME", Cx, H - 170.0f, 40.0f, White, Align::Center, Cw);
		Chalk(C, "St. Agnes Outreach", Cx, H - 100.0f, 36.0f, Blue, Align::Center, Cw, 500);
		return;
	}
	if (Which == "pawn")
	{
		Chalk(C, "WE BUY", Cx, 150.0f, 90.0f, White, Align::Center, Cw, 900);
		Chalk(C, "GOLD", Cx, 270.0f, 130.0f, Yellow, Align::Center, Cw, 900);
		Chalk(C, "TOP $ FOR", Cx, 400.0f, 60.0f, White, Align::Center, Cw);
		Chalk(C, "GUITARS", Cx, 480.0f, 76.0f, Pink, Align::Center, Cw, 900);
		Chalk(C, "LOANS \xC2\xB7 30 DAYS", Cx, 600.0f, 46.0f, Blue, Align::Center, Cw);
		if (const shop::Venue* V = shop::FindVenue("pawn"))
		{
			Chalk(C, Up(shop::HoursLine(*V, World)), Cx, H - 100.0f, 36.0f, White, Align::Center, Cw);
		}
		return;
	}
	if (Which == "ginos")
	{
		Chalk(C, "GINO'S", Cx, 150.0f, 120.0f, Hex(0xff8a65), Align::Center, Cw, 900);
		Chalk(C, "SLICE", Cx, 240.0f, 70.0f, White, Align::Center, Cw);
		Chalk(C, "BY THE SLICE", Cx, 380.0f, 50.0f, Yellow, Align::Center, Cw);
		Chalk(C, "PLAIN \xC2\xB7 PEPPERONI", Cx, 460.0f, 44.0f, White, Align::Center, Cw);
		Chalk(C, "GARLIC KNOTS", Cx, 530.0f, 44.0f, White, Align::Center, Cw);
		const shop::Venue* V = shop::FindVenue("ginos");
		Chalk(C, V ? Up(shop::HoursLine(*V, World)) : std::string("OPEN TIL 2 AM"), Cx, H - 100.0f, 44.0f, Blue, Align::Center, Cw, 900);
		return;
	}
	// Kim's produce: what's in the crates by the door.
	Chalk(C, "KIM'S", Cx, 150.0f, 120.0f, Green, Align::Center, Cw, 900);
	Chalk(C, "PRODUCE", Cx, 240.0f, 70.0f, White, Align::Center, Cw);
	Chalk(C, "FRESH TODAY", Cx, 370.0f, 50.0f, Yellow, Align::Center, Cw);
	Chalk(C, "APPLES \xC2\xB7 PEARS", Cx, 450.0f, 46.0f, White, Align::Center, Cw);
	Chalk(C, "SQUASH \xC2\xB7 GREENS", Cx, 520.0f, 46.0f, White, Align::Center, Cw);
	Chalk(C, "FLOWERS", Cx, 590.0f, 46.0f, Pink, Align::Center, Cw);
	const shop::Venue* V = shop::FindVenue("kims");
	Chalk(C, V ? Up(shop::HoursLine(*V, World)) : std::string("OPEN 7 AM \xE2\x80\x93 9 PM"), Cx, H - 100.0f, 40.0f, Blue, Align::Center, Cw, 900);
}

void Timetable(Canvas& C, const std::string& Stop, double World)
{
	const float W = TimetableW;
	const float H = TimetableH;
	const bool East = Stop != "market-west";
	const Color Navy = Hex(0x13294b);
	const Color Gold = Hex(0xf2c14e);
	C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0xf6f5f0)));
	// The livery band: CITY TRANSIT and the route.
	C.FillRect({0.0f, 0.0f, W, 170.0f}, Paint(Navy));
	C.FillCircle(92.0f, 85.0f, 58.0f, Paint(Gold));
	C.Text("14", 92.0f, 108.0f, Ts(64.0f, 900, Navy, Align::Center));
	C.Text("CITY TRANSIT", 170.0f, 72.0f, Ts(34.0f, 900, Hex(0xffffff)));
	FitText(C, East ? "EMBERCREST via MARKET" : "DOWNTOWN via FIFTH", 170.0f, 118.0f, 30.0f, 700, Gold, Align::Left, W - 190.0f);
	C.Text(East ? "MARKET ST & FIFTH \xC2\xB7 EASTBOUND" : "MARKET ST & FIFTH \xC2\xB7 WESTBOUND", 30.0f, 214.0f, Ts(22.0f, 800, Navy));
	// The next buses (the town's clock), then the posted schedule.
	C.FillRect({30.0f, 236.0f, W - 60.0f, 3.0f}, Paint(Navy));
	C.Text("NEXT", 30.0f, 290.0f, Ts(26.0f, 900, Navy));
	const std::vector<double> Next = town::BusTimes(East ? "market-east" : "market-west", World, 3);
	float Nx = 130.0f;
	for (size_t I = 0; I < Next.size(); ++I)
	{
		const std::string T = net::TimeLabel(Next[I]);
		Nx += C.Text(T, Nx, 290.0f, Ts(30.0f, I == 0 ? 900 : 600, I == 0 ? Hex(0xc0262f) : Navy)) + 28.0f;
	}
	if (Next.empty())
	{
		C.Text("See the schedule below", 130.0f, 290.0f, Ts(24.0f, 600, Navy));
	}
	C.FillRect({30.0f, 316.0f, W - 60.0f, 2.0f}, Paint(Alpha(Navy, 0.4f)));
	C.Text(East ? "DEPARTS (EVERY DAY)" : "ARRIVES (EVERY DAY)", 30.0f, 360.0f, Ts(22.0f, 900, Navy));
	// The hours down the left, the minutes past each in a row: :05 and :35 by day, :35 overnight (eastbound);
	// :20 and :50 by day, :50 overnight (westbound).
	// Each column's first hour says AM or PM, as a printed timetable does.
	const char* const Hours[] = {"5 AM", "6", "7", "8", "9", "10", "11", "NOON", "1 PM", "2", "3", "4", "5 PM", "6", "7", "8", "9", "10", "11", "MIDNIGHT", "1 AM", "2", "3", "4"};
	for (int I = 0; I < 24; ++I)
	{
		const int Hr = (I + 5) % 24;
		const bool Night = Hr < 6;
		const int Col = I / 12;
		const float X = 40.0f + Fl(Col) * (W - 80.0f) * 0.5f;
		const float Y = 400.0f + Fl(I % 12) * 46.0f;
		if (I % 2 == 0)
		{
			C.FillRect({X - 10.0f, Y - 30.0f, (W - 80.0f) * 0.5f - 10.0f, 46.0f}, Paint(Alpha(Navy, 0.05f)));
		}
		C.Text(Hours[I], X, Y, Ts(22.0f, 800, Navy));
		const std::string Mins = East ? (Night ? std::string(":35") : std::string(":05   :35")) : (Night ? std::string(":50") : std::string(":20   :50"));
		C.Text(Mins, X + 150.0f, Y, Ts(22.0f, 600, Hex(0x2b2723), Align::Left, Baseline::Alphabetic, true));
	}
	C.Text("Schedules may change. Hail the driver at night.", W * 0.5f, H - 44.0f, Ts(18.0f, 600, Alpha(Navy, 0.75f), Align::Center));
}

void ShelterAd(Canvas& C)
{
	const float W = ShelterAdW;
	const float H = ShelterAdH;
	// RiverLine's night campaign: the river at night as a line of light, a phone, the line.
	C.FillRect({0.0f, 0.0f, W, H}, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(0x07101e), Hex(0x0e1d36)));
	for (int K = 0; K < 3; ++K)
	{
		std::vector<Vec2> River;
		for (int I = 0; I <= 40; ++I)
		{
			const float X = W * Fl(I) / 40.0f;
			River.push_back({X, H * 0.62f + Fl(K) * 22.0f + std::sin(Fl(I) * 0.45f + Fl(K)) * 30.0f});
		}
		C.StrokePolyline(River, false, K == 0 ? Hex(0x27d3c3) : Rgba(39, 211, 195, 0.35f - 0.1f * Fl(K)), K == 0 ? 8.0f : 4.0f, true);
	}
	// The phone, lit, a hand of cards on its screen.
	const Rect Phone{W * 0.5f - 150.0f, H * 0.18f, 300.0f, 560.0f};
	C.GlowRoundRect(Phone, 36.0f, Rgba(39, 211, 195, 0.35f), 60.0f);
	C.FillRoundRect(Phone, 36.0f, Paint(Hex(0x0a0d14)));
	C.FillRoundRect({Phone.X + 14.0f, Phone.Y + 14.0f, Phone.W - 28.0f, Phone.H - 28.0f}, 26.0f, Paint::Linear({0.0f, Phone.Y}, {0.0f, Phone.Y + Phone.H}, Hex(0x0f6a4e), Hex(0x083a2b)));
	DrawCard(C, 51, Phone.X + 44.0f, Phone.Y + 170.0f, 100.0f, 140.0f);
	DrawCard(C, 38, Phone.X + 156.0f, Phone.Y + 170.0f, 100.0f, 140.0f);
	C.Text("RiverLine", W * 0.5f, H * 0.76f + 40.0f, Ts(110.0f, 900, Hex(0xffffff), Align::Center));
	C.Text("the night shift never closes", W * 0.5f, H * 0.76f + 120.0f, Ts(46.0f, 500, Hex(0x9fe8df), Align::Center));
	C.Text("Play responsibly. 21+.", W * 0.5f, H - 40.0f, Ts(24.0f, 500, Rgba(255, 255, 255, 0.5f), Align::Center));
}

void DestinationSign(Canvas& C, const std::string& Text, bool Side)
{
	const float W = Side ? DestSignW * 0.6f : DestSignW;
	const float H = DestSignH;
	C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0x050506)));
	// Amber LEDs: the glow, the text, then the dark between the dots.
	const Color Amber = Hex(0xffb020);
	const std::vector<std::string> Parts = Split(Text, " ");
	const std::string Route = Parts.empty() ? std::string() : Parts.front();
	const std::string Dest = Text.size() > Route.size() ? Text.substr(Route.size() + 1) : std::string();
	const float Rw = C.Measure(Route, 150.0f, 900);
	for (int K = 0; K < 8; ++K)
	{
		const float A = Fl(K) * 0.785398f;
		C.Text(Route, 40.0f + std::cos(A) * 5.0f, H * 0.5f + 52.0f + std::sin(A) * 5.0f, Ts(150.0f, 900, Rgba(255, 150, 20, 0.1f)));
	}
	C.Text(Route, 40.0f, H * 0.5f + 52.0f, Ts(150.0f, 900, Amber));
	const float Dx = 40.0f + Rw + 50.0f;
	FitText(C, Dest, Dx, H * 0.5f + 40.0f, 110.0f, 700, Amber, Align::Left, W - Dx - 30.0f);
	for (float X = 0.0f; X < W; X += 8.0f)
	{
		C.FillRect({X, 0.0f, 2.0f, H}, Paint(Rgba(5, 5, 6, 0.85f)));
	}
	for (float Y = 0.0f; Y < H; Y += 8.0f)
	{
		C.FillRect({0.0f, Y, W, 2.0f}, Paint(Rgba(5, 5, 6, 0.85f)));
	}
}

std::string PlateText(uint64_t Seed)
{
	// Three letters (no I, O or Q: they read as digits) and four digits.
	static const char Letters_[] = "ABCDEFGHJKLMNPRSTUVWXYZ";
	uint64_t Z = Seed + 0x9e3779b97f4a7c15ull;
	auto Next = [&Z]() {
		Z += 0x9e3779b97f4a7c15ull;
		uint64_t X = Z;
		X = (X ^ (X >> 30)) * 0xbf58476d1ce4e5b9ull;
		X = (X ^ (X >> 27)) * 0x94d049bb133111ebull;
		return X ^ (X >> 31);
	};
	std::string Out;
	for (int K = 0; K < 3; ++K)
	{
		Out += Letters_[Next() % 23];
	}
	Out += ' ';
	for (int K = 0; K < 4; ++K)
	{
		Out += static_cast<char>('0' + Next() % 10);
	}
	return Out;
}

void Plate(Canvas& C, const std::string& Text)
{
	const float W = PlateW;
	const float H = PlateH;
	C.FillRoundRect({0.0f, 0.0f, W, H}, 22.0f, Paint::Linear({0.0f, 0.0f}, {0.0f, H}, Hex(0xf7f7f2), Hex(0xd9dbd6)));
	C.StrokeRoundRect({8.0f, 8.0f, W - 16.0f, H - 16.0f}, 16.0f, Hex(0x1f3c88), 5.0f);
	C.FillCircle(70.0f, 34.0f, 9.0f, Paint(Hex(0x9aa0a6)));
	C.FillCircle(W - 70.0f, 34.0f, 9.0f, Paint(Hex(0x9aa0a6)));
	C.Text("THE RIVER STATE", W * 0.5f, 52.0f, Ts(34.0f, 900, Hex(0x1f3c88), Align::Center));
	FitText(C, Text, W * 0.5f, 210.0f, 150.0f, 900, Hex(0x1b2a5a), Align::Center, W - 60.0f);
	C.Text("HOLLIS COUNTY", W * 0.5f, H - 26.0f, Ts(26.0f, 800, Hex(0xc0262f), Align::Center));
}

void BankClock(Canvas& C, double Minutes)
{
	const float W = ClockW;
	const float Cx = W * 0.5f;
	const float Cy = ClockH * 0.5f;
	const float R = W * 0.46f;
	// A bronze bezel, an opal dial lit from behind, Roman numerals, the hands.
	C.FillCircle(Cx, Cy, R + 24.0f, Paint::Linear({Cx - R, Cy - R}, {Cx + R, Cy + R}, Hex(0xb08d57), Hex(0x5a4526)));
	C.FillCircle(Cx, Cy, R, Paint::Radial({Cx, Cy - R * 0.2f}, 0.0f, {Cx, Cy}, R, Hex(0xfffbef), 0.7f, Hex(0xf1e9d3), Hex(0xd9cdb0)));
	static const char* const Numerals[12] = {"XII", "I", "II", "III", "IIII", "V", "VI", "VII", "VIII", "IX", "X", "XI"};
	for (int K = 0; K < 60; ++K)
	{
		const float A = Fl(K) * 0.10471976f;
		const bool Hour = K % 5 == 0;
		C.StrokePolyline({{Cx + std::sin(A) * R * (Hour ? 0.86f : 0.9f), Cy - std::cos(A) * R * (Hour ? 0.86f : 0.9f)}, {Cx + std::sin(A) * R * 0.95f, Cy - std::cos(A) * R * 0.95f}}, false,
			Hex(0x1b1a18), Hour ? 8.0f : 3.0f);
	}
	for (int K = 0; K < 12; ++K)
	{
		const float A = Fl(K) * 0.5235988f;
		C.Text(Numerals[K], Cx + std::sin(A) * R * 0.72f, Cy - std::cos(A) * R * 0.72f, Ts(68.0f, 700, Hex(0x1b1a18), Align::Center, Baseline::Middle));
	}
	C.Text("HOLLIS SAVINGS", Cx, Cy - R * 0.3f, Ts(34.0f, 800, Hex(0x5a4526), Align::Center));
	C.Text("1911", Cx, Cy + R * 0.38f, Ts(30.0f, 700, Hex(0x5a4526), Align::Center));
	const double InDay = Minutes - std::floor(Minutes / 1440.0) * 1440.0;
	const float Ha = static_cast<float>(std::fmod(InDay / 60.0, 12.0) / 12.0 * 6.2831853);
	const float Ma = static_cast<float>(std::fmod(InDay, 60.0) / 60.0 * 6.2831853);
	// A tower clock's baton hands: widest at the hub, tapering to the tip, a short stub of a tail behind.
	auto Hand = [&](float A, float Len, float Wd, float Tail) {
		const float Sx = std::sin(A);
		const float Sy = -std::cos(A);
		const float Px = -Sy;
		const float Py = Sx;
		auto At = [&](float Along, float Across) { return Vec2{Cx + Sx * Along + Px * Across, Cy + Sy * Along + Py * Across}; };
		C.FillPolygon({At(-Tail, Wd * 0.3f), At(0.0f, Wd * 0.5f), At(Len * 0.86f, Wd * 0.2f), At(Len, 0.0f), At(Len * 0.86f, -Wd * 0.2f), At(0.0f, -Wd * 0.5f), At(-Tail, -Wd * 0.3f)},
			Paint(Hex(0x14110e)));
	};
	Hand(Ha, R * 0.52f, 26.0f, R * 0.12f);
	Hand(Ma, R * 0.82f, 18.0f, R * 0.14f);
	C.FillCircle(Cx, Cy, 20.0f, Paint(Hex(0x14110e)));
	C.FillCircle(Cx, Cy, 7.0f, Paint(Hex(0xb08d57)));
}

void Marquee(Canvas& C, double World)
{
	const float W = MarqueeW;
	const float H = MarqueeH;
	const bool Friday = net::Weekday(net::DayOf(World)) == 4;
	// Two lit letter boards in a chrome frame (the chasers are the mesh's bulbs).
	C.FillRect({0.0f, 0.0f, W, H}, Paint(Hex(0x1a1410)));
	for (int Row = 0; Row < 2; ++Row)
	{
		const Rect B{40.0f, 40.0f + Fl(Row) * (H * 0.5f - 20.0f), W - 80.0f, H * 0.5f - 60.0f};
		C.FillRect(B, Paint::Linear({0.0f, B.Y}, {0.0f, B.Y + B.H}, Hex(0xfdf6e3), Hex(0xefe3c4)));
		for (float Gy = B.Y + 40.0f; Gy < B.Y + B.H; Gy += 40.0f)
		{
			C.FillRect({B.X, Gy, B.W, 2.0f}, Paint(Rgba(120, 100, 70, 0.25f)));
		}
		const std::string Line = Row == 0 ? (Friday ? "MIDNIGHT SHOW FRI" : "CLOSED FOR RENOVATION") : (Friday ? "DOORS 11:30 PM" : "REOPENING SPRING");
		FitText(C, Line, W * 0.5f, B.Y + B.H * 0.5f + 60.0f, 170.0f, 900, Row == 0 && Friday ? Hex(0xb01e3c) : Hex(0x15161a), Align::Center, B.W - 80.0f);
	}
}

void Valance(Canvas& C, const std::string& VenueId)
{
	const float W = ValanceW;
	const float H = ValanceH;
	std::string Text = Up(NameOf(VenueId));
	if (VenueId == "barber")
	{
		Text = "FIFTH ST. BARBERS \xC2\xB7 EST. 1995";
	}
	else if (VenueId == "diner")
	{
		Text = "RUBY'S \xC2\xB7 OPEN 24 HOURS";
	}
	else if (VenueId == "thrift")
	{
		Text = "SECOND CHANCE THRIFT \xC2\xB7 ST. AGNES OUTREACH";
	}
	if (Text.empty())
	{
		return;
	}
	// White lettering printed on the valance; the scallops' shadow along the bottom.
	FitText(C, Text, W * 0.5f, H * 0.5f + 34.0f, 96.0f, 900, Hex(0xf6f4ee), Align::Center, W - 120.0f);
	for (float X = 0.0f; X < W; X += 100.0f)
	{
		C.FillEllipse(X + 50.0f, H, 50.0f, 14.0f, Paint(Rgba(0, 0, 0, 0.18f)));
	}
}

// ------------------------------------------------------------------ the keyed entry point

bool FaceSize(const std::string& Key, float& W, float& H)
{
	const std::string Pre = Key.substr(0, Key.find(':'));
	struct Size
	{
		const char* Pre;
		float W;
		float H;
	};
	static const Size Sizes[] = {{"hours", HoursDecalW, HoursDecalH}, {"open", OpenSignW, OpenSignH}, {"rack", RackTagW, RackTagH}, {"tag", PriceTagW, PriceTagH},
		{"pawnticket", PawnTicketW, PawnTicketH}, {"pawncard", PawnCardW, PawnCardH}, {"whiteboard", WhiteboardW, WhiteboardH}, {"backat", BackSignW, BackSignH},
		{"sandwich", SandwichW, SandwichH}, {"timetable", TimetableW, TimetableH}, {"shelterad", ShelterAdW, ShelterAdH}, {"dest", DestSignW, DestSignH},
		{"destside", DestSignW * 0.6f, DestSignH}, {"plate", PlateW, PlateH}, {"clock", ClockW, ClockH}, {"marquee", MarqueeW, MarqueeH}, {"valance", ValanceW, ValanceH}};
	if (Pre == "board")
	{
		const bool Barber = Key == "board:barber";
		W = Barber ? PriceBoardW : LetterBoardW;
		H = Barber ? PriceBoardH : LetterBoardH;
		return Key == "board:barber" || Key == "board:diner";
	}
	for (const Size& S : Sizes)
	{
		if (Pre == S.Pre)
		{
			W = S.W;
			H = S.H;
			return true;
		}
	}
	return false;
}

bool DrawFace(Canvas& C, const std::string& Key, const Session& S, uint64_t GarmentKinds)
{
	const size_t Colon = Key.find(':');
	const std::string Pre = Key.substr(0, Colon);
	const std::string Arg = Colon == std::string::npos ? std::string() : Key.substr(Colon + 1);
	const double World = S.WorldMinutes();
	if (Pre == "hours")
	{
		if (NameOf(Arg).empty())
		{
			return false;
		}
		HoursDecal(C, Arg);
		return true;
	}
	if (Pre == "open")
	{
		OpenSign(C, Arg, World);
		return true;
	}
	if (Key == "board:barber")
	{
		PriceBoard(C);
		return true;
	}
	if (Key == "board:diner")
	{
		LetterBoard(C);
		return true;
	}
	if (Pre == "rack")
	{
		RackTag(C, Arg, S, GarmentKinds);
		return true;
	}
	if (Pre == "tag")
	{
		for (const shop::Lot& L : S.PawnShelf())
		{
			if (L.Id == Arg)
			{
				bool Auction = false;
				for (const shop::ShelfLot& Sh : S.Life.Pawn.Shelf)
				{
					Auction = Auction || (Sh.Lot == Arg && Sh.Auction);
				}
				PriceTag(C, L.Name, L.ValueCents, Auction);
				return true;
			}
		}
		return false;
	}
	if (Pre == "pawnticket")
	{
		const int N = std::atoi(Arg.c_str());
		for (const shop::Ticket& T : S.Life.Pawn.Tickets)
		{
			if (T.Number == N)
			{
				PawnTicket(C, T, LotName(S, T.Lot), World);
				return true;
			}
		}
		return false;
	}
	if (Pre == "pawncard")
	{
		PawnCard(C, std::atoi(Arg.c_str()), World);
		return true;
	}
	if (Pre == "whiteboard")
	{
		Whiteboard(C, World);
		return true;
	}
	if (Pre == "backat")
	{
		BackSign(C, World);
		return true;
	}
	if (Pre == "sandwich")
	{
		SandwichBoard(C, Arg, World);
		return true;
	}
	if (Pre == "timetable")
	{
		Timetable(C, Arg.empty() ? std::string("market-east") : Arg, World);
		return true;
	}
	if (Pre == "shelterad")
	{
		ShelterAd(C);
		return true;
	}
	if (Pre == "dest" || Pre == "destside")
	{
		DestinationSign(C, Arg, Pre == "destside");
		return true;
	}
	if (Pre == "plate")
	{
		Plate(C, Arg);
		return true;
	}
	if (Pre == "clock")
	{
		BankClock(C, Arg.empty() ? World : std::atof(Arg.c_str()));
		return true;
	}
	if (Pre == "marquee")
	{
		Marquee(C, World);
		return true;
	}
	if (Pre == "valance")
	{
		if (NameOf(Arg).empty())
		{
			return false;
		}
		Valance(C, Arg);
		return true;
	}
	return false;
}

// ------------------------------------------------------------------ garments

uint32_t GarmentTone(const wardrobe::Garment& G)
{
	if (const uint32_t Core = wardrobe::Tone(G))
	{
		return Core;
	}
	const int I = ((G.Color % 8) + 8) % 8;
	switch (G.Kind)
	{
	case wardrobe::Cut::Jeans:
	case wardrobe::Cut::Shorts:
	{
		static const uint32_t Denim[8] = {0x3d5a80, 0x1f2f4a, 0x15161a, 0x7f9bb8, 0x5b6f86, 0x2c3e5c, 0xbfa883, 0x4b5638};
		return Denim[I];
	}
	case wardrobe::Cut::Chinos:
	case wardrobe::Cut::WorkPants:
	{
		static const uint32_t Twill[8] = {0xbfa883, 0x1f2d4d, 0x4b5638, 0x15161a, 0x8a6f5a, 0x6b6e73, 0x6b5a3a, 0x3a3d44};
		return Twill[I];
	}
	case wardrobe::Cut::Sneakers:
	{
		static const uint32_t Kicks[8] = {0xf2f2ef, 0x15161a, 0xc0262f, 0x1f2d4d, 0x8c9096, 0x2f6b3d, 0xe8c56a, 0x6a1f2e};
		return Kicks[I];
	}
	case wardrobe::Cut::WorkShoes:
	{
		static const uint32_t Leather[8] = {0x5a3a22, 0x15161a, 0x8a6a3a, 0x3a2418, 0x6b4a2e, 0x2b2d31, 0x7a5a3a, 0x4a3220};
		return Leather[I];
	}
	case wardrobe::Cut::Round:
	case wardrobe::Cut::Square:
	case wardrobe::Cut::Wire:
	case wardrobe::Cut::Shades:
	case wardrobe::Cut::Aviators:
	{
		static const uint32_t Frames[8] = {0x15161a, 0x3a2418, 0xc9a24d, 0x8c9096, 0x6a1f2e, 0x1f2d4d, 0xb08d57, 0x2b2d31};
		return Frames[I];
	}
	default: return hero::OutfitTone(I);
	}
}

namespace
{
/** A garment's box: unit coordinates (0..1 across, 0..1 down) to R, keeping it square-ish and centered. */
struct Box
{
	float X0 = 0.0f;
	float Y0 = 0.0f;
	float S = 1.0f;
	explicit Box(const Rect& R)
	{
		S = std::min(R.W, R.H);
		X0 = R.X + (R.W - S) * 0.5f;
		Y0 = R.Y + (R.H - S) * 0.5f;
	}
	Vec2 P(float U, float V) const { return {X0 + U * S, Y0 + V * S}; }
	float L(float U) const { return U * S; }
	std::vector<Vec2> Poly(std::initializer_list<std::pair<float, float>> Pts) const
	{
		std::vector<Vec2> Out;
		for (const auto& Q : Pts)
		{
			Out.push_back(P(Q.first, Q.second));
		}
		return Out;
	}
};

Paint Cloth(const Box& B, const Color& Base)
{
	return Paint::Linear(B.P(0.15f, 0.0f), B.P(0.85f, 1.0f), Lum(Base, 0.12f), Lum(Base, -0.3f));
}

void Hanger(Canvas& C, const Box& B)
{
	const Color Wire = Hex(0xb9c0c9);
	C.StrokeArc(B.P(0.5f, 0.06f).X + B.L(0.03f), B.P(0.5f, 0.06f).Y, B.L(0.035f), 3.14159f, 6.28318f, Wire, B.L(0.012f), true);
	C.StrokePolyline({B.P(0.5f, 0.07f), B.P(0.5f, 0.11f)}, false, Wire, B.L(0.012f), true);
	C.StrokePolyline({B.P(0.5f, 0.11f), B.P(0.2f, 0.2f), B.P(0.8f, 0.2f), B.P(0.5f, 0.11f)}, false, Wire, B.L(0.012f), true);
}

/** A top or jacket's body with sleeves, hung on a hanger. */
std::vector<Vec2> TopShape(const Box& B, float Hem, float SleeveLen)
{
	return B.Poly({{0.36f, 0.17f}, {0.64f, 0.17f}, {0.84f, 0.24f}, {0.92f, 0.24f + SleeveLen}, {0.8f, 0.27f + SleeveLen}, {0.74f, 0.4f}, {0.74f, Hem}, {0.26f, Hem}, {0.26f, 0.4f}, {0.2f, 0.27f + SleeveLen},
		{0.08f, 0.24f + SleeveLen}, {0.16f, 0.24f}});
}

void Wear(Canvas& C, const Box& B, int Condition, float Top, float Bottom)
{
	if (Condition >= 1)
	{
		// Faded at the folds.
		C.FillPolygon(B.Poly({{0.3f, Top}, {0.42f, Top}, {0.36f, Bottom}, {0.28f, Bottom}}), Paint(Rgba(255, 255, 255, 0.07f)));
	}
	if (Condition >= 2)
	{
		// A patch and a frayed edge.
		C.FillRoundRect({B.P(0.58f, Bottom - 0.16f).X, B.P(0.58f, Bottom - 0.16f).Y, B.L(0.1f), B.L(0.08f)}, B.L(0.01f), Paint(Rgba(0, 0, 0, 0.18f)));
		C.StrokeRoundRect({B.P(0.58f, Bottom - 0.16f).X, B.P(0.58f, Bottom - 0.16f).Y, B.L(0.1f), B.L(0.08f)}, B.L(0.01f), Rgba(255, 255, 255, 0.3f), 1.0f);
		for (int K = 0; K < 5; ++K)
		{
			const float U = 0.3f + 0.1f * Fl(K);
			C.StrokePolyline({B.P(U, Bottom), B.P(U + 0.01f, Bottom + 0.02f)}, false, Rgba(255, 255, 255, 0.35f), 1.0f);
		}
	}
}

void Glasses(Canvas& C, const Box& B, wardrobe::Cut K, const Color& Frame)
{
	const float Wd = K == wardrobe::Cut::Wire || K == wardrobe::Cut::Aviators ? B.L(0.012f) : B.L(0.03f);
	const bool Dark = K == wardrobe::Cut::Shades || K == wardrobe::Cut::Aviators;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float Cx = 0.5f + 0.19f * Fl(Side);
		std::vector<Vec2> Lens;
		if (K == wardrobe::Cut::Square)
		{
			Lens = Canvas::RoundRectPath({B.P(Cx - 0.15f, 0.4f).X, B.P(Cx - 0.15f, 0.4f).Y, B.L(0.3f), B.L(0.2f)}, B.L(0.04f), 4);
		}
		else if (K == wardrobe::Cut::Aviators)
		{
			// The teardrop, mirrored: a near-straight brow bar, the outer edge sweeping down, the deepest point toward the
			// nose.
			const float In = -Fl(Side); // toward the bridge
			Lens = B.Poly({{Cx - 0.16f * In, 0.41f}, {Cx + 0.13f * In, 0.4f}, {Cx + 0.15f * In, 0.47f}, {Cx + 0.1f * In, 0.6f}, {Cx + 0.02f * In, 0.65f}, {Cx - 0.08f * In, 0.63f},
				{Cx - 0.15f * In, 0.53f}});
		}
		else if (K == wardrobe::Cut::Shades)
		{
			Lens = B.Poly({{Cx - 0.16f, 0.41f}, {Cx + 0.16f, 0.4f}, {Cx + 0.14f, 0.58f}, {Cx - 0.14f, 0.6f}});
		}
		else
		{
			Lens = Canvas::EllipsePath(B.P(Cx, 0.5f).X, B.P(Cx, 0.5f).Y, B.L(0.14f), B.L(K == wardrobe::Cut::Wire ? 0.1f : 0.12f), 0.0f, 6.2832f, 32, false);
		}
		if (K == wardrobe::Cut::Aviators)
		{
			// The pawn's gold-rims: a brown gradient lens, dark at the brow and clearing toward the cheek.
			C.FillPolygon(Lens, Paint::Linear(B.P(Cx, 0.4f), B.P(Cx, 0.65f), Rgba(52, 38, 24, 0.95f), Rgba(150, 112, 66, 0.7f)));
		}
		else
		{
			C.FillPolygon(Lens, Paint(Dark ? Rgba(20, 24, 30, 0.85f) : Rgba(200, 225, 245, 0.18f)));
		}
		C.StrokePolyline(Lens, true, Frame, Wd);
		// A glint off the glass.
		C.StrokePolyline({B.P(Cx - 0.06f, 0.45f), B.P(Cx - 0.02f, 0.43f)}, false, Rgba(255, 255, 255, Dark ? 0.3f : 0.5f), B.L(0.008f), true);
	}
	C.StrokePolyline({B.P(0.43f, 0.47f), B.P(0.5f, 0.45f), B.P(0.57f, 0.47f)}, false, Frame, Wd);
	C.StrokePolyline({B.P(0.16f, 0.46f), B.P(0.04f, 0.42f)}, false, Frame, Wd, true);
	C.StrokePolyline({B.P(0.84f, 0.46f), B.P(0.96f, 0.42f)}, false, Frame, Wd, true);
}
} // namespace

void Garment(Canvas& C, const wardrobe::Garment& G, const Rect& R)
{
	const Box B(R);
	const Color Base = Hex(GarmentTone(G));
	const Color Seam = Alpha(Lum(Base, -0.5f), 0.6f);
	// A soft shadow under whatever it is.
	C.FillEllipse(B.P(0.5f, 0.92f).X, B.P(0.5f, 0.92f).Y, B.L(0.34f), B.L(0.04f), Paint::Radial(B.P(0.5f, 0.92f), 0.0f, B.P(0.5f, 0.92f), B.L(0.34f), Rgba(0, 0, 0, 0.35f), 0.5f, Rgba(0, 0, 0, 0.12f), Rgba(0, 0, 0, 0.0f)));
	switch (G.Kind)
	{
	case wardrobe::Cut::Tee:
	{
		Hanger(C, B);
		C.FillPolygon(TopShape(B, 0.84f, 0.06f), Cloth(B, Base));
		C.StrokeArc(B.P(0.5f, 0.17f).X, B.P(0.5f, 0.17f).Y, B.L(0.07f), 0.0f, 3.14159f, Seam, B.L(0.015f));
		if (G.Graphic >= 0)
		{
			// A chest graphic: a ring and a star, in a color off the shirt.
			const Color Ink = Luma(Base) > 0.5f ? Hex(0x1b1d22) : Hex(0xf2efe6);
			C.StrokeEllipse(B.P(0.5f, 0.46f).X, B.P(0.5f, 0.46f).Y, B.L(0.1f), B.L(0.1f), Ink, B.L(0.015f));
			C.FillPolygon(B.Poly({{0.5f, 0.39f}, {0.52f, 0.44f}, {0.57f, 0.45f}, {0.53f, 0.48f}, {0.54f, 0.53f}, {0.5f, 0.5f}, {0.46f, 0.53f}, {0.47f, 0.48f}, {0.43f, 0.45f}, {0.48f, 0.44f}}), Paint(Ink));
		}
		else if (G.Print >= 0)
		{
			for (int K = 0; K < 9; ++K)
			{
				C.FillCircle(B.P(0.32f + 0.09f * Fl(K % 4), 0.3f + 0.13f * Fl(K / 4)).X, B.P(0.32f + 0.09f * Fl(K % 4), 0.3f + 0.13f * Fl(K / 4)).Y, B.L(0.018f), Paint(Alpha(Lum(Base, 0.4f), 0.7f)));
			}
		}
		Wear(C, B, G.Condition, 0.3f, 0.84f);
		break;
	}
	case wardrobe::Cut::Hoodie:
	case wardrobe::Cut::Bomber:
	case wardrobe::Cut::Flannel:
	case wardrobe::Cut::Denim:
	case wardrobe::Cut::Leather:
	case wardrobe::Cut::Track:
	{
		Hanger(C, B);
		const bool Long = G.Kind == wardrobe::Cut::Hoodie || G.Kind == wardrobe::Cut::Flannel;
		const std::vector<Vec2> Body = TopShape(B, Long ? 0.86f : 0.8f, 0.36f);
		C.FillPolygon(Body, Cloth(B, Base));
		if (G.Kind == wardrobe::Cut::Flannel)
		{
			// The plaid: dark bands and a light thread, clipped to the shirt by drawing inside its box.
			for (int K = 0; K < 6; ++K)
			{
				C.FillRect({B.P(0.28f + 0.08f * Fl(K), 0.2f).X, B.P(0.28f, 0.2f).Y, B.L(0.025f), B.L(0.64f)}, Paint(Alpha(Lum(Base, -0.55f), 0.35f)));
				C.FillRect({B.P(0.27f, 0.26f + 0.1f * Fl(K)).X, B.P(0.27f, 0.26f + 0.1f * Fl(K)).Y, B.L(0.46f), B.L(0.025f)}, Paint(Alpha(Lum(Base, -0.55f), 0.3f)));
			}
			for (float U = 0.31f; U < 0.74f; U += 0.08f)
			{
				C.FillCircle(B.P(0.5f, U + 0.06f).X, B.P(0.5f, U + 0.06f).Y, B.L(0.01f), Paint(Hex(0xe8e0cc)));
			}
		}
		if (G.Kind == wardrobe::Cut::Hoodie)
		{
			C.FillEllipse(B.P(0.5f, 0.18f).X, B.P(0.5f, 0.18f).Y, B.L(0.13f), B.L(0.06f), Paint(Lum(Base, -0.25f)));
			C.FillRoundRect({B.P(0.36f, 0.62f).X, B.P(0.36f, 0.62f).Y, B.L(0.28f), B.L(0.12f)}, B.L(0.03f), Paint(Lum(Base, -0.12f)));
			C.StrokePolyline({B.P(0.47f, 0.2f), B.P(0.46f, 0.36f)}, false, Hex(0xe9e4da), B.L(0.008f), true);
			C.StrokePolyline({B.P(0.53f, 0.2f), B.P(0.54f, 0.36f)}, false, Hex(0xe9e4da), B.L(0.008f), true);
		}
		else
		{
			// The front: a zip or the placket.
			C.StrokePolyline({B.P(0.5f, 0.19f), B.P(0.5f, Long ? 0.86f : 0.8f)}, false, G.Kind == wardrobe::Cut::Flannel ? Alpha(Lum(Base, -0.3f), 0.6f) : Hex(0x9a9da4), B.L(0.01f));
		}
		if (G.Kind == wardrobe::Cut::Bomber)
		{
			C.FillRect({B.P(0.26f, 0.76f).X, B.P(0.26f, 0.76f).Y, B.L(0.48f), B.L(0.04f)}, Paint(Lum(Base, -0.35f)));
			C.FillRoundRect({B.P(0.4f, 0.16f).X, B.P(0.4f, 0.16f).Y, B.L(0.2f), B.L(0.05f)}, B.L(0.02f), Paint(Lum(Base, -0.4f)));
		}
		if (G.Kind == wardrobe::Cut::Denim)
		{
			const Color Stitch = Alpha(Hex(0xd9953b), 0.8f);
			C.StrokePolyline({B.P(0.3f, 0.36f), B.P(0.45f, 0.36f), B.P(0.45f, 0.46f), B.P(0.3f, 0.46f), B.P(0.3f, 0.36f)}, false, Stitch, B.L(0.006f));
			C.StrokePolyline({B.P(0.55f, 0.36f), B.P(0.7f, 0.36f), B.P(0.7f, 0.46f), B.P(0.55f, 0.46f), B.P(0.55f, 0.36f)}, false, Stitch, B.L(0.006f));
			C.FillPolygon(B.Poly({{0.4f, 0.17f}, {0.5f, 0.24f}, {0.6f, 0.17f}, {0.58f, 0.27f}, {0.5f, 0.3f}, {0.42f, 0.27f}}), Paint(Lum(Base, 0.1f)));
		}
		if (G.Kind == wardrobe::Cut::Leather)
		{
			C.FillPolygon(B.Poly({{0.4f, 0.17f}, {0.5f, 0.3f}, {0.38f, 0.42f}, {0.32f, 0.24f}}), Paint(Lum(Base, 0.12f)));
			C.FillPolygon(B.Poly({{0.6f, 0.17f}, {0.5f, 0.3f}, {0.62f, 0.42f}, {0.68f, 0.24f}}), Paint(Lum(Base, 0.06f)));
			C.FillEllipse(B.P(0.36f, 0.5f).X, B.P(0.36f, 0.5f).Y, B.L(0.05f), B.L(0.14f), Paint(Rgba(255, 255, 255, 0.1f)));
		}
		if (G.Kind == wardrobe::Cut::Track)
		{
			const Color Stripe = Luma(Base) > 0.55f ? Hex(0x15161a) : Hex(0xf1eee8);
			C.StrokePolyline({B.P(0.8f, 0.25f), B.P(0.88f, 0.58f)}, false, Stripe, B.L(0.012f), true);
			C.StrokePolyline({B.P(0.2f, 0.25f), B.P(0.12f, 0.58f)}, false, Stripe, B.L(0.012f), true);
		}
		Wear(C, B, G.Condition, 0.3f, Long ? 0.86f : 0.8f);
		break;
	}
	case wardrobe::Cut::Shorts:
	case wardrobe::Cut::Jeans:
	case wardrobe::Cut::Chinos:
	case wardrobe::Cut::WorkPants:
	{
		const bool Short = G.Kind == wardrobe::Cut::Shorts;
		const float Hem = Short ? 0.52f : 0.9f;
		C.FillPolygon(B.Poly({{0.3f, 0.12f}, {0.7f, 0.12f}, {0.74f, Hem}, {0.54f, Hem}, {0.5f, 0.3f}, {0.46f, Hem}, {0.26f, Hem}}), Cloth(B, Base));
		C.FillRect({B.P(0.3f, 0.12f).X, B.P(0.3f, 0.12f).Y, B.L(0.4f), B.L(0.05f)}, Paint(Lum(Base, -0.2f)));
		C.StrokePolyline({B.P(0.5f, 0.17f), B.P(0.5f, 0.3f)}, false, Seam, B.L(0.008f));
		if (G.Kind == wardrobe::Cut::Jeans || Short)
		{
			const Color Stitch = Alpha(Hex(0xd9953b), 0.75f);
			C.StrokeArc(B.P(0.34f, 0.17f).X, B.P(0.34f, 0.17f).Y, B.L(0.07f), 0.0f, 1.57f, Stitch, B.L(0.006f));
			C.StrokeArc(B.P(0.66f, 0.17f).X, B.P(0.66f, 0.17f).Y, B.L(0.07f), 1.57f, 3.14f, Stitch, B.L(0.006f));
			C.FillCircle(B.P(0.5f, 0.145f).X, B.P(0.5f, 0.145f).Y, B.L(0.012f), Paint(Hex(0xb07a3a)));
		}
		if (G.Kind == wardrobe::Cut::WorkPants)
		{
			C.FillRoundRect({B.P(0.29f, 0.56f).X, B.P(0.29f, 0.56f).Y, B.L(0.14f), B.L(0.12f)}, B.L(0.02f), Paint(Lum(Base, -0.15f)));
			C.FillRoundRect({B.P(0.57f, 0.56f).X, B.P(0.57f, 0.56f).Y, B.L(0.14f), B.L(0.12f)}, B.L(0.02f), Paint(Lum(Base, -0.15f)));
		}
		Wear(C, B, G.Condition, 0.2f, Hem);
		break;
	}
	case wardrobe::Cut::Sneakers:
	case wardrobe::Cut::WorkShoes:
	{
		const bool Boot = G.Kind == wardrobe::Cut::WorkShoes;
		for (int K = 0; K < 2; ++K)
		{
			const float Dx = Fl(K) * 0.12f - 0.06f;
			const float Dy = Fl(K) * 0.1f;
			C.FillPolygon(B.Poly({{0.14f + Dx, 0.66f + Dy}, {0.2f + Dx, Boot ? 0.36f + Dy : 0.48f + Dy}, {0.4f + Dx, Boot ? 0.34f + Dy : 0.46f + Dy}, {0.48f + Dx, 0.56f + Dy}, {0.8f + Dx, 0.6f + Dy},
								 {0.86f + Dx, 0.7f + Dy}, {0.14f + Dx, 0.72f + Dy}}),
				Cloth(B, Base));
			C.FillRect({B.P(0.14f + Dx, 0.7f + Dy).X, B.P(0.14f + Dx, 0.7f + Dy).Y, B.L(0.72f), B.L(0.05f)}, Paint(Boot ? Hex(0x2a2018) : Hex(0xf2f2ef)));
			for (int L = 0; L < 3; ++L)
			{
				C.StrokePolyline({B.P(0.3f + Dx + 0.05f * Fl(L), (Boot ? 0.42f : 0.5f) + Dy + 0.02f * Fl(L)), B.P(0.36f + Dx + 0.05f * Fl(L), (Boot ? 0.4f : 0.48f) + Dy + 0.02f * Fl(L))}, false,
					Boot ? Hex(0xc9a24d) : Hex(0xe9e4da), B.L(0.008f), true);
			}
		}
		break;
	}
	case wardrobe::Cut::Beanie:
	{
		C.FillPolygon(Canvas::EllipsePath(B.P(0.5f, 0.6f).X, B.P(0.5f, 0.6f).Y, B.L(0.3f), B.L(0.32f), 3.14159f, 6.28318f, 24, true), Cloth(B, Base));
		C.FillRoundRect({B.P(0.18f, 0.56f).X, B.P(0.18f, 0.56f).Y, B.L(0.64f), B.L(0.14f)}, B.L(0.03f), Paint(Lum(Base, -0.15f)));
		for (int K = 0; K < 9; ++K)
		{
			C.StrokePolyline({B.P(0.22f + 0.07f * Fl(K), 0.58f), B.P(0.22f + 0.07f * Fl(K), 0.68f)}, false, Alpha(Lum(Base, -0.4f), 0.5f), B.L(0.006f));
		}
		break;
	}
	case wardrobe::Cut::BallCap:
	case wardrobe::Cut::CapBack:
	{
		const bool Back = G.Kind == wardrobe::Cut::CapBack;
		C.FillPolygon(Canvas::EllipsePath(B.P(0.46f, 0.6f).X, B.P(0.46f, 0.6f).Y, B.L(0.26f), B.L(0.24f), 3.14159f, 6.28318f, 24, true), Cloth(B, Base));
		C.FillPolygon(Back ? B.Poly({{0.2f, 0.6f}, {0.06f, 0.66f}, {0.22f, 0.66f}}) : B.Poly({{0.7f, 0.6f}, {0.94f, 0.64f}, {0.9f, 0.68f}, {0.68f, 0.66f}}), Paint(Lum(Base, -0.25f)));
		C.FillCircle(B.P(0.46f, 0.37f).X, B.P(0.46f, 0.37f).Y, B.L(0.02f), Paint(Lum(Base, -0.2f)));
		break;
	}
	case wardrobe::Cut::Bucket:
	case wardrobe::Cut::Trilby:
	{
		const bool Trilby = G.Kind == wardrobe::Cut::Trilby;
		C.FillEllipse(B.P(0.5f, 0.64f).X, B.P(0.5f, 0.64f).Y, B.L(Trilby ? 0.38f : 0.34f), B.L(0.08f), Paint(Lum(Base, -0.2f)));
		C.FillPolygon(Trilby ? B.Poly({{0.3f, 0.64f}, {0.33f, 0.4f}, {0.5f, 0.36f}, {0.67f, 0.4f}, {0.7f, 0.64f}}) : B.Poly({{0.3f, 0.64f}, {0.34f, 0.38f}, {0.66f, 0.38f}, {0.7f, 0.64f}}), Cloth(B, Base));
		if (Trilby)
		{
			C.FillRect({B.P(0.31f, 0.56f).X, B.P(0.31f, 0.56f).Y, B.L(0.38f), B.L(0.05f)}, Paint(Hex(0x1b1d22)));
			C.StrokePolyline({B.P(0.42f, 0.4f), B.P(0.5f, 0.45f), B.P(0.58f, 0.4f)}, false, Alpha(Lum(Base, -0.5f), 0.6f), B.L(0.01f), true);
		}
		break;
	}
	case wardrobe::Cut::Round:
	case wardrobe::Cut::Square:
	case wardrobe::Cut::Wire:
	case wardrobe::Cut::Shades:
	case wardrobe::Cut::Aviators: Glasses(C, B, G.Kind, Base); break;
	default: break;
	}
}

// ------------------------------------------------------------------ the pawn's lots

namespace
{
void Cup(Canvas& C, const Box& B)
{
	const Paint Gold = Paint::Linear(B.P(0.3f, 0.0f), B.P(0.7f, 0.0f), Hex(0xfff1b8), Hex(0x9a7420));
	C.FillPolygon(B.Poly({{0.3f, 0.18f}, {0.7f, 0.18f}, {0.66f, 0.42f}, {0.56f, 0.5f}, {0.44f, 0.5f}, {0.34f, 0.42f}}), Gold);
	C.StrokeArc(B.P(0.27f, 0.3f).X, B.P(0.27f, 0.3f).Y, B.L(0.08f), 1.57f, 4.71f, Hex(0xc9a24d), B.L(0.025f));
	C.StrokeArc(B.P(0.73f, 0.3f).X, B.P(0.73f, 0.3f).Y, B.L(0.08f), -1.57f, 1.57f, Hex(0xc9a24d), B.L(0.025f));
	C.FillRect({B.P(0.47f, 0.5f).X, B.P(0.47f, 0.5f).Y, B.L(0.06f), B.L(0.14f)}, Gold);
	C.FillRoundRect({B.P(0.32f, 0.64f).X, B.P(0.32f, 0.64f).Y, B.L(0.36f), B.L(0.08f)}, B.L(0.01f), Paint(Hex(0x2a1c14)));
	C.FillRoundRect({B.P(0.28f, 0.72f).X, B.P(0.28f, 0.72f).Y, B.L(0.44f), B.L(0.12f)}, B.L(0.01f), Paint(Hex(0x3a2818)));
	C.FillRect({B.P(0.36f, 0.75f).X, B.P(0.36f, 0.75f).Y, B.L(0.28f), B.L(0.05f)}, Paint(Hex(0xc9a24d)));
	C.FillEllipse(B.P(0.42f, 0.26f).X, B.P(0.42f, 0.26f).Y, B.L(0.03f), B.L(0.08f), Paint(Rgba(255, 255, 255, 0.4f)));
}

/** The $100 chip from Dee's back room, on its stand. */
void DeeChip(Canvas& C, const Box& B)
{
	const Vec2 Cc = B.P(0.5f, 0.46f);
	C.FillRoundRect({B.P(0.3f, 0.72f).X, B.P(0.3f, 0.72f).Y, B.L(0.4f), B.L(0.08f)}, B.L(0.02f), Paint(Hex(0x2a1c14)));
	C.FillCircle(Cc.X, Cc.Y, B.L(0.28f), Paint(Hex(0x15161a)));
	for (int K = 0; K < 8; ++K)
	{
		const float A = Fl(K) * 0.785398f;
		C.FillRect({Cc.X + std::cos(A) * B.L(0.24f) - B.L(0.03f), Cc.Y + std::sin(A) * B.L(0.24f) - B.L(0.03f), B.L(0.06f), B.L(0.06f)}, Paint(Hex(0xe8c56a)));
	}
	C.FillCircle(Cc.X, Cc.Y, B.L(0.18f), Paint(Hex(0xf4f1ea)));
	C.Text("$100", Cc.X, Cc.Y + B.L(0.05f), Ts(B.L(0.13f), 900, Hex(0x15161a), Align::Center));
	C.Text("DEE'S", Cc.X, Cc.Y - B.L(0.08f), Ts(B.L(0.06f), 800, Hex(0x8e1b22), Align::Center));
}

void Watch(Canvas& C, const Box& B, bool Gold, bool Diver)
{
	const Color Case = Gold ? Hex(0xd9b45a) : Hex(0xc4c9cf);
	C.FillRoundRect({B.P(0.42f, 0.08f).X, B.P(0.42f, 0.08f).Y, B.L(0.16f), B.L(0.84f)}, B.L(0.04f), Paint(Gold ? Hex(0x3a2418) : Hex(0x9aa0a6)));
	C.FillCircle(B.P(0.5f, 0.5f).X, B.P(0.5f, 0.5f).Y, B.L(0.2f), Paint::Linear(B.P(0.3f, 0.3f), B.P(0.7f, 0.7f), Lum(Case, 0.3f), Lum(Case, -0.35f)));
	C.FillCircle(B.P(0.5f, 0.5f).X, B.P(0.5f, 0.5f).Y, B.L(0.16f), Paint(Diver ? Hex(0x10233d) : Hex(0xf6f2e6)));
	if (Diver)
	{
		C.StrokeEllipse(B.P(0.5f, 0.5f).X, B.P(0.5f, 0.5f).Y, B.L(0.18f), B.L(0.18f), Hex(0x1f3c88), B.L(0.02f));
	}
	for (int K = 0; K < 12; ++K)
	{
		const float A = Fl(K) * 0.5236f;
		C.FillCircle(B.P(0.5f, 0.5f).X + std::sin(A) * B.L(0.13f), B.P(0.5f, 0.5f).Y - std::cos(A) * B.L(0.13f), B.L(0.008f), Paint(Diver ? Hex(0xe9f0f7) : Hex(0x2b2723)));
	}
	C.StrokePolyline({B.P(0.5f, 0.5f), B.P(0.5f, 0.39f)}, false, Diver ? Hex(0xe9f0f7) : Hex(0x2b2723), B.L(0.012f), true);
	C.StrokePolyline({B.P(0.5f, 0.5f), B.P(0.58f, 0.53f)}, false, Diver ? Hex(0xe9f0f7) : Hex(0x2b2723), B.L(0.012f), true);
}

void Keepsake(Canvas& C, const Box& B, const std::string& Key)
{
	if (Key == "dealerskid" || Key == "dropout")
	{
		Watch(C, B, Key == "dealerskid", Key == "dropout");
		return;
	}
	if (Key == "kitchen")
	{
		// A canvas knife roll, open: three blades.
		C.FillRoundRect({B.P(0.12f, 0.3f).X, B.P(0.12f, 0.3f).Y, B.L(0.76f), B.L(0.44f)}, B.L(0.02f), Paint(Hex(0x5a4a36)));
		for (int K = 0; K < 3; ++K)
		{
			const float Y = 0.38f + 0.11f * Fl(K);
			C.FillRect({B.P(0.18f, Y).X, B.P(0.18f, Y).Y, B.L(0.2f), B.L(0.05f)}, Paint(Hex(0x2a1c14)));
			C.FillPolygon(B.Poly({{0.38f, Y}, {0.8f, Y + 0.005f}, {0.84f, Y + 0.025f}, {0.38f, Y + 0.05f}}), Paint::Linear(B.P(0.4f, Y), B.P(0.4f, Y + 0.05f), Hex(0xe9edf2), Hex(0x8b929c)));
		}
		return;
	}
	if (Key == "bouncer")
	{
		C.StrokeEllipse(B.P(0.5f, 0.56f).X, B.P(0.5f, 0.56f).Y, B.L(0.2f), B.L(0.2f), Hex(0xd9b45a), B.L(0.07f));
		C.FillCircle(B.P(0.5f, 0.35f).X, B.P(0.5f, 0.35f).Y, B.L(0.07f), Paint(Hex(0xc0262f)));
		C.FillCircle(B.P(0.48f, 0.33f).X, B.P(0.48f, 0.33f).Y, B.L(0.02f), Paint(Rgba(255, 255, 255, 0.6f)));
		return;
	}
	if (Key == "hustler")
	{
		std::vector<Vec2> Chain;
		for (int K = 0; K <= 30; ++K)
		{
			const float A = 0.3f + Fl(K) / 30.0f * 2.54f;
			Chain.push_back(B.P(0.5f + std::cos(A) * 0.32f, 0.3f + std::sin(A) * 0.42f));
		}
		C.StrokePolyline(Chain, false, Hex(0x9a7420), B.L(0.05f), true);
		C.StrokePolyline(Chain, false, Hex(0xf2d27a), B.L(0.025f), true);
		return;
	}
	// The guitar from the suitcase's other hand.
	C.FillCircle(B.P(0.42f, 0.66f).X, B.P(0.42f, 0.66f).Y, B.L(0.2f), Paint(Hex(0xb57a3a)));
	C.FillCircle(B.P(0.52f, 0.48f).X, B.P(0.52f, 0.48f).Y, B.L(0.15f), Paint(Hex(0xc4884a)));
	C.FillCircle(B.P(0.47f, 0.58f).X, B.P(0.47f, 0.58f).Y, B.L(0.06f), Paint(Hex(0x2a1c14)));
	C.FillPolygon(B.Poly({{0.55f, 0.42f}, {0.86f, 0.1f}, {0.9f, 0.14f}, {0.59f, 0.46f}}), Paint(Hex(0x3a2418)));
	C.FillRoundRect({B.P(0.84f, 0.04f).X, B.P(0.84f, 0.04f).Y, B.L(0.1f), B.L(0.1f)}, B.L(0.02f), Paint(Hex(0x2a1c14)));
}
} // namespace

void LotPicture(Canvas& C, const Session& S, const shop::Lot& L, const Rect& R, double Time)
{
	const std::string Rest = L.Id.substr(L.Id.find(':') + 1);
	const Box B(R);
	switch (L.Kind)
	{
	case shop::LotKind::Gear:
	{
		if (const gear::Item* I = gear::Find(Rest))
		{
			streamart::Product(C, I->Pic, R, I->Color, Time);
		}
		return;
	}
	case shop::LotKind::Award:
	{
		if (const world::Award* A = AwardOf(S, L.Id))
		{
			eventart::Trophy(C, *A, R.X + R.W * 0.5f, R.Y + R.H * 0.5f, std::min(R.W, R.H) * 0.9f, Time);
		}
		return;
	}
	case shop::LotKind::Memento:
		if (Rest == "deechip")
		{
			DeeChip(C, B);
		}
		else
		{
			Cup(C, B);
		}
		return;
	case shop::LotKind::Keepsake: Keepsake(C, B, Rest); return;
	case shop::LotKind::Garment:
	{
		if (const wardrobe::Garment* G = wardrobe::Find(Rest))
		{
			Garment(C, *G, R);
		}
		return;
	}
	case shop::LotKind::Curio:
	{
		if (Rest.find("aviator") != std::string::npos || Rest.find("trilby") != std::string::npos)
		{
			wardrobe::Garment G;
			G.Kind = Rest.find("aviator") != std::string::npos ? wardrobe::Cut::Aviators : wardrobe::Cut::Trilby;
			G.Where = G.Kind == wardrobe::Cut::Aviators ? wardrobe::Slot::Eyes : wardrobe::Slot::Hat;
			G.Color = G.Kind == wardrobe::Cut::Aviators ? 2 : 4; // gold rims; a sand felt
			Garment(C, G, R);
			return;
		}
		if (Rest.find("watch") != std::string::npos)
		{
			Watch(C, B, false, false);
			return;
		}
		// The Lucky Seven: a $5 chip from a room that closed in 1998.
		const Vec2 Cc = B.P(0.5f, 0.5f);
		C.FillCircle(Cc.X, Cc.Y, B.L(0.3f), Paint(Hex(0xc0262f)));
		for (int K = 0; K < 8; ++K)
		{
			const float A = Fl(K) * 0.785398f;
			C.FillRect({Cc.X + std::cos(A) * B.L(0.26f) - B.L(0.03f), Cc.Y + std::sin(A) * B.L(0.26f) - B.L(0.03f), B.L(0.06f), B.L(0.06f)}, Paint(Hex(0xf4f1ea)));
		}
		C.FillCircle(Cc.X, Cc.Y, B.L(0.19f), Paint(Hex(0xf4f1ea)));
		C.Text("7", Cc.X, Cc.Y + B.L(0.07f), Ts(B.L(0.22f), 900, Hex(0xc0262f), Align::Center));
		C.Text("$5", Cc.X, Cc.Y - B.L(0.09f), Ts(B.L(0.07f), 800, Hex(0x2b2723), Align::Center));
		return;
	}
	}
}
} // namespace shopart
} // namespace ui
} // namespace ss
