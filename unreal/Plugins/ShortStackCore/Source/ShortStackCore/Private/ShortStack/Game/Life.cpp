#include "ShortStack/Game/Life.h"
#include "../StrictFloat.h"

#include "ShortStack/Game/Format.h"
#include "ShortStack/Game/Live.h"
#include "ShortStack/Game/Network.h"
#include "ShortStack/Rng.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace life
{
namespace life_detail
{
const double MinutesInDay = 1440.0;

double TimeOfDay(double World)
{
	return World - std::floor(World / MinutesInDay) * MinutesInDay;
}

Chips Cents(double Dollars)
{
	return static_cast<Chips>(std::llround(Dollars * 100.0));
}

std::vector<Activity> Build()
{
	std::vector<Activity> L;
	auto Add = [&](const char* Id, Kind K, const char* Title, const char* Place, double Hours, double Wage, double PayMin, double PayMax, double Energy, double Heat, double Risk,
				   int Opens, int Closes, uint32_t Color, const char* Blurb) {
		Activity A;
		A.Id = Id;
		A.Type = K;
		A.Title = Title;
		A.Place = Place;
		A.Hours = Hours;
		A.WageCents = Cents(Wage);
		A.PayMin = Cents(PayMin);
		A.PayMax = Cents(PayMax);
		A.Energy = Energy;
		A.Heat = Heat;
		A.Risk = Risk;
		A.Opens = Opens;
		A.Closes = Closes;
		A.Color = Color;
		A.Blurb = Blurb;
		L.push_back(A);
	};
	// ShiftLink: the gig app. Minimum wage, steady, exhausting.
	Add("quikstop", Kind::Job, "Night cashier", "Lucky Penny #212", 6.0, 7.25, 0.0, 6.0, 30.0, 0.0, 0.0, 21 * 60, 3 * 60, 0xff7a1a,
		"Graveyard shift at the gas station on Fifth. Scratch tickets, energy drinks, the occasional weirdo.");
	Add("washfold", Kind::Job, "Attendant", "Wash & Fold", 6.0, 8.00, 0.0, 4.0, 24.0, 0.0, 0.0, 6 * 60, 16 * 60, 0xff2e88,
		"The laundromat across the street. Folding, mopping, change for the machines. Dee's regulars tip in quarters.");
	Add("dashdrop", Kind::Job, "Delivery driver", "DashDrop", 4.0, 6.00, 12.0, 48.0, 20.0, 0.0, 0.0, 11 * 60, 23 * 60, 0xe8ff3a,
		"Food runs in your own car. Base pay is a joke; the tips are the job.");
	Add("pallet", Kind::Job, "Loader", "Pallet Pros warehouse", 8.0, 9.50, 0.0, 0.0, 45.0, 0.0, 0.0, 5 * 60, 8 * 60, 0x3b82f6,
		"Early shift on the docks. Best pay in the app, and your back will hate you.");
	// Burner: the other way to make rent.
	Add("marcus-drop", Kind::Hustle, "Drop-off", "Marcus", 2.0, 0.0, 120.0, 200.0, 10.0, 20.0, 0.06, 20 * 60, 4 * 60, 0x3ecf6e,
		"Take a backpack across town to a guy named Tre. Don't open it. Don't speed.");
	Add("marcus-run", Kind::Hustle, "The long run", "Marcus", 5.0, 0.0, 380.0, 600.0, 22.0, 35.0, 0.12, 21 * 60, 2 * 60, 0x3ecf6e,
		"Two cities, one car, no stops. Real money for people Marcus trusts.");
	Add("sam-ghost", Kind::Ghost, "Play my account", "Sam", 3.0, 0.0, 150.0, 400.0, 18.0, 0.0, 0.10, 18 * 60, 3 * 60, 0xf2c14e,
		"Sam plays big and badly. He pays you to log in as him and play his session. RiverLine security calls it ghosting.");
	// Dee's game: Tuesdays, Thursdays and Saturdays, doors at nine, the last hand at five.
	Add("dee-game", Kind::Game, "Dee's game", "Spin Cycle Laundromat, the back room", 6.0, 0.0, 0.0, 0.0, 12.0, 0.0, 0.0, 21 * 60, 3 * 60, 0xf2a541,
		"One-two no-limit behind the dryers. $40 to sit, $200 max. Dee deals, the regulars talk, and you can read every one of them.");
	L.back().Days = (1 << 1) | (1 << 3) | (1 << 5);
	// The Embercrest's card room: its desk is open from three hours before the noon game to the turbo's last call
	// (each event's own window is live::CanRegister's).
	Add("embercrest", Kind::Live, "The Embercrest", "Embercrest Casino, the card room", 8.0, 0.0, 0.0, 0.0, 25.0, 0.0, 0.0, 9 * 60, 23 * 60 + 10, 0x5fb4ff,
		"The city's card room: two or three 6-max freezeouts a day, sixty to a hundred and twenty runners, deep stacks. The 14 bus, twenty minutes.");
	// Sleep.
	Add("nap", Kind::Sleep, "Nap", "Bed", 4.0, 0.0, 0.0, 0.0, -45.0, 0.0, 0.0, 0, 1440, 0x8b5cf6, "Four hours. Enough to function.");
	Add("sleep", Kind::Sleep, "Sleep", "Bed", 8.0, 0.0, 0.0, 0.0, -90.0, 0.0, 0.0, 0, 1440, 0x8b5cf6, "A real night's sleep. The world keeps going without you.");
	return L;
}
} // namespace life_detail

using namespace life_detail;

const std::vector<Activity>& Catalog()
{
	static const std::vector<Activity> L = Build();
	return L;
}

const Activity* Find(const std::string& Id)
{
	for (const Activity& A : Catalog())
	{
		if (A.Id == Id)
		{
			return &A;
		}
	}
	return nullptr;
}

bool InWindow(const Activity& A, double World)
{
	const double T = TimeOfDay(World);
	const double O = static_cast<double>(A.Opens);
	const double C = static_cast<double>(A.Closes);
	const bool bOpen = O <= C ? (T >= O && T < C) : (T >= O || T < C);
	if (!bOpen || A.Days == 0x7f)
	{
		return bOpen;
	}
	// Past midnight, a window that wraps still belongs to the day it opened (Day 0 is a Monday).
	int Day = static_cast<int>(std::floor(World / MinutesInDay));
	if (O > C && T < C)
	{
		--Day;
	}
	const int Weekday = ((Day % 7) + 7) % 7;
	return ((A.Days >> Weekday) & 1) != 0;
}

double NextOpen(const Activity& A, double World)
{
	if (InWindow(A, World))
	{
		return World;
	}
	const double T = TimeOfDay(World);
	const double O = static_cast<double>(A.Opens);
	double At = World + (O > T ? O - T : MinutesInDay - T + O);
	for (int Guard = 0; Guard < 8 && !InWindow(A, At); ++Guard)
	{
		At += MinutesInDay;
	}
	return At;
}

void State::Record(double At, const std::string& Label, Chips Amount, int K)
{
	LedgerEntry E;
	E.At = At;
	E.Label = Label;
	E.Amount = Amount;
	E.Kind = K;
	Ledger.insert(Ledger.begin(), E);
	if (Ledger.size() > 60)
	{
		Ledger.resize(60);
	}
}

int State::TicketsFor(const std::string& TemplateId) const
{
	const auto It = Tickets.find(TemplateId);
	return It == Tickets.end() ? 0 : It->second;
}

std::string Blocked(const Activity& A, const State& L, const Context& Ctx)
{
	if (Ctx.InTournament)
	{
		return "Finish your tournament first.";
	}
	if (A.Type == Kind::Sleep)
	{
		return L.Energy >= 92.0 ? "You're wide awake." : "";
	}
	if (A.Type == Kind::Live)
	{
		// The first of the coming events the player could enter now, or why the soonest one is out of reach.
		std::string Why = "Nothing on at the Embercrest.";
		for (const live::Occurrence& O : live::Reachable(Ctx.World, 30.0))
		{
			const std::string Not = live::CanRegister(Ctx.Bankroll, L, O, Ctx.World);
			if (Not.empty())
			{
				return L.Energy < A.Energy ? "A tournament is a long night. Sleep first." : "";
			}
			if (Why == "Nothing on at the Embercrest.")
			{
				Why = Not;
			}
		}
		return Why;
	}
	if (A.Type == Kind::Game)
	{
		const Chips Need = GameMinBuyInCents;
		if (Ctx.Bankroll < Need)
		{
			return "Dee's game is " + Money(Need) + " to sit.";
		}
		if (L.Energy < A.Energy)
		{
			return "Too tired to sit at a live game. Sleep first.";
		}
		if (!InWindow(A, Ctx.World))
		{
			const double At = NextOpen(A, Ctx.World);
			const int Day = static_cast<int>(std::floor(At / MinutesInDay));
			const int Today = static_cast<int>(std::floor(Ctx.World / MinutesInDay));
			return std::string(Day == Today ? "Tonight " : net::DateLabel(Day) + ", ") + ClockString(TimeOfDay(At));
		}
		return "";
	}
	if (A.Id == "marcus-drop" || A.Id == "marcus-run")
	{
		if (L.DebtCents > 0)
		{
			return "Marcus wants his " + Money(L.DebtCents) + " first.";
		}
		if (A.Id == "marcus-run" && L.Runs < 2)
		{
			return "Marcus doesn't trust you with this yet. Do two drop-offs.";
		}
	}
	if (A.Type == Kind::Ghost && Ctx.Cashes < 1)
	{
		return "Sam only hires players who cash. Cash in a tournament.";
	}
	if (A.Type == Kind::Ghost && Ctx.World < L.BannedUntil)
	{
		return "Your RiverLine account is under review.";
	}
	if (L.Energy < A.Energy * 0.6)
	{
		return "Too tired. Sleep first.";
	}
	if (!InWindow(A, Ctx.World))
	{
		const double At = NextOpen(A, Ctx.World);
		return "Next start " + ClockString(TimeOfDay(At));
	}
	return "";
}

double NeedsDrain(const State& L)
{
	return std::max(0.0, L.Hunger - 70.0) / 30.0 * 2.5 + std::max(0.0, L.Thirst - 70.0) / 30.0 * 3.0;
}

const char* HungerWord(double Hunger)
{
	return Hunger >= 85.0 ? "Starving" : Hunger >= 65.0 ? "Hungry" : Hunger >= 40.0 ? "Peckish" : "Fed";
}

const char* ThirstWord(double Thirst)
{
	return Thirst >= 85.0 ? "Parched" : Thirst >= 65.0 ? "Thirsty" : Thirst >= 40.0 ? "Dry" : "Hydrated";
}

double RiskOf(const Activity& A, const State& L)
{
	switch (A.Type)
	{
	case Kind::Hustle: return std::min(0.85, std::max(0.0, A.Risk + L.Heat * 0.004 + L.Perks.HustleRisk));
	case Kind::Ghost: return std::min(0.85, A.Risk + 0.06 * static_cast<double>(L.Ghosts));
	default: return 0.0;
	}
}

Outcome Resolve(const Activity& A, const State& L, double Start, Rng& R, Chips Bankroll)
{
	Outcome O;
	O.ActivityId = A.Id;
	O.Start = Start;
	O.End = Start + A.Hours * 60.0;
	O.Energy = -A.Energy;
	const Chips Flat = A.PayMax > A.PayMin ? A.PayMin + static_cast<Chips>(std::llround(R.Next() * static_cast<double>(A.PayMax - A.PayMin))) : A.PayMin;
	switch (A.Type)
	{
	case Kind::Job:
	{
		// A line cook knows how to work a shift: the wage and the tips both go further.
		const Chips Wage = static_cast<Chips>(std::llround(static_cast<double>(A.WageCents) * A.Hours * L.Perks.JobPay));
		const Chips Extra = static_cast<Chips>(std::llround(static_cast<double>(Flat) * L.Perks.JobPay));
		O.Money = Wage + Extra;
		O.Title = "Shift done";
		O.Body = std::to_string(static_cast<int>(A.Hours)) + " hours at " + A.Place + ". " + Money(Wage) + " in wages" + (Extra > 0 ? " and " + Money(Extra) + (A.Id == "dashdrop" ? " in tips." : " on the side.") : ".");
		break;
	}
	case Kind::Hustle:
		if (R.Chance(RiskOf(A, L)))
		{
			// Picked up: the package is gone, the fine is due, the night is spent in holding.
			O.Bad = true;
			O.End += 8.0 * 60.0;
			O.Money = -std::min<Chips>(25000, Bankroll);
			O.Debt = A.Id == "marcus-run" ? 60000 : 20000;
			O.Heat = -L.Heat * 0.5;
			O.Title = "Picked up";
			O.Body = "A routine stop that wasn't. The bag is in an evidence locker, you spent the night in holding, and the fine is " + Money(-O.Money) + ". Marcus says you owe him " + Money(O.Debt) +
				" for the package.";
		}
		else
		{
			O.Money = Flat;
			O.Heat = A.Heat;
			O.Title = "Delivered";
			O.Body = A.Id == "marcus-run" ? "Five hours, two cities, no stops. Marcus counts it out twice and hands you " + Money(Flat) + "."
										  : "Tre takes the bag without a word. Marcus sends " + Money(Flat) + " and a thumbs up.";
		}
		break;
	case Kind::Ghost:
		if (R.Chance(RiskOf(A, L)))
		{
			O.Bad = true;
			O.BanUntil = O.End + 24.0 * 60.0;
			O.Title = "Account flagged";
			O.Body = "RiverLine security matched your login to Sam's account. Both accounts are restricted for 24 hours while they review it. Sam isn't paying for a session he lost.";
		}
		else
		{
			O.Money = Flat;
			O.Title = "Session played";
			O.Body = "Three hours as whale_sam. He's up on the night and thinks he played great. He sends " + Money(Flat) + ".";
		}
		break;
	case Kind::Sleep:
		O.Title = A.Hours >= 8.0 ? "Slept" : "Napped";
		O.Body = A.Hours >= 8.0 ? "Eight hours. The rain never stopped." : "Four hours on top of the covers.";
		break;
	case Kind::Game:
	case Kind::Live:
		// Played out at the table by the host, never resolved here.
		break;
	}
	return O;
}

Chips MysteryEnvelope(Chips BountyCents, Rng& R)
{
	// Mostly small, sometimes huge: the envelopes average about the bounty part of the buy-in.
	struct Tier
	{
		double Multiple;
		double Weight;
	};
	static const Tier Tiers[7] = {{0.4, 46.0}, {0.8, 26.0}, {1.5, 14.0}, {3.0, 8.0}, {8.0, 4.4}, {40.0, 1.3}, {250.0, 0.3}};
	double Total = 0.0;
	for (const Tier& T : Tiers)
	{
		Total += T.Weight;
	}
	double Pick = R.Next() * Total;
	for (const Tier& T : Tiers)
	{
		if ((Pick -= T.Weight) < 0.0)
		{
			return static_cast<Chips>(std::llround(static_cast<double>(BountyCents) * T.Multiple));
		}
	}
	return BountyCents;
}

double NightShiftStart(double World)
{
	const double DayStart = std::floor(World / MinutesInDay) * MinutesInDay;
	return TimeOfDay(World) >= 18.0 * 60.0 ? DayStart + 18.0 * 60.0 : DayStart - 6.0 * 60.0;
}

double Daylight(double World)
{
	const double H = TimeOfDay(World) / 60.0;
	auto Smooth = [](double A, double B, double X) {
		const double T = std::min(1.0, std::max(0.0, (X - A) / (B - A)));
		return T * T * (3.0 - 2.0 * T);
	};
	return H < 12.0 ? Smooth(4.6, 6.3, H) : 1.0 - Smooth(18.5, 20.0, H);
}
} // namespace life
} // namespace ss
