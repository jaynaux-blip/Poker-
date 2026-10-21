// The shared trademark screen (update 3 SPEC §1.10, §7.7): real names that never appear in the game, in one place, so the
// suites' copies can't drift (they replace sponsor_main.cpp's, sites_main.cpp's and video_main.cpp's own lists). Strings
// live only here, never in copy. Two tiers, both matched case-insensitively on whole words (no letter or digit on either
// side of the match, so "acr" is never "across" and "ept" never "accepted"):
//   - kSourceScan: real names that are no ordinary word. rival_sites' SitesFormatNames screens every string literal in the
//     core and the game module with it, as well as every name and inventory the other screens cover.
//   - kCopyScan: real names that are also ordinary words or fragments ("kick", "shorts", "bumper"), screened only over the
//     copy inventories (site::Inventory(), video::Inventory(), the sponsor catalog, the UI inventories and, as they land,
//     Ignite's, RunItTwice's, site::play's and video::InventoryU3()), where nothing uses them in their ordinary sense.
// Each suite keeps beside these only what is its own: the sponsor screen's drinks and gear brands ("monster" is an ordinary
// word in the game), the sites' screen's ten closed sites the world tells of.
//
// Use: if (const char* Hit = trademarks::Find(Text, trademarks::kSourceScan)) ... The grandfather list (kGrandfathered) holds
// literals a later gate's review keeps on purpose; it is empty at Phase A. Header-only; every test executable may include it.
#pragma once

#include <cctype>
#include <cstddef>
#include <string>

namespace trademarks
{
/** Real sites, platforms and wallets that are no ordinary word, real formats, series, products and tools, real creators. */
inline constexpr const char* kSourceScan[] = {
	// Sites, platforms, wallets.
	"pokerstars", "ggpoker", "partypoker", "888poker", "run it once", "americas cardroom", "natural8", "bovada", "pokerbros", "global poker", "acr poker",
	"betonline", "coinpoker", "winamax", "unibet", "wpt global", "youtube", "tiktok", "vimeo", "dailymotion", "skrill", "neteller", "paypal", "venmo", "ecopayz",
	"muchbetter",
	// Formats, series, products and tools ("Bounty Builder" and "Monster Stack" were RiverLine's until Phase A's renames).
	"spin & go", "spin and go", "spin&go", "spin & gold", "flip & go", "flip and go", "flip&go", "twister", "jackpot sit & go", "jackpot sit", "jackpot sng",
	"expresso", "zone poker", "zoom poker", "rush & cash", "all-in or fold", "fish buffet", "power path", "stars rewards", "ignition rewards", "ignition miles",
	"splash the pot", "pokercraft", "smart hud", "battle royale", "mystery battle", "bounty builder", "monster stack", "sunday million", "triton", "pokergo",
	"hold'em manager", "holdem manager", "pokertracker", "gto wizard", "youtube studio", "trueview", "content id", "shorts fund", "test & compare",
	"super thanks",
	// Real poker creators (concept_rerun_platform.md §1.3; PokerGO and Triton above; Rampage, Wolfgang and Mariano, an ordinary
	// word and two first names, in kCopyScan).
	"brad owen", "andrew neeme", "doug polk", "jonathan little", "lex veldhuis", "spraggy", "hungry horse", "jaman burton", "johnnie vibes", "kevin martin",
	"joey ingram", "alexandra botez", "hustler casino live", "lodge live"};

/** Real names that are also ordinary words or fragments: screened over the copy inventories only. */
inline constexpr const char* kCopyScan[] = {"kick", "rumble", "twitch", "888", "wsop", "world series", "wpt", "upswing", "bodog", "ignition", "shorts", "premieres",
	"backstage", "run good", "rungood", "streak freeze", "battle pass", "bumper", "ept", "acr", "supernova", "rampage", "wolfgang", "mariano"};

/** Literals a gate's review keeps on purpose despite a kSourceScan word (each with its reason); empty at Phase A. */
inline constexpr const char* kGrandfathered[] = {""};

/** ASCII lower case. */
inline std::string Lower(std::string S)
{
	for (char& C : S)
	{
		C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
	}
	return S;
}

/** Whether Word (lower case) is in Text as a whole word: no letter or digit just before or just after it. */
inline bool HasWord(const std::string& LowText, const std::string& Word)
{
	if (Word.empty())
	{
		return false;
	}
	for (size_t At = LowText.find(Word); At != std::string::npos; At = LowText.find(Word, At + 1))
	{
		const bool Before = At == 0 || !std::isalnum(static_cast<unsigned char>(LowText[At - 1]));
		const size_t End = At + Word.size();
		const bool After = End >= LowText.size() || !std::isalnum(static_cast<unsigned char>(LowText[End]));
		if (Before && After)
		{
			return true;
		}
	}
	return false;
}

/** The first of a tier's names in Text (whole words, any case), or nullptr; Skip (lower case) is passed over. */
template <size_t N>
inline const char* Find(const std::string& Text, const char* const (&Tier)[N], const char* Skip = nullptr)
{
	const std::string Low = Lower(Text);
	for (const char* W : Tier)
	{
		if (Skip && std::string(W) == Skip)
		{
			continue;
		}
		if (HasWord(Low, W))
		{
			return W;
		}
	}
	return nullptr;
}

/** Whether a literal is on the grandfather list. */
inline bool Grandfathered(const std::string& Literal)
{
	for (const char* G : kGrandfathered)
	{
		if (*G && Literal == G)
		{
			return true;
		}
	}
	return false;
}
} // namespace trademarks
