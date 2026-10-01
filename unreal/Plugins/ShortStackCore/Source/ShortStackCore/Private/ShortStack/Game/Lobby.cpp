#include "ShortStack/Game/Lobby.h"
#include "../StrictFloat.h"

namespace ss
{
namespace lobby_detail
{
TournamentSpec Base(const char* Id, const char* Name, Chips BuyIn, Chips Fee, Chips Guarantee, int Entrants, double LevelMinutes, const char* Population, const char* Speed)
{
	TournamentSpec S;
	S.Id = Id;
	S.Name = Name;
	S.BuyInCents = BuyIn;
	S.FeeCents = Fee;
	S.GuaranteeCents = Guarantee;
	S.Entrants = Entrants;
	S.StartingStack = 10000;
	S.LevelMinutes = LevelMinutes;
	S.SecondsPerHand = 42.0;
	S.Population = Population;
	S.Speed = Speed;
	S.StartClock = 2.0 * 60.0 + 11.0;
	S.TableSize = 9;
	return S;
}

LobbyEvent Listing(const char* Name, const char* Start, const char* BuyInLabel, Chips BuyInCents, const char* Game, const char* Speed, const char* Entrants, const char* Guarantee, const char* Status)
{
	LobbyEvent E;
	E.Name = Name;
	E.Start = Start;
	E.BuyInLabel = BuyInLabel;
	E.BuyInCents = BuyInCents;
	E.Game = Game;
	E.Speed = Speed;
	E.EntrantsLabel = Entrants;
	E.GuaranteeLabel = Guarantee;
	E.Status = Status;
	return E;
}

std::vector<LobbyEvent> Build()
{
	std::vector<LobbyEvent> L;
	LobbyEvent Owl = Listing("Night Owl Turbo", "Now", "$1.10", 110, "NL Hold'em", "Turbo \xC2\xB7 5 min", "1,000", "$1,000 GTD", "Late Reg");
	Owl.Joinable = true;
	Owl.Spec = Base("night-owl", "$1.10 Night Owl Turbo", 110, 10, 100000, 1000, 5.0, "micro", "Turbo");
	Owl.Blurb = "The big one tonight. 1,000 grinders, 150 paid, $178 to the winner.";
	L.push_back(Owl);

	LobbyEvent Hyper = Listing("Hyper Sprint", "Now", "$0.25", 25, "NL Hold'em", "Hyper \xC2\xB7 3 min", "180", "$41.40 pool", "Late Reg");
	Hyper.Joinable = true;
	Hyper.Spec = Base("hyper-sprint", "$0.25 Hyper Sprint", 25, 2, 0, 180, 3.0, "micro", "Hyper");
	Hyper.Blurb = "Quick and wild. 180 players, blinds up every 3 minutes.";
	L.push_back(Hyper);

	LobbyEvent Free = Listing("Midnight Freeroll", "Now", "Free", 0, "NL Hold'em", "Turbo \xC2\xB7 4 min", "1,000", "$50 pool", "Late Reg");
	Free.Joinable = true;
	Free.Spec = Base("freeroll", "Midnight Freeroll", 0, 0, 5000, 1000, 4.0, "freeroll", "Turbo");
	Free.Blurb = "Free to enter. Everyone shoves. Somebody has to win the $9.";
	L.push_back(Free);

	L.push_back(Listing("Bounty Builder", "4:00 AM", "$5.50", 550, "NL Hold'em PKO", "Regular", "612", "$3K GTD", "Registering"));
	L.push_back(Listing("Big Stack $11", "8:00 PM", "$11", 1100, "NL Hold'em", "Regular", "\xE2\x80\x94", "$10K GTD", "Tomorrow"));
	L.push_back(Listing("Sunday Showdown", "Sun 3:00 PM", "$215", 21500, "NL Hold'em", "Regular", "\xE2\x80\x94", "$1M GTD", "Starts Sun"));
	L.push_back(Listing("Step 1 \xC2\xB7 Grand Circuit", "5:30 AM", "$2.20", 220, "Satellite", "Turbo", "88", "8 tickets", "Registering"));

	// Joinable events added with the network schedule (Network.cpp maps its events here by index).
	LobbyEvent Insomniac = Listing("Insomniac Freeroll", "3:30 AM", "Free", 0, "NL Hold'em", "Turbo \xC2\xB7 4 min", "800", "$30 pool", "Registering");
	Insomniac.Joinable = true;
	Insomniac.Spec = Base("insomniac", "Insomniac Freeroll", 0, 0, 3000, 800, 4.0, "freeroll", "Turbo");
	Insomniac.Spec.StartClock = 3.5 * 60.0;
	Insomniac.Blurb = "For the ones still up. Free, fast, and $30 to fight over.";
	L.push_back(Insomniac);

	LobbyEvent Crawler = Listing("MM #26: Night Crawler", "1:30 AM", "$2.20", 220, "NL Hold'em", "Turbo \xC2\xB7 5 min", "1,184", "$2K GTD", "Late Reg");
	Crawler.Joinable = true;
	Crawler.Spec = Base("mm-26", "MM #26: Night Crawler", 220, 20, 200000, 1184, 5.0, "micro", "Turbo");
	Crawler.Blurb = "A Micro Madness title for $2.20. Late registration is open until 3:10 AM.";
	L.push_back(Crawler);
	return L;
}
} // namespace lobby_detail

const std::vector<LobbyEvent>& Lobby()
{
	static const std::vector<LobbyEvent> L = lobby_detail::Build();
	return L;
}
} // namespace ss
