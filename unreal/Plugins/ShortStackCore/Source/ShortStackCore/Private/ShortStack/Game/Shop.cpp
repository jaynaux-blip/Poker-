#include "ShortStack/Game/Shop.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Life.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Game/Session.h"
#include "ShortStack/Game/Store.h"
#include "ShortStack/Game/Wardrobe.h"
#include "ShortStack/Game/World.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

// Northside's shops (the realism update's spec, §5, §7.1 and §7.3): the venues and their hours, what each sells, the
// pawn's rules, and the save lines with their migration. The Session's side is SessionShops.cpp.

namespace ss
{
namespace shop
{
namespace shop_detail
{
constexpr double Day = 1440.0;
constexpr double Week = 7.0 * Day;
constexpr int Mon = 1, Tue = 2, Wed = 4, Thu = 8, Fri = 16, Sat = 32, Sun = 64;
constexpr int Weekdays = Mon | Tue | Wed | Thu | Fri;
constexpr int MonSat = Weekdays | Sat;
constexpr int Daily = 0x7f;
const char* const Dash = "\xE2\x80\x93";
const char* const Dot = " \xC2\xB7 ";

// Which of Northside's lines a save had (life::State::ShopLinesRead).
constexpr uint32_t ReadCloset = 1, ReadFit = 2, ReadGroom = 4, ReadVisit = 8, ReadStoreVisit = 16, ReadPawn = 32, ReadMeal = 64;

// ------------------------------------------------------------------ hours

/** The window's span when it opens on day D (world minutes), or false when it doesn't open that day. */
bool Span(const Window& W, int D, double& From, double& To)
{
	if (((W.Days >> net::Weekday(D)) & 1) == 0)
	{
		return false;
	}
	const double Start = static_cast<double>(D) * Day;
	From = Start + static_cast<double>(W.Opens);
	To = Start + static_cast<double>(W.Closes > W.Opens ? W.Closes : W.Closes + 1440);
	return true;
}

/** Whether a window covers World, and where that window ends. */
bool Covers(const std::vector<Window>& Ws, double World, double* End)
{
	const int Today = net::DayOf(World);
	for (int D = Today - 1; D <= Today; ++D)
	{
		for (const Window& W : Ws)
		{
			double From = 0.0;
			double To = 0.0;
			if (Span(W, D, From, To) && World >= From && World < To)
			{
				if (End)
				{
					*End = To;
				}
				return true;
			}
		}
	}
	return false;
}

/** The end of the opening that covers World, windows that meet run together; World + a week for around the clock. */
double EndOf(const std::vector<Window>& Ws, double World)
{
	double End = World;
	if (!Covers(Ws, World, &End))
	{
		return World;
	}
	for (int Guard = 0; Guard < 64; ++Guard)
	{
		if (End - World >= Week)
		{
			return World + Week;
		}
		double Next = End;
		if (!Covers(Ws, End, &Next) || Next <= End)
		{
			break;
		}
		End = Next;
	}
	return std::min(End, World + Week);
}

double NextStart(const std::vector<Window>& Ws, double World)
{
	double Best = -1.0;
	const int Today = net::DayOf(World);
	for (int D = Today - 1; D <= Today + 8; ++D)
	{
		for (const Window& W : Ws)
		{
			double From = 0.0;
			double To = 0.0;
			if (Span(W, D, From, To) && From > World && (Best < 0.0 || From < Best))
			{
				Best = From;
			}
		}
	}
	return Best;
}

int MinuteOfDay(double World)
{
	const double In = World - std::floor(World / Day) * Day;
	return static_cast<int>(std::lround(In)) % 1440;
}

/** A time in a sentence: "10 AM", "9:30 PM", "noon", "midnight". */
std::string Said(double World)
{
	const int M = MinuteOfDay(World);
	if (M == 0)
	{
		return "midnight";
	}
	if (M == 720)
	{
		return "noon";
	}
	const int H24 = M / 60;
	const int Mm = M % 60;
	std::string Out = std::to_string(H24 % 12 == 0 ? 12 : H24 % 12);
	if (Mm != 0)
	{
		Out += (Mm < 10 ? ":0" : ":") + std::to_string(Mm);
	}
	return Out + (H24 < 12 ? " AM" : " PM");
}

/** A time on a sign: "10", "9:30", "6 AM" (Meridiem), "MIDNIGHT". */
std::string Printed(int Minute, bool Meridiem)
{
	const int M = ((Minute % 1440) + 1440) % 1440;
	if (M == 0)
	{
		return "MIDNIGHT";
	}
	const int H24 = M / 60;
	const int Mm = M % 60;
	std::string Out = std::to_string(H24 % 12 == 0 ? 12 : H24 % 12);
	if (Mm != 0)
	{
		Out += (Mm < 10 ? ":0" : ":") + std::to_string(Mm);
	}
	if (Meridiem)
	{
		Out += H24 < 12 ? " AM" : " PM";
	}
	return Out;
}

/** One window on a sign: "10–10" (Compact), "6 AM–12 PM", "4–10 PM", "11 AM–2 AM", "10 AM–MIDNIGHT". */
std::string Range(int Opens, int Closes, bool Compact)
{
	if (Compact)
	{
		return Printed(Opens, false) + Dash + Printed(Closes, false);
	}
	const int C = ((Closes % 1440) + 1440) % 1440;
	const bool Wrap = Closes <= Opens;
	if (!Wrap && C != 0 && (Opens < 720) == (C < 720))
	{
		return Printed(Opens, false) + Dash + Printed(Closes, true); // one half of the day: "4–10 PM"
	}
	return Printed(Opens, true) + Dash + Printed(Closes, C != 0);
}

using DaySchedule = std::vector<std::pair<int, int>>;

std::string ScheduleText(const DaySchedule& S)
{
	const bool Compact = S.size() == 1 && S[0].second > S[0].first && S[0].second < 1440;
	std::string Out;
	for (const auto& W : S)
	{
		Out += (Out.empty() ? "" : " & ") + Range(W.first, W.second, Compact);
	}
	return Out;
}

/** Days on a sign: "DAILY", "TUE–SUN", "FRI–SAT", "MON, WED–FRI". */
std::string DaysText(int Mask)
{
	static const char* const Names[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
	if ((Mask & Daily) == Daily)
	{
		return "DAILY";
	}
	std::string Out;
	for (int D = 0; D < 7;)
	{
		if (((Mask >> D) & 1) == 0)
		{
			++D;
			continue;
		}
		int E = D;
		while (E + 1 < 7 && ((Mask >> (E + 1)) & 1) != 0)
		{
			++E;
		}
		Out += (Out.empty() ? "" : ", ") + std::string(Names[D]) + (E > D ? Dash + std::string(Names[E]) : std::string());
		D = E + 1;
	}
	return Out;
}

std::string SignFor(const std::vector<Window>& Ws)
{
	std::array<DaySchedule, 7> Days;
	for (const Window& W : Ws)
	{
		for (int D = 0; D < 7; ++D)
		{
			if (((W.Days >> D) & 1) != 0)
			{
				Days[static_cast<size_t>(D)].push_back({W.Opens, W.Closes});
			}
		}
	}
	std::map<DaySchedule, int> Count;
	for (DaySchedule& S : Days)
	{
		std::sort(S.begin(), S.end());
		if (!S.empty())
		{
			++Count[S];
		}
	}
	if (Count.empty())
	{
		return "CLOSED";
	}
	// The usual hours: the schedule most days keep (the earliest such day's, on a tie).
	DaySchedule Base;
	int Most = 0;
	for (const DaySchedule& S : Days)
	{
		if (!S.empty() && Count[S] > Most)
		{
			Most = Count[S];
			Base = S;
		}
	}
	int BaseMask = 0;
	int ClosedMask = 0;
	std::vector<std::pair<int, int>> Til; // (close, days): the usual opening, another closing time
	std::vector<std::pair<DaySchedule, int>> Other;
	for (int D = 0; D < 7; ++D)
	{
		const DaySchedule& S = Days[static_cast<size_t>(D)];
		if (S.empty())
		{
			ClosedMask |= 1 << D;
		}
		else if (S == Base)
		{
			BaseMask |= 1 << D;
		}
		else if (Base.size() == 1 && S.size() == 1 && S[0].first == Base[0].first)
		{
			auto It = std::find_if(Til.begin(), Til.end(), [&](const std::pair<int, int>& T) { return T.first == S[0].second; });
			if (It == Til.end())
			{
				Til.push_back({S[0].second, 1 << D});
			}
			else
			{
				It->second |= 1 << D;
			}
		}
		else
		{
			auto It = std::find_if(Other.begin(), Other.end(), [&](const std::pair<DaySchedule, int>& O) { return O.first == S; });
			if (It == Other.end())
			{
				Other.push_back({S, 1 << D});
			}
			else
			{
				It->second |= 1 << D;
			}
		}
	}
	int First = BaseMask;
	for (const auto& T : Til)
	{
		First |= T.second;
	}
	const bool BaseCompact = Base.size() == 1 && Base[0].second > Base[0].first && Base[0].second < 1440;
	std::string Out = DaysText(First) + " " + ScheduleText(Base);
	for (const auto& T : Til)
	{
		Out += Dot + DaysText(T.second) + " TIL " + Printed(T.first, !BaseCompact);
	}
	for (const auto& O : Other)
	{
		Out += Dot + DaysText(O.second) + " " + ScheduleText(O.first);
	}
	if (ClosedMask != 0)
	{
		Out += std::string(Dot) + "CLOSED " + DaysText(ClosedMask);
	}
	return Out;
}

// ------------------------------------------------------------------ the venues

std::vector<Venue> BuildVenues()
{
	std::vector<Venue> L;
	auto Add = [&L](const char* Id, const char* Name, const char* Address, Kind K, bool Enterable, uint32_t Color, const std::vector<Window>& Open) -> Venue& {
		Venue V;
		V.Id = Id;
		V.Name = Name;
		V.Address = Address;
		V.Type = K;
		V.Enterable = Enterable;
		V.Color = Color;
		V.Open = Open;
		L.push_back(V);
		return L.back();
	};
	const std::vector<Window> AllDay = {{0, 1440, Daily}};
	// The enterable shops (§5.0).
	Add("store", "Lucky Penny #212", "1820 Fifth St.", Kind::Store, true, 0xff7a1a, AllDay);
	Add("barber", "Fifth St. Barbers", "1818 Fifth St.", Kind::Barber, true, 0xc8102e, {{600, 1320, Tue | Wed | Thu | Sun}, {600, 1440, Fri | Sat}});
	Add("thrift", "Second Chance Thrift", "220 Market St.", Kind::Thrift, true, 0x2f8f6a, {{600, 1260, MonSat}, {720, 1080, Sun}});
	{
		// Staffed 06:00-12:00 and 16:00-22:00; the public gets in only then, members on the fob at any hour.
		Venue& V = Add("gym", "IronHouse Gym", "246 Market St.", Kind::Gym, true, 0xef4444, {{360, 720, Daily}, {960, 1320, Daily}});
		V.MembersAnyHour = true;
	}
	Add("diner", "Ruby's", "201 Market St.", Kind::Diner, true, 0xe23b3b, AllDay);
	Add("pawn", "Northside Pawn & Loan", "232 Market St.", Kind::Pawn, true, 0xd9a520, {{540, 1200, Mon | Tue | Wed | Thu}, {540, 1260, Fri | Sat}, {660, 1020, Sun}});
	Add("washfold", "Wash & Fold", "1811 Fifth St.", Kind::Laundry, true, 0xff2e88, AllDay);
	// The display-only fronts (§2.3, §5.6), lit by their hours.
	Add("hardware", "Hollis Hardware", "1800 Fifth St.", Kind::Display, false, 0xd2452b, {{480, 1080, MonSat}});
	Add("kims", "Kim's Produce", "1804 Fifth St.", Kind::Display, false, 0x3f9b4a, {{420, 1260, Daily}});
	Add("quikcash", "QuikCash Check Cashing", "1814 Fifth St.", Kind::Display, false, 0x2f6fde, {{480, 1260, MonSat}, {600, 1020, Sun}});
	Add("wireless", "Northside Wireless", "1916 Fifth St.", Kind::Display, false, 0x1fb6d4, {{600, 1260, Daily}});
	Add("golden-dragon", "Golden Dragon", "1801 Fifth St.", Kind::Display, false, 0xe8b33a, {{660, 120, Daily}});
	Add("botanica", "Ortiz Bot\xC3\xA1nica", "1809 Fifth St.", Kind::Display, false, 0x8e44ad, {{600, 1140, Daily}});
	Add("records", "Northside Records", "1825 Fifth St.", Kind::Display, false, 0xf2c14e, {{720, 1260, Daily}});
	Add("dental", "Northside Dental", "1901 Fifth St.", Kind::Display, false, 0x4fb3bf, {{540, 1020, Weekdays}});
	Add("pharmacy", "Northside Pharmacy", "212 Market St.", Kind::Display, false, 0x2aa876, {{480, 1320, Daily}});
	Add("ginos", "Gino's Slice", "258 Market St.", Kind::Display, false, 0xd8262f, {{660, 120, Daily}});
	Add("halden", "Halden & Sons Clothiers", "213 Market St.", Kind::Display, false, 0xc9b27c, {{660, 1140, Daily}});
	Add("nails", "Lily Nails", "221 Market St.", Kind::Display, false, 0xff7eb6, {{600, 1200, Daily}});
	Add("liquor", "Market Wine & Liquor", "227 Market St.", Kind::Display, false, 0xb03a2e, {{600, 1440, Daily}});
	Add("marlowe", "The Marlowe", "239 Market St.", Kind::Display, false, 0xc8a24a, AllDay); // an apartment lobby, lit all night
	Add("anchor", "The Anchor", "251 Market St.", Kind::Display, false, 0x3b82f6, {{960, 240, Daily}});
	Add("furniture", "Cardinal Furniture", "265 Market St.", Kind::Display, false, 0xe0a33a, {{600, 1140, Daily}});
	Add("library", "Northside Branch Library", "Cardinal Ave. at Market St.", Kind::Display, false, 0xd8cfb8, {{600, 1200, Mon | Tue | Wed | Thu}, {600, 1080, Fri | Sat}});
	// Cardinal's proxy fronts (§2.3's C-S; its kit body is P1): a dry cleaner's hours.
	Add("cleaners", "Cardinal Cleaners", "300 Cardinal Ave.", Kind::Display, false, 0x5aa9e6, {{420, 1140, Weekdays}, {480, 1020, Sat}});
	return L;
}

// ------------------------------------------------------------------ the offers

std::vector<Offer> BuildOffers()
{
	std::vector<Offer> L;
	auto Add = [&L](const std::string& Id, const char* Venue, const char* Tab, OfferKind K, const std::string& Name, Chips Price, int Minutes, const std::string& Blurb) -> Offer& {
		Offer O;
		O.Id = Id;
		O.Venue = Venue;
		O.Tab = Tab;
		O.Type = K;
		O.Name = Name;
		O.PriceCents = Price;
		O.Minutes = Minutes;
		O.Blurb = Blurb;
		L.push_back(O);
		return L.back();
	};
	const int HairSlot = static_cast<int>(hero::Slot::Hair);
	const int BeardSlot = static_cast<int>(hero::Slot::FacialHair);
	const int ColorSlot = static_cast<int>(hero::Slot::HairColor);
	const int BrowSlot = static_cast<int>(hero::Slot::Brows);

	// Fifth St. Barbers (§5.1). The CUT tab's value is hero::Look's Hair.
	auto Cut = [&](const char* Id, const char* Name, Chips Price, int Minutes, int Hair, const char* Blurb) {
		Offer& O = Add(Id, "barber", "CUT", OfferKind::Cut, Name, Price, Minutes, Blurb);
		O.LookSlot = HairSlot;
		O.LookValue = Hair;
	};
	Cut("barber/lineup", "Line-up", 1200, 15, -1, "Same cut, cleaned up: the edges, the neckline, the part put back where it was.");
	Cut("barber/shaved", "Shaved", 2500, 25, 0, "Clippers, then a straight razor and a hot towel. Down to the skin.");
	Cut("barber/buzz", "Buzz Cut", 1500, 15, 1, "One guard all over. Fifteen minutes in the chair, three weeks of not thinking about it.");
	Cut("barber/crew", "Crew Cut", 2200, 30, 2, "Short on the sides, a little length on top, tapered at the neck.");
	Cut("barber/crop", "Textured Crop", 2400, 30, 3, "A short fringe, choppy on top, faded down the sides.");
	Cut("barber/sidepart", "Side Part", 2400, 30, 4, "The part cut in with a razor and combed over. Lonnie's favorite.");
	Cut("barber/undercut", "Undercut", 2600, 30, 5, "Sides taken down hard, the top left long and swept back.");
	Cut("barber/curls", "Curls", 2600, 35, 6, "A shape for the curls: the length evened out, the sides kept tight.");
	Cut("barber/afro", "Afro", 2800, 35, 7, "A shape-up: picked out, rounded off, the line-up sharp.");
	Cut("barber/bob", "Bob", 3000, 35, 11, "Chin length, one clean line all the way round.");
	Cut("barber/shoulder", "Shoulder Length", 3000, 35, 8, "A trim: the ends cleaned up, the length kept.");
	Cut("barber/ponytail", "Ponytail", 3000, 35, 9, "A trim, then tied back clean.");
	Cut("barber/braids", "Braids", 6500, 120, 10, "Two hours of cornrows, straight back. Takes hair down past the ears to start.");
	auto Beard = [&](const char* Id, const char* Name, Chips Price, int Minutes, int Value, const char* Blurb) {
		Offer& O = Add(Id, "barber", "BEARD", OfferKind::Beard, Name, Price, Minutes, Blurb);
		O.LookSlot = BeardSlot;
		O.LookValue = Value;
	};
	Beard("barber/shave", "Clean Shave", 2500, 25, 0, "Hot towel, lather, straight razor. The old way.");
	Beard("barber/stubble", "Stubble", 1000, 15, 1, "Taken down to a shadow, the edges lined.");
	Beard("barber/mustache", "Mustache", 1200, 15, 2, "The rest shaved clean; the lip trimmed square.");
	Beard("barber/goatee", "Goatee", 1500, 20, 3, "The chin kept, the cheeks shaved, the corners connected.");
	Beard("barber/beard-short", "Short Beard", 1800, 20, 4, "Trimmed even, the neck and cheek lines set with a razor.");
	Beard("barber/beard-full", "Full Beard", 1800, 25, 5, "Shaped, not shortened: the length kept, the edges lined.");
	static const char* const ColorKeys[8] = {"black", "darkbrown", "brown", "auburn", "copper", "blonde", "platinum", "grey"};
	for (int C = 0; C < 8; ++C)
	{
		Offer& O = Add(std::string("barber/color-") + ColorKeys[C], "barber", "COLOR", OfferKind::Color, hero::OptionName(hero::Slot::HairColor, C), 4500, 60,
			C == 7 ? "Silver all over, the way it's coming in anyway. An hour in the chair." : "All-over color, rinsed and dried. An hour in the chair.");
		O.LookSlot = ColorSlot;
		O.LookValue = C;
	}
	auto Brow = [&](const char* Id, const char* Name, int Value, const char* Blurb) {
		Offer& O = Add(Id, "barber", "BROWS", OfferKind::Brows, Name, 800, 10, Blurb);
		O.LookSlot = BrowSlot;
		O.LookValue = Value;
	};
	Brow("barber/brows-natural", "Natural", 1, "The strays taken out, the shape kept.");
	Brow("barber/brows-thin", "Thin", 0, "Taken down to a clean, narrow line.");
	Brow("barber/brows-arched", "Arched", 3, "Thinned and shaped to a lift at the tail.");

	// IronHouse Gym (§5.3): workouts are free to members and to anyone with today's day pass.
	auto Workout = [&](const char* Id, const char* Name, int Minutes, double Energy, double Hunger, double Thirst, double Base, const char* Blurb) -> Offer& {
		Offer& O = Add(Id, "gym", "WORKOUTS", OfferKind::Workout, Name, 0, Minutes, Blurb);
		O.Energy = -Energy;
		O.Hunger = -Hunger;
		O.Thirst = -Thirst;
		O.FitnessBase = Base;
		O.MembersFree = true;
		return O;
	};
	Workout("gym/treadmill", "Treadmill", 30, 10.0, 5.0, 12.0, 3.0, "Thirty minutes at the window, Market St. going by. You'll want water after.");
	Workout("gym/weights", "Weights", 45, 15.0, 10.0, 8.0, 4.0, "Forty-five minutes under the bar: the most fitness for the time, and the most it takes out of you.");
	Workout("gym/bag", "Heavy bag", 20, 8.0, 3.0, 6.0, 2.0, "Twenty minutes of jabs and crosses. Clears the head for eight hours: bad beats land softer.");
	{
		Offer& O = Workout("gym/trainer", "Trainer session", 60, 14.0, 8.0, 10.0, 5.0,
			"An hour with Rosa: form, breathing, the sets you'd skip. Three sessions and she teaches you Box Breathing.");
		O.Type = OfferKind::Training;
		O.PriceCents = 4000;
		O.StaffedOnly = true;
		O.MembersFree = false;
	}
	{
		Offer& O = Add("gym/daypass", "gym", "DESK", OfferKind::DayPass, "Day pass", 1000, 0, "Any machine, the bag and the weights, today, while the desk is staffed.");
		O.StaffedOnly = true;
	}
	{
		const gear::Item* Gym = gear::Find("gym");
		Offer& O = Add("gym/membership", "gym", "DESK", OfferKind::Membership, "Membership", Gym ? Gym->PriceCents : 3499, 0,
			"Monthly. The fob opens the door at any hour, and the sauna takes the edge off tilt. The same plan as GearDrop's: it renews every 30 days.");
		O.StaffedOnly = true;
	}
	{
		Offer& O = Add("gym/shake", "gym", "DESK", OfferKind::Shake, "IronHouse shake", 499, 0, "Chocolate whey and a banana, blended at the desk, and drunk there too.");
		O.Taxed = true;
		O.Hunger = 15.0;
		O.Thirst = 20.0;
		O.Energy = 2.0;
		O.StaffedOnly = true;
	}
	for (const char* Id : {"cascade", "volt-rush"})
	{
		// The cooler: the Lucky Penny's drinks at the Lucky Penny's prices, into the bag (a card reader on the door).
		if (const store::Item* I = store::Find(Id))
		{
			Offer& O = Add(std::string("gym/") + Id, "gym", "COOLER", OfferKind::Drink, I->Name, I->PriceCents, 0, I->Blurb);
			O.Taxed = true;
			O.Hunger = I->Hunger;
			O.Thirst = I->Thirst;
			O.Energy = I->Energy;
		}
	}

	// Ruby's (§5.4): the menu, taxed; a plate takes 25-40 minutes, a drink 10-15.
	auto Plate = [&](const char* Id, const char* Tab, OfferKind K, const char* Name, Chips Price, int Minutes, double Hunger, double Thirst, double Energy, const char* Blurb) {
		Offer& O = Add(Id, "diner", Tab, K, Name, Price, Minutes, Blurb);
		O.Taxed = true;
		O.Hunger = Hunger;
		O.Thirst = Thirst;
		O.Energy = Energy;
	};
	Plate("diner/short-stack", "MENU", OfferKind::Meal, "The Short Stack", 699, 30, 45.0, 0.0, 6.0, "Three buttermilk pancakes, a pat of butter, warm syrup. Thirty-one years on the same griddle.");
	Plate("diner/two-eggs", "MENU", OfferKind::Meal, "Two eggs, home fries, toast", 749, 30, 50.0, 0.0, 5.0, "Two eggs any way, home fries off the flat-top, rye or white.");
	Plate("diner/patty-melt", "MENU", OfferKind::Meal, "Patty melt & fries", 1099, 35, 65.0, -10.0, 8.0, "Griddled rye, Swiss, onions cooked down slow, fries on the side. You'll want water.");
	Plate("diner/grilled-cheese", "MENU", OfferKind::Meal, "Grilled cheese & tomato soup", 649, 30, 40.0, 10.0, 4.0, "American on sourdough, cut corner to corner, and a cup of tomato soup to dunk it in.");
	Plate("diner/chicken-soup", "MENU", OfferKind::Meal, "Chicken noodle soup", 549, 25, 30.0, 15.0, 3.0, "Ruby's own, egg noodles and a sleeve of crackers. What the regulars order when they're sick.");
	Plate("diner/steak-eggs", "MENU", OfferKind::Meal, "Steak & eggs", 1499, 40, 75.0, 0.0, 10.0, "A strip steak how you say, two eggs, toast. The most expensive line on the card.");
	Plate("diner/pie", "MENU", OfferKind::Meal, "Slice of pie", 450, 15, 15.0, 0.0, 4.0, "Cherry. It's always cherry.");
	Plate("diner/coffee", "DRINKS", OfferKind::Drink, "Coffee (bottomless)", 225, 15, 0.0, 15.0, 8.0, "Bottomless: a refill every half hour for as long as you're sitting.");
	Plate("diner/oj", "DRINKS", OfferKind::Drink, "Orange juice", 325, 10, 0.0, 30.0, 3.0, "Fresh-squeezed is a strong word. Cold, anyway.");
	Plate("diner/milkshake", "DRINKS", OfferKind::Drink, "Milkshake", 525, 15, 15.0, 15.0, 3.0, "Vanilla, chocolate or strawberry, in the steel cup with the rest on the side.");
	return L;
}

const std::vector<Offer>& AllOffers()
{
	static const std::vector<Offer> L = BuildOffers();
	return L;
}

// ------------------------------------------------------------------ the pawn

struct Keepsake
{
	hero::Background B;
	const char* Name;
	Chips Value;
};
const Keepsake Keepsakes[] = {
	{hero::Background::Kitchen, "Knife roll (Japanese steel)", 18000},
	{hero::Background::DealersKid, "Mom's dress watch, engraved FOR 20 YEARS", 30000},
	{hero::Background::Dropout, "Steel diver's watch", 22000},
	{hero::Background::Bouncer, "Embercrest five-year service ring", 24000},
	{hero::Background::Hustler, "Gold rope chain", 35000},
	{hero::Background::Newcomer, "Acoustic guitar", 16000},
};

/** Whole dollars, half up. */
Chips WholeDollars(Chips Cents)
{
	return Cents >= 0 ? (Cents + 50) / 100 * 100 : -((-Cents + 50) / 100 * 100);
}

// ------------------------------------------------------------------ save lines

std::string Escape(const std::string& In)
{
	std::string Out;
	for (const char Ch : In)
	{
		if (Ch == '\\')
		{
			Out += "\\\\";
		}
		else if (Ch == '\n')
		{
			Out += "\\n";
		}
		else if (Ch == '\t')
		{
			Out += "\\t";
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	return Out;
}

std::string Unescape(const std::string& In)
{
	std::string Out;
	for (size_t I = 0; I < In.size(); ++I)
	{
		if (In[I] == '\\' && I + 1 < In.size())
		{
			const char N = In[++I];
			Out.push_back(N == 'n' ? '\n' : N == 't' ? '\t' : N);
		}
		else
		{
			Out.push_back(In[I]);
		}
	}
	return Out;
}

int FloorDiv(int A, int B)
{
	return A >= 0 ? A / B : -((-A + B - 1) / B);
}
} // namespace shop_detail

using namespace shop_detail;

// ------------------------------------------------------------------ venues and hours

const std::vector<Venue>& Venues()
{
	static const std::vector<Venue> L = BuildVenues();
	return L;
}

const Venue* FindVenue(const std::string& Id)
{
	for (const Venue& V : Venues())
	{
		if (V.Id == Id)
		{
			return &V;
		}
	}
	return nullptr;
}

bool IsOpen(const Venue& V, double World)
{
	return Covers(V.Open, World, nullptr);
}

bool IsStaffed(const Venue& V, double World)
{
	return Covers(V.Staffed.empty() ? V.Open : V.Staffed, World, nullptr);
}

double NextOpen(const Venue& V, double World)
{
	if (IsOpen(V, World))
	{
		return World;
	}
	const double At = NextStart(V.Open, World);
	return At < 0.0 ? World + Week : At;
}

double ClosesAt(const Venue& V, double World)
{
	return EndOf(V.Open, World);
}

std::string HoursLine(const Venue& V, double World)
{
	if (V.Open.empty())
	{
		return "Closed";
	}
	if (IsOpen(V, World))
	{
		const double Shuts = ClosesAt(V, World);
		return Shuts >= World + Week ? "Open 24 hours" : "Open until " + Said(Shuts);
	}
	const double Opens = NextStart(V.Open, World);
	if (Opens < 0.0)
	{
		return "Closed";
	}
	if (net::DayOf(Opens) == net::DayOf(World))
	{
		return "Opens " + Said(Opens);
	}
	return std::string("Opens ") + net::WeekdayName(net::DayOf(Opens)) + " " + Said(Opens);
}

std::string HoursSign(const Venue& V)
{
	if (V.Open.empty())
	{
		return "CLOSED";
	}
	if (V.MembersAnyHour)
	{
		return std::string("MEMBERS ANY HOUR") + Dot + "STAFFED " + SignFor(V.Staffed.empty() ? V.Open : V.Staffed);
	}
	// Around the clock: from half past midnight on a Monday, it never closes.
	if (EndOf(V.Open, 30.0) >= 30.0 + Week)
	{
		return "OPEN 24 HOURS";
	}
	return SignFor(V.Open);
}

std::string KeeperOf(const std::string& Venue, double World)
{
	const shop::Venue* V = FindVenue(Venue);
	if (!V || !V->Enterable)
	{
		return std::string();
	}
	const int M = MinuteOfDay(World);
	switch (V->Type)
	{
	case Kind::Store: return M >= 600 && M < 1320 ? "ray" : "benny";
	case Kind::Diner: return M >= 360 && M < 1080 ? "ruby" : "marlene";
	case Kind::Laundry: return "inez";
	case Kind::Barber: return IsStaffed(*V, World) ? "lonnie" : "";
	case Kind::Thrift: return IsStaffed(*V, World) ? "bea" : "";
	case Kind::Gym: return IsStaffed(*V, World) ? "rosa" : "";
	case Kind::Pawn: return IsStaffed(*V, World) ? "augie" : "";
	case Kind::Display: break;
	}
	return std::string();
}

// ------------------------------------------------------------------ offers

const std::vector<Offer>& Offers(const std::string& Venue)
{
	static const std::map<std::string, std::vector<Offer>> ByVenue = [] {
		std::map<std::string, std::vector<Offer>> Out;
		for (const Offer& O : AllOffers())
		{
			Out[O.Venue].push_back(O);
		}
		return Out;
	}();
	static const std::vector<Offer> None;
	const auto It = ByVenue.find(Venue);
	return It == ByVenue.end() ? None : It->second;
}

const Offer* FindOffer(const std::string& Id)
{
	for (const Offer& O : AllOffers())
	{
		if (O.Id == Id)
		{
			return &O;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------ the pawn

bool Takes(const gear::Item& I)
{
	static const char* const Ids[] = {"monitor-24", "monitor-27", "tower-mid", "tower-dual", "macro-pad", "headphones", "webcam-720", "webcam-1080", "mirrorless", "mic-usb",
		"mic-xlr", "ring-light", "key-lights", "green-screen", "chair", "espresso"};
	if (I.Monthly)
	{
		return false;
	}
	for (const char* Id : Ids)
	{
		if (I.Id == Id)
		{
			return true;
		}
	}
	return false;
}

Chips GearValue(const gear::Item& I)
{
	// 55% of GearDrop's price, to the nearest dollar (half up), in whole cents: no double rounding near the half.
	return (std::max<Chips>(0, I.PriceCents) * 55 + 5000) / 10000 * 100;
}

Chips AwardValue(const world::Award& A)
{
	// A bracelet: $900 and 1% of what it paid, at most $6,000 ($9,000 for a Main Event's). A ring: $350 and 0.5%, at
	// most $2,500.
	const Chips Prize = std::max<Chips>(0, A.Prize);
	const Chips V = A.Ring ? std::min<Chips>(250000, 35000 + Prize / 200) : std::min<Chips>(A.Main ? 900000 : 600000, 90000 + Prize / 100);
	return WholeDollars(V);
}

Lot KeepsakeOf(hero::Background B)
{
	Lot L;
	L.Kind = LotKind::Keepsake;
	L.Final = true;
	L.Id = std::string("keepsake:") + hero::InfoOf(B).Key;
	for (const Keepsake& K : Keepsakes)
	{
		if (K.B == B)
		{
			L.Name = K.Name;
			L.ValueCents = K.Value;
		}
	}
	L.SellCents = SellOf(L.ValueCents);
	L.LoanCents = LoanOf(L.ValueCents);
	return L;
}

Chips SellOf(Chips Value)
{
	return std::max<Chips>(0, Value) / 200 * 100;
}

Chips LoanOf(Chips Value)
{
	return std::max<Chips>(0, Value) / 100 * 40 / 100 * 100;
}

Chips RedeemCents(const Ticket& T, double World)
{
	if (World <= T.DueAt)
	{
		return T.LoanCents * 120 / 100;
	}
	if (World <= T.DueAt + PawnGraceDays * Day)
	{
		return T.LoanCents * 125 / 100;
	}
	return 0;
}

Chips RenewCents(const Ticket& T)
{
	return T.LoanCents * 20 / 100;
}

Chips RenewCents(const Ticket& T, double World)
{
	// The interest a redemption would charge today, without the loan: the same 20% or 25% to the cent.
	const Chips Redeem = RedeemCents(T, World);
	return Redeem > 0 ? Redeem - T.LoanCents : 0;
}

bool Renewable(const Ticket& T, double World)
{
	return RedeemCents(T, World) > 0 && World + PawnTermDays * Day - T.DueAt >= Day;
}

std::string TicketNumber(int Number)
{
	char Buf[16];
	std::snprintf(Buf, sizeof(Buf), "%04d", std::max(0, Number));
	return Buf;
}

int UsedWeek(double World)
{
	return FloorDiv(net::DayOf(World), 7); // day 0 is a Monday
}

std::vector<ShelfLot> UsedStock(int WeekNumber, uint64_t Seed, uint64_t GearKinds)
{
	const std::vector<gear::Item>& All = gear::Catalog();
	std::vector<size_t> Pool;
	for (size_t I = 0; I < All.size() && I < 64; ++I)
	{
		if (Takes(All[I]) && ((GearKinds >> I) & 1ull) != 0)
		{
			Pool.push_back(I);
		}
	}
	Rng R("pawn-used:" + std::to_string(Seed) + ":" + std::to_string(WeekNumber));
	R.Shuffle(Pool);
	const size_t Want = std::min(Pool.size(), static_cast<size_t>(4 + R.Int(5)));
	Pool.resize(Want);
	std::sort(Pool.begin(), Pool.end());
	std::vector<ShelfLot> Out;
	const double From = static_cast<double>(WeekNumber) * Week;
	for (const size_t I : Pool)
	{
		ShelfLot S;
		S.Lot = "gear:" + All[I].Id;
		S.PriceCents = GearValue(All[I]);
		S.From = From;
		S.Until = From + Week;
		Out.push_back(S);
	}
	return Out;
}

void Consign(PawnBook& B, const std::vector<std::string>& LostGearIds, double World, uint64_t Seed)
{
	std::vector<std::string> Pool;
	for (const std::string& Id : LostGearIds)
	{
		const gear::Item* I = gear::Find(Id);
		if (I && Takes(*I) && std::find(Pool.begin(), Pool.end(), Id) == Pool.end())
		{
			Pool.push_back(Id);
		}
	}
	if (Pool.empty())
	{
		return;
	}
	Rng R("pawn-lot114:" + std::to_string(Seed) + ":" + std::to_string(static_cast<long long>(std::llround(World))));
	R.Shuffle(Pool);
	const size_t Want = std::min(Pool.size(), static_cast<size_t>(1 + R.Int(3)));
	for (size_t K = 0; K < Want; ++K)
	{
		const std::string Lot = "gear:" + Pool[K];
		if (std::any_of(B.Shelf.begin(), B.Shelf.end(), [&](const ShelfLot& S) { return S.Lot == Lot; }))
		{
			continue;
		}
		ShelfLot S;
		S.Lot = Lot;
		S.PriceCents = GearValue(*gear::Find(Pool[K]));
		S.From = World + 2.0 * Day;
		S.Until = S.From + PawnShelfDays * Day;
		S.Auction = true;
		B.Shelf.push_back(S);
	}
}

std::string AuctionWords(const PawnBook& B, double World)
{
	// How a pawnbroker says what came in, not how GearDrop lists it.
	static const std::pair<const char*, const char*> Said[] = {{"monitor-24", "a 24-inch"}, {"monitor-27", "a 27-inch"}, {"tower-mid", "a desktop"},
		{"tower-dual", "a two-PC rig"}, {"macro-pad", "a macro pad"}, {"headphones", "a pair of headphones"}, {"webcam-720", "a cheap webcam"},
		{"webcam-1080", "a good webcam"}, {"mirrorless", "a mirrorless camera"}, {"mic-usb", "a USB mic"}, {"mic-xlr", "a broadcast mic"}, {"ring-light", "a ring light"},
		{"key-lights", "a pair of key lights"}, {"green-screen", "a green screen"}, {"chair", "a chair"}, {"espresso", "an espresso machine"}};
	std::vector<std::pair<Chips, std::string>> Pieces;
	for (const ShelfLot& S : B.Shelf)
	{
		if (!S.Auction || World < S.From || World >= S.Until || S.Lot.rfind("gear:", 0) != 0)
		{
			continue;
		}
		const std::string Id = S.Lot.substr(5);
		std::string Words;
		for (const auto& W : Said)
		{
			Words = Id == W.first ? std::string(W.second) : Words;
		}
		if (Words.empty())
		{
			const gear::Item* I = gear::Find(Id);
			Words = I ? "a " + I->Name : std::string();
		}
		if (!Words.empty() && std::none_of(Pieces.begin(), Pieces.end(), [&Words](const std::pair<Chips, std::string>& P) { return P.second == Words; }))
		{
			Pieces.push_back({S.PriceCents, Words});
		}
	}
	if (Pieces.empty())
	{
		return "a few pieces out of it";
	}
	// The dearest first (a stable sort: the shelf's order breaks a tie).
	std::stable_sort(Pieces.begin(), Pieces.end(), [](const std::pair<Chips, std::string>& A, const std::pair<Chips, std::string>& Bp) { return A.first > Bp.first; });
	std::string Out;
	for (size_t I = 0; I < Pieces.size(); ++I)
	{
		Out += (I == 0 ? "" : I + 1 == Pieces.size() ? " and " : ", ") + Pieces[I].second;
	}
	return Out;
}

void Age(PawnBook& B, double Days)
{
	const double By = Days * Day;
	for (Ticket& T : B.Tickets)
	{
		T.TakenAt -= By;
		T.DueAt -= By;
	}
	for (ShelfLot& S : B.Shelf)
	{
		S.From -= By;
		S.Until -= By;
	}
}

const std::vector<Curio>& Curios()
{
	static const std::vector<Curio> L = {
		{"aviators", "Gold-rim aviators", "Seventies aviators in a thin gold rim. Augie swears a pilot sold them to him.", 1800, "eyes.aviators", 0},
		{"trilby", "Felt trilby", "From the Rialto's coat check, the night it closed. Nobody ever came back for it.", 1400, "hat.trilby", 0},
		{"luckyseven", "Lucky Seven chip", "A $5 chip from the Lucky Seven card room, closed since 1998. It sits on your hole cards like it means something.", 1500, "", 1},
		{"watch", "Steel wristwatch", "A plain steel automatic on a worn leather strap. Keeps time if you wear it.", 4500, "", 2},
	};
	return L;
}

const Curio* FindCurio(const std::string& Id)
{
	for (const Curio& C : Curios())
	{
		if (C.Id == Id)
		{
			return &C;
		}
	}
	return nullptr;
}

// ------------------------------------------------------------------ save lines

void WriteLines(const life::State& L, std::string& Out)
{
	// The closet: seeded once, then what's owned and what's on.
	if (L.Closet.Seeded)
	{
		Out += "closet\tseeded\n";
	}
	for (const std::string& Id : L.Closet.Owned)
	{
		Out += "closet\town\t" + Escape(Id) + "\n";
	}
	for (int S = 0; S < wardrobe::SlotCount; ++S)
	{
		const std::string& Id = L.Closet.Worn[static_cast<size_t>(S)];
		if (!Id.empty())
		{
			Out += "closet\twear\t" + std::to_string(S) + "\t" + Escape(Id) + "\n";
		}
	}
	const body::Fitness& F = L.Fit;
	Out += "fit\t" + Fixed(F.Level, 4) + "\t" + Fixed(F.LastCounted, 2) + "\t" + std::to_string(F.Workouts) + "\t" + std::to_string(F.Sessions) + "\t" + Fixed(F.ClearHeadUntil, 2) + "\t" +
		Fixed(F.LastDecay, 2) + "\n";
	Out += "groom\t" + Fixed(L.Groom.CutAt, 2) + "\t" + Fixed(L.Groom.ShaveAt, 2) + "\t" + std::to_string(L.Groom.Cuts) + "\n";
	for (const auto& V : L.Visits)
	{
		const Visit& Vi = V.second;
		Out += "visit\t" + Escape(V.first) + "\t" + std::to_string(Vi.Count) + "\t" + Fixed(Vi.LastAt, 2) + "\t" + std::to_string(Vi.SpentCents) + "\t" + Escape(Vi.Usual) + "\t" +
			std::to_string(Vi.Tips) + "\t" + std::to_string(Vi.Stiffs) + "\n";
	}
	const PawnBook& B = L.Pawn;
	for (const Ticket& T : B.Tickets)
	{
		Out += "pawn\tticket\t" + std::to_string(T.Number) + "\t" + Escape(T.Lot) + "\t" + std::to_string(T.LoanCents) + "\t" + Fixed(T.TakenAt, 2) + "\t" + Fixed(T.DueAt, 2) + "\t" +
			std::to_string(T.Renewals) + "\n";
	}
	for (const ShelfLot& S : B.Shelf)
	{
		Out += "pawn\tshelf\t" + Escape(S.Lot) + "\t" + std::to_string(S.PriceCents) + "\t" + Fixed(S.From, 2) + "\t" + Fixed(S.Until, 2) + "\t" + (S.Auction ? "1" : "0") + "\n";
	}
	for (const std::string& Id : B.Away)
	{
		Out += "pawn\taway\t" + Escape(Id) + "\n";
	}
	Out += "pawn\tnext\t" + std::to_string(B.NextTicket) + "\n";
	Out += "meal\t" + Fixed(L.CoffeeUntil, 2) + "\t" + Fixed(L.RealMealAt, 2) + "\n";
}

bool ReadLine(const std::vector<std::string>& F, life::State& L)
{
	if (F.empty())
	{
		return false;
	}
	const std::string& Key = F[0];
	auto Num = [&F](size_t I) { return I < F.size() ? std::atof(F[I].c_str()) : 0.0; };
	auto Int = [&F](size_t I) { return I < F.size() ? std::atoi(F[I].c_str()) : 0; };
	auto Cents = [&F](size_t I) -> Chips { return I < F.size() ? static_cast<Chips>(std::strtoll(F[I].c_str(), nullptr, 10)) : 0; };
	auto Str = [&F](size_t I) { return I < F.size() ? Unescape(F[I]) : std::string(); };
	if (Key == "closet")
	{
		L.ShopLinesRead |= ReadCloset;
		const std::string What = F.size() > 1 ? F[1] : std::string();
		if (What == "seeded")
		{
			L.Closet.Seeded = true;
		}
		else if (What == "own" && F.size() >= 3)
		{
			// A garment this build doesn't know (another branch's) is dropped.
			const std::string Id = Str(2);
			if (wardrobe::Find(Id))
			{
				L.Closet.Owned.insert(Id);
			}
		}
		else if (What == "wear" && F.size() >= 4)
		{
			const int S = Int(2);
			const std::string Id = Str(3);
			const wardrobe::Garment* G = wardrobe::Find(Id);
			if (S >= 0 && S < wardrobe::SlotCount && G && static_cast<int>(G->Where) == S)
			{
				L.Closet.Worn[static_cast<size_t>(S)] = Id;
			}
		}
		return true;
	}
	if (Key == "fit")
	{
		L.ShopLinesRead |= ReadFit;
		if (F.size() >= 7)
		{
			L.Fit.Level = std::clamp(Num(1), 0.0, 100.0);
			L.Fit.LastCounted = Num(2);
			L.Fit.Workouts = Int(3);
			L.Fit.Sessions = Int(4);
			L.Fit.ClearHeadUntil = Num(5);
			L.Fit.LastDecay = Num(6);
		}
		return true;
	}
	if (Key == "groom")
	{
		L.ShopLinesRead |= ReadGroom;
		if (F.size() >= 4)
		{
			L.Groom.CutAt = Num(1);
			L.Groom.ShaveAt = Num(2);
			L.Groom.Cuts = Int(3);
		}
		return true;
	}
	if (Key == "visit")
	{
		L.ShopLinesRead |= ReadVisit;
		if (F.size() >= 8)
		{
			const std::string Venue = Str(1);
			Visit& V = L.Visits[Venue];
			V.Count = Int(2);
			V.LastAt = Num(3);
			V.SpentCents = Cents(4);
			V.Usual = Str(5);
			V.Tips = Int(6);
			V.Stiffs = Int(7);
			if (Venue == "store")
			{
				L.ShopLinesRead |= ReadStoreVisit;
			}
		}
		return true;
	}
	if (Key == "pawn")
	{
		L.ShopLinesRead |= ReadPawn;
		const std::string What = F.size() > 1 ? F[1] : std::string();
		if (What == "ticket" && F.size() >= 8)
		{
			Ticket T;
			T.Number = Int(2);
			T.Lot = Str(3);
			T.LoanCents = Cents(4);
			T.TakenAt = Num(5);
			T.DueAt = Num(6);
			T.Renewals = Int(7);
			L.Pawn.Tickets.push_back(T);
		}
		else if (What == "shelf" && F.size() >= 7)
		{
			ShelfLot S;
			S.Lot = Str(2);
			S.PriceCents = Cents(3);
			S.From = Num(4);
			S.Until = Num(5);
			S.Auction = Int(6) != 0;
			L.Pawn.Shelf.push_back(S);
		}
		else if (What == "away" && F.size() >= 3)
		{
			L.Pawn.Away.insert(Str(2));
		}
		else if (What == "next" && F.size() >= 3)
		{
			L.Pawn.NextTicket = std::max(1, Int(2));
		}
		return true;
	}
	if (Key == "meal")
	{
		L.ShopLinesRead |= ReadMeal;
		if (F.size() >= 3)
		{
			L.CoffeeUntil = Num(1);
			L.RealMealAt = Num(2);
		}
		return true;
	}
	return false;
}

void AfterParse(SaveData& D)
{
	life::State& L = D.Life;
	const uint32_t Had = L.ShopLinesRead;
	const double Now = net::MinutesPerDay * static_cast<double>(net::NightOneDay) + D.ClockMinutes;
	// The closet (OPEN_WORLD step 4): what the player made in the creator becomes what they own. A career that never met
	// the creator has nothing to seed from: it gets its closet in the creator, or with its first purchase.
	if ((Had & ReadCloset) == 0 && D.Person.Created)
	{
		L.Closet = wardrobe::SeedFrom(D.Person.Appearance);
	}
	// Nothing is worn that isn't owned (a dropped garment takes its slot with it).
	for (std::string& Id : L.Closet.Worn)
	{
		if (!Id.empty() && L.Closet.Owned.count(Id) == 0)
		{
			Id.clear();
		}
	}
	// Fitness: an existing member lands at 60, about what the old passive membership gave. LastCounted is now, so the
	// fading starts in three days; with no workout behind it, the first one counts whenever it comes (body::Gain).
	// Everyone else at 0.
	if ((Had & ReadFit) == 0 && D.Gear.count("gym") > 0)
	{
		L.Fit.Level = 60.0;
		L.Fit.LastCounted = Now;
		L.Fit.LastDecay = Now;
	}
	// Grooming: a cut a week ago, fresh enough to keep the creator's hair (and the beard as it is).
	if ((Had & ReadGroom) == 0)
	{
		L.Groom.CutAt = Now - 7.0 * Day;
		L.Groom.ShaveAt = Now - 7.0 * Day;
	}
	// The Lucky Penny's visits, once, from the counter's lines in the ledger and what was bought most.
	if ((Had & ReadStoreVisit) == 0)
	{
		Visit V;
		for (const life::LedgerEntry& E : L.Ledger)
		{
			if (E.Kind == 8 && E.Label.rfind("Lucky Penny #212", 0) == 0)
			{
				++V.Count;
				V.LastAt = std::max(V.LastAt, E.At);
				V.SpentCents -= E.Amount;
			}
		}
		if (V.Count > 0)
		{
			int Most = 0;
			for (const auto& Item : L.Bought)
			{
				if (Item.second > Most)
				{
					Most = Item.second;
					V.Usual = Item.first;
				}
			}
			L.Visits["store"] = V;
		}
	}
	// The pawn book: an older save's is empty (tickets from 412). Never a ticket number twice.
	for (const Ticket& T : L.Pawn.Tickets)
	{
		L.Pawn.NextTicket = std::max(L.Pawn.NextTicket, T.Number + 1);
	}
}
} // namespace shop
} // namespace ss
