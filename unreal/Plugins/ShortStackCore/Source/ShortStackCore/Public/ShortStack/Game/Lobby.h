#pragma once

#include "ShortStack/Tournament.h"

namespace ss
{
/** A tournament in the RiverLine lobby tonight. Port of web/src/game/events.ts. */
struct LobbyEvent
{
	bool Joinable = false; // false: flavor listing you can't join tonight
	TournamentSpec Spec;
	std::string Name;
	std::string Start;
	std::string BuyInLabel;
	Chips BuyInCents = 0;
	std::string Game;
	std::string Speed;
	std::string EntrantsLabel;
	std::string GuaranteeLabel;
	std::string Status; // "Late Reg", "Registering", "Starts Sun", "Running", "Tomorrow"
	std::string Blurb;
};

const std::vector<LobbyEvent>& Lobby();
} // namespace ss
